
// vbview.cpp
// an image viewing widget
// Copyright (c) 1998-2011 by The VoxBo Development Team

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

// one of the panels should have image manipulation options, just a
// combo box, text edit, and apply button, and a save button.  apply
// to the current layer.  options: byteswap, flipxyz, set origin, and
// many of the same things as in vbimagemunge.

enum {
  C_VIS = 0,
  C_NAME = 1,
  C_TYPE = 2,
  C_X = 3,
  C_Y = 4,
  C_Z = 5,
  C_VAL = 6,
  C_LABEL = 7
};

using namespace std;

#include <QApplication>
#include <QCursor>
#include <QFrame>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>
#include <QPrinter>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTextEdit>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidget>
#include "makestatcub.h"

#include "vbview.moc.h"
// from vbwidgets
#include "fileview.h"
#include "threshcalc.h"
#include "vbcontrast.h"

// sole constructor for vbview

VBView::VBView(QWidget *parent, const char *name) : QWidget(parent, name) {
  QHBox *tmph;
  QPushButton *button;

  init();

  // first the main infrastructure widgets

  // main layout is vertical
  QVBoxLayout *vbox = new QVBoxLayout(this);
  // things that are in the main vertical box: top layout, bottom layout
  QHBoxLayout *toplayout = new QHBoxLayout();
  QHBoxLayout *bottomlayout = new QHBoxLayout();
  vbox->addLayout(toplayout);
  vbox->addLayout(bottomlayout);
  // things that are in the top layout: image scrollview, button grid layout,
  // control widget
  sview = new QScrollArea();
  toplayout->addWidget(sview, 2);

  controlwidget = new QFrame();
  toplayout->addWidget(controlwidget, 1);
  QVBoxLayout *cbox = new QVBoxLayout;
  controlwidget->setLayout(cbox);
  // things that are in the control widget: layer table and numerous
  // context-sensitive widgets
  layertable = new QTableWidget;
  layertable->setContextMenuPolicy(Qt::CustomContextMenu);
  layertable->setStyle("selection-color:red;show-decoration-selected:0;");
  layertable->setFocusPolicy(Qt::NoFocus);
  // layertable->setDragDropMode(QAbstractItemView::InternalMove);
  // layertable->setDragEnabled(1);
  cbox->addWidget(layertable);
  // things that are in the bottom layout: the time series layout
  ts_window = new QWidget();
  ts_layout = new QHBoxLayout;
  ts_window->setLayout(ts_layout);

  QObject::connect(layertable, SIGNAL(cellClicked(int, int)), this,
                   SLOT(layertableclicked(int, int)));
  QObject::connect(layertable, SIGNAL(cellDoubleClicked(int, int)), this,
                   SLOT(layertabledclicked(int, int)));
  QObject::connect(layertable, SIGNAL(customContextMenuRequested(QPoint)), this,
                   SLOT(layertablerightclicked(QPoint)));

  ///////////////////////////////////////
  // the brain image
  ///////////////////////////////////////
  // sview->resize(260,260);
  drawarea = new VBCanvas;
  drawarea->SetImage(&currentimage);
  sview->setWidget(drawarea);
  // sview->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
  sview->setMinimumSize(256, 256);

  QObject::connect(drawarea, SIGNAL(mousemove(QMouseEvent)), this,
                   SLOT(MouseMoveEvent(QMouseEvent)));
  QObject::connect(drawarea, SIGNAL(mousepress(QMouseEvent)), this,
                   SLOT(MousePressEvent(QMouseEvent)));
  QObject::connect(drawarea, SIGNAL(leftcanvas()), this, SLOT(LeftCanvas()));

  ///////////////////////////////////////////////////
  // fixed controls right below the layer table
  ///////////////////////////////////////////////////
  tmph = new QHBox;
  tmph->addLabel("Show:");
  cb_coord2 = new QComboBox;
  cb_coord2->insertItem("voxel under mouse");
  cb_coord2->insertItem("voxel at crosshairs");
  tmph->addWidget(cb_coord2);
  cbox->addWidget(tmph);
  QObject::connect(cb_coord2, SIGNAL(activated(int)), this,
                   SLOT(NewCoordSystem(int)));
  tmph = new QHBox;
  tmph->addLabel("Coordinates relative to:");
  cb_coord1 = new QComboBox;
  cb_coord1->insertItem("corner (in voxels)");
  cb_coord1->insertItem("corner (in mm)");
  cb_coord1->insertItem("origin (in voxels)");
  cb_coord1->insertItem("origin (in mm)");
  tmph->addWidget(cb_coord1);
  cbox->addWidget(tmph);
  QObject::connect(cb_coord1, SIGNAL(activated(int)), this,
                   SLOT(NewCoordSystem(int)));

  ///////////////////////////////////////////////////
  // the (mostly) fixed controls
  ///////////////////////////////////////////////////

  viewbox = new QComboBox();
  magbox = new QComboBox();
  magbox->insertItem("25%");
  magbox->insertItem("33%");
  magbox->insertItem("50%");
  magbox->insertItem("66%");
  magbox->insertItem("100%");
  magbox->insertItem("150%");
  magbox->insertItem("200%");
  magbox->insertItem("300%");
  magbox->insertItem("400%");
  magbox->insertItem("500%");
  magbox->insertItem("600%");
  magbox->insertItem("700%");
  magbox->insertItem("800%");
  magbox->setCurrentItem(6);
  magbox->setEditable(1);
  magbox->setInsertionPolicy(QComboBox::NoInsertion);
  viewbox->insertItem("XYZ");
  viewbox->insertItem("XZY");
  viewbox->insertItem("YZX");
  viewbox->insertItem("Tri-View");
  viewbox->insertItem("Multi-View");

  QHBox *viewline = new QHBox;
  viewline->addLabel("view:");
  viewline->addWidget(viewbox);
  viewline->addSpacing(40);
  viewline->addLabel("mag:");
  viewline->addWidget(magbox);
  cbox->addWidget(viewline);

  QObject::connect(viewbox, SIGNAL(activated(const QString &)), this,
                   SLOT(NewView(const QString &)));
  QObject::connect(magbox, SIGNAL(activated(const QString &)), this,
                   SLOT(NewMag(const QString &)));

  // columns for multiview
  colbox = new QHBox;
  QSpinBox *tmps = new QSpinBox();
  tmps->setRange(1, 100);
  tmps->setSingleStep(1);
  tmps->setValue(5);
  colbox->addLabel("columns:");
  colbox->addWidget(tmps);
  cbox->addWidget(colbox);
  colbox->hide();
  QObject::connect(tmps, SIGNAL(valueChanged(int)), this,
                   SLOT(SetColumns(int)));

  // every n columns for multiview
  everynbox = new QHBox;
  tmps = new QSpinBox();
  tmps->setRange(1, 100);
  tmps->setSingleStep(1);
  tmps->setValue(1);
  everynbox->addLabel("every n:");
  everynbox->addWidget(tmps);
  cbox->addWidget(everynbox);
  everynbox->hide();
  QObject::connect(tmps, SIGNAL(valueChanged(int)), this, SLOT(SetEveryn(int)));

  // now some actual buttons
  tmph = new QHBox();
  cbox->addWidget(tmph);

  QToolButton *tb;

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_cross.png")));
  tb->setToggleButton(1);
  tb->setChecked(1);
  QToolTip::add(tb, "Toggle Crosshairs");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(ShowCrosshairs(bool)));

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_fliph.png")));
  tb->setToggleButton(1);
  tb->setChecked(0);
  QToolTip::add(tb, "Flip Image Horizontally");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(FlipH(bool)));

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_flipv.png")));
  tb->setToggleButton(1);
  tb->setChecked(0);
  QToolTip::add(tb, "Flip Image Vertically");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(FlipV(bool)));

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_origin.png")));
  tb->setToggleButton(1);
  tb->setOn(0);
  QToolTip::add(tb, "Show/Hide Origin");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(ShowOrigin(bool)));

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_iso.png")));
  tb->setToggleButton(1);
  tb->setChecked(0);
  QToolTip::add(tb, "Scale Image Isotropically");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(ToggleScaling(bool)));

  // tb=new QToolButton();
  // tb->setIconSet(QIcon(QPixmap(":/icons/b_fill.png")));
  // tb->setToggleButton(1);
  // QToolTip::add(tb,"Mask Fill Mode");
  // tmph->addWidget(tb);

  tb = new QToolButton();
  tb->setIconSet(QIcon(QPixmap(":/icons/b_draw.png")));
  tb->setToggleButton(1);
  tb->setChecked(0);
  QToolTip::add(tb, "Toggle Draw Mode");
  tmph->addWidget(tb);
  QObject::connect(tb, SIGNAL(toggled(bool)), this, SLOT(SetDrawmask(bool)));

  ///////////////////////////////////////////////////
  // the layer table
  ///////////////////////////////////////////////////

  // columns are name, visible, xyztval
  // layertable->setStyle("selection-background-color:qlineargradient(x1: 0, y1:
  // 0, x2: 0.5, y2: 0.5, stop: 0 #FF92BB, stop: 1 white)");
  layertable->setColumnCount(8);
  layertable->setSizePolicy(QSizePolicy::MinimumExpanding,
                            QSizePolicy::Minimum);
  // layertable->resizeColumnsToContents();
  int cwidth = layertable->fontMetrics().width("8888");
  int iwidth = QPixmap(":/icons/visible.png").width();
  layertable->setColumnWidth(C_VIS, iwidth);
  layertable->setColumnWidth(C_NAME, cwidth * 3);
  layertable->setColumnWidth(C_TYPE, iwidth);
  layertable->setColumnWidth(C_X, cwidth);
  layertable->setColumnWidth(C_Y, cwidth);
  layertable->setColumnWidth(C_Z, cwidth);
  layertable->setColumnWidth(C_VAL, cwidth * 2);
  layertable->setColumnWidth(C_LABEL, cwidth * 2);
  layertable->setMinimumWidth(cwidth * 10 + iwidth * 2 + 8);
  layertable->setShowGrid(0);
  layertable->horizontalHeader()->setVisible(1);
  layertable->horizontalHeader()->setClickable(0);
  layertable->verticalHeader()->setVisible(0);
  layertable->setSelectionBehavior(QAbstractItemView::SelectRows);
  layertable->setSelectionMode(QAbstractItemView::SingleSelection);
  QStringList hl;
  hl << ""
     << "name"
     << ""
     << "x"
     << "y"
     << "z"
     << "val"
     << "label";
  layertable->setHorizontalHeaderLabels(hl);

  ///////////////////////////////////////////////////
  // anat stuff
  ///////////////////////////////////////////////////

  // slice (X) selection slider
  xslicebox = new QHBox;
  xslicebox->addLabel("X position:");
  xsliceslider = new QSlider(Qt::Horizontal);
  xsliceslider->setPageStep(1);
  xposedit = new QLabel("0", xslicebox);
  xposedit->setFixedWidth(50);
  xposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  xposedit->setLineWidth(1);
  xslicebox->addWidget(xsliceslider);
  xslicebox->addWidget(xposedit);
  xslicebox->hide();
  cbox->addWidget(xslicebox);
  QObject::connect(xsliceslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewXSlice(int)));

  // slice (Y) selection slider
  yslicebox = new QHBox();
  yslicebox->addLabel("Y position:");
  ysliceslider = new QSlider(Qt::Horizontal);
  ysliceslider->setPageStep(1);
  yposedit = new QLabel("0", yslicebox);
  yposedit->setFixedWidth(50);
  yposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  yposedit->setLineWidth(1);
  yslicebox->addWidget(ysliceslider);
  yslicebox->addWidget(yposedit);
  yslicebox->hide();
  cbox->addWidget(yslicebox);
  QObject::connect(ysliceslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewYSlice(int)));

  // slice (Z) selection slider
  zslicebox = new QHBox();
  zslicebox->addLabel("Z position:");
  zsliceslider = new QSlider(Qt::Horizontal);
  zsliceslider->setPageStep(1);
  zposedit = new QLabel("0", zslicebox);
  zposedit->setFixedWidth(50);
  zposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  zposedit->setLineWidth(1);
  zslicebox->addWidget(zsliceslider);
  zslicebox->addWidget(zposedit);
  zslicebox->hide();
  cbox->addWidget(zslicebox);
  QObject::connect(zsliceslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewZSlice(int)));

  // slice (T) selection slider
  tslicebox = new QHBox();
  tslicebox->addLabel("T position:");
  tsliceslider = new QSlider(Qt::Horizontal);
  tsliceslider->setPageStep(1);
  tposedit = new QLabel("0", xslicebox);
  tposedit->setFixedWidth(50);
  tposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  tposedit->setLineWidth(1);
  tslicebox->addWidget(tsliceslider);
  tslicebox->addWidget(tposedit);
  tslicebox->hide();
  cbox->addWidget(tslicebox);
  QObject::connect(tsliceslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewTSlice(int)));

  // brightness slider
  tmph = new QHBox;
  tmph->addLabel("brightness:");
  brightslider = new QSlider(Qt::Horizontal);
  brightslider->setPageStep(10);
  brightslider->setRange(1, 100);
  brightslider->setValue(50);
  tmph->addWidget(brightslider);
  cbox->addWidget(tmph);
  QObject::connect(brightslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewBrightness(int)));

  // contrast slider
  tmph = new QHBox;
  tmph->addLabel("contrast:");
  contrastslider = new QSlider(Qt::Horizontal);
  contrastslider->setPageStep(10);
  contrastslider->setRange(1, 100);
  contrastslider->setValue(50);
  tmph->addWidget(contrastslider);
  cbox->addWidget(tmph);
  QObject::connect(contrastslider, SIGNAL(valueChanged(int)), this,
                   SLOT(NewContrast(int)));

  tmph = new QHBox;
  tmph->addLabel("opacity:");
  q_trans = new QSlider;
  q_trans->setOrientation(Qt::Horizontal);
  q_trans->setMinimum(0);
  q_trans->setMaximum(100);
  q_trans->setValue(40);
  q_trans->setSingleStep(5);
  q_trans->setPageStep(5);
  tmph->addWidget(q_trans);
  cbox->addWidget(tmph);
  QObject::connect(q_trans, SIGNAL(valueChanged(int)), this,
                   SLOT(newtrans(int)));

  ///////////////////////////////////////
  // mask-related stuff
  ///////////////////////////////////////

  maskbox = new QVBox;
  maskbox->hide();
  cbox->addWidget(maskbox);

  // button=new QPushButton("Change Mask Dimensions");
  // cbox->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(ChangeMask()));

  tmph = new QHBox;
  tmph->addLabel("Save Mask:");
  button = new QPushButton("Multi-Mask");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handle_savemask()));

  // button=new QPushButton("Separate Masks");
  // tmph->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveSeparateMasks()));

  // button=new QPushButton("Combined");
  // tmph->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveCombinedMask()));
  // maskbox->addWidget(tmph);

  maskview = new QTMaskView;
  QObject::connect(maskview, SIGNAL(changed(QWidget *)), this,
                   SLOT(handle_maskchanged(QWidget *)));
  QObject::connect(maskview, SIGNAL(selected(int)), this,
                   SLOT(handle_maskselected(int)));
  QObject::connect(maskview, SIGNAL(copied(int)), this,
                   SLOT(handle_maskcopied(int)));
  QObject::connect(maskview, SIGNAL(inforequested(int)), this,
                   SLOT(handle_inforequested(int)));
  QObject::connect(maskview, SIGNAL(savemask()), this, SLOT(handle_savemask()));
  QObject::connect(maskview, SIGNAL(newmask(QTMaskWidget *)), this,
                   SLOT(handle_newmask(QTMaskWidget *)));
  QObject::connect(maskview, SIGNAL(newradius(int)), this,
                   SLOT(NewRadius(int)));
  QObject::connect(maskview, SIGNAL(newzradius(int)), this,
                   SLOT(NewZRadius(int)));

  ///////////////////////////////////////
  // stat related stuff
  ///////////////////////////////////////

  statbox = new QVBox;
  statbox->hide();
  cbox->addWidget(statbox);

  // [calculate] threshold:___ "set by method (params)"
  tmph = new QHBox;
  button = new QPushButton("Calculate");
  connect(button, SIGNAL(clicked()), this, SLOT(GetThreshold()));
  tmph->addWidget(button);
  tmph->addLabel("threshold:");
  lowedit = new QLineEdit("3.5");
  lowedit->setFixedWidth(80);  // FIXME use font info to set width
  tmph->addWidget(lowedit);
  threshinfo = new QLabel;
  tmph->addWidget(threshinfo);
  statbox->addWidget(tmph);

  // high__  cluster __
  tmph = new QHBox;
  tmph->addLabel("high:");
  highedit = new QLineEdit("5.0");
  highedit->setFixedWidth(80);
  tmph->addWidget(highedit);
  tmph->addLabel("cluster:");
  clusteredit = new QLineEdit("1");
  clusteredit->setFixedWidth(80);
  tmph->addWidget(clusteredit);
  statbox->addWidget(tmph);

  QObject::connect(lowedit, SIGNAL(textEdited(const QString &)), this,
                   SLOT(HighlightLow()));
  QObject::connect(lowedit, SIGNAL(returnPressed()), this, SLOT(SetLow()));
  QObject::connect(highedit, SIGNAL(textEdited(const QString &)), this,
                   SLOT(HighlightHigh()));
  QObject::connect(highedit, SIGNAL(returnPressed()), this, SLOT(SetHigh()));
  QObject::connect(clusteredit, SIGNAL(textEdited(const QString &)), this,
                   SLOT(HighlightCluster()));
  QObject::connect(clusteredit, SIGNAL(returnPressed()), this,
                   SLOT(SetCluster()));

  // [] flip sign [] show <0 [] show ns voxels
  tmph = new QHBox;
  flipbox = new QCheckBox("flip sign");
  flipbox->setChecked(0);
  tmph->addWidget(flipbox);
  QObject::connect(flipbox, SIGNAL(toggled(bool)), this, SLOT(SetFlip(bool)));
  tailbox = new QCheckBox("show <0");
  tailbox->setChecked(1);
  tmph->addWidget(tailbox);
  QObject::connect(tailbox, SIGNAL(toggled(bool)), this, SLOT(SetTails(bool)));
  nsbox = new QCheckBox("render ns voxels");
  tmph->addWidget(nsbox);
  QObject::connect(nsbox, SIGNAL(toggled(bool)), this, SLOT(togglens(bool)));
  statbox->addWidget(tmph);

  // colorscale ns_colorscale
  tmph = new QHBox;
  q_scale = new VBScalewidget;
  tmph->addWidget(q_scale);
  q_scale->setscale(3.5, 5, QColor(0, 0, 200), QColor(100, 100, 255),
                    QColor(255, 0, 0), QColor(255, 255, 0));
  q_scale->setFixedSize(220, 50);
  QObject::connect(q_scale, SIGNAL(newcolors(QColor, QColor, QColor, QColor)),
                   this, SLOT(newcolors(QColor, QColor, QColor, QColor)));

  q_scale2 = new VBScalewidget;
  tmph->addWidget(q_scale2);
  q_scale2->setscale(0, 3.5, QColor(0, 0, 200), QColor(30, 255, 30));
  q_scale2->setFixedSize(220, 50);
  q_scale2->hide();
  QObject::connect(q_scale2, SIGNAL(newcolors(QColor, QColor, QColor, QColor)),
                   this, SLOT(newcolors2(QColor, QColor, QColor, QColor)));

  statbox->addWidget(tmph);

  tmph = new QHBox;
  tmph->addLabel("Find:");
  button = new QPushButton("All Regions");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(FindAllRegions()));
  button = new QPushButton("Region at Crosshairs");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(FindCurrentRegion()));
  statbox->addWidget(tmph);

  // tmph=new QHBox;
  // button=new QPushButton("Clear Regions");
  // tmph->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(ClearRegions()));
  // statbox->addWidget(tmph);

  tmph = new QHBox;
  tmph->addLabel("Save:");
  button = new QPushButton("Full map");
  button->setToolTip("save the full statistical map");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(SaveFullMap()));
  button = new QPushButton("Visible map");
  button->setToolTip("save a map of just the visible voxels");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(SaveVisibleMap()));
  button = new QPushButton("Visible mask");
  button->setToolTip("save a mask of the visible voxels");
  tmph->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(SaveRegionMask()));
  statbox->addWidget(tmph);

  // tmph=new QHBox;
  // button=new QPushButton("Dump Region Stats");
  // tmph->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(ShowRegionStats()));
  // statbox->addWidget(tmph);

  // tmph=new QHBox;
  // button=new QPushButton("Copy Checked Regions as Masks");
  // tmph->addWidget(button);
  // QObject::connect(button,SIGNAL(clicked()),this,SLOT(CopyRegions()));
  // statbox->addWidget(tmph);

  // FIXME region view

  //////////////////////////////////
  // time series browser
  //////////////////////////////////

  // previously declared...
  // ts_window=new QWidget();
  // ts_layout=new QHBoxLayout(ts_window);

  // the actual time series widget
  tspane = new PlotScreen;
  ts_layout->addWidget(tspane);
  // the listbox that shows things you can plot
  tslist = new QListWidget;
  ts_layout->addWidget(tslist);
  tslist->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QObject::connect(tslist, SIGNAL(itemSelectionChanged()), this,
                   SLOT(UpdateTS()));
  // some additional buttons
  QGroupBox *bg = new QGroupBox("Time Series Options");
  ts_layout->addWidget(bg);
  QVBoxLayout *tmpv = new QVBoxLayout;
  bg->setLayout(tmpv);
  ts_meanscalebox = new QCheckBox("Scale to mean of 1");
  ts_detrendbox = new QCheckBox("Linear detrend");
  ts_filterbox = new QCheckBox("Apply filtering");
  ts_filterbox->setToolTip("Apply the same filtering used in running the GLM.");
  ts_removebox = new QCheckBox("Remove Covariates of No Interest");
  ts_scalebox = new QCheckBox("Scale Covariates using Beta");
  ts_powerbox = new QCheckBox("Show Power Spectrum");
  ts_pcabox = new QCheckBox("PCA (RGB)");
  ts_pcabox->setToolTip("Apply the same filtering used in running the GLM.");
  tmpv->addWidget(ts_meanscalebox);
  tmpv->addWidget(ts_detrendbox);
  tmpv->addWidget(ts_filterbox);
  tmpv->addWidget(ts_removebox);
  tmpv->addWidget(ts_scalebox);
  tmpv->addWidget(ts_powerbox);
  tmpv->addWidget(ts_pcabox);

  // time series averaging
  averagebox = new QComboBox;
  averagebox->insertItem("No averaging");
  tmpv->addWidget(averagebox);
  QObject::connect(averagebox, SIGNAL(activated(const QString &)), this,
                   SLOT(NewAverage()));

  ts_maskbox = new QCheckBox("Graph average of visible region");
  ts_maskbox->setToolTip(
      "graph the average of the current layer's visible region (if un-checked, "
      "you see the same voxel displayed in the layer table");
  tmpv->addWidget(ts_maskbox);

  button = new QPushButton("Save Time Series", bg, "savets");
  tmpv->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(SaveTS()));

  button = new QPushButton("Save Graph", bg, "savegraph");
  tmpv->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(SaveGraph()));

  tmpv->addStretch(1);

  cbox->addStretch(1);

  // QObject::connect(bg,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));

  QObject::connect(ts_meanscalebox, SIGNAL(toggled(bool)), this,
                   SLOT(UpdateTS()));
  QObject::connect(ts_detrendbox, SIGNAL(toggled(bool)), this,
                   SLOT(UpdateTS()));
  QObject::connect(ts_filterbox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));
  QObject::connect(ts_removebox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));
  QObject::connect(ts_scalebox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));
  QObject::connect(ts_powerbox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));
  QObject::connect(ts_pcabox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));
  QObject::connect(ts_maskbox, SIGNAL(toggled(bool)), this, SLOT(UpdateTS()));

  setFocusPolicy(Qt::StrongFocus);
}

VBView::~VBView() {}

void VBView::setMinimal(bool b) {
  if (b) {
    controlwidget->hide();
    // buttonlayout->hide();
  } else {
    controlwidget->show();
    // buttonlayout->show();
  }
}

void VBView::init() {
  mode = vbv_mask;
  q_currentdir = ".";
  q_columns = 5;
  q_everyn = 1;
  q_crosshairs = 1;
  q_showorigin = 0;
  q_showts = 0;
  q_xslice = 0;
  q_yslice = 0;
  q_zslice = 0;
  q_volume = 0;
  q_inmm = 0;
  q_fromorigin = 0;
  q_atmouse = 1;
  q_fliph = q_flipv = 0;

  q_update = 1;

  viewlist.clear();
  q_viewmode = vbv_axial;

  q_isotropic = 0;
  q_mag = 200;
  q_xscale = 2.0;
  q_yscale = 2.0;
  q_zscale = 2.0;
  q_pitch = 0;
  q_roll = 0;
  q_yaw = 0;

  regionpos = 0;
  q_maskmode = MASKMODE_DRAW;

  q_tstype = tsmode_mouse;

  q_radius = 0;
  q_usezradius = 0;

  q_drawmask = 0;
  q_maskcolor = (255 << 16) | (255) | (1 << 24);  // set the render bit
  q_maskindex = 1;
  q_currentmask = 0;
  q_currentregion = 0;
  q_xviewsize = 0;
  q_yviewsize = 0;
  // q_bgcolor=qRgb(175,165,181);
  // q_bgcolor=qRgb(176,198,202);
  // q_bgcolor=qRgb(200,195,144);
  q_bgcolor = qRgb(185, 207, 255);
  myaverage = (TASpec *)NULL;
  currentlayer = previouslayer = layers.end();
}

void VBView::newcolors(QColor n1, QColor n2, QColor p1, QColor p2) {
  if (currentlayer == layers.end()) return;  // actually shouldn't happen
  currentlayer->q_negcolor1 = n1;
  currentlayer->q_negcolor2 = n2;
  currentlayer->q_poscolor1 = p1;
  currentlayer->q_poscolor2 = p2;
  currentlayer->render();
  RenderAll();
}

void VBView::newcolors2(QColor, QColor, QColor p1, QColor p2) {
  if (currentlayer == layers.end()) return;  // actually shouldn't happen
  currentlayer->q_nscolor1 = p1;
  currentlayer->q_nscolor2 = p2;
  currentlayer->render();
  RenderAll();
}

void VBView::newtrans(int t) {
  if (currentlayer == layers.end()) return;  // actually shouldn't happen
  currentlayer->alpha = t;
  currentlayer->render();
  RenderAll();
}

void VBView::layertableclicked(int row, int col) {
  // which layer did we click on?
  VBLayerI li = layers.begin();
  int rowx = row;
  while (rowx--) li++;

  if (col == C_VIS) {
    // we never hide the bottom-most layer, for now
    if (li == layers.begin()) return;
    if (li->q_visible) {
      li->q_visible = 0;
      QTableWidgetItem *twi = new QTableWidgetItem("");
      twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      layertable->setItem(row, col, twi);
    } else {
      li->q_visible = 1;
      QTableWidgetItem *twi =
          new QTableWidgetItem(QIcon(QPixmap(":/icons/visible.png")), "");
      twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      layertable->setItem(row, col, twi);
    }
    RenderAll();
    return;
  }
  // update the "current" layer (and the previous currentlayer)
  previouslayer = currentlayer;
  currentlayer = li;
  // do something about it
  if (previouslayer != currentlayer) updatewidgets();
}

void VBView::layertabledclicked(int row, int) {
  // which layer did we click on?
  VBLayerI li = layers.begin();
  int rowx = row;
  while (rowx--) li++;
  emit renameme(this, li->filename);
}

// update widget visibility and values so as to be in sync with
// currently selected layer

void VBView::updatewidgets() {
  // FIXME update menu items too, like graying out mask save when a
  // mask layer isn't selected

  // stat-only stuff
  if (currentlayer->type == VBLayer::vb_stat ||
      currentlayer->type == VBLayer::vb_corr ||
      currentlayer->type == VBLayer::vb_glm) {
    statbox->show();
    q_scale->setscale(currentlayer->q_thresh, currentlayer->q_high,
                      currentlayer->q_negcolor1, currentlayer->q_negcolor2,
                      currentlayer->q_poscolor1, currentlayer->q_poscolor2);
    q_scale2->setscale(0, currentlayer->q_thresh, currentlayer->q_nscolor1,
                       currentlayer->q_nscolor2);
    lowedit->setToolTip(currentlayer->threshtt.c_str());
  } else {
    statbox->hide();
  }

  if (currentlayer->type == VBLayer::vb_mask) {
    if (currentlayer != lastmasklayer) {
      lastmasklayer = currentlayer;
      maskview->clear();
      pair<uint32, VBMaskSpec> mm;
      vbforeach(mm, currentlayer->cube.maskspecs) {
        maskview->addMask(mm.first, mm.second.name,
                          QColor(mm.second.r, mm.second.g, mm.second.b));
      }
      maskview->setWindowTitle((currentlayer->filename + " masks").c_str());
    }
    maskview->show();
  } else {
    maskview->hide();
  }

  if (currentlayer->tes.data)
    tslicebox->show();
  else
    tslicebox->hide();
  // low high cluster
  lowedit->setText(strnum(currentlayer->q_thresh).c_str());
  highedit->setText(strnum(currentlayer->q_high).c_str());
  clusteredit->setText(strnum(currentlayer->q_clustersize).c_str());
  q_trans->setValue(currentlayer->alpha);
}

void VBView::layertablerightclicked(QPoint p) {
  int row = layertable->row(layertable->itemAt(p));
  // which layer did we click on?
  VBLayerI li = layers.begin();
  while (row--) li++;
  currentlayer = li;
  QMenu mm;
  mm.addAction("delete layer", this, SLOT(deletelayer()));
  mm.addAction("copy layer mask", this, SLOT(Copy()));
  if (currentlayer->type == VBLayer::vb_stat)
    mm.addAction("sign flip layer", this, SLOT(fliplayer()));
  if (currentlayer->type == VBLayer::vb_corr)
    mm.addAction("sign flip layer", this, SLOT(fliplayer()));
  mm.exec(layertable->mapToGlobal(p));
}

void VBView::deletelayer() {
  layers.erase(currentlayer);
  updateLayerTable();
  RenderAll();
}

void VBView::fliplayer() {
  currentlayer->cube *= -1;
  RenderAll();
}

void VBView::paintEvent(QPaintEvent *) { drawarea->update(); }

MyView::MyView() {}

MyView::~MyView() {}

void VBView::BuildViews(int mode) {
  if (layers.size() == 0) return;
  //   if (!q_update)
  //     return;
  viewlist.clear();
  MyView vv;

  q_viewmode = mode;

  int xx = (int)ceil(q_xscale * base_dimx);
  int yy = (int)ceil(q_yscale * base_dimy);
  int zz = (int)ceil(q_zscale * base_dimz);

  vv.xoff = 0;
  vv.yoff = 0;

  // most views require x and y position control
  xslicebox->show();
  yslicebox->show();

  if (mode == vbv_axial) {
    vv.orient = MyView::vb_xy;
    vv.position = -1;
    vv.width = xx;
    vv.height = yy;
    viewlist.push_back(vv);
    zslicebox->show();
  } else if (mode == vbv_coronal) {
    vv.orient = MyView::vb_xz;
    vv.position = -1;
    vv.width = xx;
    vv.height = zz;
    viewlist.push_back(vv);
    zslicebox->show();
  } else if (mode == vbv_sagittal) {
    vv.orient = MyView::vb_yz;
    vv.position = -1;
    vv.width = yy;
    vv.height = zz;
    viewlist.push_back(vv);
    zslicebox->show();
  } else if (mode == vbv_multi) {
    vv.orient = MyView::vb_xy;
    vv.width = xx;
    vv.height = yy;
    int col = 0;
    for (int i = 0; i < layers.front().cube.dimz; i += q_everyn) {
      vv.position = i;
      viewlist.push_back(vv);
      if (++col >= q_columns) {
        col = 0;
        vv.xoff = 0;
        vv.yoff += yy;
      } else
        vv.xoff += xx;
    }
    xslicebox->hide();
    yslicebox->hide();
    zslicebox->hide();
  } else if (mode == vbv_tri) {
    // axial
    vv.orient = MyView::vb_xy;
    vv.position = -1;
    vv.width = xx;
    vv.height = yy;
    viewlist.push_back(vv);

    // coronal
    vv.orient = MyView::vb_xz;
    vv.position = -1;
    vv.yoff += yy;
    vv.width = xx;
    vv.height = zz;
    viewlist.push_back(vv);

    // sagittal
    vv.orient = MyView::vb_yz;
    vv.position = -1;
    vv.xoff += xx;
    vv.width = yy;
    vv.height = zz;
    viewlist.push_back(vv);
    zslicebox->show();
  }
  SetViewSize();
}

void VBView::SetViewSize() {
  // set xviewsize and yviewsize for new MyView
  q_xviewsize = 0;
  q_yviewsize = 0;
  double right, bottom;
  for (int i = 0; i < (int)viewlist.size(); i++) {
    right = (viewlist[i].xoff + viewlist[i].width);
    bottom = (viewlist[i].yoff + viewlist[i].height);
    if (right > q_xviewsize) q_xviewsize = (int)ceil(right);
    if (bottom > q_yviewsize) q_yviewsize = (int)ceil(bottom);
  }

  if (currentimage.width() != q_xviewsize ||
      currentimage.height() != q_yviewsize) {
    currentimage.create(q_xviewsize, q_yviewsize, 32);
    // FIXME are we using the alpha buffer?
    currentimage.setAlphaBuffer(0);
    drawarea->setFixedSize(q_xviewsize, q_yviewsize);
  }
  currentimage.fill(q_bgcolor);
}

void VBView::UpdateView(MyView &view) {
  drawarea->updateRegion((int)(view.xoff), (int)(view.yoff), (int)(view.width),
                         (int)(view.height));
}

void VBView::updateLayerTable() {
  layertable->setRowCount(layers.size());
  VBLayerI li = layers.begin();
  QTableWidgetItem *tmpi;
  for (int i = 0; i < layertable->rowCount(); i++) {
    // make all the widgets
    for (int j = 0; j < layertable->columnCount(); j++) {
      if (j == C_VIS) continue;
      tmpi = new QTableWidgetItem("");
      tmpi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      // if (li->mask)  // FIXME set color for masked layers?
      // tmpi->setBackground(QBrush(QColor(255,220,220)));
      layertable->setItem(i, j, tmpi);
      tmpi->setToolTip(li->tooltipinfo());
    }
    if (li->q_visible) {
      QTableWidgetItem *twi =
          new QTableWidgetItem(QIcon(QPixmap(":/icons/visible.png")), "");
      twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      layertable->setItem(i, C_VIS, twi);
      twi->setToolTip(li->tooltipinfo());
    }
    tmpi = layertable->item(i, C_NAME);
    tmpi->setText(xfilename(li->filename).c_str());
    tmpi->setToolTip(li->tooltipinfo());
    QTableWidgetItem *twi;
    if (!li->mask) {
      if (li->type == VBLayer::vb_stat)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/t_stat.png")), "");
      else if (li->type == VBLayer::vb_mask)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/t_mask.png")), "");
      else if (li->type == VBLayer::vb_corr)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/t_corr.png")), "");
      // else if (li->type==VBLayer::vb_aux)
      //   twi=new QTableWidgetItem(QIcon(QPixmap(":/icons/t_aux.png")),"");
      else if (li->type == VBLayer::vb_glm)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/t_glm.png")), "");
      else
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/t_struct.png")), "");
    } else {
      if (li->type == VBLayer::vb_stat)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/m_stat.png")), "");
      else if (li->type == VBLayer::vb_mask)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/m_mask.png")), "");
      else if (li->type == VBLayer::vb_corr)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/m_corr.png")), "");
      // else if (li->type==VBLayer::vb_aux)
      //   twi=new QTableWidgetItem(QIcon(QPixmap(":/icons/m_aux.png")),"");
      else if (li->type == VBLayer::vb_glm)
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/m_glm.png")), "");
      else
        twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/m_struct.png")), "");
    }
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    twi->setToolTip(li->tooltipinfo());
    layertable->setItem(i, C_TYPE, twi);
    li++;
  }
  layertable->adjustSize();
}

void VBView::ToolChange(int tool) {
  if (tool == 0)
    mode = vbv_mask;
  else if (tool == 1)
    mode = vbv_erase;
  else if (tool == 2)
    mode = vbv_cross;
  else if (tool == 3)
    mode = vbv_fill;
}

void VBView::Print() {
  QPrinter pr(QPrinter::ScreenResolution);
  pr.setPageSize(QPrinter::Letter);
  pr.setColorMode(QPrinter::Color);
  // pr.setFullPage(TRUE);
  // pr.setResolution(600);
  // QPaintDeviceMetrics metrics(&pr);
  // int margin=metrics.logicalDpiY()*2;

  if (pr.setup(this)) {
    QPainter mypt;
    mypt.begin(&pr);
    // get actual dpi for printer
    // draw it
    // mypt.drawImage(margin,margin,*(drawarea->GetImage()),0,0,q_xviewsize,q_yviewsize);
    mypt.drawImage(0, 0, *(drawarea->GetImage()), 0, 0, q_xviewsize,
                   q_yviewsize);
    mypt.end();
  }
  // pr.newPage();
}

// handlers

void VBView::GoToOrigin() {
  if (layers.size() == 0) return;
  VBLayerI ll = layers.begin();
  int xx = ll->cube.origin[0];
  int yy = ll->cube.origin[1];
  int zz = ll->cube.origin[2];
  if (xx < 0 || xx > ll->cube.dimx - 1) return;
  if (yy < 0 || yy > ll->cube.dimy - 1) return;
  if (zz < 0 || zz > ll->cube.dimz - 1) return;
  // is our new slice same as the old one?
  q_xslice = xx;
  q_yslice = yy;
  q_zslice = zz;
  // the below will trigger calls to NewXSlice(), etc., but those
  // functions don't do anything newval==q_xslice
  xsliceslider->setValue(xx);
  ysliceslider->setValue(yy);
  zsliceslider->setValue(zz);
  xposedit->setText(strnum(xx).c_str());
  yposedit->setText(strnum(yy).c_str());
  zposedit->setText(strnum(zz).c_str());
  setAllLayerCoords(q_xslice, q_yslice, q_zslice);
  RenderAll();
}

int VBView::NewXSlice(int newval) {
  if (layers.size() == 0) return 1;
  int nv = newval;
  int maxx = layers.begin()->cube.dimx - 1;
  if (nv > maxx) nv = maxx;
  if (nv < 0) nv = 0;
  // is our new slice same as the old one?
  if (nv == q_xslice) return 0;
  xposedit->setText(strnum(nv).c_str());
  q_xslice = nv;
  setAllLayerCoords(q_xslice, q_yslice, q_zslice);
  RenderAll();
  return (0);
}

void VBView::LayerInfo() {
  if (currentlayer == layers.end()) return;
  Cube cb = currentlayer->cube;
  cb.intersect(currentlayer->rendercube);
  vector<VBRegion> rlist;
  rlist = findregions(cb, vb_ne, 0.0);

  // the below adapted from vbim, should probably be more
  // library-ified

  double vfactor = cb.voxsize[0] * cb.voxsize[1] * cb.voxsize[2];
  string regioninfo;
  for (size_t i = 0; i < rlist.size(); i++) {
    // calculate centers of mass
    double x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0, totalmass = 0;
    for (VI myvox = rlist[i].begin(); myvox != rlist[i].end(); myvox++) {
      x1 += myvox->second.x;
      y1 += myvox->second.y;
      z1 += myvox->second.z;
      x2 += myvox->second.x * myvox->second.val;
      y2 += myvox->second.y * myvox->second.val;
      z2 += myvox->second.z * myvox->second.val;
      totalmass += myvox->second.val;
    }
    x1 /= rlist[i].size();
    y1 /= rlist[i].size();
    z1 /= rlist[i].size();
    x2 /= totalmass;
    y2 /= totalmass;
    z2 /= totalmass;

    VBRegion peakrr = rlist[i].maxregion();
    VBRegion minrr = rlist[i].minregion();
    double pxx, pyy, pzz;
    peakrr.GeometricCenter(pxx, pyy, pzz);
    regioninfo +=
        (format("region %02d: count: %d\n") % i % (rlist[i].size())).str();
    if (vfactor > FLT_MIN)
      regioninfo += (format("  mm3: %g\n") % (vfactor * rlist[i].size())).str();
    regioninfo += (format("  sum: %g\n") % totalmass).str();
    regioninfo +=
        (format("  mean: %g\n") % (totalmass / (double)rlist[i].size())).str();
    regioninfo += (format("  center: %g,%g,%g\n") % x1 % y1 % z1).str();
    regioninfo += (format("  weighted ctr: %g,%g,%g\n") % x2 % y2 % z2).str();
    regioninfo +=
        (format("  peakval: %g\n") % (peakrr.begin()->second.val)).str();
    regioninfo += (format("  peakcnt: %d\n") % (peakrr.size())).str();
    regioninfo +=
        (format("  minval: %g\n") % (minrr.begin()->second.val)).str();
    regioninfo += (format("  mincnt: %d\n") % (minrr.size())).str();
    regioninfo += (format("  peak center: %g,%g,%g\n") % pxx % pyy % pzz).str();
    regioninfo +=
        (format("note: peakcnt (or lowcnt) is the total number of voxels in\n"))
            .str();
    regioninfo +=
        (format("the region with the same peak (or min) value\n")).str();
    regioninfo += "\n";
  }
  // QDialog *dd=new QDialog(this);
  QTextEdit *te = new QTextEdit;
  te->setReadOnly(1);
  te->setGeometry(200, 200, 400, 300);
  te->setText(regioninfo.c_str());
  te->show();
}

int VBView::NewYSlice(int newval) {
  if (layers.size() == 0) return 1;
  int nv = newval;
  int maxy = layers.begin()->cube.dimy - 1;
  if (nv > maxy) nv = maxy;
  if (nv < 0) nv = 0;
  // is our new slice same as the old one?
  if (nv == q_yslice) return 0;
  yposedit->setText(strnum(nv).c_str());
  q_yslice = nv;
  setAllLayerCoords(q_xslice, q_yslice, q_zslice);
  RenderAll();
  return (0);
}

int VBView::NewZSlice(int newval) {
  if (layers.size() == 0) return 1;
  int nv = newval;
  int maxz = layers.begin()->cube.dimz - 1;
  if (nv > maxz) nv = maxz;
  if (nv < 0) nv = 0;
  // is our new slice same as the old one?
  if (nv == q_zslice) return 0;
  zposedit->setText(strnum(nv).c_str());
  q_zslice = nv;
  setAllLayerCoords(q_xslice, q_yslice, q_zslice);
  RenderAll();
  return (0);
}

int VBView::NewTSlice(int newval) {
  if (layers.size() == 0) return 1;
  if (!(layers.begin()->tes.data)) return 2;
  int nv = newval;
  int maxvol = layers.begin()->tes.dimt - 1;
  if (nv < 0) nv = 0;
  if (nv > maxvol) nv = maxvol;
  if (q_volume == nv) return 0;
  tposedit->setText(strnum(nv).c_str());
  q_volume = nv;
  layers.begin()->tes.getCube(q_volume, layers.begin()->cube);
  layers.begin()->render();
  RenderAll();
  return (0);
}

int VBView::NewBrightness(int newval) {
  if (layers.size() == 0) return 0;
  layers.begin()->q_brightness = newval;
  layers.begin()->render();
  RenderAll();
  return 0;
}

int VBView::NewContrast(int newval) {
  if (layers.size() == 0) return 0;
  layers.begin()->q_contrast = newval;
  layers.begin()->render();
  RenderAll();
  return 0;
}

int VBView::NewAverage() {
  int ind = averagebox->currentItem();
  if (ind == 0) {
    myaverage = (TASpec *)NULL;
    string status = "time series averaging off";
    statusline->setText(status.c_str());
    reload_averages();
    UpdateTS();
    return 0;
  }
  if (ind > (int)trialsets.size()) return 1;
  myaverage = &(trialsets[ind - 1]);
  string status = "time series averaging " + myaverage->name + " selected - ";
  status += strnum(myaverage->startpositions.size()) +
            " trials (or whatever) averaged";
  statusline->setText(status.c_str());
  UpdateTS();
  return 0;
}

void VBView::NewCoordSystem(int) {
  int ind1 = cb_coord1->currentIndex();
  int ind2 = cb_coord2->currentIndex();
  if (ind1 == 1 || ind1 == 3)
    q_inmm = 1;
  else
    q_inmm = 0;
  if (ind1 == 0 || ind1 == 1)
    q_fromorigin = 0;
  else
    q_fromorigin = 1;
  if (ind2 == 0)
    q_atmouse = 1;
  else
    q_atmouse = 0;
  // FIXME is this 100% reliable for getting base coords at crosshairs?
  if (!q_atmouse) setAllLayerCoords(q_xslice, q_yslice, q_zslice);
}

int VBView::NewView(const QString &str) {
  // FIXME handle orient

  if (str == "XYZ") {
    colbox->hide();
    everynbox->hide();
    BuildViews(vbv_axial);
  } else if (str == "Axial") {
    colbox->hide();
    everynbox->hide();
    BuildViews(vbv_axial);
  } else if (str == "XZY") {
    colbox->hide();
    everynbox->hide();
    BuildViews(vbv_coronal);
  } else if (str == "YZX") {
    colbox->hide();
    everynbox->hide();
    BuildViews(vbv_sagittal);
  } else if (str == "Multi-View") {
    colbox->show();
    everynbox->show();
    BuildViews(vbv_multi);
  } else if (str == "Tri-View") {
    colbox->hide();
    everynbox->hide();
    BuildViews(vbv_tri);
  } else
    cout << "argh!" << endl;
  RenderAll();

  return 0;
}

int VBView::NewMag(const QString &str) {
  q_mag = strtol(xstripwhitespace(str.latin1(), "\t\n\r %"));
  UpdateScaling();
  return 0;
}

int VBView::NewRadius(int radius) {
  q_radius = radius;
  return 0;
}

int VBView::NewZRadius(int state) {
  if (state)
    q_usezradius = 1;
  else
    q_usezradius = 0;
  return 0;
}

int VBView::ToggleData(bool) {
  // FIXME do something!
  RenderAll();
  return (0);  // no error!
}

int VBView::SavePNG() {
  QString s =
      Q3FileDialog::getSaveFileName(".", "All (*.*)", this, "save image file",
                                    "Choose a filename for your snapshot");
  if (s == QString::null) return (0);

  QPixmap::grabWidget(drawarea).save(s.latin1(), "PNG");

  return (0);  // no error!
}

int VBView::ShowCrosshairs(bool state) {
  if (state)
    q_crosshairs = 1;
  else
    q_crosshairs = 0;
  RenderAll();
  // update();
  return 0;
}

int VBView::SetDrawmask(bool state) {
  if (state)
    q_drawmask = 1;
  else
    q_drawmask = 0;
  return 0;
}

int VBView::FlipH(bool state) {
  if (state)
    q_fliph = 1;
  else
    q_fliph = 0;
  RenderAll();
  // update();
  return 0;
}

int VBView::FlipV(bool state) {
  if (state)
    q_flipv = 1;
  else
    q_flipv = 0;
  RenderAll();
  // update();
  return 0;
}

int VBView::ShowOrigin(bool state) {
  if (state)
    q_showorigin = 1;
  else
    q_showorigin = 0;
  RenderAll();
  // update();
  return 0;
}

int VBView::ToggleScaling(bool state) {
  if (layers.size() == 0) return 1;
  if (layers.front().cube.voxsize[0] < FLT_MIN ||
      layers.front().cube.voxsize[1] < FLT_MIN ||
      layers.front().cube.voxsize[2] < FLT_MIN)
    return 0;
  q_isotropic = state;
  UpdateScaling();
  return 0;
}

void VBView::UpdateScaling() {
  if (layers.size() == 0) return;
  q_xscale = q_yscale = q_zscale = (float)q_mag / 100.0;

  if (q_isotropic) {
    float minsize = layers.front().cube.voxsize[0];
    if (layers.front().cube.voxsize[1] < minsize)
      minsize = layers.front().cube.voxsize[1];
    if (layers.front().cube.voxsize[2] < minsize)
      minsize = layers.front().cube.voxsize[2];
    q_xscale *= layers.front().cube.voxsize[0] / minsize;
    q_yscale *= layers.front().cube.voxsize[1] / minsize;
    q_zscale *= layers.front().cube.voxsize[2] / minsize;
  }

  BuildViews(q_viewmode);
  RenderAll();
  // update();
}

void VBView::SetTails(bool twotailed) {
  if (currentlayer == layers.end()) return;
  currentlayer->q_twotailed = twotailed;
  if (twotailed)
    q_scale->setscale(currentlayer->q_thresh, currentlayer->q_high,
                      currentlayer->q_negcolor1, currentlayer->q_negcolor2,
                      currentlayer->q_poscolor1, currentlayer->q_poscolor2);
  else
    q_scale->setscale(currentlayer->q_thresh, currentlayer->q_high,
                      currentlayer->q_poscolor1, currentlayer->q_poscolor2);
  currentlayer->render();
  RenderAll();
  UpdateTS();
}

void VBView::SetFlip(bool flipped) {
  if (currentlayer == layers.end()) return;
  currentlayer->q_flip = flipped;
  currentlayer->render();
  RenderAll();
  UpdateTS();
}

void VBView::togglens(bool ns) {
  if (currentlayer == layers.end()) return;
  if (currentlayer->type == VBLayer::vb_stat ||
      currentlayer->type == VBLayer::vb_glm) {
    currentlayer->q_ns = ns;
    currentlayer->render();
  }
  if (ns)
    q_scale2->show();
  else
    q_scale2->hide();
  RenderAll();
  UpdateTS();
}

void VBView::ChangeTSType(int tstype) {
  q_tstype = tstype;
  UpdateTS();
}

void VBView::SetColumns(int newcols) {
  q_columns = newcols;
  if (viewlist.size() > 2) {
    BuildViews(q_viewmode);
    RenderAll();
  }
}

void VBView::SetEveryn(int n) {
  q_everyn = n;
  if (viewlist.size() > 2) {
    BuildViews(q_viewmode);
    RenderAll();
  }
}

void VBView::HighlightLow() {
  if (!q_update) return;
  lowedit->setPaletteBackgroundColor(qRgb(220, 160, 160));
  if (currentlayer != layers.end()) {
    currentlayer->threshtt = "threshold set manually";
    lowedit->setToolTip("threshold set manually");
  }
}

void VBView::HighlightHigh() {
  if (!q_update) return;
  highedit->setPaletteBackgroundColor(qRgb(220, 160, 160));
}

void VBView::HighlightCluster() {
  if (!q_update) return;
  clusteredit->setPaletteBackgroundColor(qRgb(220, 160, 160));
}

void VBView::SetLow() {
  if (currentlayer == layers.end()) return;
  currentlayer->q_thresh = abs(strtod(lowedit->text().latin1()));
  if (currentlayer->q_high <= currentlayer->q_thresh) {
    currentlayer->q_high = 1.5 * currentlayer->q_thresh;
    highedit->setText(strnum(currentlayer->q_high).c_str());
  }
  lowedit->setPaletteBackgroundColor(qRgb(255, 255, 255));
  lowedit->setText(strnum(currentlayer->q_thresh).c_str());
  currentlayer->render();
  RenderAll();
  UpdateTS();
  q_scale->setscale(currentlayer->q_thresh, currentlayer->q_high,
                    currentlayer->q_negcolor1, currentlayer->q_negcolor2,
                    currentlayer->q_poscolor1, currentlayer->q_poscolor2);
  q_scale2->setscale(0, currentlayer->q_thresh, currentlayer->q_nscolor1,
                     currentlayer->q_nscolor2);
}

void VBView::SetHigh() {
  if (currentlayer == layers.end()) return;
  currentlayer->q_high = abs(strtod(highedit->text().latin1()));
  highedit->setPaletteBackgroundColor(qRgb(255, 255, 255));
  highedit->setText(strnum(currentlayer->q_high).c_str());
  currentlayer->render();
  RenderAll();
  UpdateTS();
  q_scale->setscale(currentlayer->q_thresh, currentlayer->q_high,
                    currentlayer->q_negcolor1, currentlayer->q_negcolor2,
                    currentlayer->q_poscolor1, currentlayer->q_poscolor2);
}

void VBView::SetCluster() {
  if (currentlayer == layers.end()) return;
  currentlayer->q_clustersize = strtol(clusteredit->text().latin1());
  currentlayer->render();
  clusteredit->setPaletteBackgroundColor(qRgb(255, 255, 255));
  clusteredit->setText(strnum(currentlayer->q_clustersize).c_str());
  RenderAll();
  UpdateTS();
}

void VBView::GetThreshold() {
  if (currentlayer == layers.end()) return;
  tcalc tc(currentlayer->threshdata);
  tc.showbuttons();
  int ret = tc.exec();
  if (ret == QDialog::Accepted && finite(tc.getbestthresh())) {
    string tt;
    if (tc.getbestthresh() == tc.getbfthresh())
      tt = "Bonferroni-corrected threshold set by calculator.\n";
    else
      tt = "RFT-corrected threshold set by calculator.\n";
    currentlayer->threshdata = tc.getthreshdata();
    threshold v = currentlayer->threshdata;  // convenience
    if (v.denomdf)
      tt += (format("F(%g,%g), %d comparisons") % v.effdf % v.denomdf %
             v.numVoxels)
                .str();
    else
      tt += (format("t(%g), %d comparisons") % v.effdf % v.numVoxels).str();
    lowedit->setText(strnum(tc.getbestthresh()).c_str());
    currentlayer->threshtt = tt;  // tooltip text is per-layer
    lowedit->setToolTip(tt.c_str());
    SetLow();
  }
}

int VBView::ClearMask() {
  // FIXME create mask layer if none exists
  int ret = QMessageBox::warning(this, "Verify Erase Mask",
                                 "Are you sure you want to erase your mask?",
                                 "Yes", "Cancel", "", 0, 1);
  if (ret == 1) return (0);
  // FIXME find mask layer
  ClearMasks();
  RenderAll();
  UpdateTS();
  return 0;  // no error!
}

void VBView::HandleCorr() {
  VBLayerI ll;
  if (currentlayer->type == VBLayer::vb_corr)
    ll = currentlayer;
  else
    ll = firstCorrLayer();
  // if there isn't an existing correlation layer, load the file first
  if (ll == layers.end()) LoadCorr();
  ll = firstCorrLayer();
  if (ll == layers.end()) return;
  CreateCorrelationMap();
  // ll->renderCorr();
  RenderAll();
}

// FIXME below needs work!

void VBView::ShowRegionStats() {
  // int err;
  // for (MI m=q_masks.begin(); m!=q_masks.end(); m++) {

  // }
  // for (MI m=q_regions.begin(); m!=q_regions.end(); m++) {
  //   if (!m->f_widget->f_visible)
  //     continue;
  //   VB_Vector vv=glmi.getRegionTS(m->f_region,glmi.glmflags);
  //   err=glmi.Regress(vv);
  //   for (int i=0; i<(int)glmi.contrasts.size(); i++) {
  //     glmi.contrast=glmi.contrasts[i];
  //     VB_Vector c=glmi.contrast.contrast;
  //     err=glmi.calc_stat();
  //     if (err)
  //       cout << "error " << err << " calculating stat value" << endl;
  //     else
  //       cout << "region " << m->f_widget->label
  //            << " size " << m->f_region.size()
  //            << " contrast " << glmi.contrast.name
  //            << " stat " << glmi.statval
  //            << endl;
  //   }
  // }
}

void VBView::ClearMasks() {
  // FIXME re-implement
  RenderAll();
  UpdateTS();
}

int VBView::ClearRegions() {
  return 0;
  for (int i = 0; i < (int)q_regions.size(); i++) {
    regionview->removeChild(q_regions[i].f_widget);
    delete q_regions[i].f_widget;
  }
  q_regions.clear();
  q_currentregion = -1;
  regionpos = 0;
  return 0;
}

int VBView::SaveFullMap() {
  if (currentlayer->type != VBLayer::vb_glm &&
      currentlayer->type != VBLayer::vb_stat)
    return 0;
  VBLayerI ll = currentlayer;  // convenience;
  int err;
  QString s = Q3FileDialog::getSaveFileName("Filename for map", "All (*.*)",
                                            this, "save map", "Map filename");
  if (s == QString::null) return 0;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  err = ll->cube.WriteFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (err) {
    QMessageBox::warning(this, "Map File Writing Error", "Couldn't save map.",
                         "Okay");
    return (101);
  }
  return 0;
}

int VBView::SaveVisibleMap() {
  if (currentlayer->type != VBLayer::vb_glm &&
      currentlayer->type != VBLayer::vb_stat)
    return 0;
  VBLayerI ll = currentlayer;  // convenience;
  int err;
  QString s = Q3FileDialog::getSaveFileName("Filename for map", "All (*.*)",
                                            this, "save map", "Map filename");
  if (s == QString::null) return 0;
  Cube tmp = ll->cube;
  tmp.intersect(ll->rendercube);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  err = tmp.WriteFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (err) {
    QMessageBox::warning(this, "Map File Writing Error", "Couldn't save map.",
                         "Okay");
    return (101);
  }
  return 0;
}

int VBView::SaveRegionMask() {
  if (currentlayer->type != VBLayer::vb_glm &&
      currentlayer->type != VBLayer::vb_stat)
    return 0;
  VBLayerI ll = currentlayer;  // convenience;
  int err;
  QString s = Q3FileDialog::getSaveFileName("Filename for map", "All (*.*)",
                                            this, "save map", "Map filename");
  if (s == QString::null) return 0;
  Cube tmp = ll->rendercube;
  tmp.quantize(1);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  err = tmp.WriteFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (err) {
    QMessageBox::warning(this, "Map File Writing Error", "Couldn't save map.",
                         "Okay");
    return (101);
  }
  return 0;
}

void VBView::reload_averages() {
  // clear the old stuff
  trialsets.clear();
  averagebox->clear();
  averagebox->insertItem("No averaging");
  // reload trialsets from GLM or file
  VBLayerI li = currentlayer;  // convenience
  if (li == layers.end() || li->type != VBLayer::vb_glm) li = firstStatLayer();
  if (li == layers.end()) return;
  if (currentlayer->type != VBLayer::vb_glm) return;
  if (li->glmi.stemname.size()) {
    li->glmi.loadtrialsets();
    trialsets = li->glmi.trialsets;
  } else
    trialsets = parseTAFile(averagesfile);
  if (trialsets.size() == 0) return;
  for (int i = 0; i < (int)trialsets.size(); i++)
    averagebox->insertItem(trialsets[i].name.c_str());
  myaverage = (TASpec *)NULL;
}

void VBView::update_status() {
  return;  // FIXME need to do what this function does somewhere
  char tmp[STRINGLEN];
  if (base_dims == 4)
    sprintf(tmp, "%d x %d x %d, %d time points", base_dimx, base_dimy,
            base_dimz, base_dimt);
  else if (base_dims == 3)
    sprintf(tmp, "%d x %d x %d", base_dimx, base_dimy, base_dimz);
  else
    sprintf(tmp, "<none>");

  string statstring;
  VBLayerI ll = layers.begin();
  if (ll != layers.end()) {
    sprintf(tmp, "%d x %d x %d", ll->dimx, ll->dimy, ll->dimz);
    statstring = tmp;
    if (ll->glmi.teslist.size()) {
      if (ll->glmi.glmflags & DETREND) statstring += " DETRENDED";
      if (ll->glmi.glmflags & MEANSCALE) statstring += " MEANSCALED";
    }
  } else
    statstring = "<none>";
}

void VBView::handle_maskchanged(QWidget *o) {
  bool f_colorchange = 0;
  QTMaskWidget *mw = (QTMaskWidget *)o;
  VBMaskSpec &ms = lastmasklayer->cube.maskspecs[mw->index];
  ms.name = mw->label;
  if (ms.r != mw->color.red() || ms.g != mw->color.green() ||
      ms.b != mw->color.blue()) {
    f_colorchange = 1;
    ms.r = mw->color.red();
    ms.g = mw->color.green();
    ms.b = mw->color.blue();
  }
  q_maskcolor = (1 << 24) | (ms.r << 16) | (ms.g << 8) | ms.b;
  q_maskindex = mw->index;
  if (f_colorchange) {
    currentlayer->render();
    RenderAll();
  }
}

void VBView::handle_newmask(QTMaskWidget *mw) {
  VBMaskSpec ms;
  ms.r = mw->color.red();
  ms.g = mw->color.green();
  ms.b = mw->color.blue();
  ms.name = mw->label;
  lastmasklayer->cube.maskspecs[mw->index] = ms;
}

void VBView::handle_maskselected(int i) {
  VBMaskSpec &ms = lastmasklayer->cube.maskspecs[i];
  q_maskcolor = (1 << 24) | (ms.r << 16) | (ms.g << 8) | ms.b;
  q_maskindex = i;
}

void VBView::handle_maskcopied(int ind) {
  Cube &m = lastmasklayer->cube;
  if (!q_mask.dimsequal(m))
    q_mask.SetVolume(m.dimx, m.dimy, m.dimz, vb_byte);
  else
    q_mask.zero();
  for (int i = 0; i < m.dimx * m.dimy * m.dimz; i++) {
    if (m.getValue(i) == ind) q_mask.setValue(i, 1);
  }
}

void VBView::handle_inforequested(int i) {
  return;
  VBMaskSpec &ms = lastmasklayer->cube.maskspecs[i];
  cout << ms.name << endl;
  cout << ms.r << endl;
  cout << ms.g << endl;
  cout << ms.b << endl;
}

void VBView::ByteSwap() {
  if (currentlayer == layers.end()) return;
  if (currentlayer->cube) currentlayer->cube.byteswap();
  if (currentlayer->tes) currentlayer->tes.byteswap();
  currentlayer->render();
  RenderAll();
}

void VBView::LoadAverages() {
  QString s;
  s = Q3FileDialog::getOpenFileName(
      q_currentdir.c_str(), "All (*.*)", this, "open averages file",
      "Choose the file that contains your averages");
  if (s == QString::null) return;

  averagesfile = s.latin1();
  reload_averages();
}

int VBView::ChangeMask() { return 0; }

void VBView::handle_savemask() {
  if (currentlayer == layers.end()) return;
  if (currentlayer->type != VBLayer::vb_mask) {
    QMessageBox::warning(this, "Mask File Writing Error",
                         "No mask currently loaded.", "Okay");
    return;
  }
  // FIXME should see if the mask already has a name
  QString s = Q3FileDialog::getSaveFileName(".", "All (*.*)", this, "save mask",
                                            "Choose a filename for your mask");
  if (s == QString::null) return;
  currentlayer->cube.SetFileName(s.latin1());
  if (currentlayer->cube.WriteFile()) {
    QMessageBox::warning(this, "Mask File Writing Error",
                         "Couldn't save mask for unknown reason.", "Okay");
    return;
  }
  currentlayer->q_dirty = 0;
}

int VBView::SaveImage() {
  if (!base_dims) {
    QMessageBox::warning(this, "No image currently loaded.",
                         "No image currently loaded.");
    return (100);
  }
  // FIXME save base image
  //   if (!q_filename.size()) {
  //     QString s=Q3FileDialog::getSaveFileName(".","All (*.*)",this,"save
  //     image","Choose a filename for your image"); if (s==QString::null)
  //     return (0); cube.SetFileName(s.latin1());
  //   }
  //   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  //   if (cube.WriteFile()) {
  //     QMessageBox::warning(this,"Image File Writing Error","Couldn't save
  //     image for unknown reason.","Okay");
  //     QApplication::restoreOverrideCursor();
  //     return (101);
  //   }
  //   QApplication::restoreOverrideCursor();
  //   imagenamebox->setText(cube.GetFileName().c_str());
  return (0);  // no error!
}

int VBView::Cut() {
  if (currentlayer == layers.end()) return 1;
  currentlayer->mask.invalidate();
  currentlayer->render();
  updateLayerTable();
  RenderAll();
  UpdateTS();
  return (0);  // no error!
}

int VBView::Copy() {
  if (currentlayer == layers.end()) return 1;
  Cube &rc = currentlayer->rendercube;
  if (!q_mask.dimsequal(rc))
    q_mask.SetVolume(rc.dimx, rc.dimy, rc.dimz, vb_byte);
  else
    q_mask.zero();
  for (int i = 0; i < rc.dimx * rc.dimy * rc.dimz; i++) {
    if (rc.testValue(i)) q_mask.setValue(i, 1);
  }
  return (0);  // no error!
}

int VBView::Paste() {
  if (currentlayer == layers.end()) return 1;
  Cube &rc = currentlayer->rendercube;
  Cube &rm = currentlayer->mask;
  if (!q_mask.dimsequal(rc)) {
    QMessageBox::warning(
        this, "Dimension mis-match",
        "The mask you copied and your current layer have different dimensions.",
        "Okay");
    return 1;
  }
  rm = q_mask;
  for (int i = 0; i < rc.dimx * rc.dimy * rc.dimz; i++) {
    if (!(rm.testValue(i))) rc.setValue(i, 0);
  }
  updateLayerTable();
  RenderAll();
  UpdateTS();
  return 0;  // no error!
}

int VBView::ApplyMask() {
  // apply mask from previously selected layer to current stat map
  // must have a previous layer
  if (previouslayer == currentlayer || previouslayer == layers.end()) return 1;
  // current layer must be a stat map
  if (currentlayer->type != VBLayer::vb_glm &&
      currentlayer->type != VBLayer::vb_stat)
    return 1;
  Cube *pc = &(previouslayer->rendercube);
  Cube *cc = &(currentlayer->rendercube);
  if (pc->dimx != cc->dimx || pc->dimy != cc->dimy || pc->dimz != cc->dimz)
    return 1;
  for (int i = 0; i < pc->dimx; i++) {
    for (int j = 0; j < pc->dimy; j++) {
      for (int k = 0; k < pc->dimz; k++) {
        if (!(pc->testValue(i, j, k))) cc->SetValue(i, j, k, 0);
      }
    }
  }
  RenderAll();
  UpdateTS();
  return 0;
}

int VBView::FindAllRegions() {
  if (currentlayer == layers.end()) return 1;
  if (currentlayer->type != VBLayer::vb_glm &&
      currentlayer->type != VBLayer::vb_stat)
    return 0;
  // re-rendering the layer will cause all regions to be displayed
  currentlayer->render();
  RenderAll();
  UpdateTS();
  return 0;
}

int VBView::FindCurrentRegion() {
  if (currentlayer == layers.end()) return 1;
  // first, get thecurrent voxel coord in this layer
  VBMatrix coord(4, 1);
  coord.set(0, 0, q_xslice);
  coord.set(1, 0, q_yslice);
  coord.set(2, 0, q_zslice);
  coord.set(3, 0, 1);
  coord ^= currentlayer->transform;
  // set up stuff for the growregion
  Cube *rc = &(currentlayer->rendercube);
  Cube mask;
  mask.SetVolume(rc->dimx, rc->dimy, rc->dimz, vb_byte);
  mask += 1;
  int32 xx = (int32)coord(0, 0);
  int32 yy = (int32)coord(1, 0);
  int32 zz = (int32)coord(2, 0);
  // for mask, find equal voxels
  if (currentlayer->type == VBLayer::vb_mask) {
    // find the region of equal voxels starting there
    double val = rc->getValue<int32>(xx, yy, zz);
    VBRegion rr =
        growregion(xx, yy, zz, currentlayer->rendercube, mask, vb_eq, val);
    // for all voxels, if it's not in the region, zero it out
    for (int i = 0; i < mask.dimx; i++) {
      for (int j = 0; j < mask.dimy; j++) {
        for (int k = 0; k < mask.dimz; k++) {
          if (!(rr.contains(i, j, k)))
            currentlayer->rendercube.SetValue(i, j, k, 0);
        }
      }
    }
    RenderAll();
    UpdateTS();
  }
  if (currentlayer->type == VBLayer::vb_glm ||
      currentlayer->type == VBLayer::vb_stat) {
    // find the region of nonzero voxels starting there
    VBRegion rr =
        growregion((int32)coord(0, 0), (int32)coord(1, 0), (int32)coord(2, 0),
                   currentlayer->rendercube, mask, vb_agt, 0);
    // for all voxels, if it's not in the region, zero it out
    for (int i = 0; i < mask.dimx; i++) {
      for (int j = 0; j < mask.dimy; j++) {
        for (int k = 0; k < mask.dimz; k++) {
          if (!(rr.contains(i, j, k)))
            currentlayer->rendercube.SetValue(i, j, k, 0);
        }
      }
    }
    RenderAll();
    UpdateTS();
  }
  return 0;
}

void VBView::CreateCorrelationMap() {
  VBLayerI cl;
  if (currentlayer->type == VBLayer::vb_corr)
    cl = currentlayer;
  else
    cl = firstCorrLayer();
  if (cl == layers.end()) return;
  VBMatrix coord(4, 1);
  coord.set(0, 0, q_xslice);
  coord.set(1, 0, q_yslice);
  coord.set(2, 0, q_zslice);
  coord.set(3, 0, 1);
  coord ^= cl->transform;
  int32 sx = coord(0, 0);
  int32 sy = coord(1, 0);
  int32 sz = coord(2, 0);
  // just in case...
  if (sx < 0 || sx > cl->dimx - 1 || sy < 0 || sy > cl->dimy - 1 || sz < 0 ||
      sz > cl->dimz - 1)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  VB_Vector ref, current;
  cl->tes.GetTimeSeries(sx, sy, sz);
  ref = cl->tes.timeseries;
  ref -= ref.getVectorMean();
  for (int32 i = 0; i < cl->dimx; i++) {
    for (int32 j = 0; j < cl->dimy; j++) {
      for (int32 k = 0; k < cl->dimz; k++) {
        if (!cl->tes.VoxelStored(i, j, k)) {
          cl->cube.SetValue(i, j, k, 0);
          continue;
        }
        cl->tes.GetTimeSeries(i, j, k);
        current = cl->tes.timeseries;
        current -= current.getVectorMean();
        cl->cube.SetValue(i, j, k, correlation(ref, current));
      }
    }
  }
  // panel_stats->show();

  q_update = 0;
  lowedit->setText("0.0");
  highedit->setText("1.0");
  cl->q_thresh = 0.0;
  cl->q_high = 1.0;
  // FIXME
  // q_scale->setscale(0.0,1.0,q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
  q_update = 1;

  // char tmp[STRINGLEN];
  // sprintf(tmp,"<correlation map: %d,%d,%d>",sx,sy,sz);
  cl->render();
  RenderAll();
  QApplication::restoreOverrideCursor();
}

void VBView::moveLayer(string dir) {
  if (currentlayer == layers.end()) return;
  VBLayerI dest;
  if (dir == "up") {
    dest = currentlayer;
    if (dest != layers.begin()) dest--;
    layers.splice(dest, layers, currentlayer);
  } else {
    dest = currentlayer;
    if (dest != layers.end()) dest++;
    if (dest != layers.end()) dest++;
    layers.splice(dest, layers, currentlayer);
  }
  updateLayerTable();
  RenderAll();
}

int VBView::Close() {
  // FIXME check dirty flag on all layers
  int ret = 0;
  if (0)
    ret = QMessageBox::warning(this, "Unsaved Mask",
                               "Your mask hasn't been saved.  Exit anyway?",
                               "Yes", "No", "", 0, 1);
  if (ret == 0) emit closeme(this);
  return (0);  // no error!
}

void VBView::wheelEvent(QWheelEvent *we) {
  if (we->orientation() != Qt::Vertical) return;
  if (q_viewmode == vbv_axial) {
    if (we->delta() > 0) zsliceslider->setValue(q_zslice + 1);
    if (we->delta() < 0) zsliceslider->setValue(q_zslice - 1);
  } else if (q_viewmode == vbv_coronal) {
    if (we->delta() > 0) ysliceslider->setValue(q_yslice + 1);
    if (we->delta() < 0) ysliceslider->setValue(q_yslice - 1);
  } else if (q_viewmode == vbv_sagittal) {
    if (we->delta() > 0) xsliceslider->setValue(q_xslice + 1);
    if (we->delta() < 0) xsliceslider->setValue(q_xslice - 1);
  }
}

void VBView::keyPressEvent(QKeyEvent *ke) {
  if (!base_dims) {
    QWidget::keyPressEvent(ke);
    return;
  }
  // ke->key() = Qt::Key_Down (Up,Left,Right)
  // Qt::Key_Enter, Return, Tab, Backspace, Escape, Delete, Home, End, F1..
  if (ke->ascii() == 'a' && base_dims == 4)
    tsliceslider->setValue(q_volume - 1);
  else if (ke->ascii() == 's' && base_dims == 4)
    tsliceslider->setValue(q_volume + 1);
  else if (ke->ascii() == 'z' || ke->key() == Qt::Key_Down) {
    if (q_viewmode == vbv_axial)
      zsliceslider->setValue(q_zslice - 1);
    else if (q_viewmode == vbv_coronal)
      ysliceslider->setValue(q_yslice - 1);
    else if (q_viewmode == vbv_sagittal)
      xsliceslider->setValue(q_xslice - 1);
  } else if (ke->ascii() == 'x' || ke->key() == Qt::Key_Up) {
    if (q_viewmode == vbv_axial)
      zsliceslider->setValue(q_zslice + 1);
    else if (q_viewmode == vbv_coronal)
      ysliceslider->setValue(q_yslice + 1);
    else if (q_viewmode == vbv_sagittal)
      xsliceslider->setValue(q_xslice + 1);
  } else if (ke->key() == Qt::Key_Home || ke->ascii() == 'o') {
    xsliceslider->setValue(base_origin[0]);
    ysliceslider->setValue(base_origin[1]);
    zsliceslider->setValue(base_origin[2]);
  } else if (ke->key() == Qt::Key_Delete)
    Cut();
  else if (ke->key() == Qt::Key_U)
    moveLayer("up");
  else if (ke->key() == Qt::Key_D)
    moveLayer("down");
  else if (ke->key() == Qt::Key_Space) {
    ShowCrosshairs(!q_crosshairs);
    ShowOrigin(
        q_crosshairs);  // so that if they were out of sync, now they're not
  } else if (ke->key() == '-' || ke->key() == '_') {
    if (q_mag > 109)
      q_mag -= 10;
    else if (q_mag > 14)
      q_mag -= 5;
    q_mag -= 10;
    UpdateScaling();
  } else if (ke->key() == '=' || ke->key() == '+') {
    if (q_mag < 95)
      q_mag += 5;
    else
      q_mag += 10;
    UpdateScaling();
  } else if (ke->key() == Qt::Key_F1) {
    NewView("XYZ");
  } else if (ke->key() == Qt::Key_F2) {
    NewView("XZY");
  } else if (ke->key() == Qt::Key_F3) {
    NewView("YZX");
  } else if (ke->key() == Qt::Key_F4) {
    NewView("Tri-View");
  } else if (ke->key() == Qt::Key_F5) {
    NewView("Multi-View");
  }
  // else if (ke->key()==Qt::Key_S && (ke->state() & Qt::CTRL))
  //   handle_savemask();   // FIXME mask?  or png?
  else
    QWidget::keyPressEvent(ke);
  // FIXME space to toggle current mask/region?
  return;
}

void VBView::PopMask() {
  if (currentlayer->type != VBLayer::vb_mask) return;
  if (currentlayer->undo.size() == 0) return;
  VBRegion *r = &(currentlayer->undo.front());
  for (VI vox = r->begin(); vox != r->end(); vox++) {
    // vox->second.print();
    currentlayer->cube.SetValue(vox->second.x, vox->second.y, vox->second.z,
                                vox->second.val);
  }
  currentlayer->render();
  currentlayer->undo.pop_front();
  RenderAll();
}

int VBView::LeftCanvas() {
  if (q_atmouse) {
    for (int row = 0; row < layertable->rowCount(); row++) {
      layertable->item(row, C_X)->setText("");
      layertable->item(row, C_X)->setToolTip("");
      layertable->item(row, C_Y)->setText("");
      layertable->item(row, C_Y)->setToolTip("");
      layertable->item(row, C_Z)->setText("");
      layertable->item(row, C_Z)->setToolTip("");
      layertable->item(row, C_VAL)->setText("");
      layertable->item(row, C_VAL)->setToolTip("");
      layertable->item(row, C_LABEL)->setText("");
      layertable->item(row, C_LABEL)->setToolTip("");
    }
    if (!ts_maskbox->isChecked()) UpdateTS(1);
  }
  return 0;
}

int VBView::MousePressEvent(QMouseEvent me) {
  setFocus();
  if (!layers.size()) return 0;
  double x, y, z;
  if (win2base(me.x(), me.y(), x, y, z)) return 0;

  if (me.button() == Qt::LeftButton) {
    effectivemode = mode;
    if (mode == vbv_mask &&
        me.buttons() & (Qt::ShiftButton | Qt::ControlButton))
      effectivemode = vbv_fill;
  } else {
    if (me.button() == Qt::MidButton)
      effectivemode = vbv_cross;
    else if (me.button() == Qt::RightButton)
      effectivemode = vbv_erase;
  }
  // FIXME all the above is moot, we're forcing mode to cross
  effectivemode = vbv_cross;
  int xx = (int)(x);
  int yy = (int)(y);
  int zz = (int)(z);

  // MASK DRAWING
  if (me.button() & (Qt::LeftButton | Qt::RightButton) && q_drawmask &&
      currentlayer->type == VBLayer::vb_mask) {
    // we have initiated a mask draw, let's push back the undo stuff
    // and start a new undo region
    if (currentlayer->undo.size() > 100) currentlayer->undo.pop_back();
    VBRegion rr;
    currentlayer->undo.push_front(rr);
    colormask(*currentview, VBVoxel(me.x(), me.y(), 0), currentlayer,
              (me.buttons() & Qt::LeftButton ? q_maskcolor : 0));
    return 0;
  }
  if (me.button() & (Qt::MidButton) && q_drawmask &&
      currentlayer->type == VBLayer::vb_mask) {
    // we have initiated a mask draw, let's push back the undo stuff
    // and start a new undo region
    if (currentlayer->undo.size() > 100) currentlayer->undo.pop_back();
    VBRegion rr;
    currentlayer->undo.push_front(rr);
    fillmask(*currentview, VBVoxel(me.x(), me.y(), 0), currentlayer,
             q_maskcolor);
    return 0;
  }

  if (effectivemode == vbv_cross) {
    q_update = 0;
    xsliceslider->setValue(xx);
    ysliceslider->setValue(yy);
    zsliceslider->setValue(zz);
    q_update = 1;
    RenderAll();
    if (q_tstype == tsmode_cross) UpdateTS();
    setAllLayerCoords(x, y, z);
    return 0;
  }

  // FIXME handle fill more elegantly than this!
  if (effectivemode == vbv_fill) {
    //     if (q_masks[q_currentmask].f_index==GetMaskValue(mx,my,mz))
    //       return 0;
    // FillPixel(mx,my,mz,(int)(GetMaskValue(mx,my,mz)));
    RenderAll();
    return 0;
  }

  if (effectivemode == vbv_erase) q_maskmode = MASKMODE_ERASE;
  q_maskmode = MASKMODE_DRAW;
  if (q_tstype == tsmode_mask) UpdateTS();
  return (0);
}

int VBView::MouseMoveEvent(QMouseEvent me) {
  if (!layers.size()) return 0;
  double x, y, z;
  if (win2base(me.x(), me.y(), x, y, z)) {
    LeftCanvas();
    return 0;
  }
  if (q_atmouse) setAllLayerCoords(x, y, z);

  // MASK DRAWING
  if (me.buttons() & (Qt::LeftButton | Qt::RightButton) && q_drawmask &&
      currentlayer->type == VBLayer::vb_mask) {
    colormask(*currentview, VBVoxel(me.x(), me.y(), 0), currentlayer,
              (me.buttons() & Qt::LeftButton ? q_maskcolor : 0));
    return 0;
  }

  // FIXME otherwise, for now, when any mouse button is down, we're moving the
  // cross
  if (me.buttons() & Qt::LeftButton) {
    q_update = 0;
    int xx = (int)(x);
    int yy = (int)(y);
    int zz = (int)(z);
    xsliceslider->setValue(xx);
    ysliceslider->setValue(yy);
    zsliceslider->setValue(zz);
    q_update = 1;
    RenderAll();
    if (q_tstype == tsmode_cross) UpdateTS();
    setAllLayerCoords(x, y, z);
  }
  return 0;

  if (me.buttons() & Qt::LeftButton)
    effectivemode = mode;
  else {
    if (me.buttons() & Qt::MidButton)
      effectivemode = vbv_cross;
    else if (me.buttons() & Qt::RightButton)
      effectivemode = vbv_erase;
    else
      effectivemode = vbv_none;
  }

  if (!base_dims) return (0);
  if (effectivemode == vbv_cross) {
    q_update = 0;
    xsliceslider->setValue(me.x());
    ysliceslider->setValue(me.y());
    zsliceslider->setValue(0);
    q_update = 1;
    RenderAll();
    if (q_tstype == tsmode_cross) UpdateTS();
    return 0;
  }
  // if (!sview->hasFocus())
  // sview->setFocus();

  if (effectivemode == vbv_mask)
    q_maskmode = MASKMODE_DRAW;
  else if (effectivemode == vbv_erase)
    q_maskmode = MASKMODE_ERASE;
  else
    return (0);

  q_maskmode = MASKMODE_DRAW;
  if (q_tstype == tsmode_mask) UpdateTS();
  return (0);
}

// win2base() takes the x/y position of the mouse click or whatever,
// and returns the base layer coordinate in double precision.  can be
// rounded or subsequently converted to another layer's space using
// the transform "transform" (very clever name)

int VBView::win2base(int x, int y, double &xx, double &yy, double &zz) {
  // find the view (x and y are within xoff and xoff+width)
  vector<MyView>::iterator view;
  for (view = viewlist.begin(); view != viewlist.end(); view++) {
    if (x >= view->xoff && x < view->xoff + view->width && y >= view->yoff &&
        y < view->yoff + view->height)
      break;
  }
  currentview = view;  // for later use
  // not in any view?
  if (view == viewlist.end()) return 101;
  VBMatrix coord(4, 1);
  VBMatrix newcoord;
  int pos = view->position;
  if (view->position == -1) {
    if (view->orient == MyView::vb_xy)
      pos = q_zslice;
    else if (view->orient == MyView::vb_yz)
      pos = q_xslice;
    else if (view->orient == MyView::vb_xz)
      pos = q_yslice;
  }
  newcoord = layers.begin()->calcFullTransform(
      q_xscale, q_yscale, q_zscale, view->xoff, view->yoff, view->width,
      view->height, view->orient, pos, q_fliph, q_flipv);
  coord.set(0, 0, x);
  coord.set(1, 0, y);
  coord.set(2, 0, 0);
  coord.set(3, 0, 1);
  newcoord *= coord;
  xx = newcoord(0, 0);
  yy = newcoord(1, 0);
  zz = newcoord(2, 0);
  return 0;
}

// setAllLayerCoords() should actually do everything we need to do
// when our coord changes.  should rename

void VBView::setAllLayerCoords(double x, double y, double z) {
  int row = 0;
  for (VBLayerI li = layers.begin(); li != layers.end(); li++) {
    VBMatrix coord(4, 1);
    coord.set(0, 0, x);
    coord.set(1, 0, y);
    coord.set(2, 0, z);
    coord.set(3, 0, 1);
    coord ^= li->transform;
    double xx = coord(0, 0);
    double yy = coord(1, 0);
    double zz = coord(2, 0);
    if (q_fromorigin) {
      xx -= li->cube.origin[0];
      yy -= li->cube.origin[1];
      zz -= li->cube.origin[2];
    }
    if (q_inmm) {
      xx *= li->cube.voxsize[0];
      yy *= li->cube.voxsize[1];
      zz *= li->cube.voxsize[2];
    }
    li->x = (int32)(xx);
    li->y = (int32)(yy);
    li->z = (int32)(zz);
    layertable->item(row, C_X)->setText(strnum(li->x).c_str());
    layertable->item(row, C_Y)->setText(strnum(li->y).c_str());
    layertable->item(row, C_Z)->setText(strnum(li->z).c_str());
    layertable->item(row, C_X)->setToolTip(strnum(li->x).c_str());
    layertable->item(row, C_Y)->setToolTip(strnum(li->y).c_str());
    layertable->item(row, C_Z)->setToolTip(strnum(li->z).c_str());
    if (li->cube) {
      double val = li->cube.GetValue(li->x, li->y, li->z);
      layertable->item(row, C_VAL)->setText(strnum(val).c_str());
      layertable->item(row, C_VAL)->setToolTip(strnum(val).c_str());
      if (li->type == VBLayer::vb_mask)
        if (li->cube.maskspecs.count((uint32)val))
          layertable->item(row, C_LABEL)
              ->setText(li->cube.maskspecs[(uint32)val].name.c_str());
    }
    row++;
  }
  UpdateTS();
}

void VBView::FillPixel(int, int, int, int) {
  // q_update=0;
  // if (q_currentview->orient==MyView::vb_xy)
  //   FillPixelXY(x,y,z,fromval);
  // else if (q_currentview->orient==MyView::vb_yz)
  //   FillPixelYZ(x,y,z,fromval);
  // else if (q_currentview->orient==MyView::vb_xz)
  //   FillPixelXZ(x,y,z,fromval);
  // q_update=1;
}

void VBView::FillPixelXY(int, int, int, int) {}

void VBView::FillPixelYZ(int, int, int, int) {}

void VBView::FillPixelXZ(int, int, int, int) {}

void VBView::ToggleDisplayPanel() {
  // FIXME hide stacked widget
}

void VBView::ToggleTSPanel() {
  if (ts_window->isHidden())
    ts_window->show();
  else
    ts_window->hide();
}

void VBView::ToggleOverlays() {
  if (currentlayer == layers.end() || previouslayer == layers.end()) return;
  if (currentlayer == previouslayer) return;
  if (currentlayer == layers.begin() || previouslayer == layers.begin()) return;
  int crow = -1, prow = -1, row = 0;
  VBLayerI ll = layers.begin();
  while (ll != layers.end()) {
    if (ll == currentlayer) crow = row;
    if (ll == previouslayer) prow = row;
    ll++;
    row++;
  }
  if (crow == -1 || prow == -1) return;  // shouldn't happen, but...
  if (currentlayer->q_visible) {
    currentlayer->q_visible = 0;
    QTableWidgetItem *twi = new QTableWidgetItem("");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    layertable->setItem(crow, C_VIS, twi);
    previouslayer->q_visible = 1;
    twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/visible.png")), "");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    layertable->setItem(prow, C_VIS, twi);
  } else {
    previouslayer->q_visible = 0;
    QTableWidgetItem *twi = new QTableWidgetItem("");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    layertable->setItem(prow, C_VIS, twi);
    currentlayer->q_visible = 1;
    twi = new QTableWidgetItem(QIcon(QPixmap(":/icons/visible.png")), "");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    layertable->setItem(crow, C_VIS, twi);
  }
  RenderAll();
}

void VBView::ToggleHeaderPanel() {
  //   if (panel_header->isHidden())
  //     panel_header->show();
  //   else
  //     panel_header->hide();
}

VBLayerI VBView::baseLayer() { return layers.begin(); }

VBLayerI VBView::firstStatLayer() {
  VBLayerI ll;
  for (ll = layers.begin(); ll != layers.end(); ll++)
    if (ll->type == VBLayer::vb_stat || ll->type == VBLayer::vb_glm) break;
  return ll;
}

VBLayerI VBView::firstMaskLayer() {
  VBLayerI ll;
  for (ll = layers.begin(); ll != layers.end(); ll++)
    if (ll->type == VBLayer::vb_mask) break;
  return ll;
}

VBLayerI VBView::firstCorrLayer() {
  VBLayerI ll;
  for (ll = layers.begin(); ll != layers.end(); ll++)
    if (ll->type == VBLayer::vb_corr) break;
  return ll;
}
