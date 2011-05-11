
// searchBrain.h
// Copyright (c) 2010 by The VoxBo Development Team

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

/* This interface allows user to browse/search brain region database. */

#include "brain_util.h"
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>

int parseArg(int argc, char *argv[]);
bool setDbDir();
bool isReadableReg(string);
void help_msg();
void err_msg();

void searchRegionName(string, bool, vector <regionRec> &);
void searchRegionName(string, bool, string, vector <regionRec> &);
void searchRegionNS(string, vector <regionRec> &);
void searchSynonym(string, bool, vector <synonymRec> &);
void searchSynonym(string, bool, string, vector <synonymRec> &);
void searchSynmNS(string, vector <synonymRec> &);

void printSummary(unsigned);
void showMatches(vector <regionRec>);
void showMatches(vector <synonymRec>);
void showRegion(regionRec);
int setParentChild(long, string &, vector <string> &);
int setRelationships(long, vector <string> &);

//bool validateString(const char *, const char *);

