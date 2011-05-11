
using namespace std;

#include "qwidget.h"
#include <iostream>
#include "vbqt_canvas.moc.h"

VBCanvas::VBCanvas(QWidget *parent,const char *name,WFlags f)
  : QWidget(parent,name)
{
  setWFlags(WResizeNoErase);
  setWFlags(WRepaintNoErase);
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
  if (!cim)
    return;

  QRect myrect=visibleRect();
  QPainter paint(this);
  paint.drawImage(myrect.x(),myrect.y(),*cim,myrect.x(),myrect.y(),myrect.width(),myrect.height());
}

void
VBCanvas::updateVisibleNow(QRect &r)
{
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

// void
// VBCanvas::keyPressEvent(QKeyEvent *ke)
// {
//   emit keypress(*ke);
// }
