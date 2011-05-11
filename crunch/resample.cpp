
// resample.cpp
// sinc resampling of volumes, adapted from spm
// Copyright (c) 1998-2004 by The VoxBo Development Team

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
// from SPM96b source code written for MATLAB by John Ashburner

using namespace std;

#include "vbcrunch.h"

class Resample {
private:
  string imagename;     // name of the image to be normalized
  string refname;       // name of the reference image
  string outname;       // name of the output image

  vector<string> newheaders;  // headers to be added to output file

  enum {vb_3d,vb_4d} mode;
  enum {vb_nn,vb_sinc} method;
  tokenlist args;                // structure to be used for argument parsing
  CrunchCube *mycube,*newcube;
  Tes *mytes,*newtes;
  double zsize;             // size of slices in mm
  double x1,y1,z1;          // start voxel (0 indexed) in x,y,z
  int nx,ny,nz;             // number of voxels in resampled image
  int dimx,dimy,dimz;       // original dimensions
  double voxsize[3];        // original voxsize
  double xstep,ystep,zstep; // resample interval in voxels
public:
  Resample(int argc,char **argv);
  ~Resample();
  void init();
  int Setup();           // set up parameters
  int ResampleFile();    // detect filetype
  int Resample3D();      // set up for 3d
  int Resample4D();      // set up for 4d

  int ResampleCube();       // dispatch to sinc or nn resample
  int SincResampleCube();   // do it for each cube
  int NNResampleCube();     // do it for each cube
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

  Resample *r=new Resample(argc,argv);    // init resample object, parse args
  if (!r) {
    printf("*** resample: couldn't allocate a tiny structure\n");
    exit(5);
  }
  int err;
  err=r->Setup();              // set the start, stop, nslices, etc.
  if (!err)
    err = r->ResampleFile();   // do it
  delete r;                    // clean up
  if (err)
    printf("*** resample: error resampling (%d).\n",err);
  else
    printf("resample: done.\n");
  exit(err);
}

void
Resample::init()
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

Resample::Resample(int argc,char *argv[])
{
  init();
  args.Transfer(argc-1,argv+1);
}

int
Resample::Setup()
{
  if (args.size() < 2) {
    printf("*** resample: requires at least an input and an output filename\n");
    return 110;
  }
  switch(EligibleFileTypes(args[0])[0].getDimensions()) {
  case 4:
    mytes = new Tes;
    mytes->ReadFile(args[0]);
    if (!mytes->data_valid) {
      printf("*** resample: invalid 4D file: %s\n",args[0].c_str());
      delete mytes;
      mytes=NULL;
      return 100;
    }
    dimx=mytes->dimx;
    dimy=mytes->dimy;
    dimz=mytes->dimz;
    voxsize[0]=mytes->voxsize[0];
    voxsize[1]=mytes->voxsize[1];
    voxsize[2]=mytes->voxsize[2];
    mode=vb_4d;
    break;
  case 3:
    mycube = new CrunchCube;
    mycube->ReadFile(args[0]);
    if (!mycube->data_valid) {
      printf("*** resample: invalid 3D file: %s\n",args[0].c_str());
      delete mycube;
      mycube=NULL;
      return 101;
    }
    dimx=mycube->dimx;
    dimy=mycube->dimy;
    dimz=mycube->dimz;
    voxsize[0]=mycube->voxsize[0];
    voxsize[1]=mycube->voxsize[1];
    voxsize[2]=mycube->voxsize[2];
    mode=vb_3d;
    break;
  default:
    printf("*** resample: can only resample 3D and 4D data\n");
    return 102;
    break;
  }

  x1=y1=z1=0;
  xstep=ystep=zstep=1.0;
  nx=dimx;
  ny=dimy;
  nz=dimz;
  
  for (int i=0; i<args.size(); i++) {
    if (args[i]=="-rr") {
      if (i+1 < args.size()) {
	UseZ(args[i+1],0);
	i++;
      }
    }
    else if (args[i]=="-ra") {
      if (i+1 < args.size()) {
	UseCorner(args[i+1]);
	i++;
      }
    }
    else if (args[i]=="-rz") {
      if (i+2 < args.size()) {
	zsize=strtod(args[i+2].c_str(),NULL);
	UseZ(args[i+1],zsize);
	i++;
      }
    }
    else if (args[i]=="-rd") {
      if (i+1 < args.size()) {
        UseDims(args[i+1]);
        i++;
      }
    }
    else if (args[i]=="-rc") {
      if (i+1 < args.size()) {
        UseTLHC(args[i+1]);
        i++;
      }
    }
    else if (args[i]=="-r") {
      if (i+1 < args.size()) {
        refname=args[i+1];
      }
    }
    else if (args[i]=="-aa" && i+1<args.size()) {
      SetAlign(args[i],args[i+1]);
    }
    else if (args[i]=="-d") {
      if (i+3 < args.size()) {
	nx=strtol(args[i+1]);
	ny=strtol(args[i+2]);
	nz=strtol(args[i+3]);
	// FIXME is the following correct?
	xstep=(double)(dimx)/nx;
	ystep=(double)(dimy)/ny;
	zstep=(double)(dimz)/nz;
	i+=3;
      }
    }
    else if (args[i]=="-nn") {
      method=vb_nn;
    }
    else if (args[i]=="-xx") {
      if (i+3 < args.size()) {
	x1=strtod(args[i+1]);
	xstep=strtod(args[i+2]);
	nx=strtol(args[i+3]);
	i+=3;
      }
    }
    else if (args[i]=="-yy") {
      if (i+3 < args.size()) {
	y1=strtod(args[i+1]);
	ystep=strtod(args[i+2]);
	ny=strtol(args[i+3]);
	i+=3;
      }
    }
    else if (args[i]=="-zz") {
      if (i+3 < args.size()) {
	z1=strtod(args[i+1]);
	zstep=strtod(args[i+2]);
	nz=strtol(args[i+3]);
	i+=3;
      }
    }
    // new flags: padleft, padright, padtop, padbottom, padback, padfront
    //            croplr
    //            flipx, flipy, flipz
    else if (args[i]=="--flipx") {
      x1=mycube->dimx-1;
      xstep=-1;
      nx=mycube->dimx;
    }
    else if (args[i]=="--flipy") {
      y1=mycube->dimy-1;
      ystep=-1;
      ny=mycube->dimy;
    }
    else if (args[i]=="--flipz") {
      z1=mycube->dimz-1;
      zstep=-1;
      nz=mycube->dimz;
    }
  }

  char tmps[STRINGLEN];
  sprintf(tmps,"resample_x: start %.6f step %.2f count %d",x1,xstep,nx);
  newheaders.push_back(tmps);
  printf("[I] resample: %s\n",tmps);

  sprintf(tmps,"resample_y: start %.6f step %.2f count %d",y1,ystep,ny);
  newheaders.push_back(tmps);
  printf("[I] resample: %s\n",tmps);

  sprintf(tmps,"resample_z: start %.6f step %.2f count %d",z1,zstep,nz);
  newheaders.push_back(tmps);
  printf("[I] resample: %s\n",tmps);

  return 0;
}

Resample::~Resample()
{
}

int
Resample::SetAlign(const string mode,const string ref)
{
  Cube refcube;
  if (refcube.ReadFile(refname))
    return 100;
  if (mode=="-aa" || mode=="-ax") {
    if (ref=="origin")
      x1=(float)mycube->origin[0]-(((float)refcube.origin[0]*refcube.voxsize[0])/mycube->voxsize[0]);
  }
  if (mode=="-aa" || mode=="-ay") {
    if (ref=="origin")
      y1=(float)mycube->origin[1]-(((float)refcube.origin[1]*refcube.voxsize[1])/mycube->voxsize[1]);
  }
  if (mode=="-aa" || mode=="-az") {
    if (ref=="origin")
      z1=(float)mycube->origin[2]-(((float)refcube.origin[2]*refcube.voxsize[2])/mycube->voxsize[2]);
  }
  return 0;
}

int
Resample::UseCorner(const string &refname)
{
  Cube refcube;
  stringstream tmps;
  if (refcube.ReadFile(refname)) {
    Tes reftes;
    if (!(reftes.ReadFile(refname))) {
      refcube=reftes[0];
    }
    else {
      printf("*** resample: invalid reference file: %s\n",refname.c_str());
      return 501;       // panic and exit
    }
  }
  // first get the corner coordinates of both images
  tokenlist ourline,refline;
  ourline.ParseLine(mycube->GetHeader("AbsoluteCornerPosition:"));
  refline.ParseLine(refcube.GetHeader("AbsoluteCornerPosition:"));
  if (ourline.size()!=3) {
    tmps.str("");
    tmps << "resample: input image file doesn't have an absolute corner position tag";
    printErrorMsg(VB_ERROR,tmps.str());
    return 101;
  }
  if (refline.size()!=3) {
    tmps.str("");
    tmps << "resample: reference image file doesn't have an absolute corner position tag";
    printErrorMsg(VB_ERROR,tmps.str());
    return 101;
  }
  double ourpos[3],refpos[3];
  ourpos[0]=strtod(ourline[0]);
  ourpos[1]=strtod(ourline[1]);
  ourpos[2]=strtod(ourline[2]);
  refpos[0]=strtod(refline[0]);
  refpos[1]=strtod(refline[1]);
  refpos[2]=strtod(refline[2]);
  
  // find the start point
  x1=(refpos[0]-ourpos[0])/mycube->voxsize[0];
  y1=(refpos[1]-ourpos[1])/mycube->voxsize[1];
  z1=(refpos[2]-ourpos[2])/mycube->voxsize[2];
  // the sampling interval should provide for 4x resolution in-plane
  xstep=(refcube.voxsize[0]/4.0)/mycube->voxsize[0];
  ystep=(refcube.voxsize[1]/4.0)/mycube->voxsize[1];
  zstep=refcube.voxsize[2]/mycube->voxsize[2];
  // the number of voxels should be the width of the target image
  nx=refcube.dimx*4;
  ny=refcube.dimy*4;
  nz=refcube.dimz;
  return 0;
}

int
Resample::UseZ(const string &refname,double zsize)
{
  CrunchCube *refcube;

  refcube = new CrunchCube;
  refcube->ReadFile(refname);
  if (!refcube->data_valid) {
    printf("*** resample: invalid 3D file: %s\n",refname.c_str());
    delete refcube;
    return 501;       // panic and exit
  }
  double ourstart,ourend,refstart,refend;
  // old style: startloc/endloc
  ourstart=strtod(mycube->GetHeader("StartLoc:"));
  ourend=strtod(mycube->GetHeader("EndLoc:"));
  refstart=strtod(refcube->GetHeader("StartLoc:"));
  refend=strtod(refcube->GetHeader("EndLoc:"));
  // new style: zrange
  string refzrange=refcube->GetHeader("ZRange:");
  string ourzrange=mycube->GetHeader("ZRange:");
  if (refzrange.size()) {
    tokenlist range(refzrange);
    refstart=strtod(range[0]);
    refend=strtod(range[1]);
  }
  if (ourzrange.size()) {
    tokenlist range(ourzrange);
    ourstart=strtod(range[0]);
    ourend=strtod(range[1]);
  }

  if (zsize < .001)
    zsize=refcube->voxsize[2];
  nx=dimx;
  ny=dimy;
  z1=(refstart-ourstart)/voxsize[2];
  nz=(int)((abs(refend-refstart)/zsize)+0.5)+1;
  zstep=zsize/voxsize[2];
  delete refcube;
  return 0;
}

int
Resample::UseDims(const string &refname)
{
  CrunchCube *refcube;
  refcube = new CrunchCube;
  refcube->ReadFile(refname);
  if (!refcube->data_valid) {
    printf("*** resample: invalid 3D file: %s\n",args[1].c_str());
    delete refcube;
    return 501;       // panic and exit
  }
  nx=refcube->dimx;
  ny=refcube->dimy;
  nz=refcube->dimz;
  // FIXME is the following correct?
  xstep=(double)(dimx)/nx;
  ystep=(double)(dimy)/ny;
  zstep=(double)(dimz)/nz;
  delete refcube;
  return 0;
}

int
Resample::UseTLHC(const string &refname)
{
  CrunchCube *refcube;

  refcube = new CrunchCube;
  refcube->ReadFile(refname);
  if (!refcube->data_valid) {
    printf("*** resample: invalid 3D file: %s\n",refname.c_str());
    delete refcube;
    return 501;       // panic and exit
  }
  double ourLR,refLR,ourAP,refAP;
  ourLR=refLR=ourAP=refAP=0.0;

  // new style: zrange
  string reftlhc=refcube->GetHeader("im_tlhc:");
  string ourtlhc=mycube->GetHeader("im_tlhc:");

  if (reftlhc.size()) {
    tokenlist range(reftlhc);
    refLR=strtod(range[0]);
    refAP=strtod(range[1]);
  }
  if (ourtlhc.size()) {
    tokenlist range(ourtlhc);
    ourLR=strtod(range[0]);
    ourAP=strtod(range[1]);
  }

  nx=dimx;
  ny=dimy;
  nz=dimz;
  x1=y1=z1=0;
  xstep=ystep=zstep=1.0;

  if (abs(ourLR-refLR)>0.001) {
    x1=(ourLR-refLR)/mycube->voxsize[0];
  }
  if (abs(ourAP-refAP)>0.001) {
    y1=(refAP-ourAP)/mycube->voxsize[1];
  }
  if (x1==0 && y1==0) {
    printf("resample: no fov adjustment neeeded\n");
  }

  delete refcube;
  return 0;
}

int
Resample::ResampleFile()
{
  printf("resample: resampling %s\n",args[0].c_str());
  if (mode == vb_3d)
    return Resample3D();
  else if (mode == vb_4d)
    return Resample4D();
  else {
    printf("*** resample: error\n");
    return 200;
  }
}

int
Resample::Resample3D()
{
  stringstream tmps;

  if (ResampleCube())
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
  for (size_t i=0; i<newheaders.size(); i++)
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
Resample::Resample4D()
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
    mycube=new CrunchCube((*mytes)[i]);
    if (ResampleCube())
      return 102;
    newtes->SetCube(i,newcube);
    delete newcube;
    delete mycube;
  }

  // add new headers
  for (size_t i=0; i<newheaders.size(); i++)
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

void
Resample::AdjustCornerAndOrigin(VBImage &im)
{
  vector<string> newheader;
  tokenlist args;
  for (size_t i=0; i<im.header.size(); i++) {
    args.ParseLine(im.header[i]);
    if (args[0]!="AbsoluteCornerPosition:")
      newheader.push_back(im.header[i]);
  }
  double x,y,z;
  im.GetCorner(x,y,z);
  x+=x1*im.voxsize[0];
  y+=y1*im.voxsize[1];
  z+=z1*im.voxsize[2];
  stringstream tmps;
  tmps << "AbsoluteCornerPosition: " << x << " " << y << " " << z;
  newheader.push_back(tmps.str());
  im.header=newheader;
  // the origin is easier...
//   im.origin[0]-=lround(x1);
//   im.origin[1]-=lround(y1);
//   im.origin[2]-=lround(z1);
}

int
Resample::ResampleCube()
{
  if (method==vb_nn)
    return NNResampleCube();
  else
    return SincResampleCube();
}

int
Resample::NNResampleCube()
{
  int i,j,k;

  if (!mycube)
    return 5;

  newcube=new CrunchCube();
  newcube->SetVolume(nx,ny,nz,mycube->datatype);
  newcube->voxsize[0]=fabs(xstep*mycube->voxsize[0]);
  newcube->voxsize[1]=fabs(ystep*mycube->voxsize[1]);
  newcube->voxsize[2]=fabs(zstep*mycube->voxsize[2]);

  for (k=0; k<nz; k++) {
    for (i=0; i<nx; i++) {
      for (j=0; j<ny; j++) {
	int c1=lround(x1+(xstep * i));
	int c2=lround(y1+(ystep * j));
	int c3=lround(z1+(zstep * k));
	newcube->SetValue(i,j,k,mycube->GetValue(c1,c2,c3));
      }
    }
  }

  return 0;   // no error
}

int
Resample::SincResampleCube()
{
  int i,j,k;

  if (!mycube)
    return 5;

  newcube=new CrunchCube();
  newcube->SetVolume(nx,ny,nz,mycube->datatype);
  newcube->voxsize[0]=fabs(xstep*mycube->voxsize[0]);
  newcube->voxsize[1]=fabs(ystep*mycube->voxsize[1]);
  newcube->voxsize[2]=fabs(zstep*mycube->voxsize[2]);

  RowVector c1(1),c2(1),c3(1),out(1);

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
	  resample_sinc(1,(short *)mycube->data,out,c1,c2,c3,
			mycube->dimx,mycube->dimy,mycube->dimz,5,
			0.0,1.0);
	  break;
	case vb_long:
	  resample_sinc(1,(int *)mycube->data,out,c1,c2,c3,
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
