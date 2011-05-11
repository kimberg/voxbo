
// permgen.h
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
// original version written by Tom King

#ifndef PERMGENERATOR_H
#define PERMGENERATOR_H

#include <qvariant.h>
#include <qdialog.h>
#include <q3filedialog.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QLabel>
#include <Q3Frame>
#include <QCloseEvent>
#include "../stand_alone/perm.h"
#include "glmutil.h"
#include "vbprefs.h"
#include "vbutil.h"
#include "vbio.h"
#include "vbjobspec.h"

class QCheckBox;
class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QLabel;
class QPushButton;
class Q3TextEdit;
class QComboBox;
class QLineEdit;
class QSpinBox;
class Q3Frame;

class permGenerator : public QDialog
{
    Q_OBJECT

public:
  permGenerator( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
  
  QLabel* lblY;
  QLabel* lblX;
  QLineEdit* txtPRM;
  QLabel* lblPermDir;
  QLineEdit* txtPermDir;
  QLabel* lblContrasts;
  QPushButton *btnContrasts;
  QLineEdit* txtContrasts;
  // QLabel* lblScale;
  QLabel* lblPermNumber;
  QLineEdit* txtPermNumber;
  QLabel* lblPseudoT;
  QLineEdit* txtX;
  QLineEdit* txtY;
  QLineEdit* txtZ;
  QLabel* lblZ;
  QLabel* lblPermType;
  QComboBox* cbPermType;
  QPushButton* pbGO;
  // QComboBox* cbScale;
  QPushButton* pbDir;
  QComboBox* c;
  QPushButton* pbCancel;
  QSpinBox* sbPriority;
  QLabel* lblPriority; 
  QLabel *lblSN;
  QLineEdit *txtSN;
  Q3Frame *line2;
protected:
protected slots:
  virtual void languageChange();
  void pressed();
  QString getPRMFile();
  void getContrast();
  void exitProgram();
  void mod(bool x);
  void CalculateNumPerms();
  int warnCalculateNumPerms();
private:
  void closeEvent( QCloseEvent *ce );
};

#endif // PERMGENERATOR_H
