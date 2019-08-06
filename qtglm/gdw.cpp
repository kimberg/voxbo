
// gdw.cpp
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

#include <Q3BoxLayout>
#include <Q3HBoxLayout>
#include <Q3PtrList>
#include <Q3VBoxLayout>
#include <QCloseEvent>

using namespace std;

#include "fileopen.xpm"
#include "filesave.xpm"
#include "gdw.h"

#include <q3buttongroup.h>
#include <q3filedialog.h>
#include <q3hbox.h>
#include <q3listbox.h>
#include <q3listview.h>
#include <q3popupmenu.h>
#include <q3textedit.h>
#include <q3toolbar.h>
#include <q3vbox.h>
#include <q3vgroupbox.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qfile.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qstatusbar.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvalidator.h>
#include <Q3VButtonGroup>
#include <QButtonGroup>
#include <QFileDialog>

extern VBPrefs vbp;

/* Gdw constructor */
Gdw::Gdw(int inputNumberOfPoints, int inputTR, int inputMode,
         QString inputFileName)
    : Q3MainWindow(0, "G Design Workshop", Qt::WDestructiveClose) {
  filterPath = getFilterPath();
  init(inputNumberOfPoints, inputTR);
  initGrpCount();
  setMode(inputMode);
  setupMenu();
  setupWidgets();
  setCaption("G Design Workshop: New Design");

  // In combo mode, G matrix filename is passed from glm interface
  if (mode == COMBO) {
    gFileName = inputFileName;
    setCaption("G Design Workshop: " + gFileName + ".G");
    setStatusText();
    return;
  }
  // if the input filename is null
  if (inputFileName.isEmpty()) {
    gFileName = QString::null;
    setStatusText();
    return;
  }
  // If it is SINGLE mode and input filename is not null, read it
  string tmpString((const char *)inputFileName);
  if (!chkFileStat(tmpString.c_str(), true)) {
    setStatusText();
    return;
  }

  VB_Vector testVec;
  // If it's a valid ref file, load it as condition function
  if (testVec.ReadFile(tmpString) == 0) {
    loadCondFunct(inputFileName);
    gFileName = QString::null;
  }
  // If it not ref file, assume it is G matrix file
  else
    readG(tmpString);

  setStatusText();
}

/* Initialize global variables */
void Gdw::init(int inputNumberOfPoints, int inputTR) {
  tesNum = 0;
  xMagnification = 1;
  totalReps = inputNumberOfPoints;
  TR = inputTR;

  tmpResolve = 100;
  psFlag = fitFlag = false;
  gUpdateFlag = false;
  condRef = QString::null;

  /* Make sure vb_vector pointers are initialized to be zero.
   * If not, destructor will generate segv */
  timeVector = 0;
  fftVector = 0;
  noiseVec = 0;
  condVector = 0;
  itemCounter = 0;
  d_index = -1;
  interceptID.clear();
}

/* Deconstructor: delete some vb_vector pointers */
Gdw::~Gdw() {
  if (timeVector) delete timeVector;
  if (fftVector) delete fftVector;
  if (noiseVec) delete noiseVec;
  if (condVector) delete condVector;
  if (covList.size()) covList.clear();
}

/* This function will copy an input covariate list to covList.
 * It is called when "edit" button is clicked on glm tab3 interface */
void Gdw::cpGlmList(vector<VB_Vector *> inputList) {
  if (covList.size()) covList.clear();
  int covNum = inputList.size();
  for (int i = 0; i < covNum; i++) {
    VB_Vector *tmpVec = new VB_Vector(inputList[i]);
    covList.push_back(tmpVec);
  }
}

/* This function sounds like it is doing the same thing as the previous one,
 * but it is written to transfer tmpResolve from glm to gdw. */
void Gdw::cpTmpResolve(int inputVal) { tmpResolve = inputVal; }

/* This function sets a certain condition function in gdw and sets the key list
 * view */
void Gdw::cpGlmCondFxn(QString inputFile, tokenlist inputKeys) {
  condRef = inputFile;
  condVector = new VB_Vector(inputFile.ascii());

  condKey = tokenlist(inputKeys);
  if (keyList->count()) keyList->clear();
  for (size_t m = 0; m < condKey.size(); m++) {
    QString keyString = QString(condKey(m));
    keyList->insertItem(keyString);
  }
  keyList->setCurrentItem(0);
}

/* Set up the menu */
void Gdw::setupMenu() {
  /* A toolbar which has three buttons so far: fileOpen, fileSave, editUndo */
  Q3Action *fileOpenAction;

  fileOpenAction = new Q3Action("Open File", QPixmap(fileopen), "&Open",
                                Qt::CTRL + Qt::Key_O, this, "open");
  connect(fileOpenAction, SIGNAL(activated()), this, SLOT(fileOpen()));

  fileSaveAction = new Q3Action("Save File", QPixmap(filesave), "&Save",
                                Qt::CTRL + Qt::Key_S, this, "save");
  fileSaveAction->setEnabled(false);
  connect(fileSaveAction, SIGNAL(activated()), this, SLOT(fileSave()));

  Q3ToolBar *fileTools = new Q3ToolBar(this, "file operations");
  fileTools->setLabel("File Operations");
  fileOpenAction->addTo(fileTools);
  fileSaveAction->addTo(fileTools);

  Q3ToolBar *editTools = new Q3ToolBar(this, "edit operations");
  editTools->setLabel("Edit Operations");

  /* Create the menu of "File", $QTDIR/lib/doc/examples/textedit/ is a good
   * example on using QAction to generate menubar, here we use a compact form
   * instead */
  fileMenu = new Q3PopupMenu(this);
  menuBar()->insertItem(tr("&File"), fileMenu);

  /* "File" includes the following functionalities */
  fileOpenAction->addTo(fileMenu);
  fileSaveAction->addTo(fileMenu);
  fileMenu->insertItem("Save &As", this, SLOT(fileSaveAs()));

  fileMenu->insertSeparator();
  fileMenu->insertItem("&Done", this, SLOT(close()), Qt::CTRL + Qt::Key_Q);

  /* Create the menu of "Edit" */
  Q3PopupMenu *edit = new Q3PopupMenu(this);
  menuBar()->insertItem("&Edit", edit);

  /* The submenu of "Add Interest" includes 7 functionalities */
  Q3PopupMenu *iMenu = new Q3PopupMenu(this);
  edit->insertItem("Add Interest", iMenu);
  iMenu->insertItem("Contrasts", this, SLOT(add_I_Contrasts()));
  iMenu->insertItem("Diagonal set", this, SLOT(add_I_DS()));
  iMenu->insertItem("Covariate(s) from file", this, SLOT(add_I_Single()));
  iMenu->insertItem("Trial effects", this, SLOT(add_I_Trial()));
  iMenu->insertItem("Var length trial effects", this, SLOT(add_I_varTrial()));

  /* The submenu of "Add No interest" includes 11 functionalities */
  Q3PopupMenu *nMenu = new Q3PopupMenu(this);
  edit->insertItem("Add No interest", nMenu);
  nMenu->insertItem("Contrasts", this, SLOT(add_N_Contrasts()));
  nMenu->insertItem("Diagonal set", this, SLOT(add_N_DS()));
  nMenu->insertItem("Covariate(s) from file", this, SLOT(add_N_Single()));
  nMenu->insertItem("Trial effects", this, SLOT(add_N_Trial()));
  nMenu->insertItem("Var length trial effects", this, SLOT(add_N_varTrial()));
  nMenu->insertSeparator();
  nMenu->insertItem("Global signals", this, SLOT(addGlobal()));
  nMenu->insertItem("Intercept", this, SLOT(slot_addIntercept()));
  nMenu->insertItem("Movement parameters", this, SLOT(addMovement()));
  nMenu->insertItem("Scan effects", this, SLOT(addScanfx()));
  nMenu->insertItem("Spike", this, SLOT(addSpike()));
  nMenu->insertItem("Txt file as multi-covariates", this, SLOT(addTxt()));

  /* "Modify Selection(s)" includes 16 functionalities */
  Q3PopupMenu *modMenu = new Q3PopupMenu(this);
  edit->insertItem("Modify Selection(s)", modMenu);
  modMenu->insertItem("Duplicate", this, SLOT(duplicate()));
  modMenu->insertItem("Delete", this, SLOT(delCov()), Qt::CTRL + Qt::Key_D);
  modMenu->insertSeparator();
  modMenu->insertItem("Convert to delta", this, SLOT(modC2D()));
  modMenu->insertItem("Convolve", this, SLOT(modConv()));
  modMenu->insertItem("Mean center", this, SLOT(modMean()));
  modMenu->insertItem("Mean center non-zero", this, SLOT(meanNonZero()));
  modMenu->insertItem("Multiply by covar", this, SLOT(modMult()));
  modMenu->insertItem("Orthogonalize", this, SLOT(modOrth()));
  modMenu->insertItem("Time shift", this, SLOT(modTS()));
  modMenu->insertItem("Unit excursion", this, SLOT(UnitExcurs()));
  modMenu->insertItem("Unit variance", this, SLOT(unitVar()));
  modMenu->insertSeparator();
  modMenu->insertItem("Derivative(s)", this, SLOT(modDeriv()));
  modMenu->insertItem("Eigenvector set", this, SLOT(modES()));
  modMenu->insertItem("Exponential", this, SLOT(modExpn()));
  modMenu->insertItem("Finite impulse response", this, SLOT(modFIR()));
  modMenu->insertItem("Fourier set", this, SLOT(modFS()));

  // add canned models
  Q3PopupMenu *modelMenu = new Q3PopupMenu(this);
  edit->insertItem("Add Canned Models", modelMenu);
  modelMenu->insertItem("Block Design", this, SLOT(showBlockUI()));
  modelMenu->insertItem("Paired Design", this, SLOT(showPairUI()));

  // "Select All"
  edit->insertItem("Select All", this, SLOT(selectAll()));

  /* Create the menu of "Evaluate" */
  Q3PopupMenu *evaluate = new Q3PopupMenu(this);
  menuBar()->insertItem("E&valuate", evaluate);
  /* "Evaluate" includes 6 functionalities */
  evaluate->insertItem("Efficiency", this, SLOT(efficiency()));
  evaluate->insertItem("Mean and SD", this, SLOT(meanSD()));
  evaluate->insertItem("Noise spectra", this, SLOT(evalNoise()));
  evaluate->insertSeparator();
  evaluate->insertItem("Colinearity with all [ I/N/K ]", this, SLOT(colAll()));
  evaluate->insertItem("Colinearity with interest [ I ]", this, SLOT(col_I()));
  evaluate->insertItem("Colinearity with non-interest [ N/K ]", this,
                       SLOT(col_N()));
  evaluate->insertSeparator();
  evaluate->insertItem("Linear dependence of all [ I/N/K ]", this,
                       SLOT(a_LD()));
  evaluate->insertItem("Linear dependence of all interest [ I ]", this,
                       SLOT(i_LD()));
  evaluate->insertItem("Linear dependence of all non-interest [ N/K ]", this,
                       SLOT(nk_LD()));
  evaluate->insertItem("Linear dependence of selected covariates", this,
                       SLOT(sel_LD()));

  /* Create the menu of "Tools" */
  Q3PopupMenu *tools = new Q3PopupMenu(this);
  menuBar()->insertItem("&Tools", tools);

  /* "Tools" includes four functionalities */
  tools->insertItem("Load condition function", this, SLOT(loadCondFunct()));
  tools->insertItem("Load condition labels", this, SLOT(loadCondLabel()));
  tools->insertItem("Save condition labels", this, SLOT(saveCondLabel()));
  tools->insertSeparator();
  tools->insertItem("Reset upsampling rate", this, SLOT(setUpsampling()));
  tools->insertItem("Save covariate as ref", this, SLOT(saveCov2Ref()));
}

/* Set up the widgets on the interface */
void Gdw::setupWidgets() {
  /*********************************************************************************
   * The menu bar setup is done. The next step is to set up the graphical
   *display, the condition key editor, dependent variables  editor and another
   *graphical display at the bottom
   *********************************************************************************/

  /* tab3_main is a QVBox which includes all three parts */
  Q3VBox *tab3_main = new Q3VBox(this);
  tab3_main->setMargin(10);
  tab3_main->setSpacing(2);

  /* Display a blank screen when first launched */
  upperWindow = new PlotScreen(tab3_main);
  upperWindow->setUpdatesEnabled(true);
  if (TR && tmpResolve) upperWindow->setRatio(TR / tmpResolve);
  QObject::connect(this, SIGNAL(newCovLoaded(VB_Vector *)), this,
                   SLOT(upperWindowUpdate(VB_Vector *)));
  upperWindow->setFocus();

  /* Add "magnification" and "position" slide bars with captions and a "center"
   * button in the middle */
  Q3HBox *widgetBox = new Q3HBox(tab3_main);
  widgetBox->setSpacing(20);

  QLabel *magLab = new QLabel("<b>Magnification</b>", widgetBox);
  magLab->setAlignment(Qt::AlignHCenter);

  /***********************************************************************
   * Horizontal magnification slider bar, with the tickmarks on the above
   * Maximum: 10, Minimum: 1, step: 1, default position: 1
   * May need to change later
   **********************************************************************/
  magSlider = new QSlider(1, 10, 1, 1, Qt::Horizontal, widgetBox);
  magSlider->setTickmarks(QSlider::Below);
  QObject::connect(magSlider, SIGNAL(valueChanged(int)), upperWindow,
                   SLOT(setXMag(int)));
  QObject::connect(upperWindow, SIGNAL(xMagChanged(int)), magSlider,
                   SLOT(setValue(int)));

  QPushButton *graphCenter =
      new QPushButton("Center", widgetBox);  // "center" pushbutton
  QObject::connect(graphCenter, SIGNAL(clicked()), upperWindow,
                   SLOT(centerX()));

  /* In the middle is a QHBox: two lineEdit box on the left side, two radio
   * button groups on the right */
  Q3HBox *middle = new Q3HBox(tab3_main);
  Q3VBox *paraSetBox = new Q3VBox(middle);
  /* The HBOx on top includes label and a line editor for TR input */
  Q3HBox *TREditBox = new Q3HBox(paraSetBox);
  (void)new QLabel("Time of Repetition: ", TREditBox);
  if (TR > 0)
    trString = new QLabel(QString::number(TR), TREditBox);
  else
    trString = new QLabel("Not set", TREditBox);

  QPushButton *trButton = new QPushButton("Edit", TREditBox);
  if (mode == COMBO) trButton->setDisabled(true);

  QObject::connect(trButton, SIGNAL(clicked()), this, SLOT(changeTR()));
  (void)new QLabel("", TREditBox);  // For alignment purpose

  /* Show upsampling rate value */
  Q3HBox *samplingBox = new Q3HBox(paraSetBox);
  (void)new QLabel("Upsampling Rate: ", samplingBox);
  if (tmpResolve > 0)
    samplingStr = new QLabel(QString::number(tmpResolve), samplingBox);
  else
    samplingStr = new QLabel("Not set", samplingBox);

  QPushButton *upsButton = new QPushButton("Edit", samplingBox);
  QObject::connect(upsButton, SIGNAL(clicked()), this, SLOT(setUpsampling()));
  (void)new QLabel("", samplingBox);  // For alignment purpose

  /* The next HBox includes label and a line editor for number of time points */
  Q3HBox *numberEditBox = new Q3HBox(paraSetBox);
  (void)new QLabel("Number of Points: ", numberEditBox);
  if (totalReps > 0)
    numberString = new QLabel(QString::number(totalReps), numberEditBox);
  else
    numberString = new QLabel("Not set", numberEditBox);

  QPushButton *numberButton = new QPushButton("Edit", numberEditBox);
  if (mode == COMBO) numberButton->setDisabled(true);

  QObject::connect(numberButton, SIGNAL(clicked()), this,
                   SLOT(changeNumberPoints()));
  (void)new QLabel("", numberEditBox);  // For alignment purpose

  /* Two groups of radio buttons: Time plot vs. Freq plot and Secs(Hz) vs.
   * TRs(Freq) */
  Q3VBox *radioButtons = new Q3VBox(middle);

  Q3HButtonGroup *radioButton1 = new Q3HButtonGroup(radioButtons);
  radioButton1->setLineWidth(1);  // hide this button group's border
  timePlotButt = new QRadioButton("Time Plot", radioButton1);  // Time Plot
  timePlotButt->setChecked(true);  // Default is "Time Plot"
  QRadioButton *freqPlotButt =
      new QRadioButton("Freq Plot", radioButton1);  // Freq Plot

  fftFlag = 0;  // default plot is in time domain
  secFlag = 1;  // default X axis unit is second/Hz
  QObject::connect(timePlotButt, SIGNAL(clicked()), this,
                   SLOT(timePlotClicked()));
  QObject::connect(freqPlotButt, SIGNAL(clicked()), this,
                   SLOT(freqPlotClicked()));

  (void)new QLabel("", radioButtons);  // for alignment purpose only

  Q3HButtonGroup *radioButton2 = new Q3HButtonGroup(radioButtons);
  radioButton2->setLineWidth(1);  // hide this button group's border
  secButt = new QRadioButton("Secs", radioButton2);  // "Secs" radio button
  secButt->setChecked(true);                         // Default is "Secs"
  QRadioButton *TRButt =
      new QRadioButton("TRs (Freq)", radioButton2);  // "TRs" radio button

  QObject::connect(secButt, SIGNAL(clicked()), this, SLOT(secClicked()));
  QObject::connect(TRButt, SIGNAL(clicked()), this, SLOT(TRClicked()));

  /* Condition keys and variables editor */
  Q3HBox *middleBox = new Q3HBox(tab3_main);
  middleBox->setSpacing(20);

  /* A QVGroupBox called "keyGroup", which includes a listbox, a label and a
   * line editor */
  Q3VGroupBox *keyGroup = new Q3VGroupBox("Condition Keys", middleBox);
  keyGroup->setAlignment(Qt::AlignHCenter);  // caption in the middle
  keyList = new Q3ListBox(keyGroup);         // key listbox

  /* Another HBox which includes label and a line editor */
  Q3HBox *keyEditorBox = new Q3HBox(keyGroup);
  (void)new QLabel("Label: ", keyEditorBox);
  keyEditor = new QLineEdit(keyEditorBox);

  QObject::connect(keyList, SIGNAL(highlighted(int)), this,
                   SLOT(setKeyEditor(int)));
  QObject::connect(keyEditor, SIGNAL(textChanged(const QString &)), this,
                   SLOT(setKeyText()));
  QObject::connect(keyEditor, SIGNAL(returnPressed()), this,
                   SLOT(nextKey()));  // Optional

  /* "Variables" is similar to condition keys, with three radio buttons at the
   * bottom */
  Q3VGroupBox *varGroup = new Q3VGroupBox("Variables", middleBox);
  varGroup->setAlignment(Qt::AlignHCenter);  // Caption in the middle

  /* varListView replaces varList boxes */
  varListView = new Q3ListView(varGroup);
  varListView->setSelectionMode(Q3ListView::Extended);
  varListView->addColumn("Name");
  varListView->addColumn("Type");
  varListView->setColumnAlignment(1, Qt::AlignHCenter);
  varListView->addColumn("ID");
  varListView->setColumnAlignment(2, Qt::AlignHCenter);
  varListView->addColumn("hidden", 0);
  varListView->setRootIsDecorated(true);
  // Disable automatic column sorting permanently
  varListView->setSortColumn(-1);

  QObject::connect(varListView, SIGNAL(itemRenamed(Q3ListViewItem *, int)),
                   this, SLOT(renameUpdate()));
  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));
  QObject::connect(varListView,
                   SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)),
                   this, SLOT(dbClick(Q3ListViewItem *)));
  // right click signal/slot pair in varListView
  QObject::connect(
      varListView,
      SIGNAL(rightButtonPressed(Q3ListViewItem *, const QPoint &, int)), this,
      SLOT(rightClick()));

  Q3HBox *typeBox = new Q3HBox(varGroup);
  (void)new QLabel("Change Variable Type: ", typeBox);
  typeCombo = new QComboBox(typeBox);
  typeCombo->insertItem("I: Interest");
  typeCombo->insertItem("N: No Interest");
  typeCombo->insertItem("K: Keep No Interest");
  typeCombo->insertItem("D: Dependent");
  // When covariates selected have different types, a blank item will be shown
  typeCombo->insertItem("Reserved");
  QObject::connect(typeCombo, SIGNAL(activated(int)), this,
                   SLOT(setVarType(int)));
  meanAll =
      new QCheckBox("Mean center all but intercept before saving", varGroup);
  meanAll->setChecked(true);
  setCentralWidget(tab3_main);
}

/* Right click on list view box */
void Gdw::rightClick() {
  Q3PopupMenu *rightMenu = new Q3PopupMenu(this);
  Q_CHECK_PTR(rightMenu);
  rightMenu->insertItem("Select all", this, SLOT(selectAll()));
  rightMenu->insertItem("Duplicate", this, SLOT(duplicate()));
  rightMenu->insertItem("Delete", this, SLOT(delCov()));
  rightMenu->insertSeparator();
  rightMenu->insertItem("Convert to delta", this, SLOT(modC2D()));
  rightMenu->insertItem("Convolve", this, SLOT(modConv()));
  rightMenu->insertItem("Mean center", this, SLOT(modMean()));
  rightMenu->insertItem("Mean center non-zero", this, SLOT(meanNonZero()));
  rightMenu->insertItem("Multiply by covar", this, SLOT(modMult()));
  rightMenu->insertItem("Orthogonalize", this, SLOT(modOrth()));
  rightMenu->insertItem("Time shift", this, SLOT(modTS()));
  rightMenu->insertItem("Unit excursion", this, SLOT(UnitExcurs()));
  rightMenu->insertItem("Unit variance", this, SLOT(unitVar()));
  rightMenu->insertSeparator();
  rightMenu->insertItem("Derivative(s)", this, SLOT(modDeriv()));
  rightMenu->insertItem("Eigenvector set", this, SLOT(modES()));
  rightMenu->insertItem("Exponential", this, SLOT(modExpn()));
  rightMenu->insertItem("Finite impulse response", this, SLOT(modFIR()));
  rightMenu->insertItem("Fourier set", this, SLOT(modFS()));
  rightMenu->insertSeparator();

  Q3PopupMenu *evalMenu = new Q3PopupMenu(this);
  rightMenu->insertItem("Evaluate", evalMenu);
  evalMenu->insertItem("Efficiency", this, SLOT(efficiency()));
  evalMenu->insertItem("Mean and SD", this, SLOT(meanSD()));
  evalMenu->insertItem("Noise spectra", this, SLOT(evalNoise()));
  evalMenu->insertSeparator();
  evalMenu->insertItem("Colinearity with all [ I/N/K ]", this, SLOT(colAll()));
  evalMenu->insertItem("Colinearity with interest [ I ]", this, SLOT(col_I()));
  evalMenu->insertItem("Colinearity with non-interest [ N/K ]", this,
                       SLOT(col_N()));
  evalMenu->insertSeparator();
  evalMenu->insertItem("Linear dependence of all [ I/N/K ]", this,
                       SLOT(a_LD()));
  evalMenu->insertItem("Linear dependence of all interest [ I ]", this,
                       SLOT(i_LD()));
  evalMenu->insertItem("Linear dependence of all non-interest [ N/K ]", this,
                       SLOT(nk_LD()));
  evalMenu->insertItem("Linear dependence of selected covariates", this,
                       SLOT(sel_LD()));

  rightMenu->exec(QCursor::pos());
  delete rightMenu;
}

/* This function is to to set up the status bar at the bottom */
void Gdw::setStatusText() {
  if (covList.size())
    statusBar()->message(
        QString("Number of covariate defined: %1").arg(covList.size()));
  else
    statusBar()->message(QString("Status: no covariates defined"));
}

/* reInit() will delete all previous setup and reset the whole interface.
 * It accepts an input condition function flag.
 * When this flag is true, condition function and keyList will also be reset.
 * When it's false, don't reset the condition function.
 * Note that TR isn't reset to be zero here.*/
void Gdw::reInit(bool condFlag) {
  covList.clear();
  // reset the caption for SINGLE mode
  if (mode == SINGLE) {
    gFileName = QString::null;
    this->setCaption("G Design Workshop: New Design");
  }

  tmpResolve = 100;
  timeVector = fftVector = 0;

  magSlider->setValue(1);          // Magnification slider goes back to 1
  timePlotButt->setChecked(true);  // Check "time" radiobutton
  secButt->setChecked(true);       // Check "Second" radiobutton
  psFlag = fitFlag = false;
  fftFlag = 0;  // Default plot is in time domain
  secFlag = 1;  // Default X axis unit is second/Hz
  d_index = -1;
  interceptID.clear();
  initGrpCount();

  varListView->clear();
  statusBar()->message(QString("Status: no covariates defined"));
  typeCombo->setCurrentItem(4);
  // condFlag determines whether condition functions should be reset or not
  if (condFlag) {
    condRef = QString::null;
    condKey.clear();
    condVector = 0;
    keyEditor->clear();  // Clear the condition key line editor
    keyList->clear();
  }

  upperWindow->clear();
  /* Because any text changed in keyEditor and varEditor boxes could set
   * gUpdateFlag to 1, the reinitialization of gUpdateFlag has to be put at the
   * end. Otherwise gUpdateFlag will be always 1 after calling reInit(). */
  gUpdateFlag = false;
}

/* initGrpCount() resets all possible group counters to be 0 */
void Gdw::initGrpCount() {
  contrast_count = 0, diagonal_count = 0;
  trialfx_count = 0, var_len_count = 0;
  scanfx_count = 0, global_count = 0;
  mvpr_count = 0, spike_count = 0, txt_var_count = 0;
}
/* Set up the G Matrix update flag */
void Gdw::setGUpdateFlag(bool inputFlag) {
  gUpdateFlag = inputFlag;
  if (mode == SINGLE) fileSaveAction->setEnabled(inputFlag);
}

/* Set up the mode, 0 is single mode, 1 is COMBO mode with GLM interface */
void Gdw::setMode(int inputMode) {
  if (inputMode == 0)
    mode = SINGLE;
  else
    mode = COMBO;
}

/* Set up TES file list, simply passes the right side listbox in tab2 as an
 * argument */
void Gdw::setTesList(Q3ListBox *inputList) {
  tesList = inputList;
  tesNum = tesList->count();
}

/* Function to calculate the noise model using calcPS() and fitOneOverF
 * functions */
void Gdw::calcNoiseModel() {
  VB_Vector *psVector = calcPS();
  if (psFlag) {
    double var3min = (-1.0) / (totalReps * TR / 1000.0);
    VB_Vector *fitParams = new VB_Vector(8);
    fitParams = fitOneOverF(psVector, var3min, (double)TR);
    if (fitParams->getElement(0) == 1.0) {
      VB_Vector *noiseModel = new VB_Vector(totalReps);
      double var1 = fitParams->getElement(2);
      double var2 = fitParams->getElement(3);
      double var3 = fitParams->getElement(4);
      noiseModel = makeOneOverF(totalReps, var1, var2, var3, (double)TR);

      noiseVec = new VB_Vector(totalReps / 2 + 1);
      for (int i = 0; i < totalReps / 2 + 1; i++)
        noiseVec->setElement(i, noiseModel->getElement(i));
      delete noiseModel;

      double minElement = noiseVec->getMinElement();
      (*noiseVec) += (minElement * (-2));
      double maxElement = noiseVec->getMaxElement();
      noiseVec->scaleInPlace(1.0 / maxElement);
      fitFlag = true;
    } else
      printf("Fitting not successful. No noise model defined.\n");

    delete fitParams;
  }

  delete psVector;
}

/* calcPS() is a function written specially for COMBO mode. It will calculate
 * the power spectrum based on the tes files selected in step 2.
 *
 * First, this function will search *_PS.ref files in the same directory where
 * tes files are located. If any of the corresponding *_PS.ref files isn't
 * found, psFlag will be set false. No noise model will be drawn in freqency
 * mode; If PS files are available for each tes file and they have same number
 * of elements, then these PS vectors will be combined together and an average
 * is calculated as the return value; If all PS files are available but the
 * number of elements is different, then the first PS file will be returned.
 *
 * This implementation is copied from Geoff's IDL code (VoxBo_MatrixDesign.pro,
 * lines 971-1010). */
VB_Vector *Gdw::calcPS() {
  if (TR <= 0 || totalReps <= 0 || tesList->count() <= 0) {
    QMessageBox::warning(0, "Warning!",
                         "Please make sure TR, number of images and tes file(s)\
 are set.<p>No noise model will be built.");
    return 0;
  }
  if (!chkPSstat()) {
    psFlag = false;
    return 0;
  }

  QString tesFileName = tesList->item(0)->text();
  const char *psFileName;
  psFileName = tesFileName.replace(tesFileName.length() - 4, 4, "_PS.ref");
  VB_Vector *initVector = new VB_Vector(psFileName);
  VB_Vector *psVector = new VB_Vector(psFileName);
  unsigned psLength = psVector->getLength();
  int psNum = 1;

  for (int i = 1; i < tesNum; i++) {
    tesFileName = tesList->item(i)->text();
    const char *psFileName;
    psFileName = tesFileName.replace(tesFileName.length() - 4, 4, "_PS.ref");
    VB_Vector tmpVector(psFileName);
    if (psLength != tmpVector.getLength()) {
      psFlag = true;
      return initVector;
    }

    (*psVector) += tmpVector;
    psNum++;
  }
  psFlag = true;
  psVector->scaleInPlace(1.0 / (double)psNum);
  return psVector;
}

/* Read tes file list and check if the corresponding power spectrum file exists
 * or not. Returns true if each tes file has its own ps file, returns false
 * otherwise */
bool Gdw::chkPSstat() {
  QString tesFileName, psFile;
  for (int i = 0; i < tesNum; i++) {
    tesFileName = tesList->item(i)->text();
    // If the TES file name is "my_tes_file.tes", the corresponding PS file
    // should be: "my_tes_file_PS.ref"
    psFile = tesFileName.replace(tesFileName.length() - 4, 4, "_PS.ref");
    if (!chkFileStat(psFile.ascii(), false)) return false;
  }
  return true;
}

/* chkGSstat() is copied from the one above.
 * It reads tes file list and check if the corresponding global signal file
 * exists or not. Returns true if each tes file has its own gs file. Otherwise
 * return false. */
bool Gdw::chkGSstat() {
  QString tesFileName, gsFile;
  for (int i = 0; i < tesNum; i++) {
    tesFileName = tesList->item(i)->text();
    // my_tes_file.tes -> my_tes_file_GS.ref
    gsFile = tesFileName.replace(tesFileName.length() - 4, 4, "_GS.ref");
    if (!chkFileStat(gsFile.ascii(), true)) return false;
  }
  return true;
}

/* This function will return the upsampling rate */
int Gdw::getUpsampling() { return tmpResolve; }

/* Set up TR value (in unit of millisecond) */
void Gdw::setTR(int inputTR) { TR = inputTR; }

/* Set up total number of time points */
void Gdw::setTimePoints(int inputTimePoints) { totalReps = inputTimePoints; }

/* changeTR() is slot to take care of new TR input.
 * reInit(false) is called here, which means TR is completely
 * independent with condition function information. */
void Gdw::changeTR() {
  bool ok;
  int newTR = QInputDialog::getInteger(
      "Enter New TR", "Please enter the new TR", TR, 0, 2147483647, 1, &ok);
  if (ok && (newTR != TR) &&
      (newTR % tmpResolve == 0)) {  // "OK" clicked, and it's a different value
    if (covList.size() > 0 && gUpdateFlag == true) {
      switch (QMessageBox::warning(0, "Warning!",
                                   "You have typed in a new TR.\
<p>Your previous GLM design will be aborted. Do you want to save it now?",
                                   "Yes", "No", "Cancel", 0, 2)) {
        case 0:
          fileSave();
          reInit(false);
          TR = newTR;
          upperWindow->setRatio(TR / tmpResolve);
          trString->setText(QString::number(TR));
          upperWindow->update();
          break;
        case 1:
          reInit(false);
          TR = newTR;
          upperWindow->setRatio(TR / tmpResolve);
          trString->setText(QString::number(TR));
          upperWindow->update();
          break;
        case 2:
          break;  // Nothing happens if "cancel" is clicked.
      }
    } else {
      reInit(false);
      TR = newTR;
      upperWindow->setRatio(TR / tmpResolve);
      trString->setText(QString::number(TR));
      upperWindow->update();
    }
  } else if (ok &&
             TR == newTR)  // "OK" clicked, but the TR isn't really changed
    QMessageBox::warning(0, "Warning", "Same TR, nothing changed.");
  else if (ok &&
           newTR % tmpResolve !=
               0)  // "OK" clicked, but input number isn't a multiple of 100ms
    QMessageBox::critical(0, "Error",
                          "The new TR must be multiple of " +
                              QString::number(tmpResolve) + " ms.");
}

/* changeNumberPoints() is slot to take care of new number of points input.
 * Note that reInit(true) is called here, which means condition function
 * information is always deleted when new number of points is loaded. */
void Gdw::changeNumberPoints() {
  bool ok;
  int newNumber =
      QInputDialog::getInteger("Enter New number of time points",
                               "Please enter the new number of points",
                               totalReps, 0, 2147483647, 1, &ok);

  if (ok && newNumber != totalReps) {
    if (covList.size() > 0 && gUpdateFlag == true)
      switch (
          QMessageBox::warning(0, "Warning!",
                               "You have typed in a new number of time points.\
<p>Your previous GLM design will be aborted. Do you want to save it now?",
                               "Yes", "No", "Cancel", 0, 2)) {
        case 0:
          fileSave();
          reInit(true);
          totalReps = newNumber;
          numberString->setText(QString::number(totalReps));
          upperWindow->update();
          break;
        case 1:
          reInit(true);
          totalReps = newNumber;
          numberString->setText(QString::number(totalReps));
          upperWindow->update();
          break;
        case 2:
          break;
      }
    else {
      reInit(true);
      totalReps = newNumber;
      numberString->setText(QString::number(totalReps));
      upperWindow->update();
    }
  } else if (ok && newNumber == totalReps)
    QMessageBox::warning(0, "Warning", "Same number, nothing changed.");
}

/* This slot is listening to the signal of newCovLoaded and show the new
 * covariate on the upperWindow display */
void Gdw::upperWindowUpdate(VB_Vector *newVector) {
  timeVector = newVector;
  // Downsample the time vector before fft (because Goeff did it too)
  VB_Vector *timeVector_down = downSampling(timeVector, TR / tmpResolve);
  fftVector = fftNyquist(timeVector_down);
  // If "Freq Plot" radio button is selected
  if (fftFlag) {
    upperWindow->setFirstVector(fftVector);
    if (mode == COMBO && fitFlag) {
      QColor noiseColor = Qt::red;
      VB_Vector *newNoiseVec = new VB_Vector(noiseVec);
      newNoiseVec->scaleInPlace(fftVector->getMaxElement());
      upperWindow->addVector(newNoiseVec, noiseColor);
    }

    int lowFreqIndex = getLowFreq(fftVector) - 1;
    if (lowFreqIndex > 1)
      statusBar()->message(
          QString(
              "Covariate info: frequency 0-%1 < 1% power. You can remove %2 \
frequencies safely.")
              .arg(lowFreqIndex)
              .arg(lowFreqIndex));
    else if (lowFreqIndex == 1)
      statusBar()->message(
          QString("Covariate info: frequency 0-1 < 1% power. You can remove 1 \
frequency safely."));
    else if (lowFreqIndex == 0)
      statusBar()->message(QString("Covariate info: frequency 0 < 1% power"));
    else
      statusBar()->message(
          QString("Covariate info: no low frequency cutoff found."));
    // "Freq Plot" + "Secs/Hz" combination
    if (secFlag) {
      upperWindow->setAllNewX(
          0, 1.0 / (2.0 * TR / 1000.0));  // Set unit of x axis to Hz
      upperWindow->setXCaption("Freq (Hz)");
    }
    // "Freq Plot" + "TRs (Freq)" combination
    else {
      upperWindow->setAllNewX(
          0,
          (double)totalReps / 2.0);  // Set unit of x axis to number of images
      upperWindow->setXCaption("Freq (Number)");
    }
  }

  // If "Time Plot" radio button is selected
  else {
    upperWindow->setFirstVector(timeVector);
    // "Time Plot" + "Secs/Hz" combination
    if (secFlag) {
      if (upperWindow->getFirstPlotMode() % 2 == 1)
        upperWindow->setFirstXLength((totalReps - 1) * TR / 1000.0);
      else
        upperWindow->setFirstXLength(totalReps * TR / 1000.0);
      upperWindow->setXCaption("Time (Sec)");
    }
    // "Time Plot" + "TRs (Freq)" combination
    else {
      if (upperWindow->getFirstPlotMode() % 2 == 1)
        upperWindow->setFirstXLength(totalReps - 1);
      else
        upperWindow->setFirstXLength(totalReps);
      upperWindow->setXCaption("Time (Images)");
    }
    // Print out the highlighted covariate's mean and std values
    double vecMean = timeVector_down->getVectorMean();
    double vecSD = sqrt(timeVector_down->getVariance());
    statusBar()->message(
        QString("Covariate info: mean=%1, SD=%2").arg(vecMean).arg(vecSD));
  }

  upperWindow->setYCaption("Magnitude");
  upperWindow->update();
}

/* Slot for "File -> Open" */
void Gdw::fileOpen() {
  if (gUpdateFlag)
    switch (QMessageBox::warning(
        0, "Warning!", "Do you want to save your previous GLM design?", "Yes",
        "No", "Cancel", 0, 2)) {
      case 0:
        fileSave();
        break;
      case 1:
        break;
      case 2:  // Nothing happens if "cancel" is clicked.
        return;
    }
  QString loadedFilename = Q3FileDialog::getOpenFileName(
      QString::null, "G matrix files (*.G)", this, "Open a G matrix file",
      "Choose a G Matrix file to open");
  if (loadedFilename.isEmpty()) return;

  const string s1(loadedFilename.ascii());
  if (chkFileStat(loadedFilename.ascii(), true)) readG(s1);
}

/* readG() reads a certain G file into the upper window and set up other
 * parameters based on the input G matrix file */
void Gdw::readG(const string inputName) {
  pregStat = true;
  if (!chkG(inputName)) return;
  gMatrix = myGInfo.gMatrix;

  // Do NOT overwrite G matrix filename in COMBO mode
  if (mode == SINGLE) gFileName = QString(inputName.c_str());

  reInit(true);
  upperWindow->setRatio(TR / tmpResolve);
  if (myGInfo.condStat) cpCondInfo();

  setGname(inputName);
  if (pregStat) pregStat = chkPreG(inputName);
  buildCovList(pregStat);
  buildTree();
}

/* Set gFilename and interface caption */
void Gdw::setGname(string inputName) {
  if (mode == SINGLE) {
    gFileName = (QString)(inputName.c_str());
    this->setCaption("G Design Workshop: " + gFileName);
  } else
    this->setCaption("G Design Workshop: " + gFileName + ".G");

  // If the filename ends with ".G", truncate ".G" for .preG save purpose
  if (gFileName.right(2) == ".G") gFileName.truncate(gFileName.length() - 2);

  // Because setVarEditor() always sets gUpdateFlag to be true, reset it to be
  // false here.
  if (mode == SINGLE) setGUpdateFlag(false);
  // In COMBO mode, loading a new G matrix file means it is already updated
  else
    setGUpdateFlag(true);
}

/* buildCovList() reads each column of gMatrix or pregMatrix (if available) into
 * covList */
void Gdw::buildCovList(bool pregStat) {
  if (interceptID.size()) interceptID.clear();

  VB_Vector tmpVec;
  int upRatio = TR / tmpResolve;
  for (uint32 i = 0; i < gMatrix.n; i++) {
    // If preG is valid, use it instead
    if (pregStat) {
      tmpVec = pregMatrix.GetColumn(i);
      VB_Vector *newVector = new VB_Vector(tmpVec);
      // record intercept index
      if (newVector->getVariance() <= 1e-15) interceptID.push_back(i);
      covList.push_back(newVector);
    }
    // Otherwise use G file
    else {
      tmpVec = gMatrix.GetColumn(i);
      VB_Vector *downVector = new VB_Vector(tmpVec);
      VB_Vector *newVector = upSampling(downVector, upRatio);
      if (newVector->getVariance() <= 1e-15) interceptID.push_back(i);
      covList.push_back(newVector);
      delete downVector;
    }
  }
}

/* buildTree() builds the varListView tree based on the parameter lines in G
 * matrix file */
void Gdw::buildTree() {
  QString nameStr, sectionStr, typeStr;
  QStringList qList;
  for (int i = 0; i < (int)myGInfo.nameList.size(); i++) {
    nameStr = QString(myGInfo.nameList[i].c_str());
    typeStr = QString(myGInfo.typeList[i].c_str());
    QStringList qList = QStringList::split("->", nameStr);
    Q3ListViewItem *parent = varListView->firstChild();
    for (int j = 0; j < qList.size(); j++) {
      sectionStr = *qList.at(j);
      // Covariate is a direct child of varListView
      if (qList.size() == 1) {
        (void)new Q3ListViewItem(varListView, getLastChild(varListView),
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
          parent = new Q3ListViewItem(varListView, getLastChild(varListView),
                                      sectionStr);
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
  plotCov(0);
  // label intercept covariate(s) in hidden column
  for (unsigned k = 0; k < interceptID.size(); k++) {
    QString tmpStr = QString::number(interceptID[k]);
    varListView->findItem(tmpStr, 2)->setText(3, "intercept");
  }
}

/* searchDepth0() searches the direct child(ren) of varListView and returns the
 * QListViewtem
 * that is a group item and name matches the input QString. Returns 0 if name
 * not found */
Q3ListViewItem *Gdw::searchDepth0(QString grpName) {
  Q3ListViewItem *child = varListView->firstChild();
  while (child) {
    if (child->text(2).isEmpty() && child->text(0) == grpName) return child;
    child = child->nextSibling();
  }
  return 0;
}

/* plotCov() draws a covariate in upperWindow based on the input
 * covariate index and set up related parameters correctly. */
void Gdw::plotCov(unsigned inputIndex) {
  if (!covList.size()) return;
  if (inputIndex >= covList.size()) return;

  Q3ListViewItem *myItem =
      varListView->findItem(QString::number(inputIndex), 2);
  if (!myItem) return;

  QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
                      SLOT(selectionUpdate()));
  selID.clear();
  selID.push_back(inputIndex);
  itemCounter = 1;
  varListView->setSelected(myItem, true);
  varListView->setCurrentItem(myItem);
  int covType = convtType(myItem->text(1));
  typeCombo->setCurrentItem(covType);
  emit(newCovLoaded(covList[inputIndex]));
  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));
}

/* chkG() makes sure the input G file is valid */
bool Gdw::chkG(string inputName) {
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
  if (!cmpSampling(myGInfo.sampling)) return false;
  return true;
}

/* Compare TR in G amtrix with TR defined on the interface */
bool Gdw::cmpTR(int headerTR) {
  // When no TR in G matrix header
  if (headerTR == -1) {
    if (TR)
      QMessageBox::warning(0, "Warning",
                           "No TR information available in G matrix file. \
<br>The original value will be used.");
    else {
      QMessageBox::warning(0, "Warning",
                           "No TR information available. \
<br>The default value of 2000ms is used.");
      TR = 2000;
      trString->setText("2000");
    }
    return true;
  } else if (mode == COMBO && TR != headerTR) {
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
  } else if (mode == SINGLE && TR != headerTR) {
    TR = headerTR;
    trString->setText(QString::number(TR));
  }
  return true;
}

/* Compare totalReps on the interface with totalReps in G matrix header */
bool Gdw::cmpTotalReps(int rowNum) {
  // If number of time points not set yet, set it to be the number of rows in
  // matrix
  if (totalReps == 0) {
    totalReps = rowNum;
    numberString->setText(QString::number(totalReps));
    return true;
  }

  // Inconsistency in combo mode: error message
  if (totalReps != rowNum && mode == COMBO) {
    QMessageBox::critical(
        0, "Error!",
        QString("The number of rows (%1) in G matrix not match \
the original number of time points (%2)")
            .arg(rowNum)
            .arg(totalReps));
    return false;
  }

  // In SINGLE mode, if number of time points doesn't match, give a warning
  // message
  if (totalReps != rowNum && mode == SINGLE) {
    totalReps = rowNum;
    numberString->setText(QString::number(totalReps));
  }
  return true;
}

/* Compare sampling rate in the header with the one defined on the interface */
bool Gdw::cmpSampling(int headerSampling) {
  if (headerSampling == 0) {
    printf(
        "G matrix warning: No upsampling rate information in the matrix file's "
        "header.\n");
    printf("Default value of 100ms is used.\n");
    tmpResolve = 100;
  } else if (tmpResolve != headerSampling)
    tmpResolve = headerSampling;

  if (TR % tmpResolve != 0) {
    QMessageBox::critical(
        0, "Error!",
        QString("TR (%1) is not multiple of sampling rate(%2)")
            .arg(TR)
            .arg(tmpResolve));
    return false;
  }

  // Check condition function length again, because TR or sampling or totalReps
  // might have been changed
  if (myGInfo.condStat) {
    int condLen = myGInfo.condVector->getLength();
    if ((TR / tmpResolve) % (condLen / totalReps) != 0) {
      QString helpStr =
          "G matrix error: Condition function can not be upsampled with "
          "current sampling rate.";
      QMessageBox::critical(0, "Error!", helpStr);
      return false;
    }
  }
  // Pass!
  samplingStr->setText(QString::number(tmpResolve));
  upperWindow->setRatio(TR / tmpResolve);
  return true;
}

/* chkPreG() makes sure the preG file exists and in valid format */
bool Gdw::chkPreG(string inputName) {
  string pregName = inputName;
  int dotPost = pregName.rfind(".");
  pregName.erase(dotPost);
  pregName.append(".preG");

  // Start to read preG file
  if (pregMatrix.ReadHeader(pregName) || pregMatrix.ReadFile(pregName) ||
      !pregMatrix.m || !pregMatrix.n) {
    printf("[E] couldn't read %s as a valid G matrix file\n", pregName.c_str());
    return false;
  }

  gHeaderInfo myPreGInfo = gHeaderInfo(pregMatrix);
  if (!myPreGInfo.chkInfo(true)) return false;

  return cmpG2preG(myGInfo, myPreGInfo);
}

/* Copy condition function name, condition keys and condition function vector
 * from G matrix header */
void Gdw::cpCondInfo() {
  int condLen = myGInfo.condVector->getLength();
  condVector = new VB_Vector(condLen);
  for (int i = 0; i < condLen; i++)
    (*condVector)[i] = (myGInfo.condVector)->getElement(i);

  condKey = tokenlist(myGInfo.condKey);
  condRef = QString(myGInfo.condfxn.c_str());

  for (size_t m = 0; m < condKey.size(); m++) {
    QString keyString = QString(condKey(m));
    keyList->insertItem(keyString);
  }
  keyList->setCurrentItem(0);
}

/* Slot for "File -> Save" */
void Gdw::fileSave() {
  if (!covList.size()) {
    QMessageBox::information(0, "Info", "No covariates defined yet.");
    return;
  }
  if (!chkItemName()) return;
  if (gFileName.isEmpty()) {
    fileSaveAs();
    return;
  }
  if (!gUpdateFlag) {
    QMessageBox::information(0, "Info", "No changes to be saved.");
    return;
  }

  writeG(gFileName.toStdString(), TR, totalReps, tmpResolve, covList,
         varListView, condRef, condKey, meanAll->isChecked());
  setGUpdateFlag(false);
}

/* Slot for "File -> Save As" */
void Gdw::fileSaveAs() {
  if (!covList.size()) {
    QMessageBox::information(0, "Info", "No covariates defined yet.");
    return;
  }
  if (!chkItemName()) return;

  // If input filename is foo, new files generated will be: foo.G and foo.preG
  QString newName = Q3FileDialog::getSaveFileName(
      QString::null, tr("G Matrix Files (*.G);;All Files (*)"), this,
      "Save G Matrix File", "Choose a filename to save the G matrix under: ");
  if (newName.isEmpty()) return;
  if (!newName.endsWith(".G")) newName = newName + ".G";

  QFileInfo *newFileInfo = new QFileInfo(newName);
  QString pathString =
      newFileInfo->dirPath(true);  // Absolute path of the G matrix file
  QFileInfo *newFilePath = new QFileInfo(pathString);
  if (newFileInfo->exists()) {
    switch (QMessageBox::warning(0, "Warning!",
                                 newName + " already exists. Are you sure you "
                                           "want to overwrite this file now?",
                                 "Yes", "No", "Cancel", 0, 2)) {
        // "Yes" overwrites the file
      case 0:
        // Remove ".G" at the tail
        newName.remove(newName.length() - 2, 2);
        // exit if the directory is not writable
        if (!newFilePath->isWritable()) {
          QMessageBox::critical(
              0, "Error!",
              "Not permitted to write file in this directory: " + pathString);
          return;
        }
        writeG(newName.toStdString(), TR, totalReps, tmpResolve, covList,
               varListView, condRef, condKey, meanAll->isChecked());
        if (mode == SINGLE) singleSaveAs(newName);
        break;
      default:  // Do nothing for "No" and "Cancel"
        break;
    }
  } else {
    newName.remove(newName.length() - 2, 2);
    // Is this directory writable?
    if (!newFilePath->isWritable()) {
      QMessageBox::critical(
          0, "Error!",
          "Not permitted to write file in this directory: " + pathString);
      return;
    }
    writeG(newName.toStdString(), TR, totalReps, tmpResolve, covList,
           varListView, condRef, condKey, meanAll->isChecked());
    if (mode == SINGLE) singleSaveAs(newName);
  }
}

/* Function specially written for "File -> Save As" in SINGLE mode */
void Gdw::singleSaveAs(QString &inputName) {
  gFileName = inputName;
  setGUpdateFlag(false);
  this->setCaption("G Design Workshop: " + inputName + ".G");
}

/* chkViewItem() checks the item in varListView to make sure their names do not
 * include "->" and no blank names available.
 * returns true if the names are good, otherwise false */
bool Gdw::chkItemName() {
  QString nameStr;
  bool status = true;
  Q3ListViewItemIterator it(varListView);
  while (it.current()) {
    nameStr = it.current()->text(0);
    if (!nameStr.length()) {
      QMessageBox::critical(
          0, "Error",
          "Blank group/covariate name not allowed in covariate tree");
      status = false;
    } else if (nameStr.contains("->")) {
      QMessageBox::critical(
          0, "Error", "-> not allowed in group/covariate name:<p>" + nameStr);
      status = false;
    } else if (!it.current()->text(2).length() && !chkGrpName(it.current())) {
      QMessageBox::critical(
          0, "Error",
          "Duplicate group names not allowed at same tree level:<p>" + nameStr);
      status = false;
    }
    ++it;
  }
  return status;
}

/* chkGrpName() checkes the input item's name and make sure there are is no
 * duplicate group names at same tree level. Returns true if the input item is
 * not a group or no same group name found. */
bool Gdw::chkGrpName(Q3ListViewItem *inputItem) {
  QString inputName = inputItem->text(0);
  Q3ListViewItem *next = inputItem->nextSibling();
  while (next) {
    if (inputName == next->text(0)) return false;
    next = next->nextSibling();
  }
  return true;
}

/* closeEvent() is an overloaded function called when user clicks "quit" or the
 * "x" button on the upperleft corner. Note that the response will be different
 * for combo and single mode: In SINGLE mode, it will check whether the latest G
 * matrix has already saved on the disk. If yes, close the window; if no, ask
 * the user whether he wants to save it or not. In COMBO mode, since we have
 * already determined the analysis folder's name, simply
 * write the file and emit the signal. */
void Gdw::closeEvent(QCloseEvent *ce) {
  if (!covList.size()) {  // If there is no any covariates defined yet
    switch (QMessageBox::information(0, "Info",
                                     "You haven't created any covariates yet. \
<p>Are you sure you want to quit?",
                                     "Quit", "Don't Quit", QString::null, 0,
                                     1)) {
      case 0:
        ce->accept();
        if (mode == COMBO) emit(cancelSignal(true));
        break;
      case 1:
        ce->ignore();
        break;
    }
  }
  /* If covariate(s) do(es) exist, then in COMBO mode, if the G matrix has been
   * changed, simply save it with the filename determined in tab1. */
  else if (mode == COMBO) {
    emit(doneSignal(covList, varListView, tmpResolve, condRef, condKey,
                    meanAll->isChecked()));
    ce->accept();
  }
  // In SINGLE mode, if the G matrix has been changed, ask the user whether it
  // should be saved
  else if (gUpdateFlag) {
    switch (QMessageBox::information(
        0, "Info",
        "G matrix has been changed since the last time you saved it.<p>\
Do you want to save it first?",
        "Yes", "No", "Cancel", 0, 2)) {
      case 0:  // If yes, save it and close the window
        fileSave();
        ce->accept();
        break;
      case 1:  // If no, simply close the window
        ce->accept();
        break;
      case 2:  // If cancel, do nothing
        ce->ignore();
        break;
    }
  } else  // If in sigle mode and G matrix isn't changed, close the window
    ce->accept();
}

/* This function checks whether both TR and totalReps have been set.
 * Returns true if both are set, otherwise false. */
bool Gdw::chkTR_imgNum() {
  if (TR <= 0) {
    QMessageBox::critical(0, "Error!", "Invalid TR value");
    return false;
  }
  if (totalReps <= 0) {
    QMessageBox::critical(0, "Error!", "Invalid number of time points");
    return false;
  }

  return true;
}

/* This function checks these three parameters: TR, totalReps and condVector.
 * Returns true if all of three are set, otherwise false. */
bool Gdw::chkTR_imgNum_condfx() {
  if (TR <= 0) {
    QMessageBox::critical(0, "Error!", "Invalid TR value");
    return false;
  }
  if (totalReps <= 0) {
    QMessageBox::critical(0, "Error!", "Invalid number of time points");
    return false;
  }

  if (!condVector) {
    QMessageBox::critical(0, "Error!", "Condition function not found");
    return false;
  }

  return true;
}

/* This is a generic function to add contrast(s), the input string is covariate
 * type ("I" or "N") The other two add-contrasts slots for "I" and "N" are
 * wrappers based on this function */
void Gdw::addContrasts(QString inputType) {
  if (!chkTR_imgNum_condfx()) return;

  varType = inputType;
  G_Contrast *myContrast = new G_Contrast(keyList->count(), tmpResolve);
  myContrast->setCaption("Set up Contrasts Options");
  myContrast->setFixedWidth(400);
  myContrast->show();
  this->setDisabled(true);
  QObject::connect(myContrast, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myContrast,
                   SIGNAL(doneSignal(Q3TextEdit *, int, int, bool, VB_Vector *,
                                     double, QString &)),
                   this,
                   SLOT(showContrasts(Q3TextEdit *, int, int, bool, VB_Vector *,
                                      double, QString &)));
}

/* Slot for "Edit -> Add Interest -> Contrast(s)", a wrapper based on the
 * generic one. */
void Gdw::add_I_Contrasts() { addContrasts("I"); }

/* Slot to work with contrast parameters collected from contrast matrix */
void Gdw::showContrasts(Q3TextEdit *matrixText, int scaleFlag, int centerFlag,
                        bool convStat, VB_Vector *convVector, double sampling,
                        QString &tag) {
  QString lineString;
  int lineNum = matrixText->paragraphs();
  double contrastMatrix[keyList->count()];
  QString tmpVarTxt;

  /* Upsample the original condition function */
  int upRatio = (TR / tmpResolve) / (condVector->getLength() / totalReps);
  VB_Vector *condVector_up = upSampling(condVector, upRatio);
  contrast_count++;
  QString groupStr = "contrasts_g" + QString::number(contrast_count);
  Q3ListViewItem *contrastGrp =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  contrastGrp->setOpen(true);
  for (int i = 0; i < lineNum; i++) {
    tmpVarTxt = "";
    lineString = (matrixText->text(i)).simplifyWhiteSpace();
    // Skip blank row in contrast matrix
    if (!lineString.length()) continue;

    // Make new covariate name based on contrast matrix elements
    for (unsigned j = 0; j < keyList->count(); j++) {
      QString element = lineString.section(' ', j, j);
      contrastMatrix[j] = element.toDouble();
      // Exclude the element which is zero
      if (!contrastMatrix[j]) continue;

      tmpVarTxt = tmpVarTxt + element + "*" + keyList->text(j);
      if (j < keyList->count() - 1) tmpVarTxt = tmpVarTxt + " + ";
    }
    // Make sure there is no "+" at the end of the covariate name
    if (tmpVarTxt.endsWith(" + ")) tmpVarTxt.truncate(tmpVarTxt.length() - 3);

    // Build new VB_Vector to represent new contrast covariate
    VB_Vector *contrastVec = new VB_Vector(condVector_up->getLength());
    for (unsigned j = 0; j < condVector_up->getLength(); j++) {
      int n = (int)condVector_up->getElement(j);
      // scale or not?
      if (scaleFlag)
        contrastVec->setElement(j,
                                contrastMatrix[n] / countNum(condVector_up, n));
      else
        contrastVec->setElement(j, contrastMatrix[n]);
    }
    // mean-center or not?
    if (centerFlag) contrastVec->meanCenter();
    // convolution or not?
    if (convStat) {
      contrastVec = new VB_Vector(
          getConv(contrastVec, convVector, (int)sampling, tmpResolve));
      tmpVarTxt = tmpVarTxt + " (" + tag + ")";
    }
    covList.push_back(contrastVec);
    QString idStr = QString::number(covList.size() - 1);
    (void)new Q3ListViewItem(contrastGrp, getLastChild(contrastGrp), tmpVarTxt,
                             varType, idStr);
  }

  loadSingleCov(getLastChild(contrastGrp), convtType(varType));
  delete condVector_up;
}

/* Slot for ""Edit -> Add Interest -> Diagonal set" */
void Gdw::add_I_DS() { addDS("I"); }

/* Slot for ""Edit -> Add NoInterest -> Diagonal set" */
void Gdw::add_N_DS() { addDS("N"); }

/* addDS() is a generation function to add diagonal set covariate(s) */
void Gdw::addDS(QString inputType) {
  if (!chkTR_imgNum_condfx()) return;

  varType = inputType;
  G_DS *myDS = new G_DS(tmpResolve);
  myDS->setFixedWidth(350);
  myDS->setCaption("Set up Diagonal Set Options");
  myDS->show();
  this->setDisabled(true);
  QObject::connect(myDS, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(
      myDS, SIGNAL(doneSignal(int, int, bool, VB_Vector *, double, QString &)),
      this, SLOT(showDS(int, int, bool, VB_Vector *, double, QString &)));
}

/* showDS() adds new covariate(s) into covList and show them on the interface */
void Gdw::showDS(int scaleFlag, int centerFlag, bool convStat,
                 VB_Vector *convVector, double sampling, QString &tag) {
  int upRatio = (TR / tmpResolve) / (condVector->getLength() / totalReps);
  VB_Vector *condVector_up = upSampling(condVector, upRatio);
  diagonal_count++;
  QString groupStr = "diagonal_g" + QString::number(diagonal_count);
  Q3ListViewItem *diagonalGrp =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  diagonalGrp->setOpen(true);
  int upLength = condVector_up->getLength();
  int rowNum = keyList->count() - 1;
  double diagMatrix[keyList->count()];
  QString tmpVarTxt;

  for (int i = 0; i < rowNum; i++) {
    tmpVarTxt = "";
    for (unsigned j = 0; j < keyList->count(); j++) {
      if (j - i == 1)
        diagMatrix[j] = 1.0;
      else
        diagMatrix[j] = 0;
    }

    VB_Vector *diagVec = new VB_Vector(upLength);
    tmpVarTxt = keyList->text(i + 1);
    for (unsigned j = 0; j < condVector_up->getLength(); j++) {
      double m = condVector_up->getElement(j);
      int n = (int)m;
      // scale or not?
      if (scaleFlag)
        diagVec->setElement(j, diagMatrix[n] / countNum(condVector_up, n));
      else
        diagVec->setElement(j, diagMatrix[n]);
    }
    // mean-center or not?
    if (centerFlag) diagVec->meanCenter();
    // convolution or not?
    if (convStat) {
      diagVec = new VB_Vector(
          getConv(diagVec, convVector, (int)sampling, tmpResolve));
      tmpVarTxt = tmpVarTxt + " (" + tag + ")";
    }
    covList.push_back(diagVec);
    QString idStr = QString::number(covList.size() - 1);
    (void)new Q3ListViewItem(diagonalGrp, getLastChild(diagonalGrp), tmpVarTxt,
                             varType, idStr);
  }

  loadSingleCov(getLastChild(diagonalGrp), convtType(varType));
  delete condVector_up;
}

/* This is a generic function to add single covariate,
 * the input string is covariate type ("I" or "N");
 * The other two add-single-covariate slots for interest and
 * no interest are simply wrappers based on this function. */
void Gdw::addSingle(QString inputType) {
  if (!chkTR_imgNum()) return;

  // Open a FileDialog window to select ref file
  QString tmpString = QFileDialog::getOpenFileName(
      this, "Add Covariate", "",
      "Vector and Matrix files (*.ref *.mtx *.mat);;All files (*)");
  if (!tmpString.length()) return;

  VB_Vector tmpVector;
  VBMatrix mymat;
  string cname;
  string infile = tmpString.toStdString();
  if (!tmpVector.ReadFile(infile)) {
    int refLength =
        tmpVector.getLength();  // Get the number of points in REF file
    /* If the number of points in REF file isn't a multiple of the concatenated
     * total number of TES files, pop out an error message and ask the user to
     * try again. */
    if (refLength % totalReps != 0) {
      QMessageBox::critical(0, "Error!",
                            "Number of elements in REF function must be a "
                            "multiple of total number of time points.");
      return;
    } else if ((TR / tmpResolve) % (refLength / totalReps) != 0) {
      QString helpStr;
      if (TR % (refLength / totalReps) == 0) {
        int maxSamp = TR / (refLength / totalReps);
        helpStr =
            "Reference function can NOT be upsampled with the current sampling rate.\
<p>Click <b>Tools</b> to reset the upsampling rate.                     \
<p>Recommended value: " +
            QString::number(maxSamp) + "/N (N is positive integer)";
      } else
        helpStr =
            "Invalid reference function: can not be upsampled with the current "
            "sampling rate.";

      QMessageBox::critical(0, "Error!", helpStr);
      return;
    }

    int upRatio = (TR / tmpResolve) / (refLength / totalReps);
    VB_Vector *newVector = upSampling(&tmpVector, upRatio);
    covList.push_back(newVector);
    QString covID = QString::number(covList.size() - 1);
    cname = xfilename(infile);
    Q3ListViewItem *newItem =
        new Q3ListViewItem(varListView, getLastChild(varListView),
                           cname.c_str(), inputType, covID);
    loadSingleCov(newItem, convtType(inputType));
  } else if (!mymat.ReadFile(infile)) {
    int refLength = mymat.rows;
    if (refLength % totalReps != 0) {
      QMessageBox::critical(0, "Error!",
                            "Number of elements in your covariate file must be "
                            "a multiple of total number of time points.");
      return;
    } else if ((TR / tmpResolve) % (refLength / totalReps) != 0) {
      QString helpStr;
      if (TR % (refLength / totalReps) == 0) {
        int maxSamp = TR / (refLength / totalReps);
        helpStr =
            "Reference function can NOT be upsampled with the current sampling rate.\
<p>Click <b>Tools</b> to reset the upsampling rate.                     \
<p>Recommended value: " +
            QString::number(maxSamp) + "/N (N is positive integer)";
      } else
        helpStr =
            "Invalid reference function: can not be upsampled with the current "
            "sampling rate.";

      QMessageBox::critical(0, "Error!", helpStr);
      return;
    }

    int upRatio = (TR / tmpResolve) / (refLength / totalReps);
    for (uint32 i = 0; i < mymat.cols; i++) {
      VB_Vector myvec = mymat.GetColumn(i);
      VB_Vector *newVector = upSampling(&myvec, upRatio);
      covList.push_back(newVector);
      QString covID = QString::number(covList.size() - 1);
      cname = (format("%s_%d") % xfilename(infile) % i).str();
      Q3ListViewItem *newItem =
          new Q3ListViewItem(varListView, getLastChild(varListView),
                             cname.c_str(), inputType, covID);
      loadSingleCov(newItem, convtType(inputType));
    }
  } else {
    QMessageBox::critical(0, "Error!",
                          "Couldn't read your selected file either as a vector "
                          "or a Matrix file.");
  }
}

/* Slot for "Edit -> Add Interest -> Single covariate", a wrapper based on the
 * generic one. */
void Gdw::add_I_Single() { addSingle("I"); }

/* addTrialFx() is a generic function which adds trial effects of fixed value *
 * It accepts an argument which is the variable type
 * ("I" for interest, "N" for no interest or "K" for keep no interest) */
void Gdw::addTrialFx(QString inputType) {
  if (!chkTR_imgNum()) return;

  bool ok;
  int trialLength = QInputDialog::getInteger("Trial Effects Options",
                                             "Trial length (secs): ", 1, 1,
                                             TR * totalReps, 1, &ok, this);
  if (!ok) return;

  int numTrials = TR * totalReps / (trialLength * 1000) - 1;
  // Make sure trialfx group will have at least 1 child covaraite
  if (!numTrials) {
    QMessageBox::critical(
        0, "Error",
        "No trial effect covariates can be defined with a trial length of ." +
            QString::number(trialLength));
    return;
  }

  int unitLength = trialLength * 1000 / tmpResolve;
  trialfx_count++;
  QString groupStr = "trialfx_g" + QString::number(trialfx_count);
  Q3ListViewItem *trialfx =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  trialfx->setOpen(true);
  for (int i = 0; i < numTrials; i++) {
    VB_Vector *newCov = new VB_Vector(TR * totalReps / tmpResolve);
    newCov->setAll(0);
    for (int j = i * unitLength; j < (i + 1) * unitLength; j++) {
      newCov->setElement(j, 1.0);
    }
    newCov->meanCenter();
    covList.push_back(newCov);
    QString idStr = QString::number(covList.size() - 1);
    QString tmpVarTxt = "Trial effect " + QString::number(i + 1);
    (void)new Q3ListViewItem(trialfx, getLastChild(trialfx), tmpVarTxt,
                             inputType, idStr);
  }

  loadSingleCov(getLastChild(trialfx), convtType(inputType));
}

/* Slot for "Edit -> Add Interest -> Trial Effects" */
void Gdw::add_I_Trial() { addTrialFx("I"); }

/* addVarTrial() is a generic function to add trial effects from a certain ref
 * file. It will pop out a window to ask the user to pick up the ref file which
 * includes the trial length. As usual, this function accepts an argument to
 * lable the covariate's type [ I/N/K ]. */
void Gdw::addVarTrial(QString inputType) {
  if (!chkTR_imgNum()) return;

  QString trialFile = Q3FileDialog::getOpenFileName(
      QString::null, "Ref files (*.ref)", this, "Add Single Covariate",
      "Please choose the ref function");
  if (!trialFile.length()) return;

  VB_Vector trialVec;
  if (trialVec.ReadFile(trialFile.ascii())) {
    QMessageBox::critical(0, "Error", "Invalid ref file format: " + trialFile);
    return;
  }

  int trialSum = (int)trialVec.getVectorSum();
  if (trialSum != totalReps * TR / 1000) {
    QMessageBox::critical(
        0, "Error",
        "The sum of elements in " + trialFile +
            " isn't equal to the actual length. <p>Please try another file.");
    return;
  }

  if (trialVec.getLength() == 1) {
    QMessageBox::critical(
        0, "Error",
        trialFile +
            " has only line. <p>Trial effect covaraite can NOT be defined. \
<p>Please try another file.");
    return;
  }

  int trialOffset = 0;
  var_len_count++;
  QString groupStr = "var_trialfx_g" + QString::number(var_len_count);
  Q3ListViewItem *trialfx =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  trialfx->setOpen(true);
  for (unsigned i = 0; i < trialVec.getLength() - 1; i++) {
    VB_Vector *newCov = new VB_Vector(TR * totalReps / tmpResolve);
    newCov->setAll(0);
    int nonZeroUnit = (int)trialVec.getElement(i) * 1000 / tmpResolve;
    for (int j = trialOffset; j < trialOffset + nonZeroUnit; j++) {
      newCov->setElement(j, 1.0);
    }
    newCov->meanCenter();
    covList.push_back(newCov);
    QString idStr = QString::number(covList.size() - 1);
    QString tmpVarTxt = "Trial effect " + QString::number(i + 1);
    (void)new Q3ListViewItem(trialfx, getLastChild(trialfx), tmpVarTxt,
                             inputType, idStr);
    trialOffset += nonZeroUnit;
  }

  loadSingleCov(getLastChild(trialfx),
                convtType(inputType));  // Highlight the last new item
}

/* Slot for "Add -> Interest -> Var Length Trial fx" */
void Gdw::add_I_varTrial() { addVarTrial("I"); }

/* Slot for "Add -> No interest -> Intercept" */
void Gdw::slot_addIntercept() {
  // Make sure TR and totalReps are valid
  if (!chkTR_imgNum()) return;
  // Only one intercept covariate is allowed
  if (interceptID.size()) {
    QMessageBox::critical(0, "Error!", "Intercept covariate already defined");
    return;
  }

  addIntercept();
  QString idStr = QString::number(covList.size() - 1);
  loadSingleCov(varListView->findItem(idStr, 2), 2);
}

/* This is a generic function to add intercept.
 * It is also called by block and paired t-test models. */
void Gdw::addIntercept() {
  VB_Vector *newVector = new VB_Vector(totalReps * TR / tmpResolve);
  newVector->setAll(1.0);
  covList.push_back(newVector);
  QString idStr = QString::number(covList.size() - 1);
  (void)new Q3ListViewItem(varListView, getLastChild(varListView), "Intercept",
                           "K", idStr, "intercept");
  varListView->hideColumn(3);
  interceptID.push_back(covList.size() - 1);
}

/*  Slot for "Edit -> Add No interest -> Trial Effects" */
void Gdw::add_N_Trial() { addTrialFx("N"); }

/* Slot for "Edit -> Add No interest -> Var Length Trial fx" */
void Gdw::add_N_varTrial() { addVarTrial("N"); }

/* Slot for "Edit -> Add No interest -> Scan effects" */
/* Question: In SINGLE mode, when the user clicked "OK" but the input is
 * invalid, how can I keep the dialog box?
 * A possible solution is to write a new widget instead of using QT's
 * QInputDialog class. */
void Gdw::addScanfx() {
  if (mode == SINGLE)
    singleScanfx();
  else
    comboScanfx();
}

/* singleScanfx() adds scan effect cavariate in single mode */
void Gdw::singleScanfx() {
  if (!chkTR_imgNum()) return;

  bool ok = false;
  QString text = QInputDialog::getText(
      "Set up scan effects parameters",
      "Please type in the number of scans and number of TRs \
in each scan separated by a \",\". <p>For example: \"2: 100, 200\" means there are two scans. \
The first scan includes 100 TRs and the second 200 TRs.",
      QLineEdit::Normal, QString::null, &ok, this);
  if (!ok) return;
  if (text.isEmpty()) {
    QMessageBox::critical(0, "Error!", "No input found.");
    return;
  }

  text = text.simplifyWhiteSpace();
  int colonPos = text.find(":", 0);
  if (colonPos == -1) {
    QMessageBox::critical(
        0, "Error!",
        "Number of scans not found. Please type in the numbers again.");
    return;
  }

  bool scanNumConvert = false;
  int scanNum = (text.section(":", 0, 0)).toInt(&scanNumConvert, 10);
  if (!scanNumConvert) {
    QMessageBox::critical(
        0, "Error!",
        "Invalid input for number of scans. Please type in the number again.");
    return;
  }

  int trArray[scanNum];
  int totalTR = 0;
  text.remove(
      0, colonPos + 1);  // Removes the number of scan section (includes ":")
  bool trNumConvert = false;

  for (int i = 0; i < scanNum; i++) {
    trArray[i] = (text.section(",", i, i)).toInt(&trNumConvert, 10);
    if (!trNumConvert) {
      QMessageBox::critical(
          0, "Error!",
          "The input number's format is incorrect. Please type it again.");
      return;
    }
    totalTR += trArray[i];
  }
  if (totalTR != totalReps) {
    QMessageBox::critical(0, "Error",
                          "The total number of TRs you typed in is unequal to \
the original number of time points. Please type in your number again.");
    return;
  }

  int offset = 0;
  scanfx_count++;
  QString groupStr = "scanfx_g" + QString::number(scanfx_count);
  Q3ListViewItem *scanfx =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  scanfx->setOpen(true);
  for (int i = 0; i < scanNum - 1; i++) {
    VB_Vector *newVector = new VB_Vector(totalReps * TR / tmpResolve);
    int singleLength = trArray[i] * TR / tmpResolve;

    for (int j = 0; j < totalReps * TR / tmpResolve; j++) {
      if (j < offset)
        newVector->setElement(j, 0);
      else if (j < singleLength + offset)
        newVector->setElement(j, 1.0);
      else
        newVector->setElement(j, 0);
    }
    offset += singleLength;
    newVector->meanCenter();
    covList.push_back(newVector);
    QString idStr = QString::number(covList.size() - 1);
    QString tmpVarTxt = "Scan Effect " + QString::number(i + 1);
    (void)new Q3ListViewItem(scanfx, getLastChild(scanfx), tmpVarTxt, "N",
                             idStr);
  }
  loadSingleCov(getLastChild(scanfx), 1);
}

/* comboScanfx() adds scan effect covariates in combo mode */
void Gdw::comboScanfx() {
  if (tesNum == 0) {
    QMessageBox::critical(0, "Error", "No TES files defined yet.");
    return;
  }
  if (tesNum == 1) {
    QMessageBox::critical(0, "Error",
                          "Only one TES file found. No scan effects defined.");
    return;
  }

  Tes tmpTes;
  int offset = 0;
  scanfx_count++;
  QString groupStr = "scanfx_g" + QString::number(scanfx_count);
  Q3ListViewItem *scanfx =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  scanfx->setOpen(true);
  for (int i = 0; i < tesNum - 1; i++) {
    const char *tmpFileName = tesList->item(i)->text();
    string selectedFiles(tmpFileName);
    tmpTes.ReadFile(selectedFiles);
    int singleLength = tmpTes.dimt * TR / tmpResolve;
    VB_Vector *newVector = new VB_Vector(totalReps * TR / tmpResolve);

    for (int j = 0; j < totalReps * TR / tmpResolve; j++) {
      if (j < offset)
        newVector->setElement(j, 0);
      else if (j < singleLength + offset)
        newVector->setElement(j, 1.0);
      else
        newVector->setElement(j, 0);
    }

    newVector->meanCenter();
    covList.push_back(newVector);
    QString tmpVarTxt = "Scan Effect " + QString::number(i + 1);
    QString idStr = QString::number(covList.size() - 1);
    (void)new Q3ListViewItem(scanfx, getLastChild(scanfx), tmpVarTxt, "N",
                             idStr);
    offset += singleLength;
  }
  loadSingleCov(getLastChild(scanfx), 1);
}

/* Slot for "Edit -> Add No interest -> Global signals" */
void Gdw::addGlobal() {
  if (mode == SINGLE) {
    QMessageBox::information(
        0, "Information",
        "Please use GLM interface to define TES files first.");
    return;
  }

  // First make sure each tes file has its own gloabl signal file, if not, close
  // the function
  if (!chkGSstat()) return;

  // Assume that the global signal files are in the same directory as the TES
  // files
  QString tesFileName;
  VB_Vector *tmpVector = new VB_Vector();  // This vector will hold data read
                                           // from a certain GS file
  int tmpLength;
  int startPos = 0;
  global_count++;
  QString groupStr = "global_signal_g" + QString::number(global_count);
  Q3ListViewItem *gSignal =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  gSignal->setOpen(true);
  for (int i = 0; i < tesNum; i++) {
    VB_Vector *newVector = new VB_Vector(totalReps);
    newVector->setAll(0);
    tesFileName = tesList->item(i)->text();
    // If the TES file name is "my_tes_file.tes", the corresponding GS file
    // should be: "my_tes_file_GS.ref"
    const char *gsFileName;
    gsFileName = tesFileName.replace(tesFileName.length() - 4, 4, "_GS.ref");
    if (tmpVector->ReadFile(gsFileName)) {
      QMessageBox::critical(0, "Error",
                            "Invalid ref file format: " + QString(gsFileName));
      delete tmpVector;
      return;
    }

    tmpVector->meanCenter();  // Geoff did it, so do I
    tmpLength = tmpVector->getLength();

    for (int j = startPos; j < tmpLength + startPos;
         j++)  // Then assign some elements to be the value in GS file
      newVector->setElement(j, tmpVector->getElement(j - startPos));

    VB_Vector *newVector_up = upSampling(newVector, TR / tmpResolve);
    covList.push_back(newVector_up);
    QString idStr = QString::number(covList.size() - 1);
    QString tmpVarTxt = "Global Signal " + QString::number(i + 1);
    (void)new Q3ListViewItem(gSignal, getLastChild(gSignal), tmpVarTxt, "N",
                             idStr);

    startPos += tmpLength;
    delete newVector;
  }

  loadSingleCov(getLastChild(gSignal), 1);
  delete tmpVector;
}

/* This is a function to collect movement parameter files
 * for each tes file before adding covariates */
tokenlist Gdw::getMPFiles() {
  tokenlist mpFiles = tokenlist();
  QString tesFileName, mpFileName;
  for (int i = 0; i < tesNum; i++) {
    tesFileName = tesList->item(i)->text();
    mpFileName = tesFileName.left(tesFileName.length() - 4) + "_MoveParams.ref";

    QFileInfo newFileInfo = QFileInfo(mpFileName);
    // If the default movement parameter filename not found, ask user to choose
    // one
    if (!newFileInfo.exists()) {
      mpFileName = Q3FileDialog::getOpenFileName(
          QString::null, "Movement parameter files (*.ref)", this,
          "Open a movement parameter file",
          "Choose a movement parameter file for " + tesFileName);

      if (mpFileName.isEmpty()) {
        QMessageBox::information(0, "Info",
                                 "Movement parameter file for " + tesFileName +
                                     " not available. No covariates added.");
        mpFiles.clear();
        return mpFiles;
      }
    }
    mpFiles.Add(mpFileName.ascii());
  }

  return mpFiles;
}

/* Slot for "Edit -> Add No interest -> Movement params" */
void Gdw::addMovement() {
  if (mode == SINGLE) {
    QMessageBox::information(
        0, "Information",
        "Please use GLM interface to define TES files first.");
    return;
  }

  tokenlist mpFiles = getMPFiles();
  if (!mpFiles.size()) return;

  int mpLength, unitLength;
  int startPost = 0;
  mvpr_count++;
  for (int i = 0; i < tesNum; i++) {
    const char *mpFileName = mpFiles(i);
    VB_Vector *mpTotal = new VB_Vector(mpFileName);
    mpLength = mpTotal->getLength();
    unitLength = mpLength / 7;
    QString groupStr = "move_param_g" + QString::number(mvpr_count) + " set #" +
                       QString::number(i + 1);
    Q3ListViewItem *mpGrp =
        new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
    mpGrp->setOpen(true);
    for (int j = 0; j < 6; j++) {
      VB_Vector *newVector = new VB_Vector(totalReps);
      newVector->setAll(0);
      // Read 1/7 of the total movement parameter vector into moveinfo and mean
      // center it
      VB_Vector *moveInfo = new VB_Vector(unitLength);
      double moveVal;
      for (int k = 0; k < unitLength; k++) {
        moveVal = mpTotal->getElement(k * 7 + j);
        moveInfo->setElement(k, moveVal);
      }
      moveInfo->meanCenter();

      for (int k = 0; k < unitLength; k++)
        newVector->setElement(k + startPost, moveInfo->getElement(k));
      delete moveInfo;

      VB_Vector *newVector_up = upSampling(newVector, TR / tmpResolve);
      covList.push_back(newVector_up);
      QString idStr = QString::number(covList.size() - 1);
      QString tmpVarTxt;
      switch (j) {
        case 0:
          tmpVarTxt = "X";
          break;
        case 1:
          tmpVarTxt = "Y";
          break;
        case 2:
          tmpVarTxt = "Z";
          break;
        case 3:
          tmpVarTxt = "Pitch";
          break;
        case 4:
          tmpVarTxt = "Roll";
          break;
        case 5:
          tmpVarTxt = "Yaw";
          break;
      }
      (void)new Q3ListViewItem(mpGrp, getLastChild(mpGrp), tmpVarTxt, "N",
                               idStr);
      delete newVector;
    }
    startPost += unitLength;
    delete mpTotal;
  }

  loadSingleCov(varListView->lastItem(), 1);
}

/* Slot for "Edit -> Add No interest -> Single covariate", a wrapper based on
 * the generic one */
void Gdw::add_N_Single() { addSingle("N"); }

/* Slot for "Edit -> Add No interest -> Contrast(s)", a wrapper based on the
 * generic one */
void Gdw::add_N_Contrasts() { addContrasts("N"); }

/* Slot for "Add No interest -> Spike" */
void Gdw::addSpike() {
  if (!chkTR_imgNum()) return;

  bool ok = false;
  QString text = QInputDialog::getText(
      "Define spike position",
      "Please type in spike covariate's position in unit of TR. \
<br>It should be a series of integers separated by <b>,</b> or <b>:</b>. \
<p>A good example will be: \"20, 30, 98-100\". \
<br>Here \"98-100\" is equivalent to \"98, 99, 100\".\
<p>Note that spike's position starts from <b>0</b> instead of 1.",
      QLineEdit::Normal, QString::null, &ok, this);
  if (!ok) return;

  if (text.isEmpty()) {
    QMessageBox::critical(0, "Error!", "No input found.");
    return;
  }

  vector<int> spike = numberlist((const string)text.ascii());
  if (!chkSpike(spike)) return;

  VB_Vector tmpVec(totalReps);
  int spikeIndex;
  spike_count++;
  QString groupStr = "spike_g" + QString::number(spike_count);
  Q3ListViewItem *spikeGrp =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  spikeGrp->setOpen(true);
  for (int i = 0; i < (int)spike.size(); i++) {
    tmpVec.setAll(0);
    spikeIndex = spike[i];
    tmpVec[spikeIndex] = 1.0;
    tmpVec.meanCenter();  // mean center by default
    covList.push_back(upSampling(&tmpVec, TR / tmpResolve));
    QString idStr = QString::number(covList.size() - 1);
    (void)new Q3ListViewItem(spikeGrp, getLastChild(spikeGrp),
                             QString::number(spikeIndex), "N", idStr);
  }

  loadSingleCov(getLastChild(spikeGrp), 1);  // Highlight the new item
}

/* chkSpike() is a function to check the spike string to make sure it is valid
 */
bool Gdw::chkSpike(vector<int> inputSpike) {
  if (!inputSpike.size()) {
    QMessageBox::critical(0, "Error", "Invalid spike position");
    return false;
  }

  for (int i = 0; i < (int)inputSpike.size(); i++) {
    if (inputSpike[i] < 0 || inputSpike[i] >= totalReps) {
      QMessageBox::critical(
          0, "Error",
          "Spike position out of range:" + QString::number(inputSpike[i]));
      return false;
    }
  }
  return true;
}

/* "Add No interest -> Txt file as multi-covariate" */
void Gdw::addTxt() {
  if (!chkTR_imgNum()) return;

  // Open a FileDialog window to select txt file
  QString tmpString = Q3FileDialog::getOpenFileName(
      QString::null, "Txt files (*.txt)", this,
      "Add Multiple Covariates from Txt File", "Please choose the txt file.");
  if (!tmpString.length()) return;

  const char *txtFile =
      tmpString;  // Convert the input filename from QString to const char *
  int txtColNum = getTxtColNum(txtFile);
  int txtRowNum = getTxtRowNum(txtFile);

  if (txtColNum < 0) {
    QMessageBox::critical(0, "Error", "File NOT readable: " + tmpString);
    return;
  }
  if (txtRowNum == 0) {
    QMessageBox::critical(0, "Error", "No uncommented lines in: " + tmpString);
    return;
  }
  if (txtRowNum != totalReps) {
    QMessageBox::critical(0, "Error",
                          "Number of elements in txt file doesn't match the "
                          "total number of time points");
    return;
  }

  std::vector<VB_Vector *> txtCov;
  for (int i = 0; i < txtColNum; i++) {
    VB_Vector *txtVec = new VB_Vector(txtRowNum);
    txtVec->setAll(0);
    txtCov.push_back(txtVec);
  }
  int readStat = readTxt(txtFile, txtCov);
  if (readStat == 1) {
    QMessageBox::critical(0, "Error",
                          "input file doesn't have exactly the same number of "
                          "elements in all rows");
    return;
  }

  int upRatio = (TR / tmpResolve) / (txtRowNum / totalReps);
  txt_var_count++;
  QString groupStr = "txt_var_g" + QString::number(txt_var_count);
  Q3ListViewItem *txtVarGrp =
      new Q3ListViewItem(varListView, getLastChild(varListView), groupStr);
  txtVarGrp->setOpen(true);
  for (int k = 0; k < txtColNum; k++) {
    VB_Vector *newVector = upSampling(txtCov[k], upRatio);
    covList.push_back(newVector);
    QString idStr = QString::number(covList.size() - 1);
    QString tmpVarTxt = "col #" + QString::number(k + 1);
    (void)new Q3ListViewItem(txtVarGrp, getLastChild(txtVarGrp), tmpVarTxt, "N",
                             idStr);
  }
  txtCov.clear();
  loadSingleCov(getLastChild(txtVarGrp), 1);  // Highlight the new item
}

/* Slot for "Edit -> Select All" */
void Gdw::selectAll() { varListView->selectAll(true); }

/* Slot for "Modify Selection(s) -> Duplicate"
 * Note that because covList is a vector of VB_Vector pointers, we cannot simply
 * push_back() the currently highlighted element in the vector to duplicate it.
 * This way the new element will always be equal to the original element, even
 * after the user has manipulated the original or new covariate (eg. convolve,
 * mean center, etc.) So another brandnew VB_Vector object is declared and put
 * at the end of covariateList. */
void Gdw::duplicate() {
  if (!chkSel()) return;

  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed:\
<p>More than one dependent covariate would exist after it");
    return;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    cpItem(it.current());
    ++it;
  }
  setGUpdateFlag(true);
}

/* cpItem() will recursively copy the input listViewItem, written for covariate
 * duplication orgDepth is input item's original depth */
void Gdw::cpItem(Q3ListViewItem *inputItem) {
  if (inputItem->text(2).isEmpty())
    cpGrp(inputItem);
  else
    cpCov(inputItem);
}

/* cpGrp() only add the new group item as the last child of parentItem.
 * None of its child will be copied */
void Gdw::cpGrp(Q3ListViewItem *inputItem) {
  // If the group item is a direct child of varListView, the new group name will
  // be: <original_name>_cp
  if (inputItem->depth() == 0) {
    QString tmpStr = inputItem->text(0) + "_cp";
    (void)new Q3ListViewItem(varListView, getLastChild(varListView), tmpStr);
    return;
  }

  // If inputItem's parent is not selected, insert inputItem as a direct child
  // of varListView
  if (!inputItem->parent()->isSelected()) {
    (void)new Q3ListViewItem(varListView, getLastChild(varListView),
                             inputItem->text(0));
    return;
  }

  (void)new Q3ListViewItem(findParent(inputItem),
                           getLastChild(findParent(inputItem)),
                           inputItem->text(0));
}

/* cpCov() will insert newItem as the last child of parentItem
 * and push back the corresponding vb_vector into covList */
void Gdw::cpCov(Q3ListViewItem *inputItem) {
  int covID = inputItem->text(2).toInt();
  VB_Vector *newVec = new VB_Vector(covList[covID]);
  covList.push_back(newVec);
  QString idStr = QString::number(covList.size() - 1);

  if (inputItem->depth() == 0 || !inputItem->parent()->isSelected()) {
    (void)new Q3ListViewItem(varListView, getLastChild(varListView),
                             inputItem->text(0), inputItem->text(1), idStr);
    return;
  }

  (void)new Q3ListViewItem(findParent(inputItem),
                           getLastChild(findParent(inputItem)),
                           inputItem->text(0), inputItem->text(1), idStr);
}

/* findParent() returns inputItem's parent item when it is
 * inserted into the tree as a new item */
Q3ListViewItem *Gdw::findParent(Q3ListViewItem *inputItem) {
  int lastDepth = varListView->lastItem()->depth();
  int inputDepth = getNewDepth(inputItem);
  if (lastDepth < inputDepth) return varListView->lastItem();

  Q3ListViewItem *newParent = varListView->lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

/* getNewDepth() returns the input item's new depth in the tree after it is
 * copied. It checks if its direct parent is selected or not. If yes, check its
 * parent's parent, until it reached the point that one of its grandparent is
 * not selected. The new depth is equal to the number of checks */
int Gdw::getNewDepth(Q3ListViewItem *inputItem) {
  int newDepth = 0;
  Q3ListViewItem *itemParent = inputItem->parent();
  while (itemParent) {
    if (itemParent->isSelected()) newDepth++;
    itemParent = itemParent->parent();
  }
  return newDepth;
}

/* Slot for "Edit -> Modify Selection(s) -> Delete" */
void Gdw::delCov() {
  if (!varListView->childCount()) {
    QMessageBox::critical(0, "Error", "No covariate defined yet.");
    return;
  }

  // Make sure there is at least one selected item
  if (!itemCounter) {
    QMessageBox::critical(0, "Error", "No covariate selected yet.");
    return;
  }

  if (selID.size() == covList.size())
    delAll();
  else
    delPart();
}

/* delAll will first ask user's confirmation to delete all covariates */
void Gdw::delAll() {
  switch (QMessageBox::warning(
      0, "Warning!", "Are you sure you want to delete all covariates?", "Yes",
      "Cancel", 0, 0, 1)) {
    case 0:
      covList.clear();
      timeVector = fftVector = 0;
      d_index = -1;
      interceptID.clear();
      varListView->clearSelection();
      varListView->clear();
      typeCombo->setCurrentItem(4);
      upperWindow->clear();
      upperWindow->update();
      initGrpCount();
      setGUpdateFlag(false);
      statusBar()->message(QString("Status: no covariates defined"));
      break;
    case 1:
      break;
  }
}

/* delPart() will delete selected covariates/groups and update the index.
 * Note that a group item is deleted only when all of its direct/indirect
 * children are selected. If not the group item will be kept with its other
 * survived child(ren). */
void Gdw::delPart() {
  // If D type is involved, reset d_index
  if (chkD()) d_index = -1;

  QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
                      SLOT(selectionUpdate()));
  Q3PtrList<Q3ListViewItem> lst;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    lst.append(it.current());
    ++it;
  }

  varListView->clearSelection();
  int selSize = lst.count();
  for (int i = 0; i < selSize; i++) {
    Q3ListViewItem *last = lst.last();
    lst.remove(lst.last());
    if (!last->text(2).isEmpty())
      delete last;
    else if (chkGrpSel(last))
      delete last;
  }

  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));
  updateID();
  updateInterceptID();
  setGUpdateFlag(true);
}

/* chkGrpSel() checks whether the input group item's direct/indirect children
 * are all selected or not.
 * Returns true if all selected, false otherwise */
bool Gdw::chkGrpSel(Q3ListViewItem *inputGrp) {
  Q3ListViewItem *myChild = inputGrp->firstChild();
  while (myChild) {
    if (!myChild->isSelected()) return false;
    if (myChild->childCount() && !chkGrpSel(myChild)) return false;
    myChild = myChild->nextSibling();
  }
  return true;
}

/* updateID() is another function written for deleting covariates.
 * It updates covList array and the survived covariates' indices to
 * make sure they match each other */
void Gdw::updateID() {
  // If there is no any real covariate in the selected item, skip the removal
  if (!selID.size()) return;
  // sorting selID is required to calculate the index offset
  sort(selID.begin(), selID.end());
  unsigned orgSize = covList.size();
  // First delete selected covariates from covList array
  for (unsigned i = 0; i < selID.size(); i++)
    covList.erase(covList.begin() + selID[i] - i);

  // Now change the index in varListView to match covList
  for (unsigned i = 0; i < orgSize; i++) {
    unsigned offset = chkID(i);
    if (offset)
      (varListView->findItem(QString::number(i), 2))
          ->setText(2, QString::number(i - offset));
  }

  if (!varListView->childCount()) initGrpCount();

  // Show a blank plot window now
  timeVector = fftVector = 0;
  typeCombo->setCurrentItem(4);
  upperWindow->clear();
  upperWindow->update();
  selID.clear();
  itemCounter = 0;
}

/* This function returns intercept covariate's ID. Return -1 if no intercept is
 * defined */
void Gdw::updateInterceptID() {
  interceptID.clear();
  QString col2, col3;
  Q3ListViewItemIterator it(varListView);
  while (it.current()) {
    col2 = it.current()->text(2);
    col3 = it.current()->text(3);
    if (col2.length() && col3 == "intercept")
      interceptID.push_back(col2.toUInt());
    it++;
  }
}

/* chkID() is written specially for "delete" functionality.
 * It checks whether an input integer equals any of the elements
 * in selID array. If yes, return 0, otherwise returns the number
 * of elements in selID that are less than the inputID. */
unsigned Gdw::chkID(int inputID) {
  for (unsigned i = 0; i < selID.size(); i++) {
    if (inputID == selID[i])
      return 0;
    else if (inputID < selID[i])
      return i;
  }

  return selID.size();
}

/* chkSel() makes sure that at least one covariate is selected
 * before modifying, returns true if positive, otherwise false. */
bool Gdw::chkSel() {
  if (!covList.size()) {
    QMessageBox::critical(0, "Error", "No covariate defined yet!");
    return false;
  }

  if (!selID.size()) {
    QMessageBox::critical(0, "Error", "No covariate selected yet!");
    return false;
  }

  return true;
}

/* chkD() checks the status of D type covariate.
 * returns true if D type exists and is one of the selected covariates,
 * otherwise false This function is mainly used for duplicate/delete operation;
 * or when the "modify" operation not only modifies selected covariate, but also
 * generates new covariate(s) of the same type based on the selection */
bool Gdw::chkD() {
  if (d_index == -1) return false;

  for (int i = 0; i < (int)selID.size(); i++) {
    if (d_index == selID[i]) return true;
  }

  return false;
}

/* This function checks whether intercept covariate is selected or not */
bool Gdw::chkIntercept() {
  if (!interceptID.size()) return false;

  QString col3;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    col3 = it.current()->text(3);
    if (col3 == "intercept") return true;
    it++;
  }

  return false;
}

/* updateMod() shows the single modified covariate in upper plot window
 * and set gUpdateFlag to be true, since G matrix has been changed */
void Gdw::updateMod() {
  if (selID.size() == 1) emit(newCovLoaded(covList[selID[0]]));

  setGUpdateFlag(true);
}

/* Slot for "Edit -> Modify Selection(s) -> Mean center" */
void Gdw::modMean() {
  if (!chkSel()) return;

  for (int i = 0; i < (int)selID.size(); i++) covList[selID[i]]->meanCenter();
  updateMod();
}

/* Slot for "Edit -> Modify Selection(s) -> Mean center non-zero" */
void Gdw::meanNonZero() {
  if (!chkSel()) return;

  int varIndex = 0, numNonZero = 0;
  int length = totalReps * TR / tmpResolve;
  double nonZeroMean = 0, element = 0;
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j];
    numNonZero = countNonZero(covList[varIndex]);
    nonZeroMean = (covList[varIndex])->getVectorSum() / (double)numNonZero;
    for (int i = 0; i < length; i++) {
      element = (covList[varIndex])->getElement(i);
      if (element) (covList[varIndex])->setElement(i, element - nonZeroMean);
    }
  }
  updateMod();
}

/* Slot for "Edit -> Modify Selection(s) -> Unit variance" */
void Gdw::unitVar() {
  if (!chkSel()) return;

  for (int i = 0; i < (int)selID.size(); i++) covList[selID[i]]->unitVariance();

  updateMod();
}

/* Slot for "Edit -> Modify Selection(s) -> Unit excursion" */
void Gdw::UnitExcurs() {
  if (!chkSel()) return;

  int varIndex;
  double vecMax = 0, vecMin = 0;
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i];
    vecMin = (covList[varIndex])->getMinElement();
    (*(covList[varIndex])) += (0 - vecMin);
    vecMax = (covList[varIndex])->getMaxElement();
    (covList[varIndex])->scaleInPlace(1.0 / vecMax);
  }

  updateMod();
}

/* Slot for "Edit -> Modify Selection(s) -> Convolve" */
void Gdw::modConv() {
  if (!chkSel()) return;

  G_Convolve *myConv = new G_Convolve(tmpResolve);
  myConv->setCaption("Convolve options");
  myConv->show();
  this->setDisabled(true);
  QObject::connect(myConv, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myConv, SIGNAL(doneSignal(VB_Vector *, double, QString &)),
                   this, SLOT(showConv(VB_Vector *, double, QString &)));
}

/* This slot is to show the covariate after convolution */
void Gdw::showConv(VB_Vector *convVector, double sampling, QString &tag) {
  // update covList array
  int varIndex;
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i];
    VB_Vector *tmpVec = covList[varIndex];
    covList[varIndex] =
        new VB_Vector(getConv(tmpVec, convVector, (int)sampling, tmpResolve));
  }

  // update covariate name based on the tag string from convolve interface
  QString newStr, idStr;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    idStr = it.current()->text(2);
    if (!idStr.isEmpty() && tag.length() > 0) {
      newStr = it.current()->text(0) + " (" + tag + ")";
      it.current()->setText(0, newStr);
    }
    it++;
  }

  updateMod();
}

/* Slot for "Edit -> Modify Selection(s) -> Exponential" */
void Gdw::modExpn() {
  if (!chkSel()) return;

  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed: \
<p>more than one dependent covariate would exist in G matrix after it");
    return;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  G_Expn *myExpn = new G_Expn();
  myExpn->setCaption("Exponential Options");
  myExpn->show();
  this->setDisabled(true);
  QObject::connect(myExpn, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myExpn, SIGNAL(doneSignal(double, int)), this,
                   SLOT(showExpn(double, int)));
}

/* showExpn() shows the new exponential covariate */
void Gdw::showExpn(double power, int centerFlag) {
  int varIndex;
  double tmpVal, expVal;
  int vecLength = totalReps * TR / tmpResolve;
  // Sort selID to make the new index ranking easier
  sort(selID.begin(), selID.end());
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j];
    VB_Vector *newVec = new VB_Vector(vecLength);
    for (int i = 0; i < vecLength; i++) {
      tmpVal = (covList[varIndex + j])->getElement(i);
      expVal = pow(tmpVal, power);
      newVec->setElement(i, expVal);
    }
    if (centerFlag) {
      newVec->meanCenter();
      newVec->unitVariance();
    }
    covList.insert(covList.begin() + varIndex + j + 1, newVec);
  }
  // Insert the new covariate right after original one with a modified name
  QString newStr, idStr;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    idStr = it.current()->text(2);
    if (!idStr.isEmpty()) {
      newStr = it.current()->text(0) + " [^" + QString::number(power) + "]";
      if (it.current()->parent())
        (void)new Q3ListViewItem(it.current()->parent(), it.current(), newStr,
                                 it.current()->text(1), "-1");
      else
        (void)new Q3ListViewItem(varListView, it.current(), newStr,
                                 it.current()->text(1), "-1");
    }
    it++;
  }
  rankID();
  updateMod();
}

/* rankID() ranks the covariates from the top of the tree to the bottom
 * serially. Assumption: the covariates are always ranked from small to big in
 * up->down tree view */
void Gdw::rankID() {
  QString idStr;
  int counter = 0;
  Q3ListViewItemIterator it(varListView);
  while (it.current()) {
    idStr = it.current()->text(2);
    if (!idStr.isEmpty()) {
      it.current()->setText(2, QString::number(counter));
      counter++;
    }
    it++;
  }
  // selID and d_index must be updated now
  updateSelID();
  update_d_index();
}

/* updateSelID() keeps selID array updated. This function should be
 * called after rankID(), when new covariates have been added. */
void Gdw::updateSelID() {
  selID.clear();
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  QString idStr;
  while (it.current()) {
    idStr = it.current()->text(2);
    if (idStr.size()) selID.push_back(idStr.toInt());
    it++;
  }
}

/* update_d_index() keeps d_index updated. This function should be
 * called after rankID(), when new covariates have been added. */
void Gdw::update_d_index() {
  if (d_index == -1) return;
  if (varListView->findItem("D", 1))
    d_index = varListView->findItem("D", 1)->text(2).toInt();
}

/* Slot for "Edit -> Modify Selection(s) -> Multiply by covar" */
void Gdw::modMult() {
  if (!chkSel()) return;

  G_Multiply *myMult = new G_Multiply(varListView);
  myMult->setCaption("Multiply Options");
  myMult->show();
  this->setDisabled(true);
  QObject::connect(myMult, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myMult, SIGNAL(doneSignal(int)), this, SLOT(showMult(int)));
}

/* Function to show multiplied covariate */
void Gdw::showMult(int selection) {
  int varIndex;
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j];
    (covList[varIndex])->elementByElementMult(covList[selection]);
  }
  updateMod();
}

/* Slot for "Modify Selection(s) -> Derivative(s)" */
void Gdw::modDeriv() {
  if (!chkSel()) return;

  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed: \
<p>more than one dependent covariate would exist in G matrix after it");
    return;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  if (totalReps * TR / tmpResolve % 2 != 0) {
    QMessageBox::critical(0, "Error",
                          "The number of observations must be an even number.");
    return;
  }

  G_Deriv *myDeriv = new G_Deriv();
  myDeriv->setCaption("Derivative Options");
  myDeriv->show();
  this->setDisabled(true);
  QObject::connect(myDeriv, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myDeriv, SIGNAL(doneSignal(unsigned, int)), this,
                   SLOT(showDeriv(unsigned, int)));
}

/* This slot will show the derivative covariates for a selected covariate */
void Gdw::showDeriv(unsigned numDeriv, int inputType) {
  // Sort selID to make the new index ranking easier
  sort(selID.begin(), selID.end());
  int varIndex;
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j] + j * numDeriv;
    for (unsigned i = 0; i < numDeriv; i++) {
      VB_Vector *currentVec = covList[varIndex];
      VB_Vector *newVec = derivative(currentVec);
      covList.insert(covList.begin() + varIndex + 1, newVec);
      varIndex++;
    }
  }

  insertDerivGrp(numDeriv, inputType);
  rankID();
  updateMod();
}

/* insertDerivGrp() builds a new subgroup based on selected covariate(s) to
 * include both original and new derivative covariate(s) */
void Gdw::insertDerivGrp(unsigned numDeriv, int inputType) {
  QString typeStr;
  if (inputType == 0)
    typeStr = "I";
  else
    typeStr = "N";

  QString newStr, idStr;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    QString orgName = it.current()->text(0);
    idStr = it.current()->text(2);
    if (!idStr.isEmpty()) {
      (void)new Q3ListViewItem(it.current(), it.current()->text(0),
                               it.current()->text(1), it.current()->text(2));
      it.current()->setText(0, it.current()->text(0) + " (deriv group)");
      it.current()->setText(1, QString::null);
      it.current()->setText(2, QString::null);
      for (unsigned i = 0; i < numDeriv; i++) {
        newStr = orgName + " [ deriv #" + QString::number(i + 1) + " ]";
        (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                                 newStr, typeStr, "-1");
      }
      it.current()->setOpen(true);
    }
    it++;
  }
}

/* Slot for "Modify Selection(s) -> Fourier set" */
void Gdw::modFS() {
  if (!chkSel()) return;

  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed: \
<p>more than one dependent covariate would exist in G matrix after it");
    return;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  double totalTime = (double)(totalReps * TR) / 1000.0;
  G_Fourier *myFS = new G_Fourier(totalTime);
  myFS->setCaption("Fourier basis set options");
  myFS->show();
  this->setDisabled(true);
  QObject::connect(myFS, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myFS, SIGNAL(doneSignal(int, int, int, int)), this,
                   SLOT(showFS(int, int, int, int)));
}

/* This function will show the new Fourier set covariate on the display window.
 */
void Gdw::showFS(int period, int number, int addFlag, int deltaFlag) {
  fs_set_covList(period, number, addFlag, deltaFlag);
  fs_set_view(number, addFlag);
  /* clearSelection() here not only clears current highlighted items, but also
   * emits selectionChanged()
   * signal, which calls selectionUpdate() to re-select the item and its
   * child(ren). */
  varListView->clearSelection();
  rankID();
  setGUpdateFlag(true);
}

/* update covList array after new Fourier Set covariates are added */
void Gdw::fs_set_covList(int period, int number, int addFlag, int deltaFlag) {
  // QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
  // SLOT(selectionUpdate()));
  // Sort selID to make the new index ranking easier
  sort(selID.begin(), selID.end());

  int windowWidth = period * 1000 / tmpResolve;
  int numComp = 2 * number + addFlag;
  int orderG = totalReps * TR / tmpResolve;

  double pix2 = 2.0 * 3.14159;
  int varIndex;
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j] + j * (numComp - 1);
    VB_Vector *currentVec = new VB_Vector(covList[varIndex]);

    if (addFlag) {
      insertCovDC(windowWidth, varIndex, deltaFlag);
      varIndex++;
    } else
      covList.erase(covList.begin() + varIndex);

    VB_Vector *fourierVec = new VB_Vector(orderG);
    for (int i = 1; i <= number; i++) {
      fourierVec->setAll(0);
      for (int k = 0; k < windowWidth; k++)
        (*fourierVec)[k] = sin(pix2 * k / windowWidth * i);
      VB_Vector *newVec = fs_getFFT(currentVec, fourierVec, deltaFlag);
      covList.insert(covList.begin() + varIndex, newVec);

      for (int k = 0; k < windowWidth; k++)
        (*fourierVec)[k] = cos(pix2 * k / windowWidth * i);
      VB_Vector *newVec2 = fs_getFFT(currentVec, fourierVec, deltaFlag);
      covList.insert(covList.begin() + varIndex + 1, newVec2);
      varIndex += 2;
    }
    delete currentVec;
    delete fourierVec;
  }
  // QObject::connect(varListView, SIGNAL(selectionChanged()), this,
  // SLOT(selectionUpdate()));
}

/* insertCovDC() inserts the DC covariate in covList array */
void Gdw::insertCovDC(int windowWidth, int varIndex, int deltaFlag) {
  int orderG = totalReps * TR / tmpResolve;
  VB_Vector *fourierVec = new VB_Vector(orderG);
  fourierVec->setAll(0);
  for (int j = 0; j < windowWidth; j++) fourierVec->setElement(j, 1.0);

  VB_Vector *currentVec = new VB_Vector(covList[varIndex]);
  VB_Vector *newVec = fs_getFFT(currentVec, fourierVec, deltaFlag);
  delete currentVec;
  delete fourierVec;
  covList.at(varIndex) = newVec;
}

/* fs_getFFT() returns the new sin/cos vb_vector */
VB_Vector *Gdw::fs_getFFT(VB_Vector *currentVec, VB_Vector *fourierVec,
                          int deltaFlag) {
  if (deltaFlag) calcDelta(currentVec);
  VB_Vector *newVec = new VB_Vector(fftConv(currentVec, fourierVec, false));
  newVec->meanCenter();
  newVec->unitVariance();

  return newVec;
}

/* Update varListView for new Fourier Set covariates
 * This step could be done at the same time when covList is updated,
 * but that makes debugging more difficult */
void Gdw::fs_set_view(int number, int addFlag) {
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  QString orgName, orgType, orgID;
  while (it.current()) {
    orgID = it.current()->text(2);
    if (!orgID.isEmpty()) {
      orgName = it.current()->text(0);
      orgType = it.current()->text(1);
      it.current()->setText(0, orgName + " (FS group)");
      it.current()->setText(1, QString::null);
      it.current()->setText(2, QString::null);
      if (addFlag)
        (void)new Q3ListViewItem(it.current(), orgName + " [DC]", orgType,
                                 orgID);
      QString nameStr;
      for (int i = 1; i <= number; i++) {
        nameStr = orgName + " [sin #" + QString::number(i) + "]";
        (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                                 nameStr, orgType, orgID);
        nameStr = orgName + " [cos #" + QString::number(i) + "]";
        (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                                 nameStr, orgType, orgID);
      }
      it.current()->setOpen(true);
    }
    ++it;
  }
}

/* Slot for "Modify Selection(s) -> Eigenvector set" */
void Gdw::modES() {
  if (!chkSel()) return;

  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed: \
<p>more than one dependent covariate would exist in G matrix after it");
    return;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  es_set_covList();
  es_set_view();
  /* clearSelection() here not only clears current highlighted items, but also
   * emits selectionChanged()
   * signal, which calls selectionUpdate() to re-select the item and its
   * child(ren). */
  varListView->clearSelection();
  rankID();
  setGUpdateFlag(true);
}

/* update covList for new eigenvector set covariates */
void Gdw::es_set_covList() {
  int orderG = totalReps * TR / tmpResolve;
  double magFactor = 2000.0 / (double)tmpResolve;
  if (magFactor != floor(magFactor)) {
    QMessageBox::critical(0, "Error", "Invalid upsampling rate!");
    return;
  }

  VB_Vector *eigenVec1 = new VB_Vector(orderG);
  eigenVec1->setAll(0);
  VB_Vector *eigenVec2 = new VB_Vector(orderG);
  eigenVec2->setAll(0);
  VB_Vector *eigenVec3 = new VB_Vector(orderG);
  eigenVec3->setAll(0);

  VB_Vector *tmpVec1 =
      new VB_Vector((string)filterPath.latin1() + "Eigen1.ref");
  tmpVec1->sincInterpolation((unsigned)magFactor);
  VB_Vector *tmpVec2 =
      new VB_Vector((string)filterPath.latin1() + "Eigen2.ref");
  tmpVec2->sincInterpolation((unsigned)magFactor);
  VB_Vector *tmpVec3 =
      new VB_Vector((string)filterPath.latin1() + "Eigen3.ref");
  tmpVec3->sincInterpolation((unsigned)magFactor);
  int vecLength = tmpVec1->getLength();
  for (int i = 0; i < vecLength; i++) {
    (*eigenVec1)[i] = (*tmpVec1)[i];
    (*eigenVec2)[i] = (*tmpVec2)[i];
    (*eigenVec3)[i] = (*tmpVec3)[i];
  }

  // Sort selID to make the new index ranking easier
  sort(selID.begin(), selID.end());
  // Modify selected covariates one by one
  int varIndex;
  for (int j = 0; j < (int)selID.size(); j++) {
    varIndex = selID[j] + 2 * j;
    VB_Vector *myVec = covList[varIndex];
    calcDelta(myVec);

    VB_Vector *newVec1 = new VB_Vector(fftConv(myVec, eigenVec1, false));
    VB_Vector *newVec2 = new VB_Vector(fftConv(myVec, eigenVec2, false));
    VB_Vector *newVec3 = new VB_Vector(fftConv(myVec, eigenVec3, false));

    covList.at(varIndex) = newVec1;
    covList.insert(covList.begin() + varIndex + 1, newVec2);
    covList.insert(covList.begin() + varIndex + 2, newVec3);
    delete myVec;
  }
  delete tmpVec1;
  delete tmpVec2;
  delete tmpVec3;
  delete eigenVec1;
  delete eigenVec2;
  delete eigenVec3;
}

/* update varListView after adding eigenvector set covariates */
void Gdw::es_set_view() {
  QString orgName, orgType, orgID;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    orgID = it.current()->text(2);
    if (!orgID.isEmpty()) {
      orgName = it.current()->text(0);
      orgType = it.current()->text(1);
      it.current()->setText(0, orgName + " (eigen group)");
      it.current()->setText(1, QString::null);
      it.current()->setText(2, QString::null);
      (void)new Q3ListViewItem(it.current(), orgName + " [Eigen1]", orgType,
                               orgID);
      (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                               orgName + " [Eigen2]", orgType, orgID);
      (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                               orgName + " [Eigen3]", orgType, orgID);
      it.current()->setOpen(true);
    }
    it++;
  }
}

/* Slot for "Modify Selection(s) -> Time shift" */
void Gdw::modTS() {
  // Make sure selection exists
  if (!chkSel()) return;
  // Make sure input value is valid
  bool ok;
  int msecShift = QInputDialog::getInteger(
      "Time Shift Option",
      "Please type in time shift value in unit of ms. <br>\
Positive values lag right.",
      0, -2147483647, 2147483647, 1, &ok);
  if (!ok || !msecShift) return;

  // Update covList
  int varIndex;
  double imageShift;
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i];
    imageShift = (double)msecShift / (double)tmpResolve;
    (covList[varIndex])->phaseShift(imageShift);
  }

  // Update treeview
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  QString orgName, orgID, newLabel;
  while (it.current()) {
    orgID = it.current()->text(2);
    if (!orgID.isEmpty()) {
      newLabel =
          it.current()->text(0) + " [shift " + QString::number(msecShift) + "]";
      it.current()->setText(0, newLabel);
    }
    it++;
  }

  updateMod();
}

/* Slot for "Modify Selection(s) -> Orthogonalize" */
void Gdw::modOrth() {
  if (!chkOrth()) return;

  G_Orth *myOrth = new G_Orth(varListView);
  myOrth->setCaption("Orthogonalize Options");
  myOrth->show();
  this->setDisabled(true);
  QObject::connect(myOrth, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myOrth, SIGNAL(doneSignal(std::vector<int>)), this,
                   SLOT(showOrth(std::vector<int>)));
}

/* chkOrth() makes sure there are at least one covariate which is not selected
 */
bool Gdw::chkOrth() {
  if (!chkSel()) return false;

  if (covList.size() == 1) {
    QMessageBox::critical(
        0, "Error", "Orthogonalization not allowed with only one covariate!");
    return false;
  }

  if (covList.size() == selID.size()) {
    QMessageBox::critical(
        0, "Error",
        "At least one independent covariate should be unselected for \
orthogonalization!");
    return false;
  }

  if (chkD()) {
    QMessageBox::critical(
        0, "Error",
        "Orthogonalization operation not allowed on dependent covariate");
    return false;
  }

  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return false;
  }

  if (d_index != -1 && covList.size() - selID.size() == 1) {
    QMessageBox::critical(
        0, "Error",
        "At least one independent covariate should be unselected for \
orthogonalization operation!");
    return false;
  }

  return true;
}

/* Show orthogonalized covariate */
void Gdw::showOrth(std::vector<int> orthID) {
  // Build subG that consists of covariates defined by orthID
  int rowNum = totalReps * TR / tmpResolve;
  int colNum = orthID.size();
  VBMatrix subG(rowNum, colNum);
  VB_Vector tmpVec;
  for (int i = 0; i < colNum; i++) {
    tmpVec = *covList[orthID[i]];
    subG.SetColumn(i, tmpVec);
  }

  // Modify selected covariates one by one by subtracting fits
  int varIndex;
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i];
    VB_Vector orthVec = calcfits(subG, *covList[varIndex]);
    if (orthVec.size() < 1) {
      QMessageBox::critical(0, "Error",
                            "Non-dependent covariates are linearly dependent, "
                            "invalid Orthogonalization!");
      return;
    }
    (*(covList[varIndex])) -= orthVec;
  }
  updateMod();
}

/* Slot for "Modify Selection(s) -> Convert to delta" */
void Gdw::modC2D() {
  if (!chkSel()) return;

  int varIndex;
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i];
    calcDelta(covList[varIndex]);
  }

  updateMod();
}

/* Modify Selection(s) -> Finite Impulse Response */
void Gdw::modFIR() {
  if (!chkSel()) return;
  if (chkD()) {
    QMessageBox::critical(0, "Error",
                          "Operation not allowed: \
<p>more than one dependent covariate would exist in G matrix after it");
    return;
  }
  if (chkIntercept()) {
    QMessageBox::critical(0, "Error",
                          "Operation not permitted on intercept covariate");
    return;
  }

  // Ask user to type in a input value as number of TRs
  bool ok;
  int numTR = QInputDialog::getInteger(
      "Set Finite Impulse Response",
      "Please type in the number of TRs used in finite inpulse response", 0, 0,
      totalReps, 1, &ok);
  // Make sure the input value is valid
  if (!ok || !numTR) return;

  fir_set_covList(numTR);
  fir_set_view(numTR);
  /* clearSelection() here not only clears current highlighted items, but also
   * emits selectionChanged()
   * signal, which calls selectionUpdate() to re-select the item and its
   * child(ren). */
  varListView->clearSelection();
  rankID();
  setGUpdateFlag(true);
}

/* Put new FIR covariate(s) into covList */
void Gdw::fir_set_covList(int numTR) {
  int varIndex;
  double timeShift;
  // Sort selID to make the new index ranking easier
  sort(selID.begin(), selID.end());
  for (int i = 0; i < (int)selID.size(); i++) {
    varIndex = selID[i] + i * numTR;
    // First convert the highlighted covariates to delta
    calcDelta(covList[varIndex]);
    // Generate equal number of time-shifted covariates based on the selected
    // one
    for (int j = 1; j <= numTR; j++) {
      VB_Vector *newVec = new VB_Vector();
      timeShift = (double)j * TR / tmpResolve;
      covList[varIndex]->phaseShift(timeShift, *newVec);
      covList.insert(covList.begin() + varIndex + j, newVec);
    }
  }
}

/* Update varListView after FIR covariates are added */
void Gdw::fir_set_view(int numTR) {
  QString orgName, orgType, orgID, newLabel;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  while (it.current()) {
    orgID = it.current()->text(2);
    if (!orgID.isEmpty()) {
      orgName = it.current()->text(0);
      orgType = it.current()->text(1);
      it.current()->setText(0, orgName + " (FIR group)");
      it.current()->setText(1, QString::null);
      it.current()->setText(2, QString::null);
      (void)new Q3ListViewItem(it.current(), orgName + " [FIR0]", orgType,
                               orgID);
      for (int i = 1; i <= numTR; i++) {
        newLabel = orgName + " [FIR" + QString::number(i) + "]";
        (void)new Q3ListViewItem(it.current(), getLastChild(it.current()),
                                 newLabel, orgType, orgID);
      }
      it.current()->setOpen(true);
    }
    it++;
  }
}

/* Slot for "Evaluate -> Colinearity with all" */
void Gdw::colAll() {
  if (!chkColinear()) return;
  getColinear(0);
}

/* Slot for "Evaluate -> Colinearity with interest" */
void Gdw::col_I() {
  if (!chkColinear()) return;
  getColinear(1);
}

/* Slot for "Evaluate -> Colinearity with no interest" */
void Gdw::col_N() {
  if (!chkColinear()) return;
  getColinear(2);
}

/* chkColinear() makes sure there are at least two covariates defined and only
 * one is selected */
bool Gdw::chkColinear() {
  if (d_index == -1 && covList.size() < 2) {
    QMessageBox::critical(0, "Error",
                          "At least two independent covariates are required "
                          "for colinearity evaluation!");
    return false;
  }
  if (d_index != -1 && covList.size() < 3) {
    QMessageBox::critical(0, "Error",
                          "At least two independent covariates are required "
                          "for colinearity evaluation!");
    return false;
  }
  if (selID.size() != 1) {
    QMessageBox::critical(
        0, "Error",
        "Please select one and only one covariate for colinearity evaluation!");
    return false;
  }
  if (selID[0] == d_index) {
    QMessageBox::critical(
        0, "Error",
        "Type D covariate can not be selected to evaluate colinearity!");
    return false;
  }

  return true;
}

/* getColinear() is a generic functions to show colinear values.
 * It accepts an integer value as the covariate category:
 * 0: all covariates
 * 1: all of intrerest ("I")
 * 2: all of no interest (includes both "N" and "K") */
void Gdw::getColinear(unsigned colType) {
  if (colType > 2) {
    QMessageBox::critical(0, "Error", "getColinear(): Invalid covariate type");
    return;
  }

  int selIndex = selID[0];
  if (!(covList[selIndex])->getVariance()) {
    QMessageBox::critical(
        0, "Error",
        "Selected covariate is a constant. Can not evaluate colinearity.");
    return;
  }

  std::vector<int> colList = findColID(colType);
  QString helpStr;
  if (colType == 0)
    helpStr = "[ I/N/K ]";
  else if (colType == 1)
    helpStr = "of interest type [ I ]";
  else
    helpStr = "of no interest type [ N/K ]";
  if (!colList.size()) {
    QMessageBox::critical(
        0, "Error", "Other independent covariates " + helpStr + " not found.");
    return;
  }

  int rowNum = totalReps, colNum = colList.size();
  int ratio = TR / tmpResolve;
  VB_Vector currentVec = *downSampling(covList[selIndex], ratio);
  VBMatrix subA(rowNum, colNum);
  VB_Vector tmpVec;
  int j = 0;
  for (int i = 0; i < (int)colList.size(); i++) {
    j = colList[i];
    tmpVec = *downSampling(covList[j], ratio);
    subA.SetColumn(i, tmpVec);
  }

  double colVal = calcColinear(subA, currentVec);
  if (colVal < 0) {
    QMessageBox::information(0, "Colinearity Info",
                             "Invalid colinearity value");
    return;
  }

  QMessageBox::information(
      0, "Colinearity Info",
      "The colinearity between the selected \
covariate and all the other independent covariates " +
          helpStr + " is <b>" + QString::number(colVal) +
          "</b>.<P>This value is the correlation between the covariate and the best linear \
combination of the other independent variables. \
<P>Note that this is calculated prior to any exogenous smoothing that might be applied.");
}

/* findColID() returns an array of integers that is the index of colinearity
 * covariate(s). It accepts two arguments: the first is the index of the
 * currently selected covariate.
 * This index will be skipped. The second is type of colinearity covariate. */
std::vector<int> Gdw::findColID(int colType) {
  std::vector<int> colList;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Unselected);
  QString idStr, typeStr;
  while (it.current()) {
    typeStr = it.current()->text(1);
    idStr = it.current()->text(2);
    if (colType == 0 && idStr.length() && typeStr != "D")
      colList.push_back(idStr.toInt());
    else if (colType == 1 && idStr.length() && typeStr == "I")
      colList.push_back(idStr.toInt());
    else if (colType == 2 && idStr.length() &&
             (typeStr == "N" || typeStr == "K"))
      colList.push_back(idStr.toInt());
    ++it;
  }
  return colList;
}

/* Slot for "Evaluate -> Linear dependence of all" */
void Gdw::a_LD() {
  if (!chkLD()) return;
  showLD(0);
}

/* Slot for "Evaluate -> Linear dependence of interest" */
void Gdw::i_LD() {
  if (!chkLD()) return;
  showLD(1);
}

/* Slot for "Evaluate -> Linear dependence of non-interest" */
void Gdw::nk_LD() {
  if (!chkLD()) return;
  showLD(2);
}

/* Slot for "Evaluate -> Linear dependence of selected" */
void Gdw::sel_LD() {
  if (!chkLD()) return;
  if (!selID.size()) {
    QMessageBox::critical(0, "Error", "No covariate selected yet.");
    return;
  }
  if (chkD()) {
    QMessageBox::critical(
        0, "Error",
        "Please deselect type D cocariate before checking linear dependence.");
    return;
  }
  if (selID.size() == 1) {
    QMessageBox::critical(0, "Error",
                          "Invalid operation: <br>\
only one independent covariate available for linear dependence check");
    return;
  }

  showLD(3);
}

/* showLD() calculates determinant of the matrix based on the input covariate
 * type */
void Gdw::showLD(int ldType) {
  std::vector<int> ldList = find_LD_ID(ldType);

  QString helpStr;
  if (ldType == 0)
    helpStr = "[ I/N/K ]";
  else if (ldType == 1)
    helpStr = "of interest [ I ]";
  else if (ldType == 2)
    helpStr = "of no interest [ N/K ]";
  else
    helpStr = "that are selected";

  if (!ldList.size()) {
    QMessageBox::critical(0, "Error",
                          "No independent covariate " + helpStr + " found.");
    return;
  }
  if (ldList.size() == 1) {
    QMessageBox::critical(
        0, "Error",
        "Invalid operation: <br>only one independent covariate " + helpStr +
            " available for linear dependence check");
    return;
  }

  double gDeterm = ldDeterm(ldList);
  QString tmpStr = "determinant = " + QString::number(gDeterm);
  if (gDeterm == 0)
    QMessageBox::information(
        0, "Info",
        tmpStr + "<p>The covariates " + helpStr + " ARE linearly dependent!");
  else if (gDeterm < 0.000001)
    QMessageBox::information(0, "Info",
                             tmpStr + "<p>The covariates " + helpStr +
                                 " are CLOSE to linear dependence.");
  else
    QMessageBox::information(0, "Info",
                             tmpStr + "<p>The covariates " + helpStr +
                                 " are NOT linearly dependent.");
}

/* chkLD() checks the G matrix status before calculating determinant.
 * Returns true if it passws, false otherwise */
bool Gdw::chkLD() {
  // No covariates defined at all
  if (!covList.size()) {
    QMessageBox::critical(0, "Error", "No independent covariates found.");
    return false;
  }
  // Only one covariate is defined and it is type D
  if (d_index != -1 && covList.size() == 1) {
    QMessageBox::critical(0, "Error", "No independent covariates found.");
    return false;
  }
  // Only one independent covariate
  if (d_index == -1 && covList.size() == 1) {
    QMessageBox::critical(0, "Error",
                          "Invalid operation: <br>\
only one independent covariate available for linear dependence check");
    return false;
  }
  // Only one independent covariate
  if (d_index != -1 && covList.size() == 2) {
    QMessageBox::critical(0, "Error",
                          "Invalid operation: <br>\
only one independent covariate available for linear dependence check");
    return false;
  }
  // Pass!
  return true;
}

/* find_LD_ID() accepts an integer to label the covariate type and
 * returns the covariate index that matches the type.
 * 0: I/N/K    1: I     2: N/K     3: selected covariates     */
std::vector<int> Gdw::find_LD_ID(int ldType) {
  std::vector<int> ldList;
  if (ldType == 0) {
    for (int i = 0; i < (int)covList.size(); i++) {
      if (i != d_index) ldList.push_back(i);
    }
    return ldList;
  }

  if (ldType == 3) return selID;

  Q3ListViewItemIterator it(varListView);
  QString idStr, typeStr;
  while (it.current()) {
    typeStr = it.current()->text(1);
    idStr = it.current()->text(2);
    if (ldType == 1 && idStr.length() && typeStr == "I")
      ldList.push_back(idStr.toInt());
    else if (ldType == 2 && idStr.length() &&
             (typeStr == "N" || typeStr == "K"))
      ldList.push_back(idStr.toInt());
    ++it;
  }
  return ldList;
}

/* This function calculates the determinant of the downsampled G matrix */
double Gdw::ldDeterm(std::vector<int> ldList) {
  int downRatio = TR / tmpResolve;
  int colNum = ldList.size();
  VBMatrix downG(totalReps, colNum);

  VB_Vector tmpVec;
  int j = 0;
  for (int i = 0; i < colNum; i++) {
    j = ldList[i];
    tmpVec = *downSampling(covList[j], downRatio);
    downG.SetColumn(i, tmpVec);
  }

  return getDeterm(downG);
}

/* Slot for "Evaluate -> Mean and SD" */
void Gdw::meanSD() {
  if (!covList.size()) {
    QMessageBox::critical(0, "Error", "No covariate defined yet!");
    return;
  }
  if (!selID.size()) {
    QMessageBox::critical(0, "Error", "No covariate selected yet!");
    return;
  }

  int myIndex;
  VB_Vector myVec;
  double vecMean = 0, vecSD = 0;
  QString helpStr = "";
  for (int i = 0; i < (int)selID.size(); i++) {
    myIndex = selID[i];
    myVec = *downSampling(covList[myIndex], TR / tmpResolve);
    vecMean = myVec.getVectorMean();
    vecSD = sqrt(myVec.getVariance());
    helpStr += QString("Covariate #%1:<br> Mean = %2, SD = %3<p>")
                   .arg(i)
                   .arg(vecMean)
                   .arg(vecSD);
  }
  QMessageBox::information(0, "Information", helpStr);
}

/* Slot for "Evaluate -> Efficiency" */
void Gdw::efficiency() {
  if (covList.size() < 2) {
    QMessageBox::critical(
        0, "Error",
        "At least two covariates are required to evaluate efficiency!");
    return;
  }

  G_Eff *myEff = new G_Eff(varListView, covList, TR, totalReps, tmpResolve);
  myEff->setCaption("Check Efficiency");
  myEff->show();
  this->setDisabled(true);
  myEff->setFocus();
  QObject::connect(myEff, SIGNAL(cancelSignal(bool)), this,
                   SLOT(setEnabled(bool)));
  QObject::connect(myEff, SIGNAL(delSignal(std::vector<int>)), this,
                   SLOT(showEff(std::vector<int>)));
}

/* Function to show the new G matrix after efficiency evaluation */
void Gdw::showEff(std::vector<int> delList) {
  // If all covariates will be deleted, call delAll()
  if (delList.size() == covList.size()) {
    delAll();
    return;
  }

  // Ignore selection change signal first
  QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
                      SLOT(selectionUpdate()));
  // If only some of the covaraites will be deleted, remove them from covList
  // first
  for (int i = 0; i < (int)delList.size(); i++)
    covList.erase(covList.begin() + delList[i] - i);
  // Sort selection ID
  sort(selID.begin(), selID.end());
  std::vector<int> surviver = findDiff(delList);

  // If some (not all) of the selected covariates will be kept, clear all
  // selection now
  if (surviver.size() < selID.size()) varListView->clearSelection();
  // Delete items in delList
  for (int i = 0; i < (int)delList.size(); i++)
    delete varListView->findItem(QString::number(delList[i]), 2);
  // Reselect survived items
  if (surviver.size() && surviver.size() < selID.size())
    for (int i = 0; i < (int)surviver.size(); i++)
      (varListView->findItem(QString::number(delList[i]), 2))
          ->setSelected(true);
  // All selected items are deleted, update upperWindow
  else if (!surviver.size()) {
    timeVector = fftVector = 0;
    typeCombo->setCurrentItem(4);
    upperWindow->clear();
    upperWindow->update();
  }

  // Update ID field
  rankID();
  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));
  setGUpdateFlag(true);
  // update intercept status
  updateInterceptID();
}

/* chkElement() checks if the index of D type covariate is one of the members in
 * delList array. returns true if element exists and is one of the selected
 * covariates, otherwise false. THis function is written specially for
 * efficiency evaluation */
bool Gdw::chkElement(int inElement, std::vector<int> inList) {
  int inSize = inList.size();
  if (inElement < inList[0] || inElement > inList[inSize - 1]) return false;

  for (int i = 0; i < inSize; i++) {
    if (inElement == inList[i]) return true;
  }
  return false;
}

/* findDiff() compares selID and delList and put the elements which are in selID
 * but not in inList into diffList. diffList is returned. */
std::vector<int> Gdw::findDiff(std::vector<int> inList) {
  int j;
  std::vector<int> diffList;
  for (int i = 0; i < (int)selID.size(); i++) {
    j = selID[i];
    if (!chkElement(j, inList)) diffList.push_back(j);
  }
  return diffList;
}

/* Slot for "Evaluate -> Noise spectra" */
void Gdw::evalNoise() {
  if (!chkNoise()) return;
  VB_Vector *psVec = getPSVec();
  if (!psVec) {
    QMessageBox::warning(0, "Error!", "No power spectrum vector found.");
    return;
  }
  double var3min = (-1.0) / (totalReps * TR / 1000.0);
  VB_Vector *fitParams = new VB_Vector(8);
  fitParams = fitOneOverF(psVec, var3min, (double)TR);
  if (fitParams->getElement(0) != 1.0) {
    QMessageBox::critical(0, "Error",
                          "Fitting not successful. No noise model defined.");
    return;
  }

  double var1 = fitParams->getElement(2);
  double var2 = fitParams->getElement(3);
  double var3 = fitParams->getElement(4);
  VB_Vector *noiseVec1 = makeOneOverF(totalReps, var1, var2, var3, (double)TR);
  VB_Vector *noiseVec2 = new VB_Vector(totalReps / 2 + 1);
  for (int i = 0; i < totalReps / 2 + 1; i++)
    noiseVec2->setElement(i, noiseVec1->getElement(i));
  delete noiseVec1;

  double minElement = noiseVec2->getMinElement();
  (*noiseVec2) += (minElement * (-2));
  double maxElement = noiseVec2->getMaxElement();
  noiseVec2->scaleInPlace(1.0 / maxElement);
  int varIndex = selID[0];
  VB_Vector *currentVec = downSampling(covList[varIndex], TR / tmpResolve);
  currentVec->getPS();
  VB_Vector *halfVec = new VB_Vector(currentVec->getLength() / 2);

  for (int i = 0; i < (int)currentVec->getLength() / 2; i++)
    (*halfVec)[i] = (*currentVec)[i];
  delete currentVec;

  double vecMax = halfVec->getMaxElement();
  halfVec->scaleInPlace(1.0 / vecMax);
  PlotScreen *noiseWindow = new PlotScreen();
  noiseWindow->setCaption("Noise Spectra Evaluation");
  noiseWindow->setPlotMode(1);
  noiseWindow->setFirstVector(halfVec);
  QColor noiseColor = Qt::red;
  noiseWindow->addVector(noiseVec2, noiseColor);
  noiseWindow->setXCaption("Freq (Number)");
  noiseWindow->setYCaption("Magnitude");
  noiseWindow->update();
  noiseWindow->show();
}

/* chkNoise() makes sure there is only one covariate selected now */
bool Gdw::chkNoise() {
  if (!covList.size()) {
    QMessageBox::critical(0, "Error", "No covariates defined yet.");
    return false;
  }
  if (!selID.size()) {
    QMessageBox::critical(0, "Error", "No covariates selected yet.");
    return false;
  }
  if (selID.size() > 1) {
    QMessageBox::critical(0, "Error",
                          "Please select one and only one covariate.");
    return false;
  }
  return true;
}

/* getPSVec() is a function written specially for noise evaluation.
 * It calculates the power spectrum based on either the tes files
 * selected in previous step or user's input file.
 * If it is combo mode and _PS.ref files are all available and have same length,
 * average them and return the final result. Otherwise ask user to choose one
 * file */
VB_Vector *Gdw::getPSVec() {
  if (mode == COMBO && chkPSstat()) {
    QString tesFileName = tesList->item(0)->text();
    const char *psFileName;
    psFileName = tesFileName.replace(tesFileName.length() - 4, 4, "_PS.ref");
    VB_Vector *psVector = new VB_Vector(psFileName);
    unsigned psLength = psVector->getLength();
    int psNum = 1;
    for (int i = 1; i < tesNum; i++) {
      tesFileName = tesList->item(i)->text();
      const char *psFileName =
          tesFileName.replace(tesFileName.length() - 4, 4, "_PS.ref").ascii();
      VB_Vector *tmpVector = new VB_Vector(psFileName);
      if (psLength != tmpVector->getLength()) break;
      (*psVector) += tmpVector;
      psNum++;
    }
    if (psNum == tesNum) {
      psVector->scaleInPlace(1.0 / (double)psNum);
      return psVector;
    }
    delete psVector;
  }
  // If it is SINGLE mode, or _PS.ref files not found in combo mode, or found
  // but not same length
  QString psFilename = Q3FileDialog::getOpenFileName(
      QString::null, "G matrix files (*_PS.ref)", this,
      "Open a power spectrum file", "Choose a *_PS.ref file");
  if (psFilename.isEmpty()) return 0;
  VB_Vector *tmpVector = new VB_Vector();
  if (tmpVector->ReadFile(psFilename.ascii()) == 0) return tmpVector;
  return 0;
}

/* Slot for "Tools -> Load condition function", wrapper of loadCondFunct(string)
 */
void Gdw::loadCondFunct() {
  // Open a FileDialog window to select ref file
  QString inputString = Q3FileDialog::getOpenFileName(
      QString::null, "Ref files (*.ref)", this, "Load condition function",
      "Please choose the condition function.");
  if (inputString.isEmpty()) return;
  loadCondFunct(inputString);
}

/* A generic function which reads the input filename as condition function name
 */
void Gdw::loadCondFunct(QString &tmpString) {
  if (!chkCondition(tmpString.ascii())) return;
  condRef = tmpString;
  // Now delete the original conditional keys in the listbox
  keyList->clear();
  // Add keys into the condition key listbox
  for (size_t m = 0; m < condKey.size(); m++) {
    QString keyString = QString(condKey(m));
    keyList->insertItem(keyString);
  }
  keyList->setCurrentItem(0);
  setGUpdateFlag(true);
}

/* chkCondition() checks the condition function file.
 * Returns true if all parameters are good, otherwise false. */
bool Gdw::chkCondition(const char *condFile) {
  condKey.clear();
  condVector = new VB_Vector();
  int refStat = getCondVec(condFile, condKey, condVector);
  if (refStat == -1) {  // Quit if the file isn't readable
    QMessageBox::critical(0, "Condition Function Error!",
                          "File not readable: " + QString(condFile));
    return false;
  } else if (refStat == -2) {
    QMessageBox::critical(
        0, "Condition Function Error!",
        "Different number of keys in header and content: " + QString(condFile));
    return false;
  } else if (refStat == 1) {
    QMessageBox::critical(
        0, "Condition Function Error!",
        "Different keys in header and content: " + QString(condFile));
    return false;
  }

  int condLength = condVector->getLength();
  if (!totalReps) {
    totalReps = condLength;
    numberString->setText(QString::number(totalReps));
  }
  // Make sure the number of elements in condition function is a multiple of
  // total time points
  else if (condLength % totalReps != 0) {
    QMessageBox::critical(
        0, "Error!",
        "Number of elements in condition function must be a multiple of \
total number of time points.");
    condKey.clear();
    condVector = 0;
    return false;
  }
  // Make sure condition function can be upsampled
  if ((TR / tmpResolve) % (condLength / totalReps) != 0) {
    QString helpStr;
    if (TR % (condLength / totalReps) == 0) {
      int maxSamp = TR / (condLength / totalReps);
      helpStr =
          "Condition function can not be upsampled with the current sampling rate.\
<p>Click <b>Tools</b> to reset the upsampling rate. \
<p>Recommended value: " +
          QString::number(maxSamp) + "/N (N is positive integer)";
    } else
      helpStr =
          "Invalid condition function: can not be upsampled with the current "
          "sampling rate.";
    QMessageBox::critical(0, "Error!", helpStr);
    condKey.clear();
    condVector = 0;
    return false;
  }
  return true;
}

/* Slot for "Tools -> Load condition labels"
 * If the number of lines in condition label file is more than that of the keys
 * in original condition function, warn the user of possible error and let
 * him/her decide whether to continue; If the first number is less than the
 * second, report an error and quit; otherwise preceed without any complaints.
 */
void Gdw::loadCondLabel() {
  if (!condVector) {
    QMessageBox::critical(0, "Error!", "Condition function not defined yet.");
    return;
  }

  tokenlist newCondKey;
  QString tmpString = Q3FileDialog::getOpenFileName(
      QString::null, "Condition label files (*.txt)", this,
      "Load condition function", "Please choose the new condition label file.");
  if (tmpString.isEmpty()) return;

  const char *tmpStr2 = tmpString;
  int readFlag = getCondLabel(newCondKey, tmpStr2);
  // Quit if the file chosen isn't readable
  if (readFlag == -1) {
    QMessageBox::critical(0, "Error!",
                          "The file you choose isn't readable, \
please change its permission or try another file.");
    return;
  }

  unsigned int keyNum = newCondKey.size();
  if (keyNum > keyList->count()) {
    switch (QMessageBox::warning(
        0, "Warning!",
        "The  file you choose contains more condition \
keys than you need for the current condition function. <p>Click <b>continue</b> to use the first " +
            QString::number(keyList->count()) +
            " keys in this file. <P>Click <b>Cancel</b> to go back \
and load another condition label file.",
        "Continue", "Cancel", 0, 0, 1)) {
      case 0:
        for (unsigned int i = 0; i < keyList->count(); i++) {
          QString newKey = QString(newCondKey(i));
          keyList->changeItem(newKey, i);
          condKey[i] = newCondKey[i];
        }
        setGUpdateFlag(true);
        break;
      case 1:
        break;
    }
  } else if (keyNum == keyList->count()) {
    for (unsigned int i = 0; i < keyList->count(); i++) {
      QString newKey = QString(newCondKey(i));
      keyList->changeItem(newKey, i);
      condKey[i] = newCondKey[i];
    }
    setGUpdateFlag(true);
  } else
    QMessageBox::critical(0, "Error!",
                          "The number of condition keys in this file \
is less than that required by the condition function. <p>Please choose another condition label file.");
}

/* Slot for "Tools -> Save condition labels" */
void Gdw::saveCondLabel() {
  if (keyList->count() == 0) {
    QMessageBox::information(0, "Error", "No condition keys found.");
    return;
  }

  QString labelFile = Q3FileDialog::getSaveFileName(
      QString::null, tr("TXT files (*.txt);;All files (*)"), this,
      "Save Condition Labels",
      "Choose a filename to save condition labels under: ");
  if (labelFile.isEmpty()) return;

  QFileInfo *labFileInfo = new QFileInfo(labelFile);
  QString pathString = labFileInfo->dirPath(true);
  QFileInfo *labFilePath = new QFileInfo(pathString);

  if (labFileInfo->exists()) {
    switch (QMessageBox::warning(
        0, "Warning!",
        labelFile + " already exists. Are you sure you want to overwrite \
this file now?",
        "Yes", "No", "Cancel", 0, 2)) {
      case 0:  // "Yes" overwrites the file
        if (labFilePath->isWritable()) {
          writeLabFile(labelFile);
        } else
          QMessageBox::critical(
              0, "Error!",
              "Not permitted to write file in this directory: " + pathString);
        break;
      case 1:  // "No" does nothing
        break;
      case 2:  // "Cancel" closes the function
        break;
        return;
    }
  } else if (labFilePath->isWritable())
    writeLabFile(labelFile);
  else
    QMessageBox::critical(
        0, "Error!",
        "Not permitted to write file in this directory: " + pathString);
}

/* setUpsampling() will reset the upsampling rate (tmpResolve) */
void Gdw::setUpsampling() {
  bool ok;
  int newSampling = QInputDialog::getInteger(
      "Enter New upsampling rate", "Please enter the new upsampling rate (ms)",
      tmpResolve, 0, 2147483647, 1, &ok);
  if (!ok) return;

  if (newSampling != tmpResolve && TR % newSampling == 0) {
    reInit(true);
    tmpResolve = newSampling;
    if (TR) upperWindow->setRatio(TR / tmpResolve);
    upperWindow->update();
    samplingStr->setText(QString::number(tmpResolve));
  } else if (TR % newSampling != 0) {
    QMessageBox::critical(
        0, "Error!",
        "Invalid value. TR must be multiple of the new sampling rate.");
    return;
  }
}

/* writeLabFile() will generate a condition label file with the name of input
 * QString */
void Gdw::writeLabFile(QString &inputFile) {
  const char *filename = inputFile.ascii();
  FILE *condLabFile;
  condLabFile = fopen(filename, "w");

  fprintf(condLabFile, ";VB98\n");
  fprintf(condLabFile, ";TXT1\n\n");

  const char *tmpString;
  for (unsigned i = 0; i < keyList->count(); i++) {
    tmpString = keyList->text(i);
    fprintf(condLabFile, tmpString);
    fprintf(condLabFile, "\n");
  }

  fclose(condLabFile);
}

/* Slot for "Tools -> Save covariate as ref" */
void Gdw::saveCov2Ref() {
  if (!covList.size()) {
    QMessageBox::critical(0, "Error", "No covariate defined yet");
    return;
  }
  if (selID.size() != 1) {
    QMessageBox::critical(0, "Error",
                          "Please select one and only one covariate");
    return;
  }

  QString covFile = Q3FileDialog::getSaveFileName(
      QString::null, tr("REF files (*.ref);;All files (*)"), this,
      "Save Single Covariate",
      "Choose a filename to save selecteded covariate under: ");
  if (covFile.isEmpty()) return;

  QFileInfo *covFileInfo = new QFileInfo(covFile);
  QString pathString = covFileInfo->dirPath(true);
  QFileInfo *covFilePath = new QFileInfo(pathString);

  if (covFileInfo->exists()) {
    switch (QMessageBox::warning(
        0, "Warning!",
        covFile + " already exists. Are you sure you want to overwrite \
this file now?",
        "Yes", "No", "Cancel", 0, 2)) {
      case 0:  // "Yes" overwrites the file
        if (covFilePath->isWritable()) {
          writeCovFile(covFile);
        } else
          QMessageBox::critical(
              0, "Error!",
              "Not permitted to write file in this directory: " + pathString);
        break;
      case 1:  // "No" does nothing
        break;
      case 2:  // "Cancel" closes the function
        break;
        return;
    }
  } else if (covFilePath->isWritable())
    writeCovFile(covFile);
  else
    QMessageBox::critical(
        0, "Error!",
        "Not permitted to write file in this directory: " + pathString);
}

/* This function generates a REF file based on currently highlighted covariate.
 * The new file's name is the input string. */
void Gdw::writeCovFile(QString &inputName) {
  const char *filename = inputName;
  int selIndex = selID[0];
  QString selName =
      getCovName(varListView->findItem(QString::number(selIndex), 2));
  QString selLine = ";; Name: " + selName + "\n";
  FILE *covFile;
  covFile = fopen(filename, "w");

  fprintf(covFile, ";VB98\n");
  fprintf(covFile, ";REF1\n");
  fprintf(covFile, ";\n");
  fprintf(covFile, ";; Single covariate from G matrix\n");
  fprintf(covFile, selLine.ascii());
  fprintf(covFile, ";\n");

  char numLine[100];
  VB_Vector *downVec = downSampling(covList[selIndex], TR / tmpResolve);
  for (unsigned i = 0; i < downVec->getLength(); i++) {
    sprintf(numLine, "%f\n", downVec->getElement(i));
    fprintf(covFile, numLine);
  }

  fclose(covFile);
  delete downVec;
}

/* Slot when "Secs" is chosen */
void Gdw::secClicked() {
  if (secFlag) return;
  if (selID.size() != 1) {
    secFlag = 1;
    return;
  }

  if (fftFlag) {
    upperWindow->setAllNewX(0, 1000.0 / (2.0 * TR));
    upperWindow->setXCaption("Freq (Hz)");
  } else {
    if (upperWindow->getFirstPlotMode() % 2 == 1)
      upperWindow->setFirstXLength((totalReps - 1) * TR / 1000.0);
    else
      upperWindow->setFirstXLength(totalReps * TR / 1000.0);
    upperWindow->setXCaption("Time (Sec)");
  }

  upperWindow->update();
  secFlag = 1;
}

/* Slot when "TRs" is chosen */
void Gdw::TRClicked() {
  if (!secFlag) return;
  if (selID.size() != 1) {
    secFlag = 0;
    return;
  }

  if (fftFlag) {
    upperWindow->setAllNewX(0, (double)totalReps / 2.0);
    upperWindow->setXCaption("Freq (Number)");
  } else {
    if (upperWindow->getFirstPlotMode() % 2 == 1)
      upperWindow->setAllNewX(0, totalReps - 1.0);
    else
      upperWindow->setAllNewX(0, totalReps);
    upperWindow->setXCaption("Time (Images)");
  }

  upperWindow->update();
  secFlag = 0;
}

/* Slot when "Time Plot" is chosen */
void Gdw::timePlotClicked() {
  // Do nothing if it is already in time plot mode
  if (!fftFlag) return;
  // Do nothing if no vector stored in upperWindow
  if (!upperWindow->getVecNum()) {
    fftFlag = 0;
    return;
  }

  upperWindow->setFirstVector(timeVector);
  QString xCaption;
  if (secFlag) {
    xCaption = "Time (Sec)";
    if (upperWindow->getFirstPlotMode() % 2 == 1)
      upperWindow->setFirstXLength((totalReps - 1) * TR / 1000.0);
    else
      upperWindow->setFirstXLength(totalReps * TR / 1000.0);
  } else {
    xCaption = "Time (Images)";
    if (upperWindow->getFirstPlotMode() % 2 == 1)
      upperWindow->setFirstXLength(totalReps - 1);
    else
      upperWindow->setFirstXLength(totalReps);
  }
  secButt->setText("Secs");  // Label the button "Secs"
  upperWindow->setXCaption(xCaption);

  upperWindow->update();
  fftFlag = 0;
  // Print out the highlighted covariate's mean and std values
  VB_Vector *timeVector_down = downSampling(timeVector, TR / tmpResolve);
  double vecMean = timeVector_down->getVectorMean();
  double vecSD = sqrt(timeVector_down->getVariance());
  statusBar()->message(
      QString("Covariate info: mean=%1, SD=%2").arg(vecMean).arg(vecSD));
}

/* Slot when "Freq Plot" is chosen */
void Gdw::freqPlotClicked() {
  // Do nothing if it is already in time plot mode
  if (fftFlag) return;
  // Do nothing if no vector stored in upperWindow
  if (!upperWindow->getVecNum()) {
    fftFlag = 1;
    return;
  }

  upperWindow->setFirstVector(fftVector);
  if (mode == COMBO && fitFlag) {
    QColor noiseColor = Qt::red;
    VB_Vector *newNoiseVec = new VB_Vector(noiseVec);
    newNoiseVec->scaleInPlace(fftVector->getMaxElement());
    upperWindow->addVector(newNoiseVec, noiseColor);
  }
  upperWindow->setPlotMode(1);

  int lowFreqIndex = getLowFreq(fftVector) - 1;
  if (lowFreqIndex > 1)
    statusBar()->message(
        QString("Covariate info: frequency 0-%1 < 1% power. You can remove %2 \
frequencies safely.")
            .arg(lowFreqIndex)
            .arg(lowFreqIndex));
  else if (lowFreqIndex == 1)
    statusBar()->message(
        QString("Covariate info: frequency 0-1 < 1% power. You can remove 1 \
frequency safely."));
  else if (lowFreqIndex == 0)
    statusBar()->message(QString("Covariate info: frequency 0 < 1% power"));
  else
    statusBar()->message(
        QString("Covariate info: no low frequency cutoff found."));

  QString xCaption;
  if (secFlag) {
    xCaption = "Freq (Hz)";
    upperWindow->setAllNewX(0,
                            1000.0 / (2.0 * TR));  // Set unit of x axis to Hz
  } else {
    xCaption = "Freq (Number)";
    upperWindow->setAllNewX(
        0, (double)totalReps / 2.0);  // Set unit of x axis to number of images
  }
  secButt->setText("Hz");  // Label the button "Hz"
  upperWindow->setXCaption(xCaption);
  upperWindow->update();
  fftFlag = 1;
}

/* Slot to set the text in left key line editor */
void Gdw::setKeyEditor(int i) {
  QString txtInBox = keyList->text(i);
  // Disconnect the following signal/slot pair temporarily to avoid infinite
  // loops
  QObject::disconnect(keyEditor, SIGNAL(textChanged(const QString &)), this,
                      SLOT(setKeyText()));
  keyEditor->setText(txtInBox);
  keyEditor->setFocus();
  // Reconnect it now
  QObject::connect(keyEditor, SIGNAL(textChanged(const QString &)), this,
                   SLOT(setKeyText()));
}

/* Slot to set the text in left key list box */
void Gdw::setKeyText() {
  int currentIndex = keyList->currentItem();
  QString newKey = keyEditor->text();
  // Disconnect the following signal/slot pair temporarily to avoid infinite
  // loops
  QObject::disconnect(keyList, SIGNAL(highlighted(int)), this,
                      SLOT(setKeyEditor(int)));
  keyList->changeItem(newKey, currentIndex);
  condKey[currentIndex] = newKey.ascii();

  // Reconnect it now
  QObject::connect(keyList, SIGNAL(highlighted(int)), this,
                   SLOT(setKeyEditor(int)));
  setGUpdateFlag(true);
}

/* nextKey() is a slot which replies keyList->returnPressed() signal.
 * It will highlight the next condition key in the key listbox. */
void Gdw::nextKey() {
  int keyIndex = keyList->currentItem();
  if (keyIndex < (int)keyList->count() - 1)
    keyList->setCurrentItem(keyIndex + 1);
  else
    keyList->setCurrentItem(0);
}

/* renameUpdate() takes care of itemRenamed() signal from varListView.
 * For now it simply sets gUpdateFlag to be true. */
void Gdw::renameUpdate() { setGUpdateFlag(true); }

/* selectGroup() selects all real covariates in a certain group */
void Gdw::selectGrp(Q3ListViewItem *inputItem) {
  // If the input item hasn't been selected, have it selected
  if (!varListView->isSelected(inputItem))
    varListView->setSelected(inputItem, true);

  // Do nothing if it doesn't have child
  if (!inputItem->childCount()) return;

  Q3ListViewItem *myChild = inputItem->firstChild();
  while (myChild) {
    selectGrp(myChild);
    myChild = myChild->nextSibling();
  }
}

/* selectionUpdate() takes care of the selection change signal from varListView
 * If the new item has child, highlighted is a group, do nothing;
 * if the new item doesn't have child, it must be a real covariate,
 * emit newCovLoaded() signal and change the typeComBo value.
 * Note: itemCounter is different from covList.size().
 * It is possible (although not very likely) that user creates a group with a
 * few covariates, then he/she deletes the real covariates, but the empty group
 * item is still there. itemCounter is defined to give user ability to remove
 * empty group. */
void Gdw::selectionUpdate() {
  selID.clear();
  itemCounter = 0;

  QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
                      SLOT(selectionUpdate()));
  selectGrp(varListView->currentItem());
  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));

  int typeStat = -1;
  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  QString typeStr, idStr;
  while (it.current()) {
    idStr = it.current()->text(2);
    if (!idStr.isEmpty()) {
      selID.push_back(idStr.toLong());
      if (typeStat == -1) {
        typeStr = it.current()->text(1);
        typeStat = 0;
      } else if (typeStat == 0 && typeStr != it.current()->text(1))
        typeStat = 1;
    }
    itemCounter++;
    ++it;
  }

  if (typeStat == -1 || typeStat == 1)
    typeStat = 4;
  else
    typeStat = convtType(typeStr);

  if (selID.size() == 1)
    singleCovUpdate(typeStat);
  else
    multiCovUpdate(typeStat);
}

/* convtType() translates input covariate's type into an integer.
 * returns 0 for "I", 1 for "N", 2 for "K", 3 for "D", 4 otherwise */
int Gdw::convtType(QString inputType) {
  if (inputType == "I")
    return 0;
  else if (inputType == "N")
    return 1;
  else if (inputType == "K")
    return 2;
  else if (inputType == "D")
    return 3;
  else
    return 4;
}

/* varListView selection update: when there is exactly one covariate highlighted
 */
void Gdw::singleCovUpdate(int covType) {
  typeCombo->setCurrentItem(covType);
  QString idStr = varListView->currentItem()->text(2);
  int covID = idStr.toInt();
  emit(newCovLoaded(covList[covID]));
}

/* multi-selection (includes both multi and extended) update in varListView */
void Gdw::multiCovUpdate(int covType) {
  typeCombo->setCurrentItem(covType);
  timeVector = fftVector = 0;
  statusBar()->clear();
  upperWindow->clear();
  upperWindow->update();
}

/* loadSingleCov() highlights a new item and shows it in upper plot window
 * Selection change signal/slot pair is disbaled, upper plot window is updated
 * manually. Hopefully it will make the update faster. */
void Gdw::loadSingleCov(Q3ListViewItem *newItem, int covType) {
  QObject::disconnect(varListView, SIGNAL(selectionChanged()), this,
                      SLOT(selectionUpdate()));

  selID.clear();
  selID.push_back(covList.size() - 1);
  itemCounter = 1;
  varListView->clearSelection();
  varListView->setSelected(newItem, true);
  varListView->setCurrentItem(newItem);
  singleCovUpdate(covType);
  setGUpdateFlag(true);

  QObject::connect(varListView, SIGNAL(selectionChanged()), this,
                   SLOT(selectionUpdate()));
}

/* dbClick() takes care of the double click signal from varListView */
void Gdw::dbClick(Q3ListViewItem *newItem) {
  // Don't do anything if no item is selected
  if (!newItem) return;

  // If clicked item isn't "All", enable renaming option
  newItem->setRenameEnabled(0, true);
  newItem->startRename(0);
}

/* setVarType() is a slot that takes care of typeCombo's activated() signal.
 * It will change selected covariates(s)' type according to typeCombo's new
 * hightlighted item index. */
void Gdw::setVarType(int typeIndex) {
  if (!selID.size()) return;

  // The last item in typeCombo is reserved
  if (typeIndex == 4) {
    QMessageBox::critical(
        0, "Error", "Blank covariate type is reserved for display purpose!");
    return;
  }

  // Type D is available for only one covariate
  if (typeIndex == 3) {
    // Error message if D already exists
    if (d_index != -1) {
      QMessageBox::critical(0, "Error",
                            "D type covariate already exists. <br>\
Only one covariate could be set to type D!");
      return;
    }
    // Error message if more than one covariate is selected to set to type D
    if (selID.size() > 1) {
      QMessageBox::critical(0, "Error",
                            "Only one covariate could be set to type D!");
      return;
    }
    varListView->findItem(QString::number(selID[0]), 2)->setText(1, "D");
    d_index = selID[0];
    return;
  }

  // Time to deal with I/N/K types
  QString newType;
  if (typeIndex == 0)
    newType = "I";
  else if (typeIndex == 1)
    newType = "N";
  else
    newType = "K";

  Q3ListViewItemIterator it(varListView, Q3ListViewItemIterator::Selected);
  QString typeStr;
  while (it.current()) {
    typeStr = it.current()->text(1);
    if (!typeStr.isEmpty() && typeStr != newType)
      it.current()->setText(1, newType);
    ++it;
  }
  // If type D is one of the selected for I/N/K, reset d_index to -1
  if (chkD()) d_index = -1;
}

/* This function launches block design interface &*/
void Gdw::showBlockUI() {
  // check TR
  if (!TR) {
    QMessageBox::critical(0, "Error!", "TR is required for model design");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!",
                          "Number of time points is required for model design");
    return;
  }
  // make sure TR is good
  if (TR % tmpResolve) {
    QMessageBox::critical(0, "Error",
                          "TR is not a multiple of upsampling rate.");
    return;
  }

  this->setDisabled(true);

  myBlock = new BlockDesign();
  myBlock->show();
  myBlock->setFixedSize(myBlock->width(), myBlock->height());

  myBlock->nameEditor->setText("block");
  myBlock->nameEditor->setFocus();
  myBlock->nameEditor->selectAll();

  QIntValidator *onVal = new QIntValidator(myBlock->onEditor);
  onVal->setBottom(1);
  myBlock->onEditor->setValidator(onVal);
  QIntValidator *offVal = new QIntValidator(myBlock->offEditor);
  offVal->setBottom(1);
  myBlock->offEditor->setValidator(offVal);
  QIntValidator *numberVal = new QIntValidator(myBlock->numberEditor);
  numberVal->setBottom(2);
  myBlock->numberEditor->setValidator(numberVal);

  QObject::connect(myBlock->okButton, SIGNAL(clicked()), this, SLOT(okBlock()));
  QObject::connect(myBlock->cancelButton, SIGNAL(clicked()), this,
                   SLOT(cancelBlock()));
}

/* This function deals with the signal emitted from "ok" button on block design
 */
void Gdw::okBlock() {
  if (!chkBlockUI()) return;

  myBlock->close();
  this->setEnabled(true);

  if (!totalReps) {
    totalReps = totalReps_block;
    numberString->setText(QString::number(totalReps));
  }

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

  int upRatio = TR / tmpResolve;
  VB_Vector *newVector = upSampling(blockVec, upRatio);
  covList.push_back(newVector);
  QString covID = QString::number(covList.size() - 1);
  delete blockVec;
  Q3ListViewItem *newItem =
      new Q3ListViewItem(varListView, getLastChild(varListView),
                         myBlock->nameEditor->text(), "I", covID);
  // Add intercept
  if (!interceptID.size()) addIntercept();

  // Highlights the block design covariate
  loadSingleCov(newItem, convtType("I"));
}

/* This function deals with the signal emitted from "cancel" button on block
 * design */
void Gdw::cancelBlock() {
  myBlock->close();
  this->setEnabled(true);
}

/* This function checks the input on block design interface to make sure:
 * (1) Effect name field is not blank;
 * (2) Block on/off length is positive integer;
 * (3) Number of blocks is an integer larger than 1. */
bool Gdw::chkBlockUI() {
  QString blockStr = myBlock->nameEditor->text();
  if (blockStr.isEmpty()) {
    QMessageBox::critical(myBlock, "Error", "Effect name not found.");
    return false;
  }

  int onLength = myBlock->onEditor->text().toInt();
  if (onLength < 1) {
    QMessageBox::critical(myBlock, "Error", "Minimum block length of on is 1.");
    return false;
  }

  int offLength = myBlock->offEditor->text().toInt();
  if (offLength < 1) {
    QMessageBox::critical(myBlock, "Error",
                          "Minimum block length of off is 1.");
    return false;
  }

  if (myBlock->ms->isChecked()) {
    if (onLength % TR != 0) {
      QMessageBox::critical(myBlock, "Error",
                            "Length of on block must be a multiple of TR.");
      return false;
    }
    if (offLength % TR != 0) {
      QMessageBox::critical(myBlock, "Error",
                            "Length of off block must be a multiple of TR.");
      return false;
    }

    onLength = onLength / TR;
    offLength = offLength / TR;
  }

  int blockNum = myBlock->numberEditor->text().toInt();
  if (blockNum < 2) {
    QMessageBox::critical(myBlock, "Error",
                          "Minimum value of number of blocks is 2.");
    return false;
  }

  first = 1, second = 0;
  firstLen = onLength, secondLen = offLength;
  int pairNum = blockNum / 2;
  int endLen = 0;
  if (myBlock->offFirst->isChecked()) {
    firstLen = offLength, secondLen = onLength;
    first = 0, second = 1;
  }
  if (blockNum % 2) endLen = firstLen;

  totalReps_block = (firstLen + secondLen) * pairNum + endLen;
  if (totalReps && totalReps != totalReps_block) {
    QMessageBox::critical(myBlock, "Error",
                          "Number of points on this interface doesn't match "
                          "the value on main interface.");
    return false;
  }

  return true;
}

/* This function launches paired design interface */
void Gdw::showPairUI() {
  // check TR
  if (!TR) {
    QMessageBox::critical(0, "Error!", "TR is required for model design");
    return;
  }
  // check totalReps
  if (!totalReps) {
    QMessageBox::critical(0, "Error!",
                          "Number of time points is required for model design");
    return;
  }
  // make sure TR is good
  if (TR % tmpResolve) {
    QMessageBox::critical(0, "Error",
                          "TR is not a multiple of upsampling rate.");
    return;
  }

  this->setDisabled(true);

  myPair = new PairDesign();
  myPair->show();
  myPair->setFixedSize(myPair->width(), myPair->height());

  myPair->nameEditor->setFocus();
  myPair->nameEditor->selectAll();
  QIntValidator *numberVal = new QIntValidator(myPair->numberEditor);
  numberVal->setBottom(2);
  myPair->numberEditor->setValidator(numberVal);

  QObject::connect(myPair->okButton, SIGNAL(clicked()), this, SLOT(okPair()));
  QObject::connect(myPair->cancelButton, SIGNAL(clicked()), this,
                   SLOT(cancelPair()));
}

/* This slot takes care of "ok" button click from paired design interface */
void Gdw::okPair() {
  if (!chkPairUI()) return;

  myPair->close();
  this->setEnabled(true);

  if (!totalReps) {
    totalReps = totalReps_pair;
    numberString->setText(QString::number(totalReps));
  }

  VB_Vector *mainVec = new VB_Vector(totalReps);
  mainVec->setAll(0);
  if (myPair->group->isChecked()) {
    for (int i = totalReps / 2; i < totalReps; i++) mainVec->setElement(i, 1);
  } else {
    for (int i = 1; i < totalReps; i += 2) mainVec->setElement(i, 1);
  }
  VB_Vector *mainVec_up = upSampling(mainVec, TR / tmpResolve);
  covList.push_back(mainVec_up);
  delete mainVec;
  // add the first group covariate, whose name is the input effect name string
  QString idStr = QString::number(covList.size() - 1);
  Q3ListViewItem *effect_item =
      new Q3ListViewItem(varListView, getLastChild(varListView),
                         myPair->nameEditor->text(), "I", idStr);

  // add a new group to include subject covariates
  Q3ListViewItem *pairGrp =
      new Q3ListViewItem(varListView, getLastChild(varListView), "subjects");
  pairGrp->setOpen(true);
  // add the other n-1 covariates into the tree view
  for (int i = 0; i < totalReps / 2 - 1; i++) {
    VB_Vector *subVec = new VB_Vector(totalReps);
    subVec->setAll(0);
    if (myPair->group->isChecked()) {
      subVec->setElement(i, 1);
      subVec->setElement(i + totalReps / 2, 1);
    } else {
      subVec->setElement(2 * i, 1);
      subVec->setElement(2 * i + 1, 1);
    }
    VB_Vector *subVec_up = upSampling(subVec, TR / tmpResolve);
    covList.push_back(subVec_up);
    idStr = QString::number(covList.size() - 1);
    QString subName = "subject " + QString::number(i + 1);
    (void)new Q3ListViewItem(pairGrp, getLastChild(pairGrp), subName, "I",
                             idStr);
    delete subVec;
  }

  // Add intercept
  if (!interceptID.size()) addIntercept();

  // Highlights the block design covariate
  loadSingleCov(effect_item, convtType("I"));
}

/* This slot takes care of "cancel" button click from paired design interface */
void Gdw::cancelPair() {
  myPair->close();
  this->setEnabled(true);
}

/* This function checks the input on paired design interface */
bool Gdw::chkPairUI() {
  QString nameStr = myPair->nameEditor->text();
  if (nameStr.isEmpty()) {
    QMessageBox::critical(myPair, "Error", "Effect name not found.");
    return false;
  }

  int subNum = myPair->numberEditor->text().toInt();
  if (subNum < 2) {
    QMessageBox::critical(myPair, "Error",
                          "Minimum value of number of subjects is 2.");
    return false;
  }

  totalReps_pair = 2 * subNum;
  if (totalReps && totalReps != totalReps_pair) {
    QMessageBox::critical(myPair, "Error",
                          "Number of points on this interface doesn't match "
                          "the value on main interface.");
    return false;
  }

  return true;
}

/**********************************************************************
 * G_Contrast class definition
 **********************************************************************/
G_Contrast::G_Contrast(int keyNum, int inputResolve, QWidget *parent,
                       const char *name)
    : QWidget(parent, name) {
  // Set basic parameters
  condKeyNum = keyNum;
  tmpResolve = inputResolve;

  // QT interface design
  Q3BoxLayout *contrast_main = new Q3VBoxLayout(this, 10);
  // First is explanation text
  QLabel *text = new QLabel(
      "Enter a contrast matrix below to define your covariates. <p>You need to "
      "enter " +
          QString::number(keyNum) +
          " values each separated by a space on each row.\
<p>Leave a column of zeroes for the baseline condition. \
<p>Each row defines a separate covariate.",
      this);
  contrast_main->addWidget(text);

  // matrix line input box
  matrixText = new Q3TextEdit(this);
  contrast_main->addWidget(matrixText);

  // Two push buttons after matrix input box: "Diagonal matrix" and "Show
  // example line"
  Q3HBoxLayout *pushButtons = new Q3HBoxLayout(contrast_main, 30);
  QPushButton *diagonalMat = new QPushButton("Diagonal matrix", this);
  pushButtons->addWidget(diagonalMat);
  QObject::connect(diagonalMat, SIGNAL(clicked()), this, SLOT(makeDiagMat()));

  QPushButton *addZeroLn = new QPushButton("Show example line", this);
  pushButtons->addWidget(addZeroLn);
  QObject::connect(addZeroLn, SIGNAL(clicked()), this, SLOT(addZeroLine()));

  // Two radio buttons for contrast options: scale, center and normalize
  Q3HButtonGroup *scaleButtons = new Q3HButtonGroup(this);
  scaleButtons->setLineWidth(0);  // hide this button group's border
  QRadioButton *noScale = new QRadioButton("Do not scale", scaleButtons);
  noScale->setFixedWidth(100);
  QRadioButton *scale = new QRadioButton("Scale by trial count", scaleButtons);
  scale->setFixedWidth(150);
  scale->setChecked(true);
  scaleFlag = 1;
  contrast_main->addWidget(scaleButtons);
  QObject::connect(noScale, SIGNAL(clicked()), this, SLOT(noScaleClicked()));
  QObject::connect(scale, SIGNAL(clicked()), this, SLOT(scaleClicked()));

  Q3HButtonGroup *offsetButtons = new Q3HButtonGroup(this);
  offsetButtons->setLineWidth(0);  // hide this button group's border
  QRadioButton *leaveOffset = new QRadioButton("Leave offset", offsetButtons);
  leaveOffset->setFixedWidth(100);
  QRadioButton *centerNorm =
      new QRadioButton("Center and normalize", offsetButtons);
  centerNorm->setFixedWidth(150);
  centerNorm->setChecked(true);
  centerFlag = 1;
  contrast_main->addWidget(offsetButtons);
  QObject::connect(leaveOffset, SIGNAL(clicked()), this, SLOT(offsetClicked()));
  QObject::connect(centerNorm, SIGNAL(clicked()), this, SLOT(centerClicked()));

  // Convolution or not? (By default it is disabled)
  Q3BoxLayout *convolution = new Q3VBoxLayout(contrast_main, 10);
  convolution->setMargin(10);
  convButt = new QRadioButton("Convolve Contrast Covariate(s)", this);
  convolution->addWidget(convButt);
  QObject::connect(convButt, SIGNAL(clicked()), this, SLOT(convClicked()));
  // Set convolution kernel (filter)
  Q3BoxLayout *filterBox = new Q3HBoxLayout(convolution);
  filter = new QLabel("Filter: ", this);
  combo = new QComboBox(this);
  combo->insertItem("Load");
  combo->insertItem("First eigenvector");
  combo->insertItem("Blocked IRF");
  combo->insertItem("Poisson");
  filterBox->addWidget(filter);
  filterBox->addWidget(combo);
  filterBox->addStretch(5);  // Force the widgets to shrink to minimum size
  filterName = new QLabel(" ", this);
  convolution->addWidget(filterName);
  QObject::connect(combo, SIGNAL(activated(int)), this,
                   SLOT(filterSelected(int)));
  // Set convolution kernel sampling rate
  Q3BoxLayout *sampleBox = new Q3HBoxLayout(convolution);
  samplingLab = new QLabel("Filter sampling (msecs): ", this);
  QString tmpString("2000");
  samplingEditor = new QLineEdit(tmpString, this);
  sampleBox->addWidget(samplingLab);
  sampleBox->addWidget(samplingEditor);
  sampleBox->addStretch(5);
  // Set convolution tag (for labelling purpose only)
  Q3BoxLayout *tagBox = new Q3HBoxLayout(convolution);
  tag = new QLabel("Label tag: ", this);
  tagEditor = new QLineEdit(this);
  tagBox->addWidget(tag);
  tagBox->addWidget(tagEditor);
  tagBox->addStretch(5);

  // By default, disable convolution part
  convStat = false;
  filter->setEnabled(false);
  combo->setEnabled(false);
  filterName->setEnabled(false);
  samplingLab->setEnabled(false);
  samplingEditor->setEnabled(false);
  tag->setEnabled(false);
  tagEditor->setEnabled(false);

  // "Cancel" and "Done" buttons at the bottom
  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(contrast_main, 30);
  QPushButton *cancel = new QPushButton("Cancel", this);
  QPushButton *done = new QPushButton("Done", this);
  cancel->setMaximumWidth(80);
  done->setMaximumWidth(80);
  finalFunct->addWidget(cancel, 4);
  finalFunct->addWidget(done, 4);
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

/* Deconstructor does nothing so far */
G_Contrast::~G_Contrast() {}

/* makeDiagMat() clears matrix text editor box and
 * add diagnal matrix elements in that area. */
void G_Contrast::makeDiagMat() {
  matrixText->blockSignals(
      true);  // Disable signals from matrixText temporariliy
  matrixText->clear();
  QString tmp;
  for (int i = 1; i < condKeyNum; i++) {
    tmp = "";
    for (int j = 0; j < condKeyNum; j++) {
      if (i == j)
        tmp = tmp + "1 ";
      else
        tmp = tmp + "0 ";
    }
    matrixText->insertParagraph(tmp, -1);
  }
  matrixText->blockSignals(false);  // Now enable signals from matrixText
}

/* loadMatrix() is supposed to be working when "Load matrix" button is clicked.
 * Since the button isn't that helpful and Geoff's IDL code doesn't make sense
 * to me either, this functionality isn't available now. */
void G_Contrast::loadMatrix() {}

/* addZeroLine() appends a line with all elements set to be zero.
 * It basically add one line as an example. Not that helpful.
 * To make it worse, this kind of format is actually invalid. */
void G_Contrast::addZeroLine() {
  matrixText->blockSignals(
      true);  // Disable signals from matrixText temporariliy
  QString tmp = "";
  for (int j = 0; j < condKeyNum; j++) tmp = tmp + "0 ";
  matrixText->insertParagraph(tmp, -1);
  matrixText->blockSignals(false);  // Now enable signals from matrixText
}

/* set scale flag to be false */
void G_Contrast::noScaleClicked() { scaleFlag = 0; }

/* set scale flag to be true */
void G_Contrast::scaleClicked() { scaleFlag = 1; }

/* set center flag to be false */
void G_Contrast::offsetClicked() { centerFlag = 0; }

/* set center flag to be true */
void G_Contrast::centerClicked() { centerFlag = 1; }

/* This slot enables convolution part when convolution setup radio button is
 * clicked */
void G_Contrast::convClicked() {
  if (convButt->isChecked())
    convStat = true;
  else
    convStat = false;

  filter->setEnabled(convStat);
  combo->setEnabled(convStat);
  filterName->setEnabled(convStat);
  samplingLab->setEnabled(convStat);
  samplingEditor->setEnabled(convStat);
  tag->setEnabled(convStat);
  tagEditor->setEnabled(convStat);
}

/* Slot to take care of the filter selection from combo box */
void G_Contrast::filterSelected(int selection) {
  QString filterPath = getFilterPath();
  switch (selection) {
    case 0:
      filterFile = Q3FileDialog::getOpenFileName(
          QString::null, "Ref files (*.ref)", this, "Load a filter file",
          "Please choose your filter.");
      if (filterFile.length() != 0) {
        filterName->setText("<b>" + filterFile + "</b>");
      }
      break;

    case 1:
      filterFile = filterPath + "Eigen1.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 2:
      filterFile = filterPath + "EmpiricalBlocked_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 3:
      filterFile = filterPath + "Poisson_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;
  }
}

/* Overloaded closeEvent() function */
void G_Contrast::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit cancelSignal(true);
}

/* slot to take care of "cancel" button */
void G_Contrast::cancelClicked() { this->close(); }

/* Slot for "Done" button. When this button is clicked, all rows are checked.
 * If ok the window is closed and a signal is emited for contrast matrix
 * building */
void G_Contrast::doneClicked() {
  // Make sure the text edit area isn't blank
  if (matrixText->text().isEmpty()) {
    QMessageBox::critical(this, "Error", "No matrix defined yet.");
    return;
  }

  int lineNum = matrixText->paragraphs();
  QString lineString;
  for (int i = 0; i < lineNum; i++) {
    lineString = (matrixText->text(i)).simplifyWhiteSpace();
    // error generated if the blank line isn't the last
    if (lineString.isEmpty() && i != lineNum - 1) {
      QMessageBox::critical(
          this, "Error",
          "Row " + QString::number(i + 1) + " invalid: blank line");
      return;
    }
    // ignore the last blank line
    else if (lineString.isEmpty() && i == lineNum - 1)
      continue;

    int spaceCount = 0;
    for (int j = 0; j < lineString.length(); j++) {
      if (lineString.at(j) == ' ') spaceCount++;
    }
    // Make sure elements on each line match condition keys
    if (spaceCount != condKeyNum - 1) {
      QMessageBox::critical(this, "Error",
                            "The number of elements on row " +
                                QString::number(i) +
                                " not equal to that of condition keys.");
      return;
    }

    QString numString;
    bool ok;
    double line[condKeyNum];
    for (int j = 0; j < condKeyNum; j++) {
      numString = lineString.section(" ", j, j);
      line[j] = numString.toDouble(&ok);
      // Make sure input elements are valid
      if (!ok) {
        QMessageBox::critical(
            this, "Error",
            "Invalid value on row " + QString::number(i) + ": " + numString);
        return;
      }
    }
    int k;
    for (k = 0; k < condKeyNum; k++) {
      if (line[k])
        break;
      else
        continue;
    }
    // Make sure each line has at least one non-zero element
    if (k == condKeyNum) {
      QMessageBox::critical(
          this, "Error",
          "Row " + QString::number(i) + " invalid: all elements are zero!");
      return;
    }
  }

  // Check convolution parameters
  if (!chkConvolve()) return;

  // Everything is fine now
  QString tagStr = tagEditor->text();
  this->close();
  emit doneSignal(matrixText, scaleFlag, centerFlag, convStat, convVector,
                  samplingVal, tagStr);
}

/* This function checks whether convolution parameters are set up
 * correctly or not;. Returns true if yes, returns false if not. */
bool G_Contrast::chkConvolve() {
  // Always pass if convolution is not enabled
  if (!convStat) return true;

  // Is the filter file already set up?
  if (filterFile.isEmpty()) {
    QMessageBox::critical(this, "Error", "No filter file loaded yet!");
    return false;
  }

  // Is the filter a valid ref format file?
  convVector = new VB_Vector();
  int readStat = convVector->ReadFile((const char *)filterFile);
  if (readStat) {
    QMessageBox::critical(this, "Error", "Invalid filter file: " + filterFile);
    return false;
    ;
  }

  samplingVal = samplingEditor->text().toDouble();
  // Is sampling value valid?
  if (samplingVal < (double)tmpResolve) {
    QMessageBox::critical(this, "Error",
                          "Convolution sampling rate must be equal to or "
                          "larger than G design sampling rate!");
    return false;
  }

  return true;
}

/**********************************************************************
 * G_DS class definition: for diagonal set covariates generation
 **********************************************************************/
G_DS::G_DS(int inputResolve, QWidget *parent, const char *name)
    : QWidget(parent, name) {
  // Set basic parameters
  tmpResolve = inputResolve;

  // QT interface design
  Q3BoxLayout *ds_main = new Q3VBoxLayout(this, 10);

  // Two radio buttons for contrast options: scale, center and normalize
  Q3HButtonGroup *scaleButtons = new Q3HButtonGroup(this);
  scaleButtons->setLineWidth(0);  // hide this button group's border
  QRadioButton *noScale = new QRadioButton("Do not scale", scaleButtons);
  noScale->setFixedWidth(100);
  QRadioButton *scale = new QRadioButton("Scale by trial count", scaleButtons);
  scale->setFixedWidth(150);
  noScale->setChecked(true);
  scaleFlag = 0;
  ds_main->addWidget(scaleButtons);
  QObject::connect(noScale, SIGNAL(clicked()), this, SLOT(noScaleClicked()));
  QObject::connect(scale, SIGNAL(clicked()), this, SLOT(scaleClicked()));

  Q3HButtonGroup *offsetButtons = new Q3HButtonGroup(this);
  offsetButtons->setLineWidth(0);  // hide this button group's border
  QRadioButton *leaveOffset = new QRadioButton("Leave offset", offsetButtons);
  leaveOffset->setFixedWidth(100);
  QRadioButton *centerNorm =
      new QRadioButton("Center and normalize", offsetButtons);
  centerNorm->setFixedWidth(150);
  centerNorm->setChecked(true);
  centerFlag = 1;
  ds_main->addWidget(offsetButtons);
  QObject::connect(leaveOffset, SIGNAL(clicked()), this, SLOT(offsetClicked()));
  QObject::connect(centerNorm, SIGNAL(clicked()), this, SLOT(centerClicked()));

  // Convolution or not? (By default it is disabled.) Mainly copied from
  // G_Convolve widget
  Q3BoxLayout *convolution = new Q3VBoxLayout(ds_main, 10);
  convolution->setMargin(10);
  convButt = new QRadioButton("Convolve Diagonal Set Covariate(s)", this);
  convolution->addWidget(convButt);
  QObject::connect(convButt, SIGNAL(clicked()), this, SLOT(convClicked()));
  // Set convolution kernel (filter)
  Q3BoxLayout *filterBox = new Q3HBoxLayout(convolution);
  filter = new QLabel("Filter: ", this);
  combo = new QComboBox(this);
  combo->insertItem("Load");
  combo->insertItem("First eigenvector");
  combo->insertItem("Blocked IRF");
  combo->insertItem("Poisson");
  filterBox->addWidget(filter);
  filterBox->addWidget(combo);
  filterBox->addStretch(5);  // Force the widgets to shrink to minimum size
  filterName = new QLabel(" ", this);
  convolution->addWidget(filterName);
  QObject::connect(combo, SIGNAL(activated(int)), this,
                   SLOT(filterSelected(int)));
  // Set convolution kernel sampling rate
  Q3BoxLayout *sampleBox = new Q3HBoxLayout(convolution);
  samplingLab = new QLabel("Filter sampling (msecs): ", this);
  QString tmpString("2000");
  samplingEditor = new QLineEdit(tmpString, this);
  sampleBox->addWidget(samplingLab);
  sampleBox->addWidget(samplingEditor);
  sampleBox->addStretch(5);
  // Set convolution tag (for labelling purpose only)
  Q3BoxLayout *tagBox = new Q3HBoxLayout(convolution);
  tag = new QLabel("Label tag: ", this);
  tagEditor = new QLineEdit(this);
  tagBox->addWidget(tag);
  tagBox->addWidget(tagEditor);
  tagBox->addStretch(5);

  // By default, disable convolution part
  convStat = false;
  filter->setEnabled(false);
  combo->setEnabled(false);
  filterName->setEnabled(false);
  samplingLab->setEnabled(false);
  samplingEditor->setEnabled(false);
  tag->setEnabled(false);
  tagEditor->setEnabled(false);

  // "Cancel" and "Done" buttons at the bottom
  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(ds_main, 30);
  QPushButton *cancel = new QPushButton("Cancel", this);
  QPushButton *done = new QPushButton("Done", this);
  cancel->setMaximumWidth(80);
  done->setMaximumWidth(80);
  finalFunct->addWidget(cancel, 4);
  finalFunct->addWidget(done, 4);
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

/* Deconstructor does nothing so far */
G_DS::~G_DS() {}

/* set scale flag to be false */
void G_DS::noScaleClicked() { scaleFlag = 0; }

/* set scale flag to be true */
void G_DS::scaleClicked() { scaleFlag = 1; }

/* set center flag to be false */
void G_DS::offsetClicked() { centerFlag = 0; }

/* set center flag to be true */
void G_DS::centerClicked() { centerFlag = 1; }

/* This slot enables convolution part when convolution setup radio button is
 * clicked */
void G_DS::convClicked() {
  if (convButt->isChecked())
    convStat = true;
  else
    convStat = false;

  filter->setEnabled(convStat);
  combo->setEnabled(convStat);
  filterName->setEnabled(convStat);
  samplingLab->setEnabled(convStat);
  samplingEditor->setEnabled(convStat);
  tag->setEnabled(convStat);
  tagEditor->setEnabled(convStat);
}

/* Slot to take care of the filter selection from combo box */
void G_DS::filterSelected(int selection) {
  QString filterPath = getFilterPath();
  switch (selection) {
    case 0:
      filterFile = Q3FileDialog::getOpenFileName(
          QString::null, "Ref files (*.ref)", this, "Load a filter file",
          "Please choose your filter.");
      if (filterFile.length() != 0) {
        filterName->setText("<b>" + filterFile + "</b>");
      }
      break;

    case 1:
      filterFile = filterPath + "Eigen1.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 2:
      filterFile = filterPath + "EmpiricalBlocked_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 3:
      filterFile = filterPath + "Poisson_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;
  }
}

/* Overloaded closeEvent() function */
void G_DS::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit cancelSignal(true);
}

/* slot to take care of "cancel" button */
void G_DS::cancelClicked() { this->close(); }

/* Slot for "Done" button. When this button is clicked, all rows are checked.
 * If ok the window is closed and a signal is emited for contrast matrix
 * building */
void G_DS::doneClicked() {
  // Check convolution parameters
  if (!chkConvolve()) return;

  // Everything is fine now
  QString tagStr = tagEditor->text();
  this->close();
  emit doneSignal(scaleFlag, centerFlag, convStat, convVector, samplingVal,
                  tagStr);
}

/* This function checks whether convolution parameters are set up
 * correctly or not;. Returns true if yes, returns false if not. */
bool G_DS::chkConvolve() {
  // Always pass if convolution is not enabled
  if (!convStat) return true;

  // Is the filter file already set up?
  if (filterFile.isEmpty()) {
    QMessageBox::critical(this, "Error", "No filter file loaded yet!");
    return false;
  }

  // Is the filter a valid ref format file?
  convVector = new VB_Vector();
  int readStat = convVector->ReadFile((const char *)filterFile);
  if (readStat) {
    QMessageBox::critical(this, "Error", "Invalid filter file: " + filterFile);
    return false;
    ;
  }

  samplingVal = samplingEditor->text().toDouble();
  // Is sampling value valid?
  if (samplingVal < (double)tmpResolve) {
    QMessageBox::critical(this, "Error",
                          "Convolution sampling rate must be equal to or "
                          "larger than G design sampling rate!");
    return false;
  }

  return true;
}

/**********************************************************
 * This is the class which defines the convolve interface *
 **********************************************************/
/* First comes the constructor */
G_Convolve::G_Convolve(int inputResolve, QWidget *parent, const char *name)
    : QWidget(parent, name) {
  tmpResolve = inputResolve;
  Q3BoxLayout *convolve_main = new Q3VBoxLayout(this, 10);
  Q3BoxLayout *filterBox = new Q3HBoxLayout(convolve_main);
  QLabel *filter = new QLabel("Filter: ", this);
  combo = new QComboBox(this);
  combo->insertItem("Load");
  combo->insertItem("First eigenvector");
  combo->insertItem("Blocked IRF");
  combo->insertItem("Poisson");
  filterBox->addWidget(filter);
  filterBox->addWidget(combo);
  filterBox->addStretch(5);  // Force the widgets to shrink to minimum size

  filterName = new QLabel(" ", this);
  convolve_main->addWidget(filterName);

  QObject::connect(combo, SIGNAL(activated(int)), this,
                   SLOT(filterSelected(int)));

  Q3BoxLayout *sampleBox = new Q3HBoxLayout(convolve_main);
  QLabel *sampling = new QLabel("Filter sampling (msecs): ", this);
  QString tmpString("2000");
  samplingEditor = new QLineEdit(tmpString, this);
  sampleBox->addWidget(sampling);
  sampleBox->addWidget(samplingEditor);
  sampleBox->addStretch(5);

  Q3BoxLayout *tagBox = new Q3HBoxLayout(convolve_main);
  QLabel *tag = new QLabel("Label tag: ", this);
  tagEditor = new QLineEdit(this);
  tagBox->addWidget(tag);
  tagBox->addWidget(tagEditor);
  tagBox->addStretch(5);

  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(convolve_main, 30);
  QPushButton *cancel = new QPushButton("Cancel", this);
  QPushButton *done = new QPushButton("Done", this);
  finalFunct->addWidget(cancel);
  finalFunct->addWidget(done);
  finalFunct->addStretch(5);
  convolve_main->addSpacing(20);

  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

/* Deconstructor does nothing so far */
G_Convolve::~G_Convolve() {}

/* Slot to take care of the filter selection from combo box */
void G_Convolve::filterSelected(int selection) {
  QString filterPath = getFilterPath();
  switch (selection) {
    case 0:
      filterFile = Q3FileDialog::getOpenFileName(
          QString::null, "Ref files (*.ref)", this, "Load a filter file",
          "Please choose your filter.");
      if (filterFile.length() != 0) {
        filterName->setText("<b>" + filterFile + "</b>");
      }
      break;

    case 1:
      filterFile = filterPath + "Eigen1.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 2:
      filterFile = filterPath + "EmpiricalBlocked_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;

    case 3:
      filterFile = filterPath + "Poisson_IRF.ref";
      filterName->setText("<b>" + filterFile + "</b>");
      break;
  }
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Convolve::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "cancel" button */
void G_Convolve::cancelClicked() { this->close(); }

/* Slot to take care of the click of "Done" button */
void G_Convolve::doneClicked() {
  // Is the filter file already set up?
  if (filterFile.isEmpty()) {
    QMessageBox::critical(this, "Error", "No filter file loaded yet!");
    return;
  }

  // Is the filter a valid ref format file?
  convVector = new VB_Vector();
  int readStat = convVector->ReadFile((const char *)filterFile);
  if (readStat) {
    QMessageBox::critical(this, "Error", "Invalid filter file: " + filterFile);
    return;
  }

  double sampling = samplingEditor->text().toDouble();
  // Is sampling value valid?
  if (sampling < (double)tmpResolve) {
    QMessageBox::critical(this, "Error",
                          "Convolution sampling rate must be equal to or "
                          "larger than G design sampling rate!");
    return;
  }

  // Everything is ready, let's go
  this->setEnabled(false);
  QString tag = tagEditor->text();
  emit doneSignal(convVector, sampling,
                  tag);  // This signal doesn't include deltaFlag
  this->close();
  this->setEnabled(true);
}

/* This is the class which defines the derivative interface */
/* First comes the constructor */
G_Deriv::G_Deriv(QWidget *parent, const char *name) : QWidget(parent, name) {
  Q3BoxLayout *deriv_main = new Q3VBoxLayout(this, 10);

  Q3BoxLayout *numDeriv = new Q3HBoxLayout(deriv_main);
  QLabel *text = new QLabel("Number of derivatives: ", this);
  derivEdit = new QLineEdit("1", this);
  numDeriv->addWidget(text);
  numDeriv->addWidget(derivEdit);
  numDeriv->addStretch(5);

  Q3HButtonGroup *radioButtons = new Q3HButtonGroup(this);
  radioButtons->setLineWidth(0);
  (void)new QRadioButton("Interest", radioButtons);
  QRadioButton *noInterButt = new QRadioButton("No interest", radioButtons);
  noInterButt->setChecked(true);
  varType = 1;
  QObject::connect(radioButtons, SIGNAL(clicked(int)), this,
                   SLOT(setVarType(int)));
  deriv_main->addWidget(radioButtons);

  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(deriv_main, 30);
  QPushButton *done = new QPushButton("Done", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  finalFunct->addWidget(done);
  finalFunct->addWidget(cancel);
  finalFunct->addStretch(5);
  deriv_main->addSpacing(20);

  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

/* Deconstructor does nothing so far */
G_Deriv::~G_Deriv() {}

/* Slot to take care of the two radio buttons */
void G_Deriv::setVarType(int flag) { varType = flag; }

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Deriv::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "cancel" button */
void G_Deriv::cancelClicked() { this->close(); }

/* Slot to take care of the click of "Done" button */
void G_Deriv::doneClicked() {
  bool ok;
  unsigned numDeriv = derivEdit->text().toUInt(&ok);
  if (!ok || numDeriv == 0) {
    QMessageBox::critical(this, "Error",
                          "Invalid number of derivatives. Please try again.");
    return;
  }
  if (numDeriv > 18) {
    QMessageBox::information(this, "Info",
                             "The maximum of number of derivatives is 18. \
<p>Larger value will be reset to 18.");
    numDeriv = 18;
  }

  emit doneSignal(numDeriv, varType);
  this->close();
}

/* G_Orth defines the orthogonalize interface */
/* First comes the constructor */
G_Orth::G_Orth(Q3ListView *inputView, QWidget *parent, const char *name)
    : QWidget(parent, name) {
  Q3BoxLayout *orth_main = new Q3VBoxLayout(this, 10);
  QLabel *textLine1 =
      new QLabel("<b>Orthogonalize covariate with respect to:", this);
  orth_main->addWidget(textLine1);

  Q3BoxLayout *covLayout = new Q3HBoxLayout(orth_main);
  Q3VButtonGroup *radioButtons = new Q3VButtonGroup(this);
  radioButtons->setLineWidth(0);
  (void)new QRadioButton("all unselected", radioButtons);
  QRadioButton *interestButt =
      new QRadioButton("all unselected interest", radioButtons);
  (void)new QRadioButton("all unselected no interest", radioButtons);
  (void)new QRadioButton("specific unselected", radioButtons);
  QObject::connect(radioButtons, SIGNAL(clicked(int)), this,
                   SLOT(selectCov(int)));
  // Default is "all unselected interest"
  interestButt->setChecked(true);
  covGroup = 1;
  covLayout->addWidget(radioButtons);

  Q3BoxLayout *covBox = new Q3VBoxLayout(covLayout);
  QLabel *textLine2 = new QLabel("Pick Covariate:", this);
  covBox->addWidget(textLine2);

  // Build covView
  covView = new Q3ListView(this);
  covView->setSelectionMode(Q3ListView::Extended);
  covView->addColumn("Name");
  covView->addColumn("Type");
  covView->setColumnAlignment(1, Qt::AlignHCenter);
  covView->addColumn("ID");
  covView->setColumnAlignment(2, Qt::AlignHCenter);
  covView->setRootIsDecorated(true);
  covView->setSortColumn(-1);
  covBox->addWidget(covView);
  // Copy items from inputView
  Q3ListViewItemIterator it(inputView);
  while (it.current()) {
    cpItem(it.current());
    ++it;
  }
  // Don't allow the user to play with it, unless "specific unselected"
  // radiobutton is selected
  covView->setEnabled(false);

  Q3BoxLayout *botButt = new Q3HBoxLayout(orth_main);
  QPushButton *done = new QPushButton("Done", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  botButt->addWidget(done);
  botButt->addWidget(cancel);
  botButt->addStretch(5);
  orth_main->addSpacing(20);
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

/* Deconstructor does nothing so far */
G_Orth::~G_Orth() {}

/* cpItem() will recursively copy the input listViewItem, written for covariate
 * duplication orgDepth is input item's original depth */
void G_Orth::cpItem(Q3ListViewItem *inputItem) {
  Q3ListViewItem *newItem;
  // Copy a group item and disable it
  if (inputItem->text(2).isEmpty()) {
    // Ignore the empty group
    if (!inputItem->childCount()) return;
    if (inputItem->depth() == 0)
      newItem = new Q3ListViewItem(covView, getLastChild(covView),
                                   inputItem->text(0));
    else
      newItem = new Q3ListViewItem(findParent(inputItem),
                                   getLastChild(findParent(inputItem)),
                                   inputItem->text(0));
    newItem->setOpen(true);
    newItem->setEnabled(false);
    return;
  }
  // Copy a real covariate
  if (inputItem->depth() == 0)
    newItem =
        new Q3ListViewItem(covView, getLastChild(covView), inputItem->text(0),
                           inputItem->text(1), inputItem->text(2));
  else
    newItem = new Q3ListViewItem(
        findParent(inputItem), getLastChild(findParent(inputItem)),
        inputItem->text(0), inputItem->text(1), inputItem->text(2));
  // If input item is selected in original tree view, disable it
  if (inputItem->isSelected()) newItem->setEnabled(false);
  // Disable type D covariate too
  else if (inputItem->text(1) == "D")
    newItem->setEnabled(false);
  // By default, select covariates of type "I"
  else if (inputItem->text(1) == "I")
    newItem->setSelected(true);
}

/* Modified based on gdw's same function */
Q3ListViewItem *G_Orth::findParent(Q3ListViewItem *inputItem) {
  int lastDepth = covView->lastItem()->depth();
  int inputDepth = inputItem->depth();
  if (lastDepth < inputDepth) return covView->lastItem();

  Q3ListViewItem *newParent = covView->lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

/* Slot to highlight selected covariates in listbox on right side */
void G_Orth::selectCov(int selection) {
  // Don't do anything if the same radiobutton is clicked
  if (covGroup == selection) return;
  // Activate the tree view before change selection
  covView->setEnabled(true);
  // If "specific unselected" is clicked, keep the original selection and exit
  if (selection == 3) {
    covGroup = selection;
    return;
  }

  // typeStr is null, "I" or "N"
  QString typeStr;
  if (selection == 1)
    typeStr = "I";
  else if (selection == 2)
    typeStr = "N";

  // Clear the selection for the first 3 radiobuttons, generate selection
  // automatically
  covView->clearSelection();
  Q3ListViewItemIterator it(covView, Q3ListViewItemIterator::Selectable);
  while (it.current()) {
    if (!typeStr.length())
      it.current()->setSelected(true);
    else if (it.current()->text(1) == typeStr)
      it.current()->setSelected(true);
    ++it;
  }

  covView->setEnabled(false);
  covGroup = selection;
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Orth::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "cancel" button */
void G_Orth::cancelClicked() { this->close(); }

/* Slot to take care of the click of "done" button.
 * This function emits a signal which includes an array of integers that
 * lables the index/ID of selected covariates on orthogonalization interface. */
void G_Orth::doneClicked() {
  Q3ListViewItemIterator it(covView, Q3ListViewItemIterator::Selected);
  int j;
  while (it.current()) {
    // This condition is actually redundant, but it makes me feel safer
    if (!it.current()->text(2).isEmpty()) {
      j = it.current()->text(2).toInt();
      orthID.push_back(j);
    }
    ++it;
  }
  // Make sure at least one covariate is selected
  if (!orthID.size()) {
    QMessageBox::critical(0, "Error",
                          "No covariate selected in orhtogonalization option");
    return;
  }

  this->close();
  emit doneSignal(orthID);
}

/* Here comes a class for exponential interface */
G_Expn::G_Expn(QWidget *parent, const char *name) : QWidget(parent, name) {
  Q3BoxLayout *exp_main = new Q3VBoxLayout(this, 10);
  QLabel *text1 = new QLabel("Replace param with param^x", this);
  exp_main->addWidget(text1);

  Q3HBoxLayout *inputLine = new Q3HBoxLayout(exp_main);
  QLabel *text2 = new QLabel("Exponential (x): ", this);
  inputBox = new QLineEdit("2", this);
  inputLine->addWidget(text2);
  inputLine->addWidget(inputBox);

  Q3HButtonGroup *offsetButtons = new Q3HButtonGroup(this);
  offsetButtons->setLineWidth(0);  // hide this button group's border
  (void)new QRadioButton("Leave offset", offsetButtons);
  QRadioButton *centerNorm =
      new QRadioButton("Center and normalize", offsetButtons);
  centerNorm->setChecked(true);
  centerFlag = 1;
  exp_main->addWidget(offsetButtons);
  QObject::connect(offsetButtons, SIGNAL(clicked(int)), this,
                   SLOT(setFlag(int)));

  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(exp_main, 30);
  QPushButton *done = new QPushButton("Done", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  cancel->setMaximumWidth(80);
  done->setMaximumWidth(80);
  finalFunct->addWidget(done);
  finalFunct->addWidget(cancel);

  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

/* Deconstructor does nothing so far */
G_Expn::~G_Expn() {}

/* Slot to take care of the radio buttons to set whether center and
 * normalization is needed */
void G_Expn::setFlag(int flag) { centerFlag = flag; }

/* Slot to take care of the click of "Done" button */
void G_Expn::doneClicked() {
  QString expText = inputBox->text();
  bool ok;
  double expVal = expText.toDouble(&ok);

  if (!ok) {
    QMessageBox::critical(this, "Error", "Invalid exponenetial value!");
    return;
  }

  emit doneSignal(expVal, centerFlag);
  this->close();
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Expn::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "cancel" button */
void G_Expn::cancelClicked() { this->close(); }

/* Class for multiply interface */
G_Multiply::G_Multiply(Q3ListView *inputView, QWidget *parent, const char *name)
    : QWidget(parent, name) {
  Q3BoxLayout *multpl_main = new Q3VBoxLayout(this, 10);
  QLabel *text1 = new QLabel("Pick covariate by which to multiply", this);
  multpl_main->addWidget(text1);

  covView = new Q3ListView(this);
  covView->addColumn("Name");
  covView->addColumn("Type");
  covView->setColumnAlignment(1, Qt::AlignHCenter);
  covView->addColumn("ID");
  covView->setColumnAlignment(2, Qt::AlignHCenter);
  covView->setRootIsDecorated(true);
  covView->setSortColumn(-1);
  multpl_main->addWidget(covView);

  Q3ListViewItemIterator it(inputView);
  while (it.current()) {
    cpItem(it.current());
    ++it;
  }

  Q3BoxLayout *botButt = new Q3HBoxLayout(multpl_main);
  QPushButton *done = new QPushButton("Done", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  botButt->addWidget(done);
  botButt->addWidget(cancel);
  botButt->addStretch(5);
  multpl_main->addSpacing(20);

  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

/* Deconstructor does nothing so far */
G_Multiply::~G_Multiply() {}

/* cpItem() will recursively copy the input listViewItem, written for covariate
 * duplication orgDepth is input item's original depth */
void G_Multiply::cpItem(Q3ListViewItem *inputItem) {
  if (inputItem->text(2).isEmpty()) {
    if (!inputItem->childCount()) return;
    Q3ListViewItem *newItem;
    if (inputItem->depth() == 0)
      newItem = new Q3ListViewItem(covView, getLastChild(covView),
                                   inputItem->text(0));
    else
      newItem = new Q3ListViewItem(findParent(inputItem),
                                   getLastChild(findParent(inputItem)),
                                   inputItem->text(0));
    newItem->setOpen(true);
    newItem->setEnabled(false);
    return;
  }

  if (inputItem->depth() == 0)
    (void)new Q3ListViewItem(covView, getLastChild(covView), inputItem->text(0),
                             inputItem->text(1), inputItem->text(2));
  else
    (void)new Q3ListViewItem(
        findParent(inputItem), getLastChild(findParent(inputItem)),
        inputItem->text(0), inputItem->text(1), inputItem->text(2));
}

/* Modified based on gdw's same function */
Q3ListViewItem *G_Multiply::findParent(Q3ListViewItem *inputItem) {
  int lastDepth = covView->lastItem()->depth();
  int inputDepth = inputItem->depth();
  if (lastDepth < inputDepth) return covView->lastItem();

  Q3ListViewItem *newParent = covView->lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Multiply::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "Cancel" button */
void G_Multiply::cancelClicked() { this->close(); }

/* Slot to take care of the click of "Done" button
 * When "Done" is clicked, it simply emit a signal
 * which includes the index number of the selected covariate. */
void G_Multiply::doneClicked() {
  if (!covView->currentItem()) {
    QMessageBox::critical(0, "Error", "No covariate selected");
    return;
  }

  int selection = covView->currentItem()->text(2).toInt();
  this->close();
  emit(doneSignal(selection));
}

/* Class for Fourier set interface */
G_Fourier::G_Fourier(double timeLength, QWidget *parent, const char *name)
    : QWidget(parent, name) {
  totalTime = timeLength;
  Q3BoxLayout *fourier_main = new Q3VBoxLayout(this, 10);
  QLabel *finalText = new QLabel(
      "This tool will convolve your selected parameters with \
a set of sines and cosines that are harmonics of the period to be modeled, starting \
with the first harmonic. \
<br>The zeroeth frequency (DC component) of the period can also be included if desired. \
<br>The \"Delta covariate\" option will convert your input parameters to simple delta \
functions (recommended).<p>",
      this);
  fourier_main->addWidget(finalText);

  Q3BoxLayout *line1 = new Q3HBoxLayout(fourier_main);
  QLabel *label1 = new QLabel("Period to model (seconds): ", this);
  periodEditor = new QLineEdit("0", this);
  QLabel *label2 = new QLabel("Number of harmonics: ", this);
  numberEditor = new QLineEdit("0", this);

  line1->addWidget(label1);
  line1->addWidget(periodEditor);
  line1->addWidget(label2);
  line1->addWidget(numberEditor);

  Q3HButtonGroup *group1 = new Q3HButtonGroup(this);
  group1->setLineWidth(0);  // hide this button group's border
  QRadioButton *button1 = new QRadioButton("DO NOT add freq zero", group1);
  (void)new QRadioButton("DO add freq zero", group1);
  button1->setChecked(true);
  addFlag = 0;
  fourier_main->addWidget(group1);
  QObject::connect(group1, SIGNAL(clicked(int)), this, SLOT(setAddZero(int)));

  Q3HButtonGroup *group2 = new Q3HButtonGroup(this);
  group2->setLineWidth(0);  // hide this button group's border
  (void)new QRadioButton("DO NOT alter covariates", group2);
  QRadioButton *button2 = new QRadioButton("Delta covariates", group2);
  button2->setChecked(true);
  deltaFlag = 1;
  fourier_main->addWidget(group2);
  QObject::connect(group2, SIGNAL(clicked(int)), this, SLOT(setDelta(int)));

  Q3HBoxLayout *finalFunct = new Q3HBoxLayout(fourier_main, 30);
  QPushButton *done = new QPushButton("Done", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  cancel->setMaximumWidth(80);
  done->setMaximumWidth(80);
  finalFunct->addWidget(done);
  finalFunct->addWidget(cancel);

  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  QObject::connect(done, SIGNAL(clicked()), this, SLOT(doneClicked()));
}

/* Destructor doesn't do anything now */
G_Fourier::~G_Fourier() {}

/* Set "add zero frequency" flag, 0 is no, 1 is yes */
void G_Fourier::setAddZero(int input) { addFlag = input; }

/* Set "delta covariate" flag, 0 is no, 1 is yes */
void G_Fourier::setDelta(int input) { deltaFlag = input; }

/* Slot to take care of the click of "Done" button */
void G_Fourier::doneClicked() {
  bool ok;
  // Convert period value
  QString periodTxt = periodEditor->text();
  int period = periodTxt.toInt(&ok);
  if (!ok || period <= 0) {
    QMessageBox::critical(this, "Error", "Invalid period value!");
    return;
  }
  if ((double)period > totalTime) {
    QMessageBox::critical(this, "Error",
                          "Period must be less than or equal to " +
                              QString::number(totalTime) + " seconds!");
    return;
  }

  // Convert number of harmonics
  QString numberTxt = numberEditor->text();
  int number = numberTxt.toInt(&ok);
  if (!ok || number <= 0) {
    QMessageBox::critical(this, "Error", "Invalid number of harmonics!");
    return;
  }

  emit doneSignal(period, number, addFlag, deltaFlag);
  this->close();
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Fourier::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot to take care of the click of "cancel" button */
void G_Fourier::cancelClicked() { this->close(); }

/************************************************************************
 *             Class for efficiency evaluation Interface
 ************************************************************************/
G_Eff::G_Eff(Q3ListView *inputView, std::vector<VB_Vector *> inputCovList,
             int inTR, int inTotalReps, int inTmpResolve, QWidget *parent,
             const char *name)
    : QWidget(parent, name) {
  int covNum = inputCovList.size();
  init(inTR, inTotalReps, inTmpResolve, covNum);
  for (int i = 0; i < covNum; i++) covList.push_back(inputCovList[i]);

  Q3BoxLayout *eff_main = new Q3VBoxLayout(this, 10);
  QLabel *text1 = new QLabel("<b>Select a filter to evaluate:</b> ", this);
  eff_main->addWidget(text1);

  Q3VButtonGroup *radioButtons1 = new Q3VButtonGroup(this);
  radioButtons1->setLineWidth(0);
  QRadioButton *butt1 = new QRadioButton("Do not downsample", radioButtons1);
  (void)new QRadioButton("Downsample before convolution", radioButtons1);
  (void)new QRadioButton("Downsample after convolution", radioButtons1);
  butt1->setChecked(true);
  downFlag = 0;
  QObject::connect(radioButtons1, SIGNAL(clicked(int)), this,
                   SLOT(setDownFlag(int)));
  eff_main->addWidget(radioButtons1);

  QLabel *filter = new QLabel("<b>Filter:</b> ", this);
  combo = new QComboBox(this);
  combo->insertItem("None");
  combo->insertItem("Load");
  combo->insertItem("First eigenvector");
  combo->insertItem("Blocked IRF");
  combo->insertItem("Poisson");
  eff_main->addWidget(filter);
  eff_main->addWidget(combo);

  filterLabel = new QLabel("", this);
  eff_main->addWidget(filterLabel);
  QObject::connect(combo, SIGNAL(activated(int)), this,
                   SLOT(filterSelected(int)));

  Q3BoxLayout *displayBox = new Q3HBoxLayout(eff_main);
  Q3BoxLayout *leftDisplay = new Q3VBoxLayout(displayBox);
  QLabel *text2 = new QLabel("<b>Display:</b>", this);
  leftDisplay->addWidget(text2, 1, Qt::AlignCenter);

  Q3VButtonGroup *radioButtons2 = new Q3VButtonGroup(this);
  radioButtons2->setLineWidth(0);
  (void)new QRadioButton("All covariates", radioButtons2);
  QRadioButton *interestButt =
      new QRadioButton("All of interest", radioButtons2);
  (void)new QRadioButton("All of no interest", radioButtons2);
  interestButt->setChecked(true);
  typeFlag = 1;
  QObject::connect(radioButtons2, SIGNAL(clicked(int)), this,
                   SLOT(selectCovType(int)));
  leftDisplay->addWidget(radioButtons2, 8, Qt::AlignTop);

  Q3BoxLayout *rightDisplay = new Q3VBoxLayout(displayBox);
  QLabel *text3 =
      new QLabel("<b>Covariates and their relative efficiencies:</b> ", this);
  rightDisplay->addWidget(text3);

  // Copy the input tree, select 1st selectable covariate and set baseIndex
  cpView(inputView);
  Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
  if (all.current()) {
    covView->setSelected(all.current(), true);
    baseIndex = all.current()->text(2).toInt();
  }

  QObject::connect(covView, SIGNAL(selectionChanged(Q3ListViewItem *)), this,
                   SLOT(modEffBase(Q3ListViewItem *)));
  rightDisplay->addWidget(covView);

  Q3BoxLayout *cutoffBox = new Q3HBoxLayout(eff_main);
  QLabel *text4 = new QLabel("<b>Efficiency cut-off:</b> ", this);
  cutoffBox->addWidget(text4);
  cutoffEdit = new QLineEdit("0.02", this);
  cutoffBox->addWidget(cutoffEdit);

  Q3BoxLayout *botButt = new Q3HBoxLayout(eff_main);
  QPushButton *delButt = new QPushButton("Delete", this);
  QPushButton *cancel = new QPushButton("Cancel", this);
  botButt->addWidget(delButt);
  botButt->addWidget(cancel);
  botButt->addStretch(5);
  QObject::connect(delButt, SIGNAL(clicked()), this, SLOT(delClicked()));
  QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

  modFilterVec();
  modRawEff();
}

/* Deconstructor does nothing so far */
G_Eff::~G_Eff() {
  if (filterVec) delete filterVec;
}

/* init() initialize some simple parameters */
void G_Eff::init(int inTR, int inTotalReps, int inTmpResolve, int covNum) {
  TR = inTR;
  totalReps = inTotalReps;
  tmpResolve = inTmpResolve;
  baseIndex = -1;
  for (int i = 0; i < covNum; i++) rawEff.push_back(-1.0);
  filterVec = new VB_Vector();
}

/* This slot will set the downsampling flag in radiobutton1 */
void G_Eff::setDownFlag(int selection) {
  downFlag = selection;
  modFilterVec();
  modRawEff();
}

/* Slot to take care of the filter selection from combo box */
void G_Eff::filterSelected(int selection) {
  QString filterPath = getFilterPath();
  switch (selection) {
    case 0:
      filterFile = QString::null;
      filterLabel->clear();
      break;
    case 1:
      filterFile = Q3FileDialog::getOpenFileName(
          QString::null, "Ref files (*.ref)", this, "Load a filter file",
          "Please choose your filter.");
      if (filterFile.length()) {
        filterLabel->setText("Filter filename: " + filterFile);
      }
      break;
    case 2:
      filterFile = filterPath + "Eigen1.ref";
      filterLabel->setText("Filter filename: " + filterFile);
      break;
    case 3:
      filterFile = filterPath + "EmpiricalBlocked_IRF.ref";
      filterLabel->setText("Filter filename: " + filterFile);
      break;
    case 4:
      filterFile = filterPath + "Poisson_IRF.ref";
      filterLabel->setText("Filter filename: " + filterFile);
      break;
  }

  modFilterVec();
  modRawEff();
}

/* Select proper covaraites when radio button is clicked */
void G_Eff::selectCovType(int selection) {
  typeFlag = selection;
  modView();
  modRawEff();
}

/* modEffBase() is the slot which replies the highlighted(int) signal
 * from newCovBox listbox. It will use the new highlighted covariate as
 * the base to update other covariates' efficiency. So the highlighted
 * covariate's efficiency will be always 1.0                          */
void G_Eff::modEffBase(Q3ListViewItem *newItem) {
  baseIndex = newItem->text(2).toInt();
  int tmpIndex;
  double baseEff = getRawEff(baseIndex);
  if (baseEff == -1.0 || baseEff == 0) {
    QMessageBox::critical(
        this, "Error!",
        QString(
            "Covariate #%1 has an invalid efficiency value! Evaluation aborted")
            .arg(baseIndex));
    Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
    while (all.current()) {
      all.current()->setText(3, QString::null);
      ++all;
    }
    return;
  }

  double tmpEff;
  Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
  while (all.current()) {
    tmpIndex = all.current()->text(2).toInt();
    tmpEff = rawEff[tmpIndex];
    all.current()->setText(3, getEffStr(tmpEff / baseEff));
    ++all;
  }
}

/* cpView() copies input QListView and add it on the main interface */
void G_Eff::cpView(Q3ListView *inputView) {
  covView = new Q3ListView(this);
  covView->addColumn("Name");
  covView->addColumn("Type");
  covView->setColumnAlignment(1, Qt::AlignHCenter);
  covView->addColumn("ID");
  covView->setColumnAlignment(2, Qt::AlignHCenter);
  covView->addColumn("Efficiency");
  covView->setColumnAlignment(3, Qt::AlignHCenter);
  covView->setRootIsDecorated(true);
  covView->setSortColumn(-1);
  Q3ListViewItemIterator it(inputView);
  while (it.current()) {
    cpItem(it.current());
    ++it;
  }
}

/* modViewEff() only modifies efficiency value, not enable/disable items in
 * covView */
void G_Eff::modRawEff() {
  // Don't do anything if no valid base covariate
  if (baseIndex == -1) return;

  double baseEff = getRawEff(baseIndex);
  if (baseEff == -1 || baseEff == 0) {
    QMessageBox::critical(
        this, "Error!",
        QString(
            "Covariate #%1 has an invalid efficiency value! Evaluation aborted")
            .arg(baseIndex));
    Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
    while (all.current()) {
      all.current()->setText(3, QString::null);
      ++all;
    }
    return;
  }

  double tmpEff;
  int tmpIndex;
  Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
  while (all.current()) {
    tmpIndex = all.current()->text(2).toInt();
    tmpEff = getRawEff(tmpIndex);
    if (tmpEff == -1) {
      QMessageBox::critical(this, "Error!",
                            QString("Covariate #%1 has an invalid efficiency "
                                    "value! Evaluation aborted")
                                .arg(tmpIndex));
      all.current()->setText(3, QString::null);
      return;
    }
    rawEff[tmpIndex] = tmpEff;
    all.current()->setText(3, getEffStr(tmpEff / baseEff));
    ++all;
  }
}

/* getEffStr() converts the input efficiency of double type to a QString */
QString G_Eff::getEffStr(double inputEff) {
  double foo = inputEff;
  // Reset the efficiency to be zero if it is less than or equal to 0.01
  if (foo <= 0.01) foo = 0;

  QString effStr = QString::number(foo).left(4);
  // 1.00 or 0.00 makes the interface look nicer
  if (effStr == "1")
    effStr = "1.00";
  else if (effStr == "0")
    effStr = "0.00";

  return effStr;
}

/* modView() will enable/disable covariates in covView and set the first
 * selectable item as the efficiency base */
void G_Eff::modView() {
  baseIndex = -1;
  for (int i = 0; i < (int)rawEff.size(); i++) rawEff[i] = -1.0;

  QObject::disconnect(covView, SIGNAL(selectionChanged(Q3ListViewItem *)), this,
                      SLOT(modEffBase(Q3ListViewItem *)));
  if (covView->selectedItem())
    covView->setSelected(covView->selectedItem(), false);

  Q3ListViewItemIterator all(covView);
  QString typeStr;
  while (all.current()) {
    typeStr = all.current()->text(1);
    all.current()->setText(3, QString::null);
    if (typeFlag == 0 && typeStr.length())
      all.current()->setEnabled(true);
    else if (typeFlag == 1 && typeStr == "I")
      all.current()->setEnabled(true);
    else if (typeFlag == 2 && (typeStr == "N" || typeStr == "K"))
      all.current()->setEnabled(true);
    else
      all.current()->setEnabled(false);
    ++all;
  }

  Q3ListViewItemIterator all2(covView, Q3ListViewItemIterator::Selectable);
  if (all2.current()) {
    baseIndex = all2.current()->text(2).toInt();
    covView->setSelected(all2.current(), true);
    modRawEff();
  }

  QObject::connect(covView, SIGNAL(selectionChanged(Q3ListViewItem *)), this,
                   SLOT(modEffBase(Q3ListViewItem *)));
}

/* getRawEff() returns a certain covariate's raw efficiency value */
double G_Eff::getRawEff(int inputIndex) {
  double neuralPower, boldPower;
  VB_Vector *tmpVar = covList[inputIndex];
  neuralPower = getSquareSum(*tmpVar);
  if (!neuralPower) {
    return -1.0;
  }

  boldPower = getBold(tmpVar);
  return boldPower / neuralPower;
}

/* modFilterVec() will modify filterVec based on the selected filter file. */
void G_Eff::modFilterVec() {
  int orderG = totalReps * TR / tmpResolve;
  int ratio = orderG / totalReps;

  // (Do not downsample | Downsample after convolution) & filter file available
  if (downFlag != 1 && !filterFile.isNull()) {
    filterVec = new VB_Vector((const char *)filterFile);
    /* dhu bug: need to make sure 2000 % tmpResolve == 0 here
       if not, pop out an error message */
    filterVec->sincInterpolation(2000 / tmpResolve);
    /* dhu bug: need to make sure orderG - filterVec->getLength() > 0,
       if  orderG - filterVec->getLength() == 0, tmpVec is not needed
       if orderG - filterVec->getLength() < 0, pop out an error message */
    VB_Vector tmpVec(orderG - filterVec->getLength());
    tmpVec.setAll(0);
    filterVec->concatenate(tmpVec);
  }
  // (Do not downsample | Downsample after convolution) & filter file not
  // available
  else if (downFlag != 1 && filterFile.isNull()) {
    filterVec = new VB_Vector(orderG);
    filterVec->setAll(0);
    (*filterVec)[0] = 1.0;
  }
  // Downsample before convolution & filter file available
  else if (downFlag == 1 && !filterFile.isNull()) {
    filterVec = new VB_Vector((const char *)filterFile);
    filterVec->sincInterpolation((2000 / tmpResolve) / ratio);
    VB_Vector tmpVec(totalReps - filterVec->getLength());
    tmpVec.setAll(0);
    filterVec->concatenate(tmpVec);
  }
  // Downsample before convolution & filter file not available
  else if (downFlag == 1 && filterFile.isNull()) {
    filterVec = new VB_Vector(totalReps);
    filterVec->setAll(0);
    (*filterVec)[0] = 1.0;
  }

  filterVec->meanCenter();
  filterVec->normMag();
}

/* getBold() reads an input VB_Vector and calculates the efficiency of
 * this covariate. A double value of the efficiency is returned. */
double G_Eff::getBold(VB_Vector *inputCov) {
  int orderG = totalReps * TR / tmpResolve;
  int ratio = orderG / totalReps;
  double boldPower = 0;

  VB_Vector *vec2, *vec3, filteredParam;
  switch (downFlag) {
    case 0:  // Do not downsample
      filteredParam = fftConv(inputCov, filterVec, true);
      boldPower = getSquareSum(filteredParam);
      break;
    case 1:  // Downsample before convolution
      vec2 = downSampling(inputCov, ratio);
      filteredParam = fftConv(vec2, filterVec, true);
      boldPower = getSquareSum(filteredParam) * (double)ratio;
      delete vec2;
      break;
    case 2:  // Downsample after convolution
      filteredParam = fftConv(inputCov, filterVec, true);
      vec3 = downSampling(&filteredParam, ratio);
      boldPower = getSquareSum(*vec3) * (double)ratio;
      delete vec3;
      break;
  }

  return boldPower;
}

/* Function that returns the input vector's sum of squares of each element. */
double G_Eff::getSquareSum(VB_Vector &inputVec) {
  double sum = 0;
  for (int i = 0; i < (int)inputVec.getLength(); i++)
    sum += inputVec[i] * inputVec[i];
  return sum;
}

/* cpItem() will recursively copy the input listViewItem, written for covariate
 * duplication orgDepth is input item's original depth */
void G_Eff::cpItem(Q3ListViewItem *inputItem) {
  Q3ListViewItem *newItem;
  if (inputItem->text(2).isEmpty()) {
    if (!inputItem->childCount()) return;
    if (inputItem->depth() == 0)
      newItem = new Q3ListViewItem(covView, getLastChild(covView),
                                   inputItem->text(0));
    else
      newItem = new Q3ListViewItem(findParent(inputItem),
                                   getLastChild(findParent(inputItem)),
                                   inputItem->text(0));
    newItem->setOpen(true);
    newItem->setEnabled(false);
    return;
  }

  if (inputItem->depth() == 0)
    newItem =
        new Q3ListViewItem(covView, getLastChild(covView), inputItem->text(0),
                           inputItem->text(1), inputItem->text(2));
  else
    newItem = new Q3ListViewItem(
        findParent(inputItem), getLastChild(findParent(inputItem)),
        inputItem->text(0), inputItem->text(1), inputItem->text(2));
  // By default, only enable type I covariates
  if (newItem->text(1) != "I") newItem->setEnabled(false);
}

/* Modified based on gdw's same function */
Q3ListViewItem *G_Eff::findParent(Q3ListViewItem *inputItem) {
  int lastDepth = covView->lastItem()->depth();
  int inputDepth = inputItem->depth();
  if (lastDepth < inputDepth) return covView->lastItem();

  Q3ListViewItem *newParent = covView->lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

/* Slot for "Delete" */
void G_Eff::delClicked() {
  bool ok;
  double cutoffVal = cutoffEdit->text().toDouble(&ok);
  if (!ok) {
    QMessageBox::critical(this, "Error", "Invalid cutoff value!");
    return;
  }

  QString delStr = "<br>";
  std::vector<int> delList;
  double tmpEff;
  Q3ListViewItemIterator all(covView, Q3ListViewItemIterator::Selectable);
  while (all.current()) {
    if (all.current()->text(3).length()) {
      tmpEff = all.current()->text(3).toDouble();
      if (tmpEff >= 0 && tmpEff < cutoffVal) {
        delList.push_back(all.current()->text(2).toInt());
        delStr += QString("-- Covariate #%1<br>").arg(all.current()->text(2));
      }
    }
    ++all;
  }
  // Sort delList to make life easier when it is passed back to gdw interface
  sort(delList.begin(), delList.end());

  if (!delList.size()) {
    QMessageBox::information(this, "Info", "No covariates will be deleted.");
    this->close();
    emit(cancelSignal(true));
    return;
  }

  switch (QMessageBox::warning(
      this, "Warning!",
      "Are you sure you want to delete the following covariate(s)?" + delStr,
      "Yes", "No", 0, 0, 1)) {
    case 0:
      emit delSignal(delList);
      this->close();
      break;
    case 1:
      break;
  }
}

/* closeEvent() is overloaded so that when user clicks "File->close" or "x" on
 * the corner, he/she won't get a disbaled gdw interface */
void G_Eff::closeEvent(QCloseEvent *ce) {
  ce->accept();
  emit(cancelSignal(true));
}

/* Slot for "Cancel" */
void G_Eff::cancelClicked() { this->close(); }

/* Compare two VBMatrix headers to determine whether they match each other.
 * This function is called by both glm and gdw. */
bool cmpG2preG(gHeaderInfo myGInfo, gHeaderInfo pregInfo) {
  if (myGInfo.TR != pregInfo.TR) {
    printf("Different TR in G and preG file. preG file is ignored.\n");
    return false;
  }

  if (myGInfo.colNum != pregInfo.colNum) {
    printf(
        "Different number of columns in G and preG file. preG file is "
        "ignored.\n");
    return false;
  }

  if (myGInfo.sampling != pregInfo.sampling) {
    printf(
        "Different sampling rate in G and preG file. preG file is ignored.\n");
    return false;
  }

  if (myGInfo.TR && myGInfo.sampling &&
      myGInfo.rowNum * myGInfo.TR / myGInfo.sampling != pregInfo.rowNum) {
    printf(
        "The number of rows in preG file is NOT the upsampled number based on "
        "G file.\n");
    printf("preG file is ignored.\n");
    return false;
  }

  if (myGInfo.condfxn != pregInfo.condfxn) {
    printf(
        "Different condition function filenames in G and preG file. preG file "
        "is ignored.\n");
    return false;
  }

  if (myGInfo.condfxn.length() &&
      !cmpArray(myGInfo.condKey, pregInfo.condKey)) {
    printf(
        "Different Condition lines in G and preG file. preG file is "
        "ignored.\n");
    return false;
  }

  if (!cmpArray(myGInfo.nameList, pregInfo.nameList)) {
    printf(
        "Different covariate names in G and preG file. preG file is "
        "ignored.\n");
    return false;
  }

  if (!cmpArray(myGInfo.typeList, pregInfo.typeList)) {
    printf(
        "Different covariate types in G and preG file. preG file is "
        "ignored.\n");
    return false;
  }

  return true;
}

/* This function compares two tokenlist objects.
 * Returns true their elements are same, otherwise false */
bool cmpArray(tokenlist listA, tokenlist listB) {
  if (!listA.size() || !listB.size() || listA.size() != listB.size())
    return false;

  for (size_t i = 0; i < listA.size(); i++) {
    if (listA[i] != listB[i]) return false;
  }
  return true;
}

/* This function compares two tokenlist objects.
 * Returns true their elements are same, otherwise false */
bool cmpArray(deque<string> listA, deque<string> listB) {
  if (!listA.size() || !listB.size() || listA.size() != listB.size())
    return false;

  for (unsigned i = 0; i < listA.size(); i++) {
    if (listA[i] != listB[i]) {
      return false;
    }
  }
  return true;
}

/* This function returns the filters directory's absolute path by calling
 * vbp.rootdir */
QString getFilterPath() {
  QString vbStr(vbp.rootdir.c_str());
  if (!vbStr.endsWith("/")) vbStr += "/";
  QString bar(vbStr + "elements/filters/");
  return bar;
}

/* This function will copy an input covatiate tree view to varListView on gdw
 * interface. It is called when "edit" button is clicked on glm tab3 interface
 */
void cpView(Q3ListView *inputView, Q3ListView *outputView) {
  if (outputView->childCount()) outputView->clear();

  if (!inputView->childCount()) return;

  Q3ListViewItemIterator it(inputView);
  while (it.current()) {
    cpItem(it.current(), outputView);
    ++it;
  }
}

/* This function will recursively copy the input listViewItem, written for
 * covariate duplication orgDepth is input item's original depth */
void cpItem(Q3ListViewItem *inputItem, Q3ListView *outputView) {
  Q3ListViewItem *newItem;
  if (inputItem->text(2).isEmpty()) {
    if (inputItem->depth() == 0)
      newItem = new Q3ListViewItem(outputView, getLastChild(outputView),
                                   inputItem->text(0));
    else
      newItem =
          new Q3ListViewItem(findOrgParent(inputItem, outputView),
                             getLastChild(findOrgParent(inputItem, outputView)),
                             inputItem->text(0));
    newItem->setOpen(true);
    return;
  }

  if (inputItem->depth() == 0)
    newItem = new Q3ListViewItem(outputView, getLastChild(outputView),
                                 inputItem->text(0), inputItem->text(1),
                                 inputItem->text(2), inputItem->text(3));
  else
    newItem = new Q3ListViewItem(
        findOrgParent(inputItem, outputView),
        getLastChild(findOrgParent(inputItem, outputView)), inputItem->text(0),
        inputItem->text(1), inputItem->text(2), inputItem->text(3));
}

/* Modified based on gdw's same function */
Q3ListViewItem *findOrgParent(Q3ListViewItem *inputItem,
                              Q3ListView *outputView) {
  int lastDepth = outputView->lastItem()->depth();
  int inputDepth = inputItem->depth();
  if (lastDepth < inputDepth) return outputView->lastItem();

  Q3ListViewItem *newParent = outputView->lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

/* findGrp() searches the input parent's direct child and returns the
 * QListViewItem group
 * whose name is the input QString. Returns 0 if not found. */
Q3ListViewItem *findGrp(Q3ListViewItem *parent, QString grpName) {
  Q3ListViewItem *child = parent->firstChild();
  while (child) {
    if (child->text(2).isEmpty() && child->text(0) == grpName) return child;
    child = child->nextSibling();
  }
  return 0;
}

/* This getLastChild() will return a QListView's last direct child
 * QListViewItem. Return 0 if the input QListViewItem does not have any child.
 */
Q3ListViewItem *getLastChild(Q3ListView *parentView) {
  if (!parentView->childCount()) return 0;

  Q3ListViewItem *myChild = parentView->firstChild();
  for (int i = 0; i < parentView->childCount() - 1; i++)
    myChild = myChild->nextSibling();

  return myChild;
}

/* This getLastChild() will return a QListViewItem's last direct child
 * QListViewItem. Return 0 if the input QListViewItem does not have any child.
 */
Q3ListViewItem *getLastChild(Q3ListViewItem *parentItem) {
  if (!parentItem->childCount()) return 0;

  Q3ListViewItem *myChild = parentItem->firstChild();
  for (int i = 0; i < parentItem->childCount() - 1; i++)
    myChild = myChild->nextSibling();

  return myChild;
}

/* This function will write .G and .preG file */
void writeG(string stemname, int TR, int totalReps, int tmpResolve,
            vector<VB_Vector *> covList, Q3ListView *covView, QString &condRef,
            tokenlist condKey, bool meanAll) {
  int colNum = covList.size();
  int pregRowNum = totalReps * TR / tmpResolve;
  int gRowNum = totalReps;
  // Convert the QString to const string for VBMatrix()
  const string fn2 = stemname + ".G";
  const string fn3 = stemname + ".preG";
  VBMatrix *myPreG = new VBMatrix(pregRowNum, colNum);
  VBMatrix *myG = new VBMatrix(gRowNum, colNum);

  // Add date when this file is created
  string myTime, myDate, timeHeader;
  // maketimedate() is defined in libvoxbo/vbutil.cpp, recommended by Dan
  maketimedate(myTime, myDate);
  timeHeader = (string) "DateCreated:\t" + myTime + (string) "_" + myDate +
               (string) "\n";
  myPreG->AddHeader(timeHeader);
  myG->AddHeader(timeHeader);

  QString tmpQString = "TR(ms):\t\t" + QString::number(TR);
  const string tmpString1((const char *)tmpQString);
  myPreG->AddHeader(tmpString1);
  myG->AddHeader(tmpString1);
  tmpQString = "Sampling(ms):\t" + QString::number(tmpResolve);
  const string tmpString2((const char *)tmpQString);
  myPreG->AddHeader(tmpString2);
  myG->AddHeader(tmpString2);

  if (condRef.length()) {
    QString tmpQStr = "ConditionFile:\t" + condRef;
    const string tmpStr(tmpQStr.ascii());
    myPreG->AddHeader(tmpStr);
    myG->AddHeader(tmpStr);
  }
  // Convert "keyText" into "Condition:<TAB>index<TAB>keyText" in the condition
  // key list box
  for (size_t i = 0; i < condKey.size(); i++) {
    // QString keyString = keyList->text(i);
    QString keyString = "Condition:\t" + QString::number(i) + "\t" + condKey(i);
    // const string keyString2((const char *)keyString);
    myPreG->AddHeader(keyString.ascii());
    myG->AddHeader(keyString.ascii());
  }

  // Add a blank line here
  myPreG->AddHeader((string) "\n");
  myG->AddHeader((string) "\n");

  // Convert "I: varText" into "Parameter:<TAB>index<TAB>type<TAB>varText"
  QStringList nameList, typeList;
  getCovInfo(nameList, typeList, covView);
  QString myName, myType, varHeader;
  for (unsigned i = 0; i < covList.size(); i++) {
    myName = nameList[i];
    myType = typeList[i];
    // Create header line for each covariate
    if (myType == "I")
      varHeader = "Parameter:\t" + QString::number(i) + "\tInterest\t" + myName;
    else if (myType == "N")
      varHeader =
          "Parameter:\t" + QString::number(i) + "\tNoInterest\t" + myName;
    else if (myType == "K")
      varHeader =
          "Parameter:\t" + QString::number(i) + "\tKeepNoInterest\t" + myName;
    else if (myType == "D")
      varHeader =
          "Parameter:\t" + QString::number(i) + "\tDependent\t" + myName;
    else
      varHeader =
          "Parameter:\t" + QString::number(i) + "\tUndefined\t" + myName;

    const string headerString(varHeader.ascii());
    myPreG->AddHeader(headerString);
    myG->AddHeader(headerString);
  }

  // Write *.preG file now
  for (int i = 0; i < colNum; i++) {
    // If the covariate's variance is less than or equal to 1e-15,
    // assume it is intercept and don't mean center it.
    if (meanAll && covList[i]->getVariance() > 1e-15) covList[i]->meanCenter();
    myPreG->SetColumn((const int)i, *covList[i]);
  }
  myPreG->WriteFile(fn3);
  myPreG->clear();
  delete myPreG;
  // Write *.G file now
  int downRatio = TR / tmpResolve;
  VB_Vector tmpVec;
  for (int i = 0; i < colNum; i++) {
    tmpVec = *downSampling(covList[i], downRatio);
    if (meanAll && tmpVec.getVariance() > 1e-15) tmpVec.meanCenter();
    myG->SetColumn((const int)i, tmpVec);
  }
  myG->WriteFile(fn2);
  myG->clear();
  delete myG;
}

/* getCovInfo() collects each real covariate's name and type, put them into
 * two QStringList arguments respectively: nameList and typeList */
void getCovInfo(QStringList &nameList, QStringList &typeList,
                Q3ListView *covView) {
  Q3ListViewItemIterator it(covView);
  QString nameStr;
  while (it.current()) {
    if (it.current()->text(2).length()) {
      nameStr = getCovName(it.current());
      nameList.push_back(nameStr);
      typeList.push_back(it.current()->text(1));
    }
    it++;
  }
}

/* getCovName() returns the input covariate's full name, starting from depth 1
 */
QString getCovName(Q3ListViewItem *inputItem) {
  int depth = inputItem->depth();
  QString fullName = inputItem->text(0);
  Q3ListViewItem *parent = inputItem->parent();
  for (int i = 0; i < depth; i++) {
    fullName = parent->text(0) + "->" + fullName;
    parent = parent->parent();
  }
  return fullName;
}

/* getLowFreq() will evaluate the input VB_Vector instance and return the
 * number of frequencies which are lower than 1% of the total power spectrum.
 * Note that the returned number isn't the largest index of the cutoff
 * frequency.
 *
 * In Geoff's IDL code, ps2percent = (inputVector->getVectorSum()) * 0.01 / 2;
 * which is one half of my calculation. After talking with Geoff, do not divide
 * by 2. */
int getLowFreq(VB_Vector *inputVector) {
  double ps1percent = (inputVector->getVectorSum()) * 0.01;
  double subPower = inputVector->getElement(0);
  int i = 0;
  while (subPower < ps1percent) {
    i++;
    subPower += inputVector->getElement(i);
  }
  return i;
}

/* This method is to calculate a VB_Vector's fft and return another vector up to
 * Nyquist frequency */
VB_Vector *fftNyquist(VB_Vector *inputVector) {
  int totalLength = inputVector->getLength();
  VB_Vector *fullFFT = new VB_Vector(totalLength);
  inputVector->getPS(fullFFT);

  int newLength = totalLength / 2 + 1;
  VB_Vector *halfFFT = new VB_Vector(newLength);

  for (int i = 0; i < newLength; i++) {
    halfFFT->setElement(i, fullFFT->getElement(i));
  }

  delete fullFFT;
  return halfFFT;
}
