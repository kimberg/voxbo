
// vb_render.cpp
// functionality to support rendering of brain images and overlays
// Copyright (c) 1998-2005 by The VoxBo Development Team

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

#include "vb_render.h"

VBRenderer::VBRenderer()
{
  low=3.5;
  high=5.0;
  twotailed=1;
  anatomy=mask=overlay=NULL;
}

int
VBRenderer::SavePNG(string &fname)
{
  if (qim.save(fname.c_str(),"PNG"))
    return 0;
  else
    return 100;
}

int
VBRenderer::RenderSlices()
{
  int xpos=0,ypos=0,xo,yo;
  int rows=(anatomy->dimz+4)/5;

  if (!qim.create(anatomy->dimx*5,anatomy->dimy*rows,32))
    return 100;

  // note that the following code assumes we have an overlay that has
  // the same slices as our anatomical

  // FIXME the following code doesn't lead to anything, but we really
  // need to figure out the extent, in millimeters, of the anatomical
  // anatomy and the overlay, and then do the right thing
//   double x0=(0.0-anatomy->origin[0])*anatomy->voxsize[0];
//   double x1=(anatomy->dimx-anatomy->origin[0])*anatomy->voxsize[0];
//   double y0=(0.0-anatomy->origin[1])*anatomy->voxsize[1];
//   double y1=(anatomy->dimy-anatomy->origin[1])*anatomy->voxsize[1];
  
  int xratio=0,yratio=0;

  if (overlay) {
    xratio=anatomy->dimx/overlay->dimx;
    yratio=anatomy->dimy/overlay->dimy;
  }

  for (int i=0; i<anatomy->dimz; i++) {
    xo=xpos*anatomy->dimx;
    yo=ypos*anatomy->dimy;
    RenderSlice(i,1,1,xo,yo);
    if (overlay)
      // if (anatomy->dimx==overlay->dimx)
        RenderOverlay(i,xratio,yratio,xo,yo);
    // RenderMask(anatomy,qim,1,1,xo,yo);
    // RenderSlice(anatomy,qim,1,1,xo,yo);
    xpos+=1;
    if (xpos>4) {
      xpos=0;
      ypos++;
    }
  }
  return 0;
}

void
VBRenderer::RenderSlice(int slice,int xmag,int ymag,int xoffset,int yoffset)
{
  int i,j,k,l,rr,gg,bb;
  unsigned int *p;
  int val,scanline;

  for(i=0; i<anatomy->dimx; i++) {
    for(j=0; j<anatomy->dimy; j++) {
      // figure out the greyscale value
      val=scaledvalue(anatomy->get_minimum(),anatomy->get_maximum(),60,80,anatomy->GetValue(i,j,slice));
      rr=gg=bb=val;

      // render the actual voxel, taking the magnification into account
      for(k=0; k<ymag; k++) {
        scanline=yoffset+((anatomy->dimy-j-1)*ymag)+k;
        p=(unsigned int *)qim.scanLine(scanline);
        p+=xoffset+xmag*i;
        for(l=0; l<xmag; l++) {
	  *p=qRgb(rr,gg,bb);
	  p++;
	}
      }
    }
  }
}

void
VBRenderer::RenderOverlay(int slice,int xmag,int ymag,int xoffset,int yoffset)
{
  int i,j,k,l,rr,gg,bb,scanline,ind;
  unsigned int *p;
  double val;
  QRgb oldrgb;

  for(i=0; i<overlay->dimx; i++) {
    for(j=0; j<overlay->dimy; j++) {
      // figure out the scale index
      val=overlay->GetValue(i,j,slice);
      if (val<0 && twotailed==0) continue;
      if (fabs(val) < low) continue;
      // calculate a 0 to 39 index on the red-yellow scale
      ind=(int)(40.0*(fabs(val)-low)/(high-low));
      if (ind > 39) ind=39;
      
      if (val < 0) {
        rr=0;
        gg=(int)(6.36*ind+0.5);
        bb=255;
      }
      else {
        rr=255;
        gg=(int)(6.36*ind+0.5);
        bb=0;
      }
      
      // render the actual voxel, taking the magnification into account
      for(k=0; k<ymag; k++) {
        scanline=yoffset+((overlay->dimy-j-1)*ymag)+k;
        p=(unsigned int *)qim.scanLine(scanline);
        p+=xoffset+xmag*i;
        for(l=0; l<xmag; l++) {
          oldrgb=*p;
          //           rr=rr*qRed(oldrgb)/255;
          //           gg=gg*qGreen(oldrgb)/255;
          //           bb=bb*qBlue(oldrgb)/255;
          //           rr=qRed(oldrgb);
          //           gg=qGreen(oldrgb);
          //           bb=qBlue(oldrgb);
          // *p=qRgb(rr*qRed(oldrgb)/255,gg*qGreen(oldrgb)/255,bb*qBlue(oldrgb)/255);
          *p=qRgb(rr,gg,bb);
          p++;
        }
      }
    }
  }
}


inline short
scaledvalue(double minval,double maxval,double brightness,double contrast,double val)
{
  double onewidth=maxval-minval;
  double center=minval+(onewidth/2.0);
  center-=(double)(brightness*onewidth)/100.0;
  center=(maxval+(onewidth/2.0))-(brightness*onewidth*2.0)/100.0;
  double contrasthalfwidth=(onewidth*2.0*(101.0-contrast))/200.0;
  double low=center-contrasthalfwidth;
  double high=center+contrasthalfwidth;
  double bval = (((val - low) * 255.0 / (high - low)));
  if (bval > 255) bval=255;
  if (bval < 0) bval=0;
  return (short)bval;
}
