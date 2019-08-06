
// time_series_avg.h
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
#ifndef TIME_SERIES_AVG_H
#define TIME_SERIES_AVG_H

/*********************************************************************
 * Required include files.                                            *
 *********************************************************************/
#include <algorithm>
#include <string>
#include <vector>
#include "vb_common_incs.h"
#include "vbutil.h"

/*********************************************************************
 * Using the standard namespace.                                      *
 *********************************************************************/
using namespace std;

/*********************************************************************
 * Required structs.                                                  *
 *********************************************************************/
struct coords3D {
  unsigned short x;
  unsigned short y;
  unsigned short z;
};  // typedef struct coords3D

/* >>>>>>>>>>>>           FUNCTION PROTOTYPES          <<<<<<<<<<<< */

bool checkBoundsXYZ(const short x, const short y, const short z,
                    const unsigned short dimX, const unsigned short dimY,
                    const unsigned short dimZ);

bool checkBoundsXYZ(const coords3D *theCoords, const coords3D *theDims);

unsigned short getTesValue(const unsigned long index, const Tes *theTes,
                           const unsigned short tIndex, double &signal);

unsigned short regionalTimeSeries(const vector<unsigned long> &theRegion,
                                  const vector<string> &tesNames,
                                  vector<vector<double> > &theSeries,
                                  bool meanNormFlag = false) throw();

template <class Iterator>
void scale(Iterator start, Iterator end, const double factor);
template <class Iterator>
double average(Iterator start, Iterator end);

/* >>>>>>>>>>>>         END FUNCTION PROTOTYPES        <<<<<<<<<<<< */

#endif  // TIME_SERIES_AVG_H
