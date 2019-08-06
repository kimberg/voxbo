
// vbse.cpp
// estimate smoothness from residual map
// Copyright (c) 1998-2005 by Team VoxBo

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
// some code written by Kosh Banerjee, based on code by Geoff Aguirre,
// based on methods by Kiebel et al.

using namespace std;

#include <stdio.h>
#include <string.h>
#include <sstream>
#include "vbio.h"
#include "vbse.hlp.h"
#include "vbutil.h"

void vbse_help();
void vbse_version();
int residualSmoothness(Tes &resTes, VB_Vector &mapSmoothness);
unsigned short shiftLeft(const unsigned short index,
                         const unsigned short dimIndex);

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbse_help();
    exit(0);
  } else if (argc != 3) {
    vbse_help();
    exit(100);
  }
  string infile = argv[1];
  string outfile = argv[2];

  Tes mytes;
  if (mytes.ReadFile(infile)) {
    printf("[E] vbse: couldn't read file %s\n", infile.c_str());
    exit(200);
  }

  printf("[I] vbse: estimating smoothness from %s\n", infile.c_str());
  VB_Vector smoothness(3);
  residualSmoothness(mytes, smoothness);
  smoothness.setFileName(outfile);
  if (smoothness.WriteFile()) {
    printf("[E] vbse: error writing file %s\n", outfile.c_str());
    exit(110);
  } else
    printf("[I] vbse: done.\n");
  exit(0);
}

int residualSmoothness(Tes &resTes, VB_Vector &mapSmoothness) {
  uint32 volume = resTes.dimx * resTes.dimy * resTes.dimz;
  vector<uint32> maskPoints;
  vector<uint32> xNonZeros;
  vector<uint32> yNonZeros;
  vector<uint32> zNonZeros;

  double *rssq = new double[volume];
  memset(rssq, 0, sizeof(double) * volume);
  vector<uint32> noBrain;

  for (int i = 0; i < resTes.dimx; i++) {
    for (int j = 0; j < resTes.dimy; j++) {
      for (int k = 0; k < resTes.dimz; k++) {
        unsigned char xValue = 0, yValue = 0, zValue = 0;

        if (i != 0) {
          xValue = resTes.mask[resTes.voxelposition(i - 1, j, k)];
        }  // if

        if (j != 0) {
          yValue = resTes.mask[resTes.voxelposition(i, j - 1, k)];
        }  // if

        if (k != 0) {
          zValue = resTes.mask[resTes.voxelposition(i, j, k - 1)];
        }  // if

        if (resTes.mask[resTes.voxelposition(i, j, k)] && xValue && yValue &&
            zValue) {
          maskPoints.push_back(resTes.voxelposition(i, j, k));

          xNonZeros.push_back(
              resTes.voxelposition(shiftLeft(i, resTes.dimx), j, k));
          yNonZeros.push_back(
              resTes.voxelposition(i, shiftLeft(j, resTes.dimy), k));
          zNonZeros.push_back(
              resTes.voxelposition(i, j, shiftLeft(k, resTes.dimz)));
        }  // if

        for (int ii = 0; ii < resTes.dimt; ii++) {
          double tempDouble = resTes.GetValue(i, j, k, ii);
          rssq[resTes.voxelposition(i, j, k)] += tempDouble * tempDouble;
        }  // for i
        rssq[resTes.voxelposition(i, j, k)] =
            sqrt(rssq[resTes.voxelposition(i, j, k)]);

        if (rssq[resTes.voxelposition(i, j, k)] == 0.0) {
          noBrain.push_back(resTes.voxelposition(i, j, k));
          rssq[resTes.voxelposition(i, j, k)] = 1.0;
        }  // if

        for (unsigned short t = 0; t < resTes.dimt; t++) {
          resTes.SetValue(i, j, k, t,
                          resTes.GetValue(i, j, k, t) /
                              rssq[resTes.voxelposition(i, j, k)]);
        }  // for t
      }
    }
  }

  delete[] rssq;

  VB_Vector SSQ_Deriv_X(volume);
  VB_Vector SSQ_Deriv_Y(volume);
  VB_Vector SSQ_Deriv_Z(volume);

  VB_Vector pointData2(resTes.dimt);
  double tempDouble = log(16.0);

  for (size_t i = 0; i < maskPoints.size(); i++) {
    VB_Vector pointData(resTes, maskPoints[i]);

    VB_Vector pointDataX(resTes, xNonZeros[i]);
    pointData2 = pointData - pointDataX;
    pointData2 *= pointData2;
    pointData2 /= tempDouble;
    SSQ_Deriv_X[maskPoints[i]] = sqrt(pointData2.getVectorSum());

    VB_Vector pointDataY(resTes, yNonZeros[i]);
    pointData2 = pointData - pointDataY;
    pointData2 *= pointData2;
    pointData2 /= tempDouble;
    SSQ_Deriv_Y[maskPoints[i]] = sqrt(pointData2.getVectorSum());

    VB_Vector pointDataZ(resTes, zNonZeros[i]);
    pointData2 = pointData - pointDataZ;
    pointData2 *= pointData2;
    pointData2 /= tempDouble;
    SSQ_Deriv_Z[maskPoints[i]] = sqrt(pointData2.getVectorSum());
  }  // for i

  double unbiasT = ((double)resTes.dimt - 2.0) / ((double)resTes.dimt - 1.0);

  mapSmoothness[0] = SSQ_Deriv_X.getVectorSum() / (double)maskPoints.size();
  mapSmoothness[1] = SSQ_Deriv_Y.getVectorSum() / (double)maskPoints.size();
  mapSmoothness[2] = SSQ_Deriv_Z.getVectorSum() / (double)maskPoints.size();

  SSQ_Deriv_X *= SSQ_Deriv_Y;
  SSQ_Deriv_X *= SSQ_Deriv_Z;
  double resels = SSQ_Deriv_X.getVectorSum() / (double)maskPoints.size();
  tempDouble =
      resels / (mapSmoothness[0] * mapSmoothness[1] * mapSmoothness[2]);
  tempDouble = pow(tempDouble, (1.0 / 3.0));

  mapSmoothness[0] = (1.0 / (mapSmoothness[0] * tempDouble)) * unbiasT;
  mapSmoothness[1] = (1.0 / (mapSmoothness[1] * tempDouble)) * unbiasT;
  mapSmoothness[2] = (1.0 / (mapSmoothness[2] * tempDouble)) * unbiasT;

  return 0;
}

unsigned short shiftLeft(const unsigned short index,
                         const unsigned short dimIndex) {
  if (index == 0) return (dimIndex - 1);
  return (index - 1);
}

void vbse_help() { cout << boost::format(myhelp) % vbversion; }

void vbse_version() { printf("VoxBo vbse (v%s)\n", vbversion.c_str()); }
