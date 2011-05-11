
#include "dbqscorebox.moc.h"
#include <QPainter>

using namespace std;

DBQScoreBox::DBQScoreBox(QWidget *parent)
  : QFrame(parent)
{
  f_editable=0;
  f_dirty=0;
  f_set=0;
  f_originallyset=0;

  layout=NULL;
  childlayout=NULL;
  label=NULL;
  button_delete=NULL;
  button_revert=NULL;
  // valueline=NULL;

  // scorenameid and scorevalueid should be set explicitly, but...
  scorename="";
  scorevalueid=0;
  setAutoFillBackground(1);

  style_clean="";
  style_dirty="";
  style_deleted="*{margin:0;padding:0;text-decoration:line-through;}";
  style_deleted="QLineEdit {text-decoration:line-through;}";

  style_labelclean="QLabel {color:darkblue;font-weight:bold;text-align:right;margin:2;}";
  style_labeldirty="QLabel {color:darkred;font-weight:bold;text-align:right;margin:2;}";
  style_labeldeleted="QLabel {color:darkred;font-weight:bold;text-align:right;margin:2;text-decoration:line-through;}";
}

void
DBQScoreBox::updateAppearance()
{
  if (f_editable) {
    button_delete->show();
    button_revert->show();
  }
  else {
    button_delete->hide();
    button_revert->hide();
  }

  if (f_set==1)
    button_delete->setText("delete");
  else
    button_delete->setText("set");

  // if (f_set==1 && f_originallyset==0)
  //   f_dirty=1;
  // if (f_set==0 && f_originallyset==0) {
  //   f_dirty=0;
  // }

  if (f_set==0 && f_originallyset==1) {
    setStyleSheet(style_deleted.c_str());
    if (label) label->setStyleSheet(style_labeldeleted.c_str());
  }
  else if (f_dirty) {
    setStyleSheet(style_dirty.c_str());
    if (label) label->setStyleSheet(style_labeldirty.c_str());
  }
  else {
    setStyleSheet(style_clean.c_str());
    if (label) label->setStyleSheet(style_labelclean.c_str());
  }
}
