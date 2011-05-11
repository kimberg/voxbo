
// vbmaskmunge.cpp
// mask munging util for voxbo
// Copyright (c) 2003-2007 by The VoxBo Development Team

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

#include <math.h>
#include <sstream>
#include "vbutil.h"
#include "vbio.h"
#include "imageutils.h"
#include "vbversion.h"
#include "vbmaskmunge.hlp.h"

void vbmaskmunge_help();
void vbmaskmunge_version();

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vbmaskmunge_help();
    exit(0);
  }
  arghandler ah;
  string errstr;
  ah.setArgs("-s","--smooth",3);
  ah.setArgs("-t","--thresh",1);
  ah.setArgs("-x","--cutoff",1);
  ah.setArgs("-c","--combine",0);
  ah.setArgs("-u","--union",0);
  ah.setArgs("-n","--count",0);
  ah.setArgs("-i","--intersect",0);
  ah.setArgs("-q","--quantize",0);
  ah.setArgs("-z","--zzinvert",0);
  ah.setArgs("-d","--diff",0);
  ah.setArgs("-r","--ratio",0);
  ah.setArgs("-m","--multiply",0);
  ah.setArgs("-p","--prepend",1);
  ah.setArgs("-a","--apply",1);
  ah.setArgs("-o","--output",1);
  ah.setArgs("-h","--help",0);
  ah.setArgs("-v","--version",0);
  ah.parseArgs(argc,argv);

  if ((errstr=ah.badArg()).size()) {
    cout << "[E] vbmaskmunge: " << errstr << endl;
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vbmaskmunge_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vbmaskmunge_version();
    exit(0);
  }

  tokenlist filelist=ah.getUnflaggedArgs();
  enum {mm_none,mm_combine,mm_union,mm_intersect,mm_sum,mm_diff,mm_ratio,mm_product};

  tokenlist args;
  string outx,prepend="m",target;
  int smoothflag=0,threshflag=0,outfileflag=0,quantizeflag=0,applyflag=0,invertflag=0;
  int cutoffflag=0;
  int combinemode=mm_none;
  int quantizeval=1;
  double sx=0,sy=0,sz=0,thresh=0.0,cutoff=0.0;
  args=ah.getFlaggedArgs("-o");
  if (args.size()) {
    outfileflag=1;
    outx=args[0];
  }
  args=ah.getFlaggedArgs("-s");
  if (args.size()) {
    smoothflag=1;
    sx=strtod(args[0]);
    sy=strtod(args[1]);
    sz=strtod(args[2]);
  }
  args=ah.getFlaggedArgs("-t");
  if (args.size()) {
    threshflag=1;
    thresh=strtod(args[0]);
  }
  args=ah.getFlaggedArgs("-x");
  if (args.size()) {
    cutoffflag=1;
    cutoff=strtod(args[0]);
  }
  args=ah.getFlaggedArgs("-p");
  if (args.size()) {
    prepend=args[0];
  }
  if (ah.flagPresent("-c")) {
    quantizeflag=1;
    combinemode=mm_combine;
  }
  if (ah.flagPresent("-u")) {
    combinemode=mm_union;
  }
  if (ah.flagPresent("-n")) {
    quantizeflag=1;
    combinemode=mm_sum;
  }
  if (ah.flagPresent("-d")) {
    quantizeflag=0;
    combinemode=mm_diff;
  }
  if (ah.flagPresent("-r")) {
    quantizeflag=0;
    combinemode=mm_ratio;
  }
  if (ah.flagPresent("-m")) {
    quantizeflag=0;
    combinemode=mm_product;
  }
  if (ah.flagPresent("-i")) {
    combinemode=mm_intersect;
  }
  if (ah.flagPresent("-q"))
    quantizeflag=1;
  if (ah.flagPresent("-z"))
    invertflag=1;
  args=ah.getFlaggedArgs("-a");
  if (args.size()) {
    applyflag=1;
    combinemode=mm_intersect;
    target=args[0];
  }

  Cube master;
  for (size_t i=0; i<filelist.size(); i++) {
    // work out the input and output filenames
    string infile=filelist[i];
    Cube cb;
    cb.ReadFile(infile);
    if (!cb.data) {
      Tes ts;
      if (!(ts.ReadFile(infile)))
        ts.ExtractMask(cb);
    }
    if (!cb.data) {
      cout << "[E] vbmaskmunge: error reading input file " << infile << ", ignoring" << endl;
      continue;
    }
    if (smoothflag)
      smoothCube(cb,sx,sy,sz);
    if (threshflag)
      cb.thresh(thresh);
    if (cutoffflag)
      cb.cutoff(cutoff);
    if (quantizeflag)
      cb.quantize(quantizeval);
    if (invertflag)
      cb.invert();
    string outfile;
    if (outfileflag)
      outfile=outx;
    else
      outfile=xdirname(infile)+(string)"/"+prepend+xfilename(infile);
    cb.SetFileName(outfile);

    if (combinemode==mm_none) {
      stringstream tmps;
      tmps.str("");
      tmps << "vbmaskmunge_date: " << timedate();
      cb.AddHeader(tmps.str());
      
      if (smoothflag) {
        tmps.str("");
        tmps << "vbmaskmunge_smooth: " << sx << " " << sy << " " << sz;
        cb.AddHeader(tmps.str());
      }
      if (threshflag) {
        tmps.str("");
        tmps << "vbmaskmunge_thresh: " << thresh;
        if (quantizeflag) tmps << " (quantized)";
        cb.AddHeader(tmps.str());
      }
      if (cb.WriteFile())
        cout << "[E] vbmaskmunge: error writing file " << outfile << endl;
      else
        cout << "[I] vbmaskmunge: wrote file " << outfile << endl;
    }
    else if (combinemode==mm_intersect) {
      if (i==0) {
        master=cb;
      }
      else
        master.intersect(cb);
    }
    else if (combinemode==mm_sum) {
      if (i==0) {
        master=cb;
      }
      else
        master+=cb;
    }
    else if (combinemode==mm_diff) {
      if (i==0) {
        master=cb;
      }
      else
        master-=cb;
    }
    else if (combinemode==mm_ratio) {
      if (i==0) {
        master=cb;
      }
      else
        master/=cb;
    }
    else if (combinemode==mm_product) {
      if (i==0) {
        master=cb;
      }
      else
        master*=cb;
    }
    else if (combinemode==mm_union) {
      if (i==0) {
        master=cb;
      }
      else
        master.unionmask(cb);
    }
    else if (combinemode==mm_combine) {
      if (i==0) {
        master=cb;
        quantizeval++;
      }
      else {
        master.unionmask(cb);
        quantizeval++;
      }
    }
  }
  if (applyflag) {
    string outfile;
    if (outfileflag)
      outfile=outx;
    else
      outfile=xdirname(target)+(string)"/"+prepend+xfilename(target);
    Cube cb;
    Tes ts;
    if (!(cb.ReadFile(target))) {
      cb.intersect(master);
      cb.SetFileName(outfile);
      if (cb.WriteFile())
        cout << "[E] vbmaskmunge: error writing file " << outfile << endl;
      else
        cout << "[I] vbmaskmunge: wrote file " << outfile << endl;
    }
    else if (!(ts.ReadFile(target))) {
      ts.intersect(master);
      ts.SetFileName(outfile);
      if (ts.WriteFile())
        cout << "[E] vbmaskmunge: error writing file " << outfile << endl;
      else
        cout << "[I] vbmaskmunge: wrote file " << outfile << endl;
    }
  }
  else if (combinemode!=mm_none) {
    string outfile;
    string infile=filelist[0];
    if (outfileflag)
      outfile=outx;
    else
      outfile=xdirname(infile)+(string)"/"+prepend+xfilename(infile);
    master.SetFileName(outfile);
    if (master.WriteFile())
      cout << "[E] vbmaskmunge: error writing file " << outfile << endl;
    else
      cout << "[I] vbmaskmunge: wrote file " << outfile << endl;
  }
  exit(0);
}

void
vbmaskmunge_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbmaskmunge_version()
{
  printf("VoxBo vbmaskmunge (v%s)\n",vbversion.c_str());
}
