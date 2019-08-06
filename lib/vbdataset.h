
// vbdataset.h
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
// original version written by Mjumbe Poe

#ifndef VBDATASET_H
#define VBDATASET_H

#include "tokenlist.h"
#include "vbscripttools.h"

#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

using std::istream;
using std::list;
using std::map;
using std::ostream;
using std::string;

namespace VB {
class DataSet {
  friend class Sequence;

 public:
  DataSet();
  DataSet(const string& filename);
  DataSet(istream& stream);
  DataSet(const DataSet& ds);
  virtual ~DataSet();

  struct Member {
    string name;
    string value;
    map<string, string> info;

    bool operator<(const Member& m) const { return name < m.name; }
  };

  /* struct Node
     {
     string name;
     Node* parent;
     list<Node> children;
     list<Member> members;
     };*/

  virtual string get_name() const;
  void set_name(const string& n);
  virtual string get_long_name() const;
  int get_child_count() const;

  void insert_member(Member* m, bool overwrite = true);
  Member* insert_member(const string& n, bool overwrite = true);
  void remove_member(const Member* m);
  void remove_member(const string& n);
  const list<Member*> get_members(bool recurse = true) const;
  Member* get_member(string datname, bool recurse = true) const;

  DataSet* get_parent() const;
  void set_parent(DataSet* ds);

  DataSet* get_root() const;
  void set_root(DataSet* ds);

  void insert_child(DataSet* g);
  DataSet* insert_child(const string& n);
  void remove_child(DataSet* g);
  DataSet* get_child(const string& childname) const;
  const list<DataSet*>& get_children() const;
  list<DataSet*> get_children(const string& path) const;

  unsigned get_id() const { return _id; }

  string get_resolved_string(const string& s) const;
  tokenlist to_tokenlist() const;
  map<string, string> to_map() const;

  istream& read(istream& in);
  friend istream& operator>>(istream& in, DataSet& dsd);

  void spit_tree_to_stdout(string prefix = "");

  DataSet& operator=(const DataSet& ds);

  /*
   * import_members
   *
   * Take the members from ds and inset them into this dataset node. Searches
   * recursively for the imported members. Inserts directly into this node.
   */
  void import_members(const DataSet* ds, bool overwrite = false);

  /*
   * import_inherited_members
   *
   * Grab the members from all ancestor nodes and insert directly into this
   * dataset node.
   */
  void import_inherited_members();

 public:
  list<DataSet*> _children;
  list<Member*> _members;
  DataSet* _parent;
  DataSet* _root;

 protected:
  int _instance_count;

  string _name;
  string _description;

  static unsigned ID;
  unsigned _id;

  /* virtual void insert_source(DataContainer* dc);
     virtual void remove_source(DataContainer* dc);
     virtual void insert_sink(DataContainer* dc);
     virtual void remove_sink(DataContainer* dc);*/

  void copy(const DataSet& ds);
  void destroy();
};

istream& operator>>(istream& in, DataSet& dsd);
}  // namespace VB

#endif
