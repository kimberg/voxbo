
// analyze.cpp
// VoxBo I/O support for Analyze(TM) img/hdr format
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include "vbio.h"
#include "vbutil.h"

using boost::format;

extern "C" {

#include "analyze.h"

int analyze_read_header(string fname, IMG_header *xhdr, VBImage *vol) {
  if (xhdr == NULL && vol == NULL) return 200;
  // img hdr and vbimage hdr point to either local or passed storage
  IMG_header hh, *hdr = xhdr;
  if (hdr == NULL) hdr = &hh;
  memset(hdr, 0, sizeof(IMG_header));  // just in case
  // munge the header file name
  if (xgetextension(fname) == "img") fname = xsetextension(fname, "hdr");

  FILE *fp;
  int swapflag = 0;

  fp = fopen(fname.c_str(), "r");
  if (!fp) return (100);

  int cnt = fread(hdr, 1, sizeof(struct IMG_header), fp);
  fclose(fp);
  if (cnt == 0) return (100);

  // figure out the file byte order
  VB_byteorder bo = my_endian();
  if (hdr->dim[0] < 0 || hdr->dim[0] > 7) {
    swapflag = 1;
    if (bo == ENDIAN_BIG)
      bo = ENDIAN_LITTLE;
    else
      bo = ENDIAN_BIG;
  }

  // byte swap if we're little endian
  if (swapflag) {
    swap(&hdr->sizeof_hdr);
    swap(&hdr->bitpix);
    swap(hdr->dim, 8);
    swap(&hdr->datatype);
    swap(hdr->pixdim, 8);
    swap(&hdr->vox_offset, 10);  // vox_offset through glmin, incl. funused1-3
    swap(&hdr->extents);
    swap(&hdr->views, 8);  // incl. omax,omin,smax,smin
    swap((int16 *)(&hdr->origin), 3);
  }

  // sanity check
  // extents should be 16384
  // dim[0] should be 3 because we only read 3D files

  if (hdr->dim[0] != 3 && hdr->dim[0] != 4) {
    if (hdr->dim[1] < 2 || hdr->dim[2] < 2 || hdr->dim[3] < 2) return 100;
  }

  if (!(hdr->datatype & DT_ACCEPTABLE))  // mask of acceptable datatypes
    return (100);

  if (hdr->funused1 < FLT_MIN) hdr->funused1 = 1.0;

  if (vol == NULL) return 0;

  // now copy the img header into our structures
  vol->scl_slope = hdr->funused1;
  vol->scl_inter = 0.0;
  vol->filebyteorder = bo;
  vol->dimx = hdr->dim[1];
  vol->dimy = hdr->dim[2];
  vol->dimz = hdr->dim[3];
  vol->dimt = hdr->dim[4];
  vol->voxsize[0] = hdr->pixdim[1];
  vol->voxsize[1] = hdr->pixdim[2];
  vol->voxsize[2] = hdr->pixdim[3];
  int16 *pp = (int16 *)hdr->origin;
  vol->origin[0] = *pp - 1;
  pp++;
  vol->origin[1] = *pp - 1;
  pp++;
  vol->origin[2] = *pp - 1;

  int32 ocode = (int32)hdr->orient;
  if (ocode == 0)
    vol->orient = "LPI";
  else if (ocode == 1)
    vol->orient = "LIP";
  else if (ocode == 2)
    vol->orient = "AIL";
  else if (ocode == 3)
    vol->orient = "RPI";
  else if (ocode == 4)
    vol->orient = "RIP";
  else if (ocode == 5)
    vol->orient = "AIR";
  else
    vol->orient = "XXX";

  if (hdr->datatype == DT_UNSIGNED_CHAR)
    vol->SetDataType(vb_byte);
  else if (hdr->datatype == DT_SIGNED_SHORT)
    vol->SetDataType(vb_short);
  else if (hdr->datatype == DT_SIGNED_INT)
    vol->SetDataType(vb_long);
  else if (hdr->datatype == DT_FLOAT)
    vol->SetDataType(vb_float);
  else if (hdr->datatype == DT_DOUBLE)
    vol->SetDataType(vb_double);
  else
    vol->SetDataType(vb_short);
  vol->header_valid = 1;
  if (vol->scl_slope > FLT_MIN) {
    vol->f_scaled = 1;
    vol->altdatatype = vol->datatype;
  }
  return (0);  // no error!
}

int write_analyze_header(string &fname, Cube &cb) {
  FILE *fp;
  IMG_header ihead;

  fp = fopen(fname.c_str(), "w");
  if (!fp) return (100);

  // now fill in the fields of the header structure that we like

  ihead.sizeof_hdr = sizeof(struct IMG_header);  // should be 348!
  // data_type unused
  for (int i = 0; i < 10; i++) ihead.data_type[i] = '\0';
  for (int i = 0; i < 18; i++) ihead.db_name[i] = '\0';
  ihead.extents = 16384;
  ihead.session_error = 0;
  ihead.regular = 'r';  // i have no idea
  ihead.vox_offset = 0;
  ihead.hkey_un0 = 0;
  ihead.dim[0] = 3;  // 3D files only, thank you
  ihead.dim[1] = cb.dimx;
  ihead.dim[2] = cb.dimy;
  ihead.dim[3] = cb.dimz;
  ihead.dim[4] = 1;  // only 1 image per file
  ihead.dim[5] = 0;
  ihead.dim[6] = 0;
  ihead.dim[7] = 0;
  // in nifti, vox_units, cal_units, and unused1 become three float
  // intent codes
  for (int i = 0; i < 4; i++) ihead.vox_units[i] = 0;
  for (int i = 0; i < 8; i++) ihead.cal_units[i] = 0;
  ihead.unused1 = 0;
  // data type set later
  ihead.bitpix = cb.datasize * 8;
  ihead.dim_un0 = 0;      // slice_start in nifti
  ihead.pixdim[0] = 0.0;  // just in case?  orientation-related in nifti
  ihead.pixdim[1] = cb.voxsize[0];
  ihead.pixdim[2] = cb.voxsize[1];
  ihead.pixdim[3] = cb.voxsize[2];
  ihead.pixdim[4] = 0;
  ihead.pixdim[5] = 0;
  ihead.pixdim[6] = 0;
  ihead.pixdim[7] = 0;
  ihead.vox_offset = 0.0;
  if (cb.f_scaled)
    ihead.funused1 = cb.scl_slope;
  else
    ihead.funused1 = 1.0;
  ihead.funused2 = 0.0;  // nifti scaling offset
  ihead.funused3 = 0.0;  // nifti slice end, code, and xyzt units
  ihead.cal_max = 0;
  ihead.cal_min = 0;
  ihead.compressed = 0;  // nifti slice duration
  ihead.verified = 0;    // nifti time axis shift
  ihead.glmax = 0;       // unused
  ihead.glmin = 0;       // unused
  for (int i = 0; i < 80; i++) ihead.descrip[i] = ' ';
  for (int i = 0; i < 24; i++) ihead.aux_file[i] = ' ';
  // orient set later

  // nifti diverges a bit here
  int16 *pp = (int16 *)ihead.origin;
  *pp = cb.origin[0] + 1;
  pp++;
  *pp = cb.origin[1] + 1;
  pp++;
  *pp = cb.origin[2] + 1;
  ihead.origin[6] = 0;
  ihead.origin[7] = 0;
  ihead.origin[8] = 0;
  ihead.origin[9] = 0;
  for (int i = 0; i < 10; i++) {
    ihead.generated[i] = '\0';
    ihead.scannum[i] = '\0';
    ihead.patient_id[i] = '\0';
    ihead.exp_date[i] = '\0';
    ihead.exp_time[i] = '\0';
  }
  for (int i = 0; i < 3; i++) ihead.hist_un0[i] = '\0';
  ihead.views = 0;
  ihead.vols_added = 0;
  ihead.start_field = 0;
  ihead.field_skip = 0;
  ihead.omax = 0;
  ihead.omin = 0;
  ihead.smax = 0;
  ihead.smin = 0;

  switch (cb.datatype) {
    case vb_byte:
      ihead.datatype = DT_UNSIGNED_CHAR;
      break;
    case vb_short:
      ihead.datatype = DT_SIGNED_SHORT;
      break;
    case vb_long:
      ihead.datatype = DT_SIGNED_INT;
      break;
    case vb_float:
      ihead.datatype = DT_FLOAT;
      break;
    case vb_double:
      ihead.datatype = DT_DOUBLE;
      break;
  }

  // SPM doesn't like orient, so we clear it
  ihead.orient = 0;

  // byte swap if we're little endian
  if (my_endian() != cb.filebyteorder) {
    swap(&ihead.sizeof_hdr);
    swap(&ihead.bitpix);
    swap(ihead.dim, 8);
    swap(&ihead.datatype);
    swap(ihead.pixdim, 8);
    swap(&ihead.vox_offset, 10);  // vox_offset through glmin, incl. funused1-3
    swap(&ihead.extents);
    swap(&ihead.views, 8);  // incl. omax,omin,smax,smin
    swap((int16 *)(&ihead.origin), 3);
  }

  int cnt = fwrite(&ihead, sizeof(struct IMG_header), 1, fp);
  fclose(fp);
  if (cnt == 0) return (100);
  return (0);  // no error!
}

void print_analyze_header(IMG_header &ihead) {
  cout << format("sizeof_hdr: %d\n") % ihead.sizeof_hdr;
  cout << format("data_type: %10s\n") % ihead.data_type;
  cout << format("extents: %d\n") % ihead.extents;
  cout << format("dims: %ld %ld %ld %ld %ld %ld %ld %ld\n") % ihead.dim[0] %
              ihead.dim[1] % ihead.dim[2] % ihead.dim[3] % ihead.dim[4] %
              ihead.dim[5] % ihead.dim[6] % ihead.dim[7];
  cout << format("vox_units: %d %d %d %d\n") % ihead.vox_units[0] %
              ihead.vox_units[1] % ihead.vox_units[2] % ihead.vox_units[3];
  cout << format("cal_units: %d %d %d %d %d %d %d %d\n") % ihead.cal_units[0] %
              ihead.cal_units[1] % ihead.cal_units[2] % ihead.cal_units[3] %
              ihead.cal_units[4] % ihead.cal_units[5] % ihead.cal_units[6] %
              ihead.cal_units[7];
  cout << format("datatype: %d\n") % ihead.datatype;
  cout << format("bitpix: %d\n") % ihead.bitpix;
  cout << format("pixdim: %f %f %f %f %f %f %f %f\n") % ihead.pixdim[0] %
              ihead.pixdim[1] % ihead.pixdim[2] % ihead.pixdim[3] %
              ihead.pixdim[4] % ihead.pixdim[5] % ihead.pixdim[6] %
              ihead.pixdim[7];
  cout << format("vox_offset: %f\n") % ihead.vox_offset;
  cout << format("funused1: %f\n") % ihead.funused1;
  cout << format("funused2: %f\n") % ihead.funused2;
  cout << format("funused3: %f\n") % ihead.funused3;
  cout << format("cal_max: %f\n") % ihead.cal_max;
  cout << format("cal_min: %f\n") % ihead.cal_min;
  cout << format("compressed: %d\n") % ihead.compressed;
  cout << format("verified: %d\n") % ihead.verified;
  cout << format("glmax: %d\n") % ihead.glmax;
  cout << format("glmin: %d\n") % ihead.glmin;
  int16 *oo = (int16 *)ihead.origin;
  cout << format("origin: %d %d %d %d %d\n") % oo[0] % oo[1] % oo[2] % oo[3] %
              oo[4];
  cout << format("generated: %10s\n") % ihead.generated;
  cout << format("scannum: %10s\n") % ihead.scannum;
  cout << format("patient_id: %10s\n") % ihead.patient_id;
  cout << format("exp_date: %10s\n") % ihead.exp_date;
  cout << format("exp_time: %10s\n") % ihead.exp_time;
  cout << format("hist_un0: %3s\n") % ihead.hist_un0;
  cout << format("views: %d\n") % ihead.views;
  cout << format("vols_added: %d\n") % ihead.vols_added;
  cout << format("start_field: %d\n") % ihead.start_field;
  cout << format("field_skip: %d\n") % ihead.field_skip;
  cout << format("omax: %d\n") % ihead.omax;
  cout << format("omin: %d\n") % ihead.omin;
  cout << format("smax: %d\n") % ihead.smax;
  cout << format("smin: %d\n") % ihead.smin;
}

int read_head_img3d(Cube *cb) {
  int err = analyze_read_header(cb->GetFileName(), NULL, cb);
  return err;
}

int read_data_img3d(Cube *cb) {
  string imgname = cb->GetFileName();
  string ext = xgetextension(imgname);
  if (ext == "hdr")
    imgname = xsetextension(imgname, "img");
  else if (ext != "img")
    return 104;

  if (cb->dimx < 1 || cb->dimy < 1 || cb->dimz < 1) {
    cb->data_valid = 0;  // make sure
    return 105;
  }
  cb->SetVolume(cb->dimx, cb->dimy, cb->dimz, cb->datatype);
  if (!cb->data) return 110;

  FILE *fp = fopen(imgname.c_str(), "r");
  if (!fp) {
    delete[] cb->data;
    cb->data = (unsigned char *)NULL;
    cb->data_valid = 0;
    return (120);
  }
  int bytelen = cb->dimx * cb->dimy * cb->dimz;
  int cnt = fread(cb->data, cb->datasize, bytelen, fp);
  fclose(fp);
  if (cnt < bytelen) {
    delete[] cb->data;
    cb->data = (unsigned char *)NULL;
    cb->data_valid = 0;
    return (130);
  }
  if (my_endian() != cb->filebyteorder) cb->byteswap();
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

int write_img3d(Cube *cb)  // (VBFFParams &ri)
{
  string imgname = xsetextension(cb->GetFileName(), "img");
  string hdrname = xsetextension(cb->GetFileName(), "hdr");

  if (!cb->data_valid)  // only write the good stuff
    return (100);
  if (!cb->data)  // should never happen, but what the heck
    return (105);

  FILE *fp = fopen(imgname.c_str(), "w");
  if (!fp) return (110);
  int bytelen = cb->dimx * cb->dimy * cb->dimz;
  // un-scale/swap for writing
  if (cb->f_scaled) {
    *cb -= cb->scl_inter;
    *cb /= cb->scl_slope;
    if (cb->altdatatype == vb_byte || cb->altdatatype == vb_short ||
        cb->altdatatype == vb_long)
      cb->convert_type(cb->altdatatype);
  }
  if (my_endian() != cb->filebyteorder) cb->byteswap();
  // write
  int cnt = fwrite(cb->data, cb->datasize, bytelen, fp);
  fclose(fp);
  if (write_analyze_header(hdrname, *cb)) {
    unlink(imgname.c_str());
    return (100);
  }
  // swap back
  if (my_endian() != cb->filebyteorder) cb->byteswap();
  if (cb->f_scaled) {
    if (cb->datatype == vb_byte || cb->datatype == vb_short ||
        cb->datatype == vb_long)
      cb->convert_type(vb_float);
    *cb *= cb->scl_slope;
    *cb += cb->scl_inter;
  }
  if (cnt < bytelen) return (120);
  return (0);  // no error!
}

vf_status test_img3d(unsigned char *buf, int bufsize, string filename) {
  // fail if it's a nifti file
  if (bufsize < 348) return vf_no;
  if (strncmp((char *)buf + 344, "n+1", 4) == 0 ||
      strncmp((char *)buf + 344, "ni1", 4) == 0)
    return vf_no;

  IMG_header ihead;
  Cube dummy;
  string ext = xgetextension(filename);
  if (ext != "img" && ext != "hdr") return vf_no;
  if (analyze_read_header(filename, &ihead, NULL)) return vf_no;
  if (ihead.dim[0] == 3) return vf_yes;
  if (ihead.dim[0] == 4 && ihead.dim[4] == 1) return vf_yes;
  return vf_no;
}

vf_status test_img4d(unsigned char *buf, int bufsize, string filename) {
  // fail if it's a nifti file
  if (bufsize < 348) return vf_no;
  if (strncmp((char *)buf + 344, "n+1", 4) == 0 ||
      strncmp((char *)buf + 344, "ni1", 4) == 0)
    return vf_no;

  string ext = xgetextension(filename);
  if (ext != "img" && ext != "hdr") return vf_no;
  IMG_header ihead;
  Cube dummy;
  if (analyze_read_header(filename, &ihead, NULL)) return vf_no;
  if (ihead.dim[0] == 4 && ihead.dim[4] > 1) return vf_yes;
  return vf_no;
}

vf_status test_imgdir(unsigned char *, int, string filename) {
  IMG_header ihead;
  Cube dummy;
  struct stat st;
  if (!stat(filename.c_str(), &st)) {
    if (!(S_ISDIR(st.st_mode))) return vf_no;
  }
  string pat = img_patfromname(filename);
  vglob vg(pat);
  // no match, go home
  if (vg.size() < 2) return vf_no;
  string firstfile = vg[0];
  if (analyze_read_header(firstfile, &ihead, NULL)) return vf_no;
  // it needs to be 3D, because we're reading multiple files
  if (ihead.dim[0] == 3) return vf_yes;
  if (ihead.dim[0] == 4 && ihead.dim[4] == 1) return vf_yes;
  return vf_no;
}

int read_head_img4d(Tes *tes) {
  int err = analyze_read_header(tes->GetFileName(), NULL, tes);
  return err;
}

int read_head_imgdir(Tes *tes) {
  string pat = img_patfromname(tes->GetFileName());
  vglob vg(pat);
  if (vg.size() == 0) return 106;
  int err = analyze_read_header(vg[0], NULL, tes);
  tes->dimt = vg.size();
  return err;
}

int read_data_imgdir(Tes *tes, int start, int count) {
  Cube cb;
  // honor volume range
  if (start == -1) {
    start = 0;
    count = tes->dimt;
  }
  if (start + count > tes->dimt) return 220;
  tes->dimt = count;

  string fname = tes->GetFileName();
  string pat = img_patfromname(fname);
  tokenlist filenames = vglob(pat);

  if (filenames.size() - 1 < (uint32)(start + count - 1)) return 110;
  for (int i = start; i < start + count; i++) {
    cb.SetFileName(filenames[i]);
    if (read_head_img3d(&cb)) {
      tes->invalidate();
      return 101;
    }
    if (i == 0) {
      tes->SetVolume(cb.dimx, cb.dimy, cb.dimz, tes->dimt, cb.datatype);
      if (!tes->data) return 120;
      tes->voxsize[0] = cb.voxsize[0];
      tes->voxsize[1] = cb.voxsize[1];
      tes->voxsize[2] = cb.voxsize[2];
      tes->origin[0] = cb.origin[0];
      tes->origin[1] = cb.origin[1];
      tes->origin[2] = cb.origin[2];
      tes->filebyteorder = cb.filebyteorder;
      tes->header = cb.header;
    }
    if (read_data_img3d(&cb)) {
      tes->invalidate();
      return 102;
    }
    tes->SetCube(i, cb);
    tes->AddHeader((string) "vb2tes_filename: " + filenames[i]);
  }
  tes->Remask();
  return (0);  // no error!
}

int read_data_img4d(Tes *ts, int start, int count) {
  string imgname = ts->GetFileName();
  string ext = xgetextension(imgname);
  if (ext == "hdr")
    imgname = xsetextension(imgname, "img");
  else if (ext != "img")
    return 104;

  // we must have previously read a valid header
  if (ts->dimx < 1 || ts->dimy < 1 || ts->dimz < 1 || ts->dimt < 1) {
    ts->data_valid = 0;  // make sure
    return 105;
  }
  // honor volume range
  if (start == -1) {
    start = 0;
    count = ts->dimt;
  } else if (start + count > ts->dimt)
    return 220;
  ts->dimt = count;
  // create 4d volume
  ts->SetVolume(ts->dimx, ts->dimy, ts->dimz, ts->dimt, ts->datatype);
  if (!ts->data) return 110;
  FILE *fp = fopen(imgname.c_str(), "r");
  if (!fp) {
    ts->invalidate();
    return (119);
  }
  // misnomer, actually the number of voxels per volume
  int bytelen = ts->dimx * ts->dimy * ts->dimz;
  Cube cb(ts->dimx, ts->dimy, ts->dimz, ts->datatype);
  // skip to the first volume requested
  fseek(fp, start * bytelen * cb.datasize, SEEK_CUR);
  for (int i = 0; i < ts->dimt; i++) {
    int cnt = fread(cb.data, cb.datasize, bytelen, fp);
    if (cnt < bytelen) {
      fclose(fp);
      ts->invalidate();
      return 122;
    }
    ts->SetCube(i, cb);
  }
  fclose(fp);
  if (my_endian() != ts->filebyteorder) ts->byteswap();
  if (ts->f_scaled) {
    if (ts->datatype == vb_byte || ts->datatype == vb_short ||
        ts->datatype == vb_long)
      ts->convert_type(vb_float);
    *ts *= ts->scl_slope;
    *ts += ts->scl_inter;
  }
  ts->data_valid = 1;
  return 0;
}

int write_img4d(Tes *) { return 104; }

int write_imgdir(Tes *mytes) {
  char outname[STRINGLEN];
  struct stat st;
  int err;

  mkdir(mytes->GetFileName().c_str(), 0777);
  err = stat(mytes->GetFileName().c_str(), &st);
  if (err) return (100);
  if (!(S_ISDIR(st.st_mode))) return 101;
  for (int i = 0; i < mytes->dimt; i++) {
    Cube *cube = new Cube((*mytes)[i]);
    sprintf(outname, "%s/%s%.3d.img", mytes->GetFileName().c_str(),
            xfilename(mytes->GetFileName()).c_str(), i);
    cube->SetFileFormat("img3d");
    cube->SetFileName(outname);
    if (cube->WriteFile()) {
      delete cube;
      return 105;
    }
    delete cube;
  }
  return (0);  // no error!
}

string img_patfromname(const string fname) {
  struct stat st;

  string pat = fname;
  // if it's a stem
  if (stat(pat.c_str(), &st)) pat += "*.img";
  // if it's a dir
  else if (S_ISDIR(st.st_mode))
    pat += "/*.img";
  return pat;
}

}  // extern "C"
