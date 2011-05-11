
using namespace std;

#include "qwidget.h"
#include "qframe.h"
#include "qcolordialog.h"
#include "qpopupmenu.h"
#include "qcursor.h"
#include "qclipboard.h"
#include "qfiledialog.h"
#include <iostream>
#include "vbqt_scalewidget.moc.h"

VBScalewidget::VBScalewidget(QWidget *parent,const char *name,WFlags f)
  : QFrame(parent,name)
{
  setFrameStyle(QFrame::Box|QFrame::Plain);
  q_low=0.0;
  q_high=10.0;
  q_negcolor1=QColor(0,0,200);
  q_negcolor2=QColor(0,0,255);
  q_poscolor1=QColor(200,0,0);
  q_poscolor2=QColor(255,0,0);
  setWFlags(WResizeNoErase);
  setWFlags(WRepaintNoErase);
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
VBScalewidget::drawContents(QPainter *paint)
{
  drawcolorbars(paint,contentsRect(),12);
}

void
VBScalewidget::drawcolorbars(QPainter *paint,QRect myrect,int fontsize)
{
  paint->setPen(QColor(0,0,0));
  paint->setBackgroundColor(QColor(255,255,255));
  paint->eraseRect(myrect);
  QFont ff("Helvetica");
  ff.setPixelSize(fontsize);
  paint->setFont(ff);

  int rr,gg,bb;
  float pct;
  char tmp[64];

  int xoff=myrect.left();
  int yoff=myrect.top();
  int s_width=(myrect.width()-36)/2;
  int s_height=myrect.height()-fontsize-10;
  q_top=yoff+5;
  q_bottom=q_top+s_height;
  q_lowleft=xoff+12;
  q_lowright=xoff+12+s_width;
  q_highleft=xoff+24+s_width;
  q_highright=xoff+24+s_width+s_width;

  // NEG FIRST
  paint->fillRect(q_lowleft,q_top,s_width,s_height,QColor(0,0,0));
  for (int i=q_lowleft+1; i<=q_lowright-2; i++) {
    pct=(float)(i-(q_lowleft+1))/((q_lowright-2)-(q_lowleft+1));
    rr=q_negcolor2.red()+(int)(pct*(q_negcolor1.red()-q_negcolor2.red()));
    gg=q_negcolor2.green()+(int)(pct*(q_negcolor1.green()-q_negcolor2.green()));
    bb=q_negcolor2.blue()+(int)(pct*(q_negcolor1.blue()-q_negcolor2.blue()));
    paint->setPen(QColor(rr,gg,bb));
    paint->drawLine(i,q_top+1,i,q_bottom-2);
  }
  paint->setPen(QColor(0,0,0));
  sprintf(tmp,"%g",q_high*-1);
  paint->drawText(q_lowleft,7+s_height,s_width/2,fontsize*2,Qt::AlignLeft,QString(tmp));
  sprintf(tmp,"%g",q_low*-1);
  paint->drawText(q_lowright-s_width/2,7+s_height,s_width/2,fontsize*2,Qt::AlignRight,QString(tmp));


  // POS SECOND
  paint->fillRect(q_highleft,q_top,s_width,s_height,QColor(0,0,0));
  for (int i=q_highleft+1; i<=q_highright-2; i++) {
    pct=(float)(i-(q_highleft+1))/((q_highright-2)-(q_highleft+1));
    rr=q_poscolor1.red()+(int)(pct*(q_poscolor2.red()-q_poscolor1.red()));
    gg=q_poscolor1.green()+(int)(pct*(q_poscolor2.green()-q_poscolor1.green()));
    bb=q_poscolor1.blue()+(int)(pct*(q_poscolor2.blue()-q_poscolor1.blue()));
    paint->setPen(QColor(rr,gg,bb));
    paint->drawLine(i,q_top+1,i,q_bottom-2);
  }
  paint->setPen(QColor(0,0,0));
  sprintf(tmp,"%g",q_low);
  paint->drawText(q_highleft,7+s_height,s_width/2,fontsize*2,Qt::AlignLeft,QString(tmp));
  sprintf(tmp,"%g",q_high);
  paint->drawText(q_highright-s_width/2,7+s_height,s_width/2,fontsize*2,Qt::AlignRight,QString(tmp));
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
  if (me->y()<q_top || me->y()>q_bottom)
    return;
  int n1=me->x()-q_lowleft;
  int n2=me->x()-q_lowright;
  int p1=me->x()-q_highleft;
  int p2=me->x()-q_highright;
  if (n1>=0 && n2<=0) {
    if (abs(n1)<abs(n2)) {
      QColor tmp=QColorDialog::getColor(q_negcolor2);
      if (tmp.isValid()) {
        q_negcolor2=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
    else {
      // n2
      QColor tmp=QColorDialog::getColor(q_negcolor1);
      if (tmp.isValid()) {
        q_negcolor1=tmp;
        emit newcolors(q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
        update(contentsRect());
      }
    }
  }
  else if (p1>=0 && p2<=0) {
    if (abs(p1)<abs(p2)) {
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
  QPopupMenu foo(this);
  foo.insertItem("Copy to Clipboard",1);
  foo.insertItem("Save PNG",2);
  foo.insertItem("Copy Clipboard (highres)",3);
  foo.insertItem("Save PNG (highres)",4);
  int ret=foo.exec(QCursor::pos());
  QPixmap pm;

  // grab the lowres or highres pixmap
  if (ret==1 || ret==2) {
    pm=QPixmap::grabWidget(this,contentsRect().left(),contentsRect().top(),
                           contentsRect().width(),contentsRect().height());
  }
  else if (ret==3 || ret==4) {
    int ww=1200,hh=150,ff=48;
    pm.resize(ww,hh);
    QPainter pp(&pm);
    drawcolorbars(&pp,QRect(0,0,ww,hh),ff);
    pp.end();
  }
  // copy to clipboard or save to file
  if (ret==1||ret==3) {
    QClipboard *cb=QApplication::clipboard();
    cb->setPixmap(pm);    
  }
  else if (ret==2||ret==4) {
    QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image file","Choose a filename for the scale widget");
    if (s!=QString::null)
      pm.save(s.latin1(),"PNG");;
  }
}
