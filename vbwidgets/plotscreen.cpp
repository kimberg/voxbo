
// plotscreen.cpp
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

#include "plotscreen.h"
#include <math.h>
#include <q3simplerichtext.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <algorithm>
#include <iostream>

/* Basic constructor takes no arguments */
PlotScreen::PlotScreen(QWidget* parent) : QScrollArea(parent) { init(); }

/* Destructor */
PlotScreen::~PlotScreen() { clear(); }

/* This method is to initialize variables in PlotScreen class. */
void PlotScreen::init() {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  pw = new PlotWidget;
  setWidget(pw);

  int hBarHeight = horizontalScrollBar()->height();
  QWidget::setMinimumSize(pw->width() + 2 * frameWidth(),
                          pw->height() + 2 * frameWidth() + hBarHeight);
  setPaletteBackgroundColor(pw->bkgdColor);
  setFocusPolicy(Qt::ClickFocus);  // Interface has to be clicked before focus

  connect(pw, SIGNAL(xMagChanged(int)), this, SLOT(passMagSignal(int)));
  connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(setVisibleRange(int)));
}

/* Overloaded resizeEvent() */
void PlotScreen::resizeEvent(QResizeEvent* e) {
  QWidget::resizeEvent(e);
  int pw_width = pw->xMagnification * width();
  int pw_height = height() - 2 * frameWidth() - horizontalScrollBar()->height();
  pw->resize(pw_width, pw_height);
}

/* setMinimumSize() is an overloaded function.
 * It also changes plotWidth, plotHeight, etc */
void PlotScreen::setMinimumSize(unsigned inputWidth, unsigned inputHeight) {
  if (inputWidth < 100) {
    QMessageBox::critical(0, "Error", "Minimum PlotScreen window width is 100");
    return;
  }
  if (inputHeight < 100) {
    QMessageBox::critical(0, "Error",
                          "Minimum PlotScreen window height is 100");
    return;
  }

  // ignores the value if it is in zoom mode
  if (pw->xMagnification != 1) return;

  int newWidth = inputWidth - 2 * frameWidth();
  int newHeight =
      inputHeight - 2 * frameWidth() - horizontalScrollBar()->height();
  pw->resize(newWidth, newHeight);
  QWidget::setMinimumSize(inputWidth, inputHeight);
}

/* This method is to set the window's size, first number  width and second
 * height */
void PlotScreen::setWindowSize(unsigned inputWidth, unsigned inputHeight) {
  if (inputWidth < 100) {
    QMessageBox::critical(0, "Error", "Minimum PlotScreen window width is 100");
    return;
  }
  if (inputHeight < 100) {
    QMessageBox::critical(0, "Error",
                          "Minimum PlotScreen window height is 100");
    return;
  }

  // ignores the value if it is in zoom mode
  if (pw->xMagnification != 1) return;

  int newWidth = inputWidth - 2 * frameWidth();
  int newHeight =
      inputHeight - 2 * frameWidth() - horizontalScrollBar()->height();
  pw->resize(newWidth, newHeight);

  QWidget::setFixedSize(inputWidth, inputHeight);
}

/* This method is to set the window's width */
void PlotScreen::setWindowWidth(unsigned inputWidth) {
  if (inputWidth < 100) {
    QMessageBox::critical(0, "Error", "Minimum PlotScreen window width is 100");
    return;
  }

  int newWidth = inputWidth - 2 * frameWidth();
  pw->setFixedWidth(newWidth);
  QWidget::setFixedWidth(inputWidth);
}

/* This method is to set the window's height */
void PlotScreen::setWindowHeight(unsigned inputHeight) {
  if (inputHeight < 100) {
    QMessageBox::critical(0, "Error",
                          "Minimum PlotScreen window height is 100");
    return;
  }

  int newHeight =
      inputHeight - 2 * frameWidth() - horizontalScrollBar()->height();
  pw->setFixedHeight(newHeight);
  QWidget::setFixedHeight(inputHeight);
}

/* This method is to set the frame's width */
void PlotScreen::setFrameWidth(unsigned inputWidth) {
  pw->frameWidth = inputWidth;
}

/* This method sets the curve line's width in plot window */
void PlotScreen::setCurveLineWidth(unsigned inputWidth) {
  pw->curveLineWidth = inputWidth;
}

/* This method is to set the plot's size, first number width and second height
 */
void PlotScreen::setPlotSize(unsigned inputWidth, unsigned inputHeight) {
  pw->setPlotSize(inputWidth, inputHeight);
}

/* This method is to set the plot's width */
void PlotScreen::setPlotWidth(unsigned inputWidth) {
  pw->setPlotWidth(inputWidth);
}

/* This method is to set the plot's height */
void PlotScreen::setPlotHeight(unsigned inputHeight) {
  pw->setPlotHeight(inputHeight);
}

/* Set outer window size and inner plot size together */
void PlotScreen::setFixedSize(unsigned outWidth, unsigned outHeight,
                              unsigned inWidth, unsigned inHeight) {
  pw->setFixedSize(outWidth, outHeight, inWidth, inHeight);
  QWidget::setFixedSize(outWidth, outHeight);
}

/* Wrapper function to addVector(VB_Vector &, QColor ) */
int PlotScreen::addVector(const VB_Vector* inputVec, QColor inputColor) {
  return pw->addVector(*inputVec, inputColor);
}

/* This method is to add a new vector into the list of VB_Vector objects which
 * will be shown. Note that this function also initializes the corresponding
 * element in xMinList, xMaxList, colorList, yMinList, yMaxList and plot mode.
 * By default, x minimum is 0, x maximum is the vector's length - 1, the plot
 * mode is 1 (connect points together). The reason why maximum is the vector's
 * length-1 is because in mode 1, if there are n elements in the vector, only n
 * - 1 lines are drawn. The number of points counting from 0 to length -1 will
 * be the number of elements.
 *
 * It returns index of the new vector. */
int PlotScreen::addVector(const VB_Vector& newVec, QColor inputColor) {
  return pw->addVector(newVec, inputColor);
}

/* Wrapper function of  int addVector(VB_Vector &, double , double , QColor,
 * unsigned) */
int PlotScreen::addVector(const VB_Vector* inputVec, double inputXMin,
                          double inputXLength, QColor inputColor,
                          unsigned mode) {
  return pw->addVector(inputVec, inputXMin, inputXLength, inputColor, mode);
}

/* This function will add a new vector and set its min and max length on X axis.
 * It does addVector(VB_Vector *), setNewVecX() and setPlotColor() in one step.
 * It returns the new vector's index. */
int PlotScreen::addVector(const VB_Vector& inputVec, double inputXMin,
                          double inputXLength, QColor inputColor,
                          unsigned mode) {
  return pw->addVector(inputVec, inputXMin, inputXLength, inputColor, mode);
}

/* Read an input file into a new VB_Vector object, then add it to vecList. */
int PlotScreen::addVecFile(const char* inputFile, QColor inputColor) {
  return pw->addVecFile(inputFile, inputColor);
}

/* Read an input file and set it's range on X axis and plot color. */
int PlotScreen::addVecFile(const char* inputFile, double inputXMin,
                           double inputXLength, QColor inputColor,
                           unsigned mode) {
  return pw->addVector(inputFile, inputXMin, inputXLength, inputColor, mode);
}

/* This method is to delete a certain VB_Vector object from vector container. */
void PlotScreen::delVector(unsigned vecIndex) { pw->delVector(vecIndex); }

/* This function resets active curve's index */
void PlotScreen::resetActiveCurve(unsigned vecIndex) {
  pw->resetActiveCurve(vecIndex);
}

/* This method is to set a new vector's starting value on X axis. */
void PlotScreen::setNewVecXMin(unsigned vecIndex, double inputXMin) {
  pw->setNewVecXMin(vecIndex, inputXMin);
}

/* This method is to set a new vector's total length on X axis. */
void PlotScreen::setNewVecXLength(unsigned vecIndex, double inputXLength) {
  pw->setNewVecXLength(vecIndex, inputXLength);
}

/* This method is to set a new vector's starting value and the total length on X
 * axis. It combines setNewVecXMin() and setNewVecXLength() together. */
void PlotScreen::setNewVecX(unsigned vecIndex, double inputXMin,
                            double inputXLength) {
  pw->setNewVecX(vecIndex, inputXMin, inputXLength);
}

/* This method will set all vector's minimum and maximum on X axis */
void PlotScreen::setAllNewX(double inputXMin, double inputXLength) {
  pw->setAllNewX(inputXMin, inputXLength);
}

/* Set a certain curve's color. */
void PlotScreen::setPlotColor(unsigned inputIndex, QColor inputColor) {
  if (inputIndex < pw->vecList.size())
    pw->colorList[inputIndex] = inputColor;
  else
    printf("setPlotColor(): vecIndex out of range\n");
}

/* This method will set the plot mode for all vectors in the pool
 * It only accepts one argument: the new plot mode */
void PlotScreen::setPlotMode(unsigned inputMode) {
  if (inputMode == 0 || inputMode > 4) {
    printf("setPlotMode(unsigned): invalid plot mode.\n");
    return;
  }

  for (unsigned i = 0; i < pw->plotModeList.size(); i++)
    pw->plotModeList[i] = inputMode;
}

/* This function will set a certain vector's plot mode in the pool
 * It accepts two arguments: the first integer is the vector's index,
 * the second is the new mode for that vector. */
void PlotScreen::setPlotMode(unsigned vecIndex, unsigned inputMode) {
  if (vecIndex >= pw->plotModeList.size())
    printf("setPlotMode(unsigned, unsigned): vecIndex out of range.\n");
  else if (inputMode == 0 || inputMode > 4)
    printf("setPlotMode(unsigned, unsigned): invalid plot mode.\n");
  else
    pw->plotModeList[vecIndex] = inputMode;
}

/* This method is to clear the screen completely. */
void PlotScreen::clear() { pw->clear(); }

/* Update both scroll area and plot widget inside */
void PlotScreen::update() {
  QScrollArea::update();
  pw->update();
}

/* setFirstVector() will clear the vectors pool and add one vector into the
 * pool. This function is written specially for gdw interface, since only one
 * covariates will be shown on the upperwindow. */
void PlotScreen::setFirstVector(VB_Vector* inputVec) {
  pw->setFirstVector(inputVec);
}

/* setFirstXMarkMin() will set the first vector's original minimum value marked
 * on X axis. It is written specially for gdw interface */
void PlotScreen::setFirstXMarkMin(double inputValue) {
  pw->setNewVecXMin(0, inputValue);
}

/* This method is to set the first vector's original total length on X axis.
 * It is written specially for gdw interface. */
void PlotScreen::setFirstXLength(double inputLength) {
  pw->setNewVecXLength(0, inputLength);
}

/* This function will return the first vector's minimum element value */
double PlotScreen::getFirstXMin() { return pw->xMinList[0]; }

/* This function returns the full length of X axis */
double PlotScreen::getFullXLength() {
  double tmpVal = pw->getMax(pw->xMaxList) - pw->getMin(pw->xMinList);
  return tmpVal;
}

/* This function will return the first vector's total length shown on X axis. */
double PlotScreen::getFirstXLength() {
  return (pw->xMaxList[0] - pw->xMinList[0]);
}

/* This function returns the first vector's plot mode */
int PlotScreen::getFirstPlotMode() { return pw->plotModeList[0]; }

/* This function is written specially for gdw interface so that when plot mode
 * is changed from even to odd or from odd to even, the upsampling ratio is
 * considered. */
void PlotScreen::setRatio(unsigned inputRatio) {
  if (inputRatio == 0) {
    printf("setRatio() in PlotWidget: ratio must be positive\n");
    return;
  }

  pw->ratio = inputRatio;
}

/* This function will return the number of vectors in the pool. */
unsigned PlotScreen::getVecNum() { return pw->vecList.size(); }

/* This method is to set the graph's backgrond color */
void PlotScreen::setBkgdColor(QColor& inputColor) {
  pw->bkgdColor = inputColor;
  pw->setPaletteBackgroundColor(inputColor);
}

/* This method is to set the frame edge's color */
void PlotScreen::setEdgeColor(QColor& inputColor) {
  pw->edgeColor = inputColor;
}

/* This method is to set axis' color */
void PlotScreen::setAxisColor(QColor& inputColor) {
  pw->axisColor = inputColor;
}

/* This method is to set X and Y axis caption color */
void PlotScreen::setCaptionColor(QColor& inputColor) {
  pw->captionColor = inputColor;
}

/* This method is to set caption on X axis */
void PlotScreen::setXCaption(QString inputText) { pw->xCaption = inputText; }

/* This method is to set X axis caption's coordinates */
void PlotScreen::setXCaptionPost(unsigned x, unsigned y) {
  pw->xCaptionPostX = x;
  pw->xCaptionPostY = y;
}

/* This method is to set caption on Y axis */
void PlotScreen::setYCaption(QString inputText) { pw->yCaption = inputText; }

/* This method is to set Y axis caption's coordinates */
void PlotScreen::setYCaptionPost(unsigned x, unsigned y) {
  pw->yCaptionPostX = x, pw->yCaptionPostY = y;
}

/* enableFixedY() will set zoomYFlag to a certain value */
void PlotScreen::enableFixedY(bool inputFlag) { pw->enableFixedY(inputFlag); }

/* getFixedYFlag() returns the current value of zoomYFlag */
bool PlotScreen::getFixedYFlag() { return pw->zoomYFlag; }

/* setFixedY() will set the lower and upper bounds on Y axis */
void PlotScreen::setFixedY(double inputStart, double inputEnd) {
  pw->setFixedY(inputStart, inputEnd);
}

/* This method returns the original input Vector */
VB_Vector PlotScreen::getInputVector(unsigned vecIndex) {
  return pw->vecList[vecIndex];
}

/* This function sets the color of vertical line (generated by mouse press) */
void PlotScreen::setVLineColor(QColor inputColor) {
  pw->vLineColor = inputColor;
}

/* This function will enable/disbale mouse press, release and movement */
void PlotScreen::setMouseEnabled(bool inputStat) {
  pw->mouseEnabled = inputStat;
}

/* This function will enable/disable shift key functionality */
void PlotScreen::setShiftEnabled(bool inputStat) {
  pw->shiftEnabled = inputStat;
}

/* This function will enable/disable space key functionality */
void PlotScreen::setSpaceEnabled(bool inputStat) {
  pw->spaceEnabled = inputStat;
}

/* This function will enable/disable F1-F4 function keys */
void PlotScreen::setFKeyEnabled(bool inputStat) { pw->FKeyEnabled = inputStat; }

/* Emit x magnification change signal */
void PlotScreen::passMagSignal(int newVal) { emit(xMagChanged(newVal)); }

/* Slot that sets the visible range for PlotWidget */
void PlotScreen::setVisibleRange(int newVal) {
  if (pw->width() <= width()) {
    pw->visible_start = 0;
    return;
  }

  float hbar_range =
      horizontalScrollBar()->maximum() - horizontalScrollBar()->minimum();
  pw->visible_start = (pw->width() - width()) * newVal / hbar_range;
}

/* This is a slot to restore the default X axis starting and ending positions */
void PlotScreen::centerX() { pw->centerX(); }

/* This is a slot to change x axis magnification */
void PlotScreen::setXMag(int inputVal) { pw->setXMag(inputVal); }

/*************************************************************
 * Member functions of PlotWidget
 *************************************************************/
/* Basic constructor takes no arguments */
PlotWidget::PlotWidget(QWidget* parent) : QFrame(parent) { init(); }

/* Destructor */
PlotWidget::~PlotWidget() {
  clear();

  if (plotVector) delete plotVector;
}

/* This method is to initialize variables in PlotWidget class. */
void PlotWidget::init() {
  windowWidth = 600, windowHeight = 200,
  frameWidth = 2;  // default size of the window and frame width
  plotWidth = 500, plotHeight = 100;  // default size of the plot
  leftOffset = (windowWidth - plotWidth) /
               2;  // space between the frame and plot on left side
  upOffset = (windowHeight - plotHeight) /
             2;              // space between the frame and plot on upper side
  bkgdColor = Qt::black;     // default background color is black
  edgeColor = Qt::white;     // default edge color is white
  axisColor = Qt::white;     // default X and Y axis color is white
  captionColor = Qt::white;  // default X and Y axis caption's color is white
  plotMode = 1;              // default mode is 1
  plotVector = 0;

  xCaption = "X Axis";  // Default X axis
  yCaption = "Y Axis";  // Default Y axis
  xCaptionPostX =
      windowWidth / 2 -
      10;  // X caption: 10 pixels to the left side of the center of X axis
  xCaptionPostY = upOffset + plotHeight +
                  35;  // X caption coordinate of y: 35 pixels down from X axis
  yCaptionPostX =
      leftOffset -
      20;  // Y caption of x: 20 pixels to the left side of the center
  yCaptionPostY =
      upOffset - 10;  // Y axis caption of y: 10 pixels down from upper edge
  yDivision = 0;
  yDivisionInPixel = 0;
  zoomYFlag = false;
  zoomYStart = zoomYEnd = 0;

  setLineWidth(frameWidth);
  setFrameStyle(Panel | Sunken);
  setBackgroundMode(Qt::PaletteBase);

  QWidget::setMinimumSize(windowWidth + 2 * frameWidth,
                          windowHeight + 2 * frameWidth);
  setPaletteBackgroundColor(bkgdColor);

  curveLineWidth = 0;
  QFont myFont("Helvetica", 8);
  setFont(myFont);

  mouseX = mouseY = myX = 0;
  vLineColor = Qt::white;
  visible_start = 0;

  mouseEnabled = true;   // mouse is enabled by default
  shiftEnabled = true;   // shift key enabled by default
  spaceEnabled = true;   // space key enabled by default
  FKeyEnabled = true;    // F1-F4 keys enabled by default
  shiftPressed = false;  // shift key not pressed by default
  activeCurve = 0;       // 1st vector is active by default
  ratio = 1;

  xMagnification = 1;
  setFocusPolicy(Qt::ClickFocus);  // Interface has to be clicked before focus
}

/* This method is to clear the screen completely. */
void PlotWidget::clear() {
  vecList.clear();
  xMinList.clear();
  xMaxList.clear();
  yMinList.clear();
  yMaxList.clear();
  colorList.clear();
  plotModeList.clear();

  xStartPost.clear();
  xLengthInPixel.clear();

  // Reset highlighted curve's index and pen color to default
  activeCurve = 0;
}

/* Function that is in charge of painting. */
void PlotWidget::paintEvent(QPaintEvent*) {
  if (!vecList.size()) return;

  QPainter p(this);
  p.setPen(edgeColor);
  p.drawRect(leftOffset, upOffset, plotWidth, plotHeight);

  drawXAxis(p);
  drawYAxis(p);
  drawGraph(p);
  if (myX) {
    addVLine(p);
    addVLineTxt(p);
  }
}

/* Overloaded resizeEvent() */
void PlotWidget::resizeEvent(QResizeEvent* e) {
  QWidget::resizeEvent(e);
  windowHeight = height() - 2 * frameWidth;
  windowWidth = width() - 2 * frameWidth;
  resizePlot();
}

/* Reset plot size */
void PlotWidget::resizePlot() {
  int totalWidth = windowWidth + 2 * frameWidth;
  if (totalWidth >= 300)
    plotWidth = windowWidth - 100;
  else
    plotWidth = windowWidth - 40;

  int totalHeight = windowHeight + 2 * frameWidth;
  if (totalHeight >= 200)
    plotHeight = windowHeight - 100;
  else
    plotHeight = windowHeight - 40;

  updateSize();
  updateGeometry();
}

/* This method is to set the plot's size, first number width and second height
 */
void PlotWidget::setPlotSize(unsigned inputWidth, unsigned inputHeight) {
  if (inputWidth + 40 > windowWidth) {
    QMessageBox::critical(0, "Error",
                          "Outer window width should be at least 40 pixels "
                          "larger than inner width");
    return;
  }
  if (inputHeight + 40 > windowHeight) {
    QMessageBox::critical(0, "Error",
                          "Outer window height should be at least 40 pixels "
                          "larger than inner height");
    return;
  }

  plotWidth = inputWidth;
  plotHeight = inputHeight;
  updateSize();
}

/* This method is to set the plot's width */
void PlotWidget::setPlotWidth(unsigned inputWidth) {
  if (inputWidth + 40 > windowWidth) {
    printf("%d: Invalid plot width\n", inputWidth);
    return;
  }
  plotWidth = inputWidth;
  updateSize();
}

/* This method is to set the plot's height */
void PlotWidget::setPlotHeight(unsigned inputHeight) {
  if (inputHeight + 40 > windowHeight) {
    printf("%d: Invalid plot height\n", inputHeight);
    return;
  }

  plotHeight = inputHeight;
  updateSize();
}

/* Set outer window size and inner plot size together */
void PlotWidget::setFixedSize(unsigned outWidth, unsigned outHeight,
                              unsigned inWidth, unsigned inHeight) {
  unsigned w1 = outWidth - 2 * frameWidth;
  unsigned h1 = outHeight - 2 * frameWidth;

  if (inWidth + 40 > w1) {
    QMessageBox::critical(0, "Error",
                          "Outer window width should be at least 40 pixels "
                          "larger than inner width");
    return;
  }
  if (inHeight + 40 > h1) {
    QMessageBox::critical(0, "Error",
                          "Outer window height should be at least 40 pixels "
                          "larger than inner height");
    return;
  }
  if (w1 < 100) {
    QMessageBox::critical(0, "Error", "Minimum PlotWidget window width is 100");
    return;
  }
  if (h1 < 100) {
    QMessageBox::critical(0, "Error",
                          "Minimum PlotWidget window height is 100");
    return;
  }

  windowWidth = w1, windowHeight = h1;
  plotWidth = inWidth, plotHeight = inHeight;
  updateSize();
  QWidget::setFixedSize(outWidth, outHeight);
}

/* updateSize() will update the variables which are related to the window size.
 * It is called in setWindowSize(), setWindowWidth(), setWindowHeight() and
 * setPlotSize(), setPlotWidth(), setPlotHeight() */
void PlotWidget::updateSize() {
  leftOffset = (windowWidth - plotWidth) / 2;
  upOffset = (windowHeight - plotHeight) / 2;
  xCaptionPostX = width() / 2 - 10;
  xCaptionPostY = windowHeight - upOffset + 35;
  yCaptionPostX = leftOffset - 20;
  yCaptionPostY = upOffset - 10;
}

/* Wrapper function to addVector(VB_Vector &, QColor ) */
int PlotWidget::addVector(const VB_Vector* inputVec, QColor inputColor) {
  return addVector(*inputVec, inputColor);
}

/* This method is to add a new vector into the list of VB_Vector objects which
 * will be shown. Note that this function also initializes the corresponding
 * element in xMinList, xMaxList, colorList, yMinList, yMaxList and plot mode.
 * By default, x minimum is 0, x maximum is the vector's length - 1, the plot
 * mode is 1 (connect points together). The reason why maximum is the vector's
 * length-1 is because in mode 1, if there are n elements in the vector, only n
 * - 1 lines are drawn. The number of points counting from 0 to length -1 will
 * be the number of elements.
 *
 * It returns index of the new vector. */
int PlotWidget::addVector(const VB_Vector& newVec, QColor inputColor) {
  vecList.push_back(newVec);
  xMinList.push_back(0);
  xMaxList.push_back((double)newVec.getLength() - 1);
  colorList.push_back(inputColor);

  double tmpMin, tmpMax;
  if (newVec.getVariance() < 1e-10)
    tmpMin = tmpMax = newVec.getVectorMean();
  else {
    tmpMin = newVec.getMinElement();
    tmpMax = newVec.getMaxElement();
  }
  yMinList.push_back(tmpMin);
  yMaxList.push_back(tmpMax);

  plotModeList.push_back(1);
  xStartPost.push_back(0);
  xLengthInPixel.push_back(0);

  return vecList.size() - 1;
}

/* Wrapper function of  int addVector(VB_Vector &, double , double , QColor,
 * unsigned) */
int PlotWidget::addVector(const VB_Vector* inputVec, double inputXMin,
                          double inputXLength, QColor inputColor,
                          unsigned mode) {
  return addVector(*inputVec, inputXMin, inputXLength, inputColor, mode);
}

/* This function will add a new vector and set its min and max length on X axis.
 * It does addVector(VB_Vector *), setNewVecX() and setPlotColor() in one step.
 * It returns the new vector's index. */
int PlotWidget::addVector(const VB_Vector& inputVec, double inputXMin,
                          double inputXLength, QColor inputColor,
                          unsigned mode) {
  if (inputXLength <= 0) {
    printf("addVector(): inputXLength must be positive.\n");
    return -1;
  }

  if (mode == 0 || mode > 4) {
    printf("addVector(): invalid plot mode.\n");
    return -2;
  }

  vecList.push_back(inputVec);
  xMinList.push_back(inputXMin);
  xMaxList.push_back(inputXMin + inputXLength);
  colorList.push_back(inputColor);

  double tmpMin, tmpMax;
  if (inputVec.getVariance() < 1e-10)
    tmpMin = tmpMax = inputVec.getVectorMean();
  else {
    tmpMin = inputVec.getMinElement();
    tmpMax = inputVec.getMaxElement();
  }
  yMinList.push_back(tmpMin);
  yMaxList.push_back(tmpMax);

  plotModeList.push_back(mode);
  xStartPost.push_back(0);
  xLengthInPixel.push_back(0);

  return vecList.size() - 1;
}

/* Read an input file into a new VB_Vector object, then add it to vecList. */
int PlotWidget::addVecFile(const char* inputFile, QColor inputColor) {
  VB_Vector newVector(inputFile);
  return addVector(newVector, inputColor);
}

/* Read an input file and set it's range on X axis and plot color. */
int PlotWidget::addVecFile(const char* inputFile, double inputXMin,
                           double inputXLength, QColor inputColor,
                           unsigned mode) {
  VB_Vector newVec(inputFile);
  return addVector(newVec, inputXMin, inputXLength, inputColor, mode);
}

/* This method is to delete a certain VB_Vector object from vector container. */
void PlotWidget::delVector(unsigned vecIndex) {
  if (vecIndex < vecList.size()) {
    vecList.erase(vecList.begin() + vecIndex);
    xMinList.erase(xMinList.begin() + vecIndex);
    xMaxList.erase(xMaxList.begin() + vecIndex);
    yMinList.erase(yMinList.begin() + vecIndex);
    yMaxList.erase(yMaxList.begin() + vecIndex);
    colorList.erase(colorList.begin() + vecIndex);
    plotModeList.erase(plotModeList.begin() + vecIndex);
    xStartPost.erase(xStartPost.begin() + vecIndex);
    xLengthInPixel.erase(xLengthInPixel.begin() + vecIndex);
    // Reset active curve's index
    resetActiveCurve(vecIndex);
  } else
    printf("delVector(): vecIndex out of range\n");
}

/* This function resets active curve's index */
void PlotWidget::resetActiveCurve(unsigned vecIndex) {
  /* If the highlighted curve is going to be deleted and it is
   * the last curve, set the first curve to be highlighted */
  if (vecIndex == activeCurve && vecIndex == vecList.size() - 1)
    activeCurve = 0;
  // If the deleted curve is before the highlighted, decrease activeCurve by 1
  else if (vecIndex < activeCurve)
    activeCurve--;
}

/* This method is to set a new vector's starting value on X axis. */
void PlotWidget::setNewVecXMin(unsigned vecIndex, double inputXMin) {
  if (vecIndex < vecList.size()) {
    xMinList[vecIndex] = inputXMin;
  } else
    printf("setNewVecXMin(): vecIndex out of range\n");
}

/* This method is to set a new vector's total length on X axis. */
void PlotWidget::setNewVecXLength(unsigned vecIndex, double inputXLength) {
  if (inputXLength <= 0) {
    printf("setNewVecXLength(): inputXLength must be positive.\n");
    return;
  }

  if (vecIndex < vecList.size())
    xMaxList[vecIndex] = xMinList[vecIndex] + inputXLength;
  else
    printf("setNewVecXLength(): vecIndex out of range\n");
}

/* This method is to set a new vector's starting value and the total length on X
 * axis. It combines setNewVecXMin() and setNewVecXLength() together. */
void PlotWidget::setNewVecX(unsigned vecIndex, double inputXMin,
                            double inputXLength) {
  if (inputXLength <= 0) {
    printf("setNewVecX(): inputXLength must be positive.\n");
    return;
  }

  if (vecIndex < vecList.size()) {
    xMinList[vecIndex] = inputXMin;
    xMaxList[vecIndex] = inputXMin + inputXLength;
  } else
    printf("setNewVecX(): vecIndex out of range\n");
}

/* This method will set all vector's minimum and maximum on X axis */
void PlotWidget::setAllNewX(double inputXMin, double inputXLength) {
  for (unsigned i = 0; i < vecList.size(); i++)
    setNewVecX(i, inputXMin, inputXLength);
}

/* setFirstVector() will clear the vectors pool and add one vector into the
 * pool. This function is written specially for gdw interface, since only one
 * covariates will be shown on the upperwindow. */
void PlotWidget::setFirstVector(VB_Vector* inputVec) {
  if (vecList.size() > 0) clear();

  addVector(inputVec);
}

/* setFirstXMarkMin() will set the first vector's original minimum value marked
 * on X axis. It is written specially for gdw interface */
void PlotWidget::setFirstXMarkMin(double inputValue) {
  setNewVecXMin(0, inputValue);
}

/* This method is to set the first vector's original total length on X axis.
 * It is written specially for gdw interface. */
void PlotWidget::setFirstXLength(double inputLength) {
  if (inputLength <= 0) {
    printf("setFirstXLength(): inputLength must be positive.\n");
    return;
  }

  setNewVecXLength(0, inputLength);
}

/* This is the function to calculate the marked values which will be shown on X
 * axis */
void PlotWidget::calcXMark() {
  plotXMarkMin = getMin(xMinList);
  plotXLength = getMax(xMaxList) - plotXMarkMin;
}

/* This method is to draw X axis, including the caption. */
void PlotWidget::drawXAxis(QPainter& p) {
  p.setPen(axisColor);
  // if ((int) xCaptionPostX >= visibleWidth() || xCaptionPostY >= windowHeight)
  if (xCaptionPostX >= windowWidth || xCaptionPostY >= windowHeight)
    QMessageBox::critical(
        0, "Error", "The position assigned for X axis caption is not correct.");
  else
    p.drawText(xCaptionPostX, xCaptionPostY, xCaption);

  calcXMark();
  double n = getXRange(plotXLength);
  double xDivision = n / 100.0;  // Each division's value on X axis
  double xDivisionNumDb =
      plotXLength / xDivision;  // This variable makes the x mark more accurate
  int xDivisionNumber =
      (int)xDivisionNumDb;  // Number of divisions on X axis (integer)
  double xDivisionInPixel =
      (double)plotWidth /
      xDivisionNumDb;  // Each division's value on X axis in pixel

  /* Distance from the upper edge of window to X axis, (theoretically it should
   * be upOffset + plotHeight, but minus one make it look nicer.) */
  int xAxisPost = upOffset + plotHeight - 1;  // -1 makes it look nicer
  int shortMark = 2,
      longMark = 5;  // Short mark is 2 pixels, longer one is 5 pixels

  for (int i = 1; i <= xDivisionNumber;
       i++) {  // Draw a shorter line to mark each division
    p.drawLine(leftOffset + (int)(xDivisionInPixel * i), xAxisPost,
               leftOffset + (int)(xDivisionInPixel * i),
               xAxisPost - shortMark);  // bottom
    p.drawLine(leftOffset + (int)(xDivisionInPixel * i), upOffset,
               leftOffset + (int)(xDivisionInPixel * i),
               upOffset + shortMark);  // up
  }

  int increment = getIncrement(
      xDivisionNumber);  // This variable controls the interval of the marks
  int labelWidth = plotWidth / (xDivisionNumber / increment + 1);
  int xMarkX = leftOffset - labelWidth / 2;  // x coordinate of marks on x axis
  int xMarkY = upOffset + plotHeight + 15;   // y coordinate of marks on x axis

  double xMarkInDb;
  for (int i = 0; i <= xDivisionNumber; i += increment) {
    p.drawLine(leftOffset + (int)(xDivisionInPixel * i), xAxisPost,
               leftOffset + (int)(xDivisionInPixel * i),
               xAxisPost - longMark);  // bottom
    p.drawLine(leftOffset + (int)(xDivisionInPixel * i), upOffset,
               leftOffset + (int)(xDivisionInPixel * i),
               upOffset + longMark);  // up

    xMarkInDb = plotXMarkMin + xDivision * i;
    if (fabs(xMarkInDb) < 1e-7 && xDivision > 1e-7) xMarkInDb = 0;
    QString xLabel = QString::number(xMarkInDb);
    // mark the divisions
    p.drawText(xMarkX + (int)(xDivisionInPixel * i), xMarkY - 10, labelWidth,
               20, Qt::AlignHCenter, xLabel);
  }
}

/* calcYMark() will compare the max and min values of each VB_Vector objects
 * and figure out the final min and max */
void PlotWidget::calcYMark() {
  if (zoomYFlag) {
    plotYMarkMin = zoomYStart;
    plotYLength = zoomYEnd - zoomYStart;
    if (yDivision) return;
  } else {
    plotYMarkMin = getMin(yMinList);
    plotYLength = getMax(yMaxList) - plotYMarkMin;
  }
  // Set yDivision, which is each division's value on Y axis
  if (plotYLength > 0) {
    double yRange = getYRange(plotYLength);
    if (plotYLength / (yRange / 10.0) >= 5)
      yDivision = yRange / 10.0;
    else if (plotYLength / (yRange / 20.0) >= 5)
      yDivision = yRange / 20.0;
    else
      yDivision = yRange / 50.0;
  } else if (plotYMarkMin)
    yDivision = plotYMarkMin;
  else
    yDivision = 1;
}

/* This method is to draw Y axis, including its caption */
void PlotWidget::drawYAxis(QPainter& p) {
  p.setPen(axisColor);
  // Draw Y axis caption
  if (yCaptionPostX <= 0 || yCaptionPostY <= 0)
    QMessageBox::critical(
        0, "Error",
        "The position you assigned for Y axis caption is not correct. \
<P>Please change it and try again.");
  else
    p.drawText(yCaptionPostX, yCaptionPostY, yCaption);

  calcYMark();
  double max = plotYLength + plotYMarkMin;
  double yDivisionNumber;
  // Most of the times plotYLength should be positive
  if (plotYLength > 0) {
    if (zoomYFlag) {
      yBaseValue = plotYMarkMin;  // Minimum value of the labels on Y axis
      yMaxValue =
          plotYLength + plotYMarkMin;  // Maximum value of the labels on Y axis
      // Number of divisions on Y axis
      yDivisionNumber = plotYLength / yDivision;
    }
    // If zoomYFlag isn't set, draw Y axis automatically
    else {
      double yMin = floor(plotYMarkMin / yDivision);
      double diffMin = plotYMarkMin - yMin * yDivision;
      /* 1e-2 is an arbitrary value to make sure no point will overlap with X
       * axis. No paractical use, it just makes the plot look silightly nicer.
       * Same setup for yMax. */
      if (diffMin <= yDivision * 1e-2) yMin--;
      double yMax = ceil(max / yDivision);
      double diffMax = yMax * yDivision - max;
      if (diffMax <= yDivision * 1e-2) yMax++;
      ;

      yBaseValue = yDivision * yMin;  // Minimum value of the labels on Y axis
      yMaxValue = yDivision * yMax;   // Maximum value of the labels on Y axis
      yDivisionNumber = yMax - yMin;
    }
    yDivisionInPixel = (double)plotHeight / yDivisionNumber;
    // Distance from left side of the frame to right side of the plot area, -1
    // makes it seem nicer
    int yAxisPost = leftOffset + plotWidth - 1;
    // Distance from minimum Y axis mark point to upper side of the frame
    int yMinPost = upOffset + plotHeight;
    double foo = yBaseValue / yDivision;
    int foo_int = (int)foo;
    double firstMark, firstY;
    int yStartDivision;
    if (foo == (double)foo_int) {
      firstMark = 0;
      firstY = yBaseValue;
      yStartDivision = foo_int;
    } else if (foo > 0) {
      firstMark = ((double)foo_int + 1.0 - foo) * yDivisionInPixel;
      firstY = ((double)foo_int + 1.0) * yDivision;
      yStartDivision = foo_int + 1;
    } else {
      firstMark = ((double)foo_int - foo) * yDivisionInPixel;
      firstY = (double)foo_int * yDivision;
      yStartDivision = foo_int;
    }
    /* Force the mark value to be zero when it's smaller than 1e-7 and the
     * division is larger than 1e-7. Without this somtimes it's nonzero when it
     * is supposed to be because of rounding error. */
    if (fabs(firstY) < 1.0e-7 && yDivision > 1.0e-7) firstY = 0;

    int increment = getIncrement(yDivisionNumber);
    int shortMark = 2, longMark = 5;
    int yMarkX = 0;                          // x coordinate of Y axis marks
    int yMarkY = upOffset + plotHeight - 5;  // y coordinate of Y axis marks
    // Loop to mark divisions in shorter line
    for (int i = 0; i <= (int)yDivisionNumber; i++) {
      if ((yStartDivision + i) % increment != 0) {
        p.drawLine(leftOffset,
                   yMinPost - (int)(firstMark + yDivisionInPixel * i),
                   leftOffset + shortMark,
                   yMinPost - (int)(firstMark + yDivisionInPixel * i));  // Left
        p.drawLine(
            yAxisPost, yMinPost - (int)(firstMark + yDivisionInPixel * i),
            yAxisPost - shortMark,
            yMinPost - (int)(firstMark + yDivisionInPixel * i));  // Right
      } else {
        p.drawLine(leftOffset,
                   yMinPost - (int)(firstMark + yDivisionInPixel * i),
                   leftOffset + longMark,
                   yMinPost - (int)(firstMark + yDivisionInPixel * i));  // Left
        p.drawLine(
            yAxisPost, yMinPost - (int)(firstMark + yDivisionInPixel * i),
            yAxisPost - longMark,
            yMinPost - (int)(firstMark + yDivisionInPixel * i));  // Right
        if (fabs(firstY) < 1.0e-7 && yDivision > 1.0e-7) firstY = 0;
        QString yLabel = QString::number(firstY);
        yLabel.truncate(7);  // Only keep the first 7 characters in the label
        // label Y axis divisions with value
        p.drawText(yMarkX, yMarkY - (int)(firstMark + yDivisionInPixel * i),
                   leftOffset - 5, 20, Qt::AlignRight, yLabel);
      }
      firstY += yDivision;
    }

  }

  /* When all the elements are equal to a positive value, set Y axis base value
   * to be zero, separate Y axis in two divisions, with the element's value in
   * the middle. */
  else if (max > 0) {
    int yDivisionNumber = 2;
    yDivisionInPixel = (double)plotHeight / (double)yDivisionNumber;
    yBaseValue = 0;  // Set Y base value to be 0
    yMaxValue = 2 * max;

    // Now mark each division using longer marks on left and right sides
    p.drawLine(leftOffset, upOffset + plotHeight / 2, leftOffset + 5,
               upOffset + plotHeight / 2);
    p.drawLine(leftOffset + plotWidth, upOffset + plotHeight / 2,
               leftOffset + plotWidth - 5, upOffset + plotHeight / 2);
    QString yLabel1 = QString::number(max);
    yLabel1.truncate(7);
    QString yLabel2 = QString::number(max * 2);
    yLabel2.truncate(7);
    // Label the divisions on the left side: 0, max, 2*max (from bottom to upper
    // side)
    int yMarkX = 0;
    p.drawText(yMarkX, upOffset + plotHeight - 8, leftOffset - 5, 20,
               Qt::AlignRight, "0");
    p.drawText(yMarkX, upOffset + plotHeight / 2 - 8, leftOffset - 5, 20,
               Qt::AlignRight, yLabel1);
    p.drawText(yMarkX, upOffset - 8, leftOffset - 5, 20, Qt::AlignRight,
               yLabel2);
  }

  /* When all the elements are equal to a negative value, set Y axis base value
   * to be twice the value, separate Y axis in  two divisions, with the
   * element's value in the middle. */
  else if (max < 0) {
    int yDivisionNumber = 2;
    yDivisionInPixel = (double)plotHeight / (double)yDivisionNumber;
    yBaseValue = 2 * max;  // Set Y base value to be 2*max
    yMaxValue = 0;

    // Now mark each division using longer marks on left and right sides
    p.drawLine(leftOffset, upOffset + plotHeight / 2, leftOffset + 5,
               upOffset + plotHeight / 2);
    p.drawLine(leftOffset + plotWidth, upOffset + plotHeight / 2,
               leftOffset + plotWidth - 5, upOffset + plotHeight / 2);
    QString yLabel1 = QString::number(max);
    yLabel1.truncate(7);
    QString yLabel2 = QString::number(max * 2);
    yLabel2.truncate(7);
    // Label the divisions on the left side: 2*max, max, 0 (from bottom to upper
    // side)
    int yMarkX = 0;
    p.drawText(yMarkX, upOffset - 8, leftOffset - 5, 20, Qt::AlignRight, "0");
    p.drawText(yMarkX, upOffset + plotHeight / 2 - 8, leftOffset - 5, 20,
               Qt::AlignRight, yLabel1);
    p.drawText(yMarkX, upOffset + plotHeight - 8, leftOffset - 5, 20,
               Qt::AlignRight, yLabel2);
  }

  /* When all the elements are equal to zero, set Y base value to be -1, maximum
   * value of Y axis is +1, zero is in the middle. */
  else {
    int yDivisionNumber = 2;
    yDivisionInPixel = (double)plotHeight / (double)yDivisionNumber;
    yBaseValue = -1;
    yMaxValue = 1;

    // Now mark each division using longer marks on left and right sides
    p.drawLine(leftOffset, upOffset + plotHeight / 2, leftOffset + 5,
               upOffset + plotHeight / 2);
    p.drawLine(leftOffset + plotWidth, upOffset + plotHeight / 2,
               leftOffset + plotWidth - 5, upOffset + plotHeight / 2);

    int yMarkX = 0;
    p.drawText(yMarkX, upOffset - 8, leftOffset - 5, 20, Qt::AlignRight, "1");
    p.drawText(yMarkX, upOffset + plotHeight / 2 - 8, leftOffset - 5, 20,
               Qt::AlignRight, "0");
    p.drawText(yMarkX, upOffset + plotHeight - 8, leftOffset - 5, 20,
               Qt::AlignRight, "-1");
  }
}

/* drawGraph() draws the graph inside, it's only a wrapper for drawInModeX() (X
 * is 1, 2, 3 or 4) */
void PlotWidget::drawGraph(QPainter& p) {
  for (unsigned i = 0; i < vecList.size(); i++) {
    calcXIndex(i);

    int currentWidth = curveLineWidth;
    if (vecList.size() > 1 && i == activeCurve) currentWidth += 2;
    QPen curvePen(colorList[i], currentWidth);
    p.setPen(curvePen);

    unsigned currentMode = plotModeList[i];
    plotVector = new VB_Vector(vecList[i]);
    if (currentMode == 1)
      drawInMode1(p, i);
    else if (currentMode == 2)
      drawInMode2(p, i);
    else if (currentMode == 3)
      drawInMode3(p, i);
    else if (currentMode == 4)
      drawInMode4(p, i);
    else
      printf("drawGraph(): invalid plot mode.\n");
  }
}

/* calcXIndex() calculates starting and ending index of a certain vector
 * based on the vector's plotXLength and plotXMarkMin. */
void PlotWidget::calcXIndex(unsigned vecIndex) {
  double xMin = xMinList[vecIndex];
  double xMax = xMaxList[vecIndex];

  xLengthInPixel[vecIndex] = (xMax - xMin) / plotXLength * plotWidth;
  xStartPost[vecIndex] =
      (xMin - plotXMarkMin) / plotXLength * plotWidth + (double)leftOffset;
}

/* checkVal() is a simple function to check whether an input value is between
 * yBaseValue and yMaxValue. It returns true if it is, false if not. This
 * unction is written to check
 * whether a certain element should be drawn or not in the graph. */
int PlotWidget::checkVal(double inputVal) {
  if (inputVal > yMaxValue) return 1;
  if (inputVal < yBaseValue) return -1;
  return 0;
}

/* calcXEdge() calculates the x coorinate of the edge points.
 * This function accepts four arguments:
 * yStart is the y value of first points, yEdge is y axis end value,
 * yEnd is the second point's y cvalue, which will be out of Y axis range;
 * xTotal is the difference of the two points' X coordinates;
 * This function returns the x coordinate of the point where Y axis edge and
 * the line connetcted by the two points intersect. */
double PlotWidget::calcXEdge(double yStart, double yEdge, double yEnd,
                             double xTotal) {
  double yTotal = fabs(yEnd - yStart);
  double yShownup = fabs(yEdge - yStart);

  return xTotal * yShownup / yTotal;
}

/* This method is to draw the graph in mode 1, which simply connects the points
 * together. The biggest difference between mode 1 and mode 2 is: In mode 1, if
 * there are n elements in a vector, n-1 lines will be drawn. But in mode 2, the
 * number of lines drawn will be n, since each element will correspond to a
 * horizonal line. */
void PlotWidget::drawInMode1(QPainter& p, unsigned vecIndex) {
  // default is 149, theoretically should be 150, but 149 looks nicer
  int yMinPost = upOffset + plotHeight - 1;
  int yMaxPost = upOffset - 1;
  int element1 = 0, element2 = 0;
  double m;
  double val1, val2;
  unsigned vecLen = vecList[vecIndex].getLength();

  // Note the difference between mode 1 and 2 here!
  xInPixel = xLengthInPixel[vecIndex] / (double)(vecLen - 1);
  for (unsigned x = 0; x < vecLen - 1; x++) {
    val1 = plotVector->getElement(x);
    val2 = plotVector->getElement(x + 1);
    element1 = (int)((plotVector->getElement(x) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    element2 = (int)((plotVector->getElement(x + 1) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    m = (x + 1) * xInPixel;
    // If both points are in Y axis range, connect them by a line
    if (checkVal(val1) == 0 && checkVal(val2) == 0) {
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1, (int)(xStartPost[vecIndex] + m),
                 yMinPost - element2);
    }
    // If the first point is in Y range, but the second point is bigger than Y
    // maximum value, connect until the Y upper edge
    else if (checkVal(val1) == 0 && checkVal(val2) == 1) {
      double xEdge = calcXEdge(val1, yMaxValue, val2, xInPixel);
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1,
                 (int)(xStartPost[vecIndex] + m - xInPixel + xEdge), yMaxPost);
    }
    // If the first point is in Y range, but the second point is less than Y
    // minimum value, connect until the Y lower edge
    else if (checkVal(val1) == 0 && checkVal(val2) == -1) {
      double xEdge = calcXEdge(val1, yBaseValue, val2, xInPixel);
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1,
                 (int)(xStartPost[vecIndex] + m - xInPixel + xEdge), yMinPost);
    }
    // If the first point is out of Y range (upper), but the second point is in
    // Y range, connect from the Y upper edge to the second point
    else if (checkVal(val1) == 1 && checkVal(val2) == 0) {
      double xEdge = calcXEdge(val1, yMaxValue, val2, xInPixel);
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel + xEdge), yMaxPost,
                 (int)(xStartPost[vecIndex] + m), yMinPost - element2);
    }
    // If the first point is out of Y range (lower), but the second point is in
    // the range, connect until the Y lower edge
    else if (checkVal(val1) == -1 && checkVal(val2) == 0) {
      double xEdge = calcXEdge(val1, yBaseValue, val2, xInPixel);
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel + xEdge), yMinPost,
                 (int)(xStartPost[vecIndex] + m), yMinPost - element2);
    }
  }
}

/* This method draws the graph in mode 2, which first draws a horizontal line,
 * then connect the lines together. It's  designed to simulate Geoff's IDL code
 * to draw the block design reference function (eg. motor1.ref). */
void PlotWidget::drawInMode2(QPainter& p, unsigned vecIndex) {
  int yMinPost = upOffset + plotHeight - 1;
  int yMaxPost = upOffset - 1;
  int element1 = 0, element2 = 0;
  double val1 = 0, val2 = 0;
  double m;
  unsigned vecLen = vecList[vecIndex].getLength();

  // Key difference between mode 1, 3 and 2, 4
  xInPixel = xLengthInPixel[vecIndex] / (double)vecLen;
  for (unsigned x = 0; x < vecLen - 1; x++) {
    val1 = plotVector->getElement(x);
    val2 = plotVector->getElement(x + 1);
    element1 = (int)((plotVector->getElement(x) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    element2 = (int)((plotVector->getElement(x + 1) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    m = (x + 1) * xInPixel;
    // If both points are in Y range
    if (checkVal(val1) == 0 && checkVal(val2) == 0) {
      // Draw a flat line first
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1, (int)(xStartPost[vecIndex] + m),
                 yMinPost - element1);
      // Then draw a vertical line to connect the lines together, copied from
      // Geoff's IDL interface
      p.drawLine((int)(xStartPost[vecIndex] + m), yMinPost - element1,
                 (int)(xStartPost[vecIndex] + m), yMinPost - element2);
    }
    // If the first point is in range but second point out of Y range (upper)
    else if (checkVal(val1) == 0 && checkVal(val2) == 1) {
      // First draw a flat line
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1, (int)(xStartPost[vecIndex] + m),
                 yMinPost - element1);
      // Then draw a vertical line up to Y axis maximum
      p.drawLine((int)(xStartPost[vecIndex] + m), yMinPost - element1,
                 (int)(xStartPost[vecIndex] + m), yMaxPost);
    }
    // If the first point is out of Y range (upper), but the second point is
    // inside
    else if (checkVal(val1) == 1 && checkVal(val2) == 0) {
      // Simply draw a vertical line between Y maximum and the second point
      p.drawLine((int)(xStartPost[vecIndex] + m), yMaxPost,
                 (int)(xStartPost[vecIndex] + m), yMinPost - element2);
    }
    // If the first point is outside (lower) and second point is inside
    else if (checkVal(val1) == -1 && checkVal(val2) == 0) {
      // Simply draw a vertical line between Y minimum and the second point
      p.drawLine((int)(xStartPost[vecIndex] + m), yMaxPost,
                 (int)(xStartPost[vecIndex] + m), yMinPost - element2);
    }
  }
  // If the last point is inside, draw a flat line for it
  if (checkVal(val2) == 0)
    p.drawLine(
        (int)(xStartPost[vecIndex] + xLengthInPixel[vecIndex] - xInPixel),
        yMinPost - element2,
        (int)(xStartPost[vecIndex] + xLengthInPixel[vecIndex]),
        yMinPost - element2);
}

/* drawInMode3() is similar to drawInMode1(), but it only draws a small point
 * for each element It DOESN'T connect the points together. */
void PlotWidget::drawInMode3(QPainter& p, unsigned vecIndex) {
  int yMinPost = upOffset + plotHeight - 1;
  int element1 = 0;
  double m;
  double val1;
  unsigned vecLen = vecList[vecIndex].getLength();
  xInPixel = xLengthInPixel[vecIndex] / (double)(vecLen - 1);
  for (unsigned x = 0; x <= vecLen - 1; x++) {
    val1 = plotVector->getElement(x);
    element1 = (int)((plotVector->getElement(x) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    m = (x + 1) * xInPixel;
    // Draw a circle whose radius is 2 and the center is where the point should
    // be
    if (checkVal(val1) == 0)
      p.drawEllipse((int)(xStartPost[vecIndex] + m - xInPixel) - 2,
                    yMinPost - element1 - 2, 4, 4);
  }
}

/* drawInMode4() is similar to drawInMode2(), but it only draws a horizonal
 * line. It doesn't connect these lines together. */
void PlotWidget::drawInMode4(QPainter& p, unsigned vecIndex) {
  int yMinPost = upOffset + plotHeight - 1;
  int element1 = 0, element2 = 0;
  double m;
  double val1 = 0, val2 = 0;
  unsigned vecLen = vecList[vecIndex].getLength();
  // The calculation of xInPixel is one of the key differences between mode 1, 3
  // and 2, 4
  xInPixel = xLengthInPixel[vecIndex] / (double)(vecLen);
  for (unsigned x = 0; x < vecLen - 1; x++) {
    val1 = plotVector->getElement(x);
    val2 = plotVector->getElement(x + 1);
    element1 = (int)((plotVector->getElement(x) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    element2 = (int)((plotVector->getElement(x + 1) - yBaseValue) / yDivision *
                     yDivisionInPixel);
    m = (x + 1) * xInPixel;
    // Draw a horizontal line
    if (checkVal(val1) == 0)
      p.drawLine((int)(xStartPost[vecIndex] + m - xInPixel),
                 yMinPost - element1, (int)(xStartPost[vecIndex] + m),
                 yMinPost - element1);
  }

  // Draw the horizonal line for the last element
  if (checkVal(val2) == 0)
    p.drawLine(
        (int)(xStartPost[vecIndex] + xLengthInPixel[vecIndex] - xInPixel),
        yMinPost - element2,
        (int)(xStartPost[vecIndex] + xLengthInPixel[vecIndex]),
        yMinPost - element2);
}

/* Method to draw text which rotates certain degrees at a certain point */
void PlotWidget::drawRotatedText(QPainter& p, float degrees, int x, int y,
                                 const QString& text) {
  p.save();
  p.translate(x, y);       // Translate the painter to a certain point
  p.rotate(degrees);       // Rotate the coordinate
  p.drawText(0, 0, text);  // Draw text at new coordinates of (0,0)
  p.restore();
}

/* enableFixedY() will set zoomYFlag to a certain value */
void PlotWidget::enableFixedY(bool inputFlag) {
  zoomYFlag = inputFlag;
  if (!zoomYFlag) {
    yDivision = 0;
    zoomYStart = zoomYEnd = 0;
  } else if (zoomYStart == 0 && zoomYEnd == 0) {
    zoomYStart = yBaseValue;
    zoomYEnd = yMaxValue;
  }
}

/* setFixedY() will set the lower and upper bounds on Y axis */
void PlotWidget::setFixedY(double inputStart, double inputEnd) {
  if (vecList.size() == 0)
    QMessageBox::critical(
        0, "Error",
        "setFixedY(): Please first input a vector for the plot area.");
  else if (inputStart > getMax(yMaxList))
    QMessageBox::critical(0, "Error",
                          "setFixedY(): The starting value on Y axis is larger "
                          "than the maximum permitted.");
  else if (inputEnd < getMin(yMinList))
    QMessageBox::critical(0, "Error",
                          "setFixedY(): The ending value on Y axis is less "
                          "than the minimum permitted.");
  else if (inputEnd <= inputStart)
    QMessageBox::critical(0, "Error",
                          "setFixedY(): The ending value must be larger than "
                          "the starting value.");
  else {
    zoomYStart = inputStart;
    zoomYEnd = inputEnd;
    yDivision = 0;
    zoomYFlag = true;
  }
}

/* This method is used to gauge X axis range.
 * Note: the inputNumber must be nonnegative! */
double PlotWidget::getXRange(double inputNumber) {
  double n = 1.0;
  // when inputNumber is non-positive, return 0
  if (inputNumber <= 0)
    return 0;
  else if (inputNumber == 1)
    return 1.0;
  else if (inputNumber > 1.0) {
    while (inputNumber > n) n *= 10.0;
    return n;  // returnNumber / 10 < inputNumber <= returnNumber
  } else {
    while (inputNumber <= n) n /= 10.0;
    return n * 10;  // returnNumber / 10 < inputNumber <= returnNumber
  }
}

/* This method is used to gauge Y axis range.
 * Note the difference between getXYange() and getYRange() */
double PlotWidget::getYRange(double inputNumber) {
  double n = 1.0;
  // when inputNumber is non-positive, return 0
  if (inputNumber <= 0)
    return 0;
  else if (inputNumber == 1)
    return 1.0;
  else if (inputNumber > 1.0) {
    while (inputNumber > n) {
      // if a number is between 1.0 and 1.1, return 1, because an extra division
      // will cover it
      if (inputNumber < n * 1.1)
        return n;
      else
        n *= 10.0;
    }
    return n;  // returnNumber / 10 < inputNumber <= returnNumber
  } else {
    while (inputNumber <= n) n /= 10.0;
    return n * 10;  // returnNumber / 10 < inputNumber <= returnNumber
  }
}

/* This method determines the seperation of each marked values on x/y axis. */
int PlotWidget::getIncrement(double inputNumber) {
  // If there are more than 50 divisions, mark the value once every 10 divisions
  if (inputNumber > 50.0) return 10;
  // If there are more than 10 divisions, mark the value once every 5 divisions
  if (inputNumber > 10.0) return 5;
  // If there are 5 - 10 divisions, mark the value once every 2 divisions
  if (inputNumber > 5.0) return 2;
  // If there are less than or equal to 5 divisions, mark all of them
  return 1;
}

/* This method returns the minimum of the vectors' starting value on X axis */
double PlotWidget::getMin(std::vector<double> inputArr) {
  double tmp = inputArr[0];
  for (int i = 1; i < (int)inputArr.size(); i++) {
    if (tmp > inputArr[i])
      tmp = inputArr[i];
    else
      continue;
  }

  return tmp;
}

/* This method returns the maximum of the vectors' ending value on X axis */
double PlotWidget::getMax(std::vector<double> inputArr) {
  double tmp = inputArr[0];
  if (inputArr.size() == 1) return tmp;

  for (int i = 1; i < (int)inputArr.size(); i++) {
    if (tmp < inputArr[i])
      tmp = inputArr[i];
    else
      continue;
  }
  return tmp;
}

/* When mouse is pressed, draw a vertical line at its current position,
 * print out the current coordinate of the curve on the screen */
void PlotWidget::mousePressEvent(QMouseEvent* myMouse) {
  // when mouse button is clicked, set the keyboard focus to current screen
  setFocus();

  // Do nothing if mouse is disabled or no curve at all
  if (!mouseEnabled || vecList.size() == 0) return;

  mouseX = myMouse->x();
  mouseY = myMouse->y();
  // Do nothing if mouse is out of X axis range
  if (!chkMouseX()) {
    myX = 0;
    return;
  }
  /* if myX is zero, do not draw vertical line (for example, the mouse
   * position is inside plot window but outside of active curve's range. */
  setMyX();
  update();
}

/* This function checks whether a point's X coordinate is out of scale or not */
bool PlotWidget::chkMouseX() {
  // Is mouse position out of plot range?
  if ((unsigned)mouseX < leftOffset ||
      (unsigned)mouseX > plotWidth + leftOffset)
    return false;
  // Is mouse out of visible area?
  if (mouseX < visible_start ||
      mouseX > visible_start + parentWidget()->width())
    return false;

  return true;
}

/* This function will set up myX, which is the position where a vertical line is
 * drawn. Depending on whether shift key is pressed, the line position is
 * different. */
void PlotWidget::setMyX() {
  if (shiftPressed) {
    myX = mouseX;
    return;
  }

  double startPix = xStartPost[activeCurve];
  double totalPix = xLengthInPixel[activeCurve];
  if (mouseX < startPix || mouseX > startPix + totalPix) {
    myX = 0;
    return;
  }

  int currentMode = plotModeList[activeCurve];
  int vecLen = vecList[activeCurve].getLength();
  double x_ratio = ((double)mouseX - startPix) / totalPix;
  double x_index, ratio2;
  if (currentMode % 2) {
    x_index = x_ratio * (vecLen - 1);
    double x1 = round(x_index);
    ratio2 = x1 / (vecLen - 1);
  } else {
    x_index = x_ratio * vecLen;
    double x1 = round(x_index);
    ratio2 = x1 / vecLen;
  }

  myX = (unsigned)(startPix + totalPix * ratio2);
}

/* This function adds a QWidget on screen when mouse is pressed */
void PlotWidget::addVLine(QPainter& p) {
  p.setPen(vLineColor);
  if (vecList.size() > 1) p.setPen(colorList[activeCurve]);
  p.drawLine(myX, upOffset, myX, upOffset + plotHeight);
}

/* This function draws X and Y coordinates according to the shift key and plot
 * mode status */
void PlotWidget::addVLineTxt(QPainter& p) {
  QFont lineFont = QFont();
  lineFont.setPixelSize(upOffset / 3);
  lineFont.setBold(true);
  p.setFont(lineFont);
  // Print message if selected curve is out of range
  if (xLengthInPixel[activeCurve] == 0) {
    p.drawText(visible_start + parentWidget()->width() / 4, upOffset / 2,
               parentWidget()->width() / 2, upOffset / 2 - 1, Qt::AlignLeft,
               "Selected curve not shown");
    return;
  }

  if (vecList.size() > 1) p.setPen(colorList[activeCurve]);

  if (shiftPressed && plotModeList[activeCurve] % 2 != 0)
    setXY_13();
  else if (shiftPressed && plotModeList[activeCurve] % 2 == 0)
    setXY_24();
  else if (plotModeList[activeCurve] % 2 != 0)
    setXY_shift13();
  else {
    setXY_shift24();
  }

  p.drawText(visible_start + parentWidget()->width() / 4, upOffset / 2,
             parentWidget()->width() / 4, upOffset / 2 - 1, Qt::AlignRight,
             vline_x);
  p.drawText(visible_start + parentWidget()->width() / 2, upOffset / 2,
             parentWidget()->width() / 4, upOffset / 2 - 1, Qt::AlignLeft,
             vline_y);
}

/* When mouse is released, erase the vertical line and number on screen */
void PlotWidget::mouseReleaseEvent(QMouseEvent*) {
  if (!mouseEnabled || vecList.size() == 0) return;

  mouseX = mouseY = 0;
  myX = 0;
  update();
}

/* This function tracks the mouse movement and erases previous vertical line,
 * then draw another vertical line at current position. */
void PlotWidget::mouseMoveEvent(QMouseEvent* myMouse) {
  if (!mouseEnabled || vecList.size() == 0) return;

  mouseX = myMouse->x();
  mouseY = myMouse->y();
  if (!chkMouseX()) {
    myX = 0;
    return;
  }

  setMyX();
  update();
}

/* Mouse wheel movement will also zoom in/out in X direction */
void PlotWidget::wheelEvent(QWheelEvent* e) {
  if (!mouseEnabled || vecList.size() == 0) return;

  // when e->delta() is 120, wheel is moving upward, zoom in X axis
  if (e->delta() > 0) pressUp();
  // when e->delta() is -120, wheel is moving downward, zoom out X axis
  else
    pressDown();
}

/* Overwrited function to handle key press event */
void PlotWidget::keyPressEvent(QKeyEvent* e) {
  // Ignore ctrl and s key presses so that they will be handled by parent
  // widget; Otherwise the two keys won't response in parent widget
  if (e->key() == Qt::Key_Control || e->key() == Qt::Key_S) e->ignore();

  // Do nothing if no vector available
  if (vecList.size() == 0) return;
  // shift key pressed
  if (e->key() == Qt::Key_Shift) pressShift();
  // space key pressed
  else if (e->key() == Qt::Key_Space)
    pressSpace();
  // up arrow key pressed
  else if (e->key() == Qt::Key_Up)
    pressUp();
  // down arrow key pressed
  else if (e->key() == Qt::Key_Down)
    pressDown();
  // "1" key is pressed
  else if (e->key() == Qt::Key_1)
    press1key();
  // pressFKey() deals with the rest of keyboard presses
  else
    pressFKey(e);
}

/* Overwritten function to handle key release event */
void PlotWidget::keyReleaseEvent(QKeyEvent* e) {
  // Ignore ctrl and s key presses so that they will be handled by parent
  // widget; Otherwise the two keys won't response in parent widget
  if (e->key() == Qt::Key_Control || e->key() == Qt::Key_S) e->ignore();

  // Do nothing if no vector available
  if (vecList.size() == 0) return;

  // If the released key is not shift, ignore it
  if (e->key() != Qt::Key_Shift) return;
  // If shift key is disabled, ignore it
  if (!shiftEnabled) return;

  shiftPressed = false;
  if (xLengthInPixel[activeCurve] == 0) return;
  if (!chkMouseX()) return;

  // When shift key is released, change X and Y coordinates print out
  myX = mouseX;
  update();
}

/* This function deals with shift key press.
 * It changes X and Y coordinates printout. */
void PlotWidget::pressShift() {
  if (!shiftEnabled) return;

  shiftPressed = true;
  if (xLengthInPixel[activeCurve] == 0) return;

  if (!chkMouseX()) return;

  setMyX();
  update();
}

/* This function deals with space key press.
 * It activates the next curve and update the whole screen. */
void PlotWidget::pressSpace() {
  if (!spaceEnabled) return;
  if (vecList.size() < 2) return;

  int newIndex = (activeCurve + 1) % vecList.size();
  if (xLengthInPixel[newIndex] == 0) return;

  activeCurve = newIndex;
  update();
}

/* This function deals with up arrow key press.
 * It changes the starting and ending position of the curve.
 * It is equivalent to moving magnification slider bar to rigth side (zoom in)
 */
void PlotWidget::pressUp() {
  if (xMagnification == 10) return;

  xMagnification++;
  int newWidth = xMagnification * parentWidget()->width();
  resize(newWidth, height());
  update();

  emit(xMagChanged(xMagnification));
}

/* This function deals with down arrow key press.
 * It changes the starting and ending position of the curve.
 * It is equivalent to moving magnification slider bar to left side (zoom out)
 */
void PlotWidget::pressDown() {
  if (xMagnification == 1) return;

  xMagnification--;
  int newWidth = xMagnification * parentWidget()->width();
  resize(newWidth, height());
  update();

  emit(xMagChanged(xMagnification));
}

/* This function deals with "1" key press.
 * It changes the starting and ending position of the curve.
 * It is equivalent to clicking the center button to restore the default view */
void PlotWidget::press1key() { centerX(); }

/* This is a slot to restore the default X axis starting and ending positions */
void PlotWidget::centerX() {
  if (xMagnification == 1) return;

  xMagnification = 1;
  int newWidth = parentWidget()->width();
  resize(newWidth, height());
  update();

  emit(xMagChanged(1));
}

/* This is a slot to change x axis magnification */
void PlotWidget::setXMag(int inputVal) {
  xMagnification = inputVal;
  int newWidth = xMagnification * parentWidget()->width();
  resize(newWidth, height());
  update();
}

/* This function deals with F1-F4 key press.
 * It changes activated curve's plot mode and update the whole screen. */
void PlotWidget::pressFKey(QKeyEvent* e) {
  if (!FKeyEnabled) return;

  unsigned newMode = 0;
  if (e->key() == Qt::Key_F1)
    newMode = 1;
  else if (e->key() == Qt::Key_F2)
    newMode = 2;
  else if (e->key() == Qt::Key_F3)
    newMode = 3;
  else if (e->key() == Qt::Key_F4)
    newMode = 4;
  // If the key pressed is not F1-F4, ignore it
  if (newMode == 0) return;
  // If the new mode is same as original, ignore it
  if (newMode == plotModeList[activeCurve]) return;

  int orgMode = plotModeList[activeCurve];
  // Change X axis length if the mode is changed from 1 or 3 to 2 or 4
  if (orgMode % 2 != 0 && newMode % 2 == 0) {
    double orgLength = xMaxList[activeCurve] - xMinList[activeCurve];
    double vecLen = (double)(vecList[activeCurve].getLength() / ratio);
    double newLen = orgLength / (vecLen - 1) * vecLen;
    setNewVecXLength(activeCurve, newLen);
  }
  // Change X axis length if the mode is changed from 2 or 4 to 1 or 3
  else if (orgMode % 2 == 0 && newMode % 2 != 0) {
    double orgLength = xMaxList[activeCurve] - xMinList[activeCurve];
    double vecLen = (double)(vecList[activeCurve].getLength() / ratio);
    setNewVecXLength(activeCurve, orgLength / vecLen * (vecLen - 1));
  }
  plotModeList[activeCurve] = newMode;

  update();
}

/* This function prints out the current mouse position's coordinate */
void PlotWidget::setXY_13() {
  double x_ratio = (double)(mouseX - leftOffset) / (double)plotWidth;
  double x_graph = plotXMarkMin + x_ratio * plotXLength;
  vline_x = "X=" + QString::number(x_graph) + ", ";

  double startPix = xStartPost[activeCurve];
  double totalPix = xLengthInPixel[activeCurve];
  if (mouseX < startPix || myX > startPix + totalPix)
    vline_y = "Y=NA";
  else {
    int vecLen = vecList[activeCurve].getLength();
    ;
    double x_ratio = (double)(mouseX - startPix) / totalPix;
    double x_index = x_ratio * (vecLen - 1);
    int x1 = (int)x_index;
    double y_graph;
    if (x1 == vecLen - 1)
      y_graph = vecList[activeCurve].getElement(x1);
    else {
      double y1 = vecList[activeCurve].getElement(x1);
      double y2 = vecList[activeCurve].getElement(x1 + 1);
      y_graph = y1 + (y2 - y1) * (x_index - x1);
    }
    vline_y = "Y=" + QString::number(y_graph);
  }
}

/* This function prints out the current mouse position's X coordinate */
void PlotWidget::setXY_24() {
  double x_ratio = (double)(mouseX - leftOffset) / (double)plotWidth;
  double x_graph = plotXMarkMin + x_ratio * plotXLength;
  vline_x = "X=" + QString::number(x_graph) + ", ";

  double startPix = xStartPost[activeCurve];
  double totalPix = xLengthInPixel[activeCurve];
  if (mouseX < startPix || mouseX > startPix + totalPix)
    vline_y = "Y=NA";
  else {
    int vecLen = vecList[activeCurve].getLength();
    double x_ratio = ((double)mouseX - startPix) / totalPix;
    double x_index = x_ratio * vecLen;
    int x1 = (int)x_index;
    if (x1 > vecLen - 1) x1 = vecLen - 1;
    double y_graph = vecList[activeCurve].getElement(x1);
    vline_y = "Y=" + QString::number(y_graph);
  }
}

/* This function prints out X coordinate of the data point that is closest to
 * mouse position */
void PlotWidget::setXY_shift13() {
  double startPix = xStartPost[activeCurve];
  double totalPix = xLengthInPixel[activeCurve];
  if (mouseX < startPix || mouseX > startPix + totalPix)
    vline_x = "X=NA, ";
  else {
    int vecLen = vecList[activeCurve].getLength();
    double x_ratio = ((double)mouseX - startPix) / totalPix;
    double x_index = x_ratio * (vecLen - 1);
    double x1 = round(x_index);
    double ratio2 = x1 / (vecLen - 1);
    double dataStart = startPix + totalPix * ratio2;
    x_ratio = (dataStart - leftOffset) / plotWidth;
    double x_graph = plotXMarkMin + x_ratio * plotXLength;
    vline_x = "X=" + QString::number(x_graph) + ", ";
  }

  if (mouseX < startPix || mouseX > startPix + totalPix)
    vline_y = "Y=NA";
  else {
    int vecLen = vecList[activeCurve].getLength();
    double x_ratio = (double)(mouseX - startPix) / totalPix;
    double x_index = x_ratio * (vecLen - 1);
    int x_index_int = (int)round(x_index);
    double y_graph = vecList[activeCurve].getElement(x_index_int);
    vline_y = "Y=" + QString::number(y_graph);
  }
}

/* This function prints out X coordinate of the data point that is closest to
 * mouse position */
void PlotWidget::setXY_shift24() {
  double startPix = xStartPost[activeCurve];
  double totalPix = xLengthInPixel[activeCurve];
  if (mouseX < startPix || mouseX > startPix + totalPix)
    vline_x = "X=NA, ";
  else {
    int vecLen = vecList[activeCurve].getLength();
    double x_ratio = ((double)mouseX - startPix) / totalPix;
    double x_index = x_ratio * vecLen;
    double x1 = round(x_index);
    double ratio2 = x1 / vecLen;
    double dataStart = startPix + totalPix * ratio2;
    x_ratio = (dataStart - leftOffset) / plotWidth;
    double x_graph = plotXMarkMin + x_ratio * plotXLength;
    vline_x = "X=" + QString::number(x_graph) + ", ";
  }

  if (mouseX < startPix || mouseX > startPix + totalPix)
    vline_y = "Y=NA";
  else {
    int vecLen = vecList[activeCurve].getLength();
    double x_ratio = ((double)mouseX - startPix) / totalPix;
    double x_index = x_ratio * vecLen;
    int x1 = (int)round(x_index);
    if (x1 > vecLen - 1) x1 = vecLen - 1;
    double y_graph = vecList[activeCurve].getElement(x1);
    vline_y = "Y=" + QString::number(y_graph);
  }
}
