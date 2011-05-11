
// voxbo.h
// VoxBo headers
// Copyright (c) 1998,1999 by The VoxBo Development Team

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
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "vbutil.h"
#include "vbprefs.h"
#include "vbjobspec.h"

#define MAXFILENAME 128
#define MAXHOSTS 64                   // maximum number of eligible hosts
#define MAXFILES 512                  // maximum number of files in a job
#define MAXTAGLEN 32                  // maximum length of scan tag in chars

#ifndef STRINGLEN
#define STRINGLEN 512                 // used widely for lots of stuff
#endif

