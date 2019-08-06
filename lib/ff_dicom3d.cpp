
// ff_dicom3d.cpp
// VoxBo I/O plug-in for DICOM format, with siemens extensions
// Copyright (c) 2003-2005 by The VoxBo Development Team

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
#include "vbio.h"
#include "vbutil.h"

extern "C" {

#include "dicom.h"

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF dcm3d_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "DICOM 3D";
  tmp.extension = "dcm";
  tmp.signature = "dcm3d";
  tmp.dimensions = 3;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_3D = test_dcm3d_3D;
  tmp.read_head_3D = read_head_dcm3d_3D;
  tmp.read_data_3D = read_data_dcm3d_3D;
  return tmp;
}

}  // extern "C"
