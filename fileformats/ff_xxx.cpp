
// ff_nifti3d.cpp
// VoxBo file I/O code for nifti-1 format, 3D n+1 files
// Copyright (c) 2005-2006 by The VoxBo Development Team

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

#include "nifti.h"

vf_status xtest_n13d_3D(unsigned char *buf,int bufsize,string filename);
int xread_head_n13d_3D(Cube *cb);
int xread_data_n13d_3D(Cube *cb);
int xwrite_n13d_3D(Cube *cb);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF nifti3d_vbff()
#endif
{
  VBFF tmp;
  tmp.name="NIfTI 3D";
  tmp.extension="nii";
  tmp.signature="n13d";
  tmp.dimensions=3;
  tmp.version_major=vbversion_major;
  tmp.version_minor=vbversion_minor;
  tmp.test_3D=xtest_n13d_3D;
  tmp.read_head_3D=xread_head_n13d_3D;
  tmp.read_data_3D=xread_data_n13d_3D;
  tmp.write_3D=xwrite_n13d_3D;
  return tmp;
}

vf_status
xtest_n13d_3D(unsigned char *buf,int bufsize,string filename)
{
  string xt=xgetextension(filename);
  if (xt!="nii" && xt!= "img" && xt != "hdr")
    return vf_no;
  NIFTI_header nn;
  if (nifti_read_header(filename,&nn,NULL))
    return vf_no;
  if (nn.dim[0]==3)
    return vf_yes;
  return vf_no;
}

int
xread_head_n13d_3D(Cube *cb)
{
  int err=nifti_read_header(cb->GetFileName(),NULL,cb);
  return err;
}

int
xread_data_n13d_3D(Cube *cb)
{
  if (!cb->header_valid) {
    if (nifti_read_header(cb->GetFileName(),NULL,cb))
      return 101;
  }
  int err=nifti_read_3D_data(*cb);
  return err;
}

int
xwrite_n13d_3D(Cube *cb)
{
  if (!cb->data_valid)
      return 101;
  int err=nifti_write_3D(cb->GetFileName(),*cb);
  return err;
}

} // extern "C"
