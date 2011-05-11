
// vbio.h
// environment and db class, wrapper of Berkeley DB's native DbEnv and Db classes
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

#ifndef MYDB_H
#define MYDB_H

#include <string>
#include <db_cxx.h>
#include "typedefs.h"

using namespace std;

// myEnv definition
class myEnv 
{
 public:
  myEnv();
  myEnv(const std::string&);
  ~myEnv() { close(); }

  DbEnv *getEnv() { return env; }
  int open(const std::string&);
  int open();
  int close();
  operator bool() {return (env != NULL);}

 private:
  DbEnv *env;    // DYK: changed to a ptr, struct is not reusable
  std::string envDir;
  static const uint32 log_size;
};

// mydb definition
class mydb
{
 public:
  enum key_comp { cmp_int, cmp_lex };
  enum dup_flag { no_dup, sort_default, sort_int };

  mydb();
  // ctor for transactional db, which includes an env argument
  mydb(const string &fname,DbEnv *myenv,key_comp keyCompare=cmp_int,
       uint32 openflags=0,dup_flag dupsort=no_dup,DBTYPE myType=DB_BTREE);
  mydb(const string &dirname,const string &fname,uint32 flags);  // convenience
 
  ~mydb() { close(); } 

  // open db in transactional environment (should be called after default ctor)
  int open(const string &fname,DbEnv *myenv,key_comp keyCompare=cmp_int,
           uint32 openflags=0,dup_flag dupsort=no_dup,DBTYPE myType=DB_BTREE);

  void initxxx();
  int close();

  Db& getDb() { return *dbp; }
  // bool isOpen() { return stat; }
  int32 countRec();
  operator bool() {return (dbp!=NULL);}

 private:
  Db* dbp;
  std::string filename;
  // bool stat;
};

/* functions for mydb key comparision */
int compare_int(Db*, const Dbt*, const Dbt*);       // recommended version
int compare_int_deprc(DB*, const DBT*, const DBT*); // DB and DBT are deprecated
int compare_str2int(DB*, const DBT*, const DBT*);   // DB and DBT are deprecated

int dupsort_int(Db*, const Dbt*, const Dbt*);

#endif
