
// cub2pngs.cpp
// turn a cub file into a directory of PNG files
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
#include "vbutil.h"
#include "vbio.h"
#include "cub2pngs.hlp.h"

void cub2pngs_help();

int
main(int argc,char *argv[])
{
  tokenlist args;
  Cube *cub;

  args.Transfer(argc-1,argv+1);

  if (args.size() == 0) {
    cub2pngs_help();
    exit(0);
  }

  if (args.size() != 2 && args.size() != 3) {
    cub2pngs_help();
    exit(5);
  }

  if (args.size() == 2)
    args.Add(xfilename(args[1]));
  cub = new Cube;
  if (cub->ReadFile(args[0])) {
    printf("cub2pngs: couldn't read cub file %s\n",args[0].c_str());
    exit(109);
  }
  if (!cub->data_valid) {
    printf("cub2pngs: couldn't validate cub file %s\n",args[0].c_str());
    exit(100);
  }
  if (createfullpath(args[1])) {
    printf("cub2pngs: couldn't find/create containing directory %s.",args(1));
    exit(101);
  }
  chdir(args(1));
  char name[STRINGLEN];
  printf("Writing PNG files");
  for (int i=0; i<cub->dimz; i++) {
    printf("."); fflush(stdout);
    sprintf(name,"%s%03d.png",args(2),i);
    if (WritePNG(*cub,i,name))
      printf("\ncub2pngs: error writing PNG file %s.\n",name);
  }
  printf("done\n");
  exit(0);
}

void
cub2pngs_help()
{
  cout << boost::format(myhelp) % vbversion;
}
