
#include "dbqbrain.moc.h"
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>

using namespace std;

DBQBrain::DBQBrain(QWidget *parent)
  : DBQScoreBox(parent)
{
  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(0);
  // layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

  iview=new VBView;
  iview->setMinimal(1);
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
DBQBrain::setValue(const DBscorevalue &val) // const Cube &newcube)
{
  cb=val.v_cube;
  f_originallyset=1;
  f_set=1;
  updateAppearance();
}

void
DBQBrain::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQBrain::updateAppearance()
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
DBQBrain::loadclicked()
{
  if (iview->LoadImage())
    return;
  f_dirty=1;
  f_set=1;
  updateAppearance();
  return;

  QString fn=QFileDialog::getOpenFileName();
  if (fn.size()==0) return;
  if (cb.ReadFile(fn.toStdString())) {
    QMessageBox::warning(0,"Error",QString("Couldn't open file ")+fn);
    return;
  }
  f_dirty=1;
  f_set=1;
  updateAppearance();
}

void
DBQBrain::revertclicked()
{
  f_dirty=0;
  f_set=f_originallyset;
  cb=originalvalue;
  updateAppearance();
}

void
DBQBrain::deleteclicked()
{
  if (f_set) f_set=0;
  else f_set=1;
  updateAppearance();
}

void
DBQBrain::getValue(DBscorevalue &val)
{
  val.v_cube=cb;
  val.scorename=scorename;
  return;
}

