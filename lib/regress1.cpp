
// regress1.cpp
// VoxBo regression routines
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
// original version written by Dan Kimberg

// where should the determination about betas of interest be made?

// all of the following are GLMInfo methods, and should have a valid
// stemname and receive, at mininum, a timeseries.  when loading
// matrices, they should check to see if it's already loaded.

using namespace std;

#include "glmutil.h"

using namespace std;

class VBCovar {
 public:
  Tes tesdata;
  VB_Vector vecdata;
  int dependent;
};

void buildg(VBMatrix &G, int x, int y, int z, uint32 rankg, uint32 orderg,
            vector<VBCovar> &covs);

// new regress function that just does the regression and returns the
// results.  if there's no f1 matrix, first we try to load it, then we
// try to calculate it from g.  in the autocorrelated case, r and
// exofilt are loaded if not available.

int GLMInfo::Regress(VB_Vector &timeseries) {
  if (gMatrix.m == 0) {
    gMatrix.ReadFile(stemname + ".G");
    if (!gMatrix.m) return 200;
  }

  // m=rows=orderg=number of observations
  // n=cols=rankg=number of variables

  // load or create F1 if needed
  if (makeF1()) return 201;
  if (glmflags & AUTOCOR) {
    // load R, exofilt, tracerv, fft exofilt, all if needed
    if (rMatrix.m == 0) {
      rMatrix.ReadFile(stemname + ".R");
      if (!rMatrix.m) return 202;
    }
    if (exoFilt.getLength() == 0) {
      exoFilt.ReadFile(stemname + ".ExoFilt");
      if (exoFilt.getLength() == 0) return 203;
    }
    if (traceRV.getLength() == 0) {
      traceRV.ReadFile(stemname + ".traces");
      if (traceRV.getLength() == 0) return 204;
    }
    if (realExokernel.size() == 0 || imagExokernel.size() == 0) {
      realExokernel.resize(exoFilt.getLength());
      imagExokernel.resize(exoFilt.getLength());
      exoFilt.fft(realExokernel, imagExokernel);
      realExokernel[0] = 1.0;
      imagExokernel[0] = 0.0;
    }
  }
  // do the regression
  if (glmflags & AUTOCOR)
    calcbetas(timeseries);
  else
    calcbetas_nocor(timeseries);

  return 0;
}

int GLMInfo::RegressIndependent(VB_Vector &timeseries) {
  // if F1 matrix is not set, pinv the G matrix
  if (f1Matrix.m == 0) {
    f1Matrix.init(gMatrix.n, gMatrix.m);
    if (pinv(gMatrix, f1Matrix)) return 1;
  }
  calcbetas_nocor(timeseries);
  return 0;
}

int GLMInfo::VecRegress(vector<string> ivnames, string dvname) {
  bool f_int = 0;
  int int_col = -1;
  VBMatrix mat;
  VB_Vector vec, mydv;
  vector<VB_Vector> covs;
  vector<string> names;
  // m=rows=orderg=number of observations
  // n=cols=rankg=number of variables
  uint32 orderg = 0, rankg;
  vbforeach(string iv, ivnames) {
    if (iv[0] == 'I') {
      f_int = 1;
      int_col = covs.size();
      covs.push_back(vec);
      names.push_back("Intercept");
      continue;
    }
    iv = iv.substr(1);
    if (!(vec.ReadFile(iv))) {
      if (orderg == 0) orderg = vec.size();
      if (orderg != vec.size()) {
        cout << format("[E] inconsistent variable dimensions\n");
        return 101;
      }
      covs.push_back(vec);
      names.push_back(iv);
    } else if (!(mat.ReadFile(iv))) {
      if (orderg == 0) orderg = mat.rows;
      if (orderg != mat.rows) {
        cout << format("[E] inconsistent variable dimensions\n");
        return 102;
      }
      for (uint32 i = 0; i < mat.cols; i++) {
        covs.push_back(mat.GetColumn(i));
        names.push_back(iv + " column " + strnum(i));
      }
    } else {
      cout << format("[E] error loading %s as covariate\n") % iv;
      return 103;
    }
  }
  if (orderg == 0) {
    cout << format("[E] no valid IVs found\n");
    return 104;
  }
  if (f_int) {
    VB_Vector tmpv(orderg);
    tmpv *= 0.0;
    tmpv += 1.0;
    covs[int_col] = tmpv;
  }
  rankg = covs.size();
  gMatrix.resize(orderg, rankg);
  int col = 0;
  vbforeach(VB_Vector & cc, covs) {
    gMatrix.SetColumn(col, cc);
    col++;
  }

  if (mydv.ReadFile(dvname)) {
    cout << format("[E] couldn't load DV from %s\n") % dvname;
    return 105;
  }
  if (mydv.size() != orderg) {
    cout << format("[E] DV length doesn't match length of IVs\n");
    return 106;
  }

  // perform the regression
  if (makeF1()) return 201;
  permute_if_needed(mydv);
  calcbetas_nocor(mydv);
  // betas are now in variable betas
  betas.AddHeader("Your covariates, in order:");
  vbforeach(string name, names) betas.AddHeader((string) "IV: " + name);
  return 0;  // no error!
}

// the idea of volumeregress() is to build our set of covariates out
// of volume sets (dirs of cub files) or vectors (same vec for each
// voxel).

int GLMInfo::VolumeRegress(Cube mask, int part, int nparts,
                           vector<string> ivnames, string dvname,
                           vector<VBMatrix> &ivmats) {
  if (mask.datatype != vb_byte) return 137;
  // m=rows=orderg=number of observations
  // n=cols=rankg=number of variables
  uint32 orderg = 0, rankg;
  rankg = ivnames.size();
  if (rankg == 0) return 122;

  if (!(mask.data)) return 101;
  int dimx = mask.dimx, dimy = mask.dimy, dimz = mask.dimz;
  if (dimx == 0 || dimy == 0 || dimz == 0) return 102;

  // edit the mask for the part we're doing: count the voxels, then go
  // through again and zero the ones we don't want
  int voxelcount = 0;
  int index = 0;
  for (int k = 0; k < dimz; k++) {
    for (int j = 0; j < dimy; j++) {
      for (int i = 0; i < dimx; i++) {
        if (mask.testValueUnsafe<char>(index)) voxelcount++;
        index++;
      }
    }
  }
  int partsize = (int)ceilf((float)voxelcount / (float)nparts);
  int firstvox = partsize * (part - 1);
  int lastvox = (partsize * part) - 1;
  if (lastvox > voxelcount - 1) lastvox = voxelcount - 1;

  // new zero the ones outside our "part"
  index = 0;
  for (int k = 0; k < dimz; k++) {
    for (int j = 0; j < dimy; j++) {
      for (int i = 0; i < dimx; i++) {
        if (mask.testValueUnsafe<char>(index)) {
          if (index < firstvox || index > lastvox) mask.SetValue(i, j, k, 0.0);
          index++;
        }
      }
    }
  }

  // read the data for the dependent variable
  VBCovar depvar;
  if (depvar.tesdata.ReadFile(dvname)) {
    if (depvar.vecdata.ReadFile(dvname)) return 241;
  }
  if (depvar.tesdata.data)
    orderg = depvar.tesdata.dimt;
  else
    orderg = depvar.vecdata.size();
  if (orderg < 1) return 191;

  printf("[I] vbregress: volume contains %d voxels\n", dimx * dimy * dimz);
  printf("[I] vbregress: mask contains %d real voxels\n", voxelcount);
  if (part == 1 && nparts == 1)
    printf("[I] vbregress: doing the entire volume\n");
  else
    printf("[I] vbregress: we're doing parts %d through %d (%d total) now\n",
           firstvox, lastvox, lastvox - firstvox + 1);

  // iterate across covariates, grabbing what we need
  vector<VBCovar> covs;
  int volumeivcount = 0;  // number of ivs that are volume data
  int matind = 0;

  for (int i = 0; i < (int)ivnames.size(); i++) {
    VBCovar cc;
    if (ivnames[i][0] == 'I') {
      cc.vecdata.resize(orderg);
      cc.vecdata *= 0.0;
      cc.vecdata += 1.0;
      covs.push_back(cc);
    } else if (ivnames[i][0] == 'G') {
      if (ivmats[matind].m != orderg) return 201;
      for (uint32 j = 0; j < ivmats[matind].n; j++) {
        cc.vecdata = ivmats[matind].GetColumn(j);
        covs.push_back(cc);
      }
      continue;
    } else if (ivnames[i][0] == 'V') {
      if (cc.tesdata.ReadFile(ivnames[i].substr(1)))
        cc.vecdata.ReadFile(ivnames[i].substr(1));
      if (cc.tesdata.data) {
        if (dimx == 0) {
          dimx = cc.tesdata.dimx;
          dimy = cc.tesdata.dimy;
          dimz = cc.tesdata.dimz;
        } else if (cc.tesdata.dimx != dimx || cc.tesdata.dimy != dimy ||
                   cc.tesdata.dimz != dimz) {
          return 192;
        }
        if (cc.tesdata.dimt != (int)orderg) return 127;
        cc.tesdata.intersect(mask);
        volumeivcount++;
      } else if (cc.vecdata.size()) {
        if (cc.vecdata.size() != orderg) return 128;
      } else {
        return 129;
      }
      covs.push_back(cc);
    }
  }

  interestlist.clear();
  keeperlist.clear();
  for (int i = 0; i < (int)ivnames.size(); i++) {
    keeperlist.push_back(i);
    interestlist.push_back(i);
  }
  int betacount = keeperlist.size();
  printf("[I] beta count: %d\n[I] orderg: %d\n[I] rankg: %d\n", betacount,
         orderg, rankg);
  paramtes.init(dimx, dimy, dimz, betacount + 1, vb_float);
  residtes.init(dimx, dimy, dimz, orderg, vb_float);
  statcube.init(dimx, dimy, dimz, vb_float);
  rawcube.init(dimx, dimy, dimz, vb_float);

  // NOW FINALLY WE CAN DO SOME REGRESSION
  VB_Vector signal;
  if (depvar.vecdata.size()) {
    signal = depvar.vecdata;
    permute_if_needed(signal);
  }
  index = 0;
  for (int k = 0; k < dimz; k++) {
    for (int j = 0; j < dimy; j++) {
      for (int i = 0; i < dimx; i++) {
        if (mask.testValueUnsafe<char>(index)) {
          buildg(gMatrix, i, j, k, orderg, rankg, covs);
          if (depvar.tesdata.data) {
            depvar.tesdata.GetTimeSeries(i, j, k);
            signal = depvar.tesdata.timeseries;
            if (glmflags & MEANSCALE) signal.meanNormalize();
            if (glmflags & DETREND) signal.removeDrift();
            permute_if_needed(signal);
          }
          // force F1 (pinv(G)) to be recalculated if need be
          if (volumeivcount > 0) f1Matrix.clear();
          int err = RegressIndependent(signal);
          if (err) return 131;
          // bang the params into paramtes
          for (int m = 0; m < (int)keeperlist.size(); m++)
            paramtes.SetValue(i, j, k, m, betas[keeperlist[m]]);
          // FIXME need setValue<float> versions for tes::setvalue
          if (!(glmflags & EXCLUDEERROR))
            paramtes.SetValue(i, j, k, paramtes.dimt - 1,
                              betas[betas.getLength() - 1]);
          // bang the resids info residtes
          for (uint32 ind = 0; ind < orderg; ind++)
            residtes.SetValue(i, j, k, ind, residuals[ind]);
          // FIXME print percent done occasionally
          calc_stat();
          statcube.SetValue(i, j, k, statval);
          rawcube.SetValue(i, j, k, rawval);
        }
        index++;
      }
    }
  }
  return 0;
}

void buildg(VBMatrix &G, int x, int y, int z, uint32 rankg, uint32 orderg,
            vector<VBCovar> &covs) {
  // needinit just tells us if the G matrix already had data in it
  int needinit = 0;
  if (G.m != rankg || G.n != orderg) {
    G.init(rankg, orderg);
    needinit = 1;
  }
  for (int i = 0; i < (int)covs.size(); i++) {
    // if this variable has tes data, it varies spatially and needs to be set
    if (covs[i].tesdata.data) {
      covs[i].tesdata.GetTimeSeries(x, y, z);
      G.SetColumn(i, covs[i].tesdata.timeseries);
    }
    // otherwise we only need to set it the first time through
    else if (needinit)
      G.SetColumn(i, covs[i].vecdata);
  }
}

int GLMInfo::TesRegress(int part, int nparts, uint32 flags) {
  if (teslist.size() == 0) return 55;
  tesgroup.resize(teslist.size());
  int dimx = 0, dimy = 0, dimz = 0, dimt = 0;
  int i, j, k, m;
  vector<string> newhdr;

  // first create the intersection mask
  loadcombinedmask();
  // check dims
  dimx = tesgroup[0].dimx;
  dimy = tesgroup[0].dimy;
  dimz = tesgroup[0].dimz;
  dimt = tesgroup[0].dimt;
  for (i = 1; i < (int)teslist.size(); i++) {
    dimt += tesgroup[i].dimt;
    // sanity check all the dimensions
    if (tesgroup[i].dimx != dimx || tesgroup[i].dimy != dimy ||
        tesgroup[i].dimz != dimz)
      return 102;
  }

  gMatrix.ReadFile(stemname + ".G");
  if (!gMatrix.m) return 102;

  // count the voxels, create a vector
  // int voxelcount=0;
  vector<int> xs, ys, zs;
  for (i = 0; i < dimx; i++) {
    for (j = 0; j < dimy; j++) {
      for (k = 0; k < dimz; k++) {
        if (mask.testValue(i, j, k)) {
          xs.push_back(i);
          ys.push_back(j);
          zs.push_back(k);
        }
      }
    }
  }
  int partsize = (int)ceilf((float)xs.size() / (float)nparts);
  int firstvox = partsize * (part - 1);
  int lastvox = (partsize * part) - 1;
  if (lastvox > (int)xs.size() - 1) lastvox = xs.size() - 1;

  printf("[I] vbregress: volume contains %d voxels\n", dimx * dimy * dimz);
  printf("[I] vbregress: volume contains %d real voxels\n", (int)xs.size());
  printf("[I] vbregress: we're doing %d through %d now\n", firstvox, lastvox);
  printf("[I] vbregress: regressing %d voxels\n", lastvox - firstvox + 1);
  if (dependentindex > -1) {
    printf("[I] vbregress: using covariate %d (%s) as the dependent variable\n",
           dependentindex, cnames[dependentindex].c_str() + 1);
  }
  if (perm_signs.size()) printf("[I] vbregress: using sign permutation\n");
  if (perm_order.size()) printf("[I] vbregress: using order permutation\n");
  int interval = (lastvox - firstvox) / 20;
  if (interval < 5) interval = lastvox - firstvox + 10;

  // set up sub-residual collection
  int nresids = 64;
  int stride = 1;
  if (nresids > dimt)
    nresids = dimt;
  else
    stride = dimt / nresids;

  // iterate across the brain, calling the regression code.  if g
  // matrix is to be recalculated for each step, init it.
  int betacount = keeperlist.size();
  if (!(flags & EXCLUDEERROR)) betacount++;
  // FIXME make sure the following really get set
  paramtes.init(dimx, dimy, dimz, betacount, vb_double);
  residtes.init(dimx, dimy, dimz, nresids, vb_double);
  // copy origin and voxel sizes to paramtes header
  paramtes.origin[0] = tesgroup[0].origin[0];
  paramtes.origin[1] = tesgroup[0].origin[1];
  paramtes.origin[2] = tesgroup[0].origin[2];
  paramtes.voxsize[0] = tesgroup[0].voxsize[0];
  paramtes.voxsize[1] = tesgroup[0].voxsize[1];
  paramtes.voxsize[2] = tesgroup[0].voxsize[2];

  int xx, yy, zz, countdown = interval;
  VB_Vector dependentvar;
  // if dependent var is in g matrix, grab it and set it once
  if (dependentindex > -1) {
    dependentvar = gMatrix.GetColumn(dependentindex);
    permute_if_needed(dependentvar);
  }
  for (i = firstvox; i <= lastvox; i++) {
    xx = xs[i];
    yy = ys[i];
    zz = zs[i];
    // load the time series
    VB_Vector signal;
    for (m = 0; m < (int)tesgroup.size(); m++) {
      tesgroup[m].ReadTimeSeries(tesgroup[m].GetFileName(), xx, yy, zz);
      if (flags & MEANSCALE) tesgroup[m].timeseries.meanNormalize();
      if (flags & DETREND) tesgroup[m].timeseries.removeDrift();
      signal.concatenate(tesgroup[m].timeseries);
    }
    // if the G matrix has the dependent var, put signal in there
    if (dependentindex > -1) {
      gMatrix.SetColumn(dependentindex, signal);
      f1Matrix.clear();
    } else {
      dependentvar = signal;
      permute_if_needed(dependentvar);
    }
    int err = Regress(dependentvar);
    if (err) return err;
    // bang the params into paramtes
    for (m = 0; m < (int)keeperlist.size(); m++)
      paramtes.SetValue(xx, yy, zz, m, betas[keeperlist[m]]);
    if (!(flags & EXCLUDEERROR))
      paramtes.SetValue(xx, yy, zz, m, betas[betas.getLength() - 1]);
    // bang the resids info residtes
    int tt = 0;
    for (int ind = 0; ind < nresids; ind++) {
      residtes.SetValue(xx, yy, zz, ind, residuals[tt]);
      tt += stride;
    }
    countdown--;
    if (countdown == 0) {
      printf("[I] vbregress: percent done: %d\n",
             ((i - firstvox) * 100) / (lastvox - firstvox + 1));
      fflush(stdout);
      countdown = interval;
    }
  }
  // set prm flags, perhaps including effdf
  // first copy header from first tes file
  if (tesgroup.size()) {
    for (int i = 0; i < (int)tesgroup[0].header.size(); i++) {
      paramtes.header.push_back("copied_header: " + tesgroup[0].header[i]);
    }
  }
  // mean norm flag
  if (flags & MEANSCALE)
    paramtes.AddHeader("DataScale:\tmean");
  else
    paramtes.AddHeader("DataScale:\tnone");
  residtes.AddHeader((string) "residual_stride: " + strnum(stride));
  // detrend flag
  string optionstring = "Option:";
  if (flags & DETREND) optionstring += " detrend";
  if (flags & EXCLUDEERROR) optionstring += " excludeerror";
  if (!(flags & (DETREND + EXCLUDEERROR))) optionstring += " none";
  paramtes.AddHeader(optionstring);
  // effective degrees of freedom
  if (traceRV.size() == 3)
    paramtes.AddHeader((string) "EffDegFree:\t" + strnum(traceRV[2]));

  if (nparts == 1) {
    paramtes.SetFileName(stemname + ".prm");
    residtes.SetFileName(stemname + ".res");
  } else {
    paramtes.SetFileName(stemname + ".prm_part_" + strnum(part));
    residtes.SetFileName(stemname + ".res_part_" + strnum(part));
  }
  if (paramtes.WriteFile()) return 101;
  if (residtes.WriteFile()) return 201;
  return 0;  // no error!
}

// the following is the new gold standard calculatebetas routine -- it
// scales the error term by tracerv if possible

int GLMInfo::calcbetas(VB_Vector &signal) {
  const size_t length = signal.getLength();
  VB_Vector realSignalFFT(length);
  VB_Vector imagSignalFFT(length);
  signal.fft(realSignalFFT, imagSignalFFT);
  VB_Vector realProd(length);
  VB_Vector imagProd(length);
  VB_Vector::compMult(realSignalFFT, imagSignalFFT, realExokernel,
                      imagExokernel, realProd, imagProd);
  VB_Vector kx(length);
  VB_Vector::complexIFFTReal(realProd, imagProd, kx);

  betas.resize(f1Matrix.m + 1);
  residuals.resize(rMatrix.m);
  betas *= 0.0;
  residuals *= 0.0;

  if (length != f1Matrix.n || length != rMatrix.n) return 101;

  // B=(F1)(KX) -- F1 is nvars x ntime, KX is ntime x 1
  for (uint32 i = 0; i < f1Matrix.m; i++) {
    for (uint32 j = 0; j < f1Matrix.n; j++) {
      betas[i] += f1Matrix(i, j) * kx[j];
    }
  }

  residuals.resize(signal.getLength());
  // resids= (R)(KX) -- R is ntime x ntime, KX is ntime x 1
  for (uint32 i = 0; i < rMatrix.m; i++) {
    for (uint32 j = 0; j < rMatrix.n; j++) {
      residuals[i] += rMatrix(i, j) * kx[j];
    }
  }
  betas[betas.getLength() - 1] =
      (residuals.euclideanProduct(residuals) / traceRV[0]);
  return 0;
}

// this version of calculatebetas is appropriate for
// non-autocorrelated data.

int GLMInfo::calcbetas_nocor(VB_Vector &signal) {
  const int length = signal.getLength();

  betas.resize(gMatrix.n + 1);
  residuals.resize(length);
  betas *= 0.0;
  residuals *= 0.0;

  if (f1Matrix.n != signal.getLength()) return 101;

  // B=F1*X -- F1 is nvars x ntime, X is ntime x 1
  for (uint32 i = 0; i < f1Matrix.m; i++) {
    betas[i] = 0;
    for (uint32 j = 0; j < f1Matrix.n; j++) {
      betas[i] += f1Matrix(i, j) * signal[j];
    }
  }
  // put the fitted values in the residuals
  for (uint32 i = 0; i < gMatrix.m; i++) {
    for (uint32 j = 0; j < gMatrix.n; j++) {
      residuals[i] += gMatrix(i, j) * betas[j];
    }
  }
  // now subtract fitted values from signal
  for (int i = 0; i < length; i++) residuals[i] = signal[i] - residuals[i];

  // FIXME -- is this the unscaled SSE? (contra the autocor version)  doesn't
  // look like it.
  double effdf = gMatrix.m - gMatrix.n;
  betas[betas.getLength() - 1] = residuals.euclideanProduct(residuals) / effdf;

  return 0;
}

// VecRegressX() is called by vbregress only when there are no tes
// files, and the dependent variable is supposed to be in the G
// matrix.  probably never happens right now.

int GLMInfo::VecRegressX(uint32 flags) {
  if (dependentindex < 0) return 101;

  int betacount = keeperlist.size();
  if (!(flags & EXCLUDEERROR)) betacount++;

  gMatrix.ReadFile(stemname + ".G");
  if (!gMatrix.m) return 102;

  if (dependentindex > (int)gMatrix.n - 1) return 103;
  VB_Vector mysignal = gMatrix.GetColumn(dependentindex);
  if (mysignal.size() == 0) return 104;
  // delete the dependent variable from the G matrix
  VBMatrix newG(gMatrix.m, gMatrix.n - 1);
  int gcol = 0;
  for (uint32 i = 0; i < newG.n; i++) {
    if (gcol == dependentindex) gcol++;
    VB_Vector v = gMatrix.GetColumn(gcol);
    newG.SetColumn(i, v);
    gcol++;
  }
  gMatrix = newG;
  // perform the regression
  VB_Vector paramvec(betacount);
  permute_if_needed(mysignal);
  int err = Regress(mysignal);
  if (err) return err;
  int m;
  for (m = 0; m < (int)keeperlist.size(); m++)
    paramvec[m] = betas[keeperlist[m]];
  if (!(flags & EXCLUDEERROR)) paramvec[m] = betas[betas.getLength() - 1];

  if (paramvec.WriteFile(stemname + "_results.vec")) return 150;
  return 0;  // no error!
}
