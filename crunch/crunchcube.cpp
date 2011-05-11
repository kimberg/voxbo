
// crunchcube.cpp
// methods for the CrunchCube class, derived from Cube for purposes
//   of crunching
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

#include "vbcrunch.h"

CrunchCube::CrunchCube()
{
  Init();
}

void
CrunchCube::Init()
{
  transform.resize(6);
  transform.fill(0.0);
  dimx = dimy = dimz = 0;
  voxsize[0]=voxsize[1]=voxsize[2]=0.0;
  scl_slope=1.0;
  scl_inter=0.0;
  origin[0]=origin[1]=origin[2]=0;
  M.resize(0,0);
  savedM.resize(0,0);
  M1.resize(0,0);
  datasize=offset=voxels=0;
  datatype=vb_short;
  header.clear();
  data = (unsigned char *)NULL;
  header_valid=data_valid=0;
}

CrunchCube::CrunchCube(const Cube &old)
{
  Init();
  dimx = old.dimx;
  dimy = old.dimy;
  dimz = old.dimz;
  origin[0] = old.origin[0];
  origin[1] = old.origin[1];
  origin[2] = old.origin[2];
  scl_slope=old.scl_slope;
  scl_inter=old.scl_inter;
  voxsize[0] = old.voxsize[0];
  voxsize[1] = old.voxsize[1];
  voxsize[2] = old.voxsize[2];
  datatype = old.datatype;
  datasize = old.datasize;
  offset=old.offset;
  voxels = old.voxels;
  header_valid = old.header_valid;
  data_valid = old.data_valid;
  header = old.header;
  if (old.data && old.voxels > 0) {
    data = new unsigned char[voxels*datasize];
    if (!data) {
      fprintf(stderr,"vbcrunch failed to allocate space for a cube\n");
      exit(5);
    }
    memcpy(data,old.data,voxels*datasize);
  }
  else
    data = (unsigned char *)NULL;
}
