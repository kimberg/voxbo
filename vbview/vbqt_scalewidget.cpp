
// vbqt_scalewidget.cpp
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

#include <QApplication>
#include <QPixmap>
#include <QMouseEvent>
#include <QFrame>
#include <QMenu>
#include "qcolordialog.h"
#include "qcursor.h"
#include "qclipboard.h"
#include <QFileDialog>
#include <iostream>
#include "vbqt_scalewidget.moc.h"

VBScalewidget::VBScalewidget(QWidget *parent)
  : QFrame(parent)
{
  setFrameStyle(QFrame::Box|QFrame::Plain);
  q_low=0.0;
  q_high=10.0;
  q_negcolor1=QColor(0,0,200);
  q_negcolor2=QColor(0,0,255);
  q_poscolor1=QColor(200,0,0);
  q_poscolor2=QColor(255,0,0);
}

void
VBScalewidget::paintEvent(QPaintEvent *)
{
  QPainter pp(this);
  drawcolorbars(&pp,QRect(0,0,width(),height()),12);
}

void
VBScalewidget::setscale(float low,float high,QColor negcolor1,QColor negcolor2,
         QColor poscolor1,QColor poscolor2)
{
  q_low=low;
  q_high=high;
  q_negcolor1=negcolor1;
  q_negcolor2=negcolor2;
  q_poscolor1=poscolor1;
  q_poscolor2=poscolor2;
  update();
}

void
VBScalewidget::setscale(float low,float high,QColor poscolor1,QColor poscolor2)
{
  q_low=low;
  q_high=high;
  q_negcolor1.setAlpha(0);
  q_negcolor2.setAlpha(0);
  q_poscolor1=poscolor1;
  q_poscolor2=poscolor2;
  update();
}

void
VBScalewidget::drawContents(QPainter *paint)
{
  drawcolorbars(paint,contentsRect(),12);
}

void
VBScalewidget::drawcolorbars(QPainter *paint,QRect myrect,int fontsize)
{
  f_vertical=0;
  if (myrect.height() > myrect.width()) f_vertical=1;
  paint->setPen(QColor(0,0,0));
  paint->setBackgroundColor(QColor(255,255,255));
  paint->eraseRect(myrect);
  QFont ff("Helvetica");
  ff.setPixelSize(fontsize);
  paint->setFont(ff);
  const int fontwidth=QFontMetrics(ff).width("9.9999");
  const int ISPACE=QFontMetrics(ff).width("i");

  bool f_neg=q_negcolor1.alpha();

  char tmp[64];
  int xoff=myrect.left();
  int yoff=myrect.top();
  const int SPACE=fontsize/2;

  if (f_vertical && f_neg) {
    int barsize=(myrect.height()-SPACE-SPACE-SPACE)/2;
    posrect.setRect(xoff+SPACE,yoff+SPACE,myrect.width()-SPACE-fontwidth,barsize);
    negrect.setRect(xoff+SPACE,yoff+SPACE+barsize+SPACE,myrect.width()-SPACE-fontwidth,barsize);
  }
  else if (f_vertical && !f_neg) {
    int barsize=myrect.height()-SPACE-SPACE;
    posrect.setRect(xoff+SPACE,yoff+SPACE,myrect.width()-SPACE-fontwidth,barsize);
  }
  if (!f_vertical && f_neg) {
    int barsize=(myrect.width()-SPACE-SPACE-SPACE)/2;
    negrect.setRect(xoff+SPACE,yoff+SPACE,barsize,myrect.height()-SPACE-fontsize);
    posrect.setRect(xoff+SPACE+barsize+SPACE,yoff+SPACE,barsize,myrect.height()-SPACE-fontsize);
  }
  if (!f_vertical && !f_neg) {
    int barsize=myrect.width()-SPACE-SPACE;
    posrect.setRect(xoff+SPACE,yoff+SPACE,barsize,myrect.height()-SPACE-fontsize);
  }

  // black rect that will eventually leave just the hairline
  paint->fillRect(posrect,QColor(0,0,0));
  if (f_neg) paint->fillRect(negrect,QColor(0,0,0));

  // now the colors and labels
  if (f_neg && f_vertical) {
    shaderect(paint,negrect,q_negcolor1,q_negcolor2);
    paint->setPen(QColor(0,0,0));
    sprintf(tmp,"%g",q_high*-1); 
    paint->drawText(negrect.right()+ISPACE,negrect.top(),myrect.width()-negrect.right(),negrect.height(),Qt::AlignLeft|Qt::AlignBottom,QString(tmp));
    sprintf(tmp,"%g",q_low*-1);
    paint->drawText(negrect.right()+ISPACE,negrect.top(),myrect.width()-negrect.right(),negrect.height(),Qt::AlignLeft|Qt::AlignTop,QString(tmp));
  }
  else if (f_neg && !f_vertical) {
    shaderect(paint,negrect,q_negcolor2,q_negcolor1);
    paint->setPen(QColor(0,0,0));
    sprintf(tmp,"%g",q_high*-1);
    paint->drawText(negrect.left(),negrect.bottom(),negrect.width(),myrect.height()-negrect.bottom(),Qt::AlignLeft|Qt::AlignTop,QString(tmp));
    sprintf(tmp,"%g",q_low*-1);
    paint->drawText(negrect.left(),negrect.bottom(),negrect.width(),myrect.height()-negrect.bottom(),Qt::AlignRight|Qt::AlignTop,QString(tmp));
  }
  // NOW THE POSITIVE BAR
  if (f_vertical) {
    shaderect(paint,posrect,q_poscolor2,q_poscolor1);
    paint->setPen(QColor(0,0,0));
    sprintf(tmp,"%g",q_low);
    paint->drawText(posrect.right()+ISPACE,posrect.top(),myrect.width()-posrect.right(),posrect.height(),Qt::AlignLeft|Qt::AlignBottom,QString(tmp));
    sprintf(tmp,"%g",q_high);
    paint->drawText(posrect.right()+ISPACE,posrect.top(),myrect.width()-posrect.right(),posrect.height(),Qt::AlignLeft|Qt::AlignTop,QString(tmp));
  }
  else {
    shaderect(paint,posrect,q_poscolor1,q_poscolor2);
    paint->setPen(QColor(0,0,0));
    sprintf(tmp,"%g",q_low);
    paint->drawText(posrect.left(),posrect.bottom(),posrect.width(),myrect.height()-posrect.bottom(),Qt::AlignLeft|Qt::AlignTop,QString(tmp));
    sprintf(tmp,"%g",q_high);
    paint->drawText(posrect.left(),posrect.bottom(),posrect.width(),myrect.height()-posrect.bottom(),Qt::AlignRight|Qt::AlignTop,QString(tmp));
  }
}

void
VBScalewidget::shaderect(QPainter *paint,QRect &r,QColor &c1,QColor &c2)
{
  int rr,gg,bb;
  float pct;
  // if it's horizontal, we go left to right, vertical we go top to bottom
  if (f_vertical) {
    for (int i=r.top()+1; i<r.bottom(); i++) {
      pct=(float)(i-(r.top()+1))/((r.bottom()-1)-(r.top()+1.0));
      rr=c1.red()+(int)(pct*(c2.red()-c1.red()));
      gg=c1.green()+(int)(pct*(c2.green()-c1.green()));
      bb=c1.blue()+(int)(pct*(c2.blue()-c1.blue()));
      paint->setPen(QColor(rr,gg,bb));
      paint->drawLine(r.left()+1,i,r.right()-1,i);
    }
  }
  else {
    for (int i=r.left()+1; i<r.right(); i++) {
      pct=(float)(i-(r.left()+1))/((r.right()-1)-(r.left()+1));
      rr=c1.red()+(int)(pct*(c2.red()-c1.red()));
      gg=c1.green()+(int)(pct*(c2.green()-c1.green()));
      bb=c1.blue()+(int)(pct*(c2.blue()-c1.blue()));
      paint->setPen(QColor(rr,gg,bb));
      paint->drawLine(i,r.top()+1,i,r.bottom()-1);
    }
  }
}


void
VBScalewidget::mousePressEvent(QMouseEvent *me)
{
  if (me->button()==Qt::RightButton) {
    contextmenu();
    return;
  }
}

void
VBScalewidget::mouseDoubleClickEvent(QMouseEvent *me)
{
  if (negrect.contains(me->x(),me->y())) {
    if ((f_vertical && me->y() < negrect.top()+(negrect.bottom()-negrect.top())/2) ||
        (!f_vertical && me->x() <negrect.left()+(negrect.right()-negrect.left())/2)) {
      QColor tmp=QColorDialog::getColor(q_negcolor2);
      if (tmp.isValid()) {
        q_negcolor2=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
    else {
      QColor tmp=QColorDialog::getColor(q_negcolor1);
      if (tmp.isValid()) {
        q_negcolor1=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
  }
  else if (posrect.contains(me->x(),me->y())) {
    if ((f_vertical && me->y() > posrect.top()+(posrect.bottom()-posrect.top())/2) ||
        (!f_vertical && me->x() < posrect.left()+(posrect.right()-posrect.left())/2)  ) {
      // p1
      QColor tmp=QColorDialog::getColor(q_poscolor1);
      if (tmp.isValid()) {
        q_poscolor1=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
    else {
      // p2
      QColor tmp=QColorDialog::getColor(q_poscolor2);
      if (tmp.isValid()) {
        q_poscolor2=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
  }
}





void
VBScalewidget::contextmenu()
{
  QMenu foo(this);
  QAction *copy1,*save1,*copy2,*save2,*copy3,*save3,*ret;
  copy1=foo.addAction("Copy to Clipboard");
  save1=foo.addAction("Save PNG");
  copy2=foo.addAction("Copy Clipboard (highres)");
  save2=foo.addAction("Save PNG (highres)");
  copy3=foo.addAction("Copy Clipboard vertical (highres)");
  save3=foo.addAction("Save PNG vertical (highres)");
  ret=foo.exec(QCursor::pos());
  QPixmap pm;

  // grab the lowres or highres pixmap
  if (ret==copy1 || ret==save1) {
    pm=QPixmap::grabWidget(this,contentsRect().left(),contentsRect().top(),
                           contentsRect().width(),contentsRect().height());
  }
  else if (ret==copy2 || ret==save2) {
    int ww=1200,hh=150,ff=48;
    pm.resize(ww,hh);
    QPainter pp(&pm);
    drawcolorbars(&pp,QRect(0,0,ww,hh),ff);
    pp.end();
  }
  else if (ret==copy3 || ret==save3) {
    int ww=250,hh=1200,ff=48;
    pm.resize(ww,hh);
    QPainter pp(&pm);
    drawcolorbars(&pp,QRect(0,0,ww,hh),ff);
    pp.end();
  }
  // copy to clipboard or save to file
  if (ret==copy1||ret==copy2||ret==copy3) {
    QClipboard *cb=QApplication::clipboard();
    cb->setPixmap(pm);    
  }
  else if (ret==save1||ret==save2||ret==save3) {
    QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image file","Choose a filename for the scale widget");
    if (s!=QString::null)
      pm.save(s.latin1(),"PNG");;
  }
}
