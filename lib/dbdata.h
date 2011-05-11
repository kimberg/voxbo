
// dbdata.h
// header information for i/o functions
/* Simple class designed to hold env, DBs and global maps.
 * When it is used by remote client, env and DBs are not opened.
 * Maps are set up by data transferred from server. */
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

#ifndef DBDATA_H
#define DBDATA_H

#include "mydb.h"
#include "mydefs.h"
#include "brain_tab.h"
#include <string>
#include <map>

class DBdata
{
 public:
  int open(string dir="");
  void setDbNames(string dirname="");
  int close();
  int closeTables();
  int loadRegionData();
  int readTypes(string fname);
  int readViews(string fname);
  int readScorenames(string fname);
  // int32 getScoreNames(DbTxn*); 
  void add_scorename(const DBscorename&);
  void print_types();

  int initDB(string inputDir, string admin_passwd="admin", uint32 id_start=100);

  myEnv env;
  mydb sysDB, userDB, userGrpDB, userRelationDB, permDB;
  mydb scoreNameDB, sessionDB, scoreValueDB, patientScoreDB;
  mydb contactDB, patientDB, patientListDB;
  //mydb patientGrpDB, patientGrpMemDB;
  mydb studyDB;
  mydb viewDB, viewEntryDB; // not used right now
  mydb regionNameDB, namespaceDB, synonymDB, regionRelationDB;
  // some dump functions for debugging mostly, only work for local dbs
  void dumpscorevalues();
  void dumpsys();
  void dumppatients();

  string dirname;
  string typeFilename;
  string viewFilename;
  string scoreNameFilename;
  string sysDbName, userDbName, permissionDbName;
  string scoreNameDbName, sessionDbName, scoreValueDbName, patientScoreDbName;
  string regionDbName, synonymDbName, regionRelationDbName, namespaceDbName;
  string patientDbName, patientListDbName;
  string errMsg;
  
  map<string,DBtype> typemap;                // access types by name
  map<string,DBscorename> scorenames;        // all the possible score names
  // map<string,string> testmap;             // map test (or node) name onto scorename id
  multimap<string,string> scorenamechildren; // map each scorename to its children
  map<string,DBviewspec> viewspecs;
  map<int32,regionRec> regionNameMap;
  map<int32,synonymRec> synonymMap;
  map<int32,regionRelationRec> regionRelationMap;
};

#endif
