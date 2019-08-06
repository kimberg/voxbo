
// setorigin.cpp
// set the origin of one image to correspond to that of another
// Copyright (c) 1998-2004 by The VoxBo Development Team

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
#include <sstream>
#include "setorigin.hlp.h"
#include "vbio.h"
#include "vbutil.h"

enum { m_strip, m_center };

int setorigin_map(tokenlist &args);
int setorigin_copy(tokenlist &args);
int setorigin_new(tokenlist &args, int mode);
int setorigin_guess(tokenlist &args);
int setorigin_set(tokenlist &args);
void setorigin_help();

int main(int argc, char *argv[]) {
  tzset();         // make sure all times are timezone corrected
  if (argc < 2) {  // not enough args, display autodocs
    setorigin_help();
    exit(0);
  }

  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  int err;
  if (args[0] == "-c")
    err = setorigin_copy(args);
  else if (args[0] == "-m")
    err = setorigin_map(args);
  else if (args[0] == "-g")
    err = setorigin_guess(args);
  else if (args[0] == "-s")
    err = setorigin_new(args, m_strip);
  else if (args[0] == "-x")
    err = setorigin_new(args, m_center);
  else
    err = setorigin_set(args);

  exit(err);
}

int setorigin_set(tokenlist &args) {
  stringstream tmps;

  // FIXME maybe handle x,y,z syntax
  if (args.size() != 4) {
    setorigin_help();
    return (101);
  }

  Cube cb;
  Tes ts;
  int err = 0;

  float o1 = 0.0, o2 = 0.0, o3 = 0.0;
  if (args.size() == 4) {
    o1 = strtod(args[1]);
    o2 = strtod(args[2]);
    o3 = strtod(args[3]);
  }

  if (cb.ReadFile(args[0]) == 0) {
    cb.SetOrigin(o1, o2, o3);
    if (cb.WriteFile()) {
      printf("[E] setorigin: error writing %s\n", args(0));
      err = 100;
    } else {
      printf("[I] setorigin: origin set for file %s\n", args(0));
    }
  } else if (ts.ReadFile(args[0]) == 0) {
    ts.SetOrigin(o1, o2, o3);
    if (ts.WriteFile()) {
      printf("[E] setorigin: error writing %s\n", args(0));
      err = 100;
    } else {
      printf("[I] setorigin: origin set for %s\n", args(0));
    }
  } else {
    tmps.str("");
    tmps << "setorigin: couldn't read " << args[0] << " as 3D or 4D data";
    printErrorMsg(VB_ERROR, tmps.str());
  }

  return (err);
}

int setorigin_guess(tokenlist &args) {
  if (args.size() != 2) {
    setorigin_help();
    return (101);
  }

  Cube cb;
  Tes ts;
  int err = 0;
  int o1, o2, o3;
  stringstream tmps;

  if (cb.ReadFile(args[1]) == 0) {
    o1 = cb.dimx;
    o2 = cb.dimy;
    o3 = cb.dimz;
    guessorigin(o1, o2, o3);
    cb.SetOrigin(o1, o2, o3);
    if (cb.WriteFile()) {
      printf("[E] setorigin: error writing %s\n", args(1));
      err = 100;
    } else {
      printf("[I] setorigin: origin set for file %s\n", args(1));
      printErrorMsg(VB_INFO, tmps.str());
    }
  } else if (ts.ReadFile(args[1]) == 0) {
    o1 = ts.dimx;
    o2 = ts.dimy;
    o3 = ts.dimz;
    guessorigin(o1, o2, o3);
    ts.SetOrigin(o1, o2, o3);
    if (ts.WriteFile()) {
      printf("[E] setorigin: error writing %s\n", args(1));
      err = 100;
    } else {
      printf("[I] setorigin: origin set for %s\n", args(1));
    }
  }

  return (err);
}

int setorigin_map(tokenlist &args) {
  Cube mycube, refcube;
  stringstream tmps;

  if (args.size() < 3) {
    setorigin_help();
    return (101);
  }

  if (refcube.ReadFile(args[1])) {
    tmps.str("");
    tmps << "setorigin: invalid 3D file " << args[1];
    printErrorMsg(VB_ERROR, tmps.str());
    exit(5);
  }
  if (mycube.ReadFile(args[2])) {
    tmps.str("");
    tmps << "setorigin: invalid 3D file " << args[2];
    printErrorMsg(VB_ERROR, tmps.str());
    exit(100);
  }
  double ourxpos, ourypos, ourzpos;
  double refxpos, refypos, refzpos;
  mycube.GetCorner(ourxpos, ourypos, ourzpos);
  refcube.GetCorner(refxpos, refypos, refzpos);

  // first do the z, which is complicated
  int refx = refcube.origin[0];  // for convenience
  int refy = refcube.origin[1];
  int refz = refcube.origin[2];
  if (refx == 0 && refy == 0 && refz == 0) {
    refx = refcube.dimx;
    refy = refcube.dimy;
    refz = refcube.dimz;
    guessorigin(refx, refy, refz);
  }
  // first calculate the absolute position of the origin in mm
  double xposition = refxpos + (refx * refcube.voxsize[0]);
  double yposition = refypos + (refy * refcube.voxsize[1]);
  double zposition = refzpos + (refz * refcube.voxsize[2]);
  tmps.str("");
  tmps << "setorigin: x position " << xposition << "  refx: " << refx;
  printErrorMsg(VB_INFO, tmps.str());
  tmps.str("");
  tmps << "setorigin: y position " << yposition << "  refy: " << refy;
  printErrorMsg(VB_INFO, tmps.str());
  tmps.str("");
  tmps << "setorigin: z position " << zposition << "  refz: " << refz;
  printErrorMsg(VB_INFO, tmps.str());

  // now figure out what voxel that is on the reference brain
  xposition = (xposition - ourxpos) / mycube.voxsize[0];
  yposition = (yposition - ourypos) / mycube.voxsize[1];
  zposition = (zposition - ourzpos) / mycube.voxsize[2];

  // now do the x and y, which is just a matter of converting voxel
  // sizes, since we assume the images are aligned
  mycube.origin[0] = lround(xposition);
  mycube.origin[1] = lround(yposition);
  mycube.origin[2] = lround(zposition);

  int err = mycube.WriteFile();
  if (err) {
    tmps.str("");
    tmps << "setorigin: error writing the origin-corrected cube "
         << mycube.GetFileName();
    printErrorMsg(VB_ERROR, tmps.str());
    return (102);
  } else {
    tmps.str("");
    tmps << "setorigin: wrote origin-corrected cube " << mycube.GetFileName();
    printErrorMsg(VB_INFO, tmps.str());
    return (0);
  }
  return (0);
}

int setorigin_copy(tokenlist &args) {
  if (args.size() < 3) {
    setorigin_help();
    return (101);
  }

  int origin[3], err = 0;
  stringstream tmps;
  Tes ts;
  Cube cb;

  if (ts.ReadFile(args[1]) == 0) {
    origin[0] = ts.origin[0];
    origin[1] = ts.origin[1];
    origin[2] = ts.origin[2];
  } else if (cb.ReadFile(args[1]) == 0) {
    origin[0] = cb.origin[0];
    origin[1] = cb.origin[1];
    origin[2] = cb.origin[2];
  } else {
    printf("[E] setorigin: data must be in a recognized 3D or 4D format\n");
    exit(10);
  }

  for (size_t i = 2; i < args.size(); i++) {
    // vector<VBFF>tfiletypes=EligibleFileTypes(args[i]);
    if (ts.ReadFile(args[i]) == 0) {
      ts.origin[0] = origin[0];
      ts.origin[1] = origin[1];
      ts.origin[2] = origin[2];
      if (ts.WriteFile()) {
        tmps.str("");
        tmps << "setorigin: error writing to 4D image " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
      } else {
        tmps.str("");
        tmps << "setorigin: copied origin to 4D image " << args[i];
        printErrorMsg(VB_INFO, tmps.str());
      }
    } else if (cb.ReadFile(args[i]) == 0) {
      cb.origin[0] = origin[0];
      cb.origin[1] = origin[1];
      cb.origin[2] = origin[2];
      if (cb.WriteFile()) {
        tmps.str("");
        tmps << "setorigin: error writing to 3D image " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
      } else {
        tmps.str("");
        tmps << "setorigin: copied origin to 3D image " << args[i];
        printErrorMsg(VB_INFO, tmps.str());
      }
    } else {
      tmps.str("");
      tmps << "setorigin: file " << args[i]
           << " is not a recognized 3D or 4D file type";
      printErrorMsg(VB_ERROR, tmps.str());
      err = 190;
    }
  }
  return (err);
}

int setorigin_new(tokenlist &args, int mode) {
  if (args.size() < 2) {
    setorigin_help();
    return (101);
  }

  Cube cb;
  Tes ts;
  int err = 0;
  stringstream tmps;

  for (size_t i = 1; i < args.size(); i++) {
    if (cb.ReadFile(args[i]) == 0) {
      if (mode == m_strip)
        cb.SetOrigin(0, 0, 0);
      else if (mode == m_center)
        cb.SetOrigin(cb.dimx / 2, cb.dimy / 2, cb.dimz / 2);
      if (cb.WriteFile()) {
        tmps.str("");
        tmps << "setorigin: error writing " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
        err = 100;
      } else {
        tmps.str("");
        if (mode == m_strip)
          tmps << "setorigin: stripped origin from " << args[i];
        if (mode == m_center)
          tmps << "setorigin: set origin to center for " << args[i];
        printErrorMsg(VB_INFO, tmps.str());
      }
    } else if (ts.ReadFile(args[i]) == 0) {
      if (mode == m_strip)
        ts.SetOrigin(0, 0, 0);
      else if (mode == m_center)
        ts.SetOrigin(ts.dimx / 2, ts.dimy / 2, ts.dimz / 2);
      if (ts.WriteFile()) {
        tmps.str("");
        tmps << "setorigin: error writing " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
        err = 100;
      } else {
        tmps.str("");
        if (mode == m_strip)
          tmps << "setorigin: stripped origin from " << args[i];
        if (mode == m_center)
          tmps << "setorigin: set origin to center for " << args[i];
        printErrorMsg(VB_ERROR, tmps.str());
      }
    }
  }

  return (err);
}

void setorigin_help() { cout << boost::format(myhelp) % vbversion; }
