
// norm_utils.cpp
// Dan Kimberg, 1998-2002

using namespace std;

#include <octave/dbleSVD.h>
#include "vbcrunch.h"

Matrix ret_m1, ret_m2;
double ret_d1;

void set_element(Matrix &m, int pos, double val);
RowVector findnonzero(const Matrix &mat);
RowVector findnonzero(const RowVector &vec);
void set_elements(Matrix &mat, RowVector &locs, RowVector &vals);
void add_elements(RowVector &vec, const RowVector &locs,
                  const ColumnVector &additions);
void add_elements(Matrix &mat, const RowVector &locs,
                  const ColumnVector &additions);

// dan_affsub1.c
// closely based on
// spm_affsub1.m 1.1 John Ashburner FIL 96/07/08
// function [alpha, beta, chi2] = spm_affsub1(REF,IMAGE,MG,MF,Hold,samp,P)

int dan_affsub1(CrunchCube *REF, CrunchCube *IMAGE, const Matrix &MG,
                const Matrix &MF, int Hold, int samp, const RowVector &P) {
  Matrix alpha(13, 13, 0.0), beta(13, 1, 0.0), dMdP(13, 13, 0.0);
  int skipx, skipy, skipz, i, j, n;
  Matrix X, Y, Mat;
  Matrix mtmp1, mtmp2, tmp, dFdM, F, dx, dy, dz;
  RowVector tP, t0, XM, YM, ZM, mask1, X1, Y1, Z1;
  double chi2;

  // Coordinates of templates
  mtmp1.resize(REF->dimx, REF->dimx);
  mtmp1.fill(0.0);
  for (i = 0; i < REF->dimx; i++) mtmp1(i, i) = i + 1;
  mtmp2.resize(REF->dimx, REF->dimy);
  mtmp2.fill(1.0);
  X = mtmp1 * mtmp2;

  // these matrices don't really have to be built again
  mtmp1.resize(REF->dimx, REF->dimy);
  mtmp1.fill(1.0);

  mtmp2.resize(REF->dimy, REF->dimy);
  mtmp2.fill(0.0);
  for (i = 0; i < REF->dimy; i++) mtmp2(i, i) = i + 1;
  Y = mtmp1 * mtmp2;

  // Sample about every samp mm
  skipx = (int)MAX(round(samp / REF->voxsize[0]), 1);
  skipy = (int)MAX(round(samp / REF->voxsize[1]), 1);
  skipz = (int)MAX(round(samp / REF->voxsize[2]), 1);

  RowVector mask0(0);
  for (i = 0; i < X.cols() * X.rows(); i++) {
    double tmp1 = get_element(X, i);
    double tmp2 = get_element(Y, i);
    if ((tmp1 / skipx) == (double)((int)(tmp1 / skipx)) &&
        (tmp2 / skipy) == (double)((int)(tmp2 / skipy)))
      mask0.resize(mask0.length() + 1, (double)i);
  }

  Mat = ((MG.inverse() * dan_matrix(P.transpose())) * MF).inverse();

  // rate of change of matrix elements with respect to parameters

  tmp = Mat;
  tmp.resize(3, 4, 0.0);
  tmp = tmp.transpose();

  t0 = vectorize(tmp);
  t0.resize(t0.length() + 1, 0.0);  // zero tacked onto the end
  for (i = 0; i < 12; i++) {
    tP = P;
    tP(i) += 0.01;
    tmp = ((MG.inverse() * dan_matrix(tP.transpose())) * MF).inverse();
    tmp.resize(3, 4, 0.0);
    tmp = tmp.transpose();
    for (j = 0; j < dMdP.rows() - 1; j++) {
      dMdP(j, i) = (get_element(tmp, j) - t0(j)) / .01;
    }
    dMdP(j, i) = (0 - t0(j)) / .01;
  }
  for (i = 0; i < dMdP.rows() - 1; i++) dMdP(i, 12) = 0;
  dMdP(i, 12) = 1;

  chi2 = 0;
  n = 0;

  // is p=1 correct, or do i want to start at 0???
  for (int p = 1; p <= REF->dimz; p += skipz) {
    XM = index(X, mask0);
    YM = index(Y, mask0);

    //% Transformed template coordinates.
    X1 = (Mat(0, 0) * XM) + (Mat(0, 1) * YM) + ((Mat(0, 2) * p) + Mat(0, 3));
    Y1 = (Mat(1, 0) * XM) + (Mat(1, 1) * YM) + ((Mat(1, 2) * p) + Mat(1, 3));
    Z1 = (Mat(2, 0) * XM) + (Mat(2, 1) * YM) + ((Mat(2, 2) * p) + Mat(2, 3));
    //% Only resample from within the volume IMAGE.
    mask1.resize(0);
    for (i = 0; i < X1.length(); i++) {
      if (X1(i) >= 1 && Y1(i) >= 1 && Z1(i) >= 1 &&
          X1(i) < (IMAGE->dimx - .01) && Y1(i) < (IMAGE->dimy - .01) &&
          Z1(i) < (IMAGE->dimz - .01)) {
        mask1.resize(mask1.length() + 1, i);
      }
    }

    // Don't waste time on an empty plane.
    if (mask1.length() > 0) {
      // Only resample from within the volume IMAGE.
      if (mask1.length() != mask0.length()) {
        X1 = index(X1, mask1);
        Y1 = index(Y1, mask1);
        Z1 = index(Z1, mask1);
        XM = index(XM, mask1);
        YM = index(YM, mask1);
      }

      dFdM.resize(mask1.length(), 13);
      dFdM.fill(0.0);

      // Sample image to normalise & get local NEGATIVE derivatives
      F = dan_sample_vol(IMAGE, X1, Y1, Z1, Hold);
      dx = (F - dan_sample_vol(IMAGE, X1 + .01, Y1, Z1, Hold)) / (.01);
      dy = (F - dan_sample_vol(IMAGE, X1, Y1 + .01, Z1, Hold)) / (.01);
      dz = (F - dan_sample_vol(IMAGE, X1, Y1, Z1 + .01, Hold)) / (.01);
      // Generate Design Matrix
      for (i = 0; i < dFdM.rows(); i++) {
        dFdM(i, 0) = XM(i) * dx(i, 0);
        dFdM(i, 1) = YM(i) * dx(i, 0);
        dFdM(i, 2) = p * dx(i, 0);
        dFdM(i, 3) = dx(i, 0);
        dFdM(i, 4) = XM(i) * dy(i, 0);
        dFdM(i, 5) = YM(i) * dy(i, 0);
        dFdM(i, 6) = p * dy(i, 0);
        dFdM(i, 7) = dy(i, 0);
        dFdM(i, 8) = XM(i) * dz(i, 0);
        dFdM(i, 9) = YM(i) * dz(i, 0);
        dFdM(i, 10) = p * dz(i, 0);
        dFdM(i, 11) = dz(i, 0);
      }

      // Sample reference image(s)
      ZM.resize(mask1.length());
      ZM.fill((double)p);

      tmp = dan_sample_vol(REF, XM, YM, ZM, Hold);
      for (i = 0; i < dFdM.rows(); i++) dFdM(i, 12) = tmp(i, 0);
      for (i = 0; i < F.rows(); i++) F(i, 0) -= tmp(i, 0) * P(12);

      for (i = 0; i < F.rows(); i++)
        for (j = 0; j < F.cols(); j++) chi2 += pow(F(i, j), 2);
      n += F.rows() * F.cols();

      //% Most of the work
      alpha = alpha + dan_atranspa(dFdM);

      beta = beta + (dFdM.transpose() * F);
      dFdM.resize(0, 0);
    }
  }

  alpha = (dMdP.transpose() * alpha) * dMdP;
  beta = dMdP.transpose() * beta;
  // chi2   = chi2/(n - 12 - size(REF,2));
  chi2 /= (double)(n - 13);

  ret_m1 = alpha;
  ret_m2 = beta;
  ret_d1 = chi2;
  return TRUE;
}

// % FORMAT nP = spm_affsub2(REF,IMAGE,MG,MF,Hold,samp,oP,free,pdesc,gorder)
// % MG        - Space of the template image(s).
// % MF        - Space of the object image.
// % samp      - Frequency (in mm) of sampling.
// % oP        - Old parameter estimates.
// % free      - Ones and zeros indicating which parameters to fit.
// % pdesc     - Description of parameters.
// % nP        - New parameter estimates.

// dan_affsub2.c
// based closely on
// spm_affsub2.m 1.4 John Ashburner FIL 96/08/21
// function P = spm_affsub2(REF,IMAGE,MG,MF,Hold,samp,P,free,pdesc,gorder)

RowVector dan_affsub2(CrunchCube *REF, CrunchCube *IMAGE, const Matrix &MG,
                      const Matrix &MF, int Hold, int samp, RowVector P,
                      const RowVector &free, const RowVector &pdesc) {
  double pchi2, bestchi2, ochi2, chi2_t;
  int iter, countdown, i, nf;
  Matrix alpha_t, beta_t, aqqq, mtmp1, mtmp2;
  RowVector bestP, vtmp, qq, qqq, bqq, pp, ppp;
  Matrix alpha(P.length(), P.length(), 0.0);
  RowVector beta(P.length(), 0.0);
  ColumnVector cvtmp;

  pchi2 = 9e99;
  iter = 1;
  countdown = 0;
  bestP = P;
  bestchi2 = 9e99;

  while (iter <= 64 && countdown < 3) {
    ochi2 = pchi2;
    pchi2 = 1;
    alpha.fill(0.0);
    beta.fill(0.0);

    //% generate alpha and beta
    //%-----------------------------------------------------------------------

    // totally unnecessary loop!
    for (int im = 1; im <= 1; im++) {
      pp = findnonzero(pdesc);
      ppp = findnonzero(pdesc.transpose() * pdesc);
      vtmp = index(P, pp);
      dan_affsub1(REF, IMAGE, MG, MF, Hold, samp, vtmp);
      alpha_t = ret_m1;
      beta_t = ret_m2;
      chi2_t = ret_d1;
      cvtmp = vectorize(beta_t).transpose() / chi2_t;
      add_elements(beta, pp, cvtmp);
      cvtmp = vectorize(alpha_t).transpose() / chi2_t;
      add_elements(alpha, ppp, cvtmp);
      pchi2 *= chi2_t;
    }

    // If \chi^2 is better than the previous best, then save the parameters
    // from the previous iteration.

    if (pchi2 < bestchi2) {
      bestchi2 = pchi2;
      bestP = P;
    }

    //% Update parameter estimates
    //%-----------------------------------------------------------------------

    // qq  = find(free);
    // qqq = find(free*free');
    // nf  = sum(free ~= 0);
    qq = findnonzero(free);
    qqq = findnonzero(free.transpose() * free);
    nf = 0;
    for (i = 0; i < free.length(); i++)
      if (free(i)) nf++;
    vtmp = index(alpha, qqq);
    aqqq = reshape(vtmp, nf, nf);
    bqq = index(beta, qq);

    // P(qq) = P(qq) + pinv(reshape(alpha(qqq), nf, nf))*beta(qq);
    ColumnVector cvtmp = pinv(aqqq) * bqq.transpose();
    add_elements(P, qq, cvtmp);

    //% Check stopping criteria. If satisfied then just do another few more
    //% iterations before stopping.
    //%-----------------------------------------------------------------------
    if ((2 * (ochi2 - pchi2) / (ochi2 + pchi2)) < 0.002)
      countdown = countdown + 1;

    iter++;
  }
  return bestP;
}

// spm_atranspa.c 1.1 (c) John Ashburner 95/11/29";

Matrix dan_atranspa(const Matrix &A) {
  Matrix C(A.cols(), A.cols(), 0.0);
  int i, j1, j2;
  double c;
  int n = A.cols();

  /* Generate half of symmetric matrix C */
  for (j1 = 0; j1 < n; j1++) {
    for (j2 = 0; j2 <= j1; j2++) {
      c = 0.0;

      /* Work down columns in inner loop
         to reduce paging */
      for (i = 0; i < A.rows(); i++) c += A(i, j1) * A(i, j2);
      set_element(C, j1 * n + j2, c);
    }
  }

  /* Generate other half */
  for (j1 = 0; j1 < A.cols(); j1++)
    for (j2 = 0; j2 < j1; j2++)
      set_element(C, j2 * n + j1, get_element(C, j1 * n + j2));

  return C;
}

// dan_dctmtx.c
// create basis functions for discrete cosine transform
// see Fundementals of Digital Image Processing, A. Jain, 1989, 150-154
// double-check operator precedence throughout

Matrix dan_dctmtx(int N, int K) {
  int k, i;
  Matrix C(N, K, 0.0);

  for (i = 0; i < N; i++) {
    C(i, 0) = 1.0 / sqrt((double)N);
  }
  for (k = 1; k < K; k++) {
    for (i = 0; i < N; i++) {
      C(i, k) = sqrt(2.0 / N) * cos(PI * (2.0 * i + 1.0) * (k) / (2.0 * N));
    }
  }
  return C;
}

Matrix dan_dctmtx(int N, int K, const RowVector &n) {
  int k, i;
  Matrix C(n.length(), K, 0.0);

  for (i = 0; i < n.length(); i++) {
    C(i, 0) = 1.0 / sqrt((double)N);
  }
  for (k = 1; k < K; k++) {
    for (i = 0; i < n.length(); i++) {
      C(i, k) = sqrt(2.0 / N) * cos(PI * (2.0 * n(i) + 1.0) * (k) / (2.0 * N));
    }
  }
  return C;
}

Matrix dan_dctmtxdiff(int N, int K) {
  int k, i;
  Matrix C(N, K, 0.0);

  for (k = 1; k < K; k++) {
    for (i = 0; i < N; i++)
      C(i, k) = -(pow(2.0, 0.5)) * pow(1.0 / N, 0.5) *
                sin(0.5 * PI * (2.0 * i * (k + 1) - 2 * i + k) / N) * PI *
                (double)k / N;
  }
  return C;
}

Matrix dan_dctmtxdiff(int N, int K, const RowVector &n) {
  int k, i;
  Matrix C(n.length(), K, 0.0);

  for (k = 1; k < K; k++) {
    for (i = 0; i < n.length(); i++)
      C(i, k) = -(pow(2.0, 0.5)) * pow((1.0 / N), 0.5) *
                sin(0.5 * PI * (2.0 * n(i) * k - 1 * n(i) + k) / N) * PI * (k) /
                N;
  }
  return C;
}

// dan_snbasis.c - completely based on
// spm_snbasis_map.m 1.5 John Ashburner FIL 96/08/21

int dan_snbasis_map(CrunchCube *REF, CrunchCube *IMAGE, Matrix &Affine, int k1,
                    int k2, int k3, int iter, int fwhm, double sd1) {
  Matrix IC0, IC1, IC2;
  Matrix basX, basY, basZ, dbasX, dbasY, dbasZ, alpha, beta;
  Matrix dd1, dd2, dd3, T, Transform;
  int i, s1, s2;
  double remainder, var, stabilise;

  //% Number of basis functions for x, y & z
  k1 = MAX(k1, 1);
  k1 = MIN(k1, (int)REF->dimx);
  k2 = MAX(k2, 1);
  k2 = MIN(k2, (int)REF->dimy);
  k3 = MAX(k3, 1);
  k3 = MIN(k3, (int)REF->dimz);

  //% Scaling is to improve stability.
  //%-----------------------------------------------------------------------
  stabilise = 64.0;
  basX = dan_dctmtx(REF->dimx, k1) * stabilise;
  basY = dan_dctmtx(REF->dimy, k2) * stabilise;
  basZ = dan_dctmtx(REF->dimz, k3) * stabilise;

  dbasX = dan_dctmtxdiff(REF->dimx, k1) * stabilise;
  dbasY = dan_dctmtxdiff(REF->dimy, k2) * stabilise;
  dbasZ = dan_dctmtxdiff(REF->dimz, k3) * stabilise;

  dd1.resize(k1, 1);
  for (i = 0; i < k1; i++) dd1(i, 0) = pow(((PI * i) / REF->dimx), 2);

  dd2.resize(k2, 1);
  for (i = 0; i < k2; i++) dd2(i, 0) = pow(((PI * i) / REF->dimy), 2);

  dd3.resize(k3, 1);
  for (i = 0; i < k3; i++) dd3(i, 0) = pow(((PI * i) / REF->dimz), 2);

  IC0 = kron(kron(ones(k3, 1), ones(k2, 1)), dd1) +
        kron(kron(ones(k3, 1), dd2), ones(k1, 1)) +
        kron(kron(dd3, ones(k2, 1)), ones(k1, 1));

  IC0 = IC0 / (3.0 * ((double)(REF->dimx * REF->dimy * REF->dimz) /
                      (double)((k1 * k2 * k3) - 1)));
  IC0 = IC0 * (pow(sd1, -2) * pow(stabilise, 6));
  IC1 = IC0;
  IC1 = IC1.stack(IC0);
  IC1 = IC1.stack(IC0);
  IC1 = IC1.stack(zeros(4, 1));

  IC2.resize(IC1.rows() * IC1.cols(), IC1.rows() * IC1.cols());
  IC2.fill(0.0);
  for (i = 0; i < (IC1.rows() * IC1.cols()); i++) {
    IC2(i, i) = get_element(IC1, i);
  }

  // IC2 seems to be identical to matlab's output
  // Generate starting estimates.

  s1 = 3 * k1 * k2 * k3;
  s2 = s1 + 4;
  T.resize(s2, 1);
  T.fill(0.0);
  T(s1, 0) = 1;

  for (i = 0; i < iter; i++) {
    dan_brainwarp(REF, IMAGE, Affine, basX, basY, basZ, dbasX, dbasY, dbasZ, T,
                  fwhm);
    alpha = ret_m1;
    beta = ret_m2;
    var = ret_d1;
    if (i > -1) {
      T = backslash(alpha + (IC2 * var), (alpha * T) + beta);
      // T seems to be almost but not quite identical to matlab's output
    } else
      T = T + backslash(alpha, beta);
  }

  //% Dimensions and values of the 3D-DCT
  //%-----------------------------------------------------------------------
  // Transform = reshape(T(1:s1),prod(k),3)*stabilise.^3;
  // T.resize(s1,1);

  Transform = reshape(T, k1 * k2 * k3, 3) * pow(stabilise, 3);

  //  Dims = [REF(1:3,1)'; k];
  Matrix Dims(2, 3, 0.0);
  Dims(0, 0) = REF->dimx;
  Dims(0, 1) = REF->dimy;
  Dims(0, 2) = REF->dimz;
  Dims(1, 0) = k1;
  Dims(1, 1) = k2;
  Dims(1, 2) = k3;

  // Scaling for each template image.

  remainder = get_element(T, s1);  // formerly T((1:4:4) + s1);

  ret_m1 = Transform;
  ret_m2 = Dims;
  ret_d1 = remainder;
  return TRUE;
}

// does this really compute the kronecker tensor product?
Matrix kron(const Matrix &A, const Matrix &B) {
  Matrix C(A.rows() * B.rows(), A.cols() * B.cols());
  int i, j, k, l;

  for (i = 0; i < A.rows(); i++) {
    for (j = 0; j < A.cols(); j++) {
      for (k = 0; k < B.rows(); k++) {
        for (l = 0; l < B.cols(); l++) {
          C(i * B.rows() + k, j * B.cols() + l) = A(i, j) * B(k, l);
        }
      }
    }
  }
  return C;
}

RowVector dan_span(double from, double to, double increment) {
  int size = ((int)((to - from) / increment)) + 1;
  if (size < 0) size = 0;
  RowVector tmp(size);

  for (int i = 0; i < tmp.length(); i++) tmp(i) = from + (i * increment);
  return (tmp);
}

// does this work??
double get_element(const Matrix &m, int pos) {
  return (m(pos % m.rows(), (int)pos / m.rows()));
}

void set_element(Matrix &m, int pos, double val) {
  m(pos % m.rows(), (int)pos / m.rows()) = val;
}

Matrix reshape(const Matrix &mat, int m, int n) {
  Matrix mtmp(m, n, 0.0);
  int i, j;
  int k, l;

  k = l = 0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++) {
      mtmp(j, i) = mat(k, l);
      k++;
      if (k >= mat.rows()) {
        k = 0;
        l++;
        if (l >= mat.cols()) {
          cerr << "reshape(Matrix) - ran out of stuff\n";
          return mtmp;
        }
      }
    }
  }
  return mtmp;
}

Matrix reshape(const ColumnVector &vec, int m, int n) {
  Matrix mtmp(m, n, 0.0);
  int i, j;
  int k;

  k = 0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++) {
      if (k >= vec.length()) {
        cerr << "error in cvec reshape() - ran out of vector stuff\n";
        return mtmp;
      } else {
        mtmp(j, i) = vec(k);
        k++;
      }
    }
  }
  return mtmp;
}

Matrix reshape(const RowVector &vec, int m, int n) {
  Matrix mtmp(m, n, 0.0);
  int i, j;
  int k;

  k = 0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++) {
      if (k >= vec.length()) {
        cerr << "error in rvec reshape() - ran out of vector stuff\n";
        return mtmp;
      } else {
        mtmp(j, i) = vec(k);
        k++;
      }
    }
  }
  return mtmp;
}

RowVector index(const Matrix &mat, const RowVector &vec) {
  RowVector ret(vec.length());
  int i;

  for (i = 0; i < vec.length(); i++) ret(i) = get_element(mat, (int)vec(i));
  return ret;
}

RowVector index(const RowVector &v1, const RowVector &vec) {
  RowVector ret(vec.length());

  for (int i = 0; i < vec.length(); i++) ret(i) = v1((int)vec(i));
  return ret;
}

RowVector findnonzero(const Matrix &mat) {
  RowVector ret(0);
  int k = 0;
  for (int i = 0; i < mat.cols(); i++) {
    for (int j = 0; j < mat.rows(); j++) {
      if (mat(j, i)) ret.resize(ret.length() + 1, (double)k);
      k++;
    }
  }
  return ret;
}

RowVector findnonzero(const RowVector &vec) {
  RowVector ret(0);
  int k = 0;
  for (int i = 0; i < vec.length(); i++) {
    if (vec(i)) ret.resize(ret.length() + 1, (double)k);
    k++;
  }
  return ret;
}

void set_elements(Matrix &mat, RowVector &locs, RowVector &vals) {
  for (int i = 0; i < locs.length(); i++) {
    set_element(mat, (int)locs(i), vals(i));
  }
}

void add_elements(RowVector &vec, const RowVector &locs,
                  const ColumnVector &additions) {
  int k = 0, ind;
  for (int i = 0; i < locs.length(); i++) {
    ind = (int)locs(i);
    vec(ind) += additions(k++);
  }
}

void add_elements(Matrix &mat, const RowVector &locs,
                  const ColumnVector &additions) {
  int k = 0, ind;
  for (int i = 0; i < locs.length(); i++) {
    ind = (int)locs(i);
    set_element(mat, ind, get_element(mat, ind) + additions(k++));
  }
}

Matrix pinv(const Matrix &A) {
  Matrix Ux;
  int maxnm, m, n, i, j, r;
  double tol;

  m = A.rows();
  n = A.cols();

  if (m > n)
    maxnm = (int)m;
  else
    maxnm = (int)n;

  Matrix AP(n, m, 0.0);

  SVD decomp(A);
  Matrix U = decomp.left_singular_matrix();
  Matrix V = decomp.right_singular_matrix();
  DiagMatrix S = decomp.singular_values();

  tol = maxnm * (double)S(0, 0) * DBL_MIN;
  r = 0;
  for (i = 0; i < S.length(); i++)
    if (S(i, i) > tol) r++;
  if (r) {
    Matrix Sm(r, r, 0.0);
    for (j = 0; j < r; j++) Sm(j, j) = 1 / (double)S(j, j);
    V.resize(V.rows(), r);
    U.resize(U.rows(), r);
    Ux = U.transpose();
    AP = (V * Sm) * Ux;
  } else {
    Matrix AP(n, m, 0.0);
  }
  return AP;
}

Matrix dan_sample_vol(CrunchCube *map, const RowVector &x, const RowVector &y,
                      const RowVector &z, int hold) {
  Matrix xx(x.length(), 1), yy(x.length(), 1), zz(x.length(), 1);
  xx.insert(x.transpose(), 0, 0);
  yy.insert(y.transpose(), 0, 0);
  zz.insert(z.transpose(), 0, 0);
  return dan_sample_vol(map, xx, yy, zz, hold);
}

Matrix dan_sample_vol(CrunchCube *map, const Matrix &x, const Matrix &y,
                      const Matrix &z, int hold) {
  int m, n, k, dimx, dimy, dimz;
  double background = 0.0;
  RowVector xv, yv, zv;

  dimx = map->dimx;
  dimy = map->dimy;
  dimz = map->dimz;

  m = x.rows();
  n = x.cols();

  hold = abs(hold);

  RowVector img(m * n, 0.0);

  if (hold > 127 || hold == 2) cerr << "Bad hold value.";

  xv = vectorize(x);
  yv = vectorize(y);
  zv = vectorize(z);

  double scale = map->scl_slope;
  if (scale == 0.0) scale = 1.0;

  if (map->datatype == vb_short && hold == 1) {
    resample_1(m * n, (short *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
               background, scale);
  } else if (map->datatype == vb_long && hold == 1) {
    resample_1(m * n, (int *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
               background, scale);
  } else if (map->datatype == vb_byte && hold == 1) {
    resample_1(m * n, (unsigned char *)map->data, img, xv, yv, zv, dimx, dimy,
               dimz, background, scale);
  } else if (map->datatype == vb_float && hold == 1) {
    resample_1(m * n, (float *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
               background, scale);
  } else if (map->datatype == vb_double && hold == 1) {
    resample_1(m * n, (double *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
               background, scale);
  }

  else if (map->datatype == vb_short && hold > 2) {
    resample_sinc(m * n, (short *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
                  hold, background, scale);
  } else if (map->datatype == vb_long && hold > 2) {
    resample_sinc(m * n, (int *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
                  hold, background, scale);
  } else if (map->datatype == vb_byte && hold > 2) {
    resample_sinc(m * n, (unsigned char *)map->data, img, xv, yv, zv, dimx,
                  dimy, dimz, hold, background, scale);
  } else if (map->datatype == vb_float && hold > 2) {
    resample_sinc(m * n, (float *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
                  hold, background, scale);
  } else if (map->datatype == vb_double && hold > 2) {
    resample_sinc(m * n, (double *)map->data, img, xv, yv, zv, dimx, dimy, dimz,
                  hold, background, scale);
  }

  else {
    cerr << "in dan_sample_vol() - unrecognized type " << map->datatype << endl;
  }

  Matrix Y(m, n);

  k = 0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) Y(j, i) = img(k++);
  return Y;
}

// sinc resampling

template <class T>
void resample_sinc(int m, T *vol, RowVector &out, const RowVector &x,
                   const RowVector &y, const RowVector &z, int dimx, int dimy,
                   int dimz, int nn, double background, double scale) {
  int i, dim1xdim2 = dimx * dimy;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    if (z(i) >= 1 - TINY && z(i) <= dimz + TINY && y(i) >= 1 - TINY &&
        y(i) <= dimy + TINY && x(i) >= 1 - TINY && x(i) <= dimx + TINY) {
      T *dp1;
      double dat = 0.0, *tp1, *tp1end, *tp2end, *tp3end;

      make_lookup(x(i), nn, dimx, &dx1, tablex, &tp3end);
      make_lookup(y(i), nn, dimy, &dy1, tabley, &tp2end);
      make_lookup(z(i), nn, dimz, &dz1, tablez, &tp1end);

      tp1 = tablez;
      dy1 *= dimx;
      dp1 = vol + dim1xdim2 * dz1;

      while (tp1 <= tp1end) {
        T *dp2 = dp1 + dy1;
        double dat2 = 0.0, *tp2 = tabley;
        while (tp2 <= tp2end) {
          register double dat3 = 0.0, *tp3 = tablex;
          register T *dp3 = dp2 + dx1;
          while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
          dat2 += dat3 * *(tp2++);
          dp2 += dimx;
        }
        dat += dat2 * *(tp1++);
        dp1 += dim1xdim2;
      }
      out(i) = scale * dat;
    } else
      out(i) = background;
  }
}

template <class T>
void resample_1(int m, T *vol, RowVector &out, const RowVector &x,
                const RowVector &y, const RowVector &z, int dimx, int dimy,
                int dimz, double background, double scale) {
  int i, dim1xdim2 = dimx * dimy;

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    double xi, yi, zi;
    xi = x(i);
    yi = y(i);
    zi = z(i);

    if (zi >= 1 - TINY && zi <= dimz + TINY && yi >= 1 - TINY &&
        yi <= dimy + TINY && xi >= 1 - TINY && xi <= dimx + TINY) {
      double k111, k112, k121, k122, k211, k212, k221, k222;
      double dx1, dx2, dy1, dy2, dz1, dz2;
      int off1, off2, offx, offy, offz, xcoord, ycoord, zcoord;

      xcoord = (int)floor(xi);
      dx1 = xi - xcoord;
      dx2 = 1.0 - dx1;
      ycoord = (int)floor(yi);
      dy1 = yi - ycoord;
      dy2 = 1.0 - dy1;
      zcoord = (int)floor(zi);
      dz1 = zi - zcoord;
      dz2 = 1.0 - dz1;

      xcoord = (xcoord < 1) ? ((offx = 0), 1)
                            : ((offx = (xcoord >= dimx) ? 0 : 1), xcoord);
      ycoord = (ycoord < 1) ? ((offy = 0), 1)
                            : ((offy = (ycoord >= dimy) ? 0 : dimx), ycoord);
      zcoord = (zcoord < 1)
                   ? ((offz = 0), 1)
                   : ((offz = (zcoord >= dimz) ? 0 : dim1xdim2), zcoord);

      off1 = xcoord + dimx * (ycoord + dimy * zcoord);
      k222 = vol[off1];
      k122 = vol[off1 + offx];
      off2 = off1 + offy;
      k212 = vol[off2];
      k112 = vol[off2 + offx];
      off1 += offz;
      k221 = vol[off1];
      k121 = vol[off1 + offx];
      off2 = off1 + offy;
      k211 = vol[off2];
      k111 = vol[off2 + offx];

      /* resampled pixel value (trilinear interpolation) */
      out(i) =
          scale *
          (((k222 * dx2 + k122 * dx1) * dy2 + (k212 * dx2 + k112 * dx1) * dy1) *
               dz2 +
           ((k221 * dx2 + k121 * dx1) * dy2 + (k211 * dx2 + k111 * dx1) * dy1) *
               dz1);
    } else
      out(i) = background;
  }
}

void resample_short_sinc(int m, short *vol, RowVector &out, const RowVector &x,
                         const RowVector &y, const RowVector &z, int dimx,
                         int dimy, int dimz, int nn, double background,
                         double scale) {
  int i, dim1xdim2 = dimx * dimy;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    if (z(i) >= 1 - TINY && z(i) <= dimz + TINY && y(i) >= 1 - TINY &&
        y(i) <= dimy + TINY && x(i) >= 1 - TINY && x(i) <= dimx + TINY) {
      short *dp1;
      double dat = 0.0, *tp1, *tp1end, *tp2end, *tp3end;

      make_lookup(x(i), nn, dimx, &dx1, tablex, &tp3end);
      make_lookup(y(i), nn, dimy, &dy1, tabley, &tp2end);
      make_lookup(z(i), nn, dimz, &dz1, tablez, &tp1end);

      tp1 = tablez;
      dy1 *= dimx;
      dp1 = vol + dim1xdim2 * dz1;

      while (tp1 <= tp1end) {
        short *dp2 = dp1 + dy1;
        double dat2 = 0.0, *tp2 = tabley;
        while (tp2 <= tp2end) {
          register double dat3 = 0.0, *tp3 = tablex;
          register short *dp3 = dp2 + dx1;
          while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
          dat2 += dat3 * *(tp2++);
          dp2 += dimx;
        }
        dat += dat2 * *(tp1++);
        dp1 += dim1xdim2;
      }
      out(i) = scale * dat;
    } else
      out(i) = background;
  }
}

void resample_char_sinc(int m, unsigned char *vol, RowVector &out,
                        const RowVector &x, const RowVector &y,
                        const RowVector &z, int dimx, int dimy, int dimz,
                        int nn, double background, double scale) {
  int i, dim1xdim2 = dimx * dimy;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    if (z(i) >= 1 - TINY && z(i) <= dimz + TINY && y(i) >= 1 - TINY &&
        y(i) <= dimy + TINY && x(i) >= 1 - TINY && x(i) <= dimx + TINY) {
      unsigned char *dp1;
      double dat = 0.0, *tp1, *tp1end, *tp2end, *tp3end;

      make_lookup(x(i), nn, dimx, &dx1, tablex, &tp3end);
      make_lookup(y(i), nn, dimy, &dy1, tabley, &tp2end);
      make_lookup(z(i), nn, dimz, &dz1, tablez, &tp1end);

      tp1 = tablez;
      dy1 *= dimx;
      dp1 = vol + dim1xdim2 * dz1;

      while (tp1 <= tp1end) {
        unsigned char *dp2 = dp1 + dy1;
        double dat2 = 0.0, *tp2 = tabley;
        while (tp2 <= tp2end) {
          register double dat3 = 0.0, *tp3 = tablex;
          register unsigned char *dp3 = dp2 + dx1;
          while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
          dat2 += dat3 * *(tp2++);
          dp2 += dimx;
        }
        dat += dat2 * *(tp1++);
        dp1 += dim1xdim2;
      }
      out(i) = scale * dat;
    } else
      out(i) = background;
  }
}

// sinc resampling
void resample_int_sinc(int m, int *vol, RowVector &out, const RowVector &x,
                       const RowVector &y, const RowVector &z, int dimx,
                       int dimy, int dimz, int nn, double background,
                       double scale) {
  int i, dim1xdim2 = dimx * dimy;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    if (z(i) >= 1 - TINY && z(i) <= dimz + TINY && y(i) >= 1 - TINY &&
        y(i) <= dimy + TINY && x(i) >= 1 - TINY && x(i) <= dimx + TINY) {
      int *dp1;
      double dat = 0.0, *tp1, *tp1end, *tp2end, *tp3end;

      make_lookup(x(i), nn, dimx, &dx1, tablex, &tp3end);
      make_lookup(y(i), nn, dimy, &dy1, tabley, &tp2end);
      make_lookup(z(i), nn, dimz, &dz1, tablez, &tp1end);

      tp1 = tablez;
      dy1 *= dimx;
      dp1 = vol + dim1xdim2 * dz1;

      while (tp1 <= tp1end) {
        int *dp2 = dp1 + dy1;
        double dat2 = 0.0, *tp2 = tabley;
        while (tp2 <= tp2end) {
          register double dat3 = 0.0, *tp3 = tablex;
          register int *dp3 = dp2 + dx1;
          while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
          dat2 += dat3 * *(tp2++);
          dp2 += dimx;
        }
        dat += dat2 * *(tp1++);
        dp1 += dim1xdim2;
      }
      out(i) = scale * dat;
    } else
      out(i) = background;
  }
}

void resample_short_1(int m, short *vol, RowVector &out, const RowVector &x,
                      const RowVector &y, const RowVector &z, int dimx,
                      int dimy, int dimz, double background, double scale) {
  int i, dim1xdim2 = dimx * dimy;

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    double xi, yi, zi;
    xi = x(i);
    yi = y(i);
    zi = z(i);

    if (zi >= 1 - TINY && zi <= dimz + TINY && yi >= 1 - TINY &&
        yi <= dimy + TINY && xi >= 1 - TINY && xi <= dimx + TINY) {
      double k111, k112, k121, k122, k211, k212, k221, k222;
      double dx1, dx2, dy1, dy2, dz1, dz2;
      int off1, off2, offx, offy, offz, xcoord, ycoord, zcoord;

      xcoord = (int)floor(xi);
      dx1 = xi - xcoord;
      dx2 = 1.0 - dx1;
      ycoord = (int)floor(yi);
      dy1 = yi - ycoord;
      dy2 = 1.0 - dy1;
      zcoord = (int)floor(zi);
      dz1 = zi - zcoord;
      dz2 = 1.0 - dz1;

      xcoord = (xcoord < 1) ? ((offx = 0), 1)
                            : ((offx = (xcoord >= dimx) ? 0 : 1), xcoord);
      ycoord = (ycoord < 1) ? ((offy = 0), 1)
                            : ((offy = (ycoord >= dimy) ? 0 : dimx), ycoord);
      zcoord = (zcoord < 1)
                   ? ((offz = 0), 1)
                   : ((offz = (zcoord >= dimz) ? 0 : dim1xdim2), zcoord);

      off1 = xcoord + dimx * (ycoord + dimy * zcoord);
      k222 = vol[off1];
      k122 = vol[off1 + offx];
      off2 = off1 + offy;
      k212 = vol[off2];
      k112 = vol[off2 + offx];
      off1 += offz;
      k221 = vol[off1];
      k121 = vol[off1 + offx];
      off2 = off1 + offy;
      k211 = vol[off2];
      k111 = vol[off2 + offx];

      /* resampled pixel value (trilinear interpolation) */
      out(i) =
          scale *
          (((k222 * dx2 + k122 * dx1) * dy2 + (k212 * dx2 + k112 * dx1) * dy1) *
               dz2 +
           ((k221 * dx2 + k121 * dx1) * dy2 + (k211 * dx2 + k111 * dx1) * dy1) *
               dz1);
    } else
      out(i) = background;
  }
}

void resample_char_1(int m, unsigned char *vol, RowVector &out,
                     const RowVector &x, const RowVector &y, const RowVector &z,
                     int dimx, int dimy, int dimz, double background,
                     double scale) {
  int i, dim1xdim2 = dimx * dimy;

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    double xi, yi, zi;
    xi = x(i);
    yi = y(i);
    zi = z(i);

    if (zi >= 1 - TINY && zi <= dimz + TINY && yi >= 1 - TINY &&
        yi <= dimy + TINY && xi >= 1 - TINY && xi <= dimx + TINY) {
      double k111, k112, k121, k122, k211, k212, k221, k222;
      double dx1, dx2, dy1, dy2, dz1, dz2;
      int off1, off2, offx, offy, offz, xcoord, ycoord, zcoord;

      xcoord = (int)floor(xi);
      dx1 = xi - xcoord;
      dx2 = 1.0 - dx1;
      ycoord = (int)floor(yi);
      dy1 = yi - ycoord;
      dy2 = 1.0 - dy1;
      zcoord = (int)floor(zi);
      dz1 = zi - zcoord;
      dz2 = 1.0 - dz1;

      xcoord = (xcoord < 1) ? ((offx = 0), 1)
                            : ((offx = (xcoord >= dimx) ? 0 : 1), xcoord);
      ycoord = (ycoord < 1) ? ((offy = 0), 1)
                            : ((offy = (ycoord >= dimy) ? 0 : dimx), ycoord);
      zcoord = (zcoord < 1)
                   ? ((offz = 0), 1)
                   : ((offz = (zcoord >= dimz) ? 0 : dim1xdim2), zcoord);

      off1 = xcoord + dimx * (ycoord + dimy * zcoord);
      k222 = vol[off1];
      k122 = vol[off1 + offx];
      off2 = off1 + offy;
      k212 = vol[off2];
      k112 = vol[off2 + offx];
      off1 += offz;
      k221 = vol[off1];
      k121 = vol[off1 + offx];
      off2 = off1 + offy;
      k211 = vol[off2];
      k111 = vol[off2 + offx];

      /* resampled pixel value (trilinear interpolation) */
      out(i) =
          scale *
          (((k222 * dx2 + k122 * dx1) * dy2 + (k212 * dx2 + k112 * dx1) * dy1) *
               dz2 +
           ((k221 * dx2 + k121 * dx1) * dy2 + (k211 * dx2 + k111 * dx1) * dy1) *
               dz1);
    } else
      out(i) = background;
  }
}

void resample_int_1(int m, int *vol, RowVector &out, const RowVector &x,
                    const RowVector &y, const RowVector &z, int dimx, int dimy,
                    int dimz, double background, double scale) {
  int i, dim1xdim2 = dimx * dimy;

  vol -= (1 + dimx * (1 + dimy));
  for (i = 0; i < m; i++) {
    double xi, yi, zi;
    xi = x(i);
    yi = y(i);
    zi = z(i);

    if (zi >= 1 - TINY && zi <= dimz + TINY && yi >= 1 - TINY &&
        yi <= dimy + TINY && xi >= 1 - TINY && xi <= dimx + TINY) {
      double k111, k112, k121, k122, k211, k212, k221, k222;
      double dx1, dx2, dy1, dy2, dz1, dz2;
      int off1, off2, offx, offy, offz, xcoord, ycoord, zcoord;

      xcoord = (int)floor(xi);
      dx1 = xi - xcoord;
      dx2 = 1.0 - dx1;
      ycoord = (int)floor(yi);
      dy1 = yi - ycoord;
      dy2 = 1.0 - dy1;
      zcoord = (int)floor(zi);
      dz1 = zi - zcoord;
      dz2 = 1.0 - dz1;

      xcoord = (xcoord < 1) ? ((offx = 0), 1)
                            : ((offx = (xcoord >= dimx) ? 0 : 1), xcoord);
      ycoord = (ycoord < 1) ? ((offy = 0), 1)
                            : ((offy = (ycoord >= dimy) ? 0 : dimx), ycoord);
      zcoord = (zcoord < 1)
                   ? ((offz = 0), 1)
                   : ((offz = (zcoord >= dimz) ? 0 : dim1xdim2), zcoord);

      off1 = xcoord + dimx * (ycoord + dimy * zcoord);
      k222 = vol[off1];
      k122 = vol[off1 + offx];
      off2 = off1 + offy;
      k212 = vol[off2];
      k112 = vol[off2 + offx];
      off1 += offz;
      k221 = vol[off1];
      k121 = vol[off1 + offx];
      off2 = off1 + offy;
      k211 = vol[off2];
      k111 = vol[off2 + offx];

      /* resampled pixel value (trilinear interpolation) */
      out(i) =
          scale *
          (((k222 * dx2 + k122 * dx1) * dy2 + (k212 * dx2 + k112 * dx1) * dy1) *
               dz2 +
           ((k221 * dx2 + k121 * dx1) * dy2 + (k211 * dx2 + k111 * dx1) * dy1) *
               dz1);
    } else
      out(i) = background;
  }
}

Matrix ones(int m, int n) {
  Matrix tmp(m, n, 1.0);
  return tmp;
}

RowVector ones(int d) {
  RowVector tmp(d, 1.0);
  return tmp;
}

Matrix zeros(int m, int n) {
  Matrix tmp(m, n, 0.0);
  return tmp;
}

RowVector zeros(int d) {
  RowVector tmp(d, 0.0);
  return tmp;
}
