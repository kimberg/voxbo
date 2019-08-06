
// vbsim.cpp
// VoxBo simulated data generator
// Copyright (c) 2003-2010 by The VoxBo Development Team

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

#include <assert.h>
#include "imageutils.h"
#include "vbio.h"
#include "vbutil.h"
#ifdef CYGWIN
#include "ieeefp.h"
#endif
#include "vbsim.hlp.h"

int get_datasize(VB_datatype type);

class VBSim {
 private:
  int dimx, dimy, dimz, dimt;

  Cube anat;  // anatomy
  string anatname;
  double n_mean, n_variance, n_fwhm;  // noise
  VB_Vector signal;
  string outfile;
  VB_datatype datatype;
  Tes mytes;
  Cube mycube;
  int AddUniform(Cube &cb, double low, double range, double sx, double sy,
                 double sz);
  double gaussian_random(double sigma);
  gsl_rng *rng;
  int AddGaussian(Cube &cb, double mu, double variance, double sx = 0.0,
                  double sy = 0.0, double sz = 0.0);

 public:
  int Go(int argc, char **argv);
};

void vbsim_help();

int main(int argc, char *argv[]) {
  stringstream tmps;
  tzset();  // make sure all times are timezone corrected
  VBSim sim;
  int err = sim.Go(argc - 1, argv + 1);
  exit(err);
}

int VBSim::Go(int argc, char *argv[]) {
  tokenlist args;
  rng = NULL;
  dimx = 0;
  dimy = 0;
  dimz = 0;
  dimt = 0;
  n_mean = 10.0;
  n_variance = 5.0;
  n_fwhm = 0.0;
  float vx = 1.0;
  float vy = 1.0;
  float vz = 1.0;
  float vt = 2000;
  uint32 rngseed = VBRandom();

  args.Transfer(argc, argv);

  if (args.size() == 0) {
    vbsim_help();
    exit(0);
  }

  for (size_t i = 0; i < args.size(); i++) {
    // -d x y z for dims
    if (args[i] == "-d" && i < args.size() - 4) {
      dimx = strtol(args[i + 1]);
      dimy = strtol(args[i + 2]);
      dimz = strtol(args[i + 3]);
      dimt = strtol(args[i + 4]);
      i += 4;
    }
    // -c for loading anatomy
    else if (args[i] == "-c" && i < args.size() - 1) {
      anatname = args[i + 1];
      i++;
    }
    // -z for voxel sizes
    else if (args[i] == "-z" && i < args.size() - 3) {
      vx = strtod(args[++i]);
      vy = strtod(args[++i]);
      vz = strtod(args[++i]);
      vt = strtod(args[++i]);
    }
    // -n for per-volume noise
    else if (args[i] == "-n" && i < args.size() - 3) {
      n_mean = strtod(args[i + 1]);
      n_variance = strtod(args[i + 2]);
      n_fwhm = strtod(args[i + 3]);
      i += 3;
    } else if (args[i] == "-s" && i < args.size() - 1)
      rngseed = strtol(args[++i]);
    else if (args[i] == "-o" && i < args.size() - 1) {
      outfile = args[i + 1];
      i++;
    } else {
      printf("[E] vbsim: unrecognized argument %s\n", args(i));
      return 140;
    }
  }
  if (dimx < 1 || dimy < 0 || dimz < 0 || dimt < 0) {
    printf("[E] vbsim: bad dimensions\n");
    return 110;
  }

  // initialize RNG
  rng = gsl_rng_alloc(gsl_rng_mt19937);
  assert(rng);
  gsl_rng_set(rng, rngseed);

  // FIXME tell the user here what we're doing

  // CREATE ANATOMY (constant image)
  if (anatname.size()) {
    if (anat.ReadFile(anatname)) {
      printf("[E] vbsim: couldn't read %s\n", anatname.c_str());
      return 101;
    }
    if (anat.dimx != dimx || anat.dimy != dimy || anat.dimz != dimz) {
      printf("[E] vbsim: %s doesn't match your dimensions\n", anatname.c_str());
      return 102;
    }
  } else {
    // create volume according to dimensions, add random noise if requested
    anat.SetVolume(dimx, dimy, dimz, vb_float);
  }

  // SPECIAL CASE-- vecs if dimy=dimz=dimt==0
  if (dimt == 0 && dimy == 0 && dimz == 0) {
    printf("[I] vbsim: creating a vector with %d elements with N(%g,%g)\n",
           dimx, n_mean, sqrt(n_variance));
    if (outfile == "") outfile = "data.ref";
    VB_Vector vv(dimx);

    if (!(isnan(n_mean))) {
      for (int32 i = 0; i < dimx; i++)
        vv[i] = n_mean + gaussian_random(sqrt(abs(n_variance)));
    }
    if (vv.WriteFile(outfile)) {
      printf("[E] vbsim: error writing 1D file %s\n", outfile.c_str());
      exit(120);
    }
    printf("[I] vbsim: wrote 1D file %s\n", outfile.c_str());
    exit(0);
  }

  // SPECIAL CASE-- cubes if dimt==0
  if (dimt == 0) {
    printf("[I] vbsim: creating a %dx%dx%d 3D volume with N(%g,%g)\n", dimx,
           dimy, dimz, n_mean, sqrt(n_variance));
    if (outfile == "") outfile = "data.cub";
    Cube vol(dimx, dimy, dimz, vb_float);
    if (!(isnan(n_mean)))
      AddGaussian(vol, n_mean, n_variance, n_fwhm, n_fwhm, n_fwhm);
    vol += anat;
    vol.setVoxSizes(vx, vy, vz, vt);
    if (vol.WriteFile(outfile)) {
      printf("[E] vbsim: error writing 3D volume %s\n", outfile.c_str());
      exit(120);
    }
    printf("[I] vbsim: wrote 3D volume %s\n", outfile.c_str());
    exit(0);
  }

  printf("[I] vbsim: creating a %dx%dx%dx%d 4D volume with N(%g,%g)\n", dimx,
         dimy, dimz, dimt, n_mean, sqrt(n_variance));
  if (outfile == "") outfile = "data.tes";
  // CREATE FUNCTIONALS (variable images, one per time point)
  mytes.SetVolume(dimx, dimy, dimz, dimt, vb_float);
  for (int i = 0; i < dimt; i++) {
    Cube vol(dimx, dimy, dimz, vb_float);
    if (!(isnan(n_mean)))
      AddGaussian(vol, n_mean, n_variance, n_fwhm, n_fwhm, n_fwhm);
    vol += anat;
    mytes.SetCube(i, vol);
  }
  mytes.setVoxSizes(vx, vy, vz, vt);
  if (mytes.WriteFile(outfile)) {
    printf("[E] vbsim: error writing 4D volume %s\n", outfile.c_str());
    return 120;
  }
  printf("[I] vbsim: wrote 4D volume %s\n", outfile.c_str());
  return 0;
}

int VBSim::AddUniform(Cube &cb, double low, double range, double sx, double sy,
                      double sz) {
  Cube cb2 = cb;
  cb2.zero();
  double val;
  // build random volume
  for (int i = 0; i < cb.dimx; i++) {
    for (int j = 0; j < cb.dimy; j++) {
      for (int k = 0; k < cb.dimz; k++) {
        val = VBRandom();
        val /= (double)0xffffffff;
        val *= range;
        val += low;
        cb2.SetValue(i, j, k, val);
      }
    }
  }
  // smooth if requested
  if (sx != 0.0 || sy != 0.0 || sz != 0.0) smoothCube(cb2, sx, sy, sz, 1);
  ;
  cb += cb2;
  return 0;
}

int VBSim::AddGaussian(Cube &cb, double mu, double variance, double sx,
                       double sy, double sz) {
  double sigma = sqrt(abs(variance));
  Cube cb2 = cb;
  cb2.zero();
  double val;
  // build random volume
  for (int i = 0; i < cb2.dimx; i++) {
    for (int j = 0; j < cb2.dimy; j++) {
      for (int k = 0; k < cb2.dimz; k++) {
        val = mu + gaussian_random(sigma);
        cb2.SetValue(i, j, k, val);
      }
    }
  }
  // smooth if requested
  if (sx != 0.0 || sy != 0.0 || sz != 0.0) smoothCube(cb2, sx, sy, sz, 1);
  cb += cb2;
  return 0;
}

double VBSim::gaussian_random(double sigma) {
  return gsl_ran_gaussian(rng, sigma);
}

void vbsim_help() { cout << boost::format(myhelp) % vbversion; }
