
// afni.cpp
// VoxBo file I/O code for AFNI brik/head format
// Copyright (c) 1998-2001 by The VoxBo Development Team

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
// <kimberg@mail.med.upenn.edu>.

// based on documentation of the AFNI file formats by Bob Cox,
// downloaded from http://FIXME

using namespace std;

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "vbutil.h"
#include "vbio.h"

int
Cube::ReadBRIK(const string &fname)
{
  int pos;
  string stem=fname;

  pos = stem.find(".BRIK");
  if (pos == (int)string::npos)
    pos = stem.find(".HEAD");
  if (pos == (int)string::npos)
    return (101);
  stem.erase(pos,5);
  if (ReadBRIKHeader(stem+".HEAD"))
    return (102);
  if (ReadBRIKData(stem+".BRIK"))
    return (103);
  return (0);  // no error!
}

int
Tes::ReadBRIK(const string &fname)
{
  int pos;
  string stem=fname;

  pos = stem.find(".BRIK");
  if (pos == (int)string::npos)
    pos = stem.find(".HEAD");
  if (pos == (int)string::npos)
    return (101);
  stem.erase(pos,5);
  if (ReadBRIKHeader(stem+".HEAD"))
    return (102);
  if (ReadBRIKData(stem+".BRIK"))
    return (103);
  return (0);  // no error!
}

int
Cube::ReadBRIKData(const string &fname)
{
  FILE *fp=fopen(fname.c_str(),"r");
  if (!fp)
    return (100);

  filename=fname;
  data=new unsigned char[datasize*dimx*dimy*dimz];
  if (!data)
    return (100);
  int cnt=fread(data,datasize,dimx*dimy*dimz,fp);
  if (filebyteorder!=my_endian())
    swapn((unsigned char *)data,datasize,dimx*dimy*dimz);
  fclose(fp);
  if (cnt!=dimx*dimy*dimz)  // that's how many things of size datasize we should have
    return (101);
  data_valid=1;
  return (0);  // no error!
}

int
Tes::ReadBRIKData(const string &fname)
{
  FILE *fp;
  int i,j,k,t,idx;
  
  filename=fname;
  fp = fopen(fname.c_str(),"r");
  if (!fp)
    return (100);
  SetVolume(dimx,dimy,dimz,dimt,datatype);
  unsigned char *buf=new unsigned char[datasize*dimx*dimy*dimz];
  if (!buf)
    return (101);
  for (i=0; i<dimx*dimy*dimz; i++)
    buildvoxel(i);
  
  for (t=0; t<dimt; t++) {
    // read a whole cube
    if (fread(buf,datasize,dimx*dimy*dimz,fp) < (unsigned int)datasize) {
      fclose(fp);
      return (102);
    }
    if (filebyteorder!=my_endian())
      swapn(buf,datasize,dimx*dimy*dimz);
    // sort it into the tes structure
    for (i=0; i<dimx; i++) {
      for (j=0; j<dimy; j++) {
	for (k=0; k<dimz; k++) {
	  idx=voxelposition(i,j,k);
	  memcpy(data[idx]+(t*datasize),buf+(idx*datasize),datasize);
	}
      }
    }
  }
  data_valid=1;
  fclose(fp);
  return (0);  // no error!
}
  
int
VBImage::ReadBRIKHeader(const string &fname)
{
  FILE *fp;
  char atype[STRINGLEN],aname[STRINGLEN],tmp[STRINGLEN];
  int acount;
  tokenlist args;
  int cnt,i;
  vector<float> vfloat;
  vector<int> vint;
  string vstring,sname,stype;
  int tmpint;
  float tmpfloat;
  
  filename=fname;
  fp = fopen(fname.c_str(),"r");
  if (!fp)
    return (100);

  while (TRUE) {
    cnt=fscanf(fp," type = %s name = %s count = %d",
	       atype,aname,&acount);
    sname=aname;
    stype=atype;

    if (cnt < 3)
      break;

    // grab the actual data
    if (stype=="integer-attribute") {
      vint.clear();
      for (i=0; i<acount; i++) {
	fscanf(fp," %d",&tmpint);
	vint.push_back(tmpint);
      }
    }
    else if (stype=="float-attribute") {
      vfloat.clear();
      for (i=0; i<acount; i++)
	fscanf(fp," %f",&tmpfloat);
      vfloat.push_back(tmpfloat);
    }
    else if (stype=="string-attribute") {
      fscanf(fp,"'");
      cnt=fread(tmp,1,acount-1,fp);
      tmp[cnt]='\0';
      vstring=tmp;
      fread(tmp,1,3,fp);
    }

    // not handled but maybe should be: TYPESTRING, SCENE_DATA, DELTA,
    // TAXIS STUFF IDCODE_STRING, IDCODE_DATE, BRICK_STATS,
    // BRICK_FLOAT_FACS, BRICK_LABS, BRICK_STATAUX, STAT_AUX

    // now handle the ones we recognize
    if (sname=="DATASET_RANK" && stype=="integer-attribute" && acount>=2) {
      dimt=vint[1];
    }
    else if (sname=="DATASET_DIMENSIONS" && stype=="integer-attribute" && acount>=3) {
      dimx=vint[0];
      dimy=vint[1];
      dimz=vint[2];
    }
    else if (sname=="ORIENT_SPECIFIC" && stype=="integer-attribute" && acount==3) {
      // FIXME handle the orientation codes
    }
    else if (sname=="ORIGIN" && stype=="float-attribute" && acount==3) {
      // FIXME would be nice to have double origins
      origin[0]=(int)round(vfloat[0]);
      origin[1]=(int)round(vfloat[1]);
      origin[2]=(int)round(vfloat[2]);
    }
    else if (sname=="BYTEORDER_STRING" && stype=="string-attribute" && acount==1) {
      if (vstring=="LSB_FIRST")
	filebyteorder=ENDIAN_LITTLE;
      else if (vstring=="MSB_FIRST")
	filebyteorder=ENDIAN_BIG;
    }
    else if (sname=="BRICK_TYPES" && stype=="integer-attribute" && acount==dimt) {
      // just use the first one, they're going to be all the same for us
      if (vint[0] == 0) datatype=vb_byte,datasize=1;
      if (vint[0] == 1) datatype=vb_short,datasize=2;
      if (vint[0] == 2) datatype=vb_long,datasize=4;
      if (vint[0] == 3) datatype=vb_float,datasize=4;
      if (vint[0] == 4) datatype=vb_double,datasize=8;
      if (vint[0] == 5) datatype=vb_byte,datasize=1;
    }
    else if (sname=="xxx" && stype=="xxx" && acount==333) {
    }
  }
  fclose(fp);
  if (dimt>1)
    filetype=t_brik4d;
  else
    filetype=t_brik3d;

  return (0);    // no error!
}

//  int
//  Tes::WriteBRIKHEAD()
//  {
//    string imgname,hdrname;
//    int pos,cnt;

//    if (!valid)       // only write the good stuff
//      return (100);
//    if (!data)        // should never happen, but what the heck
//      return (100);
//    imgname = filename;
//    pos = imgname.find(".brik");
//    if (pos == (int)string::npos)
//      imgname = imgname + ".brik";
//    hdrname = imgname;
//    hdrname.replace(hdrname.find(".brik"),4,".head");

//    return(0);   // no error!
//  }
