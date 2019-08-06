
DEFUNCT !

// dir.h
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Tom King

#ifndef DIR_H
#define DIR_H

#include <q3listview.h>
#include <qdialog.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qvariant.h>
// Added by qt3to4:
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include "vbprefs.h"

    class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QComboBox;
class QLabel;
class QLineEdit;
class Q3ListView;
class Q3ListViewItem;
class QPushButton;
class dirView;

class dirView : public Q3ListView {
  Q_OBJECT
 public:
  dirView(QWidget* parent = 0, const char* name = 0);
  ~dirView();
 public slots:
 signals:
 protected:
  void contentsMousePressEvent(QMouseEvent* e);

 private:
};

class dir : public QDialog {
  Q_OBJECT

 public:
  dir(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
      Qt::WFlags fl = 0);
  ~dir();

  QPushButton* pbMkdir;
  QPushButton* pbRmdir;
  QPushButton* pbRendir;
  QComboBox* cbRecentdirs;
  dirView* lvDirList;
  QLabel* lblSelected;
  QLineEdit* leSelect;
  QPushButton* pbOK;
  QPushButton* pbCancel;
  void SetDirectory(string);
  char* returnDesiredDir();
  void insertIntoCurrentDirsComboBox(char*);
 public slots:
  virtual void dir_destroyed(QObject*);

 protected:
 private:
  int cancel;
  string directory;
 protected slots:
  virtual void languageChange();
  void populateListBox();
  int createDirectory();
  int deleteFile();
  int renameFile();
  int cancelDirectoryForm();
  void selectedDirectory(Q3ListViewItem*);
  void changeToPreviousDir(const QString&);
  void changeDirectories();
};

#endif  // DIR_H
