
// vb2img.cpp
// convert arbitrary 3D data to ANALYZE(tm) (aka .img) format
// Copyright (c) 1998-2004 by The VoxBo Development Team

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
#include "vb2img.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void vb2img_help();
void vb2img_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vb2img_help();
    exit(0);
  }

  string infile, outfile;
  struct stat st;

  int floatflag = 0, nanflag = 1, err;

  arghandler ah;
  string errstr;
  ah.setArgs("-f", "--nofloat", 0);
  ah.setArgs("-n", "--nonan", 0);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vb2img: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vb2img_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vb2img_version();
    exit(0);
  }
  tokenlist filelist = ah.getUnflaggedArgs();
  if (filelist.size() != 2) {
    printf("[E] vb2img: requires an input and an output file\n");
    exit(10);
  }
  if (ah.flagPresent("-f")) floatflag = 1;
  if (ah.flagPresent("-n")) nanflag = 1;

  infile = filelist[0];
  outfile = filelist[1];

  printf("[I] vb2img: converting file %s to %s\n", infile.c_str(),
         outfile.c_str());

  if (stat(infile.c_str(), &st)) {
    printf("[E] vb2img: couldn't find input file %s.\n", infile.c_str());
    exit(5);
  }

  Cube mycube;
  if ((err = mycube.ReadFile(infile.c_str()))) {
    printf("[E] vb2img: couldn't read file %s as 3D data (err %d)\n",
           infile.c_str(), err);
    exit(5);
  }
  if (!mycube.data_valid) {
    printf("[E] vb2img: couldn't extract valid 3D data from file %s.\n",
           infile.c_str());
    exit(5);
  }

  // remove NaNs if requested
  if (nanflag) mycube.removenans();
  // convert to float if requested
  if (floatflag && mycube.datatype != vb_float)
    mycube.convert_type(vb_float, VBSETALT | VBNOSCALE);

  if (mycube.SetFileFormat("img3d")) {
    printf("[E] vb2img: file format img1 not available\n");
    exit(106);
  }

  mycube.SetFileName(outfile);
  if (mycube.WriteFile()) {
    printf("[E] vb2img: error writing file %s\n", outfile.c_str());
    exit(110);
  } else
    printf("[I] vb2img: done.\n");
  exit(0);
}

void vb2img_help() { cout << boost::format(myhelp) % vbversion; }

void vb2img_version() { printf("VoxBo vb2img (v%s)\n", vbversion.c_str()); }
