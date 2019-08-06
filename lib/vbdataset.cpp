
// vbdataset.cpp
//
// Copyright (c) 2007-2010 by The VoxBo Development Team

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

#include "vbdataset.h"
#include "vbutil.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;
using namespace VB;

unsigned DataSet::ID = 0;

/* DataSet Constructors/Destructors
 *
 * A DataSet is a tree of data.  Its default constructor will just initialize
 * variables.  Passing a /filename/ or a /stream/ to the constructor will read
 * in the DataSet from the file, if it exists.
 */
DataSet::DataSet()
    : _parent(0),
      _root(this),
      _instance_count(0),
      _name("DataSet"),
      _description(""),
      _id(ID++) {
  _members.clear();
  _children.clear();
}

DataSet::DataSet(const std::string& filename)
    : _parent(0),
      _root(this),
      _instance_count(0),
      _name("DataSet"),
      _description(""),
      _id(ID++) {
  _members.clear();
  _children.clear();

  ifstream infile;
  infile.open(filename.c_str());
  if (infile.good()) infile >> *this;
}

DataSet::DataSet(std::istream& stream)
    : _parent(0),
      _root(this),
      _instance_count(0),
      _name("DataSet"),
      _description(""),
      _id(ID++) {
  _members.clear();
  _children.clear();

  stream >> *this;
}

DataSet::DataSet(const DataSet& ds)
    : _parent(0),
      _root(this),
      _instance_count(0),
      _name(ds._name),
      _description(ds._description),
      _id(ds._id) {
  copy(ds);
}

DataSet::~DataSet() { destroy(); }

/* get_name
 *
 * Return the [short] name of the dataset.
 */
string DataSet::get_name() const { return _name; }

/* set_name
 *
 * Sets the [short] name of the dataset.
 */
void DataSet::set_name(const std::string& n) { _name = n; }

/* get_long_name
 *
 * Return the [short] name of the dataset appended to the name of its parent
 * dataset in the tree, if a parent exists.
 */
string DataSet::get_long_name() const {
  if (this != _root && _parent)
    return (_parent->get_long_name() + ":" + _name);
  else
    return _name;
}

/* get_child
 *
 * Return a pointer to the child DataSet that has the name /childname/.
 */
DataSet* DataSet::get_child(const string& path) const {
  if (path == "") {
    return const_cast<DataSet*>(this);
  } else {
    size_t pos = path.find(":");

    string sub_path = "";
    if (pos != string::npos) sub_path = path.substr(pos + 1);
    trim(sub_path);

    if (pos == 0) return get_child(sub_path);

    string childname = path.substr(0, pos);
    trim(childname);

    vbforeach(DataSet * child,
              _children) if (child->get_name() ==
                             childname) return child->get_child(sub_path);
  }

  return 0;
}

/* get_children
 *
 * Return the set of child datasets of /this/ in the tree.
 */
const list<DataSet*>& DataSet::get_children() const { return _children; }

/* get_children
 *
 * Returns the iteration set of DataSet nodes for this path.  Does this through
 * recursion.
 */
list<DataSet*> DataSet::get_children(const string& path) const {
  list<DataSet*> datasets;

  if (path == "") {
    datasets.push_back(const_cast<DataSet*>(this));
  } else {
    size_t pos = path.find(":");

    string further_subds_names = "";
    if (pos != string::npos) further_subds_names = path.substr(pos + 1);
    trim(further_subds_names);

    if (pos == 0) return get_children(further_subds_names);

    string subds_name = path.substr(0, pos);
    trim(subds_name);

    if (subds_name.find("*") != string::npos) {
      set<DataSet*> ds_storage;
      vbforeach(DataSet * subds, get_children()) {
        if (matches(subds->get_name(), subds_name)) {
          list<DataSet*> further_subds =
              subds->get_children(further_subds_names);

          ds_storage.insert(further_subds.begin(), further_subds.end());
        }
      }
      datasets.insert(datasets.end(), ds_storage.begin(), ds_storage.end());
    } else {
      DataSet* subds = get_child(subds_name);
      if (subds) datasets = subds->get_children(further_subds_names);
    }
  }

  return datasets;
}

/* get_child_count()
 *
 * Return the number of children of this dataset.
 */
int DataSet::get_child_count() const { return _children.size(); }

/* insert_child
 *
 * Add a child dataset to the /children/ (if it is not there already).  Set
 * the parent of that child to /this/.
 */
void DataSet::insert_child(DataSet* g) {
  if (find(_children.begin(), _children.end(), g) == _children.end()) {
    _children.push_back(g);
    g->set_parent(this);
    g->set_root(_root);
  }
}

DataSet* DataSet::insert_child(const string& n) {
  DataSet* child = new DataSet();
  child->_name = n;
  insert_child(child);
  return child;
}

/* remove_child
 *
 * Remove a child from the /children/.
 */
void DataSet::remove_child(DataSet* g) {
  list<DataSet*>::iterator child_iter =
      find(_children.begin(), _children.end(), g);
  if (child_iter != _children.end()) {
    _children.erase(child_iter);
    g->set_parent(0);
  }
}

/* set_parent
 *
 * Set the dataset's parent in the tree to /ds/.
 */
void DataSet::set_parent(DataSet* ds) {
  if (_parent != ds) {
    DataSet* old_parent = _parent;
    _parent = ds;

    if (_parent)
      set_root(_parent->_root);
    else
      set_root(this);

    if (old_parent) old_parent->remove_child(this);
    if (_parent) _parent->insert_child(this);
  }
}

/* get_parent
 *
 * Return the dataset's parent.
 */
DataSet* DataSet::get_parent() const { return _parent; }

/* set_root
 *
 * Set the dataset's root in the tree to /ds/.
 */
void DataSet::set_root(DataSet* ds) {
  if (ds == 0)
    DEBUG_OUT(
        "YO!!! WTF!!! THIS ROOT IS 0!!!  sumthin is seriously wrong...\n");
  _root = ds;

  for (list<DataSet*>::const_iterator iter = get_children().begin();
       iter != get_children().end(); ++iter) {
    DataSet* child_dsp = *iter;
    child_dsp->set_root(_root);
  }
}

/* get_root
 *
 * Return the dataset's root node.
 */
DataSet* DataSet::get_root() const { return _root; }

/* get_member
 *
 * Return a pointer to a Member that has the name /datname/.
 */
DataSet::Member* DataSet::get_member(string datname, bool recurse) const {
  vbforeach(const Member* mem,
            _members) if (mem->name == datname) return const_cast<Member*>(mem);

  if (recurse && _parent != 0)
    return _parent->get_member(datname);

  else
    return 0;
}

void DataSet::insert_member(Member* m, bool override) {
  // look (non-recursively) for the member.
  Member* old_m = get_member(m->name, false);
  if (old_m) {
    if (!override) return;
    remove_member(old_m);
  }
  _members.push_back(m);
}

DataSet::Member* DataSet::insert_member(const std::string& n, bool override) {
  Member* m;
  if (!override && (m = get_member(n, false))) return m;

  m = new Member;
  m->name = n;
  m->value = "";
  insert_member(m, override);

  return m;
}

void DataSet::remove_member(const Member* m) {
  _members.remove(const_cast<Member*>(m));
  if (m) delete m;
}

void DataSet::remove_member(const std::string& n) {
  vbforeach(const Member* mem, _members) {
    if (mem->name == n) remove_member(mem);
  }
}

/* get_members
 *
 * Return the set of data members of this dataset.
 */
const list<DataSet::Member*> DataSet::get_members(bool recurse) const {
  set<string> mem_names;
  if (!recurse) return _members;

  list<Member*> my_members;
  vbforeach(const Member* mem, _members) {
    if (mem_names.insert(mem->name).second)
      my_members.push_back(const_cast<Member*>(mem));
  }

  if (_parent != 0) {
    list<Member*> parent_members = _parent->get_members();
    vbforeach(const Member* mem, parent_members) {
      if (mem_names.insert(mem->name).second)
        my_members.push_back(const_cast<Member*>(mem));
    }
  }

  return my_members;
}

/* operator>> ... (and helper functions)
 *
 * Read a dataset /ds/ from the stream /in/.
 *
 * set_member : read in a member name and value from /in/, and insert that
 * member into the dataset /ds/.
 */
void set_member(istream& in, DataSet& ds) {
  string token, key, value;

  if (!in.good()) return;

  in >> token;
  size_t pos = token.find("=");

  if (pos == string::npos) return;

  key = token.substr(0, pos);
  value = token.substr(pos + 1);

  if (value[0] == '\"') {
    value = value.substr(1);
    if (value[value.length() - 1] != '\"') {
      while (in.good()) {
        char c = in.get();
        if (c != '\"')
          value += c;
        else
          break;
      }
    } else {
      value = value.substr(0, value.length() - 1);
    }
  }

  DataSet::Member* mem(new DataSet::Member());
  mem->name = key;
  mem->value = value;
  ds.insert_member(mem);
}

/* peek_token
 *
 * Look at the next token in the stream, but pretend like you didn't.
 */
string peek_token(istringstream& str) {
  string token;
  int p = str.tellg();

  str >> token;

  str.seekg(p);
  str.clear();

  return token;
}

std::istream& DataSet::read(std::istream& in) {
  //  cerr << endl;
  //  cerr << "==================== reading a dataset ====================" <<
  //  endl;

  string line, token, key, value, groupname;
  DataSet* parent;
  DataSet* child = this;

  while (in.good()) {
    getline(in, line);
    trim(line);

    // If we have comments or blank lines...
    if (line[0] == '#' || line == "") {
      // ...ignore it and go to the next
      continue;
    }

    // If we have lines that pertain to the most recently specified child
    // dataset...
    else if (line[0] == ':') {
      line = line.substr(1, line.length() - 1);
    }

    // If we have lines that pertain to the root dataset...
    else {
      child = this;
    }

    istringstream str;
    str.str(line);
    while (str.good()) {
      // Only peek at the token, in case it's a member variable that we need to
      // set (because 'set_member' uses the stream; maybe it shouldn't, but it
      // does).
      token = peek_token(str);

      // We should only either have a member variable definition or the path of
      // a child dataset as the first token on a line.
      if (token.find("=") != string::npos) {
        set_member(str, *child);
      } else {
        // Pull the path [token], since we only peeked at it before.
        string path;
        str >> path;

        child = this;

        // ...split the path on colons
        list<string> strs = split(path, ":");
        vbforeach(string childname, strs) {
          parent = child;
          child = parent->get_child(childname);
          if (child == 0) {
            child = parent->insert_child(childname);
          }
        }

        set_member(str, *child);
      }
    }
  }

  ++_instance_count;
  return in;
}

istream& VB::operator>>(istream& in, DataSet& ds) { return ds.read(in); }

void DataSet::spit_tree_to_stdout(string prefix) {
  cout << prefix << "+--" << get_name() << endl;

  vbforeach(const Member* mem, _members) {
    cout << prefix << "|  +  var:" << mem->name << "=" << mem->value << endl;
  }

  vbforeach(DataSet * ds, get_children()) {
    if (ds)
      ds->spit_tree_to_stdout(prefix + "|  ");
    else
      cerr << "bad child dataset!" << endl;
  }
}

DataSet& DataSet::operator=(const DataSet& ds) {
  destroy();
  copy(ds);
  return *this;
}

void DataSet::destroy() {
  vbforeach(Member * member, _members) { delete member; }

  vbforeach(DataSet * child, _children) {
    //    child->destroy();
    delete child;
  }
}

void DataSet::copy(const DataSet& ds) {
  _id = ds._id;
  _instance_count = 0;
  _parent = 0;
  _root = this;
  _name = ds._name;
  _description = ds._description;

  _members.clear();
  vbforeach(const Member* mem, ds._members) { insert_member(new Member(*mem)); }
  _children.clear();
  vbforeach(DataSet * child, ds._children) {
    insert_child(new DataSet(*child));
  }
}

void DataSet::import_members(const DataSet* ds, bool overwrite) {
  if (ds) {
    std::list<Member*> in_mems = ds->get_members();
    for (list<Member*>::iterator iter = in_mems.begin(); iter != in_mems.end();
         ++iter) {
      Member*& mem = *iter;

      insert_member(new Member(*mem), overwrite);
    }
  }
}

/* get_resolved_string
 *
 * Fill a string with appropriate values from the dataset.  Like fill_vars2,
 * except with a VB::DataSet.
 */
string DataSet::get_resolved_string(const string& s) const {
  map<string, string> vars = to_map();
  string temp = s;
  fill_vars2(temp, vars);
  return temp;
}

// DYK: merged to_tokenlist and to_map because variable sets are now
// always maps.  this function makes a map<string,string> of all
// var=val pairs, and then does replacement internally.

map<string, string> DataSet::to_map() const {
  map<string, string> vars, temp;

  list<DataSet::Member*> members = get_members();
  vbforeach(DataSet::Member * member, members) {
    vars[member->name] = member->value;
  }

  int cnt;
  do {
    cnt = 0;
    temp = vars;
    for (map<string, string>::iterator i = vars.begin(); i != vars.end(); i++)
      cnt += fill_vars2(i->second, temp);
  } while (cnt != 0);

  return vars;
}

void DataSet::import_inherited_members() { import_members(_parent, false); }
