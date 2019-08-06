
// vb_render.h
// headers for vb_render.h
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

#include <QImage>
#include <string>
#include <vector>
#include "tokenlist.h"
#include "vbio.h"
#include "vbprefs.h"
#include "vbutil.h"

class VBRenderer {
 public:
  float low;      // threshold for overlays
  float high;     // high value for scaling color overlays
  int twotailed;  // plot negative values or just positive?
  QImage qim;
  Cube *anatomy, *mask, *overlay;
  VBRenderer();
  int SavePNG(string &fname);
  int RenderSlices();
  void RenderSlice(int slice, int xmag, int ymag, int xoffset, int yoffset);
  void RenderOverlay(int slice, int xmag, int ymag, int xoffset, int yoffset);
};

short scaledvalue(double minval, double maxval, double brightness,
                  double contrast, double val);
