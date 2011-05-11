
// vbtransform.cpp
// apply an affine transform, resample
// Copyright (c) 2007 by The VoxBo Development Team

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

// note that the guts of the underlying sinc resample code were ported
// from SPM96b source code written by John Ashburner

using namespace std;

#include <stdio.h>
#include <string>
#include "vbutil.h"
#include "vbio.h"

#define PI 3.14159265358979
#define TINY 0.05


void make_lookup(double coord,int nn,int dim,int *d1,double *table,
		 double **ptpend);
template <class T>
void
resample_sinc(int m,T *vol,VB_Vector &out,const VB_Vector &x,
	      const VB_Vector &y,const VB_Vector &z,
	      int dimx,int dimy,int dimz,int nn,double background,
	      double scale);

class VBTransform {
private:
  string imagename;     // name of the image to be normalized
  string refname;       // name of the reference image
  string outname;       // name of the output image

  vector<string> newheaders;  // headers to be added to output file

  enum {vb_3d,vb_4d} mode;
  enum {vb_nn,vb_sinc} method;
  tokenlist args;                // structure to be used for argument parsing
  Cube *mycube,*newcube;
  Tes *mytes,*newtes;
  double zsize;             // size of slices in mm
  double x1,y1,z1;          // start voxel (0 indexed) in x,y,z
  int nx,ny,nz;             // number of voxels in resampled image
  int dimx,dimy,dimz;       // original dimensions
  double voxsize[3];        // original voxsize
  double xstep,ystep,zstep; // resample interval in voxels
public:
  VBTransform(int argc,char **argv);
  ~VBTransform();
  void init();
  int Setup();           // set up parameters
  int VBTransformFile();    // detect filetype
  int VBTransform3D();      // set up for 3d
  int VBTransform4D();      // set up for 4d

  int VBTransformCube();       // dispatch to sinc or nn resample
  int SincVBTransformCube();   // do it for each cube
  int NNVBTransformCube();     // do it for each cube
  void AdjustCornerAndOrigin(VBImage &im);

  // methods for working out the resample parameters
  int UseZ(const string &refname,double zsize);
  int UseDims(const string &refname);
  int UseTLHC(const string &refname);
  int UseCorner(const string &refname);
  int SetAlign(const string mode,const string ref);

};

void resample_help();

int
main(int argc,char *argv[])
{
  tzset();                     // make sure all times are timezone corrected
  if (argc < 2) {              // not enough args, display autodocs
    resample_help();
    exit(0);
  }

  VBTransform *r=new VBTransform(argc,argv);    // init resample object, parse args
  if (!r) {
    printf("*** resample: couldn't allocate a tiny structure\n");
    exit(5);
  }
  int err;
  err=r->Setup();              // set the start, stop, nslices, etc.
  if (!err)
    err = r->VBTransformFile();   // do it
  delete r;                    // clean up
  if (err)
    printf("*** resample: error resampling (%d).\n",err);
  else
    printf("resample: done.\n");
  exit(err);
}

void
VBTransform::init()
{
  imagename=refname="";
  zsize=0.0;
  x1=y1=z1=0;
  xstep=ystep=zstep=1.0;
  nx=41;
  ny=51;
  nz=27;
  method=vb_sinc;
}

VBTransform::VBTransform(int argc,char *argv[])
{
  init();
  args.Transfer(argc-1,argv+1);
}

int
VBTransform::TransformFile()
{
  printf("resample: resampling %s\n",args[0].c_str());
  if (mode == vb_3d)
    return VBTransform3D();
  else if (mode == vb_4d)
    return VBTransform4D();
  else {
    printf("*** resample: error\n");
    return 200;
  }
}

int
VBTransform::Transform3D()
{
  stringstream tmps;

  if (VBTransformCube())
    return 101;
  newcube->SetFileName(args[1]);
  newcube->SetFileFormat(mycube->GetFileFormat());
  newcube->header=mycube->header;

  if (mycube->origin[0]) {
    newcube->origin[0]=lround((mycube->origin[0]-x1)/xstep);
    newcube->origin[1]=lround((mycube->origin[1]-y1)/ystep);
    newcube->origin[2]=lround((mycube->origin[2]-z1)/zstep);
  }

  // add new headers
  for (int i=0; i<(int)newheaders.size(); i++)
    newcube->AddHeader(newheaders[i]);
  newcube->AddHeader((string)"resample_date: "+timedate());

  AdjustCornerAndOrigin(*newcube);
  if (newcube->WriteFile())
    printf("*** resample: error writing %s\n",args[1].c_str());
  else
    printf("resample: wrote %s.\n",args[1].c_str());
  delete newcube;
  return 0;
}

int
VBTransform::Transform4D()
{
  newtes=new Tes();
  newtes->SetVolume(nx,ny,nz,mytes->dimt,mytes->datatype);
  newtes->voxsize[0]=fabs(xstep*mytes->voxsize[0]);
  newtes->voxsize[1]=fabs(ystep*mytes->voxsize[1]);
  newtes->voxsize[2]=fabs(zstep*mytes->voxsize[2]);

  if (mytes->origin[0]) {
    newtes->origin[0]=lround((mytes->origin[0]-x1)/xstep);
    newtes->origin[1]=lround((mytes->origin[1]-y1)/ystep);
    newtes->origin[2]=lround((mytes->origin[2]-z1)/zstep);
  }

  for (int i=0; i<mytes->dimt; i++) {
    mycube=new Cube((*mytes)[i]);
    if (VBTransformCube())
      return 102;
    newtes->SetCube(i,newcube);
    delete newcube;
    delete mycube;
  }

  // add new headers
  for (int i=0; i<(int)newheaders.size(); i++)
    newtes->AddHeader(newheaders[i]);
  newtes->AddHeader((string)"resample_date: "+timedate());

  newtes->SetFileName(args[1]);
  newtes->SetFileFormat(mytes->GetFileFormat());
  newtes->header=mytes->header;
  AdjustCornerAndOrigin(*newtes);
  if (int err=newtes->WriteFile())
    printf("*** resample: error %d writing %s\n",err,args[1].c_str());
  else
    printf("resample: wrote %s.\n",args[1].c_str());
  return 0;
}

int
VBTransform::ResampleCube()
{
  if (method==vb_nn)
    return NNVBTransformCube();
  else
    return SincVBTransformCube();
}

int
VBTransform::NNResampleCube()
{
  int i,j,k;
  if (!mycube)
    return 5;
  newcube=new Cube();
  newcube->SetVolume(nx,ny,nz,mycube->datatype);
  VBMatrix coord(4,1);
  for (i=0; i<nx; i++) {
    for (j=0; j<ny; j++) {
      for (k=0; k<nz; k++) {
        coord.set(0,0,i);
        coord.set(1,0,j);
        coord.set(2,0,k);
        coord.set(3,0,0.0);
        itransform*=coord;
        int c1=lround(itransform(0,0));
        int c2=lround(itransform(1,0));
        int c3=lround(itransform(2,0));
        newcube->SetValue(i,j,k,mycube->GetValue(c1,c2,c3));
      }
    }
  }

  return 0;   // no error
}

int
VBTransform::SincVBTransformCube()
{
  int i,j,k;

  if (!mycube)
    return 5;

  newcube=new Cube();
  newcube->SetVolume(nx,ny,nz,mycube->datatype);
  newcube->voxsize[0]=fabs(xstep*mycube->voxsize[0]);
  newcube->voxsize[1]=fabs(ystep*mycube->voxsize[1]);
  newcube->voxsize[2]=fabs(zstep*mycube->voxsize[2]);

  VB_Vector c1(1),c2(1),c3(1),out(1);

  for (k=0; k<nz; k++) {
    for (i=0; i<nx; i++) {
      for (j=0; j<ny; j++) {
        c1(0)=x1+(xstep * i)+1;   // +1 because the algorithm 1-indexes
        c2(0)=y1+(ystep * j)+1;
        c3(0)=z1+(zstep * k)+1;
        switch (mycube->datatype) {
        case vb_byte:
          resample_sinc(1,(unsigned char *)mycube->data,out,c1,c2,c3,
                        mycube->dimx,mycube->dimy,mycube->dimz,5,
                        0.0,1.0);
          break;
        case vb_short:
          resample_sinc(1,(int16 *)mycube->data,out,c1,c2,c3,
                        mycube->dimx,mycube->dimy,mycube->dimz,5,
                        0.0,1.0);
          break;
        case vb_long:
          resample_sinc(1,(int32 *)mycube->data,out,c1,c2,c3,
                        mycube->dimx,mycube->dimy,mycube->dimz,5,
                        0.0,1.0);
          break;
        case vb_float:
          resample_sinc(1,(float *)mycube->data,out,c1,c2,c3,
                        mycube->dimx,mycube->dimy,mycube->dimz,5,
                        0.0,1.0);
          break;
        case vb_double:
          resample_sinc(1,(double *)mycube->data,out,c1,c2,c3,
                        mycube->dimx,mycube->dimy,mycube->dimz,5,
                        0.0,1.0);
          break;
        }
        newcube->SetValue(i,j,k,out(0));
      }
    }
  }

  return 0;   // no error
}

void
resample_help()
{
  printf("\nVoxBo resample (v%s)\n",vbversion.c_str());
  printf("usage: resample <in> <out> [flags]\n");
  printf("flags:\n");
  printf("   -rr <ref>      ref=image in desired space\n");
  printf("   -rc <ref>      ref=image with desired field of view\n");
  printf("   -rz <ref> [z]  same but specify size in z\n");
  printf("   -rd <ref>      ref=image of desired dimensions\n");
  printf("   -ra <ref>      aligns to ref on all dims, using corner position\n");
  printf("   -d  <x y z>    desired dimensions\n");
  printf("   -nn            use nearest neighbor interpolation (default is sinc)\n");
  printf("   -xx <a b c>    a=start voxel (from 0); b=interval in voxels; c=# of voxels\n");
  printf("   -yy <a b c>    same for y dimension\n");
  printf("   -zz <a b c>    same for z dimension\n");
  printf("\n");
  printf("   -r     generic ref cube\n");
  printf("   -aa, -ax, -ay, -az  [origin/corner]\n");
  printf("   -sa, -sx, -sy, -sz  [n]\n");
  printf("\n");
  printf("Some examples for an axial image of dimensions 64x64x21:\n");
  printf("  invert the order of the slices: resample in.cub out.cub -zz 20 -1 21\n");
  printf("  swap left-right: resample in.cub out.cub -xx 63 -1 64\n");
  printf("  interpolate in-plane: resample in.cub out.cub -xx 0 .25 256 -yy 0 .25 256\n");
  printf("  crop to 40x64x21: resample in.cub out.cub -xx 12 1 40\n");
  printf("\n");
}









template <class T>
void
resample_sinc(int m,T *vol,VB_Vector &out,const VB_Vector &x,
              const VB_Vector &y,const VB_Vector &z,
              int dimx,int dimy,int dimz,int nn,double background,
              double scale)
{
  int i, dim1xdim2 = dimx*dimy;
  int dx1, dy1, dz1;
  static double tablex[255],tabley[255],tablez[255];
  
  vol -= (1 + dimx*(1 + dimy));
  for (i=0; i<m; i++) {
    if (z(i)>=1-TINY && z(i)<=dimz+TINY &&
        y(i)>=1-TINY && y(i)<=dimy+TINY &&
        x(i)>=1-TINY && x(i)<=dimx+TINY) {
      T *dp1;
      double dat=0.0, *tp1, *tp1end, *tp2end, *tp3end;
      
      make_lookup(x(i), nn, dimx, &dx1, tablex, &tp3end);
      make_lookup(y(i), nn, dimy, &dy1, tabley, &tp2end);
      make_lookup(z(i), nn, dimz, &dz1, tablez, &tp1end);
      
      tp1 = tablez;
      dy1 *= dimx;
      dp1 = vol + dim1xdim2*dz1;
      
      while(tp1 <= tp1end) {
        T *dp2 = dp1 + dy1;
        double dat2 = 0.0,
          *tp2 = tabley;
        while (tp2 <= tp2end) {
          register double dat3 = 0.0, *tp3 = tablex;
          register T *dp3 = dp2 + dx1;
          while(tp3 <= tp3end)
            dat3 += *(dp3++) * *(tp3++);
          dat2 += dat3 * *(tp2++);
          dp2 += dimx;
        }
        dat += dat2 * *(tp1++);
        dp1 += dim1xdim2;
      }
      out(i) = scale*dat;
    }
    else out(i) = background;
  }
}

// Generate a sinc lookup table with a Hanning filter envelope
void
make_lookup(double coord,int nn,int dim,int *d1,double *table,double **ptpend)
{
  register int d2, d, fcoord;
  register double *tp, *tpend, dtmp;

  if (fabs(coord-rint(coord))<0.00001)
    {
      /* Close enough to use nearest neighbour */
      *d1=(int)rint(coord);
      if (*d1<1 || *d1>dim) /* Pixel location outside image */
	*ptpend = table-1;
      else
	{
	  table[0]=1.0;
	  *ptpend = table;
	}
    }
  else
    {
      fcoord = (int)floor(coord);
      *d1 = fcoord-nn;
      if (*d1<1) *d1=1;
      d2 = fcoord+nn;
      if (d2>dim) d2 = dim;

      *ptpend = tpend = table+(d2 - *d1);
      d = *d1, tp = table;
      while (tp <= tpend)
	{
	  dtmp = PI*(coord-(double)(d++));
	  *(tp++) = sin(dtmp)/dtmp* 0.5*(1.0 + cos(dtmp/nn)) ;
	}
    }
}
