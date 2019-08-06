
DEFUNCT !
// dir.cpp
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

#include "dir.h"
#include <q3header.h>
#include <q3listview.h>
#include <q3whatsthis.h>
#include <qcombobox.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qvariant.h>
#include <QMouseEvent>

    dirView::dirView(QWidget * parent, char const *name)
    : Q3ListView(parent, name) {
}
dirView::~dirView() {}

void dirView::contentsMousePressEvent(QMouseEvent *e) {
  Q3ListViewItem *c = firstChild();
  while (c) {
    setSelected(c, false);
    c = c->nextSibling();
  }
  QPoint p(contentsToViewport(e->pos()));
  Q3ListViewItem *i = itemAt(p);
  setSelected(i, true);
}

dir::dir(QWidget *parent, const char *name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)

{
  if (!name) setName("dir");

  pbMkdir = new QPushButton(this, "pbMkdir");
  pbMkdir->setGeometry(QRect(10, 10, 101, 31));
  connect(pbMkdir, SIGNAL(clicked()), this, SLOT(createDirectory()));

  pbRmdir = new QPushButton(this, "pbRmdir");
  pbRmdir->setGeometry(QRect(110, 10, 100, 31));
  connect(pbRmdir, SIGNAL(clicked()), this, SLOT(deleteFile()));

  pbRendir = new QPushButton(this, "pbRendir");
  pbRendir->setGeometry(QRect(210, 10, 100, 31));
  connect(pbRendir, SIGNAL(clicked()), this, SLOT(renameFile()));

  cbRecentdirs = new QComboBox(FALSE, this, "cbRecentdirs");
  cbRecentdirs->setEnabled(TRUE);
  cbRecentdirs->setGeometry(QRect(75, 50, 171, 31));
  connect(cbRecentdirs, SIGNAL(activated(const QString &)), this,
          SLOT(changeToPreviousDir(const QString &)));

  lvDirList = new dirView(this, "lvDirList");
  lvDirList->addColumn(tr("Directories"));
  lvDirList->setGeometry(QRect(10, 90, 291, 221));
  connect(lvDirList, SIGNAL(doubleClicked(Q3ListViewItem *)), this,
          SLOT(selectedDirectory(Q3ListViewItem *)));
  lvDirList->setSelectionMode(Q3ListView::Multi);

  lblSelected = new QLabel(this, "lblSelected");
  lblSelected->setGeometry(QRect(10, 330, 301, 20));

  leSelect = new QLineEdit(this, "leSelect");
  leSelect->setGeometry(QRect(10, 350, 301, 30));

  pbOK = new QPushButton(this, "pbOK");
  pbOK->setGeometry(QRect(120, 390, 90, 31));
  connect(pbOK, SIGNAL(clicked()), this, SLOT(changeDirectories()));

  pbCancel = new QPushButton(this, "pbCancel");
  pbCancel->setGeometry(QRect(220, 390, 90, 31));
  connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelDirectoryForm()));

  languageChange();
  resize(QSize(337, 431).expandedTo(minimumSizeHint()));
}

dir::~dir() {}

void dir::languageChange() {
  setCaption(tr("Pick Directory"));
  pbMkdir->setText(tr("Create Dir"));
  pbRmdir->setText(tr("Delete File"));
  pbRendir->setText(tr("Rename File"));
  lvDirList->header()->setLabel(0, tr("Directories"));
  lblSelected->setText(QString::null);
  pbOK->setText(tr("OK"));
  pbCancel->setText(tr("Cancel"));
  char presentDir[STRINGLEN];
  getcwd(presentDir, STRINGLEN - 1);
  string fieldText = "Selection:";
  fieldText += presentDir;
  cancel = 0;
  lblSelected->setText(tr(fieldText.c_str()));
  insertIntoCurrentDirsComboBox(presentDir);
  populateListBox();
}

void dir::dir_destroyed(QObject *) {
  qWarning("dir::dir_destroyed(QObject*): Not implemented yet");
}

void dir::SetDirectory(string name) {
  directory = name;
  cbRecentdirs->clear();
  chdir(name.c_str());
  insertIntoCurrentDirsComboBox(const_cast<char *>(name.c_str()));
  string sdirectory = "Selected: ";
  sdirectory += name.c_str();
  lblSelected->setText(name.c_str());
  populateListBox();
  return;
}

void dir::populateListBox() {
  lvDirList->clear();
  struct stat st;
  char pat[STRINGLEN];
  char presentDir[STRINGLEN];
  if (!directory.size()) {
    getcwd(presentDir, STRINGLEN - 1);
    strcpy(pat, presentDir);
  } else
    strcpy(pat, directory.c_str());
  directory = "";  // so it does not over-ride changes
  strcat(pat, "/*");
  vglob vg(pat);
  Q3ListViewItem *item;
  item = new Q3ListViewItem(lvDirList, "./");
  item = new Q3ListViewItem(lvDirList, "../");
  for (size_t i = 0; i < vg.size(); i++) {
    if (stat(vg[i].c_str(), &st)) continue;
    if (S_ISDIR(st.st_mode)) {
      string dir = vg[i];
      item = new Q3ListViewItem(lvDirList, dir.c_str());
    }
  }
  return;
}

int dir::createDirectory() {
  bool ok;
  struct stat st;
  string fname = "Directory to create:";
  QString newName =
      QInputDialog::getText("Create Directory", fname.c_str(),
                            QLineEdit::Normal, QString::null, &ok, this);
  if (!ok) return 1;
  if (!stat(newName.ascii(), &st)) {
    string text = "The requested directory already exists.";
    QMessageBox::information(this, "Create Directory", text.c_str());
    return 0;
  }
  if (ok && !newName.isEmpty()) {
    int status = mkdir(newName.ascii(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (status >= 0) {
      string text = "fileview created the directory ";
      text += newName.ascii();
      QMessageBox::information(this, "Create Directory", text.c_str());
    } else {
      string text = "fileView failed to create directory ";
      text += newName.ascii();
      text += ". Check permissions or disk space.";
      QMessageBox::warning(this, "Warning!", text.c_str());
    }
  }
  populateListBox();
  return 0;
}

int dir::deleteFile() {
  bool ok;
  struct stat st;
  string fname = "Delete this file:";
  QString newName =
      QInputDialog::getText("File to delete:", fname.c_str(), QLineEdit::Normal,
                            QString::null, &ok, this);
  if (!ok) return 1;
  if (stat(newName.ascii(), &st) && ok && !newName.isEmpty()) {
    string warningText = "The file ";
    warningText += newName.ascii();
    warningText += " did not exist in this diectory.";
    QMessageBox::information(this, "Delete file", warningText.c_str());
    return -1;
  }
  if (ok && !newName.isEmpty()) {
    int status = unlink(newName.ascii());
    if (status >= 0) {
      string text = "fileview deleted the file ";
      text += newName.ascii();
      QMessageBox::information(this, "Delete file", text.c_str());
    } else {
      string text = "fileView failed to delete file ";
      text += newName.ascii();
      text += ". Check permissions.";
      QMessageBox::warning(this, "Warning!", text.c_str());
    }
  }
  populateListBox();
  return 0;
}

int dir::renameFile() {
  bool ok;
  struct stat st;
  string fname = "Rename this file: ";
  QString oldName =
      QInputDialog::getText("Rename File", fname.c_str(), QLineEdit::Normal,
                            QString::null, &ok, this);
  if ((!ok) || (oldName.isEmpty())) return 1;
  if (stat(oldName.ascii(), &st) && ok && !oldName.isEmpty()) {
    string warningText = "The file ";
    warningText += oldName.ascii();
    warningText += " did not exist in this diectory.";
    QMessageBox::information(this, "Rename File", warningText.c_str());
    return -1;
  }
  fname = "New name for file:";
  QString newName =
      QInputDialog::getText("Rename File", fname.c_str(), QLineEdit::Normal,
                            QString::null, &ok, this);
  if (ok && !newName.isEmpty()) {
    int status = rename(oldName.ascii(), newName.ascii());
    if (!status) {
      string text = "fileview renamed the file ";
      text += leSelect->text().ascii();
      text += " to ";
      text += newName.ascii();
      QMessageBox::information(this, "file renamed", text.c_str());
    } else {
      string text = "failed to rename file ";
      text += leSelect->text().ascii();
      QMessageBox::information(this, "file renamed", text.c_str());
    }
  }
  populateListBox();
  return 0;
}

int dir::cancelDirectoryForm() {
  cancel = 1;
  this->close();
  return 0;
}

void dir::selectedDirectory(Q3ListViewItem *lbi) {
  chdir(lbi->text(0).ascii());
  char presentDir[STRINGLEN];
  getcwd(presentDir, STRINGLEN);
  insertIntoCurrentDirsComboBox(const_cast<char *>(presentDir));
  string text = "Selected: ";
  text += presentDir;
  lblSelected->setText(text.c_str());
  populateListBox();
  return;
}

void dir::changeToPreviousDir(const QString &dir) {
  cbRecentdirs->clear();
  chdir(dir.ascii());
  insertIntoCurrentDirsComboBox(const_cast<char *>(dir.ascii()));
  string sdirectory = "Selected: ";
  sdirectory += dir.ascii();
  lblSelected->setText(sdirectory.c_str());
  populateListBox();
  return;
}

// function changes dirs in parent form
void dir::changeDirectories() {
  this->close();
  return;
}

void dir::insertIntoCurrentDirsComboBox(char *dir) {
  cbRecentdirs->insertItem(dir, 0);
  for (int num = 1; num < cbRecentdirs->count(); num++) {
    if (strcmp(dir, cbRecentdirs->text(num).ascii()) == 0)
      cbRecentdirs->removeItem(num);
  }
}
