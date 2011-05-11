
#include "dbqimage.moc.h"
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QBuffer>

using namespace std;

DBQImage::DBQImage(QWidget *parent)
  : DBQScoreBox(parent)
{
  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(0);
  // layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

  iscene=new QGraphicsScene;
  iview=new QGraphicsView(iscene);
  iview->scale(0.5,0.5);
  iview->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  QPixmap tmpp("aaa.png");
  ipix=iscene->addPixmap(tmpp);
  tmpp.save("foo.jpg");
  ipix->setVisible(1);
  layout->addWidget(iview);
  layout->setStretchFactor(iview,20);
  iview->show();

  // unsetlabel=new QLabel;
  // unsetlabel->setPixmap(QPixmap(":/icons/icon_noimage.png"));
  // layout->addWidget(unsetlabel);

  button_load=new QPushButton("load");
  layout->addWidget(button_load);
 
  layout->insertStretch(-1,0);

  button_delete=new QPushButton("set",this);
  layout->addWidget(button_delete);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);
  // buttons aligned to top
  layout->setAlignment(button_delete,Qt::AlignTop);
  layout->setAlignment(button_revert,Qt::AlignTop);

  QObject::connect(button_load,SIGNAL(clicked()),this,SLOT(loadclicked()));
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
}

void
DBQImage::setValue(const DBscorevalue &val)  // dblock &block)
{
  pm.loadFromData(val.v_pixmap.data,val.v_pixmap.size);
  originalvalue=pm;
  ipix->setPixmap(pm);
  f_originallyset=1;
  f_set=1;
  updateAppearance();
}

void
DBQImage::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQImage::updateAppearance()
{
  DBQScoreBox::updateAppearance();
  if (f_editable && f_set) {
    button_load->show();
  }
  else {
    button_load->hide();
  }
  if (f_set) {
    iview->show();
    //unsetlabel->hide();
  }
  else {
    iview->hide();
    //unsetlabel->show();
  }
}

void
DBQImage::loadclicked()
{
  QString fn=QFileDialog::getOpenFileName();
  QPixmap tmpp;
  if (fn.size()==0) return;
  if (!tmpp.load(fn)) {
    // FIXME put up some kind of error dialog
    QMessageBox::warning(0,"Error",QString("Couldn't open file ")+fn);
    return;
  }
  f_dirty=1;
  f_set=1;
  ipix->setPixmap(tmpp);

  updateAppearance();
}

void
DBQImage::revertclicked()
{
  f_dirty=0;
  f_set=f_originallyset;
  ipix->setPixmap(originalvalue);
  updateAppearance();
}

void
DBQImage::deleteclicked()
{
  if (f_set) f_set=0;
  else f_set=1;
  updateAppearance();
}

void
DBQImage::getValue(DBscorevalue &val)
{
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  pm.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
  val.v_pixmap.init((uint8 *)(bytes.data()),bytes.size());
  val.scorename=scorename;
}

