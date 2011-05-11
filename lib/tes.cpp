
// tes.cpp
// VoxBo Tes class
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

using namespace std;

#include "vbutil.h"
#include "vbio.h"

Tes::Tes()
{
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  init();
}

Tes::Tes(const string &file)
{
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  init();
  ReadFile(file);
}

Tes::Tes(int in_dimx,int in_dimy,int in_dimz,int in_dimt,VB_datatype in_type)
{
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  init(in_dimx,in_dimy,in_dimz,in_dimt,in_type);
}

Tes::Tes(const Tes &ts) : VBImage(ts)
{
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  init();
  *this=ts;
}

void
Tes::init()
{
  VBImage::init();
  zero();
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  realvoxels=0;
}

int
Tes::init(int in_dimx,int in_dimy,int in_dimz,int in_dimt,VB_datatype in_type)
{
  init();
  return (SetVolume(in_dimx,in_dimy,in_dimz,in_dimt,in_type));
}

void
Tes::SetVoxSizes(float v1,float v2,float v3)
{
  voxsize[0]=v1;
  voxsize[1]=v2;
  voxsize[2]=v3;
}

int
Tes::SetVolume(uint32 in_dimx,uint32 in_dimy,uint32 in_dimz,uint32 in_dimt,VB_datatype in_type)
{
  if (in_dimx>MAX_DIM || in_dimy>MAX_DIM || in_dimz>MAX_DIM || in_dimt>MAX_DIM)
    return 101;
  dimx = in_dimx;
  dimy = in_dimy;
  dimz = in_dimz;
  dimt = in_dimt;
  voxels = dimx * dimy * dimz;
  SetDataType(in_type);
  InitData(); 
  InitMask(0);
  header_valid=1;
  return 0;
}

int
Tes::DimsValid()
{
  if (dimx<1 || dimy<1 || dimz<1 || dimt<1)
    return 0;
  // FIXME might want to rule out some improbably large sizes here
  return 1;
}

int
Tes::InitMask(short val)
{
  if (!DimsValid())
    return 101;
  if (mask && !f_mirrored)
    delete [] mask;
  f_mirrored=0;
  mask = new unsigned char[dimx*dimy*dimz];
  if (!mask)
    return 102;
  for (int i=0; i<dimx*dimy*dimz; i++)
    mask[i]=val;
  return 0;
}

int
Tes::InitData()
{
  if (!DimsValid())
    return 101;
  if (data && !f_mirrored) {
    for (int i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
        delete [] data[i];
    }
    delete [] data;
  }
  f_mirrored=0;
  data = new unsigned char *[dimx*dimy*dimz];
  if (!data)
    return 102;
  for (int i=0; i<dimx*dimy*dimz; i++)
    data[i]=(unsigned char *)NULL;
  data_valid=1;
  return 0;
}

Tes::~Tes()
{
  invalidate();
}

void
Tes::zero()
{
  if (!data)
    return;
  scl_slope=0.0;
  scl_inter=0.0;
  if (data) {  // no mirror check, we're modifying both copies
    for (int i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
        zerovoxel(i);
    }
  }
}

void
Tes::invalidate()
{
  zero();
  header.clear();
  if (data && !f_mirrored)
    delete [] data;
  if (mask && !f_mirrored)
    delete [] mask;
  f_mirrored=0;
  mask = (unsigned char *)NULL;
  data = (unsigned char **)NULL;
  realvoxels=0;
  data_valid=0;
  header_valid=0;
}

int
Tes::ReadFile(const string &fname,int start,int count)
{
  // never call reparse here, because it will end up converting
  // foo.tes:3 to foo.tes, and we'll end up reading it as 4D
  int err;
  if ((err=ReadHeader(fname)))
    return err;
  if ((err=ReadData(fname,start,count)))
    return err;
  return 0;
}

int
Tes::ReadHeader(const string &fname)
{
  // never call reparse here, because it will end up converting
  // foo.tes:3 to foo.tes, and we'll end up reading it as 4D
  init();
  if (fname.size()==0)
    return 104;
  filename=fname;
  vector<VBFF> ftypes=EligibleFileTypes(fname,4);
  if (ftypes.size()==0)
    return 101;
  // FIXME on error we could be nice and try multiple types
  fileformat=ftypes[0];
  if (!fileformat.read_head_4D)
    return 102;
  int err=fileformat.read_head_4D(this);
  return err;
}

int
Tes::ReadData(const string &fname,int start,int count)
{
  // never call reparse here, because it will end up converting
  // foo.tes:3 to foo.tes, and we'll end up reading it as 4D

  // FIXME if we have a valid header, ReadData needs to check if fname
  // matches filename
  filename=fname;
  int err;
  if (!header_valid) {
    if ((err=ReadHeader(fname)))
      return err;
  }
  if (!fileformat.read_data_4D)
    return 102;
  err=fileformat.read_data_4D(this,start,count);
  return err;
}

int
Tes::ReadTimeSeries(const string &fname,int x,int y,int z)
{
  int err;
  if (!header_valid) {
    if ((err=ReadHeader(fname)))
      return err;
  }
  if (!fileformat.read_ts_4D)
    return 102;
  err=fileformat.read_ts_4D(*this,x,y,z);
  return err;
}

int
Tes::ReadVolume(const string &fname,int t,Cube &cb)
{
  int err;
  if (!header_valid) {
    if ((err=ReadHeader(fname)))
      return err;
  }
  if (!fileformat.read_vol_4D)
    return 102;
  cb.init();
  err=fileformat.read_vol_4D(*this,cb,t);
  return err;
}

int
Tes::WriteFile(const string fname)
{
  VBFF original;
  // save the original format, then null it
  original=fileformat;
  fileformat.init();
  if (fname.size()) filename=fname;
  // reparse filename for tags, then see if our file format is set.
  // if not, try to do it by extension
  ReparseFileName();
  // if reparse didn't find anything, try by extension
  if (!fileformat.write_4D)
    fileformat=findFileFormat(filename,4);
  // if not, try original file's format
  if (!fileformat.write_4D)
    fileformat=original;
  // if not, try cub1
  if (!fileformat.write_4D)
    fileformat=findFileFormat("tes1");
  // if not (should never happen), bail
  if (!fileformat.write_4D)
    return 200;
  int err=fileformat.write_4D(this);
  return err;
}

int
Tes::SetCube(int t,const Cube &cube)
{
  if (t > dimt-1 || cube.dimx != dimx || cube.dimy != dimy
      || cube.dimz != dimz)
    return 0;
  const Cube *src;
  Cube newcube;
  src=&cube;
  if (cube.datatype!=datatype) {
    newcube=cube;
    newcube.convert_type(datatype);
    src=&newcube;
  }

  // FIXME should templatize the below
  for (int index=0;index<dimx*dimy*dimz; index++) {
    switch (datatype) {
    case vb_byte:
      {
      unsigned char val,*ptr;
      ptr=(unsigned char *)src->data;
      val=ptr[index];
      if (!data[index]) {
        if (val==0) continue;
        buildvoxel(index);
      }
      ptr=data[index];
      *(ptr+t)=val;
      }
      break;
    case vb_short:
      {
      int16 val,*ptr;
      ptr=(int16 *)src->data;
      val=ptr[index];
      if (!data[index]) {
        if (val==0) continue;
        buildvoxel(index);
      }
      ptr=(int16 *)data[index];
      *(ptr+t)=val;
      }
      break;
    case vb_long:
      {
      int32 val,*ptr;
      ptr=(int32 *)src->data;
      val=ptr[index];
      if (!data[index]) {
        if (val==0) continue;
        buildvoxel(index);
      }
      ptr=(int32 *)data[index];
      *(ptr+t)=val;
      }
      break;
    case vb_float:
      {
      float val,*ptr;
      ptr=(float *)src->data;
      val=ptr[index];
      if (!data[index]) {
        if (fabs(val)<FLT_MIN) continue;
        buildvoxel(index);
      }
      ptr=(float *)data[index];
      *(ptr+t)=val;
      }
      break;
    case vb_double:
      {
      double val,*ptr;
      ptr=(double *)src->data;
      val=ptr[index];
      if (!data[index]) {
        if (fabs(val)<DBL_MIN) continue;
        buildvoxel(index);
      }
      ptr=(double *)data[index];
      *(ptr+t)=val;
      break;
      }
    }
  }
  return 1;  // meaningless
}

// int
// Tes::oldSetCube(int index,const Cube &cube)
// {
//   int i,j,k;
//   if (index > dimt-1 || cube.dimx != dimx || cube.dimy != dimy
//       || cube.dimz != dimz)
//     return 0;
//   for (i=0; i<dimx; i++) {
//     for (j=0; j<dimy; j++) {
//       for (k=0; k<dimz; k++) {
//         SetValue(i,j,k,index,cube.GetValue(i,j,k));
//       }
//     }
//   }
//   return 1;  // meaningless
// }

int
Tes::GetTimeSeries(int x,int y,int z)
{
  if (!inbounds(x,y,z))
    return 101;
  timeseries.resize(dimt);
  for (int i=0; i<dimt; i++)
    timeseries[i]=GetValue(x,y,z,i);
  return 0;
}

int
Tes::MergeTes(Tes &src)
{
  Tes *dest=this;
  if (src.dimx!=dest->dimx) return 101;
  if (src.dimy!=dest->dimy) return 101;
  if (src.dimz!=dest->dimz) return 101;
  if (src.dimt!=dest->dimt) return 101;
  if (src.datatype!=dest->datatype)
    return 102;
  for (int i=0; i<dimx*dimy*dimz; i++) {
    if (src.data[i]==NULL)
      continue;
    if (data[i]==NULL)
      buildvoxel(i);
    memcpy(dest->data[i],src.data[i],datasize*dimt);
  }
  return 0;
}

Cube
Tes::getCube(const int index)
{
  Cube cb;
  getCube(index,cb);
  return cb;
}

int
Tes::getCube(int index,list<Cube> &cubelist)
{
  Cube tmpc;
  list<Cube> tmplist;
  tmplist.push_back(tmpc);
  int err=getCube(index,tmplist.front());
  if (err)
    return err;
  cubelist.splice(cubelist.end(),tmplist);
  return 0;
}

// FIXME the following convenient operator is an inefficient blight
// and should be removed as soon as possible

Cube
Tes::operator[](const int index)
{
  Cube c;
  getCube(index,c);
  return c;
}

int
Tes::getCube(const int index,Cube &c)
{
  c.init();
  if (!data_valid)
    return 101;
  c.dimx = dimx;
  c.dimy = dimy;
  c.dimz = dimz;
  c.datatype=datatype;
  c.datasize=datasize;
  c.altdatatype=altdatatype;
  c.voxels=dimx*dimy*dimz;
  c.data = new unsigned char[dimx*dimy*dimz*datasize];
  if (!c.data) exit(101);  // shouldn't happen
  c.CopyHeader(*this);

  // the following works when teses are stores as times series, which they are
 
  if (c.data) {
    memset(c.data,0,dimx*dimy*dimz*datasize);
    unsigned char *pos=c.data;
    uint32 ord=0,cpos=index*datasize;
    // long pos=0;
    // once we have a better Tes::getValue(), we can rewrite the following to be a lot faster
    for (int i=0; i<dimx*dimy*dimz; i++) {
      if (mask[i])
        memcpy(pos,data[ord]+cpos,datasize);
      pos+=datasize;
      ord++;
    }
  }
  else {
    return 103;
  }

  c.header_valid=1;
  c.data_valid=1;
  return 0;  // no error!
}

void
Tes::Remask()
{
  if (!mask)  // safety first
    return;

  int i,j,k,t,index;
  realvoxels=0;

  index=0;
  for (k=0; k<dimz; k++) {
    for (j=0; j<dimy; j++) {
      for (i=0; i<dimx; i++) {
        mask[index]=0;
        for (t=0; t<dimt; t++) {
          if (fabs(GetValue(i,j,k,t))>DBL_MIN) {
            mask[index]=1;
            realvoxels++;
            break;
          }
        }
        index++;
      }
    }
  }
}

double
Tes::GetValue(VBVoxel &v,int t) const
{
  return GetValue(v.x,v.y,v.z,t);
}

double
Tes::GetValue(int x,int y,int z,int t) const
{
  if (!inbounds(x,y,z) || t>dimt-1)
    return 0.0;
  int index = voxelposition(x,y,z);
  if (!data)                                  // application error
    return 0.0;
  if (!data[index])                               // not stored, so presumed 0
    return 0.0;

  unsigned char *ptr = data[index] + (t * datasize);
  double val=0.0;
  switch (datatype) {
  case vb_byte:
    val=(double)*((unsigned char *)ptr);
    break;
  case vb_short:
    val=(double)*((int16 *)ptr);
    break;
  case vb_long:
    val=(double)*((int32 *)ptr);
    break;
  case vb_float:
    val=(double)*((float *)ptr);
    break;
  case vb_double:
    val=(double)*((double *)ptr);
    break;
  }
  return val;
}

double
Tes::GetValueUnsafe(int x,int y,int z,int t) const
{
  int index = voxelposition(x,y,z);
  if (!data[index])                               // not stored, so presumed 0
    return 0.0;

  unsigned char *ptr = data[index] + (t * datasize);
  double val=0.0;
  switch (datatype) {
  case vb_byte:
    val=(double)*((unsigned char *)ptr);
    break;
  case vb_short:
    val=(double)*((int16 *)ptr);
    break;
  case vb_long:
    val=(double)*((int32 *)ptr);
    break;
  case vb_float:
    val=(double)*((float *)ptr);
    break;
  case vb_double:
    val=(double)*((double *)ptr);
    break;
  }
  return val;
}

template<class T>
T
Tes::getValue(VBVoxel &v,int t) const
{
  return getValue<T>(v.x,v.y,v.z,t);
}

template<class T>
T
Tes::getValue(int x,int y,int z,int t) const
{
  if (!inbounds(x,y,z) || t>dimt-1)
    return 0.0;
  int index = voxelposition(x,y,z);
  if (!data)                                  // application error
    return 0.0;
  if (!data[index])                               // not stored, so presumed 0
    return 0.0;

  unsigned char *ptr = data[index] + (t * datasize);
  T val=0;
  switch (datatype) {
  case vb_byte:
    val=(T)*((unsigned char *)ptr);
    break;
  case vb_short:
    val=(T)*((int16 *)ptr);
    break;
  case vb_long:
    val=(T)*((int32 *)ptr);
    break;
  case vb_float:
    val=(T)*((float *)ptr);
    break;
  case vb_double:
    val=(T)*((double *)ptr);
    break;
  }
  return val;
}

bool
Tes::GetMaskValue(int x,int y,int z) const
{
  if (!inbounds(x,y,z))
    return 0;
  int index=voxelposition(x,y,z);
  return (mask[index]);
}

bool
Tes::GetMaskValue(int index) const
{
  return (mask[index]);
}

// bool
// Tes::getMaskValueUnsafe(index) const
// {
//   unsigned char *ptr = mask+index;
//   return *ptr;
// }

int
Tes::VoxelStored(int x,int y,int z)
{
  if (!inbounds(x,y,z))
    return 0;
  int index=voxelposition(x,y,z);
  if (data[index])
    return 1;
  else
    return 0;
}

void
Tes::SetValue(int x,int y,int z,int t,double val)
{
  if (!inbounds(x,y,z) || t > dimt-1)
    return;
  int index = voxelposition(x,y,z);
  if (!data[index] && fabs(val)<DBL_MIN)  // already set!
    return;
  if (!data[index])
    buildvoxel(index);
  unsigned char *ptr = data[index] + (t * datasize);
  switch (datatype) {
  case vb_byte:
    *((unsigned char *)ptr)=(unsigned char)round(val);
    break;
  case vb_short:
    *((int16 *)ptr)=(int16)round(val);
    break;
  case vb_long:
    *((int32 *)ptr)=(int32)round(val);
    break;
  case vb_float:
    *((float *)ptr)=(float)val;
    break;
  case vb_double:
    *((double *)ptr)=val;
    break;
  }
}

int
Tes::maskcount()
{
  if (!data)
    return 0;
  int count=0;
  for (int i=0; i<dimx*dimy*dimz; i++)
    if (mask[i]) count++;
  realvoxels=count;
  return count;
}

unsigned char *
Tes::buildvoxel(int x,int y,int z)
{
  if (!data)
    return NULL;
  int index;
  if (y<0 && z<0)
    index=x;
  else
    index=voxelposition(x,y,z);
  if (data[index])
    return data[index];
  data[index]=new unsigned char [dimt*datasize];
  memset(data[index],0,dimt*datasize);
  realvoxels++;
  mask[index]=1;
  return data[index];
}

void
Tes::byteswap()
{
  if (!data)
    return;
  int i;
  switch(datatype) {
  case vb_short:
    for (i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
	swap((int16 *)data[i],dimt);
    }
    break;
  case vb_long:
    for (i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
	swap((int32 *)data[i],dimt);
    }
    break;
  case vb_byte:
    // no action necessary!
    break;
  case vb_float:
    for (i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
	swap((float *)data[i],dimt);
    }
    break;
  case vb_double:
    for (i=0; i<dimx*dimy*dimz; i++) {
      if (data[i])
	swap((double *)data[i],dimt);
    }
    break;
  }
}

double
Tes::GrandMean()
{
  double grandmean,timemean;
  int xyz;
  
  grandmean=0.0;
  for (int l=0; l<dimt; l++) {
    timemean=0.0;
    xyz=0;
    for (int i=0; i<dimx; i++) {
      for (int j=0; j<dimy; j++) {
	for (int k=0; k<dimz; k++) {
	  if (GetMaskValue(i,j,k)) {
	    timemean+=GetValue(i,j,k,l);
	    xyz++;
	  }
	}
      }
    }
    timemean /= xyz;
    grandmean+=timemean;
  }
  grandmean/=dimt;
  return grandmean;
}

// should print the total number of real (masked) voxels

void
Tes::print() const
{
  cout << *this;
}

ostream&
operator<<(ostream &os,const Tes &ts)
{
  os << endl << "+- 4D Image file " << xfilename(ts.GetFileName()) << " ("
     << ts.fileformat.getName() << ")"
     << " (" << DataTypeName(ts.f_scaled ? ts.altdatatype : ts.datatype)
     << (ts.f_scaled ? ", scaled)" : ")")
     << endl;
  if (!ts.header_valid) {
    os << "+- invalid 4D data\n";
    return os;
  }
  if (xdirname(ts.GetFileName()) != ".")
    os << "| path: " << xdirname(ts.GetFileName()) << "/" << endl;
  os << "| " << ts.dimx << "x" << ts.dimy << "x" << ts.dimz << " voxels, ";
  os << ts.dimt << " time points" << endl;
  os.setf(ios::fixed,ios::floatfield);
  os.precision(4);
  os << "| " << ts.voxsize[0] << " x " << ts.voxsize[1] << " x " << ts.voxsize[2] << " mm, TR=" << ts.voxsize[3] << "ms" << endl;
  os << "| realvoxels: " << ts.realvoxels << " of " << ts.dimx*ts.dimy*ts.dimz << endl;
  os.precision(1);
  os << "| " << ts.meglen() << "MB on disk (" <<
    (ts.filebyteorder == ENDIAN_BIG ? "msbfirst" : "lsbfirst") << ")" << endl;
  os << "| origin: (" << ts.origin[0] << "," << ts.origin[1] << "," << ts.origin[2] << ")" << endl;
  os.precision(2);
  if (ts.voxsize[0]>FLT_MIN && ts.voxsize[1]>FLT_MIN && ts.voxsize[1]>FLT_MIN) {
    string tmp=(format("[%g,%g,%g;%g,%g,%g]")
                %(ts.voxsize[0]*ts.origin[0])
                %(ts.voxsize[1]*ts.origin[1])
                %(ts.voxsize[2]*ts.origin[2])
                %(ts.voxsize[0]*(ts.dimx-ts.origin[0]-1))
                %(ts.voxsize[1]*(ts.dimy-ts.origin[1]-1))
                %(ts.voxsize[2]*(ts.dimz-ts.origin[2]-1))
                ).str();
    os << "| bounding box: "<<tmp<<endl;
  }


  if (ts.f_scaled) os << "| slope: " << ts.scl_slope << "," << "intercept: " << ts.scl_inter << endl;
  if (ts.header.size() > 0) {
    os << "+--user header----------" << endl;
    for (int i=0; i<(int)ts.header.size(); i++)
      os << "| " << ts.header[i] << endl;
  }
  os << "+-----------------------" << endl;

  return os;
}

void
Tes::printbrief(const string &flags) const
{
  string myflags=flags;
  if (!myflags.size())
    myflags="fdvt";

  cout << GetFileName();
  for (int i=0; i<(int)myflags.size(); i++) {
    if (i==0)
      cout << ": ";
    else
      cout << ", ";
    if (myflags[i]=='f')
      cout << "(" << fileformat.getName() << ")";
    else if (myflags[i]=='d')
      cout << dimx << "x" << dimy << "x" << dimz << "x" << dimt;
    else if (myflags[i]=='v') {
      cout << voxsize[0] << "x" << voxsize[1] << "x" << voxsize[2] << "mm, TR=" << voxsize[3];
    }
    else if (myflags[i]=='o')
      cout << origin[0] << "x" << origin[1] << "x" << origin[2];
    else if (myflags[i]=='r')
      cout << orient;
    else if (myflags[i]=='t')
      cout << "(" << DataTypeName(datatype) << (f_scaled ? ", scaled)" : ")");

  }
  cout << endl;
}

Tes &
Tes::operator+=(const Tes &ts)
{
  int x=dimx;
  int y=dimy;
  int z=dimz;
  int t=dimt;
  if (ts.dimx < x) x=ts.dimx;
  if (ts.dimy < y) y=ts.dimy;
  if (ts.dimz < z) z=ts.dimz;
  if (ts.dimt < t) t=ts.dimt;
  for (int i=0; i<x; i++) {
    for (int j=0; j<y; j++) {
      for (int k=0; k<z; k++) {
        for (int l=0; l<t; l++) {
          SetValue(i,j,k,l,GetValue(i,j,k,l)+ts.GetValue(i,j,k,l));
        }
      }
    }
  }
  return *this;
}

Tes &
Tes::operator+=(double num)
{
  for (int i=0; i<dimx; i++) {
    for (int j=0; j<dimy; j++) {
      for (int k=0; k<dimz; k++) {
        for (int l=0; l<dimt; l++) {
          SetValue(i,j,k,l,GetValue(i,j,k,l)+num);
        }
      }
    }
  }
  return *this;
}

Tes &
Tes::operator-=(double num)
{
  for (int i=0; i<dimx; i++) {
    for (int j=0; j<dimy; j++) {
      for (int k=0; k<dimz; k++) {
        for (int l=0; l<dimt; l++) {
          SetValue(i,j,k,l,GetValue(i,j,k,l)-num);
        }
      }
    }
  }
  return *this;
}

Tes &
Tes::operator*=(double num)
{
  for (int i=0; i<dimx; i++) {
    for (int j=0; j<dimy; j++) {
      for (int k=0; k<dimz; k++) {
        for (int l=0; l<dimt; l++) {
          SetValue(i,j,k,l,GetValue(i,j,k,l)*num);
        }
      }
    }
  }
  return *this;
}

Tes &
Tes::operator/=(double num)
{
  for (int i=0; i<dimx; i++) {
    for (int j=0; j<dimy; j++) {
      for (int k=0; k<dimz; k++) {
        for (int l=0; l<dimt; l++) {
          SetValue(i,j,k,l,GetValue(i,j,k,l)/num);
        }
      }
    }
  }
  return *this;
}

Tes &
Tes::operator=(const Tes &ts)
{
  return copytes(ts);
}

Tes &
Tes::copytes(const Tes &ts,bool mirrorflag)
{
  if (!ts.header_valid)
    return *this;
  init();
  // copy header, then data
  CopyHeader(ts);
  dimx=ts.dimx;
  dimy=ts.dimy;
  dimz=ts.dimz;
  dimt=ts.dimt;
  datasize=ts.datasize;
  voxels=ts.voxels;
  offset=ts.offset;
  data_valid=ts.data_valid;
  realvoxels=ts.realvoxels;
  datatype=ts.datatype;
  fileformat=ts.fileformat;
  if (mirrorflag) {
    data=ts.data;
    mask=ts.mask;
  }
  else {
    if (ts.data) {
      data=new unsigned char *[dimx*dimy*dimz];
      if (!data) exit(999);
      for (int i=0; i<dimx*dimy*dimz; i++) {
	if (ts.data[i]) {
	  data[i]=new unsigned char[dimt*datasize];
	  if (!data[i]) exit(999);
	  memcpy(data[i],ts.data[i],dimt*datasize);
	}
	else
	  data[i]=NULL;
      }
    }
    else
      data=(unsigned char **)NULL;
    if (ts.mask) {
      mask=new unsigned char[dimx*dimy*dimz];
      if (mask)
	memcpy(mask,ts.mask,dimx*dimy*dimz);
    }
    else
      mask=(unsigned char *)NULL;
  }
  SetFileName(ts.GetFileName());
  filebyteorder=ts.filebyteorder;
  return *this;
}

// return length of data in MB

float
Tes::meglen() const
{
  if (header_valid)
    return (float) (dimt * realvoxels * datasize) / (1024 * 1024);
  return 0.0;
}

const unsigned char *
Tes::GetMaskPtr()
{
  return mask;
}

int
Tes::ExtractMask(Cube &target)
{
  if (!header_valid || !mask)
    return 101;
  target.SetVolume(dimx,dimy,dimz,vb_byte);
  int index=0;
  for (int k=0; k<dimz; k++) {
    for (int j=0; j<dimy; j++) {
      for (int i=0; i<dimx; i++) {
        if (GetMaskValue(i,j,k))
          target.data[index]=1;
        index++;
      }
    }
  }
  target.voxsize[0]=voxsize[0];
  target.voxsize[1]=voxsize[1];
  target.voxsize[2]=voxsize[2];
  return 0;
}

int
Tes::convert_type(VB_datatype newtype,uint16 flags)
{
  // FIXME this is dangerous if we're mirroring, should probably
  // unmirror first
  if (!data)
    return 100;
  if (datatype!=newtype) {
    int ind=-1;
    for (int k=0; k<dimz; k++) {
      for (int j=0; j<dimy; j++) {
        for (int i=0; i<dimx; i++) {
          ind++;
          if (data[ind]==NULL) continue;
          unsigned char *tmp=convert_buffer(data[ind],dimt,datatype,newtype);
          if (tmp==NULL) {
            invalidate();
            return 120;
          }
          delete [] data[ind];
          data[ind]=tmp;
        }
      }
    }
    SetDataType(newtype);
  }
  if (flags & VBSETALT)
    altdatatype=newtype;
  if (flags & VBNOSCALE) {
    f_scaled=0;
    scl_slope=scl_inter=0;
  }
  return 0;
}

void
Tes::zerovoxel(int index)
{
  delete [] data[index];
  data[index]=(unsigned char *)NULL;
  mask[index]=0;
}

void
Tes::zerovoxel(int x,int y,int z)
{
  zerovoxel(voxelposition(x,y,z));
}

void
Tes::removenans()
{
  int ind=-1;
  for (int k=0; k<dimz; k++) {
    for (int j=0; j<dimy; j++) {
      for (int i=0; i<dimx; i++) {
        ind++;
        if (data[ind]==NULL) continue;
        for (int t=0; t<dimt; t++) {
          if (!(finite(GetValue(i,j,k,t))))
            SetValue(i,j,k,t,0.0);
        }
      }
    }
  }
}

void
Tes::intersect(Cube &cb)
{
  for (int i=0; i<cb.dimx; i++) {
    for (int j=0; j<cb.dimy; j++) {
      for (int k=0; k<cb.dimz; k++) {
        if (GetMaskValue(i,j,k) && !(cb.testValue(i,j,k)))
          zerovoxel(i,j,k);
      }
    }
  }
}

void
Tes::applymask(Cube &m)
{
  for (int i=0; i<m.dimx*m.dimy*m.dimz; i++) {
	if (data[i] && !(m.testValue(i)))
      zerovoxel(i);
  }
}

void
Tes::compact()
{
  // smush the voxels to the front, while counting
  int ind=0;
  for (int i=0; i<dimx*dimy*dimz; i++) {
    if (mask[i]) {
      if (ind!=i) {
        mask[ind]=mask[i];
        mask[i]=0;
        data[ind]=data[i];
        data[i]=0;
      }
      ind++;
    }
  }
  // adjust dims
  dimx=ind;
  dimy=dimz=1;
  // now copy the mask over
  unsigned char *newmask=new unsigned char[ind];
  memcpy(newmask,mask,ind);
  delete [] mask;
  mask=newmask;
}

int
Tes::resizeInclude(const set<int> &includeset)
{
  if (includeset.empty())
    return 101;
  if (*(includeset.begin())<0)
    return 102;
  if (*(includeset.rbegin())>dimt-1)
    return 103;
  Tes tmptes;
  tmptes=*this;
  tmptes.SetVolume(dimx,dimy,dimz,includeset.size(),datatype);
  tmptes.header_valid=1;
  int ind=0;
  for (int i=0; i<dimt; i++) {
    if (includeset.count(i))
      tmptes.SetCube(ind++,(*this)[i]);
  }
  *this=tmptes;
  return 0;
}

int
Tes::resizeExclude(const set<int> &excludeset)
{
  if (excludeset.size()>(size_t)dimt)
    return 101;
  if (*(excludeset.begin())<0)
    return 102;
  if (*(excludeset.rbegin())>dimt-1)
    return 103;
  Tes tmptes;
  tmptes=*this;
  tmptes.SetVolume(dimx,dimy,dimz,dimt-excludeset.size(),datatype);
  tmptes.header_valid=1;
  int ind=0;
  for (int i=0; i<dimt; i++) {
    if (!(excludeset.count(i)))
      tmptes.SetCube(ind++,(*this)[i]);
  }
  *this=tmptes;
  return 0;
}

VB_Vector
getTS(vector<string> &teslist,int x,int y,int z,uint32 flags)
{
  VB_Vector signal;
  for (int i=0; i<(int)teslist.size(); i++) {
    Tes mytes;
    if (mytes.ReadTimeSeries(teslist[i],x,y,z)) {
      signal.clear();
      return signal;
    }
    if (flags & MEANSCALE)
      mytes.timeseries.meanNormalize();
    if (flags & DETREND)
      mytes.timeseries.removeDrift();
    signal.concatenate(mytes.timeseries);
  }
  return signal;
}

// FIXME was there a reason why we do the detrending after each voxel
// and don't wait for the whole region?

VB_Vector
getRegionTS(vector<string> &teslist,VBRegion &rr,uint32 flags)
{
  VB_Vector vv;
  uint64 xx,yy,zz;
  // if we can't get data, return the empty vector
  if (rr.size()==0) return vv;

  // read entire tes, then retrieve each ts
  if (rr.size()>10) {
    for (size_t i=0; i<teslist.size(); i++) {
      Tes mytes;
      if (mytes.ReadFile(teslist[i])) {
        vv.clear();
        return vv;
      }
      VB_Vector tsvv(mytes.dimt);
      tsvv.zero(); // *=0.0;
      for (VI myvox=rr.begin(); myvox!=rr.end(); myvox++) {
        rr.getxyz(myvox->first,xx,yy,zz);
        if (!(mytes.GetMaskValue(xx,yy,zz))) {
          vv.clear();
          return vv;
        }
        if (mytes.GetTimeSeries(xx,yy,zz)) {
          vv.clear();
          return vv;
        }
        if (flags & MEANSCALE)
          mytes.timeseries.meanNormalize();
        if (flags & DETREND)
          mytes.timeseries.removeDrift();
        tsvv+=mytes.timeseries;
      }
      tsvv/=rr.size();
      vv.concatenate(tsvv);
    }
    return vv;
  }
  // read each ts from disk
  else {
    // prefetch one just to get the vector started, then zero it
    vv=getTS(teslist,0,0,0,flags);
    vv.zero();
    for (VI myvox=rr.begin(); myvox!=rr.end(); myvox++) {
      rr.getxyz(myvox->first,xx,yy,zz);
      vv+=getTS(teslist,xx,yy,zz,flags);
    }
    if (rr.size())
      vv/=rr.size();
    return vv;
  }
}

VBMatrix
getRegionComponents(vector<string> &teslist,VBRegion &rr,uint32 flags)
{
  Tes tlist[teslist.size()];
  VBMatrix empty;
  int timepoints=0;
  string noname="";
  // iterate across teses, reading headers
  for (int i=0; i<(int)teslist.size(); i++) {
    if (tlist[i].ReadHeader(teslist[i]))
      return empty;
    timepoints+=tlist[i].dimt;
  }
  // build matrix
  VBMatrix components(timepoints,rr.size());
  int row=0;
  uint64 xx,yy,zz;

  // now iterate across teses and regions, grabbing the relevant timeseries
  for (int i=0; i<(int)teslist.size(); i++) {
    int j=0;
    for (VI myvox=rr.begin(); myvox!=rr.end(); myvox++) {
      rr.getxyz(myvox->first,xx,yy,zz);
      if (tlist[i].ReadTimeSeries(noname,xx,yy,zz))
        return empty;
      if (flags & MEANSCALE)
        tlist[i].timeseries.meanNormalize();
      if (flags & DETREND)
        tlist[i].timeseries.removeDrift();
      for (int m=row; m<row+tlist[i].dimt; m++) {
        gsl_matrix_set(&components.mview.matrix,m,j,tlist[i].timeseries[m-row]);
      }
    }
    j++;
    row+=tlist[i].dimt;
  }
  VBMatrix tmp,E;
  VB_Vector lambdas;
  if (pca(components,lambdas,tmp,E))
    return empty;
  return tmp;
}

VBRegion
restrictRegion(vector<string> &teslist,VBRegion &rr)
{
  VBRegion newreg;
  Cube mask,tesmask;

  // preload tes masks (in headers)
  for (size_t i=0; i<teslist.size(); i++) {
    Tes ts;
    if (ts.ReadHeader(teslist[i]))
      continue;
    if (!ts.fileformat.f_headermask) {
      if (ts.ReadFile(teslist[i]))
        continue;
    }
    if (ts.ExtractMask(tesmask))
      continue;
    if (!mask.data)
      mask=tesmask;
    else
      mask.intersect(tesmask);
  }
  // add masked-in voxels to new region
  for (VI myvox=rr.begin(); myvox!=rr.end(); myvox++) {
    if (mask.testValue(myvox->second.x,myvox->second.y,myvox->second.z))
      newreg.add(myvox->second);
  }
  return newreg;
}

template char Tes::getValue<char>(int x,int y,int z,int t) const;
template int16 Tes::getValue<int16>(int x,int y,int z,int t) const;
template int32 Tes::getValue<int32>(int x,int y,int z,int t) const;
template float Tes::getValue<float>(int x,int y,int z,int t) const;
template double Tes::getValue<double>(int x,int y,int z,int t) const;

