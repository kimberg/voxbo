
#include "dbqlineedit.moc.h"
#include <QValidator>

using namespace std;

DBQLineEdit::DBQLineEdit(QWidget *parent)
  : DBQScoreBox(parent)
{
  originalvalue="";

  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(0);
  // layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

  ee=new QLineEdit();
  layout->addWidget(ee);
  layout->setStretchFactor(ee,20);
  ee->setFrame(0);
  ee->setReadOnly(0);
  ee->setText(originalvalue.c_str());
  // ee->selectAll();
  
  layout->insertStretch(-1,0);

  button_delete=new QPushButton("delete",this);
  layout->addWidget(button_delete);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);
  
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
  QObject::connect(ee,SIGNAL(textEdited(const QString &)),this,SLOT(textedited(const QString &)));
}

void
DBQLineEdit::setValue(const DBscorevalue &val)
{
  originalvalue=val.v_string;
  ee->setText(originalvalue.c_str());
  ee->selectAll();
  f_originallyset=1;
  f_set=1;
  updateAppearance();
}

void
DBQLineEdit::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQLineEdit::setIntValidator(int32 min,int32 max)
{
  QValidator *vv=new QIntValidator(min,max,this);
  ee->setValidator(vv);
}

void
DBQLineEdit::setDoubleValidator(double min,double max,int decimals)
{
  QDoubleValidator *vv=new QDoubleValidator(min,max,decimals,this);
  vv->setNotation(QDoubleValidator::StandardNotation);
  ee->setValidator(vv);
}

void
DBQLineEdit::updateAppearance()
{
  if (f_set != f_originallyset ||
      (f_set && (ee->text().toStdString()==originalvalue)))
    f_dirty=1;
  else
    f_dirty=0;
  DBQScoreBox::updateAppearance();
  if (f_editable)
    ee->setReadOnly(0);
  else
    ee->setReadOnly(1);
  if (f_set)
    ee->show();
  else
    ee->hide();
}

void
DBQLineEdit::revertclicked()
{
  f_set=f_originallyset;
  if (f_originallyset)
    ee->setText(originalvalue.c_str());
  updateAppearance();
  if (f_set)
    ee->setFocus(Qt::OtherFocusReason);
}

void
DBQLineEdit::deleteclicked()
{
  f_set=!f_set;
  updateAppearance();
  if (f_set)
    ee->setFocus(Qt::OtherFocusReason);
}

void
DBQLineEdit::textedited(const QString &)
{
  updateAppearance();
}

void
DBQLineEdit::getValue(DBscorevalue &val)
{
  val.v_string=ee->text().toStdString();
  val.scorename=scorename;
  return;
}
