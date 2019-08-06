
// makematkg.cpp
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

#include <iostream>
#include "regression.h"
#include "vb_common_incs.h"

using namespace std;

void usage(const unsigned short exitValue, char *progName);

/*********************************************************************
 * This program finishes off Tes regression.                          *
 *********************************************************************/
int main(int argc, char *argv[]) {
  SEGV_HANDLER
  string matrixStemName;
  if (argc == 1) usage(1, argv[0]);

  /*********************************************************************
   * Now processing the command line options.                           *
   *                                                                    *
   * -h ==> Display usage information.                                  *
   * -m ==> Specifies the matrix stem name.                             *
   * -v ==> Print out the gobal VoxBo version number.                   *
   *                                                                    *
   * VARIABLES:                                                         *
   * printHelp - a flag, used to determine if the "-h" command line     *
   *             option was used or not.                                *
   * printVersion - a flag, used to determine if the "-v" command line  *
   *                option was used or not.                             *
   *********************************************************************/
  arghandler a;
  a.setArgs("-h", "--help", 0);
  a.setArgs("-m", "--matrixstemname", 1);
  a.setArgs("-v", "--version", 0);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
    errstring = "[E] unknown flag: " + errstring;
    printErrorMsg(VB_ERROR, errstring.c_str());
    exit(-1);
  }
  if (a.flagPresent("-h")) usage(0, argv[0]);
  matrixStemName = a.getFlaggedArgs("-m")[0];
  if (a.flagPresent("-v")) printf("\nVoxBo v%s\n", vbversion.c_str());
  if (matrixStemName.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Must specify the matrix stem name, using the \"-m\" option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }
  makeMatrixKG(matrixStemName);
  printf("[I] makematkg: success.\n");
  return 0;
}

void usage(const unsigned short, char *) {
  printf("\nVoxBo makematkg (v%s)\n", vbversion.c_str());
  printf("summary: ");
  printf(" Make the KG matrix.\n");
  printf("usage:\n");
  printf(" makematkg -h -m[matrix stem name] -v\n");
  printf("flags:\n");
  printf(" -h                        Print usage information. Optional.\n");
  printf(
      " -m <matrix stem name>                       Specify the matrix stem "
      "name. Required.\n");
  printf(
      " -v                        Global VoxBo version number. Optional.\n\n");
  exit(-1);
}
