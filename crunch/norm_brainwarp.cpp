
// spm_brainwarp.c 1.8 (c) John Ashburner MRCCU/FIL 96/08/08";

#include <math.h>
#include "vbcrunch.h"
extern double floor(), fabs();

// INPUTS
// T[3*nz*ny*nx + ni*4] - current transform

// dt2           - data type of image to normalize
// scale2        - scale factor of image to normalize
// dim2[3]       - x, y & z dimensions of image to normalize
// dat2[dim2[2]*dim2[1]*dim2[0]]     - voxels of image to normalize

// ni            - number of templates
// dt1[ni]       - data types of templates
// scale1[ni]    - scale factors of templates
// dim1[3]       - x, y & z dimensions of templates
// dat1[ni*dim1[2]*dim1[1]*dim1[0]] - voxels of templates

// nx            - number of basis functions in x
// BX[dim1[0]*nx]- basis functions in x
// dBX[dim1[0]*nx]- derivatives of basis functions in x
// ny            - number of basis functions in y
// BY[dim1[1]*ny]- basis functions in y
// dBY[dim1[1]*ny]- derivatives of basis functions in y
// nz            - number of basis functions in z
// BZ[dim1[2]*nz]- basis functions in z
// dBZ[dim1[2]*nz]- derivatives of basis functions in z

// M[4*4]        - transformation matrix
// samp[3]       - frequency of sampling template.

// OUTPUTS
// alpha[(3*nz*ny*nx+ni*4)^2] - A'A
//  beta[(3*nz*ny*nx+ni*4)]   - A'b
// pss[1*1]                   - sum of squares difference
// pnsamp[1*1]                - number of voxels sampled

void mrqcof(double T[], double alpha[], double beta[], double pss[], int dt2,
            double scale2, int dim2[], unsigned char dat2[], int ni, int dt1[],
            double scale1[], int dim1[], unsigned char *dat1[], int nx,
            double BX[], double dBX[], int ny, double BY[], double dBY[],
            int nz, double BZ[], double dBZ[], double M[], int samp[],
            int *pnsamp) {
  int i1, i2, s0[3], x1, x2, y1, y2, z1, z2, m1, m2, nsamp = 0, ni4;
  double dvds0[3], dvds1[3], *dvdt, s2[3], *ptr1, *ptr2, *Tz, *Ty, tmp, *betaxy,
      *betax, *alphaxy, *alphax, ss = 0.0, *scale1a;

  double *Jz[3][3], *Jy[3][3], J[3][3];
  double *bz3[3], *by3[3], *bx3[3];

  bx3[0] = dBX;
  bx3[1] = BX;
  bx3[2] = BX;
  by3[0] = BY;
  by3[1] = dBY;
  by3[2] = BY;
  bz3[0] = BZ;
  bz3[1] = BZ;
  bz3[2] = dBZ;

  ni4 = ni * 4;

  /* rate of change of voxel with respect to change in parameters */
  dvdt = (double *)calloc(3 * nx + ni4, sizeof(double));

  /* Intermediate storage arrays */
  Tz = (double *)calloc(3 * nx * ny, sizeof(double));
  Ty = (double *)calloc(3 * nx, sizeof(double));
  betax = (double *)calloc(3 * nx + ni4, sizeof(double));
  betaxy = (double *)calloc(3 * nx * ny + ni4, sizeof(double));
  alphax = (double *)calloc((3 * nx + ni4) * (3 * nx + ni4), sizeof(double));
  alphaxy = (double *)calloc((3 * nx * ny + ni4) * (3 * nx * ny + ni4),
                             sizeof(double));

  for (i1 = 0; i1 < 3; i1++) {
    for (i2 = 0; i2 < 3; i2++) {
      Jz[i1][i2] = (double *)calloc(nx * ny, sizeof(double));
      Jy[i1][i2] = (double *)calloc(nx, sizeof(double));
    }
  }

  /* pointer to scales for each of the template images */
  scale1a = T + 3 * nx * ny * nz;

  /* only zero half the matrix */
  m1 = 3 * nx * ny * nz + ni4;
  for (x1 = 0; x1 < m1; x1++) {
    for (x2 = 0; x2 <= x1; x2++) alpha[m1 * x1 + x2] = 0.0;
    beta[x1] = 0.0;
  }

  for (s0[2] = 0; s0[2] < dim1[2];
       s0[2] += samp[2]) /* For each plane of the template images */
  {
    /* build up the deformation field (and derivatives) from it's seperable form
     */
    for (i1 = 0, ptr1 = T; i1 < 3; i1++, ptr1 += nz * ny * nx)
      for (x1 = 0; x1 < nx * ny; x1++) {
        /* intermediate step in computing nonlinear deformation field */
        tmp = 0.0;
        for (z1 = 0; z1 < nz; z1++)
          tmp += ptr1[x1 + z1 * ny * nx] * BZ[dim1[2] * z1 + s0[2]];
        Tz[ny * nx * i1 + x1] = tmp;

        /* intermediate step in computing Jacobian of nonlinear deformation
         * field */
        for (i2 = 0; i2 < 3; i2++) {
          tmp = 0;
          for (z1 = 0; z1 < nz; z1++)
            tmp += ptr1[x1 + z1 * ny * nx] * bz3[i2][dim1[2] * z1 + s0[2]];
          Jz[i2][i1][x1] = tmp;
        }
      }

    /* only zero half the matrix */
    m1 = 3 * nx * ny + ni4;
    for (x1 = 0; x1 < m1; x1++) {
      for (x2 = 0; x2 <= x1; x2++) alphaxy[m1 * x1 + x2] = 0.0;
      betaxy[x1] = 0.0;
    }

    for (s0[1] = 0; s0[1] < dim1[1];
         s0[1] += samp[1]) /* For each row of the template images plane */
    {
      /* build up the deformation field (and derivatives) from it's seperable
       * form */
      for (i1 = 0, ptr1 = Tz; i1 < 3; i1++, ptr1 += ny * nx) {
        for (x1 = 0; x1 < nx; x1++) {
          /* intermediate step in computing nonlinear deformation field */
          tmp = 0.0;
          for (y1 = 0; y1 < ny; y1++)
            tmp += ptr1[x1 + y1 * nx] * BY[dim1[1] * y1 + s0[1]];
          Ty[nx * i1 + x1] = tmp;

          /* intermediate step in computing Jacobian of nonlinear deformation
           * field */
          for (i2 = 0; i2 < 3; i2++) {
            tmp = 0;
            for (y1 = 0; y1 < ny; y1++)
              tmp += Jz[i2][i1][x1 + y1 * nx] * by3[i2][dim1[1] * y1 + s0[1]];
            Jy[i2][i1][x1] = tmp;
          }
        }
      }

      /* only zero half the matrix */
      m1 = 3 * nx + ni4;
      for (x1 = 0; x1 < m1; x1++) {
        for (x2 = 0; x2 <= x1; x2++) alphax[m1 * x1 + x2] = 0.0;
        betax[x1] = 0.0;
      }

      for (s0[0] = 0; s0[0] < dim1[0];
           s0[0] += samp[0]) /* For each pixel in the row */
      {
        double trans[3];

        /* nonlinear deformation of the template space, followed by the affine
         * transform */
        for (i1 = 0, ptr1 = Ty; i1 < 3; i1++, ptr1 += nx) {
          /* compute nonlinear deformation field */
          tmp = 1.0;
          for (x1 = 0; x1 < nx; x1++)
            tmp += ptr1[x1] * BX[dim1[0] * x1 + s0[0]];
          trans[i1] = tmp + s0[i1];

          /* compute Jacobian of nonlinear deformation field */
          for (i2 = 0; i2 < 3; i2++) {
            if (i1 == i2)
              tmp = 1.0;
            else
              tmp = 0;
            for (x1 = 0; x1 < nx; x1++)
              tmp += Jy[i2][i1][x1] * bx3[i2][dim1[0] * x1 + s0[0]];
            J[i2][i1] = tmp;
          }
        }

        /* Affine component */
        s2[0] = M[0 + 4 * 0] * trans[0] + M[0 + 4 * 1] * trans[1] +
                M[0 + 4 * 2] * trans[2] + M[0 + 4 * 3];
        s2[1] = M[1 + 4 * 0] * trans[0] + M[1 + 4 * 1] * trans[1] +
                M[1 + 4 * 2] * trans[2] + M[1 + 4 * 3];
        s2[2] = M[2 + 4 * 0] * trans[0] + M[2 + 4 * 1] * trans[1] +
                M[2 + 4 * 2] * trans[2] + M[2 + 4 * 3];

        /* is the transformed position in range? */
        if (s2[0] >= 1.0 && s2[0] < dim2[0] && s2[1] >= 1.0 &&
            s2[1] < dim2[1] && s2[2] >= 1.0 && s2[2] < dim2[2]) {
          double val000, val001, val010, val011, val100, val101, val110, val111;
          double dx1, dy1, dz1, dx2, dy2, dz2;
          double v, dv;
          int32 xcoord, ycoord, zcoord, off1, off2;

          nsamp++;

          /* coordinates to resample from */
          xcoord = (int32)floor(s2[0]);
          ycoord = (int32)floor(s2[1]);
          zcoord = (int32)floor(s2[2]);

          /* Calculate the interpolation weighting factors*/
          dx1 = s2[0] - xcoord;
          dx2 = 1.0 - dx1;
          dy1 = s2[1] - ycoord;
          dy2 = 1.0 - dy1;
          dz1 = s2[2] - zcoord;
          dz2 = 1.0 - dz1;

          /* get pixel values of 8 nearest neighbours */
          off1 = xcoord - 1 + dim2[0] * (ycoord - 1 + dim2[1] * (zcoord - 1));
          switch (dt2) {
            case vb_byte:
              val111 = dat2[off1];
              val011 = dat2[off1 + 1];
              off2 = off1 + dim2[0];
              val101 = dat2[off2];
              val001 = dat2[off2 + 1];
              off1 += dim2[0] * dim2[1];
              val110 = dat2[off1];
              val010 = dat2[off1 + 1];
              off2 = off1 + dim2[0];
              val100 = dat2[off2];
              val000 = dat2[off2 + 1];
              break;
            case vb_short:
              val111 = ((int16 *)dat2)[off1];
              val011 = ((int16 *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val101 = ((int16 *)dat2)[off2];
              val001 = ((int16 *)dat2)[off2 + 1];
              off1 += dim2[0] * dim2[1];
              val110 = ((int16 *)dat2)[off1];
              val010 = ((int16 *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val100 = ((int16 *)dat2)[off2];
              val000 = ((int16 *)dat2)[off2 + 1];
              break;
            case vb_long:
              val111 = ((int32 *)dat2)[off1];
              val011 = ((int32 *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val101 = ((int32 *)dat2)[off2];
              val001 = ((int32 *)dat2)[off2 + 1];
              off1 += dim2[0] * dim2[1];
              val110 = ((int32 *)dat2)[off1];
              val010 = ((int32 *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val100 = ((int32 *)dat2)[off2];
              val000 = ((int32 *)dat2)[off2 + 1];
              break;
            case vb_float:
              val111 = ((float *)dat2)[off1];
              val011 = ((float *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val101 = ((float *)dat2)[off2];
              val001 = ((float *)dat2)[off2 + 1];
              off1 += dim2[0] * dim2[1];
              val110 = ((float *)dat2)[off1];
              val010 = ((float *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val100 = ((float *)dat2)[off2];
              val000 = ((float *)dat2)[off2 + 1];
              break;
            case vb_double:
              val111 = ((double *)dat2)[off1];
              val011 = ((double *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val101 = ((double *)dat2)[off2];
              val001 = ((double *)dat2)[off2 + 1];
              off1 += dim2[0] * dim2[1];
              val110 = ((double *)dat2)[off1];
              val010 = ((double *)dat2)[off1 + 1];
              off2 = off1 + dim2[0];
              val100 = ((double *)dat2)[off2];
              val000 = ((double *)dat2)[off2 + 1];
              break;
            default:
              cerr << "Bad data type." << endl;
              exit(0);
          }

          /* resampled pixel value */
          v = (((val111 * dx2 + val011 * dx1) * dy2 +
                (val101 * dx2 + val001 * dx1) * dy1) *
                   dz2 +
               ((val110 * dx2 + val010 * dx1) * dy2 +
                (val100 * dx2 + val000 * dx1) * dy1) *
                   dz1) *
              scale2;

          /* local gradients accross resampled pixel (in space of object image)
           */
          dvds0[0] =
              ((dy2 * (val011 - val111) + dy1 * (val001 - val101)) * dz2 +
               (dy2 * (val010 - val110) + dy1 * (val000 - val100)) * dz1) *
              scale2;

          dvds0[1] =
              ((dx2 * (val101 - val111) + dx1 * (val001 - val011)) * dz2 +
               (dx2 * (val100 - val110) + dx1 * (val000 - val010)) * dz1) *
              scale2;

          dvds0[2] =
              ((dx2 * (val110 - val111) + dx1 * (val010 - val011)) * dy2 +
               (dx2 * (val100 - val101) + dx1 * (val000 - val001)) * dy1) *
              scale2;

          /* affine transform the gradients of object image*/
          dvds1[0] = M[0 + 4 * 0] * dvds0[0] + M[1 + 4 * 0] * dvds0[1] +
                     M[2 + 4 * 0] * dvds0[2];
          dvds1[1] = M[0 + 4 * 1] * dvds0[0] + M[1 + 4 * 1] * dvds0[1] +
                     M[2 + 4 * 1] * dvds0[2];
          dvds1[2] = M[0 + 4 * 2] * dvds0[0] + M[1 + 4 * 2] * dvds0[1] +
                     M[2 + 4 * 2] * dvds0[2];

          /* nonlinear transform the gradients to the same space as the template
           */
          dvds0[0] =
              J[0][0] * dvds1[0] + J[0][1] * dvds1[1] + J[0][2] * dvds1[2];
          dvds0[1] =
              J[1][0] * dvds1[0] + J[1][1] * dvds1[1] + J[1][2] * dvds1[2];
          dvds0[2] =
              J[2][0] * dvds1[0] + J[2][1] * dvds1[1] + J[2][2] * dvds1[2];

          /* there is no change in the contribution from BY and BZ, so only
             work from BX */
          for (i1 = 0; i1 < 3; i1++) {
            for (x1 = 0; x1 < nx; x1++)
              dvdt[i1 * nx + x1] = -dvds0[i1] * BX[dim1[0] * x1 + s0[0]];
          }

          /* coordinate of template voxel to sample */
          off2 = s0[0] + dim1[0] * (s0[1] + dim1[1] * s0[2]);

          /* dv will be the difference between the object image
             and the linear combination of templates */
          dv = v;
          for (i1 = 0; i1 < ni; i1++) {
            double tmp;
            switch (dt1[i1]) {
              case vb_byte:
                tmp = ((unsigned char *)(dat1[i1]))[off2] * scale1[i1];
                break;
              case vb_short:
                tmp = ((int16 *)(dat1[i1]))[off2] * scale1[i1];
                break;
              case vb_long:
                tmp = ((int32 *)(dat1[i1]))[off2] * scale1[i1];
                break;
              case vb_float:
                tmp = ((float *)(dat1[i1]))[off2] * scale1[i1];
                break;
              case vb_double:
                tmp = ((double *)(dat1[i1]))[off2] * scale1[i1];
                break;
              default:
                cerr << "Bad data type." << endl;
                exit(0);
            }

            /* linear combination of image and image modulated by constant
               gradients in x, y and z */
            dvdt[i1 * 4 + 3 * nx] = tmp;
            dvdt[i1 * 4 + 1 + 3 * nx] = tmp * s2[0];
            dvdt[i1 * 4 + 2 + 3 * nx] = tmp * s2[1];
            dvdt[i1 * 4 + 3 + 3 * nx] = tmp * s2[2];

            dv -= dvdt[i1 * 4 + 3 * nx] * scale1a[i1 * 4];
            dv -= dvdt[i1 * 4 + 1 + 3 * nx] * scale1a[i1 * 4 + 1];
            dv -= dvdt[i1 * 4 + 2 + 3 * nx] * scale1a[i1 * 4 + 2];
            dv -= dvdt[i1 * 4 + 3 + 3 * nx] * scale1a[i1 * 4 + 3];
          }

          /* cf Numerical Recipies "mrqcof.c" routine */
          m1 = 3 * nx + ni4;
          for (x1 = 0; x1 < m1; x1++) {
            for (x2 = 0; x2 <= x1; x2++)
              alphax[m1 * x1 + x2] += dvdt[x1] * dvdt[x2];
            betax[x1] += dvdt[x1] * dv;
          }

          /* sum of squares */
          ss += dv * dv;
        }
      }

      m1 = 3 * nx * ny + ni4;
      m2 = 3 * nx + ni4;

      /* Kronecker tensor products */
      for (y1 = 0; y1 < ny; y1++) {
        double wt = BY[dim1[1] * y1 + s0[1]];

        for (i1 = 0; i1 < 3; i1++) /* loop over deformations in x, y and z */
        {
          /* spatial-spatial covariances */
          for (i2 = 0; i2 <= i1;
               i2++) /* symmetric matrixes - so only work on half */
          {
            for (y2 = 0; y2 <= y1; y2++) {
              /* Kronecker tensor products with BY'*BY */
              double wt2 = wt * BY[dim1[1] * y2 + s0[1]];

              ptr1 = alphaxy + nx * (m1 * (ny * i1 + y1) + ny * i2 + y2);
              ptr2 = alphax + nx * (m2 * i1 + i2);

              for (x1 = 0; x1 < nx; x1++) {
                for (x2 = 0; x2 <= x1; x2++)
                  ptr1[m1 * x1 + x2] += wt2 * ptr2[m2 * x1 + x2];
              }
            }
          }

          /* spatial-intensity covariances */
          ptr1 = alphaxy + nx * (m1 * ny * 3 + ny * i1 + y1);
          ptr2 = alphax + nx * (m2 * 3 + i1);
          for (x1 = 0; x1 < ni4; x1++) {
            for (x2 = 0; x2 < nx; x2++)
              ptr1[m1 * x1 + x2] += wt * ptr2[m2 * x1 + x2];
          }

          /* spatial component of beta */
          for (x1 = 0; x1 < nx; x1++)
            betaxy[x1 + nx * (ny * i1 + y1)] += wt * betax[x1 + nx * i1];
        }
      }
      ptr1 = alphaxy + nx * (m1 * ny * 3 + ny * 3);
      ptr2 = alphax + nx * (m2 * 3 + 3);
      for (x1 = 0; x1 < ni4; x1++) {
        /* intensity-intensity covariances  */
        for (x2 = 0; x2 <= x1; x2++) ptr1[m1 * x1 + x2] += ptr2[m2 * x1 + x2];

        /* intensity component of beta */
        betaxy[nx * ny * 3 + x1] += betax[nx * 3 + x1];
      }
    }

    m1 = 3 * nx * ny * nz + ni4;
    m2 = 3 * nx * ny + ni4;

    /* Kronecker tensor products */
    for (z1 = 0; z1 < nz; z1++) {
      double wt = BZ[dim1[2] * z1 + s0[2]];

      for (i1 = 0; i1 < 3; i1++) /* loop over deformations in x, y and z */
      {
        /* spatial-spatial covariances */
        for (i2 = 0; i2 <= i1;
             i2++) /* symmetric matrixes - so only work on half */
        {
          for (z2 = 0; z2 <= z1; z2++) {
            /* Kronecker tensor products with BZ'*BZ */
            double wt2 = wt * BZ[dim1[2] * z2 + s0[2]];

            ptr1 = alpha + nx * ny * (m1 * (nz * i1 + z1) + nz * i2 + z2);
            ptr2 = alphaxy + nx * ny * (m2 * i1 + i2);
            for (y1 = 0; y1 < ny * nx; y1++) {
              for (y2 = 0; y2 <= y1; y2++)
                ptr1[m1 * y1 + y2] += wt2 * ptr2[m2 * y1 + y2];
            }
          }
        }

        /* spatial-intensity covariances */
        ptr1 = alpha + nx * ny * (m1 * nz * 3 + nz * i1 + z1);
        ptr2 = alphaxy + nx * ny * (m2 * 3 + i1);
        for (y1 = 0; y1 < ni4; y1++) {
          for (y2 = 0; y2 < ny * nx; y2++)
            ptr1[m1 * y1 + y2] += wt * ptr2[m2 * y1 + y2];
        }

        /* spatial component of beta */
        for (y1 = 0; y1 < ny * nx; y1++)
          beta[y1 + nx * ny * (nz * i1 + z1)] += wt * betaxy[y1 + nx * ny * i1];
      }
    }

    ptr1 = alpha + nx * ny * (m1 * nz * 3 + nz * 3);
    ptr2 = alphaxy + nx * ny * (m2 * 3 + 3);
    for (y1 = 0; y1 < ni4; y1++) {
      /* intensity-intensity covariances */
      for (y2 = 0; y2 <= y1; y2++) ptr1[m1 * y1 + y2] += ptr2[m2 * y1 + y2];

      /* intensity component of beta */
      beta[nx * ny * nz * 3 + y1] += betaxy[nx * ny * 3 + y1];
    }
  }

  /* Fill in the symmetric bits
     - OK I know some bits are done more than once - but it shouldn't matter. */

  m1 = 3 * nx * ny * nz + ni4;
  for (i1 = 0; i1 < 3; i1++) {
    double *ptrz, *ptry, *ptrx;
    for (i2 = 0; i2 <= i1; i2++) {
      ptrz = alpha + nx * ny * nz * (m1 * i1 + i2);
      for (z1 = 0; z1 < nz; z1++)
        for (z2 = 0; z2 <= z1; z2++) {
          ptry = ptrz + nx * ny * (m1 * z1 + z2);
          for (y1 = 0; y1 < ny; y1++)
            for (y2 = 0; y2 <= y1; y2++) {
              ptrx = ptry + nx * (m1 * y1 + y2);
              for (x1 = 0; x1 < nx; x1++)
                for (x2 = 0; x2 < x1; x2++)
                  ptrx[m1 * x2 + x1] = ptrx[m1 * x1 + x2];
            }
          for (x1 = 0; x1 < nx * ny; x1++)
            for (x2 = 0; x2 < x1; x2++) ptry[m1 * x2 + x1] = ptry[m1 * x1 + x2];
        }
      for (x1 = 0; x1 < nx * ny * nz; x1++)
        for (x2 = 0; x2 < x1; x2++) ptrz[m1 * x2 + x1] = ptrz[m1 * x1 + x2];
    }
  }
  for (x1 = 0; x1 < nx * ny * nz * 3; x1++)
    for (x2 = 0; x2 < x1; x2++) alpha[m1 * x2 + x1] = alpha[m1 * x1 + x2];

  ptr1 = alpha + nx * ny * nz * 3;
  ptr2 = alpha + nx * ny * nz * 3 * m1;
  for (x1 = 0; x1 < nx * ny * nz * 3; x1++)
    for (x2 = 0; x2 < ni4; x2++) ptr1[m1 * x1 + x2] = ptr2[m1 * x2 + x1];

  ptr1 = alpha + nx * ny * nz * 3 * (m1 + 1);
  for (x1 = 0; x1 < ni4; x1++)
    for (x2 = 0; x2 < x1; x2++) ptr1[m1 * x2 + x1] = ptr1[m1 * x1 + x2];

  *pss = ss;
  *pnsamp = nsamp;

  free((char *)dvdt);
  free((char *)Tz);
  free((char *)Ty);
  free((char *)betax);
  free((char *)betaxy);
  free((char *)alphax);
  free((char *)alphaxy);

  for (i1 = 0; i1 < 3; i1++) {
    for (i2 = 0; i2 < 3; i2++) {
      free((char *)Jz[i1][i2]);
      free((char *)Jy[i1][i2]);
    }
  }
}

// [Alpha,Beta,Var] =
// spm_brainwarp(REF,IMAGE,Affine,basX,basY,basZ,dbasX,dbasY,dbasZ,T,fwhm);
int dan_brainwarp(CrunchCube *REF, CrunchCube *IMAGE, const Matrix &Affine,
                  const Matrix &BX, const Matrix &BY, const Matrix &BZ,
                  const Matrix &dBX, const Matrix &dBY, const Matrix &dBZ,
                  const Matrix &T, double fwhm) {
  int32 i, j, nx, ny, nz, ni = 1, samp[3], m;
  int32 dim1[3], dim2[3], dt1[32], dt2, nsamp;
  double scale1[32], scale2;
  unsigned char *dat1[32], *dat2;
  double *alpha, *beta, *var;
  double *vBX, *vBY, *vBZ, *vdBX, *vdBY, *vdBZ, *vAffine, *vT;

  ni = 1;  // only one image ever passed
  dim1[0] = (int)REF->dimx;
  dim1[1] = (int)REF->dimy;
  dim1[2] = (int)REF->dimz;

  if (BX.rows() != dim1[0]) cerr << "Wrong sized X basis functions." << endl;
  nx = BX.cols();
  if (dBX.rows() != dim1[0] || dBX.cols() != nx)
    cerr << "Wrong sized X basis function derivatives." << endl;

  if (BY.rows() != dim1[1]) cerr << "Wrong sized Y basis functions." << endl;
  ny = BY.cols();
  if (BY.rows() != dim1[1] || dBY.cols() != ny)
    cerr << "Wrong sized Y basis function derivatives." << endl;

  if (BZ.rows() != dim1[2]) cerr << "Wrong sized Z basis functions." << endl;
  nz = BZ.cols();
  if (BZ.rows() != dim1[2] || dBZ.cols() != nz)
    cerr << "Wrong sized Z basis function derivatives." << endl;

  if ((T.rows() * T.cols()) != 3 * nx * ny * nz + ni * 4)
    cerr << "Transform is wrong size." << endl;

  /* sample about every fwhm/2 */
  samp[0] = (int)round(fwhm / 2.0 / REF->voxsize[0]);
  samp[0] = (int)((samp[0] < 1) ? 1 : samp[0]);
  samp[1] = (int)round(fwhm / 2.0 / REF->voxsize[1]);
  samp[1] = (int)((samp[1] < 1) ? 1 : samp[1]);
  samp[2] = (int)round(fwhm / 2.0 / REF->voxsize[2]);
  samp[2] = (int)((samp[2] < 1) ? 1 : samp[2]);

  dat1[0] = REF->data;
  dt1[0] = REF->datatype;
  scale1[0] = REF->scl_slope;
  if (scale1[0] == 0.0) scale1[0] = 1.0;

  dat2 = IMAGE->data;
  dim2[0] = (int)IMAGE->dimx;
  dim2[1] = (int)IMAGE->dimy;
  dim2[2] = (int)IMAGE->dimz;
  dt2 = IMAGE->datatype;
  scale2 = IMAGE->scl_slope;
  if (scale2 == 0.0) scale2 = 1.0;

  m = 3 * nx * ny * nz + ni * 4;
  alpha = new double[m * m];
  beta = new double[m];
  var = new double[1];

  vBX = arrayize(BX);
  vBY = arrayize(BY);
  vBZ = arrayize(BZ);
  vdBX = arrayize(dBX);
  vdBY = arrayize(dBY);
  vdBZ = arrayize(dBZ);
  vAffine = arrayize(Affine);
  vT = arrayize(T);

  mrqcof(vT, alpha, beta, var, dt2, scale2, dim2, dat2, ni, dt1, scale1, dim1,
         dat1, nx, vBX, vdBX, ny, vBY, vdBY, nz, vBZ, vdBZ, vAffine, samp,
         &nsamp);
  free(vBX);
  free(vBY);
  free(vBZ);
  free(vdBX);
  free(vdBY);
  free(vdBZ);
  free(vAffine);
  free(vT);

  // alpha
  ret_m1.resize(m, m);
  for (i = 0; i < m; i++) {
    for (j = 0; j < m; j++) {
      ret_m1(i, j) = alpha[j * m + i];
    }
  }
  // beta
  ret_m2.resize(m, 1);
  for (i = 0; i < m; i++) ret_m2(i, 0) = beta[i];
  ret_d1 = var[0] / (nsamp - (3 * nx * ny * nz + ni * 4));
  delete alpha;
  delete beta;
  delete var;

  return TRUE;
}
