


// DEFUNCT


// vec.cpp
// VoxBo vector file I/O code
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
// original version written by Dan Kimberg
// numerous changes/additions by Kosh Banerjee
// many of those changes later undone by Dan
//
// original version written by Dan Kimberg



// class Vec {
//  private:
//   string filename;
//  public:
//   double *data;
//   int length,valid;
//   VB_datatype datatype;

//   VBFF fileformat;
//   vector<string>header;       // unformatted text header

//   Vec();
//   Vec(const Vec &old);        // copy constructor
//   Vec(const VB_Vector& orig); // copy from a VB_Vector
//   Vec(const string &);        // load from a file
//   Vec(const char*);           // load from file (disambiguation)
//   Vec(int len);               // just allocate it
//   ~Vec();
//   void print();
//   void printbrief(const string &flags="");            // print some info

//   // the new i/o functions
//   int ReadFile(const string &fname);
//   int WriteFile(const string fname="");
//   //   int WriteFile(const string &fname);
//   void AddHeader(const string &str);       // tack string onto header

//   double& operator[](const int index);
//   double& operator()(const int index);
//   void SetValue(const int index,double value);
//   int size() const;
//   double max() const;
//   void resize(int len);

//   // operators
//   Vec &operator=(const Vec &old);
//   Vec &operator+=(double val);
//   Vec &operator-=(double val);
//   Vec &operator*=(double val);
//   Vec &operator/=(double val);
//   Vec &operator+=(Vec &v);
//   Vec &operator-=(Vec &v);
//   Vec &operator*=(Vec &v);
//   Vec &operator/=(Vec &v);
//   // accessors
//   string GetFileName() const;
//   void SetFileName(const string &fname);
// };



using namespace std;

#include "vbutil.h"
#include "vbio.h"

//////////////////////////////////////////////////////////
// vectors are always doubles
// when loading a REF1, space is allocated dynamically in
//   increments of 100 doubles
// when writing a REF1, all comments are discarded
//////////////////////////////////////////////////////////

Vec::Vec()
{
  data=(double *)NULL;
  length=0;
  valid=0;
}

Vec::Vec(int len)
{
  data = new double[len];
  length = len;
  if (data) {
    memset(data,0,len*sizeof(double));
    valid=1;
  }
}

Vec::Vec(const VB_Vector& orig)
{
  length = orig.size();
  data = new double[length];
  if (data)
  {
    memcpy(data, orig.theVector->data, sizeof(double) * length);
    valid = 1;
  }
  else
  {
    fprintf(stderr,"vbcrunch failed to allocate space for a vector\n");
    exit(5);
  }
  
  
  for (int i = 0; i < length; ++i)
    data[i] = orig[i];
}

Vec::Vec(const string &fname)
{
  data=(double *)NULL;
  length=0;
  valid=0;
  ReadFile(fname);
}

Vec::Vec(const char* fname)
{
  data=(double *)NULL;
  length=0;
  valid=0;
  ReadFile(fname);
}

Vec::~Vec()
{
  delete [] data;
  data = (double *)NULL;
}

// copy constructor

Vec::Vec(const Vec &old)
{
  data=(double *)NULL;
  length=0;
  valid=0;
  *this=old;
}

Vec &
Vec::operator=(const Vec &old)
{
  if (old.data && old.length > 0) {
    length=old.length;
    fileformat=old.fileformat;
    valid = old.valid;
    data = new double[old.length];
    if (!data) {
      fprintf(stderr,"vbcrunch failed to allocate space for a vector\n");
      exit(5);
    }
    memcpy(data,old.data,length*sizeof(double));
  }
  else {
    data = (double *)NULL;
    length=0;
    valid=0;
  }
  return *this;
}


void
Vec::resize(int len)
{
  if (data)
    delete [] data;
  data=new double[len];
  length=len;
  if (data) {
    memset(data,0,len*sizeof(double));
    valid=1;
  }
  else
    valid=0;
}

void
Vec::print()
{
  printbrief();
  return;
}

void
Vec::printbrief(const string &)  // flag argument not used at present
{
  VB_Vector tmpv(this);
  
  double mean;
  mean=0;
  for (int i=0; i<length; i++)
    mean+=data[i];
  mean /= length;
  printf("%s: %d elements, mean of %g, variance of %g\n",filename.c_str(),length,
         tmpv.getVectorMean(),tmpv.getVariance());
}

double&
Vec::operator[](const int index)
{
  return data[index];
}

double&
Vec::operator()(const int index)
{
  return data[index];
}

void
Vec::SetValue(const int index,double val)
{
  data[index]=val;
}

int
Vec::size() const
{
  return length;
}

double
Vec::max() const
{
  double max=0;
  if (length)
    max=data[0];
  for (int i=1; i<length; i++)
    if (data[i]>max)
      max=data[i];
  return max;
}

Vec &
Vec::operator+=(Vec &v)
{
  for (int i=0; i<length; i++)
    data[i]+=v[i];
  return *this;
}


Vec &
Vec::operator-=(Vec &v)
{
  for (int i=0; i<length; i++)
    data[i]-=v[i];
  return *this;
}


Vec &
Vec::operator*=(Vec &v)
{
  for (int i=0; i<length; i++)
    data[i]*=v[i];
  return *this;
}


Vec &
Vec::operator/=(Vec &v)
{
  for (int i=0; i<length; i++)
    data[i]/=v[i];
  return *this;
}


Vec &
Vec::operator+=(double val)
{
  for (int i=0; i<length; i++)
    data[i]+=val;
  return *this;
}

Vec &
Vec::operator-=(double val)
{
  for (int i=0; i<length; i++)
    data[i]-=val;
  return *this;
}

Vec &
Vec::operator*=(double val)
{
  for (int i=0; i<length; i++)
    data[i]*=val;
  return *this;
}

Vec &
Vec::operator/=(double val)
{
  for (int i=0; i<length; i++)
    data[i]/=val;
  return *this;
}

string
Vec::GetFileName() const
{
  return filename;
}

void
Vec::SetFileName(const string &fname)
{
  filename=fname;
}

int
Vec::ReadFile(const string &fname)
{
  filename=fname;
  vector<VBFF> ftypes=EligibleFileTypes(fname,1);
  if (ftypes.size()==0)
    return 101;
  // FIXME on error we could be nice and try multiple types
  fileformat=ftypes[0];
  if (!fileformat.read_1D)
    return 102;
  int err=fileformat.read_1D(this);
  return err;
}

int
Vec::WriteFile(const string fname)
{
  VBFF original;
  // save the original format, then null it
  original=fileformat;
  fileformat.init();
  if (fname.size()) filename=fname;
  if (!fileformat.write_1D)   // should always be true
    fileformat=findFileFormat(filename,1);
  // if not, try original file's format
  if (!fileformat.write_1D)
    fileformat=original;
  // if not, try cub1
  if (!fileformat.write_1D)
    fileformat=findFileFormat("ref1");
  // if not (should never happen), bail
  if (!fileformat.write_1D)
    return 200;
  int err=fileformat.write_1D(this);
  return err;
}

void
Vec::AddHeader(const string &str)
{
  header.push_back((string)str);
}
