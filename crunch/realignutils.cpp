
// realignutils.cpp
// a lot of the computational guts of the realignment routines
// new code Copyright (c) 1998-2002 by The VoxBo Development Team
// based closely on original MATLAB code by John Ashburner

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

// realign_twoimages()

CrunchCube *
realign_twoimages(CrunchCube *REF,CrunchCube *sREF,CrunchCube *VOL)
{
  Matrix dQ(6,6,0.0),dXdQ,X,Y;
  RowVector vtmp(9,0.0),q(6,0.0);
  Matrix bb,sb,C1,mtmp;
  Matrix B,x;
  Matrix d,dX,C2,y,q0,q1,M;
  RowVector S,sbv,q2,Xm,Ym,tmp,Mask,dv,Count;
  CrunchCube *sVOL,*NEWVOL;
  int i,j,k,Hold,h,Mx,Nx,n,err,dim[3];
  double a;

  // just some initialization, probably missed a few important ones
  err=0;

  // now into the direct SPM translation
  Hold = 3;

  // sREF = dan_smooth_image(REF,8,8,8)
  // the above commented out because now we smooth the ref once to do
  // an entire series

  // spm: center, bounding box, and margins
  bb = dan_bb_image(REF);
  // vectorize sb, because that's how we use it
  sb = diff(bb);
  sbv = vectorize(sb);
  // spm: compute matrices, dQ = 6 ortholinear transformation parameters
  a = PI/180;
  dQ(0,0) = 1;
  dQ(1,1) = 1;
  dQ(2,2) = 1;
  dQ(3,3) = a;
  dQ(4,4) = a;
  dQ(5,5) = a;

  // spm: define height of transverse slices (S) used in subsampling volume
  // V1(6) is map->voxsize[2]
  S = dan_linspace((bb(0,2) + 6/(sREF->voxsize[2])),(bb(1,2) - 8/(sREF->voxsize[2])),8);
  h = 5;                                // number of recursions
  Mx = (int)sbv(0);                     // rows per slice
  Nx = (int)sbv(1);                     // columns per slice
  n = Mx*Nx;                            // voxels per slice

  C1 = dan_get_space_image(REF);

  // spm: compute X (the reference image) and dX/dQ (the effects of moving X).

  X.resize(n*S.length(),1,0.0);
  Y.resize(n*S.length(),1,0.0);
  dXdQ.resize(n*S.length(),6,0.0);
  
  vtmp(0) = -(bb(0,0));
  vtmp(1) = -(bb(0,1));
  // vtmp(2) set inside the loop
  vtmp(3) = 0;
  vtmp(4) = 0;
  vtmp(5) = 0;
  vtmp(6) = 1;
  vtmp(7) = 1;
  vtmp(8) = 1;
  for (i=0; i < S.length(); i++) {
    vtmp(2) = -(S(i));
    B = dan_matrix(vtmp);
    x = dan_slice_vol(sREF,B.inverse(),Mx,Nx,Hold);
    copysegment(X,x,n,i*n);
    for (j=0; j < 6; j++) {
      mtmp = (B * (C1.inverse() * (dan_matrix(dQ.row(j)) * C1))).inverse();
      d = dan_slice_vol(sREF,mtmp,Mx,Nx,Hold);
      dX = d - x;
      copysegment(dXdQ,dX,n,(i*n)+(j*(dXdQ.rows())));
    }
  }

  // Here is the documentation from SPM on what this code does:
  //     least squares solution for Q the movements where:
  //     Y = X + dX/dQ.Q  => Q = [-dX/dQ Y]\X
  //     The estimate is repeated iteratively h times and the estimates of
  //     Q are cumulated arithmetically (an aproximation but good enough)

  //==================
  // this is the big loop
  // each image is smoothed and parameters are calculated
  //==================

  q.fill(0.0);
  C2 = dan_get_space_image(VOL);
  sVOL = dan_smooth_image(VOL,8,8,8);
  
  vtmp(0) = -(bb(0,0));
  vtmp(1) = -(bb(0,1));
  // vtmp(2) set within the loop
  vtmp(3) = 0;
  vtmp(4) = 0;
  vtmp(5) = 0;
  vtmp(6) = 1;
  vtmp(7) = 1;
  vtmp(8) = 1;
  for (i = 0; i < h; i++) {
    for (j = 0; j < S.length(); j++) {
      vtmp(2) = -(S(j));
      B = dan_matrix(vtmp);
      mtmp = (B * (C1.inverse() * (dan_matrix(q) * C2))).inverse();
      y = dan_slice_vol(sVOL,mtmp,Mx,Nx,Hold);
      copysegment(Y,y,n,j*n);
    }
    q1 = -dXdQ;
    q1 = q1.append(Y.column(0));
    q0 = backslash(q1,X);
    q2 = vectorize(q0);
    q2.resize(6,0.0);
    q = q - (RowVector)(dQ * q2.transpose());
  }

  delete sVOL;

  //================== saving realignment parameters
  VOL->M = dan_get_space_image(VOL);
  dan_get_space_image(VOL,dan_matrix(q) * VOL->M);
  //==================

  //================== apply realignment parameters
  // get properties of first image
  Hold = 5;
  M = dan_get_space_image(REF);
  dim[0] = (int)REF->dimx;
  dim[1] = (int)REF->dimy;
  dim[2] = (int)REF->dimz;
  //==================

  //// seem to be calling dan_get_space_image() more than necessary
  //// since the maps are now kept in memory, we can probably
  //// store the output of this function with the MAP instead of on disk

  // get properties of the rest of the images

  REF->M = backslash(M,dan_get_space_image(REF));
  VOL->M = backslash(M,dan_get_space_image(VOL));

  Xm.resize((int)(dim[0]*dim[1]),0.0);
  Ym.resize((int)(dim[0]*dim[1]),0.0);
  Xm.fill(0.0);
  Ym.fill(0.0);
  k=0;
  for (i=0; i<dim[1]; i++) {
    for(j=0; j<dim[0]; j++) {
      Xm(k) = j+1;
      Ym(k) = i+1;
      k++;
    }
  }
  
  // save x,y,z,pitch,roll,yaw
  NEWVOL = new CrunchCube(VOL);
  NEWVOL->transform = q;

  vtmp(0) = 0;
  vtmp(1) = 0;
  // vtmp(2) set inside the loop
  vtmp(3) = 0;
  vtmp(4) = 0;
  vtmp(5) = 0;
  vtmp(6) = 1;
  vtmp(7) = 1;
  vtmp(8) = 1;

  // now actually do the realignment!
  int offset = 0;
  for (j=1; j<=dim[2]; j++) {
    vtmp(2) = -j;
    B = dan_matrix(vtmp);

    // first calculate the masks for this plane
    Count.resize(dim[0]*dim[1],0.0);
    Mask.resize(dim[0]*dim[1],0.0);
    Mask.fill(0.0);

    REF->M1 = (B * REF->M).inverse();
    VOL->M1 = (B * VOL->M).inverse();
    // M1, M, and B appear to be good throughout
    Mask = mask_from_image(REF,Xm,Ym);
    Count = Mask;
    Mask = mask_from_image(VOL,Xm,Ym);
    Count = Count + Mask;
    for (k=0; k<Count.length(); k++) {
      if (Count(k) > 1)
        Mask(k)=1.0;
      else
        Mask(k)=0.0;
    }

    // formerly, loop across files started here
    // now, we just do the volume to be mapped
    
    //// make sure dan_slice_vol() doesn't modify the VOL directly
    
    d = dan_slice_vol(VOL,VOL->M1,dim[0],dim[1],Hold);
    dv = vectorize(d);
    
    // apply the mask

    // bound the image according to the datatype -- not necessary for floats
    if (VOL->datatype == vb_byte)
      dan_bound(dv,(double)0,(double)255);
    else if (VOL->datatype == vb_short)
      dan_bound(dv,(double)-32768,(double)32767); 
    else if (VOL->datatype == vb_long)
      dan_bound(dv,-(double)pow(2.0,31.0),(double)pow(2.0,31.0)-1);
    
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
    }
    offset += dv.length();
  }
  return NEWVOL;
}

RowVector
mask_from_image(CrunchCube *map,RowVector &Xm,RowVector &Ym)
{
  RowVector Mask,tmp;
  int k;

  tmp.resize(Xm.length());
  tmp.fill(0.0);
  Mask.resize(Xm.length());
  Mask.fill(0.0);
  for (k=0; k<tmp.length(); k++) {
    tmp(k) = (map->M1(0,0)*Xm(k)) + (map->M1(0,1)*Ym(k)) + (map->M1(0,3));
    if ((tmp(k) >= (1.0-TINY)) && (tmp(k) <= ((double)map->dimx+TINY)))
      Mask(k)=1;
    else
      Mask(k)=0;
  }
  for (k=0; k<tmp.length(); k++) {
    tmp(k) = (map->M1(1,0) * Xm(k)) + (map->M1(1,1) * Ym(k)) + (map->M1(1,3));
    if ((tmp(k) >= (1.0-TINY)) && (tmp(k) <= ((double)map->dimy+TINY)) && Mask(k))
      Mask(k)=1;
    else
      Mask(k) = 0;
  }
  for (k=0; k<tmp.length(); k++) {
    tmp(k) = (map->M1(2,0)*Xm(k)) + (map->M1(2,1)*Ym(k)) + (map->M1(2,3));
    if ((tmp(k) >= (1.0-TINY)) && (tmp(k) <= ((double)map->dimz+TINY)) && Mask(k))
      Mask(k)=1;
    else
      Mask(k) = 0;
  }
  return Mask;
}













// dan_bb_image()

Matrix
dan_bb_image(CrunchCube *map)
{
  Matrix bb(2,3);
  double meanx,meany,meanz;
  double xmin,ymin;
  int minx,maxx,miny,maxy,minz,maxz,i;

  RowVector x((int)map->dimx,0.0);
  RowVector y((int)map->dimy,0.0);
  RowVector z((int)map->dimz,0.0);
  
  maxx=maxy=maxz=0;
  meanx = meany = meanz = 0.0;

  dan_box_image(map,x,y,z);

  xmin = x.min();
  ymin = y.min();
  
  x = x - xmin;
  y = y - ymin;

  for(i=0;i<x.length();i++)
    meanx+=x(i);
  meanx /= (double)x.length();

  for(i=0;i<y.length();i++)
    meany+=y(i);
  meany /= (double)y.length();

  for(i=0;i<z.length();i++)
    meanz+=z(i);
  meanz /= (double)z.length();

  meanx /= 2.0;
  meany /= 2.0;
  meanz /= 2.0;

  minx=miny=minz=maxx=maxy=maxz=0;
  for(i=0;i<x.length();i++) {
    if (x(i) >= meanx) {
      maxx = i+1;
      if (!minx) minx=i+1;
    }
  }
  for(i=0;i<y.length();i++) {
    if (y(i) >= meany) {
      maxy = i+1;
      if (!miny) miny=i+1;
    }
  }
  for(i=0;i<z.length();i++) {
    if (z(i) >= meanz) {
      maxz = i+1;
      if (!minz) minz=i+1;
    }
  }

  bb(0,0) = minx;
  bb(1,0) = maxx;
  bb(0,1) = miny;
  bb(1,1) = maxy;
  bb(0,2) = minz;
  bb(1,2) = maxz;

  return bb;
}

void
dan_box_image(CrunchCube *map,RowVector &X,RowVector &Y,RowVector &Z)
{
  double *kd;
  int d[3];
  int i,x,y,z,N,*ki;
  unsigned char *ku;
  short *ks;
  float *kf;

  d[0] = (int)map->dimx;
  d[1] = (int)map->dimy;
  d[2] = (int)map->dimz;
  N = d[0]*d[1]*d[2];

  // integrate
  
  if (map->datatype == vb_byte) {
    ku = (unsigned char *)map->data;
    for (i = 0; i < N; i++) {
      z     = (int) floor((double)i / (d[0]*d[1]));
      y     = (int) floor((double)(i - z*(d[0]*d[1]) ) / (d[0]));
      x     = (int) i - z*(d[0]*d[1]) - y*d[0];
      X(x) += (double) ku[i];
      Y(y) += (double) ku[i];
      Z(z) += (double) ku[i];
    }
  }
  else if (map->datatype == vb_short) {
    ks = (short *)map->data;
    for (i = 0; i < N; i++) {
      z     = (int) floor((double)i / (d[0]*d[1]));
      y     = (int) floor((double)(i - z*(d[0]*d[1]) ) / (d[0]));
      x     = (int) i - z*(d[0]*d[1]) - y*d[0];
      X(x) += (double) ks[i];
      Y(y) += (double) ks[i];
      Z(z) += (double) ks[i];
    }
  }
  else if (map->datatype == vb_long) {
    ki = (int *)map->data;
    for (i = 0; i < N; i++) {
      z     = (int) floor((double)i / (d[0]*d[1]));
      y     = (int) floor((double)(i - z*(d[0]*d[1]) ) / (d[0]));
      x     = (int) i - z*(d[0]*d[1]) - y*d[0];
      X(x) += (double) ki[i];
      Y(y) += (double) ki[i];
      Z(z) += (double) ki[i];
    }
  }
  else if (map->datatype == vb_float) {
    kf = (float *)map->data;
    for (i = 0; i < N; i++) {
      z     = (int) floor((double)i / (d[0]*d[1]));
      y     = (int) floor((double)(i - z*(d[0]*d[1]) ) / (d[0]));
      x     = (int) i - z*(d[0]*d[1]) - y*d[0];
      X(x) += (double) kf[i];
      Y(y) += (double) kf[i];
      Z(z) += (double) kf[i];
    }
  }
  else if (map->datatype == vb_double) {
    kd = (double *)map->data;
    for (i = 0; i < N; i++) {
      z     = (int) floor((double)i / (d[0]*d[1]));
      y     = (int) floor((double)(i - z*(d[0]*d[1]) ) / (d[0]));
      x     = (int) i - z*(d[0]*d[1]) - y*d[0];
      X(x) += (double) kd[i];
      Y(y) += (double) kd[i];
      Z(z) += (double) kd[i];
    }
  }
  else
    cerr << "error: dan_box() data type not supported" << endl;
}

void
dan_bound(RowVector &vec,double low,double high)
{
  int i;

  for (i=0; i<vec.length(); i++) {
    vec(i) = round(vec(i));
    if (vec(i) < low) vec(i) = low;
    if (vec(i) > high) vec(i) = high;
  }
}

// dan_matrix()

Matrix
dan_matrix(const RowVector &P)
{
  RowVector p;
  Matrix A(4,4),B(4,4),AX(4,4);
  int n,i;
  
  n = P.length();
  p=P;
  p.resize(12,0.0);
  for (i=n; i<12; i++) {
    if (i < 9 && i > 5)
      p(i)=1;
    else
      p(i)=0;
  }
  ident(A);
  ident(B);

  B(0,3)=p(0);
  B(1,3)=p(1);
  B(2,3)=p(2);
  AX = A * B;
  
  ident(B);
  B(1,1)=cos(p(3));
  B(1,2)=sin(p(3));
  B(2,1)=-sin(p(3));
  B(2,2)=cos(p(3));
  A = AX * B;

  ident(B);
  B(0,0)=cos(p(4));
  B(0,2)=sin(p(4));
  B(2,0)=-sin(p(4));
  B(2,2)=cos(p(4));
  AX = A * B;

  ident(B);
  B(0,0)=cos(p(5));
  B(0,1)=sin(p(5));
  B(1,0)=-sin(p(5));
  B(1,1)=cos(p(5));
  A = AX * B;

  ident(B);
  B(0,0)=p(6);
  B(1,1)=p(7);
  B(2,2)=p(8);
  AX = A * B;

  ident(B);
  B(0,1)=p(9);
  B(0,2)=p(10);
  B(1,2)=p(11);
  A = AX * B;

  return A;
}


// dan_slice_vol()
Matrix
dan_slice_vol(CrunchCube *map,const Matrix &mat,int m,int n,int hold)
{
    int xdim,ydim,zdim,status,i,j,k;
    double background;
    Matrix Y;
    double *img;
    
    background=0.0;
    
    xdim = abs((int)map->dimx);
    ydim = abs((int)map->dimy);
    zdim = abs((int)map->dimz);

    // get transformation matrix
    
    img = new double[m*n];
    for (i=0; i<m*n; i++)
      img[i]=0.0;
    
    status = 1;
    if (map->datatype == vb_long)
      status = slice_int_sinc(mat,img,m,n,(int *)map->data,xdim,ydim,zdim,
			      hold, background);
    else if (map->datatype == vb_short)
      status = slice_short_sinc(mat,img,m,n,(short *)map->data,xdim,ydim,zdim,
				hold, background);
    //        {
    //  	for (int z=0; z<4; z++) {
    //  	  status = slice_short_sinc_threaded(mat,img,m,n,(short *)map->data,xdim,ydim,zdim,
    //  					     hold, background);
    //  	}
    //        }
    else if (map->datatype == vb_byte)
      status = slice_sinc(mat,img,m,n,(unsigned char *)(map->data),xdim,ydim,zdim,
			  hold, background);
    else if (map->datatype == vb_float)
      status = slice_sinc(mat,img,m,n,(float *)(map->data),xdim,ydim,zdim,
			  hold, background);
    else if (map->datatype == vb_double)
      status = slice_sinc(mat,img,m,n,(double *)(map->data),xdim,ydim,zdim,
			  hold, background);
    else {
      cerr << "error: dan_slice_vol(): data type other than signed int or short" << endl;
    }
    
    if (status)
      cerr << "error: dan_slice_vol(): slicing failed\n" << endl;
    Y.resize(m,n,0.0);
    k=0;
    for(i=0; i<n; i++)
      for(j=0; j<m; j++)
	Y(j,i)=img[k++];
    delete [] img;
    return Y;
}

template<class T>
int
slice_sinc(const Matrix &mat,double *image,int xdim1,int ydim1,T *vol,
	   int xdim2,int ydim2,int zdim2,int nn,double background)
{
  double dat2,*tp2;
  register T *dp1,*dp2,*dp3;  // register
  register double dat3;
  register double *tp3;
  int dim1xdim2 = xdim2*ydim2;
  int dx1, dy1, dz1;
  static double tablex[255], tabley[255], tablez[255];
  double y, dx3, dy3, dz3, ds3;
  // new ones
  double x,x3,y3,z3,s3,x4,y4,z4,dat,*tp1,*tp1end,*tp2end,*tp3end;

  dx3=mat(0,0);
  dy3=mat(1,0);
  dz3=mat(2,0);
  ds3=mat(3,0);
  
  vol -= (1 + xdim2*(1 + ydim2));
  
  for(y=1; y<=ydim1; y++)
    {

      x3 = mat(0,3) + y*mat(0,1);
      y3 = mat(1,3) + y*mat(1,1);
      z3 = mat(2,3) + y*mat(2,1);
      s3 = mat(3,3) + y*mat(3,1);

      for(x=1; x<=xdim1; x++)
	{

	  s3 += ds3;
	  if (s3 == 0.0) return(-1);
	  x4=(x3 += dx3)/s3;
	  y4=(y3 += dy3)/s3;
	  z4=(z3 += dz3)/s3;

	  if (z4>=1.0-TINY && z4<=(double)zdim2+TINY &&
	      y4>=1.0-TINY && y4<=(double)ydim2+TINY &&
	      x4>=1.0-TINY && x4<=(double)xdim2+TINY)
	    {
	      dat=0.0;

	      make_lookup(x4, nn, xdim2, &dx1, tablex, &tp3end);
	      make_lookup(y4 , nn, ydim2, &dy1, tabley, &tp2end);
	      make_lookup(z4, nn, zdim2, &dz1, tablez, &tp1end);

	      tp1 = tablez;
	      dy1 *= xdim2;
	      dp1 = vol + dim1xdim2*dz1;

	      while(tp1 <= tp1end)
		{
		  dp2 = dp1 + dy1;
		  dat2 = 0.0; tp2 = tabley;
		  while (tp2 <= tp2end)
		    {
		      dat3 = 0.0; tp3 = tablex;
		      dp3 = dp2 + dx1;
		      while(tp3 <= tp3end)
			dat3 += *(dp3++) * *(tp3++);
		      dat2 += dat3 * *(tp2++);
		      dp2 += xdim2;
		    }
		  dat += dat2 * *(tp1++);
		  dp1 += dim1xdim2;
		}
	      *(image++) = dat;
	    }
	  else *(image++) = background;
	}
    }
  return(0);
}

