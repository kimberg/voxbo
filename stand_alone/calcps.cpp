
// calcps.cpp
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

/*********************************************************************
 * calcps.cpp                                                         *
 * VoxBo CalcPS utility.                                              *
 * K. Banerjee  03/06/2001                                            *
 *                                                                    *
 * This program computes the Grand Mean Power Spectrum of the input   *
 * 4D data file.                                                      *
 *                                                                    *
 * INPUTS:                                                            *
 * -------                                                            *
 * 4D data file name. Required.                                       *
 *                                                                    *
 * OUTPUTS:                                                           *
 * --------                                                           *
 * The Power spectrum is written to a file called *_PS.ref where      *
 * "*" represents the input 4D data file name without the ".tes"      *
 * extension.                                                         *
 *********************************************************************/

void usage(const unsigned short exitValue, char *progName);

#include "calc_gs_ps.h"

int main(int argc, char *argv[]) {
  SEGV_HANDLER
  string tesFile;

  if (argc == 1) usage(1, argv[0]);

  /*********************************************************************
   * Now processing the command line options.                           *
   * -h ==> Display usage information.                                  *
   * -i ==> Specifies the input 4D data file.                           *
   * -v ==> Print global VoxBo version.                                 *
   *********************************************************************/
  arghandler a;
  a.setArgs("-h", "--help", 0);
  a.setArgs("-i", "--inputfile", 1);
  a.setArgs("-v", "--version", 0);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
    errstring = "[E] unknown flag: " + errstring;
    printErrorMsg(VB_ERROR, errstring.c_str());
    exit(-1);
  }
  if (a.flagPresent("-h")) usage(0, argv[0]);
  tesFile = a.getFlaggedArgs("-i")[0];
  if (a.flagPresent("-v")) printf("\nVoxBo v%s\n", vbversion.c_str());
  if (tesFile.length() == 0) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "]  Must specify the input 4D data file name.";
    printErrorMsg(VB_ERROR, errorMsg.str());
    usage(1, argv[0]);
  }
  CalcPs myPs(tesFile);
  myPs.printPsData();
  return 0;
}

void usage(const unsigned short, char *) {
  printf("\nVoxBo calcps (v%s)\n", vbversion.c_str());
  printf("summary: ");
  printf(" Calc PS routine for VoxBo.\n");
  printf("usage:\n");
  printf(" calcps -h -i[4D data file name] -v\n");
  printf("flags:\n");
  printf(" -h                        Print usage information. Optional.\n");
  printf(
      " -i <4D data file>         Specify the input 4D data file. Required.\n");
  printf(
      " -v                        Global VoxBo version number. Optional.\n\n");
  exit(-1);
}
