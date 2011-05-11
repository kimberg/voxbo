
// endian.cpp
// byteorder-related functions for VoxBo
// Copyright (c) 1998-2001 by The VoxBo Development Team

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

#include "vbutil.h"
#include <netinet/in.h>

void
swapn(unsigned char *uc,int dsize,int len)
{
  if (dsize==2)
    swap((uint16 *)uc,len);
  else if (dsize==4)
    swap((uint32 *)uc,len);
  else if (dsize==8)
    swap((double *)uc,len);
}

void
swap(uint16 *sh,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(sh + i);
    tmp = foo[0];
    foo[0] = foo[1];
    foo[1] = tmp;
  }
}

void
swap(int16 *sh,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(sh + i);
    tmp = foo[0];
    foo[0] = foo[1];
    foo[1] = tmp;
  }
}

void
swap(uint32 *lng,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(lng + i);
    tmp = foo[0];
    foo[0] = foo[3];
    foo[3] = tmp;
    tmp = foo[1];
    foo[1] = foo[2];
    foo[2] = tmp;
  }
}

void
swap(int32 *lng,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(lng + i);
    tmp = foo[0];
    foo[0] = foo[3];
    foo[3] = tmp;
    tmp = foo[1];
    foo[1] = foo[2];
    foo[2] = tmp;
  }
}

void
swap(float *flt,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(flt + i);
    tmp = foo[0];
    foo[0] = foo[3];
    foo[3] = tmp;
    tmp = foo[1];
    foo[1] = foo[2];
    foo[2] = tmp;
  }
}

void
swap(double *dbl,int len)
{
  unsigned char *foo,tmp;
  for (int i=0; i<len; i++) {
    foo = (unsigned char *)(dbl + i);
    tmp = foo[0];
    foo[0] = foo[7];
    foo[7] = tmp;
    tmp = foo[1];
    foo[1] = foo[6];
    foo[6] = tmp;
    tmp = foo[2];
    foo[2] = foo[5];
    foo[5] = tmp;
    tmp = foo[3];
    foo[3] = foo[4];
    foo[4] = tmp;
  }
}

VB_byteorder
my_endian()
{
  if (ntohs(1)==1)
    return ENDIAN_BIG;
  else
    return ENDIAN_LITTLE;
}
