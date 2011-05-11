
#include "dbqbool.moc.h"

using namespace std;

DBQBool::DBQBool(QWidget *parent)
  : DBQScoreBox(parent)
{
  otherindex=-1;

  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(1);
  // layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

//   cb=new QCheckBox();
//   cb->setTristate(1);
//   cb->setCheckState(Qt::PartiallyChecked);
//   layout->addWidget(cb);
  cb=new QComboBox();
  cb->addItem("<no value>");
  cb->addItem("no");
  cb->addItem("yes");

  layout->insertStretch(-1,0);

  button_new=new QPushButton("new",this);
  layout->addWidget(button_new);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);
  button_delete=new QPushButton("delete",this);
  layout->addWidget(button_delete);
  
  originalvalue="";
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(button_new,SIGNAL(clicked()),this,SLOT(newclicked()));
  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
  QObject::connect(cb,SIGNAL(stateChanged(int)),this,SLOT(changed(int)));
}

void
DBQBool::changed(int s)
{
  f_dirty=1;
  updateAppearance();
}

void
DBQBool::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQBool::updateAppearance()
{
  if (getValue()==originalvalue)
    f_dirty=0;
  else
    f_dirty=1;
  DBQScoreBox::updateAppearance();
  if (f_editable && !f_deleted) {
    button_new->show();
    button_revert->show();
    button_delete->show();
    cb->setCheckable(1);
  }
  else {
    button_new->hide();
    button_revert->hide();
    button_delete->hide();
    cb->setCheckable(0);
  }
}

void
DBQBool::setLabel(const string &lab)
{
  cb->setText(lab.c_str());
}

void
DBQBool::setValue(const DBscorevalue &val)
{
  if (val.v_string=="yes") {
    cb->setCheckState(Qt::Checked);
    originalvalue="yes";
  }
  else if (val.v_string=="no") {
    cb->setCheckState(Qt::Unchecked);
    originalvalue="no";
  }
  else {
    cb->setCheckState(Qt::PartiallyChecked);
    originalvalue="";
  }
  f_dirty=0;
  updateAppearance();
}

void
DBQBool::newclicked()
{
}

void
DBQBool::revertclicked()
{
  f_deleted=0;
  f_dirty=0;
  setValue(originalvalue);
}

void
DBQBool::deleteclicked()
{
  if (f_deleted)
    f_deleted=0;
  else
    f_deleted=1;
  updateAppearance();
}

string
DBQBool::getValue()
{
  return (cb->currentText().toStdString());
//   if (cb->currentText()=="yes")
//     return "yes";
//   if (cb->checkState()==Qt::Unchecked)
//     return "no";
//   if (cb->checkState()==Qt::Checked)
//     return "yes";
//   return "";
}
