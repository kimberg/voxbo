
// threshcalc.h
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

#ifndef THRESHCALC_H
#define THRESHCALC_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "myboxes.h"
#include "statthreshold.h"

class tcalc : public QDialog {
  Q_OBJECT
public:
  tcalc(threshold v_in,QWidget* parent=0,Qt::WFlags fl=0);
  QLabel* lblVoxelNumber;
  QLabel* lblVoxelSizes;
  QLabel* lblFWHM;
  QLabel* lblEffDf;
  QLabel* lbldenomdf;
  QLabel* lblalpha;
  QLabel* lblCriticalTValue;
  QLabel* lblres;
  QLineEdit* txtX;
  QLineEdit* txtZ;
  QLineEdit* txtY;
  QLineEdit* txtFWHM;
  QLineEdit* txtEffdf;
  QLineEdit* txtDenomDf;
  QLineEdit* txtAlpha;
  QPushButton* pbUpdate;
  QLineEdit* txtVoxelNumber;
  QHBox *buttonbox;
  double getbfthresh();
  double getrftthresh();
  double getbestthresh();
  void showbuttons();
  threshold getthreshdata();
protected:
protected slots:
  virtual void languageChange();
  void update();
private:
  void closeEvent( QCloseEvent *ce );
  threshold v;
  double bfthresh;
  double rftthresh;
};

#endif // THRESHCALC_H
