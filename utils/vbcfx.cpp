
// vbcfx.cpp
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
// original version written by Dan Kimberg

using namespace std;

#include "glmutil.h"
#include "vbio.h"

string infilename, newRef, avgName;

int parseArg(tokenlist);
void printHelp();
bool chkCfgStat(int, const char *);
bool chkOrgFile(string);
bool chkNewFile(string);
void writeAvg(string, tokenlist, VB_Vector *);
void wrtAvgHead(FILE *);

int main(int argc, char *argv[]) {
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  int argStat = parseArg(args);
  // print out usage message for illegal arguments
  if (argStat == 0) {
    printHelp();
    exit(1);
  }
  // Make sure original condition function file exists and has right permission
  if (!chkOrgFile(infilename)) exit(1);

  tokenlist condKey;
  VB_Vector *outVec = new VB_Vector();
  int refStat = getCondVec(infilename.c_str(), condKey, outVec);
  // Make sure the original condition function is in correct format
  if (!chkCfgStat(refStat, infilename.c_str())) exit(1);
  // Make sure new ref file's name is valid
  if ((argStat == 1 || argStat == 3) && !chkNewFile(newRef)) exit(1);
  // Make sure new avg file's name is valid
  if ((argStat == 2 || argStat == 3) && !chkNewFile(avgName)) exit(1);

  // Generate new condition function in numerical format
  if (newRef.length()) outVec->WriteFile(newRef);
  // Generate avarages txt file
  if (avgName.length()) writeAvg(avgName, condKey, outVec);

  delete outVec;
  return 0;
}

/* Parse arguments in command line
 * returns 0 if arguments are in illegal format;
 * returns 1 if only new condition function filename is given;
 * returns 2 if only averages filename is given;
 * returns 3 if both new condtion function and averages filenames are given. */
int parseArg(tokenlist inputArg) {
  int argNum = inputArg.size();
  if (argNum != 2 && argNum != 3 && argNum != 5) return 0;

  int orgIndex = -2, aFlag = -2, nFlag = -2;
  for (int i = 0; i < argNum; i++) {
    if (inputArg[i] == "-a") {
      if (aFlag != -2) return 0;
      aFlag = i;
    } else if (inputArg[i] == "-n") {
      if (nFlag != -2) return 0;
      nFlag = i;
    } else if (orgIndex == -2 && i != aFlag + 1 && i != nFlag + 1)
      orgIndex = i;
  }

  if (orgIndex == -2 || nFlag - aFlag == 1 || aFlag - nFlag == 1) return 0;
  if (nFlag == argNum - 1 || aFlag == argNum - 1) return 0;

  infilename = inputArg[orgIndex];
  if (argNum == 2) {
    if (aFlag != -2 || nFlag != -2) return 0;
    newRef = inputArg[1];
    return 1;
  }
  // Three arguments: orginal filename, -a/-n, followed by new filename
  if (argNum == 3) {
    if (aFlag != -2 && nFlag != -2) return 0;
    if (aFlag == -2 && nFlag == -2) return 0;
    if (nFlag != -2) {
      newRef = inputArg[nFlag + 1];
      return 1;
    }
    if (aFlag != -2) {
      avgName = inputArg[aFlag + 1];
      return 2;
    }
  }
  // Five arguments: original filename, -n, followed by new ref, -a, averages
  // filename
  if (argNum == 5) {
    if (aFlag == -2 || nFlag == -2) return 0;
    newRef = inputArg[nFlag + 1];
    avgName = inputArg[aFlag + 1];
    return 3;
  }

  return 0;
}

/* Check the original condition function file's status */
bool chkOrgFile(string inputName) {
  const char *infilename = inputName.c_str();
  if (access(infilename, R_OK)) {
    printf("%s: not a readable file\n", infilename);
    return false;
  }

  return true;
}

/* Checks original condition function status */
bool chkCfgStat(int refStat, const char *cfgName) {
  if (refStat == -1) {
    printf("File not readable: %s\n", cfgName);
    return false;
  } else if (refStat == -2) {
    printf("Error: different number of keys in header and content: %s\n",
           cfgName);
    return false;
  } else if (refStat == 1) {
    printf("Error: different keys in header and content: %s\n", cfgName);
    return false;
  }

  return true;
}

/* Check the availability and permission of the new file */
bool chkNewFile(string inputName) {
  int newStat = checkOutputFile(inputName.c_str(), FALSE);
  if (newStat == 0) {
    printf("%s: File exists and parent directory not writeable\n",
           inputName.c_str());
    return false;
  }
  if (newStat == 1) {
    printf("%s: File exists\n", inputName.c_str());
    return false;
  }
  if (newStat == 2) {
    printf("%s: Parent directory not writeable\n", inputName.c_str());
    return false;
  }

  return true;
}

/* This function writes averages file */
void writeAvg(string avgName, tokenlist condKey, VB_Vector *condVec) {
  FILE *fp = fopen(avgName.c_str(), "w");
  if (!fp) {
    printf("Averages file generation failed: %s\n", avgName.c_str());
    return;
  }

  wrtAvgHead(fp);
  int keyNum = condKey.size();
  std::vector<string> keyLines;
  for (int j = 0; j < keyNum; j++) keyLines.push_back("");
  for (int i = 0; i < (int)condVec->getLength(); i++) {
    for (int j = 0; j < keyNum; j++) {
      if (j == condVec->getElement(i)) {
        char foo[1024];
        sprintf(foo, "  trial %d\n", i);
        keyLines[j] += (string)foo;
      }
    }
  }
  for (int i = 0; i < keyNum; i++) {
    fprintf(fp, "average %s\n", condKey(i));
    fprintf(fp, "  units vols\n");
    fprintf(fp, "  interval 1\n");
    fprintf(fp, "  nsamples 10\n");
    fprintf(fp, "%s", keyLines[i].c_str());
    fprintf(fp, "end\n");
    if (i != keyNum - 1) fprintf(fp, "\n");
  }

  fclose(fp);
}

/* This function writes the avg file's comment lines (copied from voxbo wiki
 * page) */
void wrtAvgHead(FILE *avgFile) {
  fprintf(avgFile, "# averages.txt\n");
  fprintf(avgFile,
          "# in this file you can specify how to trial-average your data\n");
  fprintf(avgFile,
          "# (you can also block-average or whatever else you need)\n");
  fprintf(avgFile, "# your total data points: 200\n");
  fprintf(avgFile, "# your TR: 3000\n");
  fprintf(avgFile, "#\n");
  fprintf(avgFile,
          "# each way of averaging your data gets a section that looks like "
          "the following:\n");
  fprintf(avgFile, "#\n");
  fprintf(avgFile, "# average <name>\n");
  fprintf(avgFile, "#   units <time/vols>\n");
  fprintf(avgFile, "#   interval <n>\n");
  fprintf(avgFile, "#   nsamples <n>\n");
  fprintf(avgFile, "#   tr <ms>\n");
  fprintf(avgFile, "#   trial <n>...\n");
  fprintf(avgFile, "#   trialset <first> <interval> <count>\n");
  fprintf(avgFile, "# end\n");
  fprintf(avgFile, "#\n");
  fprintf(avgFile,
          "# units: whether the other parameters are in volumes or seconds\n");
  fprintf(avgFile,
          "# interval: interval in time or volumes between samples within the "
          "trial\n");
  fprintf(avgFile, "# nsamples: number of time points to include per trial\n");
  fprintf(avgFile, "# tr: sets the tr if you're using time units\n");
  fprintf(avgFile,
          "# trial: start times/vols for your trials, you can include multiple "
          "trial lines\n");
  fprintf(avgFile,
          "# trialsets: if your trials are evenly spaced, specify the start of "
          "the\n");
  fprintf(avgFile,
          "#     first trial, the interval between trial starts, and the trial "
          "count\n");
  fprintf(avgFile, "#\n");
  fprintf(avgFile, "# lines beginning with a '#' are comments!\n\n");
}

/* Print out usage information */
void printHelp() {
  printf("VoxBo vbcfx v%s:\n", vbversion.c_str());
  printf("    Convert a text format condition function to numerical format\n");
  printf("    and/or generate averages.txt file\n\n");
  printf("Usage:\n\n");
  printf("-- To convert condition function to numerical format only:\n");
  printf("  vbcfx <input ref filename> <new ref filename>\n");
  printf("  or:\n");
  printf("  vbcfx <input ref filename> -n <new ref filename>\n");
  printf("     <input ref filename>: original text format ref file\n");
  printf("       <new ref filename>: new numerical format ref file\n\n");
  printf("-- To generate averages.txt file only:\n");
  printf("  vbcfx <input ref filename> -a <averages filename>\n");
  printf("       <average filename>: original text format ref file\n\n");
  printf("-- To convert condition function and generate averages file:\n");
  printf(
      "  vbcfx <input ref filename> -n <new ref filename> -a <averages "
      "filename>\n");
  printf("  or:\n");
  printf(
      "  vbcfx <input ref filename> -a <averages filename> -n <new ref "
      "filename>\n");
}
