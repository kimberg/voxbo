
// ff_roi.cpp
// VoxBo file I/O code for MRIcro's ROI format
// Copyright (c) 2007 by The VoxBo Development Team

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vbutil.h"
#include "vbio.h"

extern "C" {

#include "analyze.h"

vf_status test_roi_3D(unsigned char *buf,int bufsize,string filename);
int read_head_roi_3D(Cube *cb);
int read_data_roi_3D(Cube *cb);
int write_roi_3D(Cube *cb);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF roi_vbff()
#endif
{
  VBFF tmp;
  tmp.name="MRIcro roi";
  tmp.extension="roi";
  tmp.signature="roi";
  tmp.dimensions=3;
  tmp.version_major=vbversion_major;
  tmp.version_minor=vbversion_minor;
  tmp.test_3D=test_roi_3D;
  tmp.read_head_3D=read_head_roi_3D;
  tmp.read_data_3D=read_data_roi_3D;
  // tmp.write_3D=write_roi_3D;
  return tmp;
}

vf_status
test_roi_3D(unsigned char *,int,string filename)
{
  if (filename.find(".roi")!=string::npos)
    return vf_yes;
  return vf_no;
}

int
read_head_roi_3D(Cube *cb)
{
  // if we already have dims, great!
  if (cb->dimx>0 && cb->dimy>0 && cb->dimz>0)
    return 0;
  // see if there's a hdr file
  IMG_header ihdr;
  if (analyze_read_header(xsetextension(cb->GetFileName(),"hdr"),&ihdr,NULL))
    return 101;
  cb->dimx=ihdr.dim[1];
  cb->dimy=ihdr.dim[2];
  cb->dimz=ihdr.dim[3];
  cb->voxsize[0]=ihdr.pixdim[1];
  cb->voxsize[1]=ihdr.pixdim[2];
  cb->voxsize[2]=ihdr.pixdim[3];
  cb->SetDataType(vb_byte);
  if (cb->dimx>0 && cb->dimy>0 && cb->dimz>0)
    return 0;
  return 102;
}

int
read_data_roi_3D(Cube *cb)
{
  cb->SetVolume(cb->dimx,cb->dimy,cb->dimz,vb_byte);
  FILE *fp;
  cb->header_valid=0;
  fp = fopen(cb->GetFileName().c_str(),"r");
  if (!fp)
    return (100);
  cb->header.clear();
  uint16 shdr[2];  // slice number and nwords in slice
  uint16 sdat[2];  // voxel start, voxel count
  while(1) {
    if (fread(&shdr,sizeof(int16),2,fp) !=2) {
      fclose(fp);
      return(0);
    }
    if (my_endian()!=ENDIAN_LITTLE)
      swap(shdr,2);
    // shdr[0] is 1-indexed, so
    shdr[0]--;
    if (shdr[0] > cb->dimz-1 || shdr[0]>32767) {
      fclose(fp);
      return 104;
    }
    int xx,yy;
    for (int i=0; i<(shdr[1]-2)/2; i++) {
      if (fread(&sdat,sizeof(int16),2,fp) !=2) {
        fclose(fp);
        return(102);
      }
      // sdat[0] is 1-indexed, so
      if (my_endian()!=ENDIAN_LITTLE)
        swap(sdat,2);
      sdat[0]--;
      xx=sdat[0]%cb->dimx;
      yy=sdat[0]/cb->dimx;
      for (int j=0; j<sdat[1]; j++) {
        cb->SetValue(xx,yy,shdr[0],1);
        xx++;
        if (xx>cb->dimx-1) {
          xx=0;
          yy++;
        }
      }
    }
  }
  return 0;
}

int
write_roi_3D(Cube *)
{
  // FIXME later!
  return 1;
}

} // extern "C"
