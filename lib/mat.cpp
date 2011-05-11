
// mat.cpp
// VoxBo Matrix classes
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



//////////////////
// NOTES
//////////////////

// voxbo matrix files are always stored in row-major format -- i.e.,
// the first n values constitute the first row.  they can be single
// precision or double precision floating point on disk, but as of
// this writing they're always double internally,and always written
// out as doubles.

using namespace std;

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <string>
#include <assert.h>
#include "vbutil.h"
#include "vbio.h"

#include "vb_vector.h"

// constructors, etc.

VBMatrix::VBMatrix()
{
  init();
}

// copy constructor

VBMatrix::VBMatrix(const VBMatrix &mat)
{
  init();
  *this=mat;
}

VBMatrix::VBMatrix(int xrows,int xcols)
{
  init();
  rows=xrows;
  cols=xcols;
  rowdata=new double[rows*cols];
  assert(rowdata);
  memset(rowdata,0,rows*cols*sizeof(double));
  mview=gsl_matrix_view_array(rowdata,m,n);
}

// constructor with just a filename, optional row/column ranges

VBMatrix::VBMatrix(const string &fname,int r1,int r2,int c1,int c2)
{
  init();
  ReadFile(fname,r1,r2,c1,c2);
}

VBMatrix::VBMatrix(VB_Vector& vec)
{
  init(vec.getLength(),1);
  SetColumn(0,vec);
}

VBMatrix::~VBMatrix()
{
  clear();
}

void
VBMatrix::init()
{
  filebyteorder=ENDIAN_BIG;
  rowdata=(double *)NULL;
  m=0;
  n=0;
  matfile=(FILE *)NULL;
  transposed=0;
}

void
VBMatrix::init(int xrows,int xcols)
{
  filebyteorder=ENDIAN_BIG;
  rows=xrows;
  cols=xcols;
  rowdata=new double[rows*cols];
  assert(rowdata);
  memset(rowdata,0,rows*cols*sizeof(double));
  mview=gsl_matrix_view_array(rowdata,rows,cols);
  matfile=(FILE *)NULL;
  transposed=0;
}

void
VBMatrix::resize(int xrows,int xcols)
{
  if (rowdata)
    delete [] rowdata;
  rowdata=new double[xrows*xcols];
  assert(rowdata);
  rows=xrows;
  cols=xcols;
  mview=gsl_matrix_view_array(rowdata,rows,cols);
}

bool
VBMatrix::headerValid()
{
  if (m>0 && n>0)
    return 1;
  return 0;
}

// VBMatrix::operator bool() const
// {
//   if (rowdata)
//     return 1;
//   return 0;
// }

bool
VBMatrix::valid()
{
  if (rowdata)
    return 1;
  return 0;
}

VBMatrix &
VBMatrix::operator=(const VBMatrix &mat)
{
  if (rowdata) {
    delete [] rowdata;
    rowdata=(double *)NULL;
  }
  init();
  offset=mat.offset;
  header=mat.header;
  filename=mat.filename;
  rows=mat.rows;
  cols=mat.cols;
  filebyteorder=mat.filebyteorder;
  transposed=mat.transposed;
  if (mat.rowdata) {
    rowdata=new double[rows*cols];
    assert(rowdata);
    mview=gsl_matrix_view_array(rowdata,rows,cols);
    memcpy(rowdata,mat.rowdata,sizeof(double)*rows*cols);
  }
  return *this;
}

VBMatrix &
VBMatrix::operator=(gsl_matrix *mat)
{
  if (rowdata)
    delete [] rowdata;
  rows=mat->size1;
  cols=mat->size2;
  rowdata=new double[sizeof(double)*rows*cols];
  assert(rowdata);
  mview=gsl_matrix_view_array(rowdata,rows,cols);
  gsl_matrix_memcpy(&mview.matrix,mat);
  return *this;
}

void
VBMatrix::float2double()
{
  if (datatype!=vb_float)
    return;
  double *newrowdata=new double[m*n];
  assert (newrowdata);
  float *fp=(float *)rowdata;
  double *dp=(double *)newrowdata;
  for (size_t i=0; i<m; i++) {
    for (size_t j=0; j<n; j++) {
      *dp++=*fp++;
    }
  }
  delete [] rowdata;
  rowdata=newrowdata;
  mview=gsl_matrix_view_array(rowdata,rows,cols);
  datatype=vb_double;
  datasize=sizeof(double);
}

void
VBMatrix::clear()
{
  if (matfile)
    fclose(matfile);
  if (rowdata)
    delete [] rowdata;
  matfile=(FILE *)NULL;
  rowdata=(double *)NULL;
  m=n=0;
  header.clear();
  init();
}

void
VBMatrix::printinfo()
{
  cout << format("[I] Matrix %s, %dx%d, ")%
    (filename.size() ? filename.c_str() : "<anon>")%
    rows%cols;
  cout << format("type double(%d)\n")%sizeof(double);
}

// printCorrelations() assumes canonical g matrix configuration:
// covariates are in columns, names are in the header

void
VBMatrix::printColumnCorrelations()
{
  vector<string> cvnames;
  tokenlist args;
  for (size_t i=0; i<header.size(); i++) {
    args.ParseLine(header[i]);
    if (args[0]=="Parameter:" && args.size()>=4)
      cvnames.push_back(args.Tail(3));
  }
  if (cvnames.size()!=cols)
    printf("[I] ignoring parameter names\n");
  for (uint32 i=0; i<cols; i++) {
    for (uint32 j=0; j<cols; j++) {
    printf("[I] correlation between %s and %s: %g\n",
           cvnames[i].c_str(),
           cvnames[j].c_str(),
           correlation(GetColumn(i),GetColumn(j)));
    }
  }
}

void
VBMatrix::print()
{
  printinfo();
  for (uint32 i=0; i<m; i++)
    printrow(i);
}

void
VBMatrix::printrow(int row)
{
  if (!rowdata) {
    printf("<no data>\n");
    return;
  }
  printf("    %03d: ",row);
  int ind=row*cols;
  for (uint32 i=0; i<cols; i++) {
    printf("% 9.5f ",rowdata[ind++]);
    fflush(stdout);
  }
  printf("\n");
}

void
VBMatrix::zero()
{
  if (!rowdata)
    return;
  gsl_matrix_set_zero(&mview.matrix);
}

void
VBMatrix::ident()
{
  if (!rowdata)
    return;
  gsl_matrix_set_identity(&mview.matrix);
}

void
VBMatrix::random()
{
  if (!rowdata)
    return;
  for (uint32 i=0; i<rows; i++)
    for (uint32 j=0; j<cols; j++)
      gsl_matrix_set(&mview.matrix,i,j,VBRandom()/1000.0);
}

int
VBMatrix::set(uint32 r,uint32 c,double val)
{
  if (r>rows-1||c>cols-1||!(valid()))
    return 101;
  gsl_matrix_set(&mview.matrix,r,c,val);
  return 0;
}

double
VBMatrix::operator()(uint32 r,uint32 c)
{
  if (r>m-1 || c>n-1)
    return 0.0;  // shouldn't happen
  return *(rowdata+(n*r)+c);
}

void
VBMatrix::SetRow(uint32 row,const VB_Vector &vec)
{
  gsl_matrix_set_row(&mview.matrix,row,vec.theVector);
}

void
VBMatrix::SetColumn(uint32 col,const VB_Vector &vec)
{
  gsl_matrix_set_col(&mview.matrix,col,vec.theVector);
}

VB_Vector
VBMatrix::GetRow(uint32 row)
{
  VB_Vector myrow(gsl_matrix_row(&mview.matrix,row).vector);
  return myrow;
}

VB_Vector
VBMatrix::GetColumn(uint32 col)
{
  VB_Vector mycol(gsl_matrix_column(&mview.matrix,col).vector);
  return mycol;
}

void
VBMatrix::DeleteColumn(uint32 col)
{
  if (!valid())
    return;
  if (col>cols-1)
    return;
  VBMatrix tmp=*this;
  resize(rows,cols-1);
  for (uint32 i=0; i<col; i++)
    SetColumn(i,tmp.GetColumn(i));
  for (uint32 i=col; i<cols; i++)
    SetColumn(i,tmp.GetColumn(i+1));
}

string
VBMatrix::GetFileName() const
{
  return filename;
}

void
VBMatrix::AddHeader(const string &str)
{
  header.push_back((string)str);
}

VBMatrix &
VBMatrix::operator+=(const VBMatrix &mat)
{
  gsl_matrix_add(&this->mview.matrix,&mat.mview.matrix);
  return *this;
}

VBMatrix &
VBMatrix::operator-=(const VBMatrix &mat)
{
  gsl_matrix_sub(&this->mview.matrix,&mat.mview.matrix);
  return *this;
}

VBMatrix &
VBMatrix::operator*=(VBMatrix &mat)
{
  int r=(this->transposed ? this->cols : this->rows);
  int c=(mat.transposed ? mat.rows : mat.cols);
  VBMatrix ret(r,c);
  CBLAS_TRANSPOSE_t t1=(this->transposed ? CblasTrans : CblasNoTrans);
  CBLAS_TRANSPOSE_t t2=(mat.transposed ? CblasTrans : CblasNoTrans);
  gsl_blas_dgemm(t1,t2,1.0,&this->mview.matrix,&mat.mview.matrix,0.0,&ret.mview.matrix);
  *this=ret;
  return *this;
}

// premultiply operator

VBMatrix &
VBMatrix::operator^=(VBMatrix &mat)
{
  int r=(mat.transposed ? mat.cols : mat.rows);
  int c=(this->transposed ? this->rows : this->cols);
  VBMatrix ret(r,c);
  CBLAS_TRANSPOSE_t t1=(this->transposed ? CblasTrans : CblasNoTrans);
  CBLAS_TRANSPOSE_t t2=(mat.transposed ? CblasTrans : CblasNoTrans);
  gsl_blas_dgemm(t2,t1,1.0,&mat.mview.matrix,&this->mview.matrix,0.0,&ret.mview.matrix);
  *this=ret;
  return *this;
}

VBMatrix &
VBMatrix::operator+=(const double &d)
{
  gsl_matrix_add_constant(&this->mview.matrix,d);
  return *this;
}

VBMatrix &
VBMatrix::operator-=(const double &d)
{
  gsl_matrix_add_constant(&this->mview.matrix,(double)0.0-d);
  return *this;
}

VBMatrix &
VBMatrix::operator*=(const double &d)
{
  gsl_matrix_scale(&this->mview.matrix,d);
  return *this;
}

VBMatrix &
VBMatrix::operator/=(const double &d)
{
  gsl_matrix_scale(&this->mview.matrix,(double)1.0/d);
  return *this;
}

double
VBMatrix::trace()
{
  if (m!=n) return nan("nan");
  double tr=0.0;
  for (uint32 i=0; i<m; i++)
    tr+=(*this)(i,i);
  return tr;
}

int
VBMatrix::WriteFile(const string fname)
{
  VBFF original;
  // save the original format, then null it
  original=fileformat;
  fileformat.init();
  if (fname.size()) filename=fname;
  // ReparseFileName();
  // if reparse didn't find anything, try by extension
  if (!fileformat.write_2D)
    fileformat=findFileFormat(filename,2);
  // if not, try original file's format
  if (!fileformat.write_2D)
    fileformat=original;
  // if not, try cub1
  if (!fileformat.write_2D)
    fileformat=findFileFormat("mat1");
  // if not (should never happen), bail
  if (!fileformat.write_2D)
    return 200;
  int err=fileformat.write_2D(this);
  return err;
}


int
VBMatrix::ReadFile(const string &fname,uint32 r1,uint32 rn,uint32 c1,uint32 cn)
{
  int err;
  err=ReadHeader(fname);
  if (err) return err;
  err=ReadData(filename,r1,rn,c1,cn);
  if (err) return err;
  return 0;
}

int
VBMatrix::ReadHeader(const string &fname)
{
  if (fname.size()==0)
    return 104;
  init();
  filename=fname;
  //  ReparseFileName();
  vector<VBFF> ftypes=EligibleFileTypes(fname,2);
  if (ftypes.size()==0)
    return 101;
  // FIXME on error we could be nice and try multiple types
  fileformat=ftypes[0];
  if (!fileformat.read_head_2D)
    return 102;
  int err=fileformat.read_head_2D(this);
  return err;
}

int
VBMatrix::ReadData(const string &fname,uint32 r1,uint32 rn,uint32 c1,uint32 cn)
{
  filename=fname;
  int err=0;
  if (rows==0 && cols==0) {
    if ((err=ReadHeader(fname)))
      return err;
  }
  if (!fileformat.read_data_2D)
    return 102;
  err=fileformat.read_data_2D(this,r1,rn,c1,cn);
  return err;
}


// NON-MEMBER FUNCTIONS DEALING WITH MATRICES

int
invert(const VBMatrix &src,VBMatrix &dest)
{
  if (!src.m==src.n)
    throw "invert: matrix must be square";
  gsl_matrix *tmp1=gsl_matrix_alloc(src.m,src.n);
  if (!tmp1)
    throw "invert: couldn't allocate matrix";
  gsl_matrix *tmp2=gsl_matrix_alloc(src.m,src.n);
  if (!tmp2)
    throw "invert: couldn't allocate matrix";
  gsl_permutation *p=gsl_permutation_calloc(src.m);
  if (!p)
    throw "invert: couldn't allocate matrix";
  int signum=0;
  gsl_matrix_memcpy(tmp1,&src.mview.matrix);
  gsl_linalg_LU_decomp(tmp1,p,&signum);
  if (abs(gsl_linalg_LU_det(tmp1, signum))<FLT_MIN) {
    gsl_matrix_free(tmp1);
    gsl_matrix_free(tmp2);
    gsl_permutation_free(p);
    dest.clear();
    return 1;
  }    
  gsl_linalg_LU_invert(tmp1,p,tmp2);
  gsl_matrix_free(tmp1);
  gsl_permutation_free(p);
  dest=tmp2;
  gsl_matrix_free(tmp2);
  return 0;
}

int
pinv(const VBMatrix &src,VBMatrix &dest)
{
  dest.zero();
  gsl_matrix *tmp1=gsl_matrix_calloc(src.n,src.n);
  if (!tmp1)
    throw "invert: couldn't allocate matrix";

  gsl_matrix *tmp2=gsl_matrix_calloc(src.n,src.n);
  if (!tmp2)
    throw "invert: couldn't allocate matrix";

  gsl_permutation *p=gsl_permutation_calloc(src.n);
  if (!p)
    throw "invert: couldn't allocate matrix";
  int signum=0;
  gsl_blas_dgemm(CblasTrans,CblasNoTrans,1.0,&src.mview.matrix,&src.mview.matrix,0.0,tmp1);
  gsl_linalg_LU_decomp(tmp1,p,&signum);
  if (abs(gsl_linalg_LU_det(tmp1,signum))<FLT_MIN)
    return 1;
  gsl_linalg_LU_invert(tmp1,p,tmp2);
  // realloc tmp1 to nxm
  gsl_matrix_free(tmp1);
  tmp1=gsl_matrix_calloc(src.n,src.m);
  if (!tmp1)
    throw "invert: couldn't allocate matrix";
  // tmp1=tmp2 KGt
  gsl_blas_dgemm(CblasNoTrans,CblasTrans,1.0,tmp2,&src.mview.matrix,0.0,tmp1);
  // do some housekeeping
  gsl_matrix_free(tmp2);
  gsl_permutation_free(p);
  // put the result in dest
  dest=tmp1;
  gsl_matrix_free(tmp1);
  return 0;
}

int
pca(VBMatrix &data,VB_Vector &lambdas,VBMatrix &components,VBMatrix &E)
{
  gsl_vector *work;
  int M=data.m; /* Columns... */
  int N=data.n; /* Rows... */
  int i,j;

  lambdas.resize(N);
  E.init(N,N);
  work=gsl_vector_calloc(N);
  if(!work)
    return 101;
  components=data;
  // center the data vectors
  for (uint32 i=0; i<components.n; i++) {
    VB_Vector tmp=components.GetColumn(i);
    tmp-=tmp.getVectorMean();
    components.SetColumn(i,tmp);
  }

  // first, calculate SVD
  gsl_linalg_SV_decomp(&components.mview.matrix,&E.mview.matrix,lambdas.theVector,work);

  // now scale the components so that eigenvectors are orthonormal and
  // still X=P*E^t, with E eigenvectors and P expansion coefficients

  for(i=0;i<M;i++){
    for(j=0;j<N;j++){
      gsl_matrix_set(&components.mview.matrix,i,j,
                     gsl_matrix_get(&components.mview.matrix,i,j)*
                     gsl_vector_get(lambdas.theVector,j));
    }
  }
  gsl_vector_free(work);
  return 0;
}
