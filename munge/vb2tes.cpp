
// vb2tes.cpp
// convert 4D data to 4D file in supported formats (not just TES1)
// Copyright (c) 1998-2007 by The VoxBo Development Team

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
#include "vb2tes.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void vb2tes_help();
void vb2tes_version();
void read_multifile_pat(Tes &tes, string infile);
void read_multifile_list(Tes &tes, tokenlist &filelist);

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vb2tes_help();
    exit(0);
  }
  tokenlist args;
  string infile, outfile;
  set<int> includeset, excludeset;
  int floatflag = 0, nanflag = 0, multiflag = 0;

  arghandler ah;
  string errstr;
  ah.setArgs("-i", "--include", 1);
  ah.setArgs("-e", "--exclude", 1);
  ah.setArgs("-f", "--nofloat", 0);
  ah.setArgs("-m", "--multi", 1);
  ah.setArgs("-n", "--nonan", 0);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vb2tes: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vb2tes_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vb2tes_version();
    exit(0);
  }
  tokenlist filelist = ah.getUnflaggedArgs();
  if (ah.flagPresent("-f")) floatflag = 1;
  if (ah.flagPresent("-n")) nanflag = 1;

  args = ah.getFlaggedArgs("-i");
  if (args.size()) includeset = numberset(args[0]);
  args = ah.getFlaggedArgs("-e");
  if (args.size()) excludeset = numberset(args[0]);
  args = ah.getFlaggedArgs("-m");
  if (args.size()) {
    multiflag = 1;
    outfile = args[0];
  }
  if (filelist.size() != 2 && !multiflag) {
    printf("[E] vb2tes: requires an input and an output file\n");
    exit(10);
  }

  if (multiflag) {
    printf("[I] vb2tes: converting %s to TES file %s\n", infile.c_str(),
           outfile.c_str());
  } else {
    infile = filelist[0];
    outfile = filelist[1];
    printf("[I] vb2tes: converting %s to TES file %s\n", infile.c_str(),
           outfile.c_str());
  }

  Tes mytes;
  if (multiflag) {
    read_multifile_list(mytes, filelist);
  } else {
    if (mytes.ReadFile(infile)) {
      Cube mycube;
      if (!mycube.ReadFile(infile)) {
        mytes.SetVolume(mycube.dimx, mycube.dimy, mycube.dimz, 1,
                        mycube.datatype);
        mytes.SetCube(0, mycube);
        mytes.header = mycube.header;
        mytes.data_valid = 1;
      }
    }
    if (!mytes.data_valid) read_multifile_pat(mytes, infile);
  }

  if (!mytes.data_valid) {
    printf("[E] vb2tes: couldn't read 3D or 4D data from input\n");
    exit(5);
  }

  // merge include and exclude lists into just an include list
  if (includeset.size() && excludeset.size()) {
    for (set<int>::iterator i = excludeset.begin(); i != excludeset.end(); i++)
      includeset.erase(*i);
    excludeset.clear();
  }

  if (includeset.size()) mytes.resizeInclude(includeset);
  if (excludeset.size()) mytes.resizeExclude(excludeset);
  // remove NaNs and Infs if requested
  if (nanflag) mytes.removenans();

  // convert to float if requested
  if (floatflag && mytes.datatype != vb_float)
    mytes.convert_type(vb_float, VBSETALT | VBNOSCALE);

  mytes.fileformat = VBFF();
  if (mytes.WriteFile(outfile)) {
    printf("[E] vb2tes: error writing file %s\n", outfile.c_str());
    exit(110);
  } else
    printf("[I] vb2tes: done.\n");
  exit(0);
}

void read_multifile_pat(Tes &tes, string infile) {
  struct stat st;
  string pat;
  int dims_set = 0, ind = 0;

  if (!(stat(infile.c_str(), &st))) {
    if (S_ISDIR(st.st_mode)) pat = infile + "/*";
  } else
    pat = infile + "*";
  vglob vg(pat);
  if (vg.size() < 1) return;
  tokenlist fnames;
  for (size_t i = 0; i < vg.size(); i++) {
    if (xgetextension(vg[i]) != "hdr") fnames.Add(vg[i]);
  }
  for (size_t i = 0; i < fnames.size(); i++) {
    Cube cb;
    cb.ReadFile(fnames[i]);
    if (!cb.data_valid) {
      tes.invalidate();
      return;
    }
    if (!dims_set) {
      tes.SetVolume(cb.dimx, cb.dimy, cb.dimz, fnames.size() - i,
                    cb.GetDataType());
      tes.voxsize[0] = cb.voxsize[0];
      tes.voxsize[1] = cb.voxsize[1];
      tes.voxsize[2] = cb.voxsize[2];
      tes.filebyteorder = cb.filebyteorder;
      if (!tes.data) return;
      tes.data_valid = 1;
      dims_set = 1;
      tes.header = cb.header;
    }
    tes.SetCube(ind++, cb);
  }
}

void read_multifile_list(Tes &tes, tokenlist &filelist) {
  string pat;
  int dims_set = 0, ind = 0;

  for (int i = 0; i < (int)filelist.size(); i++) {
    Cube cb;
    cb.ReadFile(filelist[i]);
    if (!cb.data_valid) continue;
    if (!dims_set) {
      tes.SetVolume(cb.dimx, cb.dimy, cb.dimz, filelist.size(),
                    cb.GetDataType());
      tes.voxsize[0] = cb.voxsize[0];
      tes.voxsize[1] = cb.voxsize[1];
      tes.voxsize[2] = cb.voxsize[2];
      tes.filebyteorder = cb.filebyteorder;
      if (!tes.data) return;
      tes.data_valid = 1;
      dims_set = 1;
      tes.header = cb.header;
    }
    tes.SetCube(ind++, cb);
  }
}

void vb2tes_help() { cout << boost::format(myhelp) % vbversion; }

void vb2tes_version() { printf("VoxBo vb2tes (v%s)\n", vbversion.c_str()); }
