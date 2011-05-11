
// ff_tes.cpp
// VoxBo I/O plug-in for VoxBo Tes format (.tes)
// Copyright (c) 1998-2010 by The VoxBo Development Team

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sstream>
#include <zlib.h>
#include "vbio.h"

using namespace std;
using boost::format;

extern "C" {

vf_status tes1_test(unsigned char *buf,int bufsize,string filename);
int tes1_read_ts(Tes &mytes,int x,int y,int z);
int tes1_read_vol(Tes &ts,Cube &cb,int t);
int tes1_write(Tes *mytes);
int tes1_read_head(Tes *mytes);
int tes1_read_data(Tes *mytes,int start=-1,int count=-1);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF tes1_vbff()
#endif
{
  VBFF tmp;
  tmp.name="VoxBo TES1";
  tmp.extension="tes";
  tmp.signature="tes1";
  tmp.dimensions=4;
  tmp.f_fastts=1;
  tmp.f_headermask=1;
  tmp.version_major=vbversion_major;
  tmp.version_minor=vbversion_minor;
  tmp.test_4D=tes1_test;
  tmp.read_head_4D=tes1_read_head;
  tmp.read_data_4D=tes1_read_data;
  tmp.read_ts_4D=tes1_read_ts;
  tmp.read_vol_4D=tes1_read_vol;
  tmp.write_4D=tes1_write;
  return tmp;
}

vf_status
tes1_test(unsigned char *buf,int bufsize,string)
{
  tokenlist args;
  args.SetSeparator("\n");
  if (bufsize<40)
    return vf_no;
  args.ParseLine((char *)buf);
  if (args[0] != "VB98" || args[1] != "TES1")
    return vf_no;
  return vf_yes;
}

int
tes1_read_ts(Tes &ts,int x,int y,int z)
{
  gzFile fp;
  string keyword;
  tokenlist args;
  int cnt;
  
  if (!ts.header_valid)   // the header includes the mask
    return (100);

  // handle the case of a masked-out voxel
  if (!(ts.GetMaskValue(x,y,z))) {
    ts.timeseries.resize(ts.dimt);
    for (int i=0; i<ts.dimt; i++)
      ts.timeseries.setElement(i,0.0);
    return (0);  // no error!
  }

  fp=gzopen(ts.GetFileName().c_str(),"r");
  if (!fp)
    return (100);

  // skip the header and mask
  gzseek(fp,ts.offset,SEEK_SET);

  // figure out how many time series to skip over
  int maskposition=ts.voxelposition(x,y,z);
  int includedseries=0;
  for (int i=0; i<maskposition; i++) {
    if (ts.mask[i])
      includedseries++;
  }
  gzseek(fp,includedseries*ts.dimt*ts.datasize,SEEK_CUR);

  unsigned char tmpdata[ts.datasize*ts.dimt];
  cnt=gzread(fp,tmpdata,ts.datasize*ts.dimt);
  gzclose(fp);
  if (cnt!=ts.dimt*ts.datasize)
    return 101;

  if (my_endian() != ts.filebyteorder)
    swapn(tmpdata,ts.datasize,ts.dimt);
  ts.timeseries.resize(ts.dimt);
  unsigned char *ptr=tmpdata;
  for (int i=0; i<ts.dimt; i++) {
    ts.timeseries.setElement(i,toDouble(ts.datatype,ptr));
    ptr += ts.datasize;
  }
  if (ts.f_scaled) {
    ts.timeseries*=ts.scl_slope;
    ts.timeseries+=ts.scl_inter;
  }

  return(0);  // no error!
}

int
tes1_read_vol(Tes &ts,Cube &cb,int t)
{
  gzFile fp;
  string keyword;
  tokenlist args;
  int cnt;
  
  // Tes::ReadHeader should already have read the header, which
  // includes the mask
  if (!ts.header_valid)
    return (100);
  if (t<0 || t>ts.dimt-1)
    return 101;

  fp=gzopen(ts.GetFileName().c_str(),"r");
  if (!fp)
    return (100);

  // skip the header and mask and advance to our image position
  gzseek(fp,ts.offset+(t*ts.datasize),SEEK_SET);
  
  cb.SetVolume(ts.dimx,ts.dimy,ts.dimz,ts.datatype);
  if (!cb.data)
    return 102;
  int index=0;
  for (int k=0; k<ts.dimz; k++) {
    for (int j=0; j<ts.dimy; j++) {
      for (int i=0; i<ts.dimx; i++) {
	if (ts.mask[index]) {
	  cnt=gzread(fp,cb.data+(ts.datasize*index),ts.datasize);
	  if (cnt!=ts.datasize) {
	    gzclose(fp);
	    return 103;
	  }
	  gzseek(fp,ts.datasize*(ts.dimt-1),SEEK_CUR);
	}
	index++;
      }
    }
  }
  gzclose(fp);
  if (my_endian() != ts.filebyteorder)
    cb.byteswap();
  if (ts.f_scaled) {
    if (ts.datatype==vb_byte || ts.datatype==vb_short || ts.datatype==vb_long)
      cb.convert_type(vb_float);
    cb*=ts.scl_slope;
    cb+=ts.scl_inter;
  }
  return(0);  // no error!
}

int
tes1_write(Tes *mytes)
{
  string fname=mytes->GetFileName();
  // tmpfname must preserve extension!
  string tmpfname=(format("%s/tmp_%d_%d_%s")%xdirname(fname)%
                   getpid()%time(NULL)%xfilename(fname)).str();
  mytes->Remask();
  string hdr;
  string buf;
  hdr+="VB98\nTES1\n";
  hdr+="DataType: ";
  switch (mytes->f_scaled ? mytes->altdatatype : mytes->datatype) {
  case (vb_byte): hdr+="Byte\n"; break;
  case (vb_short): hdr+="Integer\n"; break;
  case (vb_long): hdr+="Long\n"; break;
  case (vb_float): hdr+="Float\n"; break;
  case (vb_double): hdr+="Double\n"; break;
  default: hdr+="Integer\n"; break;
  }
  buf=(format("VoxDims(TXYZ): %d %d %d %d\n")%mytes->dimt%mytes->dimx%mytes->dimy%mytes->dimz).str();
  hdr+=buf;
  if ((mytes->voxsize[0] + mytes->voxsize[1] + mytes->voxsize[2]) > 0.0) {
    buf=(format("VoxSizes(XYZ): %.4f %.4f %.4f\n")%mytes->voxsize[0]%mytes->voxsize[1]%mytes->voxsize[2]).str();
    hdr+=buf;
  }
  buf=(format("TR(msecs): %.4f\n")%mytes->voxsize[3]).str();
  hdr+=buf;
  if ((mytes->origin[0] + mytes->origin[1] + mytes->origin[2]) > 0) { // FIXME could be bad
    buf=(format("Origin(XYZ): %d %d %d\n")%mytes->origin[0]%mytes->origin[1]%mytes->origin[2]).str();
    hdr+=buf;
  }
  // force big-endian
  mytes->filebyteorder=ENDIAN_BIG;
  if (mytes->filebyteorder==ENDIAN_BIG)
    hdr+="Byteorder: msbfirst\n";
  else
    hdr+="Byteorder: lsbfirst\n";
  hdr+="Orientation: "+mytes->orient+"\n";
  if (mytes->f_scaled) {
    hdr+="scl_slope: "+strnum(mytes->scl_slope)+"\n";
    hdr+="scl_inter: "+strnum(mytes->scl_inter)+"\n";
  }
  for (int i=0; i<(int)mytes->header.size(); i++)
    hdr+=mytes->header[i]+"\n";
  hdr+="\x0c\n";

  zfile zfp;
  zfp.open(tmpfname,"w");
  if (!zfp)
    return 101;
  zfp.write(hdr.c_str(),hdr.size());
  // write the mask
  zfp.write(mytes->mask,mytes->dimx*mytes->dimy*mytes->dimz);
  // un-swap and un-scale if needed
  if (mytes->f_scaled) {
    *mytes-=mytes->scl_inter;
    *mytes/=mytes->scl_slope;
    if (mytes->altdatatype==vb_byte || mytes->altdatatype==vb_short || mytes->altdatatype==vb_long)
      mytes->convert_type(mytes->altdatatype);
  }
  if (my_endian() != mytes->filebyteorder)
    mytes->byteswap();
  int sz,cnt;
  for (int i=0; i<mytes->dimx*mytes->dimy*mytes->dimz; i++) {
    if (mytes->mask[i] == 0)
      continue;
    sz=mytes->datasize*mytes->dimt;
    cnt=zfp.write(mytes->data[i],sz);
    if (cnt !=sz) {
      zfp.close_and_unlink();
      return(102);
    }
  }
  if (my_endian() != mytes->filebyteorder)            // swap it back
    mytes->byteswap();
  // re-scale and re-swap if needed
  if (mytes->f_scaled) {
    if (mytes->datatype==vb_byte || mytes->datatype==vb_short || mytes->datatype==vb_long)
      mytes->convert_type(vb_float);
    *mytes*=mytes->scl_slope;
    *mytes+=mytes->scl_inter;
  }
  zfp.close();
  if (rename(tmpfname.c_str(),fname.c_str()))
    return (103);
  return (0);  // no error!
}




int
tes1_read_head(Tes *mytes)
{
  gzFile fp;
  string keyword;
  char line[STRINGLEN];
  tokenlist args;
  
  mytes->header_valid=0;
  fp=gzopen(mytes->GetFileName().c_str(),"r");
  if (!fp) {
    return (100);
  }
  mytes->header.clear();
  if (gzread(fp,line,10) != 10) {
    gzclose(fp);
    return(100);
  }
  if (strncmp(line,"VB98\nTES1\n",10)) {
    gzclose(fp);
    return(100);
  }
  while (gzgets(fp,line,STRINGLEN)) {
    if (line[0] == 12)
      break;
    stripchars(line,"\n");
    args.ParseLine(line);
    keyword=args[0];
    // discard trailing colons
    if (keyword[keyword.size()-1]==':')
      keyword.replace(keyword.size()-1,1,"");
    // parse known headers
    if (equali(keyword,"voxdims(txyz)") && args.size() >4) {
      mytes->dimt=strtol(args[1]);
      mytes->dimx=strtol(args[2]);
      mytes->dimy=strtol(args[3]);
      mytes->dimz=strtol(args[4]);
      continue;
    }
    if (equali(keyword,"datatype") && args.size() >1) {
      parsedatatype(args[1],mytes->datatype,mytes->datasize);
      continue;
    }
    if (equali(keyword,"voxsizes(xyz)") && args.size() >3) {
      mytes->voxsize[0]=strtod(args[1]);
      mytes->voxsize[1]=strtod(args[2]);
      mytes->voxsize[2]=strtod(args[3]);
      continue;
    }
    if (equali(keyword,"tr(msecs)") && args.size() >1) {
      mytes->voxsize[3]=strtod(args[1]);
      continue;
    }
    if (equali(keyword,"origin(xyz)") && args.size() >3) {
      mytes->origin[0]=strtol(args[1]);
      mytes->origin[1]=strtol(args[2]);
      mytes->origin[2]=strtol(args[3]);
      continue;
    }
    if (equali(keyword,"byteorder") && args.size() >1) {
      if (equali(args[1],"msbfirst"))
	mytes->filebyteorder=ENDIAN_BIG;
      else if (equali(args[1],"lsbfirst"))
	mytes->filebyteorder=ENDIAN_LITTLE;
      continue;
    }
    if (equali(keyword,"orientation") && args.size() >1) {
      mytes->orient=args[1];
      continue;
    }
    if (equali(keyword,"scl_slope") && args.size()>1) {
      mytes->scl_slope=strtod(args[1]);
      continue;
    }
    if (equali(keyword,"scl_inter") && args.size()>1) {
      mytes->scl_inter=strtod(args[1]);
      continue;
    }
    mytes->AddHeader(line);
  }
  if (mytes->dimt == 0 || mytes->dimx == 0 || mytes->dimy == 0 || mytes->dimz == 0) {
    gzclose(fp);
    return (100);
  }
  // it's scaled if scl_slope is neither 0 nor 1 (first line)
  // or scl_slope is 1 and scl_inter is nonzero (second line)
  if ((fabs(mytes->scl_slope)>FLT_MIN && fabs(mytes->scl_slope-1.0)>FLT_MIN)
      || (fabs(mytes->scl_slope-1.0)<FLT_MIN && fabs(mytes->scl_inter)>FLT_MIN)) {
    mytes->f_scaled=1;
    mytes->altdatatype=mytes->datatype;
  }
  
  // FIXME check for unusually large dims?

  if (mytes->datasize==0) {  // error with datatype
    gzclose(fp);
    return(100);
  }

  // clear/initialize the volume
  mytes->SetVolume(mytes->dimx,mytes->dimy,mytes->dimz,mytes->dimt,mytes->datatype);
  // read the mask
  if (mytes->InitMask(0)) {
    gzclose(fp);
    return 110;
  }
  
  int cnt=gzread(fp,mytes->mask,mytes->voxels);
  if (cnt < mytes->voxels) {
    gzclose(fp);
    return (100);
  }
  mytes->maskcount();
  mytes->offset=gztell(fp);
  gzclose(fp);

  mytes->header_valid=1;
  return(0);  // no error!
}

int
tes1_read_data(Tes *mytes,int start,int count)
{
  gzFile fp;
  string keyword;
  tokenlist args;
  int cnt;
  
  if (!mytes->header_valid)
    return 101;
  if (mytes->InitData())
    return 102;

  fp=gzopen(mytes->GetFileName().c_str(),"r");
  if (!fp)
    return (102);

  // honor volume range
  if (start==-1) {
    start=0;
    count=mytes->dimt;
  }
  else if (start+count>mytes->dimt)
    return 220;
  int endskip=mytes->dimt-(start+count);
  mytes->dimt=count;

  // seek to the beginning of the data -- note that header_valid
  // implies the mask is correct and the data array exists
  gzseek(fp,mytes->offset,SEEK_SET);
  
  mytes->realvoxels=0;
  for (int i=0; i<mytes->dimx*mytes->dimy*mytes->dimz; i++) {
    if (mytes->mask[i] == 0)
      continue;
    mytes->buildvoxel(i);    // make sure memory is allocated for that voxel 
    // skip omitted initial volumes
    if (start>0)
      gzseek(fp,start*mytes->datasize,SEEK_CUR);
    // read time series data
    cnt=gzread(fp,mytes->data[i],mytes->datasize*mytes->dimt);
    if (cnt != mytes->datasize*mytes->dimt) {
      mytes->data_valid=0;
      break;
    }
    // skip omitted end volumes
    if (endskip>0)
      gzseek(fp,endskip*mytes->datasize,SEEK_CUR);
  }
  gzclose(fp);
  if (my_endian() != mytes->filebyteorder)
    mytes->byteswap();
  if (mytes->f_scaled) {
    if (mytes->datatype==vb_byte || mytes->datatype==vb_short || mytes->datatype==vb_long)
      mytes->convert_type(vb_float);
    *mytes*=mytes->scl_slope;
    *mytes+=mytes->scl_inter;
  }
  mytes->data_valid=1;

  return(0);  // no error!
}

} // extern "C"
