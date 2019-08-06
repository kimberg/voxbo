
// fitOneOverF.cpp
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

using namespace std;

#include "fitOneOverF.h"

double a2fixed = 0;  // global variable defined for two-parameter fitting

/*************************************************************************
 * expb_f is the function to be fitted. fittingVar is a gsl vector
 * which includes three variables to be fitted.
 * In GSL documentation, this variable is named "x",
 * which caused some confusion with the input X values.
 *
 * params is a variable of data type which includes the
 * number of elements in the input X and Y, input values of X and Y,
 * and standard deviation at each point.
 *************************************************************************/
int expb_f(const gsl_vector *fittingVar, void *params, gsl_vector *f) {
  size_t n = ((struct data *)params)->n;
  double *inputX = ((struct data *)params)->inputX;
  double *inputY = ((struct data *)params)->inputY;
  double *sigma = ((struct data *)params)->sigma;
  double a0 = gsl_vector_get(fittingVar, 0);
  double a1 = gsl_vector_get(fittingVar, 1);
  double a2 = gsl_vector_get(fittingVar, 2);

  for (size_t i = 0; i < n; i++) {
    /* Model Yi = 1.0 / (a0 * (Xi + a2)) + a1 */
    double t = inputX[i];
    double Yi = 1.0 / (a0 * (t + a2)) + a1;
    gsl_vector_set(f, i, (Yi - inputY[i]) / sigma[i]);
  }

  return GSL_SUCCESS;
}

/*************************************************************************
 * expb_df includes the first order partialderivatives of each parameter *
 *************************************************************************/
int expb_df(const gsl_vector *fittingVar, void *params, gsl_matrix *J) {
  size_t n = ((struct data *)params)->n;
  double *inputX = ((struct data *)params)->inputX;
  double *sigma = ((struct data *)params)->sigma;
  double a0 = gsl_vector_get(fittingVar, 0);
  double a2 = gsl_vector_get(fittingVar, 2);

  for (size_t i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj, */
    /* where fi = (Yi - yi)/sigma[i],      */
    /*       Yi = 1.0 / (a0 * (Xi + a2)) + a1  */
    /* and the xj are the parameters (a0, a1, a2) */
    double t = inputX[i];
    double s = 1.0 / sigma[i];
    double tmp = -1.0 / (a0 * (t + a2));

    gsl_matrix_set(J, i, 0, tmp / a0 * s);
    gsl_matrix_set(J, i, 1, s);
    gsl_matrix_set(J, i, 2, tmp * s / (t + a2));
  }

  return GSL_SUCCESS;
}

/************************************************************************
 * expb_fdf combines expb_f anf expb_df together.                       *
 ************************************************************************/
int expb_fdf(const gsl_vector *fittingVar, void *params, gsl_vector *f,
             gsl_matrix *J) {
  expb_f(fittingVar, params, f);
  expb_df(fittingVar, params, J);
  return GSL_SUCCESS;
}

/* Print out three parameters */
void print_state(size_t iter, gsl_multifit_fdfsolver *s) {
  printf("iter: %3d parameters are: %15.8f %15.8f %15.8f |f(x)| = %g\n",
         (int)iter, gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1),
         gsl_vector_get(s->x, 2), gsl_blas_dnrm2(s->f));
}

/*************************************************************************************
 * curvefit() is the main function to calculate the three fitting parameters.
 * This function requires 8 arguments:
 * #1: vector of x values for fitting
 * #2: vector of y values for fitting
 * #3: vector of sigma values at each point (not available in IDL's curvefit
 *function #4: initial guess for first parameter (steepness) #5: initial guess
 *for second parameter (y-offset) #6: initial guess for third parameter
 *(x-shift) #7: output file's name #8: print mode (true: print out a summary on
 *stdout; false: no any screen printout
 **************************************************************************************/
VB_Vector *curvefit(VB_Vector *xVec, VB_Vector *yVec, VB_Vector *sigmaVec,
                    double var3min, double var1_init, double var2_init,
                    double var3_init, const char *outputFile, bool printFlag) {
  /* fittingVec will include 8 elements: fittingStatus, # of iteration,
   * three fitting parameters' value, three deviation values
   * If status is 1.0, it means fitting is successful. 0 means fitting isn't
   * successful.*/
  VB_Vector *fittingVec = new VB_Vector(8);
  size_t N = xVec->getLength();
  const gsl_multifit_fdfsolver_type *T;
  gsl_multifit_fdfsolver *s;

  int status;
  size_t iter = 0;
  const size_t n = N;
  const size_t p = 3;
  gsl_matrix *covar = gsl_matrix_alloc(p, p);

  double inputX[N], inputY[N], sigma[N];
  struct data d = {n, inputX, inputY, sigma};
  gsl_multifit_function_fdf f;

  double fitting_init[3] = {var1_init, var2_init, var3_init};
  gsl_vector_view fittingVar = gsl_vector_view_array(fitting_init, p);
  const gsl_rng_type *type;
  gsl_rng *r;

  gsl_rng_env_setup();
  type = gsl_rng_default;
  r = gsl_rng_alloc(type);

  f.f = &expb_f;
  f.df = &expb_df;
  f.fdf = &expb_fdf;
  f.n = n;
  f.p = p;
  f.params = &d;

  for (size_t i = 0; i < n; i++) {
    inputX[i] = xVec->getElement(i);
    inputY[i] = yVec->getElement(i);

    /* Set sigma to be 0.1 at each point. This value is completely arbitrary.
     * It won't change the fitting variables' values, but the deviation of each
     * fitting variable (the value after "+/-") will be affected. */
    sigma[i] = sigmaVec->getElement(i);
  }

  T = gsl_multifit_fdfsolver_lmsder;
  s = gsl_multifit_fdfsolver_alloc(T, n, p);
  gsl_multifit_fdfsolver_set(s, &f, &fittingVar.vector);

  if (printFlag) print_state(iter, s);

  do {
    iter++;
    status = gsl_multifit_fdfsolver_iterate(s);
    if (printFlag) {
      printf("status = %s\n", gsl_strerror(status));
      print_state(iter, s);
    }

    // Make sure var3 is always larger than minimum. If not, call curverfit12()
    if (gsl_vector_get(s->x, 2) <= var3min) {
      double var3 = var3min * 0.99;
      printf(
          "******************************************************************"
          "\n");
      printf("x-shift minimum value reached.\n");
      printf("Arbitrarily set x-shift to %.5f and fit the other two ...\n",
             var3);
      printf(
          "******************************************************************"
          "\n");
      gsl_multifit_fdfsolver_free(s);
      return curvefit12(xVec, yVec, sigmaVec, var3, gsl_vector_get(s->x, 0),
                        gsl_vector_get(s->x, 1), outputFile, printFlag);
    }

    if (status) break;

    status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
  } while (status == GSL_CONTINUE && iter < 500);

  gsl_multifit_covar(s->J, 0.0, covar);

#define FIT(i) gsl_vector_get(s->x, i)
#define ERR(i) sqrt(gsl_matrix_get(covar, i, i))

  if (printFlag) {
    printf("\nElements in covariance matrix are:\n");
    gsl_matrix_fprintf(stdout, covar, "%g");

    printf("\nparameter #1: steepness = %.5f +/- %.5f\n", FIT(0), ERR(0));
    printf("parameter #2: y-offset  = %.5f +/- %.5f\n", FIT(1), ERR(1));
    printf("parameter #3: x-shift   = %.5f +/- %.5f\n", FIT(2), ERR(2));
    printf("status = %s\n\n", gsl_strerror(status));
  }

  // Write the fitting result to a file (if output filename is available)
  if (outputFile != '\0') {
    FILE *f1 = fopen(outputFile, "w");
    fprintf(f1, ";VB98\n");
    fprintf(f1, ";REF1\n");
    fprintf(f1, "; GSL nonlinear least-squares fitting\n");
    string myTime, myDate;
    maketimedate(myTime,
                 myDate);  // maketimedate() is defined in libvoxbo/vbutil.cpp
    fprintf(f1, "; Date created: %s_%s\n", myTime.data(), myDate.data());
    fprintf(f1, "; Initial guess: %f, %f, %f\n", var1_init, var2_init,
            var3_init);
    fprintf(f1, "; Number of iteration: %d\n", (int)iter);
    fprintf(f1, "; Status = %s\n", gsl_strerror(status));
    fprintf(f1, "; Deviation: %f, %f, %f\n\n", ERR(0), ERR(1), ERR(2));
    fprintf(f1, "%f\n%f\n%f\n", FIT(0), FIT(1), FIT(2));
    fclose(f1);
  }

  // Set elements in fittingVec. If status is 0 (success), set elements one by
  // one. Otherwise set all elements to be zero.
  if (status == 0) {
    fittingVec->setElement(0, 1.0);
    fittingVec->setElement(1, iter);
    fittingVec->setElement(2, FIT(0));
    fittingVec->setElement(3, FIT(1));
    fittingVec->setElement(4, FIT(2));

    fittingVec->setElement(5, ERR(0));
    fittingVec->setElement(6, ERR(1));
    fittingVec->setElement(7, ERR(2));
  } else
    fittingVec->setAll(0);

  gsl_multifit_fdfsolver_free(s);
  return fittingVec;
}

/* Two parameter version of expb_f */
int expb_f12(const gsl_vector *fittingVar, void *params, gsl_vector *f) {
  size_t n = ((struct data *)params)->n;
  double *inputX = ((struct data *)params)->inputX;
  double *inputY = ((struct data *)params)->inputY;
  double *sigma = ((struct data *)params)->sigma;

  double a0 = gsl_vector_get(fittingVar, 0);
  double a1 = gsl_vector_get(fittingVar, 1);

  for (size_t i = 0; i < n; i++) {
    /* Model Yi = 1.0 / (a0 * (Xi + var3)) + a1 */
    double t = inputX[i];
    double Yi = 1.0 / (a0 * (t + a2fixed)) + a1;
    gsl_vector_set(f, i, (Yi - inputY[i]) / sigma[i]);
  }

  return GSL_SUCCESS;
}

/***************************************************************************
 * expb_df12 includes the first order partialderivatives of each parameter *
 ***************************************************************************/
int expb_df12(const gsl_vector *fittingVar, void *params, gsl_matrix *J) {
  size_t n = ((struct data *)params)->n;
  double *inputX = ((struct data *)params)->inputX;
  double *sigma = ((struct data *)params)->sigma;
  double a0 = gsl_vector_get(fittingVar, 0);

  for (size_t i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj, */
    /* where fi = (Yi - yi)/sigma[i],      */
    /*       Yi = 1.0 / (a0 * (Xi + a2fixed)) + a1  */
    /* and the xj are the parameters (a0, a1, a2fixed) */
    double t = inputX[i];
    double s = 1.0 / sigma[i];
    double tmp = -1.0 / (a0 * (t + a2fixed));
    gsl_matrix_set(J, i, 0, s * tmp / a0);
    gsl_matrix_set(J, i, 1, s);
  }

  return GSL_SUCCESS;
}

/************************************************************************
 * expb_fdf combines expb_f anf expb_df together.                       *
 ************************************************************************/
int expb_fdf12(const gsl_vector *fittingVar, void *params, gsl_vector *f,
               gsl_matrix *J) {
  expb_f12(fittingVar, params, f);
  expb_df12(fittingVar, params, J);

  return GSL_SUCCESS;
}

/* Print out two parameters */
void print_state12(size_t iter, gsl_multifit_fdfsolver *s) {
  printf("iter: %3d parameters are: %15.8f %15.8f |f(x)| = %g\n", (int)iter,
         gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1),
         gsl_blas_dnrm2(s->f));
}

/* curvefit12() fits the input X and Y vectors based on this equation:
 * Y = 1.0 / [var1 * (X + var3min * 0.99)] + var2
 * Note that var3min * 0.99 is a fixed arbitrary value to make sure
 * that when 1/f function is expanded to a longer range (totalReps * TR / 2000),
 * the initial value wouldn't be negative and enormously large. */
VB_Vector *curvefit12(VB_Vector *xVec, VB_Vector *yVec, VB_Vector *sigmaVec,
                      double var3, double var1_init, double var2_init,
                      const char *outputFile, bool printFlag) {
  a2fixed = var3;
  /* fittingVec will include 8 elements: fittingStatus, # of iteration,
   * three fitting parameters' value, three deviation values
   * If status is 1.0, it means fitting is successful. 0 means fitting isn't
   * successful.*/
  VB_Vector *fittingVec = new VB_Vector(8);
  size_t N = xVec->getLength();
  const gsl_multifit_fdfsolver_type *T;
  gsl_multifit_fdfsolver *s;

  int status;
  size_t iter = 0;
  const size_t n = N;
  const size_t p = 2;
  gsl_matrix *covar = gsl_matrix_alloc(p, p);

  double inputX[N], inputY[N], sigma[N];
  struct data d = {n, inputX, inputY, sigma};
  gsl_multifit_function_fdf f;

  double fitting_init[2] = {var1_init, var2_init};
  gsl_vector_view fittingVar = gsl_vector_view_array(fitting_init, p);
  const gsl_rng_type *type;
  gsl_rng *r;

  gsl_rng_env_setup();
  type = gsl_rng_default;
  r = gsl_rng_alloc(type);

  f.f = &expb_f12;
  f.df = &expb_df12;
  f.fdf = &expb_fdf12;
  f.n = n;
  f.p = p;
  f.params = &d;

  for (size_t i = 0; i < n; i++) {
    inputX[i] = xVec->getElement(i);
    inputY[i] = yVec->getElement(i);
    /* Set sigma to be 0.1 at each point. This value is completely arbitrary.
     * It won't change the fitting variables' values, but the deviation of each
     * fitting variable (the value after "+/-") will be affected. */
    sigma[i] = sigmaVec->getElement(i);
  }

  T = gsl_multifit_fdfsolver_lmsder;
  s = gsl_multifit_fdfsolver_alloc(T, n, p);
  gsl_multifit_fdfsolver_set(s, &f, &fittingVar.vector);

  if (printFlag) print_state12(iter, s);

  do {
    iter++;
    status = gsl_multifit_fdfsolver_iterate(s);

    if (printFlag) {
      printf("status = %s\n", gsl_strerror(status));
      print_state12(iter, s);
    }

    if (status) break;
    status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
  } while (status == GSL_CONTINUE && iter < 500);

  gsl_multifit_covar(s->J, 0.0, covar);

#define FIT(i) gsl_vector_get(s->x, i)
#define ERR(i) sqrt(gsl_matrix_get(covar, i, i))

  if (printFlag) {
    printf("\nElements in covariance matrix are:\n");
    gsl_matrix_fprintf(stdout, covar, "%g");

    printf("\nparameter #1: steepness = %.5f +/- %.5f\n", FIT(0), ERR(0));
    printf("parameter #2: y-offset  = %.5f +/- %.5f\n", FIT(1), ERR(1));
    printf("parameter #3: x-shift   = %.5f (fixed)\n", var3);
    printf("status = %s\n\n", gsl_strerror(status));
  }

  // Write the fitting result to a file (if output filename is available)
  if (outputFile != '\0') {
    FILE *f1 = fopen(outputFile, "w");
    fprintf(f1, ";VB98\n");
    fprintf(f1, ";REF1\n");
    fprintf(f1, "; GSL nonlinear least-squares fitting\n");
    string myTime, myDate;
    maketimedate(myTime,
                 myDate);  // maketimedate() is defined in libvoxbo/vbutil.cpp
    fprintf(f1, "; Date created: %s_%s\n", myTime.data(), myDate.data());
    fprintf(f1, "; Initial guess: %f, %f\n", var1_init, var2_init);
    fprintf(f1, "; Number of iteration: %d\n", (int)iter);
    fprintf(f1, "; Status = %s\n", gsl_strerror(status));
    fprintf(f1, "; Deviation: %f, %f\n\n", ERR(0), ERR(1));
    fprintf(f1, "%f\n%f\n%f\n", FIT(0), FIT(1), var3);
    fclose(f1);
  }

  // Set elements in fittingVec. If status is 0 (success),
  // set elements one by one, Otherwise set all elements to zero.
  if (status == 0) {
    fittingVec->setElement(0, 1.0);
    fittingVec->setElement(1, iter);
    fittingVec->setElement(2, FIT(0));
    fittingVec->setElement(3, FIT(1));
    fittingVec->setElement(4, var3);
    fittingVec->setElement(5, ERR(0));
    fittingVec->setElement(6, ERR(1));
    fittingVec->setElement(7, 0);
  } else
    fittingVec->setAll(0);

  gsl_multifit_fdfsolver_free(s);
  return fittingVec;
}

/******************************************************************************************
 * wrapper function for fitting calculation. It accepts filenames for x, y,
 *sigma values. * It also accepts filename for initial guess. *
 ******************************************************************************************/
VB_Vector *curvefit(const char *xFilename, const char *yFilename,
                    const char *sigmaFilename, double var3min,
                    const char *initFile, const char *outputFile) {
  // Check xFilename format
  VB_Vector *xVec = new VB_Vector();
  string xString(xFilename);
  if (xVec->ReadFile(xString)) {
    printf("Invalid file format for X: %s\n", xFilename);
    return 0;
  }

  // Check yFilename format
  VB_Vector *yVec = new VB_Vector();
  string yString(yFilename);
  if (yVec->ReadFile(yString)) {
    printf("Invalid file format for Y: %s\n", yFilename);
    return 0;
  }

  // Check sigmaFilename format
  VB_Vector *sigmaVec = new VB_Vector();
  string sigmaString(sigmaFilename);
  if (sigmaVec->ReadFile(sigmaString)) {
    printf("Invalid file format for sigma: %s\n", sigmaFilename);
    return 0;
  }

  // Check initFile format
  VB_Vector *initVec = new VB_Vector();
  string initString(initFile);
  if (initVec->ReadFile(initString)) {
    printf("Invalid file format for initialization: %s\n", initFile);
    return 0;
  }

  /* Read the file which has initial values for three parameters */
  double var1_init = initVec->getElement(0);
  double var2_init = initVec->getElement(1);
  double var3_init = initVec->getElement(2);

  return curvefit(xVec, yVec, sigmaVec, var3min, var1_init, var2_init,
                  var3_init, outputFile);
}

/******************************************************************************************
 * Wrapper function for fitting calculation. It accepts filenames for x, y,
 *sigma values. * input sigma values is a constant at each point. *
 ******************************************************************************************/
VB_Vector *curvefit(const char *xFilename, const char *yFilename,
                    double inputSigma, double var3min, double var1_init,
                    double var2_init, double var3_init,
                    const char *outputFile) {
  /* Read input file for X and Y values */
  VB_Vector *xVec = new VB_Vector();
  string xString(xFilename);
  if (xVec->ReadFile(xString)) {
    printf("Invalid file format for X: %s\n", xFilename);
    return 0;
  }

  VB_Vector *yVec = new VB_Vector(yFilename);
  string yString(yFilename);
  if (yVec->ReadFile(yString)) {
    printf("Invalid file format for Y: %s\n", yFilename);
    return 0;
  }

  /* Read input file for sigma values */
  int length = xVec->getLength();
  VB_Vector *sigmaVec = new VB_Vector(length);
  sigmaVec->setAll(inputSigma);

  return curvefit(xVec, yVec, sigmaVec, var3min, var1_init, var2_init,
                  var3_init, outputFile);
}

/******************************************************************************************
 * Wrapper function for fitting calculation. It accepts filenames for x, y,
 *sigma values. *
 ******************************************************************************************/
VB_Vector *curvefit(const char *xFilename, const char *yFilename,
                    const char *sigmaFilename, double var3min, double var1_init,
                    double var2_init, double var3_init,
                    const char *outputFile) {
  // Check xFilename format
  VB_Vector *xVec = new VB_Vector();
  string xString(xFilename);
  if (xVec->ReadFile(xString)) {
    printf("Invalid file format for X: %s\n", xFilename);
    return 0;
  }

  // Check yFilename format
  VB_Vector *yVec = new VB_Vector();
  string yString(yFilename);
  if (yVec->ReadFile(yString)) {
    printf("Invalid file format for Y: %s\n", yFilename);
    return 0;
  }

  // Check sigmaFilename format
  VB_Vector *sigmaVec = new VB_Vector();
  string sigmaString(sigmaFilename);
  if (sigmaVec->ReadFile(sigmaString)) {
    printf("Invalid file format for sigma: %s\n", sigmaFilename);
    return 0;
  }

  return curvefit(xVec, yVec, sigmaVec, var3min, var1_init, var2_init,
                  var3_init, outputFile);
}

/************************************************************************************************
 * This is the generic function to get fitting for a input power spectrum
 *vector. It has the following arguments: #1 (psVecIn): input vector which
 *include power spectrum elements #2 (ignorePSIn): another vector which defines
 *frequencies ignored (elements which is equal to 0) #3: input TR value in unit
 *of ms (default is 2000) #4: input sigma value (default is 0.1) #5: initial
 *guess for parameter 1 (default is 20.0) #6: initial guess for parameter 2
 *(default is 2.0) #7: initial guess for parameter 3 (default is -0.0001) #8:
 *output filename (default is empty) #9: flag to show printout or not (default
 *is yes)
 ************************************************************************************************/
VB_Vector *fitOneOverF(VB_Vector *psVecIn, VB_Vector *ignorePSin,
                       double var3min, double TRin, double sigma, double var1,
                       double var2, double var3, const char *outputFile,
                       bool printFlag) {
  int cmpStatus = cmpVec(psVecIn, ignorePSin);
  if (cmpStatus == 2) {
    printf(
        "Error: The number of elements in ps vector doesn't match ignorePS "
        "vector.\n");
    return 0;
  }

  double TR = TRin / 1000.0;
  double var1init = var1, var2init = var2, var3init = var3;

  int numObs = psVecIn->getLength();
  double duration = numObs * TR;
  int sigLen = numObs / 2 - 4;
  VB_Vector *xVec = new VB_Vector(sigLen);
  VB_Vector *signal = new VB_Vector(sigLen);
  VB_Vector *ignoreVec = new VB_Vector(sigLen);

  for (int i = 0; i < sigLen; i++) {
    double tmpVal = ignorePSin->getElement(i + 1);
    ignoreVec->setElement(i, tmpVal);
    xVec->setElement(i, ((double)i + 1.0) / duration);
    tmpVal = sqrt(psVecIn->getElement(i + 1));
    signal->setElement(i, tmpVal);
  }
  var2init = signal->getElement(sigLen - 1);

  size_t counter = 0;
  size_t numNonZero = findNonZero(ignoreVec);

  if (numNonZero == 0) {
    printf("Error: no freqency found in ignorePS vector\n");
    return 0;
  }

  VB_Vector *subx = new VB_Vector(numNonZero);
  VB_Vector *subsignal = new VB_Vector(numNonZero);
  VB_Vector *sigmaVec = new VB_Vector(numNonZero);
  sigmaVec->setAll(sigma);

  for (size_t i = 0; i < signal->getLength(); i++) {
    if (ignoreVec->getElement(i) != 0) {
      subsignal->setElement(counter, signal->getElement(i));
      subx->setElement(counter, xVec->getElement(i));
      counter++;
    } else
      continue;
  }

  if (counter == 0) {
    printf("Error: no freqency found in ignorePS vector\n");
    return 0;
  }

  VB_Vector *newVec = new VB_Vector(8);
  newVec = curvefit(subx, subsignal, sigmaVec, var3min, var1init, var2init,
                    var3init, outputFile, printFlag);

  return newVec;
}

/******************************************************************************************
 * Function used by generic fitOneOverF().
 * Returns 0 if 1st vector has same number of elements as 2nd vector does;
 * Returns 1 if 2nd vector's length is multiple of 1st vector's;
 * Returns 2 otherwise
 *******************************************************************************************/
int cmpVec(VB_Vector *shortVec, VB_Vector *longVec) {
  int length_s = shortVec->getLength();
  int length_l = longVec->getLength();

  if (length_s == length_l)
    return 0;
  else if (length_l % length_s == 0)
    return 1;
  else
    return 2;
}

/******************************************************************************************
 * Function used by generic fitOneOverF().
 * It returns number of non-zero elements in the input vector.
 ******************************************************************************************/
size_t findNonZero(VB_Vector *inputVec) {
  size_t counter = 0;
  for (size_t i = 0; i < inputVec->getLength(); i++) {
    if (inputVec->getElement(i) != 0)
      counter++;
    else
      continue;
  }

  return counter;
}

/******************************************************************************************
 * Wrapper function based on generic fitOneOverF().
 * Only power spectrum vector is specified.
 * All elements in ignorePS vector will be set to be 1 (no frequencies will be
 *ignored.)
 *******************************************************************************************/
VB_Vector *fitOneOverF(VB_Vector *psVec, double var3min, double TRin,
                       double sigma, double var1, double var2, double var3,
                       const char *outputFile, bool printFlag) {
  VB_Vector *ignorePSin = new VB_Vector(psVec->getLength());
  ignorePSin->setAll(1.0);

  return fitOneOverF(psVec, ignorePSin, var3min, TRin, sigma, var1, var2, var3,
                     outputFile, printFlag);
}

/******************************************************************************************
 * Wrapper function based on generic fitOneOverF().
 * It accepts a filename for ps vector.
 *******************************************************************************************/
VB_Vector *fitOneOverF(const char *psFile, double var3min, double TRin,
                       double sigma, double var1, double var2, double var3,
                       const char *outputFile, bool printFlag) {
  // Check psFile
  VB_Vector *psVec = new VB_Vector();
  string psString(psFile);
  if (psVec->ReadFile(psString)) {
    printf("Invalid file format for power spectrum: %s\n", psFile);
    return 0;
  }

  VB_Vector *ignorePSin = new VB_Vector(psVec->getLength());
  ignorePSin->setAll(1.0);

  return fitOneOverF(psVec, ignorePSin, var3min, TRin, sigma, var1, var2, var3,
                     outputFile, printFlag);
}

/******************************************************************************************
 * Wrapper function based on generic fitOneOverF().
 * It accepts a filename for reference function.
 * Reference function will be used to build ignorePS vector
 *******************************************************************************************/
VB_Vector *fitOneOverF(VB_Vector *psVec, const char *refFunc, double var3min,
                       double TRin, double sigma, double var1, double var2,
                       double var3, const char *outputFile, bool printFlag) {
  VB_Vector *refLocal = new VB_Vector();
  tokenlist condKey;
  int refStat = getCondVec(refFunc, condKey, refLocal);
  if (refStat == -1) {
    printf("File not readable: %s\n", refFunc);
    return 0;
  } else if (refStat == -2) {
    printf("Error: different number of keys in header and content: %s\n",
           refFunc);
    return 0;
  } else if (refStat == 1) {
    printf("Error: different keys in header and content: %s\n", refFunc);
    return 0;
  }

  refLocal->meanCenter();
  VB_Vector *psRef = new VB_Vector(refLocal->getLength());
  refLocal->getPS(psRef);

  VB_Vector *ignorePS = new VB_Vector(refLocal->getLength());
  ignorePS->setAll(1.0);
  double refPSmax = psRef->getMaxElement();

  for (size_t i = 0; i < ignorePS->getLength(); i++) {
    if (psRef->getElement(i) > refPSmax * 0.01)
      ignorePS->setElement(i, 0);
    else
      continue;
  }

  return fitOneOverF(psVec, ignorePS, var3min, TRin, sigma, var1, var2, var3,
                     outputFile, printFlag);
}

/******************************************************************************************
 * Wrapper function based on generic fitOneOverF().
 * It accepts a filename for reference function.
 * Reference function will be used to build ignorePS vector
 *******************************************************************************************/
VB_Vector *fitOneOverF(const char *psFile, const char *refFunc, double var3min,
                       double TRin, double sigma, double var1, double var2,
                       double var3, const char *outputFile, bool printFlag) {
  VB_Vector *psVec = new VB_Vector();
  string psString(psFile);
  if (psVec->ReadFile(psString)) {
    printf("Invalid file format for power spectrum: %s\n", psFile);
    return 0;
  }

  VB_Vector *refLocal = new VB_Vector();
  tokenlist condKey;
  int refStat = getCondVec(refFunc, condKey, refLocal);
  if (refStat == -1) {
    printf("File not readable: %s\n", refFunc);
    return 0;
  } else if (refStat == -2) {
    printf("Error: different number of keys in header and content: %s\n",
           refFunc);
    return 0;
  } else if (refStat == 1) {
    printf("Error: different keys in header and content: %s\n", refFunc);
    return 0;
  }

  refLocal->meanCenter();
  VB_Vector *psRef = new VB_Vector(refLocal->getLength());
  refLocal->getPS(psRef);

  VB_Vector *ignorePS = new VB_Vector(refLocal->getLength());
  ignorePS->setAll(1.0);
  double refPSmax = psRef->getMaxElement();

  for (size_t i = 0; i < ignorePS->getLength(); i++) {
    if (psRef->getElement(i) > refPSmax * 0.01)
      ignorePS->setElement(i, 0);
    else
      continue;
  }

  return fitOneOverF(psVec, ignorePS, var3min, TRin, sigma, var1, var2, var3,
                     outputFile, printFlag);
}

/******************************************************************************************
 * generic function to make time domain representation of the 1/f curve given a
 *set of three parameters, number of images and TR (default is 2000 ms)
 ******************************************************************************************/
VB_Vector *makeOneOverF(int numImages, double var1, double var2, double var3,
                        double TRin) {
  double TR = TRin / 1000.0;
  int halfLen = numImages / 2;
  double xVal, yVal;

  VB_Vector *freqVec = new VB_Vector(numImages);
  for (int i = 1; i <= halfLen; i++) {
    xVal = (double)i / (TR * numImages);
    yVal = 1.0 / (var1 * (xVal + var3)) + var2;
    freqVec->setElement(i, yVal);
  }

  if (numImages % 2 == 0) {
    int j = 0;
    for (int i = halfLen + 1; i < numImages; i++) {
      yVal = freqVec->getElement(halfLen - j);
      freqVec->setElement(i, yVal);
      j++;
    }
  } else {
    int j = 0;
    for (int i = halfLen + 2; i < numImages; i++) {
      yVal = freqVec->getElement(halfLen - j);
      freqVec->setElement(i, yVal);
      j++;
    }
    yVal = freqVec->getElement(halfLen + 2);
    freqVec->setElement(halfLen + 1, yVal);
  }

  freqVec->setElement(0, 0);

  freqVec->scaleInPlace(1.0 / freqVec->getMaxElement());
  VB_Vector *realPart = new VB_Vector(numImages);
  VB_Vector *imagPart = new VB_Vector(numImages);
  imagPart->setAll(0);
  freqVec->ifft(realPart, imagPart);
  realPart->normMag();

  return realPart;
}

/**************************************************************************
 * wrapper function based on generic makeOneOverF()
 * It accepts a filename which includes three fitting parameters.
 **************************************************************************/
VB_Vector *makeOneOverF(int numImages, const char *paramFile, double TRin) {
  VB_Vector *initGuess = new VB_Vector(paramFile);
  double var1 = initGuess->getElement(0);
  double var2 = initGuess->getElement(1);
  double var3 = initGuess->getElement(2);

  return makeOneOverF(numImages, var1, var2, var3, TRin);
}
