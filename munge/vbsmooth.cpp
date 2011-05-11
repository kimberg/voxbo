
// vbsmooth.cpp
// smoothing utility for voxbo
// Copyright (c) 2003-2004 by The VoxBo Development Team

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
#include "imageutils.h"
#include "vbversion.h"
#include <math.h>
#include <sstream>
#include "vbsmooth.hlp.h"

int vbsmooth_smooth(tokenlist &args);
int smooth_file(string infile,string ofile);
int vbsmooth_estimate(tokenlist &args);
void vbsmooth_help();
void vbsmooth_version();

int
main(int argc,char *argv[])
{
  tzset();                     // make sure all times are timezone corrected
  if (argc < 2) {              // not enough args, display autodocs
    vbsmooth_help();
    exit(0);
  }

  tokenlist args;
  args.Transfer(argc-1,argv+1);
  if (args[0]=="-v") {
    vbsmooth_version();
    exit(0);
  }
  if (args[0]=="-h") {
    vbsmooth_help();
    exit(0);
  }


  int err;
  if (args[0]=="-e")
    err=vbsmooth_estimate(args);
  else
    err=vbsmooth_smooth(args);

  exit(err);
}

// FIXME should class-ify this

Cube mask,remask;
double sx,sy,sz;
enum smode {vb_mm,vb_vox} mode;
string maskfile,remaskfile;
string prepend="s";
int sflag=0;

int
vbsmooth_smooth(tokenlist &args)
{
  vector<string> filelist;
  string outfile;

  // default smoothing kernel in vox
  mode=vb_vox;
  sx=sy=sz=3.0;

  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-mm" && i<args.size()-3) {
      mode=vb_mm;
      sx=strtod(args[i+1]);
      sy=strtod(args[i+2]);
      sz=strtod(args[i+3]);
      i+=3;
    }
    else if (args[i]=="-vox" && i<args.size()-3) {
      mode=vb_vox;
      sx=strtod(args[i+1]);
      sy=strtod(args[i+2]);
      sz=strtod(args[i+3]);
      i+=3;
    }
    else if (args[i]=="-m"&&i<args.size()-1) {
      maskfile=args[i+1];
      i++;
    }
    else if (args[i]=="-r"&&i<args.size()-1) {
      remaskfile=args[i+1];
      i++;
    }
    else if (args[i]=="-s"&&i<args.size()-1) {
      remaskfile=args[i+1];
      sflag=1;
      i++;
    }
    else if (args[i]=="-p"&&i<args.size()-1) {
      prepend=args[i+1];
      i++;
    }
    else if (args[i]=="-o" && i<args.size()-1) {
      outfile=args[i+1];
      i++;
    }
    else {
      filelist.push_back(args[i]);
    }
  }

  stringstream tmps;
  vector<VBFF>filetypes;
  Tes tes;

  if (sx<0 || sy<0 || sz<0) {
    tmps.str("");
    tmps << "vbsmooth: invalid smoothing kernel ";
    printErrorMsg(VB_ERROR,tmps.str());
    return 100;
  }

  if (maskfile.size()) {
    filetypes=EligibleFileTypes(maskfile);
    if (filetypes.size()<1) {
      printf("[E] vbsmooth: can't understand file %s",maskfile.c_str());
      return 100;
    }
    if (filetypes[0].getDimensions()==3)
      mask.ReadFile(maskfile);
    else {
      tes.ReadHeader(maskfile);
      if (tes.ExtractMask(mask)) {
        printf("[E] vbsmooth: can't extract mask from 4d file %s",maskfile.c_str());
        return 100;
      }
    }
  }
  if (remaskfile.size()) {
    filetypes=EligibleFileTypes(remaskfile);
    if (filetypes.size()<1) {
      printf("[E] vbsmooth: couldn't open (re)mask file %s\n",remaskfile.c_str());
      return 100;
    }
    if (filetypes[0].getDimensions()==3)
      remask.ReadFile(remaskfile);
    else {
      tes.ReadHeader(remaskfile);
      if (tes.ExtractMask(remask)) {
        printf("[E] vbsmooth: can't extract mask from 4d file %s\n",remaskfile.c_str());
        return 100;
      }
      remask.convert_type(vb_float,VBSETALT|VBNOSCALE);
    }
  }
  int err=0;
  for (int i=0; i<(int)filelist.size(); i++) {
    if (smooth_file(filelist[i],outfile))
      err++;
  }
  return err;
}

int
smooth_file(string infile,string ofile)
{
  stringstream tmps;
  vector<VBFF>filetypes;
  Cube cube;
  Tes tes;
  string outfile;

  if (ofile.size())
    outfile=ofile;
  else
    outfile=xdirname(infile)+(string)"/"+prepend+xfilename(infile);
  filetypes=EligibleFileTypes(infile);
  if (filetypes[0].getDimensions()==3) {
    if (cube.ReadFile(infile)) {
      tmps.str("");
      tmps << "vbsmooth: error reading " << infile;
      printErrorMsg(VB_ERROR,tmps.str());
      return 100;
    }
    if (mode==vb_mm) {
      sx/=cube.voxsize[0];
      sy/=cube.voxsize[1];
      sz/=cube.voxsize[2];
    }
    // if it's "fancy" remask, quantize to 1.0, smooth, thresh at 0.5, and quantize again
    if (sflag) {
      remask.quantize(1.0);
      smoothCube(remask,sx,sy,sz);
      remask.thresh(0.5);
      remask.quantize(1.0);
    } 
    printf("[I] vbsmooth: smoothing %s with a kernel (voxels) of %f,%f,%f\n",infile.c_str(),sx,sy,sz);

    if (mask.data)
      smoothCube_m(cube,mask,sx,sy,sz);
    else
      smoothCube(cube,sx,sy,sz);
    tmps.str("");
    tmps << "SpatialSmooth: " << timedate() << " " << sx << " " << sy << " " << sz;
    cube.AddHeader(tmps.str());
    cube.SetFileName(outfile);
    if (remask.data)
      cube.intersect(remask);
    if (cube.WriteFile()) {
      printf("[E] vbsmooth: error writing %s\n",infile.c_str());
      return 101;
    }
    else {
      printf("[I] vbsmooth: smoothed 4D data written to %s\n",outfile.c_str());
      return 0;
    }
  }
  else if (filetypes[0].getDimensions()==4) {
    if (tes.ReadFile(infile)) {
      printf("[E] vbsmooth: couldn't open file %s\n",infile.c_str());
      return 102;
    }
    if (mode==vb_mm) {
      sx/=tes.voxsize[0];
      sy/=tes.voxsize[1];
      sz/=tes.voxsize[2];
    }
    // if it's "fancy" remask, quantize to 1.0, smooth, thresh at 0.5, and quantize again
    if (sflag) {
      remask.quantize(1.0);
      smoothCube(remask,sx,sy,sz);
      remask.thresh(0.5);
      remask.quantize(1.0);
    }
    printf("[I] vbsmooth: smoothing %s with a kernel (voxels) of %f,%f,%f\n",infile.c_str(),sx,sy,sz);

    for (int i=0; i<tes.dimt; i++) {
      Cube cb;
      tes.getCube(i,cb);
      if (mask.data)
        smoothCube_m(cb,mask,sx,sy,sz);
      else
        smoothCube(cb,sx,sy,sz);
      if (remask.data)
        cb.intersect(remask);
      tes.SetCube(i,&cb);
    }
    tmps.str("");
    tmps << "SpatialSmooth: " << timedate() << " " << sx << " " << sy << " " << sz;
    tes.AddHeader(tmps.str());
    tes.SetFileName(outfile);
    if (tes.WriteFile()) {
      printf("[E] vbsmooth: error writing output file %s\n",outfile.c_str());
      return 103;
    }
    else {
      printf("[I] vbsmooth: smoothed 4D data written to %s\n",outfile.c_str());
    }
  }
  else {
    printf("[E] vbsmooth: couldn't open file %s\n",infile.c_str());
    return 0;
  }

  return 0;
}

int
vbsmooth_estimate(tokenlist &)
{
  printf("[E] vbsmooth: estimation not yet implemented\n");
  return 101;
}

void
vbsmooth_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbsmooth_version()
{
  printf("VoxBo vbsmooth (v%s)\n",vbversion.c_str());
}
