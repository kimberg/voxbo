
// glm_stats.cpp
// all the calc_xxx and calc_xxx_cube functions
// Copyright (c) 2003-2010 by the VoxBo Development Team
//
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
// includes code written by Dan Kimberg, Dongbo Hu, and Tom King, all
// mixed in together

#include <gsl/gsl_cdf.h>
#include "glmutil.h"
#include "imageutils.h"
#include "vbio.h"

// legitimate scales (synonyms on lines together):
// t
// tp tp1 tp/1
// tp2, tp/2
// tz, tz1, tz/1
// tz2, tz/2
// f
// fp
// fz
// beta rawbeta b rb
// intercept int i pct percent
// error err
// hyp
// phase

int GLMInfo::calc_stat() {
  statval = nan("nan");
  string myscale = xstripwhitespace(vb_tolower(contrast.scale));
  if (myscale == "t") return calc_t();
  if (myscale == "f") return calc_f();
  if (myscale == "intercept" || myscale == "int" || myscale == "i" ||
      myscale == "pct" || myscale == "percent")
    return calc_pct();
  if (myscale == "error" || myscale == "err") return calc_error();
  if (myscale == "beta" || myscale == "rawbeta" || myscale == "rb" ||
      myscale == "b")
    return calc_beta();
  if (myscale == "hyp") return calc_hyp();
  if (myscale == "phase") return calc_phase();

  int err;
  // t-derived stats
  if (myscale[0] == 't') {
    if ((err = calc_t())) return err;
    if ((err = convert_t())) return err;
    return 0;
  }
  // F-derived stats
  if (myscale[0] == 'f') {
    if ((err = calc_f())) return err;
    if ((err = convert_f())) return err;
    return 0;
  }
  return 101;
}

int GLMInfo::convert_t() {
  rawval = statval;
  if (effdf < 0) {
    if (traceRV.getLength() == 3)
      effdf = traceRV[2];
    else if (traceRV.ReadFile(stemname + ".traces") == 0) {
      if (traceRV.getLength() == 3)
        effdf = traceRV[2];
      else
        return 204;
    }
    // non-autocorrelated, try trace(r)^2/trace(r^2), where r is
    // residual-forming matrix I-GG'
    else {
      VBMatrix g0 = gMatrix;
      VBMatrix g1(g0.n, g0.m);
      if (pinv(g0, g1)) return 221;
      g0 *= g1;
      VBMatrix r(g0.m, g0.m);
      r.ident();
      r -= g0;
      VBMatrix rr = r;
      rr *= r;
      effdf = r.trace();
      effdf *= effdf;
      effdf /= rr.trace();
    }
  }
  string myscale = vb_tolower(contrast.scale);
  int zflag = 0;
  int qflag = 0;
  int twotailed = 0;

  for (size_t i = 1; i < myscale.size(); i++) {
    if (myscale[i] == 'p') continue;  // assumed!
    if (myscale[i] == 'z')
      zflag = 1;
    else if (myscale[i] == 'q')
      qflag = 1;
    else if (myscale[i] == '1')
      twotailed = 0;
    else if (myscale[i] == '2')
      twotailed = 1;
    else
      return 211;
  }
  double pval, origp;
  bool neg = (rawval < 0 ? 1 : 0);
  // the Q function gives the upper tail probability, P the lower
  if (twotailed) {
    if (neg)
      pval = gsl_cdf_tdist_P(rawval, effdf);
    else
      pval = gsl_cdf_tdist_Q(rawval, effdf);
    origp = pval;
    pval *= 2.0;
  } else
    origp = pval = gsl_cdf_tdist_Q(rawval, effdf);

  if (zflag)
    statval = gsl_cdf_ugaussian_Qinv(origp);
  else if (qflag)
    statval = 1 - pval;
  else
    statval = pval;
  return 0;
}

int GLMInfo::convert_t_cube() {
  rawcube = statcube;
  int err;
  for (int i = 0; i < statcube.dimx; i++) {
    for (int j = 0; j < statcube.dimy; j++) {
      for (int k = 0; k < statcube.dimz; k++) {
        statval = statcube.GetValue(i, j, k);
        err = convert_t();
        if (err) return err;
        statcube.SetValue(i, j, k, statval);
      }
    }
  }
  return 0;
}

int GLMInfo::convert_f_cube() {
  rawcube = statcube;
  for (int i = 0; i < statcube.dimx; i++) {
    for (int j = 0; j < statcube.dimy; j++) {
      for (int k = 0; k < statcube.dimz; k++) {
        statval = statcube.GetValue(i, j, k);
        convert_f();
        statcube.SetValue(i, j, k, statval);
      }
    }
  }
  return 0;
}

int GLMInfo::convert_f() {
  // first calculate numerator df, which is just the number of vars in
  // contrast.  we previously used the total number of vars, which
  // overestimated df1 and therefore resulted in a too-low p-value
  rawval = statval;
  statval = 0;
  int numdf = 0;
  for (size_t i = 0; i < contrast.contrast.size(); i++)
    if (fabs(contrast.contrast[i]) > FLT_MIN) numdf++;
  if (effdf < 0) {
    if (traceRV.getLength() == 3)
      effdf = traceRV[2];
    else if (traceRV.ReadFile(stemname + ".traces") == 0) {
      if (traceRV.getLength() == 3)
        effdf = traceRV[2];
      else
        return 204;
    }
    // non-autocorrelated, try trace(r)^2/trace(r^2), where r is
    // residual-forming matrix I-GG'
    else {
      VBMatrix g0 = gMatrix;
      VBMatrix g1(g0.n, g0.m);
      if (pinv(g0, g1)) return 221;
      g0 *= g1;
      VBMatrix r(g0.m, g0.m);
      r.ident();
      r -= g0;
      VBMatrix rr = r;
      rr *= r;
      effdf = r.trace();
      effdf *= effdf;
      effdf /= rr.trace();
    }
  }
  string myscale = vb_tolower(contrast.scale);
  int zflag = 0;
  int qflag = 0;
  for (size_t i = 0; i < myscale.size(); i++) {
    if (myscale[i] == 'p') continue;  // assumed!
    if (myscale[i] == 'z')
      zflag = 1;
    else if (myscale[i] == 'q')
      qflag = 1;
    else
      return 211;
  }

  double pval = gsl_cdf_fdist_Q(rawval, numdf, effdf);
  if (qflag) {
    statval = 1.0 - pval;
    return 0;
  }
  if (zflag) {
    statval = gsl_cdf_ugaussian_Qinv(pval);
    return 0;
  }
  statval = pval;
  return 0;
}

int GLMInfo::calc_stat_cube() {
  // load parameter estimates if not already loaded
  if (paramtes.dimt < 1) paramtes.ReadFile(stemname + ".prm");
  if (paramtes.dimt < 1) return 201;

  string myscale = xstripwhitespace(vb_tolower(contrast.scale));
  if (myscale == "t") return calc_t_cube();
  if (myscale == "intercept" || myscale == "int" || myscale == "i" ||
      myscale == "percent" || myscale == "pct")
    return calc_pct_cube();
  if (myscale == "error" || myscale == "err") return calc_error_cube();
  if (myscale == "f") return calc_f_cube();
  if (myscale == "beta" || myscale == "rawbeta" || myscale == "rb" ||
      myscale == "b")
    return calc_beta_cube();
  if (myscale == "hyp") return calc_hyp_cube();
  if (myscale == "phase") return calc_phase_cube();
  if (myscale[0] == 't') {
    int err = calc_t_cube();
    if (err) return err;
    err = convert_t_cube();
    if (err) return err;
    return 0;
  }
  if (myscale[0] == 'f') {
    int err = calc_f_cube();
    if (err) return err;
    err = convert_f_cube();
    if (err) return err;
    return 0;
  }

  return 101;
}

int GLMInfo::calc_t() {
  statval = 0.0;
  if (contrast.contrast.size() != gMatrix.n) return 101;
  double fact = calcfact();
  double error = betas[betas.getLength() - 1];
  // printf("error: %.10f    beta: %.10f   fact %.10f\n",error,betas[0],fact);
  error = sqrt(error * fact);
  for (size_t i = 0; i < contrast.contrast.size(); i++)
    statval += betas[i] * contrast.contrast[i];
  statval /= error;
  return 0;
}

int GLMInfo::calc_f() {
  VB_Vector cc = contrast.contrast;
  if (betas.size() < 1) {
    statval = nan("nan");
    return 101;
  }

  // error,
  double error = betas[betas.size() - 1];
  // count included vars
  vector<int> includedlist;
  for (size_t i = 0; i < cc.size(); i++)
    if (fabs(cc[i]) > FLT_MIN) includedlist.push_back(i);
  int includedvars = includedlist.size();

  VBMatrix iso(includedvars, nvars);
  VBMatrix var_iso(1, 1);
  iso *= 0.0;
  var_iso *= 0.0;
  VBMatrix V;
  for (int i = 0; i < includedvars; i++)
    gsl_matrix_set(&(iso.mview.matrix), i, includedlist[i],
                   cc[includedlist[i]]);
  V.ReadFile(stemname + ".V");
  f1Matrix.ReadFile(stemname + ".F1");
  // FIXME make sure F1 and V are valid
  var_iso = iso;
  var_iso *= f1Matrix;
  var_iso *= V;
  f1Matrix.transposed = 1;
  var_iso *= f1Matrix;
  f1Matrix.transposed = 0;
  iso.transposed = 1;
  var_iso *= iso;

  // used below
  VBMatrix inverted_var;
  VBMatrix F_numerator(1, 1);
  inverted_var = var_iso;
  invert(var_iso, inverted_var);

  // create the isobetas, betas for the non-zero contrast elements
  VBMatrix isobetas(includedvars, 1);
  for (int m = 0; m < includedvars; m++)
    gsl_matrix_set(&(isobetas.mview.matrix), m, 0, betas[includedlist[m]]);
  // F_numerator=transpose(IsoBetas)##invert(Var_IsoBetas)## $
  //    IsoBetas/InterestCount
  F_numerator = isobetas;
  F_numerator.transposed = 1;
  F_numerator *= inverted_var;
  isobetas /= (double)includedvars;
  F_numerator *= isobetas;
  statval = F_numerator(0, 0) / error;

  return 0;
}

int GLMInfo::calc_error() {
  if (betas.size() < 1) {
    statval = nan("nan");
    return 101;
  }
  double error = betas[betas.getLength() - 1];
  statval = sqrt(error);
  return 0;
}

int GLMInfo::calc_pct() {
  if (interceptindex > (int)betas.size() - 1) {
    statval = nan("nan");
    return (101);
  }
  // calc beta (same as below)
  statval = 0.0;
  if (contrast.contrast.size() != gMatrix.n) return 101;
  for (size_t i = 0; i < contrast.contrast.size(); i++)
    statval += betas[i] * contrast.contrast[i];
  // scale by intercept
  statval /= betas[interceptindex];
  return 0;
}

int GLMInfo::calc_beta() {
  if (betas.size() < 1) {
    statval = nan("nan");
    return 101;
  }
  statval = 0.0;
  if (contrast.contrast.size() != gMatrix.n) return 101;
  for (size_t i = 0; i < contrast.contrast.size(); i++)
    statval += betas[i] * contrast.contrast[i];
  return 0;
}

int GLMInfo::calc_hyp() {
  if (betas.size() < 1) {
    statval = nan("nan");
    return 101;
  }
  statval = 0.0;
  if (contrast.contrast.size() != gMatrix.n) return 101;
  for (size_t i = 0; i < contrast.contrast.size(); i++)
    statval += betas[i] * betas[i] * contrast.contrast[i];
  statval = pow(statval, 1.0 / contrast.contrast.getVectorSum());
  return 0;
}

int GLMInfo::calc_phase() {
  if (betas.size() < 1) {
    statval = nan("nan");
    return 101;
  }
  statval = 0.0;
  if (contrast.contrast.size() != gMatrix.n) return 101;
  double b1 = nan("nan"), b2 = nan("nan");
  for (size_t i = 0; i < contrast.contrast.size(); i++) {
    if (contrast.contrast[i] > 0) b1 = betas[i];
    if (contrast.contrast[i] < 0) b2 = betas[i];
  }
  if (b2 == 0.0 || !isfinite(b1) || !isfinite(b2))
    statval = 0.0;
  else
    statval = atan2(b1, b2) * (180.0 / PI);
  return 0;
}

int GLMInfo::calc_error_cube() {
  // load variance volume, init stat cube
  paramtes.getCube(paramtes.dimt - 1, statcube);
  statcube.CopyHeader(paramtes);

  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        statcube.SetValue(i, j, k, sqrt(statcube.GetValue(i, j, k)));
      }
    }
  }
  return 0;
}

int GLMInfo::calc_pct_cube() {
  if (interceptindex < 0) return 101;
  int realinterceptindex = -1;
  for (size_t m = 0; m < keeperlist.size(); m++) {
    if (keeperlist[m] == interceptindex) realinterceptindex = m;
  }
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // convenience
  VB_Vector cc = contrast.contrast;

  double sval, weight;
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        sval = 0;
        for (size_t m = 0; m < keeperlist.size(); m++) {
          weight = cc[keeperlist[m]];
          if (fabs(weight) > FLT_MIN)
            sval += paramtes.GetValue(i, j, k, m) * weight;
        }
        statcube.SetValue(
            i, j, k, sval / paramtes.GetValue(i, j, k, realinterceptindex));
      }
    }
  }
  return 0;
}

int GLMInfo::calc_t_cube() {
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // load variance volume
  Cube errorCube;
  paramtes.getCube(paramtes.dimt - 1, errorCube);
  // for convenience
  VB_Vector cc = contrast.contrast;

  // error=sqrt(error*fact)
  double fact = calcfact();
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        errorCube.SetValue(i, j, k, sqrt(errorCube.GetValue(i, j, k) * fact));
      }
    }
  }
  if (pseudoT.size() == 3 && pseudoT.getMinElement() > FLT_MIN) {
    Cube smask;
    paramtes.ExtractMask(smask);
    smoothCube(errorCube, pseudoT[0], pseudoT[1], pseudoT[2]);
    smoothCube(smask, pseudoT[0], pseudoT[1], pseudoT[2]);
    errorCube /= smask;
    errorCube.intersect(smask);
  }

  double t, weight;
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        t = 0;
        for (size_t m = 0; m < keeperlist.size(); m++) {
          weight = cc[keeperlist[m]];
          if (fabs(weight) > FLT_MIN)
            t += paramtes.GetValue(i, j, k, m) * weight;
        }
        statcube.SetValue(i, j, k, t / errorCube.GetValue(i, j, k));
      }
    }
  }
  return 0;
}

int GLMInfo::calc_beta_cube() {
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // convenience
  VB_Vector cc = contrast.contrast;
  double t, weight;
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        t = 0;
        for (size_t m = 0; m < keeperlist.size(); m++) {
          weight = cc[keeperlist[m]];
          if (fabs(weight) > FLT_MIN)
            t += paramtes.GetValue(i, j, k, m) * weight;
        }
        statcube.SetValue(i, j, k, t);
      }
    }
  }
  return 0;
}

int GLMInfo::calc_hyp_cube() {
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // convenience
  VB_Vector cc = contrast.contrast;
  double t, weight;
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        t = 0.0;
        for (size_t m = 0; m < keeperlist.size(); m++) {
          weight = cc[keeperlist[m]];
          if (fabs(weight) > FLT_MIN) {
            t += pow(paramtes.GetValue(i, j, k, m) * weight, 2.0) * weight;
          }
        }
        t = pow(t, 1.0 / cc.getVectorSum());
        statcube.SetValue(i, j, k, t);
      }
    }
  }
  return 0;
}

int GLMInfo::calc_phase_cube() {
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // convenience
  VB_Vector cc = contrast.contrast;
  double t, weight, b1, b2;
  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        t = 0.0;
        b1 = nan("nan");
        b2 = nan("nan");
        for (size_t m = 0; m < keeperlist.size(); m++) {
          weight = cc[keeperlist[m]];
          if (weight > FLT_MIN) b1 = paramtes.GetValue(i, j, k, m);
          if (weight < FLT_MIN) b2 = paramtes.GetValue(i, j, k, m);
        }
        if (b2 == 0.0 || !isfinite(b1) || !isfinite(b2))
          statcube.SetValue(i, j, k, 0.0);
        else
          statcube.SetValue(i, j, k, atan2(b1, b2) * (180.0 / PI));
      }
    }
  }
  return 0;
}

int GLMInfo::calc_f_cube() {
  // init stat cube volume
  statcube.SetVolume(paramtes.dimx, paramtes.dimy, paramtes.dimz, vb_double);
  statcube.CopyHeader(paramtes);
  // load variance volume
  Cube errorCube;
  paramtes.getCube(paramtes.dimt - 1, errorCube);
  // for convenience
  VB_Vector cc = contrast.contrast;
  // count included vars
  vector<int> includedlist;
  for (size_t i = 0; i < cc.size(); i++)
    if (fabs(cc[i]) > FLT_MIN) includedlist.push_back(i);
  int includedvars = includedlist.size();

  VBMatrix iso(includedvars, nvars);
  VBMatrix var_iso(1, 1);
  iso *= 0.0;
  var_iso *= 0.0;
  VBMatrix V;
  for (int i = 0; i < includedvars; i++)
    gsl_matrix_set(&(iso.mview.matrix), i, includedlist[i],
                   cc[includedlist[i]]);
  V.ReadFile(stemname + ".V");
  f1Matrix.ReadFile(stemname + ".F1");
  // FIXME make sure F1 and V are valid
  var_iso = iso;
  var_iso *= f1Matrix;
  var_iso *= V;
  f1Matrix.transposed = 1;
  var_iso *= f1Matrix;
  f1Matrix.transposed = 0;
  iso.transposed = 1;
  var_iso *= iso;

  // used below
  VBMatrix inverted_var;
  VBMatrix F_numerator(1, 1);
  inverted_var = var_iso;
  invert(var_iso, inverted_var);

  // NB--
  // iso is keepers x nvars, betas is nvars x 1, isobetas is
  // keepers x 1 (weighted keepers); var_iso is keepers x
  // keepers, therefore f_numerator is 1x1!

  for (int i = 0; i < paramtes.dimx; i++) {
    for (int j = 0; j < paramtes.dimy; j++) {
      for (int k = 0; k < paramtes.dimz; k++) {
        if (paramtes.GetMaskValue(i, j, k) == 0)  // skip empty voxels
          continue;
        // create the isobetas, betas for the non-zero contrast elements
        paramtes.GetTimeSeries(i, j, k);
        VBMatrix isobetas(includedvars, 1);
        int ii = 0;
        for (size_t m = 0; m < keeperlist.size(); m++) {
          if (fabs(cc[keeperlist[m]]) > FLT_MIN) {
            if (ii > includedvars - 1) return 102;
            gsl_matrix_set(&(isobetas.mview.matrix), ii, 0,
                           paramtes.timeseries[m] * cc[keeperlist[m]]);
            ii++;
          }
        }
        // F_numerator=transpose(IsoBetas)##invert(Var_IsoBetas)## $
        //    IsoBetas/InterestCount
        F_numerator = isobetas;
        F_numerator.transposed = 1;
        F_numerator *= inverted_var;
        isobetas /= (double)includedvars;
        F_numerator *= isobetas;
        statcube.SetValue(i, j, k,
                          F_numerator(0, 0) / errorCube.GetValue(i, j, k));
      }
    }
  }
  return 0;
}

// t=cB/error, where error=SSE*fact/tracrv
// fact=cF1F3c

// when there's autocorrelation, we have the F1 and F3 matrices.  when
// there isn't, F1F3 reduces to invert(GtG)

double GLMInfo::calcfact() {
  double fact = 1.0;  // default
  if (f1Matrix.m == 0) f1Matrix.ReadFile(stemname + ".F1");
  if (f3Matrix.m == 0) f3Matrix.ReadFile(stemname + ".F3");
  if (f1Matrix.m && f3Matrix.m) {  // some autocorrelation
    VBMatrix c(contrast.contrast);
    VBMatrix ct(contrast.contrast);
    ct.transposed = 1;
    ct *= f1Matrix;
    ct *= f3Matrix;
    ct *= c;
    fact = ct(0, 0);
  } else {
    VBMatrix foo(gMatrix);
    foo.transposed = 1;
    foo *= gMatrix;
    VBMatrix bar(foo.m, foo.m);
    invert(foo, bar);
    VBMatrix c(contrast.contrast);
    VBMatrix ct(contrast.contrast);
    ct.transposed = 1;
    ct *= bar;
    ct *= c;
    fact = ct(0, 0);
  }
  return fact;
}
