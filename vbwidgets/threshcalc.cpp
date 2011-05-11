
// threshcalc.cpp
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
// original version written by Tom King, modified severely by Dan
// Kimberg

using namespace std;

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <q3textedit.h>
#include <QLayout>
#include <QLineEdit>
#include <QCloseEvent>
#include "myboxes.h"
#include "threshcalc.h"

tcalc::tcalc(threshold v_in,QWidget* parent,Qt::WFlags fl)
  : QDialog(parent,"tcalc",fl)
{
  v=v_in;
  setName("tcalc");
  QFormLayout *layout=new QFormLayout();
  setLayout(layout);

  lblVoxelNumber = new QLabel( this, "lblVoxelNumber" );
  txtVoxelNumber = new QLineEdit( this, "txtVoxelNumber" );
  layout->addRow(lblVoxelNumber,txtVoxelNumber);

  lblVoxelSizes = new QLabel( this, "lblVoxelSizes" );
  txtX = new QLineEdit( this, "txtX" );
  txtY = new QLineEdit(this, "txtY" );
  txtZ = new QLineEdit(this, "txtZ" );
  QHBox *hb=new QHBox;
  hb->addWidget(txtX);
  hb->addWidget(txtY);
  hb->addWidget(txtZ);
  layout->addRow(lblVoxelSizes,hb);

  lblFWHM = new QLabel( this, "lblFWHM" );
  txtFWHM = new QLineEdit(this, "txtFWHM" );
  layout->addRow(lblFWHM,txtFWHM);

  lblEffDf = new QLabel( this, "txtEffDf" );
  txtEffdf = new QLineEdit(this, "txtEffdf" );
  layout->addRow(lblEffDf,txtEffdf);

  lbldenomdf = new QLabel( this, "lbldenomdf" );
  txtDenomDf = new QLineEdit(this, "txtDenomDf" );
  layout->addRow(lbldenomdf,txtDenomDf);

  lblalpha = new QLabel( this, "lblalpha" );
  txtAlpha = new QLineEdit(this, "txtAlpha" );
  layout->addRow(lblalpha,txtAlpha);

  lblres=new QLabel(this,"lblres");
  layout->addRow(lblres);

  buttonbox=new QHBox;
  layout->addWidget(buttonbox);
  QPushButton *button=new QPushButton("Use");
  connect(button,SIGNAL(clicked()),this,SLOT(accept()));
  buttonbox->addWidget(button);
  button=new QPushButton("Cancel");
  connect(button,SIGNAL(clicked()),this,SLOT(reject()));
  buttonbox->addWidget(button);
  buttonbox->hide();
  languageChange();
  connect(txtVoxelNumber,SIGNAL(textChanged(const QString &)), this,SLOT(update()));
  connect(txtX,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  connect(txtY,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  connect(txtZ,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  connect(txtFWHM,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  connect(txtEffdf,SIGNAL(textChanged(const QString &)),this,SLOT(update()));  
  connect(txtDenomDf,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  connect(txtAlpha,SIGNAL(textChanged(const QString &)),this,SLOT(update()));
  update();   // initial calculation
}

void
tcalc::languageChange()
{
  setCaption( tr( "Threshold calculator" ) );
  lbldenomdf->setText( tr( "denom df:" ) );
  lblFWHM->setText( tr( "Smoothness\n(fwhm in mm):" ) );
  lblEffDf->setText( tr( "eff df:" ) );
  lblalpha->setText( tr( "alpha:" ) );
  lblVoxelSizes->setText( tr( "Voxel  Sizes (mm):" ) );
  lblVoxelNumber->setText( tr( "# voxels:" ) );
  txtVoxelNumber->insert(strnum(v.numVoxels).c_str());
  txtX->insert(strnum(v.vsize[0]).c_str());
  txtY->insert(strnum(v.vsize[1]).c_str());
  txtZ->insert(strnum(v.vsize[2]).c_str());
  txtFWHM->insert(strnum(v.fwhm).c_str());
  txtEffdf->insert(strnum(v.effdf).c_str());
  txtDenomDf->insert(strnum(v.denomdf).c_str());
  txtAlpha->insert(strnum(v.pValPeak).c_str());
  setFixedWidth(this->fontMetrics().width("W")*25);  // heuristic for window width
  update();
}

void
tcalc::update()
{
  bool f_ftest=0;
  v.fwhm = strtod(txtFWHM->text().toStdString());
  v.numVoxels = strtol(txtVoxelNumber->text().toStdString());
  v.pValPeak=strtod(txtAlpha->text().toStdString());
  string restext;
  v.effdf = strtod(txtEffdf->text().toStdString());
  v.denomdf = strtod(txtDenomDf->text().toStdString());
  if (v.denomdf>FLT_MIN) {
    f_ftest=1;
    restext=(format("Critical value for F(%g,%g):")%
             v.effdf%v.denomdf).str();
  }
  else {
    restext=(format("Critical value for t(%g):")%
             v.effdf).str();
  }
  v.searchVolume=lround(strtod(txtX->text().toStdString())*
                        strtod(txtY->text().toStdString())*
                        strtod(txtZ->text().toStdString()))*
    v.numVoxels;
  v.pValExtent=.05;
  v.clusterThreshold = .001;

  string r2="RFT threshold not available";
  string r3="Bonferroni threshold not available";
  stat_threshold(v);
  if (v.peakthreshold<1e99)
    r2=(format("RFT threshold: %g")%v.peakthreshold).str();
  if (v.bonpeakthreshold<1e99)
    r3=(format("Bonferroni threshold: %g")%v.bonpeakthreshold).str();
  restext+="\n   "+r2+"\n   "+r3;
  lblres->setText(restext.c_str());
  bfthresh=(v.bonpeakthreshold<1e99 ? v.bonpeakthreshold : nan("nan"));
  rftthresh=(v.peakthreshold<1e99 ? v.peakthreshold : nan("nan"));
}

double
tcalc::getbfthresh()
{
  return bfthresh;
}

double
tcalc::getrftthresh()
{
  return rftthresh;
}

double
tcalc::getbestthresh()
{
  if (!finite(bfthresh))
    return rftthresh;   // if they're both nans, this is still okay
  if (bfthresh<rftthresh)
    return bfthresh;
  return rftthresh;
}

threshold
tcalc::getthreshdata()
{
  return v;
}

void
tcalc::showbuttons()
{
  buttonbox->show();
}

void
tcalc::closeEvent( QCloseEvent *)
{
  reject();
  return;
}

