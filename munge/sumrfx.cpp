
// sumrfx.cpp
// do some simple things with 4D rfx volumes
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "sumrfx.hlp.h"
#include "vbio.h"
#include "vbutil.h"

int process_singlemask(vector<VBRegion> &rlist, string fname);
int process_multimask(vector<VBRegion> &rlist, string fname);

void do_sumcube(string fname, int index, Cube &mycube, Cube &mask,
                double thresh, vector<VBRegion> &rlist);
void do_sumrfx(const string &fname, const string &mname, double thresh,
               vector<VBRegion> &rlist);
void sumrfx_help();

int detailflag = 0;
int compareflag = 0;
int nozeroflag = 0;

int main(int argc, char *argv[]) {
  tokenlist args;
  double thresh = 0.0;
  string maskfile;
  string datafile;
  vector<string> singlemasklist;
  vector<string> multimasklist;

  args.Transfer(argc - 1, argv + 1);

  if (args.size() == 0) {
    sumrfx_help();
    exit(0);
  }

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-t" && (i + 1) < args.size()) {
      thresh = strtod(args[i + 1]);
      i++;
      continue;
    } else if (args[i] == "-m" && (i + 1) < args.size()) {
      maskfile = args[i + 1];
      i++;
      continue;
    } else if (args[i] == "-d") {
      detailflag = 1;
      continue;
    } else if (args[i] == "-c") {
      compareflag = 1;
      continue;
    } else if (args[i] == "-nz") {
      nozeroflag = 1;
      continue;
    } else if (args[i] == "-sm" && (i + 1) < args.size()) {
      singlemasklist.push_back(args[i + 1]);
      i++;
      continue;
    } else if (args[i] == "-mm" && (i + 1) < args.size()) {
      multimasklist.push_back(args[i + 1]);
      i++;
      continue;
    } else {
      datafile = args[i];
    }
  }
  vector<VBRegion> rlist;
  for (size_t i = 0; i < multimasklist.size(); i++)
    process_multimask(rlist, multimasklist[i]);
  for (size_t i = 0; i < singlemasklist.size(); i++)
    process_singlemask(rlist, singlemasklist[i]);

  do_sumrfx(datafile, maskfile, thresh, rlist);

  exit(0);
}

int process_singlemask(vector<VBRegion> &rlist, string fname) {
  Cube mask;
  if (mask.ReadFile(fname)) {
    return 101;
  }
  VBRegion reg;
  reg = findregion_mask(mask, vb_gt, 0.0);
  reg.name = xfilename(fname);
  if (!(reg.voxels.size())) return 102;
  rlist.push_back(reg);
  return 0;
}

int process_multimask(vector<VBRegion> &rlist, string fname) {
  Cube mask;
  tokenlist args;
  if (mask.ReadFile(fname)) {
    return 101;
  }
  for (int i = 0; i < (int)mask.header.size(); i++) {
    args.ParseLine(mask.header[i]);
    if (args[0] != "vb_maskspec:") continue;
    int value = strtol(args[1]);
    VBRegion reg;
    reg = findregion_mask(mask, vb_eq, (double)value);
    if (reg.voxels.size()) rlist.push_back(reg);
  }
  return 0;
}

void do_sumrfx(const string &fname, const string &mname, double thresh,
               vector<VBRegion> &rlist) {
  int i, j, k;
  Tes rfx;
  Cube mask;

  rfx.ReadFile(fname);
  if (!rfx.data_valid) {
    printf("sumrfx: invalid 4D file %s.\n", fname.c_str());
    return;
  }
  // specified mask, intersect with tes mask
  if (mname.size()) {
    mask.ReadFile(mname);
    for (i = 0; i < mask.dimx; i++)
      for (j = 0; j < mask.dimy; j++)
        for (k = 0; k < mask.dimz; k++)
          if (rfx.GetMaskValue(i, j, k) < 0.5) mask.SetValue(i, j, k, 0);
  }
  // default mask
  else {
    mask.SetVolume(rfx.dimx, rfx.dimy, rfx.dimz, vb_byte);
    for (i = 0; i < mask.dimx; i++)
      for (j = 0; j < mask.dimy; j++)
        for (k = 0; k < mask.dimz; k++)
          mask.SetValue(i, j, k, rfx.GetMaskValue(i, j, k));
  }
  if (!mask.data_valid) {
    printf("sumrfx: invalid mask file %s.\n", mname.c_str());
    return;
  }

  if (mask.dimx != rfx.dimx || mask.dimy != rfx.dimy || mask.dimz != rfx.dimz) {
    printf("sumrfx: mask and rfx volume dimensions don't match\n");
    return;
  }

  printf("sumrfx: thresh %.2f mask file: %s\n", thresh, mname.c_str());

  for (i = 0; i < rfx.dimt; i++) {
    Cube mycube;
    rfx.getCube(i, mycube);
    if (!mycube.data_valid) continue;
    do_sumcube(rfx.GetFileName(), i, mycube, mask, thresh, rlist);
  }
}

void do_sumcube(string fname, int index, Cube &mycube, Cube &mask,
                double thresh, vector<VBRegion> &rlist) {
  Cube tmpmask = mask;
  int i, j, k;

  printf("%s-%02d\n", fname.c_str(), index);
  // eliminate from mask any sub-threshold voxels
  for (i = 0; i < mycube.dimx; i++) {
    for (j = 0; j < mycube.dimy; j++) {
      for (k = 0; k < mycube.dimz; k++) {
        if (mycube.GetValue(i, j, k) < thresh) tmpmask.SetValue(i, j, k, 0);
      }
    }
  }

  // now find regions
  vector<VBRegion> regionlist;
  regionlist = findregions(tmpmask, vb_gt, 0.0);

  // now that we have a list of regions we can go through them and produce some
  // stats
  double grandsum = 0.0;
  double val;
  int grandcnt = 0;
  for (i = 0; i < (int)regionlist.size(); i++) {
    double sum = 0.0, x = 0.0, y = 0.0, z = 0.0;
    int cnt = 0;
    for (VI myvox = regionlist[i].begin(); myvox != regionlist[i].end();
         myvox++) {
      if (val == 0.0 && nozeroflag) continue;
      sum += val;
      grandsum += val;
      x += myvox->second.x;
      y += myvox->second.y;
      z += myvox->second.z;
      cnt++;
      grandcnt++;
    }
    if (detailflag)
      printf("   region %d: %d voxels, mean %.2f (%d,%d,%d)\n", i, cnt,
             sum / (double)cnt, (int)x / cnt, (int)y / cnt, (int)z / cnt);
  }
  printf("  sum: %.3f  count: %d  average(sum/cnt): %.6f\n", grandsum, grandcnt,
         grandsum / (double)grandcnt);

  // now do the region comparisons
  if (rlist.size()) {
    // build a vb_vector for each regionlist
    vector<VB_Vector> myvecs;
    for (i = 0; i < (int)rlist.size(); i++) {
      vector<double> vals;
      for (VI myvox = rlist[i].begin(); myvox != rlist[i].end(); myvox++) {
        val =
            mycube.GetValue(myvox->second.x, myvox->second.y, myvox->second.z);
        if (val == 0.0 && nozeroflag) continue;
        if (val < thresh) continue;
        vals.push_back(val);
      }
      VB_Vector tmpv(vals.size());
      for (j = 0; j < (int)vals.size(); j++) tmpv[j] = vals[j];
      myvecs.push_back(tmpv);
    }
    for (i = 0; i < (int)rlist.size() - 1; i++) {
      for (j = i + 1; j < (int)rlist.size(); j++) {
        double tvalue = ttest(myvecs[i], myvecs[j]);
        cout << "  region comparison: " << rlist[i].name << " vs. "
             << rlist[j].name << endl;
        cout << "    means: " << myvecs[i].getVectorMean() << " vs. "
             << myvecs[j].getVectorMean() << endl;
        cout << "    voxels: " << myvecs[i].getLength() << " vs. "
             << myvecs[j].getLength() << endl;
        cout << "    t value= " << tvalue << endl;
      }
    }
  }
}

void sumrfx_help() { cout << boost::format(myhelp) % vbversion; }
