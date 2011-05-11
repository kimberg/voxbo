
// dicominfo.cpp
// dump DICOM headers
// Copyright (c) 1998-2006 by The VoxBo Development Team

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
void dicominfo_help();
void dicominfo_version();

int
main(int argc,char **argv)
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  if (args.size()==0) {
    dicominfo_help();
    exit(0);
  }
  if (args[0]=="-h") {
    dicominfo_help();
    exit(0);
  }
  if (args[0]=="-v") {
    dicominfo_version();
    exit(0);
  }

  // dicominfo dci;
  for (size_t i=0; i<args.size(); i++)
    print_dicom_header(args[i]);
  exit(0);
}


void
dicominfo_help()
{
  printf("\nVoxBo dicominfo (v%s)\n",vbversion.c_str());
  printf("summary:\n");
  printf("  dump raw dicom fields in human-readable form\n");
  printf("usage:\n");
  printf("  dicominfo  <file>\n");
  printf("\n");
}

void
dicominfo_version()
{
  printf("VoxBo dicominfo (v%s)\n",vbversion.c_str());
}





}
