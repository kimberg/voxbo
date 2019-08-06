
// matrixutils.cpp
// various matrix manipulation primitives
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

using namespace std;

#include "vbcrunch.h"

// dan_linspace()
// seems to work

RowVector dan_linspace(double first, double last, int points) {
  double increment;
  RowVector result(points);
  int i;

  increment = (last - first) / (points - 1);
  for (i = 0; i < points - 1; i++) result(i) = first + (i * increment);
  result(points - 1) = last;
  return result;
}

// copysegment()
// seems to work
// this function is probably just a glorified (and inefficient) set_col()
// if it happens to be along column boundaries.
// this is probably the case, so i should check and rewrite where necessary

void copysegment(Matrix &to, const Matrix &from, int count, int offset) {
  int to_mi, to_ni, from_mi, from_ni, i;

  to_mi = offset % to.rows();
  to_ni = (int)offset / to.rows();
  from_mi = from_ni = 0;
  for (i = 0; i < count; i++) {
    to(to_mi++, to_ni) = from(from_mi++, from_ni);
    if (to_mi > to.rows() - 1) {
      to_mi = 0;
      to_ni++;
    }
    if (from_mi > from.rows() - 1) {
      from_mi = 0;
      from_ni++;
    }
  }
}

// diff()
// seems to work
Matrix diff(const Matrix &m) {
  Matrix tmpm(m.rows() - 1, m.cols());
  int i, j;

  for (i = 0; i < m.cols(); i++)
    for (j = 0; j < m.rows() - 1; j++) tmpm(j, i) = (m(j + 1, i)) - (m(j, i));
  return tmpm;
}

// dan_reshape44()
// seems to work
Matrix dan_reshape44(Matrix m, int col) {
  Matrix ret(4, 4);

  ret(0, 0) = m(0, col);
  ret(1, 0) = m(1, col);
  ret(2, 0) = m(2, col);
  ret(3, 0) = m(3, col);
  ret(0, 1) = m(4, col);
  ret(1, 1) = m(5, col);
  ret(2, 1) = m(6, col);
  ret(3, 1) = m(7, col);
  ret(0, 2) = m(8, col);
  ret(1, 2) = m(9, col);
  ret(2, 2) = m(10, col);
  ret(3, 2) = m(11, col);
  ret(0, 3) = m(12, col);
  ret(1, 3) = m(13, col);
  ret(2, 3) = m(14, col);
  ret(3, 3) = m(15, col);
  return ret;
}

void ident(Matrix &mat) {
  mat.fill(0.0);
  for (int i = 0; i < MIN(mat.rows(), mat.cols()); i++) mat(i, i) = 1.0;
}

// backslash()

Matrix backslash(const Matrix &a, const Matrix &b) {
  int info;
  if (a.rows() == a.columns()) {
    double rcond = 0.0;
    Matrix result = a.solve(b, info, rcond);
    if (info != -2) return result;
  }

  int rank;
  return a.lssolve(b, info, rank);
}

// Matrix
// m_backslash(const Matrix A,const Matrix &bb)
// {
//   Matrix tmp,res,QR;
//   RowVector diag,b,x;

//   if (A.rows() == A.cols()) {
//     return (A.inverse()*bb);
//   }
//   else {
//     b=bb.column(0);
//     if (bb->n > 1)
//       cerr << "argh!" << endl
//     QR = m_get(A->m,A->n);
//     QR = m_copy(A,QR);
//     diag = v_get(A->n);
//     QRfactor(QR,diag);
//     /* set values of b here */
//     x = QRsolve(QR,diag,b,NULL);
//     tmp=m_get(x->dim,1);
//     set_col(tmp,0,x);
//     return tmp;
//   }
// }

// vectorize()

RowVector vectorize(const Matrix &in) {
  RowVector res(in.rows() * in.cols());
  int i, j, k;

  k = 0;
  for (i = 0; i < in.cols(); i++)
    for (j = 0; j < in.rows(); j++) res(k++) = in(j, i);
  return res;
}

double *arrayize(const Matrix &mat) {
  double *tmp;
  int i, j, k;

  tmp = new double[mat.rows() * mat.cols()];
  // tmp = (double *)malloc(sizeof(double) * mat.rows() * mat.cols());
  if (!tmp) {
    cerr << "vbcrunch failed to allocate space for an array" << endl;
    exit(5);
  }
  k = 0;
  for (i = 0; i < mat.cols(); i++)
    for (j = 0; j < mat.rows(); j++) tmp[k++] = mat(j, i);
  return tmp;
}

// arrayize() for rowvectors

double *arrayize(const RowVector &vec) {
  double *tmp;

  tmp = new double[vec.length()];
  // tmp = (double *)malloc(sizeof(double) * vec.length());
  if (!tmp) {
    cerr << "vbcrunch failed to allocate space for an array" << endl;
    exit(5);
  }
  for (int i = 0; i < vec.length(); i++) tmp[i] = vec(i);
  return tmp;
}
