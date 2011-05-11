
// glmutil.h
// Header file for glm-related library code
// Copyright (c) 2003-2010 by the VoxBo Development Team
//
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
// original version written by Dongbo Hu
// code contributed by Dan Kimberg
// code contributed by Tom King

#ifndef GLMUTILS_H
#define GLMUTILS_H

#include "statthreshold.h"
#include "vbio.h"
#include "vbjobspec.h"

class GLMParams {
public:
  string name;              // name of the sequence
  string dirname;           // root directory
  string stem;              // stem name for analysis
  vector<string>scanlist;   // list of scans (data files)
  int lows,highs;           // low and high frequencies to remove
  string middles;           // middle frequencies to remove
  uint32 pieces;            // n pieces for matrix jobs
  string kernelname;        // temporal smoothing kernel
  double kerneltr;          // sampling interval of kernel file
  string noisemodel;        // noise model
  string refname;           // file containing reference function
  string glmfile;           // filename of .glm file
  string gmatrix;           // g matrix file
  string email;             // email address
  vector<string> contrasts; // list of contrasts to add to contrasts.txt
  int pri;
  bool auditflag,meannorm,emailflag,meannormset,driftcorrect;
  double TR;
  uint32 orderg;
  bool valid;
  bool rfxgflag;            // should we just dummy up a G matrix with all 1's?
  VBSequence seq;
  void init();
  int CreateGLMDir();
  void FixRelativePaths();
  void CreateGLMJobs();
  void CreateGLMJobs2();
  vector<string> CreateGLMScript();
  int WriteGLMFile(string filename="");
  void Validate(bool f_ignorewarnings);
  int createsamplefiles();
};

class VBContrast {
 public:
  string name;
  string scale;
  VB_Vector contrast;
  int parsemacro(tokenlist &line,int nvars,vector<int> &keeperlist);
  void print();
};

class TASpec {
public:
  void init();
  void addtrialset(double first,double interval,int count);
  int parsefile(string fname);
  int parseline(string line);
  VB_Vector getTrialAverage(VB_Vector &data);
  void print();

  string name;
  vector<double> startpositions;  // position in seconds of each trial start
  double interval;
  int nsamples;
  double TR;
  enum {ta_time,ta_vols} units;
};

vector<TASpec> parseTAFile(string fname);

class GLMInfo {
 public:
  string stemname;               // stem name for glm files
  string anatomyname;            // name of display volume file
  vector<string> teslist;        // list of tes files
  vector<Tes> tesgroup;          // the actual teses
  vector<string> cnames;         // all the covariate names, prepended with INKUD
  vector<VBContrast> contrasts;  // all the likely contrasts
  VBContrast contrast;           // the current contrast
  vector<TASpec> trialsets;
  int nvars;                     // number of variables in G matrix
  int dependentindex;            // which var, if any, in G is dependent
  int interceptindex;            // which var, if any, in G is the intercept
  uint32 glmflags;               // MEANSCALE, DETREND, AUTOCOR, and EXCLUDEERROR
  short rescount;                // number of residuals to save,
                                 // currently only used by volume
                                 // regression
  double effdf;                  // effective degrees of freedom, init to nan
  threshold thresh;              // structure for stat_threshold (RFT)

  // components of the ExoFilt kernel
  VB_Vector realExokernel,imagExokernel;

  // various bits of the GLM
  VBMatrix gMatrix,f1Matrix,rMatrix,f3Matrix;
  VB_Vector exoFilt,residuals,betas,traceRV;
  VB_Vector pseudoT;
  vector<int> keeperlist;      // indices of betas to keep
  vector<int> interestlist;    // indices of betas of interest
  vector<int> nointerestlist;  // indices of betas of no interest
  // results for volume regression
  Tes paramtes;
  Tes residtes;
  Cube statcube;               // most recently calculated stat cube
  Cube rawcube;                // raw stat cube, e.g., t values from which p vals derived
  Cube mask;                   // combined mask for GLM

  // public methods
  // time series from all tes files
  VB_Vector getTS(int x,int y,int z,uint32 flags=0);
  VB_Vector getRegionTS(VBRegion &rr,uint32 flags);
  VBMatrix getRegionComponents(VBRegion &rr,uint32 flags);
  VBRegion restrictRegion(VBRegion &rr);
  int filterTS(VB_Vector &signal);         // apply the exofilt
  int adjustTS(VB_Vector &signal);         // adjust for covariates of no interest
  int makeF1();                            // get or build F1
  // int makeKG();                            // get or build KG

  VB_Vector getResid(VBRegion &rr,uint32 flags);
  VB_Vector getResid(int x,int y,int z,uint32 flags);
  VB_Vector getCovariate(int x,int y,int z,int paramindex,int scaledflag);
  // NOT YET IMPLEMENTED
  VB_Vector calc_resid_R(int x,int y,int z);   // R*KX (above should adjudicate)
  VB_Vector calc_resid_fit(int x,int y,int z); // when there's no R
  VB_Vector load_resid(int x,int y,int z);     // find it in resid file

  GLMInfo();
  void init();
  void setup(string name);
  // the following are called by setup()
  void findstem(string name);
  void findanatomy();
  void findtesfiles();
  void getcovariatenames();
  void loadcontrasts();
  void loadtrialsets();
  void getglmflags();
  void loadcombinedmask();
  void initthresh();
  // VB_Vector subcontrast_keepers();

  // permutation vectors
  VB_Vector perm_signs;
  VB_Vector perm_order;
  void permute_if_needed(VB_Vector &vec);

  // regression-related
  int Regress(VB_Vector &timeseries);
  int RegressIndependent(VB_Vector &timeseries);
  int VecRegressX(uint32 flags=0);
  int VecRegress(vector<string> ivnames,string dvname);
  int TesRegress(int part,int nparts,uint32 flags=0);
  int VolumeRegress(Cube mask,int part,int nparts,vector<string>ivnames,string dvname,vector<VBMatrix> &ivmats);
  int calcbetas_nocor(VB_Vector& signal);
  int calcbetas(VB_Vector& signal);

  // timeseries stats
  int calc_stat();
  int calc_t();
  int calc_f();
  int calc_pct();
  int calc_beta();
  int calc_error();
  int calc_hyp();
  int calc_phase();
  int convert_t();
  int convert_f();
  int convert_t_cube();
  int convert_f_cube();
  double statval,rawval;

  // whole volume stats
  int calc_stat_cube();
  int calc_t_cube();
  int calc_f_cube();
  int calc_pct_cube();
  int calc_beta_cube();
  int calc_error_cube();
  int calc_hyp_cube();
  int calc_phase_cube();
  double calcfact();

  string statmapExists(string glmdir, VB_Vector &contrast, string scale);
  int parsecontrast(const string &str);
  void print();

 private:
};

// Functions for reading a condition function with strings (based on
// gdw's condition function loading)
int readCondFile(tokenlist &headerkey, tokenlist &output, const char *inputfile);
tokenlist getContentKey(tokenlist &inputLine);
void sortElement(tokenlist &inputList);
int getCondVec(const char *condFile, tokenlist &condKey, VB_Vector *condVec);
int cmpString(const char *inputString, deque<string> inputList);
int cmpElement(deque<string> input1, deque<string> input2);
int getCondLabel(tokenlist &outputToken, const char * inputFile);

// some generic functions moved from gdw
VB_Vector * upSampling(VB_Vector *inputtVector, int upRatio);
VB_Vector * downSampling(VB_Vector *inputVector, int downRatio);
VB_Vector getConv(VB_Vector *inputVector, VB_Vector *inputConv, int inputSampling, int tmpResolve);
VB_Vector fftConv(VB_Vector *inputVector, VB_Vector *convVector, bool zeroFlag);
double getDeterm(VBMatrix &);
double calcColinear(VBMatrix &ivs, VB_Vector &dv);
VB_Vector calcfits(VBMatrix &inMat, VB_Vector &inVec);
void calcDelta(VB_Vector *inputVec);

// simple functions to deal with input and outfut file check,
// originally written for vbfit
int checkOutputFile(const char *filename, bool ovwFlag);

// A simple function written for "add contrast"
int countNum(VB_Vector *inputVector, int m);
// A simple function written for "mean center non-zero"
int countNonZero(VB_Vector *inputVector);
// Three functions for adding multiple covariates from a txt file
int getTxtColNum(const char *);
int getTxtRowNum(const char *);
int readTxt(const char *inputFile, std::vector< VB_Vector *> txtCov);
VB_Vector * derivative(VB_Vector *);

int validscale(string scale);

#endif
