
// vbthresh.cpp
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
// original version written by Kosh Banerjee, modified by Dan Kimberg

using namespace std;

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <climits>
#include <ctime>
#include "vbprefs.h"
#include "vbutil.h"
#include "vbio.h"
//#include "utils.h"
//#include "koshutil.h"
#include "vbthresh.hlp.h"

unsigned int determineCutOff(const int binSize, Tes& tesFile);
void processData(Tes& theTes, const int cutOff);
void vbthresh_help();
void vbthresh_version();

int
main(int argc, char *argv[])
{
  /*********************************************************************
  * VARIABLES:   TYPE:           DESCRIPTION:                          *
  * ----------   -----           ------------                          *
  * cutOff       int             Holds the threshold cut off value.    *
  * tesFile      string          Holds the input 4D data file name.    *
  * outFile      string          Holds the output file name.           *
  * binSize      int             Holds the bin size (if the histogram  *
  *                              thresh holding is used).              *
  *********************************************************************/
  int cutOff = 0;
  int binSize = 0;
  string tesFile;
  string outFile;
  if (argc == 1) {
    vbthresh_help();
    exit(0);
  }

  /*********************************************************************
  * Now processing the command line options.                           *
  * -h ==> Display usage information.                                  *
  * -i ==> Specifies the input 4D data file.                           *
  * -o ==> Specifies the output file name.                             *
  * -c ==> Specifies the threshold cut off value.                      *
  * -b ==> Specifies the bin size for histogram thresh holding.        *
  * -v ==> Print global VoxBo version.                                 *
  *                                                                    *
  * Now processing the command line options. printHelp is a flag       *
  * variable, used to determine if the "-h" command line option was    *
  * used or not. printVersion is a flag variable, used to determine if *
  * the user wants the global VoxBo version printed or not.            *
  *********************************************************************/
  arghandler a;
  a.setArgs("-h", "--help", 0);
  a.setArgs("-i", "--inputfile", 1);
  a.setArgs("-o", "--outfile", 1);
  a.setArgs("-c", "--threshold", 1);
  a.setArgs("-b", "--binsize", 1);
  a.setArgs("-v", "--version", 0);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
     errstring = "[E] unknown flag: " + errstring;
     printErrorMsg(VB_ERROR, errstring.c_str());
     exit(-1);
  }
  if (a.flagPresent("-h")) {
    vbthresh_help();
    exit(0);
  }
  tesFile = a.getFlaggedArgs("-i")[0];
  outFile = a.getFlaggedArgs("-o")[0];
  cutOff = atoi(a.getFlaggedArgs("-c")[0].c_str());
  binSize = atoi(a.getFlaggedArgs("-b")[0].c_str());
  if (a.flagPresent("-v")) {
    vbthresh_version();
    exit(0);
  }
  if (binSize < 0) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] binSize: [" << binSize << "] must be > 0.";
    printErrorMsg(VB_ERROR, errorMsg.str());
  } 
  if (tesFile.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Must specify the input 4D data file name.";
    printErrorMsg(VB_ERROR, errorMsg.str());
  } 
  if (outFile.size() == 0)
    outFile = tesFile;
  /*********************************************************************
  * If the threshold cut off value was not specified at the command    *
  * line and binSize == 0, then cutOff is assigned the default value   *
  * of 600. NOTE: If binSize is 0, then it is assumed that "-b" was    *
  * not used as a command line option for this program.                *
  *********************************************************************/
  if ( (cutOff == 0) && (binSize == 0) )
    cutOff = 600;
  Tes theTes(tesFile);
  if (!theTes.data_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Unable to read VoxBo 4D file [" << tesFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  } 
  if (binSize > 0)
    cutOff = determineCutOff(binSize, theTes);
  processData(theTes, cutOff);
  string timeStr=timedate();
  ostringstream headerLine;
  headerLine << "Thresh_Abs:" << "\t" << timeStr << "\t" << cutOff;
  theTes.AddHeader(headerLine.str());
  theTes.SetFileName(outFile);
  if (theTes.WriteFile(outFile)) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Could not write to output file [" << theTes.filename << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 6);
  } 
  return 0;
} 

// if the minimum signal value is less than the threshold value for a
// particular voxel, that voxel is zeroed out for the whole time
// series

void
processData(Tes& theTes, const int cutOff)
{
  for (int i=0; i<theTes.dimx; i++) {
    for (int j=0; j<theTes.dimy; j++) {
      for (int k=0; k<theTes.dimz; k++) {
        int voxelpos=theTes.voxelposition(i,j,k);
        theTes.GetTimeSeries(i,j,k);
        if (!(theTes.mask[voxelpos]))
          continue;
        if (theTes.timeseries.getMinElement() < (double) cutOff)
          theTes.zerovoxel(voxelpos);
      }
    }
  }
} 

/*********************************************************************
* This function determines the thresh hold cut off value for the     *
* histogram method. The following steps are implemented to compute   *
* the value:                                                         *
*                                                                    *
* 1. The minimum and maximum signal values are found, say a and b,   *
*    respectively.                                                   *
* 2. An unsigned integer array of length [(b - a) / binSize] is      *
*    allocated, each element intialized to 0.                        *
* 3. Each signal value is examined again to increment the appropriate*
*    element of the array allocated in the preceding step.           *
* 4. The cut off value will be the first local minimum in the array  *
*    allocated in step (2).                                          *
*                                                                    *
* INPUT VARIABLES:  TYPE:                DESCRIPTION:                *
* ----------------  -----                ------------                *
* binSize           const unsigned int   The length of each interval *
*                                        for the histogram.          *
*                                                                    *
* theTes            Tes&                 The 4D data object.         *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* thresh hold cut     unsigned int    The calculated thresh hold     *
* off value                           cut off value.                 *
********************************************************************/
unsigned int
determineCutOff(const int binSize, Tes& tesFile)
{
  Tes series(tesFile);
  if (!series.data_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Unable to read VoxBo 4D file [" << tesFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  } 
  long signalMin = LONG_MAX;
  long signalMax = LONG_MIN;
  unsigned int dimT = series.dimt;

  for (int i=0; i<series.dimx; i++) {
    for (int j=0; j<series.dimy; j++) {
      for (int k=0; k<series.dimz; k++) {
        series.GetTimeSeries(i,j,k);
        signalMin = min(signalMin, (long ) series.timeseries.getMinElement());
        signalMax = max(signalMax, (long ) series.timeseries.getMaxElement());
      }
    }
  }

  /*********************************************************************
  * arrLength is set to (int ) ((signalMax - signalMin) / binSize).    *
  * arrLength is equal to the number of needed "bins" for the          *
  * histogram.                                                         *
  *                                                                    *
  * histogram[] will keep track of the number of values in each "bin." *
  * It is intialized to all zeros.                                     *
  *********************************************************************/
  unsigned int arrLength = (int ) ((signalMax - signalMin) / binSize);
  unsigned long histogram[arrLength];
  memset(histogram, 0, sizeof(long) * arrLength);

  for (int ii=0; ii<series.dimx; ii++) {
    for (int jj=0; jj<series.dimy; jj++) {
      for (int kk=0; kk<series.dimz; kk++) {
        series.GetTimeSeries(ii,jj,kk);
        /*********************************************************************
         * Assume:                                                            *
         *                                                                    *
         * binSize = 5                                                        *
         * signalMin = 10                                                     *
         * signalMin = 60                                                     *
         * The above values imply that arrLength = 10.                        *
         * dimT = 21                                                          *
         * tSeries = [ 10 15 20 25 30 35 40 45 50 55 60 11 16 21 26 31 36     *
         *             41 46 51 56 ]                                          *
         *                                                                    *
         * Then histogram[] must end up as:                                   *
         *                                                                    *
         * histogram = [ 2 2 2 2 2 2 2 2 2 3 ]                                *
         *                                                                    *
         * where                                                              *
         * histogram[0] = number of elements of tSeries[] in [10, 15).        *
         * histogram[1] = number of elements of tSeries[] in [15, 20).        *
         * histogram[2] = number of elements of tSeries[] in [20, 25).        *
         * histogram[3] = number of elements of tSeries[] in [25, 30).        *
         * histogram[4] = number of elements of tSeries[] in [30, 35).        *
         * histogram[5] = number of elements of tSeries[] in [35, 40).        *
         * histogram[6] = number of elements of tSeries[] in [40, 45).        *
         * histogram[7] = number of elements of tSeries[] in [45, 50).        *
         * histogram[8] = number of elements of tSeries[] in [50, 55).        *
         * histogram[9] = number of elements of tSeries[] in [55, 60].        *
         *                                                                    *
         * NOTE: the sum of the elements in histogram[] must equal the length *
         * of, i.e., the number of elements in, tSeries[].                    *
         *                                                                    *
         * The following for loop is used to traverse histogram[].            *
         *********************************************************************/
        for (unsigned int i = 0; i < arrLength; i++) {
          /*********************************************************************
           * The following for loop traverses tSeries[].                        *
           *********************************************************************/
          for (unsigned int j = 0; j < dimT; j++) { 
            /*********************************************************************
             * This if is sued to determine which "bin", i.e., which element of   *
             * histogram[], gets incremented. The lower value of the bin is given *
             * (signalMin + (i * binSize)) and the upper value is given by        *
             * (signalMin + ((i+1) * binSize)). NOTE: The lower value is          *
             * inclusive and the upper value is exclusive.                        *
             *********************************************************************/
            if ( ((signalMin + (i * binSize)) <= series.timeseries[j]) && (series.timeseries[j] < (signalMin + ((i+1) * binSize))) )
              histogram[i]++;
            /*********************************************************************
             * This "else if" is used to see if tSeries[j] equals signalMax. If   *
             * so, then the last element of histogram[] is incremented. We only   *
             * want to do this one time, which implies we must do it for a single *
             * value of i. We can use any value of i in [0, arrLength) so zero is *
             * chosen, somewhat arbitrarily.                                      *
             *********************************************************************/
            else if ( (series.timeseries[j] == signalMax) && (i == 0) )
              histogram[arrLength - 1]++;
          } 
        } 
      }
    }
  }

  /*********************************************************************
  * The array histogram[] represents a histogram.                      *
  * To get the cut off value, we want the first local minimum that is  *
  * greater than zero (point b in the above illustration).             *
  *********************************************************************/
  int localMin = 0;
  for (unsigned int i = 1; i < arrLength; i++) {
    /*********************************************************************
    * If histogram[i] > histogram[localMin], then histogram[i] has just  *
    * started to increase. This means that the current value of localMin *
    * is an index of a local minimum in histogram[]. We must also check  *
    * to see if this local minimum is greater than zero.                 *
    *********************************************************************/
    if ( (histogram[i] > histogram[localMin]) && ((signalMin + (localMin * binSize)) > 0) )
      break;
    /*********************************************************************
    * If program flow ends up here, then histogram[i] is still decreasing*
    * and so we have not yet found the index of the first local minimum. *
    * Therefore, localMin is set to i.                                   *
    *********************************************************************/
    else
      localMin = i;
  } 

  /*********************************************************************
  * Now returning the lower boundary value of the "bin" in histogram[] *
  * corresponding to localMin.                                         *
  *********************************************************************/
  return (signalMin + (localMin * binSize));
} 


void
vbthresh_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbthresh_version()
{
  printf("VoxBo vbthresh (v%s)\n",vbversion.c_str());
}

