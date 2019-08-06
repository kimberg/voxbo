
// vbpreplib.h
// class definitions for classes used in vbprep.cpp
// Copyright (c) 1998-2002 by The VoxBo Development Team

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
// original version written by Tom King based on code by Daniel Y. Kimberg

#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include "vbio.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbutil.h"

class VBPJob {
 public:
  VBPJob();
  string jobtype;
  tokenlist args;
  int lastjobnum;
  int runonce;
  int runparallel;
  void clear();
};

class VBVariable {
 public:
  VBVariable(){};
  string name;
  string type;
  string defaultValue;
  string currentValue;
  string description;
  vector<string> value;
};

class VBPFile {
 public:
  VBPFile();
  tokenlist filename;
  string filetype;
  int dimensions;
  int lastjob;
};

class VBPrep {
 public:
  VBPrep();
  VBSequence seq;
  int BuildJobs(VBPrefs &vbp);
  vector<VBPJob> joblist;
  void ClearData();
  void ClearJobs();
  VBJobSpec js;
  string priority;
  string sequenceName;
  string directory;
  string email;
  tokenlist globals;
  vector<VBPFile> filelist;
};

class VBPData {
 public:
  VBPData(const VBPrefs &vbp);
  VBPrep study;
  vector<VBVariable> varlist;
  vector<VBPrep> data;
  int StoreDataFromFile(string fname, string selectedSequence);
  string GetDocumentation(string fname);
  int Clear();

 private:
  int ParseFile(string fname, string selectedSequence);
  string ScriptName(string name);
  VBPrefs vbp;
};
