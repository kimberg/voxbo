
using namespace std;

VBHelp::VBHelp(QWidget *parent=0,const char *name=0)
  : QWidget(parent,name)
{
  browser=new QTextBrowser(helpbox);
  browser->setMimeSourceFactory(qms);
  browser->setSource("voxbo.html");
  browser->setMinimumSize(300,200);

  // set up a cheap row of buttons on the bottom
  QHBox *mainbuttons;
  QPushButton *button;
  mainbuttons=new QHBox(this);
  mainbuttons->setSpacing(20);
  mainbuttons->setMargin(10);
  topleftlayout->addWidget(mainbuttons);

  button=new QPushButton("Load HTML",mainbuttons,"load");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(LoadHTML()));

  button=new QPushButton("Quit",mainbuttons,"quit");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(Quit()));
}



int
VBHelp::NewSearch()
{
  // find the right stuff for the new search term
}
