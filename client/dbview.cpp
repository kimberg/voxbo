
// dbview.cpp
// 
// Copyright (c) 2008-2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 3, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file named COPYING.  If not, write
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA
//
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg

#include <QApplication>
#include <QMainWindow>
#include <QScrollArea>
#include <QLayout>
#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QTableWidget>
#include <QValidator>
#include <QHeaderView>

#include <iostream>

#include "mydefs.h"
#include "tokenlist.h"
#include "vbutil.h"
#include "dbqscorebox.h"
#include "dbqcombo.h"
#include "dbqlineedit.h"
#include "dbqtextedit.h"
#include "dbqtimedate.h"
#include "dbqimage.h"
#include "dbqstub.h"
#include "dbqbrain.h"
#include "dbdialogs.h"
#include "dbview.moc.h"

#include "boost/format.hpp"

using namespace std;
using boost::format;

DBview::DBview(DBclient *c,int32 patientid)
{
  // FIXME hourglass while requesting patient?
  int err=c->reqOnePatient(patientid,patient);
  if (err)
    return;  // app's responsibility to test this dbview
  client=c;
  init();
}

// the below constructor is mostly for testing
DBview::DBview(DBclient *c,DBpatient &p)
{
  patient=p;
  client=c;
  init();
}

void
DBview::init()
{
  nextid=-2;             // fake id for values created in the interface

  // styles
  //style_main+="*{background-color:white;}";
  style_main+="QPushButton {margin:0;padding:1;border:1px solid black;background-color:green;}";
  style_main+="QPushButton:pressed {background-color:red;}";
  style_main+="QFrame {margin:1;padding:0;margin-left:0;xbackground-color:white;}";
  style_scorelabel+="QLabel {color:darkblue;font-weight:bold;text-align:right;margin:2;}";
  style_addscore+="DBnewbutton {color:darkred;border:0px;border-radius:22px;margin:0;padding:0;text-align:left;}";

  style_sessionbanner="padding:2px;margin:2px;border:1px solid black;"
    "color:darkgreen;background-color:lightgray;";

  // main tab widget
  resize(600,1200);
  tabwidget=new QTabWidget;
  tabwidget->setTabsClosable(1);
  QObject::connect(tabwidget,SIGNAL(tabCloseRequested(int)),this,SLOT(handleTabClose(int)));

  setCentralWidget(tabwidget);

  // toolbar
  toolbar=new QToolBar;
  addToolBar(toolbar);
  QComboBox *cb=new QComboBox;
  cb->addItem("default view");
  cb->addItem("view all");
  cb->addItem("view changed");   // FIXME only display if some fields will be editable
  cb->addItem("hide all");       // FIXME patently bizarre entry for testing only!
  toolbar->addWidget(cb);
  QObject::connect(cb,SIGNAL(currentIndexChanged(int)),this,SLOT(handleViewSelect(int)));

  // buttons
  QPushButton *button;
  button=new QPushButton("Submit");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleSubmit()));

  button=new QPushButton("Revert All");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleRevertAll()));
  
  button=new QPushButton("New Session");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleNewSession()));
  
  button=new QPushButton("New Test");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleNewTest()));
  
  button=new QPushButton("Edit");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleEdit()));
  
  button=new QPushButton("Dump (DEBUG)");
  toolbar->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(handleDump()));
  
  // status bar
  statusbar=new QStatusBar;
  setStatusBar(statusbar);
  statusbar->setSizeGripEnabled(0);
  statusbar->showMessage("Welcome!");

  show();
}

void
DBview::handleSubmit()
{
  // FIXME should we ask for confirmation?  maybe not if we allow reverting
  // FIXME!
  int32 thisid,oldid;
  // set<int32> ids;
  map<int32,int32> newids;
  int32 f_newpatient=0;

  set<int32> newsessions;
  set<int32> newscorevalues;
  set<int32> dirtyscorevalues;
  set<int32> updatescorevalues;

  // make a list of all the new ids we need: patientid, sessionids,
  // and scorevalueids, in that order.

  if (patient.patientID<0)
    f_newpatient=patient.patientID;
  pair<int32,DBsession> spair;
  vbforeach (spair,patient.sessions) {
    if (spair.second.id<0)
      newsessions.insert(spair.second.id);
  }
  for (size_t i=0; i<scoreboxlist.size(); i++) {
    thisid=scoreboxlist[i]->scorevalueid;
    if (scoreboxlist[i]->f_dirty) {
      DBscorevalue sv;
      scoreboxlist[i]->getValue(sv);
      sv.id=thisid;
      patient.scores[thisid]=sv;
      if (thisid<0) {
        newscorevalues.insert(thisid);
        updatescorevalues.insert(thisid);
      }
      else {
        dirtyscorevalues.insert(thisid);
        updatescorevalues.insert(thisid);
      }
    }
  }
  // request all the new ids we need
  int newidcnt=newsessions.size()+newscorevalues.size()+(f_newpatient ? 1 : 0);
  int32 firstid=client->reqID(newidcnt);
  if (firstid<1) {
    // FIXME put up an error message
    QMessageBox::information(this,"id problem","id problem");
    return;
  }
  // remap patient id
  if (patient.patientID<0) {
    newids[patient.patientID]=firstid;
    patient.patientID=firstid;
    firstid++;
  }

  // renumber the sessions in the dbpatient.sessions map
  vbforeach(int32 ns,newsessions) {
    DBsession ss=patient.sessions[ns];
    patient.sessions.erase(ns);
    oldid=ss.id;
    ss.id=firstid++;
    newids[oldid]=ss.id;
    patient.sessions[ss.id]=ss;
  }
  // renumber the scorevalues in the dbpatient.scores map
  vbforeach(int32 ns,newscorevalues) {
    DBscorevalue sv=patient.scores[ns];
    patient.scores.erase(ns);
    oldid=sv.id;
    sv.id=firstid++;
    sv.patient=patient.patientID;
    newids[oldid]=sv.id;
    patient.scores[sv.id]=sv;
    cout << format("NEW VALUE: old %d new %d string value %s\n") % oldid % sv.id % sv.v_string;
  }
  // remap the ids is updatescorevalues
  set<int32> newusv;
  vbforeach(int32 id,updatescorevalues)
    newusv.insert(newids[id]);
  updatescorevalues=newusv;

  // spew out some info about fields to be updated
  vbforeach(int32 ns,updatescorevalues) {
    DBscorevalue sv=patient.scores[ns];
    cout << format("UPDATE VALUE %d: string value %s (for %d)\n") % sv.id % sv.v_string % sv.patient;
  }
  // for (set<int32>::iterator si=ids.begin(); si!=ids.end(); si++) {
  //   newvalues.insert(firstid);
  //   patient.scores[*si].id=firstid++;
  // }
  int ret;
  // if it's a new patient, we have a function to push it all at once
  if (f_newpatient) {
    patient.print();
    ret=client->putNewPatient(patient);
    if (ret)
      QMessageBox::information(this,"error committing new patient","error committing new patient");
    else
      QMessageBox::information(this,"new patient committed","new patient committed");
  }
  // otherwise push the new sessions and updated scores
  else {
    vector<DBsession> stmp;
    vbforeach (int32 s,newsessions)
      stmp.push_back(patient.sessions[s]);
    if (stmp.size())
      ret=client->putNewSession(stmp);
    ret=client->putScoreValues(updatescorevalues,patient.scores);
    cout << "putscorevalues ret " << ret << " " << client->errMsg << endl;
  }
  // FIXME handle bad return codes!
  // FIXME also handle good return codes, committed fields are no longer dirty
}


void
DBview::handleRevertAll()
{
  // FIXME
  DBnewsession ppp(this);
  ppp.exec();
  DBpicksession qqq(this,&patient);
  qqq.exec();
  DBpicktest rrr(this,client);
  rrr.exec();
  DBlocallogin lll(this);
  lll.exec();





  // FIXME if there are no changes, just return
  // FIXME ask for verification if there are actual changes
  // make a list of all the negative ids
  for (size_t i=0; i<scoreboxlist.size(); i++) {
    DBQScoreBox *s=scoreboxlist[i];
    if (s->f_dirty || (s->f_set != s->f_originallyset)) {
      s->revertclicked();
    }
  }
}


void
DBview::handleNewSession()
{
  DBnewsession d(this);
  d.exec();
  cout << "FIXME not yet" << endl;
  // FIXME pop up confirmation
}

void
DBview::handleNewTest()
{
  // FIXME does this work well enough?
  DBpicktest d(this,client);
  d.exec();
  DBviewspec vs;
  vs.name="Your newly added test (FIXME)";
  vs.entries.push_back((string)"test "+d.selectedtest());
  client->dbs.viewspecs["dynamic-generated"]=vs;
  layout_view("dynamic-generated");
}

void
DBview::handleDump()
{
  // FIXME debug method for dumping stuff to cout
  format scoreformat("score %d, child of %d : %s (%s) : %s\n");
  pair<int32,DBscorevalue> score;
  vbforeach (score,patient.scores) {
    cout << scoreformat % score.second.id % score.second.parentid % score.second.scorename %
      score.second.datatype % score.second.printable();
  }
}

void
DBview::handleEdit()
{
  // FIXME switch to edit mode
  // FIXME first get write lock, remember to give it up when we commit or close
  for (size_t i=0; i<scoreboxlist.size(); i++)
    scoreboxlist[i]->setEditable(1);
}

void
DBview::handleTabClose(int index)
{
  // FIXME do we want to make sure nothing important is in that tab???
  tabwidget->removeTab(index);
}


void
DBview::handleViewSelect(int index)
{
  if (index==0) {
    for (size_t i=0; i<scoreboxlist.size(); i++) {
      if (scoreboxlist[i]->f_set) {
        scoreboxlist[i]->show();
        scoreboxlist[i]->label->show();
      }
      else {
        scoreboxlist[i]->hide();
        scoreboxlist[i]->label->hide();
      }
    }
  }
  else if (index==1) {
    for (size_t i=0; i<scoreboxlist.size(); i++) {
      scoreboxlist[i]->show();
      scoreboxlist[i]->label->show();
    }
  }
  else if (index==2) {
    for (size_t i=0; i<scoreboxlist.size(); i++) {
      if (scoreboxlist[i]->f_dirty) {
        scoreboxlist[i]->show();
        scoreboxlist[i]->label->show();
      }
      else if (scoreboxlist[i]->f_set != scoreboxlist[i]->f_originallyset) {
        scoreboxlist[i]->show();
        scoreboxlist[i]->label->show();
      }
      else {
        scoreboxlist[i]->hide();
        scoreboxlist[i]->label->hide();
      }
    }
  }
  else if (index==3) {
    for (size_t i=0; i<scoreboxlist.size(); i++) {
      scoreboxlist[i]->hide();
      scoreboxlist[i]->label->hide();
    }
  }
}

void
DBview::layout_view(string viewname)
{
  DBviewspec vs=client->dbs.viewspecs[viewname];

  QFrame *myframe;
  QVBoxLayout *layout=NULL;

  for (size_t i=0; i<vs.entries.size(); i++) {
    tokenlist toks;
    toks.ParseLine(vs.entries[i]);

    if (layout==NULL && toks[0]!="newtab") {
      myframe=new QFrame;
      tabwidget->addTab(myframe,vs.name.c_str());
      layout=new QVBoxLayout;
      layout->setSpacing(0);
      layout->setMargin(0);
      layout->setAlignment(Qt::AlignTop);
      myframe->setLayout(layout);
    }
    
    if (toks.size()==0) continue;
    if (toks[0][0]=='#') continue;
    if (toks[0][0]=='$') continue;
    if (toks[0][0]=='!') continue;
    if (toks[0][0]==';') continue;

    if (toks[0]=="newtab") {
      myframe=new QFrame;
      tabwidget->addTab(myframe,(toks.size()==1 ? "<view>" : toks.Tail(1).c_str()));
      layout=new QVBoxLayout;
      layout->setSpacing(0);
      layout->setMargin(0);
      layout->setAlignment(Qt::AlignTop);
      myframe->setLayout(layout);
    }
    else if (toks[0]=="banner")
      layout_banner(layout,toks);
    else if (toks[0]=="test")
      layout_test(layout,toks);
    else if (toks[0]=="sessionlist")
      layout_sessionlist(layout);
    else if (toks[0]=="testlist")
      layout_testlist(layout);
    else if (toks[0]=="session")
      layout_session(layout,strtol(toks[1]));
    else {
      cout << format("unrecognized view directive: %s\n") % toks[0];  // FIXME
    }
  }
}


void
DBview::layout_banner(QVBoxLayout *layout,tokenlist &toks)
{
  QFrame *fm=new QFrame;
  fm->setFrameStyle(QFrame::Panel||QFrame::Raised);
  layout->addWidget(fm);
  QHBoxLayout *hb=new QHBoxLayout;
  QLabel *lab;
  fm->setLayout(hb);
  // FIXME set spacing etc.
  for (size_t i=1; i<toks.size(); i++) {
    if (toks[i]=="todaysdate") {
      DBdate dd;
      lab=new QLabel(dd.getDateStr());
      lab->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
      hb->addWidget(lab);
    }
    else if (toks[i]=="username") {
      lab=new QLabel("your username here");
      lab->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
      hb->addWidget(lab);
    }
    else if (toks[i]=="string" && i<toks.size()-1) {
      lab=new QLabel(toks(i+1));
      lab->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
      hb->addWidget(lab);
    }
    else if (toks[i]=="space") {
      hb->addSpacing(25);
      hb->addStretch(100);
    }
    else if (toks[i]=="score" && i<toks.size()-1) {
      vector<int32> children=getchildren(patient.names,toks[i+1]);
      // FIXME just use last one
      if (children.size()) {
        // FIXME the following line creates an entry in patent->scores if it doesn't exist!
        DBscorevalue sv=patient.scores[children[children.size()-1]];
        lab=new QLabel(sv.v_string.c_str());
        lab->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
        hb->addWidget(lab);
      }
      i++;
    }
    else {
    }
  }
  // FIXME logo
  // FIXME patient rec #
  // FIXME update button
  // FIXME date/time
  // FIXME show/hide missing scores
}

void
DBview::layout_test(QVBoxLayout *layout,tokenlist &toks)
{
  QScrollArea *myscrollarea=new QScrollArea;
  QFrame *myframe=new QFrame;
  QFormLayout *mylayout=new QFormLayout;

  layout->addWidget(myscrollarea);
  myframe->setLayout(mylayout);
  myscrollarea->setWidget(myframe);
  myscrollarea->setWidgetResizable(1);
  mylayout->setSpacing(0);
  mylayout->setMargin(0);
  mylayout->setAlignment(Qt::AlignTop);
  //mylayout->setColumnStretch(0,0);
  //mylayout->setColumnStretch(1,10);
  
  // myframe->setAutoFillBackground(1);
  // mylayout->addWidget(new QLabel("<b>name</b>"),0,0);
  // mylayout->addWidget(new QLabel("<b>value</b>"),0,1);

  // parse flags
  bool f_mostrecent=0;
  bool f_sessioninfo=0;
  for (size_t i=2; i<toks.size(); i++) {
    if (toks[i]=="mostrecent")
      f_mostrecent=1;
    if (toks[i]=="sessioninfo")
      f_sessioninfo=1;
  }

  string scorename=toks[1];

  vector<int32> instances=getchildren(patient.names,scorename);

  // if the mostrecent flag is set, only show matching scores with the
  // most recent session date

  // FIXME if the field is repeating, we need to group all values with the same sid together

  if (f_mostrecent) {
    vector<int32> mostrecentids;
    int32 mostrecentid=instances[0];
    mostrecentids.push_back(mostrecentid);
    int32 tmpsid=patient.scores[mostrecentid].sessionid;
    DBdate mostrecentdate=patient.sessions[tmpsid].date;
    for (size_t i=1; i<instances.size(); i++) {
      int32 sid=patient.scores[instances[i]].sessionid;
      DBdate thisdate=patient.sessions[sid].date;
      if (thisdate>mostrecentdate) {
        mostrecentids.clear();
        mostrecentids.push_back(instances[i]);
        mostrecentdate=thisdate;
      }
      if (thisdate==mostrecentdate) {
        mostrecentids.push_back(instances[i]);
      }
    }
  }
  for (size_t i=0; i<instances.size(); i++) {
    int32 tmpsid=patient.scores[instances[i]].sessionid;
    DBsession *ss=&(patient.sessions[tmpsid]);
    if (f_sessioninfo) {
      QLabel *lab=new QLabel;
      string stext=(string)"Session <a href=\"sessionid:"+strnum(tmpsid)+"\">"+strnum(tmpsid)+"</a> ("+
        ss->date.getDateStr()+"), examined by "+ss->examiner;
      lab->setText(stext.c_str());
      lab->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);
      lab->setStyleSheet(style_sessionbanner.c_str());
      // mylayout->addWidget(lab,mylayout->rowCount(),0,1,2,Qt::AlignLeft);
      mylayout->addRow(lab);
      QObject::connect(lab,SIGNAL(linkActivated(QString)),this,SLOT(handleSessionLink(QString)));
    }
    layout_node(mylayout,scorename,instances[i],NULL);
  }
  // even if there are no instances, we still lay out some nodes
  if (instances.size()==0) {
    layout_node(mylayout,scorename,-1,NULL);
  }
}

void
DBview::layout_testlist(QVBoxLayout *layout)
{
  // all root tests, regardless of session, maybe alphabetically?  chronologically?
  vector<int32> testids=getchildren(patient.children,0);
  if (testids.size()==0) {
    // FIXME put something informative up in a qlabel maybe
    return;
  }
  QTableWidget *tw=new QTableWidget;
  tw->setRowCount(testids.size());
  tw->setColumnCount(3);
  QStringList headers;
  headers << "testname" << "date" << "info";
  tw->setHorizontalHeaderLabels(headers);
  tw->verticalHeader()->hide();
  tw->setAlternatingRowColors(1);
  tw->setShowGrid(0);
  tw->setFocusPolicy(Qt::NoFocus);
  tw->setSelectionMode(QAbstractItemView::SingleSelection);
  tw->setSelectionBehavior(QAbstractItemView::SelectRows);
  tw->setSortingEnabled(1);
  QObject::connect(tw,SIGNAL(itemDoubleClicked(QTableWidgetItem *)),this,SLOT(handleitemclick(QTableWidgetItem *)));

  // if (client->dbs.scorenames.count(nameid)==0)

  int thisrow=0;
  vbforeach(int32 i,testids) {
    string tname=patient.scores[i].scorename;
    // cout << *i << " " << client->dbs.scorenames[i].name << endl;
    QTableWidgetItem *it1=new QTableWidgetItem(tname.c_str());
    it1->setData(role_link,QVariant(QString("test ")+QString(strnum(i).c_str())));

    QTableWidgetItem *it2=new QTableWidgetItem("foobar");
    QTableWidgetItem *it3=new QTableWidgetItem("more info here!");
    it1->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    it2->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    it3->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);   
    tw->setItem(thisrow,0,it1);
    tw->setItem(thisrow,1,it2);
    tw->setItem(thisrow,2,it3);
    thisrow++;
  }
  tw->resizeColumnsToContents();
  layout->addWidget(tw,0,Qt::AlignTop);

}

void
DBview::layout_session(QVBoxLayout *layout,int32 sessionid)
{
  // FIXME details at top
  // use patient.sessiontests to find all tests for this session
  vector<int> testids=getchildren(patient.sessiontests,sessionid);
  if (testids.size()==0) {
    // FIXME put something informative up in a qlabel maybe
    return;
  }

  QScrollArea *myscrollarea=new QScrollArea;
  QFrame *myframe=new QFrame;
  QFormLayout *mylayout=new QFormLayout;

  layout->addWidget(myscrollarea);
  myframe->setLayout(mylayout);
  myscrollarea->setWidget(myframe);
  myscrollarea->setWidgetResizable(1);
  mylayout->setSpacing(0);
  mylayout->setMargin(0);
  mylayout->setAlignment(Qt::AlignTop);
  // mylayout->setColumnStretch(0,0);
  // mylayout->setColumnStretch(1,10);

  for (size_t i=0; i<testids.size(); i++) {
    string scorename=patient.scores[testids[i]].scorename;
    layout_node(mylayout,scorename,testids[i],NULL);
  }
}

// handle item click is our general purpose handler for item clicks in
// table widgets.  it checks for associated string data, and if found,
// treats the string as a sort of URL.

void
DBview::handleitemclick(QTableWidgetItem *twi)
{
  string link=twi->data(role_link).toString().toStdString();
  DBviewspec vs;
  tokenlist toks;
  toks.ParseLine(link);
  // cout << "link: " << link << endl;
  if (toks[0]=="session") {
    vs.name=(string)"Session "+toks[1];
    vs.entries.push_back((string)"session "+toks[1]);
    client->dbs.viewspecs["dynamic-generated"]=vs;
    layout_view("dynamic-generated");
  }
  else if (toks[0]=="test") {
    vs.name=(string)"Test "+toks[1];
    vs.entries.push_back((string)"test "+toks[1]);
    client->dbs.viewspecs["dynamic-generated"]=vs;
    layout_view("dynamic-generated");
  }
  else
    cout << "FIXME unknown link type" << endl;
}

void
DBview::layout_sessionlist(QVBoxLayout *layout)
{
  // layout->setMargin(10);
  QTableWidget *tw=new QTableWidget;
  tw->setRowCount(patient.sessions.size()+1);  // +1 for the 0 session (no session)
  tw->setColumnCount(3);
  QStringList headers;
  headers << "sessionid" << "date" << "info";
  tw->setHorizontalHeaderLabels(headers);
  tw->verticalHeader()->hide();
  tw->setAlternatingRowColors(1);
  tw->setSelectionMode(QAbstractItemView::SingleSelection);
  tw->setSortingEnabled(1);
  QObject::connect(tw,SIGNAL(itemDoubleClicked(QTableWidgetItem *)),this,SLOT(handleitemclick(QTableWidgetItem *)));

  map<int32,DBsession>::iterator si;
  int thisrow=0;
  
  // fixed "nosession" session
  // string slink="<a href=\"sesssion:"+strnum(0)+"\">"+strnum(0)+"</a>";
  QTableWidgetItem *it1=new QTableWidgetItem(strnum(0).c_str());
  it1->setData(role_link,QVariant(QString("session ")+QString(strnum(0).c_str())));
  QTableWidgetItem *it2=new QTableWidgetItem(QString("no session"));
  QTableWidgetItem *it3=new QTableWidgetItem("no session");
  it1->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  it2->setFlags(Qt::ItemIsEnabled);
  it3->setFlags(Qt::ItemIsEnabled);   
  tw->setItem(thisrow,0,it1);
  tw->setItem(thisrow,1,it2);
  tw->setItem(thisrow,2,it3);
  thisrow++;
  for (si=patient.sessions.begin(); si!=patient.sessions.end(); si++) {
    // slink="<a href=\"sesssion:"+strnum(si->first)+"\">"+strnum(si->first)+"</a>";
    it1=new QTableWidgetItem(strnum(si->first).c_str());
    it1->setData(role_link,QVariant(QString("session ")+QString(strnum(si->first).c_str())));
    it2=new QTableWidgetItem((si->second.date.getDateStr()));
    it3=new QTableWidgetItem("more info here!");
    it1->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    it2->setFlags(Qt::ItemIsEnabled);
    it3->setFlags(Qt::ItemIsEnabled);   
    tw->setItem(thisrow,0,it1);
    tw->setItem(thisrow,1,it2);
    tw->setItem(thisrow,2,it3);
    thisrow++;
  }
  tw->resizeColumnsToContents();
  layout->addWidget(tw,0,Qt::AlignTop);
}

void
DBview::handleSessionLink(QString name)
{
  // FIXME
  cout << name.toStdString() << endl;
}

DBQScoreBox *
DBview::layout_node(QFormLayout *layout,const string &scorename,int32 valueid,DBQScoreBox *parent,int row)
{
  if (client->dbs.scorenames.count(scorename)==0)
    return NULL;   // FIXME shouldn't happen
  DBscorename &sn=client->dbs.scorenames[scorename];

  DBQScoreBox *sb=NULL;
  vector<string> namechildren;
  vector<int32> valuechildren;

  // if it's a leaf, lay it out, plus if non-existing, lay out new node link
  if (sn.flags.count("leaf")) {
    sb=layout_single_score(layout,scorename,valueid,parent,row);
    // now put up a "new xxx" link when appropriate
    if (sn.flags.count("repeating") && valueid<0)
      layout_new_node_link(layout,scorename,parent);
    return sb;
  }
  // if it's a stub, lay out the stub, then each child, then the stub repeater
  else if (!sn.flags.count("repeating")) {
    // layout stub
    sb=layout_stub(layout,sn,valueid,row);
    // if it's a NEW stub, mark it dirty
    if (valueid<0)
      sb->f_dirty=1;
    // find all children of this scorename
    namechildren=getchildren(client->dbs.scorenamechildren,scorename);
    if (valueid>-1)
      valuechildren=getchildren(patient.children,valueid);
    else
      valuechildren.clear();
    for (size_t i=0; i<namechildren.size(); i++) {       // for each childscorename
      bool foundone=0;
      for (size_t j=0; j<valuechildren.size(); j++) {    // find each matching scorevalue
        if (patient.scores[valuechildren[j]].scorename==namechildren[i]) {
          layout_node(sb->childlayout,namechildren[i],valuechildren[j],sb);
          foundone=1;
        }
      }
      if (!foundone)
        layout_node(sb->childlayout,namechildren[i],nextid--,sb);
      // if it's repeating and we have edit privileges, put up a "new xxx" link
//       DBscorename *tmpc=&(client->dbs.scorenames[namechildren[i]]);
//       if (tmpc->f_repeating) {
//         layout_new_node_link(sb->childlayout,namechildren[i],sb);
//       }
    }
    return sb;
  }
  // FIXME non-leaf repeating
  else {
    cout << "shouldn't happen yet!" << endl;
    // sb=layout_stub(layout,si);
    //layout_subtree(layout,si);
    return sb;  // should never happen
  }
}

void
DBview::layout_new_node_link(QFormLayout *layout,const string &scorename,DBQScoreBox *parent)
{
  if (parent) {
    string perm=patient.scores[parent->scorevalueid].permission;
    if (perm!="rw") return;
  }

  // int newrow=layout->rowCount();
  DBscorename *sn=&(client->dbs.scorenames[scorename]);
  string linktext=(string)"add new "+sn->name+"...";
  DBnewbutton *mybutton=new DBnewbutton(linktext);
  mybutton->setStyleSheet(style_addscore.c_str());
  mybutton->layout=layout;
  mybutton->scorename=sn;
  mybutton->parent=parent;
  layout->addRow("",mybutton);
  QObject::connect(mybutton,SIGNAL(clicked(QFormLayout *,DBscorename *,DBQScoreBox *,int)),
                   this,SLOT(handle_new_node_link(QFormLayout *,DBscorename *,DBQScoreBox *,int)));
  // FIXME probably need a custom widget, so that we can associate the needed info
}

void
DBview::handle_new_node_link(QFormLayout *layout,DBscorename *sn,DBQScoreBox *parent,int row)
{
  // don't need return value? DBQScoreBox *tmp=layout_node(layout,sn->name,nextid--,parent,row);
  layout_node(layout,sn->name,nextid--,parent,row);

  // FIXME add it to any lists?
  // layout->insertRow(row,"hereitis",tmp);
}

DBQScoreBox *
DBview::layout_stub(QFormLayout *layout,DBscorename &sn,int32 valueid,int row)
{
  DBscorevalue *sv;
  // if the stub doesn't already exist, create it
  if (valueid<0) {
    DBscorevalue vv;
    vv.id=nextid--;
    valueid=vv.id;
    vv.scorename=sn.name;
    vv.datatype="stub";
    vv.patient=patient.patientID;
    if (sn.flags.count("default")) {
      string dv=sn.flags["default"];
      vv.deserialize((uint8 *)dv.c_str(),dv.size()+1);
    }
    vv.sessionid=-1;
    // FIXME why no parents for stubs???
    //if (parent)
    //vv.parentid=parent->scorevalueid;
    vv.index=0;
    // if (parent) {
    //   string perm=patient.scores[parent->scorevalueid].permission;
    //   vv.permission=perm;
    // }
    patient.scores[vv.id]=vv;
  }
  sv=&(patient.scores[valueid]);

  QLabel *label=new QLabel((scorebasename(sn.name)+":").c_str());
  label->setStyleSheet(style_scorelabel.c_str());

  DBQStub *stub=new DBQStub(NULL);
  stub->setEditable(0);

  stub->scorename=sn.name;
  stub->scorevalueid=sv->id;
  layout->insertRow(row,label,stub);
  int f_writable=0;
  // if (sv->permission=="rw")
  f_writable=0;
  stub->setEditable(f_writable);
  scoreboxlist.push_back(stub);
  stub->label=label;

  return stub;
}

DBQScoreBox *
DBview::layout_single_score(QFormLayout *layout,const string &scorename,int32 valueid,DBQScoreBox *parent,int row)
{
  // first figure out what we're talking about
  DBscorename &sn=client->dbs.scorenames[scorename];
  DBscorevalue *sv;
  DBQScoreBox *ret=NULL;

  // if the value doesn't already exist, create it
  if (valueid<0) {
    DBscorevalue vv;
    vv.id=nextid--;
    valueid=vv.id;
    vv.scorename=scorename;
    vv.datatype=sn.datatype;
    vv.patient=patient.patientID;
    if (sn.flags.count("default")) {
      string dv=sn.flags["default"];
      vv.deserialize((uint8 *)dv.c_str(),dv.size()+1);
    }
    vv.sessionid=-1;
    if (parent)
      vv.parentid=parent->scorevalueid;
    vv.index=0;
    if (parent) {
      string perm=patient.scores[parent->scorevalueid].permission;
      vv.permission=perm;
    }
    patient.scores[vv.id]=vv;
  }
  if (patient.scores.count(valueid)==0)
    exit(0);   // should never happen absent corruption
  sv=&(patient.scores[valueid]);

  // next make sure we have a DBQScoreBox structure to work with
  if (sn.datatype=="shortstring" || sn.datatype=="string") {
    DBQLineEdit *le=new DBQLineEdit;
    ret=le;
  }
  else if (sn.datatype=="datetime" || sn.datatype=="timedate" ||
           sn.datatype=="time" || sn.datatype=="date") {
    DBQTimeDate *td=new DBQTimeDate;
    if (sn.datatype=="time") {
      td->setFormat(1,0);
    }
    else if (sn.datatype=="date") {
      td->setFormat(0,1);
    }
    else if (sn.datatype=="datetime" || sn.datatype=="timedate") {
      td->setFormat(1,1);
    }
    ret=td;
  }
  else if (sn.datatype=="integer") {
    DBQLineEdit *le=new DBQLineEdit;
    le->setIntValidator(-999999999,999999999);
    ret=le;
  }
  else if (sn.datatype=="float") {
    DBQLineEdit *le=new DBQLineEdit;
    le->setDoubleValidator(-9e+99,9e+99,4);
    ret=le;
  }
  else if (sn.datatype=="text") {
    DBQTextEdit *le=new DBQTextEdit;
    ret=le;
  }
  else if (sn.datatype=="image") {
    DBQImage *le=new DBQImage;
    ret=le;
  }
  else if (sn.datatype=="brainimage") {
    DBQBrain *le=new DBQBrain;
    ret=le;
  }
  else if (sn.datatype=="bool") {
    DBQCombo *cb=new DBQCombo;
    cb->addValue("no");
    cb->addValue("yes");
    ret=cb;
  }
  else if (client->dbs.typemap.count(sn.datatype)) {
    DBtype tp=client->dbs.typemap[sn.datatype];
    DBQCombo *cb=new DBQCombo;
    for (size_t i=0; i<tp.values.size(); i++)
      cb->addValue(tp.values[i]);
    if (sn.flags.count("customizable"))
      cb->allowOther();
    ret=cb;
  }
  else {
    cout << "FIXME shouldn't happen" << endl;
    DBQLineEdit *le=new DBQLineEdit;
    ret=le;
  }
  if (valueid>=0)
    ret->setValue(*sv);

  // now do all the generic stuff
  ret->scorename=sn.name;
  ret->scorevalueid=sv->id;
  int f_writable=0;
  // if (sv->permission=="rw")
  f_writable=0;
  ret->setEditable(f_writable);
  scoreboxlist.push_back(ret);
  QLabel *label=new QLabel((scorebasename(sn.name)+":").c_str());
  label->setStyleSheet(style_scorelabel.c_str());
  ret->label=label;
  if (layout)
    layout->insertRow(row,label,ret);

  return ret;
}


DBnewbutton::DBnewbutton(string txt)
{
  setText(txt.c_str());
  QObject::connect(this,SIGNAL(clicked()),this,SLOT(heardclick()));
}


void
DBnewbutton::heardclick()
{
  int row;
  QFormLayout::ItemRole rr;
  layout->getWidgetPosition(this,&row,&rr);
  emit(clicked(layout,scorename,parent,row));
}

