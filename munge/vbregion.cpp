
// vbregion.cpp
// region of interest info for voxbo stat cubes
// Copyright (c) 2003-2008 by The VoxBo Development Team

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
#include <string.h>
#include <algorithm>
#include "vbio.h"
#include "vbregion.hlp.h"
#include "vbutil.h"

void vbregion_help();

int main(int argc, char *argv[]) {
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  if (args.size() < 1) {
    vbregion_help();
    exit(100);
  }

  string maskfile = "";
  double threshold = 0.0;
  list<string> cubelist;
  int f_thresh = 0, f_absthresh = 0, f_mask = 0;
  stringstream tmps;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-m" && i < args.size() - 1) {
      maskfile = args[i + 1];
      i++;
    } else if (args[i] == "-t" && i < args.size() - 1) {
      threshold = strtod(args[i + 1]);
      f_thresh = 1;
      i++;
    } else if (args[i] == "-a" && i < args.size() - 1) {
      threshold = strtod(args[i + 1]);
      f_absthresh = 1;
      i++;
    } else {
      cubelist.push_back(args[i]);
    }
  }

  Cube mycub, mask;

  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      printf("[E] vbregion: invalid mask volume %s\n", maskfile.c_str());
      exit(170);
    }
    f_mask = 1;
  }
  for (list<string>::iterator mm = cubelist.begin(); mm != cubelist.end();
       mm++) {
    if (mycub.ReadFile(*mm)) {
      printf("[E] vbregion: couldn't read cube %s\n", mm->c_str());
      continue;
    }
    if (f_mask) mycub.intersect(mask);
    vector<VBRegion> rlist;
    if (f_thresh)
      rlist = findregions(mycub, vb_gt, threshold);
    else if (f_absthresh)
      rlist = findregions(mycub, vb_agt, threshold);
    else
      rlist = findregions(mycub, vb_gt, 0.0);
    uint64 xx, yy, zz;
    double xxx, yyy, zzz, val;
    int index = 1;
    // make sure origin is sane for origin-corrected coordinates
    if (mycub.origin[0] == 0 && mycub.origin[1] == 0 && mycub.origin[2] == 0) {
      mycub.guessorigin();
      if (mycub.origin[0] || mycub.origin[1] || mycub.origin[2])
        printf(
            "[I] vbregion: guessing origin %d,%d,%d based on image "
            "dimensions\n",
            mycub.origin[0], mycub.origin[1], mycub.origin[2]);
    }
    printf("[I] vbregion: cube %s has %d regions\n", mm->c_str(),
           (int)(rlist.size()));
    if (!(mycub.voxsize[0] > 0.0 && mycub.voxsize[1] > 0.0 &&
          mycub.voxsize[2] > 0.0)) {
      printf(
          "[W] vbregion: no voxel sizes found, all origin-relative "
          "coordinates\n");
      printf("[W] vbregion: are in voxels, not mm.\n");
    }
    for (vector<VBRegion>::iterator rr = rlist.begin(); rr != rlist.end();
         rr++) {
      printf("region %d info:\n", index++);
      printf("  %d voxels\n", rr->size());
      rr->max(xx, yy, zz, val);

      printf("  max voxel at %d,%d,%d ", (int)xx, (int)yy, (int)zz);
      if (mycub.voxsize[0] > 0.0 && mycub.voxsize[1] > 0.0 &&
          mycub.voxsize[2] > 0.0)
        printf("(%.2f,%.2f,%.2f mm relative to origin), ",
               mycub.voxsize[0] * (xx - mycub.origin[0]),
               mycub.voxsize[1] * (yy - mycub.origin[1]),
               mycub.voxsize[2] * (zz - mycub.origin[2]));
      else
        printf("(%d,%d,%d voxels relative to origin), ",
               (int)(xx - mycub.origin[0]), (int)(yy - mycub.origin[1]),
               (int)(zz - mycub.origin[2]));
      printf("value is %g\n", val);

      rr->min(xx, yy, zz, val);
      printf("  min voxel at %d,%d,%d ", (int)xx, (int)yy, (int)zz);
      if (mycub.voxsize[0] > 0.0 && mycub.voxsize[1] > 0.0 &&
          mycub.voxsize[2] > 0.0)
        printf("(%.2f,%.2f,%.2f mm relative to origin), ",
               mycub.voxsize[0] * (xx - mycub.origin[0]),
               mycub.voxsize[1] * (yy - mycub.origin[1]),
               mycub.voxsize[2] * (zz - mycub.origin[2]));
      else
        printf("(%d,%d,%d voxels relative to origin), ",
               (int)(xx - mycub.origin[0]), (int)(yy - mycub.origin[1]),
               (int)(zz - mycub.origin[2]));
      printf("value is %g\n", val);

      rr->GeometricCenter(xxx, yyy, zzz);
      printf("  geometric center is at %.2f,%.2f,%.2f ", xxx, yyy, zzz);
      if (mycub.voxsize[0] > 0.0 && mycub.voxsize[1] > 0.0 &&
          mycub.voxsize[2] > 0.0)
        printf("(%.2f,%.2f,%.2f mm relative to origin), ",
               mycub.voxsize[0] * (xxx - mycub.origin[0]),
               mycub.voxsize[1] * (yyy - mycub.origin[1]),
               mycub.voxsize[2] * (zzz - mycub.origin[2]));
      else
        printf("(%.2f,%.2f,%.2f voxels relative to origin), ",
               (xxx - mycub.origin[0]), (yyy - mycub.origin[1]),
               (zzz - mycub.origin[2]));
      printf("value is %g\n", val);
    }
  }
  exit(0);
}

void vbregion_help() { cout << boost::format(myhelp) % vbversion; }
