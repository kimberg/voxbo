
// imageutils.h
// header information for image processing utils
// Copyright (c) 1998-2007 by The VoxBo Development Team

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

using namespace std;

#include "vbio.h"

// nonmember functions for teses and cubes
int smoothCube_m(Cube &cube, Cube &mask, double s0, double s1, double s2);
int smoothCube(Cube &cube, double s0, double s1, double s2, bool f_correct = 0);
void SNRMap(Tes &tes, Cube &cb);
int buildGaussianKernel(Cube &cube, double s0, double s1, double s2);
int maskKernel(Cube &kernel, Cube &mask, int x, int y, int z);
double getKernelAverage(Cube &cube, Cube &kernel, int x, int y, int z);
int smooth3D(Cube &cube, Cube &mask, Cube &rawkernel);

// resample-related
class Resample {
 private:
  string imagename;  // name of the image to be normalized
  string refname;    // name of the reference image
  string outname;    // name of the output image

  vector<string> newheaders;  // headers to be added to output file

  enum { vb_3d, vb_4d } mode;
  enum { vb_nn, vb_sinc } method;
  Cube *newcube;
  Tes *newtes;
  // double zsize;             // size of slices in mm

  double x1, y1, z1;           // start voxel (0 indexed) in x,y,z
  int nx, ny, nz;              // number of voxels in resampled image
  double xstep, ystep, zstep;  // resample interval in voxels

  // int dimx,dimy,dimz;       // original dimensions
  // double voxsize[3];        // original voxsize
 public:
  Resample();
  ~Resample();
  void init();
  // int ResampleFile();    // detect filetype
  // int Resample3D();      // set up for 3d
  // int Resample4D();      // set up for 4d

  int SincResampleCube(const Cube &mycube,
                       Cube &newcube);                    // do it for each cube
  int NNResampleCube(const Cube &mycube, Cube &newcube);  // do it for each cube
  void AdjustCornerAndOrigin(VBImage &im);

  // methods for working out the resample parameters
  int UseZ(Cube &cb, Cube &refcube, double zsize);
  int UseDims(const Cube &cb, const Cube &refcube);
  int UseSpecifiedDims(const Cube &cb, int dimx, int dimy, int dimz);
  int UseTLHC(const Cube &cb, const Cube &refcube);
  int UseCorner(const Cube &cb, const Cube &refcube);
  int UseCorner2(const Cube &cb, const Cube &refcube);
  int SetAlign(const string mode, const string ref);

  void SetXX(double xx1, double xxstep, int nxx);
  void SetYY(double yy1, double yystep, int nyy);
  void SetZZ(double zz1, double zzstep, int nzz);

  vector<string> headerstrings();

  void SetupDims(Cube &cb, const Cube &ref);
};

void make_lookup(double coord, int nn, int dim, int *d1, double *table,
                 double **ptpend);
template <class T>
void resample_sinc(int m, T *vol, VB_Vector &out, const VB_Vector &x,
                   const VB_Vector &y, const VB_Vector &z, int dimx, int dimy,
                   int dimz, int nn, double background, double scale);

void zero_smallregions(Cube &cb, double thresh);
void rotatecube(Cube &cb, float pitch, float roll, float yaw);

// image manipulations
void enlarge(Cube &cb, uint32 radius, uint32 xradius, uint32 yradius,
             uint32 zradius);
