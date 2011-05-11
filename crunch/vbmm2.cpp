
// vbmm2.cpp
// VoxBo matrix multiplication
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

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <string>
#include "vbutil.h"
#include "vbio.h"

void do_print(tokenlist &args);
void do_printsub(tokenlist &args);
void do_compare(tokenlist &args);
int do_ident(tokenlist &args);
int do_zeros(tokenlist &args);
int do_random(tokenlist &args);
int do_invert(tokenlist &args);
int do_add(tokenlist &args);
int do_subtract(tokenlist &args);
int do_xyt(tokenlist &args);
int do_xy(tokenlist &args);
int do_imxy(tokenlist &args);
int do_f3(tokenlist &args);
int do_pinv(tokenlist &args);
int do_pca(tokenlist &args);
int do_assemblecols(tokenlist &args);
int do_assemblerows(tokenlist &args);
int do_xyz(tokenlist &args);
int do_reshape(tokenlist &args);
// vector stuff
int do_applyfilter(tokenlist &args);

void vbmm_help();

int
main(int argc,char *argv[])
{
  tokenlist args;
  int err=0;
  string cmd;

  args.Transfer(argc-1,argv+1);
  if (args.size()<1) {
    vbmm_help();
    exit(0);
  }
  
  cmd=args[0];
  args.DeleteFirst();

  if (cmd=="-xy")
    err=do_xy(args);
  else if (cmd=="-imxy")
    err=do_imxy(args);
  else if (cmd=="-f3")
    err=do_f3(args);
  else if (cmd=="-pinv")
    err=do_pinv(args);
  else if (cmd=="-pca")
    err=do_pca(args);
  else if (cmd=="-xyz")
    err=do_xyz(args);
  else if (cmd=="-xyt")
    err=do_xyt(args);
  else if (cmd=="-add")
    err=do_add(args);
  else if (cmd=="-subtract")
    err=do_subtract(args);
  else if (cmd=="-invert")
    err=do_invert(args);
  else if (cmd=="-ident")
    err=do_ident(args);
  else if (cmd=="-zeros")
    err=do_zeros(args);
  else if (cmd=="-assemblecols")
    err=do_assemblecols(args);
  else if (cmd=="-assemblerows")
    err=do_assemblerows(args);
  else if (cmd=="-reshape")
    err=do_reshape(args);
  else if (cmd=="-random")
    err=do_random(args);
  else if (cmd=="-print")
    do_print(args);
  else if (cmd=="-printsub")
    do_printsub(args);
  else if (cmd=="-compare")
    do_compare(args);
  else if (cmd=="-applyfilter")
    do_applyfilter(args);
  else {
    printf("[E] vbmm2: unknown operation %s",cmd.c_str());
    exit(10);
  }

  exit(err);
}

// ident args: name size

int
do_ident(tokenlist &args)
{
  int r;

  if (args.size() < 2) {
    printf("[E] vbmm2: need a name and a size to create a matrix.\n");
    return 5;
  }

  r = strtol(args[1]);
  if (r < 0) {
    printf("[E] vbmm2: size for identity matrix must be > 0.\n");
    return 10;
  }

  printf("[I] vbmm2: creating identity matrix %s of size %d.\n",args[0].c_str(),r);

  VBMatrix target(r,r);
  target.ident();
  if (target.WriteFile(args[0]))
    printf("[E] vbmm2: identity matrix %s (%dx%d) not created.\n",args[0].c_str(),r,r);
  else
    printf("[I] vbmm2: identity matrix %s (%dx%d) created.\n",args[0].c_str(),r,r);
  return 0;
}

// zeros args: name size

int
do_zeros(tokenlist &args)
{
  if (args.size() < 2) {
    printf("[E] vbmm2: need a name and a size to create a matrix.\n");
    return 5;
  }

  int r,c;
  c = r = strtol(args[1]);
  if (args.size() > 2)         // needed only if it's not square
    c = strtol(args[2]);
  if (r < 0 || c < 0) {
    printf("[E] vbmm2: dimensions for zero matrix must be > 0.\n");
    return 10;
  }

  printf("[I] vbmm2: creating %dx%d zero matrix %s.\n",r,c,args[0].c_str());

  VBMatrix target(c,r);
  target.zero();
  if (target.WriteFile(args[0]))
    printf("[E] vbmm2: zero matrix %s (%dx%d) not created.\n",args[0].c_str(),r,c);
  else
    printf("[I] vbmm2: zero matrix %s (%dx%d) created.\n",args[0].c_str(),r,c);
  return 0;
}

int
do_random(tokenlist &args)
{
  if (args.size() < 2) {
    printf("[E] vbmm2: need a name and a size to create a matrix.\n");
    return 5;
  }
  int r,c;
  c = r = strtol(args[1]);
  if (args.size() > 2)         // needed only if it's not square
    c = strtol(args[2]);
  if (r < 0 || c < 0) {
    printf("[E] vbmm2: dimensions for random matrix must be > 0.\n");
    return 10;
  }

  printf("vbmm2: creating random %dx%d matrix %s.\n",r,c,args[0].c_str());

  VBMatrix target(r,c);
  target.random();
  if (target.WriteFile(args[0]))
    printf("[E] vbmm2: error writing random %dx%d matrix %s\n",r,c,args[0].c_str());
  else
    printf("[I] vbmm2: random %dx%d matrix %s created\n",r,c,args[0].c_str());
  return 0;
}

// args: in1 in2 out

int
do_subtract(tokenlist &args)
{
  if (args.size() != 3) {
    printf("[E] vbmm2: usage: vbmm2 -subtract in1 in2 out\n");
    return 5;
  }

  VBMatrix mat1(args[0]);
  VBMatrix mat2(args[1]);

  if (mat1.m == 0 || mat1.n == 0) {
    printf("[E] vbmm2: first matrix was bad.\n");
    return 101;
  }
  if (mat2.m == 0 || mat2.n == 0) {
    printf("[E] vbmm2: second matrix was bad.\n");
    return 102;
  }
  if (mat1.m != mat2.m || mat1.n != mat2.n) {
    fprintf(stderr,"[E] vbmm2: matrix dimensions don't match.\n");
    return 103;
  }
  printf("[I] vbmm2: subtracting matrix %s from matrix %s.\n",args[1].c_str(),args[0].c_str());
  mat1-=mat2;
  mat1.WriteFile(args[2]);
  printf("[I] vbmm2: done.\n");

  return 0;
}

int
do_add(tokenlist &args)
{
  if (args.size() != 3) {
    printf("[E] vbmm2: usage: vbmms -add in1 in2 out\n");
    return 5;
  }

  VBMatrix mat1(args[0]);
  VBMatrix mat2(args[1]);

  if (mat1.m == 0 || mat1.n == 0) {
    printf("[E] vbmm2: first matrix was bad.\n");
    return 101;
  }
  if (mat2.m == 0 || mat2.n == 0) {
    printf("[E] vbmm2: second matrix was bad.\n");
    return 102;
  }
  if (mat1.m != mat2.m || mat1.n != mat2.n) {
    fprintf(stderr,"[E] vbmm2: matrix dimensions don't match.\n");
    return 103;
  }

  
  printf("[I] vbmm2: adding matrix %s and matrix %s.\n",args[0].c_str(),args[1].c_str());
  mat1+=mat2;
  mat1.WriteFile(args[2]);
  printf("[I] vbmm2: done.\n");

  return 0;
}

// the below should be kept in sync with similar code in vbcmp.cpp

void
do_compare(tokenlist &args)
{
  if (args.size()!=2) {
    printf("[E] vbmm2: usage: vbmm2 -compare <mat1> <mat2>\n");
    return;
  }
  VBMatrix mat1(args[0]);
  VBMatrix mat2(args[1]);
  if (mat1.m!=mat2.m) {
    printf("[E] vbmm2: matrices have different row count\n");
    return;
  }
  if (mat1.n!=mat2.n) {
    printf("[E] vbmm2: matrices have different column count\n");
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
    printf("[I] vbmm2: matrices are identical\n");
  else {
    printf("[I] vbmm2: %d total cells\n",mat1.m*mat1.n);
    printf("[I] vbmm2:    total: %d different cells, mean abs diff %g, max diff %g\n",diffs_all,totals_all,max_all);
    printf("[I] vbmm2: diagonal: %d different cells, mean abs diff %g, max diff %g\n",diffs_diag,totals_diag,max_diag);
    printf("[I] vbmm2: off-diag: %d different cells, mean abs diff %g, max diff %g\n",diffs_off,totals_off,max_off);
  }
}

// xy args: in1 in2 out [col1 col2]

int
do_xy(tokenlist &args)
{
  int c1,c2;
  
  if (args.size() != 3 && args.size() != 5) {
    printf("[E] vbmm2: usage: vbmm2 -xy in1 in2 out [c1 c2]\n");
    return 5;
  }
  VBMatrix mat1,mat2;
  mat1.ReadHeader(args[0]);
  mat2.ReadHeader(args[1]);
  if (mat1.m==0||mat1.n==0||mat2.m==0||mat2.n==0) {
    printf("[E] vbmm2: couldn't read matrix headers\n");
    return 100;
  }
  if (mat1.ReadFile(args[0])) {
    printf("[E] vbmm2: first matrix was bad.\n");
    return 100;
  }

  if (args.size()==5) {
    c1 = strtol(args[3]);
    c2 = strtol(args[4]);
    if (c1<0 || c2<0) {
      printf("[E] vbmm2: invalid column numbers\n");
      return 107;
    }
  }
  else {
    c1=0;
    c2=mat2.cols-1;
  }

  // figure out the outfile name for this part, if we're not doing the whole thing
  string outname=args[2];
  char tmps[128];
  if (c1!=0 || (uint32)c2!=mat2.cols-1) {
    sprintf(tmps,"_%08d_%08d",c1,c2);
    outname += tmps;
  }

  if (mat2.ReadFile(args[1],0,0,c1,c2)) {
    printf("[E] vbmm2: second matrix was bad.\n");
    return 101;
  }

  printf("[I] vbmm2: x=%s\n",args(0));
  printf("[I] vbmm2: y=%s\n",args(1));
  printf("[I] vbmm2: multiplying x by y (cols %d to %d).\n",c1,c2);
  mat1*=mat2;
  if (mat1.WriteFile(outname))
    printf("[E] vbmm2: failed!\n");
  else
    printf("[I] vbmm2: done.\n");

  return 0;
}

int
do_xyt(tokenlist &args)
{
  int c1,c2;
  
  if (args.size() != 3 && args.size() != 5) {
    printf("[E] vbmm2: usage: vbmm2 -xy in1 in2 out [c1 c2]\n");
    return 5;
  }
  
  VBMatrix mat1,mat2;
  mat1.ReadHeader(args[0]);
  mat2.ReadHeader(args[1]);
  if (mat1.m==0||mat1.n==0||mat2.m==0||mat2.n==0) {
    printf("[E] vbmm2: couldn't read matrix headers\n");
    return 100;
  }

  if (mat1.ReadFile(args[0])) {
    printf("[E] vbmm2: first matrix was bad.\n");
    return 100;
  }
  if (args.size()==5) {
    c1 = strtol(args[3]);
    c2 = strtol(args[4]);
  }
  else {
    c1 = 0;
    c2 = mat2.rows-1;
  }

  // figure out the outfile name
  string outname=args[2];
  char tmps[128];
  if (c1!=0 || (uint32)c2!=mat2.rows-1) {
    sprintf(tmps,"_%05d_%05d",c1,c2);
    outname += tmps;
  }

  if (mat2.ReadFile(args[1],c1,c2,0,0)) {
    printf("[E] vbmm2: second matrix was bad\n");
    return 101;
  }

  printf("[I] vbmm2: x=%s\n",args(0));
  printf("[I] vbmm2: y=%s\n",args(1));
  printf("[I] vbmm2: multiplying x by y (cols %d to %d).\n",c1,c2);
  mat2.transposed=1;
  mat1*=mat2;
  if (mat1.WriteFile(outname))
    printf("[E] vbmm2: failed\n");
  else
    printf("[I] vbmm2: done\n");

  return 0;
}

int
do_xyz(tokenlist &args)
{
  if (args.size() != 4) {
    printf("vbmm2: usage: vbmm2 -xyz in1 in2 in3 out\n");
    return 5;
  }
  
  VBMatrix mat1(args[0]);
  VBMatrix mat2(args[1]);

  if (!(mat1.valid() && mat2.valid())) {
    printf("[E] vbmm2: bad input matrix\n");
    return (100);
  }
  if (mat1.n != mat2.m) {
    printf("[E] vbmm2: bad matrix dimensions for xyz.\n");
    return (104);
  }

  printf("vbmm2: multiplying matrix %s by matrix %s.\n",args(0),args(1));
  mat1*=mat2;
  printf("vbmm2: multiplying matrix %s by matrix %s.\n",args(1),args(2));
  VBMatrix mat3(args[2]);
  if (mat1.n != mat3.m) {
    printf("[E] vbmm2: bad matrix dimensions for xyz.\n");
    return (104);
  }
  mat1*=mat3;
  printf("vbmm2: done.\n");

  mat1.WriteFile(args[3]);

  return 0;
}

int
do_reshape(tokenlist &args)
{
  if (args.size()!=4) {
    printf("[E] vbmm2: reshape takes four arguments\n");
    return (101);
  }
  string dir=vb_tolower(args[3]);
  if (dir!="rows" && dir!="cols") {
    printf("[E] vbmm2: the last argument must be either rows or cols\n");
    return (101);
  }
  int dim=strtol(args[2]);
  if (dim<1) {
    printf("[E] vbmm2: provided dimension must be positive\n");
    return (101);
  }
  VB_Vector invec(args[0]);
  if (!invec.size()) { 
    printf("[E] vbmm2: couldn't read input vector file %s\n",args(0));
    return (101);
  }
  VBMatrix outmat;
  int otherdim=invec.size()/dim;
  if (invec.size()%dim) otherdim++;
  if (dir=="rows") {
    outmat.resize(dim,otherdim);
    outmat.zero();
    uint32 ind=0;
    for (uint32 cc=0; cc<outmat.cols; cc++) {
      for (uint32 rr=0; rr<outmat.rows; rr++) {
        if (ind<invec.size())
          outmat.set(rr,cc,invec(ind++));
      }
    }
  }
  if (dir=="cols") {
    outmat.resize(otherdim,dim);
    outmat.zero();
    uint32 ind=0;
    for (uint32 rr=0; rr<outmat.rows; rr++) {
      for (uint32 cc=0; cc<outmat.cols; cc++) {
        if (ind<invec.size())
          outmat.set(rr,cc,invec(ind++));
      }
    }
  }
  if (outmat.WriteFile(args[1])) {
    printf("[E] vbmm2: error writign output file %s\n",args(2));
    return (101);
  }
  printf("[I] vbmm2: wrote %s\n",args(1));
  return 0;  
}

int
do_assemblerows(tokenlist &args)
{
  vglob vg;
  string outfile;
  if (args.size()>1) {
    for (size_t i=0; i<args.size()-1; i++)
      vg.append(args[i]);
    outfile=args[args.size()-1];
  }
  else {
    vg.append(args[0]+"_*_*");
    outfile=args[0];
  }
  if (vg.size() < 1) {
    printf("[E] vbmm2: no parts found for %s\n",outfile.c_str());
    return (101);
  }
  vector<VBMatrix *> mats;
  uint32 rows=0,cols=0;
  // first read all the headers
  for (size_t i=0; i<vg.size(); i++) {
    VBMatrix tmp;
    tmp.ReadHeader(vg[i]);
    if (!tmp.headerValid()) {
      printf("[E] vbmm2: invalid matrix in assemble list\n");
      return (102);
    }
    if (cols==0)
      cols=tmp.n;
    rows+=tmp.m;
    if (cols != tmp.n) {
      printf("[E] vbmm2: wrong-sized matrix %s in assemble list\n",vg[i].c_str());
      return (103);
    }
  }
  if (rows < 1 || cols < 1) {
    printf("[E] vbmm2: invalid size for assembled matrix: %d x %d\n",rows,cols);
    return (103);
  }
  VBMatrix newmat(rows,cols);
  
  uint32 ind=0;
  for (size_t i=0; i<vg.size(); i++) {
    VBMatrix tmp(vg[i]);
    for (uint32 j=0; j<tmp.m; j++) {
      VB_Vector vv=tmp.GetRow(j);
      newmat.SetRow(ind,vv);
      ind++;
    }
  }
  // try to unlink the actual files now that we're merged
  if (args.size()==1)
    for (size_t i=0; i<vg.size(); i++)
      unlink(vg[i].c_str());
  if (newmat.WriteFile(outfile)) {
    printf("[E] vbmm2: error writing %s\n",outfile.c_str());
    return 1;
  }
  else {
    printf("[I] vbmm2: assembled %d files into %s\n",(int)vg.size(),outfile.c_str());
    return 0;
  }
  return (0);  // no error!
}

int
do_assemblecols(tokenlist &args)
{
  vglob vg;
  string outfile;
  if (args.size()>1) {
    for (size_t i=0; i<args.size()-1; i++)
      vg.append(args[i]);
    outfile=args[args.size()-1];
  }
  else {
    vg.append(args[0]+"_*_*");
    outfile=args[0];
  }
  if (vg.size() < 1) {
    printf("[E] vbmm2: no parts found for %s\n",outfile.c_str());
    return (101);
  }
  vector<VBMatrix *> mats;
  uint32 rows=0,cols=0;
  // first read all the headers
  for (size_t i=0; i<vg.size(); i++) {
    VBMatrix tmp;
    tmp.ReadHeader(vg[i]);
    if (!tmp.headerValid()) {
      printf("[E] vbmm2: invalid matrix in assemble list\n");
      return (102);
    }
    if (rows==0)
      rows=tmp.m;
    cols+=tmp.n;
    if (rows != tmp.m) {
      printf("[E] vbmm2: wrong-sized matrix %s in assemble list\n",vg[i].c_str());
      return (103);
    }
  }
  if (rows < 1 || cols < 1) {
    printf("[E] vbmm2: invalid size for assembled matrix: %d x %d\n",rows,cols);
    return (103);
  }
  VBMatrix newmat(rows,cols);
  
  int ind=0;
  for (uint32 i=0; i<vg.size(); i++) {
    VBMatrix tmp(vg[i]);
    for (uint32 j=0; j<tmp.n; j++) {
      VB_Vector vv=tmp.GetColumn(j);
      newmat.SetColumn(ind,vv);
      ind++;
    }
  }
  // try to unlink the actual files now that we're merged
  if (args.size()==1)
    for (size_t i=0; i<vg.size(); i++)
      unlink(vg[i].c_str());
  if (newmat.WriteFile(outfile)) {
    printf("[E] vbmm2: error writing %s\n",outfile.c_str());
    return 1;
  }
  else {
    printf("[I] vbmm2: assembled %d files into %s\n",(int)vg.size(),outfile.c_str());
    return 0;
  }
  return (0);  // no error!
}

int
do_imxy(tokenlist &args)
{
  if (args.size() != 3 && args.size() != 5) {
    printf("[E] vbmm2: usage: vbmm -imxy in1 in2 out\n");
    return (100);
  }
  
  VBMatrix mat1(args[0]);
  VBMatrix mat2(args[1]);

  if (mat1.m <= 0 || mat1.n <= 0) {
    printf("[E] vbmm2: first matrix was bad.\n");
    return (101);
  }
  if (mat2.m == 0 || mat2.n == 0) {
    printf("[E] vbmm2: second matrix was bad.\n");
    return (102);
  }
  if (mat1.n != mat2.m) {
    printf("[E] vbmm2: incompatible matrix dimensions for I-XY.\n");
    return (103);
  }

  printf("[I] vbmm2: x=%s\n",args(0));
  printf("[I] vbmm2: y=%s\n",args(1));
  printf("[I] vbmm2: calcualting I-XY\n");
  mat1*=mat2;
  for (uint32 i=0; i<mat1.m; i++) {
    VB_Vector tmp=mat1.GetRow(i);
    for (uint32 j=0; j<mat1.n; j++) {
      tmp[j]*=(double)-1.0;
      if (i==j)
        tmp[j]+=(double)1.0;
    }
    mat1.SetRow(i,tmp);
  }
  printf("[I] vbmm2: done.\n");
  mat1.WriteFile(args[2]);
  return 0;
}

int
do_f3(tokenlist &args)
{
  if (args.size() != 3) {
    printf("[E] vbmm2: usage: vbmm -f3 v kg out\n");
    return (100);
  }
  printf("[I] vbmm2: creating F3 matrix (V*KG*invert(KGtKG))\n");
  printf("[I] vbmm2: V: %s\n",args(0));
  printf("[I] vbmm2: KG: %s\n",args(1));
  VBMatrix v(args[0]);
  VBMatrix kg(args[1]);
  VBMatrix kgt=kg;
  VBMatrix tmp;

  if (v.m==0 || kg.m==0 || kgt.m==0) {
    printf("[E] vbmm2: couldn't read matrices\n");
    return 100;
  }
  if (v.n != kg.m) {
    printf("[E] vbmm2: incompatible matrix dimensions\n");
    return 102;
  }

  kgt.transposed=1;
  kgt*=kg;
  kgt.transposed=0;
  if (invert(kgt,tmp)) {
    printf("[E] vbmm2: failed to invert KGt (singular matrix)\n");
    return 103;
  }
  kgt.clear();  // free mem
  v*=kg;
  kg.clear();   // free mem
  v*=tmp;
  v.WriteFile(args[2]);
  printf("[I] vbmm2: wrote F3 matrix %s\n",args(2));

  return 0;
}

int
do_invert(tokenlist &args)
{
  if (args.size() != 2) {
    printf("[E] vbmm2: usage: vbmm -invert in out\n");
    return (100);
  }
  
  VBMatrix mat(args[0]);

  if (mat.m <= 0 || mat.n <= 0 || mat.m != mat.n) {
    printf("[E] vbmm2: input matrix for invert was bad.\n");
    return (101);
  }
  VBMatrix target(mat.m,mat.m);
  printf("vbmm2: inverting matrix %s.\n",args[0].c_str());
  if (invert(mat,target)) {
    printf("[E] vbmm2: failed (singular matrix\n");
    return 1;
  }
  else {
    target.WriteFile(args[1]);
    printf("vbmm2: done.\n");
  }
  return 0;
}

// do_pinv() computes the pseudo-inverse, which is
// inverse(KGtKG) ## KGt

int
do_pinv(tokenlist &args)
{
  if (args.size() != 2) {
    printf("[E] vbmm2: usage: vbmm -pinv in out\n");
    return (100);
  }
  
  VBMatrix mat(args[0]);

  if (mat.m <= 0 || mat.n <= 0) {
    printf("[E] vbmm2: input matrix for pinv was bad.\n");
    return (101);
  }

  VBMatrix target(mat.n,mat.m);
  printf("[I] vbmm2: pinv'ing matrix %s.\n",args[0].c_str());
  pinv(mat,target);
  printf("[I] vbmm2: done.\n");
  target.WriteFile(args[1]);
  return 0;
}

// do_pca() calculates the principle components

int
do_pca(tokenlist &args)
{
  if (args.size() != 2) {
    printf("[E] vbmm2: usage: vbmm -pca in out\n");
    return (100);
  }
  
  VBMatrix mat(args[0]);

  if (mat.m <= 0 || mat.n <= 0) {
    printf("[E] vbmm2: input matrix for pinv was bad.\n");
    return (101);
  }

  VB_Vector lambdas;
  VBMatrix pcs;
  VBMatrix E;
  printf("vbmm2: pca'ing matrix %s.\n",args[0].c_str());
  pca(mat,lambdas,pcs,E);
  lambdas.print();
  printf("vbmm2: done.\n");
  pcs.WriteFile(args[1]);
  return 0;
}

void
do_print(tokenlist &args)
{
  if (args.size() < 1)
    return;
  for (size_t i=0; i<args.size(); i++) {
    VBMatrix tmp(args[i]);
    if (tmp.valid())
      tmp.print();
    else {
      printf("[E] vbmm2: couldn't open matrix %s to print.\n",args(i));
    }
  }
}

void
do_printsub(tokenlist &args)
{
  if (args.size() != 5)
    return;
  int r1=strtol(args[1]);
  int r2=strtol(args[2]);
  int c1=strtol(args[3]);
  int c2=strtol(args[4]);
  VBMatrix mat(args[0],r1,r2,c1,c2);
  if (!mat.rowdata) {
    printf("[E] vbmm2: couldn't read data\n");
    return;
  }
  printf("[I] vbmm: requested rows %d-%d and cols %d-%d of %s\n",
         r1,r2,c1,c2,args(0));
  mat.print();
}

int
do_applyfilter(tokenlist &args)
{
  if (args.size()!=3) {
    cout << "[E] vbmm2: applyfilter takes 3 arguments\n";
    exit(5);
  }
  VB_Vector signal,exofilt;
  signal.ReadFile(args[0]);
  exofilt.ReadFile(args[1]);
  uint32 len=signal.size();
  if (len<1 || exofilt.size()!=len) {
    cout << "[E] vbmm2: problem reading files or non-matching lengths\n";
    exit(5);
  }
  VB_Vector rsig(len),isig(len);
  VB_Vector rfilt(len),ifilt(len);
  signal.fft(rsig,isig);
  exofilt.fft(rfilt,ifilt);
  rfilt[0]=1.0;
  ifilt[0]=0.0;
  VB_Vector rprod(len),iprod(len),kx(len);
  VB_Vector::compMult(rsig,isig,rfilt,ifilt,rprod,iprod);
  VB_Vector::complexIFFTReal(rprod,iprod,kx);
  if (kx.WriteFile(args[2])) {
    cout << "[E] vbmm2: error writing " << args[2] << endl;
    exit(5);
  }
  cout << "[I] vbmm2: wrote filtered vector " << args[2] << endl;
  exit(0);

}

void
vbmm_help()
{
  printf("\nVoxBo vbmm2 (v%s)\n",vbversion.c_str());
  printf("summary:\n");
  printf("  vbmm does various bits of matrix arithmetic with matrix files.\n");
  printf("  below, in and out refer to the input and output matrices.  c1 and c2\n");
  printf("  refer to the start and end columns to be produced (for parallelization).\n");
  printf("usage:\n");
  printf("  vbmm2 -xyt <in1> <in2> <out> <c1> <c2 >     do part of XYt\n");
  printf("  vbmm2 -xy <in1> <in2> <out> <c1> <c2>       do part of XY\n");
  printf("  vbmm2 -imxy <in1> <in2> <out>               I-XY in core\n");
  printf("  vbmm2 -f3 <v> <kg> <out>                    V*KG*invert(KTtKG)\n");
  printf("  vbmm2 -xyz <in1> <in2> <in3> <out>          XYZ in core\n");
  printf("  vbmm2 -assemblecols <out>                   assemble out from available parts\n");
  printf("  vbmm2 -assemblecols <in1> <in2>... <out>    assemble all in files into out\n");
  printf("  vbmm2 -assemblerows <out>                   assemble out from available parts\n");
  printf("  vbmm2 -assemblerows <in1> <in2>... <out>    assemble all in files into out\n");
  printf("  vbmm2 -add <in1> <in2> <out>                add two matrices\n");
  printf("  vbmm2 -subtract <in1> <in2> <out>           subtract in2 from in1\n");
  printf("  vbmm2 -invert <in> <out>                    invert in\n"); 
  printf("  vbmm2 -pinv <in> <out>                      pseudo-inverse, in core\n");
  printf("  vbmm2 -pca <in> <out>                       calculte pca\n"); 
  printf("  vbmm2 -ident <name> <size>                  create an identity matrix\n");
  printf("  vbmm2 -zeros <name> <cols> [rows]           create a zero matrix\n");
  printf("  vbmm2 -random <name> <cols> [rows]          create a matrix of random numbers\n");
  printf("  vbmm2 -print <name>                         display a matrix\n");
  printf("  vbmm2 -printsub <name> <r1> <r2> <c1> <c2>  display part of a matrix\n");
  printf("  vbmm2 -compare <mat1> <mat2>                compare two matrices\n");
  printf("  vbmm2 -reshape <in> <out> <n> <rows/cols>   reshape an input vector into a matrix\n");
  printf("  vbmm2 -applyfilter <in> <filter> <out>      apply \"ExoFilt\" filter to vector\n");
  printf("notes:\n");
  printf("  -assemblerows and -assemblecols, given a single argument, will try\n");
  printf("  to find the pieces for that file, and will delete those pieces when\n");
  printf("  done.  Given more than one argument, all but the last argument will be\n");
  printf("  taken as input filenames, and the last will be used for the output.\n");
  printf("\n");
  printf("  -reshape allows you to fix the number of rows or columsn.  if you fix\n");
  printf("  the rows, the vector is laid out column-by-column.  if you fix the cols,\n");
  printf("  the vector is paid out row-by-row.\n");
  printf("\n");
}
