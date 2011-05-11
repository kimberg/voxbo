
// vbxts.cpp
// dump ROI stats
// Copyright (c) 2005-2009 by The VoxBo Development Team

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
#include <sstream>
#include "vbutil.h"
#include "vbio.h"
#include "glmutil.h"
#include "vbxts.hlp.h"

void vbxts_help();
void vbxts_version();

// generated output file name can be glmname/averagename/maskname

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vbxts_help();
    exit(0);
  }
  tokenlist args;

  vector<int>includelist,excludelist;
  int q_driftflag=0,q_meannormflag=0;
  int q_nodriftflag=0,q_nomeannormflag=0;
  int q_filterflag=0,q_removeflag=0,q_powerflag=0;
  vector<string>masklist;
  string glmname;
  string outfile;
  vector<string> teslist;
  vector<VBRegion> regionlist;
  vector<string> averagelist;
  long tsflags=0;

  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-d")
      q_driftflag=1;
    else if (args[i]=="-a" && i+1<args.size())
      averagelist.push_back(args[++i]);
    else if (args[i]=="-n")
      q_meannormflag=1;
    else if (args[i]=="-xd")
      q_nodriftflag=1;
    else if (args[i]=="-xn")
      q_nomeannormflag=1;
    else if (args[i]=="-m" && i+1<args.size())
      masklist.push_back(args[++i]);
    else if (args[i]=="-p" && i+3<args.size()) {
      VBRegion rr;
      uint32 xx=strtol(args[i+1]);
      uint32 yy=strtol(args[i+2]);
      uint32 zz=strtol(args[i+3]);
      rr.add(xx,yy,zz,0.0);
      rr.name=(string)"point_"+strnum(xx)+"_"+strnum(yy)+"_"+strnum(zz);
      regionlist.push_back(rr);
      i+=3;
    }
    else if (args[i]=="-g" && i+1<args.size())
      glmname=args[++i];
    else if (args[i]=="-t" && i+1<args.size())
      teslist.push_back(args[++i]);
    else if (args[i]=="-f")
      q_filterflag=1;
    else if (args[i]=="-o" && i+1<args.size())
      outfile=args[++i];
    else if (args[i]=="-x")
      q_removeflag=1;
    else if (args[i]=="-k")
      q_powerflag=1;
    else if (args[i]=="-h") {
      vbxts_help();
      exit(0);
    }
    else if (args[i]=="-v") {
      vbxts_version();
      exit(0);
    }
    else {
      printf("[E] vbxts: unrecognized argument: %s\n",args(i));
      exit(5);
    }
  }
  if (glmname.size()==0 && teslist.size()==0) {
    printf("[E] vbxts: you must specify either a GLM directory or a tes file\n");
    exit(5);
  }
  if (averagelist.size()==0)
    averagelist.push_back("none");

  // first convert the masks to regions
  int mx=0,my=0,mz=0;
  for (int i=0; i<(int)masklist.size(); i++) {
    Cube mask;
    VBRegion myregion;
    mask.ReadFile(masklist[i]);
    if (!mask.data) {
      printf("[E] vbxts: %s couldn't be loaded as a mask\n",masklist[i].c_str());
      continue;
    }
    if (mx==0) {
      mx=mask.dimx; my=mask.dimy; mz=mask.dimz;
    }
    else if (mx!=mask.dimx || my!=mask.dimy || mz!=mask.dimz) {
      printf("[E] vbxts: mask %s has inconsistent dimensions, ignoring\n",masklist[i].c_str());
      continue;
    }
    myregion.convert(mask,vb_agt,0.0);
    myregion.name=xfilename(masklist[i]);
    regionlist.push_back(myregion);
//     printf("[I] vbxts: mask %s includes %d voxels\n",
//            masklist[i].c_str(),myregion.size());
  }

  if (regionlist.size()==0) {
    printf("[E] vbxts: must specify either a valid mask or a point\n");
    exit(120);
  }

  GLMInfo glmi;
  vector <TASpec> averages;
  if (glmname.size()) {
    glmi.setup(glmname);
    if (glmi.stemname.size()==0) {
      printf("[E] vbxts: %s isn't a valid GLM directory\n",glmname.c_str());
      exit(200);
    }
    teslist=glmi.teslist;
    tsflags=glmi.glmflags;
    if (averagelist[0]=="all")
      averages=glmi.trialsets;
    else {
      for (int i=0; i<(int)averagelist.size(); i++) {
        for (int j=0; j<(int)glmi.trialsets.size(); j++) {
          if (averagelist[i]==glmi.trialsets[j].name)
            averages.push_back(glmi.trialsets[j]);
        }
      }
    }    
  }

  VB_Vector vv;
  if (q_driftflag) tsflags |= DETREND;
  if (q_meannormflag) tsflags |= MEANSCALE;
  if (q_nodriftflag) tsflags &= ~(DETREND);
  if (q_nomeannormflag) tsflags &= ~(MEANSCALE);
  printf("[I] vbxts: mean scaling: %s\n[I] vbxts: detrending: %s\n",
         (tsflags&MEANSCALE ? "yes" : "no"),
         (tsflags&DETREND ? "yes" : "no"));
  
  // FIXME CHECK MASK DIMENSION MATCH
  for (int j=0; j<(int)regionlist.size(); j++) {
    VBRegion tmpregion;
    tmpregion=restrictRegion(teslist,regionlist[j]);
    printf("[I] vbxts: region %s (%d voxel%s)\n",regionlist[j].name.c_str(),
           tmpregion.size(),(tmpregion.size()>1?"s":""));
    // load time series
    vv=getRegionTS(teslist,tmpregion,tsflags);
    // things you can only do with GLM data
    if (glmname.size()) {
      if (q_filterflag)
        glmi.filterTS(vv);
      if (q_removeflag)
        glmi.adjustTS(vv);
    }
    if (averages.size()) {
      for (int i=0; i<(int)averages.size(); i++) {
        VB_Vector tmpx;
        tmpx=averages[i].getTrialAverage(vv);
        if (q_powerflag) {
          tmpx=fftnyquist(tmpx);
          tmpx[0]=0;
        }
        if (outfile.size())
          tmpx.WriteFile(outfile);
        else
          tmpx.WriteFile("vbxts_"+regionlist[j].name+"_"+averages[i].name+".ref");
      }
    }
    else {
      if (q_powerflag) {
        vv=fftnyquist(vv);
        vv[0]=0;
      }
      if (outfile.size())
        vv.WriteFile(outfile);
      else
        vv.WriteFile("vbxts_"+regionlist[j].name+".ref");
    }
  }
  exit(0);
}

void
vbxts_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbxts_version()
{
  printf("VoxBo vbxts (v%s)\n",vbversion.c_str());
}
