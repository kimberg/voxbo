
// vbview_io.cpp
// load/set images and whatnot
// Copyright (c) 1998-2010 by The VoxBo Development Team

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

#include "fileview.h"
#include "makestatcub.h"
#include "qinputdialog.h"
#include "vbcontrast.h"
#include "vbview.h"

int VBView::SetImage(Cube &cb) {
  list<VBLayer> newlayers;
  newlayers.push_back(VBLayer());
  newlayers.front().cube = cb;
  return SetImage(newlayers);
}

int VBView::LoadImage(string fname) {
  list<VBLayer> newlayers;

  // get the filenames if needed
  if (!fname.size()) {
    fileview fv;
    fv.ShowDirectoriesAlso(1);
    fv.ShowImageInformation(1);
    vector<string> ff = fv.Go();
    if (ff.size() == 0) return 101;
    fname = ff[0];
  }
  q_currentdir = xdirname(fname);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  // by pushing it onto a temporary list, we can load the cube and
  // know we won't have to copy all the data later in order to splice
  // it into the main list
  newlayers.push_back(VBLayer());
  // read image data
  int err = newlayers.begin()->cube.ReadFile(fname);
  newlayers.begin()->filename = xfilename(fname);
  if (!err) {
    err = SetImage(newlayers);
    QApplication::restoreOverrideCursor();
    return err;
  }
  err = newlayers.begin()->tes.ReadFile(fname);
  if (!err) {
    err = SetImage(newlayers);
    QApplication::restoreOverrideCursor();
    return err;
  }
  QApplication::restoreOverrideCursor();
  return 101;
}

int VBView::SetImage(list<VBLayer> &newlayers) {
  VBLayerI tmp = newlayers.begin();
  if (layers.size() == 0) emit renameme(this, xfilename(tmp->filename));
  // if it's 4d data, grab the first cube
  if (tmp->tes.data) {
    tmp->tes.getCube(0, tmp->cube);
    tmp->dimt = tmp->tes.dimt;
  }
  // set some layer parameters and render -- FIXME!!!
  tmp->q_brightness = 50;
  tmp->q_contrast = 50;
  tmp->type = VBLayer::vb_struct;
  // FIXME should just use what's in the cube, don't need separate dimxyz
  tmp->dimx = tmp->cube.dimx;
  tmp->dimy = tmp->cube.dimy;
  tmp->dimz = tmp->cube.dimz;
  if (layers.size() == 0) {
    tmp->alpha = 100;
    tmp->transform.ident();
    base_dimx = tmp->dimx;
    base_dimy = tmp->dimy;
    base_dimz = tmp->dimz;
    base_dims = 3;
    if (tmp->tes.data) {
      base_dims = 4;
      base_dimt = tmp->dimt;
    }
  } else {
    tmp->alpha = 50;
    tmp->setAlignment(layers.begin());
  }
  newlayers.front().render();
  // splice it into the list
  layers.splice(layers.end(), newlayers);
  BuildViews(q_viewmode);
  RenderAll();
  updateLayerTable();

  xsliceslider->setRange(0, tmp->dimx - 1);
  xsliceslider->setValue(tmp->dimx / 2);
  ysliceslider->setRange(0, tmp->dimy - 1);
  ysliceslider->setValue(tmp->dimy / 2);
  zsliceslider->setRange(0, tmp->dimz - 1);
  zsliceslider->setValue(tmp->dimz / 2);
  if (tmp->tes.data) {
    tsliceslider->setRange(0, tmp->dimt - 1);
    tsliceslider->setValue(0);
  }
  QApplication::restoreOverrideCursor();
  return 0;
}

int VBView::NewMask() {
  if (layers.size() == 0) {
    QMessageBox::warning(
        this, "No Base Layer",
        "Your have to load an image before you can create a mask.", "Okay");
    return 1;
  }
  VBQTnewmasklayer foo(this, layers.begin()->cube);
  if (!(foo.exec())) return 1;
  list<VBLayer> newlayers;
  newlayers.push_back(VBLayer());
  newlayers.begin()->cube.SetVolume(foo.retx, foo.rety, foo.retz, vb_int16);
  return SetMask(newlayers);
}

void VBView::NewOutline() {
  if (currentlayer == layers.end()) {
    QMessageBox::warning(
        this, "No Base Layer",
        "Your have to load an image before you can create a mask.", "Okay");
    return;
  }
  list<VBLayer> newlayers;
  newlayers.push_back(VBLayer());
  Cube tmpc = currentlayer->rendercube;
  tmpc.quantize(1);
  tmpc.convert_type(vb_byte);
  newlayers.begin()->cube = tmpc;
  enlarge(newlayers.begin()->cube, 1, 1, 1, 0);
  newlayers.begin()->cube -= tmpc;
  SetMask(newlayers);
}

int VBView::LoadMask(string fname) {
  if (layers.size() == 0) {
    QMessageBox::warning(
        this, "No Base Layer",
        "Your have to load an image before you can load a mask.", "Okay");
    return 1;
  }
  vector<string> fnames;

  // get the filenames if needed
  if (!fname.size()) {
    fileview fv;
    fv.ShowDirectoriesAlso(1);
    fv.ShowImageInformation(1);
    fnames = fv.Go();
    if (fnames.size() == 0) return 101;
  } else
    fnames.push_back(fname);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  vbforeach(string ff, fnames) {
    list<VBLayer> newlayers;
    q_currentdir = xdirname(ff);
    // by pushing it onto a temporary list, we can load the cube and
    // know we won't have to copy all the data later in order to splice
    // it into the main list
    newlayers.push_back(VBLayer());
    // read image data
    if (newlayers.begin()->cube.ReadFile(ff))
      QMessageBox::warning(this, "Mask Error",
                           "Error loading one of your masks.", "Okay");
    newlayers.begin()->filename = xfilename(ff);
    SetMask(newlayers);
  }
  QApplication::restoreOverrideCursor();
  return 0;
}

int VBView::SetMask(Cube &mask) {
  if (layers.size() == 0) {
    // FIXME complain with a messagebox maybe
    return 1;
  }
  list<VBLayer> newlayers;
  newlayers.push_back(VBLayer());
  newlayers.front().cube = mask;
  return SetMask(newlayers);
}

int VBView::SetMask(list<VBLayer> &newlayers) {
  VBLayerI tmp = newlayers.begin();
  // build a colortable if none exists
  tcolor tc;
  if (tmp->cube.maskspecs.size() == 0) {
    for (int i = 0; i < tmp->cube.dimx * tmp->cube.dimy * tmp->cube.dimz; i++) {
      uint32 val = tmp->cube.getValue<int32>(i);
      if (val == 0) continue;
      if (tmp->cube.maskspecs.count(val)) continue;
      VBMaskSpec ms;
      ms.r = tc.r;
      ms.g = tc.g;
      ms.b = tc.b;
      tc.next();
      ms.name = "Region " + strnum(val);
      tmp->cube.maskspecs[val] = ms;
    }
  }

  // set some layer parameters and render
  tmp->type = VBLayer::vb_mask;
  if (layers.size() == 0) {
    tmp->alpha = 100;
    tmp->transform.ident();
  } else {
    tmp->alpha = 50;
    tmp->setAlignment(layers.begin());
  }
  newlayers.front().render();
  // splice it into the list
  layers.splice(layers.end(), newlayers);
  BuildViews(q_viewmode);
  RenderAll();
  updateLayerTable();
  QApplication::restoreOverrideCursor();
  return 0;
}

int VBView::LoadStat(string fname) {
  if (layers.size() == 0) {
    QMessageBox::warning(
        this, "No Base Layer",
        "Your have to load an image before you can load a stat map.", "Okay");
    return 1;
  }
  vector<string> fnames;

  // get the filenames if needed
  if (!fname.size()) {
    fileview fv;
    fv.ShowDirectoriesAlso(1);
    fv.ShowImageInformation(1);
    fnames = fv.Go();
    if (fnames.size() == 0) return 101;
  } else
    fnames.push_back(fname);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  vbforeach(string ff, fnames) {
    list<VBLayer> newlayers;
    q_currentdir = xdirname(ff);
    // by pushing it onto a temporary list, we can load the cube and
    // know we won't have to copy all the data later in order to splice
    // it into the main list
    newlayers.push_back(VBLayer());
    VBLayerI tmp = newlayers.begin();
    // read image data
    if (tmp->cube.ReadFile(ff))
      QMessageBox::warning(this, "Map Error", "Error loading one of your maps.",
                           "Okay");
    newlayers.begin()->filename = xfilename(ff);
    // auto-scale to range
    tmp->q_thresh = 3.5;
    tmp->q_high = tmp->cube.get_maximum();
    // set some layer parameters and render
    tmp->type = VBLayer::vb_stat;
    tmp->alpha = 40;
    tmp->setAlignment(layers.begin());
    tmp->dimx = tmp->cube.dimx;
    tmp->dimy = tmp->cube.dimy;
    tmp->dimz = tmp->cube.dimz;
    newlayers.front().render();
    // newlayers.front().rethresh();
    // splice it into the list
    layers.splice(layers.end(), newlayers);
  }
  // BuildViews(q_viewmode);
  RenderAll();
  updateLayerTable();
  QApplication::restoreOverrideCursor();
  return 0;
}

int VBView::LoadGLM(string fname) {
  // get the filenames if needed
  if (!fname.size()) {
    fileview fv;
    fv.ShowDirectoriesAlso(1);
    fv.ShowImageInformation(1);
    fv.SetPattern("*.prm");
    vector<string> fnames = fv.Go();
    if (fnames.size() == 0) return 101;
    fname = fnames[0];
  }

  list<VBLayer> newlayers;
  q_currentdir = xdirname(fname);
  // by pushing it onto a temporary list, we can load the cube and
  // know we won't have to copy all the data later in order to splice
  // it into the main list
  newlayers.push_back(VBLayer());
  VBLayerI tmp = newlayers.begin();
  tmp->glmi.setup(fname);
  // if no base layer, try to load anatomy
  if (layers.size() == 0) LoadImage(tmp->glmi.anatomyname);
  if (layers.size() == 0) {
    QMessageBox::warning(this, "No Anatomy",
                         "No anatomical found on which to overlay your GLM.",
                         "Okay");
    return 1;
  }

  VB::VBContrastParamScalingWidget con(this, "contrast selector");
  con.LoadContrastInfo(tmp->glmi.stemname);
  if (con.exec() == QDialog::Rejected) return 2;
  if (con.selectedContrast() == NULL) return 3;
  tmp->glmi.contrast = *(con.selectedContrast());
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (tmp->glmi.calc_stat_cube()) {
    QMessageBox::warning(this, "GLM Error", "Error calculating your contrast.",
                         "Okay");
    return 3;
  }
  tmp->cube = tmp->glmi.statcube;
  newlayers.begin()->filename = xfilename(fname);
  // init threshold, store the params in layer structure
  tmp->glmi.initthresh();
  tmp->threshdata = tmp->glmi.thresh;
  stat_threshold(tmp->glmi.thresh);
  if (tmp->glmi.thresh.bonpeakthreshold < tmp->glmi.thresh.peakthreshold) {
    tmp->q_thresh = tmp->glmi.thresh.bonpeakthreshold;
    tmp->threshtt = "Bonferroni-corrected threshold set automatically\n";
  } else {
    tmp->q_thresh = tmp->glmi.thresh.peakthreshold;
    tmp->threshtt = "RFT-corrected threshold set automatically\n";
  }
  threshold v = tmp->threshdata;  // for convenience
  if (v.denomdf)
    tmp->threshtt +=
        (format("F(%g,%g), %d comparisons") % v.effdf % v.denomdf % v.numVoxels)
            .str();
  else
    tmp->threshtt +=
        (format("t(%g), %d comparisons") % v.effdf % v.numVoxels).str();

  tmp->q_high = tmp->cube.get_maximum();
  // set some layer parameters and render
  tmp->type = VBLayer::vb_glm;
  tmp->alpha = 40;
  tmp->setAlignment(layers.begin());
  tmp->dimx = tmp->cube.dimx;
  tmp->dimy = tmp->cube.dimy;
  tmp->dimz = tmp->cube.dimz;
  newlayers.front().render();
  // newlayers.front().rethresh();
  // splice it into the list
  layers.splice(layers.end(), newlayers);
  // populate the voxel surfing listbox
  tslist->clear();
  tspane->clear();
  tslist->addItem("Raw time series");
  tslist->addItem("Fitted values");
  tslist->addItem("Residuals");
  for (size_t i = 0; i < tmp->glmi.cnames.size(); i++)
    tslist->addItem(tmp->glmi.cnames[i].c_str() + 1);
  // tslist->setSelected(0,1);
  tslist->setCurrentRow(0);
  // populate the trial averaging listbox
  reload_averages();

  RenderAll();
  updateLayerTable();
  QApplication::restoreOverrideCursor();
  return 0;
}

int VBView::LoadCorr(string fname) {
  if (layers.size() == 0) {
    // FIXME complain with a messagebox maybe
    return 1;
  }
  list<VBLayer> newlayers;

  // get the filenames if needed
  if (!fname.size()) {
    fileview fv;
    fv.ShowDirectoriesAlso(1);
    fv.ShowImageInformation(1);
    vector<string> ff = fv.Go();
    if (ff.size() == 0) return 101;
    fname = ff[0];
  }
  q_currentdir = xdirname(fname);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  // by pushing it onto a temporary list, we can load the cube and
  // know we won't have to copy all the data later in order to splice
  // it into the main list
  newlayers.push_back(VBLayer());
  VBLayerI tmp = newlayers.begin();
  // read image data
  int err = tmp->tes.ReadFile(fname);
  if (err) {
    QApplication::restoreOverrideCursor();
    return 101;
  }
  // set some layer parameters and return.  note that we don't render
  // this layer immediately when it's loaded
  tmp->type = VBLayer::vb_corr;
  tmp->filename = xfilename(fname);
  tmp->q_visible = 1;
  tmp->alpha = 40;
  tmp->q_thresh = 0.0;
  tmp->q_high = 1.0;
  tmp->dimx = tmp->tes.dimx;
  tmp->dimy = tmp->tes.dimy;
  tmp->dimz = tmp->tes.dimz;
  tmp->tes.getCube(
      0, tmp->cube);  // so that it inherits alignment info, just in case
  tmp->cube.convert_type(vb_float);
  // tmp->cube.SetVolume(tmp->dimx,tmp->dimy,tmp->dimz,vb_float);
  tmp->setAlignment(layers.begin());
  tmp->render();
  // splice it into the list
  layers.splice(layers.end(), newlayers);
  RenderAll();
  updateLayerTable();
  QApplication::restoreOverrideCursor();
  return 0;
}

// int
// VBView::LoadAux(string fname)
// {
//   if (layers.size()==0) {
//     // FIXME complain with a messagebox maybe
//     return 1;
//   }
//   list<VBLayer> newlayers;

//   // get the filenames if needed
//   if (!fname.size()) {
//     fileview fv;
//     fv.ShowDirectoriesAlso(1);
//     fv.ShowImageInformation(1);
//     vector<string> ff=fv.Go();
//     if (ff.size()==0)
//       return 101;
//     fname=ff[0];
//   }
//   q_currentdir=xdirname(fname);
//   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//   // by pushing it onto a temporary list, we can load the cube and
//   // know we won't have to copy all the data later in order to splice
//   // it into the main list
//   newlayers.push_back(VBLayer());
//   VBLayerI tmp=newlayers.begin();
//   // read image data
//   int err=tmp->cube.ReadFile(fname);
//   if (err) {
//     QApplication::restoreOverrideCursor();
//     return 101;
//   }
//   // set some layer parameters and render
//   tmp->type=VBLayer::vb_aux;
//   tmp->q_visible=0;
//   tmp->filename=fname;
//   tmp->dimx=tmp->cube.dimx;
//   tmp->dimy=tmp->cube.dimy;
//   tmp->dimz=tmp->cube.dimz;
//   tmp->setAlignment(layers.begin());
//   // splice it into the list
//   layers.splice(layers.end(),newlayers);
//   updateLayerTable();
//   QApplication::restoreOverrideCursor();
//   return 0;
// }
