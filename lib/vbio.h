
// vbio.h
// header information for i/o functions
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
// numerous changes/additions by Kosh Banerjee
// many of those changes later undone by Dan

using namespace std;

#ifndef VBIO_H
#define VBIO_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "vbutil.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_block.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_sys.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_vector_double.h>

// the following headers are treated specially:
// VoxDims(XYZ)  dimx/y/z
// DataType      datatype
// VoxSizes(XYZ) voxsize[0-2]
// Origin(XYZ)   origin[0-2]
// Byteorder     byteorder
// Orientation   orient
// scl_slope     scl_slope
// scl_inter     scl_inter
// TR(msecs)     voxsize[3]

// matching criteria for voxel finding: greater than, greater than or
// equal, absolute value greater than, etc.
enum vf_crit_type {
  vb_gt,
  vb_ge,
  vb_agt,
  vb_age,
  vb_lt,
  vb_le,
  vb_eq,
  vb_ne,
  vb_any
};
enum vf_status { vf_no = 1, vf_maybe = 2, vf_yes = 3 };
enum vf_datatype { vf_1d = 1, vf_3d = 3, vf_4d = 4 };
// flags for convert_type
enum { VBSETALT = 0x01, VBNOSCALE = 0x02 };
const int64 MAX_DIM = 2000000;

// helpful forward declarations
class VB_Vector;
class VBMatrix;
class VBFF;
class VBImage;
class Cube;
class Tes;
class Vec;
class VBRegion;
class VBVoxel;
class vcoord;

////////////////////////////////////////
// VBFF Stuff (needs to be cleaned up!)
////////////////////////////////////////

// typedefs

typedef vf_status (*test1Dfun)(unsigned char *buf, int bufsize, string fname);
typedef int (*read1Dfun)(VB_Vector *vec);
typedef int (*write1Dfun)(VB_Vector *vec);

typedef vf_status (*test2Dfun)(unsigned char *buf, int bufsize, string fname);
typedef int (*readhead2Dfun)(VBMatrix *);
typedef int (*readdata2Dfun)(VBMatrix *, uint32 r1, uint32 rn, uint32 c1,
                             uint32 cn);
typedef int (*write2Dfun)(VBMatrix *);

typedef vf_status (*test3Dfun)(unsigned char *buf, int bufsize, string fname);
typedef int (*readhead3Dfun)(Cube *);
typedef int (*readdata3Dfun)(Cube *);
typedef int (*write3Dfun)(Cube *);

typedef vf_status (*test4Dfun)(unsigned char *buf, int bufsize, string fname);
typedef int (*readhead4Dfun)(Tes *mytes);
typedef int (*readdata4Dfun)(Tes *mytes, int start, int count);
typedef int (*readts4Dfun)(Tes &mytes, int x, int y, int z);
typedef int (*readvol4Dfun)(Tes &ts, Cube &cb, int t);
typedef int (*write4Dfun)(Tes *mytes);
typedef VBFF (*ffinitfn)();

// typedef vector<VBVoxel>::iterator VI;

class VBFF {
 public:
  string name;
  string extension;
  string signature;
  string path;
  int version_major, version_minor;
  int dimensions;
  // the fastts flag indicates, for 4d data read from disk, that time
  // series access is faster than volume-based access.  currently true
  // only for tes format.  similarly for f_headermask, which indicates
  // that the mask is loaded after reading only the header.  FIXME
  // these should really be in a map or something, so that we can add
  // arbitrary flags more easily.
  bool f_fastts;
  bool f_headermask;

  // static stuff
  static vector<VBFF> filetypelist;
  static void install_filetype(VBFF newff);
  static int LoadFileTypes();
  static void LoadBuiltinFiletypes();
  static void printffblurb();

  VBFF();
  void init();

  // accessors
  string getName() const;
  string getSignature() const;
  string getPath() const;
  int getDimensions() const;
  static int LoadFiletype(string dlfile);
  void print() const;

  // all the function pointers

  // for 1D data
  test1Dfun test_1D;
  read1Dfun read_1D;
  write1Dfun write_1D;

  // for 2D data
  test2Dfun test_2D;
  readhead2Dfun read_head_2D;
  readdata2Dfun read_data_2D;
  write2Dfun write_2D;

  // for 3D data
  test3Dfun test_3D;
  readhead3Dfun read_head_3D;
  readdata3Dfun read_data_3D;
  write3Dfun write_3D;

  // for 4D data
  test4Dfun test_4D;
  readhead4Dfun read_head_4D;
  readdata4Dfun read_data_4D;
  readts4Dfun read_ts_4D;
  readvol4Dfun read_vol_4D;
  write4Dfun write_4D;
};

#include "vb_vector.h"

// utility functions
string GetHeader(vector<string> headers, string tag);

// filetype related utility functions
VBFF findFileFormat(const string &signature);
VBFF findFileFormat(const string &fname, int dims);
string DataTypeName(const VB_datatype &dt);
void parsedatatype(const string &dtype, VB_datatype &datatype, int &datasize);
vector<VBFF> EligibleFileTypes(string fname, int dims = 0);
double toDouble(VB_datatype datatype, unsigned char *ptr);
unsigned char *convert_buffer(unsigned char *ptr, int n, VB_datatype oldtype,
                              VB_datatype newtype);

template <class T1, class T2>
unsigned char *convertbuffer2(T1 *from, int n);

// built-in file formats

extern "C" {
VBFF cub1_vbff();
VBFF tes1_vbff();
VBFF ref1_vbff();
VBFF mat1_vbff();
VBFF mtx_vbff();
VBFF dcm3d_vbff();
VBFF dcm4d_vbff();
VBFF img3d_vbff();
VBFF img4d_vbff();
VBFF imgdir_vbff();
VBFF nifti3d_vbff();
VBFF nifti4d_vbff();
VBFF roi_vbff();
VBFF ge_vbff();
VBFF vmp3d_vbff();
}

class VBMaskSpec {
 public:
  VBMaskSpec() {}
  VBMaskSpec(string nn, uint16 rr, uint16 gg, uint16 bb) {
    name = nn;
    r = rr;
    g = gg;
    b = bb;
  }
  uint16 r, g, b;
  string name;
};

class VBImage {
 private:
 public:
  int dimx, dimy, dimz, dimt;
  float voxsize[4];  // voxel sizes in mm, tr (for 4d data) in msec
  int voxels;        // total spatial voxels
  long offset;       // offset into file for data
  double scl_slope;  // slope of scale factor
  double scl_inter;  // intercept of scale factor
  bool f_scaled;     // are scl_slope/scl_inter used?
  int origin[3];
  // NIfTI-1.1 orientation stuff
  short qform_code, sform_code;
  float qoffset[3];
  float quatern_b, quatern_c, quatern_d;
  float srow_x[4];
  float srow_y[4];
  float srow_z[4];
  string orient;  // L-R A-P I-S for x/y/z, neurological axials are LPI
  VB_byteorder filebyteorder;  // ENDIAN_BIG or ENDIAN_LITTLE
  string filename;             // filename read or to-be-written
  vector<string> header;       // unformatted text header
  VBFF fileformat;
  VB_datatype datatype;     // the datatype in memory at the moment
  VB_datatype altdatatype;  // the datatype on disk
  int datasize;             // altdatasize;
  int header_valid, data_valid;
  map<uint32, VBMaskSpec> maskspecs;
  void addMaskSpec(int index, int r, int g, int b, string name);
  bool f_mirrored;
  int32 subvolume;  // used to index the part of a tes file
                    // to be read.  non-negative values are
                    // volume numbers.  magic number -1
                    // means no subvolume. magic number -2
                    // means the mask volume.
  // application-specific usable fields (e.g., used by vbim to
  // maintain temporary partial filenames)
  int32 id1, id2;
  string id3, id4;

  virtual ~VBImage();

  // accessors
  string GetFileName() const;
  void SetFileName(const string &fname);
  VBFF GetFileFormat() const;
  void SetFileFormat(VBFF format);
  int SetFileFormat(const string &format);
  void SetDataType(VB_datatype type);
  VB_datatype GetDataType();
  int GetCorner(double &x, double &y, double &z);
  int GetDataSize();
  void ReparseFileName();
  int inbounds(int x, int y, int z) const;
  void SetOrigin(float x, float y, float z);
  void setVoxSizes(float x, float y, float z, float t);
  bool dimsequal(const VBImage &im);

  int voxelposition(int x, int y, int z) const;
  void getXYZ(int32 &x, int32 &y, int32 &z, const uint32 point) const;
  VBVoxel getvoxel(const uint32 index) const;

  // common member functions
  void AddHeader(const string &str);  // tack string onto header
  string GetHeader(string str) const;
  int WriteHeader(string tag, string newContents);
  void CopyHeader(const VBImage &im);

  // virtual functions must be instantiated
  virtual void init() = 0;
  void init_nifti();
  virtual void zero() = 0;
  virtual void byteswap() = 0;
  virtual void print() const = 0;
  virtual void printbrief(const string &flags = "") const = 0;
  virtual void invalidate() = 0;
  template <class T>
  T scaledvalue(T val) const {
    if (scl_slope != 0.0)
      return val * scl_slope + scl_inter;
    else
      return val + scl_inter;
  }
};

class Cube : public VBImage {
 public:
  // constructors, etc.
  Cube();
  Cube(Cube *);  // duplicate an old cube
  Cube(const Cube &cb);
  Cube(VBRegion &rr);
  // Cube(const string &);        // load from CUB1
  Cube(int in_dimx, int in_dimy, int in_dimz, VB_datatype in_type);
  ~Cube();

  // conversion operator for testing validity
  operator bool() const { return (bool)data; }

  // operators
  Cube &operator+=(double num);
  Cube &operator-=(double num);
  Cube &operator*=(double num);
  Cube &operator/=(double num);
  Cube &operator+=(const Cube &cb);
  Cube &operator-=(const Cube &cb);
  Cube &operator/=(const Cube &cb);
  Cube &operator*=(const Cube &cb);
  Cube &operator=(double num);
  Cube &operator=(const Cube &cb);

  // etc.
  Cube &copycube(const Cube &cb, bool mirrorflag = 0);
  int SetVolume(uint32 in_dimx, uint32 in_dimy, uint32 in_dimz,
                VB_datatype in_type);
  int SetVolume(const Cube &cb, VB_datatype dt);
  int SetVolume(const Cube &cb);

  // the new i/o functions
  int ReadFile(const string &fname);
  int ReadHeader(const string &fname);
  int ReadData(const string &fname);
  int WriteFile(const string fname = "");
  int ReadLabels();
  void string2header(string &hdr);
  string header2string();

  // the actual data
  unsigned char *data, *altdata;

  template <class T>
  T getValueSafe(int x, int y, int z) const;
  template <class T>
  bool testValueSafe(int x, int y, int z) const;
  template <class T>
  bool testValueUnsafe(int x, int y, int z) const;
  template <class T>
  bool testValueUnsafe(int index) const;
  template <class T>
  T getValue(int x, int y, int z) const;
  template <class T>
  T getValue(int index) const;
  template <class T>
  T getValue(VBVoxel &v) const;
  template <class T>
  void setValue(int index, T value);
  template <class T>
  int setValue(int x, int y, int z, T value) const;

  double getValue(int index) const;
  double GetValue(VBVoxel &v) const;           // return value cast as a double
  double GetValue(int x, int y, int z) const;  // return value cast as a double
  bool testValue(int x, int y, int z) const;   // value != 0 ?
  bool testValue(int index) const;             // value != 0 ?
  // FIXME get rid of this!
  void SetValue(int x, int y, int z, double val);

  void resize(int x, int y, int z);
  int convert_type(VB_datatype newtype, uint16 flags = 0);
  int adjust_for_scl_read();
  int adjust_for_scl_prewrite();
  int adjust_for_scl_postwrite();

  // manipulations, housekeeping, etc.
  void init();
  int init(int in_dimx, int in_dimy, int in_dimz, VB_datatype in_type);
  virtual void print() const;                               // print some info
  virtual void printbrief(const string &flags = "") const;  // print some info
  virtual void invalidate();
  void zero();
  void zero(int x1, int x2, int y1, int y2, int z1, int z2);
  float meglen() const;
  void byteswap();
  void flipx();
  void flipy();
  void flipz();
  void leftify();
  void rightify();
  void thresh(double val);
  void threshabs(double val);
  void cutoff(double val);
  void quantize(double num);
  void abs();
  void invert();
  void unionmask(Cube &cb);
  void intersect(Cube &cb);
  void applymask(Cube &m);
  void removenans();
  uint32 count();
  int is_surface(int x, int y, int z);
  friend ostream &operator<<(ostream &os, const Cube &cb);

  // return max and min values (calculated if not already set)
  double get_maximum();
  double get_minimum();
  int32 get_nonfinites();
  void guessorigin();
  void calcminmax();

 private:
  double minval;
  double maxval;
  int32 nonfinites;
};

// some basic nonmember functions for Cubes
int operator==(const Cube &c1, const Cube &c2);

// Cube region-related stuff
class VBVoxel {
 public:
  VBVoxel() { init(0, 0, 0); }
  VBVoxel(int xx, int yy, int zz) { init(xx, yy, zz); }
  VBVoxel(int xx, int yy, int zz, double vv) { init(xx, yy, zz, vv); }
  void init(int xx, int yy, int zz, double vv = 0.0) {
    flags = 0;
    x = xx;
    y = yy;
    z = zz;
    val = vv;
  }
  int32 x, y, z;
  double val;
  uint32 flags;
  // status flags
  void setCool(bool f = 1) {
    if (f)
      flags |= 0x01;
    else
      flags &= ~0x01;
  }
  bool cool() { return flags & 0x01; }
  void print() { cout << format("VBVoxel(%d,%d,%d)=%g\n") % x % y % z % val; }
};

bool vcompare(VBVoxel x, VBVoxel y);

// this typedef is for making iterators into the voxel list of a region
typedef map<uint64, VBVoxel>::iterator VI;

class vcoord {
 public:
  vcoord() { init(); }
  vcoord(double x, double y, double z, double t = 0) { init(x, y, z, t); }
  vcoord(VBVoxel &v) {
    init();
    val[0] = v.x;
    val[1] = v.y;
    val[2] = v.z;
  }
  template <class T>
  explicit vcoord(T val[]) {
    init();
    copy(val);
  }
  template <class T>
  void copy(T v[]) {
    val[0] = v[0];
    val[1] = v[1];
    val[2] = v[2];
  }
  void init() { val[0] = val[1] = val[2] = val[3] = 0; }
  void init(double x, double y, double z, double t = 0) {
    init();
    val[0] = x, val[1] = y, val[2] = z, val[3] = t;
  }
  vcoord &operator+=(vcoord &c) {
    val[0] += c[0];
    val[1] += c[1];
    val[2] += c[2];
    return *this;
  }
  vcoord &operator-=(vcoord &c) {
    val[0] -= c[0];
    val[1] -= c[1];
    val[2] -= c[2];
    return *this;
  }
  // vcoord operator+(const vcoord &c);
  // vcoord operator-(const vcoord &c);
  string str() {
    return strnum(val[0]) + "x" + strnum(val[1]) + "x" + strnum(val[2]);
  }
  double &operator[](uint16 i) {
    assert(i < 4);
    return val[i];
  }

 private:
  double val[4];  // up to 4, but really 3 for now
};

class VBRegion {
 public:
  VBRegion();
  VBRegion(Cube &cb, int crit_type = vb_agt, double crit_value = 0.0);
  uint64 dimx, dimy, dimz;  // dimensions of underlying image
  vcoord voxsizes;
  // the following four variables are set by info()
  vcoord center;      // geometric center of region
  vcoord wcenter;     // weighted center of region
  double totalmass;   // total mass of region
  vcoord peakcenter;  // geometric center of region's peak voxel set

  string name;  // name of the region (e.g., mask name)
  double val;   // some value of interest can be stored here
  map<uint64, VBVoxel> voxels;
  uint64 x, y, z, v;
  VI begin() { return voxels.begin(); }
  VI end() { return voxels.end(); }
  void print();
  void convert(Cube &cb, int crit_type = vb_agt, double crit_val = 0.0);
  void GeometricCenter(double &x, double &y, double &z);
  void max(uint64 &x, uint64 &y, uint64 &z, double &val);
  void min(uint64 &x, uint64 &y, uint64 &z, double &val);
  VBRegion maxregion();
  VBRegion minregion();
  VBRegion minnonzeroregion();
  void clear();
  int size();
  void add(uint64 ind, double val = 0.0);
  void add(uint64 x, uint64 y, uint64 z, double val = 0.0);
  void add(const VBVoxel &v);
  void getxyz(uint64 index, uint64 &x, uint64 &y, uint64 &z);
  void remove(uint64 x, uint64 y, uint64 z);
  void remove_i(uint64 index);
  bool contains_i(uint64 index);
  bool contains(uint64 x, uint64 y, uint64 z);
  void merge(VBRegion &r);
  string info();

 private:
  // index-based things need to be private to make sure we don't get
  // crazy and try to calculate valid indices outside this class
};

VBRegion findregion_mask(Cube &mask, int crit_type, double crit_val);
vector<VBRegion> findregions(Cube &mycub, int crit_type, double crit_val);
vector<VBRegion> findregions(Cube &mycub, Cube &mask, int crit_type,
                             double crit_val);
VBRegion growregion(int x, int y, int z, Cube &cb, Cube &mask, int crit_type,
                    double crit_val);
bool voxelmatch(double val, int crit_type, double crit_val);
double voxeldistance(const VBVoxel &v1, const VBVoxel &v2);
double voxeldistance(uint32 x1, uint32 y1, uint32 z1, uint32 x2, uint32 y2,
                     uint32 z2);
int poscomp(VBVoxel &v1, VBVoxel &v2);

class Tes : public VBImage {
 public:
  // constructors
  Tes();
  Tes(Tes *);           // duplicate an old tes
  Tes(const Tes &ts);   // copy constructor
  Tes(const string &);  // load from a file
  Tes(int in_dimx, int in_dimy, int in_dimz, int in_dimt, VB_datatype in_type);
  ~Tes();

  // conversion operator for testing validity
  operator bool() const { return (bool)data; }

  void init();
  int init(int in_dimx, int in_dimy, int in_dimz, int in_dimt,
           VB_datatype in_type);
  void zero();
  void byteswap();
  float meglen() const;
  int maskcount();
  unsigned char **data;
  unsigned char *mask;
  VB_Vector timeseries;
  unsigned char *buildvoxel(int x, int y = -1, int z = -1);
  int VoxelStored(int x, int y, int z);

  // the new i/o functions
  int ReadFile(const string &fname, int start = -1, int count = -1);
  int ReadHeader(const string &fname);
  int ReadData(const string &fname, int start = -1, int count = -1);
  int ReadTimeSeries(const string &fname, int x, int y, int z);
  int ReadVolume(const string &fname, int t, Cube &cb);
  int WriteFile(const string fname = "");

  int realvoxels;  // number of voxels with >0 nonzero voxels

  // operators
  Tes &operator+=(double num);
  Tes &operator-=(double num);
  Tes &operator*=(double num);
  Tes &operator/=(double num);
  Tes &operator+=(const Tes &ts);
  Tes &operator=(const Tes &ts);
  friend ostream &operator<<(ostream &os, const Tes &ts);

  Tes &copytes(const Tes &cb, bool mirrorflag = 0);

  int SetVolume(uint32 in_dimx, uint32 in_dimy, uint32 in_dimz, uint32 in_dimt,
                VB_datatype in_type);
  int InitMask(short val);
  int InitData();
  void SetVoxSizes(float v1, float v2, float v3);
  int convert_type(VB_datatype newtype, uint16 flags = 0);
  int DimsValid();

  double GetValue(int x, int y, int z, int t) const;
  double GetValue(VBVoxel &v, int t) const;
  double GetValueUnsafe(int x, int y, int z, int t) const;
  template <class T>
  T getValue(int x, int y, int z, int t) const;
  template <class T>
  T getValue(VBVoxel &v, int t) const;
  bool GetMaskValue(int x, int y, int z) const;
  bool GetMaskValue(int index) const;
  const unsigned char *GetMaskPtr();
  int ExtractMask(Cube &target);
  void SetValue(int x, int y, int z, int t, double val);
  void Remask();  // generate a new mask based on all-zero voxels
  int MergeTes(Tes &src);
  int resizeInclude(const set<int> &includelist);
  int resizeExclude(const set<int> &excludelist);
  int SetCube(int t, const Cube &cub);  // bang cube data into place
  int getCube(int index, Cube &c);  // grab cube into existing cube structure
  Cube getCube(const int index);
  int getCube(int index, list<Cube> &cubelist);
  int GetTimeSeries(int x, int y, int z);
  double GrandMean();
  Cube operator[](const int index);
  virtual void print() const;                               // print some info
  virtual void printbrief(const string &flags = "") const;  // print some info
  virtual void invalidate();
  void removenans();
  void intersect(Cube &cb);
  void applymask(Cube &m);
  void compact();
  void zerovoxel(int x, int y, int z);
  void zerovoxel(int voxelposition);
};

VB_Vector getTS(vector<string> &teslist, int x, int y, int z, uint32 flags);
VB_Vector getRegionTS(vector<string> &teslist, VBRegion &rr, uint32 flags);
VBMatrix getRegionComponents(vector<string> &teslist, VBRegion &rr,
                             uint32 flags);
VBRegion restrictRegion(vector<string> &teslist, VBRegion &rr);

class VBMatrix {
 public:
  vector<string> header;
  string filename;
  union {
    uint32 m;
    uint32 rows;
  };
  union {
    uint32 n;
    uint32 cols;
  };
  uint32 offset;
  bool transposed;
  double *rowdata;
  VBFF fileformat;

  // the below apply only to the input datatype
  VB_datatype datatype;
  int datasize;

  VB_byteorder filebyteorder;  // big- or little-endian
  FILE *matfile;
  gsl_matrix_view mview;

  VBMatrix();
  VBMatrix(const VBMatrix &mat);
  VBMatrix(const string &fname, int r1 = 0, int r2 = 0, int c1 = 0, int c2 = 0);
  VBMatrix(int rows, int cols);
  VBMatrix(VB_Vector &vec);
  ~VBMatrix();

  void init();
  void init(int xrows, int xcols);
  void clear();
  void resize(int xrows, int xcols);
  bool headerValid();

  void zero();
  void ident();
  void random();

  // I/O functions
  int ReadFile(const string &fname, uint32 r1 = 0, uint32 rn = 0, uint32 c1 = 0,
               uint32 cn = 0);
  int ReadHeader(const string &fname);
  int ReadData(const string &fname, uint32 r1, uint32 rn, uint32 c1, uint32 cn);
  int WriteFile(string fname = "");
  int WriteData();
  void printColumnCorrelations();

  // set and get for columns
  void SetRow(const uint32 row, const VB_Vector &vec);
  void SetColumn(const uint32 column, const VB_Vector &vec);
  VB_Vector GetRow(uint32 row);
  VB_Vector GetColumn(uint32 row);
  void DeleteColumn(uint32 column);

  string GetFileName() const;
  void SetDatatype(const VB_datatype &dt);
  void print();
  void printinfo();
  void printrow(int row);
  void AddHeader(const string &str);
  void GoToCol(int col);
  void float2double();

  // new operators
  int set(uint32 r, uint32 c, double val);
  double operator()(uint32 r, uint32 c);
  // operator bool() const;
  bool valid();
  VBMatrix &operator=(const VBMatrix &mat);
  VBMatrix &operator=(gsl_matrix *mat);
  VBMatrix &operator+=(const VBMatrix &mat);
  VBMatrix &operator-=(const VBMatrix &mat);
  VBMatrix &operator*=(VBMatrix &mat);
  VBMatrix &operator^=(VBMatrix &mat);
  VBMatrix &operator/=(const VBMatrix &mat);
  VBMatrix &operator+=(const double &d);
  VBMatrix &operator-=(const double &d);
  VBMatrix &operator*=(const double &d);
  VBMatrix &operator/=(const double &d);
  double trace();
};

// nonmember functions involving matrices
int MultiplyPartsXY(VBMatrix &mat1, VBMatrix &mat2, VBMatrix &target, int r1,
                    int r2, int offset = 0);
int MultiplyPartsXtY(VBMatrix &mat1, VBMatrix &mat2, VBMatrix &target, int r1,
                     int r2, int offset = 0);
int MultiplyPartsXYt(VBMatrix &mat1, VBMatrix &mat2, VBMatrix &target, int r1,
                     int r2, int offset = 0);
double RowTimesCol(VB_datatype, unsigned char *row, VB_datatype,
                   unsigned char *col, int vecsize);
int invert(const VBMatrix &src, VBMatrix &dest);
int pinv(const VBMatrix &src, VBMatrix &dest);
int pca(VBMatrix &data, VB_Vector &lambdas, VBMatrix &pcs, VBMatrix &E);

// more nonmember functions
int WritePNG(const Cube &cube, int slice, const string &filename);
bool dimsConsistent(int &x, int &y, int &z, int newx, int newy, int newz);

/*********************************************************************
 * The following struct is used hold the (x, y, z) coordinates for    *
 * the Tes mask. Recall that the mask is actually stored as a one     *
 * dimensional array, even though the mask represents a three         *
 * dimensional box. It will be necessary, given an index in the 1-D   *
 * mask array, to find the corresponding (x, y, z) coordinates. The   *
 * following struct is declared for this purpose.                     *
 *********************************************************************/

struct MaskXYZCoord {
  unsigned int x;
  unsigned int y;
  unsigned int z;
};

/*********************************************************************
 * Function used to calculate the (x, y, z) coordinates corresponding *
 * to an index in Tes->mask[].                                        *
 *********************************************************************/
void setMaskCoords(MaskXYZCoord *maskCoord, unsigned int i, unsigned int dimX,
                   unsigned int dimY, unsigned int dimZ);

/*********************************************************************
 * Functions to add a header line to the header data member of a      *
 * VBImage object.                                                    *
 *********************************************************************/
void addHeaderLine(VBImage *vb, const char *format, ...);
void addHeaderLine(VBImage &vb, const char *format, ...);

/*********************************************************************
 * Prototypes to validate a 4D data file.                             *
 *********************************************************************/
bool validate4DFile(const string tesFile);

/*********************************************************************
 * Prototype for functions to copy the header lines from one VBImage  *
 * object to another.                                                 *
 *********************************************************************/
void copyHeader(const VBImage *src, VBImage *dest);
void copyHeader(const VBImage &src, VBImage &dest);

/*********************************************************************
 * Prototype for function to orient cube image based on desired       *
 * orientation (const string in).                                     *
 *********************************************************************/
int vbOrient(Cube &incube, Cube &outCube, const string in, const string out,
             int interleaved);

/*********************************************************************
 * Prototype for functions to orient tes images based on desired      *
 * orientation (const string in)                                      *
 *********************************************************************/
int vbOrientTes(Tes &tes, Tes &outTes, const string in, const string out,
                int interleaved);

#endif  // VBIO_H
