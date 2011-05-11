
// dicom.h
// simple structure for dicom file info
// Copyright (c) 2003-2010 by The VoxBo Development Team

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

class dicominfo {
 public:
  // study/series/acqusition indentify the exam, with instance identifying the slice
  int32 study;
  int32 series;
  int32 acquisition;
  int32 instance;

  // pulse sequence, protocol, etc.
  string imagetype;        // contains "MOSAIC" if mosaiced
  string date;
  string time;
  string protocol;   // protocol name  e.g. ep2d_bold_moco
  string sequence;     // sequence name  e.g.
  string receive_coil;
  string transmit_coil;
  // string patientname;

  // acqusition info
  double slicethickness;
  double tr;     // repetition time
  double te;     // echo time
  double navg;   // number of averages
  double ti;     // inversion time
  double fieldstrength;
  double spacing;
  double flipangle;
  double win_center;
  double win_width;
  int32 dimx;
  int32 dimy;
  int32 dimz;     // slices in the file (usually 1 except for mosaics)
  int32 slices;   // slices in the volume
  int32 xfov;
  int32 yfov;
  int32 rows;
  int32 cols;
  float voxsize[3];
  float position[3];   // DICOM position
  float spos[3];       // siemens position
  float moveparam[6];  // movement parameters
  // float zvoxsize;
  float slthick;
  float skip;
  double zpos;
  string phaseencodedirection;

  // subject info
  string dob,sex,age;

  // offset to data, data size
  int32 offset;
  int32 datasize;
  int32 bpp;          // bits per pixel
  int32 bps;          // bits actually stored (should mask the rest to zero)
  int32 mosaicflag;
  VB_byteorder byteorder;
  dicominfo();
  void init();
};

class dicomge {
public:
  uint16 group,element;
  dicomge(uint16 g,uint16 e) {group=g;element=e;}
  bool operator<(const dicomge &ge) const;
};

// class dicomnames is used for one thing: 
class dicomnames {
public:
  dicomnames();          // constructor populates the map
  string operator()(dicomge ge);
  string operator()(uint16 g,uint16 e);
private:
  void populate();
  map<dicomge,string> names;    // elementnames
};

int read_dicom_header(string filename,dicominfo &dci);
int print_dicom_header(string filename);
void write_LO(FILE *ofile,VB_byteorder byteorder,uint16 group,uint16 element,string otag);
int anonymize_dicom_header(string infile,string out1,string out2,
                           set<uint16> &stripgroups,set<dicomge> &stripges,
                           set<string> &stripvrs,int &removedfields);
void transfer_dicom_header(dicominfo &dci,VBImage &im);
string patfromname(const string fname);
int read_multiple_slices(Cube *cb,tokenlist &filelist);
int read_multiple_slices_from_files(Cube *cb,vector<string>filenames);

vf_status test_dcm4d_4D(unsigned char *buf,int bufsize,string filename);
int read_head_dcm4d_4D(Tes *mytes);
int read_data_dcm4d_4D(Tes *mytes,int start=-1,int count=-1);

vf_status test_dcm3d_3D(unsigned char *buf,int bufsize,string filename);
int read_head_dcm3d_3D(Cube *cb);
int read_data_dcm3d_3D(Cube *cb);

#define SIEMENS_TAG "### ASCCONV BEGIN ###"
#define SIEMENS_TAG_END "### ASCCONV END ###"
