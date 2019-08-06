
// vbinterpolate.cpp
// interpolate volumes in a 4D file
// Copyright (c) 2006 by The VoxBo Development Team

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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include "vbinterpolate.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void interpolate(Tes &im, vector<int> missings, int linearflag);
void vbinterpolate_help();
void vbinterpolate_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbinterpolate_help();
    exit(0);
  }
  tokenlist args;
  string infile, outfile;
  int floatflag = 0, nanflag = 1, linearflag = 0;

  arghandler ah;
  string errstr;
  ah.setArgs("-f", "--nofloat", 0);
  ah.setArgs("-n", "--nonan", 0);
  ah.setArgs("-l", "--linear", 0);
  ah.setArgs("-h", "--help", 0);
  ah.setArgs("-v", "--version", 0);
  ah.parseArgs(argc, argv);

  if ((errstr = ah.badArg()).size()) {
    printf("[E] vbinterpolate: %s\n", errstr.c_str());
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vbinterpolate_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vbinterpolate_version();
    exit(0);
  }
  tokenlist filelist = ah.getUnflaggedArgs();
  if (filelist.size() != 3) {
    printf(
        "[E] vbinterpolate: requires an input and an output file, plus "
        "indices\n");
    exit(10);
  }
  if (ah.flagPresent("-f")) floatflag = 1;
  if (ah.flagPresent("-n")) nanflag = 1;
  if (ah.flagPresent("-l")) linearflag = 1;

  infile = filelist[0];
  outfile = filelist[1];
  vector<int> targets = numberlist(filelist[2]);

  printf("[I] vbinterpolate: interpolating\n");
  Tes im;
  im.ReadFile(infile);
  if (!(im.data_valid)) {
    printf("[E] vbinterpolate: couldn't make a valid 4D volume out of %s\n",
           infile.c_str());
    exit(5);
  }
  // remove NaNs and Infs if requested
  if (nanflag) im.removenans();
  // convert to float if requested
  if (floatflag && im.datatype != vb_float)
    im.convert_type(vb_float, VBSETALT | VBNOSCALE);

  interpolate(im, targets, linearflag);

  im.SetFileName(outfile);
  if (im.WriteFile()) {
    printf("[E] vbinterpolate: error writing file %s\n", outfile.c_str());
    exit(110);
  } else
    printf("[I] vbinterpolate: done.\n");
  exit(0);
}

void interpolate(Tes &im, vector<int> missings, int linearflag) {
  int points = im.dimt - missings.size();
  VB_Vector tesmask(im.dimt);
  tesmask *= 0.0;
  tesmask += 1.0;
  for (int i = 0; i < (int)missings.size(); i++) tesmask[missings[i]] = 0.0;

  VB_Vector xa(points);
  VB_Vector ya(points);

  gsl_interp *myinterp;
  if (linearflag)
    myinterp = gsl_interp_alloc(gsl_interp_linear, points);
  else
    myinterp = gsl_interp_alloc(gsl_interp_cspline, points);

  double *xptr = xa.getTheVector()->data;
  double *yptr = ya.getTheVector()->data;
  double val;

  for (int i = 0; i < im.dimx; i++) {
    for (int j = 0; j < im.dimy; j++) {
      for (int k = 0; k < im.dimz; k++) {
        if (!(im.GetMaskValue(i, j, k))) continue;
        int interpind = 0;
        for (int m = 0; m < im.dimt; m++) {
          if (tesmask[m]) {
            xa[interpind] = m;
            ya[interpind] = im.GetValue(i, j, k, m);
            interpind++;
          }
        }
        gsl_interp_init(myinterp, xptr, yptr, points);
        for (int m = 0; m < (int)missings.size(); m++) {
          val = gsl_interp_eval(myinterp, xptr, yptr, missings[m], NULL);
          im.SetValue(i, j, k, missings[m], val);
        }
      }
    }
  }
  gsl_interp_free(myinterp);
}

void vbinterpolate_help() { cout << boost::format(myhelp) % vbversion; }

void vbinterpolate_version() {
  printf("VoxBo vbinterpolate (v%s)\n", vbversion.c_str());
}
