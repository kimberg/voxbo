
// fileview.h
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
// original version written by Tom King, modified by Dan Kimberg

#ifndef FILEVIEW_H
#define FILEVIEW_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "myboxes.h"
#include "vbprefs.h"

class fileview : public QDialog {
  Q_OBJECT

 public:
  fileview(QWidget* parent = 0, const char* name = 0, bool modal = false,
           Qt::WFlags fl = Qt::WType_TopLevel);
  ~fileview();

  QPushButton* pbDirectory;
  QLineEdit* leDirectory;
  QLineEdit* leFileNamePattern;
  QHBox* grpFileBorder;
  QTreeWidget* lvFiles;
  void SetDirectory(string);
  void SetPattern(string);
  void ShowImageInformation(bool x);
  vector<string> Go();
  void ShowDirectoriesAlso(bool x);

 protected:
 private:
  bool showdirs;
  bool okayed;
  string directory;
  vector<string> returnedFiles;
  vector<string> returnSelectedFiles();
 signals:
 public slots:
  bool Okayed();
 protected slots:
  void Handler();
  void HandleUp();
  void HandleHome();
  void HandleRoot();
  void HandleNewWD();
  void Selected(QTreeWidgetItem* item, int col);
  void populateListBox();
  void Cancel();
  void grayDir();
};

#endif  // FILEVIEW_H
