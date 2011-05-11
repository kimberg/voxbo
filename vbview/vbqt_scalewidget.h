
// vbqt_scalewidget.h
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

#include <qimage.h>
#include <qpainter.h>
#include <QMouseEvent>
#include <QFrame>
#include <stdlib.h>

class VBScalewidget : public QFrame {
  Q_OBJECT
public:
  VBScalewidget(QWidget *parent=0);
  void drawContents(QPainter *);
public slots:
  void setscale(float low,float high,QColor negcolor1,QColor negcolor2,
              QColor poscolor1,QColor poscolor2);
  void setscale(float low,float high,QColor poscolor1,QColor poscolor2);
protected:
  // void drawContents(QPainter *p,int clipx,int clipy,int clipw,int cliph);
private slots:
signals:
  void newcolors(QColor,QColor,QColor,QColor);
private:
  void contextmenu();
  void drawcolorbars(QPainter *,QRect,int);
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
  void paintEvent(QPaintEvent *pe);
  void shaderect(QPainter *paint,QRect &r,QColor &c1,QColor &c2);

  QColor q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2;
  float q_low,q_high;
  bool f_vertical;
  QRect posrect,negrect;
};
