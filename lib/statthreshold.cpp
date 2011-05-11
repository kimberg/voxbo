
// statthreshold.cpp
// library code to produce thresholds
// Copyright (c) 1998-2010 by The VoxBo Development Team, except as noted below

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
// ported by Tom King and Dan Kimberg

// This code was written to implement the algorithm previously
// implemented by Keith Worsley in his MATLAB code.  The copyright
// notice for that code follows.

/******************************************************************************
 * COPYRIGHT:   Copyright 2003 K.J. Worsley                                    *
 *              Department of Mathematics and Statistics,                      *
 *              McConnell Brain Imaging Center,                                *
 *              Montreal Neurological Institute,                               *
 *              McGill University, Montreal, Quebec, Canada.                   *
 *              keith.worsley@mcgill.ca , www.math.mcgill.ca/keith             *
 *                                                                             *
 *              Permission to use, copy, modify, and distribute this           *
 *              software and its documentation for any purpose and without     *
 *              fee is hereby granted, provided that this copyright            *
 *              notice appears in all copies. The author and McGill University *
 *              make no representations about the suitability of this          *
 *              software for any purpose.  It is provided "as is" without      *
 *              express or implied warranty.                                   *
 ******************************************************************************/

#include "statthreshold.h"
#include <limits>

const double DMIN=numeric_limits<double>::min();
const double DMAX=numeric_limits<double>::max();

using namespace std;

#define pi 3.1415926535897932384626433832795028841

gsl_vector*
nchoosekln(gsl_vector *n, gsl_vector *k)
{
  gsl_vector *ret = gsl_vector_calloc((int)k->size);
  if (!ret) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for(int z = 0; z < (int)n->size; z++) {
    gsl_vector_set(ret, z, DMIN);
  }
  for (int index = 0; index < (int)k->size; index++) {
    if (gsl_vector_get(n, index) >= 0 && gsl_vector_get(k, index) >= 0 && (gsl_vector_get(n, index) >= gsl_vector_get(k, index))) {
      gsl_vector_set(ret, index, -log(gsl_vector_get(n, index)+1) -gsl_sf_lnbeta(gsl_vector_get(k, index)+1,gsl_vector_get(n,index)-gsl_vector_get(k,index)+1));
    }
  }
  return ret;
}

void
interp1(string, vector<double>mx, vector<double>my, double &ixstl, double &thresholds)
{
  vector<double>ret;
  double val = 0.0;
  if (ixstl > mx[mx.size()-1] || ixstl < mx[0]) {
    val = DMAX;
    thresholds=val;
    val = 0;
  }
  else {
    for (int p = 0; p < (int)mx.size(); p++) {
      if (mx[p] >= ixstl) {
        if (mx.size() > 1) 
          val = my[p-1]+((my[p]-my[p-1])/(mx[p]-mx[p-1]))*((ixstl-mx[p-1]));
        else
          val = my[p];
        thresholds=val;
        val = 0;
        break;
      }          
    }
  }
}

void
interp1(string type, gsl_vector *x, gsl_vector *y, double &ix,double &thresholds)
{
  double val = 0.0;
  double diff = 0.0;
  double mindiff = DMAX; 
  int index = 0;
  mindiff = DMAX;
  index = -1;
  val = 0;
  for (int p = 1; p < (int)x->size; p++) {
    diff = gsl_vector_get(x, p) - ix;
    if (fabs(diff) < fabs(mindiff)) {
      mindiff = diff;
      index = p;
    }
  }
  if (index == -1) {
    thresholds=DMAX;
    return;
  } 
  if (mindiff < 0)
    val = gsl_vector_get(y, index-1)+(gsl_vector_get(y,index)-gsl_vector_get(y,index-1))/(gsl_vector_get(x,index)-gsl_vector_get(x,index-1))*(ix-gsl_vector_get(x,index-1));
  else
    val = gsl_vector_get(y, index+1)+(gsl_vector_get(y,index)-gsl_vector_get(y,index+1))/(gsl_vector_get(x,index)-gsl_vector_get(x,index+1))*(ix-gsl_vector_get(x,index+1)); 
  thresholds=val;

  if (type.size())
    cout << setprecision(20) << type << " " << "threshold" << " " << thresholds << endl;
  return;
}

void
minterp1(string type, gsl_vector *x, gsl_vector *y, double &ix, double &iy)
{
  vector<double>mxstl;
  vector<double>mystl;
  double ixstl;
  double thresholds;
  int n = (int)x->size;
  double xx = gsl_vector_get(x, 0);
  mxstl.push_back(gsl_vector_get(x, 0));
  mystl.push_back(gsl_vector_get(y, 0));
  for (int i = 1; i < n; i++) {
    if (gsl_vector_get(x, i) > xx) {
      xx = gsl_vector_get(x, i);
      mxstl.push_back(xx);
      mystl.push_back(gsl_vector_get(y, i));
    }
  }
  ixstl=ix;
  interp1(type, mxstl, mystl, ixstl, thresholds);
  if (type.size())
    cout << type << setprecision(20) << " threshold" << " " << thresholds << endl;
  iy=thresholds;
  return;
}

int
stat_threshold(threshold &v)
{
  // in case we don't get real values...
  v.bonpeakthreshold=nan("nan");
  v.peakthreshold=nan("nan");
  double fwhm = v.fwhm;
  double pValPeak=v.pValPeak;
  double clusterThreshold = v.clusterThreshold;
  double pValExtent=v.pValExtent;
  if (v.searchVolume == 0) v.searchVolume = 1000000;
  if (v.numVoxels == 0) v.numVoxels = 1000000;
  if (pValPeak==0) pValPeak=.05;
  if (clusterThreshold == 0) clusterThreshold = .001;
  if (pValExtent==0) pValExtent=.05;
  int lsv = sizeof(v.searchVolume)/sizeof(long);
  int numDimensions = 3;

  int dfLimit = 4;
  double df1=DMAX, df2=DMAX, dfw1=DMAX, dfw2=DMAX;
  int is_tstat = 0;
  if (v.denomdf==0) is_tstat=1;

  if (v.effdf<FLT_MIN) return 101;
  if (is_tstat) {
    if (v.effdf<3) return 102;
    df1 = 1;
    df2 = v.effdf;
  }
  else {
    if (v.denomdf<3) return 103;
    df1 = v.effdf;
    df2 = v.denomdf;
  }
  if (df2 > 1000) df2 = DMAX;
  dfw1 = DMAX;
  dfw2 = DMAX;
  double pvalextent=pValExtent;
  gsl_vector *t = gsl_vector_calloc(1901);
  if (!t) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  int tcount = 0;
  for (double val = 100; val > 10.1; val-=.1, tcount++) 
    gsl_vector_set(t, tcount, val*val);
  for (double val = 10; val >= 0; val-=.01, tcount++) 
    gsl_vector_set(t, tcount, val*val); 
  int n = 1901;
  //gsl_vector *n1 = gsl_vector_calloc(n-1);
  gsl_vector *tt = gsl_vector_calloc(n-1);
  if (!tt) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (tcount=0; tcount < (n-1); tcount++) {
    if (tcount != n-2) {
      gsl_vector_set(tt, tcount, ((gsl_vector_get(t, tcount)+gsl_vector_get(t, tcount+1))/2.0));
    }
    else 
      gsl_vector_set(tt, tcount, (gsl_vector_get(t, tcount)/2.0));
  }
  gsl_vector *b = gsl_vector_calloc(n-1);
  if (!b) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  if (df2 == DMAX) {
    gsl_vector_memcpy(b, tt);
    gsl_vector_scale(b, df1);
    gsl_vector_scale(b, -.5);
    gsl_vector_add_constant(b, -gsl_sf_lngamma(df1/2.0)-(df1/2.0*log(2.0)));
    gsl_vector *temp = gsl_vector_calloc(n-1); 
    if (!temp) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector_memcpy(temp, tt);
    gsl_vector_scale(temp, df1);
    for (int tempcount=0; tempcount < (n-1); tempcount++) 
      gsl_vector_set(temp, tempcount, log((double)gsl_vector_get(temp, tempcount)));
    gsl_vector_scale(temp, (df1/2.0-1));
    gsl_vector *final = gsl_vector_calloc(n-1);
    if (!final) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int tempcount=0; tempcount < (n-1); tempcount++)
      gsl_vector_set(final, tempcount, gsl_vector_get(temp, tempcount) + gsl_vector_get(b, tempcount));
    for (int bcount=0; bcount < (n-1); bcount++) 
      gsl_vector_set(b, bcount, exp(gsl_vector_get(final, bcount)));
    if (temp) gsl_vector_free(temp);
  }
  else {
    gsl_vector *u = gsl_vector_calloc(n-1);
    if (!u) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector_memcpy(u, tt);
    for (int xx=0; xx < (int)b->size; xx++) {
      gsl_vector_set(u, xx, df1*gsl_vector_get(tt, xx)/df2);
      gsl_vector_set(b, xx, -(df1+df2)/2.0*log(1+gsl_vector_get(u, xx))+(df1/2.0-1)*log(gsl_vector_get(u, xx)));
      gsl_vector_set(b, xx, exp(gsl_vector_get(b, xx)-gsl_sf_lnbeta(df1/2.0,df2/2.0))*df1/df2);
    }
  }
  int boolFwhmTrue = 0;
  if (fwhm>FLT_MIN) boolFwhmTrue = 1;
  int D = numDimensions*boolFwhmTrue;
  gsl_matrix *tau = gsl_matrix_calloc(D+1, n);
  if (!tau) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_vector *difft = gsl_vector_calloc(n-1);
  if (!difft) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int taucount=0; taucount < (n-1); taucount++) { 
    if (taucount + 1 < n)  
      gsl_vector_set(difft, taucount, gsl_vector_get(t, taucount+1) - gsl_vector_get(t, taucount)); 
    else
      gsl_vector_set(difft, taucount, 0);
  }
  gsl_vector *bdifft = gsl_vector_calloc(n-1);
  if (!bdifft) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int taucount=0; taucount < (n-1); taucount++) { 
    gsl_vector_set(bdifft, taucount, gsl_vector_get(b, taucount)*gsl_vector_get(difft,taucount));
  }
  gsl_vector *csbdifft = gsl_vector_calloc(n-1);
  if (!csbdifft) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int taucount=0; taucount < (n-1); taucount++) {
    if (taucount == 0)
      gsl_vector_set(csbdifft, 0, gsl_vector_get(bdifft,taucount));
    else
      gsl_vector_set(csbdifft, taucount, gsl_vector_get(csbdifft,taucount-1)+gsl_vector_get(bdifft,taucount)); 
  }
  gsl_matrix_set(tau, 0, 0, 0.0-gsl_vector_get(csbdifft,0));
  for (int taucount=1; taucount < (n); taucount++) {
    //cout << taucount << endl;
    gsl_matrix_set(tau,0,taucount,0.0-gsl_vector_get(csbdifft,taucount-1));
  }
  if (difft) gsl_vector_free(difft);
  if (bdifft) gsl_vector_free(bdifft);
  if (csbdifft) gsl_vector_free(csbdifft);

  //finding EC densities
  gsl_vector *y = gsl_vector_calloc(t->size);      
  if (!y) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_vector_memcpy(y, t);   
  for (int i = 0; i < (int)y->size; i++) {
    gsl_vector_set(y, i, gsl_vector_get(y, i)*df1);
  }
  double min = 0.0;
  gsl_vector *s1=gsl_vector_calloc((int)y->size);
  if (!s1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  double cons = 0.0;
  gsl_vector*jj=0;
  for (int N = 1;  N <= (D); N++) {
    gsl_vector_set_zero(s1);
    for (int i = 0; i <= N-1; i++) {
      if ((N - 1 - i) > i)
        min = i;
      else
        min = N - 1 - i;
      if (jj) gsl_vector_free(jj); 
      jj = gsl_vector_calloc(int(min)+1);
      if (!jj) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      for (int value = 0; value <= min; value++)  
        gsl_vector_set(jj,value,value);
      gsl_vector *s2 = gsl_vector_calloc(int(min)+1);
      if (!s2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_vector *arg1 = gsl_vector_calloc(int(min)+1);
      if (!arg1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_vector *arg2 = gsl_vector_calloc(int(min)+1);
      if (!arg2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      double temp = 0.0;
      if (df2 == DMAX) {
        if (min) {
          temp = df1 - 1.0;
          gsl_vector_memcpy(arg1, jj);
          gsl_vector_memcpy(arg2, jj);
          gsl_vector_add_constant(arg1, temp);
          gsl_vector_sub(arg1, jj);
          temp = N - 1 - i;
          gsl_vector_scale(arg2, -1);
          gsl_vector_add_constant(arg2, temp);
          for (int m = 0; m < (int)s2->size; m++) {
            gsl_vector_set(s2, m,  gsl_vector_get(nchoosekln(arg1, arg2), m));
          }
          gsl_vector_set_zero(arg1);
          gsl_vector_set_zero(arg2);
          gsl_vector_memcpy(arg1, jj);
          for (int x = 0; x < (int)arg1->size; x++) {
            gsl_vector_set(arg1, x, gsl_vector_get(arg1, x) + 1);
            gsl_vector_set(arg1, x, gsl_sf_lngamma(gsl_vector_get(arg1, x))); 
            gsl_vector_set(s2, x, gsl_vector_get(s2, x) - gsl_vector_get(arg1,x));
          }
          gsl_vector_set_zero(arg1);
          gsl_vector_memcpy(arg1, jj);
          for (int x = 0; x < (int)arg1->size; x++) {
            gsl_vector_set(arg1, x, gsl_sf_lngamma(i - (gsl_vector_get(arg1, x)) + 1));
            gsl_vector_set(s2, x, gsl_vector_get(s2, x) - gsl_vector_get(arg1,x));
          }
          gsl_vector_set_zero(arg1);
          gsl_vector_memcpy(arg1, jj);
          for (int x = 0; x < (int)arg1->size; x++) {
            gsl_vector_set(arg1, x, gsl_vector_get(arg1, x)*log(2.0)); 
            gsl_vector_set(s2, x, gsl_vector_get(s2, x) - gsl_vector_get(arg1,x));
          }
          double sums2 = 0.0;           
          for (int val = 0; val < (int)s2->size; val++) 
            sums2 += exp(gsl_vector_get(s2,val));
          gsl_vector_set(s2, 0, sums2);
          sums2 = 0;
        }
        else {
          gsl_vector *n = gsl_vector_calloc(1);
          if (!n) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k = gsl_vector_calloc(1);
          if (!k) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector_set(n, 0, df1 - 1.0);
          gsl_vector_set(k, 0, N - 1 - i);
          s2 = nchoosekln(n, k);
          for (int m = 0; m < (int)s2->size; m++) {
            gsl_vector_set(s2,m,exp(gsl_vector_get(s2, m)-gsl_sf_lngamma(1)-gsl_sf_lngamma(i+1)));
          }
        }
      }
      else {
        if (min) {
          gsl_vector *n1 = gsl_vector_calloc(int(min)+1);
          if (!n1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k1 = gsl_vector_calloc(int(min)+1);
          if (!k1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *n2 = gsl_vector_calloc(int(min)+1);
          if (!n2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k2 = gsl_vector_calloc(int(min)+1);
          if (!k2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *n3 = gsl_vector_calloc(int(min)+1);
          if (!n3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k3 = gsl_vector_calloc(int(min)+1);
          if (!k3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r1 = gsl_vector_calloc(int(min)+1);
          if (!r1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r2 = gsl_vector_calloc(int(min)+1);
          if (!r2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r3 = gsl_vector_calloc(int(min)+1);      
          if (!r3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *sumrs = gsl_vector_calloc(int(min)+1);
          if (!sumrs) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector_memcpy(n1, jj);
          gsl_vector_memcpy(k1, jj);
          gsl_vector_memcpy(n2, jj);
          gsl_vector_memcpy(k2, jj);
          gsl_vector_memcpy(n3, jj);
          gsl_vector_memcpy(k3, jj);           
          for (int n1c=0; n1c < (int)n1->size; n1c++)
            gsl_vector_set(n1, n1c, df1-1+gsl_vector_get(jj, n1c)-gsl_vector_get(jj, n1c));
          for (int k1c=0; k1c < (int)k1->size; k1c++)
            gsl_vector_set(k1, k1c, N-1-i-gsl_vector_get(jj, k1c));
          for (int n2c=0; n2c < (int)n2->size; n2c++)
            gsl_vector_set(n2, n2c, (df1+df2-N)/2.0+gsl_vector_get(jj, n2c)-1);
          for (int n3c=0; n3c < (int)n3->size; n3c++)
            gsl_vector_set(n3, n3c, df2-1+gsl_vector_get(jj, n3c)-gsl_vector_get(jj, n3c));
          for (int k3c=0; k3c < (int)k3->size; k3c++)
            gsl_vector_set(k3, k3c, i - gsl_vector_get(jj, k3c));     
          double sums2 = 0.0;      
          for (int r=0; r < (int)r1->size; r++) {
            gsl_vector_set(r1, r, gsl_vector_get(nchoosekln(n1, k1), r));
            gsl_vector_set(r2, r, gsl_vector_get(nchoosekln(n2, k2), r));
            gsl_vector_set(r3, r, gsl_vector_get(nchoosekln(n3, k3), r));
            gsl_vector_set(sumrs, r, exp(gsl_vector_get(r1, r) + gsl_vector_get(r2, r) + gsl_vector_get(r3, r) -i*log(df2)));
            sums2 += gsl_vector_get(sumrs, r);
            gsl_vector_set(s2, 0, sums2);
          }
          sums2 = 0;
          gsl_vector_set_zero(sumrs);
          gsl_vector_free(n1);
          gsl_vector_free(n2);
          gsl_vector_free(n3);
          gsl_vector_free(k1);
          gsl_vector_free(k2);
          gsl_vector_free(k3);
          gsl_vector_free(r1);
          gsl_vector_free(r2);
          gsl_vector_free(r3);
          gsl_vector_free(sumrs);
        }
        else {
          gsl_vector *n1 = gsl_vector_calloc(int(min)+1);
          if (!n1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k1 = gsl_vector_calloc(int(min)+1);
          if (!k1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *n2 = gsl_vector_calloc(int(min)+1);
          if (!n2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k2 = gsl_vector_calloc(int(min)+1);
          if (!k2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *n3 = gsl_vector_calloc(int(min)+1);
          if (!n3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *k3 = gsl_vector_calloc(int(min)+1);
          if (!k3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r1 = gsl_vector_calloc(int(min)+1);
          if (!r1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r2 = gsl_vector_calloc(int(min)+1);
          if (!r2) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector *r3 = gsl_vector_calloc(int(min)+1);
          if (!r3) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
          gsl_vector_set(n1, 0, df1 - 1.0 + gsl_vector_get(jj, 0) - gsl_vector_get(jj, 0));
          gsl_vector_set(k1, 0, N - 1 - i - gsl_vector_get(jj, 0));
          gsl_vector_set(n2, 0, (df1 + df2 - N)/2.0 + gsl_vector_get(jj, 0) -1);
          gsl_vector_set(k2, 0, gsl_vector_get(jj, 0));      
          gsl_vector_set(n3, 0, df2-1+gsl_vector_get(jj, 0)-gsl_vector_get(jj, 0));
          gsl_vector_set(k3, 0, i - gsl_vector_get(jj, 0));     
          gsl_vector_set(r1, 0, gsl_vector_get(nchoosekln(n1, k1), 0));
          gsl_vector_set(r2, 0, gsl_vector_get(nchoosekln(n2, k2), 0));
          gsl_vector_set(r3, 0, gsl_vector_get(nchoosekln(n3, k3), 0));
          gsl_vector_set(s2, 0,exp(gsl_vector_get(r1, 0)+gsl_vector_get(r2, 0)+gsl_vector_get(r3, 0)-i*log(df2)));
          gsl_vector_free(n1);
          gsl_vector_free(n2);
          gsl_vector_free(n3);
          gsl_vector_free(k1);
          gsl_vector_free(k2);
          gsl_vector_free(k3);
          gsl_vector_free(r1);
          gsl_vector_free(r2);
          gsl_vector_free(r3); 
        }
      }
      int isPositive = 0;
      if (gsl_vector_get(s2, 0) > 0)
        isPositive = 1;

      if (isPositive) {
        isPositive = 0;
        double temp = pow(-1.0, N-1-i);
        double temp1= i+(df1-N)/2.0; 
        gsl_vector *s1copy = gsl_vector_calloc((int)s1->size);
        if (!s1copy) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl; 
        gsl_vector_memcpy(s1copy, s1);
        for (int x = 0; x < (int)s1->size; x++) {
          gsl_vector_set(s1, x, temp*(pow(gsl_vector_get(y, x), temp1)));
          gsl_vector_set(s1, x, gsl_vector_get(s1, x) * gsl_vector_get(s2, 0)); 
          gsl_vector_set(s1, x, gsl_vector_get(s1, x) + gsl_vector_get(s1copy, x));
        }
        gsl_vector_set_zero(s1copy);
        gsl_vector_free(s1copy);        
      }
    }
    if (df2 == DMAX) {
      cons = -gsl_sf_lngamma(df1/2.0) - N/2.0*log(2*pi) - (df1 - 2.0)/2.0*log(2.0) + gsl_sf_lngamma(N);

      for (int tc = 0; tc < n; tc++) {
        gsl_matrix_set(tau,N,tc,exp(cons-(gsl_vector_get(y, tc)/2.0))*gsl_vector_get(s1, tc));
      }
    }
    else {
      cons = -gsl_sf_lngamma(df1/2.0) - N/2.0*log(2*pi) - (df1 - 2.0)/2.0*log(2.0) + gsl_sf_lngamma(N) + gsl_sf_lngamma((df1 + df2 - N)/2.0) - gsl_sf_lngamma(df2/2.0) - (df1-N)/2.0*log(df2/2.0);
      for (int tc = 0; tc < n; tc++)
        gsl_matrix_set(tau,N,tc,exp(cons-(df1+df2-2)/2.0*log(1+gsl_vector_get(y, tc)/df2))*gsl_vector_get(s1, tc));
    }
  }
  //adding sphere to search region
  if (jj) gsl_vector_free(jj);
  double j=0;
  double involSphere;
  involSphere=exp(j*log(2.0)+j/2.0*log(pi)+ gsl_sf_lngamma(1.0)- gsl_sf_lngamma((2-j)/2.0)- gsl_sf_lngamma(j + 1));
  
  //create toeplitz matrix
  double insertionValue = 0;
  gsl_matrix *rho = gsl_matrix_calloc(n, D + 1);
  if (!rho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_matrix *toeplitz = gsl_matrix_calloc(D + 1, 1+ D);
  if (!toeplitz) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int rindex = 0; rindex < (D + 1); rindex++) {
    for (int cindex = 0; cindex < (1 + D); cindex++) {
      if (cindex < (int)involSphere) {
        insertionValue = involSphere;
        gsl_matrix_set(toeplitz, rindex, (cindex+rindex)%(1 + D), insertionValue);
      }
    }
  }
  gsl_blas_dgemm(CblasTrans, CblasTrans, 1.0, tau, toeplitz, 0.0, rho);
  gsl_matrix_free(toeplitz);
  gsl_matrix_free(tau);
  gsl_matrix *nrho=0;
  if (is_tstat) {
    //t=[sqrt(t(n1)) -fliplr(sqrt(t))];
    gsl_vector *newt = gsl_vector_calloc((2*((int)t->size))-1);
    if (!newt) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int index = 0; index < ((int)t->size-1); index++) {
      double value = gsl_vector_get(t, t->size - 1);
      gsl_vector_set(newt, index, sqrt(gsl_vector_get(t, index))-value);
    }
    gsl_vector_set(newt, ((int)t->size-1), 0);
    for (int index = ((int)t->size); index < 2*((int)t->size)-1; index++) {
      gsl_vector_set(newt, index, -1 * gsl_vector_get(newt, (((int)t->size) -1) - (index - (((int)t->size)-1)))); 
    }
    gsl_vector_free(t);
    t = gsl_vector_calloc((int)newt->size);
    if (!t) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector_memcpy(t, newt);
    gsl_vector_free(newt);
    //rho=[rho(:,n1) diag(-(-1).^(0:D))*fliplr(rho)]/2
    gsl_matrix * diagalmostident = gsl_matrix_calloc(D+1, D+1);
    if (!diagalmostident) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int cindex = 0; cindex < D+1; cindex++)
      for (int rindex = 0; rindex < D+1; rindex++) {
        if (cindex == rindex) {
          if ((cindex % 2) == 1)
            insertionValue = (1);
          else
            insertionValue = -1;
        }
        gsl_matrix_set(diagalmostident, cindex, rindex, insertionValue);
        insertionValue = 0;
      }
    gsl_matrix *fliplrRho = gsl_matrix_calloc(rho->size1, rho->size2);
    if (!fliplrRho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl; 
    gsl_matrix_memcpy(fliplrRho, rho);
    for (int x = 0; x < (int)fliplrRho->size1; x++) 
      for (int y = 0; y < (int)fliplrRho->size2; y++) {
        gsl_matrix_set(fliplrRho, (fliplrRho->size1) - x - 1, y, gsl_matrix_get(rho, x, y));
      }
    gsl_matrix *intermediate = gsl_matrix_calloc(D+1, rho->size1);
    if (!intermediate) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, diagalmostident, fliplrRho, 0.0, intermediate); 
    nrho = gsl_matrix_calloc(2*(rho->size1)-1, rho->size2);
    if (!nrho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int x = 0; x < ((int)rho->size1)-1; x++)
      for (int y = 0; y < (int)rho->size2; y++)
        gsl_matrix_set(nrho, x, y, .5 * gsl_matrix_get(rho, x, y));
    for (int x = (int)rho->size1-1; x < (int)nrho->size1; x++)
      for (int y = 0; y < (int)nrho->size2; y++) {
        gsl_matrix_set(nrho, x, y, .5 * gsl_matrix_get(intermediate, y, x - ((int)(rho->size1) - 1))); 
      }
    //rho(1,n-1+(1:n))=rho(1,n-1+(1:n))+1;
    for (int rindex = (int)rho->size1-1; rindex < (int)nrho->size1; rindex++) {
      gsl_matrix_set(nrho, rindex, 0, gsl_matrix_get(nrho, rindex, 0) + 1);
    }
    n=2*n-1; 
  }
  gsl_matrix *pval_rf=0;
  gsl_matrix *invol=0;
  if (fwhm > 0) {
    double radius = 0.0;
    gsl_vector *resels = gsl_vector_calloc(4);
    if (!resels) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    if (lsv == 1) {
      radius = pow((v.searchVolume/((4.0/3.0)*pi)), (1.0/3.0));
      gsl_vector_set(resels, 0, 1);
      gsl_vector_set(resels, 1, 4*radius/fwhm);
      gsl_vector_set(resels, 2, pow(radius/fwhm, 2)*2*pi); 
      gsl_vector_set(resels, 3, v.searchVolume/pow(fwhm, 3));
    }
    else 
      for (int dim = 0; dim < (D+1); dim++)
        gsl_vector_set(resels, dim, v.searchVolume/pow(fwhm, dim));
    invol = gsl_matrix_calloc(1, D+1);
    if (!invol) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int dim = 0; dim < D+1; dim++) {
      gsl_matrix_set(invol, 0, dim, gsl_vector_get(resels, dim) * pow((4*log(2.0)), (dim/2.0)));
    }
    if (is_tstat) {
      pval_rf = gsl_matrix_calloc((int)invol->size1, (int)nrho->size1);
      if (!pval_rf) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, invol, nrho, 0.0, pval_rf);
    }
    else {
      nrho = gsl_matrix_calloc((int)rho->size1, (int)rho->size2);
      if (!nrho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_matrix_memcpy(nrho, rho);
      pval_rf = gsl_matrix_calloc((int)invol->size1, (int)nrho->size1);
      if (!pval_rf) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, invol, nrho, 0.0, pval_rf);
    }
  }
  else {
    if (!is_tstat) {
      nrho = gsl_matrix_calloc((int)rho->size1, (int)rho->size2);
      if (!nrho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_matrix_memcpy(nrho, rho);
    }
    pval_rf = gsl_matrix_calloc(1, (int)nrho->size1);
    if (!pval_rf) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int index = 0; index < (int)pval_rf->size2; index++)
      gsl_matrix_set(pval_rf, 0, index, DMAX);
  }
  //Bonferroni
  gsl_vector *pt = gsl_vector_calloc((int)nrho->size1);
  if (!pt) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int element = 0; element < (int)nrho->size1; element++) {
    gsl_vector_set(pt, element, gsl_matrix_get(nrho, element, 0));
  }
  gsl_vector *pval_bon = gsl_vector_calloc((int)nrho->size1);
  if (!pval_bon) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int element = 0; element < (int)pval_bon->size; element++)
    gsl_vector_set(pval_bon, element, v.numVoxels*gsl_vector_get(pt, element));
  gsl_vector *pval = gsl_vector_calloc((int)nrho->size1);
  if (!pval) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int element = 0; element < (int)nrho->size1; element++) {
    gsl_vector_set(pval, element, gsl_matrix_get(pval_rf, 0, element));
  }
  int tlim = 1;
  gsl_vector *bonsmoothing = gsl_vector_calloc((int)nrho->size1);
  if (!bonsmoothing) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int element = 0; element < (int)nrho->size1; element++) {
    gsl_vector_set(bonsmoothing, element, gsl_vector_get(pval_bon, element));
  }
  double peakThreshold;
  if (pValPeak <= tlim) {
    minterp1("", pval, t, pValPeak, peakThreshold); 
    v.peakthreshold=peakThreshold;
    minterp1("", bonsmoothing, t, pValPeak, peakThreshold);
    v.bonpeakthreshold=peakThreshold;
  }
  else {
    //pValPeak is treated as a peak value
    interp1("", t, pval, pValPeak, peakThreshold); 
    v.pvalpeak=peakThreshold;
    interp1("", t, bonsmoothing, pValPeak, peakThreshold);
    v.bonpvalpeak=peakThreshold;
  }
  gsl_vector_free(bonsmoothing);
  double extentThreshold=DMAX;
  double extentThreshold_1=DMAX;
  
  if (fwhm <= 0) {
    extentThreshold=extentThreshold_1=peakThreshold;
    return 0;
  }
  double cluster=clusterThreshold;
  gsl_vector_free(tt);
  double tt2;
  double ClusterThreshold;
  //cluster threshol
  if (clusterThreshold > tlim)
    tt2=clusterThreshold;
  else {
    //cluster threshold is treated as a probability
    minterp1("", pt, t, cluster, tt2); 
    v.clusterthreshold=ClusterThreshold=tt2;
  }
  gsl_vector *rhoRow = gsl_vector_calloc((int)nrho->size1);
  if (!rhoRow) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int i = 0; i < (int)rhoRow->size; i++) 
    gsl_vector_set(rhoRow, i, gsl_matrix_get(nrho, i, D));
  double rhoD;
  interp1("", t, rhoRow, tt2, rhoD);
  double p;
  interp1("", t, pt, tt2, p);
  //pre-selected peak
  gsl_vector *pval1 = gsl_vector_calloc((int)nrho->size1);
  if (!pval1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int i = 0; i < (int)pval1->size; i++) {
    gsl_vector_set(pval1, i, (gsl_vector_get(rhoRow, i)/rhoD));
  }
  double peakThreshold_1;
  if (pValPeak <= tlim) {
    minterp1("", pval1, t, pValPeak, peakThreshold_1); 
    v.peakthreshold1=peakThreshold_1;
  }
  else {
    //pValPeak is treated as a peak value
    interp1("", t, pval1, pValPeak, peakThreshold_1);       
    v.pvalpeak1=peakThreshold_1;
  }
  //expected number of clusters
  double a = 0;
  double numrv = 0;
  gsl_matrix *f = 0;
  gsl_vector *pSMax = 0;
  gsl_vector_free(y);
  double ny = 0.0;
  double dy = 0.0;
  double b1 = 0.0;
  gsl_matrix *mu = 0;
  double EL = gsl_matrix_get(invol, 0, D) * rhoD;
  cons = gsl_sf_gamma(D/2.0+1)*pow((4*log(2.0)),D/2.0)/pow(fwhm, D)*(rhoD)/p;

  double pS;
  if ((df2 == DMAX) && (dfw1 == DMAX)) {
    if (pValExtent < tlim) {
      pS=-log(1 - pValExtent)/EL;
      extentThreshold=pow((-log(pS)), D/2.0)/cons;
      pS=-log(1 - pValExtent); 
      extentThreshold_1=pow(-log(pS),D/2.0)/cons;
      v.extentthreshold=extentThreshold;
      v.extentthreshold1=extentThreshold_1;
    }
    else {
      //pValExtent is now treated as a spatial extent
      double pValExtent_1;
      pS=exp(pow(-(pValExtent*cons), (2/D)));
      pValExtent=1 - exp(-pS*EL);
      extentThreshold=pValExtent;
      pValExtent_1=1 - exp(-pS);
      extentThreshold_1=pValExtent_1;
      v.extentthreshold=extentThreshold;
      v.extentthreshold1=extentThreshold_1;
    }
  }
  else {
    //find dbn of S by taking logs then using fft for convolution
    ny = pow(2.0, 12.0);
    a = D/2.0;
    double minval = ((df1+df2 < dfw1) ? df1+df2 : dfw1);
    double maxval = ((sqrt(2.0/minval) > 1) ? sqrt(2.0/minval) : 1);
    double b2 = a * 10 * maxval;
    b1 = 0.0;
    if (df2 != DMAX) 
      b1 = a * log((1 - pow((1 - 0.000001), 2/(df2 - D))) * df2/2.0); 
    else
      b1 = a * log(-log(1 - .000001));
    dy = (b2 - b1)/ny;
    b1 = round(b1/dy)*dy;
    y = gsl_vector_calloc((int)ny);
    if (!y) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < ny; i++) {
      gsl_vector_set(y, i, (i)*dy+b1);
      //cout << i << " y " << gsl_vector_get(y, i) << endl;
    }
    numrv = 1 + ((D + 1) * ((df2 == DMAX) ? 1 : 0)) + (D * ((dfw1 == DMAX) ? 1 : 0)) + ((dfw2 == DMAX) ? 1 : 0);
    f = gsl_matrix_calloc((int)ny, (int)numrv);
    if (!f) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    mu = gsl_matrix_calloc(1, (int)numrv);
    if (!mu) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector *yy = gsl_vector_calloc((int)ny);
    if (!yy) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    if (df2 != DMAX) {
      //density of log(Beta(1,(df2-D)/2)^(D/2))
      for (int i = 0; i < ny; i++) {
        gsl_vector_set(yy, i, (exp(gsl_vector_get(y, i)/a)/df2*2.0));
        gsl_vector_set(yy, i, gsl_vector_get(yy, i) * ((gsl_vector_get(yy, i) < 1) ? 1 : 0));
        gsl_matrix_set(f, i, 0, pow((1 -gsl_vector_get(yy, i)), ((df2 - D)/2.0 -1))*((df2-D)/2.0)*gsl_vector_get(yy,i)/a); 
      }
      gsl_matrix_set(mu, 0, 0, exp(gsl_sf_lngamma(a+1)+gsl_sf_lngamma((df2-D+2)/2.0)-gsl_sf_lngamma((df2+2)/2)+a*log(df2/2.0)));

    }
    else {  
      //density of log(exp(1)^(D/2))
      for (int i = 0; i < ny; i++) {
        gsl_vector_set(yy, i, exp(gsl_vector_get(y,i)/a));
        gsl_matrix_set(f, 0, i, exp(-gsl_vector_get(yy, i) * gsl_vector_get(yy, i)/a));
      }
      gsl_matrix_set(mu, 0, 0, exp(gsl_sf_lngamma(a+1)));

    }
    gsl_vector *nuv=0;
    gsl_vector *aav=0;
    gsl_vector *nuvappended=0;
    gsl_vector *aavappended=0;
    if (df2 != DMAX) {
      nuv = gsl_vector_calloc((int)D + 1); //nuv = [df1+df2-D  df2+2-(1:D)]
      if (!nuv) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      gsl_vector_set(nuv, 0, df1+df2-D);
    }
    for (int i = 1; i < (D +1); i++)
      gsl_vector_set(nuv, i, df2+2-i); 
    aav = gsl_vector_calloc(D + 1);
    if (!aav) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector_set(aav, 0, a);
    for (int i = 1; i < (D+1); i++) {
      gsl_vector_set(aav, i, -.5);
    }
    if (dfw1 != DMAX) {
      nuvappended = gsl_vector_calloc((int)nuv->size + (int)D + 1);
      if (!nuvappended) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      aavappended = gsl_vector_calloc((int)aav->size + (int)D + 1);
      if (!aavappended) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      if (dfw1 > dfLimit) {
        for (int i = 0; i < (int)(nuv->size); i++)
          gsl_vector_set(nuvappended, i, gsl_vector_get(nuv, i)); 
        for (int i = (int)(nuv->size), j = 0; i < (int)(nuv->size + D + 1); i++, j++)
          gsl_vector_set(nuvappended, i, (dfw1 - dfw1/dfw2 - j)); 
      }
      else {
        for (int i = 0; i < (int)(nuv->size); i++)
          gsl_vector_set(nuvappended, i, gsl_vector_get(nuv, i));
        for (int i = (int)(nuv->size); i < (int)(nuv->size + D + 1); i++)
          gsl_vector_set(nuvappended, i, (dfw1 - dfw1/dfw2));    
      }
      for (int i = 0; i < ((int)aav->size); i++)
        gsl_vector_set(aavappended, i, gsl_vector_get(aav, i));
      for (int i = ((int)aav->size); i < ((int)aav->size + D + 1); i++)
        gsl_vector_set(aavappended, i, .5);
    }
    if (dfw2 != DMAX) {
      if (!nuvappended) {
        nuvappended = gsl_vector_calloc((int)nuv->size + 1);
        if (!nuvappended) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      }
      if (!aavappended) {
        aavappended = gsl_vector_calloc((int)aav->size + 1);
        if (!aavappended) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
      }
      for (int i = 0; i < (int)(nuv->size); i++)
        gsl_vector_set(nuvappended, i, gsl_vector_get(nuv, i));
      for (int i = 0; i < (int)(aav->size); i++)
        gsl_vector_set(aavappended, i, gsl_vector_get(aav, i));
      gsl_vector_set(nuvappended, (int)nuv->size, dfw2);
      gsl_vector_set(aavappended, (int)aav->size, -a);
    }
    gsl_vector *nu = gsl_vector_calloc((int)numrv);
    if (!nu) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector *aa = gsl_vector_calloc((int)numrv);
    if (!aa) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_vector_free(yy);
    yy = gsl_vector_calloc((int)y->size);
    if (!yy) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < numrv-1; i++) {
      gsl_vector_set(nu, i, gsl_vector_get(nuv, i));
      gsl_vector_set(aa, i, gsl_vector_get(aav, i));
      for (int j = 0; j < (int)y->size; j++) {
        gsl_vector_set(yy, j, gsl_vector_get(y, j)/gsl_vector_get(aa, i) + log(gsl_vector_get(nu, i))); 
        //cout << "yy" << j << ": " << gsl_vector_get(yy, j) << endl;
      }
      //density of log((chi^2_nu/nu)^aa
      for (int j = 0; j < (int)y->size; j++) {
        gsl_matrix_set(f, j, i+1, exp(gsl_vector_get(nu, i)/2.0*gsl_vector_get(yy, j)-exp(gsl_vector_get(yy,j))/2.0-(gsl_vector_get(nu, i)/2.0)*log(2.0)-gsl_sf_lngamma(gsl_vector_get(nu, i)/2.0))/fabs(gsl_vector_get(aa,i))); 
      }
      gsl_matrix_set(mu, 0, i+1, exp(gsl_sf_lngamma(gsl_vector_get(nu, i)/2.0+gsl_vector_get(aa, i))-gsl_sf_lngamma(gsl_vector_get(nu, i)/2.0)-gsl_vector_get(aa, i)*log(gsl_vector_get(nu, i)/2.0)));

      gsl_vector_set_zero(nu);
      gsl_vector_set_zero(aa);
      gsl_vector_set_zero(yy);
    }
    //Check: plot(y, f); sum(f*dy,1) should be 1
    gsl_vector *omega = gsl_vector_calloc((int)ny+1);
    if (!omega) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    gsl_complex *omegaComplex = new gsl_complex[(int)ny];
    if (!omegaComplex) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < (int)ny; i++)
      gsl_vector_set(omega, i, ((2*pi)*(i))/ny/dy);
    for (int i = 0; i < (int)ny; i++) {
      GSL_SET_COMPLEX(&omegaComplex[i], cos(-b1*gsl_vector_get(omega, i))*dy, sin(-b1*gsl_vector_get(omega,i))*dy);
    }
    //prodfft=prod(fft(f),2).*shift.^(numrv-1)
    for (int i = 0; i < (int)ny; i++) {
      omegaComplex[i] = gsl_complex_pow_real(omegaComplex[i], double((numrv-1))); 
      //cout << i << " " << GSL_REAL(omegaComplex[i]) << " " << GSL_IMAG(omegaComplex[i]) << endl; 
    }
    VB_Vector vbf[(int)numrv];
    VB_Vector realArray1[(int)numrv];
    VB_Vector imagArray1[(int)numrv];
    for (int x = 0; x < (int)numrv; x++) {
      vbf[x].resize((int)ny);
      realArray1[x].resize((int)ny);
      imagArray1[x].resize((int)ny);
    }
    for (int x = 0; x < (int)f->size2; x++) {
      for (int y = 0; y < (int)f->size1; y++) { 
        vbf[x][y] = gsl_matrix_get(f, y, x);  
      }
      vbf[x].fft(realArray1[x], imagArray1[x]);
      for (int y = 0; y < ny; y++) {
        realArray1[x][y] *= (ny);
        imagArray1[x][y] *= (ny);
        //cout << realArray1[x][y] << " " << imagArray1[x][y] << endl;
      }
    }
    gsl_complex *complexarray = new gsl_complex[(int)ny];
    gsl_complex *complexitem = new gsl_complex;
    for (int fff= 0; fff < (int)ny; fff++) {
      GSL_SET_COMPLEX(complexitem, realArray1[0][fff], imagArray1[0][fff]);
      for (int ggg= 1; ggg < (int)numrv; ggg++) {
        GSL_SET_COMPLEX(&complexarray[fff], realArray1[ggg][fff], imagArray1[ggg][fff]); 
        *complexitem = gsl_complex_mul(*complexitem, complexarray[fff]); 
      } 
      GSL_SET_COMPLEX(&complexarray[fff], GSL_REAL(*complexitem), GSL_IMAG(*complexitem));
      //cout << GSL_REAL(*complexitem) << " " << GSL_IMAG(*complexitem) << endl;
      memset((void*)complexitem, 0, sizeof(gsl_complex));
    }
    gsl_complex *prodfft = new gsl_complex[(int)ny];
    VB_Vector realProd((int)ny);
    VB_Vector imagProd((int)ny);
    for (int i = 0; i < (int)ny; i++) {
      //cout << GSL_REAL(complexarray[i]) << ", " << GSL_IMAG(complexarray[i]) << " times " <<
      //        GSL_REAL(omegaComplex[i]) << ", " << GSL_IMAG(omegaComplex[i]) << endl;
      prodfft[i] = gsl_complex_mul(complexarray[i], omegaComplex[i]);    
      realProd[i] = GSL_REAL(prodfft[i]);
      imagProd[i] = GSL_IMAG(prodfft[i]);
      //cout << GSL_REAL(prodfft[i]) << " " << GSL_IMAG(prodfft[i]) << endl;
    }
    delete [] omegaComplex;
    VB_Vector realProdFFT;
    realProdFFT.resize((int)ny);
    //density of y=log(B^(D/2)*U^(D/2)/sqrt(det(Q)))
    //ff=real(ifft(prodfft))
    VB_Vector::complexIFFTReal(realProd, imagProd, realProdFFT); 
    //check plot(y, ff); sum(ff*dy) should be 1
    gsl_vector *ff = gsl_vector_calloc((int)ny);
    if (!ff) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < (int)ny; i++) {
      gsl_vector_set(ff, i, realProdFFT[i]/ny);
    }
    double mu0 = gsl_matrix_get(mu, 0, 0); 
    for (int i = 1; i < numrv; i++)
      mu0 *= gsl_matrix_get(mu, 0, i);
    //check: plot(y,ff.*exp(y)); sum(ff.*exp(y)*dy.*(y<10)) should equal mu0
    double alpha =  p/rhoD/mu0*pow(fwhm,D)/pow(4*log(2.0),D/2.0);
    //integrate the density to get the p-value for one cluster
    gsl_vector *pStemp = gsl_vector_calloc((int)ny);
    if (!pStemp) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    double total = 0.0;
    for (int i = (int)ff->size-1; i >= 0; i--) {
      total+=gsl_vector_get(ff, i);
      gsl_vector_set(pStemp, i, (total * dy));
      //cout << "pS" << i << ": " << gsl_vector_get(pStemp, i) << endl; //" ff: " << gsl_vector_get(ff, i) << " total: " << total << endl;
    }
    gsl_vector *pSx=gsl_vector_alloc((int)ny);
    for (int i = (int)ny - 1; i >= 0; i--) {
      gsl_vector_set(pSx, i, gsl_vector_get(pStemp, i));    // FIXME pSx set but never used???
    }
    pSMax = gsl_vector_calloc((int)ny);
    if (!pSMax) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < (int)ny; i++) {
      gsl_vector_set(pSMax, i, 1 - exp(-1*gsl_vector_get(pStemp, i)*EL));
    }
    double yval;
    double pValExtent_1;
    if (pValExtent <= tlim) {
      pValExtent=-1.0*pValExtent;
      for (int i = 0; i < (int)pSMax->size; i++)
        gsl_vector_set(pSMax, i, gsl_vector_get(pSMax, i)*(-1));
      minterp1("", pSMax, y, pValExtent, yval);  
      //spatial extent is alpha*exp(Y) -dy/2 correction for mid-point rule
      extentThreshold=alpha*exp(yval-(dy/2.0));
      if (yval>FLT_MIN)
        v.extentthreshold=extentThreshold;
      //for a single cluster
      for (int i = 0; i < (int)pStemp->size; i++) 
        gsl_vector_set(pStemp, i, gsl_vector_get(pStemp, i)*-1);
      yval=0;
      minterp1("", pStemp,y,pValExtent,yval); 
      double extentthresh1;
      extentthresh1=alpha*exp(yval-(dy/2.0));
      if (yval>FLT_MIN)
        v.extentthreshold1=extentthresh1;
    }
    else {
      //pValExtent is now treated as a spatial extent
      pValExtent=log(pvalextent/alpha)+dy/2.0;
      pValExtent_1=log(pvalextent/alpha)+dy/2.0;
      interp1("", y, pSMax, pValExtent, pValExtent); 
      v.pvalextent=(unsigned int)pValExtent;
      extentThreshold_1=(unsigned int)pValExtent;
      //for a single cluster
      interp1("",y, pStemp, pValExtent_1, pValExtent_1);      
      v.pvalextent1=(unsigned int)pValExtent_1;
      extentThreshold_1=(unsigned int)pValExtent_1;
    }
  }
  return 0;
}
