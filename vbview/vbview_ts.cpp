
// vbview_ts.cpp
// vbview timeseries-related functions
// Copyright (c) 1998-2011 by The VoxBo Development Team

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

void VBView::UpdateTS(bool clearflag) {
  if (clearflag) {
    while (tspane->getVecNum()) tspane->delVector(0);
    tspane->update();
    return;
  }

  // Either the current or the first layer must have time series data.
  // Time series data can be either a loaded 4D volume or a valid GLM.
  if (layers.size() == 0) return;
  VBLayerI li = currentlayer;
  if (li == layers.end()) li = layers.begin();
  if (!li->tes && !li->glmi.teslist.size()) li = layers.begin();
  if (!li->tes && !li->glmi.teslist.size()) return;

  if (ts_window->isHidden()) return;

  uint32 flags = 0;
  if (ts_detrendbox->isChecked()) flags |= DETREND;
  if (ts_meanscalebox->isChecked()) flags |= MEANSCALE;

  // FIRST: GRAB THE TIME SERIES WE'RE GOING TO NEED (IF ANY)

  // iterate across the mask, translate into the dimensions of our tes
  // file.  build voxel list (mask, crosshairs, or current voxel) in
  // mask coordinates.  something like that.
  VBRegion myregion;
  if (ts_maskbox->isChecked()) {
    Cube &cb = li->rendercube;
    for (int i = 0; i < cb.dimx; i++) {
      for (int j = 0; j < cb.dimy; j++) {
        for (int k = 0; k < cb.dimz; k++) {
          if (cb.testValue(i, j, k)) myregion.add(i, j, k, 0.0);
        }
      }
    }
  } else
    myregion.add(li->x, li->y, li->z, 0.0);

  // re-init graph
  while (tspane->getVecNum()) tspane->delVector(0);
  tspane->setXCaption("volume number");
  tspane->setYCaption("value");

  if (!(myregion.size())) return;

  // handle pca for tes data
  if (li->tes && ts_pcabox->isChecked() && myregion.size() > 1) {
    // set widgets properly
    ts_filterbox->hide();
    ts_removebox->hide();
    ts_scalebox->hide();
    // do PCA
    VBMatrix components(li->tes.dimt, myregion.size());
    int ind = 0;
    for (VI vox = myregion.begin(); vox != myregion.end(); vox++) {
      li->tes.GetTimeSeries(vox->second.x, vox->second.y, vox->second.z);
      if (ts_meanscalebox->isChecked()) li->tes.timeseries.meanNormalize();
      if (ts_detrendbox->isChecked()) li->tes.timeseries.removeDrift();
      components.SetColumn(ind++, li->tes.timeseries);
    }
    VBMatrix pcax, E;
    VB_Vector lambdas, vv;
    if (pca(components, lambdas, pcax, E)) return;
    if (pcax.n > 0) {
      vv = pcax.GetColumn(0);
      tspane->addVector(vv, "red");
    }
    if (pcax.n > 1) {
      vv = pcax.GetColumn(1);
      tspane->addVector(vv, "green");
    }
    if (pcax.n > 2) {
      vv = pcax.GetColumn(2);
      tspane->addVector(vv, "blue");
    }
    return;
  }

  // handle tes data and exit
  if (li->tes) {
    // set widgets properly
    ts_filterbox->hide();
    ts_removebox->hide();
    ts_scalebox->hide();
    // now build the time series
    VB_Vector vv;
    for (VI myvox = myregion.begin(); myvox != myregion.end(); myvox++) {
      if (li->tes.GetTimeSeries(myvox->second.x, myvox->second.y,
                                myvox->second.z))
        continue;
      if (ts_meanscalebox->isChecked()) li->tes.timeseries.meanNormalize();
      if (ts_detrendbox->isChecked()) li->tes.timeseries.removeDrift();
      if (vv.getLength() != li->tes.timeseries.getLength())
        vv = li->tes.timeseries;
      else
        vv += li->tes.timeseries;
    }
    vv /= myregion.size();
    if (ts_powerbox->isChecked()) {
      vv = fftnyquist(vv);
      vv[0] = 0;
    }
    if (myaverage) vv = myaverage->getTrialAverage(vv);
    tspane->addVector(vv, "blue");
    tspane->update();
    return;
  }

  // the rest of this stuff is only for GLM layers, so:
  if (li->glmi.teslist.size() == 0) return;

  // set widgets properly for GLM
  ts_filterbox->show();
  ts_removebox->show();
  ts_scalebox->show();

  li->glmi.loadcombinedmask();

  // average time series and residuals
  // RAW TIME SERIES
  if (tslist->item(0)->isSelected()) {
    if (ts_pcabox->isChecked()) {
      VBMatrix pca = li->glmi.getRegionComponents(myregion, flags);
      VB_Vector vv;
      if (pca.n > 0) {
        vv = pca.GetColumn(0);
        tspane->addVector(vv, "red");
      }
      if (pca.n > 1) {
        vv = pca.GetColumn(1);
        tspane->addVector(vv, "green");
      }
      if (pca.n > 2) {
        vv = pca.GetColumn(2);
        tspane->addVector(vv, "blue");
      }
    } else {
      VB_Vector vv;
      vv = li->glmi.getRegionTS(myregion, flags);
      if (vv.size() == 0) return;
      if (ts_filterbox->isChecked()) li->glmi.filterTS(vv);
      if (ts_removebox->isChecked()) li->glmi.adjustTS(vv);
      if (myaverage) vv = myaverage->getTrialAverage(vv);
      if (ts_powerbox->isChecked()) {
        vv = fftnyquist(vv);
        vv[0] = 0;
      }
      tspane->addVector(vv, "blue");
    }
  }

  // int xx,yy,zz;

  // FITTED VALUES
  if (tslist->item(1)->isSelected()) {
    // first, let's get the betas.  easiest just to regress
    VB_Vector vv;
    vv = li->glmi.getRegionTS(myregion, flags);
    if (vv.size() == 0) return;
    if (li->glmi.Regress(vv)) return;
    // now grab the KG (or just G) matrix
    VBMatrix KG;
    if (KG.ReadFile(li->glmi.stemname + ".KG"))
      if (KG.ReadFile(li->glmi.stemname + ".G")) return;
    // copy the betas of interest only
    VBMatrix b2(KG.cols, 1);
    b2.zero();
    for (int i = 0; i < (int)li->glmi.interestlist.size(); i++)
      b2.set(li->glmi.interestlist[i], 0,
             li->glmi.betas[li->glmi.interestlist[i]]);
    b2.SetColumn(0, li->glmi.betas);
    KG *= b2;
    VB_Vector tmp = KG.GetColumn(0);
    tspane->addVector(tmp, "yellow");
  }

  // OLD FITTED VALUES - sum of scaled covariates
  // FIXME grossly inefficient!  reads each covariate again for each voxel!
  // if (0&&tslist->currentRow()==1 && li->glmi.teslist.size()) {
  //   VB_Vector vv,vtotal;
  //   for (int v=0; v<myregion.size(); v++) {
  //     xx=myregion[v].x;
  //     yy=myregion[v].y;
  //     zz=myregion[v].z;
  //     for (int i=0; i<(int)li->glmi.cnames.size(); i++) {
  //       if (li->glmi.cnames[i][0]!='I')
  //         continue;
  //       if (vv.size()==0)
  //         vv=li->glmi.getCovariate(xx,yy,zz,i,1);
  //       else
  //         vv+=li->glmi.getCovariate(xx,yy,zz,i,1);
  //     }
  //     if (v==0)
  //       vtotal=vv;
  //     else
  //       vtotal+=vv;
  //   }
  //   vtotal/=myregion.size();
  //   if (myaverage)
  //     vtotal=myaverage->getTrialAverage(vtotal);
  //   if (ts_powerbox->isChecked()) {
  //     vtotal=fftnyquist(vtotal);
  //     vtotal[0]=0;
  //   }
  //   tspane->addVector(vtotal,"yellow");
  // }

  // RESIDUALS
  if (tslist->item(2)->isSelected()) {
    VB_Vector vv;
    vv = li->glmi.getResid(myregion, flags);
    if (myaverage) vv = myaverage->getTrialAverage(vv);
    if (ts_powerbox->isChecked()) {
      vv = fftnyquist(vv);
      vv[0] = 0;
    }
    tspane->addVector(vv, "yellow");
  }

  // UNSCALED INDIVIDUAL COVARIATES (doesn't need the voxel list!)
  if (li->glmi.cnames.size()) {
    for (int i = 0; i < (int)li->glmi.cnames.size(); i++) {
      if (!(tslist->item(i + 3)->isSelected())) continue;
      VB_Vector vv;
      vv = li->glmi.getCovariate(li->x, li->y, li->z, i, 0);
      if (myaverage) vv = myaverage->getTrialAverage(vv);
      if (ts_powerbox->isChecked()) {
        vv = fftnyquist(vv);
        vv[0] = 0;
      }
      tspane->addVector(vv, "red");
    }
  }
  tspane->update();
}

void VBView::SaveTS() {
  VB_Vector mine = tspane->getInputVector(0);
  if (mine.size() < 1) return;
  QString s = QFileDialog::getSaveFileName(
      ".", "All (*.*)", this, "save time series file",
      "Choose a filename for your time series");
  if (s == QString::null) return;

  mine.setFileName(s.latin1());
  if (mine.WriteFile()) {
    QMessageBox::critical(this, "Error saving time series",
                          "Your time series could not be saved.",
                          "I understand");
  }
}

void VBView::SaveGraph() {
  QString s =
      QFileDialog::getSaveFileName(".", "All (*.*)", this, "save image file",
                                   "Choose a filename for your snapshot");
  if (s == QString::null) return;

  QPixmap::grabWidget(tspane).save(s.latin1(), "PNG");
}
