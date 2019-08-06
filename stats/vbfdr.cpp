
// vbfdr.cpp
// calculate fdr thresholds
// Copyright (c) 2009 by The VoxBo Development Team

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

#include <stdio.h>
#include <string.h>
#include "stats.h"
#include "vbfdr.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void vbfdr_help();
void vbfdr_version();

int main(int argc, char *argv[]) {
  if (argc < 2) {
    vbfdr_help();
    exit(0);
  }

  tokenlist args;
  vector<string> filelist;
  args.Transfer(argc - 1, argv + 1);
  float q = 0;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-v")
      vbfdr_version();
    else if (args[i] == "-h")
      vbfdr_help();
    else if (args[i] == "-1" && i < args.size() - 1) {
      q = strtod(args[++i]);
      if (q <= 0.0) q = FLT_MIN;
      if (q >= 1.0) q = 1.0 - FLT_MIN;
    } else if (args[i] == "-q" && i < args.size() - 1) {
      q = strtod(args[++i]);
    } else
      filelist.push_back(args[i]);
  }
  if (filelist.empty()) {
    vbfdr_help();
    exit(112);
  }

  if (filelist.size() != 1) {
    vbfdr_help();
    exit(110);
  }

  Tes ts;
  if (ts.ReadFile(filelist[0])) {
    cout << format("[E] vbfdr: couldn't find 4D data in %s\n") % filelist[0];
    exit(150);
  }
  Cube cb;
  ts.getCube(0, cb);

  if (cb.get_minimum() < 0 || cb.get_maximum() > 1) {
    cout << format("[I] vbfdr: invalid range for p map\n");
  }

  VBVoxel vv = find_fdr_thresh(ts, q);
  if (vv.cool()) {
    for (int i = 1; i < ts.dimt; i++) {
      cout << format(
                  "[I] vbfdr: FDR value %g (must be equalled or exceeded)\n") %
                  ts.GetValue(vv.x, vv.y, vv.z, i);
    }
    cout << format("[I] vbfdr: this value found at %d,%d,%d\n") % vv.x % vv.y %
                vv.z;
  } else {
    cout << format("[I] vbfdr: no FDR value could be identified\n");
  }

  exit(0);
}

void vbfdr_help() { cout << boost::format(myhelp) % vbversion; }

void vbfdr_version() { printf("VoxBo vbfdr (v%s)\n", vbversion.c_str()); }
