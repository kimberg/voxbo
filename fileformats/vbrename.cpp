
// vbrename.cpp
// rename dicom files that have appropriate fields, etc.
// Copyright (c) 2003-2005 by The VoxBo Development Team

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
#include "vbio.h"
#include "vbrename.hlp.h"
#include "vbutil.h"

class VBRename {
 public:
  int forceflag;
  bool f_anon;
  int Go(tokenlist &args);
  int Rename(string filename);
};

extern "C" {

#include "dicom.h"
void vbrename_help();
void vbrename_version();

int main(int argc, char *argv[]) {
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  if (args.size() < 1) {
    vbrename_help();
    exit(0);
  }
  VBRename vbr;
  int err = vbr.Go(args);
  exit(err);
}

int VBRename::Go(tokenlist &args) {
  int bad = 0, good = 0;
  struct stat st;
  tokenlist filelist, targetlist;

  forceflag = 0;
  f_anon = 1;
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-f") {
      forceflag = 1;
      continue;
    } else if (args[i] == "-n")
      f_anon = 0;
    else if (args[i] == "-h") {
      vbrename_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbrename_version();
      exit(0);
    }
    targetlist.Add(args[i]);
  }

  for (size_t i = 0; i < targetlist.size(); i++) {
    filelist.clear();
    // no such file or directory
    if (stat(targetlist(i), &st)) {
      bad++;
      continue;
    }
    // it's a directory, do all the files
    if (S_ISDIR(st.st_mode)) {
      vglob vg(targetlist[i] + "/*");
      for (size_t m = 0; m < vg.size(); m++) filelist.Add(vg[m]);
    }
    // it's a file or something, try to do it
    else
      filelist.Add(args[i]);
    for (size_t j = 0; j < filelist.size(); j++) {
      if (Rename(filelist[j])) {
        cout << format("[E] vbrename: error renaming %s\n") % filelist[j];
        bad++;
      } else
        good++;
    }
  }
  cout << format("[I] vbrename: %d files renamed successfully, %d errors\n") %
              good % bad;
  return 0;
}

int VBRename::Rename(string fname) {
  dicominfo dci;
  struct stat st;
  int i;
  if (read_dicom_header(fname, dci)) return 101;
  string newdirname =
      (format("%04d_%s") % dci.series % xstripwhitespace(dci.protocol)).str();
  string newfilename = (format("%04d_%s_%s") % dci.instance %
                        xstripwhitespace(dci.date) % xstripwhitespace(dci.time))
                           .str();
  // make input file user-readwritable if possible
  if (!stat(fname.c_str(), &st)) {
    chmod(fname.c_str(), st.st_mode | S_IRUSR | S_IWUSR);
  }
  // make containing dir user-readwritable if possible
  if (!stat(xdirname(fname).c_str(), &st)) {
    chmod(xdirname(fname).c_str(), st.st_mode | S_IRUSR | S_IWUSR);
  }
  for (i = 0; i < (int)newdirname.size(); i++)
    if (newdirname[i] == '/') newdirname[i] = '_';
  for (i = 0; i < (int)newfilename.size(); i++)
    if (newfilename[i] == '/') newfilename[i] = '_';
  if (newdirname.size() < 1 || newdirname.size() < 1) return 102;

  string dirname = xdirname(fname) + (string) "/" + newdirname;
  if (stat(dirname.c_str(), &st)) {
    if (mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)) {
      if (errno != EEXIST) return 140;
    }
    if (stat(dirname.c_str(), &st)) return 160;
  }
  if (!(S_ISDIR(st.st_mode))) return 150;

  string newname = dirname + "/" + newfilename;
  int err = rename(fname.c_str(), newname.c_str());
  if (err || !f_anon) return err;
  // anonymize
  set<uint16> stripgroups;
  set<dicomge> stripges;
  set<string> stripvrs;
  int removed = 1;
  err = anonymize_dicom_header(newname, newname, "", stripgroups, stripges,
                               stripvrs, removed);
  return err;
}

void vbrename_help() { cout << boost::format(myhelp) % vbversion; }

void vbrename_version() { printf("VoxBo vbrename (v%s)\n", vbversion.c_str()); }

}  // extern "C"
