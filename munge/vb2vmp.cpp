
// vb2vmp.cpp
// convert arbitrary 3D data to BrainVoyager(tm) vmp format
// Copyright (c) 2005 by The VoxBo Development Team

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
#include <string.h>
#include <sstream>
#include "vbutil.h"
#include "vbio.h"
#include "vb2vmp.hlp.h"

void vb2vmp_help();
void vb2vmp_version();

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vb2vmp_help();
    exit(0);
  }

  string infile,outfile;
  struct stat st;

  int nanflag=1;

  arghandler ah;
  string errstr;
  ah.setArgs("-n","--nonan",0);
  ah.setArgs("-h","--help",0);
  ah.setArgs("-v","--version",0);
  ah.parseArgs(argc,argv);

  if ((errstr=ah.badArg()).size()) {
    printf("[E] vb2vmp: %s\n",errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vb2vmp_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vb2vmp_version();
    exit(0);
  }
  tokenlist filelist=ah.getUnflaggedArgs();
  if (filelist.size()!=2) {
    printf("[E] vb2vmp: requires an input and an output file\n");
    exit(10);
  }
  if (ah.flagPresent("-n"))
    nanflag=1;

  infile = filelist[0];
  outfile = filelist[1];
  printf("[I] vb2vmp: converting %s to %s\n",infile.c_str(),outfile.c_str());

  printf("[I] vb2vmp: converting file %s to %s\n",infile.c_str(),outfile.c_str());

  if (stat(infile.c_str(),&st)) {
    printf("[E] vb2vmp: couldn't find input file %s.\n",infile.c_str());
    exit(5);
  }

  Cube mycube;
  if (mycube.ReadFile(infile.c_str())) {
    printf("[E] vb2vmp: couldn't read file %s as 3D data\n",infile.c_str());
    exit(5);
  }
  if (!mycube.data_valid) {
    printf("[E] vb2vmp: couldn't extract valid 3D data from file %s.\n",infile.c_str());
    exit(5);
  }

  // remove NaNs if requested
  if (nanflag)
    mycube.removenans();
  // convert to float if requested
  if (mycube.datatype != vb_float)
    mycube.convert_type(vb_float,VBSETALT|VBNOSCALE);

  if (mycube.SetFileFormat("vmp")) {
    printf("[E] vb2vmp: file format vmp not available\n");
    exit (106);
  }

  mycube.SetFileName(outfile);
  if (mycube.WriteFile()) {
    printf("[E] vb2vmp: error writing file %s\n",outfile.c_str());
    exit(110);
  }
  else
    printf("[I] vb2vmp: done.\n");
  exit(0);
}

void
vb2vmp_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vb2vmp_version()
{
  printf("VoxBo vb2vmp (v%s)\n",vbversion.c_str());
}
