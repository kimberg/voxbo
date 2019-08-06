
// myboxes.cpp
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

#include "myboxes.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

QHBox::QHBox(QWidget *parent) : QFrame(parent) {
  setFrameStyle(QFrame::NoFrame);
  layout = new QHBoxLayout;
  setLayout(layout);
  layout->setAlignment(Qt::AlignLeft);
  layout->setContentsMargins(0, 0, 0, 0);
}

QVBox::QVBox(QWidget *parent) : QFrame(parent) {
  setFrameStyle(QFrame::NoFrame);
  layout = new QVBoxLayout;
  setLayout(layout);
  layout->setAlignment(Qt::AlignTop);
  layout->setContentsMargins(0, 0, 0, 0);
}

void QHBox::addLabel(const char *text) {
  QLabel *tmp = new QLabel(text);
  addWidget(tmp);
}

void QVBox::addLabel(const char *text) {
  QLabel *tmp = new QLabel(text);
  addWidget(tmp);
}
