
// ff_da.cpp
// VoxBo file I/O code for a locally used file format
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vbio.h"
#include "vbutil.h"

extern "C" {

// conditionalize the function name to prevent conflicts between
// fileformats when it's not used as a plug-in

vf_status test_da4_4D(unsigned char *buf, int bufsize, string filename);
int read_head_da4_4D(Tes *mytes);
int read_data_da4_4D(Tes *mytes, int start = -1, int count = -1);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF da4_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "Penn/GE .im";
  tmp.extension = "im";
  tmp.signature = "da4";
  tmp.dimensions = 4;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_4D = test_da4_4D;
  tmp.read_head_4D = read_head_da4_4D;
  tmp.read_data_4D = read_data_da4_4D;
  return tmp;
}

int ReadGEHeader(Tes *tes, const string &filename);
int ReadDA(Tes *tes, const string &filename);

vf_status test_da4_4D(unsigned char *buf, int bufsize, string filename) {
  if (filename.substr(filename.size() - 3) != ".im") return vf_no;
  Tes tes;
  if (ReadGEHeader(&tes, filename)) return vf_no;
  if (tes.dimt < 1) return vf_no;
  return vf_yes;
}

int read_head_da4_4D(Tes *tes) {
  int err = ReadGEHeader(tes, tes->GetFileName());
  return err;
}

int read_data_da4_4D(Tes *tes, int start, int count) {
  return ReadDA(tes, tes->GetFileName());
}

int ReadDA(Tes *tes, const string &filename) {
  tes->data_valid = 0;
  FILE *fp;
  fp = fopen(filename.c_str(), "r");
  // ReadGEHeader(fp);
  tes->SetVolume(tes->dimy, tes->dimx, tes->dimz, tes->dimt, vb_short);

  // let's make an array for slice order
  fseek(fp, 10240, SEEK_SET);
  int16 slicenum[tes->dimz], tmps;
  unsigned char buf[40];
  for (int i = 0; i < tes->dimz; i++) {
    fread(buf, 40, 1, fp);
    tmps = *((int16 *)(buf + 2));
    if (my_endian() != ENDIAN_BIG) swap(&tmps);
    tmps--;
    if (tmps < tes->dimz) slicenum[tmps] = i;
  }

  // first 39940 is the header, so let's get past that
  fseek(fp, 39940, SEEK_SET);

  int xx, yy;
  int32 voxcount[tes->dimz];
  fread(&voxcount, sizeof(int32), tes->dimz,
        fp);  // how many voxels in this slice?
  if (my_endian() != ENDIAN_BIG) swap(voxcount, tes->dimz);
  int zz, zzz;
  for (zz = 0; zz < tes->dimz; zz++) {
    zzz = slicenum[zz];
    if (voxcount[zz] == 0) continue;
    if (voxcount[zz] < 0 ||
        voxcount[zz] > tes->dimx * tes->dimy)  // sanity check voxel count
      return 150;
    int32 planepositions[voxcount[zz]];
    fread(planepositions, sizeof(int32), voxcount[zz],
          fp);  // positions of the voxels
    if (my_endian() != ENDIAN_BIG) swap(planepositions, voxcount[zz]);
    int16 planeseries[voxcount[zz]];
    for (int tt = 0; tt < tes->dimt; tt++) {
      fread(planeseries, sizeof(int16), voxcount[zz], fp);
      if (my_endian() != ENDIAN_BIG) swap(planeseries, voxcount[zz]);
      for (int i = 0; i < voxcount[zz]; i++) {
        if (planepositions[i] > tes->dimx * tes->dimy) continue;
        xx = planepositions[i] / tes->dimy;
        yy = tes->dimy - (planepositions[i] % tes->dimy) - 1;
        tes->SetValue(xx, yy, zzz, tt, planeseries[i]);
      }
    }
  }
  fclose(fp);
  tes->data_valid = 1;
  return (0);  // no error!
}

int ReadGEHeader(Tes *tes, const string &filename) {
  unsigned char buf[4096];
  int16 bv, ps, hn;
  int32 rps;
  FILE *ifile;

  tes->data_valid = 0;
  tes->header_valid = 0;
  if (!(ifile = fopen(filename.c_str(), "r"))) return 120;

  // 256 is more than enough for the first bit
  if (fread(buf, 256, 1, ifile) != 1) {
    fclose(ifile);
    return 100;
  }

  // stuff needed to calculate nimages below
  bv = *((int16 *)(buf + 76));
  hn = *((int16 *)(buf + 78));
  ps = *((int16 *)(buf + 82));
  rps = *((int32 *)(buf + 116));
  if (my_endian() != ENDIAN_BIG) {
    swap(&bv);
    swap(&ps);
    swap(&hn);
    swap(&rps);
  }

  // put these fields in place as though they were shorts
  // 68: nslices, aka dimz
  *(&tes->dimz) = *((int16 *)(buf + 68));
  // 74: nframes, aka dimy
  *(&tes->dimy) = *((int16 *)(buf + 74));
  // 80: frame size, aka dimx
  *(&tes->dimx) = *((int16 *)(buf + 80));

  if (my_endian() != ENDIAN_BIG) {
    swap((int16 *)&tes->dimx);
    tes->dimx = *((int16 *)&tes->dimx);
    swap((int16 *)&tes->dimy);
    tes->dimy = *((int16 *)&tes->dimy);
    swap((int16 *)&tes->dimz);
    tes->dimz = *((int16 *)&tes->dimz);
    swap((int16 *)&tes->dimt);
    tes->dimt = *((int16 *)&tes->dimt);
  }

  // the magic formula for the number of images
  // note that dimx and dimy are swapped, because we're flipping it
  tes->dimt =
      rps / (2 * ps * (1 + bv + tes->dimy + hn) * tes->dimz * tes->dimx);

  int offset = 2048 + 4096 + 4096 + 20480 + 2052 + 2052 + 2048 + 1024 + 1020;
  float dims[6], spacing;

  int16 rgain1, rgain2, tgain;
  int32 TR, TE, TI, TE2;
  int16 nechoes, xsize, ysize, plane;
  float xfov, yfov, nex;
  char tmp[STRINGLEN], hdr[STRINGLEN];
  char startras[2];  // ,endras[2];
  float startloc, endloc;

  fseek(ifile, offset + 26, SEEK_SET);
  fread(tes->voxsize + 2, sizeof(float), 1, ifile);
  fseek(ifile, offset + 116, SEEK_SET);
  fread(&spacing, sizeof(float), 1, ifile);
  if (my_endian() != ENDIAN_BIG) {
    swap(tes->voxsize + 2, 1);
    swap(&spacing, 1);
  }

  fseek(ifile, offset + 34, SEEK_SET);
  fread(dims, sizeof(float), 6, ifile);
  if (my_endian() != ENDIAN_BIG) swap(dims, 6);
  tes->voxsize[0] = dims[0] / tes->dimx;
  tes->voxsize[1] = dims[1] / tes->dimy;
  tes->voxsize[2] += spacing;
  xfov = (int32)dims[0];
  yfov = (int32)dims[1];

  // find size
  fseek(ifile, offset + 30, SEEK_SET);
  fread(&xsize, sizeof(int16), 1, ifile);
  fread(&ysize, sizeof(int16), 1, ifile);
  if (my_endian() != ENDIAN_BIG) {
    swap(&xsize);
    swap(&ysize);
  }
  // cout << xsize << " " << ysize << endl;

  fseek(ifile, offset + 194, SEEK_SET);
  fread(&TR, sizeof(int32), 1, ifile);
  fread(&TI, sizeof(int32), 1, ifile);
  fread(&TE, sizeof(int32), 1, ifile);
  fread(&TE2, sizeof(int32), 1, ifile);
  fread(&nechoes, sizeof(int16), 1, ifile);
  fseek(ifile, offset + 218, SEEK_SET);
  fread(&nex, sizeof(float), 1, ifile);
  fseek(ifile, offset + 248, SEEK_SET);
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
    // swap(voxsize+2,1);
    swap(&startloc, 1);
    swap(&endloc, 1);
    // swap(&spacing,1);
    swap(&plane, 1);
  }
  // tes->voxsize[2]+=spacing;
  fseek(ifile, offset + 308, SEEK_SET);
  fgets(tmp, 33, ifile);
  tmp[33] = '\0';
  sprintf(hdr, "Pulsesequence:\t%s", tmp);
  tes->AddHeader(hdr);

  sprintf(hdr, "TR(msecs):\t%ld", TR / 1000);
  tes->AddHeader(hdr);
  // sprintf(hdr,"StartRAS:\t%c",startras[0]);  tes->AddHeader(hdr);
  // sprintf(hdr,"EndRAS:\t%c",endras[0]);  tes->AddHeader(hdr);

  if (startras[0] == 'I' || startras[0] == 'S') {
    if (startloc < endloc)
      tes->orient = "RPI";
    else
      tes->orient = "RPS";
  } else if (startras[0] == 'L' || startras[0] == 'R') {
    if (startloc < endloc)
      tes->orient = "AIL";
    else
      tes->orient = "AIR";
  } else {
    if (startloc < endloc)
      tes->orient = "RIP";
    else
      tes->orient = "RIA";
  }
  tes->orient = "RPI";  // FIXME must be hardcoded?

  sprintf(hdr, "Orientation:\t%s", tes->orient.c_str());
  tes->AddHeader(hdr);
  sprintf(hdr, "ZRange:\t%f\t%f", startloc, endloc);
  tes->AddHeader(hdr);
  sprintf(hdr, "FOV:\t%.2fx%.2f", xfov, yfov);
  tes->AddHeader(hdr);
  sprintf(hdr, "Scaninfo: TE=%ld nechoes=%d nex=%.1f TG=%d RG=%d/%d", TE,
          nechoes, nex, tgain, rgain1, rgain2);
  tes->AddHeader(hdr);

  // FIXME - sanity checking
  fclose(ifile);
  tes->header_valid = 1;
  return (0);  // no error!
}
}
