
// gerecon.cpp
// port of dave alsop's recon code for 5.x data
// Copyright (c) 1998-2000 by The VoxBo Development Team

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
// <kimberg@mail.med.upenn.edu>.

#include "vbcrunch.h"

class GERecon {
private:
  char refimage[STRINGLEN];
  char outfile[STRINGLEN];
  tokenlist args;
  CrunchCube *refcub,*sref,*mycub,*newcub;
  int startcub,i,o1,o2,o3;
  char paramfile[STRINGLEN];
  int maxiterations;
  double spikethresh;
  double driftthresh;
  double tolerance;
  Matrix moveparams;
  double minvoxsize;
  int maxdim;
  double tol_trans,tol_rot;
  Tes *mytes;                     // current tes file
  char timestring[STRINGLEN];     // text representation of time of realignment
public:
  Realignment(int argc,char **argv);
  ~Realignment();
  int Crunch();
  void Check();
  void WriteMoveParams(const char *tesname,char *outfile);
};

void gerecon_help();

int
main(int argc,char *argv[])
{
  tzset();                     // make sure all times are timezone corrected
  if (argc < 2) {              // not enough args, display autodocs
    gerecon_help();
    exit(0);
  }

  GERecon *r=new GERecon(argc,argv);    // init realign object
  if (!r) {
    printf("realign error: couldn't allocate a tiny realignment structure\n");
    exit(5);
  }
  int error = r->Crunch();                      // do the crunching
  delete r;                                     // clean up
  exit(error);
}

GERecon::GERecon(int argc,char *argv[])
{
  refimage[0]=0;
  maxiterations=MAXITERATIONS;
  spikethresh=SPIKETHRESH;
  driftthresh=DRIFTTHRESH;
  tolerance=TOLERANCE;

  for (i=1; i<argc; i++) {
    if (dancmp(argv[i],"-r")) {
      if (i < argc-1) {
	strncpy(refimage,argv[i+1],STRINGLEN);
	refimage[STRINGLEN-1]=0;
	i++;
      }
    }
    else if (dancmp(argv[i],"-p")) {
      if (i < argc-1) {
	strncpy(paramfile,argv[i+1],STRINGLEN);
	paramfile[STRINGLEN-1]=0;
	i++;
      }
    }
    else if (dancmp(argv[i],"-t")) {
      if (i < argc-1) {
	tolerance = strtod(argv[i+1],NULL);
	// should use some default if invalid
	i++;
      }
    }
    else if (dancmp(argv[i],"-i")) {
      if (i < argc-1) {
	maxiterations=strtol(argv[i+1],NULL,10);
	i++;
      }
    }
    else if (dancmp(argv[i],"-s")) {
      if (i < argc-1) {
	spikethresh=strtod(argv[i+1],NULL);
	refimage[STRINGLEN-1]=0;
	i++;
      }
    }
    else if (dancmp(argv[i],"-o")) {
      if (i < argc-1) {
	strncpy(outfile,argv[i+1],STRINGLEN);
	outfile[STRINGLEN-1]=0;
	i++;
      }
    }
    else if (dancmp(argv[i],"-d")) {
      if (i < argc-1) {
	driftthresh=strtod(argv[i+1],NULL);
	i++;
      }
    }
    else if (argv[i][0] != '-') {
      args.Add(argv[i]);
      continue;
    }
  }
}

GERecon::ReconFMRI()
{
  // convert the csi run with 
  // call rawfmri5x
}


GERecon::CSICor()
{
  short chopoff;
  float corrarr[xdim][ydim][nslices];

  if (hdr.rec.reps == 0 && hdr.image.im_psdname=="fmriepi")
    chopoff=0;
  else
    chopoff=1;
  
}

GERecon::ReconDir()
{
  // csicoords is a 3x3x100 array

  // grab header info
  // csi images are identified as 32-image files, from those we grab hdr.data_acq_tab(0:99).gw_points
  // we also grab the xdim, ydim, im_te, and csioffs=hdr.rec.user(19)-1000.0
  // call csicor() to convert the csi file
}


GERecon::~GERecon()
{
}

void
gerecon_help()
{
  printf("realign - another fine component of VoxBo!\n\n");
  printf("usage: realign [flags] file [file ...]\n");
  printf("flags:\n");
  printf("    -r <refimage>       refimage is the reference image\n");
  printf("    -o <outfile>        set the name for the realigned file\n");
}
