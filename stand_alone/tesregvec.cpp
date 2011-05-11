
// tesregvec.cpp
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
int main(int argc, char *argv[])
{

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
* -r ==> Specifies name of ref file                                  *
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
  string reffile;
  int autocorrelate = 0;
  arghandler a;
  a.setArgs("-h", "--help", 0);
  a.setArgs("-m", "--matrixstemname", 1);
  a.setArgs("-r", "--reffile", 1);
  a.setArgs("-a", "--autocorrelation", 0);
  a.setArgs("-v", "--version", 0);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
     errstring = "[E] unknown flag: " + errstring;
     printErrorMsg(VB_ERROR, errstring.c_str());
     exit(-1);
  }
  if (a.flagPresent("-h")) {
     usage(0, argv[0]);
     exit(0);
  }
  if (a.flagPresent("-v"))
    printf("\nVoxBo v%s\n",vbversion.c_str());
  tokenlist temp = a.getFlaggedArgs("-m");
  matrixStemName = temp[0];
  temp = a.getFlaggedArgs("-r");
  reffile = temp[0];
  if (a.flagPresent("-a")) autocorrelate = 1;
/*********************************************************************
* Now checking to see if the global VoxBo version needs to be        *
* printed.                                                           *
*********************************************************************/
  if (printVersion)
    printf("\nVoxBo v%s\n",vbversion.c_str());
  
  if (matrixStemName.size() == 0)
  {
    ostringstream errorMsg;
    errorMsg << "Must specify the matrix stem name, using the \"-m\" option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg, 1);
  } // if

  if (reffile.size() == 0)
  {
    ostringstream errorMsg;
    errorMsg << "Must specify the ref file name, using the \"-r\" option.";
    printErrorMsgAndExit(VB_ERROR, errorMsg, 1);
  } // if

  tesRegVec(reffile, matrixStemName, autocorrelate);

/*********************************************************************
* Now returning.                                                     *
*********************************************************************/
  return 0;

} // main

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
void usage(const unsigned short exitValue, char *progName)
{
  genusage(exitValue, progName, "- Tes regression start.",
           "-h                       Print usage information. Optional.",
           "-m <matrix stem name>    Specify the matrix stem name. Required.",
           "-r <ref file>            File containing vector values.",
           "-a <autocorrelate>       check vector for autocorrelation.",
           "-v                       Global VoxBo version number. Optional.",
           "");

} // void usage(const unsigned short exitValue, char *progName)

