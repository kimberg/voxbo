
// nifti.cpp
// VoxBo I/O support for NIFTI-1.1 format
// Copyright (c) 2005-2010 by The VoxBo Development Team

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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sstream>
#include <zlib.h>
#include "vbutil.h"
#include "vbio.h"

#define NIFTI_MIN_OFFSET_BOGUS 348

extern "C" {

#include "nifti.h"

// read_nifti_header() assumes the input is an nii, hdr, or img file.
// if it's an image file, it tries to read the corresponding hdr.

int
nifti_read_header(string fname,NIFTI_header *xhdr,VBImage *vol)
{
  if (xhdr==NULL && vol==NULL)
    return 200;
  // nifti_header and vbimage header point to either local or passed
  // storage
  NIFTI_header hh,*hdr=xhdr;
  if (hdr==NULL) hdr=&hh;
  memset(hdr,0,sizeof(NIFTI_header));  // just in case
  // munge the header file name
  // string ff=fname;
  if (xgetextension(fname)=="img")
    fname=xsetextension(fname,"hdr");
  // read the data
  VB_byteorder bo=my_endian();
  gzFile fp;
  bool f_swap=0;
  if (!(fp = gzopen(fname.c_str(),"r")))
    return (100);
  int cnt = gzread(fp,hdr,sizeof(NIFTI_header));
  if (cnt!=sizeof(NIFTI_header)) {
    gzclose(fp);
    return (101);
  }
  // byte swap if needed
  if (hdr->dim[0]<1 || hdr->dim[0]>7)
    f_swap=1;
  if (f_swap) {
    if (bo==ENDIAN_BIG) bo=ENDIAN_LITTLE;
    else bo=ENDIAN_BIG;
    nifti_swap_header(*hdr);
  }
  // check magic bytes for sanity
  if (hdr->magic[3]!='\0') {
    gzclose(fp);
    return 119;
  }
  if ((string)hdr->magic != "ni1"
      && (string)hdr->magic != "n+1") {
    gzclose(fp);
    return 102;
  }

  // see if we have an extension
  struct {uint32 esize,ecode;} n11ext;
  if (vol && gztell(fp)==348 && hdr->vox_offset>=352) {
    uint8 exts[4];
    cnt=gzread(fp,exts,4);
    if (cnt==4 && exts[0]!=0) {
      while(gzread(fp,&n11ext,8)==8) {
        // swap if needed
        if (f_swap)
          swap(&n11ext.esize),swap(&n11ext.ecode);
        // is it voxbo ecode?
        if (n11ext.esize==0 && n11ext.ecode==0) break; // test for sentinel
        if (n11ext.ecode!=NIFTI_ECODE_VOXBO) {
          gzseek(fp,n11ext.esize,SEEK_CUR);
          continue;
        }
        // copy to voxbo header
        char buf[n11ext.esize+1];
        if (gzread(fp,buf,n11ext.esize)!=(int)n11ext.esize) {
          gzclose(fp);
          return 104;
        }
        buf[n11ext.esize]='\0';
        tokenlist tmpt;
        tmpt.SetSeparator("\n\r");
        tmpt.ParseLine(buf);
        for (size_t i=0; i<tmpt.size(); i++)
          vol->header.push_back(tmpt[i]);
      }
    }
  }

  gzclose(fp);

  // if we're not putting it into a VBImage, we can return now
  if (vol==NULL)
    return 0;

  // now transfer stuff to cube

  vol->filebyteorder=bo;

  // crash and burn on some immediate problems we can't handle
  if (hdr->sizeof_hdr != 348)
    return 106;
  if (hdr->dim[0]<2 || hdr->dim[0]>4)
    return 107;
  // FIXME ignoring sform_code
  // if (hdr->sform_code>0)
  //   return 110;

  // check and set datatype, set datasize manually
  if (hdr->datatype==DT_UINT8 || hdr->datatype==DT_INT8) {
    vol->datatype=vb_byte;
    vol->datasize=1;
  }
  else if (hdr->datatype==DT_INT16 || hdr->datatype==DT_UINT16) {
    vol->datatype=vb_short;
    vol->datasize=2;
  }
  else if (hdr->datatype==DT_INT32 || hdr->datatype==DT_UINT32) {
    vol->datatype=vb_long;
    vol->datasize=4;
  }
  else if (hdr->datatype==DT_FLOAT32) {
    vol->datatype=vb_float;
    vol->datasize=4;
  }
  else if (hdr->datatype==DT_FLOAT64) {
    vol->datatype=vb_double;
    vol->datasize=8;
  }
  else
    return 105;
  vol->scl_slope=hdr->scl_slope;
  vol->scl_inter=hdr->scl_inter;
  if (fabs(vol->scl_slope)>FLT_MIN) {
    vol->f_scaled=1;
    vol->altdatatype=vol->datatype;
  }
  vol->dimx=hdr->dim[1];
  vol->dimy=hdr->dim[2];
  vol->dimz=hdr->dim[3];
  vol->dimt=hdr->dim[4];
  vol->voxsize[0]=hdr->pixdim[1];
  vol->voxsize[1]=hdr->pixdim[2];
  vol->voxsize[2]=hdr->pixdim[3];
  vol->voxsize[3]=hdr->pixdim[4];
  // make sure spatial dims are in mm
  if (XYZT_TO_SPACE(hdr->xyzt_units)==NIFTI_UNITS_METER) {
    vol->voxsize[0]/=1000;
    vol->voxsize[1]/=1000;
    vol->voxsize[2]/=1000;
  }
  else if (XYZT_TO_SPACE(hdr->xyzt_units)==NIFTI_UNITS_MICRON) {
    vol->voxsize[0]*=1000;
    vol->voxsize[1]*=1000;
    vol->voxsize[2]*=1000;
  }
  // make sure time dim is in msecs
  if (XYZT_TO_TIME(hdr->xyzt_units)==NIFTI_UNITS_SEC)
    vol->voxsize[3]*=1000;
  else if (XYZT_TO_TIME(hdr->xyzt_units)==NIFTI_UNITS_USEC)
    vol->voxsize[3]/=1000;

  // convert nifti's complicated orientation coding scheme
  vol->orient="RPI";
  vol->qoffset[0]=hdr->qoffset_x;
  vol->qoffset[1]=hdr->qoffset_y;
  vol->qoffset[2]=hdr->qoffset_z;
  vol->origin[0]=(int)hdr->qoffset_x;
  vol->origin[1]=(int)hdr->qoffset_y;
  vol->origin[2]=(int)hdr->qoffset_z;
  vol->qform_code=hdr->qform_code;
  vol->sform_code=hdr->sform_code;
  vol->quatern_b=hdr->quatern_b;
  vol->quatern_c=hdr->quatern_c;
  vol->quatern_d=hdr->quatern_d;
  vol->srow_x[0]=hdr->srow_x[0];
  vol->srow_x[1]=hdr->srow_x[1];
  vol->srow_x[2]=hdr->srow_x[2];
  vol->srow_x[3]=hdr->srow_x[3];
  vol->srow_y[0]=hdr->srow_y[0];
  vol->srow_y[1]=hdr->srow_y[1];
  vol->srow_y[2]=hdr->srow_y[2];
  vol->srow_y[3]=hdr->srow_y[3];
  vol->srow_z[0]=hdr->srow_z[0];
  vol->srow_z[1]=hdr->srow_z[1];
  vol->srow_z[2]=hdr->srow_z[2];
  vol->srow_z[3]=hdr->srow_z[3];
  // FIXME if qform!=0, use srowxyz[3] somehow, but this isn't exactly right
  if (vol->qform_code) {
    vol->origin[0]=vol->srow_x[3];
    vol->origin[1]=vol->srow_y[3];
    vol->origin[2]=vol->srow_z[3];
  }

  // other nifti fields we're going to preserve but not play with
  vol->AddHeader((string)"nifti_dim_info "+hdr->dim_info);
  vol->AddHeader((string)"nifti_intent_p1 "+strnum(hdr->intent_p1));
  vol->AddHeader((string)"nifti_intent_p2 "+strnum(hdr->intent_p2));
  vol->AddHeader((string)"nifti_intent_p3 "+strnum(hdr->intent_p3));
  vol->AddHeader((string)"nifti_intent_code "+strnum(hdr->intent_code));
  vol->AddHeader((string)"nifti_pixdim0 "+strnum(hdr->pixdim[0]));
  vol->AddHeader((string)"nifti_slice_end "+strnum(hdr->slice_end));
  vol->AddHeader((string)"nifti_slice_code "+hdr->slice_code);
  vol->AddHeader((string)"nifti_cal_max "+strnum(hdr->cal_max));
  vol->AddHeader((string)"nifti_cal_min "+strnum(hdr->cal_min));
  vol->AddHeader((string)"nifti_slice_duration "+strnum(hdr->slice_duration));
  vol->AddHeader((string)"nifti_toffset "+strnum(hdr->toffset));

  vol->offset=(int)hdr->vox_offset;
  if (vol->offset<NIFTI_MIN_OFFSET_BOGUS) vol->offset=NIFTI_MIN_OFFSET_BOGUS;
  vol->header_valid=1;
  return 0;
}

string
nifti_typestring(int16 dt)
{
  switch (dt) {
  case DT_UINT8:
    return "uint8";
  case DT_INT16:
    return "int16";
  case DT_INT32:
    return "int32";
  case DT_FLOAT32:
    return "float32";
  case DT_COMPLEX64:
    return "complex64";
  case DT_FLOAT64:
    return "float64";
  case DT_RGB24:
    return "rgb24";
  case DT_INT8:
    return "int8";
  case DT_UINT16:
    return "uint16";
  case DT_UINT32:
    return "uint32";
  case DT_INT64:
    return "int64";
  case DT_UINT64:
    return "uint64";
  case DT_FLOAT128:
    return "float128";
  case DT_COMPLEX128:
    return "complex128";
  case DT_COMPLEX256:
    return "complex256";
  default:
    return "<notype>";
  }
}

void
print_nifti_header(NIFTI_header &ihead)
{
  uint16 nspace=XYZT_TO_SPACE(ihead.xyzt_units);
  uint16 ntime=XYZT_TO_TIME(ihead.xyzt_units);
  string spaceunits,timeunits;
  if (nspace==NIFTI_UNITS_METER) spaceunits="m";
  if (nspace==NIFTI_UNITS_MM) spaceunits="mm";
  if (nspace==NIFTI_UNITS_MICRON) spaceunits="microns";
  if (ntime==NIFTI_UNITS_SEC) timeunits="s";
  if (ntime==NIFTI_UNITS_MSEC) timeunits="ms";
  if (ntime==NIFTI_UNITS_USEC) timeunits="us";
  if (ntime==NIFTI_UNITS_HZ) timeunits="Hz";
  if (ntime==NIFTI_UNITS_PPM) timeunits="ppm";
  if (ntime==NIFTI_UNITS_RADS) timeunits="rads";

  cout << format("          sizeof_hdr: %d\n") % ihead.sizeof_hdr;
  cout << format("            dim_info: '%c'\n") % ihead.dim_info;
  cout << format("                dims: %d %d %d %d %d %d %d %d\n") % ihead.dim[0]
    % ihead.dim[1] % ihead.dim[2] % ihead.dim[3] % ihead.dim[4] % ihead.dim[5]
    % ihead.dim[6] % ihead.dim[7];
  cout << format("         intent code: %d (params %g %g %g)\n")%ihead.intent_code
    % ihead.intent_p1 % ihead.intent_p2 % ihead.intent_p3;
  cout << format("         intent_name: %.16s\n") % ihead.intent_name;
  cout << format("            datatype: %d (%s)\n") % ihead.datatype % nifti_typestring(ihead.datatype);
  cout << format("              bitpix: %d\n") % ihead.bitpix;
  cout << format("         slice_start: %d\n") % ihead.slice_start;
  cout << format("           pixdim[0]: %g\n") % ihead.pixdim[0];

  string tmps=(format("voxel sizes (%s)")%spaceunits).str();
  cout << format("%20s: %g %g %g\n")
    % tmps % ihead.pixdim[1] % ihead.pixdim[2] % ihead.pixdim[3];
  
  cout << format("                  TR: %g%s\n") % ihead.pixdim[4] % timeunits;

  cout << format("         pixdim[5-7]: %g %g %g\n")
    % ihead.pixdim[5] % ihead.pixdim[6] % ihead.pixdim[7];

  cout << format("               units: %d\n") % (uint16)ihead.xyzt_units;
  cout << format("          vox offset: %g\n") % ihead.vox_offset;
  cout << format("         scale slope: %g\n") % ihead.scl_slope;
  cout << format("     scale intercept: %g\n") % ihead.scl_inter;
  cout << format("           slice_end: %d\n") % ihead.slice_end;
  cout << format("         slice order: '%c'\n") % ihead.slice_code;
  cout << format("         cal_min/max: %g/%g\n") % ihead.cal_min % ihead.cal_max;
  cout << format("      slice duration: %g\n") % ihead.slice_duration;
  cout << format("             toffset: %g\n") % ihead.toffset;
  cout << format("          decription: %.80s\n") % ihead.descrip;
  cout << format("            aux_file: %.24s\n") % ihead.aux_file;
  cout << format("         qform/sform: %d/%d\n") % ihead.qform_code % ihead.sform_code;
  cout << format("       quatern b/c/d: %g/%g/%g\n") % ihead.quatern_b % ihead.quatern_c % ihead.quatern_d;
  cout << format("       qoffset x/y/z: %g/%g/%g\n") % ihead.qoffset_x % ihead.qoffset_y % ihead.qoffset_z;
  cout << format("              srow_x: %8.4g %8.4g %8.4g %8.4g\n") % ihead.srow_x[0] % ihead.srow_x[1]
    % ihead.srow_x[2] % ihead.srow_x[3];
  cout << format("              srow_y: %8.4g %8.4g %8.4g %8.4g\n") % ihead.srow_y[0] % ihead.srow_y[1]
    % ihead.srow_y[2] % ihead.srow_y[3];
  cout << format("              srow_z: %8.4g %8.4g %8.4g %8.4g\n") % ihead.srow_z[0] % ihead.srow_z[1]
    % ihead.srow_z[2] % ihead.srow_z[3];
  cout << format("               magic: %.4s\n") % ihead.magic;
}

void
nifti_from_VB_datatype(NIFTI_header &hdr,const VB_datatype datatype)
{
  switch(datatype) {
  case vb_byte:
    hdr.datatype=DT_UINT8;
    hdr.bitpix=8;
    break;
  case vb_short:
    hdr.datatype=DT_INT16;
    hdr.bitpix=16;
    break;
  case vb_long:
    hdr.datatype=DT_UINT32;
    hdr.bitpix=32;
    break;
  case vb_float:
    hdr.datatype=DT_FLOAT32;
    hdr.bitpix=32;
    break;
  case vb_double:
    hdr.datatype=DT_FLOAT64;
    hdr.bitpix=64;
    break;
  }
  return;
}

// 3D version

int
nifti_write_3D(string fname,Cube &im)
{
  // tmpfname must preserve extension!
  string tmpfname=(format("%s/tmp_%d_%d_%s")%xdirname(fname)%
                   getpid()%time(NULL)%xfilename(fname)).str();
  NIFTI_header hdr;
  bool f_ext=0;
  size_t offset=NIFTI_MIN_OFFSET;
  // unscale if needed
  if (im.f_scaled) {
    im-=im.scl_inter;
    im/=im.scl_slope;
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(im.altdatatype);
  }
  // copy stuff that's the same for 3D and 4D
  voxbo2nifti_header(im,hdr);
  // now set some 3D-specific stuff
  hdr.xyzt_units=NIFTI_UNITS_MM;
  hdr.dim[0]=3;
  strcpy(hdr.descrip,"NIfTI-1 3D file produced by VoxBo");
  // build the voxbo header extension, so we can get the offset right
  string buf;
  if (f_ext) {
    for (size_t i=0; i<im.header.size(); i++)
      buf+=im.header[i]+'\n';
    // the offset is an extra ecode+esize, plus the size of the extension, plus the sentinel
    if (buf.size())
      offset+=buf.size()+16;
  }
  hdr.vox_offset=offset;
  // swap if needed
  if (im.filebyteorder!=my_endian()) {
    nifti_swap_header(hdr);
    im.byteswap();
  }
  // open the file, write the header
  zfile zfp;
  zfp.open(tmpfname,"w");
  if (!zfp)
    return 101;
  size_t cnt;
  cnt=zfp.write(&hdr,sizeof(NIFTI_header));
  if (cnt!=sizeof(NIFTI_header)) {
    zfp.close_and_unlink();
    return 102;
  }
  // write voxbo nifti extension
  if (f_ext && im.header.size()) {
    zfp.write("X\0\0\0",4);
    uint32 ecode=NIFTI_ECODE_VOXBO;
    uint32 esize;
    string buf;
    for (size_t i=0; i<im.header.size(); i++)
      buf+=im.header[i]+'\n';
    esize=buf.size();
    if (im.filebyteorder!=my_endian())
      swap(&ecode),swap(&esize);
    cnt=zfp.write(&esize,4);
    cnt+=zfp.write(&ecode,4);
    cnt+=zfp.write(buf.c_str(),buf.size());
    if (cnt!=buf.size()+8) {
      zfp.close_and_unlink();
      return 102;
    }
    // write the sentinel
    zfp.write("\0\0\0\0",4);
  }
  else
    zfp.write("\0\0\0\0",4);
  // write out the data
  size_t sz=im.dimx*im.dimy*im.dimz*im.datasize;
  zfp.seek(offset,SEEK_SET);
  cnt=zfp.write(im.data,sz);
  zfp.close();
  if (cnt!=sz) {
    zfp.close_and_unlink();
    return 103;
  }
  // re-swap/scale, we may still need the data!
  if (im.f_scaled) {
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(vb_float);
    im*=im.scl_slope;
    im+=im.scl_inter;
  }
  if (im.filebyteorder!=my_endian())
    im.byteswap();
  if (rename(tmpfname.c_str(),fname.c_str()))
    return (103);
  return 0;
}

int
nifti_write_4D(string fname,Tes &im)
{
  // tmpfname must preserve extension
  string tmpfname=(format("%s/tmp_%d_%d_%s")%xdirname(fname)%
                   getpid()%time(NULL)%xfilename(fname)).str();
  NIFTI_header hdr;
  size_t offset=NIFTI_MIN_OFFSET;
  bool f_ext=0;
  // un-scale if needed
  if (im.f_scaled) {
    im-=im.scl_inter;
    im/=im.scl_slope;
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(im.altdatatype);
  }
  // copy stuff that's the same for 3D and 4D
  voxbo2nifti_header(im,hdr);
  // now set some 4D-specific stuff
  hdr.dim[0]=4;
  hdr.dim[4]=im.dimt;
  hdr.pixdim[4]=im.voxsize[3];
  hdr.xyzt_units=NIFTI_UNITS_MM | NIFTI_UNITS_MSEC;
  double tr=im.voxsize[3];
  if (tr<FLT_MIN) tr=1000;
  hdr.pixdim[4]=tr;
  strcpy(hdr.descrip,"NIfTI-1 4D file produced by VoxBo");
  // build the voxbo header extension, so we can get the offset right
  string buf;
  if (f_ext) {
    for (size_t i=0; i<im.header.size(); i++)
      buf+=im.header[i]+'\n';
    // the offset is an extra ecode+esize, plus the size of the extension, plus the sentinel
    if (buf.size())
      offset+=buf.size()+16;
  }
  hdr.vox_offset=offset;
  // swap if needed
  if (im.filebyteorder!=my_endian()) {
    nifti_swap_header(hdr);
    im.byteswap();
  }
  // open file, write out the header
  zfile zfp;  // zfp takes care of compression
  zfp.open(tmpfname,"w");
  if (!zfp)
    return 101;
  size_t cnt;
  cnt=zfp.write(&hdr,sizeof(NIFTI_header));
  if (cnt!=sizeof(NIFTI_header)) {
    zfp.close_and_unlink();
    return 102;
  }
  // write voxbo nifti extension
  if (f_ext && im.header.size()) {
    zfp.write("X\0\0\0",4);
    uint32 ecode=NIFTI_ECODE_VOXBO;
    uint32 esize;
    string buf;
    for (size_t i=0; i<im.header.size(); i++)
      buf+=im.header[i]+'\n';
    esize=buf.size();
    if (im.filebyteorder!=my_endian())
      swap(&ecode),swap(&esize);
    cnt=zfp.write(&esize,4);
    cnt+=zfp.write(&ecode,4);
    cnt+=zfp.write(buf.c_str(),buf.size());
    if (cnt!=buf.size()+8) {
      zfp.close_and_unlink();
      return 102;
    }
    // write the sentinel
    zfp.write("\0\0\0\0",4);
  }
  else
    zfp.write("\0\0\0\0",4);
  // write out the data
  size_t sz=im.dimx*im.dimy*im.dimz*im.datasize;
  zfp.seek(offset,SEEK_SET);
  for (int i=0; i<im.dimt; i++) {
    Cube cb=im[i];
    cnt=zfp.write(cb.data,sz);
    if (cnt!=sz) {
      zfp.close_and_unlink();
      return 103;
    }
  }
  zfp.close();
  // re-scale/swap back, we may still need the data!
  if (im.f_scaled) {
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(vb_float);
    im*=im.scl_slope;
    im+=im.scl_inter;
  }
  if (im.filebyteorder!=my_endian())
    im.byteswap();
  if (rename(tmpfname.c_str(),fname.c_str()))
    return (103);
  return 0;
}

// nifti_read_3D_data() assumes the header is already sucked into cb,
// and we don't need the nifti header anymore (we do know the offset)

int
nifti_read_3D_data(Cube &im)
{
  string fname=im.GetFileName();
  if (xgetextension(fname)=="hdr")
    fname=xsetextension(fname,"img");
  im.SetVolume(im.dimx,im.dimy,im.dimz,im.datatype);
  if (im.dimx<1 || im.dimy<1 || im.dimz<1) {
    im.data_valid=0;
    return 105;
  }
  if (!im.data)
    return 101;
  gzFile fp=gzopen(fname.c_str(),"r");
  if (!fp) {
    delete [] im.data;
    im.data=(unsigned char *)NULL;
    im.data_valid=0;
    return (119);
  }
  if (gzseek(fp,im.offset,SEEK_SET)==-1) {
    gzclose(fp);
    delete [] im.data;
    im.data=(unsigned char *)NULL;
    im.data_valid=0;
    return (120);
  }
  size_t bytelen=im.dimx*im.dimy*im.dimz;
  size_t cnt=gzread(fp,im.data,im.datasize*bytelen);
  gzclose(fp);
  if (cnt!=bytelen*im.datasize) {
    delete [] im.data;
    im.data=(unsigned char *)NULL;
    im.data_valid=0;
    return (130);
  }
  if (my_endian() != im.filebyteorder)
    im.byteswap();
  if (im.f_scaled) {
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(vb_float);
    im*=im.scl_slope;
    im+=im.scl_inter;
  }
  im.data_valid=1;
  return 0;
}

int
nifti_read_4D_data(Tes &im,int start,int count)
{
  string fname=im.GetFileName();
  if (xgetextension(fname)=="hdr")
    fname=xsetextension(fname,"img");
  im.SetVolume(im.dimx,im.dimy,im.dimz,im.dimt,im.datatype);
  if (im.dimx<1 || im.dimy<1 || im.dimz<1 || im.dimt<1) {
    im.data_valid=0;
    return 105;
  }
  if (!im.data)
    return 101;
  gzFile fp=gzopen(fname.c_str(),"r");
  if (!fp) {
    im.invalidate();
    return (119);
  }
  if (gzseek(fp,im.offset,SEEK_SET)==-1) {
    gzclose(fp);
    im.invalidate();
    return (120);
  }

  // honor volume range
  if (start==-1) {
    start=0;
    count=im.dimt;
  }
  else if (start+count>im.dimt)
    return 220;
  im.dimt=count;

  // misnomer, actually the number of voxels per volume
  size_t bytelen=im.dimx*im.dimy*im.dimz;
  Cube cb(im.dimx,im.dimy,im.dimz,im.datatype);
  // skip the omitted volumes
  if (gzseek(fp,cb.datasize*bytelen*start,SEEK_CUR)==-1) {
    gzclose(fp);
    im.invalidate();
    return 121;
  }
  for (int i=0; i<im.dimt; i++) {
    size_t cnt=gzread(fp,cb.data,cb.datasize*bytelen);
    if (cnt!=bytelen*cb.datasize){
      gzclose(fp);
      im.invalidate();
      return 110;
    }
    // we need to byteswap at the cube level bnecause SetCube() below
    // might need to check values
    if (my_endian() != im.filebyteorder)
      cb.byteswap();
    im.SetCube(i,cb);    
  }
  if (im.f_scaled) {
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      im.convert_type(vb_float);
    im*=im.scl_slope;
    im+=im.scl_inter;
  }
  gzclose(fp);
  im.data_valid=1;
  im.Remask();
  return 0;
}

int
nifti_read_ts(Tes &im,int x,int y,int z)
{
  string fname=im.GetFileName();
  if (xgetextension(fname)=="hdr")
    fname=xsetextension(fname,"img");
  if (x<0 || y<0 || z<0 || x>im.dimx-1 || y>im.dimy-1 || z>im.dimz-1)
    return 101;
  // cb.SetVolume(im.dimx,im.dimy,im.dimz,im.datatype);
  gzFile fp=gzopen(fname.c_str(),"r");
  if (!fp)
    return (119);
  if (gzseek(fp,im.offset,SEEK_SET)==-1) {
    gzclose(fp);
    return (120);
  }

  // skip t volumes
  int bytelen=im.dimx*im.dimy*im.dimz;
  
  // skip to the voxel of interest in the first volume
  if (gzseek(fp,im.voxelposition(x,y,z)*im.datasize,SEEK_CUR)==-1) {
    gzclose(fp);
    im.invalidate();
    return 121;
  }
  unsigned char tmpdata[im.datasize*im.dimt];
  int index=0;
  for (int i=0; i<im.dimt; i++) {
    size_t cnt=gzread(fp,tmpdata+index,im.datasize);
    if (cnt!=(size_t)im.datasize){
      gzclose(fp);
      im.invalidate();
      return 110;
    }
    index+=im.datasize;
    gzseek(fp,(bytelen-1)*im.datasize,SEEK_CUR);
  }
  gzclose(fp);
  if (my_endian() != im.filebyteorder)
    swapn(tmpdata,im.datasize,im.dimt);
  im.timeseries.resize(im.dimt);
  unsigned char *ptr=tmpdata;
  for (int i=0; i<im.dimt; i++) {
    im.timeseries.setElement(i,toDouble(im.datatype,ptr));
    ptr += im.datasize;
  }
  if (im.f_scaled) {
    im.timeseries*=im.scl_slope;
    im.timeseries+=im.scl_inter;
  }
  return 0;
}

int
nifti_read_vol(Tes &im,Cube &cb,int t)
{
  string fname=im.GetFileName();
  if (xgetextension(fname)=="hdr")
    fname=xsetextension(fname,"img");
  if (t<0 || t>im.dimt-1)
    return 101;
  cb.SetVolume(im.dimx,im.dimy,im.dimz,im.datatype);
  gzFile fp=gzopen(fname.c_str(),"r");
  if (!fp) {
    cb.invalidate();
    return (119);
  }
  if (gzseek(fp,im.offset,SEEK_SET)==-1) {
    gzclose(fp);
    cb.invalidate();
    return (120);
  }

  // skip t volumes
  int bytelen=im.dimx*im.dimy*im.dimz;
  // skip the omitted volumes
  if (gzseek(fp,cb.datasize*bytelen*t,SEEK_CUR)==-1) {
    gzclose(fp);
    im.invalidate();
    return 121;
  }
  int cnt=gzread(fp,cb.data,cb.datasize*bytelen);
  if (cnt!=bytelen*cb.datasize){
    gzclose(fp);
    im.invalidate();
    return 110;
  }
  gzclose(fp);
  if (my_endian() != im.filebyteorder)
    cb.byteswap();
  if (im.f_scaled) {
    if (im.altdatatype==vb_byte || im.altdatatype==vb_short || im.altdatatype==vb_long)
      cb.convert_type(vb_float);
    cb*=im.scl_slope;
    cb+=im.scl_inter;
  }

  return 0;
}

void
nifti_swap_header(NIFTI_header &hdr)
{
  swap(&(hdr.sizeof_hdr));
  swap(hdr.dim,8);
  swap(hdr.pixdim,8);
  swap(&(hdr.datatype));
  swap(&(hdr.bitpix));
  swap(&(hdr.vox_offset));
  swap(&(hdr.cal_max));
  swap(&(hdr.cal_min));

  swap(&(hdr.intent_p1));
  swap(&(hdr.intent_p2));
  swap(&(hdr.intent_p3));
  swap(&(hdr.intent_code));
  swap(&(hdr.slice_start));
  swap(&(hdr.slice_end));
  swap(&(hdr.scl_slope));
  swap(&(hdr.scl_inter));
  swap(&(hdr.slice_duration));
  swap(&(hdr.toffset));
  swap(&(hdr.qform_code));
  swap(&(hdr.sform_code));
  swap(&(hdr.quatern_b));
  swap(&(hdr.quatern_c));
  swap(&(hdr.quatern_d));
  swap(&(hdr.qoffset_x));
  swap(&(hdr.qoffset_y));
  swap(&(hdr.qoffset_z));
  swap(hdr.srow_x,4);
  swap(hdr.srow_y,4);
  swap(hdr.srow_z,4);
}

// the following function copies header info from voxbo to nifti.
// this function is called for both the 3D and the 4D case.  the
// reverse function is all done within the nifti_read_header function

void
voxbo2nifti_header(VBImage &im,NIFTI_header &hdr)
{
  // prep the structure
  memset(&hdr,0,sizeof(NIFTI_header));
  hdr.sizeof_hdr=348;
  strcpy(hdr.magic,"n+1");
  hdr.regular='r';
  // copy stuff that's the same for 3D and 4D
  hdr.dim[1]=im.dimx;
  hdr.dim[2]=im.dimy;
  hdr.dim[3]=im.dimz;
  hdr.dim[4]=1;
  hdr.dim[5]=1;
  hdr.dim[6]=1;
  hdr.dim[7]=1;
  nifti_from_VB_datatype(hdr,im.datatype);  // sets datatype and bitpix
  hdr.pixdim[0]=-1;
  hdr.pixdim[1]=im.voxsize[0];
  hdr.pixdim[2]=im.voxsize[1];
  hdr.pixdim[3]=im.voxsize[2];
  hdr.pixdim[4]=im.voxsize[3];
  hdr.pixdim[5]=1.0;
  hdr.pixdim[6]=1.0;
  hdr.pixdim[7]=1.0;
  if (isfinite(im.qoffset[0])) {
    hdr.qoffset_x=im.qoffset[0];
    hdr.qoffset_y=im.qoffset[1];
    hdr.qoffset_z=im.qoffset[2];
  }
  else {
    hdr.qoffset_x=im.origin[0];
    hdr.qoffset_y=im.origin[1];
    hdr.qoffset_z=im.origin[2];
  }
  hdr.scl_slope=im.scl_slope;
  hdr.scl_inter=im.scl_inter;

  hdr.vox_offset=NIFTI_MIN_OFFSET;
  hdr.qform_code=im.qform_code;
  hdr.sform_code=im.sform_code;
  hdr.quatern_b=im.quatern_b;
  hdr.quatern_c=im.quatern_c;
  hdr.quatern_d=im.quatern_d;
  hdr.srow_x[0]=im.srow_x[0];
  hdr.srow_x[1]=im.srow_x[1];
  hdr.srow_x[2]=im.srow_x[2];
  hdr.srow_x[3]=im.srow_x[3];
  hdr.srow_y[0]=im.srow_y[0];
  hdr.srow_y[1]=im.srow_y[1];
  hdr.srow_y[2]=im.srow_y[2];
  hdr.srow_y[3]=im.srow_y[3];
  hdr.srow_z[0]=im.srow_z[0];
  hdr.srow_z[1]=im.srow_z[1];
  hdr.srow_z[2]=im.srow_z[2];
  hdr.srow_z[3]=im.srow_z[3];

  tokenlist hh;
  for (size_t i=0; i<im.header.size(); i++) {
    hh.ParseLine(im.header[i]);
    if (hh[0]=="nifti_dim_info ")
      hdr.dim_info=hh[1][0];
    else if (hh[0]=="nifti_intent_p1")
      hdr.intent_p1=strtod(hh[1]);
    else if (hh[0]=="nifti_intent_p2")
      hdr.intent_p2=strtod(hh[1]);
    else if (hh[0]=="nifti_intent_p3")
      hdr.intent_p3=strtod(hh[1]);
    else if (hh[0]=="nifti_intent_code")
      hdr.intent_code=strtol(hh[1]);
    else if (hh[0]=="nifti_pixdim0")
      hdr.pixdim[0]=strtod(hh[1]);
    else if (hh[0]=="nifti_slice_end")
      hdr.slice_end=strtol(hh[1]);
    else if (hh[0]=="nifti_slice_code")
      hdr.slice_code=hh[1][0];
    else if (hh[0]=="nifti_cal_max")
      hdr.cal_max=strtod(hh[1]);
    else if (hh[0]=="nifti_cal_min")
      hdr.cal_min=strtod(hh[1]);
    else if (hh[0]=="nifti_slice_duration")
      hdr.slice_duration=strtod(hh[1]);
    else if (hh[0]=="nifti_toffset")
      hdr.toffset=strtod(hh[1]);
  }
}

} // extern "C"
