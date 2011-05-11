
// vbff.cpp
// file format support code
// Copyright (c) 2000-2010 by The VoxBo Development Team

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
#include <iostream>
#include <fstream>
#include <sstream>
#include "dlfcn.h"
#include <sys/stat.h>
#include <zlib.h>
#include "vbio.h"

// constants, static members, etc.
const int BUFSIZE=16384;
vector<VBFF> VBFF::filetypelist;

// find file format based on signature

VBFF
findFileFormat(const string &signature)
{
  // if we haven't initialized the static filetypelist yet, do it now
  if (VBFF::filetypelist.size()==0)
    VBFF::LoadFileTypes();
  
  for (int i=0; i<(int)VBFF::filetypelist.size(); i++) {
    if (signature==VBFF::filetypelist[i].getSignature()) {
      return VBFF::filetypelist[i];
    }
  }
  VBFF nullff;
  return nullff;
}

// find file format based on filename and dims

VBFF
findFileFormat(const string &fname,int dims)
{
  // if we haven't initialized the static filetypelist yet, do it now
  if (VBFF::filetypelist.size()==0)
    VBFF::LoadFileTypes();
  string ext=xgetextension(fname);
  // if it's gzipped, get the next extension
  if (ext=="gz")
    ext=xgetextension(xsetextension(fname,""));
  // find n-dim filetypes with this extension
  for (int i=0; i<(int)VBFF::filetypelist.size(); i++) {
    if (VBFF::filetypelist[i].extension != ext) continue;
    if (dims==1 && !VBFF::filetypelist[i].write_1D) continue;
    if (dims==2 && !VBFF::filetypelist[i].write_2D) continue;
    if (dims==3 && !VBFF::filetypelist[i].write_3D) continue;
    if (dims==4 && !VBFF::filetypelist[i].write_4D) continue;
    return VBFF::filetypelist[i];
  }
  VBFF nullff;
  return nullff;
}

// this is solely for writing, so we to check for an appropriate
// write_ function

VBFF *
EligibleFileTypesByExtension(string extn,int dims)
{
  // if we haven't initialized the filetypelist yet, do it now
  if (VBFF::filetypelist.size()==0)
    VBFF::LoadFileTypes();

  // find n-dim filetypes with this extension
  for (int i=0; i<(int)VBFF::filetypelist.size(); i++) {
    if (VBFF::filetypelist[i].extension != extn) continue;
    if (dims==1 && !VBFF::filetypelist[i].write_1D) continue;
    if (dims==2 && !VBFF::filetypelist[i].write_2D) continue;
    if (dims==3 && !VBFF::filetypelist[i].write_3D) continue;
    if (dims==4 && !VBFF::filetypelist[i].write_4D) continue;
    return &(VBFF::filetypelist[i]);
  }
  return NULL;
}

vector<VBFF>
EligibleFileTypes(string fname,int dims)
{
  vector<VBFF> types,maybes;
  unsigned char buf[BUFSIZE];
  vf_status tmpstatus;
  int slugsize;

  // if we haven't initialized the filetypelist yet, do it now
  if (VBFF::filetypelist.size()==0)
    VBFF::LoadFileTypes();

  // grab a slug from the file
  gzFile infile=gzopen(fname.c_str(),"r");
  if (infile) {
    slugsize=gzread(infile,buf,BUFSIZE);
    buf[BUFSIZE-1]='\0';
    if (slugsize < BUFSIZE)
      buf[slugsize]='\0';
    gzclose(infile);
  }
  else {        // this is the case for directories
    buf[0]='\0';
    slugsize=0;
  }
  // go through the whole list, calling the typetest function with the slug
  vector<VBFF>::iterator ff;
  for (ff=VBFF::filetypelist.begin(); ff!=VBFF::filetypelist.end(); ff++) {
    // cout << "trying " << ff->name << endl;
    tmpstatus=vf_no;
    // three special cases in case we require support for 1d, 3d, or
    // 4d functionality
    if (dims==1 && ff->test_1D==NULL) continue;
    if (dims==2 && ff->test_2D==NULL) continue;
    if (dims==3 && ff->test_3D==NULL) continue;
    if (dims==4 && ff->test_4D==NULL) continue;
    // run the fileformat's test functions, right now we quit on the
    // first yes
    if (ff->test_3D) {
      tmpstatus=ff->test_3D(buf,slugsize,fname);
      if (tmpstatus==vf_yes) {
        types.push_back(*ff);
        return types;
      }
      else if (tmpstatus==vf_maybe)
        maybes.push_back(*ff);
    }
    else if (ff->test_4D) {
      tmpstatus=ff->test_4D(buf,slugsize,fname);
      if (tmpstatus==vf_yes) {
        types.push_back(*ff);
        return types;
      }
      else if (tmpstatus==vf_maybe)
        maybes.push_back(*ff);
    }
    else if (ff->test_1D) {
      tmpstatus=ff->test_1D(buf,slugsize,fname);
      if (tmpstatus==vf_yes) {
        types.push_back(*ff);
        return types;
      }
      else if (tmpstatus==vf_maybe)
        maybes.push_back(*ff);
    }
    else if (ff->test_2D) {
      tmpstatus=ff->test_2D(buf,slugsize,fname);
      if (tmpstatus==vf_yes) {
        types.push_back(*ff);
        return types;
      }
      else if (tmpstatus==vf_maybe)
        maybes.push_back(*ff);
    }
  }
  // return definite if we have one, otherwise maybes
  if (types.size())
    return types;
  else
    return maybes;
}

int
VBFF::LoadFileTypes()
{
  // FIXME: for the time being, no dynamic ff
  LoadBuiltinFiletypes();
  return 0;

  vector <VBFF> ftlist;
  vglob vg;
  // try /lib
  vg.append("/lib/ff_*.o");
  // try /usr/lib
  vg.append("/usr/lib/ff_*.o");
  // try /usr/local/lib
  vg.append("/usr/local/lib/ff_*.o");
  // try /usr/local/VoxBo/etc/filetypes
  vg.append("/usr/local/VoxBo/etc/fileformats/ff_*.o");
  if (getenv("VOXBO_FILETYPES"))
    vg.append((format("%s/ff_*.o")%getenv("VOXBO_FILETYPES")).str());
  if (getenv("VOXBO_FILEFORMATS"))
    vg.append((format("%s/ff_*.o")%getenv("VOXBO_FILEFORMATS")).str());

  for (size_t i=0; i<vg.size(); i++)
    LoadFiletype(vg[i]);

  // load built-ins last, so that plug-ins get first crack
  LoadBuiltinFiletypes();
  return 0;
}

void
VBFF::printffblurb()
{
  printf("  The format for image output files is determined by the file extension.\n");
  printf("  To force a specific byteorder, append [big] or [little] to the filename\n");
  printf("  (e.g., foo.cub[big]).  VoxBo cub and tes formats and NIfTI 3D and 4D\n");
  printf("  volumes may be stored compressed by tacking .gz onto the filename.\n");
  // printf("  To force...\n");
}

// DYK: the following method is no longer used, but just commented out
// in case we want to bring it back

int
VBFF::LoadFiletype(string)
{
  return 0;
//   void *filehandle;
//   filehandle=dlopen(dlfile.c_str(),RTLD_NOW);
//   // FIXME below appropriate use of error msg?
//   if (!filehandle) {
//     printf("[E] vbff.cpp: error opening %s: %s\n",dlfile.c_str(),dlerror());
//     return 101;
//   }
//   ffinitfn initfn;
//   VBFF tmpff;
//   initfn=(ffinitfn)dlsym(filehandle,"vbff");
//   if (!initfn)
//     return 102;
//   tmpff=initfn(); 
//   tmpff.path=dlfile;
//   install_filetype(tmpff);
//   return (0);
}

void
VBFF::install_filetype(VBFF newff)
{
  if (newff.version_major!=vbversion_major ||
      newff.version_minor!=vbversion_minor)
    return;
  // remove anything with the same signature
  for (int j=0; j<(int)filetypelist.size(); j++) {
    if (filetypelist[j].getSignature()==newff.getSignature()) {
      // FIXME this could be okay -- currently we install plug-ins
      // first so that they get first crack at files. so we shouldn't
      // overwrite earlier installed formats
      return;
      filetypelist.erase(filetypelist.begin()+j);
      break;
    }
  }
  VBFF::filetypelist.push_back(newff);
}

void
VBFF::LoadBuiltinFiletypes()
{
  VBFF tmp;

  // VoxBo types
  VBFF::install_filetype(cub1_vbff());
  VBFF::install_filetype(tes1_vbff());
  VBFF::install_filetype(ref1_vbff());
  VBFF::install_filetype(mat1_vbff());
  VBFF::install_filetype(mtx_vbff());

  // Analyze types
  VBFF::install_filetype(img3d_vbff());
  VBFF::install_filetype(img4d_vbff());
  VBFF::install_filetype(imgdir_vbff());

  // DICOM types
  VBFF::install_filetype(dcm3d_vbff());
  VBFF::install_filetype(dcm4d_vbff());
  // NIfTI types
  VBFF::install_filetype(nifti3d_vbff());
  VBFF::install_filetype(nifti4d_vbff());
  // MRICRO
  VBFF::install_filetype(roi_vbff());
  // GE
  VBFF::install_filetype(ge_vbff());
  // BrainVoyager
  VBFF::install_filetype(vmp3d_vbff());
}

void
VBFF::print() const
{
  cout << "VoxBo File Type \"" << name << "\"" << endl;
  cout << "   sig: " << signature << endl;
  cout << "test3D: " << test_3D << endl;
  cout << "head3D: " << read_head_3D << endl;
  cout << "data3D: " << read_data_3D << endl;
  cout << "test4D: " << test_4D << endl;
  cout << "head4D: " << read_head_4D << endl;
  cout << "data4D: " << read_data_4D << endl;
}

void
VBFF::init()
{
  dimensions=0;
  version_major=version_minor=0;
  f_fastts=0;

  test_1D=(test1Dfun)NULL;
  read_1D=(read1Dfun)NULL;
  write_1D=(write1Dfun)NULL;

  test_2D=(test2Dfun)NULL; 
  read_head_2D=(readhead2Dfun)NULL;
  read_data_2D=(readdata2Dfun)NULL;
  write_2D=(write2Dfun)NULL;

  test_3D=(test3Dfun)NULL;
  read_head_3D=(readhead3Dfun)NULL;
  read_data_3D=(readdata3Dfun)NULL;
  write_3D=(write3Dfun)NULL;

  test_4D=(test4Dfun)NULL;
  read_head_4D=(readhead4Dfun)NULL;
  read_data_4D=(readdata4Dfun)NULL;
  write_4D=(write4Dfun)NULL;
  read_ts_4D=(readts4Dfun)NULL;
  read_vol_4D=(readvol4Dfun)NULL;
  name="NONE";
  signature="NONE";
  extension="NONE";
  path="built-in";
}

VBFF::VBFF()
{
  init();
}

string
VBFF::getName() const
{
  return (string)name;
}

string
VBFF::getSignature() const
{
  return (string)signature;
}

string
VBFF::getPath() const
{
  return path;
}

int
VBFF::getDimensions() const
{
  return dimensions;
}

string
GetHeader(vector<string> headers,string tag)
{
  tokenlist args;
  for (int i=0; i<(int)headers.size(); i++) {
    args.ParseLine(headers[i]);
    if (!args.size()) continue;
    if (args[0][args[0].size()-1]==':')
      args[0].erase(args[0].end()-1);
    if (vb_tolower(args[0])==vb_tolower(tag))
      return (headers[i]);
  }
  string tmp;
  return (tmp);
}

