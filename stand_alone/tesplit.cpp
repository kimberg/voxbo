
// tesplit.cpp
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

using namespace std;

/*********************************************************************
* Required include files.                                            *
*********************************************************************/
#include "vb_common_incs.h"

/* >>>>>>>>>>>>           FUNCTION PROTOTYPES          <<<<<<<<<<<< */

void usage(const unsigned short exitValue, char *progName);
void cr8SubSeries(int begin, int end, const Tes& inputTes,
Tes& outputTes, const string& outFileName);

/* >>>>>>>>>>>>         END FUNCTION PROTOTYPES        <<<<<<<<<<<< */

/*********************************************************************
* This program splits 4D data files by separating out time series in *
* any one (or more) of three ways.                                   *
*********************************************************************/
int main(int argc, char *argv[])
{

/*********************************************************************
* Installing a signal handler for SIGSEGV.                           *
*********************************************************************/
  SEGV_HANDLER

/*********************************************************************
* tesfile will hold the input 4D data file name.                     *
*********************************************************************/
  string tesFile;

/*********************************************************************
* outFileStem will hold the stem name of the output file(s).         *
*********************************************************************/
  string outFileStem;

/*********************************************************************
* If no command line options have been used, then usage information  *
* is displayed and the program exits.                                *
*********************************************************************/
  if (argc == 1)
  {
    usage(1, argv[0]);
  } // if

/*********************************************************************
* Now processing the command line options.                           *
*                                                                    *
* -h ==> Display usage information.                                  *
* -i ==> Specifies the input 4D data file.                           *
* -o ==> Specifies the output files' stem name.                      *
* -r ==> If used, then the input file is removed.                    *
* -e ==> Extract a sub-time series.                                  *
* -d ==> Divide into equal sized sub-time series, except for possibly*
*        the last sub-time series.                                   *
* -b ==> Allot out consecutive time series into bins, as if dealing  *
*        cards.                                                      *
* -v ==> Print out the global VoxBo version number.                  *
*                                                                    *
* VARIABLES:                                                         *
* printHelp - a flag, used to determine if the "-h" command line     *
*             option was used or not.                                *
* printVersion - a flag, used to determine if the "-v" command line  *
*                option was used or not.                             *
* deleteInputFile - a flag, used to determine if the "-d" command    *
*                   line option was used or not.                     *
* extractionRange - used to hold the desired extraction range for    *
*                   a sub-set of the time series.                    *
* outputDir - used to hold the output files' directory name, if      *
*             user wants the output files in another directory (as   *
*             opposed to the current directory).                     *
* divSize - used to hold the divisor. Must be in [1, dimT].          *
* numBins - used to hold the number of bins.                         *
*********************************************************************/
  bool printHelp = false;
  bool printVersion = false;
  bool deleteInputFile = false;
  string extractionRange;
  string outputDir;
  int divSize = -1;
  int numBins = 0;
  string extractedName;
  string extractedDir;

  processOpts(argc, argv, ":hi:o:re:d:b:f:v", "bZZbZiiZb", &printHelp,
  &tesFile, &outFileStem, &deleteInputFile, &extractionRange, &divSize,
  &numBins, &outputDir, &printVersion);

  if (outputDir.size()) extractedDir = outputDir;
  if (outFileStem.size()) extractedName = outFileStem;

/*********************************************************************
* Now checking to see if usage information needs to be printed.      *
*********************************************************************/
  if (printHelp)
  {
    usage(0, argv[0]);
  } // if

/*********************************************************************
* If the input 4D data file has not been specified, then an error    *
* message is sent to stderr, and then usage information is           *
* displayed.                                                         *
*********************************************************************/
  if (tesFile.size() == 0)
  {
    ostringstream errorMsg;
    errorMsg << "Must specify the input 4D data file name.";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } // if

/*********************************************************************
* If the user did indeed specify an input file name, we now check to *
* see if it's user readable.                                         *
*********************************************************************/
  else
  {

/*********************************************************************
* If the input file is not readable, then an error message is        *
* printed and this program exits.                                    *
*********************************************************************/
    if (access(tesFile.c_str(),R_OK))
    {
      ostringstream errorMsg;
      errorMsg << "The input 4D data file ["
      << tesFile << "] is not processes readable.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

/*********************************************************************
* Now making sure that we actually have a 4D data file.              *
*********************************************************************/
    if (!validate4DFile(tesFile))
    {
      ostringstream errorMsg;
      errorMsg << "The file [" << tesFile
      << "] is not a 4D VoxBo data file.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

  } // else

/*********************************************************************
* Checking to ensure that the user selected at least one way of      *
* generating sub-sets of time series. If the user did not select a   *
* way of generating sub-sets of times series, then an error message  *
* and usage information are printed out.                             *
*********************************************************************/
  if ( (extractionRange.size() == 0) && (divSize <= 0) && (numBins <= 0) )
  {
    ostringstream errorMsg;
    errorMsg << "Must select at least one way of generating a time series sub-set.";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } // if

/*********************************************************************
* If the user specified an output directory, we prepend the          *
* directory containing tesFile to outputDir.                         *
*********************************************************************/
  if (outputDir.size() > 0)
  {
    outputDir = xdirname(tesFile) + "/" + outputDir + "/";
  } // if

/*********************************************************************
* If outputDir is the empty string, then it is set to the current    *
* directory.                                                         *
*********************************************************************/
  else
  {
    outputDir = "./";
  } // else

/*********************************************************************
* We check to see if the user specified a directory in which to save *
* the output files and if that directory exists or not. NOTE: If     *
* outputDir does not exist, access() will return -1, else 0 is       *
* returned.                                                          *
*********************************************************************/
  if ( (outputDir.size() > 0) && access(outputDir.c_str(), F_OK) )
  {

/*********************************************************************
* If the user did indeed specify a directory in which to save the    *
* output files, this output directory is interpreted to be a         *
* sub-directory of the directory which contains the input Tes file.  *
* (Of course, the directory containing the input Tes file could be   *
* the current directory.) Therefore, we will need to insert the      *
* output directory name after the directory path contained in the    *
* name of the input Tes file, tesFile.                               *
*********************************************************************/

    string tempDirName=xdirname(tesFile) + "/" + outputDir;

/*********************************************************************
* If tempDirName is not valid, it may be because it does not exist.  *
* In such a case, we need to call mkdir() to create it.              *
*********************************************************************/
    if (!vb_direxists(tempDirName))
    {

/*********************************************************************
* We need to call mkdir() to create the directory errno is set to 0  *
* and if mkdir() returns -1, indicating an error, an error message   *
* is sent to stderr with the appropriate reason for the error, as    *
* determined by strerror(errno). Then this program exits. The second *
* argument to mkdir(), named mode and of type mode_t, determines the *
* permissions on the created directory by:                           *
*                                                                    *
*                      (mode & ~umask)                               *
*********************************************************************/
      errno = 0;
      if (mkdir(tempDirName.c_str(), 0776) == -1)
      {
        ostringstream errorMsg;
        errorMsg << "Error in creating directory ["
        << tempDirName << "] due to: " << strerror(errno);
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } // if

    } // if

/*********************************************************************
* If tempDirName2 is not process writeable, then an error message is *
* sent to std err, and then this program exits.                      *
*********************************************************************/
    if (access(tempDirName.c_str(),W_OK))
    {
      ostringstream errorMsg;
      errorMsg << "The directory ["
      << tempDirName << "] is not writeable by this process.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

  } // if

/*********************************************************************
* If an output file stem name was not specified at the command line, *
* or the "-f" option was used, then  outFileStem is set to the first *
* field of the input 4D data file name and '.' is used as the field  *
* delimiter. NOTE: The "-f" option overrides the "-o" option.        *
*********************************************************************/
  if ( (outFileStem.size() == 0) || (outputDir.size() > 0) )
  {

/*********************************************************************
* Now assembling outFileStem.                                        *
*********************************************************************/
    outFileStem = outputDir + "/" + extractedName;//S.getToken(0);

  } // if

/*********************************************************************
* If program flow ends up here, then outFileStem is not the empty    *
* string. However, it is possible that the user specified just a     *
* directory name or a directory name without the final "/".          *
*********************************************************************/
  else
  {

    if (access(outFileStem.c_str(), F_OK) != -1)
    {

/*********************************************************************
* If outFileStem is just a directory name, then a "/" character is   *
* concatenated, just to be on the safe side (a second "/" after a    *
* directory name does not cause problems). Then the first field      *
* (where {'.', ' ', '\t'} are used as the field delimiters) from the *
* input 4D data file name is also concatenated to outfileStem.       *
*********************************************************************/
      if (vb_direxists(outFileStem))
      {
        outFileStem += "/" + extractedName;//S.getToken(0);
      } // if

    } // if

  } // else

/*********************************************************************
* If the directory that will contain the output files is not user    *
* writeable, then an error message is printed and then this program  *
* exits.                                                             *
*********************************************************************/
  if (access(xdirname(outFileStem).c_str(),W_OK))
  {
    ostringstream errorMsg;
    errorMsg << "Effective user id ["
    << geteuid() << "] does not have write permission in the directory ["
             << xdirname(outFileStem) << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Now instantiating a Tes object so the input 4D data file can be    *
* processed.                                                         *
*********************************************************************/
  Tes inputTes(tesFile);

/*********************************************************************
* If we were unable to read the 4D data file, an error message is    *
* printed out and this program exits.                                *
*********************************************************************/
  if (!inputTes.data_valid)
  {
    ostringstream errorMsg;
    errorMsg << "Unable to read file: [" << tesFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Now checking to see if the input 4D data file has already been     *
* split or not. If the file has already been split a number of times *
* and it has already been joined the same number of times, then      *
* splitting the file is allowed. Otherwise, an error message is      *
* printed and then this program exits. The the following for loop is *
* used to examine each header line, looking for lines whose first    *
* token is "TesSplit:" or "TesJoin:". The variable splitCount is     *
* used to keep track of the number of "TesSplit:" header lines,      *
* while the variable joinCount is used to keep track of the number   *
* of "TesJoin:" lines. NOTE: The space and tab characters are used   *
* as field delimiters.                                               *
*********************************************************************/
  int splitCount = 0, joinCount = 0;
  for (int i = 0; i < (int ) inputTes.header.size(); i++)
  {
    if (inputTes.header[i].size() > 0)
    {

/*********************************************************************
* Tokenizing the current header line.                                *
*********************************************************************/
      tokenlist S(inputTes.header[i]);

/*********************************************************************
* If we have a "TesSplit:" header line, then splitCount is           *
* incremented.                                                       *
*********************************************************************/
      if (S[0] == "TesSplit:")
      {
        splitCount++;
      } // if

/*********************************************************************
* If we have a "TesJoin:" header line, then joinCount is             *
* incremented.                                                       *
*********************************************************************/
      else if (S[0] == "TesJoin:")
      {
        joinCount++;
      } // else if

    } // if

  } // for i

/*********************************************************************
* If splitCount is not the same as joinCount, then an error message  *
* is printed and then this program exits.                            *
*********************************************************************/
  if (splitCount != joinCount)
  {
    ostringstream errorMsg;
     errorMsg << "The VoxBo 4D data file ["
    << inputTes.filename << "] has not been split [" << splitCount
    <<"] and joined [" << joinCount <<"] the same number of times.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Now assigning dimT, the length of the time dimension.              *
*********************************************************************/
  int dimT = inputTes.dimt;

/*********************************************************************
* If the string length of extractionRange is bigger than 0, then the *
* user wants to extract a range of time series.                      *
*********************************************************************/
  if (extractionRange.size() > 0)
  {

/*********************************************************************
* The C-style string extractionRange is validated to ensure that the *
* extraction range is in the proper format. If it is not, an error   *
* message is printed and then this program exits.                    *
* pattern[] is used to hold the regular expression as a C-style      *
* string. The regular expression says: one or more digits at the     *
* beginning, then a dash followed by one or more digits at the end.  *
*********************************************************************/
    char pattern[19];
    memset(pattern, 0, sizeof(pattern));
    strcpy(pattern, "^[0-9]\\+\\-[0-9]\\+$");
    if (!validateString(pattern, extractionRange.c_str()))
    {
      ostringstream errorMsg;
      errorMsg << "Extraction range must be of the form positive integer, dash, positive integer. Example: 23-45.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

    // use tokenlist to parse e.g., 23-47
    tokenlist S(extractionRange, "-");

/*********************************************************************
* Now getting the beginning and end points of the extraction.        *
*********************************************************************/
    int beginExt = (int ) strtol(S[0]);
    int endExt = (int ) strtol(S[1]);

/*********************************************************************
* Now making sure that beginExt <= endExt.                           *
*********************************************************************/
    if (beginExt > endExt)
    {

/*********************************************************************
* An error message is printed and then this program exits.           *
*********************************************************************/
      ostringstream errorMsg;
      errorMsg << "The start of the extraction ["
      << beginExt << "] must be <= to the end of the extraction [" << endExt << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);

    } // if

/*********************************************************************
* Now ensuring that beginExt >= 0.                                   *
*********************************************************************/
    if (beginExt < 0)
    {

/*********************************************************************
* An error message is printed and then this program exits.           *
*********************************************************************/
      ostringstream errorMsg;
      errorMsg << "The start of the extraction ["
      << beginExt << "] must be >= 0.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);

    } // if

/*********************************************************************
* Now ensuring that endExt < dimT.                                   *
*********************************************************************/
    if (endExt >= dimT)
    {

/*********************************************************************
* An error message is printed and then this program exits.           *
*********************************************************************/
      ostringstream errorMsg;
      errorMsg << "The end of the extraction ["
      << endExt << "] must be < dimT [" << dimT << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);

    } // if

/*********************************************************************
* Now that we have finished checking beginExt and endExt, we can go  *
* ahead and extract the desired time series. Instantiating outTes,   *
* a Tes object that will consist of the desired sub-set of the time  *
* series from inputTes.                                              *
*********************************************************************/
    Tes outTes = Tes();

/*********************************************************************
* Assembling the file name for outTes.                               *
*********************************************************************/
    //this code is so complex that this is the best way not to mess us anything
    //else, short of a re-write
    string outFileName; // = outFileStem + "Extracted.tes";
    if (extractedDir.size()>0 && extractedDir[0]!='/') {
       outFileName = extractedDir+"/";
    }
    else if (extractedDir[0]=='/') {
       outFileName = xdirname(outFileStem);
    }
    else {
       outFileName = "./";
    }
    if (extractedName.size()) {
       outFileName+=("/"+extractedName);
    }
    else {
       outFileName+="extracted.tes";
    }
/*********************************************************************
* Calling cr8SubSeries() to populate outTes.                         *
*********************************************************************/
    cr8SubSeries(beginExt, endExt, inputTes, outTes, outFileName);

/*********************************************************************
* Now adding all the header lines from the input 4D data file to     *
* outTes' header.                                                    *
*********************************************************************/
    copyHeader(inputTes, outTes);

/*********************************************************************
* Now adding the "VoxSizes(XYZ)" and "Origin(XYZ)" header lines from *
* inputTes to outTes because these two header lines are  not stored  *
* in the Tes::header data member.                                    *
*********************************************************************/
    addHeaderLine(outTes, "Sfff", "VoxSizes(XYZ):",
    inputTes.voxsize[0], inputTes.voxsize[1], inputTes.voxsize[2]);
    addHeaderLine(outTes, "Siii", "Origin(XYZ):",
    inputTes.origin[0], inputTes.origin[1], inputTes.origin[2]);

/*********************************************************************
* Now adding the header line to outTes for the extraction process    *
* performed on the original input file. The line will resemble:      *
*                                                                    *
* Indicates a                                                        *
* Tes "split".                                                       *
*   |                                                                *
*   |                                                                *
*  \|/                                                               *
* TesSplit: Extraction Tue_Dec_21_15:12:52_1999 file.tes 23-56 180   *
*              ^               ^                   ^       ^    ^    *
*        ______|               |                   |       |    |    *
*        |                     |                   |       |    |    *
* The name of this             |                   |       |    |    *
* operation.                   |                   |       |    |    *
*                    The time and date of          |       |    |    *
*                    this operation.               |       | original*
*                                                  |       |   dimT  *
*                                             The name of  |         *
*                                             the original |         *
*                                             input file.  |         *
*                                                          |         *
*                                                   The extraction   *
*                                                   range.           *
*                                                                    *
* NOTE: Each of the above fields will be delimited by tabs.          *
*********************************************************************/
    addHeaderLine(outTes, "SSSSSi", "TesSplit:", "Extraction", timedate().c_str(),
    tesFile.c_str(), extractionRange.c_str(), dimT);

/*********************************************************************
* Now writing out the output file. Upon success, Tes::WriteFile()    *
* returns 0. If that is not the case, then an error message is       *
* written out and then this program exits.                           *
*********************************************************************/
    if (outTes.WriteFile())
    {
      ostringstream errorMsg;
      errorMsg << "Unable to write Tes file [" << outTes.filename << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

  } // if

/*********************************************************************
* If divSize is greater than 0, then the users wants to split the    *
* time series in the input 4D data file into consecutively equal     *
* sized sub-sets, except for possibly the last sub-set.              *
*********************************************************************/
  if (divSize > 0)
  {

/*********************************************************************
* Making sure that divSize <= dimT. If it's not, then an error       *
* message is printed out and then this program exits.                *
*********************************************************************/
    if (divSize > dimT)
    {
      ostringstream errorMsg;
      errorMsg << "The division size [" << divSize
      << "] must be <= the time dimension: [" << dimT << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

/*********************************************************************
* Making sure that divSize > 0. If it's not, then an error message   *
* is printed out and then this program exits.                        *
*********************************************************************/
    if (divSize <= 0)
    {
      ostringstream errorMsg;
      errorMsg << "The division size ["
      << divSize << "] must be > 0.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

/*********************************************************************
* The following for loop is used to traverse the time series from    *
* the input 4D data file.                                            *
*                                                                    *
* numFile - used to number the output files.                         *
* numString[] - used to hold numFile as a C-style string.            *
*********************************************************************/
    char numString[TIME_STR_LEN];
    int numFile = 0;
    for (int i = 0; (i + divSize) <= dimT; i += divSize, numFile++)
    {

/*********************************************************************
* Now that we have verified that divSize has a valid value, we can   *
* go ahead and process the input 4D data file. An output Tes object  *
* is instantiated.                                                   *
*********************************************************************/
      Tes outTes = Tes();

/*********************************************************************
* Assembling the output file name. First, numFile is turned into a   *
* C-style string.                                                    *
*********************************************************************/
      memset(numString, 0, TIME_STR_LEN);
      sprintf(numString, "%d", numFile);
      string outFileName = outFileStem + "DivFile" + numString + ".tes";

/*********************************************************************
* Calling cr8SubSeries() to extract the appropriate sub-set of the   *
* time series and populate outTes.                                   *
*********************************************************************/
      cr8SubSeries(i, i + divSize - 1, inputTes, outTes, outFileName);

/*********************************************************************
* Now adding all the header lines from the input 4D data file to     *
* outTes' header.                                                    *
*********************************************************************/
      copyHeader(inputTes, outTes);

/*********************************************************************
* Now adding the "VoxSizes(XYZ)" and "Origin(XYZ)" header lines from *
* inputTes to outTes because these two header lines are  not stored  *
* in the Tes::header data member.                                    *
*********************************************************************/
      addHeaderLine(outTes, "Sfff", "VoxSizes(XYZ):",
      inputTes.voxsize[0], inputTes.voxsize[1], inputTes.voxsize[2]);
      addHeaderLine(outTes, "Siii", "Origin(XYZ):",
      inputTes.origin[0], inputTes.origin[1], inputTes.origin[2]);

/*********************************************************************
* Now adding the header line to outTes for the extraction process    *
* performed on the original input file. The line will resemble:      *
*                                                                    *
* Indicates a                                                        *
* Tes "split".                                                       *
*   |                                                                *
*   |                                                                *
*  \|/                                                               *
* TesSplit: Divided Tue_Dec_21_15:12:52_1999 file.tes 10-19 180      *
*              ^               ^                ^       ^    ^---    *
*        ______|               |                |       |___    |    *
*        |                     |                |___       |    |    *
* The name of this             |                   |       |    |    *
* operation.                   |                   |       | original*
*                    The time and date of          |       |   dimT  *
*                    this operation.               |       |         *
*                                                  |       |         *
*                                             The name of  |         *
*                                             the original |         *
*                                             input file.  |         *
*                                                          |         *
*                                                   The extraction   *
*                                                   range.           *
*                                                                    *
* NOTE: Each of the above fields will be delimited by tabs.          *
*                                                                    *
* numString[] - used to hold the range of the current division, as a *
*               C-style string.                                      *
*********************************************************************/
      memset(numString, 0, TIME_STR_LEN);
      sprintf(numString, "%d-%d", i, i + divSize - 1);
      addHeaderLine(outTes, "SSSSSi", "TesSplit:", "Divided", timedate().c_str(),
                    tesFile.c_str(), numString, dimT);

/*********************************************************************
* Now writing out the output file. Upon success, Tes::WriteFile()    *
* returns 0. If that is not the case, then an error message is       *
* written out and then this program exits.                           *
*********************************************************************/
      if (outTes.WriteFile())
      {
        ostringstream errorMsg;
        errorMsg << "Unable to write Tes file ["
        << outTes.filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } // if

    } // for i

/*********************************************************************
* Upon exiting the above for loop, we may still have some time series*
* that have not been extracted from the original 4D data file. This  *
* will happen when (dimT % (divSize + 1)) != 0.                      *
*********************************************************************/
    int leftOver = 0;
    if ( (leftOver = (dimT % divSize)) != 0 )
    {

/*********************************************************************
* Now that we have verified that divSize has a valid value, we can   *
* go ahead and process the input 4D data file. An output Tes object  *
* is instantiated.                                                   *
*********************************************************************/
      Tes outTes = Tes();

/*********************************************************************
* Assembling the output file name.                                   *
*                                                                    *
* numString[] - used to hold numFile as a C-style string.            *
*********************************************************************/
      memset(numString, 0, TIME_STR_LEN);
      sprintf(numString, "%d", numFile);
      string outFileName = string(outFileStem) + string("DivFile")
                  + string(numString) + string(".tes");

/*********************************************************************
* Calling cr8SubSeries() to extract the appropriate sub-set of the   *
* time series and populate outTes.                                   *
*********************************************************************/
      cr8SubSeries(dimT - leftOver, dimT - 1, inputTes, outTes, outFileName);

/*********************************************************************
* Now adding all the header lines from the input 4D data file to     *
* outTes' header.                                                    *
*********************************************************************/
      copyHeader(inputTes, outTes);

/*********************************************************************
* Now adding the "VoxSizes(XYZ)" and "Origin(XYZ)" header lines from *
* inputTes to outTes because these two header lines are  not stored  *
* in the Tes::header data member.                                    *
*********************************************************************/
      addHeaderLine(outTes, "Sfff", "VoxSizes(XYZ):",
      inputTes.voxsize[0], inputTes.voxsize[1], inputTes.voxsize[2]);
      addHeaderLine(outTes, "Siii", "Origin(XYZ):",
      inputTes.origin[0], inputTes.origin[1], inputTes.origin[2]);

/*********************************************************************
* Now adding the header line to outTes for the extraction process    *
* performed on the original input file. The line will resemble:      *
*                                                                    *
* Indicates a                                                        *
* Tes "split".                                                       *
*   |                                                                *
*   |                                                                *
*  \|/                                                               *
* TesSplit: Divided Tue_Dec_21_15:12:52_1999 file.tes 10-19 180      *
*              ^               ^                ^       ^    ^---    *
*        ______|               |                |       |___    |    *
*        |                     |                |___       |    |    *
* The name of this             |                   |       |    |    *
* operation.                   |                   |       | original*
*                    The time and date of          |       |   dimT  *
*                    this operation.               |       |         *
*                                                  |       |         *
*                                             The name of  |         *
*                                             the original |         *
*                                             input file.  |         *
*                                                          |         *
*                                                   The extraction   *
*                                                   range.           *
*                                                                    *
* NOTE: Each of the above fields will be delimited by tabs.          *
*                                                                    *
* numString[] - used to hold the range of the current division, as a *
*               C-style string.                                      *
*********************************************************************/
      memset(numString, 0, TIME_STR_LEN);
      sprintf(numString, "%d-%d", dimT - leftOver, dimT - 1);
      addHeaderLine(outTes, "SSSSSi", "TesSplit:", "Divided",timedate().c_str(),
                    tesFile.c_str(), numString, dimT);

/*********************************************************************
* Now writing out the output file. Upon success, Tes::WriteFile()    *
* returns 0. If that is not the case, then an error message is       *
* written out and then this program exits.                           *
*********************************************************************/
      if (outTes.WriteFile())
      {
        ostringstream errorMsg;
        errorMsg << "Unable to write Tes file ["
        << outTes.filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } // if

    } // if

  } // if

/*********************************************************************
* If numBins is greater than 0, then the users wants to "deal out"   *
* time series into bins.                                             *
*********************************************************************/
  if (numBins > 0)
  {

/*********************************************************************
* Ensuring that numBins is <= dimT.                                  *
*********************************************************************/
    if (numBins > dimT)
    {
      ostringstream errorMsg;
      errorMsg << "numBins ["
      << numBins << "] must be less than the time dimension: [" << dimT << "]";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

/*********************************************************************
* The array outTesArr[] is used to hold the required number of Tes   *
* objects (from which the output files will be written).             *
*********************************************************************/
    Tes outTesArr[numBins];

/*********************************************************************
* leftOver holds the number of time series that are "extra".         *
*********************************************************************/
    int leftOver = dimT % numBins;

/*********************************************************************
* Integer division is carried out to determine the number of time    *
* series in each output file. This number plus 1 (numSeries + 1)     *
* will be the number of time series in each output file whose index  *
* in outTesArr[] is less than leftOver.                              *
*********************************************************************/
    int numSeries = dimT / numBins;

/*********************************************************************
* indexBinStr is used to hold indexBin.                              *
*********************************************************************/
    ostringstream indexBinStr;

/*********************************************************************
* The following for loop is used to traverse outTesArr[], populating *
* each Tes object with the appropriate time series from the input 4D *
* data file.                                                         *
*********************************************************************/
    for (int indexBin = 0; indexBin < numBins; indexBin++)
    {

/*********************************************************************
* Converting indexBin into a C-style string and creating the output  *
* file name.                                                         *
*********************************************************************/
      indexBinStr.str("");
      indexBinStr << indexBin;
      string outFileName = outFileStem + "Bin_" + indexBinStr.str() + ".tes";

/*********************************************************************
* Assigning an empty Tes object to outTesArr[indexBin] and then      *
* setting its output file name.                                      *
*********************************************************************/
      outTesArr[indexBin] = Tes();
      outTesArr[indexBin].SetFileName(outFileName);

/*********************************************************************
* If indexBin < leftOver, then outTesArr[indexBin] gets an "extra"   *
* time series. Otherwise, outTesArr[indexBin] gets numSeries time    *
* series.                                                            *
*********************************************************************/
      if (indexBin < leftOver)
      {
        outTesArr[indexBin].SetVolume(inputTes.dimx, inputTes.dimy, inputTes.dimz, numSeries + 1, inputTes.datatype);
      } // if
      else
      {
        outTesArr[indexBin].SetVolume(inputTes.dimx, inputTes.dimy, inputTes.dimz, numSeries, inputTes.datatype);
      } // else

/*********************************************************************
* j is used to index the time series from the Tes object instantiated*
* with the input 4D data file. Say the input 4D data file has:       *
*                                                                    *
* VoxDims(TXYZ):  180     40      64      21                         *
*                                                                    *
* and that numBins is 2 (an even/odd split). Then there will be 2    *
* output files. The first output file will get time series           *
* {0, 2, 4, 6, ..., 178}. The second output file will get time series*
* {1, 3, 5, 7, ..., 179}. Therefore, j must be incremented by 2 in   *
* each iteration of the following for loop, i.e., j must be          *
* incremented by numBins.                                            *
*********************************************************************/
      int j = indexBin;

/*********************************************************************
* startSeries holds the number of the first time series to be put in *
* the current output file. Preserving startSeries in the header will *
* allow the re-joining of the output files created by this program.  *
*********************************************************************/
      int startSeries = indexBin;

/*********************************************************************
* The following for loop is used to populate the time series in      *
* outTesArr[indexBin].                                               *
*********************************************************************/
      for (int indexOut = 0; indexOut < outTesArr[indexBin].dimt; indexOut++, j += numBins)
      {

/*********************************************************************
* OPEN_SPATIAL_LOOPS is a macro used to index the (x, y, z)          *
* coordinates of a time series.                                      *
*********************************************************************/
// qq Can make this faster !
        OPEN_SPATIAL_LOOPS(inputTes)

          outTesArr[indexBin].SetValue(indexX, indexY, indexZ, indexOut, inputTes.GetValue(indexX, indexY, indexZ, j));

/*********************************************************************
* CLOSE_SPATIAL_LOOPS is a macro used to close the for loops opened  *
* by OPEN_SPATIAL_LOOPS.                                             *
*********************************************************************/
        CLOSE_SPATIAL_LOOPS

      } // for indexOut

/*********************************************************************
* Now adding all the header lines from the input 4D data file to     *
* outTes' header.                                                    *
*********************************************************************/
      copyHeader(inputTes, outTesArr[indexBin]);

/*********************************************************************
* Now adding the "VoxSizes(XYZ)" and "Origin(XYZ)" header lines from *
* inputTes to outTesArr[indexBin] because these two header lines are *
* not stored in the Tes::header data member.                         *
*********************************************************************/
      addHeaderLine(outTesArr[indexBin], "Sfff", "VoxSizes(XYZ):",
      inputTes.voxsize[0], inputTes.voxsize[1], inputTes.voxsize[2]);
      addHeaderLine(outTesArr[indexBin], "Siii", "Origin(XYZ):",
      inputTes.origin[0], inputTes.origin[1], inputTes.origin[2]);

/*********************************************************************
* Now adding the header line to outTesArr[indexBin] for this         *
* splitting process. The line will resemble:                         *
*                                                                    *
* Indicates a                  The first time series from the input  *
* Tes "split".                 4D data file to appear in this output *
*   |                          file.                                 *
*   |                            |__________________________         *
*  \|/                                                     \|/       *
* TesSplit: Binned Tue_Dec_21_15:12:52_1999 file.tes 10 180 2        *
*             ^             ^                 ^      ^   ^           *
*        _____|             |            _____|      |   |____       *
*        |                  |            |___        |       |       *
* The name of this          |                |       |       |       *
* operation.                |                |       |    original   *
*                    The time and date of    |       |      dimT     *
*                    this operation.         |       |               *
*                                            |       |               *
*                                       The name of  |               *
*                                       the original |               *
*                                       input file.  |               *
*                                                    |               *
*                                                   The number of    *
*                                                   bins.            *
*                                                                    *
* NOTE: Each of the above fields will be delimited by tabs.          *
*********************************************************************/
      addHeaderLine(outTesArr[indexBin], "SSSSiii", "TesSplit:",
                    "Binned", timedate().c_str(), tesFile.c_str(), numBins, dimT, startSeries);

/*********************************************************************
* Now writing out the output file. Upon success, Tes::WriteFile()    *
* returns 0. If that is not the case, then an error message is       *
* written out and then this program exits.                           *
*********************************************************************/
      if (outTesArr[indexBin].WriteFile())
      {
        ostringstream errorMsg;
        errorMsg << "Unable to write Tes file ["
        << outTesArr[indexBin].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } // if

    } // for indexBin

  } // if

/*********************************************************************
* deleteInputFile is examined to see if the user wants to delete the *
* input file.                                                        *
*********************************************************************/
  if (deleteInputFile)
  {

/*********************************************************************
* errno is set to 0. If the unlink() system call fails, then errno   *
* will be set to a non-zero integer.                                 *
*********************************************************************/
    errno = 0;

/*********************************************************************
* If unlink() returns a negative int, then an error occurred.        *
*********************************************************************/
    if (unlink(tesFile.c_str()) < 0)
    {

/*********************************************************************
* Printing out an appropriate error message.                         *
*********************************************************************/
      ostringstream errorMsg;
      errorMsg << "Error in removing file ["
      << tesFile << "] due to: " << strerror(errno);
      printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);

    } // if

  } // if

  return 0;

} // main

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
void usage(const unsigned short exitValue, char *progName)
{
  printf("\nVoxBo tesplit (v%s)\n",vbversion.c_str());
  genusage(exitValue, progName, "- 4D data file splitting routine for VoxBo.",
           "-h -i[4D data file name] -o[output stem name] -r -e[range]\n               -d[divisor] -b[bins] -v",
           "-h                        Print usage information. Optional.",
           "-i <4D data file>         Specify the input 4D data file. Required.",
           "-o <output stem name>     Specify the output stem file name. Optional.",
           "                          Default output stem file name is the first",
           "                          part of the input 4D data file name.",
           "                          NOTE: This option is overridden by -f.",
           "-r                        If used, then remove the input file. Optional.",
           "                          Default is not to remove the input file.",
           "-e <range>                Extraction range. Example: -e 23-56",
           "                          NOTE: Slices are indexed beginning with 0.",
           "-d <divisor>              Split the 4D data file by divisor. The final",
           "                          output file may have fewer time series than divisor.",
           "-b <bins>                 Each \"bin\" gets a time series until the time",
           "                          series are exhausted. Using \"-b 2\" results",
           "                          in an even/odd split, resulting in 2 output files.",
           "                          NOTE: At least one of -e, -d, and -b is required.",
           "-f <output directory>     Save the output files in this directory, as",
           "                          opposed to the current directory. Optional.",
           "                          NOTE: If the output directory does not exist,",
           "                          it will be created and it will be a sub-directory",
           "                          of the directory containing the input 4D data file.",
           "                          NOTE: This option overrides -o.",
           "-v                        Global VoxBo version number. Optional.",
           "                          NOTE: Previously split files are not allowed to",
           "                          be split until they are joined.",
           "");

} // void usage(const unsigned short exitValue, char *progName)

/*********************************************************************
* This function extracts a sub-time series from the input Tes object *
* and populates the output Tes object.                               *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* begin              int             The start point of the          *
*                                    extraction.                     *
* end                int             The end point of the extraction.*
*                                    NOTE: The extraction is         *
*                                    inclusive.                      *
* inputTes           Tes *           Pointer to the Tes object from  *
*                                    which the extraction takes      *
*                                    place.                          *
* outputTes          Tes *           Pointer to the output Tes       *
*                                    object.                         *
* outFileName        string          The output Tes object's file    *
*                                    name.                           *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* None.                                                              *
*                                                                    *
*********************************************************************/
void cr8SubSeries(int begin, int end, const Tes& inputTes,
Tes& outputTes, const string& outFileName)
{

/*********************************************************************
* If the begin point of the extraction is < 0, then an error message *
* is printed and then this program exits.                            *
*********************************************************************/
  if (begin < 0)
  {
    ostringstream errorMsg;
    errorMsg << "The begin point [" << begin
    << "] must be non-negative.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* If the end point of the extraction is >= dimT, then an error       *
* message is printed and then this program exits.                    *
*********************************************************************/
  if (end >= inputTes.dimt)
  {
    ostringstream errorMsg;
    errorMsg << "The end point ["
    << end << "] must be less than the time dimension: [" << inputTes.dimt << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* newDimT - holds the number of time series we need to extract.      *
*********************************************************************/
    int newDimT = end - begin + 1;

/*********************************************************************
* Constructing the output Tes object with the proper dimensions and  *
* data type.                                                         *
*********************************************************************/
    outputTes.SetVolume(inputTes.dimx, inputTes.dimy,
    inputTes.dimz, newDimT, inputTes.datatype);

/*********************************************************************
* The following series of nested for loops are used to get the       *
* appropriate signal value from inputTes and assign in to outputTes. *
*********************************************************************/
    OPEN_SPATIAL_LOOPS(inputTes)

/*********************************************************************
* outIndexT is used to index the T dimension of outputTes.           *
*********************************************************************/
      int outIndexT = 0;

/*********************************************************************
* This for loop indexes the desired sub-set of time series,          *
* inclusively.                                                       *
*********************************************************************/
      for (int indexT = begin; indexT <= end; indexT++)
      {

/*********************************************************************
* Now getting the appropriate value from inputTes and assigning it to*
* output Tes.                                                        *
*********************************************************************/
        outputTes.SetValue(indexX, indexY, indexZ, outIndexT,
        inputTes.GetValue(indexX, indexY, indexZ, indexT));

/*********************************************************************
* Incrementing outIndexT.                                            *
*********************************************************************/
        outIndexT++;

      } // for indexT

    CLOSE_SPATIAL_LOOPS

/*********************************************************************
* Now setting the file name for the outputTes object.                *
*********************************************************************/
    outputTes.SetFileName(outFileName);

} // void cr8SubSeries(int begin, int end, const Tes& inputTes,
  // Tes& outputTes, const string& outFileName)
