
// vbview.cpp
// an image viewing widget
// Copyright (c) 1998-2007 by The VoxBo Development Team

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

#include "vbview.h"
#include "fileview.h"
#include "vbcontrast.h"
#include "qprinter.h"
#include "qsimplerichtext.h"
#include "qpaintdevicemetrics.h"
#include "qinputdialog.h"
#include "makestatcub.h"
#include "qtooltip.h"

#include "b_mask.xpm"
#include "b_fill.xpm"
#include "b_erase.xpm"
#include "b_cross.xpm"
#include "b_origin.xpm"

// A VBView must have a cube and a mask
// may have a tes
// may have an overlay
// when the cube is loaded we calculate max and min values, etc.

/*
void VBView::show()
{
  if (displayvisible) displaydockwindow->show();
  if (masksvisible) masksdockwindow->show();
  if (statsvisible) statsdockwindow->show();

  QWidget::show();
}

void VBView::hide()
{
  displaydockwindow->hide();
  masksdockwindow->hide();
  statsdockwindow->hide();
  
  QWidget::hide();
}
*/

// sole constructor for vbview

VBView::VBView(QWidget *parent,const char *name)
  : QWidget(parent,name)
{
  init();

  QPushButton *button;
  QHBox *hb;
  QWidget *ww;
  QLabel *lab;

  // This viewlayout only has one child widget, the mainsplitter.  I
  // don't even know why it exists.
  QBoxLayout *viewlayout = new QHBoxLayout( this );
  
  // This mainsplitter is the the vertical (stacked) splitter that holds the top
  // boxes, the plot widget, and notes.
  mainsplitter = new QSplitter( this, "main splitter" );
  mainsplitter->setOrientation(QSplitter::Vertical);
  mainsplitter->setOpaqueResize();
  mainsplitter->setChildrenCollapsible(false);
  viewlayout->addWidget(mainsplitter);
  
  // mainbox is the widget at the top which is the parent of the brain image
  // widget as well as the file information.  The file info is not in mainsplitter
  // because it should not be resizeable
  QWidget *mainbox = new QWidget( mainsplitter, "main box");
  QVBoxLayout *mainlayout = new QVBoxLayout( mainbox, 4, 4, "main layout" );
  mainlayout->setMargin(4);
  mainlayout->setSpacing(4);

  ///////////////////////////////////////
  // TOP -- the brain image(s)
  ///////////////////////////////////////

  // topsplitter is the widget at the top which is the parent of the brain image
  // widget as well as the three control widget panels.
//  topsplitter = new QSplitter( mainbox, "top splitter" );
//  topsplitter->setOpaqueResize();
//  topsplitter->setChildrenCollapsible(false);
//  mainlayout->addWidget(topsplitter, 1);

  // topbox is the widget at the top which is the parent of the brain image
  // widget as well as the three control widget panels.  It has a horizontal
  // (side-by-side) layout.
  topbox = new QWidget( mainbox, "top box" );
  mainlayout->addWidget(topbox, 1);
  toplayout = new QHBoxLayout( topbox, 4, 4, "top layout" );

  // sview is the scroll view for the brain image widget.
//  sview=new QScrollView(topsplitter);
  sview = new QScrollView(topbox);
  sview->setPaletteBackgroundColor(q_bgcolor);
  sview->viewport()->setPaletteBackgroundColor(q_bgcolor);
  sview->setMinimumSize(260,260);
  sview->resize(260,260);
  toplayout->addWidget(sview);

  // bbox is a widget created for housing the drawarea VBCanvas (brain image
  // widget).  sview is the parent widget of bbox.
  toplayout->setStretchFactor(sview,1);
  QVBox *bbox=new QVBox( this, "b box" );
  sview->addChild(bbox);
  drawarea=new VBCanvas(bbox);
  drawarea->SetImage(&currentimage);

  QObject::connect(drawarea,SIGNAL(mousemove(QMouseEvent)),this,SLOT(MouseMoveEvent(QMouseEvent)));
  QObject::connect(drawarea,SIGNAL(mousepress(QMouseEvent)),this,SLOT(MousePressEvent(QMouseEvent)));

  ///////////////////////////////////////
  // PANEL - ToolButton Panel
  ///////////////////////////////////////
  panel_tools=new QVBox(topbox);
  toplayout->addWidget(panel_tools);

  QToolButton *tb;

  tb=new QToolButton(panel_tools);
  tb->setIconSet(QIconSet(QPixmap((const char**)b_cross_xpm)));
  tb->setToggleButton(1);
  tb->setOn(0);
  QToolTip::add(tb,"toggle crosshairs");
  QObject::connect(tb,SIGNAL(clicked()),this,SLOT(ToggleCrosshairs()));

  tb=new QToolButton(panel_tools);
  tb->setIconSet(QIconSet(QPixmap((const char**)b_origin_xpm)));
  tb->setToggleButton(1);
  tb->setOn(0);
  QToolTip::add(tb,"show and go to origin");
  QObject::connect(tb,SIGNAL(clicked()),this,SLOT(ToggleOrigin()));

  // spacer
  ww=new QWidget(panel_tools);
  ww->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);

  ///////////////////////////////////////
  // PANEL - Image Info Panel
  ///////////////////////////////////////
  panel_display=new QVGroupBox("Display Options",topbox);
  toplayout->addWidget(panel_display);
  panel_display->setFixedWidth(260);
//  panel_display->setSpacing(4);

  // VIEW, MAG, ISOTROPIC
  hb=new QHBox(panel_display);
  viewbox=new QComboBox(FALSE,hb,"viewbox");
  magbox=new QComboBox(TRUE,hb,"magbox");
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
  magbox->setCurrentItem(5);
  magbox->setInsertionPolicy(QComboBox::NoInsertion);
  viewbox->insertItem("XYZ");
  viewbox->insertItem("Axial");
  viewbox->insertItem("Coronal");
  viewbox->insertItem("Sagittal");
  viewbox->insertItem("Tri-View");
  viewbox->insertItem("Multi-View");
  viewbox->setCurrentItem(4);
  QObject::connect(viewbox,SIGNAL(activated(const QString&)),this,SLOT(NewView(const QString&)));
  QObject::connect(magbox,SIGNAL(activated(const QString&)),this,SLOT(NewMag(const QString&)));


  // columns for multiview
  hb=new QHBox(panel_display);
  colbox=new QSpinBox(1,100,1,hb);
  lab=new QLabel("columns",hb); 
  colbox->setValue(5);
  colbox->setFixedWidth(35);
  //ww=new QWidget(hb);
  //ww->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);
  QObject::connect(colbox,SIGNAL(valueChanged(int)),this,SLOT(SetColumns(int)));

  // isotropic checkbox
  QCheckBox *scalebox=new QCheckBox("isotropic scaling",hb);
  scalebox->setChecked(0);
  QObject::connect(scalebox,SIGNAL(toggled(bool)),this,SLOT(ToggleScaling(bool)));

  // image and mask data labels
  imageposlabel=new QLabel("-",panel_display);
  imageposlabel->hide();
  maskposlabel=new QLabel("-",panel_display);
  maskposlabel->hide();
  statposlabel=new QLabel("-",panel_display);
  statposlabel->hide();

  // slice (X) selection slider
  xslicebox=new QHBox(panel_display);
  new QLabel("X Position: ",xslicebox);
  xsliceslider=new QSlider(Qt::Horizontal,xslicebox);
  xposedit=new QLabel("0",xslicebox);
  xposedit->setFixedWidth(50);
  xposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  xposedit->setLineWidth(1);
  xslicebox->hide();
  xsliceslider->setPageStep(1);
  QObject::connect(xsliceslider,SIGNAL(valueChanged(int)),this,SLOT(NewXSlice(int)));

  // slice (Y) selection slider
  yslicebox=new QHBox(panel_display);
  new QLabel("Y position: ",yslicebox);
  ysliceslider=new QSlider(Qt::Horizontal,yslicebox);
  yposedit=new QLabel("0",yslicebox);
  yposedit->setFixedWidth(50);
  yposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  yposedit->setLineWidth(1);
  yslicebox->hide();
  ysliceslider->setPageStep(1);
  QObject::connect(ysliceslider,SIGNAL(valueChanged(int)),this,SLOT(NewYSlice(int)));

  // slice (Z) selection slider
  zslicebox=new QHBox(panel_display);
  new QLabel("Z position: ",zslicebox);
  zsliceslider=new QSlider(Qt::Horizontal,zslicebox);
  zposedit=new QLabel("0",zslicebox);
  zposedit->setFixedWidth(50);
  zposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  zposedit->setLineWidth(1);
  zslicebox->hide();
  zsliceslider->setPageStep(1);
  QObject::connect(zsliceslider,SIGNAL(valueChanged(int)),this,SLOT(NewZSlice(int)));

  // volume selection slider
  volumebox=new QHBox(panel_display);
  lab=new QLabel("Timepoint: ",volumebox);
  volumeslider=new QSlider(Qt::Horizontal,volumebox);
  tposedit=new QLabel("0",volumebox);
  tposedit->setFixedWidth(50);
  tposedit->setFrameStyle(QFrame::Box | QFrame::Plain);
  tposedit->setLineWidth(1);
  volumebox->hide();
  volumeslider->setPageStep(1);
  QObject::connect(volumeslider,SIGNAL(valueChanged(int)),this,SLOT(NewVolume(int)));

  // brightness slider
  hb=new QHBox(panel_display);
  lab=new QLabel("Brightness:",hb);
  brightslider=new QSlider(Qt::Horizontal,hb);
  brightslider->setPageStep(10);
  brightslider->setRange(1,100);
  brightslider->setValue(50);
  QObject::connect(brightslider,SIGNAL(valueChanged(int)),this,SLOT(NewBrightness(int)));

  // contrast slider
  hb=new QHBox(panel_display);
  lab=new QLabel("Contrast: ",hb);
  contrastslider=new QSlider(Qt::Horizontal,hb);
  contrastslider->setPageStep(10);
  contrastslider->setRange(1,100);
  contrastslider->setValue(50);
  QObject::connect(contrastslider,SIGNAL(valueChanged(int)),this,SLOT(NewContrast(int)));

  button=new QPushButton("Set Origin and Save",panel_display,"setorigin");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SetOrigin()));

  // spacer
  ww=new QWidget(panel_display);
  ww->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);

  // display status line
  q_dstatus=new QLineEdit("",panel_display);
  q_dstatus->setFrameStyle(QFrame::Box | QFrame::Plain);
  q_dstatus->setLineWidth(1);
  q_dstatus->setReadOnly(1);
  q_dstatus->setPaletteBackgroundColor(QColor(252,229,255));

  // LOGO CODE REMOVED FOR THE MOMENT!
  //   char fn[STRINGLEN];
  //   sprintf(fn,"%s/elements/logos/voxbolink.png",vbp.rootdir.c_str());
  //   struct stat st;
  //   if (!(stat(fn,&st))) {
  //     QPixmap *logo=new QPixmap(fn);
  //     hb=new QHBox(this);
  //     lab=new QLabel(hb);
  //     lab->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
  //     lab->setPixmap(*logo);
  //     lab->setFrameStyle(QFrame::Box | QFrame::Plain);
  //     lab->setLineWidth(1);
  //     lab->setAlignment(Qt::AlignLeft);
  //     imageinfolayout->addWidget(hb);
  //   }

  ///////////////////////////////////////
  // PANEL - Masks Panel
  ///////////////////////////////////////
  panel_masks=new QVGroupBox("Masks",topbox);
  toplayout->addWidget(panel_masks);
  // panel_masks->setFixedWidth(280);
//  panel_display->setSpacing(4);

  // mask-related buttons

  button=new QPushButton("Change Mask Dimensions",panel_masks,"changemask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(ChangeMask()));

  hb=new QHBox(panel_masks);
  new QLabel("Save:",hb);

  button=new QPushButton("Multi-Mask",hb,"savemultimask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveMask()));

  button=new QPushButton("Separate",hb,"saveseparatemasks");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveSeparateMasks()));

  button=new QPushButton("Combined",hb,"savecombinedmask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveCombinedMask()));

  hb=new QHBox(panel_masks);
  new QLabel("drawing radius:",hb);
  radiusbox=new QSpinBox(0,30,1,hb);
  radiusbox->setValue(0);
  zradiusbox=new QCheckBox("use radius in z too",hb);
  QObject::connect(radiusbox,SIGNAL(valueChanged(int)),this,SLOT(NewRadius(int)));
  QObject::connect(zradiusbox,SIGNAL(stateChanged(int)),this,SLOT(NewZRadius(int)));

  hb=new QHBox(panel_masks);
  button=new QPushButton("Show All Masks",hb,"showallmasks");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(ShowAllMasks()));
  button=new QPushButton("Hide All Masks",hb,"hideallmasks");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(HideAllMasks()));

  hb=new QHBox(panel_masks);
  button=new QPushButton("Apply Selected Masks",hb,"applymasks");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(ApplyMasks()));
  button=new QPushButton("UnMask",hb,"unmask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(UnMask()));

  button=new QPushButton("New Mask Value",panel_masks,"newmask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(NewMask()));
  
  // maskview=new QScrollView(panel_masks);
  maskview=new QTMaskList(panel_masks);
  maskview->setPaletteBackgroundColor(paletteBackgroundColor());
  maskview->viewport()->setPaletteBackgroundColor(paletteBackgroundColor());
  maskview->setFrameStyle(QFrame::NoFrame);
  
  // insert the initial single mask, check it off
  NewMask();

  // spacer
  ww=new QWidget(panel_masks);
  ww->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);

  // display status line
  q_mstatus=new QLineEdit("",panel_masks);
  q_mstatus->setFrameStyle(QFrame::Box | QFrame::Plain);
  q_mstatus->setLineWidth(1);
  q_mstatus->setReadOnly(1);
  q_mstatus->setPaletteBackgroundColor(QColor(252,229,255));

  // display status line2
  q_mstatus2=new QLineEdit("",panel_masks);
  q_mstatus2->setFrameStyle(QFrame::Box | QFrame::Plain);
  q_mstatus2->setLineWidth(1);
  q_mstatus2->setReadOnly(1);
  q_mstatus2->setPaletteBackgroundColor(QColor(220,229,255));

  ///////////////////////////////////////
  // PANEL - Stats Panel
  ///////////////////////////////////////
  panel_stats=new QVGroupBox("Stat Map",topbox);
  panel_stats->setFixedWidth(240);
  toplayout->addWidget(panel_stats);

  // thresholds for overlay
  hb=new QHBox(panel_stats);
  lab=new QLabel("Low:",hb);
  lowedit=new QLineEdit("3.5",hb);
  QObject::connect(lowedit,SIGNAL(textChanged(const QString&)),this,SLOT(HighlightLow()));
  QObject::connect(lowedit,SIGNAL(returnPressed()),this,SLOT(SetLow()));
  lab=new QLabel("High:",hb);
  highedit=new QLineEdit("5.0",hb);
  QObject::connect(highedit,SIGNAL(textChanged(const QString&)),this,SLOT(HighlightHigh()));
  QObject::connect(highedit,SIGNAL(returnPressed()),this,SLOT(SetHigh()));
  lab=new QLabel("Cluster:",hb);
  clusteredit=new QLineEdit("1",hb);
  QObject::connect(clusteredit,SIGNAL(textChanged(const QString&)),this,SLOT(HighlightCluster()));
  QObject::connect(clusteredit,SIGNAL(returnPressed()),this,SLOT(SetCluster()));

  q_scale=new VBScalewidget(panel_stats);
  q_negcolor1=QColor(0,0,200);
  q_negcolor2=QColor(100,100,255);
  q_poscolor1=QColor(255,0,0);
  q_poscolor2=QColor(255,255,0);
  q_scale->setscale(3.5,5,q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
  q_scale->setFixedSize(220,50);
  QObject::connect(q_scale,SIGNAL(newcolors(QColor,QColor,QColor,QColor)),this,SLOT(newcolors(QColor,QColor,QColor,QColor)));
  
  hb=new QHBox(panel_stats);
  // twotailed checkbox
  QCheckBox *cb=new QCheckBox("two-tailed",hb);
  cb->setChecked(1);
  QObject::connect(cb,SIGNAL(toggled(bool)),this,SLOT(SetTails(bool)));
  // overlay checkbox
  overlaybox=new QCheckBox("hide overlay",hb);
  overlaybox->setChecked(0);
  // imageinfolayout->addWidget(overlaybox);
  QObject::connect(overlaybox,SIGNAL(toggled(bool)),this,SLOT(HideOverlay(bool)));

  hb=new QHBox(panel_stats);
  lab=new QLabel("Find:",hb);
  button=new QPushButton("All Regions",hb,"findallregions");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(FindAllRegions()));
  button=new QPushButton("At Crosshairs",hb,"findcurrentregions");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(FindCurrentRegion()));

  button=new QPushButton("Clear Regions",panel_stats,"clearregions");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(ClearRegions()));

  hb=new QHBox(panel_stats);
  button=new QPushButton("Save Full map",hb,"savefullmap");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveFullMap()));
  button=new QPushButton("Save Visible map",hb,"savevisiblemap");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveVisibleMap()));

  button=new QPushButton("Dump Region Stats",panel_stats,"dumpregionstats");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(ShowRegionStats()));

  button=new QPushButton("Copy Checked Regions as Masks",panel_stats,"copyregions");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(CopyRegions()));

  regionview=new QTMaskList(panel_stats);
  regionview->setPaletteBackgroundColor(paletteBackgroundColor());
  regionview->viewport()->setPaletteBackgroundColor(paletteBackgroundColor());
  regionview->setFrameStyle(QFrame::NoFrame);
  
  // spacer
  ww=new QWidget(panel_stats);
  ww->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);

  // display status line
  q_sstatus=new QLineEdit("",panel_stats);
  q_sstatus->setFrameStyle(QFrame::Box | QFrame::Plain);
  q_sstatus->setLineWidth(1);
  q_sstatus->setReadOnly(1);
  q_sstatus->setPaletteBackgroundColor(QColor(252,229,255));

  // display status line2
  q_sstatus2=new QLineEdit("",panel_stats);
  q_sstatus2->setFrameStyle(QFrame::Box | QFrame::Plain);
  q_sstatus2->setLineWidth(1);
  q_sstatus2->setReadOnly(1);
  q_sstatus2->setPaletteBackgroundColor(QColor(220,229,255));

  panel_stats->hide();

  ///////////////////////////////////////
  // MIDDLE -- status line and filenames
  ///////////////////////////////////////

  statusline=new QLineEdit("<idle>",mainbox);
  statusline->setReadOnly(1);
  statusline->setPaletteBackgroundColor(QColor(252,255,229));
  mainlayout->addWidget(statusline);

  // volume name
  hb=new QHBox(mainbox);
  button=new QPushButton("Image",hb,"loadimage");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(LoadFile()));
  imagenamebox=new QLineEdit("<none>",hb);
  imagenamebox->setReadOnly(1);
  mainlayout->addWidget(hb);

  // maptes
  hb=new QHBox(mainbox);
  button=new QPushButton("Correlation Map Image",hb,"loadimage");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(LoadCorrelationVolume()));
  image2namebox=new QLineEdit("<none>",hb);
  image2namebox->setReadOnly(1);
  mainlayout->addWidget(hb);

  // mask name
  hb=new QHBox(mainbox);
  button=new QPushButton("Mask",hb,"loadmask");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(LoadMask()));
  masknamebox=new QLineEdit("<none>",hb);
  masknamebox->setReadOnly(1);
  mainlayout->addWidget(hb);

  // overlay name
  hb=new QHBox(mainbox);
  button=new QPushButton("Overlay",hb,"loadoverlay");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(LoadOverlay()));
  overlaynamebox=new QLineEdit("<none>",hb);
  overlaynamebox->setReadOnly(1);
  mainlayout->addWidget(hb);

  //////////////////////////////////
  // BOTTOM -- time series browser
  //////////////////////////////////

// -- BEG Mjumbewu 2005.11.29
  // last thing on toplayout -- the header panel
  panel_ts=new QHBox(mainsplitter);
  //panel_ts=new QHBox(mainsplitter);
// -- END Mjumbewu 2005.11.29

// -- COM Mjumbewu 2005.11.29
//  panel_ts=new QHBox(this);
//  mainlayout->addWidget(panel_ts);

  // the actual time series pane
  tspane=new PlotScreen(panel_ts);
  // the listbox that shows things you can plot
  tslist=new QListBox(panel_ts);
  tslist->setSelectionMode(QListBox::Extended);
  QObject::connect(tslist,SIGNAL(selectionChanged()),this,SLOT(UpdateTS()));
  // some additional buttons
  QGroupBox *bg=new QVButtonGroup("Time Series Options",panel_ts);
  ts_meanscalebox=new QCheckBox("Mean scale",bg);
  ts_detrendbox=new QCheckBox("Linear detrend",bg);
  ts_filterbox=new QCheckBox("Apply filtering",bg);
  ts_removebox=new QCheckBox("Remove Covariates of No Interest",bg);
  ts_scalebox=new QCheckBox("Scale Covariates using Beta",bg);
  ts_powerbox=new QCheckBox("Show Power Spectrum",bg);
  ts_pcabox=new QCheckBox("PCA (RGB)",bg);

  // time series averaging
  averagebox=new QComboBox(FALSE,bg,"viewbox");
  averagebox->insertItem("No averaging");
  QObject::connect(averagebox,SIGNAL(activated(const QString&)),this,SLOT(NewAverage()));

  ts_mousebutton=new QRadioButton("Graph voxel under mouse",bg);
  ts_mousebutton->setChecked(1);

  ts_maskbutton=new QRadioButton("Graph selected mask",bg);
  ts_crossbutton=new QRadioButton("Graph voxel at crosshairs",bg);

  button=new QPushButton("Save Time Series",bg,"savets");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveTS()));

  button=new QPushButton("Save Graph",bg,"savegraph");
  QObject::connect(button,SIGNAL(clicked()),this,SLOT(SaveGraph()));

  // QObject::connect(bg,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));

  QObject::connect(ts_meanscalebox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_detrendbox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_filterbox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_removebox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_scalebox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_powerbox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_pcabox,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));

  QObject::connect(ts_mousebutton,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_maskbutton,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  QObject::connect(ts_crossbutton,SIGNAL(toggled(bool)),this,SLOT(UpdateTS()));
  
  //   ts_averagetype=new QListBox(bg);
  //   ts_averagetype->insertItem("Graph voxel under mouse");
  //   ts_averagetype->insertItem("Graph selected mask region");
  //   ts_averagetype->insertItem("Graph voxel at crosshairs");
  //   ts_averagetype->insertItem("Graph supra-threshold voxels [coming soon]");
  //   QObject::connect(ts_averagetype,SIGNAL(highlighted(int)),this,SLOT(ChangeTSType(int)));
  //   ts_averagetype->setSelected(0,1);
  
  // tslayout->addWidget(bg);
  panel_ts->hide();

  // -- BEG Mjumbewu 2005.11.28
  // last thing on toplayout -- the header panel
  panel_header = new QTextEdit(mainsplitter);
  panel_header->setTextFormat(PlainText);
  panel_header->setWordWrap(QTextEdit::NoWrap);
  panel_header->setText("No image loaded.");
  panel_header->setMinimumWidth(280);
  panel_header->setReadOnly(1);
  panel_header->hide();
  // -- END Mjumbewu 2005.11.28

  // last thing on toplayout -- the header panelp
  // -- COM Mjumbewu 2005.11.28
  //  headerlayout=new QVBoxLayout(mainlayout);
  //  panel_header=new QTextEdit(this);
  //  panel_header->setTextFormat(PlainText);
  //  panel_header->setWordWrap(QTextEdit::NoWrap);
  //  panel_header->setText("No image loaded.");
  //  panel_header->setMinimumWidth(280);
  //  panel_header->setReadOnly(1);
  //  panel_header->hide();
  //  headerlayout->addWidget(panel_header);
}

VBView::~VBView()
{
  // FIXME free up view.ivals and svals for all views
}

void
VBView::init()
{
  mode=vbv_mask;
  q_currentdir=".";
  q_hideoverlay=0;
  q_twotailed=1;
  q_columns=5;
  q_lowthresh=3.5;
  q_highthresh=5.0;
  q_clustersize=1;
  q_crosshairs=1;
  q_showorigin=0;
  q_showts=0;
  q_mag=150;
  q_xslice=-1;
  q_yslice=-1;
  q_zslice=-1;
  q_volume=-1;

  // windowing
  q_low=1.0;
  q_high=0.0;
  q_factor=255.0;

  q_update=1;

  viewlist.clear();
  // q_viewmode=vbv_axial;
  q_viewmode=vbv_tri;

  q_isotropic=0;
  q_xscale=1.0;
  q_yscale=1.0;
  q_zscale=1.0;

  maskpos=0;
  regionpos=0;
  q_maskxratio=1;
  q_maskyratio=1;
  q_maskzratio=1;
  q_statxratio=1;
  q_statyratio=1;
  q_statzratio=1;
  q_maskmode=MASKMODE_DRAW;

  q_radius=0;
  q_usezradius=0;

  q_brightness=50;
  q_contrast=50;
  q_maskdirty=0;
  q_currentmask=0;
  q_currentregion=0;
  q_ix=q_iy=q_mx=q_my=q_iz=q_mz=0;
  q_xviewsize=0;
  q_yviewsize=0;
  // q_bgcolor=qRgb(175,165,181);
  // q_bgcolor=qRgb(176,198,202);
  q_bgcolor=qRgb(200,195,144);
  myaverage=(TASpec *)NULL;
}

void
VBView::newcolors(QColor n1,QColor n2,QColor p1,QColor p2)
{
  q_negcolor1=n1;
  q_negcolor2=n2;
  q_poscolor1=p1;
  q_poscolor2=p2;
  RenderAll();
}

void
VBView::paintEvent(QPaintEvent *)
{
  drawarea->update();
}

void
VBView::setscale()
{
  double center,width;
  vector<double>vals;
  double val;
  int xint=cube.dimx/20;
  int yint=cube.dimy/20;
  int zint=cube.dimz/40;
  if (xint<1) xint=1;
  if (yint<1) yint=1;
  if (zint<1) zint=1;
 
  int nozeros=1;
  for (int i=0; i<cube.dimx; i+=xint) {
    for (int j=0; j<cube.dimy; j+=yint) {
      for (int k=0; k<cube.dimz; k+=zint) {
        val=cube.GetValue(i,j,k);
        if (!isfinite(val)) continue;
        if (nozeros) {
          if (val<0.0)
            nozeros=0;
          else if (val==0.0)
            continue;
        }
        vals.push_back(val);
      }
    }
  }
  if (vals.size()==0) {
    q_low=0.0;
    q_high=1.0;
    q_factor=255.0;
    return;
  }
  sort(vals.begin(),vals.end());
  // cut off the tails
  q_low=vals[vals.size()/50];
  q_high=vals[vals.size()*49/50];
  if (q_low == q_high) {
    q_low=0;
    q_high=vals[vals.size()-1];
    q_factor=255.0/q_high;
    return;
  }
  // set window center and width
  width=q_high-q_low;
  center=q_low+width/2.0;
  double increment=pow(8.0,0.02);
  width=width*pow(increment,50.0-q_contrast);
  double lowestcenter=q_low-width/2.0;
  double highestcenter=q_high+width/2.0;
  center=lowestcenter+(highestcenter-lowestcenter)*((100.0-q_brightness)/100.0);
  q_low=center-width/2.0;
  q_high=center+width/2.0;
  if (q_low>=q_high)
    q_high=q_low+1.0;
  q_factor=255.0/(q_high-q_low);
  return;
}

inline short
scaledvalue(double val,double low,double high,double factor)
{
  double bval = (val - low) * factor;
  if (bval > 255) bval=255;
  if (bval < 0) bval=0;
  return (short)bval;
}

void
VBView::UniRender(MyView &view)
{
  if (!q_update)
    return;
  int xsize,ysize;
  int xcross,ycross;          // view coords of crosshairs
  int xorigin=-1,yorigin=-1;  // view coords of origin if >=0
  float i_xinterval,i_yinterval;
  float m_xinterval,m_yinterval;
  float s_xinterval,s_yinterval;
  if (view.orient==MyView::vb_xy) {  // typically axial
    xsize=cube.dimx;
    ysize=cube.dimy;
    xcross=(int)((double)q_xslice*(q_xscale));
    ycross=(int)((double)(cube.dimy-q_yslice-1)*(q_yscale));
    if (view.position==cube.origin[2]) {
      xorigin=(int)((double)cube.origin[0]*(q_xscale));
      yorigin=(int)((double)(cube.dimy-cube.origin[1]-1)*(q_yscale));
    }
    i_xinterval=1.0/(q_xscale);
    i_yinterval=1.0/(q_yscale);
    m_xinterval=i_xinterval/q_maskxratio;
    m_yinterval=i_yinterval/q_maskyratio;
    s_xinterval=i_xinterval/q_statxratio;
    s_yinterval=i_yinterval/q_statyratio;
  }
  else if (view.orient==MyView::vb_yz) {  // typically sagittal
    xsize=cube.dimy;
    ysize=cube.dimz;
    xcross=(int)((double)q_yslice*(q_yscale));
    ycross=(int)((double)(cube.dimz-q_zslice-1)*(q_zscale)); 
    if (view.position==cube.origin[0]) {
      xorigin=(int)((double)cube.origin[1]*(q_yscale));
      yorigin=(int)((double)(cube.dimz-cube.origin[2]-1)*(q_zscale));
    }
    i_xinterval=1.0/(q_yscale);
    i_yinterval=1.0/(q_zscale);
    m_xinterval=i_xinterval/q_maskyratio;
    m_yinterval=i_yinterval/q_maskzratio;
    s_xinterval=i_xinterval/q_statyratio;
    s_yinterval=i_yinterval/q_statzratio;
  }
  else { // (view.orient==MyView::vb_xz) {  // typically coronal
    xsize=cube.dimx;
    ysize=cube.dimz;
    xcross=(int)((double)q_xslice*(q_xscale));
    ycross=(int)((double)(cube.dimz-q_zslice-1)*(q_zscale));
    if (view.position==cube.origin[1]) {
      xorigin=(int)((double)cube.origin[0]*(q_xscale));
      yorigin=(int)((double)(cube.dimz-cube.origin[2]-1)*(q_zscale));
    }
    i_xinterval=1.0/(q_xscale);
    i_yinterval=1.0/(q_zscale);
    m_xinterval=i_xinterval/q_maskxratio;
    m_yinterval=i_yinterval/q_maskzratio;
    s_xinterval=i_xinterval/q_statxratio;
    s_yinterval=i_yinterval/q_statzratio;
  }

  // for each position in this view
  // FIXME iz, mz, and sz aren't used
  int rr,gg,bb;
  float ix,iy,iz;
  float mx,my,mz;
  float sx,sy,sz;
  iy=0.0;
  iz=view.position;
  mx=0.0;
  my=0.0;
  mz=iz/q_maskzratio;

  sx=0.0;
  sy=0.0;
  sz=iz/q_statzratio;

  // int mval;
  double sval;
  unsigned int *p;
  int index;

  for (int j=0; j<view.height; j++) {
    ix=0.0;
    mx=0.0;
    sx=0.0;
    p=(unsigned int *)currentimage.scanLine(view.yoff+j);
    p+=(view.xoff);
    for (int i=0; i<view.width; i++) {
      index=((int)iy*xsize)+(int)ix;
      rr=gg=bb=view.ivals[index];
      // stat maps
      if (statmap.data && q_hideoverlay==0) {
        sval=view.svals[index];

        // FIXME should really render statmaps as REGIONS, so that we
        // don't have to make the value 0.0 special

//         if (q_twotailed || sval>0.0)
        if (sval != 0.0)
          overlayvalue(q_lowthresh,q_highthresh,sval,rr,gg,bb);
        sx+=s_xinterval;
      }
      // crosshairs
      if (q_crosshairs && (i==xcross || j==ycross)) {
        rr=180; gg=255; bb=180;
      }
      // origin
      if (q_showorigin && xorigin>-1) {
        double dist=sqrt((double)((i-xorigin)*(i-xorigin))
                         +(double)((j-yorigin)*(j-yorigin)));
        if (dist>9.5 && dist < 10.5) {
          rr=255;
          gg=0;
          bb=0;
        }
      }
      ix+=i_xinterval;
      mx+=m_xinterval;
      *p=qRgb(rr,gg,bb);
      p++;
    } 
    iy+=i_yinterval;
    my+=m_yinterval;
    if (statmap.data) sy+=s_yinterval;
  }
  // render masks
  for (int i=0; i<(int)q_masks.size(); i++) {
    if (!(q_masks[i].f_widget->f_visible))
      continue;
    //q_update=0;
    for (int j=0; j<(int)q_masks[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      UniRenderMask(view,vv.x,vv.y,vv.z,q_masks[i].f_color);
    }
    //q_update=1;
  }
  // render regions
  for (int i=0; i<(int)q_regions.size(); i++) {
    if (!(q_regions[i].f_widget->f_visible))
      continue;
    if (i!=q_currentregion)
      continue;
    //q_update=0;
    for (int j=0; j<(int)q_regions[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_regions[i].f_region.vox[j];
      UniRenderMask(view,vv.x,vv.y,vv.z,q_regions[i].f_color);
    }
    //q_update=1;
  }
  return;
}

void
VBView::RenderRegionOnAllViews(VBRegion &rr,QColor cc)
{
  if (!q_update)
    return;
  for (int i=0; i<(int)viewlist.size(); i++) {
    for (int j=0; j<(int)rr.size(); j++) {
      UniRenderMask(viewlist[i],rr[j].x,rr[j].y,rr[j].z,cc);
    }
  }
}

// // getmaskregion() -- convert the mask region to a drawregion; we can
// // then render the draw region generically by including brain
// // (draw2img), map, and stat

// int
// VBView::NewRenderMask(VBV_Mask mm)
// {
//   for (vector<MyView>::iterator vv=viewlist.begin(); vv!=viewlist.end(); vv++) {
//     for (int i=0; i<mm.f_region.size(); i++) {
//       int x1,y1,x2,y2;
//       if (Mask2DrawRegion(*vv,mm.f_region[i].x,mm.f_region[i].y,mm.f_region[i].z,
//                           x1,x2,y1,y2))
//         continue;
//       // we have a draw region, let's go voxel by voxel
//       for (int x=x1; x<=x2; x++) {
//         for (int y=y1; y<=y2; y++) {
          
          
          
          
//         }
//       }
//     }
//     if (Mask2DrawRegion(

  
// }


// int
// VBView::Mask2DrawRegion(MyView &view,int mx,int my,int mz,int x1,int x2,int y1,int y2)
// {
//   if (mz!=view.position/q_maskzratio)
//     return 1;
//   // build a box of view pixels
//   x1=(int)((double)(mx-1)*q_maskxratio*q_xscale);
//   x2=(int)((double)(mx+1)*q_maskxratio*q_xscale);
//   y1=(int)((double)(q_masky-my-2)*q_maskyratio*q_yscale);
//   y2=(int)((double)(q_masky-my+0)*q_maskyratio*q_yscale);
//   if (x1<0) x1=0;
//   if (x2>view.width-1) x2=view.width-1;
//   if (y1<0) y1=0;
//   if (y2>view.height-1) y2=view.height-1;
//   return 0;
// }

void
VBView::UniRenderMask(MyView &view,int mx,int my,int mz,QColor color)
{
  if (!q_update)
    return;
  int rr,gg,bb;
  int x1,x2,y1,y2;  // draw range to cover
  int dx,dy,dz;
  int sx,sy,sz;
  unsigned int *p;
  if (view.orient==MyView::vb_xy) {
    if (mz != view.position/q_maskzratio)  // not on this slice
      return;
    x1=(int)((double)(mx-1)*q_maskxratio*q_xscale);
    x2=(int)((double)(mx+1)*q_maskxratio*q_xscale);
    y1=(int)((double)(q_masky-my-2)*q_maskyratio*q_yscale);
    y2=(int)((double)(q_masky-my+0)*q_maskyratio*q_yscale);
    if (x1<0) x1=0;
    if (x2>view.width-1) x2=view.width-1;
    if (y1<0) y1=0;
    if (y2>view.height-1) y2=view.height-1;
    
    for (int j=y1; j<=y2; j++) {
      p=(unsigned int *)currentimage.scanLine(view.yoff+j);
      p+=view.xoff+x1;
      for (int i=x1; i<=x2; i++) {
        dx=view.xoff+i; dy=view.yoff+j; dz=0;
        Draw2Mask(dx,dy,dz);
        if (dx==mx && dy==my) {
          dx=view.xoff+i; dy=view.yoff+j; dz=0;
          sx=view.xoff+i; sy=view.yoff+j; sz=0;
          Draw2Img(dx,dy,dz);
          Draw2Stat(sx,sy,sz);
          // FIXME below: cube value already in ival array
          rr=gg=bb=scaledvalue(cube.GetValue(dx,dy,dz),q_low,q_high,q_factor);
          // stat maps
          if (statmap.data && q_hideoverlay==0 && q_maskmode!=MASKMODE_DRAW) {
            double sval=statmap.GetValue(sx,sy,sz);
            if (q_twotailed || sval>0.0)
              overlayvalue(q_lowthresh,q_highthresh,sval,rr,gg,bb);
          }
          if (q_maskmode==MASKMODE_DRAW) {
            rr=(int)((rr/255.0)*color.red());
            gg=(int)((gg/255.0)*color.green());
            bb=(int)((bb/255.0)*color.blue());
          }
          *p=qRgb(rr,gg,bb);
//           *p=qRgb((int)(rr*0.75)+(int)(color.red()*0.25),
//                   (int)(gg*0.75)+(int)(color.green()*0.25),
//                   (int)(bb*0.75)+(int)(color.blue()*0.25));
        }
        p++;
      }
    }
    if (q_update)
      drawarea->updateRegion((int)(view.xoff+x1),(int)(view.yoff+y1),
			     (int)(x2-x1+1),(int)(y2-y1+1));
  }
  else if (view.orient==MyView::vb_yz) {
    if (mx != view.position/q_maskxratio)  // not on this slice
      return;
    x1=(int)((double)(my-1)*q_maskyratio*q_yscale);
    x2=(int)((double)(my+1)*q_maskyratio*q_yscale);
    y1=(int)((double)(q_maskz-mz-2)*q_maskzratio*q_zscale);
    y2=(int)((double)(q_maskz-mz+0)*q_maskzratio*q_zscale);
    if (x1<0) x1=0;
    if (x2>view.width-1) x2=view.width-1;
    if (y1<0) y1=0;
    if (y2>view.height-1) y2=view.height-1;
    
    for (int j=y1; j<=y2; j++) {
      p=(unsigned int *)currentimage.scanLine(view.yoff+j);
      p+=view.xoff+x1;
      for (int i=x1; i<=x2; i++) {
        dx=view.xoff+i; dy=view.yoff+j; dz=0;
        Draw2Mask(dx,dy,dz);
        if (dy==my && dz==mz) {
          dx=view.xoff+i; dy=view.yoff+j; dz=0;
          sx=view.xoff+i; sy=view.yoff+j; sz=0;
          Draw2Img(dx,dy,dz);
          Draw2Stat(sx,sy,sz);
          // FIXME below: cube value already in ival array
          rr=gg=bb=scaledvalue(cube.GetValue(dx,dy,dz),q_low,q_high,q_factor); 
          // stat maps
          if (statmap.data && q_hideoverlay==0 && q_maskmode!=MASKMODE_DRAW) {
            double sval=statmap.GetValue(sx,sy,sz);
            if (q_twotailed || sval>0.0)
              overlayvalue(q_lowthresh,q_highthresh,sval,rr,gg,bb);
          }
          if (q_maskmode==MASKMODE_DRAW) {
            rr=(int)((rr/255.0)*color.red());
            gg=(int)((gg/255.0)*color.green());
            bb=(int)((bb/255.0)*color.blue());
          }
          *p=qRgb(rr,gg,bb);
        }
        p++;
      }
    }
    if (q_update)
      drawarea->updateRegion((int)(view.xoff+x1),(int)(view.yoff+y1),
			     (int)(x2-x1+1),(int)(y2-y1+1));
  }
  else if (view.orient==MyView::vb_xz) {
    if (my != view.position/q_maskyratio)  // not on this slice
      return;
    x1=(int)((double)(mx-1)*q_maskxratio*q_xscale);
    x2=(int)((double)(mx+1)*q_maskxratio*q_xscale);
    y1=(int)((double)(q_maskz-mz-2)*q_maskzratio*q_zscale);
    y2=(int)((double)(q_maskz-mz+0)*q_maskzratio*q_zscale);
    if (x1<0) x1=0;
    if (x2>view.width-1) x2=view.width-1;
    if (y1<0) y1=0;
    if (y2>view.height-1) y2=view.height-1;
    
    for (int j=y1; j<=y2; j++) {
      p=(unsigned int *)currentimage.scanLine(view.yoff+j);
      p+=view.xoff+x1;
      for (int i=x1; i<=x2; i++) {
        dx=view.xoff+i; dy=view.yoff+j; dz=0;
        Draw2Mask(dx,dy,dz);
        if (dx==mx && dz==mz) {
          dx=view.xoff+i; dy=view.yoff+j; dz=0;
          sx=view.xoff+i; sy=view.yoff+j; sz=0;
          Draw2Img(dx,dy,dz);
          Draw2Stat(sx,sy,sz);
          // FIXME below: cube value already in ival array
          rr=gg=bb=scaledvalue(cube.GetValue(dx,dy,dz),q_low,q_high,q_factor);
          // stat maps
          if (statmap.data && q_hideoverlay==0 && q_maskmode!=MASKMODE_DRAW) {
            double sval=statmap.GetValue(sx,sy,sz);
            if (q_twotailed || sval>0.0)
              overlayvalue(q_lowthresh,q_highthresh,sval,rr,gg,bb);
          }
          if (q_maskmode==MASKMODE_DRAW) {
            rr=(int)((rr/255.0)*color.red());
            gg=(int)((gg/255.0)*color.green());
            bb=(int)((bb/255.0)*color.blue());
          }
          *p=qRgb(rr,gg,bb);
        }
        p++;
      }
    }
    if (q_update)
      drawarea->updateRegion((int)(view.xoff+x1),(int)(view.yoff+y1),
			     (int)(x2-x1+1),(int)(y2-y1+1));
  }
}

MyView::MyView()
{
  ivals=(int *)NULL;
  svals=(float *)NULL;
}

MyView::~MyView()
{
  if (ivals) delete [] ivals;
  if (svals) delete [] svals;
}

void
VBView::img2coords(int x,int y,int z,int &mx,int &my,int &mz,int &sx,int &sy,int &sz)
{
  if (q_masks.size()) {
    mx=(int)(x/q_maskxratio);
    my=(int)(y/q_maskyratio);
    mz=(int)(z/q_maskzratio);
  }
  else
    mx=my=mz=-1;
  if (statmap.data) {
    sx=(int)(x/q_statxratio);
    sy=(int)(y/q_statyratio);
    sz=(int)(z/q_statzratio);
  }
  else
    sx=sy=sz=-1;
}

void
VBView::BuildViews(int mode)
{
  if (!q_update)
    return;
  viewlist.clear();
  MyView vv;

  q_viewmode=mode;
  
  int xx=(int)ceil(q_xscale*cube.dimx);
  int yy=(int)ceil(q_yscale*cube.dimy);
  int zz=(int)ceil(q_zscale*cube.dimz);

  vv.xoff=0;
  vv.yoff=0;

  if (mode==vbv_axial) {
    vv.orient=MyView::vb_xy;
    vv.position=q_zslice;
    vv.width=xx;
    vv.height=yy;
    viewlist.push_back(vv);
    zslicebox->show();
  }
  else if (mode==vbv_coronal) {
    vv.orient=MyView::vb_xz;
    vv.position=q_yslice;
    vv.width=xx;
    vv.height=zz;
    viewlist.push_back(vv);
    zslicebox->show();
  }
  else if (mode==vbv_sagittal) {
    vv.orient=MyView::vb_yz;
    vv.position=q_xslice;
    vv.width=yy;
    vv.height=zz;
    viewlist.push_back(vv);
    zslicebox->show();
  }
  else if (mode==vbv_multi) {
    vv.orient=MyView::vb_xy;
    vv.width=xx;
    vv.height=yy;
    int col=0;
    for (int i=0; i<cube.dimz; i++) {
      vv.position=i;
      viewlist.push_back(vv);
      if (++col >= q_columns) {
        col=0;
        vv.xoff=0;
        vv.yoff+=yy;
      }
      else
        vv.xoff+=xx;
    }
    zslicebox->hide();
  }
  else if (mode==vbv_tri) {
    // axial
    vv.orient=MyView::vb_xy;
    vv.position=q_zslice;
    vv.width=xx;
    vv.height=yy;
    viewlist.push_back(vv);
    
    // coronal
    vv.orient=MyView::vb_xz;
    vv.position=q_yslice;
    vv.yoff+=yy;
    vv.width=xx;
    vv.height=zz;
    viewlist.push_back(vv);

    // sagittal
    vv.orient=MyView::vb_yz;
    vv.position=q_xslice;
    vv.xoff+=xx;
    vv.width=yy;
    vv.height=zz;
    viewlist.push_back(vv);
    zslicebox->show();
  }
  SetViewSize();
}

void
VBView::SetViewSize()
{
  // set xviewsize and yviewsize for new MyView
  q_xviewsize=0;
  q_yviewsize=0;
  double right,bottom;
  for (int i=0; i<(int)viewlist.size(); i++) {
    right=(viewlist[i].xoff+viewlist[i].width);
    bottom=(viewlist[i].yoff+viewlist[i].height);
    if (right > q_xviewsize)
      q_xviewsize=(int)ceil(right);
    if (bottom > q_yviewsize)
      q_yviewsize=(int)ceil(bottom);
    PreCalcView(viewlist[i]);
  }

  if (currentimage.width()!=q_xviewsize || currentimage.height()!=q_yviewsize) {
    currentimage.create(q_xviewsize,q_yviewsize,32);
    currentimage.setAlphaBuffer(1);
    drawarea->setFixedSize(q_xviewsize,q_yviewsize);
  }
  currentimage.fill(q_bgcolor);
}

void
VBView::PreCalcView(MyView &view)
{
  // if (!q_update) return;
  int x1,y1,z1;               // img coords for corner of slice
  int xincr_x,xincr_y,xincr_z,yincr_x,yincr_y,yincr_z,xsize,ysize;
  if (view.orient==MyView::vb_xy) {  // typically axial
    x1=0; y1=cube.dimy-1; z1=view.position;
    xincr_x=1; xincr_y=0; xincr_z=0;
    yincr_x=0; yincr_y=-1; yincr_z=0;
    xsize=cube.dimx;
    ysize=cube.dimy;
  }
  else if (view.orient==MyView::vb_yz) {  // typically sagittal
    x1=view.position; y1=0; z1=cube.dimz-1;
    xincr_x=0; xincr_y=1; xincr_z=0;
    yincr_x=0; yincr_y=0; yincr_z=-1;
    xsize=cube.dimy;
    ysize=cube.dimz;
  }
  else { // (view.orient==MyView::vb_xz) {  // typically coronal
    x1=0; y1=view.position; z1=cube.dimz-1;
    xincr_x=1; xincr_y=0; xincr_z=0;
    yincr_x=0; yincr_y=0; yincr_z=-1;
    xsize=cube.dimx;
    ysize=cube.dimz;
  }

  if (view.ivals) delete [] view.ivals;
  if (view.svals) delete [] view.svals;
  view.ivals=new int[xsize*ysize];
  view.svals=new float[xsize*ysize];

  int xpos,ypos,zpos,ind=0;
  for (int j=0; j<ysize; j++) {
    for (int i=0; i<xsize; i++) {
      xpos=x1+(i*xincr_x)+(j*yincr_x);
      ypos=y1+(i*xincr_y)+(j*yincr_y);
      zpos=z1+(i*xincr_z)+(j*yincr_z);
      view.ivals[ind]=scaledvalue(cube.GetValue(xpos,ypos,zpos),q_low,q_high,q_factor);
      if (statmap.data)
	view.svals[ind]=statmap.GetValue((int)(xpos/q_statxratio),
				     (int)(ypos/q_statyratio),
				     (int)(zpos/q_statzratio));
      ind++;
    }
  }
}

void
VBView::UpdateView(MyView &view)
{
  drawarea->updateRegion((int)(view.xoff),
                         (int)(view.yoff),
                         (int)(view.width),
                         (int)(view.height));
}

void
VBView::RenderAll()
{
  if (!q_update) return;
  for (int i=0; i<(int)viewlist.size(); i++)
    RenderSingle(viewlist[i]);
  drawarea->updateVisibleNow();
}

void
VBView::RenderSingle(MyView &view)
{
  if (!q_update) return;
  if (view.orient==MyView::vb_xy)
    UniRender(view);
  else if (view.orient==MyView::vb_yz)
    UniRender(view);
  else if (view.orient==MyView::vb_xz)
    UniRender(view);
}

void
VBView::ToolChange(int tool)
{
  if (tool==0)
    mode=vbv_mask;
  else if (tool==1)
    mode=vbv_erase;
  else if (tool==2)
    mode=vbv_cross;
  else if (tool==3)
    mode=vbv_fill;
}

void
VBView::Print()
{
  QPrinter pr(QPrinter::ScreenResolution);
  pr.setPageSize(QPrinter::Letter);
  pr.setColorMode(QPrinter::Color);
  // pr.setFullPage(TRUE);
  // pr.setResolution(600);
  //QPaintDeviceMetrics metrics(&pr);
  //int margin=metrics.logicalDpiY()*2;
  
  if (pr.setup(this)) {
    QPainter mypt;
    mypt.begin(&pr);
    // get actual dpi for printer
    // draw it
    // mypt.drawImage(margin,margin,*(drawarea->GetImage()),0,0,q_xviewsize,q_yviewsize);
    mypt.drawImage(0,0,*(drawarea->GetImage()),0,0,q_xviewsize,q_yviewsize);
    mypt.end();
  }
  // pr.newPage();
}

int
VBView::LoadFile()
{
  fileview fv;
  fv.ShowDirectoriesAlso(1);
  fv.ShowImageInformation(1);
  fv.Go();
  tokenlist ff=fv.ReturnFiles();
  return (SetImage(ff[0]));
}

// int
// VBView::LoadDir()
// {
//   QString s;
//   s=QFileDialog::getExistingDirectory(q_currentdir.c_str(),this,"open image file or directory","Choose a file or directory to view",FALSE,TRUE);
//   if (s==QString::null) return (0);
//   return (SetImage(s.latin1()));
// }

int
VBView::SetImage(string fname)
{
  q_currentdir=xdirname(fname);
  q_filename=fname;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (!tes.ReadFile(fname)) {
    tes.getCube(0,cube);
    volumeslider->setRange(0,tes.dimt-1);
    volumeslider->setValue(0);
    volumebox->show();
    q_volume=0;
  }
  else if (!cube.ReadFile(fname)) { 
    tes.invalidate();
    tes.data_valid=0;
    volumebox->hide();
  }
  QApplication::restoreOverrideCursor();
  if (!tes.data && !cube.data) {
    string msg=(string)"Couldn't find image data in "+fname;
    QMessageBox::warning(this,"Image Loading Error",msg.c_str(),"Okay");
    return (100);
  }

  q_update=0;

  if (cube.dimx < 128 && cube.dimy < 128) {
    q_mag=400;
    q_maskx=cube.dimx;
    q_masky=cube.dimy;
    q_maskz=cube.dimz;
    q_maskxratio=1;
    q_maskyratio=1;
    q_maskzratio=1;
  }
  else {
    q_mag=150;
    q_maskx=cube.dimx/4;
    q_masky=cube.dimy/4;
    q_maskz=cube.dimz;
    q_maskxratio=4;
    q_maskyratio=4;
    q_maskzratio=1;
  }

  ClearMasks();
  NewMask();
  update_status();

  setscale();
  zsliceslider->setRange(0,cube.dimz-1);
  zsliceslider->setValue(cube.dimz/2);

  xsliceslider->setRange(0,cube.dimx-1);
  xsliceslider->setValue(cube.dimx/2);

  ysliceslider->setRange(0,cube.dimy-1);
  ysliceslider->setValue(cube.dimy/2);

  zslicebox->show();
  xslicebox->show();
  yslicebox->show();

  NewZSlice(cube.dimz/2);
  if (q_mag==100)
    magbox->setCurrentText("100%");
  else if (q_mag==150)
    magbox->setCurrentText("150%");
  else if (q_mag==400)
    magbox->setCurrentText("400%");
  emit renameme(this,xfilename(fname));
  imagenamebox->setText(fname.c_str());
  CopyInfo();
  q_update=1;

  // updatescaling() also builds views and renders
  UpdateScaling();

  // populate the voxel surfing listbox
  tslist->clear();
  tspane->clear();
  if (tes.data)
    tslist->insertItem("Raw time series");
  tslist->setSelected(0,1);
  // remove GLM-specific checkboxes
  ts_filterbox->hide();
  ts_removebox->hide();
  ts_scalebox->hide();
  return 0;
}

int
VBView::SetImage(Cube &newcube)
{
  cube=newcube;
  string fname=cube.GetFileName();
  tes.invalidate();
  tes.data_valid=0;
  volumebox->hide();
  // and now the rest of the stuff
  q_update=0;

  if (cube.dimx < 128 && cube.dimy < 128) {
    q_mag=400;
    q_maskx=cube.dimx;
    q_masky=cube.dimy;
    q_maskz=cube.dimz;
    q_maskxratio=1;
    q_maskyratio=1;
    q_maskzratio=1;
  }
  else {
    q_mag=150;
    q_maskx=cube.dimx/4;
    q_masky=cube.dimy/4;
    q_maskz=cube.dimz;
    q_maskxratio=4;
    q_maskyratio=4;
    q_maskzratio=1;
  }

  ClearMasks();
  NewMask();
  update_status();

  setscale();
  zsliceslider->setRange(0,cube.dimz-1);
  zsliceslider->setValue(cube.dimz/2);

  xsliceslider->setRange(0,cube.dimx-1);
  xsliceslider->setValue(cube.dimx/2);

  ysliceslider->setRange(0,cube.dimy-1);
  ysliceslider->setValue(cube.dimy/2);

  zslicebox->show();
  xslicebox->show();
  yslicebox->show();

  NewZSlice(cube.dimz/2);
  if (q_mag==100)
    magbox->setCurrentText("100%");
  else if (q_mag==150)
    magbox->setCurrentText("150%");
  else if (q_mag==400)
    magbox->setCurrentText("400%");
  emit renameme(this,xfilename(fname));
  imagenamebox->setText(fname.c_str());
  CopyInfo();
  q_update=1;

  // updatescaling() also builds views and renders
  UpdateScaling();

  // depopulate the voxel surfing listbox
  tslist->clear();
  tspane->clear();
  return 0;
}

int
VBView::SetImage(Tes &newtes)
{
  tes=newtes;
  string fname=tes.GetFileName();
  tes.getCube(0,cube);
  volumeslider->setRange(0,tes.dimt-1);
  volumeslider->setValue(0);
  volumebox->show();
  q_volume=0;
  // and now the rest of the stuff
  q_update=0;

  if (cube.dimx < 128 && cube.dimy < 128) {
    q_mag=400;
    q_maskx=cube.dimx;
    q_masky=cube.dimy;
    q_maskz=cube.dimz;
    q_maskxratio=1;
    q_maskyratio=1;
    q_maskzratio=1;
  }
  else {
    q_mag=150;
    q_maskx=cube.dimx/4;
    q_masky=cube.dimy/4;
    q_maskz=cube.dimz;
    q_maskxratio=4;
    q_maskyratio=4;
    q_maskzratio=1;
  }

  // BuildViews(q_viewmode);

  ClearMasks();
  NewMask();
  update_status();

  setscale();
  zsliceslider->setRange(0,cube.dimz-1);
  zsliceslider->setValue(cube.dimz/2);

  xsliceslider->setRange(0,cube.dimx-1);
  xsliceslider->setValue(cube.dimx/2);

  ysliceslider->setRange(0,cube.dimy-1);
  ysliceslider->setValue(cube.dimy/2);

  zslicebox->show();
  xslicebox->show();
  yslicebox->show();

  NewZSlice(cube.dimz/2);
  if (q_mag==100)
    magbox->setCurrentText("100%");
  else if (q_mag==150)
    magbox->setCurrentText("150%");
  else if (q_mag==400)
    magbox->setCurrentText("400%");
  emit renameme(this,xfilename(fname));
  imagenamebox->setText(fname.c_str());
  CopyInfo();
  q_update=1;

  // updatescaling() also builds views and renders
  UpdateScaling();
  // populate the voxel surfing listbox
  tslist->clear();
  tspane->clear();
  tslist->insertItem("Raw time series"); 
  tslist->setSelected(0,1);
  // remove GLM-specific checkboxes
  ts_filterbox->hide();
  ts_removebox->hide();
  ts_scalebox->hide();
  return 0;
}

void
VBView::CopyInfo()
{
  stringstream tmps;
  panel_header->clear();
  panel_header->setCurrentFont(QFont("Helvetica",10,QFont::DemiBold));
  // do the main info
  tmps.str("");
  if (tes.data_valid)
    tmps << "4D data from file " << xfilename(tes.GetFileName()) << endl;
  else if (cube.data_valid)
    tmps << "3D data from file " << xfilename(cube.GetFileName()) << endl;
  else {
    panel_header->insert("No image loaded");
    return;
  }
  panel_header->insert(tmps.str().c_str());

  tmps.str("");
  tmps << "Voxel Sizes in mm: " << cube.voxsize[0] << " x " << cube.voxsize[1] << " x "
       << cube.voxsize[2] << "\n";
  panel_header->insert(tmps.str().c_str());

  tmps.str("");
  tmps << "Image Dimensions in voxels: " << cube.dimx << " x " << cube.dimy << " x "
       << cube.dimz << "\n";
  panel_header->insert(tmps.str().c_str());

  tmps.str("");
  tmps << "Origin voxel: (" << cube.origin[0] << "," << cube.origin[1] << ","
       << cube.origin[2] << ")\n";
  panel_header->insert(tmps.str().c_str());

  panel_header->setCurrentFont(QFont("Helvetica",10,QFont::Light));
  for (int i=0; i<(int)cube.header.size(); i++) {
    panel_header->insert(cube.header[i].c_str());
    panel_header->insert("\n");
  }
  panel_header->setContentsPos(0,0);
}

// handlers

int
VBView::NewXSlice(int newval)
{
  if (q_xslice==newval)
    return 0;
  q_xslice=newval;
  if (q_xslice > cube.dimx-1)
    q_xslice=cube.dimx-1;
  if (q_xslice<0)
    q_xslice=0;
  for (int i=0; i<(int)viewlist.size(); i++) {
    if (viewlist[i].orient==MyView::vb_yz) {
      viewlist[i].position=q_xslice;
      PreCalcView(viewlist[i]);
    }
  }
  RenderAll();
  UpdateTS();

  UpdatePosition();
  return (0);
}

int
VBView::NewYSlice(int newval)
{
  if (q_yslice==newval)
    return 0;
  q_yslice=newval;
  if (q_yslice > cube.dimy-1)
    q_yslice=cube.dimy-1;
  if (q_yslice<0)
    q_yslice=0;
  for (int i=0; i<(int)viewlist.size(); i++) {
    if (viewlist[i].orient==MyView::vb_xz) {
      viewlist[i].position=q_yslice;
      PreCalcView(viewlist[i]);
    }
  }
  RenderAll();
  UpdateTS();

  UpdatePosition();
  return (0);
}

int
VBView::NewZSlice(int newval)
{
  if (q_zslice==newval)
    return 0;
  q_zslice=newval;
  if (q_zslice > cube.dimz-1)
    q_zslice=cube.dimz-1;
  if (q_zslice<0)
    q_zslice=0;
  for (int i=0; i<(int)viewlist.size(); i++) {
    if (viewlist[i].orient==MyView::vb_xy) {
      viewlist[i].position=q_zslice;
      PreCalcView(viewlist[i]);
    }
  }
  RenderAll();
  UpdateTS();

  UpdatePosition();
  return (0);
}

int
VBView::NewVolume(int newval)
{
  if (q_volume==newval)
    return 0;
  q_volume=newval;
  if (q_volume > tes.dimt-1)
    q_volume=tes.dimt-1;
  if (q_volume<0)
    q_volume=0;
  tes.getCube(q_volume,cube);
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  UpdatePosition();
  return (0);
}



void
VBView::UpdatePosition()
{
  int mx,my,mz,sx,sy,sz;
  img2coords(q_ix,q_iy,q_iz,mx,my,mz,sx,sy,sz);
  char tmp[512];
  if (tes.data_valid) {
    sprintf(tmp,"Image: (%d,%d,%d,%d)=%g",q_ix,q_iy,q_iz,q_volume,
	    cube.GetValue(q_ix,q_iy,q_iz));
  }
  else {
    sprintf(tmp,"Image: (%d,%d,%d)=%g",q_ix,q_iy,q_iz,
	    cube.GetValue(q_ix,q_iy,q_iz));
  }
  imageposlabel->setText(tmp);
  imageposlabel->show();

  if (q_masks.size()) {
    sprintf(tmp,"Mask: (%d,%d,%d)=%d",q_mx,q_my,q_mz,
            GetMaskValue(q_mx,q_my,q_mz));
    maskposlabel->setText(tmp);
    maskposlabel->show();
  }
  else {
    maskposlabel->hide();
  }

  if (statmap.data) {
    sprintf(tmp,"StatMap: (%d,%d,%d)=%g",sx,sy,sz,
            backupmap.GetValue(sx,sy,sz));
    statposlabel->setText(tmp);
    statposlabel->show();
  }
  else {
    statposlabel->hide();
  }

  sprintf(tmp,"%d",q_xslice);
  xposedit->setText(tmp);
  sprintf(tmp,"%d",q_yslice);
  yposedit->setText(tmp);
  sprintf(tmp,"%d",q_zslice);
  zposedit->setText(tmp);
  sprintf(tmp,"%d",q_volume);
  tposedit->setText(tmp);
  if (q_tstype==tsmode_mouse)
    UpdateTS();
}

// void
// VBView::UpdateTesTS()
// {
//   // get rid of the old vectors
//   while (tspane->getVecNum())
//     tspane->delVector(0);

//   tspane->setXCaption("volume number");
//   tspane->setYCaption("value");

//   // build a voxel list 0=position 1=mask 2=crosshairs 3=supra-threshold
//   vector<VBVoxel> vlist;
//   VBVoxel vx;
//   if (q_tstype==tsmode_mask && q_masks.size() && q_currentmask>-1) {
//     for (int i=0; i<cube.dimx; i++) {
//       for (int j=0; j<cube.dimy; j++) {
//         for (int k=0; k<cube.dimz; k++) {
// 	  // convert cube coord to mask coord
// 	  int mx,my,mz,sx,sy,sz;
// 	  img2coords(i,j,k,mx,my,mz,sx,sy,sz);
//           if (GetMaskValue(mx,my,mz)==q_masks[q_currentmask].f_index) {
//             vx.x=i; vx.y=j; vx.z=k;
//             vlist.push_back(vx);
//           }
//         }
//       }
//     }
//   }
//   else if (q_tstype==tsmode_cross) {
//     vx.x=q_xslice; vx.y=q_yslice; vx.z=q_zslice;
//     vlist.push_back(vx);
//   }
//   else {
//     vx.x=q_ix; vx.y=q_iy; vx.z=q_iz;
//     vlist.push_back(vx);
//   }

//   if (!(vlist.size()))
//     return;

//   // average time series and residuals

//   // RAW TIME SERIES
//   if (tslist->isSelected(0) && glmi.teslist.size()) {
//     exit(10);
//     VB_Vector vv;
//     for (int v=0; v<(int)vlist.size(); v++) {
//       VB_Vector vtmp;
//       for (int i=0; i<(int)glmi.teslist.size(); i++) {
//         Tes mytes;
//         mytes.ReadTimeSeries(glmi.teslist[i],vlist[v].x,vlist[v].y,vlist[v].z);
//         vtmp.concatenate(mytes.timeseries);
//       }
//       if (vv.getLength()!=vtmp.getLength())
//         vv=vtmp;
//       else
//         vv+=vtmp;
//     }
//     vv /= vlist.size();
//     if (ts_powerbox->isChecked()) {
//       vv=fftnyquist(vv);
//       vv[0]=0;
//     }
//     if (myaverage)
//       vv=myaverage->getTrialAverage(vv);
//     tspane->addVector(vv,"blue");
//   }

//   // RAW TES TIME SERIES
//   if (tslist->isSelected(0) && glmi.teslist.size()==0 && tes.data) {
//     VB_Vector vv;
//     for (int v=0; v<(int)vlist.size(); v++) {
//       if (tes.GetTimeSeries(vlist[v].x,vlist[v].y,vlist[v].z))
//         continue;
//       if (ts_meanscalebox->isChecked())
//         tes.timeseries.meanNormalize();
//       if (ts_detrendbox->isChecked())
//         tes.timeseries.removeDrift();
//       if (vv.getLength()!=tes.timeseries.getLength())
//         vv=tes.timeseries;
//       else
//         vv+=tes.timeseries;
//     }
//     vv /= vlist.size();
//     if (ts_powerbox->isChecked()) {
//       vv=fftnyquist(vv);
//       vv[0]=0;
//     }
//     if (myaverage)
//       vv=myaverage->getTrialAverage(vv);
//     tspane->addVector(vv,"blue");
//   }
//   tspane->update();
// }

// void
// VBView::SaveTS()
// {
//   VB_Vector mine=tspane->getInputVector(0);
//   if (mine.size()<1)
//     return;
//   QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save time series file","Choose a filename for your time series");
//   if (s==QString::null) return;

//   mine.setFileName(s.latin1());
//   if (mine.WriteFile()) {
//     QMessageBox::critical(this,"Error saving time series","Your time series could not be saved.",
//                           "I understand");
//   }
// }

// void
// VBView::SaveGraph()
// {
//    QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image file","Choose a filename for your snapshot");
//   if (s==QString::null) return;

//   QPixmap::grabWidget(tspane).save(s.latin1(),"PNG");
// }

// void
// VBView::UpdateTS()
// {
//   if (panel_ts->isHidden())
//     return;
//   if (ts_mousebutton->isChecked())
//     q_tstype=tsmode_mouse;
//   else if (ts_maskbutton->isChecked())
//     q_tstype=tsmode_mask;
//   else if (ts_crossbutton->isChecked())
//     q_tstype=tsmode_cross;

//   if (glmi.teslist.size()==0 && tes.data) {
//     UpdateTesTS();
//     return;
//   }
//   if (glmi.teslist.size()==0)
//     return;

//   glmi.loadcombinedmask();
//   // get rid of the old vectors
//   while (tspane->getVecNum())
//     tspane->delVector(0);

//   tspane->setXCaption("volume number");
//   tspane->setYCaption("value");

//   // build a voxel list (option 0=position 1=mask 2=crosshairs 3=supra-threshold)
//   VBRegion myregion;
//   if (q_tstype==tsmode_mask && q_masks.size() && q_currentmask>-1) {
//     for (int i=0; i<q_maskx; i++) {
//       for (int j=0; j<q_masky; j++) {
//         for (int k=0; k<q_maskz; k++) {
//           if (GetMaskValue(i,j,k)==q_masks[q_currentmask].f_index) {
//             if (glmi.mask.GetValue(i,j,k))
//               myregion.add(i,j,k);
//           }
//         }
//       }
//     }
//   }
//   else if (q_tstype==tsmode_cross) {
//     int mx,my,mz,sx,sy,sz;
//     img2coords(q_xslice,q_yslice,q_zslice,mx,my,mz,sx,sy,sz);
//     myregion.add(mx,my,mz);
//   }
//   else
//     myregion.add(q_mx,q_my,q_mz);

//   if (!(myregion.size()))
//     return;
  
//   long flags=0;
//   if (ts_detrendbox->isChecked()) flags|=DETREND;
//   if (ts_meanscalebox->isChecked()) flags|=MEANSCALE;

//   // average time series and residuals
//   // RAW TIME SERIES
//   if (tslist->isSelected(0) && glmi.teslist.size()) {
//     if (ts_pcabox->isChecked()) {
//       VBMatrix pca=glmi.getRegionComponents(myregion,flags);
//       VB_Vector vv;
//       if (pca.n>0) {
//         vv=pca.GetColumn(0);
//         tspane->addVector(vv,"red");
//       }
//       if (pca.n>1) {
//         vv=pca.GetColumn(1);
//         tspane->addVector(vv,"green");
//       }
//       if (pca.n>2) {
//         vv=pca.GetColumn(2);
//         tspane->addVector(vv,"blue");
//       }
//     }
//     else {
//       VB_Vector vv;
//       vv=glmi.getRegionTS(myregion,flags);
//       if (vv.size()==0)
//         return;
//       if (ts_filterbox->isChecked())
//         glmi.filterTS(vv);
//       if (ts_removebox->isChecked())
//         glmi.adjustTS(vv);
//       if (myaverage)
//         vv=myaverage->getTrialAverage(vv);
//       if (ts_powerbox->isChecked()) {
//         vv=fftnyquist(vv);
//         vv[0]=0;
//       }
//       tspane->addVector(vv,"blue");
//     }
//   }

//   int xx,yy,zz;

//   // FITTED VALUES
//   if (tslist->isSelected(1) && glmi.teslist.size()) {
//     // first, let's get the betas.  easiest just to regress
//     VB_Vector vv;
//     vv=glmi.getRegionTS(myregion,flags);
//     if (vv.size()==0)
//       return;
//     if (glmi.Regress(vv))
//       return;
//     // now grab the KG (or just G) matrix
//     VBMatrix KG;
//     if (KG.ReadMAT1(glmi.stemname+".KG"))
//       if (KG.ReadMAT1(glmi.stemname+".G"))
//         return;
//     // copy the betas of interest only
//     VBMatrix b2(KG.cols,1);
//     b2.zero();
//     for (int i=0; i<(int)glmi.interestlist.size(); i++)
//       b2.set(glmi.interestlist[i],0,glmi.betas[glmi.interestlist[i]]);
//     b2.SetColumn(0,glmi.betas);
//     KG*=b2;
//     VB_Vector tmp=KG.GetColumn(0);
//     if (myaverage)
//       tmp=myaverage->getTrialAverage(tmp);
//     if (ts_powerbox->isChecked()) {
//       tmp=fftnyquist(tmp);
//       tmp[0]=0;
//     }
//     tspane->addVector(tmp,"yellow");
//   }

//   // OLD FITTED VALUES - sum of scaled covariates
//   // FIXME grossly inefficient!  reads each covariate again for each voxel!
//   if (0&&tslist->isSelected(1) && glmi.teslist.size()) {
//     VB_Vector vv,vtotal;
//     for (int v=0; v<myregion.size(); v++) {
//       xx=myregion[v].x;
//       yy=myregion[v].y;
//       zz=myregion[v].z;
//       for (int i=0; i<(int)glmi.cnames.size(); i++) {
//         if (glmi.cnames[i][0]!='I')
//           continue;
//         if (vv.size()==0)
//           vv=glmi.getCovariate(xx,yy,zz,i,1);
//         else
//           vv+=glmi.getCovariate(xx,yy,zz,i,1);
//       }
//       if (v==0)
//         vtotal=vv;
//       else
//         vtotal+=vv;
//     }
//     vtotal/=myregion.size();
//     if (myaverage)
//       vtotal=myaverage->getTrialAverage(vtotal);
//     if (ts_powerbox->isChecked()) {
//       vtotal=fftnyquist(vtotal);
//       vtotal[0]=0;
//     }
//     tspane->addVector(vtotal,"yellow");
//   }

//   // RESIDUALS
//   if (tslist->isSelected(2) && glmi.teslist.size()) {
//     VB_Vector vv;
//     vv=glmi.getResid(myregion,flags);
//     if (myaverage)
//       vv=myaverage->getTrialAverage(vv);
//     if (ts_powerbox->isChecked()) {
//       vv=fftnyquist(vv);
//       vv[0]=0;
//     }
//     tspane->addVector(vv,"yellow");
//   }

//   // UNSCALED INDIVIDUAL COVARIATES (doesn't need the voxel list!)
//   if (glmi.cnames.size()) {
//     for (int i=0; i<(int)glmi.cnames.size(); i++) {
//       if (!tslist->isSelected(i+3)) continue;
//       VB_Vector vv;
//       vv=glmi.getCovariate(q_mx,q_my,q_mz,i,0);
//       if (myaverage)
//         vv=myaverage->getTrialAverage(vv);
//       if (ts_powerbox->isChecked()) {
//         vv=fftnyquist(vv);
//       vv[0]=0;
//     }
//      tspane->addVector(vv,"red");
//     }
//   }
//   tspane->update();
// }

int
VBView::NewBrightness(int newval)
{
  q_brightness=newval;
  setscale();
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  return (0);
}

int
VBView::NewContrast(int newval)
{
  q_contrast=newval;
  setscale();
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  return (0);
}

int
VBView::NewAverage()
{
  int ind=averagebox->currentItem();
  if (ind==0) {
    myaverage=(TASpec *)NULL;
    string status="time series averaging off";
    statusline->setText(status.c_str());
    reload_averages();
    UpdateTS();
    return 0;
  }
  if (ind > (int)trialsets.size())
    return 1;
  myaverage=&(trialsets[ind-1]);
  string status="time series averaging "+myaverage->name+ " selected - ";
  status+=strnum(myaverage->startpositions.size())+" trials (or whatever) averaged";
  statusline->setText(status.c_str());
  UpdateTS();
  return 0;
}

int
VBView::NewView(const QString &str)
{
  // FIXME handle orientation
  string orient=vb_toupper(cube.GetHeader("orientation:"));
  for (int i=0; i<2; i++) {
    if (orient[i]=='L') orient[i]='R';
    if (orient[i]=='I') orient[i]='S';
    if (orient[i]=='P') orient[i]='A';
  }

  if (str=="XYZ")
    BuildViews(vbv_axial);
  else if (str=="Axial")
    BuildViews(vbv_axial);
  else if (str=="Coronal")
    BuildViews(vbv_coronal);
  else if (str=="Sagittal")
    BuildViews(vbv_sagittal);
  else if (str=="Multi-View")
    BuildViews(vbv_multi);
  else if (str=="Tri-View")
    BuildViews(vbv_tri);
  else
    cout << "argh!" << endl;
  RenderAll();
  
  return 0;
}

int
VBView::NewMag(const QString &str)
{
  q_mag=strtol(str.latin1());
  UpdateScaling();
  return 0;
}

int
VBView::NewRadius (int radius)
{
  q_radius=radius;
  return 0;
}

int
VBView::NewZRadius (int state)
{
  if (state) q_usezradius=1;
  else q_usezradius=0;
  return 0;
}

int
VBView::ToggleData(bool flag)
{
  // FIXME do something!
  RenderAll();
  return (0);  // no error!
}

int
VBView::SavePNG()
{
  QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image file","Choose a filename for your snapshot");
  if (s==QString::null) return (0);

  QPixmap::grabWidget(drawarea).save(s.latin1(),"PNG");

  return (0);  // no error!
}

int
VBView::ToggleCrosshairs()
{
  if (q_crosshairs==0)
    q_crosshairs=1;
  else
    q_crosshairs=0;
  RenderAll();
  update();
  return 0;
}

int
VBView::HideOverlay(bool state)
{
  if (state)
    q_hideoverlay=1;
  else
    q_hideoverlay=0;
  if (statmap.data) {
    RenderAll();
    update();
  }
  return 0;
}

int
VBView::ToggleOrigin()
{
  if (q_showorigin==0)
    q_showorigin=1;
  else
    q_showorigin=0;
  if (q_showorigin==1) {
    q_xslice=cube.origin[0];
    q_yslice=cube.origin[1];
    q_zslice=cube.origin[2];
    for (int i=0; i<(int)viewlist.size(); i++) {
      if (viewlist[i].orient==MyView::vb_xy) {
        viewlist[i].position=q_zslice;
        PreCalcView(viewlist[i]);
      }
    }
    UpdateTS();
    UpdatePosition();
  }
  RenderAll();
  update();
  return 0;
}

int
VBView::ToggleScaling(bool state)
{
  if (!cube.data)
    return 0;
  if (cube.voxsize[0]==0.0 || cube.voxsize[1]==0.0 || cube.voxsize[2]==0.0)
    return 0;
  q_isotropic=state;
  UpdateScaling();
  return 0;
}

void
VBView::UpdateScaling()
{
  if (!cube.data)
    return;
  q_xscale=q_yscale=q_zscale=(float)q_mag/100.0;
  
  if (q_isotropic) {
    float minsize=cube.voxsize[0];
    if (cube.voxsize[1]<minsize) minsize=cube.voxsize[1];
    if (cube.voxsize[2]<minsize) minsize=cube.voxsize[2];
    q_xscale*=cube.voxsize[0]/minsize;
    q_yscale*=cube.voxsize[1]/minsize;
    q_zscale*=cube.voxsize[2]/minsize;
  }

  BuildViews(q_viewmode);
  RenderAll();
  update();
}

int
VBView::TSpane(bool state)
{
  q_showts=state;
  if (state)
    panel_ts->show();
  else
    panel_ts->hide();
  return 0;
}

void
VBView::SetTails(bool twotailed)
{
  q_twotailed=twotailed;
  if (statmap.data) {
    ReThreshold();
    RenderAll();
  }
}

void
VBView::LoadCorrelationVolume()
{
  if (!(cube.data))
    return;
  
  QString s;
  s=QFileDialog::getOpenFileName(q_currentdir.c_str(),"All (*.*)",this,"open time series file","Choose a 4D volume");
  if (s==QString::null)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  int ret=q_maptes.ReadFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (ret) return;

  statmap.SetVolume(q_maptes.dimx,q_maptes.dimy,q_maptes.dimz,vb_float);
  q_statxratio=cube.dimx/statmap.dimx;
  q_statyratio=cube.dimy/statmap.dimy;
  q_statzratio=cube.dimz/statmap.dimz;
  update_status();
  panel_stats->show();
  image2namebox->setText(s.ascii());
  BuildViews(q_viewmode);
  RenderAll();
}

void
VBView::ChangeTSType(int tstype)
{
  q_tstype=tstype;
  UpdateTS();
}

void
VBView::SetColumns(int newcols)
{
  q_columns=newcols;
  if (viewlist.size()>2) {
    BuildViews(q_viewmode);
    RenderAll();
  }
}

void
VBView::HighlightLow()
{
  if (!q_update) return;
  lowedit->setPaletteBackgroundColor(qRgb(220,160,160));
}

void
VBView::HighlightHigh()
{
  if (!q_update) return;
  highedit->setPaletteBackgroundColor(qRgb(220,160,160));
}

void
VBView::HighlightCluster()
{
  if (!q_update) return;
  clusteredit->setPaletteBackgroundColor(qRgb(220,160,160));
}

void
VBView::SetLow()
{
  q_lowthresh=strtod(lowedit->text().latin1());
  lowedit->setPaletteBackgroundColor(qRgb(255,255,255));
  lowedit->setText(strnum(q_lowthresh).c_str());
  if (!statmap.data)
    return;
  ReThreshold();
  RenderAll();
  q_scale->setscale(q_lowthresh,q_highthresh,q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
}

void
VBView::SetHigh()
{
  q_highthresh=strtod(highedit->text().latin1());
  highedit->setPaletteBackgroundColor(qRgb(255,255,255));
  highedit->setText(strnum(q_highthresh).c_str());
  if (!statmap.data)
    return;
  // ReThreshold();
  RenderAll();
  q_scale->setscale(q_lowthresh,q_highthresh,q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
}

void
VBView::SetCluster()
{
  q_clustersize=strtol(clusteredit->text().latin1());
  clusteredit->setPaletteBackgroundColor(qRgb(255,255,255));
  clusteredit->setText(strnum(q_clustersize).c_str());
  if (!statmap.data)
    return;
  ReThreshold();
  RenderAll();
}

void
VBView::ReThreshold()
{
  if (!q_update) return;
  vector<VBRegion> regions;
  // first, find regions in the backupmap
  // cout << "finding regions" << endl;
  if (q_twotailed)
    regions=findregions(backupmap,vb_agt,q_lowthresh);
  else
    regions=findregions(backupmap,vb_gt,q_lowthresh);
  // cout << "done" << endl;
  // clear the old statmap
  statmap*=0.0;
  // copy regions above cluster size to statmap
  vector<VBRegion>::iterator rr;
  int x,y,z;
  for (rr=regions.begin(); rr!=regions.end(); rr++) {
    if (rr->size()<q_clustersize) continue;
    for (int i=0; i<rr->size(); i++) {
      x=(*rr)[i].x;
      y=(*rr)[i].y;
      z=(*rr)[i].z;
      statmap.SetValue(x,y,z,backupmap.GetValue(x,y,z));
    }
  }
  // FIXME the below recreates some stuff that's supposed to aid in
  // faster rendering, but here seems to create an unfortunate
  // slowness whenever we change thresholds
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
}

int
VBView::ClearMask()
{
  int ret=QMessageBox::warning(this,"Verify Erase Mask","Are you sure you want to erase your mask?","Yes","Cancel","",0,1);
  if (ret==1) return(0);
  PushMask();
  ClearMasks();

  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  return (0);  // no error!
}

int
VBView::SelectMask(QTMaskWidget *mw)
{
  QTMaskWidget *oldmw;
  string maskinfo;
  for (int i=0; i<(int)q_masks.size(); i++) {
    oldmw=q_masks[i].f_widget;
    if (oldmw==mw) {
      q_currentmask=i;
      oldmw->f_selected=1;
      // maskinfo=q_masks[i].f_name+": "+strnum(q_masks[i].f_region.size())+" voxels";
    }
    else {
      oldmw->f_selected=0;
    }
    q_masks[i].f_widget->update();
  }
  UpdateTS();
  UpdateMaskStatus();
  // q_mstatus2->setText(maskinfo.c_str());
  return (0);  // no error!
}

void
VBView::UpdateMaskStatus()
{
  string maskinfo;
  if (q_currentmask>-1) {
    int i=q_currentmask;
    maskinfo=q_masks[i].f_name+": "+strnum(q_masks[i].f_region.size())+" voxels";
  }
  else
    maskinfo="no mask selected";
  q_mstatus2->setText(maskinfo.c_str());
}

int
VBView::SelectRegion(QTMaskWidget *mw)
{
  QTMaskWidget *oldmw;
  string maskinfo;
  q_currentregion=0;
  for (int i=0; i<(int)q_regions.size(); i++) {
    oldmw=q_regions[i].f_widget;
    if (oldmw==mw  && !oldmw->f_selected) {
      q_currentregion=i;
      oldmw->f_selected=1;
      if (oldmw->f_visible)
        RenderRegionOnAllViews(q_regions[i].f_region,q_regions[i].f_color);
      oldmw->update();
      maskinfo=q_regions[i].f_name+": "+strnum(q_regions[i].f_region.size())+" voxels";
    }
    else {
      if (oldmw->f_selected!=0) {
        q_maskmode=MASKMODE_ERASE;
        RenderRegionOnAllViews(q_regions[i].f_region,q_regions[i].f_color);
        q_maskmode=MASKMODE_DRAW;
      }
      oldmw->f_selected=0;
      oldmw->update();
    }
    q_regions[i].f_widget->update();
  }
  UpdateTS();
  q_sstatus2->setText(maskinfo.c_str());
  return (0);  // no error!
}

void
VBView::ShowMaskInfo(QTMaskWidget *mw)
{
  vector<VBV_Mask>::iterator start,end,mm;
  if (mw->type=='M') {
    start=q_masks.begin();
    end==q_masks.end();
  }
  else {
    start=q_regions.begin();
    end==q_regions.end();
  }

  for (mm=start; mm!=end; mm++) {
    if (mm->f_widget==mw) {
      cout << "[I] vbview mask      index: " << mm->f_index << endl;
      cout << "[I] vbview mask    visible: " << mm->f_widget->f_visible << endl;
      cout << "[I] vbview mask    opacity: " << mm->f_opacity << endl;
      cout << "[I] vbview mask regionsize: " << mm->f_region.vox.size() << endl;
      cout << "[I] vbview mask       name: " << mm->f_name << endl;
      cout << "[I] vbview mask      color: "
	   << mm->f_color.red() << " "
	   << mm->f_color.green() << " "
	   << mm->f_color.blue() << endl;
      return;
    }
  }
  cout << "[E] invalid mask selected (shouldn't happen)" << endl;
  return;
}

// FIXME below needs work!

void
VBView::ShowRegionStats()
{
  int err;
  for (MI m=q_masks.begin(); m!=q_masks.end(); m++) {
    
  }
  for (MI m=q_regions.begin(); m!=q_regions.end(); m++) {
    if (!m->f_widget->f_visible)
      continue;
    VB_Vector vv=glmi.getRegionTS(m->f_region,glmi.glmflags);
    err=glmi.Regress(vv);
    for (int i=0; i<(int)glmi.contrasts.size(); i++) {
      glmi.contrast=glmi.contrasts[i];
      VB_Vector c=glmi.contrast.contrast;
      err=glmi.calc_stat();
      if (err)
        cout << "error " << err << " calculating stat value" << endl;
      else
        cout << "region " << m->f_widget->label
             << " size " << m->f_region.size()
             << " contrast " << glmi.contrast.name
             << " stat " << glmi.statval
             << endl;
    }
  }
}

// FIXME would be useful!

void
VBView::ShowMaskStat(QTMaskWidget *mw)
{
  QMessageBox::warning(this,"Lazy Programmer Warning","This feature has not yet been implemented.","Okay");
  return;
  cout << "attempting to show a stat" << endl;
  VBV_Mask *mymask=NULL;
  for (int i=0; i<(int)q_masks.size(); i++) {
    if (q_masks[i].f_widget==mw) {
      mymask=&(q_masks[i]);
      break;
    }
  }
  if (!mymask)
    return;
  if (!glmi.teslist.size())
    return;
  int xx,yy,zz;
  VB_Vector vv;
  cout << glmi.teslist.size() << glmi.teslist[0] << endl;
  for (int v=0; v<(int)mymask->f_region.size(); v++) {
    cout << v << endl;
    xx=mymask->f_region[v].x;
    yy=mymask->f_region[v].y;
    zz=mymask->f_region[v].z;
    cout << v << endl;
    if (vv.getLength()==0)
      vv=glmi.getTS(xx,yy,zz,glmi.glmflags);
    else
      vv+=glmi.getTS(xx,yy,zz,glmi.glmflags);
    cout << v << endl;
  }
  cout << 99 << endl;
  vv /= mymask->f_region.size();
  int err=glmi.Regress(vv);
  if (err) return;

  double tval=0;
  VB_Vector betas,resid;
  // FIXME below assumes non-autocorrelated!
  cout << 44 << endl;
  // glmi.calculateBetas2_nocor(vv,glmi.gMatrix,resid,betas);
  cout << 55 << endl;
  VB_Vector cc(2);
  cc[0]=1;  cc[1]=0;
  // glmi.calc_t(betas,cc,tval);
  cout << 66 << endl;
  cout << "tval: " << tval << endl;
  return;
}

int
VBView::UpdateMask()
{
  // FIXME gruesomely inefficient, updates *all* masks
  for (int i=0; i<(int)q_masks.size(); i++)
    q_masks[i].f_color=q_masks[i].f_widget->color;
  RenderAll();
  return (0);  // no error!
}

int
VBView::UpdateRegion()
{
  // FIXME like above, does more than it needs to usually
  for (int i=0; i<(int)q_regions.size(); i++)
    q_regions[i].f_color=q_regions[i].f_widget->color;
  RenderAll();
  return (0);  // no error!
}

int
VBView::LoadOverlay()
{
  QString s;
  s=QFileDialog::getOpenFileName(q_currentdir.c_str(),"All (*.*)",this,"open statistical overlay","Choose a statistical map to overlay");
  if (s==QString::null) return (0);
  return LoadOverlay(s.latin1());
}

int
VBView::LoadOverlay(string fname)
{

  if (statmap.ReadFile(fname)) {
    QMessageBox::warning(this,"Couldn't Load Overlay","We were unable to load your statistical overlay file.","Okay");
    return 0;
  }
  backupmap=statmap;   // save backup so we can mask at will
  // check to see if we're in integral multiples and equal slices
  if (cube.dimx % statmap.dimx || statmap.dimy % statmap.dimy) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your overlay dimensions aren't integral divisors of your cube dimensions.","Okay");
  }
  else {
    q_statxratio=cube.dimx/statmap.dimx;
    q_statyratio=cube.dimy/statmap.dimy;
    q_statzratio=cube.dimz/statmap.dimz;
    update_status();
  }
  if (cube.dimz != statmap.dimz) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your data and your stat map have different numbers of slices.","Okay");
  }

  panel_stats->show();
  ReThreshold();
//   for (int i=0; i<(int)viewlist.size(); i++)
//     PreCalcView(viewlist[i]);
  RenderAll();
  overlaynamebox->setText(statmap.GetFileName().c_str());
  return 0;
}

int
VBView::SetOverlay(string fname)
{
  Cube cb;
  if (cb.ReadFile(fname))
    QMessageBox::critical(this,"Overlay loading error","Couldn't load your overlay.","Okay");
  else
    SetOverlay(cb);
  emit renameme(this,xfilename(fname));
  return 0;
}

int
VBView::SetOverlay(Cube &cb)
{
  statmap=cb;
  backupmap=statmap;   // save backup so we can mask at will
  // check to see if we're in integral multiples and equal slices
  if (cube.dimx % statmap.dimx || statmap.dimy % statmap.dimy) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your overlay dimensions aren't integral divisors of your cube dimensions.","Okay");
  }
  else {
    q_statxratio=cube.dimx/statmap.dimx;
    q_statyratio=cube.dimy/statmap.dimy;
    q_statzratio=cube.dimz/statmap.dimz;
    update_status();
  }
  if (cube.dimz != statmap.dimz) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your data and your stat map have different numbers of slices.","Okay");
  }
  
  panel_stats->show();
  ReThreshold();
  //   for (int i=0; i<(int)viewlist.size(); i++)
  //     PreCalcView(viewlist[i]);
  RenderAll();
  overlaynamebox->setText(statmap.GetFileName().c_str());
  return 0;
}

void
VBView::ClearMasks()
{
  for (int i=0; i<(int)q_masks.size(); i++) {
    maskview->removeChild(q_masks[i].f_widget);
    delete q_masks[i].f_widget;    
  }
  q_masks.clear();
  q_currentmask=-1;
  maskpos=0;
  RenderAll();
  update_status();
}

int
VBView::ClearRegions()
{
  for (int i=0; i<(int)q_regions.size(); i++) {
    regionview->removeChild(q_regions[i].f_widget);
    delete q_regions[i].f_widget;
  }
  q_regions.clear();
  q_currentregion=-1;
  regionpos=0;
  return 0;
}

int
VBView::SaveFullMap()
{
  int err;
  QString s=QFileDialog::getSaveFileName("Filename for map","All (*.*)",this,"save map","Map filename");
  if (s==QString::null) return 0;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  err=backupmap.WriteFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (err) {
    QMessageBox::warning(this,"Map File Writing Error","Couldn't save map.","Okay");
    return (101);
  }
  return 0;
}

int
VBView::SaveVisibleMap()
{
  // ReThreshold();
  int err;
  QString s=QFileDialog::getSaveFileName("Filename for map","All (*.*)",this,"save map","Map filename");
  if (s==QString::null) return 0;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  err=statmap.WriteFile(s.latin1());
  QApplication::restoreOverrideCursor();
  if (err) {
    QMessageBox::warning(this,"Map File Writing Error","Couldn't save map.","Okay");
    return (101);
  }
  return 0;
}

int
VBView::CopyRegions()
{
  for (int i=0; i<(int)q_regions.size(); i++) {
    if (q_regions[i].f_widget->f_visible) {
      VBV_Mask tmp=q_regions[i];
      NewMaskWidget(tmp);
    }
  }
  return 0;
}


// FIXME needs to be completely redone

int
VBView::LoadMask()
{
  fileview fv;
  fv.ShowDirectoriesAlso(1);
  fv.ShowImageInformation(1);
  fv.Go();
  tokenlist ff=fv.ReturnFiles();
  if (ff.size()<1)
    return 0;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // kill existing masks
  ClearMasks();

  for (int i=0; i<ff.size(); i++) {
    Cube mask;
    mask.dimx=cube.dimx;
    mask.dimy=cube.dimy;
    mask.dimz=cube.dimz;
    if (mask.ReadFile(ff[i])) {
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this,"Mask Loading Error","Couldn't load your selected mask.","Okay");

      return 101;
    }
    if (i==0) {
      // check to see if we're in integral multiples and equal slices
      if (cube.dimx % mask.dimx || cube.dimy % mask.dimy || cube.dimz % mask.dimz) {
        QMessageBox::warning(this,"Mask Dimension Error","Your mask dimensions aren't integral divisors of your cube dimensions.","Okay");
        QApplication::restoreOverrideCursor();
        return 102;
      }
      q_maskxratio=cube.dimx/mask.dimx;
      q_maskyratio=cube.dimy/mask.dimy;
      q_maskzratio=cube.dimz/mask.dimz;
      q_maskx=mask.dimx;
      q_masky=mask.dimy;
      q_maskz=mask.dimz;
      update_status();
    }
    else if (q_maskx!=mask.dimx||q_masky!=mask.dimy||q_maskz!=mask.dimz) {
      QMessageBox::warning(this,"Mask Dimension Error","Skipping mask with inconsistent dimensions.","Okay");
    }
    // create mask widgets from all mask colors with names
    for (size_t i=0; i<mask.maskspecs.size(); i++) {
      if (mask.maskspecs[i].name.size()) {
        VBV_Mask mm;
        mm.f_masktype='M';
        mm.f_opacity=50;
        mm.f_color=qRgb(mask.maskspecs[i].r,mask.maskspecs[i].g,mask.maskspecs[i].b);
        mm.f_index=i;
        mm.f_name=mask.maskspecs[i].name;
        NewMaskWidget(mm);
      }
    }

    // rebuild masktable
    int match;
    double val;
    for (int i=0; i<mask.dimx; i++) {
      for (int j=0; j<mask.dimy; j++) {
        for (int k=0; k<mask.dimz; k++) {
          val=mask.GetValue(i,j,k);
          if (val==0) continue;
          match=0;
          for (int m=0; m<(int)q_masks.size(); m++) {
            if (val==q_masks[m].f_index) {
              AddMaskIndex(i,j,k,m);
              match=1;
            }
          }
          if (!match) {
            VBV_Mask mm;
            mm.f_masktype='M';
            mm.f_opacity=50;
            mm.f_color=qRgb(255,200,200);
            mm.f_index=(int)val;
            mm.f_name="anonymous mask";
            NewMaskWidget(mm);
            SetMaskValue(i,j,k,(int)val);
          }
        }
      }
    }
  }
  
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  if (ff.size()==1)
    masknamebox->setText(ff(0));
  else
    masknamebox->setText("<multiple masks loaded>");
  QApplication::restoreOverrideCursor();
  return (0);  // no error!
}

int
VBView::SetMask(Cube &mask)
{
  // kill existing masks
  ClearMasks();

  if (1) {
    if (1) {
      // check to see if we're in integral multiples and equal slices
      if (cube.dimx % mask.dimx || cube.dimy % mask.dimy || cube.dimz % mask.dimz) {
        QMessageBox::warning(this,"Mask Dimension Error","Your mask dimensions aren't integral divisors of your cube dimensions.","Okay");
        QApplication::restoreOverrideCursor();
        return 102;
      }
      q_maskxratio=cube.dimx/mask.dimx;
      q_maskyratio=cube.dimy/mask.dimy;
      q_maskzratio=cube.dimz/mask.dimz;
      q_maskx=mask.dimx;
      q_masky=mask.dimy;
      q_maskz=mask.dimz;
      update_status();
    }
    else if (q_maskx!=mask.dimx||q_masky!=mask.dimy||q_maskz!=mask.dimz) {
      QMessageBox::warning(this,"Mask Dimension Error","Skipping mask with inconsistent dimensions.","Okay");
    }
    // create mask widgets from all mask colors with names
    for (size_t i=0; i<mask.maskspecs.size(); i++) {
      if (mask.maskspecs[i].name.size()) {
        VBV_Mask mm;
        mm.f_masktype='M';
        mm.f_opacity=50;
        mm.f_color=qRgb(mask.maskspecs[i].r,mask.maskspecs[i].g,mask.maskspecs[i].b);
        mm.f_index=i;
        mm.f_name=mask.maskspecs[i].name;
        NewMaskWidget(mm);
      }
    }

    // rebuild masktable
    int match;
    double val;
    for (int i=0; i<mask.dimx; i++) {
      for (int j=0; j<mask.dimy; j++) {
        for (int k=0; k<mask.dimz; k++) {
          val=mask.GetValue(i,j,k);
          if (val==0) continue;
          match=0;
          for (int m=0; m<(int)q_masks.size(); m++) {
            if (val==q_masks[m].f_index) {
              AddMaskIndex(i,j,k,m);
              match=1;
            }
          }
          if (!match) {
            VBV_Mask mm;
            mm.f_masktype='M';
            mm.f_opacity=50;
            mm.f_color=qRgb(255,200,200);
            mm.f_index=(int)val;
            mm.f_name="anonymous mask";
            NewMaskWidget(mm);
            SetMaskValue(i,j,k,(int)val);
          }
        }
      }
    }
  }
  
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  masknamebox->setText(mask.GetFileName());
  QApplication::restoreOverrideCursor();
  return (0);  // no error!
}

int
VBView::LoadMaskSample()
{
  Cube maskx;
  // following is just a stopgap, needs lots of fixing
  QString s=QFileDialog::getOpenFileName(".","All (*.*)",this,"open image file","Choose a file to view");
  if (s==QString::null) return (0);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (maskx.ReadFile(s.latin1()))
    return 101;

  // check to see if we're in integral multiples and equal slices
  if (cube.dimx % maskx.dimx || cube.dimy % maskx.dimy || cube.dimz % maskx.dimz) {
    QMessageBox::warning(this,"Mask Dimension Warning","Your mask dimensions aren't integral divisors of your cube dimensions.","Okay");
  }
  else {
    q_maskxratio=cube.dimx/maskx.dimx;
    q_maskyratio=cube.dimy/maskx.dimy;
    q_maskzratio=cube.dimz/maskx.dimz;
    q_maskx=cube.dimx;
    q_masky=cube.dimy;
    q_maskz=cube.dimz;
  }

  ClearMasks();
  NewMask();

  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
  masknamebox->setText("<none>");
  QApplication::restoreOverrideCursor();
  return (0);  // no error!
}

void
VBView::NewMask(char masktype)
{
  VBV_Mask mm;
  mm.f_masktype='M';
  mm.f_opacity=50;

  if (q_masks.size()==1) mm.f_color=qRgb(40,255,40);
  else if (q_masks.size()==2) mm.f_color=qRgb(120,120,255);
  else if (q_masks.size()==3) mm.f_color=qRgb(255,255,0);
  else if (q_masks.size()==4) mm.f_color=qRgb(0,255,255);
  else if (q_masks.size()==5) mm.f_color=qRgb(255,0,255);
  else mm.f_color=qRgb(255,0,0);

  mm.f_index=q_masks.size()+1;
  char buf[STRINGLEN];
  sprintf(buf,"mask %d",(int)q_masks.size()+1);
  mm.f_name=buf;
  NewMaskWidget(mm);
}

void
VBView::NewMaskWidget(VBV_Mask &mm)
{
  // here's the corresponding widget for the interface
  mm.f_widget=new QTMaskWidget(maskview->viewport(),"",mm.f_name.c_str());
  mm.f_widget->SetColor(mm.f_color);
  mm.f_widget->SetIndex(mm.f_index);
  mm.f_widget->setFixedWidth(maskview->viewport()->width());
  mm.f_widget->SetType(mm.f_masktype);
  maskview->addChild(mm.f_widget,2,maskpos);
  maskpos+=24;
  maskview->resizeContents(maskview->visibleWidth(),maskpos);
  //maskview->updateContents();
  //maskview->repaintContents();
  QObject::connect(maskview,SIGNAL(newsize(int,int)),mm.f_widget,SLOT(setwidth(int,int)));

  mm.f_widget->show();
  mm.f_widget->update();
  QObject::connect(mm.f_widget,SIGNAL(selected(QTMaskWidget *)),this,SLOT(SelectMask(QTMaskWidget *)));
  QObject::connect(mm.f_widget,SIGNAL(changed()),this,SLOT(UpdateMask()));
  QObject::connect(mm.f_widget,SIGNAL(inforequested(QTMaskWidget *)),this,SLOT(ShowMaskInfo(QTMaskWidget *)));
  QObject::connect(mm.f_widget,SIGNAL(statrequested(QTMaskWidget *)),this,SLOT(ShowMaskStat(QTMaskWidget *)));
  q_masks.push_back(mm);
  if (q_masks.size()==1) {
    q_currentmask=0;
    q_masks[0].f_widget->f_selected=1;
  }
}

void
VBView::NewRegionWidget(VBV_Mask &mm)
{
  // here's the corresponding widget for the interface
  mm.f_widget=new QTMaskWidget(regionview->viewport(),"",mm.f_name.c_str());
  mm.f_widget->SetColor(mm.f_color);
  mm.f_widget->SetIndex(mm.f_index);
  mm.f_widget->setFixedWidth(regionview->viewport()->width());
  mm.f_widget->SetType(mm.f_masktype);
  regionview->addChild(mm.f_widget,2,regionpos);
  regionpos+=24;
  regionview->resizeContents(regionview->visibleWidth(),regionpos);
  //maskview->updateContents();
  //maskview->repaintContents();
  QObject::connect(regionview,SIGNAL(newsize(int,int)),mm.f_widget,SLOT(setwidth(int,int)));

  mm.f_widget->show();
  mm.f_widget->update();
  QObject::connect(mm.f_widget,SIGNAL(selected(QTMaskWidget *)),this,SLOT(SelectRegion(QTMaskWidget *)));
  QObject::connect(mm.f_widget,SIGNAL(changed()),this,SLOT(UpdateRegion()));
  QObject::connect(mm.f_widget,SIGNAL(inforequested(QTMaskWidget *)),this,SLOT(ShowMaskInfo(QTMaskWidget *)));
  QObject::connect(mm.f_widget,SIGNAL(statrequested(QTMaskWidget *)),this,SLOT(ShowMaskStat(QTMaskWidget *)));
  q_regions.push_back(mm);
}

void
VBView::RegionHandleMenu(QTMaskWidget *mm,string msg)
{
  // cout << msg << endl;
}

int
VBView::LoadGLM()
{
  QString s;
  QString myfilter="Overlays (*.cub *.img *.nii *.dcm *.prm);;All Files (*.*)";
  s=QFileDialog::getOpenFileName(q_currentdir.c_str(),myfilter,this,"open statistical overlay","Choose a statistical map or GLM to overlay");
  if (s==QString::null) return (0);

  if (xgetextension(s.latin1())!="prm")
    LoadOverlay(s.latin1());  // FIXME pass file name

  // DYK

//   VBQT_GLMSelect glms(this,"",1);
//   if (glms.exec()==QDialog::Rejected)
//     return 102;

  glmi.setup(s.latin1());

  cout << 1 << endl;
  VB::VBContrastParamScalingWidget www(this,"hey hey hey");
  cout << 2 << endl;
  www.LoadContrastInfo(glmi.stemname);
  if (www.exec()==QDialog::Rejected)
    return 102;
  cout << 3 << endl;
  
  // init our glmi using info from the glms version
  glmi.contrast=*(www.selectedContrast());
  cout << 4 << endl;
  // find anatomy file
  if (!glmi.anatomyname.size() && !cube.data) {
    // find it
    QString s;
    s=QFileDialog::getOpenFileName(q_currentdir.c_str(),"All (*.*)",this,"open image file","Choose a file to view");
    if (s==QString::null)
      return 120;
    glmi.anatomyname=s.latin1();
  }
  if (glmi.anatomyname.size())
    SetImage(glmi.anatomyname);
  int err=glmi.calc_stat_cube();
  statmap=glmi.statcube;
  backupmap=statmap;
  cout << 5 << endl;
  glmi.statcube.WriteFile("xxx.cub");
  if (err) {
    QMessageBox::warning(this,"Bad stat map","Stat map didn't build, sorry!","Okay");
    return 101;
  }
  // set overlay name
  string tmp="[GLM statmap from "+glmi.stemname+"] contrast: "+glmi.contrast.name;
  overlaynamebox->setText(tmp.c_str());

  // check to see if we're in integral multiples and equal slices
  if (cube.dimx % statmap.dimx || statmap.dimy % statmap.dimy) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your overlay dimensions aren't integral divisors of your cube dimensions.","Okay");
  }
  else {
    q_statxratio=cube.dimx/statmap.dimx;
    q_statyratio=cube.dimy/statmap.dimy;
    q_statzratio=cube.dimz/statmap.dimz;
  }
  if (cube.dimz != statmap.dimz) {
    QMessageBox::warning(this,"Overlay Dimension Warning","Your data and your stat map have different numbers of slices.","Okay");
  }

  // mask should be same shape as functional data and empty
  q_maskxratio=q_statxratio;
  q_maskyratio=q_statyratio;
  q_maskzratio=q_statzratio;
  q_maskx=statmap.dimx;
  q_masky=statmap.dimy;
  q_maskz=statmap.dimz;
  ClearMasks();
  NewMask();
  update_status();

  panel_stats->show();
  ReThreshold();
//   for (int i=0; i<(int)viewlist.size(); i++)
//     PreCalcView(viewlist[i]);
  RenderAll();

  // populate the voxel surfing listbox
  tslist->clear();
  tspane->clear();
  tslist->insertItem("Raw time series");
  tslist->insertItem("Fitted values");
  tslist->insertItem("Residuals");
  for (int i=0; i<(int)glmi.cnames.size(); i++)
    tslist->insertItem(glmi.cnames[i].c_str()+1);
  tslist->setSelected(0,1);
  // populate the trial averaging listbox
  reload_averages();
  // make sure GLM-specific applicable buttons are there
  ts_filterbox->show();
  ts_removebox->show();
  ts_scalebox->show();
  // set detrend and meanscale checkboxes as for glm
  if (glmi.glmflags & MEANSCALE)
    ts_meanscalebox->setChecked(1);
  else
    ts_meanscalebox->setChecked(0);
  if (glmi.glmflags & DETREND)
    ts_detrendbox->setChecked(1);
  else
    ts_detrendbox->setChecked(0);
  trialsets=glmi.trialsets;
  
  return 0;
}

void
VBView::reload_averages()
{
  // clear the old stuff
  trialsets.clear();
  averagebox->clear();
  averagebox->insertItem("No averaging");
  // reload trialsets from GLM or file
  if (glmi.stemname.size()) {
    glmi.loadtrialsets();
    trialsets=glmi.trialsets;
  }
  else
    trialsets=parseTAFile(averagesfile);
  if (trialsets.size()==0)
    return;
  for (int i=0; i<(int)trialsets.size(); i++)
    averagebox->insertItem(trialsets[i].name.c_str());
  myaverage=(TASpec *)NULL;
}

void
VBView::update_status()
{
  char tmp[STRINGLEN];
  if (tes.data)
    sprintf(tmp,"%d x %d x %d, %d time points",tes.dimx,tes.dimy,tes.dimz,tes.dimt);
  else if (cube.data)
    sprintf(tmp,"%d x %d x %d",cube.dimx,cube.dimy,cube.dimz);
  else
    sprintf(tmp,"<none>");
  q_dstatus->setText(tmp);

  if (tes.data || cube.data)
    sprintf(tmp,"%d x %d x %d",q_maskx,q_masky,q_maskz);
  else
    sprintf(tmp,"<none>");
  q_mstatus->setText(tmp);

  string statstring;
  if (statmap.data) {
    sprintf(tmp,"%d x %d x %d",statmap.dimx,statmap.dimy,statmap.dimz);
    statstring=tmp;
    if (glmi.teslist.size()) {
      if (glmi.glmflags & DETREND)
        statstring+=" DETRENDED";
      if (glmi.glmflags & MEANSCALE)
        statstring+=" MEANSCALED";
    }
  }
  else
    statstring="<none>";
  q_sstatus->setText(statstring.c_str());
}

int
VBView::ApplyMasks()
{
  Cube newstatmap=statmap;
  newstatmap*=0.0;
  for (int i=0; i<(int)q_masks.size(); i++) {
    if (!(q_masks[i].f_widget->f_visible))
      continue;
    for (int j=0; j<(int)q_masks[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      newstatmap.SetValue(vv.x,vv.y,vv.z,statmap.GetValue(vv.x,vv.y,vv.z));
    }
  }
  statmap=newstatmap;
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  // ReThreshold();
  RenderAll();
  return 0;
}

int
VBView::UnMask()
{
  statmap=backupmap;
  ReThreshold();
  RenderAll();
  return 0;
}

int
VBView::SaveSeparateMasks()
{
  if (q_masks.size()==0) {
    QMessageBox::warning(this,"Mask File Writing Error","No mask currently loaded.","Okay");
    return (100);
  }
  int i,err;
  Cube savemask;
  stringstream tmp2;

  // delete the old header lines
  vector<string>newheader;
  tokenlist line;
  for (int i=0; i<(int)savemask.header.size(); i++) {
    line.ParseLine(savemask.header[i]);
    if (line[0]=="vb_maskspec:")
      continue;
    newheader.push_back(savemask.header[i]);
  }
  for (i=0; i<(int)q_masks.size(); i++) {
    if (!(q_masks[i].f_widget->f_visible))
      continue;
    //     if (q_masks[i].f_masktype!='M')
    //       continue;
    savemask.header=newheader;
    savemask.SetVolume(q_maskx,q_masky,q_maskz,vb_short);
    for (int j=0; j<(int)q_masks[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      // saved separate masks always use index of 1
      // savemask.SetValue(vv.x,vv.y,vv.z,q_masks[i].f_index);
      savemask.SetValue(vv.x,vv.y,vv.z,1);
    }

    char tmp[512];
    sprintf(tmp,"vb_maskspec: 1 %d %d %d \"%s\"",q_masks[i].f_color.red(),
            q_masks[i].f_color.green(),q_masks[i].f_color.blue(),
	    q_masks[i].f_name.c_str());
    savemask.AddHeader(tmp);
    sprintf(tmp,"Filename for mask %s",q_masks[i].f_name.c_str());
    tmp2.str("");
    tmp2 << xdirname(q_filename) + (string)"/" + xrootname(xfilename(q_filename)) +
      (string)"_"+q_masks[i].f_widget->label+".cub";
    // QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save mask",tmp);
    QString s=QFileDialog::getSaveFileName(tmp2.str().c_str(),"All (*.*)",this,"save mask",tmp);
    if (s==QString::null) return (0);
    savemask.SetFileName(s.latin1());
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    err=savemask.WriteFile();
    QApplication::restoreOverrideCursor();
    if (err) {
      QMessageBox::warning(this,"Mask File Writing Error","Couldn't save mask for unknown reason.","Okay");
      return (101);
    }
  }
  return 0;
}

void
VBView::ByteSwap()
{
  cube.byteswap();
  setscale();
  for (int i=0; i<(int)viewlist.size(); i++)
    PreCalcView(viewlist[i]);
  RenderAll();
}

void
VBView::LoadAverages()
{
  QString s;
  s=QFileDialog::getOpenFileName(q_currentdir.c_str(),"All (*.*)",this,"open averages file","Choose the file that contains your averages");
  if (s==QString::null) return;

  averagesfile=s.latin1();
  reload_averages();
}

int
VBView::SetOrigin()
{
  cube.origin[0]=q_xslice;
  cube.origin[1]=q_yslice;
  cube.origin[2]=q_zslice;
  SaveImage();
  CopyInfo();
  if (q_showorigin)
    RenderAll();
  return 0;
}

int
VBView::ChangeMask()
{
  if (!(cube.data)) {
    QMessageBox::warning(this,"Warning","You can't set your mask dimensions until you have some image data loaded","Okay");
    return 102;
  }
  string defdims=strnum(cube.dimx)+"x"+strnum(cube.dimy)+"x"+strnum(cube.dimz);
  QString s=
    QInputDialog::getText("Mask Dimensions",
                          "Please enter the new mask dimensions in a form like XxYxZ",
                          QLineEdit::Normal,
                          defdims.c_str());
  tokenlist args;
  args.SetSeparator("x, ");
  args.ParseLine(s.ascii());
  if (args.size()!=3)
    return 2;
  int xx=strtol(args[0]);
  int yy=strtol(args[1]);
  int zz=strtol(args[2]);
  if (cube.dimx%xx || cube.dimy%yy || cube.dimz%zz) {
    QMessageBox::warning(this,"Warning","Your mask dimensions must be divisors of your image dimensions (for now)","Okay");
    return 104;
  }
  q_maskxratio=cube.dimx/xx;
  q_maskyratio=cube.dimy/yy;
  q_maskzratio=cube.dimz/zz;
  q_maskx=xx;
  q_masky=yy;
  q_maskz=zz;
  ClearMasks();
  return 0;
}

int
VBView::SaveMask()
{
  if (q_masks.size()==0) {
    QMessageBox::warning(this,"Mask File Writing Error","No mask currently loaded.","Okay");
    return (100);
  }
  // delete the old header lines
  vector<string>newheader;
  tokenlist line;
  for (int i=0; i<(int)maskheader.size(); i++) {
    line.ParseLine(maskheader[i]);
    if (line[0]=="vb_maskspec:")
      continue;
    newheader.push_back(maskheader[i]);
  }
  Cube mask;
  mask.header=newheader;
  // add the header lines for the mask colors
  for (int i=0; i<(int)q_masks.size(); i++) {
    if (!(q_masks[i].f_widget->f_visible))
      continue;
    char tmp[512];
    sprintf(tmp,"vb_maskspec: %d %d %d %d \"%s\"",q_masks[i].f_index,
            q_masks[i].f_color.red(),q_masks[i].f_color.green(),q_masks[i].f_color.blue(),
            q_masks[i].f_widget->label.c_str());
    mask.AddHeader(tmp);
  }
  // BUILD THE ACTUAL MASK
  mask.SetVolume(q_maskx,q_masky,q_maskz,vb_short);
  mask.voxsize[0]=cube.voxsize[0]*q_maskxratio;
  mask.voxsize[1]=cube.voxsize[1]*q_maskyratio;
  mask.voxsize[2]=cube.voxsize[2]*q_maskzratio;
  mask.origin[0]=cube.origin[0]/q_maskxratio;
  mask.origin[1]=cube.origin[1]/q_maskyratio;
  mask.origin[2]=cube.origin[2]/q_maskzratio;
  for (int i=0; i<(int)q_masks.size(); i++) {
    for (int j=0; j<(int)q_masks[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      mask.SetValue(vv.x,vv.y,vv.z,q_masks[i].f_index);
    }
  }

  // FIXME should see if the mask already has a name
  QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save mask","Choose a filename for your mask");
  if (s==QString::null) return (0);
  mask.SetFileName(s.latin1());
  if (mask.WriteFile()) {
    QMessageBox::warning(this,"Mask File Writing Error","Couldn't save mask for unknown reason.","Okay");
    return (101);
  }
  q_maskdirty=0;
  masknamebox->setText(mask.GetFileName().c_str());
  return (0);  // no error!
}

int
VBView::SaveCombinedMask()
{
  if (q_masks.size()==0) {
    QMessageBox::warning(this,"Mask File Writing Error","No mask currently loaded.","Okay");
    return (100);
  }
  // delete the old header lines
  vector<string>newheader;
  tokenlist line;
  for (int i=0; i<(int)maskheader.size(); i++) {
    line.ParseLine(maskheader[i]);
    if (line[0]=="vb_maskspec:")
      continue;
    newheader.push_back(maskheader[i]);
  }
  Cube mask;
  mask.header=newheader;
  // BUILD THE ACTUAL MASK
  mask.SetVolume(q_maskx,q_masky,q_maskz,vb_short);
  for (int i=0; i<(int)q_masks.size(); i++) {
    for (int j=0; j<(int)q_masks[i].f_region.vox.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      mask.SetValue(vv.x,vv.y,vv.z,1);
    }
  }

  // FIXME should see if the mask already has a name
  QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save mask","Choose a filename for your mask");
  if (s==QString::null) return (0);
  mask.SetFileName(s.latin1());
  if (mask.WriteFile()) {
    QMessageBox::warning(this,"Mask File Writing Error","Couldn't save mask for unknown reason.","Okay");
    return (101);
  }
  q_maskdirty=0;
  return (0);  // no error!
}

int
VBView::SaveImage()
{
  if (!cube.data_valid) {
    QMessageBox::warning(this,"No image currently loaded.","No image currently loaded.");
    return (100);
  }
  if (!q_filename.size()) {
    QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image","Choose a filename for your image");
    if (s==QString::null) return (0);
    cube.SetFileName(s.latin1());
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (cube.WriteFile()) {
    QMessageBox::warning(this,"Image File Writing Error","Couldn't save image for unknown reason.","Okay");
    QApplication::restoreOverrideCursor();
    return (101);
  }
  QApplication::restoreOverrideCursor();
  q_imagedirty=0;
  imagenamebox->setText(cube.GetFileName().c_str());
  return (0);  // no error!
}

int
VBView::CutSlice()
{
  PushMask();
  for (int xx=0; xx<q_maskx; xx++) {
    for (int yy=0; yy<q_masky; yy++) {
      ClearMaskValue(xx,yy,q_mz); 
      q_maskmode=MASKMODE_ERASE;
      for (int i=0; i<(int)viewlist.size(); i++)
        UniRenderMask(viewlist[i],xx,yy,q_mz,QColor(0,0,0));
      q_maskmode=MASKMODE_DRAW;
      q_maskdirty=1;
      sview->setFocus();
    }
  }
  drawarea->updateVisibleNow();
  return (0);  // no error!
}

int
VBView::CopySlice()
{
  int mx=0,my=0,mz;
  mz=q_zslice;
  Draw2Mask(mx,my,mz);
  q_maskslice.f_region.clear();
  for (int i=0; i<(int)q_masks.size(); i++) {
    for (int j=0; j<(int)q_masks[i].f_region.size(); j++) {
      VBVoxel vv=q_masks[i].f_region.vox[j];
      vv.val=i;
      if (vv.z==mz)
	q_maskslice.f_region.vox.push_back(vv);
      continue;
    }
  }
  // cout << q_maskslice.f_region.size() << endl;
  return (0);  // no error!
}

int
VBView::PasteSlice()
{
  int mx=0,my=0,mz;
  mz=q_zslice;
  Draw2Mask(mx,my,mz);
  if (!q_maskslice.f_region.size())
    return 0;
  PushMask();
  for (int i=0; i<(int)q_maskslice.f_region.size(); i++) {
    int xx=q_maskslice.f_region[i].x;
    int yy=q_maskslice.f_region[i].y;
    int index=(int)(q_maskslice.f_region[i].val);
    QColor color=q_masks[index].f_color;
    // cout << i << " " << index << " " << xx << " " << q_mx << endl;
    for (int j=0; j<(int)viewlist.size(); j++)
      UniRenderMask(viewlist[j],xx,yy,mz,color);
    SetMaskIndex(xx,yy,mz,index);
    q_maskdirty=1;
  }
  drawarea->updateVisibleNow();
  return (0);  // no error!
}

int
VBView::FindAllRegions()
{
  ClearRegions();
  // FIXME -- put the region count in the pink tip bar
  // make sure region list is clear
  // call findregions on the statmap
  vector<VBRegion> statregions;
  if (q_twotailed)
    statregions=findregions(statmap,vb_agt,q_lowthresh);
  else
    statregions=findregions(statmap,vb_gt,q_lowthresh);
  // cout << "regions: " << statregions.size() << endl;
  int count=statregions.size();
  if (count>500) {
    count=500;
    QMessageBox::warning(this,"Too Many Regions","There are over 500 regions, we're just going to give you the first 500.","Okay");
  }
  // create the VBV_Masks and populate the widget
  q_currentregion=-1;
  for (int i=0; i<count; i++) {
    // q_regions has all the actual regions, regionview has the widgets
    VBV_Mask mm;
    mm.f_masktype='R';
    mm.f_opacity=50;
    mm.f_color=qRgb(240,80,200);
    mm.f_index=999;
    mm.f_name=(string)"Region "+strnum(i);
    mm.f_region=statregions[i];
    NewRegionWidget(mm);
  }
  return 0;
}

int
VBView::FindCurrentRegion()
{
  int mx,my,mz,sx,sy,sz;
  img2coords(q_xslice,q_yslice,q_zslice,mx,my,mz,sx,sy,sz);
  Cube mask=statmap;
  mask*=0;
  mask+=1.0;
  VBRegion rr=growregion(sx,sy,sz,statmap,mask,vb_agt,0.0);
  if (!rr.size())
    return 0;
  VBV_Mask mm;
  mm.f_masktype='R';
  mm.f_opacity=50;
  mm.f_color=qRgb(240,80,200);
  mm.f_index=999;
  mm.f_name=(string)"Region "+strnum(sx)+","+strnum(sy)+","+strnum(sz);
  mm.f_region=rr;
  NewRegionWidget(mm);
  SelectRegion(q_regions[q_regions.size()-1].f_widget);

  return 0;
}

void
VBView::CreateCorrelationMap()
{
  int sx=q_xslice;
  int sy=q_yslice;
  int sz=q_zslice;

  if (q_maptes.dimt<2) return;  // must be 4D data
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  VB_Vector ref,current;
  q_maptes.GetTimeSeries(sx,sy,sz);
  ref=q_maptes.timeseries;
  ref-=ref.getVectorMean();
  for (int i=0; i<statmap.dimx; i++) {
    for (int j=0; j<statmap.dimy; j++) {
      for (int k=0; k<statmap.dimz; k++) {
        if (!q_maptes.VoxelStored(i,j,k)) {
          statmap.SetValue(i,j,k,0);
          continue;
        }
        q_maptes.GetTimeSeries(i,j,k);
        current=q_maptes.timeseries;
        current-=current.getVectorMean();
        statmap.SetValue(i,j,k,correlation(ref,current));
      }
    }
  }
  backupmap=statmap;
  panel_stats->show();

  q_update=0;
  lowedit->setText("0.0");
  highedit->setText("1.0");
  q_lowthresh=0.0;
  q_highthresh=1.0;
  q_scale->setscale(0.0,1.0,q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2);
  q_update=1;

  char tmp[STRINGLEN];
  sprintf(tmp,"<correlation map: %d,%d,%d>",sx,sy,sz);
  overlaynamebox->setText(tmp);

  ReThreshold();
  RenderAll();
  QApplication::restoreOverrideCursor();
}

int
VBView::ShowAllMasks()
{
  for (int i=0; i<(int)q_masks.size(); i++)
    q_masks[i].f_widget->f_visible=1;
  RenderAll();
  return 0;
}

int
VBView::HideAllMasks()
{
  for (int i=0; i<(int)q_masks.size(); i++)
    q_masks[i].f_widget->f_visible=0;
  RenderAll();
  return 0;
}

int
VBView::Close()
{
  int ret=0;
  if (q_maskdirty)
    ret=QMessageBox::warning(this,"Unsaved Mask","Your mask hasn't been saved.  Exit anyway?","Yes","No","",0,1);
  if (ret==0)
    emit closeme(this);
  return (0);  // no error!
}

int
VBView::KeyPressEvent(QKeyEvent ke)
{
  if (!cube.data_valid)
    return 0;
  // ke.key() = Qt::Key_Down (Up,Left,Right)
  // Qt::Key_Enter, Return, Tab, Backspace, Escape, Delete, Home, End, F1..
  if (ke.ascii()=='a' && tes.data_valid)
    volumeslider->setValue(q_volume-1);
  else if (ke.ascii()=='s' && tes.data_valid)
    volumeslider->setValue(q_volume+1);
  else if (ke.ascii()=='z' || ke.key()==Qt::Key_Down)
    zsliceslider->setValue(q_zslice-1);
  else if (ke.ascii()=='x' || ke.key()==Qt::Key_Up)
    zsliceslider->setValue(q_zslice+1);
  else if (ke.key()==Qt::Key_Home || ke.ascii()=='o') {
    xsliceslider->setValue(cube.origin[0]);
    ysliceslider->setValue(cube.origin[1]);
    zsliceslider->setValue(cube.origin[2]);
  }
  else if (ke.key()==Qt::Key_End)
    zsliceslider->setValue(cube.dimz-1);
  else if (ke.key()==Qt::Key_Delete)
    CutSlice();
  else if (ke.key()==Qt::Key_S && (ke.state() & Qt::CTRL))
    SaveMask();
  // FIXME space to toggle current mask/region?
  return 0;
}

void
VBView::PushMask()
{
  q_savedmasks=q_masks;
}

void
VBView::PopMask()
{
  if (q_savedmasks.size())
    q_masks=q_savedmasks;
  RenderAll();
}

int
VBView::GetMaskValue(int x,int y,int z)
{
  for (int i=0; i<(int)q_masks.size(); i++)
    if (q_masks[i].f_region.contains(x,y,z))
        return q_masks[i].f_index;
  return 0;
}

int
VBView::GetMaskIndex(int x,int y,int z)
{
  for (int i=0; i<(int)q_masks.size(); i++)
    if (q_masks[i].f_region.contains(x,y,z))
        return i;
  return -1;
}

void
VBView::ClearMaskValue(int x,int y,int z)
{
  for (int i=0; i<(int)q_masks.size(); i++)
    q_masks[i].f_region.remove(x,y,z);
}

void
VBView::SetMaskValue(int x,int y,int z,int maskvalue)
{
  ClearMaskValue(x,y,z);
  for (int i=0; i<(int)q_masks.size(); i++)
    if (q_masks[i].f_index==maskvalue)
      q_masks[i].f_region.add_noquestions(x,y,z);
}

void
VBView::SetMaskIndex(int x,int y,int z,int maskindex)
{
  ClearMaskValue(x,y,z);
  q_masks[maskindex].f_region.add_noquestions(x,y,z);
}

void
VBView::AddMaskIndex(int x,int y,int z,int maskindex)
{
  q_masks[maskindex].f_region.add_noquestions(x,y,z);
}

int
VBView::MousePressEvent(QMouseEvent me)
{
  if (me.button()==Qt::LeftButton) {
    effectivemode=mode;
    if (mode==vbv_mask && me.state()&(Qt::ShiftButton|Qt::ControlButton))
      effectivemode=vbv_fill;
  }
  else {
    if (me.button()==Qt::MidButton)
      effectivemode=vbv_cross;
    else if (me.button()==Qt::RightButton)
      effectivemode=vbv_erase;
  }

  int ix,mx,iy,my,iz=0,mz=0;
  ix=mx=me.x(); iy=my=me.y(); 
  if (Draw2Mask(mx,my,mz))
    return 0;
  if (Draw2Img(ix,iy,iz))
    return 0;

  if (effectivemode==vbv_cross) {
    q_update=0;
    if (xslicebox->isVisible())
      xsliceslider->setValue(ix);
    if (yslicebox->isVisible())
      ysliceslider->setValue(iy);
    if (zslicebox->isVisible())
      zsliceslider->setValue(iz);
    q_update=1;
    RenderAll();
    if (q_tstype==tsmode_cross)
      UpdateTS();
    return 0;
  }

  PushMask();
  // FIXME handle fill more elegantly than this!
  if (effectivemode==vbv_fill) {
    if (q_masks[q_currentmask].f_index==GetMaskValue(mx,my,mz))
      return 0;
    FillPixel(mx,my,mz,(int)(GetMaskValue(mx,my,mz)));
    RenderAll();
    return 0;
  }
  
  if (effectivemode==vbv_erase)
    q_maskmode=MASKMODE_ERASE;
  ColorPixel(mx,my,mz);
  UpdateMaskStatus();
  q_maskmode=MASKMODE_DRAW;
  if (q_tstype==tsmode_mask)
    UpdateTS();
  return (0);
}

int
VBView::MouseMoveEvent(QMouseEvent me)
{
  if (me.state()&Qt::LeftButton)
    effectivemode=mode;
  else {
    if (me.state()&Qt::MidButton)
      effectivemode=vbv_cross;
    else if (me.state()&Qt::RightButton)
      effectivemode=vbv_erase;
    else
      effectivemode=vbv_none;
  }

  if (!cube.data_valid)
    return (0);
  int ix,mx,iy,my,iz=0,mz=0;
  ix=mx=me.x(); iy=my=me.y(); 
  if (Draw2Mask(mx,my,mz))
    return 0;
  if (Draw2Img(ix,iy,iz))
    return 0;
  if (effectivemode==vbv_cross) {
    q_update=0;
    if (xslicebox->isVisible())
      xsliceslider->setValue(ix);
    if (yslicebox->isVisible())
      ysliceslider->setValue(iy);
    if (zslicebox->isVisible())
      zsliceslider->setValue(iz);
    q_update=1;
    RenderAll();
    if (q_tstype==tsmode_cross)
      UpdateTS();
    return 0;
  }
  if (ix >= 0 && iy >= 0 && iz >= 0 &&
      ix<cube.dimx && iy<cube.dimy && iz <cube.dimz) {
    q_ix=ix; q_iy=iy; q_iz=iz;
    q_mx=mx; q_my=my; q_mz=mz;
  }
  UpdatePosition();
  if (!sview->hasFocus())
    sview->setFocus();

  if (effectivemode==vbv_mask)
    q_maskmode=MASKMODE_DRAW;
  else if (effectivemode==vbv_erase)
    q_maskmode=MASKMODE_ERASE;
  else
    return (0);

  ColorPixel(mx,my,mz);
  UpdateMaskStatus();
  q_maskmode=MASKMODE_DRAW;
  if (q_tstype==tsmode_mask)
    UpdateTS();
  return (0);
}

void
VBView::ColorPixel(int x,int y,int z)
{
  if (q_maskmode==MASKMODE_DRAW&&q_currentmask<0)
    return;
  int xx,yy,zz;
  int q_zradius=0;
  if (q_usezradius)
    q_zradius=q_radius;
  for (xx=x-q_radius; xx<=x+q_radius; xx++) {
    for (yy=y-q_radius; yy<=y+q_radius; yy++) {
      for (zz=z-q_zradius; zz<=z+q_zradius; zz++) {
        if (q_radius>0) {
          double dist=sqrt((double)((xx-x)*(xx-x))
                           +(double)((yy-y)*(yy-y))
                           +(double)((zz-z)*(zz-z)));
          if (dist>q_radius)
            continue;
        }
        if (xx > q_maskx-1 || xx<0)
          continue;
        if (yy > q_masky-1 || yy<0)
          continue;
        if (zz > q_maskz-1 || zz<0)
          continue;
        // set the actual mask value in the mask and masktable
        if (q_maskmode==MASKMODE_DRAW)
          SetMaskIndex(xx,yy,zz,q_currentmask);
        else
          ClearMaskValue(xx,yy,zz);
        // set the actual mask value in the mvals for each view
        for (int i=0; i<(int)viewlist.size(); i++)
          UniRenderMask(viewlist[i],xx,yy,zz,q_masks[q_currentmask].f_color);
        q_maskdirty=1;
        sview->setFocus();
      }
    }
  }
}

void
VBView::FillPixel(int x,int y,int z,int fromval)
{
  q_update=0;
  if (q_currentview->orient==MyView::vb_xy)
    FillPixelXY(x,y,z,fromval);
  else if (q_currentview->orient==MyView::vb_yz)
    FillPixelYZ(x,y,z,fromval);
  else if (q_currentview->orient==MyView::vb_xz)
    FillPixelXZ(x,y,z,fromval);
  q_update=1;
  UpdateMaskStatus();
}

void
VBView::FillPixelXY(int x,int y,int z,int fromval)
{
  deque<int>xs,ys;
  int xx,yy;

  xs.push_back(x);
  ys.push_back(y);

  while(xs.size()) {
    xx=xs[0]; yy=ys[0];
    if (xx > q_maskx-1 || xx<0) {
      xs.erase(xs.begin());
      ys.erase(ys.begin());
      continue;
    }
    if (yy > q_masky-1 || yy<0) {
      xs.erase(xs.begin());
      ys.erase(ys.begin());
      continue;
    }
    SetMaskIndex(xx,yy,z,q_currentmask);
    xs.erase(xs.begin());
    ys.erase(ys.begin());
    // note that if we push_back, bad things can happen
    if (GetMaskValue(xx-1,yy,z)==fromval) {
      xs.push_front(xx-1);
      ys.push_front(yy);
    }
    if (GetMaskValue(xx+1,yy,z)==fromval) {
      xs.push_front(xx+1);
      ys.push_front(yy);
    }
    if (GetMaskValue(xx,yy-1,z)==fromval) {
      xs.push_front(xx);
      ys.push_front(yy-1);
    }
    if (GetMaskValue(xx,yy+1,z)==fromval) {
      xs.push_front(xx);
      ys.push_front(yy+1);
    }
  }
}

void
VBView::FillPixelYZ(int x,int y,int z,int fromval)
{
  deque<int>ys,zs;
  int yy,zz;

  ys.push_back(y);
  zs.push_back(z);

  while(ys.size()) {
    yy=ys[0]; zz=zs[0];
    if (yy > q_masky-1 || yy<0) {
      ys.erase(ys.begin());
      zs.erase(zs.begin());
      continue;
    }
    if (zz > q_maskz-1 || zz<0) {
      ys.erase(ys.begin());
      zs.erase(zs.begin());
      continue;
    }
    SetMaskIndex(x,yy,zz,q_currentmask);
    ys.erase(ys.begin());
    zs.erase(zs.begin());
    // note that if we push_back, bad things can happen
    if (GetMaskValue(x,yy-1,zz)==fromval) {
      ys.push_front(yy-1);
      zs.push_front(zz);
    }
    if (GetMaskValue(x,yy+1,zz)==fromval) {
      ys.push_front(yy+1);
      zs.push_front(zz);
    }
    if (GetMaskValue(x,yy,zz-1)==fromval) {
      ys.push_front(yy);
      zs.push_front(zz-1);
    }
    if (GetMaskValue(x,yy,zz+1)==fromval) {
      ys.push_front(yy);
      zs.push_front(zz+1);
    }
  }
}

void
VBView::FillPixelXZ(int x,int y,int z,int fromval)
{
  deque<int>xs,zs;
  int xx,zz;

  xs.push_back(x);
  zs.push_back(z);

  while(xs.size()) {
    xx=xs[0]; zz=zs[0];
    if (xx > q_maskx-1 || xx<0) {
      xs.erase(xs.begin());
      zs.erase(zs.begin());
      continue;
    }
    if (zz > q_maskz-1 || zz<0) {
      xs.erase(xs.begin());
      zs.erase(zs.begin());
      continue;
    }
    SetMaskIndex(xx,y,zz,q_currentmask);
    xs.erase(xs.begin());
    zs.erase(zs.begin());
    // note that if we push_back, bad things can happen
    if (GetMaskValue(xx-1,y,zz)==fromval) {
      xs.push_front(xx-1);
      zs.push_front(zz);
    }
    if (GetMaskValue(xx+1,y,zz)==fromval) {
      xs.push_front(xx+1);
      zs.push_front(zz);
    }
    if (GetMaskValue(xx,y,zz-1)==fromval) {
      xs.push_front(xx);
      zs.push_front(zz-1);
    }
    if (GetMaskValue(xx,y,zz+1)==fromval) {
      xs.push_front(xx);
      zs.push_front(zz+1);
    }
  }
}

int
VBView::overlayvalue(double low,double high,double val,int &rr,int &gg,int &bb)
{
  if (abs(val) <= low)
    return -1;
  double pct=(abs(val)-low)/(high-low);
  if (pct>1.0)pct=1.0;
  if (val>0) {
    rr=q_poscolor1.red();
    rr+=(int)(pct*(q_poscolor2.red()-rr));
    gg=q_poscolor1.green();
    gg+=(int)(pct*(q_poscolor2.green()-gg));
    bb=q_poscolor1.blue();
    bb+=(int)(pct*(q_poscolor2.blue()-bb));
  }
  else {
    rr=q_negcolor1.red();
    rr+=(int)(pct*(q_negcolor2.red()-rr));
    gg=q_negcolor1.green();
    gg+=(int)(pct*(q_negcolor2.green()-gg));
    bb=q_negcolor1.blue();
    bb+=(int)(pct*(q_negcolor2.blue()-bb));
  }
  return 0;
}

// int
// VBView::FIXME_OLD_overlayvalue(double low,double high,double val,int &rr,int &gg,int &bb)
// {
//   if (abs(val) <= low)
//     return -1;

//   // calculate a 0 to 39 index on the red-yellow scale
//   int ind=(int)(40.0*(abs(val)-low)/(high-low));
//   if (ind > 39) ind=39;
  
//   if (val < 0) {
//     rr=0;
//     gg=(int)(6.36*ind+0.5);
//     bb=255;
//   }
//   else {
//     rr=255;
//     gg=(int)(6.36*ind+0.5);
//     bb=0;
//   }
//   return 0;
// }

// draw2mask and draw2img convert from screen coordinates to mask and
// img.  on input, only x and y need to be set meaningfully, since
// your screen probably has no depth.

int
VBView::Draw2Mask(int &x,int &y,int &z)
{
  int xrel,yrel;
  MyView *mv;
  for (int i=0; i<(int)viewlist.size(); i++) {
    mv=&(viewlist[i]);
    xrel=x-mv->xoff;
    yrel=y-mv->yoff;
    if (xrel<0 || xrel>=mv->width || yrel<0 || yrel>=mv->height)
      continue;

    if (mv->orient==MyView::vb_xy) {
      x=(int)(xrel/(q_xscale));
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=mv->position;
      x/=q_maskxratio;
      y/=q_maskyratio;
      z/=q_maskzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_yz) {
      x=mv->position;
      y=(int)(xrel/(q_yscale));
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      x/=q_maskxratio;
      y/=q_maskyratio;
      z/=q_maskzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_xz) {
      x=(int)(xrel/(q_xscale));
      y=mv->position;
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      x/=q_maskxratio;
      y/=q_maskyratio;
      z/=q_maskzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_zy) {
      x=mv->position;
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=cube.dimz-(int)(xrel/(q_zscale))-1;
      x/=q_maskxratio;
      y/=q_maskyratio;
      z/=q_maskzratio;
      q_currentview=mv;
      return 0;
    }
  }
  x=y=z=0;
  return 101;
}

int
VBView::Draw2Img(int &x,int &y,int &z)
{
  int xrel,yrel;
  MyView *mv;
  for (int i=0; i<(int)viewlist.size(); i++) {
    mv=&(viewlist[i]);
    xrel=x-mv->xoff;
    yrel=y-mv->yoff;
    if (xrel<0 || xrel>=mv->width || yrel<0|| yrel>=mv->height)
      continue;

    if (mv->orient==MyView::vb_xy) {
      x=(int)(xrel/(q_xscale));
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=mv->position;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_yz) {
      x=mv->position;
      y=(int)(xrel/(q_yscale));
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_xz) {
      x=(int)(xrel/(q_xscale));	
      y=mv->position;
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_zy) {
      x=mv->position;
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=cube.dimz-(int)(xrel/(q_zscale))-1;
      q_currentview=mv;
      return 0;
    }
  }
  x=y=z=0;
  return 101;
}


int
VBView::Draw2Stat(int &x,int &y,int &z)
{
  int xrel,yrel;
  MyView *mv;
  for (int i=0; i<(int)viewlist.size(); i++) {
    mv=&(viewlist[i]);
    xrel=x-mv->xoff;
    yrel=y-mv->yoff;
    if (xrel<0 || xrel>=mv->width || yrel<0 || yrel>=mv->height)
      continue;

    if (mv->orient==MyView::vb_xy) {
      x=(int)(xrel/(q_xscale));
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=mv->position;
      x/=q_statxratio;
      y/=q_statyratio;
      z/=q_statzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_yz) {
      x=mv->position;
      y=(int)(xrel/(q_yscale));
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      x/=q_statxratio;
      y/=q_statyratio;
      z/=q_statzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_xz) {
      x=(int)(xrel/(q_xscale));
      y=mv->position;
      z=cube.dimz-(int)(yrel/(q_zscale))-1;
      x/=q_statxratio;
      y/=q_statyratio;
      z/=q_statzratio;
      q_currentview=mv;
      return 0;
    }
    else if (mv->orient==MyView::vb_zy) {
      x=mv->position;
      y=cube.dimy-(int)(yrel/(q_yscale))-1;
      z=cube.dimz-(int)(xrel/(q_zscale))-1;
      x/=q_statxratio;
      y/=q_statyratio;
      z/=q_statzratio;
      q_currentview=mv;
      return 0;
    }
  }
  x=y=z=0;
  return 101;
}



void
VBView::ToggleDisplayPanel()
{
  if (panel_display->isHidden())
    panel_display->show();
  else
    panel_display->hide();
  
//  displayvisible = !displayvisible;
}

void
VBView::ToggleMaskPanel()
{
  if (panel_masks->isHidden())
    panel_masks->show();
  else
    panel_masks->hide();
  
//  masksvisible = !masksvisible;
}

void
VBView::ToggleStatPanel()
{
  if (panel_stats->isHidden())
    panel_stats->show();
  else
    panel_stats->hide();
    
//  statsvisible = !statsvisible;
}

void
VBView::ToggleTSPanel()
{
  if (panel_ts->isHidden())
    panel_ts->show();
  else
    panel_ts->hide();
}

void
VBView::ToggleHeaderPanel()
{
  if (panel_header->isHidden())
    panel_header->show();
  else
    panel_header->hide();
}

