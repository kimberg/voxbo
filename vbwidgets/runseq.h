
// runseq.h
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
// original version written by Dan Kimberg

using namespace std;

#ifndef RUNSEQ_H
#define RUNSEQ_H

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include "vbprefs.h"
#include "vbjobspec.h"
#include "vbutil.h"

class QRunSeq : public QDialog {
  Q_OBJECT
public:
  QRunSeq(QWidget *parent=0);
  QRunSeq(VBPrefs &vbp,VBSequence &seq,uint32 njobs);
  int Go(VBPrefs &vbp,VBSequence &seq,uint32 njobs);
private slots:
  void handleQuit();
  void handlePause();
  void handleTimer();
private:
  bool f_quit;
  int donecnt;
  map<pid_t,VBJobSpec> pmap;
  uint32 njobs;
  // widgets
  QTextEdit *mytext;
  QTimer *mytimer;
  QPushButton *b_quit;
  QPushButton *b_pause;
  QProgressBar *pbar;
  VBPrefs vbp;
  VBSequence seq;
};

class QDisp : public QDialog {
  Q_OBJECT
public:
  QDisp(QWidget *parent=0);
  string disp;
  QLineEdit *mylabel;
private slots:
  void handleStop() {disp="stop"; accept();}
  void handleSkip() {disp="skip"; accept();}
  void handleEdit() {disp="edit"; accept();}
  void handleRetry() {disp="retry"; accept();}
private:
};

#endif // RUNSEQ_H
