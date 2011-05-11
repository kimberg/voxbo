
// calg_gs_ls.h
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

#ifndef CALC_GS_PS_H
#define CALC_GS_PS_H

/*********************************************************************
* This header file defines the classes CalcGs and CalcPs.            *
*********************************************************************/
#include <cstring>
#include <ctime>
#include <climits>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>
#include "vb_common_incs.h"
#include "timeseries.h"

const int FIELD_WIDTH = 13;
const int GS_PRECISION = 7;
class CalcGs {
  private:
/*********************************************************************
* DATA MEMBERS:                                                      *
* ------------------------------------------------------------------ *
*                                                                    *
* gsFile - name of the output GS file.                               *
* gsTes - a Tes object, instantiated by using the 4D data file name. *
* timeStr - a C++ string, used to hold the date and time of          *
*           when the current CalcGs object was instantiated.         *
* sigArr - will hold the sum of the modified time series signal      *
*          values.                                                   *
* unitVar - if true, then the user has chosen for this->sigArr to    *
*           have unit variance.                                      *
* gsTSeries - a TimeSeries object; used to process the signal values *
*             from the 4D data file.                                 *
* oldMean - if the user chose for the global spectrum to have unit   *
*           variance, then the mean prior to making the power        *
*           spectrum have unit variance will be stored in oldMean.   *
* oldStdDev - if the user chose for the global spectrum to have unit *
*             variance, then the standard deviation prior to making  *
*             the power spectrum have unit variance will be stored   *
*             in oldMean.                                            *
* gsHeader1 - first part of the header for the GS file.              *
* gsHeader2 - second part of the header for the GS file.             *
* gsHeader3 - third part of the header for the GS file.              *
*********************************************************************/
    string gsFile;
    Tes gsTes;
    string timeStr;
    VB_Vector sigArr;
    bool unitVar;
    TimeSeries gsTSeries;
    double oldMean;
    double oldStdDev;

    static const string gsHeader1;
    static const string gsHeader2;
    static const string gsHeader3;

/*********************************************************************
* PRIVATE METHODS:                                                   *
* ------------------------------------------------------------------ *
*                                                                    *
* init() - initializes those data members not initialized by the     *
*          regular constructors.                                     *
* getSignals() - gets the time series signal values from the 4D data *
*                file.                                               *
* getMeanForGS() - computes the mean of this->sigArr.                *
* getVarianceForGS() - computes the variance of this->sigArr.        *
*********************************************************************/
    void init(const string& tFile);
    void getSignals();

  public:

    CalcGs(const string& tFile);
    CalcGs(const char *tFile);
    CalcGs(const CalcGs& c); // Copy constructor.
    ~CalcGs();
    string getGsFile() const;
    string getTesFile() const;

/*********************************************************************
* PUBLIC METHODS:                                                    *
* ------------------------------------------------------------------ *
*                                                                    *
* normalizeVariance() - normalizes this->sigArr to have unit         *
*                       variance.                                    *
* printGsData() - prints out the GS information to a file.           *
*********************************************************************/
    int normalizeVariance();
    int printGsData();

}; 

class CalcPs {
  private:

/*********************************************************************
* DATA MEMBERS:                                                      *
* ------------------------------------------------------------------ *
*                                                                    *
* tesFile - holds the name of the 4D data file whose PS is to be     *
*           computed.                                                *
* psFile - name of the output PS file.                               *
* psTes - A Tes object, instantiated by using the 4D data file name. *
* timeStr - a C-style string, used to hold the date and time of      *
*           when the current CalcPs object was instantiated.         *
* sigArr - will hold the Grand Mean Power Spectrum.                  *
* psTSeries - A TimeSeries object; used to process the signal values *
*             from the 4D data file.                                 *
* psHeader1 - first part of the header for the PS file.              *
* psHeader2 - second part of the header for the PS file.             *
* psHeader3 - third part of the header for the PS file.              *
*********************************************************************/
    string tesFile;
    string psFile;
    Tes psTes;
    string timeStr;
    VB_Vector sigArr;
    TimeSeries psTSeries;

    static const string psHeader1;
    static const string psHeader2;
    static const string psHeader3;

/*********************************************************************
* PRIVATE METHODS:                                                   *
* ------------------------------------------------------------------ *
*                                                                    *
* init() - initializes those data members not initialized by the     *
*          regular constructors.                                     *
* getSignals() - gets the time series signal values from the 4D data *
*                file.                                               *
*********************************************************************/
    void init(const string& tesFile);
    void getSignals();
    void getPowerSpectrum(VB_Vector& inputArr);
  public:
    CalcPs(const string& tFile);
    CalcPs(const char *tFile);
    CalcPs(const CalcPs& c); // Copy constructor.
    ~CalcPs();
    string getTesFile() const;
    string getPsFile() const;
    int printPsData();

}; 

#endif // CALC_GS_PS_H
