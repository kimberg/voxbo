
// norm_main.cpp
// Dan Kimberg, 1998-2002
// based on SPM code in MATLAB, written mainly by John Ashburner

// neurological convention
// allow customized
// 12 affine params
// 4x5x4 basis functions
// 8 iterations
// smoothing factor 8
// smoothness of deformation field constant 0.02
// Hold = interpolation method = 5 (for sinc)
// aff_parms is probably 000000111000
// free is 111111111111 (12 ones, 12 free parameters)

using namespace std;

#include "vbcrunch.h"

int
dan_sn3d(CrunchCube *REF,CrunchCube *IMAGE,Matrix &affine,Matrix &dims,
	 Matrix &transform,Matrix &MF)
{
  CrunchCube *sIMAGE,*sREF;
  Matrix MG;
  RowVector pdesc(13,1.0),fr(13,1.0),mean0,p1,prms;
  RowVector affp(12,0.0);
  int np;
  double scales;
  
  affp(7) = affp(8) = 1.0;
  affp(6) = 1.0;

  // smoothing is in
  sIMAGE = dan_smooth_image(IMAGE,8,8,8);
  sREF = dan_smooth_image(REF,8,8,8);
  MF = dan_get_space_image(IMAGE);
  MG = dan_get_space_image(sREF);
  
  // Affine Normalisation
  np = 1;
  mean0 = affp;
  mean0.resize(mean0.length()+1,1.0);
  
  p1 = dan_affsub2(sREF,sIMAGE,MG,MF,1,8,mean0,fr,pdesc);
  p1 = dan_affsub2(sREF,sIMAGE,MG,MF,1,4,p1,fr,pdesc);

  prms = p1;
  prms.resize(12);
  affine = ((MG.inverse() * dan_matrix(prms)) *MF).inverse();

  dan_snbasis_map(sREF,sIMAGE,affine,4,5,4,8,8,0.02);
  transform = ret_m1; dims = ret_m2; scales = ret_d1;

  dims.resize(6,3,0.0);
  dims(2,0) = sREF->voxsize[0];
  dims(2,1) = sREF->voxsize[1];
  dims(2,2) = sREF->voxsize[2];
  dims(3,0) = sREF->origin[0];
  dims(3,1) = sREF->origin[1];
  dims(3,2) = sREF->origin[2];
  dims(4,0) = sIMAGE->dimx;
  dims(4,1) = sIMAGE->dimy;
  dims(4,2) = sIMAGE->dimz;
  dims(5,0) = sIMAGE->voxsize[0];
  dims(5,1) = sIMAGE->voxsize[1];
  dims(5,2) = sIMAGE->voxsize[2];

  delete sIMAGE;
  delete sREF;
  return TRUE;
}

// dan_write_sn.cpp
// adapted from SPM96 code:
// spm_write_sn.m 1.4 John Ashburner MRCCU/FIL 96/09/11
// c/c++ port by Dan Kimberg

CrunchCube *
dan_write_sn(CrunchCube *VOL,Matrix &affine,Matrix &dims,Matrix &transform,
	     Matrix &MF,Matrix &bb,RowVector &Vox,int Hold,int affdefault)
{
  Matrix tx,ty,tz;
  Matrix MP,mtmp,mtmp1,Mult;
  Matrix basX,basY,basZ,X,Y;
  RowVector x,y,z,Dim(3,0.0),dv;
  Matrix X2,Y2,Z2,d;
  Matrix X1,Y1,Z1,Mask;
  int i,j,k,l,affine_only,d1,d2,d3,offset;
  CrunchCube *NEWVOL;

  offset=0;                // used later to keep track of plane as we're writing

  NEWVOL = new CrunchCube(VOL);
  MP = dan_get_space_image(VOL);

  // MP         maps this_image -> original_mm
  // MG         maps template   -> normalised_mm
  // MF         maps orig_image -> original_mm
  // Transform  maps template   -> orig_image
  // 
  // We want     template   -> this_image
  // 
  //       Transform          MF          inv(MP)
  // template -> orig_image -> original_mm -> this_image
  
  // from mm space to voxel space.

  x = dan_span(bb(0,0),bb(1,0),Vox(0));
  for(i=0; i < x.length(); i++)
    x(i) = (x(i) / dims(2,0)) + dims(3,0);

  y = dan_span(bb(0,1),bb(1,1),Vox(1));
  for(i=0; i < y.length(); i++)
    y(i) = (y(i) / dims(2,1)) + dims(3,1);

  z = dan_span(bb(0,2),bb(1,2),Vox(2));
  for(i=0; i < z.length(); i++)
    z(i) = (z(i) / dims(2,2)) + dims(3,2);

  Dim(0) = x.length();
  Dim(1) = y.length();
  Dim(2) = z.length();

  NEWVOL->voxsize[0] = Vox(0);
  NEWVOL->voxsize[1] = Vox(1);
  NEWVOL->voxsize[2] = Vox(2);
  NEWVOL->resize(x.length(),y.length(),z.length());

  NEWVOL->origin[0] = (int)round((-bb(0,0) / Vox(0)) + 1);
  NEWVOL->origin[1] = (int)round((-bb(0,1) / Vox(1)) + 1);
  NEWVOL->origin[2] = (int)round((-bb(0,2) / Vox(2)) + 1);

  // for some reason, can't multiply columnvector by rowvector
  mtmp.resize(x.length(),1);
  mtmp1.resize(1,(int)Dim(1));
  mtmp.insert(x.transpose(),0,0);
  mtmp1.fill(1.0);
  X = mtmp * mtmp1;

  mtmp.resize((int)Dim(0),1);
  mtmp1.resize(1,y.length());
  mtmp.fill(1.0);
  mtmp1.insert(y,0,0);
  Y = mtmp * mtmp1;

  affine_only = affdefault;             // if any zeros, do affine only
  for(i=0; i < dims.cols(); i++)
    if (dims(1,i) == 0.0)
      affine_only = 1;
  if (affine_only) {    
    basX = zeros(1,1);
    basY = zeros(1,1);
    basZ = zeros(1,1);
  }
  else {
    // is the -1 below to compensate for matlab's 1-indexing?  or do i need it?
    basX = dan_dctmtx((int)dims(0,0),(int)dims(1,0),x-1.0);
    basY = dan_dctmtx((int)dims(0,1),(int)dims(1,1),y-1.0);
    basZ = dan_dctmtx((int)dims(0,2),(int)dims(1,2),z-1.0);
  }

  // affine_only is 0 for test case

  // Cycle over planes

  for (j=0; j < z.length(); j++) {
    // Nonlinear deformations

    if (!affine_only) {
      // 2D transforms for each plane
      d1=(int)dims(1,0);
      d2=(int)dims(1,1);
      d3=(int)dims(1,2);
      ColumnVector cvtmp = basZ.row(j).transpose();
      mtmp = reshape(transform.column(0),d1*d2,d3);
      tx = reshape(mtmp * cvtmp,d1,d2);
      mtmp = reshape(transform.column(1),d1*d2,d3);
      ty = reshape(mtmp * cvtmp,d1,d2);
      mtmp = reshape(transform.column(2),d1*d2,d3);
      tz = reshape(mtmp * cvtmp,d1,d2);

      X1 = X + (basX * tx * basY.transpose());
      Y1 = Y + (basX * ty * basY.transpose());
      Z1 = z(j) + (basX * tz * basY.transpose());
    }

    // try it both ways -- doesn't matter!
    // Mult = backslash(MP,MF * affine);
    Mult = backslash(MP,MF) * affine;
    if (!affine_only) {
      X2= Mult(0,0)*X1 + Mult(0,1)*Y1 + Mult(0,2)*Z1 + Mult(0,3);
      Y2= Mult(1,0)*X1 + Mult(1,1)*Y1 + Mult(1,2)*Z1 + Mult(1,3);
      Z2= Mult(2,0)*X1 + Mult(2,1)*Y1 + Mult(2,2)*Z1 + Mult(2,3);
    }
    else {
      X2= Mult(0,0)*X + Mult(0,1)*Y + (Mult(0,2)*z(j) + Mult(0,3));
      Y2= Mult(1,0)*X + Mult(1,1)*Y + (Mult(1,2)*z(j) + Mult(1,3));
      Z2= Mult(2,0)*X + Mult(2,1)*Y + (Mult(2,2)*z(j) + Mult(2,3));
    }
    
    Mask = X2;  // just for the size, we're going to overwrite it
    for(k=0; k < X2.rows(); k++) {
      for(l=0; l < X2.cols(); l++) {
	if ((X2(k,l) >= 1-TINY) &&
	    (X2(k,l) <= (VOL->dimx+TINY)) &&
	    (Y2(k,l) >= 1-TINY) &&
	    (Y2(k,l) <= (VOL->dimy+TINY)) &&
	    (Z2(k,l) >= 1-TINY) &&
	    (Z2(k,l) <= (VOL->dimz+TINY)))
	  Mask(k,l)=1;
	else
	  Mask(k,l)=0;
      }
    }

    // try it both ways -- doesn't matter!
    // Mult = backslash(MP,MF * affine);

    Mult = backslash(MP,MF) * affine;

    if (!affine_only) {
      X2= Mult(0,0)*X1 + Mult(0,1)*Y1 + Mult(0,2)*Z1 + Mult(0,3);
      Y2= Mult(1,0)*X1 + Mult(1,1)*Y1 + Mult(1,2)*Z1 + Mult(1,3);
      Z2= Mult(2,0)*X1 + Mult(2,1)*Y1 + Mult(2,2)*Z1 + Mult(2,3);
    }
    else {
      X2= Mult(0,0)*X + Mult(0,1)*Y + (Mult(0,2)*z(j) + Mult(0,3));
      Y2= Mult(1,0)*X + Mult(1,1)*Y + (Mult(1,2)*z(j) + Mult(1,3));
      Z2= Mult(2,0)*X + Mult(2,1)*Y + (Mult(2,2)*z(j) + Mult(2,3));
    }

    d = dan_sample_vol(VOL,X2,Y2,Z2,Hold);
    
    for(k=0; k < d.rows(); k++)
      for(l=0; l < d.cols(); l++)
	d(k,l) *= Mask(k,l);
    dv = vectorize(d);
    
    // remove because we always use scale=1
    // for(k=0; k<dv.length(); k++)
    // dv(k) /= VOL->scalefactor;
    
    // bound the non-floating datatypes

    if (VOL->datatype == vb_byte)
      dan_bound(dv,(double)0,(double)255);
    else if (VOL->datatype == vb_short)
	dan_bound(dv,(double)-32768,(double)32767);
    else if (VOL->datatype == vb_long)
      dan_bound(dv,(double)-pow(2.0,32.0),(double)pow(2.0,31.0)-1);
    
    // write one plane of data
    
    for (k=0; k<dv.length(); k++) {
      if (VOL->datatype == vb_float)
	((float *)(NEWVOL->data))[k+offset] = (float) dv(k);
      else if (VOL->datatype == vb_double)
	((double *)(NEWVOL->data))[k+offset] = (double) dv(k);
      else if (VOL->datatype == vb_long)
	((int *)(NEWVOL->data))[k+offset] = (int) dv(k);
      else if (VOL->datatype == vb_short)
	((short *)(NEWVOL->data))[k+offset] = (short) dv(k);
      else if (VOL->datatype == vb_byte)
	((unsigned char *)(NEWVOL->data))[k+offset] = (unsigned char) dv(k);
      else
	cerr << "vbcrunch: bad datatype on writing normalized image to cube\n";
    }
    offset += dv.length();
  }
  return NEWVOL;
}
