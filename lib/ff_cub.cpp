
// ff_cub.cpp
// I/O code for VoxBo file formats (tes, cub, ref)
// Copyright (c) 1998-2009 by The VoxBo Development Team

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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>
#include <sstream>
#include "vbio.h"
#include "vbutil.h"

extern "C" {

vf_status cub1_test(unsigned char *buf, int bufsize, string filename);
int cub1_read_head(Cube *cb);
int cub1_read_data(Cube *cb);
int cub1_write(Cube *cb);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF cub1_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "VoxBo CUB1";
  tmp.extension = "cub";
  tmp.signature = "cub1";
  tmp.dimensions = 3;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_3D = cub1_test;
  tmp.read_head_3D = cub1_read_head;
  tmp.read_data_3D = cub1_read_data;
  tmp.write_3D = cub1_write;
  return tmp;
}

vf_status cub1_test(unsigned char *buf, int bufsize, string) {
  tokenlist args;
  args.SetSeparator("\n");
  if (bufsize < 40) return vf_no;
  args.ParseLine((char *)buf);
  if (args[0] != "VB98" || args[1] != "CUB1") return vf_no;
  return vf_yes;
}

int cub1_read_head(Cube *cb) {
  gzFile fp;
  char line[STRINGLEN];
  tokenlist args;

  fp = gzopen(cb->filename.c_str(), "r");
  if (!fp) return (100);
  cb->header.clear();
  if (gzread(fp, line, 10) != 10) {
    gzclose(fp);
    return (150);
  }
  if (strncmp(line, "VB98\nCUB1\n", 10)) {
    gzclose(fp);
    return (151);
  }
  string hdr;
  while (gzgets(fp, line, STRINGLEN)) {
    if (line[0] == 12) break;
    hdr += line;
  }
  cb->string2header(hdr);
  cb->offset = gztell(fp);
  gzclose(fp);
  if (cb->scl_slope > FLT_MIN) {
    cb->f_scaled = 1;
    cb->altdatatype = cb->datatype;
  }
  return (0);  // no error!
}

int cub1_read_data(Cube *cb) {
  gzFile fp;

  fp = gzopen(cb->filename.c_str(), "r");
  if (!fp) return (100);
  gzseek(fp, cb->offset, SEEK_SET);
  cb->SetVolume(cb->dimx, cb->dimy, cb->dimz, cb->datatype);
  if (!cb->data_valid) {
    gzclose(fp);
    return 154;
  }
  int cnt = gzread(fp, cb->data, cb->datasize * cb->voxels);
  gzclose(fp);
  if (cnt != cb->voxels * cb->datasize) return (155);
  if (my_endian() != cb->filebyteorder)
    swapn(cb->data, cb->datasize, cb->voxels);
  if (cb->f_scaled) {
    if (cb->datatype == vb_byte || cb->datatype == vb_short ||
        cb->datatype == vb_long)
      cb->convert_type(vb_float);
    *cb *= cb->scl_slope;
    *cb += cb->scl_inter;
  }
  cb->data_valid = 1;
  return (0);  // no error!
}

int cub1_write(Cube *cb) {
  // open the tmp file and write out the header
  string fname = cb->GetFileName();
  // tmpfname must preserve extension!
  string tmpfname = (format("%s/tmp_%d_%d_%s") % xdirname(fname) % getpid() %
                     time(NULL) % xfilename(fname))
                        .str();
  // cout << fname << " " << tmpfname << endl;
  zfile zfp;
  zfp.open(tmpfname, "w");
  if (!zfp) return 101;

  // write the actual data
  if (cb->f_scaled) {
    *cb -= cb->scl_inter;
    *cb /= cb->scl_slope;
    if (cb->altdatatype == vb_byte || cb->altdatatype == vb_short ||
        cb->altdatatype == vb_long)
      cb->convert_type(cb->altdatatype);
  }
  if (my_endian() != cb->filebyteorder)  // swap if needed
    cb->byteswap();

  // build the header (must be built after type conversion!)
  string hdr;
  hdr += "VB98\nCUB1\n";
  hdr += cb->header2string();
  hdr += "\x0c\n";
  // write the header and then the data
  int cnt, towrite = cb->datasize * cb->voxels;
  zfp.write(hdr.c_str(), hdr.size());
  cnt = zfp.write(cb->data, towrite);
  zfp.close();
  // scale/swap it back
  if (my_endian() != cb->filebyteorder) cb->byteswap();
  if (cb->f_scaled) {
    if (cb->datatype == vb_byte || cb->datatype == vb_short ||
        cb->datatype == vb_long)
      cb->convert_type(vb_float);
    *cb *= cb->scl_slope;
    *cb += cb->scl_inter;
  }
  if (cnt != towrite) {
    unlink(tmpfname.c_str());
    return (102);
  }
  if (rename(tmpfname.c_str(), fname.c_str())) return (103);
  return (0);  // no error!
}

}  // extern "C"
