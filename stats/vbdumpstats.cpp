
// vbdumpstats.cpp
// dump ROI stats
// Copyright (c) 2005-2009 by The VoxBo Development Team

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
#include "glmutil.h"
#include "vbdumpstats.hlp.h"
#include "vbio.h"
#include "vbutil.h"

void domycontrast(GLMInfo &glmi, vector<string> contrastlist, int q_tabflag,
                  string rname);
void vbdumpstats_help();
void vbdumpstats_version();

// class VBDumpStats {
// public:
//   // the things we need to know
//   VBContrast contrast;
//   string glmdir;
//   Cube mask;
//   // accumulated stats
//   vector<double> stats;
//   // methods
//   int ProcessFile(string &str);
//   void CalcStat();
// };

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbdumpstats_help();
    exit(0);
  }
  tokenlist args;

  string contraststring, q_scale = "t";
  vector<int> includelist, excludelist;
  int q_driftflag = 0, q_meannormflag = 0, q_tabflag = 0;
  int q_nodriftflag = 0, q_nomeannormflag = 0;
  int q_component = -1;
  vector<string> masklist;
  vector<string> filelist;
  vector<string> contrastlist;
  vector<VBRegion> regionlist;

  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-c" && i + 1 < args.size()) {
      contrastlist.push_back(args[i + 1]);
      i++;
    } else if (args[i] == "-d")
      q_driftflag = 1;
    else if (args[i] == "-n")
      q_meannormflag = 1;
    else if (args[i] == "-d0")
      q_nodriftflag = 1;
    else if (args[i] == "-n0")
      q_nomeannormflag = 1;
    else if (args[i] == "-t")
      q_tabflag = 1;
    else if (args[i] == "-m" && i + 1 < args.size()) {
      masklist.push_back(args[i + 1]);
      i++;
    } else if (args[i] == "-pca" && i + 1 < args.size()) {
      q_component = strtol(args[i + 1]);
      i++;
    } else if (args[i] == "-p" && i + 3 < args.size()) {
      VBRegion rr;
      uint32 xx = strtol(args[i + 1]);
      uint32 yy = strtol(args[i + 2]);
      uint32 zz = strtol(args[i + 3]);
      rr.add(xx, yy, zz, 0.0);
      rr.name =
          (string) "point_" + strnum(xx) + "_" + strnum(yy) + "_" + strnum(zz);
      regionlist.push_back(rr);
      i += 3;
    } else if (args[i] == "-h") {
      vbdumpstats_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbdumpstats_version();
      exit(0);
    } else {
      filelist.push_back(args[i]);
    }
  }
  if (filelist.size() < 1) {
    printf("[E] vbdumpstats: you must specify at least one GLM directory\n");
    exit(5);
  }

  int mx = 0, my = 0, mz = 0;
  for (int i = 0; i < (int)masklist.size(); i++) {
    Cube mask;
    VBRegion myregion;
    mask.ReadFile(masklist[i]);
    if (!mask.data) {
      printf("[E] vbdumpstats: %s couldn't be loaded as a mask\n",
             masklist[i].c_str());
      continue;
    }
    if (mx == 0) {
      mx = mask.dimx;
      my = mask.dimy;
      mz = mask.dimz;
    } else if (mx != mask.dimx || my != mask.dimy || mz != mask.dimz) {
      printf("[E] vbdumpstats: mask %s has inconsistent dimensions, ignoring\n",
             masklist[i].c_str());
      continue;
    }
    myregion.convert(mask, vb_agt, 0.0);
    myregion.name = xfilename(masklist[i]);
    regionlist.push_back(myregion);
    //     printf("[I] vbdumpstats: mask %s includes %d voxels\n",
    //            masklist[i].c_str(),myregion.size());
  }

  if (regionlist.size() == 0) {
    printf("[E] vbdumpstats: must specify either a valid mask or a point\n");
    exit(120);
  }

  for (int i = 0; i < (int)filelist.size(); i++) {
    GLMInfo glmi;
    glmi.setup(filelist[i]);
    if (glmi.stemname.size() == 0) {
      printf("[E] vbdumpstats: %s isn't a valid GLM directory\n",
             filelist[i].c_str());
      continue;
    }
    printf("[I] vbdumpstats: GLM %s\n", glmi.stemname.c_str());
    VB_Vector vv;
    long tsflags = 0;
    tsflags = glmi.glmflags;
    if (q_driftflag) tsflags |= DETREND;
    if (q_meannormflag) tsflags |= MEANSCALE;
    if (q_nodriftflag) tsflags &= ~(DETREND);
    if (q_nomeannormflag) tsflags &= ~(MEANSCALE);
    printf(
        "[I] vbdumpstats: mean scaling: %s\n[I] vbdumpstats: detrending: %s\n",
        (tsflags & MEANSCALE ? "yes" : "no"),
        (tsflags & DETREND ? "yes" : "no"));

    // FIXME CHECK MASK DIMENSION MATCH
    for (int j = 0; j < (int)regionlist.size(); j++) {
      VBRegion tmpregion;
      tmpregion = glmi.restrictRegion(regionlist[j]);
      printf("[I] vbdumpstats: region %s (%d voxel%s)\n",
             regionlist[j].name.c_str(), tmpregion.size(),
             (tmpregion.size() == 1 ? "" : "s"));
      // load time series
      if (q_component > -1) {
        VBMatrix tmp = glmi.getRegionComponents(tmpregion, tsflags);
        if ((int)tmp.n <= q_component) {
          printf("[E] vbdumpstats: couldn't extract requested component (%d)\n",
                 q_component);
          exit(166);
        }
        vv = tmp.GetColumn(q_component);
      } else
        vv = glmi.getRegionTS(tmpregion, tsflags);
      // NOTE: filtering gets done in the regression routine
      // do the regression
      if (vv.size() == 0) {
        printf("[E] vbdumpstats: couldn't retrieve timeseries data\n");
        exit(15);
      }
      int err = glmi.Regress(vv);
      if (err) {
        printf("[E] vbdumpstats: error %d regressing time series\n", err);
        exit(15);
      }
      domycontrast(glmi, contrastlist, q_tabflag, regionlist[i].name);
    }
  }
  exit(0);
}

void domycontrast(GLMInfo &glmi, vector<string> contrastlist, int q_tabflag,
                  string rname) {
  vector<VBContrast> contrasts;
  if (contrastlist.size() == 0)
    contrasts = glmi.contrasts;
  else {
    for (int i = 0; i < (int)contrastlist.size(); i++) {
      if (glmi.parsecontrast(contrastlist[i])) {
        printf("[E] vbdumpstats: couldn't parse contrast %s for GLM %s\n",
               contrastlist[i].c_str(), glmi.stemname.c_str());
        return;
      }
      contrasts.push_back(glmi.contrast);
    }
  }
  if (contrasts.size() == 0) {
    printf("[E] vbdumpstats: no valid contrasts for GLM %s\n",
           glmi.stemname.c_str());
    exit(10);
  }

  for (int i = 0; i < (int)contrasts.size(); i++) {
    glmi.contrast = contrasts[i];

    int err = glmi.calc_stat();
    if (err) {
      printf("[E] vbdumpstats: calc_stat returned error %d\n", err);
      exit(150);
    }

    string realscale = glmi.contrast.scale;
    if (glmi.contrast.scale == "t") {
      double tval = glmi.statval;
      double p1 = nan("nan");
      double p2 = nan("nan");

      glmi.statval = tval;
      glmi.contrast.scale = "p1";
      if (!(glmi.convert_t())) p1 = glmi.statval;
      glmi.statval = tval;
      glmi.contrast.scale = "p2";
      if (!(glmi.convert_t())) p2 = glmi.statval;
      if (q_tabflag)
        printf("%s\t%s\t%s\t%s\t%.2f\t%.12f\t%.4f\t%.4f\n",
               glmi.stemname.c_str(), rname.c_str(), glmi.contrast.name.c_str(),
               realscale.c_str(), glmi.traceRV[2], tval, p1, p2);
      else
        printf(
            "[%s] contrast=%s scale=%s effdf=%.2f stat=%.12f one-tailed_p=%.4f "
            "two-tailed_p=%.4f\n",
            glmi.stemname.c_str(), glmi.contrast.name.c_str(),
            realscale.c_str(), glmi.traceRV[2], tval, p1, p2);
    } else if (glmi.contrast.scale == "f") {
      double fval = glmi.statval;
      double pval = nan("nan");
      glmi.contrast.scale = "p";
      if (!(glmi.convert_f())) pval = glmi.statval;
      if (q_tabflag)
        printf("%s\t%s\t%s\t%.2f\t%.12f\t%.4f\n", glmi.stemname.c_str(),
               glmi.contrast.name.c_str(), realscale.c_str(), glmi.traceRV[2],
               fval, pval);
      else
        printf("[%s] contrast=%s scale=%s effdf=%.2f stat=%.12f p=%.4f\n",
               glmi.stemname.c_str(), glmi.contrast.name.c_str(),
               realscale.c_str(), glmi.traceRV[2], fval, pval);
    } else {
      if (q_tabflag)
        printf("%s\t%s\t%s\t%.12f\n", glmi.stemname.c_str(),
               glmi.contrast.name.c_str(), realscale.c_str(), glmi.statval);
      else
        printf("[%s] contrast=%s scale=%s stat=%.12f\n", glmi.stemname.c_str(),
               glmi.contrast.name.c_str(), realscale.c_str(), glmi.statval);
    }
  }
}

void vbdumpstats_help() { cout << boost::format(myhelp) % vbversion; }

void vbdumpstats_version() {
  printf("VoxBo vbdumpstats (v%s)\n", vbversion.c_str());
}
