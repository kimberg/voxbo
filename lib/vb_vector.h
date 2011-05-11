
// vb_vector.h
// VoxBo vector class
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
// original version written by Kosh Bannerjee
// various additions and changes by Dan Kimberg

/*********************************************************************
 * This class defines the VB_Vector object. Basically, this class is  *
 * a wrapper around functions from the GNU Scientific Library. See    *
 * http://www.gnu.org/software/gsl/                                   *
 *********************************************************************/

/*********************************************************************
 * Include guard.                                                     *
 *********************************************************************/
#ifndef VB_VECTOR_H
#define VB_VECTOR_H

/*********************************************************************
 * Required include files.                                            *
 *********************************************************************/
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <gsl/gsl_block.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_vector_double.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_sys.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_sort_vector.h>
#include "vbio.h"

// qq How to get file name from ostream and istream?
// qq Test all exceptions!
// qq Use GSL vector views to implement parallelism.

/*********************************************************************
 * Using the standard namespace.                                      *
 *********************************************************************/
using namespace std;

class VB_Vector
{

private:

  /*********************************************************************
   * DATA MEMBERS:                                                      *
   *                                                                    *
   * NAME              TYPE           DESCRIPTION                       *
   * ----              ----           -----------                       *
   * theVector         gsl_vector *   Pointer to a GSL gsl_vector       *
   *                                  struct. This struct holds all the *
   *                                  required information for the      *
   *                                  vector.                           *
   * fileName          string         File name of the VoxBo vector     *
   *                                  file.                             *
   * valid             bool           Set to true if reading a VoxBo    *
   *                                  vector file succeeds.             *
   * dataType          VB_datatype    The data type of the vector.      *
   * fileFormat        VBFF           The VoxBo file type of the file   *
   *                                  from which the vector was read.   *
   *********************************************************************/
  string fileName;
  bool valid;
  VB_datatype dataType;
  VBFF fileFormat;  // DYK: made it a non-pointer

  /*********************************************************************
   * GSL implements its own error handler. GSL also implements a        *
   * function to turn off its error handler. currentGSLErrorHandler is  *
   * used to store the value, i.e., function name, of the current error *
   * handler. NOTE: Making this variable static means that exactly one  *
   * error handler function can be stored for all instances of this     *
   * class.                                                             *
   *********************************************************************/
  static gsl_error_handler_t *currentGSLErrorHandler;

  /*********************************************************************
   * Method to turn off the current GSL error handler.                  *
   *********************************************************************/
  void turnOffGSLErrorHandler() const;

  /*********************************************************************
   * Method to restore the GSL error handler to                         *
   * this->currentGSLErrorHandler.                                      *
   *********************************************************************/
  void restoreGSLErrorHandler() const;

  /*********************************************************************
   * METHODS:                                                           *
   *********************************************************************/

  void init(const size_t length) throw (GenericExcep);
  void init(const bool validFlag, const VB_datatype dType, const VBFF fType);
  void init(const bool validFlag, const VB_datatype dType, const string signature);

  /*********************************************************************
   * These private static methods are used to make exception throwing   *
   * easier.                                                            *
   *********************************************************************/
  void GSLVectorMemcpy(gsl_vector *dest, const gsl_vector *src) const throw (GenericExcep);

  void checkVectorRange(const size_t index, const int lineNumber,
                        const char *fileName, const char *prettyFunctionName) const throw (GenericExcep);

  static void checkVectorLengths(const gsl_vector *V1, const gsl_vector *V2,
                                 const int lineNumber, const char *fileName, const char *prettyFunctionName) throw (GenericExcep);

  static void checkVectorLengths(const size_t len1, const size_t len2,
                                 const int lineNumber, const char *fileName, const char *prettyFunctionName) throw (GenericExcep);

  static void vectorNull(const gsl_vector *v) throw (GenericExcep);

  static void checkGSLStatus(const int status, const int lineNumber,
                             const char *fileName, const char *prettyFunctionName);

  static void createException(const string& errorMsg, const int lineNumber,
                              const string& fileName, const string& prettyFunctionName);

  static void createException(const char *errorMsg, const int lineNumber,
                              const char *fileName, const char *prettyFunctionName);

  static void checkFiniteness(const gsl_vector *v, const int lineNumber,
                              const char *fileName, const char *prettyFunctionName);

  /*********************************************************************
   * This method allocates memory to a gsl_matrix struct.               *
   *********************************************************************/
  gsl_matrix *initMatrix(const size_t rows, const size_t cols) const throw (GenericExcep);

  /*********************************************************************
   * Private static method.                                             *
   *********************************************************************/
  static void makePhi(double *phi, const int length, const double timeShift); // QQ tested
public:
  gsl_vector *theVector;
  VB_Vector(); // Default constructor. // QQ tested
  VB_Vector(const size_t length); // QQ tested
  VB_Vector(const double *data, const size_t length); // QQ tested
  VB_Vector(const gsl_vector *V2); // QQ tested
  VB_Vector(const gsl_vector& V2); // QQ tested
  VB_Vector(const VB_Vector& V2); // Copy constructor. // QQ tested
  VB_Vector(const VB_Vector *V); // QQ tested
  VB_Vector(const string& vecFile); // QQ tested
  VB_Vector(const char *vecFile); // QQ tested
  VB_Vector(const vector<double>& theVector); // QQ tested
  VB_Vector(const vector<double> *theVector); // QQ tested
  //  VB_Vector(const Vec *theVector);
  //  VB_Vector(const Vec& theVector);
  VB_Vector(const bitmask &bm);
  VB_Vector(const Tes& theTes, const unsigned long tSeriesIndex);
  ~VB_Vector(); // QQ tested

  void clear();
  void print();

  /*********************************************************************
   * This accessor method returns the valid data member.                *
   *********************************************************************/
  inline bool getState()
  {
    return this->valid;
  } // inline bool getState()

  /*********************************************************************
   * Instance methods to calculate the Euclidean inner product.         *
   *********************************************************************/
  double euclideanProduct(const VB_Vector& V2) const; // QQ tested
  double euclideanProduct(const VB_Vector *V2) const; // QQ tested
  double euclideanProduct(const gsl_vector& V2) const; // QQ tested
  double euclideanProduct(const gsl_vector *V2) const; // QQ tested

  /*********************************************************************
   * Static methods to calculate the Euclidean inner product.           *
   *********************************************************************/
  static double euclideanProduct(const gsl_vector *V1, const gsl_vector *V2); // QQ tested
  static double euclideanProduct(const gsl_vector& V1, const gsl_vector& V2); // QQ tested

  /*********************************************************************
   * Methods to return the length of the VB_Vector.                      *
   *********************************************************************/
  inline size_t
  getLength() const
  {
    if (this->theVector)
      return this->theVector->size;
    else
      return 0;
  }
  
  inline size_t
  size() const
  {
    if (this->theVector)
      return this->theVector->size;
    else
      return 0;
  }
  
  /*********************************************************************
   * Method to get the specified vector element.                        *
   *********************************************************************/
  double getElement(const size_t index) const; // QQ tested
  
  /*********************************************************************
   * Method to set the specified element to the specified value.        *
   *********************************************************************/
  void setElement(const size_t index, const double value); // QQ tested

  /*********************************************************************
   * This method sets all the elements of the vector to the specified   *
   * value.                                                             *
   *********************************************************************/
  inline void setAll(const double value) // QQ tested
  {

    /*********************************************************************
     * Calling gsl_vector_set_all() to set all the value in               *
     * this->theVector to the specified value.                            *
     *********************************************************************/
    gsl_vector_set_all(this->theVector, value);

  } // inline void VB_Vector::setAll(const double value)

  /*********************************************************************
   * Instance method to return theVector data member.                   *
   *********************************************************************/
  inline gsl_vector *getTheVector() const // QQ tested
  {
    return this->theVector;
  } // inline gsl_vector *getTheVector() const

  /*********************************************************************
   * The gsl_vector struct is:                                          *
   *                                                                    *
   * typedef struct                                                     *
   * {                                                                  *
   *   size_t size;                                                     *
   *   size_t stride;                                                   *
   *   double *data;                                                    *
   *   gsl_block *block;                                                *
   *   int owner;                                                       *
   * } gsl_vector;                                                      *
   *                                                                    *
   * The following methods return these fields (except for the field    *
   * size, since we already have VB_Vector::getLength().)               *
   *********************************************************************/

  /*********************************************************************
   * Method to return this->theVector->data.                            *
   *********************************************************************/
  inline double *getData() const // QQ tested
  {
    return this->theVector->data;
  } // inline double *getData() const

  /*********************************************************************
   * Methods to scale this->theVector in place. That is,                *
   * a[i] = alpha * a[i].                                               *
   *********************************************************************/
  void scaleInPlace(const double alpha); // QQ tested

  /*********************************************************************
   * This method returns the maximum element from the vector.           *
   *********************************************************************/
  inline double getMaxElement() const // QQ tested
  {

    /*********************************************************************
     * Calling gsl_vector_max() to fetch the maximum element from         *
     * this->theVector.                                                   *
     *********************************************************************/
    return gsl_vector_max(this->theVector);

  } // inline double VB_Vector::getMaxElement() const

  /*********************************************************************
   * This method returns the minimum element from the vector.           *
   *********************************************************************/
  inline double getMinElement() const // QQ tested
  {

    /*********************************************************************
     * Calling gsl_vector_min() to fetch the minimum element from         *
     * this->theVector.                                                   *
     *********************************************************************/
    return gsl_vector_min(this->theVector);

  } // inline double VB_Vector::getMinElement() const

  /*********************************************************************
   * This method returns the index of a maximum element from the vector.*
   * If there is more than one index that corresponds to a maximum, the *
   * lowest index is returned.                                          *
   *********************************************************************/
  inline size_t getMaxElementIndex() const // QQ tested
  {

    /*********************************************************************
     * Calling gsl_vector_max_index() to fetch the index.                 8
     *********************************************************************/
    return gsl_vector_max_index(this->theVector);

  } // inline size_t VB_Vector::getMaxElementIndex() const

  /*********************************************************************
   * This method returns the index of a minimum element from the vector.*
   * If there is more than one index that corresponds to a minimum, the *
   * lowest index is returned.                                          *
   *********************************************************************/
  inline size_t getMinElementIndex() const // QQ tested
  {

    /*********************************************************************
     * Calling gsl_vector_min_index() to fetch the index.                 8
     *********************************************************************/
    return gsl_vector_min_index(this->theVector);

  } // inline size_t VB_Vector::getMinElementIndex() const

  /*********************************************************************
   * Overloaded "+" operator, implements ordinary vector addition.      *
   *********************************************************************/
  VB_Vector operator+(const VB_Vector *V2) const; // QQ tested
  VB_Vector operator+(const VB_Vector& V2) const; // QQ tested
  VB_Vector operator+(const gsl_vector& V2) const; // QQ tested
  VB_Vector operator+(const gsl_vector *V2) const; // QQ tested

  /*********************************************************************
   * Overloaded "+" operator, as friend functions, to preserve the      *
   * commutative property. NOTE: There will no overloaded "+" operator  *
   * with the prototype:                                                *
   *                                                                    *
   * friend VB_Vector operator+(const gsl_vector *V1,                   *
   * const VB_Vector *V2);                                              *
   *                                                                    *
   * because "+" can not be overloaded when its arguments are 2         *
   * pointers.                                                          *
   *********************************************************************/
  friend VB_Vector operator+(const gsl_vector *V1, const VB_Vector& V2); // QQ tested
  friend VB_Vector operator+(const gsl_vector& V1, const VB_Vector& V2); // QQ tested
  friend VB_Vector operator+(const gsl_vector& V1, const VB_Vector *V2); // QQ tested

  /*********************************************************************
   * Overloaded "-" operator.                                           *
   *********************************************************************/
  VB_Vector operator-(const VB_Vector *V2) const; // QQ tested
  VB_Vector operator-(const VB_Vector& V2) const; // QQ tested
  VB_Vector operator-(const gsl_vector& V2) const; // QQ tested
  VB_Vector operator-(const gsl_vector *V2) const; // QQ tested

  /*********************************************************************
   * Overloaded "*" operator, returns the Euclidean inner product.      *
   *********************************************************************/
  double operator*(const VB_Vector *V2) const; // QQ tested
  double operator*(const VB_Vector& V2) const; // QQ tested
  double operator*(const gsl_vector& V2) const; // QQ tested
  double operator*(const gsl_vector *V2) const; // QQ tested

  /*********************************************************************
   * Overloaded "*" operator, as friend functions, to preserve the      *
   * commutative property. NOTE: There will no overloaded "*" operator  *
   * with the prototype:                                                *
   *                                                                    *
   * friend double operator*(const gsl_vector *V1,                      *
   * const VB_Vector *V2);                                              *
   *                                                                    *
   * because "*" can not be overloaded when its arguments are 2         *
   * pointers. These overloaded operators simply return the Euclidean   *
   * inner product of the 2 input vectors.                              *
   *********************************************************************/
  friend double operator*(const gsl_vector *V1, const VB_Vector& V2); // QQ tested
  friend double operator*(const gsl_vector& V1, const VB_Vector& V2); // QQ tested
  friend double operator*(const gsl_vector& V1, const VB_Vector *V2); // QQ tested

  /*********************************************************************
   * Overloaded "*" operators that do scaling. A friend function is     *
   * used to preserve commutativity.                                    *
   *********************************************************************/
  VB_Vector operator*(const double alpha) const; // QQ tested
  friend VB_Vector operator*(const double alpha, const VB_Vector& V); // QQ tested

  /*********************************************************************
   * Overloaded "[]" and "()" operators. The overloaded "()" operator   *
   * does range checking, whereas the overloaded "[]" operator does not.*
   *********************************************************************/
  double& operator[](const size_t index) const; // QQ tested
  double& operator()(const size_t index) const; // QQ tested

  /*********************************************************************
   * Overloaded "==" operator.                                          *
   *********************************************************************/
  bool operator==(const VB_Vector *V2) const; // QQ tested
  bool operator==(const VB_Vector& V2) const; // QQ tested
  bool operator==(const gsl_vector *V2) const; // QQ tested
  bool operator==(const gsl_vector& V2) const; // QQ tested

  /*********************************************************************
   * Overloaded "==" operator, as friend functions, to preserve the     *
   * commutative property.                                              *
   *********************************************************************/
  friend bool operator==(const gsl_vector *V1, const VB_Vector& V2); // QQ tested
  friend bool operator==(const gsl_vector& V1, const VB_Vector& V2); // QQ tested

  /*********************************************************************
   * Overloaded "!=" operator.                                          *
   *********************************************************************/
  bool operator!=(const VB_Vector *V2) const; // QQ tested
  bool operator!=(const VB_Vector& V2) const; // QQ tested
  bool operator!=(const gsl_vector *V2) const; // QQ tested
  bool operator!=(const gsl_vector& V2) const; // QQ tested

  /*********************************************************************
   * Overloaded "!=" operator, as friend functions, to preserve the     *
   * commutative property.                                              *
   *********************************************************************/
  friend bool operator!=(const gsl_vector *V1, const VB_Vector& V2); // QQ tested
  friend bool operator!=(const gsl_vector& V1, const VB_Vector& V2); // QQ tested

  /*********************************************************************
   * Overloaded "=" operator. NOTE: Returning a const disallows:        *
   *                                                                    *
   *       (a = b) = c;                                                 *
   *                                                                    *
   * Returning a reference allows:                                      *
   *                                                                    *
   *       a = b = c;                                                   *
   *********************************************************************/
  const VB_Vector& operator=(const VB_Vector& V2); // QQ tested
  const VB_Vector &zero();

  /*********************************************************************
   * Overloaded "<<" operators.                                         *
   *********************************************************************/
  friend ostream& operator<<(ostream& outStream, const VB_Vector& V); // QQ tested
  friend ostream& operator<<(ostream& outStream, const VB_Vector *V); // QQ tested

  /*********************************************************************
   * Operators "<<" and ">>" overloaded as left and right shift         *
   * operators, respectively.                                           *
   *********************************************************************/
  VB_Vector& operator<<(size_t i); // QQ tested
  VB_Vector& operator>>(size_t i); // QQ tested

  /*********************************************************************
   * Misc. overloaded operators.                                        *
   *********************************************************************/
  VB_Vector& operator+=(const double alpha); // QQ tested
  VB_Vector& operator-=(const double alpha); // QQ tested
  VB_Vector& operator*=(const double alpha); // QQ tested
  VB_Vector& operator/=(const double alpha); // QQ tested
  VB_Vector& operator+=(const VB_Vector& V); // QQ tested
  VB_Vector& operator-=(const VB_Vector& V); // QQ tested
  VB_Vector& operator*=(const VB_Vector& V); // QQ tested
  VB_Vector& operator/=(const VB_Vector& V); // QQ tested
  VB_Vector& operator+=(const VB_Vector *V); // QQ tested
  VB_Vector& operator-=(const VB_Vector *V); // QQ tested
  VB_Vector& operator*=(const VB_Vector *V); // QQ tested
  VB_Vector& operator/=(const VB_Vector *V); // QQ tested
  VB_Vector& operator+=(const gsl_vector *V); // QQ tested
  VB_Vector& operator-=(const gsl_vector *V); // QQ tested
  VB_Vector& operator*=(const gsl_vector *V); // QQ tested
  VB_Vector& operator/=(const gsl_vector *V); // QQ tested
  VB_Vector& operator+=(const gsl_vector& V); // QQ tested
  VB_Vector& operator-=(const gsl_vector& V); // QQ tested
  VB_Vector& operator*=(const gsl_vector& V); // QQ tested
  VB_Vector& operator/=(const gsl_vector& V); // QQ tested

  /*********************************************************************
   * Inline methods to access and mutate this->fileName.                *
   *********************************************************************/

  /*********************************************************************
   * Returns this->fileName.                                            *
   *********************************************************************/
  inline string getFileName() const // QQ tested
  {
    return this->fileName;
  } // inline string getFileName() const

  /*********************************************************************
   * Assigns the input string object to this->fileName.                 *
   *********************************************************************/
  inline void setFileName(const string newFileName) // QQ tested
  {
    this->fileName = newFileName;
  } // inline void setFileName(const string newFileName)

  /*********************************************************************
   * Assigns the input C-style string to this->fileName.                *
   *********************************************************************/
  inline void setFileName(const char *newFileName) // QQ tested
  {
    this->setFileName(string(newFileName));
  } // inline void setFileName(const char *newFileName)

  /*********************************************************************
   * Method to resize the vector.                                       *
   *********************************************************************/
  void resize(size_t newLength); // QQ tested

  /*********************************************************************
   * Methods to read a VoxBo vector file.                               *
   *********************************************************************/
  //void readREF1(); // QQ tested
  //void readREF1(const string& file); // QQ tested
  //void readREF1(const char *file); // QQ tested

  /*********************************************************************
   * Methods to write out the VoxBo vector file.                        *
   *********************************************************************/
  //void writeREF1(); // QQ tested
  //void writeREF1(const string& newFileName); // QQ tested
  //void writeREF1(const char *newFileName); // QQ tested

  /*********************************************************************
   * Instance and static methods to get the vector mean.                *
   *********************************************************************/
  double getVectorMean() const; // QQ tested
  static double getVectorMean(const VB_Vector& V); // QQ tested
  static double getVectorMean(const VB_Vector *V); // QQ tested
  static double getVectorMean(const gsl_vector *V); // QQ tested
  static double getVectorMean(const gsl_vector& V); // QQ tested

  /*********************************************************************
   * Instance method to compute the vector sum.                         *
   *********************************************************************/
  double getVectorSum() const;

  /*********************************************************************
   * Instance methods for unit variance.                                *
   *********************************************************************/
  void unitVariance(); // QQ tested
  VB_Vector unitVarianceVector() const; // QQ tested

  /*********************************************************************
   * Instance method to calculate the variance for the vector.          *
   *********************************************************************/
  double getVariance() const; // QQ tested

  /*********************************************************************
   * Instance methods for convolution. NOTE: These methods implement    *
   * the MATLAB style convolution function.                             *
   *********************************************************************/
  void convolve(const VB_Vector& v); // QQ tested
  void convolve(const VB_Vector *v); // QQ tested
  void convolve(const gsl_vector& v); // QQ tested
  void convolve(const gsl_vector *kernel); // QQ tested
  VB_Vector convolve2(const VB_Vector& v) const; // QQ tested
  VB_Vector convolve2(const VB_Vector *v) const; // QQ tested
  VB_Vector convolve2(const gsl_vector& v) const; // QQ tested
  VB_Vector convolve2(const gsl_vector *v) const; // QQ tested

  /*********************************************************************
   * Static methods to convolve 2 vectors. NOTE: These methods implement*
   * the MATLAB style convolution function.                             *
   *********************************************************************/
  static VB_Vector convolve(const VB_Vector *v1, const VB_Vector *v2); // QQ tested
  static VB_Vector convolve(const VB_Vector& v1, const VB_Vector& v2); // QQ tested
  static VB_Vector convolve(const gsl_vector *v1, const gsl_vector *v2); // QQ tested
  static VB_Vector convolve(const gsl_vector& v1, const gsl_vector& v2); // QQ tested

  /*********************************************************************
   * Instance methods to concatenate 2 vectors.                         *
   *********************************************************************/
  void concatenate(const VB_Vector& V); // QQ tested
  void concatenate(const VB_Vector *V); // QQ tested
  void concatenate(const gsl_vector& V); // QQ tested
  void concatenate(const gsl_vector *V); // QQ tested
  VB_Vector concatenate2(const VB_Vector& V) const; // QQ tested
  VB_Vector concatenate2(const VB_Vector *V) const; // QQ tested
  VB_Vector concatenate2(const gsl_vector& V) const; // QQ tested
  VB_Vector concatenate2(const gsl_vector *V) const; // QQ tested

  /*********************************************************************
   * Static methods to concatenate 2 vectors.                           *
   *********************************************************************/
  static VB_Vector concatenate(const VB_Vector& V1, const VB_Vector& V2); // QQ tested
  static VB_Vector concatenate(const VB_Vector *V1, const VB_Vector *V2); // QQ tested
  static VB_Vector concatenate(const gsl_vector& V1, const gsl_vector& V2); // QQ tested
  static VB_Vector concatenate(const gsl_vector *V1, const gsl_vector *V2); // QQ tested

  /*********************************************************************
   * Instance method to set the data elements in the vector.            *
   *********************************************************************/
  void setData(const double *theData, const size_t length); // QQ tested
  void setData(const gsl_vector *v); // QQ tested
  void setData(const gsl_vector& v); // QQ tested
  void setData(const VB_Vector *V); // QQ tested
  void setData(const VB_Vector& V); // QQ tested

  // adjust the time series a bit
  void meanCenter();
  int meanNormalize();
  int removeDrift();

  /*********************************************************************
   * Instance methods to ortogonalize.                                  *
   *********************************************************************/
  void orthogonalize(const vector<VB_Vector> reference);
  VB_Vector orthogonalize(const VB_Vector &myVec, const vector<VB_Vector> reference) const;

  /*********************************************************************
   * Static methods to write out a gsl_matrix struct to stdout.         *
   *********************************************************************/
  static void printMatrix(const gsl_matrix *M); // QQ tested
  static void printMatrix(const gsl_matrix& M); // QQ tested

  /*********************************************************************
   * Instance FFT and inverse FFT methods.                              *
   *********************************************************************/
  void fft(VB_Vector& realPart, VB_Vector& imagPart) const; // QQ tested
  void fft(VB_Vector *realPart, VB_Vector *imagPart) const; // QQ tested
  void ifft(VB_Vector& realPart, VB_Vector& imagPart) const; // QQ tested
  void ifft(VB_Vector *realPart, VB_Vector *imagPart) const; // QQ tested

  /*********************************************************************
   * Instance method to reverse the entries.                            *
   *********************************************************************/
  void reverse(); // QQ tested

  /*********************************************************************
   * Instance methods to compute the power spectrum.                    *
   *********************************************************************/
  void getPS(VB_Vector &result) const; // QQ tested
  void getPS(VB_Vector *result) const; // QQ tested
  void getPS() throw(); // QQ tested

  /*********************************************************************
   * Methods for sinc interpolation.                                    *
   *********************************************************************/
  static void sincInterpolation(const VB_Vector& timeSeries,
                                const unsigned int expFactor, VB_Vector& newSignal); // QQ tested
  void sincInterpolation(const unsigned int expFactor,
                         VB_Vector& newSignal) const; // QQ tested
  void sincInterpolation(const unsigned int expFactor); // QQ tested

  /*********************************************************************
   * Phase shift methods.                                               *
   *********************************************************************/
  static void phaseShift(const VB_Vector& tSeries,
                         const double timeShift, VB_Vector& shiftedSignal); // QQ tested
  void phaseShift(const double timeShift, VB_Vector& shiftedSignal) const; // QQ tested
  void phaseShift(const double timeShift); // QQ tested

  /*********************************************************************
   * Instance methods to nomalize the  magnitude component of a signal  *
   * to unity, and to preserve the phase component.                     *
   *********************************************************************/
  void normMag(); // QQ tested
  void normMag(VB_Vector& normalizedVec) const;

  /*********************************************************************
   * These methods apply the input function, which takes a single       *
   * argument of type double and returns a double, to each element of   *
   * the VB_Vector.                                                     *
   *********************************************************************/
  void applyFunction(double (*theFunction)(double)) throw(); // QQ tested
  void applyFunction(double (*theFunction)(double), VB_Vector& theResult) const throw(); // QQ tested
  void applyFunction(double (*theFunction)(double), VB_Vector *theResult) const throw(); // QQ tested

  /*********************************************************************
   * Instance method for element-by-element multiply.                   *
   *********************************************************************/
  void elementByElementMult(const VB_Vector *vec) throw();
  void elementByElementMult(const VB_Vector& vec) throw();

  /*********************************************************************
   * Static method to multiply complex vectors.                         *
   *********************************************************************/
  static void compMult(const VB_Vector& real1,
                       const VB_Vector& imag1, const VB_Vector& real2,
                       const VB_Vector& imag2, VB_Vector& realProd, VB_Vector& imagProd)
    throw (GenericExcep);

  /*********************************************************************
   * Static methods to compute complex FFT and inverse FFT.             *
   *********************************************************************/
  static void complexFFT(const VB_Vector& real,
                         const VB_Vector& imag, VB_Vector& realFFT, VB_Vector& imagFFT)
    throw (GenericExcep);
  static void complexIFFT(const VB_Vector& real,
                          const VB_Vector& imag, VB_Vector& realIFFT, VB_Vector& imagIFFT)
    throw (GenericExcep);
  static void complexIFFTReal(const VB_Vector& real,
                              const VB_Vector& imag, VB_Vector& realIFFT) throw (GenericExcep);

  double* begin() const;
  double* end() const;

  // FIXME : DYK : added the following stuff to handle the new i/o system
  int ReadFile(const string &fname);
  int WriteFile(string fname="");
  vector<string>header;       // unformatted text header
  void AddHeader(const string str) {header.push_back(str);}
  // DYK
  int permute(const VB_Vector &v);
  int permute(VBMatrix &m,int col);
  void sort() {if (theVector) gsl_sort_vector(theVector);}
}; // class VB_Vector

// DYK: nonmember functions

double ttest(const VB_Vector &v1,const VB_Vector &v2);
VB_Vector fftnyquist(VB_Vector &vv);
VB_Vector cspline_resize(VB_Vector vec,double factor);

double correlation(const VB_Vector &v1,const VB_Vector &v2);
double covariance(const VB_Vector &v1,const VB_Vector &v2);

#endif // VB_VECTOR_H
