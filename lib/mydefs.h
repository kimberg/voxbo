
// mydefs.h
// 
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
// original version written by Dan Kimberg and Dongbo Hu

/* Definitions of new DB classes that are mainly used by the GUI on
 * client side.  Classes in this file are designed for user interface,
 * so they are not exactly the same as classes defined to
 * add/modify/retrieve records in db file.*/
 
#ifndef DBDEFS_H
#define DBDEFS_H

#include <map>
#include <string>
#include <stdint.h>
#include <vector>
// #include <QPixmap>
#include "vbio.h"
#include "bdb_tab.h"
#include "dbdate.h"

// CLASS DECLARATIONS
class DBtype;
class DBpatient;
class DBscorename;
class DBscorevalue;
class DBsession;

// CLASS DEFINITIONS

class DBviewspec {
 public:
  string name;
  vector<string> entries;
  unsigned char *serialized();
  int deserialize(const string &str);
};

class DBtype {
 public:
  DBtype();
  DBtype(void*);
  void clear();
  void deserialize(void*);
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;
  void print() const;

  string name;
  string description;
  uint8 t_custom;            // flag set if user-specified values are allowed
  vector<string> values;
};

class DBsession {
 public:
  DBsession();
  DBsession (void*);
  void clear();
  void deserialize(void*);
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;
  void print() const;

  int32 id, studyID;
  int32 patient;
  string examiner;
  DBdate date;
  uint8 pubFlag;
  string notes;
};

class DBpatient {
 public:
  DBpatient();
  void clear();
  DBpatient(int32, vector<DBscorevalue>, vector<DBsession>);
  void setData(int32, vector<DBscorevalue>, vector<DBsession>);
  void updateTime(uint32);
  void setSessionMap(vector<DBsession>&);
  void resetMaps();
  uint32 getSessionSize() const;
  void serializeSessions(char*) const;
  operator bool() {return (patientID>0);}
  void print() const;

  int32 patientID; // patient ID
  map<int32, DBsession> sessions;        // map a sessionid to the session
  map<int32, DBscorevalue> scores;       // all the scorevalues for this patient mapped by id
  multimap<int32, int32> children;       // multimap a score value to its children
  multimap<string, int32> names;         // map a scorename to all the instances for this patient
  multimap<int32, int32> sessiontests;   // multimap a session to its tests (scores with parent 0)
};

class DBscorename {
 public:
  DBscorename();
  DBscorename(void*);
  void clear();
  void deserialize(void*);
  int32 getSize() const;
  void serialize(char*) const;
  void show() const;
  void print() const;

  string name;            // this is the scorename's unique id
  string screen_name;     // user-visible name
  string datatype;        // datatype either built-in or in typemap
  string desc;

  // flags contains attribute-value pairs for all the strings.  for
  // boolean flags, if they are in the map, they are considered to be
  // set (i.e., true). flags include:

  // leaf, defer, customizable, searchable, repeating, editable,
  // neverhide, min, max, default, places (decimal places for float,
  // number of lines for text)
  map<string,string> flags;
};

class DBscorevalue {
 public:
  DBscorevalue();
  DBscorevalue(void*);
  DBscorevalue(const DBscorevalue&);
  
  void clear();
  void deserialize(void*);
  uint32 deserializeHdr(void*);
  void serialize(char*);
  uint32 serializeHdr(char*) const;
  uint32 getSize();
  uint32 getHdrSize() const;
  uint32 getDatSize();
  string printable(bool full=0) const;
 
  void show();
  string getDatStr();
  void print() const;

  // serialize and deserialize functions written by Dan
  pair<uint8*, uint32> serialize();
  void deserialize(uint8* data, uint32 length);

  string key;            // unique key built from id, index and another numerical label
  int32 id;              // score value id (test result id), to establish relationships
  int32 patient;         // redundant on client side, but is useful for server
  string scorename;      // reference back to the relevant DBscorename
  string datatype;
  int32 sessionid;       // may be used later
  int32 parentid;        // id of parent score
  uint32 index;          // for repeating scores, the index value
  uint8 deleted;         // flag that tells whether a record has been deleted
  string permission;     // r/rw/b permission

  // small bits of history about score
  DBdate whenset;        // time at which record is set in db 
  string setby;

  DBdate v_date;    // for datatypes date, time, and datetime
  Cube v_cube;      // for datatype brainimage
  // QPixmap v_pixmap; // for datatype image
  dblock v_pixmap;  // formerly a QPixmap, now a dblock with png data
  string v_string;  // for datatypes string, shortstring, text, bool,
                    // int, float, and all custom or unrecognized 
                    // types
};

class DBpatientlist {
 public:
  DBpatientlist();
  DBpatientlist(void*);
  int32 getSize() const;
  void serialize(char*) const;
  void show() const;

  int32 id;
  int32 ownerID;
  string name, search_strategy, notes;
  DBdate runDate, modDate;
  uint8 dirtyFlag;
  set<int32> patientIDs;
};

// Utility functions written by Dan, first few should be templatized probably
vector<int32> getchildren(multimap<int32,int32>& mymap,int32 parent);
vector<int32> getchildren(multimap<string,int32>& mymap,string parent);
vector<string> getchildren(multimap<string,string>& mymap,string parent);
string scoreparent(const string& str);
string scoretest(const string& str);
string scorebasename(const string& str);

#endif // DBDEFS_H
