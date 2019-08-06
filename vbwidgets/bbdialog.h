
// bbdialog.h
// dialogs for structure browser
// Copyright (c) 2010 by The VoxBo Development Team

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
// original version written by Dan Kimberg, based on a design by
// Dongbo Hu.

#ifndef BBDIALOG_H
#define BBDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include "brain_util.h"
#include "myboxes.h"

using namespace std;

bool setFiles(string rDbName, string rrDbName, string sDbName, string& dbHome);

// BBdialog is a tand-alone class that takes care of user interface display
class BBdialog : public QDialog {
  Q_OBJECT

 public:
  BBdialog(QWidget* parent = 0);

 private:
  void buildUI();
  void buildList();
  bool setFiles();
  int parseRegionName(QString, bool, QString&, QString&);
  bool chkNameSpace(QString);
  void setNameSpace(string);
  void searchRegion(string, string);
  void searchSynonym(string, string);
  void clearUI();

  void showParentChild();
  void showParent();
  void showChild();

  void showSynonym();
  bool chkName(string);
  void showRelation();
  void keyPressEvent(QKeyEvent*);

  // data members
  QLineEdit* ui_name;
  QListWidget* ui_hintList;
  QComboBox* ui_namespace;
  QListWidget* ui_parent;
  QListWidget* ui_child;
  QListWidget* ui_synonyms;
  QLineEdit* ui_source;
  QLineEdit* ui_link;
  QListWidget* ui_relationship;

  string dbHome, rDbName, rrDbName, sDbName;
  vector<string> nameList;
  vector<string> spaceList;
  long regionID;
  string regionName, name_space;

 private slots:
  void popupList();
  void selectName(QListWidgetItem*);
  void changeNS();
  void toParent(QListWidgetItem*);
  void toChild(QListWidgetItem*);
  void toRelated(QListWidgetItem*);
  void resetUI();
  void about();
};

#endif
