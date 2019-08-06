
// sliceacq.cpp
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
// original version written by Kosh Banerjee
// modified slightly by Dan Kimberg, to fix little issues

/*********************************************************************
 * This program carries out Slice Acquisition Correction. To carry    *
 * out this task, 2 quantities will have to be either specified by    *
 * the user or have to be determined by this program. These two       *
 * quantities are:                                                    *
 *                                                                    *
 *   sliceTime                                                        *
 *   firstSliceTime                                                   *
 *                                                                    *
 * The hierarchy to determine the above two quantities is:            *
 *   1) The user specifies them on the command line.                  *
 *   2) This program reads the Slice Acquisition configuration file.  *
 *   3) firstSliceTime is set to 0 and sliceTime to TR/dimZ.          *
 *********************************************************************/
#include "sliceacq.h"

/*********************************************************************
 * Need a global VBPrefs object named vbp. vbp is used to determine   *
 * the VoxBo root directory.                                          *
 *********************************************************************/
VBPrefs vbp;

void usage(const unsigned short exitValue, char* progName);

int main(int argc, char* argv[]) {
  tesHeaderInfo reqHeaderInfo;
  reqHeaderInfo.tr = 0;
  reqHeaderInfo.pulseSeq = "";
  reqHeaderInfo.fieldStrength = 0.0;
  string paramFilePath;
  string tesFile;
  string outFile;

  /*********************************************************************
   * VARIABLES:                                                         *
   * redo - flag used to determine if the input 4D data file is to be   *
   *        corrected, even though is has already been corrected. The   *
   *        default is to not recorrect a 4D data file.                 *
   * alreadyCorrected - will be set to true if the input 4D data file   *
   *                    has already been corrected.                     *
   * dimT - the maximum value of the time dimension in the 4D data file.*
   * dimX - the maximum value of the X dimension in the 4D data file.   *
   * dimY - the maximum value of the Y dimension in the 4D data file.   *
   * dimZ - the maximum value of the Z dimension in the 4D data file.   *
   * sliceTime - used to hold the user specified value of slice time.   *
   * firstSliceTime - time when the first slice was taken in ms.        *
   *********************************************************************/
  bool redo = false;
  bool alreadyCorrected = false;
  unsigned int dimT = 0;
  unsigned int dimX = 0;
  unsigned int dimY = 0;
  unsigned int dimZ = 0;
  double sliceTime = 0.0;
  double firstSliceTime = -1.0;

  /*********************************************************************
   * If no command line options have been used, then usage information  *
   * is displayed and the program exits.                                *
   *********************************************************************/
  if (argc == 1) {
    usage(1, argv[0]);
  }  // if

  /*********************************************************************
   * Now processing the command line options.                           *
   * -h ==> Display usage information.                                  *
   * -i ==> Specifies the input 4D data file.                           *
   * -o ==> Specifies the output 4D data file name.                     *
   * -r ==> Override the default of not correcting a file if it has     *
   *        already been corrected.                                     *
   * -s ==> Used to specify the slice time.                             *
   * -t ==> Time when the first slice was taken in milliseconds.        *
   * -c ==> User sepcified TR, in milliseconds.                         *
   * -p ==> Specify the configuration file.                             *
   * -v ==> Print out the gobal VoxBo version number.                   *
   *                                                                    *
   * Now processing the command line options. printHelp is a flag       *
   * variable, used to determine if the "-h" command line option was    *
   * used or not. printVersion is a flag variable, used to determine if *
   * the "-v" command line option was used or not.                      *
   *********************************************************************/
  bool printHelp = false;
  bool printVersion = false;
  bool interleaved = false;  // DYK
  int userTR = -1;
  // DYK added interleaved
  processOpts(argc, argv, ":hi:o:rs:t:c:vp:n", "bZZbddibZb", &printHelp,
              &tesFile, &outFile, &redo, &sliceTime, &firstSliceTime, &userTR,
              &printVersion, &paramFilePath, &interleaved);

  /*********************************************************************
   * Now checking to see if usage information needs to be printed.      *
   *********************************************************************/
  if (printHelp) {
    usage(0, argv[0]);
  }  // if

  /*********************************************************************
   * Now checking to see if the global VoxBo version needs to be        *
   * printed.                                                           *
   *********************************************************************/
  if (printVersion) {
    printf("\nVoxBo v%s\n", vbversion.c_str());
  }  // if

  /*********************************************************************
   * If just sliceTime was specified at the command line (and not       *
   * firstSliceTime), then firstSliceTime is assumed to be zero.        *
   *********************************************************************/
  if ((sliceTime > 0.0) && (firstSliceTime == -1.0)) {
    firstSliceTime = 0.0;
  }  // if

  /*********************************************************************
   * If the input 4D data file has not been specified, then an error    *
   * message is sent to stderr, and then usage information is           *
   * displayed.                                                         *
   *********************************************************************/
  if (tesFile.size() == 0) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Must specify the input 4D data file name.";
    printErrorMsg(VB_ERROR, errorMsg.str());
    usage(1, argv[0]);
  }  // if

  /*********************************************************************
   * If an output file name was not specified at the command line, then *
   * the default value of the input 4D data file is assigned.           *
   *********************************************************************/
  if (outFile.size() == 0) {
    outFile = tesFile;
  }  // if

  /*********************************************************************
   * instantiating a Tes object from tesFile.                           *
   *********************************************************************/
  Tes theTes(tesFile);

  /*********************************************************************
   * If theTes has invalid data, then an error message is printed and   *
   * then this program exits.                                           *
   *********************************************************************/
  if (!theTes.data_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Invalid data for Tes file ["
             << tesFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * If theTes has an invalid header, then an error message is printed  *
   * and then this program exits.                                       *
   *********************************************************************/
  if (!theTes.header_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] Invalid header for Tes file ["
             << tesFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  // DYK grab TR info
  reqHeaderInfo.tr = theTes.voxsize[3];

  /*********************************************************************
   * The following for loop is used to traverse the string objects in   *
   * the header data member of the Tes class to see if the input 4D data*
   * file has already been corrected or not, and to get the values of   *
   * TR, PulseSeq, and FieldStrength.                                   *
   *********************************************************************/
  for (int i = 0; i < (int)theTes.header.size(); i++) {
    /*********************************************************************
     * tempStr is assigned theTes.header[i].                              *
     *********************************************************************/
    string tempStr = theTes.header[i];

    /*********************************************************************
     * If tempStr is non-empty, then we can get tokens from it, using a   *
     * space and a tab as field delimiters.                               *
     *********************************************************************/
    if (tempStr.length() > 0) {
      tokenlist strToken(tempStr);

      /*********************************************************************
       * Now if "AcqCorrect:" is in tempStr, then the input 4D data file    *
       * has already been corrected.                                        *
       *********************************************************************/
      if (strToken[0] == "AcqCorrect:") {
        /*********************************************************************
         * alreadyCorrected is set to true since the input 4D data file has   *
         * already been corrected.                                            *
         *********************************************************************/
        alreadyCorrected = true;

      }  // if

      /*********************************************************************
       * If the current header line holds the TR value, then it is assigned *
       * to reqHeaderInfo.tr. NOTE: The TR value will be the second token   *
       * (token number 1) in the header line.                               *
       *********************************************************************/
      // DYK: TR now in voxsize[3]
      //       else if (strToken.getToken(0) == "TR(msecs):")
      //       {
      //         reqHeaderInfo.tr = atoi(strToken.getToken(1).c_str());
      //       } // else if

      /*********************************************************************
       * If the current header line holds the PulseSeq value, then it is    *
       * assigned to reqHeaderInfo.pulseSeq. NOTE: The PulseSeq value will  *
       * be the second token (token number 1) in the header line.           *
       *********************************************************************/
      else if (strToken[0] == "PulseSeq:") {
        reqHeaderInfo.pulseSeq = strToken[1];
      }  // else if

      /*********************************************************************
       * If the current header line holds the FieldStrength value, then it  *
       * is assigned to reqHeaderInfo.fieldStrength. NOTE: The Field        *
       * Strength value will be the second token (token number 1) in the    *
       * header line.                                                       *
       *********************************************************************/
      else if (strToken[0] == "FieldStrength:")
        reqHeaderInfo.fieldStrength = strtod(strToken[1]);

    }  // if

  }  // for i

  /*********************************************************************
   * If the user supplied a TR value at the command line, then          *
   * reqHeaderInfo.tr is set to that value.                             *
   *********************************************************************/
  if (userTR > 0) {
    reqHeaderInfo.tr = userTR;
  }  // if

  /*********************************************************************
   * If just firstSliceTime was specified at the command line (and not  *
   * sliceTime), then sliceTIme is set to (tr - fileSliceTIme)/dimZ.    *
   *********************************************************************/
  if ((sliceTime <= 0.0) && (firstSliceTime >= 0.0)) {
    sliceTime = (reqHeaderInfo.tr - firstSliceTime) / dimZ;
  }  // if

  /*********************************************************************
   * If the input 4D data file has already been corrected and the redo  *
   * flag was not used, then an error message in printed out and this   *
   * program exits.                                                     *
   *********************************************************************/
  if ((alreadyCorrected == true) && (redo == false)) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] The 4D data file [" << tesFile
             << "] has already been corrected.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * Now assigning dimT, dimX, dimY, and dimZ.                          *
   *********************************************************************/
  dimT = theTes.dimt;
  dimX = theTes.dimx;
  dimY = theTes.dimy;
  dimZ = theTes.dimz;

  /*********************************************************************
   * If the user did not specify sliceTime nor firstSliceTime on the    *
   * command line, then we need to determine these quantities.          *
   *********************************************************************/
  if ((sliceTime <= 0.0) && (firstSliceTime == -1.0)) {
    /*********************************************************************
     * Now checking to see if the user specified a parameter file. If the *
     * user did not specify a parameter file (at the command line), then  *
     * the default value is used.                                         *
     *********************************************************************/
    if (paramFilePath.size() == 0) {
      vbp.init();
      paramFilePath = vbp.rootdir + "etc/sliceacq.cfg";
    }  // if

    /*********************************************************************
     * Now getting the slice time and time of first slice from the Slice  *
     * Acquisition parameters file.                                       *
     *********************************************************************/
    int pauseFlag = 0;
    readParamFile(paramFilePath, reqHeaderInfo.pulseSeq,
                  reqHeaderInfo.fieldStrength, &sliceTime, &pauseFlag,
                  reqHeaderInfo.tr, dimZ);

    /*********************************************************************
     * If sliceTime > 0, then it was successfully determined from the     *
     * Slice Acquisition configuration file. In that case, we check       *
     * pauseFlag to see how to set firstSliceTime.                        *
     *********************************************************************/
    if (sliceTime > 0.0) {
      /*********************************************************************
       * If pauseFlag is zero, then there was no pause. This implies that   *
       * firstSliceTime is zero.                                            *
       *********************************************************************/
      if (!pauseFlag) {
        firstSliceTime = 0.0;
      }  // if

      /*********************************************************************
       * If pauseFlag is one, then there was a pause. firstSliceTime is set *
       * accordingly.                                                       *
       *********************************************************************/
      else {
        firstSliceTime = (double)reqHeaderInfo.tr - ((double)dimZ * sliceTime);
      }  // else

    }  // if

    /*********************************************************************
     * If sliceTime is still zero after the call to readParamFile(), then *
     * we were unable to look it up from the configuration file.          *
     * it is assumed that firstSliceTime is 0 (no pause) and sliceTime is *
     * set to TR/dimZ.                                                    *
     *********************************************************************/
    else {
      firstSliceTime = 0.0;
      sliceTime = (double)reqHeaderInfo.tr / (double)dimZ;
    }  // else

  }  // if

  /*********************************************************************
   * The the correction parameter values are invalid, then an error     *
   * message is printed and then this program exits.                    *
   *********************************************************************/
  if (!sanityCheckOfValues(sliceTime, firstSliceTime, reqHeaderInfo.tr, dimZ)) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Correction parameter values are not valid: TR = ["
             << reqHeaderInfo.tr << "] dimZ = [" << dimZ << "] sliceTime = ["
             << sliceTime << "] firstSliceTime = [" << firstSliceTime << "].";
    // DYK
    if (interleaved)
      errorMsg << "  Order is interleaved.";
    else
      errorMsg << "  Order is ascending.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * If the correction parameter values are valid, they are simply      *
   * printed out.                                                       *
   *********************************************************************/
  else {
    ostringstream errorMsg;
    errorMsg << "Correction parameter values are: TR = [" << reqHeaderInfo.tr
             << "] dimZ = [" << dimZ << "] sliceTime = [" << sliceTime
             << "] firstSliceTime = [" << firstSliceTime << "].";
    // DYK
    if (interleaved)
      errorMsg << "  Order is interleaved.";
    else
      errorMsg << "  Order is ascending.";
    printErrorMsg(VB_INFO, errorMsg.str());
  }  // else

  /*********************************************************************
   * Now declaring two VB_Vector objects.                               *
   *********************************************************************/
  VB_Vector timeAcquired(dimZ);
  VB_Vector shiftAmount(dimZ);

  /*********************************************************************
   * The following for loop is used to assign values to timeAcquired[]  *
   * and shiftAmount[].                                                 *
   *********************************************************************/
  // DYK: added the following to handle interleaved or not
  for (unsigned int i = 0; i < dimZ; i++) {
    if (interleaved)
      timeAcquired[i] = (double)interleavedorder(i, dimZ);
    else
      timeAcquired[i] = (double)i;
  }
  for (unsigned int i = 0; i < dimZ; i++) {
    // DYK commented out in favor of the above: timeAcquired[i] = (double ) i;
    shiftAmount[i] = ((timeAcquired[i] * sliceTime) + firstSliceTime) /
                     (double)reqHeaderInfo.tr;
  }  // for i

  /*********************************************************************
   * totalSpatialVoxels will hold the number of spatial voxels.         *
   *********************************************************************/
  unsigned long totalSpatialVoxels = theTes.voxels;

  /*********************************************************************
   * tempX, tempY, and tempZ will hold spatial coordinates.             *
   *********************************************************************/
  int32 tempX, tempY, tempZ;

  /*********************************************************************
   * The following for loop isued to processes the non-zero time series.*
   *********************************************************************/
  for (unsigned int i = 0; i < totalSpatialVoxels; i++) {
    /*********************************************************************
     * Converting the index i (which should be viewed as an index of      *
     * Tes::data[]) into its coresponding spatial coordinates.            *
     *********************************************************************/
    theTes.getXYZ(tempX, tempY, tempZ, i);

    /*********************************************************************
     * If the mask value at (tempX, tempY, tempZ) is not zero, then we    *
     * need to process the time series.                                   *
     *********************************************************************/
    if (theTes.GetMaskValue(tempX, tempY, tempZ) != (int)0) {
      /*********************************************************************
       * shiftedSeries will hold the phase shifted time series.             *
       *********************************************************************/
      VB_Vector shiftedSeries(dimT);

      theTes.GetTimeSeries(tempX, tempY, tempZ);

      /*********************************************************************
       * Phase shifting the time series.                                    *
       *********************************************************************/
      phaseShift(theTes.timeseries, shiftAmount[tempZ], shiftedSeries);

      /*********************************************************************
       * Setting the new values.                                            *
       *********************************************************************/
      for (unsigned int j = 0; j < dimT; j++) {
        theTes.SetValue(tempX, tempY, tempZ, j, shiftedSeries[j]);
      }  // for j

    }  // if

  }  // for i

  /*********************************************************************
   * Now building up the header line that is to be added.               *
   *********************************************************************/
  string headerLine("AcqCorrect:\t");
  headerLine += timedate() + "\t";
  ostringstream tempo;
  tempo << sliceTime << "\t" << firstSliceTime;
  headerLine += tempo.str();

  /*********************************************************************
   * Now adding the new header line to the existing header.             *
   *********************************************************************/
  theTes.AddHeader(headerLine);

  /*********************************************************************
   * Setting the output file name to outFile.                           *
   *********************************************************************/
  theTes.SetFileName(outFile);

  /*********************************************************************
   * Now writing out the processed data to outFile. If failure is       *
   * encountered, then an error message is written and this program     *
   * exits. NOTE: Tes::WriteFile() returns 0 upon success.              *
   *********************************************************************/
  if (theTes.WriteFile()) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Could not write to output file [" << outFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 4);
  }  // if

  /*********************************************************************
   * Returning 0.                                                       *
   *********************************************************************/
  return 0;

}  // main

/*********************************************************************
 * This function calls genusage() to print out the usage information. *
 *                                                                    *
 * INPUTS:                                                            *
 * -------                                                            *
 * exitValue: The desired exit value.                                 *
 * progName: String holding the program name.                         *
 *                                                                    *
 * OUTPUTS:                                                           *
 * --------                                                           *
 * None.                                                              *
 *********************************************************************/
void usage(const unsigned short exitValue, char* progName) {
  vbp.init();
  string tempStr("                          " + vbp.rootdir +
                 "etc/sliceacq.cfg");

  genusage(
      exitValue, progName, "- Acquisition correction routine for VoxBo.",
      "-h -r -i[4D data file name] -o[output file name] \n                "
      "-s[slice time] -t[time of 1st slice] -c[TR time]\n                "
      "-p[configuration file] -v",
      "-h                        Print usage information. Optional.",
      "-r                        When used, the default behavior of not "
      "performing",
      "                          correction on a corrected file is overridden.",
      "                          Optional.",
      "-i <4D data file>         Specify the input 4D data file. Required.",
      "-o <output file>          Specify the output file name. Optional.",
      "                          Default output file name is the input 4D data "
      "file",
      "                          name.",
      "-s <slice time in ms>     Specify the slice time in milliseconds.",
      "                          Values <= 0 are treated as if the slice time "
      "was not",
      "                          specified. Optional.",
      "-t <time of 1st slice>    Time of first slice in milliseconds. Must be "
      "a non-",
      "                          negative float. Optional.",
      "                          NOTE: If -s is used but not -t, then time of "
      "first",
      "                          slice is assumed to be 0. Conversely, if -t "
      "is used",
      "                          but not -s then slice time is set to",
      "                          (tr - time of 1st slice)/dimZ.",
      "-c <TR time in ms>        Specify the TR time as an integer number of",
      "                          milliseconds. This overrides the TR value "
      "from the",
      "                          header. Optional.",
      "                          NOTE: Non-positive values are ignored.",
      "-p                        Configuration file path. Default is",
      tempStr.c_str(),
      "-v                        Global VoxBo version number. Optional.",
      "-n                        Assume interleaved slice order (default is "
      "ascending).",
      "");

}  // void usage(const unsigned short exitValue, char *progName)

/*********************************************************************
 * This function reads the Slice Acquisition configuration file to    *
 * determine the appropriate slice time and time of first slice       *
 * values. If this function is unable to determine these 2 values, an *
 * error message is printed and this program exits.                   *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * paramFileName      const string&   The configuration file name.    *
 * seq                const string&   The sequence name.              *
 * fieldStrength      const double    The field strength value.       *
 * sliceTime          double *        Pointer to the slice time       *
 *                                    value, filled in by this        *
 *                                    function.                       *
 * pauseFlag          int *           Pointer to the pause flag.      *
 * tr                 const int       The TR value.                   *
 * dimZ               const int       The dimZ value.                 *
 *                                    filled in by this function.     *
 *                                                                    *
 * OUTPUT VARIABLES:                                                  *
 * -----------------                                                  *
 * None.                                                              *
 *********************************************************************/
void readParamFile(const string& paramFileName, const string& seq,
                   const double fieldStrength, double* sliceTime,
                   int* pauseFlag, const int tr, const int dimZ) {
  /*********************************************************************
   * sliceTime is initialized to -1.0 (a nonsensical value for slice    *
   * time) and pauseFlag are initialized to 0.                          *
   *********************************************************************/
  *sliceTime = -1.0;
  *pauseFlag = 0;

  /*********************************************************************
   * Now we must read the Slice Acquisition configuration file to figure*
   * out the slice time and to determine if there was a pause in the    *
   * sequence, given that we know the pulse sequence and the field      *
   * strength. Now opening the Slice Acquisition configuration file for *
   * reading only.                                                      *
   *********************************************************************/
  FILE* prefsReader = fopen(paramFileName.c_str(), "r");

  /*********************************************************************
   * Now checking to see if the configuration file was properly opened  *
   * for reading or not. If it was not, then an error message is        *
   * printed and then this program exits.                               *
   *********************************************************************/
  if (!prefsReader) {
    return;
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Unable to read the configuration file [" << paramFileName
             << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * fileLine[] will hold each line read in by prefsReader.             *
   *********************************************************************/
  char fileLine[ACQCORRECT_PREFS_LENGTH];
  memset(fileLine, 0, ACQCORRECT_PREFS_LENGTH);

  /*********************************************************************
   * The following while loop is used to read each line of the Slice    *
   * Acquisition configuration file.                                    *
   *********************************************************************/
  while (fgets(fileLine, ACQCORRECT_PREFS_LENGTH, prefsReader)) {
    /*********************************************************************
     * Ignore lines that begin with a '#' since they are comment lines    *
     * and ignore blank lines.                                            *
     *********************************************************************/
    if ((fileLine[0] != '#') && (strlen(fileLine) > 0)) {
      /*********************************************************************
       * Now the line just read in is used to instantiate a tokenlist       *
       * object, using space and tab as the field delimiters. NOTE: The     *
       * configuration file will have lines like:                           *
       *                                                                    *
       * SEQUENCE_TYPE FIELD_STRENGTH ST FLAG                               *
       *                                                                    *
       * where                                                              *
       *                                                                    *
       * SEQUENCE_TYPE = sequence type, e.g., fmriepi                       *
       * FIELD_STRENGTH = field strength, e.g., 1.5                         *
       * ST = slice time, e.g., 69.0. NOTE: A slice time of 0 indicates     *
       *      that slice time must be set to tr/dimZ.                       *
       * FLAG = 0 indicates no pause at start of sequence                   *
       *        1 indicates a pause at start of sequence                    *
       *********************************************************************/
      tokenlist S(fileLine, " \t");

      /*********************************************************************
       * If the first token (token number zero) in paramFileName[] matches  *
       * the pulse sequence and the second token (token number 1) matches   *
       * the fieldStrength value, then we have found our required line.     *
       *********************************************************************/
      if ((S[0] == seq) && (strtod(S[1]) == fieldStrength)) {
        /*********************************************************************
         * Now assigning sliceTime and pauseFlag.                             *
         *********************************************************************/
        *sliceTime = strtod(S[2]);
        *pauseFlag = strtol(S[3]);

        /*********************************************************************
         * If zero appears in the slice time field, then slice time is set to *
         * tr/dimZ.                                                           *
         *********************************************************************/
        if ((*sliceTime) == 0.0) {
          (*sliceTime) = (double)tr / (double)dimZ;
        }  // if

        /*********************************************************************
         * Now getting out of this while loop since we have found the         *
         * required slice time and pauseFlag values.                          *
         *********************************************************************/
        break;

      }  // if

    }  // if

    /*********************************************************************
     * Now clearing fileLine[].                                           *
     *********************************************************************/
    memset(fileLine, 0, ACQCORRECT_PREFS_LENGTH);

  }  // while

  /*********************************************************************
   * Closing the configuration file.                                    *
   *********************************************************************/
  fclose(prefsReader);

  /*********************************************************************
   * If sliceTime is still 0.0, then we were unable to find the         *
   * appropriate line the the configuration file. Therefore, an error   *
   * error message is printed out and then this program exits.          *
   *********************************************************************/
  if ((*sliceTime) == 0.0) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Unable to determine slice time and time of first slice. "
                "Used SEQUENCE = ["
             << seq << "] FIELD STRENGTH = [" << fieldStrength
             << "] PARAMETER FILE = [" << paramFileName << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

}  // void readParamFile(const string& paramFileName, const string& seq,
   // const double fieldStrength, double *sliceTime, int *pauseFlag,
   // const int tr, const int dimZ)

/*********************************************************************
 * This function returns true if                                      *
 * (TR - firstSlice - (sliceTime * dimZ)) >= 0. This is a simple      *
 * "sanity check" on these values.                                    *
 *********************************************************************/
bool sanityCheckOfValues(const double sliceTime, const double firstSlice,
                         const int tr, const int dimZ) {
  /*********************************************************************
   * A tolerance is declared and initialized.                           *
   *********************************************************************/
  const double epsilon = 0.0001;

  /*********************************************************************
   * diff is used to hold TR - firstSlice - (sliceTime * dimZ). If diff *
   * ends up being non-negative, then we are okay. However, due to the  *
   * vagaries of floating point arithmetic, diff could end up being     *
   * negative, but close to zero.                                       *
   *********************************************************************/
  const double diff = (double)tr - firstSlice - (sliceTime * (double)dimZ);

  /*********************************************************************
   * absDiff is used to hold the absolute value of the diff.            *
   *********************************************************************/
  const double absDiff = fabs(diff);

  /*********************************************************************
   * We check to see if absDiff is bounded by epsilon or that diff is   *
   * non-negative, and that tr, dimZ, and sliceTime are all positive,   *
   * and that firstSlice is non-negative.                               *
   *********************************************************************/
  return (((absDiff <= epsilon) || (diff > 0.0)) && (tr > 0) &&
          (firstSlice >= 0.0) && (sliceTime > 0.0) && (dimZ > 0));

}  // bool sanityCheckOfValues(const double sliceTime, const double firstSlice,
   // const int tr, const int dimZ)

/*********************************************************************
 * This function carries out Sinc Interpolation on the input          *
 * VB_Vector object. This function is derived from the IDL function   *
 * SincInterpo, found in VoxBo_Fourier.pro.                           *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * timeSeries        const VB_Vector& Timeseries vector object.       *
 * expFactor          int             The number of times the original*
 *                                    interval is to be expanded. For *
 *                                    example, if the interval is 1   *
 *                                    second long in the time series, *
 *                                    but half second intervals are   *
 *                                    desired, then expFactor should  *
 *                                    be 2.                           *
 * newSignal          VB_Vector&      The output vector, whose        *
 *                                    length = timeSeries.getLength() *
 *                                    * expFactor.                    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * newSignal           VB_Vector&     The output vector.              *
 ********************************************************************/
void sincInterpolate(const VB_Vector& timeSeries, const unsigned int expFactor,
                     VB_Vector& newSignal) {
  /*********************************************************************
   * The length of timeSeries is stored in length.                      *
   *********************************************************************/
  const unsigned long length = timeSeries.getLength();

  /*********************************************************************
   * If the length of timeSeries is less than 2 (mostly like 1, but     *
   * could be zero), then print an error message and exit.              *
   *********************************************************************/
  if (length < 2) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "]  Need length to be >= 2: length = [" << length << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * realPart and imagPart will hold the real and imaginary parts of    *
   * the FFT of timeSeries, respectively.                               *
   *********************************************************************/
  VB_Vector realPart(length);
  VB_Vector imagPart(length);

  /*********************************************************************
   * Now getting the FFT of timeSeries.                                 *
   *********************************************************************/
  timeSeries.fft(realPart, imagPart);

  /*********************************************************************
   * Making sure that newSignal is properly sized.                      *
   *********************************************************************/
  newSignal.resize(length * expFactor);

  /*********************************************************************
   * phi is a VB_Vector needed to carry out the sinc interpolation.     *
   * phi[0] will always be zero. The "lower half" values of phi will    *
   * be calculated and the "upper half" values of phi depend on its     *
   * "lower half" values. Specifically, the "lower half" values are     *
   * computed up to and including phi[length/2]. Then the "lower half"  *
   * values are reflected about the x-axis, and then about the          *
   * vertical line y = (length/2). Assume that the x-axis indexes phi   *
   * and the y-axis is phi[i]. When length is 10, the plot of phi       *
   * looks like:                                                        *
   *                                                                    *
   *                                                                    *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *    1.570 |                   o                                     *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *    1.256 |               o                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *     .942 |           o                                             *
   *          |                                                         *
   *          |                                                         *
   *     .628 |       o                                                 *
   *          |                                                         *
   *     .314 |   o                                                     *
   *          |                                                         *
   *     -----o---+---+---+---+---+---+---+---+---+---+                 *
   *          |   1   2   3   4   5   6   7   8   9                     *
   *    -.314 |                                   o                     *
   *          |                                                         *
   *    -.628 |                               o                         *
   *          |                                                         *
   *          |                                                         *
   *    -.942 |                           o                             *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *   -1.256 |                       o                                 *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *                                                                    *
   *********************************************************************/
  VB_Vector phi(length);

  /*********************************************************************
   * The following for loop is used to carry out the remaining work for *
   * the sinc interpolation.                                            *
   *********************************************************************/
  for (unsigned int i = 0; i < expFactor; i++) {
    /*********************************************************************
     * Calculating timeShift.                                             *
     *********************************************************************/
    double timeShift = (-1.0) * (double)i / (double)expFactor;

    /*********************************************************************
     * Now calling makePhi() to compute the elements of phi.              *
     *********************************************************************/
    makePhi(phi, timeShift);

    /*********************************************************************
     * We need to have a complex vector called "shifter"; realShifter     *
     * will be its real part and imagShifter will be its imaginary part.  *
     * "shifter" is used to hold the product of 2 complex vectors:        *
     *                                                                    *
     *     (cos(phi), sin(phi)) * FFT(timeSeries)                         *
     *********************************************************************/
    VB_Vector realShifter(length);
    VB_Vector imagShifter(length);

    /*********************************************************************
     * The following for loop is used to compute the complex product      *
     * described above.                                                   *
     *********************************************************************/
    for (unsigned int j = 0; j < length; j++) {
      /*********************************************************************
       * Assigning the value of the jth element of shifter.                 *
       *********************************************************************/
      realShifter[j] =
          (cos(phi[j]) * realPart[j]) - (sin(phi[j]) * imagPart[j]);
      imagShifter[j] =
          (cos(phi[j]) * imagPart[j]) + (sin(phi[j]) * realPart[j]);
    }  // for j

    /*********************************************************************
     * We now need to compute the inverse FFT of shifter. To that end:    *
     *                                                                    *
     * realRealShifter - holds the real part of the IFFT of the real part *
     *                   of shifter.                                      *
     * imagRealShifter - holds the imaginary part of the IFFT of the real *
     *                   part of shifter.                                 *
     * realImagShifter - holds the real part of the IFFT of the imaginary *
     *                   part of shifter.                                 *
     * imagRealShifter - holds the imaginary part of the IFFT of the      *
     *                   imaginary part of shifter.                       *
     *********************************************************************/
    VB_Vector realRealShifter(length);
    VB_Vector imagRealShifter(length);
    VB_Vector realImagShifter(length);
    VB_Vector imagImagShifter(length);

    /*********************************************************************
     * Now computing the IFFT of shifter.                                 *
     *********************************************************************/
    realShifter.ifft(realRealShifter, imagRealShifter);
    imagShifter.ifft(realImagShifter, imagImagShifter);

    /*********************************************************************
     * Getting the real part of the inverse FFT of (realShifter + i *     *
     * imagShifter) and assigning it to newSignal.                        *
     *********************************************************************/
    for (unsigned int j = 0; j < length; j++) {
      newSignal[j * expFactor + i] = realRealShifter[j] - imagImagShifter[j];
    }  // for j

  }  // for i

}  // void sincInterpolate(const VB_Vector& timeSeries,
   // const unsigned int expFactor, VB_Vector& newSignal)

/*********************************************************************
 * This function is a port of the IDL function PhaseShift, found in   *
 * VoxBo_Fourier.pro. This function "shifts" a signal in time to      *
 * provide an output vector that represents the same continuous)      *
 * signal sampled starting either later or earlier. This is           *
 * accomplished by a simple  shift of the phase of the sines that make*
 * up the signal. Recall that a Fourier transform allows for a        *
 * representation of any signal as the linear combination of          *
 * sinusoids of different frequencies and phases. Effectively, we will*
 * add a constant to the phase of every frequency, shifting the data  *
 * in time.                                                           *
 *                                                                    *
 * INPUTS:                                                            *
 * -------                                                            *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * tSeries            const VB_Vector& The time series to be shifted. *
 *                                                                    *
 * timeShift          double          Number of images to shift to    *
 *                                    the right, i.e., to lag the     *
 *                                    signal with respect to the      *
 *                                    original domain.                *
 *                                                                    *
 * shiftedSignal      VB_Vector&      Will hold the shifted signal.   *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * shiftedSignal       VB_Vector&      The shifted signal will be     *
 *                                     stored here.                   *
 ********************************************************************/
void phaseShift(const VB_Vector& tSeries, const double timeShift,
                VB_Vector& shiftedSignal) {
  /*********************************************************************
   * The number of elements of tSeries is stored in length.             *
   *********************************************************************/
  const unsigned long length = tSeries.getLength();

  /*********************************************************************
   * phi is a VB_Vector needed to carry out the phase shift.            *
   *********************************************************************/
  VB_Vector phi(length);

  /*********************************************************************
   * Now calling makePhi() to fill in the values in phi.                *
   *********************************************************************/
  makePhi(phi, timeShift);

  /*********************************************************************
   * Now taking the FFT of tSeries. The real part will be stored in     *
   * realPart and the imaginary part will be sotred in imagPart.        *
   *********************************************************************/
  VB_Vector realPart(length);
  VB_Vector imagPart(length);
  tSeries.fft(realPart, imagPart);

  /*********************************************************************
   * shifter is the filter by which the signal will be convolved to     *
   * introduce the phase shift. It is constructed explicitly in the     *
   * Fourier, i.e., frequency, domain. In the time domain, it may be    *
   * described as an impulse (delta function) that has been shifted in  *
   * time by the amount specified by timeShift. Now shifter is          *
   * instantiated as two VB_Vector objects, its real and imaginary      *
   * parts. The length of shifter is tSeries.getLength().               *
   *********************************************************************/
  VB_Vector realShifter(length);
  VB_Vector imagShifter(length);

  /*********************************************************************
   * The following for loop carries out the convolution by making the   *
   * cos() and sin() of phi into a complex number and then multiplying  *
   * it by the corresponding complex number from the FFT of tSeries.    *
   *********************************************************************/
  for (unsigned int i = 0; i < length; i++) {
    realShifter[i] = (cos(phi[i]) * realPart[i]) - (sin(phi[i]) * imagPart[i]);
    imagShifter[i] = (cos(phi[i]) * imagPart[i]) + (sin(phi[i]) * realPart[i]);
  }  // for i

  /*********************************************************************
   * We now need to compute the inverse FFT of shifter. To that end:    *
   *                                                                    *
   * realRealShifter - holds the real part of the IFFT of the real part *
   *                   of shifter.                                      *
   * imagRealShifter - holds the imaginary part of the IFFT of the real *
   *                   part of shifter.                                 *
   * realImagShifter - holds the real part of the IFFT of the imaginary *
   *                   part of shifter.                                 *
   * imagRealShifter - holds the imaginary part of the IFFT of the      *
   *                   imaginary part of shifter.                       *
   *********************************************************************/
  VB_Vector realRealShifter(length);
  VB_Vector imagRealShifter(length);
  VB_Vector realImagShifter(length);
  VB_Vector imagImagShifter(length);

  /*********************************************************************
   * Now computing the IFFT of shifter.                                 *
   *********************************************************************/
  realShifter.ifft(realRealShifter, imagRealShifter);
  imagShifter.ifft(realImagShifter, imagImagShifter);

  /*********************************************************************
   * Now we assign the real part of the inverse FFT of shifter to       *
   * shiftedSignal.                                                     *
   *********************************************************************/
  shiftedSignal = realRealShifter - imagImagShifter;

}  // void phaseShift(const VB_Vector& tSeries, const double timeShift,
   // VB_Vector& shiftedSignal)

/*********************************************************************
 * phi is a VB_Vector needed to carry out the phase shift. phi[0]     *
 * will always be zero. The "lower half" values of phi will be        *
 * calculated and the "upper half" values of phi depend on its        *
 * "lower half" values. Specifically, the "lower half" values are     *
 * computed up to and including phi[length/2]. Then the "lower half"  *
 * values are reflected about the x-axis, and then about the          *
 * vertical line y = (length/2). Assume that the x-axis indexes phi   *
 * and the y-axis is phi[i]. If length is 10, the plot of phi looks   *
 * like:                                                              *
 *                                                                    *
 *                                                                    *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *    1.570 |                   o                                     *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *    1.256 |               o                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *     .942 |           o                                             *
 *          |                                                         *
 *          |                                                         *
 *     .628 |       o                                                 *
 *          |                                                         *
 *     .314 |   o                                                     *
 *          |                                                         *
 *     -----o---+---+---+---+---+---+---+---+---+---+                 *
 *          |   1   2   3   4   5   6   7   8   9                     *
 *    -.314 |                                   o                     *
 *          |                                                         *
 *    -.628 |                               o                         *
 *          |                                                         *
 *          |                                                         *
 *    -.942 |                           o                             *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *   -1.256 |                       o                                 *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *                                                                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * phi                VB_Vector&      Vector used to hold the result. *
 * timeShift          const double    A constant needed to compute    *
 *                                    the phase shift.                *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * phi                 VB_Vector&      Holds the results.             *
 ********************************************************************/
void makePhi(VB_Vector& phi, const double timeShift) {
  /*********************************************************************
   * length stores the number of elements in phi.                       *
   *********************************************************************/
  const unsigned long length = phi.getLength();

  /*********************************************************************
   * halfLength holds the floor of length / 2.                          *
   *********************************************************************/
  const unsigned long halfLength = length / 2;

  /*********************************************************************
   * Even number of values.                                             *
   *********************************************************************/
  if ((length % 2) == 0) {
    /*********************************************************************
     * Now calculating the values for phi (for the case of an even        *
     * number of values in timeSeries).                                   *
     *********************************************************************/
    for (unsigned int j = 1; j <= halfLength; j++) {
      /*********************************************************************
       * The following line calculates the "lower half" values of phi.      *
       *********************************************************************/
      phi[j] = -1.0 * (timeShift * TWOPI) / ((double)length / (double)j);

      /*********************************************************************
       * When j is not equal to halfLength, then calculate the "upper half" *
       * values of phi.                                                     *
       *********************************************************************/
      if (j != halfLength) {
        phi[length - j] = -1.0 * phi[j];
      }  // if

    }  // for j

  }  // if

  /*********************************************************************
   * Odd number of values.                                              *
   *********************************************************************/
  else {
    /*********************************************************************
     * Now calculating the values for phi (for the case of an odd         *
     * number of values in timeSeries).                                   *
     *********************************************************************/
    for (unsigned int j = 1; j <= halfLength; j++) {
      /*********************************************************************
       * The following line calculates the "lower half" values of phi.      *
       *********************************************************************/
      phi[j] = -1.0 * (timeShift * TWOPI) / ((double)length / (double)j);

      /*********************************************************************
       * The following line calculates the "upper half" values of phi.      *
       *********************************************************************/
      phi[length - j] = -1.0 * phi[j];

    }  // for j

  }  // else

}  // void makePhi(VB_Vector& phi, const double timeShift)
