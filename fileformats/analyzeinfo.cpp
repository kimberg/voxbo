
// analyzeinfo.cpp
// dump analyze headers
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


#include "analyze.h"
#include "nifti.h"

void analyzeinfo_help();
void analyzeinfo_version();

int
main(int argc,char **argv)
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  if (args.size()==0) {
    analyzeinfo_help();
    exit(0);
  }
  if (args[0]=="-h") {
    analyzeinfo_help();
    exit(0);
  }
  if (args[0]=="-v") {
    analyzeinfo_version();
    exit(0);
  }
  IMG_header ihead;
  NIFTI_header nh;
  Cube cb;
  for (size_t i=0; i<args.size(); i++) {
    cout << format("[I] file %s:\n")%args[i];
    if (nifti_read_header(args[i],&nh,NULL)==0)
      print_nifti_header(nh);
    else if (analyze_read_header(args[i],&ihead,NULL)==0)
      print_analyze_header(ihead);
    else {
      cout << format("[E] %s: couldn't read %s as an Analyze or a NIfTI file\n")
        % argv[0] % args[i];
    }
    cout << endl;
  }
  exit(0);
}

void
analyzeinfo_help()
{
  printf("\nVoxBo analyzeinfo (v%s)\n",vbversion.c_str());
  printf("summary:\n");
  printf("  dump raw analyze fields in human-readable form\n");
  printf("usage:\n");
  printf("  analyzeinfo  <file>\n");
  printf("\n");
}

void
analyzeinfo_version()
{
  printf("VoxBo analyzeinfo (v%s)\n",vbversion.c_str());
}



} // extern "C"

