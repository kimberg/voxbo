
// timeseries.cpp
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

#include "timeseries.h"

TimeSeries::TimeSeries() {
  this->currentSeries = 0;
  this->dimX = 0;
  this->dimY = 0;
  this->dimZ = 0;
  this->dimT = 0;
  this->indexX = 0;
  this->indexY = 0;
  this->indexZ = 0;
  this->theTes = Tes();
  this->tesFileName = "";
}

TimeSeries::TimeSeries(const string &tesFileName) {
  try {
    this->init(tesFileName);
  } catch (GenericExcep &e) {
    e.what(__LINE__, __FILE__, __FUNCTION__);
  }
}

TimeSeries::TimeSeries(const Tes &theTes) {
  try {
    this->init(theTes.filename);
  } catch (GenericExcep &e) {
    e.what(__LINE__, __FILE__, __FUNCTION__);
  }
}

TimeSeries::TimeSeries(const char *tesFileName) {
  try {
    this->init(tesFileName);
  } catch (GenericExcep &e) {
    e.what(__LINE__, __FILE__, __FUNCTION__);
  }
}

TimeSeries::TimeSeries(const TimeSeries &T) {
  this->indexX = T.indexX;
  this->indexY = T.indexY;
  this->indexZ = T.indexZ;
  this->dimX = T.dimX;
  this->dimY = T.dimY;
  this->dimZ = T.dimZ;
  this->dimT = T.dimT;
  this->currentSeries = T.currentSeries;
  this->tesFileName = T.tesFileName;
  this->theTes = T.theTes;
}

TimeSeries::~TimeSeries() {}

void TimeSeries::init(const string &tesFileName) throw(GenericExcep) {
  this->tesFileName = tesFileName;
  this->indexX = this->indexY = this->indexZ = 0;
  this->currentSeries = 0;
  this->theTes = Tes(this->tesFileName);
  if (!this->theTes.data_valid) {
    string errMsg("Unable to read 4D data file: [" + this->tesFileName + "].");
    throw GenericExcep(__LINE__, __FILE__, __FUNCTION__, errMsg.c_str());
  }
  this->dimX = this->theTes.dimx;
  this->dimY = this->theTes.dimy;
  this->dimZ = this->theTes.dimz;
  this->dimT = this->theTes.dimt;
}

/*********************************************************************
 * This method is used to retrieve the next time series, i.e., the    *
 * next set of signal values.                                         *
 *                                                                    *
 * Assume that this->dimT = 3, this->dimX = this->dimY = this->dimZ   *
 * = 2. Then we have the following:                                   *
 *                                                                    *
 *         t        x        y        z        signal                 *
 * ------------------------------------------------------------------ *
 *       { 0        0        0        0        s0 }                   *
 *       { 1        0        0        0        s1 }-->Time Series 0   *
 *       { 2        0        0        0        s2 }                   *
 *                                                                    *
 *       { 0        1        0        0        s3 }                   *
 *       { 1        1        0        0        s4 }-->Time Series 1   *
 *       { 2        1        0        0        s5 }                   *
 *                                                                    *
 *       { 0        0        1        0        s6 }                   *
 *       { 1        0        1        0        s7 }-->Time Series 2   *
 *       { 2        0        1        0        s8 }                   *
 *                                                                    *
 *       { 0        1        1        0        s9 }                   *
 *       { 1        1        1        0        s10 }->Time Series 3   *
 *       { 2        1        1        0        s11 }                  *
 *                                                                    *
 *       { 0        0        0        1        s12 }                  *
 *       { 1        0        0        1        s13 }->Time Series 4   *
 *       { 2        0        0        1        s14 }                  *
 *                                                                    *
 *       { 0        1        0        1        s15 }                  *
 *       { 1        1        0        1        s16 }->Time Series 5   *
 *       { 2        1        0        1        s17 }                  *
 *                                                                    *
 *       { 0        0        1        1        s18 }                  *
 *       { 1        0        1        1        s19 }->Time Series 6   *
 *       { 2        0        1        1        s20 }                  *
 *                                                                    *
 *       { 0        1        1        1        s21 }                  *
 *       { 1        1        1        1        s22 }->Time Series 7   *
 *       { 2        1        1        1        s23 }                  *
 *                                                                    *
 * (Of course, a set of signal values, i.e., the set of signal values *
 * for {t0, t1, t2}, may all be zero. In such a case, the time series *
 * does not get stored.)                                              *
 *                                                                    *
 * By calling this method repeatedly, we will retrieve each of the    *
 * time series. The first call will get Times Series 0, the next call *
 * will get Time Series 1, etc.                                       *
 *                                                                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * tSeries            double *        A pointer to an array of        *
 *                                    doubles, of length this->dimT.  *
 *                                                                    *
 * OUTPUT VARIABLES:  TYPE:           DESCRIPTION:                    *
 * -----------------  -----           ------------                    *
 * tSeries            double *        Will hold a time series.        *
 * 1 or 0             int                                             *
 *                                                                    *
 * It is expected that this method will be used in a while loop.      *
 * Upon successful population of tSeries, 1 is returned. If all of    *
 * the time series have been retrieved, and this method is called     *
 * again, 0 is returned. Here is an example:                          *
 *                                                                    *
 *   while (myTimeSeries->getCurrentTimeSeries(tSeries))                 *
 *   {                                                                *
 *                                                                    *
 *     // Do something with the values in tSeries.                    *
 *                                                                    *
 *   }                                                                *
 *                                                                    *
 *********************************************************************/
int TimeSeries::getCurrentTimeSeries(double *tSeries) {
  if (this->currentSeries < (unsigned long)this->theTes.voxels) {
    /*********************************************************************
     * If the mask value is not 0 for the current values of this->indexX, *
     * this->indexY, and this->indexZ, then go ahead and get the time     *
     * series signal values. Recall that if the mask value is zero, then  *
     * all the signal values of the time series is 0, and so the time     *
     * series does not get stored.                                        *
     *********************************************************************/
    if (this->theTes.mask[this->currentSeries]) {
      theTes.getXYZ(this->indexX, this->indexY, this->indexZ,
                    this->currentSeries);  // TGK change
      if (this->theTes.GetTimeSeries(this->indexX, this->indexY, this->indexZ))
        return 0;
      memcpy(tSeries, this->theTes.timeseries.getData(),
             this->dimT * sizeof(double));
    } else
      memset(tSeries, 0, sizeof(double) * this->dimT);
    this->currentSeries++;
    return 1;
  } else {
    return 0;
  }
}

unsigned int TimeSeries::getDimX() const { return this->dimX; }

unsigned int TimeSeries::getDimY() const { return this->dimY; }

unsigned int TimeSeries::getDimZ() const { return this->dimZ; }

unsigned int TimeSeries::getDimT() const { return this->dimT; }

unsigned int TimeSeries::getIndexX() const { return this->indexX; }

unsigned int TimeSeries::getIndexY() const { return this->indexY; }

unsigned int TimeSeries::getIndexZ() const { return this->indexZ; }

unsigned long TimeSeries::getCurrentSeries() const {
  return this->currentSeries;
}

Tes *TimeSeries::getTesFile() { return &this->theTes; }

void TimeSeries::reset() { this->currentSeries = 0; }

void TimeSeries::zeroOut(const unsigned int x, const unsigned int y,
                         unsigned const int z) throw() {
  this->zeroOut(this->theTes.voxelposition(x, y, z));
}

void TimeSeries::toString() const {
  cout << "this->currentSeries = [" << this->currentSeries << "]" << endl;
  cout << "this->dimX =          [" << this->dimX << "]" << endl;
  cout << "this->dimY =          [" << this->dimY << "]" << endl;
  cout << "this->dimZ =          [" << this->dimZ << "]" << endl;
  cout << "this->dimT =          [" << this->dimT << "]" << endl;
  cout << "this->indexX =        [" << this->indexX << "]" << endl;
  cout << "this->indexY =        [" << this->indexY << "]" << endl;
  cout << "this->indexZ =        [" << this->indexZ << "]" << endl;
  cout << "this->tesFileName =   [" << this->tesFileName << "]" << endl;
}

unsigned int TimeSeries::getTimeSeriesIndex() const {
  return (this->getCurrentSeries() / this->dimT);
}

int TimeSeries::getTimeSeries(double *tSeries, const unsigned int i) throw() {
  if ((i >= (unsigned long)this->theTes.voxels))
    memset(tSeries, 0, this->theTes.dimt * sizeof(double));
  unsigned int tempCurrentSeries = this->currentSeries;
  this->currentSeries = i;
  int retVal = this->getCurrentTimeSeries(tSeries);
  this->currentSeries = tempCurrentSeries;
  return retVal;
}

/*********************************************************************
 * This method retrieves the time series for a specific set of        *
 * spatial coordinates (x, y, z).                                     *
 *                                                                    *
 * INPUT VARIABLES: TYPE:              DESCRIPTION:                   *
 * ---------------- -----              ------------                   *
 * tSeries          double *           This input array of doubles    *
 *                                     will be populated with the     *
 *                                     time series.                   *
 * xIndex           const unsigned int The x coordinate of the        *
 *                                     spatial point.                 *
 * yIndex           const unsigned int The y coordinate of the        *
 *                                     spatial point.                 *
 * zIndex           const unsigned int The z coordinate of the        *
 *                                     spatial point.                 *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void TimeSeries::getTimeSeries(double *tSeries, const unsigned int xIndex,
                               const unsigned int yIndex,
                               const unsigned int zIndex) throw() {
  this->getTimeSeries(tSeries,
                      this->theTes.voxelposition(xIndex, yIndex, zIndex));
}

int TimeSeries::getSameSeries(double *tSeries) throw() {
  int retVal = this->getCurrentTimeSeries(tSeries);
  this->currentSeries--;
  return retVal;
}

void TimeSeries::zeroOut(const unsigned long index) throw() {
  /*********************************************************************
   * Ensuring that index is a valid index in this->theTes.data[]. If,   *
   * for some reason, index is not a valid index, we take the view that *
   * the specified time series is "already zeroed out". Also, we need   *
   * to ensure that we have an  allocated time series at                *
   * this->theTes.data[index].                                          *
   *********************************************************************/
  if ((index < (unsigned long)this->theTes.voxels) &&
      (this->theTes.mask[index]))
    this->theTes.zerovoxel(index);
}
