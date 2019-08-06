
// vbqt_masker.cpp
// Copyright (c) 2006-2010 by The VoxBo Development Team

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

#include <QColorDialog>
#include <QCursor>
#include <QFrame>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSpinBox>

#include "myboxes.h"
#include "vbqt_masker.moc.h"
#include "vbutil.h"

QTMaskView::QTMaskView() : QWidget() {
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // vertical layout
  QVBoxLayout *layout = new QVBoxLayout;
  setLayout(layout);
  // list of masks in a scrollarea containing a widget containing a layout
  area = new QScrollArea;
  area->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  // area->setWidgetResizable(1);
  layout->addWidget(area);
  area->setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::MinimumExpanding);

  mwidget = new QWidget();
  area->setWidget(mwidget);
  mlayout = new QVBoxLayout();
  mwidget->setLayout(mlayout);
  mlayout->setSpacing(0);
  mlayout->setMargin(2);
  mwidget->show();
  QPushButton *but;

  QHBox *tmph = new QHBox;
  tmph->addLabel("radius:");
  radiusbox = new QSpinBox;
  radiusbox->setRange(0, 30);
  radiusbox->setSingleStep(1);
  radiusbox->setValue(0);
  tmph->addWidget(radiusbox);
  zradiusbox = new QCheckBox("use radius in z");
  tmph->addWidget(zradiusbox);
  layout->addWidget(tmph);
  QObject::connect(radiusbox, SIGNAL(valueChanged(int)), this,
                   SIGNAL(newradius(int)));
  QObject::connect(zradiusbox, SIGNAL(stateChanged(int)), this,
                   SIGNAL(newzradius(int)));

  but = new QPushButton("Save Mask");
  layout->addWidget(but);
  QObject::connect(but, SIGNAL(clicked()), this, SIGNAL(savemask()));

  but = new QPushButton("New Mask Color");
  layout->addWidget(but);
  QObject::connect(but, SIGNAL(clicked()), this, SLOT(newmask()));

  onpalette.setColor(QPalette::Window, QColor(255, 230, 0));
  offpalette.setColor(QPalette::Window, palette().color(QPalette::Window));

  changemapper = new QSignalMapper;
  selectmapper = new QSignalMapper;
  infomapper = new QSignalMapper;
  copymapper = new QSignalMapper;
  // handle signal mappers that aggregate change/select events
  QObject::connect(changemapper, SIGNAL(mapped(QWidget *)), this,
                   SIGNAL(changed(QWidget *)));
  QObject::connect(selectmapper, SIGNAL(mapped(int)), this,
                   SLOT(handleselect(int)));
  QObject::connect(infomapper, SIGNAL(mapped(int)), this,
                   SIGNAL(inforequested(int)));
  QObject::connect(copymapper, SIGNAL(mapped(int)), this, SIGNAL(copied(int)));
}

void QTMaskView::clear() {
  // remove all the widgets from the interface and mappers
  vbforeach(QWidget * w, maskwidgets) {
    mlayout->remove(w);
    changemapper->removeMappings(w);
    selectmapper->removeMappings(w);
    infomapper->removeMappings(w);
    copymapper->removeMappings(w);
    delete w;
  }
  maskwidgets.clear();
}

void QTMaskView::newmask() {
  set<uint32> taken;
  uint32 newid = 0;
  vbforeach(QTMaskWidget * mm, maskwidgets) taken.insert(mm->index);
  for (size_t i = 1; i < maskwidgets.size() + 2; i++) {
    if (!taken.count(i)) {
      newid = i;
      break;
    }
  }
  QTMaskWidget *mw = addMask(newid, "newmask", QColor(100, 160, 100));
  emit newmask(mw);
  handleselect(newid);
}

void QTMaskView::keyPressEvent(QKeyEvent *ke) {
  if (!(ke->key() == Qt::Key_C && (ke->state() & Qt::CTRL))) return;
  emit copied(lastselected);
}

QTMaskWidget *QTMaskView::addMask(uint32 id, string name, QColor color) {
  QTMaskWidget *mw = new QTMaskWidget(name);
  mw->setcolor(color);
  mw->setlabel(name);
  mw->setindex(id);
  mlayout->addWidget(mw);
  mw->show();
  mwidget->adjustSize();
  maskwidgets.insert(mw);
  if (maskwidgets.size() == 1) handleselect(id);
  selectmapper->setMapping(mw, id);
  changemapper->setMapping(mw, mw);
  infomapper->setMapping(mw, id);
  copymapper->setMapping(mw, id);
  QObject::connect(mw, SIGNAL(selected()), selectmapper, SLOT(map()));
  QObject::connect(mw, SIGNAL(changed()), changemapper, SLOT(map()));
  QObject::connect(mw, SIGNAL(inforequested()), infomapper, SLOT(map()));
  QObject::connect(mw, SIGNAL(copied()), copymapper, SLOT(map()));
  return mw;
}

void QTMaskView::handleselect(int i) {
  lastselected = i;
  vbforeach(QTMaskWidget * m, maskwidgets) {
    if (m->f_selected && m->index != (uint32)i) {
      m->f_selected = 0;
      m->setPalette(offpalette);
    } else if (!m->f_selected && m->index == (uint32)i) {
      m->f_selected = 1;
      m->setPalette(onpalette);
      area->ensureWidgetVisible(m);
    }
  }
  emit selected(i);
}

QTMaskWidget::QTMaskWidget(string maskname) : QFrame() {
  QHBoxLayout *layout = new QHBoxLayout;
  setLayout(layout);
  layout->setSpacing(2);
  layout->setMargin(2);

  // setFixedHeight(24);
  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(1);
  setAutoFillBackground(1);

  char tmp[128];
  f_visible = 1;
  f_selected = 0;
  index = 999;
  color.setRgb(255, 100, 100);
  label = maskname;

  // the widget for the label
  edit_label = new QLineEdit(maskname.c_str(), this);
  edit_label->setFrameStyle(QFrame::Box | QFrame::Plain);
  edit_label->setLineWidth(1);
  layout->addWidget(edit_label);
  QObject::connect(edit_label, SIGNAL(textChanged(const QString &)), this,
                   SLOT(newlabel(const QString &)));

  // the widget for the index
  sprintf(tmp, "%d", index);
  edit_index = new QLineEdit(tmp, this);
  edit_index->setFixedWidth(28);
  edit_index->setFrameStyle(QFrame::Box | QFrame::Plain);
  edit_index->setLineWidth(1);
  layout->addWidget(edit_index);
  QObject::connect(edit_index, SIGNAL(textChanged(const QString &)), this,
                   SLOT(newindex(const QString &)));

  // on/off checkbox
  checkbox = new QCheckBox("", this);
  checkbox->setChecked(1);
  layout->addWidget(checkbox);

  cframe = new VBQT_colorbox();
  layout->addWidget(cframe);
  QObject::connect(cframe, SIGNAL(selected()), this, SIGNAL(selected()));
  QObject::connect(cframe, SIGNAL(inforequested()), this,
                   SIGNAL(inforequested()));
  QObject::connect(cframe, SIGNAL(copied()), this, SIGNAL(copied()));

  // signals for the overall widget
  // QObject::connect(cframe,SIGNAL(rightclicked()),this,SLOT(contextmenu()));
  // QObject::connect(cframe,SIGNAL(clicked()),this,SLOT(clicked()));
  QObject::connect(cframe, SIGNAL(colorchanged(QColor)), this,
                   SLOT(newcolor(QColor)));
  QObject::connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(ctoggled(bool)));
}

void QTMaskWidget::newlabel(const QString &text) {
  label = text.latin1();
  emit changed();
}

void QTMaskWidget::newindex(const QString &text) {
  index = strtol(text.latin1());
  emit changed();
}

void QTMaskWidget::newcolor(QColor newcolor) {
  color = newcolor;
  emit changed();  // so that we can update view when mask changes color
}

void QTMaskWidget::Toggle() { checkbox->toggle(); }

void QTMaskWidget::ctoggled(bool) {
  f_visible = !f_visible;
  emit changed();
}

void QTMaskWidget::SetType(char c) {
  type = c;
  // cframe->type=c;
};

void QTMaskWidget::contextmenu() {
  //   QMenu foo(this);
  //   foo.addAction("set color",this,SLOT(setcolor()));
  //   foo.addAction("toggle",this,SLOT(Toggle()));
  //   foo.addAction("info",this,SLOT(inforequested()));
  //   foo.popup(QCursor::pos());
}

void QTMaskWidget::setindex(int ind) {
  index = ind;
  char tmp[128];
  sprintf(tmp, "%d", index);
  edit_index->setText(tmp);
}

void QTMaskWidget::setlabel(string str) { edit_label->setText(str.c_str()); }

void QTMaskWidget::setcolor(QColor newcolor) {
  cframe->setcolor(newcolor);
  color = newcolor;
}

void QTMaskWidget::GetInfo(QColor &col, int &ind, string &lab) {
  col = color;
  ind = index;
  lab = label;
}

void QTMaskWidget::paintEvent(QPaintEvent *) {
  //   // update();
  //   return;
  //   QPainter paint(drawarea);
  //   paint.fillRect(0,0,20,20,color);
  //   if (f_selected) {
  //     QPainter foo(this);
  //     // QColor mycolor(200,200,0);  // GOLD
  //     // QColor mycolor(150,250,50);  // GREENISH
  //     QColor mycolor(255,230,0);  // ORANGEISH
  //     foo.fillRect(0,0,500,500,mycolor);
  //   }
  // drawarea->update();
  // cframe->update();
  // edit_label->update();
  // edit_index->update();
}

VBQT_colorbox::VBQT_colorbox() : QFrame() {
  setMouseTracking(0);
  setFixedSize(22, 22);
  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(1);
  setAutoFillBackground(1);
  palette.setColor(QPalette::Window, QColor(100, 160, 100));
  setPalette(palette);
}

void VBQT_colorbox::xpaintEvent(QPaintEvent *) {
  return;
  // QPainter paint(this);
  // paint.fillRect(0,0,20,20,color);
  if (type == 'M')
    bitBlt(this, 0, 0, pm_m, 0, 0, 20, 20);
  else if (type == 'R')
    bitBlt(this, 0, 0, pm_r, 0, 0, 20, 20);
  //   else if (type=="A")
  //     bitBlt(this,0,0,pm_a,0,0,20,20);
  //   else if (type=="L")
  //     bitBlt(this,0,0,pm_a,0,0,20,20);
}

void VBQT_colorbox::mouseDoubleClickEvent(QMouseEvent *) {
  // double-click to set color is too easy to do by accident
  getnewcolor();
}

void VBQT_colorbox::mousePressEvent(QMouseEvent *) { emit selected(); }

void VBQT_colorbox::getnewcolor() {
  QColor tmp = QColorDialog::getColor(color);
  if (tmp.isValid()) {
    setcolor(tmp);
    // update();
    emit colorchanged(tmp);
  }
}

void VBQT_colorbox::setcolor(QColor newcolor) {
  color = newcolor;
  palette.setColor(QPalette::Window, color);
  setPalette(palette);
}

void VBQT_colorbox::contextMenuEvent(QContextMenuEvent *) {
  QMenu foo(this);
  foo.addAction("set color", this, SLOT(getnewcolor()));
  foo.addAction("info", this, SIGNAL(inforequested()));
  foo.addAction("copy", this, SIGNAL(copied()));
  foo.exec(QCursor::pos());
}
