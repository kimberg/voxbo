
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
// original version written by Kosh Banerjee
// changes/additions by Dan Kimberg

using namespace std;

#include <gsl/gsl_fit.h>
#include <gsl/gsl_statistics_double.h>
#include "vbio.h"

gsl_error_handler_t *VB_Vector::currentGSLErrorHandler = NULL;

VB_Vector::VB_Vector() {
  this->init(false, vb_double, "ref1");
  this->theVector = NULL;
}

VB_Vector::VB_Vector(const size_t len) {
  this->init(false, vb_double, "ref1");
  init(len);
}

void VB_Vector::clear() {
  if (this->valid) gsl_vector_free(this->theVector);
  this->init(false, vb_double, "ref1");
  this->theVector = NULL;
  this->valid = false;
}

void VB_Vector::print() {
  printf("vector:\n");
  for (size_t i = 0; i < size(); i++)
    printf("  %010d: %g\n", (int)i, getElement(i));
}

void VB_Vector::init(const size_t len) throw(GenericExcep) {
  if (this->valid) gsl_vector_free(this->theVector);
  this->valid = false;
  if (len > 0) {
    this->theVector = gsl_vector_calloc(len);
    if (this->theVector != NULL) this->valid = true;
  }
}

void VB_Vector::init(const bool validFlag, const VB_datatype dType,
                     const string signature) {
  init(validFlag, dType, findFileFormat(signature));
}

void VB_Vector::init(const bool validFlag, const VB_datatype dType,
                     const VBFF fType) {
  this->valid = validFlag;
  this->dataType = dType;
  this->fileFormat = fType;
}

/*********************************************************************
 * This method allocates memory to the input gsl_matrix struct.       *
 *********************************************************************/
gsl_matrix *VB_Vector::initMatrix(const size_t rows, const size_t cols) const
    throw(GenericExcep) {
  /*********************************************************************
   * Allocating the required memory to m. All elements of m will be     *
   * initialized to 0.                                                  *
   *********************************************************************/
  gsl_matrix *m = gsl_matrix_calloc(rows, cols);

  /*********************************************************************
   * If m is null, then we were unsuccessful in allocating memory for   *
   * it. Therefore, a GenericExcep is thrown.                           *
   *********************************************************************/
  if (m == NULL) {
    /*********************************************************************
     * errorMsg[] will hold an appropriate error message.                 *
     *********************************************************************/
    char errorMsg[OPT_STRING_LENGTH];
    memset(errorMsg, 0, OPT_STRING_LENGTH);

    /*********************************************************************
     * Populating errorMsg[] with the appropriate error message.          *
     *********************************************************************/
    sprintf(errorMsg,
            "The requested matrix size [%d x %d] could not be allocated.",
            (int)rows, (int)cols);

    /*********************************************************************
     * Now throwing the exception.                                        *
     *********************************************************************/
    throw GenericExcep(__LINE__, __FILE__, __FUNCTION__, errorMsg);

  }  // if

  /*********************************************************************
   * Now returning the address of the allocated gsl_matrix struct.      *
   *********************************************************************/
  return m;

}  // gsl_matrix *VB_Vector::initMatrix(const size_t rows, const size_t cols)
   // const throw (GenericExcep)

/*********************************************************************
 * This private method is used as a wrapper around gsl_vector_memcpy()*
 * to enable throwing an exception (if necessary).                    *
 *********************************************************************/
void VB_Vector::GSLVectorMemcpy(gsl_vector *dest, const gsl_vector *src) const
    throw(GenericExcep) {
  /*********************************************************************
   * gsl_vector_memcpy() is called to copy src to dest . It's return    *
   * value will be processed by VB_Vector::checkGSLStatus() and if the  *
   * return value is non-zero, then VB_Vector::checkGSLStatus() will    *
   * throw (and catch) an exception with an appropriate error message   *
   * (as returned by gsl_strerror()).                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_memcpy(dest, src), __LINE__, __FILE__,
                            __FUNCTION__);

}  // void VB_Vector::GSLVectorMemcpy(gsl_vector *dest, const gsl_vector *src)
   // const throw (GenericExcep)

/*********************************************************************
 * This private method throws an exception if the input index is not  *
 * within the range of the length of this instance of VB_Vector.      *
 *********************************************************************/
void VB_Vector::checkVectorRange(const size_t index, const int lineNumber,
                                 const char *fileName,
                                 const char *prettyFunctionName) const
    throw(GenericExcep) {
  /*********************************************************************
   * If the index is out of range of the vector length, then an         *
   * exception is thrown.                                               *
   *********************************************************************/
  if (index >= this->getLength()) {
    /*********************************************************************
     * errorMsg[] will hold the error message that the index is out of    *
     * range.                                                             *
     *********************************************************************/
    char errorMsg[OPT_STRING_LENGTH];
    memset(errorMsg, 0, OPT_STRING_LENGTH);

    /*********************************************************************
     * Populating errorMsg[] with the appropriate error message.          *
     *********************************************************************/
    sprintf(errorMsg, "The index [%d] is not in the vector range [0, %d].",
            (int)index, (int)(this->getLength() - 1));

    /*********************************************************************
     * Now throwing the exception.                                        *
     *********************************************************************/
    throw GenericExcep(lineNumber, fileName, prettyFunctionName, errorMsg);

  }  // if

}  // void VB_Vector::checkVectorRange(const size_t index, const int lineNumber,
   // const char *fileName, const char *prettyFunctionName) const throw
   // (GenericExcep)

/*********************************************************************
 * This private static method throws an exception if the lengths of   *
 * the 2 input gsl_vector structs are not equal.                      *
 *********************************************************************/
void VB_Vector::checkVectorLengths(
    const gsl_vector *V1, const gsl_vector *V2, const int lineNumber,
    const char *fileName, const char *prettyFunctionName) throw(GenericExcep) {
  /*********************************************************************
   * We now check to see that we don't have a NULL gsl_vector. If either*
   * gsl_vector argument is NULL, then an exception is thrown with an   *
   * appropriate error message.                                         *
   *********************************************************************/
  if (V1 == NULL || V2 == NULL) {
    ostringstream errorMsg;
    errorMsg << "Have a NULL gsl_vector in [" << __FUNCTION__ << "].";
    throw GenericExcep(lineNumber, fileName, prettyFunctionName,
                       errorMsg.str());
  }  // if

  /*********************************************************************
   * If the vector lengths do no match, then an exception is thrown.    *
   *********************************************************************/
  if (V1->size != V2->size) {
    /*********************************************************************
     * errorMsg[] will hold the error message that the vector lengths are *
     * not equal.                                                         *
     *********************************************************************/
    char errorMsg[OPT_STRING_LENGTH];
    memset(errorMsg, 0, OPT_STRING_LENGTH);

    /*********************************************************************
     * Populating errorMsg[] with the appropriate error message.          *
     *********************************************************************/
    sprintf(errorMsg, "Unequal vector lengths: [%d] and [%d]", (int)(V1->size),
            (int)(V2->size));

    /*********************************************************************
     * Now throwing the exception.                                        *
     *********************************************************************/
    throw GenericExcep(lineNumber, fileName, prettyFunctionName, errorMsg);

  }  // if

}  // void VB_Vector::checkVectorLengths(const gsl_vector *V1, const gsl_vector
   // *V2, const int lineNumber, const char *fileName, const char
   // *prettyFunctionName) throw (GenericExcep)

/*********************************************************************
 * This private static method throws an exception if the two input    *
 * size_t variables, len1 and len2, are not equal.                    *
 *********************************************************************/
void VB_Vector::checkVectorLengths(
    const size_t len1, const size_t len2, const int lineNumber,
    const char *fileName, const char *prettyFunctionName) throw(GenericExcep) {
  /*********************************************************************
   * If the vectors length do no match, then an exception is thrown.    *
   *********************************************************************/
  if (len1 != len2) {
    /*********************************************************************
     * errorMsg[] will hold the error message that the vector lengths are *
     * not equal.                                                         *
     *********************************************************************/
    char errorMsg[OPT_STRING_LENGTH];
    memset(errorMsg, 0, OPT_STRING_LENGTH);

    /*********************************************************************
     * Populating errorMsg[] with the appropriate error message.          *
     *********************************************************************/
    sprintf(errorMsg, "Unequal vector lengths: [%d] and [%d]", (int)len1,
            (int)len2);

    /*********************************************************************
     * Now throwing the exception.                                        *
     *********************************************************************/
    throw GenericExcep(lineNumber, fileName, prettyFunctionName, errorMsg);

  }  // if

}  // void VB_Vector::checkVectorLengths(const size_t len1, const size_t len2,
   // const int lineNumber, const char *fileName, const char
   // *prettyFunctionName) throw (GenericExcep)

/*********************************************************************
 * This private static method throws a GenericExcep if the input      *
 * gsl_vector struct is null.                                         *
 *********************************************************************/
void VB_Vector::vectorNull(const gsl_vector *v) throw(GenericExcep) {
  /*********************************************************************
   * If the input gsl_vector struct is null, then the exception is      *
   * thrown.                                                            *
   *********************************************************************/
  if (v == NULL) {
    throw GenericExcep(__LINE__, __FILE__, __FUNCTION__,
                       "ERROR: Unable to allocate memory for VB_Vector.");
  }  // if

}  // void VB_Vector::vectorNull(const gsl_vector *v) throw (GenericExcep)

/*********************************************************************
 * This private static method throws a GenericExcep and catches it    *
 * if the input variable status is non-zero. This method should be    *
 * called after a GSL function returns its status.                    *
 *********************************************************************/
void VB_Vector::checkGSLStatus(const int status, const int lineNumber,
                               const char *fileName,
                               const char *prettyFunctionName) {
  /*********************************************************************
   * If status is non-zero, then try/catch blocks are used to throw a   *
   * GenericExcep and then process it.                                  *
   *********************************************************************/
  if (status) {
    try {
      throw GenericExcep(lineNumber, fileName, prettyFunctionName,
                         gsl_strerror(status));
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(lineNumber, fileName, prettyFunctionName);
    }  // catch
  }    // if

}  // void VB_Vector::checkGSLStatus(const int status, const int lineNumber,
   // const char *fileName, const char *prettyFunctionName)

/*********************************************************************
 * This static method throws and catches an exception of type         *
 * GenericExcep.                                                      *
 *********************************************************************/
void VB_Vector::createException(const char *errorMsg, const int lineNumber,
                                const char *fileName,
                                const char *prettyFunctionName) {
  /*********************************************************************
   * Now throwing and catching the exception.                           *
   *********************************************************************/
  try {
    throw GenericExcep(lineNumber, fileName, prettyFunctionName, errorMsg);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(lineNumber, fileName, prettyFunctionName);
  }  // catch

}  // void VB_Vector::createException(const char *errorMsg, const int
   // lineNumber, const char *fileName, const char *prettyFunctionName)

/*********************************************************************
 * This static method throws and catches an exception of type         *
 * GenericExcep.                                                      *
 *********************************************************************/
void VB_Vector::createException(const string &errorMsg, const int lineNumber,
                                const string &fileName,
                                const string &prettyFunctionName) {
  /*********************************************************************
   * Now throwing the exception.                                        *
   *********************************************************************/
  VB_Vector::createException(errorMsg.c_str(), lineNumber, fileName.c_str(),
                             prettyFunctionName.c_str());

}  // void VB_Vector::createException(const string& errorMsg, const int
   // lineNumber, const string& fileName, const string& prettyFunctionName)

/*********************************************************************
 * This static method will call VB_Vector::createException() if any   *
 * elements of the input gsl_struct are Inf or Nan.                   *
 *********************************************************************/
void VB_Vector::checkFiniteness(const gsl_vector *v, const int lineNumber,
                                const char *fileName,
                                const char *prettyFunctionName) {
  /*********************************************************************
   * The following for loop is used to examine each element of the      *
   * input gsl_vector struct.                                           *
   *********************************************************************/
  for (size_t i = 0; i < v->size; i++) {
    /*********************************************************************
     * If the input double to gsl_finite() is an Inf or a Nan, then       *
     * gsl_finite() will return 0. Otherwise, 1 is returned.              *
     *********************************************************************/
    if (!gsl_finite(v->data[i])) {
      /*********************************************************************
       * errorMsg[] will hold the error message that an Inf or Nan was      *
       * encountered.                                                       *
       *********************************************************************/
      char errorMsg[OPT_STRING_LENGTH];
      memset(errorMsg, 0, OPT_STRING_LENGTH);

      /*********************************************************************
       * Populating errorMsg[] with the appropriate error message.          *
       *********************************************************************/
      sprintf(errorMsg, "The vector element at index [%d] is an Inf or a NaN.",
              (int)i);

      /*********************************************************************
       * Now VB_Vector::createException() is called to trhow and catch a    *
       * GenericExcep so that an appropriate error message gets displayed.  *
       *********************************************************************/
      VB_Vector::createException(errorMsg, lineNumber, fileName,
                                 prettyFunctionName);

      /*********************************************************************
       * We now break from the for loop since we have found at least 1 Inf  *
       * or Nan.                                                            *
       *********************************************************************/
      break;

    }  // if

  }  // for i

}  // void VB_Vector::checkFiniteness(const gsl_vector *v, const int lineNumber,
   // const char *fileName, const char *prettyFunctionName)

/*********************************************************************
 * This constructor uses the input variable len as the length of the  *
 * the vector and the input variable data as the elements of the      *
 * vector. NOTE: Is is expected that data is an array of doubles whose*
 * length is len.                                                     *
 *********************************************************************/
VB_Vector::VB_Vector(const double *data, const size_t len) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    this->init(len);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now copying the values from data to this->theVector->data.         *
   *********************************************************************/
  memcpy(this->theVector->data, data, this->theVector->size * sizeof(double));

}  // VB_Vector::VB_Vector(const double *data, const size_t len)

/*********************************************************************
 * This constructor builds a VB_Vector from the input gsl_vector      *
 * struct.                                                            *
 *********************************************************************/
VB_Vector::VB_Vector(const gsl_vector *V2) {
  /*********************************************************************
   * Setting this->fileName, this->valid, this->dataType, and           *
   * this->fileFormat. Since we don't have an actual file name, a       *
   * temporary file name is used.                                       *
   *********************************************************************/
  char tmpFileName[STRINGLEN];
  memset(tmpFileName, 0, STRINGLEN);
  strcpy(tmpFileName, "./tmp-");
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * Setting this->theVector to NULL.                                   *
   *********************************************************************/
  this->theVector = NULL;

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    /*********************************************************************
     * Allocating a gsl_vector struct and all the elements will be set to *
     * 0.                                                                 *
     *********************************************************************/
    this->init(V2->size);

  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now copying the values from V2.data to this->theVector->data. The  *
   * try/catch blocks are used to process the exception, if one is      *
   * thrown.                                                            *
   *********************************************************************/
  try {
    this->GSLVectorMemcpy(this->theVector, V2);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

}  // VB_Vector::VB_Vector(const gsl_vector *V2)

/*********************************************************************
 * This constructor builds a VB_Vector from the input gsl_vector      *
 * struct.                                                            *
 *********************************************************************/
VB_Vector::VB_Vector(const gsl_vector &V2) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * Setting this->theVector to NULL.                                   *
   *********************************************************************/
  this->theVector = NULL;

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    /*********************************************************************
     * Allocating a gsl_vector struct and all the elements will be set to *
     * 0.                                                                 *
     *********************************************************************/
    this->init(V2.size);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now copying the values from V2.data to this->theVector->data. The  *
   * try/catch blocks are used to process the exception, if one is      *
   * thrown.                                                            *
   *********************************************************************/
  try {
    this->GSLVectorMemcpy(this->theVector, &V2);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

}  // VB_Vector::VB_Vector(const gsl_vector& V2)

/*********************************************************************
 * This is the copy constructor.                                      *
 *********************************************************************/
VB_Vector::VB_Vector(const VB_Vector &V2) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, V2.dataType, V2.fileFormat);

  /*********************************************************************
   * Setting this->fileName to V2.fileName.                             *
   *********************************************************************/
  this->fileName = V2.fileName;

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    /*********************************************************************
     * If V2.theVector is not null, then a gsl_vector struct is allocated *
     * and all the elements of this->theVector->data[] will be set to 0.  *
     * Otherwise, this->theVector is set to NULL.                         *
     *********************************************************************/
    if (V2.theVector) {
      this->init(V2.theVector->size);

      /*********************************************************************
       * Now copying the values from V2.theVector to this->theVector. The   *
       * try/catch blocks are used to process the exception, if one is      *
       * thrown.                                                            *
       *********************************************************************/
      try {
        this->GSLVectorMemcpy(this->theVector, V2.theVector);
      }  // try
      catch (GenericExcep &e) {
        e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
      }  // catch

    }  // if
    else {
      this->theVector = NULL;
    }  // else

  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

}  // VB_Vector::VB_Vector(const VB_Vector& V2)

/*********************************************************************
 * This constructor instantiates a VB_Vector from the input VBVector  *
 * pointer.                                                           *
 *********************************************************************/
VB_Vector::VB_Vector(const VB_Vector *V) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, V->dataType, V->fileFormat);

  /*********************************************************************
   * Setting this->fileName to V->fileName.                             *
   *********************************************************************/
  this->fileName = V->fileName;

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    /*********************************************************************
     * If V->theVector is not null, then a gsl_vector struct is allocated *
     * and all the elements of this->theVector->data[] will be set to 0.  *
     * Otherwise, this->theVector is set to NULL.                         *
     *********************************************************************/
    if (V->theVector) {
      this->init(V->theVector->size);
    }  // if
    else {
      this->theVector = NULL;
    }  // else

  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now copying the values from V->theVector to this->theVector. The   *
   * try/catch blocks are used to process the exception, if one is      *
   * thrown.                                                            *
   *********************************************************************/
  try {
    this->GSLVectorMemcpy(this->theVector, V->theVector);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

}  // VB_Vector::VB_Vector(const VB_Vector *V)

VB_Vector::VB_Vector(const string &vecFile) {
  this->init(false, vb_double, "ref1");
  this->ReadFile(vecFile);
}

/*********************************************************************
 * Constructor to read in a vector from a VoxBo vector file.          *
 *********************************************************************/
VB_Vector::VB_Vector(const char *vecFile) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * Setting this->fileName.                                            *
   *********************************************************************/
  this->fileName = vecFile;

  /*********************************************************************
   * Reading the VoxBo vector file.                                     *
   *********************************************************************/
  if (this->ReadFile(vecFile)) {
    ostringstream errorMsg;
    errorMsg << "[" << __FUNCTION__ << "]: Unable to read the file [" << vecFile
             << "].";
    printErrorMsg(VB_WARNING, errorMsg.str());
  }  // if

}  // VB_Vector::VB_Vector(const char *vecFile)

/*********************************************************************
 * This constructor creates an instance of VB_Vector from a           *
 * reference to a vector <double>.                                    *
 *********************************************************************/
VB_Vector::VB_Vector(const vector<double> &theVector) {
  /*********************************************************************
   * Now setting this->valid (to false), this->dataType, and            *
   * this->fileFormat. Also, this->fileName will be set to temporary file *
   * name.                                                              *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    this->init(theVector.size());
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now the elements from theVector are copied to this->theVector.     *
   *********************************************************************/
  copy(theVector.begin(), theVector.end(), this->theVector->data);

}  // VB_Vector::VB_Vector(const vector<double>& theVector)

/*********************************************************************
 * This constructor creates an instance of VB_Vector from a           *
 * pointer to a vector <double>.                                      *
 *********************************************************************/
VB_Vector::VB_Vector(const vector<double> *theVector) {
  /*********************************************************************
   * Now setting this->valid (to false), this->dataType, and            *
   * this->fileFormat. Also, this->fileName will be set to temporary    *
   * file name.                                                         *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    this->init(theVector->size());
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now the elements from theVector are copied to this->theVector.     *
   *********************************************************************/
  copy(theVector->begin(), theVector->end(), this->theVector->data);

}  // VB_Vector::VB_Vector(const vector<double> *theVector)

// DYK: constructor from a binary bitmask, FIXME should probably be
// declared explicit
VB_Vector::VB_Vector(const bitmask &bm) {
  this->init(false, vb_double, "ref1");
  this->init(bm.size());
  for (size_t i = 0; i < bm.size(); i++) {
    if (bm[i])
      this->theVector->data[i] = 1;
    else
      this->theVector->data[i] = 0;
  }
}

/*********************************************************************
 * This constructor is used to quickly generate a VB_Vector object    *
 * from the time series stored at the specified index.                *
 *********************************************************************/
VB_Vector::VB_Vector(const Tes &theTes, const unsigned long tSeriesIndex) {
  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat.         *
   *********************************************************************/
  this->init(false, vb_double, "ref1");

  /*********************************************************************
   * We call this->init() and catch its exception, if one is thrown.    *
   *********************************************************************/
  try {
    this->init(theTes.dimt);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * If theTes.data[tSeriesIndex] is not NULL, then it holds a          *
   * non-zero time series. NOTE: If in fact theTes.data[tSeriesIndex]   *
   * is NULL, then we have a time series that is entirely zero.         *
   * Therefore, we do not need to do anything else since the above call *
   * to this->init() sets all the elements to 0.                        *
   *********************************************************************/
  if (theTes.data[tSeriesIndex]) {
    /*********************************************************************
     * We now switch on the possible data types for theTes.               *
     *********************************************************************/
    switch (theTes.datatype) {
      case vb_byte:

        /*********************************************************************
         * The following for loop assigns each element of this instance of    *
         * VB_Vector.                                                         *
         *********************************************************************/
        for (long i = 0; i < theTes.dimt; i++) {
          this->theVector->data[i] =
              *((unsigned char *)(theTes.data[tSeriesIndex] +
                                  (i * theTes.datasize)));
        }  // for i

        break;

      case vb_short:

        /*********************************************************************
         * The following for loop assigns each element of this instance of    *
         * VB_Vector.                                                         *
         *********************************************************************/
        for (long i = 0; i < theTes.dimt; i++) {
          this->theVector->data[i] =
              *((int16 *)(theTes.data[tSeriesIndex] + (i * theTes.datasize)));
        }  // for i

        break;

      case vb_long:

        /*********************************************************************
         * The following for loop assigns each element of this instance of    *
         * VB_Vector.                                                         *
         *********************************************************************/
        for (long i = 0; i < theTes.dimt; i++) {
          this->theVector->data[i] =
              *((int32 *)(theTes.data[tSeriesIndex] + (i * theTes.datasize)));
        }  // for i

        break;

      case vb_float:

        /*********************************************************************
         * The following for loop assigns each element of this instance of    *
         * VB_Vector.                                                         *
         *********************************************************************/
        for (long i = 0; i < theTes.dimt; i++) {
          this->theVector->data[i] =
              *((float *)(theTes.data[tSeriesIndex] + (i * theTes.datasize)));
        }  // for i

        break;

      case vb_double:

        /*********************************************************************
         * The following for loop assigns each element of this instance of    *
         * VB_Vector.                                                         *
         *********************************************************************/
        for (long i = 0; i < theTes.dimt; i++) {
          this->theVector->data[i] =
              *((double *)(theTes.data[tSeriesIndex] + (i * theTes.datasize)));
        }  // for i

        break;

    }  // switch

  }  // if

}  // VB_Vector::VB_Vector(const Tes& theTes, const unsigned long tSeriesIndex)

/*********************************************************************
 * Method to turn off the current GSL error handler.                  *
 *********************************************************************/
void VB_Vector::turnOffGSLErrorHandler() const {
  VB_Vector::currentGSLErrorHandler = gsl_set_error_handler_off();
}  // void VB_Vector::turnOffGSLErrorHandler() const

/*********************************************************************
 * Method to restore the GSL error handler to                         *
 * VB_Vector::->currentGSLErrorHandler.                               *
 *********************************************************************/
void VB_Vector::restoreGSLErrorHandler() const {
  /*********************************************************************
   * If VB_Vector::currentGSLErrorHandler is not null, then the GSL     *
   * error handler is restored to VB_Vector::currentGSLErrorHandler.    *
   *********************************************************************/
  if (VB_Vector::currentGSLErrorHandler) {
    gsl_set_error_handler(VB_Vector::currentGSLErrorHandler);
  }  // if

}  // void VB_Vector::restoreGSLErrorHandler() const

/*********************************************************************
 * The destructor.                                                    *
 *********************************************************************/
VB_Vector::~VB_Vector() {
  /*********************************************************************
   * If this->valid is true, then a gsl_vector struct was allocated to  *
   * this->theVector. Therefore, we delete the memory.                  *
   *********************************************************************/
  if (this->valid) {
    gsl_vector_free(this->theVector);
  }  // if

}  // VB_Vector::~VB_Vector()

/*********************************************************************
 * This method simply returns the specified element from the vector.  *
 *********************************************************************/
double VB_Vector::getElement(const size_t index) const {
  /*********************************************************************
   * We now check to make sure that index is within                     *
   * [0, this->theVector->size - 1]. The try/catch blocks are used to   *
   * process any exception, if one is thrown by                         *
   * this->checkVectorRange().                                          *
   *********************************************************************/
  try {
    this->checkVectorRange(index, __LINE__, __FILE__, __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now returning the specified element value.                         *
   *********************************************************************/
  return gsl_vector_get(this->theVector, index);

}  // double VB_Vector::getElement(const size_t index) const

/*********************************************************************
 * This method sets the specified element to the specified value.     *
 *********************************************************************/
void VB_Vector::setElement(const size_t index, const double value) {
  /*********************************************************************
   * We now check to make sure that index is within                     *
   * [0, this->theVector->size - 1]. The try/catch blocks are used to   *
   * process any exception, if one is thrown by                         *
   * this->checkVectorRange().                                          *
   *********************************************************************/
  try {
    this->checkVectorRange(index, __LINE__, __FILE__, __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Calling gsl_vector_set() to set the specified element to the       *
   * specified value.                                                   *
   *********************************************************************/
  gsl_vector_set(this->theVector, index, value);

}  // void VB_Vector::setElement(const size_t index, const double value)

/*********************************************************************
 * This method simply calculates the Euclidean inner product of this  *
 * instance of VB_Vector and the input VB_Vector, V2.                 *
 *********************************************************************/
double VB_Vector::euclideanProduct(const VB_Vector &V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(V2.theVector);

}  // double VB_Vector::euclideanProduct(const VB_Vector& V2) const

/*********************************************************************
 * This method simply calculates the Euclidean inner product of this  *
 * instance of VB_Vector and the input VB_Vector, V2.                 *
 *********************************************************************/
double VB_Vector::euclideanProduct(const VB_Vector *V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(V2->theVector);

}  // double VB_Vector::euclideanProduct(const VB_Vector *V2) const

/*********************************************************************
 * This method simply calculates the Euclidean inner product of this  *
 * instance of VB_Vector and the input gsl_vector struct, V2.         *
 *********************************************************************/
double VB_Vector::euclideanProduct(const gsl_vector &V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(&V2);

}  // double VB_Vector::euclideanProduct(const gsl_vector& V2) const

/*********************************************************************
 * This method simply calculates the Euclidean inner product of this  *
 * instance of VB_Vector and the input gsl_vector struct, V2.         *
 *********************************************************************/
double VB_Vector::euclideanProduct(const gsl_vector *V2) const {
  /*********************************************************************
   * By default, if gsl_blas_ddot() has an error, the GSL error         *
   * handler will be invoked, which calls abort(), creating a core dump.*
   * To forgo this behavior, the GSL error handler is turned off and    *
   * the current error handler is stored in                             *
   * VB_Vector::currentGSLErrorHandler. This is accomplished by the     *
   * call to this->turnOffGSLErrorHandler().                            *
   *********************************************************************/
  this->turnOffGSLErrorHandler();

  /*********************************************************************
   * The following try/catch blocks are used to ensure that the vector  *
   * lengths are equal.                                                 *
   *********************************************************************/
  try {
    VB_Vector::checkVectorLengths(this->theVector, V2, __LINE__, __FILE__,
                                  __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * result will hold the Euclidean inner product.                      *
   *********************************************************************/
  double result = 0.0;

  /*********************************************************************
   * Calling gsl_blas_ddot() to compute the Eucliden inner product.     *
   * It's return value will be processed by VB_Vector::checkGSLStatus() *
   * and if the return value is non-zero, then                          *
   * VB_Vector::checkGSLStatus() will throw (and catch) an exception    *
   * with an appropriate error message (as returned by gsl_strerror()). *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_blas_ddot(this->theVector, V2, &result),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * We now restore the standard GSL error handler by calling           *
   * this->restoreGSLErrorHandler().                                    *
   *********************************************************************/
  this->restoreGSLErrorHandler();

  /*********************************************************************
   * Returning the inner product.                                       *
   *********************************************************************/
  return result;

}  // double VB_Vector::euclideanProduct(const gsl_vector *V2) const

/*********************************************************************
 * This static method calculates the Euclidean inner product of the   *
 * 2 input gsl_vector structs.                                        *
 *********************************************************************/
double VB_Vector::euclideanProduct(const gsl_vector *V1, const gsl_vector *V2) {
  /*********************************************************************
   * Checking to see if the 2 input gsl_vector structs have the same    *
   * length or not.                                                     *
   *********************************************************************/
  try {
    VB_Vector::checkVectorLengths(V1, V2, __LINE__, __FILE__, __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * result will hold the Euclidean inner product.                      *
   *********************************************************************/
  double result = 0.0;

  /*********************************************************************
   * Calling gsl_blas_ddot() to compute the Eucliden inner product. It's*
   * return value will be processed by VB_Vector::checkGSLStatus(). If  *
   * the return value is non-zero, then VB_Vector::checkGSLStatus() will*
   * throw (and catch) an exception with an appropriate error message   *
   * (as returned by gsl_strerror()).                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_blas_ddot(V1, V2, &result), __LINE__, __FILE__,
                            __FUNCTION__);

  /*********************************************************************
   * Returning the Euclidean inner product.                             *
   *********************************************************************/
  return result;

}  // double VB_Vector::euclideanProduct(const gsl_vector *V1, const gsl_vector
   // *V2)

/*********************************************************************
 * This static method calculates the Euclidean inner product of the   *
 * 2 input gsl_vector structs.                                        *
 *********************************************************************/
double VB_Vector::euclideanProduct(const gsl_vector &V1, const gsl_vector &V2) {
  /*********************************************************************
   * Calling VB_Vector::euclideanProduct(const gsl_vector *,            *
   * const gsl_vector *) to return the Euclidean inner product.         *
   *********************************************************************/
  return VB_Vector::euclideanProduct(&V1, &V2);

}  // double VB_Vector::euclideanProduct(const gsl_vector& V1, const gsl_vector&
   // V2)

/*********************************************************************
 * This overloaded operator returns the vector sum of this instance   *
 * of VB_Vector and the input gsl_vector.                             *
 *********************************************************************/
VB_Vector VB_Vector::operator+(const gsl_vector *V2) const {
  /*********************************************************************
   * If the vector lengths are not equal, then a GenericExcep is thrown.*
   * The try/catch blocks are used to process the exception, which will *
   * have an appropriate error message.                                 *
   *********************************************************************/
  try {
    VB_Vector::checkVectorLengths(this->theVector, V2, __LINE__, __FILE__,
                                  __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * vectorSum is instantiated to hold the vector sum.                  *
   *********************************************************************/
  VB_Vector vectorSum = VB_Vector(this->theVector->size);

  /*********************************************************************
   * Now copying the values from this->theVector to vectorSum.theVector.*
   * The try/catch blocks are used to process the exception, if one is  *
   * thrown.                                                            *
   *********************************************************************/
  try {
    this->GSLVectorMemcpy(vectorSum.theVector, this->theVector);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * gsl_vector_add() is called to compute the vector sum and its       *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then an error occurred with the GSL      *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add(vectorSum.theVector, V2), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning the vector sum.                                      *
   *********************************************************************/
  return vectorSum;

}  // VB_Vector VB_Vector::operator+(const gsl_vector *V2) const

/*********************************************************************
 * This overloaded operator returns the vector sum of this instance   *
 * of VB_Vector and the input VB_Vector.                              *
 *********************************************************************/
VB_Vector VB_Vector::operator+(const VB_Vector &V2) const {
  /*********************************************************************
   * Returning ((*this) + V2.theVector).                                *
   *********************************************************************/
  return ((*this) + V2.theVector);

}  // VB_Vector VB_Vector::operator+(const VB_Vector& V2) const

/*********************************************************************
 * This overloaded operator returns the vector difference of this     *
 * instance of VB_Vector and the input gsl_vector.                    *
 *********************************************************************/
VB_Vector VB_Vector::operator-(const gsl_vector *V2) const {
  /*********************************************************************
   * If the vector lengths are not equal, then a GenericExcep is thrown.*
   * The try/catch blocks are used to process the exception, which will *
   * have an appropriate error message.                                 *
   *********************************************************************/
  try {
    VB_Vector::checkVectorLengths(this->theVector, V2, __LINE__, __FILE__,
                                  __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * vectorDiff is instantiated to hold the vector difference.          *
   *********************************************************************/
  VB_Vector vectorDiff = VB_Vector(this->theVector->size);

  /*********************************************************************
   * Now copying the values from this->theVector to                     *
   * vectorDiff.theVector. The try/catch blocks are used to process the *
   * exception, if one is thrown.                                       *
   *********************************************************************/
  try {
    this->GSLVectorMemcpy(vectorDiff.theVector, this->theVector);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * gsl_vector_sub() is called to compute the vector difference and its*
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then an error occurred with the GSL      *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_sub(vectorDiff.theVector, V2), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning the vector difference.                               *
   *********************************************************************/
  return vectorDiff;

}  // VB_Vector VB_Vector::operator-(const gsl_vector *V2) const

/*********************************************************************
 * This overloaded operator returns the vector difference of this     *
 * instance of VB_Vector and the input VB_Vector.                     *
 *********************************************************************/
VB_Vector VB_Vector::operator-(const VB_Vector &V2) const {
  /*********************************************************************
   * Returning ((*this) - V2.theVector).                                *
   *********************************************************************/
  return ((*this) - V2.theVector);

}  // VB_Vector VB_Vector::operator-(const VB_Vector& V2) const

/*********************************************************************
 * This overloaded operator returns the vector difference of this     *
 * instance of VB_Vector and the input VB_Vector.                     *
 *********************************************************************/
VB_Vector VB_Vector::operator-(const VB_Vector *V2) const {
  /*********************************************************************
   * Returning the difference of (*this) and V2->theVector.             *
   *********************************************************************/
  return ((*this) - V2->theVector);

}  // VB_Vector VB_Vector::operator-(const VB_Vector *V2) const

/*********************************************************************
 * This overloaded operator returns the vector difference of this     *
 * instance of VB_Vector and the input gsl_vector struct.             *
 *********************************************************************/
VB_Vector VB_Vector::operator-(const gsl_vector &V2) const {
  /*********************************************************************
   * Returning the difference of (*this) and (&V2).                     *
   *********************************************************************/
  return ((*this) - (&V2));

}  // VB_Vector VB_Vector::operator-(const gsl_vector& V2) const

/*********************************************************************
 * This is the overloaded [] operator. NOTE: A reference is returned, *
 * allowing assignment.                                               *
 *********************************************************************/
double &VB_Vector::operator[](const size_t index) const {
  /*********************************************************************
   * Returning the specified reference.                                 *
   *********************************************************************/
  return this->theVector->data[index];

}  // double&  VB_Vector::operator[](const size_t index) const

/*********************************************************************
 * Overloaded "()" operator. Behaves just like the overloaded "[]"    *
 * operator for this class except index is range checked.             *
 *********************************************************************/
double &VB_Vector::operator()(const size_t index) const {
  /*********************************************************************
   * A try/catch block is used to determine if index is out of range or *
   * not.                                                               *
   *********************************************************************/
  try {
    /*********************************************************************
     * We now check to make sure that index is within                     *
     * [0, this->theVector->size - 1].                                    *
     *********************************************************************/
    this->checkVectorRange(index, __LINE__, __FILE__, __FUNCTION__);

  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Returning the specified reference.                                 *
   *********************************************************************/
  return this->theVector->data[index];

}  // double& VB_Vector::operator()(const size_t index) const

/*********************************************************************
 * This overloaded operator checks for equality between this instance *
 * of VB_Vector and the input VB_Vector.                              *
 *********************************************************************/
bool VB_Vector::operator==(const VB_Vector *V2) const {
  /*********************************************************************
   * Returning the value of the call to operator==(gsl_vector *).       *
   *********************************************************************/
  return ((*this) == V2->theVector);

}  // bool VB_Vector::operator==(const VB_Vector *V2) const

/*********************************************************************
 * This overloaded operator checks for equality between this instance *
 * of VB_Vector and the input VB_Vector.                              *
 *********************************************************************/
bool VB_Vector::operator==(const VB_Vector &V2) const {
  /*********************************************************************
   * Returning the value of the call to operator==(gsl_vector *).       *
   *********************************************************************/
  return ((*this) == V2.theVector);

}  // bool VB_Vector::operator==(const VB_Vector& V2) const

/*********************************************************************
 * This overloaded operator checks for equality between this instance *
 * of VB_Vector and the input gsl_vector struct.                      *
 *********************************************************************/
bool VB_Vector::operator==(const gsl_vector *V2) const {
  /*********************************************************************
   * If both this->theVector and V2 are NULL, then true is returned.    *
   *********************************************************************/
  if (this->theVector == NULL && V2 == NULL) return true;

  /*********************************************************************
   * If one of {this->theVector, V2} is NULL and the other is not, then *
   * false is returned.                                                 *
   *********************************************************************/
  if ((this->theVector && V2 == NULL) || (this->theVector == NULL && V2))
    return false;

  /*********************************************************************
   * If this->theVector->size and V2->size are not equal, then false is *
   * returned.                                                          *
   *********************************************************************/
  if (this->theVector->size != V2->size) {
    return false;
  }  // if

  for (size_t i = 0; i < size(); i++)
    if (abs((*this)[i] - gsl_vector_get(V2, i)) > DBL_MIN) return 0;
  return 1;
}  // bool VB_Vector::operator==(const gsl_vector *V2) const

/*********************************************************************
 * This overloaded operator checks for equality between this instance *
 * of VB_Vector and the input gsl_vector struct.                      *
 *********************************************************************/
bool VB_Vector::operator==(const gsl_vector &V2) const {
  /*********************************************************************
   * Returning the value of the call to operator==(gsl_vector *).       *
   *********************************************************************/
  return ((*this) == (&V2));

}  // bool VB_Vector::operator==(const gsl_vector& V2) const

/*********************************************************************
 * This overloaded operator checks for inequality between this        *
 * instance of VB_Vector and the input VB_Vector.                     *
 *********************************************************************/
bool VB_Vector::operator!=(const VB_Vector *V2) const {
  /*********************************************************************
   * Returning the negation of the equality check.                      *
   *********************************************************************/
  return !((*this) == V2);

}  // bool VB_Vector::operator!=(const VB_Vector *V2) const

/*********************************************************************
 * This overloaded operator checks for inequality between this        *
 * instance of VB_Vector and the input VB_Vector.                     *
 *********************************************************************/
bool VB_Vector::operator!=(const VB_Vector &V2) const {
  /*********************************************************************
   * Returning the negation of the equality check.                       *
   *********************************************************************/
  return !((*this) == V2);

}  // bool VB_Vector::operator!=(const VB_Vector& V2) const

/*********************************************************************
 * This overloaded operator checks for inequality between this        *
 * instance of VB_Vector and the input gsl_vector struct.             *
 *********************************************************************/
bool VB_Vector::operator!=(const gsl_vector &V2) const {
  /*********************************************************************
   * Returning the negation of the equality check.                      *
   *********************************************************************/
  return !((*this) == V2);

}  // bool VB_Vector::operator!=(const gsl_vector& V2) const

/*********************************************************************
 * This overloaded operator checks for inequality between this        *
 * instance of VB_Vector and the input gsl_vector struct.             *
 *********************************************************************/
bool VB_Vector::operator!=(const gsl_vector *V2) const {
  /*********************************************************************
   * Returning the negation of the equality check.                      *
   *********************************************************************/
  return !((*this) == V2);

}  // bool VB_Vector::operator!=(const gsl_vector *V2) const

const VB_Vector &VB_Vector::operator=(const VB_Vector &V2) {
  if (this == (&V2)) return *this;
  if (V2.getLength() == 0) {
    clear();
    return *this;
  }
  this->init(this->valid, V2.dataType, V2.fileFormat);
  this->init(V2.getLength());
  if (!(this->theVector)) return *this;
  this->fileName = V2.fileName;
  this->GSLVectorMemcpy(this->theVector, V2.theVector);
  return *this;
}

const VB_Vector &VB_Vector::zero() {
  gsl_vector_set_zero(this->theVector);
  return *this;
}

/*********************************************************************
 * This overloaded operator returns the vector sum of this instance   *
 * of VB_Vector and the input VB_Vector.                              *
 *********************************************************************/
VB_Vector VB_Vector::operator+(const VB_Vector *V2) const {
  /*********************************************************************
   * Returning the sum of (*this) and V2->theVector.                    *
   *********************************************************************/
  return ((*this) + V2->theVector);

}  // VB_Vector VB_Vector::operator+(const VB_Vector *V2) const

/*********************************************************************
 * This overloaded operator returns the vector sum of this instance   *
 * of VB_Vector and the input gsl_vector struct.                      *
 *********************************************************************/
VB_Vector VB_Vector::operator+(const gsl_vector &V2) const {
  /*********************************************************************
   * Returning the sum of (*this) and (&V2).                            *
   *********************************************************************/
  return ((*this) + (&V2));

}  // VB_Vector VB_Vector::operator+(const gsl_vector& V2) const

/*********************************************************************
 * This overloaded "+" operator is a friend function. This overloaded *
 * operator is implemented to preserve the commutative property for   *
 * addition between VB_Vector and gsl_vector.                         *
 *********************************************************************/
VB_Vector operator+(const gsl_vector *V1, const VB_Vector &V2) {
  /*********************************************************************
   * Returning the sum of V2 and V1.                                    *
   *********************************************************************/
  return (V2 + V1);

}  // VB_Vector operator+(const gsl_vector *V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded "+" operator is a friend function. This overloaded *
 * operator is implemented to preserve the commutative property for   *
 * addition between VB_Vector and gsl_vector.                         *
 *********************************************************************/
VB_Vector operator+(const gsl_vector &V1, const VB_Vector *V2) {
  /*********************************************************************
   * Returning the sum of V2 and V1.                                    *
   *********************************************************************/
  return ((*V2) + (&V1));

}  // VB_Vector operator+(const gsl_vector& V1, const VB_Vector *V2)

/*********************************************************************
 * This overloaded "+" operator is a friend function. This overloaded *
 * operator is implemented to preserve the commutative property for   *
 * addition between VB_Vector and gsl_vector.                         *
 *********************************************************************/
VB_Vector operator+(const gsl_vector &V1, const VB_Vector &V2) {
  /*********************************************************************
   * Returning the sum of V2 and V1.                                    *
   *********************************************************************/
  return (V2 + (&V1));

}  // VB_Vector operator+(const gsl_vector& V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded operator is implemented as a friend function so    *
 * that commutativity can be preserved for "==".                      *
 *********************************************************************/
bool operator==(const gsl_vector *V1, const VB_Vector &V2) {
  /*********************************************************************
   * If V2.theVector->size and V1->size are not equal, then false is    *
   * returned.                                                          *
   *********************************************************************/
  if (V2.getLength() != V1->size) {
    return false;
  }  // if
  for (size_t i = 0; i < V1->size; i++)
    if (abs(gsl_vector_get(V1, i) - V2[i]) > DBL_MIN) return 0;
  return 1;
}  // bool operator==(const gsl_vector *V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded operator is implemented as a friend function so    *
 * that commutativity can be preserved for "==".                      *
 *********************************************************************/
bool operator==(const gsl_vector &V1, const VB_Vector &V2) {
  for (size_t i = 0; i < V1.size; i++)
    if (abs(gsl_vector_get(&V1, i) - V2[i]) > DBL_MIN) return 0;
  return 1;
}  // bool operator==(const gsl_vector& V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded "!=" is implemented as a friend function to        *
 * preserve commutativity.                                            *
 *********************************************************************/
bool operator!=(const gsl_vector *V1, const VB_Vector &V2) {
  /*********************************************************************
   * Returning the negation of the equality check.                      *
   *********************************************************************/
  return !(V2 == V1);

}  // bool operator!=(const gsl_vector *V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded "!=" is implemented as a friend function to        *
 * preserve commutativity.                                            *
 *********************************************************************/
bool operator!=(const gsl_vector &V1, const VB_Vector &V2) {
  /*********************************************************************
   * Returning the negation of the equality check.                      *
   *********************************************************************/
  return !(V2 == V1);

}  // bool operator!=(const gsl_vector& V1, const VB_Vector& V2)

/*********************************************************************
 * This overloaded "<<" operator is implemented as a friend function. *
 *********************************************************************/
ostream &operator<<(ostream &outStream, const VB_Vector &V) {
  /*********************************************************************
   * Now calling operator<<(ostream&, const VB_Vector *), whose return  *
   * value, in turn, is returned by this function.                      *
   *********************************************************************/
  return (outStream << (&V));

}  // ostream& operator<<(ostream& outStream, const VB_Vector& V)

/*********************************************************************
 * This overloaded "<<" operator is implemented as a friend function. *
 *********************************************************************/
ostream &operator<<(ostream &outStream, const VB_Vector *V) {
  /*********************************************************************
   * Printing out the file name, valid flag, data type, and file type   *
   * data members.                                                      *
   *********************************************************************/
  outStream << "Vector File Name  = [" << V->fileName << "]" << endl;
  outStream << "Vector Valid      = [" << V->valid << "]" << endl;
  outStream << "Vector Data Type  = [" << DataTypeName(V->dataType) << "]"
            << endl;
  // DYK replaced the following
  // outStream << "Vector File Type  = [" << (V->fileFormat ?
  // V->fileFormat->getName() : "NONE") << "]" << endl;
  outStream << "Vector File Type  = [" << V->fileFormat.getName() << "]"
            << endl;

  /*********************************************************************
   * If the input VB_Vector does not have a null theVector data member, *
   * then the vector elements are printed out.                          *
   *********************************************************************/
  if (V->theVector) {
    /*********************************************************************
     * Writing out the fields from the data member theVector, a           *
     * gsl_vector struct.                                                 *
     *********************************************************************/
    outStream << "gsl_vector stride = [" << V->theVector->stride << "]" << endl;
    outStream << "gsl_vector owner  = [" << V->theVector->owner << "]" << endl;
    outStream << "Vector Length     = [" << V->theVector->size << "]" << endl;

    /*********************************************************************
     * The following for loop is used to write out the vector elements.   *
     *********************************************************************/
    for (size_t i = 0; i < V->theVector->size; i++) {
      outStream << "element[" << i << "] = [" << V->theVector->data[i] << "]"
                << endl;
    }  // for i

  }  // if

  /*********************************************************************
   * If program flow ends up here, then V->getTheVector() returned a    *
   * null gsl_vector.                                                   *
   *********************************************************************/
  else {
    outStream << "NULL gsl_vector." << endl;
  }

  /*********************************************************************
   * Now returning outStream.                                           *
   *********************************************************************/
  return outStream;

}  // ostream& operator<<(ostream& outStream, const VB_Vector *V)

VB_Vector::operator bitmask() {
  bitmask bm;
  bm.resize(size());
  bm.clear();
  for (size_t i = 0; i < size(); i++)
    if (fabs((*this)[i]) > FLT_MIN) bm.set(i);
  return bm;
}

/*********************************************************************
 * This method scales the elements of this instance of VB_Vector by   *
 * alpha in place.                                                    *
 *********************************************************************/
void VB_Vector::scaleInPlace(const double alpha) {
  /*********************************************************************
   * gsl_vector_scale() is called to do the scaling and its             *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_scale(this->theVector, alpha), __LINE__,
                            __FILE__, __FUNCTION__);

}  // void VB_Vector::scaleInPlace(const double alpha)

/*********************************************************************
 * This overloaded "*" operator returns the Euclidean inner product of*
 * this instance of VB_Vector with the input vector.                  *
 *********************************************************************/
double VB_Vector::operator*(const VB_Vector *V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(V2->theVector);

}  // double VB_Vector::operator*(const VB_Vector *V2) const

/*********************************************************************
 * This overloaded "*" operator returns the Euclidean inner product of*
 * this instance of VB_Vector with the input vector.                  *
 *********************************************************************/
double VB_Vector::operator*(const VB_Vector &V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(V2.theVector);

}  // double VB_Vector::operator*(const VB_Vector& V2) const

/*********************************************************************
 * This overloaded "*" operator returns the Euclidean inner product of*
 * this instance of VB_Vector with the input vector.                  *
 *********************************************************************/
double VB_Vector::operator*(const gsl_vector &V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(&V2);

}  // double VB_Vector::operator*(const gsl_vector& V2) const

/*********************************************************************
 * This overloaded "*" operator returns the Euclidean inner product of*
 * this instance of VB_Vector with the input vector.                  *
 *********************************************************************/
double VB_Vector::operator*(const gsl_vector *V2) const {
  /*********************************************************************
   * Now returning the Euclidean inner product.                         *
   *********************************************************************/
  return this->euclideanProduct(V2);

}  // double VB_Vector::operator*(const gsl_vector& V2) const

/*********************************************************************
 * Overloaded "*" operator (a friend function) that returns the       *
 * Euclidean inner product of the 2 input vectors.                    *
 *********************************************************************/
double operator*(const gsl_vector *V1, const VB_Vector &V2) {
  /*********************************************************************
   * Now calling V2.euclideanProduct(V1) to compute the Euclidean inner *
   * product.                                                           *
   *********************************************************************/
  return V2.euclideanProduct(V1);

}  // double operator*(const gsl_vector *V1, const VB_Vector& V2)

/*********************************************************************
 * Overloaded "*" operator (a friend function) that returns the       *
 * Euclidean inner product of the 2 input vectors.                    *
 *********************************************************************/
double operator*(const gsl_vector &V1, const VB_Vector &V2) {
  /*********************************************************************
   * Now calling V2.euclideanProduct(V1) to compute the Euclidean inner *
   * product.                                                           *
   *********************************************************************/
  return V2.euclideanProduct(&V1);

}  // double operator*(const gsl_vector& V1, const VB_Vector& V2)

/*********************************************************************
 * Overloaded "*" operator (a friend function) that returns the       *
 * Euclidean inner product of the 2 input vectors.                    *
 *********************************************************************/
double operator*(const gsl_vector &V1, const VB_Vector *V2) {
  /*********************************************************************
   * Now calling V2.euclideanProduct(V1) to compute the Euclidean inner *
   * product.                                                           *
   *********************************************************************/
  return V2->euclideanProduct(&V1);

}  // double operator*(const gsl_vector& V1, const VB_Vector *V2)

/*********************************************************************
 * This overloaded operator adds the input scalar to all elements of  *
 * this instance of VB_Vector, in place.                              *
 *********************************************************************/
VB_Vector &VB_Vector::operator+=(const double alpha) {
  /*********************************************************************
   * gsl_vector_add_constant() is called to add the constant and its    *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add_constant(this->theVector, alpha),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator+=(const double alpha)

/*********************************************************************
 * This overloaded operator subtracts the input scalar to all         *
 * elements of this instance of VB_Vector, in place.                  *
 *********************************************************************/
VB_Vector &VB_Vector::operator-=(const double alpha) {
  /*********************************************************************
   * gsl_vector_add_constant() is called to subtract the constant and   *
   * its return value is passed to VB_Vector::checkGSLStatus(). If the  *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(
      gsl_vector_add_constant(this->theVector, -1.0 * alpha), __LINE__,
      __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator-=(const double alpha)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the input scalar, in place.                        *
 *********************************************************************/
VB_Vector &VB_Vector::operator*=(const double alpha) {
  /*********************************************************************
   * gsl_vector_scale() is called to do the scaling and its             *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_scale(this->theVector, alpha), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator*=(const double alpha)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the reciprocal of the input scalar, in place.      *
 *********************************************************************/
VB_Vector &VB_Vector::operator/=(const double alpha) {
  /*********************************************************************
   * If alpha is 0.0, then an exception is thrown.                      *
   *********************************************************************/
  if (alpha == 0.0) {
    /*********************************************************************
     * Calling VB_Vector::createException() to throw and catch a          *
     * GenericExcep so that the appropriate error messaqge can be printed.*
     *********************************************************************/
    VB_Vector::createException(string("Can not divide by a zero scalar value."),
                               __LINE__, __FILE__, __FUNCTION__);

  }  // if

  /*********************************************************************
   * Calling gsl_vector_scale() to carry out the scaling by the         *
   * reciprocal. status will hold the return value of                   *
   * gsl_vector_scale().                                                *
   *********************************************************************/
  double reciprocal = 1.0 / alpha;

  /*********************************************************************
   * gsl_vector_scale() is called to do the needed scaling and its      *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_scale(this->theVector, reciprocal),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * VB_Vector::checkFiniteness() is called to ensure that all elements *
   * of this->theVector are finite, i.e., no Inf's nor Nan's.           *
   *********************************************************************/
  VB_Vector::checkFiniteness(this->theVector, __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator/=(const double alpha)

/*********************************************************************
 * This overloaded operator adds the input vector to this instance of *
 * VB_Vector, in place.                                               *
 *********************************************************************/
VB_Vector &VB_Vector::operator+=(const VB_Vector &V) {
  /*********************************************************************
   * gsl_vector_add() is called to compute the vector sum and its       *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add(this->theVector, V.theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator+=(const VB_Vector& V)

/*********************************************************************
 * This overloaded operator subtracts the input vector from this      *
 * instance of VB_Vector, in place.                                   *
 *********************************************************************/
VB_Vector &VB_Vector::operator-=(const VB_Vector &V) {
  /*********************************************************************
   * gsl_vector_sub() is called to compute the vector difference and its*
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_sub(this->theVector, V.theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator-=(const VB_Vector& V)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator*=(const VB_Vector &V) {
  /*********************************************************************
   * gsl_vector_mul() is called to compute the element by element       *
   * product and its return value is passed to                          *
   * VB_Vector::checkGSLStatus(). If the return value is non-zero, then *
   * a error occurred with the GSL function. VB_Vector::checkGSLStatus()*
   * will then process the error by writing out an appropriate error    *
   * message and then causing this program to exit.                     *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_mul(this->theVector, V.theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator*=(const VB_Vector& V)

/*********************************************************************
 * This overloaded operator divides each element of this instance     *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator/=(const VB_Vector &V) {
  /*********************************************************************
   * gsl_vector_div() is called to do the element by element division   *
   * and its return value is passed to VB_Vector::checkGSLStatus(). If  *
   * the return value is non-zero, then a error occurred with the GSL   *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_div(this->theVector, V.theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * VB_Vector::checkFiniteness() is called to ensure that all elements *
   * of this->theVector are finite, i.e., no Inf's nor Nan's.           *
   *********************************************************************/
  VB_Vector::checkFiniteness(this->theVector, __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator/=(const VB_Vector& V)

/*********************************************************************
 * This overloaded operator adds the input vector to this instance of *
 * VB_Vector, in place.                                               *
 *********************************************************************/
VB_Vector &VB_Vector::operator+=(const VB_Vector *V) {
  /*********************************************************************
   * gsl_vector_add() is called to compute the vector sum and its       *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add(this->theVector, V->theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator+=(const VB_Vector *V)

/*********************************************************************
 * This overloaded operator subtracts the input vector from this      *
 * instance of VB_Vector, in place.                                   *
 *********************************************************************/
VB_Vector &VB_Vector::operator-=(const VB_Vector *V) {
  /*********************************************************************
   * gsl_vector_sub() is called to compute the vector difference and its*
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_sub(this->theVector, V->theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator-=(const VB_Vector *V)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator*=(const VB_Vector *V) {
  /*********************************************************************
   * gsl_vector_mul() is called to compute the element by element       *
   * product and its return value is passed to                          *
   * VB_Vector::checkGSLStatus(). If the return value is non-zero, then *
   * a error occurred with the GSL function. VB_Vector::checkGSLStatus()*
   * will then process the error by writing out an appropriate error    *
   * message and then causing this program to exit.                     *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_mul(this->theVector, V->theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator*=(const VB_Vector *V)

/*********************************************************************
 * This overloaded operator divides each element of this instance     *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator/=(const VB_Vector *V) {
  /*********************************************************************
   * gsl_vector_div() is called to do the element by element division   *
   * and its return value is passed to VB_Vector::checkGSLStatus(). If  *
   * the return value is non-zero, then a error occurred with the GSL   *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_div(this->theVector, V->theVector),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * VB_Vector::checkFiniteness() is called to ensure that all elements *
   * of this->theVector are finite, i.e., no Inf's nor Nan's.           *
   *********************************************************************/
  VB_Vector::checkFiniteness(this->theVector, __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator/=(const VB_Vector *V)

/*********************************************************************
 * This overloaded operator adds the input vector to this instance of *
 * VB_Vector, in place.                                               *
 *********************************************************************/
VB_Vector &VB_Vector::operator+=(const gsl_vector *V) {
  /*********************************************************************
   * gsl_vector_add() is called to compute the vector sum and its       *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add(this->theVector, V), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator+=(const gsl_vector *V)

/*********************************************************************
 * This overloaded operator subtracts the input vector from this      *
 * instance of VB_Vector, in place.                                   *
 *********************************************************************/
VB_Vector &VB_Vector::operator-=(const gsl_vector *V) {
  /*********************************************************************
   * gsl_vector_sub() is called to compute the vector difference and its*
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_sub(this->theVector, V), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator-=(const gsl_vector *V)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator*=(const gsl_vector *V) {
  /*********************************************************************
   * gsl_vector_mul() is called to compute the element by element       *
   * product and its return value is passed to                          *
   * VB_Vector::checkGSLStatus(). If the return value is non-zero, then *
   * a error occurred with the GSL function. VB_Vector::checkGSLStatus()*
   * will then process the error by writing out an appropriate error    *
   * message and then causing this program to exit.                     *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_mul(this->theVector, V), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator*=(const gsl_vector *V)

/*********************************************************************
 * This overloaded operator divides each element of this instance     *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator/=(const gsl_vector *V) {
  /*********************************************************************
   * gsl_vector_div() is called to do the element by element division   *
   * and its return value is passed to VB_Vector::checkGSLStatus(). If  *
   * the return value is non-zero, then a error occurred with the GSL   *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_div(this->theVector, V), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * VB_Vector::checkFiniteness() is called to ensure that all elements *
   * of this->theVector are finite, i.e., no Inf's nor Nan's.           *
   *********************************************************************/
  VB_Vector::checkFiniteness(this->theVector, __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator/=(const gsl_vector *V)

/*********************************************************************
 * This overloaded operator adds the input vector to this instance of *
 * VB_Vector, in place.                                               *
 *********************************************************************/
VB_Vector &VB_Vector::operator+=(const gsl_vector &V) {
  /*********************************************************************
   * gsl_vector_add() is called to compute the vector sum and its       *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_add(this->theVector, (&V)), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator+=(const gsl_vector& V)

/*********************************************************************
 * This overloaded operator subtracts the input vector from this      *
 * instance of VB_Vector, in place.                                   *
 *********************************************************************/
VB_Vector &VB_Vector::operator-=(const gsl_vector &V) {
  /*********************************************************************
   * gsl_vector_sub() is called to compute the vector difference and its*
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_sub(this->theVector, (&V)), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator-=(const gsl_vector& V)

/*********************************************************************
 * This overloaded operator multiplies each element of this instance  *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator*=(const gsl_vector &V) {
  /*********************************************************************
   * gsl_vector_mul() is called to compute the element by element       *
   * product and its return value is passed to                          *
   * VB_Vector::checkGSLStatus(). If the return value is non-zero, then *
   * a error occurred with the GSL function. VB_Vector::checkGSLStatus()*
   * will then process the error by writing out an appropriate error    *
   * message and then causing this program to exit.                     *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_mul(this->theVector, (&V)), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator*=(const gsl_vector& V)

/*********************************************************************
 * This overloaded operator divides each element of this instance     *
 * of VB_Vector by the corresponding element of the input vector, in  *
 * place.                                                             *
 *********************************************************************/
VB_Vector &VB_Vector::operator/=(const gsl_vector &V) {
  /*********************************************************************
   * gsl_vector_div() is called to do the element by element division   *
   * and its return value is passed to VB_Vector::checkGSLStatus(). If  *
   * the return value is non-zero, then a error occurred with the GSL   *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_div(this->theVector, (&V)), __LINE__,
                            __FILE__, __FUNCTION__);

  /*********************************************************************
   * VB_Vector::checkFiniteness() is called to ensure that all elements *
   * of this->theVector are finite, i.e., no Inf's nor Nan's.           *
   *********************************************************************/
  VB_Vector::checkFiniteness(this->theVector, __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning a reference to this instance of VB_Vector.           *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator/=(const gsl_vector& V)

/*********************************************************************
 * This method resizes the vector to the desired new length and all   *
 * of the vector elements will be set to zero.                        *
 *********************************************************************/
void VB_Vector::resize(size_t newLength) {
  /*********************************************************************
   * If this->theVector is NULL (no gsl_vector was allocated) or if the *
   * current size of this instance of VB_Vector is not equal to the     *
   * new desired length, then a resizing is carried out.                *
   *********************************************************************/
  if (this->theVector == NULL || (this->theVector->size != newLength)) {
    /*********************************************************************
     * Calling this->init() to do the resizing. All vector elements will  *
     * be set to zero. try/catch blocks are used to process any exception *
     * thrown by this->init().                                            *
     *********************************************************************/
    try {
      this->init(newLength);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch
  }    // if

  /*********************************************************************
   * If program flow ends up here, then this instance of VB_Vector is   *
   * already of the required size. Therefore, we simply overwrite       *
   * this->theVector->data with zeroes.                                 *
   *********************************************************************/
  else if (this->theVector->size == newLength) {
    memset(this->theVector->data, 0, sizeof(double) * newLength);
  }  // else if

}  // void VB_Vector::resize(size_t newLength)

/*********************************************************************
 * This instance method computes and returns the vector sum.          *
 *********************************************************************/
double VB_Vector::getVectorSum() const {
  double sum = 0.0;

  for (unsigned long i = 0; i < this->getLength(); i++) {
    sum += this->theVector->data[i];
  }  // for i

  return sum;
}  // double VB_Vector::getVectorSum() const

double VB_Vector::getVectorMean() const {
  return getVectorSum() / (double)getLength();
}

double VB_Vector::getVectorMean(const gsl_vector *V) {
  return gsl_stats_mean(V->data, V->stride, V->size);
}

double VB_Vector::getVectorMean(const gsl_vector &V) {
  return VB_Vector::getVectorMean(&V);
}

double VB_Vector::getVectorMean(const VB_Vector &V) {
  return VB_Vector::getVectorMean(V.theVector);
}

double VB_Vector::getVectorMean(const VB_Vector *V) {
  return VB_Vector::getVectorMean(V->theVector);
}

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * VB_Vector, in place. This method implements the MATLAB style       *
 * convolution function.                                              *
 *********************************************************************/
void VB_Vector::convolve(const VB_Vector &v) {
  this->convolve(v.theVector);
}  // void VB_Vector::convolve(const VB_Vector& v)

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * VB_Vector, in place. This method implements the MATLAB style       *
 * convolution function.                                              *
 *********************************************************************/
void VB_Vector::convolve(const VB_Vector *v) {
  this->convolve(*v);
}  // void VB_Vector::convolve(const VB_Vector *v)

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * gsl_vector, in place.                                              *
 *                                                                    *
 * Here is the definition of the convolution used:                    *
 *   1. Let the resulting vector of the convolution of 2 vectors A    *
 *      and B be denoted by: (A, B).                                  *
 *   2. Let Az denote the vector A concatenated with a zero vector    *
 *      of the same length as A. Similarly for B.                     *
 *   3. Let the length of A be m and the length of B be n.            *
 *   4. Let [i] represent the i'th element of a vector.               *
 *   5. Then (A, B)[k] is given by the sum:                           *
 *                                                                    *
 *                                                                    *
 *            k                                                       *
 *          ----                                                      *
 *          \                                                         *
 *           \    Az[j] * Bz[k - j]                                   *
 *           /                                                        *
 *          /___                                                      *
 *          j = 0                                                     *
 *                                                                    *
 *      for k = 0, 1, ..., m + n - 2.                                 *
 *                                                                    *
 * NOTE: Some texts allow the vector resulting from the convolution   *
 * to have length (m + n) and not length (m + n - 1) as we do here.   *
 * However, the last element in this vector of length (m + n) is      *
 * always zero. This method simply omits this last element that is    *
 * always zero.  This method implements the MATLAB style convolution  *
 * function.                                                          *
 *********************************************************************/
void VB_Vector::convolve(const gsl_vector *kernel) {
  /*********************************************************************
   * Since this method overwrites this instance of VB_Vector with the   *
   * result of the convolution, we need to save this instance of        *
   * VB_Vector.                                                         *
   *********************************************************************/
  VB_Vector tempVec(this);

  /*********************************************************************
   * We now resize this instance of VB_Vector to have the required size *
   * of the result of the convolution. try/catch blocks are used in     *
   * case this->init() throws an exception.                             *
   *********************************************************************/
  try {
    this->init(this->getLength() + kernel->size - 1);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * The following for loop indexes the vector resulting from the       *
   * convolution.                                                       *
   *********************************************************************/
  for (size_t k = 0; k < this->getLength(); k++) {
    /*********************************************************************
     * The following for loop is used to compute the k'th element of the  *
     * convoluted vector.                                                 *
     *********************************************************************/
    for (size_t j = 0; j <= k; j++) {
      /*********************************************************************
       * Recall that the definition of vector convolution used in this      *
       * method involves concatenating each vector with a zero vector       *
       * whose length is equal to the length of the vector itself. In order *
       * to keep things as efficient as possible, the concatenation of the  *
       * zero vector was skipped, but we "assume" that each vector was      *
       * concatenated with a zero vector of appropriate length. However,    *
       * since neither of the 2 vectors was actually concatenated with a    *
       * zero vector, we need to check to see if we are actually accessing  *
       * a valid element in each vector or not. Specifically, when          *
       * j >= tempVec.theVector->size, tempVec[j] is not a valid in         *
       * tempVec. Had we actually appeneded a zero vector to tempVec,       *
       * tempVec[j] (for j >= tempVec.theVector->size and                   *
       * j < (2 * tempVec.theVector->size)) would be a valid element of     *
       * tempVec and it would equal zero. Similarly for the input vector    *
       * kernel when (k - j) >= kernel->size and                            *
       * (k - j) < (2 * kernel->size). Therefore, the following if block is *
       * is used to ensure that only valid elements of tempVec and kernel   *
       * are accessed. Doing things this way allows us to skip the step of  *
       * appending a zero vector to each of the 2 vectors whose convolution *
       * we calculate here.                                                 *
       *********************************************************************/
      if ((j < tempVec.getLength()) && ((k - j) < kernel->size)) {
        (*this)[k] += tempVec[j] * kernel->data[k - j];
      }  // if
    }    // for j
  }      // for k

}  // void VB_Vector::convolve(const gsl_vector *kernel)

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * gsl_vector, in place. This method implements the MATLAB style      *
 * convolution function.                                              *
 *********************************************************************/
void VB_Vector::convolve(const gsl_vector &v) {
  this->convolve(&v);
}  // void VB_Vector::convolve(const gsl_vector& v)

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * VB_Vector and returns the convolved vector. This method implements *
 * the MATLAB style convolution function.                             *
 *********************************************************************/
VB_Vector VB_Vector::convolve2(const VB_Vector &v) const {
  /*********************************************************************
   * A copy of this instance of VB_Vector is saved to vTemp. Then vTemp *
   * is convolved with the input VB_Vector. Finally, vTemp is returned. *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.
  vTemp.convolve(v.theVector);
  return vTemp;

}  // VB_Vector VB_Vector::convolve2(const VB_Vector& v) const

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * VB_Vector and returns the convolved vector. This method implements *
 * the MATLAB style convolution function.                             *
 *********************************************************************/
VB_Vector VB_Vector::convolve2(const VB_Vector *v) const {
  /*********************************************************************
   * A copy of this instance of VB_Vector is saved to vTemp. Then vTemp *
   * is convolved with the input VB_Vector. Finally, vTemp is returned. *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.
  vTemp.convolve(v->theVector);
  return vTemp;

}  // VB_Vector VB_Vector::convolve2(const VB_Vector *v) const

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * gsl_vector and returns the convolved vector. This method implements*
 * the MATLAB style convolution function.                             *
 *********************************************************************/
VB_Vector VB_Vector::convolve2(const gsl_vector &v) const {
  /*********************************************************************
   * A copy of this instance of VB_Vector is saved to vTemp. Then vTemp *
   * is convolved with the input gsl_vector. Finally, vTemp is returned.*
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.
  vTemp.convolve(&v);
  return vTemp;

}  // VB_Vector VB_Vector::convolve2(const gsl_vector& v) const

/*********************************************************************
 * This method convolves this instance of VB_Vector with the input    *
 * gsl_vector and returns the convolved vector. This method implements*
 * the MATLAB style convolution function.                             *
 *********************************************************************/
VB_Vector VB_Vector::convolve2(const gsl_vector *v) const {
  /*********************************************************************
   * A copy of this instance of VB_Vector is saved to vTemp. Then vTemp *
   * is convolved with the input gsl_vector. Finally, vTemp is returned.*
   *********************************************************************/
  VB_Vector v2(this);  // NOTE: vTemp.fileName is set to this->fileName.
  v2.convolve(v);
  return v2;

}  // VB_Vector VB_Vector::convolve2(const gsl_vector *v) const

/*********************************************************************
 * This is a static method that convolves the 2 input gsl_vector      *
 * variables and returns a VB_Vector. This method implements the      *
 * MATLAB style convolution function.                                 *
 *********************************************************************/
VB_Vector VB_Vector::convolve(const gsl_vector *v1, const gsl_vector *v2) {
  /*********************************************************************
   * vTemp is instantiated from v1 and then convolved with v2. Finally, *
   * vTemp is returned.                                                 *
   *********************************************************************/
  VB_Vector vTemp(v1);  // NOTE: vTemp.fileName is empty.
  vTemp.convolve(v2);
  return vTemp;

}  // VB_Vector VB_Vector::convolve(const gsl_vector *v1, const gsl_vector *v2)

/*********************************************************************
 * This is a static method that convolves the 2 input gsl_vector      *
 * variables and returns a VB_Vector. This method implements the      *
 * MATLAB style convolution function.                                 *
 *********************************************************************/
VB_Vector VB_Vector::convolve(const gsl_vector &v1, const gsl_vector &v2) {
  /*********************************************************************
   * Returning the convolution.                                         *
   *********************************************************************/
  return VB_Vector::convolve(&v1, &v2);

}  // VB_Vector VB_Vector::convolve(const gsl_vector& v1, const gsl_vector& v2)

/*********************************************************************
 * This is a static method that convolves the 2 input VB_Vector       *
 * variables and returns a VB_Vector. This method implements the      *
 * MATLAB style convolution function.                                 *
 *********************************************************************/
VB_Vector VB_Vector::convolve(const VB_Vector &v1, const VB_Vector &v2) {
  /*********************************************************************
   * Returning the convolution.                                         *
   *********************************************************************/
  return VB_Vector::convolve(v1.theVector, v2.theVector);

}  // VB_Vector VB_Vector::convolve(const VB_Vector& v1, const VB_Vector& v2)

/*********************************************************************
 * This is a static method that convolves the 2 input VB_Vector       *
 * variables and returns a VB_Vector. This method implements the      *
 * MATLAB style convolution function.                                 *
 *********************************************************************/
VB_Vector VB_Vector::convolve(const VB_Vector *v1, const VB_Vector *v2) {
  /*********************************************************************
   * Returning the convolution.                                         *
   *********************************************************************/
  return VB_Vector::convolve(v1->theVector, v2->theVector);

}  // VB_Vector VB_Vector::convolve(const VB_Vector *v1, const VB_Vector *v2)

/*********************************************************************
 * This instance method computes the variance of the vector.          *
 *********************************************************************/
double VB_Vector::getVariance() const {
  /*********************************************************************
   * variance will hold the variance of the vector. It's initialized to *
   * zero.                                                              *
   *********************************************************************/
  double variance = 0.0;

  /*********************************************************************
   * mean is assigned the vector mean.                                  *
   *********************************************************************/
  double mean = this->getVectorMean();

  /*********************************************************************
   * This for loop is used to compute the sum of the square of the      *
   * difference between each vector element value and the mean value.   *
   *********************************************************************/
  for (size_t i = 0; i < this->theVector->size; i++) {
    variance += ((*this)[i] - mean) * ((*this)[i] - mean);
  }  // for

  /*********************************************************************
   * Now returning the variance.                                        *
   *********************************************************************/
  return variance / (double)(this->theVector->size - 1);

}  // double VB_Vector::getVariance() const

/*********************************************************************
 * This instance method makes this instance of VB_Vector have unit    *
 * variance.                                                          *
 *********************************************************************/
void VB_Vector::unitVariance() {
  /*********************************************************************
   * stdDev is assigned the standard deviation of the vector.           *
   *********************************************************************/
  double stdDev = sqrt(this->getVariance());

  /*********************************************************************
   * Variance is computed by using the following formula:               *
   *                                                                    *
   *          n - 1                                                     *
   *          ----                                                      *
   *          \            _                                            *
   * sigma^2 = \    x(i) - x                                            *
   *           /   ----------------                                     *
   *          /___    (n - 1)                                           *
   *          i = 0                                                     *
   *                                                                    *
   * where n = the vector size.                                         *
   *                                                                    *
   * If the standard deviation is not 0, then the following for loop    *
   * will normalize this->theVector to have unit variance.              *
   *********************************************************************/
  if (stdDev != 0.0) {
    (*this) /= stdDev;
  }  // if

}  // void VB_Vector::unitVariance()

/*********************************************************************
 * This instance method returns a VB_Vector with unit variance derived*
 * from this instance of VB_Vector.                                   *
 *********************************************************************/
VB_Vector VB_Vector::unitVarianceVector() const {
  /*********************************************************************
   * This instance of VB_Vector is assigned to vTemp. Then vTemp is     *
   * made to have unit variance, after which it is returned.            *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.
  vTemp.unitVariance();
  return vTemp;

}  // VB_Vector VB_Vector::unitVarianceVector() const

/*********************************************************************
 * This instance method appends the input VB_Vector to this instance  *
 * of VB_Vector, in place.                                            *
 *********************************************************************/
void VB_Vector::concatenate(const VB_Vector &V) {
  /*********************************************************************
   * Calling this->concatenate(const gsl_vector *) to carry out the     *
   * concatenation operation.                                           *
   *********************************************************************/
  this->concatenate(V.theVector);

}  // void VB_Vector::concatenate(const VB_Vector& V)

/*********************************************************************
 * This instance method appends the input VB_Vector to this instance  *
 * of VB_Vector, in place.                                            *
 *********************************************************************/
void VB_Vector::concatenate(const VB_Vector *V) {
  /*********************************************************************
   * Calling this->concatenate(const gsl_vector *) to carry out the     *
   * concatenation operation.                                           *
   *********************************************************************/
  this->concatenate(V->theVector);

}  // void VB_Vector::concatenate(const VB_Vector *V)

/*********************************************************************
 * This instance method appends the input gsl_vector to this instance *
 * of VB_Vector, in place.                                            *
 *********************************************************************/
void VB_Vector::concatenate(const gsl_vector &V) {
  /*********************************************************************
   * Calling this->concatenate(const gsl_vector *) to carry out the     *
   * concatenation operation.                                           *
   *********************************************************************/
  this->concatenate(&V);

}  // void VB_Vector::concatenate(const gsl_vector& V)

/*********************************************************************
 * This instance method appends the input gsl_vector to this instance *
 * of VB_Vector, in place.                                            *
 *********************************************************************/
void VB_Vector::concatenate(const gsl_vector *V) {
  /*********************************************************************
   * If both the vector to be concatenated have positive length:        *
   *********************************************************************/
  if ((this->theVector != NULL) && (V != NULL)) {
    /*********************************************************************
     * By default, if the memory allocation fails,                        *
     * then gsl_vector_calloc() will invoke the GSL error handler         *
     * which calls abort(), creating a core dump. To forgo this behavior, *
     * the GSL error handler is turned off and the current error handler  *
     * is stored in VB_Vector::currentGSLErrorHandler. This is            *
     * accomplished by the call to this->turnOffGSLErrorHandler().        *
     *********************************************************************/
    this->turnOffGSLErrorHandler();

    /*********************************************************************
     * It's possible that the gsl_vector data member of this instance of  *
     * VB_Vector has the same address as V, i.e., this instance of        *
     * VB_Vector is being concatenated with itself, which we want to      *
     * allow. Therefore, two temprary gsl_vector structs are allocated.   *
     * this->theVector will be copied to tempVec and V will be copied to  *
     * tempVec2.                                                          *
     *********************************************************************/
    gsl_vector *tempVec = gsl_vector_calloc(this->getLength());
    gsl_vector *tempVec2 = gsl_vector_calloc(V->size);

    /*********************************************************************
     * We now restore the standard GSL error handler by calling           *
     * this->restoreGSLErrorHandler().                                    *
     *********************************************************************/
    this->restoreGSLErrorHandler();

    /*********************************************************************
     * Ensuring that tempVec is non-null. If tempVec is null, then the    *
     * try/catch will handle the exception thrown by                      *
     * VB_Vector::vectorNull(), which will have an appropriate error      *
     * message, as given by gsl_strerror().                               *
     *********************************************************************/
    try {
      VB_Vector::vectorNull(tempVec);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * Ensuring that tempVec2 is non-null. If tempVec2 is null, then the  *
     * try/catch will handle the exception thrown by                      *
     * VB_Vector::vectorNull(), which will have an appropriate error      *
     * message, as given by gsl_strerror().                               *
     *********************************************************************/
    try {
      VB_Vector::vectorNull(tempVec2);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * Copying this->theVector to tempVec. If VB_Vector::GSLVectorMemcpy()*
     * throws an exception (which will have an appropriate error message, *
     * as determined by gsl_strerror()) then it is be caught here.        *
     *********************************************************************/
    try {
      VB_Vector::GSLVectorMemcpy(tempVec, this->theVector);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * Copying V to tempVec. If VB_Vector::GSLVectorMemcpy() throws an    *
     * exception (which will have an appropriate error message, as        *
     * determined by gsl_strerror()) then it is be caught here.           *
     *********************************************************************/
    try {
      VB_Vector::GSLVectorMemcpy(tempVec2, V);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * Resizing this->theVector to (this->theVector->size + V->size). If  *
     * an exception occurrs, which will have an appropriate error message *
     * as determined by gsl_strerror(), then it is caught here.           *
     *********************************************************************/
    try {
      this->init(this->getLength() + V->size);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * Now assigning the values from tempVec to this instance of          *
     * VB_Vector.                                                         *
     *********************************************************************/
    memcpy(this->theVector->data, tempVec->data,
           sizeof(double) * tempVec->size);

    /*********************************************************************
     * Now assigning the values from tempVec2 to this instance of         *
     * VB_Vector.                                                         *
     *********************************************************************/
    memcpy(this->theVector->data + tempVec->size, tempVec2->data,
           sizeof(double) * tempVec2->size);

    /*********************************************************************
     * Freeing the memory allocated to tempVec and tempVec2.              *
     *********************************************************************/
    gsl_vector_free(tempVec);
    gsl_vector_free(tempVec2);

  }  // if

  /*********************************************************************
   * If V is NULL, then we do nothing since there's no point in         *
   * concatenating an empty vector to this instance of VB_Vector.       *
   *********************************************************************/
  else if (V == NULL) {
  }

  /*********************************************************************
   * If this instance of VB_Vector is empty, i.e., has zero length, or  *
   * V is empty.                                                        *
   *********************************************************************/
  else if (this->theVector == NULL)  // qq Could this->theVector ever be NULL ?
  {
    /*********************************************************************
     * By default, if the memory allocation fails,                        *
     * then gsl_vector_calloc() will invoke the GSL error handler         *
     * which calls abort(), creating a core dump. To forgo this behavior, *
     * the GSL error handler is turned off and the current error handler  *
     * is stored in VB_Vector::currentGSLErrorHandler. This is            *
     * accomplished by the call to this->turnOffGSLErrorHandler().        *
     *********************************************************************/
    this->turnOffGSLErrorHandler();

    /*********************************************************************
     * Allocating sufficient space for this->theVector.                   *
     *********************************************************************/
    this->theVector = gsl_vector_calloc(V->size);

    /*********************************************************************
     * Ensuring that this->theVector is non-null. If this->theVector is   *
     * NULL, then the try/catch will handle the exception thrown by       *
     * VB_Vector::vectorNull(), which will have an appropriate error      *
     * message, as given by gsl_strerror().                               *
     *********************************************************************/
    try {
      VB_Vector::vectorNull(this->theVector);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * We now restore the standard GSL error handler by calling           *
     * this->restoreGSLErrorHandler().                                    *
     *********************************************************************/
    this->restoreGSLErrorHandler();

    /*********************************************************************
     * Copying V to tempVec. If VB_Vector::GSLVectorMemcpy() throws an    *
     * exception (which will have an appropriate error message, as        *
     * determined by gsl_strerror()) then it is be caught here.           *
     *********************************************************************/
    try {
      VB_Vector::GSLVectorMemcpy(this->theVector, V);
    }  // try
    catch (GenericExcep &e) {
      e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
    }  // catch

    /*********************************************************************
     * At this point, we have successfully copied V to this->theVector.   *
     * Therefore, this->valid is set to true.                             *
     *********************************************************************/
    this->valid = true;

  }  // else if

}  // void VB_Vector::concatenate(const gsl_vector *V)

/*********************************************************************
 * This instance method appends the input VB_Vector to this instance  *
 * of VB_Vector, not in place, and returns the concatenated VB_Vector.*
 *********************************************************************/
VB_Vector VB_Vector::concatenate2(const VB_Vector &V) const {
  /*********************************************************************
   * This instance of VB_Vector is saved to vTemp.                      *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate2(const VB_Vector& V) const

/*********************************************************************
 * This instance method appends the input VB_Vector to this instance  *
 * of VB_Vector, not in place, and returns the concatenated VB_Vector.*
 *********************************************************************/
VB_Vector VB_Vector::concatenate2(const VB_Vector *V) const {
  /*********************************************************************
   * This instance of VB_Vector is saved to vTemp.                      *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate2(const VB_Vector *V) const

/*********************************************************************
 * This instance method appends the input gsl_vector to this instance *
 * of VB_Vector, not in place, and returns the concatenated VB_Vector.*
 *********************************************************************/
VB_Vector VB_Vector::concatenate2(const gsl_vector *V) const {
  /*********************************************************************
   * This instance of VB_Vector is saved to vTemp.                      *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate2(const gsl_vector *V) const

/*********************************************************************
 * This instance method appends the input gsl_vector to this instance *
 * of VB_Vector, not in place, and returns the concatenated VB_Vector.*
 *********************************************************************/
VB_Vector VB_Vector::concatenate2(const gsl_vector &V) const {
  /*********************************************************************
   * This instance of VB_Vector is saved to vTemp.                      *
   *********************************************************************/
  VB_Vector vTemp(this);  // NOTE: vTemp.fileName is set to this->fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate2(const gsl_vector& V) const

/*********************************************************************
 * This is a static method that concatenates the 2 input vectors and  *
 * returns the concatenated vector.                                   *
 *********************************************************************/
VB_Vector VB_Vector::concatenate(const VB_Vector &V1, const VB_Vector &V2) {
  /*********************************************************************
   * V1 is saved to vTemp.                                              *
   *********************************************************************/
  VB_Vector vTemp(V1);  // NOTE: vTemp.fileName is set to V1.fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V2);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate(const VB_Vector& V1, const VB_Vector& V2)

/*********************************************************************
 * This is a static method that concatenates the 2 input vectors and  *
 * returns the concatenated vector.                                   *
 *********************************************************************/
VB_Vector VB_Vector::concatenate(const VB_Vector *V1, const VB_Vector *V2) {
  /*********************************************************************
   * V1 is saved to vTemp.                                              *
   *********************************************************************/
  VB_Vector vTemp(V1);  // NOTE: vTemp.fileName is set to V1->fileName.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V2);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate(const VB_Vector *V1, const VB_Vector *V2)

/*********************************************************************
 * This is a static method that concatenates the 2 input vectors and  *
 * returns the concatenated vector.                                   *
 *********************************************************************/
VB_Vector VB_Vector::concatenate(const gsl_vector &V1, const gsl_vector &V2) {
  /*********************************************************************
   * V1 is saved to vTemp.                                              *
   *********************************************************************/
  VB_Vector vTemp(V1);  // NOTE: vTemp.fileName is empty.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V2);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate(const gsl_vector& V1, const gsl_vector&
   // V2)

/*********************************************************************
 * This is a static method that concatenates the 2 input vectors and  *
 * returns the concatenated vector.                                   *
 *********************************************************************/
VB_Vector VB_Vector::concatenate(const gsl_vector *V1, const gsl_vector *V2) {
  /*********************************************************************
   * V1 is saved to vTemp. Then vTemp is concatenated with V1. Finally, *
   * vtemp is returned.                                                 *
   *********************************************************************/
  VB_Vector vTemp(V1);  // NOTE: vTemp.fileName is empty.

  /*********************************************************************
   * Setting vTemp.fileName to the empty string.                        *
   *********************************************************************/
  vTemp.fileName = string("");

  /*********************************************************************
   * Setting this->valid, this->dataType, and this->fileFormat. init()  *
   * will also assign a new file name to vTemp.fileName.                *
   *********************************************************************/
  vTemp.init(false, vb_double, "ref1");

  /*********************************************************************
   * The input vector is concatenated to vTemp and then vTemp is        *
   * returned.                                                          *
   *********************************************************************/
  vTemp.concatenate(V2);
  return vTemp;

}  // VB_Vector VB_Vector::concatenate(const gsl_vector *V1, const gsl_vector
   // *V2)

/*********************************************************************
 * This is the overloaded left shift operator. The last i elements    *
 * from the right are set to zero.                                    *
 *********************************************************************/
VB_Vector &VB_Vector::operator<<(size_t i) {
  /*********************************************************************
   * If i is zero, i.e., no shift, then (*this) is simply returned.     *
   *********************************************************************/
  if (i == 0) {
    return (*this);
  }  // if

  /*********************************************************************
   * If i equals or exceeds the length of this vector, then the vector  *
   * is simply set to all zeros.                                        *
   *********************************************************************/
  if (i >= this->getLength()) {
    this->init(this->getLength());
    return (*this);
  }  // if

  /*********************************************************************
   * This for loop translates the elements of this instance of VB_Vector*
   * i spots to the left.                                               *
   *********************************************************************/
  for (size_t j = 0; j < this->getLength() - i; j++) {
    (*this)[j] = (*this)[j + i];
  }  // for j

  /*********************************************************************
   * This for loop sets the last i right hand side elements of this     *
   * instance of VB_Vector to zero.                                     *
   *********************************************************************/
  for (size_t j = this->getLength() - i; j < this->getLength(); j++) {
    (*this)[j] = 0.0;
  }  // for j

  /*********************************************************************
   * Now returning this instance of VB_Vector.                          *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator<<(size_t i)

/*********************************************************************
 * This is the overloaded right shift operator. The first i elements  *
 * from the left are set to zero.                                     *
 *********************************************************************/
VB_Vector &VB_Vector::operator>>(size_t i) {
  /*********************************************************************
   * If i is zero, i.e., no shift, then (*this) is simply returned.     *
   *********************************************************************/
  if (i == 0) {
    return (*this);
  }  // if

  /*********************************************************************
   * If i equals or exceeds the length of this vector, then the vector  *
   * is simply set to all zeros.                                        *
   *********************************************************************/
  if (i >= this->getLength()) {
    this->init(this->getLength());
    return (*this);
  }  // if

  /*********************************************************************
   * This for loop translates the elements of this instance of VB_Vector*
   * i spots to the right. First we need to make a copy of this         *
   * instance of VB_Vector since its elements get overwritten from the  *
   * left.                                                              *
   *********************************************************************/
  VB_Vector tempVec(this);
  for (size_t j = i; j < this->getLength(); j++) {
    (*this)[j] = tempVec[j - i];
  }  // for j

  /*********************************************************************
   * This for loop sets the first i left hand side elements of this     *
   * instance of VB_Vector to zero.                                     *
   *********************************************************************/
  for (size_t j = 0; j < i; j++) {
    (*this)[j] = 0.0;
  }  // for j

  /*********************************************************************
   * Now returning this instance of VB_Vector.                          *
   *********************************************************************/
  return (*this);

}  // VB_Vector& VB_Vector::operator>>(size_t i)

/*********************************************************************
 * This overloaed operator scales this instance of VB_Vector by the   *
 * input alpha and returns the scaled vector.                         *
 *********************************************************************/
VB_Vector VB_Vector::operator*(const double alpha) const {
  /*********************************************************************
   * A copy is made of this instance of VB_Vector.                      *
   *********************************************************************/
  VB_Vector tempVec(this);

  /*********************************************************************
   * gsl_vector_scale() is called to do the scaling and its             *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_scale(tempVec.theVector, alpha),
                            __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Now returning the scaled vector.                                   *
   *********************************************************************/
  return tempVec;

}  // VB_Vector VB_Vector::operator*(const double alpha) const

/*********************************************************************
 * This friend function scales the input VB_Vector by the input       *
 * double. This function is implemented to preserve commutativity.    *
 *********************************************************************/
VB_Vector operator*(const double alpha, const VB_Vector &V) {
  /*********************************************************************
   * The copy constructor is used to make a copy of V.                  *
   *********************************************************************/
  VB_Vector tempVec(V);

  /*********************************************************************
   * gsl_vector_scale() is called to do the scaling and its             *
   * return value is passed to VB_Vector::checkGSLStatus(). If the      *
   * return value is non-zero, then a error occurred with the GSL       *
   * function. VB_Vector::checkGSLStatus() will then process the error  *
   * by writing out an appropriate error message and then causing this  *
   * program to exit.                                                   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_vector_scale(tempVec.theVector, alpha),
                            __LINE__, __FILE__, __FUNCTION__);
  /*********************************************************************
   * tempVec is scaled by alpha and then returned.                      *
   *********************************************************************/
  /*  tempVec.scaleInPlace(alpha);*/
  return tempVec;

}  // VB_Vector operator*(const double alpha, const VB_Vector& V)

void VB_Vector::meanCenter() { (*this) -= this->getVectorMean(); }

/*********************************************************************
 * This method resets the elements of this instance of VB_Vector from *
 * the input array of doubles. The input variable length is assumed   *
 * to be the length of the array theData.                             *
 *********************************************************************/
void VB_Vector::setData(const double *theData, const size_t length) {
  /*********************************************************************
   * We call this->init() to resize this instance of VB_Vector and      *
   * catch its exception, if one is thrown.                             *
   *********************************************************************/
  try {
    this->init(length);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  memcpy(this->theVector->data, theData, sizeof(double) * length);

}  // void VB_Vector::setData(const double *theData, const size_t length)

/*********************************************************************
 * This method resets the data values of this instance of VB_Vector   *
 * from the data values of the input gsl_vector.                      *
 *********************************************************************/
void VB_Vector::setData(const gsl_vector *v) {
  /*********************************************************************
   * Calling this->setData(const double *, const size_t) to reset the   *
   * values.                                                            *
   *********************************************************************/
  this->setData(v->data, v->size);

}  // void VB_Vector::setData(const gsl_vector *v)

/*********************************************************************
 * This method reset the data values of this instance of VB_Vector    *
 * from the data values of the input gsl_vector.                      *
 *********************************************************************/
void VB_Vector::setData(const gsl_vector &v) {
  /*********************************************************************
   * Calling this->setData(const double *, const size_t) to reset the   *
   * values.                                                            *
   *********************************************************************/
  this->setData(v.data, v.size);

}  // void VB_Vector::setData(const gsl_vector& v)

/*********************************************************************
 * This method reset the data values of this instance of VB_Vector    *
 * from the data values of the input VB_Vector.                       *
 *********************************************************************/
void VB_Vector::setData(const VB_Vector *V) {
  /*********************************************************************
   * Calling this->setData(const double *, const size_t) to reset the   *
   * values.                                                            *
   *********************************************************************/
  this->setData(V->theVector->data, V->getLength());

}  // void VB_Vector::setData(const VB_Vector *V)

/*********************************************************************
 * This method reset the data values of this instance of VB_Vector    *
 * from the data values of the input VB_Vector.                       *
 *********************************************************************/
void VB_Vector::setData(const VB_Vector &V) {
  /*********************************************************************
   * Calling this->setData(const double *, const size_t) to reset the   *
   * values.                                                            *
   *********************************************************************/
  this->setData(V.theVector->data, V.getLength());

}  // void VB_Vector::setData(const VB_Vector& V)

/*********************************************************************
 * This method multiplies this instance of VB_Vector by the           *
 * orthogonal projector:                                              *
 *                                                                    *
 *                T  -1 T                                             *
 *       (I - A (A A)  A )                                            *
 *                                                                    *
 *  where:                                                            *
 *                                                                    *
 *  A is the matrix created from the VB_Vector's found in the         *
 *  argument reference.                                               *
 *  I is an appropriately dimensioned identity matrix.                *
 *                                                                    *
 *  Basically, if this instance of VB_Vector is denoted by x, then    *
 *  the following computation is carried out:                         *
 *                                                                    *
 *                T  -1 T                                             *
 *  x' = (I - A (A A)  A )x                                           *
 *                                                                    *
 *  where x' is this instance of VB_Vector after all the computations *
 *  have been carried out.                                            *
 *                                                                    *
 * To implement the computations in an efficient manner as possible,  *
 * the following steps are carried out (assume A is (m x n) and that  *
 * x is (m x 1)):                                                     *
 *                                                                    *
 *             T                                                      *
 * 1. Let v = A x  (v is then (n x 1))                                *
 *                     T  -1                                          *
 *    ==> x' = x - A (A A)  v                                         *
 *                                                                    *
 *              T  -1                                                 *
 * 2. Let y = (A A)  v  (y is (n x 1))                                *
 *    This linear system will be solved for y (this saves us the      *
 *    computationally expensive step of computing a matrix inverse).  *
 *                                                                    *
 *    ==> x' = x - Ay                                                 *
 *                                                                    *
 * NOTE: It is expected that A has linearly independent columns.      *
 *********************************************************************/
void VB_Vector::orthogonalize(const vector<VB_Vector> reference) {
  /*********************************************************************
   * We now see if the input variable reference represents an           *
   * overdetermined system or not. If it does, then an exception is     *
   * thrown (by calling VB_Vector::createException()) and then caught.  *
   * An error message is assembled first.                               *
   *********************************************************************/
  if (this->getLength() < reference.size()) {
    /*********************************************************************
     * errorMsg[] will hold the error message that the index is out of    *
     * range.                                                             *
     *********************************************************************/
    char errorMsg[OPT_STRING_LENGTH];
    memset(errorMsg, 0, OPT_STRING_LENGTH);

    /*********************************************************************
     * Populating errorMsg[] with the appropriate error message.          *
     *********************************************************************/
    sprintf(errorMsg,
            "The vector length [%d] is < the number of column vectors [%d] "
            "(overdetermined system).",
            (int)(this->getLength()), (int)(reference.size()));

    /*********************************************************************
     * Now calling VB_Vector::createException() to throw and catch the    *
     * exception.                                                         *
     *********************************************************************/
    VB_Vector::createException(errorMsg, __LINE__, __FILE__, __FUNCTION__);

  }  // if

  /*********************************************************************
   * By default, if the memory allocation fails,                        *
   * then gsl_matrix_calloc() (called by this->initMatrix()) will invoke*
   * the GSL error handler which calls abort(), creating a core dump.   *
   * To forgo this behavior, the GSL error handler is turned off and    *
   * the current error handler is stored in                             *
   * VB_Vector::currentGSLErrorHandler. This is accomplished by the     *
   * call to this->turnOffGSLErrorHandler().                            *
   *********************************************************************/
  this->turnOffGSLErrorHandler();

  /*********************************************************************
   * Allocating a gsl_matrix with rows reference[0].getLength() and     *
   * columns reference.size(). All the elements in the gsl_matrix will  *
   * be initialized to 0. try/catch blocks are used to process the      *
   * exception that may be thrown by this->initMatrix().                *
   *********************************************************************/
  gsl_matrix *A = NULL;
  try {
    A = this->initMatrix(reference[0].getLength(), reference.size());
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * Now allocating a gsl_matrix struct correctly dimensioned to hold   *
   *                 T                                                  *
   * the product of A  and A. All its elements will be initialized to 0.*
   * try/catch blocks are used to process the exception that may be     *
   * thrown by this->initMatrix().                                      *
   *********************************************************************/
  gsl_matrix *aTa = NULL;
  try {
    aTa = this->initMatrix(reference.size(), reference.size());
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * We now populate A with the VB_Vectors from reference. Each         *
   * VB_Vector from reference is seen as a column vector of A. If       *
   * gsl_matrix_set_col() returns a non-zero return value, then an      *
   * error occurred, which is handled by checkGSLStatus().              *
   *********************************************************************/
  for (size_t i = 0; i < reference.size(); i++) {
    VB_Vector::checkGSLStatus(
        gsl_matrix_set_col(A, i, reference[i].getTheVector()), __LINE__,
        __FILE__, __FUNCTION__);
  }  // for i

  /*********************************************************************
   *                                                   T                *
   * v will be used to hold the matrix-vector product A x. Since A is   *
   * (m x n) and x is (m x 1), v needs to have length n, which is the   *
   * number of columns in A, found in the field A->size2.               *
   *********************************************************************/
  VB_Vector v(A->size2);

  /*********************************************************************
   * Now carrying out the matrix-vector multiplication (Ax). The product*
   * will be stored in v. The prototype for the function                *
   * gsl_blas_dgemv() is:                                               *
   *                                                                    *
   *   int  gsl_blas_dgemv (CBLAS_TRANSPOSE_t TransA,                   *
   *                        double alpha,                               *
   *                        const gsl_matrix * A,                       *
   *                        const gsl_vector * x,                       *
   *                        double beta,                                *
   *                        gsl_vector * y);                            *
   *                                                                    *
   * and the actual operations carried out by gsl_blas_dgemv() are:     *
   *                                                                    *
   *  y := alpha (op(A)) x + beta (y)                                   *
   *                                                                    *
   *  where op(A) =                                                     *
   *   T                                                                *
   *  A  if TransA is CblasTrans                                        *
   *  A  if TransA is CblasNoTrans                                      *
   *                                                      T             *
   * In this specific case, we are simply computing v = (A x). If       *
   * gsl_blas_dgemv() returns a non-zero value, then an error occurred, *
   * which gets handled by checkGSLStatus().                            *
   *********************************************************************/
  VB_Vector::checkGSLStatus(
      gsl_blas_dgemv(CblasTrans, 1.0, A, this->theVector, 0.0, v.theVector),
      __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   *                                              T                     *
   * Now carrying out the matrix multiplication (A  * A). The product   *
   * will be stored in aTa. The prototype for the function              *
   * gsl_blas_dgemm() is:                                               *
   *                                                                    *
   *   int  gsl_blas_dgemm (CBLAS_TRANSPOSE_t TransA,                   *
   *                        CBLAS_TRANSPOSE_t TransB,                   *
   *                        double alpha,                               *
   *                        const gsl_matrix * A,                       *
   *                        const gsl_matrix * B,                       *
   *                        double beta,                                *
   *                        gsl_matrix * C);                            *
   *                                                                    *
   * and the actual operations carried out by gsl_blas_dgemm() are:     *
   *                                                                    *
   *  C := alpha * op(A) * op(B) + beta * C                             *
   *                                                                    *
   *  where op(A) =                                                     *
   *   T                                                                *
   *  A  if TransA is CblasTrans                                        *
   *  A  if TransA is CblasNoTrans                                      *
   *  Similarly for op(B).                                              *
   *                                                       T            *
   * In this specific case, we are simply computing aTa = A  A.         *
   *********************************************************************/
  VB_Vector::checkGSLStatus(
      gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, A, A, 0.0, aTa), __LINE__,
      __FILE__, __FUNCTION__);

  /*********************************************************************
   * As the second step, we use gsl_linalg_cholesky_decomp() to compute *
   * Cholesky factorization of aTa (since is is expected that the       *
   * columns of A are linearly independent, aTa will be positive        *
   * definite). After gsl_linalg_cholesky_decomp(), aTa will hold the   *
   * Cholesky factors. checkGSLStatus() is used to process any non-zero *
   * value, i.e., an error, returned by gsl_linalg_cholesky_decomp().   *
   *********************************************************************/
  VB_Vector::checkGSLStatus(gsl_linalg_cholesky_decomp(aTa), __LINE__, __FILE__,
                            __FUNCTION__);

  /*********************************************************************
   *                                              T  -1                 *
   * We need to find the vector y such that y = (A A)  v. We will do    *
   * this by solving the linear system:                                 *
   *     T                                                              *
   *   (A A)y = v                                                       *
   * This is why we previously computed the Cholesky factorization of   *
   * aTa.                                                               *
   *                                                                    *
   * y is initialized to be a vector of length aTa->size1 and all of    *
   * its elements will be 0.                                            *
   *********************************************************************/
  VB_Vector y(aTa->size1);

  /*********************************************************************
   * Now gsl_linalg_cholesky_solve() is used to solve for y. If         *
   * gsl_linalg_cholesky_solve() returns a non-zero value, then an error*
   * occurred, which gets handled by checkGSLStatus().                  *
   *********************************************************************/
  VB_Vector::checkGSLStatus(
      gsl_linalg_cholesky_solve(aTa, v.theVector, y.theVector), __LINE__,
      __FILE__, __FUNCTION__);

  /*********************************************************************
   * We now call gsl_blas_dgemv() to do the matrix-vector multiplication*
   * (Ay). This product will be contained in v.theVector (since we no   *
   * longer need v, we can safely overwrite it). However, v needs to be *
   * resized to have length A->size1 (the number of rows in A). If      *
   * gsl_blas_dgemv() returns a non-zero value, then an error occurred, *
   * which gets handled by checkGSLStatus().                            *
   *********************************************************************/
  v.init(A->size1);
  VB_Vector::checkGSLStatus(
      gsl_blas_dgemv(CblasNoTrans, 1.0, A, y.theVector, 0.0, v.theVector),
      __LINE__, __FILE__, __FUNCTION__);

  /*********************************************************************
   * Finally, we now subtract v from this instance of VB_Vector and     *
   * store the difference in this instance of VB_Vector.                *
   *********************************************************************/
  (*this) -= v;

  /*********************************************************************
   * We now restore the standard GSL error handler by calling           *
   * this->restoreGSLErrorHandler().                                    *
   *********************************************************************/
  this->restoreGSLErrorHandler();

  /*********************************************************************
   * Freeing the previously allocated memory for A and aTa.             *
   *********************************************************************/
  gsl_matrix_free(A);
  gsl_matrix_free(aTa);

}  // void VB_Vector::orthogonalize(const vector<VB_Vector> reference)

/*********************************************************************
 * This method multiplies the input VB_Vector myVec by the orthogonal *
 * projector:                                                         *
 *                                                                    *
 *                T  -1 T                                             *
 *       (I - A (A A)  A )                                            *
 *                                                                    *
 * wehre A is a matrix derived from the inout reference and returns   *
 * the resulting vector.                                              *
 *********************************************************************/
VB_Vector VB_Vector::orthogonalize(const VB_Vector &myVec,
                                   const vector<VB_Vector> reference) const {
  /*********************************************************************
   * Instantiating tempVec from myVec.                                  *
   *********************************************************************/
  VB_Vector tempVec(myVec);

  /*********************************************************************
   * Multiplying tempVec by the projector and then returning tempVec.   *
   *********************************************************************/
  tempVec.orthogonalize(reference);
  return tempVec;

}  // VB_Vector VB_Vector::orthogonalize(const VB_Vector &myVec, const
   // vector<VB_Vector> reference)

/*********************************************************************
 * This static method writes out the input gsl_matrix struct to       *
 * stdout.                                                            *
 *********************************************************************/
void VB_Vector::printMatrix(const gsl_matrix *M) {
  /*  gsl_matrix_fprintf(stdout, M, "%g");*/

  for (size_t i = 0; i < M->size1; i++) {
    for (size_t j = 0; j < M->size2; j++) {
      if (j == 0) {
        cout << "[ ";
      }  // if
      cout << gsl_matrix_get(M, i, j);
      if (j != (M->size2 - 1)) {
        cout << ", ";
      }  // if
      else {
        cout << " ]" << endl;
      }  // else
    }    // for j
  }      // for i
  cout << endl;

}  // void VB_Vector::printMatrix(const gsl_matrix *M)

/*********************************************************************
 * This static method writes out the input gsl_matrix struct to       *
 * stdout.                                                            *
 *********************************************************************/
void VB_Vector::printMatrix(const gsl_matrix &M) {
  VB_Vector::printMatrix(&M);
}  // void VB_Vector::printMatrix(const gsl_matrix& M)

/*********************************************************************
 * This method computes the FFT of this instance of VB_Vector and     *
 * stores the real and imaginary parts into the 2 input VB_Vectors.   *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * realPart           VB_Vector *     Will hold the real part of the  *
 *                                    computed FFT.                   *
 * imagPart           VB_Vector *     Will hold the imaginary part of *
 *                                    the computed FFT.               *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::fft(VB_Vector *realPart, VB_Vector *imagPart) const {
  /*********************************************************************
   * Ensuring that realPart and imagPart are properly sized.            *
   *********************************************************************/
  if (this->getLength() != realPart->getLength()) {
    realPart->resize(this->theVector->size);
  }  // if
  if (this->getLength() != imagPart->getLength()) {
    imagPart->resize(this->theVector->size);
  }  // if

  /*********************************************************************
   * halfSize will be (this->theVector->size / 2) if the length of this *
   * instance of VB_Vector is even. Otherwise halfSize will be          *
   * ((this->theVector->size / 2) - 1).                                 *
   *********************************************************************/
  const unsigned int halfSize = this->theVector->size / 2;

  /*********************************************************************
   * even will be true if the length of this instance of VB_Vector is   *
   * even, false otherwise.                                             *
   *********************************************************************/
  const bool even = ((halfSize * 2) == this->theVector->size);

  /*********************************************************************
   * The data from this instance of VB_Vector is copied to FFT[].       *
   *********************************************************************/
  double FFT[this->theVector->size];
  memcpy(FFT, this->theVector->data, this->theVector->size * sizeof(double));

  /*********************************************************************
   * The following 2 variables are needed by GSL's                      *
   * gsl_fft_real_transform() function. waveTable is a lookup table for *
   * sines and cosines while workSpace is a required work space.        *
   *********************************************************************/
  gsl_fft_real_wavetable *waveTable =
      gsl_fft_real_wavetable_alloc(this->theVector->size);
  gsl_fft_real_workspace *workSpace =
      gsl_fft_real_workspace_alloc(this->theVector->size);

  /*********************************************************************
   * If waveTable is NULL, then an exception is thrown and then caught  *
   * by VB_Vector::createException() to inform the user of this error.  *
   *********************************************************************/
  if (!waveTable) {
    VB_Vector::createException("Unable to allocate gsl_fft_real_wavetable.",
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * If workSpace is NULL, then an exception is thrown and then caught  *
   * by VB_Vector::createException() to inform the user of this error.  *
   *********************************************************************/
  if (!workSpace) {
    VB_Vector::createException("Unable to allocate gsl_fft_real_workspace.",
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * Now computing the FFT. If the call to gsl_fft_real_transform() is  *
   * successful, then status will be 0.                                 *
   *********************************************************************/
  int status = gsl_fft_real_transform(FFT, 1, this->theVector->size, waveTable,
                                      workSpace);

  /*********************************************************************
   * If status is non-zero, then VB_Vector::createException() is called *
   * to throw and catch an exception to inform the user of the error.   *
   * The error message will contain the reason for the error, as        *
   * returned by gsl_strerror(status).                                  *
   *********************************************************************/
  if (status) {
    VB_Vector::createException(string(gsl_strerror(status) + string(".")),
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * We will need to scale FFT[] by the reciprocal of the length of     *
   * this instance of VB_Vector to make this implementation of the FFT  *
   * identical to IDL's. To that end, oneOverSize is used to hold the   *
   * reciprocal of the length of this instance of VB_Vector.            *
   *********************************************************************/
  const double oneOverSize = 1.0 / this->theVector->size;

  /*********************************************************************
   * FFT[] now holds the discrete Fourier transform of this instance of *
   * VB_Vector. However, the storage scheme is not straight forward and *
   * is called "half complex." (It's advantage is that it takes the     *
   * exact same mount of space as the origianl vector.) Here is a       *
   * descpription of the the "half complex" storage scheme from p. 200  *
   * of the GSL Reference Manual:                                       *
   *                                                                    *
   * ... the half-complex transform of a real sequence is stored with   *
   * frequencies in increasing order, starting at zero, with the real   *
   * and imaginary parts of each frequency in neighboring locations.    *
   * When a value is know to be real, its imaginary part is not stored. *
   *                                                                    *
   * Here are 2 examples, the first is for n = 5 (odd case), the second *
   * for n = 6 (even case):                                             *
   *                                                                    *
   *         complex[0].real = halfcomplex[0]                           *
   *         complex[0].imag = 0 (always 0)                             *
   *         complex[1].real = halfcomplex[1]                           *
   *         complex[1].imag = halfcomplex[2]                           *
   *         complex[2].real = halfcomplex[3]                           *
   *         complex[2].imag = halfcomplex[4]                           *
   *         complex[3].real = halfcomplex[3]                           *
   *         complex[3].imag = -halfcomplex[4]                          *
   *         complex[4].real = halfcomplex[1]                           *
   *         complex[4].imag = -halfcomplex[2]                          *
   *                                                                    *
   *         complex[0].real = halfcomplex[0]                           *
   *         complex[0].imag = 0 (always 0)                             *
   *         complex[1].real = halfcomplex[1]                           *
   *         complex[1].imag = halfcomplex[2]                           *
   *         complex[2].real = halfcomplex[3]                           *
   *         complex[2].imag = halfcomplex[4]                           *
   *         complex[3].real = halfcomplex[5]                           *
   *         complex[3].imag = 0 (always 0 in even case)                *
   *         complex[4].real = halfcomplex[3]                           *
   *         complex[4].imag = -halfcomplex[4]                          *
   *         complex[5].real = halfcomplex[1]                           *
   *         complex[5].imag = -halfcomplex[2]                          *
   *                                                                    *
   * We will have to "unpack" this half-complex storage to populate     *
   * realPart[] and imagPart[] with the real and imaginary parts of     *
   * FFT[]. We now set realPart[0] and imagPart[0].                     *
   *********************************************************************/
  (*realPart)[0] = FFT[0] * oneOverSize;
  (*imagPart)[0] = 0.0;

  /*********************************************************************
   * The following for loop is used to populate the the reaminder of    *
   * realPart[] and imagPart[] from FFT[]. This for loop also scales    *
   * the entries of FFT[] by oneOverSize.                               *
   *********************************************************************/
  for (unsigned int i = 1; i < this->theVector->size; i++) {
    if (i < halfSize) {
      (*realPart)[i] = FFT[(2 * i) - 1] * oneOverSize;
      (*imagPart)[i] = FFT[(2 * i)] * oneOverSize;
    }  // if
    else if (i == halfSize) {
      if (even) {
        (*realPart)[i] = FFT[this->theVector->size - 1] * oneOverSize;
        (*imagPart)[i] = 0.0;
      }  // if
      else {
        (*realPart)[i] = FFT[this->theVector->size - 2] * oneOverSize;
        (*imagPart)[i] = FFT[this->theVector->size - 1] * oneOverSize;
      }  // if
    }    // else if
    else {
      /*********************************************************************
       * NOTE: Since the index (this->theVector->size - i) is < i,          *
       * (*realPart)[this->theVector->size - i] has already been scaled by  *
       * oneOverSize. Similarly for (*imagPart)[this->theVector->size - i]. *
       *********************************************************************/
      (*realPart)[i] = (*realPart)[this->theVector->size - i];
      (*imagPart)[i] = -1.0 * (*imagPart)[this->theVector->size - i];

    }  // else

  }  // for i

  /*********************************************************************
   * Now freeing waveTable and workSpace.                               *
   *********************************************************************/
  gsl_fft_real_wavetable_free(waveTable);
  gsl_fft_real_workspace_free(workSpace);

}  // void VB_Vector::fft(VB_Vector *realPart, VB_Vector *imagPart) const

/*********************************************************************
 * This method computes the FFT of this instance of VB_Vector and     *
 * stores the real and imaginary parts into the 2 input VB_Vectors.   *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * realPart           VB_Vector&      Will hold the real part of the  *
 *                                    computed FFT.                   *
 * imagPart           VB_Vector&      Will hold the imaginary part of *
 *                                    the computed FFT.               *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::fft(VB_Vector &realPart, VB_Vector &imagPart) const {
  /*********************************************************************
   * We now call this->fft(VB_Vector *realPart, VB_Vector *imagPart).   *
   *********************************************************************/
  this->fft(&realPart, &imagPart);

}  // void VB_Vector::fft(VB_Vector& realPart, VB_Vector& imagPart) const

/*********************************************************************
 * This method computes the inverse FFT of this instance of VB_Vector *
 * and stores the real and imaginary parts into the 2 input           *
 * VB_Vectors.                                                        *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * realPart           VB_Vector *     Will hold the real part of the  *
 *                                    computed inverse FFT.           *
 * imagPart           VB_Vector *     Will hold the imaginary part of *
 *                                    the computed inverse FFT.       *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::ifft(VB_Vector *realPart, VB_Vector *imagPart) const {
  /*********************************************************************
   * Ensuring that realPart and imagPart are appropriately sized.       *
   *********************************************************************/
  if (this->getLength() != realPart->getLength()) {
    realPart->resize(this->theVector->size);
  }  // if
  if (this->getLength() != imagPart->getLength()) {
    imagPart->resize(this->theVector->size);
  }  // if

  /*********************************************************************
   * status will be used to hold the return status values from the GSL  *
   * functions.                                                         *
   *********************************************************************/
  int status = 0;

  /*********************************************************************
   * complexVec[] will be a complex representation of the elements of   *
   * this instance of VB_Vector. Of course, the imaginary parts will    *
   * all be zero. The real parts will be in the even numbered indexes   *
   * of complexVec[] and the imaginary parts will be in the odd         *
   * numbered indexes of complexVec[].                                  *
   *********************************************************************/
  double complexVec[this->theVector->size * 2];

  /*********************************************************************
   * Calling gsl_fft_real_unpack() to populate complexVec[].            *
   *********************************************************************/
  status = gsl_fft_real_unpack(this->theVector->data,
                               (gsl_complex_packed_array)complexVec, 1,
                               this->theVector->size);

  /*********************************************************************
   * If status is non-zero, then VB_Vector::createException() is called *
   * to throw and catch an exception to inform the user of the error.   *
   * The error message will contain the reason for the error, as        *
   * returned by gsl_strerror(status).                                  *
   *********************************************************************/
  if (status) {
    VB_Vector::createException(string(gsl_strerror(status) + string(".")),
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * The following 2 variables are needed by GSL's                      *
   * gsl_fft_complex_backward() function. waveTable is a lookup table   *
   * for sines and cosines while workSpace is a required work space.    *
   *********************************************************************/
  gsl_fft_complex_wavetable *waveTable =
      gsl_fft_complex_wavetable_alloc(this->theVector->size);
  gsl_fft_complex_workspace *workSpace =
      gsl_fft_complex_workspace_alloc(this->theVector->size);

  /*********************************************************************
   * If waveTable is NULL, then an exception is thrown and then caught  *
   * by VB_Vector::createException() to inform the user of this error.  *
   *********************************************************************/
  if (!waveTable) {
    VB_Vector::createException("Unable to allocate gsl_fft_complex_wavetable.",
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * If workSpace is NULL, then an exception is thrown and then caught  *
   * by VB_Vector::createException() to inform the user of this error.  *
   *********************************************************************/
  if (!workSpace) {
    VB_Vector::createException("Unable to allocate gsl_fft_complex_workspace.",
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * Now computing the inverse FFT.                                     *
   *********************************************************************/
  status = gsl_fft_complex_backward(complexVec, 1, this->theVector->size,
                                    waveTable, workSpace);

  /*********************************************************************
   * If status is non-zero, then VB_Vector::createException() is called *
   * to throw and catch an exception to inform the user of the error.   *
   * The error message will contain the reason for the error, as        *
   * returned by gsl_strerror(status).                                  *
   *********************************************************************/
  if (status) {
    VB_Vector::createException(string(gsl_strerror(status) + string(".")),
                               __LINE__, __FILE__, __FUNCTION__);
  }  // if

  /*********************************************************************
   * At this point, complexVec[] contains the inverse FFT of this       *
   * instance of VB_Vector; its even indexed elements holds the real    *
   * parts and its odd indexed elements holds the imaginary part. Now   *
   * we populate realPart and imagPart appropriately with the following *
   * for loop. i indexes complexVec[] and j indexes realPart and        *
   * imagPart.                                                          *
   *********************************************************************/
  for (unsigned int i = 0, j = 0; j < this->theVector->size; j++) {
    /*********************************************************************
     * Assigning the real part of the the j'th complex number from        *
     * complexVec[] to (*realPart)[j]. Note that i is even and i++ causes *
     * i to be odd.                                                       *
     *********************************************************************/
    realPart->theVector->data[j] = complexVec[i++];

    /*********************************************************************
     * Assigning the imaginary part of the the j'th complex number from   *
     * complexVec[] to (*imagPart)[j]. Note that i is odd and i++ causes  *
     * i to be even.                                                      *
     *********************************************************************/
    imagPart->theVector->data[j] = complexVec[i++];

  }  // for i, j

  /*********************************************************************
   * Now freeing waveTable and workSpace.                               *
   *********************************************************************/
  gsl_fft_complex_wavetable_free(waveTable);
  gsl_fft_complex_workspace_free(workSpace);

}  // void VB_Vector::ifft(VB_Vector *realPart, VB_Vector *imagPart) const

/*********************************************************************
 * This method computes the inverse FFT of this instance of VB_Vector *
 * and stores the real and imaginary parts into the 2 input           *
 * VB_Vectors.                                                        *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * realPart           VB_Vector&      Will hold the real part of the  *
 *                                    computed inverse FFT.           *
 * imagPart           VB_Vector&      Will hold the imaginary part of *
 *                                    the computed inverse FFT.       *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::ifft(VB_Vector &realPart, VB_Vector &imagPart) const {
  /*********************************************************************
   * We now call this->ifft(VB_Vector *realPart, VB_Vector *imagPart).  *
   *********************************************************************/
  this->ifft(&realPart, &imagPart);

}  // void VB_Vector::ifft(VB_Vector& realPart, VB_Vector& imagPart) const

/*********************************************************************
 * Instance method computes the power spectrum of this instance of    *
 * VB_Vector. This method is derived from VoxBo_Fourier.pro's         *
 * ReturnPS function. The power spectrum will be stored in result.    *
 *********************************************************************/
void VB_Vector::getPS(VB_Vector &result) const {
  /*********************************************************************
   * Since we will need the FFT of this instance of VB_Vector, realPart *
   * is needed to hold the real part of the FFT and imagPart is need to *
   * hold the imaginary part of the FFT.                                *
   *********************************************************************/
  VB_Vector realPart(this->getLength());
  VB_Vector imagPart(this->getLength());

  /*********************************************************************
   * Ensuring that result is of the appropriate size.                   *
   *********************************************************************/
  if (this->theVector->size != result.theVector->size) {
    result.resize(this->theVector->size);
  }  // if

  /*********************************************************************
   * Geting the FFT of this instance of VB_Vector.                      *
   *********************************************************************/
  this->fft(realPart, imagPart);

  /*********************************************************************
   * The following for loop is used to assign the power spectrum values *
   * to result. The power spectrum values are computed from:            *
   *                                                                    *
   * fft(this) * conjugate(fft(this))                                   *
   *                                                                    *
   * where fft(this) is the FFT of this instance of VB_Vector, "*"      *
   * denotes element-by-element multiplication of the 2 VB_Vector's,    *
   * and conjuagte() is the complex comjugate of a VB_Vector.           *
   *********************************************************************/
  for (unsigned long i = 0; i < this->theVector->size; i++) {
    result[i] = (realPart[i] * realPart[i]) + (imagPart[i] * imagPart[i]);
  }  // for i

}  // void VB_Vector::getPS(VB_Vector &result) const

/*********************************************************************
 * Instance method to compute the power spectrum of this instance of  *
 * VB_Vector. This method is derived from VoxBo_Fourier.pro's         *
 * ReturnPS function. The power spectrum will be stored in result.    *
 *********************************************************************/
void VB_Vector::getPS(VB_Vector *result) const {
  this->getPS(*result);
}  // void VB_Vector::getPS(VB_Vector *result) const

/*********************************************************************
 * Instance method to compute the power spectrum of this instance of  *
 * VB_Vector. This method is derived from VoxBo_Fourier.pro's         *
 * ReturnPS function.                                                 *
 *********************************************************************/
void VB_Vector::getPS() throw() {
  VB_Vector tempVec(this->getLength());
  this->getPS(tempVec);
  (*this) = tempVec;

}  // void VB_Vector::getPS() throw()

/*********************************************************************
 * This method simply reverses the vector entries.                    *
 *********************************************************************/
void VB_Vector::reverse() {
  /*********************************************************************
   * Calling gsl_vector_reverse() to carry out the reversing of the     *
   * vector elements.                                                   *
   *********************************************************************/
  gsl_vector_reverse(this->theVector);

}  // void VB_Vector::reverse()

/*********************************************************************
 * This static method carries out Sinc Interpolation on the input     *
 * VB_Vector object. This method is derived from the IDL function     *
 * SincInterpo, found in VoxBo_Fourier.pro.                           *
 *                                                                    *
 * INPUT VARIABLES: TYPE:             DESCRIPTION:                    *
 * ---------------- -----             ------------                    *
 * timeSeries       const VB_Vector&  Input VB_Vector object that will*
 *                                    be sinc interpolated (this      *
 *                                    VB_Vector represents a time     *
 *                                    series).                        *
 * expFactor        int               The number of times the original*
 *                                    interval is to be expanded. For *
 *                                    example, if the interval is 1   *
 *                                    second long in the time series, *
 *                                    but half second intervals are   *
 *                                    desired, then expFactor should  *
 *                                    be 2.                           *
 * newSignal        VB_Vector&        Reference to the VB_Vector      *
 *                                    object that will hold the result*
 *                                    of the sinc interpolation. The  *
 *                                    length will be                  *
 *                                    (timeseries.getLength() *       *
 *                                    expFactor).                     *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 ********************************************************************/
void VB_Vector::sincInterpolation(const VB_Vector &timeSeries,
                                  const unsigned int expFactor,
                                  VB_Vector &newSignal) {
  /*********************************************************************
   * If the length of timeSeries is less than 2 (mostly like 1, but     *
   * could be zero), then an error message is printed and then this     *
   * program exits.                                                     *
   *********************************************************************/
  if (timeSeries.getLength() < 2) {
    ostringstream errorMsg;
    errorMsg << "[" << __FUNCTION__
             << "]: Need length to be >= 2. VB_Vector length = ["
             << timeSeries.getLength() << "].";
    printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
  }  // if

  /*********************************************************************
   * length is used to store the length of timeSeries.                  *
   *********************************************************************/
  const unsigned long length = timeSeries.getLength();

  /*********************************************************************
   * Now ensuring that newSignal is properly sized.                     *
   *********************************************************************/
  if (newSignal.getLength() != (length * expFactor)) {
    newSignal.resize(length * expFactor);
  }  // if

  /*********************************************************************
   * We now compute the FFT of timeSeries. realPartFFT is used to hold  *
   * the real part of timeSeries' FFT. Similarly, imagPartFFT is used   *
   * to hold the imaginary part of timeSeries' FFT.                     *
   *********************************************************************/
  VB_Vector realPartFFT(timeSeries.getLength());
  VB_Vector imagPartFFT(timeSeries.getLength());
  timeSeries.fft(realPartFFT, imagPartFFT);

  /*********************************************************************
   * halfLength is the floor of (timeSeries.getLength()/2).             *
   *********************************************************************/
  int halfLength = timeSeries.getLength() / 2;

  /*********************************************************************
   * phi[] is an array needed to carry out the sinc interpolation.      *
   * phi[0] will always be zero. The "lower half" values of phi[] will  *
   * be calculated and the "upper half" values of phi[] depend on its   *
   * "lower half" values. Specifically, the "lower half" values are     *
   * computed up to and including phi[halfLength/2]. Then the "lower    *
   * half" values are reflected about the x-axis, and then about the    *
   * vertical line y = halfLength. Assume that the x-axis indexes phi[] *
   * and the y-axis is phi[i]. When length is 10, the graph of phi[]    *
   * looks like:                                                        *
   *                                                                    *
   *                                                                    *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *    1.570 |                   o                                     *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *    1.256 |               o                                         *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *     .942 |           o                                             *
   *          |                                                         *
   *          |                                                         *
   *     .628 |       o                                                 *
   *          |                                                         *
   *     .314 |   o                                                     *
   *          |                                                         *
   *     -----o---+---+---+---+---+---+---+---+---+---+                 *
   *          |   1   2   3   4   5   6   7   8   9                     *
   *    -.314 |                                   o                     *
   *          |                                                         *
   *    -.628 |                               o                         *
   *          |                                                         *
   *          |                                                         *
   *    -.942 |                           o                             *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *   -1.256 |                       o                                 *
   *          |                                                         *
   *          |                                                         *
   *          |                                                         *
   *                                                                    *
   *********************************************************************/
  double phi[length];
  memset(phi, 0, sizeof(double) * length);

  /*********************************************************************
   * The following for loop is used to carry out the remaining work for *
   * the sinc interpolation.                                            *
   *********************************************************************/
  for (unsigned int i = 0; i < expFactor; i++) {
    /*********************************************************************
     * Calculating timeShift.                                             *
     *********************************************************************/
    double timeShift = (double)i / (double)expFactor;

    /*********************************************************************
     * Even number of values.                                             *
     *********************************************************************/
    if ((length % 2) == 0) {
      /*********************************************************************
       * Now calculating the values for phi[] (for the case of an even      *
       * number of values in timeSeries[].                                  *
       *********************************************************************/
      for (int j = 1; j <= halfLength; j++) {
        /*********************************************************************
         * The following line calculates the "lower half" values of phi[].    *
         *********************************************************************/
        phi[j] = (timeShift * TWOPI) / ((double)length / (double)j);

        /*********************************************************************
         * When j is not equal to halfLength, then calculate the "upper half" *
         * values of phi[].                                                   *
         *********************************************************************/
        if (j != halfLength) {
          phi[length - j] = -1.0 * phi[j];
        }  // if

      }  // for j

    }  // if

    /*********************************************************************
     * Odd number of values.                                              *
     *********************************************************************/
    else {
      /*********************************************************************
       * Now calculating the values for phi[] (for the case of an odd       *
       * number of values in timeSeries[].                                  *
       *********************************************************************/
      for (int j = 1; j <= halfLength; j++) {
        /*********************************************************************
         * The following line calculates the "lower half" values of phi[].    *
         *********************************************************************/
        phi[j] = (timeShift * TWOPI) / ((double)length / (double)j);

        /*********************************************************************
         * The following line calculates the "upper half" values of phi[].    *
         *********************************************************************/
        phi[length - j] = -1.0 * phi[j];

      }  // for j

    }  // else

    /*********************************************************************
     * The elements of shifterReal will hold the real part of the complex *
     * product [Complex(cos(phi[j]), sin(phi[j]) *                        *
     * Complex(realPartFFT[j], imagPartFFT[j])], while the elements of    *
     * shifterImag hold the imaginary part. The following for loop        *
     * populates both shifterReal and shifterImag. Eventually, we'll need *
     * to take the inverse FFT of (shifterReal + i*shifterImag).          *
     *********************************************************************/
    VB_Vector shifterReal(length);
    VB_Vector shifterImag(length);
    for (unsigned int j = 0; j < length; j++) {
      /*********************************************************************
       * Assigning the value of the jth element of shifterReal and the jth  *
       * element of shifterImag.                                            *
       *********************************************************************/
      shifterReal[j] =
          ((cos(phi[j]) * realPartFFT[j]) - (sin(phi[j]) * imagPartFFT[j]));
      shifterImag[j] =
          ((cos(phi[j]) * imagPartFFT[j]) + (sin(phi[j]) * realPartFFT[j]));

    }  // for j

    /*********************************************************************
     * We must view shifterReal and shifterImag as a complex vector,      *
     * whose jth element is (shifterReal[j] + i*shifterImag[j]). Since we *
     * do not have an acutal complex array, the inverse FFT of shifterReal*
     * and shifterImag need to be computed separately. To that end,       *
     * realPartFFTShifter is used to hold the real part of the inverse    *
     * FFT of shifterReal, imagPartFFTShifter is used to hold the         *
     * imaginary part of of the inverse FFT of shifterReal;               *
     * realPartFFTShifterImag is used to hold the real part of the        *
     * inverse FFT of shifterImag and imagPartFFTShifter is resued to     *
     * hold the imaginary part of of the inverse FFT of shifterImag (we   *
     * are not interested in the imaginary part of the inverse FFT's of   *
     * shifterReal). Ultimately, we will need just the real part of the   *
     * inverse FFT of the complex vector (shifterReal + i*shifterImag).   *
     *********************************************************************/
    VB_Vector realPartFFTShifter(shifterReal.getLength());
    VB_Vector imagPartFFTShifter(shifterReal.getLength());
    VB_Vector realPartFFTShifterImag(shifterImag.getLength());

    /*********************************************************************
     * Now computing the inverse FFT's.                                   *
     *********************************************************************/
    shifterReal.ifft(realPartFFTShifter, imagPartFFTShifter);
    shifterImag.ifft(realPartFFTShifterImag, imagPartFFTShifter);

    /*********************************************************************
     * Let (a + i*b) be the complex vector representing the inverse FFT   *
     * of shifterReal. Similarly, let (c + i*d) be the complex vector     *
     * representing the inverse FFT of shifterImag. Then the real part of *
     * the the inverse FFT of the complex vector (shifterReal +           *
     * i*shifterImag) is represented by the vector (a - d). So now we     *
     * place the real part in realPartFFTShifter.                         *
     *********************************************************************/
    realPartFFTShifter -= imagPartFFTShifter;

    /*********************************************************************
     * Now the real part of the inverse FFT of the complex vector         *
     * (shifterReal + i*shifterImag) is assigned to newSignal             *
     * appropriately.                                                     *
     *********************************************************************/
    for (unsigned int j = 0; j < length; j++) {
      newSignal[j * expFactor + i] = realPartFFTShifter[j];
    }  // for j

  }  // for i

}  // void VB_Vector::sincInterpolation(const VB_Vector& timeSeries,
   // const unsigned int expFactor, VB_Vector& newSignal)

/*********************************************************************
 * This constant instance method carries out Sinc Interpolation on    *
 * this instance of VB_Vector. This method is derived from the IDL    *
 * function SincInterpo, found in VoxBo_Fourier.pro.                  *
 *                                                                    *
 * INPUT VARIABLES: TYPE:             DESCRIPTION:                    *
 * ---------------- -----             ------------                    *
 * expFactor        int               The number of times the original*
 *                                    interval is to be expanded. For *
 *                                    example, if the interval is 1   *
 *                                    second long in the time series, *
 *                                    but half second intervals are   *
 *                                    desired, then expFactor should  *
 *                                    be 2.                           *
 * newSignal        VB_Vector&        Reference to the VB_Vector      *
 *                                    object that will hold the result*
 *                                    of the sinc interpolation. The  *
 *                                    length will be                  *
 *                                    (this->getLength() * expFactor).*
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 ********************************************************************/
void VB_Vector::sincInterpolation(const unsigned int expFactor,
                                  VB_Vector &newSignal) const {
  /*********************************************************************
   * Simply calling the static method to compute the sinc interpolation.*
   *********************************************************************/
  VB_Vector::sincInterpolation(*this, expFactor, newSignal);

}  // void VB_Vector::sincInterpolation(const unsigned int expFactor,
   // VB_Vector& newSignal) const

/*********************************************************************
 * This non-constant instance method carries out Sinc Interpolation on*
 * this instance of VB_Vector. This method is derived from the IDL    *
 * function SincInterpo, found in VoxBo_Fourier.pro.                  *
 *                                                                    *
 * INPUT VARIABLES: TYPE:             DESCRIPTION:                    *
 * ---------------- -----             ------------                    *
 * expFactor        int               The number of times the original*
 *                                    interval is to be expanded. For *
 *                                    example, if the interval is 1   *
 *                                    second long in the time series, *
 *                                    but half second intervals are   *
 *                                    desired, then expFactor should  *
 *                                    be 2.                           *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 ********************************************************************/
void VB_Vector::sincInterpolation(const unsigned int expFactor) {
  /*********************************************************************
   * tempVector is a copy of this instance of VB_Vector. newSignal will *
   * be used to hold the interpolated signal.                           *
   *********************************************************************/
  VB_Vector tempVector(this);
  VB_Vector newSignal = VB_Vector();

  /*********************************************************************
   * Doing the sinc interpolation and then assigning newSignal to this  *
   * instance of VB_Vector.                                             *
   *********************************************************************/
  tempVector.sincInterpolation(expFactor, newSignal);
  (*this) = newSignal;

}  // void VB_Vector::sincInterpolation(const unsigned int expFactor)

/*********************************************************************
 * This method is a port of the IDL function PhaseShift, found in     *
 * VoxBo_Fourier.pro. This method "shifts" a signal in time to        *
 * provide an output vector that represents the same continuous       *
 * signal sampled starting either later or earlier. This is           *
 * accomplished by a simple  shift of the phase of the sines that     *
 * makes up the signal. Recall that a Fourier transform allows for a  *
 * representation of any signal as the linear combination of          *
 * sinusoids of different frequencies and phases. Effectively, we will*
 * add a constant to the phase of every frequency, shifting the data  *
 * in time.                                                           *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:             DESCRIPTION:                  *
 * ----------------   -----             ------------                  *
 * tSeries            const VB_Vector&  The time series to be shifted.*
 *                                                                    *
 * timeShift          double            Number of images to shift to  *
 *                                      the right, i.e., to lag the   *
 *                                      signal with respect to the    *
 *                                      original domain.              *
 *                                                                    *
 * shiftedSignal      VB_Vector&        The VB_Vector object that     *
 *                                      will hold the shifted signal. *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * shiftedSignal       VB_Vector&      The shifted signal will be     *
 *                                     stored here.                   *
 ********************************************************************/
void VB_Vector::phaseShift(const VB_Vector &tSeries, const double timeShift,
                           VB_Vector &shiftedSignal) {
  /*********************************************************************
   * Making sure that shiftedSignal is appropriately sized.             *
   *********************************************************************/
  if (tSeries.getLength() != shiftedSignal.getLength()) {
    shiftedSignal.resize(tSeries.theVector->size);
  }  // if

  /*********************************************************************
   * phi[] is an array needed to carry out the phase shift. Now         *
   * declaring phi[] and setting all of its elements to zero.           *
   *********************************************************************/
  double *phi = new double[tSeries.theVector->size];
  memset(phi, 0, sizeof(double) * tSeries.getLength());

  /*********************************************************************
   * Now calling makePhi() to fill in the values in phi[].              *
   *********************************************************************/
  VB_Vector::makePhi(phi, tSeries.getLength(), timeShift);

  /*********************************************************************
   * Now taking the FFT of tSeries[]. realPartFFT is used to hold the   *
   * real part of the FFT, while imagPartFFT is used to hold the        *
   * imaginary part.                                                    *
   *********************************************************************/
  VB_Vector realPartFFT = VB_Vector();
  VB_Vector imagPartFFT = VB_Vector();
  tSeries.fft(realPartFFT, imagPartFFT);

  /*********************************************************************
   * shifter is the filter by which the signal will be convolved to     *
   * introduce the phase shift. It is constructed explicitly in the     *
   * Fourier, i.e., frequency, domain. In the time domain, it may be    *
   * described as an impulse (delta function) that has been shifted in  *
   * time by the amount specified by timeShift. shifter is actually     *
   * instantiated as two VB_Vector objects: shifterReal and shifterImag.*
   *********************************************************************/
  VB_Vector shifterReal(tSeries.getLength());
  VB_Vector shifterImag(tSeries.getLength());

  /*********************************************************************
   * The following for loop carries out the convolution.                *
   *********************************************************************/
  for (unsigned int i = 0; i < tSeries.getLength(); i++) {
    shifterReal[i] =
        (cos(phi[i]) * realPartFFT[i]) - (sin(phi[i]) * imagPartFFT[i]);
    shifterImag[i] =
        (cos(phi[i]) * imagPartFFT[i]) + (sin(phi[i]) * realPartFFT[i]);
  }  // for i

  /*********************************************************************
   * Now deleting the memory allocated for phi[].                       *
   *********************************************************************/
  delete[] phi;
  phi = 0;

  /*********************************************************************
   * Now we need to get the inverse FFT of shifter. Therefore, we need  *
   * to take the inverse FFT's of shifterReal and shifterImag. Each of  *
   * these inverse FFT's will have a real part and an imaginary part.   *
   *                                                                    *
   * For shifterReal, its inverse FFT parts will be stored in           *
   * shifterRealIFFTReal and shifterRealIFFTImag. For shifterImag, its  *
   * inverse FFT parts will be stored in shifterImagIFFTReal and        *
   * shifterImagIFFTImag.                                               *
   *********************************************************************/
  VB_Vector shifterRealIFFTReal(tSeries.getLength());
  VB_Vector shifterRealIFFTImag(tSeries.getLength());
  VB_Vector shifterImagIFFTReal(tSeries.getLength());
  VB_Vector shifterImagIFFTImag(tSeries.getLength());

  /*********************************************************************
   * Now computing the inverse FFT's of shifterReal and shifterImag.    *
   *********************************************************************/
  shifterReal.ifft(shifterRealIFFTReal, shifterRealIFFTImag);
  shifterImag.ifft(shifterImagIFFTReal, shifterImagIFFTImag);

  /*********************************************************************
   * Now assigning the real part of the inverse FFT of shifter to       *
   * shiftedSignal.                                                     *
   *********************************************************************/
  shiftedSignal = shifterRealIFFTReal - shifterImagIFFTImag;

}  // void VB_Vector::phaseShift(const VB_Vector& tSeries,
   // const double timeShift, VB_Vector& shiftedSignal)

/*********************************************************************
 * This instance method calls the static method                       *
 * VB_Vector::phaseShift() to compute the phase shift, using this     *
 * instance of VB_Vector as the first argument to the static          *
 * VB_Vector::phaseShift() method.                                    *
 *********************************************************************/
void VB_Vector::phaseShift(const double timeShift,
                           VB_Vector &shiftedSignal) const {
  VB_Vector::phaseShift(*this, timeShift, shiftedSignal);
}  // void VB_Vector::phaseShift(const double timeShift,
   // VB_Vector& shiftedSignal) const

/*********************************************************************
 * This instance method calls the static method                       *
 * VB_Vector::phaseShift() to compute the phase shift, using this     *
 * instance of VB_Vector as the first argument to the static          *
 * VB_Vector::phaseShift() method. Then the resulting phase shifted   *
 * VB_Vector is assigned to this instance of VB_Vector.               *
 *********************************************************************/
void VB_Vector::phaseShift(const double timeShift) {
  /*********************************************************************
   * tempVector will be used to hold the phase shifted vector.          *
   *********************************************************************/
  VB_Vector tempVector = VB_Vector();

  /*********************************************************************
   * Calling the static method VB_Vector::phaseShift() to carry out the *
   * phase shifting. Then tempVector is assigned to this instance of    *
   * VB_Vector.                                                         *
   *********************************************************************/
  VB_Vector::phaseShift(*this, timeShift, tempVector);
  (*this) = tempVector;

}  // void VB_Vector::phaseShift(const double timeShift)

/*********************************************************************
 * phi[] is an array needed to carry out the phase shift. phi[0]      *
 * will always be zero. The "lower half" values of phi[] will be      *
 * calculated and the "upper half" values of phi[] depend on its      *
 * "lower half" values. Specifically, the "lower half" values are     *
 * computed up to and including phi[length/2]. Then the "lower half"  *
 * values are reflected about the x-axis, and then about the          *
 * vertical line y = (length/2). Assume that the x-axis indexes phi[] *
 * and the y-axis is phi[i]. If length is 10, the graph of phi[] looks*
 * like:                                                              *
 *                                                                    *
 *                                                                    *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *    1.570 |                   o                                     *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *    1.256 |               o                                         *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *     .942 |           o                                             *
 *          |                                                         *
 *          |                                                         *
 *     .628 |       o                                                 *
 *          |                                                         *
 *     .314 |   o                                                     *
 *          |                                                         *
 *     -----o---+---+---+---+---+---+---+---+---+---+                 *
 *          |   1   2   3   4   5   6   7   8   9                     *
 *    -.314 |                                   o                     *
 *          |                                                         *
 *    -.628 |                               o                         *
 *          |                                                         *
 *          |                                                         *
 *    -.942 |                           o                             *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *   -1.256 |                       o                                 *
 *          |                                                         *
 *          |                                                         *
 *          |                                                         *
 *                                                                    *
 *                                                                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * phi                double *        Array used to hold the result.  *
 *                                                                    *
 * length             int             Length of phi[].                *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * phi                 double *       Holds the results.              *
 ********************************************************************/
void VB_Vector::makePhi(double *phi, const int length, const double timeShift) {
  /*********************************************************************
   * Setting all elements in phi[] to zero.                             *
   *********************************************************************/
  memset(phi, 0, sizeof(double) * length);

  /*********************************************************************
   * halfLength holds the floor of length / 2.                          *
   *********************************************************************/
  int halfLength = length / 2;

  /*********************************************************************
   * Even number of values.                                             *
   *********************************************************************/
  if ((length % 2) == 0) {
    /*********************************************************************
     * Now calculating the values for phi[] (for the case of an even      *
     * number of values in timeSeries[].                                  *
     *********************************************************************/
    for (int j = 1; j <= halfLength; j++) {
      /*********************************************************************
       * The following line calculates the "lower half" values of phi[].    *
       *********************************************************************/
      phi[j] = -1.0 * (timeShift * TWOPI) / ((double)length / (double)j);

      /*********************************************************************
       * When j is not equal to halfLength, then calculate the "upper half" *
       * values of phi[].                                                   *
       *********************************************************************/
      if (j != halfLength) {
        phi[length - j] = -1.0 * phi[j];
      }  // if

    }  // for j

  }  // if

  /*********************************************************************
   * Odd number of values.                                              *
   *********************************************************************/
  else {
    /*********************************************************************
     * Now calculating the values for phi[] (for the case of an odd       *
     * number of values in timeSeries[].                                  *
     *********************************************************************/
    for (int j = 1; j <= halfLength; j++) {
      /*********************************************************************
       * The following line calculates the "lower half" values of phi[].    *
       *********************************************************************/
      phi[j] = -1.0 * (timeShift * TWOPI) / ((double)length / (double)j);

      /*********************************************************************
       * The following line calculates the "upper half" values of phi[].    *
       *********************************************************************/
      phi[length - j] = -1.0 * phi[j];

    }  // for j

  }  // else

}  // void VB_Vector::makePhi(double *phi, const int length, const double
   // timeShift)

/*********************************************************************
 * This instance method normalizes the magnitude component of a signal*
 * to unity while preserving the phase component. This method is a    *
 * port of the IDL NormMag function, found in VoxBo_Fourier.pro.      *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * this               VB_Vector *     The input signal.               *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::normMag() {
  /*********************************************************************
   * We now need to compute the FFT of this instance of VB_Vector.      *
   * realFFT will hold the real part of the FFT, while imagIFFT will    *
   * hold the imaginary part of the FFT.                                *
   *********************************************************************/
  VB_Vector realFFT = VB_Vector();
  VB_Vector imagIFFT = VB_Vector();
  // DYK: the following ugly bit avoids some bad instability problems
  for (uint32 i = 0; i < this->getLength(); i++)
    if (abs((*this)[i]) < 1e-8) (*this)[i] = 0.0;
  this->fft(realFFT, imagIFFT);

  /*********************************************************************
   * magImage[] will hold the modulus of each complex number from the   *
   * FFT of this instance of VB_Vector.                                *
   *********************************************************************/
  double *magImage = new double[this->getLength()];

  /*********************************************************************
   * theZeros is used to hold the indices of where magImage is zero.    *
   *********************************************************************/
  vector<unsigned long> theZeros;

  /*********************************************************************
   * The following for loop populates magImage.                         *
   *********************************************************************/
  for (unsigned long i = 0; i < this->getLength(); i++) {
    magImage[i] = sqrt((realFFT[i] * realFFT[i]) + (imagIFFT[i] * imagIFFT[i]));

    /*********************************************************************
     * NOTE: If the modulus is zero, then the phase angle is undefined.   *
     * To get around this, magImage[i] is set to 1.0, temporarily. Also,  *
     * the index is added to theZeros.                                    *
     *********************************************************************/
    if (magImage[i] == 0.0) {
      magImage[i] = 1.0;
      theZeros.push_back(i);
    }  // if

  }  // for i

  /*********************************************************************
   * phaseImage[i] will be used to hold the arc-cosine of realFTT[i]    *
   * divided by magImage[i].                                            *
   *********************************************************************/
  double *phaseImage = new double[this->getLength()];

  /*********************************************************************
   * This for loop populates realFFT[i].                                *
   *********************************************************************/
  for (unsigned long i = 0; i < this->getLength(); i++) {
    phaseImage[i] = acos(realFFT[i] / magImage[i]);

    /*********************************************************************
     * Since the domain of acos() is limited, we need to check the sign   *
     * of imagIFFT[i].                                                     *
     *********************************************************************/
    if (imagIFFT[i] < 0.0) {
      phaseImage[i] = TWOPI - phaseImage[i];
    }  // if
  }    // for i

  /*********************************************************************
   * The following for loop is sued to "restore" the zero values in     *
   * magImage[] and phaseImage[].                                       *
   *********************************************************************/
  for (unsigned long i = 0; i < theZeros.size(); i++) {
    magImage[theZeros[i]] = 0.0;
    phaseImage[theZeros[i]] = 0.0;
  }  // for i

  /*********************************************************************
   * magImageMax is assigned the maximum value of magImage[].           *
   *********************************************************************/
  double magImageMax = magImage[0];
  for (size_t i = 1; i < this->getLength(); i++)
    if (magImage[i] > magImageMax) magImageMax = magImage[i];

  /*********************************************************************
   * realPart is used to hold the real part of the normalized signal    *
   * and imagPart is used to hold the imaginary part of the normalized  *
   * signal.                                                            *
   *********************************************************************/
  VB_Vector realPart(this->getLength());
  VB_Vector imagPart(this->getLength());

  /*********************************************************************
   * The following for loop is used to populate both realPart and       *
   * imagPart.                                                          *
   *********************************************************************/
  for (unsigned long i = 0; i < this->getLength(); i++) {
    realPart[i] = (magImage[i] / magImageMax) * cos(phaseImage[i]);
    imagPart[i] = (magImage[i] / magImageMax) * sin(phaseImage[i]);
  }  // for i

  /*********************************************************************
   * We now need to convert back to the time domain. realPartFFTReal    *
   * will be used to hold the real part of the inverse FFT of realPart  *
   * and realPartFFTImag will be used to hold the imaginary part of the *
   * inverse FFT of realPart.                                           *
   *********************************************************************/
  VB_Vector realPartFFTReal = VB_Vector();
  VB_Vector realPartFFTImag = VB_Vector();
  realPart.ifft(realPartFFTReal, realPartFFTImag);

  /*********************************************************************
   * imagPartFFTReal will be used to hold the real part of the inverse  *
   * FFT of imagPart and imagPartFFTImag will be used to hold the       *
   * imaginary part of the inverse FFT of imagPart.                     *
   *********************************************************************/
  VB_Vector imagPartFFTReal = VB_Vector();
  VB_Vector imagPartFFTImag = VB_Vector();
  imagPart.ifft(imagPartFFTReal, imagPartFFTImag);

  /*********************************************************************
   * Now setting this instance of VB_Vector to its normalized form.     *
   *********************************************************************/
  (*this) = realPartFFTReal - imagPartFFTImag;

  /*********************************************************************
   * Deleting previously allocated memory.                              *
   *********************************************************************/
  delete[] phaseImage;
  phaseImage = NULL;
  delete[] magImage;
  magImage = NULL;

}  // void VB_Vector::normMag()

/*********************************************************************
 * This instance method normalizes the magnitude component of a signal*
 * to unity while preserving the phase component. This method is a    *
 * port of the IDL NormMag function, found in VoxBo_Fourier.pro.      *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * normalizedVec      VB_Vector&      Will hold the normalized vector.*
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::normMag(VB_Vector &normalizedVec) const {
  /*********************************************************************
   * Setting normalizedVec equal to this instance of VB_Vector and then *
   * normalizing normalizedVec.                                         *
   *********************************************************************/
  normalizedVec = (*this);
  normalizedVec.normMag();

}  // void VB_Vector::normMag(VB_Vector& normalizedVec) const

/*********************************************************************
 * This method applies the input function (actually a function        *
 * pointer) to each element of this instance of VB_Vector. The        *
 * input function must take a single double argument and return a     *
 * double. This method is meant to apply sqrt(), cos(), sin(), etc.,  *
 * to each element.                                                   *
 *                                                                    *
 * INPUT VARIABLES:  TYPE:                 DESCRIPTION:               *
 * ----------------  -----                 ------------               *
 * theFunction       double (*) (double)   A pointer to a function    *
 *                                         that takes a single        *
 *                                         double argument and        *
 *                                         returns a double.          *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::applyFunction(double (*theFunction)(double)) throw() {
  /*********************************************************************
   * The following for loop is used to traverse this instance of        *
   * VB_Vector and apply the input function to each element.            *
   *********************************************************************/
  for (unsigned long i = 0; i < this->getLength(); i++) {
    (*this)[i] = (*theFunction)((*this)[i]);
  }  // for i

}  // void VB_Vector::applyFunction(double (*theFunction)(double)) throw()

/*********************************************************************
 * This method applies the input function (actually a function        *
 * pointer) to each element of this instance of VB_Vector and stores  *
 * the result in theResult. The input function must take a single     *
 * double argument and return a double. This method is meant to       *
 * apply sqrt(), cos(), sin(), etc., to each element.                 *
 *                                                                    *
 * INPUT VARIABLES:  TYPE:                 DESCRIPTION:               *
 * ----------------  -----                 ------------               *
 * theFunction       double (*) (double)   A pointer to a function    *
 *                                         that takes a single        *
 *                                         double argument and        *
 *                                         returns a double.          *
 * theResult         VB_Vector&            Will hold the result.      *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::applyFunction(double (*theFunction)(double),
                              VB_Vector &theResult) const throw() {
  /*********************************************************************
   * Setting theResult equal to this instance of VB_Vector and then     *
   * calling VB_Vector::applyFunction().                                *
   *********************************************************************/
  theResult = (*this);
  theResult.applyFunction(theFunction);

}  // void VB_Vector::applyFunction(double (*theFunction)(double), VB_Vector&
   // theResult) const throw()

/*********************************************************************
 * This method applies the input function (actually a function        *
 * pointer) to each element of this instance of VB_Vector and stores  *
 * the result in theResult. The input function must take a single     *
 * double argument and return a double. This method is meant to       *
 * apply sqrt(), cos(), sin(), etc., to each element.                 *
 *                                                                    *
 * INPUT VARIABLES:  TYPE:                 DESCRIPTION:               *
 * ----------------  -----                 ------------               *
 * theFunction       double (*) (double)   A pointer to a function    *
 *                                         that takes a single        *
 *                                         double argument and        *
 *                                         returns a double.          *
 * theResult         VB_Vector *           Will hold the result.      *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::applyFunction(double (*theFunction)(double),
                              VB_Vector *theResult) const throw() {
  /*********************************************************************
   * Setting theResult equal to this instance of VB_Vector and then     *
   * calling VB_Vector::applyFunction().                                *
   *********************************************************************/
  (*theResult) = (*this);
  theResult->applyFunction(theFunction);

}  // void VB_Vector::applyFunction(double (*theFunction)(double), VB_Vector
   // *theResult) const throw()

/*********************************************************************
 * This method carries out an element-by-element multiplication       *
 * between this instance of VB_Vector and the input VB_Vector. This   *
 * instance of VB_Vector is overwritten with the new values.          *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:              DESCRIPTION:                 *
 * ----------------   -----              ------------                 *
 * vec                const VB_Vector *  The input vector.            *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::elementByElementMult(const VB_Vector *vec) throw() {
  /*********************************************************************
   * The following try/catch blocks are used to ensure that the vector  *
   * lengths are equal.                                                 *
   *********************************************************************/
  try {
    VB_Vector::checkVectorLengths(this->theVector, vec->theVector, __LINE__,
                                  __FILE__, __FUNCTION__);
  }  // try
  catch (GenericExcep &e) {
    e.whatNoExit(__LINE__, __FILE__, __FUNCTION__);
  }  // catch

  /*********************************************************************
   * The following for loop carries out the element-by-element          *
   * multiplication.                                                    *
   *********************************************************************/
  for (unsigned long i = 0; i < this->getLength(); i++) {
    (*this)[i] *= (*vec)[i];
  }  // for i

}  // void VB_Vector::elementByElementMult(const VB_Vector *vec) throw()

/*********************************************************************
 * This method carries out an element-by-element multiplication       *
 * between this instance of VB_Vector and the input VB_Vector. This   *
 * instance of VB_Vector is overwritten with the new values.          *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:              DESCRIPTION:                 *
 * ----------------   -----              ------------                 *
 * vec                const VB_Vector&   The input vector.            *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void VB_Vector::elementByElementMult(const VB_Vector &vec) throw() {
  /*********************************************************************
   * Calling VB_Vector::elementByElementMult(const VB_Vector *vec) to   *
   * carry out the element-by-element multiplication.                   *
   *********************************************************************/
  this->elementByElementMult(&vec);

}  // void VB_Vector::elementByElementMult(const VB_Vector& vec) throw()

/*********************************************************************
 * This static method multiplies 2 "complex" VB_Vectors: the first    *
 * complex VB_Vector has its real and imaginary parts passed in;      *
 * similarly for the second VB_Vector. The real part of the product   *
 * will be saved to realProd and the imaginary part will be saved to  *
 * imagProd.                                                          *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:             DESCRIPTION:                  *
 * ----------------   -----             ------------                  *
 * real1              const VB_Vector&  The real part of the first    *
 *                                      VB_Vector.                    *
 * imag1              const VB_Vector&  The imaginary part of the     *
 *                                      first VB_Vector.              *
 * real2              const VB_Vector&  The real part of the second   *
 *                                      VB_Vector.                    *
 * imag2              const VB_Vector&  The imaginary part of the     *
 *                                      second VB_Vector.             *
 * realProd           VB_Vector&        Will hold the real part of    *
 *                                      the product.                  *
 * imagProd           VB_Vector&        Will hold the imaginary part  *
 *                                      of the product.               *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * GenericExcep                                                       *
 *********************************************************************/
void VB_Vector::compMult(const VB_Vector &real1, const VB_Vector &imag1,
                         const VB_Vector &real2, const VB_Vector &imag2,
                         VB_Vector &realProd,
                         VB_Vector &imagProd) throw(GenericExcep) {
  /*********************************************************************
   * First, we need to ensure that all the vector lengths are equal. If *
   * any of the sizes do not match, then a GenericExcep is thrown.      *
   *********************************************************************/
  VB_Vector::checkVectorLengths(real1.theVector, imag1.theVector, __LINE__,
                                __FILE__, __FUNCTION__);
  VB_Vector::checkVectorLengths(real2.theVector, imag2.theVector, __LINE__,
                                __FILE__, __FUNCTION__);
  VB_Vector::checkVectorLengths(real1.theVector, imag2.theVector, __LINE__,
                                __FILE__, __FUNCTION__);

  /*********************************************************************
   * Ensuring that realProd and imagProd are appropriately sized.       *
   *********************************************************************/
  if (real1.getLength() != realProd.getLength()) {
    realProd.resize(real1.theVector->size);
  }  // if
  if (real1.getLength() != imagProd.getLength()) {
    imagProd.resize(real1.theVector->size);
  }  // if

  /*********************************************************************
   * The following for loop carries out the complex multiplication. The *
   * real part of the product is saved to realProd, while the imaginary *
   * part is saved to imagProd.                                         *
   *********************************************************************/
  for (unsigned long i = 0; i < real1.theVector->size; i++) {
    realProd.theVector->data[i] =
        (real1.theVector->data[i] * real2.theVector->data[i]) -
        (imag1.theVector->data[i] * imag2.theVector->data[i]);
    imagProd.theVector->data[i] =
        (real1.theVector->data[i] * imag2.theVector->data[i]) +
        (imag1.theVector->data[i] * real2.theVector->data[i]);
  }  // for i

}  // void VB_Vector::compMult(const VB_Vector& real1, const VB_Vector& imag1,
   // const VB_Vector& real2, const VB_Vector& imag2, VB_Vector& realProd,
   // VB_Vector& imagProd) throw (GenericExcep)

/*********************************************************************
 * This static method computes the FFT of a "complex" VB_Vector. The  *
 * real part of the VB_Vector is passed in as real and the imaginary  *
 * part of the VB_Vector is passed in as imag. The real part of the   *
 * FFT is saved in realFFT while the imaginary part is saved to       *
 * imagIFFT.                                                          *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:             DESCRIPTION:                  *
 * ----------------   -----             ------------                  *
 * real               const VB_Vector&  The real part of the input    *
 *                                      VB_Vector.                    *
 * imag               const VB_Vector&  The imaginary part of the     *
 *                                      input VB_Vector.              *
 * realFFT            VB_Vector&        Will hold the real part       *
 *                                      of the FFT.                   *
 * imagIFFT            VB_Vector&       Will hold the imaginary part  *
 *                                      of the FFT.                   *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * GenericExcep                                                       *
 *********************************************************************/
void VB_Vector::complexFFT(const VB_Vector &real, const VB_Vector &imag,
                           VB_Vector &realFFT,
                           VB_Vector &imagIFFT) throw(GenericExcep) {
  /*********************************************************************
   * Ensuring that the input VB_Vectors real and imag have the same     *
   * size. If they don't have the same size, then a GenericExcep is     *
   * thrown.                                                            *
   *********************************************************************/
  VB_Vector::checkVectorLengths(real.theVector, imag.theVector, __LINE__,
                                __FILE__, __FUNCTION__);

  /*********************************************************************
   * Ensuring that realFFT and imagIFFT are sized appropriately.        *
   *********************************************************************/
  if (real.getLength() != realFFT.getLength()) {
    realFFT.resize(real.theVector->size);
  }  // if
  if (real.getLength() != imagIFFT.getLength()) {
    imagIFFT.resize(real.theVector->size);
  }  // if

  /*********************************************************************
   * realFFTreal and realFFTimag will hold the real and imaginary parts *
   * of the FFT of real, respectively.                                  *
   *********************************************************************/
  VB_Vector realFFTreal(real.getLength());
  VB_Vector realFFTimag(real.getLength());

  /*********************************************************************
   * imagIFFTreal and imagIFFTimag will hold the real and imaginary     *
   * parts of the FFT of imag, respectively.                            *
   *********************************************************************/
  VB_Vector imagIFFTreal(real.getLength());
  VB_Vector imagIFFTimag(real.getLength());

  /*********************************************************************
   * Getting the FFT of real.                                           *
   *********************************************************************/
  real.fft(realFFTreal, realFFTimag);

  /*********************************************************************
   * Getting the FFT of imag.                                           *
   *********************************************************************/
  imag.fft(imagIFFTreal, imagIFFTimag);

  /*********************************************************************
   * Now realFFT will hold the real part of the FFT of                  *
   * (real + i * imag).                                                 *
   *********************************************************************/
  realFFT = realFFTreal - imagIFFTimag;

  /*********************************************************************
   * Now imagIFFT will hold the imaginary part of the FFT of            *
   * (real + i * imag).                                                 *
   *********************************************************************/
  imagIFFT = realFFTimag + imagIFFTreal;

}  // void VB_Vector::complexFFT(const VB_Vector& real,
   // const VB_Vector& imag, VB_Vector& realFFT, VB_Vector& imagIFFT)
   // throw (GenericExcep)

/*********************************************************************
 * This static method computes the inverse FFT of a "complex"         *
 * VB_Vector. The real part of the VB_Vector is passed in as real and *
 * the imaginary part of the VB_Vector is passed in as imag. The      *
 * real part of the inverse FFT is saved in realIFFT while the        *
 * imaginary part is saved to imagIFFT.                               *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:             DESCRIPTION:                  *
 * ----------------   -----             ------------                  *
 * real               const VB_Vector&  The real part of the input    *
 *                                      VB_Vector.                    *
 * imag               const VB_Vector&  The imaginary part of the     *
 *                                      input VB_Vector.              *
 * realIFFT            VB_Vector&        Will hold the real part      *
 *                                      of the inverse FFT.           *
 * imagIFFT            VB_Vector&        Will hold the imaginary part *
 *                                      of the inverse FFT.           *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * GenericExcep                                                       *
 *********************************************************************/
void VB_Vector::complexIFFT(const VB_Vector &real, const VB_Vector &imag,
                            VB_Vector &realIFFT,
                            VB_Vector &imagIFFT) throw(GenericExcep) {
  /*********************************************************************
   * Ensuring that the input VB_Vectors real and imag have the same     *
   * size. If they don't have the same size, then a GenericExcep is     *
   * thrown.                                                            *
   *********************************************************************/
  VB_Vector::checkVectorLengths(real.theVector, imag.theVector, __LINE__,
                                __FILE__, __FUNCTION__);

  /*********************************************************************
   * Ensuring that realIFFT and imagIFFT are sized appropriately.       *
   *********************************************************************/
  if (real.getLength() != realIFFT.getLength()) {
    realIFFT.resize(real.theVector->size);
  }  // if
  if (real.getLength() != imagIFFT.getLength()) {
    imagIFFT.resize(real.theVector->size);
  }  // if

  /*********************************************************************
   * realIFFTreal and realIFFTimag will hold the real and imaginary     *
   * parts of the inverse FFT of real, respectively.                    *
   *********************************************************************/
  VB_Vector realIFFTreal(real.getLength());
  VB_Vector realIFFTimag(real.getLength());

  /*********************************************************************
   * imagIFFTreal and imagIFFTimag will hold the real and imaginary     *
   * parts of the inverse FFT of imag, respectively.                    *
   *********************************************************************/
  VB_Vector imagIFFTreal(real.getLength());
  VB_Vector imagIFFTimag(real.getLength());

  /*********************************************************************
   * Getting the inverse FFT of real.                                   *
   *********************************************************************/
  real.ifft(realIFFTreal, realIFFTimag);

  /*********************************************************************
   * Getting the inverse FFT of imag.                                   *
   *********************************************************************/
  imag.ifft(imagIFFTreal, imagIFFTimag);

  /*********************************************************************
   * Now realIFFT will hold the real part of the inverse FFT of         *
   * (real + i * imag).                                                 *
   *********************************************************************/
  realIFFT = realIFFTreal - imagIFFTimag;

  /*********************************************************************
   * Now imagIFFT will hold the imaginary part of the inverse FFT of    *
   * (real + i * imag).                                                 *
   *********************************************************************/
  imagIFFT = realIFFTimag + imagIFFTreal;

}  // void VB_Vector::complexIFFT(const VB_Vector& real,
   // const VB_Vector& imag, VB_Vector& realIFFT, VB_Vector& imagIFFT)
   // throw (GenericExcep)

/*********************************************************************
 * This static method computes the inverse FFT of a "complex"         *
 * VB_Vector. The real part of the VB_Vector is passed in as real and *
 * the imaginary part of the VB_Vector is passed in as imag. The      *
 * real part of the inverse FFT is saved in realIFFT while the        *
 * imaginary part is not saved.                                       *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:             DESCRIPTION:                  *
 * ----------------   -----             ------------                  *
 * real               const VB_Vector&  The real part of the input    *
 *                                      VB_Vector.                    *
 * imag               const VB_Vector&  The imaginary part of the     *
 *                                      input VB_Vector.              *
 * realIFFT            VB_Vector&        Will hold the real part      *
 *                                      of the inverse FFT.           *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                                                                *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * GenericExcep                                                       *
 *********************************************************************/
void VB_Vector::complexIFFTReal(const VB_Vector &real, const VB_Vector &imag,
                                VB_Vector &realIFFT) throw(GenericExcep) {
  /*********************************************************************
   * Ensuring that the input VB_Vectors real and imag have the same     *
   * size. If they don't have the same size, then a GenericExcep is     *
   * thrown.                                                            *
   *********************************************************************/
  VB_Vector::checkVectorLengths(real.theVector, imag.theVector, __LINE__,
                                __FILE__, __FUNCTION__);

  /*********************************************************************
   * Ensuring that realIFFT is sized appropriately.                     *
   *********************************************************************/
  if (real.getLength() != realIFFT.getLength()) {
    realIFFT.resize(real.theVector->size);
  }  // if

  /*********************************************************************
   * realIFFTreal and realIFFTimag will hold the real and imaginary     *
   * parts of the inverse FFT of real, respectively.                    *
   *********************************************************************/
  VB_Vector realIFFTreal(real.getLength());
  VB_Vector realIFFTimag(real.getLength());

  /*********************************************************************
   * imagIFFTreal and imagIFFTimag will hold the real and imaginary     *
   * parts of the inverse FFT of imag, respectively.                    *
   *********************************************************************/
  VB_Vector imagIFFTreal(real.getLength());
  VB_Vector imagIFFTimag(real.getLength());

  /*********************************************************************
   * Getting the inverse FFT of real.                                   *
   *********************************************************************/
  real.ifft(realIFFTreal, realIFFTimag);

  /*********************************************************************
   * Getting the inverse FFT of imag.                                   *
   *********************************************************************/
  imag.ifft(imagIFFTreal, imagIFFTimag);

  /*********************************************************************
   * Now realIFFT will hold the real part of the inverse FFT of         *
   * (real + i * imag).                                                 *
   *********************************************************************/
  realIFFT = realIFFTreal - imagIFFTimag;

}  // void VB_Vector::complexIFFTReal(const VB_Vector& real,
   // const VB_Vector& imag, VB_Vector& realIFFT) throw (GenericExcep)

int VB_Vector::meanNormalize() {
  double avg = this->getVectorMean();
  if (fabs(avg) < 1.0) {
    if (avg < 0.0) {
      *this -= 1.0;
      avg -= 1.0;
    } else if (avg >= 0.0) {
      *this += 1.0;
      avg += 1.0;
    }
  }
  *this /= avg;
  return 0;
}

int VB_Vector::removeDrift() {
  double intercept = 0.0, slope = 0.0, cov00 = 0.0, cov01 = 0.0, cov11 = 0.0,
         chisq = 0.0;
  int size = getLength();
  double mean = 0;
  double x[size], y[size], w[size];
  for (int i = 0; i < size; i++) {
    x[i] = i;
    y[i] = getElement(i);
    w[i] = 1.0;
  }
  gsl_fit_wlinear(x, 1, w, 1, y, 1, size, &intercept, &slope, &cov00, &cov01,
                  &cov11, &chisq);
  mean = getVectorMean();
  for (int index = 0; index < size; index++)
    setElement(index, (getElement(index) - (intercept + slope * index)) + mean);
  return 0;
}

double *VB_Vector::begin() const {
  if (this->theVector) return this->theVector->data;
  return NULL;
}

double *VB_Vector::end() const {
  if (this->theVector) return this->theVector->data + this->theVector->size;
  return NULL;
}  // double* VB_Vector::end() const

// FIXME : DYK : added the following functions to handle the new i/o system

int VB_Vector::ReadFile(const string &fname) {
  fileName = fname;
  vector<VBFF> ftypes = EligibleFileTypes(fname, 1);
  if (ftypes.size() == 0) return 101;
  // FIXME on error we could be nice and try multiple types
  fileFormat = ftypes[0];
  if (!fileFormat.read_1D) return 102;
  int err = fileFormat.read_1D(this);
  return err;

  // Vec iovec;
  // int err;
  // if ((err=iovec.ReadFile(fname)))
  //   return err;
  // fileFormat=iovec.fileformat;
  // init(iovec.size());
  // memcpy(this->theVector->data, iovec.data, sizeof(double) *
  // this->getLength()); return 0;
}

int VB_Vector::WriteFile(string fname) {
  VBFF original;
  // save the original format, then null it
  original = fileFormat;
  fileFormat.init();
  if (fname.size()) fileName = fname;
  if (!fileFormat.write_1D)  // should always be true
    fileFormat = findFileFormat(fileName, 1);
  // if not, try original file's format
  if (!fileFormat.write_1D) fileFormat = original;
  // if not, try cub1
  if (!fileFormat.write_1D) fileFormat = findFileFormat("ref1");
  // if not (should never happen), bail
  if (!fileFormat.write_1D) return 200;
  int err = fileFormat.write_1D(this);
  return err;

  // Vec iovec;
  // int err;
  // iovec.resize(getLength());
  // memcpy(iovec.data, this->theVector->data, sizeof(double) *
  // this->getLength()); iovec.fileformat=fileFormat;
  // iovec.SetFileName(fileName);
  // iovec.header=header;
  // err=iovec.WriteFile();
  // return err;
}

// DYK: added the following non-member function to do a t-test

double ttest(const VB_Vector &v1, const VB_Vector &v2) {
  double s1 = v1.getVariance();
  double s2 = v2.getVariance();
  double mean1 = v1.getVectorMean();
  double mean2 = v2.getVectorMean();
  return (mean1 - mean2) / sqrt((s1 / v1.getLength()) + (s2 / v2.getLength()));
}

// DYK: added the following non-member function to calculate power
// spectrum

VB_Vector fftnyquist(VB_Vector &vv) {
  int len = vv.getLength();
  VB_Vector fullFFT(len);
  vv.getPS(&fullFFT);

  int newlen = len / 2 + 1;
  VB_Vector halfFFT(newlen);

  for (int i = 0; i < newlen; i++) {
    halfFFT.setElement(i, fullFFT.getElement(i));
  }
  return halfFFT;
}

// DYK: added to the following better-behaved function for resampling
// vectors

VB_Vector cspline_resize(VB_Vector vec, double factor) {
  int newsize = (int)((float)vec.size() * factor);
  VB_Vector bogus(vec.size());
  for (size_t i = 0; i < vec.size(); i++) bogus.setElement(i, i);
  VB_Vector newvector(newsize);
  double interval = 1.0 / factor;

  double *xptr = bogus.getTheVector()->data;
  double *yptr = vec.getTheVector()->data;
  gsl_interp *myinterp = gsl_interp_alloc(gsl_interp_cspline, vec.size());
  // gsl_interp *myinterp=gsl_interp_alloc(gsl_interp_linear,vec.size());
  gsl_interp_init(myinterp, xptr, yptr, vec.size());
  double val, pos = 0.0;
  for (int i = 0; i < newsize; i++) {
    val = gsl_interp_eval(myinterp, xptr, yptr, pos, NULL);
    newvector.setElement(i, val);
    pos += interval;
  }
  gsl_interp_free(myinterp);
  return newvector;
}

// DYK: added the following useful functions

double correlation(const VB_Vector &v1, const VB_Vector &v2) {
  if (v1.size() != v2.size()) return nan("");
  double sd1 = sqrt(v1.getVariance());
  double sd2 = sqrt(v2.getVariance());
  return (covariance(v1, v2) / (sd1 * sd2));
}

double covariance(const VB_Vector &v1, const VB_Vector &v2) {
  if (v1.size() != v2.size()) return nan("");
  return gsl_stats_covariance(v1.getTheVector()->data, 1,
                              v2.getTheVector()->data, 1, v1.size());
}

int VB_Vector::permute(const VB_Vector &v) {
  if (size() != v.size()) return 1;
  VB_Vector tmp(size());
  for (size_t i = 0; i < size(); i++) tmp[i] = getElement((int)v[i]);
  for (size_t i = 0; i < size(); i++) setElement(i, tmp[i]);
  return 0;
}

int VB_Vector::permute(VBMatrix &m, int col) {
  VB_Vector v;
  v = m.GetColumn(col);
  return permute(v);
}
