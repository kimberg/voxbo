
// vbconv.cpp
// convert data from one format to another
// Copyright (c) 2006-2007 by The VoxBo Development Team

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

using namespace std;

#include <stdio.h>
#include <string.h>
#include <sstream>
#include "vbconv.hlp.h"
#include "vbio.h"
#include "vbutil.h"

int ConvertMultiple(list<string> &filelist, int nanflag, int floatflag,
                    Tes &newtes);
int WriteTes(Tes &newtes, string outfile, int extractflag, int floatflag,
             int nanflag, set<int> includeset, set<int> excludeset);
void vbconv_help();
void vbconv_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbconv_help();
    exit(0);
  }
  tokenlist args;
  string outfile;
  int floatflag = 0, nanflag = 0, extractflag = 0;
  set<int> includeset, excludeset;
  list<string> filelist;
  VBFF nullff;

  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-f")
      floatflag = 1;
    else if (args[i] == "-n")
      nanflag = 1;
    else if (args[i] == "-i" && i < args.size() - 1) {
      includeset = numberset(args[++i]);
      if (includeset.empty()) {
        cout << "[E] vbconv: invalid inclusion range specified with -i\n";
        exit(111);
      }
    } else if (args[i] == "-e" && i < args.size() - 1) {
      excludeset = numberset(args[++i]);
      if (excludeset.empty()) {
        cout << "[E] vbconv: invalid exclusion range specified with -e\n";
        exit(111);
      }
    } else if (args[i] == "-x" && i < args.size() - 1)
      extractflag = 1;
    else if (args[i] == "-v") {
      vbconv_version();
      exit(0);
    } else if (args[i] == "-h") {
      vbconv_help();
      exit(0);
    } else if (args[i] == "-o" && i < args.size() - 1)
      outfile = args[++i];
    else {
      filelist.push_back(args[i]);
    }
  }

  if (filelist.size() < 1) {
    printf("[E] vbconv: requires at least one input file\n");
    exit(10);
  }
  // if there's no -o flag and exactly two files specified, convert
  // the first to the second
  if (filelist.size() == 2 && outfile.size() == 0) {
    outfile = filelist.back();
    filelist.pop_back();
  }
  if (outfile.size() == 0) {
    printf("[E] vbconv: requires an output filename be provided\n");
    exit(11);
  }

  // multiple files, must be 3D/4D combination
  if (filelist.size() > 1) {
    Tes newtes;
    ConvertMultiple(filelist, nanflag, floatflag, newtes);
    if (WriteTes(newtes, outfile, extractflag, floatflag, nanflag, includeset,
                 excludeset))
      exit(223);
    exit(0);  // just in case
  }
  int err;
  // just one file, see what kind
  Cube cb;
  if (cb.ReadFile(filelist.front()) == 0) {
    cb.fileformat = nullff;
    if (floatflag && cb.datatype != vb_float) cb.convert_type(vb_float);
    if (nanflag) cb.removenans();
    if ((err = cb.WriteFile(outfile))) {
      printf("[E] vbconv: error %d writing %s\n", err, outfile.c_str());
      exit(4);
    } else {
      printf("[I] vbconv: wrote cube %s\n", outfile.c_str());
      exit(0);
    }
  }
  Tes ts;
  if (ts.ReadFile(filelist.front()) == 0) {
    ts.fileformat = nullff;
    if ((err = WriteTes(ts, outfile, extractflag, floatflag, nanflag,
                        includeset, excludeset))) {
      printf("[E] vbconv: error %d writing %s\n", err, outfile.c_str());
      exit(4);
    } else {
      printf("[I] vbconv: wrote 4D volume %s\n", outfile.c_str());
      exit(0);
    }
  }
  VB_Vector vv;
  if (vv.ReadFile(*filelist.begin()) == 0) {
    // vv.fileformat=nullff; // FIXME -- can we set 1d fileformat?
    if (vv.WriteFile(outfile)) {
      printf("[E] vbconv: error writing %s\n", outfile.c_str());
      exit(4);
    } else {
      printf("[I] vbconv: wrote vector %s\n", outfile.c_str());
      exit(0);
    }
  }
  printf("[E] vbconv: couldn't make sense of file %s\n",
         filelist.front().c_str());
  exit(100);
}

int ConvertMultiple(list<string> &filelist, int nanflag, int floatflag,
                    Tes &newtes) {
  // printf("[I] vbconv: converting %d files\n",(int)filelist.size());
  // pre-scan for dimensions
  int dimx = 0, dimy = 0, dimz = 0, dimt = 0;
  VB_datatype dtype = vb_short;
  for (list<string>::iterator ff = filelist.begin(); ff != filelist.end();
       ff++) {
    Cube cb;
    Tes ts;
    VBImage *im;
    if (cb.ReadHeader(*ff) == 0) {
      im = &cb;
      dimt++;
    } else if (ts.ReadHeader(*ff) == 0) {
      im = &ts;
      dimt += ts.dimt;
    } else {
      printf("[E] vbconv: couldn't read %s\n", ff->c_str());
      exit(51);
    }
    if (ff == filelist.begin()) {
      dimx = im->dimx;
      dimy = im->dimy;
      dimz = im->dimz;
      dimt = 1;
      dtype = im->datatype;
    } else if (im->dimx != dimx || im->dimy != dimy || im->dimz != dimz) {
      printf("[E] vbconv: incompatible dimensions in file %s\n", ff->c_str());
      exit(51);
    }
  }
  int index = 0;
  if (floatflag) dtype = vb_float;

  // now grab all the images
  newtes.SetVolume(dimx, dimy, dimz, dimt, dtype);
  int findex = 1;
  for (list<string>::iterator ff = filelist.begin(); ff != filelist.end();
       ff++) {
    printf("[I] vbconv: converting file %d of %d\n", findex++,
           (int)filelist.size());
    newtes.AddHeader("vbconv: included file " + *ff);
    Cube cb;
    Tes ts;
    if (cb.ReadFile(*ff) == 0) {
      newtes.SetCube(index++, cb);
    } else if (ts.ReadFile(*ff) == 0) {
      for (int i = 0; i < ts.dimt; i++) {
        ts.getCube(i, cb);
        newtes.SetCube(index++, cb);
      }
    } else {
      printf("[E] vbconv: couldn't read data from %s\n", ff->c_str());
      exit(5);
    }
  }
  if (nanflag) newtes.removenans();
  return 0;
}

int WriteTes(Tes &newtes, string outfile, int extractflag, int floatflag,
             int nanflag, set<int> includeset, set<int> excludeset) {
  VBFF nullff;

  // merge include and exclude lists into just an include list
  if (includeset.size() && excludeset.size()) {
    for (set<int>::iterator i = excludeset.begin(); i != excludeset.end(); i++)
      includeset.erase(*i);
    excludeset.clear();
  }
  newtes.fileformat = nullff;
  if (extractflag == 0) {
    if (includeset.size()) newtes.resizeInclude(includeset);
    if (excludeset.size()) newtes.resizeExclude(excludeset);
    if (floatflag && newtes.datatype != vb_float) newtes.convert_type(vb_float);
    if (nanflag) newtes.removenans();
    if (newtes.WriteFile(outfile)) {
      printf("[E] vbconv: couldn't write output file %s\n", outfile.c_str());
      exit(55);
    } else {
      printf("[I] vbconv: wrote file %s\n", outfile.c_str());
      exit(0);
    }
  }
  printf("[I] vbconv: writing %d volumes: ", newtes.dimt);
  fflush(stdout);
  int err;
  for (int i = 0; i < newtes.dimt; i++) {
    if (includeset.size() && !includeset.count(i)) continue;
    if (excludeset.count(i)) continue;
    Cube cb = newtes[i];
    string tmpout = outfile;
    char indx[10];
    sprintf(indx, "%05d", i);
    size_t pos, minpos = 0;
    while ((pos = tmpout.find("XXX", minpos)) != string::npos) {
      tmpout.replace(pos, 3, indx);
      minpos = pos + 5;
    }
    if ((err = cb.WriteFile(tmpout))) {
      printf("[E] vbconv: couldn't write %s [%d]\n", tmpout.c_str(), err);
      exit(5);
    }
    printf(".");
    fflush(stdout);
  }
  printf(" done\n");
  exit(0);
}

void vbconv_help() { cout << boost::format(myhelp) % vbversion; }

void vbconv_version() { printf("VoxBo vbconv (v%s)\n", vbversion.c_str()); }
