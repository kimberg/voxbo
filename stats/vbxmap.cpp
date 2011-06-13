
// vbcmap.cpp
// calculate a chi-squared test or fisher exact in each voxel
// Copyright (c) 2011 by The VoxBo Development Team

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

using namespace std;

#include <stdio.h>
#include <string.h>
#include "vbutil.h"
#include "vbio.h"
#include "stats.h"
#include "makestatcub.h"
#include "vbxmap.hlp.h"

void vbcmap_help();
void vbcmap_version();

int
main(int argc,char *argv[])
{
  if (argc<2) {
    vbcmap_help();
    exit(0);
  }

  tokenlist args;
  vector<string>filelist;
  string dvname,ivname,outfile,maskfile,pfile,pmapname;
  args.Transfer(argc-1,argv+1);
  int part=1,nparts=1;
  string perm_mat;
  int perm_index=-1;
  int minlesions=2;
  bool f_yates=0;
  bool f_fisher=0;
  bool f_zscore=0;
  bool f_flip=0;
  bool f_fdr=0;
  bool f_twotailed=0;
  bool f_nodup=0;
  float q=0;

  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-v")
      vbcmap_version();
    else if (args[i]=="-h")
      vbcmap_help();
    else if (args[i]=="-2")
      f_twotailed=1;
    else if (args[i]=="-z")
      f_zscore=1;
    else if (args[i]=="-f")
      f_flip=1;
    else if (args[i]=="-x")
      f_fisher=1;
    else if (args[i]=="-y")
      f_yates=1;
    else if (args[i]=="-nodup")
      f_nodup=1;
    else if (args[i]=="-pfile" && i<args.size()-1) {
      pfile=args[++i];
    }
    else if (args[i]=="-pmap" && i<args.size()-1) {
      pmapname=args[++i];
    }
    else if (args[i]=="-m" && i<args.size()-1) {
      maskfile=args[++i];
    }
    else if (args[i]=="-q" && i<args.size()-1) {
      f_fdr=1;
      q=strtod(args[++i]);
    }
    else if (args[i]=="-n" && i<args.size()-1) {
      minlesions=strtol(args[++i]);
      if (minlesions<2) minlesions=2;
    }
    else if (args[i]=="-op" && i<args.size()-2) {
      perm_mat=args[++i];
      perm_index=strtol(args[++i]);
    }
    else if (args[i]=="-p" && i<args.size()-2) {
      part=strtol(args[++i]);
      nparts=strtol(args[++i]);
    }
    else
      filelist.push_back(args[i]);
  }
  if (filelist.size()!=3) {
    vbcmap_help();
    exit(112);
  }
  ivname=filelist[0];
  dvname=filelist[1];
  outfile=filelist[2];

  Cube tmap,mask;
  Tes ts;
  VB_Vector depvar;
  if (ts.ReadFile(ivname)) {
    printf("[E] vbcmap: couldn't get grouping info from %s\n",ivname.c_str());
    exit(101);
  }
  if (depvar.ReadFile(dvname)) {
    printf("[E] vbcmap: couldn't get dependent variable info from %s\n",dvname.c_str());
    exit(102);
  }
  // build our mask
  ts.ExtractMask(mask);

  if (maskfile.size()) {
    Cube tmask;
    if (tmask.ReadFile(maskfile)) {
      printf("[E] vbcmap: couldn't read mask file %s\n",maskfile.c_str());
      exit(103);
    }
    if (!(tmask.dimsequal(mask))) {
      printf("[E] vbcmap: lesion maps and mask files have inconsistent dimensions\n");
      exit(104);
    }
    mask.intersect(tmask);
  }

  // permute order of dv if requested
  VB_Vector perm_order;
  if (perm_index>-1) {
    VBMatrix vm(perm_mat,0,0,perm_index,perm_index);
    perm_order=vm.GetColumn(0);
    VB_Vector tmp(depvar.size());
    for (uint32 i=0; i<depvar.size(); i++)
      tmp[i]=depvar[(int)perm_order[i]];
    depvar=tmp;
  }
  // convert dv to bitmask
  bitmask dvbm;
  dvbm.resize(depvar.size());
  dvbm.clear();
  for (size_t i=0; i<depvar.size(); i++) {
    if (!f_flip && fabs(depvar[i])>FLT_MIN)
      dvbm.set(i);
    if (f_flip && !(fabs(depvar[i])>FLT_MIN))
      dvbm.set(i);
  }
  
  string partstring;
  if (nparts>1)
    partstring="_part_"+strnum(part);

  bitmask bm;
  bm.resize(ts.dimt);
  map<bitmask,x2val> statlookup;
  map<bitmask,x2val>::iterator iter;
  Cube statmap(ts.dimx,ts.dimy,ts.dimz,vb_float);
  Cube pmap;
  Tes cimap;
  x2val res;
  Cube fdrmask;
  if (f_fdr || pmapname.size()) {
    pmap.SetVolume(ts.dimx,ts.dimy,ts.dimz,vb_float);
    fdrmask=mask;
    fdrmask.zero();
  }
  vector<double> pvals;   // all p vals used in fdr calculation
  bool f_non1=0;
  int16 val;
  for (int i=0; i<ts.dimx; i++) {
    for (int j=0; j<ts.dimy; j++) {
      for (int k=0; k<ts.dimz; k++) {
        if (!mask.testValue(i,j,k))
          continue;
        for (int m=0; m<ts.dimt; m++) {
          val=ts.getValue<int16>(i,j,k,m);
          if (val) {
            bm.set(m);
            if (val!=1) f_non1=1;
          }
          else
            bm.unset(m);
        }
        if (bm.count()<minlesions)
          continue;
        if (ts.dimt-bm.count()<2)
          continue;
        iter=statlookup.find(bm);
        if (iter==statlookup.end()) {
          // this is a new pattern
          if (f_fdr)
            fdrmask.SetValue(i,j,k,1);
          if (f_fisher)
            res=calc_fisher(bm,dvbm);
          else
            res=calc_chisquared(bm,dvbm,f_yates);
          if (f_twotailed)
            res.p*=2.0;
          // FIXME??? if we need the p value or z score, get it
          // if doing fdr, stash the p value
          if (f_fdr || pmapname.size()) {
            pmap.SetValue(i,j,k,res.p);
            pvals.push_back(res.p);
          }
          // if doing confidence intervals, stash that
          if (f_zscore)
            statmap.SetValue(i,j,k,res.z);
          else
            statmap.SetValue(i,j,k,res.x2);
          statlookup[bm]=res;
        }
        else {
          // this is a previously encountered pattern
          if (f_fdr && !f_nodup) {
            fdrmask.SetValue(i,j,k,1); 
            pvals.push_back(iter->second.p);
          }
          statmap.SetValue(i,j,k,iter->second.x2);
          if (f_fdr || pmapname.size())
            pmap.SetValue(i,j,k,iter->second.p);
        }
      }
    }
  }
  if (f_non1) {
    cout << "[W] vbcmap: non-0/1 values found in lesion map" << endl;
  }
  if (f_fdr) {
    vector<fdrstat> ffs=calc_multi_fdr_thresh(statmap,pmap,fdrmask,q);
    if (ffs.size()) {
      cout << (format("[I] vbcmap: FDR calculation included %d voxels with p values from %.4f to %.4f\n")
               %ffs[0].nvoxels%ffs[0].low%ffs[0].high).str();
      statmap.AddHeader("# the following thresholds must be exceeded for FDR control");
      vbforeach(fdrstat ff,ffs) {
        if (ff.maxind>=0)
          cout << (format("[I] vbcmap: FDR threhsold for q=%.2f is %.4f\n")%ff.q%ff.statval).str();
        else
          cout << (format("[I] vbcmap: no FDR threhsold could be identified for q=%.2f\n")%ff.q).str();
        string tmps=(format("fdrthresh: %g %g")%ff.q%ff.statval).str().c_str();
        statmap.AddHeader(tmps);
      }
    }
  }
  printf("[I] vbcmap: unique lesion patterns: %d\n",(int)statlookup.size());
  if (statmap.WriteFile(outfile)) {
    printf("[E] vbcmap: couldn't write stat map to %s\n",outfile.c_str());
    exit(111);
  }
  printf("[I] vbcmap: wrote stat map to %s\n",outfile.c_str());
  if (pfile.size() && pvals.size()) {
    VB_Vector pvec(pvals.size());
    int ind=0;
    vbforeach(double pp,pvals)
      pvec[ind++]=pp;
    if (pvec.WriteFile(pfile))
      cout << format("[E] vbcmap: error writing p file %s\n")%pfile;
    else
      cout << format("[I] vbcmap: wrote p file %s\n")%pfile;
  }
  if (pmapname.size()) {
    if (pmap.WriteFile(pmapname)) {
      printf("[E] vbcmap: couldn't write p map to %s\n",pmapname.c_str());
      exit(111);
    }
    printf("[I] vbcmap: wrote p map to %s\n",pmapname.c_str());
  }
  exit(0);
}


void
vbcmap_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbcmap_version()
{
  printf("VoxBo vbcmap (v%s)\n",vbversion.c_str());
}
