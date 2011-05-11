/*********************************************************************
* Copyright (c) 1998, 1999, 2001, 2002 by Center for Cognitive       *
* Neuroscience, University of Pennsylvania.                          *
*                                                                    *
* This program is free software; you can redistribute it and/or      *
* modify it under the terms of the GNU General Public License,       *
* version 2, as published by the Free Software Foundation.           *
*                                                                    *
* This program is distributed in the hope that it will be useful,    *
* but WITHOUT ANY WARRANTY; without even the implied warranty of     *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
* General Public License for more details.                           *
*                                                                    *
* You should have received a copy of the GNU General Public License  *
* along with this program; see the file named COPYING.  If not,      *
* write to the Free Software Foundation, Inc., 59 Temple Place,      *
* Suite 330, Boston, MA 02111-1307 USA                               *
*                                                                    *
* For general information on VoxBo, including the latest complete    *
* source code and binary distributions, manual, and associated       *
* files, see the VoxBo home page at: http://www.voxbo.org/           *
*                                                                    *
* Original version written by Kosh Banerjee.                         *
* <banerjee@wernicke.ccn.upenn.edu>.                                 *
*********************************************************************/

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
* This function retrieves the header line specified by headerKey.    *
*********************************************************************/
  string getHeaderLine(const string& headerKey,
  const vector<string>& theHeader);

/*********************************************************************
* This function retrieves the header tokens from the header line     *
* specified by headerKey.                                            *
*********************************************************************/
  string utils::getHeaderLineTokens(const string& headerKey,
  const vector<string>& theHeader, const string& theDelim = " ");

/*********************************************************************
* Prototypes for functions to print out matrices.                    *
*********************************************************************/
  void printMat(VBMatrix& M) throw();
  void printGSLMat(const gsl_matrix *m) throw();
  void printMatFile(VBMatrix& M, const char *matName, 
  const char *file) throw();
  void printGSLMatFile(const gsl_matrix *m, const char *matName, 
  const char *file) throw();

}; // namespace utils

/*********************************************************************
* This function creates the permutation specified by a rank in       *
* lexicographic order.                                               *
*********************************************************************/
void permLexUnrank(size_t r, gsl_permutation *pi);

/*********************************************************************
* This function computes the rank, in the lexicographic order, of    *
* the input permutation.                                             *
*********************************************************************/
size_t permLexRank(const gsl_permutation *pi);

/*********************************************************************
* This function computes the rank, in the minimal change ordering, of*
* the input permutation.                                             *
*********************************************************************/
size_t trotterJohnsonRank(const gsl_permutation *pi);

/*********************************************************************
* This function creates the permutation specified by a rank in       *
* the minimal change ordering.                                       *
*********************************************************************/
void trotterJohnsonUnrank(const size_t r, gsl_permutation *pi);

/*********************************************************************
* This function prints the elements of the input permutation in      *
* standard form, i.e., each element is in {1, 2, 3, ..., n}.         *
*********************************************************************/
void printStdPerm(const gsl_permutation *pi);

/*********************************************************************
* This function computes the ratio of the factorials of the two input*
* integers.                                                          *
*********************************************************************/
bool factorialRatio(const size_t n, const size_t d, size_t &ratio);

/*********************************************************************
* This function decrements the elements of the input permutation.    *
*********************************************************************/
void decPerm(gsl_permutation *pi);

#endif // UTILS_H


