
// vbview_render.cpp
// vbview's render function
// Copyright (c) 1998-2010 by The VoxBo Development Team

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

#include "vbview.h"

// prototypes for purely local functions
inline QRgb alphablend(QRgb oldval,QRgb newval,int alpha);
void inline affine_add1x(VBMatrix &pos,VBMatrix &trans);
void inline affine_cr(VBMatrix &pos,VBMatrix &trans,int nx);

void
VBView::RenderAll()
{
  if (!q_update) return;
  for (int i=0; i<(int)viewlist.size(); i++)
    RenderSingle(viewlist[i]);
  drawarea->update();
}

void
VBView::RenderSingle(MyView &view)
{
  if (!q_update) return;
  if (view.orient==MyView::vb_xy)
    UniRender(view);
  else if (view.orient==MyView::vb_yz)
    UniRender(view);
  else if (view.orient==MyView::vb_xz)
    UniRender(view);
}

void
VBView::UniRender(vbrect rrect)
{
  // iterate through views, find ones whose 
  for (int i=0; i<(int)viewlist.size(); i++) {
    vbrect vrect(viewlist[i].xoff,viewlist[i].yoff,
                 viewlist[i].width,viewlist[i].height);
    vrect=vrect&rrect;  // intersection of view rect and render rect
    if (vrect)
      RenderSingle(viewlist[i]);
  }
}

void
VBView::UniRender(MyView &view)
{
  UniRenderView(view);
  return;
  if (!q_update)
    return;
  if (!layers.size())
    return;
  for (VBLayerI l=layers.begin(); l!=layers.end(); l++) {
    if (l==layers.begin() || (l->q_visible && l->alpha > 0))
      {};//UniRenderLayer(view,l);
  }
}

void
VBView::UniRenderView(MyView &view)
{
  if (!q_update)
    return;
  if (!layers.size())
    return;
  for (VBLayerI l=layers.begin(); l!=layers.end(); l++) {
    if (l==layers.begin() || (l->q_visible && l->alpha > 0))
      UniRenderLayer(view,l,vbrect(view.xoff,view.yoff,view.width,view.height));
  }
}

void
VBView::UniRenderLayer(MyView &view,VBLayerI layer,vbrect rrect)
{
  if (!q_update) return;
  if (!layer->q_visible) return;

  uint32 *p;
  VBMatrix traverse_x,traverse_cr,full;
  // calculate the transformation from window coordinates to this
  // layer's rendercube coordinates
  int pos=view.position;
  if (view.position==-1) {
    if (view.orient==MyView::vb_xy) pos=q_zslice;
    else if (view.orient==MyView::vb_yz) pos=q_xslice;
    else if (view.orient==MyView::vb_xz) pos=q_yslice;
  }
  layer->calcFullTransform(q_xscale,q_yscale,q_zscale,
                           view.xoff,view.yoff,view.width,
                           view.height,view.orient,pos,
                           q_fliph,q_flipv);
  full=layer->full; // for convenience
  // grab corner (0,0,0) voxel
  VBMatrix position(4,1);
  position.set(0,0,rrect.x);
  position.set(1,0,rrect.y);
  position.set(2,0,0);
  position.set(3,0,1);
  // now premultiply to get corresponding coord in rendercube
  // position^=layer->full;
  position^=full;
  traverse_x.init(4,1);
  traverse_x.set(0,0,full(0,0));
  traverse_x.set(1,0,full(1,0));
  traverse_x.set(2,0,full(2,0));
  traverse_x.set(3,0,0);
  traverse_cr.init(4,1);

  // the below replaced with the even more below, because we're
  // sometimes not drawing the entire width of the view
  traverse_cr.set(0,0,full(0,1)-(full(0,0)*rrect.w));
  traverse_cr.set(1,0,full(1,1)-(full(1,0)*rrect.w));
  traverse_cr.set(2,0,full(2,1)-(full(2,0)*rrect.w));

  traverse_cr.set(3,0,0);
  // voila!  now, each time we increment x or y, we can do it without
  // matrix multiplication

  // let's figure out which values of i and j below should get
  // crosshairs.  find the coord in image space and map backwards.
  // int xcross=-1,ycross=-1;
  // only calculate crosshairs for the base layer, but we'll render
  // them for all layers
  if (q_crosshairs && layer==layers.begin()) {
    VBMatrix tmpm(4,1),tmp2(4,4);
    invert(full,tmp2);
    tmpm.set(3,0,1);
    // we add 0.5 so that we can be roughly in the middle of the
    // voxel, not the corner
    if (view.orient==MyView::vb_xy) {
      tmpm.set(0,0,(double)q_xslice+0.5);
      tmpm.set(1,0,(double)q_yslice+0.5);
    }
    else if (view.orient==MyView::vb_yz) {
      tmpm.set(1,0,(double)q_yslice+0.5);
      tmpm.set(2,0,(double)q_zslice+0.5);
    }
    else if (view.orient==MyView::vb_xz) {
      tmpm.set(0,0,(double)q_xslice+0.5);
      tmpm.set(2,0,(double)q_zslice+0.5);
    }
    tmp2*=tmpm;
    xcross=(int)(tmp2(0,0));
    ycross=(int)(tmp2(1,0));
  }
  
  double xpos[3],xtx[3],xtcr[3];
  xpos[0]=position(0,0);
  xpos[1]=position(1,0);
  xpos[2]=position(2,0);
  xtx[0]=traverse_x(0,0);
  xtx[1]=traverse_x(1,0);
  xtx[2]=traverse_x(2,0);
  xtcr[0]=traverse_cr(0,0);
  xtcr[1]=traverse_cr(1,0);
  xtcr[2]=traverse_cr(2,0);
  QRgb newval,oldval;
  bool f_blend=(layer!=layers.begin());
  int32 xx,yy,zz;
  VBRegion myorigin;
  for (int j=rrect.y; j<rrect.y+rrect.h; j++) {
    p=(uint32 *)currentimage.scanLine(j);
    p+=(rrect.x);
    for (int i=rrect.x; i<rrect.x+rrect.w; i++) {
      oldval=*p;
      // we truncate these floating point coordinates, because when
      // you're magnifying a voxel, you don't start using the next
      // voxel's value until you cross the border.
      xx=(int)xpos[0];
      yy=(int)xpos[1];
      zz=(int)xpos[2];
      newval=layer->rendercube.getValue<int32>(xx,yy,zz);
      if (q_showorigin && xx==layer->rendercube.origin[0] && yy==layer->rendercube.origin[1] &&
          zz==layer->rendercube.origin[2]) {
        myorigin.add(i,j,0,0);
      }
      if (f_blend) {
        if (newval>>24) {
          newval=alphablend(oldval,newval,layer->alpha);
          *p=newval;
        }
      }
      else
        *p=newval;
      if (q_crosshairs && (j==ycross || i==xcross)) {
        *p=qRgb(255,0,0);
      }
      p++;
      xpos[0]+=xtx[0];
      xpos[1]+=xtx[1];
      xpos[2]+=xtx[2];
    }
    xpos[0]+=xtcr[0];
    xpos[1]+=xtcr[1];
    xpos[2]+=xtcr[2];
  }
  if (myorigin.size()) {
    double x,y,z;
    myorigin.GeometricCenter(x,y,z);
    // FIXME not 11, should adjust to magnification
    int32 x1=x-11,xn=x+11;
    int32 y1=y-11,yn=y+11;
    if (x1<view.xoff) x1=view.xoff;
    if (xn>view.xoff+view.width-1) xn=view.xoff+view.width-1;
    if (y1<view.yoff) y1=view.yoff;
    if (yn>view.yoff+view.height-1) yn=view.yoff+view.height-1;
    for (int j=y1; j<=yn; j++) {
      p=(uint32 *)currentimage.scanLine(j);
      p+=x1;

      for (int i=x1; i<=xn; i++) {
        double dist=sqrt((double)((i-x)*(i-x))
                         +(double)((j-y)*(j-y)));
        if (dist>9.5 && dist < 10.5)
          *p=qRgb(255,255,0);
        p++;
      }
    }
  }
  drawarea->update();
}

void inline
affine_add1x(VBMatrix &pos,VBMatrix &trans)
{
  pos.set(0,0,pos(0,0)+trans(0,0));
  pos.set(1,0,pos(1,0)+trans(1,0));
  pos.set(2,0,pos(2,0)+trans(2,0));
}

void inline
affine_cr(VBMatrix &pos,VBMatrix &trans,int nx)
{
  // first get back to the first voxel in this row
  pos.set(0,0,pos(0,0)-(trans(0,0)*nx));
  pos.set(1,0,pos(1,0)-(trans(1,0)*nx));
  pos.set(2,0,pos(2,0)-(trans(2,0)*nx));
  // now increment y
  pos.set(0,0,pos(0,0)+trans(0,1));
  pos.set(1,0,pos(1,0)+trans(1,1));
  pos.set(2,0,pos(2,0)+trans(2,1));
 
}

inline QRgb
alphablend(QRgb oldval,QRgb newval,int alpha)
{
  int oldfact=100-alpha;
  int newfact=alpha;
  int rr=(((oldval>>16)&0xff)*oldfact/100)+(((newval>>16)&0xff)*newfact/100);
  int gg=(((oldval>>8)&0xff)*oldfact/100)+(((newval>>8)&0xff)*newfact/100);
  int bb=(((oldval)&0xff)*oldfact/100)+(((newval)&0xff)*newfact/100);
  return qRgb(rr,gg,bb);
}


// fillmask() takes a view, location in that view, selected mask
// layer, and color.  it figures out which mask voxel was clicked and
// executes a fill to create the region to be colored.  then it calls
// colorpixels().

int32
VBView::fillmask(MyView &view,VBVoxel loc,VBLayerI masklayer,uint32 maskcolor)
{
  // calculate the transformation from window coordinates to this
  // layer's rendercube coordinates
  int pos=view.position;
  if (view.position==-1) {
    if (view.orient==MyView::vb_xy) pos=q_zslice;
    else if (view.orient==MyView::vb_yz) pos=q_xslice;
    else if (view.orient==MyView::vb_xz) pos=q_yslice;
  }
  masklayer->calcFullTransform(q_xscale,q_yscale,q_zscale,
                               view.xoff,view.yoff,view.width,
                               view.height,view.orient,pos,
                               q_fliph,q_flipv);
  VBMatrix full=masklayer->full; // for convenience
  VBMatrix position(4,1);
  // calculate the mask coordinates we're working on from the base
  // coord provided
  int xx,yy,zz;
  VBRegion maskreg;
  position.set(0,0,loc.x);
  position.set(1,0,loc.y);
  position.set(2,0,0);
  position.set(3,0,1);
  position^=full;    // premultiply to get layer coordinate
  xx=(int)position(0,0);
  yy=(int)position(1,0);
  zz=(int)position(2,0);
  maskreg.add(xx,yy,zz);

  // the preceding is like colormask()

  int xrad=1,yrad=1,zrad=1;
  if (view.orient==MyView::vb_xy) zrad=0;
  else if (view.orient==MyView::vb_yz) xrad=0;
  else if (view.orient==MyView::vb_xz) yrad=0;

  VBRegion queue,visited;
  queue.add(xx,yy,zz);
  int32 oldvalue=masklayer->cube.getValue<int32>(xx,yy,zz);
  while (queue.size()) {
    // pop the first voxel
    VBVoxel vv=queue.begin()->second;
    queue.remove(vv.x,vv.y,vv.z);
    visited.add(vv.x,vv.y,vv.z);
    // see if it's the right oldvalue
    if (masklayer->cube.getValue<int32>(vv.x,vv.y,vv.z)==oldvalue)
      maskreg.add(vv.x,vv.y,vv.z);
    else
      continue;

    if (xrad && vv.x>0)
      if (!(visited.contains(vv.x-1,vv.y,vv.z)))
        queue.add(vv.x-1,vv.y,vv.z);
    if (xrad && vv.x<masklayer->cube.dimx-1)
      if (!visited.contains(vv.x+1,vv.y,vv.z))
        queue.add(vv.x+1,vv.y,vv.z);
    if (yrad && vv.y>0)
      if (!visited.contains(vv.x,vv.y-1,vv.z))
        queue.add(vv.x,vv.y-1,vv.z);
    if (yrad && vv.y<masklayer->cube.dimy-1)
      if (!visited.contains(vv.x,vv.y+1,vv.z))
        queue.add(vv.x,vv.y+1,vv.z);
    if (zrad && vv.z>0)
      if (!visited.contains(vv.x,vv.y,vv.z-1))
        queue.add(vv.x,vv.y,vv.z-1);
    if (zrad && vv.z<masklayer->cube.dimz-1)
      if (!visited.contains(vv.x,vv.y,vv.z+1))
        queue.add(vv.x,vv.y,vv.z+1);
  }

  // what follows is just like colormask()

  for (VI mv=maskreg.begin(); mv!=maskreg.end(); mv++) {
    // if needed, push the old value on the current undo list
    if (!(masklayer->undo.front().contains(mv->second.x,mv->second.y,mv->second.z)))
      masklayer->undo.front().add(mv->second.x,mv->second.y,mv->second.z,
                                  masklayer->cube.GetValue(mv->second.x,mv->second.y,mv->second.z));
    // now write the new value
    masklayer->cube.SetValue(mv->second.x,mv->second.y,mv->second.z,(maskcolor?q_maskindex:0));
    masklayer->rendercube.SetValue(mv->second.x,mv->second.y,mv->second.z,maskcolor);
  }
  VBVoxel cvox(xx,yy,zz);
  vbforeach(MyView &view,viewlist) {
    colorpixels(view,cvox,maskreg,masklayer);
  }
  return 0;
}

// colormask() takes a view, a location in that view, the selected
// mask layer, and the new mask color.  it figures out which mask
// voxel was clicked, expands the region to be colored using the
// current radius, and then calls colormask_affine_2().

int32
VBView::colormask(MyView &view,VBVoxel loc,VBLayerI masklayer,uint32 maskcolor)
{
  // calculate the transformation from window coordinates to this
  // layer's rendercube coordinates
  int pos=view.position;
  if (view.position==-1) {
    if (view.orient==MyView::vb_xy) pos=q_zslice;
    else if (view.orient==MyView::vb_yz) pos=q_xslice;
    else if (view.orient==MyView::vb_xz) pos=q_yslice;
  }
  masklayer->calcFullTransform(q_xscale,q_yscale,q_zscale,
                               view.xoff,view.yoff,view.width,
                               view.height,view.orient,pos,
                               q_fliph,q_flipv);
  VBMatrix full=masklayer->full; // for convenience
  VBMatrix position(4,1);
  // calculate the mask coordinates we're working on from the base
  // coord provided
  int xx,yy,zz;
  VBRegion maskreg;
  position.set(0,0,loc.x);
  position.set(1,0,loc.y);
  position.set(2,0,0);
  position.set(3,0,1);
  position^=full;    // premultiply to get layer coordinate
  xx=(int)position(0,0);
  yy=(int)position(1,0);
  zz=(int)position(2,0);
  int zr=(q_usezradius ? q_radius : 0);
  maskreg.add(xx,yy,zz);
  double dist;
  for (int i=xx-q_radius; i<=xx+q_radius; i++) {
    if (i<0 || i>masklayer->cube.dimx-1) continue;
    for (int j=yy-q_radius; j<=yy+q_radius; j++) {
      if (i<0 || i>masklayer->cube.dimx-1) continue;
      for (int k=zz-zr; k<=zz+zr; k++) {
        if (i<0 || i>masklayer->cube.dimx-1) continue;
        dist=sqrt(((xx-i)*(xx-i))+((yy-j)*(yy-j))+((zz-k)*(zz-k)));
        if (dist<=q_radius)
          maskreg.add(i,j,k);
      }
    }
  }
  for (VI mv=maskreg.begin(); mv!=maskreg.end(); mv++) {
    // if needed, push the old value on the current undo list
    if (!(masklayer->undo.front().contains(mv->second.x,mv->second.y,mv->second.z)))
      masklayer->undo.front().add(mv->second.x,mv->second.y,mv->second.z,
                                  masklayer->cube.GetValue(mv->second.x,mv->second.y,mv->second.z));
    // now write the new value
    masklayer->cube.SetValue(mv->second.x,mv->second.y,mv->second.z,(maskcolor?q_maskindex:0));
    masklayer->rendercube.SetValue(mv->second.x,mv->second.y,mv->second.z,maskcolor);
  }
  VBVoxel cvox(xx,yy,zz);
  vbforeach(MyView &view,viewlist) {
    colorpixels(view,cvox,maskreg,masklayer);
  }
  return 0;
}

// the below function does just one thing, which is to find the
// affected region of the view and render it.  doing this is a hassle
// compared to just drawing the whole view, but is fast, and allows us
// to avoid redrawing each view in its entirety when drawing masks,
// which means mask drawing can be nice and fast, even on slowish
// machines

int32
VBView::colorpixels(MyView &view,VBVoxel cvox,VBRegion &maskreg,VBLayerI masklayer)
{
  // calculate the transformation from window coordinates to this
  // layer's rendercube coordinates
  int pos=view.position;
  if (pos==-1) {
    if (view.orient==MyView::vb_xy) pos=q_zslice;
    else if (view.orient==MyView::vb_yz) pos=q_xslice;
    else if (view.orient==MyView::vb_xz) pos=q_yslice;
  }
  masklayer->calcFullTransform(q_xscale,q_yscale,q_zscale,
                               view.xoff,view.yoff,view.width,
                               view.height,view.orient,pos,
                               q_fliph,q_flipv);
  VBMatrix full=masklayer->full;
  VBMatrix mask2screen;
  invert(full,mask2screen);
  // now premultiply cvox to get central view coord
  VBMatrix position(4,1);
  position.set(0,0,cvox.x);
  position.set(1,0,cvox.y);
  position.set(2,0,cvox.z);
  position.set(3,0,1);
  position^=mask2screen;
  // our queue of view pixels to hit starts with this one, visited starts empty
  VBRegion queue,visited;
  queue.add(position(0,0),position(1,0),0);

  int xpos,ypos,zpos;
  VBVoxel vv;
  int32 xmin=position(0,0),xmax=position(0,0),ymin=position(1,0),ymax=position(1,0);
  while (queue.size()) {
    // pop the first voxel
    vv=queue.begin()->second;
    queue.remove(vv.x,vv.y,vv.z);
    visited.add(vv.x,vv.y,0);
    // see if it maps onto maskcoord
    position.set(0,0,vv.x);
    position.set(1,0,vv.y);
    position.set(2,0,0);
    position.set(3,0,1);
    position^=full;    // premultiply to get layer coordinate
    xpos=(int)position(0,0); ypos=(int)position(1,0); zpos=(int)position(2,0);
    // if this voxel didn't map onto the mask voxel, don't pursue it
    if (!(maskreg.contains(xpos,ypos,zpos))) continue;
    // right now we don't do the actual coloring of vv.x,vv.y -- we
    // just keep track of the box and do a restricted unirender later
    if (vv.x<xmin) xmin=vv.x;
    if (vv.x>xmax) xmax=vv.x;
    if (vv.y<ymin) ymin=vv.y;
    if (vv.y>ymax) ymax=vv.y;
    for (int xx=vv.x-1; xx<=vv.x+1; xx++) {
      if (xx<view.xoff || xx>view.xoff+view.width-1) continue;
      for (int yy=vv.y-1; yy<=vv.y+1; yy++) {
        if (yy<view.yoff || yy>view.yoff+view.height-1) continue;
        if (visited.contains(xx,yy,0)) continue;
        // no choice but to...
        queue.add(xx,yy,0,0.0);
      }
    }
  }
  vbrect rr(xmin,ymin,xmax-xmin+1,ymax-ymin+1);
  for (VBLayerI l=layers.begin(); l!=layers.end(); l++) {
    if (l==layers.begin() || (l->q_visible && l->alpha > 0))
      UniRenderLayer(view,l,rr);
  }
  //drawarea->update();
  return 0;
}

