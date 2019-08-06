
// vbmunge.cpp
// generalized munging
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

#include <stdio.h>
#include <string.h>
#include "vbio.h"
#include "vbmunge.hlp.h"
#include "vbutil.h"

class VBMunge {
 private:
  enum modetype {
    m_flipx,
    m_flipy,
    m_flipz,
    m_leftify,
    m_rightify,
    m_addcube,
    m_striporigin
  };
  vector<modetype> modes;
  tokenlist args;
  string extrafile;
  vector<string> filelist;
  int munge3d(const string &fname);
  int munge4d(const string &fname);
  int munge(Cube *cub, modetype mode);
  void help();

 public:
  int Go(int argc, char **argv);
};

void leftify(Cube &cub);
void rightify(Cube &cub);
void addcube(Cube &cub, string infile);

int main(int argc, char *argv[]) {
  VBMunge *vbm = new VBMunge();
  int err = vbm->Go(argc - 1, argv + 1);
  exit(err);
}

int VBMunge::Go(int argc, char **argv) {
  args.Transfer(argc, argv);
  if (args.size() == 0) {
    help();
    return (0);
  }
  if (args.size() < 2) {
    help();
    return (100);
  }
  args.Transfer(argc, argv);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-x")
      modes.push_back(m_flipx);
    else if (args[i] == "-y")
      modes.push_back(m_flipy);
    else if (args[i] == "-z")
      modes.push_back(m_flipz);
    else if (args[i] == "-l")
      modes.push_back(m_leftify);
    else if (args[i] == "-r")
      modes.push_back(m_rightify);
    else if (args[i] == "-s")
      modes.push_back(m_striporigin);
    else if (args[i] == "-a" && i + 1 < args.size()) {
      modes.push_back(m_addcube);
      extrafile = args[i + 1];
      i++;
    } else
      filelist.push_back(args[i]);
  }
  for (int i = 0; i < (int)filelist.size(); i++) {
    cout << "Processing " << filelist[i] << "..." << flush;
    vector<VBFF> typelist = EligibleFileTypes(filelist[i]);
    switch (typelist[0].getDimensions()) {
      case 3:
        munge3d(filelist[i]);
        break;
      case 4:
        munge4d(filelist[i]);
        break;
      default:
        cout << "bad filetype..." << endl;
        break;
    }
    cout << "done." << endl;
  }
  return 0;
}

int VBMunge::munge4d(const string &fname) {
  Cube *cube;
  Tes *tes = new Tes();
  if (tes->ReadFile(fname)) {
    printf("error: couldn't read 4D file\n");
    return 1;
  }
  if (!tes->data_valid) {
    printf("error: invalid 4D file\n");
    return 1;
  }
  for (int i = 0; i < (int)modes.size(); i++) {
    if (modes[i] == m_striporigin)
      tes->SetOrigin(0, 0, 0);
    else {
      for (int j = 0; j < tes->dimt; j++) {
        cube = new Cube((*tes)[j]);
        munge(cube, modes[i]);
        tes->SetCube(j, *cube);
        delete cube;
      }
    }
  }
  int err = tes->WriteFile();
  delete tes;
  return (err);
}

int VBMunge::munge3d(const string &fname) {
  Cube *cube = new Cube;
  if (cube->ReadFile(fname)) {
    printf("error: couldn't read 3D file\n");
    return 2;
  }
  if (!cube->data_valid) {
    printf("error: invalid 3D file\n");
    return 2;
  }
  for (int i = 0; i < (int)modes.size(); i++) {
    if (modes[i] == m_striporigin)
      cube->SetOrigin(0, 0, 0);
    else
      munge(cube, modes[i]);
  }
  return (cube->WriteFile());
}

int VBMunge::munge(Cube *cub, VBMunge::modetype mode) {
  if (mode == m_flipx)
    cub->flipx();
  else if (mode == m_flipy)
    cub->flipy();
  else if (mode == m_flipz)
    cub->flipz();
  else if (mode == m_leftify)
    leftify(*cub);
  else if (mode == m_rightify)
    rightify(*cub);
  else if (mode == m_addcube)
    addcube(*cub, extrafile);
  return 0;
}

void leftify(Cube &cub) {
  for (int i = (cub.dimx + 1) / 2; i <= cub.dimx; i++) {
    for (int j = 0; j < cub.dimy; j++) {
      for (int k = 0; k < cub.dimz; k++) {
        cub.SetValue(i, j, k, 0.0);
      }
    }
  }
}

void rightify(Cube &cub) {
  for (int i = 0; i <= cub.dimx / 2; i++) {
    for (int j = 0; j < cub.dimy; j++) {
      for (int k = 0; k < cub.dimz; k++) {
        cub.SetValue(i, j, k, 0.0);
      }
    }
  }
}

void addcube(Cube &cub, string infile) {
  Cube *cb = new Cube;
  if (cb->ReadFile(infile.c_str())) {
    printf("couldn't read add image\n");
    return;
  }
  if (!cb->data_valid) {
    printf("couldn't open add image\n");
    return;
  }
  if (cub.dimx != cb->dimx || cub.dimy != cb->dimy || cub.dimz != cb->dimz) {
    printf("dimension mismatch\n");
    return;
  }
  for (int i = 0; i <= cub.dimx; i++) {
    for (int j = 0; j < cub.dimy; j++) {
      for (int k = 0; k < cub.dimz; k++) {
        cub.SetValue(i, j, k, cub.GetValue(i, j, k) + cb->GetValue(i, j, k));
      }
    }
  }
}

void VBMunge::help() { cout << boost::format(myhelp) % vbversion; }
