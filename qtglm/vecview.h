
// vecview.h
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

#ifndef VECVIEW_H
#define VECVIEW_H

#include <q3filedialog.h>
#include <q3mainwindow.h>
#include <q3vbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qslider.h>
#include "plotscreen.h"
// Added by qt3to4:
#include <QKeyEvent>

class VecView : public Q3MainWindow {
  Q_OBJECT

 public:
  VecView(const char *inputFile, int mode, QWidget *parent = 0,
          const char *name = 0);
  VecView(VB_Vector *inputVec, bool mvpmFlag, QWidget *parent = 0,
          const char *name = 0);
  VecView(tokenlist refList, QWidget *parent = 0, const char *name = 0);
  ~VecView();
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);

 private slots:
  void changeVec(int);
  void changeMP(int);
  void toggleShow(bool);
  void toggleScale(bool);
  void togglePS(bool);

 private:
  void init();
  void initColor();
  void vecShow(VB_Vector *);
  void mvpmShow(VB_Vector *);
  void matShow();
  void setCommon();
  void setVecList(string);
  void setVecBox();
  void plotMP();

  Q3VBox *mainBox;
  PlotScreen *mainScreen;
  QSlider *magSlider;
  QSlider *vecSlider;
  QCheckBox *showAll;
  QCheckBox *autoScale;
  int plotStat;
  bool psChecked;
  tokenlist refFiles;
  tokenlist colorList;
  std::vector<VB_Vector> vecList;
  std::vector<VB_Vector> psList;
  bool ctrlPressed;
};

int checkInFile(char *fileName, bool mvpmFlag);
void vecview_help();
void vecview_version();

#endif
