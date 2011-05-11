
// vblock.cpp
// standalone range locking program for IDL
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
// <kimberg@mail.med.upenn.edu>.

using namespace std;

#include"voxbo.h"

void
lockfile(FILE *fp,short locktype,int pos,int len)
{
  struct flock numlock;
  numlock.l_type = locktype;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp),F_SETLKW,&numlock);
}

void
unlockfile(FILE *fp,int pos,int len)
{
  fflush(fp);
  struct flock numlock;
  numlock.l_type = F_UNLCK;
  numlock.l_start = pos;
  numlock.l_len = len;
  numlock.l_whence = SEEK_SET;
  fcntl(fileno(fp),F_SETLKW,&numlock);
}

int
main(int argc,char *argv[])
{
  FILE *fp;
  int pos,len;

  setbuf(stdout,NULL);
  setbuf(stdin,NULL);

  if (argc != 4) {
    printf("vblock: args should be fname, start, length of segment to write lock\n");
    exit(1);
  }
  fp = fopen(argv[1],"r+");
  if (!fp) {
    printf("vblock: couldn't open the file in question\n");
    exit(1);
  }
  pos = strtol(argv[2],NULL,10);
  len = strtol(argv[3],NULL,10);
  lockfile(fp,F_WRLCK,pos,len);
  printf("1");
  getchar();
  unlockfile(fp,pos,len);
  fclose(fp);
  printf("2");
  exit(0);
}
