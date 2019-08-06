
// vbstatmap.cpp
// generate statistical maps
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
// original version written by Dan Kimberg
// developed substantially by Tom King
// and then later by Dan again

using namespace std;

#include <ctype.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_rng.h>
#include <sys/signal.h>
#include <algorithm>
#include <cctype>
#include <deque>
#include <list>
#include <sstream>
#include <vector>
#include "makestatcub.h"
#include "tokenlist.h"
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbstatmap.hlp.h"
#include "vbutil.h"

void vbstatmap_help();

gsl_rng *theRNG = NULL;
VBPrefs vbp;

int main(int argc, char *argv[]) {
  vbp.init();
  bool f_fdr = 0;
  double q = 0.01;
  tokenlist tcontrasts, tpseudot;
  VB_Vector contrasts, pseudot;
  int err = 0;
  string glmdir, dest, file, maskfile;
  tokenlist temp;
  Cube mask;
  GLMInfo glmi;
  if (argc == 1) {
    vbstatmap_help();
    exit(0);
  }
  arghandler a;
  a.setArgs("-q", "--qvalue", 1);
  a.setArgs("-p", "--smooth", 3);
  a.setArgs("-c", "--contrast", 1);
  a.setArgs("-o", "--output", 1);
  a.setArgs("-m", "--mask", 1);
  a.parseArgs(argc, argv);
  string errstring = a.badArg();
  if (errstring.size()) {
    printf("[E] vbstatmap: unrecognized flag: %s.\n", errstring.c_str());
    exit(101);
  }
  temp = a.getFlaggedArgs("-q");
  if (temp.size()) {
    q = strtod(temp[0]);
    f_fdr = 1;
  }
  temp = a.getFlaggedArgs("-o");
  dest = temp[0];
  temp = a.getFlaggedArgs("-p");
  if (temp.size()) {
    pseudot.resize(temp.size());
    for (size_t num = 0; num < temp.size(); num++)
      pseudot[num] = strtod(temp[num]);
  }
  temp = a.getUnflaggedArgs();
  glmdir = temp[0];  // stemname
  temp = a.getFlaggedArgs("-c");
  if (temp.size()) {
    glmi.setup(glmdir);
    if (glmi.parsecontrast(temp[0]) != 0) {
      printf("[E] vbstatmap: failed to derive a valid contrast.\n");
      exit(101);
    }
  } else {
    printf("[E] vbstatmap: failed to derive a valid contrast.\n");
    exit(101);
  }

  // FIXME commented out mechanism for cacheing stat maps
  //   string statfile = glmi.statcubeExists(glmdir, glmi.contrast.contrast,
  //   scale); if (statfile.size()) {
  //      printf("[I] vbstatmap: data can be found for %s map in %s.\n",
  //      scale.c_str(), statfile.c_str()); if (dest.size()==0) return 0;
  //   }

  temp = a.getFlaggedArgs("-m");
  maskfile = temp[0];
  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      printf("[E] vbstatmap: mask file %s does not exist.", maskfile.c_str());
      exit(135);
    }
    Tes prm;
    if (prm.ReadHeader(glmi.stemname + ".prm")) {
      printf("[E] vbstatmap: no parameter file found.");
      exit(137);
    }
    if (prm.dimx != mask.dimx || prm.dimy != mask.dimy ||
        prm.dimz != mask.dimz) {
      printf(
          "[E] vbstatmap: the mask file did have same dimensions as prm file.");
      exit(130);
    }
    if (!(mask.data)) {
      printf("[E] vbstatmap: the mask file did not constain data.");
      exit(131);
    }
  }
  if ((err = glmi.calc_stat_cube())) {
    printf("[E] vbstatmap: error %d calculating stat cube.\n", err);
    exit(101);
  }
  if (mask.data) glmi.statcube.intersect(mask);

  //   string randomfile = glmdir +"/map_" + VBRandom_filename() + ".cub";
  //   cb.SetFileName(randomfile);
  //   Tes prm(glmdir+"/"+glmdir+".prm");
  //   string ts = "TimeStamp: " + prm.GetHeader("TimeStamp:");
  //   if (ts.size() > 11)
  //     cb.AddHeader(ts);
  glmi.statcube.AddHeader("contrast_parameters: " + glmi.stemname);
  glmi.statcube.AddHeader("contrast_name: " + glmi.contrast.name);
  glmi.statcube.AddHeader("contrast_scale: " + glmi.contrast.scale);
  char number[128];
  string tmp = "contrast_vector: ";
  for (uint32 i = 0; i < glmi.contrast.contrast.size(); i++) {
    sprintf(number, "%.2f", glmi.contrast.contrast[i]);
    tmp += (string)number + " ";
  }
  glmi.statcube.AddHeader(tmp);
  if (pseudot.size() == 0)
    tmp = "pseudot_vector: 0 0 0";
  else {
    tmp = "pseudot_vector: ";
    for (uint32 i = 0; i < pseudot.size(); i++) {
      sprintf(number, "%.0f", pseudot[i]);
      tmp += number;
      tmp += " ";
    }
  }
  glmi.statcube.AddHeader(tmp);
  if (f_fdr) {
    vector<fdrstat> ffs;
    if (mask.data)
      ffs = calc_multi_fdr_thresh(glmi.rawcube, glmi.statcube, mask, q);
    else
      ffs = calc_multi_fdr_thresh(glmi.rawcube, glmi.statcube, glmi.rawcube, q);
    if (ffs.size()) {
      cout << (format("[I] vbstatmap: FDR calculation included %d voxels with "
                      "p values from %.4f to %.4f\n") %
               ffs[0].nvoxels % ffs[0].low % ffs[0].high)
                  .str();
      glmi.statcube.AddHeader(
          "# the following thresholds must be exceeded for FDR control");
      vbforeach(fdrstat ff, ffs) {
        if (ff.maxind >= 0)
          cout << (format("[I] vbstatmap: FDR threhsold for q=%.2f is %.4f\n") %
                   ff.q % ff.statval)
                      .str();
        else
          cout << (format("[I] vbstatmap: no FDR threhsold could be identified "
                          "for q=%.2f\n") %
                   ff.q)
                      .str();
        string tmps =
            (format("fdrthresh: %g %g") % ff.q % ff.statval).str().c_str();
        glmi.statcube.AddHeader(tmps);
      }
    }
  }
  if (dest.size()) {
    if (glmi.statcube.WriteFile(dest)) {
      printf("[E] vbstatmap: error writing %s\n", dest.c_str());
      exit(120);
    } else
      printf("[I] vbstatmap: writing %s map succeeded.\n",
             glmi.contrast.scale.c_str());
  }
  exit(0);
}

void vbstatmap_help() { cout << boost::format(myhelp) % vbversion; }
