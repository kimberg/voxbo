
// utils.h
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
* This header file is for utility functions.                         *
*********************************************************************/

/*********************************************************************
* Inclusion guard.                                                   *
*********************************************************************/
#ifndef UTILS_H
#define UTILS_H

/*********************************************************************
* Needed header files.                                               *
*********************************************************************/
#include <iostream>
#include <fstream>
#include <ctime>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <sys/resource.h>
#include <unistd.h>
#include <gsl/gsl_errno.h>
#include "vb_common_incs.h"

/*********************************************************************
* Using the std namespace.                                           *
* (really, we shouldn't declare using any namespace in a header 
*  file...)
*********************************************************************/
using namespace std;

/*********************************************************************
* Macro to install the signal handler for SIGSEGV.                   *
*********************************************************************/
#define SEGV_HANDLER signal(SIGSEGV, utils::seg_fault);

/*********************************************************************
* Macro to turn off the default GSL error handler.                   *
*********************************************************************/
#define GSL_ERROR_HANDLER_OFF gsl_set_error_handler_off();

/*********************************************************************
* Defining the utils namespace.                                      *
*********************************************************************/
namespace utils
{

/*********************************************************************
* Needed constants.                                                  *
*********************************************************************/
  const int TEMP_STRING_LENGTH = 128;
  const short REF_TYPE = 1;
  const short TXT_TYPE = 2;
  const short MAT_TYPE = 3;
  const short UNKN_TYPE = -1;

/*********************************************************************
* Prototype for handling the SIGSEGV signal.                         *
*********************************************************************/
  void seg_fault(int signalNumber);

/*********************************************************************
* The following function tries to distinguish between REF1, TXT1,    *
* MAT1 files.                                                        *
*********************************************************************/
  short refOrTextOrMat(const string& vbFile,
  bool printErr = false);

/*********************************************************************
* This function determines the dimensions of the input file.         *
*********************************************************************/
  unsigned short determineVBFileDims(const string& theFile);

/*********************************************************************
* This function determins the file format of the input file.         *
*********************************************************************/
  char * determineVBFile(const string& theFile);

/*********************************************************************
* This function determines the VBFF name of the input file.          *
*********************************************************************/
  string determineVBFFName(const string& theFile);

/*********************************************************************
* This function returns true if the input file is readable.          *
*********************************************************************/
  bool isFileReadable(const string& fileName) throw();

/*********************************************************************
* This function returns the specified bit from the input object.     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* theObj             const class T&  The input object.               *
* bitNum             const unsgined short Specifies the bit to be    *
*                                         retireved.                 *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* N/A                 unsigned short The specified bit.              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
  template<class T> unsigned short getBit(const T& theObj, const unsigned short bitNum)
  {

/*********************************************************************
* If the specified bit is outside the range of the size of theObj    *
* (in bits), then an appropriate error message is printed, and then  *
* this program exits.                                                *
*********************************************************************/
    if (bitNum >= (sizeof(theObj) * 8))
    {
      ostringstream errorMsg;
      errorMsg << "The specfied bit number [" << bitNum
      << "] is greater than the number of available bits [" << sizeof(theObj) << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);

    } // if

/*********************************************************************
* At this point, we know that bitNum is within the number of bits    *
* ths comprise thObj. Therefore, To get the desired bit, 1 is        *
* shifted by bitNum and that result is bit-wise and'ed with theObj.  *
* If this result is positive, then 1 is returned. Otherwise, 0 is    *
* returned.                                                          *
*********************************************************************/
    if ((1 << bitNum) & theObj)
    {
      return 1;
    }
    return 0;

  } // template<class T> unsigned short getBit(const T& theObj, const unsigned short bitNum)

/*********************************************************************
* This function reads in names of Tes files from the "*.sub" file    *
* found in the directory specified by the input argument             *
* matrixStemName.                                                    *
*********************************************************************/
  int readInTesFiles(const string &matrixStemName,
  vector<string> &tesList) throw();

}; // namespace utils

#endif // UTILS_H
