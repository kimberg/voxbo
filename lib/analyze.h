
// analyze.h
// structure for analyze headers
// Copyright (c) 1998-2003 by The VoxBo Development Team

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

// acceptable types for ANALYZE format data
#define DT_BINARY 1
#define DT_UNSIGNED_CHAR 2
#define DT_SIGNED_SHORT 4
#define DT_SIGNED_INT 8
#define DT_FLOAT 16
#define DT_DOUBLE 64
// my mask for acceptable types
#define DT_ACCEPTABLE 94

struct IMG_header {
  // first 40 bytes, header_key
  int32 sizeof_hdr;
  char data_type[10];
  char db_name[18];
  int32 extents;
  int16 session_error;
  char regular;
  char hkey_un0;
  // next 108 bytes, image_dimension
  int16 dim[8];  // 40,42,44,46,48,50,52,54
  char vox_units[4];
  char cal_units[8];
  int16 unused1;
  int16 datatype;  // 70
  int16 bitpix;
  int16 dim_un0;
  float pixdim[8];   // 76,80,84,88,92,96,100,104
  float vox_offset;  // 108
  float funused1;    // normally roi_scale?
  float funused2;    // normally funused1?
  float funused3;    // normally funused2?
  float cal_max;
  float cal_min;
  int32 compressed;
  int32 verified;
  int32 glmax;
  int32 glmin;
  // next 200 bytes, data_history
  char descrip[80];  // 148
  char aux_file[24];
  char orient;
  // used to be short origin[5];
  char origin[10];
  char generated[10];
  char scannum[10];
  char patient_id[10];
  char exp_date[10];
  char exp_time[10];
  char hist_un0[3];
  int32 views;
  int32 vols_added;
  int32 start_field;
  int32 field_skip;
  int32 omax;
  int32 omin;
  int32 smax;
  int32 smin;
};

int analyze_read_header(string fname, IMG_header *xhdr, VBImage *vol);
int write_analyze_header(string &fname, Cube &cb);
void print_analyze_header(IMG_header &ihead);

string img_patfromname(const string fname);
int read_head_img3d(Cube *cb);
int read_data_img3d(Cube *cb);
int write_img3d_3D(Cube *cb);

vf_status test_img4d(unsigned char *buf, int bufsize, string filename);
int read_head_img4d(Tes *mytes);
int read_data_img4d(Tes *mytes, int start = -1, int count = -1);
int write_img4d(Tes *mytes);

vf_status test_imgdir(unsigned char *buf, int bufsize, string filename);
int read_head_imgdir(Tes *mytes);
int read_data_imgdir(Tes *mytes, int start = -1, int count = -1);
int write_imgdir(Tes *mytes);

vf_status test_img3d(unsigned char *buf, int bufsize, string filename);
int read_head_img3d(Cube *cb);
int read_data_img3d(Cube *cb);
int write_img3d(Cube *cb);
