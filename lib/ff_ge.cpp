
// ff_ge.cpp
// VoxBo I/O plug-in for GE 3D file directories (I.xxx)
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "vbio.h"
#include "vbutil.h"

extern "C" {

vf_status test_ge3_3D(unsigned char *buf, int bufsize, string filename);
int read_head_ge3_3D(Cube *cb);
int read_data_ge3_3D(Cube *cb);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF ge_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "GE I.XXX";
  tmp.extension = "I.*";
  tmp.signature = "ge3";
  tmp.dimensions = 3;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_3D = test_ge3_3D;
  tmp.read_head_3D = read_head_ge3_3D;
  tmp.read_data_3D = read_data_ge3_3D;
  return tmp;
}

// MUST PROVIDE THE FOLLOWING FOR 3D TYPES
// vf_status test_sig_3D(char *,int,string);
// int test_sig_3D(Cube *);
// int read_head_sig_3D(Cube *);
// int read_data_sig_3D(Cube *);
// int write_sig_3D(Cube *);
// vf_datatype datatype;
// int dimensions;

string ge_patfromname(const string fname);

vf_status test_ge3_3D(unsigned char *, int, string filename) {
  if (vglob(filename + "/I.[0-9][0-9][0-9]").size())
    return vf_yes;
  else
    return vf_no;
}

int read_head_ge3_3D(Cube *cb) {
  int img;
  int16 xsize, ysize, rgain1, rgain2, tgain;
  int32 TR, TE, TI, TE2, imgoff, hdroff, seriesptr;
  short nechoes, plane;
  FILE *ifile;
  float dims[6], xfov, yfov, nex, spacing;
  char tmp[STRINGLEN], hdr[STRINGLEN];
  char startras[2], endras[2];
  float startloc, endloc;

  cb->dimx = cb->dimy = cb->dimz = 0;
  cb->data_valid = cb->header_valid = 0;

  string pattern = ge_patfromname(cb->GetFileName());
  vglob vg(pattern);
  if (vg.size() == 0) return 105;

  // not really looping below!  we just use the first image for the
  // header
  for (img = 0; img < 1; img++) {
    ifile = fopen(vg[img].c_str(), "r");
    if (!ifile) continue;

    // find image offset
    fseek(ifile, 4, SEEK_SET);
    fread(&imgoff, sizeof(int32), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&imgoff);

    // find series offset
    fseek(ifile, 140, SEEK_SET);
    fread(&seriesptr, sizeof(int32), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&seriesptr);

    // find header offset
    fseek(ifile, 148, SEEK_SET);
    fread(&hdroff, sizeof(int32), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&hdroff);

    // find size
    fseek(ifile, hdroff + 30, SEEK_SET);
    fread(&xsize, sizeof(int16), 1, ifile);
    fread(&ysize, sizeof(int16), 1, ifile);
    if (my_endian() != ENDIAN_BIG) {
      swap(&xsize);
      swap(&ysize);
    }
    fseek(ifile, hdroff + 34, SEEK_SET);
    fread(dims, sizeof(float), 6, ifile);
    if (my_endian() != ENDIAN_BIG) swap(dims, 6);
    xfov = (int)dims[0];
    yfov = (int)dims[1];

    // sanity checking
    if (xsize < 1 || ysize < 1 || xsize > 2048 || ysize > 2048) break;

    // plane is at series + 72

    // get all the basic statistics
    cb->dimx = xsize;
    cb->dimy = ysize;
    cb->dimz = vg.size();
    cb->SetDataType(vb_short);
    cb->voxsize[0] = dims[4];
    cb->voxsize[1] = dims[5];
    // stuff from the series hdr
    fseek(ifile, seriesptr + 72, SEEK_SET);
    fread(&plane, sizeof(int16), 1, ifile);
    fseek(ifile, seriesptr + 120, SEEK_SET);
    fread(&startras, sizeof(char), 2, ifile);
    fread(&startloc, sizeof(float), 1, ifile);
    fread(&endras, sizeof(char), 2, ifile);
    fread(&endloc, sizeof(float), 1, ifile);
    // stuff from the img hdr
    fseek(ifile, hdroff + 26, SEEK_SET);
    fread(cb->voxsize + 2, sizeof(float), 1, ifile);
    fseek(ifile, hdroff + 116, SEEK_SET);
    fread(&spacing, sizeof(float), 1, ifile);

    // NEW STUFF
    char im_loc_ras[2];
    float im_loc;
    float im_ctr[3];
    float im_norm[3];
    float im_tlhc[3];
    float im_trhc[3];
    float im_brhc[3];

    fseek(ifile, hdroff + 124, SEEK_SET);
    fread(im_loc_ras, sizeof(char), 2, ifile);
    fread(&im_loc, sizeof(float), 1, ifile);
    fread(im_ctr, sizeof(float), 3, ifile);
    fread(im_ctr, sizeof(float), 3, ifile);
    fread(im_norm, sizeof(float), 3, ifile);
    fread(im_tlhc, sizeof(float), 3, ifile);
    fread(im_trhc, sizeof(float), 3, ifile);
    fread(im_brhc, sizeof(float), 3, ifile);
    if (my_endian() != ENDIAN_BIG) {
      swap(&im_loc, 1);
      swap(im_ctr, 3);
      swap(im_norm, 3);
      swap(im_tlhc, 3);
      swap(im_trhc, 3);
      swap(im_brhc, 3);
    }
    // AGE?
    // patname is plus 97
    // cout << seriesptr << endl;
    fseek(ifile, seriesptr - 1024 + 97 + 25, SEEK_SET);
    short patage;
    fread(&patage, sizeof(int16), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&patage);
    // cout << patage << endl;

    // BACK TO OLD STUFF
    fseek(ifile, hdroff + 194, SEEK_SET);
    fread(&TR, sizeof(int32), 1, ifile);
    fread(&TI, sizeof(int32), 1, ifile);
    fread(&TE, sizeof(int32), 1, ifile);
    fread(&TE2, sizeof(int32), 1, ifile);
    fread(&nechoes, sizeof(int16), 1, ifile);
    fseek(ifile, hdroff + 218, SEEK_SET);
    fread(&nex, sizeof(float), 1, ifile);
    fseek(ifile, hdroff + 248, SEEK_SET);
    fread(&tgain, sizeof(int16), 1, ifile);
    fread(&rgain1, sizeof(int16), 1, ifile);
    fread(&rgain2, sizeof(int16), 1, ifile);
    if (my_endian() != ENDIAN_BIG) {
      swap(&TR);
      swap(&TI);
      swap(&TE);
      swap(&TE2);
      swap(&nechoes);
      swap(&nex);
      swap(&tgain);
      swap(&rgain1);
      swap(&rgain2);
      swap(cb->voxsize + 2, 1);
      swap(&startloc, 1);
      swap(&endloc, 1);
      swap(&spacing, 1);
      swap(&plane, 1);
    }
    cb->voxsize[2] += spacing;
    fseek(ifile, hdroff + 308, SEEK_SET);
    fgets(tmp, 33, ifile);
    tmp[33] = '\0';
    sprintf(hdr, "Pulsesequence:\t%s", tmp);
    cb->AddHeader(hdr);

    sprintf(hdr, "TR(usecs):\t%d", (int)TR);
    cb->AddHeader(hdr);
    // sprintf(hdr,"StartRAS:\t%c",startras[0]);  cb->AddHeader(hdr);
    // sprintf(hdr,"EndRAS:\t%c",endras[0]);  cb->AddHeader(hdr);

    if (startras[0] == 'I' || startras[0] == 'S') {
      if (startloc < endloc)
        cb->orient = "RPI";
      else
        cb->orient = "RPS";
    } else if (startras[0] == 'L' || startras[0] == 'R') {
      if (startloc < endloc)
        cb->orient = "AIL";
      else
        cb->orient = "AIR";
    } else {
      if (startloc < endloc)
        cb->orient = "RIP";
      else
        cb->orient = "RIA";
    }

    // sprintf(hdr,"Orientation:\t%s",cb->orient.c_str());  cb->AddHeader(hdr);
    sprintf(hdr, "ZRange:\t%f\t%f", startloc, endloc);
    cb->AddHeader(hdr);
    sprintf(hdr, "FOV:\t%.2fx%.2f", xfov, yfov);
    cb->AddHeader(hdr);
    sprintf(hdr, "Scaninfo: TE=%d nechoes=%d nex=%.1f TG=%d RG=%d/%d", (int)TE,
            (int)nechoes, nex, (int)tgain, (int)rgain1, (int)rgain2);
    cb->AddHeader(hdr);
    // sprintf(hdr,"im_loc_ras: %c %c",im_loc_ras[0],im_loc_ras[1]);
    // cb->AddHeader(hdr); sprintf(hdr,"im_loc: %f",im_loc); cb->AddHeader(hdr);
    // sprintf(hdr,"im_ctr: %f %f %f",im_ctr[0],im_ctr[1],im_ctr[2]);
    // cb->AddHeader(hdr); sprintf(hdr,"im_norm: %f %f
    // %f",im_norm[0],im_norm[1],im_norm[2]);  cb->AddHeader(hdr);
    // sprintf(hdr,"im_tlhc: %f %f %f",im_tlhc[0],im_tlhc[1],im_tlhc[2]);
    // cb->AddHeader(hdr); sprintf(hdr,"im_trhc: %f %f
    // %f",im_trhc[0],im_trhc[1],im_trhc[2]);  cb->AddHeader(hdr);
    // sprintf(hdr,"im_brhc: %f %f %f",im_brhc[0],im_brhc[1],im_brhc[2]);
    // cb->AddHeader(hdr);
    sprintf(hdr, "AbsoluteCornerPosition: %f %f %f", 0.0 - im_tlhc[0],
            im_tlhc[1], im_tlhc[2]);
    cb->AddHeader(hdr);
    sprintf(hdr, "PatientAge: %d", patage);
    cb->AddHeader(hdr);

    fclose(ifile);
  }
  if (cb->dimx) cb->header_valid = 1;
  return (0);  // no error!
}

int read_data_ge3_3D(Cube *cb) {
  int32 j, ind, imgoff, hdroff;
  int16 xsize, ysize;
  FILE *ifile;

  vglob vg(ge_patfromname(cb->GetFileName()));
  if (vg.size() == 0) return 115;
  cb->SetVolume(cb->dimx, cb->dimy, vg.size(), vb_short);
  if (!cb->data_valid) return 120;

  for (size_t img = 0; img < vg.size(); img++) {
    ifile = fopen(vg[img].c_str(), "r");
    if (!ifile) continue;

    // find image offset
    fseek(ifile, 4, SEEK_SET);
    fread(&imgoff, sizeof(int32), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&imgoff);

    // find header offset
    fseek(ifile, 148, SEEK_SET);
    fread(&hdroff, sizeof(int32), 1, ifile);
    if (my_endian() != ENDIAN_BIG) swap(&hdroff);

    // find size
    fseek(ifile, hdroff + 30, SEEK_SET);
    fread(&xsize, sizeof(int16), 1, ifile);
    fread(&ysize, sizeof(int16), 1, ifile);
    if (my_endian() != ENDIAN_BIG) {
      swap(&xsize);
      swap(&ysize);
    }

    // sanity checking
    if (xsize < 1 || ysize < 1 || xsize > 1024 || ysize > 1024) continue;

    // read in the actual data

    fseek(ifile, imgoff, SEEK_SET);
    for (j = 0; j < cb->dimy; j++) {
      // find the right plane
      ind = cb->dimx * cb->dimy * img;
      // select the row (flipped top to bottom)
      ind += cb->dimx * (cb->dimy - j - 1);
      fread(((int16 *)(cb->data)) + ind, sizeof(int16), cb->dimx, ifile);
      if (my_endian() != ENDIAN_BIG)
        swap(((int16 *)(cb->data)) + ind, cb->dimx);
    }
    fclose(ifile);
  }
  cb->data_valid = 1;
  return 0;
}

int write_ge3_3D(Cube *) { return 1; }

// if the filename is a directory, we look for filename/I.*; if it's
// a file; we just use it; if err, we use filename*

string ge_patfromname(const string fname) {
  struct stat st;

  string pat = fname;
  if (stat(pat.c_str(), &st))
    pat += "*";
  else if (S_ISDIR(st.st_mode)) {
    pat += "/I.*";
  }
  return pat;
}

}  // extern "C"
