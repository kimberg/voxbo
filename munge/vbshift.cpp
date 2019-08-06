
// vbshift.cpp
// VoxBo shift correction module (corrects local shifts of whole voxels, with
// wrap) Copyright (c) 2003 by The VoxBo Development Team

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

// #include <stdio.h>
#include <string.h>
#include <sstream>
#include "vbio.h"
#include "vbshift.hlp.h"
#include "vbutil.h"

class shiftspec {
 public:
  int x, y, z;
  double score;
};

class VBShifter {
 private:
  Cube refcube, mycube;
  string refname, imagename, outname;
  string paramfile;
  tokenlist args;
  vector<shiftspec> speclist;
  void Compare(int xoff, int yoff, int zoff, Cube &mycube);
  int x1, x2, y1, y2, z1, z2, ssx, ssy, ssz;
  void ShiftImage(Cube &mycube);
  void ShiftImage(Tes &mytes);
  int LoadRefCube();
  int ShiftFile();

 public:
  int Go(tokenlist &args);
};

void vbshift_help();

int main(int argc, char *argv[]) {
  stringstream tmps;
  tokenlist args;
  tzset();         // make sure all times are timezone corrected
  if (argc < 2) {  // not enough args, display autodocs
    vbshift_help();
    exit(0);
  }

  args.Transfer(argc - 1, argv + 1);
  VBShifter r;
  int err = r.Go(args);
  exit(err);
}

int VBShifter::Go(tokenlist &args) {
  int err = 0;

  if (args.size() < 3) {
    vbshift_help();
    return 200;
  }
  imagename = args[0];
  outname = args[1];

  x1 = -4;
  x2 = 4;
  y1 = -4;
  y2 = 4;
  z1 = -4;
  z2 = 4;
  ssx = ssy = ssz = 0;

  for (size_t i = 2; i < args.size(); i++) {
    if (args[i] == "-x") {
      if (i < args.size() - 1) {
        ssx = strtol(args[i + 1]);
        i++;
      }
    } else if (args[i] == "-y") {
      if (i < args.size() - 1) {
        ssy = strtol(args[i + 1]);
        i++;
      }
    } else if (args[i] == "-z") {
      if (i < args.size() - 1) {
        ssz = strtol(args[i + 1]);
        i++;
      }
    } else if (args[i] == "-r") {
      if (i < args.size() - 1) {
        refname = args[i + 1];
        i++;
      }
    }
  }
  if (refname.size()) err = LoadRefCube();
  if (err) return err;
  return (ShiftFile());
}

void VBShifter::Compare(int xoff, int yoff, int zoff, Cube &mycube) {
  // we're going to calculate a score associated with the given offset
  shiftspec ss;
  int xx, yy, zz;

  // init our shiftspec
  ss.x = xoff;
  ss.y = yoff;
  ss.z = zoff;
  ss.score = 0.0;
  // add up all the voxel differences
  for (int i = 0; i < refcube.dimx; i++) {
    for (int j = 0; j < refcube.dimy; j++) {
      for (int k = 0; k < refcube.dimz; k++) {
        xx = i + xoff;
        yy = j + yoff;
        zz = k + zoff;
        while (xx < 0) xx += refcube.dimx;
        while (yy < 0) yy += refcube.dimy;
        while (zz < 0) zz += refcube.dimz;
        while (xx > refcube.dimx - 1) xx -= refcube.dimx;
        while (zz > refcube.dimz - 1) zz -= refcube.dimy;
        while (yy > refcube.dimy - 1) yy -= refcube.dimz;
        ss.score +=
            fabs(refcube.GetValue(i, j, k) - mycube.GetValue(xx, yy, zz));
      }
    }
  }
  speclist.push_back(ss);
}

int VBShifter::LoadRefCube() {
  stringstream tmps;
  if (!(refcube.ReadFile(refname))) {
    tmps.str("");
    tmps << "vbshift: read reference volume from " << refname;
    printErrorMsg(VB_INFO, tmps.str());
    return 0;
  }
  Tes tmptes;
  if (!(tmptes.ReadFile(refname))) {
    refcube = tmptes[0];
    tmps.str("");
    tmps << "vbshift: read reference from the first volume of " << refname;
    printErrorMsg(VB_INFO, tmps.str());
    return 0;
  }
  tmps.str("");
  tmps << "vbshift: couldn't read reference from " << refname;
  printErrorMsg(VB_INFO, tmps.str());
  return 100;
}

void VBShifter::ShiftImage(Cube &mycube) {
  int i, j, k, tmpx, tmpy, tmpz;
  struct tm *mytm;
  time_t mytime;
  stringstream tmps;

  mytime = time(NULL);
  mytm = localtime(&mytime);
  char timestring[STRINGLEN];
  strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
  mycube.AddHeader((string) "vbshift_date: " + timestring);

  if (refcube.data_valid) {
    tmps.str("");
    tmps << "vbshift: comparing " << imagename << " to reference volume "
         << refname;
    printErrorMsg(VB_INFO, tmps.str());
    // first build a list of comparisons at various offsets
    for (i = x1; i <= x2; i++) {
      for (j = y1; j <= y2; j++) {
        for (k = z1; k <= z2; k++) {
          Compare(i, j, k, mycube);
        }
      }
    }

    int good = 0;
    for (i = 1; i < (int)speclist.size(); i++) {
      if (speclist[i].score < speclist[good].score) good = i;
    }
    ssx = speclist[good].x;
    ssy = speclist[good].y;
    ssz = speclist[good].z;
    mycube.AddHeader((string) "vbshift_ref: " + refname);
  }
  if (ssx == 0 && ssy == 0 && ssz == 0)
    mycube.AddHeader("vbshift: no adjustment necessary");
  else {
    Cube newcube = mycube;
    for (int i = 0; i < mycube.dimx; i++) {
      for (int j = 0; j < mycube.dimy; j++) {
        for (int k = 0; k < mycube.dimz; k++) {
          tmpx = i + ssx;
          tmpy = j + ssy;
          tmpz = k + ssz;
          while (tmpx < 0) tmpx += mycube.dimx;
          while (tmpy < 0) tmpy += mycube.dimy;
          while (tmpz < 0) tmpz += mycube.dimz;
          while (tmpx > mycube.dimx - 1) tmpx -= mycube.dimx;
          while (tmpy > mycube.dimy - 1) tmpy -= mycube.dimy;
          while (tmpz > mycube.dimz - 1) tmpz -= mycube.dimz;
          newcube.SetValue(i, j, k, mycube.GetValue(tmpx, tmpy, tmpz));
        }
      }
    }
    mycube = newcube;
  }
  tmps.str("");
  tmps << "vbshift_offset: " << ssx << " " << ssy << " " << ssz;
  mycube.AddHeader(tmps.str());
  printErrorMsg(VB_INFO, tmps.str());
}

void VBShifter::ShiftImage(Tes &mytes) {
  int i, j, k;
  struct tm *mytm;
  time_t mytime;
  stringstream tmps;

  mytime = time(NULL);
  mytm = localtime(&mytime);
  char timestring[STRINGLEN];
  strftime(timestring, STRINGLEN, "%d%b%Y_%T", mytm);
  mytes.AddHeader((string) "vbshift_date: " + timestring);

  if (refcube.data_valid) {
    tmps.str("");
    tmps << "vbshift: comparing " << imagename << " to reference volume "
         << refname;
    printErrorMsg(VB_INFO, tmps.str());
    Cube mycube = mytes[0];
    // first build a list of comparisons at various offsets
    for (i = x1; i <= x2; i++) {
      for (j = y1; j <= y2; j++) {
        for (k = z1; k <= z2; k++) {
          Compare(i, j, k, mycube);
        }
      }
    }

    int good = 0;
    for (i = 1; i < (int)speclist.size(); i++) {
      if (speclist[i].score < speclist[good].score) good = i;
    }
    ssx = speclist[good].x;
    ssy = speclist[good].y;
    ssz = speclist[good].z;
    mytes.AddHeader((string) "vbshift_ref: " + refname);
  }
  if (ssx == 0 && ssy == 0 && ssz == 0)
    mytes.AddHeader("vbshift: no adjustment necessary");
  else {
    // make a copy of the data pointer array and copy the new pointer
    // array from the old one
    unsigned char **tmpdata =
        new unsigned char *[mytes.dimx * mytes.dimy * mytes.dimz];
    memcpy(tmpdata, mytes.data,
           sizeof(unsigned char *) * mytes.dimx * mytes.dimy * mytes.dimz);
    int oldpos, newpos, tmpx, tmpy, tmpz;
    for (i = 0; i < mytes.dimx; i++) {
      for (j = 0; j < mytes.dimy; j++) {
        for (k = 0; k < mytes.dimz; k++) {
          tmpx = i + ssx;
          tmpy = j + ssy;
          tmpz = k + ssz;
          while (tmpx < 0) tmpx += mytes.dimx;
          while (tmpy < 0) tmpy += mytes.dimy;
          while (tmpz < 0) tmpz += mytes.dimz;
          while (tmpx > mytes.dimx - 1) tmpx -= mytes.dimx;
          while (tmpy > mytes.dimy - 1) tmpy -= mytes.dimy;
          while (tmpz > mytes.dimz - 1) tmpz -= mytes.dimz;
          oldpos = mytes.voxelposition(tmpx, tmpy, tmpz);
          newpos = mytes.voxelposition(i, j, k);
          mytes.data[newpos] = tmpdata[oldpos];
        }
      }
    }
    delete tmpdata;
    mytes.Remask();
  }

  tmps.str("");
  tmps << "vbshift_offset: " << ssx << " " << ssy << " " << ssz;
  mytes.AddHeader(tmps.str());
  printErrorMsg(VB_INFO, tmps.str());
}

int VBShifter::ShiftFile() {
  stringstream tmps;
  Tes mytes;
  Cube mycube;
  int dims = 0;

  if (!(mytes.ReadFile(imagename)))
    dims = 4;
  else if (!(mycube.ReadFile(imagename)))
    dims = 3;
  if (dims == 0) {
    tmps.str("");
    tmps << "vbshift: couldn't read file " << imagename;
    printErrorMsg(VB_ERROR, tmps.str());
    return 140;
  }

  int err;
  if (dims == 4) {
    ShiftImage(mytes);
    mytes.SetFileName(outname);
    err = mytes.WriteFile();
  } else {
    ShiftImage(mycube);
    mycube.SetFileName(outname);
    err = mycube.WriteFile();
  }
  if (!err) {
    tmps.str("");
    tmps << "vbshift: wrote " << outname;
    printErrorMsg(VB_INFO, tmps.str());
    return 0;
  } else {
    tmps.str("");
    tmps << "vbshift: error writing " << outname;
    printErrorMsg(VB_ERROR, tmps.str());
    return 110;
  }
}

void vbshift_help() { cout << boost::format(myhelp) % vbversion; }
