
// ff_mat.cpp
// I/O code for VoxBo 2D file formats (mat1, text)
// Copyright (c) 2010 by The VoxBo Development Team

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
#include <fstream>
#include "vbio.h"
#include "vbutil.h"

extern "C" {

vf_status mat1_test(unsigned char *buf, int bufsize, string filename);
int mat1_read_head(VBMatrix *mat);
int mat1_read_data(VBMatrix *mat, uint32 r1, uint32 rn, uint32 c1, uint32 cn);
int mat1_write(VBMatrix *mat);

vf_status mtx_test(unsigned char *buf, int bufsize, string filename);
int mtx_read_head(VBMatrix *mat);
int mtx_read_data(VBMatrix *mat, uint32 r1, uint32 rn, uint32 c1, uint32 cn);
int mtx_write(VBMatrix *mat);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF mat1_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "VoxBo MAT1";
  tmp.extension = "mat";
  tmp.signature = "mat1";
  tmp.dimensions = 2;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_2D = mat1_test;
  tmp.read_head_2D = mat1_read_head;
  tmp.read_data_2D = mat1_read_data;
  tmp.write_2D = mat1_write;
  return tmp;
}

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF mtx_vbff()
#endif
{
  VBFF tmp;
  tmp.name = "VoxBo MATtext (aka mtx)";
  tmp.extension = "mtx";
  tmp.signature = "mtx";
  tmp.dimensions = 2;
  tmp.version_major = vbversion_major;
  tmp.version_minor = vbversion_minor;
  tmp.test_2D = mtx_test;
  tmp.read_head_2D = mtx_read_head;
  tmp.read_data_2D = mtx_read_data;
  tmp.write_2D = mtx_write;
  return tmp;
}

int mat1_read_head(VBMatrix *mat) {
  mat->clear();
  char line[STRINGLEN];
  string keyword;
  tokenlist args;

  mat->matfile = fopen(mat->filename.c_str(), "r");
  if (!mat->matfile) return (101);
  while (fgets(line, STRINGLEN, mat->matfile)) {
    if (line[0] == 12) break;
    stripchars(line, "\n");
    args.ParseLine(line);
    keyword = args[0];
    // discard trailing colons
    if (keyword[keyword.size() - 1] == ':')
      keyword.replace(keyword.size() - 1, 1, "");
    // parse known headers
    if (equali(keyword, "voxdims(xy)") && args.size() > 2) {
      mat->m = strtol(args[2]);
      mat->n = strtol(args[1]);
      continue;
    }
    if (equali(keyword, "byteorder") && args.size() > 1) {
      if (equali(args[1], "msbfirst"))
        mat->filebyteorder = ENDIAN_BIG;
      else if (equali(args[1], "lsbfirst"))
        mat->filebyteorder = ENDIAN_LITTLE;
      continue;
    }
    if (equali(keyword, "datatype") && args.size() > 1) {
      parsedatatype(args[1], mat->datatype, mat->datasize);
      continue;
    }
    mat->AddHeader(line);
  }
  mat->offset = ftell(mat->matfile);
  fclose(mat->matfile);
  mat->matfile = (FILE *)NULL;
  return 0;
}

int mat1_read_data(VBMatrix *mat, uint32 r1, uint32 rn, uint32 c1, uint32 cn) {
  if (mat->rowdata) delete[] mat->rowdata;
  mat->rowdata = (double *)NULL;
  if (!(mat->headerValid()) || mat->filename.size())
    if (mat1_read_head(mat)) return (110);
  if (!(mat->headerValid())) return 211;
  uint32 rowcount = mat->m;
  uint32 colcount = mat->n;
  if (r1 == 0 && rn == 0)
    r1 = 0, rn = rowcount;
  else
    rowcount = rn - r1 + 1;
  if (c1 == 0 && cn == 0)
    c1 = 0, cn = colcount;
  else
    colcount = cn - c1 + 1;

  mat->rowdata = new double[rowcount * colcount];
  assert(mat->rowdata);
  // open matfile for read
  if ((mat->matfile = fopen(mat->filename.c_str(), "r")) == NULL) return 103;
  fseek(mat->matfile, mat->offset, SEEK_SET);

  // handle row subset
  if (rowcount != mat->m)
    fseek(mat->matfile, r1 * mat->n * mat->datasize, SEEK_CUR);
  // all cols, so we can grab the whole block
  if (colcount == mat->n) {
    int cnt =
        fread(mat->rowdata, mat->datasize, rowcount * colcount, mat->matfile);
    if (cnt < (int)(rowcount * colcount)) {
      mat->clear();
      return 154;
    }
  }
  // subset of cols, so we have to keep seeking
  else {
    fseek(mat->matfile, mat->datasize * c1, SEEK_CUR);  // skip first slug
    for (uint32 i = 0; i < rowcount; i++) {
      int cnt = fread(mat->rowdata + (i * colcount), mat->datasize, colcount,
                      mat->matfile);
      if (cnt < (int)colcount) {
        mat->clear();
        return 155;
      }
      fseek(mat->matfile, mat->datasize * (mat->n - colcount), SEEK_CUR);
    }
  }
  mat->rows = rowcount;
  mat->cols = colcount;
  fclose(mat->matfile);
  mat->matfile = (FILE *)NULL;
  // swap appropriately for input data size
  if (my_endian() != mat->filebyteorder)
    swapn((unsigned char *)mat->rowdata, mat->datasize, mat->m * mat->n);
  mat->float2double();
  mat->mview = gsl_matrix_view_array(mat->rowdata, mat->rows, mat->cols);
  return 0;
}

int mat1_write(VBMatrix *mat) {
  if (mat->matfile) fclose(mat->matfile);
  mat->matfile = fopen(mat->filename.c_str(), "w+");
  if (!mat->matfile) return 101;
  // setbuf(mat->matfile,NULL);
  fprintf(mat->matfile, "VB98\nMAT1\n");
  fprintf(mat->matfile, "DataType:\tDouble\n");
  fprintf(mat->matfile, "VoxDims(XY):\t%d\t%d\n", mat->n, mat->m);
  fprintf(mat->matfile, "# NOTE: first dim is cols and the second is rows\n");
  fprintf(mat->matfile, "Byteorder:\tmsbfirst\n");
  // user headers
  for (size_t i = 0; i < mat->header.size(); i++)
    fprintf(mat->matfile, "%s\n", mat->header[i].c_str());
  // separator before data
  fprintf(mat->matfile, "%c\n", 12);
  mat->offset = ftell(mat->matfile);

  unsigned int size = mat->m * mat->n;
  // fseek(mat->matfile,offset,SEEK_SET);
  if (my_endian() != mat->filebyteorder) swap(mat->rowdata, size);
  if (fwrite(mat->rowdata, sizeof(double), mat->m * mat->n, mat->matfile) <
      size)
    return (103);
  if (my_endian() != mat->filebyteorder) swap(mat->rowdata, size);
  fclose(mat->matfile);
  mat->matfile = (FILE *)NULL;
  return (0);
}

vf_status mat1_test(unsigned char *buf, int bufsize, string) {
  if (bufsize < 20) return vf_no;
  tokenlist args;
  args.SetSeparator("\n");
  args.ParseLine((char *)buf);
  if (args[0] != "VB98" || args[1] != "MAT1") return vf_no;
  return vf_yes;
}

vf_status mtx_test(unsigned char *, int,
                   string fname)  // buf and bufsize unused
{
  VBMatrix mat;
  mat.filename = fname;
  if (mtx_read_data(&mat, 0, 9, 0, 0)) return vf_no;
  return vf_yes;
}

int mtx_read_head(VBMatrix *mat) {
  // FIXME right now we just have to read the whole file, to get the
  // number of rows
  return mtx_read_data(mat, 0, 0, 0, 0);
}

int mtx_read_data(VBMatrix *mat, uint32 r1, uint32 rn, uint32 c1, uint32 cn) {
  // note that rown/coln is the index of the last row/col, not the count
  uint32 rows = 0, cols = 0;
  vector<double> myarray;
  const int BUFSZ = 1024 * 1024 * 10;  // arbitrary 10MB row size limit
  char *buf;  // buf allocated on heap, not stack, because it's big
  buf = new char[BUFSZ];
  if (!buf) return 99;
  ifstream ifile;
  tokenlist ltoks;
  ltoks.SetSeparator(" \t,\n\r");
  ifile.open(mat->filename.c_str(), ios::in);
  if (ifile.fail()) {
    delete[] buf;
    return 222;
  }
  while (ifile.getline(buf, BUFSZ)) {
    // comments are headers
    if (buf[0] == '#' || buf[0] == '%' || buf[0] == ';') {
      mat->AddHeader(xstripwhitespace(buf + 1));
      continue;
    }
    ltoks.ParseLine(buf);
    // skip blank lines
    if (ltoks.size() == 0) continue;
    if (cols == 0) cols = ltoks.size();
    if (cols != ltoks.size()) {
      ifile.close();
      delete[] buf;
      return 101;
    }
    pair<bool, double> num;
    for (size_t i = 0; i < ltoks.size(); i++) {
      num = strtodx(ltoks[i]);
      if (num.first) {
        ifile.close();
        delete[] buf;
        return 102;
      }
      myarray.push_back(num.second);
    }
    rows++;
    if (rn > 0 && rows > rn) break;  // we have all the rows we need
  }
  delete[] buf;
  ifile.close();
  if (rows == 0 || cols == 0) return 171;
  if (c1 == 0 && cn == 0) c1 = 0, cn = cols - 1;
  if (r1 == 0 && rn == 0) r1 = 0, rn = rows - 1;
  if (r1 > rows - 1) r1 = rows - 1;
  if (rn > rows - 1) rn = rows - 1;
  if (c1 > cols - 1) c1 = cols - 1;
  if (cn > cols - 1) cn = cols - 1;
  mat->init(rn - r1 + 1, cn - c1 + 1);
  size_t ind = 0;
  for (uint32 i = 0; i < rows; i++) {
    for (uint32 j = 0; j < cols; j++) {
      if (i >= r1 && i <= rn && j >= c1 && j <= cn)
        mat->set(i - r1, j - c1, myarray[ind]);
      ind++;
    }
  }
  return 0;
}

int mtx_write(VBMatrix *mat) {
  if (mat->matfile) fclose(mat->matfile);
  mat->matfile = fopen(mat->filename.c_str(), "w+");
  if (!mat->matfile) return 101;
  // setbuf(mat->matfile,NULL);
  fprintf(mat->matfile, "# VB98\n# MTX\n");
  fprintf(mat->matfile, "# dims: %d %d\n", mat->m, mat->n);
  fprintf(mat->matfile, "# NOTE: first dim is rows and the second is cols\n");
  // user headers
  for (size_t i = 0; i < mat->header.size(); i++)
    fprintf(mat->matfile, "# %s\n", mat->header[i].c_str());
  // separator before data

  for (uint32 i = 0; i < mat->m; i++) {
    for (uint32 j = 0; j < mat->n; j++) {
      if (fprintf(mat->matfile, "%.5f ", (*mat)(i, j)) < 0) {
        fclose(mat->matfile);
        return 101;
      }
    }
    if (fprintf(mat->matfile, "\n") < 0) {
      fclose(mat->matfile);
      return 102;
    }
  }
  fclose(mat->matfile);
  mat->matfile = (FILE *)NULL;
  return (0);
}

}  // extern "C"
