
// glminfo.cpp
// print out random info about a glm directory
// Copyright (c) 2004-2009 by The VoxBo Development Team

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
#include "glminfo.hlp.h"
#include "glmutil.h"
#include "vbio.h"
#include "vbutil.h"

void glminfo_help();
void glminfo_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    glminfo_help();
    exit(0);
  }
  tokenlist args;
  int contrastflag = 1;
  int interestflag = 0;
  int allvarsflag = 1;
  int longflag = 0;
  int rmulflag = 0;
  string glmdir;
  string maskname;

  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-l")
      longflag = 1;
    else if (args[i] == "-i") {
      allvarsflag = 0;
      interestflag = 1;
    } else if (args[i] == "-r")
      rmulflag = 1;
    else if (args[i] == "-v") {
      glminfo_version();
      exit(0);
    } else if (args[i] == "-m" && i < args.size() - 1)
      maskname = args[++i];
    else if (args[i] == "-h") {
      glminfo_help();
      exit(0);
    } else
      glmdir = args[i];
  }

  GLMInfo glmi;
  VB_Vector tmpv;
  VBMatrix KG, G;
  Tes prm;

  // set up and print basic information that we're doing one way or
  // another
  glmi.setup(glmdir);
  printf("\nVoxBo glminfo (v%s)\n\n", vbversion.c_str());
  printf("GLM directory: %s\n", glmdir.c_str());

  // g matrix stuff
  if (G.ReadHeader(glmi.stemname + ".G") == 0) {
    printf("G matrix columns (rank): %d (number of variables)\n", G.cols);
    printf("G matrix rows (order): %d (number of observations)\n", G.rows);
  } else
    printf("Couldn't read G matrix header.\n");
  printf("Number of 4D (tes) files: %d\n", (int)glmi.teslist.size());
  printf("Total independent variables: %d",
         (int)glmi.interestlist.size() + (int)glmi.nointerestlist.size());
  printf(" (%d of interest, %d not)\n", (int)glmi.interestlist.size(),
         (int)glmi.nointerestlist.size());
  if (glmi.dependentindex > -1) printf("Dependent variables: 1\n");
  printf("Detrend: %s\nMean scaling: %s\n",
         ((glmi.glmflags & DETREND) ? "on" : "off"),
         ((glmi.glmflags & MEANSCALE) ? "on" : "off"));
  // smoothness estimate
  tmpv.ReadFile(glmi.stemname + ".se");
  if (tmpv.size() == 3)
    printf("FWHM smoothness(xyz): %g %g %g (mean=%g)\n", tmpv[0], tmpv[1],
           tmpv[2], (tmpv[0] + tmpv[1] + tmpv[2]) / (double)3.0);
  else
    printf("FWHM smoothness: not found\n");
  // traces
  tmpv.ReadFile(glmi.stemname + ".traces");
  if (tmpv.size() == 3)
    printf("Effective degrees of freedom (effdf): %g\n", tmpv[2]);
  else
    printf("FWHM smoothness: not found\n");

  if (prm.ReadHeader(glmi.stemname + ".prm"))
    printf("Couldn't open stat volume\n");
  else {
    printf("Stat volume dimensions: %dx%dx%d, %d time points\n", prm.dimx,
           prm.dimy, prm.dimz, prm.dimt);
    printf("Stat volume voxels: %d (%d in mask)\n",
           prm.dimx * prm.dimy * prm.dimz, prm.realvoxels);
    printf("Stat volume voxel sizes: %g x %g x %gmm\n", prm.voxsize[0],
           prm.voxsize[1], prm.voxsize[2]);
  }

  if (contrastflag) {
    printf("Contrasts:\n");
    for (size_t i = 0; i < glmi.contrasts.size(); i++) {
      printf("  [contrast] %s\n", glmi.contrasts[i].name.c_str());
      if (longflag) glmi.contrasts[i].print();
    }
  }
  if (interestflag || allvarsflag) {
    printf("Independent variables:\n");
    for (size_t i = 0; i < glmi.cnames.size(); i++) {
      char type = glmi.cnames[i][0];
      if (!allvarsflag && type != 'I') continue;
      if (type == 'I')
        printf("  [  Interest] ");
      else if (type == 'K')
        printf("  [ KeepNoInt] ");
      else if (type == 'N')
        printf("  [NoInterest] ");
      else if (type == 'D')
        printf("  [ Dependent] ");
      else
        printf("  [   Unknown] ");
      printf(" %s\n", glmi.cnames[i].c_str() + 1);
    }
  }
  if (glmi.trialsets.size()) {
    printf("Trialsets (for trial averaging):\n");
    vector<TASpec>::iterator tt;
    for (tt = glmi.trialsets.begin(); tt != glmi.trialsets.end(); tt++) {
      printf("  [%s] %d trials, %d samples each\n", tt->name.c_str(),
             (int)tt->startpositions.size(), (int)tt->nsamples);
    }
  }
  if (maskname.size()) {
    glmi.loadcombinedmask();
    if (glmi.mask.data) glmi.mask.WriteFile(maskname);
  }
  if (rmulflag) {
    printf("Rmul (collinearity) for all IVs of interest in the KG matrix:\n");
    KG.ReadFile(glmi.stemname + ".KG");
    if (KG.n < 2)
      printf("  there is only one IV in your design matrix\n");
    else {
      for (size_t i = 0; i < glmi.interestlist.size(); i++) {
        VBMatrix modkg = KG;
        VB_Vector vv;
        vv = KG.GetColumn(glmi.interestlist[i]);
        // modkg=KG;
        modkg.DeleteColumn(glmi.interestlist[i]);
        double rmul = calcColinear(modkg, vv);
        printf("  %s: %g\n", glmi.cnames[glmi.interestlist[i]].c_str() + 1,
               rmul);
      }
      printf("\n");
    }
  }

  // FIXME print useful information about the mean scale, detrend, and
  // maybe rmul
  printf("Notes:\n");
  printf("  Mean scaling and detrending are applied separately to the time\n");
  printf(
      "  series from each voxel, within each 4D (tes) file.  Mean scaling\n");
  printf("  means dividing the time series by its mean.  Detrending means\n");
  printf("  fitting a regression line to the time series and adjusting the\n");
  printf("  data around the midpoint to set the slope to zero.\n");
  printf("\n");
  if (rmulflag) {
    printf("  Rmul is a measure of the collinearity calculated from the\n");
    printf("  correlation between a given regressor and the best linear\n");
    printf("  combination of the remaining regressors.\n");
    printf("\n");
  }
  exit(0);
}

void glminfo_help() { cout << boost::format(myhelp) % vbversion; }

void glminfo_version() { printf("VoxBo glminfo (v%s)\n", vbversion.c_str()); }
