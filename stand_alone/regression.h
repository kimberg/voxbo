
// regression.h
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

#ifndef REGRESSION_H
#define REGRESSION_H

#include <gsl/gsl_fit.h>
#include <cctype>
#include <fstream>
#include <vector>
#include "vb_common_incs.h"

void readInTesFiles(const string& matrixStemName,
                    vector<string>& tesList) throw();

void makeMatrixK(const string& matrixStemName) throw();
void makeMatrixKG(const string& matrixStemName) throw();
void makeMatrixF3(const string& matrixStemName) throw();
bool isFileReadable(const string& fileName) throw();

unsigned short checkTesDims(const unsigned short dimT,
                            const unsigned short dimX,
                            const unsigned short dimY,
                            const unsigned short dimZ,
                            const Tes& theTes) throw();

unsigned short shiftLeft(const unsigned short index,
                         const unsigned short dimIndex);

void computeTraces(const string& matrixStemName);

#endif  // REGRESSION_H
