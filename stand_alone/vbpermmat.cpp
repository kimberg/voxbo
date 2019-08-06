
// vbpermmat.cpp
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
#include "perm.h"
#include "vbio.h"
#include "vbutil.h"

void vbpermmat_help();
void vbpermmat_version();

gsl_rng *theRNG = NULL;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbpermmat_help();
    exit(0);
  }
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  if (args.size() != 4) {
    printf("[E] vbppermmat: incorrect number of arguments\n");
    exit(100);
  }
  string filename = args[0];
  int nperms = strtol(args[1]);
  int ndata = 0;
  string type = vb_tolower(args[3]);

  VB_Vector vv;
  Tes ts;
  if (!vv.ReadFile(args[2])) {
    if (vv.size() > 1) ndata = vv.size();
  } else if (!ts.ReadHeader(args[2])) {
    if (ts.dimt > 1) ndata = ts.dimt;
  }
  if (ndata == 0) ndata = strtol(args[2]);

  VB_permtype method;
  if (type == "sign")
    method = vb_signperm;
  else if (type == "data" || type == "order")
    method = vb_orderperm;
  else {
    printf("[E] vbpermmat: unrecognized permutation type %s\n", type.c_str());
    exit(102);
  }
  VBMatrix pmat = createPermMatrix(nperms, ndata, method, 0);
  if (pmat.m == 1) {
    printf("[E] vbpermmat: error creating permutation matrix\n");
    exit(106);
  }
  if (pmat.WriteFile(filename)) {
    printf("[E] vbpermmat: error writing permutation matrix\n");
    exit(106);
  }
  printf("[I] vbpermmat: permutation matrix %s written\n", filename.c_str());

  exit(0);
}

void vbpermmat_help() {
  printf("\nVoxBo vbpermmat (v%s)\n", vbversion.c_str());
  printf("summary:\n");
  printf("  generate a matrix that defines permutations:\n");
  printf("usage:\n");
  printf("  vbpermmat <file> <nperms> <ndata> <type>\n");
  printf("  vbpermmat <file> <nperms> <tes file> <type>\n");
  printf("  vbpermmat <file> <nperms> <ref file> <type>\n");
  printf("notes:\n");
  printf("  nperms is the number of permutations to generate\n");
  printf("  ndata is the number of data points\n");
  printf("  type is either \"sign\" or \"data\"\n");
  printf("  if third argument is a tes file, dimt will be taken as ndata\n");
  printf(
      "  if third argument is a ref file, its length will be taken as ndata\n");
  printf("\n");
}

void vbpermmat_version() {
  printf("VoxBo vbpermmat (v%s)\n", vbversion.c_str());
}
