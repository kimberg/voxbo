
// dcmsplit.cpp
// split a DICOM file into identifying and non-identifying parts
// Copyright (c) 2009-2010 by The VoxBo Development Team

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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include "dcmsplit.hlp.h"
#include "vbio.h"
#include "vbutil.h"

using namespace std;
using boost::format;

extern "C" {

#include "dicom.h"

int32 hextol(const string &str);
int do_dcmsplit(string infile);
void dcmsplit_help();
void dcmsplit_version();

// argh!  globals!
string outfile, outpat, anonfile, anonpat;
set<uint16> stripgroups;
set<dicomge> stripges;
set<string> stripvrs;
bool f_checkfirst = 1;

int main(int argc, char **argv) {
  if (argc < 2) {
    dcmsplit_help();
    exit(0);
  }
  vector<string> filelist;
  bool f_recursive = 0;

  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      dcmsplit_help();
      exit(0);
    } else if (args[i] == "-v") {
      dcmsplit_version();
      exit(0);
    } else if (args[i] == "-dc") {
      f_checkfirst = 0;
      exit(0);
    } else if (args[i] == "-g" && i < args.size() - 1)
      stripgroups.insert(hextol(args[++i]));
    else if (args[i] == "-e" && i < args.size() - 2) {
      stripges.insert(dicomge(hextol(args[i + 1]), hextol(args[i + 2])));
      i += 2;
    } else if (args[i] == "-o" && i < args.size() - 1)
      outpat = args[++i];
    else if (args[i] == "-s" && i < args.size() - 1)
      anonpat = args[++i];
    else if (args[i] == "-r")
      f_recursive = 1;
    else if (args[i] == "-d" && i < args.size() - 1)
      stripvrs.insert(args[++i]);
    else
      filelist.push_back(args[i]);
  }

  if (!f_recursive) {  // just the files we listed
    vbforeach(string fn, filelist) {
      outfile = outpat;
      replace_string(outfile, "FILE", fn);
      replace_string(outfile, "BASE", xsetextension(fn, "", 1));
      anonfile = anonpat;
      replace_string(anonfile, "FILE", fn);
      replace_string(anonfile, "BASE", xsetextension(fn, "", 1));
      do_dcmsplit(fn);
    }
  } else {  // the files/dirs listed and their recursive contents
    deque<string> hitlist;
    vbforeach(string s, filelist) hitlist.push_back(s);
    while (hitlist.size()) {
      string ff = hitlist.front();
      hitlist.pop_front();
      if (vb_fileexists(ff)) {
        do_dcmsplit(ff);
        continue;
      }
      if (!vb_direxists(ff)) continue;
      // okay, we are a directory
      vglob vg(ff + "/*");
      for (size_t ii = 0; ii < vg.size(); ii++) hitlist.push_back(vg[ii]);
    }
  }
  exit(0);
}

int do_dcmsplit(string infile) {
  int err;
  int removed = 1;
  if (f_checkfirst) {
    err = anonymize_dicom_header(infile, "", "", stripgroups, stripges,
                                 stripvrs, removed);
    if (err > 200) {
      cout << format(
                  "[E] dcmsplit: %s is not a well-formed DICOM file (%d)\n") %
                  infile % err;
      return 180;
    } else if (err) {
      cout << format("[E] dcmsplit: error %d checking %s\n") % err % infile;
      return 180;
    }
  }
  if (removed == 0) {
    cout << format("[E] dcmsplit: skipping %s\n") % infile;
    return 0;
  }

  if (outfile.empty()) {
    if ((err = anonymize_dicom_header(infile, infile, anonfile, stripgroups,
                                      stripges, stripvrs, removed))) {
      if (err == 222)
        cout << format("[E] dcmsplit: %s is not a well-formed DICOM file\n") %
                    infile;
      else
        cout << format("[E] dcmsplit: %s: anonymize error %d\n") % infile % err;
    } else if (removed == 0)
      cout << format("[I] dcmsplit: skipping %s (no identifying fields)\n") %
                  infile;
    else
      cout << format("[I] dcmsplit: anonymized %s\n") % infile;
  } else {
    if ((err = anonymize_dicom_header(infile, outfile, anonfile, stripgroups,
                                      stripges, stripvrs, removed))) {
      if (err == 222)
        cout << format("[E] dcmsplit: %s is not a well-formed DICOM file\n") %
                    infile;
      else
        cout << format("[E] dcmsplit: %s: anonymize error %d\n") % infile % err;
    } else if (removed == 0) {
      cout << format("[I] dcmsplit: skipping %s (no identifying fields)\n") %
                  infile;
    } else {
      cout << format("[I] dcmsplit: splitting %s\n") % infile;
      cout << format("[I] dcmsplit:   de-identified data are in: %s\n") %
                  outfile;
      if (anonfile.size())
        cout << format("[I] dcmsplit:   identifying data are in: %s\n") %
                    anonfile;
    }
  }
  return 0;
}

int32 hextol(const string &str) {
  string tmp = xstripwhitespace(str);
  stringstream ss(tmp);
  int32 res;
  ss >> hex >> res;
  if (ss.fail() || (size_t)ss.tellg() < tmp.size()) return 0;
  return res;
}

void dcmsplit_help() { cout << boost::format(myhelp) % vbversion; }

void dcmsplit_version() { printf("VoxBo dcmsplit (v%s)\n", vbversion.c_str()); }
}
