
// vbviewmain.cpp
// VoxBo discount image viewer
// Copyright (c) 2002-2007 by The VoxBo Development Team

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

#include "vbview.moc.h"
#include "qmenubar.h"
#include "qpopupmenu.h"
#include "fileview.h"

void vbview_help();
void vbview_version();

VBPrefs vbp;

int
main(int argc,char **argv)
{
  vector<string> filelist;
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  string anatname,maskname;
  for (int i=0; i<args.size(); i++) {
    if (args[i]=="-v") {
      vbview_version();
      exit(0);
    }
    else if (args[i]=="-h") {
      vbview_help();
      exit(0);
    }
    else if (args[i]=="-a" && i<args.size()-1) {
      anatname=args[++i];
    }
    else if (args[i]=="-m" && i<args.size()-1) {
      maskname=args[++i];
    }
    else
      filelist.push_back(args[i]);
  }


  QApplication a(argc,argv); // qt init
  // QFont f("Helv",10,0);
  // a.setFont(f);
  a.setStyle(new QWindowsStyle());
  VBViewMain op;
  a.setMainWidget(&op);
  // op.setFont(f);
  op.show();

  Cube mask;
  if (maskname.size())
    mask.ReadFile(maskname);

  VBView *qtv;
  if (filelist.size()==0) {
    op.AddView();
  }
  else if (anatname.size()) {
    Cube anat;
    if (anat.ReadFile(anatname))
      QMessageBox::warning(&op,"Anatomy reading error","Couldn't read anatomy file","Okay");
    else {
      for (size_t i=0; i<filelist.size(); i++) {
        qtv=op.AddView();
        if (qtv) {
          qtv->SetImage(anat);
          if (mask.data)
            qtv->SetMask(mask);
          qtv->SetOverlay(filelist[i]);
        }
      }
    }
  }
  else {
    for (size_t i=0; i<filelist.size(); i++) {
      qtv=op.AddView();
      if (qtv)
        qtv->SetImage(args[i]);
    }
  }
  return (a.exec());
}

// the sole constructor for VBViewMain sets up the tabbed area, a few
// buttons, and the help system

VBViewMain::VBViewMain(QWidget *parent,const char *name)
  : QVBox(parent,name)
{
  setGeometry(10,10,700,450);

  // the tabbed window area
  imagetab=new QTabWidget(this);
  if (!imagetab) exit(999);

  // let's do a menubar, complete with keyboard shortcuts
  QMenuBar *menubar=new QMenuBar(this);
  QPopupMenu *filemenu=new QPopupMenu(this);
  QPopupMenu *viewmenu=new QPopupMenu(this);
  QPopupMenu *maskmenu=new QPopupMenu(this);
  QPopupMenu *statmenu=new QPopupMenu(this);
  QPopupMenu *specialmenu=new QPopupMenu(this);
  QPopupMenu *helpmenu=new QPopupMenu(this);

  // File menu
  filemenu->insertItem("New &Tab",this,SLOT(NewPane()),CTRL+Key_T);
  filemenu->insertItem("&Open",this,SLOT(OpenFile()),CTRL+Key_O);
  //  filemenu->insertItem("Open Directory",this,SLOT(OpenDirectory()),0);
  filemenu->insertItem("Save",this,SLOT(SaveImage()),0);
  filemenu->insertItem("&Save PNG",this,SLOT(SavePNG()),CTRL+Key_S);
  filemenu->insertItem("&Print",this,SLOT(Print()),0);
  filemenu->insertItem("Save &As",this,SLOT(SaveImage()),CTRL+Key_A);
  filemenu->insertItem("&Close",this,SLOT(Close()),CTRL+Key_W);
  filemenu->insertItem("&Quit",this,SLOT(Quit()),CTRL+Key_Q);
  menubar->insertItem("&File",filemenu);

  // View menu
  viewmenu->insertItem("Single Axial",this,SLOT(ViewAxial()),0);
  viewmenu->insertItem("Single Coronal",this,SLOT(ViewCoronal()),0);
  viewmenu->insertItem("Single Sagittal",this,SLOT(ViewSagittal()),0);
  viewmenu->insertItem("MultiSlice",this,SLOT(ViewMulti()),0);
  viewmenu->insertItem("TriView",this,SLOT(ViewTri()),0);
  viewmenu->insertSeparator();
  viewmenu->insertItem("Show/Hide Display Panel",this,SLOT(ToggleDisplayPanel()),Key_F5);
  viewmenu->insertItem("Show/Hide Mask Panel",this,SLOT(ToggleMaskPanel()),Key_F6);
  viewmenu->insertItem("Show/Hide Stat Panel",this,SLOT(ToggleStatPanel()),Key_F7);
  viewmenu->insertItem("Show/Hide TimeSeries Panel",this,SLOT(ToggleTSPanel()),Key_F8);
  viewmenu->insertItem("Show/Hide Header Info",this,SLOT(ToggleHeaderPanel()),Key_F9);
  menubar->insertItem("&View",viewmenu);

  // Mask menu
  maskmenu->insertItem("Load Mask",this,SLOT(LoadMask()));
  maskmenu->insertItem("Load Mask Sample",this,SLOT(LoadMaskSample()));
  maskmenu->insertItem("Save Mask",this,SLOT(SaveMask()));
  maskmenu->insertItem("Save Separate Masks",this,SLOT(SaveSeparateMasks()));
  maskmenu->insertItem("Clear Mask",this,SLOT(ClearMask()));
  maskmenu->insertSeparator();
  maskmenu->insertItem("Copy Slice",this,SLOT(CopySlice()),CTRL+Key_C);
  maskmenu->insertItem("Cut Slice",this,SLOT(CutSlice()),CTRL+Key_X);
  maskmenu->insertItem("Paste Slice",this,SLOT(PasteSlice()),CTRL+Key_V);
  maskmenu->insertItem("Undo",this,SLOT(Undo()),CTRL+Key_Z);
  menubar->insertItem("&Mask",maskmenu);

  // Stat menu
  statmenu->insertItem("Load Stat Map",this,SLOT(LoadOverlay()));
  statmenu->insertItem("View VoxBo GLM",this,SLOT(LoadGLM()),CTRL+Key_G);
  statmenu->insertItem("Create Correlation Map",this,SLOT(CorrelationMap()),CTRL+Key_T);
  menubar->insertItem("&Stats",statmenu);

  // Special menu
  //specialmenu->insertItem("Create SNR Map",this,SLOT(AddSNRView()));
  specialmenu->insertItem("Byteswap image",this,SLOT(ByteSwap()));
  specialmenu->insertItem("Load averages file",this,SLOT(LoadAverages()));
  menubar->insertItem("&Special",specialmenu);
  menubar->insertSeparator();
  
  // Help menu
  helpmenu->insertItem("About vbview",this,SLOT(About()));
  menubar->insertItem("&Help",helpmenu);
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

void
VBViewMain::AddSNRView()
{
  VBView *qtv=new VBView(imagetab);
  if (!qtv) return;
  Cube cb;
  SNRMap(((VBView *)(imagetab->currentPage()))->tes,cb);
  qtv->SetImage(cb);
  imagetab->addTab(qtv,"<SNR image>");
  imagetab->setCurrentPage(imagetab->count()-1);
  QObject::connect(qtv,SIGNAL(closeme(QWidget *)),this,SLOT(ClosePanel(QWidget *)));
  QObject::connect(qtv,SIGNAL(renameme(QWidget *,string)),this,SLOT(RenamePanel(QWidget *,string)));
  return;
}

void
VBViewMain::ByteSwap()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ByteSwap();
}

void
VBViewMain::LoadAverages()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadAverages();
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
VBViewMain::Help()
{
  QMessageBox::warning(this,"Foo Bar","Not implemented","Okay","Okay","Great");
}

void
VBViewMain::OpenFile()
{
  fileview fv;
  fv.ShowDirectoriesAlso(1);
  fv.ShowImageInformation(1);
  fv.Go();
  tokenlist ff=fv.ReturnFiles();
  for (int i=0; i<ff.size(); i++) {
    VBView *vv=AddView();
    if (!vv) return;
    vv->SetImage(ff[i]);
  }

  return;
//   if ((VBView *)imagetab->currentPage())
//     ((VBView *)imagetab->currentPage())->LoadFile();
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
    ((VBView *)imagetab->currentPage())->KeyPressEvent(*ke);
}

void
VBViewMain::LoadMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadMask();
}

void
VBViewMain::LoadMaskSample()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadMaskSample();
}

void
VBViewMain::SaveMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->SaveMask();
}

void
VBViewMain::SaveSeparateMasks()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->SaveSeparateMasks();
}

void
VBViewMain::ClearMask()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ClearMask();
}

void
VBViewMain::CopySlice()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->CopySlice();
}

void
VBViewMain::CutSlice()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->CutSlice();
}

void
VBViewMain::PasteSlice()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->PasteSlice();
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

void
VBViewMain::ToggleMaskPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleMaskPanel();
}

void
VBViewMain::ToggleStatPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleStatPanel();
}

void
VBViewMain::ToggleTSPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleTSPanel();
}

void
VBViewMain::ToggleHeaderPanel()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->ToggleHeaderPanel();
}

void
VBViewMain::LoadOverlay()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadOverlay();
}

void
VBViewMain::LoadGLM()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->LoadGLM();
}

void
VBViewMain::CorrelationMap()
{
  if ((VBView *)imagetab->currentPage())
    ((VBView *)imagetab->currentPage())->CreateCorrelationMap();
}

void
VBViewMain::About()
{
  string text=
    (string)"This is vbview, the image viewer that comes with VoxBo.\n"+
    "For more information about VoxBo, see www.voxbo.org/wiki.  "+
    "\n\nYou are using version: "+vbversion;
  QMessageBox::about(this,QString("About vbview"),(QString)text.c_str());
}



void
vbview_help()
{
  printf("\nVoxBo vbview (v%s)\n",vbversion.c_str());
  printf("usage:\n");
  printf("  vbview [flags] [<file> ...]\n");
  printf("flags:\n");
  printf("  -a <3D file>  use as anatomy for subsequent overlay maps\n");
  printf("  -h            show help\n");
  printf("  -v            show version\n");
  printf("\n");
}


void
vbview_version()
{
  printf("VoxBo vbview (v%s)\n",vbversion.c_str());
}
