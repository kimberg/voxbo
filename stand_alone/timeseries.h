
// timeseries.h
// Copyright (c) 1998-2010 by The VoxBo Development Team
//
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
 * Include guard.                                                     *
 *********************************************************************/
#ifndef TIMESERIES_H
#define TIMESERIES_H

/*********************************************************************
 * Required include files.                                            *
 *********************************************************************/
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include "genericexcep.h"
#include "utils.h"
#include "vb_common_incs.h"

class TimeSeries {
 private:
  /*********************************************************************
   * DATA MEMBERS:                                                      *
   * currentSeries - keeps track of the current time series. The range  *
   *                 for currentSeries is [0, dimx * dimy * dimz - 1].  *
   * dimT - the number of time series.                                  *
   * dimX - the number of voxels in the X direction.                    *
   * dimY - the number of voxels in the Y direction.                    *
   * dimZ - the number of voxels in the Z direction.                    *
   * indexX - used to hold the X spatial index corresponding to a voxel *
   *          position.                                                 *
   * indexY - used to hold the Y spatial index corresponding to a voxel *
   *          position.                                                 *
   * indexZ - used to hold the Z spatial index corresponding to a voxel *
   *          position.                                                 *
   * theTes - a Tes object whose time series is to be read.             *
   * tesFileName - the 4D data file name.                               *
   *********************************************************************/
  int32 currentSeries;
  int32 dimX, dimY, dimZ, dimT;
  int32 indexX, indexY, indexZ;
  Tes theTes;
  string tesFileName;

  /*********************************************************************
   * Private methods.                                                   *
   *********************************************************************/
  void init(const string &tesFile) throw(GenericExcep);
  void init(const Tes &theTes) throw(GenericExcep);

 public:
  /*********************************************************************
   * Constructors:                                                      *
   *********************************************************************/
  TimeSeries();
  TimeSeries(const string &tesFileName);
  TimeSeries(const char *tesFileName);
  TimeSeries(const Tes &theTes);
  TimeSeries(const TimeSeries &T);  // Copy constructor.

  /*********************************************************************
   * Destructor:                                                        *
   *********************************************************************/
  ~TimeSeries();

  /*********************************************************************
   * Accessor methods.                                                  *
   *********************************************************************/
  unsigned int getDimX() const;
  unsigned int getDimY() const;
  unsigned int getDimZ() const;
  unsigned int getDimT() const;

  unsigned int getIndexX() const;
  unsigned int getIndexY() const;
  unsigned int getIndexZ() const;
  unsigned long getCurrentSeries() const;

  Tes *getTesFile();

  /*********************************************************************
   * Public methods.                                                    *
   *********************************************************************/
  int getCurrentTimeSeries(double *tSeries);
  void reset();
  void zeroOut(const unsigned int x, const unsigned int y,
               const unsigned int z) throw();
  void zeroOut(const unsigned long index) throw();
  void toString() const;
  unsigned int getTimeSeriesIndex() const;
  int getTimeSeries(double *tSeries, const unsigned int i) throw();

  void getTimeSeries(double *tSeries, const unsigned int xIndex,
                     unsigned int yIndex, const unsigned int zIndex) throw();
  int getSameSeries(double *tSeries) throw();

};  // class TimeSeries

#endif  // TIMESERIES_H
