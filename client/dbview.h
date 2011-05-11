
// dbview.h
// Copyright (c) 2010 by The VoxBo Development Team

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

#include <QMainWindow>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QVariant>

#include <vector>
#include <string>

#include "mydefs.h"
#include "dbclient.h"
#include "tokenlist.h"
#include "vbutil.h"
#include "dbqscorebox.h"

// locally used Qt Roles
const int role_link=Qt::UserRole;

class DBview : public QMainWindow {
  Q_OBJECT
public:
  DBview(DBclient *c,DBpatient &p);
  DBview(DBclient *c,int32 patientid);
  void init();
  QTabWidget *tabwidget;
  QToolBar *toolbar;
  QStatusBar *statusbar;
  DBpatient patient;
  DBclient *client;
  // string name;
  int32 nextid;              // fake id for values created in the interface
  vector<DBQScoreBox *> scoreboxlist;
  string style_main;
  string style_scorelabel;      // style for the label next to each score
  string style_sessionbanner;   // style for the session info banner/separator
  string style_addscore;        // style for the little "add xxx..." link

  // layout methods
  DBQScoreBox *layout_node(QFormLayout *layout,const string &scorename,int32 valueid,DBQScoreBox *parent,int row=-1);
  DBQScoreBox *layout_stub(QFormLayout *layout,DBscorename &sn,int32 valueid,int row=-1);
  DBQScoreBox *layout_single_score(QFormLayout *layout,const string &scorename,int32 valueid,DBQScoreBox *parent,int row=-1);

  void layout_view(string viewname);
  void layout_banner(QVBoxLayout *layout,tokenlist &toks);
  void layout_test(QVBoxLayout *layout,tokenlist &toks);
  void layout_sessionlist(QVBoxLayout *layout);
  void layout_testlist(QVBoxLayout *layout);
  void layout_session(QVBoxLayout *layout,int32 sessionid);
  void layout_new_node_link(QFormLayout *layout,const string &scorename,DBQScoreBox *parent);
  operator bool() {return (bool)patient;}

public slots:
  void handleViewSelect(int index);
  void handleTabClose(int index);

  void handleSubmit();
  void handleRevertAll();
  void handleNewSession();
  void handleNewTest();
  void handleEdit();
  void handleDump();

  void handleSessionLink(QString name);
  void handle_new_node_link(QFormLayout *layout,DBscorename *sn,DBQScoreBox *parent,int row);
  void handleitemclick(QTableWidgetItem *twi);
};


class DBnewbutton : public QPushButton {
  Q_OBJECT
public:
  DBnewbutton(string txt);
  QFormLayout *layout;
  DBscorename *scorename;
  DBQScoreBox *parent;
public slots:
  void heardclick();
signals:
  void clicked(QFormLayout *,DBscorename *,DBQScoreBox *,int row);
};
