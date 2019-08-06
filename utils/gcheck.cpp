
// gcheck.cpp
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

#include <stdio.h>
#include <string>

#include "glmutil.h"
#include "vbio.h"
#include "vbprefs.h"
#include "vbutil.h"

void printMS(string covStr, string gfilename);
void printLA(string gfilename);
void printDeterm(double gDeterm);
void printLP(string covStr, string gfilename);
bool chkCovIndex(vector<int> inputVec, int colNum);
void printColUse();
void printCA(string covStr, string gfilename);
void printCP(string covString, string refStr, string gfilename);
void printHelp();

int readG(string gfilename, VBMatrix &newMat);
bool chkOneIndex(string covStr, int colNum);
bool chkMultiIndex(vector<int> inputVec, int colNum);
bool chkRefIndex(int covIndex, vector<int> refIndex, int colNum);

/* main() function */
int main(int argc, char *argv[]) {
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  // Make sure there are enough arguments
  if (args.size() < 2) {
    printHelp();
    exit(0);
  }

  // "-h" will print out usage
  if (args[0] == "-h") {
    printHelp();
    exit(0);
  }
  // "-ms" will print out mean and standard deviation
  if (args[0] == "-ms") {
    if (args.size() == 3)
      printMS(args[1], args[2]);
    else
      printf("[E] gcheck usage: gcheck -ms <covariate_index> <G_filename>\n");
    exit(0);
  }
  // "-l" will print out the linear dependence for the whole G matrix
  if (args[0] == "-la") {
    if (args.size() == 2)
      printLA(args[1]);
    else
      printf("[E] gcheck usage: gcheck -la <G_filename>\n");
    exit(0);
  }
  // "-lp" will expect two arguments: 1st parsed as covariate index, 2nd as G
  // filename
  if (args[0] == "-lp") {
    if (args.size() == 3)
      printLP(args[1], args[2]);
    else
      printf(
          "[E] gcheck -lp needs two arguments: \n\
the first is a string that defines covariate index, the second is G matrix filename\n");
    exit(0);
  }
  // "-c" will check colinearity of a certain covariate in G matrix
  if (args[0] == "-c") {
    if (args.size() != 3 && args.size() != 5)
      printColUse();
    else if (args.size() == 3)
      printCA(args[1], args[2]);
    else if (args[2] == "-r")
      printCP(args[1], args[3], args[4]);
    else
      printColUse();
    exit(0);
  }

  printHelp();
  return 0;
}

/* readG() reads gfilename into a VBMatrix object newMat.
 * returns 0 if reading is correct;
 * returns 1 if the reading is incorrect. */
int readG(string gfilename, VBMatrix &newMat) {
  if (newMat.ReadHeader((string)gfilename) == 0 && newMat.m && newMat.n)
    return 0;
  else {
    printf("[E]: invalid G file format: %s\n", gfilename.c_str());
    return 1;
  }
}

/* printMS() prints out the mean and standard deviation of input G matrix file
 */
void printMS(string covStr, string gfilename) {
  VBMatrix myMat;
  if (readG(gfilename, myMat)) return;

  int colNum = myMat.n;
  if (!chkOneIndex(covStr, colNum)) return;

  VB_Vector newVector = myMat.GetColumn(atoi(covStr.c_str()));

  double vecMean = newVector.getVectorMean();
  double vecSD = sqrt(newVector.getVariance());
  printf("Mean of covariate #%s: %f\n", covStr.c_str(), vecMean);
  printf("Standard deviation of covariate #%s: %f\n", covStr.c_str(), vecSD);
}

/* chkOneIndex() checks the single input covariate index string to make sure
 * it is converted correctly and not out of range;
 * returns true if it is correct, false otherwise */
bool chkOneIndex(string covStr, int colNum) {
  // Is covStr converted correctly?
  int covIndex = atoi(covStr.c_str());
  if (covIndex == 0 && covStr != "0") {
    printf("[E]: Invalid covariate index: %s\n", covStr.c_str());
    return false;
  }
  // Is covStr out of range?
  if (covIndex < 0 || covIndex >= colNum) {
    printf("[E]: input covariate index out of range: %d\n", covIndex);
    return false;
  }
  return true;
}

/* printLA() prints out the linear dependence of the whole input G matrix file
 */
void printLA(string gfilename) {
  VBMatrix myMat;
  if (readG(gfilename, myMat)) return;
  double gDeterm = getDeterm(myMat);
  printDeterm(gDeterm);
}

/* printDeterm() prints out determinant value and evaluation hints about it */
void printDeterm(double gDeterm) {
  printf("Determinant = %f\n\n", gDeterm);
  if (gDeterm == 0)
    printf("The covariates ARE linearly dependent!\n");
  else if (gDeterm < 0.000001)
    printf("The covariates are CLOSE to linear dependence.\n");
  else
    printf("The covariates are NOT linearly dependent.\n");
}

/* printLP() prints out input G matrix file's partial linear dependence
 * based on covStr  */
void printLP(string covStr, string gfilename) {
  VBMatrix myMat;
  if (readG(gfilename, myMat)) return;

  int totalReps = myMat.m;
  int colNum = myMat.n;
  vector<int> covIndex = numberlist(covStr);

  if (covIndex.size() == 1) {
    printf(
        "[E]: only one input covariate found. Invalid linear dependence "
        "evaluation.\n");
    return;
  }

  if (!chkMultiIndex(covIndex, colNum)) return;

  int newColNum = covIndex.size();
  VBMatrix newMat(totalReps, newColNum);
  for (int i = 0; i < newColNum; i++)
    newMat.SetColumn(i, myMat.GetColumn(covIndex[i]));

  double gDeterm = getDeterm(newMat);
  printDeterm(gDeterm);
}

/* chkIndex() checks whether the input covariate index is out of range or not.
 * returns true if it is OK, false otherwise */
bool chkMultiIndex(vector<int> inputVec, int colNum) {
  if (!inputVec.size()) {
    printf("[E]: invalid input covariate index\n");
    return false;
  }

  for (int i = 0; i < (int)inputVec.size(); i++) {
    if (inputVec[i] < 0 || inputVec[i] >= colNum) {
      printf("[E]: input covariate index out of range: %d\n", inputVec[i]);
      return false;
    }
  }

  return true;
}

/* printColUse() prints out gcheck check colinearity option's usage */
void printColUse() {
  printf("[E] gcheck -c usage:\n");
  printf("        gcheck -c <base covariate index> <G_filename>\n");
  printf(
      "        gcheck -c <base_covariate_index> -r <reference_covariate_index> "
      "<G_filename>\n");
}

/* printCA() prints out covStr's colinearity relative to the rest of input G
 * matrix file */
void printCA(string covStr, string gfilename) {
  VBMatrix myMat;
  if (readG(gfilename, myMat)) return;

  if (!chkOneIndex(covStr, myMat.n)) return;

  int covIndex = atoi(covStr.c_str());
  VB_Vector currentVec = myMat.GetColumn(covIndex);

  int rowNum = myMat.m;
  int colNum = myMat.n - 1;
  VBMatrix subA(rowNum, colNum);
  int j = 0;
  for (int i = 0; i < colNum + 1; i++) {
    if (i == covIndex) continue;
    subA.SetColumn(j, myMat.GetColumn(i));
    j++;
  }

  double colVal = calcColinear(subA, currentVec);
  printf("The colinearity between covariate #%s and the rest is: %f\n\n",
         covStr.c_str(), colVal);
  if (colVal < 0) {
    printf("Invalid negative value found.\n");
    return;
  }
  printf(
      "This value is the correlation between the covariate and the best "
      "linear\n");
  printf("combination of the other independent variables.\n");
  printf(
      "This is calculated prior to any exogenous smoothing that might be "
      "applied.\n");
}

/* printCP() prints out covStr's colinearity relative to refStr covariates */
void printCP(string covStr, string refStr, string gfilename) {
  VBMatrix myMat;
  if (readG(gfilename, myMat)) return;

  if (!chkOneIndex(covStr, myMat.n)) return;

  int covIndex = atoi(covStr.c_str());
  vector<int> refIndex = numberlist(refStr);
  if (!chkRefIndex(covIndex, refIndex, myMat.n)) return;

  VB_Vector currentVec = myMat.GetColumn(covIndex);
  int rowNum = myMat.m;
  int colNum = refIndex.size();
  VBMatrix subA(rowNum, colNum);
  for (int i = 0; i < colNum; i++) {
    subA.SetColumn(i, myMat.GetColumn(refIndex[i]));
  }

  double colVal = calcColinear(subA, currentVec);
  printf("The colinearity between covariate #%s and the rest is: %f\n\n",
         covStr.c_str(), colVal);
  if (colVal < 0) {
    printf("Invalid negative value found.\n");
    return;
  }
  printf(
      "This value is the correlation between the covariate and the best "
      "linear\n");
  printf("combination of the other independent variables.\n");
  printf(
      "This is calculated prior to any exogenous smoothing that might be "
      "applied.\n");
}

/* chkRefIndex() checks whether elements in refIndex array is equal to covIndex
 * or out of range; return true if it passes, otherwise false. */
bool chkRefIndex(int covIndex, vector<int> refIndex, int colNum) {
  if (!refIndex.size()) {
    printf("[E]: invalid ref covariate index\n");
    return false;
  }

  for (int i = 0; i < (int)refIndex.size(); i++) {
    if (refIndex[i] == covIndex) {
      printf(
          "[E]: ref covariate index should not equal to the base covariate "
          "index (%d)\n",
          covIndex);
      return false;
    }

    else if (refIndex[i] < 0 || refIndex[i] >= colNum) {
      printf("[E]: input covariate index out of range: %d\n", refIndex[i]);
      return false;
    }
  }

  return true;
}

/* printHelp() prints out usage information */
void printHelp() {
  printf("\nVoxBo gcheck (v%s) Usage:\n\n", vbversion.c_str());

  printf("--- gcheck -ms <covariate_index> <G_filename>\n");
  printf(
      "      print out mean and standard deviation values of a certain "
      "covariate in G matrix file\n");
  printf(
      "      IMPORTANT: input covariate index starts from 0 instead of 1!\n\n");

  printf("--- gcheck -la <G_filename>\n");
  printf(
      "      print out linear dependence information of the whole G "
      "matrix\n\n");

  printf("--- gcheck -lp <selected_covariate_index> <G_filename>\n");
  printf(
      "      print out linear dependence information of the sub matrix built "
      "from certain cavariate(s)\n");
  printf(
      "      For example, to check linear dependency of the covariate #1, #3, "
      "#4 and #5 in\n");
  printf("      foo.G file, type:\n");
  printf("           gcheck -lp 1:3:4:5 foo.G\n");
  printf("      or:  gcheck -lp 1:3-5 foo.G\n\n");

  printf("--- gcheck -c <covariate_index> <G_filename>\n");
  printf(
      "      print out a certain covariate's colinearity value with the rest "
      "of the covarite(s)\n");
  printf("      in G matrix file.\n\n");

  printf(
      "--- gcheck -c <covariate_index> -r <reference_covarite_index> "
      "<G_filename>\n");
  printf(
      "      print out a certain covarite's colinearity value with sub G "
      "matrix built from reference\n");
  printf("      covarite(s) in G matrix file.\n");
  printf(
      "      For example, to print out the colinearity value of covarite #2 in "
      "foo.G file with\n");
  printf("      covarites #3, #5 and #6, type:\n");
  printf("          gcheck -c 2 -r 3:5:6 foo.G\n");
  printf("      or: gcheck -c 2 -r 3:5-6 foo.G\n\n");
}
