
#include "dbqtextedit.moc.h"

using namespace std;

DBQTextEdit::DBQTextEdit(QWidget *parent)
  : DBQScoreBox(parent)
{
  originalvalue="";

  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(0);
  this->setLayout(layout);

  ee=new QTextBrowser();
  layout->addWidget(ee);
  layout->setStretchFactor(ee,20);
  ee->setReadOnly(0);
  ee->setUndoRedoEnabled(1);
  ee->setText(originalvalue.c_str());
  
  buttonarea=new QWidget();
  QVBoxLayout *blayout=new QVBoxLayout();
  blayout->setSpacing(1);
  blayout->setContentsMargins(0,0,0,0);
  blayout->setAlignment(Qt::AlignTop);
  layout->addWidget(buttonarea);
  buttonarea->setLayout(blayout);
  button_bold=new QToolButton();
  button_bold->setIcon(QIcon(":/icons/icon_bold.png"));
  button_bold->setCheckable(1);
  button_ital=new QToolButton();
  button_ital->setIcon(QIcon(":/icons/icon_ital.png"));
  button_ital->setCheckable(1);
  button_line=new QToolButton();
  button_line->setIcon(QIcon(":/icons/icon_line.png"));
  button_line->setCheckable(1);
  button_black=new QToolButton();
  button_black->setIcon(QIcon(":/icons/icon_black.png"));
  button_black->setCheckable(1);
  button_red=new QToolButton();
  button_red->setIcon(QIcon(":/icons/icon_red.png"));
  button_red->setCheckable(1);
  button_blue=new QToolButton();
  button_blue->setIcon(QIcon(":/icons/icon_blue.png"));
  button_blue->setCheckable(1);
  button_green=new QToolButton();
  button_green->setIcon(QIcon(":/icons/icon_green.png"));
  button_green->setCheckable(1);
  blayout->addWidget(button_bold);
  blayout->addWidget(button_ital);
  blayout->addWidget(button_line);
  blayout->addWidget(button_black);
  blayout->addWidget(button_red);
  blayout->addWidget(button_blue);
  blayout->addWidget(button_green);

  // valueline=new QLineEdit();
  // valueline->setFrame(0);
  // valueline->setReadOnly(1);
  // valueline->setText("<nodata>");
  // layout->addWidget(valueline);
  // layout->setStretchFactor(valueline,2);
  // valueline->hide();
 
  layout->insertStretch(-1,0);

  button_delete=new QPushButton("set",this);
  layout->addWidget(button_delete);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);
  // buttons aligned to top
  layout->setAlignment(button_delete,Qt::AlignTop);
  layout->setAlignment(button_revert,Qt::AlignTop);

  QObject::connect(ee,SIGNAL(currentCharFormatChanged(const QTextCharFormat &)),this,SLOT(update_format(const QTextCharFormat &)));
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
  QObject::connect(ee,SIGNAL(textChanged()),this,SLOT(textedited()));

  QObject::connect(button_bold,SIGNAL(clicked(bool)),this,SLOT(toggle_bold(bool)));
  QObject::connect(button_ital,SIGNAL(clicked(bool)),this,SLOT(toggle_ital(bool)));
  QObject::connect(button_line,SIGNAL(clicked(bool)),this,SLOT(toggle_line(bool)));
  QObject::connect(button_black,SIGNAL(clicked(bool)),this,SLOT(toggle_black(bool)));
  QObject::connect(button_red,SIGNAL(clicked(bool)),this,SLOT(toggle_red(bool)));
  QObject::connect(button_blue,SIGNAL(clicked(bool)),this,SLOT(toggle_blue(bool)));
  QObject::connect(button_green,SIGNAL(clicked(bool)),this,SLOT(toggle_green(bool)));
}

void
DBQTextEdit::update_format(const QTextCharFormat &f)
{
  if (f.fontWeight()==QFont::Bold)
    button_bold->setChecked(1);
  else
    button_bold->setChecked(0);

  if (f.fontItalic())
    button_ital->setChecked(1);
  else
    button_ital->setChecked(0);
  
  if (f.fontUnderline())
    button_line->setChecked(1);
  else
    button_line->setChecked(0);

  QColor cc=f.foreground().color();

  if (cc==Qt::black)
    button_black->setChecked(1);
  else
    button_black->setChecked(0);

  if (cc==Qt::red)
    button_red->setChecked(1);
  else
    button_red->setChecked(0);

  if (cc==Qt::darkGreen)
    button_green->setChecked(1);
  else
    button_green->setChecked(0);

  if (cc==Qt::blue)
    button_blue->setChecked(1);
  else
    button_blue->setChecked(0);

}

void
DBQTextEdit::toggle_bold(bool t)
{
  if (t) ee->setFontWeight(QFont::Bold);
  else ee->setFontWeight(QFont::Normal);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_ital(bool t)
{
  ee->setFontItalic(t);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_line(bool t)
{
  ee->setFontUnderline(t);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_red(bool)
{
  ee->setTextColor(Qt::red);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_blue(bool)
{
  ee->setTextColor(Qt::blue);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_green(bool)
{
  ee->setTextColor(Qt::darkGreen);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::toggle_black(bool)
{
  ee->setTextColor(Qt::black);
  ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::setValue(const DBscorevalue &val)
{
  originalvalue=val.v_string;
  ee->setHtml(originalvalue.c_str());
  f_originallyset=1;
  f_set=1;
  updateAppearance();
}

void
DBQTextEdit::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQTextEdit::updateAppearance()
{
  if (f_set!=f_originallyset)
    f_dirty=1;
  else if (f_set && (string)ee->toHtml().toStdString()!=originalvalue)
    f_dirty=1;
  else
    f_dirty=0;
  DBQScoreBox::updateAppearance();
  if (f_editable && f_set) {
    ee->setReadOnly(0);
    buttonarea->show();
  }
  else {
    ee->setReadOnly(1);
    buttonarea->hide();
  }
  if (f_set) {
    //valueline->hide();
    // buttonarea->show();
    ee->show();
  }
  else {
    //valueline->show();
    // buttonarea->hide();
    ee->hide();
  }
}

void
DBQTextEdit::revertclicked()
{
  f_set=f_originallyset;
  if (f_originallyset)
    ee->setHtml(originalvalue.c_str());
  updateAppearance();
  if (f_set)
    ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::deleteclicked()
{
  f_set=!f_set;
  updateAppearance();
  if (f_set)
    ee->setFocus(Qt::OtherFocusReason);
}

void
DBQTextEdit::textedited()
{
  f_dirty=1;
  updateAppearance();
}

void
DBQTextEdit::getValue(DBscorevalue &val)
{
  val.v_string=(string)ee->toHtml().toStdString();
  val.scorename=scorename;
}
