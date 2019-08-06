
// vbview_layers.cpp
// layer-related code
// Copyright (c) 1998-2011 by The VoxBo Development Team

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
int32 scaledvalue(float val, float low, float high, float factor);
VBMatrix aff_pitch(float degrees);
VBMatrix aff_roll(float degrees);
VBMatrix aff_yaw(float degrees);

VBLayer::VBLayer() {
  alpha = 100;
  q_brightness = 50;
  q_contrast = 50;
  q_thresh = 1.0;
  q_high = 0.0;
  q_twotailed = 1;
  q_flip = 0;
  q_clustersize = 0;
  q_factor = 255.0;
  q_visible = 1;
  q_ns = 0;
  type = vb_struct;
  q_dirty = 0;
  transform.resize(4, 4);
  transform.ident();
  full.resize(4, 4);
  full.ident();
  q_negcolor1 = QColor(0, 0, 200);
  q_negcolor2 = QColor(100, 100, 255);
  q_poscolor1 = QColor(255, 0, 0);
  q_poscolor2 = QColor(255, 255, 0);
  q_nscolor1 = QColor(0, 0, 200);
  q_nscolor2 = QColor(30, 255, 30);
}

QString VBLayer::tooltipinfo() {
  string fstring = "%s\n%dx%dx%d voxels\n%gx%gx%g mm";
  if (mask) fstring += "\n(currently masked)";
  format ff(fstring);
  QString tmp = (ff % xfilename(filename) % cube.dimx % cube.dimy % cube.dimz %
                 cube.voxsize[0] % cube.voxsize[1] % cube.voxsize[2])
                    .str()
                    .c_str();
  return tmp;
}

// setAlignment() sets the sampling transformation for this layer
// relative to the base layer.  when completed, the matrix transform
// will convert base layer coordinates to this layer's coordinates.

void VBLayer::setAlignment(VBLayerI base) {
  VBImage *im;
  if (tes)
    im = &tes;
  else if (cube)
    im = &cube;
  else
    return;
  // special case of matching dimensions, we ignore everything else
  if (base->dimx == im->dimx && base->dimy == im->dimy &&
      base->dimz == im->dimz) {
    transform = base->transform;
    return;
  }
  // if we have meaningful voxel sizes, use them and trust the origin
  // FIXME
  if (0 && base->cube.voxsize[0] > FLT_MIN && cube.voxsize[0] > FLT_MIN) {
    transform.ident();
    transform.set(0, 0, base->cube.voxsize[0] / im->voxsize[0]);
    transform.set(1, 1, base->cube.voxsize[1] / im->voxsize[1]);
    transform.set(2, 2, base->cube.voxsize[2] / im->voxsize[2]);
    //     transform.set(0,3,cube.origin[0]-((base->cube.origin[0]+base->x1)*xinterval));
    //     transform.set(1,3,cube.origin[1]-((base->cube.origin[1]+base->y1)*yinterval));
    //     transform.set(2,3,cube.origin[2]-((base->cube.origin[2]+base->z1)*zinterval));
    return;
  }
  // as a last resort, go by dimensions
  transform.ident();
  transform.set(0, 0, (float)im->dimx / (float)base->dimx);
  transform.set(1, 1, (float)im->dimy / (float)base->dimy);
  transform.set(2, 2, (float)im->dimz / (float)base->dimz);
}

void VBLayer::render() {
  switch (type) {
    case vb_struct:
      setScale();
      renderStruct();
      break;
    case vb_mask:
      renderMask();
      break;
    case vb_stat:
      renderStat();
      break;
    case vb_corr:
      renderCorr();
      break;
    case vb_glm:
      renderStat();
      // case vb_aux:
      //   break;
  }
  if (mask && mask.dimsequal(cube)) {
    for (int i = 0; i < mask.dimx * mask.dimy * mask.dimz; i++)
      if (!mask.testValue(i)) rendercube.setValue(i, 0);
  }
}

// FIXME renderStruct() is slow, and has to be called every time we
// adjust the contrast or brightness, not to mention initial load

void VBLayer::renderStruct() {
  if (!rendercube.data)
    rendercube.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_long);
  rendercube.origin[0] = cube.origin[0];
  rendercube.origin[1] = cube.origin[1];
  rendercube.origin[2] = cube.origin[2];
  int32 val;
  int32 tmp;
  for (int i = 0; i < cube.dimx * cube.dimy * cube.dimz; i++) {
    tmp = scaledvalue(cube.getValue<float>(i), q_thresh, q_high, q_factor);
    val = tmp;
    val |= tmp << 8;
    val |= tmp << 16;
    val |= 100 << 25;
    rendercube.setValue(i, val);
  }
}

// unnamed float below used to be high.  now that's rolled into
// factor.

inline int32 scaledvalue(float val, float low, float, float factor) {
  int32 bval = lround((val - low) * factor);
  if (bval > 255) bval = 255;
  if (bval < 0) bval = 0;
  return bval;
}

void VBLayer::renderMask() {
  if (!rendercube.data)
    rendercube.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_long);
  else
    rendercube.zero();
  int32 val;
  int32 tmp;
  for (int i = 0; i < cube.dimx * cube.dimy * cube.dimz; i++) {
    tmp = cube.getValue<int32>(i);
    if (!tmp) continue;
    VBMaskSpec ms = cube.maskspecs[tmp];
    val = ms.b;
    val |= ms.g << 8;
    val |= ms.r << 16;
    val |= 1 << 24;
    rendercube.setValue(i, val);
  }
}

void VBLayer::renderStat() {
  if (!rendercube.data)
    rendercube.SetVolume(cube.dimx, cube.dimy, cube.dimz, vb_long);
  // FIXME dangerous reliance on cube internal structure
  int32 *ptr = (int32 *)rendercube.data;
  // bool f_all=fabs(q_thresh)<FLT_MIN;
  float val, aval;
  // if we're using a cluster size criterion, find the clusters now
  if (q_clustersize > 1) {
    rendercube.zero();
    vector<VBRegion> regions;
    if (q_twotailed)
      regions = findregions(cube, vb_agt, q_thresh);
    else if (q_flip)
      regions = findregions(cube, vb_lt, 0 - q_thresh);
    else
      regions = findregions(cube, vb_gt, q_thresh);
    vbforeach(VBRegion & rr, regions) {
      if (rr.size() < q_clustersize) continue;
      for (VI myvox = rr.begin(); myvox != rr.end(); myvox++) {
        rendercube.setValue(myvox->second.x, myvox->second.y, myvox->second.z,
                            1);
      }
    }
  }
  for (int i = 0; i < cube.dimx * cube.dimy * cube.dimz; i++) {
    // first set the render value
    if (q_flip)
      val = 0 - cube.getValue<float>(i);
    else
      val = cube.getValue<float>(i);
    // one-tailed display and value < 0 is easy, just mark not to
    // render and don't even bother calculating a value;
    if (!q_twotailed && val < -FLT_MIN) {
      ptr[i] = 0;
      continue;
    }
    // if we're doing clusters and this isn't in one, ignore it
    if (q_clustersize > 1 && rendercube.getValue<int32>(i) == 0) continue;
    aval = fabs(val);
    // if not > thresh and we're not doing ns voxels, continue
    if (!q_ns && !(aval - q_thresh > FLT_MIN)) {
      ptr[i] = 0;
      continue;
    }
    // calculate a value
    ptr[i] = (int32)overlayvalue(val, aval);
  }
}

uint32 VBLayer::overlayvalue(double val, double aval) {
  if (aval < FLT_MIN) return 0;
  // supra-threshold values
  if (aval - q_thresh > FLT_MIN) {
    double pct = (aval - q_thresh) / (q_high - q_thresh);
    int rr, gg, bb;
    if (pct > 1.0) pct = 1.0;
    if (val >
        (0 - FLT_MIN)) {  // zeros (or neg values < epsilon) are always positive
      rr = q_poscolor1.red();
      rr += (int)(pct * (q_poscolor2.red() - rr));
      gg = q_poscolor1.green();
      gg += (int)(pct * (q_poscolor2.green() - gg));
      bb = q_poscolor1.blue();
      bb += (int)(pct * (q_poscolor2.blue() - bb));
    } else {
      rr = q_negcolor1.red();
      rr += (int)(pct * (q_negcolor2.red() - rr));
      gg = q_negcolor1.green();
      gg += (int)(pct * (q_negcolor2.green() - gg));
      bb = q_negcolor1.blue();
      bb += (int)(pct * (q_negcolor2.blue() - bb));
    }
    uint32 ret = rr << 16;
    ret |= gg << 8;
    ret |= bb;
    ret |= 1 << 24;
    return ret;
  }
  // sub-threshold but thresh is basically zero, we render in lowest poscolor
  else if (q_thresh < FLT_MIN) {
    uint32 ret = q_poscolor1.red() << 16;
    ret |= q_poscolor1.green() << 8;
    ret |= q_poscolor1.blue();
    ret |= 1 << 24;
    return ret;
  }
  // sub-threshold values
  else {
    double pct = aval / q_thresh;
    int rr, gg, bb;
    if (pct > 1.0) pct = 1.0;

    rr = q_nscolor1.red();
    rr += (int)(pct * (q_nscolor2.red() - rr));
    gg = q_nscolor1.green();
    gg += (int)(pct * (q_nscolor2.green() - gg));
    bb = q_nscolor1.blue();
    bb += (int)(pct * (q_nscolor2.blue() - bb));

    uint32 ret = rr << 16;
    ret |= gg << 8;
    ret |= bb;
    ret |= 1 << 24;
    return ret;
  }
}

void VBLayer::renderCorr() { renderStat(); }

void VBLayer::setScale() {
  double center, width;
  vector<double> vals;
  double val;
  int xint = cube.dimx / 20;
  int yint = cube.dimy / 20;
  int zint = cube.dimz / 40;
  if (xint < 1) xint = 1;
  if (yint < 1) yint = 1;
  if (zint < 1) zint = 1;

  int nozeros = 1;
  for (int i = 0; i < cube.dimx; i += xint) {
    for (int j = 0; j < cube.dimy; j += yint) {
      for (int k = 0; k < cube.dimz; k += zint) {
        val = cube.GetValue(i, j, k);
        if (!isfinite(val)) continue;
        if (nozeros) {
          if (val < 0.0)
            nozeros = 0;
          else if (val == 0.0)
            continue;
        }
        vals.push_back(val);
      }
    }
  }
  if (vals.size() == 0) {
    q_thresh = 0.0;
    q_high = 1.0;
    q_factor = 255.0;
    return;
  }
  sort(vals.begin(), vals.end());
  // cut off the tails
  q_thresh = vals[vals.size() / 50];
  q_high = vals[vals.size() * 49 / 50];
  if (q_thresh == q_high) {
    q_thresh = 0;
    q_high = vals[vals.size() - 1];
    q_factor = 255.0 / q_high;
    return;
  }
  // set window center and width
  width = q_high - q_thresh;
  center = q_thresh + width / 2.0;
  double increment = pow(8.0, 0.02);
  width = width * pow(increment, 50.0 - q_contrast);
  double lowestcenter = q_thresh - width / 2.0;
  double highestcenter = q_high + width / 2.0;
  center = lowestcenter +
           (highestcenter - lowestcenter) * ((100.0 - q_brightness) / 100.0);
  q_thresh = center - width / 2.0;
  q_high = center + width / 2.0;
  if (q_thresh >= q_high) q_high = q_thresh + 1.0;
  q_factor = 255.0 / (q_high - q_thresh);
  return;
}

// calcFullTransform() first calculates a transformation from the
//   drawing area on the screen to the base layer.  then it
//   premultiplies that transform by the already-calculated one that
//   transforms the base layer coordinates into our layer's
//   coordinates.  the end result is a transform from the screen
//   coordinate to our layer volume's coordinates.  this takes into
//   account the current scaling (magnification and isotropic scaling
//   flag), the current rotation, the position of the view in the
//   window, everything.

VBMatrix VBLayer::calcFullTransform(float xscale, float yscale, float zscale,
                                    int xoff, int yoff, int vwidth, int vheight,
                                    int orient, float position, bool q_fliph,
                                    bool q_flipv) {
  // we already have a base -> current layer transform
  // we just need to build a screen to base layer transform

  // we take the screen coordinate and first offset it so that zero is zero
  VBMatrix stob(4, 4), tmp(4, 4);
  // offset it
  stob.ident();
  stob.set(0, 3, 0 - xoff);
  stob.set(1, 3, 0 - yoff);

  // up is down
  if (!q_flipv) {
    tmp.ident();
    tmp.set(1, 1, -1);
    tmp.set(1, 3, vheight - 1);
    stob ^= tmp;
  }
  if (q_fliph) {
    tmp.ident();
    tmp.set(0, 0, -1);
    tmp.set(0, 3, vwidth - 1);
    stob ^= tmp;
  }
  if (orient == MyView::vb_xy) {
    // scale it appropriately
    tmp.ident();
    tmp.set(0, 0, 1.0 / xscale);
    tmp.set(1, 1, 1.0 / yscale);
    stob ^= tmp;
    // find the right slice
    tmp.ident();
    tmp.set(2, 3, position);
    stob ^= tmp;
  } else if (orient == MyView::vb_xz) {  // coronal
    // scale it appropriately
    tmp.ident();
    tmp.set(0, 0, 1.0 / xscale);
    tmp.set(1, 1, 1.0 / zscale);
    stob ^= tmp;
    // rotate it
    tmp.zero();
    tmp.set(0, 0, 1);
    tmp.set(1, 2, 1);
    tmp.set(2, 1, 1);
    tmp.set(3, 3, 1);
    stob ^= tmp;
    // find the right slice
    tmp.ident();
    tmp.set(1, 3, position);
    stob ^= tmp;
  } else if (orient == MyView::vb_yz) {  // sagittal
    // scale it appropriately
    tmp.ident();
    tmp.set(0, 0, 1.0 / yscale);
    tmp.set(1, 1, 1.0 / zscale);
    stob ^= tmp;
    // rotate it
    tmp.zero();
    tmp.set(0, 2, 1);
    tmp.set(1, 0, 1);
    tmp.set(2, 1, 1);
    tmp.set(3, 3, 1);
    stob ^= tmp;
    // find the right slice
    tmp.ident();
    tmp.set(0, 3, position);
    stob ^= tmp;
  }
  stob ^= transform;
  full = stob;
  return stob;
}

VBMatrix aff_pitch(float degrees) {
  VBMatrix tmp(4, 4);
  float radians = (degrees / 180.0) * PI;
  tmp.ident();
  tmp.set(1, 1, cos(radians));
  tmp.set(2, 1, sin(radians));
  tmp.set(1, 2, 0.0 - sin(radians));
  tmp.set(2, 2, cos(radians));
  return tmp;
}

VBMatrix aff_roll(float degrees) {
  VBMatrix tmp(4, 4);
  float radians = (degrees / 180.0) * PI;
  tmp.ident();
  tmp.set(0, 0, cos(radians));
  tmp.set(2, 0, 0.0 - sin(radians));
  tmp.set(0, 2, sin(radians));
  tmp.set(2, 2, cos(radians));
  return tmp;
}

VBMatrix aff_yaw(float degrees) {
  VBMatrix tmp(4, 4);
  float radians = (degrees / 180.0) * PI;
  tmp.ident();
  tmp.set(0, 0, cos(radians));
  tmp.set(1, 0, sin(radians));
  tmp.set(0, 1, 0.0 - sin(radians));
  tmp.set(1, 1, cos(radians));
  return tmp;
}
