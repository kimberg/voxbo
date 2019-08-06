
// smooth.cpp
// smoothing and related routines for use in realignment and normalization
// new code Copyright (c) 1998-2002 by The VoxBo Development Team
// based on original MATLAB code for SPM by John Ashburner

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

CrunchCube *dan_smooth_image(CrunchCube *map, int s0, int s1, int s2) {
  CrunchCube *V;
  RowVector S(3), ijk(3);
  Matrix mtmp;
  int i, j, k;
  double xsum, ysum, zsum;
  short x1, y1, z1;

  S(0) = s0;
  S(1) = s1;
  S(2) = s2;
  // compute convolution parameters
  S(0) /= map->voxsize[0];
  S(1) /= map->voxsize[1];
  S(2) /= map->voxsize[2];
  if (S(0) < 1) S(0) = 1;
  if (S(1) < 1) S(1) = 1;
  if (S(2) < 1) S(2) = 1;
  S(0) = S(0) / sqrt(8.0 * log(2.0));
  S(1) = S(1) / sqrt(8.0 * log(2.0));
  S(2) = S(2) / sqrt(8.0 * log(2.0));
  x1 = (short)round(6.0 * S(0));
  y1 = (short)round(6.0 * S(1));
  z1 = (short)round(6.0 * S(2));

  RowVector x((2 * x1) + 1);
  RowVector y((2 * y1) + 1);
  RowVector z((2 * z1) + 1);
  for (i = -x1; i <= x1; i++) x(i + x1) = i;
  for (i = -y1; i <= y1; i++) y(i + y1) = i;
  for (i = -z1; i <= z1; i++) z(i + z1) = i;
  for (i = 0; i < x.length(); i++)
    x(i) = exp(-pow(x(i), 2) / (2 * pow(S(0), 2)));
  for (i = 0; i < y.length(); i++)
    y(i) = exp(-pow(y(i), 2) / (2 * pow(S(1), 2)));
  for (i = 0; i < z.length(); i++)
    z(i) = exp(-pow(z(i), 2) / (2 * pow(S(2), 2)));

  xsum = sum(x);
  ysum = sum(y);
  zsum = sum(z);
  for (i = 0; i < x.length(); i++) x(i) /= xsum;
  for (i = 0; i < y.length(); i++) y(i) /= ysum;
  for (i = 0; i < z.length(); i++) z(i) /= zsum;

  i = (x.length() - 1) / 2;
  j = (y.length() - 1) / 2;
  k = (z.length() - 1) / 2;

  ijk(0) = -i;
  ijk(1) = -j;
  ijk(2) = -k;

  V = new CrunchCube(map);
  dan_conv_image(map, V, x, y, z, ijk);
  mtmp = dan_get_space_image(map);
  dan_get_space_image(V, mtmp);

  return V;
}

short fixshort(double tmp) {
  if (tmp > 32767.0) tmp = 32767.0;
  if (tmp < -32768.0) tmp = -32768.0;
  return ((short)round(tmp));
}

// dan_conv_image
// applies the gaussian filter (computed in dan_smooth()) to the provided image

void dan_conv_image(CrunchCube *map, CrunchCube *newmap, const RowVector &filtx,
                    const RowVector &filty, const RowVector &filtz,
                    const RowVector &offsets) {
  int xdim, ydim, zdim;
  unsigned char *Vol;
  double *tmp;

  xdim = (int)(map->dimx);
  ydim = (int)(map->dimy);
  zdim = (int)(map->dimz);

  if (!map->data || !newmap->data) {
    cerr << "bad map or newmap came into conv_image()" << endl;
    exit(0);
  }
  Vol = map->data;

  // Vol = data in memory
  // xdim, ydim, zdim

  tmp = new double[xdim * ydim * zdim];
  if (!tmp) {
    cerr << "failed to allocate space for an image" << endl;
    exit(0);
  }
  for (int i = 0; i < xdim * ydim * zdim; i++) tmp[i] = 0.0;

  if (convxyz(Vol, abs(xdim), abs(ydim), abs(zdim), filtx, filty, filtz,
              filtx.length(), filty.length(), filtz.length(),
              (int)floor(offsets(0)), (int)floor(offsets(1)),
              (int)floor(offsets(2)), (FILE *)0, map->datatype, tmp) != 0) {
    fprintf(stderr, "error in conv_image - convxyz returned an error code\n");
  }

  if (map->datatype == vb_long) {
    for (int i = 0; i < newmap->voxels; i++)
      ((int *)newmap->data)[i] = (int)round(tmp[i]);
  } else if (map->datatype == vb_short) {
    for (int i = 0; i < newmap->voxels; i++)
      ((short *)(newmap->data))[i] = (short)round(tmp[i]);
  } else if (map->datatype == vb_byte) {
    for (int i = 0; i < newmap->voxels; i++)
      ((unsigned char *)(newmap->data))[i] = (unsigned char)(tmp[i] + 0.5);
  } else if (map->datatype == vb_float) {
    for (int i = 0; i < newmap->voxels; i++)
      ((float *)(newmap->data))[i] = (float)tmp[i];
  } else if (map->datatype == vb_double) {
    for (int i = 0; i < newmap->voxels; i++)
      ((float *)(newmap->data))[i] = tmp[i];
  } else {
    fprintf(stderr, "error in conv_image: unrecognized datatype.\n");
  }
  delete[] tmp;
}

int convxyz(unsigned char *vol, int xdim, int ydim, int zdim,
            const RowVector &filtx, const RowVector &filty,
            const RowVector &filtz, int fxdim, int fydim, int fzdim, int xoff,
            int yoff, int zoff, FILE *fp, int datatype, double *ovol) {
  double *tmp, *buff, **sortedv, *obuf;
  int xy, z, k, fstart, fend, startz, endz;

  if (fp != (FILE *)0) {
    cerr << "bogus args to convxyz" << endl;
    exit(0);
  }
  tmp = (double *)calloc(xdim * ydim * fzdim, sizeof(double));
  buff = (double *)calloc(((ydim > xdim) ? ydim : xdim), sizeof(double));
  obuf = (double *)calloc(xdim * ydim, sizeof(double));
  sortedv = (double **)calloc(fzdim, sizeof(double *));

  startz = ((fzdim + zoff - 1 < 0) ? fzdim + zoff - 1 : 0);
  endz = zdim + fzdim + zoff - 1;

  if (!buff || !tmp || !obuf || !sortedv) {
    cerr << "Serious allocation problem in convxyz" << endl;
    return 0;
  }
  for (z = startz; z < endz; z++) {
    double sum2 = 0.0;

    if (z >= 0 && z < zdim) {
      convxy(vol + (z * xdim * ydim * get_datasize(datatype)), xdim, ydim,
             filtx, filty, fxdim, fydim, xoff, yoff,
             tmp + ((z % fzdim) * xdim * ydim), buff, datatype);
    }
    if (z - fzdim - zoff + 1 >= 0 && z - fzdim - zoff + 1 < zdim) {
      fstart = ((z >= zdim) ? z - zdim + 1 : 0);
      fend = ((z - fzdim < 0) ? z + 1 : fzdim);

      for (k = 0; k < fzdim; k++) {
        int z1 = (((z - k) % fzdim) + fzdim) % fzdim;
        sortedv[k] = &(tmp[z1 * xdim * ydim]);
      }

      for (k = fstart, sum2 = 0.0; k < fend; k++) sum2 += filtz(k);

      if (sum2) {
        for (xy = 0; xy < xdim * ydim; xy++) {
          double sum1 = 0.0;
          for (k = fstart; k < fend; k++) sum1 += filtz(k) * sortedv[k][xy];
          obuf[xy] = sum1 / sum2;
        }
      } else
        for (xy = 0; xy < xdim * ydim; xy++) obuf[xy] = 0.0;
      switch (datatype) {
        case vb_byte:
          for (xy = 0; xy < xdim * ydim; xy++) {
            if (obuf[xy] > 255.0) obuf[xy] = 255.0;
            if (obuf[xy] < 0.0) obuf[xy] = 0.0;
          }
          break;
        case vb_short:
          for (xy = 0; xy < xdim * ydim; xy++) {
            if (obuf[xy] > 32767.0) obuf[xy] = 32767.0;
            if (obuf[xy] < -32768.0) obuf[xy] = -32768.0;
          }
          break;
        case vb_long:
          break;
        case vb_float:
          break;
        case vb_double:
          break;
        default:
          cerr << "Unrecognized type in convxyz() in smooth.cpp." << endl;
          break;
      }

      for (xy = 0; xy < xdim * ydim; xy++) {
        *(ovol++) = obuf[xy];
      }
    }
  }
  free((char *)tmp);
  free((char *)buff);
  free((char *)obuf);
  free((char *)sortedv);
  return (0);
}

void convxy(unsigned char *pl, int xdim, int ydim, const RowVector &filtx,
            const RowVector &filty, int fxdim, int fydim, int xoff, int yoff,
            double *out, double *buff, int datatype) {
  double *rs;
  int x, y, k;

  rs = out;
  for (y = 0; y < ydim; y++) {
    switch (datatype) {
      case vb_byte:
        for (x = 0; x < xdim; x++)
          buff[x] = ((unsigned char *)pl)[x + y * xdim];
        break;
      case vb_short:
        for (x = 0; x < xdim; x++) buff[x] = ((short *)pl)[x + y * xdim];
        break;
      case vb_long:
        for (x = 0; x < xdim; x++) buff[x] = ((int *)pl)[x + y * xdim];
        break;
      case vb_float:
        for (x = 0; x < xdim; x++) buff[x] = ((float *)pl)[x + y * xdim];
        break;
      case vb_double:
        for (x = 0; x < xdim; x++) buff[x] = ((double *)pl)[x + y * xdim];
        break;
      default:
        cerr << "Unrecognize type in convxy() in smooth.cpp." << endl;
        break;
    }
    for (x = 0; x < xdim; x++) {
      double sum1 = 0.0;
      int fstart, fend;

      fstart = ((x - xoff >= xdim) ? x - xdim - xoff + 1 : 0);
      fend = ((x - (xoff + fxdim) < 0) ? x - xoff + 1 : fxdim);
      for (k = fstart; k < fend; k++) sum1 += buff[x - xoff - k] * filtx(k);
      rs[x] = sum1;
    }
    rs += xdim;
  }
  for (x = 0; x < xdim; x++) {
    for (y = 0; y < ydim; y++) buff[y] = out[x + y * xdim];

    for (y = 0; y < ydim; y++) {
      double sum1 = 0.0;
      int fstart, fend;

      fstart = ((y - yoff >= ydim) ? y - ydim - yoff + 1 : 0);
      fend = ((y - (yoff + fydim) < 0) ? y - yoff + 1 : fydim);

      for (k = fstart; k < fend; k++) sum1 += buff[y - yoff - k] * filty(k);
      out[y * xdim + x] = sum1;
    }
    rs += xdim;
  }
}

// below adapted from: spm_vol_utils.c 1.5 (c) John Ashburner 96/07/19";

// Generate a sinc lookup table with a Hanning filter envelope
void make_lookup(double coord, int nn, int dim, int *d1, double *table,
                 double **ptpend) {
  register int d2, d, fcoord;
  register double *tp, *tpend, dtmp;

  if (fabs(coord - round(coord)) < 0.00001) {
    /* Close enough to use nearest neighbour */
    *d1 = (int)round(coord);
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

// Sinc resampling
int slice_int_sinc(const Matrix &mat, double *image, int xdim1, int ydim1,
                   int *vol, int xdim2, int ydim2, int zdim2, int nn,
                   double background) {
  int dim1xdim2 = xdim2 * ydim2;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];
  double y, dx3, dy3, dz3, ds3;
  double x4, y4, z4;
  double dat, *tp1, *tp1end, *tp2end, *tp3end;
  int *dp1, *dp2;
  register double dat3, *tp3;
  register int *dp3;

  dx3 = mat(0, 0);
  dy3 = mat(1, 0);
  dz3 = mat(2, 0);
  ds3 = mat(3, 0);

  vol -= (1 + xdim2 * (1 + ydim2));

  for (y = 1; y <= ydim1; y++) {
    double x;
    double x3 = mat(0, 3) + y * mat(0, 1);
    double y3 = mat(1, 3) + y * mat(1, 1);
    double z3 = mat(2, 3) + y * mat(2, 1);
    double s3 = mat(3, 3) + y * mat(3, 1);

    for (x = 1; x <= xdim1; x++) {
      // double x4,y4,z4;
      s3 += ds3;
      if (s3 == 0.0) return (-1);
      x4 = (x3 += dx3) / s3;
      y4 = (y3 += dy3) / s3;
      z4 = (z3 += dz3) / s3;

      if (z4 >= 1.0 - TINY && z4 <= (double)zdim2 + TINY && y4 >= 1.0 - TINY &&
          y4 <= (double)ydim2 + TINY && x4 >= 1.0 - TINY &&
          x4 <= (double)xdim2 + TINY) {
        // int *dp1;
        // double dat=0.0, *tp1, *tp1end, *tp2end, *tp3end;
        dat = 0.0;

        make_lookup(x4, nn, xdim2, &dx1, tablex, &tp3end);
        make_lookup(y4, nn, ydim2, &dy1, tabley, &tp2end);
        make_lookup(z4, nn, zdim2, &dz1, tablez, &tp1end);

        tp1 = tablez;
        dy1 *= xdim2;
        dp1 = vol + dim1xdim2 * dz1;

        while (tp1 <= tp1end) {
          // int *dp2 = dp1 + dy1;
          dp2 = dp1 + dy1;
          double dat2 = 0.0, *tp2 = tabley;
          while (tp2 <= tp2end) {
            // register double dat3 = 0.0, *tp3 = tablex;
            // register int *dp3 = dp2 + dx1;
            dat3 = 0.0;
            tp3 = tablex;
            dp3 = dp2 + dx1;
            while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
            dat2 += dat3 * *(tp2++);
            dp2 += xdim2;
          }
          dat += dat2 * *(tp1++);
          dp1 += dim1xdim2;
        }
        *(image++) = dat;
      } else
        *(image++) = background;
    }
  }
  return (0);
}

int slice_short_sinc(const Matrix &mat, double *image, int xdim1, int ydim1,
                     short *vol, int xdim2, int ydim2, int zdim2, int nn,
                     double background) {
  register double dat3, *tp3;
  register short *dp3;
  register double dat2, *tp2;
  register short *dp1, *dp2;
  int dim1xdim2 = xdim2 * ydim2;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];
  double y, dx3, dy3, dz3, ds3;
  // new ones
  double x, x3, y3, z3, s3, x4, y4, z4, dat, *tp1, *tp1end, *tp2end, *tp3end;

  dx3 = mat(0, 0);
  dy3 = mat(1, 0);
  dz3 = mat(2, 0);
  ds3 = mat(3, 0);

  vol -= (1 + xdim2 * (1 + ydim2));

  for (y = 1; y <= ydim1; y++) {
    x3 = mat(0, 3) + y * mat(0, 1);
    y3 = mat(1, 3) + y * mat(1, 1);
    z3 = mat(2, 3) + y * mat(2, 1);
    s3 = mat(3, 3) + y * mat(3, 1);

    for (x = 1; x <= xdim1; x++) {
      s3 += ds3;
      if (s3 == 0.0) return (-1);
      x4 = (x3 += dx3) / s3;
      y4 = (y3 += dy3) / s3;
      z4 = (z3 += dz3) / s3;

      if (z4 >= 1.0 - TINY && z4 <= (double)zdim2 + TINY && y4 >= 1.0 - TINY &&
          y4 <= (double)ydim2 + TINY && x4 >= 1.0 - TINY &&
          x4 <= (double)xdim2 + TINY) {
        dat = 0.0;

        make_lookup(x4, nn, xdim2, &dx1, tablex, &tp3end);
        make_lookup(y4, nn, ydim2, &dy1, tabley, &tp2end);
        make_lookup(z4, nn, zdim2, &dz1, tablez, &tp1end);

        tp1 = tablez;
        dy1 *= xdim2;
        dp1 = vol + dim1xdim2 * dz1;

        while (tp1 <= tp1end) {
          dp2 = dp1 + dy1;
          dat2 = 0.0;
          tp2 = tabley;
          while (tp2 <= tp2end) {
            dat3 = 0.0;
            tp3 = tablex;
            dp3 = dp2 + dx1;
            while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
            dat2 += dat3 * *(tp2++);
            dp2 += xdim2;
          }
          dat += dat2 * *(tp1++);
          dp1 += dim1xdim2;
        }
        *(image++) = dat;
      } else
        *(image++) = background;
    }
  }
  return (0);
}

int slice_short_sinc_threaded(const Matrix &mat, double *image, int xdim1,
                              int ydim1, short *vol, int xdim2, int ydim2,
                              int zdim2, int nn, double background) {
  register double dat3, *tp3;
  register short *dp3;
  register double dat2, *tp2;
  register short *dp1, *dp2;
  int dim1xdim2 = xdim2 * ydim2;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];
  double y, dx3, dy3, dz3, ds3;
  // new ones
  double x, x3, y3, z3, s3, x4, y4, z4, dat, *tp1, *tp1end, *tp2end, *tp3end;

  dx3 = mat(0, 0);
  dy3 = mat(1, 0);
  dz3 = mat(2, 0);
  ds3 = mat(3, 0);

  vol -= (1 + xdim2 * (1 + ydim2));

  for (y = 1; y <= ydim1; y++) {
    x3 = mat(0, 3) + y * mat(0, 1);
    y3 = mat(1, 3) + y * mat(1, 1);
    z3 = mat(2, 3) + y * mat(2, 1);
    s3 = mat(3, 3) + y * mat(3, 1);

    for (x = 1; x <= xdim1; x++) {
      s3 += ds3;
      if (s3 == 0.0) return (-1);
      x4 = (x3 += dx3) / s3;
      y4 = (y3 += dy3) / s3;
      z4 = (z3 += dz3) / s3;

      if (z4 >= 1.0 - TINY && z4 <= (double)zdim2 + TINY && y4 >= 1.0 - TINY &&
          y4 <= (double)ydim2 + TINY && x4 >= 1.0 - TINY &&
          x4 <= (double)xdim2 + TINY) {
        dat = 0.0;

        make_lookup(x4, nn, xdim2, &dx1, tablex, &tp3end);
        make_lookup(y4, nn, ydim2, &dy1, tabley, &tp2end);
        make_lookup(z4, nn, zdim2, &dz1, tablez, &tp1end);

        tp1 = tablez;
        dy1 *= xdim2;
        dp1 = vol + dim1xdim2 * dz1;

        while (tp1 <= tp1end) {
          dp2 = dp1 + dy1;
          dat2 = 0.0;
          tp2 = tabley;
          while (tp2 <= tp2end) {
            dat3 = 0.0;
            tp3 = tablex;
            dp3 = dp2 + dx1;
            while (tp3 <= tp3end) dat3 += *(dp3++) * *(tp3++);
            dat2 += dat3 * *(tp2++);
            dp2 += xdim2;
          }
          dat += dat2 * *(tp1++);
          dp1 += dim1xdim2;
        }
        *(image++) = dat;
      } else
        *(image++) = background;
    }
  }
  return (0);
}

// dan_get_space_image()
// two functions, one takes a matrix, one doesn't

Matrix dan_get_space_image(CrunchCube *map) {
  Matrix M(4, 4);
  RowVector off(3), orig(3);

  if (map->savedM.cols() == 4) {
    return map->savedM;
  }

  orig(0) = map->origin[0];
  orig(1) = map->origin[1];
  orig(2) = map->origin[2];

  if ((int)orig(0) == 0 && (int)orig(1) == 0 && (int)orig(2) == 0) {
    orig(0) = map->dimx / 2.0;
    orig(1) = map->dimy / 2.0;
    orig(2) = map->dimz / 2.0;
  }

  off(0) = map->voxsize[0] * -1.0;
  off(1) = map->voxsize[1] * -1.0;
  off(2) = map->voxsize[2] * -1.0;
  off = product(off, orig);

  M(0, 0) = map->voxsize[0];
  M(0, 1) = 0;
  M(0, 2) = 0;
  M(0, 3) = off(0);
  M(1, 0) = 0;
  M(1, 1) = map->voxsize[1];
  M(1, 2) = 0;
  M(1, 3) = off(1);
  M(2, 0) = 0;
  M(2, 1) = 0;
  M(2, 2) = map->voxsize[2];
  M(2, 3) = off(2);
  M(3, 0) = 0;
  M(3, 1) = 0;
  M(3, 2) = 0;
  M(3, 3) = 1;
  return M;
}

Matrix dan_get_space_image(CrunchCube *map, const Matrix &matrix) {
  Matrix M(4, 4), mt(4, 4);
  RowVector off(3), vx(3);
  int i, j;
  double sum;

  // delete the matrix if it exists
  map->savedM.resize(0, 0);

  M = matrix;
  vx(0) = abs(M(0, 0));
  vx(1) = abs(M(1, 1));
  vx(2) = abs(M(2, 2));
  off(0) = -(vx(0)) * round(-M(0, 3) / vx(0));
  off(1) = -(vx(1)) * round(-M(1, 3) / vx(1));
  off(2) = -(vx(2)) * round(-M(2, 3) / vx(2));
  mt(0, 0) = vx(0);
  mt(0, 1) = 0;
  mt(0, 2) = 0;
  mt(0, 3) = off(0);
  mt(1, 0) = 0;
  mt(1, 1) = vx(1);
  mt(1, 2) = 0;
  mt(1, 3) = off(1);
  mt(2, 0) = 0;
  mt(2, 1) = 0;
  mt(2, 2) = vx(2);
  mt(2, 3) = off(2);
  mt(3, 0) = 0;
  mt(3, 1) = 0;
  mt(3, 2) = 0;
  mt(3, 3) = 1;
  sum = 0.0;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++) sum += pow(matrix(i, j) * mt(i, j), 2);
  if (sum > (double)(FLT_MIN * FLT_MIN * 12.0)) map->savedM = M;
  return M;
}
