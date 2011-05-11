
// vbqt_masker.h
// widget for setting and selecting mask attributes
// Copyright (c) 1998-2010 by The VoxBo Development Team

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

#include <qwidget.h>
#include <qcheckbox.h>
#include <qpainter.h>
#include <q3vbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <q3scrollview.h>
#include <QScrollArea>
#include <QPixmap>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSignalMapper>
#include <iostream>
#include <string>
#include "vbutil.h"

class VBQT_colorbox : public QFrame {
  Q_OBJECT
public:
  VBQT_colorbox();
  int editable;
  char type;
public slots:
  void getnewcolor();
  void setcolor(QColor newcolor);
private:
  void xpaintEvent(QPaintEvent *);
  void mousePressEvent(QMouseEvent *me);
  void mouseDoubleClickEvent(QMouseEvent *me);
  void contextMenuEvent(QContextMenuEvent *);
  static QPixmap *pm_a,*pm_m,*pm_r;
  QColor color;
  QPalette palette;
signals:
  void colorchanged(QColor tmp);
  void selected();
  void copied();
  void inforequested();
};

class QTMaskWidget : public QFrame {
  Q_OBJECT
private:
  void paintEvent(QPaintEvent *);
  QCheckBox *checkbox;
  VBQT_colorbox *cframe;  // FIXME not used???
  QWidget *drawarea;
  QLineEdit *edit_label,*edit_index;
public:
  QTMaskWidget(string maskname="");
  void setlabel(string str);
  void setindex(int ind);
  void setcolor(QColor newcolor);
  void GetInfo(QColor &color,int &index,string &label);
  void SetType(char c);
  void Toggle();
  uint32 index;
  QColor color;
  string label;
  bool f_selected;
  bool f_visible;
  char type;
public slots:
  void ctoggled(bool);
  void contextmenu();
  // void newcolor(QColor newcolor);
  void newlabel(const QString &);
  void newindex(const QString &);
  void newcolor(QColor color);
signals:
  void changed();
  void selected();
  void copied();
  void inforequested();
};

class QTMaskView : public QWidget {
  Q_OBJECT
public:
  QTMaskView();
  void clear();
  QTMaskWidget *addMask(uint32 id,string name,QColor color);
public slots:
  void handleselect(int i);
  void newmask();
signals:
  // mask-specific things
  void changed(QWidget *m);
  void selected(int i);
  void copied(int i);
  void inforequested(int i);
  // button hits
  void savemask();
  void newmask(QTMaskWidget *mw);
  void newradius(int);
  void newzradius(int);
private:
  QPalette onpalette,offpalette;
  QScrollArea *area;
  QWidget *mwidget;
  QVBoxLayout *mlayout;
  set<QTMaskWidget *>maskwidgets;
  QSignalMapper *selectmapper,*changemapper,*infomapper,*copymapper;
  int lastselected;
  void keyPressEvent(QKeyEvent *ke);
  QSpinBox *radiusbox;              // mask draw radius
  QCheckBox *zradiusbox;            // mask draw-in-3d flag
};
