
// plotscreen.h
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

#ifndef PLOTSCREEN_H
#define PLOTSCREEN_H

#include "vbio.h"
#include <vector>

#include <QWheelEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QFrame>

class QColor;
class QLabel;
class QString;
class PlotWidget;

class PlotScreen : public QScrollArea 
{
  Q_OBJECT

public:
  PlotScreen(QWidget *parent = 0);
  ~PlotScreen();

  void setMinimumSize(unsigned newWidth, unsigned newHeight );
  void setWindowSize(unsigned width, unsigned height);
  void setWindowWidth(unsigned width);
  void setWindowHeight(unsigned height);
  void setPlotSize(unsigned width, unsigned height);
  void setPlotWidth(unsigned );
  void setPlotHeight(unsigned );
  void setFixedSize(unsigned outWidth, unsigned outHeight, unsigned inWidth, unsigned inHeight);
  void setFrameWidth(unsigned width);
  void setCurveLineWidth(unsigned lineWidth = 0);

  void setBkgdColor(QColor &);
  void setEdgeColor(QColor &);
  void setAxisColor(QColor &);
  void setCaptionColor(QColor &);

  void setXCaption(QString);
  void setXCaptionPost(unsigned x, unsigned y);
  void setYCaption(QString);
  void setYCaptionPost(unsigned x, unsigned y);

  void enableFixedY(bool);
  bool getFixedYFlag();
  void setFixedY(double start, double end);

  /* New methods to draw multiple curves on one screen */
  int addVector(const VB_Vector*, QColor inputColor = "green");
  int addVector(const VB_Vector&, QColor inputColor = "green");
  int addVector(const VB_Vector*, double xMarkMin, double xLength, QColor plotColor, unsigned mode);
  int addVector(const VB_Vector&, double xMarkMin, double xLength, QColor plotColor, unsigned mode);
  int addVecFile(const char* filename, QColor inputColor = "green");
  int addVecFile(const char* filename, double xMarkMin, double xLength, QColor plotColor, unsigned mode);

  void delVector(unsigned );
  void resetActiveCurve(unsigned );
  void setNewVecXMin(unsigned vecIndex, double xMin);
  void setNewVecXLength(unsigned vecIndex, double xLength);
  void setNewVecX(unsigned vecIndex, double xMarkMin, double xLength);
  void setAllNewX(double xMarkMin, double xLength);

  void setPlotColor(unsigned vecIndex, QColor);
  void setPlotMode(unsigned plotMode);
  void setPlotMode(unsigned vecIndex, unsigned plotMode);

  void clear();
  void update();
  unsigned getVecNum();
  VB_Vector getInputVector(unsigned vecIndex);
  double getFullXLength();
  double getXRange(double);
  double getYRange(double);

  /* The functions below are specilly written to manipulate the first vector. They are
   * mainly used for Gdw interface, where only one covariate shows up on the screen. */
  void setFirstVector(VB_Vector *);
  void setFirstXMarkMin(double);
  void setFirstXLength(double);
  void setRatio(unsigned);
  double getFirstXLength();
  double getFirstXMin();
  int getFirstPlotMode();

  // mouse and keyboard events
  void setVLineColor(QColor );
  void setMouseEnabled(bool );
  void setShiftEnabled(bool );
  void setSpaceEnabled(bool );
  void setFKeyEnabled(bool );

public slots:
  void passMagSignal(int);
  void setVisibleRange(int);  
  void setXMag(int);
  void centerX();

signals:
  void xMagChanged(int );

private:
  void init();
  void resizeEvent(QResizeEvent*);

  PlotWidget* pw;
};


class PlotWidget : public QFrame 
{
  Q_OBJECT

  friend class PlotScreen;

public:
  PlotWidget(QWidget *parent = 0);
  ~PlotWidget();

private:
  // member functions
  void init();
  void clear();
  virtual void paintEvent(QPaintEvent*);

  void resizeEvent(QResizeEvent*);
  void resizePlot();
  void setPlotSize(unsigned, unsigned);
  void setPlotWidth(unsigned );
  void setPlotHeight(unsigned );
  void setFixedSize(unsigned outWidth, unsigned outHeight, unsigned inWidth, unsigned inHeight);
  void updateSize();

  int addVector(const VB_Vector*, QColor inputColor = "green");
  int addVector(const VB_Vector&, QColor inputColor = "green");
  int addVector(const VB_Vector*, double xMarkMin, double xLength, QColor plotColor, unsigned mode);
  int addVector(const VB_Vector&, double xMarkMin, double xLength, QColor plotColor, unsigned mode);
  int addVecFile(const char* filename, QColor inputColor = "green");
  int addVecFile(const char* filename, double xMarkMin, double xLength, QColor plotColor, unsigned mode);

  void delVector(unsigned );
  void resetActiveCurve(unsigned );
  void setNewVecXMin(unsigned vecIndex, double xMin);
  void setNewVecXLength(unsigned vecIndex, double xLength);
  void setNewVecX(unsigned vecIndex, double xMarkMin, double xLength);
  void setAllNewX(double xMarkMin, double xLength);

  void setFirstVector(VB_Vector *);
  void setFirstXMarkMin(double);
  void setFirstXLength(double);

  void calcXMark();   
  void calcXIndex(unsigned vecIndex);
  void calcYMark();
  void drawXAxis(QPainter&);
  void drawYAxis(QPainter&);
  void drawGraph(QPainter&);
  void drawInMode1(QPainter&, unsigned);
  void drawInMode2(QPainter&, unsigned);
  void drawInMode3(QPainter&, unsigned);
  void drawInMode4(QPainter&, unsigned);
  void drawRotatedText(QPainter&, float, int, int, const QString &); 

  void enableFixedY(bool);
  void setFixedY(double, double);
  double getXRange(double);
  double getYRange(double);
  int getIncrement(double);

  double getMin(vector< double >);
  double getMax(vector< double >);
  double calcXEdge(double yStart, double yEdge, double yEnd, double xTotal); 
  int checkVal(double);

  void mousePressEvent(QMouseEvent*);   // mouse press
  void mouseReleaseEvent(QMouseEvent*); // mouse release
  void mouseMoveEvent(QMouseEvent*);    // mouse movement
  void wheelEvent (QWheelEvent*);       // mouse wheel movement
  void keyPressEvent(QKeyEvent*);       // key pressed on keyboard
  void keyReleaseEvent(QKeyEvent*);     // key released on keyboard
  void pressShift();
  void pressSpace();
  void pressUp();
  void pressDown();
  void press1key();
  void centerX();
  void setXMag(int);
  void pressFKey(QKeyEvent *e);

  bool chkMouseX();
  void setMyX();
  void addVLine(QPainter&);
  void addVLineTxt(QPainter&);

  void setXY_13();
  void setXY_24();
  void setXY_shift13();
  void setXY_shift24();

signals:
  void xMagChanged(int );

private:
  // member variables
  QColor bkgdColor, edgeColor;
  QColor axisColor, captionColor;
  VB_Vector *plotVector;        
  QString xCaption, yCaption;

  unsigned windowWidth, windowHeight, frameWidth; // outside window size and the frame width                   
  unsigned plotWidth, plotHeight;                 // size of the plot window
  unsigned leftOffset, upOffset;                  // coordinates of the plot's upper left coner
  unsigned xCaptionPostX, xCaptionPostY;          // X axis caption's coordinates 
  unsigned yCaptionPostX, yCaptionPostY;          // Y axis caption's coordinates

  double yBaseValue;                    // Minimum value marked on Y axis
  double yMaxValue;                     // Maximum value marked on Y axis
  double yDivision;                     // magnitude of each division on Y axis
  double yDivisionInPixel;              // number of pixels each y division corresponds to  
  bool zoomYFlag;                       // flag to show Y axis selectively
  double zoomYStart, zoomYEnd;          // customized Y scale starting and ending points

  std::vector< VB_Vector > vecList;
  std::vector< double > xMinList;       // vector to hold each curve's minimum on X axis
  std::vector< double > xMaxList;       // vector to hold each curve's maximum on X axis
  std::vector< double > yMinList;       // vector to hold each curve's minimum on Y axis
  std::vector< double > yMaxList;       // vector to hold each curve's maximum on Y axis
  std::vector< QColor > colorList;      // vector to hold each curve's plot colors
  std::vector< unsigned > plotModeList; // vector to hold each vector's plot mode (1, 2, 3 or 4)
  std::vector< double > xStartPost;
  std::vector< double > xLengthInPixel;

  double xInPixel;         // number of pixels between two nearby elements in the vector on X axis
  double plotXMarkMin;     // minimum X axis mark value (assigned for zoom in/out functionality)
  double plotXLength;      // length shown on X axis, not the number of points in input VB_Vector
  double plotYMarkMin;     // minimum mark value on Y axis
  double plotYLength;      // length shown on Y Axis 
  int plotMode;            // mode to draw graph: 1 indicates drawGraphMode1(), 2 drawGraphMode2(), etc
  int curveLineWidth;      // width of the pen line used to draw the curve 

  unsigned myX;            // myX is X coordinate where vertical line is drawn
  int mouseX, mouseY;      // mouseX is X coordinate of current mouse position 
  QColor vLineColor;
  int visible_start;
  QString vline_x, vline_y;

  bool mouseEnabled;
  bool shiftEnabled;
  bool spaceEnabled;
  bool FKeyEnabled;
  bool shiftPressed;
  unsigned activeCurve;
  unsigned ratio;

  int xMagnification;  // X axis zoom-in level
};


#endif
