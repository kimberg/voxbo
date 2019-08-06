
// makestatcub.cpp
// library code to produce different stat cubes
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
// original version written by Kosh Banjeree and Tom King, architected
// by Dan Kimberg

#include "makestatcub.h"
#include "imageutils.h"

int makeStatCub(Cube& cube, string& matrixStemName, VBContrast& contrast,
                VB_Vector& pseudoT, Tes& tes) {
  int err = 0;
  struct stat st;
  double numTails = 1;
  Cube pcube, statcube;
  // get gHeader: after these calls, gHeader = headerMatrix.header
  string headerName = matrixStemName + ".G";
  VBMatrix headerMatrix;
  if (stat(headerName.c_str(), &st)) return 91;
  headerMatrix.ReadHeader(headerName);
  // set orderG and rankG
  unsigned short orderG = headerMatrix.m;
  unsigned short rankG = headerMatrix.n;
  // set F1
  string F1Name = matrixStemName + ".F1";
  if (stat(F1Name.c_str(), &st)) return 92;
  VBMatrix F1(F1Name);
  // set F3
  string F3Name = matrixStemName + ".F3";
  if (stat(F3Name.c_str(), &st)) return 93;
  VBMatrix F3(F3Name);
  // set V
  string VName = matrixStemName + ".V";
  if (stat(VName.c_str(), &st)) return 94;
  VBMatrix V(VName);
  string effdfName = matrixStemName + ".traces";
  if (stat(effdfName.c_str(), &st)) return 95;
  VB_Vector traceVec(effdfName);
  double effdf = 0;
  if (traceVec.size()) {
    effdf = traceVec[2];
  }
  // set keepBetas
  vector<unsigned long> keepBetas;
  tokenlist line;
  for (unsigned int elementNumber = 0;
       elementNumber < headerMatrix.header.size(); elementNumber++) {
    line = headerMatrix.header[elementNumber];
    line[0] = vb_toupper(line[0]);
    line[2] = vb_toupper(line[2]);
    if (line.size())
      if (((line[0] == "PARAMETER:") && (line[2] == "INTEREST")) ||
          ((line[0] == "PARAMETER:") && (line[2] == "KEEPNOINTEREST"))) {
        keepBetas.push_back((unsigned long)atoi(line[1].c_str()));
      }
    line.clear();
  }

  // set betasOfInt
  vector<unsigned long> betasOfInt;
  for (unsigned int elementNumber = 0;
       elementNumber < headerMatrix.header.size(); elementNumber++) {
    line = headerMatrix.header[elementNumber];
    line[0] = vb_toupper(line[0]);
    line[2] = vb_toupper(line[2]);
    if (line.size())
      if ((line[0] == "PARAMETER:") && (line[2] == "INTEREST")) {
        betasOfInt.push_back((unsigned long)atoi(line[1].c_str()));
      }
    line.clear();
  }
  vector<unsigned long> betasToPermute;

  if (contrast.scale == "t" || contrast.scale == "t/1" ||
      contrast.scale == "t/2") {
    err = TStatisticCube(cube, contrast.contrast, pseudoT, tes, rankG, F1, F3,
                         betasOfInt, betasToPermute);
  } else if (contrast.scale == "i")
    err = InterceptTermPercentChange(
        cube, matrixStemName, contrast.contrast, pseudoT, tes,
        headerMatrix.header, orderG, rankG, V, F1, F3, effdf, keepBetas,
        betasOfInt, betasToPermute, contrast.scale);
  else if (contrast.scale == "rb" || contrast.scale == "beta")
    err = RawBetaValues(cube, matrixStemName, contrast.contrast, pseudoT, tes,
                        headerMatrix.header, orderG, rankG, V, F1, F3, effdf,
                        keepBetas, betasOfInt, betasToPermute, contrast.scale);
  else if (contrast.scale == "f")
    err = FStatisticCube(cube, matrixStemName, contrast.contrast, pseudoT, tes,
                         headerMatrix.header, orderG, rankG, V, F1, F3, effdf,
                         keepBetas, betasOfInt, betasToPermute, contrast.scale);
  else if (contrast.scale == "tp" || contrast.scale == "tp/1" ||
           contrast.scale == "tp/2") {
    err = TStatisticCube(cube, contrast.contrast, pseudoT, tes, rankG, F1, F3,
                         betasOfInt, betasToPermute);
    if (err == 0) {
      if (contrast.scale == "tp/2") numTails = 2;
      err = TTestPMap(cube, tes, numTails, effdf);
    }
  } else if (contrast.scale == "fp") {
    err = FStatisticCube(cube, matrixStemName, contrast.contrast, pseudoT, tes,
                         headerMatrix.header, orderG, rankG, V, F1, F3, effdf,
                         keepBetas, betasOfInt, betasToPermute, contrast.scale);
    if (err == 0) err = FTestPMap(cube, tes, betasOfInt.size(), effdf);
  } else if (contrast.scale == "tz" || contrast.scale == "tz/1" ||
             contrast.scale == "tz/2") {
    err = TStatisticCube(cube, contrast.contrast, pseudoT, tes, rankG, F1, F3,
                         betasOfInt, betasToPermute);
    if (err == 0) {
      if (contrast.scale == "tz/2") numTails = 2;  // default = 1;
      err = TTestZMap(cube, tes, numTails, effdf);
    }
  }
  if (contrast.scale == "fz") {
    err = FStatisticCube(cube, matrixStemName, contrast.contrast, pseudoT, tes,
                         headerMatrix.header, orderG, rankG, V, F1, F3, effdf,
                         keepBetas, betasOfInt, betasToPermute, contrast.scale);
    if (err == 0) err = FTestZMap(cube, tes, betasOfInt.size(), effdf);
  }
  for (int i = 0; i < 3; i++) {
    cube.origin[i] = tes.origin[i];
    cube.voxsize[i] = tes.voxsize[i];
  }
  return err;
}

int TStatisticCube(Cube& cube, VB_Vector& contrasts, VB_Vector& pseudoT,
                   Tes& paramTes, unsigned short rankG, VBMatrix& F1,
                   VBMatrix& F3, vector<unsigned long>& betasOfInt,
                   const vector<unsigned long>& betasToPermute) {
  Cube errorCube(paramTes.dimx, paramTes.dimy, paramTes.dimz,
                 paramTes.datatype);
  Cube statCube(paramTes.dimx, paramTes.dimy, paramTes.dimz, paramTes.datatype);

  int X = 0, Y = 0, Z = 0;

  for (X = 0; X < paramTes.dimx; X++) {
    for (Y = 0; Y < paramTes.dimy; Y++) {
      for (Z = 0; Z < paramTes.dimz; Z++) {
        errorCube.SetValue(X, Y, Z,
                           paramTes.GetValue(X, Y, Z, paramTes.dimt - 1));
        statCube.SetValue(X, Y, Z, 0.0);
      }
    }
  }

  if ((betasToPermute.size() == (size_t)contrasts.size()) &&
      (betasToPermute.size() < rankG)) {
    VB_Vector newContrasts(rankG);
    for (size_t i = 0; i < betasToPermute.size(); i++) {
      newContrasts[betasToPermute[i]] = contrasts[i];
    }
    contrasts.resize(newContrasts.size());
    for (size_t i = 0; i < (size_t)contrasts.size(); i++) {
      contrasts[i] = newContrasts[i];
    }
  }

  if ((size_t)paramTes.dimt - 1 != (betasOfInt.size() + 1)) {
    if (paramTes.dimt - 1 == (rankG + 1)) {
      betasOfInt.resize(rankG);
      for (size_t i = 0; i < rankG; i++) {
        betasOfInt[i] = i;
      }
    }
  }

  double conAbsSum = 0.0;
  vector<unsigned long> interceptPos;
  for (size_t i = 0; i < (size_t)contrasts.size(); i++) {
    conAbsSum += fabs(contrasts[i]);
  }
  if (!conAbsSum) {
    for (X = 0; X < paramTes.dimx; X++) {
      for (Y = 0; Y < paramTes.dimy; Y++) {
        for (Z = 0; Z < paramTes.dimz; Z++) {
          errorCube.SetValue(X, Y, Z, sqrt(errorCube.GetValue(X, Y, Z)));
        }
      }
    }
    cube = errorCube;
    return 0;
  }

  // calculate the fact
  VBMatrix c(contrasts);
  VBMatrix ct(contrasts);
  ct.transposed = 1;
  ct *= F1;
  ct *= F3;
  ct *= c;
  double fact = ct(0, 0);

  for (X = 0; X < paramTes.dimx; X++) {
    for (Y = 0; Y < paramTes.dimy; Y++) {
      for (Z = 0; Z < paramTes.dimz; Z++) {
        errorCube.SetValue(X, Y, Z, sqrt(errorCube.GetValue(X, Y, Z) * fact));
      }
    }
  }

  // smooth the error map for pseudo-t map
  if (pseudoT.size() == 3 && pseudoT.getMaxElement() > 0.0) {
    Cube smoothedMask;
    smoothedMask = errorCube;

    smoothCube(errorCube, pseudoT[0], pseudoT[1], pseudoT[2]);
    double temp = 0.0;
    for (int i = 0; i < paramTes.dimx; i++)
      for (int j = 0; j < paramTes.dimy; j++)
        for (int k = 0; k < paramTes.dimz; k++)
          if (paramTes.GetMaskValue(i, j, k) == 0)
            smoothedMask.SetValue(i, j, k, 0.0);
          else
            smoothedMask.SetValue(i, j, k, 1.0);
    smoothCube(smoothedMask, pseudoT[0], pseudoT[1], pseudoT[2]);

    for (int i = 0; i < paramTes.dimx; i++)
      for (int j = 0; j < paramTes.dimy; j++)
        for (int k = 0; k < paramTes.dimz; k++)
          if (paramTes.GetMaskValue(i, j, k) == 0)
            errorCube.SetValue(i, j, k, 0.0);
          else {
            temp = errorCube.GetValue(i, j, k) / smoothedMask.GetValue(i, j, k);
            errorCube.SetValue(i, j, k, temp);
          }
  }

  VB_Vector betas(rankG);
  unsigned long planeSize = paramTes.dimx * paramTes.dimy;
  unsigned long b = 0;
  int i = 0, j = 0, k = 0;
  for (i = 0; i < paramTes.dimx; i++) {
    for (j = 0; j < paramTes.dimy; j++) {
      for (k = 0; k < paramTes.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) == 0) {
        } else {
          b = paramTes.voxelposition(i, j, k) % planeSize;
          for (int t = 0; t < paramTes.dimt - 1; t++) {
            betas[t] = paramTes.GetValue(
                (b % paramTes.dimx), (b / paramTes.dimx),
                (paramTes.voxelposition(i, j, k) / planeSize), t);
          }
          // temp fix while removing stand_alone code
          double product = 0.0;
          for (int num = 0; num < (int)betas.getLength(); num++)
            product += (contrasts[num] * betas[num]);
          statCube.SetValue(i, j, k, product / errorCube.GetValue(i, j, k));
        }
      }
    }
  }
  cube = statCube;
  return 0;
}

int InterceptTermPercentChange(Cube& cube, const string& matrixStemName,
                               VB_Vector& contrasts, VB_Vector&, Tes& paramTes,
                               const vector<string>& gHeader, unsigned short,
                               unsigned short rankG, VBMatrix&, VBMatrix&,
                               VBMatrix&, const double, vector<unsigned long>&,
                               vector<unsigned long>& betasOfInt,
                               const vector<unsigned long>& betasToPermute,
                               string) throw() {
  if (matrixStemName.size() == 0) return 99;

  Cube errorCube(paramTes.dimx, paramTes.dimy, paramTes.dimz,
                 paramTes.datatype);
  Cube statCube(paramTes.dimx, paramTes.dimy, paramTes.dimz, paramTes.datatype);
  int X = 0, Y = 0, Z = 0;

  for (X = 0; X < paramTes.dimx; X++) {
    for (Y = 0; Y < paramTes.dimy; Y++) {
      for (Z = 0; Z < paramTes.dimz; Z++) {
        errorCube.SetValue(X, Y, Z,
                           paramTes.GetValue(X, Y, Z, paramTes.dimt - 1));
        statCube.SetValue(X, Y, Z, 0.0);
      }
    }
  }

  if ((betasToPermute.size() == (size_t)contrasts.size()) &&
      (betasToPermute.size() < rankG)) {
    VB_Vector newContrasts(rankG);
    for (size_t i = 0; i < betasToPermute.size(); i++)
      newContrasts[betasToPermute[i]] = contrasts[i];
    contrasts.resize(newContrasts.size());
    for (size_t i = 0; i < (size_t)contrasts.size(); i++)
      contrasts[i] = newContrasts[i];
  }

  if ((size_t)paramTes.dimt - 1 != (betasOfInt.size() + 1)) {
    if (paramTes.dimt - 1 == (rankG + 1)) {
      betasOfInt.resize(rankG);
      for (size_t i = 0; i < rankG; i++) betasOfInt[i] = i;
    }
  }

  vector<unsigned long> interceptPos;

  if (gHeader.size()) {
    for (unsigned short i = 0; i < gHeader.size(); i++) {
      if (gHeader[i].size() > 0) {
        tokenlist S(gHeader[i]);
        string name = vb_tolower(S[3]);
        if (S[0] == "Parameter:" && name == "intercept")
          interceptPos.push_back(strtol(S[1]));
      }
    }
  } else
    return 102;

  if (interceptPos.size() != 1) return 152;

  double conAbsSum = 0.0;
  for (size_t i = 0; i < (size_t)contrasts.size(); i++)
    conAbsSum += fabs(contrasts[i]);

  if (!conAbsSum) {
    Cube intCube(paramTes.dimx, paramTes.dimy, paramTes.dimz,
                 paramTes.datatype);
    int X, Y, Z;
    for (X = 0; X < paramTes.dimx; X++) {
      for (Y = 0; Y < paramTes.dimy; Y++) {
        for (Z = 0; Z < paramTes.dimz; Z++) {
          intCube.SetValue(X, Y, Z,
                           paramTes.GetValue(X, Y, Z, interceptPos[0]));
        }
      }
    }
    cube = intCube;
    return 0;
  }

  VB_Vector betas(rankG);
  unsigned long planeSize = paramTes.dimx * paramTes.dimy;
  unsigned long b = 0;
  int i = 0, j = 0, k = 0;

  for (i = 0; i < paramTes.dimx; i++) {
    for (j = 0; j < paramTes.dimy; j++) {
      for (k = 0; k < paramTes.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) == 0) {
        } else {
          b = paramTes.voxelposition(i, j, k) % planeSize;
          for (int t = 0; t < paramTes.dimt - 1; t++) {
            betas[t] = paramTes.GetValue(
                (b % paramTes.dimx), (b / paramTes.dimx),
                (paramTes.voxelposition(i, j, k) / planeSize), t);
          }
          // temp fix while removing stand_alone code
          double product = 0.0;
          for (int num = 0; num < (int)betas.getLength(); num++)
            product += (contrasts[num] * betas[num]);
          statCube.SetValue(i, j, k, product / betas[interceptPos[0]]);
        }
      }
    }
  }
  cube = statCube;
  return 0;
}

int RawBetaValues(Cube& cube, const string& matrixStemName,
                  VB_Vector& contrasts, VB_Vector&, Tes& paramTes,
                  const vector<string>&, unsigned short, unsigned short rankG,
                  VBMatrix&, VBMatrix&, VBMatrix&, const double,
                  vector<unsigned long>&, vector<unsigned long>& betasOfInt,
                  const vector<unsigned long>& betasToPermute, string) throw() {
  if (matrixStemName.size() == 0) return 99;

  Cube errorCube(paramTes.dimx, paramTes.dimy, paramTes.dimz,
                 paramTes.datatype);
  Cube statCube(paramTes.dimx, paramTes.dimy, paramTes.dimz, paramTes.datatype);

  int X = 0, Y = 0, Z = 0;

  for (X = 0; X < paramTes.dimx; X++) {
    for (Y = 0; Y < paramTes.dimy; Y++) {
      for (Z = 0; Z < paramTes.dimz; Z++) {
        errorCube.SetValue(X, Y, Z,
                           paramTes.GetValue(X, Y, Z, paramTes.dimt - 1));
        statCube.SetValue(X, Y, Z, 0.0);
      }
    }
  }

  if ((betasToPermute.size() == (size_t)contrasts.size()) &&
      (betasToPermute.size() < rankG)) {
    VB_Vector newContrasts(rankG);
    for (size_t i = 0; i < betasToPermute.size(); i++)
      newContrasts[betasToPermute[i]] = contrasts[i];
    contrasts.resize(newContrasts.size());
    for (size_t i = 0; i < (size_t)contrasts.size(); i++)
      contrasts[i] = newContrasts[i];

    if ((size_t)paramTes.dimt - 1 != (betasOfInt.size() + 1)) {
      if (paramTes.dimt - 1 == (rankG + 1)) betasOfInt.resize(rankG);
      for (size_t i = 0; i < rankG; i++) betasOfInt[i] = i;
    }
  }

  double conAbsSum = 0.0;
  for (size_t i = 0; i < (size_t)contrasts.size(); i++)
    conAbsSum += fabs(contrasts[i]);

  if (!conAbsSum) {
    int X, Y, Z;
    for (X = 0; X < paramTes.dimx; X++) {
      for (Y = 0; Y < paramTes.dimy; Y++) {
        for (Z = 0; Z < paramTes.dimz; Z++) {
          errorCube.SetValue(X, Y, Z, sqrt(errorCube.GetValue(X, Y, Z)));
        }
      }
    }
    cube = errorCube;
    return 0;
  }

  unsigned long planeSize = paramTes.dimx * paramTes.dimy;
  unsigned long b = 0;
  int i = 0, j = 0, k = 0;

  gsl_matrix* Betas = gsl_matrix_calloc(1, rankG);
  gsl_matrix* contrastMatrix = gsl_matrix_calloc(contrasts.size(), 1);
  gsl_matrix* result = gsl_matrix_calloc(Betas->size1, contrastMatrix->size2);
  for (int x = 0; x < (int)contrastMatrix->size1; x++)
    gsl_matrix_set(contrastMatrix, x, 0, contrasts[x]);

  double betaValue = 0.0;
  for (i = 0; i < paramTes.dimx; i++) {
    for (j = 0; j < paramTes.dimy; j++) {
      for (k = 0; k < paramTes.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) == 0) {
        } else {
          b = paramTes.voxelposition(i, j, k) % planeSize;
          for (int t = 0; t < paramTes.dimt - 1; t++) {
            betaValue = paramTes.GetValue(
                (b % paramTes.dimx), (b / paramTes.dimx),
                (paramTes.voxelposition(i, j, k) / planeSize), t);
            gsl_matrix_set(Betas, 0, t, betaValue);
            betaValue = 0.0;
          }
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Betas, contrastMatrix,
                         0.0, result);
          statCube.SetValue(i, j, k, gsl_matrix_get(result, 0, 0));
          gsl_matrix_set_zero(result);
          gsl_matrix_set_zero(Betas);
        }
      }
    }
  }
  if (Betas) gsl_matrix_free(Betas);
  if (contrastMatrix) gsl_matrix_free(contrastMatrix);
  if (result) gsl_matrix_free(result);
  cube = statCube;
  return 0;
}

int FStatisticCube(Cube& cube, const string& matrixStemName,
                   VB_Vector& contrasts, VB_Vector&, Tes& paramTes,
                   const vector<string>&, unsigned short, unsigned short rankG,
                   VBMatrix& V, VBMatrix& F1, VBMatrix&, const double,
                   vector<unsigned long>&, vector<unsigned long>& betasOfInt,
                   const vector<unsigned long>& betasToPermute,
                   string) throw() {
  if (matrixStemName.size() == 0) return 99;

  Cube errorCube(paramTes.dimx, paramTes.dimy, paramTes.dimz,
                 paramTes.datatype);
  Cube statCube(paramTes.dimx, paramTes.dimy, paramTes.dimz, paramTes.datatype);

  int X = 0, Y = 0, Z = 0;

  for (X = 0; X < paramTes.dimx; X++) {
    for (Y = 0; Y < paramTes.dimy; Y++) {
      for (Z = 0; Z < paramTes.dimz; Z++) {
        errorCube.SetValue(X, Y, Z,
                           paramTes.GetValue(X, Y, Z, paramTes.dimt - 1));
        statCube.SetValue(X, Y, Z, 0.0);
      }
    }
  }

  if ((betasToPermute.size() == (size_t)contrasts.size()) &&
      (betasToPermute.size() < rankG)) {
    VB_Vector newContrasts(rankG);
    for (size_t i = 0; i < betasToPermute.size(); i++)
      newContrasts[betasToPermute[i]] = contrasts[i];
    contrasts.resize(newContrasts.size());
    for (size_t i = 0; i < (size_t)contrasts.size(); i++)
      contrasts[i] = newContrasts[i];

    if ((size_t)paramTes.dimt - 1 != (betasOfInt.size() + 1)) {
      if (paramTes.dimt - 1 == (rankG + 1)) betasOfInt.resize(rankG);
      for (size_t i = 0; i < rankG; i++) betasOfInt[i] = i;
    }
  }

  double conAbsSum = 0.0;
  for (size_t i = 0; i < (size_t)contrasts.size(); i++)
    conAbsSum += fabs(contrasts[i]);

  if (!conAbsSum) {
    int X, Y, Z;
    for (X = 0; X < paramTes.dimx; X++) {
      for (Y = 0; Y < paramTes.dimy; Y++) {
        for (Z = 0; Z < paramTes.dimz; Z++) {
          errorCube.SetValue(X, Y, Z, sqrt(errorCube.GetValue(X, Y, Z)));
        }
      }
    }
    cube = errorCube;
    return 0;
  }

  gsl_matrix* iso = NULL;
  gsl_matrix* varIsoBetas = NULL;
  vector<unsigned long> noInterest;
  vector<unsigned long> interest;

  for (size_t i = 0; i < (size_t)contrasts.size(); i++) {
    if (contrasts[i] == 0.0) {
      noInterest.push_back(i);
    } else
      interest.push_back(i);
  }

  // begin:iso##fac1##V##transpose(fac1)##transpose(iso)
  iso = gsl_matrix_calloc(interest.size(), contrasts.size());
  if (!iso) return 104;
  for (int i = 0; i < (int)interest.size(); i++)
    for (int j = 0; j < (int)contrasts.size(); j++)
      gsl_matrix_set(iso, i, interest[i], contrasts[interest[i]]);
  gsl_matrix* isoFac1 = gsl_matrix_calloc(iso->size1, F1.n);
  if (!isoFac1) return 105;
  gsl_matrix* F1Matrix = gsl_matrix_calloc(F1.m, F1.n);
  for (int i = 0; i < (int)F1.m; i++)
    for (int j = 0; j < (int)F1.n; j++)
      gsl_matrix_set(F1Matrix, i, j, F1(i, j));
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, iso, F1Matrix, 0.0, isoFac1);
  gsl_matrix* isoFac1V = gsl_matrix_calloc(isoFac1->size1, V.n);
  if (!isoFac1V) return 106;
  gsl_matrix* VMatrix = gsl_matrix_alloc(V.m, V.n);
  for (uint32 i = 0; i < V.m; i++)
    for (uint32 j = 0; j < V.n; j++) gsl_matrix_set(VMatrix, i, j, V(i, j));
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, isoFac1, VMatrix, 0.0,
                 isoFac1V);
  gsl_matrix_free(isoFac1);
  gsl_matrix* VF1 = gsl_matrix_calloc(isoFac1V->size1, F1Matrix->size1);
  if (!VF1) return 107;
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, isoFac1V, F1Matrix, 0.0, VF1);
  gsl_matrix_free(isoFac1V);
  varIsoBetas = gsl_matrix_calloc(VF1->size1, iso->size1);
  if (!varIsoBetas) return 108;
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, VF1, iso, 0.0, varIsoBetas);
  gsl_matrix_free(VMatrix);
  gsl_matrix_free(VF1);
  gsl_matrix_free(F1Matrix);
  // end:iso##fac1##V##transpose(fac1)##transpose(iso)

  // begin: invert(varIsoBetas)
  gsl_permutation* LUPerm = gsl_permutation_calloc(varIsoBetas->size1);
  if (!LUPerm) return 112;
  int permSign = 0;
  gsl_linalg_LU_decomp(varIsoBetas, LUPerm, &permSign);
  gsl_matrix* invVarIsoBetas =
      gsl_matrix_calloc(varIsoBetas->size1, varIsoBetas->size2);
  if (!invVarIsoBetas) return 113;
  if (gsl_linalg_LU_invert(varIsoBetas, LUPerm, invVarIsoBetas) != GSL_SUCCESS)
    return 114;
  gsl_permutation_free(LUPerm);
  // end: invert(varIsoBetas)

  gsl_matrix* Betas = gsl_matrix_calloc(rankG, 1);
  gsl_matrix* isoBetas = gsl_matrix_calloc(iso->size1, Betas->size2);
  gsl_matrix* TisoBetasInvVarIsoBetas =
      gsl_matrix_calloc(isoBetas->size2, invVarIsoBetas->size2);
  gsl_matrix* fnumerator =
      gsl_matrix_calloc(TisoBetasInvVarIsoBetas->size1, isoBetas->size2);
  unsigned long planeSize = paramTes.dimx * paramTes.dimy;
  unsigned long b = 0;
  double betaValue;
  int i = 0, j = 0, k = 0;

  for (i = 0; i < paramTes.dimx; i++) {
    for (j = 0; j < paramTes.dimy; j++) {
      for (k = 0; k < paramTes.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) == 0) {
        } else {
          b = paramTes.voxelposition(i, j, k) % planeSize;
          for (int t = 0; t < paramTes.dimt - 1; t++) {
            betaValue = paramTes.GetValue(
                (b % paramTes.dimx), (b / paramTes.dimx),
                (paramTes.voxelposition(i, j, k) / planeSize), t);
            gsl_matrix_set(Betas, t, 0, betaValue);
            betaValue = 0.0;
          }

          // note: can merge interest==1 and interest > 1 into block below
          double fNumerator = 0.0;
          // F_numerator=transpose(IsoBetas)##invert(Var_IsoBetas)##IsoBetas/InterestCount
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, iso, Betas, 0.0,
                         isoBetas);
          gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, isoBetas,
                         invVarIsoBetas, 0.0, TisoBetasInvVarIsoBetas);
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0,
                         TisoBetasInvVarIsoBetas, isoBetas, 0.0, fnumerator);
          fNumerator = gsl_matrix_get(fnumerator, 0, 0) / interest.size();
          if (isoBetas) gsl_matrix_set_zero(isoBetas);
          if (TisoBetasInvVarIsoBetas)
            gsl_matrix_set_zero(TisoBetasInvVarIsoBetas);
          if (Betas) gsl_matrix_set_zero(Betas);
          statCube.SetValue(i, j, k, fNumerator / errorCube.GetValue(i, j, k));
          if (fnumerator) gsl_matrix_set_zero(fnumerator);
        }
      }
    }
  }

  if (isoBetas) gsl_matrix_free(isoBetas);
  if (TisoBetasInvVarIsoBetas) gsl_matrix_free(TisoBetasInvVarIsoBetas);
  if (Betas) gsl_matrix_free(Betas);
  if (fnumerator) gsl_matrix_free(fnumerator);
  if (iso) gsl_matrix_free(iso);
  if (varIsoBetas) gsl_matrix_free(varIsoBetas);

  cube = statCube;
  return 0;
}

int TTestPMap(Cube& cube, Tes& paramTes, double numTails, double effdf) {
  int i = 0, j = 0, k = 0;
  double pVal = 0.0, tVal = 0.0;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      for (k = 0; k < cube.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) != 0) {
          tVal = cube.GetValue(i, j, k);
          pVal = gsl_cdf_tdist_Q(tVal, effdf);
          if (numTails == 2) {
            if (tVal < 0) pVal = 1 - pVal;
            pVal *= 2.0;
          }
          cube.SetValue(i, j, k, pVal);
        }
      }
    }
  }
  return 0;
}

int TTestZMap(Cube& cube, Tes& paramTes, double numTails, double effdf) {
  int i = 0, j = 0, k = 0;
  double pVal = 0.0, zVal = 0.0, tVal = 0.0;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      for (k = 0; k < cube.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) != 0) {
          tVal = cube.GetValue(i, j, k);
          pVal = gsl_cdf_tdist_Q(tVal, effdf);
          if (numTails == 2) {
            if (tVal < 0) pVal = 1 - pVal;
            pVal *= 2.0;
          }
          zVal = gsl_cdf_ugaussian_Qinv(pVal);
          if (tVal < 0.0) zVal = 0 - zVal;
          cube.SetValue(i, j, k, zVal);
        }
      }
    }
  }
  return 0;
}

int FTestPMap(Cube& cube, Tes& paramTes, double numCovariates, double effdf) {
  int i = 0, j = 0, k = 0;
  double pVal = 0.0;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      for (k = 0; k < cube.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) != 0) {
          pVal = gsl_cdf_fdist_Q(cube.GetValue(i, j, k), numCovariates, effdf);
          cube.SetValue(i, j, k, pVal);
        }
      }
    }
  }
  return 0;
}

int FTestZMap(Cube& cube, Tes& paramTes, double numCovariates, double effdf) {
  int i = 0, j = 0, k = 0;
  double pVal = 0.0, zVal = 0.0;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      for (k = 0; k < cube.dimz; k++) {
        if (paramTes.GetMaskValue(i, j, k) != 0) {
          pVal = gsl_cdf_fdist_Q(cube.GetValue(i, j, k), numCovariates, effdf);
          zVal = gsl_cdf_ugaussian_Qinv(pVal);
          cube.SetValue(i, j, k, zVal);
        }
      }
    }
  }
  return 0;
}

vector<fdrstat> calc_multi_fdr_thresh(Cube& statcube, Cube& pcube, Cube& mask,
                                      double q) {
  vector<double> myqs;
  if (q < DBL_MIN) {
    myqs.push_back(0.01);
    myqs.push_back(0.02);
    myqs.push_back(0.03);
    myqs.push_back(0.04);
    myqs.push_back(0.05);
    myqs.push_back(0.10);
    myqs.push_back(0.15);
    myqs.push_back(0.20);
    myqs.push_back(0.40);
  } else
    myqs.push_back(q);
  return calc_multi_fdr_thresh(statcube, pcube, mask, myqs);
}

vector<fdrstat> calc_multi_fdr_thresh(Cube& statcube, Cube& pcube, Cube& mask,
                                      vector<double> qs) {
  vector<VBVoxel> plist;
  int i, j, k;
  double pvalue = 0.0;
  VBVoxel voxel;
  vector<fdrstat> fdrstats;

  vbforeach(double qval, qs) fdrstats.push_back(fdrstat(qval));
  for (i = 0; i < pcube.dimx; i++) {
    for (j = 0; j < pcube.dimy; j++) {
      for (k = 0; k < pcube.dimz; k++) {
        if (mask.data && !(mask.GetValue(i, j, k))) continue;
        pvalue = pcube.GetValue(i, j, k);
        voxel.val = fabs(pvalue);
        voxel.x = i;
        voxel.y = j;
        voxel.z = k;
        plist.push_back(voxel);
      }
    }
  }
  if (plist.size() == 0) return fdrstats;
  sort(plist.begin(), plist.end(), vcompare);
  // go through, testing P(i)<=(i/V)(q/cv)
  // cv=1, so our factor to multiply i by is just q/V
  vbforeach(fdrstat & ff, fdrstats) {
    ff.maxind = -1;
    ff.qv = ff.q / plist.size();
    ff.low = plist[0].val;
    ff.high = plist[plist.size() - 1].val;
    ff.nvoxels = plist.size();
  }
  // int maxind=-1;
  // double qv=q/plist.size();
  double vv;
  for (i = 0; i < (int)plist.size(); i++) {
    vv = plist[i].val;
    vbforeach(fdrstat & ff, fdrstats) {
      if (vv <= (double)(i + 1) * ff.qv) ff.maxind = i;
    }
  }
  vbforeach(fdrstat & ff, fdrstats) {
    if (ff.maxind >= 0)
      ff.statval = fabs(statcube.GetValue(
          plist[ff.maxind].x, plist[ff.maxind].y, plist[ff.maxind].z));
    else
      ff.statval = 0;
  }
  return fdrstats;
}
