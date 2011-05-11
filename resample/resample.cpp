
// resample.cpp
// sinc resampling of volumes, adapted from spm
// Copyright (c) 1998-2008 by The VoxBo Development Team

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

// note that the guts of the underlying sinc resample code were ported
// from SPM96b source code written by John Ashburner

using namespace std;

#include <stdio.h>
#include <string>
#include "vbutil.h"
#include "vbio.h"
#include "imageutils.h"

void resample_help();

int
main(int argc,char *argv[])
{
  tzset();                     // make sure all times are timezone corrected
  if (argc < 2) {              // not enough args, display autodocs
    resample_help();
    exit(0);
  }

  Resample rr;
  enum {mode_rr,mode_rc,mode_rz,mode_rd,mode_ra,mode_d,mode_rw,mode_none} mode=mode_none;

  tokenlist args;
  tokenlist files;
  string reffile;
  int err;
  int zsize=1,dimx=1,dimy=1,dimz=1;
  double x1=0,y1=0,z1=0;          // start voxel (0 indexed) in x,y,z
  int nx=1,ny=1,nz=1;             // number of voxels in resampled image
  double xstep=1,ystep=1,zstep=1; // resample interval in voxels
  bool q_nnflag=0,q_xxflag=0,q_yyflag=0,q_zzflag=0;
  args.Transfer(argc-1,argv+1);
  for(size_t i=0; i<args.size(); i++) {
    if (args[i]=="-rr" && i<args.size()-1) {
      mode=mode_rr;
      reffile=args[++i];
    }
    else if (args[i]=="-rc" && i<args.size()-1) {
      mode=mode_rc;
      reffile=args[++i];
    }
    else if (args[i]=="-rz" && i<args.size()-2) {
      mode=mode_rz;
      reffile=args[++i];
      zsize=strtol(args[++i]);
    }
    else if (args[i]=="-rd" && i<args.size()-1) {
      mode=mode_rd;
      reffile=args[++i];
    }
    else if (args[i]=="-ra" && i<args.size()-1) {
      mode=mode_ra;
      reffile=args[++i];
    }
    else if (args[i]=="-rw" && i<args.size()-1) {
      mode=mode_rw;
      reffile=args[++i];
    }
    else if (args[i]=="-d" && i<args.size()-3) {
      mode=mode_d;
      dimx=strtol(args[++i]);
      dimy=strtol(args[++i]);
      dimz=strtol(args[++i]);
    }
    else if (args[i]=="-xx" && i<args.size()-3) {
      q_xxflag=1;
      x1=strtod(args[++i]);
      xstep=strtod(args[++i]);
      nx=strtol(args[++i]);
    }
    else if (args[i]=="-yy" && i<args.size()-3) {
      q_yyflag=1;
      y1=strtod(args[++i]);
      ystep=strtod(args[++i]);
      ny=strtol(args[++i]);
    }
    else if (args[i]=="-zz" && i<args.size()-3) {
      q_zzflag=1;
      z1=strtod(args[++i]);
      zstep=strtod(args[++i]);
      nz=strtol(args[++i]);
    }
    else if (args[i]=="-nn")
      q_nnflag=1;
    else
      files.Add(args[i]);
  }

  // grab a 
  Cube refvol;
  Cube cb;
  Tes ts;
  if (reffile.size()) {
    if (refvol.ReadFile(reffile)) {
      Tes ts;
      if (ts.ReadFile(reffile)) {
        printf("[E] resample: couldn't read ref volume %s\n",reffile.c_str());
        exit(202);
      }
      ts.getCube(0,refvol);
    }
  }
  if (files.size()==1)
    files.Add("r"+files[0]);
  if (files.size()!=2) {
    printf("[E] resample: you need to specify one input and one output file\n");
    exit(202);
  }
  if (cb.ReadFile(files[0])) {
    if (ts.ReadFile(files[0])) {
      printf("[E] resample: couldn't read input file %s\n",files[0].c_str());
      exit(202);
    }
    ts.getCube(0,cb);
  }

  // set default resample
  rr.SetXX(0.0,1.0,cb.dimx);
  rr.SetYY(0.0,1.0,cb.dimy);
  rr.SetZZ(0.0,1.0,cb.dimz);
  // process the special modes
  if (mode==mode_rr)
    rr.UseZ(cb,refvol,0);
  else if (mode==mode_ra)
    rr.UseCorner(cb,refvol);
  else if (mode==mode_rw)
    rr.UseCorner2(cb,refvol);
  else if (mode==mode_rz) {
    rr.UseZ(cb,refvol,zsize);
  }
  else if (mode==mode_rd)
    rr.UseDims(cb,refvol);
  else if (mode==mode_rc)
    rr.UseTLHC(cb,refvol);
  else if (mode==mode_d)
    rr.UseSpecifiedDims(cb,dimx,dimy,dimz);
  // process any overrides
  if (q_xxflag)
    rr.SetXX(x1,xstep,nx);
  if (q_yyflag)
    rr.SetYY(y1,ystep,ny);
  if (q_zzflag)
    rr.SetZZ(z1,zstep,nz);
  Cube newcube;
  if (ts.data) {
    Tes newtes;
    for (int i=0; i<ts.dimt; i++) {
      ts.getCube(i,cb);
      if (q_nnflag)
        rr.NNResampleCube(cb,newcube);
      else
        rr.SincResampleCube(cb,newcube);
      if (i==0) {
        newtes.SetVolume(newcube.dimx,newcube.dimy,newcube.dimz,ts.dimt,newcube.datatype);
        newtes.header=newcube.header;
      }
      newtes.SetCube(i,newcube);
    }
    newtes.header=ts.header;
    vector<string> hh=rr.headerstrings();
    newtes.header.insert(newtes.header.end(),hh.begin(),hh.end());
    for (size_t i=0; i<hh.size(); i++)
      printf("[I] resample: %s\n",hh[i].c_str());
    err=newtes.WriteFile(files[1]);
  }
  else {
    if (q_nnflag)
      rr.NNResampleCube(cb,newcube);
    else
      rr.SincResampleCube(cb,newcube);
    vector<string> hh=rr.headerstrings();
    newcube.header.insert(newcube.header.end(),hh.begin(),hh.end());
    for (size_t i=0; i<hh.size(); i++)
      printf("[I] resample: %s\n",hh[i].c_str());
    err=newcube.WriteFile(files[1]);
  }
  if (err)
    printf("[E] resample: error writing file %s\n",files[1].c_str());
  else
    printf("[I] resample: done\n");
  exit(err);
}

void
resample_help()
{
  printf("\nVoxBo resample (v%s)\n",vbversion.c_str());
  printf("usage: resample <in> <out> [flags]\n");
  printf("flags:\n");
  printf("   -rr <ref>      ref=image in desired space\n");
  printf("   -rc <ref>      ref=image with desired field of view\n");
  printf("   -rz <ref> [z]  same but specify size in z\n");
  printf("   -rd <ref>      ref=image of desired dimensions\n");
  printf("   -ra <ref>      aligns to ref on all dims, using corner position\n");
  printf("   -rw <ref>      aligns to ref on all dims, using corner position, same voxel sizes\n");
  printf("   -d  <x y z>    desired dimensions\n");
  printf("   -nn            use nearest neighbor interpolation (default is sinc)\n");
  printf("   -xx <a b c>    a=start voxel (from 0); b=interval in voxels; c=# of voxels\n");
  printf("   -yy <a b c>    same for y dimension\n");
  printf("   -zz <a b c>    same for z dimension\n");
  printf("\n");
  printf("   -r     generic ref cube\n");
  printf("   -aa, -ax, -ay, -az  [origin/corner]\n");
  printf("   -sa, -sx, -sy, -sz  [n]\n");
  printf("\n");
  printf("Some examples for an axial image of dimensions 64x64x21:\n");
  printf("  invert the order of the slices: resample in.cub out.cub -zz 20 -1 21\n");
  printf("  swap left-right: resample in.cub out.cub -xx 63 -1 64\n");
  printf("  interpolate in-plane: resample in.cub out.cub -xx 0 .25 256 -yy 0 .25 256\n");
  printf("  crop to 40x64x21: resample in.cub out.cub -xx 12 1 40\n");
  printf("\n");
}
