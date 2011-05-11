
// vb2imgs.cpp
// convert arbitrary 4D data to a directory of ANALYZE(tm) files
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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include "vbutil.h"
#include "vbio.h"
#include "vb2imgs.hlp.h"

void vb2imgs_help();
void vb2imgs_version();

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vb2imgs_help();
    exit(0);
  }
  string prefix;
  int err,floatflag=0,nanflag=0;
  tokenlist args;

  arghandler ah;
  string errstr;
  ah.setArgs("-p","--prefix",1);
  ah.setArgs("-f","--nofloat",0);
  ah.setArgs("-n","--nonan",0);
  ah.setArgs("-h","--help",0);
  ah.setArgs("-v","--version",0);
  ah.parseArgs(argc,argv);

  if ((errstr=ah.badArg()).size()) {
    cout << "[E] vb2imgs: " << errstr << endl;
    exit(10);
  }
  if (ah.flagPresent("-h")) {
    vb2imgs_help();
    exit(0);
  }
  if (ah.flagPresent("-v")) {
    vb2imgs_version();
    exit(0);
  }
  if (ah.flagPresent("-f"))
    floatflag=1;
  args=ah.getFlaggedArgs("-p");
  if (args.size()==1) {
    prefix=args[0];
    exit(0);
  }

  args=ah.getUnflaggedArgs();
  printf("[I] vb2imgs: converting %s to an img directory\n",args(0));

  Tes mytes;
  mytes.ReadFile(args[0]);
  if (!mytes.data_valid) {
    printf("[E] vb2imgs: couldn't read input file %s\n",args(0));
    exit(5);
  }

  // remove NaNs and Infs if requested
  if (nanflag)
    mytes.removenans();
  // convert to float if requested
  if (floatflag && mytes.datatype != vb_float)
    mytes.convert_type(vb_float,VBSETALT|VBNOSCALE);

  err=mytes.SetFileFormat("imgdir");  // FIXME should probably check err
  mytes.SetFileName(args[1]);
  mytes.ReparseFileName();
  err=mytes.WriteFile();
  if (err) {
    printf("[E] vb2imgs: done, but there were errors writing files.\n");
  }
  else {
    printf("[I] vb2imgs: done.\n");
  }
}

void
vb2imgs_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vb2imgs_version()
{
  printf("VoxBo vb2imgs (v%s)\n",vbversion.c_str());
}
