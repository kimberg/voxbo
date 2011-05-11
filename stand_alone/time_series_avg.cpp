
// time_series_avg.cpp
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

#include "time_series_avg.h"

// useless function, really

unsigned short getTesValue(const unsigned long index, const Tes *theTes,
                           const unsigned short tIndex, double& signal) {
  int xx,yy,zz;
  theTes->getXYZ(xx,yy,zz,index);
  if (theTes->inbounds(xx,yy,zz)) {
    signal = theTes->GetValue(xx,yy,zz,tIndex);
    return (0);
  }
  return (1);
} 

/*********************************************************************
* This template function multiplies each element of the vector<>     *
* object, as defined by the 2 input iterators, by factor.            *
*********************************************************************/
template<class Iterator> void scale(Iterator start, Iterator end, const double factor) {
  while (start != end) {
    (*start) *= factor;
    start++;
  } 
} 

/*********************************************************************
* This template function computes the average of the vector<> object,*
* as defined by the 2 input iterators.                               *
*********************************************************************/
template<class Iterator> double average(Iterator start, Iterator end) {
  double sum = 0.0;
  unsigned long counter = 0;
  while (start != end) {
    sum += (*start);
    start++;
    counter++;
  } 
  return (sum / (double ) counter);
} 

/*********************************************************************
* This function retrieves the time series for the specifed voxels    *
* from the specified Tes files.                                      *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* theRegion  const vector<unsigned long>&  These integers specify    *
*                                          the region, i.e., the     *
*                                          set of voxels.            *
* tesNames   const vector<string>&   The Tes file names.             *
* theSeries  vector<vector<double> >&  This container will hold the  *
*                                      retrieved time series.        *
*                                                                    *
* EXAMPLE:                                                           *
*                                                                    *
* Say that one specifies voxels 23, 24, and 25 (which would be       *
* contained in theRegion) and the Tes files file1.tes and file2.tes  *
* (which would be contained in tesNames). Then upon returning from   *
* this function, theSeries would resemble:                           *
*                                                                    *
* theSeries[0]: t0 t1 t2 ... t(T-1)  (for voxel 23)                  *
* theSeries[1]: t0 t1 t2 ... t(T-1)  (for voxel 24)                  *
* theSeries[2]: t0 t1 t2 ... t(T-1)  (for voxel 25)                  *
*                                                                    *
* where T is the sum of the dimt's from file1.tes and file2.tes.     *
* Basically, theSeries would be a 3 by T "matrix".                   *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* Status flag.        unsigned short                                 *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
unsigned short regionalTimeSeries(const vector<unsigned long> &theRegion,
const vector<string> &tesNames, vector<vector<double> >& theSeries,
bool meanNormFlag) throw () {
  if (theRegion.size() == 0) {
    printErrorMsg(VB_ERROR, "The region size is zero.");
    return 5;
  } 
  if (tesNames.size() == 0) {
    printErrorMsg(VB_ERROR, "No Tes files were specified.");
    return 5;
  } 
  unsigned short firstX = 0;
  unsigned short firstY = 0;
  unsigned short firstZ = 0;
  double signal = 0.0;
  theSeries.clear();
  theSeries.resize(theRegion.size());
  vector<unsigned short> theDimTs(tesNames.size(), 0);
  for (unsigned long i = 0; i < tesNames.size(); i++) {
    Tes tempTes(tesNames[i]);
    if (!tempTes.data_valid) {
      ostringstream errorMsg;
      errorMsg << "Tes file [" << tesNames[i] << "] was unable to be read.";
      printErrorMsg(VB_ERROR, errorMsg.str());
      return 1;
    } 
    theDimTs[i] = tempTes.dimt;
    if (i == 0) {
       firstX = tempTes.dimx;
       firstY = tempTes.dimy;
       firstZ = tempTes.dimz;
    } 
    else {
      if ( (firstX != tempTes.dimx) || (firstY != tempTes.dimy) || (firstZ != tempTes.dimz) ) {
        ostringstream errorMsg;
        errorMsg << "Current spatial dimensions (" << tempTes.dimx << ", "
        << tempTes.dimy << ", " << tempTes.dimz << ") from Tes file ["
        << tesNames[i] << "] does not equal initial spatial dimensions (" << firstX
        << ", " << firstY << ", " << firstZ << ") from Tes file [" << tesNames[0] << "].";
        printErrorMsg(VB_ERROR, errorMsg.str());
        return 13;
      } 
    } 
    for (unsigned long j = 0; j < theRegion.size(); j++) {
      unsigned long currentSize = theSeries[j].size();
      theSeries[j].resize(theSeries[j].size() + tempTes.dimt);
      for (unsigned short k = 0; k < tempTes.dimt; k++) {
        getTesValue(theRegion[j], &tempTes, k, signal);
        theSeries[j][currentSize + k] = signal;
      } 
    } 
  } 

  if (meanNormFlag) {
    for (unsigned long j = 0; j < theDimTs.size(); j++) {
      unsigned long offSetBegin = 0;
      unsigned long offSetEnd = 0;
      for (unsigned long k = 0; k <=j; k++) {
        if (k > 0)
          offSetBegin += theDimTs[k - 1];
        offSetEnd += theDimTs[k];
      } 
      for (unsigned long i = 0; i < theSeries.size(); i++) {
        double avg = average(theSeries[i].begin() + offSetBegin, theSeries[i].begin() + offSetEnd);
        if (avg != 0.0)
          scale(theSeries[i].begin() + offSetBegin, theSeries[i].begin() + offSetEnd, 1.0 / avg);
      } 
    } 
  } 
  return 0;
} 
