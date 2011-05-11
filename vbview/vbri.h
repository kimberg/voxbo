
// vbri.h
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

using namespace std;

#include <QMainWindow>

class VBmask {
public:
  VBmask() {rating=0;  xslice=yslice=zslice=0;  rnum=VBRandom();}
  string imgname;
  int rating;
  int xslice,yslice,zslice;
  string notes;
  uint32 rnum;
};

class VBri : public QMainWindow {
  Q_OBJECT
public:
  VBri();
  void resizeEvent(QResizeEvent *re);
  string v_motd;
  void keyPressEvent(QKeyEvent *ke);
  void process(string configfile);

private slots:
  int openconfig();
  int openconfig(string configfile);
  void postprocessconfig();
  void handleIndex(int ind);
  void handleFirst();
  void handlePrev();
  void handleNext();
  void handleLast();
  void handleDone();
  void handleRating(int r);
  void handleNotes();
  void handleSplit();
  void closeEvent(QCloseEvent *ce);
 private:
  void setup_widgets();
  void arrangeChildren();
  void randomizemasks();
  void displayrating(int num);
  void writeCurrent();
  void writeAll(string ofile="");
  void updateRating();

  // bits of data
  string configfile,outfile,leftimg,rightimg;
  list<Cube> cubelist;
  map<string,list<Cube>::iterator> cubemap;
  vector<VBmask> masklist;
  vector<VBmask>::iterator maskptr;
  uint16 currentindex;

  // actions for everyone
  // QAction *a_openconfig;

  // central widget
  QFrame *myframe;
  // QMenuBar *w_menubar;
  // stuff inside central widget
  QLabel *w_logo;
  QLabel *w_banner;
  VBView *w_view1;
  VBView *w_view2;
  QVBox *w_ratebox;
  QHBox *w_navbox;
  QVBox *w_detailbox;
  QPushButton *w_firstbutton;
  QPushButton *w_prevbutton;
  QPushButton *w_nextbutton;
  QPushButton *w_lastbutton;
  QPushButton *w_donebutton;
  QLabel *w_maskname;
  QLabel *w_ratinglabel1,*w_ratinglabel2;
  QTextEdit *w_ratingnotes;
  QSlider *w_rating;
  QSlider *w_index;
  // menus
  // QMenu *filemenu;

  // widget building
  QVBox *makeratebox();
  QHBox *makenavbox();
  QVBox *makedetailbox();
};
