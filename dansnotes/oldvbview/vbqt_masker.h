
// vbqt_masker.h
// widget for setting and selecting mask attributes
// Copyright (c) 1998-2006 by The VoxBo Development Team

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
#include <qvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qscrollview.h>
#include <iostream>
#include <string>

class VBQT_colorbox : public QVBox {
  Q_OBJECT
public:
  VBQT_colorbox(QWidget *parent=0,const char *name=0);
  int editable;
  char type;
public slots:
  void getnewcolor();
  void setcolor(QColor newcolor);
  QColor getcolor();
private:
  void xpaintEvent(QPaintEvent *);
  void mousePressEvent(QMouseEvent *me);
  // void mouseDoubleClickEvent(QMouseEvent *me);
  // void contextMenuEvent(QContextMenuEvent *);
  QColor color;
  static QPixmap *pm_a,*pm_m,*pm_r;
signals:
  void rightclicked();
  void colorchanged(QColor tmp);
  void clicked();
};

class QTMaskWidget : public QHBox {
  Q_OBJECT
private:
  void paintEvent(QPaintEvent *);
  QCheckBox *checkbox;
  VBQT_colorbox *cframe;
  QWidget *drawarea;
  QLineEdit *edit_label,*edit_index;
public:
  QTMaskWidget(QWidget *parent=0,const char *name=0,string maskname="");
  void SetColor(QColor &c);
  void SetLabel(string &str);
  void SetIndex(int ind);
  void GetInfo(QColor &color,int &index,string &label);
  void SetType(char c);
  void Toggle();
  int index;
  QColor color;
  string label;
  int f_selected;
  int f_visible;
  char type;
  //   QSizePolicy sizePolicy();
public slots:
  void ctoggled(bool);
  void clicked();
  void contextmenu();
  void setnewcolor(QColor newcolor);
  void newlabel(const QString &);
  void newindex(const QString &);
  void setwidth(int w,int h);
signals:
  void changed();
  void selected(QTMaskWidget *);
  void inforequested(QTMaskWidget *);
  void statrequested(QTMaskWidget *);
};

class QTMaskList : public QScrollView {
  Q_OBJECT
public:
  QTMaskList(QWidget *parent);
public slots:
  void viewportResizeEvent(QResizeEvent *rw);
signals:
  void newsize(int w,int h);
};
