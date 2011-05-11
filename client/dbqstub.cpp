
#include "dbqstub.moc.h"

using namespace std;

DBQStub::DBQStub(QWidget *parent)
  : DBQScoreBox(parent)
{
  f_set=1;
  setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);
  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(1);
  layout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
  this->setLayout(layout);

  button_new=new QPushButton("new",this);
  layout->addWidget(button_new);
  button_new->hide();

  QFrame *frame=new QFrame();
  frame->setFrameStyle(QFrame::StyledPanel);
  // frame->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);
  layout->addWidget(frame,1000);
  childlayout=new QFormLayout;
  childlayout->setMargin(0);
  childlayout->setSpacing(0); 
  childlayout->setAlignment(Qt::AlignTop);
  // childlayout->setColumnStretch(0,0);
  // childlayout->setColumnStretch(1,1000);
  frame->setLayout(childlayout);
  
  QObject::connect(button_new,SIGNAL(clicked()),this,SLOT(handle_newclicked()));
}

void
DBQStub::setValue(const DBscorevalue &val)
{
  if ((string)"yes"==val.v_string)
    f_set=1;
  else
    f_set=0;
}

void
DBQStub::getValue(DBscorevalue &val)
{
  val.scorename=scorename;
}

void
DBQStub::handle_newclicked()
{
  emit newclicked(this);
}

void
DBQStub::revertclicked()
{
  // this is a null instantiation of the pure virtual method in
  // DBQScoreBox.  we don't need to do anything visible to revert
  // stubs.
}

void
DBQStub::setEditable(bool e)
{
  if (e) {
    button_new->hide();
  }
  else {
    // button_new->show();
  }
}

void
DBQStub::addChild(DBQScoreBox *)
{
  //  childlayout->addWidget(sb);
  //  children.push_back(sb);
}
