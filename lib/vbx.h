
// vbx.h
// headers for job-running stuff
// Copyright (c) 1998-2005 by The VoxBo Development Team

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

#include "vbutil.h"
#include "vbprefs.h"
#include "vbjobspec.h"

#define MAXFILENAME 128
#define MAXHOSTS 64                   // maximum number of eligible hosts
#define MAXFILES 512                  // maximum number of files in a job
#define MAXTAGLEN 32                  // maximum length of scan tag in chars

// defined in system libraries
extern char **environ;
// defined in same file as main()
//extern VBPrefs vbp;

// vbx.cpp (job execution primitives)
int runseq(VBPrefs &vbp,VBSequence &seq,uint32 njobs);
set<int32> readyjobs(VBSequence &seq,uint16 max);
int run_voxbo_job(VBPrefs &vbp,VBJobSpec &js);
void tell_scheduler(string qdir,string hostname,string buf);
