
// makestatcub.h
// header for functionality that creates statistical cubes
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
// original version written by Kosh Banjeree and Tom King, and
// architected by Dan Kimberg.

#include <gsl/gsl_cdf.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_vector.h>
#include <cmath>
#include <map>
#include "glmutil.h"
#include "vbio.h"

using namespace std;

extern gsl_rng* theRNG;

int TStatisticCube(Cube& cube, VB_Vector& contrasts, VB_Vector& pseudoT,
                   Tes& paramTes, unsigned short rankG, VBMatrix& F1,
                   VBMatrix& F3, vector<unsigned long>& betasOfInt,
                   const vector<unsigned long>& betasToPermute);

int InterceptTermPercentChange(
    Cube& cube, const string& matrixStemName, VB_Vector& contrasts,
    VB_Vector& pseudoT, Tes& paramTes, const vector<string>& gHeader,
    unsigned short orderG, unsigned short rankG, VBMatrix& V, VBMatrix& F1,
    VBMatrix& F3, const double effDf, vector<unsigned long>& keepBetas,
    vector<unsigned long>& betasOfInt,
    const vector<unsigned long>& betasToPermute, string scale) throw();

int RawBetaValues(Cube& cube, const string& matrixStemName,
                  VB_Vector& contrasts, VB_Vector& pseudoT, Tes& paramTes,
                  const vector<string>& gHeader, unsigned short orderG,
                  unsigned short rankG, VBMatrix& V, VBMatrix& F1, VBMatrix& F3,
                  const double effDf, vector<unsigned long>& keepBetas,
                  vector<unsigned long>& betasOfInt,
                  const vector<unsigned long>& betasToPermute,
                  string scale) throw();

int FStatisticCube(Cube& cube, const string& matrixStemName,
                   VB_Vector& contrasts, VB_Vector& pseudoT, Tes& paramTes,
                   const vector<string>& gHeader, unsigned short orderG,
                   unsigned short rankG, VBMatrix& V, VBMatrix& F1,
                   VBMatrix& F3, const double effDf,
                   vector<unsigned long>& keepBetas,
                   vector<unsigned long>& betasOfInt,
                   const vector<unsigned long>& betasToPermute,
                   string scale) throw();

// passed a T stat cube, returns a pmap cube based on the t values
int TTestPMap(Cube& cube, Tes& paramTes, double numTails, double effdf);

// passed a F stat cube, returns a pmap cube based on the t values
int FTestPMap(Cube& cube, Tes& paramTes, double numCovariates, double effdf);

// passed a T stat cube, returns a zmap cube based on the p values
int TTestZMap(Cube& cube, Tes& paramTes, double numTails, double effdf);

// passed a F stat cube, returns a zmap cube based on the p values
int FTestZMap(Cube& cube, Tes& paramTes, double numCovariates, double effdf);

int makeStatCub(Cube& cube, string& matrixStemName, VBContrast& contrast,
                VB_Vector& pseudoT, Tes& tes);

class fdrstat {
 public:
  fdrstat() { init(); }
  fdrstat(double qval) {
    init();
    q = qval;
  }
  void init() {
    q = 0.01;
    qv = 0.0;
    low = 0;
    high = 0;
    maxind = -1;
    statval = 0;
    nvoxels = 0;
  }
  double q;
  double qv;  // storage for i/V * q/cv
  double statval;
  int maxind;
  double low, high;  // low,high p for sanity checking
  uint32 nvoxels;
};

// FDR stuff
// double calc_fdr_thresh(Cube &statcube,Cube &pcube,Cube &mask,double q=.01);
vector<fdrstat> calc_multi_fdr_thresh(Cube& statcube, Cube& pcube, Cube& mask,
                                      double q = 0.0);
vector<fdrstat> calc_multi_fdr_thresh(Cube& statcube, Cube& pcube, Cube& mask,
                                      vector<double> qs);
