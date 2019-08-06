
// vbqt_glmselect.cpp
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

#include <QBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include "glmutil.h"
#include "myboxes.h"
#include "vbqt_glmselect.moc.h"
#include "vbutil.h"

VBQT_GLMSelect::VBQT_GLMSelect(QWidget *parent, const char *name, bool modal)
    : QDialog(parent, name, modal) {
  QPushButton *button;
  QHBox *hb;
  QLabel *lab;

  setMinimumWidth(280);
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(4);
  layout->setMargin(2);

  hb = new QHBox(this);

  button = new QPushButton("Parameter file:");
  hb->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(getnewstem()));
  stemlabel = new QLineEdit("<none selected>");
  hb->addWidget(stemlabel);

  layout->addWidget(hb);

  // the scale for the stat map
  lab = new QLabel("Parameter scaling:", this);
  layout->addWidget(lab);

  //   scalebox=new QListBox(this);
  //   scalebox->insertItem("beta (unscaled)");
  //   scalebox->insertItem("t/1 (scaled by error, one-tailed)");
  //   scalebox->insertItem("t/2 (scaled by error, two-tailed)");
  //   scalebox->insertItem("F (F-test)");
  //   scalebox->insertItem("error (just the error)");
  //   scalebox->insertItem("intercept (percent change)");
  //   scalebox->insertItem("tp/1 (p map from one-tailed t-test)");
  //   scalebox->insertItem("tp/2 (p map from two-tailed t-test)");
  //   scalebox->insertItem("Fp (p map from F-test)");
  //   scalebox->insertItem("tZ/1 (Z-scores from one-tailed t-test)");
  //   scalebox->insertItem("tZ/2 (Z-scores from two-tailed t-test)");
  //   scalebox->insertItem("FZ (Z-scores from F-test)");
  //   QObject::connect(scalebox,SIGNAL(selectionChanged()),this,SLOT(newscale()));
  //   layout->addWidget(scalebox);

  contrastbox = new QListWidget(this);
  QObject::connect(contrastbox, SIGNAL(selectionChanged()), this,
                   SLOT(newcontrast()));
  layout->addWidget(contrastbox);

  hb = new QHBox(this);
  layout->addWidget(hb);

  // the widget for the contrast
  lab = new QLabel("Contrast spec:", hb);
  // layout->addWidget(lab);

  contrastline = new QLineEdit("", hb);
  contrastline->setMinimumWidth(180);
  QObject::connect(contrastline, SIGNAL(textChanged(const QString &)), this,
                   SLOT(newcontrastline(const QString &)));
  contrastline->setDisabled(1);
  // layout->addWidget(contrastline);

  button = new QPushButton("Cancel", this);
  layout->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(reject()));

  button = new QPushButton("Done", this);
  layout->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(accept()));
}

void VBQT_GLMSelect::newcontrast() {
  int item = contrastbox->currentRow();
  if (item == (int)contrastbox->count() - 1)  // "custom"
    contrastline->setDisabled(0);
  else {
    contrastline->setDisabled(1);
    if (glmi.contrasts.size()) {
      glmi.contrast = glmi.contrasts[item];
    }
  }
}

void VBQT_GLMSelect::newcontrastline(const QString &text) {
  tokenlist foo;
  foo.ParseLine(text.latin1());
  glmi.contrast.parsemacro(foo, glmi.nvars, glmi.keeperlist);
}

QString VBQT_GLMSelect::getcontrast() {
  if (contrastbox->currentItem()->text() != "custom")
    return contrastbox->currentItem()->text();
  return contrastline->text();
}

void VBQT_GLMSelect::getnewstem() {
  QString s;
  s = QFileDialog::getOpenFileName(".", "Parameter Files (*.prm)", this, "xxx",
                                   "Choose a parameter file");
  if (s == QString::null) return;
  string dd = s.latin1();
  if (dd.substr(dd.size() - 4, 4) == ".prm") dd.erase(dd.size() - 4, 4);
  stemlabel->setText(dd.c_str());
  glmi.setup(dd);
  VBContrast foo = glmi.contrasts[0];
  glmi.contrast = glmi.contrasts[0];
  contrastbox->clear();
  for (int i = 0; i < (int)glmi.contrasts.size(); i++)
    contrastbox->insertItem(-1, glmi.contrasts[i].name.c_str());
  contrastbox->insertItem(-1, "custom");
  contrastbox->setCurrentRow(0);
}
