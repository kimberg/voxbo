
// regress1.h
// VoxBo regression code
// Copyright (c) 1998-2006 by The VoxBo Development Team

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
// original version written by Dan Kimberg

#ifndef REGRESS1_H
#define REGRESS1_H

#include <gsl/gsl_fit.h>
#include <cctype>
#include <fstream>
#include <vector>
#include "vbio.h"

void readInTesFiles(const string& matrixStemName,
                    vector<string>& tesList) throw();

void makeMatrixK(const string& matrixStemName) throw();

void makeMatrixKG(const string& matrixStemName) throw();

void makeMatrixF3(const string& matrixStemName) throw();

bool isFileReadable(const string& fileName) throw();

void calculateBetas2(VB_Vector& theSignal, VBMatrix& f1Matrix,
                     VBMatrix& rMatrix, VB_Vector& realExokernel,
                     VB_Vector& imagExokernel, VB_Vector& residuals,
                     VB_Vector& allBetas);

const unsigned short checkTesDims(const unsigned short dimT,
                                  const unsigned short dimX,
                                  const unsigned short dimY,
                                  const unsigned short dimZ,
                                  const Tes& theTes) throw();

void assembleResidualPS(const string& matrixStemName,
                        const unsigned int numSteps);

unsigned short residualSmoothness(const string& matrixStemName,
                                  vector<double>& mapSmoothness);

const unsigned short shiftLeft(const unsigned short index,
                               const unsigned short dimIndex);

void computeTraces(const string& matrixStemName);
int makeMatrixR(tokenlist& args);
void makeFakeExofilt(string matrixStemName);
VB_Vector vecRegress(string ref, string matrixStemName, int autocorrelate);

#endif  // REGRESS1_H
