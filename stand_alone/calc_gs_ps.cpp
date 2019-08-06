
// calc_gs_ps.cpp
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
 * This file implements the classes for CalcGS and CalcPs.            *
 *********************************************************************/

#include "calc_gs_ps.h"
const string CalcGs::gsHeader1 =
    ";VB98\n;REF1\n;\n; Global signal calculated from ";
const string CalcGs::gsHeader2 = "\n;   on ";
const string CalcGs::gsHeader3 =
    "\n;\n; This is the average of all (brain) voxel time-series\n;   "
    "normalized to have unit variance.\n;\n";

CalcGs::CalcGs(const string& tFile) { this->init(tFile); }

CalcGs::CalcGs(const char* tFile) { this->init(tFile); }

CalcGs::CalcGs(const CalcGs& c) {
  this->gsFile = c.gsFile;
  this->unitVar = c.unitVar;
  this->oldMean = c.oldMean;
  this->oldStdDev = c.oldStdDev;
  this->sigArr = c.sigArr;
  this->gsTes = c.gsTes;
  this->gsTSeries = c.gsTSeries;
  this->timeStr = c.timeStr;
}

void CalcGs::init(const string& tFile) {
  this->gsTes = Tes(tFile);
  if (!this->gsTes.data_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "]  The input 4D data file ["
             << tFile << "] has invalid data.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }

  /*********************************************************************
   * Now we have to add the current date and time to the header that    *
   * will be written to the gsFile (the output file). For example, if   *
   * if this program was invoked on a 4D data file on May 24, 1998      *
   * 16:28:47, we need to add the following line to the header:         *
   *                                                                    *
   *                 Sun_May_24_16:28:47_1998                           *
   *                                                                    *
   *********************************************************************/
  this->timeStr = timedate();

  /*********************************************************************
   * The header of the output file must resemble the following:         *
   *                                                                    *
   * ;VB98                                                              *
   * ;REF1                                                              *
   * ;                                                                  *
   * ; Global signal calculated from lattin02/lattin02.tes              *
   * ;   on Mon_Feb_26_15:45:49_2001                                    *
   * ;                                                                  *
   * ; This is the average of all (brain) voxel time-series             *
   * ;   normalized to have unit variance.                              *
   * ;                                                                  *
   *                                                                    *
   *                                                                    *
   *                                                                    *
   * (NOTE: The preceding 3 blank lines are a part of the header.)      *
   * The 2 variable parts of the header are the subdirectory and 4D data*
   * file name, e.g., "lattin02/lattin02.tes" and the date and time of  *
   * when this class was instantiated (which roughly matches the date   *
   * and time of when a program using this class was invoked), e.g.,    *
   * "Mon_Feb_26_15:45:49_2001". The 3 constant parts of the headers    *
   * are:                                                               *
   * 1. From the first character of the header to the space character   *
   *    immediately before the subdirectory and 4D data file name. This *
   *    is what the static data member gsHeader1 has been set to.       *
   * 2. The character immediately after the subdirectory and 4D data    *
   *    file name to the space character immediately before the date and*
   *    time. The static data member gsHeader2 has been assigned to     *
   *    this part.                                                      *
   * 3. The new line immediately after the date and time to the new     *
   *    line that defines the last line (which, in fact is blank) of    *
   *    the header. The static data member gsHeader3 has been assigned  *
   *    this part.                                                      *
   *                                                                    *
   * We have already calculated the date and time. Now we need to       *
   * figure out the subdirectory and file name.                         *
   *                                                                    *
   * The GS output file name will consist of the 4D data file name      *
   * (without the ".tes" part) concatenated with "_GS.ref".             *
   *********************************************************************/
  this->gsFile =
      xdirname(tFile) + "/" + xfilename(xrootname(tFile)) + "_GS.ref";
  this->unitVar = false;
  this->gsTSeries = Tes(this->gsTes.filename);
  this->oldMean = 0.0;
  this->oldStdDev = 0.0;
}

CalcGs::~CalcGs() {}

string CalcGs::getTesFile() const { return this->gsTes.filename; }

string CalcGs::getGsFile() const { return this->gsFile; }

void CalcGs::getSignals() {
  this->sigArr.resize(this->gsTes.dimt);
  /*********************************************************************
   * Say we have:                                                       *
   *                                                                    *
   *        dimt = 3, dimx = dimy = dimz = 2                            *
   *                                                                    *
   * This then implies that the data member "data" (which is an array)  *
   * of the object this->gsTes looks like the signal column:            *
   *                                                                    *
   *        TIME, XYZ coord.         SIGNAL VALUE                       *
   *        ----------------         ------------                       *
   *        [t0  x0  y0  z0] ------> s0                                 *
   *        [t1  x0  y0  z0] ------> s1                                 *
   *        [t2  x0  y0  z0] ------> s2                                 *
   *                                                                    *
   *        [t0  x1  y0  z0] ------> s3                                 *
   *        [t1  x1  y0  z0] ------> s4                                 *
   *        [t2  x1  y0  z0] ------> s5                                 *
   *                                                                    *
   *        [t0  x0  y1  z0] ------> s6                                 *
   *        [t1  x0  y1  z0] ------> s7                                 *
   *        [t2  x0  y1  z0] ------> s8                                 *
   *                                                                    *
   *        [t0  x1  y1  z0] ------> s9                                 *
   *        [t1  x1  y1  z0] ------> s10                                *
   *        [t2  x1  y1  z0] ------> s11                                *
   *                                                                    *
   *        [t0  x0  y0  z1] ------> s12                                *
   *        [t1  x0  y0  z1] ------> s13                                *
   *        [t2  x0  y0  z1] ------> s14                                *
   *                                                                    *
   *        [t0  x1  y0  z1] ------> s15                                *
   *        [t1  x1  y0  z1] ------> s16                                *
   *        [t2  x1  y0  z1] ------> s17                                *
   *                                                                    *
   *        [t0  x0  y1  z1] ------> s18                                *
   *        [t1  x0  y1  z1] ------> s19                                *
   *        [t2  x0  y1  z1] ------> s20                                *
   *                                                                    *
   *        [t0  x1  y1  z1] ------> s21                                *
   *        [t1  x1  y1  z1] ------> s22                                *
   *        [t2  x1  y1  z1] ------> s23                                *
   *                                                                    *
   * (Of course, if all the signal values are zero for a particular time*
   * series, then the time series does not get stored.)                 *
   * This then means that sigArr's length is 3 and we want to           *
   * populate sigArr in the following way:                              *
   *                                                                    *
   * sigArr[0] = s0 + s3 + s6 + s9  + s12 + s15 + s18 + s21             *
   * sigArr[1] = s1 + s4 + s7 + s10 + s13 + s16 + s19 + s22             *
   * sigArr[2] = s2 + s5 + s8 + s11 + s14 + s17 + s20 + s23             *
   *                                                                    *
   * Each time series will be read in (for example, the signal values   *
   * {s0, s1, s2} comprise the time series t0) and then added to the    *
   * array this->sigArr. NOTE: This class' TimeSeries data member is    *
   * used to read in the time series values.                            *
   *********************************************************************/

  /*********************************************************************
   * A time series will be read into tempSigArr. We need to do some     *
   * manipulation to the time series before it is added to this->sigArr *
   * and the manipulations will be carried out in signal. NOTE: The     *
   * logic for processing time series is derived from the IDL           *
   * procedure CalcGSPS, found in VoxBo_DataPrep.pro. Both tempSigArr   *
   * and signal are now declared and their elements will all be zero.   *
   *********************************************************************/
  VB_Vector tempSigArr(this->gsTes.dimt);
  VB_Vector signal(this->gsTes.dimt);

  /*********************************************************************
   * The following while loop is used to retrieve each time series into *
   * tempSigArr and then the body of the while loop processes the       *
   * signal values. The while loop will exit when zero is returned by   *
   * this->gsTSeries.getCurrentTimeSeries().                            *
   *********************************************************************/
  while (this->gsTSeries.getCurrentTimeSeries(tempSigArr.getData())) {
    signal += tempSigArr;
    double total = signal.getVectorSum() / (double)this->gsTes.dimt;
    signal -= total;
  }
  signal /= this->gsTes.realvoxels;
  sigArr += signal;
}

/*********************************************************************
 * This method writes the *_GS.ref file. If for some reason, the time *
 * series signal values fail to be read from the 4D data file, 0 is   *
 * returned. Otherwise, 1 is returned.                                *
 *********************************************************************/
int CalcGs::printGsData() {
  ofstream outFile(this->gsFile.c_str());
  if (!outFile) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "]  Unable to open ["
             << this->gsFile << "] to write header.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 7);
  }

  if (this->unitVar) {
    outFile << CalcGs::gsHeader1 << xdirname(this->gsTes.filename) << "/"
            << xfilename(this->gsTes.filename) << CalcGs::gsHeader2
            << this->timeStr << CalcGs::gsHeader3;
    outFile << "; stdev " << oldStdDev << endl;
    outFile << "; mean " << oldMean << endl << endl << endl;
  } else {
    outFile << CalcGs::gsHeader1 << xdirname(this->gsTes.filename) << "/"
            << xfilename(this->gsTes.filename) << CalcGs::gsHeader2
            << this->timeStr
            << "\n;\n; This is the average of all (brain) voxel time-series\n; "
               "  not normalized to have unit variance.\n;\n\n\n";
  }

  if (this->sigArr.getLength() == 0) this->getSignals();

  if (this->sigArr.getLength() > 0) {
    outFile << setprecision(GS_PRECISION);
    for (int i = 0; i < this->gsTes.dimt; i++) {
      outFile.width(FIELD_WIDTH);
      outFile << this->sigArr[i] << endl;
    }
    outFile.close();
    return (1);
  }
  return (0);
}

/*********************************************************************
 * This method normalizes this->sigArr to have unit variance. If for  *
 * some reason, the signal values fail to be retrieved from the 4D    *
 * data file, 0 is returned. Otherwise, 1 is returned.                *
 *********************************************************************/
int CalcGs::normalizeVariance() {
  if (this->sigArr.getLength() == 0) this->getSignals();
  double stanDev = sqrt(this->sigArr.getVariance());
  this->oldStdDev = stanDev;
  this->oldMean = this->sigArr.getVectorMean();
  if (this->sigArr.getLength() > 0) {
    this->sigArr.unitVariance();
    this->unitVar = true;
    return (1);
  }
  return (0);
}

const string CalcPs::psHeader1 =
    ";VB98\n;REF1\n;\n; Grand mean power spectrum calculated from ";
const string CalcPs::psHeader2 = "\n;   on ";
const string CalcPs::psHeader3 =
    "\n;\n; This is the average of all (brain) voxel power-spectra\n;\n\n\n";

CalcPs::CalcPs(const string& tFile) { this->init(tFile); }

CalcPs::CalcPs(const char* tFile) { this->init(tFile); }

CalcPs::CalcPs(const CalcPs& c) {
  this->tesFile = c.tesFile;
  this->psFile = c.psFile;
  this->sigArr = c.sigArr;
  this->timeStr = c.timeStr;
  this->psTes = c.psTes;
  this->psTSeries = c.psTSeries;
}

void CalcPs::init(const string& tFile) {
  this->tesFile = tFile;
  this->psTes = Tes(tFile);
  if (!this->psTes.data_valid) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "]  The input 4D data file ["
             << tFile << "] could not be read.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }

  /*********************************************************************
   * Now we have to add the current date and time to the header that    *
   * will be written to the psFile (the output file). For example, if   *
   * if this program was invoked on a 4D data file on May 24, 1998      *
   * 16:28:47, we need to add the following line to the header:         *
   *                                                                    *
   *                 Sun_May_24_16:28:47_1998                           *
   *                                                                    *
   *********************************************************************/
  this->timeStr = timedate();

  /*********************************************************************
   * The header of the output file must resemble the following:         *
   *                                                                    *
   * ;VB98                                                              *
   * ;REF1                                                              *
   * ;                                                                  *
   * ; Grand mean power spectrum calculated from lattin02/lattin02.tes  *
   * ;   on Mon_Feb_26_15:45:49_2001                                    *
   * ;                                                                  *
   * ; This is the average of all (brain) voxel power-spectra           *
   * ;                                                                  *
   *                                                                    *
   *                                                                    *
   *                                                                    *
   * (NOTE: The preceding 3 blank lines are a part of the header.)      *
   * The 2 variable parts of the header are the subdirectory and 4D data*
   * file name, e.g., "lattin02/lattin02.tes" and the date and time of  *
   * when this class was instantiated (which roughly matches the date   *
   * and time of when a program using this class was invoked), e.g.,    *
   * "Mon_Feb_26_15:45:49_2001". The 3 constant parts of the headers    *
   * are:                                                               *
   * 1. From the first character of the header to the space character   *
   *    immediately before the subdirectory and 4D data file name. This *
   *    is what the static data member psHeader1 has been set to.       *
   * 2. The character immediately after the subdirectory and 4D data    *
   *    file name to the space character immediately before the date and*
   *    time. The static data member psHeader2 has been assigned to     *
   *    this part.                                                      *
   * 3. The new line immediately after the date and time to the new     *
   *    line that defines the last line (which, in fact is blank) of    *
   *    the header. The static data member psHeader3 has been assigned  *
   *    this part.                                                      *
   *                                                                    *
   * We have already calculated the date and time. Now we need to       *
   * figure out the subdirectory and file name.                         *
   *********************************************************************/

  /*********************************************************************
   * The output file name will consist of the 4D data file name (without*
   * the ".tes" part) concatenated with "_PS.ref".                      *
   *********************************************************************/
  this->psFile =
      xdirname(tFile) + "/" + xfilename(xrootname(tFile)) + "_PS.ref";
  this->psTSeries = TimeSeries(this->tesFile);
}

CalcPs::~CalcPs() {}

string CalcPs::getTesFile() const { return this->psTes.filename; }

string CalcPs::getPsFile() const { return this->psFile; }

void CalcPs::getSignals() {
  /*********************************************************************
   * Now resizing the vector that will hold the Grand Mean Power        *
   * Spectrum. Then the array is initialized to all zeros.              *
   *********************************************************************/
  this->sigArr.resize(this->psTes.dimt);
  /*********************************************************************
   * A time series will be read into tempSigArr. We need to do some     *
   * manipulation to the time series before it is added to              *
   * this->sigArr and the manipulations will be carried out in          *
   * signal. NOTE: The logic for processing time series is derived      *
   * from the IDL procedure CalcPSPS, found in VoxBo_DataPrep.pro. The  *
   * data member this->psTSeries is used to retrieve each of the time   *
   * series. Both tempSigArr and signal are declared and all of their   *
   * elements will be zero.                                             *
   *********************************************************************/
  VB_Vector tempSigArr(this->psTes.dimt);
  VB_Vector signal(this->psTes.dimt);
  /*********************************************************************
   * The following while loop is used to retrieve each time series. The *
   * body of the while loop processes the signal values in a time       *
   * series.                                                            *
   *********************************************************************/
  while (this->psTSeries.getCurrentTimeSeries(tempSigArr.getData())) {
    double total = tempSigArr.getVectorSum() / (double)this->psTes.dimt;
    tempSigArr -= total;
    signal = tempSigArr;
    this->getPowerSpectrum(signal);
    signal /= (double)this->psTes.realvoxels;
    this->sigArr += signal;
  }
}

/*********************************************************************
 * This method writes the *_PS.ref file. If for some reason, the time *
 * series signal values fail to be read from the 4D data file, 0 is   *
 * returned. Otherwise, 1 is returned.                                *
 *********************************************************************/
int CalcPs::printPsData() {
  ofstream outFile(this->psFile.c_str());
  if (!outFile) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "]  Unable to open ["
             << this->psFile << "] to write header.";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 7);
  }

  outFile << CalcPs::psHeader1 << xdirname(this->psTes.filename) << "/"
          << xfilename(this->psTes.filename) << CalcPs::psHeader2
          << this->timeStr << CalcPs::psHeader3;
  if (this->sigArr.getLength() == 0) this->getSignals();

  if (this->sigArr.getLength() > 0) {
    outFile << setprecision(GS_PRECISION);
    for (int i = 0; i < this->psTes.dimt; i++) {
      outFile.width(FIELD_WIDTH);
      outFile << this->sigArr[i] << endl;
    }
    outFile.close();
    return (1);
  }
  return (0);
}

/*********************************************************************
 * This method places the results from the computation:               *
 *                                                                    *
 *        FFT(inputArr) * ComplexConjugate(FFT(inputArr))             *
 *                                                                    *
 * (where "*" represents multiplication of corresponding elements     *
 * from the two arrays) into inputArr[]. The above computation is     *
 * carried out by the following steps:                                *
 *                                                                    *
 * 1. FFT of inputArr[] which equals a vector of the form:            *
 *                                                                    *
 *        a(1) + ib(1)                                                *
 *        a(2) + ib(2)                                                *
 *        a(3) + ib(3)                                                *
 *             .                                                      *
 *             .                                                      *
 *             .                                                      *
 *        a(n) + ib(n), where i = sqrt(-1).                           *
 *                                                                    *
 * 2. Note that the complex conjugate of the above vector is:         *
 *                                                                    *
 *        a(1) - ib(1)                                                *
 *        a(2) - ib(2)                                                *
 *        a(3) - ib(3)                                                *
 *             .                                                      *
 *             .                                                      *
 *             .                                                      *
 *        a(n) - ib(n)                                                *
 *                                                                    *
 * 3. Now element by element multiplication of the vector from (1)    *
 *    and the vector from (2) is simply:                              *
 *                                                                    *
 *        a(1)^2 + b(1)^2                                             *
 *        a(2)^2 + b(2)^2                                             *
 *        a(3)^2 + b(3)^2                                             *
 *               .                                                    *
 *               .                                                    *
 *               .                                                    *
 *        a(n)^2 + b(n)^2                                             *
 *                                                                    *
 *    Each element is the square of the modulus of each complex       *
 *    number found in the vector from step (1).                       *
 *                                                                    *
 * INPUTS:                                                            *
 * -------------------------------------------------------------------*
 * inputArr - pointer to an array of floats.                          *
 *********************************************************************/
void CalcPs::getPowerSpectrum(VB_Vector& inputArr) {
  VB_Vector realPart(inputArr.getLength());
  VB_Vector imagPart(inputArr.getLength());
  inputArr.fft(realPart, imagPart);
  realPart.elementByElementMult(realPart);
  imagPart.elementByElementMult(imagPart);
  inputArr = realPart + imagPart;
}
