
// ff_img3d.cpp
// VoxBo file I/O code for ANALYZE(tm) format
// Copyright (c) 1998-2006 by The VoxBo Development Team

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

#include "analyze.h"

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF img3d_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "Analyze 3D";
  tmp.extension = "img";
  tmp.signature = "img3d";
  tmp.dimensions = 3;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_3D = test_img3d;
  tmp.read_head_3D = read_head_img3d;
  tmp.read_data_3D = read_data_img3d;
  tmp.write_3D = write_img3d;
  return tmp;
}

}  // extern "C"
