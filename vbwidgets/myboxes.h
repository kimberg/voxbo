
// myboxes.h
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

#ifndef MYBOXES_H
#define MYBOXES_H

#include <QLabel>
#include <QLayout>
#include <QSpacerItem>

class QHBox : public QFrame {
  Q_OBJECT
 public:
  QHBox(QWidget *parent = 0);
  // simple aliases to layout functions
  void addLayout(QLayout *l, int s = 0) { layout->addLayout(l, s); }
  void addSpacing(int s) { layout->addSpacing(s); }
  void addStretch(int s = 0) { layout->addStretch(s); }
  void addWidget(QWidget *w, int s = 0, Qt::Alignment a = 0) {
    layout->addWidget(w, s, a);
  }
  void setSpacing(int s) { layout->setSpacing(s); }
  // utility functions
  void addLabel(const char *text);
  // data
  QHBoxLayout *layout;
};

class QVBox : public QFrame {
  Q_OBJECT;

 public:
  QVBox(QWidget *parent = 0);
  // simple aliases to layout functions
  void addLayout(QLayout *l, int s = 0) { layout->addLayout(l, s); }
  void addSpacing(int s) { layout->addSpacing(s); }
  void addStretch(int s = 0) { layout->addStretch(s); }
  void addWidget(QWidget *w, int s = 0, Qt::Alignment a = 0) {
    layout->addWidget(w, s, a);
  }
  void setSpacing(int s) { layout->setSpacing(s); }
  // utility functions
  void addLabel(const char *text);
  // data
  QVBoxLayout *layout;
};

#endif  // MYBOXES_H
