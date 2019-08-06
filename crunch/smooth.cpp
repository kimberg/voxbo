
// smooth.cpp
// VoxBo smoothing module
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

// The guts of the underlying code were based on SPM source code
// written in MATLAB by John Ashburner.  See the SPM web site at:
// (http://www.fil.ion.bpmf.ac.uk/spm/)

using namespace std;

#include "vbcrunch.h"

class Smoothing {
 private:
  string infile, outfile;
  int s0, s1, s2;

 public:
  Smoothing(int argc, char **argv);
  ~Smoothing();
  int Crunch();
};

void vbsmooth_help();

int main(int argc, char *argv[]) {
  tzset();         // make sure all times are timezone corrected
  if (argc < 2) {  // not enough args, display autodocs
    realign_help();
    exit(0);
  }

  Smoothing *r = new Smoothing(argc, argv);  // init realign object
  if (!r) {
    printf("smoothing error: couldn't allocate a tiny structure\n");
    exit(5);
  }
  int error = r->Crunch();  // do the crunching
  delete r;                 // clean up
  exit(error);
}

Smoothing::Smoothing(int argc, char *argv[]) {
  if (argc != 4 && argc != 6) {
    smooth_help();
    exit(5);
  }
  infile = argv[1];
  outfile = argv[2];
  s0 = s1 = s2 = strtod(argv[3], NULL);
  if (argc == 6) {
    s1 = strtod(argv[4], NULL);
    s2 = strtod(argv[5], NULL);
  }
}

int Smoothing::Crunch() {}

Smoothing::~Smoothing() {}

void smooth_help() {
  printf("smooth - another fine component of VoxBo!\n\n");
  printf("usage: smooth <infile> <outfile> <s1> [s2 s3]\n");
}
