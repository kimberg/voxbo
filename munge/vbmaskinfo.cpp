
// vbmaskinfo.cpp
// mask/lesion summary utility, mostly for counting unique voxels
// Copyright (c) 2005-2011 by The VoxBo Development Team

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

#include <math.h>
#include <map>
#include "glmutil.h"
#include "vbio.h"
#include "vbmaskinfo.hlp.h"
#include "vbutil.h"
#include "vbversion.h"

class pdata {
 public:
  pdata() { voxels = 0; }
  int voxels;
  VBVoxel firstvox;
};

int reduce(Tes &ts, Cube mymask, size_t npats, int minpatients, string matfile,
           double maxrmul, double countweight, string mapfile);
void vbmaskinfo_help();
void vbmaskinfo_version();

class img {
 public:
  Tes ts;
  Cube cb;
  int pos;
};

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbmaskinfo_help();
    exit(0);
  }

  int minpatients = 1;
  size_t npats = 0;  // number of vectors to keep
  string matfile, maskfile, mapfile;
  double maxrmul = 1.0 - FLT_MIN;
  double countweight = 1.0;
  size_t nrandom = 0;
  string coordfile;

  tokenlist args, filelist;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-r" && i < args.size() - 5) {
      matfile = args[++i];
      mapfile = args[++i];
      npats = strtol(args[++i]);
      maxrmul = strtod(args[++i]);
      countweight = strtod(args[++i]);
      // if it's >=1, set it to special >1 value
      if (maxrmul >= 1.0) maxrmul = 3.0;
      // otherwise, cap it at just under 1
      else if (1.0 - maxrmul < FLT_MIN)
        maxrmul = 1.0 - FLT_MIN;
    } else if (args[i] == "-s" && i < args.size() - 2) {
      coordfile = args[++i];
      nrandom = strtol(args[++i]);
    } else if (args[i] == "-h") {
      vbmaskinfo_help();
      exit(0);
    } else if (args[i] == "-m" && i < args.size() - 1)
      maskfile = args[++i];
    else if (args[i] == "-v") {
      vbmaskinfo_version();
      exit(0);
    } else if (args[i] == "-n" && i < args.size() - 1) {
      pair<bool, int32> mm = strtolx(args[++i]);
      if (mm.first) {
        cout << format("[E] vbmaskinfo: non-numeric argument to -n: %s\n") %
                    args[i];
        exit(98);
      }
      minpatients = mm.second;
    } else if (args[i] == "-m" && i < args.size() - 1) {
      minpatients = strtol(args[++i]);
    } else if (args[i][0] == '-') {
      cout << format("[E] vbmaskinfo: couldn't handle flag %s\n") % args[i];
      exit(122);
    } else
      filelist.Add(args[i]);
  }

  Tes bigtes, tes;
  Cube cube, mask;
  int dimx = 0, dimy = 0, dimz = 0, dimt = 0;
  if (maskfile.size()) {
    if (mask.ReadFile(maskfile)) {
      cout << format("[E] vbmaskinfo: error reading mask file %s\n") % maskfile;
      exit(101);
    }
    cout << format("[I] vbmaskinfo: masking with %s\n") % maskfile;
    dimx = mask.dimx;
    dimy = mask.dimy;
    dimz = mask.dimz;
  }

  cout << format("[I] vbmaskinfo: minimum lesions for inclusion: %d\n") %
              minpatients;

  for (size_t i = 0; i < filelist.size(); i++) {
    if (tes.ReadHeader(filelist[i]) == 0) {
      if (dimx == 0) {
        dimx = tes.dimx;
        dimy = tes.dimy;
        dimz = tes.dimz;
      }
      if (tes.dimx != dimx || tes.dimy != dimy || tes.dimz != dimz) {
        printf("[E] vbmaskinfo: inconsistent image dimensions\n");
        exit(109);
      }
      dimt += tes.dimt;
    } else if (cube.ReadHeader(filelist[i]) == 0) {
      if (dimx == 0) {
        dimx = cube.dimx;
        dimy = cube.dimy;
        dimz = cube.dimz;
      }
      if (cube.dimx != dimx || cube.dimy != dimy || cube.dimz != dimz) {
        printf("[E] vbmaskinfo: inconsistent image dimensions\n");
        exit(109);
      }
      dimt++;
    } else {
      printf("[E] vbmaskinfo: couldn't read %s\n", filelist[i].c_str());
      exit(109);
    }
  }
  if (dimt == 0) {
    printf("[E] vbmaskinfo: no valid volumes read\n");
    exit(129);
  }

  img myimages[filelist.size()];

  int pos = 0;
  for (size_t i = 0; i < filelist.size(); i++) {
    if (myimages[i].ts.ReadFile(filelist[i]) == 0) {
      myimages[i].pos = pos;
      pos += myimages[i].ts.dimt;
    } else if (myimages[i].cb.ReadFile(filelist[i]) == 0) {
      myimages[i].pos = pos;
      pos++;
    } else {
      printf("[E] vbmaskinfo: couldn't read %s\n", filelist[i].c_str());
      exit(109);
    }
  }

  if (npats > 0 && filelist.size() == 1) {
    if (tes.ReadFile(filelist[0])) {
      exit(999);
    }
    reduce(tes, mask, npats, minpatients, matfile, maxrmul, countweight,
           mapfile);
    exit(0);
  }

  printf("[I] vbmaskinfo: counting patterns (min patients %d)\n", minpatients);
  map<bitmask, pdata> patterns;
  vector<bitmask> allbitmasks;

  int includedvoxels = 0;
  VB_Vector myvec(dimt);

  int i, j, k, t;
  bitmask bm;
  bm.resize(dimt);
  int16 val;
  bool f_non1 = 0;
  for (i = 0; i < dimx; i++) {
    for (j = 0; j < dimy; j++) {
      for (k = 0; k < dimz; k++) {
        if (mask)
          if (!(mask.testValue(i, j, k))) continue;
        bm.clear();

        for (size_t v = 0; v < filelist.size(); v++) {
          if (myimages[v].ts.data) {
            for (t = 0; t < myimages[v].ts.dimt; t++) {
              val = myimages[v].ts.getValue<int16>(i, j, k, t);
              if (val) {
                bm.set(myimages[v].pos + t);
                if (val != 1) f_non1 = 1;
              }
            }
          } else {
            val = myimages[v].cb.getValue<int16>(i, j, k);
            if (val) {
              bm.set(myimages[v].pos);
              if (val != 1) f_non1 = 1;
            }
          }
        }
        if (bm.count() < minpatients) continue;
        // get a reference to the data (pdata) for this bitmask
        pdata *pv = &(patterns[bm]);
        if (pv->voxels == 0) {
          pv->firstvox.init(i, j, k);
          allbitmasks.push_back(bm);
        }
        pv->voxels++;
        includedvoxels++;
      }
    }
  }
  int voxeltotal[dimt + 1];   // number of voxels with n lesions
  int uniquetotal[dimt + 1];  // number of unique voxels with n lesions
  for (int i = 0; i < dimt + 1; i++) voxeltotal[i] = uniquetotal[i] = 0;
  for (map<bitmask, pdata>::iterator pp = patterns.begin();
       pp != patterns.end(); pp++) {
    int cnt = pp->first.count();
    voxeltotal[cnt] += pp->second.voxels;
    uniquetotal[cnt]++;
  }
  if (f_non1) printf("[W] non-0/1 values found in masks\n");
  printf("[I] total patients: %d\n", dimt);
  printf("[I] total voxels in volume: %d\n", dimx * dimy * dimz);
  printf("[I] voxels with included data: %d\n", includedvoxels);
  printf("[I] uncounted voxels: %d\n", (dimx * dimy * dimz) - includedvoxels);
  printf("[I] total distinct voxels: %d\n", (int)patterns.size());
  for (int i = minpatients; i < dimt + 1; i++) {
    if (uniquetotal[i] > 0)
      printf("[I] voxels with %d lesions: %d voxels, %d distinct voxels\n", i,
             voxeltotal[i], uniquetotal[i]);
  }
  printf(
      "[I] Distinct voxels are voxels with different patterns of lesioned\n");
  printf(
      "[I] patients.  Sets of voxels in which exactly the same patients are\n");
  printf("[I] lesioned are counted as a single distinct voxel.\n");

  // select n random voxels if that's what we're doing
  set<uint32> indices;
  if (nrandom > 0) {
    VBMatrix coords(nrandom, 3);
    gsl_rng *rng;
    rng = gsl_rng_alloc(gsl_rng_mt19937);
    assert(rng);
    gsl_rng_set(rng, VBRandom());
    for (uint32 i = 0; i < nrandom; i++) {
      // if (i%1000==0) gsl_rng_set(rng,VBRandom());  // re-seed every 1000?
      uint32 ind = lround(gsl_ran_flat(rng, 0, allbitmasks.size() - 1));
      indices.insert(ind);
      pdata mydat = patterns[allbitmasks[ind]];
      coords.set(i, 0, mydat.firstvox.x);
      coords.set(i, 1, mydat.firstvox.y);
      coords.set(i, 2, mydat.firstvox.z);
    }
    cout << "unique indices: " << indices.size() << endl;
    if (coords.WriteFile(coordfile)) {
      cout << "[E] vbmaskinfo: error writing coordinate file " << coordfile
           << endl;
      exit(22);
    }
  }

  exit(0);
}

int reduce(Tes &ts, Cube mymask, size_t npats, int minpatients, string matfile,
           double maxrmul, double countweight, string mapfile) {
  Cube mask, colormap;
  bitmask bm;
  map<bitmask, double> patterns;
  map<bitmask, double> keptpatterns;
  pair<bitmask, double> pat;
  int dimt;
  VBMatrix kpmat;
  int mincount, maxcount;
  double minvar, maxvar;
  bool f_scale = 0;
  double varweight;

  if (countweight < 0.0 || countweight > 100.0) {
    cout << format("[E] vbmaskinfo: invalid count weight %g\n") % countweight;
    exit(4);
  }
  if (countweight > 1.0) countweight /= 100.0;
  varweight = 1.0 - countweight;
  if (abs(countweight) > FLT_MIN && abs(varweight) > FLT_MIN) f_scale = 1;

  cout << format("[I] vbmaskinfo: countweight=%g  varianceweight=%g\n") %
              countweight % varweight;

  // the color map will eventually have a map of the voxels included
  // in the patterns, with a different color (int) for each pattern
  colormap.SetVolume(ts.dimx, ts.dimy, ts.dimz, vb_int32);
  set<int32> keptcolors;
  map<bitmask, int32> colorkey;
  dimt = ts.dimt;
  bm.resize(dimt);
  ts.ExtractMask(mask);
  if (mymask) mask.intersect(mymask);
  // collect all the patterns, with counts
  int32 nextcolor = 1;
  for (int i = 0; i < ts.dimx; i++) {
    for (int j = 0; j < ts.dimy; j++) {
      for (int k = 0; k < ts.dimz; k++) {
        if (!mask.testValue(i, j, k)) continue;
        bm.clear();
        for (int t = 0; t < ts.dimt; t++) {
          if (ts.GetValueUnsafe(i, j, k, t) > FLT_MIN) bm.set(t);
        }
        if (bm.count() < minpatients) continue;
        if (patterns.count(bm)) {
          patterns[bm]++;
          colormap.SetValue(i, j, k, colorkey[bm]);
        } else {
          patterns[bm] = 1;
          colorkey[bm] = nextcolor;
          colormap.SetValue(i, j, k, nextcolor);
          nextcolor++;
        }
      }
    }
  }

  // get min and max stuff.  to this point, the pattern is stored with
  // the count.
  minvar = maxvar = VB_Vector(patterns.begin()->first).getVariance();
  mincount = maxcount = patterns.begin()->second;
  double var;
  vbforeach(pat, patterns) {
    var = VB_Vector(pat.first).getVariance();
    if (var < minvar) minvar = var;
    if (var > maxvar) maxvar = var;
    if (pat.second < mincount) mincount = pat.second;
    if (pat.second > maxcount) maxcount = pat.second;
  }

  vbforeach(pat, patterns) {
    double wvar = VB_Vector(pat.first).getVariance();
    double wcount = pat.second;
    if (f_scale) {
      wvar -= minvar;
      wvar /= maxvar - minvar;
      wcount -= mincount;
      wcount /= maxcount - mincount;
    }
    patterns[pat.first] = (varweight * wvar) + (countweight * wcount);
  }

  cout << format("%d total patterns\n") % patterns.size();
  // now do our stuff
  int removed = 0;
  double removedtotal = 0, kepttotal = 0;
  VB_Vector v1(dimt);
  // icol is created so that we can add an intercept easily
  VB_Vector icol(dimt);
  icol += 1;
  v1 += 1;
  while (1) {
    if (patterns.size() == 0) break;

    // find the largest score
    double maxcnt = 0;
    bitmask maxpat;
    vbforeach(pat, patterns) {
      if (pat.second > maxcnt || maxpat.size() == 0) {
        maxpat = pat.first;
        maxcnt = pat.second;
      }
    }
    // now we have the largest score, we either add or remove it (we
    // always keep the single largest)
    if (keptpatterns.size() == 0) {
      keptpatterns[maxpat] = maxcnt;
      kepttotal += maxcnt;
      patterns.erase(maxpat);
      kpmat.resize(dimt, 2);
      kpmat.SetColumn(0, icol);
      kpmat.SetColumn(1, VB_Vector(maxpat));
      cout << format("pattern added: %d kept, %d left") % keptpatterns.size() %
                  patterns.size()
           << endl;
      continue;
    }
    // otherwise see if it's linearly dependent on previously kept pats
    // FIRST, convert our set into a matrix
    VB_Vector checkvec(maxpat);
    double rmul;
    if (maxrmul < 1.9)
      rmul = calcColinear(kpmat, checkvec);
    else
      rmul = 0.000001;  // should always be kept
    if (rmul > maxrmul || rmul < 0 || abs(rmul - 1.0) < FLT_MIN) {
      cout << format("pattern with rmul of %.2f removed: ") % rmul;
      removed++;
      removedtotal += pat.second;
    } else {
      keptpatterns[maxpat] = maxcnt;
      kepttotal += maxcnt;
      cout << "pattern    kept: ";
      // rebuild the matrix of kept patterns
      kpmat.resize(dimt, keptpatterns.size() + 1);
      kpmat.SetColumn(0, icol);
      int ind = 1;
      vbforeach(pat, keptpatterns) kpmat.SetColumn(ind++, VB_Vector(pat.first));
    }
    patterns.erase(maxpat);
    cout << format("%g score, %d count, %d now kept, %d now left") % maxcnt %
                maxpat.count() % keptpatterns.size() % patterns.size()
         << endl;
    if (keptpatterns.size() >= npats) break;
  }
  cout << format("final pattern count: %d (%g total score)\n") %
              keptpatterns.size() % kepttotal;
  cout << format("final patterns removed: %d (%g total score)\n") % removed %
              removedtotal;
  if (keptpatterns.size()) {
    VBMatrix m(keptpatterns.begin()->first.size(), keptpatterns.size());
    int col = 0;
    pair<bitmask, int> bb;
    vbforeach(bb, keptpatterns) { m.SetColumn(col++, VB_Vector(bb.first)); }
    if (m.WriteFile(matfile)) {
      cout << format("[E] vbmaskinfo: error writing feature file %s\n") %
                  matfile;
      exit(201);
    } else {
      cout << format("[I] vbmaskinfo: wrote feature file %s\n") % matfile;
    }
    if (mapfile != "none") {
      // build set of colorkeys associated with kept patterns
      vbforeach(pat, keptpatterns) {
        cout << "keeping " << colorkey[pat.first] << endl;
        keptcolors.insert(colorkey[pat.first]);
      }
      for (int i = 0; i < ts.dimx * ts.dimy * ts.dimz; i++) {
        int32 val = colormap.getValue<int32>(i);
        if (!(keptcolors.count(val))) colormap.setValue(i, 0);
      }
      if (colormap.WriteFile(mapfile)) {
        cout << format("[E] vbmaskinfo: error writing colormap %s\n") % matfile;
        exit(201);
      } else {
        cout << format("[I] vbmaskinfo: wrote colormap %s\n") % matfile;
      }
    }
  }
  exit(0);
}

void vbmaskinfo_help() { cout << boost::format(myhelp) % vbversion; }

void vbmaskinfo_version() {
  printf("VoxBo vbmaskinfo (v%s)\n", vbversion.c_str());
}
