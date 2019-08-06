
// vbcontrast.cpp
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
// original version written by Tom King?

#include "vbcontrast.h"

#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qsizepolicy.h>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <fstream>

using namespace std;
using namespace VB;

const char *CAPTION = "Contrast Parameter Editor";
int NUM_OF_SCALES = 0;

/*----------------------------------------------------------------------------*
 * VBContrastParamScalingWidget
 *
 * A widget for setting and displaying contrast parameters vectors.
 *----------------------------------------------------------------------------*/
VBContrastParamScalingWidget::VBContrastParamScalingWidget(QWidget *parent,
                                                           const char *name)
    : QDialog(parent, name) {
  mScalesText.push_back("t");
  mScalesText.push_back("i");
  mScalesText.push_back("beta");
  mScalesText.push_back("f");
  mScalesText.push_back("err");
  mScalesText.push_back("tp");
  mScalesText.push_back("tp2");
  mScalesText.push_back("fp");
  mScalesText.push_back("fp2");
  mScalesText.push_back("tz");
  mScalesText.push_back("fz");

  for (unsigned i = 0; i < mScalesText.size(); ++i) {
    mScalesIndex[mScalesText[i]] = i;
    ++NUM_OF_SCALES;
  }

  mScalesIndex["intercept"] = mScalesIndex["int"] = mScalesIndex["pct"] =
      mScalesIndex["percent"] = mScalesIndex["i"];
  mScalesIndex["b"] = mScalesIndex["rawbeta"] = mScalesIndex["rb"] =
      mScalesIndex["beta"];
  mScalesIndex["error"] = mScalesIndex["err"];
  mScalesIndex["tp1"] = mScalesIndex["tp/1"] = mScalesIndex["tp"];
  mScalesIndex["tp/2"] = mScalesIndex["tp2"];
  mScalesIndex["fp1"] = mScalesIndex["fp/1"] = mScalesIndex["fp"];
  mScalesIndex["fp/2"] = mScalesIndex["fp2"];

  /////////////////////////////////////////////////////////////////////////////

  QBoxLayout *main_lay;
  QBoxLayout *top_lay;
  QBoxLayout *bottom_btns_lay;
  QBoxLayout *contrast_lay;
  QBoxLayout *cont_buttons_lay;
  QBoxLayout *cont_scale_lay;
  QBoxLayout *cont_param_lay;
  QBoxLayout *param_props_lay;
  QBoxLayout *param_btns_lay;

  main_lay = new QVBoxLayout(this, 0, 5);

  top_lay = new QHBoxLayout(main_lay, 5);

  contrast_lay = new QVBoxLayout(top_lay, 0);
  contrast_lay->setResizeMode(QLayout::SetMinimumSize);
  contrast_lay->addWidget(new QLabel("Contrast Vectors:", this));
  mContrastList = new VB::ContrastsView(this);
  mContrastList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  contrast_lay->addWidget(mContrastList);
  cont_scale_lay = new QHBoxLayout(contrast_lay);
  cont_scale_lay->addWidget(new QLabel("Scale by...:", this));
  mScaleByCombo = new QComboBox(this);
  mScaleByCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  cont_scale_lay->addWidget(mScaleByCombo);
  cont_buttons_lay = new QHBoxLayout(contrast_lay);
  mNewContrast = new QPushButton("New", this);
  cont_buttons_lay->addWidget(mNewContrast);
  mDupContrast = new QPushButton("Duplicate", this);
  cont_buttons_lay->addWidget(mDupContrast);
  mDelContrast = new QPushButton("Delete", this);
  cont_buttons_lay->addWidget(mDelContrast);

  QFrame *v_separator = new QFrame(this);
  v_separator->setFrameStyle(QFrame::VLine);
  top_lay->addWidget(v_separator);

  cont_param_lay = new QVBoxLayout(top_lay, 0);
  cont_param_lay->addWidget(new QLabel("Contrast Parameter Weights:", this));
  mContrastParamList = new ContParamsView(this);
  mContrastParamList->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
  cont_param_lay->addWidget(mContrastParamList);

  param_props_lay = new QHBoxLayout(cont_param_lay);
  param_props_lay->addWidget(new QLabel("Weight:", this));
  mParamWeight = new QDecimalSpinBox(2, this);
  param_props_lay->addWidget(mParamWeight);

  mShowInterestOnly = new QCheckBox("Show Covariates of Interest Only", this);
  cont_param_lay->addWidget(mShowInterestOnly);
  param_btns_lay = new QHBoxLayout(cont_param_lay);
  mTargetVectorButton =
      new QPushButton("Derive weights from target vector...", this);
  param_btns_lay->addWidget(mTargetVectorButton);
  mZeroAllButton = new QPushButton("Zero all weights", this);
  param_btns_lay->addWidget(mZeroAllButton);

  QFrame *h_separator = new QFrame(this);
  h_separator->setFrameStyle(QFrame::HLine);
  main_lay->addWidget(h_separator);

  // the buttons on the bottom
  bottom_btns_lay = new QHBoxLayout(main_lay, 0);
  mOpenButton = new QPushButton("Browse for GLM...", this);
  mOpenButton->hide();
  bottom_btns_lay->addWidget(mOpenButton);
  mSaveButton = new QPushButton("Done", this);
  bottom_btns_lay->addWidget(mSaveButton);
  mCancelButton = new QPushButton("Cancel", this);
  bottom_btns_lay->addWidget(mCancelButton);

  initialize();

  /////////////////////////////////////////////////////////////////////////////

  mGLMInfo = 0;
  mWriteOnAccept = false;

  connect(mContrastList, SIGNAL(selectionChanged()), this,
          SLOT(onContrastVectorSelected()));
  connect(mContrastList,
          SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this,
          SLOT(onContrastVectorDoubleClicked(Q3ListViewItem *, const QPoint &,
                                             int)));
  connect(mContrastParamList, SIGNAL(selectionChanged()), this,
          SLOT(onContrastParamsSelected()));

  connect(mParamWeight, SIGNAL(valueChanged(int)), this,
          SLOT(changeWeight(int)));

  connect(mScaleByCombo, SIGNAL(activated(int)), this,
          SLOT(onContrastScaleChanged(int)));
  connect(mShowInterestOnly, SIGNAL(toggled(bool)), mContrastParamList,
          SLOT(showInterestOnly(bool)));

  connect(mNewContrast, SIGNAL(clicked()), this, SLOT(onNewContrast()));
  connect(mDupContrast, SIGNAL(clicked()), this, SLOT(onDupContrast()));
  connect(mDelContrast, SIGNAL(clicked()), this, SLOT(onDelContrast()));

  connect(mZeroAllButton, SIGNAL(clicked()), this, SLOT(zeroAll()));

  connect(mOpenButton, SIGNAL(clicked()), this, SLOT(onBrowseForParamFile()));
  connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(mSaveButton, SIGNAL(clicked()), this, SLOT(accept()));

  // Print out diagnostic information with the value of the weight has changed.
  //  connect( mParamWeight, SIGNAL( valueChanged( int ) ),
  //           this, SLOT( diagnostics( int ) ) );
}

/* ~VBContrastParamScalingWidget()
 *
 * VBContrastParamScalingWidget destructor
 */
VBContrastParamScalingWidget::~VBContrastParamScalingWidget() {}

/* void diagnostics( int ) [slot]
 *
 * Prints selected diagnostic information about the state of the widget.
 */
void VBContrastParamScalingWidget::diagnostics(int) {
  VBContrast *contrast = mContrastList->selectedContrast();
  if (contrast) {
    cerr << "Contrast diagnostics:" << endl;
    cerr << contrast->contrast << endl;
  }
}

/* void initialize()
 *
 * Set the initial state of the widget.
 */
void VBContrastParamScalingWidget::initialize() {
  setCaption(CAPTION);

  mScaleByCombo->insertItem("scaled error (t value)");
  mScaleByCombo->insertItem("intercept term percent change");
  mScaleByCombo->insertItem("nothing, just use raw beta values");
  mScaleByCombo->insertItem("ratio of error (F value)");
  mScaleByCombo->insertItem("nothing, just show raw error values");
  mScaleByCombo->insertItem("p map of t values");             // 1-tailed test
  mScaleByCombo->insertItem("two-tailed p map of t values");  // 2-tailed test
  mScaleByCombo->insertItem("p map for F values");
  mScaleByCombo->insertItem("two-tailed p map for F values");
  mScaleByCombo->insertItem("Z map of t values");
  mScaleByCombo->insertItem("Z map for F values");
  mScaleByCombo->insertItem("(no contrast selected)");
  mScaleByCombo->setCurrentItem(NUM_OF_SCALES);
  mScaleByCombo->setEnabled(false);

  mParamWeight->setMinValue(-10000);  // FIX ME: come up with better
  mParamWeight->setMaxValue(10000);   //         min and max values.
  clearContrastParamProps();

  mContrastParamList->setEnabled(false);
  mShowInterestOnly->setChecked(true);

  mTargetVectorButton->setEnabled(false);
  mZeroAllButton->setEnabled(false);

  mNewContrast->setEnabled(false);
  mDupContrast->setEnabled(false);
  mDelContrast->setEnabled(false);

  mSaveButton->setEnabled(false);
}

/*
 * LoadContrastInfo reads information for populating the widget in through a
 * GLMInfo structure.
 */
void VBContrastParamScalingWidget::LoadContrastInfo(string stemname) {
  // /xjet/kimberg/demo/glms/subjects/aaa
  if (mGLMInfo) delete mGLMInfo;

  mGLMInfo = new GLMInfo();
  mGLMInfo->setup(stemname);

  // If there were no problems then cnames should have some non-zero
  // number of elements.  Otherwise, an error should be signified.
  if (mGLMInfo->cnames.empty()) {
    std::cerr << "the glm info in " << stemname << " is empty." << std::endl;
    return;
  }

  // Prepare and build the parameter tree
  mContrastParamList->clear();
  mContrastParamList->buildTree(mGLMInfo, !mShowInterestOnly->isChecked());

  // Populate the contrast names list
  mContrastList->buildList(mGLMInfo);

  setCaption(CAPTION + QString(" -- ") + mFileStem.c_str());
  mSaveButton->setEnabled(true);
  mNewContrast->setEnabled(true);
}

/*
 * WriteContrastInfo writes information in the widget out to a contrast.txt file
 * in the glm directory.
 */
void VBContrastParamScalingWidget::WriteContrastInfo(string stemname) {
  ofstream output;

  //  output.open((stemname + ".contrasts").c_str());
  int pos = stemname.rfind("/");
  stemname = stemname.substr(0, ++pos);
  output.open((stemname + "contrasts.txt").c_str());
  if (output.good()) {
    VBContrast *contrast;
    Q3ListViewItemIterator iter(mContrastList);
    while (iter.current()) {
      contrast = mContrastList->contrastAt(*iter);
      output << contrast->name << " " << contrast->scale << " vec ";

      for (uint32 i = 0; i < contrast->contrast.size(); ++i)
        // only write out covariates of interest...
        if (mGLMInfo->cnames[i][0] == 'I')
          output << contrast->contrast[i] << " ";

      output << endl;
      ++iter;
    }
  }
}

/*
 * Sets whether the contrast file will be written/updated when the widget
 * closes.
 */
void VBContrastParamScalingWidget::writeFilesOnExit(bool s) {
  mWriteOnAccept = s;
}

/*
 * Show or hide the browse button (hidden by default).
 */
void VBContrastParamScalingWidget::showBrowseButton(bool s) {
  if (s)
    mOpenButton->show();
  else
    mOpenButton->hide();
}

/*
 * Return the VBContrast pointer that corresponds to the currently selected
 * item in /mContrastList/.
 */
VBContrast *VBContrastParamScalingWidget::selectedContrast() {
  return mContrastList->selectedContrast();
}

/* void onContrastVectorSelected() [slot]
 *
 * onContrastVectorSelected is called when the selected item in the contrast
 * vector list has changed.
 */
void VBContrastParamScalingWidget::onContrastVectorSelected() {
  Q3ListViewItem *item = mContrastList->selectedItem();
  VBContrast *contrast = mContrastList->contrastAt(item);

  if (contrast) {
    mContrastParamList->setEnabled(true);

    mScaleByCombo->setCurrentItem(mScalesIndex[contrast->scale]);
    mScaleByCombo->setEnabled(true);

    mContrastParamList->setContrast(*contrast);
    mNewContrast->setEnabled(true);
    mDupContrast->setEnabled(true);
    mDelContrast->setEnabled(true);

    mZeroAllButton->setEnabled(true);
  } else {
    mScaleByCombo->insertItem("(no contrast selected)");
    mScaleByCombo->setCurrentItem(NUM_OF_SCALES);
    mScaleByCombo->setEnabled(false);
    mContrastParamList->clearContrast();
    mContrastParamList->setEnabled(false);

    mDupContrast->setEnabled(false);
    mDelContrast->setEnabled(false);

    mZeroAllButton->setEnabled(false);
  }
  mContrastParamList->clearSelection();
}

/*
 * Upon double click, emit a signal with the currently selected contrast and
 * close the widget.
 */
void VBContrastParamScalingWidget::onContrastVectorDoubleClicked(
    Q3ListViewItem *, const QPoint &, int) {
  if (mContrastList->selectedContrast() != 0) accept();
}

/* void onContrastParamsSelected() [slot]
 *
 * onContrastParamsSelected is called when the selected item in the contrast
 * parameter list has changed.
 */
void VBContrastParamScalingWidget::onContrastParamsSelected() {
  list<Q3ListViewItem *>::iterator sel_iter;
  list<Q3ListViewItem *> &sel_list = mContrastParamList->selectedItems();

  // On no selection...
  if (sel_list.empty()) {
    clearContrastParamProps();
    return;
  }

  Q3ListViewItem *item;
  //  int name_col = mContrastParamList->columnNumber(ContParamsView::NAME_COL);
  //  int id_col = mContrastParamList->columnNumber(ContParamsView::ID_COL);
  int type_col = mContrastParamList->columnNumber(ContParamsView::TYPE_COL);
  int weight_col = mContrastParamList->columnNumber(ContParamsView::WEIGHT_COL);

  // On just one item selected...
  if (sel_list.size() == 1) {
    item = sel_list.front();

    if (item->text(type_col) == "I")
      mParamWeight->setEnabled(true);
    else
      mParamWeight->setEnabled(false);
    mParamWeight->setValue(int(item->text(weight_col).toDouble() * 100));

    return;
  }

  // On multiple items selected...
  for (sel_iter = mContrastParamList->selectedItems().begin();
       sel_iter != mContrastParamList->selectedItems().end(); ++sel_iter) {
  }
}

/* void onContrastScaleChanged() [slot]
 *
 * onContrastScaleChanged is called when the user selects a new scale for the
 * current contrast.
 */
void VBContrastParamScalingWidget::onContrastScaleChanged(int scale) {
  if (scale == NUM_OF_SCALES) {
    mScaleByCombo->setCurrentItem(
        mScalesIndex[mContrastList->selectedContrast()->scale]);
    return;
  }

  mContrastList->selectedContrast()->scale = mScalesText[scale];
  mContrastList->selectedItem()->setText(1, mScalesText[scale].c_str());
}

/* void clearContrastParamProps()
 *
 * clearContrastParamProps is called whenever no contrast parameter is selected
 * from the contrast parameter list.  It sets the contrast parameter property
 * widgets to disabled and clears their values.
 */
void VBContrastParamScalingWidget::clearContrastParamProps() {
  mParamWeight->setEnabled(false);
  mParamWeight->setValue(0);
}

/* void onBrowseForParamFile() [slot]
 *
 * onBrowseForParamFile is called when the Open or Browse button is clicked.
 * It looks for files that have the extension ".prm".
 */
void VBContrastParamScalingWidget::onBrowseForParamFile() {
  // Open a file dialog with the mask *.prm
  Q3FileDialog file_dial(QString::null, "Parameter Files (*.prm)", this,
                         "open file dialog", false);
  file_dial.show();
  QString s = file_dial.getOpenFileName(
      QString::null, "Parameter Files (*.prm)", this, "open file dialog",
      "Choose a parameter file...");

  // If a glm folder (or prm file) was chosen, then load it.
  if (s != QString::null) {
    mFileStem = s.left(s.length() - 4).ascii();
    LoadContrastInfo(mFileStem);
  }
}

/* onNewContrast()
 *
 * onNewContrast is called when the user presses the new contrast button.
 */
void VBContrastParamScalingWidget::onNewContrast() {
  // Get the name for the new contrast.
  bool ok;
  QString name = QInputDialog::getText(
      this, "Create a new contrast...",
      "Please enter a name for this contrast vector:", QLineEdit::Normal,
      QString::null, &ok);

  if (!ok || name.isEmpty()) {
    return;
  }

  // Insert the contrast into the list.
  VBContrast *newcont = new VBContrast();
  newcont->name = name.ascii();
  newcont->scale = "t";

  Q3ListViewItemIterator iter(mContrastParamList);
  int numvar = 0;
  while (iter.current()) {
    if (!iter.current()
             ->text(mContrastParamList->columnNumber(ContParamsView::ID_COL))
             .isEmpty())
      ++numvar;

    ++iter;
  }
  newcont->contrast.resize(numvar);

  mContrastList->insertContrast(newcont);
  mContrastList->setSelected(mContrastList->lastItem(), true);
  zeroAll();
}

/* onDupContrast()
 *
 * onDupContrast is called when the user presses the duplicate contrast button.
 */
void VBContrastParamScalingWidget::onDupContrast() {
  QString copyFromName = mContrastList->selectedContrast()->name.c_str();

  // Get the name for the new contrast.
  bool ok;
  QString name = QInputDialog::getText(
      this, "Duplicate the contrast '" + copyFromName + "'...",
      "Please enter a name for this contrast vector:", QLineEdit::Normal,
      QString::null, &ok);

  if (!ok || name.isEmpty()) {
    return;
  }

  // Insert the contrast into the list.
  VBContrast *copy = new VBContrast(*(mContrastList->selectedContrast()));
  copy->name = name.ascii();
  mContrastList->insertContrast(copy);
  mContrastList->setSelected(mContrastList->lastItem(), true);
}

void VBContrastParamScalingWidget::onDelContrast() {
  QString deleteName = mContrastList->selectedContrast()->name.c_str();
  /*
    if (mContrastList->childCount() <= 1)
    {
      QMessageBox::critical(this,
        QString("Cannot delete the contrast '%1'...").arg(deleteName),
        QString("There must be at least one contrast in the contrast list.  ").
          append("Cannot delete the contrast '%1'").arg(deleteName));
      return;
    }
  */
  int button = QMessageBox::warning(
      this, QString("Remove the contrast '%1'...").arg(deleteName),
      QString("Are you sure you want to delete the contrast '%1'?  ")
          .arg(deleteName),
      QMessageBox::Yes | QMessageBox::Default,
      QMessageBox::No | QMessageBox::Escape);

  if (button == QMessageBox::No) return;

  VBContrast *contrast = mContrastList->selectedContrast();
  mContrastList->takeContrast(contrast);
}

/* zeroAll()
 *
 * zeroAll sets all of the covariates for the current contrast to a given
 * value (which is, by default, 0.0).
 */
void VBContrastParamScalingWidget::zeroAll(float value) {
  VBContrast *contrast = mContrastList->selectedContrast();
  int size = contrast->contrast.size();
  for (int i = 0; i < size; ++i) {
    contrast->contrast[i] = value;
  }
  mContrastParamList->setColumnText(
      mContrastParamList->columnNumber(ContParamsView::WEIGHT_COL),
      QString::number(value, 'f'));
}

void VBContrastParamScalingWidget::changeType(int type) {
  VBContrast *contrast = mContrastList->selectedContrast();
  if (!contrast) return;

  QString stype;
  switch (type) {
    case 1:
      stype = "I";
      break;
    case 2:
      stype = "N";
      break;
    case 3:
      stype = "K";
      break;
    case 4:
      stype = "U";
      break;
    case 5:
      stype = "D";
      break;
    default:
      return;
  }

  // update the list view...
  mContrastParamList->setSelectedColumnText(ContParamsView::TYPE_COL, stype);

  // update the GLM.
  list<int>::iterator iter;
  for (iter = mContrastParamList->selectedItemIDs().begin();
       iter != mContrastParamList->selectedItemIDs().end(); ++iter) {
    // If there were some GLM to keep track of the changing types, then here
    // is where that object would be updated.  Each iter represents the index
    // of the contrast parameter to which it corresponds.
  }
}

void VBContrastParamScalingWidget::changeWeight(int weight) {
  VBContrast *contrast = mContrastList->selectedContrast();
  if (!contrast) return;

  double dweight = double(weight) / 100.0;
  QString weight_str = QString::number(dweight, 'f', 2);

  // update the list view...
  mContrastParamList->setSelectedColumnText(ContParamsView::WEIGHT_COL,
                                            weight_str);

  // update the contrast...
  list<int>::iterator iter;
  for (iter = mContrastParamList->selectedItemIDs().begin();
       iter != mContrastParamList->selectedItemIDs().end(); ++iter) {
    contrast->contrast[*iter] = dweight;
  }
}

/* void accept() [slot]
 *
 * accept is called when the user wishes to close the
 * VBContrastParamScalingWidget and save their changes.
 */
void VBContrastParamScalingWidget::accept() {
  if (mWriteOnAccept && mContrastList->childCount() > 0)
    WriteContrastInfo(mFileStem);
  else if (mContrastList->childCount() <= 0)
    QMessageBox::critical(this, "Cannot write contrast file...",
                          "No contrasts exist in the contrast list.  Cannot "
                          "write contrast file.");

  emit contrastAccepted(mContrastList->selectedContrast());
  return QDialog::accept();
}

void VBContrastParamScalingWidget::reject() { return QDialog::reject(); }
