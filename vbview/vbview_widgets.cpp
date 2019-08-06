
// vbview_widgets.cpp
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

#include "myboxes.h"
#include "vbview_widgets.moc.h"

using namespace std;

VBQTnewmasklayer::VBQTnewmasklayer(QWidget *parent, int basex, int basey,
                                   int basez)
    : QDialog(parent) {
  init(basex, basey, basez);
}

VBQTnewmasklayer::VBQTnewmasklayer(QWidget *parent, const Cube &cb)
    : QDialog(parent) {
  init(cb.dimx, cb.dimy, cb.dimz);
}

void VBQTnewmasklayer::init(int basex, int basey, int basez) {
  // set<int> xs;
  // set<int> ys;
  // set<int> zs;

  // for (int i=1; i<5; i++) {
  //   if (!(basex%i)) xs.insert(basex/i);
  //   if (!(basey%i)) ys.insert(basey/i);
  //   if (!(basez%i)) zs.insert(basez/i);
  // }
  // vector<VBVoxel> dims;  // not really voxels, actually dims
  // vbforeach(int xx,xs) {
  //   vbforeach(int yy,ys) {
  //     vbforeach(int zz,zs) {
  //       dims.push_back(VBVoxel(xx,yy,zz));
  //       VBVoxel(xx,yy,zz).print();
  //     }
  //   }
  // }

  // IN THE LOOP WHEN WE ADD THE LISTWIDGETITEMS, MAP EACH ITEM
  // POINTER TO THE VBVOXEL

  int xyzwidth = fontMetrics().width("88888");

  // w_list=new QListWidget(this);
  dimbox = new QHBox(this);

  dimlabel = new QLabel("<b>Set Dimensions</b>", this);

  QLabel *tmpl;
  w_xv = new QLineEdit();
  w_xv->setFixedWidth(xyzwidth);
  w_xv->setText(strnum(basex).c_str());
  dimbox->addWidget(w_xv);
  tmpl = new QLabel("x");
  dimbox->addWidget(tmpl);
  w_yv = new QLineEdit();
  w_yv->setFixedWidth(xyzwidth);
  w_yv->setText(strnum(basey).c_str());
  dimbox->addWidget(w_yv);
  tmpl = new QLabel("x");
  dimbox->addWidget(tmpl);
  w_zv = new QLineEdit();
  w_zv->setFixedWidth(xyzwidth);
  w_zv->setText(strnum(basez).c_str());
  dimbox->addWidget(w_zv);

  acceptbutton = new QPushButton("Accept", this);
  cancelbutton = new QPushButton("Cancel", this);
  QObject::connect(acceptbutton, SIGNAL(clicked()), this,
                   SLOT(acceptclicked()));
  QObject::connect(cancelbutton, SIGNAL(clicked()), this,
                   SLOT(cancelclicked()));

  arrangeChildren();
}

void VBQTnewmasklayer::acceptclicked() {
  pair<bool, int32> val;
  val = strtolx(w_xv->text().toStdString());
  if (!val.first)
    retx = val.second;
  else
    return;
  val = strtolx(w_yv->text().toStdString());
  if (!val.first)
    rety = val.second;
  else
    return;
  val = strtolx(w_zv->text().toStdString());
  if (!val.first)
    retz = val.second;
  else
    return;
  accept();
}

void VBQTnewmasklayer::cancelclicked() { reject(); }

void VBQTnewmasklayer::resizeEvent(QResizeEvent *re) {
  QDialog::resizeEvent(re);
  arrangeChildren();
}

void VBQTnewmasklayer::arrangeChildren() {
  // get natural sizes for certain widgets
  // w_list->adjustSize();
  dimlabel->adjustSize();
  dimbox->adjustSize();
  acceptbutton->adjustSize();
  cancelbutton->adjustSize();

  // listw, dimlabel, dimbox
  // okay, cancel
  // w_list->setGeometry(10,10,w_list->width(),w_list->height());
  // int lright=w_list->width()+10;
  // int lbottom=w_list->height()+10;
  int hpos = 10;
  int vpos = 10;
  dimlabel->setGeometry(hpos, vpos, width() - 20, dimlabel->height());
  vpos += 10 + dimlabel->height();
  dimbox->setGeometry(hpos, vpos, dimbox->width(), dimbox->height());
  vpos += 10 + dimbox->height();
  acceptbutton->setGeometry(hpos, vpos, acceptbutton->width(),
                            acceptbutton->height());
  hpos += 10 + acceptbutton->width();
  cancelbutton->setGeometry(hpos, vpos, cancelbutton->width(),
                            cancelbutton->height());
}
