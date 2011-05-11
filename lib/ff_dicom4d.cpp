
// ff_dicom4d.cpp
// VoxBo I/O plug-in for DICOM format, with siemens extensions
// Copyright (c) 2003 by The VoxBo Development Team

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
#include <unistd.h>
#include <ctype.h>
#include <sstream>
#include "vbutil.h"
#include "vbio.h"

extern "C" {

#include "dicom.h"

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF dcm4d_vbff()
#endif
{
  VBFF tmp;
  tmp.name="DICOM 4D";
  tmp.extension="dcm";
  tmp.signature="dcm4d";
  tmp.dimensions=4;
  tmp.f_fastts=0;
  tmp.version_major=vbversion_major;
  tmp.version_minor=vbversion_minor;
  tmp.test_4D=test_dcm4d_4D;
  tmp.read_head_4D=read_head_dcm4d_4D;
  tmp.read_data_4D=read_data_dcm4d_4D;
  return tmp;
}

} // extern "C"
