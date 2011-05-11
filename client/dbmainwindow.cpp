
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <iostream>

#include "dbclient.h"
#include "dbview.h"
#include "searchPatient.h"
#include "dbdialogs.h"
#include "dbmainwindow.moc.h"

// internal enums for event handling
enum {AL_NONE,AL_IMPORTANT,AL_MESSAGES,AL_LISTS,AL_USERINFO,AL_DBINFO,
      AL_FORMS,AL_RESULTS,AL_CONTACTS};

const string headerstyle1="color:red;";

DBmainwindow::DBmainwindow()
{
  setGeometry(150,150,800,600);
  myframe=new QFrame;
  setCentralWidget(myframe);
  a_open=new QAction("&Open local database",this);
  a_connect=new QAction("&Connect to remote database",this);
  a_close=new QAction("&Close/Disconnect",this);
  a_create=new QAction("Create &new database",this);
  a_search=new QAction("&Search database",this);
  a_test=new QAction("tes&t",this);

  a_newuser=new QAction("new user",this);
  a_newpatient=new QAction("new patient",this);
  a_online=new QAction("take database offline",this);

  QObject::connect(a_open,SIGNAL(triggered()),this,SLOT(open()));
  QObject::connect(a_connect,SIGNAL(triggered()),this,SLOT(connect()));
  QObject::connect(a_create,SIGNAL(triggered()),this,SLOT(create()));
  QObject::connect(a_search,SIGNAL(triggered()),this,SLOT(search()));
  QObject::connect(a_test,SIGNAL(triggered()),this,SLOT(test()));
  QObject::connect(a_close,SIGNAL(triggered()),this,SLOT(close()));

  QObject::connect(a_newuser,SIGNAL(triggered()),this,SLOT(newuser()));
  QObject::connect(a_newpatient,SIGNAL(triggered()),this,SLOT(newpatient()));
  QObject::connect(a_online,SIGNAL(triggered()),this,SLOT(online()));

  client=NULL;
  w_menubar=menuBar();
  setMenuBar(w_menubar);

  filemenu=w_menubar->addMenu("File");
  filemenu->addAction(a_open);
  filemenu->addAction(a_connect);
  filemenu->addAction(a_create);
  filemenu->addAction(a_close);
  secondmenu=w_menubar->addMenu("More Stuff");
  secondmenu->addAction(a_search);
  secondmenu->addAction(a_test);

  adminmenu=w_menubar->addMenu("Admin");
  adminmenu->addAction(a_newuser);
  adminmenu->addAction(a_newpatient);
  adminmenu->addAction(a_online);

  //w_statusbar=statusBar();
  //w_statusbar->showMessage("hahahaha");
  w_menubar->show();
  filemenu->show();
  currentpanel=AL_NONE;
  setup();
  updatevisiblewidgets();
  arrangeChildren();
}


void
DBmainwindow::resizeEvent(QResizeEvent *re)
{
  QMainWindow::resizeEvent(re);
  if (client) {
    arrangeChildren();
  }
}

void
DBmainwindow::open()
{
  DBlocallogin ll(this);
  // FIXME bad defaults!
  ll.setusername("admin");
  ll.setpassword("test");
  ll.setdirname("test");
  while (1) {
    int ret=ll.exec();
    if (ret!=QDialog::Accepted)
      return;
    client=new localClient(ll.username(),ll.password(),ll.dirname());
    if (!(client->login()))
      break;
    delete client;
    client=NULL;
    ll.setmessage("nice try!");  // FIXME should probably say something else
  }
  populate();
  w_arealist->setCurrentItem(al_aboutitem,QItemSelectionModel::Select);
  currentpanel=AL_DBINFO;
  updatevisiblewidgets();
  arrangeChildren();
}

void
DBmainwindow::connect()
{
  DBremotelogin ll(this);
  // FIXME bad defaults!
  ll.setusername("kimberg");
  ll.setpassword("kimberg");
  ll.setservername("rage.uphs.upenn.edu");
  ll.setserverport(6010);
  int ret=ll.exec();
  if (ret!=QDialog::Accepted)
    return;
  client=new remoteClient(ll.username(),ll.password(),ll.servername(),ll.serverport());
  if (client->login()) {
    delete client;
    client=NULL;
    // FIXME should probably say something here
    return;
  }
  
  populate();
  w_arealist->setCurrentItem(al_aboutitem,QItemSelectionModel::Select);
  currentpanel=AL_DBINFO;
  updatevisiblewidgets();
  arrangeChildren();
}

void
DBmainwindow::create()
{
  cout << "create not yet implemented, but let's open a window" << endl;
  DBmainwindow *tmpw=new DBmainwindow;
  tmpw->show();
}

// FIXME obsolete this?  or use it as an alternative to the quick
// search?

void
DBmainwindow::search()
{
  // if (!client) return;
  // SearchPatient *sp_form = new SearchPatient(client);
  // sp_form->show();
}


void
DBmainwindow::test()
{
  DBnewsession t1(this);
  t1.exec();
  DBpatient pp;
  DBpicksession t2(this,&pp);
  t2.exec();
  if (client) {
    DBpicktest t3(this,client);
    t3.exec();
  }
}

void
DBmainwindow::close()
{
  if (!client) return;
  client->exit();
  client=NULL;
  arrangeChildren();
  updatevisiblewidgets();
}

void
DBmainwindow::newuser()
{
  if (!client) return;
  DBuserinfo uu(this);
  uu.setusername("<new username here>");
  if (uu.exec()==QDialog::Rejected)
    return;
  userGUI ug;
  ug.setAccount(uu.username());
  ug.setPasswd(uu.password());
  ug.setName(uu.name());
  ug.setPhone(uu.phone());
  ug.setEmail(uu.email());
  ug.setAddress(uu.address());
  int err;
  if ((err=client->putNewUser(ug))==0)
    QMessageBox::critical(0, "Yes!", "Your new user has been created.");
  else {
    QMessageBox::critical(0, "Error", "Your new user has not been created.");
    cout << err << endl;
  }
}

void
DBmainwindow::newpatient()
{
  DBpatient pp;
  pp.patientID=-1;
  DBview *dd=new DBview(client,pp);
  dd->layout_view("newpatient");
  dd->show();
}

void
DBmainwindow::online()
{
  cout << "FIXME coming soon" << endl;
}

void
DBmainwindow::handleAreaList()
{
  QList<QListWidgetItem *> selections=w_arealist->selectedItems();
  if (selections.size()!=1) return;
  QListWidgetItem *it=selections[0];
  if (it->type()==currentpanel) {
    // FIXME refresh certain panels?
    return;
  }
  // otherwise hide, then show
  w_messages->hide();
  w_lists->hide();
  w_userinfo->hide();
  w_dbinfo->hide();
  w_resultsbox->hide();
  currentpanel=it->type();

  switch (currentpanel) {
  case AL_NONE:
    return;
  case AL_MESSAGES:
    w_messages->show();
    return;
  case AL_LISTS:
    w_lists->show();
    return;
  case AL_USERINFO:
    w_userinfo->show();
    return;
  case AL_DBINFO:
    w_dbinfo->show();
    return;
  case AL_RESULTS:
    w_resultsbox->show();
    return;
  }
}

void
DBmainwindow::handleSearch()
{
  if (s_searchline->text().isEmpty()) {
    // FIXME
    QMessageBox::critical(0, "Error", "You didn't enter anything to search for");
    return;
  }
  // if (searchPart->isChecked() && !currentPatients.size()) {
  //   QMessageBox::critical(0, "Error", "Search aborted: no patients found yet.");
  //   return;
  // }
  //if (searchPart->isChecked())
  //tags_out.patientIDs = currentPatients;
  
  // put search info into proper form for server
  patientSearchTags s_tags;
  // FIXME s_tags should be the right scorename when needed
  // int field_index = s_field->currentIndex();
  s_tags.scoreName="";
  s_tags.relationship = s_relation->currentText().toStdString();
  s_tags.searchStr = s_searchline->text().toStdString();
  s_tags.case_sensitive = false;

  // if (insensButt->isChecked())
  //   tags_out.case_sensitive = false; 
  // else
  //   tags_out.case_sensitive = true;


  
  // execute the search
  vector<patientMatch> pmList;
  client->reqSearchPatient(s_tags,pmList);  
  // FIXME right now reqSearchPatient() returns nonzero both on error
  // and if the search doesn't turn anything up.  the latter should
  // really return 0 with an empty vector of patientMatch

  // vector<int32> results;
  w_searchresults->clear();
  for (size_t i=0; i<pmList.size(); i++) {
    QListWidgetItem *tmp=new QListWidgetItem(strnum(pmList[i].patientID).c_str());
    tmp->setData(Qt::UserRole,QVariant(pmList[i].patientID));
    w_searchresults->addItem(tmp);
  }
  
  w_arealist->clearSelection();
  w_arealist->setCurrentItem(al_resultsitem,QItemSelectionModel::Select);
}

void
DBmainwindow::handleSaveResults()
{
  cout << "FIXME: nothing doing" << endl;
}

void
DBmainwindow::handleSearchResultClicked(QListWidgetItem *it)
{
  uint32 patientid=it->data(Qt::UserRole).toInt();
  cout << patientid << endl;
  DBview *pview = new DBview(client,patientid);
  if (pview && *pview) {
    pview->layout_view("patient");
    pview->show();
  }
  else {
    // FIXME shouldn't happen warning
    cout << "shouldnt happen" << endl;
    cout << pview->patient.patientID << endl;
    delete pview;
  }
}

// setup() creates all the widgets and calls arrangeChildren() to set
// their geometries

void
DBmainwindow::setup()
{
  // LOGO
  w_logo=new QLabel(myframe);
  w_logo->setPixmap(QPixmap(":/icons/a_logo.png"));
  w_logo->show();
  // DB BANNER
  w_banner=new QLabel(myframe);
  w_banner->setText("Welcome to the database");
  w_banner->hide();
  // MOTD
  w_motd=new QLabel(myframe);
  w_motd->setText("Watch out for falling pianos!");
  w_motd->hide();
  // AREAS
  w_arealist=new QListWidget(myframe);
  al_aboutitem=new QListWidgetItem("About this database",w_arealist,AL_DBINFO);
  al_resultsitem=new QListWidgetItem("Your Search Results",w_arealist,AL_RESULTS);
  w_arealist->addItem(al_aboutitem);
  w_arealist->addItem(new QListWidgetItem("Your Messages",w_arealist,AL_MESSAGES));
  w_arealist->addItem(new QListWidgetItem("Your Patient Lists",w_arealist,AL_LISTS));
  w_arealist->addItem(new QListWidgetItem("Your Account",w_arealist,AL_USERINFO));
  w_arealist->addItem(new QListWidgetItem("Your Forms",w_arealist,AL_FORMS));
  w_arealist->addItem(al_resultsitem);
  w_arealist->addItem(new QListWidgetItem("Your Contact Records",w_arealist,AL_CONTACTS));
  w_arealist->addItem(new QListWidgetItem("Other Stuff",w_arealist,AL_IMPORTANT));
  QObject::connect(w_arealist,SIGNAL(itemSelectionChanged()),this,SLOT(handleAreaList()));
  w_arealist->setSelectionMode(QAbstractItemView::SingleSelection);
  w_arealist->hide();
  // LISTS, USERINFO, DBINFO, SEARCHRESULTS
  w_lists=makeplistbox();
  w_dbinfo=makedbinfobox();
  w_userinfo=makeuserinfobox();
  w_resultsbox=makeresultsbox();
  w_lists->hide();
  w_dbinfo->hide();
  w_userinfo->hide();
  w_resultsbox->hide();
  // MESSAGES
  w_messages=new QFrame(myframe);
  w_messages->setFrameStyle(QFrame::Box|QFrame::Plain);
  w_messages->hide();
  // SEARCHBOX
  w_searchbox=makesearchbox();
  w_searchbox->hide();
}

// populate interface using stuff in the client record

void
DBmainwindow::populate()
{
  // search results should be cleared
  w_searchresults->clear();
  // searchable fields in search box, plus other args
  searchScoreIDs.clear();
  map<string, DBscorename>::const_iterator iter;
  for (iter = client->dbs.scorenames.begin(); iter != client->dbs.scorenames.end(); ++iter) {
    if ((iter->second).flags.count("searchable")) {
      string tmpStr = (iter->second).screen_name; 
      s_field->addItem(QString::fromStdString(tmpStr));
      searchScoreIDs.push_back((iter->second).name);
    }
  }
  s_field->setCurrentIndex(0);
  s_relation->setCurrentIndex(0);
  s_searchline->clear();
  // patient lists
  w_plisttree->clear();
  if (client->pList.size()) {
    QList<QTreeWidgetItem *>items;
    QStringList sl;
    vbforeach(DBpatientlist &pl,client->pList) {
      sl << pl.name.c_str() << pl.search_strategy.c_str() << pl.notes.c_str();
      items.append(new QTreeWidgetItem((QTreeWidget *)0,sl));
    }
    w_plisttree->insertTopLevelItems(0,items);
  }
  // FIXME populate the forms
  // when we logged on, we should have requested patient(userid)
  // find all the scorenames that are children of the node "forms"
  //   and all the instances of those children
  // each such child should have certain fields, including a brief
  //   description, requester, due date
  // e.g., forms:posttest:duedate
  // clicking a form opens it up in a special dbview
  // that special dbview has a button for submitting the form, that
  //   makes it read-only

  // FIXME populate most recent contacts
  // button to load up all contacts (in groups of 100 at least)
}

void
DBmainwindow::updatevisiblewidgets()
{
  w_logo->show();
  bool f_client;
  if (client) f_client=1;  else f_client=0;
  w_banner->setVisible(f_client);
  w_motd->setVisible(f_client);
  w_arealist->setVisible(f_client);
  w_dbinfo->setVisible(f_client);
  w_searchbox->setVisible(f_client);

  w_messages->setVisible(0);
  w_lists->setVisible(0);
  w_userinfo->setVisible(0);
  w_resultsbox->setVisible(0);
}

QHBox *
DBmainwindow::makesearchbox()
{
  QHBox *hb=new QHBox(myframe);

  QPushButton *tmpb=new QPushButton("Search");
  hb->addWidget(tmpb);
  QObject::connect(tmpb,SIGNAL(clicked()),this,SLOT(handleSearch()));

  s_field = new QComboBox;
  s_field->addItem("Any Field");

  hb->addWidget(s_field);

  s_relation = new QComboBox;
  s_relation->addItem("include");
  s_relation->addItem("equal");
  s_relation->addItem("wildcard");
  hb->addWidget(s_relation);
  
  s_searchline=new QLineEdit;
  hb->addWidget(s_searchline);
  return hb;
}

QVBox *
DBmainwindow::makeplistbox()
{
  // treewidget with searches
  w_lists=new QVBox(myframe);
  QLabel *tmps=new QLabel("Your Patient Lists");
  tmps->setStyleSheet(headerstyle1.c_str());
  w_lists->addWidget(tmps);
  w_plisttree=new QTreeWidget;
  w_lists->addWidget(w_plisttree);
  w_plisttree->setColumnCount(3);
  QStringList tmpl;
  tmpl<<"Name"<<"Strategy"<<"Num";
  w_plisttree->setHeaderLabels(tmpl);
  w_plisttree->setRootIsDecorated(0);
  w_plisttree->setAllColumnsShowFocus(1);

  // button area at the bottom
  QHBox *hb=new QHBox;
  w_lists->addWidget(hb);
  QPushButton *button;
  button=new QPushButton("update");
  hb->addWidget(button);
  button=new QPushButton("open");
  hb->addWidget(button);
  return w_lists;
}

QVBox *
DBmainwindow::makeresultsbox()
{
  // listwidget with search results
  w_resultsbox=new QVBox(myframe);
  QLabel *tmps=new QLabel("Your Search Results");
  tmps->setStyleSheet(headerstyle1.c_str());
  w_resultsbox->addWidget(tmps);
  w_searchresults=new QListWidget;
  QObject::connect(w_searchresults,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,
                   SLOT(handleSearchResultClicked(QListWidgetItem *)));
  w_resultsbox->addWidget(w_searchresults);

  // button area at the bottom
  QHBox *hb=new QHBox;
  w_resultsbox->addWidget(hb);
  QPushButton *button;
  button=new QPushButton("save as list");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleSaveResults()));
  hb->addWidget(button);
  return w_resultsbox;
}

QVBox *
DBmainwindow::makeuserinfobox()
{
  // treewidget with searches
  w_userinfo=new QVBox(myframe);
  QLabel *tmps=new QLabel("Your User Info");
  tmps->setStyleSheet(headerstyle1.c_str());
  w_userinfo->addWidget(tmps);
  // FIXME user info here

  // button area at the bottom
  QHBox *hb=new QHBox;
  w_userinfo->addWidget(hb);
  QPushButton *button;
  button=new QPushButton("update");
  hb->addWidget(button);
  button=new QPushButton("revert");
  hb->addWidget(button);
  return w_userinfo;
}

QVBox *
DBmainwindow::makedbinfobox()
{
  // treewidget with searches
  w_dbinfo=new QVBox(myframe);
  QLabel *tmps=new QLabel("Database Info");
  tmps->setStyleSheet(headerstyle1.c_str());
  w_dbinfo->addWidget(tmps);
  // FIXME db info here
  QFrame *foo=new QFrame();
  w_dbinfo->addWidget(foo);
  foo->setFixedHeight(120);
  foo->setFrameStyle(QFrame::Box|QFrame::Sunken);
  return w_dbinfo;
}

void
DBmainwindow::arrangeChildren()
{
  int pos=0;
  const int SPACING=20;

  // LOGO AT TOP LEFT
  w_logo->adjustSize();
  w_logo->setGeometry(10,pos+10,w_logo->width(),w_logo->height());
  w_logo->show();

  // BANNER AT TOP RIGHT
  w_banner->adjustSize();
  w_banner->setGeometry(myframe->width()-w_banner->width()-10,pos+10,w_banner->width(),w_banner->height());

  pos=w_logo->geometry().bottom();
  if (w_banner->geometry().bottom()>pos) pos=w_logo->geometry().bottom();
  
  // MOTD IF NEEDED
  if (v_motd.size()) {
    w_motd->setFixedWidth(myframe->width()*2/3);
    w_motd->adjustSize();
    w_motd->setGeometry(myframe->width()/6,pos+SPACING,w_motd->width(),w_motd->height());
    pos=w_motd->geometry().bottom();
    w_motd->show();
  }
  else
    w_motd->hide();

  pos+=SPACING;

  int wtmp=(myframe->width()-30)/3;
  int htmp=myframe->height()-pos-SPACING-SPACING-40;   // 40 is for search stuff

  // AREALIST BOTTOM LEFT
  w_arealist->setGeometry(10,pos,wtmp,htmp);
  
  // LISTS AND MESSAGES BOTTOM RIGHT
  w_lists->setGeometry(myframe->width()-10-wtmp*2,pos,wtmp*2,htmp);
  w_messages->setGeometry(myframe->width()-10-wtmp*2,pos,wtmp*2,htmp);
  w_userinfo->setGeometry(myframe->width()-10-wtmp*2,pos,wtmp*2,htmp);
  w_dbinfo->setGeometry(myframe->width()-10-wtmp*2,pos,wtmp*2,htmp);
  w_resultsbox->setGeometry(myframe->width()-10-wtmp*2,pos,wtmp*2,htmp);

  pos+=htmp+SPACING;

  // SEARCH HBOX ON THE VERY BOTTOM
  w_searchbox->setGeometry(10,pos+SPACING,myframe->width()-SPACING-SPACING,40);
}
