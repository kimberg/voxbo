
// vbcrunch.h
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

// vbcrunch.h
// headers needed for crunching stuff

using namespace std;

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>

#include "vbutil.h"
#include "vbio.h"
#include <oct.h>

#define PI 3.14159265358979
#define MAGIC 110494
#define TINY 0.05

#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)>(b)) ? (b) : (a))
#endif

// the crunchcube class is an inheritor of the Cube class that adds a
// few data structures for doing things like realignment, etc.  I'd
// have put these in the parent class but this way the i/o lib doesn't
// require matrix structures, etc.

class CrunchCube : public Cube {
 public:
  Matrix M,savedM,M1;
  RowVector transform;
  CrunchCube();
  CrunchCube(const Cube &cub);
  void Init();
  void Preview(char *title);
};

// prototypes from dan_smooth.c

void convxy(unsigned char *pl,int dimx,int dimy,
	    const RowVector &filtx,const RowVector &filty,
	    int fxdim,int fydim,int xoff,
	    int yoff,double *out,double *buff,int datatype);
int convxyz(unsigned char *vol,int dimx,int dimy,int dimz,
	const RowVector &filtx,const RowVector &filty,const RowVector &filtz,
	int fxdim,int fydim,int fzdim,int xoff,int yoff,int zoff,FILE *fp,
	int datatype,double *ovol);
void dan_conv_vol(CrunchCube *,char *,RowVector ,RowVector ,RowVector ,RowVector );
//int dan_hread(char *,IMG_header &);
//int dan_hwrite(char *,IMG_header &);
CrunchCube *dan_map_vol(char *);
int suffix_hdr(char *);
int suffix_img(char *);
int suffix_mat(char *);
int get_datasize(int foo);
void dan_unmap_vol(CrunchCube *);

Matrix dan_bb(char *);
Matrix diff(const Matrix &);
Matrix dan_slice_vol(CrunchCube *map,const Matrix &mat,int m,int n,int hold);
Matrix dan_matrix(const RowVector &P);
void copysegment(Matrix &to,const Matrix &from,int count,int offset);
RowVector dan_linspace(double first,double last,int points);
Matrix dan_reshape44(Matrix m,int col);
void dan_bound(RowVector &vec,double low,double high);
RowVector vectorize(const Matrix &in);
Matrix backslash(const Matrix &one,const Matrix &two);
Matrix dan_box(char *filename,RowVector dim,short type);
int realign_compute(int nfiles,char **filename);
int parsename(char *fname,char *dirname,char *outfilename);
int parse_filelist(char *filename,char ***argv);

void make_lookup(double coord,int nn,int dim,int *d1,double *table,
		 double **ptpend);
int slice_int_sinc(const Matrix &mat,double *image,int xdim1,int ydim1,
		   int *vol,
		   int xdim2,int ydim2,int zdim2,int nn,double background);
int slice_short_sinc(const Matrix &mat,double *image,int xdim1,int ydim1,
		     short *vol,
		     int xdim2,int ydim2,int zdim2,int nn,double background);

template <class T>
int
slice_sinc(const Matrix &mat,double *image,int xdim1,int ydim1,T *vol,
	   int xdim2,int ydim2,int zdim2,int nn,double background);

void print(const Matrix &mat,int size=5);
void print(const char *,const Matrix &mat,int size=5);
void print(const RowVector &vec,int size=5);
void print(const char *,const RowVector &vec,int size=5);
void print(CrunchCube *map);
void print(const char *s,const ColumnVector &vec,int size=5);

void ident(Matrix &mat);

// functions i don't still need to write

double sum(const RowVector &v);

CrunchCube *dan_smooth_image(CrunchCube *,int,int,int);
Matrix dan_get_space_image(CrunchCube *);
Matrix dan_get_space_image(CrunchCube *,const Matrix &);
Matrix dan_bb_image(CrunchCube *);
RowVector mask_from_image(CrunchCube *map,RowVector &,RowVector &);
RowVector dan_slice_image(CrunchCube *,Matrix,int,int,int);
void dan_conv_image(CrunchCube *,CrunchCube *,const RowVector&,const RowVector&,
		    const RowVector&,const RowVector&);
void dan_box_image(CrunchCube *map,RowVector &,RowVector &,RowVector &);

void write_map(CrunchCube *map);
CrunchCube *read_map(short datatype,short datasize,short x,short y,short z,
		 float xsize,float ysize,float zsize);

int realign_stdio();
int realign_rlist(char *rlist);
int realign_args(int argc,char **argv);

void read_short(FILE *fp,short &s);
void read_long(FILE *fp,long &s);
void read_float(FILE *fp,float &s);
void read_char(FILE *fp,char *s,int len);
void write_short(FILE *fp,short &s);
void write_long(FILE *fp,long &s);
void write_float(FILE *fp,float &s);
void write_char(FILE *fp,char *,int len);

CrunchCube *realign_twoimages(CrunchCube *REF,CrunchCube *SREF,CrunchCube *VOL);

void printnonzero(void *,int len);

//===============
// norm protos
//===============

// return value globals
extern Matrix ret_m1,ret_m2,ret_m3;
extern double ret_d1;

void mrqcof(RowVector &T,double alpha[],double beta[],double pss[],int dt2,
       double scale2,int dim2[],unsigned char dat2[],int ni,int dt1[],
       double scale1[],int dim1[],unsigned char *dat1[],int nx,double BX[],
       double dBX[],int ny,double BY[],double dBY[],int nz,double BZ[],
       double dBZ[],double M[],int samp[],int *pnsamp);

int dan_brainwarp(CrunchCube *REF,CrunchCube *IMAGE,const Matrix &Affine,
		  const Matrix &BX,const Matrix &BY,const Matrix &BZ,
		  const Matrix &dBX,const Matrix &dBY,const Matrix &dBZ,
		  const Matrix &T,double fwhm);

int dan_sn3d(CrunchCube *REF,CrunchCube *IMAGE,Matrix &affine,Matrix &dims,
	     Matrix &transform,Matrix &MF);
CrunchCube *
dan_write_sn(CrunchCube *VOL,Matrix &Affine,Matrix &Dims,Matrix &Transform,
	     Matrix &MF,Matrix &bb,RowVector &Vox,int Hold,int affdefault=0);
int dan_affsub1(CrunchCube *REF,CrunchCube *IMAGE,const Matrix &MG,const Matrix &MF,
		int Hold,int samp,const RowVector &P);
RowVector dan_affsub2(CrunchCube *REF,CrunchCube *IMAGE,const Matrix &MG,
		      const Matrix &MF,int Hold,int samp,
		      RowVector P,const RowVector &free,
		      const RowVector &pdesc);
void atranspa(int m,int n,const Matrix &A,const Matrix &C);
Matrix dan_atranspa(const Matrix &A);
Matrix
dan_dctmtx(int N,int K);
Matrix
dan_dctmtx(int N,int K,const RowVector &n);
Matrix
dan_dctmtxdiff(int N,int K);
Matrix
dan_dctmtxdiff(int N,int K,const RowVector &n);
int
dan_snbasis_map(CrunchCube *REF,CrunchCube *IMAGE,Matrix &Affine,
		int k1,int k2,int k3,int iter,int fwhm,double sd1);

Matrix kron(const Matrix &A,const Matrix &B);
RowVector dan_span(double from,double to,double increment);
double get_element(const Matrix &m,int pos);
Matrix reshape(const Matrix &mat,int m,int n);
Matrix reshape(const RowVector &vec,int m,int n);
Matrix reshape(const ColumnVector &vec,int m,int n);
RowVector index(const Matrix &mat,const RowVector &vec);
RowVector index(const RowVector &v1,const RowVector &vec);
Matrix pinv(const Matrix &A);

Matrix dan_sample_vol(CrunchCube *map,const RowVector &x,const RowVector &y,
		      const RowVector &z,int hold);
Matrix dan_sample_vol(CrunchCube *map,const Matrix &x,const Matrix &y,
		      const Matrix &z,int hold);

template <class T>
void
resample_sinc(int m,T *vol,RowVector &out,const RowVector &x,
	      const RowVector &y,const RowVector &z,
	      int dimx,int dimy,int dimz,int nn,double background,
	      double scale);

template <class T>
void
resample_1(int m,T *vol,RowVector &out,const RowVector &x,
	   const RowVector &y,const RowVector &z,int dimx,int dimy,
	   int dimz,double background,double scale);

void
resample_short_sinc(int m,short *vol,RowVector &out,const RowVector &x,
		    const RowVector &y,const RowVector &z,
		    int dimx,int dimy,int dimz,int nn,double background,
		    double scale);
void
resample_int_sinc(int m,int *vol,RowVector &out,const RowVector &x,
		  const RowVector &y,const RowVector &z,
		  int dimx,int dimy,int dimz,int nn,double background,
		  double scale);
void
resample_short_1(int m,short *vol,RowVector &out,const RowVector &x,
		 const RowVector &y,const RowVector &z,int dimx,int dimy,
		 int dimz,double background,double scale);

void
resample_int_1(int m,int *vol,RowVector &out,const RowVector &x,
	       const RowVector &y,const RowVector &z,int dimx,int dimy,
	       int dimz,double background,double scale);

void
resample_char_1(int m,unsigned char *vol,RowVector &out,const RowVector &x,
		const RowVector &y,const RowVector &z,int dimx,int dimy,
		int dimz,double background,double scale);

void
resample_char_sinc(int m,unsigned char *vol,RowVector &out,const RowVector &x,
		   const RowVector &y,const RowVector &z,int dimx,int dimy,
		   int dimz,int nn,double background,double scale);

Matrix ones(int m,int n);
RowVector ones(int d);
Matrix zeros(int m,int n);
RowVector zeros(int d);
double *arrayize(const RowVector &vec);
double *arrayize(const Matrix &mat);
