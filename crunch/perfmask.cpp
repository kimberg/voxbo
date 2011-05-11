
// perfmask.cpp
// create mask from a segmentation, for perfusion purposes
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

#include "vbcrunch.h"

class Voxel {
public:
  int x,y,z;
};

class Region {
public:
  vector<Voxel> vox;
};

class Perfmask {
private:
  tokenlist args;                // structure to be used for argument parsing
  CrunchCube *mycube,*newcube,*incube;
  double zsize;             // size of slices in mm
  double x1,y1,z1;          // start voxel (0 indexed) in x,y,z
  int nx,ny,nz;             // number of voxels in resampled image
  int dimx,dimy,dimz;       // original dimensions
  float keepvalue;          // the mask value we want to keep
  double voxsize[3];        // original voxsize
  double xstep,ystep,zstep; // resample interval in voxels
public:
  Perfmask(int argc,char **argv);
  int Go();              // do it
  int ResampleCube();    // do it for each cube
  int UseDims(const string &refname);
  void SelectRegions(CrunchCube *rcube);
};

void growregion(Region &reg,Cube &cb,int x,int y,int z);
int regionsize(const Region &r1,const Region &r2);
void perfmask_help();

int
main(int argc,char *argv[])
{
  tzset();                     // make sure all times are timezone corrected
  if (argc < 2) {              // not enough args, display autodocs
    perfmask_help();
    exit(0);
  }

  Perfmask *r=new Perfmask(argc,argv);    // init perfmask object, parse args
  if (!r) {
    printf("*** perfmask: couldn't allocate a tiny structure\n");
    exit(5);
  }
  int err;
  err=r->Go();
  delete r;                    // clean up
  if (err)
    printf("*** perfmask: error resampling (%d).\n",err);
  else
    printf("perfmask: done.\n");
  exit(err);
}

Perfmask::Perfmask(int argc,char *argv[])
{
  args.Transfer(argc-1,argv+1);
  zsize=0.0;
  x1=y1=z1=0;
  xstep=ystep=zstep=1.0;
  nx=41;
  ny=51;
  nz=27;
  keepvalue=1.0;
}

int
Perfmask::Go()
{
  if (args.size() < 3) {
    printf("*** perfmask: requires segmentation, reference epi, and output filename\n");
    return 110;
  }

  switch(ImageType(FileFormat(args[0]))) {
  case vb_3d:
    mycube = new CrunchCube;
    mycube->ReadFile(args[0]);
    if (!mycube->data_valid) {
      printf("*** perfmask: invalid 3D file: %s\n",args[0].c_str());
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
    break;
  default:
    printf("*** perfmask: unrecognized filetype\n");
    return 103;
    break;
  }

  x1=y1=z1=0;
  xstep=ystep=zstep=1.0;
  nx=dimx;
  ny=dimy;
  nz=dimz;

  UseDims(args[1]);
  
  for (int i=0; i<args.size(); i++) {
    // 
  }

  // printf("perfmask: x start %.2f step %.2f count %d\n",x1,xstep,nx);
  // printf("perfmask: y start %.2f step %.2f count %d\n",y1,ystep,ny);
  // printf("perfmask: z start %.2f step %.2f count %d\n",z1,zstep,nz);


  // mask out the non-1's
  for (int i=0; i<mycube->dimx; i++) {
    for (int j=0; j<mycube->dimy; j++) {
      for (int k=0; k<mycube->dimz; k++) {
	if (abs(mycube->GetValue(i,j,k)-keepvalue) > 0.2) {
	  mycube->SetValue(i,j,k,0.0);
	}
      }
    }
  }

  incube=new CrunchCube();
  incube->SetVolume(mycube->dimx,mycube->dimy,mycube->dimz,vb_float);
  incube->voxsize[0]=mycube->voxsize[0];
  incube->voxsize[1]=mycube->voxsize[1];
  incube->voxsize[2]=mycube->voxsize[2];
  // copy data, now in floats
  for (int i=0; i<mycube->dimx; i++) {
    for (int j=0; j<mycube->dimy; j++) {
      for (int k=0; k<mycube->dimz; k++) {
	incube->SetValue(i,j,k,mycube->GetValue(i,j,k));
      }
    }
  }

  SelectRegions(incube);

  if (ResampleCube())
    return 101;

  // mask out the non-1's
  for (int i=0; i<newcube->dimx; i++) {
    for (int j=0; j<newcube->dimy; j++) {
      for (int k=0; k<newcube->dimz; k++) {
	if (abs(newcube->GetValue(i,j,k)-keepvalue) < 0.2)
	  newcube->SetValue(i,j,k,keepvalue);
	else
	  newcube->SetValue(i,j,k,0.0);
      }
    }
  }

  newcube->SetFileName(args[2]);
  newcube->header=mycube->header;

  if (mycube->origin[0]) {
    newcube->origin[0]=(int)round((mycube->origin[0]-x1)/xstep);
    newcube->origin[1]=(int)round((mycube->origin[1]-y1)/ystep);
    newcube->origin[2]=(int)round((mycube->origin[2]-z1)/zstep);
  }

  if (newcube->WriteFile())
    printf("*** perfmask: error writing %s\n",args[2].c_str());
  else
    printf("perfmask: wrote %s.\n",args[2].c_str());
  delete newcube;
  return 0;
}



void
Perfmask::SelectRegions(CrunchCube *rcube)
{
  vector<Region> regionlist;
  int i,j,k;
  double val;

  // find all the regions
  for (i=0; i<rcube->dimx; i++) {
    for (j=0; j<rcube->dimy; j++) {
      for (k=0; k<rcube->dimz; k++) {
	val=rcube->GetValue(i,j,k);
	if (val != 0.0) {
	  Region rg;
	  growregion(rg,*rcube,i,j,k);  // clears the voxels as a side effect!
	  regionlist.push_back(rg);
	}
      }
    }
  }
  // sort by size
  sort(regionlist.begin(),regionlist.end(),regionsize);
  
  // mark up the cube for just those regions larger than 5 voxels
  cout << regionlist.size() << endl;
  for (i=0; i<(int)regionlist.size(); i++) {
    k=regionlist[i].vox.size();
    cout << k << endl;
    if (k < 200)
      continue;
    for (j=0; j<(int)regionlist[i].vox.size(); j++) {
      rcube->SetValue(regionlist[i].vox[j].x,regionlist[i].vox[j].y,regionlist[i].vox[j].z,1.0);
    }
  }
}

int
regionsize(const Region &r1,const Region &r2)
{
  return (r1.vox.size() > r2.vox.size());
}

void
growregion(Region &reg,Cube &cb,int x,int y,int z)
{
  Voxel vx;
  vx.x=x;
  vx.y=y;
  vx.z=z;
  
  reg.vox.push_back(vx);
  cb.SetValue(x,y,z,0.0);
  int i,j,k;
  for (i=x-1; i<x+2; i++) {
    if (i<0 || i>cb.dimx-1) continue;
    for (j=y-1; j<y+2; j++) {
      if (j<0 || j>cb.dimy-1) continue;
      for (k=z-1; k<z+2; k++) {
	if (k<0 || k>cb.dimz-1) continue;
	if (i==x && j==y && k==z) continue;
	if (cb.GetValue(i,j,k))
	  growregion(reg,cb,i,j,k);
      }
    }
  }
}


int
Perfmask::UseDims(const string &refname)
{
  CrunchCube *refcube;
  refcube = new CrunchCube;
  refcube->ReadFile(refname);
  if (!refcube->data_valid) {
    printf("*** perfmask: invalid 3D file: %s\n",args[1].c_str());
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
Perfmask::ResampleCube()
{
  int i,j,k;

  if (!incube)
    return 5;

  newcube=new CrunchCube();
  newcube->SetVolume(nx,ny,nz,incube->datatype);
  newcube->voxsize[0]=xstep*incube->voxsize[0];
  newcube->voxsize[1]=ystep*incube->voxsize[1];
  newcube->voxsize[2]=zstep*incube->voxsize[2];

  RowVector c1(1),c2(1),c3(1),out(1);

  for (k=0; k<nz; k++) {
    for (i=0; i<nx; i++) {
      for (j=0; j<ny; j++) {
	c1(0)=x1+(xstep * i)+1;   // +1 because the algorithm 1-indexes
	c2(0)=y1+(ystep * j)+1;
	c3(0)=z1+(zstep * k)+1;
	switch (incube->datatype) {
	case vb_byte:
	  resample_sinc(1,(unsigned char *)incube->data,out,c1,c2,c3,
			incube->dimx,incube->dimy,incube->dimz,5,
			0.0,1.0);
	  break;
	case vb_short:
	  resample_sinc(1,(short *)incube->data,out,c1,c2,c3,
			incube->dimx,incube->dimy,incube->dimz,5,
			0.0,1.0);
	  break;
	case vb_long:
	  resample_sinc(1,(int *)incube->data,out,c1,c2,c3,
			incube->dimx,incube->dimy,incube->dimz,5,
			0.0,1.0);
	  break;
	case vb_float:
	  resample_sinc(1,(float *)incube->data,out,c1,c2,c3,
			incube->dimx,incube->dimy,incube->dimz,5,
			0.0,1.0);
	  break;
	case vb_double:
	  resample_sinc(1,(double *)incube->data,out,c1,c2,c3,
			incube->dimx,incube->dimy,incube->dimz,5,
			0.0,1.0);
	  break;
	}
	newcube->SetValue(i,j,k,out(0));
      }
    }
  }

  return 0;   // no error
}

void growregion(Region &reg,Cube &cb,int x,int y,int z);
void
perfmask_help()
{
  printf("\nVoxBo perfmask (v%s)\n",vbversion.c_str());
  printf("summary: create a mask from a segmentation in the space of an epi image\n");
  printf("usage: perfmask <segimg> <epi> <out> [flags]\n");
  printf("flags:\n");
  printf("\n");
}
