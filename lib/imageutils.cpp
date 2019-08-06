
// imageutils.cpp
// non-member functions applicable to 3D and 4D data
// Copyright (c) 2003-2011 by The VoxBo Development Team

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
// some code written with reference to old SPM code by John Ashburner

using namespace std;

#include "imageutils.h"
#include "vbio.h"
#include "vbutil.h"

void conv3d(Cube &cube, VB_Vector &kx, VB_Vector &ky, VB_Vector &kz);
void conv3dx(Cube &cube, VB_Vector &kx, VB_Vector &ky, VB_Vector &kz);
int returnReverseOrientation(string &s);

int smoothCube_m(Cube &cube, Cube &mask, double s0, double s1, double s2) {
  if (mask.dimx != cube.dimx || mask.dimy != cube.dimy ||
      mask.dimz != cube.dimz)
    return 101;
  Cube smask = mask;
  if (smoothCube(smask, s0, s1, s2)) return 102;
  if (smoothCube(cube, s0, s1, s2)) return 103;
  double val;
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        val = smask.GetValue(i, j, k);
        if (val > 0.0) cube.SetValue(i, j, k, cube.GetValue(i, j, k) / val);
      }
    }
  }
  return 0;
}

int smoothCube(Cube &cube, double s0, double s1, double s2, bool f_correct) {
  int i;
  double xsum, ysum, zsum;
  short x1, y1, z1;

  if (s0 < 1) s0 = 1;
  if (s1 < 1) s1 = 1;
  if (s2 < 1) s2 = 1;
  s0 = s0 / sqrt(8.0 * log(2.0));
  s1 = s1 / sqrt(8.0 * log(2.0));
  s2 = s2 / sqrt(8.0 * log(2.0));
  x1 = lround(6.0 * s0);
  y1 = lround(6.0 * s1);
  z1 = lround(6.0 * s2);

  VB_Vector x((2 * x1) + 1);
  VB_Vector y((2 * y1) + 1);
  VB_Vector z((2 * z1) + 1);

  for (i = -x1; i <= x1; i++) x(i + x1) = i;
  for (i = -y1; i <= y1; i++) y(i + y1) = i;
  for (i = -z1; i <= z1; i++) z(i + z1) = i;
  for (i = 0; i < (int)x.getLength(); i++)
    x(i) = exp(-pow(x(i), 2) / (2 * pow(s0, 2)));
  for (i = 0; i < (int)y.getLength(); i++)
    y(i) = exp(-pow(y(i), 2) / (2 * pow(s1, 2)));
  for (i = 0; i < (int)z.getLength(); i++)
    z(i) = exp(-pow(z(i), 2) / (2 * pow(s2, 2)));

  xsum = x.getVectorSum();
  ysum = y.getVectorSum();
  zsum = z.getVectorSum();
  for (i = 0; i < (int)x.getLength(); i++) x(i) /= xsum;
  for (i = 0; i < (int)y.getLength(); i++) y(i) /= ysum;
  for (i = 0; i < (int)z.getLength(); i++) z(i) /= zsum;

  if (f_correct)
    conv3dx(cube, x, y, z);
  else
    conv3d(cube, x, y, z);
  return 0;
}

void conv3d(Cube &cube, VB_Vector &kx, VB_Vector &ky, VB_Vector &kz) {
  int i, j, k, pad;

  // conv xy
  pad = kz.getLength() / 2;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      VB_Vector v(cube.dimz + pad);
      // transfer data
      for (k = 0; k < cube.dimz; k++) v[k] = cube.GetValue(i, j, k);
      // convolve
      v.convolve(kz);
      // transfer back
      for (k = 0; k < cube.dimz; k++) cube.SetValue(i, j, k, v[k + pad]);
    }
  }

  // conv yz
  pad = kx.getLength() / 2;
  for (j = 0; j < cube.dimy; j++) {
    for (k = 0; k < cube.dimz; k++) {
      VB_Vector v(cube.dimx + pad);
      // transfer data
      for (i = 0; i < cube.dimx; i++) v[i] = cube.GetValue(i, j, k);
      // convolve
      v.convolve(kx);
      // transfer back
      for (i = 0; i < cube.dimx; i++) cube.SetValue(i, j, k, v[i + pad]);
    }
  }

  // conv xz
  pad = ky.getLength() / 2;
  for (i = 0; i < cube.dimx; i++) {
    for (k = 0; k < cube.dimz; k++) {
      VB_Vector v(cube.dimy + pad);
      // transfer data
      for (j = 0; j < cube.dimy; j++) v[j] = cube.GetValue(i, j, k);
      // convolve
      v.convolve(ky);
      // transfer back
      for (j = 0; j < cube.dimy; j++) cube.SetValue(i, j, k, v[j + pad]);
    }
  }
}

// convolvex() is a convolution function that does not average in 0's

void convolvex(VB_Vector &v1, VB_Vector &kernel) {
  VB_Vector ret(v1.size() + kernel.size() - 1);

  for (size_t k = 0; k < ret.size(); k++) {
    double mass = 0;
    for (size_t j = 0; j <= k; j++) {
      if ((j < v1.size()) && ((k - j) < kernel.size())) {
        ret[k] += v1[j] * kernel[k - j];
        // the following condition needed because v1 is padded with 0s
        if (j < v1.size() - kernel.size() / 2) mass += kernel[k - j];
      }
    }
    if (mass > FLT_MIN) ret[k] = ret[k] / mass;
  }
  v1 = ret;
}

// conv3dx() is just conv3d(), but it calls convolvex() to avoid
// averaging in 0's from outside the volume

void conv3dx(Cube &cube, VB_Vector &kx, VB_Vector &ky, VB_Vector &kz) {
  int i, j, k, pad;

  // conv xy
  pad = kz.getLength() / 2;
  for (i = 0; i < cube.dimx; i++) {
    for (j = 0; j < cube.dimy; j++) {
      VB_Vector v(cube.dimz + pad);
      // transfer data
      for (k = 0; k < cube.dimz; k++) v[k] = cube.GetValue(i, j, k);
      // convolve
      convolvex(v, kz);
      // transfer back
      for (k = 0; k < cube.dimz; k++) cube.SetValue(i, j, k, v[k + pad]);
    }
  }

  // conv yz
  pad = kx.getLength() / 2;
  for (j = 0; j < cube.dimy; j++) {
    for (k = 0; k < cube.dimz; k++) {
      VB_Vector v(cube.dimx + pad);
      // transfer data
      for (i = 0; i < cube.dimx; i++) v[i] = cube.GetValue(i, j, k);
      // convolve
      convolvex(v, kx);
      // transfer back
      for (i = 0; i < cube.dimx; i++) cube.SetValue(i, j, k, v[i + pad]);
    }
  }

  // conv xz
  pad = ky.getLength() / 2;
  for (i = 0; i < cube.dimx; i++) {
    for (k = 0; k < cube.dimz; k++) {
      VB_Vector v(cube.dimy + pad);
      // transfer data
      for (j = 0; j < cube.dimy; j++) v[j] = cube.GetValue(i, j, k);
      // convolve
      convolvex(v, ky);
      // transfer back
      for (j = 0; j < cube.dimy; j++) cube.SetValue(i, j, k, v[j + pad]);
    }
  }
}

// the following function is for online SNR mapping mostly -- for
// off-line we should tend to use ReadTimeSeries, in case of really
// large tes files

void SNRMap(Tes &tes, Cube &cb) {
  // would be nice to have a method to invalidate cube
  if (!tes.data) return;
  if (tes.dimt < 3) return;
  cb.SetVolume(tes.dimx, tes.dimy, tes.dimz, vb_double);
  if (!cb.data) return;
  VB_Vector v;
  double mean, sd;
  for (int i = 0; i < tes.dimx; i++) {
    for (int j = 0; j < tes.dimy; j++) {
      for (int k = 0; k < tes.dimz; k++) {
        if (!(tes.data[tes.voxelposition(i, j, k)])) continue;
        tes.GetTimeSeries(i, j, k);
        mean = tes.timeseries.getVectorMean();
        sd = tes.timeseries.getVariance();
        sd *= sd;
        if (sd < .000000001)
          cb.SetValue(i, j, k, 0.0);
        else
          cb.SetValue(i, j, k, mean / sd);
      }
    }
  }
}

int vbOrientTes(Tes &tes, Tes &outTes, string in, string out, int interleaved) {
  int err = 0;
  Cube reorientedCube;
  Cube initialCube;
  for (int i = 0; i < tes.dimt; i++) {
    err = tes.getCube(i, initialCube);
    if (err) {
      return 5;
    }
    reorientedCube = initialCube;
    err = vbOrient(initialCube, reorientedCube, in, out, interleaved);
    if (err) {
      return 6;
    } else {
      // resize the tes file to match the possibly rotated cube. Only do this
      // once (don't waste the cpu's time.)
      if (i == 0)
        outTes.SetVolume(reorientedCube.dimx, reorientedCube.dimy,
                         reorientedCube.dimz, tes.dimt, tes.datatype);
      err = outTes.SetCube(i, reorientedCube);
      // SetCube returns true(=1) on success, so this makes us check for 1 and
      // reset err = 0 below
      if (err != 1) {
        return 7;
      } else
        err = 0;
    }
  }
  string cornerPositions = reorientedCube.GetHeader("AbsoluteCornerPosition:");
  outTes.WriteHeader("AbsoluteCornerPosition:", cornerPositions);
  for (int dim = 0; dim < 3; dim++) {
    outTes.voxsize[dim] = reorientedCube.voxsize[dim];
    outTes.origin[dim] = reorientedCube.origin[dim];
  }
  return 0;
}

// FIXME make sure the below also sets the orientation code

int vbOrient(Cube &inCube, Cube &outCube, string in, string out,
             int interleaved) {
  // to ensure that the user entered the strings in upper case
  in = vb_toupper(in);
  out = vb_toupper(out);

  if ((in.size() != 3) || (out.size() != 3)) return 1;

  if (validateOrientation(in) < 0) return 2;

  if (validateOrientation(out) < 0) return 3;

  string initOrient(in);
  string desiredOrient(out);

  int srcx = 0;
  int srcy = 0;
  int srcz = 0;
  int initx = 0;
  int inity = 0;
  int initz = 0;
  int xincr = 0;
  int yincr = 0;
  int zincr = 0;
  int xfrom = 0;
  int yfrom = 0;
  int zfrom = 0;
  int newxsize = 0;
  int newysize = 0;
  int newzsize = 0;

  // the initial and desired orientation strings affect the variables below,
  // and, ultimately, how we transcribe the data from the initial cube (cube) to
  // the desired orientation cube (outCube). Briefly, we scan the initial string
  // with each letter from the desired string. Whether the same letter exists in
  // both strings, whether the letter's inverse A<->P, I<->S, L<->R exists, and
  // the location of the letter or its inverse in the initial string tells us
  // which axes in the first cube map to which axes in the second, from which
  // direction we should be copying the voxels, and how many voxels line each
  // side of the cube.

  switch (in.find(desiredOrient[0])) {
    case 0:
      xincr = 1;
      xfrom = 0;
      srcx = initx = 0;
      newxsize = inCube.dimx;
      break;
    case 1:
      xincr = 1;
      xfrom = 1;
      srcx = initx = 0;
      newxsize = inCube.dimy;
      break;
    case 2:
      xincr = 1;
      xfrom = 2;
      srcx = initx = 0;
      newxsize = inCube.dimz;
      break;
    default:
      break;
  }

  switch (in.find(desiredOrient[1])) {
    case 0:
      yincr = 1;
      yfrom = 0;
      srcy = inity = 0;
      newysize = inCube.dimx;
      break;
    case 1:
      yincr = 1;
      yfrom = 1;
      srcy = inity = 0;
      newysize = inCube.dimy;
      break;
    case 2:
      yincr = 1;
      yfrom = 2;
      srcy = inity = 0;
      newysize = inCube.dimz;
      break;
    default:
      break;
  }

  switch (in.find(desiredOrient[2])) {
    case 0:
      zincr = 1;
      zfrom = 0;
      srcz = initz = 0;
      newzsize = inCube.dimx;
      break;
    case 1:
      zincr = 1;
      zfrom = 1;
      srcz = initz = 0;
      newzsize = inCube.dimy;
      break;
    case 2:
      zincr = 1;
      zfrom = 2;
      srcz = initz = 0;
      newzsize = inCube.dimz;
      break;
    default:
      break;
  }

  string reverseInitial(in);
  returnReverseOrientation(reverseInitial);
  switch (reverseInitial.find(desiredOrient[0])) {
    case 0:
      xincr = -1;
      xfrom = 0;
      srcx = initx = inCube.dimx - 1;
      newxsize = inCube.dimx;
      break;
    case 1:
      xincr = -1;
      xfrom = 1;
      srcx = initx = inCube.dimy - 1;
      newxsize = inCube.dimy;
      break;
    case 2:
      xincr = -1;
      xfrom = 2;
      srcx = initx = inCube.dimz - 1;
      newxsize = inCube.dimz;
      break;
    default:
      break;
  }
  switch (reverseInitial.find(desiredOrient[1])) {
    case 0:
      yincr = -1;
      yfrom = 0;
      srcy = inity = inCube.dimx - 1;
      newysize = inCube.dimx;
      break;
    case 1:
      yincr = -1;
      yfrom = 1;
      srcy = inity = inCube.dimy - 1;
      newysize = inCube.dimy;
      break;
    case 2:
      yincr = -1;
      yfrom = 2;
      srcy = inity = inCube.dimz - 1;
      newysize = inCube.dimz;
      break;
    default:
      break;
  }
  switch (reverseInitial.find(desiredOrient[2])) {
    case 0:
      zincr = -1;
      zfrom = 0;
      srcz = initz = inCube.dimx - 1;
      newzsize = inCube.dimx;
      break;
    case 1:
      zincr = -1;
      zfrom = 1;
      srcz = initz = inCube.dimy - 1;
      newzsize = inCube.dimy;
      break;
    case 2:
      zincr = -1;
      zfrom = 2;
      srcz = initz = inCube.dimz - 1;
      newzsize = inCube.dimz;
      break;
    default:
      break;
  }

  // resize the new cube
  if (xfrom == 0 && yfrom == 1 && zfrom == 2)
    outCube.resize(inCube.dimx, inCube.dimy, inCube.dimz);
  if (xfrom == 0 && yfrom == 2 && zfrom == 1)
    outCube.resize(inCube.dimx, inCube.dimz, inCube.dimy);
  if (xfrom == 1 && yfrom == 2 && zfrom == 0)
    outCube.resize(inCube.dimy, inCube.dimz, inCube.dimx);
  if (xfrom == 1 && yfrom == 0 && zfrom == 2)
    outCube.resize(inCube.dimy, inCube.dimx, inCube.dimz);
  if (xfrom == 2 && yfrom == 0 && zfrom == 1)
    outCube.resize(inCube.dimz, inCube.dimx, inCube.dimy);
  if (xfrom == 2 && yfrom == 1 && zfrom == 0)
    outCube.resize(inCube.dimz, inCube.dimy, inCube.dimx);

  string cornerPositions = inCube.GetHeader("AbsoluteCornerPosition:");
  tokenlist corners, newCorners;
  corners.ParseLine(cornerPositions.c_str());

  char position[STRINGLEN];
  double val = 0.0;

  // assign new voxel sizes and a new origin
  if (xincr > 0) {
    outCube.origin[0] = inCube.origin[xfrom];
    newCorners.Add(corners[xfrom]);
  } else {
    outCube.origin[0] = newxsize - inCube.origin[xfrom];
    val = 0.0 - atof(corners[xfrom].c_str());
    sprintf(position, "%f", val);
    newCorners.Add(position);
  }
  if (yincr > 0) {
    outCube.origin[1] = inCube.origin[yfrom];
    newCorners.Add(corners[yfrom]);
  } else {
    outCube.origin[1] = newysize - inCube.origin[yfrom];
    val = 0.0 - atof(corners[yfrom].c_str());
    sprintf(position, "%f", val);
    newCorners.Add(position);
  }
  if (zincr > 0) {
    outCube.origin[2] = inCube.origin[zfrom];
    newCorners.Add(corners[zfrom]);
  } else {
    outCube.origin[2] = newzsize - inCube.origin[zfrom];
    val = 0.0 - atof(corners[zfrom].c_str());
    sprintf(position, "%f", val);
    newCorners.Add(position);
  }

  outCube.WriteHeader("AbsoluteCornerPosition:", newCorners.MakeString());

  outCube.voxsize[0] = inCube.voxsize[xfrom];
  outCube.voxsize[1] = inCube.voxsize[yfrom];
  outCube.voxsize[2] = inCube.voxsize[zfrom];

  // based on the initial and desired orientations, we know how to copy the data
  // from the old cube to the new. In short, we transcibe the voxels from the old
  // cube (cube) to the new cube (outCube) by modifying the old cube's origin
  // (initx, inity, initz) and traversing the old cube in the directions
  // necessary to fetch voxels to populate the new cube in the desired way.

  // cout << "xfrom,src,incr: " << xfrom << " " << srcx << " " << xincr << endl;
  // cout << "yfrom,src,incr: " << yfrom << " " << srcy << " " << yincr << endl;
  // cout << "zfrom,src,incr: " << zfrom << " " << srcz << " " << zincr << endl;

  for (int i = 0; i < inCube.dimx; i++) {
    if (xfrom == 1)
      srcx = initx;
    else if (yfrom == 1)
      srcy = inity;
    else if (zfrom == 1)
      srcz = initz;
    for (int j = 0; j < inCube.dimy; j++) {
      if (xfrom == 2)
        srcx = initx;
      else if (yfrom == 2)
        srcy = inity;
      else if (zfrom == 2)
        srcz = initz;
      for (int k = 0; k < inCube.dimz; k++) {
        if (interleaved) {
          if (zfrom == 2)
            outCube.SetValue(srcx, srcy, interleavedorder(srcz, outCube.dimz),
                             inCube.GetValue(i, j, k));
          else if (yfrom == 2)
            outCube.SetValue(srcx, interleavedorder(srcy, outCube.dimy), srcz,
                             inCube.GetValue(i, j, k));
          else if (xfrom == 2)
            outCube.SetValue(interleavedorder(srcx, outCube.dimx), srcy, srcz,
                             inCube.GetValue(i, j, k));
        } else
          outCube.SetValue(srcx, srcy, srcz, inCube.GetValue(i, j, k));
        if (xfrom == 2)
          srcx += xincr;
        else if (yfrom == 2)
          srcy += yincr;
        else if (zfrom == 2)
          srcz += zincr;
      }
      if (xfrom == 1)
        srcx += xincr;
      else if (yfrom == 1)
        srcy += yincr;
      else if (zfrom == 1)
        srcz += zincr;
    }
    if (xfrom == 0)
      srcx += xincr;
    else if (yfrom == 0)
      srcy += yincr;
    else if (zfrom == 0)
      srcz += zincr;
  }

  return 0;
}

int returnReverseOrientation(string &s) {
  for (unsigned int i = 0; i < s.size(); i++) {
    if (s[i] == 'L')
      s[i] = 'R';
    else if (s[i] == 'R')
      s[i] = 'L';
    else if (s[i] == 'A')
      s[i] = 'P';
    else if (s[i] == 'P')
      s[i] = 'A';
    else if (s[i] == 'I')
      s[i] = 'S';
    else if (s[i] == 'S')
      s[i] = 'I';
    else
      return -1;
  }
  return 0;
}

VBMatrix affine_pitch(VBMatrix &m, double pitch) {
  VB_Vector row(4);
  VBMatrix out(4, 4);
  VBMatrix tr(4, 4);

  row[0] = 1.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = 0.0;
  tr.SetRow(0, row);

  row[0] = 0.0;
  row[1] = cos(pitch);
  row[2] = 0.0 - sin(pitch);
  row[3] = 0.0;
  tr.SetRow(1, row);

  row[0] = 0.0;
  row[1] = sin(pitch);
  row[2] = cos(pitch);
  row[3] = 0.0;
  tr.SetRow(2, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = 1.0;
  tr.SetRow(3, row);
  out = tr;
  out *= m;
  return (out);
}

VBMatrix affine_roll(VBMatrix &m, double roll) {
  VB_Vector row(4);
  VBMatrix out(4, 4);
  VBMatrix tr(4, 4);

  row[0] = cos(roll);
  row[1] = 0.0;
  row[2] = sin(roll);
  row[3] = 0.0;
  tr.SetRow(0, row);

  row[0] = 0.0;
  row[1] = 1.0;
  row[2] = 0.0;
  row[3] = 0.0;
  tr.SetRow(1, row);

  row[0] = 0.0 - sin(roll);
  row[1] = 0.0;
  row[2] = cos(roll);
  row[3] = 0.0;
  tr.SetRow(2, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = 1.0;
  tr.SetRow(3, row);
  out = tr;
  out *= m;
  return (out);
}

VBMatrix affine_yaw(VBMatrix &m, double yaw) {
  VB_Vector row(4);
  VBMatrix out(4, 4);
  VBMatrix tr(4, 4);

  row[0] = cos(yaw);
  row[1] = 0.0 - sin(yaw);
  row[2] = 0.0;
  row[3] = 0.0;
  tr.SetRow(0, row);

  row[0] = sin(yaw);
  row[1] = cos(yaw);
  row[2] = 0.0;
  row[3] = 0.0;
  tr.SetRow(1, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 1.0;
  row[3] = 0.0;
  tr.SetRow(2, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = 1.0;
  tr.SetRow(3, row);
  out = tr;
  out *= m;
  return out;
}

VBMatrix affine_translate(VBMatrix &m, double dx, double dy, double dz) {
  VB_Vector row(4);
  VBMatrix out(4, 4);
  VBMatrix tr(4, 4);

  row[0] = 1.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = dx;
  tr.SetRow(0, row);

  row[0] = 0.0;
  row[1] = 1.0;
  row[2] = 0.0;
  row[3] = dy;
  tr.SetRow(1, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 1.0;
  row[3] = dz;
  tr.SetRow(2, row);

  row[0] = 0.0;
  row[1] = 0.0;
  row[2] = 0.0;
  row[3] = 1.0;
  tr.SetRow(3, row);
  out = tr;
  out *= m;
  return out;
}

// void
// TransformCube(Cube &cb,VBMatrix &mat)
// {
//   // use the existing extent
//   Cube newcube=cb;
//   // for each new voxel, find the old voxel and resample from there
//   VBMatrix rmat;
//   VBMatrix coords(4,1,vb_double);
//   coords.mview(3)=0.0;
//   invert(mat,rmat);
//   for (int i=0; i<cb.dimx; i++) {
//     coords.mview(0)=i;
//     for (int j=0; j<cb.dimy; j++) {
//       coords.mview(1)=j;
//       for (int k=0; k<cb.dimz; k++) {
//         coords.mview(2)=k;
//         // oldcoord=rmat * coords
//         // do nn resample
//       }
//     }
//   }
// }

// createresampledvolume() resamples src into correspondence with
// dest, taking account of q_mag

// THIS IS NOT YET USED ANYWHERE

void createresampledvolume(Cube &dest, Cube &src) {
  double q_mag = 1.0;
  // get corners
  double sx, sy, sz, dx, dy, dz;
  dest.GetCorner(dx, dy, dz);
  src.GetCorner(sx, sy, sz);
  // set start position on src to corner of dest
  sx = ((double)(dx - sx)) / src.voxsize[0];
  sy = ((double)(dy - sy)) / src.voxsize[1];
  sz = ((double)(dz - sz)) / src.voxsize[2];
  // now set sampling interval
  double ix, iy, iz;
  ix = (dest.voxsize[0] / src.voxsize[0]) / q_mag;
  iy = (dest.voxsize[1] / src.voxsize[1]) / q_mag;
  iz = (dest.voxsize[2] / src.voxsize[2]) / q_mag;

  Cube newvolume;
  newvolume.SetVolume((int)round(dest.dimx * q_mag),
                      (int)round(dest.dimy * q_mag),
                      (int)round(dest.dimz * q_mag), vb_float);

  // set current positions to start positions
  double xx = sx;
  double yy = sy;
  double zz = sz;
  for (int i = 0; i < newvolume.dimx; i++) {
    yy = sy;
    for (int j = 0; j < newvolume.dimy; j++) {
      zz = sz;
      for (int k = 0; k < newvolume.dimz; k++) {
        newvolume.SetValue(
            i, j, k,
            src.GetValue((int)round(xx), (int)round(yy), (int)round(zz)));
        zz += iz;
      }
      yy += iy;
    }
    xx += ix;
  }
  src = newvolume;
}

// THE BELOW IS USED TO DO PSEUDO-GAUSSIAN SMOOTHING (DOESN'T INCLUDE EXTRA-MASK
// VOXELS)

// s0/s1/s2 are the fwhm kernel in voxels

int buildGaussianKernel(Cube &cube, double s0, double s1, double s2) {
  short x1, y1, z1;

  if (s0 < 1) s0 = 1;
  if (s1 < 1) s1 = 1;
  if (s2 < 1) s2 = 1;
  s0 /= sqrt(8.0 * log(2.0));
  s1 /= sqrt(8.0 * log(2.0));
  s2 /= sqrt(8.0 * log(2.0));
  x1 = lround(6.0 * s0);
  y1 = lround(6.0 * s1);
  z1 = lround(6.0 * s2);
  cout << x1 << endl;

  VB_Vector x((2 * x1) + 1);
  VB_Vector y((2 * y1) + 1);
  VB_Vector z((2 * z1) + 1);

  for (int i = -x1; i <= x1; i++) x(i + x1) = i;
  for (int i = -y1; i <= y1; i++) y(i + y1) = i;
  for (int i = -z1; i <= z1; i++) z(i + z1) = i;
  for (int i = 0; i < (int)x.getLength(); i++)
    x(i) = exp(-pow(x(i), 2) / (2 * pow(s0, 2)));
  for (int i = 0; i < (int)y.getLength(); i++)
    y(i) = exp(-pow(y(i), 2) / (2 * pow(s1, 2)));
  for (int i = 0; i < (int)z.getLength(); i++)
    z(i) = exp(-pow(z(i), 2) / (2 * pow(s2, 2)));

  x /= x.getVectorSum();
  y /= y.getVectorSum();
  z /= z.getVectorSum();

  Cube kernel(x.size(), y.size(), z.size(), vb_float);
  kernel.zero();
  for (int i = 0; i < kernel.dimx; i++) {
    for (int j = 0; j < kernel.dimy; j++) {
      for (int k = 0; k < kernel.dimz; k++) {
        kernel.SetValue(i, j, k, x(i) * y(j) * z(k));
      }
    }
  }
  cube = kernel;
  return 0;
}

int maskKernel(Cube &kernel, Cube &mask, int x, int y, int z) {
  int cx = kernel.dimx / 2;
  int cy = kernel.dimy / 2;
  int cz = kernel.dimz / 2;
  for (int i = 0; i < kernel.dimx; i++) {
    for (int j = 0; j < kernel.dimy; j++) {
      for (int k = 0; k < kernel.dimz; k++) {
        if (mask.GetValue(x - cx + i, y - cy + j, z = cz + k) == 0.0)
          kernel.SetValue(i, j, k, 0.0);
      }
    }
  }
  double kernelsum = 0.0;
  for (int i = 0; i < kernel.dimx; i++) {
    for (int j = 0; j < kernel.dimy; j++) {
      for (int k = 0; k < kernel.dimz; k++) {
        kernelsum += kernel.GetValue(i, j, k);
      }
    }
  }
  if (kernelsum > 0.0) kernel *= ((double)1.0 / kernelsum);
  return 0;
}

int smooth3D(Cube &cube, Cube &mask, Cube &rawkernel) {
  Cube kernel = rawkernel;
  Cube newcube = cube;
  for (int i = 0; i < cube.dimx; i++) {
    cout << i << endl;
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        maskKernel(kernel, mask, i, j, k);
        newcube.SetValue(i, j, k, getKernelAverage(cube, kernel, i, j, k));
      }
    }
  }
  cube = newcube;
  return 0;
}

double getKernelAverage(Cube &cube, Cube &kernel, int x, int y, int z) {
  int cx = kernel.dimx / 2;
  int cy = kernel.dimy / 2;
  int cz = kernel.dimz / 2;
  double value = 0.0;
  for (int i = 0; i < kernel.dimx; i++) {
    for (int j = 0; j < kernel.dimy; j++) {
      for (int k = 0; k < kernel.dimz; k++) {
        value += cube.GetValue(x - cx + i, y - cy + j, z = cz + k);
      }
    }
  }
  return value;
}

// RESAMPLE-RELATED

void Resample::init() {
  imagename = refname = "";
  x1 = y1 = z1 = 0;
  xstep = ystep = zstep = 1.0;
  nx = 10;
  ny = 10;
  nz = 10;
  method = vb_sinc;
}

Resample::Resample() { init(); }

vector<string> Resample::headerstrings() {
  vector<string> newheaders;
  char tmps[512];

  sprintf(tmps, "resample_x: start %.6f step %.2f count %d", x1, xstep, nx);
  newheaders.push_back(tmps);

  sprintf(tmps, "resample_y: start %.6f step %.2f count %d", y1, ystep, ny);
  newheaders.push_back(tmps);

  sprintf(tmps, "resample_z: start %.6f step %.2f count %d", z1, zstep, nz);
  newheaders.push_back(tmps);

  newheaders.push_back("resample_date: " + timedate());

  return newheaders;
}

Resample::~Resample() {}

void Resample::SetXX(double xx1, double xxstep, int nxx) {
  x1 = xx1;
  xstep = xxstep;
  nx = nxx;
}

void Resample::SetYY(double yy1, double yystep, int nyy) {
  y1 = yy1;
  ystep = yystep;
  ny = nyy;
}

void Resample::SetZZ(double zz1, double zzstep, int nzz) {
  z1 = zz1;
  zstep = zzstep;
  nz = nzz;
}

// int
// Resample::SetAlign(const string mode,const string ref)
// {
//   Cube refcube;
//   if (refcube.ReadFile(refname))
//     return 100;
//   if (mode=="-aa" || mode=="-ax") {
//     if (ref=="origin")
//       x1=(float)mycube->origin[0]-(((float)refcube.origin[0]*refcube.voxsize[0])/mycube->voxsize[0]);
//   }
//   if (mode=="-aa" || mode=="-ay") {
//     if (ref=="origin")
//       y1=(float)mycube->origin[1]-(((float)refcube.origin[1]*refcube.voxsize[1])/mycube->voxsize[1]);
//   }
//   if (mode=="-aa" || mode=="-az") {
//     if (ref=="origin")
//       z1=(float)mycube->origin[2]-(((float)refcube.origin[2]*refcube.voxsize[2])/mycube->voxsize[2]);
//   }
//   return 0;
// }

int Resample::UseZ(Cube &cb, Cube &refcube, double zsize) {
  double ourstart, ourend, refstart, refend;
  // old style: startloc/endloc
  ourstart = strtod(cb.GetHeader("StartLoc:"));
  ourend = strtod(cb.GetHeader("EndLoc:"));
  refstart = strtod(cb.GetHeader("StartLoc:"));
  refend = strtod(cb.GetHeader("EndLoc:"));
  // new style: zrange
  string refzrange = refcube.GetHeader("ZRange:");
  string ourzrange = cb.GetHeader("ZRange:");
  if (refzrange.size()) {
    tokenlist range(refzrange);
    refstart = strtod(range[0]);
    refend = strtod(range[1]);
  }
  if (ourzrange.size()) {
    tokenlist range(ourzrange);
    ourstart = strtod(range[0]);
    ourend = strtod(range[1]);
  }

  if (zsize < .001) zsize = refcube.voxsize[2];
  nx = cb.dimx;
  ny = cb.dimy;
  z1 = (refstart - ourstart) / cb.voxsize[2];
  nz = (int)((fabs(refend - refstart) / zsize) + 0.5) + 1;
  zstep = zsize / cb.voxsize[2];
  return 0;
}

int Resample::UseDims(const Cube &cb, const Cube &refcube) {
  nx = refcube.dimx;
  ny = refcube.dimy;
  nz = refcube.dimz;
  // FIXME is the following what we really want?
  xstep = (double)(cb.dimx) / nx;
  ystep = (double)(cb.dimy) / ny;
  zstep = (double)(cb.dimz) / nz;
  return 0;
}

int Resample::UseSpecifiedDims(const Cube &cb, int dimx, int dimy, int dimz) {
  nx = dimx;
  ny = dimy;
  nz = dimz;
  // FIXME is the following what we really want?
  xstep = (double)(cb.dimx) / nx;
  ystep = (double)(cb.dimy) / ny;
  zstep = (double)(cb.dimz) / nz;
  return 0;
}

int Resample::UseTLHC(const Cube &cb, const Cube &refcube) {
  double ourLR, refLR, ourAP, refAP;
  ourLR = refLR = ourAP = refAP = 0.0;

  // new style: zrange
  string reftlhc = refcube.GetHeader("im_tlhc:");
  string ourtlhc = cb.GetHeader("im_tlhc:");

  if (reftlhc.size()) {
    tokenlist range(reftlhc);
    refLR = strtod(range[0]);
    refAP = strtod(range[1]);
  }
  if (ourtlhc.size()) {
    tokenlist range(ourtlhc);
    ourLR = strtod(range[0]);
    ourAP = strtod(range[1]);
  }

  nx = cb.dimx;
  ny = cb.dimy;
  nz = cb.dimz;
  x1 = y1 = z1 = 0;
  xstep = ystep = zstep = 1.0;

  if (fabs(ourLR - refLR) > 0.001) {
    x1 = (ourLR - refLR) / cb.voxsize[0];
  }
  if (fabs(ourAP - refAP) > 0.001) {
    y1 = (refAP - ourAP) / cb.voxsize[1];
  }
  if (x1 == 0 && y1 == 0) {
    printf("resample: no fov adjustment neeeded\n");
  }
  return 0;
}

int Resample::UseCorner(const Cube &cb, const Cube &refcube) {
  stringstream tmps;
  // first get the corner coordinates of both images
  tokenlist ourline, refline;
  ourline.ParseLine(cb.GetHeader("AbsoluteCornerPosition:"));
  refline.ParseLine(refcube.GetHeader("AbsoluteCornerPosition:"));
  if (ourline.size() != 3) {
    return 101;
  }
  if (refline.size() != 3) {
    return 102;
  }
  double ourpos[3], refpos[3];
  ourpos[0] = strtod(ourline[0]);
  ourpos[1] = strtod(ourline[1]);
  ourpos[2] = strtod(ourline[2]);
  refpos[0] = strtod(refline[0]);
  refpos[1] = strtod(refline[1]);
  refpos[2] = strtod(refline[2]);

  // find the start point
  x1 = (refpos[0] - ourpos[0]) / cb.voxsize[0];
  y1 = (refpos[1] - ourpos[1]) / cb.voxsize[1];
  z1 = (refpos[2] - ourpos[2]) / cb.voxsize[2];
  // the sampling interval should provide for 4x resolution in-plane
  xstep = (refcube.voxsize[0] / 4.0) / cb.voxsize[0];
  ystep = (refcube.voxsize[1] / 4.0) / cb.voxsize[1];
  zstep = refcube.voxsize[2] / cb.voxsize[2];
  // the number of voxels should be the width of the target image
  nx = refcube.dimx * 4;
  ny = refcube.dimy * 4;
  nz = refcube.dimz;
  return 0;
}

int Resample::UseCorner2(const Cube &cb, const Cube &refcube) {
  stringstream tmps;
  // first get the corner coordinates of both images
  tokenlist ourline, refline;
  ourline.ParseLine(cb.GetHeader("AbsoluteCornerPosition:"));
  refline.ParseLine(refcube.GetHeader("AbsoluteCornerPosition:"));
  if (ourline.size() != 3) {
    return 101;
  }
  if (refline.size() != 3) {
    return 102;
  }
  double ourpos[3], refpos[3];
  ourpos[0] = strtod(ourline[0]);
  ourpos[1] = strtod(ourline[1]);
  ourpos[2] = strtod(ourline[2]);
  refpos[0] = strtod(refline[0]);
  refpos[1] = strtod(refline[1]);
  refpos[2] = strtod(refline[2]);

  // find the start point
  x1 = (refpos[0] - ourpos[0]) / cb.voxsize[0];
  y1 = (refpos[1] - ourpos[1]) / cb.voxsize[1];
  z1 = (refpos[2] - ourpos[2]) / cb.voxsize[2];
  // the sampling interval should provide for same resolution in-plane
  xstep = (refcube.voxsize[0]) / cb.voxsize[0];
  ystep = (refcube.voxsize[1]) / cb.voxsize[1];
  zstep = refcube.voxsize[2] / cb.voxsize[2];
  // the number of voxels should be the width of the target image
  nx = refcube.dimx;
  ny = refcube.dimy;
  nz = refcube.dimz;
  return 0;
}

void Resample::SetupDims(Cube &cb, const Cube &ref) {
  x1 = y1 = z1 = 0;
  nx = ref.dimx;
  ny = ref.dimy;
  nz = ref.dimz;
  // FIXME is the following ideal?
  xstep = (double)(cb.dimx) / nx;
  ystep = (double)(cb.dimy) / ny;
  zstep = (double)(cb.dimz) / nz;
  return;
}

void Resample::AdjustCornerAndOrigin(VBImage &im) {
  vector<string> newheader;
  tokenlist args;
  for (int i = 0; i < (int)im.header.size(); i++) {
    args.ParseLine(im.header[i]);
    if (args[0] != "AbsoluteCornerPosition:") newheader.push_back(im.header[i]);
  }
  double x, y, z;
  im.GetCorner(x, y, z);
  x += x1 * im.voxsize[0];
  y += y1 * im.voxsize[1];
  z += z1 * im.voxsize[2];
  stringstream tmps;
  tmps << "AbsoluteCornerPosition: " << x << " " << y << " " << z;
  newheader.push_back(tmps.str());
  im.header = newheader;
  // the origin is easier...
  //   im.origin[0]-=lround(x1);
  //   im.origin[1]-=lround(y1);
  //   im.origin[2]-=lround(z1);
}

int Resample::NNResampleCube(const Cube &mycube, Cube &newcube) {
  int i, j, k;
  newcube.SetVolume(nx, ny, nz, mycube.datatype);
  newcube.voxsize[0] = fabs(xstep * mycube.voxsize[0]);
  newcube.voxsize[1] = fabs(ystep * mycube.voxsize[1]);
  newcube.voxsize[2] = fabs(zstep * mycube.voxsize[2]);
  newcube.origin[0] = lround((mycube.origin[0] - x1) / xstep);
  newcube.origin[1] = lround((mycube.origin[1] - y1) / ystep);
  newcube.origin[2] = lround((mycube.origin[2] - z1) / zstep);
  AdjustCornerAndOrigin(newcube);

  for (k = 0; k < nz; k++) {
    for (i = 0; i < nx; i++) {
      for (j = 0; j < ny; j++) {
        int c1 = lround(x1 + (xstep * i));
        int c2 = lround(y1 + (ystep * j));
        int c3 = lround(z1 + (zstep * k));
        newcube.SetValue(i, j, k, mycube.GetValue(c1, c2, c3));
      }
    }
  }
  return 0;  // no error
}

int Resample::SincResampleCube(const Cube &mycube, Cube &newcube) {
  int i, j, k;

  newcube.SetVolume(nx, ny, nz, mycube.datatype);
  newcube.voxsize[0] = fabs(xstep * mycube.voxsize[0]);
  newcube.voxsize[1] = fabs(ystep * mycube.voxsize[1]);
  newcube.voxsize[2] = fabs(zstep * mycube.voxsize[2]);
  newcube.origin[0] = lround((mycube.origin[0] - x1) / xstep);
  newcube.origin[1] = lround((mycube.origin[1] - y1) / ystep);
  newcube.origin[2] = lround((mycube.origin[2] - z1) / zstep);
  AdjustCornerAndOrigin(newcube);

  VB_Vector c1(1), c2(1), c3(1), out(1);

  for (k = 0; k < nz; k++) {
    for (i = 0; i < nx; i++) {
      for (j = 0; j < ny; j++) {
        c1(0) = x1 + (xstep * i) + 1;  // +1 because the algorithm 1-indexes
        c2(0) = y1 + (ystep * j) + 1;
        c3(0) = z1 + (zstep * k) + 1;
        switch (mycube.datatype) {
          case vb_byte:
            resample_sinc(1, (unsigned char *)mycube.data, out, c1, c2, c3,
                          mycube.dimx, mycube.dimy, mycube.dimz, 5, 0.0, 1.0);
            break;
          case vb_short:
            resample_sinc(1, (int16 *)mycube.data, out, c1, c2, c3, mycube.dimx,
                          mycube.dimy, mycube.dimz, 5, 0.0, 1.0);
            break;
          case vb_long:
            resample_sinc(1, (int32 *)mycube.data, out, c1, c2, c3, mycube.dimx,
                          mycube.dimy, mycube.dimz, 5, 0.0, 1.0);
            break;
          case vb_float:
            resample_sinc(1, (float *)mycube.data, out, c1, c2, c3, mycube.dimx,
                          mycube.dimy, mycube.dimz, 5, 0.0, 1.0);
            break;
          case vb_double:
            resample_sinc(1, (double *)mycube.data, out, c1, c2, c3,
                          mycube.dimx, mycube.dimy, mycube.dimz, 5, 0.0, 1.0);
            break;
        }
        newcube.SetValue(i, j, k, out(0));
      }
    }
  }

  return 0;  // no error
}

template <class T>
void resample_sinc(int m, T *vol, VB_Vector &out, const VB_Vector &x,
                   const VB_Vector &y, const VB_Vector &z, int dimx, int dimy,
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

// Generate a sinc lookup table with a Hanning filter envelope
void make_lookup(double coord, int nn, int dim, int *d1, double *table,
                 double **ptpend) {
  register int d2, d, fcoord;
  register double *tp, *tpend, dtmp;

  if (fabs(coord - rint(coord)) < 0.00001) {
    /* Close enough to use nearest neighbour */
    *d1 = (int)rint(coord);
    if (*d1 < 1 || *d1 > dim) /* Pixel location outside image */
      *ptpend = table - 1;
    else {
      table[0] = 1.0;
      *ptpend = table;
    }
  } else {
    fcoord = (int)floor(coord);
    *d1 = fcoord - nn;
    if (*d1 < 1) *d1 = 1;
    d2 = fcoord + nn;
    if (d2 > dim) d2 = dim;

    *ptpend = tpend = table + (d2 - *d1);
    d = *d1, tp = table;
    while (tp <= tpend) {
      dtmp = PI * (coord - (double)(d++));
      *(tp++) = sin(dtmp) / dtmp * 0.5 * (1.0 + cos(dtmp / nn));
    }
  }
}

void zero_smallregions(Cube &cb, double thresh) {
  vector<VBRegion> rlist;
  vector<VBRegion>::iterator rr;
  vector<VBVoxel>::iterator vv;
  rlist = findregions(cb, vb_ne, 0.0);
  pair<uint32, double> myvox;
  for (rr = rlist.begin(); rr != rlist.end(); rr++) {
    if (rr->size() < thresh) {
      for (VI myvox = rr->begin(); myvox != rr->end(); myvox++) {
        cb.setValue(myvox->first, 0.0);
      }
    }
  }
}

void rotatecube(Cube &cb, float pitch, float roll, float yaw) {
  VBMatrix transform(4, 4);
  transform.ident();
  if (pitch != 0.0) {
    VBMatrix tmp(4, 4);
    float radians = (pitch / 180.0) * PI;
    tmp.ident();
    tmp.set(1, 1, cos(radians));
    tmp.set(2, 1, sin(radians));
    tmp.set(1, 2, 0.0 - sin(radians));
    tmp.set(2, 2, cos(radians));
    tmp *= transform;
    transform = tmp;
  }
  if (roll != 0.0) {
    VBMatrix tmp(4, 4);
    float radians = (roll / 180.0) * PI;
    tmp.ident();
    tmp.set(0, 0, cos(radians));
    tmp.set(2, 0, 0.0 - sin(radians));
    tmp.set(0, 2, sin(radians));
    tmp.set(2, 2, cos(radians));
    tmp *= transform;
    transform = tmp;
  }
  if (yaw != 0.0) {
    VBMatrix tmp(4, 4);
    float radians = (yaw / 180.0) * PI;
    tmp.ident();
    tmp.set(0, 0, cos(radians));
    tmp.set(1, 0, sin(radians));
    tmp.set(0, 1, 0.0 - sin(radians));
    tmp.set(1, 1, cos(radians));
    tmp *= transform;
    transform = tmp;
  }
  Cube mycube = cb;
  mycube *= 0;

  VB_Vector c1(1), c2(1), c3(1), out(1);
  for (int i = 0; i < mycube.dimx; i++) {
    for (int j = 0; j < mycube.dimy; j++) {
      for (int k = 0; k < mycube.dimz; k++) {
        VBMatrix oldcoord(4, 1), oldcoord2;

        oldcoord.set(0, 0, (double)i - cb.origin[0]);
        oldcoord.set(1, 0, (double)j - cb.origin[1]);
        oldcoord.set(2, 0, (double)k - cb.origin[2]);
        //         oldcoord.set(0,0,(double)i);
        //         oldcoord.set(1,0,(double)j);
        //         oldcoord.set(2,0,(double)k);

        oldcoord.set(3, 0, 1);
        oldcoord2 = transform;
        oldcoord2 *= oldcoord;
        c1(0) =
            oldcoord2(0, 0) + 1 +
            cb.origin[0];  // +1 because resample_sinc expects 1-indexed coords
        c2(0) = oldcoord2(1, 0) + 1 + cb.origin[1];
        c3(0) = oldcoord2(2, 0) + 1 + cb.origin[2];
        // c1(0)=i+1;
        // c2(0)=j+1;
        // c3(0)=k+1;
        switch (mycube.datatype) {
          case vb_byte:
            resample_sinc(1, (unsigned char *)cb.data, out, c1, c2, c3, cb.dimx,
                          cb.dimy, cb.dimz, 5, 0.0, 1.0);
            break;
          case vb_short:
            resample_sinc(1, (int16 *)cb.data, out, c1, c2, c3, cb.dimx,
                          cb.dimy, cb.dimz, 5, 0.0, 1.0);
            break;
          case vb_long:
            resample_sinc(1, (int32 *)cb.data, out, c1, c2, c3, cb.dimx,
                          cb.dimy, cb.dimz, 5, 0.0, 1.0);
            break;
          case vb_float:
            resample_sinc(1, (float *)cb.data, out, c1, c2, c3, cb.dimx,
                          cb.dimy, cb.dimz, 5, 0.0, 1.0);
            break;
          case vb_double:
            resample_sinc(1, (double *)cb.data, out, c1, c2, c3, cb.dimx,
                          cb.dimy, cb.dimz, 5, 0.0, 1.0);
            break;
        }
        mycube.SetValue(i, j, k, out(0));
      }
    }
  }
  cb = mycube;
}

// only consider voxels in the xyzradii, but test using the plain
// "radius"

void enlarge(Cube &cb, uint32 radius, uint32 xradius, uint32 yradius,
             uint32 zradius) {
  // vector hit list of voxels to color
  vector<VBVoxel> hitlist;
  int32 xx, yy, zz, x1, x2, y1, y2, z1, z2;
  for (int v = 0; v < cb.dimx * cb.dimy * cb.dimz; v++) {
    if (!cb.testValue(v)) continue;
    cb.getXYZ(xx, yy, zz, v);
    x1 = xx - xradius;
    if (x1 < 0) x1 = 0;
    y1 = yy - yradius;
    if (y1 < 0) y1 = 0;
    z1 = zz - zradius;
    if (z1 < 0) z1 = 1;
    x2 = xx + xradius;
    if (x2 > cb.dimx - 1) x2 = cb.dimz - 1;
    y2 = yy + yradius;
    if (y2 > cb.dimy - 1) y2 = cb.dimy - 1;
    z2 = zz + zradius;
    if (z2 > cb.dimz - 1) z2 = cb.dimz - 1;
    for (int i = x1; i <= x2; i++) {
      for (int j = y1; j <= y2; j++) {
        for (int k = z1; k <= z2; k++) {
          double dist = voxeldistance(i, j, k, xx, yy, zz);
          if (!(radius - dist < FLT_MIN)) continue;
          hitlist.push_back(VBVoxel(i, j, k));
        }
      }
    }
  }
  vbforeach(VBVoxel & vv, hitlist) cb.setValue(vv.x, vv.y, vv.z, 1.0);
}
