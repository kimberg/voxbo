
// vbcmp.cpp
// compare 3d and 4d files, data only
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vbutil.h"
#include "vbio.h"
#include "vbcmp.hlp.h"

using namespace std;

void vbcmp_help();
void vbcmp_version();
void cmp1d(VB_Vector &v1,VB_Vector &v2,string spacer);
void cmp2d(VBMatrix &mat1,VBMatrix &mat2,string spacer);
void cmp3d(Cube &c1,Cube &c2,Cube &mask,string spacer);
void cmp4d(Tes &t1,Tes &t2,Cube &mask,string spacer);

int
main(int argc,char *argv[])
{
  tokenlist args;
  string maskfile;
  vector<string> filelist;

  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-m" && i<args.size()-1)
      maskfile=args[++i];
    else if (args[i]=="-h") {
      vbcmp_help();
      exit(0);
    }
    else if (args[i]=="-v") {
      vbcmp_version();
      exit(0);
    }
    else
      filelist.push_back(args[i]);
  }

  if (filelist.size() < 2) {
    vbcmp_help();
    exit(0);
  }
  Cube mask;
  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      cout << format("[E] vbcmp: error reading mask file %s\n")%maskfile;
      exit(2);
    }
  }

  string spacer;
  if (filelist.size()>2) spacer="   ";
  for (size_t i=0; i<filelist.size(); i++) {
    for (size_t j=0; j<filelist.size(); j++) {
      if (i>=j) continue;
      if (filelist.size()>2)
        cout << format("[I] vbcmp: comparing %s and %s\n")%filelist[i]%filelist[j];
      int err1,err2;
      Cube cb1,cb2;
      err1=cb1.ReadFile(filelist[i]);
      err2=cb2.ReadFile(filelist[j]);
      if (!err1 && !err2) {
        cmp3d(cb1,cb2,mask,spacer);
        continue;
      }
      Tes ts1,ts2;
      err1=ts1.ReadFile(filelist[i]);
      err2=ts2.ReadFile(filelist[j]); 
      if (!err1 && !err2) {
        cmp4d(ts1,ts2,mask,spacer);
        continue;
      }
      // try vectors before matrices, because all vectors are matrices
      VB_Vector v1,v2;
      err1=v1.ReadFile(filelist[i]);
      err2=v2.ReadFile(filelist[j]);
      if (!err1 && !err2) {
        cmp1d(v1,v2,spacer);
        continue;
      }
      VBMatrix m1,m2;
      err1=m1.ReadFile(filelist[i]);
      err2=m2.ReadFile(filelist[j]);
      if (!err1 && !err2) {
        cmp2d(m1,m2,spacer);
        continue;
      }
      cout << format("[E] vbcmp: couldn't read %s and %s as same type\n")%filelist[i]%filelist[j];
    }
  }
  exit(100);
}

void
cmp1d(VB_Vector &v1,VB_Vector &v2,string spacer)
{
  if (v1.size()!=v2.size()) {
    printf("[E] vbcmp: %smatrices have different row count\n",spacer.c_str());
    return;
  }

  int diffs_all=0;
  double totals_all=0.0,max_all=0.0;
  double diff;
  for (uint32 i=0; i<v1.size(); i++) {
    diff=fabs(v1(i)-v2(i));
    if (diff==0.0) continue;
    diffs_all++;
    totals_all+=diff;
    if (diff>max_all) max_all=diff;
  }
  if (diffs_all)
    totals_all/=(double)diffs_all;
  if (diffs_all==0)
    printf("[I] vbcmp: %svectors are identical\n",spacer.c_str());
  else {
    printf("[I] vbcmp: %s%d total elements\n",spacer.c_str(),(int)v1.size());
    printf("[I] vbcmp: %s   total: %d different elements, mean abs diff %g, max diff %g\n",spacer.c_str(),diffs_all,totals_all,max_all);
    printf("[I] vbcmp: %s   correlation between vectors (Pearson's r) is: %g\n",spacer.c_str(),correlation(v1,v2));
  }
}

// the below should be kept in sync with similar code in vbmm2.cpp

void
cmp2d(VBMatrix &mat1,VBMatrix &mat2,string spacer)
{
  if (mat1.m!=mat2.m) {
    printf("[E] vbcmp: %smatrices have different row count\n",spacer.c_str());
    return;
  }
  if (mat1.n!=mat2.n) {
    printf("[E] vbcmp: %smatrices have different column count\n",spacer.c_str());
    return;
  }
  int diffs_all=0,diffs_diag=0,diffs_off=0;
  double totals_all=0.0,totals_diag=0.0,totals_off=0.0;
  double max_all=0.0,max_diag=0.0,max_off=0.0;
  double diff;
  
  for (uint32 i=0; i<mat1.m; i++) {
    for (uint32 j=0; j<mat1.n; j++) {
      diff=fabs(mat1(i,j)-mat2(i,j));
      if (diff==0.0) continue;
      diffs_all++;
      totals_all+=diff;
      if (diff>max_all) max_all=diff;
      if (i==j) {
        diffs_diag++;
        totals_diag+=diff;
        if (diff>max_diag) max_diag=diff;
      }
      else {
        diffs_off++;
        totals_off+=diff;
        if (diff>max_off) max_off=diff;
      }
    }
  }
  if (diffs_all)
    totals_all/=(double)diffs_all;
  if (diffs_diag)
    totals_diag/=(double)diffs_diag;
  if (diffs_off)
    totals_off/=(double)diffs_off;
  if (diffs_all==0)
    printf("[I] vbcmp: %smatrices are identical\n",spacer.c_str());
  else {
    printf("[I] vbcmp: %sout of %d total cells:\n",spacer.c_str(),mat1.m*mat1.n);
    printf("[I] vbcmp: %s   total: %d different cells, mean abs diff %g, max diff %g\n",spacer.c_str(),diffs_all,totals_all,max_all);
    if (mat1.m==mat1.n) {
      printf("[I] vbcmp: %sdiagonal: %d different cells, mean abs diff %g, max diff %g\n",spacer.c_str(),diffs_diag,totals_diag,max_diag);
      printf("[I] vbcmp: %soff-diag: %d different cells, mean abs diff %g, max diff %g\n",spacer.c_str(),diffs_off,totals_off,max_off);
    }
  }
}

void
cmp3d(Cube &c1,Cube &c2,Cube &mask,string spacer)
{
  if (!(c1.dimsequal(c2))) {
    printf("[I] vbcmp: %sdimensions don't match, can't compare.\n",spacer.c_str());
    return;
  }
  if (mask.data && !(c1.dimsequal(mask))) {
    printf("[I] vbcmp: %smask dimensions don't agree with images\n",spacer.c_str());
    return;
  }
  double totaldiff,maxdiff=0.0,v1,v2,diff;
  uint32 diffcount,voxelcount;
  int i,j,k;

  diffcount=0;
  totaldiff=0.0;
  int maxx=0,maxy=0,maxz=0;
  voxelcount=c1.dimx * c1.dimy * c1.dimz;
  // pre-count mask
  if (mask.data) {
    int newvoxelcount=0;
    for (size_t i=0; i<voxelcount; i++)
      if (mask.testValue(i))
        newvoxelcount++;
    voxelcount=newvoxelcount;
  }
  VB_Vector vec1(voxelcount),vec2(voxelcount);
  uint64 ind=0;
  for (i=0; i<c1.dimx; i++) {
    for (j=0; j<c1.dimy; j++) {
      for (k=0; k<c1.dimz; k++) {
        if (mask.data)
          if (!(mask.testValue(i,j,k))) continue;
        v1=c1.GetValue(i,j,k);
        v2=c2.GetValue(i,j,k);
        // to support correlation, smush into vectors:
        vec1[ind]=v1; vec2[ind]=v2;  ind++;        
        if (v1 != v2) {
          if (isnan(v1) && isnan(v2)) continue;
          if (isinf(v1) && isinf(v2)) continue;
          diffcount++;
          diff=fabs(v1-v2);
          totaldiff += diff;
          if (diff>maxdiff) {
            maxdiff=diff;
            maxx=i;
            maxy=j;
            maxz=k;
          }
        }
      }
    }
  }
  if (!diffcount) {
    printf("[I] vbcmp: %sthe data are identical\n",spacer.c_str());
  }
  else {
    cout << format("[I] vbcmp: %sdifferent voxels: %ld of %ld (%.0f%%)\n")
      % spacer % diffcount % voxelcount % ((diffcount*100.0)/voxelcount);
    cout << format("[I] vbcmp: %smean difference: %.8f\n") % spacer % (totaldiff/diffcount);
    cout << format("[I] vbcmp: %smax difference: %.8f (%d,%d,%d)\n")
      % spacer % maxdiff % maxx % maxy % maxz;
    cout << format("[I] vbcmp: %scorrelation between images: %g\n")%spacer%correlation(vec1,vec2);
  }
}

void
cmp4d(Tes &t1,Tes &t2,Cube &mask,string spacer)
{
  if (!(t1.dimsequal(t2))) {
    cout << format("[I] vbcmp: %sdimensions don't match, can't compare.\n")%spacer;
    return;
  }
  if (mask.data && !(t1.dimsequal(mask))) {
    cout << format("[I] vbcmp: %smask dimensions don't agree with images\n")%spacer;
    return;
  }
  double totaldiff,maxdiff=0.0,v1,v2,diff;
  long diffcount,voxelcount;
  int i,j,k,m,voldiff;
  int differingvolumes=0;

  diffcount=0;
  totaldiff=0.0;
  voxelcount=0;
  for (m=0; m<t1.dimt; m++) {
    voldiff=0;
    for (i=0; i<t1.dimx; i++) {
      for (j=0; j<t1.dimy; j++) {
        for (k=0; k<t1.dimz; k++) {
          if (mask.data)
            if (!(mask.testValue(i,j,k)))
              continue;
          if (m==0) voxelcount++;
          v1=t1.GetValue(i,j,k,m);
          v2=t2.GetValue(i,j,k,m);
          if (v1 != v2) {
            voldiff=1;
            diffcount++;
            diff=fabs(v1-v2);
            totaldiff += diff;
            if (diff>maxdiff) maxdiff=diff;
          }
        }
      }
    }
    if (voldiff)
      differingvolumes++;
  }
  if (!diffcount) {
    printf("[I] vbcmp: %sthe data are identical.\n",spacer.c_str());
  }
  else {
    printf("[I] vbcmp: %sdifferent voxels: %ld of %ld (%.0f%%)\n",spacer.c_str(),diffcount,voxelcount,
	   (double)(diffcount*100.0)/voxelcount);
    printf("[I] vbcmp: %smean difference: %.8f\n",spacer.c_str(),totaldiff/diffcount);
    printf("[I] vbcmp: %smax difference: %.8f\n",spacer.c_str(),maxdiff);
    printf("[I] vbcmp: %s%d of %d volumes differ\n",spacer.c_str(),differingvolumes,t1.dimt);
  }
}

void
vbcmp_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbcmp_version()
{
  printf("VoxBo vbcmp (v%s)\n",vbversion.c_str());
}
