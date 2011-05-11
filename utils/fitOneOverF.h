
// fitOneOverF.h
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
// original version written by Dongbo Hu

#ifndef FITONEOVERF_H
#define FITONEOVERF_H

#include "vbio.h"
#include "glmutil.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <vector>

struct data {
  size_t n;
  double * inputX;
  double * inputY;
  double * sigma;
};

/* functions required by nonlinear least-sqaures fitting */
int expb_f (const gsl_vector *fittingVar , void *params, gsl_vector * f);
int expb_df (const gsl_vector * fittingVar, void *params, gsl_matrix * J);
int expb_fdf (const gsl_vector * fittingVar, void *params, gsl_vector * f, gsl_matrix * J);
void print_state (size_t iter, gsl_multifit_fdfsolver * s);


/* generic functions to do the fitting */
VB_Vector * curvefit(VB_Vector *x, VB_Vector *y, VB_Vector *sigma, double var3min, double var1, 
		     double var2, double var3, const char * outputFile = '\0', bool printFlag = true);
/* Wrapper functions based on the generic curvefit() */
VB_Vector * curvefit (const char *xFilename, const char *yFilename, const char *sigmaFilename,
		     double var3min, const char *initFile, const char * outputFile);
VB_Vector * curvefit (const char *  xFilename, const char * yFilename, double inputSigma, double var3min,
		     double var1_init, double var2_init, double var3_init, const char *outputFile);
VB_Vector * curvefit (const char *xFilename, const char *yFilename, const char *sigmaFilename, double var3min,
		     double var1_init, double var2_init, double var3_init, const char * outputFile);

/* functions to fit the power spectrum by two parameters (var3 is fixed now) */
int expb_f12 (const gsl_vector *fittingVar , void *params, gsl_vector * f);
int expb_df12 (const gsl_vector * fittingVar, void *params, gsl_matrix * J);
int expb_fdf12 (const gsl_vector * fittingVar, void *params, gsl_vector * f, gsl_matrix * J);
void print_state12 (size_t iter, gsl_multifit_fdfsolver * s);
VB_Vector * curvefit12 (VB_Vector *xVec, VB_Vector *yVec, VB_Vector *sigmaVec, 
		      double var3, double var1_init, double var2_init, 
		      const char *outputFile, bool printFlag);

/* generic functions to deal with power spectrum files and ignorePS vectors */
VB_Vector * fitOneOverF (VB_Vector *psVec, VB_Vector *ignorePS, double var3min, double TR = 2000.0, 
			double sigma = 0.1, double var1 = 20.0, double var2 = 2.0, double var3 = 0, 
			const char * outputFile = '\0', bool printFlag = true);
/* Wrapper functions based on the generic fitOneOverF() */
VB_Vector * fitOneOverF (VB_Vector *psVec, double var3min, double TR = 2000.0, double sigma = 0.1, 
			double var1 = 20.0, double var2 = 2.0, double var3 = 0, 
			const char * outputFile = '\0', bool printFlag = true);
VB_Vector * fitOneOverF (const char *psFile, double var3min, double TR = 2000.0, double sigma = 0.1, 
			double var1 = 20.0, double var2 = 2.0, double var3 = 0, 
			const char * outputFile = '\0', bool printFlag = true);
VB_Vector * fitOneOverF (VB_Vector *psVec, const char *refFunc, double var3min, double TR = 2000.0, 
			double sigma = 0.1, double var1 = 20.0, double var2 = 2.0, double var3 = 0, 
			const char * outputFile = '\0', bool printFlag = true);
VB_Vector * fitOneOverF (const char *psFile, const char *refFunc, double var3min, double TR = 2000.0, 
			double sigma = 0.1, double var1 = 20.0, double var2 = 2.0, double var3 = 0, 
			const char * outputFile = '\0', bool printFlag = true);

/* Generic function to produce the time domain representation of the 1/f curve given three parameters */
VB_Vector * makeOneOverF (int numImages, double var1, double var2, double var3, double TRin = 2000);
/* wrapper function based on generic makeOneOverF() */
VB_Vector * makeOneOverF (int numImages, const char * paramFile, double TRin = 2000);


/* Functions used by generic fitOneOverF() */
int cmpVec(VB_Vector *shortVec, VB_Vector *longVec);

size_t findNonZero(VB_Vector *);

/* function to average multiple power spectrum files */
VB_Vector * averagePS(std::vector<string> psFiles);


#endif
