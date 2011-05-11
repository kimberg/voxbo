
// dbmainwindow.h
// 
// Copyright (c) 2009 by The VoxBo Development Team

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

#include <QMainWindow>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
// #include <QStatusBar>
#include <QListWidget>
#include <QComboBox>
#include <QTreeWidget>
#include "myboxes.h"

#include "dbclient.h"

class DBmainwindow : public QMainWindow {
  Q_OBJECT
public:
  DBmainwindow();
  void resizeEvent(QResizeEvent *re);
  string v_motd;
private slots:
  void open();
  void connect();
  void create();
  void search();
  void test();
  void close();
  void newuser();
  void newpatient();
  void online();
  void handleAreaList();
  void handleSearch();
  void handleSaveResults();
  void handleSearchResultClicked(QListWidgetItem *it);
private:
  DBclient *client;
  int currentpanel;
  void setup();
  void populate();
  void updatevisiblewidgets();
  void arrangeChildren();
  // actions for everyone
  QAction *a_open,*a_connect,*a_create,*a_search,*a_test,*a_close;
  // actions for admins
  QAction *a_newuser,*a_online,*a_newpatient;

  // central widget
  QFrame *myframe;
  QMenuBar *w_menubar;
  // QStatusBar *w_statusbar;
  // stuff inside central widget
  QLabel *w_logo;
  QLabel *w_banner;
  QLabel *w_motd;
  QFrame *w_messages;
  // the "arealist" and selected items therein
  QListWidgetItem *al_aboutitem,*al_resultsitem;
  QListWidget *w_arealist;
  // menus
  QMenu *filemenu;
  QMenu *adminmenu;
  QMenu *secondmenu;

  // user info
  QVBox *w_userinfo;
  // db info
  QVBox *w_dbinfo;
  // patient lists
  QVBox *w_lists;
  QTreeWidget *w_plisttree;
  // search box and search results
  QHBox *w_searchbox;
  QComboBox *s_field;
  QComboBox *s_relation;
  QLineEdit *s_searchline;
  vector<string> searchScoreIDs;
  QVBox *w_resultsbox;
  QListWidget *w_searchresults;

  // widget building
  QHBox *makesearchbox();
  QVBox *makeplistbox();
  QVBox *makedbinfobox();
  QVBox *makeuserinfobox();
  QVBox *makeresultsbox();
};
