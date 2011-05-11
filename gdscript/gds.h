
// gds.h
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
// original version written by Dongbo Hu

#ifndef GDS_H
#define GDS_H

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include "vbprefs.h"
#include "vbutil.h"
#include "vbio.h"
#include "glmutil.h"

class Covar {
 public:
  Covar();
  ~Covar();
  void init();

  string varname;
  VB_Vector varvec;
  string group;
  char type;
  int cpCounter;
};

class Contrast {
 public:
  Contrast();
  ~Contrast();
  void init();

  bool scaleFlag, centerFlag;
  vector <VB_Vector> matrix;  
};

class diagonalSet {
 public:
  diagonalSet();
  ~diagonalSet();
  void init();

  bool scaleFlag, centerFlag;
};

/*****************************************************************
 *  gHeaderInfo class: written to load G matrix info for reading 
 *****************************************************************/
class gHeaderInfo
{
 public:
  gHeaderInfo();
  gHeaderInfo(VBMatrix );
  ~gHeaderInfo();
  void getInfo(VBMatrix );
  int chkInfo();

 private:
  void init();
  int chkCondfxn();

 public:
  int rowNum, colNum, TR, sampling;
  bool condStat;
  string condfxn;
  tokenlist condKey, typeList, nameList;
  VB_Vector *condVector;
};

class gSession {
 public:
  gSession();
  ~gSession();
  void init();
  void reInit();

  void writeG();
  int  chkSessionName(const char *);
  int  chkdirname(const char *);
  int  chkinfile(const char *);
  char chkType(string );
  int chkCovName(string );
  bool chkUniName(string );
  void end();
  void chkEnd();
  void reset();
  void chkCommon();
  void newCovMod();

  void addSingle();
  void chkSingle();
  void addIntercept();
  void addTrialfx();
  void addDiagonal();
  void addContrast();
  void chkCondition();
  void chkCondKey();
  int chkKeyName(string );

  void addVarTrialfx();
  void chkTrialFile();
  void addScanfx();
  void chkScanLen();
  void addGS();
  void chkGS();
  void addMP();
  void chkMP();
  void addSpike();
  void chkSpike(string, int);
  int chkTesStr(string);
  void chkTxt();
  void addTxt();

  void singleOpt(Covar &, string);
  void modCov();
  void mcNonZero(VB_Vector &);
  void unitExcursion(VB_Vector &);
  void modConvolve(VB_Vector &);
  int  chkOption(const char *);
  int  chkConvol(tokenlist );
  int  getCovID(string );
  string getFullName(unsigned );

  void cpCov();
  void derivMod();
  void expnMod();

  void modPlus();
  void chkModPlus();
  void eigenSet();
  void prepNew();
  void insertES();
  void firSet();
  void insertFIR();
  void fourierSet();
  void insertFS();
  void insertCovDC(int windowWidth);
  VB_Vector * fs_getFFT(VB_Vector *, VB_Vector *);
  char chkOrthType(string );
  void getOrthID(char );
  void orthogonalize(VB_Vector &);
  void delCov();
  void delAllCov();
  void resetTR();
  void resetSampling();
  void resetLen();
  void readG(const char *gdsName, const char *matName);
  bool chkG(const char *gdsName, const char *matName);
  bool chkPreG(const char *gdsName, const char *matName);
  void reset4openG();
  void buildCovList(const char *gdsName, const char *matName);
  void saveLabel(const char *);
  void saveCov(int, const char *);
  int chkDS(string );
  void resetEff();
  char chkEffType(string );
  bool typeMatch(char, int );
  void chkEffVar();
  void exeEff();
  // bool effMatch(unsigned );
  VB_Vector getFilterVec();
  double getRawEff(unsigned );
  double getBold(VB_Vector);
  double getSquareSum(VB_Vector );

  int startLine;
  int TR, totalReps, tmpResolve;
  int trialLen, ts;
  string inputFilename;
  bool gDirFlag;
  string dirname, gFilename, condfxn, condLabFile;
  VB_Vector condVec;
  tokenlist teslist, condKey, userCondKey, scanLen, covName;
  vector <long> lenList;
  vector <bool> tesReal;
  bool fakeTes;

  Covar newVar;
  bool newCovFlag;
  bool validity, validateOnly;
  bool singleFlag, diagonalFlag, contrastFlag;
  bool trialFlag, varTrialFlag, interceptFlag;
  bool scanFlag, gsFlag, mpFlag, spikeFlag, txtFlag, cpFlag;

  tokenlist option, convolOpt;
  Contrast newContrast;
  diagonalSet newDS;
  string singleFile, trialFile, txtFile;
  vector <int> spike;
  bool gsessionFlag, samplingFlag;
  bool doneCommon, doneCondition;

  int singleID;
  bool modCovFlag, derivFlag, expnFlag;
  int derivNum, derivIndex;
  double expnNum;

  bool modPlusFlag, esFlag, firFlag, fsFlag;
  vector <Covar> newList;
  int esIndex, firIndex, fsIndex;
  int firNum, fsPeriod, fsHarmonics;
  bool fsZeroFreq, fsDeltaCov;

  int multiplyID;
  bool orthFlag, orthTypeFlag, orthNameFlag;
  char orthType;
  vector <int> orthID;
  vector <int> delID;
  //vector <int> multiID;

  bool effFlag, effTypeFlag, dsFlag, cutoffFlag, filterFlag;
  bool meanAll;
  int effBaseIndex;
  double baseEff;
  char effType;
  int dsOption;
  double cutoff;
  string effFilter;

  vector <Covar> covList;
};

class scriptReader {
 public:
  scriptReader();
  ~scriptReader();
  bool parseFile(string , int inLineNum = 0);
  void makeAllG();

  vector <gSession> sessionList;
  bool validity, validateOnly;
  int gcounter;
};

#endif
