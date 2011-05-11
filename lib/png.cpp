
// png.cpp
// produce png files from mostly cubes (nonmember functions)
// Copyright (c) 1998-2002 by The VoxBo Development Team

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

using namespace std;

#include <stdio.h>
#include "vbutil.h"
#include "vbio.h"
#include <png.h>

// for backwards compatibility with older libpngs
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

unsigned char scaledvalue(double val,double low, double high);
void CalcMaxMin(const Cube &mycube,double &mincube,double &maxcube);

// WritePNG() produces a B/W PNG

int
WritePNG(const Cube &cube,int slice,const string &filename)
{
   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;
   png_uint_32 width,height;
   
   width=cube.dimx;
   height=cube.dimy;

   /* open the file */
   fp = fopen(filename.c_str(),"wb");
   if (fp == NULL)
      return (101);

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return (102);
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return (103);
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* If we get here, we had a problem reading the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (104);
   }

   png_init_io(png_ptr, fp);

   png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);

   png_uint_32 k;
   png_byte image[height][width];
   png_bytep row_pointers[height];
   double minval,maxval;
   CalcMaxMin(cube,minval,maxval);
   
   for (unsigned int i=0; i<width; i++) {
     for (unsigned int j=0; j<height; j++) {
       image[j][i]=scaledvalue(cube.GetValue(i,height-j-1,slice),minval,maxval);
     }
   }
   for (k = 0; k < height; k++)
     row_pointers[k] = image[k];

   png_write_image(png_ptr, row_pointers);
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);
   fclose(fp);
   return (0);
}

void
CalcMaxMin(const Cube &mycube,double &mincube,double &maxcube)
{
  int i,j,k;
  double val;
  
  maxcube = mincube = mycube.GetValue(0,0,0);
  for(i=0; i<mycube.dimx; i++) {
    for(j=0; j<mycube.dimy; j++) {
      for(k=0; k<mycube.dimz; k++) {
	val = mycube.GetValue(i,j,k);
	if (val > maxcube) maxcube = val;
	if (val < mincube) mincube = val;
      }
    }
  }
  val=(maxcube-mincube)/2.0;
  // mincube -= val;
  // maxcube += val;
  maxcube-=val;
}

unsigned char
scaledvalue(double val,double low, double high)
{
  double bval = (((val - low) * 255.0 / (high - low)));
  if (bval > 255) bval=255;
  if (bval < 0) bval=0;
  return (short)bval;
}
