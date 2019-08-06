
// vbio.cpp
// VoxBo file I/O code
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
// contributions merged from Kosh Banerjee

using namespace std;

#include "vbio.h"
#include "vbutil.h"

string DataTypeName(const VB_datatype &dt) {
  switch (dt) {
    case vb_byte:
      return "byte";
    case vb_short:
      return "int16";
    case vb_long:
      return "int32";
    case vb_float:
      return "float";
    case vb_double:
      return "double";
  };
  return "";
}

void parsedatatype(const string &dtype, VB_datatype &datatype, int &datasize) {
  string dt = vb_tolower(dtype);
  if (dt == "integer" || dt == "short" || dt == "int16")
    datatype = vb_short;
  else if (dt == "long" || dt == "int32")
    datatype = vb_long;
  else if (dt == "float")
    datatype = vb_float;
  else if (dt == "double")
    datatype = vb_double;
  else if (dt == "byte")
    datatype = vb_byte;
  else
    datatype = vb_byte;
  datasize = getdatasize(datatype);
}

double toDouble(VB_datatype datatype, unsigned char *ptr) {
  switch (datatype) {
    case vb_byte:
      return (double)*((uint8 *)ptr);
      break;
    case vb_short:
      return (double)*((int16 *)ptr);
      break;
    case vb_long:
      return (double)*((int32 *)ptr);
      break;
    case vb_float:
      return (double)*((float *)ptr);
      break;
    case vb_double:
      return (double)*((double *)ptr);
      break;
    default:
      return 0.0;
  }
}

template <class T1, class T2>
unsigned char *convertbuffer2(T1 *from, int n) {
  T2 *to = new T2[n];
  if (!to) return NULL;
  for (int i = 0; i < n; i++) to[i] = (T2)from[i];
  return (unsigned char *)to;
}

unsigned char *convert_buffer(unsigned char *ptr, int n, VB_datatype oldtype,
                              VB_datatype newtype) {
  if (oldtype == vb_byte && newtype == vb_byte)
    return convertbuffer2<unsigned char, unsigned char>(ptr, n);
  if (oldtype == vb_byte && newtype == vb_short)
    return convertbuffer2<unsigned char, int16>(ptr, n);
  if (oldtype == vb_byte && newtype == vb_long)
    return convertbuffer2<unsigned char, int32>(ptr, n);
  if (oldtype == vb_byte && newtype == vb_float)
    return convertbuffer2<unsigned char, float>(ptr, n);
  if (oldtype == vb_byte && newtype == vb_double)
    return convertbuffer2<unsigned char, double>(ptr, n);

  if (oldtype == vb_short && newtype == vb_byte)
    return convertbuffer2<int16, unsigned char>((int16 *)ptr, n);
  if (oldtype == vb_short && newtype == vb_short)
    return convertbuffer2<int16, int16>((int16 *)ptr, n);
  if (oldtype == vb_short && newtype == vb_long)
    return convertbuffer2<int16, int32>((int16 *)ptr, n);
  if (oldtype == vb_short && newtype == vb_float)
    return convertbuffer2<int16, float>((int16 *)ptr, n);
  if (oldtype == vb_short && newtype == vb_double)
    return convertbuffer2<int16, double>((int16 *)ptr, n);

  if (oldtype == vb_long && newtype == vb_byte)
    return convertbuffer2<int32, unsigned char>((int32 *)ptr, n);
  if (oldtype == vb_long && newtype == vb_short)
    return convertbuffer2<int32, int16>((int32 *)ptr, n);
  if (oldtype == vb_long && newtype == vb_long)
    return convertbuffer2<int32, int32>((int32 *)ptr, n);
  if (oldtype == vb_long && newtype == vb_float)
    return convertbuffer2<int32, float>((int32 *)ptr, n);
  if (oldtype == vb_long && newtype == vb_double)
    return convertbuffer2<int32, double>((int32 *)ptr, n);

  if (oldtype == vb_float && newtype == vb_byte)
    return convertbuffer2<float, unsigned char>((float *)ptr, n);
  if (oldtype == vb_float && newtype == vb_short)
    return convertbuffer2<float, int16>((float *)ptr, n);
  if (oldtype == vb_float && newtype == vb_long)
    return convertbuffer2<float, int32>((float *)ptr, n);
  if (oldtype == vb_float && newtype == vb_float)
    return convertbuffer2<float, float>((float *)ptr, n);
  if (oldtype == vb_float && newtype == vb_double)
    return convertbuffer2<float, double>((float *)ptr, n);

  if (oldtype == vb_double && newtype == vb_byte)
    return convertbuffer2<double, unsigned char>((double *)ptr, n);
  if (oldtype == vb_double && newtype == vb_short)
    return convertbuffer2<double, int16>((double *)ptr, n);
  if (oldtype == vb_double && newtype == vb_long)
    return convertbuffer2<double, int32>((double *)ptr, n);
  if (oldtype == vb_double && newtype == vb_float)
    return convertbuffer2<double, float>((double *)ptr, n);
  if (oldtype == vb_double && newtype == vb_double)
    return convertbuffer2<double, double>((double *)ptr, n);
  return NULL;
}

VBImage::~VBImage() {}

// voxelposition() takes x, y, and z coordinates and transforms them
// into an ordinal position

int VBImage::voxelposition(int x, int y, int z) const {
  return (dimx * ((dimy * z) + y)) + x;
}

void VBImage::getXYZ(int32 &x, int32 &y, int32 &z, const uint32 point) const {
  unsigned int a = dimx * dimy;
  unsigned int b = point % a;
  z = point / a;
  y = b / dimx;
  x = b % dimx;
}

VBVoxel VBImage::getvoxel(const uint32 point) const {
  VBVoxel tmp;
  int32 x, y, z;
  getXYZ(x, y, z, point);
  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  return tmp;
}

// accessors

string VBImage::GetFileName() const { return filename; }

void VBImage::SetFileName(const string &fname) { filename = fname; }

VBFF VBImage::GetFileFormat() const { return fileformat; }

void VBImage::SetFileFormat(VBFF format) { fileformat = format; }

int VBImage::SetFileFormat(const string &format) {
  fileformat = findFileFormat(format);
  return 0;
}

void VBImage::AddHeader(const string &str) { header.push_back((string)str); }

// CopyHeader() copies most of the header stuff, except dims and
// datasize, which should be set when the data are created.  FIXME we
// probably need a header class so that copyheader doesn't have to
// "know" what headers to copy.  but for now the list is in vbio.h.

void VBImage::CopyHeader(const VBImage &im) {
  voxsize[0] = im.voxsize[0];
  voxsize[1] = im.voxsize[1];
  voxsize[2] = im.voxsize[2];
  voxsize[3] = im.voxsize[3];
  origin[0] = im.origin[0];
  origin[1] = im.origin[1];
  origin[2] = im.origin[2];
  qoffset[0] = im.qoffset[0];
  qoffset[1] = im.qoffset[1];
  qoffset[2] = im.qoffset[2];
  maskspecs = im.maskspecs;
  id1 = im.id1;
  id2 = im.id2;
  id3 = im.id3;
  id4 = im.id4;
  subvolume = im.subvolume;
  orient = im.orient;
  scl_slope = im.scl_slope;
  scl_inter = im.scl_inter;
  f_scaled = im.f_scaled;
  altdatatype = im.altdatatype;
  filebyteorder = im.filebyteorder;
  header = im.header;
  header_valid = im.header_valid;
}

string VBImage::GetHeader(string str) const {
  tokenlist hdr;
  string lowertag;
  for (int i = 0; i < (int)header.size(); i++) {
    hdr.ParseLine(header[i].c_str());
    if (hdr.size() < 1) continue;
    lowertag = hdr[0];
    if (vb_tolower(str) == vb_tolower(lowertag)) return ((string)hdr.Tail());
  }
  return (string) "";
}

int VBImage::WriteHeader(string tag, string newContents) {
  tokenlist hdr;
  string lowertag;
  string origTag = tag;
  for (int i = 0; i < (int)header.size(); i++) {
    hdr.ParseLine(header[i].c_str());
    if (hdr.size() < 1) continue;
    lowertag = hdr[0];
    if (vb_tolower(tag) == vb_tolower(lowertag)) {
      header[i] = (origTag + " " + newContents);
      return 0;
    }
  }
  return 1;
}

void VBImage::SetDataType(VB_datatype type) {
  datatype = type;
  datasize = getdatasize(type);
}

VB_datatype VBImage::GetDataType() { return datatype; }

int VBImage::GetDataSize() { return datasize; }

void VBImage::SetOrigin(float x, float y, float z) {
  origin[0] = (int32)roundf(x);
  origin[1] = (int32)roundf(y);
  origin[2] = (int32)roundf(z);
  qoffset[0] = x;
  qoffset[1] = y;
  qoffset[2] = z;
}

int VBImage::GetCorner(double &x, double &y, double &z) {
  tokenlist args;

  x = y = z = 0.0;
  // try absolute corner position first
  string corner = GetHeader("AbsoluteCornerPosition:");
  if (corner.size()) {
    args.ParseLine(corner);
    if (args.size() > 2) {
      x = strtod(args[0]);
      y = strtod(args[1]);
      z = strtod(args[2]);
      return 0;
    }
    return 0;
  }
  // older new style: zrange
  string zrange = GetHeader("ZRange:");
  if (zrange.size()) {
    args.ParseLine(zrange);
    if (args.size()) {
      z = strtod(args[0]);
      return 0;
    }
    return 0;
  }
  // yet older: im_tlhc
  string tlhc = GetHeader("im_tlhc:");
  if (tlhc.size()) {
    args.ParseLine(tlhc);
    if (args.size() > 2) {
      x = strtod(args[0]);
      y = strtod(args[1]);
      z = strtod(args[2]);
      return 0;
    }
  }
  // oldest style: startloc/endloc
  string startloc = GetHeader("StartLoc:");
  if (startloc.size()) {
    args.ParseLine(startloc);
    if (args.size()) {
      z = strtod(args[1]);
      return 0;
    }
  }
  return 101;
}

void VBImage::ReparseFileName() {
  // filenames such as foo.cub[big] get made into big-endian
  size_t leftpos, rightpos;
  // unsigned int leftpos,rightpos;
  leftpos = filename.find_last_of("[");
  rightpos = filename.find_last_of("]");
  if (rightpos > leftpos + 1 && leftpos != string::npos &&
      rightpos != string::npos) {
    tokenlist args;
    args.SetSeparator("/,;x");
    args.ParseLine(filename.substr(leftpos + 1, rightpos - leftpos - 1));
    filename = filename.substr(0, leftpos);
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i] == "big" || args[i] == "msbfirst") {
        filebyteorder = ENDIAN_BIG;
      } else if (args[i] == "small" || args[i] == "little" ||
                 args[i] == "lsbfirst") {
        filebyteorder = ENDIAN_LITTLE;
      }
      // the following is useful for formats that don't specify dims,
      // like totally raw data and MRIcro ROI files.  3D only at the
      // moment.
      else if ((args[i] == "dims" || args[i] == "dim") && i + 3 < args.size()) {
        dimx = strtol(args[i + 1]);
        dimy = strtol(args[i + 2]);
        dimz = strtol(args[i + 3]);
      } else {
        fileformat = findFileFormat(args[i]);
      }
    }
  }
  size_t cpos = filename.find_last_of(":");
  if (cpos != string::npos) {
    // if it's fname:mask, set subvolume to magic value -2
    string tag = filename.substr(cpos + 1, string::npos);
    if (vb_tolower(tag) == "mask") {
      subvolume = -2;
      filename = filename.substr(0, cpos);
    } else {
      pair<bool, int32> ret = strtolx(tag);
      if (!ret.first) {  // if we succeeded
        subvolume = ret.second;
        filename = filename.substr(0, cpos);
      }
    }
  }
}

// region functionality

// findregion_mask() creates a region from all the voxels in a mask
// that match the criterion

VBRegion findregion_mask(Cube &mask, int crit_type, double crit_val) {
  VBRegion reg;
  int i, j, k;
  double val;

  for (i = 0; i < mask.dimx; i++) {
    for (j = 0; j < mask.dimy; j++) {
      for (k = 0; k < mask.dimz; k++) {
        val = mask.GetValue(i, j, k);
        if (!voxelmatch(mask.GetValue(i, j, k), crit_type, crit_val)) continue;
        VBVoxel vv;
        vv.x = i;
        vv.y = j;
        vv.z = k;
        vv.val = val;
        reg.add(i, j, k, val);
      }
    }
  }
  return reg;
}

// findregions() creates a vector of regions, each of which is a set
// of contiguous voxels matching some criterion.  the first overloaded
// version creates an all-inclusive mask.  the second version
// restricts its search to in-mask voxels.

vector<VBRegion> findregions(Cube &mycub, int crit_type, double crit_val) {
  Cube mask;
  mask.SetVolume(mycub.dimx, mycub.dimy, mycub.dimz, vb_byte);
  // set mask to all-1's
  for (int i = 0; i < mycub.dimx; i++) {
    for (int j = 0; j < mycub.dimy; j++) {
      for (int k = 0; k < mycub.dimz; k++) {
        mask.SetValue(i, j, k, 1.0);
      }
    }
  }

  return findregions(mycub, mask, crit_type, crit_val);
}

vector<VBRegion> findregions(Cube &mycub, Cube &mask, int crit_type,
                             double crit_val) {
  vector<VBRegion> rlist;
  for (int i = 0; i < mycub.dimx; i++) {
    for (int j = 0; j < mycub.dimy; j++) {
      for (int k = 0; k < mycub.dimz; k++) {
        if (mask.GetValue(i, j, k) == 0) continue;
        if (voxelmatch(mycub.GetValue(i, j, k), crit_type, crit_val)) {
          VBRegion rr = growregion(i, j, k, mycub, mask, crit_type, crit_val);
          rr.voxsizes.copy(mycub.voxsize);
          rlist.push_back(rr);
        }
      }
    }
  }
  return rlist;
}

VBRegion growregion(int x, int y, int z, Cube &cb, Cube &mask, int crit_type,
                    double crit_val) {
  int32 i, j, k;
  uint32 xx, yy, zz;
  VBRegion myreg, hitlist;
  VBVoxel vx;
  double val;
  myreg.dimx = cb.dimx;
  myreg.dimy = cb.dimy;
  myreg.dimz = cb.dimz;
  hitlist.dimx = cb.dimx;
  hitlist.dimy = cb.dimy;
  hitlist.dimz = cb.dimz;

  hitlist.add(x, y, z, cb.getValue<double>(x, y, z));
  mask.SetValue(x, y, z, 0.0);

  // for each newly added voxel, check its neighborhood, add masked,
  // matching voxels to hitlist
  int lastitem = 0;
  while (hitlist.size()) {
    VI myvox = hitlist.begin();
    myreg.add(myvox->first, myvox->second.val);
    xx = myvox->second.x;
    yy = myvox->second.y;
    zz = myvox->second.z;
    hitlist.remove_i(myvox->first);
    for (i = (int32)xx - 1; i < (int32)xx + 2; i++) {
      if (i < 0 || i > cb.dimx - 1) continue;
      for (j = (int32)yy - 1; j < (int32)yy + 2; j++) {
        if (j < 0 || j > cb.dimy - 1) continue;
        for (k = (int32)zz - 1; k < (int32)zz + 2; k++) {
          if (k < 0 || k > cb.dimz - 1) continue;
          if (!(mask.testValue(i, j, k))) continue;
          val = cb.GetValue(i, j, k);
          if (voxelmatch(val, crit_type, crit_val)) {
            vx.x = i;
            vx.y = j;
            vx.z = k;
            vx.val = val;
            hitlist.add(i, j, k, val);
            lastitem++;
            mask.SetValue(i, j, k, 0.0);
          }
        }
      }
    }
  }
  return myreg;
}

bool voxelmatch(double val, int crit_type, double crit_val) {
  switch (crit_type) {
    case vb_any:
      return true;
    case vb_gt:
      if (val - crit_val >= DBL_MIN)
        return true;
      else
        return false;
      break;
    case vb_ge:
      if (!(crit_val - val >= DBL_MIN))
        return true;
      else
        return false;
      break;
    case vb_agt:
      if (fabs(val) - crit_val >= DBL_MIN)
        return true;
      else
        return false;
      break;
    case vb_age:
      if (!(crit_val - fabs(val) >= DBL_MIN))
        return true;
      else
        return false;
      break;
    case vb_lt:
      if (crit_val - val >= DBL_MIN)
        return true;
      else
        return false;
      break;
    case vb_le:
      if (!(val - crit_val >= DBL_MIN))
        return true;
      else
        return false;
      break;
    case vb_eq:
      if (fabs(val - crit_val) < DBL_MIN)
        return true;
      else
        return false;
      break;
    case vb_ne:
      if (fabs(val - crit_val) >= DBL_MIN)
        return true;
      else
        return false;
      break;
    default:
      return false;
  }
  return false;
}

double voxeldistance(const VBVoxel &v1, const VBVoxel &v2) {
  int d1 = v1.x - v2.x;
  int d2 = v1.y - v2.y;
  int d3 = v1.z - v2.z;
  return sqrt((d1 * d1) + (d2 * d2) + (d3 * d3));
}

double voxeldistance(uint32 x1, uint32 y1, uint32 z1, uint32 x2, uint32 y2,
                     uint32 z2) {
  int d1 = x1 - x2;
  int d2 = y1 - y2;
  int d3 = z1 - z2;
  return sqrt((d1 * d1) + (d2 * d2) + (d3 * d3));
}

bool dimsConsistent(int &x, int &y, int &z, int newx, int newy, int newz) {
  if (x <= 0) {
    x = newx;
    y = newy;
    z = newz;
    return 1;
  }
  if (x != newx) return 0;
  if (y != newy) return 0;
  if (z != newz) return 0;
  return 1;
}

VBRegion::VBRegion() { dimx = dimy = dimz = 0; }

VBRegion::VBRegion(Cube &cb, int crit_type, double crit_val) {
  convert(cb, crit_type, crit_val);
}

void VBRegion::convert(Cube &cb, int crit_type, double crit_val) {
  if (!cb.data) return;
  // convert mask to a region
  dimx = cb.dimx;
  dimy = cb.dimy;
  dimz = cb.dimz;
  voxsizes.copy(cb.voxsize);
  // int vcnt=cb.dimx*cb.dimy*cb.dimz;
  double v;
  for (int i = 0; i < cb.dimx; i++) {
    for (int j = 0; j < cb.dimy; j++) {
      for (int k = 0; k < cb.dimz; k++) {
        v = cb.getValue<double>(i, j, k);
        if (voxelmatch(v, crit_type, crit_val)) add(i, j, k, v);
      }
    }
  }
}

void VBRegion::clear() {
  voxels.clear();
  voxsizes.init();
}

int VBRegion::size() { return voxels.size(); }

void VBRegion::remove_i(uint64 index) { voxels.erase(index); }

void VBRegion::remove(uint64 x, uint64 y, uint64 z) {
  voxels.erase((MAX_DIM * ((MAX_DIM * z) + y)) + x);
}

void VBRegion::add(const VBVoxel &v) { add(v.x, v.y, v.z, v.val); }

void VBRegion::add(uint64 x, uint64 y, uint64 z, double val) {
  uint64 ind = (MAX_DIM * ((MAX_DIM * z) + y)) + x;
  // FIXME we're just going to convert it right back into xyz's in a second
  add(ind, val);
}

void VBRegion::add(uint64 ind, double val) {
  uint64 x, y, z;
  getxyz(ind, x, y, z);
  VBVoxel v(x, y, z, val);
  voxels[ind] = v;
}

bool VBRegion::contains_i(uint64 index) { return voxels.count(index); }

bool VBRegion::contains(uint64 x, uint64 y, uint64 z) {
  return voxels.count((MAX_DIM * ((MAX_DIM * z) + y)) + x);
}

void VBRegion::print() {
  cout << "Region " << name << endl;
  cout << "Voxel count: " << voxels.size() << endl;
  cout << "Value: " << val << endl;
  double x, y, z;
  GeometricCenter(x, y, z);
  cout << "Center: " << x << "," << y << "," << z << endl;
}

void VBRegion::getxyz(uint64 index, uint64 &x, uint64 &y, uint64 &z) {
  uint64 md2 = (uint64)MAX_DIM * MAX_DIM;
  z = index / md2;
  index -= z * md2;
  y = index / MAX_DIM;
  index -= y * MAX_DIM;
  x = index;
}

void VBRegion::GeometricCenter(double &x, double &y, double &z) {
  int voxelcount = 0;
  double sumx = 0, sumy = 0, sumz = 0;
  VI vi = begin();
  while (vi != end()) {
    sumx += vi->second.x;
    sumy += vi->second.y;
    sumz += vi->second.z;
    voxelcount++;
    vi++;
  }
  if (voxelcount > 0) {
    x = sumx / (double)voxelcount;
    y = sumy / (double)voxelcount;
    z = sumz / (double)voxelcount;
  }
}

VBRegion VBRegion::maxregion() {
  VBRegion rr;
  if (!voxels.size()) return rr;
  double maxval = begin()->second.val;
  for (VI v = begin(); v != end(); v++) {
    // if the voxel is larger than the max, make it the max
    if (v->second.val - maxval > DBL_MIN) {
      rr.clear();
      rr.add(v->second);
      maxval = v->second.val;
    }
    // if it's equal to the max, collect it
    else if (fabs(v->second.val - maxval) < DBL_MIN) {
      rr.add(v->second);
      maxval = v->second.val;
    }
  }
  return rr;
}

VBRegion VBRegion::minregion() {
  VBRegion rr;
  if (!voxels.size()) return rr;
  double minval = begin()->second.val;
  for (VI v = begin(); v != end(); v++) {
    // if the voxel is smaller than the min, make it the min
    if (minval - v->second.val > DBL_MIN) {
      rr.clear();
      rr.add(v->second);
      minval = v->second.val;
    }
    // if it's equal to the min, collect it
    else if (fabs(v->second.val - minval) < DBL_MIN) {
      rr.add(v->second);
      minval = v->second.val;
    }
  }
  return rr;
}

VBRegion VBRegion::minnonzeroregion() {
  bool f_found = 0;
  VBRegion rr;
  if (!voxels.size()) return rr;
  double minval = begin()->second.val;
  for (VI v = begin(); v != end(); v++) {
    if (fabs(v->second.val) < DBL_MIN) continue;
    // if we haven't found one yet...
    if (!f_found) {
      rr.add(v->second);
      minval = v->second.val;
      f_found = 1;
    }
    // if the voxel is smaller than the min, make it the min
    else if (minval - v->second.val > DBL_MIN) {
      rr.clear();
      rr.add(v->second);
      minval = v->second.val;
    }
    // if it's equal to the min, collect it
    else if (fabs(v->second.val - minval) < DBL_MIN) {
      rr.add(v->second);
      minval = v->second.val;
    }
  }
  return rr;
}

void VBRegion::max(uint64 &x, uint64 &y, uint64 &z, double &val) {
  if (!voxels.size()) {
    x = 0;
    y = 0;
    z = 0;
    val = 0.0;
    return;
  }

  uint64 index = begin()->first;
  val = begin()->second.val;

  for (VI v = begin(); v != end(); v++) {
    if (v->second.val > val) {
      val = v->second.val;
      index = v->first;
    }
  }
  getxyz(index, x, y, z);
}

void VBRegion::min(uint64 &x, uint64 &y, uint64 &z, double &val) {
  if (!voxels.size()) {
    x = 0;
    y = 0;
    z = 0;
    val = 0.0;
    return;
  }

  uint64 index = begin()->first;
  val = begin()->second.val;

  for (VI v = begin(); v != end(); v++) {
    if (v->second.val < val) {
      val = v->second.val;
      index = v->first;
    }
  }
  getxyz(index, x, y, z);
}

void VBRegion::merge(VBRegion &r) {
  for (VI v = r.begin(); v != r.end(); v++) add(v->first, v->second.val);
}

string VBRegion::info() {
  double vfactor = voxsizes[0] * voxsizes[1] * voxsizes[2];
  string regioninfo;
  double x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
  totalmass = 0;
  for (VI myvox = begin(); myvox != end(); myvox++) {
    x1 += myvox->second.x;
    y1 += myvox->second.y;
    z1 += myvox->second.z;
    x2 += myvox->second.x * myvox->second.val;
    y2 += myvox->second.y * myvox->second.val;
    z2 += myvox->second.z * myvox->second.val;
    totalmass += myvox->second.val;
  }
  x1 /= size();
  y1 /= size();
  z1 /= size();
  x2 /= totalmass;
  y2 /= totalmass;
  z2 /= totalmass;
  center.init(x1, y1, z1);
  wcenter.init(x2, y2, z2);

  VBRegion peakrr = maxregion();
  VBRegion minrr = minregion();
  VBRegion mnzr = minnonzeroregion();
  double pxx, pyy, pzz;
  peakrr.GeometricCenter(pxx, pyy, pzz);
  peakcenter.init(pxx, pyy, pzz);
  regioninfo += (format("  voxels: %d\n") % (size())).str();
  if (vfactor > FLT_MIN)
    regioninfo += (format("  mm3: %g\n") % (vfactor * size())).str();
  regioninfo += (format("  sum: %g\n") % totalmass).str();
  regioninfo += (format("  mean: %g\n") % (totalmass / (double)size())).str();
  regioninfo += (format("  center: %g,%g,%g\n") % x1 % y1 % z1).str();
  regioninfo += (format("  weighted ctr: %g,%g,%g\n") % x2 % y2 % z2).str();
  regioninfo +=
      (format("  peakval: %g\n") % (peakrr.begin()->second.val)).str();
  regioninfo += (format("  peakcnt: %d\n") % (peakrr.size())).str();
  regioninfo += (format("  minval: %g\n") % (minrr.begin()->second.val)).str();
  regioninfo += (format("  mincnt: %d\n") % (minrr.size())).str();
  regioninfo += (format("  peak center: %g,%g,%g\n") % pxx % pyy % pzz).str();
  return regioninfo;
}

bool vcompare(VBVoxel x, VBVoxel y) { return x.val < y.val; }

int poscomp(VBVoxel &v1, VBVoxel &v2) {
  if (v1.z < v2.z) return -1;
  if (v1.z > v2.z) return 1;

  if (v1.y < v2.y) return -1;
  if (v1.y > v2.y) return 1;

  if (v1.x < v2.x) return -1;
  if (v1.x > v2.x) return 1;
  return 0;
}

int VBImage::inbounds(int x, int y, int z) const {
  if (x < 0 || y < 0 || z < 0) return 0;
  if (x > dimx - 1 || y > dimy - 1 || z > dimz - 1) return 0;
  return 1;
}

void VBImage::init() {
  dimx = dimy = dimz = dimt = 0;
  voxsize[0] = voxsize[1] = voxsize[2] = 0.0;
  voxsize[3] = 1000.0;
  voxels = 0;
  offset = 0;
  scl_slope = scl_inter = 0.0;
  f_scaled = 0;
  origin[0] = origin[1] = origin[2] = 0;
  orient = "RPI";
  filebyteorder = ENDIAN_BIG;
  filename = "";
  datasize = 0;
  header.clear();
  header_valid = 0;
  data_valid = 0;
  f_mirrored = 0;
  subvolume = -1;
  init_nifti();
}

// init nifti-specific fields

void VBImage::init_nifti() {
  qform_code = sform_code = 0;
  qoffset[0] = qoffset[1] = qoffset[2] = 0.0;
  quatern_b = quatern_c = quatern_d = 0.0;
  srow_x[0] = srow_x[1] = srow_x[2] = srow_x[3] = 0.0;
  srow_y[0] = srow_y[1] = srow_y[2] = srow_y[3] = 0.0;
  srow_z[0] = srow_z[1] = srow_z[2] = srow_z[3] = 0.0;
  srow_x[0] = 1.0;
  srow_y[1] = 1.0;
  srow_z[2] = 1.0;
}

void VBImage::setVoxSizes(float x, float y, float z, float t) {
  if (x >= 0.0) voxsize[0] = x;
  if (y >= 0.0) voxsize[1] = y;
  if (z >= 0.0) voxsize[2] = z;
  if (t >= 0.0) voxsize[3] = t;
}

void VBImage::addMaskSpec(int index, int r, int g, int b, string name) {
  VBMaskSpec ms;
  ms.r = r;
  ms.g = g;
  ms.b = b;
  ms.name = name;
  maskspecs[index] = ms;
}

bool VBImage::dimsequal(const VBImage &im) {
  if (dimx != im.dimx) return 0;
  if (dimy != im.dimy) return 0;
  if (dimz != im.dimz) return 0;
  if (dimt != im.dimt && dimt && im.dimt) return 0;
  return 1;
}

/*********************************************************************
 * This function calculates the (x, y, z) coordinates corresponding to*
 * the input Tes mask index, i. The (x, y, z) coordinates are         *
 * assigned to the x, y, and z fields of the input struct maskCoord.  *
 *********************************************************************/
void setMaskCoords(MaskXYZCoord *maskCoord, unsigned int i, unsigned int dimX,
                   unsigned int dimY, unsigned int) {
  /*********************************************************************
   * Now calculating the X coordinate.                                  *
   *********************************************************************/
  maskCoord->x = i % dimX;

  /*********************************************************************
   * Now calculating the Y coordinate.                                  *
   *********************************************************************/
  maskCoord->y = ((i - maskCoord->x) / dimX) % dimY;

  /*********************************************************************
   * Now calculating the Z coordinate. The formula for calculating the  *
   * Z coordinate comes from:                                           *
   *    i = (dimX * dimY * z) + (dimX * y) + x                          *
   *********************************************************************/
  maskCoord->z = ((i - maskCoord->x) - (dimX * maskCoord->y)) / (dimX * dimY);

}  // void setMaskCoords(MaskXYZCoord *maskCoord, unsigned int i, unsigned int
   // dimX, unsigned int dimY, unsigned int dimZ)

/*********************************************************************
 * This function creates a tab delimited string object from the       *
 * variable part of the parameter list passed to this function and    *
 * adds the string object to input VBImage object's header data       *
 * member. NOTE: If the number of variable type specifiers, i.e.,     *
 * the length of *format, does not equal the actual number of items   *
 * in the variable number of parameters, then the behavior of the     *
 * function is undefined. The behavior is also undefined when a type  *
 * specifier does not match the type of its corresponding variable.   *
 * Each of the following examples results in undefined behavior of    *
 * this function:                                                     *
 *                                                                    *
 * bool b;                                                            *
 * char c;                                                            *
 * addHeaderLine(myTes, "bb", b, c);                                  *
 * addHeaderLine(myTes, "bcc", b, c);                                 *
 * addHeaderLine(myTes, "b", b, c);                                   *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * vb                 VBImage *       The input VBImage object.       *
 * format             const char *    A C-style string that specifies *
 *                                    the types of the remaining      *
 *                                    parameters.                     *
 * ...                The variable set of parameters.                 *
 *                                                                    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void addHeaderLine(VBImage *vb, const char *format, ...) {
  /*********************************************************************
   * line is used to build up the string object that will be added to   *
   * the header data member.                                            *
   *********************************************************************/
  string line;

  /*********************************************************************
   * tabString[] is a C-style string used to hold the tab character.    *
   * The tab character is the delimiter for each line in the VBImage    *
   * object's header.                                                   *
   *********************************************************************/
  char tabString[2];
  tabString[0] = '\t';
  tabString[1] = 0;

  /*********************************************************************
   * Declaring and clearing tempLine[]. This C-style string is used to  *
   * temporarily contain each field of the header line. tempLine[] will *
   * always be populated by on of the parameters from the variable list *
   * of parameters passed to this function.                             *
   *********************************************************************/
  char tempLine[TIME_STR_LEN];
  memset(tempLine, 0, TIME_STR_LEN);

  /*********************************************************************
   * The variable ap (of type va_list) is used to step through the      *
   * variable list of parameters.                                       *
   *********************************************************************/
  va_list ap;

  /*********************************************************************
   * The macro va_start() is sued to begin the process of stepping      *
   * through the variable list of parameters. NOTE: The second argument *
   * to the va_start() macro must be the last known variable passed to  *
   * this function, which is the variable format.                       *
   *********************************************************************/
  va_start(ap, format);

  /*********************************************************************
   * varParamIndex is declared and initialized to 1. varParamIndex is   *
   * employed to index the variable list of parameters.                 *
   *********************************************************************/
  int varParamIndex = 1;

  /*********************************************************************
   * formatLen holds the length of the format variable passed to this   *
   * function.                                                          *
   *********************************************************************/
  int formatLen = string(format).length();

  /*********************************************************************
   * The following while loop is used to traverse the variable list of  *
   * parameters passed to this function. breakLoop will be set to true  *
   * when the last variable type specifier is encountered in *format.   *
   *********************************************************************/
  bool breakLoop = false;
  while (*format) {
    /*********************************************************************
     * If the next character from the current character pointed to by     *
     * format is '\0', then we have no more parameters to process.        *
     * Therefore, we set breakLoop to true.                               *
     *********************************************************************/
    if (*(format + 1) == 0) {
      breakLoop = true;
    }  // if

    /*********************************************************************
     * The follwoing switch block is used to process each parameter after *
     * it is "picked off" by the va_arg() macro. Each of the "case"       *
     * blocks corresponds to one of the following variable type           *
     * specifiers:                                                        *
     *                                                                    *
     *  b ==> bool                                                        *
     *  c ==> char                                                        *
     *  C ==> unsigned char                                               *
     *  s ==> short                                                       *
     *  u ==> unsigned short                                              *
     *  i ==> int                                                         *
     *  I ==> unsigned int                                                *
     *  l ==> long                                                        *
     *  L ==> unsigned long                                               *
     *  f ==> float                                                       *
     *  d ==> double                                                      *
     *  S ==> char * (A C-style string)                                   *
     *********************************************************************/
    switch (*format++) {
        /*********************************************************************
         * Case for bool type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the bool variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'b':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, bool));                         *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, bool)". Therefore,     *
         * "va_arg(ap, bool)" was changed to "(bool ) va_arg(ap, int)".       *
         *********************************************************************/
        sprintf(tempLine, "%d", (bool)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for char type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the char variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'c':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%c", va_arg(ap, char));                         *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, char)". Therefore,     *
         * "va_arg(ap, char)" was changed to "(char ) va_arg(ap, int)".       *
         *********************************************************************/
        sprintf(tempLine, "%c", (char)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned char type variables. tempLine[] is cleared and   *
         * then populated by a C-style string representation of the unsigned  *
         * char variable, returned by the va_arg() macro. The first argument  *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'C':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, unsigned char));                *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, unsigned char)".       *
         * Therefore, "va_arg(ap, unsigned char)" was changed to              *
         * "(unsigned char ) va_arg(ap, int)".                                *
         *********************************************************************/
        sprintf(tempLine, "%c", (unsigned char)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for short type variables. tempLine[] is cleared and then      *
         * populated by a C-style string representation of the short variable,*
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 's':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, short));                        *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, short)". Therefore,    *
         * "va_arg(ap, short)" was changed to "(short ) va_arg(ap, int)".     *
         *********************************************************************/
        sprintf(tempLine, "%d", (short)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned short type variables. tempLine[] is cleared and  *
         * then populated by a C-style string representation of the unsigned  *
         * short variable, returned by the va_arg() macro. The first argument *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'u':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, unsigned short));               *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, unsigned short)".      *
         * Therefore, "va_arg(ap, unsigned short)" was changed to             *
         * "(unsigned short ) va_arg(ap, int)".                               *
         *********************************************************************/
        sprintf(tempLine, "%u", (unsigned short)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for int type variables. tempLine[] is cleared and then        *
         * populated by a C-style string representation of the int variable,  *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'i':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%d", va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned int type variables. tempLine[] is cleared and    *
         * then populated by a C-style string representation of the unsigned  *
         * int variable, returned by the va_arg() macro. The first argument   *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'I':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%d", va_arg(ap, unsigned int));
        break;

        /*********************************************************************
         * Case for long type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the long variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'l':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%ld", va_arg(ap, long));
        break;

        /*********************************************************************
         * Case for unsigned long type variables. tempLine[] is cleared and   *
         * then populated by a C-style string representation of the unsigned  *
         * long variable, returned by the va_arg() macro. The first argument  *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'L':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%lu", va_arg(ap, unsigned long));
        break;

        /*********************************************************************
         * Case for float type variables. tempLine[] is cleared and then      *
         * populated by a C-style string representation of the float variable,*
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable. NOTE: The second argument passed to va_arg() is   *
         * actually "double" when it should be "float". However, using "float"*
         * in this case resulted in 0.0 always being returned by va_arg(). To *
         * work around this bug, the type double is used in va_arg() and then *
         * the subsequent type of the return value from va_arg() is type cast *
         * to a float.                                                        *
         *********************************************************************/
      case 'f':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%g", (float)va_arg(ap, double));
        break;

        /*********************************************************************
         * Case for double type variables. tempLine[] is cleared and then     *
         * populated by a C-style string representation of the double         *
         * variable, returned by the va_arg() macro. The first argument to    *
         * va_arg() must always be of type va_list and the second argument is *
         * the type of the variable.                                          *
         *********************************************************************/
      case 'd':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%f", va_arg(ap, double));
        break;

        /*********************************************************************
         * Case for char * type variables. tempLine[] is cleared and then     *
         * populated by a C-style string representation of the char *         *
         * variable, returned by the va_arg() macro. The first argument to    *
         * va_arg() must always be of type va_list and the second argument is *
         * the type of the variable.                                          *
         *********************************************************************/
      case 'S':
        memset(tempLine, 0, TIME_STR_LEN);
        strcpy(tempLine, va_arg(ap, char *));
        break;

        /*********************************************************************
         * If we encounter an unexpected variable type specifier, then an     *
         * error message is printed and then this program exits.              *
         *********************************************************************/
      default:
        cerr << "ERROR: Unrecognized variable type specifier: ["
             << (*(--format)) << "]" << endl;
        exit(1);
        break;

    }  // switch

    /*********************************************************************
     * Concatenating tempLine[] to line.                                  *
     *********************************************************************/
    line += string(tempLine);

    /*********************************************************************
     * If we are not at the last element to add to line, then concatenate *
     * tabString[] to line.                                               *
     *********************************************************************/
    if (varParamIndex != formatLen) {
      line += string(tabString);
    }  // if

    /*********************************************************************
     * Incrementing varParamIndex.                                        *
     *********************************************************************/
    varParamIndex++;

    /*********************************************************************
     * If breakLoop is set to true, then we break out of this while loop. *
     *********************************************************************/
    if (breakLoop) {
      break;
    }  // if

  }  // while

  /*********************************************************************
   * Now "closing up" the variable list of parameters passed to this    *
   * function.                                                          *
   *********************************************************************/
  va_end(ap);

  /*********************************************************************
   * Now adding line to the header data member of vb.                   *
   *********************************************************************/
  vb->AddHeader(line);

}  // void addHeaderLine(VBImage *vb, const char *format, ...)

/*********************************************************************
 * This function creates a tab delimited string object from the       *
 * variable part of the parameter list passed to this function and    *
 * adds the string object to input VBImage object's header data       *
 * member. NOTE: If the number of variable type specifiers, i.e.,     *
 * the length of *format, does not equal the actual number of items   *
 * in the variable number of parameters, then the behavior of the     *
 * function is undefined. The behavior is also undefined when a type  *
 * specifier does not match the type of its corresponding variable.   *
 * Each of the following examples results in undefined behavior of    *
 * this function:                                                     *
 *                                                                    *
 * bool b;                                                            *
 * char c;                                                            *
 * addHeaderLine(myTes, "bb", b, c);                                  *
 * addHeaderLine(myTes, "bcc", b, c);                                 *
 * addHeaderLine(myTes, "b", b, c);                                   *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * vb                 VBImage&        The input VBImage object.       *
 * format             const char *    A C-style string that specifies *
 *                                    the types of the remaining      *
 *                                    parameters.                     *
 * ...                The variable set of parameters.                 *
 *                                                                    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
 * -----------------   -----           ------------                   *
 * None.                                                              *
 *********************************************************************/
void addHeaderLine(VBImage &vb, const char *format, ...) {
  /*********************************************************************
   * line is used to build up the string object that will be added to   *
   * the header data member.                                            *
   *********************************************************************/
  string line;

  /*********************************************************************
   * tabString[] is a C-style string used to hold the tab character.    *
   * The tab character is the delimiter for each line in the VBImage    *
   * object's header.                                                   *
   *********************************************************************/
  char tabString[2];
  tabString[0] = '\t';
  tabString[1] = 0;

  /*********************************************************************
   * Declaring and clearing tempLine[]. This C-style string is used to  *
   * temporarily contain each field of the header line. tempLine[] will *
   * always be populated by on of the parameters from the variable list *
   * of parameters passed to this function.                             *
   *********************************************************************/
  char tempLine[TIME_STR_LEN];
  memset(tempLine, 0, TIME_STR_LEN);

  /*********************************************************************
   * The variable ap (of type va_list) is used to step through the      *
   * variable list of parameters.                                       *
   *********************************************************************/
  va_list ap;

  /*********************************************************************
   * The macro va_start() is sued to begin the process of stepping      *
   * through the variable list of parameters. NOTE: The second argument *
   * to the va_start() macro must be the last known variable passed to  *
   * this function, which is the variable format.                       *
   *********************************************************************/
  va_start(ap, format);

  /*********************************************************************
   * varParamIndex is declared and initialized to 1. varParamIndex is   *
   * employed to index the variable list of parameters.                 *
   *********************************************************************/
  int varParamIndex = 1;

  /*********************************************************************
   * formatLen holds the length of the format variable passed to this   *
   * function.                                                          *
   *********************************************************************/
  int formatLen = string(format).length();

  /*********************************************************************
   * The following while loop is used to traverse the variable list of  *
   * parameters passed to this function. breakLoop will be set to true  *
   * when the last variable type specifier is encountered in *format.   *
   *********************************************************************/
  bool breakLoop = false;
  while (*format) {
    /*********************************************************************
     * If the next character from the current character pointed to by     *
     * format is '\0', then we have no more parameters to process.        *
     * Therefore, we set breakLoop to true.                               *
     *********************************************************************/
    if (*(format + 1) == 0) {
      breakLoop = true;
    }  // if

    /*********************************************************************
     * The follwoing switch block is used to process each parameter after *
     * it is "picked off" by the va_arg() macro. Each of the "case"       *
     * blocks corresponds to one of the following variable type           *
     * specifiers:                                                        *
     *                                                                    *
     *  b ==> bool                                                        *
     *  c ==> char                                                        *
     *  C ==> unsigned char                                               *
     *  s ==> short                                                       *
     *  u ==> unsigned short                                              *
     *  i ==> int                                                         *
     *  I ==> unsigned int                                                *
     *  l ==> long                                                        *
     *  L ==> unsigned long                                               *
     *  f ==> float                                                       *
     *  d ==> double                                                      *
     *  S ==> char * (A C-style string)                                   *
     *********************************************************************/
    switch (*format++) {
        /*********************************************************************
         * Case for bool type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the bool variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'b':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, bool));                         *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, bool)". Therefore,     *
         * "va_arg(ap, bool)" was changed to "(bool ) va_arg(ap, int)".       *
         *********************************************************************/
        sprintf(tempLine, "%d", (bool)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for char type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the char variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'c':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%c", va_arg(ap, char));                         *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, char)". Therefore,     *
         * "va_arg(ap, char)" was changed to "(char ) va_arg(ap, int)".       *
         *********************************************************************/
        sprintf(tempLine, "%c", (char)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned char type variables. tempLine[] is cleared and   *
         * then populated by a C-style string representation of the unsigned  *
         * char variable, returned by the va_arg() macro. The first argument  *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'C':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, unsigned char));                *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, unsigned char)".       *
         * Therefore, "va_arg(ap, unsigned char)" was changed to              *
         * "(unsigned char ) va_arg(ap, int)".                                *
         *********************************************************************/
        sprintf(tempLine, "%c", (unsigned char)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for short type variables. tempLine[] is cleared and then      *
         * populated by a C-style string representation of the short variable,*
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 's':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, short));                        *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, short)". Therefore,    *
         * "va_arg(ap, short)" was changed to "(short ) va_arg(ap, int)".     *
         *********************************************************************/
        sprintf(tempLine, "%d", (short)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned short type variables. tempLine[] is cleared and  *
         * then populated by a C-style string representation of the unsigned  *
         * short variable, returned by the va_arg() macro. The first argument *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'u':
        memset(tempLine, 0, TIME_STR_LEN);

        /*********************************************************************
         * Originally, the following line of code was:                        *
         *                                                                    *
         * sprintf(tempLine, "%d", va_arg(ap, unsigned short));               *
         *                                                                    *
         * However, gcc 2.96 does not like "va_arg(ap, unsigned short)".      *
         * Therefore, "va_arg(ap, unsigned short)" was changed to             *
         * "(unsigned short ) va_arg(ap, int)".                               *
         *********************************************************************/
        sprintf(tempLine, "%u", (unsigned short)va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for int type variables. tempLine[] is cleared and then        *
         * populated by a C-style string representation of the int variable,  *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'i':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%d", va_arg(ap, int));
        break;

        /*********************************************************************
         * Case for unsigned int type variables. tempLine[] is cleared and    *
         * then populated by a C-style string representation of the unsigned  *
         * int variable, returned by the va_arg() macro. The first argument   *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'I':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%d", va_arg(ap, unsigned int));
        break;

        /*********************************************************************
         * Case for long type variables. tempLine[] is cleared and then       *
         * populated by a C-style string representation of the long variable, *
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable.                                                   *
         *********************************************************************/
      case 'l':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%ld", va_arg(ap, long));
        break;

        /*********************************************************************
         * Case for unsigned long type variables. tempLine[] is cleared and   *
         * then populated by a C-style string representation of the unsigned  *
         * long variable, returned by the va_arg() macro. The first argument  *
         * to va_arg() must always be of type va_list and the second argument *
         * is the type of the variable.                                       *
         *********************************************************************/
      case 'L':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%lu", va_arg(ap, unsigned long));
        break;

        /*********************************************************************
         * Case for float type variables. tempLine[] is cleared and then      *
         * populated by a C-style string representation of the float variable,*
         * returned by the va_arg() macro. The first argument to va_arg()     *
         * must always be of type va_list and the second argument is the type *
         * of the variable. NOTE: The second argument passed to va_arg() is   *
         * actually "double" when it should be "float". However, using "float"*
         * in this case resulted in 0.0 always being returned by va_arg(). To *
         * work around this bug, the type double is used in va_arg() and then *
         * the subsequent type of the return value from va_arg() is type cast *
         * to a float.                                                        *
         *********************************************************************/
      case 'f':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%g", (float)va_arg(ap, double));
        break;

        /*********************************************************************
         * Case for double type variables. tempLine[] is cleared and then     *
         * populated by a C-style string representation of the double         *
         * variable, returned by the va_arg() macro. The first argument to    *
         * va_arg() must always be of type va_list and the second argument is *
         * the type of the variable.                                          *
         *********************************************************************/
      case 'd':
        memset(tempLine, 0, TIME_STR_LEN);
        sprintf(tempLine, "%f", va_arg(ap, double));
        break;

        /*********************************************************************
         * Case for char * type variables. tempLine[] is cleared and then     *
         * populated by a C-style string representation of the char *         *
         * variable, returned by the va_arg() macro. The first argument to    *
         * va_arg() must always be of type va_list and the second argument is *
         * the type of the variable.                                          *
         *********************************************************************/
      case 'S':
        memset(tempLine, 0, TIME_STR_LEN);
        strcpy(tempLine, va_arg(ap, char *));
        break;

        /*********************************************************************
         * If we encounter an unexpected variable type specifier, then an     *
         * error message is printed and then this program exits.              *
         *********************************************************************/
      default:
        cerr << "ERROR: Unrecognized variable type specifier: ["
             << (*(--format)) << "]" << endl;
        exit(1);
        break;

    }  // switch

    /*********************************************************************
     * Concatenating tempLine[] to line.                                  *
     *********************************************************************/
    line += string(tempLine);

    /*********************************************************************
     * If we are not at the last element to add to line, then concatenate *
     * tabString[] to line.                                               *
     *********************************************************************/
    if (varParamIndex != formatLen) {
      line += string(tabString);
    }  // if

    /*********************************************************************
     * Incrementing varParamIndex.                                        *
     *********************************************************************/
    varParamIndex++;

    /*********************************************************************
     * If breakLoop is set to true, then we break out of this while loop. *
     *********************************************************************/
    if (breakLoop) {
      break;
    }  // if

  }  // while

  /*********************************************************************
   * Now "closing up" the variable list of parameters passed to this    *
   * function.                                                          *
   *********************************************************************/
  va_end(ap);

  /*********************************************************************
   * Now adding line to the header data member of vb.                   *
   *********************************************************************/
  vb.AddHeader(line);

}  // void addHeaderLine(VBImage *vb, const char *format, ...)

bool validate4DFile(const string tesFile) {
  vector<VBFF> listoftypes = EligibleFileTypes(tesFile);
  if (listoftypes.size() == 0) {
    return false;
  }  // if

  return (listoftypes[0].getDimensions() == 4);

  //  return 1;*/
  // FIXME : DYK : functionality gutted by dan!  may or may not need
  // to be re-implemented
}

/*********************************************************************
 * This function copies the header lines from the src VBImage object  *
 * to the header of the dest VBImage object.                          *
 *********************************************************************/
void copyHeader(const VBImage *src, VBImage *dest) {
  for (int i = 0; i < (int)src->header.size(); i++) {
    dest->AddHeader(src->header[i]);
  }  // for i
}  // void copyHeader(const VBImage *src, VBImage *dest)

/*********************************************************************
 * This function copies the header lines from the src VBImage object  *
 * to the header of the dest VBImage object.                          *
 *********************************************************************/
void copyHeader(const VBImage &src, VBImage &dest) {
  for (int i = 0; i < (int)src.header.size(); i++) {
    dest.AddHeader(src.header[i]);
  }  // for i
}  // void copyHeader(const VBImage& src, VBImage& dest)
