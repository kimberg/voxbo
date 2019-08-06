
// vbimagemunge.cpp
// general-purpose munging util for voxbo
// Copyright (c) 2003-2009 by The VoxBo Development Team

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

#include <boost/format.hpp>
#include <fstream>
#include "imageutils.h"
#include "vbimagemunge.hlp.h"
#include "vbio.h"
#include "vbutil.h"
#include "vbversion.h"

void vbimagemunge_help();
void vbimagemunge_version();

int cube_combine(Cube &cb, tokenlist &args);
void selectcubes(list<Cube> &clist, size_t n);
bool comparedims(list<Cube> cubelist);
void cube_random01(Cube &cb);
void cube_printregioninfo(Cube &cb);
void reallyload(Cube &cube);
void build_oplist();

// ugly globals -- code needs some reorg
Cube mycube;
int multi_index;

class imageop {
 public:
  string name;
  int nargs;
  tokenlist args;
  // an operator is combining if either initfn or finishfn is non-null
  vbreturn (*initfn)(list<Cube> &cubelist, tokenlist &args);
  vbreturn (*procfn)(Cube &cube, tokenlist &args);
  vbreturn (*finishfn)(list<Cube> &cubelist, tokenlist &args);
  // storage for combining operators
  // sole constructor requires name and nargs
  imageop();
  imageop(string xname, int xnargs, vbreturn (*xprocfn)(Cube &, tokenlist &));
  void init(string xname, int xnargs, vbreturn (*xprocfn)(Cube &, tokenlist &));
  // random methods
  bool iscombining() { return (initfn != NULL || finishfn != NULL); }
};

imageop::imageop() { init("", 0, NULL); }

imageop::imageop(string xname, int xnargs,
                 vbreturn (*xprocfn)(Cube &, tokenlist &)) {
  init(xname, xnargs, xprocfn);
}

void imageop::init(string xname, int xnargs,
                   vbreturn (*xprocfn)(Cube &, tokenlist &)) {
  name = xname;
  nargs = xnargs;
  args.clear();
  initfn = NULL;
  procfn = xprocfn;
  finishfn = NULL;
}

// three operator lists that are handled differently
list<imageop> phase1;  // do the cubes one at a time.  if last operator is
                       // combining, feed.
list<imageop> phase2;  // do each operator for all cubes
typedef list<imageop>::iterator OPI;
typedef list<Cube>::iterator CUBI;

map<string, imageop> oplist;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbimagemunge_help();
    exit(0);
  }
  build_oplist();
  tokenlist args;
  list<string> filelist;
  // string outfile;
  args.Transfer(argc - 1, argv + 1);
  bool inops = 0;
  int phase = 1;
  for (size_t i = 0; i < args.size(); i++) {
    if (inops) {
      if (oplist.count(args[i]) == 0) {
        printf("[E] vbimagemunge: unknown operator %s\n", args(i));
        exit(202);
      }
      imageop myop = oplist[args[i]];
      if (i + myop.nargs >= args.size()) {
        printf("[E] vbimagemunge: operator %s requires %d arguments\n",
               myop.name.c_str(), myop.nargs);
        exit(203);
      }
      myop.args.Add(args[i]);  // first arg is the operator name
      for (int j = 0; j < myop.nargs; j++) myop.args.Add(args[++i]);
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
    } else {
      if (args[i] == "--") {
        inops = 1;
      } else if (args[i] == "-h") {
        vbimagemunge_help();
        exit(0);
      } else if (args[i] == "-v") {
        vbimagemunge_version();
        exit(0);
      } else
        filelist.push_back(args[i]);
    }
  }

  list<Cube> cubelist;
  Cube master;

  // pre-load all the files.  we should figure out how not to load all
  // the image data from 4D files, maybe.

  for (list<string>::iterator ff = filelist.begin(); ff != filelist.end();
       ff++) {
    printf("[I] vbimagemunge: reading file %s\n", ff->c_str());
    Cube cb;
    Tes ts;
    if (cb.ReadHeader(*ff) == 0) {
      cubelist.push_back(cb);
      continue;
    }
    if (ts.ReadFile(*ff)) {
      printf("[E] vbimagemunge: couldn't read file %s, continuing anyway\n",
             ff->c_str());
      continue;
    }
    for (int i = 0; i < ts.dimt; i++) {
      Cube tmpc;
      cubelist.push_back(tmpc);
      ts.getCube(i, cubelist.back());
    }
  }
  if (cubelist.size() == 0) {
    printf("[E] vbimagemunge: no valid input cubes found\n");
    exit(11);
  }

  // first do phase 1
  int index = 0;
  if (phase1.size()) {
    if (phase1.back().iscombining() && phase1.back().initfn)
      phase1.back().initfn(cubelist, phase1.back().args);
    for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
      for (OPI oo = phase1.begin(); oo != phase1.end(); oo++) {
        if (oo->procfn) oo->procfn(*cc, oo->args);
      }
      index++;
    }
    if (phase1.back().iscombining() && phase1.back().finishfn)
      phase1.back().finishfn(cubelist, phase1.back().args);
  }
  // if we have a phase2 block, now do that
  for (OPI oo = phase2.begin(); oo != phase2.end(); oo++) {
    if (oo->initfn) oo->initfn(cubelist, oo->args);
    index = 0;
    for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
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
  double value = strtod(args[6]);
  int f_needval = 0;
  if (method == "sum" || method == "average" || method == "sumthresh" ||
      method == "averagethresh")
    f_needval = 1;

  if (xx < 1 || yy < 1 || zz < 1) {
    printf("[E] vbimagemunge: invalid subcube dimensions\n");
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
    // pct, any sumthresh, averagethresh
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
    else if (method == "any") {
      if (count) newvalue = 1;
    } else if (method == "sumthresh") {
      if (total > value) newvalue = 1;
    } else if (method == "averagethresh") {
      if ((total / rsize) > value) newvalue = 1;
    } else
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
  double value = strtod(args[6]);
  int f_needval = 0;
  if (method == "sum" || method == "average" || method == "sumthresh" ||
      method == "averagethresh")
    f_needval = 1;

  if (xx < 1 || yy < 1 || zz < 1) {
    printf("[E] vbimagemunge: invalid subcube dimensions\n");
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
        // pct, any sumthresh, averagethresh
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
        else if (method == "any") {
          if (count) newvalue = 1;
        } else if (method == "sumthresh") {
          if (total > value) newvalue = 1;
        } else if (method == "averagethresh") {
          if ((total / rsize) > value) newvalue = 1;
        } else
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

void cube_printregioninfo(Cube &cb) {
  vector<VBRegion> rlist = findregions(cb, vb_ne, 0.0);
  int totalvoxels = 0;
  for (size_t i = 0; i < rlist.size(); i++) totalvoxels += rlist[i].size();
  printf("[I] vbimagemunge: %d voxels in %d regions found in volume %s\n",
         totalvoxels, (int)(rlist.size()), cb.GetFileName().c_str());
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

    printf("[I] vbimagemunge:   region %d, %d voxels,", (int)i,
           (int)(rlist[i].size()));
    printf(" center of mass: %g,%g,%g (weighted: %g,%g,%g)\n", x1, y1, z1, x2,
           y2, z2);
    // totalvoxels+=rlist[i].size();
  }
  // printf("[I] vbimagemunge:   %d total voxels in volume
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

vbreturn op_smoothmm(Cube &cube, tokenlist &) {
  reallyload(cube);
  printf("smoothmm not implemented\n");
  exit(5);
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
    printf("[E] vbimagemunge: error downsampling\n");
    exit(212);
  }
  return 0;
}

vbreturn op_convert(Cube &cube, tokenlist &args) {
  reallyload(cube);
  Cube tmpc;
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
    printf("[E] vbimagemunge: error converting datatype\n");
    exit(212);
  }
  cube = tmpc;
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
      printf("[E] vbimagemunge: couldn't read file %s\n", args(1));
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
      printf("[E] vbimagemunge: couldn't read file %s\n", args(1));
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
      printf("[E] vbimagemunge: couldn't read file %s\n", args(1));
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
      printf("[E] vbimagemunge: couldn't read file %s\n", args(1));
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
      printf("[E] vbimagemunge: couldn't read file %s\n", args(1));
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

vbreturn op_write(Cube &cube, tokenlist &args) {
  reallyload(cube);
  if (cube.WriteFile(args[1]))
    printf("[E] vbimagemunge: error writing file %s\n", args(1));
  else
    printf("[I] vbimagemunge: wrote file %s\n", args(1));
  return 0;
}

vbreturn op_writeprefixed(Cube &cube, tokenlist &args) {
  reallyload(cube);
  string xd = xdirname(cube.GetFileName());
  string xf = xfilename(cube.GetFileName());
  string fname = xd + "/" + args[1] + xf;
  if (xd == ".") fname = args[1] + xf;
  if (cube.WriteFile(fname))
    printf("[E] vbimagemunge: error writing file %s\n", fname.c_str());
  else
    printf("[I] vbimagemunge: wrote file %s\n", fname.c_str());
  return 0;
}

vbreturn op_regioninfo(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube_printregioninfo(cube);
  return 0;
}

vbreturn op_remap(Cube &cube, tokenlist &args) {
  reallyload(cube);
  ifstream fs;
  fs.open(args(1), ios::in);
  if (!fs) {
    printf("[E] vbimagemunge: couldn't read map file %s\n", args(1));
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
      printf("[E] vbimagemunge: duplicate value %d found in map %s\n", fromval,
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
  printf(
      "[I] vbimagemunge: applied map file %s (%d voxels mapped, %d unmapped)\n",
      args(1), mappedcnt, unmappedcnt);
  return 0;
}

vbreturn op_info(Cube &cube, tokenlist &) {
  reallyload(cube);
  printf("[I] cube %s: min=%f max=%f infs/nans=%d\n",
         cube.GetFileName().c_str(), cube.get_minimum(), cube.get_maximum(),
         (int)cube.get_nonfinites());
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

vbreturn op_union(Cube &cube, tokenlist &) {
  reallyload(cube);
  cube.quantize(1.0);
  mycube.unionmask(cube);
  return 0;
}

vbreturn op_intersect(Cube &cube, tokenlist &) {
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

// finishfns for combining operations

vbreturn op_overlapmask(list<Cube> &cubelist, tokenlist &) {
  if (cubelist.size() != 2 && cubelist.size() != 3) {
    printf(
        "[E] vbimagemunge: overlapmask can only work with 2 or 3 input "
        "masks\n");
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
      printf(
          "[E] vbimagemunge: overlapmask requires masks of matching "
          "dimensions\n");
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
  outmask.AddHeader((string) "vb_maskspec: 1 255 0 0 " + cc1->filename);
  outmask.AddHeader((string) "vb_maskspec: 2 0 0 255 " + cc2->filename);
  outmask.AddHeader((string) "vb_maskspec: 3 170 0 170 \"" + cc1->filename +
                    "/" + cc2->filename + "\"");
  if (cubelist.size() == 3) {
    outmask.AddHeader((string) "vb_maskspec: 4 220 220 0 " + cc3->filename);
    outmask.AddHeader((string) "vb_maskspec: 5 255 140 0 \"" + cc1->filename +
                      "/" + cc3->filename + "\"");
    outmask.AddHeader((string) "vb_maskspec: 6 0 255 0 \"" + cc2->filename +
                      "/" + cc3->filename + "\"");
    outmask.AddHeader((string) "vb_maskspec: 7 140 140 80\"" + cc1->filename +
                      "/" + cc2->filename + "/" + cc3->filename + "\"");
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
  bool mymap[cubelist.size()];
  memset(mymap, 0, cubelist.size() * sizeof(bool));
  for (size_t i = 0; i < nlist.size(); i++)
    if (nlist[i] < (int)cubelist.size()) mymap[nlist[i]] = 1;
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
  bool mymap[cubelist.size()];
  memset(mymap, 0, cubelist.size() * sizeof(bool));
  for (size_t i = 0; i < nlist.size(); i++)
    if (nlist[i] < (int)cubelist.size()) mymap[nlist[i]] = 1;
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
  int ind = 0;
  for (CUBI cc = cubelist.begin(); cc != cubelist.end(); cc++) {
    reallyload(*cc);
    ts.SetCube(ind++, *cc);
    cc->invalidate();
  }
  if (ts.WriteFile(args[1]))
    printf("[E] vbimagemunge: couldn't write file %s\n", args[1].c_str());
  else
    printf("[I] vbimagemunge: wrote file %s\n", args[1].c_str());
  return 0;
}

void build_oplist() {
  // non-combining ones are easy
  oplist["smoothvox"] = imageop("smoothvox", 3, op_smoothvox);
  oplist["smoothmm"] = imageop("smoothmm", 3, op_smoothmm);
  oplist["thresh"] = imageop("thresh", 1, op_thresh);
  oplist["rotate"] = imageop("rotate", 3, op_rotate);
  oplist["regionat"] = imageop("regionat", 3, op_regionat);
  oplist["threshabs"] = imageop("threshabs", 1, op_threshabs);
  oplist["cutoff"] = imageop("cutoff", 1, op_cutoff);
  oplist["invert"] = imageop("invert", 0, op_invert);
  oplist["splitregions"] = imageop("splitregions", 1, op_splitregions);

  oplist["flipx"] = imageop("flipx", 0, op_flipx);
  oplist["flipy"] = imageop("flipy", 0, op_flipy);
  oplist["flipz"] = imageop("flipz", 0, op_flipz);
  oplist["zeroleft"] = imageop("zeroleft", 0, op_zeroleft);
  oplist["zeroright"] = imageop("zeroright", 0, op_zeroright);
  oplist["bigendian"] = imageop("bigendian", 0, op_bigendian);
  oplist["littleendian"] = imageop("zeroright", 0, op_littleendian);
  oplist["byteswap"] = imageop("zeroright", 0, op_byteswap);

  oplist["orient"] = imageop("", 99, op_orient);
  oplist["quantize"] = imageop("quantize", 1, op_quantize);
  oplist["remap"] = imageop("remap", 1, op_remap);
  oplist["combine"] = imageop("combine", 6, op_combine);
  oplist["convert"] = imageop("convert", 1, op_convert);
  oplist["removesmallregions"] =
      imageop("removesmallregions", 1, op_removesmallregions);
  oplist["add"] = imageop("add", 1, op_add);
  oplist["sub"] = imageop("sub", 1, op_sub);
  oplist["mult"] = imageop("mult", 1, op_mult);
  oplist["div"] = imageop("div", 1, op_div);
  oplist["nminus"] = imageop("nminus", 1, op_nminus);
  oplist["random01"] = imageop("random01", 0, op_random01);
  oplist["write"] = imageop("write", 1, op_write);
  oplist["writeprefixed"] = imageop("writeprefixed", 1, op_writeprefixed);
  oplist["regioninfo"] = imageop("regioninfo", 0, op_regioninfo);
  oplist["info"] = imageop("info", 0, op_info);

  // combining operators take a little more work
  imageop tmp;

  // by doing it this way, all the cubes are in core before we select.
  // if we create an initfn that stores the inclusion map and a procfn
  // that invalidates the data for the ones we don't want, we can be
  // tighter about memory

  tmp = imageop("overlapmask", 0, NULL);
  tmp.finishfn = op_overlapmask;
  oplist[tmp.name] = tmp;

  tmp = imageop("select", 1, NULL);
  tmp.finishfn = op_select;
  oplist[tmp.name] = tmp;

  tmp = imageop("include", 1, NULL);
  tmp.finishfn = op_include;
  oplist[tmp.name] = tmp;

  tmp = imageop("exclude", 1, NULL);
  tmp.finishfn = op_exclude;
  oplist[tmp.name] = tmp;

  tmp = imageop("sum", 0, NULL);
  tmp.initfn = op_allzeros;
  tmp.procfn = op_sum;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("average", 0, NULL);
  tmp.initfn = op_allzeros;
  tmp.procfn = op_sum;
  tmp.finishfn = op_scale;
  oplist[tmp.name] = tmp;

  tmp = imageop("multi", 0, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_multi;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("union", 0, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_union;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("intersect", 0, NULL);
  tmp.initfn = op_allones;
  tmp.procfn = op_intersect;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("count", 0, NULL);
  tmp.initfn = op_shortzeros;
  tmp.procfn = op_count;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("product", 0, NULL);
  tmp.initfn = op_allones;
  tmp.procfn = op_product;
  tmp.finishfn = op_null;
  oplist[tmp.name] = tmp;

  tmp = imageop("write4d", 1, NULL);
  tmp.finishfn = op_write4d;
  oplist[tmp.name] = tmp;
}

void reallyload(Cube &cube) {
  if (cube.data) return;
  if (cube.ReadData(cube.GetFileName()))
    printf("[E] vbimagemunge: error reading file %s\n",
           cube.GetFileName().c_str());
}

void vbimagemunge_help() { cout << boost::format(myhelp) % vbversion; }

void vbimagemunge_version() {
  printf("VoxBo vbimagemunge (v%s)\n", vbversion.c_str());
}
