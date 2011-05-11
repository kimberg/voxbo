
// vbvec2hdr.cpp
// copy information from a vec to an image header
// Copyright (c) 2010 by The VoxBo Development Team

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

#include "vbutil.h"
#include "vbio.h"
//#include "vbprefs.h"
#include "vbvec2hdr.hlp.h"

int newheader(string tag,string file,string &result);
void vbvec2hdr_help();
void vbvec2hdr_version();

int
main(int argc,char *argv[])
{
  if (argc==1) {
    vbvec2hdr_help();
    exit(106);
  }
  tokenlist args;
  vector<string> newheaders;
  string outfile;
  args.Transfer(argc-1,argv+1);
  
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-o" && i<args.size()-1) {
      outfile=args[i+1];
      i+=1;
    }
    else if (args[i]=="-t" && i<args.size()-2) {
      string newhdr;
      if (newheader(args[i+1],args[i+2],newhdr)) {
        printf("[E] vbvec2hdr: problem adding header for tag %s\n",args(i+1));
        exit(102);
      }
      newheaders.push_back(newhdr);
      i+=2;
    }
    else if (args[i]=="-v") {
      vbvec2hdr_version();
      exit(0);
    }
    else if (args[i]=="-h") {
      vbvec2hdr_help();
      exit(0);
    }
    else {
      printf("[E] vbvec2hdr: unrecognized flag %s\n",args(i));
      exit(100);
    }
  }
  if (!(outfile.size())) {
    printf("[E] vbvec2hdr: no output file specified\n");
    exit(100);
  }
  if (!(newheaders.size())) {
    printf("[E] vbvec2hdr: no new headers specified\n");
    exit(100);
  }
  Cube cb;
  Tes ts;
  if (!cb.ReadFile(outfile)) {
    for (int i=0; i<(int)newheaders.size(); i++)
      cb.AddHeader(newheaders[i]);
    if (cb.WriteFile()) {
      printf("[E] vbvec2hdr: couldn't write 3D file\n");
      exit(200);
    }
  }
  else if (!ts.ReadFile(outfile)) {
    for (int i=0; i<(int)newheaders.size(); i++)
      ts.AddHeader(newheaders[i]);
    if (ts.WriteFile()) {
      printf("[E] vbvec2hdr: couldn't write 4D file\n");
      exit(204);
    }
  }
  else {
    printf("[E] vbvec2hdr: couldn't read file\n");
    exit(205);
  }
  printf("[I] vbvec2hdr: done adding %d headers\n",(int)newheaders.size());
  exit(0);
}

int
newheader(string tag,string file,string &result)
{
  VB_Vector foo;
  if (foo.ReadFile(file))
    return 101;
  result=tag+" ";
  char tmp[STRINGLEN];
  for (uint32 i=0; i<foo.size(); i++) {
    sprintf(tmp,"%f ",foo[i]);
    result+=tmp;
  }
  return 0;
}

void
vbvec2hdr_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbvec2hdr_version()
{
  printf("VoxBo vbvec2hdr (v%s)\n",vbversion.c_str());
}
