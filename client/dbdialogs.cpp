
// dbdialogs.cpp
// dialogs for db client
// Copyright (c) 2011 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg

#include <QPushButton>
#include <QStringList>
#include <QFormLayout>

#include "dbdialogs.moc.h"
#include "myboxes.h"

using namespace std;

DBnewsession::DBnewsession(QWidget *parent)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);

  hb=new QHBox;
  layout->addWidget(hb);

  hb->addLabel("Name:");
  w_name=new QLineEdit();
  hb->addWidget(w_name);

  hb=new QHBox;
  layout->addWidget(hb);

  hb->addLabel("Date:");
  w_date=new QDateTimeEdit();
  w_date->setCalendarPopup(1);
  w_date->setDisplayFormat("d-MMM-yyyy h:mmap");
  hb->addWidget(w_date);

  hb=new QHBox;
  layout->addWidget(hb);

  hb->addLabel("Location:");
  w_location=new QLineEdit();
  hb->addWidget(w_location);

  hb=new QHBox;
  layout->addWidget(hb);

  hb->addLabel("Examiner:");
  w_examiner=new QLineEdit();
  hb->addWidget(w_examiner);

  hb=new QHBox;
  layout->addWidget(hb);

  hb->addLabel("Notes:");
  w_notes=new QLineEdit();
  hb->addWidget(w_notes);

  layout->insertSpacing(-1,40);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));
  setMinimumSize(320,1);
  setWindowTitle("mydialog");
}

DBpicksession::DBpicksession(QWidget *parent,DBpatient *p)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);

  w_tree=new QTreeWidget();
  layout->addWidget(w_tree);
  w_tree->setColumnCount(3);
  QStringList hh;
  hh<<"Session ID"<<"date"<<"examiner";
  w_tree->setHeaderLabels(hh);
  w_tree->setAllColumnsShowFocus(1);
  w_tree->setRootIsDecorated(0);
  w_tree->setUniformRowHeights(1);

  pair<int32,DBsession> s;
  int f_first=1;
  vbforeach(s,p->sessions) {
    QTreeWidgetItem *it=new QTreeWidgetItem(w_tree);
    if (f_first) {
      w_tree->setCurrentItem(it);
      f_first=0;
    }
    it->setText(0,strnum(s.second.id).c_str());
    it->setText(1,s.second.date.getDateStr());
    it->setText(2,s.second.examiner.c_str());
    it->setData(0,0,QVariant(s.second.id));
    w_tree->addTopLevelItem(it);
  }

  layout->insertSpacing(-1,40);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("mydialog");
}

int32
DBpicksession::selectedsession()
{
  return w_tree->currentItem()->data(0,0).toUInt();
}

DBpicktest::DBpicktest(QWidget *parent,DBclient *c)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);

  w_tree=new QTreeWidget();
  layout->addWidget(w_tree);
  w_tree->setColumnCount(3);
  QStringList hh;
  hh<<"Test ID"<<"name"<<"that's it?";
  w_tree->setHeaderLabels(hh);
  w_tree->setAllColumnsShowFocus(1);
  w_tree->setRootIsDecorated(0);
  w_tree->setUniformRowHeights(1);

  int f_first=1;
  vector<string> testlist=getchildren(c->dbs.scorenamechildren,"");
  vbforeach(string &s,testlist) {
    QTreeWidgetItem *it=new QTreeWidgetItem(w_tree);
    if (f_first) {
      w_tree->setCurrentItem(it);
      f_first=0;
    }
    it->setText(0,s.c_str());
    it->setText(1,s.c_str());
    it->setText(2,"nomore");
    it->setData(0,0,QVariant(s.c_str()));
    w_tree->addTopLevelItem(it);
  }

  layout->insertSpacing(-1,40);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("mydialog");
}

DBplist::DBplist(QWidget *parent,DBpatientlist &)
  : QDialog(parent)
{
  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);
  
  // FIXME actually we need a place to edit the name, patients, and
  // notes
  w_tree=new QTreeWidget();
  layout->addWidget(w_tree);
  w_tree->setColumnCount(3);
  QStringList hh;
  hh<<"Test ID"<<"name"<<"that's it?";
  w_tree->setHeaderLabels(hh);
  w_tree->setAllColumnsShowFocus(1);
  w_tree->setRootIsDecorated(0);
  w_tree->setUniformRowHeights(1);

  layout->insertSpacing(-1,40);

  QHBox *hb=new QHBox();
  layout->addWidget(hb);
  QPushButton *button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("choose a patient list");
}

string
DBpicktest::selectedtest()
{
  return w_tree->currentItem()->data(0,0).toString().toStdString();
}

DBlocallogin::DBlocallogin(QWidget *parent)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(0);
  layout->setMargin(0);
  this->setLayout(layout);

  QFormLayout *myform=new QFormLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  layout->addLayout(myform);

  w_username=new QLineEdit();
  w_password=new QLineEdit();
  w_password->setEchoMode(QLineEdit::Password);
  w_dirname=new QLineEdit();

  myform->addRow("username:",w_username);
  myform->addRow("password:",w_password);
  myform->addRow("directory:",w_dirname);

  w_message=new QLabel();
  QPalette plt;
  plt.setColor(QPalette::WindowText,Qt::red);
  w_message->setPalette(plt);
  layout->addWidget(w_message);

  layout->insertSpacing(-1,25);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("mydialog");
}


DBremotelogin::DBremotelogin(QWidget *parent)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(0);
  layout->setMargin(0);
  this->setLayout(layout);

  QFormLayout *myform=new QFormLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  layout->addLayout(myform);

  w_username=new QLineEdit();
  w_password=new QLineEdit();
  w_password->setEchoMode(QLineEdit::Password);
  w_servername=new QLineEdit();
  w_serverport=new QLineEdit();
  w_serverport->setText("6010");

  myform->addRow("username:",w_username);
  myform->addRow("password:",w_password);
  myform->addRow("server:",w_servername);
  myform->addRow("port:",w_serverport);

  w_message=new QLabel();
  QPalette plt;
  plt.setColor(QPalette::WindowText,Qt::red);
  w_message->setPalette(plt);
  layout->addWidget(w_message);

  layout->insertSpacing(-1,25);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("mydialog");
}


DBuserinfo::DBuserinfo(QWidget *parent)
  : QDialog(parent)
{
  QHBox *hb;
  QPushButton *button;

  QVBoxLayout *layout=new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);

  QFormLayout *myform=new QFormLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(3);
  layout->addLayout(myform);

  w_username=new QLineEdit();
  w_password=new QLineEdit();
  w_name=new QLineEdit();
  w_phone=new QLineEdit();
  w_email=new QLineEdit();
  w_address=new QLineEdit();
  w_password->setEchoMode(QLineEdit::Password);

  myform->addRow("username:",w_username);
  myform->addRow("password:",w_password);
  myform->addRow("name:",w_name);
  myform->addRow("phone:",w_phone);
  myform->addRow("email:",w_email);
  myform->addRow("address:",w_address);

  layout->insertSpacing(-1,40);

  hb=new QHBox();
  layout->addWidget(hb);
  button=new QPushButton("Done");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  button=new QPushButton("Cancel");
  hb->addWidget(button);
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  setMinimumSize(320,1);
  setWindowTitle("User data");
}
