
// cube.cpp
// VoxBo Cube class
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

#include <fstream>
#include "vbio.h"
#include "vbutil.h"

Cube::Cube() {
  data = (unsigned char *)NULL;
  init();
}

Cube::Cube(int in_dimx, int in_dimy, int in_dimz, VB_datatype in_type) {
  data = (unsigned char *)NULL;
  init(in_dimx, in_dimy, in_dimz, in_type);
}

Cube::~Cube() { invalidate(); }

Cube::Cube(const Cube &cb) : VBImage(cb) {
  data = (unsigned char *)NULL;
  init();
  *this = cb;
}

Cube::Cube(VBRegion &rr) {
  data = (unsigned char *)NULL;
  init();
  SetVolume(rr.dimx, rr.dimy, rr.dimz, vb_byte);
  for (VI v = rr.begin(); v != rr.end(); v++) {
    setValue<char>(v->second.x, v->second.y, v->second.z, 1);
  }
}

void Cube::init() {
  VBImage::init();
  if (data && !f_mirrored) delete[] data;
  f_mirrored = 0;
  data = (unsigned char *)NULL;
  datasize = offset = voxels = 0;
  minval = 1;
  maxval = 0;  // invalid values, forces min and max to be calculated
}

int Cube::init(int in_dimx, int in_dimy, int in_dimz, VB_datatype in_type) {
  init();
  return (SetVolume(in_dimx, in_dimy, in_dimz, in_type));
}

// the following is indeed used in a few places

Cube::Cube(Cube *old) {
  data = (unsigned char *)NULL;
  init();
  *this = *old;
}

int Cube::SetVolume(const Cube &cb) {
  return SetVolume(cb.dimx, cb.dimy, cb.dimz, cb.datatype);
}

int Cube::SetVolume(const Cube &cb, VB_datatype dt) {
  return SetVolume(cb.dimx, cb.dimy, cb.dimz, dt);
}

int Cube::SetVolume(uint32 in_dimx, uint32 in_dimy, uint32 in_dimz,
                    VB_datatype in_type) {
  if (data && !f_mirrored) delete[] data;
  if (in_dimx > MAX_DIM || in_dimy > MAX_DIM || in_dimz > MAX_DIM) return 101;
  f_mirrored = 0;
  dimx = in_dimx;
  dimy = in_dimy;
  dimz = in_dimz;
  SetDataType(in_type);
  voxels = dimx * dimy * dimz;
  header_valid = 1;
  data = new unsigned char[dimx * dimy * dimz * datasize];
  zero();
  if (data) data_valid = 1;
  return 0;
}

int Cube::ReadFile(const string &fname) {
  int err;
  err = ReadHeader(fname);
  if (err) return err;
  err = ReadData(filename);  // reading the header may have changed filename
  if (err) return err;
  return 0;
}

int Cube::ReadHeader(const string &fname) {
  if (fname.size() == 0) return 104;
  // preserve dimensions across init just in case (e.g., for mricro roi files)
  int tmpx = dimx;
  int tmpy = dimy;
  int tmpz = dimz;
  init();
  dimx = tmpx;
  dimy = tmpy;
  dimz = tmpz;
  filename = fname;
  // reparsefilename will get dims for files that don't specify them,
  // and will handle fname:0 syntax
  ReparseFileName();
  if (subvolume > -1) {
    Tes ts;
    ts.filename = filename;
    vector<VBFF> ftypes = EligibleFileTypes(filename, 4);
    if (ftypes.size() == 0) return 101;
    fileformat = ftypes[0];
    if (!fileformat.read_head_4D || !fileformat.read_vol_4D) return 102;
    int err = fileformat.read_head_4D(&ts);
    dimx = ts.dimx;
    dimy = ts.dimy;
    dimz = ts.dimz;
    datatype = ts.datatype;
    // copy the tes header but preserve the subvolume id
    int32 sv = subvolume;
    CopyHeader(ts);
    subvolume = sv;
    return err;
  } else if (subvolume == -2) {  // i.e., foo.tes:mask
    Tes ts;
    ts.filename = filename;
    vector<VBFF> ftypes = EligibleFileTypes(filename, 4);
    if (ftypes.size() == 0) return 101;
    fileformat = ftypes[0];
    if (!fileformat.read_head_4D) return 102;
    int err = fileformat.read_head_4D(&ts);
    if (err) return 105;
    dimx = ts.dimx;
    dimy = ts.dimy;
    dimz = ts.dimz;
    datatype = vb_byte;
    // copy the tes header but preserve the subvolume id
    int32 sv = subvolume;
    CopyHeader(ts);
    subvolume = sv;
    return err;
  }
  vector<VBFF> ftypes = EligibleFileTypes(fname, 3);
  if (ftypes.size() == 0) return 101;

  // FIXME on error we could be nice and try multiple types
  fileformat = ftypes[0];
  if (!fileformat.read_head_3D) return 102;
  int err = fileformat.read_head_3D(this);
  if (!err) ReadLabels();  // ignore errors here
  return err;
}

int Cube::ReadData(const string &fname) {
  filename = fname;
  data_valid = 0;
  int err;
  if (subvolume > -1) {
    Tes ts;
    err = ts.ReadHeader(filename);
    if (err) return err;
    err = fileformat.read_vol_4D(ts, *this, subvolume);
    return err;
  } else if (subvolume == -2) {  // i.e., foo.tes:mask
    Tes ts;
    if (fileformat.signature == "tes1")
      err = ts.ReadHeader(filename);
    else
      err = ts.ReadFile(filename);
    if (err) return err;
    ts.ExtractMask(*this);
    return 0;
  }
  if (!header_valid) {
    if ((err = ReadHeader(fname))) return err;
  }
  if (!fileformat.read_data_3D) return 102;
  err = fileformat.read_data_3D(this);
  return err;
}

// independent of file format, if we find a file called fname.txt
// (maybe with .gz stripped), see if we can read labels

int Cube::ReadLabels() {
  tcolor tc;
  // clist.push_back(tcolor(255,0,0));
  // clist.push_back(tcolor(0,255,0));
  // c/list.push_back(tcolor(0,0,255));
  // clist.push_back(tcolor(210,210,0));
  // clist.push_back(tcolor(0,210,210));
  // clist.push_back(tcolor(210,0,210));

  string lname;
  if (vb_fileexists(filename + ".txt"))
    lname = filename + ".txt";
  else if (vb_fileexists(xsetextension(filename, "") + ".txt"))
    lname = xsetextension(filename, "") + ".txt";
  else
    return 1;
  ifstream infile;
  infile.open(lname.c_str());
  if (infile.fail()) return 2;
  char line[STRINGLEN];
  tokenlist args;
  pair<bool, int32> mid;
  while (!infile.eof()) {
    infile.getline(line, STRINGLEN);
    args.ParseLine(line);
    if (!args.size()) continue;
    // if a line has anything, it should have both a number and a label
    if (args.size() == 1) {
      infile.close();
      return 7;
    }
    // make sure first token is completely parseable as an integer
    mid = strtolx(args[0]);
    if (mid.first) {  // parse error
      infile.close();
      return 3;
    }
    VBMaskSpec mm;
    mm.r = tc.r;
    mm.g = tc.g;
    mm.b = tc.b;
    tc.next();
    mm.name = args.Tail();
    maskspecs[mid.second] = mm;
  }
  infile.close();
  return 0;
}

int Cube::WriteFile(const string fname) {
  VBFF original;
  // save the original format, then null it
  original = fileformat;
  fileformat.init();
  if (fname.size()) filename = fname;
  ReparseFileName();
  // if reparse didn't find anything, try by extension
  if (!fileformat.write_3D) fileformat = findFileFormat(filename, 3);
  // if not, try original file's format
  if (!fileformat.write_3D) fileformat = original;
  // if not, try cub1
  if (!fileformat.write_3D) fileformat = findFileFormat("cub1");
  // if not (should never happen), bail
  if (!fileformat.write_3D) return 200;
  int err = fileformat.write_3D(this);
  return err;
}

void Cube::resize(int x, int y, int z) {
  dimx = x;
  dimy = y;
  dimz = z;
  if (data && !f_mirrored) delete[] data;
  f_mirrored = 0;
  data = (unsigned char *)NULL;
  voxels = dimx * dimy * dimz;
  data = new unsigned char[voxels * datasize];
  if (!data) data_valid = 0;
}

void Cube::print() const { cout << *this; }

ostream &operator<<(ostream &os, const Cube &cb) {
  os << endl
     << "+- 3D Image file " << xfilename(cb.GetFileName()) << " ("
     << cb.fileformat.getName() << ")"
     << " (" << DataTypeName(cb.f_scaled ? cb.altdatatype : cb.datatype)
     << (cb.f_scaled ? ", scaled)" : ")") << endl;
  if (!cb.header_valid) {
    os << "+- invalid 3D data\n";
    // return os;
  }
  if (xdirname(cb.GetFileName()) != ".")
    os << "| path: " << xdirname(cb.GetFileName()) << "/" << endl;
  os << "| " << cb.dimx << "x" << cb.dimy << "x" << cb.dimz << " voxels"
     << endl;
  os.setf(ios::fixed, ios::floatfield);
  os.precision(4);
  os << "| " << cb.voxsize[0] << " x " << cb.voxsize[1] << " x "
     << cb.voxsize[2] << " mm" << endl;
  os.precision(1);
  os << "| " << cb.meglen() << "MB on disk ("
     << (cb.filebyteorder == ENDIAN_BIG ? "msbfirst" : "lsbfirst") << ")"
     << endl;
  os << "| origin: (" << cb.origin[0] << "," << cb.origin[1] << ","
     << cb.origin[2] << ")" << endl;
  os.precision(2);
  if (cb.voxsize[0] > FLT_MIN && cb.voxsize[1] > FLT_MIN &&
      cb.voxsize[1] > FLT_MIN) {
    string tmp =
        (format("[%g,%g,%g;%g,%g,%g]") % (cb.voxsize[0] * cb.origin[0]) %
         (cb.voxsize[1] * cb.origin[1]) % (cb.voxsize[2] * cb.origin[2]) %
         (cb.voxsize[0] * (cb.dimx - cb.origin[0] - 1)) %
         (cb.voxsize[1] * (cb.dimy - cb.origin[1] - 1)) %
         (cb.voxsize[2] * (cb.dimz - cb.origin[2] - 1)))
            .str();
    os << "| bounding box: " << tmp << endl;
  }
  os.precision(4);
  if (cb.f_scaled)
    os << "| slope: " << cb.scl_slope << ","
       << "intercept: " << cb.scl_inter << endl;
  if (cb.header.size() > 0) {
    os << "+--user header----------" << endl;
    for (int i = 0; i < (int)cb.header.size(); i++)
      os << "| " << cb.header[i] << endl;
  }
  os << "+-----------------------" << endl;
  return os;
}

void Cube::printbrief(const string &flags) const {
  string myflags = flags;
  if (!myflags.size()) myflags = "fdvt";

  cout << GetFileName();
  for (int i = 0; i < (int)myflags.size(); i++) {
    if (i == 0)
      cout << ": ";
    else
      cout << ", ";
    if (myflags[i] == 'f')
      cout << "(" << fileformat.getName() << ")";
    else if (myflags[i] == 'd')
      cout << dimx << "x" << dimy << "x" << dimz;
    else if (myflags[i] == 'v')
      cout << voxsize[0] << "x" << voxsize[1] << "x" << voxsize[2] << "mm";
    else if (myflags[i] == 'o')
      cout << origin[0] << "x" << origin[1] << "x" << origin[2];
    else if (myflags[i] == 'r')
      cout << orient;
    else if (myflags[i] == 't')
      cout << "(" << DataTypeName(datatype) << (f_scaled ? ", scaled)" : ")");
  }
  cout << endl;
}

void Cube::zero() {
  if (!data) return;
  memset(data, 0, dimx * dimy * dimz * datasize);
  minval = 1;
  maxval = 0;  // invalid values, forces min and max to be calculated
}

void Cube::invalidate() {
  if (data && !f_mirrored) delete[] data;
  header.clear();
  data = (unsigned char *)NULL;
  data_valid = 0;
  header_valid = 0;
}

double Cube::GetValue(VBVoxel &v) const { return GetValue(v.x, v.y, v.z); }

double Cube::GetValue(int x, int y, int z) const {
  switch (datatype) {
    case vb_byte:
      return (double)getValueSafe<unsigned char>(x, y, z);
      break;
    case vb_short:
      return (double)getValueSafe<int16>(x, y, z);
      break;
    case vb_long:
      return (double)getValueSafe<int32>(x, y, z);
      break;
    case vb_float:
      return (double)getValueSafe<float>(x, y, z);
      break;
    case vb_double:
      return getValueSafe<double>(x, y, z);
      break;
  }
  exit(999);  // shouldn't happen, but still...
  return 0;
}

bool Cube::testValue(int x, int y, int z) const {
  switch (datatype) {
    case vb_byte:
      return testValueSafe<unsigned char>(x, y, z);
      break;
    case vb_short:
      return testValueSafe<int16>(x, y, z);
      break;
    case vb_long:
      return testValueSafe<int32>(x, y, z);
      break;
    case vb_float:
      return testValueSafe<float>(x, y, z);
      break;
    case vb_double:
      return testValueSafe<double>(x, y, z);
      break;
  }
  exit(999);  // shouldn't happen, but still...
  return 0;
}

bool Cube::testValue(int index) const {
  switch (datatype) {
    case vb_byte:
      return testValueUnsafe<char>(index);
      break;
    case vb_short:
      return testValueUnsafe<int16>(index);
      break;
    case vb_long:
      return testValueUnsafe<int32>(index);
      break;
    case vb_float:
      return testValueUnsafe<float>(index);
      break;
    case vb_double:
      return testValueUnsafe<double>(index);
      break;
  }
  exit(999);  // shouldn't happen, but still...
  return 0;
}

void Cube::SetValue(int x, int y, int z, double val) {
  if (x < 0 || y < 0 || z < 0) return;
  if (x > dimx - 1 || y > dimy - 1 || z > dimz - 1) return;
  unsigned char *ptr;
  int index = dimx * ((dimy * z) + y) + x;
  ptr = data + (index * datasize);
  switch (datatype) {
    case vb_byte:
      *((unsigned char *)ptr) = (unsigned char)round(val);
      break;
    case vb_short:
      *((int16 *)ptr) = (int16)round(val);
      break;
    case vb_long:
      *((int32 *)ptr) = (int32)round(val);
      break;
    case vb_float:
      *((float *)ptr) = (float)val;
      break;
    case vb_double:
      *((double *)ptr) = val;
      break;
  }
}

Cube &Cube::operator+=(const Cube &cb) {
  if (dimx != cb.dimx || dimy != cb.dimy || dimz != cb.dimz) {
    zero();
    return *this;
  }
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) + cb.getValue<unsigned char>(i));
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) + cb.getValue<int16>(i));
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) + cb.getValue<int32>(i));
        break;
      case vb_float:
        setValue(i, getValue<float>(i) + cb.getValue<float>(i));
        break;
      case vb_double:
        setValue(i, getValue<double>(i) + cb.getValue<double>(i));
        break;
    }
  }
  return *this;
}

Cube &Cube::operator-=(const Cube &cb) {
  if (dimx != cb.dimx || dimy != cb.dimy || dimz != cb.dimz) {
    zero();
    return *this;
  }
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) - cb.getValue<unsigned char>(i));
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) - cb.getValue<int16>(i));
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) - cb.getValue<int32>(i));
        break;
      case vb_float:
        setValue(i, getValue<float>(i) - cb.getValue<float>(i));
        break;
      case vb_double:
        setValue(i, getValue<double>(i) - cb.getValue<double>(i));
        break;
    }
  }
  return *this;
}

Cube &Cube::operator*=(const Cube &cb) {
  if (dimx != cb.dimx || dimy != cb.dimy || dimz != cb.dimz) {
    zero();
    return *this;
  }
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) * cb.getValue<unsigned char>(i));
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) * cb.getValue<int16>(i));
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) * cb.getValue<int32>(i));
        break;
      case vb_float:
        setValue(i, getValue<float>(i) * cb.getValue<float>(i));
        break;
      case vb_double:
        setValue(i, getValue<double>(i) * cb.getValue<double>(i));
        break;
    }
  }
  return *this;
}

Cube &Cube::operator/=(const Cube &cb) {
  if (dimx != cb.dimx || dimy != cb.dimy || dimz != cb.dimz) {
    zero();
    return *this;
  }
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) / cb.getValue<unsigned char>(i));
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) / cb.getValue<int16>(i));
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) / cb.getValue<int32>(i));
        break;
      case vb_float:
        setValue(i, getValue<float>(i) / cb.getValue<float>(i));
        break;
      case vb_double:
        setValue(i, getValue<double>(i) / cb.getValue<double>(i));
        break;
    }
  }
  return *this;
}

// FIXME the following 4 need to be templatized

Cube &Cube::operator+=(double num) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) + num);
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) + num);
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) + num);
        break;
      case vb_float:
        setValue(i, getValue<float>(i) + num);
        break;
      case vb_double:
        setValue(i, getValue<double>(i) + num);
        break;
    }
  }
  return *this;
}

Cube &Cube::operator-=(double num) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) - num);
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) - num);
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) - num);
        break;
      case vb_float:
        setValue(i, getValue<float>(i) - num);
        break;
      case vb_double:
        setValue(i, getValue<double>(i) - num);
        break;
    }
  }
  return *this;
}

Cube &Cube::operator*=(double num) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) * num);
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) * num);
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) * num);
        break;
      case vb_float:
        setValue(i, getValue<float>(i) * num);
        break;
      case vb_double:
        setValue(i, getValue<double>(i) * num);
        break;
    }
  }
  return *this;
}

Cube &Cube::operator/=(double num) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    switch (datatype) {
      case vb_byte:
        setValue(i, getValue<unsigned char>(i) / num);
        break;
      case vb_short:
        setValue(i, getValue<int16>(i) / num);
        break;
      case vb_long:
        setValue(i, getValue<int32>(i) / num);
        break;
      case vb_float:
        setValue(i, getValue<float>(i) / num);
        break;
      case vb_double:
        setValue(i, getValue<double>(i) / num);
        break;
    }
  }
  return *this;
}

Cube &Cube::operator=(double num) {
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        SetValue(i, j, k, num);
      }
    }
  }
  return *this;
}

// FIXME make sure all the relevant fields are copied here and in the
// corresponding operator in Tes

Cube &Cube::operator=(const Cube &cb) { return copycube(cb); }

Cube &Cube::copycube(const Cube &cb, bool mirrorflag) {
  init();
  if (!cb.header_valid) return *this;
  if (data && !f_mirrored) {
    delete[] data;
    data = (unsigned char *)NULL;
  }
  f_mirrored = 0;
  // copy header, then data
  CopyHeader(cb);
  dimx = cb.dimx;
  dimy = cb.dimy;
  dimz = cb.dimz;
  dimt = cb.dimt;  // not needed for cubes, but we might sometimes stash
                   // something here
  datasize = cb.datasize;
  datatype = cb.datatype;
  // altdatasize=cb.altdatasize;
  altdatatype = cb.altdatatype;
  voxels = cb.voxels;
  offset = cb.offset;
  data_valid = cb.data_valid;
  fileformat = cb.fileformat;
  // filename and data
  SetFileName(cb.GetFileName());
  data = (unsigned char *)NULL;
  if (!cb.data) return *this;
  // copy or mirrordata
  if (mirrorflag) {
    data = cb.data;
    f_mirrored = 1;
  } else {
    data = new unsigned char[voxels * datasize];
    memcpy(data, cb.data, voxels * datasize);
  }
  return *this;
}

int operator==(const Cube &c1, const Cube &c2) {
  int differences = 0;
  if (c1.voxels != c2.voxels || c1.datatype != c2.datatype) {
    return 0;
  }
  if (c1.data == c2.data) {
    return 1;
  }
  for (int i = 0; i < c1.voxels * c2.datasize; i++) {
    if (c1.data[i] != c2.data[i]) {
      differences++;
    }
  }
  if (differences == 0)
    return 1;
  else
    return 0;
}

// return length of data in MB

float Cube::meglen() const {
  if (header_valid) return (float)(voxels * datasize) / (1024 * 1024);
  return 0;
}

////////////////////////////////
// cube manipulation
////////////////////////////////

void Cube::byteswap() {
  if (!data) return;
  switch (datatype) {
    case vb_short:
      swap((int16 *)data, voxels);
      break;
    case vb_long:
      swap((int32 *)data, voxels);
      break;
    case vb_byte:
      // no action necessary!
      break;
    case vb_float:
      swap((float *)data, voxels);
      break;
    case vb_double:
      swap((double *)data, voxels);
      break;
  }
}

void Cube::flipx() {
  double oldval, newval;
  for (int i = 0; i < dimx / 2; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        oldval = GetValue(i, j, k);
        newval = GetValue(dimx - i - 1, j, k);
        SetValue(i, j, k, newval);
        SetValue(dimx - i - 1, j, k, oldval);
      }
    }
  }
}

void Cube::flipy() {
  double oldval, newval;
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy / 2; j++) {
      for (int k = 0; k < dimz; k++) {
        oldval = GetValue(i, j, k);
        newval = GetValue(i, dimy - j - 1, k);
        SetValue(i, j, k, newval);
        SetValue(i, dimy - j - 1, k, oldval);
      }
    }
  }
}

void Cube::flipz() {
  double oldval, newval;
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz / 2; k++) {
        oldval = GetValue(i, j, k);
        newval = GetValue(i, j, dimz - k - 1);
        SetValue(i, j, k, newval);
        SetValue(i, j, dimz - k - 1, oldval);
      }
    }
  }
}

void Cube::zero(int x1, int x2, int y1, int y2, int z1, int z2) {
  if (dimx < 1) return;
  if (dimy < 1) return;
  if (dimz < 1) return;
  // 0,0 means whole extent
  if (x1 == 0 && x2 == 0) x2 = dimx - 1;
  if (y1 == 0 && y2 == 0) y2 = dimy - 1;
  if (z1 == 0 && z2 == 0) z2 = dimz - 1;
  x1 = max(0, x1);
  x2 = min(dimx - 1, x2);
  y1 = max(0, y1);
  y2 = min(dimy - 1, y2);
  z1 = max(0, z1);
  z2 = min(dimz - 1, z2);
  for (int i = x1; i <= x2; i++) {
    for (int j = y1; j <= y2; j++) {
      for (int k = z1; k <= z2; k++) {
        setValue(i, j, k, 0);
      }
    }
  }
}

void Cube::leftify() { zero(((dimx + 1) / 2), dimx - 1, 0, 0, 0, 0); }

void Cube::rightify() { zero(0, ((dimx + 1) / 2) - 1, 0, 0, 0, 0); }

void Cube::thresh(double val) {
  double thisval;
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        thisval = GetValue(i, j, k);
        if (thisval <= val) SetValue(i, j, k, 0.0);
      }
    }
  }
}

void Cube::threshabs(double val) {
  double thisval;
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        thisval = GetValue(i, j, k);
        if (fabs(thisval) <= val) SetValue(i, j, k, 0.0);
      }
    }
  }
}

void Cube::cutoff(double val) {
  double thisval;
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        thisval = GetValue(i, j, k);
        if (thisval >= val) SetValue(i, j, k, 0.0);
      }
    }
  }
}

void Cube::quantize(double num) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    if (testValue(i)) setValue(i, num);
  }
  return;
}

void Cube::abs() {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    setValue(i, fabs(getValue(i)));
  }
  return;
}

void Cube::applymask(Cube &m) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    if (!(m.testValue(i))) setValue(i, 0);
  }
  minval = 1;
  maxval = 0;
  return;
}

void Cube::invert() {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    if (testValue(i))
      setValue(i, 0);
    else
      setValue(i, 1);
  }
  return;
}

void Cube::intersect(Cube &cb) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    if (!cb.testValue(i)) setValue(i, 0);
  }
  return;
}

// FIXME make sure the following does the right thing

void Cube::unionmask(Cube &cb) {
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    if (cb.testValue(i)) setValue(i, 1);
  }
}

int Cube::is_surface(int x, int y, int z) {
  if (x == 0 || y == 0 || z == 0) return 1;
  if (x == dimx - 1 || y == dimy - 1 || z == dimz - 1) return 1;
  if (GetValue(x - 1, y, z) == 0) return 1;
  if (GetValue(x + 1, y, z) == 0) return 1;
  if (GetValue(x, y - 1, z) == 0) return 1;
  if (GetValue(x, y + 1, z) == 0) return 1;
  if (GetValue(x, y, z - 1) == 0) return 1;
  if (GetValue(x, y, z + 1) == 0) return 1;
  return 0;
}

void Cube::removenans() {
  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        if (!(finite(GetValue(i, j, k)))) SetValue(i, j, k, 0.0);
      }
    }
  }
}

double Cube::get_maximum() {
  if (minval > maxval) calcminmax();
  return maxval;
}

double Cube::get_minimum() {
  if (minval > maxval) calcminmax();
  return minval;
}

int32 Cube::get_nonfinites() {
  if (minval > maxval) calcminmax();
  return nonfinites;
}

void Cube::calcminmax() {
  nonfinites = minval = maxval = 0;
  if (!data) return;
  minval = maxval = GetValue(0, 0, 0);
  double thisval;
  for (int i = 0; i < dimx * dimy * dimz; i++) {
    thisval = getValue<double>(i);
    if (!(finite(thisval))) {
      nonfinites++;
      continue;
    }
    if (thisval > maxval) maxval = thisval;
    if (thisval < minval) minval = thisval;
  }
}

string Cube::header2string() {
  string hdr;

  hdr += "DataType: ";
  switch (f_scaled ? altdatatype : datatype) {
    case (vb_byte):
      hdr += "Byte\n";
      break;
    case (vb_short):
      hdr += "Integer\n";
      break;
    case (vb_long):
      hdr += "Long\n";
      break;
    case (vb_float):
      hdr += "Float\n";
      break;
    case (vb_double):
      hdr += "Double\n";
      break;
    default:
      hdr += "Integer\n";
      break;
  }
  hdr += (boost::format("VoxDims(XYZ): %d %d %d\n") % dimx % dimy % dimz).str();
  if ((voxsize[0] + voxsize[1] + voxsize[2]) > 0.0)
    hdr += (boost::format("VoxSizes(XYZ): %.4f %.4f %.4f\n") % voxsize[0] %
            voxsize[1] % voxsize[2])
               .str();
  if ((origin[0] + origin[1] + origin[2]) > 0)
    hdr += (boost::format("Origin(XYZ): %d %d %d\n") % origin[0] % origin[1] %
            origin[2])
               .str();
  if (filebyteorder == ENDIAN_BIG)
    hdr += "Byteorder: msbfirst\n";
  else
    hdr += "Byteorder: lsbfirst\n";
  hdr += "Orientation: " + orient + "\n";
  if (f_scaled) {
    hdr += "scl_slope: " + strnum(scl_slope) + "\n";
    hdr += "scl_inter: " + strnum(scl_inter) + "\n";
  }
  pair<uint32, VBMaskSpec> pp;
  vbforeach(pp, maskspecs) hdr +=
      (format("vb_maskspec: %d %d %d %d %s\n") % pp.first % pp.second.r %
       pp.second.g % pp.second.b % pp.second.name)
          .str();
  for (int i = 0; i < (int)header.size(); i++) hdr += header[i] + "\n";
  return hdr;
}

void Cube::string2header(string &hdr) {
  string dtype;
  tokenlist args, wholeline;
  wholeline.SetSeparator("\n");

  wholeline.ParseLine(hdr);

  for (size_t i = 0; i < wholeline.size(); i++) {
    args.ParseLine(wholeline[i]);

    if (args[0] == "VoxDims(XYZ):" && args.size() > 3) {
      dimx = strtol(args[1]);
      dimy = strtol(args[2]);
      dimz = strtol(args[3]);
    } else if (args[0] == "DataType:" && args.size() > 1)
      dtype = args[1];
    else if (args[0] == "VoxSizes(XYZ):" && args.size() > 3) {
      voxsize[0] = strtod(args[1]);
      voxsize[1] = strtod(args[2]);
      voxsize[2] = strtod(args[3]);
    } else if (args[0] == "Origin(XYZ):" && args.size() > 3) {
      origin[0] = strtol(args[1]);
      origin[1] = strtol(args[2]);
      origin[2] = strtol(args[3]);
    } else if (args[0] == "Byteorder:" && args.size() > 1) {
      if (args[1] == "lsbfirst")
        filebyteorder = ENDIAN_LITTLE;
      else
        filebyteorder = ENDIAN_BIG;
      continue;
    } else if (args[0] == "Orientation:" && args.size() > 1)
      orient = args[1];
    else if (args[0] == "scl_slope:")
      scl_slope = strtod(args[1]);
    else if (args[0] == "scl_inter:")
      scl_inter = strtod(args[1]);
    else if (args[0] == "vb_maskspec:" && args.size() > 5) {
      addMaskSpec(strtol(args[1]), strtol(args[2]), strtol(args[3]),
                  strtol(args[4]), args[5]);
    } else {
      string tmp = wholeline[i];
      stripchars(tmp, "\n");
      header.push_back(tmp);
    }
  }
  parsedatatype(dtype, datatype, datasize);
  voxels = dimx * dimy * dimz;
  header_valid = 1;
}

uint32 Cube::count() {
  if (!data) return 0;
  uint32 cnt = 0;
  for (int i = 0; i < dimx * dimy * dimz; i++)
    if (testValue(i)) cnt++;
  return cnt;
}

void Cube::guessorigin() {
  if (origin[0] || origin[1] || origin[2]) return;
  if (dimx == 53 && dimy == 63 && dimz == 46) {
    origin[0] = 26;
    origin[1] = 37;
    origin[2] = 17;
  } else if (dimx == 91 && dimy == 109 && dimz == 91) {
    origin[0] = 45;
    origin[1] = 63;
    origin[2] = 36;
  } else if (dimx == 182 && dimy == 218 && dimz == 182) {
    origin[0] = 90;
    origin[1] = 126;
    origin[2] = 72;
  } else if (dimx == 181 && dimy == 217 && dimz == 181) {
    origin[0] = 90;
    origin[1] = 126;
    origin[2] = 72;
  } else if (dimx == 157 && dimy == 189 && dimz == 136) {
    origin[0] = 78;
    origin[1] = 112;
    origin[2] = 50;
  } else if (dimx == 61 && dimy == 73 && dimz == 61) {
    origin[0] = 30;
    origin[1] = 42;
    origin[2] = 24;
  } else if (dimx == 79 && dimy == 95 && dimz == 69) {
    origin[0] = 39;
    origin[1] = 56;
    origin[2] = 25;
  } else if (dimx == 79 && dimy == 95 && dimz == 68) {
    origin[0] = 39;
    origin[1] = 56;
    origin[2] = 25;
  }
}

int Cube::convert_type(VB_datatype newtype, uint16 flags) {
  if (!data) return 100;
  if (datatype != newtype) {
    int32 n = dimx * dimy * dimz;
    unsigned char *tmp = convert_buffer(data, n, datatype, newtype);
    if (tmp == NULL) {
      invalidate();
      return 120;
    }
    if (!f_mirrored) delete[] data;
    data = tmp;
    SetDataType(newtype);
  }
  if (flags & VBSETALT) altdatatype = newtype;
  if (flags & VBNOSCALE) {
    f_scaled = 0;
    scl_slope = scl_inter = 0;
  }
  return 0;
}

template <class T>
T Cube::getValueSafe(int x, int y, int z) const {
  if (x < 0 || y < 0 || z < 0) return 0;
  if (x > dimx - 1 || y > dimy - 1 || z > dimz - 1) return 0;
  T *ptr = (T *)data;
  int index = dimx * ((dimy * z) + y) + x;
  if (!f_scaled) return ptr[index];
  T val = ptr[index];
  return val;
};

double Cube::getValue(int index) const {
  // FIXME ugly!
  char *ptr1;
  int16 *ptr2;
  int32 *ptr3;
  float *ptr4;
  double *ptr5;
  double val = 0.0;
  switch (datatype) {
    case vb_byte:
      ptr1 = (char *)data;
      ptr1 += index;
      val = (double)*ptr1;
      break;
    case vb_short:
      ptr2 = (int16 *)data;
      ptr2 += index;
      val = (double)*ptr2;
      break;
    case vb_long:
      ptr3 = (int32 *)data;
      ptr3 += index;
      val = (double)*ptr3;
      break;
    case vb_float:
      ptr4 = (float *)data;
      ptr4 += index;
      val = (double)*ptr4;
      break;
    case vb_double:
      ptr5 = (double *)data;
      ptr5 += index;
      val = (double)*ptr5;
      break;
  }
  return val;
}

template <class T>
bool Cube::testValueSafe(int x, int y, int z) const {
  if (x < 0 || y < 0 || z < 0) return 0;
  if (x > dimx - 1 || y > dimy - 1 || z > dimz - 1) return 0;
  T *ptr = (T *)data;
  int index = dimx * ((dimy * z) + y) + x;
  if (ptr[index]) return 1;
  return 0;
};

template <class T>
bool Cube::testValueUnsafe(int x, int y, int z) const {
  T *ptr = (T *)data;
  int index = dimx * ((dimy * z) + y) + x;
  if (ptr[index]) return 1;
  return 0;
};

template <class T>
bool Cube::testValueUnsafe(int index) const {
  T *ptr = (T *)data;
  if (ptr[index]) return 1;
  return 0;
};

template <class T>
T Cube::getValue(VBVoxel &v) const {
  return getValue<T>(v.x, v.y, v.z);
}

template <class T>
T Cube::getValue(int x, int y, int z) const {
  switch (datatype) {
    case vb_byte:
      return (T)getValueSafe<unsigned char>(x, y, z);
      break;
    case vb_short:
      return (T)getValueSafe<int16>(x, y, z);
      break;
    case vb_long:
      return (T)getValueSafe<int32>(x, y, z);
      break;
    case vb_float:
      return (T)getValueSafe<float>(x, y, z);
      break;
    case vb_double:
      return (T)getValueSafe<double>(x, y, z);
      break;
  }
  exit(999);  // FIXME shouldn't happen, but still...
  return 0;
};

template <class T>
T Cube::getValue(int index) const {
  if (index > dimx * dimy * dimz || !data) cout << "Shouldn't happen" << endl;
  switch (datatype) {
    case vb_byte:
      return (T)((unsigned char *)data)[index];
      break;
    case vb_short:
      return (T)((int16 *)data)[index];
      break;
    case vb_long:
      return (T)((int32 *)data)[index];
      break;
    case vb_float:
      return (T)((float *)data)[index];
      break;
    case vb_double:
      return (T)((double *)data)[index];
      break;
  }
  exit(999);  // FIXME shouldn't happen, but still...
  return 0;
};

template <class T>
void Cube::setValue(int index, T value) {
  if (index > dimx * dimy * dimz || !data) cout << "Shouldn't happen" << endl;
  switch (datatype) {
    case vb_byte:
      ((unsigned char *)data)[index] = value;
      break;
    case vb_short:
      ((int16 *)data)[index] = value;
      break;
    case vb_long:
      ((int32 *)data)[index] = value;
      break;
    case vb_float:
      ((float *)data)[index] = value;
      break;
    case vb_double:
      ((double *)data)[index] = value;
      break;
  }
}

template <class T>
int Cube::setValue(int x, int y, int z, T value) const {
  if (x < 0 || y < 0 || z < 0) return 0;
  if (x > dimx - 1 || y > dimy - 1 || z > dimz - 1) return 0;
  // T *ptr=(T *)data;
  int index = dimx * ((dimy * z) + y) + x;
  switch (datatype) {
    case vb_byte:
      ((unsigned char *)data)[index] = value;
      break;
    case vb_short:
      ((int16 *)data)[index] = value;
      break;
    case vb_long:
      ((int32 *)data)[index] = value;
      break;
    case vb_float:
      ((float *)data)[index] = value;
      break;
    case vb_double:
      ((double *)data)[index] = value;
      break;
  }
  return 1;
};

// explicit instantiations.  we need to do each function individually
// because cube isn't a template class, it just has 8 separate
// template functions

template char Cube::getValueSafe<char>(int x, int y, int z) const;
template int16 Cube::getValueSafe<int16>(int x, int y, int z) const;
template int32 Cube::getValueSafe<int32>(int x, int y, int z) const;
template float Cube::getValueSafe<float>(int x, int y, int z) const;
template double Cube::getValueSafe<double>(int x, int y, int z) const;

template bool Cube::testValueSafe<char>(int x, int y, int z) const;
template bool Cube::testValueSafe<int16>(int x, int y, int z) const;
template bool Cube::testValueSafe<int32>(int x, int y, int z) const;
template bool Cube::testValueSafe<float>(int x, int y, int z) const;
template bool Cube::testValueSafe<double>(int x, int y, int z) const;

template bool Cube::testValueUnsafe<char>(int x, int y, int z) const;
template bool Cube::testValueUnsafe<int16>(int x, int y, int z) const;
template bool Cube::testValueUnsafe<int32>(int x, int y, int z) const;
template bool Cube::testValueUnsafe<float>(int x, int y, int z) const;
template bool Cube::testValueUnsafe<double>(int x, int y, int z) const;

template bool Cube::testValueUnsafe<char>(int index) const;
template bool Cube::testValueUnsafe<int16>(int index) const;
template bool Cube::testValueUnsafe<int32>(int index) const;
template bool Cube::testValueUnsafe<float>(int index) const;
template bool Cube::testValueUnsafe<double>(int index) const;

template char Cube::getValue<char>(int x, int y, int z) const;
template int16 Cube::getValue<int16>(int x, int y, int z) const;
template int32 Cube::getValue<int32>(int x, int y, int z) const;
template float Cube::getValue<float>(int x, int y, int z) const;
template double Cube::getValue<double>(int x, int y, int z) const;

template char Cube::getValue<char>(int index) const;
template int16 Cube::getValue<int16>(int index) const;
template int32 Cube::getValue<int32>(int index) const;
template float Cube::getValue<float>(int index) const;
template double Cube::getValue<double>(int index) const;

template char Cube::getValue<char>(VBVoxel &v) const;
template int16 Cube::getValue<int16>(VBVoxel &v) const;
template int32 Cube::getValue<int32>(VBVoxel &v) const;
template float Cube::getValue<float>(VBVoxel &v) const;
template double Cube::getValue<double>(VBVoxel &v) const;

template void Cube::setValue<char>(int index, char value);
template void Cube::setValue<int16>(int index, int16 value);
template void Cube::setValue<int32>(int index, int32 value);
template void Cube::setValue<float>(int index, float value);
template void Cube::setValue<double>(int index, double value);

template int Cube::setValue<char>(int x, int y, int z, char value) const;
template int Cube::setValue<int16>(int x, int y, int z, int16 value) const;
template int Cube::setValue<int32>(int x, int y, int z, int32 value) const;
template int Cube::setValue<float>(int x, int y, int z, float value) const;
template int Cube::setValue<double>(int x, int y, int z, double value) const;
