
// vbmerge4d.cpp
// convert arbitrary 4D data to VoxBo TES1
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
#include "vbio.h"
#include "vbmerge4d.hlp.h"
#include "vbutil.h"

void vbmerge4d_help();
void vbmerge4d_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbmerge4d_help();
    exit(0);
  }
  arghandler ah;
  string errstr;
  ah.setArgs("-o", "--outfile", 1);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vbmerge4d: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vbmerge4d_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vbmerge4d_version();
    exit(0);
  }

  tokenlist patlist = ah.getUnflaggedArgs();
  tokenlist infilelist;
  // build infilelist
  for (size_t i = 0; i < patlist.size(); i++) {
    vglob vg(patlist[i]);
    for (size_t j = 0; j < vg.size(); j++) infilelist.Add(vg[j]);
  }
  string outfile;
  tokenlist args = ah.getFlaggedArgs("-o");
  if (args.size()) outfile = args[0];
  if (outfile.size() == 0 || infilelist.size() == 0) {
    printf("[E] vbmerge4d: requires both an input and an output file\n");
    exit(10);
  }

  Tes mytes;
  for (size_t i = 0; i < infilelist.size(); i++) {
    if (i == 0) {
      if (mytes.ReadFile(infilelist[i])) {
        printf("[E] vbmerge4d: couldn't read file %s\n", infilelist[i].c_str());
        exit(101);
      }
      continue;
    }
    Tes tespart;
    if (tespart.ReadFile(infilelist[i])) {
      printf("[E] vbmerge4d: couldn't read file %s\n", infilelist[i].c_str());
      exit(102);
    }
    if (mytes.MergeTes(tespart)) {
      printf("[E] vbmerge4d: error merging data from %s\n",
             infilelist[i].c_str());
      exit(103);
    }
  }
  mytes.SetFileName(outfile);
  if (mytes.WriteFile()) {
    printf("[E] vbmerge4d: error writing file %s\n", outfile.c_str());
    exit(110);
  } else
    printf("[I] vbmerge4d: done.\n");
  exit(0);
}

void vbmerge4d_help() { cout << boost::format(myhelp) % vbversion; }

void vbmerge4d_version() {
  printf("VoxBo vbmerge4d (v%s)\n", vbversion.c_str());
}
