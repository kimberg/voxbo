
// vbri.cpp
// Copyright (c) 2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Dan Kimberg

// TODO
// keyboard brightness/contrast adjustment in vbview
// keyboard show/hide lesion

#include <QAction>
//#include <QMenu>
//#include <QMenuBar>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <iostream>

#include "vbview.h"
#include "vbri.moc.h"

// const string headerstyle1="color:red;";

bool maskcmp(const VBmask &m1,const VBmask &m2);
void find_origin(Cube &cb);
string nextcheckpointname();
string nextmaskname();

using namespace std;
using boost::format;

int
main(int argc,char **argv)
{
  // Qt application
  QApplication app(argc,argv);
  // parse arguments (none so far)
  tokenlist args;
  string parsefile;
  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-p" && i<args.size()-1) {
      parsefile=args[++i];
    }
  }
  if (parsefile.size()) {
    cout << "[I] vbri: parse mode\n";
    VBri *mw=new VBri;
    mw->process(parsefile);
    exit(0);
  }
  // create a db main window inside a qt main window
  VBri *mw=new VBri;
  mw->show();
  return app.exec();
}

VBri::VBri()
{
  setGeometry(150,150,800,600);
  myframe=new QFrame;
  setCentralWidget(myframe);
  // a_openconfig=new QAction("Open config file",this);

  // QObject::connect(a_openconfig,SIGNAL(triggered()),this,SLOT(openconfig()));

  // w_menubar=menuBar();
  // setMenuBar(w_menubar);

  // filemenu=w_menubar->addMenu("Your only menu");
  // filemenu->addAction(a_openconfig);

  // w_menubar->show();
  // filemenu->show();

  setup_widgets();
  arrangeChildren();
  setFocusPolicy(Qt::StrongFocus);
  outfile="vbri.log";
  openconfig();
  postprocessconfig();
}

void
VBri::closeEvent(QCloseEvent *ce)
{
  writeAll(nextcheckpointname());
  ce->accept();
}

void
VBri::resizeEvent(QResizeEvent *re)
{
  QMainWindow::resizeEvent(re);
  arrangeChildren();
}

void
VBri::keyPressEvent(QKeyEvent *ke)
{
  if (ke->ascii()=='1')
    w_rating->setValue(1);
  else if (ke->ascii()=='2')
    w_rating->setValue(2);
  else if (ke->ascii()=='3')
    w_rating->setValue(3);
  else if (ke->ascii()=='4')
    w_rating->setValue(4);
  else if (ke->ascii()=='5')
    w_rating->setValue(5);
  else if (ke->ascii()=='6')
    w_rating->setValue(6);
  else if (ke->ascii()=='7')
    w_rating->setValue(7);
  else if (ke->ascii()=='0')
    w_rating->setValue(0);
  else if (ke->key()==Qt::Key_Left || ke->ascii()=='p')
    handlePrev();
  else if (ke->key()==Qt::Key_Right || ke->ascii()=='n')
    handleNext();
}

int
VBri::openconfig()
{
  configfile=QFileDialog::getOpenFileName(this,"select config file").toStdString();
  if (!configfile.size()) return 1;
  return openconfig(configfile);
}

int
VBri::openconfig(string configfile)
{
  ifstream infile;
  infile.open(configfile.c_str());
  if (!infile) {
    QMessageBox::critical(this,"Error","Couldn't open config file.");
    return 2;
  }
  chdir(xdirname(configfile).c_str());

  char buf[STRINGLEN];
  tokenlist args;
  while (infile.getline(buf,STRINGLEN,'\n')) {
    args.ParseLine(buf);
    if (args.size()==0) continue;
    if (args[0][0]=='#' || args[0][0] == ';')
      continue;
    if (args[0]=="image" && args.size()==3) {
      Cube cb;
      if (cb.ReadFile(args[2])) {
        QMessageBox::critical(this,"Error",(format("Couldn't open image %s")%args[2]).str().c_str());
        infile.close();
        return 3;
      }
      find_origin(cb);
      cubelist.push_back(cb);
      cubemap[args[1]]=--cubelist.end();
    }
    else if (args[0]=="left") {
      leftimg=args[1];
    }
    else if (args[0]=="right") {
      rightimg=args[1];
    }
    else if (args[0]=="mask") {
      VBmask vbm;
      vbm.imgname=args[1];
      if (args.size()>2) {
        vbm.rating=strtol(args[2]);
        vbm.xslice=strtol(args[3]);
        vbm.yslice=strtol(args[3]);
        vbm.zslice=strtol(args[3]);
        vbm.notes=args.Tail(4);
      }
      else if (cubemap.count(vbm.imgname)) {
        // FIXME get it from the header...
        vbm.xslice=cubemap[vbm.imgname]->origin[0];
        vbm.yslice=cubemap[vbm.imgname]->origin[1];
        vbm.zslice=cubemap[vbm.imgname]->origin[2];
      }
      masklist.push_back(vbm);
    }
    else if (args[0]=="outfile") {
      if (args.size()>1)
        outfile=args[1];
    }
    else if (args[0]=="randomize") {
      randomizemasks();
    }
    else {
      QMessageBox::critical(this,"Error",((string)"Bad config line: "+buf).c_str());
      infile.close();
      return 4;
    }
  }
  infile.close();
  return 0;
}

void
VBri::process(string configfile)
{
  if (openconfig(configfile)) {
    printf("[E] vbri: config file error\n");
    return;
  }
  vector<int> wratings;
  vector<int> wpratings;
  vector<int> weightedwratings;
  vector<int> weightedwpratings;
  uint32 totalwvoxels=0;
  uint32 totalwpvoxels=0;
  uint32 totalwratings=0;
  uint32 totalwpratings=0;
  uint32 totalweightedwratings=0;
  uint32 totalweightedwpratings=0;

  vbforeach(VBmask mm,masklist) {
    if (mm.rating==0) {
      cout << format("[I] vbri: rating 0 found in %s\n")%mm.imgname;
      continue;
    }
    uint32 cnt=cubemap[mm.imgname]->count();
    cout << "count: " << cnt << endl;
    if (mm.imgname.substr(0,2)=="w_") {
      wratings.push_back(mm.rating);
      weightedwratings.push_back(mm.rating*cnt);
      totalwratings+=mm.rating;
      totalweightedwratings+=mm.rating*cnt;
      totalwvoxels+=cnt;
    }
    else if (mm.imgname.substr(0,3)=="wp_") {
      wpratings.push_back(mm.rating);
      weightedwpratings.push_back(mm.rating*cnt);
      totalwpratings+=mm.rating;
      totalweightedwpratings+=mm.rating*cnt;
      totalwpvoxels+=cnt;
    }
    else {
      cout << "[E] vbri: not w or wp: " << mm.imgname << endl;
    }
  }
  cout << format("[I] vbri: mean w rating %g (%d pieces)\n")
    %((double)totalwratings/(double)wratings.size())%wratings.size();
  cout << format("[I] vbri: mean wp rating %g (%d pieces)\n")
    %((double)totalwpratings/(double)wpratings.size())%wpratings.size();
  cout << format("[I] vbri: mean weighted w rating %g (%d voxels)\n")
    %((double)totalweightedwratings/(double)totalwvoxels)%totalwvoxels;
  cout << format("[I] vbri: mean weighted wp rating %g (%d voxels)\n")
    %((double)totalweightedwpratings/(double)totalwpvoxels)%totalwpvoxels;
}


void
VBri::postprocessconfig()
{
  w_view1->layers.clear();
  w_view2->layers.clear();
  if (leftimg.size()) {
    if (w_view1->SetImage(*(cubemap[leftimg]))) {
      QMessageBox::critical(this,"Error","Bad img file.");
      return;
    }
  }
  if (rightimg.size()) {
    if (w_view2->SetImage(*(cubemap[rightimg]))) {
      QMessageBox::critical(this,"Error","Bad img file.");
      return;
    }
  }
  w_index->setRange(1,masklist.size());
  maskptr=masklist.begin();
  currentindex=1;
  updateRating();
}

void
find_origin(Cube &cb)
{
  cb.origin[0]=cb.dimx/2;
  cb.origin[1]=cb.dimy/2;
  cb.origin[2]=cb.dimz/2;
  for (int k=0; k<cb.dimz; k++) {
    for (int j=0; j<cb.dimy; j++) {
      for (int i=0; i<cb.dimx; i++) {
        if (cb.testValue(i,j,k)) {
          cb.origin[0]=i;
          cb.origin[1]=j;
          cb.origin[2]=k;
          return;
        }
      }
    }
  }
  return;
}

void
VBri::setup_widgets()
{
  // LOGO
  w_logo=new QLabel(myframe);
  w_logo->setPixmap(QPixmap(":/icons/a_logo.png"));
  w_logo->show();
  // DB BANNER
  w_banner=new QLabel(myframe);
  w_banner->setText("Welcome to VBrateimage!");
  w_banner->show();

  // VBVIEWS
  w_view1=new VBView(myframe);
  w_view2=new VBView(myframe);
  w_view1->setMinimal(1);
  w_view2->setMinimal(1);

  // BOXES
  w_ratebox=makeratebox();
  w_navbox=makenavbox();
  w_detailbox=makedetailbox();
}

void
VBri::displayrating(int num)
{
  if (num<0 || num>7) {
    w_ratinglabel1->setText("X");
    w_ratinglabel2->setText("invalid");
    return;
  }
  w_ratinglabel1->setText(strnum(num).c_str());
  if (num==0)
    w_ratinglabel2->setText("unrated");
  else if (num==1)
    w_ratinglabel2->setText("definitely intact");
  else if (num==2)
    w_ratinglabel2->setText("probably intact");
  else if (num==3)
    w_ratinglabel2->setText("best guess is intact");
  else if (num==4)
    w_ratinglabel2->setText("neutral");
  else if (num==5)
    w_ratinglabel2->setText("best guess is lesioned");
  else if (num==6)
    w_ratinglabel2->setText("probably lesioned");
  else if (num==7)
    w_ratinglabel2->setText("definitely lesioned");
}

void
VBri::handleRating(int r)
{
  displayrating(r);
  // w_ratinglabel->setText(strnum(r).c_str());
  if (maskptr!=masklist.end())
    maskptr->rating=r;
}

void
VBri::handleNotes()
{
  if (maskptr!=masklist.end())
    maskptr->notes=w_ratingnotes->text().toStdString();
}

void
VBri::handleIndex(int ind)
{
  if (masklist.size()==0) return;
  writeCurrent();
  currentindex=ind;
  maskptr=masklist.begin();
  maskptr+=ind-1;
  updateRating();
}

void
VBri::handleFirst()
{
  writeCurrent();
  if (masklist.size()==0) return;
  maskptr=masklist.begin();
  currentindex=1;
  updateRating();
}

void
VBri::handlePrev()
{
  if (currentindex>0)
    w_index->setValue(currentindex-1);
  return;
                       
  writeCurrent();
  if (masklist.size()==0) return;
  if (maskptr==masklist.begin())
    return;
  maskptr--;
  currentindex--;
  updateRating();
}

void
VBri::handleNext()
{
  if (currentindex<masklist.size())
    w_index->setValue(currentindex+1);
  return;

  writeCurrent();
  if (masklist.size()==0) return;
  maskptr++;
  if (maskptr==masklist.end()) {maskptr--;return;}
  currentindex++;
  updateRating();
}

void
VBri::handleLast()
{
  writeCurrent();
  if (masklist.size()==0) return;
  maskptr=masklist.end();
  maskptr--;
  currentindex=masklist.size();
  updateRating();
}

void
VBri::handleSplit()
{
  Cube highcube,lowcube;
  lowcube=*(cubemap[maskptr->imgname]);
  highcube=lowcube;
  int zslice=w_view1->q_zslice;
  int lowvoxels=0,highvoxels=0;
  for (int i=0; i<lowcube.dimx; i++) {
    for (int j=0; j<lowcube.dimy; j++) {
      for (int k=0; k<lowcube.dimz; k++) {
        if (!highcube.testValue(i,j,k)) // could test either one
          continue;
        if (k<=zslice) {
          highcube.SetValue(i,j,k,0);
          lowvoxels++;
        }
        else {
          lowcube.SetValue(i,j,k,0);
          highvoxels++;
        }
      }
    }
  }
  if (lowvoxels==0 || highvoxels==0) {
    lowcube.invalidate();
    highcube.invalidate();
    QMessageBox::critical(this,"Error","One of your split masks is empty.");
    return;
  }
  VBmask highmask,lowmask;
  string lowfname=nextmaskname();
  string highfname=nextmaskname();
  string lowname=lowfname.substr(0,lowfname.find("."));
  string highname=highfname.substr(0,highfname.find("."));
  // find first voxels, write the cubes and add to img list
  find_origin(lowcube);
  find_origin(highcube);
  // FIXME add headers to lowcube and highcube indicating where they
  // came from
  lowcube.WriteFile(lowfname);
  highcube.WriteFile(highfname);
  cubelist.push_back(lowcube);
  cubemap[lowname]=--cubelist.end();
  cubelist.push_back(highcube);
  cubemap[highname]=--cubelist.end();
  cubemap.erase(maskptr->imgname);
  // make the mask records
  lowmask.imgname=lowname;
  lowmask.rating=0;
  lowmask.xslice=w_view1->q_xslice;
  lowmask.yslice=w_view1->q_yslice;
  lowmask.zslice=w_view1->q_zslice;
  highmask.imgname=highname;
  highmask.rating=0;
  highmask.xslice=highcube.origin[0];
  highmask.yslice=highcube.origin[1];
  highmask.zslice=highcube.origin[2];

  maskptr=masklist.insert(maskptr,lowmask);
  maskptr++;
  *maskptr=highmask;
  maskptr--;
  w_index->setRange(1,masklist.size());
  updateRating();
}

void
VBri::updateRating()
{
  if (!masklist.size()) return;
  if (w_view1->layers.size()==2)
    w_view1->layers.pop_back();
  w_view1->SetMask(*(cubemap[maskptr->imgname]));
  // scroll img to correct location for this lesion
  w_view1->NewXSlice(maskptr->xslice);
  w_view1->NewYSlice(maskptr->yslice);
  w_view1->NewZSlice(maskptr->zslice);
  // make sure the vbview origin is set to the lesion origin
  w_view1->base_origin[0]=cubemap[maskptr->imgname]->origin[0];
  w_view1->base_origin[1]=cubemap[maskptr->imgname]->origin[1];
  w_view1->base_origin[2]=cubemap[maskptr->imgname]->origin[2];
  w_rating->setSliderPosition(maskptr->rating);
  w_ratingnotes->setText(maskptr->notes.c_str());
  string tmp=(format("mask %d of %d")%currentindex%masklist.size()).str();
  w_maskname->setText(tmp.c_str());
}

void
VBri::handleDone()
{
  // FIXME -- write ALL the data to the data file
}

void
VBri::writeCurrent()
{
  if (maskptr==masklist.end())
    return;
  maskptr->xslice=w_view1->q_xslice;
  maskptr->yslice=w_view1->q_yslice;
  maskptr->zslice=w_view1->q_zslice;
  ofstream ofile(outfile.c_str(),ios::app);
  if (!ofile)
    return;
  ofile << format("%s: %s (%s) %d %s\n")%timedate() % maskptr->imgname
    % cubemap[maskptr->imgname]->filename
    % maskptr->rating%maskptr->notes;
  ofile.close();
}

void
VBri::writeAll(string fname)
{
  if (masklist.size()==0)
    return;
  if (fname=="") fname="checkpoint.conf";
  ofstream ofile(fname.c_str(),ios::app);
  if (!ofile)
    return;
  ofile << "\n# started with config file " << configfile << "\n\n";
  pair<string,list<Cube>::iterator> mmm;
  vbforeach(mmm,cubemap) {
    ofile << "image "+mmm.first+" "+mmm.second->filename << endl;
  }
  ofile << "left " << leftimg << endl;
  ofile << "right " << rightimg << endl;
  ofile << "outfile " << outfile << endl;
  vbforeach(VBmask mm,masklist) {
    ofile << format("mask %s %d %d %d %d %s\n") % mm.imgname % mm.rating
      % mm.xslice % mm.yslice % mm.zslice % mm.notes;
  }
  ofile.close();
}

QVBox *
VBri::makeratebox()
{
  QVBox *vb=new QVBox(myframe);
  QHBox *hb;
  hb=new QHBox(myframe);
  vb->addWidget(hb);

  hb->addLabel("definitely intact (1)");
  w_rating=new QSlider(Qt::Horizontal);
  w_rating->setTickInterval(1);
  w_rating->setTickPosition(QSlider::TicksBelow);
  w_rating->setRange(0,7);
  w_rating->setPageStep(1);
  w_rating->setSingleStep(1);
  w_rating->setSliderPosition(4);
  hb->addWidget(w_rating);
  hb->addLabel("definitely lesioned (7)");

  hb=new QHBox;
  vb->addWidget(hb);
  hb->addStretch(1);
  hb->addLabel("neutral (4)");
  hb->addStretch(1);
  // QObject::connect(w_rating,SIGNAL(sliderMoved(int)),this,SLOT(handleRating(int)));
  QObject::connect(w_rating,SIGNAL(valueChanged(int)),this,SLOT(handleRating(int)));

  return vb;
}


QHBox *
VBri::makenavbox()
{
  QHBox *hb=new QHBox(myframe);
  w_index=new QSlider(Qt::Horizontal);
  w_index->setTickInterval(1);
  w_index->setTickPosition(QSlider::TicksBelow);
  w_index->setRange(1,1);
  w_index->setPageStep(1);
  w_index->setSingleStep(1);
  w_index->setSliderPosition(1);
  hb->addWidget(w_index);
  QObject::connect(w_index,SIGNAL(valueChanged(int)),this,SLOT(handleIndex(int)));
  return hb;
}

QVBox *
VBri::makedetailbox()
{
  QVBox *vb=new QVBox(myframe);
  w_maskname=new QLabel(vb);
  w_maskname->setAlignment(Qt::AlignCenter);
  w_ratinglabel1=new QLabel(vb);
  w_ratinglabel2=new QLabel(vb);
  QFont f=myframe->font();
  f.setPixelSize(90);
  w_ratinglabel1->setFont(f);
  f.setPixelSize(18);
  w_ratinglabel2->setFont(f);
  w_maskname->setFont(f);
  w_ratinglabel1->setAlignment(Qt::AlignCenter);
  w_ratinglabel2->setAlignment(Qt::AlignCenter);
  w_ratinglabel2->setWordWrap(1);
  // w_ratinglabel1->setFrameStyle(QFrame::Panel | QFrame::Raised);
  w_ratingnotes=new QTextEdit(myframe);
  vb->addStretch();
  vb->addWidget(w_maskname);
  vb->addWidget(w_ratinglabel1);
  vb->addWidget(w_ratinglabel2);
  vb->addWidget(w_ratingnotes);
  QPushButton *bb=new QPushButton("Split",myframe);
  vb->addWidget(bb);
  QObject::connect(bb,SIGNAL(clicked()),this,SLOT(handleSplit()));
  vb->addStretch();
  QObject::connect(w_ratingnotes,SIGNAL(textChanged()),this,SLOT(handleNotes()));
  displayrating(0);
  return vb;
}

void
VBri::arrangeChildren()
{
  int pos=0;
  const int SPACING=20;

  // LOGO AT TOP LEFT
  w_logo->adjustSize();
  w_logo->setGeometry(10,pos+10,w_logo->width(),w_logo->height());
  w_logo->show();

  // BANNER AT TOP RIGHT
  w_banner->adjustSize();
  w_banner->setGeometry(myframe->width()-w_banner->width()-10,pos+10,w_banner->width(),w_banner->height());

  pos=w_logo->geometry().bottom();
  if (w_banner->geometry().bottom()>pos) pos=w_logo->geometry().bottom();
  
  pos+=SPACING;

  w_ratebox->adjustSize();
  int wtmp=(myframe->width()-20-150)/2;
  int htmp=myframe->height()-pos-SPACING-SPACING-10-(w_ratebox->height()*2);

  // VIEWS and the stuff in between
  w_view1->setGeometry(10,pos,wtmp,htmp);
  w_view2->setGeometry(10+wtmp+150,pos,wtmp,htmp);
  w_detailbox->setGeometry(10+wtmp,pos,150,htmp);
  
  pos+=htmp+SPACING;
  
  // ratebox and buttons
  w_ratebox->setGeometry(10,pos,wtmp*2,w_ratebox->height());
  pos+=w_ratebox->height();
  w_navbox->setGeometry(10,pos,wtmp*2,w_ratebox->height());
}

string
nextcheckpointname()
{
  string fname;
  for (int i=0; i<100000; i++) {
    fname=(format("check_%03d.conf")%i).str();
    if (!vb_fileexists(fname))
      return fname;
  }
  return (string)"data.dat";
}

string
nextmaskname()
{
  static int i=0;
  string fname;
  for (int xx=0; xx<100000; xx++) {
    fname=(format("splitmask_%03d.nii.gz")%i++).str();
    if (!vb_fileexists(fname))
      return fname;
  }
  return (string)"nomask.nii.gz";
}

void
VBri::randomizemasks()
{
  sort(masklist.begin(),masklist.end(),maskcmp);
}

bool
maskcmp(const VBmask &m1,const VBmask &m2)
{
  if (m1.rnum<m2.rnum)
    return 1;
  return 0;
}
