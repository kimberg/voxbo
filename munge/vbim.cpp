
// vbim.cpp
// general-purpose munging util for voxbo (used to be vbim)
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

#include <fstream>
#include "imageutils.h"
#include "vbim.hlp.h"
#include "vbio.h"
#include "vbutil.h"
#include "vbversion.h"

void vbim_help(string key = "");
void vbim_version();

int cube_combine(Cube &cb, tokenlist &args);
void selectcubes(list<Cube> &clist, size_t n);
bool comparedims(list<Cube> cubelist);
void cube_random01(Cube &cb);
void cube_printregioninfo(Cube &cb, tokenlist &args);
void reallyload(Cube &cube);
void build_oplist();

// ugly globals -- code needs some reorg
Cube mycube;
VBVoxel myvoxel;
int multi_index;
bool f_read4d = 0;
gsl_rng *rng;

class imageop {
 public:
  string name;
  size_t minargs, maxargs;
  tokenlist args;
  // generally, an operator is combining if either initfn or finishfn is
  // non-null
  vbreturn (*initfn)(list<Cube> &cubelist, tokenlist &args);
  vbreturn (*procfn)(Cube &cube, tokenlist &args);
  vbreturn (*finishfn)(list<Cube> &cubelist, tokenlist &args);
  bool f_ncwithargs;  // if set, this op becomes non-combining when called with
                      // arguments
  // storage for combining operators
  // sole constructor requires name and min/maxargs
  imageop();
  imageop(string xname, int mina, int maxa,
          vbreturn (*xprocfn)(Cube &, tokenlist &));
  void init(string xname, int mina, int maxa,
            vbreturn (*xprocfn)(Cube &, tokenlist &));
  // random methods
  bool iscombining() { return (initfn != NULL || finishfn != NULL); }
};

imageop::imageop() { init("", 0, 0, NULL); }

imageop::imageop(string xname, int mina, int maxa,
                 vbreturn (*xprocfn)(Cube &, tokenlist &)) {
  init(xname, mina, maxa, xprocfn);
}

void imageop::init(string xname, int mina, int maxa,
                   vbreturn (*xprocfn)(Cube &, tokenlist &)) {
  name = xname;
  minargs = mina;
  maxargs = maxa;
  args.clear();
  initfn = NULL;
  procfn = xprocfn;
  finishfn = NULL;
  f_ncwithargs = 0;
}

// two operator lists that are handled differently: in phase 1, we do
// as many operations as possible for each cube; in phase 2, we do
// each operator for each cube before proceeding to the next operator.
list<imageop> phase1;
list<imageop> phase2;
typedef list<imageop>::iterator OPI;
typedef list<Cube>::iterator CUBI;

map<string, imageop> oplist;

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    vbim_help();
    exit(0);
  }
  build_oplist();
  tokenlist args;
  list<string> filelist;
  // string outfile;
  args.Transfer(argc - 1, argv + 1);
  int phase = 1;
  string opname;
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h" || args[i] == "--help") {
      vbim_help(args[i + 1]);
      exit(0);
    }
    if (args[i] == "-v" || args[i] == "--version") {
      vbim_version();
      exit(0);
    }
    if (args[i].substr(0, 2) == "--")
      opname = args[i].substr(2, string::npos);
    else if (args[i][0] == '-')
      opname = args[i].substr(1, string::npos);
    else {
      // non-flag arguments before first flag are considered infiles
      filelist.push_back(args[i]);
      continue;
    }
    // special case files -- not an op because of deferred loading
    if (opname == "files" || opname == "infiles" || opname == "input" ||
        opname == "f" || opname == "i") {
      while (i + 1 < args.size()) {
        if (args[i + 1][0] == '-') break;
        i++;
        filelist.push_back(args[i]);
      }
      continue;
    }

    if (oplist.count(opname) == 0) {
      cout << format("[E] vbim: unknown operator %s\n") % args[i];
      exit(202);
    }

    imageop myop = oplist[opname];
    myop.args.Add(opname);  // probably pointless
    // read all the args
    while (i + 1 < args.size()) {
      if (args[i + 1][0] == '-' && myop.args.size() - 1 >= myop.minargs) break;
      i++;
      myop.args.Add(args[i]);
    }
    // sanity check arg count
    if (myop.args.size() - 1 < myop.minargs ||
        myop.args.size() - 1 > myop.maxargs) {
      cout << format("[E] vbim: operator %s takes from %d to %d arguments\n") %
                  opname % myop.minargs % myop.maxargs;
      exit(202);
    }
    // special case operators that become non-combining with arguments
    if (myop.f_ncwithargs && myop.args.size() > 1) {
      myop.initfn = NULL;
      myop.finishfn = NULL;
    }

    // now decide which list gets the op
    if (phase == 2) phase2.push_back(myop);
    // if we're in phase 1 and it's parallel, just put it on
    else if (!myop.iscombining())
      phase1.push_back(myop);
    // if we're in phase 1 and it's combining, see if we have any parallels
    // first
    else {
      if (phase1.size())
        phase1.push_back(myop);
      else
        phase2.push_back(myop);
      phase = 2;
    }
  }

  list<Cube> cubelist;
  Cube master;

  // pre-load all the files.  we should figure out how not to load all
  // the image data from 4D files, maybe.

  for (list<string>::iterator ff = filelist.begin(); ff != filelist.end();
       ff++) {
    printf("[I] vbim: reading file %s\n", ff->c_str());
    Cube cb;
    Tes ts;
    if (cb.ReadHeader(*ff) == 0) {
      cubelist.push_back(cb);
      continue;
    }
    if (ts.ReadFile(*ff)) {
      printf("[E] vbim: couldn't read file %s, continuing anyway\n",
             ff->c_str());
      continue;
    }
    f_read4d = 1;
    for (int i = 0; i < ts.dimt; i++) {
      Cube tmpc;
      cubelist.push_back(tmpc);
      ts.getCube(i, cubelist.back());
    }
  }
  if (cubelist.size() == 0 &&
      (phase1.size() || phase2.front().name != "newvol")) {
    printf("[E] vbim: no valid input cubes found\n");
    exit(11);
  }

  // first do phase 1
  int index = 0;
  int phase1cflag = 0;
  if (phase1.size()) {
    // if (phase1.back().iscombining() && phase1.back().initfn)
    // phase1.back().initfn(cubelist,phase1.back().args);
    for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
      cc->id1 = index;
      for (OPI oo = phase1.begin(); oo != phase1.end(); oo++) {
        if (oo->initfn && !phase1cflag) {
          oo->initfn(cubelist, oo->args);
          phase1cflag = 1;
        }
        if (oo->procfn) oo->procfn(*cc, oo->args);
      }
      index++;
      // invalidate cube if we have no more use for it
      if (phase2.empty() && !(phase1.back().finishfn)) cc->invalidate();
    }
    if (phase1.back().iscombining() && phase1.back().finishfn)
      phase1.back().finishfn(cubelist, phase1.back().args);
  }
  // if we have a phase2 block, now do that
  for (OPI oo = phase2.begin(); oo != phase2.end(); oo++) {
    if (oo->initfn) oo->initfn(cubelist, oo->args);
    index = 0;
    for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
      cc->id1 = index;
      if (oo->procfn) oo->procfn(*cc, oo->args);
      index++;
    }
    if (oo->finishfn) oo->finishfn(cubelist, oo->args);
  }

  exit(0);
}

// for "combine" the arguments are:
// combine fixed/moving x y z op value

template <class T>
int cube_combine_fixed(Cube &cb, tokenlist &args) {
  int xx = strtol(args[2]);
  int yy = strtol(args[3]);
  int zz = strtol(args[4]);
  string method = args[5];
  int f_needval = 0;
  if (method == "sum" || method == "average") f_needval = 1;

  if (xx < 1 || yy < 1 || zz < 1) {
    printf("[E] vbim: invalid subcube dimensions\n");
    return 101;
  }

  Cube cb2(cb);
  // initial subcube range 0:xx-1, etc.
  int x1 = 0, y1 = 0, z1 = 0;
  int xn = xx - 1, yn = yy - 1, zn = zz - 1;

  int i, j, k, count;
  double total;
  T val;
  while (1) {
    count = 0;
    total = 0.0;
    if (x1 > cb.dimx - 1 || y1 > cb.dimy - 1 || z1 > cb.dimz - 1) break;
    if (xn > cb.dimx - 1) xn = cb.dimx - 1;
    if (yn > cb.dimy - 1) yn = cb.dimy - 1;
    if (zn > cb.dimz - 1) zn = cb.dimz - 1;
    for (i = x1; i <= xn; i++) {
      for (j = y1; j <= yn; j++) {
        for (k = z1; k <= zn; k++) {
          if (f_needval) {
            val = cb.getValue<T>(i, j, k);
            if (val > 0.0) count++;
            total += val;
          } else if (cb.testValue(i, j, k))
            count++;
        }
      }
    }
    // calculate the replacement value -- sum, count, average,
    // pct
    T newvalue = 0;
    int rsize = (xn - x1 + 1) * (yn - y1 + 1) * (zn - z1 + 1);
    if (method == "count")
      newvalue = count;
    else if (method == "sum")
      newvalue = total;
    else if (method == "pct")
      newvalue = (count * 100) / rsize;
    else if (method == "average")
      newvalue = total / rsize;
    else
      newvalue = 0;
    // all voxels in the subcube get the identical new value
    for (i = x1; i <= xn; i++) {
      for (j = y1; j <= yn; j++) {
        for (k = z1; k <= zn; k++) {
          cb2.setValue<T>(i, j, k, newvalue);
        }
      }
    }
    // next subcube
    if (xn < cb.dimx - 1) {
      x1 += xx;
      xn += xx;
    } else if (yn < cb.dimy - 1) {
      x1 = 0;
      xn = xx - 1;
      y1 += yy;
      yn += yy;
    } else if (zn < cb.dimz - 1) {
      x1 = y1 = 0;
      xn = xx - 1;
      yn = yy - 1;
      z1 += zz;
      zn += zz;
    } else
      break;
  }
  cb = cb2;
  return 0;
}

template <class T>
int cube_combine_moving(Cube &cb, tokenlist &args) {
  int xx = strtol(args[2]);
  int yy = strtol(args[3]);
  int zz = strtol(args[4]);
  string method = args[5];
  int f_needval = 0;
  if (method == "sum" || method == "average") f_needval = 1;

  if (xx < 1 || yy < 1 || zz < 1) {
    printf("[E] vbim: invalid subcube dimensions\n");
    return 101;
  }

  Cube cb2(cb);
  // initial subcube range 0:xx-1, etc.
  int x1 = 0, y1 = 0, z1 = 0;
  int xn = xx - 1, yn = yy - 1, zn = zz - 1;

  int i, j, k, count;
  double total;
  T val;

  for (int xxx = 0; xxx <= cb.dimx; xxx++) {
    for (int yyy = 0; yyy <= cb.dimy; yyy++) {
      for (int zzz = 0; zzz <= cb.dimz; zzz++) {
        count = 0;
        total = 0.0;
        // build a neighborhood around the voxel
        x1 = xxx - xx / 2;
        xn = x1 + xx - 1;
        y1 = yyy - yy / 2;
        yn = y1 + yy - 1;
        z1 = zzz - zz / 2;
        zn = z1 + zz - 1;
        // clip it to cube boundaries
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (z1 < 0) z1 = 0;
        if (xn > cb.dimx - 1) xn = cb.dimx - 1;
        if (yn > cb.dimy - 1) yn = cb.dimy - 1;
        if (zn > cb.dimz - 1) zn = cb.dimz - 1;
        for (i = x1; i <= xn; i++) {
          for (j = y1; j <= yn; j++) {
            for (k = z1; k <= zn; k++) {
              if (f_needval) {
                val = cb.getValue<T>(i, j, k);
                if (val > 0.0) count++;
                total += val;
              } else if (cb.testValue(i, j, k))
                count++;
            }
          }
        }
        // calculate the replacement value -- sum, count, average,
        // pct
        T newvalue = 0;
        int rsize = (xn - x1 + 1) * (yn - y1 + 1) * (zn - z1 + 1);
        if (method == "count")
          newvalue = count;
        else if (method == "sum")
          newvalue = total;
        else if (method == "pct")
          newvalue = (count * 100) / rsize;
        else if (method == "average")
          newvalue = total / rsize;
        else
          newvalue = 0;
        // set just the original voxel
        cb2.setValue<T>(xxx, yyy, zzz, newvalue);
      }
    }
  }
  cb = cb2;
  return 0;
}

int cube_combine(Cube &cb, tokenlist &args) {
  if (args[1] == "fixed" || args[1] == "f") {
    switch (cb.datatype) {
      case vb_byte:
        return cube_combine_fixed<char>(cb, args);
        break;
      case vb_short:
        return cube_combine_fixed<int16>(cb, args);
        break;
      case vb_long:
        return cube_combine_fixed<int32>(cb, args);
        break;
      case vb_float:
        return cube_combine_fixed<float>(cb, args);
        break;
      case vb_double:
        return cube_combine_fixed<double>(cb, args);
        break;
    }
  } else if (args[1] == "moving" || args[1] == "m") {
    switch (cb.datatype) {
      case vb_byte:
        return cube_combine_moving<char>(cb, args);
        break;
      case vb_short:
        return cube_combine_moving<int16>(cb, args);
        break;
      case vb_long:
        return cube_combine_moving<int32>(cb, args);
        break;
      case vb_float:
        return cube_combine_moving<float>(cb, args);
        break;
      case vb_double:
        return cube_combine_moving<double>(cb, args);
        break;
    }
  }

  return 101;  // shouldn't happen
}

void cube_printregioninfo(Cube &cb, tokenlist &args) {
  bool f_masked = 0;
  vector<VBRegion> rlist;
  if (args.size() > 1) {
    f_masked = 1;
    Cube mask;
    if (mask.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read mask %s\n", args(1));
      return;
    }
    if (!mask.data || mask.dimx != cb.dimx || mask.dimy != cb.dimy ||
        mask.dimz != cb.dimz) {
      printf("[E] vbim: non-matching dimensions for mask %s\n", args(1));
      return;
    }
    VBRegion r;
    for (int i = 0; i < mask.dimx; i++) {
      for (int j = 0; j < mask.dimy; j++) {
        for (int k = 0; k < mask.dimz; k++) {
          if (!mask.testValue(i, j, k)) continue;
          r.add(i, j, k, cb.GetValue(i, j, k));
        }
      }
    }
    rlist.push_back(r);
    VBRegion ro = r.maxregion();
    cout << ro.size() << " " << ro.begin()->second.val << endl;
  } else
    rlist = findregions(cb, vb_ne, 0.0);
  int totalvoxels = 0;
  for (size_t i = 0; i < rlist.size(); i++) totalvoxels += rlist[i].size();
  if (!f_masked)
    printf("[I] vbim: %d voxels in %d non-zero regions found in volume %s\n",
           totalvoxels, (int)(rlist.size()), cb.GetFileName().c_str());
  // make the combination region
  if (!f_masked) {
    VBRegion rx;
    rlist.insert(rlist.begin(), rx);
    vbforeach(VBRegion & r, rlist) { rlist.front().merge(r); }
  }
  double vfactor = cb.voxsize[0] * cb.voxsize[1] * cb.voxsize[2];
  for (size_t i = 0; i < rlist.size(); i++) {
    // calculate centers of mass
    double x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0, totalmass = 0;
    for (VI myvox = rlist[i].begin(); myvox != rlist[i].end(); myvox++) {
      x1 += myvox->second.x;
      y1 += myvox->second.y;
      z1 += myvox->second.z;
      x2 += myvox->second.x * myvox->second.val;
      y2 += myvox->second.y * myvox->second.val;
      z2 += myvox->second.z * myvox->second.val;
      totalmass += myvox->second.val;
    }
    x1 /= rlist[i].size();
    y1 /= rlist[i].size();
    z1 /= rlist[i].size();
    x2 /= totalmass;
    y2 /= totalmass;
    z2 /= totalmass;

    VBRegion peakrr = rlist[i].maxregion();
    VBRegion minrr = rlist[i].minregion();
    double pxx, pyy, pzz;
    peakrr.GeometricCenter(pxx, pyy, pzz);

    if (i == 0) {
      printf("[I] vbim:   all regions combined:\n");
      printf("[I] vbim:                 voxels: %d\n", rlist[i].size());
      if (vfactor > FLT_MIN)
        printf("[I] vbim:                    mm3: %g\n",
               vfactor * rlist[i].size());
      printf("[I] vbim:                    sum: %g\n", totalmass);
      printf("[I] vbim:                   mean: %g\n",
             totalmass / (double)rlist[i].size());
      printf("[I] vbim:                 center: %g,%g,%g\n", x1, y1, z1);
      printf("[I] vbim:           weighted ctr: %g,%g,%g\n", x2, y2, z2);
      printf("[I] vbim:                peakval: %g\n",
             peakrr.begin()->second.val);
      printf("[I] vbim:                peakcnt: %d\n", peakrr.size());
      printf("[I] vbim:                 minval: %g\n",
             minrr.begin()->second.val);
      printf("[I] vbim:                 mincnt: %d\n", minrr.size());
      printf("[I] vbim:            peak center: %g,%g,%g\n", pxx, pyy, pzz);
      printf(
          "[I] vbim: note: peakcnt (or lowcnt) is the total number of voxels "
          "in\n");
      printf("[I] vbim:       the region with the same peak (or min) value\n");
      continue;
    }
    printf("[I] vbim:   region %02d: count: %d\n", (int)i,
           (int)rlist[i].size());
    if (vfactor > FLT_MIN)
      printf("[I] vbim:                mm3: %g\n", vfactor * rlist[i].size());
    printf("[I] vbim:                sum: %g\n", totalmass);
    printf("[I] vbim:               mean: %g\n",
           totalmass / (double)rlist[i].size());
    printf("[I] vbim:             center: %g,%g,%g\n", x1, y1, z1);
    printf("[I] vbim:       weighted ctr: %g,%g,%g\n", x2, y2, z2);
    printf("[I] vbim:            peakval: %g\n", peakrr.begin()->second.val);
    printf("[I] vbim:            peakcnt: %d\n", peakrr.size());
    printf("[I] vbim:             minval: %g\n", minrr.begin()->second.val);
    printf("[I] vbim:             mincnt: %d\n", minrr.size());
    printf("[I] vbim:        peak center: %g,%g,%g\n", pxx, pyy, pzz);
    printf(
        "[I] vbim: note: peakcnt (or lowcnt) is the total number of voxels "
        "in\n");
    printf("[I] vbim:       the region with the same peak (or min) value\n");
    // totalvoxels+=rlist[i].size();
  }
  // printf("[I] vbim:   %d total voxels in volume
  // %s\n",totalvoxels,cb.GetFileName().c_str());
}

void cube_random01(Cube &cb) {
  uint32 rr = 0;
  int pos = 32;
  cb.zero();
  for (int i = 0; i < cb.dimx; i++) {
    for (int j = 0; j < cb.dimy; j++) {
      for (int k = 0; k < cb.dimz; k++) {
        if (pos > 31) {
          rr = VBRandom();
          pos = 0;
        }
        if (rr & 1 << pos) cb.SetValue(i, j, k, 1.0);
        pos++;
      }
    }
  }
}

void selectcubes(list<Cube> &clist, size_t n) {
  if (n >= clist.size() || n < 1) return;
  map<uint32, bool> randlist;
  while (randlist.size() < n) randlist[VBRandom()] = 1;
  while (randlist.size() < clist.size()) randlist[VBRandom()] = 0;
  map<uint32, bool>::iterator mm = randlist.begin();
  list<Cube>::iterator cc = clist.begin();
  int oldn = clist.size();
  for (int i = 0; i < oldn; i++) {
    if (mm->second)
      cc++;
    else
      cc = clist.erase(cc);
    mm++;
  }
}

bool comparedims(list<Cube> cubelist) {
  if (cubelist.size() < 2) return 1;
  list<Cube>::iterator cc = cubelist.begin();
  int dimx = cc->dimx;
  int dimy = cc->dimy;
  int dimz = cc->dimz;
  cc++;
  while (cc != cubelist.end()) {
    if (cc->dimx != dimx || cc->dimy != dimy || cc->dimz != dimz) return 0;
    cc++;
  }
  return 1;
}

// initfns

vbreturn op_shortzeros(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  mycube.SetVolume(cubelist.front().dimx, cubelist.front().dimy,
                   cubelist.front().dimz, vb_short);
  return 0;
}

vbreturn op_allones(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  reallyload(cubelist.front());
  mycube = cubelist.front();
  mycube.quantize(1.0);
  return 0;
}

vbreturn op_allzeros(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  reallyload(cubelist.front());
  mycube = cubelist.front();
  mycube.zero();
  mycube.convert_type(vb_double, 1);
  mycube.f_scaled = 0;
  return 0;
}

vbreturn op_initrng(list<Cube> &, tokenlist &args) {
  uint32 rngseed;
  if (args.size() > 3)
    rngseed = strtol(args[3]);
  else
    rngseed = VBRandom();
  rng = gsl_rng_alloc(gsl_rng_mt19937);
  assert(rng);
  gsl_rng_set(rng, rngseed);
  return 0;
}

vbreturn op_freerng(list<Cube> &, tokenlist &) {
  gsl_rng_free(rng);
  rng = NULL;
  return 0;
}

vbreturn op_copy(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  reallyload(cubelist.front());
  mycube = cubelist.front();
  return 0;
}

// procfn

vbreturn op_smoothvox(Cube &cube, tokenlist &args) {
  reallyload(cube);
  smoothCube(cube, strtod(args[1]), strtod(args[2]), strtod(args[3]));
  return 0;
}

vbreturn op_smoothvox2(Cube &cube, tokenlist &args) {
  reallyload(cube);
  smoothCube(cube, strtod(args[1]), strtod(args[2]), strtod(args[3]), 1);
  return 0;
}

vbreturn op_smoothmm(Cube &cube, tokenlist &args) {
  reallyload(cube);
  smoothCube(cube, strtod(args[1]) / cube.voxsize[0],
             strtod(args[2]) / cube.voxsize[1],
             strtod(args[3]) / cube.voxsize[2]);
  return 0;
}

vbreturn op_smoothmm2(Cube &cube, tokenlist &args) {
  reallyload(cube);
  smoothCube(cube, strtod(args[1]) / cube.voxsize[0],
             strtod(args[2]) / cube.voxsize[1],
             strtod(args[3]) / cube.voxsize[2], 1);
  return 0;
}

vbreturn op_shift(Cube &cube, tokenlist &args) {
  reallyload(cube);
  int xoff = strtol(args[1]);
  int yoff = strtol(args[2]);
  int zoff = strtol(args[3]);
  Cube tmp = cube;
  tmp.zero();
  int sx, sy, sz;
  for (int i = 0; i < cube.dimx; i++) {
    sx = i - xoff;
    if (sx < 0 || sx > cube.dimx - 1) continue;
    for (int j = 0; j < cube.dimy; j++) {
      sy = j - yoff;
      if (sy < 0 || sy > cube.dimy - 1) continue;
      for (int k = 0; k < cube.dimz; k++) {
        sz = k - zoff;
        if (sz < 0 || sz > cube.dimz - 1) continue;
        tmp.SetValue(i, j, k, cube.GetValue(sx, sy, sz));
      }
    }
  }
  cube = tmp;
  return 0;
}

vbreturn op_thresh(Cube &cube, tokenlist &args) {
  reallyload(cube);
  cube.thresh(strtod(args[1]));
  return 0;
}

vbreturn op_rotate(Cube &cube, tokenlist &args) {
  reallyload(cube);
  rotatecube(cube, strtod(args[1]), strtod(args[2]), strtod(args[3]));
  return 0;
}

vbreturn op_nonans(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.removenans();
  return 0;
}

vbreturn op_regionat(Cube &cube, tokenlist &args) {
  reallyload(cube);
  Cube mask;
  mask.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_short);
  mask += 1;
  VBRegion rr;
  rr = growregion(strtol(args[1]), strtol(args[2]), strtol(args[3]), cube, mask,
                  vb_agt, FLT_MIN);
  mask *= 0;
  for (VI myvox = rr.begin(); myvox != rr.end(); myvox++) {
    mask.setValue(myvox->second.x, myvox->second.y, myvox->second.z, (int32)1);
  }
  cube = mask;
  return 0;
}

// FIXME splitregions inexplicably writes output right away.
// it shouldn't!

vbreturn op_splitregions(Cube &cube, tokenlist &args) {
  reallyload(cube);
  vector<VBRegion> regions;
  regions = findregions(cube, vb_gt, 0.0);
  Cube mask;
  mask.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_byte);
  int index = 0;
  string fname;
  vbforeach(VBRegion & rr, regions) {
    mask.zero();
    for (VI myvox = rr.begin(); myvox != rr.end(); myvox++)
      mask.setValue<char>(myvox->second.x, myvox->second.y, myvox->second.z, 1);
    fname = args[1];
    string num = (format("%05d") % index).str();
    replace_string(fname, "XXX", num);
    mask.WriteFile(fname);
    index++;
  }
  cube = mask;
  return 0;
}

vbreturn op_subdivide(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (args.size() != 4) {
    cout << "[E] wrong number of arguments for subdivide\n";
    return 101;
  }
  // FIXME create the subcubes here
  Cube subcubes(cube.dimx, cube.dimy, cube.dimz, vb_int16);
  int cx = strtol(args[1]);
  int cy = strtol(args[2]);
  int cz = strtol(args[3]);
  int nx = cube.dimx / cx;
  if (cube.dimx % cx) nx++;
  int ny = cube.dimy / cy;
  if (cube.dimy % cy) ny++;
  int nz = cube.dimz / cz;
  if (cube.dimz % cz) nz++;

  int32 label = 1;
  for (int ii = 0; ii < nx; ii++) {
    for (int jj = 0; jj < ny; jj++) {
      for (int kk = 0; kk < nz; kk++) {
        // iterate within subcube
        for (int i = ii * cx; i < (ii + 1) * cx; i++) {
          if (i >= cube.dimx) break;
          for (int j = jj * cy; j < (jj + 1) * cy; j++) {
            if (j >= cube.dimy) break;
            for (int k = kk * cz; k < (kk + 1) * cz; k++) {
              if (k >= cube.dimz) break;
              subcubes.setValue<int32>(i, j, k, label);
            }
          }
        }
        label++;
      }
    }
  }

  int32 v1, v2;
  map<twovals, int32> submaskvals;
  Cube newcube(cube.dimx, cube.dimy, cube.dimz, vb_int16);
  label = 1;
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        v1 = cube.getValue<int32>(i, j, k);
        if (!v1) continue;
        v2 = subcubes.getValue<int32>(i, j, k);
        if (!submaskvals.count(twovals(v1, v2)))
          submaskvals[twovals(v1, v2)] = label++;
        newcube.setValue<int32>(i, j, k, submaskvals[twovals(v1, v2)]);
      }
    }
  }
  cout << format(
              "[I] vbim: subdivided your volume into %d distinct regions\n") %
              (label - 1);
  cube = newcube;
  return 0;
}

vbreturn op_threshabs(Cube &cube, tokenlist &args) {
  reallyload(cube);
  cube.threshabs(strtod(args[1]));
  return 0;
}

vbreturn op_cutoff(Cube &cube, tokenlist &args) {
  reallyload(cube);
  cube.cutoff(strtod(args[1]));
  return 0;
}

vbreturn op_orient(Cube &cube, tokenlist &) {
  reallyload(cube);
  printf("orient not implemented\n");
  exit(5);
  return 0;
}

vbreturn op_setspace(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (args.size() < 2) {
    cube.init_nifti();
    return 0;
  }
  if (args.size() == 2 && vb_tolower(args[1]) == "acp") {
    string acp = cube.GetHeader("AbsoluteCornerPosition:");
    tokenlist acpx;
    acpx.ParseLine(acp);
    if (acpx.size() != 3) {
      cout << format("[E] vbim: couldn't get acp from file %s\n") % args[1];
      exit(102);
    }
    cube.init_nifti();
    double xx, yy, zz;
    xx = strtod(acpx[0]);
    yy = strtod(acpx[1]);
    zz = strtod(acpx[2]);
    cube.qform_code = 2;
    cube.sform_code = 2;
    cube.qoffset[0] = 0 - xx;
    cube.qoffset[1] = 0 - yy;
    cube.qoffset[2] = 0 - zz;
    cube.origin[0] = (int32)(0 - xx);
    cube.origin[1] = (int32)(0 - yy);
    cube.origin[2] = (int32)(0 - zz);
    cube.srow_x[3] = 0 - xx;
    cube.srow_y[3] = 0 - yy;
    cube.srow_z[3] = 0 - zz;
    return 0;
  }
  VBImage *im = NULL;
  Cube cb;
  Tes ts;
  if (!cb.ReadFile(args[1]))
    im = &cb;
  else if (!ts.ReadFile(args[1]))
    im = &ts;
  if (!im) {
    cout << format("[E] vbim: couldn't read space from file %s\n") % args[1];
    exit(101);
  }
  cube.voxsize[0] = im->voxsize[0];
  cube.voxsize[1] = im->voxsize[1];
  cube.voxsize[2] = im->voxsize[2];
  cube.voxsize[3] = im->voxsize[3];
  cube.origin[0] = im->origin[0];
  cube.origin[1] = im->origin[1];
  cube.origin[2] = im->origin[2];
  cube.qform_code = im->qform_code;
  cube.sform_code = im->sform_code;
  cube.qoffset[0] = im->qoffset[0];
  cube.qoffset[1] = im->qoffset[1];
  cube.qoffset[2] = im->qoffset[2];
  cube.quatern_b = im->quatern_b;
  cube.quatern_c = im->quatern_c;
  cube.quatern_d = im->quatern_d;
  cube.srow_x[0] = im->srow_x[0];
  cube.srow_x[1] = im->srow_x[1];
  cube.srow_x[2] = im->srow_x[2];
  cube.srow_x[3] = im->srow_x[3];
  cube.srow_y[0] = im->srow_y[0];
  cube.srow_y[1] = im->srow_y[1];
  cube.srow_y[2] = im->srow_y[2];
  cube.srow_y[3] = im->srow_y[3];
  cube.srow_z[0] = im->srow_z[0];
  cube.srow_z[1] = im->srow_z[1];
  cube.srow_z[2] = im->srow_z[2];
  cube.srow_z[3] = im->srow_z[3];
  cube.orient = im->orient;
  return 0;
}

vbreturn op_randomvoxel(Cube &cube, tokenlist &args) {
  // reallyload(cube);  // don't need it, just the header
  Cube cb;
  if (cb.ReadFile(args[1])) {
    cout << format("[E] vbim: couldn't read randomvoxel mask file from %s\n") %
                args[1];
    exit(101);
  }
  if (!cube.dimsequal(cb)) {
    cout << format("[E] vbim: bad dimensions for randomvoxel mask file %s\n") %
                args[1];
    exit(101);
  }
  uint64 maskcount = 0;
  for (int64 i = 0; i < cb.dimx * cb.dimy * cb.dimz; i++)
    if (cb.testValue(i)) maskcount++;
  rng = gsl_rng_alloc(gsl_rng_mt19937);
  assert(rng);
  gsl_rng_set(rng, VBRandom());
  uint64 ind = lround(gsl_ran_flat(rng, 0, maskcount - 1));
  gsl_rng_free(rng);
  maskcount = 0;
  for (int64 i = 0; i < cb.dimx * cb.dimy * cb.dimz; i++) {
    if (cb.testValue(i)) {
      if (maskcount == ind) {
        myvoxel = cb.getvoxel(i);
        myvoxel.setCool();
        break;
      }
      maskcount++;
    }
  }
  return 0;
}

vbreturn op_randomvoxelnear(Cube &, tokenlist &args) {
  double x = strtod(args[1]);
  double y = strtod(args[2]);
  double z = strtod(args[3]);
  double xsig = strtod(args[4]);
  double ysig = strtod(args[5]);
  double zsig = strtod(args[6]);
  rng = gsl_rng_alloc(gsl_rng_mt19937);
  assert(rng);
  gsl_rng_set(rng, VBRandom());

  x += gsl_ran_gaussian(rng, xsig);
  y += gsl_ran_gaussian(rng, ysig);
  z += gsl_ran_gaussian(rng, zsig);

  gsl_rng_free(rng);
  myvoxel.init(fabs(x), fabs(y), fabs(z));
  myvoxel.setCool();
  return 0;
}

vbreturn op_drawsphere(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (!(myvoxel.cool())) {
    cout << format("[E] vbim: no voxel set for sphere drawing\n");
    exit(101);
  }
  int radius = strtol(args[1]);
  int x1 = max(0, myvoxel.x - radius);
  int x2 = min(cube.dimx - 1, myvoxel.x + radius);
  int y1 = max(0, myvoxel.y - radius);
  int y2 = min(cube.dimy - 1, myvoxel.y + radius);
  int z1 = max(0, myvoxel.z - radius);
  int z2 = min(cube.dimz - 1, myvoxel.z + radius);
  for (int i = x1; i <= x2; i++) {
    for (int j = y1; j <= y2; j++) {
      for (int k = z1; k <= z2; k++) {
        double dist = sqrt(((i - myvoxel.x) * (i - myvoxel.x)) +
                           ((j - myvoxel.y) * (j - myvoxel.y)) +
                           ((k - myvoxel.z) * (k - myvoxel.z)));
        if (lround(dist) <= radius) cube.setValue(i, j, k, 1);
      }
    }
  }
  return 0;
}

vbreturn op_quantize(Cube &cube, tokenlist &args) {
  reallyload(cube);
  cube.quantize(strtod(args[1]));
  return 0;
}

vbreturn op_invert(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.invert();
  return 0;
}

vbreturn op_abs(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.abs();
  return 0;
}

vbreturn op_signflip(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube *= -1;
  return 0;
}

vbreturn op_flipx(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.flipx();
  return 0;
}

vbreturn op_flipy(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.flipy();
  return 0;
}

vbreturn op_flipz(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.flipz();
  return 0;
}

vbreturn op_zeroleft(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.rightify();
  return 0;
}

vbreturn op_zeroright(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.leftify();
  return 0;
}

vbreturn op_zero(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (args.size() == 0) {
    cube.zero();
    return 0;
  }
  int dimx = cube.dimx;
  int dimy = cube.dimy;
  int dimz = cube.dimz;
  char dim = vb_tolower(args[1])[0];
  string mode = vb_tolower(args[2]);
  int n = 0;
  if (args.size() > 3) n = strtol(args[3]);
  if (dim != 'x' && dim != 'y' && dim != 'z') {
    cout << "[E] vbim: invalid argument for -zero\n";
    return 1;
  }
  // e.g., x first 20; z last 10; y firsthalf; etc.
  if (dim == 'x' && mode == "first" && n > 0) {
    cube.zero(0, n - 1, 0, 0, 0, 0);
    return 0;
  }
  if (dim == 'y' && mode == "first" && n > 0) {
    cube.zero(0, 0, 0, n - 1, 0, 0);
    return 0;
  }
  if (dim == 'z' && mode == "first" && n > 0) {
    cube.zero(0, 0, 0, 0, 0, n - 1);
    return 0;
  }
  if (dim == 'x' && mode == "last" && n > 0) {
    cube.zero(dimx - n, dimx - 1, 0, 0, 0, 0);
    return 0;
  }
  if (dim == 'y' && mode == "last" && n > 0) {
    cube.zero(0, 0, dimy - n, dimy - 1, 0, 0);
    return 0;
  }
  if (dim == 'z' && mode == "last" && n > 0) {
    cube.zero(0, 0, 0, 0, dimz - n, dimz - 1);
    return 0;
  }
  if (dim == 'x' && mode == "firsthalf") {
    cube.zero(0, ((dimx + 1) / 2) - 1, 0, 0, 0, 0);
    return 0;
  }
  if (dim == 'x' && mode == "lasthalf") {
    cube.zero(((dimx + 1) / 2), dimx - 1, 0, 0, 0, 0);
    return 0;
  }
  if (dim == 'y' && mode == "firsthalf") {
    cube.zero(0, 0, 0, ((dimy + 1) / 2) - 1, 0, 0);
    return 0;
  }
  if (dim == 'y' && mode == "lasthalf") {
    cube.zero(0, 0, ((dimy + 1) / 2), dimy - 1, 0, 0);
    return 0;
  }
  if (dim == 'z' && mode == "firsthalf") {
    cube.zero(0, 0, 0, 0, 0, ((dimz + 1) / 2) - 1);
    return 0;
  }
  if (dim == 'z' && mode == "lasthalf") {
    cube.zero(0, 0, 0, 0, ((dimz + 1) / 2), dimz - 1);
    return 0;
  }
  cout << "[E] vbim: invalid arguments for -zero\n";
  exit(1);
}

vbreturn op_bigendian(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.filebyteorder = ENDIAN_BIG;
  return 0;
}

vbreturn op_littleendian(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.filebyteorder = ENDIAN_LITTLE;
  return 0;
}

vbreturn op_byteswap(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.byteswap();
  return 0;
}

vbreturn op_combine(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (cube_combine(cube, args)) {
    printf("[E] vbim: error downsampling\n");
    exit(212);
  }
  return 0;
}

class sdata {
 public:
  int32 count;
  double sum;
  int32 nzcount;
};

vbreturn op_maskselect(Cube &cube, tokenlist &args) {
  reallyload(cube);
  set<int32> nums = numberset(args[1]);
  if (nums.empty()) {
    cout << format("[E] vbim: bad set of masks to select: %s\n") % args[1];
    exit(144);
  }
  for (int i = 0; i < cube.dimx * cube.dimy * cube.dimz; i++) {
    int32 val = cube.getValue<int32>(i);
    if (!(nums.count(val))) cube.setValue(i, 0);
  }
  return 0;
}

vbreturn op_maskcombine(Cube &cube, tokenlist &args) {
  string mode = vb_tolower(args[2]);
  reallyload(cube);
  Cube mask;
  mask.ReadFile(args[1]);
  if (!mask) {
    cout << format("[E] vbim: couldn't read mask file %s\n") % args[1];
    exit(212);
  }
  if (mask.dimx != cube.dimx || mask.dimy != cube.dimy ||
      mask.dimz != cube.dimz) {
    cout << format(
                "[E] vbim: incompatible dimensions between mask %s and image "
                "data\n") %
                args[1];
    exit(213);
  }
  map<int32, sdata> smap;
  map<int32, sdata>::iterator sit;
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        int32 mval = mask.getValue<int32>(i, j, k);
        if (!mval) continue;
        if (!smap.count(mval)) {
          smap[mval].count = 0;
          smap[mval].sum = 0.0;
        }
        smap[mval].count++;
        double val = cube.getValue<double>(i, j, k);
        smap[mval].sum += val;
        if (fabs(val) >= DBL_MIN) smap[mval].nzcount++;
      }
    }
  }
  if (mode == "count") {
    for (sit = smap.begin(); sit != smap.end(); sit++)
      sit->second.sum = sit->second.nzcount;
  } else if (mode == "average") {
    for (sit = smap.begin(); sit != smap.end(); sit++)
      sit->second.sum /= sit->second.count;
  } else if (mode == "pct") {
    for (sit = smap.begin(); sit != smap.end(); sit++)
      sit->second.sum = (double)sit->second.nzcount / sit->second.count;
  }
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        int32 mval = mask.getValue<int32>(i, j, k);
        if (!mval)
          cube.setValue<double>(i, j, k, 0.0);
        else
          cube.setValue<double>(i, j, k, smap[mval].sum);
      }
    }
  }
  return 0;
}

vbreturn op_convert(Cube &cube, tokenlist &args) {
  reallyload(cube);
  string newtype = vb_tolower(args[1]);
  int err = 1;
  if (newtype == "byte") err = cube.convert_type(vb_byte, VBSETALT);
  if (newtype == "int16") err = cube.convert_type(vb_short, VBSETALT);
  if (newtype == "int32") err = cube.convert_type(vb_long, VBSETALT);
  if (newtype == "float")
    err = cube.convert_type(vb_float, VBSETALT | VBNOSCALE);
  if (newtype == "double")
    err = cube.convert_type(vb_double, VBSETALT | VBNOSCALE);

  if (err) {
    printf("[E] vbim: error converting datatype\n");
    exit(212);
  }
  return 0;
}

vbreturn op_removesmallregions(Cube &cube, tokenlist &) {
  reallyload(cube);
  return 0;
}

vbreturn op_add(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (vb_fileexists(args[1])) {
    Cube tmp;
    if (tmp.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read file %s\n", args(1));
      exit(202);
    }
    cube += tmp;
  } else
    cube += strtod(args[1]);
  return 0;
}

vbreturn op_sub(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (vb_fileexists(args[1])) {
    Cube tmp;
    if (tmp.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read file %s\n", args(1));
      exit(202);
    }
    cube -= tmp;
  } else
    cube -= strtod(args[1]);
  return 0;
}

vbreturn op_mult(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (vb_fileexists(args[1])) {
    Cube tmp;
    if (tmp.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read file %s\n", args(1));
      exit(202);
    }
    cube *= tmp;
  } else
    cube *= strtod(args[1]);
  return 0;
}

vbreturn op_div(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (vb_fileexists(args[1])) {
    Cube tmp;
    if (tmp.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read file %s\n", args(1));
      exit(202);
    }
    cube /= tmp;
  } else
    cube /= strtod(args[1]);
  return 0;
}

vbreturn op_nminus(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (vb_fileexists(args[1])) {
    Cube tmp;
    if (tmp.ReadFile(args[1])) {
      printf("[E] vbim: couldn't read file %s\n", args(1));
      exit(202);
    }
    cube = tmp - cube;
  } else {
    cube -= strtod(args[1]);
    cube *= -1.0;
  }
  return 0;
}

vbreturn op_random01(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube_random01(cube);
  return 0;
}

vbreturn op_writevec(Cube &cube, tokenlist &args) {
  reallyload(cube);
  int n = cube.dimx * cube.dimy * cube.dimz;
  VB_Vector vv(n), somevalues;
  int index = 0;
  for (int i = 0; i < n; i++) {
    double val = cube.getValue<double>(i);
    if (fabs(val) < DBL_MIN) continue;
    vv.setElement(index++, val);
  }
  somevalues.resize(index);
  for (int i = 0; i < index; i++) somevalues.setElement(i, vv[i]);
  qsort(somevalues.theVector->data, index, sizeof(double), morethan);
  if (somevalues.WriteFile(args[1]))
    printf("[E] vbim: error writing file %s\n", args(1));
  else
    printf("[I] vbim: wrote file %s\n", args(1));
  return 0;
}

vbreturn op_writeprefixed(Cube &cube, tokenlist &args) {
  reallyload(cube);
  string xd = xdirname(cube.GetFileName());
  string xf = xfilename(cube.GetFileName());
  string fname = xd + "/" + args[1] + xf;
  if (xd == ".") fname = args[1] + xf;
  if (cube.WriteFile(fname))
    printf("[E] vbim: error writing file %s\n", fname.c_str());
  else
    printf("[I] vbim: wrote file %s\n", fname.c_str());
  return 0;
}

vbreturn op_regioninfo(Cube &cube, tokenlist &args) {
  reallyload(cube);
  cube_printregioninfo(cube, args);
  return 0;
}

vbreturn op_remap(Cube &cube, tokenlist &args) {
  reallyload(cube);
  ifstream fs;
  fs.open(args(1), ios::in);
  if (!fs) {
    printf("[E] vbim: couldn't read map file %s\n", args(1));
    return 1;
  }
  const short bufsz = 1024;
  char buf[bufsz];
  map<int, float> mymap;
  int fromval;
  float toval;
  tokenlist mapargs;
  while (fs.good()) {
    fs.getline(buf, bufsz);
    if (fs.gcount() == 0) break;
    mapargs.ParseLine(buf);
    if (mapargs.size() != 2) continue;
    fromval = strtol(mapargs[0]);
    toval = strtod(mapargs[1]);
    if (mymap.count(fromval)) {
      printf("[E] vbim: duplicate value %d found in map %s\n", fromval,
             args(1));
      fs.close();
      return 1;
    }
    mymap[fromval] = toval;
  }
  fs.close();

  // now build the new cube
  Cube newcube;
  newcube.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_float);
  int mappedcnt = 0, unmappedcnt = 0;
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        int key = cube.getValue<int32>(i, j, k);
        if (mymap.count(key)) {
          newcube.SetValue(i, j, k, mymap[key]);
          mappedcnt++;
        } else {
          newcube.SetValue(i, j, k, key);
          unmappedcnt++;
        }
      }
    }
  }
  cube = newcube;
  printf("[I] vbim: applied map file %s (%d voxels mapped, %d unmapped)\n",
         args(1), mappedcnt, unmappedcnt);
  return 0;
}

vbreturn op_info(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.calcminmax();
  printf("[I] cube %s: min=%f max=%f infs/nans=%d\n",
         cube.GetFileName().c_str(), cube.get_minimum(), cube.get_maximum(),
         (int)cube.get_nonfinites());
  return 0;
}

vbreturn op_mask(Cube &cube, tokenlist &args) {
  bool f_maskout = 0;
  if (args[0] == "maskout") f_maskout = 1;
  Cube mask;
  if (mask.ReadFile(args[1])) {
    cout << format("[E] vbim: couldn't open mask file %s\n") % args[1];
    exit(101);
  }
  reallyload(cube);
  if (!cube.dimsequal(mask)) {
    cout << format("[E] vbim: mask dimensions don't match volume dimensions\n");
    exit(102);
  }
  for (int i = 0; i < mask.dimx; i++) {
    for (int j = 0; j < mask.dimy; j++) {
      for (int k = 0; k < mask.dimz; k++) {
        if (f_maskout) {
          if (mask.testValue(i, j, k)) cube.SetValue(i, j, k, 0.0);
        } else {
          if (!mask.testValue(i, j, k)) cube.SetValue(i, j, k, 0.0);
        }
      }
    }
  }
  return 0;
}

vbreturn op_tr(Cube &cube, tokenlist &args) {
  pair<bool, double> ret = strtodx(args[1]);
  if (ret.first) {
    cout << format("[E] vbim: invalid TR %s\n") % args[1];
    exit(111);
  }
  cube.voxsize[3] = ret.second;
  return 0;
}

vbreturn op_vs(Cube &cube, tokenlist &args) {
  pair<bool, double> ret1 = strtodx(args[1]);
  pair<bool, double> ret2 = strtodx(args[2]);
  pair<bool, double> ret3 = strtodx(args[3]);
  if (ret1.first) {
    cout << format("[E] vbim: invalid voxel size %s\n") % args[1];
    exit(111);
  }
  if (ret2.first) {
    cout << format("[E] vbim: invalid voxel size %s\n") % args[2];
    exit(111);
  }
  if (ret3.first) {
    cout << format("[E] vbim: invalid voxel size %s\n") % args[3];
    exit(111);
  }
  cube.voxsize[0] = ret1.second;
  cube.voxsize[1] = ret2.second;
  cube.voxsize[2] = ret3.second;
  return 0;
}

vbreturn op_oo(Cube &cube, tokenlist &args) {
  pair<bool, int32> ret1 = strtolx(args[1]);
  pair<bool, int32> ret2 = strtolx(args[2]);
  pair<bool, int32> ret3 = strtolx(args[3]);
  if (ret1.first) {
    cout << format("[E] vbim: invalid origin %s\n") % args[1];
    exit(111);
  }
  if (ret2.first) {
    cout << format("[E] vbim: invalid origin %s\n") % args[2];
    exit(111);
  }
  if (ret3.first) {
    cout << format("[E] vbim: invalid origin %s\n") % args[3];
    exit(111);
  }
  cube.origin[0] = ret1.second;
  cube.origin[1] = ret2.second;
  cube.origin[2] = ret3.second;
  return 0;
}

vbreturn op_addnoise(Cube &cube, tokenlist &args) {
  double mu = strtod(args[1]);
  double sigma = strtod(args[2]);
  double val;
  reallyload(cube);
  for (int i = 0; i < cube.dimx; i++) {
    for (int j = 0; j < cube.dimy; j++) {
      for (int k = 0; k < cube.dimz; k++) {
        val = cube.getValue<double>(i, j, k);
        val += mu + gsl_ran_gaussian(rng, sigma);
        cube.SetValue(i, j, k, val);
      }
    }
  }
  return 0;
}

// procfns for combining operations

vbreturn op_sum(Cube &cube, tokenlist &) {
  reallyload(cube);
  mycube += cube;
  return 0;
}

vbreturn op_product(Cube &cube, tokenlist &) {
  reallyload(cube);
  mycube *= cube;
  return 0;
}

vbreturn op_multi(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.quantize(1.0);
  cube.invert();
  mycube *= cube;
  cube.invert();
  cube *= multi_index;
  mycube += cube;
  multi_index++;
  return 0;
}

vbreturn op_union(Cube &cube, tokenlist &args) {
  // with one argument, we just mask each cube
  if (args.size() > 1) {
    Cube mask;
    if (mask.ReadFile(args[1])) {
      cout << format("[E] vbim: couldn't load union mask %s\n") % args[1]
           << endl;
      exit(141);
    }
    reallyload(cube);
    cube.unionmask(mask);
    return 0;
  }
  // with no arguments, we do the combining operation to build mycube
  reallyload(cube);
  // cube.quantize(1.0);
  mycube.unionmask(cube);
  return 0;
}

vbreturn op_intersect(Cube &cube, tokenlist &args) {
  // with one argument, we just intersect each cube
  if (args.size() > 1) {
    Cube mask;
    if (mask.ReadFile(args[1])) {
      cout << format("[E] vbim: couldn't load intersect mask %s\n") % args[1]
           << endl;
      exit(141);
    }
    reallyload(cube);
    cube.intersect(mask);
    return 0;
  }
  // with no arguments, we do the combining operation to build mycube
  reallyload(cube);
  mycube.intersect(cube);
  return 0;
}

vbreturn op_count(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.quantize(1.0);
  mycube += cube;
  return 0;
}

vbreturn op_write(Cube &cube, tokenlist &args) {
  reallyload(cube);
  string fname = cube.filename;
  if (args.size() > 1) fname = args[1];
  if (args.size() > 2) {
    replace_string(fname, "IND", (format("%05d") % cube.id1).str());
    replace_string(fname, "SUB", (format("%05d") % cube.id2).str());
    replace_string(fname, "TAG", (format("%s") % cube.id3).str());
  }
  if (cube.WriteFile(fname))
    cout << format("[E] vbim: error writing file %s\n") % fname;
  else
    cout << format("[I] vbim: wrote file %s\n") % fname;
  return 0;
}

// finishfns for combining operations

vbreturn op_separatemasks(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() != 1) return 101;
  map<uint32, VBRegion> rmap;
  int32 maxval = 0;

  CUBI cc = cubelist.begin();
  reallyload(*cc);
  for (int i = 0; i < cc->dimx; i++) {
    for (int j = 0; j < cc->dimy; j++) {
      for (int k = 0; k < cc->dimz; k++) {
        int32 val = cc->getValue<int32>(i, j, k);
        if (!val) continue;
        VBRegion &rtmp = rmap[val];
        rtmp.dimx = cc->dimx;
        rtmp.dimy = cc->dimy;
        rtmp.dimz = cc->dimz;
        rtmp.add(i, j, k, 1);
        if (val > maxval) maxval = val;
      }
    }
  }

  // now set up the new cubelist
  list<Cube> newcubelist;
  pair<uint32, VBRegion> r;
  string fmtstring =
      (format("%%0%dd") % lround(ceil(log(maxval) / log(10.0)))).str();
  vbforeach(r, rmap) {
    r.second.name = (format(fmtstring) % r.first).str();
    if (cc->maskspecs.count(r.first))
      r.second.name = cc->maskspecs[r.first].name;
    Cube cb(r.second);
    if (r.second.name.size()) {
      cb.filename = r.second.name + ".nii.gz";
      cb.id2 = r.first;
      cb.id3 = r.second.name;
    }
    newcubelist.push_back(cb);
  }
  cubelist = newcubelist;
  return 0;
}

vbreturn op_oddeven(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  CUBI cceven, ccodd;
  cceven = cubelist.begin();
  ccodd = cceven;
  ccodd++;
  list<Cube> newcubelist;
  while (cceven != cubelist.end() && ccodd != cubelist.end()) {
    reallyload(*cceven);
    reallyload(*ccodd);
    *ccodd -= *cceven;
    newcubelist.push_back(*ccodd);
    cceven->invalidate();
    ccodd->invalidate();
    cceven++;
    ccodd++;
    cceven++;
    ccodd++;
  }
  cubelist.clear();
  cubelist = newcubelist;
  return 0;
}

vbreturn op_uniquemask(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  Tes ts;
  Cube *cb = &(cubelist.front());
  ts.SetVolume(cb->dimx, cb->dimy, cb->dimz, cubelist.size(), cb->datatype);
  int ind = 0;
  for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
    reallyload(*cc);
    ts.SetCube(ind++, *cc);
    cc->invalidate();
  }
  Cube mask;
  mask.SetVolume(ts.dimx, ts.dimy, ts.dimz, vb_byte);
  bitmask bm;
  bm.resize(ts.dimt);
  set<bitmask> bset;
  for (int i = 0; i < ts.dimx; i++) {
    for (int j = 0; j < ts.dimy; j++) {
      for (int k = 0; k < ts.dimz; k++) {
        for (int m = 0; m < ts.dimt; m++) {
          if (fabs(ts.GetValue(i, j, k, m)) >= DBL_MIN)
            bm.set(m);
          else
            bm.unset(m);
        }
        if (bset.count(bm) == 0) {
          mask.setValue<char>(i, j, k, 1);
          bset.insert(bm);
        }
      }
    }
  }
  ts.invalidate();
  cubelist.clear();
  cubelist.push_back(mask);
  return 0;
}

vbreturn op_overlapmask(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() != 2 && cubelist.size() != 3) {
    printf("[E] vbim: overlapmask can only work with 2 or 3 input masks\n");
    return 101;
  }
  CUBI cc1, cc2, cc3;
  cc1 = cubelist.begin();
  cc2 = cc1;
  cc2++;
  cc3 = cc2;
  cc3++;
  reallyload(*cc1);
  reallyload(*cc2);
  cc1->filename = xsetextension(xfilename(cc1->filename), "");
  cc2->filename = xsetextension(xfilename(cc2->filename), "");
  if (cubelist.size() == 3) {
    reallyload(*cc3);
    cc3->filename = xsetextension(xfilename(cc3->filename), "");
  }

  // check dims
  int dimx = cc1->dimx;
  int dimy = cc1->dimy;
  int dimz = cc1->dimz;
  for (CUBI cc = ++(cubelist.begin()); cc != cubelist.end(); cc++) {
    //  for (size_t j=1; j<cubelist.size(); j++) {
    if (cc->dimx != dimx || cc->dimy != dimy || cc->dimz != dimz) {
      printf("[E] vbim: overlapmask requires masks of matching dimensions\n");
      return 101;
    }
  }

  Cube outmask;
  outmask.SetVolume(dimx, dimy, dimz, vb_byte);
  uint8 val = 0;
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    val = 0;
    if (cc1->testValue(i)) val += 1;
    if (cc2->testValue(i)) val += 2;
    if (cubelist.size() == 3)
      if (cc3->testValue(i)) val += 4;
    outmask.setValue<char>(i, val);
  }
  tcolor tc;
  outmask.maskspecs[1] = VBMaskSpec(cc1->filename, tc.r, tc.g, tc.b);
  tc.next();
  outmask.maskspecs[2] = VBMaskSpec(cc2->filename, tc.r, tc.g, tc.b);
  tc.next();
  outmask.maskspecs[3] =
      VBMaskSpec(cc1->filename + "/" + cc2->filename, tc.r, tc.g, tc.b);
  tc.next();

  if (cubelist.size() == 3) {
    outmask.maskspecs[4] = VBMaskSpec(cc3->filename, tc.r, tc.g, tc.b);
    tc.next();
    outmask.maskspecs[5] =
        VBMaskSpec(cc1->filename + "/" + cc3->filename, tc.r, tc.g, tc.b);
    tc.next();
    outmask.maskspecs[6] =
        VBMaskSpec(cc2->filename + "/" + cc3->filename, tc.r, tc.g, tc.b);
    tc.next();
    outmask.maskspecs[7] =
        VBMaskSpec(cc1->filename + "/" + cc2->filename + "/" + cc3->filename,
                   tc.r, tc.g, tc.b);
  }
  cubelist.clear();
  cubelist.push_back(outmask);
  return 0;
}

vbreturn op_null(list<Cube> &cubelist, tokenlist &) {
  cubelist.clear();
  cubelist.push_back(mycube);
  return 0;
}

vbreturn op_scale(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() == 0) return 101;
  mycube /= cubelist.size();
  cubelist.clear();
  cubelist.push_back(mycube);
  return 0;
}

vbreturn op_select(list<Cube> &cubelist, tokenlist &args) {
  selectcubes(cubelist, strtol(args[1]));
  return 0;
}

vbreturn op_include(list<Cube> &cubelist, tokenlist &args) {
  vector<int> nlist = numberlist(args[1]);
  if (nlist.empty()) {
    cout << "[E] vbim: invalid range argument for include operator\n";
    exit(111);
  }
  bool mymap[cubelist.size()];
  memset(mymap, 0, cubelist.size() * sizeof(bool));
  for (size_t i = 0; i < nlist.size(); i++) {
    if (nlist[i] < 0 || nlist[i] >= (int)cubelist.size()) {
      cout << "[E] vbim: invalid range for include operator\n";
      exit(111);
    }
    mymap[nlist[i]] = 1;
  }
  list<Cube>::iterator cc = cubelist.begin();
  int osize = cubelist.size();
  for (int i = 0; i < osize; i++) {
    if (!mymap[i])
      cc = cubelist.erase(cc);
    else
      cc++;
  }
  return 0;
}

vbreturn op_exclude(list<Cube> &cubelist, tokenlist &args) {
  vector<int> nlist = numberlist(args[1]);
  if (nlist.empty()) {
    cout << "[E] vbim: invalid range argument for exclude operator\n";
    exit(111);
  }
  bool mymap[cubelist.size()];
  memset(mymap, 0, cubelist.size() * sizeof(bool));
  for (size_t i = 0; i < nlist.size(); i++) {
    if (nlist[i] < 0 || nlist[i] >= (int)cubelist.size()) {
      cout << "[E] vbim: invalid range for include operator\n";
      exit(111);
    }
    mymap[nlist[i]] = 1;
  }
  list<Cube>::iterator cc = cubelist.begin();
  int osize = cubelist.size();
  for (int i = 0; i < osize; i++) {
    if (mymap[i])
      cc = cubelist.erase(cc);
    else
      cc++;
  }
  return 0;
}

vbreturn op_write4d(list<Cube> &cubelist, tokenlist &args) {
  if (cubelist.size() == 0) return 101;
  Tes ts;
  Cube *cb = &(cubelist.front());
  ts.SetVolume(cb->dimx, cb->dimy, cb->dimz, cubelist.size(), cb->datatype);
  ts.CopyHeader(cubelist.front());
  int ind = 0;
  for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
    reallyload(*cc);
    ts.SetCube(ind++, *cc);
  }
  int err;
  if ((err = ts.WriteFile(args[1])))
    printf("[E] vbim: couldn't write file %s (%d)\n", args[1].c_str(), err);
  else
    printf("[I] vbim: wrote file %s\n", args[1].c_str());
  return 0;
}

vbreturn op_writecompacted(list<Cube> &cubelist, tokenlist &args) {
  if (cubelist.size() == 0) return 101;
  Tes ts;
  Cube *cb = &(cubelist.front());
  ts.SetVolume(cb->dimx, cb->dimy, cb->dimz, cubelist.size(), cb->datatype);
  ts.CopyHeader(cubelist.front());
  int ind = 0;
  for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
    reallyload(*cc);
    ts.SetCube(ind++, *cc);
    // cc->invalidate();
  }
  ts.compact();
  int err;
  if ((err = ts.WriteFile(args[1])))
    printf("[E] vbim: couldn't write file %s (%d)\n", args[1].c_str(), err);
  else
    printf("[I] vbim: wrote file %s\n", args[1].c_str());
  return 0;
}

vbreturn op_writepca(list<Cube> &cubelist, tokenlist &args) {
  if (cubelist.size() == 0) return 101;
  Cube mask;
  if (mask.ReadFile(args[2])) {
    cout << format("[E] vbim: couldn't read mask file %s\n") % mask;
    return 102;
  }
  if (mask.dimx != cubelist.front().dimx ||
      mask.dimy != cubelist.front().dimy ||
      mask.dimz != cubelist.front().dimz) {
    cout << format("[E] vbim: mismatched mask dimensions\n");
    return 103;
  }
  int n = mask.count();
  VBMatrix data(cubelist.size(), n);
  int ind = 0;
  for (int i = 0; i < mask.dimx * mask.dimy * mask.dimz; i++) {
    if (!(mask.testValue(i))) continue;
    int j = 0;
    for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++)
      data.set(j++, ind, cc->getValue(i));
  }
  // DO THE PCA
  VB_Vector l(1);
  VBMatrix comp;
  VBMatrix e;
  cout << format("m=%d n=%d\n") % data.m % data.n;
  if (pca(data, l, comp, e)) {
    cout << format("[E] vbim: error doing PCA\n");
    return 104;
  }
  // COPY TO TES
  int requested = strtol(args[1]);
  Tes ts(1, 1, comp.n, comp.m, vb_float);
  ts.CopyHeader(cubelist.front());
  for (size_t i = 0; i < comp.m; i++) {
    for (int j = 0; j < requested; j++) {
      ts.SetValue(0, 0, j, i, comp(i, j));
    }
  }
  int err;
  if ((err = ts.WriteFile(args[1])))
    printf("[E] vbim: couldn't write file %s (%d)\n", args[1].c_str(), err);
  else
    printf("[I] vbim: wrote file %s\n", args[1].c_str());
  return 0;
}

vbreturn op_o(list<Cube> &cubelist, tokenlist &args) {
  if (f_read4d) return op_write4d(cubelist, args);
  // we only read 3d files, so we should write out each file
  int ind = 0;
  vbforeach(Cube & c, cubelist) {
    string fname = c.filename;
    if (args.size() > 1) {
      fname = args[1];
      string num = (format("%05d") % ind).str();
      replace_string(fname, "XXX", num);
    }
    if (c.WriteFile(fname))
      cout << format("[E] vbim: error writing file %s\n") % fname;
    else
      cout << format("[I] vbim: wrote file %s\n") % fname;
    ind++;
  }
  return 0;
}

vbreturn op_newvol(list<Cube> &cubelist, tokenlist &args) {
  cubelist.clear();
  int32 dimx = strtol(args[1]);
  int32 dimy = strtol(args[2]);
  int32 dimz = strtol(args[3]);
  int32 dimt = strtol(args[4]);
  VB_datatype dt;
  int ds;
  parsedatatype(args[5], dt, ds);

  if (dimx < 0 || dimy < 0 || dimz < 0 || dimt < 0 || dimx > MAX_DIM ||
      dimy > MAX_DIM || dimz > MAX_DIM || dimy > MAX_DIM) {
    cout << "[E] vbim: invalid dimensions for new volume\n";
    exit(199);
  }
  if (dimt < 1) dimt = 1;
  for (int i = 0; i < dimt; i++) {
    cubelist.push_back(new Cube);
    cubelist.back().SetVolume(dimx, dimy, dimz, dt);
    if (!cubelist.back()) {
      cout << "[E] vbim: couldn't allocate space for volume\n";
      exit(199);
    }
  }
  return 0;
}

void build_oplist() {
  // non-combining ones are easy
  oplist["smoothvox"] = imageop("smoothvox", 3, 3, op_smoothvox);
  oplist["smoothvox2"] = imageop("smoothvox2", 3, 3, op_smoothvox2);
  oplist["smoothmm"] = imageop("smoothmm", 3, 3, op_smoothmm);
  oplist["smoothmm2"] = imageop("smoothmm2", 3, 3, op_smoothmm2);
  oplist["shift"] = imageop("shift", 3, 3, op_shift);
  oplist["thresh"] = imageop("thresh", 1, 1, op_thresh);
  oplist["rotate"] = imageop("rotate", 3, 3, op_rotate);
  oplist["nonans"] = imageop("nonans", 0, 0, op_nonans);
  oplist["regionat"] = imageop("regionat", 3, 3, op_regionat);
  oplist["threshabs"] = imageop("threshabs", 1, 1, op_threshabs);
  oplist["cutoff"] = imageop("cutoff", 1, 1, op_cutoff);
  oplist["invert"] = imageop("invert", 0, 0, op_invert);
  oplist["abs"] = imageop("abs", 0, 0, op_abs);
  oplist["signflip"] = imageop("signflip", 0, 0, op_signflip);
  oplist["splitregions"] = imageop("splitregions", 1, 1, op_splitregions);

  oplist["tr"] = imageop("tr", 1, 1, op_tr);
  oplist["vs"] = imageop("vs", 3, 3, op_vs);
  oplist["oo"] = imageop("oo", 3, 3, op_oo);

  oplist["mask"] = imageop("mask", 1, 1, op_mask);
  oplist["maskout"] = imageop("mask", 1, 1, op_mask);

  oplist["flipx"] = imageop("flipx", 0, 0, op_flipx);
  oplist["flipy"] = imageop("flipy", 0, 0, op_flipy);
  oplist["flipz"] = imageop("flipz", 0, 0, op_flipz);
  oplist["zeroleft"] = imageop("zeroleft", 0, 0, op_zeroleft);
  oplist["zeroright"] = imageop("zeroright", 0, 0, op_zeroright);
  oplist["zero"] = imageop("zero", 2, 3, op_zero);
  oplist["bigendian"] = imageop("bigendian", 0, 0, op_bigendian);
  oplist["littleendian"] = imageop("zeroright", 0, 0, op_littleendian);
  oplist["byteswap"] = imageop("zeroright", 0, 0, op_byteswap);
  oplist["subdivide"] = imageop("subdivide", 3, 3, op_subdivide);

  oplist["setspace"] = imageop("", 0, 1, op_setspace);
  oplist["randomvoxel"] = imageop("", 0, 1, op_randomvoxel);
  oplist["randomvoxelnear"] = imageop("", 6, 6, op_randomvoxelnear);
  oplist["drawsphere"] = imageop("", 1, 1, op_drawsphere);
  oplist["orient"] = imageop("", 99, 99, op_orient);
  oplist["quantize"] = imageop("quantize", 1, 1, op_quantize);
  oplist["remap"] = imageop("remap", 1, 1, op_remap);
  oplist["combine"] = imageop("combine", 5, 5, op_combine);
  oplist["maskcombine"] = imageop("maskcombine", 2, 2, op_maskcombine);
  oplist["maskselect"] = imageop("maskselect", 1, 1, op_maskselect);
  oplist["convert"] = imageop("convert", 1, 1, op_convert);
  oplist["removesmallregions"] =
      imageop("removesmallregions", 1, 1, op_removesmallregions);
  oplist["add"] = imageop("add", 1, 1, op_add);
  oplist["sub"] = imageop("sub", 1, 1, op_sub);
  oplist["mult"] = imageop("mult", 1, 1, op_mult);
  oplist["div"] = imageop("div", 1, 1, op_div);
  oplist["nminus"] = imageop("nminus", 1, 1, op_nminus);
  oplist["random01"] = imageop("random01", 0, 0, op_random01);
  oplist["writevec"] = imageop("write", 1, 1, op_writevec);
  oplist["writeprefixed"] = imageop("writeprefixed", 1, 1, op_writeprefixed);
  oplist["regioninfo"] = imageop("regioninfo", 0, 1, op_regioninfo);
  oplist["info"] = imageop("info", 0, 0, op_info);
  oplist["write"] = imageop("write", 0, 2, op_write);

  // combining operators take a little more work
  imageop tmp;

  // by doing it this way, all the cubes are in core before we select.
  // if we create an initfn that stores the inclusion map and a procfn
  // that invalidates the data for the ones we don't want, we can be
  // tighter about memory

  tmp = imageop("o", 1, 1, NULL);
  tmp.finishfn = op_o;
  oplist[tmp.name] = tmp;

  tmp = imageop("oddeven", 0, 0, NULL);
  tmp.finishfn = op_oddeven;
  oplist[tmp.name] = tmp;

  tmp = imageop("separatemasks", 0, 0, NULL);
  tmp.finishfn = op_separatemasks;
  oplist[tmp.name] = tmp;

  tmp = imageop("uniquemask", 0, 0, NULL);
  tmp.finishfn = op_uniquemask;
  oplist[tmp.name] = tmp;

  tmp = imageop("overlapmask", 0, 0, NULL);
  tmp.finishfn = op_overlapmask;
  oplist[tmp.name] = tmp;

  tmp = imageop("select", 1, 1, NULL);
  tmp.finishfn = op_select;
  oplist[tmp.name] = tmp;

  tmp = imageop("include", 1, 1, NULL);
  tmp.finishfn = op_include;
  oplist[tmp.name] = tmp;

  tmp = imageop("exclude", 1, 1, NULL);
  tmp.finishfn = op_exclude;
  oplist[tmp.name] = tmp;

  tmp = imageop("sum", 0, 0, NULL);
  tmp.initfn = op_allzeros;
  tmp.procfn = op_sum;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("addnoise", 2, 3, NULL);
  tmp.initfn = op_initrng;
  tmp.procfn = op_addnoise;
  tmp.finishfn = op_freerng;
  oplist[tmp.name] = tmp;

  tmp = imageop("newvol", 5, 5, NULL);
  tmp.finishfn = op_newvol;
  oplist[tmp.name] = tmp;

  tmp = imageop("average", 0, 0, NULL);
  tmp.initfn = op_allzeros;
  tmp.procfn = op_sum;
  tmp.finishfn = op_scale;
  oplist[tmp.name] = tmp;

  tmp = imageop("multi", 0, 0, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_multi;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("union", 0, 1, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_union;
  tmp.finishfn = op_null;
  tmp.f_ncwithargs = 1;
  oplist[tmp.name] = tmp;

  tmp = imageop("intersect", 0, 1, NULL);
  tmp.initfn = op_allones;
  tmp.procfn = op_intersect;
  tmp.finishfn = op_null;
  tmp.f_ncwithargs = 1;
  oplist[tmp.name] = tmp;

  tmp = imageop("count", 0, 0, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_count;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("product", 0, 0, NULL);
  tmp.initfn = op_allones;
  tmp.procfn = op_product;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("write4d", 1, 1, NULL);
  tmp.finishfn = op_write4d;
  oplist[tmp.name] = tmp;

  tmp = imageop("writecompacted", 1, 1, NULL);
  tmp.finishfn = op_writecompacted;
  oplist[tmp.name] = tmp;

  tmp = imageop("writepca", 3, 3, NULL);
  tmp.finishfn = op_writepca;
  oplist[tmp.name] = tmp;
}

void reallyload(Cube &cube) {
  if (cube.data) return;
  if (cube.ReadData(cube.GetFileName())) {
    printf("[E] vbim: error reading file %s\n", cube.GetFileName().c_str());
    exit(222);
  }
}

void vbim_help(string key) {
  if (key == "examples")
    cout << format(myhelp_examples) % vbversion;
  else
    cout << format(myhelp) % vbversion;
}

void vbim_version() { cout << format("VoxBo vbim (v%s)\n") % vbversion; }
