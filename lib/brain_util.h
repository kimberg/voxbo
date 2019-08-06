
// br_util.h
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

/* This header file includes some generic functions to read/write records
 * in the following three brain region database tables:
 * region_name.db
 * synonym.db
 * region_relation.db
 */

#ifndef BRAIN_UTIL_H
#define BRAIN_UTIL_H

#include <vector>
#include "brain_tab.h"
#include "mydb.h"

int chkRegionName(string, string, string, string);
int chkSynonymStr(string, string, string, string);

int getAllRegions(string, string, vector<string> &, vector<string> &);
int getRegions(string, string, string, vector<string> &);

int getAllSynonyms(string, string, vector<string> &, vector<string> &);
int getSynonyms(string, string, string, vector<string> &);

int getRegionRec(string, string, string, string, regionRec &);
long getRegionID(string, string, string, string);
int getRegionName(string, string, long, string &, string &);

int getPrimary(string, string, string, string, string &);
int getParentChild(string, string, long, long *, vector<long> &);
int getParent(string, string, long, long *);
int getChild(string, string, long, vector<long> &);
int getSynonym(string, string, string, string, vector<string> &);

long getRelationID(string, string, long, long, string);
int getRel_ui(string, string, long, vector<long> &, vector<string> &);
int getRel_noChild(string, string, long, long, long, vector<string> &);
int getRel_noChildPart(string, string, long, long, vector<string> &);
int getRel_all(string, string, long, long, vector<string> &);
string trRel_db2ui(string, bool);

// Function written specially for command line search
int findRegionNames(string, string, string, bool, vector<regionRec> &);
int findRegionNames(string, string, string, bool, string, bool,
                    vector<regionRec> &);
int findRegionNS(string, string, string, bool, vector<regionRec> &);
int findSynonyms(string, string, string, bool, vector<synonymRec> &);
int findSynonyms(string, string, string, bool, string, bool,
                 vector<synonymRec> &);
int findSynmNS(string, string, string, bool, vector<synonymRec> &);
string toLowerCase(string);

#endif
