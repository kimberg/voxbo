
// tes2cub.cpp
// extract a single 3D image from a TES1 file
// Copyright (c) 1998-2002 by The VoxBo Development Team

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
#include "vbutil.h"
#include "vbio.h"
#include "tes2cub.hlp.h"

void tes2cub_help();

int
main(int argc,char *argv[])
{
  tokenlist args;
  Tes *tes;
  Cube *cub;
  int ind=0;
  stringstream tmps;

  args.Transfer(argc-1,argv+1);

  if (args.size() == 0) {
    tes2cub_help();
    exit(0);
  }

  if (args.size() != 2 && args.size() != 3) {
    tes2cub_help();
    exit(5);
  }

  if (args.size() == 3)
    ind = strtol(args[2]);
  tes = new Tes();
  if (tes->ReadFile(args[0])) {
    tmps.str("");
    tmps << "tes2cub: couldn't read tes file " << args[0];
    printErrorMsg(VB_ERROR,tmps.str());
    exit(5);
  }
  if (!tes->data_valid) {
    tmps.str("");
    tmps << "tes2cub: tes file " << args[0] << "isn't valid.";
    printErrorMsg(VB_ERROR,tmps.str());
    exit(5);
  }
  if (ind > tes->dimt) {
    tmps.str("");
    tmps << "tes2cub: index (" << ind << ") is beyond the last image (" << tes->dimt << ").";
    printErrorMsg(VB_ERROR,tmps.str());
    exit(5);
  } 
  cub = new Cube((*tes)[ind]);
  if (!cub->data_valid) {
    tmps.str("");
    tmps << "tes2cub: error extracting the cube from the 4D file (shouldn't happen!).";
    printErrorMsg(VB_ERROR,tmps.str());
    exit(5);
  } 
  if (!cub->WriteFile(args[1])) {
    tmps.str("");
    tmps << "tes2cub: wrote cube " << ind << " to file " << args[1] << ".";
    printErrorMsg(VB_INFO,tmps.str());
    exit(0);
  }
  else {
    tmps.str("");
    tmps << "tes2cub: failed to write extracted cube to file " << args[1] << ".";
    printErrorMsg(VB_INFO,tmps.str());
    exit(5);
  }
  exit(0);
}

void
tes2cub_help()
{
  cout << boost::format(myhelp) % vbversion;
}
