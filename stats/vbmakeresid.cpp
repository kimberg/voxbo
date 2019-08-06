
// vbmakeresid.cpp
// build a residual volume for an existing GLM directory
// Copyright (c) 2004-2009 by The VoxBo Development Team

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
#include "glmutil.h"
#include "vbio.h"
#include "vbmakeresid.hlp.h"
#include "vbutil.h"

void vbmakeresid_help();
void vbmakeresid_version();

int main(int argc, char *argv[]) {
  string infile, outfile;
  stringstream tmps;
  vector<VBVoxel> vlist;
  string stem;
  tokenlist wtok;
  wtok.SetSeparator(" ,");
  vector<int> weights;

  tokenlist args;
  int nresids = -1;
  int stride = 1;

  arghandler ah;
  string errstr;
  ah.setArgs("-n", "--nresids", 1);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vbmakeresid: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vbmakeresid_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vbmakeresid_version();
    exit(0);
  }
  tokenlist filelist = ah.getUnflaggedArgs();
  if (filelist.size() != 2) {
    vbmakeresid_help();
    exit(11);
  }
  if (ah.flagPresent("-n")) {
    args = ah.getFlaggedArgs("-n");
    if (args.size() == 1) nresids = strtol(args[0]);
  }

  stem = filelist[0];
  outfile = filelist[1];

  GLMInfo gi;
  Tes prm, resid;
  VB_Vector sig;

  gi.setup(stem);
  string prmname = xsetextension(gi.stemname, "prm");
  prm.ReadHeader(prmname);
  sig = gi.getTS(0, 0, 0);
  uint32 ntimepoints = sig.getLength();

  if (nresids == -1 || nresids > (int)ntimepoints) {
    nresids = ntimepoints;
    stride = 1;
  } else {
    stride = ntimepoints / nresids;
  }

  if (prm.dimx < 1 || prm.dimy < 1 || prm.dimz < 1) {
    cout << "[E] vbmakeresid: bad dimensions for prm file" << endl;
    exit(101);
  }
  if (ntimepoints < 1) {
    cout << "[E] vbmakeresid: no time points found" << endl;
    exit(102);
  }

  resid.SetVolume(prm.dimx, prm.dimy, prm.dimz, nresids, vb_double);
  resid.print();
  for (int k = 0; k < prm.dimz; k++) {
    cout << "[I] vbmakeresid: calculating residuals for slice " << k << endl;
    for (int i = 0; i < prm.dimx; i++) {
      for (int j = 0; j < prm.dimy; j++) {
        if (!(prm.GetMaskValue(i, j, k))) continue;
        // cout << 2 << endl;
        sig = gi.getResid(i, j, k, gi.glmflags);
        if (sig.size() < ntimepoints) {
          printf("[I] vbmakeresid: couldn't get residuals\n");
          exit(110);
        }
        // cout << 3 << endl;
        int tt = 0;
        // cout << sig.size() << endl;
        // printf("%d %d %d %d\n",i,j,k,nresids);
        for (int ind = 0; ind < nresids; ind++) {
          // cout << tt << endl;
          resid.SetValue(i, j, k, ind, sig[tt]);
          tt += stride;
        }
        // cout << 4 << endl;
      }
    }
  }
  resid.SetFileName(outfile);
  char tmp[STRINGLEN];
  sprintf(tmp, "vbmakeresid: %d residuals, stride of %d", nresids, stride);
  resid.AddHeader(tmp);
  if (resid.WriteFile()) {
    cout << "[E] vbmakeresid: couldn't write file" << endl;
  } else {
    cout << "[I] vbmakeresid: done" << endl;
  }
  exit(0);
}

void vbmakeresid_help() { cout << boost::format(myhelp) % vbversion; }

void vbmakeresid_version() {
  printf("VoxBo vbmakeresid (v%s)\n", vbversion.c_str());
}
