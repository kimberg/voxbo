
// sortmvpm.cpp
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

#include "vbio.h"
#include "glmutil.h"

void printHelp();
int  chkInFile(const char *);
void sortmvpm(const char *, const char *);

int main(int argc, char *argv[])
{
  // if not exactly two arguments, enter help message
  if (argc != 3) {
    printHelp();
    exit(1);
  }

  tokenlist args;
  args.Transfer(argc - 1,argv + 1);

  const char * infilename = args(0);
  if (access(infilename,R_OK)) {
    printf("%s: can't read file\nAborted ...\n", infilename);
    return 0;
  }

  int inStat = chkInFile(infilename);
  if (inStat == 1) {
    printf("%s: invalid ref format\n", infilename);
    return 0;
  }
  if (inStat == 2) {
    printf("%s: invalid mvpm file, number of elements is NOT a multiple of 7\n", infilename);
    return 0;
  }

  string outfilename = args[1];
  int outStat = checkOutputFile(outfilename.c_str(), FALSE);
  if (outStat == 0) {
    printf("%s: File exists and parent directory not writeable\n", outfilename.c_str());
    return 0;
  }
  if (outStat == 1) {
    printf("%s: File exists\n", outfilename.c_str());
    return 0;
  }
  if (outStat == 2) {
    printf("%s: Parent directory not writeable\n", outfilename.c_str());
    return 0;    
  }
  
  sortmvpm(infilename, (const char *)outfilename.c_str());  
  return 0;
}

void printHelp()
{
  printf("VoxBo sortmvpm: \n");
  printf("  Convert a movement parameter ref file to a new file with 7 elements per line\n\n");
  printf("usage:\n");
  printf("  sortmvpm <input mvpm filename> <output filename>\n");
  printf("        <input mvpm filename>:  original movement parameter ref file\n");
  printf("            <output filename>:  new file that has 7 elements per line\n");
}

/* checkInFile() checks the input file to make sure it is a valid 
 * movement parameter ref file. 
 * returns 0 if it is a good mvpm ref file
 * returns 1 if it is not even a ref format;
 * returns 2 if it is ref file but number of elements is not a multiple of 7 */
int chkInFile(const char * fileName)
{
  VB_Vector testVec;
  if (testVec.ReadFile((string)fileName))
    return 1;

  int vecSize = testVec.getLength();
  if (vecSize % 7 == 0) 
    return 0;
  return 2;
}

/* sortmvpm() reads the first argument as the input filename and put 7 elements 
 * into a new file (2nd argument) */
void sortmvpm(const char *infile, const char *outfile)
{
  VB_Vector mvpmVec(infile);
  int vecLen = mvpmVec.getLength();
  FILE *fp = fopen(outfile,"w");
  printf("Writing %s ... ", outfile);  
  fflush(stdout);
  if (fp) {
    fprintf(fp, "; This file is converted from %s by sortmvpm\n", infile);
    double tmpVal;
    for (int i = 0; i < vecLen; i++) {
      tmpVal = mvpmVec[i];
      if ((i+1) % 7)
	fprintf(fp, "%.9f\t", tmpVal);
      else
	fprintf(fp, "%.9f\n", tmpVal);
    }

    fclose(fp);
    printf("done.\n");
  }
  else {
    printf("failed.\n");
  }
}
