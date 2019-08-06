
// vb_nifti11.h
// VoxBo I/O support for NIFTI-1.1 format
// Copyright (c) 2005-2006 by The VoxBo Development Team

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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include "nifti11_ref.h"
#include "vbio.h"
#include "vbutil.h"

typedef struct nifti_1_header NIFTI_header;

const int NIFTI_MIN_OFFSET = 352;
const uint32 NIFTI_ECODE_VOXBO = 28;

// int nifti_test(string &fname,NIFTI_header hdr,Cube &cb);
int nifti_read_header(string fname, NIFTI_header *xhdr, VBImage *vol);
void nifti_from_VB_datatype(NIFTI_header &hdr, const VB_datatype datatype);
int nifti_write_3D(string fname, Cube &cb);
int nifti_write_4D(string fname, Tes &im);
int nifti_read_3D_data(Cube &cb);
int nifti_read_4D_data(Tes &ts, int start = -1, int count = -1);
void nifti_swap_header(NIFTI_header &hdr);
int nifti_read_vol(Tes &ts, Cube &cb, int t);
int nifti_read_ts(Tes &ts, int x, int y, int z);
void voxbo2nifti_header(VBImage &im, NIFTI_header &hdr);
string nifti_typestring(int16 dt);
void print_nifti_header(NIFTI_header &ihead);
