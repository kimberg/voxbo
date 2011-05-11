
// vecview.cpp
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

using namespace std;

#include <QPixmap>
#include <QLabel>
#include <QKeyEvent>
#include <iostream>
#include <qapplication.h>
#include <qcheckbox.h>
#include <q3hbox.h>
#include <Q3HButtonGroup>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstatusbar.h>
#include "vecview.h"
#include "vecview.hlp.h"

// FIXME arg handling below is messy
/* Here comes the main function for vecview binary file 
 * If there is only one argument, check the argument to
 * see if it is a valid ref or VBMatrix file. If it is ref,
 * show it in mode 1; if it's VBMatrix, show it in mode 2;
 * otherwise report an error message.
 *
 * If there are two argument, make sure the first is "-m",
 * then check the 2nd argument to make sure it's a valid 
 * movement parameter ref file. If yes, show it in mode 3.
 * If not, report an error message.

 * For any other auguments, print out a usage line. */ 
int main( int argc, char **argv )
{
  QApplication a( argc, argv );

  VecView *w;
  char *fileName;
  int fileMode;
  
  if (argc == 2) {
    if ((string)argv[1] == "-h") {
      vecview_help();
      exit(0);
    }
    else if ((string)argv[1]=="-v") {
      vecview_version();
      exit(0);
    }
  }

  if (argc == 2) {
    fileName = argv[1];
    fileMode = checkInFile(fileName, false);

    if (fileMode < 0) {
      printf("%s: not a valid ref or VBMatrix file.\n", fileName);
      return 0;
    }

    w = new VecView(fileName, fileMode);
    w->setCaption("vecview: " + QString(fileName));
  }
  
  // Is it movement parameter files?
  else if (argc >= 3 && dancmp(argv[1], "-m")) {
    VB_Vector *totalVec = new VB_Vector();
    QString nameCombo = "";
    for (int i = 2; i < argc; i++) {
      fileName = argv[i];
      fileMode = checkInFile(fileName, true);
      
      if (fileMode == -2) {
        printf("%s: invalid movement parameter file.\n", fileName);
        return 0;
      }
      else if (fileMode == -3) {
        printf("%s: invalid ref file.\n", fileName);
        return 0;
      }
      // Combine name together to show on title bar
      if (i != argc - 1)
        nameCombo += QString(fileName) + " + ";
      else
        nameCombo += QString(fileName);
      
      VB_Vector tmpVec(fileName);
      totalVec->concatenate(tmpVec);
    }

    w = new VecView(totalVec, true);
    w->setCaption("vecview: " + nameCombo);
  }

  // "-s" accepts multiple ref files and draw them seperately
  else if (argc >= 3 && dancmp(argv[1], "-a")) {
    VB_Vector *totalVec = new VB_Vector();
    QString nameCombo = "";
    for (int i = 2; i < argc; i++) {
      fileName = argv[i];
      fileMode = checkInFile(fileName, false);
      if (fileMode != 1) {
        printf("%s: invalid ref file.\n", fileName);
        return 0;
      }
      // Combine name together to show on title bar
      if (i != argc - 1)
        nameCombo += QString(fileName) + " + ";
      else
        nameCombo += QString(fileName);
      
      VB_Vector tmpVec(fileName);
      totalVec->concatenate(tmpVec);
    }
    w = new VecView(totalVec, false);
    w->setCaption("vecview: " + nameCombo);
  }

  // Otherwise concatenate all arguments as one big ref file
  else if (argc >= 3) {
    tokenlist refList;
    for (int i = 1; i < argc; i++) {
      fileName = argv[i];
      fileMode = checkInFile(fileName, false);
      if (fileMode != 1) {
        printf("%s: invalid ref file.\n", fileName);
        return 0;
      }
      refList.Add(fileName);
    }
    w = new VecView(refList);
  }

  // Print help message
  else {
    vecview_help();
    return 0;
  }

  a.setMainWidget(w);
  w->show();
  return a.exec();
}

/* Constructor which accepts a single filename and inputMode */
VecView::VecView( const char * inputFile, int inputMode, QWidget *parent, const char *name )
  : Q3MainWindow( parent, name )
{
  init();

  string fileString = inputFile;
  if (inputMode == 1) {
    VB_Vector *myVec = new VB_Vector(fileString);
    vecShow(myVec);
  }
  else {
    initColor();
    setVecList(fileString);
    matShow();
  }

  setCentralWidget(mainBox);
}

/* Another constructor which accepts vb_vector and movement parameter flag */
VecView::VecView( VB_Vector *inputVec, bool mvpmFlag, QWidget *parent, const char *name )
  : Q3MainWindow( parent, name )
{
  init();

  if (mvpmFlag)
    mvpmShow(inputVec);
  else 
    vecShow(inputVec);

  setCentralWidget(mainBox);
}

/* This constructor shows multiple ref files */
VecView::VecView( tokenlist refList, QWidget *parent, const char *name )
  : Q3MainWindow( parent, name )
{
  refFiles = refList;
  init();
  initColor();
  plotStat = 1;

  for (size_t i = 0; i < refList.size(); i++) {
    VB_Vector myVec(refList[i]);
    vecList.push_back(myVec);
    psList.push_back(fftnyquist(myVec));
  }

  mainScreen->addVector(vecList[0]);
  setCommon();
  setVecBox();
  setCentralWidget(mainBox);

  int vecLen = vecList[0].getLength();
  statusBar()->message(QString("%1 length: %2").arg(refFiles(0)).arg(vecLen));
}

/* Simple destructor */
VecView::~VecView()
{
  mainScreen->clear();
  if (colorList.size())
    colorList.clear();
}

/* Initialize the interface */
void VecView::init()
{
  plotStat = 0;
  psChecked = false;
  mainBox = new Q3VBox(this);
  mainScreen = new PlotScreen(mainBox);

  mainScreen->setMinimumSize(600, 300);
  mainScreen->setUpdatesEnabled(true);
  mainBox->setStretchFactor(mainScreen, 20);
  ctrlPressed = false;
}

/* Set predetermined 9 colors */
void VecView::initColor()
{
  if (colorList.size())
    colorList.clear();
  colorList.Add("green");
  colorList.Add("red");
  colorList.Add("blue");
  colorList.Add("cyan");
  colorList.Add("magenta");
  colorList.Add("yellow");
  colorList.Add("gray");
  colorList.Add("lightGray");
  colorList.Add("white");
}

/* vecShow() shows a simple ref file (mode 1) */
void VecView::vecShow(VB_Vector *inputVec)
{
  plotStat = 0;
  vecList.push_back(inputVec);
  VB_Vector psVec = fftnyquist(*inputVec);
  psList.push_back(psVec);

  mainScreen->addVector(inputVec);
  setCommon();
  statusBar()->message(QString("Ref file length: %1").arg(inputVec->getLength()));
}

/* matShow() shows a VBMatrix file (mode 2). 
 * It adds a slider bar at the bottom so that user can show different columns. */
void VecView::matShow()
{
  plotStat = 2;
  mainScreen->addVector(vecList[0]);
  setCommon();
  setVecBox();

  int vecLen = vecList[0].getLength();
  int vecNum = vecList.size();
  if (vecNum > 1)
    statusBar()->message(QString("Column #1 (G matrix dimension: %1 rows, %2 columns)").arg(vecLen).arg(vecNum));
  else
    statusBar()->message(QString("Column #1 (G matrix dimension: %1 rows, 1 column)").arg(vecLen));
}

/* This function adds the vector selection box on the interface */
void VecView::setVecBox()
{
  Q3HBox *vecBox = new Q3HBox(mainBox);
  vecBox->setMargin(10);
  vecBox->setSpacing(20);

  QLabel *vecTxt = new QLabel(vecBox);
  if (plotStat == 2)
    vecTxt->setText("<b>Browse Columns</b>");
  else
    vecTxt->setText("<b>Browse Ref Files</b>");

  int vecNum = vecList.size();
  vecSlider = new QSlider(0, vecNum - 1, 1, 0, Qt::Horizontal, vecBox);
  vecSlider->setTickmarks( QSlider::Below );
  QObject::connect(vecSlider, SIGNAL(valueChanged(int)), this, SLOT(changeVec(int)));

  showAll = new QCheckBox("Show All", vecBox);
  QObject::connect(showAll, SIGNAL(toggled(bool)), this, SLOT(toggleShow(bool)));
  autoScale = new QCheckBox("Y Auto Scale", vecBox);
  autoScale->setChecked(true);
  QObject::connect(autoScale, SIGNAL(toggled(bool)), this, SLOT(toggleScale(bool)));
}

/* This function replies "Show All" checkbox toggle action. */
void VecView::toggleShow(bool showStat)
{
  mainScreen->clear();
  if (showStat) {
    QString statusTxt = "";
    for (int i = 0; i < (int) vecList.size(); i++) {
      int j = i % colorList.size();
      if (!psChecked)
	mainScreen->addVector(vecList[i], QColor(colorList(j)));
      else
	mainScreen->addVector(psList[i], QColor(colorList(j)));

      if (plotStat == 2 && i != (int) vecList.size() - 1)
	statusTxt += "Column #" + QString::number(i + 1) + ": " + QString(colorList(j)) + "; ";
      else if (plotStat == 2) 
	statusTxt += "Column #" + QString::number(i + 1) + ": " + QString(colorList(j));
      else if (i != (int) vecList.size() - 1)
	statusTxt += QString(refFiles(i)) + ": " + QString(colorList(j)) + "; ";
      else
	statusTxt += QString(refFiles(i)) + ": " + QString(colorList(j));
    }
    
    statusBar()->message(statusTxt);      
    vecSlider->setDisabled(true);
  }
  else {
    vecSlider->setEnabled(true);
    int vecIndex = vecSlider->value();
    int vecLen = vecList[vecIndex].getLength();
    int vecNum = vecList.size();
    if (plotStat == 2 && vecNum > 1)
      statusBar()->message(QString("Column #%1 (G matrix dimension: %2 rows, %3 columns)").
			   arg(vecIndex + 1).arg(vecLen).arg(vecNum));
    else if (plotStat == 2)
      statusBar()->message(QString("Column #%1 (G matrix dimension: %2 rows, 1 column)").
			   arg(vecIndex + 1).arg(vecLen));
    else
      statusBar()->message(QString("%1 length: %2").arg(refFiles(vecIndex)).arg(vecLen));

    if (!psChecked)
      mainScreen->addVector(vecList[vecIndex]);
    else
      mainScreen->addVector(psList[vecIndex]);
  }
  mainScreen->update();
}

/* This function replies "Y Auto Scale" checkbox toggle action. */
void VecView::toggleScale(bool scaleStat)
{
  if (scaleStat)
    mainScreen->enableFixedY(false);
  else
    mainScreen->enableFixedY(true);
  mainScreen->update();
}

/* mvpmShow() shows a movement parameter ref file (mode 3).
 * Three radiobuttons at the bottom are available for user to 
 * choose different groups: X/Y/Z, Pitch/Roll/Yaw, Iteration. */
void VecView::mvpmShow(VB_Vector *inputVec)
{
  plotStat = 3;
  int unitLength = inputVec->getLength() / 7;
  for (int mpIndex = 0; mpIndex < 7; mpIndex++) {
    VB_Vector tmpVec(unitLength);
    for (int i = 0; i < unitLength; i++)
      tmpVec.setElement(i, inputVec->getElement(i * 7 + mpIndex));
    vecList.push_back(tmpVec);
    psList.push_back(fftnyquist(tmpVec));
  }

  mainScreen->addVector(vecList[0], "green");
  mainScreen->addVector(vecList[1], "red");
  mainScreen->addVector(vecList[2], "yellow");

  setCommon();
  Q3HButtonGroup *mpGroup = new Q3HButtonGroup(mainBox); 
  mpGroup->setLineWidth(0);

  QRadioButton *XYZ = new QRadioButton("X (green), Y (red), Z (yellow)", mpGroup);
  (void) new QRadioButton("Pitch (green), Roll (red), Yaw (yellow)", mpGroup);
  (void) new QRadioButton("Iterations", mpGroup);
  XYZ->setChecked(true);
  QObject::connect(mpGroup, SIGNAL(clicked(int)), this, SLOT(changeMP(int)));

  statusBar()->message(QString("Movement parameter component length: %1").arg(unitLength));
}

/* setCommon() sets the common widgets in three mode:
 * magnification slider bar with text, position slider bar 
 * with text and a center pushbutton. */
void VecView::setCommon()
{
  mainScreen->setXCaption("X Axis");
  mainScreen->setYCaption("Y Axis");
  mainScreen->update();

  /* Add "magnification" and "position" slide bars and a "center" button in the middle */
  Q3VBox *mag_pos = new Q3VBox(mainBox);
  mag_pos->setSpacing(1);
  Q3HBox *widgetBox = new Q3HBox(mag_pos);
  widgetBox->setSpacing(20);
  widgetBox->setMargin(2);

  QLabel *magLab = new QLabel("<b>Magnification</b>", widgetBox);
  magLab->setAlignment(Qt::AlignHCenter);

  /***********************************************************************
   * Horizontal magnification slider bar, with the tickmarks on the above 
   * Maximum: 10, Minimum: 1, step: 1, default position: 1
   **********************************************************************/
  magSlider = new QSlider(1, 10, 1, 1, Qt::Horizontal, widgetBox);
  magSlider->setTickmarks( QSlider::Below );
  QObject::connect(magSlider, SIGNAL(valueChanged(int)), mainScreen, SLOT(setXMag(int)));
  QObject::connect(mainScreen, SIGNAL(xMagChanged(int)), magSlider, SLOT(setValue(int)));

  QPushButton *graphCenter = new QPushButton("Center", widgetBox); // "center" pushbutton
  QObject::connect(graphCenter, SIGNAL(clicked()), mainScreen, SLOT(centerX()));

  QCheckBox *psOption = new QCheckBox("Show Power Spectrum", widgetBox);
  QObject::connect(psOption, SIGNAL(toggled(bool)), this, SLOT(togglePS(bool)));

  mainScreen->setFocus();
}

/* This function reads the input matrix file and puts each column into vecList array */
void VecView::setVecList(string filename)
{
  VBMatrix *myMat = new VBMatrix(filename);
  for (size_t i = 0; i < myMat->n; i++) {
    VB_Vector tmpVec = myMat->GetColumn(i);
    vecList.push_back(tmpVec);
    psList.push_back(fftnyquist(tmpVec));
  }

}

/* togglePS() determines whether vector is plotted in time or frequency domain */
void VecView::togglePS(bool psStat)
{
  psChecked = psStat;
  mainScreen->clear();
  // Plot single vector's power spectrum
  if (psChecked && plotStat == 0)
    mainScreen->addVector(psList[0]);
  // Plot single vector in regular domain
  else if (plotStat == 0)
    mainScreen->addVector(vecList[0]);
  // Plot multiple vectors (input is either G matrix or more than one ref files)
  else if (plotStat == 1 || plotStat == 2) {
    /* Automatically set auto scale, because very likely the 
     * scale in time and frequency domain will be very different */
    autoScale->setChecked(true);
    mainScreen->enableFixedY(false);
    if (showAll->isChecked()) {
      for (unsigned i = 0; i < psList.size(); i++) {
	int j = i % colorList.size();
	if (psChecked)
	  mainScreen->addVector(psList[i], QColor(colorList(j)));
	else
	  mainScreen->addVector(vecList[i], QColor(colorList(j)));
      }	
    }
    else {
      int vecIndex = vecSlider->value();
      if (psChecked)
	mainScreen->addVector(psList[vecIndex]);
      else
	mainScreen->addVector(vecList[vecIndex]);
    }
  }
  // plot movement parameter file
  else 
    plotMP();

  mainScreen->update();
}

/* changeVec() takes care of the signal from vector browser slider bar. */
void VecView::changeVec(int i)
{
  int vecLen = vecList[i].getLength();
  int vecNum = vecList.size();
  if (plotStat == 2 && vecNum > 1)
    statusBar()->message(QString("Column #%1 (G matrix dimension: %2 rows, %3 columns)").
			 arg(i + 1).arg(vecLen).arg(vecNum));
  else if (plotStat == 2)
    statusBar()->message(QString("Column #%1 (G matrix dimension: %2 rows, 1 column)").
			 arg(i + 1).arg(vecLen));
  else
    statusBar()->message(QString("%1 length: %2").arg(refFiles(i)).arg(vecLen));
  
  mainScreen->clear();
  if (!psChecked)
    mainScreen->addVector(vecList[i]);
  else
    mainScreen->addVector(psList[i]);

  mainScreen->update();
}

/* changeMP() takes care of the signal from the movement parameter 
 * radiobutton group. */ 
void VecView::changeMP(int mpGroup)
{
  mainScreen->clear();
  plotStat = 3 + mpGroup;
  plotMP();
  mainScreen->update();
}

/* Plot movement parameter vectors */
void VecView::plotMP()
{
  if (plotStat == 3 && psChecked) {
    mainScreen->addVector(psList[0], "green");
    mainScreen->addVector(psList[1], "red");
    mainScreen->addVector(psList[2], "yellow");
  }
  else if (plotStat == 3) {
    mainScreen->addVector(vecList[0], "green");
    mainScreen->addVector(vecList[1], "red");
    mainScreen->addVector(vecList[2], "yellow");
  }
  else if (plotStat == 4 && psChecked) {
    mainScreen->addVector(psList[3], "green");
    mainScreen->addVector(psList[4], "red");
    mainScreen->addVector(psList[5], "yellow");
  }
  else if (plotStat == 4) {
    mainScreen->addVector(vecList[3], "green");
    mainScreen->addVector(vecList[4], "red");
    mainScreen->addVector(vecList[5], "yellow");
  }
  else if (psChecked)
    mainScreen->addVector(psList[6]);
  else
    mainScreen->addVector(vecList[6]);
}

/* Overwrited function for ctrl+s implementation */
void VecView::keyPressEvent( QKeyEvent *e )
{
  if (e->key() == Qt::Key_Control)
    ctrlPressed = true;
  else if (ctrlPressed && e->key() == Qt::Key_S) {
    QString s = Q3FileDialog::getSaveFileName(".", "All (*.*)", this, "save image file",
					   "Choose a filename for your snapshot");
    if (s == QString::null) 
      return;
    QPixmap::grabWidget(mainScreen).save(s.latin1(),"PNG");
  }
}

/* Overwrited function for ctrl+s implementation */
void VecView::keyReleaseEvent ( QKeyEvent * e )
{
  if (e->key() == Qt::Key_Control)
    ctrlPressed = false;
}

/* checkInFile() checks the input file to make sure it is a valid 
 * ref, VBMatrix or movement parameter type. 
 * Note the various return values for different check result. */ 
int checkInFile(char * fileName, bool mvpmFlag)
{
  VB_Vector testVec;
  VBMatrix testMat;

  if (!mvpmFlag) {
    if (testVec.ReadFile((string)fileName) == 0)
      return 1;
    else if (testMat.ReadHeader((string)fileName) == 0 && testMat.m && testMat.n)
      return 2;
    else 
      return -1;
  }

  if (testVec.ReadFile((string)fileName) == 0) {
    int vecSize = testVec.getLength();
    if (vecSize % 7 == 0) 
      return 3;
    else 
      return -2;
  }
  
  return -3;
}

/* Print out help message */
void vecview_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void vecview_version()
{
  printf("VoxBo vecview (v%s)\n",vbversion.c_str());
}
