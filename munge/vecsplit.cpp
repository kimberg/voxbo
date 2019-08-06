
// vecsplit.cpp
// little hack to bust up movement param files
// Copyright (c) 2004 by The VoxBo Development Team

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
#include "vbutil.h"
#include "vecsplit.hlp.h"

void vecsplit_help();

int main(int argc, char *argv[]) {
  tokenlist args;
  int ind;
  string filename;

  args.Transfer(argc - 1, argv + 1);

  if (args.size() == 0) {
    vecsplit_help();
    exit(0);
  }

  VB_Vector x, y, z, pitch, roll, yaw;
  for (size_t i = 0; i < args.size(); i++) {
    VB_Vector invec(args[i]);
    if (invec.getLength() < 7) continue;
    int points = invec.getLength() / 7;
    x.resize(points);
    y.resize(points);
    z.resize(points);
    pitch.resize(points);
    roll.resize(points);
    yaw.resize(points);
    for (int j = 0; j < points; j++) {
      ind = j * 7;
      x[j] = invec[ind + 0];
      y[j] = invec[ind + 1];
      z[j] = invec[ind + 2];
      pitch[j] = invec[ind + 3];
      roll[j] = invec[ind + 4];
      yaw[j] = invec[ind + 5];
    }

    filename = args[i];
    filename.erase(filename.size() - 4, 4);

    x.setFileName(filename + "_x.ref");
    x.WriteFile();

    y.setFileName(filename + "_y.ref");
    y.WriteFile();

    z.setFileName(filename + "_z.ref");
    z.WriteFile();

    pitch.setFileName(filename + "_pitch.ref");
    pitch.WriteFile();

    roll.setFileName(filename + "_roll.ref");
    roll.WriteFile();

    yaw.setFileName(filename + "_yaw.ref");
    yaw.WriteFile();
  }

  exit(0);
}

void vecsplit_help() { cout << boost::format(myhelp) % vbversion; }
