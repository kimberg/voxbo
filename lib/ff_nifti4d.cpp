
// ff_nifti4d.cpp
// VoxBo file I/O code for nifti-1 format, 4D n+1 files
// Copyright (c) 2005-2009 by The VoxBo Development Team

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

#include "nifti.h"

vf_status test_n14d_4D(unsigned char *buf, int bufsize, string filename);
int read_head_n14d_4D(Tes *ts);
int read_data_n14d_4D(Tes *ts, int start = -1, int count = -1);
int write_n14d_4D(Tes *ts);
int read_ts_n14d(Tes &mytes, int x, int y, int z);
int read_vol_n14d(Tes &ts, Cube &cb, int t);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF nifti4d_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "NIfTI 4D";
  tmp.extension = "nii";
  tmp.signature = "n14d";
  tmp.dimensions = 4;
  tmp.f_fastts = 0;
  tmp.f_headermask = 0;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_4D = test_n14d_4D;
  tmp.read_head_4D = read_head_n14d_4D;
  tmp.read_data_4D = read_data_n14d_4D;
  tmp.read_ts_4D = read_ts_n14d;
  tmp.read_vol_4D = read_vol_n14d;
  tmp.write_4D = write_n14d_4D;
  return tmp;
}

vf_status test_n14d_4D(unsigned char *, int, string filename) {
  string xt = xgetextension(filename);
  if (xt != "nii" && xt != "img" && xt != "hdr" && xt != "gz") return vf_no;
  NIFTI_header nn;
  int err = nifti_read_header(filename, &nn, NULL);
  if (err) return vf_no;
  if (nn.dim[0] == 4) return vf_yes;
  return vf_no;
}

int read_head_n14d_4D(Tes *ts) {
  int err = nifti_read_header(ts->GetFileName(), NULL, ts);
  if (err == 0) {
    ts->InitMask(1);
  }
  return err;
}

int read_data_n14d_4D(Tes *ts, int, int)  // start and count arguments removed
{
  if (!ts->header_valid) {
    if (nifti_read_header(ts->GetFileName(), NULL, ts)) return 101;
  }
  int err = nifti_read_4D_data(*ts);
  return err;
}

int read_ts_n14d(Tes &ts, int x, int y, int z) {
  if (!ts.header_valid) {
    if (nifti_read_header(ts.GetFileName(), NULL, &ts)) return 101;
  }
  int err = nifti_read_ts(ts, x, y, z);
  return err;
}

int read_vol_n14d(Tes &ts, Cube &cb, int t) {
  if (!ts.header_valid) {
    if (nifti_read_header(ts.GetFileName(), NULL, &ts)) return 101;
  }
  int err = nifti_read_vol(ts, cb, t);
  return err;
}

int write_n14d_4D(Tes *ts) {
  if (!ts->data_valid) return 101;
  int err = nifti_write_4D(ts->GetFileName(), *ts);
  return err;
  return 0;
}

}  // extern "C"
