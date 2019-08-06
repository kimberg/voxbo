
// perm.h
//
// Copyright (c) 1998-2004 by The VoxBo Development Team

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
 * Include guard.                                                     *
 *********************************************************************/
#ifndef PERM_H
#define PERM_H

/*********************************************************************
 * Required include files.                                            *
 *********************************************************************/
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_rng.h>
#include <cmath>
#include <fstream>
#include <map>
#include "glmutil.h"
#include "makestatcub.h"
#include "time_series_avg.h"
#include "utils.h"

enum VB_permtype { vb_noperm, vb_orderperm, vb_signperm };

class permclass {
 public:
  permclass();
  ~permclass();
  void AddPrmTimeStamp(string matrixStemName);
  string GetPrmTimeStamp();
  void SetFileName(string filename);
  void SavePermClass();
  void LoadPermClass();
  void Clear();
  void Print();
  string stemname;
  string permdir;
  string m_prmTimeStamp;
  string scale;
  VB_permtype method;
  int nperms;
  string contrast;
  string pseudotlist;
  string filename;
  uint32 rngseed;
  static string methodstring(const VB_permtype pt);
  static VB_permtype methodtype(const string tp);
};

/*********************************************************************
 * Using the standard namespace.                                      *
 *********************************************************************/
using namespace std;

/*********************************************************************
 * Needed constants.                                                  *
 *********************************************************************/
const unsigned short MAX_PERMS = 1000;
const unsigned short PERMUTATION_LIMIT = 6;
const unsigned short SIGN_PERMUTATION_LIMIT = 11;

/*********************************************************************
 * Needed external variable.                                          *
 *********************************************************************/
extern gsl_rng *theRNG;

/* >>>>>>>>>>>>           FUNCTION PROTOTYPES          <<<<<<<<<<<< */

/*********************************************************************
 * This function initializes the global variable theRNG.              *
 *********************************************************************/
void initRNG(uint32 rngseed);

/*********************************************************************
 * This function deallocates the global variable theRNG.              *
 *********************************************************************/
void freeRNG();

/*********************************************************************
 * This function sets up the permutation analysis matrix.             *
 *********************************************************************/
int permStart(permclass pc);

/*********************************************************************
 * This function computes the factorial of the input integer.         *
 *********************************************************************/
size_t factorial(const size_t n);

/*********************************************************************
 * This function prints the elements of the input permutation in      *
 * GSL form, i.e., each element is in {0, 1, 2, ..., n - 1}.          *
 *********************************************************************/
void printGSLPerm(const gsl_permutation *pi);

/*********************************************************************
 * This function computes a random permutation.                       *
 *********************************************************************/
void randPerm(gsl_permutation *pi);

/*********************************************************************
 * This function increments the elements of the input permutation.    *
 *********************************************************************/
void incPerm(gsl_permutation *pi);

/*********************************************************************
 * This function prints the name of the GSL random number generator.  *
 *********************************************************************/
void printRNGName(gsl_rng *theRNG);

/*********************************************************************
 * This function determines if the two input permutations are equal   *
 * or not.                                                            *
 *********************************************************************/
bool arePermsEqual(const gsl_permutation *p1, const gsl_permutation *p2,
                   const size_t begin = 0);

/*********************************************************************
 * This function checks to see if r is <= (n! - 1).                   *
 *********************************************************************/
void verifyPermRank(const size_t r, const size_t n);

/*********************************************************************
 * This function creates the specified permutation.                   *
 *********************************************************************/
void unrank1(const size_t n, const size_t r, gsl_permutation *pi);

/*********************************************************************
 * This function computes the rank of the input permutation.          *
 *********************************************************************/
size_t rank1(const size_t n, gsl_permutation *pi, gsl_permutation *piInv);

/*********************************************************************
 * This function determines if the permutation pi is already in the   *
 * the permutation hash table or not.                                 *
 *********************************************************************/
bool inHash(map<size_t, vector<gsl_permutation *> > &permHash,
            gsl_permutation *pi);

/*********************************************************************
 * This function prints out a list of the available GSL random number *
 * generators.                                                        *
 *********************************************************************/
void availableGSLRNGs();

/*********************************************************************
 * This function prints out the contents of the input map container.  *
 *********************************************************************/
void printHash(map<size_t, vector<gsl_permutation *> > &theHash);

/*********************************************************************
 * This function frees the memory allocated to the gsl_permutation    *
 * structs contained in the map.                                      *
 *********************************************************************/
void freePermHash(map<size_t, vector<gsl_permutation *> > &theHash);

/*********************************************************************
 * This function returns true if the 2 input sign permutation arrays  *
 * are equal. Otherwise, false is returned.                           *
 *********************************************************************/
bool areSignsEqual(const short *arr1, const short *arr2, const size_t len);

/*********************************************************************
 * This function retutns true if the input sign permutation is already*
 * in the input sign permutation hash table. Otherwise, the input     *
 * permutation is added to the hash table and false is returned.      *
 *********************************************************************/
bool inSignHash(map<short, vector<short *> > &signHash, short *pi,
                const size_t len);

/*********************************************************************
 * This function frees the memory allocated to the elements in the    *
 * input map container (which are sign permutation arrays).           *
 *********************************************************************/
void freeSignHash(map<short, vector<short *> > &theHash);

/*********************************************************************
 * This function prints the elements in the input map container       *
 * (which are sign permutation arrays).                               *
 *********************************************************************/
void printSignHash(map<short, vector<short *> > &theHash, const size_t len);

/*********************************************************************
 * This function prints out the input sign permutation array.         *
 *********************************************************************/
void printSignPerm(const short *thePerm, const size_t len);

/*********************************************************************
 * This function generates a random sign permutation.                 *
 *********************************************************************/
void randSignPerm(short *pi, const size_t len);

/*********************************************************************
 * This function computes the "rank" of a sign permutation.           *
 *********************************************************************/
size_t rankSignPerm(const short *signPerm, const size_t len);

/*********************************************************************
 * This function carries out a single permutation analysis step.      *
 *********************************************************************/
int permStep(string &matrixStemName, const string &permDir,
             const unsigned short permIndex, VB_permtype method,
             VBContrast &contrast, VB_Vector &pseudoT, int exhaustive) throw();

/*********************************************************************
 * This function computes the permutation R matrix and fact value.    *
 *********************************************************************/
int doFactR(const string &matrixStemName, const string &permDir,
            VB_Vector &contrasts, gsl_matrix *permuteG, VBMatrix &vMatrixFile,
            double *fact, gsl_matrix *R, gsl_matrix *F1) throw();

// DYK: added function to create perm matrix to spec
VBMatrix createPermMatrix(int nperms, int ndata, VB_permtype method,
                          uint32 seed);

/* >>>>>>>>>>>>         END FUNCTION PROTOTYPES        <<<<<<<<<<<< */

#endif  // PERM_H
