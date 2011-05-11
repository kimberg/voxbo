
// fillmask.cpp
// fill in missing slices in a mask (e.g., lesion)
// Copyright (c) 1998-2002 by The VoxBo Development Team

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
#include "fillmask.hlp.h"

void fillmask_help();
int getsampleslice(int start,int interval,int slice,int dimz);

int
main(int argc,char *argv[])
{
  tokenlist args;
  string infile,outfile;
  int start,interval,sampleslice;
  double val;

  args.Transfer(argc-1,argv+1);

  if (args.size() == 0) {
    fillmask_help();
    exit(0);
  }

  if (args.size() != 4) {
    fillmask_help();
    exit(5);
  }

  start=strtol(args[2]);
  interval=strtol(args[3]);
  printf("fillmasking %s to %s beginning slice %d and every %d...\n",args(0),args(1),start,interval);
  infile = args[0];
  outfile = args[1];

  Cube mycube,newcube;
  mycube.ReadFile(infile);
  if (!mycube.data_valid) {
    printf("\nCouldn't make a valid cube out of %s.\n",infile.c_str());
    exit(5);
  }
  // cout << mycube.GetFileType() << " " << t_img3d << endl;
  newcube=mycube;
  for (int k=0; k<mycube.dimz; k++) {
    sampleslice=getsampleslice(start,interval,k,mycube.dimz);
    for (int i=0; i<mycube.dimx; i++) {
      for (int j=0; j<mycube.dimy; j++) {
	val=mycube.GetValue(i,j,sampleslice);
	newcube.SetValue(i,j,k,val);
      }
    }
  }
  newcube.SetFileName(outfile);
  newcube.SetFileFormat("img3d");
  if (newcube.WriteFile())
    printf("Error writing file %s.\n",outfile.c_str());
  else
    printf("Done.\n");
  exit(0);
}

int
getsampleslice(int start,int interval,int slice,int dimz)
{
  for (int i=0; i<dimz; i++) {
    if (slice-i>=0 && slice-i <dimz)
      if ((((slice-i)-start)%interval)==0)
	return (slice-i);
    if (slice+i>=0 && slice+i <dimz)
      if ((((slice+i)-start)%interval)==0)
	return (slice+i);
  }
  return slice;
}

void
fillmask_help()
{
  cout << boost::format(myhelp) % vbversion;
}
