
// tesregfinish.cpp
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
 * Required include files.                                            *
 *********************************************************************/
#include <iostream>
#include "regression.h"
#include "vb_common_incs.h"

/*********************************************************************
 * Using the standard namespace.                                      *
 *********************************************************************/
using namespace std;

/* >>>>>>>>>>>>           FUNCTION PROTOTYPES          <<<<<<<<<<<< */

void usage(const unsigned short exitValue, char *progName);

/* >>>>>>>>>>>>         END FUNCTION PROTOTYPES        <<<<<<<<<<<< */

/*********************************************************************
 * This program finishes off Tes regression.                          *
 *********************************************************************/
int main(int argc, char *argv[]) {
  /*********************************************************************
   * Installing a signal handler for SIGSEGV.                           *
   *********************************************************************/
  SEGV_HANDLER

  /*********************************************************************
   * Declaring a string object to hold the argument for the "-m"        *
   * option, the matrix stem name.                                      *
   *********************************************************************/
  string matrixStemName;

  /*********************************************************************
   * If no command line options have been used, then usage information  *
   * is displayed and the program exits.                                *
   *********************************************************************/
  if (argc == 1) {
    usage(1, argv[0]);
  }  // if

  /*********************************************************************
   * Now processing the command line options.                           *
   *                                                                    *
   * -h ==> Display usage information.                                  *
   * -m ==> Specifies the matrix stem name.                             *
   * -s ==> Specifies the number of step for the regression.            *
   * -e ==> Exclude error flag.                                         *
   * -v ==> Print out the gobal VoxBo version number.                   *
   *                                                                    *
   * VARIABLES:                                                         *
   * printHelp - a flag, used to determine if the "-h" command line     *
   *             option was used or not.                                *
   * printVersion - a flag, used to determine if the "-v" command line  *
   *                option was used or not.                             *
   * excludeError - a flag, used to determine if the exlcude error flag *
   *            is to used.                                             *
   * numSteps - the number of total regression steps.                   *
   *********************************************************************/
  bool printHelp = false;
  bool printVersion = false;
  bool excludeError = false;
  unsigned long numSteps = 0;
  processOpts(argc, argv, ":hm:evs:", "bZbbL", &printHelp, &matrixStemName,
              &excludeError, &printVersion, &numSteps);

  /*********************************************************************
   * Now checking to see if usage information needs to be printed.      *
   *********************************************************************/
  if (printHelp) {
    usage(0, argv[0]);
  }  // if

  /*********************************************************************
   * Now checking to see if the global VoxBo version needs to be        *
   * printed.                                                           *
   *********************************************************************/
  if (printVersion) {
    printf("\nVoxBo v%s\n", vbversion.c_str());
  }  // if

  /*********************************************************************
   * Now making sure that matrixStemName is non-empty, since it is a    *
   * required option. If matrixStemName is empty, then an error         *
   * message is printed and then this program exits.                    *
   *********************************************************************/
  if (matrixStemName.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Must specify the matrix stem name, using the \"-m\" option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg, 1);
  }  // if

  /*********************************************************************
   * Making sure that numSteps is positive. If it is not, then an error *
   * message is printed and then this program exits.                    *
   *********************************************************************/
  if (numSteps == 0) {
    ostringstream errorMsg;
    errorMsg << "Number of regression steps [" << numSteps
             << "] must be a positive integer.";
    printErrorMsgAndExit(VB_ERROR, errorMsg, 1);
  }  // if

  /*********************************************************************
   * Now tesRegressionFinish() is called with the appropriate arguments.*
   *********************************************************************/
  tesRegressionFinish(matrixStemName, excludeError, numSteps);

  /*********************************************************************
   * Now returning.                                                     *
   *********************************************************************/
  return 0;

}  // main

/*********************************************************************
 * This function calls genusage() to print out the usage information. *
 *                                                                    *
 * INPUTS:                                                            *
 * -------                                                            *
 * progName: String holding the program name.                         *
 *                                                                    *
 * OUTPUTS:                                                           *
 * --------                                                           *
 * None.                                                              *
 *********************************************************************/
void usage(const unsigned short exitValue, char *progName) {
  genusage(
      exitValue, progName, "- Tes regression start.",
      "-h -m[matrix stem name] -s[number of steps] -e -v",
      "-h                        Print usage information. Optional.",
      "-m <matrix stem name>     Specify the matrix stem name. Required.",
      "-s <number of steps>      The total number of regression steps.",
      "                          Must be a positive integer. Required.",
      "-e                        If used, then the exclude error flag is true.",
      "                          Optional.",
      "-v                        Global VoxBo version number. Optional.", "");

}  // void usage(const unsigned short exitValue, char *progName)
