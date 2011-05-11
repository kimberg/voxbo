
// gds.cpp
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

using namespace std;

#include "gds.h"

extern VBPrefs vbp;

const int BUFLEN = 1024;
int lineNum;
gSession mySession;

/**********************************************************
 * Covar class: holds a single covariate's information
 **********************************************************/
/* Covar constructor */
Covar::Covar()
{
  init();
}

/* Destructor */
Covar::~Covar()
{

}

/* Simple initialization */
void Covar::init()
{
  varname = "undefined";
  group = "";
  type = 'u';
  cpCounter = 0;
}

/**********************************************************
 * Contrast class: holds contrast covariate's options
 * It includes three paremeters: 
 * scale flag, center-norm flag, contrast matrix
 **********************************************************/
/* Constructor */
Contrast::Contrast()
{
  init();
}

/* Desctructor */
Contrast::~Contrast()
{
  init();
}

/* Initialization */
void Contrast::init()
{
  scaleFlag = true;
  centerFlag = true;

  if (matrix.size())
    matrix.clear();
}

/**********************************************************
 * diagonalSet class: holds diagonal set's options
 * It includes only two paremeters: 
 * scale flag, center-norm flag
 **********************************************************/
/* Constructor */
diagonalSet::diagonalSet()
{
  init();
}

/* Desctructor */
diagonalSet::~diagonalSet()
{
  init();
}

/* Initialization */
void diagonalSet::init()
{
  scaleFlag = false;
  centerFlag = true;
}

/*********************************************************************
 * scriptReader class: reads the whole input gds file and build the 
 * corresponding G matrix, could have multiple gSessions
 *********************************************************************/
/* Constructor */
scriptReader::scriptReader()
{
  validity = true;
  validateOnly = false;
  gcounter = 0;
}

/* Desctructor */
scriptReader::~scriptReader()
{

}

/* build G matrix files for each session */
void scriptReader::makeAllG()
{
  for (int i = 0; i < (int) sessionList.size(); i++)
    sessionList[i].writeG();
}

/* Parse the input file */
bool scriptReader::parseFile(string filename, int inLineNum)
{
  ifstream infile;
  char buf[BUFLEN];
  tokenlist args;
  //stringstream tmps;

  infile.open(filename.c_str());
  if (!infile) {
    printf("GDS I/O ERROR: couldn't open %s\n", filename.c_str());
    return false;  // error!
  }

  lineNum = 0;  // Make sure line number is always counted in local file
  while (infile.getline(buf,BUFLEN,'\n')) {
    lineNum++;
    args.ParseLine(buf); // ParseLine() already takes care of comment lines ("#" or ";" lines)
    if (!args.size())
      continue;

    // "include will combine another script file
    if (args[0] == "include") {
      if (args.size() == 2) {
        int orgLineNum = lineNum;
        validity = parseFile(args[1], orgLineNum);
        lineNum = orgLineNum;
        mySession.inputFilename = filename; 
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- include needs one and only one argument: <another gds filename>\n");
        mySession.validity = false;
      }
      continue; 
    }

    // "gsession" is followed by a brand-new G matrix design session
    if (args[0] == "gsession") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        mySession.gsessionFlag = true;
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- gsession is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
        continue;
      }
      // If there is previous session before this one
      if (gcounter) {
        mySession.chkEnd();
        // Final check of condition function (in case it has never been used)
        if (mySession.condfxn.length() && !mySession.doneCondition)
          mySession.chkCondition();

        // Make sure previous session does include covariate(s)
        if (mySession.validity && !validateOnly && !mySession.covList.size()) {
          printf("GDS ERROR in %s [line %d]: %s session doesn't include any covariates\n", 
                 filename.c_str(), lineNum, mySession.dirname.c_str());
          mySession.validity = false;
        }

        validity = (validity && mySession.validity);
        // Take care of the previous session
        if (mySession.validity && !validateOnly)
          //  sessionList.push_back(mySession);
          mySession.writeG();
      }

      mySession.reInit();
      mySession.startLine = lineNum;
      mySession.gsessionFlag = true;
      mySession.inputFilename = filename;
      mySession.validateOnly = this->validateOnly;

      if (args.size() != 2) {
        printf("gds syntax error in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- gsession takes exactly one argument\n");
        mySession.validity = false;
      }
      else {
        int gFileStat = mySession.chkSessionName(args(1));
        if (gFileStat == 0) {
          printf("gds error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- Parent directory not exist: %s\n", xdirname(args[1]).c_str());
          mySession.validity = false;
        }
        else if (gFileStat == 1) {
          printf("gds error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- Parent directory not writeable: %s\n", xdirname(args[1]).c_str());
          mySession.validity = false;
        }
        else if (gFileStat == 2) {
          printf("gds error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- Directory exists: %s\n", args(1));
          mySession.validity = false;
        }
        else if (gFileStat == 3) {
          string pregStr = xrootname(args[1]) + ".preG";
          printf("gds error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- Directory exists: %s\n", pregStr.c_str());
          mySession.validity = false;
        }
      }
      gcounter++;
    }
    // Make sure all other tags have a "gSession" tag before it
    else if (!mySession.gsessionFlag) {
      printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
      printf("--- gsession line not found before %s\n", args(0));
      mySession.validity = false;
      continue;
    }
    // openG opens an existing G matrix file
    else if (args[0] == "openG") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- openG is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
        continue;
      }
      if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- openG needs one and only one argument: <G matrix filename>\n");
        mySession.validity = false;
        continue;
      }
      if (mySession.chkG(filename.c_str(), args(1))) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- openG is reading a G matrix file that already exists.\n");
        printf("    Previous covariates will be deleted.\n");
        printf("    It may also clear previous scan definition and overwrite TR/length/sampling values.\n");
        mySession.readG(filename.c_str(), args(1));
      }
    }

    // Read tes files
    else if (args[0] == "scan") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scan is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
        continue;
      }
      if (mySession.doneCommon) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scan lines must be before newcov/modcov/modcov+ section\n");
        mySession.validity = false;
        continue;
      }
      if (args.size() == 2) {
        if (!mySession.chkinfile(args(1))) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum); 
          printf("--- invalid scan file: %s\n", args(1));
          mySession.validity = false;
          continue;
        }
        mySession.teslist.Add(args[1]);
        mySession.tesReal.push_back(true);
        mySession.lenList.push_back(-1);
      }
      else if (args.size() == 3) {
        mySession.teslist.Add(args[1]);
        mySession.tesReal.push_back(false);
        mySession.fakeTes = true;
        mySession.lenList.push_back(strtol(args(2)));
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scan line should be written as:\n");
        printf("    scan <real tes filename> or: scan <fake name> length\n");
        mySession.validity = false;
      }
    }
    // Read TR
    else if (args[0] == "TR" || args[0] == "tr") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (mySession.doneCommon) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- TR is being reset, covariates defined before this line will be cleared\n");
        mySession.resetTR();
      }
      else if (mySession.TR) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- TR is being reset, previous value will be overwritten\n");
      }

      if (args.size() == 2) {
        if (strtol(args[1]) > 0)
          mySession.TR = strtol(args[1]);
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid TR: %s\n", args(1));
          mySession.validity = false;
        }
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- TR needs one and only one argument: <TR value (unit: ms)>\n");
        mySession.validity = false;
      }
    }
    // Read upsampling rate
    else if (args[0] == "sampling") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (mySession.doneCommon) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- sampling is being reset, covariates defined before this line will be cleared\n");
        mySession.resetSampling();
      }
      else if (mySession.samplingFlag) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- sampling already exists, previous definition will be overwritten\n");
      }
      if (args.size() == 2) {
        if (strtol(args[1]) > 0) {
          mySession.tmpResolve = strtol(args[1]);
          mySession.samplingFlag = true;
        }
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid sampling rate: %s\n", args[1].c_str());
          mySession.validity = false;
        }
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- sampling needs one and only one argument: <upsampling rate (unit: ms)>\n");
        mySession.validity = false;
      }
    }
    // Define total number of time points (images)
    else if (args[0] == "length") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (mySession.doneCommon) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- length is being reset, covariate(s), scan file(s) and condition function\n");
        printf("     defined before this line will be cleared\n");
        mySession.resetLen();
      }
      else if (mySession.totalReps) {
        printf("GDS syntax WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- length line already exists, previous definition will be overwritten\n");
      }
      if (args.size() == 2) {
        if (strtol(args[1]))
          mySession.totalReps = strtol(args[1]);
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid length: %s\n", args[1].c_str());
          mySession.validity = false;
        }
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- length needs one and only one argument: <# of images>\n");
        mySession.validity = false;
      }
    }
    // Define condition function
    else if (args[0] == "condition") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition needs one and only one argument: <condition function filename>\n");
        mySession.validity = false;
        continue;
      }
      if (mySession.chkinfile(args(1))) {
        if (mySession.condfxn.length()) {
          printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- condition line already exists, previous definition will be overwritten\n");
        }
        mySession.condfxn = args[1];
        mySession.doneCondition = false;
      }
      else {
        printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid condition function file: %s\n", args(1));
        mySession.validity = false;
      }
    }
    /* Define condition label file, it needs one and only one argument as filename */
    else if (args[0] == "condition-label-file") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (!mySession.condfxn.length()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition function not defined\n");
        mySession.validity = false;
        continue;
      }
      if (mySession.condLabFile.length()) {
        printf("GDS WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition-label-file line already exists.\n");
        printf("    Previous definition will be overwritten\n");
      }
      if (mySession.userCondKey.size()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition-label-name line already exists\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition-label-file needs one and only one argumentas: <condition label filename>\n");
        mySession.validity = false;
      }
      else if (!mySession.chkinfile(args(1))) {
        printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid condition label file: %s\n", args(1));
        mySession.validity = false;
      }
      else {
        mySession.condLabFile = args[1];
        mySession.doneCondition = false;
      }
    }
    /* Define condition label strings */
    else if (args[0] == "condition-label-name") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (!mySession.condfxn.length()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition function not defined\n");
        mySession.validity = false;
        continue;
      }
      if (mySession.condLabFile.length()) {
        printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition-label-file already exists\n");
        mySession.validity = false;
      }
      else if (args.size() == 1) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition-label-name takes at least one argument\n");
        mySession.validity = false;
      }
      else {
        for (size_t i = 0; i < args.size() - 1; i++)
          mySession.userCondKey.Add(args[1+i]);
        mySession.doneCondition = false;
      }
    }
    /* modify a certain condition label's name */
    else if (args[0] == "mod-condition-label") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (!mySession.condfxn.length()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition function not defined\n");
        mySession.validity = false;
        continue;
      }
      if (args.size() != 3) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- mod-condition-label needs two and only two arguments:\n");
        printf("    <original condition label name> <new condition label name>\n");
        mySession.validity = false;
      }	
      else {
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (mySession.validity) {
          if (mySession.chkKeyName(args[1]) == -1) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- condition label name not found: %s", args(1));
            mySession.validity = false;
          }
          else
            mySession.condKey[mySession.chkKeyName(args[1])] = args[2];
        }
      }
    }
    /* save condition labels as a txt file */
    else if (args[0] == "save-condition-label") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
        continue;
      }
      if (!mySession.condfxn.length()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- condition function not defined\n");
        mySession.validity = false;
        continue;
      }
      if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- save-condition-label needs one and only one argument: <txt filename>\n");
        mySession.validity = false;
      }	
      else {
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (mySession.validity) {
          int fileStat = checkOutputFile(args(1), false);
          if (fileStat == 0) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- %s already exists, parent directory not writeable\n", args(1));
            mySession.validity = false;
          }	
          else if (fileStat == 1) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- %s already exists\n", args(1));
            mySession.validity = false;
          }
          else if (fileStat == 2) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- parent directory not writable: %s\n", args(1));
            mySession.validity = false;
          }
          else if (!validateOnly)
            mySession.saveLabel(args(1));
        }
      }
    }
    /* save a certain covariate as a ref file */
    else if (args[0] == "save-cov") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
      }
      else if (args.size() != 3) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- save-cov needs two and only two arguments: <covariate name> <ref filename>\n");
        mySession.validity = false;
      }	
      else if (!validateOnly) {
        int selection = mySession.getCovID(args[1]);
        int fileStat = checkOutputFile(args(2), false);
        if (selection == -1) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- covariate name not found: %s\n", args(1));
          mySession.validity = false;
        }
        else if (selection == -2) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- more than one covariates match the name: %s\n", args(1));
          mySession.validity = false;
        }
        else if (fileStat == 0) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- %s already exists, parent directory not writeable\n", args(2));
          mySession.validity = false;
        }	
        else if (fileStat == 1) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- %s already exists\n", args(2));
          mySession.validity = false;
        }
        else if (fileStat == 2) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- parent directory not writable: %s\n", args(2));
          mySession.validity = false;
        }
        else if (mySession.validity)
          mySession.saveCov(selection, args(2));
      }
    }
    // mean-center-all
    else if (args[0] == "mean-center-all") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- %s is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n", args(0));
        mySession.validity = false;
      }
      else if (args.size() != 1) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- mean-center-all does NOT need any arguments\n");
        mySession.validity = false;
      }	
      else
        mySession.meanAll = true;
    }
    /****************************************************
     * gds syntax lines that start with "newcov" keyword
     ****************************************************/
    else if (args[0] == "newcov") {
      if (!mySession.doneCommon)
        mySession.chkCommon();
      mySession.chkEnd();

      if (args.size() < 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- newcov needs at least one argument\n");
        mySession.validity = false;
      }
      // Add single covariate
      else if (args[1] == "single") {
        mySession.newCovFlag = true;
        mySession.singleFlag = true;
        if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- newcov single takes one and only one argument: <ref filename>\n");
          mySession.validity = false;
        }
        else if (!mySession.chkinfile(args(2))) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid covariate file: %s\n", args(2));
          mySession.validity = false;
        }
        else {
          // Default name is "undefined"
          mySession.newVar.varname = "undefined";
          // Default type is 'I'
          mySession.newVar.type = 'I';
          mySession.singleFile = args[2];	
        }
      }
      // Old tag for adding single covariate, (obsolescent, kept for compatibility)
      else if (args[1] == "add-new") {
        printf("GDS SYNTAX WARNING in %s [line %d]:\n", filename.c_str(), lineNum); 
        printf("--- add-new is an obsolescent tag, recommended format to add single covariate:\n");
        printf("\tnewcov single <ref file name>\n");
        mySession.chkEnd();
        mySession.newCovFlag = true;
        mySession.singleFlag = true;
      	// Default type is 'I'
        mySession.newVar.type = 'I';
        if (args.size() == 2)
          mySession.newVar.varname = "undefined";
        else if (args.size() == 3) 
          mySession.newVar.varname = args[2];
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-new takes one and only one argument: <ref filename>\n");
          mySession.validity = false;
        }
      }
      // Add intercept
      else if (args[1] == "intercept" || args[1] == "Intercept") {
        mySession.newCovFlag = true;
        mySession.interceptFlag = true;
        // Default type is 'K'	
        mySession.newVar.type = 'K';
        // Default name
        if (args.size() == 2)
          mySession.newVar.varname = "Intercept";
        // Use the argument as name
        else if (args.size() == 3) 
          mySession.newVar.varname = args[2];
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- intercept takes an optional argument (covariate name)\n");
          mySession.validity = false;
        }
      }
      // Obsolescent tag for intercept, kept for compatibility
      else if (args[1] == "add-intercept") {
        printf("GDS SYNTAX WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- add-intercept is an obsolescent tag, use intercept instead\n");
        mySession.newCovFlag = true;
        mySession.interceptFlag = true;
        // Default type is 'N'	
        mySession.newVar.type = 'K';
        // Default name
        if (args.size() == 2)
          mySession.newVar.varname = "Intercept";
        // Use the argument as name
        else if (args.size() == 3) 
          mySession.newVar.varname = args[2];
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-intercept takes an optional argument (covariate name)\n");
          mySession.validity = false;
        }
      }
      // "trial effect" takes one argument 
      else if (args[1] == "trial-effect") {
        mySession.newCovFlag = true;
        mySession.trialFlag = true;
        if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- trial-effect needs one and only one argument\n");
          mySession.validity = false;
        }
        else {
          int tmpInt = atoi(args(2));
          if (tmpInt <= 0) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- invalid trial effect value: %s\n", args(2));
            mySession.validity = false;
          }
          else {
            mySession.trialLen = tmpInt;
            // Default type is 'I'
            mySession.newVar.type = 'I';
          }
        }
      }
      // Add diagonal set
      else if (args[1] == "diagonal") {
        mySession.newCovFlag = true;
        mySession.diagonalFlag = true;
      	// Default type is 'I'
        mySession.newVar.type = 'I';
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (!mySession.condKey.size()) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- diagonal requires a valid condition function\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- diagonal doesn't take any argument\n");
          mySession.validity = false;
        }
      }
      // Obsolescent tag to add diagonal set
      else if (args[1] == "add-diagonal") {
        printf("GDS SYNTAX WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- add-diagonal is an obsolescent tag, use diagonal instead\n");
        mySession.chkEnd();
        mySession.newCovFlag = true;
        mySession.diagonalFlag = true;
      	// Default type is 'I'
        mySession.newVar.type = 'I';
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (!mySession.condKey.size()) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-diagonal requires a valid condition function\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-diagonal doesn't take any argument\n");
          mySession.validity = false;
        }
      }
      // Add contrasts
      else if (args[1] == "contrast") {
        mySession.newCovFlag = true;
        mySession.contrastFlag = true;
      	// Default type is 'I'
        mySession.newVar.type = 'I';
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (!mySession.condKey.size()) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- contrast requires a valid condition function\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- contrast doesn't take any argument\n");
          mySession.validity = false;
        }
      }
      // Obsolescent tag to add contrasts, kept for compatibility
      else if (args[1] == "add-contrast") {
        printf("GDS SYNTAX WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- add-contrast is an obsolescent tag, use diagonal instead\n");
        mySession.chkEnd();
        mySession.newCovFlag = true;
        mySession.contrastFlag = true;
      	// Default type is 'I'
        mySession.newVar.type = 'I';
        if (!mySession.doneCondition)
          mySession.chkCondition();
        if (!mySession.condKey.size()) {
          printf("gds syntax error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-contrast requires a valid condition function\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("gds syntax error in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- add-contrast doesn't take any argument\n");
          mySession.validity = false;
        }
      }
      // Add var length trial effects
      else if (args[1] == "var-trialfx") {
        mySession.newCovFlag = true;
        mySession.varTrialFlag = true;
        if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- newcov var-trialfx needs one and only one argument: <trial file path>\n");
          mySession.validity = false;
        }
        else if (!mySession.chkinfile(args(2))) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid var length trialfx file: %s\n", args(2));
          mySession.validity = false;
        }
        else {
          // Default type is 'I'
          mySession.newVar.type = 'I';
          mySession.trialFile = args[2];
        }
      }
      // Add scan effect
      else if (args[1] == "scan-effect") {
        mySession.newCovFlag = true;
        mySession.scanFlag = true;
        // Default type is 'N'
        mySession.newVar.type = 'N';
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum); 
          printf("--- scan-effect doesn't take any argument\n");
          mySession.validity = false;
        }
        else if (mySession.teslist.size() == 1) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- only one tes file available, scan-effect line is illegal\n");
          mySession.validity = false;
        }
      }
      // Add global signals
      else if (args[1] == "global-signal") {
        mySession.newCovFlag = true;
        mySession.gsFlag = true;
        // Default type is 'N'
        mySession.newVar.type = 'N';
        if (mySession.fakeTes) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fake tes files defined, GS file not exist\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- global-signal doesn't take any argument\n");
          mySession.validity = false;
        }
        else if (!mySession.teslist.size()) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- tes file(s) must be defined for global signal design\n");
          mySession.validity = false;
        }
      }
      // Add movement parameters
      else if (args[1] == "move-params") {
        mySession.newCovFlag = true;
        mySession.mpFlag = true;
        // Default ype is 'N'
        mySession.newVar.type = 'N';
        if (mySession.fakeTes) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fake tes files found, movement parameter file not exist\n");
          mySession.validity = false;
        }
        if (args.size() > 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- move-params doesn't take any argument\n");
          mySession.validity = false;
        }
        else if (!mySession.teslist.size()) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- tes file(s) must be defined for movement parameter design\n");
          mySession.validity = false;
        }
      }
      // Spike covariate(s) is a new functionality
      else if (args[1] == "spike") {
        mySession.newCovFlag = true;
        mySession.spikeFlag = true;
        // Default type is 'N'
        mySession.newVar.type = 'N';
        if (args.size() == 3)
          mySession.chkSpike(args[2], -1);
        else if (args.size() > 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum); 
          printf("--- spike only takes an optional argument: absolute spike position\n");
          mySession.validity = false;
        }
      }
      // Add multiple column txt file
      else if (args[1] == "txt-file") {
        mySession.newCovFlag = true;
        mySession.txtFlag = true;   
        // Default type is 'N'
        mySession.newVar.type = 'N';   
        if (args.size() == 3) 
          mySession.txtFile = args[2];
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- txt-file requires a multiple column txt file\n");
          mySession.validity = false;
        }
      }
      // Copy a certain covariate
      else if (args[1] == "cp") {
        mySession.newCovFlag = true;
        mySession.cpFlag = true;   
        if (args.size() == 3) { 
          if (!mySession.validateOnly) {
            mySession.singleID = mySession.getCovID(args[2]);
            if (mySession.singleID == -1) {
              printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
              printf("--- covariate name not found: %s\n", args(2));
              mySession.validity = false;
            }
            else if (mySession.singleID == -2) {
              printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
              printf("--- more than one covariates match the name: %s\n", args(2));
              mySession.validity = false;
            }
          }
        }
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- newcov cp requires exactly one argument: covariate name\n");
          mySession.validity = false;
        }
      }
      // unknown tag after newcov
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- unknown tag in newcov line: %s\n", args[1].c_str());
        mySession.validity = false;
      }
    }

    /*********************************************************
     * gds syntax lines between "newcov xxx" and "end" tags
     *********************************************************/

    // type: interest / NoInterest / KeepNoInterest
    else if (args[0] == "type") {
      if (!mySession.newCovFlag && !mySession.modCovFlag && !mySession.modPlusFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- newcov/modcov/modcov+ line not found before type\n");
        mySession.validity = false;
        continue;
      }
      char foo = mySession.chkType(args[1]);
      if (args.size() == 2 && foo)
        mySession.newVar.type = foo;
      else if (args.size() == 2 && !foo) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid covariate type: %s\n", args(1));
        mySession.validity = false;
      }
      else { 
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- type needs one and only one argument: <covariate type (I/N/K)>\n");
        mySession.validity = false;
      }	
    }
    // "varfile" is obsolescent, kept for compatibility
    else if (args[0] == "varfile") {
      printf("GDS SYNTAX WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
      printf("--- varfile is an obsolescent tag, recommended format to add single covariate is:\n");
      printf("\tnewcov single <ref file name>\n");
      if (!mySession.singleFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- varfile is valid only for add-new\n");
        mySession.validity = false;
      }
      else if (args.size() == 2 && mySession.chkinfile(args(1)))
        mySession.singleFile = args[1];
      else if (args.size() == 2 && !mySession.chkinfile(args(1))) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid covariate file: %s\n", args(1));
        mySession.validity = false;
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- varfile takes exactly one argument\n");
        mySession.validity = false;
      }
    }   
    // Geoff's requst: customize new covariate's  name in script
    else if (args[0] == "cov-name") {
      if (!mySession.newCovFlag && !mySession.modCovFlag && !mySession.modPlusFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("---  newcov/modcov/modcov+ before cov-name\n");
        mySession.validity = false;
        continue;
      }
      if (args.size() > 1) {
        for (size_t i = 1; i < args.size(); i++) { 
          if (mySession.chkCovName(args[i]) == -1) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- \"->\" is not allowed in covarite name string\n");
            mySession.validity = false;
          }
          else if (mySession.chkCovName(args[i]) == -2) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- same covariate name not allowed in one section: %s\n", args(i));
            mySession.validity = false;
          }
          else if (!mySession.validateOnly && !mySession.chkUniName(args[i])) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- Covariate name already used in same gSession: %s\n", args(i));
            mySession.validity = false;
          }
          else
            mySession.covName.Add(args[i]);
        }
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- cov-name requires at least one argument\n");
        mySession.validity = false;
      }
    }

    // group is an optional keyword for new covariate
    // dhubug: when "group" tag is met, check the new group name plus new or 
    // existing covariate name to make sure the combination is unique.
    // (Similar to the name check for cov-name. This step is not done yet.)
    else if (args[0] == "group") {
      if (!mySession.newCovFlag && !mySession.modCovFlag && !mySession.modPlusFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
    	printf("--- group must be in newcov/modcov/modcov+ session\n");
    	mySession.validity = false;
      }
      else if (mySession.newVar.group.length()) {
        printf("GDS syntax WARNING in %s [line %d]:\n", filename.c_str(), lineNum);
    	printf("--- Redundant group line: group already defined in this section\n");
      }
      else if (args.size() == 2)
    	mySession.newVar.group = args[1];
      else {
    	printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
    	printf("--- group needs one and only one argument: <covariate group name>\n");
    	mySession.validity = false;
      }
    }

    // "option" is simulating "modify" menu
    else if (args[0] == "option") {
      if (!mySession.newCovFlag && !mySession.modCovFlag && !mySession.modPlusFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- option must be in newcov/modcov/modcov+ session\n");
        mySession.validity = false;
      }
      else if (args.size() < 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- option takes at least one argument\n");
        mySession.validity = false;
      }
      // "convolve" option is special because it takes at least two arguments
      else if (args[1] == "convolve") {
        for (size_t i = 2; i < args.size(); i++)
          mySession.convolOpt.Add(args[i]);
        int convStat = mySession.chkConvol(mySession.convolOpt);
        if (convStat == 0)
          mySession.option.Add("convolve");
        else {
          mySession.convolOpt.clear();
          mySession.validity = false;
        }
      }
      else if (args[1] == "time-shift") {
        if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option time-shift needs one and only one argument: <time shift value (unit: ms)>\n");
          mySession.validity = false;
        }
        else {
          mySession.ts = atoi(args[2].c_str());
          if (!mySession.ts) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- invalid time shift value: %s\n", args[2].c_str());
            mySession.validity = false;
          }
          else
            mySession.option.Add("ts");
        }
      }
      // "derivative" adds new covariate(s) based on an existing one;
      // it is limited in "newcov cp" session
      else if (args[1] == "derivative") {
        if (!mySession.cpFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- derivative option is ONLY allowed in \"newcov cp\" section\n");
          mySession.validity = false;
        }
        else if (mySession.derivFlag || mySession.expnFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- derivative option is exclusive with itself or exponential option\n");
          printf("    Use another modcov section for these actions instead\n");	  
          mySession.validity = false;
        }	  
        else if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option derivative needs one and only one argument: <# of derivative>\n");
          mySession.validity = false;
        }
        else {
          int tmpInt = atoi(args[2].c_str());
          if (tmpInt <= 0) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- invalid derivative number (must be a positive integer): %s\n", args[2].c_str());
            mySession.validity = false;
          }
          else if ((mySession.totalReps * mySession.TR / mySession.tmpResolve) % 2) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- derivative option not allowed for covariate with odd length\n");
            mySession.validity = false;
          }
          else {
            mySession.derivFlag = true;
            mySession.derivIndex = mySession.option.size();
            mySession.derivNum = tmpInt;
            mySession.option.Add("deriv");
          }
        }
      }
      // "exponential" adds a new covariate based on an existing one;
      // it is limited in "newcov cp" session
      else if (args[1] == "exponential") {
        if (!mySession.cpFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- exponential option is ONLY allowed in \"newcov cp\" section\n");
          mySession.validity = false;
        }
        else if (mySession.derivFlag || mySession.expnFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- exponential option is exclusive with itself or derivative:\n");
          printf("    Use another modcov section for these actions instead\n");	  
          mySession.validity = false;
        }	  
        else if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option exponential needs one and only one argument: <# of exponential>\n");
          mySession.validity = false;
        }
        else {
          double tmpVal = atof(args[2].c_str());
          if (tmpVal == 0) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- invalid exponential number: %s\n", args[2].c_str());
            mySession.validity = false;
          }
          else {
            mySession.expnFlag = true;
            mySession.expnNum = tmpVal;
            mySession.option.Add("expn");
          }
        }
      }
      // "eigen-set" is eigenvector set option, it removes the original covariate 
      // and adds three new covariates; it is limited in modcov+ section
      else if (args[1] == "eigen-vector") {
        if (!mySession.modPlusFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- eigen-vector option is ONLY allowed in \"modcov+\" section\n");
          mySession.validity = false;
        }
        else if (mySession.esFlag || mySession.firFlag || mySession.fsFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- eigen-vector option is exclusive with itself or the following options:\n");
          printf("    fir fourier\n");
          printf("    Use another modcov+ section for these actions instead\n");	  
          mySession.validity = false;
        }	  
        else if (args.size() != 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option eigen-vector does NOT need any argument\n");
          mySession.validity = false;
        }
        else {
          mySession.esFlag = true;
          mySession.esIndex = mySession.option.size();
          mySession.option.Add("es");	  
        }
      }

      // "fir" is finite impulse response option, it modifies original covariate and 
      // adds a few new ones; it is limited in modcov+ section
      else if (args[1] == "fir") {
        if (!mySession.modPlusFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fir option is ONLY allowed in \"modcov+\" section\n");
          mySession.validity = false;
        }
        else if (mySession.esFlag || mySession.firFlag || mySession.fsFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fir option is exclusive with itself or the following options:\n");
          printf("    eigen-vector fourier\n");
          printf("    Use another modcov+ section for these actions instead\n");	  
          mySession.validity = false;
        }	  
        else if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option fir needs one and only one argument: <# of TR>\n");
          mySession.validity = false;
        }
        else {
          int tmpInt = atoi(args[2].c_str());
          if (tmpInt <= 0) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- fir's argument must be a positive integer: %s\n", args[2].c_str());
            mySession.validity = false;
          }
          else {
            mySession.firFlag = true;
            mySession.firIndex = mySession.option.size();
            mySession.firNum = tmpInt;
            mySession.option.Add("fir");
          }
        }
      }
      // "fourier-set" is Fourier set option, it removes original covariate and 
      // adds a few new ones; it is limited in modcov+ section
      else if (args[1] == "fourier-set") {
        if (!mySession.modPlusFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fourier-set option is ONLY allowed in \"modcov+\" section\n");
          mySession.validity = false;
        }
        else if (mySession.esFlag || mySession.firFlag || mySession.fsFlag) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fourier-set option is exclusive with itself or the following options:\n");
          printf("    eigen-vector fourier\n");
          printf("    Use another modcov+ section for these actions instead\n");	  
          mySession.validity = false;
        }	  
        else if (args.size() != 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option fourier-set doesn't need argument\n");
          printf("    Use fs-peroid, fs-harmonics, fs-zero-freq and fs-alter-cov to define the parameters\n");
          mySession.validity = false;
        }
        else {
          mySession.fsFlag = true;
          mySession.esIndex = mySession.option.size();
          mySession.option.Add("fir");
       	}
      }
      // "multiply" is a regular option that accepts one argument as covariate name
      else if (args[1] == "multiply") {
        if (args.size() != 3) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option multiply needs one and only one argument: <covariate name>\n");
          mySession.validity = false;
        }
        else if (!mySession.validateOnly) {
          int tmpVal = mySession.getCovID(args[2]);
          if (tmpVal == -1) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- covariate name not found: %s\n", args(2));
            mySession.validity = false;
          }
          else if (tmpVal == -2) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- more than one covariates match the name: %s\n", args(2));
            mySession.validity = false;
          }
          else {
            mySession.multiplyID = tmpVal;
            mySession.option.Add("multi");
          }
        }
        else
          mySession.option.Add("multi");
      }
      // "orthog" does orthogonalization, it is a regular option that doesn't need any argument
      else if (args[1] == "orthog") {
        if (args.size() != 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- option orthog does NOT need argument;\n");
          printf("--- please use orth-type or orth-name tag to set its parameter.\n");
          mySession.validity = false;
        }
        else {
          mySession.option.Add("orth");
          mySession.orthFlag = true;
        }
      }
      // For regular options that only modify covariate and don't need any arguments
      // They are: mean-center, mean-center-non-zero, unit-variance, unit-excursion, convert-delta
      else if (args.size() == 2) {
        if (mySession.chkOption(args(1)))
          mySession.option.Add(args[1]);
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- unknown option: %s\n", args(1));
          mySession.validity = false;
        }
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- unknown option: %s\n", args(1));
        mySession.validity = false;
      }
    }

    // 1st tag for orthogalization parameter: covariate type (exclusive with orth-name)
    else if (args[0] == "orth-type") {
      if (!mySession.orthFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- orth-type is only valid for orthog option\n");
        mySession.validity = false;
      }
      else if (mySession.orthNameFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- orth-name line found in the same section\n");
        printf("--- orth-name and orth-type are exclusive\n");
        mySession.validity = false;
      }
      else {
        mySession.orthTypeFlag = true;
        if (args.size() != 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- orth-type takes one and only one argument: <covariate type>\n");
          mySession.validity = false;
        }
        else if (!mySession.chkOrthType(args[1])) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- unknown covariate type for orthogonalization: %s\n", args(1));
          mySession.validity = false;
        }
        else if (!mySession.validateOnly) {
          char tmpChar = mySession.chkOrthType(args[1]);
          mySession.getOrthID(tmpChar);
          if (!mySession.orthID.size()) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- independent covariate of type %s not found\n", args(1));
            mySession.validity = false;
          }
        }
      }
    }
    // 2nd tag for orthogalization parameter: covariate name(s) (exclusive with orth-type)
    else if (args[0] == "orth-name") {
      if (!mySession.orthFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- orth-name is only valid for orthog option\n");
        mySession.validity = false;
      }
      else if (mySession.orthTypeFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- orth-type line found in the same section\n");
        printf("--- orth-name and orth-type are exclusive\n");
        mySession.validity = false;
      }
      else {
        mySession.orthNameFlag = true;
        if (args.size() < 2) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- orth-name needs at least one argument: <covariate name>\n");
          mySession.validity = false;
        }
        else if (!mySession.validateOnly) {
          int orthIndex;
          for (size_t x = 1; x < args.size(); x++) {
            orthIndex = mySession.getCovID(args[x]);
            if (orthIndex == -1) {
              printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
              printf("--- covariate name not found: %s\n", args(x));
              mySession.validity = false;
            }
            else if (orthIndex == -2) {
              printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
              printf("--- more than one covariates match the name: %s\n", args(x));
              mySession.validity = false;
            }
            else if (orthIndex == mySession.singleID) {
              printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
              printf("--- covariate has been selected for modification: %s\n", args(x));
              mySession.validity = false;
            }
            else
              mySession.orthID.push_back(orthIndex);
          }
        }
      }
    }

    // Fourier set parameter: period to model (integer)
    else if (args[0] == "fs-period") {
      if (!mySession.fsFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-period is only valid for fourier-set option\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-period takes one and only one argument: <period value (unit: second)>\n");
        mySession.validity = false;
      }
      else {
        if (!mySession.doneCommon)
          mySession.chkCommon();
        double totalTime = (double) mySession.TR * mySession.totalReps / 1000.0;
        int period = atoi(args(1));
        if (period <= 0) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- invalid fs-period value: %s\n", args(1));
          mySession.validity = false;
        }
        else if ((double) period > totalTime) {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- fs-period must be less than or equal to %d seconds\n", (int)totalTime);
          mySession.fsPeriod = period;
          mySession.validity = false;
        } 
        else
          mySession.fsPeriod = period;
      }
    }
    // Fourier set parameter: number of harmonics (integer)
    else if (args[0] == "fs-harmonics") {
      if (!mySession.fsFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-harmonics is only valid for fourier-set option\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-harmonics takes one and only one argument: <# of harmonics>\n");
        mySession.validity = false;
      }
      else if (atoi(args(1)) <= 0) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid fs-harmonics value: %s\n", args(1));
        mySession.validity = false;
      }
      else 
        mySession.fsHarmonics = atoi(args(1));
    }
    // Fourier set parameter: add zero frequency or not (y/n)
    else if (args[0] == "fs-zero-freq") {
      if (!mySession.fsFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-zero-freq is only valid for fourier-set option\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-zero-freq takes one and only one argument: <y/n>\n");
        mySession.validity = false;
      }
      else if (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
               args[1] == "y" || args[1] == "Y" || args[1] == "1")
        mySession.fsZeroFreq = true;
      else if (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
               args[1] == "N" || args[1] == "n" || args[1] == "0")
        mySession.fsZeroFreq = false;
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid fs-zero-freq argument: %s\n", args(1));
        mySession.validity = false;
      }
    }
    // Fourier set parameter: alter covariate or not (y/n)
    else if (args[0] == "fs-delta-cov") {
      if (!mySession.fsFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-delta-cov is only valid for fourier-set option\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- fs-delta-cov takes one and only one argument: <y/n>\n");
        mySession.validity = false;
      }
      else if (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
               args[1] == "y" || args[1] == "Y" || args[1] == "1")
        mySession.fsDeltaCov = true;
      else if (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
               args[1] == "N" || args[1] == "n" || args[1] == "0")
        mySession.fsDeltaCov = false;
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid fs-delta-cov argument: %s\n", args(1));
        mySession.validity = false;
      }
    }

    // "matrix-row" is specially designed for adding contrasts
    else if (args[0] == "matrix-row") {
      if (!mySession.contrastFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- matrix-row is only valid for adding contrasts\n");
        mySession.validity = false;
      }
      else if (mySession.condKey.size() && args.size() != mySession.condKey.size() + 1) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- the number of arguments for matrix-row doesn't match the number of condition keys\n");
        mySession.validity = false;
      }
      else if (mySession.condKey.size() && args.size() == mySession.condKey.size() + 1) {	  
        VB_Vector tmpVec(mySession.condKey.size());
        for (size_t i = 1; i < args.size(); i++)
          tmpVec.setElement(i - 1, strtod(args(i)));
        mySession.newContrast.matrix.push_back(tmpVec);
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- matrix-row requires a valid condition function\n");
        mySession.validity = false;
      }
    }
    // "scale" is a keyword specially designed for "add-contrast"
    else if (args[0] == "scale") {
      if (!mySession.contrastFlag && !mySession.diagonalFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scale is valid only in contrast or diagonal section\n");
        mySession.validity = false;
      }
      else if (mySession.contrastFlag && args.size() == 2 
               && (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
                   args[1] == "y" || args[1] == "Y" || args[1] == "1"))
        mySession.newContrast.scaleFlag = true;
      else if (mySession.contrastFlag && args.size() == 2 
               && (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
                   args[1] == "N" || args[1] == "n" || args[1] == "0"))
        mySession.newContrast.scaleFlag = false;
      else if (mySession.diagonalFlag && args.size() == 2 
               && (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
                   args[1] == "y" || args[1] == "Y" || args[1] == "1"))
        mySession.newDS.scaleFlag = true;
      else if (mySession.diagonalFlag && args.size() == 2 
               && (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
                   args[1] == "N" || args[1] == "n" || args[1] == "0"))
        mySession.newDS.scaleFlag = false;
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scale needs one and only one argument: <y/n>\n");
        mySession.validity = false;
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid arguments for scale: %s\n", args(1));
        mySession.validity = false;
      }
    }
    // "center-norm" is another special tag for "add-contrast"
    else if (args[0] == "center-norm") {
      if (!mySession.contrastFlag && !mySession.diagonalFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- center-norm is valid only for contrast or diagonal section\n");
        mySession.validity = false;
      }
      else if (mySession.contrastFlag && args.size() == 2 
               && (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
                   args[1] == "y" || args[1] == "Y" || args[1] == "1"))
        mySession.newContrast.centerFlag = true;
      else if (mySession.contrastFlag && args.size() == 2 && 
               (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
                args[1] == "N" || args[1] == "n" || args[1] == "0"))
        mySession.newContrast.centerFlag = false;
      else if (mySession.diagonalFlag && args.size() == 2 
               && (args[1] == "yes" || args[1] == "Yes" || args[1] == "YES" || 
                   args[1] == "y" || args[1] == "Y" || args[1] == "1"))
        mySession.newDS.centerFlag = true;
      else if (mySession.diagonalFlag && args.size() == 2 
               && (args[1] == "no" || args[1] == "No" || args[1] == "NO" || 
                   args[1] == "N" || args[1] == "n" || args[1] == "0"))
        mySession.newDS.centerFlag = false;
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- center-norm needs one and only one argument: <y/n>\n");
        mySession.validity = false;
      }
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid arguments for center-norm: %s\n", args(1));
        mySession.validity = false;
      }
    }
    // "scan-length" is a keyword for scan-effect when no tes files defined
    else if (args[0] == "scan-length") {
      if (!mySession.scanFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scan-length is valid only in newcov scan-effect section\n");
        mySession.validity = false;
      }
      else if (mySession.teslist.size()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- tes file(s) found, scan-length definition is illegal\n");
        mySession.validity = false;
      }
      else if (args.size() < 3) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- scan-length takes at least two arguments\n");
        mySession.validity = false;
      }
      else if (mySession.scanLen.size()) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- redundant scan-length line\n");
        mySession.validity = false;
      }
      else 
        for (size_t k = 1; k < args.size(); k++)
          mySession.scanLen.Add(args[k]);
    }
    // "absolute" is specially designed to add spike covariates at obsolute position
    else if (args[0] == "absolute") {
      if (!mySession.spikeFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- absolute is valid only in newcov spike section\n");
        mySession.validity = false;
      }
      else if (args.size() == 2)
        mySession.chkSpike(args[1], -1);
      else {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- absolute requiress one and only one argument: spike position\n");
        mySession.validity = false;
      }
    }
    // "relative" defines spike position relative to a real/fake tes file
    else if (args[0] == "relative") {
      if (!mySession.spikeFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- relative is valid only in newcov spike section\n");
        mySession.validity = false;
      }
      else if (args.size() != 3) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- relative requires two and only two arguments: <tes file name/index> and <spike position>\n");
        mySession.validity = false;	
      }
      else {
        int tesIndex = mySession.chkTesStr(args[1]);
        if (tesIndex >= 0)
          mySession.chkSpike(args[2], tesIndex);
        else {
          printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- unknown tes name/index: %s\n", args(1));
          mySession.validity = false;
        }
      }
    }
    // "modcov" modifies a certain covariate 
    else if (args[0] == "modcov") {
      mySession.chkEnd();
      if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- modcov needs one and only one arguments: <covariate name>\n");
        mySession.validity = false;
      }
      else if (!mySession.validateOnly) {
        mySession.singleID = mySession.getCovID(args[1]);
        if (mySession.singleID == -1) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- covariate name not found: %s\n", args(1));
          mySession.validity = false;
        }
        else if (mySession.singleID == -2) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- more than one covariates match the name: %s\n", args(1));
          mySession.validity = false;
        }
        mySession.modCovFlag = true;
      }
      else
        mySession.modCovFlag = true;
    }

    // "modcov+" is designed eigenvector set, FIR and Fourier set options
    else if (args[0] == "modcov+") {
      mySession.chkEnd();
      if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- modcov+ needs one and only one arguments: <covariate name>\n");
        mySession.validity = false;
      }
      else if (!mySession.validateOnly) {
        mySession.singleID = mySession.getCovID(args[1]);
        if (mySession.singleID == -1) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- covariate name not found: %s\n", args(1));
          mySession.validity = false;
        }
        else if (mySession.singleID == -2) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- more than one covariates match the name: %s\n", args(1));
          mySession.validity = false;
        }	
        mySession.modPlusFlag = true;
      }
      else
        mySession.modPlusFlag = true;
    }

    // "modgrp" will modify covariates in a certain group
    //     if (args[0] == "modgrp") {
    //       cout << "debug" << endl;
    //     }
    //     // "subgrp" will define subgroup names in an existing group
    //     if (args[0] == "subgrp") {
    //       cout << "debug" << endl;
    //     }
    
    // Keep adding more syntax line here ...


    // "end" is taken care of by end() function
    else if (args[0] == "end")
      mySession.end();
    // delete a certain covariate
    else if (args[0] == "del-cov") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- del-cov line is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() < 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- del-cov needs at least one arguments: <covariate name>\n");
        mySession.validity = false;
      }
      else if (!mySession.validateOnly) {
        int delIndex;
        for (size_t x = 1; x < args.size(); x++) {
          delIndex = mySession.getCovID(args[x]);
          if (delIndex == -1) {
            printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- covariate name not found: %s\n", args(x));
            mySession.validity = false;
          }
          else if (delIndex == -2) {
            printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
            printf("--- more than one covariates match the name: %s\n", args(x));
            mySession.validity = false;
          }
          else
            mySession.delID.push_back(delIndex);
        }
        if (mySession.validity)
          mySession.delCov();
      }
    }
    // delete all covariates
    else if (args[0] == "del-all-cov") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- del-all-cov line is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() > 1) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- del-all-cov does not need any argument\n");
        mySession.validity = false;
      }
      else if (mySession.covList.size())
        mySession.covList.clear();
    }
    // Check efficiency
    else if (args[0] == "chkeff") {
      if (mySession.newCovFlag || mySession.modCovFlag || mySession.modPlusFlag || mySession.effFlag) {
        mySession.effFlag = true;
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- chkeff line is NOT allowed in the middle of newcov/modcov/modcov+/chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        mySession.effFlag = true;
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- chkeff needs one and only one argument: <covariate name>\n");
        mySession.validity = false;
      }
      else if (!validateOnly) {
        mySession.effFlag = true;
        int selection = mySession.getCovID(args[1]);
        if (selection == -1) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- covariate name not found: %s\n", args(1));
          mySession.validity = false;
        }
        else if (selection == -2) {
          printf("GDS ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
          printf("--- more than one covariates match the name: %s\n", args(1));
          mySession.validity = false;
        }
        else
          mySession.effBaseIndex = selection;
      }
      else
        mySession.effFlag = true;
    }
    // downsample option in efficiency check
    else if (args[0] == "downsample") {
      mySession.dsFlag = true;
      if (!mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- \"downsample\" is allowed ONLY in chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- downsample needs one and only one argument: <downsample option>\n");
        mySession.validity = false;
      }
      else if (!mySession.chkDS(args[1])) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- unknown downsample option: %s\n", args(1));
        printf("    acceptable options are: no, n, before, after\n");
        mySession.validity = false;
      }
      else
        mySession.dsOption = mySession.chkDS(args[1]);
    }
    // filter name for efficiency check
    else if (args[0] == "filter") {
      mySession.filterFlag = true;
      if (!mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- \"filter\" is allowed ONLY in chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- filter needs one and only one argument: <filter filename>\n");
        mySession.validity = false;
      }
      else if (!mySession.chkinfile(args(1))) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- invalid filter file: %s\n", args(1));
        mySession.validity = false;
      }
      else
        mySession.effFilter = args[1];
    }
    // type for efficiency check
    else if (args[0] == "eff-type") {
      mySession.effTypeFlag = true;
      if (!mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- \"eff-type\" is allowed ONLY in chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- eff-type needs one and only one argument: <covariate type>\n");
        mySession.validity = false;
      }
      else if (!mySession.chkEffType(args[1])) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- unknown covariate type for efficiency check: %s\n", args(1));
        printf("    acceptable arguments are: A/I/N/K\n");
        mySession.validity = false;
      }
      // Make sure this type is same as the base covariate's type
      else if (!mySession.validateOnly && 
               !mySession.typeMatch(mySession.chkEffType(args[1]), mySession.effBaseIndex)) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- type does not match the that of eff-base: %s\n", args(1));
        mySession.validity = false;
      }
      else	       
        mySession.effType = mySession.chkEffType(args[1]);
    }
    // cutoff value for efficiency check
    else if (args[0] == "eff-cutoff") {
      mySession.cutoffFlag = true;
      if (!mySession.effFlag) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- \"eff-cutoff\" is allowed ONLY in chkeff section\n");
        mySession.validity = false;
      }
      else if (args.size() != 2) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- eff-cutoff needs one and only one argument: <cutoff value>\n");
        mySession.validity = false;
      }
      else if (atof(args(1)) <= 0) {
        printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
        printf("--- eff-cutoff argument is not a positive value: %s\n", args(1));
        mySession.validity = false;
      }
      else 
        mySession.cutoff = atof(args(1));
    }
    // Any other unknown tags will  give SYNTAX ERROR
    else {
      printf("GDS SYNTAX ERROR in %s [line %d]:\n", filename.c_str(), lineNum);
      printf("--- unknown tag: %s\n", args(0));
      mySession.validity = false;
    }
  }
  // If inLineNum is nonzero, the file is read by an "include" line
  if (inLineNum) {
    infile.close();
    return (validity && mySession.validity);
  }

  // File parsing is done
  mySession.chkEnd();
  // Final check of condition function (in case it has never been used)
  if (mySession.condfxn.length() && !mySession.doneCondition)
    mySession.chkCondition();

  // Make sure the ending session does include covariate(s)
  if (mySession.validity && !validateOnly) {
    if (mySession.covList.size())
      //sessionList.push_back(mySession);
      mySession.writeG();
    else {
      printf("GDS ERROR in %s: %s session doesn't include any covariates\n", 
             filename.c_str(), mySession.dirname.c_str());
      mySession.validity = false;
    }
  }

  validity = (validity && mySession.validity);
  infile.close();

  if (validateOnly) {
    if (validity)
      printf("\nSyntax check status of %s: pass\n", filename.c_str());
    else
      printf("\nSyntax check status of %s: fail\n", filename.c_str());
  }
  
  else if (!validity) {
    printf("\nFinal status of %s: fail\n", filename.c_str());
    printf("G matrix file generation aborted\n");
  }

  return validity;
}

/************************************************************************************
 * gSession class is the center of gds program. It loads all paremeters from a 
 * single gSession to make G matrix files later. 
 ************************************************************************************/
/* gSession constructor */
gSession::gSession()
{
  init();
}

/* Destructor */
gSession::~gSession()
{
  covList.clear();
}

/* Simple initialization */
void gSession::init()
{
  validity = true;
  meanAll = false;
  fakeTes = false;
  doneCommon = doneCondition = false;
  startLine = 0;
  TR = totalReps = 0;
  gsessionFlag = samplingFlag = false;
  tmpResolve = 100;
  inputFilename = "";
  dirname = "";
  condfxn = "";
  condLabFile = "";
  newVar.init();
  newContrast.init();
  newDS.init();
  singleFile = trialFile = txtFile = "";
  ts = 0;

  newCovFlag = modCovFlag = modPlusFlag = false;
  singleFlag = interceptFlag = false;
  diagonalFlag = contrastFlag = false;
  trialFlag = varTrialFlag = false;
  interceptFlag = scanFlag = false;
  gsFlag = mpFlag = spikeFlag = txtFlag = cpFlag = false;

  singleID = -1;
  derivFlag = esFlag = expnFlag = firFlag = fsFlag = false;
  fsPeriod = fsHarmonics = -1;  // Fourier set default: period and number of harmonics
  fsZeroFreq = false;           // Fourier set default: no zero frequency   
  fsDeltaCov = true;            // Fourier set defualt: convert covariates to delta
  orthFlag = orthNameFlag = orthTypeFlag = false;  // orthogonalization default

  resetEff();
}

/* reset() will reset newVar, option and covolvOpt variables,
 * called by end(), chkEnd() */
void gSession::reset()
{
  newVar.init();
  if (newVar.varvec.getLength())
    newVar.varvec.setAll(0);
  if (covName.size())
    covName.clear();
  if (option.size())
    option.clear();
  if (convolOpt.size())
    convolOpt.clear();
  if (singleFile.length())
    singleFile = "";
  if (trialFile.length())
    trialFile = "";
  if (txtFile.length())
    txtFile = "";
  if (scanLen.size())
    scanLen.clear();
  if (spike.size())
    spike.clear();
  if (newList.size())
    newList.clear();
  if (orthID.size())
    orthID.clear();
  if (delID.size())
    delID.clear();

  singleID = -1;
  newCovFlag = modCovFlag = modPlusFlag = false;
  derivFlag = esFlag = expnFlag = firFlag = fsFlag = false;

  fsPeriod = fsHarmonics = -1;  // Fourier set default: period and number of harmonics
  fsZeroFreq = false;           // Fourier set default: no zero frequency   
  fsDeltaCov = true;            // Fourier set defualt: convert covariates to delta
  orthFlag = orthNameFlag = orthTypeFlag = false;  // orthogonalization default
}

/* resetTR() is written for resetting TR. */
void gSession::resetTR()
{
  // reset TR to 0 first
  TR = 0;
  // reset some variables
  doneCommon = doneCondition = false;
  newContrast.init();
  newDS.init();
  singleFile = trialFile = txtFile = "";
  ts = 0;
  // reset flags
  singleFlag = interceptFlag = false;
  diagonalFlag = contrastFlag = false;
  trialFlag = varTrialFlag = false;
  interceptFlag = scanFlag = false;
  gsFlag = mpFlag = spikeFlag = txtFlag = cpFlag = false;
  newCovFlag = modCovFlag = modPlusFlag = false;
  reset();
  // reset tes files
  if (teslist.size())
    teslist.clear();
  if (lenList.size())
    lenList.clear();
  if (tesReal.size())
    tesReal.clear();
  fakeTes = false;
  // reset previous covariates
  if (covList.size())
    covList.clear();
}
  
/* resetSampling() is written for resetting sampling rate */
void gSession::resetSampling()
{
  // reset tmpResolve to 100 first
  tmpResolve = 100;
  // reset some variables
  doneCommon = doneCondition = false;
  newContrast.init();
  newDS.init();
  singleFile = trialFile = txtFile = "";
  ts = 0;
  // reset flags
  singleFlag = interceptFlag = false;
  diagonalFlag = contrastFlag = false;
  trialFlag = varTrialFlag = false;
  interceptFlag = scanFlag = false;
  gsFlag = mpFlag = spikeFlag = txtFlag = cpFlag = false;
  newCovFlag = modCovFlag = modPlusFlag = false;
  reset();
  // reset previous covariates
  if (covList.size())
    covList.clear();
}

/* resetLen() is written for resetting length 
 * Note that this function doesn't clear the gsession name, TR and sampling */
void gSession::resetLen()
{
  // reset totalReps to 0 first
  totalReps = 0;
  // reset some variables
  doneCommon = doneCondition = false;
  newContrast.init();
  newDS.init();
  singleFile = trialFile = txtFile = "";
  ts = 0;
  // reset flags
  singleFlag = interceptFlag = false;
  diagonalFlag = contrastFlag = false;
  trialFlag = varTrialFlag = false;
  interceptFlag = scanFlag = false;
  gsFlag = mpFlag = spikeFlag = txtFlag = cpFlag = false;
  newCovFlag = modCovFlag = modPlusFlag = false;
  reset();
  // reset tes files
  if (teslist.size())
    teslist.clear();
  if (lenList.size())
    lenList.clear();
  if (tesReal.size())
    tesReal.clear();
  fakeTes = false;
  // reset condition function info
  condfxn = "";
  condLabFile = "";
  if (condKey.size())
    condKey.clear();
  if (userCondKey.size())
    userCondKey.clear();
  // reset previous covariates
  if (covList.size())
    covList.clear();
}

/* reset4NewG() resets some parameters before reading a valid G matrix file */
void gSession::reset4openG()
{
  // clear up previous covariates and all parameters
  validity = true;
  doneCommon = doneCondition = false;
  TR = totalReps = 0;
  tmpResolve = 100;
  condfxn = "";
  condLabFile = "";
  newContrast.init();
  newDS.init();
  singleFile = trialFile = txtFile = "";
  ts = 0;

  newCovFlag = modCovFlag = modPlusFlag = false;
  singleFlag = interceptFlag = false;
  diagonalFlag = contrastFlag = false;
  trialFlag = varTrialFlag = false;
  interceptFlag = scanFlag = false;
  gsFlag = mpFlag = spikeFlag = txtFlag = cpFlag = false;

  derivFlag = esFlag = expnFlag = firFlag = fsFlag = false;
  fsPeriod = fsHarmonics = -1;  // Fourier set default: period and number of harmonics
  fsZeroFreq = false;           // Fourier set default: no zero frequency   
  fsDeltaCov = true;            // Fourier set defualt: convert covariates to delta
  orthFlag = orthNameFlag = orthTypeFlag = false;  // orthogonalization default
  reset();

  // reset tes files
  if (teslist.size())
    teslist.clear();
  if (lenList.size())
    lenList.clear();
  if (tesReal.size())
    tesReal.clear();
  fakeTes = false;
  // reset previous covariates
  if (covList.size())
    covList.clear();
}

/* reset chkeff variables */
void gSession::resetEff()
{
  effFlag = effTypeFlag = dsFlag = cutoffFlag = filterFlag = false;
}

/* reInit() clears everything.
 * This function is called when "gsession" tag is met */
void gSession::reInit()
{
  init();

  if (teslist.size())
    teslist.clear();
  if (lenList.size())
    lenList.clear();
  if (tesReal.size())
    tesReal.clear();
  fakeTes = false;
  if (condKey.size())
    condKey.clear();
  if (userCondKey.size())
    userCondKey.clear();
  if (covList.size())
    covList.clear();
}

/* This function checks the validity of gSession's argument */
int gSession::chkSessionName(const char *inputStr)
{
  string foo(inputStr);
  string parent = xdirname(foo);
  // Return 0 if parent dir doesn't exist
  if (!vb_direxists(parent))
    return 0;
  // Return 1 if parent dir is not writeable
  if (access(parent.c_str(),W_OK))
    return 1;

  int len = foo.length();
  string rootStr = xrootname(foo);
  // Return 2 if .G exists and is a directory
  if (foo.substr(len - 2, 2) == ".G" && vb_direxists(foo))
    return 2;
  // Return 3 if .preG exists and is a directory
  else if (foo.substr(len - 2, 2) == ".G" && vb_direxists(rootStr + ".preG"))
    return 3;
  else if (foo.substr(len - 2, 2) == ".G") {
    gDirFlag = false;
    gFilename = xrootname(foo);
  }
  else {
    gDirFlag = true;
    dirname = foo;
  }
  return 4;
}

int
gSession::chkdirname(const char *indirname)
{
  if (!access(indirname,W_OK))
    return 0;
  return 1;
}

/* Check an input file's status.
 * Return 0 if not valid, returns 1 otherwise. */
int gSession::chkinfile(const char *inputname)
{
  if (access(inputname,R_OK))
    return 0;
  return 1;
}

/* Check a variable's type, returns 1 if it's I/N/K, return 0 otherwise. */
char gSession::chkType(string inputType)
{
  if (inputType == "I" || inputType == "i" || inputType == "Interest" || inputType == "interest")
    return 'I';
  if (inputType == "N" || inputType == "n" || inputType == "NoInterest" || 
      inputType == "Nointerest" || inputType == "noInterest" || inputType == "nointerest")
    return 'N';
  if (inputType == "K" || inputType == "k" || inputType == "KeepNoInterest" || inputType == "keepNoInterest" || 
      inputType == "keepNointerest" || inputType == "keepNoInterest" || inputType == "keepnointerest")
    return 'K';
  //   if (inputType == "D" || inputType == "d" || inputType == "Dependent" || inputType == "dependent")
  //     return 'D';
  return 0;
}

/* chkCovName() makes sure the input string (covariate name) is valid; 
 * returns -1 if it has "->" in the name;
 * returns -2 if it is same as one of the previous defined names;
 * returns 0 otherwise */
int gSession::chkCovName(string inputStr)
{
  size_t a = inputStr.find("->");
  if (a < string::npos)
    return -1;

  for (size_t i = 0; i < covName.size(); i++) {
    if (inputStr == covName[i])
      return -2;
  }

  return 0;
}

/* chkUniName() returns true if the new name defined is already available in 
 * previous covList; returns false otherwise */ 
bool gSession::chkUniName(string inputStr)
{
  string newFullName;
  if (newVar.group.length())
    newFullName = newVar.group + "->" + inputStr;
  else
    newFullName = newVar.group + "->" + inputStr;

  if (getCovID(newFullName) == -1)
    return true;
  else
    return false;
}

/* end() interprets "end" line in each section.
 * This function is called when "end" tag is met. */
void gSession::end()
{
  // Make sure "orthog" option is correct
  if (orthFlag && !orthTypeFlag && !orthNameFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid orthog option: no orth-type or orth-name lines found\n");
    validity = false;
  }

  if (singleFlag) {
    chkSingle();
    if (validity && !validateOnly)
      addSingle();
    reset();
    singleFlag = false;
  }
  else if (interceptFlag) {
    if (validity && !validateOnly)
      addIntercept();
    reset();
    interceptFlag = false;
  }
  else if (trialFlag) {
    if (validity && !validateOnly)
      addTrialfx();
    reset();
    trialFlag = false;
  }
  else if (diagonalFlag) {
    if (validity && !validateOnly)
      addDiagonal();
    reset();
    newDS.init();
    diagonalFlag = false;
  }
  else if (contrastFlag) {
    if (!newContrast.matrix.size()) {
      printf("GDS SYNTAX ERROR in %s contrast section [ended at line %d]:\n", 
             inputFilename.c_str(), lineNum);
      printf("--- matrix-row line needed for contrast matrix definition\n");
      validity = false;
    }
    if (validity && !validateOnly)
      addContrast();
    reset();
    newContrast.init();
    contrastFlag = false;
  }
  else if (varTrialFlag) {
    chkTrialFile();
    if (validity && !validateOnly)
      addVarTrialfx();
    reset();
    varTrialFlag = false;
  }
  else if (scanFlag) {
    if (validity && !teslist.size())
      chkScanLen();
    if (validity && !validateOnly)
      addScanfx();
    reset();
    scanFlag = false;
  }
  else if (gsFlag) {
    if (validity)
      chkGS();
    if (validity && !validateOnly)
      addGS();
    reset();
    gsFlag = false;
  }
  else if (mpFlag) {
    if (validity)
      chkMP();
    if (validity && !validateOnly)
      addMP();
    reset();
    mpFlag = false;
  }    
  else if (spikeFlag) {
    if (validity && !validateOnly)
      addSpike();
    reset();
    spikeFlag = false;
  }
  else if (txtFlag) {
    if (validity)
      chkTxt();
    if (validity && !validateOnly)
      addTxt();
    reset();
    txtFlag = false;
  }
  else if (cpFlag) {
    if (validity && !validateOnly)
      cpCov();
    reset();
    cpFlag = false; 
  }
  else if (modCovFlag) {
    if (validity && !validateOnly)
      modCov();
    reset();
  }
  else if (modPlusFlag) {
    chkModPlus();
    if (validity && !validateOnly)
      modPlus();
    reset();
  }
  else if (effFlag) {
    chkEffVar();
    if (validity && !validateOnly)
      exeEff();
    resetEff();
  }

  else {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- redundant end tag\n");
    validity = false;
  }
}

/* chkEnd() checks if each section has an "end" tag to close it.
 * This function is called when:
 * (1) "newcov" tag is met; 
 * (2) "gsession" is met;
 * (3) gds file parsing is done. */
void gSession::chkEnd()
{
  if (singleFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov single (or add-new) section doesn't end\n");
    singleFlag = false;
    validity = false;
    reset();
  }

  if (interceptFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov intercept (or add-intercept) section doesn't end\n");
    interceptFlag = false;
    validity = false;
    reset();
  }

  if (trialFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov trial-effect section doesn't end\n");
    trialFlag = false;
    validity = false;
    reset();
  }  

  if (diagonalFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov diagonal (or add-diagonal) section doesn't end\n");
    diagonalFlag = false;
    validity = false;
    reset();
  }

  if (contrastFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov contrast (or add-contrast) section doesn't end\n");
    contrastFlag = false;
    validity = false;
    reset();
  }

  if (varTrialFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov var-trialfx section doesn't end\n");
    varTrialFlag = false;
    validity = false;
    reset();
  }

  if (scanFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov scan-effect section doesn't end\n");
    scanFlag = false;
    validity = false;
    reset();
  }

  if (gsFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov global-signal section doesn't end\n");
    gsFlag = false;
    validity = false;
    reset();
  }

  if (mpFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov move-params section doesn't end\n");
    mpFlag = false;
    validity = false;
    reset();
  }
  
  if (spikeFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov spike section doesn't end\n");
    spikeFlag = false;
    validity = false;
    reset();
  }

  if (txtFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov txt-file section doesn't end\n");
    txtFlag = false;
    validity = false;
    reset();
  }

  if (cpFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- newcov cp section doesn't end\n");
    cpFlag = false;
    validity = false;
    reset();
  }

  if (modCovFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- modcov section doesn't end\n");
    modCovFlag = false;
    validity = false;
    reset();
  }

  if (modPlusFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- modcov+ section doesn't end\n");
    modPlusFlag = false;
    validity = false;
    reset();
  }

  if (effFlag) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- chkeff section doesn't end\n");
    validity = false;
    resetEff();
  }
}

/* Check common variables: TR, totalReps and G matrix filename.
 * This function is called when "newcov" is met and in chkCondition() */
void gSession::chkCommon()
{
  doneCommon = true;
  // Get TR and number of points information from tes files
  int tesTR = 0, tesReps = 0;
  if (teslist.size()) {
    Tes myTes;
    int tmpTR;
    for (size_t i = 0; i < teslist.size(); i++) {
      if (tesReal[i]) {
        myTes.ReadHeader(teslist[i]);
        tmpTR = static_cast<int> (myTes.voxsize[3]);
        if (!tesTR && tmpTR)
          tesTR = tmpTR;
        else if ( tesTR && tmpTR && tesTR != tmpTR) {
          printf("gds syntax error in %s [gSession starting from line %d]:\n", 
                 inputFilename.c_str(), startLine);
          printf("--- different TR found in TES files\n");
          tesTR = tesReps = 0;
          validity = false;
          return;
        }
        lenList[i] = myTes.dimt;
      }
      tesReps += lenList[i];
    }
  }
  // Compare tesTR with user-defined TR (if available)
  if (!TR && !tesTR) {
    printf("gds syntax error in %s [[gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- TR or scan line(s) required to define TR\n");
    validity = false;
    return;
  }
  if (!TR && tesTR)
    TR = tesTR;
  if (TR && tesTR && TR != tesTR) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- TR in tes file's header (%d) doesn't match the value on line %d (%d)\n", 
           tesTR, lineNum, TR);
    validity = false;
    return;
  }

  if (TR % tmpResolve != 0) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- TR (%d) is NOT multiple of sampling rate (%d)\n", TR, tmpResolve);
    validity = false;
  }
 
  // Compare tesReps with user-defined totalReps (if available)
  if (!totalReps && !tesReps) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- length or scan lines required to define the total number of images\n");
    validity = false;
    return;
  }
  if (!totalReps && tesReps)
    totalReps = tesReps;
  if (totalReps && tesReps && totalReps != tesReps) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- total length in tes files (%d) doesn't match the value defined in length line(%d)\n", 
           tesReps, totalReps);
    validity = false;
    return;
  }
  // Make sure the analysis folder is valid
  if (gDirFlag && !dirname.length()) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", inputFilename.c_str(), startLine);
    printf("--- gsession's name (analysis folder) not defined\n");
    validity = false;
    return;
  }

  if (gDirFlag)
    gFilename=dirname+"/"+xfilename(dirname);
}

/* Add single covariate */
void gSession::addSingle()
{
  VB_Vector foo(singleFile);
  // set up varvec
  int upRatio = (TR / tmpResolve) / (foo.getLength() / totalReps);
  newVar.varvec = VB_Vector(upSampling(&foo, upRatio));
  newCovMod();
  // set up varname
  if (covName.size())
    newVar.varname = covName[0];
  // put new covariate into covList array
  covList.push_back(newVar);
}

/* getFullName() returns the input covariate's full name */
string gSession::getFullName(unsigned covIndex)
{
  if (covList[covIndex].group.length())
    return covList[covIndex].group + "->" + covList[covIndex].varname;
 
  return covList[covIndex].varname;
}

/* Check variables in "Add new covariate", returns 0 if everything is Ok. */
void gSession::chkSingle()
{
  if (!singleFile.length()) {
    printf("GDS SYNTAX ERROR in %s single section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- no valid ref file defined\n");
    validity = false;
    return;
  }
  else if (newVar.type == 'u') {
    printf("GDS SYNTAX ERROR in %s single section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- no valid type defined\n");
    validity = false;
    return;
  }

  VB_Vector foo;
  int newStat = foo.ReadFile(singleFile);
  if (newStat) {
    printf("GDS SYNTAX ERROR in %s single section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid varfile format: %s\n", singleFile.c_str());
    validity = false;
  }
  else if (foo.getLength() % totalReps != 0) {
    printf("GDS SYNTAX ERROR in %s single section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- number of elements (%d) in %s is NOT multiple of total length (%d)\n", 
           (int)foo.getLength(), singleFile.c_str(), totalReps); 
    validity = false;
  }
  else if ((TR / tmpResolve) % (foo.getLength() / totalReps) != 0) {
    printf("GDS SYNTAX ERROR in %s single section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- reference function (%s) with a length of %d can NOT be upsampled at the \
current sampling rate (%d)\n", singleFile.c_str(), (int)foo.getLength(), tmpResolve); 
    validity = false;
  }
}

/* Modify newVar by options;
 * read the options one by one, option order does matter! 
 * Note that the 5 advanced options are not permitted in newcov session.
 * They are: derivative, eigenvector set, exponential, FIR and Fourier set. */
void gSession::newCovMod()
{
  for (size_t i = 0; i < option.size(); i++)
    singleOpt(newVar, option[i]);
}
   
/* modCov() is written for "modcov" tag */
void gSession::modCov()
{
  for (size_t i = 0; i < option.size(); i++)
    singleOpt(covList[singleID], option[i]);

  if (covName.size())
    covList[singleID].varname = covName[0];
  if (newVar.group.length())
    covList[singleID].group = newVar.group;
  if (newVar.type != 'u')
    covList[singleID].type = newVar.type;
}

/* chkModPlus() makes sure eigen vector, fir or Fourier set options are all valid */
void gSession::chkModPlus()
{
  double magFactor = 2000.0 / (double) tmpResolve;
  if (esFlag && magFactor != floor(magFactor)) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- eigen-vector option requires 2000 to be multiple of sampling rate\n");
    validity = false;
  }
  
  if (fsFlag && fsPeriod == -1) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("---  fs-period line not found in fourier-set option\n");
    validity = false;
  }


  if (fsFlag && fsHarmonics == -1) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("---  fs-harmonics line not found in fourier-set option\n");
    validity = false;
  }

}

/* modPlus() is written to deal the options that will not only 
 * modify a certain covariate, but also generates new covariates.
 * These options are: eigen-vector, fir, fourier-set            */
void gSession::modPlus()
{
  if (esFlag)
    eigenSet();
  else if (firFlag)
    firSet();
  else if (fsFlag)
    fourierSet();
  else 
    modCov();
}

/* eigenSet() removes selected covariate and adds three new eigenvector ones. */
void gSession::eigenSet()
{
  // Copy original covariate to newVar, then delete the original
  prepNew();
  covList.erase(covList.begin() + singleID);

  for (int i = 0; i < esIndex; i++)
    singleOpt(newVar, option[i]);
  insertES();
}

/* prepNew() is written specially for modcov+ tag. It copies selected covariate to 
 * newVar. If no type/group defined, the original ones will be copied */
void gSession::prepNew()
{
  if (!newVar.group.length())
    newVar.group = covList[singleID].group;
    
  if (newVar.type == 'u')
    newVar.type = covList[singleID].type;

  newVar.varname = covList[singleID].varname;
  newVar.varvec = covList[singleID].varvec;
}

/* insertES() inserts three new eigen vector set covariates, then execute the rest of
 * options after eigen-vector, rename covariates */
void gSession::insertES()
{
  int orderG = totalReps * TR / tmpResolve;
  double magFactor = 2000.0 / (double) tmpResolve;
  char foo[BUFLEN];
  VB_Vector *myVec = new VB_Vector(newVar.varvec);
  calcDelta(myVec);
  string orgName = newVar.varname;
  char bar[BUFLEN];
  // Use vbp to get voxbo installation directory's absolute path
  string vbStr = vbp.rootdir;
  int strLen = vbStr.length();
  if (vbStr[strLen - 1] != '/')
    vbStr += "/";

  for (size_t i = 0; i < 3; i++) {
    VB_Vector *eigenVec = new VB_Vector(orderG);
    eigenVec->setAll(0);
    sprintf(foo, "%selements/filters/Eigen%d.ref", vbStr.c_str(), (int)i + 1); 
    VB_Vector *tmpVec = new VB_Vector(foo);
    tmpVec->sincInterpolation((unsigned)magFactor);

    int vecLength = tmpVec->getLength();
    for (int j = 0; j < vecLength; j++)
      (*eigenVec)[j] = (*tmpVec)[j];
  
    newVar.varvec = fftConv(myVec, eigenVec, false);
    delete tmpVec;
    delete eigenVec;

    sprintf(bar, " [Eigen%d]",(int)i + 1);
    newVar.varname = orgName + string(bar);

    for (size_t j = esIndex + 1; j < option.size(); j++)
      singleOpt(newVar, option[j]);

    if (i < covName.size())
      newVar.varname = covName[i];

    covList.insert(covList.begin() + singleID + i, newVar);
  }
  delete myVec;
}
 
/* firSet() modifies original covariate and adds a few new ones based on number of fir */
void gSession::firSet()
{
  // Copy original covariate to newVar, then delete the original
  prepNew();
  covList.erase(covList.begin() + singleID);

  for (int i = 0; i < esIndex; i++)
    singleOpt(newVar, option[i]);
  insertFIR();
}

/* insertFIR() create FIR set covariates, execute the options after fir, insert new 
 * covariates right after the original covariate */
void gSession::insertFIR()
{
  string orgName = newVar.varname;
  // First convert the selected covariate to delta  
  calcDelta(&(newVar.varvec));
  VB_Vector orgVec = newVar.varvec;
  // Generate equal number of time-shifted covariates based on the selected one
  double timeShift;
  char bar[BUFLEN];
  for (size_t j = 0; j <= (size_t)firNum; j++) {
    timeShift = (double) j * TR / tmpResolve; 
    if (j > 0)
      orgVec.phaseShift(timeShift, newVar.varvec);
    // default FIR name
    sprintf(bar, " [FIR%d]", (int)j);
    newVar.varname = orgName + string(bar);
    // Take care of options after FIR
    for (size_t k = firIndex + 1; k < option.size(); k++)
      singleOpt(newVar, option[k]);
    // User-defined covariate name
    if (j < covName.size())
      newVar.varname = covName[j];
    covList.insert(covList.begin() + singleID + j, newVar);
  } 
}

/* fourierSet() removes original covariate and adds a few new 
 * covariates based on Fourier set parameters defined. */
void gSession::fourierSet()
{
  // Copy original covariate to newVar, then delete the original
  prepNew();
  covList.erase(covList.begin() + singleID);

  for (int i = 0; i < fsIndex; i++)
    singleOpt(newVar, option[i]);
  insertFS();
}

/* insertFS() inserts Fourier set covariates */
void gSession::insertFS()
{
  string orgName = newVar.varname;
  VB_Vector *orgVec = new VB_Vector(newVar.varvec);
  int windowWidth = fsPeriod * 1000 / tmpResolve;
  int orderG = totalReps * TR / tmpResolve;
  double pix2 = 2.0 * PI;
  // newIndex tracks the position where the next new covariate should be inserted
  int newIndex = singleID;  
  int nameIndex = 0;
  if (fsZeroFreq) {
    insertCovDC(windowWidth);
    // Increase newIndex because DC component is an extra covariate
    newIndex++;
    nameIndex = 1;
  }

  int m = 0;  // m is harmonics index
  char foo[BUFLEN];
  VB_Vector *fourierVec = new VB_Vector(orderG);
  for (size_t i = 1; i <= 2 * (uint32)fsHarmonics; i++) {
    fourierVec->setAll(0);
    if (i % 2) {
      m = (i + 1) / 2;
      for (int k = 0; k < windowWidth; k++) 
        (*fourierVec)[k] = sin(pix2 * k / windowWidth * m);
      sprintf(foo, " [sin #%d]", m); 
    }
    else {
      m = i / 2;
      for (int k = 0; k < windowWidth; k++) 
        (*fourierVec)[k] = cos(pix2 * k / windowWidth * m);
      sprintf(foo, " [cos #%d]", m); 
    }
    newVar.varvec = *fs_getFFT(orgVec, fourierVec);
    newVar.varname = orgName + foo;
    for (size_t k = fsIndex + 1; k < option.size(); k++)
      singleOpt(newVar, option[k]);
    if (i - 1 + nameIndex < covName.size()) 
      newVar.varname = covName[i - 1 + nameIndex];
    covList.insert(covList.begin() + newIndex, newVar);
    newIndex++;
  }
  delete orgVec;
  delete fourierVec;
}

/* insertCovDC() inserts the DC covariate in covList array */
void gSession::insertCovDC(int windowWidth)
{
  int orderG = totalReps * TR / tmpResolve;
  VB_Vector *fourierVec = new VB_Vector(orderG);
  fourierVec->setAll(0);
  for (int j = 0; j < windowWidth; j++)
    fourierVec->setElement(j, 1.0);

  VB_Vector *currentVec = new VB_Vector(newVar.varvec);
  newVar.varvec = *fs_getFFT(currentVec, fourierVec);
  delete fourierVec;
  delete currentVec;
  newVar.varname = newVar.varname + " [DC]";

  for (size_t k = fsIndex + 1; k < option.size(); k++)
    singleOpt(newVar, option[k]);
  if (covName.size())
    newVar.varname = covName[0];
  covList.insert(covList.begin() + singleID, newVar);
}

/* fs_getFFT() returns the new sin/cos vb_vector */
VB_Vector * gSession::fs_getFFT(VB_Vector *currentVec, VB_Vector *fourierVec)
{
  if (fsDeltaCov)
    calcDelta(currentVec);
  VB_Vector *newVec = new VB_Vector(fftConv(currentVec, fourierVec, false));
  newVec->meanCenter();
  newVec->unitVariance();
  
  return newVec;
}

/* commonOpt() modifies an input covariate based on the input option 
 * Options allowed are: mean-center, mean-center-non-zero, unit-variance, unit-excursion, 
 * convert-delta, convolve, ts (time shift), multi (multiply), orth (orthogonalize) */
void gSession::singleOpt(Covar &inputVar, string inputOpt)
{
  if (inputOpt == "mean-center")
    inputVar.varvec.meanCenter();
  else if (inputOpt == "mean-center-non-zero")
    mcNonZero(inputVar.varvec);
  else if (inputOpt == "unit-variance")
    inputVar.varvec.unitVariance();
  else if (inputOpt == "unit-excursion")
    unitExcursion(inputVar.varvec);
  else if (inputOpt == "convert-delta")
    calcDelta(&(inputVar.varvec));
  else if (inputOpt == "convolve") {
    modConvolve(inputVar.varvec);
    inputVar.varname = inputVar.varname + "(" + convolOpt[2] + ")";
  }
  else if (inputOpt == "ts") {
    double imageShift = (double) ts / (double) tmpResolve;
    inputVar.varvec.phaseShift(imageShift);
    char foo[BUFLEN];
    sprintf(foo, " [shift %d]", ts); 
    inputVar.varname = inputVar.varname + string(foo);	
  }
  else if (inputOpt == "expn") {
    for (unsigned j = 0; j < inputVar.varvec.getLength(); j++)
      inputVar.varvec[j] = pow(inputVar.varvec[j], expnNum);
    expnNum = 0;
    char foo[BUFLEN];
    sprintf(foo, " [^%.1f]", expnNum); 
    inputVar.varname = inputVar.varname + string(foo);
  }
  else if (inputOpt == "multi")
    inputVar.varvec.elementByElementMult(covList[multiplyID].varvec);
  else if (inputOpt == "orth")
    orthogonalize(inputVar.varvec);

}

/* Do "mean center non-zero" to input vb_vector */
void gSession::mcNonZero(VB_Vector &inputVec)
{
  int numNonZero = countNonZero(&inputVec);
  double nonZeroMean = inputVec.getVectorSum() / (double) numNonZero;
  int length = inputVec.getLength();
  double element;
  for (int i = 0; i < length; i++) {
    element = inputVec.getElement(i);
    if (element)
      inputVec.setElement(i, element - nonZeroMean);
  }
}

/* Do "unit excursion" to input vb_vector */
void gSession::unitExcursion(VB_Vector &inputVec)
{
  double vecMin = inputVec.getMinElement();
  inputVec+=(0 - vecMin);
  double vecMax = inputVec.getMaxElement();
  inputVec.scaleInPlace(1.0 / vecMax);
}

/* Do "convolve" to input vb_vector */
void gSession::modConvolve(VB_Vector &inputVec)
{
  VB_Vector *currentVec = new VB_Vector(inputVec);
  VB_Vector *convolVec = new VB_Vector(convolOpt(0));
  int kernTR = atoi(convolOpt(1));
  inputVec = getConv(currentVec, convolVec, kernTR, tmpResolve);
  delete currentVec;
  delete convolVec;
}

/* chkOrthType() is similar to chkType(), but the input type could be "ALL/All/all/A/a" */
char gSession::chkOrthType(string inputType)
{
  if (inputType == "ALL" || inputType == "All" || inputType == "all" || inputType == "A" || inputType == "a")
    return 'A';

  if (inputType == "I" || inputType == "i" || inputType == "Interest" || inputType == "interest")
    return 'I';

  if (inputType == "N" || inputType == "n" || inputType == "NoInterest" || 
      inputType == "Nointerest" || inputType == "noInterest" || inputType == "nointerest")
    return 'N';

  if (inputType == "K" || inputType == "k" || inputType == "KeepNoInterest" || inputType == "keepNoInterest" || 
      inputType == "keepNointerest" || inputType == "keepNoInterest" || inputType == "keepnointerest")
    return 'K';

  return 0;
}

/* getOrhtID() collects orthogonalization covariate index and put in orthID array */
void gSession::getOrthID(char inputType)
{
  if (orthID.size())
    orthID.clear();

  // "A" means orthID includes all covariates except singleID
  if (inputType == 'A') {
    for (int i = 0; i < (int) covList.size(); i++) {
      if (i != singleID) 
        orthID.push_back(i);
    }
    return;
  }

  for (int i = 0; i < (int) covList.size(); i++) {
    if (i != singleID && covList[i].type == inputType)
      orthID.push_back(i);
  }
}

/* Orthogonalize input vb_vector */
void gSession::orthogonalize(VB_Vector & inputVec)
{
  if (!orthID.size())
    return;

  // Build subG that consists of covariates defined by orthID
  int rowNum = totalReps * TR / tmpResolve;
  int colNum = orthID.size();
  VBMatrix subG(rowNum, colNum);
  // DYK subG.MakeInCore();
  for (int i = 0; i < colNum; i++)
    subG.SetColumn(i, covList[orthID[i]].varvec);

  // Modify selected covariate by subtracting fitted values
  VB_Vector orthVec=calcfits(subG,inputVec);
  if (orthVec.size()<1) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid orthogonalization: covariates are linearly dependent!\n");
    validity = false;
    return;
  }
  inputVec-=orthVec;
}

/* delCov() deletes covariate whose index is in delID */
void gSession::delCov()
{
  if (!delID.size())
    return;

  // Sort the numbers in delID first. It makes removal easier
  sort(delID.begin(), delID.end());
  for (int i = 0; i < (int) delID.size(); i++)
    covList.erase(covList.begin() + delID[i] - i);
  delID.clear();
}

/* Check options, returns 1 for valid option, 0 otherwise */
int gSession::chkOption(const char* input)
{
  if (!strcmp(input, "mean-center"))
    return 1;
  if (!strcmp(input, "mean-center-non-zero"))
    return 1;
  if (!strcmp(input, "unit-variance"))
    return 1;
  if (!strcmp(input, "unit-excursion"))
    return 1;
  if (!strcmp(input, "convert-delta"))
    return 1;

  return 0;
}

/* Check variables in convolution option */
int gSession::chkConvol(tokenlist convOpt)
{
  if (convOpt.size() < 2 || convOpt.size() > 3) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- convolve option needs two or three arguments in this format:\n");
    printf("option convolve <kernel filename> <convolution TR> <optional tag>\n");
    return 1;
  }

  if (!chkinfile(convOpt(0))) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid convolution file: %s\n", convOpt(0));
    return 2;
  }
    
  VB_Vector testVec;
  if (testVec.ReadFile(convOpt[0])) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid ref file format: %s\n", convOpt(0));
    return 2;
  }
    
  if (atoi(convOpt(1)) <= 0) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- invalid convolution TR: %s\n", convOpt(1));
    return 3;
  } 
 
  int sampling = atoi(convOpt(1));
  int vecLen = testVec.getLength();
  if (sampling * vecLen / tmpResolve > totalReps * TR / tmpResolve) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- filter has longer time than covariate\n");
    return 4;
  }

  return 0;
}

/* Add intercept covariate */
void gSession::addIntercept()
{
  // set up varvec
  int vecLength = totalReps * TR / tmpResolve;
  newVar.varvec = VB_Vector(vecLength);
  newVar.varvec.setAll(1.0);
  newCovMod();
  // set up varname
  if (covName.size())
    newVar.varname = covName[0];
  // put in covList array
  covList.push_back(newVar);
}

/* Add trial effects, based on code in gdw.cpp */
void gSession::addTrialfx()
{
  int numTrials = TR * totalReps / (trialLen * 1000) - 1;
  int unitLength = trialLen * 1000 / tmpResolve;
  for (size_t i = 0; i < (uint32)numTrials; i++) {
    // setup varvec
    newVar.varvec.resize(TR * totalReps / tmpResolve);
    newVar.varvec.setAll(0);
    for (size_t j = i * unitLength; j < (i + 1) * unitLength; j++)
      newVar.varvec.setElement(j, 1.0);
    newVar.varvec.meanCenter();
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "trialfx-%d", ((int)i + 1));
      newVar.varname = foo;
    }
    // put in covList array
    covList.push_back(newVar);
  }
}

/* Add diagonal set */
void gSession::addDiagonal()
{
  int upRatio = (TR / tmpResolve) / (condVec.getLength() / totalReps);
  VB_Vector *condVec_up = upSampling(&condVec, upRatio);

  int upLength = condVec_up->getLength();
  int rowNum = condKey.size() - 1;
  double diagMatrix[rowNum + 1];

  for (size_t i = 0; i < (uint32)rowNum; i++) {
    for (size_t j = 0; j < (uint32)rowNum + 1; j++) {
      if (j - i == 1)
        diagMatrix[j] = 1.0;
      else
        diagMatrix[j] = 0;	
    }
    // set up varvec
    newVar.varvec.resize(upLength);
    for (unsigned j = 0; j < condVec_up->getLength(); j++) {
      int n = (int) condVec_up->getElement(j);
      newVar.varvec.setElement(j, diagMatrix[n]);
      // scale or not?
      if (newDS.scaleFlag)
        newVar.varvec[j] /= (double) countNum(condVec_up, n);
    }
    // center-norm or not?
    if (newDS.centerFlag)
      newVar.varvec.meanCenter();
    // modification options
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else
      newVar.varname = condKey[i+1];
    // put in covList array
    covList.push_back(newVar);
  }

  delete condVec_up;
}

/* Make sure condition function is good */
void gSession::chkCondition()
{
  // Make sure TR, totalReps and tmpResolve are all set
  if (!doneCommon)
    chkCommon();

  doneCondition = true;
  // Make sure condition function exists
  if (!condfxn.length()) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- not defined\n");
    validity = false;
    return;
  }

  condKey.clear();
  int condStat = getCondVec(condfxn.c_str(), condKey, &condVec);
  if (condStat == -1) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- file not readable: %s\n", condfxn.c_str());
    validity = false;
    return;
  }
  if (condStat == -2) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- %s has different number of condition keys in header and content\n", condfxn.c_str());
    validity = false;
    return;
  }
  if (condStat == 1) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- %s has different condition keys in header and content\n", condfxn.c_str());
    validity = false;
    return;
  }

  int condLength = condVec.getLength();
  if (condLength % totalReps != 0) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- number of elements (%d) in %s is NOT multiple of the number of time points defined (%d)\n", 
           condLength, condfxn.c_str(), totalReps);
    validity = false;
    return;
  }
  // Also make sure condition function can be correctly upsampled
  if ( (TR / tmpResolve) % (condLength / totalReps) != 0 ) {
    printf("Condition function error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- %s can NOT be upsampled with the current sampling rate (%dms)\n",
           condfxn.c_str(), tmpResolve);
    validity = false;
    return;
  }

  /* Compare condition keys in condition function file with user-defined ones in 
   * condition-labe;-file or condition-label-name lines */
  chkCondKey();
}

/* chkCondKey() Compare condition keys in condition function file with user-defined ones in 
 * condition-label-file or condition-label-name lines */
void gSession::chkCondKey()
{
  if (!condLabFile.length() && !userCondKey.size())
    return;

  if (condLabFile.length() && userCondKey.size()) {
    printf("gds syntax error in %s [gSession starting from line %d]:\n", 
           inputFilename.c_str(), startLine);
    printf("--- condition-label-file and condition-label-name lines are exclusive\n");
    validity = false;
    return;
  }

  if (condLabFile.length()) {
    int readFlag = getCondLabel(userCondKey, condLabFile.c_str());
    if (readFlag == -1) { // Quit if the file chosen isn't readable
      printf("gds syntax error in %s [gSession starting from line %d]:\n", inputFilename.c_str(), startLine);
      printf("--- condition-label-file not readable\n");
      validity = false;
      return;
    }
  }
  
  size_t numKey = condKey.size();
  if (numKey != userCondKey.size()) {
    numKey = userCondKey.size();  
    printf("gds error in %s [gSession starting from line %d]:\n", inputFilename.c_str(), startLine);
    printf("--- The number of user-defined condition keys is not equal to that defined in condition function file\n");
    validity = false;
    return;
  }

  condKey.clear();
  for (size_t i = 0; i < numKey; i++)
    condKey.Add(userCondKey[i]);

}

/* chkKeyName() compares the input string with any of condition key names. 
 * If the input is same as one of the keys, return the key's index; otherwise returns -1 */
int gSession::chkKeyName(string inputStr)
{
  for (size_t i = 0; i < condKey.size(); i++) {
    if (inputStr == condKey[i])
      return i;
  }
  return -1;
}

/* Add contrast based on contrast matrix */
void gSession::addContrast()
{
  int upRatio = (TR / tmpResolve) / (condVec.getLength() / totalReps);
  VB_Vector *condVec_up = upSampling(&condVec, upRatio);

  size_t upLength = condVec_up->getLength();
  size_t lineNum = newContrast.matrix.size();
  double matRow[condKey.size()];
  char tmpStr2[BUFLEN];
  string tmpStr;
  for (size_t i = 0; i < lineNum; i++) {
    // set up matRow[] and tmpStr, which may be used for varname
    tmpStr = "contrast/";
    for (size_t j = 0; j < condKey.size(); j++) {
      matRow[j] = (newContrast.matrix[i]).getElement(j);
      if (!matRow[j])
        continue;
      sprintf(tmpStr2, "%.2f", matRow[j]);
      tmpStr += (string)tmpStr2 + "*" + condKey[j];
      if (j < condKey.size() - 1)
        tmpStr.append("+");
    }

    // set up varvec
    newVar.varvec.resize(upLength);
    for (size_t j = 0; j < upLength; j++) {
      int n = (int) condVec_up->getElement(j);
      newVar.varvec.setElement(j, matRow[n]);      
      if (newContrast.scaleFlag)
        newVar.varvec[j] /= (double) countNum(condVec_up, n);
    }
    if (newContrast.centerFlag)
      newVar.varvec.meanCenter();
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      // Remove '+' at the end
      if (tmpStr[tmpStr.length() - 1] == '+') 
        tmpStr.erase(tmpStr.length() - 1);
      newVar.varname = tmpStr;
    }
    // put in covList array
    covList.push_back(newVar);
  }
  delete condVec_up;
}

/* Add var length trial effect */
void gSession::addVarTrialfx()
{
  VB_Vector trialVec(trialFile);
  int trialOffset = 0;
  for (size_t i = 0; i < trialVec.getLength() - 1; i++) {
    // set up varvec
    newVar.varvec.resize(TR * totalReps / tmpResolve);
    newVar.varvec.setAll(0);
    int nonZeroUnit = (int) trialVec.getElement(i) * 1000 / tmpResolve;
    for (int j = trialOffset; j < trialOffset + nonZeroUnit; j++) {
      newVar.varvec.setElement(j, 1.0);
    }
    newVar.varvec.meanCenter();
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "var-trialfx-%d", ((int)i + 1));
      newVar.varname = foo;
    }
    // put in covList
    covList.push_back(newVar);
    // Increment trialOffset
    trialOffset += nonZeroUnit;
  }
}

/* chkTrialFile() will make sure the sum of elements in 
 * trial file is equal to actual length (in second) */
void gSession::chkTrialFile()
{
  VB_Vector trialVec(trialFile);
  int trialSum = (int) trialVec.getVectorSum();
  if (trialSum != totalReps * TR / 1000) {
    printf("GDS SYNTAX ERROR in %s trial-effect section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- sum of elements (%d) in %s doesn't match the actual length (%d)\n", 
           trialSum, trialFile.c_str(), totalReps * TR / 1000);
    validity = false;
  }
}

/* Add scan effect */
void gSession::addScanfx()
{
  vector <int> scanLenVec;
  // if tes files are defined, use their length to build scanLenVec 
  if (!scanLen.size()) {
    for (size_t i = 0; i < teslist.size() - 1; i++)
      scanLenVec.push_back(lenList[i]);
  }
  //  if no tes files defined, use scan-length arguments to build scanLenVec
  else {
    for (size_t i = 0; i < scanLen.size() - 1; i++)
      scanLenVec.push_back(atoi(scanLen(i)));
  }

  int offset = 0, end = 0;
  newVar.varvec.resize(totalReps * TR / tmpResolve);
  for (size_t i = 0; i < scanLenVec.size(); i++) {
    // set up varvec
    newVar.varvec.setAll(0);
    end = offset + scanLenVec[i] * TR / tmpResolve;
    for (int j = 0; j < totalReps * TR / tmpResolve; j++) {
      if (j >= offset && j < end)
        newVar.varvec[j] = 1.0;
    }
    newVar.varvec.meanCenter();
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "scanfx-%d", ((int)i + 1));
      newVar.varname = foo;
    }
    // Put in covList array
    covList.push_back(newVar);
    // Increment offset
    offset += scanLenVec[i] * TR / tmpResolve;
  }
}

/* chkScanLen() will make sure the sum of scan-length arguments is equal to total repetition */
void gSession::chkScanLen()
{
  if (!scanLen.size()) {
    printf("GDS SYNTAX ERROR in %s scan-effect section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- no scan-length line found to define scan-effect length\n");
    validity = false;
    return;
  }
 
  int totalLen = 0;
  for (size_t i = 0; i < scanLen.size(); i++) {
    if (atoi(scanLen(i)) <= 0) {
      printf("GDS SYNTAX ERROR in %s scan-effect section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);     
      printf("--- invalid scan length: %s\n", scanLen(i));
      validity = false;
      return;
    } 
    totalLen += atoi(scanLen(i));
  }
  if (totalLen != totalReps) {
    printf("GDS SYNTAX ERROR in %s scan-effect section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- sum of scan-length arguments (%d) not equal to total number of time points (%d)\n", 
           totalLen, totalReps);
    validity = false;
  }
}
 
/* Add global signal */
void gSession::addGS()
{
  string tesStr, gsFile;
  int start = 0;

  for (size_t i = 0; i < teslist.size(); i++) {
    tesStr = teslist[i];
    gsFile = tesStr.substr(0, tesStr.length() - 4);
    gsFile.append("_GS.ref");

    VB_Vector newVec(totalReps);
    newVec.setAll(0);

    VB_Vector gsVec(gsFile);
    int gsLength = gsVec.getLength();
    gsVec.meanCenter();
    // set up varvec
    for (int j = start; j <  gsLength + start; j++)
      newVec[j] = gsVec[j - start];
    newVar.varvec = VB_Vector(upSampling(&newVec, TR /tmpResolve));
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "global-signal-%d", ((int)i + 1));
      newVar.varname = foo;
    }
    // put in covList
    covList.push_back(newVar);
    // Increment start
    start += gsLength;
  }
}

/* chkGS() checks the availability of global signal file(s) based on tes filename(s) */
void gSession::chkGS() 
{
  if (!teslist.size()) {
    validity = false;
    return;
  }

  string foo, gsFile;
  VB_Vector gsVec;
  for (size_t i = 0; i < teslist.size(); i++) {
    foo = teslist[i];
    gsFile = foo.substr(0, foo.length() - 4);
    gsFile.append("_GS.ref");
    if (!chkinfile(gsFile.c_str())) {
      printf("GDS SYNTAX ERROR in %s global-signal section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- global signal file not found: %s\n", gsFile.c_str()); 
      validity = false;
    }
    else if (gsVec.ReadFile(gsFile)) {
      printf("GDS SYNTAX ERROR in %s global-signal section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- global signal file has invalid ref format: %s\n", gsFile.c_str()); 
      validity = false;
    }
  }
}

/* Add movement parameters */
void gSession::addMP()
{
  string tesStr, mpFile;
  int start = 0;

  for (size_t i = 0; i < teslist.size(); i++) {
    tesStr = teslist[i];
    mpFile = tesStr.substr(0, tesStr.length() - 4);
    mpFile.append("_MoveParams.ref");

    VB_Vector mpVec(mpFile);
    int unitLength = mpVec.getLength() / 7;
    VB_Vector moveInfo(unitLength);

    VB_Vector newVec(totalReps);
    newVec.setAll(0);
    for (size_t j = 0; j < 6; j++) {
      // Read 1/7 of the total movement parameter vector into moveinfo and mean center it
      for (int k = 0; k < unitLength; k++)
        moveInfo[k] = mpVec[k * 7 + j];
      moveInfo.meanCenter();
      for (int k = 0; k < unitLength; k++)
        newVec[k + start] = moveInfo[k]; 
      // set up varvec
      newVar.varvec = VB_Vector(upSampling(&newVec, TR / tmpResolve));
      newCovMod();
      // set up varname
      if (i* 6 + j < covName.size())
        newVar.varname = covName[i * 6 + j];
      else {
        char foo[BUFLEN];
        switch(j) {
        case 0:
          sprintf(foo, "move-params-%d-X", ((int)i + 1));
          break;
        case 1:
          sprintf(foo, "move-params-%d-Y", ((int)i + 1));
          break;
        case 2:
          sprintf(foo, "move-params-%d-Z", ((int)i + 1));
          break;
        case 3:
          sprintf(foo, "move-params-%d-pitch", ((int)i + 1));
          break;
        case 4:
          sprintf(foo, "move-params-%d-roll", ((int)i + 1));
          break;
        case 5:
          sprintf(foo, "move-params-%d-yaw", ((int)i + 1));
          break;
        }
        newVar.varname = foo;
      }
      // put in covList array
      covList.push_back(newVar);
    }
    start += unitLength;
  }
}

/* chkMP() will check availability of movement parameter file(s) based on tes filename(s) */
void gSession::chkMP()
{
  if (!teslist.size()) {
    validity = false;
    return;
  }

  string foo, mpFile;
  VB_Vector mpVec;
  for (size_t i = 0; i < teslist.size(); i++) {
    foo = teslist[i];
    mpFile = foo.substr(0, foo.length() - 4);
    mpFile.append("_MoveParams.ref");
    if (!chkinfile(mpFile.c_str())) {
      printf("GDS SYNTAX ERROR in %s move-params section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- movement parameter file not found: %s\n", mpFile.c_str()); 
      validity = false;
    }
    else if (mpVec.ReadFile(mpFile)) {
      printf("GDS SYNTAX ERROR in %s move-params section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- movement parameter file has invalid ref format: %s\n", mpFile.c_str()); 
      validity = false;
    }
    else if (mpVec.getLength() % 7 != 0) {
      printf("GDS SYNTAX ERROR in %s move-params section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- number of elements (%d) in movement parameter file %s is NOT multiple of 7\n", 
             (int)mpVec.getLength(), mpFile.c_str()); 
      validity = false;
    }    
  }
}

/* Add spike covariate(s) */
void gSession::addSpike()
{
  int spikeIndex = 0;
  VB_Vector newVec(totalReps);
  for (size_t i = 0; i < spike.size(); i++) {
    // set up varvec
    newVec.setAll(0);
    spikeIndex = spike[i];
    newVec[spikeIndex] = 1.0;
    newVec.meanCenter();  // mean center by default
    newVar.varvec = VB_Vector(upSampling(&newVec, TR / tmpResolve));
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "spike-%d", spikeIndex);
      newVar.varname = foo;
    }
    // put in covList array
    covList.push_back(newVar);
  }
}

/* chkSpike() makes sure the spike values are valid */
void gSession::chkSpike(string spikeStr, int tesIndex)
{
  vector <int> localSpike = numberlist(spikeStr);
  if (!localSpike.size()) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- invalid spike definition: %s\n", spikeStr.c_str());
    validity = false;
    return;
  }

  int localLen;
  if (tesIndex == -1)
    localLen = totalReps;
  else
    localLen = lenList[tesIndex];
  
  for (int i = 0; i < (int) localSpike.size(); i++) {
    if (localSpike[i] < 0 || localSpike[i] >= localLen) {
      printf("GDS SYNTAX ERROR in %s move-params section [ended at line %d]:\n",
             inputFilename.c_str(), lineNum);
      printf("--- spike index out of range: %d\n", localSpike[i]);
      validity = false;
      spike.clear();
      return;
    }
  }

  // startPos in defined to convert relative spike position to absulute position
  int startPos = 0;
  for (int i = 0; i < tesIndex; i++)
    startPos += lenList[i];
  for (int i = 0; i < (int) localSpike.size(); i++)
    spike.push_back(localSpike[i] + startPos);
}

/* chkTesStr() checks the validity of tes name string in "relative" line */
int gSession::chkTesStr(string tesStr)
{
  // If the input string is equal to one of the elements in teslist array, return its index
  for (int i = 0; i < (int) teslist.size(); i++) {
    if (tesStr == teslist[i])
      return i;
  }
  // "0" is special, because when atoi() also returns 0 when the 
  // string can't be converted successfully to an integer
  if (tesStr == "0")
    return 0;
  // Returns the integer converted from the string as the index
  int tesIndex = atoi(tesStr.c_str());
  if (tesIndex)
    return tesIndex;
  // Returns -1 if it's NOT converted successfully
  return -1;
}
 
/* chkTxt() makes sure the txt file is good */
void gSession::chkTxt()
{
  const char *txtChar = txtFile.c_str();
  int txtColNum = getTxtColNum(txtChar);
  int txtRowNum = getTxtRowNum(txtChar);

  if (txtColNum < 0) {
    printf("GDS ERROR in %s txt-file section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- invalid txt file: %s\n", txtChar);
    validity = false;
    return;
  }
  if (txtRowNum == 0) {
    printf("GDS ERROR in %s txt-file section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- no uncommented lines in: %s\n", txtChar);
    validity = false;
    return;
  }
  if (txtRowNum != totalReps) {
    printf("GDS ERROR in %s txt-file section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- the number of rows in %s NOT match the total number of time points", txtChar);
    validity = false;
    return;
  }

  std::vector< VB_Vector *> txtCov;
  for (int i = 0; i < txtColNum; i++) {
    VB_Vector *txtVec = new VB_Vector(txtRowNum);
    txtVec->setAll(0);
    txtCov.push_back(txtVec);
  }
  int readStat = readTxt(txtChar, txtCov); 
  if (readStat == 1) {
    printf("GDS ERROR in %s txt-file section [ended at line %d]:\n",
           inputFilename.c_str(), lineNum);
    printf("--- %s does NOT have exactly the same number of elements in all rows", txtChar);
    validity = false;
    return;
  }
}

/* addTxt() adds each column in txt file as a seperate covariate */
void gSession::addTxt()
{
  const char *txtChar = txtFile.c_str();
  size_t txtColNum = getTxtColNum(txtChar);
  size_t txtRowNum = getTxtRowNum(txtChar);
  std::vector< VB_Vector *> txtCov;
  for (size_t i = 0; i < txtColNum; i++) {
    VB_Vector *txtVec = new VB_Vector(txtRowNum);
    txtVec->setAll(0);
    txtCov.push_back(txtVec);
  }
  readTxt(txtChar, txtCov);

  for (size_t i = 0; i < txtColNum; i++) {
    // set up varvec
    newVar.varvec = VB_Vector(upSampling(txtCov[i], TR / tmpResolve));
    newCovMod();
    // set up varname
    if (i < covName.size())
      newVar.varname = covName[i];
    else {
      char foo[BUFLEN];
      sprintf(foo, "txtVar-%d", (int)i + 1);
      newVar.varname = foo;
    }
    // put in covList array
    covList.push_back(newVar);
  }
}

/* cpCov() duplicates a covariate that already exists.
 * "newcov cp" syntax is written mainly written for derivative and exponential 
 * options, in which new covariate(s) will be generaged based on a certain covariate.
 * The base covariate is labelled by singleID */
void gSession::cpCov()
{
  covList[singleID].cpCounter++;
  prepNew();
  // reset the name
  char foo[BUFLEN];
  sprintf(foo, "_cp%d", covList[singleID].cpCounter);
  newVar.varname = covList[singleID].varname + string(foo);

  if (derivFlag)
    derivMod();
  else if (expnFlag)
    expnMod();
  else {
    newCovMod();
    // set up varname
    if (covName.size())
      newVar.varname = covName[0];
    // put in covList array
    covList.push_back(newVar);
  }
}

/* derivMod() is similar to newCovMod(), but the option list includes "deriv",
 * which could add more than one new covariate. THat's the key difference. */
void gSession::derivMod()
{
  // Execute option(s) before "deriv"
  for (int i = 0; i < derivIndex; i++)
    singleOpt(newVar, option[i]);
  
  // Execute "deriv" option, which may generate more than one new covariate 
  vector <Covar> newList;
  VB_Vector baseVec = newVar.varvec;
  for (size_t k = 0; k < (uint32)derivNum; k++) {
    Covar tmpVar;
    tmpVar.type = newVar.type;
    tmpVar.group = newVar.group;
    tmpVar.varvec = *derivative(&baseVec);
    baseVec = tmpVar.varvec;
    char foo[BUFLEN];
    sprintf(foo, " [deriv #%d]", (int)k); 
    tmpVar.varname = newVar.varname + string(foo);
    // Execute option(s) after "deriv"
    for (size_t j = derivIndex + 1; j < option.size(); j++) 
      singleOpt(tmpVar, option[j]);

    // if covName is available, use it
    if (k < covName.size())
      tmpVar.varname = covName[k];

    // Put new covariate into newList temporarily
    newList.push_back(tmpVar);
  }
  
  // Add covariates in newList right after the base covariate
  for (int k = 0; k < derivNum; k++)
    covList.insert(covList.begin() + singleID + k + 1, newList[k]);

  // Clean up derivative-related variables
  newList.clear();
  derivNum = 0;
  derivIndex = -1;
}

/* expnMod() is similar to newCovMod(), but the option includes "expn",
 * which will add a new covariate that is next to the base covariate 
 * The reason why this function is written is because the new covariate 
 * will be added right after */
void gSession::expnMod()
{
  // Modify covariate based on options
  newCovMod();
  // set up varname
  if (covName.size())
    newVar.varname = covName[0];
  // put the new covariate right after original one in covList array
  covList.insert(covList.begin() + singleID + 1, newVar);
}

/* getCovID() converts an input string (covariate name) into its index in covList;
 * returns -1 if the covariate name is not found at all, returns -2 if more than one
 * covariate have same name, returns the real covariate index if there is one and only 
 * one covariate's full name matches the input string*/
int gSession::getCovID(string inputStr)
{
  vector <unsigned> matchID;
  for (unsigned i = 0; i < covList.size(); i++) {
    if (getFullName(i) == inputStr)
      matchID.push_back(i);
  }

  if (matchID.size() == 1)
    return matchID[0];
  else if (matchID.size() == 0)
    return -1;
  else
    return -2;
}

// /* Check derivative type string: only interest no interest permitted */
// char gSession::chkDerivType(string inputStr)
// {
//   if (inputStr == "Interest" || inputStr == "interest")
//     return 'I';
//   if (inputStr == "I" || inputStr == "i")
//     return 'I';
//   if (inputStr == "NoInterest" || inputStr == "nointerest")
//     return 'N';
//   if (inputStr == "N" || inputStr == "n")
//     return 'N';
//   return 'u';
// }

/* Build G (and preG) matrix based on input file */
void gSession::writeG()
{
  // Check if the analysis folder exists. If not, make it.
  if (gDirFlag) {
    if (!(vb_direxists(dirname)))
      mkdir(dirname.c_str(),0777);
  }

  int colNum = covList.size();
  int pregRowNum = totalReps * TR / tmpResolve;
  int gRowNum = totalReps;

  VBMatrix pregMatrix(pregRowNum, colNum);
  VBMatrix gMatrix(gRowNum, colNum);

  // Add date when this file is created
  string myTime, myDate, timeHeader;
  // maketimedate(): defined in libvoxbo/vbutil.cpp, recommended by Dan
  maketimedate(myTime, myDate);
  timeHeader = (string)"DateCreated:\t" + myTime + (string)"_" + myDate + (string)"\n";
  pregMatrix.AddHeader(timeHeader);
  gMatrix.AddHeader(timeHeader);

  char foo[BUFLEN];
  sprintf(foo, "TR(ms):\t\t%d", TR); 
  const string tmpString(foo);
  pregMatrix.AddHeader(tmpString);
  gMatrix.AddHeader(tmpString);
  sprintf(foo, "Sampling(ms):\t%d", tmpResolve); 
  const string smplString(foo);
  pregMatrix.AddHeader(smplString);
  gMatrix.AddHeader(smplString);

  if (condfxn.length() && condKey.size()) {
    pregMatrix.AddHeader((string)"ConditionFile:\t" + condfxn);
    gMatrix.AddHeader((string)"ConditionFile:\t" + condfxn);
    // "Condition:<TAB>n<TAB>keyText" line
    for (size_t i = 0; i < condKey.size(); i++) {
      sprintf(foo, "Condition:\t%d\t%s", (int)i, condKey(i));
      const string keyString2(foo);
      pregMatrix.AddHeader(keyString2);
      gMatrix.AddHeader(keyString2);
    }
  }
  // Add a blank line here
  pregMatrix.AddHeader((string)"\n");
  gMatrix.AddHeader((string)"\n");

  // "Parameter:<TAB>n<TAB>Interest<TAB>varText" line
  char varType;
  const char *fullType;
  const char *varText;
  for (int i = 0; i < colNum; i++) {
    varType = covList[i].type;
    // Covariate full name
    if (covList[i].group.length()) {
      string nameStr = covList[i].group + "->" + covList[i].varname;
      varText = nameStr.c_str();
    }
    else
      varText = covList[i].varname.c_str();
    // Covariate full type
    if (varType == 'I')
      fullType = "Interest";
    else if (varType == 'N')
      fullType = "NoInterest";
    else if (varType == 'K')
      fullType = "KeepNoInterest"; 
    else 
      fullType = "Undefined"; 

    sprintf(foo, "Parameter:\t%d\t%s\t%s", i, fullType, varText);     
    const string headerString(foo);
    pregMatrix.AddHeader(headerString);
    gMatrix.AddHeader(headerString);
  }

  int downRatio = TR / tmpResolve;
  VB_Vector tmpVec, tmpVec2;
  for (int i = 0; i < colNum; i++) {
    // The original covariates are loaded into pregMatrix
    tmpVec = covList[i].varvec;
    // If meanAll is enabled, mean center all but intercept
    if (meanAll && tmpVec.getVariance() > 1e-15) 
      tmpVec.meanCenter();
    pregMatrix.SetColumn((const int)i, tmpVec);
    // Downsampling each covariate and load the downsampled vector into gMatrix
    tmpVec2 = *downSampling(&(covList[i].varvec), downRatio);
    // If meanAll is enabled, mean center all but intercept
    if (meanAll && tmpVec2.getVariance() > 1e-15) 
      tmpVec2.meanCenter();
    gMatrix.SetColumn((const int)i, tmpVec2);;
  }

  const string gname(gFilename + ".G");
  const string pregname(gFilename + ".preG"); 

  // Write *.G file now
  printf("\ngds is writing: %s\t... ", gname.c_str()); 
  gMatrix.WriteFile(gname);
  gMatrix.clear();
  printf("done\n");

  // Write *.preG file now
  printf("gds is writing: %s\t... ", pregname.c_str()); 
  pregMatrix.WriteFile(pregname);
  pregMatrix.clear();

  printf("done\n\n");
}

/* readG() reads the input VBMatrix and loads parameters into the current session */
void gSession::readG(const char *gdsName, const char *matName)
{
  reset4openG();

  VBMatrix inputG;
  inputG.header.clear();
  inputG.ReadHeader(matName);
  gHeaderInfo myGInfo;
  myGInfo.getInfo(inputG);
  if (myGInfo.TR != -1)
    TR = myGInfo.TR;
  totalReps = myGInfo.rowNum;
  if (myGInfo.sampling) {
    tmpResolve = myGInfo.sampling;
    samplingFlag = true;
  }
  if (myGInfo.condStat) {
    condfxn = myGInfo.condfxn;
    condKey = tokenlist(myGInfo.condKey);
  }
  if (!validateOnly)
    buildCovList(gdsName, matName);
}

/* chkG() checks the validity of input VBMatrix */
bool gSession::chkG(const char *gdsName, const char *matName)
{
  if (access(matName,R_OK)) {
    printf("GDS ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s not readable\n", matName);
    validity = false;
    return false;
  }

  VBMatrix inputG;
  inputG.header.clear();
  if (inputG.ReadHeader(matName)) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- invalid G matrix file format: %s\n", matName);
    validity = false;
    return false;
  }
  if (inputG.m == 0) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- G matrix file does not have any rows: %s\n", matName);
    validity = false;
    return false;
  }
  if (inputG.n == 0) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- G matrix file does not have any columns: %s\n", matName);
    validity = false;
    return false;
  }

  gHeaderInfo myGInfo;
  myGInfo.getInfo(inputG);
  int gStat = myGInfo.chkInfo();
  if (gStat == 1) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: # of covariates does not match # of columns\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 2) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: TR in the header is not a multiple of sampling rate\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 3) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: condition function in the header not readable\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 4) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: condition function has different number of keys in header and content\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 5) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: condition function defined in the header has different keys in header and content\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 6) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: # of condition key lines in the header is different from that in condition function\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 7) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: # of elements in condition function is NOT a multiple of total number of time points\n", matName); 
    validity = false;
    return false;
  }
  if (gStat == 8) {
    printf("GDS SYNTAX ERROR in %s [line %d]:\n", gdsName, lineNum);
    printf("--- %s: condition function can NOT be upsampled with the sampling rate\n", matName); 
    validity = false;
    return false;
  }
  
  return true;
}

/* chkPreG() makes sure *.preG file is valid, returns true if it is, false otherwise */
bool gSession::chkPreG(const char *gdsName, const char *matName)
{
  string pregName(matName);
  int dotPost = pregName.rfind(".");
  pregName.erase(dotPost);
  pregName.append(".preG");

  if (access(matName,R_OK)) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: %s not readable\n", pregName.c_str());
    return false;
  }

  VBMatrix inputPreG;
  inputPreG.header.clear();
  if (inputPreG.ReadHeader(pregName)) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: %s has invalid matrix format\n", pregName.c_str());
    return false;
  }
  else if (inputPreG.m == 0) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: %s does not have any rows\n", pregName.c_str());
    return false;
  }
  else if (inputPreG.n == 0) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: %s does not have any columns\n", pregName.c_str());
    return false;
  }
  // preG file header info
  gHeaderInfo pregInfo = gHeaderInfo(inputPreG);

  // G file header info
  VBMatrix inputG;
  inputG.header.clear();
  inputG.ReadHeader(matName);
  gHeaderInfo myGInfo;
  myGInfo.getInfo(inputG);

  if (myGInfo.TR != pregInfo.TR) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different TR in G and preG file\n"); 
    return false;
  }
  if (myGInfo.colNum != pregInfo.colNum) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different number of columns in G and preG file\n"); 
    return false;
  }
  if (myGInfo.sampling != pregInfo.sampling) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different sampling rate in G and preG file\n"); 
    return false;
  }
  if (myGInfo.TR && myGInfo.sampling && myGInfo.rowNum * myGInfo.TR / myGInfo.sampling != pregInfo.rowNum) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: # of rows in preG is NOT the upsampled number based on G file\n");
    return false;
  }
  if (myGInfo.condfxn != pregInfo.condfxn) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different condition function filenames in G and preG file\n"); 
    return false;
  }
  if (myGInfo.condfxn.length() && cmpElement(myGInfo.condKey, pregInfo.condKey) != 0) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different Condition lines in G and preG file\n"); 
    return false;
  }
  if (cmpElement(myGInfo.nameList, pregInfo.nameList) != 0) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different covariate names in G and preG file\n"); 
    return false;
  }
  if (cmpElement(myGInfo.typeList, pregInfo.typeList) != 0) {
    printf("GDS INFO in %s [line %d]:\n", gdsName, lineNum);
    printf("--- preG file ignored: different covariate types in G and preG file\n"); 
    return false;
  }

  return true;
}

/* buildCovList() reads a valid G matrix file and put covariate parameters into 
 * covList. If the argument is true, use preG file, otherwise use G file.     */
void gSession::buildCovList(const char *gdsName, const char *matName)
{
  // Make sure TR/totalReps/sampling are good
  if (!doneCommon)
    chkCommon();
  // exit if invalid
  if (!validity)
    return;
  // Clear previous covariates
  if (covList.size())
    covList.clear();

  // Check preG file's status to decide whether G or preG should be used
  bool pregStat = chkPreG(gdsName, matName);
  string myGname(matName);
  if (pregStat) {
    int dotPost = myGname.rfind(".");
    myGname.erase(dotPost);
    myGname.append(".preG");
  }
  // Load G/preG information to covList array
  VBMatrix inputG(myGname);
  gHeaderInfo myGInfo(inputG);
  Covar tmpCov;
  int strLen, separator;
  VB_Vector tmpVec;
  int upRatio = TR / tmpResolve;
  int vecSize;
  for (int i = 0; i < myGInfo.colNum; i++) {
    strLen = myGInfo.nameList[i].length();
    separator = myGInfo.nameList[i].rfind("->");
    // Does the covariate belong to a group?
    if (separator < 0 || separator > strLen - 1) {
      tmpCov.group = "";
      tmpCov.varname = myGInfo.nameList[i];
    }
    else {
      tmpCov.group = myGInfo.nameList[i].substr(0, separator - 1);
      tmpCov.varname = myGInfo.nameList[i].substr(separator + 1, strLen - 1);
    }
    tmpCov.type = myGInfo.typeList[i].at(0);
    // No upsampling when using preG file
    if (pregStat) {
      tmpCov.varvec = VB_Vector(totalReps * TR / tmpResolve);
      tmpVec=inputG.GetColumn(i);
      vecSize = tmpVec.size();
      for (int j = 0; j < vecSize; j++)
        tmpCov.varvec[j] = tmpVec[j];
    }
    // Upsample the G file based on sampling rate
    else {
      tmpVec = inputG.GetColumn(i);
      vecSize = tmpVec.size();
      VB_Vector downVector(vecSize);
      for (int j = 0; j < vecSize; j++)
        downVector[j] = tmpVec[j];
      tmpCov.varvec = *upSampling(&downVector, upRatio);
    }
    covList.push_back(tmpCov);
  }
}

/* saveLabel() saves the condition keys into a txt file */
void gSession::saveLabel(const char *outFile)
{
  FILE *condLabFile;
  condLabFile = fopen(outFile, "w");
  printf("gds is saving condition labels in %s ... ", outFile);
  fprintf(condLabFile, ";VB98\n");
  fprintf(condLabFile, ";TXT1\n\n");  
  for (size_t i = 0; i < condKey.size(); i++) {
    fprintf(condLabFile, condKey(i));
    fprintf(condLabFile, "\n");
  }
  
  fclose(condLabFile);
  printf("done\n\n");
}

/* saveSingleCov() saves a certain covariate as ref file */
void gSession::saveCov(int covID, const char *filename)
{
  string myComments = ";; Name: " + getFullName((unsigned) covID) + "\n";
  FILE *covFile;
  covFile = fopen(filename, "w");
  printf("gds is saving covariate %s in %s ... ", getFullName((unsigned) covID).c_str(), filename);
  fprintf(covFile, ";VB98\n");
  fprintf(covFile, ";REF1\n");
  fprintf(covFile, ";\n");
  fprintf(covFile, ";; Single covariate from G matrix\n");
  fprintf(covFile, myComments.c_str());
  fprintf(covFile, ";\n");

  char numLine[100];
  VB_Vector downVec = *downSampling(&(covList[covID].varvec), TR / tmpResolve);
  for (unsigned i = 0; i < downVec.getLength(); i++) {
    sprintf(numLine, "%f\n", downVec.getElement(i)); 
    fprintf(covFile, numLine);
  }
  
  fclose(covFile);
  printf("done\n\n");
}

/* chkDS() checks the downsample option in chkeff section;
 * returns 0 if the input string argument is invalid;
 * returns 1 if no downsample at all;
 * returns 2 if downsample before convolution;
 * returns 3 if downsample after convolution; */
int gSession::chkDS(string inputStr)
{
  if (inputStr == "NO" || inputStr == "No" || inputStr == "no" || inputStr == "N" || inputStr == "n")
    return 1;
  if (inputStr == "BEFORE" || inputStr == "Before" || inputStr == "before")
    return 2;
  if (inputStr == "AFTER" || inputStr == "After" || inputStr == "after")
    return 3;

  return 0;
}

/* chkEffType() reads the argument and returns chkeff covariate type;
 * It is similar to chkOrthType(), but only three types are acceptable: A/I/N 
 * N includes both N and K types in efficiency check */
char gSession::chkEffType(string inputType)
{
  if (inputType == "ALL" || inputType == "All" || inputType == "all" || inputType == "A" || inputType == "a")
    return 'A';

  if (inputType == "I" || inputType == "i" || inputType == "Interest" || inputType == "interest")
    return 'I';

  if (inputType == "N" || inputType == "n" || inputType == "NoInterest" || 
      inputType == "Nointerest" || inputType == "noInterest" || inputType == "nointerest")
    return 'N';

  return 0;
}
/* baseTypeMatch() compares the base covariate type with eff-type argument, 
 * returns true i fthey match each other, otherwise false;
 * note 1: it will be always true if eff-type argument is "all";
 * note 2: in efficiency check, both N and K types are treated as type N */
bool gSession::typeMatch(char typeArg, int inputIndex)
{
  if (typeArg == 'A')
    return true;
  if (typeArg == 'I' && covList[inputIndex].type == 'I')
    return true;
  if (typeArg == 'N' && (covList[inputIndex].type == 'I' || covList[inputIndex].type == 'K'))
    return true;

  return false;
}

/* chkEffVar() makes sure chkeff variables are all set correctly */
void gSession::chkEffVar()
{
  // Give a warning if filter not defined
  if (!filterFlag) {
    printf("GDS WARNING in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- filter not defined\n");
  }
  // Error message if downsample line not found
  if (!dsFlag) {
    printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- downsample option not defined\n");
    validity = false;
    return;
  }
  // Error message if eff-type line not found
  if (!effTypeFlag) {
    printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- eff-type not defined\n");
    validity = false;
    return;
  }
  // Error message if cutoff line not found
  if (!cutoffFlag) {
    printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- eff-cutoff not defined\n");
    validity = false;
    return;
  }
  // Error message if 2000 couldn't be divided by tmpResolve
  if (2000 % tmpResolve) {
    printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- chkeff requires that 2000 be devided by the current sampling rate (%d)\n", tmpResolve);
    validity = false;
    return;
  }
  // Make sure filter file is in good shape
  if (effFilter.length()) {
    VB_Vector foo;
    int filterStat = foo.ReadFile(effFilter);
    // Is the filter file in ref format?
    if (filterStat) {
      printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- invalid file format: %s\n", effFilter.c_str());
      validity = false;
      return;
    }
    // In "downsample before convolution" option, is the filter vector good for interpolation?
    if (dsOption == 2 && 2000 % TR) {
      printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- \"downsample before convolution\" option requires 2000 be divided by TR (%d)\n", tmpResolve);
      validity = false;
      return;
    }
    // Does the filter include too many elements?
    if ((int) foo.getLength() * 2000 > totalReps * TR) {
      printf("GDS SYNTAX ERROR in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- filter file includes too many elements: %s\n", effFilter.c_str());
      validity = false;
      return;
    }
  }
  if (!validateOnly) {
    double tmpVal = mySession.getRawEff(effBaseIndex);
    // Make sure the base covariate's elements are not all zero (like interept)
    if (tmpVal == -1) {
      printf("GDS ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- all elements are zero, invalid base covariate for efficiency check: %s\n",
             getFullName(effBaseIndex).c_str());
      validity = false;
    }
    // Make sure the base covariate's raw efficiency is not zero
    else if (tmpVal == 0) {
      printf("GDS ERROR in %s [line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- raw efficiency is zero, invalid base covariate for efficiency check: %s\n",
             getFullName(effBaseIndex).c_str());
      validity = false;
    }
    else
      baseEff = tmpVal;
  }
}

/* exeEff() will collect the covariate ID that will be deleted based 
 * on the chkeff parameters and execute the removal               */
void gSession::exeEff()
{
  vector <int> effID;
  // collect covariate index whose VALID efficiency is less than the cutoff value
  for (unsigned i = 0; i < covList.size(); i++) {
    if (!typeMatch(effType, (int)i))
      continue;

    double tmpEff = getRawEff(i);
    if (tmpEff == -1) {
      printf("GDS WARNING in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
      printf("--- covariate has invalid efficiency value: %s\n", getFullName(i).c_str());
      printf("--- efficiency check on this covariate skipped\n");
      continue;
    }
    double finalEff = tmpEff / baseEff;
    if (finalEff <= 0.01)
      finalEff = 0;
    if (finalEff < cutoff)
      effID.push_back(i);
  }
  // Start deleting covariates whose index is on effID list
  for (unsigned j = 0; j < effID.size(); j++) {
    int covIndex = effID[j] - j;
    printf("GDS INFO in %s chkeff section [ended at line %d]:\n", inputFilename.c_str(), lineNum);
    printf("--- covariate deleted because of efficiency check: %s\n", getFullName(covIndex).c_str());
    covList.erase(covList.begin() + covIndex);
  }
}
 
/* getFilterVec() reads the efficiency check filter and returns the filter 
 * vb_vector based on the downsample option */
VB_Vector gSession::getFilterVec()
{
  int orderG = totalReps * TR / tmpResolve;
  VB_Vector filterVec;

  // "Do not downsample" or "Downsample after convolution" 
  if (dsOption != 2) {
    // if filter is available
    if (effFilter.length()) {
      filterVec.ReadFile(effFilter);
      filterVec.sincInterpolation(2000 / tmpResolve);
      if ((unsigned) orderG > filterVec.getLength()) {
        VB_Vector tmpVec(orderG - filterVec.getLength());
        tmpVec.setAll(0);
        filterVec.concatenate(tmpVec);
      }
    }
    // if filter is not available
    else {
      filterVec = VB_Vector(orderG);
      filterVec.setAll(0);
      filterVec[0] = 1.0;
    }
  }
  // Downsample before convolution && filter is available 
  else if (effFilter.length()) {  
    filterVec.ReadFile(effFilter);
    filterVec.sincInterpolation(2000 / TR);
    if (totalReps > (int) filterVec.getLength()) {
      VB_Vector tmpVec(totalReps - filterVec.getLength());
      tmpVec.setAll(0);
      filterVec.concatenate(tmpVec);
    }
  }
  // Downsample before convolution && filter file not available 
  else {  
    filterVec = VB_Vector(totalReps);
    filterVec.setAll(0);
    filterVec[0] = 1.0;
  }
  // mean center and normalize the filter
  filterVec.meanCenter();
  filterVec.normMag();

  return filterVec;
}

/* getRawEff() is copied from gdw.cpp, it returns a certain covariate's raw 
 * efficiency value, inputIndex is that covariate's index in covList;
 * returns -1 if all elements in that covariate are zero */
double gSession::getRawEff(unsigned inputIndex)
{
  double neuralPower, boldPower;
  neuralPower = getSquareSum(covList[inputIndex].varvec);
  if (!neuralPower) {
    return -1.0;
  }

  boldPower = getBold(covList[inputIndex].varvec);
  return boldPower / neuralPower;
}

/* getBold() is copied from gdw.cpp, 
 * it returns the input VB_Vector's bold value */
double gSession::getBold(VB_Vector inputVec)
{
  int ratio = TR / tmpResolve;
  double boldPower = 0;

  VB_Vector vec2, vec3, filteredParam;
  VB_Vector filterVec = getFilterVec();
  switch(dsOption) {
  case 1:  // Do not downsample
    filteredParam = fftConv(&inputVec, &filterVec, true);
    boldPower = getSquareSum(filteredParam);
    break;
  case 2:  // Downsample before convolution
    vec2 = downSampling(&inputVec, ratio);
    filteredParam = fftConv(&vec2, &filterVec, true);
    boldPower = getSquareSum(filteredParam) * (double)ratio;
    break;
  case 3:  // Downsample after convolution
    filteredParam = fftConv(&inputVec, &filterVec, true);
    vec3 = *downSampling(&filteredParam, ratio);
    boldPower = getSquareSum(vec3) * (double)ratio;      
    break;
  }

  return boldPower;
}

/* getBold() is copied from gdw.cpp, 
 * it returns the sum of squares of the elements in inputVec */
double gSession::getSquareSum(VB_Vector inputVec)
{
  double sum = 0;
  for (int i = 0; i < (int)inputVec.getLength(); i++)
    sum += inputVec[i] * inputVec[i];
  return sum;
}

/********************************************************************************
 * gHeaderInfo class is written copied from gdw.cpp 
 ********************************************************************************/
/* Basic constructor */
gHeaderInfo::gHeaderInfo()
{
  init();
}

/* This constructor takes VBMatrix as an argument */
gHeaderInfo::gHeaderInfo(VBMatrix inputG)
{
  init();
  getInfo(inputG);
}

/* Destructor */
gHeaderInfo::~gHeaderInfo()
{

}

/* Initialize parameters */
void gHeaderInfo::init()
{
  TR = -1;
  rowNum = colNum = 0;
  sampling = 0;
  condfxn="";
  condStat = false;
  condKey.clear();
  typeList.clear();  
  nameList.clear();
}

/* Collect TR, sampling, totalReps, condition function information 
 * because the .G and .preG files generated by Geoff's code doesn't include TR and sampling 
 * rate information, we cannot simply assume that the third line will hold TR and the 4th 
 * line will hold sampling rate value. It is only available in my header files.
 * The header is checked line by line for TR and sampling rate value. 
 * This is for compatibility of different header format between mine and Geoff's. 
 * When the header is eventually fixed, we can check a certain line instead of each line. */
void gHeaderInfo::getInfo(VBMatrix inputG)
{
  vector<string>tmpHeader = inputG.header;
  int strLen = -1, tab1st = -1, tab2nd = -1;
  string tmpStr, key, name, type;
  for (unsigned i = 0; i < tmpHeader.size(); i++) {
    strLen = tmpHeader[i].length();
    tab1st = tmpHeader[i].find("\t");
    if ((tmpHeader[i]).substr(0, 7) == "TR(ms):") {
      tmpStr = tmpHeader[i].substr(tab1st + 1, strLen - 1);
      TR = atoi(tmpStr.c_str());
    } 
    else if ((tmpHeader[i]).substr(0, 13) == "Sampling(ms):") {
      tmpStr = tmpHeader[i].substr(tab1st + 1, strLen - 1);
      sampling = atoi(tmpStr.c_str());
    }
    else if ((tmpHeader[i]).substr(0, 14) == "ConditionFile:")
      condfxn = tmpHeader[i].substr(tab1st + 1, strLen - 1);
    else if ((tmpHeader[i]).substr(0, 13) == "ConditionKey:") {
      tab2nd = tmpHeader[i].rfind("\t");
      key = tmpHeader[i].substr(tab2nd + 1, strLen - 1);
      condKey.Add(key);
    }
    else if ((tmpHeader[i]).substr(0, 10) == "Condition:") {
      tab2nd = tmpHeader[i].rfind("\t");
      key = tmpHeader[i].substr(tab2nd + 1, strLen - 1);
      condKey.Add(key);
    }
    else if ((tmpHeader[i]).substr(0, 10) == "Parameter:") { 
      string type_name = tmpHeader[i].substr(tab1st + 1, strLen - 1);
      tab1st = type_name.find("\t");
      tab2nd = type_name.rfind("\t");
      type = type_name.substr(tab1st + 1, tab1st + 1);
      name = type_name.substr(tab2nd + 1, type_name.length() - 1);
      typeList.Add(type);
      nameList.Add(name);
    }
  }
  tmpHeader.clear();
  rowNum = inputG.m, colNum = inputG.n;
}

/* Check if the information in header is consistent or not */
int gHeaderInfo::chkInfo()
{
  if (typeList.size() != (uint32)colNum || nameList.size() != (uint32)colNum)
    return 1;
  if (TR && sampling && TR % sampling != 0)
    return 2;

  return chkCondfxn();
}

/* Make sure condition function is in good shape */
int gHeaderInfo::chkCondfxn()
{
  if (!condfxn.length())
    return 0;

  tokenlist condKeyInFile;
  condVector = new VB_Vector();
  int refStat = getCondVec(condfxn.c_str(), condKeyInFile, condVector);
  if (refStat == -1) // Quit if the file isn't readable
    return 3;
  if (refStat == -2)
    return 4;
  if (refStat == 1) 
    return 5;
  if (condKey.size() != condKeyInFile.size()) 
    return 6;

  int condLen = condVector->getLength();
  // Make sure the number of elements in condition function is a multiple of total time points
  if (condLen % rowNum != 0) 
    return 7;

  // Make sure condition function can be upsampled
  if (sampling && (TR / sampling) % (condLen / rowNum) != 0)
    return 8;

  condStat = true;
  return 0;
}

