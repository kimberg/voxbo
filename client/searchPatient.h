
// searchPatient.h
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
// original version written by Dongbo Hu

/****************************************************************************
**
** Header of searchPatient interface.
**
****************************************************************************/

#ifndef SEARCHPATIENT_H
#define SEARCHPATIENT_H

#include <QListWidget>
#include <QMainWindow>
#include <QRadioButton>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <string>
#include "db_util.h"
#include "dbclient.h"

class SearchPatient : public QMainWindow
{
  Q_OBJECT

public:
  SearchPatient(DBclient *c);
  void setupUI();
  void addTitle();
  void addSearch();
  void addButtons();
  void setDockWindow();
  void setTags(patientSearchTags &);
  void showMatches(const vector<patientMatch> &);

public slots:
  void startSearch();
  void clickReset();
  void clickCancel();
  void showOnePatient();

public:
  DBclient* dbcp;

  QScrollArea *mainArea;
  QWidget *sp_ui;
  QVBoxLayout *spLayout;

  QComboBox *s_field;
  QComboBox *s_relation;
  QLineEdit *s_inputLn;
  QGroupBox *optBox1;
  QRadioButton *searchAll, *searchPart;
  QRadioButton *sensButt, *insensButt;
  QWidget *searchResults;
  QLabel *r_line1, *r_line2;
  QListWidget *r_list;

  vector <string> searchScoreIDs;  // FIXME dan changed this from int32 to string -- ok?
  vector <int32> currentPatients;

  //string dbHome, scoreValueDbName, sessionDbName;
  //string adminDbName, pidStr, patientDbName;
};

bool isNewElement(vector<string>, string);
bool isNewElement(vector<int32>, int32);

/* class ShowPatient : public QMainWindow */
/* { */
/*   Q_OBJECT */

/* public: */
/*   ShowPatient(int, vector<string>); */

/*   int showPatientInfo(int, vector<string>); */
/*   void addScoreInfo(string, string); */

/*   string dbHome, scoreDbName, scoreValueDbName; */
/*   QScrollArea *mainArea; */
/*   QWidget *main_ui; */
/*   QVBoxLayout *mainLayout; */
/* }; */


#endif
