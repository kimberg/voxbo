
// vbperminfo.cpp
// hack to print out information about permutation analyses
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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <gsl/gsl_cdf.h>
#include "vbutil.h"
#include "vbio.h"
#include "vbperminfo.hlp.h"

void vbperminfo_help();
void calc_peak(tokenlist &args);
void calc_peak_cluster(tokenlist &args);
void calc_peakmin(tokenlist &args);
void calc_cluster(tokenlist &args);
void calc_cutoff(tokenlist &args);
void calc_byvoxel(tokenlist &args);
void calc_dist(tokenlist &args);
double find_cluster_thresh(Cube &cb,int crit_type,int k);
vector<string> getfilenames(string fname);

int
main(int argc,char *argv[])
{
  tokenlist args;

  args.Transfer(argc-1,argv+1);

  // sanity check args
  if (args.size() < 1) {
    vbperminfo_help();
    exit(100);
  }

  if (args[0]=="-c") {
    calc_cluster(args);
  }
  else if (args[0]=="-p") {
    calc_peak(args);
  }
  else if (args[0]=="-r") {
    calc_cutoff(args);
  }
  else if (args[0]=="-k") {
    calc_peak_cluster(args);
  }
  else if (args[0]=="-pm") {
    calc_peakmin(args);
  }
  else if (args[0]=="-d" || args[0]=="-dr") {
    calc_dist(args);
  }
  else if (args[0]=="-u") {
    calc_byvoxel(args);
  }
  else {
    vbperminfo_help();
    exit(102);
  }
}

void
calc_byvoxel(tokenlist &args)
{
  string truevol;
  string iterfile;
  string outfile;
  for (size_t i=1; i<args.size(); i++) {
    if (args[i]=="-t" && i<args.size()-1)
      truevol=args[++i];
    else if (args[i]=="-i" && i<args.size()-1)
      iterfile=args[++i];
    else if (args[i]=="-o" && i<args.size()-1)
      outfile=args[++i];
  }
  if (truevol.empty() || iterfile.empty() || outfile.empty()) {
    vbperminfo_help();
    exit(102);
  }
  Cube tcube;
  Tes iterations;
  // read true volume
  if (tcube.ReadFile(truevol)) {
    cout << format("[E] vbperminfo: couldn't read true stat volume %s\n")%truevol;
    exit(110);
  }

  // we never go by time series, it's too slow for real cases.  but
  // the code is still in here, just in case
  enum {bytimeseries,byvolume,byfile,allatonce} mode;
  mode=allatonce;
  // read header of iterations
  if (iterations.ReadHeader(iterfile)==0) {
    if (iterations.fileformat.f_fastts)
      //mode=bytimeseries;
      mode=byvolume;
    else if (iterations.fileformat.read_vol_4D)
      mode=byvolume;
  }
  else {
    mode=byfile;
  }
  if (mode==allatonce)
    cout << "[I] vbperminfo: reading your iterations all at once\n";
  if (mode==bytimeseries)
    cout << "[I] vbperminfo: reading your iterations one position at a time\n";
  if (mode==byvolume)
    cout << "[I] vbperminfo: reading your iterations one volume at a time\n";
  if (mode==byfile)
    cout << "[I] vbperminfo: reading your iterations one file at a time\n";
  

  double val,pval,zval;
  Tes statvolume;
  Cube nevolume,mask;
  int notexceededcnt;
  int voxelcount=0;
  int itercnt=0;
  statvolume.SetVolume(tcube.dimx,tcube.dimy,tcube.dimz,3,vb_double);
  nevolume.SetVolume(tcube.dimx,tcube.dimy,tcube.dimz,vb_long);

  if (mode==bytimeseries) {
    // header is already read, so we do have the mask
    itercnt=iterations.dimt;
    iterations.ExtractMask(mask);
    for (int i=0; i<iterations.dimx; i++) {
      for (int j=0; j<iterations.dimy; j++) {
        for (int k=0; k<iterations.dimz; k++) {
          // if it's not in data, don't do anything to cause it to be included
          if (!iterations.GetMaskValue(i,j,k))
            continue;
          voxelcount++;
          if (iterations.ReadTimeSeries(iterfile,i,j,k)) {
            cout << format("[E] vbperminfo: couldn't read series data from iteration volume\n");
            exit(110);
          }
          val=tcube.GetValue(i,j,k);
          // iterations.timeseries is our series, val is our ref val
          // get the p value by counting the proportion of elements exceeded
          notexceededcnt=0;
          for (uint32 m=0; m<iterations.timeseries.size(); m++) {
            if (!(val>iterations.timeseries[m]))
              notexceededcnt++;
          }
          nevolume.setValue<int32>(i,j,k,notexceededcnt);
        }
      }
    }
  }
  else if (mode==byvolume) {
    // header is already read, so we do have the mask
    Cube cb;
    mask.SetVolume(tcube.dimx,tcube.dimy,tcube.dimz,vb_byte);
    itercnt=iterations.dimt;
    for (int t=0; t<iterations.dimt; t++) {
      if (iterations.ReadVolume(iterfile,t,cb)) {
        cout << format("[E] vbperminfo: couldn't read volume data from iteration volume %s\n")%iterfile;
        exit(111);
      }
      for (int i=0; i<cb.dimx*cb.dimy*cb.dimz; i++) {
        // if it's not in data, skip it
        if (!iterations.GetMaskValue(i))
          continue;
        val=cb.getValue(i);
        if (!mask.testValueUnsafe<char>(i)) {
          if (fabs(val)>=DBL_MIN)
            mask.setValue<char>(i,1);
        }
        voxelcount++;
        if (!(tcube.getValue<double>(i)>val)) {
          int32 ne=nevolume.getValue<int32>(i);
          nevolume.setValue<int32>(i,ne+1);
        }
      }
    }
  }
  else if (mode==byfile) {
    // header is NOT already read, so we need to keep track of whether
    // or not we've seen nonzero values for each voxel
    vector<string> fnames=getfilenames(iterfile);
    if (fnames.empty()) {
      cout << format("[E] vbperminfo: no iteration volumes found by that name or location\n");
      exit(112);
    }
    itercnt=fnames.size();
    Cube cb;
    mask.SetVolume(tcube.dimx,tcube.dimy,tcube.dimz,vb_byte);
    for (size_t t=0; t<fnames.size(); t++) {
      if (cb.ReadFile(fnames[t])) {
        cout << format("[E] vbperminfo: couldn't read volume data from iteration volume %s\n")%fnames[t];
        exit(111);
      }
      for (int i=0; i<cb.dimx*cb.dimy*cb.dimz; i++) {
        // if we have nonzero data, note it
        val=cb.getValue<double>(i);
        if (!mask.testValueUnsafe<char>(i)) {
          if (fabs(val)>=DBL_MIN)
            mask.setValue<char>(i,1);
        }
        if (!(tcube.getValue<double>(i)>val)) {
          int32 ne=nevolume.getValue<int32>(i);
          nevolume.setValue<int32>(i,ne+1);
        }
      }
    }
  }
  else {
    cout << "NOT HANDLED YET" << endl;
    exit(200);
  }
  for (int i=0; i<mask.dimx; i++) {
    for (int j=0; j<mask.dimy; j++) {
      for (int k=0; k<mask.dimz; k++) {
        if (!(mask.testValueUnsafe<char>(i,j,k))) continue;
        pval=(double)nevolume.GetValue(i,j,k)/(double)itercnt;
        zval=gsl_cdf_ugaussian_Qinv(pval);
        statvolume.SetValue(i,j,k,0,pval);
        statvolume.SetValue(i,j,k,1,tcube.GetValue(i,j,k));
        statvolume.SetValue(i,j,k,2,zval);
      }
    }
  }
  statvolume.AddHeader("vol_type: 0 p");
  statvolume.AddHeader("vol_type: 1 original stat");
  statvolume.AddHeader("vol_type: 2 z");

  cout << format("[I] vbperminfo: %d voxels processed\n")%voxelcount;
  if (statvolume.WriteFile(outfile)) {
    cout << format("[E] vbperminfo: couldn't write output file %s\n")%outfile;
    exit(110);
  }
  cout << format("[I] vbperminfo: wrote output volume %s\n")%outfile;
  exit(0);
}

void
calc_dist(tokenlist &args)
{
  // if f_rev==0, we return the proportion >= our value
  // if f_rev==1, we return the proportion <= our value
  bool f_rev=0;
  if (args[0]=="-dr") f_rev=1;
  if (args.size()!=3) {
    vbperminfo_help();
    exit(202);
  }
  double crit=-FLT_MAX;
  VB_Vector vv;
  // if second arg is a ref file, use it!
  if (!vv.ReadFile(args[2]))
    if (vv.size()==1)
      crit=vv[0];
  if (vv.size()!=1) { // couldn't get it from a file
    pair<bool,double> critx=strtodx(args[2]);
    if (critx.first) {
      cout << "[E] vbperminfo: couldn't derive a valid true value from " << args[2] << endl;
      exit(147);
    }
    crit=critx.second;
  }

  if (vv.ReadFile(args[1])) {
    cout << "[E] vbperminfo: couldn't open vector file " << args[1] << endl;
    exit(121);
  }
  if (vv.size()<100)
    cout << "[W] vbperminfo: number of elements in vector is only " << vv.size() << endl;
  vv.sort();
  int32 cnt=0;
  for (uint32 i=0; i<vv.size(); i++) {
    if (f_rev) {
      if ((vv[i]-crit)<DBL_MIN)
        cnt++;
    }
    else {
      if ((crit-vv[i])<DBL_MIN)
        cnt++;
    }
  }
  cout << (format("[I] vbperminfo: your p value is %g\n")%((double)cnt/(double)(vv.size()))).str();
}

void
calc_cutoff(tokenlist &args)
{
  if (args.size() < 2 || args.size() > 3) {
    vbperminfo_help();
    exit(120);
  }

  VB_Vector myvec;
  double alpha=0.05;

  if (myvec.ReadFile(args[1])) {
    cout << "[E] vbperminfo: couldn't open result vector file " << args[1] << endl;
    exit(121);
  }

  // sort vector ascending
  myvec.sort();  // sorts ascending, luckily
  if (args.size()>2)
    alpha=strtod(args[2]);
  int sz=myvec.size();
  int index=(int)ceil((1.0-alpha)*sz)-1;

  cout << "[I] vbperminfo: File: " << args[1] << endl;
  cout << "[I] vbperminfo: Total permutations: " << myvec.size() << endl;
  if (alpha>0) {
    cout << "[I] vbperminfo: Alpha: " << alpha << endl;
    cout << "[I] vbperminfo: Cutoff index: " << index+1 << " (from 1 to " << myvec.size() << ")" << endl;
    cout << "[I] vbperminfo: Cutoff value: " << myvec[index] << endl;
  }
  else {
    alpha=0.001;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.005;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.01;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.025;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.05;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.07;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.10;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.15;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.20;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.30;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.40;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
    alpha=0.50;
    index=(int)ceil((1.0-alpha)*sz)-1;
    printf("[I] vbperminfo: Alpha: %.4f\n",alpha);
    printf("[I] vbperminfo: Cutoff index: %d (from 1 to %d)\n",index+1,(int)(myvec.size()));
    printf("[I] vbperminfo: Cutoff value: %.6f\n",myvec[index]);
    printf("\n");
  }

  exit(0);
}

void
calc_peak(tokenlist &args)
{
  if (args.size() < 3 || args.size() > 5) {
    vbperminfo_help();
    exit(120);
  }

  stringstream tmps;
  string iterfile=args[1];
  string outfile=args[2];
  string maskfile;
  if (args.size()>4)
    maskfile=args[4];
  double alpha=0.05;
  Tes iterations;
  VB_Vector myvec;
  Cube mask;

  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      cout << "[I] vbperminfo: couldn't read mask file " << maskfile << endl;
      exit(144);
    }
  }

  enum {byvolume,byfile,allatonce} mode;
  mode=allatonce;
  // read header of iterations
  if (iterations.ReadHeader(iterfile)==0) {
    mode=byvolume;
    if (iterations.fileformat.read_vol_4D)
      mode=byvolume;
    else
      mode=allatonce;
  }
  else
    mode=byfile;

  if (mode==byvolume)
    cout << "[I] vbperminfo: reading your iterations one volume at a time\n";
  if (mode==byfile)
    cout << "[I] vbperminfo: reading your iterations one file at a time\n";
  if (mode==allatonce)
    cout << "[I] vbperminfo: reading your iterations all at once\n";

  if (mode==byvolume) {
    myvec.resize(iterations.dimt);
    Cube cb;
    for (int t=0; t<iterations.dimt; t++) {
      if (iterations.ReadVolume(iterfile,t,cb)) {
        cout << format("[E] vbperminfo: couldn't read volume data from iteration volume %s\n")%iterfile;
        exit(111);
      }
      if (mask) {
        if (!(cb.dimsequal(mask))) {
          cout << format("[E] vbperminfo: volume %s dimensions don't match mask\n")%iterfile;
          exit(177);
        }
        cb.applymask(mask);
      }
      myvec[t]=cb.get_maximum();
    }
  }
  else if (mode==byfile) {
    vector<string> fnames=getfilenames(iterfile);
    if (fnames.empty()) {
      cout << format("[E] vbperminfo: no iteration volumes found by that name or location\n");
      exit(112);
    }
    int itercnt=fnames.size();
    myvec.resize(itercnt);
    Cube cb;
    for (int t=0; t<itercnt; t++) {
      if (cb.ReadFile(fnames[t])) {
        cout << format("[E] vbperminfo: couldn't read volume data from iteration volume %s\n")%fnames[t];
        exit(111);
      }
      if (mask) {
        if (!(cb.dimsequal(mask))) {
          cout << format("[E] vbperminfo: volume %s dimensions don't match mask\n")%fnames[t];
          exit(177);
        }
        cb.applymask(mask);
      }
      myvec[t]=cb.get_maximum();
    }
  }
  else if (mode==allatonce) {
    if (iterations.ReadFile(iterfile)) {
      cout << format("[E] vbperminfo: couldn't read iteration volume %s\n")%iterfile;
      exit(111);
    }
    myvec.resize(iterations.dimt);
    if (mask) {
      if (!(iterations.dimsequal(mask))) {
        cout << format("[E] vbperminfo: volume %s dimensions don't match mask\n")%iterfile;
        exit(177);
      }
      iterations.applymask(mask);
    }

    Cube cb;
    for (int t=0; t<iterations.dimt; t++) {
      iterations.getCube(t,cb);
      myvec[t]=cb.get_maximum();
    }
  }

  // now figure out the cutoff value for the supplied alpha or 0.5
  myvec.sort();   // sorts ascending, luckily
  if (args.size()>3)
    alpha=strtod(args[3]);
  int sz=myvec.size();
  int index=(int)ceil((1.0-alpha)*sz)-1;

  tmps.str("");
  tmps << "Peak threshold results";
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Permutation data source: " << iterfile;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Total permutations: " << myvec.size();
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Alpha: " << alpha;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff index: " << index+1 << " (from 1 to " << myvec.size() << ")";
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff value: " << myvec[index];
  myvec.AddHeader(tmps.str());


  alpha=0.005;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.01;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.025;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.05;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.07;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.10;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.15;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.20;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.30;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.40;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  alpha=0.50;
  index=(int)ceil((1.0-alpha)*sz)-1;
  tmps.str("");
  tmps << "Cutoff for alpha " << alpha << ": " << myvec[index];
  myvec.AddHeader(tmps.str());

  // now write the whole business out
  if (myvec.WriteFile(outfile)) {
    cout << "[E] vbperminfo: couldn't write result vector " << outfile << endl;
    exit(10);
  }
  exit(0);  // no error!
}

// the peakmin calculation means finding the min across n volumes for
// each voxel, and the finding the maximum minval across the volume.
// when there's only one volume, it reduces to just finding the max
// value.

class iterationset {
  vector<string>filenames;
  Tes mytes;
};

void
calc_peakmin(tokenlist &args)
{
  if (args.size() < 2) {
    vbperminfo_help();
    exit(120);
  }

  string maskfile="";
  string outfile="resultvalues.ref";
  double alpha=0.05;
  vector<string>stemlist;
  int p,v;
  stringstream tmps;
  for (size_t i=1; i<args.size(); i++) {
    if (args[i]=="-m" && i<args.size()-1) {
      maskfile=args[i+1];
      i++;
    }
    else if (args[i]=="-a" && i<args.size()-1) {
      alpha=strtod(args[i+1]);
      i++;
    }
    else if (args[i]=="-o" && i<args.size()-1) {
      outfile=args[i+1];
      i++;
    }
    else {
      stemlist.push_back(args[i]);
    }
  }

  int nsets=stemlist.size();
  vector<string> fnamelist[nsets];
  Cube mycub[nsets],mask;
  Tes teslist[nsets];
  int xx=0,yy=0,zz=0;
  int nperms=0;

  // FIXME sanity check alpha

  // build our list of stuff
  for (int i=0; i<nsets; i++) {
    Tes tt;
    if (!(tt.ReadHeader(stemlist[i]))) {
      fnamelist[i].push_back(stemlist[i]);
      if (!dimsConsistent(xx,yy,zz,tt.dimx,tt.dimy,tt.dimz)) {
        printf("[E] vbperminfo: dimension mismatch\n");
        exit(202);
      }
      if (nperms==0)
        nperms=tt.dimt;
      else if (tt.dimt<nperms)
        nperms=tt.dimt;
      continue;
    }
    vglob vg(stemlist[i]+"*");
    if (vg.size()<2)
      vg.load(stemlist[i]+"/*.cub");
    if (vg.size()<2)
      vg.load(stemlist[i]+"/*.img");
    if (vg.size()<2)
      vg.load(stemlist[i]+"/*.nii");
    if (vg.size()<2)
      vg.load(stemlist[i]+"/*");
    if (vg.size()>1) {
      for (size_t j=0; j<vg.size(); j++) {
        fnamelist[i].push_back(vg[j]);
      }
      if (nperms==0)
        nperms=fnamelist[i].size();
      else if ((int)fnamelist[i].size()<nperms)
        nperms=fnamelist[i].size();
      continue;
    }
    printf("[E] vbperminfo: couldn't make sense of iteration set %s\n",
           stemlist[i].c_str());
  }
  
  // load our mask
  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      printf("[E] vbperminfo: invalid mask volume %s\n",maskfile.c_str());
      exit(170);
    }
    if (!(dimsConsistent(xx,yy,zz,mask.dimx,mask.dimy,mask.dimz))) {
      printf("[E] vbperminfo: perm cube and mask dimensions don't agree\n");
      exit(171);
    }
  }

  VB_Vector minvec(nperms);
  VB_Vector maxvec(nperms);
  double minval,val,peakmin=0.0;
  double maxval,peakmax=0.0;
  int voxelcount=0;
  int xstart=-1,xcount=-1;
  for (p=0; p<nperms; p++) {
    // make sure we have a slice of the 4D volumes loaded
    if (xstart+xcount-1<p) {
      printf("[I] vbperminfo: Percent done: %d\n",(p*100)/nperms);
      xstart=p;
      xcount=200;
      if (xstart+xcount>nperms-1) xcount=nperms-xstart;
      for (int i=0; i<nsets; i++) {
        if (fnamelist[i].size()==1) {
          teslist[i].init();
          teslist[i].ReadHeader(fnamelist[i][0]);
          if (!(dimsConsistent(xx,yy,zz,teslist[i].dimx,
                               teslist[i].dimy,teslist[i].dimz))) {
            printf("[E] vbperminfo: dimension mismatch\n");
            exit(240);
          }
          teslist[i].ReadData(fnamelist[i][0],xstart,xcount);
        }
      }
    }
    for (v=0; v<nsets; v++) {
      if (fnamelist[v].size()==1)
        teslist[v].getCube(p-xstart,mycub[v]);
      else
        mycub[v].ReadFile(fnamelist[v][p]);
      if (!(dimsConsistent(xx,yy,zz,mycub[v].dimx,mycub[v].dimy,mycub[v].dimz))) {
        printf("[E] vbperminfo: dimension mismatch\n");
        exit(240);
      }
    }
    voxelcount=0;

    for (int i=0; i<xx*yy*zz; i++) {
      // if we're masked out, skip it
      if (mask.data)
        if (!(mask.testValue(i)))
          continue;
      // for this voxel, check all volumes for the min value
      minval=maxval=mycub[0].getValue<double>(i);
      for (v=1; v<nsets; v++) {
        val=mycub[v].getValue<double>(i);
        if (val < minval)
          minval=val;
        if (val > maxval)
          maxval=val;
      }
      if (voxelcount==0) {
        peakmin=minval;
        peakmax=maxval;
      }
      else {
        if (minval > peakmin)
          peakmin=minval;
        if (maxval > peakmax)
          peakmax=maxval;
      }
      voxelcount++;
    }

    minvec[p]=peakmin;
    maxvec[p]=peakmax;
  }

  // FIXME temporarily using maxvec instead of minvec
  // minvec=maxvec;
  // now figure out the cutoff value for the supplied alpha or 0.5
  minvec.sort();  // sorts ascending, luckily
  int sz=minvec.size();
  int index=(int)ceil((1.0-alpha)*sz)-1;
  string info;

  tmps.str("");
  tmps << "Peak minimum (vbpermfinfo -pm) permutation summary";
  minvec.AddHeader(tmps.str());

  for (size_t i=0; i<stemlist.size(); i++) {
    tmps.str("");
    tmps << "Permutation directory: " << stemlist[i];
    minvec.AddHeader(tmps.str());
  }

  tmps.str("");
  tmps << "Total permutations: " << minvec.size();
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Alpha: " << alpha;
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff index: " << index+1 << " (from 1 to " << minvec.size() << ")";
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff value: " << minvec[index];
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Dimensions: " << xx << " x " << yy << " x " << zz;
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Mask: ";
  if (maskfile.size()) tmps << maskfile;
  else tmps << "none";
  minvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Voxel count: " << voxelcount;
  minvec.AddHeader(tmps.str());

  alpha=0.005;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.01;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.025;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.05;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.07;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.10;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.15;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.20;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.30;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.40;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  alpha=0.50;
  index=(int)ceil((1.0-alpha)*sz)-1;
  minvec.AddHeader("Cutoff for alpha "+strnum(alpha)+": "+strnum(minvec[index]));

  // now write the whole business out
  if (minvec.WriteFile(outfile)) {
    cout << "[E] vbperminfo: couldn't write result vector " << outfile << endl;
    exit(100);
  }
  exit(0);  // no error!
}

void
calc_cluster(tokenlist &args)
{
  if (args.size() < 4 || args.size() > 5) {
    vbperminfo_help();
    exit(120);
  }

  double alpha=0.05;
  stringstream tmps;
  string stem=args[1];
  string outfile=args[2];
  double crit_val=strtod(args[3]);
  
  vglob vg(stem+"*");
  if (vg.size()<1) {
    cout << "[E] vbperminfo: no permutation cubes found" << endl;
    exit(140);
  }
  VB_Vector myvec(vg.size());
  for (size_t i=0; i<vg.size(); i++) {
    Cube cb;
    if (cb.ReadFile(vg[i])) {
      cout << "[E] vbperminfo: couldn't read file " << vg[i] << endl;
      exit(140);
    }
    vector<VBRegion> rlist;
    if (crit_val>0)
      rlist=findregions(cb,vb_gt,crit_val);
    else
      rlist=findregions(cb,vb_agt,crit_val);
    if (!rlist.size()) {
      myvec[i]=0;
      continue;
    }
    int maxsize=rlist[0].size();
    for (int j=1; j<(int)rlist.size(); j++) {
      if ((int)rlist[j].size()>maxsize)
        maxsize=rlist[j].size();
    }
    myvec[i]=maxsize;
  }

  // now figure out the cutoff value for the supplied alpha or 0.5
  myvec.sort();  // sorts ascending, luckily
  if (args.size()>4)
    alpha=strtod(args[4]);
  int sz=myvec.size();
  int index=(int)ceil((1.0-alpha)*sz)-1;
  string info;

  tmps.str("");
  tmps << "Cluster permutation results, cluster threshold of " << crit_val;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Permutation cube stem: " << stem;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Total permutations: " << myvec.size();
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Alpha: " << alpha;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff index: " << index+1 << " (from 1 to " << myvec.size() << ")";
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff value: " << myvec[index];
  myvec.AddHeader(tmps.str());

  // now write the whole business out
  if (myvec.WriteFile(outfile)) {
    cout << "[E] vbperminfo: couldn't write result vector " << outfile << endl;
    exit(130);
  }
  exit(0);  // no error!
}

void
calc_peak_cluster(tokenlist &args)
{
  if (args.size() != 5) {
    vbperminfo_help();
    exit(120);
  }

  string stem=args[1];
  string outfile=args[2];
  int k=strtol(args[3]);
  double alpha=strtod(args[4]);
  int crit_type=vb_gt;
  
  stringstream tmps;

  vglob vg(stem+"*");
  if (vg.size() < 1) {
    cout << "[E] vbperminfo: no permutation cubes found" << endl;
    exit(140);
  }
  VB_Vector myvec(vg.size());
  double thresh;
  for (size_t i=0; i<vg.size(); i++) {
    Cube cb;
    if (cb.ReadFile(vg[i])) {
      cout << "[E] vbperminfo: couldn't read file " << vg[i] << endl;
      exit(140);
    }
    //continue;
    thresh=find_cluster_thresh(cb,crit_type,k);
    if (thresh>=0.0)
      myvec[i]=thresh;
    else
      printf("[E] vbperminfo: couldn't find a usable threshold for volume %s\n",
             vg[i].c_str());
  }

  // now figure out the cutoff value for the supplied alpha
  myvec.sort();  // sorts ascending, luckily
  if (args.size()>4)
    alpha=strtod(args[4]);
  int sz=myvec.size();
  int index=(int)ceil((1.0-alpha)*sz)-1;
  string info;

  tmps.str("");
  tmps << "Cluster-constrained permutation results, cluster size of " << k;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Permutation cube stem: " << stem;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Total permutations: " << myvec.size();
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Alpha: " << alpha;
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff index: " << index+1 << " (from 1 to " << myvec.size() << ")";
  myvec.AddHeader(tmps.str());

  tmps.str("");
  tmps << "Cutoff value: " << myvec[index];
  myvec.AddHeader(tmps.str());

  // now write the whole business out
  if (myvec.WriteFile(outfile)) {
    cout << "[E] vbperminfo: couldn't write result vector " << outfile << endl;
    exit(130);
  }

}

double
find_cluster_thresh(Cube &cb,int crit_type,int k)
{
  if (crit_type!=vb_agt && crit_type!=vb_gt)
    return -1.0;
  vector<double> cubevals;
  vector<VBRegion> rlist;
  double val;
  for (int i=0; i<cb.dimx; i++) {
    for (int j=0; j<cb.dimy; j++) {
      for (int k=0; k<cb.dimz; k++) {
        val=cb.getValue<double>(i,j,k);
        if (crit_type==vb_agt)
          val=fabs(val);
        if (crit_type==vb_gt && val<0)
          continue;
        cubevals.push_back(val);
      }
    }
  }
  sort(cubevals.begin(),cubevals.end());
  int low=0,high=cubevals.size()-1,middle,clusters;
  int lowclusters=1;
  int highclusters=0;
  middle=low+((high-low)*19/20);   // heuristic good starting point, avoid too many cycles of whole-brain clusters
  while(TRUE) {
    rlist=findregions(cb,crit_type,cubevals[middle]);
    clusters=0;
    for (int i=0; i<(int)rlist.size(); i++) {
      if ((int)rlist[i].size() >=k)
        clusters++;
    }
    if (clusters) {
      low=middle;
      lowclusters=clusters;
    }
    else {
      high=middle;
      highclusters=clusters;
    }
    if (high-low <=1)
      break;
    middle=low+((high-low)/2);
  }
  if (lowclusters && highclusters==0)
    return cubevals[high];
  return -2.0;
}

// read4dfiles will first try to read fname/*, then fname*, always
// excluding .hdr files

vector<string>
getfilenames(string fname)
{
  vglob vg;
  vector<string> fnames;
  string stem;

  vg.load(fname+"/*");
  if (vg.size()==0)
    vg.load(fname+"*");

  for (size_t i=0; i<vg.size(); i++)
    if (xgetextension(vg[i])!="hdr")
      fnames.push_back(vg[i]);

  return fnames;
}

void
vbperminfo_help()
{
  cout << boost::format(myhelp) % vbversion;
}
