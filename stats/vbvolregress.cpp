
// vbvolregress.cpp
// carry out volume regression using regular GLM
// Copyright (c) 2007-2010 by The VoxBo Development Team

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

// replace IND in the output file name if it's a perm
// if -o is omitted, generate statmap_contrastname.suffix,
// params_contrastname.suffix45, and resids_contrastname.suffix4d if -prm is
// there, generate params, if -res is there generate res if -prm=xxx is there,
// use xxx, same with res, but still replace IND if it's a perm

using namespace std;

#include <stdio.h>
#include <string.h>
#include <sstream>
#include "glmutil.h"
#include "makestatcub.h"
#include "vbio.h"
#include "vbutil.h"
#include "vbvolregress.hlp.h"

void vbvolregress_help();
void vbvolregress_version();

int main(int argc, char *argv[]) {
  if (argc < 2) {
    vbvolregress_help();
    exit(0);
  }

  string permstring = "PERM";
  string partstring = "PART";
  string contstring = "CONT";
  int f_listvars = 0;
  tokenlist args;
  string infile, outfile;
  string dvname;
  string maskname;
  vector<string> ivnames;
  vector<string> contrastlist;
  args.Transfer(argc - 1, argv + 1);
  int part = 1, nparts = 1;
  vector<VBMatrix> ivmats;
  string perm_mat;
  enum { vb_none, vb_sign, vb_order } permtype = vb_none;
  int signperm_index = -1;
  int orderperm_index = -1;
  string stemname = "noname";
  string suffix = "cub";
  string mapname, prmname, resname, distname;
  string spx, opx;
  bool f_volume = 0;
  bool f_fdr = 0;
  float q = 0.0;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-v") {
      vbvolregress_version();
      exit(0);
    } else if (args[i] == "-h") {
      vbvolregress_help();
      exit(0);
    } else if (args[i] == "-s" && i < args.size() - 1)
      suffix = args[++i];
    else if ((args[i] == "-o" || args[i] == "-mapfile") && i < args.size() - 1)
      mapname = args[++i];
    else if (args[i] == "-m" && i < args.size() - 1)
      maskname = args[++i];
    else if (args[i] == "-prmfile" && i < args.size() - 1)
      prmname = args[++i];
    else if (args[i] == "-resfile" && i < args.size() - 1)
      resname = args[++i];
    else if (args[i] == "-distfile" && i < args.size() - 1)
      distname = args[++i];
    else if (args[i] == "-sp" && i < args.size() - 2) {
      perm_mat = args[++i];
      permtype = vb_sign;
      signperm_index = strtol(args[++i]);
    } else if (args[i] == "-op" && i < args.size() - 2) {
      perm_mat = args[++i];
      permtype = vb_order;
      orderperm_index = strtol(args[++i]);
    } else if (args[i] == "-spx" && i < args.size() - 1) {
      perm_mat = args[++i];
      permtype = vb_sign;
    } else if (args[i] == "-opx" && i < args.size() - 1) {
      perm_mat = args[++i];
      permtype = vb_order;
    } else if (args[i] == "-p" && i < args.size() - 2) {
      part = strtol(args[++i]);
      nparts = strtol(args[++i]);
    } else if (args[i] == "-q" && i < args.size() - 1) {
      q = strtod(args[++i]);
      f_fdr = 1;
    } else if (args[i] == "-c" && i < args.size() - 1)
      contrastlist.push_back(args[++i]);
    else if (args[i] == "-dv" && i < args.size() - 1)
      dvname = args[++i];
    else if (args[i] == "-iv" && i < args.size() - 1)
      ivnames.push_back((string) "V" + args[++i]);
    else if (args[i] == "-gv" && i < args.size() - 1) {
      int gcnt = strtol(args[i + 1]);
      if (gcnt < 2 || gcnt > 999) {
        printf("[E] vbvolregress: -gv arguments must have 2-999 groups\n");
        return 220;
      }
      int glen = 0, tmp;
      vector<int> sizes;
      for (size_t g = i + 2; g < i + 2 + gcnt; g++) {
        if (g > args.size() - 1) {
          printf("[E] vbvolregress: not enough arguments for your -gv group\n");
          exit(210);
        }
        tmp = strtol(args[g]);
        glen += tmp;
        sizes.push_back(tmp);
      }
      VBMatrix mat(glen, sizes.size() - 1);
      int gstart = sizes[0];
      for (int g = 1; g < (int)sizes.size(); g++) {
        VB_Vector vv(glen);
        for (int j = gstart; j < gstart + sizes[g]; j++) vv[j] = 1;
        gstart += sizes[g];
        vv.meanCenter();
        mat.SetColumn(g - 1, vv);
      }
      mat.filename = "testmat";
      ivmats.push_back(mat);
      ivnames.push_back("Ggenerated");
    } else if (args[i] == "-int") {
      ivnames.push_back("Iintercept");
    } else if (args[i] == "-x") {
      f_listvars = 1;
    } else {
      cout << "[E] vbvolregress: unrecognized argument " << args[i] << endl;
      exit(99);
    }
  }

  // just list the vars in order, don't do any actual processing
  if (f_listvars) {
    int matcnt = 0;
    for (int i = 0; i < (int)ivnames.size(); i++) {
      if (ivnames[i][0] == 'I') {
        printf("[I] vbvolregress: IV: intercept\n");
      } else if (ivnames[i][0] == 'G') {
        printf("[I] vbvolregress: IV: group %d (%d groups, %d variables)\n",
               matcnt, ivmats[matcnt].n + 1, ivmats[matcnt].n);
        matcnt++;
      } else if (ivnames[i][0] == 'V') {
        printf("[I] vbvolregress: IV: %s\n", ivnames[i].c_str() + 1);
      }
    }
    printf("[I] vbvolregress: DV: %s\n", dvname.c_str());
    exit(0);
  }

  GLMInfo glmi;
  Cube tmask;
  Tes ts;
  // build our mask
  if (!(ts.ReadHeader(dvname))) {
    f_volume = 1;
    ts.ExtractMask(tmask);
  }
  // find masks in tes files
  for (int i = 0; i < (int)ivnames.size(); i++) {
    if (ivnames[i].size() < 2) continue;
    if (!(ts.ReadHeader(ivnames[i].substr(1)))) {
      f_volume = 1;
      if (tmask.dimx == 0) {
        ts.ExtractMask(tmask);
      } else {
        Cube tmp;
        ts.ExtractMask(tmp);
        if (tmp.dimx != tmask.dimx || tmp.dimy != tmask.dimy ||
            tmp.dimz != tmask.dimz) {
          printf("[E] vbvolregress: IV file %s has inconsistent dimensions\n",
                 ivnames[i].c_str());
          exit(201);
        }
        tmask.intersect(tmp);
      }
    }
  }
  if (maskname.size()) {
    f_volume = 1;
    Cube mask;
    if (mask.ReadFile(maskname)) {
      printf("[E] vbvolregress: error reading mask file %s\n",
             maskname.c_str());
      exit(200);
    }
    if (tmask.dimx && (mask.dimx != tmask.dimx || mask.dimy != tmask.dimy ||
                       mask.dimz != tmask.dimz)) {
      printf(
          "[E] vbvolregress: mask file %s has dimensions inconsistent with \n",
          maskname.c_str());
      exit(200);
    }
    if (tmask.data)
      tmask.intersect(mask);
    else
      tmask = mask;
  }

  VBMatrix vm;
  if (perm_mat.size()) {
    if (vm.ReadFile(perm_mat)) {
      printf("[E] vbvolregress: error getting permutations\n");
      exit(200);
    }
  }
  vector<int32> perminds;

  // do a specifix sign permutation...
  if (signperm_index > -1) perminds.push_back(signperm_index);
  // ...or order permutation...
  else if (orderperm_index > -1)
    perminds.push_back(orderperm_index);
  // ...or all of them...
  else if (permtype != vb_none) {
    for (uint32 jj = 0; jj < vm.cols; jj++) perminds.push_back(jj);
  }
  // ...or (most typically) just the unpermuted regression
  else
    perminds.push_back(-1);

  if (!(tmask.data)) {
    tmask.SetVolume(1, 1, 1, vb_short);
    tmask.SetValue(0, 0, 0, 1);
  }

  if (perminds.size() > 1 && distname.size()) unlink(distname.c_str());

  string part_tag = "_part_" + strnum(part);

  // if no map output and no contrasts, make sure we have at least a
  // dummy contrast, so that we can still generate residuals or
  // whatever
  if (contrastlist.size() == 0 && mapname.empty())
    contrastlist.push_back("default t spikes 0");

  // create interest list (all vars in this case) for contrast definition
  vector<int> ilist;
  // all vars of nominal interest
  for (int i = 0; i < (int)ivnames.size(); i++) ilist.push_back(i);

  cout << "[I] vbvolregress: the following contrasts were specified:\n";
  vbforeach(string & ccs, contrastlist) {
    VBContrast cc;
    tokenlist cspec;
    cspec.ParseLine(ccs);
    cc.parsemacro(cspec, ilist.size(), ilist);
    printf("[I] contrast %s (%s):\n", cc.name.c_str(), cc.scale.c_str());
    for (uint32 i = 0; i < cc.contrast.size(); i++)
      printf("      %s: %.1f\n", ivnames[i].c_str() + 1, cc.contrast[i]);
  }

  string w_mapname, w_prmname, w_resname;
  // do the contrasts for each permutation requested
  for (size_t permi = 0; permi < perminds.size(); permi++) {
    string perm_tag = (format("%06d") % perminds[permi]).str();
    // set up the proper permutation here
    if (perminds[permi] > -1) {
      if (permtype == vb_sign)
        glmi.perm_order = vm.GetColumn(perminds[permi]);
      else
        glmi.perm_signs = vm.GetColumn(perminds[permi]);
    }
    // now iterate over contrasts
    for (int c = 0; c < (int)contrastlist.size(); c++) {
      VBContrast cc;
      tokenlist cspec;
      cspec.ParseLine(contrastlist[c]);
      glmi.contrast.parsemacro(cspec, ilist.size(), ilist);

      if (f_volume) {
        // do the volume regression
        glmi.rescount = 0;
        int err =
            glmi.VolumeRegress(tmask, part, nparts, ivnames, dvname, ivmats);
        if (err) {
          printf("[E] vbvolregress (vol): failed with %s (%d)!\n",
                 glmi.stemname.c_str(), err);
          exit(err);
        }

        // print out FDR thresh if requested
        if (f_fdr) {
          vector<fdrstat> ffs =
              calc_multi_fdr_thresh(glmi.rawcube, glmi.statcube, tmask, q);
          if (ffs.size()) {
            cout << (format("[I] vbvolregress: FDR calculation included %d "
                            "voxels with p values from %.4f to %.4f\n") %
                     ffs[0].nvoxels % ffs[0].low % ffs[0].high)
                        .str();
            vbforeach(fdrstat ff, ffs) {
              if (ff.maxind >= 0)
                cout << (format("[I] vbvolregress: FDR threhsold for q=%.2f is "
                                "%.4f\n") %
                         ff.q % ff.statval)
                            .str();
              else
                cout << (format("[I] vbvolregress: no FDR threhsold could be "
                                "identified for q=%.2f\n") %
                         ff.q)
                            .str();
            }
          }
        }

        // set up the output filenames
        w_mapname = mapname;
        replace_string(w_mapname, permstring, perm_tag);
        replace_string(w_mapname, partstring, part_tag);
        replace_string(w_mapname, contstring, glmi.contrast.name);
        w_prmname = prmname;
        replace_string(w_prmname, permstring, perm_tag);
        replace_string(w_prmname, partstring, part_tag);
        replace_string(w_prmname, contstring, glmi.contrast.name);
        w_resname = resname;
        replace_string(w_resname, permstring, perm_tag);
        replace_string(w_resname, partstring, part_tag);
        replace_string(w_resname, contstring, glmi.contrast.name);

        // write stat values to dist file
        if (distname.size()) {
          if (appendline(
                  distname,
                  (format("%.20g") % glmi.statcube.get_maximum()).str())) {
            printf("[E] vbvolregress: couldn't write dist file\n");
            exit(101);
          }
        }
        // write stat map
        if (mapname.size()) {
          if (glmi.statcube.WriteFile(w_mapname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_mapname.c_str());
            exit(171);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_mapname.c_str());
        }
        // write param file
        if (prmname.size()) {
          if (glmi.paramtes.WriteFile(w_prmname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_prmname.c_str());
            exit(172);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_prmname.c_str());
        }
        // write resids
        if (resname.size()) {
          if (glmi.residtes.WriteFile(w_resname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_resname.c_str());
            exit(173);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_resname.c_str());
        }
        printf("[I] vbvolregress: done with %s\n", glmi.stemname.c_str());
      } else {
        // do vector regression
        glmi.rescount = 0;
        int err = glmi.VecRegress(ivnames, dvname);
        if (err) {
          printf("[E] vbvolregress (vec): failed with %s (%d)!\n",
                 glmi.stemname.c_str(), err);
          exit(err);
        }
        glmi.calc_stat();
        VB_Vector statvec(1);
        statvec[0] = glmi.statval;

        // set up the output filenames
        w_mapname = mapname;
        replace_string(w_mapname, permstring, perm_tag);
        replace_string(w_mapname, partstring, part_tag);
        replace_string(w_mapname, contstring, glmi.contrast.name);
        w_prmname = prmname;
        replace_string(w_prmname, permstring, perm_tag);
        replace_string(w_prmname, partstring, part_tag);
        replace_string(w_prmname, contstring, glmi.contrast.name);
        w_resname = resname;
        replace_string(w_resname, permstring, perm_tag);
        replace_string(w_resname, partstring, part_tag);
        replace_string(w_resname, contstring, glmi.contrast.name);

        // write stat values to dist file
        if (distname.size()) {
          if (appendline(distname, (format("%.20g") % statvec[0]).str())) {
            printf("[E] vbvolregress: couldn't write dist file\n");
            exit(101);
          }
        }
        // write stat, param, and resid vecs
        if (mapname.size()) {
          if (statvec.WriteFile(w_mapname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_mapname.c_str());
            exit(171);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_mapname.c_str());
        }

        // write param file
        if (prmname.size()) {
          if (glmi.betas.WriteFile(w_prmname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_prmname.c_str());
            exit(172);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_prmname.c_str());
        }
        // write resids
        if (resname.size()) {
          if (glmi.residuals.WriteFile(w_resname)) {
            printf("[E] vbvolregress: error writing file %s\n",
                   w_resname.c_str());
            exit(173);
          } else
            printf("[I] vbvolregress: wrote file %s\n", w_resname.c_str());
        }
      }
    }
  }
  if (distname.size() && vb_fileexists(distname))
    cout << "[I] vbvolregress: wrote distribution file " << distname << endl;

  exit(0);
}

void vbvolregress_help() { cout << boost::format(myhelp) % vbversion; }

void vbvolregress_version() {
  printf("VoxBo vbvolregress (v%s)\n", vbversion.c_str());
}
