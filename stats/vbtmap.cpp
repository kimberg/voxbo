
// vbtmap.cpp
// calculate a t-test in each voxel
// Copyright (c) 2009-2011 by The VoxBo Development Team

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
#include "vbtmap.hlp.h"

void vbtmap_help();
void vbtmap_version();

int
main(int argc,char *argv[])
{
  if (argc<2) {
    vbtmap_help();
    exit(0);
  }

  tokenlist args;
  vector<string>filelist;
  string dvname,ivname,outfile,maskfile,pfile;
  args.Transfer(argc-1,argv+1);
  int part=1,nparts=1;
  string perm_mat;
  int perm_index=-1;
  int minlesions=2;
  bool f_welchs=0;
  bool f_zscore=0;
  bool f_flip=0;
  bool f_fdr=0;
  bool f_twotailed=0;
  bool f_nodup=0;
  float q=0;
  string cifile;
  double cival=95,tcialpha=0.975;
  bool f_ci=0;

  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-v")
      vbtmap_version();
    else if (args[i]=="-h")
      vbtmap_help();
    else if (args[i]=="-w")
      f_welchs=1;
    else if (args[i]=="-2")
      f_twotailed=1;
    else if (args[i]=="-z")
      f_zscore=1;
    else if (args[i]=="-f")
      f_flip=1;
    else if (args[i]=="-nodup")
      f_nodup=1;
    else if (args[i]=="-pfile" && i<args.size()-1) {
      pfile=args[++i];
    }
    else if (args[i]=="-cifile" && i<args.size()-2) {
      f_ci=1;
      cifile=args[++i];
      cival=strtod(args[++i]);
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
    vbtmap_help();
    exit(112);
  }
  ivname=filelist[0];
  dvname=filelist[1];
  outfile=filelist[2];

  if (f_ci) {
    if (cival<0 || cival>100) {
      cout << "[E] desired confidence interval must be in the 0-100 range\n";
      exit(1);
    }
    if (cival>1.0) cival/=100.0;
    // for a 95% CI we're going to use a tCI based on alpha=97.5%
    tcialpha=cival+(1.0-cival)/2.0;
  }

  Cube tmap,mask;
  Tes ts;
  VB_Vector depvar;
  if (ts.ReadFile(ivname)) {
    printf("[E] vbtmap: couldn't get grouping info from %s\n",ivname.c_str());
    exit(101);
  }
  if (depvar.ReadFile(dvname)) {
    printf("[E] vbtmap: couldn't get dependent variable info from %s\n",dvname.c_str());
    exit(102);
  }
  // build our mask
  ts.ExtractMask(mask);

  if (maskfile.size()) {
    Cube tmask;
    if (tmask.ReadFile(maskfile)) {
      printf("[E] vbtmap: couldn't read mask file %s\n",maskfile.c_str());
      exit(103);
    }
    if (!(tmask.dimsequal(mask))) {
      printf("[E] vbtmap: lesion maps and mask files have inconsistent dimensions\n");
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

  string partstring;
  if (nparts>1)
    partstring="_part_"+strnum(part);

  bitmask bm;
  bm.resize(ts.dimt);
  map<bitmask,tval> statlookup;
  map<bitmask,tval>::iterator iter;
  Cube statmap(ts.dimx,ts.dimy,ts.dimz,vb_float);
  Cube pmap;
  Tes cimap;
  tval res;
  Cube fdrmask;
  if (f_fdr) {
    pmap.SetVolume(ts.dimx,ts.dimy,ts.dimz,vb_float);
    fdrmask=mask;
    fdrmask.zero();
  }
  if (f_ci) {
    cimap.SetVolume(ts.dimx,ts.dimy,ts.dimz,3,vb_float);
    string cihdr=
      "# %d%% confidence interval volume containing:\n"
      "# volume 0: mean difference\n"
      "# volume 1: lower bound\n"
      "# volume 2: upper bound\n"
      "# produced by vbtmap (v%s)";
    cihdr=(format(cihdr)%(int)(cival*100.0)%vbversion).str();
    cimap.header.push_back(cihdr);
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
          if (f_welchs)
            res=calc_welchs(depvar,bm);
          else
            res=calc_ttest(depvar,bm);
          // if we're flipping, flip now
          if (f_flip) res.t*=-1.0;
          // if we need the p value or z score, get it
          if (f_zscore || f_fdr)
            t_to_p_z(res,f_twotailed);
          // if doing fdr, stash the p value
          if (f_fdr) {
            pmap.SetValue(i,j,k,res.p); 
            pvals.push_back(res.p);
          }
          // if doing confidence intervals, stash that
          if (f_ci) {
            if (f_flip) res.diff*=-1;
            res.halfci=res.stderror*gsl_cdf_tdist_Pinv(tcialpha,res.df);
            cimap.SetValue(i,j,k,0,res.diff-res.halfci);
            cimap.SetValue(i,j,k,1,res.diff);
            cimap.SetValue(i,j,k,2,res.diff+res.halfci);
          }
          if (f_zscore)
            statmap.SetValue(i,j,k,res.z);
          else
            statmap.SetValue(i,j,k,res.t);
          statlookup[bm]=res;
        }
        else {
          // this is a previously encountered pattern
          if (f_fdr && !f_nodup) {
            fdrmask.SetValue(i,j,k,1); 
            pvals.push_back(iter->second.p);
          }
          if (f_zscore)
            statmap.SetValue(i,j,k,iter->second.z);
          else
            statmap.SetValue(i,j,k,iter->second.t);
          if (f_fdr)
            pmap.SetValue(i,j,k,iter->second.p);
          if (f_ci) {
            cimap.SetValue(i,j,k,0,iter->second.diff-iter->second.halfci);
            cimap.SetValue(i,j,k,1,iter->second.diff);
            cimap.SetValue(i,j,k,2,iter->second.diff+iter->second.halfci);
          }
        }
      }
    }
  }
  if (f_non1) {
    cout << "[W] vbtmap: non-0/1 values found in lesion map" << endl;
  }
  if (f_ci) {
    if (cimap.WriteFile(cifile))
      cout << format("[E] vbtmap: error writing CIfile %s\n")%cifile;
    else
      cout << format("[I] vbtmap: wrote CI file %s\n")%cifile;
  }
  if (f_fdr) {
    vector<fdrstat> ffs=calc_multi_fdr_thresh(statmap,pmap,fdrmask,q);
    if (ffs.size()) {
      cout << (format("[I] vbtmap: FDR calculation included %d voxels with p values from %.4f to %.4f\n")
               %ffs[0].nvoxels%ffs[0].low%ffs[0].high).str();
      statmap.AddHeader("# the following thresholds must be exceeded for FDR control");
      vbforeach(fdrstat ff,ffs) {
        if (ff.maxind>=0)
          cout << (format("[I] vbtmap: FDR threhsold for q=%.2f is %.4f\n")%ff.q%ff.statval).str();
        else
          cout << (format("[I] vbtmap: no FDR threhsold could be identified for q=%.2f\n")%ff.q).str();
        string tmps=(format("fdrthresh: %g %g")%ff.q%ff.statval).str().c_str();
        statmap.AddHeader(tmps);
      }
    }
  }
  if (pfile.size() && pvals.size()) {
    VB_Vector pvec(pvals.size());
    int ind=0;
    vbforeach(double pp,pvals)
      pvec[ind++]=pp;
    if (pvec.WriteFile(pfile))
      cout << format("[E] vbtmap: error writing p file %s\n")%pfile;
    else
      cout << format("[I] vbtmap: wrote p file %s\n")%pfile;
  }
  printf("[I] vbtmap: unique lesion patterns: %d\n",(int)statlookup.size());
  if (statmap.WriteFile(outfile)) {
    printf("[E] vbtmap: couldn't write stat map to %s\n",outfile.c_str());
    exit(111);
  }
  printf("[I] vbtmap: wrote stat map to %s\n",outfile.c_str());
  exit(0);
}


void
vbtmap_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbtmap_version()
{
  printf("VoxBo vbtmap (v%s)\n",vbversion.c_str());
}
