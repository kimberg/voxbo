
// extractmask.cpp
// extracts a mask from an atlas
// Copyright (c) 1998-2004 by The VoxBo Development Team

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
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "vbutil.h"
#include "vbio.h"
#include "extractmask.hlp.h"

class value {
public:
  double val;
  int cnt;
};

class Extractor {
public:
  Cube original;
  Cube newcube;
  int xval,xvflag;
  void init(tokenlist &args);
  void Go();
  double regioncode(int x,int y,int z);
  int newdim[3];
};

void
Extractor::init(tokenlist &args)
{
  // defaults
  xvflag=0;
  newdim[0]=41;
  newdim[1]=51;
  newdim[2]=27;

  newcube.voxsize[0]=3.75;
  newcube.voxsize[1]=3.75;
  newcube.voxsize[2]=5.00;

  newcube.origin[0]=21;
  newcube.origin[1]=31;
  newcube.origin[2]=10;

  original.ReadFile(args[0]);

  if (args.size()==1)
    newcube.SetFileName((string)"m"+original.GetFileName());

  if (args.size()==2 || args.size()==3) {
    newcube.SetFileName(args[1]);
    if (args.size()==3) {
      xval=strtol(args[2]);
      xvflag=1;
    }
  }

  if (args.size()==11 || args.size()==12) {
    newcube.SetFileName(args[1]);
    newdim[0]=strtol(args[2]);
    newdim[1]=strtol(args[3]);
    newdim[2]=strtol(args[4]);
    
    newcube.voxsize[0]=strtod(args[5]);
    newcube.voxsize[1]=strtod(args[6]);
    newcube.voxsize[2]=strtod(args[7]);
    
    newcube.origin[0]=strtol(args[8]);
    newcube.origin[1]=strtol(args[9]);
    newcube.origin[2]=strtol(args[10]);
    if (args.size()==12) {
      xval=strtol(args[11]);
      xvflag=1;
    }
  }
  newcube.SetVolume(newdim[0],newdim[1],newdim[2],original.datatype);
}

void extractmask_help();

int
main(int argc,char *argv[])
{
  tokenlist args;
  // extent relative to origin and voxel sizes of new file

  args.Transfer(argc-1,argv+1);
  if (args.size() < 1) {
    extractmask_help();
    exit(0);
  }
  Extractor ee;
  ee.init(args);
  // FIXME check for small (close to 0) voxel sizes
  ee.Go();
  exit(0);
}

void
Extractor::Go()
{
  double val;

  for (int i=0; i<newcube.dimx; i++) {
    for (int j=0; j<newcube.dimy; j++) {
      for (int k=0; k<newcube.dimz; k++) {
	val=regioncode(i,j,k);
	if (xvflag==0 || val==xval)
	  newcube.SetValue(i,j,k,val);
      }
    }
  }
  newcube.WriteFile();

  return;
}

double
Extractor::regioncode(int x,int y,int z)
{
  vector<value> valuelist;
  double x0,x1,y0,y1,z0,z1;
  int xx0,xx1,yy0,yy1,zz0,zz1;
  double val;
  int added;

  // xyz are the newcube coordinates of the voxel we'd like.  subtract
  // the origin of the new cube and divide by newvox to get that in mm
  // relative to the origin.

  x0=(double)(x-newcube.origin[0])*newcube.voxsize[0];
  x1=(double)((x+1)-newcube.origin[0])*newcube.voxsize[0];
  y0=(double)(y-newcube.origin[1])*newcube.voxsize[1];
  y1=(double)((y+1)-newcube.origin[1])*newcube.voxsize[1];
  z0=(double)(z-newcube.origin[2])*newcube.voxsize[2];
  z1=(double)((z+1)-newcube.origin[2])*newcube.voxsize[2];

  // cout << "x: " << x0 << ":" << x1 << endl;
  // cout << "y: " << y0 << ":" << y1 << endl;
  // cout << "z: " << z0 << ":" << z1 << endl;

  // now then divide by oldvox and subtract from original origin to
  // get original cube voxel.

  xx0=original.origin[0]+(int)(x0/original.voxsize[0]);
  xx1=original.origin[0]+(int)(x1/original.voxsize[0]);
  yy0=original.origin[1]+(int)(y0/original.voxsize[1]);
  yy1=original.origin[1]+(int)(y1/original.voxsize[1]);
  zz0=original.origin[2]+(int)(z0/original.voxsize[2]);
  zz1=original.origin[2]+(int)(z1/original.voxsize[2]);

  // cout << "coords: " << xx0 << ":" << xx1;
  // cout << ", " << yy0 << ":" << yy1;
  // cout << ", " << zz0 << ":" << zz1 << endl;
   
  for (int i=xx0; i<xx1; i++) {
    if (i<0 || i>=original.dimx)
      continue;
    for (int j=yy0; j<yy1; j++) {
      if (j<0 || j>=original.dimy)
	continue;
      for (int k=zz0; k<zz1; k++) {
	if (k<0 || k>=original.dimz)
	  continue;
	val=original.GetValue(i,j,k);
	added=0;
	for (int m=0; m<(int)valuelist.size(); m++) {
	  if (valuelist[m].val==val) {
	    valuelist[m].cnt++;
	    added=1;
	    break;
	  }
	}
	if (added==0) {
	  value tmp;
	  tmp.val=val;
	  tmp.cnt=1;
	  valuelist.push_back(tmp);
	}
	break;
      }
    }
  }
  // cout << "voxels: " << (xx1-xx0)*(yy1-yy0)*(zz1-zz0) << " ";
  // cout << "vals: " << valuelist.size() << " ";
  int commoncnt=0;
  double commonval=0.0;
  for (int i=0; i<(int)valuelist.size(); i++) {
    if (valuelist[i].cnt > commoncnt) {
      commonval=valuelist[i].val;
      commoncnt=valuelist[i].cnt;
    }
  }
  // cout << "common: " << commonval << "(" << commoncnt << ")" << endl;
  return (commonval);
}

void
extractmask_help()
{
  cout << boost::format(myhelp) % vbversion;
}
