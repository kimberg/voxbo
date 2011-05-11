
// sliceacq.h
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

/*********************************************************************
* Include guard.                                                     *
*********************************************************************/
#ifndef SLICEACQ_H
#define SLICEACQ_H

/*********************************************************************
* Required include files for sinc.cpp.                               *
*********************************************************************/
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "utils.h"
#include "vb_common_incs.h"

/*********************************************************************
* Needed constants.                                                  *
*********************************************************************/
const int ACQCORRECT_PREFS_LENGTH = 256;

/*********************************************************************
* The following struct is used to hold the values of TR, PulseSeq,   *
* and FieldStrength, found in the header of a Tes file.              *
*********************************************************************/
struct tesHeaderInfo
{
  unsigned short tr;
  string pulseSeq;
  double fieldStrength;
};

/* >>>>>>>>>>>>           FUNCTION PROTOTYPES          <<<<<<<<<<<< */

void sincInterpolate(const VB_Vector& timeSeries,
const unsigned int expFactor, VB_Vector& newSignal);

void phaseShift(const VB_Vector& tSeries, const double timeShift,
VB_Vector& shiftedSignal);

void makePhi(VB_Vector& phi, const double timeShift);

// qq get rid of getMaskValueByIndex?
int getMaskValueByIndex(const Tes& myTes, const unsigned int ind,
const unsigned int dimX, const unsigned int dimY,
const unsigned int dimZ);

void readParamFile(const string& paramFileName, const string& seq,
const double fieldStrength, double *sliceTime, int *pauseFlag,
const int tr, const int dimZ);

bool sanityCheckOfValues(const double sliceTime, const double firstSlice,
const int tr, const int dimZ);

/* >>>>>>>>>>>>         END FUNCTION PROTOTYPES        <<<<<<<<<<<< */

#endif // SLICEACQ_H
