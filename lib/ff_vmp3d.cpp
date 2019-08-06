
// ff_brainvoyager.cpp
// VoxBo file I/O code for BrainVoyager(tm) 3D format (version 2?)
// Copyright (c) 2005 by The VoxBo Development Team

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
// based on code by David Brainard

using namespace std;

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vbio.h"
#include "vbutil.h"

extern "C" {

vf_status test_vmp_3D(unsigned char *buf, int bufsize, string filename);
int write_vmp_3D(Cube *cb);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF vmp3d_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "BrainVoyager(tm) VMP";
  tmp.extension = "vmp";
  tmp.signature = "vmp";
  tmp.dimensions = 3;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_3D = test_vmp_3D;
  tmp.write_3D = write_vmp_3D;
  return tmp;
}

vf_status test_vmp_3D(unsigned char *, int, string) { return vf_no; }

// int
// read_head_vmp_3D(Cube *cb)
// {
//   return (0);    // no error!
// }

// int
// read_data_vmp_3D(Cube *cb)
// {
//   return(0);    // no error!
// }

struct BV_VMP_Header {
  short version;
  int nmaps;
  int maptype;
  int nlags;
  //{
  int clustersize;
  unsigned char enableclustercheck;
  float statthresh;
  float statcolorthresh;
  int df1;
  int df2;
  int bonferroni;
  unsigned char critR;
  unsigned char critG;
  unsigned char critB;
  unsigned char maxR;
  unsigned char maxG;
  unsigned char maxB;
  unsigned char enablesmpcolor;
  float transcolorfactor;
  //}
  int dimx;
  int dimy;
  int dimz;
  int xstart;
  int xend;
  int ystart;
  int yend;
  int zstart;
  int zend;
  int resolutionmm;
};

int write_vmp_3D(Cube *cb) {
  string imgname = xsetextension(cb->GetFileName(), "vmp");

  if (!cb->data_valid)  // only write the good stuff
    return (100);
  if (!cb->data)  // should never happen, but what the heck
    return (105);
  FILE *fp = fopen(imgname.c_str(), "w");
  if (!fp) return (110);

  BV_VMP_Header h1;

  h1.version = 2;
  h1.nmaps = 1;
  h1.maptype = 1;
  h1.nlags = 0;

  h1.clustersize = 50;
  h1.enableclustercheck = 0;
  h1.statthresh = 0;          // FIXME minval
  h1.statcolorthresh = 1000;  // FIXME maxval
  h1.df1 = 157;
  h1.df2 = 0;
  h1.bonferroni = 54228;
  h1.critR = 0;
  h1.critG = 0;
  h1.critB = 100;
  h1.maxR = 0;
  h1.maxG = 0;
  h1.maxB = 255;
  h1.enablesmpcolor = 0;
  h1.transcolorfactor = 1;

  h1.dimx = cb->dimx;
  h1.dimy = cb->dimy;
  h1.dimz = cb->dimz;
  //   h1.xstart=0-cb->origin[0];
  //   h1.xend=cb->dimx-1-cb->origin[0];
  //   h1.ystart=0-cb->origin[1];
  //   h1.yend=cb->dimy-1-cb->origin[1];
  //   h1.zstart=0-cb->origin[2];
  //   h1.zend=cb->dimz-1-cb->origin[2];
  h1.xstart = 0;
  h1.xend = cb->dimx - 1;
  h1.ystart = 0;
  h1.yend = cb->dimy - 1;
  h1.zstart = 0;
  h1.zend = cb->dimz - 1;
  h1.resolutionmm = 1;

  // top header
  fwrite(&(h1.version), sizeof(int16), 1, fp);
  fwrite(&(h1.nmaps), sizeof(int32), 1, fp);
  fwrite(&(h1.maptype), sizeof(int32), 1, fp);
  fwrite(&(h1.nlags), sizeof(int32), 1, fp);
  // header for each image (only one at the moment)
  fwrite(&(h1.clustersize), sizeof(int32), 1, fp);
  fwrite(&(h1.enableclustercheck), sizeof(char), 1, fp);
  fwrite(&(h1.statthresh), sizeof(float), 1, fp);
  fwrite(&(h1.statcolorthresh), sizeof(float), 1, fp);
  fwrite(&(h1.df1), sizeof(int32), 1, fp);
  fwrite(&(h1.df2), sizeof(int32), 1, fp);
  fwrite(&(h1.bonferroni), sizeof(int32), 1, fp);
  fwrite(&(h1.critR), sizeof(char), 1, fp);
  fwrite(&(h1.critG), sizeof(char), 1, fp);
  fwrite(&(h1.critB), sizeof(char), 1, fp);
  fwrite(&(h1.maxR), sizeof(char), 1, fp);
  fwrite(&(h1.maxG), sizeof(char), 1, fp);
  fwrite(&(h1.maxB), sizeof(char), 1, fp);
  fwrite(&(h1.enablesmpcolor), sizeof(char), 1, fp);
  fwrite(&(h1.transcolorfactor), sizeof(float), 1, fp);
  // the file name
  char fname[cb->GetFileName().size()];
  strcpy(fname, cb->GetFileName().c_str());
  fwrite(fname, strlen(fname) + 1, 1, fp);
  // the bottom of the header
  fwrite(&(h1.dimx), sizeof(int32), 1, fp);
  fwrite(&(h1.dimy), sizeof(int32), 1, fp);
  fwrite(&(h1.dimz), sizeof(int32), 1, fp);
  fwrite(&(h1.xstart), sizeof(int32), 1, fp);
  fwrite(&(h1.xend), sizeof(int32), 1, fp);
  fwrite(&(h1.ystart), sizeof(int32), 1, fp);
  fwrite(&(h1.yend), sizeof(int32), 1, fp);
  fwrite(&(h1.zstart), sizeof(int32), 1, fp);
  fwrite(&(h1.zend), sizeof(int32), 1, fp);
  fwrite(&(h1.resolutionmm), sizeof(int32), 1, fp);

  // the data
  int bytelen = cb->dimx * cb->dimy * cb->dimz;
  int cnt = fwrite(cb->data, cb->datasize, bytelen, fp);
  fclose(fp);
  if (cnt < bytelen) return (120);
  return (0);  // no error!
}

}  // extern "C"
