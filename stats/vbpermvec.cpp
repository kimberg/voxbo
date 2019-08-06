
// vbpermvec.cpp
// permute a vector using a perm mat
// Copyright (c) 2010 by The VoxBo Development Team

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
#include "vbio.h"
#include "vbpermvec.hlp.h"
#include "vbutil.h"

void vbpermvec_help();
void vbpermvec_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbpermvec_help();
    exit(0);
  }
  string infile, permfile, outfile;
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-i" && i < args.size() - 1)
      infile = args[++i];
    else if (args[i] == "-p" && i < args.size() - 1)
      permfile = args[++i];
    else if (args[i] == "-o" && i < args.size() - 1)
      outfile = args[++i];
    else {
      printf("[E] vbpermvec: unrecognized argument: %s\n", args(i));
      exit(5);
    }
  }
  if (permfile.empty() || infile.empty() || outfile.empty()) {
    printf("[E] vbpermvec: you must specify infile, permfile, and outfile\n");
    exit(5);
  }
  VBMatrix m;
  if (m.ReadFile(permfile)) {
    printf("[E] vbpermvec: invalid perm file\n");
    exit(10);
  }
  VB_Vector v;
  if (v.ReadFile(infile)) {
    printf("[E] vbpermvec: invalid input vector\n");
    exit(10);
  }
  if (m.m != v.size()) {
    printf("[E] vbpermvec: mismatched sizes\n");
    exit(10);
  }
  for (size_t i = 0; i < m.n; i++) {
    VB_Vector tmpv = v;
    v.permute(m, i);
    string filename = outfile;
    string inds = (format("%08d") % i).str();
    replace_string(filename, "IND", inds);
    tmpv.WriteFile(filename);
  }

  exit(0);
}

void vbpermvec_help() { cout << boost::format(myhelp) % vbversion; }

void vbpermvec_version() {
  printf("VoxBo vbpermvec (v%s)\n", vbversion.c_str());
}
