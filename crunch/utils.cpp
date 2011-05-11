
// utils.cpp
// dumb utility functions for various parts of the realign port
// Copyright (c) 1998-2003 by The VoxBo Development Team

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

int
suffix_hdr(char *fname)
{
  int i;

  i=strlen(fname);

  if (i < 5)
    strcat(fname,".hdr");
  else if (fname[i-4] != '.')
    strcat(fname,".hdr");
  else
    strcpy(fname+i-3,"hdr");
  return 1;
}

int
suffix_img(char *fname)
{
  int i;
  i=strlen(fname);

  if (i < 5)
    strcat(fname,".img");
  else if (fname[i-4] != '.')
    strcat(fname,".img");
  else
    strcpy(fname+i-3,"img");
  return 1;
}

int
suffix_mat(char *fname)
{
  int i;
  i=strlen(fname);

  if (i < 5)
    strcat(fname,".mat");
  else if (fname[i-4] != '.')
    strcat(fname,".mat");
  else
    strcpy(fname+i-3,"mat");
  return 1;
}

int get_datasize(int type)
{
  if (type == vb_byte) return(sizeof(unsigned char));
  else if (type == vb_short) return(sizeof(short));
  else if (type == vb_long) return(sizeof(int));
  else if (type == vb_float) return(sizeof(float));
  else if (type == vb_double) return(sizeof(double));
  else return(0);
}

double
sum(const RowVector &v)
{
  double tot=0.0;
  for(int i=0; i<v.length(); i++) {
    tot += v(i);
  }
  return tot;
}

// parsename()
// seems to work
int
parsename(char *fname,char *dirname,char *outfilename)
{
  int i,ind;
  char fullname[STRINGLEN];
  
  ind=-1;
  strncpy(fullname,fname,STRINGLEN-1);
  for (i=0; i<(int)strlen(fullname); i++) {
    if (fullname[i] == '/')
      ind=i;
  }
  
  if (ind > (int)(strlen(fullname)-2)) {
    cerr << "error: parsename(): no filename" << endl;
    return FALSE;
  }
  if (ind>-1) {
    fullname[ind]='\0';
    strcpy(dirname,fullname);
    strcat(dirname,"/");
    strcpy(outfilename,fullname+ind+1);
  }
  else {
    strcpy(outfilename,fullname);
    dirname[0]='\0';
  }
  return TRUE;
}

void
print(const Matrix &mat,int size)
{
  print("Anonymous matrix",mat,size);
}

void
print(const RowVector &vec,int size)
{
  print("Anonymous rowvector",vec,size);
}

void
print(const char *s,const Matrix &mat,int size)
{
  int i,j;
  
  j=0;
  cerr << endl << s << "(" << mat.rows() << "x" << mat.cols() << "):" << endl;
  for (i=0; i<mat.rows(); i++) {
    if (i<size || (mat.rows() -i <size)) {
      print((char *)NULL,mat.row(i),size);
    }
    else if (j++==0)
      cerr << "..." << endl;
  }
}

void
print(const char *s,const RowVector &vec,int size)
{
  int i,j;
  
  j=0;
  if (s != NULL)
    cerr << endl << s << " (" << vec.length() << "):" << endl;
  for(i=0; i<vec.length(); i++) {
    if (i<size || (vec.length() - i < size))
      cerr << " " << vec(i);
    else if (j++==0)
      cerr << "...  ";
  }
  cerr << endl;
}

void
print(const char *s,const ColumnVector &vec,int size)
{
  int i,j;
  
  j=0;
  cerr << endl << s << ":" << endl;
  for (i=0; i<vec.length(); i++) {
    if (i<size || (vec.length() -i < size)) {
      cerr << " " << vec(i);
    }
    else if (j++==0)
      cerr << "..." << endl;
  }
}
