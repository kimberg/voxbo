
// vbregress.cpp
// carry out voxbo regression using mGLM (or regular GLM)
// Copyright (c) 2005-2007 by The VoxBo Development Team

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
#include "vbutil.h"

void vbregress_help();
void vbregress_version();

int main(int argc, char *argv[]) {
  tokenlist args;
  string infile, outfile;

  if (argc < 2) {
    vbregress_help();
    exit(0);
  }

  arghandler ah;
  string errstr;
  ah.setArgs("-s", "--signperm", 2);
  ah.setArgs("-o", "--orderperm", 2);
  ah.setArgs("-m", "--meannorm", 0);
  ah.setArgs("-d", "--detrend", 0);
  ah.setArgs("-e", "--excludeerror", 0);
  ah.setArgs("-p", "--part", 2);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vbregress: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vbregress_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vbregress_version();
    exit(0);
  }

  long flags = 0;
  if (ah.flagPresent("-m")) flags |= MEANSCALE;
  if (ah.flagPresent("-d")) flags |= DETREND;
  if (ah.flagPresent("-e")) flags |= EXCLUDEERROR;

  int part = 1, nparts = 1;
  args = ah.getFlaggedArgs("-p");
  if (args.size()) {
    part = strtol(args[0]);
    nparts = strtol(args[1]);
  }

  string perm_mat;
  int signperm_index = -1;
  int orderperm_index = -1;
  args = ah.getFlaggedArgs("-s");
  if (args.size()) {
    perm_mat = args[0];
    signperm_index = strtol(args[1]);
  }
  args = ah.getFlaggedArgs("-o");
  if (args.size()) {
    perm_mat = args[0];
    orderperm_index = strtol(args[1]);
  }

  tokenlist filelist = ah.getUnflaggedArgs();
  if (filelist.size() != 1) {
    printf("[E] vbregress: requires a glm directory\n");
    exit(10);
  }
  GLMInfo glmi;
  glmi.setup(filelist[0]);
  // glmi.print();
  if (signperm_index > -1) {
    VBMatrix pmat(perm_mat);
    if (pmat.m)
      glmi.perm_signs = pmat.GetColumn(signperm_index);
    else {
      printf("[E] vbregress: couldn't get permutation vector\n");
      exit(122);
    }
  }
  if (orderperm_index > -1) {
    VBMatrix pmat(perm_mat);
    if (pmat.m)
      glmi.perm_order = pmat.GetColumn(signperm_index);
    else {
      printf("[E] vbregress: couldn't get permutation vector\n");
      exit(123);
    }
  }
  int err;
  if (glmi.teslist.size()) {
    if ((err = glmi.TesRegress(part, nparts, flags))) {
      printf("[E] vbregress: error %d regressing\n", err);
      exit(101);
    }
  } else {
    if ((err = glmi.VecRegressX())) {
      printf("[E] vbregress: error %d regressing\n", err);
      exit(101);
    }
  }

  printf("[I] vbregress: done!\n");
  exit(0);
}

void vbregress_help() {
  printf("\nVoxBo vbregress (v%s)\n", vbversion.c_str());
  printf("usage:\n");
  printf("  vbregress  <glmdir>\n");
  printf("flags:\n");
  printf("  -m             mean scale\n");
  printf("  -d             linear detrend\n");
  printf("  -s <mat> <ind> sign permutation\n");
  printf("  -o <mat> <ind> order permutation\n");
  printf("  -e             exclude error term from parameter file\n");
  printf("  -p <part> <nparts>   do just part of it\n");
  printf("  -h             show help\n");
  printf("  -v             show version\n");
  printf("\n");
  printf("notes:\n");
  printf("  linear detrend fits a regression line to the data and removes\n");
  printf("  any linear trend.  mean scaling divides each time series\n");
  printf("  by its mean.  both of these steps are performed independently\n");
  printf("  for each 4D file included in the regression.\n");
}

void vbregress_version() {
  printf("VoxBo vbregress (v%s)\n", vbversion.c_str());
}
