
// stats.cpp
// VoxBo library of various stats functions
// Copyright (c) 2008-2011 by The VoxBo Development Team

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
// code contributed by Kosh Banerjee and Thomas King

using namespace std;

#include "stats.h"
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_sf_gamma.h>
#include "vbio.h"
#include "vbutil.h"

tval calc_ttest(VB_Vector &vec, bitmask &bm) {
  size_t cnt = bm.count();
  if (cnt == 0 || cnt == vec.size()) return tval();
  VB_Vector v1(cnt), v2(vec.size() - cnt);
  int ind1 = 0, ind2 = 0;
  for (uint32 i = 0; i < vec.size(); i++) {
    if (bm[i])
      v1[ind1++] = vec[i];
    else
      v2[ind2++] = vec[i];
  }
  return calc_ttest(v1, v2);
}

tval calc_ttest(VB_Vector &v1, VB_Vector &v2) {
  int n1 = v1.size();
  int n2 = v2.size();
  if (n1 < 2 || n2 < 2) return tval(0, n1 + n2 - 2);
  double var1 = v1.getVariance();
  double var2 = v2.getVariance();
  double mean1 = v1.getVectorMean();
  double mean2 = v2.getVectorMean();
  double pooledsd =
      sqrt((((n1 - 1) * var1) + ((n2 - 1) * var2)) / (n1 + n2 - 2));
  double t = (mean1 - mean2) /
             (pooledsd * sqrt((1.0 / (double)n1) + (1.0 / (double)n2)));
  double df = n1 + n2 - 2;
  // double foo= sqrt((1.0/n1)+(1.0/n2));
  // cout << "tci=" << gsl_cdf_tdist_Pinv(0.975,df) << endl;
  // cout << pooledsd*foo*gsl_cdf_tdist_Qinv(0.025,df) << endl;
  tval ret;
  ret.t = t;
  ret.df = df;
  ret.diff = mean1 - mean2;
  ret.sd = pooledsd;
  ret.stderror = pooledsd * sqrt((1.0 / n1) + (1.0 / n2));
  return ret;
}

tval calc_welchs(VB_Vector &vec, bitmask &bm) {
  int cnt = bm.count();
  VB_Vector v1(cnt), v2(vec.size() - cnt);
  int ind1 = 0, ind2 = 0;
  for (uint32 i = 0; i < vec.size(); i++) {
    if (bm[i])
      v1[ind1++] = vec[i];
    else
      v2[ind2++] = vec[i];
  }
  return calc_welchs(v1, v2);
}

tval calc_welchs(VB_Vector &v1, VB_Vector &v2) {
  int n1 = v1.size();
  int n2 = v2.size();
  if (n1 < 2 || n2 < 2)  // shouldn't happen
    return tval(0, n1 + n2 - 2);
  double var1 = v1.getVariance();
  double var2 = v2.getVariance();
  double mean1 = v1.getVectorMean();
  double mean2 = v2.getVectorMean();
  double t = (mean1 - mean2) / sqrt((var1 / (double)n1) + (var2 / (double)n2));
  double df = pow((var1 / n1) + (var2 / n2), 2.0) /
              (pow(var1, 2) / ((double)(n1 * n1) * (n1 - 1)) +
               pow(var2, 2.0) / ((double)(n2 * n2) * (n2 - 1)));
  return tval(t, df);
}

x2val calc_chisquared(bitmask groups, bitmask deficit, bool yatesflag) {
  x2val res;
  if (groups.size() != deficit.size()) return res;
  double a = 0, b = 0, c = 0, d = 0;
  for (size_t i = 0; i < groups.size(); i++) {
    if (groups[i]) {
      if (deficit[i])
        a += 1.0;
      else
        b += 1.0;
    } else {
      if (deficit[i])
        c += 1.0;
      else
        d += 1.0;
    }
  }
  double x2 = ((a * d) - (b * c));
  if (yatesflag) {
    double N = a + b + c + d;
    x2 = fabs(x2) - (N / 2.0);
    x2 *= x2 * N;
    x2 /= (a + b) * (c + d) * (a + c) * (b + d);
  } else {
    x2 *= x2 * (groups.size());
    x2 /= (a + b) * (c + d) * (a + c) * (b + d);
  }
  res.x2 = x2;
  res.df = 1;
  res.p = gsl_cdf_chisq_Q(x2, 1) / 2.0;
  res.c00 = d;
  res.c01 = c;
  res.c10 = b;
  res.c11 = a;
  return res;
}

x2val calc_fisher(bitmask groups, bitmask deficit) {
  x2val res;
  res.p = -1;
  if (groups.size() != deficit.size()) return res;
  double a = 0, b = 0, c = 0, d = 0;
  for (size_t i = 0; i < groups.size(); i++) {
    if (groups[i]) {
      if (deficit[i])
        a += 1.0;
      else
        b += 1.0;
    } else {
      if (deficit[i])
        c += 1.0;
      else
        d += 1.0;
    }
  }
  res.x2 = 0;
  res.df = 1;
  res.c00 = d;
  res.c01 = c;
  res.c10 = b;
  res.c11 = a;

  uint32 k = res.c11;
  uint32 n1 = res.c11 + res.c01;
  uint32 n2 = res.c10 + res.c00;
  uint32 t = res.c11 + res.c10;
  cout << gsl_cdf_hypergeometric_Q(k, n1, n2, t) << endl;
  cout << gsl_cdf_hypergeometric_Q(k, n2, n1, t) << endl;
  cout << gsl_cdf_hypergeometric_P(k, n1, n2, t) << endl;
  cout << gsl_cdf_hypergeometric_P(k, n2, n1, t) << endl;
  double fisher = gsl_sf_fact(a + b) * gsl_sf_fact(c + d) * gsl_sf_fact(a + c) *
                  gsl_sf_fact(b + d);
  fisher /= gsl_sf_fact(a) * gsl_sf_fact(b) * gsl_sf_fact(c) * gsl_sf_fact(d) *
            gsl_sf_fact(groups.size());
  cout << fisher << endl;

  res.p = gsl_cdf_hypergeometric_Q(res.c11, res.c10 + res.c11,
                                   res.c00 + res.c01, res.c11);
  return res;
}

void t_to_p_z(tval &res, bool twotailed) {
  double zval, pval, origp;
  bool neg = (res.t < 0 ? 1 : 0);
  // the Q function gives the upper tail probability, P the lower
  if (twotailed) {
    if (neg)
      pval = gsl_cdf_tdist_P(res.t, res.df);
    else
      pval = gsl_cdf_tdist_Q(res.t, res.df);
    origp = pval;
    pval *= 2.0;
  } else
    origp = pval = gsl_cdf_tdist_Q(res.t, res.df);

  zval = gsl_cdf_ugaussian_Qinv(origp);
  res.p = pval;
  res.z = zval;
}

VBVoxel find_fdr_thresh(Tes &vol, double q) {
  vector<VBVoxel> plist;
  int i, j, k;
  double pvalue = 0.0;
  VBVoxel voxel;

  for (i = 0; i < vol.dimx; i++) {
    for (j = 0; j < vol.dimy; j++) {
      for (k = 0; k < vol.dimz; k++) {
        if (!vol.VoxelStored(i, j, k)) continue;
        pvalue = vol.GetValue(i, j, k, 0);
        if (pvalue < DBL_MIN) {
          // cout << "shouldn't happen " << i << " " << j << " " << k << endl;
          // cout << DBL_MIN << " " << pvalue << endl;
        }
        voxel.val = fabs(pvalue);  // why abs?  p<0 should never happen
        voxel.x = i;
        voxel.y = j;
        voxel.z = k;
        plist.push_back(voxel);
      }
    }
  }
  sort(plist.begin(), plist.end(), vcompare);
  int maxind = -1;
  // go through, testing P(i)<=(i/V)(q/cv)
  // cv=1, so our factor to multiply i by is just q/V
  double qv = q / plist.size();
  for (i = 0; i < (int)plist.size(); i++) {
    if (plist[i].val <= (double)(i + 1) * qv) maxind = i;
  }
  // printf("[DEBUG] number of voxels: %d\n",(int)plist.size());
  // printf("[DEBUG]    winning index: %d\n",maxind);
  // printf("[DEBUG]     lowest value: %.8f\n",plist[0].val);
  // printf("[DEBUG]    highest value: %.8f\n",plist[plist.size()-1].val);
  if (maxind >= 0) {
    plist[maxind].setCool();
    return plist[maxind];
  }
  voxel.x = 0;
  voxel.y = 0;
  voxel.z = 0;
  voxel.val = nan("nan");
  return voxel;
}

// FIXME the below is a skeleton

float calc_bm() {
  // combine all the values, rank order them
  // calculate r1/r2=mean rank from group 1/2
  // tbm=n1n1(r2-r1) / (n1+n2) * sqrt(n1s2+n2s2)
  // s1=sum(rank - value - r1 + (n1+1/2))^2   / n1-1
  return 0.0;
}
