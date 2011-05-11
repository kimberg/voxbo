
// vbqt_canvas.cpp
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

#include <QPaintEvent>
#include <QMouseEvent>

using namespace std;

#include "qwidget.h"
#include <iostream>
#include "vbqt_canvas.moc.h"

VBCanvas::VBCanvas(QWidget *parent,const char *name,Qt::WFlags)
  : QWidget(parent,name)
{
  setMouseTracking(1);
  cim=NULL;
}

void
VBCanvas::SetImage(QImage *im)
{
  if (im)
    setFixedSize(im->width(),im->height());
  cim=im;
}

QImage *
VBCanvas::GetImage()
{
  return cim;
}

void
VBCanvas::updateVisibleNow()
{
  return; // FIXME
  if (!cim)
    return;
  // FIXME obsolete code below
  QRect myrect=visibleRect();
  QPainter paint(this);
  paint.drawImage(myrect.x(),myrect.y(),*cim,myrect.x(),myrect.y(),myrect.width(),myrect.height());
}

void
VBCanvas::updateVisibleNow(QRect &r)
{
  return; // FIXME
  if (!cim)
    return;

  QRect myrect=visibleRect() & r;
  QPainter paint(this);
  paint.drawImage(myrect.x(),myrect.y(),*cim,myrect.x(),myrect.y(),myrect.width(),myrect.height());
}

void
VBCanvas::paintEvent(QPaintEvent *pe)
{
  if (!cim)
    return;

  QRect myrect=pe->region().boundingRect();
  QPainter paint(this);
  paint.drawImage(myrect.x(),myrect.y(),*cim,myrect.x(),myrect.y(),myrect.width(),myrect.height());
}

void
VBCanvas::updateRegion(int x,int y,int width,int height)
{
  return; // FIXME
  if (!cim)
    return;
  QPainter paint(this);
  paint.drawImage(x,y,*cim,x,y,width,height);
}

void
VBCanvas::mousePressEvent(QMouseEvent *me)
{
  emit mousepress(*me);
}

void
VBCanvas::mouseMoveEvent(QMouseEvent *me)
{
  emit mousemove(*me);
}

void
VBCanvas::moveEvent(QMoveEvent *)
{
  QMouseEvent me(QEvent::MouseMove,mapFromGlobal(QCursor::pos()),
                 Qt::NoButton,Qt::NoButton,Qt::NoModifier);
  emit mousemove(me);
  // cout << "moved" << endl;
}

void
VBCanvas::leaveEvent(QEvent *)
{
  emit leftcanvas();
}

// void
// VBCanvas::keyPressEvent(QKeyEvent *ke)
// {
//   emit keypress(*ke);
// }
