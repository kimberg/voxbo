
// vbview_widgets.h
// Copyright (c) 2011 by The VoxBo Development Team

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

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include "myboxes.h"
#include "vbio.h"

using namespace std;

class VBQTnewmasklayer : public QDialog {
  Q_OBJECT
 public:
  VBQTnewmasklayer(QWidget *parent, int, int, int);
  VBQTnewmasklayer(QWidget *parent, const Cube &cb);
  void init(int, int, int);
  void resizeEvent(QResizeEvent *re);
  int retx, rety, retz;
 public slots:
 private slots:
  void acceptclicked();
  void cancelclicked();

 private:
  VBVoxel selectedresolution;
  map<QListWidgetItem *, VBVoxel> itemmap;
  QLabel *dimlabel;
  QHBox *dimbox;
  QLineEdit *w_xv, *w_yv, *w_zv;
  QLineEdit *w_xmm, *w_ymm, *w_zmm;
  QListWidget *w_list;
  QPushButton *acceptbutton, *cancelbutton;

  void arrangeChildren();
 signals:
};
