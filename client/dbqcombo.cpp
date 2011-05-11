
#include "dbqcombo.moc.h"

using namespace std;

DBQCombo::DBQCombo(QWidget *parent)
  : DBQScoreBox(parent)
{
  otherindex=-1;
  currentvalue=originalvalue="";
  f_set=0;
  f_originallyset=0;

  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(1);
  // layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

  cb=new QComboBox();
  cb->setEditable(0);
  // cb->setEditText("");
  // cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  // cb->addItem("");
  //originalindex=0;
  layout->addWidget(cb);
  layout->setStretchFactor(cb,2);

  ee=new QLineEdit();
  ee->setText("");
  layout->addWidget(ee);
  layout->setStretchFactor(ee,2);
  ee->hide();

  valueline=new QLineEdit();
  valueline->setText("<nodata>");
  valueline->setFrame(0);
  valueline->setReadOnly(1);
  layout->addWidget(valueline);
  layout->setStretchFactor(valueline,2);
  valueline->hide();
 
  layout->insertStretch(-1,0);

  button_delete=new QPushButton("set",this);
  layout->addWidget(button_delete);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);
  
  //originalindex=0;
  originalvalue="";
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
  QObject::connect(ee,SIGNAL(textEdited(const QString &)),this,SLOT(textedited()));
  // QObject::connect(cb,SIGNAL(currentIndexChanged(int)),this,SLOT(indexchanged()));
  QObject::connect(cb,SIGNAL(activated(int)),this,SLOT(indexchanged()));
}

void
DBQCombo::clearValues()
{
  while (cb->count())
    cb->removeItem(0);
  f_dirty=0;
  otherindex=-1;
  updateAppearance();
}

void
DBQCombo::addValue(const string &value)
{
  cb->addItem(value.c_str());
  if ((int)value.size()>cb->minimumContentsLength())
    cb->setMinimumContentsLength(value.size());
  if (cb->count()==1) {
    originalvalue=currentvalue=value;
    valueline->setText(value.c_str());
    cb->setCurrentIndex(0);
  }
  // updateAppearance();
}

void
DBQCombo::allowOther()
{
  cb->addItem("other...");
  otherindex=cb->count()-1;
}

void
DBQCombo::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQCombo::updateAppearance()
{
  if (f_set != f_originallyset)
    f_dirty=1;
  else if (!f_set)
    f_dirty=0;
  else if (currentvalue==originalvalue)
    f_dirty=0;
  else
    f_dirty=1;
  valueline->setText(currentvalue.c_str());
  DBQScoreBox::updateAppearance();
  int ind=-1;
  if ((ind=cb->findText(QString(currentvalue.c_str())))!=-1) {
    cb->setCurrentIndex(ind);
  }
  else if (otherindex>-1) {
    ee->setText(currentvalue.c_str());
    cb->setCurrentIndex(otherindex);
  }
  // shouldn't happen!
  else {
    cout << "shouldn't happen!" << endl;
    cb->addItem(currentvalue.c_str());
    cb->setCurrentIndex(cb->count()-1);
  }

  if (f_editable && f_set) {
    cb->show();
    if (otherindex==cb->currentIndex()) {
      ee->show();
      ee->setEnabled(1);
    }
    else {
      ee->hide();
      ee->setEnabled(0);
    }
  }
  else {
    cb->hide();
    ee->hide();
  }
}

void
DBQCombo::setValue(const DBscorevalue &val)
{
  originalvalue=currentvalue=val.v_string;
  f_set=f_originallyset=1;
  // if there's no "other" option and this item is not in the list, add it
  if (otherindex==-1 && cb->findText(currentvalue.c_str())==-1)
    cb->addItem(currentvalue.c_str());
  updateAppearance();
}

void
DBQCombo::deleteclicked()
{
  f_set=!f_set;
  updateAppearance();
  if (f_set)
    cb->setFocus(Qt::OtherFocusReason);
}

void
DBQCombo::revertclicked()
{
  f_set=f_originallyset;
  if (f_originallyset)
    currentvalue=originalvalue;
  updateAppearance();
  if (f_set)
    cb->setFocus(Qt::OtherFocusReason);
}

void
DBQCombo::indexchanged()
{
  f_set=1;
  if (cb->currentIndex()==otherindex)
    currentvalue=ee->text().toStdString();
  else
    currentvalue=cb->currentText().toStdString();
  updateAppearance();
}

void
DBQCombo::textedited()
{
  currentvalue=ee->text().toStdString();
  updateAppearance();
}

void
DBQCombo::getValue(DBscorevalue &val)
{
  val.v_string=valueline->text().toStdString();
  val.scorename=scorename;
}
