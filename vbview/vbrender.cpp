
// vbrender.cpp
// command line interface for rendering a statistical overlay
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

#include <qimage.h>
#include <string>
#include <vector>
#include <signal.h>
#include "tokenlist.h"
#include "vbprefs.h"
#include "vbutil.h"
#include "vbio.h"
#include "vb_render.h"

class RenderMe {
public:
  RenderMe();
  int Go(tokenlist &args);
  string anatomyname;
  string overlayname;
  string outfilename;
  string maskname;
  Cube anatomy;
  Cube overlay;
  Cube mask;
  VBRenderer vbr;
  double low,high;
  void help();
};

int
main(int argc,char **argv)
{
  tokenlist args;

  // fork off xvfb
  pid_t vfbpid=fork();
  
  if (vfbpid==0)
    execlp("Xvfb",":11",NULL);
  setenv("DISPLAY",":11",1);
  
  args.Transfer(argc-1,argv+1);
  RenderMe rm;
  int ret=rm.Go(args);

  // kill xvfb
  if (vfbpid>1)
    kill(vfbpid,9);

  exit(ret);
}

RenderMe::RenderMe()
{
  low=0; high=10;
}

int
RenderMe::Go(tokenlist &args)
{
  if (!args.size()) {
    help();
    return 102;
  }
  for (int i=0; i<args.size(); i++) {
    if (args[i]=="-m" && i+1 < args.size()) {
      maskname=args[i+1];
      i++;
    }
    else if (args[i]=="-d" && i+1 < args.size()) {
      overlayname=args[i+1];
      i++;
    }
    else if (args[i]=="-o" && i+1 < args.size()) {
      outfilename=args[i+1];
      i++;
    }
    else if (args[i]=="-a" && i+1 < args.size()) {
      anatomyname=args[i+1];
      i++;
    }
    else if (args[i]=="-1") {
      vbr.twotailed=0;
    }
    else if (args[i]=="-t" && i+2 < args.size()) {
      vbr.low=strtod(args[i+1]);
      vbr.high=strtod(args[i+2]);
      i+=2;
    }
  }

  if (outfilename.size() && !anatomyname.size() && !overlayname.size()) {
    help();
    return 101;
  }

  char tmp[STRINGLEN];

  if (anatomyname.size()) {
    anatomy.ReadFile(anatomyname);
    if (!anatomy.data_valid){
      sprintf(tmp,"vbrender: anatomy file %s not valid",anatomyname.c_str());
      printErrorMsg(VB_ERROR,tmp);
      return 102;
    }
    vbr.anatomy=&anatomy;
  }

  if (overlayname.size()) {
    overlay.ReadFile(overlayname);
    if (!overlay.data_valid){
      sprintf(tmp,"vbrender: statistical overlay file %s not valid",overlayname.c_str());
      printErrorMsg(VB_ERROR,tmp);
      return 102;
    }
    vbr.overlay=&overlay;
  }

  if (maskname.size()) {
    mask.ReadFile(maskname);
    if (!mask.data_valid){
      sprintf(tmp,"vbrender: mask file %s not valid",maskname.c_str());
      printErrorMsg(VB_ERROR,tmp);
      return 102;
    }
    vbr.mask=&mask;
  }

  QImage im;
  if (vbr.RenderSlices())
    return 103;
  if (vbr.SavePNG(outfilename))
    return 103;
  return 0;
}

void
RenderMe::help()
{
  printf("\nVoxBo vbrender (v%s)\n",vbversion.c_str());
  printf("usage:\n");
  printf("  vbrender [flags]\n");
  printf("flags:\n");
  printf("  -a <file>       anatomy file\n");
  printf("  -d <file>       data (stat cube) file\n");
  printf("  -m <file>       mask file\n");
  printf("  -1              one-tailed (no blue)\n");
  printf("  -o <file>       output file for rendering\n");
  printf("  -t <low> <high> below low not rendered, high and above get max value\n");
  printf("notes:\n");
  printf("  if -o is specified, either -a or -d (preferably both) must also be specified\n");
  printf("\n");
}
