
// vborient.cpp
// client code for re-orienting a cub or tes file
// Copyright (c) 1998-2008 by The VoxBo Development Team

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
// original version written by Tom King based on code by Daniel Y. Kimberg
// later changes by Dan Kimberg

using namespace std;

#include <stdio.h>
#include <string.h>
#include "vbio.h"
#include "vborient.hlp.h"
#include "vbutil.h"

void vborient_help();

int main(int argc, char *argv[]) {
  int err = 0;
  int interleaved = 0;
  string in, out;
  string inFile, outFile;
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  string oldorient, neworient;
  vector<string> filelist;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-from" and i < args.size() - 1)
      oldorient = args[++i];
    else if (args[i] == "-i")
      interleaved = 1;
    else
      filelist.push_back(args[i]);
  }
  if (filelist.size() != 3) {
    vborient_help();
    exit(101);
  }

  inFile = filelist[0];
  outFile = filelist[1];
  neworient = filelist[2];

  Tes mytes;
  Cube mycube;
  if (mycube.ReadFile(inFile) == 0) {
    Cube outCube;
    outCube = mycube;

    if (oldorient.size() == 0) oldorient = mycube.orient;

    printf("[I] vborient: changing orientation for %s from %s to %s\n",
           inFile.c_str(), oldorient.c_str(), neworient.c_str());

    err = vbOrient(mycube, outCube, oldorient, neworient, interleaved);
    string tmps;
    switch (err) {
      case 0:
        tmps = timedate() + " changed orientation from " + oldorient + " to " +
               neworient;
        outCube.AddHeader(tmps);
        if (interleaved) {
          tmps = timedate() + " de-interleaved";
          outCube.AddHeader(tmps);
        }
        outCube.orient = neworient;
        if (outCube.WriteFile(outFile)) {
          printf("[E] vborient: error writing file %s\n", outFile.c_str());
          exit(201);
        } else {
          printf("[I] vborient: wrote file %s\n", outFile.c_str());
          exit(0);
        }
        break;
      case 1:
        printErrorMsg(VB_ERROR,
                      "vborient: The orientation did not have three characters "
                      "(e.g., RPI or LAS).\n");
        return -1;
      case 2:
        printErrorMsg(VB_ERROR,
                      "vborient: Initial orientation not well formed.\n");
        return -1;
      case 3:
        printErrorMsg(VB_ERROR,
                      "vborient: Desired orientation not well formed.\n");
        return -1;
      default:
        printErrorMsg(
            VB_ERROR,
            "vborient: Unspecified error returned from function vbOrient");
    }
  } else if (mytes.ReadFile(inFile) == 0) {
    Tes outTes;
    outTes = mytes;
    outTes.SetFileName(outFile);
    if (oldorient.size() == 0) oldorient = mytes.orient;

    printf("[I] vborient: changing orientation for %s from %s to %s\n",
           inFile.c_str(), oldorient.c_str(), neworient.c_str());
    err = vbOrientTes(mytes, outTes, oldorient, neworient, interleaved);
    string tmps;
    switch (err) {
      case 0:
        tmps = timedate() + " vborient: changed orientation from " + oldorient +
               " to " + neworient;
        outTes.AddHeader(tmps);
        if (interleaved) {
          tmps = timedate() + " vborient: and de-interleaved";
          outTes.AddHeader(tmps);
        }
        outTes.orient = neworient;
        if (outTes.WriteFile(outFile)) {
          printf("[E] vborient: error writing file %s\n", outFile.c_str());
          exit(201);
        } else {
          printf("[I] vborient: wrote file %s\n", outFile.c_str());
          exit(0);
        }
        break;
      case 1:
        printErrorMsg(VB_ERROR,
                      "vborient: The orientation did not have three characters "
                      "(e.g., RPI or LAS).\n");
        return -1;
      case 2:
        printErrorMsg(VB_ERROR,
                      "vborient: Initial orientation not well formed.\n");
        return -1;
      case 3:
        printErrorMsg(VB_ERROR,
                      "vborient: Desired orientation not well formed.\n");
        return -1;
      case 4:
        printErrorMsg(
            VB_ERROR,
            "vborientTes: Error in calling returnReverseOrientation().\n");
        return -1;
      case 5:
        printErrorMsg(
            VB_ERROR,
            "vborientTes: Error extracting initial cube from tes file.\n");
        return -1;
      case 6:
        printErrorMsg(VB_ERROR,
                      "vborientTes: Error reorienting cube of tes file.\n");
        return -1;
      case 7:
        printErrorMsg(VB_ERROR,
                      "vborientTes: Error in setCube() call on tes file.\n");
        return -1;
      default:
        printErrorMsg(
            VB_ERROR,
            "vborient: Unspecified error returned from function vbOrientTes");
    }
  } else {
    fprintf(stderr,
            "[E] vborient: couldn't read file as either 3D or 4D image data\n");
    exit(111);
  }
  exit(0);
}

void vborient_help() { cout << boost::format(myhelp) % vbversion; }
