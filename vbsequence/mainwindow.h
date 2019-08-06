
// mainwindow.h
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
// by Mjumbe Poe

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPixmap>
#include <QPoint>
#include <QValidator>
#include <QtGui>

#include <set>
#include <string>

#include "vbdataset.h"
#include "vbdatasetwidget.h"
#include "vbsequence.h"
#include "vbsequenceitem.h"
#include "vbsequencescene.h"
#include "vbsequenceview.h"

class QListView;

namespace QtVB {

class SequenceScene;
class SequenceView;
class JobTypeListModel;

class ExecNameValidator : public QRegExpValidator {
  Q_OBJECT

  SequenceView* sequenceView;

 public:
  ExecNameValidator(SequenceView* view, QObject* parent)
      : QRegExpValidator(QRegExp("[A-Za-z1-9_]+"), parent) {
    sequenceView = view;
  }

  shared_ptr<VB::Exec> currentExec;
  virtual State validate(QString& input, int& pos) const {
    VB::Sequence& seq = sequenceView->scene()->sequence();
    for (list<shared_ptr<VB::Exec> >::iterator iter =
             seq.root_block()->execs.begin();
         iter != seq.root_block()->execs.end(); ++iter) {
      shared_ptr<VB::Exec>& ei = *iter;

      if (currentExec != ei && input == QString::fromStdString(ei->name))
        return QValidator::Intermediate;
    }

    return QRegExpValidator::validate(input, pos);
  }
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = 0);
  virtual ~MainWindow();

 private:
  void setupWidgets();

  QValidator* execNameValidator;

  QTreeView* jobsList;
  QLineEdit* filterEditBox;

  SequenceView* sequenceWidget;
  QLineEdit* sequenceNameEditBox;
  QLineEdit* sequenceDirEditBox;
  QLineEdit* sequenceRootNodeEditBox;

  QLineEdit* execNameEditBox;
  QLabel* execDefNameLabel;
  QLineEdit* execDataPathEditBox;
  QLabel* jobCommandLabel;
  QTreeWidget* jobArgumentsWidget;
  QTreeWidget* jobFilesWidget;
  QtVB::DataSetWidget* dsWidget;

  QMenu* fileMenu;
  QAction* newAct;
  QAction* openAct;
  QAction* saveAct;
  QAction* getDSAct;
  QAction* enqueAct;
  QAction* exportAct;
  QAction* closeAct;

  QMenu* viewMenu;
  //      QAction* dataVisibleAct;

  QMenu* editMenu;
  QActionGroup* toolsGroup;
  QAction* moveToolAct;
  QAction* dependToolAct;

  QAction* deleteJobAct;
  QAction* toggleDetailAct;
  QList<QAction*> disconnectJobActs;
  QList<QAction*> disconnectDatActs;

  void createActions();
  void createMenus();

  QList<QPoint> positions;

  void updateSequenceView();

 private slots:
  void assignDataSet(const shared_ptr<VB::Job> job);
  void showSelectedExecInfo();
  void showSelectedExecInfo_helper();
  void toggleClickedBlockDetail(bool);
  void popupJobClickedMenu();
  void filterJobTypeList(const QString&);
  void jobNameChanged(const QString& name);
  void jobRunOnceStateChanged(int state);
  void jobDataPathChanged(const QString& group);

  // File actions
  void newSequence();
  void openSequence();
  void saveSequence();
  void getDSSequence();
  void enqueSequence();
  void exportSequence();
  void closeSequence();

  // View actions
  void toggleDataVisible(bool);

  // Edit actions
  void selectMoveTool();
  void selectDependTool();

  // Context Menu actions
  void deleteClickedJob();
  void disconnectClickedJobJob();
  void disconnectClickedArgRes();

 private:
  void import_jobtypes_from_dir(const char* dir, JobTypeListModel* model);
  void make_disconnect_job_actions(
      QMenu* disconnect_menu, std::list<shared_ptr<VB::Exec> > connected_jobs);
};
}  // namespace QtVB
#endif
