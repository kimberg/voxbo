
// tesregstep.cpp
//
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
 * This program carries out the specified regression step.            *
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
   * Now processing the command line options.                           *
   *                                                                    *
   * -h ==> Display usage information.                                  *
   * -m ==> Specifies the matrix stem name.                             *
   * -s ==> Specifies the number of step for the regression.            *
   * -i ==> Specifies the particular regression step for this program.  *
   * -n ==> Mean norm flag.                                             *
   * -e ==> Exclude error flag.                                         *
   * -v ==> Print out the gobal VoxBo version number.                   *
   *                                                                    *
   * VARIABLES:                                                         *
   * printHelp - a flag, used to determine if the "-h" command line     *
   *             option was used or not.                                *
   * printVersion - a flag, used to determine if the "-v" command line  *
   *                option was used or not.                             *
   * meanNorm - a flag, used to determine if the mean norm flag is to   *
   *            used.                                                   *
   * excludeError - a flag, used to determine if the exlcude error flag *
   *            is to used.                                             *
   * numSteps - the number of total regression steps.                   *
   * stepIndex - the specific step that this instance of this program   *
   *             must perform.                                          *
   *********************************************************************/
  bool printVersion = false;
  bool meanNorm = false;
  bool excludeError = false;
  unsigned long numSteps = 0;
  long stepIndex = -1;
  int drift = 0;
  arghandler a;
  a.setArgs("-h", "--help", 0);
  a.setArgs("-m", "--matrixstemname", 1);
  a.setArgs("-s", "--steps", 1);
  a.setArgs("-i", "--index", 1);
  a.setArgs("-n", "--meannorm", 0);
  a.setArgs("-e", "--excludeerror", 0);
  a.setArgs("-v", "--version", 0);
  a.setArgs("-d", "--driftcorrect", 0);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
    errstring = "[E] unknown flag: " + errstring;
    printErrorMsg(VB_ERROR, errstring.c_str());
    exit(-1);
  }
  if (a.flagPresent("-d")) drift = 1;
  if (a.flagPresent("-n")) meanNorm = true;
  if (a.flagPresent("-e")) excludeError = true;
  if (a.flagPresent("-h")) {
    usage(0, argv[0]);
    exit(0);
  }
  if (a.flagPresent("-v")) printf("\nVoxBo v%s\n", vbversion.c_str());
  tokenlist temp = a.getFlaggedArgs("-s");
  numSteps = atoi(temp[0].c_str());
  temp = a.getFlaggedArgs("-i");
  stepIndex = atol(temp[0].c_str());
  temp = a.getFlaggedArgs("-m");
  matrixStemName = temp[0];
  /*
    cout << "drift: " << drift << endl;
    cout << "meannorn: " << meanNorm << endl;
    cout << "excludeError: " << excludeError << endl;
    cout << "msn: " << matrixStemName << endl;
    cout << "numSteps: " << numSteps << endl;
    cout << "index: " << stepIndex << endl;
  */
  /*********************************************************************
   * Now checking to see if the global VoxBo version needs to be        *
   * printed.                                                           *
   *********************************************************************/
  if (printVersion) printf("\nVoxBo v%s\n", vbversion.c_str());

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
   * Ensuring that stepIndex is in the range [0, numSteps - 1],         *
   * inclusive. If it's not, then an error message is printed and then  *
   * this program exits.                                                *
   *********************************************************************/
  if ((stepIndex < 0) || (((unsigned long)stepIndex) >= numSteps)) {
    ostringstream errorMsg;
    errorMsg << "Step index must be in the range [0, " << (numSteps - 1)
             << "], inclusive,";
    printErrorMsgAndExit(VB_ERROR, errorMsg, 1);
  }  // if

  /*********************************************************************
   * vector<string> tesList will hold the names of the Tes files needed *
   * for the regression.                                                *
   *********************************************************************/
  vector<string> tesList;

  /*********************************************************************
   * Now tesRegressionStep() is called with the appropriate arguments.  *
   * NOTE: Since tesList is empty, the required Tes file names will be  *
   * read in from the file (matrixStemName + ".sub").                   *
   *********************************************************************/
  tesRegressionStep(matrixStemName, tesList, meanNorm, excludeError, numSteps,
                    stepIndex, drift);

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
      "-h -m[matrix stem name] -s[number of steps] -i[step index] -n -e -v",
      "-h                        Print usage information. Optional.",
      "-m <matrix stem name>     Specify the matrix stem name. Required.",
      "-s <number of steps>      The total number of regression steps.",
      "                          Must be a positive integer. Required.",
      "-i <step index>           The particular step for this process.",
      "                          Must be in [0, (number of steps - 1)]. "
      "Required.",
      "-n                        If used, then the mean norm flag is true.",
      "                          Optional.",
      "-e                        If used, then the exclude error flag is true.",
      "                          Optional.",
      "-d                        Remove linear drift in signal.",
      "-v                        Global VoxBo version number. Optional.", "");

}  // void usage(const unsigned short exitValue, char *progName)
