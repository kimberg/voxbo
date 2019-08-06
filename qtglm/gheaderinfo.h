
// gheaderinfo.h
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
// original version written by Dongbo Hu

using namespace std;

#include <deque>
#include <string>
#include <vector>
#include "glmutil.h"

class gHeaderInfo {
 public:
  gHeaderInfo();
  gHeaderInfo(VBMatrix);
  ~gHeaderInfo();
  bool chkInfo(bool);

 private:
  void init();
  void getInfo(VBMatrix);
  bool chkCondfxn(bool);

 public:
  bool read(const std::string& inputName, bool);

  unsigned rowNum, colNum;
  int TR, sampling;
  bool condStat;
  std::string condfxn;
  std::deque<std::string> nameList, typeList;
  tokenlist condKey;
  VB_Vector* condVector;
  VBMatrix gMatrix;
};

bool chkFileStat(const char* inputName, bool qtFlag);
