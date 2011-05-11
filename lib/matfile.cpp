
// FIXME this is unused, but could be.  get rid of long and other
// non-specific declarations!


// matfile.cpp
// read matlab mat files?
// Copyright (c) 1998-2005 by The VoxBo Development Team

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

int
ReadMATFileHeader(VBMatrix &m,string fname)
{
  // parse filename (remove bracketed stuff)
  FILE *fp=fopen(fname.c_str(),"r");
  // grab the first 124 bytes, copy the header
  char buf[128],hdr[125];
  if (fread(buf,1,128,fp)!=128) {
    fclose(fp);
    return 101;
  }
  memcpy(hdr,buf,124);
  hdr[124]='\0';
  m.AddHeader(hdr);
  // the next two shorts are the matlab version and the endian indicator
  if (buf[126]=='M')
    m.filebyteorder=ENDIAN_BIG;
  else
    m.filebyteorder=ENDIAN_LITTLE;
  int16 mversion;
  memcpy(&mversion,buf+124,2);
  int swapped=0;
  if (m.filebyteorder!=my_endian())
    swapped=1;
  if (swapped)
    swap(&mversion);
  m.AddHeader((string)"MATLAB version "+strnum(mversion));
  // now build an index of the file by repeatedly grabbing a pair of
  // longs, the second of which is nbytes, and iterating
  uint32 dtype,dlen,skip;
  while(1) {
    if (fread(&dtype,sizeof(long),1,fp)!=1) {
      fclose(fp);
      return 102;
    }
    if (fread(&dlen,sizeof(long),1,fp)!=1) {
      fclose(fp);
      return 103;
    }
    if (swapped) {
      swap(&dtype);
      swap(&dlen);
    }
    // pad to 64-bit (8 byte) boundaries
    skip=dlen;
    if (dlen%8) skip+=8-(dlen%8);
    printf("dtype %ld dlen %ld skip %ld\n",dtype,dlen,skip);
    
    // quick read the element
    unsigned char flags[8];
    if (fread(flags,1,8,fp)!=8) {
      fclose(fp);
      return 104;
    }



  }
  fclose(fp);
  return 0;
}

int
ReadMATFile(string fname,int row1=-1,int rown=-1,int col1=-1,int coln=-1)
{
}

