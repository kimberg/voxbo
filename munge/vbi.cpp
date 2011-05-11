
// vbi.cpp
// hack to print out information about various file types
// Copyright (c) 1998-2011 by The VoxBo Development Team

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

#include "vbutil.h"
#include "vbio.h"
#include "vbversion.h"
#include <iostream>
#include <sstream>
#include "vbi.hlp.h"

extern "C" {
#include "analyze.h"
#include "nifti.h"
#include "dicom.h"
}

void vbi_help();
void vbi_version();

int
main(int argc,char *argv[])
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  if (args.size()==0) {
    vbi_help();
    exit(0);
  }
  bool f_long=0;
  bool f_loaddata=0;
  bool f_fullheader=0;
  bool f_quiet=0;
  string flagstr;
  vector<string> filelist;
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-d")
      flagstr+="d";
    else if (args[i]=="-t")
      flagstr+="t";
    else if (args[i]=="-z")
      flagstr+="v";
    else if (args[i]=="-o")
      flagstr+="o";
    else if (args[i]=="-r")
      flagstr+="r";
    else if (args[i]=="-q")
      f_quiet=1;
    else if (args[i]=="-a")
      f_loaddata=1;
    else if (args[i]=="-l")
      f_long=1;
    else if (args[i]=="-f")
      f_fullheader=1;
    else if (args[i]=="-v" || args[i]=="--version") {
      vbi_version();
      exit(0);
    }
    else if (args[i]=="-h" || args[i]=="--help") {
      vbi_help();
      exit(0);
    }
    else
      filelist.push_back(args[i]);
  }

  // VB_filetype type;
  Tes mytes;
  Cube mycube;
  VBMatrix mymat;
  VB_Vector myvec;
  vector<VBFF>formats;
  VBFF fileformat;

  for (size_t i=0; i<filelist.size(); i++) {
    if (mytes.ReadHeader(filelist[i])==0) {
      if (f_loaddata)
        mytes.ReadFile(filelist[i]);
      if (f_long)
        mytes.print();
      else {
        printf("[I] ");
        mytes.printbrief(flagstr);
      }
    }
    else if (mycube.ReadHeader(filelist[i])==0) {
      if (f_loaddata)
        mycube.ReadFile(filelist[i]);
      if (f_long)
        mycube.print();
      else {
        printf("[I] ");
        mycube.printbrief(flagstr);
      }
    }
    else if (myvec.ReadFile(filelist[i])==0) {
      cout << format("[I] %s: %d elements, mean of %g, variance of %g\n")%
        filelist[i]%myvec.size()%myvec.getVectorMean()%myvec.getVariance();
    }
    else if (mymat.ReadFile(filelist[i])==0) {
      if (f_long) {
        mymat.printinfo();
        mymat.printColumnCorrelations();
      }
      else
        mymat.printinfo();
    }
    else {
      if (!f_quiet)
        cout << "[E] vbi: couldn't read file " << filelist[i] << endl;
    }
    IMG_header ihead;
    NIFTI_header nhead;
    dicominfo dci;
    if (f_fullheader && vb_fileexists(filelist[i])) {
      if (nifti_read_header(filelist[i],&nhead,NULL)==0)
        print_nifti_header(nhead);
      else if (analyze_read_header(args[i],&ihead,NULL)==0)
        print_analyze_header(ihead);
      else if (read_dicom_header(filelist[i],dci)==0)
        print_dicom_header(filelist[i]);
      return vf_no;
    }
  }
  exit(0);
}

void
vbi_help()
{
  cout << boost::format(myhelp) % vbversion;
}

void
vbi_version()
{
  printf("VoxBo vbi (v%s)\n",vbversion.c_str());
}
