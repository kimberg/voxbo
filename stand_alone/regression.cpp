
// regression.cpp
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

/*********************************************************************
* Required include file.                                             *
*********************************************************************/
#include "regression.h"

/*********************************************************************
* Using the std namespace.                                           *
*********************************************************************/
using namespace std;

/*********************************************************************
* This function reads in names of Tes files from the "*.sub" file    *
* found in the directory specified by the input argument             *
* matrixStemName.                                                    *
*                                                                    *
* INPUT VARIABLES:  TYPE:            DESCRIPTION:                    *
* ----------------  -----            ------------                    *
* matrixStemName    const string &   Specifies the directory in      *
*                                    which the "*.sub" file exists.  *
* tesList           vector<string> & The container for the Tes       *
*                                    files.                          *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void readInTesFiles(const string &matrixStemName,
vector<string> &tesList) throw()
{

/*********************************************************************
* Assembling the name for the *.sub file.                            *
*********************************************************************/
  string subFile = matrixStemName + ".sub";

/*********************************************************************
* Now making sure that the *.sub file is  process readable. If it *
* is not readable, then this program will exit.                      *
*********************************************************************/
  if (!isFileReadable(subFile))
  {
    ostringstream errorMsg;
    errorMsg << "The sub file: [" << subFile << "] is not readable.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* If the file format of subFile is not a VoxBo text file, then an    *
* unrecoverable error has occurred. Therefore, an error message is   *
* assembled, printed, and then this program exits.                   *
*********************************************************************/
  if (utils::refOrTextOrMat(subFile) != utils::TXT_TYPE)
  {
    ostringstream errorMsg;
    errorMsg << "The file [" << subFile << "] is not a VoxBo text file.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Now we read the Tes file names from subFile. First, an input stream*
* is created.                                                        *
*********************************************************************/
  ifstream subFileStream(subFile.c_str(), ios::in);

/*********************************************************************
* If we were unable to open subFile for reading, then we have an     *
* unrecoverable error. An error message is printed and then this     *
* program exits.                                                     *
*********************************************************************/
  if (!subFileStream)
  {
    ostringstream errorMsg;
    errorMsg << "The file [" << subFile << "] could not be opened for reading.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* inputLine[] is used to hold a line read in from subFileStream. It  *
* is declared and then cleared.                                      *
*********************************************************************/
  char inputLine[STRINGLEN];
  memset(inputLine, 0, STRINGLEN);

  while (subFileStream.getline(inputLine, STRINGLEN))
  {

/*********************************************************************
* If the first character in inputLine[] is a ';' or inputLine[] is   *
* empty, then the line is ignored.                                   *
*********************************************************************/
    if ( (inputLine[0] == ';') || (strlen(inputLine) == 0) )
    {
      continue;
    } // if

/*********************************************************************
* The following for loop is used to determine the index of the first *
* non-whitespace character in inputLine[].                           *
*********************************************************************/
    unsigned short nwsIndex = 0;
    unsigned short commentIndex = 0;
    for (; nwsIndex < strlen(inputLine); nwsIndex++)
    {
      if (!isspace(inputLine[nwsIndex]))
      {
        break;
      } // if
    } // for nwsIndex

/*********************************************************************
* If nwsIndex equals strlen(inputLine), then inputLine[] consists    *
* solely of white space. Therefore, we simply continue.              *
*********************************************************************/
    if (nwsIndex == strlen(inputLine))
    {
      continue;
    } // if

/*********************************************************************
* The next step is to find out where a possible trailing comment     *
* begins in inputLine[].                                             *
*********************************************************************/
    for (commentIndex = nwsIndex; commentIndex < strlen(inputLine); commentIndex++)
    {
      if ( (inputLine[commentIndex] == ';') || (inputLine[commentIndex] == '#') )
      {
        break;
      } // if
    } // for commentIndex

/*********************************************************************
* If nwsIndex equals commentIndex, then we don't have any usable     *
* information in inputLine[]. Therefore, we simply continue.         *
*********************************************************************/
    if (nwsIndex == commentIndex)
    {
      continue;
    } // if

/*********************************************************************
* Now the "good part" of inputLine[] is copied to editedLine, i.e.,  *
* editedLine will not have any leading white space nor will it have  *
* and trailing comments.                                             *
*********************************************************************/
    char *editedLine = new char[commentIndex - nwsIndex + 1];
    memset(editedLine, 0, commentIndex - nwsIndex + 1);
    memcpy(editedLine, inputLine + nwsIndex, commentIndex - nwsIndex);

    if (access(editedLine,R_OK))
    {

/*********************************************************************
* Some *.sub files don't list the Tes files with the ".tes" suffix.  *
* Therefore, we now append ".tes" to editedLine and then check to    *
* see if that file is readable or not. If that file is not readable, *
* a warning message is printed out.                                  *
*********************************************************************/
      if (access(((string)editedLine+".tes").c_str(),R_OK))
      {
        char resolvedPath[PATH_MAX];
        memset(resolvedPath, 0, PATH_MAX);
        ostringstream errorMsg;
        errorMsg << "The file [" << string(editedLine) << ".tes"
                 << "] is not readable due to: [" << editedLine << ".tes" << "].";
        printErrorMsg(VB_WARNING, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
      } // if

/*********************************************************************
* If (editedLine + ".tes") is readable, then it is added to tesList. *
*********************************************************************/
      else
      {
        tesList.push_back(string(editedLine) + ".tes");
      } // else

    } // if

/*********************************************************************
* If the file in editedLine is readable, it is added to tesList.     *
*********************************************************************/
    else
    {
      tesList.push_back(string(editedLine));
    } // else

    delete [] editedLine;
    editedLine=0;

  } // while

/*********************************************************************
* Now closing subFileStream.                                         *
*********************************************************************/
  subFileStream.close();

} // void readInTesFiles(const string &matrixStemName,
  // vector<string> &tesList) throw()

/*********************************************************************
* This routine generates the "K" matrix, which contains the          *
* representation of temporal autocorrelation under the               *
* null-hypothesis. Currently, the only option available to the user  *
* for the estimation of intrinsic temporal autocorrelation is the    *
* average data PS. Therefore, the K matrix must reflect this model   *
* and the exogenous filter. This function is ported from the IDL     *
* procedure MakeMatrixK, found in VoxBo_GLM.pro.                     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* matrixStemName     const string&   The full path to the GLM        *
*                                    directory.                      *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void
makeMatrixK(const string& matrixStemName) throw()
{

/*********************************************************************
* If matrixStemName is the empty string, then an error message is    *
* printed and then this program exits.                               *
*********************************************************************/
  if (matrixStemName.size() == 0)
  {
    ostringstream errorMsg;
    errorMsg << "LINE: [" << __LINE__ << "] FUNCTION: [" << __FUNCTION__
    << "] The matrixStemName is empty.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

  string gMatrixFile(matrixStemName + ".G");

  VBMatrix gMatrix(gMatrixFile);
  if (!gMatrix.valid()) {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate the G matrix from the file [" << gMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  }

/*********************************************************************
* Assigning orderG.                                                  *
*********************************************************************/
  unsigned long orderG = gMatrix.m;

  string intrinCorFile(matrixStemName + ".IntrinCor");
  VB_Vector intrinCor(intrinCorFile);
  string exoFiltFile(matrixStemName + ".ExoFilt");
  VB_Vector exoFilt(exoFiltFile);
  if (!intrinCor.getState() || !exoFilt.getState()) {
    cout << "[E] error reading either intrincor or exofilt file\n";
    exit(101);
  }

/*********************************************************************
* If the length of exoFilt does not equal orderG, then an error      *
* message is printed and then this program exits.                    *
*********************************************************************/
  if (exoFilt.getLength() != orderG) {
    ostringstream errorMsg;
    errorMsg << "Exogenous filter length [" << exoFilt.getLength() << "] from file [" << exoFiltFile
    << "] does not match G matrix [" << orderG << "] from file [" << gMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Preparing the filters by:                                          *
*                                                                    *
* 1. Scaling each filter so that it's maximum element is 1.          *
* 2. Setting the zeroth element to 1 (to preserve the mean of the    *
*    signal).                                                        *
*********************************************************************/
  intrinCor.getPS();
  intrinCor.applyFunction(sqrt);
  intrinCor.scaleInPlace(1.0 / intrinCor.getMaxElement());
  intrinCor[0] = 1.0;
  exoFilt.getPS();
  exoFilt.applyFunction(sqrt);
  exoFilt.scaleInPlace(1.0 / exoFilt.getMaxElement());
  exoFilt[0] = 1.0;

/*********************************************************************
* Now instantiating the K matrix (with data type of double). The K   *
* matrix will be the identity matrix convolved with intrinCor and    *
* exoFilt.                                                           *
*********************************************************************/
  VBMatrix kMatrix(orderG, orderG);

/*********************************************************************
* If the kMatrix is not in a valid state, then an error message is   *
* printed and then this program exits.                               *
*********************************************************************/
  if (!kMatrix.valid())
  {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate a valid K matrix.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* We will need to compute the FFT of each row of kMatrix. To that    *
* end, realPart will hold the real part of the FFT and imagPart will *
* hold the imaginary part of the FFT.                                *
*********************************************************************/
  VB_Vector realPart(orderG);
  VB_Vector imagPart(orderG);

/*********************************************************************
* We will need to compute the inverse FFT (IFFT) of realPart and     *
* and imagPart. To that end, realPartReal will hold the real part of *
* realPart's IFFT; realPartImag will hold the imaginary part of      *
* realPart's IFFT; imagPartReal will hold the real part of imagPart's*
* IFFT; imagPartImag will hold the imaginary part of imagPart's      *
* IFFT.                                                              *
*********************************************************************/
  VB_Vector realPartReal(orderG);
  VB_Vector realPartImag(orderG);
  VB_Vector imagPartReal(orderG);
  VB_Vector imagPartImag(orderG);

/*********************************************************************
* cornerValRecip will hold the reciprocal of the K matrix's [0, 0]   *
* entry.                                                             *
*********************************************************************/
  double cornerValRecip = 0.0;

/*********************************************************************
* The following for loop is used to "convolve" the K matrix with     *
* intrinCor and exoFilt (but first, each row of the K matrix is      *
* turned into the corresponding row of the identity matrix).         *
*********************************************************************/
  for (unsigned long rowIndex = 0; rowIndex < orderG; rowIndex++)
  {

/*********************************************************************
* The next 2 lines create a VB_Vector object that holds row number   *
* rowIndex from the identity matrix.                                 *
*********************************************************************/
    VB_Vector kMatrixRow(orderG);
    kMatrixRow[rowIndex] = 1.0;

/*********************************************************************
* We now compute the FFT of kMatrixRow.                              *
*********************************************************************/
    kMatrixRow.fft(realPart, imagPart);

/*********************************************************************
* Now element-by-element multiplication is carried out between the   *
* FFT of tempVec, intrinCor, and exoFilt.                            *
*********************************************************************/
    realPart.elementByElementMult(intrinCor);
    imagPart.elementByElementMult(intrinCor);
    realPart.elementByElementMult(exoFilt);
    imagPart.elementByElementMult(exoFilt);

/*********************************************************************
* Now the inverse FFT of realPart and imagPart are computed.         *
*********************************************************************/
    realPart.ifft(realPartReal, realPartImag);
    imagPart.ifft(imagPartReal, imagPartImag);

/*********************************************************************
* At this point, we no longer need the values in realPart. Therefore,*
* realPart is resued to hold the real part of the inverse FFT of     *
* (realPart + i*imagPart).                                           *
*********************************************************************/
    realPart = realPartReal - imagPartImag;

/*********************************************************************
* If we are working on the first row of the K matrix, we get the     *
* reciporcal of its [0, 0] entry.                                    *
*********************************************************************/
    if (rowIndex == 0)
    {
      cornerValRecip = 1.0 / realPart[0];
    } // if

/*********************************************************************
* We normalize this row of the K matrix and place it in the K matrix.*
*********************************************************************/
    realPart.scaleInPlace(cornerValRecip);
    kMatrix.SetRow(rowIndex, realPart);

  } // for rowIndex

/*********************************************************************
* Setting the K matrix file name.                                    *
*********************************************************************/
  kMatrix.filename=matrixStemName + ".K";

/*********************************************************************
* Adding the "DateCreated:" and "TaskTag:" header lines.             *
*********************************************************************/
  kMatrix.AddHeader(string("DateCreated:\t") + timedate());
  kMatrix.AddHeader(string("TaskTag:\t") + "K matrix");

/*********************************************************************
* If writing out the K matrix file fails, then an error message is   *
* printed and then this program exits.                               *
*********************************************************************/
  if (kMatrix.WriteFile())
  {
    ostringstream errorMsg;
    errorMsg << "Unable to write the K matrix file ["
    << matrixStemName + ".K" << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

} // void makeMatrixK(const string& matrixStemName) throw()

bool
isFileReadable(const string& fileName) throw()
{
  if (access(fileName.c_str(),R_OK))
    return false;
  return true;
}

/*********************************************************************
* This function checks to see if the input Tes dimensions match the  *
* dimensions of the input Tes object. If the time dimensions do not  *
* match, 1 is returned. If the X dimensions do not match, 2 is       *
* returned. If the Y dimensions do not match, 3 is returned. If the  *
* Z dimensions do not match, 4 is returned. Also, an appropriate     *
* error message is printed out if a dimension does not match. If     *
* all dimensions match, 0 is returned. NOTE: It is up to the         *
* calling function to decide what to do if a dimension does not      *
* match.                                                             *
*                                                                    *
* INPUT VARIABLES: TYPE:                 DESCRIPTION:                *
* ---------------- -----                 ------------                *
* dimT             const unsigned short  The input time dimension.   *
* dimX             const unsigned short  The X time dimension.       *
* dimY             const unsigned short  The Y time dimension.       *
* dimZ             const unsigned short  The Z time dimension.       *
* theTes           const Tes&            The input Tes object.       *
*                                                                    *
* OUTPUT VARIABLES: TYPE:                DESCRIPTION:                *
* ----------------- -----                ------------                *
* 0                 const unsigned short If all dimensions match.    *
* 1                 const unsigned short If time dimensions do not   *
*                                        match.                      *
* 2                 const unsigned short If the X dimensions do not  *
*                                        match.                      *
* 3                 const unsigned short If the Y dimensions do not  *
*                                        match.                      *
* 4                 const unsigned short If the Z dimensions do not  *
*                                        match.                      *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
unsigned short checkTesDims(const unsigned short dimT,
			    const unsigned short dimX, const unsigned short dimY,
			    const unsigned short dimZ, const Tes& theTes) throw()
{

/*********************************************************************
* Checking the time dimensions.                                      *
*********************************************************************/
  if (dimT != theTes.dimt)
  {
    ostringstream errorMsg;
    errorMsg << "The input time dimension [" << dimT
    << "] does not match the time dimension of the input Tes file ["
    << theTes.filename << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    return 1;
  } // if

/*********************************************************************
* Checking the X dimensions.                                         *
*********************************************************************/
  if (dimX != theTes.dimx)
  {
    ostringstream errorMsg;
    errorMsg << "The input X dimension [" << dimX
    << "] does not match the X dimension of the input Tes file ["
    << theTes.filename << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    return 2;
  } // if

/*********************************************************************
* Checking the Y dimensions.                                         *
*********************************************************************/
  if (dimY != theTes.dimy)
  {
    ostringstream errorMsg;
    errorMsg << "The input Y dimension [" << dimY
    << "] does not match the Y dimension of the input Tes file ["
    << theTes.filename << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    return 3;
  } // if

/*********************************************************************
* Checking the Z dimensions.                                         *
*********************************************************************/
  if (dimZ != theTes.dimz)
  {
    ostringstream errorMsg;
    errorMsg << "The input Z dimension [" << dimZ
    << "] does not match the Z dimension of the input Tes file ["
    << theTes.filename << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    return 4;
  } // if

/*********************************************************************
* At this point, all the dimensions match. Therefore, 0 is returned. *
*********************************************************************/
  return 0;

} // const unsigned short checkTesDims(const unsigned short dimT,
  // const unsigned short dimX, const unsigned short dimY,
  // const unsigned short dimZ, const Tes& theTes) throw()

unsigned short shiftLeft(const unsigned short index,
			 const unsigned short dimIndex)
{
  if (index == 0)
  {
    return (dimIndex - 1);
  } // if
  return (index - 1);
} // const unsigned short shiftLeft(const unsigned short index,
  // const unsigned short dimIndex)

/*********************************************************************
* This routine generates the "KG" matrix, which contains covariates  *
* that reflect both endogenous and exogenous smoothing. This         *
* function is ported from the IDL procedure MakeMatrixKG, found in   *
* VoxBo_GLM.pro.                                                     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* matrixStemName     const string&   The full path to the GLM        *
*                                    directory.                      *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void
makeMatrixKG(const string& matrixStemName) throw()
{
  string gMatrixFile(matrixStemName + ".G");

/*********************************************************************
* Now a VBMatrix object is instantiated from gMatrixFile. If that    *
* fails, then we have an unrecoverable error. Therefore, an error    *
* message is printed and then this program exits.                    *
*********************************************************************/
  VBMatrix gMatrix(gMatrixFile);
  if (!gMatrix.valid())
  {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate the G matrix from the file [" << gMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Assigning orderG and rankG.                                        *
*********************************************************************/
  const unsigned long orderG = gMatrix.m;
  const unsigned long rankG = gMatrix.n;

  string exoFiltFile(matrixStemName + ".ExoFilt");

  VB_Vector exoFilt(exoFiltFile);

/*********************************************************************
* If the length of exoFilt does not equal orderG, then an error      *
* message is printed and then this program exits.                    *
*********************************************************************/
  //if (exoFilt.getLength() == 0)   //TGK: if file exists but no vector, copy G to KG
  //  flagNoExoFilt = 1;
  /*else*/ if (exoFilt.getLength() != orderG)
  { //TGK: if orderG is just wrong, it's an error
    ostringstream errorMsg;
    errorMsg << "Exogenous filter length [" << exoFilt.getLength() << "] from file [" << exoFiltFile
    << "] does not match G matrix [" << orderG << "] from file [" << gMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if
/*
  if (flagNoExoFilt) { //TGK: if no exofilt, we can use G for KG
    VBMatrix emptykgMatrix(gMatrix);
    emptykgMatrix.MakeInCore();
    if (!emptykgMatrix.valid) {
       ostringstream errorMsg;
       errorMsg << "Could not instantiate a valid KG matrix.";
       printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
    }
    emptykgMatrix.SetFileName(matrixStemName + ".KG");
    emptykgMatrix.AddHeader(string("DateCreated:\t") + timedate());
    emptykgMatrix.AddHeader(string("TaskTag:\t") + "KG matrix");
    if (emptykgMatrix.WriteData()) {
       ostringstream errorMsg;
       errorMsg << "Unable to write the K matrix file ["
       << matrixStemName + ".KG" << "].";
       printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
    }
    return;
  }
*/
/*********************************************************************
* Preparing the exogenous filter by:                                 *
*                                                                    *
* 1. Scaling each filter so that it's maximum element is 1.          *
* 2. Setting the zeroth element to 1 (to preserve the mean of the    *
*    signal).                                                        *
*********************************************************************/
  exoFilt.getPS();
  exoFilt.applyFunction(sqrt);
  exoFilt[0] = 1.0;

/*********************************************************************
* Now instantiating the KG matrix (with data type of double). The KG *
* matrix will be the G matrix convolved with exoFilt.                *
*********************************************************************/
  VBMatrix kgMatrix(orderG, rankG);

/*********************************************************************
* If the kgMatrix is not in a valid state, then an error message is  *
* printed and then this program exits.                               *
*********************************************************************/
  if (!kgMatrix.valid())
  {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate a valid KG matrix.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* We will need to compute the FFT of each row of kgMatrix. To that   *
* end, realPart will hold the real part of the FFT and imagPart will *
* hold the imaginary part of the FFT.                                *
*********************************************************************/
  VB_Vector realPart(rankG);
  VB_Vector imagPart(rankG);

/*********************************************************************
* We will need to compute the inverse FFT (IFFT) of realPart and     *
* and imagPart. To that end, realPartReal will hold the real part of *
* realPart's IFFT; realPartImag will hold the imaginary part of      *
* realPart's IFFT; imagPartReal will hold the real part of imagPart's*
* IFFT; imagPartImag will hold the imaginary part of imagPart's      *
* IFFT.                                                              *
*********************************************************************/
  VB_Vector realPartReal(rankG);
  VB_Vector realPartImag(rankG);
  VB_Vector imagPartReal(rankG);
  VB_Vector imagPartImag(rankG);

/*********************************************************************
* The following for loop is used to "convolve" the G matrix with     *
* intrinCor and exoFilt (but first, each row of the K matrix is      *
* turned into the corresponding row of the identity matrix).         *
*********************************************************************/
  for (unsigned long colIndex = 0; colIndex < rankG; colIndex++)
  {

/*********************************************************************
* We now fetch column number colIndex from the G matrix and          *
* instantiate a VB_Vector.                                           *
*********************************************************************/
    VB_Vector gMatrixCol(gMatrix.GetColumn(colIndex));

/*********************************************************************
* We now compute the FFT of gMatrixCol.                              *
*********************************************************************/
    gMatrixCol.fft(realPart, imagPart);

/*********************************************************************
* Now element-by-element multiplication is carried out between the   *
* FFT of tempVec, intrinCor, and exoFilt.                            *
*********************************************************************/
    realPart.elementByElementMult(exoFilt);
    imagPart.elementByElementMult(exoFilt);

/*********************************************************************
* Now the inverse FFT of realPart and imagPart are computed.         *
*********************************************************************/
    realPart.ifft(realPartReal, realPartImag);
    imagPart.ifft(imagPartReal, imagPartImag);

/*********************************************************************
* We now set column number colIndex in the KG matrix with the real   *
* real part of the inverse FFT of (realPart + i*imagPart).           *
*********************************************************************/
    kgMatrix.SetColumn(colIndex, realPartReal - imagPartImag);

  } // for colIndex

/*********************************************************************
* Setting the K matrix file name.                                    *
*********************************************************************/
  kgMatrix.filename=matrixStemName + ".KG";

/*********************************************************************
* Adding the "DateCreated:" and "TaskTag:" header lines.             *
*********************************************************************/
  kgMatrix.AddHeader(string("DateCreated:\t") + timedate());
  kgMatrix.AddHeader(string("TaskTag:\t") + "KG matrix");

/*********************************************************************
* If writing out the K matrix file fails, then an error message is   *
* printed and then this program exits.                               *
*********************************************************************/
  if (kgMatrix.WriteFile())
  {
    ostringstream errorMsg;
    errorMsg << "Unable to write the K matrix file ["
    << matrixStemName + ".KG" << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

} // void makeMatrixKG(const string& matrixStemName) throw()

/*********************************************************************
* This function writes out the file matrixStemName + ".traces".      *
*********************************************************************/
void computeTraces(const string& matrixStemName)
{
  string gMatrixFile(matrixStemName + ".G");

/*********************************************************************
* Now a VBMatrix object is instantiated from gMatrixFile. If that    *
* fails, then we have an unrecoverable error. Therefore, an error    *
* message is printed and then this program exits.                    *
*********************************************************************/
  VBMatrix gMatrix(gMatrixFile);
  if (!gMatrix.valid()) {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate the G matrix from the file [" << gMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* Assigning orderG.                                                  *
*********************************************************************/
  const long orderG = gMatrix.m;

/*********************************************************************
* Declaring the traces Vec object and then initializing it to all    *
* zeros.                                                             *
*********************************************************************/
  VB_Vector traces(3);
  traces*=3.0;

  string rvMatrixFile(matrixStemName + ".RV");

/*********************************************************************
* Now a VBMatrix object is instantiated from rvMatrixFile. If that   *
* fails, then we have an unrecoverable error. Therefore, an error    *
* message is printed and then this program exits.                    *
*********************************************************************/
  VBMatrix rvMatrix(rvMatrixFile);
  if (!rvMatrix.valid()) {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate the RV matrix from the file [" << rvMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* If the number of rows in the RV matrix does not equal the number   *
* of rows in the G matrix, we have an unrecoverable error. Therefore,*
* an error message is printed and then this program exits.           *
*********************************************************************/
  if (rvMatrix.m != orderG)
  {
    ostringstream errorMsg;
    errorMsg << "The number of rows [" << rvMatrix.m << "] for the RV matrix ["
    << rvMatrixFile << "] does not equal the number of rows [" << orderG
    << "] for the G matrix [" << matrixStemName << ".G].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* The following for loop is used to assign the trace of the RV       *
* matrix to traces[0].                                               *
*********************************************************************/
  for (long i = 0; i < rvMatrix.n; i++)
  {
    traces[0] += rvMatrix(i, i);
  } // for i


  string rvrvMatrixFile(matrixStemName + ".RVRV");
/*********************************************************************
* Now a VBMatrix object is instantiated from rvrvMatrixFile. If that *
* fails, then we have an unrecoverable error. Therefore, an error    *
* message is printed and then this program exits.                    *
*********************************************************************/
  VBMatrix rvrvMatrix(rvrvMatrixFile);
  if (!rvrvMatrix.valid())
  {
    ostringstream errorMsg;
    errorMsg << "Could not instantiate the RVRV matrix from the file [" << rvrvMatrixFile << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* If the number of rows in the RVRV matrix does not equal the number *
* of rows in the G matrix, we have an unrecoverable error. Therefore,*
* an error message is printed and then this program exits.           *
*********************************************************************/
  if (rvrvMatrix.m != orderG)
  {
    ostringstream errorMsg;
    errorMsg << "The number of rows [" << rvrvMatrix.m << "] for the RVRV matrix ["
    << rvrvMatrixFile << "] does not equal the number of rows [" << orderG
    << "] for the G matrix [" << matrixStemName << ".G].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

/*********************************************************************
* The following for loop is used to assign the trace of the RVRV     *
* matrix to traces[1]. However, all the diagonal elements of the     *
* RVRV matrix must be non-zero. If a zero valued diagonal element is *
* found, then this program exits after printing an error message.    *
*********************************************************************/
  for (long i = 0; i < rvrvMatrix.n; i++)
  {
    if (rvrvMatrix(i, i) != 0.0)
    {
      traces[1] += rvrvMatrix(i, i);
    } // if
    else
    {
      ostringstream errorMsg;
      errorMsg << "Found a zero diagonal entry for the RVRV matrix ["
      << rvrvMatrixFile << "] at [" << i << "].";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
    } // else

  } // for i

/*********************************************************************
* Now deleting the RV and RVRV matrix files. If an error occurs with *
* unlink(), a warning message is printed.                            *
*********************************************************************/
  errno = 0;
  if (unlink(rvMatrixFile.c_str()))
  {
    ostringstream errorMsg;
    errorMsg << "Unable to remove the file [" << matrixStemName + ".RV"
    << "] due to: [" << strerror(errno) << "].";
    printErrorMsg(VB_WARNING, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
  } // if
  errno = 0;
  if (unlink(rvrvMatrixFile.c_str())) {
    ostringstream errorMsg;
    errorMsg << "Unable to remove the file [" << matrixStemName + ".RVRV"
    << "] due to: [" << strerror(errno) << "].";
    printErrorMsg(VB_WARNING, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__);
  } // if

/*********************************************************************
* Now assigning trace[2], which is the effective degree of freedom.  *
*********************************************************************/
  traces[2] = (traces[0] * traces[0]) / traces[1];

/*********************************************************************
* Adding the appropriate header lines and then writing out the Vec   *
* file. If an error occurs during the the write, an error message is *
* printed and then this program exits.                               *
*********************************************************************/
  traces.AddHeader("");
  traces.AddHeader(" Traces calculated on " + timedate());
  traces.AddHeader("   Trace RV, TraceRVRV, and effdf (=TraceRV^2/TraceRVRV)");
  traces.AddHeader("");
  if (traces.WriteFile(matrixStemName + ".traces"))
  {
    ostringstream errorMsg;
    errorMsg << "Unable to write Vec file [" << matrixStemName << ".traces].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(),  __LINE__, __FUNCTION__, __FILE__, 1);
  } // if

} // void computeTraces(const string& matrixStemName)


