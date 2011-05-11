
// statthreshold.cpp
// library code to produce thresholds
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
// original version written by Tom King and Dan Kimberg

// This code was written to implement the algorithm previous
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
interp1(string, vector<double>mx, vector<double>my, vector<double>ixstl, vector<double>&thresholds)
{
  vector<double>ret;
  double val = 0.0;
  for (int o = 0; o < (int)ixstl.size(); o++) {
    if (ixstl[o] > mx[mx.size()-1] || ixstl[o] < mx[0]) {
      val = DMAX;
      thresholds.push_back(val);
      val = 0;
    }
    else
      for (int p = 0; p < (int)mx.size(); p++) {
        if (mx[p] >= ixstl[o]) {
          if (mx.size() > 1) 
            val = my[p-1]+((my[p]-my[p-1])/(mx[p]-mx[p-1]))*((ixstl[o]-mx[p-1]));
          else
            val = my[p];
          thresholds.push_back(val);
          val = 0;
          break;
        }          
      }
  }
  return;
}

void
interp1(string type, gsl_vector *x, gsl_vector *y, gsl_vector *ix, gsl_vector *thresholds)
{
  double val = 0.0;
  double diff = 0.0;
  double mindiff = DMAX; 
  int index = 0;
  for (int o = 0; o < (int)ix->size; o++) {
    mindiff = DMAX;
    index = -1;
    val = 0;
    for (int p = 1; p < (int)x->size; p++) {
      diff = gsl_vector_get(x, p) - gsl_vector_get(ix, o);
      if (abs(diff) < abs(mindiff)) {
	mindiff = diff;
	index = p;
      }
    }
    if (index == -1) {
      gsl_vector_set(thresholds, o, DMAX);
      continue;
    } 
    if (mindiff < 0)
      val = gsl_vector_get(y, index-1)+(gsl_vector_get(y,index)-gsl_vector_get(y,index-1))/(gsl_vector_get(x,index)-gsl_vector_get(x,index-1))*(gsl_vector_get(ix,o)-gsl_vector_get(x,index-1));
    else
      val = gsl_vector_get(y, index+1)+(gsl_vector_get(y,index)-gsl_vector_get(y,index+1))/(gsl_vector_get(x,index)-gsl_vector_get(x,index+1))*(gsl_vector_get(ix,o)-gsl_vector_get(x,index+1)); 
    gsl_vector_set(thresholds, o, val);
    if (type.size())
      cout << setprecision(20) << type << " " << "threshold" << o << " " << gsl_vector_get(thresholds, o) << endl;
  }
  return;
}

void minterp1(string type, gsl_vector *x, gsl_vector *y, gsl_vector *ix, gsl_vector *iy) {
  vector<double>mxstl;
  vector<double>mystl;
  vector<double>ixstl;
  vector<double>thresholds;
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
  for (int d = 0; d < (int)ix->size; d++)
    ixstl.push_back(gsl_vector_get(ix, d)); 
  interp1(type, mxstl, mystl, ixstl, thresholds);
  for (int u = 0; u < (int)thresholds.size(); u++)
    if (type.size())
      cout << type << setprecision(20) << " threshold" << u << " " << thresholds[u] << endl;
  for (int u = 0; u < (int)thresholds.size(); u++)
    gsl_vector_set(iy, u, thresholds[u]);
  return;
} 

int
stat_threshold(threshold &v)
{
  long searchVolume = v.searchVolume;
  long numVoxels = v.numVoxels;
  double fwhm = v.fwhm;
  double df1,df2;
  double pValPeak=v.pValPeak;
  double clusterThreshold = v.clusterThreshold;
  double pValExtent=v.pValExtent;
  int nvar = v.nvar;
  double transform = v.transform;
  if (searchVolume == 0) searchVolume = 1000000;
  if (numVoxels == 0) numVoxels = 1000000;
  if (pValPeak==0) pValPeak=0.05;
  if (clusterThreshold == 0) clusterThreshold = .001;
  if (pValExtent==0) pValExtent=.05;
  if (nvar == 0) nvar = 1;
  int lsv = sizeof(searchVolume)/sizeof(long);
  int numDimensions = 0;
  if (lsv == 1) 
    numDimensions = 3;
  else
    numDimensions = sizeof(searchVolume)/sizeof(long) - 1;

  int dfLimit = 4;
  int is_tstat = 0;
  if (v.df[0] == 0) is_tstat = 1;
  if (is_tstat) {
    df1 = 1;
    df2 = v.df[1];
  }
  else {
    df1 = v.df[0];
    df2 = v.df[1];
  }
  if (df2 > 1000) df2 = DMAX;
  double pvalextent=pValExtent;
  // FIXME what the heck???
  VB_Vector t(1901);
  int tcount = 0;
  for (double val = 100; val > 10.1; val-=.1, tcount++) 
    gsl_vector_set(t, tcount, val*val);
  for (double val = 10; val >= 0; val-=.01, tcount++) 
    gsl_vector_set(t, tcount, val*val); 
  int n = 1901;
  VB_Vector tt(n-1);
  for (tcount=0; tcount < (n-1); tcount++) {
    if (tcount != n-2)
      tt[tcount]=(t[tcount]+t[tcount+1])/2.0;
    else 
      tt[tcount=t[count]/2.0;
  }
  VB_Vector b(n-1);
  if (1) {
    b=tt;
    b*=df1;
    b*=-.5;
    b+=-gsl_sf_lngamma(df1/2.0)-(df1/2.0*log(2.0));
    VB_Vector temp(n-1);
    temp=tt;
    temp*=df1;
    for (int tempcount=0; tempcount < (n-1); tempcount++) 
      temp[tempcount]=log(temp[tempcount]);
    temp*=df1/2.0-1;
    VB_Vector final(n-1);
    final=temp;
    final+=b;
    for (int bcount=0; bcount < (n-1); bcount++) 
      b[bcount]=exp(final[bcount]);
  }
  int boolFwhmTrue = 0;
  if (fwhm) boolFwhmTrue = 1;
  int D = numDimensions*boolFwhmTrue;
  gsl_matrix *tau = gsl_matrix_calloc(D+nvar, n);
  if (!tau) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  VB_Vector difft(n-1);
  for (int taucount=0; taucount < (n-1); taucount++) { 
    if (taucount + 1 < n)  
      difft[taucount]=t[taucount+1]-t[taucount];
    else
      difft[taucount]=0;
  }
  VB_Vector bdifft(n-1);
  bdifft=b;
  bdifft*=difft;

  VB_Vector csbdifft(n-1);
  for (int taucount=0; taucount < (n-1); taucount++) {
    if (taucount == 0)
      csbdifft[0]=bdifft[taucount];
    else
      csbdifft[taucount]=csbdifft[taucount-1]+bdifft[taucount];
  }
  gsl_matrix_set(tau, 0, 0, 0.0-gsl_vector_get(csbdifft,0));
  for (int taucount=1; taucount < (n); taucount++) {
    gsl_matrix_set(tau,0,taucount,0.0-gsl_vector_get(csbdifft,taucount-1));
  }

  //finding EC densities
  VB_Vector y=t;
  y*=df1;
  double min = 0.0;
  VB_Vector s1(y->size());
  double cons = 0.0;
  for (int N = 1;  N <= (D + nvar - 1); N++) {
    s1*=0.0;
    for (int i = 0; i <= N-1; i++) {
      if ((N - 1 - i) > i)
        min = i;
      else
        min = N - 1 - i;
      VB_Vector j(int(min)+1);
      for (int value = 0; value <= min; value++)  
        j[value]=value;
      VB_Vector s2(int(min)+1);
      VB_Vector arg1(int(min)+1);
      VB_Vector arg2(int(min)+1);
      double temp = 0.0;
      if (df2 == DMAX) {
        if (min) {
          temp = df1 - 1.0;
          arg1=j;
          arg2=j;
          arg1+=temp;
          arg1-=j;
          temp = N - 1 - i;
          arg2*=-1;
          arg2+=temp;
          for (int m = 0; m < (int)s2->size; m++) {
            s2[m]=gsl_vector_get(nchoosekln(arg1, arg2), m);
          }
          arg1=j;
          arg2*=0.0;
          for (int x = 0; x < (int)arg1->size; x++) {
            arg1[x]=gsl_sf_lngamma(arg1[x]+1);
            s2[x]=s2[x]-arg1[x];
          }
          arg1=j;
          for (int x = 0; x < (int)arg1->size; x++) {
            arg1[x]=gsl_sf_lngamma(i - (arg1[x]) + 1);
            s2[x]=s2[x]-arg1[x];
          }
          arg1=j;
          for (int x = 0; x < (int)arg1->size; x++) {
            arg1[x]=arg1[x]*log(2.0); 
            s2[x]=s2[x]-arg1[x];
          }
          double sums2 = 0.0;           
          for (int val = 0; val < (int)s2->size; val++) 
            sums2 += exp(s2[val]);
          s2[0]=sums2;
          sums2 = 0;
        }
        else {
          VB_Vector n(1);
          VB_Vector k(1);
          n[0]=df1-1.0;
          k[0]=N-1-i;
          s2 = nchoosekln(n, k);
          for (int m = 0; m < (int)s2->size; m++) {
            s2[m]=exp(s2[m]-gsl_sf_lngamma(1)-gsl_sf_lngamma(i+1)));
          }
        }
      }
      else {
        if (min) {
          VB_Vector n1=j;
          VB_Vector k1=j;
          VB_Vector n2=j;
          VB_Vector k2=j;
          VB_Vector n3=j;
          VB_Vector k3=j;
          VB_Vector r1(int(min)+1);
          VB_Vector r2(int(min)+1);
          VB_Vector r3(int(min)+1);
          VB_Vector sumrs(int(min)+1);

          for (int n1c=0; n1c < (int)n1->size; n1c++)
            gsl_vector_set(n1, n1c, df1-1+gsl_vector_get(j, n1c)-gsl_vector_get(j, n1c));
          for (int k1c=0; k1c < (int)k1->size; k1c++)
            gsl_vector_set(k1, k1c, N-1-i-gsl_vector_get(j, k1c));
          for (int n2c=0; n2c < (int)n2->size; n2c++)
            gsl_vector_set(n2, n2c, (df1+df2-N)/2.0+gsl_vector_get(j, n2c)-1);
          for (int n3c=0; n3c < (int)n3->size; n3c++)
            gsl_vector_set(n3, n3c, df2-1+gsl_vector_get(j, n3c)-gsl_vector_get(j, n3c));
          for (int k3c=0; k3c < (int)k3->size; k3c++)
            gsl_vector_set(k3, k3c, i - gsl_vector_get(j, k3c));     
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
          gsl_vector_free(r1);
          gsl_vector_free(r2);
          gsl_vector_free(r3);
          gsl_vector_free(sumrs);
        }
        else {
          VB_Vector n1=(int(min)+1);
          VB_Vector k1=(int(min)+1);
          VB_Vector n2=(int(min)+1);
          VB_Vector k2=(int(min)+1);
          VB_Vector n3=(int(min)+1);
          VB_Vector k3=(int(min)+1);
          VB_Vector r1(int(min)+1);
          VB_Vector r2(int(min)+1);
          VB_Vector r3(int(min)+1);
          gsl_vector_set(n1, 0, df1 - 1.0 + gsl_vector_get(j, 0) - gsl_vector_get(j, 0));
          gsl_vector_set(k1, 0, N - 1 - i - gsl_vector_get(j, 0));
          gsl_vector_set(n2, 0, (df1 + df2 - N)/2.0 + gsl_vector_get(j, 0) -1);
          gsl_vector_set(k2, 0, gsl_vector_get(j, 0));      
          gsl_vector_set(n3, 0, df2-1+gsl_vector_get(j, 0)-gsl_vector_get(j, 0));
          gsl_vector_set(k3, 0, i - gsl_vector_get(j, 0));     
          gsl_vector_set(r1, 0, gsl_vector_get(nchoosekln(n1, k1), 0));
          gsl_vector_set(r2, 0, gsl_vector_get(nchoosekln(n2, k2), 0));
          gsl_vector_set(r3, 0, gsl_vector_get(nchoosekln(n3, k3), 0));
          gsl_vector_set(s2, 0,exp(gsl_vector_get(r1, 0)+gsl_vector_get(r2, 0)+gsl_vector_get(r3, 0)-i*log(df2)));
        }
      }
      int isPositive = 0;
      if (s2[0] > 0)
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
  /*
    for (int x = 0; x < (int)tau->size1; x++)
    for (int y = 0; y < (int)tau->size2; y++)
    cout << setprecision(20) << x << " by " << y << " = " << gsl_matrix_get(tau, x, y) << endl;
  */
  //adding sphere to search region
  if (j) gsl_vector_free(j);
  j = gsl_vector_calloc(nvar); 
  if (!j) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int index = 0; index < nvar; index++) {
    if (index%2==0)
      gsl_vector_set(j, index, ((nvar-1) - (index)));
    else
      gsl_vector_set(j, index, 0);
  }
  gsl_vector *involSphere = gsl_vector_calloc(nvar);
  if (!involSphere) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for(int x = 0; x < nvar; x+=2) {
    gsl_vector_set(involSphere, nvar-x-1, exp(gsl_vector_get(j, x)*log(2.0)+gsl_vector_get(j, x)/2.0*log(pi)+ gsl_sf_lngamma((nvar+1)/2.0)- gsl_sf_lngamma((nvar+1-gsl_vector_get(j, x))/2.0)- gsl_sf_lngamma(gsl_vector_get(j, x) + 1)));

  }
  //create toeplitz matrix
  double insertionValue = 0;
  gsl_matrix *rho = gsl_matrix_calloc(n, D + 1);
  if (!rho) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_matrix *toeplitz = gsl_matrix_calloc(D + 1, nvar + D);
  if (!toeplitz) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int rindex = 0; rindex < (D + 1); rindex++) 
    for (int cindex = 0; cindex < (nvar + D); cindex++) {
      if (cindex < (int)involSphere->size) {
        insertionValue = gsl_vector_get(involSphere, cindex);
        gsl_matrix_set(toeplitz, rindex, (cindex+rindex)%(nvar + D), insertionValue);
      }
    }
  gsl_blas_dgemm(CblasTrans, CblasTrans, 1.0, tau, toeplitz, 0.0, rho);
  gsl_matrix_free(toeplitz);
  gsl_matrix_free(tau);
  gsl_matrix *nrho=0;
  if (is_tstat) {
    VB_Vector newt((2*((int)t.size()))-1);
    for (int index = 0; index < ((int)t->size-1); index++) {
      double value = t[t.size() - 1];
      newt[index]=sqrt(t[index])-value;
    }
    newt[(int)t.size()-1]=0;
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
        //cout << y << " and " << x - ((int)(rho->size1) - 1) << " vs " << x << " and " << y << endl;
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
      radius = pow((searchVolume/((4.0/3.0)*pi)), (1.0/3.0));
      gsl_vector_set(resels, 0, 1);
      gsl_vector_set(resels, 1, 4*radius/fwhm);
      gsl_vector_set(resels, 2, pow(radius/fwhm, 2)*2*pi); 
      gsl_vector_set(resels, 3, searchVolume/pow(fwhm, 3));
    }
    else 
      for (int dim = 0; dim < (D+1); dim++)
        gsl_vector_set(resels, dim, searchVolume/pow(fwhm, dim));
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
  VB_Vector pt(nrho->size1);
  for (int element = 0; element < (int)nrho->size1; element++)
    pt[element]=gsl_matrix_get(nrho, element, 0);
  VB_Vector pval_bon(nrho->size1);
  for (int element = 0; element < pval_bon.size(); element++)
    pval_bon[element]=numVoxels*gsl_vector_get(pt, element);
  // formerly gsl_vector_set(pval_bon, element, numVoxels*gsl_vector_get(pt, element));
  VB_Vector pval(nrho->size1);
  for (int element = 0; element < (int)nrho->size1; element++) {
    pval[element]=gsl_matrix_get(pval_rf, 0, element);
    // gsl_vector_set(pval, element, gsl_matrix_get(pval_rf, 0, element));
  }
  int tlim = 1;
  VB_Vector bonsmoothing(nrho->size1);
  for (int element = 0; element < (int)nrho->size1; element++)
    bonsmoothing[element]=pval_bon[element];
  VB_Vector peakThreshold(pValPeak->size);
  if (pValPeak[0] <= tlim) {
    minterp1("", pval, t, pValPeak, peakThreshold);
    for (int element = 0; element < peakThreshold.size(); element++)
      v.peakthreshold[element]=peakThreshold[element];
    minterp1("", bonsmoothing.theVector, t, pValPeak, peakThreshold);
    for (int element = 0; element < v.bonpeakthreshold.size(); element++)
      v.bonpeakthreshold[element]=peakThreshold[element];
  }
  else {
    //pValPeak is treated as a peak value
    interp1("", t, pval, pValPeak, peakThreshold); 
    for (int element = 0; element < v.pvalpeak.size(); element++)
      v.pvalpeak[element]=peakThreshold[element];
    interp1("", t, bonsmoothing.theVector, pValPeak, peakThreshold);
    for (int element = 0; element < v.bonpvalpeak.size(); element++)
      v.bonpvalpeak[element]=peakThreshold[element];
  }
  // gsl_vector_free(bonsmoothing);
  VB_Vector extentThreshold(pValPeak->size);
  VB_Vector extentThreshold_1(pValPeak->size);
  if (!extentThreshold_1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;

  for (int i = 0; i < (int)extentThreshold->size; i++)
    gsl_vector_set(extentThreshold, i, DMAX);
  for (int i = 0; i < (int)extentThreshold->size; i++)
    gsl_vector_set(extentThreshold_1, i, DMAX);
  if (fwhm <= 0) {
    for (int i = 0; i < (int)extentThreshold->size; i++)
      gsl_vector_set(extentThreshold, i, gsl_vector_get(peakThreshold, i)); 
    for (int i = 0; i < (int)extentThreshold->size; i++)
      gsl_vector_set(extentThreshold_1, i, gsl_vector_get(extentThreshold, i));   
    return 0;
  }
  gsl_vector *cluster = gsl_vector_calloc(1);
  if (!cluster) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_vector_set(cluster, 0, clusterThreshold);
  gsl_vector_free(tt);
  tt = gsl_vector_calloc(1);
  if (!tt) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  gsl_vector *ClusterThreshold = gsl_vector_calloc(1);
  if (!ClusterThreshold) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  v.clusterthreshold = gsl_vector_calloc(1);
  if (!v.clusterthreshold) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  //cluster threshol
  if (clusterThreshold > tlim) {
    gsl_vector_set(tt, 0, clusterThreshold);
  }
  else {
    //cluster threshold is treated as a probability
    minterp1("", pt, t, cluster, tt); 
    gsl_vector_set(v.clusterthreshold, 0, gsl_vector_get(tt, 0));
    gsl_vector_set(ClusterThreshold, 0, gsl_vector_get(tt, 0));
  }
  gsl_vector *rhoRow = gsl_vector_calloc((int)nrho->size1);
  if (!rhoRow) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int i = 0; i < (int)rhoRow->size; i++) 
    gsl_vector_set(rhoRow, i, gsl_matrix_get(nrho, i, D));
  gsl_vector *rhoD = gsl_vector_calloc(1);
  if (!rhoD) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  interp1("", t, rhoRow, tt, rhoD);
  gsl_vector *p = gsl_vector_calloc(1);
  if (!p) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  interp1("", t, pt, tt, p);
  //pre-selected peak
  gsl_vector *pval1 = gsl_vector_calloc((int)nrho->size1);
  if (!pval1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  for (int i = 0; i < (int)pval1->size; i++) {
    gsl_vector_set(pval1, i, (gsl_vector_get(rhoRow, i)/gsl_vector_get(rhoD, 0)));
  }
  gsl_vector *peakThreshold_1 = gsl_vector_calloc((int)pValPeak->size);
  if (!peakThreshold_1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  if (gsl_vector_get(pValPeak, 0) <= tlim) {
    minterp1("", pval1, t, pValPeak, peakThreshold_1); 
    for (int element = 0; element < (int)v.peakthreshold1->size; element++)
      gsl_vector_set(v.peakthreshold1, element, gsl_vector_get(peakThreshold_1, element));
  }
  else {
    //pValPeak is treated as a peak value
    interp1("", t, pval1, pValPeak, peakThreshold_1);       
    for (int element = 0; element < (int)v.pvalpeak1->size; element++)
      gsl_vector_set(v.pvalpeak1, element, gsl_vector_get(peakThreshold_1, element));
    //peakThreshold_1 = pValPeak_1;
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
  double EL = gsl_matrix_get(invol, 0, D) * gsl_vector_get(rhoD, 0);
  cons = gsl_sf_gamma(D/2.0+1)*pow((4*log(2.0)),D/2.0)/pow(fwhm, D)*(gsl_vector_get(rhoD, 0))/gsl_vector_get(p, 0);

  gsl_vector *pS = gsl_vector_calloc((int)pValExtent->size);
  if (!pS) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
  if (gsl_vector_get(pValExtent, 0) < tlim) {
    for (int i = 0; i < (int)pValExtent->size; i++) {
      gsl_vector_set(pS, i, -log(1 - gsl_vector_get(pValExtent, i))/EL);
      //cout << "pS: " << gsl_vector_get(pS, i) << endl;
      gsl_vector_set(extentThreshold, i, pow((-log(gsl_vector_get(pS,i))), D/2.0)/cons); 
      gsl_vector_set(pS, i, -log(1 - gsl_vector_get(pValExtent, i))); 
      gsl_vector_set(extentThreshold_1, i, pow(-log(gsl_vector_get(pS,i)),D/2.0)/cons); 
      gsl_vector_set(v.extentthreshold, i, gsl_vector_get(extentThreshold, i)); 
      gsl_vector_set(v.extentthreshold1, i, gsl_vector_get(extentThreshold_1, i));
    }
  }
  else {
    //pValExtent is now treated as a spatial extent
    gsl_vector *pValExtent_1 = gsl_vector_calloc(pValExtent->size);
    if (!pValExtent_1) cout << __FILE__ << ", line " << __LINE__ << " failed to allocate memory." << endl;
    for (int i = 0; i < (int)pValExtent->size; i++) {
      gsl_vector_set(pS, i, exp(pow(-(gsl_vector_get(pValExtent, i)*cons), (2/D))));
      gsl_vector_set(pValExtent, i, 1 - exp(-gsl_vector_get(pS, i)*EL));
      gsl_vector_set(extentThreshold, i, gsl_vector_get(pValExtent, i));
      gsl_vector_set(pValExtent_1, i, 1 - exp(-gsl_vector_get(pS, i)));
      gsl_vector_set(extentThreshold_1, i, gsl_vector_get(pValExtent_1, i));
      gsl_vector_set(v.extentthreshold, i, gsl_vector_get(extentThreshold, i));
      gsl_vector_set(v.extentthreshold1, i, gsl_vector_get(extentThreshold_1, i));
    }
  }

  return 0;
}
