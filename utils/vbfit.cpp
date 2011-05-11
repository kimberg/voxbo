
// vbfit.cpp
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

#include "fitOneOverF.h"

void printHelp();

/* functions used by vbfit main function */
double readTR(const char *);
double readTotalReps(const char *);
void checkInFile(const char *);
void checkLength(double vecLen, double totalReps, const char *filename);
void checkLength2(size_t newLen, size_t len1, const char *newStr, const char *str1);
 
/* Functions for interactive mode, commented out now because it is not being used */
// void interactive();
// string readFitFile();
// double readInputVal(double defaultVal);
// string getInputString();
// string readOutputFile(const char *outputDefault);

int main(int argc, char *argv[])
{
  // At least 3 arguments are needed: TR, totalReps, ps filename
  if (argc < 4) {
    printHelp();
    exit(1);
  }

  // Make sure TR and totalReps are valid
  double TRin = readTR(argv[1]);
  double totalReps = readTotalReps(argv[2]);
  double var3min = (-1.0) / (totalReps * TRin / 1000);

  // "-r" plus "-o" option: when both reference function and output filename are given
  if (argc >= 8 && dancmp(argv[3], "-r") && dancmp(argv[argc - 2], "-o")) {
    int refIndex = 4;
    for (int m = refIndex; m < argc - 2; m++)
      checkInFile(argv[m]);

    double sigma = 0.1;
    double var1 = 20.0, var2 = 2.0, var3 = 0;
    const char *refFunc = argv[refIndex];
      
    VB_Vector *psVec = new VB_Vector(argv[refIndex + 1]);
    size_t vecLength = psVec->getLength();
    checkLength(vecLength, totalReps, argv[refIndex + 1]);
    int counter = 1;
    for (int i = refIndex + 2; i < argc - 2; i++) {
      VB_Vector *tmpVec = new VB_Vector(argv[i]);
      size_t tmpLength = tmpVec->getLength();
      checkLength2(tmpLength, vecLength, argv[i], argv[refIndex + 1]);
      (*psVec)+=tmpVec;
      counter++;
    }
    psVec->scaleInPlace(1.0 / (double)counter);

    const char *outputFile = argv[argc - 1];
    fitOneOverF(psVec, refFunc, TRin, sigma, var3min, var1, var2, var3, outputFile);
  }
  
  // only "-r" option: when reference function is defined in arguments
  else if (argc >= 5 && dancmp(argv[2], "-r") && !dancmp(argv[argc - 2], "-o")) {
    int refIndex = 4;
    for (int m = refIndex; m < argc; m++)
      checkInFile(argv[m]);

    const char *refFunc = argv[refIndex];
    VB_Vector *psVec = new VB_Vector(argv[refIndex + 1]);
    size_t vecLength = psVec->getLength();
    checkLength(vecLength, totalReps, argv[refIndex + 1]);
    int counter = 1;
    for (int i = refIndex + 2; i < argc; i++) {
      VB_Vector *tmpVec = new VB_Vector(argv[i]);
      size_t tmpLength = tmpVec->getLength();
      checkLength2(tmpLength, vecLength, argv[i], argv[refIndex + 1]);
      (*psVec)+=tmpVec;
      counter++;
    }
    psVec->scaleInPlace(1.0 / (double) counter);
    fitOneOverF(psVec, refFunc, var3min, TRin);
  }

  // only "-o" option: when output file is defined in arguments
  else if (argc >= 5 && dancmp(argv[argc - 2], "-o")) {
    int psIndex = 3;
    for (int m = psIndex; m < argc - 2; m++)
      checkInFile(argv[m]);

    VB_Vector *psVec = new VB_Vector(argv[psIndex]);
    size_t vecLength = psVec->getLength();
    checkLength(vecLength, totalReps, argv[psIndex]);
    int counter = 1;
    for (int i = psIndex + 1; i < argc - 2; i++) {
      VB_Vector *tmpVec = new VB_Vector(argv[i]);
      size_t tmpLength = tmpVec->getLength();
      checkLength2(tmpLength, vecLength, argv[i], argv[psIndex]);
      (*psVec)+=tmpVec;
      counter++;
    }
    psVec->scaleInPlace(1.0 / (double)counter);

    double sigma = 0.1;
    double var1 = 20.0, var2 = 2.0, var3 = -0.0001;
    const char *outputFile = argv[argc - 1];
    fitOneOverF(psVec, var3min, TRin, sigma, var1, var2, var3, outputFile);
  }
  
  /* generic case: when no "-i", "-r" or "-o" found, always treat second argument as TR, 
   * others after as power spectrum filename */
  else if (argc >= 3 && !dancmp(argv[2], "-r") && !dancmp(argv[argc - 2], "-o")) {
    int psIndex = 3;
    for (int m = psIndex; m < argc; m++)
      checkInFile(argv[m]);

    VB_Vector *psVec = new VB_Vector(argv[psIndex]);
    size_t vecLength = psVec->getLength();
    checkLength(vecLength, totalReps, argv[psIndex]);
    int counter = 1;
    for (int i = psIndex + 1; i < argc; i++) {
      VB_Vector *tmpVec = new VB_Vector(argv[i]);
      size_t tmpLength = tmpVec->getLength();
      checkLength2(tmpLength, vecLength, argv[i], argv[psIndex]);
      (*psVec)+=tmpVec;
      counter++;
    }
    psVec->scaleInPlace(1.0 / (double)counter);
    fitOneOverF(psVec, var3min, TRin);
  }

  // otherwise print out help on the screen
  else 
    printHelp();

  return 0;
}

/* Function to interpret TR in the argument */
double readTR(const char *inputString)
{
  int TRin = atoi(inputString);
  if (TRin <= 0) {
    printf("Invalid TR value: %s\nAborted ... \n", inputString);
    exit(1);
  }
  return (double)TRin;
}

/* Function to interpret totalReps in the argument */
double readTotalReps(const char *inputString)
{
  int totalReps = atoi(inputString);
  if (totalReps <= 0) {
    printf("Invalid totalReps value: %s\nAborted ... \n", inputString);
    exit(1);
  }
  return (double)totalReps;
}

/* This function checks whether totalReps is multiple of inout vector length or not. */
void checkLength(double vecLength, double totalReps, const char *filename)
{
  if ((int) totalReps % (int) vecLength) {
    printf("totalReps (%d) is not a multiple of number of elements in PS file (%d):\n %s\n", 
	   (int) totalReps, (int) vecLength, filename);
    printf("Aborted ... \n");
    exit(1);
  }
}

/* This function compares the first argument (newLen) with the second argument (len1).
 * If they are not equal, , print out error message and terminate the whole process.
 * The 3rd argument is filename whose # of element is newLen. 
 * The last argument is the first filename whose # of element is len1.   */
void checkLength2(size_t newLen, size_t len1, const char *newStr, const char *str1)
{
  if (newLen != len1) {
    printf("Number of elements in %s doesn't match that in %s.\n", newStr, str1);
    printf("Aborted ... \n");
    exit(1);
  }
}

/* Function to check input filename in the arguments */
void checkInFile(const char *inputString)
{
  if (access(inputString,R_OK)) {
    printf("%s: not a readable file\nAborted ...\n", inputString);
    exit(1);
  }
}

/* Function to print out vbfit usage */
void printHelp()
{
  printf("VoxBo vbfit\n");
  printf("usage:\n");
  printf("  vbfit <TR> <totalReps> [-r <refFunc>] <ps#1> [<ps#2> ...] [-o <output>]\n\n");

  printf("        <TR>:  time of repetition (unit: ms)\n");
  printf(" <totalReps>:  total # of image based on which power spectrum will be drawn\n");
  printf("   <refFunc>:  reference function file (optional)\n");
  printf("      <ps#1>:  power spectrum file #1 (ps#2, ps#3 ... are optional)\n");
  printf("    <output>:  filename where fitting parameters will be saved\n\
               (optional, default is no file generation)\n");
  printf("\n");
}

/* This function is commented out now because the interactive mode is not 
 * really helpful. Also note there is a bug in this function: 
 * it doesn't ask user var3min variable before fitting input arrays. */
// void interactive()
// {
//   printf("Enter interactive mode ...\n\n");

//   string xFilename;
//   string yFilename;
//   double sigmaDefault = 0.1, inputSigma;

//   double var1default = 20.0, var2default = 2.0, var3default = 0;
//   double var1_init, var2_init, var3_init;

//   const char *outputDefault = "fittingParams.ref";
//   string outputFile;

//   printf("Please type in the filename which includes values for x:\n");
//   xFilename = readFitFile();

//   printf("Please type in the filename which includes values for y:\n");
//   yFilename = readFitFile();

//   getchar();  // Don't know why this is required. weird.

//   printf("Please type in the sigma values for the points fitted (default is 0.1):\n");
//   inputSigma = readInputVal(sigmaDefault);

//   printf("Please type in the initial values of the three parameters.\n");
//   printf("Note that zero is NOT acceptable.\n");
//   printf("1st parameter (steepness, default is 20.0):\n");
//   var1_init = readInputVal(var1default);

//   printf("2nd parameter (y-offset, default is 2.0):\n");
//   var2_init = readInputVal(var2default);

//   printf("3rd parameter (x-shift, default is 0):\n");
//   var3_init = readInputVal(var3default);

//   printf("Please type in the output filename (default is fittingParams.ref):\n");
//   outputFile = readOutputFile(outputDefault);
//   curvefit(xFilename.data(), yFilename.data(), inputSigma, var3min, var1_init, 
// 	   var2_init, var3_init, outputFile.data());

// }

/* function used by vbfit interactive mode */
// string readFitFile()
// {
//   char inputFilename[100];
//   scanf("%s", inputFilename);

//   int fileStatus = getxxxInputStatus(inputFilename);

//   while (fileStatus != 3) {
//     if (fileStatus == 0)
//       printf("%s doesn't exist. ", inputFilename);
//     else  if (fileStatus == 1)
//       printf("%s isn't a regular file. ", inputFilename);
//     else 
//       printf("%s isn't a readable file. ", inputFilename);

//     printf("Please try another filename.\n");
//     scanf("%s", inputFilename);
//     fileStatus = getxxxInputStatus(inputFilename);
//   }
//   return (string)inputFilename;
// }

/* function used by vbfit interactive mode */
// double readInputVal(double defaultVal)
// {
//   double inputVal;
//   const char * inputString;
//   inputString = getInputString().data();

//   if (inputString[0] == '\0')
//     return defaultVal;
//   else {
//     inputVal = atof(inputString);
//     while (!inputVal) {
//       inputString = getInputString().data();
//       if (inputString[0] == '\0')
// 	return defaultVal;
//       else inputVal = atof(inputString);
//     }
//     return inputVal;
//   }
// }

/* function used by vbfit interactive mode */
// string readOutputFile(const char *outputDefault)
// {
//   const char * inputString;
//   inputString = getInputString().data();
  
//   if (inputString[0] == '\0')
//     inputString = outputDefault;

//   bool ovwFlag = false;
//   const char *ovwString;
//   int fileStatus = checkOutputFile(inputString, ovwFlag);
//   while (fileStatus < 3) {
//     if (fileStatus == 0)
//       printf("%s exists and its parent directory isn't writeable.\n", inputString);
//     else if (fileStatus == 1) {
//       printf("%s exists. Do you want to overwrite this file? <y/n> ", inputString);
//       ovwString = getInputString().data();

//       while ((ovwString[0] != 'y') && (ovwString[0] != 'n')) {
// 	printf("Invalid input. Please type y or n: ");
// 	ovwString = getInputString().data();
//       }
//       if (ovwString[0] == 'y') {
// 	ovwFlag = true;
// 	return (string)inputString;
//       }
//       else
// 	ovwFlag = false;
//     }
//     else if (fileStatus == 2)
//       printf("%s parent directory isn't writeable.\n", inputString);

//     printf("Please type in another filename (default is fittingParams.ref):\n");
//     inputString = getInputString().data();
//     if (inputString[0] == '\0')
//       inputString = outputDefault;

//     fileStatus = checkOutputFile(inputString, ovwFlag);
//     ovwFlag = false;
//   }

//   return (string)inputString;
// }

/* function used by vbfit interactive mode */
// string getInputString()
// {
//   int m;
//   char c, tmpString[100];
//   string myString;
//   for (m = 0; (c = getchar()) != '\n'; m++) {
//     tmpString[m] = c;
//   }
//   tmpString[m] = '\0';

//   myString = tmpString;
//   return myString;
// }
