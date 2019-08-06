
// vbtool.cpp
// general-purpose voxbo tool
// Copyright (c) 2010 by The VoxBo Development Team

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
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbtool.hlp.h"
#include "vbutil.h"

VBPrefs vbp;

void vbtool_help();
void vbtool_version();
void vbtool_tip();
void vbtool_dumpconfig();

int main(int argc, char *argv[]) {
  vbp.init();
  vbp.read_jobtypes();
  if (argc < 2) {
    vbtool_help();
    exit(0);
  }
  tokenlist args;
  string mode = argv[1];
  bool f_endl = 0;  // newlines around tips
  int f_ntips = 1;  // number of tips
  args.Transfer(argc - 2, argv + 2);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      vbtool_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbtool_version();
      exit(0);
    } else if (args[i] == "-n" && i < args.size() - 1) {
      f_ntips = strtol(args[++i]);
      if (f_ntips > 50) f_ntips = 50;
      if (f_ntips < 1) f_ntips = 1;
    } else if (args[i] == "-w") {
      f_endl = 1;
    }
  }

  if (mode == "dirname") {
  } else if (mode == "conf") {
    vbtool_dumpconfig();
  } else if (mode == "contrast") {
    cout << contrasthelp;
  } else if (mode == "tip") {
    if (f_endl) cout << endl;
    for (int i = 0; i < f_ntips; i++) {
      if (i > 0) cout << endl;
      vbtool_tip();
    }
    if (f_endl) cout << endl;
  } else if (mode == "pri") {
    cout << priorityhelp;
  } else {
    cout << "[E] vbtool: unrecognized function" << endl;
    vbtool_help();
  }
  exit(0);
}

void vbtool_tip() {
  tokenlist lines;
  vglob vg;
  vg.append(vbp.rootdir + "/etc/tips.txt", vglob::f_filesonly);
  vg.append(vbp.rootdir + "/etc/localtips.txt", vglob::f_filesonly);
  vg.append(vbp.userdir + "/etc/tips.txt", vglob::f_filesonly);
  vg.append(vbp.homedir + "/.voxbo/tips.txt", vglob::f_filesonly);
  vg.append(vbp.homedir + "/voxbotips.txt", vglob::f_filesonly);
  for (size_t i = 0; i < vg.size(); i++) {
    tokenlist tmp;
    tmp.ParseFile(vg[i]);
    lines += tmp;
    lines += "------------";
  }
  string tip;
  map<uint32, string> tips;
  vbforeach(string line, lines) {
    if (line.substr(0, 3) == "---") {
      if (tip.size()) tips[VBRandom()] = tip;
      tip = "";
      continue;
    }
    // first line can't begin with a # or it's considered a comment
    if (tip.empty() && line[0] == '#') continue;
    // first line must also have some non-whitespace
    if (tip.empty() && xstripwhitespace(line).empty()) continue;
    if (tip.empty())
      tip = line;
    else
      tip += "\n" + line;
  }
  if (tip.size()) tips[VBRandom()] = tip;
  string mytip = tips.begin()->second;
  mytip = xstripwhitespace(mytip, "\n");
  cout << mytip << endl;
}

void vbtool_dumpconfig() {
  printf("\nVoxBo configuration info:\n\n");

  if (vbp.su)
    printf("You are a VoxBo super-user.\n");
  else
    printf("You are a regular VoxBo user.\n");

  // user stuff
  printf("User Config:\n");
  printf("  username: %s\n", vbp.username.c_str());
  printf("  email address: %s\n", vbp.email.c_str());
  printf("  home directory: %s\n", vbp.homedir.c_str());
  printf("  user directory: %s\n", vbp.userdir.c_str());

  // system stuff
  printf("System Config:\n");
  printf("  root directory %s\n", vbp.rootdir.c_str());
  printf("  queuedir: %s\n", vbp.queuedir.c_str());
  printf("  host name: %s\n", vbp.thishost.hostname.c_str());
  printf("  host nickname: %s\n", vbp.thishost.nickname.c_str());
  printf("  ncores: %d (%s mode)\n", vbp.cores,
         (vbp.cores ? "desktop" : "cluster"));
  printf("\n");
}

void vbtool_help() { cout << boost::format(myhelp) % vbversion; }

void vbtool_version() { printf("VoxBo vbtool (v%s)\n", vbversion.c_str()); }
