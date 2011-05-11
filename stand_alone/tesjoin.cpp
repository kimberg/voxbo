
// tesjoin.cpp
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

#include <dirent.h> // For getdirentries().
#include <fstream> // For getdirentries().
#include <vector>
#include "vbio.h" //TGK
#include "vb_common_incs.h"

const int MOVE_PARAMS = 7;

void usage(const unsigned short exitValue, char *progName);
bool verifyFile(const string& fileName);
void getVec(const string& tesFile, const int dimT, VB_Vector &pVec);
void writeMoveParamsFile(const double *paramValues, const string& outputFile, const int origDimT, const string& firstSplitFile);
string getLastTesSplitLine(const Tes& theTes);

/*********************************************************************
* This program joins a set of Tes files previously split by tesplit. *
* The exact method of joining will depend on the type of split       *
* carried out by tesplit.                                            *
*********************************************************************/
int main(int argc, char *argv[]) {
  SEGV_HANDLER
  string fileName;
  string directoryName;
  string fileList;
  string outputFile;
  if (argc == 1)
    usage(1, argv[0]);

/*********************************************************************
* Now processing the command line options.                           *
*                                                                    *
* -h ==> Display usage information.                                  *
* -d ==> Specifies the directory for the input files.                *
* -f ==> Specifies the file holding the names of the input files.    *
* -l ==> Specifies a list of input file. This list must be           *
*        enclosed by doubles quotes and space delimited.             *
* -o ==> Specifies the output file name.                             *
* -m ==> Also join the corresponding "_MoveParams.ref" files.        *
* -v ==> Print out the gobal VoxBo version number.                   *
*                                                                    *
* VARIABLES:                                                         *
* printHelp - a flag, used to determine if the "-h" command line     *
*             option was used or not.                                *
* printVersion - a flag, used to determine if the "-v" command line  *
*                option was used or not.                             *
* deleteFiles - a flag, if used then the input files will be deleted.*
* moveParams - a flag, if used then join the "_MoveParams.ref" files *
*              as well.                                              *
*********************************************************************/
  bool printHelp = false;
  bool printVersion = false;
  bool deleteFiles = false;
  bool moveParams = false;
  processOpts(argc, argv, ":hd:f:l:o:vrm", "bZZZZbbb", &printHelp,
  &directoryName, &fileName, &fileList, &outputFile, &printVersion,
  &deleteFiles, &moveParams);
  if (printHelp)
    usage(0, argv[0]);

  if (printVersion)
    printf("\nVoxBo v%s\n",vbversion.c_str());

  if ( (directoryName.size() == 0) && (fileName.size() == 0) && (fileList.size() == 0) ) {
    ostringstream errorMsg;
    errorMsg << "Must use exactly one of \"-d\", \"-f\", or \"-l\".";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } 

  if ( (directoryName.size() > 0) && ((fileName.size() > 0) || (fileList.size() > 0)) ) {
    ostringstream errorMsg;
    errorMsg << "Must use exactly one of \"-d\", \"-f\", or \"-l\".";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } 

  if ( (fileName.size() > 0) && ((directoryName.size() > 0) || (fileList.size() > 0)) ) {
    ostringstream errorMsg;
    errorMsg << "Must use exactly one of \"-d\", \"-f\", or \"-l\".";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } 

  if ( (fileList.size() > 0) && ((directoryName.size() > 0) || (fileName.size() > 0)) ) {
    ostringstream errorMsg;
    errorMsg << "Must use exactly one of \"-d\", \"-f\", or \"-l\".";
    printErrorMsg(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
    usage(1, argv[0]);
  } 
  vector<string> inputFiles;
  if (directoryName.size() > 0) {
    if (!vb_direxists(directoryName)) {
      ostringstream errorMsg;
      errorMsg << "[" << directoryName << "] is not a directory.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    if (access(directoryName.c_str(),R_OK)) {
      ostringstream errorMsg;
      errorMsg << "The directory [" << directoryName << "] is not readable.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    errno = 0;
    DIR *dPtr;
    struct dirent *dirInfo;
    dPtr = opendir(directoryName.c_str());
    if (errno != 0) {
      ostringstream errorMsg;
      errorMsg << "Opening directory ["
      << directoryName << "] failed due to [" << strerror(errno) << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    }
    dirInfo = readdir(dPtr); // Reading ".".
    dirInfo = readdir(dPtr); // Reading "..".
    while ( (dirInfo = readdir(dPtr)) != NULL ) {
      string fullFileName = directoryName + "/" + dirInfo->d_name;
      if (!vb_fileexists(fullFileName)) {
        if (vb_direxists(fullFileName))
          continue;
        else {
          ostringstream errorMsg;
          errorMsg << "[" << fullFileName
          << "] is not a regular file nor a directory.";
          printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
        } 
      } 
      if (access(fullFileName.c_str(),R_OK)) {
        ostringstream errorMsg;
        errorMsg << "The file [" << fullFileName << "] is not readable.";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      }
      if (utils::determineVBFileDims(fullFileName) == 4) {
        inputFiles.push_back(fullFileName);
      } 
      else {
        ostringstream errorMsg;
        errorMsg << "[" << fullFileName << "] is not a 4D VoxBo data file or is unreadable.";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
    } 
    closedir(dPtr);
  } 
  else if (fileName.size() > 0) {
    if (!verifyFile(fileName)) {
      ostringstream errorMsg;
      errorMsg << "[" << fileName
      << "] is either not a regular file or not readable.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    }
    char fileBuffer[PATH_MAX];
    string inputFileNames = "";
    ifstream fin(fileName.c_str());
    if (!fin) {
      ostringstream errorMsg;
      errorMsg << "Unable to open ["
      << fileName << "] for reading.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    }
    while(fin.getline(fileBuffer, PATH_MAX))
      inputFileNames = inputFileNames + " " + fileBuffer;
    tokenlist fileParser(inputFileNames);
    for (size_t i = 0; i < fileParser.size(); i++) {
      string tempFile = fileParser[i];
      if (utils::determineVBFileDims(tempFile) == 4)
        inputFiles.push_back(tempFile);
      else {
        ostringstream errorMsg;
        errorMsg << "[" << tempFile << "] is not a 4D VoxBo data file or is unreadable.";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
    } 
  } 
  else if (fileList.size() > 0) {
    tokenlist S(fileList);
    for (size_t i = 0; i < S.size(); i++) {
      string file = S[i];
      if (utils::determineVBFileDims(file) != 4) {
        ostringstream errorMsg;
        errorMsg << "[" << file << "] is not a 4D VoxBo data file or is unreadable.";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      inputFiles.push_back(file);
    } 
  } 
  Tes splitTes[inputFiles.size()];
  int origDimT = 0;
  int dimX = 0;
  int dimY = 0;
  int dimZ = 0;
  string splitType;
  VB_datatype origDataType = vb_byte;
  string dateCreatedLine;
  string rawFileLine;
  string subjectNameLine;
  string subjectAgeLine;
  double TRval=0;
  string TELine;
  string orientationLine;
  string coilNameLine;
  string pulseSeqLine;
  string fieldStrengthLine;
  string taskTagLine;
  string byteOrderLine;
  float origVoxSizes[3];
  int origOrigin[3];
  memset(origVoxSizes, 0, sizeof(float) * 3);
  memset(origOrigin, 0, sizeof(int) * 3);
  string headerSplitLine = string("");
  for (unsigned int i = 0; i < inputFiles.size(); i++) {
    splitTes[i].ReadFile(inputFiles[i]);
    if (!splitTes[i].data_valid) {
      ostringstream errorMsg;
      errorMsg << "Unable to read input Tes file: [" << inputFiles[i] << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    vector<VBFF> listoftypes = EligibleFileTypes(splitTes[i].filename);
    if ( (listoftypes.size() == 0) || (listoftypes[0].getDimensions() != 4) ) {
      ostringstream errorMsg;
      errorMsg << "[" << inputFiles[i] << "] is not a Tes file.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    headerSplitLine = getLastTesSplitLine(splitTes[i]);
    tokenlist S(headerSplitLine);
    if (i == 0) {
      dimX = splitTes[i].dimx;
      dimY = splitTes[i].dimy;
      dimZ = splitTes[i].dimz;
      origDataType = splitTes[i].datatype;
      splitType = S[1];
      origDimT = (int ) strtol(S[5]);
      origVoxSizes[0] = splitTes[i].voxsize[0];
      origVoxSizes[1] = splitTes[i].voxsize[1];
      origVoxSizes[2] = splitTes[i].voxsize[2];
      origOrigin[0] = splitTes[i].origin[0];
      origOrigin[1] = splitTes[i].origin[1];
      origOrigin[2] = splitTes[i].origin[2];
      if (splitType == string("Extraction")) {
        ostringstream errorMsg;
        errorMsg << "Split type of \"Extraction\" does not allow joining.";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      else if ( (splitType == "Divided") || (splitType == "Binned") ) {
        if (outputFile.size() == 0)
          outputFile = S[3];
      } 
      else {
        ostringstream errorMsg;
        errorMsg << "Unrecognized split type: [" << splitType << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      /*********************************************************************
      * We now assign the following lines from the header of splitTes[0].  *
      * These header lines must be the same for all splitTes[i].           *
      *********************************************************************/
      //TGK: disabled getHeaderLine, and replaced with GetHeader from tes.cpp
      splitTes[i].ReadHeader(splitTes[i].filename); 
      dateCreatedLine = splitTes[i].GetHeader("DateCreated:");
      rawFileLine = splitTes[i].GetHeader("RawFile:");
      subjectNameLine = splitTes[i].GetHeader("SubjectName:");
      subjectAgeLine = splitTes[i].GetHeader("SubjectAge:");
      TRval = splitTes[i].voxsize[3];
      TELine = splitTes[i].GetHeader("TE(msecs):");
      orientationLine = splitTes[i].GetHeader("Orientation:");
      coilNameLine = splitTes[i].GetHeader("CoilName:");
      pulseSeqLine = splitTes[i].GetHeader("PulseSeq:");
      fieldStrengthLine = splitTes[i].GetHeader("FieldStrength:");
      taskTagLine = splitTes[i].GetHeader("TaskTag:");
      byteOrderLine = splitTes[i].GetHeader("Byteorder:");
    } 
    else {
      //TGK: all the below are substitutions of GetHeader tes method where getHeaderLine had been used
      if((origVoxSizes[0] != splitTes[i].voxsize[0])||(origVoxSizes[1]!= splitTes[i].voxsize[1])||(origVoxSizes[2]!= splitTes[i].voxsize[2])) {
        ostringstream errorMsg;
        errorMsg << "The voxel sizes [" << origVoxSizes[0]
        << ", " << origVoxSizes[1] << ", " << origVoxSizes[2] << "] from the first input file ["
        << splitTes[0].filename << "] do not match the voxel sizes [" << splitTes[i].voxsize[0]
        << ", " << splitTes[i].voxsize[1] << ", " << splitTes[i].voxsize[2]
        << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ((origOrigin[0] != splitTes[i].origin[0]) || (origOrigin[1] != splitTes[i].origin[1]) || (origOrigin[2] != splitTes[i].origin[2])) {
        ostringstream errorMsg;
        errorMsg << "The origin coordinates [" << origOrigin[0]
        << ", " << origOrigin[1] << ", " << origOrigin[2] << "] from the first input file ["
        << splitTes[0].filename << "] do not match the origin coordinates ["  << splitTes[i].origin[0]
        << ", " << splitTes[i].origin[1] << ", " << splitTes[i].origin[2]
        << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 

/*********************************************************************
* We now check to see if the header lines of splitTes[i] match the   *
* header lines of splitTes[0]. If there is a mismatch, then an error *
* message is printed and then this program will exit.                *
*********************************************************************/
      if ( (dateCreatedLine.length() > 0) && (dateCreatedLine != splitTes[i].GetHeader("DateCreated:")) ) {
        ostringstream errorMsg;
        errorMsg << "The creation date [" << dateCreatedLine << "] from the first input file [" << splitTes[0].filename << "] does not match the creation date [" << splitTes[i].GetHeader("DateCreated:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (rawFileLine.length() > 0) && (rawFileLine != splitTes[i].GetHeader("RawFile:")) ) {
        ostringstream errorMsg;
        errorMsg << "The raw file name [" << rawFileLine << "] from the first input file [" << splitTes[0].filename << "] does not match the raw file name [" << splitTes[i].GetHeader("RawFile:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (subjectNameLine.length() > 0) && (subjectNameLine != splitTes[i].GetHeader("SubjectName:")) ) {
        ostringstream errorMsg;
        errorMsg << "The subject name [" << subjectNameLine << "] from the first input file [" << splitTes[0].filename << "] does not match the subject name date [" << splitTes[i].GetHeader("SubjectName:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (subjectAgeLine.length() > 0) && (subjectAgeLine != splitTes[i].GetHeader("SubjectAge:")) ) {
        ostringstream errorMsg;
        errorMsg << "The subject age [" << subjectAgeLine << "] from the first input file [" << splitTes[0].filename << "] does not match the subject age [" << splitTes[i].GetHeader("SubjectAge:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( TRval>0 && TRval != splitTes[i].voxsize[3] ) {
        ostringstream errorMsg;
        errorMsg << "The TR [" << TRval << "] from the first input file [" << splitTes[0].filename << "] does not match the TR [" << splitTes[i].voxsize[3] << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      }
      if ( (TELine.length() > 0) && (TELine != splitTes[i].GetHeader("TE(msecs):")) ) {
        ostringstream errorMsg;
        errorMsg << "The TE line [" << TELine << "] from the first input file [" << splitTes[0].filename << "] does not match the TE line [" << splitTes[i].GetHeader("TE(msecs):") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (orientationLine.length() > 0) && (orientationLine != splitTes[i].GetHeader("Orientation:")) ) {
        ostringstream errorMsg;
        errorMsg << "The orientation [" << orientationLine << "] from the first input file [" << splitTes[0].filename << "] does not match the orientation [" << splitTes[i].GetHeader("Orientation:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (coilNameLine.length() > 0) && (coilNameLine != splitTes[i].GetHeader("CoilName:")) ) {
        ostringstream errorMsg;
        errorMsg << "The coil name [" << coilNameLine << "] from the first input file [" << splitTes[0].filename << "] does not match the coil name [" << splitTes[i].GetHeader("CoilName:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (pulseSeqLine.length() > 0) && (pulseSeqLine != splitTes[i].GetHeader("PulseSeq:")) ) {
        ostringstream errorMsg;
        errorMsg << "The pulse sequence [" << pulseSeqLine << "] from the first input file [" << splitTes[0].filename << "] does not match the pulse sequence [" << splitTes[i].GetHeader("PulseSeq:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (fieldStrengthLine.length() > 0) && (fieldStrengthLine != splitTes[i].GetHeader("FieldStrength:")) ) {
        ostringstream errorMsg;
        errorMsg << "The field strength [" << fieldStrengthLine << "] from the first input file [" << splitTes[0].filename << "] does not match the field strength [" << splitTes[i].GetHeader("FieldStrength:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (taskTagLine.length() > 0) && (taskTagLine != splitTes[i].GetHeader("TaskTag:")) ) {
        ostringstream errorMsg;
        errorMsg << "The task tag [" << taskTagLine << "] from the first input file [" << splitTes[0].filename << "] does not match the task tag [" << splitTes[i].GetHeader("TaskTag:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if ( (byteOrderLine.length() > 0) && (byteOrderLine != splitTes[i].GetHeader("Byteorder:")) ) {
        ostringstream errorMsg;
        errorMsg << "The byte order [" << byteOrderLine << "] from the first input file [" << splitTes[0].filename << "] does not match the byte order [" << splitTes[i].GetHeader("Byteorder:") << "] from the current input file [" << splitTes[i].filename << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (origDataType != splitTes[i].datatype) {
        ostringstream errorMsg;
        errorMsg << " The current data type [" << splitTes[i].datatype << "] does not match the first data type [" << origDataType << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (origDimT != (int ) strtol(S[5])) {
        ostringstream errorMsg;
        errorMsg << " The current original dimT [" << (int ) strtol(S[5]) << "] does not equal the first original dimT [" << origDimT << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (dimX != splitTes[i].dimx) {
        ostringstream errorMsg;
        errorMsg << " The current dimX [" << splitTes[i].dimx << "] does not equal the first dimX [" << dimX << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (dimY != splitTes[i].dimy) {
        ostringstream errorMsg;
        errorMsg << " The current dimY [" << splitTes[i].dimy << "] does not equal the first dimY [" << dimY << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (dimZ != splitTes[i].dimz) {
        ostringstream errorMsg;
        errorMsg << " The current dimZ [" << splitTes[i].dimz << "] does not equal the first dimZ [" << dimZ << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
      if (splitType != S[1]) {
        ostringstream errorMsg;
        errorMsg << " The file [" << splitTes[i].filename << "] has split type [" << S[1] << "]. This does not match the split type of the first file [" << splitTes[0].filename << "] which is [" << splitType << "].";
        printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
      } 
    } 
  } 
  Tes outputTes = Tes();
  outputTes.SetVolume(dimX, dimY, dimZ, origDimT, origDataType);
  short tSeriesCheck[origDimT];
  memset(tSeriesCheck, 0, origDimT * sizeof(short));
  double *paramValues = NULL;
  if (moveParams) {
    paramValues = new double[origDimT * MOVE_PARAMS];
    memset(paramValues, 0, sizeof(double) * origDimT * MOVE_PARAMS);
  } 
  for (unsigned int i = 0; i < inputFiles.size(); i++) {
    tokenlist S(getLastTesSplitLine(splitTes[i]));
    if (splitType == "Divided") {
      /*********************************************************************
      * Recall that if the split type is "Divided", then the range of time *
      * series from the original Tes file will be in field number 4        *
      * (counting from 0). The range will resemble: 2-10.                  *
      *********************************************************************/
      string range = S[4];
      tokenlist S2(range,"-");
      int beginRange = (int ) strtol(S2[0]);
      int endRange = (int ) strtol(S2[1]);
      OPEN_SPATIAL_LOOPS(splitTes[i])
        /*********************************************************************
        * The following for loop is used to traverse the current input file, *
        * splitTes[i], and outputTes, the output Tes object. This for loop   *
        * copies the time series from the current input file to outputTes so *
        * the "joined" Tes file can be created. rangeIndex indexes the time  *
        * dimension in outputTes and splitDimTIndex indexes the time         *
        * dimension for splitTes[i].                                         *
        *********************************************************************/
        for (int rangeIndex = beginRange, splitDimTIndex = 0; rangeIndex <= endRange; rangeIndex++, splitDimTIndex++) {
          /*********************************************************************
          * The appropriate element of tSeriesCheck[] is set to 1 to indicate  *
          * that we have retrieved time series number rangeIndex for outputTes.*
          *********************************************************************/
          tSeriesCheck[rangeIndex] = 1;
          outputTes.SetValue(indexX, indexY, indexZ, rangeIndex, splitTes[i].GetValue(indexX, indexY, indexZ, splitDimTIndex));
        } 
      CLOSE_SPATIAL_LOOPS
        if (moveParams) {
          /*********************************************************************
           * A Vec object is created from the split movement parameter file     *
           * that corresponds to the split Tes file.                            *
           *********************************************************************/
          VB_Vector pVec;
          getVec(inputFiles[i], splitTes[i].dimt, pVec);
          memcpy(paramValues + (beginRange * MOVE_PARAMS), pVec.getData(), sizeof(double) * pVec.size());
        }
    } 
    else if (splitType == "Binned") {
      int numBins = strtol(S[4]);
      OPEN_SPATIAL_LOOPS(splitTes[i])
	/*********************************************************************
	* The following for loop is used copy the time series values from    *
	* the current input file (a "split Tes" file) into outputTes, the    *
	* "joined" Tes object. Say that the original 4D data file had dimT   *
	* equal to 6 and that numBins is 2. Then we have the following       *
	* situation for the split Tes files:                                 *
	*                                                                    *
	* FILE:                  TIME SERIES:                                *
	* -----------------------------------------------------              *
	* split_file1.tes        {0, 2, 4}                                   *
	* split_file2.tes        {1, 3, 5}                                   *
	*                                                                    *
	* For split_file1.tes, the first time series it received from the    *
	* original 4D data file is time series number 0. The number of the   *
	* second time series it received is (0 + numBins) = 2. The number    *
	* of the third (and final) time series it received is                *
	* (2 + numBins) = 4. Similarly for split_file2.tes. Therefore, the   *
	* time dimension index in outputTes begins with the number of the    *
	* first time series that the split Tes file received (from the       *
	* original unsplit 4D data file) and then is incremented by numBins  *
	* in each iteration of the following for loop.                       *
	*                                                                    *
	* startSeries is initialized to the number of the first time series  *
	* copied to splitTes[i] and then incremented by numBins.             *
	* splitIndex is initialized to 0 and incremented by 1. splitIndex is *
	* used to index the time dimension in splitTes[i].                   *
	*********************************************************************/
        for(int splitIndex=0,startSeries=strtol(S[6]);splitIndex<splitTes[i].dimt;splitIndex++,startSeries+= numBins) {
          /*********************************************************************
          * The appropriate element of tSeriesCheck[] is set to 1 to indicate  *
          * that we have retrieved time series number startSeries for          *
          * outputTes.                                                         *
          *********************************************************************/
          tSeriesCheck[startSeries] = 1;
          outputTes.SetValue(indexX, indexY, indexZ, startSeries, splitTes[i].GetValue(indexX, indexY, indexZ, splitIndex));
        } 
      CLOSE_SPATIAL_LOOPS
      if (moveParams) {
        VB_Vector pVec;
        getVec(inputFiles[i], splitTes[i].dimt, pVec);
        /*********************************************************************
	* The following for loop is used to copy the values from the split   *
	* movement parameters file to the array paramValues[]. Say that the  *
	* original Tes file, i.e., the source file for the split Tes files   *
	* being processed by this program, had dimT equal to 4 and that an   *
	* even/odd split was carried out. Then there are 2 split Tes files   *
	* and the following situation holds:                                 *
	*                                                                    *
	* FILE:               TIME SERIES:                                   *
	* ----------------------------------------------------------------   *
	* split_file0.tes     {0, 2}                                         *
	* split_file1.tes     {1, 3}                                         *
	*                                                                    *
	* So each pVec created from one of the split movement parameter      *
	* files will have a total of 14 values and paramValues[] will be of  *
	* length 28. The index k is initialized to the number of the first   *
	* time series appearing in the current split Tes file and            *
	* (k * MOVE_PARAMS) is the offset from paramValues[0] during each    *
	* iteration of the for loop. (j * MOVE_PARAMS) is the offset from    *
	* pVec->data that points to the values that need to be copied to     *
	* paramVlaues[] during each iteration of the for loop. Finally, the  *
	* number of bytes that need to be copied is                          *
	* (sizeof(double) * MOVE_PARAMS).                                    *
	*********************************************************************/
        for (size_t j = 0, k = (int ) strtol(S[6]); j < (pVec.size() / MOVE_PARAMS); j++, k += numBins)
          memcpy(paramValues + (k * MOVE_PARAMS), pVec.getData() + (j * MOVE_PARAMS), sizeof(double) * MOVE_PARAMS);
      } 
    }
    else {
      ostringstream errorMsg;
      errorMsg << " Unrecognized split type: [" << splitType << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
  } 
  /*********************************************************************
  * If the user elected to join the movement parameter files as well,  *
  * then by this point the array paramValues[] has been populated      *
  * with the all the movement parameter values. We now write out the   *
  * joined movement parameters file and then delete the memory that    *
  * had been allocated to paramValues[] previously.                    *
  *********************************************************************/
  if (moveParams) {
    writeMoveParamsFile(paramValues, outputFile, origDimT, inputFiles[0]);
    delete [] paramValues;
    paramValues=0;
  } 

  /*********************************************************************
  * As each time series was copied from a input file to outputTes, the *
  * corresponding element in tSeriesCheck[] was set to 1. The following*
  * for loop traverses tSeriesCheck[] looking for zeroes. If a zero    *
  * is found, then an error message is printed, and then this program  *
  * exits.                                                             *
  *********************************************************************/
  for (int i = 0; i < origDimT; i++) {
    if (tSeriesCheck[i] == 0) {
      ostringstream errorMsg;
      errorMsg << " Did not find the time series number [" << i << "]. Invalid join.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
  } 
  copyHeader(&(splitTes[inputFiles.size() - 1]), &outputTes);
  addHeaderLine(&outputTes, "Sfff", "VoxSizes(XYZ):",
  splitTes[0].voxsize[0], splitTes[0].voxsize[1], splitTes[0].voxsize[2]);
  addHeaderLine(&outputTes, "Siii", "Origin(XYZ):",
  splitTes[0].origin[0], splitTes[0].origin[1], splitTes[0].origin[2]);
  string timeStr;
  addHeaderLine(&outputTes, "SSS", "TesJoin:", timedate().c_str(), splitType.c_str());
  outputTes.SetFileName(outputFile);
  if (outputTes.WriteFile()) {
    ostringstream errorMsg;
    errorMsg << " Unable to write file [" << outputFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } 
  if (deleteFiles) {
    for (unsigned int i = 0; i < inputFiles.size(); i++) {
      errno = 0;
      if (unlink(inputFiles[i].c_str()) < 0) {
        ostringstream errorMsg;
        errorMsg << "Unable to delete file [" << inputFiles[i].c_str() << "] due to: [" << strerror(errno) << "].";
        printErrorMsg(VB_WARNING, errorMsg.str());
      } 
      if (moveParams) {
        string paramFile = inputFiles[i].substr(0, inputFiles[i].length() - 4) + "_MoveParams.ref";
        errno = 0;
        if (unlink(paramFile.c_str()) < 0) {
          ostringstream errorMsg;
          errorMsg << "Unable to delete file [" << paramFile.c_str() << "] due to: [" << strerror(errno) << "].";
          printErrorMsg(VB_WARNING, errorMsg.str());
        } 
      } 
    } 
    errno = 0;
    if ((directoryName.size() > 0) && (rmdir(directoryName.c_str()) == -1) && (errno != ENOTEMPTY)) {
      ostringstream errorMsg;
      errorMsg << "Unable to delete directory [" << directoryName << "] due to: [" << strerror(errno) << "].";
      printErrorMsg(VB_WARNING, errorMsg.str());
    } 
  } 
  return 0;
} 

void usage(const unsigned short exitValue, char *progName) {
  genusage(exitValue, progName, "- Tes file joining routine for VoxBo.",
           "-h -d[directory] -f[file name] -l[file list] -o[output file name] -m -r -v",
           "-h                        Print usage information. Optional.",
           "-d <directory>            Specify the directory containing the *.tes",
           "                          files to be joined.",
           "-f <file name>            Specify the file holding the names of the",
           "                          *.tes files to be joined.",
           "-l <file list>            List the *.tes files to be joined, enclosed",
           "                          in double quotes and space delimited.",
           "                          NOTE: Exactly one of \"-d\", \"-f\", or \"-l\"",
           "                          is required.",
           "-o <output file name>     Specify the output file name. Optional.",
           "                          Default output file name will be the original",
           "                          file name.",
           "-m                        Join the movement parameters files as well. Optional.",
           "-r                        If used, then remove the input files. Optional.",
           "-v                        Global VoxBo version number. Optional.",
           "");

} 

/*********************************************************************
* This function returns true if the input file is a regular file and *
* readable by this process. Otherwise, false is returned.            *
*********************************************************************/
bool verifyFile(const string& fileName) {
  if (vb_fileexists(fileName) && !access(fileName.c_str(),R_OK))
    return true;
  return false;
} 

/*********************************************************************
* This function returns a Vec object derived from the movement       *
* parameters that corresponds to the input Tes file name.            *
* Additionally, this function verifies that the movement parameters  *
* file is readable by this process, that its file format is t_ref1   *
* (a VoxBo vector file), that the Vec object was created             *
* successfully, and that the length data member of the Vec object    *
* is of the required size.                                           *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* tesFile            const string&   The Tes file name.              *
* dimT               const int       The size of the time dimension  *
*                                    of the input Tes file.          *
* pVec               Vec *           Pointer to the Vec object       *
*                                    that gets populated by the      *
*                                    values from the movement        *
*                                    parameters files.               *
*                                                                    *
* OUTPUT VARIABLES:  TYPE:           DESCRIPTION:                    *
* -----------------  -----           ------------                    *
* None.                                                              *
*********************************************************************/
void getVec(const string& tesFile, const int dimT, VB_Vector &pVec) {
  string paramFile = tesFile.substr(0, tesFile.length() - 4) + "_MoveParams.ref";
  if (!access(paramFile.c_str(),R_OK)) {
    vector<VBFF> listoftypes = EligibleFileTypes(paramFile);
    if ((listoftypes.size() == 0) || (listoftypes[0].getDimensions() != 1)) {
      ostringstream errorMsg;
      errorMsg << "The movement parameters file [" << paramFile << "] is not a VoxBo vector file.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    pVec.ReadFile(paramFile);
    if (!pVec.getState()) {
      ostringstream errorMsg;
      errorMsg << "Unable to read the movement parameters file [" << paramFile << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
    /*********************************************************************
    * For each time point, there must be MOVE_PARAMS movement parameters.*
    * So if the following conditional is true, then we know with         *
    * certainty that this split movement parameters file does not        *
    * correspond to the Tes object splitTes[i]. Therefore, an error      *
    * message is printed and then this program exits.                    *
    *********************************************************************/
    if ((dimT * MOVE_PARAMS) != (int)(pVec.size())) {
      ostringstream errorMsg;
      errorMsg << "Tes file [" << tesFile << "] does not correspond to movement parameters file [" << paramFile << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } 
  } 
  else {
    ostringstream errorMsg;
    errorMsg << "The movement parameters file [" << paramFile << "] is not readable.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } 
} 

/*********************************************************************
* This function writes out the joined movement parameters file.      *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* paramValues        const double *  The array of movement           *
*                                    parameter values.               *
* outputFile         const string&   The name of the joined Tes file.*
* origDimT           const int       The dimT of the joined Tes File.*
* firstSplitFile     const string&   The name of the first split     *
*                                    Tes file.                       *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* None.                                                              *
*********************************************************************/
void writeMoveParamsFile(const double *paramValues, const string& outputFile, const int origDimT, const string& firstSplitFile) {
  string outputParamFile = outputFile.substr(0, outputFile.size() - 4) + "_MoveParams.ref";
  errno = 0;
  FILE *fVec = fopen(outputParamFile.c_str(), "w");
  if (!fVec) {
    ostringstream errorMsg;
    errorMsg << "Unable to open REF1 output file [" << outputParamFile << "] for write due to [" << strerror(errno) << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } 
  string firstMoveParamsFile = firstSplitFile.substr(0, firstSplitFile.length() - 4) + "_MoveParams.ref";
  errno = 0;
  FILE *pFile = fopen(firstMoveParamsFile.c_str(), "r");
  if (!pFile) {
    ostringstream errorMsg;
    errorMsg <<"Unable to open first split movement parameters file ["<<firstMoveParamsFile<<"] for reading due to ["<<strerror(errno)<<"].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } 
  char line[STRINGLEN];
  memset(line, 0, STRINGLEN);
  while (fgets(line, STRINGLEN, pFile)) {
    /*********************************************************************
    * If the first character in line[] is not a ';', then we have read   *
    * all the header line so we break out of this while loop.            *
    *********************************************************************/
    if (line[0] != ';')
      break;
    fprintf(fVec, "%s", line);
  } 
  fprintf(fVec, "\n");
  for (int j = 0; j < origDimT * MOVE_PARAMS; j++)
    fprintf(fVec, "%g\n", paramValues[j]);
  fclose(fVec);
} 

/*********************************************************************
* This function returns the last "TesSplit:" header line from the    *
* input Tes object.                                                  *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* theTes             const Tes&      The input Tes object whose      *
*                                    header will be searched for     *
*                                    a "TesSplit:" line.             *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* N/A                 string          The last "TesSplit:" line to   *
*                                     appear in the header of the    *
*                                     input Tes object.              *
*********************************************************************/
string getLastTesSplitLine(const Tes& theTes) {
  string tesSplitLine = "";
  for (int i = theTes.header.size() - 1; i >= 0; i--) {
    tokenlist S(theTes.header[i]);
    if (S[0] == "TesSplit:") {
      tesSplitLine = theTes.header[i];
      break;
    } 
  } 

  /*********************************************************************
  * If tesSplitLine is still the empty string, then no "TesSplit:"     *
  * line exists in the header of theTes Therefore, this is not a split *
  * Tes file. An error message is printed and then this program exits. *
  *********************************************************************/
  if (tesSplitLine.length() == 0) {
    ostringstream errorMsg;
    errorMsg << "The file [" << theTes.filename << "] is not a split 4D data file.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
  } 

  return tesSplitLine;

} 

