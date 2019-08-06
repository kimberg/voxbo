
// mainwindow.cpp
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
// original version written by Mjumbe Poe

#include <stdlib.h>
#include <QStringList>
#include <QStringListModel>
#include <QTimer>
#include <QtGui>

#include "mainwindow.h"
#include "vbjobtypelistmodel.h"

#include <iostream>

// using namespace VB;
using namespace QtVB;
using namespace std;

VBPrefs vbp;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setupWidgets();

  setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  setWindowTitle(tr("vbsequence"));

  createActions();
  createMenus();

  connect(filterEditBox, SIGNAL(textChanged(const QString&)), this,
          SLOT(filterJobTypeList(const QString&)));
  connect(execNameEditBox, SIGNAL(textChanged(const QString&)), this,
          SLOT(jobNameChanged(const QString&)));
  connect(execDataPathEditBox, SIGNAL(textChanged(const QString&)), this,
          SLOT(jobDataPathChanged(const QString&)));
  connect(sequenceWidget->scene(), SIGNAL(selectionChanged()), this,
          SLOT(showSelectedExecInfo()));
}

void MainWindow::createActions() {
  newAct = new QAction("&New", this);
  newAct->setShortcut(QKeySequence("Ctrl+N"));
  newAct->setStatusTip("Create a new sequence");
  newAct->setEnabled(false);
  connect(newAct, SIGNAL(triggered()), this, SLOT(newSequence()));

  openAct = new QAction("&Open...", this);
  openAct->setShortcut(QKeySequence("Ctrl+O"));
  openAct->setStatusTip("Open an existing sequence...");
  connect(openAct, SIGNAL(triggered()), this, SLOT(openSequence()));

  saveAct = new QAction("&Save...", this);
  saveAct->setShortcut(QKeySequence("Ctrl+S"));
  saveAct->setStatusTip("Save current sequence to a file...");
  connect(saveAct, SIGNAL(triggered()), this, SLOT(saveSequence()));

  getDSAct = new QAction("&Apply Dataset...", this);
  connect(getDSAct, SIGNAL(triggered()), this, SLOT(getDSSequence()));

  enqueAct = new QAction("Submit to &queue", this);
  connect(enqueAct, SIGNAL(triggered()), this, SLOT(enqueSequence()));

  exportAct = new QAction("E&xport to disk...", this);
  connect(exportAct, SIGNAL(triggered()), this, SLOT(exportSequence()));

  closeAct = new QAction("&Close", this);
  closeAct->setShortcut(QKeySequence("Ctrl+W"));
  closeAct->setStatusTip("Close the current sequence");
  connect(closeAct, SIGNAL(triggered()), this, SLOT(closeSequence()));

  //  dataVisibleAct = new QAction("&Data Visible", this);
  //  dataVisibleAct->setCheckable(true);
  //  connect(dataVisibleAct, SIGNAL(toggled(bool)), this,
  //  SLOT(toggleDataVisible(bool)));

  moveToolAct = new QAction("&Move tool", this);
  moveToolAct->setShortcut(QKeySequence("Ctrl+M"));
  moveToolAct->setCheckable(true);
  connect(moveToolAct, SIGNAL(triggered()), this, SLOT(selectMoveTool()));

  dependToolAct = new QAction("&Depend tool", this);
  dependToolAct->setShortcut(QKeySequence("Ctrl+D"));
  dependToolAct->setCheckable(true);
  connect(dependToolAct, SIGNAL(triggered()), this, SLOT(selectDependTool()));

  toolsGroup = new QActionGroup(this);
  toolsGroup->addAction(moveToolAct);
  toolsGroup->addAction(dependToolAct);
  moveToolAct->setChecked(true);

  deleteJobAct = new QAction("Delete this job", this);
  connect(deleteJobAct, SIGNAL(triggered()), this, SLOT(deleteClickedJob()));

  toggleDetailAct = new QAction("Toggle block detail", this);
  toggleDetailAct->setCheckable(true);
  connect(toggleDetailAct, SIGNAL(toggled(bool)), this,
          SLOT(toggleClickedBlockDetail(bool)));
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu("&Sequence");
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveAct);
  fileMenu->addAction(getDSAct);
  fileMenu->addAction(exportAct);
  fileMenu->addAction(enqueAct);
  fileMenu->addSeparator();
  fileMenu->addAction(closeAct);

  viewMenu = menuBar()->addMenu("&View");
  //  viewMenu->addAction(dataVisibleAct);

  editMenu = menuBar()->addMenu("&Edit");
  editMenu->addAction(moveToolAct);
  editMenu->addAction(dependToolAct);
}

MainWindow::~MainWindow() {}

void MainWindow::assignDataSet(const shared_ptr<VB::Job> /*job*/) {}

void MainWindow::showSelectedExecInfo() {
  // ::NOTE:: Since, as of Qt 4.3.1, the selectionChanged signal may fire before
  //          the selectedItem set of a QGraphicsScene has actually been updated
  //          let's wait for a few miliseconds here before attempting to find
  //          the current selection.
  QTimer::singleShot(100, this, SLOT(showSelectedExecInfo_helper()));
}

void MainWindow::showSelectedExecInfo_helper() {
  ExecItem* eitem;
  shared_ptr<VB::Exec> e;

  if ((eitem = sequenceWidget->scene()->selectedExecItem()) &&
      (e = eitem->exec())) {
    execNameEditBox->setEnabled(true);
    execNameEditBox->setText(QString::fromStdString(e->name));
    execDefNameLabel->setText(QString::fromStdString(e->def->name()));
    execDataPathEditBox->setText(QString::fromStdString(e->full_path()));

    if (eitem->sequenceItemType() == "job") {
      VB::JobType* jt = static_cast<VB::JobType*>(e->def);
      if (!jt->cmds().empty())
        jobCommandLabel->setText(QString::fromStdString(
            jt->cmds().front().command));  // <-- just to see for now
    }

    // Construct the full dataset
    QString seq_path = sequenceRootNodeEditBox->text();
    std::string ds_path = seq_path.toStdString();

    QString job_path = execDataPathEditBox->text();
    if (job_path != "") {
      if (ds_path != "")
        ds_path += ":" + job_path.toStdString();
      else
        ds_path = job_path.toStdString();
    }

    VB::DataSet* root = 0;
    VB::DataSet* ds;
    try {
      root = new VB::DataSet(dsWidget->dataset());
      ds = root;
      if (ds_path != "") {
        list<VB::DataSet*> ds_list = ds->get_children(ds_path);
        if (!ds_list.empty()) {
          ds = ds_list.front();
        }
      }
    } catch (...) {
      cerr << "catch1" << endl;
      throw;
    }

    VB::DataSet::Member* mem;
    if (sequenceDirEditBox->text() != "") {
      mem = ds->insert_member(VB::WORKING_DIR_STRING,
                              false);  // <-- do not override existing member
      mem->value = sequenceDirEditBox->text().toStdString();
    }

    if (sequenceNameEditBox->text() != "") {
      mem = ds->insert_member(VB::SEQUENCE_NAME_STRING, false);
      mem->value = sequenceNameEditBox->text().toStdString();
    }

    // Populate the arguments box
    jobArgumentsWidget->clear();
    VB::JobType* jt = dynamic_cast<VB::JobType*>(e->def);
    if (jt) {
      list<VB::JobType::Argument>& jt_args = jt->args();
      for (list<VB::JobType::Argument>::iterator iter = jt_args.begin();
           iter != jt_args.end(); ++iter) {
        VB::JobType::Argument& arg = *iter;

        QStringList name_val;
        name_val.append(QString::fromStdString(arg.name));

        // exec line, dataset, default
        map<string, string>::iterator data_iter;
        VB::DataSet::Member* data_mem = 0;
        if ((data_iter = e->data.find(arg.name)) != e->data.end()) {
          name_val.append(QString::fromStdString(data_iter->second));
        } else if ((data_mem = ds->get_member(arg.name)) != 0) {
          name_val.append(QString::fromStdString(data_mem->value));
        } else if ((data_iter = arg.info.find("default")) != arg.info.end()) {
          name_val.append(QString::fromStdString(data_iter->second));
        } else {
          name_val.append("(no value found)");
        }

        jobArgumentsWidget->addTopLevelItem(new QTreeWidgetItem(name_val));
      }

      // Populate the files box
      jobFilesWidget->clear();
      foreach (const VB::JobType::File& f, jt->files()) {
        QStringList name_string_type;
        name_string_type.append(QString::fromStdString(f.id));
        name_string_type.append(
            QString::fromStdString(ds->get_resolved_string(f.in_name)));

        string informatstring = "( ";
        for (list<string>::const_iterator iter = f.in_types.begin();
             iter != f.in_types.end(); ++iter) {
          const string& format = *iter;
          informatstring += format + " ";
        }
        informatstring += ")";

        string outformatstring = "( ";
        // foreach pair<string,string> format_name in f.out_names
        for (map<string, string>::const_iterator iter = f.out_names.begin();
             iter != f.out_names.end(); ++iter) {
          const pair<const string, string>& format_name = *iter;
          outformatstring +=
              format_name.second + "[" + format_name.first + "] ";
        }
        outformatstring += ")";

        name_string_type.append(
            QString::fromStdString(informatstring + " --> " + outformatstring));

        jobFilesWidget->addTopLevelItem(new QTreeWidgetItem(name_string_type));
      }
    }

    if (root) delete root;
  }
}

void MainWindow::updateSequenceView() {
  sequenceWidget->scene()->update(sequenceWidget->scene()->sceneRect());
  sequenceWidget->scene()->arrange();
  //    jobPropsWidget->updateAll();
}

void MainWindow::jobNameChanged(const QString& name) {
  ExecItem* eitem = sequenceWidget->scene()->selectedExecItem();
  if (!eitem) {
    return;
  }

  shared_ptr<VB::Exec> e = eitem->exec();
  if (e) {
    e->name = name.toStdString();
    updateSequenceView();
  }
}

void MainWindow::jobRunOnceStateChanged(int /*state*/) {
  shared_ptr<VB::Exec> ei = sequenceWidget->scene()->selectedExecItem()->exec();
  if (ei) {
    //    job->set_run_once(state == Qt::Checked);
    //    jobRunGroupEditBox->setEnabled(!job->get_run_once());
    //    jobRunGroupEditBox->setText(job->get_run_group().c_str());
    updateSequenceView();
  }
}

void MainWindow::jobDataPathChanged(const QString& path) {
  ExecItem* eitem = sequenceWidget->scene()->selectedExecItem();
  if (!eitem) {
    return;
  }

  shared_ptr<VB::Exec> ei = eitem->exec();
  if (ei) {
    ei->path = path.toStdString();
    updateSequenceView();
  }
}

void MainWindow::filterJobTypeList(const QString& keyword) {
  int rc = jobsList->model()->rowCount();
  for (int i = 0; i < rc; ++i) {
    QAbstractItemModel* sm = jobsList->model();

    const QModelIndex& name_ind = sm->index(i, 0);
    QString name = sm->data(name_ind, Qt::DisplayRole).toString();

    const QModelIndex& desc_ind = sm->index(i, 1);
    QString desc = sm->data(desc_ind, Qt::DisplayRole).toString();

    if (name.indexOf(keyword) != -1 || desc.indexOf(keyword) != -1) {
      jobsList->setRowHidden(i, QModelIndex(), false);
    } else {
      jobsList->setRowHidden(i, QModelIndex(), true);
    }
  }
}

void MainWindow::newSequence() {
  // currently disabled (a tab system, perhaps?)
}

void MainWindow::openSequence() {
  QString seq_name = QFileDialog::getOpenFileName(
                    this,
                    "Choose a sequence file",
                    QString()/*,
                    "Sequence Files (*.sequence)"*/);

  if (seq_name != "") {
    VB::Sequence seq;
    ifstream in_file(seq_name.toAscii());
    if (in_file.good()) seq.read(in_file);
    sequenceWidget->scene()->loadSequence(seq);

    sequenceNameEditBox->setText(QString::fromStdString(seq.name()));
    sequenceDirEditBox->setText(QString::fromStdString(seq.vars()["DIR"]));

    updateSequenceView();
  }
}

void MainWindow::saveSequence() {
  QString seq_name = QFileDialog::getSaveFileName(
                    this,
                    "Save sequence file as...",
                    QString()/*,
                    "Sequence Files (*.sequence)"*/);

  if (seq_name != "") {
    VB::Sequence seq;
    sequenceWidget->scene()->saveSequence(seq);

    typedef shared_ptr<VB::Exec> ExecPointer;
    seq.sort_execs();
    seq.root_block()->def->sync_to_instance(seq.root_block());

    ofstream outfile;
    outfile.open(seq_name.toAscii());
    if (outfile.good()) {
      seq.write(outfile);
      outfile.close();
    }
  }
}

void MainWindow::getDSSequence() {
  QString ds_name = QFileDialog::getOpenFileName(
                    this,
                    "Choose a dataset file",
                    QString()/*,
                    "Dataset Files (*.dataset)"*/);

  if (ds_name != "") {
    VB::DataSet ds = VB::DataSet(ds_name.toStdString());

    dsWidget->setDataset(ds);
    dsWidget->expandAll();
    dsWidget->show();

    sequenceWidget->scene()->setDataSet(&ds);

    updateSequenceView();
  }
}

void MainWindow::enqueSequence() {
  VB::Sequence seq;
  sequenceWidget->scene()->saveSequence(seq);
  seq.sort_execs();
  seq.root_block()->def->sync_to_instance(seq.root_block());
  seq.name(sequenceNameEditBox->text().toStdString());

  VB::DataSet& widget_ds = dsWidget->dataset();
  list<VB::DataSet*> ds_list =
      widget_ds.get_children(sequenceRootNodeEditBox->text().toStdString());
  if (!ds_list.empty()) {
    for (list<VB::DataSet*>::iterator iter = ds_list.begin();
         iter != ds_list.end(); ++iter) {
      VB::DataSet* ds = *iter;
      //      ExecPoint beginPoint(shared_ptr<VB::Exec>(),0);
      //      ExecPoint endPoint(shared_ptr<VB::Exec>(),0);
      //      if (sequenceWidget->scene()->beginPointItem())
      //      {
      //        beginPoint.first =
      //        sequenceWidget->scene()->beginPointItem()->exec();

      seq.submit(*ds);
    }
  } else {
    if (sequenceRootNodeEditBox->text().contains("*"))
      QMessageBox::critical(this, "Invalid Data Set Nodes",
                            QString("The nodes specified by '%1' do not exist "
                                    "within the provided data set.")
                                .arg(sequenceRootNodeEditBox->text()));
    else
      QMessageBox::critical(this, "Invalid Data Set Node",
                            QString("The node specified by '%1' does not exist "
                                    "within the provided data set.")
                                .arg(sequenceRootNodeEditBox->text()));
  }
}

void MainWindow::exportSequence() {
  QString seq_filename = QFileDialog::getExistingDirectory(
      this, "Store sequence and jobs in...", QString());

  if (seq_filename != "") {
    VB::Sequence seq;
    sequenceWidget->scene()->saveSequence(seq);
    seq.sort_execs();
    seq.root_block()->def->sync_to_instance(seq.root_block());
    seq.name(sequenceNameEditBox->text().toStdString());

    VB::DataSet& widget_ds = dsWidget->dataset();
    list<VB::DataSet*> ds_list =
        widget_ds.get_children(sequenceRootNodeEditBox->text().toStdString());
    if (!ds_list.empty()) {
      for (list<VB::DataSet*>::iterator iter = ds_list.begin();
           iter != ds_list.end(); ++iter) {
        VB::DataSet* ds = *iter;
        seq.export_to_disk(*ds, seq_filename.toStdString());
        cerr << "exported!" << endl;
      }
    } else {
      if (sequenceRootNodeEditBox->text().contains("*"))
        QMessageBox::critical(this, "Invalid Data Set Nodes",
                              QString("The nodes specified by '%1' do not "
                                      "exist within the provided data set.")
                                  .arg(sequenceRootNodeEditBox->text()));
      else
        QMessageBox::critical(this, "Invalid Data Set Node",
                              QString("The node specified by '%1' does not "
                                      "exist within the provided data set.")
                                  .arg(sequenceRootNodeEditBox->text()));
    }
  }
}

void MainWindow::closeSequence() {}

void MainWindow::toggleDataVisible(bool /*vis*/) {
  //  sequenceWidget->setDataVisible(vis);
}

void MainWindow::selectMoveTool() {
  //  sequenceWidget->setDragMode(SequenceView::MOVE_MODE);
}

void MainWindow::selectDependTool() {
  //  sequenceWidget->setDragMode(SequenceView::DEPEND_MODE);
}

void MainWindow::make_disconnect_job_actions(
    QMenu* disconnect_menu, std::list<shared_ptr<VB::Exec> > connected_jobs) {
  // foreach shared_ptr<VB::Exec> j in connected_jobs
  for (list<shared_ptr<VB::Exec> >::iterator iter = connected_jobs.begin();
       iter != connected_jobs.end(); ++iter) {
    shared_ptr<VB::Exec>& j = *iter;

    QAction* act = new QAction(j->name.c_str(), this);
    act->setData(QString(j->name.c_str()));
    connect(act, SIGNAL(triggered()), this, SLOT(disconnectClickedJobJob()));
    disconnectJobActs.push_back(act);
    disconnect_menu->addAction(act);
  }
}

void MainWindow::popupJobClickedMenu() {
  JobItem* ji = dynamic_cast<JobItem*>(sender());

  if (ji) {
    shared_ptr<VB::Job> job = ji->job();

    QMenu popup(this);
    QMenu* disconnectMenu = popup.addMenu("Disconnect from...");

    if (job) {
      foreach (QAction* act, disconnectJobActs)
        delete act;
      disconnectJobActs.clear();

      popup.addAction(deleteJobAct);

      make_disconnect_job_actions(disconnectMenu, job->depends);
      //      disconnectMenu->addSeparator();
      //      make_disconnect_job_actions(disconnectMenu, job->get_sink_jobs());

      popup.exec(QCursor::pos());
    }
    return;
  }

  BlockItem* bi = dynamic_cast<BlockItem*>(sender());

  if (bi) {
    shared_ptr<VB::Block> block = bi->block();

    QMenu popup(this);

    if (block) {
      toggleDetailAct->setChecked(bi->execsShown());
      popup.addAction(toggleDetailAct);
      popup.exec(QCursor::pos());
    }
    return;
  }
}

void MainWindow::deleteClickedJob() {
  sequenceWidget->scene()->removeExecItem(
      qgraphicsitem_cast<ExecItem*>(sequenceWidget->scene()->focusItem()));
}

void MainWindow::toggleClickedBlockDetail(bool) {
  BlockItem* bi =
      qgraphicsitem_cast<BlockItem*>(sequenceWidget->scene()->focusItem());

  if (bi) {
    if (toggleDetailAct->isChecked())
      bi->showExecs();

    else
      bi->hideExecs();

    bi->calcBoundingRectSize();
    sequenceWidget->scene()->arrange();
  }
}

void MainWindow::disconnectClickedJobJob() {
  QAction* action = qobject_cast<QAction*>(sender());
  QString act_string = action->data().toString();

  if (action) {
    shared_ptr<VB::Exec> job1 =
        sequenceWidget->scene()->selectedExecItem()->exec();
    foreach (shared_ptr<VB::Exec> job2, job1->depends) {
      //    shared_ptr<VB::Exec> job2 =
      //    sequenceWidget->scene()->sequence()->get_job_by_name(act_string.toStdString());
      if (job2->name == act_string.toStdString() && job1 && job2) {
        /* We don't know which depends on which, but it's probably quicker to
         * just handle both directions then to check dependency.
         */
        job1->depends.remove(job2);
        job2->depends.remove(job1);
        updateSequenceView();
      }
    }
  }
}

void MainWindow::disconnectClickedArgRes() {}

void MainWindow::import_jobtypes_from_dir(const char* dir,
                                          JobTypeListModel* model) {
  string pat;
  QPixmap pixmap("job image.png");

  // .jobtype
  vglob vg((string)dir + "/*.jobtype");

  for (size_t i = 0; i < vg.size(); i++) {
    ifstream jt_file;
    jt_file.open(vg[i].c_str());

    if (jt_file.good()) {
      VB::JobType* temp_jt_pointer = new VB::JobType();
      // Don't worry about losing the pointer; it will insert itself
      // into the VB::Definitions::defs() list.
      temp_jt_pointer->read(jt_file);
      model->insertAt(0, temp_jt_pointer, pixmap);
    }
  }

  // .vjt
  vg.load((string)dir + "/*.vjt");

  for (size_t i = 0; i < vg.size(); i++) {
    ifstream jt_file;
    jt_file.open(vg[i].c_str());

    if (jt_file.good()) {
      VB::JobType* temp_jt_pointer = new VB::JobType();
      // Don't worry about losing the pointer; it will insert itself
      // into the VB::Definitions::defs() list.
      temp_jt_pointer->read(jt_file);
      model->insertAt(0, temp_jt_pointer, pixmap);
    }
  }
}

void MainWindow::setupWidgets() {
  QFrame* frame = new QFrame();
  QBoxLayout* frameLayout = new QHBoxLayout(frame);
  QBoxLayout* jobtypesLayout = new QVBoxLayout();
  QBoxLayout* sequenceLayout = new QVBoxLayout();
  QBoxLayout* seqPropsLayout = new QHBoxLayout();
  QBoxLayout* propertiesLayout = new QHBoxLayout();
  QBoxLayout* connectionsLayout = new QVBoxLayout();

  filterEditBox = new QLineEdit(this);
  sequenceNameEditBox = new QLineEdit(this);
  sequenceDirEditBox = new QLineEdit(this);
  sequenceRootNodeEditBox = new QLineEdit(this);
  execNameEditBox = new QLineEdit(this);
  execNameEditBox->setEnabled(false);
  execDataPathEditBox = new QLineEdit(this);
  execDefNameLabel = new QLabel(this);
  jobCommandLabel = new QLabel(this);
  jobArgumentsWidget = new QTreeWidget(this);
  jobArgumentsWidget->setColumnCount(2);
  QStringList labels1;
  labels1.append("Identifier");
  labels1.append("Value");
  jobArgumentsWidget->setHeaderLabels(labels1);
  jobArgumentsWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  jobFilesWidget = new QTreeWidget(this);
  jobFilesWidget->setColumnCount(3);
  QStringList labels2;
  labels2.append("Identifier");
  labels2.append("Filename");
  labels2.append("Type");
  jobFilesWidget->setHeaderLabels(labels2);
  jobFilesWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  jobsList = new QTreeView(this);
  jobsList->setColumnHidden(2, true);
  jobsList->setColumnHidden(3, true);
  jobsList->setDragEnabled(true);
  //    jobsList->setViewMode(QListView::IconMode);
  //    jobsList->setIconSize(QSize(60, 60));
  //    jobsList->setGridSize(QSize(80, 80));
  //    jobsList->setSpacing(10);
  //    jobsList->setMovement(QListView::Free);
  jobsList->setAcceptDrops(false);
  jobsList->setDropIndicatorShown(true);

  JobTypeListModel* model = new JobTypeListModel(this);
  jobsList->setModel(model);

  QPixmap pixmap("job image.png");

  vbp.init();
  vbp.read_jobtypes();
  VB::Definitions::Import_JobType_Folder();

  foreach (VB::JobType* jt, VB::Definitions::jts()) {
    model->insertAt(0, jt, pixmap);
  }

  import_jobtypes_from_dir("data", model);

  SequenceScene* scene = new SequenceScene();
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);

  sequenceWidget = new SequenceView(this);
  sequenceWidget->setAcceptDrops(true);
  sequenceWidget->showDataText(false);
  sequenceWidget->setScene(scene);
  sequenceWidget->setRenderHint(QPainter::Antialiasing);

  frameLayout->addLayout(jobtypesLayout, 1);
  jobtypesLayout->addWidget(jobsList);
  jobtypesLayout->addWidget(new QLabel("Filter by keyword:", this));
  jobtypesLayout->addWidget(filterEditBox);
  frameLayout->addLayout(sequenceLayout, 4);

  seqPropsLayout->addWidget(new QLabel("Sequence Name:", this));
  seqPropsLayout->addWidget(sequenceNameEditBox);
  seqPropsLayout->addWidget(new QLabel("Working Directoy:", this));
  seqPropsLayout->addWidget(sequenceDirEditBox);
  seqPropsLayout->addWidget(new QLabel("Dataset Root Node Path:", this));
  seqPropsLayout->addWidget(sequenceRootNodeEditBox);
  sequenceLayout->addLayout(seqPropsLayout);
  sequenceLayout->addWidget(sequenceWidget);
  sequenceLayout->addLayout(propertiesLayout);
  propertiesLayout->addLayout(connectionsLayout);
  execDefNameLabel = new QLabel("Job Name:", this);
  connectionsLayout->addWidget(execDefNameLabel);
  connectionsLayout->addWidget(execNameEditBox);
  connectionsLayout->addWidget(new QLabel("Dataset Node Path:", this));
  connectionsLayout->addWidget(execDataPathEditBox);
  //    connectionsLayout->addWidget(jobCommandLabel);

  dsWidget = new QtVB::DataSetWidget(this);
  QDockWidget* dockWidget = new QDockWidget("Data Set", this);
  dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dockWidget->setWidget(dsWidget);
  addDockWidget(Qt::RightDockWidgetArea, dockWidget);

  QVBoxLayout* vb1 = new QVBoxLayout();
  vb1->addWidget(new QLabel("Arguments:", this));
  vb1->addWidget(jobArgumentsWidget);
  QVBoxLayout* vb2 = new QVBoxLayout();
  vb2->addWidget(new QLabel("Files:", this));
  vb2->addWidget(jobFilesWidget);
  propertiesLayout->addLayout(vb1);
  propertiesLayout->addLayout(vb2);
  setCentralWidget(frame);

  execNameValidator = new ExecNameValidator(sequenceWidget, this);
  execNameEditBox->setValidator(execNameValidator);
}
