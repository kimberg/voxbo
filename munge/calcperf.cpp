
// calcperf.cpp
// calculate perfusion maps
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

using namespace std;

#include <stdio.h>
#include <string.h>
#include "calcperf.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void calcperf_help();
void calcperf_version();

int main(int argc, char *argv[]) {
  double TR = 4000;
  string maskname;
  vector<string> filelist;

  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-t" && i < args.size() - 1) {
      TR = strtod(args[++i]);
    } else if (args[i] == "-m" && i < args.size() - 1) {
      maskname = args[++i];
    } else if (args[i] == "-h") {
      calcperf_help();
      exit(0);
    } else if (args[i] == "-v") {
      calcperf_version();
      exit(0);
    } else
      filelist.push_back(args[i]);
  }

  if (filelist.size() == 0) {
    calcperf_help();
    exit(0);
  }

  if (filelist.size() > 1) {
    calcperf_help();
    exit(5);
  }

  Cube perfmap, wmask;
  perfmap.ReadFile(filelist[0]);
  if (!perfmap) {
    cout << "[E] calcperf: couldn't read " << filelist[0] << endl;
    exit(100);
  }
  wmask.ReadFile(maskname);
  if (!wmask) {
    cout << "[E] calcperf: couldn't read " << maskname << endl;
    exit(101);
  }
  if (perfmap.dimx != wmask.dimx || perfmap.dimy != wmask.dimy ||
      perfmap.dimz != wmask.dimz) {
    cout << "[E] calcperf: volumes have inconsistent dimensions" << endl;
    exit(102);
  }
  double sum = 0.0;
  int cnt = 0;
  for (int i = 0; i < perfmap.dimx; i++) {
    for (int j = 0; j < perfmap.dimy; j++) {
      for (int k = 0; k < perfmap.dimz; k++) {
        if (!wmask.testValue(i, j, k)) continue;
        sum += perfmap.GetValue(i, j, k);
        cnt++;
      }
    }
  }
  double m0blood = 1.28 * (sum / (double)cnt);
  double denom = 2.0 * 0.95 * m0blood * 0.7 * exp(0 - (TR / 1650));
  cout << denom << endl;

  exit(0);
}

void calcperf_help() { cout << boost::format(myhelp) % vbversion; }

void calcperf_version() { printf("VoxBo vbconv (v%s)\n", vbversion.c_str()); }
