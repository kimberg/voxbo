
// stats.h
// prototypes, etc. for use by voxbolib
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

#ifndef VBSTATS_H
#define VBSTATS_H

using namespace std;

#include "vbio.h"

class tval {
public:
  tval() {t=df=p=z=sd=stderror=diff=halfci=0.0;}
  tval(double xt,double xdf) {t=xt;df=xdf;}
  double t;
  double df;
  double p;
  double z;
  double sd;
  double stderror;
  double diff;
  double halfci;
};

class x2val {
public:
  x2val() {x2=df=p=z=0.0;}
  x2val(double xx2,double xdf) {x2=xx2;df=xdf;}
  double x2;
  double df;
  double p;
  double z;
  int c00,c01,c10,c11;
};

tval calc_ttest(VB_Vector &vec,bitmask &bm);
tval calc_ttest(VB_Vector &v1,VB_Vector &v2);
tval calc_welchs(VB_Vector &vec,bitmask &bm);
tval calc_welchs(VB_Vector &v1,VB_Vector &v2);
x2val calc_chisquared(bitmask bm1,bitmask bm2,bool yatesflag=0);
x2val calc_fisher(bitmask bm1,bitmask bm2);
void t_to_p_z(tval &res,bool twotailed=0);
VBVoxel find_fdr_thresh(Tes &vol,double q);

#endif // VBSTATS_H
