
// permstep.cpp
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Kosh Banerjee
// modified a bit by Dan Kimberg

#include <iostream>
#include "perm.h"

using namespace std;

void usage(const unsigned short exitValue, char *progName);

gsl_rng *theRNG = NULL;

int main(int argc, char *argv[]) {
  SEGV_HANDLER
  GSL_ERROR_HANDLER_OFF

  if (argc == 1) {
    usage(1, argv[0]);
    exit(-1);
  }

  string matrixStemName;
  string permDir;
  int exhaustive = 0;
  VB_Vector pseudoT;
  VB_permtype method = vb_noperm;
  int permIndex = 0;
  string contrast;

  GLMInfo glmi;
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      usage(0, argv[0]);
      exit(0);
    } else if (args[i] == "-v") {
      printf("\nVoxBo v%s\n", vbversion.c_str());
      exit(0);
    } else if (args[i] == "-m" && i < args.size() - 1)
      matrixStemName = args[++i];
    else if (args[i] == "-d" && i < args.size() - 1)
      permDir = args[++i];
    else if (args[i] == "-t" && i < args.size() - 1)
      method = permclass::methodtype(args[++i]);
    else if (args[i] == "-n" && i < args.size() - 1) {
      pair<bool, int32> ret;
      ret = strtolx(args[++i]);
      if (ret.first) {
        cout << "[E] permstep: invalid perm index\n";
        exit(101);
      }
      permIndex = ret.second;
    } else if (args[i] == "-c" && i < args.size() - 1)
      contrast = args[++i];
    else if (args[i] == "-p" && i < args.size() - 3) {
      pseudoT.resize(3);
      pseudoT[0] = strtod(args[++i]);
      pseudoT[1] = strtod(args[++i]);
      pseudoT[2] = strtod(args[++i]);
    } else if (args[i] == "-e")
      exhaustive = 1;
    else {
      cout << "[E] permstep: unrecognized argument: " << args[i] << endl;
      exit(99);
    }
  }
  if (matrixStemName.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Must specify the matrix stem name, using the \"-m\" option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }
  if (permDir.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Must specify the permutation directory name, using the \"-d\" "
                "option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }
  // FIXME check contrast valid?
  if (permIndex < 0) {
    ostringstream errorMsg;
    errorMsg << "You must enter a positive permIndex.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }

  glmi.setup(matrixStemName);
  glmi.parsecontrast(contrast);
  int err = 0;
  err = permStep(matrixStemName, permDir, permIndex, method, glmi.contrast,
                 pseudoT, exhaustive);
  if (err == 0)
    cout << format("[I] permstep: permutation set %s, index %d done\n") %
                permDir % permIndex;
  if (err) switch (err) {
      case 100:
        printErrorMsg(VB_ERROR, "permstep: no stem name specified.\n");
        break;
      case 101:
        printErrorMsg(VB_ERROR, "permstep: no perm directory specified.\n");
        return -1;
      case 102:
        printErrorMsg(VB_ERROR,
                      "permstep: method had value other than 0, 1, or 2.\n");
        return -1;
      case 103:
        printErrorMsg(VB_ERROR, "permstep: prm file was not found.\n");
        return -1;
      case 104:
        printErrorMsg(VB_ERROR, "permstep: there were no mask values.\n");
        break;
      case 105:
        printErrorMsg(VB_ERROR, "permstep: there was not a tes list.\n");
        return -1;
      case 106:
        printErrorMsg(VB_ERROR, "permstep: G matrix file not readable.\n");
        return -1;
      case 107:
        printErrorMsg(VB_ERROR, "permstep: G matrix file not valid.\n");
        return -1;
      case 108:
        printErrorMsg(VB_ERROR, "permstep: trace state not found.\n");
        break;
      case 109:
        printErrorMsg(VB_ERROR, "permstep: V matrix not readable.\n");
        return -1;
      case 110:
        printErrorMsg(VB_ERROR, "permstep: V matrix not valid.\n");
        return -1;
      case 111:
        printErrorMsg(VB_ERROR, "permstep: V order does not equal V rank.\n");
        return -1;
      case 112:
        printErrorMsg(VB_ERROR, "permstep: Fac1 matrix not readable.\n");
        break;
      case 113:
        printErrorMsg(VB_ERROR, "permstep: Fac1 matrix not valid.\n");
        return -1;
      case 114:
        printErrorMsg(VB_ERROR,
                      "permstep: V order does not equal Fac1 rank.\n");
        return -1;
      case 115:
        printErrorMsg(
            VB_ERROR,
            "permstep: Fac1 order does not equal number of contrasts.\n");
        return -1;
      case 116:
        printErrorMsg(VB_ERROR, "permstep: Fac3 matrix not readable.\n");
        return -1;
      case 117:
        printErrorMsg(VB_ERROR, "permstep: Fac3 matrix not valid.\n");
        break;
      case 118:
        printErrorMsg(VB_ERROR,
                      "permstep: Fac1 order does not equal Fac3 rank.\n");
        return -1;
      case 119:
        printErrorMsg(VB_ERROR,
                      "permstep: Fac1 rank does not equal Fac3 order.\n");
        return -1;
      case 120:
        printErrorMsg(VB_ERROR, "permstep: permutation matrix not readable.\n");
        return -1;
      case 121:
        printErrorMsg(VB_ERROR, "permstep: permutation matrix not valid.\n");
        break;
      case 122:
        printErrorMsg(VB_ERROR,
                      "permstep: error loading data for permutation matrix.\n");
        return -1;
      case 123:
        printErrorMsg(
            VB_ERROR,
            "permstep: permutation index exceeds number of permutations.\n");
        return -1;
      case 124:
        printErrorMsg(VB_ERROR, "permstep: no betas of interest specified.\n");
        return -1;
      case 125:
        printErrorMsg(VB_ERROR, "permstep: error loading data for G matrix.\n");
        break;
      case 126:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for R matrix.\n");
        return -1;
      case 127:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for Fac1 matrix.\n");
        return -1;
      case 128:
        printErrorMsg(VB_ERROR, "permstep: failure writing statistic cube. \n");
        return -1;
      case 129:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for Gtg matrix.\n");
        return -1;
      case 130:
        printErrorMsg(
            VB_ERROR,
            "permstep: failure allocating space for invGtg matrix.\n");
        break;
      case 131:
        printErrorMsg(
            VB_ERROR,
            "permstep: failure allocating space for LUPerm structure.\n");
        return -1;
      case 132:
        printErrorMsg(VB_ERROR, "permstep: failure inverting Gtg matrix.\n");
        return -1;
      case 133:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for Ggtg matrix.\n");
        return -1;
      case 134:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for R matrix.\n");
        break;
      case 135:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for Gtvt matrix.\n");
        return -1;
      case 136:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for F3 matrix.\n");
        return -1;
      case 137:
        printErrorMsg(VB_ERROR,
                      "permstep: failure allocating space for F3##F1 "
                      "intermediate matrix.\n");
        return -1;
      default:
        printErrorMsg(VB_ERROR, "permstep: unknown error.\n");
    }
  return 0;
}

void usage(const unsigned short, char *) {
  printf("\nVoxBo permstep (v%s)\n", vbversion.c_str());
  printf("summary: ");
  printf(" Permutation step.\n");
  printf("usage:\n");
  printf(" permstep -h -m[matrix stem name] -d[permutation directory]\n");
  printf("          -c[contrasts] -p[pseudot values] -t[permutation type]\n");
  printf("          -n[index of permutation selected] -v\n");
  printf("flags:\n");
  printf(" -h                        Print usage information. Optional.\n");
  printf(
      " -m <matrix stem name>                       Specify the matrix stem "
      "name. Required.\n");
  printf(" -d                        Permutation directory name. Required.\n");
  printf(
      " -t                        Permutation type: 0/none, 1/order, or "
      "2/sign\n");
  printf(" -c                        contrast values, in quotes. Required.\n");
  printf(" -p                        pseudoT kernel, in quotes. Required.\n");
  printf(" -v                        Global VoxBo version number. Optional.\n");
  printf(
      " -e                        Produce cubes and their inverses. "
      "Optional.\n");
  printf(
      " -n                        index of permutation to be generated. "
      "Optional.\n");
  printf("notes:                                                         \n");
  printf(
      "                           /1 and /2 force one tailed and two tailed, "
      "and\n");
  printf(
      "                           note that VoxBo defaults to one tailed "
      "testing\n");
  printf(
      " -v                        Global VoxBo version number. Optional.\n\n");
  exit(-1);
}
