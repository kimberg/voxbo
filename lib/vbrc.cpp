
// vbrc.cpp
// simplest possible resource compiler, give or take
// Copyright (c) 2009 by The VoxBo Development Team

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

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "vbrc requires exactly 1 argument\n");
    exit(10);
  }
  printf("static const char %s[]= {\n  ", argv[1]);
  char buf[1024];
  int i, cnt;
  int linecnt = 0;
  while (1) {
    cnt = fread(buf, 1, 1024, stdin);
    if (!cnt) break;
    for (i = 0; i < cnt; i++) {
      if (linecnt > 13) {
        printf("\n  ");
        linecnt = 0;
      }
      printf("0x%x,", (short)buf[i]);
      linecnt++;
    }
  }
  if (linecnt > 13) printf("\n  ");
  printf("0x%x};\n", (short)0);
  exit(0);
}
