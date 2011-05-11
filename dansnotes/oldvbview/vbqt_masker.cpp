
using namespace std;

#include <qcolordialog.h>
#include <qlineedit.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qpaintdevice.h>

//#include "colorbox_A.xpm"
//#include "colorbox_M.xpm"
//#include "colorbox_R.xpm"

#include "vbutil.h"
#include "vbqt_masker.moc.h"

QPixmap *VBQT_colorbox::pm_a=NULL;
QPixmap *VBQT_colorbox::pm_m=NULL;
QPixmap *VBQT_colorbox::pm_r=NULL;

QTMaskWidget::QTMaskWidget(QWidget *parent,const char *name,string maskname)
  : QHBox(parent,name)
{
  QWidget *ww;   // for spacers if needed

  this->setFixedHeight(24);
  this->setPaletteForegroundColor(QColor(0,0,0));
  this->setPaletteBackgroundColor(QColor(255,255,255));
  this->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->setLineWidth(1);
  this->setSpacing(3);
  this->setMargin(0);
  // this->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);

  ww=new QWidget(this);  // left spacer

  char tmp[128];
  f_visible=1;
  f_selected=0;
  index=999;
  color.setRgb(255,100,100);
  label=maskname;

  cframe=new VBQT_colorbox(this);
  cframe->setFixedSize(20,20);
  cframe->setFrameStyle(QFrame::Box | QFrame::Plain);
  cframe->setLineWidth(1);
  // drawarea=new QWidget(cframe);

  // spacer
  //ww=new QWidget(this);
  //ww->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);

  // the widget for the label
  edit_label=new QLineEdit(maskname.c_str(),this);
  edit_label->setFrameStyle(QFrame::Box | QFrame::Plain);
  edit_label->setLineWidth(1);
  //edit_label->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
  QObject::connect(edit_label,SIGNAL(textChanged(const QString &)),this,SLOT(newlabel(const QString &)));

  // the widget for the index
  sprintf(tmp,"%d",index);
  edit_index=new QLineEdit(tmp,this);
  edit_index->setFixedWidth(28);
  edit_index->setFrameStyle(QFrame::Box | QFrame::Plain);
  edit_index->setLineWidth(1);
  QObject::connect(edit_index,SIGNAL(textChanged(const QString &)),this,SLOT(newindex(const QString &)));

  // on/off checkbox
  checkbox=new QCheckBox("",this);
  checkbox->setChecked(1);

  ww=new QWidget(this);  // right spacer

  // signals for the overall widget
  QObject::connect(cframe,SIGNAL(rightclicked()),this,SLOT(contextmenu()));
  QObject::connect(cframe,SIGNAL(clicked()),this,SLOT(clicked()));
  QObject::connect(cframe,SIGNAL(colorchanged(QColor)),this,SLOT(setnewcolor(QColor)));
  QObject::connect(checkbox,SIGNAL(toggled(bool)),this,SLOT(ctoggled(bool)));
}

void
QTMaskWidget::newlabel(const QString &text)
{
  label=text.utf8();
}

void
QTMaskWidget::newindex(const QString &text)
{
  index=strtol(text.latin1());
}

void
QTMaskWidget::setnewcolor(QColor newcolor)
{
  color=newcolor;
  // cframe->setcolor(color);
  update();
  emit changed();
}

void
QTMaskWidget::Toggle()
{
  checkbox->toggle();
}

void
QTMaskWidget::ctoggled(bool val)
{
  f_visible=1-f_visible;
  emit changed();
}

void
QTMaskWidget::clicked()
{
  // f_selected=1;
  emit selected(this);
}

void
QTMaskWidget::SetType(char c)
{
  type=c;
  cframe->type=c;
};

void
QTMaskWidget::contextmenu()
{
  QPopupMenu foo(this);
  foo.insertItem("set color",1);
  foo.insertItem("toggle",2);
  // foo.insertItem("delete",3); 
  foo.insertItem("info",4);
  // foo.insertItem("stat",5);
  int ret=foo.exec(QCursor::pos());
  if (ret==1)
    cframe->getnewcolor();
  else if (ret==2)
    Toggle();
  else if (ret==3)
    cout << "not implemented" << endl;  // emit deleteme();
  else if (ret==4)
    emit inforequested(this);
  else if (ret==5)
    emit statrequested(this);
}

void
QTMaskWidget::setwidth(int w,int h)
{
  // cout << w << endl;
  setFixedWidth(w);
  update();
}

void
QTMaskWidget::SetColor(QColor &c)
{
  cframe->setPaletteBackgroundColor(c);
  color=c;
  update();
}

void
QTMaskWidget::SetIndex(int ind)
{
  index=ind;
  char tmp[128];
  sprintf(tmp,"%d",index);
  edit_index->setText(tmp);
}

void
QTMaskWidget::SetLabel(string &str)
{
  edit_label->setText(str.c_str());
}

void
QTMaskWidget::GetInfo(QColor &col,int &ind,string &lab)
{
  col=color;
  ind=index;
  lab=label;
}

void
QTMaskWidget::paintEvent(QPaintEvent *)
{
  if (f_selected)
    setPaletteBackgroundColor(QColor(255,230,0));
  else
    setPaletteBackgroundColor(QColor(255,255,255));
  
//   // update();
//   return;
//   QPainter paint(drawarea);
//   paint.fillRect(0,0,20,20,color);
//   if (f_selected) {
//     QPainter foo(this);
//     // QColor mycolor(200,200,0);  // GOLD
//     // QColor mycolor(150,250,50);  // GREENISH
//     QColor mycolor(255,230,0);  // ORANGEISH
//     foo.fillRect(0,0,500,500,mycolor);
//   }
  // drawarea->update();
  cframe->update();
  edit_label->update();
  edit_index->update();
}

void
VBQT_colorbox::xpaintEvent(QPaintEvent *)
{
  return;
  if (pm_a==NULL) {
    //pm_a=new QPixmap((const char **)colorbox_A_xpm);
    //pm_m=new QPixmap((const char **)colorbox_M_xpm);
    //pm_r=new QPixmap((const char **)colorbox_R_xpm);
  }
  // QPainter paint(this);
  //paint.fillRect(0,0,20,20,color);
  if (type=='M')
    bitBlt(this,0,0,pm_m,0,0,20,20);
  else if (type=='R')
    bitBlt(this,0,0,pm_r,0,0,20,20);
//   else if (type=="A")
//     bitBlt(this,0,0,pm_a,0,0,20,20);
//   else if (type=="L")
//     bitBlt(this,0,0,pm_a,0,0,20,20);
}

VBQT_colorbox::VBQT_colorbox(QWidget *parent,const char *name)
  : QVBox(parent,name)
{
  setMouseTracking(0);
  color=QColor(100,100,100);
  // setFixedSize(20,20);
  // setFrameStyle(QFrame::Box | QFrame::Plain);
  // setLineWidth(1);
  editable=1;
}

void
VBQT_colorbox::mousePressEvent(QMouseEvent *me)
{
  if (me->button()==QMouseEvent::RightButton)
    emit rightclicked();
  else
    emit clicked();
}

// void
// VBQT_colorbox::mouseDoubleClickEvent(QMouseEvent *me)
// {
//   // double-click to set color is too easy to do by accident
//   //   if (editable)
//   //     getnewcolor();
// }

void
VBQT_colorbox::getnewcolor()
{
  QColor tmp=QColorDialog::getColor(color);
  if (tmp.isValid()) {
    color=tmp;
    setPaletteBackgroundColor(color);
    update();
    emit colorchanged(tmp);
  }
}

void
VBQT_colorbox::setcolor(QColor newcolor)
{
  color=newcolor;
  setPaletteBackgroundColor(color);
  update();
}

QColor
VBQT_colorbox::getcolor()
{
  return color;
}

void
QTMaskList::viewportResizeEvent(QResizeEvent *rw)
{
  // emit newsize(rw->size().width(),rw->size().height());
  emit newsize(visibleWidth(),visibleHeight());
}

QTMaskList::QTMaskList(QWidget *parent)
  : QScrollView(parent)
{
}
