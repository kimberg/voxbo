
// vbview_ts.cpp
// vbview timeseries-related functions
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

void
VBView::UpdateTS()
{
  if (panel_ts->isHidden())
    return;

  // FIRST: GRAB THE TIME SERIES WE'RE GOING TO NEED (IF ANY)

  // iterate across the mask, translate into the dimensions of our tes
  // file.  build voxel list (mask, crosshairs, or current voxel) in
  // mask coordinates.  something like that.
  VBRegion myregion;
  if (ts_maskbutton->isChecked() && q_masks.size() && q_currentmask>-1) {
    for (int i=0; i<q_maskx; i++) {
      for (int j=0; j<q_masky; j++) {
        for (int k=0; k<q_maskz; k++) {
          if (GetMaskValue(i,j,k)==q_masks[q_currentmask].f_index) {
            if (glmi.teslist.size()) {
              if (glmi.mask.GetValue(i,j,k))
                myregion.add(i,j,k);
            }
            else if (tes.data) {
              if (tes.GetMaskValue(i,j,k))
                myregion.add(i,j,k);
            }
          }
        }
      }
    }
  }
  else if (ts_crossbutton->isChecked()) {
    int mx,my,mz,sx,sy,sz;
    img2coords(q_xslice,q_yslice,q_zslice,mx,my,mz,sx,sy,sz);
    myregion.add(mx,my,mz);
  }
  else
    myregion.add(q_mx,q_my,q_mz);

  // re-init graph
  while (tspane->getVecNum())
    tspane->delVector(0);
  tspane->setXCaption("volume number");
  tspane->setYCaption("value");

  if (!(myregion.size()))
    return;

  // handle tes data and exit
  if (glmi.teslist.size()==0 && tes.data) {
    VB_Vector vv;
    for (int v=0; v<(int)myregion.size(); v++) {
      if (tes.GetTimeSeries(myregion[v].x,myregion[v].y,myregion[v].z))
        continue;
      //cout << myregion[v].x << " " << myregion[v].y << " " << myregion[v].z << endl;
      //tes.timeseries.print();
      if (ts_meanscalebox->isChecked())
        tes.timeseries.meanNormalize();
      if (ts_detrendbox->isChecked())
        tes.timeseries.removeDrift();
      if (vv.getLength()!=tes.timeseries.getLength())
        vv=tes.timeseries;
      else
        vv+=tes.timeseries;
    }
    vv /= myregion.size();
    if (ts_powerbox->isChecked()) {
      vv=fftnyquist(vv);
      vv[0]=0;
    }
    if (myaverage)
      vv=myaverage->getTrialAverage(vv);
    tspane->addVector(vv,"blue");
    tspane->update();
    return;
  }

  if (glmi.teslist.size()==0)
    return;

  glmi.loadcombinedmask();

  long flags=0;
  if (ts_detrendbox->isChecked()) flags|=DETREND;
  if (ts_meanscalebox->isChecked()) flags|=MEANSCALE;

  // average time series and residuals
  // RAW TIME SERIES
  if (tslist->isSelected(0) && glmi.teslist.size()) {
    if (ts_pcabox->isChecked()) {
      VBMatrix pca=glmi.getRegionComponents(myregion,flags);
      VB_Vector vv;
      if (pca.n>0) {
        vv=pca.GetColumn(0);
        tspane->addVector(vv,"red");
      }
      if (pca.n>1) {
        vv=pca.GetColumn(1);
        tspane->addVector(vv,"green");
      }
      if (pca.n>2) {
        vv=pca.GetColumn(2);
        tspane->addVector(vv,"blue");
      }
    }
    else {
      VB_Vector vv;
      vv=glmi.getRegionTS(myregion,flags);
      if (vv.size()==0)
        return;
      if (ts_filterbox->isChecked())
        glmi.filterTS(vv);
      if (ts_removebox->isChecked())
        glmi.adjustTS(vv);
      if (myaverage)
        vv=myaverage->getTrialAverage(vv);
      if (ts_powerbox->isChecked()) {
        vv=fftnyquist(vv);
        vv[0]=0;
      }
      tspane->addVector(vv,"blue");
    }
  }

  int xx,yy,zz;

  // FITTED VALUES
  if (tslist->isSelected(1) && glmi.teslist.size()) {
    // first, let's get the betas.  easiest just to regress
    VB_Vector vv;
    vv=glmi.getRegionTS(myregion,flags);
    if (vv.size()==0)
      return;
    if (glmi.Regress(vv))
      return;
    // now grab the KG (or just G) matrix
    VBMatrix KG;
    if (KG.ReadMAT1(glmi.stemname+".KG"))
      if (KG.ReadMAT1(glmi.stemname+".G"))
        return;
    // copy the betas of interest only
    VBMatrix b2(KG.cols,1);
    b2.zero();
    for (int i=0; i<(int)glmi.interestlist.size(); i++)
      b2.set(glmi.interestlist[i],0,glmi.betas[glmi.interestlist[i]]);
    b2.SetColumn(0,glmi.betas);
    KG*=b2;
    VB_Vector tmp=KG.GetColumn(0);
    tspane->addVector(tmp,"yellow");
  }

  // OLD FITTED VALUES - sum of scaled covariates
  // FIXME grossly inefficient!  reads each covariate again for each voxel!
  if (0&&tslist->isSelected(1) && glmi.teslist.size()) {
    VB_Vector vv,vtotal;
    for (int v=0; v<myregion.size(); v++) {
      xx=myregion[v].x;
      yy=myregion[v].y;
      zz=myregion[v].z;
      for (int i=0; i<(int)glmi.cnames.size(); i++) {
        if (glmi.cnames[i][0]!='I')
          continue;
        if (vv.size()==0)
          vv=glmi.getCovariate(xx,yy,zz,i,1);
        else
          vv+=glmi.getCovariate(xx,yy,zz,i,1);
      }
      if (v==0)
        vtotal=vv;
      else
        vtotal+=vv;
    }
    vtotal/=myregion.size();
    if (myaverage)
      vtotal=myaverage->getTrialAverage(vtotal);
    if (ts_powerbox->isChecked()) {
      vtotal=fftnyquist(vtotal);
      vtotal[0]=0;
    }
    tspane->addVector(vtotal,"yellow");
  }

  // RESIDUALS
  if (tslist->isSelected(2) && glmi.teslist.size()) {
    VB_Vector vv;
    vv=glmi.getResid(myregion,flags);
    if (myaverage)
      vv=myaverage->getTrialAverage(vv);
    if (ts_powerbox->isChecked()) {
      vv=fftnyquist(vv);
      vv[0]=0;
    }
    tspane->addVector(vv,"yellow");
  }

  // UNSCALED INDIVIDUAL COVARIATES (doesn't need the voxel list!)
  if (glmi.cnames.size()) {
    for (int i=0; i<(int)glmi.cnames.size(); i++) {
      if (!tslist->isSelected(i+3)) continue;
      VB_Vector vv;
      vv=glmi.getCovariate(q_mx,q_my,q_mz,i,0);
      if (myaverage)
        vv=myaverage->getTrialAverage(vv);
      if (ts_powerbox->isChecked()) {
        vv=fftnyquist(vv);
      vv[0]=0;
    }
     tspane->addVector(vv,"red");
    }
  }
  tspane->update();

  



}


void
VBView::SaveTS()
{
  VB_Vector mine=tspane->getInputVector(0);
  if (mine.size()<1)
    return;
  QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save time series file","Choose a filename for your time series");
  if (s==QString::null) return;

  mine.setFileName(s.latin1());
  if (mine.WriteFile()) {
    QMessageBox::critical(this,"Error saving time series","Your time series could not be saved.",
                          "I understand");
  }
}

void
VBView::SaveGraph()
{
   QString s=QFileDialog::getSaveFileName(".","All (*.*)",this,"save image file","Choose a filename for your snapshot");
  if (s==QString::null) return;

  QPixmap::grabWidget(tspane).save(s.latin1(),"PNG");
}

