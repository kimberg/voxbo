
// vbhdr.cpp
// set the origin of one image to correspond to that of another
// Copyright (c) 1998-2006 by The VoxBo Development Team

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
#include "vbhdr.hlp.h"
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbutil.h"

int vbhdr_copy(vector<string> &hdr, string item, VBImage *im);
int vbhdr_set(vector<string> &hdr, string item);
int vbhdr_add(vector<string> &hdr, string item);
int vbhdr_strip(vector<string> &hdr, string item);
void vbhdr_help();
void vbhdr_version();

VBPrefs vbp;

int main(int argc, char *argv[]) {
  vbp.init();

  int err = 0;
  tzset();         // make sure all times are timezone corrected
  if (argc < 4) {  // not enough args, display autodocs
    vbhdr_help();
    exit(106);
  }
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);

  string setline, addline, deleteline, neworient, copysrc;
  int originflag = 0, voxsizeflag = 0, trflag = 0;
  float voxsizes[3], origin[3], tr = 1000;
  vector<string> filelist;
  // initialization to make the compiler happy
  voxsizes[0] = 0.0;
  voxsizes[1] = 0.0;
  voxsizes[2] = 0.0;
  origin[0] = 0.0;
  origin[1] = 0.0;
  origin[2] = 0.0;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-s" && i < args.size() - 1)
      setline = args[++i];
    else if (args[i] == "-a" && i < args.size() - 1)
      addline = args[++i];
    else if (args[i] == "-d" && i < args.size() - 1)
      deleteline = args[++i];
    else if (args[i] == "-c" && i < args.size() - 1)
      copysrc = args[++i];
    else if (args[i] == "-o" && i < args.size() - 1)
      neworient = args[++i];
    else if (args[i] == "-z" && i < args.size() - 3) {
      voxsizes[0] = strtod(args[i + 1]);
      voxsizes[1] = strtod(args[i + 2]);
      voxsizes[2] = strtod(args[i + 3]);
      voxsizeflag = 1;
      i += 3;
    } else if (args[i] == "-t" && i < args.size() - 1) {
      tr = strtod(args[++i]);
      trflag = 1;
    } else if (args[i] == "-x" && i < args.size() - 3) {
      origin[0] = strtod(args[i + 1]);
      origin[1] = strtod(args[i + 2]);
      origin[2] = strtod(args[i + 3]);
      originflag = 1;
      i += 3;
    } else if (args[i] == "-h") {
      vbhdr_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbhdr_version();
      exit(0);
    } else
      filelist.push_back(args[i]);
  }

  for (int i = 0; i < (int)filelist.size(); i++) {
    Cube cb;
    Tes ts;
    VBImage *im = NULL;
    vector<string> header;

    if ((cb.ReadFile(filelist[i])) == 0) {
      im = &cb;
      header = cb.header;
    } else if ((ts.ReadFile(filelist[i])) == 0) {
      im = &ts;
      header = ts.header;
    } else {
      printf("[E] vbhdr: unrecognized file type\n");
      exit(104);
    }

    if (setline.size()) {
      err = vbhdr_set(header, setline);
      printf("[I] vbhdr: setting the following header: %s\n", setline.c_str());
    } else if (addline.size()) {
      err = vbhdr_add(header, addline);
      printf("[I] vbhdr: adding the following header: %s\n", addline.c_str());
    } else if (deleteline.size()) {
      err = vbhdr_strip(header, deleteline);
      printf("[I] vbhdr: deleting the following header: %s\n",
             deleteline.c_str());
    } else if (copysrc.size()) {
      err = vbhdr_copy(header, copysrc, im);
      printf("[I] vbhdr: copying header from %s\n", copysrc.c_str());
    } else if (neworient.size()) {
      im->orient = neworient;
      printf("[I] vbhdr: setting orientation to %s\n", neworient.c_str());
    } else if (originflag) {
      im->origin[0] = (int)round(origin[0]);
      im->origin[1] = (int)round(origin[1]);
      im->origin[2] = (int)round(origin[2]);
      im->qoffset[0] = 0 - origin[0];
      im->qoffset[1] = 0 - origin[1];
      im->qoffset[2] = 0 - origin[2];
      printf("[I] vbhdr: setting origin to %.2f %.2f %.2f\n", im->qoffset[0],
             im->qoffset[1], im->qoffset[2]);
      printf(
          "[I] vbhdr: note that origins are counting from 0, and that "
          "non-integer\n");
      printf("[I] vbhdr: origins only work for NIfTI files\n");
    } else if (voxsizeflag) {
      im->voxsize[0] = voxsizes[0];
      im->voxsize[1] = voxsizes[1];
      im->voxsize[2] = voxsizes[2];
      printf("[I] vbhdr: setting voxel sizes to %.4f %.4f %.4f\n",
             im->voxsize[0], im->voxsize[1], im->voxsize[2]);
    } else if (trflag) {
      im->voxsize[3] = tr;
      printf("[I] vbhdr: TR to %.4fms\n", im->voxsize[3]);
    } else {
      printf("[E] vbhdr: nothing to do\n");
      return 100;
    }

    if (cb.data) {
      cb.header = header;
      err = cb.WriteFile();
      if (err) {
        printErrorMsg(VB_ERROR, "vbhdr: Error writing cube to file.\n");
        exit(err);
      }
    } else {
      ts.header = header;
      err = ts.WriteFile();
      if (err) {
        printErrorMsg(VB_ERROR, "vbhdr: Error writing cube to file.\n");
        exit(err);
      }
    }
  }

  exit(0);
}

int vbhdr_copy(vector<string> &hdr, string item, VBImage *im) {
  Tes ts;
  if (ts.ReadHeader(item) == 0) {
    hdr = ts.header;
    im->CopyHeader(ts);
    return 0;
  }
  Cube cb;
  if (cb.ReadHeader(item) == 0) {
    hdr = cb.header;
    im->CopyHeader(cb);
    return 0;
  }
  return 101;
}

int vbhdr_set(vector<string> &hdr, string item) {
  vector<string> newhdr;
  tokenlist striptags, oldtags;
  striptags.ParseLine(item);
  int foundit = 0;
  for (int i = 0; i < (int)hdr.size(); i++) {
    oldtags.ParseLine(hdr[i]);
    if (oldtags[0] != striptags[0])
      newhdr.push_back(hdr[i]);
    else {
      newhdr.push_back(item);
      foundit = 1;
    }
  }
  if (!foundit) newhdr.push_back(item);
  hdr = newhdr;
  return (0);
}

int vbhdr_add(vector<string> &hdr, string item) {
  string newitem = item;
  replace_string(newitem, "{DATE}", timedate());
  // struct utsname names;
  // uname(&names);
  replace_string(newitem, "{HOST}", vbp.thishost.nickname);
  hdr.push_back(newitem);
  return (0);
}

int vbhdr_strip(vector<string> &hdr, string item) {
  vector<string> newhdr;
  tokenlist striptags, oldtags;
  striptags.ParseLine(item);
  for (int i = 0; i < (int)hdr.size(); i++) {
    oldtags.ParseLine(hdr[i]);
    if (oldtags[0] != striptags[0]) newhdr.push_back(hdr[i]);
  }
  hdr = newhdr;
  return (0);
}

void vbhdr_help() { cout << boost::format(myhelp) % vbversion; }

void vbhdr_version() { printf("VoxBo vbhdr (v%s)\n", vbversion.c_str()); }
