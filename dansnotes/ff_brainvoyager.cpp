
// ff_brainvoyager.cpp
// VoxBo file I/O code for BrainVoyager(tm) 3D format (version 2?)
// Copyright (c) 2005 by The VoxBo Development Team

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
// based on code by David Brainard

using namespace std;

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vbutil.h"
#include "vbio.h"

extern "C" {

char name[]="BrainVoyager(tm) VMP";
char extension[]="vmp";
char signature[]="vmp";
int dimensions=3;

vf_status
test_vmp_3D(char *buf,int bufsize,string filename)
{
  IMG_header ihead;
  Cube dummy;
  string hdrname;
  // if a .img file, find the corresponding .hdr
  if (filename.size()>4) {
    if (filename.substr(filename.size()-4,4)==".img")
      hdrname=xsetextension(filename,"hdr");
    else
      hdrname=filename;
  }
  if (read_analyze_header(hdrname,ihead,dummy))
    return vf_no;
  return vf_yes;
}

int
read_head_vmp_3D(Cube *cb)
{
  IMG_header ihead;
  string hdrname=xsetextension(cb->GetFileName(),"hdr");
  if (read_analyze_header(hdrname,ihead,*cb))
    return 105;

  // now copy the img header into our structures
  cb->dimx=ihead.dim[1];
  cb->dimy=ihead.dim[2];
  cb->dimz=ihead.dim[3];
  cb->voxsize[0]=ihead.pixdim[1];
  cb->voxsize[1]=ihead.pixdim[2];
  cb->voxsize[2]=ihead.pixdim[3];
  cb->origin[0]=(*((short *)(ihead.origin)))-1;
  cb->origin[1]=(*((short *)(ihead.origin+2)))-1;
  cb->origin[2]=(*((short *)(ihead.origin+4)))-1;

  int ocode=(int)ihead.orient;
  if (ocode==0)
    cb->orient="LPI";
  else if (ocode==1)
    cb->orient="LIP";
  else if (ocode==2)
    cb->orient="AIL";
  else if (ocode==3)
    cb->orient="RPI";
  else if (ocode==4)
    cb->orient="RIP";
  else if (ocode==5)
    cb->orient="AIR";
  else
    cb->orient="XXX";

  if (ihead.datatype==DT_UNSIGNED_CHAR)
    cb->SetDataType(vb_byte);
  else if (ihead.datatype==DT_SIGNED_SHORT)
    cb->SetDataType(vb_short);
  else if (ihead.datatype==DT_SIGNED_INT)
    cb->SetDataType(vb_long);
  else if (ihead.datatype==DT_FLOAT)
    cb->SetDataType(vb_float);
  else if (ihead.datatype==DT_DOUBLE)
    cb->SetDataType(vb_double);
  else
    cb->SetDataType(vb_short);
  cb->header_valid=1;
  return (0);    // no error!
}

int
read_data_vmp_3D(Cube *cb)
{
  string imgname=xsetextension(cb->GetFileName(),"img");
  
  if (cb->dimx<1 || cb->dimy<1 || cb->dimz<1) {
    cb->data_valid=0;  // make sure
    return 105;
  }
  cb->SetVolume(cb->dimx,cb->dimy,cb->dimz,cb->datatype);
  if (!cb->data)
    return 110;

  FILE *fp = fopen(imgname.c_str(),"r");
  if (!fp) {
    delete [] cb->data;
    cb->data=(unsigned char *)NULL;
    cb->data_valid=0;
    return (120);
  }
  int bytelen=cb->dimx*cb->dimy*cb->dimz;
  int cnt=fread(cb->data,cb->datasize,bytelen,fp);
  fclose(fp);
  if (cnt<bytelen) {
    delete [] cb->data;
    cb->data=(unsigned char *)NULL;
    cb->data_valid=0;
    return (130);
  }
  if (my_endian() != cb->filebyteorder)
    cb->byteswap();
  cb->data_valid=1;
  return(0);    // no error!
}



struct BV_VMP_Header1 {
  short version;
  int nmaps;
  int maptype;
  int nlags;
};

struct BV_VMP_Header2 {
  int clustersize;
  unsigned char enableclustercheck;
  float statthresh;
  float statcolorthresh;
  int df1;
  int df2;
  int bonferroni;
  unsigned char critR;
  unsigned char critG;
  unsigned char critB;
  unsigned char maxR;
  unsigned char maxG;
  unsigned char maxB;
  unsigned char enablesmpcolor;
  float transcolorfactor;
};

struct BV_VMP_Header3 {
  int dimx;
  int dimy;
  int dimz;
  int xstart;
  int xend;
  int ystart;
  int yend;
  int zstart;
  int zend;
  int resolutionmm;
};

int
write_vmp_3D(Cube *cb)
{
  string imgname=xsetextension(cb->GetFileName(),"vmp");

  if (!cb->data_valid)       // only write the good stuff
    return (100);
  if (!cb->data)        // should never happen, but what the heck
    return (105);
  FILE *fp=fopen(imgname.c_str(),"w");
  if (!fp)
    return (110);

  BV_VMP_Header1 h1;
  BV_VMP_Header2 h2;
  BV_VMP_Header3 h3;

  h1.version=2;
  h1.nmaps=1;
  h1.maptype=1;
  h1.nlags=0;
  
  h2.clustersize=50;
  h2.enableclustercheck=0;
  h2.statthresh=0;  // FIXME minval
  h2.statcolorthresh=1000;  // FIXME maxval
  h2.df1=157;
  h2.df2=0;
  h2.bonferroni=54228;
  h2.critR=0;
  h2.critG=0;
  h2.critB=100;
  h2.maxR=0;
  h2.maxG=0;
  h2.maxB=255;
  h2.enablesmpcolor=0;
  h2.transcolorfactor=1;

  h3.dimx=cb->dimx;
  h3.dimy=cb->dimy;
  h3.dimz=cb->dimz;
  h3.xstart=0;
  h3.xend=cb->dimx-1;
  h3.ystart=0;
  h3.yend=cb->dimy-1;
  h3.zstart=0;
  h3.zend=cb->dimz-1;
  h3.resolutionmm=cb->voxsize[0];

  // top header
  fwrite(h1,sizeof(struct BV_VMP_Header1),1,fp);
  // header for each image (only one at the moment)
  fwrite(h2,sizeof(struct BV_VMP_Header2),1,fp);
  // the file name
  char fname[cb->GetFileName().size()];
  strcpy(fname,cb->GetFileName().c_str());
  fwrite(fname,strlen(fname)+1,1,fp);
  // the bottom of the header
  fwrite(h3,sizeof(struct BV_VMP_Header3),1,fp);

  // the data
  int cnt=fwrite(cb->data,cb->datasize,bytelen,fp);
  fclose(fp);
  int bytelen=cb->dimx*cb->dimy*cb->dimz;
  if (cnt < bytelen)
    return (120);
  return(0);   // no error!
}

} // extern "C"
