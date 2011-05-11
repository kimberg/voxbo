
// fileview.cpp
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Tom King and Dan Kimberg

#include "fileview.h"
#include <QApplication>
#include <q3filedialog.h>
#include <QVBoxLayout>

enum {COL_NAME=0,COL_SIZE=1,COL_INFO=2};

fileview::fileview( QWidget* parent, const char* name, bool modal , Qt::WFlags fl )
  : QDialog( parent, name, modal, fl )
{
  QPushButton* button;
  if (!name) setName( "fileview" );
  setMinimumSize(QSize(400,350));

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setAlignment(Qt::AlignTop);
  QHBox *hb;

  hb=new QHBox(this);
  layout->addWidget(hb);
  
  button=new QPushButton("Home",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button, SIGNAL(clicked()), this, SLOT(HandleHome()));

  button=new QPushButton("Up",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button, SIGNAL(clicked()), this, SLOT(HandleUp()));

  button=new QPushButton("/",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button, SIGNAL(clicked()), this, SLOT(HandleRoot()));

  hb=new QHBox(this);
  layout->addWidget(hb);
  
  button=new QPushButton("Directory: ",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button,SIGNAL(clicked()),this,SLOT(HandleNewWD()));

  leDirectory = new QLineEdit(hb,"leDirectory" );
  hb->addWidget(leDirectory);

  hb=new QHBox(this);
  layout->addWidget(hb);
  hb->addWidget(new QLabel("Filename pattern: ",hb));

  leFileNamePattern = new QLineEdit(hb,"leFileNamePattern" );
  hb->addWidget(leFileNamePattern);

  grpFileBorder = new QHBox(this);
  grpFileBorder->setLineWidth( 2 );
  layout->addWidget(grpFileBorder);

  lvFiles = new QTreeWidget;
  layout->addWidget(lvFiles);
  lvFiles->setColumnCount(3);
  lvFiles->setRootIsDecorated(0);
  QStringList hlabels;
  hlabels<<"Filename"<<"Size"<<"Information";
  lvFiles->setHeaderLabels(hlabels);
  //lvFiles->setColumnAlignment(COL_SIZE,Qt::AlignRight);
  //lvFiles->setColumnWidth(COL_INFO,0);
  lvFiles->setMinimumHeight(380);
  lvFiles->setSelectionMode(QAbstractItemView::ExtendedSelection); 

  hb=new QHBox(this);
  layout->addWidget(hb);

  button = new QPushButton("Okay",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button, SIGNAL(clicked()), this, SLOT(Handler()));

  button=new QPushButton("Cancel",hb);
  button->setAutoDefault(0);
  hb->addWidget(button);
  connect(button,SIGNAL(clicked()), this, SLOT(Cancel()));

  okayed = false;
  showdirs = false;
  setCaption("Select Files");
  char tmps[STRINGLEN];
  getcwd(tmps,STRINGLEN-1);
  leDirectory->setText(tmps);
  leFileNamePattern->clear();
  leFileNamePattern->setText("*"); 

  connect(leDirectory,SIGNAL(textChanged(const QString &)),this,SLOT(grayDir()));
  connect(leDirectory,SIGNAL(returnPressed()),this,SLOT(populateListBox()));
  connect(leFileNamePattern, SIGNAL(textChanged(const QString &)),this,SLOT(populateListBox()));
  connect(lvFiles,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(Selected(QTreeWidgetItem*,int)));
}

fileview::~fileview()
{
}

void
fileview::SetDirectory(string dir)
{
  return;
  leDirectory->clear();
  leDirectory->setText(dir.c_str());
  return;
}

void
fileview::SetPattern(string pat)
{
  leFileNamePattern->setText(pat.c_str());
}

void
fileview::ShowImageInformation(bool x)
{
  if (x)
    lvFiles->setColumnWidth(COL_INFO,200);
  else
    lvFiles->setColumnWidth(COL_INFO,0);
  return;
}   

vector<string>
fileview::Go()
{
  populateListBox();
  this->exec();
  return returnedFiles;
}

void
fileview::ShowDirectoriesAlso(bool x)
{
  showdirs = x;
  return;
}

void
fileview::populateListBox()
{
  lvFiles->clear();
  struct stat st;
  char size[STRINGLEN];
  string dir=leDirectory->text().toStdString();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // get files matching all the pats
  vglob vv;
  vv.append(dir+"/*",vglob::f_dirsonly);
  tokenlist pats(leFileNamePattern->text().toStdString(),";");
  for (size_t i=0; i<pats.size(); i++)
    vv.append(dir+"/"+pats[i],vglob::f_filesonly);
  tokenlist list=vv;

  for (size_t i=0; i<list.size(); i++) {
    if (stat(list[i].c_str(),&st))
      continue;
    if (showdirs==false) {
      if (!(S_ISREG(st.st_mode))) {
        continue;
      }
    }
    string protocol;
    if (lvFiles->columnWidth(COL_INFO) && S_ISREG(st.st_mode)) {
      string imageinfo;
      Cube cb;
      Tes ts;
      VBMatrix mat;
      VB_Vector vec;
      if (!(cb.ReadHeader(list[i]))) {
        imageinfo=(format(" 3D volume: %dx%dx%d")%cb.dimx%cb.dimy%cb.dimz).str();
        protocol=cb.GetHeader("Protocol:");
      }
      else if (!(ts.ReadHeader(list[i]))) {
        imageinfo=(format(" 4D volume: %dx%dx%d, %d volumes")%ts.dimx%ts.dimy%ts.dimz%ts.dimt).str();
        protocol=cb.GetHeader("Protocol:");
      }
      else if (!mat.ReadHeader(list[i])) {
        imageinfo=(format(" 2D matrix: %dx%d")%mat.m%mat.n).str();
      }
      else if (!vec.ReadFile(list[i])) {
        imageinfo=(format(" 1D vector: %d elements (mean %d)")%vec.size()%vec.getVectorMean()).str();
      }

      if (protocol.size())
        imageinfo+=" ("+protocol+")";
      if (S_ISDIR(st.st_mode))
        strcpy(size, "DIR");
      else
        sprintf(size,"%ldK",((long)st.st_size/1024));
      QStringList cols;
      cols<<xfilename(list[i]).c_str()
          <<(S_ISDIR(st.st_mode)?"DIR":prettysize(st.st_size).c_str())
          <<imageinfo.c_str();
      new QTreeWidgetItem(lvFiles,cols);
    }
    else {
      if (S_ISDIR(st.st_mode))
        strcpy(size, "DIR");
      else
        sprintf(size,"%ldK",((long)st.st_size/1024));
      QStringList cols;
      cols<<xfilename(list[i]).c_str()<<size;
      new QTreeWidgetItem(lvFiles,cols);
    }
  }
  QApplication::restoreOverrideCursor();
  leFileNamePattern->setPaletteBackgroundColor(qRgb(255,255,255));
  leDirectory->setPaletteBackgroundColor(qRgb(255,255,255));
  return;
}

void
fileview::Selected(QTreeWidgetItem *item,int)
{
  if ((string)item->text(1).ascii()!="DIR") {
    okayed=1;
    returnedFiles=returnSelectedFiles();
    this->close();
    return;
  }
  string newdir=(string)leDirectory->text().latin1()+"/"+item->text(0).latin1();
  leDirectory->setText(newdir.c_str());
  populateListBox();
}

void fileview::Cancel()
{
  okayed=0;
  returnedFiles.clear();
  this->close(); 
  return;
} 

vector<string>
fileview::returnSelectedFiles()
{
  string dir=xstripwhitespace(leDirectory->text().toStdString());
  vector<string> flist;
  QList<QTreeWidgetItem *>items=lvFiles->selectedItems();
  vbforeach(QTreeWidgetItem *it,items) {
    flist.push_back(dir+"/"+it->text(0).toStdString());
  }
  return flist;
}

void
fileview::Handler()
{
  okayed=1;
  returnedFiles=returnSelectedFiles();
  this->close();
  return;
}

void
fileview::HandleHome()
{
  leDirectory->setText(getenv("HOME"));
  populateListBox();
}
 
void
fileview::HandleUp()
{
  leDirectory->setText(xdirname(leDirectory->text().ascii()).c_str());
  populateListBox();
}
 
void
fileview::HandleRoot()
{
  leDirectory->setText("/");
  populateListBox();
}

void
fileview::grayDir()
{
  leDirectory->setPaletteBackgroundColor(qRgb(220,160,160));
}
 
void
fileview::HandleNewWD()
{
  QString s=Q3FileDialog::getExistingDirectory(leDirectory->text(),
                                              this,
                                              "xxx",
                                              "new dir?");
  leDirectory->setText(s.ascii());
  populateListBox();
}
 
bool fileview::Okayed()
{
  return okayed;
}


