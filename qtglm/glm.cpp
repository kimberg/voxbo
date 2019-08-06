
// glm.cpp
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
// original version written by Dongbo Hu

#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <QCloseEvent>

using namespace std;

#include <q3buttongroup.h>
#include <q3filedialog.h>
#include <q3grid.h>
#include <q3hbox.h>
#include <q3hgroupbox.h>
#include <q3listbox.h>
#include <q3listview.h>
#include <q3valuelist.h>
#include <q3vbox.h>
#include <q3vgroupbox.h>
#include <qapplication.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <Q3VButtonGroup>
#include "glm.h"
#include "runseq.h"

extern VBPrefs vbp;

/* glm constructor */
glm::glm(QWidget *parent, const char *name) : QWidget(parent, name) {
  TR = 0, totalReps = 0;
  endFlag = false;
  Q3GridLayout *mainGrid = new Q3GridLayout(this, 2, 1, 5, 5);
  tabSection = new QTabWidget(this);

  setElementPath();
  setupTab1();
  setupTab2();
  setupTab3();
  setupTab4();

  mainGrid->addWidget(tabSection, 0, 0);
  buttons = new Q3HBoxLayout(mainGrid);
  setupButtons();
}

/*************************************************************************************
 * Tab1 setup *
 *************************************************************************************/
void glm::setupTab1() {
  tab1 = new Q3VBox(tabSection);
  tab1->setMargin(20);
  tab1->setSpacing(10);
  QFileInfo fileinfo;
  Q3VGroupBox *tab1Box =
      new Q3VGroupBox("Please set up your analysis folder:", tab1);
  tab1Box->setLineWidth(0);

  Q3HBox *pathRow = new Q3HBox(tab1Box);
  pathRow->setSpacing(10);
  pathEditor = new QLineEdit(fileinfo.absFilePath(), pathRow);

  QObject::connect(pathEditor, SIGNAL(textChanged(const QString &)), this,
                   SLOT(tab1_pathChanged()));
  QObject::connect(pathEditor, SIGNAL(returnPressed()), this,
                   SLOT(enableTab2()));

  pathText = pathEditor->text();
  folderInfo = QFileInfo(pathText);
  QPushButton *browseButt = new QPushButton("&Browse...", pathRow);
  QObject::connect(browseButt, SIGNAL(clicked()), this, SLOT(tab1_browse()));

  tab1Box->addSpace(5);
  finalName = new QLabel(
      "Analysis folder already exists: <b>" + pathEditor->text() + "</b>",
      tab1Box);

  tab1Box->addSpace(20);
  Q3HBox *seqBox = new Q3HBox(tab1Box);
  (void)new QLabel("Sequence Name: ", seqBox);
  seqEditor = new QLineEdit("Make GLM matrices", seqBox);

  tab1Box->addSpace(20);
  Q3HBox *priorityBox = new Q3HBox(tab1Box);
  QLabel *priLab = new QLabel("Priority: ", priorityBox);
  priLab->setFixedWidth(50);
  priCombo = new QComboBox(priorityBox);

  QLabel *blankLab = new QLabel("", priorityBox);
  priorityBox->setStretchFactor(priCombo, 1);
  priorityBox->setStretchFactor(blankLab, 5);

  // Totally there are four priority levels
  priCombo->insertItem("1 (overnight)");
  priCombo->insertItem("2 (low priority)");
  priCombo->insertItem("3 (normal)");
  priCombo->insertItem("4 (high)");
  priCombo->insertItem("5 (emergency)");
  priCombo->setCurrentItem(1);  // Default is "Standard"

  tab1Box->addSpace(10);
  auditCheck = new QCheckBox("Audit GLM", tab1Box);
  auditCheck->setChecked(true);

  Q3HBox *emailBox = new Q3HBox(tab1Box);
  emailCheck = new QCheckBox("Email me when done: ", emailBox);
  emailCheck->setChecked(true);
  emailEditor = new QLineEdit(QString(vbp.email.c_str()), emailBox);
  connect(emailCheck, SIGNAL(toggled(bool)), emailEditor,
          SLOT(setEnabled(bool)));

  // Manually set number of pieces for matrix operations
  Q3HBox *pieceBox = new Q3HBox(tab1Box);
  pieceCheck = new QCheckBox(
      "Manually set number of pieces for matrix operations: ", pieceBox);
  pieceCheck->setChecked(false);
  pieceEditor = new QLineEdit(pieceBox);
  pieceEditor->setEnabled(false);
  // pieceEditor will only accept integers between 1 and 1000
  QValidator *validator = new QIntValidator(1, 1000, this);
  pieceEditor->setValidator(validator);
  connect(pieceCheck, SIGNAL(toggled(bool)), pieceEditor,
          SLOT(setEnabled(bool)));

  tabSection->addTab(tab1, "General Options");
}

/* Slot for "Browse" button */
void glm::tab1_browse() {
  Q3FileDialog *fileOpen = new Q3FileDialog(this);
  QString fn = fileOpen->getExistingDirectory(QString::null, this, "");
  if (!fn.isEmpty()) pathEditor->setText(fn);
}

/* Slot to take care of text change in pathText line editor */
void glm::tab1_pathChanged() {
  pathText = pathEditor->text();
  if (pathText.isEmpty()) {
    finalName->setText("No analysis folder defined yet!");
    return;
  }

  // Remove "/" at the end
  if (pathText.right(1) == "/" && pathText.length() > 1)
    pathText = pathText.left(pathText.length() - 1);

  folderInfo = QFileInfo(pathText);
  if (folderInfo.exists() && folderInfo.isDir())
    finalName->setText("Analysis folder already exists: <b>" + pathText +
                       "</b>");
  else if (folderInfo.exists())
    finalName->setText("<b>" + pathText +
                       "</b> exists, but is NOT a directory!");
  else
    finalName->setText("New analysis folder: <b>" + pathText + "</b>");
}

/* This function checks the whole glm interface to make sure all parameters
 * are available before glm submission. */
bool glm::chkSubmit() {
  // Check parameters in tab1
  if (!chkTab1()) return false;
  // Check the availability of tes file(s)
  if (!tab2_lb2->count()) {
    QMessageBox::critical(0, "Error!", "No tes files selected.");
    return false;
  }
  // Is G matrix available?
  if (!covList.size() || !tab3->covView->childCount()) {
    QMessageBox::critical(0, "Error!", "No covariates defined.");
    return false;
  }
  // If condition function is defined, are condition keys available?
  if (condRef.length() && !condKey.size()) {
    QMessageBox::critical(0, "Error!", "Condition keys not found.");
    return false;
  }
  // If filter file is defined, is kernel TR valid?
  if (filterFile.size()) {
    bool ok;
    kTR = kernelTR->text().toUInt(&ok);
    if (!ok || kTR == 0) {
      QMessageBox::critical(0, "Error", "Invalid Kernel TR");
      return false;
    }
  }
  // Does TR match that in G matrix?
  // This check may seem to be redundant. But it is needed when
  // user changes tes file selection AFTER G matrix has been designed.
  if (TR && TR != g_TR) {
    QMessageBox::critical(0, "Error",
                          "TR in tes files doesn't match TR in G matrix");
    return false;
  }
  // Does number of points in tes file match that in G matrix?
  // This check may seem to be redundant. But it is needed when
  // user changes tes file selection AFTER G matrix has been designed.
  if (totalReps != g_totalReps) {
    QMessageBox::critical(0, "Error",
                          "Number of images in tes files doesn't match the "
                          "number of imagess in G matrix");
    return false;
  }

  // If "set number of pieces for matrix operations" is checked in "General
  // options" interface, is the input value valid?
  if (pieceCheck->isChecked()) {
    if (pieceEditor->text().isEmpty()) {
      QMessageBox::critical(0, "Error",
                            "Number of pieces in \"General Options\" is blank");
      return false;
    }
    uint value = pieceEditor->text().toUInt();
    if (value == 0 || value > 1000) {
      QMessageBox::critical(0, "Error",
                            "Number of pieces in \"General Options\" must be "
                            "an integer between 1 and 1000");
      return false;
    }
  }
  return true;
}

/* This function checks analysis folder's validity */
bool glm::chkAnalysisDIR() {
  // Never ever use "/" as the analysis folder, even if the user is root!
  if (pathText == "/") {
    QMessageBox::critical(0, "Error",
                          "Please don't use / as your analysis folder.");
    return false;
  }

  // If analysis folder exists and is a directory, make sure it is writable and
  // executble
  if (folderInfo.exists() && folderInfo.isDir()) {
    if (!folderInfo.isWritable()) {
      QMessageBox::critical(
          0, "Error", "Analysis folder exists but not writable: " + pathText);
      return false;
    }
    if (!folderInfo.isExecutable()) {
      QMessageBox::critical(
          0, "Error", "Analysis folder exists but not executable: " + pathText);
      return false;
    }
    return true;
  }

  // If analysis folder exists but is not a directory, give error message
  if (folderInfo.exists()) {
    QMessageBox::critical(
        0, "Error", "<b>" + pathText + "</b> exists, but is NOT a directory!");
    return false;
  }

  // If analysis folder is new, make sure its parent directory is valid
  QString dirText = folderInfo.dirPath(true);
  QFileInfo pathInfo = QFileInfo(dirText);
  // Parent directory not exists
  if (!pathInfo.exists()) {
    QMessageBox::critical(0, "Error",
                          "Parent directory not exists: " + dirText);
    return false;
  }
  // Parient directory is not really a directory
  if (!pathInfo.isDir()) {
    QMessageBox::critical(0, "Error",
                          "File exists but not a directory: " + dirText);
    return false;
  }
  // Parient directory not writable
  if (!pathInfo.isWritable()) {
    QMessageBox::critical(0, "Error",
                          "Parent directory not writable: " + dirText);
    return false;
  }
  // Parient directory not executable
  if (!pathInfo.isExecutable()) {
    QMessageBox::critical(0, "Error",
                          "Parent directory not executable: " + dirText);
    return false;
  }

  return true;
}

/* This function checks the parameters in tab1:
 * (1) Is analysis folder valid?
 * (2) Is sequence name valid?
 * (3) If email check is selected, is email address valid? */
bool glm::chkTab1() {
  if (!chkAnalysisDIR()) return false;

  if (seqEditor->text().isEmpty()) {
    QMessageBox::critical(0, "Error", "Sequence name is blank.");
    return false;
  }

  if (emailCheck->isChecked()) {
    if (emailEditor->text().isEmpty()) {
      QMessageBox::critical(0, "Error", "Email address is blank.");
      return false;
    }
    QString email = emailEditor->text();
    if (email.contains('@') != 1 || email.contains(32) ||
        email.right(1) == "@") {
      QMessageBox::critical(0, "Error", "Invalid email address: " + email);
      return false;
    }
  }
  return true;
}

/* This function will check whether the analysis folder typed in is valid
 * or not. If yes, enable tab2. If not, pop out some error messages. */
void glm::enableTab2() {
  if (!chkTab1()) return;

  fileName = folderInfo.fileName();
  // tabSection->setTabEnabled(tab2, true);
  tabSection->setCurrentPage(1);
}

/*************************************************************************************
 * Tab2 Setup *
 *************************************************************************************/
void glm::setupTab2() {
  tab2 = new Q3VBox(tabSection);
  tab2->setMargin(20);
  Q3ButtonGroup *part21 =
      new Q3ButtonGroup(1, Qt::Horizontal, "Please select scans:", tab2);

  Q3HBox *cwd = new Q3HBox(part21);
  cwd->setMargin(5);
  QFileInfo fileinfo;
  tab2_dirName = fileinfo.absFilePath();

  /* Change this section from a simple label into label + line editor */
  (void)new QLabel("Your current working directory is: ", cwd);
  dirEditor = new QLineEdit(tab2_dirName, cwd);
  QObject::connect(dirEditor, SIGNAL(textChanged(const QString &)), this,
                   SLOT(tab2_listChanged()));

  QPushButton *browse_tab2 = new QPushButton("Browse...", cwd);
  cwd->setStretchFactor(dirEditor, 2);
  connect(browse_tab2, SIGNAL(clicked()), this, SLOT(tab2Browse()));

  /* Main layout of tab2, which includes three vboxes.
   * The left box lists the files, the middle box includes the buttons.
   * The right side listbox shows the file(s) selected. */
  Q3HBox *tab2_main = new Q3HBox(part21);
  tab2_main->setMargin(5);
  tab2_main->setSpacing(5);

  Q3VBox *tab2_vbox1 = new Q3VBox(tab2_main);
  tab2_main->setStretchFactor(tab2_vbox1, 3);

  Q3VBox *tab2_vbox2 = new Q3VBox(tab2_main);
  tab2_main->setStretchFactor(tab2_vbox2, 1);

  Q3VBox *tab2_vbox3 = new Q3VBox(tab2_main);
  tab2_main->setStretchFactor(tab2_vbox3, 3);

  // Set up the filter
  Q3HBox *hbox_filter = new Q3HBox(tab2_vbox1);
  (void)new QLabel("Filter: ", hbox_filter);
  tab2Filter = new QLineEdit("*.tes", hbox_filter);
  tab2_filterText = tab2Filter->text();
  hbox_filter->setSpacing(2);

  // Create the listbox on the left side
  tab2_lb1 = new Q3ListBox(tab2_vbox1);
  tab2_lb1->setSelectionMode(Q3ListBox::Multi);
  connect(tab2_lb1, SIGNAL(selected(int)), this, SLOT(lb1_select(int)));
  tab2_vbox1->setSpacing(5);

  // List *.tes files in current directory
  tab2_listChanged();

  // Change the filter whose text change will list new files
  connect(tab2Filter, SIGNAL(textChanged(const QString &)), this,
          SLOT(tab2_listChanged()));

  // In the middle, create some pushbuttons for left <-> right file transfer
  tab2_vbox2->setSpacing(5);
  (void)new QLabel("", tab2_vbox2);  // A fake label for alignment purpose

  // Buttons and the related actions if you dare to press them
  QPushButton *left2right = new QPushButton(" -> ", tab2_vbox2);
  connect(left2right, SIGNAL(clicked()), this, SLOT(tab2_slotLeft2Right()));

  QPushButton *left2right_all = new QPushButton("ALL->", tab2_vbox2);
  connect(left2right_all, SIGNAL(clicked()), this,
          SLOT(tab2_slotLeft2Right_all()));

  // Should it be called "Remove" or "<-"?
  QPushButton *right2left = new QPushButton("<-", tab2_vbox2);
  connect(right2left, SIGNAL(clicked()), this, SLOT(tab2_slotRight2Left()));

  QPushButton *right2left_all = new QPushButton("<-ALL", tab2_vbox2);
  connect(right2left_all, SIGNAL(clicked()), this,
          SLOT(tab2_slotRight2Left_all()));

  QPushButton *button_done = new QPushButton("DONE", tab2_vbox2);
  connect(button_done, SIGNAL(clicked()), this, SLOT(enableTab3()));

  (void)new QLabel("", tab2_vbox2);  // Another fake label for alignment purpose

  // Right Side of the layout
  QLabel *fileSelected = new QLabel("File Selected:", tab2_vbox3);
  fileSelected->setAlignment(Qt::AlignCenter);

  // create another multi-selection ListBox on the right side
  tab2_lb2 = new Q3ListBox(tab2_vbox3);
  tab2_lb2->setSelectionMode(Q3ListBox::Multi);
  connect(tab2_lb2, SIGNAL(selected(int)), this, SLOT(lb2_select(int)));
  tab2_vbox3->setSpacing(15);

  tabSection->addTab(tab2, "Data Selection");
  // tabSection->setTabEnabled(tab2, false); // Disable tab2 if tab1 isn't done
}

/* Slot for "browse" button in tab2 */
void glm::tab2Browse() {
  Q3FileDialog *dirOpen = new Q3FileDialog(this);
  tab2_dirName = dirOpen->getExistingDirectory(QString::null, this, "");
  if (tab2_dirName.isEmpty()) return;

  dirEditor->setText(tab2_dirName);
  tab2_listChanged();
}

/* tab2_listChanged() is copied from Tom's fileView.cpp.
 * With glob_t it the filter supports multi-layer text format.
 * This slot will update the file list in left side list box.
 * It listens to the signals from path line editor change,
 * filter line editor change and the browse push button */
void glm::tab2_listChanged() {
  tab2_dirName = dirEditor->text();
  tab2_filterText = tab2Filter->text();
  tab2_lb1->clear();

  struct stat st;

  vglob vg((string)tab2_dirName.ascii() + "/" + tab2_filterText.ascii());

  for (size_t i = 0; i < vg.size(); i++) {
    if (stat(vg[i].c_str(), &st)) continue;
    if (!(S_ISREG(st.st_mode))) continue;
    tab2_lb1->insertItem(vg[i].c_str());
  }
}

/* This is the slot that responds to the mouse double click action in the left
 * listbox. It will move the selected item from left to right */
void glm::lb1_select(int selection) {
  Q3ListBoxItem *item = tab2_lb1->item(selection);
  if (!chkTesFile(item->text())) return;

  int answer = tesChanged();
  if (answer < 2) {
    tab2_lb2->insertItem(item->text());
    totalReps += getTesImgNo(item->text());
    tab2_lb1->removeItem(selection);
  }

  if (answer == 1) clearG();
}

/************************************************************************
 * tab2_slotLeft2Right copies all selected items in the first ListBox
 * into the second ListBox.
 ************************************************************************/
void glm::tab2_slotLeft2Right() {
  for (unsigned int i = 0; i < tab2_lb1->count(); i++) {
    Q3ListBoxItem *item = tab2_lb1->item(i);
    if (item->isSelected() && !chkTesFile(item->text())) return;
  }

  int answer = tesChanged();
  if (answer < 2) {
    // Go through each element in the listbox, if selected, insert it in the
    // right listbox
    for (unsigned int i = 0; i < tab2_lb1->count(); i++) {
      Q3ListBoxItem *item = tab2_lb1->item(i);
      if (item->isSelected()) {
        tab2_lb2->insertItem(item->text());
        totalReps += getTesImgNo(item->text());
        tab2_lb1->removeItem(i);
        i--;  // Important!! Without which multiple selection removal won't
              // work!
      }
    }
  }

  if (answer == 1) clearG();
}

// This slot will transfer all the items in the left to right side
void glm::tab2_slotLeft2Right_all() {
  for (unsigned int i = 0; i < tab2_lb1->count(); i++) {
    Q3ListBoxItem *item = tab2_lb1->item(i);
    if (!chkTesFile(item->text())) return;
  }

  int answer = tesChanged();
  if (answer < 2) {
    // Go through all items of the first ListBox
    for (unsigned int i = 0; i < tab2_lb1->count(); i++) {
      Q3ListBoxItem *item = tab2_lb1->item(i);
      tab2_lb2->insertItem(item->text());
      totalReps += getTesImgNo(item->text());
      tab2_lb1->removeItem(i);
      i--;  // Important!! Without which multiple selection removal won't work!
    }
  }

  if (answer == 1) clearG();
}

/* This is the slot that responds to  the mouse double click action in the right
 * listbox. It will move the selected item from right to left */
void glm::lb2_select(int selection) {
  int answer = tesChanged();
  if (answer < 2) {
    Q3ListBoxItem *item = tab2_lb2->item(selection);
    tab2_lb1->insertItem(item->text());
    totalReps -= getTesImgNo(item->text());
    tab2_lb2->removeItem(selection);
    updateTR();
  }

  if (answer == 1) clearG();
}

// Transfer the selected item(s) from right to left side
void glm::tab2_slotRight2Left() {
  int answer = tesChanged();
  if (answer < 2) {
    // This loop merges the insert and remove process together, better than
    // tab2_slotLeft2Right()
    for (unsigned int i = 0; i < tab2_lb2->count(); i++) {
      Q3ListBoxItem *item = tab2_lb2->item(i);
      if (item->isSelected()) {
        tab2_lb1->insertItem(item->text());
        totalReps -= getTesImgNo(item->text());
        tab2_lb2->removeItem(i);
        i--;  // Important!!! Without which multiple selection removal won't
              // work!
      }
    }
    updateTR();
  }

  if (answer == 1) clearG();
}

/* Transfer all the items in the right side to left side */
void glm::tab2_slotRight2Left_all() {
  int answer = tesChanged();
  if (answer < 2) {
    // This slot uses one loop to insert the item in the left side and remove it
    // from the right side
    for (unsigned int i = 0; i < tab2_lb2->count(); i++) {
      Q3ListBoxItem *item = tab2_lb2->item(i);
      tab2_lb1->insertItem(item->text());
      tab2_lb2->removeItem(i);
      i--;
    }
    TR = 0, totalReps = 0;
  }

  if (answer == 1) clearG();
}

/* This function is called when tes file list is changed. It checks whether G
 * matrix has already been designed. If yes, it will ask user if he/she wants to
 * keep the original covariates or clear them and start G design from scratch.
 */
int glm::tesChanged() {
  if (!covList.size()) return 0;

  int answer =
      QMessageBox::warning(0, "Warning!",
                           "It seems that you have already designed G matrix. \
<P>Changeing your selected data may make your G matrix inconsistent with the tes files. \
<P>Do you want to keep your original G matrix or clear it up ?",
                           "Keep it and continue", "Clear it up and continue",
                           "Cancel this operation", 0, 2);
  return answer;
}

/* This function checks the input tes file to make sure its TR matches the TR of
 * selected
 * tes files. If no tes file selected in right side list box, set TR to the
 * input file's TR */
bool glm::chkTesFile(QString tesName) {
  // Is it a valid file?
  if (!chkFileStat(tesName.ascii(), true)) return false;

  int inputTR = getTesTR(tesName);
  // Is the header readable?
  if (inputTR == -1) {
    QMessageBox::critical(0, "Tes file Reading Error",
                          "Invalid header in tes file: " + tesName);
    return false;
  }
  // Does its header include TR?
  //   if (!inputTR) {
  //     QMessageBox::critical(0, "Tes file Reading Error", "Invalid TR in tes
  //     file: " + tesName); return false;
  //   }

  // If tes file doesn't have a valid TR, still accept it
  if (!inputTR) return true;

  // Is this the first tes file selected?
  if (!TR) {
    TR = inputTR;
    return true;
  }
  // TR in this file is different
  if (inputTR != TR) {
    QMessageBox::critical(
        0, "Different TR Found",
        tesName + "has different TR: " + QString::number(inputTR));
    return false;
  }
  // Silently accept good TR
  return true;
}

/* This function returns a certain tes file's TR from header */
int glm::getTesTR(QString tesName) {
  string tmpName(tesName.ascii());
  Tes myTes;
  int tesStat = myTes.ReadHeader(tmpName);
  if (tesStat) return -1;
  int tesTR = myTes.voxsize[3];
  return tesTR;
}

/* This function goes through each tes file in right listbox and update TR */
void glm::updateTR() {
  TR = 0;
  for (unsigned i = 0; i < tab2_lb2->count(); i++) {
    string tmpName(tab2_lb2->item(i)->text().ascii());
    Tes myTes;
    myTes.ReadHeader(tmpName);
    int tesTR = myTes.voxsize[3];
    if (tesTR > FLT_MIN) TR = tesTR;
  }
}

/* This function returns a certain tes file's TR from header */
int glm::getTesImgNo(QString tesName) {
  string tmpName(tesName.ascii());
  Tes myTes;
  myTes.ReadHeader(tmpName);
  return myTes.dimt;
}

/* Slot for "DONE" button: When files are selected, click the button will enable
 * tab3 But if no file selected, an error message will pop out. */
void glm::enableTab3() {
  if (!tab2_lb2->count()) {
    QMessageBox::information(0, "Error!", "No TES files selected yet.");
    return;
  }

  // tabSection->setTabEnabled(tab3, true);
  tabSection->setCurrentPage(2);
}

/**************************************************************************
 * Tab 3 setup
 **************************************************************************/
void glm::setupTab3() {
  gUpdate = false;
  meanAll = false;
  pregStat = true;
  tab3 = new glm_tab3();
  tab3_TR = 0, tmpResolve = 100;
  g_TR = 0, g_totalReps = 0;
  setupTab3_view();
  tabSection->addTab(tab3, "Design Matrix");
  // tabSection->setTabEnabled(tab3, false);
}

/* This function initializes the list view in tab3 */
void glm::setupTab3_view() {
  tab3->covView->clear();
  tab3->covView->setColumnText(0, "Name");
  tab3->covView->addColumn("Type");
  tab3->covView->setColumnAlignment(1, Qt::AlignHCenter);
  tab3->covView->addColumn("ID");
  tab3->covView->setColumnAlignment(2, Qt::AlignHCenter);
  tab3->covView->addColumn("hidden", 0);
  tab3->covView->setRootIsDecorated(true);
  tab3->covView->setSortColumn(-1);

  connect(tab3->editButt, SIGNAL(clicked()), this, SLOT(tab3_edit()));
  connect(tab3->loadButt, SIGNAL(clicked()), this, SLOT(tab3_load()));
  connect(tab3->clearButt, SIGNAL(clicked()), this, SLOT(tab3_clear()));
  connect(tab3->blockButt, SIGNAL(clicked()), this, SLOT(tab3_blockUI()));
  connect(tab3->pairButt, SIGNAL(clicked()), this, SLOT(tab3_pairUI()));
  connect(tab3->interButt, SIGNAL(clicked()), this, SLOT(tab3_inter()));
}

/* Slot for "edit" button on tab3, it launches Gdw fro G matrix editing */
void glm::tab3_edit() {
  // check TR
  if (!tab3_TR) {
    QMessageBox::critical(0, "Error!", "TR not found");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!", "Number of time points not found");
    return;
  }

  /* Since G file is not saved by gdw now, it is not that important to pass
   * gFileName to gdw now. The only use is to make gdw's caption look nice. */
  if (pathText.right(1) == "/")
    gFileName = pathText + fileName;
  else
    gFileName = pathText + "/" + fileName;

  Gdw *gdi = new Gdw(totalReps, tab3_TR, 1, gFileName);
  gdi->setTesList(tab2_lb2);
  gdi->calcNoiseModel();
  gdi->show();

  // Copy glm's treeview into gdw's tree view
  cpView(tab3->covView, gdi->varListView);
  // Copy glm's covariate list into gdw's covList
  gdi->cpGlmList(covList);
  // Copy tmpResolve too, because it might be copied from a G matrix file,
  // instead of default value of 100ms
  gdi->cpTmpResolve(tmpResolve);
  // Copy condition function information
  if (condKey.size()) gdi->cpGlmCondFxn(condRef, condKey);
  // update interceptID in gdw
  gdi->updateInterceptID();
  // If some covariates are copied from glm, gdw's status should be "update"
  if (covList.size()) gdi->setGUpdateFlag(true);

  this->setDisabled(true);
  connect(gdi, SIGNAL(cancelSignal(bool)), this, SLOT(setEnabled(bool)));
  connect(gdi,
          SIGNAL(doneSignal(vector<VB_Vector *>, Q3ListView *, int, QString,
                            tokenlist, bool)),
          this,
          SLOT(gdwDone(vector<VB_Vector *>, Q3ListView *, int, QString,
                       tokenlist, bool)));
}

/* Slot for "load" button on tab3, it loads an existing G matrix
 *  and show the covariates in the list view */
void glm::tab3_load() {
  // check TR
  if (!tab3_TR) {
    QMessageBox::critical(0, "Error!", "TR not found");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!", "Number of time points not found");
    return;
  }

  clearG();
  QString loadedFilename = Q3FileDialog::getOpenFileName(
      QString::null, "G matrix files (*.G)", this, "Load a G matrix file",
      "Choose a G Matrix file to load");
  if (!loadedFilename.isEmpty()) {
    const string s1(loadedFilename.ascii());
    tab3_readG(s1);
  }
}

/* Read an input G matrix file, add covariate information to the tree view in
 * tab3 */
void glm::tab3_readG(const string inputName) {
  if (!chkG(inputName)) return;

  gMat = myGInfo.gMatrix;
  if (myGInfo.sampling) tmpResolve = myGInfo.sampling;

  if (myGInfo.condStat) {
    condRef = QString(myGInfo.condfxn.c_str());
    condKey = tokenlist(myGInfo.condKey);
  }

  if (pregStat) pregStat = chkPreG(inputName);
  buildCovList(pregStat);
  buildTree();
  mainNextButton->setEnabled(true);
}

/* buildCovList() reads each column of gMat or pregMat (if available) into
 * covList */
void glm::buildCovList(bool pregStat) {
  VB_Vector tmpVec;
  int upRatio = tab3_TR / tmpResolve;
  int vecSize;
  for (uint32 i = 0; i < gMat.n; i++) {
    // If preG is valid, use it instead
    if (pregStat) {
      tmpVec = pregMat.GetColumn(i);
      vecSize = tmpVec.size();
      VB_Vector *newVector = new VB_Vector(tmpVec);
      // record intercept index
      if (newVector->getVariance() <= 1e-15) interceptID.push_back(i);
      covList.push_back(newVector);
    }
    // Otherwise use G file
    else {
      tmpVec = gMat.GetColumn(i);
      VB_Vector *downVector = new VB_Vector(tmpVec);
      VB_Vector *newVector = upSampling(downVector, upRatio);
      if (newVector->getVariance() <= 1e-15) interceptID.push_back(i);
      covList.push_back(newVector);
      delete downVector;
    }
  }
  g_TR = tab3_TR, g_totalReps = totalReps;
  gUpdate = true;
}

/* buildTree() builds the covView tree based on the parameter lines in G matrix
 * file */
void glm::buildTree() {
  QString nameStr, sectionStr, typeStr;
  QStringList qList;
  for (int i = 0; i < (int)myGInfo.nameList.size(); i++) {
    nameStr = QString(myGInfo.nameList[i].c_str());
    typeStr = QString(myGInfo.typeList[i].c_str());
    QStringList qList = QStringList::split("->", nameStr);
    Q3ListViewItem *parent = tab3->covView->firstChild();
    for (int j = 0; j < qList.size(); j++) {
      sectionStr = *qList.at(j);
      // Covariate is a direct child of covView
      if (qList.size() == 1) {
        (void)new Q3ListViewItem(tab3->covView, getLastChild(tab3->covView),
                                 sectionStr, typeStr, QString::number(i));
        break;
      }
      // Covariate belongs to a certain group
      if (j == qList.size() - 1) {
        (void)new Q3ListViewItem(parent, getLastChild(parent), sectionStr,
                                 typeStr, QString::number(i));
        break;
      }
      // Create the covariate's first layer group item (if not available)
      if (j == 0) {
        if (!searchDepth0(sectionStr)) {
          parent = new Q3ListViewItem(tab3->covView,
                                      getLastChild(tab3->covView), sectionStr);
          parent->setOpen(true);
        } else
          parent = searchDepth0(sectionStr);
      }
      // Create group items after the first layer
      else if (!findGrp(parent, sectionStr)) {
        parent = new Q3ListViewItem(parent, getLastChild(parent), sectionStr);
        parent->setOpen(true);
      } else
        parent = findGrp(parent, sectionStr);
    }
  }

  // label intercept covariate(s) in hidden column
  for (unsigned k = 0; k < interceptID.size(); k++) {
    QString tmpStr = QString::number(interceptID[k]);
    tab3->covView->findItem(tmpStr, 2)->setText(3, "intercept");
  }
}

/* searchDepth0() searches the direct child(ren) of covView and returns the
 * QListViewtem
 * that is a group item and name matches the input QString. Returns 0 if name
 * not found */
Q3ListViewItem *glm::searchDepth0(QString grpName) {
  Q3ListViewItem *child = tab3->covView->firstChild();
  while (child) {
    if (child->text(2).isEmpty() && child->text(0) == grpName) return child;
    child = child->nextSibling();
  }
  return 0;
}

/* chkG() makes sure the input G file is valid */
bool glm::chkG(string inputName) {
  // Only accept *.G file
  int strLen = inputName.length();
  if (inputName.substr(strLen - 2, 2) != ".G") {
    printf("[Error] Input filename not in *.G format: %s\n", inputName.c_str());
    return false;
  }

  myGInfo = gHeaderInfo();
  if (!myGInfo.read(inputName, false)) return false;

  if (!cmpTR(myGInfo.TR)) return false;
  if (!cmpTotalReps(myGInfo.rowNum)) return false;

  return true;
}

/* Compare TR in G amtrix with TR defined on the interface */
bool glm::cmpTR(int headerTR) {
  // When no TR in G matrix header
  if (headerTR == -1) {
    QMessageBox::warning(0, "Warning",
                         "No TR information available in G matrix file. \
<br>The original value will be used.");
  }
  // If there is no TR in tes files, use the TR in header
  else if (!TR) {
    tab3_TR = headerTR;
    // tab3->TR->setText("TR: " + QString::number(tab3_TR) + "ms, ");
  } else if (TR != headerTR) {
    switch (QMessageBox::warning(
        0, "Warning!",
        QString("TR in G matrix file header (%1) is different \
from the original value (%2). The original value will be used. <br>Do you want to continue?")
            .arg(headerTR)
            .arg(TR),
        "Yes", "No", 0)) {
      case 0:
        pregStat = false;
        return true;
      case 1:  // Terminate this function for "No"
        return false;
    }
  }

  return true;
}

/* Compare totalReps on the interface with totalReps in G matrix header */
bool glm::cmpTotalReps(int rowNum) {
  // Inconsistency in combo mode: error message
  if (totalReps != rowNum) {
    QMessageBox::critical(
        0, "Error!",
        QString("The number of rows (%1) in G matrix not match \
the original number of time points (%2)")
            .arg(rowNum)
            .arg(totalReps));
    return false;
  }

  return true;
}

/* chkPreG() makes sure the preG file exists and in valid format */
bool glm::chkPreG(string inputName) {
  string pregName = inputName;
  int dotPost = pregName.rfind(".");
  pregName.erase(dotPost);
  pregName.append(".preG");

  if (!vb_fileexists(pregName)) {
    printf("[E] couldn't read preG file  %s\n", pregName.c_str());
    return false;
  }

  if (pregMat.ReadHeader(pregName) || pregMat.ReadFile(pregName) ||
      !pregMat.m || !pregMat.n) {
    printf("preG file is ignored because it is not valid G matrix file: %s\n",
           pregName.c_str());
    return false;
  }

  gHeaderInfo myPreGInfo = gHeaderInfo(pregMat);
  if (!myPreGInfo.chkInfo(true)) return false;

  return cmpG2preG(myGInfo, myPreGInfo);
}

/* This slot clears up the tree view in tab3 */
void glm::tab3_clear() {
  clearG();
  mainSubmitButton->setDisabled(true);
  mainNextButton->setDisabled(true);
}

/* This function is written to initialize G matrix parameters that are specific
 * in tab3 */
void glm::clearG() {
  tab3->covView->clear();
  covList.clear();
  tmpResolve = 100;
  meanAll = false;
  g_TR = 0, g_totalReps = 0;
  gUpdate = true;
  interceptID.clear();
  condKey.clear();
  condRef = QString::null;
  pregStat = true;
}

/* This slot takes care of radiobutton clicks for each canned model */
void glm::tab3_model(int modelID) {
  if (modelID == 0)
    tab3_blockUI();
  else if (modelID == 1)
    tab3_pairUI();
}

void glm::tab3_inter() {
  clearG();
  VB_Vector *newVector = new VB_Vector(totalReps * tab3_TR / tmpResolve);
  newVector->setAll(1.0);
  covList.push_back(newVector);
  QString idStr = QString::number(covList.size() - 1);
  (void)new Q3ListViewItem(tab3->covView, getLastChild(tab3->covView),
                           "Intercept", "I", idStr, "intercept");
  tab3->covView->hideColumn(3);
  interceptID.push_back(covList.size() - 1);
  g_TR = tab3_TR, g_totalReps = totalReps;
  // cout << TR << " " << g_TR << " " << tab3_TR << endl;
  gUpdate = true;
  mainNextButton->setEnabled(true);
}

/* This slot is called when "block design" radio button is toggled. */
void glm::tab3_blockUI() {
  // check TR
  if (!tab3_TR) {
    QMessageBox::critical(0, "Error!", "TR is required for model design");
    return;
  }
  // Current TR is different from the one in G matrix (if it exists)
  if (g_TR && tab3_TR != g_TR) {
    QMessageBox::critical(0, "Error!",
                          "TR is different from the value in current G matrix");
    return;
  }
  // Current number of points is different from the one in G matrix (if it
  // exists)
  if (g_totalReps && totalReps != g_totalReps) {
    QMessageBox::critical(
        0, "Error!",
        "Number of points is different from the value in current G matrix");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!",
                          "Number of time points is required for model design");
    return;
  }
  // make sure TR is good
  if (tab3_TR % tmpResolve) {
    QMessageBox::critical(
        0, "Error",
        "TR is not a multiple of upsampling rate. Please click edit button to "
        "launch main G design interface and reset upsampling rate");
    return;
  }

  this->setDisabled(true);
  tab3_block = new BlockDesign();
  tab3_block->show();
  tab3_block->setFixedSize(tab3_block->width(), tab3_block->height());

  tab3_block->nameEditor->setText("block");
  tab3_block->nameEditor->setFocus();
  tab3_block->nameEditor->selectAll();

  QIntValidator *onVal = new QIntValidator(tab3_block->onEditor);
  onVal->setBottom(1);
  tab3_block->onEditor->setValidator(onVal);
  QIntValidator *offVal = new QIntValidator(tab3_block->offEditor);
  offVal->setBottom(1);
  tab3_block->offEditor->setValidator(offVal);
  QIntValidator *numberVal = new QIntValidator(tab3_block->numberEditor);
  numberVal->setBottom(2);
  tab3_block->numberEditor->setValidator(numberVal);

  QObject::connect(tab3_block->okButton, SIGNAL(clicked()), this,
                   SLOT(okBlock()));
  QObject::connect(tab3_block->cancelButton, SIGNAL(clicked()), this,
                   SLOT(cancelBlock()));
}

/* This function deals with the signal emitted from "ok" button on block design
 */
void glm::okBlock() {
  if (!chkBlockUI()) return;

  tab3_block->close();
  this->setEnabled(true);

  VB_Vector *blockVec = new VB_Vector(totalReps);
  int i = 0;
  while (i < totalReps) {
    for (int j = 0; j < firstLen; j++) blockVec->setElement(i + j, first);
    if ((i + firstLen) < totalReps - 1) {
      for (int k = 0; k < secondLen; k++)
        blockVec->setElement(i + firstLen + k, second);
    }
    i += firstLen + secondLen;
  }

  int upRatio = tab3_TR / tmpResolve;
  VB_Vector *newVector = upSampling(blockVec, upRatio);
  covList.push_back(newVector);
  QString covID = QString::number(covList.size() - 1);
  delete blockVec;
  (void)new Q3ListViewItem(tab3->covView, getLastChild(tab3->covView),
                           tab3_block->nameEditor->text(), "I", covID);
  // Add intercept
  if (!interceptID.size()) addIntercept();

  g_TR = tab3_TR, g_totalReps = totalReps;
  gUpdate = true;
  mainNextButton->setEnabled(true);
}

/* This is a generic function to add intercept.
 * It is also called by block and paired t-test models. */
void glm::addIntercept() {
  VB_Vector *newVector = new VB_Vector(totalReps * tab3_TR / tmpResolve);
  newVector->setAll(1.0);
  covList.push_back(newVector);
  QString idStr = QString::number(covList.size() - 1);
  (void)new Q3ListViewItem(tab3->covView, getLastChild(tab3->covView),
                           "Intercept", "K", idStr, "intercept");
  tab3->covView->hideColumn(3);
  interceptID.push_back(covList.size() - 1);
}

/* This function deals with the signal emitted from "cancel" button on block
 * design */
void glm::cancelBlock() {
  tab3_block->close();
  this->setEnabled(true);
}

/* This function checks the input on block design interface to make sure:
 * (1) Effect name field is not blank;
 * (2) Block on/off length is positive integer;
 * (3) Number of blocks is an integer larger than 1. */
bool glm::chkBlockUI() {
  QString blockStr = tab3_block->nameEditor->text();
  if (blockStr.isEmpty()) {
    QMessageBox::critical(tab3_block, "Error", "Effect name not found.");
    return false;
  }

  int onLength = tab3_block->onEditor->text().toInt();
  if (onLength < 1) {
    QMessageBox::critical(tab3_block, "Error",
                          "Minimum block length of on is 1.");
    return false;
  }

  int offLength = tab3_block->offEditor->text().toInt();
  if (offLength < 1) {
    QMessageBox::critical(tab3_block, "Error",
                          "Minimum block length of off is 1.");
    return false;
  }

  if (tab3_block->ms->isChecked()) {
    if (onLength % tab3_TR != 0) {
      QMessageBox::critical(tab3_block, "Error",
                            "Length of on block must be a multiple of TR.");
      return false;
    }
    if (offLength % tab3_TR != 0) {
      QMessageBox::critical(tab3_block, "Error",
                            "Length of off block must be a multiple of TR.");
      return false;
    }

    onLength = onLength / tab3_TR;
    offLength = offLength / tab3_TR;
  }

  int blockNum = tab3_block->numberEditor->text().toInt();
  if (blockNum < 2) {
    QMessageBox::critical(tab3_block, "Error",
                          "Minimum value of number of blocks is 2.");
    return false;
  }

  first = 1, second = 0;
  firstLen = onLength, secondLen = offLength;
  int pairNum = blockNum / 2;
  int endLen = 0;
  if (tab3_block->offFirst->isChecked()) {
    firstLen = offLength, secondLen = onLength;
    first = 0, second = 1;
  }
  if (blockNum % 2) endLen = firstLen;

  int totalReps_block = (firstLen + secondLen) * pairNum + endLen;
  if (totalReps != totalReps_block) {
    QMessageBox::critical(tab3_block, "Error",
                          "Number of points on this interface doesn't match "
                          "the value on main interface.");
    return false;
  }

  return true;
}

/* This slot is called when "block design" radio button is toggled. */
void glm::tab3_pairUI() {
  // check TR
  if (!tab3_TR) {
    QMessageBox::critical(0, "Error!", "TR is required for model design");
    return;
  }
  // Current TR is different from the one in G matrix (if it exists)
  if (g_TR && tab3_TR != g_TR) {
    QMessageBox::critical(0, "Error!",
                          "TR is different from the value in current G matrix");
    return;
  }
  // Current number of points is different from the one in G matrix (if it
  // exists)
  if (g_totalReps && totalReps != g_totalReps) {
    QMessageBox::critical(
        0, "Error!",
        "Number of points is different from the value in current G matrix");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!",
                          "Number of time points is required for model design");
    return;
  }
  // make sure TR is good
  if (tab3_TR % tmpResolve) {
    QMessageBox::critical(
        0, "Error",
        "TR is not a multiple of upsampling rate. Please click edit button to "
        "launch main G design interface and reset upsampling rate");
    return;
  }

  this->setDisabled(true);
  tab3_pair = new PairDesign();
  tab3_pair->show();
  tab3_pair->setFixedSize(tab3_pair->width(), tab3_pair->height());

  tab3_pair->nameEditor->setFocus();
  tab3_pair->nameEditor->selectAll();
  QIntValidator *numberVal = new QIntValidator(tab3_pair->numberEditor);
  numberVal->setBottom(2);
  tab3_pair->numberEditor->setValidator(numberVal);

  connect(tab3_pair->okButton, SIGNAL(clicked()), this, SLOT(okPair()));
  connect(tab3_pair->cancelButton, SIGNAL(clicked()), this, SLOT(cancelPair()));
}

/* This slot takes care of "ok" button click from paired design interface */
void glm::okPair() {
  if (!chkPairUI()) return;

  tab3_pair->close();
  this->setEnabled(true);

  VB_Vector *mainVec = new VB_Vector(totalReps);
  mainVec->setAll(0);
  if (tab3_pair->group->isChecked()) {
    for (int i = totalReps / 2; i < totalReps; i++) mainVec->setElement(i, 1);
  } else {
    for (int i = 1; i < totalReps; i += 2) mainVec->setElement(i, 1);
  }
  VB_Vector *mainVec_up = upSampling(mainVec, tab3_TR / tmpResolve);
  covList.push_back(mainVec_up);
  delete mainVec;
  // add the first group covariate, whose name is the input effect name string
  QString idStr = QString::number(covList.size() - 1);
  (void)new Q3ListViewItem(tab3->covView, getLastChild(tab3->covView),
                           tab3_pair->nameEditor->text(), "I", idStr);
  // add a new group to include subject covariates
  Q3ListViewItem *pairGrp = new Q3ListViewItem(
      tab3->covView, getLastChild(tab3->covView), "subjects");
  pairGrp->setOpen(true);
  // add the other n-1 covariates into the tree view
  for (int i = 0; i < totalReps / 2 - 1; i++) {
    VB_Vector *subVec = new VB_Vector(totalReps);
    subVec->setAll(0);
    if (tab3_pair->group->isChecked()) {
      subVec->setElement(i, 1);
      subVec->setElement(i + totalReps / 2, 1);
    } else {
      subVec->setElement(2 * i, 1);
      subVec->setElement(2 * i + 1, 1);
    }
    VB_Vector *subVec_up = upSampling(subVec, tab3_TR / tmpResolve);
    covList.push_back(subVec_up);
    idStr = QString::number(covList.size() - 1);
    QString subName = "subject " + QString::number(i + 1);
    (void)new Q3ListViewItem(pairGrp, getLastChild(pairGrp), subName, "I",
                             idStr);
    delete subVec;
  }

  // Add intercept
  if (!interceptID.size()) addIntercept();

  g_TR = tab3_TR, g_totalReps = totalReps;
  gUpdate = true;
  mainNextButton->setEnabled(true);
}

/* This slot takes care of "cancel" button click from paired design interface */
void glm::cancelPair() {
  tab3_pair->close();
  this->setEnabled(true);
}

/* This function checks the input on paired design interface */
bool glm::chkPairUI() {
  QString nameStr = tab3_pair->nameEditor->text();
  if (nameStr.isEmpty()) {
    QMessageBox::critical(tab3_pair, "Error", "Effect name not found.");
    return false;
  }

  int subNum = tab3_pair->numberEditor->text().toInt();
  if (subNum < 2) {
    QMessageBox::critical(tab3_pair, "Error",
                          "Minimum value of number of subjects is 2.");
    return false;
  }

  if (totalReps != 2 * subNum) {
    QMessageBox::critical(tab3_pair, "Error",
                          "Number of points on this interface doesn't match "
                          "the value on main interface.");
    return false;
  }

  return true;
}

/* This slot takes care of "done" signal" from gdw interface */
void glm::gdwDone(vector<VB_Vector *> gdwList, Q3ListView *gdwView,
                  int gdwSampling, QString gdwString, tokenlist gdwKeys,
                  bool gdw_meanAll) {
  this->setEnabled(true);
  // Sync tree view and covList with gdw interface
  cpGdwList(gdwList);
  cpView(gdwView, tab3->covView);
  updateInterceptID();

  tmpResolve = gdwSampling;
  condRef = gdwString;
  meanAll = gdw_meanAll;
  if (condKey.size()) condKey.clear();
  condKey = tokenlist(gdwKeys);

  if (covList.size())
    mainNextButton->setEnabled(true);
  else
    mainNextButton->setEnabled(false);
}

/* Copy gdw's covList back to glm */
void glm::cpGdwList(vector<VB_Vector *> inputList) {
  if (covList.size()) covList.clear();

  int covNum = inputList.size();
  for (int i = 0; i < covNum; i++) {
    VB_Vector *tmpVec = new VB_Vector(inputList[i]);
    covList.push_back(tmpVec);
  }

  g_TR = tab3_TR, g_totalReps = totalReps;
  gUpdate = true;
}

/* This function returns intercept covariate's ID. Return -1 if no intercept is
 * defined */
void glm::updateInterceptID() {
  interceptID.clear();
  QString col2, col3;
  Q3ListViewItemIterator it(tab3->covView);
  while (it.current()) {
    col2 = it.current()->text(2);
    col3 = it.current()->text(3);
    if (col2.length() && col3 == "intercept")
      interceptID.push_back(col2.toUInt());
    it++;
  }
}

/*******************************************************************************
 * Tab 4 setup
 *******************************************************************************/
void glm::setupTab4() {
  tab4 = new Q3VBox(tabSection);
  tab4->setMargin(20);
  tab4->setSpacing(20);

  Q3VGroupBox *smooth = new Q3VGroupBox("Define Exogenous Smoothing", tab4);
  smooth->setInsideSpacing(20);

  filter_hint = new QLabel("No covariates defined yet.", smooth);
  Q3HBox *filterRow = new Q3HBox(smooth);
  QLabel *notchLabel = new QLabel("Notch Filter: ", filterRow);
  notchLabel->setFixedWidth(80);

  Q3VBox *box1 = new Q3VBox(filterRow);
  (void)new QLabel("", filterRow);
  Q3HBox *row1 = new Q3HBox(box1);
  Q3HBox *row2 = new Q3HBox(box1);
  row1->setSpacing(10);
  row2->setSpacing(10);

  QLabel *belowTxt = new QLabel("at and below: ", row1);
  belowTxt->setFixedWidth(80);
  belowEdit = new QLineEdit("0", row1);
  belowEdit->setFixedWidth(80);
  belowLab = new QLabel("------ Hz", row1);
  belowLab->setFixedWidth(100);
  QObject::connect(belowEdit, SIGNAL(lostFocus()), this, SLOT(convertBelow()));
  QObject::connect(belowEdit, SIGNAL(returnPressed()), this,
                   SLOT(convertBelow()));

  QLabel *aboveTxt = new QLabel("at and above: ", row2);
  aboveTxt->setFixedWidth(80);
  aboveEdit = new QLineEdit("0", row2);
  aboveEdit->setFixedWidth(80);
  aboveLab = new QLabel("------ Hz", row2);
  aboveLab->setFixedWidth(100);
  QObject::connect(aboveEdit, SIGNAL(lostFocus()), this, SLOT(convertAbove()));
  QObject::connect(aboveEdit, SIGNAL(returnPressed()), this,
                   SLOT(convertAbove()));

  // Create a special line to hold the combo box 1 and textedit box for
  // user-defined TR
  Q3HBox *kerBox = new Q3HBox(smooth);
  (void)new QLabel("Convolution Kernel: ", kerBox);

  combo1 = new QComboBox(kerBox);
  combo1->insertItem("No Kernel");
  combo1->insertItem("Load");

  // Search voxbo fiters dir for smoothing kernel files
  QDir vbfilter(elementPath + "filters");
  QString tmpFilter;
  for (uint i = 0; i < vbfilter.count(); i++) {
    tmpFilter = vbfilter[i];
    if (tmpFilter == "Eigen1.ref")
      combo1->insertItem("First eigenvector");
    else if (tmpFilter == "EmpiricalBlocked_IRF.ref")
      combo1->insertItem("Blocked IRF");
    else if (tmpFilter == "Poisson_IRF.ref")
      combo1->insertItem("Poisson");
    else if (tmpFilter.endsWith(".ref")) {
      tmpFilter.remove(tmpFilter.length() - 4, 4);
      combo1->insertItem(tmpFilter);
    }
  }

  QObject::connect(combo1, SIGNAL(activated(int)), this,
                   SLOT(smoothKernSelected(int)));
  QLabel *space1 = new QLabel("", kerBox);
  (void)new QLabel("Kernel TR: ", kerBox);
  kernelTR = new QLineEdit("2000", kerBox);
  QLabel *space2 = new QLabel("", kerBox);

  kerBox->setStretchFactor(space1, 1);
  kerBox->setStretchFactor(kernelTR, 2);
  kerBox->setStretchFactor(space2, 3);
  smoothKernel = new QLabel("", smooth);

  Q3VGroupBox *timeSeries =
      new Q3VGroupBox("Time Series Correction Options", tab4);
  meanCheck = new QCheckBox("Mean norm data", timeSeries);
  meanCheck->setChecked(true);
  driftCheck = new QCheckBox("Drift correction", timeSeries);
  driftCheck->setChecked(true);

  Q3VGroupBox *noise = new Q3VGroupBox("Define Intrinsic Noise Model", tab4);
  noise->setInsideSpacing(10);

  // "Create empirical 1/f fit" button
  QPushButton *oneOverFButt =
      new QPushButton("Create empirical 1/f fit", noise);
  oneOverFButt->setFixedWidth(200);
  QObject::connect(oneOverFButt, SIGNAL(clicked()), this,
                   SLOT(oneOverFClicked()));

  combo2 = new QComboBox(noise);
  combo2->setFixedWidth(200);
  combo2->insertItem("None");
  combo2->insertItem("Load");

  // Search voxbo noise model dir for noise model files
  QDir vbnoise(elementPath + "noisemodels");
  QString tmpNoise;
  for (uint i = 0; i < vbnoise.count(); i++) {
    tmpNoise = vbnoise[i];
    if (tmpNoise == "unsmooth_params.ref")
      combo2->insertItem("Unsmooth, No GSCV");
    else if (tmpNoise == "smooth_params.ref")
      combo2->insertItem("Smooth, No GSCV");
    else if (tmpNoise == "unsmooth__GCV_params.ref")
      combo2->insertItem("Unsmooth, with GSCV");
    else if (tmpNoise == "smooth__GCV_params.ref")
      combo2->insertItem("Smooth, with GSCV");
    else if (tmpNoise.endsWith(".ref")) {
      tmpNoise.remove(tmpNoise.length() - 4, 4);
      combo2->insertItem(tmpNoise);
    }
  }

  noiseModel = new QLabel("", noise);
  noiseModel->setFixedWidth(500);
  QObject::connect(combo2, SIGNAL(activated(int)), this,
                   SLOT(noiseModSelected(int)));

  tabSection->addTab(tab4, "Filter and Noise Model");
  // tabSection->setTabEnabled(tab4, false);
}

/* This function checks each covariate and gives the minimum frequency to which
 * the power spectrum is less than 1%. */
void glm::setFilterHint() {
  if (!covList.size()) {
    filter_hint->setText("(No covariates defined yet.)");
    return;
  }

  if (!gUpdate) return;

  int lowFreqIndex = -10;
  for (unsigned i = 0; i < covList.size(); i++) {
    if (covList[i]->getVariance() <= 1e-15) continue;

    VB_Vector *downVec = downSampling(covList[i], g_TR / tmpResolve);
    VB_Vector *fftVec = fftNyquist(downVec);
    int newVal = getLowFreq(fftVec) - 1;
    if (lowFreqIndex == -10 || lowFreqIndex > newVal) lowFreqIndex = newVal;

    delete downVec;
    delete fftVec;
  }

  if (lowFreqIndex == -10)
    filter_hint->setText("(No non-intercept covariates defined yet.)");
  else if (lowFreqIndex == -1)
    filter_hint->setText("(Cut-off frequency not found.)");
  else if (lowFreqIndex == 0)
    filter_hint->setText("(Frequency 0 < 1% power for all covariates.)");
  else
    filter_hint->setText("(Frequencies 0-" + QString::number(lowFreqIndex) +
                         " < 1% power for all covariates.)");
}

/* This slot will convert the "at and below" value to a value in unit of Hz */
void glm::convertBelow() {
  bool convertStat;
  double tmpVal = belowEdit->text().toDouble(&convertStat);

  if (!convertStat) {
    QMessageBox::information(0, "Error", "Invalid input value");
    return;
  }

  if (tmpVal != 0) {
    tmpVal = tmpVal / (totalReps * g_TR / 1000.0);
    belowLab->setText(QString::number(tmpVal).left(6) + " Hz");
  } else
    belowLab->setText("------ Hz");
}

/* This slot will convert the "at and above" value to a value in unit of Hz */
void glm::convertAbove() {
  bool convertStat;
  double tmpVal = aboveEdit->text().toDouble(&convertStat);

  if (!convertStat) {
    QMessageBox::information(0, "Error", "Invalid input value");
    return;
  }

  if (tmpVal != 0) {
    tmpVal = (totalReps / 2.0 - (tmpVal - 1.0)) / (totalReps * g_TR / 1000.0);
    aboveLab->setText(QString::number(tmpVal).left(6) + " Hz");
  } else
    aboveLab->setText("------ Hz");
}

// This slot will take care of the combo box 1 options when selecting exogenous
// smoothing models
void glm::smoothKernSelected(int selection) {
  if (selection == 0) {
    filterFile = QString::null;
    smoothKernel->clear();
  } else if (selection == 1) {
    filterFile = Q3FileDialog::getOpenFileName(
        QString::null, "Ref files (*.ref)", this, "Load a filter file",
        "Please choose your filter.");
    if (filterFile.length() != 0) {
      smoothKernel->setText("File loaded is: <b>" + filterFile + "</b>");
    }
  } else {
    filterFile = getFilterFile(selection);
    smoothKernel->setText("File loaded is: <b>" + filterFile + "</b>");
  }
}

/* This function uses vbp.rootdir to get elements directory's absolute path */
void glm::setElementPath() {
  QString vbStr(vbp.rootdir.c_str());
  if (!vbStr.endsWith("/")) vbStr += "/";
  elementPath = vbStr + "elements/";
}

/* getFilterFile(int) returns the smoothing kernel's real file name */
QString glm::getFilterFile(int comboIndex) {
  if (comboIndex < 2) return QString::null;

  QString tmpTxt = combo1->text(comboIndex);
  QString realName;

  if (tmpTxt == "First eigenvector")
    realName = elementPath + "filters/Eigen1.ref";
  else if (tmpTxt == "Blocked IRF")
    realName = elementPath + "filters/EmpiricalBlocked_IRF.ref";
  else if (tmpTxt == "Poisson")
    realName = elementPath + "filters/Poisson_IRF.ref";
  else
    realName = elementPath + "filters/" + tmpTxt + ".ref";

  return realName;
}

/*This is the slot to take care of combo box 2 when selecting noise models,
 * very similar to the slot above */
void glm::noiseModSelected(int selection) {
  if (selection == 0) {
    noiseFile = QString::null;
    noiseModel->clear();
  } else if (selection == 1) {
    noiseFile = Q3FileDialog::getOpenFileName(
        QString::null, "Ref files (*.ref)", this, "Load a filter file",
        "Please choose your filter.");
    if (noiseFile.length() != 0) {
      noiseModel->setText("File loaded is: <b>" + noiseFile + "</b>");
    }
  } else {
    noiseFile = getNoiseFile(selection);
    noiseModel->setText("File loaded is: <b>" + noiseFile + "</b>");
  }
}

/* getNoiseFile(int) returns the noise model's real file name */
QString glm::getNoiseFile(int comboIndex) {
  if (comboIndex < 2) return QString::null;

  QString tmpTxt = combo2->text(comboIndex);
  QString realName;

  if (tmpTxt == "Unsmooth, No GSCV")
    realName = elementPath + "noisemodels/unsmooth_params.ref";
  else if (tmpTxt == "Smooth, No GSCV")
    realName = elementPath + "noisemodels/smooth_params.ref";
  else if (tmpTxt == "Unsmooth, with GSCV")
    realName = elementPath + "noisemodels/unsmooth__GCV_params.ref";
  else if (tmpTxt == "Smooth, with GSCV")
    realName = elementPath + "noisemodels/smooth__GCV_params.ref";
  else
    realName = elementPath + "noisemodels/" + tmpTxt + ".ref";

  return realName;
}

// Slot when "Create empirical 1/f" button is clicked
void glm::oneOverFClicked() {
  QString psFile = Q3FileDialog::getOpenFileName(
      QString::null, "PS files (*_PS.ref)", this, "Load a power spectrum file",
      "Please choose the PS file");
  if (!psFile.length()) return;

  QString condFile = Q3FileDialog::getOpenFileName(
      QString::null, "Ref files (*.ref)", this,
      "Load a condition function mask (cancel for none)",
      "Please choose condition function mask");

  QString defaultName = pathText + "/OneOverFParams.ref";
  QString paramFileName = Q3FileDialog::getSaveFileName(
      defaultName, tr("Ref Files (*.ref);;All Files (*)"), this,
      "Save Fitting Parameter File",
      "Choose a filename to save the fitting parameters under: ");
  QFileInfo *paramFileInfo = new QFileInfo(paramFileName);

  // What will happen if the file already exists?
  if (paramFileInfo->exists()) {
    // pathString is the absolute path of the G matrix file
    QString pathString = paramFileInfo->dirPath(true);
    QFileInfo *paramFilePath = new QFileInfo(pathString);
    switch (QMessageBox::warning(
        0, "Warning!",
        paramFileName + " already exists. Are you sure you want to overwrite "
                        "this file now?",
        "Yes", "No", "Cancel", 0, 2)) {
        // "Yes", simply overwrite the file
      case 0:
        if (!paramFilePath
                 ->isWritable())  // Check if this directory is writable or not
          QMessageBox::critical(
              0, "Error!",
              "You are not permitted to write file in this directory: " +
                  pathString);
        break;
        // If "No", nothing happens so far. How can I make it to start from the
        // beginning?
      case 1:
        break;
        return;
      case 2:  // If "cancel" is clicked, close the function
        break;
        return;
    }
  } else {
    // pathString is the absolute path of the G matrix file
    QString pathString = paramFileInfo->dirPath(true);
    QFileInfo *paramFilePath = new QFileInfo(pathString);
    // Check if this directory is writable or not
    if (!paramFilePath->isWritable())
      QMessageBox::critical(
          0, "Error!",
          "You are not permitted to write file in this directory: " +
              pathString);
  }
  double var3min = -1.0 / (totalReps * g_TR / 1000.0);
  if (!condFile.length())
    fitOneOverF((const char *)psFile, var3min, (double)g_TR, 0.1, 20.0, 2.0,
                -0.0001, (const char *)paramFileName);
  else
    fitOneOverF((const char *)psFile, (const char *)condFile, var3min,
                (double)g_TR, 0.1, 20.0, 2.0, -0.0001,
                (const char *)paramFileName);
}

/* Set up "Submit" and "Cancel" buttons at the bottom */
void glm::setupButtons() {
  mainSubmitButton = new QPushButton("&Submit", this);
  QPushButton *mainCancelButton = new QPushButton("&Quit", this);
  mainNextButton = new QPushButton("&Next", this);
  buttons->addStretch(2);
  buttons->addWidget(mainSubmitButton, 1, Qt::AlignRight);
  buttons->addStretch(1);
  buttons->addWidget(mainCancelButton, 1, Qt::AlignLeft);
  buttons->addStretch(1);
  buttons->addWidget(mainNextButton, 1, Qt::AlignLeft);
  buttons->addStretch(2);

  mainSubmitButton->setDisabled(true);
  connect(tabSection, SIGNAL(currentChanged(QWidget *)), this,
          SLOT(tabChange()));
  connect(mainSubmitButton, SIGNAL(clicked()), this, SLOT(submitAll()));
  connect(mainCancelButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(mainNextButton, SIGNAL(clicked()), this, SLOT(clickNext()));
}

/* Slot to enable the main "submit" button when user is on the last step
 * interface. */
void glm::tabChange() {
  int tabIdx = tabSection->currentPageIndex();
  if (tabIdx == 3) {
    if (covList.size() && tab2_lb2->count()) mainSubmitButton->setEnabled(true);
    mainNextButton->hide();
    setFilterHint();
    return;
  }

  mainNextButton->show();
  string status;
  if (tabIdx == 2) {
    if (TR) {
      tab3_TR = TR;
      status += "TR: " + strnum(tab3_TR) + "ms, ";
    } else if (tab2_lb2->count()) {
      tab3_TR = 1000;
      status += "TR: 1000ms, ";
    } else
      status += "TR: not set, ";

    if (totalReps)
      status += "; Number of Points: " + strnum(totalReps);
    else
      status += "; Number of Points: not set";
    tab3->status->setText(status.c_str());

    if (!tab3->covView->childCount()) mainNextButton->setDisabled(true);
  }
}

/* clickNext() will guide user glm's next step interface */
void glm::clickNext() {
  int glmIndex = tabSection->currentPageIndex();
  if (glmIndex == 0)
    enableTab2();
  else if (glmIndex == 1)
    enableTab3();
  else if (glmIndex == 2) {
    // tabSection->setTabEnabled(tab4, true);
    tabSection->setCurrentPage(3);
  }
}

// submitAll() populates a GLMParams structure, writes out the G
// matrix, and then uses GLMParams to create the GLM dir and job
// sequence

void glm::submitAll() {
  if (!chkSubmit()) return;
  GLMParams gp;
  gp.TR = TR;
  gp.dirname = pathText.toStdString();
  string stemname = gp.dirname + "/" + xfilename(gp.dirname);
  gp.gmatrix = stemname + ".G";
  mkdir(gp.dirname.c_str(), 0777);
  writeG(stemname, g_TR, totalReps, tmpResolve, covList, tab3->covView, condRef,
         condKey, meanAll);
  gp.name = seqEditor->text().toStdString();
  gp.dirname = pathText.toStdString();
  for (uint32 i = 0; i < tab2_lb2->count(); i++)
    gp.scanlist.push_back(tab2_lb2->text(i).toStdString());
  gp.lows = strtol(belowEdit->text().toStdString());
  gp.highs = strtol(aboveEdit->text().toStdString());
  gp.orderg = totalReps;
  if (pieceCheck->isChecked())
    gp.pieces = strtol(pieceEditor->text().toStdString());
  else
    gp.pieces = 0;
  if (filterFile.size()) {
    gp.kernelname = filterFile.toStdString();
    gp.kerneltr = kTR;
  }
  if (noiseFile.size()) gp.noisemodel = noiseFile.toStdString();
  if (condRef.length() > 0) gp.refname = condRef.toStdString();
  gp.pri = priCombo->currentItem() + 1;
  if (auditCheck->isChecked())
    gp.auditflag = 1;
  else
    gp.auditflag = 0;
  if (meanCheck->isChecked())
    gp.meannorm = 1;
  else
    gp.meannorm = 0;
  if (driftCheck->isChecked())
    gp.driftcorrect = 1;
  else
    gp.driftcorrect = 0;
  if (emailCheck->isChecked()) gp.email = emailEditor->text().toStdString();

  gp.FixRelativePaths();
  gp.CreateGLMDir();
  gp.CreateGLMJobs2();
  if (vbp.cores) {
    QRunSeq qr;
    qr.Go(vbp, gp.seq, vbp.cores);
    qr.exec();
  } else {
    if (gp.seq.Submit(vbp))
      QMessageBox::critical(0, "Error",
                            "Error submitting your GLM to the queue.");
    else
      QMessageBox::information(0, "Submitted",
                               "Your GLM has been submitted to the queue");
  }
  // this->close();
  return;
}

/* closeEvent is an overloaded function called when user clicks "quit" or the
 * "x" button
 * on the upperleft corner. If yes, close the window; if no, nothing happens. */
void glm::closeEvent(QCloseEvent *ce) {
  if (endFlag) {
    ce->accept();
    return;
  }

  switch (QMessageBox::warning(0, "Warning!",
                               "Are you sure you want to quit GLM program? \
<p>Your previous setup will be aborted <b>without</b> a backup!",
                               "Quit", "Don't Quit", QString::null, 0, 1)) {
    case 0:
      ce->accept();
      break;
    case 1:
      ce->ignore();
      break;
  }
}
