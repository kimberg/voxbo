
// vbmap.cpp
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

#include "vbutil.h"
#include "vbio.h"
#include "glmutil.h"

void vbmap_help();
void vbmap_version();

// this thing needs a more appropriate name, and a more clearly
// delineated mission, plus command line arguments

class VBmap {
public:
  VBmap(tokenlist &args) {init(args);}
  string coordfile,lesionfile,dvfile,paramfile,permfile,maxfile;
  VBMatrix permmat;
  int permind;
  VBMatrix coords;
  VBMatrix params;
  Tes ts;
  VB_Vector dv;
  int readdata();
  int go();
  int process();
  int init(tokenlist &args);
};


int
main(int argc,char *argv[])
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);

  VBmap vbm(args);
  vbm.go();
  exit(0);
}

int
VBmap::init(tokenlist &args)
{
  coordfile="coords.mtx";
  lesionfile="lesions.tes";
  dvfile="dv.ref";
  paramfile="params.mtx";
  maxfile="max.ref";
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-h")
      vbmap_help(),exit(0);
    else if (args[i]=="-v")
      vbmap_version(),exit(0);
    else if (args[i]=="-c" && i<args.size()-1)
      coordfile=args[++i];
    else if (args[i]=="-l" && i<args.size()-1)
      lesionfile=args[++i];
    else if (args[i]=="-m" && i<args.size()-1)
      maxfile=args[++i];
    else if (args[i]=="-o" && i<args.size()-1)
      paramfile=args[++i];
    else if (args[i]=="-p" && i<args.size()-2) {
      permfile=args[++i];
      permind=strtol(args[++i]);
    }
    else {
      cout << "argh!\n" << endl;  // FIXME
      exit(99);
    }
  }

  return 0;
}

int
VBmap::go()
{
  readdata();
  process();
  //  writeresults();
  return 0;
}

int
VBmap::readdata()
{
  int err;
  err=coords.ReadFile(coordfile);
  // 
  err=ts.ReadFile(lesionfile);
  // 
  err=dv.ReadFile(dvfile);
  // 
  // params matrix is one row for each pair of coordinates, columns
  // for: intercept, vox1, vox2, interaction, error, stat for int,
  // stat for v1, stat for v2, stat for v1*v2

  if (permfile.size())
    err=permmat.ReadFile(permfile);
  return 0;
}


int
VBmap::process()
{
  int err;
  GLMInfo glmi;
  VB_Vector vv(ts.dimt);      // scratch space
  params.init(coords.rows/2,9);
  // two g matrices, one with and one without interaction term
  glmi.gMatrix.init(ts.dimt,4);
  // intercept always goes in there
  vv*=0.0; vv+=1.0;
  glmi.gMatrix.SetColumn(0,vv);
  glmi.contrast.scale="t";
  glmi.contrast.contrast.resize(4);
  int row=0;
  // permute dv if needed
  if (permmat.rows) {
    VB_Vector pvec=permmat.GetColumn(permind);
    VB_Vector tmp(dv.size());
    for (size_t i=0; i<dv.size(); i++)
      tmp[i]=dv[(int)pvec[i]];
    dv=tmp;
  }
  double maxstat=0.0;
  Cube statvol(ts.dimx,ts.dimy,ts.dimz,vb_float);
  for (size_t i=0; i<coords.rows/2; i++) {
    int x1=(int)coords(i*2,0);
    int y1=(int)coords(i*2,1);
    int z1=(int)coords(i*2,2);
    int x2=(int)coords(i*2+1,0);
    int y2=(int)coords(i*2+1,1);
    int z2=(int)coords(i*2+1,2);
    // grab voxel 1, voxel 2, calculate intersection, regress!
    ts.GetTimeSeries(x1,y1,z2);
    glmi.gMatrix.SetColumn(1,ts.timeseries);
    vv=ts.timeseries;   // save it for calculating the interaction
    ts.GetTimeSeries(x2,y2,z2);
    glmi.gMatrix.SetColumn(2,ts.timeseries);
    vv*=ts.timeseries;  // now it's the interaction term!
    glmi.gMatrix.SetColumn(3,vv);
    if (vv.getVariance() < FLT_MIN)
      continue;
    if (vv==glmi.gMatrix.GetColumn(1))
      continue;
    if (vv==glmi.gMatrix.GetColumn(2))
      continue;
    glmi.f1Matrix.clear();   // force recalc of f1 (pinv(g))
    err=glmi.RegressIndependent(dv);
    if (err){
      cout << "rierr " << err << endl;
      continue;  // FIXME
    }
    // add a row to the params matrix
    params.set(row,0,glmi.betas[0]);   // intercept weight
    params.set(row,1,glmi.betas[1]);   // v1 weight
    params.set(row,2,glmi.betas[2]);   // v2 weight
    params.set(row,3,glmi.betas[3]);   // v1*v2 weight
    params.set(row,4,glmi.betas[4]);   // error

    glmi.contrast.contrast*=0.0;
    glmi.contrast.contrast[0]=1;
    glmi.calc_stat();
    params.set(row,5,glmi.statval);   // stat for intercept

    glmi.contrast.contrast*=0.0;
    glmi.contrast.contrast[1]=1;
    glmi.calc_stat();
    params.set(row,6,glmi.statval);   // stat for v1
    if (abs(glmi.statval)>maxstat) maxstat=abs(glmi.statval);
    // update first voxel if we're more extreme
    if (abs(glmi.statval)>statvol.GetValue(x1,y1,z1))
      statvol.SetValue(x1,y1,z1,abs(glmi.statval));

    glmi.contrast.contrast*=0.0;
    glmi.contrast.contrast[2]=1;
    glmi.calc_stat();
    params.set(row,7,glmi.statval);   // stat for v2
    if (abs(glmi.statval)>maxstat) maxstat=abs(glmi.statval);
    // update second voxel if we're more extreme
    if (abs(glmi.statval)>statvol.GetValue(x2,y2,z2))
      statvol.SetValue(x2,y2,z2,abs(glmi.statval));

    glmi.contrast.contrast*=0.0;
    glmi.contrast.contrast[3]=1;
    glmi.calc_stat();
    params.set(row,8,glmi.statval);   // stat for v1*v2
    if (abs(glmi.statval)>maxstat) maxstat=abs(glmi.statval);
    // update either voxel if we're more extreme
    if (abs(glmi.statval)>statvol.GetValue(x1,y1,z1))
      statvol.SetValue(x1,y1,z1,abs(glmi.statval));
    if (abs(glmi.statval)>statvol.GetValue(x2,y2,z2))
      statvol.SetValue(x2,y2,z2,abs(glmi.statval));

    // keep a map of the maximum magnitude associated with each voxel
    
    row++;
  }
  VBMatrix p2(row,params.cols);
  for (int i=0; i<row; i++)
    p2.SetRow(i,params.GetRow(i));
  p2.WriteFile(paramfile);
  VB_Vector maxstatvec(1);
  maxstatvec[0]=maxstat;
  maxstatvec.WriteFile(maxfile);
  statvol.WriteFile("statvol.nii.gz");
  return 0;
}



void vbmap_help()
{
  cout << format("usage: vbmap -c <coords> -l <lesions> -o <params> -p <permmtat> <ind>\n");
}

void vbmap_version()
{
  cout << format("version X!\n");
}

