
// vbviewmain.cpp
// VoxBo discount image viewer
// Copyright (c) 2002-2010 by The VoxBo Development Team

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

using namespace std;

#include "vbviewmain.moc.h"
#include <QMenuBar>
#include <QSignalMapper>
#include "fileview.h"
#include <QKeyEvent>
#include "vbview.hlp.h"

void vbview_help();
void vbview_version();

VBPrefs vbp;

int
main(int argc,char **argv)
{
  vbp.init();
  QApplication a(argc,argv); // qt init
  QFont f("Helv",10,0);
  a.setFont(f);
  a.setStyle(new QWindowsStyle());
  VBViewMain op;
  a.setMainWidget(&op);
  op.setFont(f);
  op.show();

  tokenlist args;
  args.Transfer(argc-1,argv+1);
  VBView *qtv=NULL;
  if (args.size()==0) {
    // op.AddView();
  }
  else {
    for (size_t i=0; i<args.size(); i++) {
      if (args[i]=="-s" && i<args.size()-1) {
        qtv->LoadStat(args[i+1]);
        i++;
      }
      // else if (args[i]=="-a" && i<args.size()-1) {
      //   qtv->LoadAux(args[i+1]);
      //   i++;
      // }
      else if (args[i]=="-m" && i<args.size()-1) {
        qtv->LoadMask(args[i+1]);
        i++;
      }
      else if (args[i]=="-g" && i<args.size()-1) {
        if (!qtv)
          qtv=op.AddView();
        qtv->LoadGLM(args[i+1]);
        i++;
      }
      else if (args[i]=="-h" || args[i]=="--help") {
        vbview_help();
        exit(0);
      }
      else if (args[i]=="-v" || args[i]=="--version") {
        vbview_version();
        exit(0);
      }
      else {
        qtv=op.AddView();
        qtv->LoadImage(args[i]);
      }
    }
  }
  return (a.exec());
}

// the sole constructor for VBViewMain sets up the tabbed area, a few
// buttons, and the help system

VBViewMain::VBViewMain(QWidget *parent,const char *)
  : Q3VBox(parent)
{
  setGeometry(10,10,720,540);

  // the tabbed window area
  imagetab=new QTabWidget(this);
  if (!imagetab) exit(999);

  // let's do a menubar, complete with keyboard shortcuts
  QMenuBar *menubar=new QMenuBar(this);
  QMenu *filemenu=new QMenu("&File",this);
  QMenu *viewmenu=new QMenu("View",this);
  QMenu *maskmenu=new QMenu("Mask",this);
  QMenu *statmenu=new QMenu("Stats",this);
  QMenu *specialmenu=new QMenu("Special",this);
  QMenu *helpmenu=new QMenu("Help",this);

  // File menu
  filemenu->addAction("New &Tab",this,SLOT(NewPane()),Qt::CTRL+Qt::Key_T);
  filemenu->addAction("&Open",this,SLOT(OpenFile()),Qt::CTRL+Qt::Key_O);

  Cube cb;
  string label;
  QSignalMapper *templatemapper=new QSignalMapper(this);
  QSignalMapper *maskmapper=new QSignalMapper(this);
  LoadMaps();
  if (volmap.size()) {
    QMenu *tmenu=new QMenu("Open Template",this);
    filemenu->addMenu(tmenu);
    pair<int,Cube> tt;
    vbforeach(tt,volmap) {
      QAction *ac;
      if (tt.second.GetHeader("maptype")=="mask")
        ac=tmenu->addAction(QString("Open ")+QString(xfilename(tt.second.id3).c_str()),maskmapper,SLOT(map()),0);
      else
        ac=tmenu->addAction(QString("Open ")+QString(xfilename(tt.second.id3).c_str()),templatemapper,SLOT(map()),0);
      if (tt.first<10) {
        ac->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_1+tt.first-1));
      }
      if (tt.second.GetHeader("maptype")=="mask")
        maskmapper->setMapping(ac,QString(tt.second.GetFileName().c_str()));
      else
        templatemapper->setMapping(ac,QString(tt.second.GetFileName().c_str()));
    }
    QObject::connect(templatemapper,SIGNAL(mapped(const QString &)),
                     this,SLOT(OpenTemplate(const QString &)));
    QObject::connect(maskmapper,SIGNAL(mapped(const QString &)),
                     this,SLOT(OpenMask(const QString &)));
  }
  
  // filemenu->addAction("Open Directory",this,SLOT(OpenDirectory()),0);
  filemenu->addAction("Save",this,SLOT(SaveImage()),0);
  filemenu->addAction("&Save PNG",this,SLOT(SavePNG()),Qt::CTRL+Qt::Key_S);
  filemenu->addAction("&Print",this,SLOT(Print()),0);
  filemenu->addAction("Save &As",this,SLOT(SaveImage()),Qt::CTRL+Qt::Key_A);
  filemenu->addAction("&Close",this,SLOT(Close()),Qt::CTRL+Qt::Key_W);
  filemenu->addAction("&Quit",this,SLOT(Quit()),Qt::CTRL+Qt::Key_Q);
  menubar->addMenu(filemenu);

  // View menu
  //viewmenu->addAction("Single Axial",this,SLOT(ViewAxial()),0);
  //viewmenu->addAction("Single Coronal",this,SLOT(ViewCoronal()),0);
  //viewmenu->addAction("Single Sagittal",this,SLOT(ViewSagittal()),0);
  //viewmenu->addAction("MultiSlice",this,SLOT(ViewMulti()),0);
  //viewmenu->addAction("TriView",this,SLOT(ViewTri()),0);
  viewmenu->insertSeparator();
  // viewmenu->addAction("Show/Hide Display Panel",this,SLOT(ToggleDisplayPanel()),Qt::Key_F5);
  // viewmenu->addAction("Show/Hide Mask Panel",this,SLOT(ToggleMaskPanel()),Qt::Key_F6);
  // viewmenu->addAction("Show/Hide Stat Panel",this,SLOT(ToggleStatPanel()),Qt::Key_F7);
  viewmenu->addAction("Toggle between overlays",this,SLOT(ToggleOverlays()),Qt::CTRL+Qt::Key_L);
  viewmenu->addAction("Show/Hide TimeSeries Panel",this,SLOT(ToggleTSPanel()),Qt::Key_F8);
  viewmenu->addAction("Show/Hide Header Info",this,SLOT(ToggleHeaderPanel()),Qt::Key_F9);
  viewmenu->addAction("Layer Info",this,SLOT(LayerInfo()),Qt::CTRL+Qt::Key_I);
  menubar->addMenu(viewmenu);

  // Mask menu
  maskmenu->addAction("New Mask",this,SLOT(NewMask()),Qt::CTRL+Qt::Key_N);
  maskmenu->addAction("Load Mask",this,SLOT(LoadMask()));
  maskmenu->addAction("Save Mask",this,SLOT(SaveMask()));
  maskmenu->addAction("Clear Mask",this,SLOT(ClearMask()));
  maskmenu->insertSeparator();
  maskmenu->addAction("Copy",this,SLOT(Copy()),Qt::CTRL+Qt::Key_C);
  maskmenu->addAction("Cut",this,SLOT(Cut()),Qt::CTRL+Qt::Key_X);
  maskmenu->addAction("Paste",this,SLOT(Paste()),Qt::CTRL+Qt::Key_V);
  maskmenu->addAction("Undo",this,SLOT(Undo()),Qt::CTRL+Qt::Key_Z);
  menubar->addMenu(maskmenu);

  // Stat menu
  statmenu->addAction("Load Stat Map",this,SLOT(LoadStat()));
  statmenu->addAction("View VoxBo GLM",this,SLOT(LoadGLM()));
  // statmenu->addAction("Load Auxiliary Map",this,SLOT(LoadAux()));
  statmenu->addAction("Create Correlation Map",this,SLOT(LoadCorr()));
  menubar->addMenu(statmenu);

  // Special menu
  //specialmenu->addAction("Create SNR Map",this,SLOT(AddSNRView()));
  // specialmenu->addAction("Create Outline",this,SLOT(NewOutline()));
  specialmenu->addAction("Byteswap image",this,SLOT(ByteSwap()));
  specialmenu->addAction("Load averages file",this,SLOT(LoadAverages()));
  specialmenu->addAction("Go to origin",this,SLOT(GoToOrigin()));
  menubar->addMenu(specialmenu);
  menubar->insertSeparator();
  
  // Help menu
  helpmenu->addAction("About vbview",this,SLOT(About()));
  helpmenu->addAction("Help",this,SLOT(Help()));
  menubar->addMenu(helpmenu);
}

void
VBViewMain::LoadMaps()
{
  tokenlist config,path,args;
  path.SetSeparator(":");
  vglob vg;
  config.ParseFile(vbp.rootdir+"/etc/maps.txt");
  for (size_t i=0; i<config.size(); i++) {
    args.ParseLine(config[i]);
    if (args[0]=="path") {
      path.clear();
      path.ParseLine(args.Tail());
      for (size_t j=0; j<path.size(); j++)
        vg.append(path[j]+"/*",vglob::f_filesonly);
      vg.append(vbp.rootdir+"/elements/templates/*");
    }

    else if (args[0]=="template" || args[0]=="mask") {
      if (vg.size()==0)
        vg.load(vbp.rootdir+"/elements/templates/*");
      int index=strtol(args[1]);
      string label=args.Tail(3);
      // if we already have this index, ignore it
      if (volmap.count(index)) continue;
      for (size_t j=0; j<vg.size(); j++) {
        if (xfilename(vg[j])==args[2]) {
          Cube cb;
          if (!cb.ReadHeader(vg[j])) {
            cb.id3=label;
            cb.AddHeader("maptype "+args[0]);
            volmap[index]=cb;
            break;
          }
        }
      }
    }
  }
}

VBView *
VBViewMain::AddView()
{
  VBView *qtv=new VBView(imagetab);
  if (!qtv) return qtv;
  imagetab->addTab(qtv,"<no image>");
  imagetab->setCurrentPage(imagetab->count()-1);
  QObject::connect(qtv,SIGNAL(closeme(QWidget *)),this,SLOT(ClosePanel(QWidget *)));
  QObject::connect(qtv,SIGNAL(renameme(QWidget *,string)),this,SLOT(RenamePanel(QWidget *,string)));
  return (qtv);  // no error!
}

// FIXME re-implement, should probably create a layer

// void
// VBViewMain::AddSNRView()
// {
//   VBView *qtv=new VBView(imagetab);
//   if (!qtv) return;
//   Cube cb;
//   SNRMap(((VBView *)(imagetab->currentPage()))->tes,cb);
//   qtv->SetImage(cb);
//   imagetab->addTab(qtv,"<SNR image>");
//   imagetab->setCurrentPage(imagetab->count()-1);
//   QObject::connect(qtv,SIGNAL(closeme(QWidget *)),this,SLOT(ClosePanel(QWidget *)));
//   QObject::connect(qtv,SIGNAL(renameme(QWidget *,string)),this,SLOT(RenamePanel(QWidget *,string)));
//   return;
// }

void
VBViewMain::NewOutline()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewOutline();
}

void
VBViewMain::ByteSwap()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ByteSwap();
}

void
VBViewMain::LayerInfo()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LayerInfo();
}

void
VBViewMain::LoadAverages()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadAverages();
}

void
VBViewMain::GoToOrigin()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->GoToOrigin();
}

void
VBViewMain::NewPane()
{
  AddView();
}

int
VBViewMain::ClosePanel(QWidget *w)
{
  if (imagetab->indexOf(w) == -1)
    return (101);
  imagetab->removePage(w);
  delete w;
  return (0);
}

int
VBViewMain::RenamePanel(QWidget *w,string label)
{
  if (imagetab->indexOf(w) == -1)
    return (101);
  imagetab->changeTab(w,label.c_str());
  return (0);
}

void
VBViewMain::SaveImage()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->SaveImage();
}

void
VBViewMain::OpenFile()
{
  fileview fv;
  fv.ShowDirectoriesAlso(1);
  fv.ShowImageInformation(1);
  vector<string> ff=fv.Go();
  vbforeach(string f,ff) {
    VBView *vv=AddView();
    if (!vv) return;
    vv->LoadImage(f);
  }
  return;
}

void
VBViewMain::OpenTemplate(const QString &ff)
{
  VBView *vv=AddView();
  if (!vv) return;
  vv->LoadImage(ff.toStdString());
}

void
VBViewMain::OpenMask(const QString &ff)
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadMask(ff.toStdString());
  return;
}

void
VBViewMain::LoadMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadMask();
  return;
}

void
VBViewMain::NewMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewMask();
  return;
}

void
VBViewMain::OpenDirectory()
{
//   if ((VBView *)imagetab->currentPage())
//     ((VBView *)imagetab->currentPage())->LoadDir();
}

void
VBViewMain::SavePNG()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->SavePNG();
}

void
VBViewMain::Print()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->Print();
}

void
VBViewMain::Close()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->Close();
}

void
VBViewMain::Quit()
{
  // QMessageBox::warning(this,"Foo Bar","Exit not implemented","Okay","Okay","Great");
  // FIXME shouldn't just exit!
  exit(0);
}

void
VBViewMain::keyPressEvent (QKeyEvent *ke)
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->keyPressEvent(ke);
}

void
VBViewMain::SaveMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->handle_savemask();
}

void
VBViewMain::ClearMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ClearMask();
}

void
VBViewMain::Copy()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->Copy();
}

void
VBViewMain::Cut()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->Cut();
}

void
VBViewMain::Paste()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->Paste();
}

void
VBViewMain::Undo()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->PopMask();
}


void
VBViewMain::ViewAxial()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewView("Axial");
}

void
VBViewMain::ViewCoronal()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewView("Coronal");
}

void
VBViewMain::ViewSagittal()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewView("Sagittal");
}

void
VBViewMain::ViewMulti()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewView("Multi-View");
}

void
VBViewMain::ViewTri()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->NewView("Tri-View");
}

void
VBViewMain::ToggleDisplayPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleDisplayPanel();
}

// void
// VBViewMain::ToggleMaskPanel()
// {
//   //if ((VBView *)imagetab->currentPage())
//   //  ((VBView *)imagetab->currentPage())->ToggleMaskPanel();
// }

// void
// VBViewMain::ToggleStatPanel()
// {
//   //  if ((VBView *)imagetab->currentPage())
//   //  ((VBView *)imagetab->currentPage())->ToggleStatPanel();
// }

void
VBViewMain::ToggleTSPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleTSPanel();
}

void
VBViewMain::ToggleOverlays()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleOverlays();
}

void
VBViewMain::ToggleHeaderPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleHeaderPanel();
}

void
VBViewMain::LoadStat()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadStat();
}

// void
// VBViewMain::LoadAux()
// {
//   if ((VBView *)imagetab->currentPage())
//     ((VBView *)imagetab->currentPage())->LoadAux();
// }

void
VBViewMain::LoadGLM()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadGLM();
}

void
VBViewMain::LoadCorr()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->HandleCorr();
}

void
VBViewMain::About()
{
  string text=
    (string)
    "This is vbview, the image viewer that comes with VoxBo.\n"+
    "For more information about VoxBo, see www.voxbo.org.  "+
    "\n\nThis is version "+vbversion+
    "\n\nThe following shortcuts may be helpful:\n"+
    "+ or - to zoom in or out (= works for zoom in too)\n"+
    "SPACE to toggle the crosshairs\n"+
    "z and x to move down and up in the Z dimension\n"+
    "a and s for previous and next volume (for 4D images)\n"+
    "u and d to move the current layer up or down\n"+
    "\n"+
    "CTRL-c to copy visible voxels from current layer as mask\n"+
    "CTRL-v to paste copied mask onto current layer\n"+
    "CTRL-x to remove masking from current layer\n"+
    "(masks can only be applied to layers with identical dimensions)";

  QMessageBox::about(this,QString("About vbview"),(QString)text.c_str());
}

void
VBViewMain::Help()
{
  string text=
    (string)"The following key shortcuts can be used here:\n"+
    "-/= to zoom in or out\n"+
    "SPACE to toggle the crosshairs\n"+
    "z/x to move down and up in the Z dimension\n"+
    "a/s for previous and next volume (for 4D images)\n"+
    "u/d to move the current layer up or down";
  QMessageBox::about(this,QString("vbview2 help"),(QString)text.c_str());
}



void
vbview_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbview_version()
{
  printf("VoxBo vbview (v%s)\n",vbversion.c_str());
}
