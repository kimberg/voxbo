
// vbexdecdef.cpp
// VoxBo component
// Copyright (c) 2004-2010 by The VoxBo Development Team

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
// written by Mjumbe Poe

#include "vbexecdef.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <tr1/memory>

#include <vbprefs.h>

using namespace std;
using namespace std::tr1;
using namespace VB;

set<ExecDef*> VB::Definitions::m_defs;
set<JobType*> VB::Definitions::m_jts;
set<BlockDef*> VB::Definitions::m_bds;

ExecDef::ExecDef() : m_count(0) { Definitions::m_defs.insert(this); }

ExecDef::~ExecDef() { Definitions::m_defs.erase(this); }

string ExecDef::name() const { return m_name; }

void ExecDef::name(const string& n) { m_name = n; }

string ExecDef::description() const { return m_desc; }

void ExecDef::description(const string& d) { m_desc = d; }

unsigned ExecDef::count() const { return m_count; }

void ExecDef::update_instances() const {
  for (set<weak_ptr<Exec> >::iterator iter = instances.begin();
       iter != instances.end(); ++iter) {
    shared_ptr<Exec> inst = iter->lock();
    if (inst)
      update_instance(inst);
    else
      instances.erase(iter);
  }
}

/*
 * To create an executable instance, be it a Block or a Job, use the declare
 * method from the appropriate definition class. The declare method will create
 * a shared pointer to a new executable instance defined by the instantiating
 * definition and return the created instance. A declaration may need to copy
 * information from the definition object. For example, all instances initially
 * are named by the name of the defining object with the number of instances
 * defined by that object concatenated.
 *
 * The declare method has a helper called update_instance that takes a pointer
 * to an Exec and sets appropriate properties of the instance.
 */
string my_itoa(int n) {
  ostringstream oss;
  oss << n;
  return oss.str();
}

/*
 * Generic executable declaring and syncing
 */

shared_ptr<Exec> ExecDef::declare() {
  return declare_with_parent(shared_ptr<Block>());
}

void ExecDef::declare_helper(shared_ptr<Exec> ei) {
  ei->def = this;
  shared_ptr<Exec> ei_parent = ei->parent.lock();

  // ::NOTE:: the following insert will initialize the value of count_iter
  //          to 0 iff the key ei->parent is not already in the map.  otherwise
  //          it will set count_iter to whatever's already in the map.
  NameCountMap::iterator count_iter =
      m_count_in.insert(NameCountPair(ei_parent, 0)).first;

  ostringstream namestream;
  string s_count = my_itoa(count_iter->second++);
  s_count = string(3 - s_count.length(), '0') + s_count;
  if (s_count == "000") s_count == "";
  namestream << (ei_parent ? ei_parent->name + "_" : string("")) << m_name
             << s_count;
  ei->name = namestream.str();

  ei->path = "";
  ei->data.clear();
  ei->depends.clear();

  update_instance(ei);
  instances.insert(ei);
}

void ExecDef::sync_to_instance(shared_ptr<Exec> ei) {
  if (ei)
    ;
  else
    DEBUG_OUT("  error: attempting to sync to executable with address 0x0\n");
}

void ExecDef::update_instance(shared_ptr<Exec> ei) const {
  if (ei)
    ;
  else
    DEBUG_OUT("  error: attempting to update executable with address 0x0\n");
}

/*
 * Job declaring and syncing
 */

shared_ptr<Exec> JobType::declare_with_parent(const shared_ptr<Block> parent) {
  shared_ptr<Job> j(new Job(parent));
  declare_helper(j);

  if (!j) DEBUG_OUT("error: was not able to successfully declare new job.\n");

  return j;
}

/*
 * Block declaring and syncing
 */

shared_ptr<Exec> BlockDef::declare_with_parent(const shared_ptr<Block> parent) {
  shared_ptr<Block> b(new Block(parent));
  declare_helper(b);

  if (!b) DEBUG_OUT("error: was not able to successfully declare new block.\n");

  return b;
}

void transfer_execs(const list<shared_ptr<Exec> >& l1,
                    list<shared_ptr<Exec> >& l2, shared_ptr<Block> new_parent) {
  typedef shared_ptr<Exec> ExecPointer;
  typedef pair<ExecPointer, ExecPointer> ExecPair;
  map<ExecPointer, ExecPointer> exec_map;

  // Build the map first, in case the execs aren sorted yet.
  vbforeach(ExecPointer exec, l1) {
    ExecPointer new_exec =
        (new_parent ? exec->def->declare_with_parent(new_parent)
                    : exec->def->declare());
    exec_map[exec] = new_exec;
    l2.push_back(new_exec);

    new_exec->name = exec->name;
    new_exec->path = exec->path;
    new_exec->data =
        exec->data;  // ::FIXME:: (or at least make sure i make sense)
  }

  vbforeach(ExecPair exec_pair, exec_map) {
    vbforeach(ExecPointer wait_exec, exec_pair.first->depends) {
      if (exec_map.find(wait_exec) == exec_map.end()) {
        DEBUG_OUT(
            "error: tried to use an executable that was not [yet] in the "
            "block.\n");
      }
      exec_pair.second->depends.push_back(exec_map[wait_exec]);
    }
  }
}

void BlockDef::update_instance(shared_ptr<Exec> e) const {
  shared_ptr<Block> b = static_pointer_cast<Block>(e);

  b->execs.clear();
  transfer_execs(m_execs, b->execs, b);
}

void BlockDef::sync_to_instance(shared_ptr<Exec> e) {
  shared_ptr<Block> b = static_pointer_cast<Block>(e);

  m_execs.clear();
  transfer_execs(b->execs, m_execs, shared_ptr<Block>());
}

/*
 * The JobType class interface provides read-write access to its arguments,
 * files, and commands. The BlockDef class interface provides read-write access
 * to its contained executable instances.  There is not yet a component to take
 * advantage of read-write JobType interface.  Such a simple component as one
 * where you can graphically modify a JobType might be useful.
 */
const list<JobType::Argument>& JobType::args() const { return m_args; }
list<JobType::Argument>& JobType::args() { return m_args; }

const list<JobType::File>& JobType::files() const { return m_files; }
list<JobType::File>& JobType::files() { return m_files; }

const vector<JobType::Command>& JobType::cmds() const { return m_cmds; }
vector<JobType::Command>& JobType::cmds() { return m_cmds; }

bool JobType::anonymous() const {
  // no such thing as an anonymous jobtype
  return false;
}

const list<shared_ptr<Exec> >& BlockDef::execs() const { return m_execs; }
list<shared_ptr<Exec> >& BlockDef::execs() { return m_execs; }

bool BlockDef::anonymous() const { return m_anonymous; }

void BlockDef::anonymous(bool a) { m_anonymous = a; }

///////////////////////////////////////////////////////////////////////////////

BlockDef::BlockDef() : ExecDef(), m_anonymous(false) {
  // All executable definitions are managed by the Definitions class.
  Definitions::m_bds.insert(this);
}

BlockDef::~BlockDef() {}

void BlockDef::read(istream& in) {
  int linenum = 0;
  string line, token;
  static unsigned int anon_block_count = 0;

  shared_ptr<Exec> prev_eip;
  // ::NOTE:: 'prev_eip' refers to the executable instance that was listed
  //          directly before the one currently being processed in the pipeline.
  //          It is, in other words, the implicitly depended on executable.

  DEBUG_OUT("========== BLOCKDEF::READ BEGIN ==========\n");
  while (in.good()) {
    ++linenum;
    getline(in, line);
    trim(line);

    DEBUG_OUT("  Got and trimmed line ");
    DEBUG_OUT(linenum);
    DEBUG_OUT(": ");
    DEBUG_OUT(line);
    DEBUG_OUT("\n");

    /* Comments and blanks */
    // check for (and get rid of) comment at end of line
    size_t comment_pos = line.find("#");
    if (comment_pos != string::npos &&
        (comment_pos == 0 || line[comment_pos - 1] != '\\')) {
      line = line.substr(0, comment_pos);
      ltrim(line);
    }

    // if it's a blank line, then loop
    if (line == "") continue;

    istringstream linestream(line);
    linestream >> token;

    if (token == "end") {
      break;
    }

    /* Pipeline */
    // if we are here, then it should be an executable line or an anonymous
    // block.
    ExecDef* edp;
    shared_ptr<Exec> eip;

    /* Anonymous block */
    if (token == "block") {
      BlockDef* bdp = new BlockDef();
      if (!bdp) {
        DEBUG_OUT(
            "  error: woah there!  couldn't create a new BlockDef.  some "
            "memory problem.  wtf?!?!?!  gotta go.\n");
        exit(1);
      }
      bdp->read(in);
      bdp->anonymous(true);
      bdp->name("anonymous_block" + zero_pad(++anon_block_count, 4));

      edp = bdp;
    }

    /* Executable line */
    else {
      string exec_def_name = token;
      edp = Definitions::Get(exec_def_name);
      if (edp == 0) {
        DEBUG_OUT("  error: Cannot find executable definition named '");
        DEBUG_OUT(exec_def_name);
        DEBUG_OUT("'.  Skipping.\n");
        continue;
      }
    }

    eip = read_inst_line(linestream, edp, m_execs, prev_eip);
    m_execs.push_back(eip);
    prev_eip = eip;
  }
  DEBUG_OUT("==========  BLOCKDEF::READ END  ==========\n");
}

void BlockDef::write(ostream& out) const {
  DEBUG_OUT("========== BLOCKDEF::WRITE BEGIN ==========\n");

  out << "defblock " << name() << vb_nl;

  out << vb_indent(2);
  write_internals(out);
  out << vb_indent(-2);

  out << "end" << vb_nl;

  DEBUG_OUT("==========  BLOCKDEF::WRITE END  ==========\n");
}

void BlockDef::write_internals(ostream& out) const {
  DEBUG_OUT("  ...writing pipeline...\n");

  typedef shared_ptr<Exec> EIPointer;
  vbforeach(const EIPointer& eip, m_execs) {
    BlockDef* bdp = dynamic_cast<BlockDef*>(eip->def);
    if (bdp && bdp->anonymous()) {
      out << "block" << eip->instance_line() << vb_nl;
      out << vb_indent(2);
      bdp->write_internals(out);
      out << vb_indent(-2) << "end" << vb_nl;
    } else {
      out << eip->instance_line() << vb_nl;
    }
  }

  DEBUG_OUT("  ...pipeline written...\n");
}

///////////////////////////////////////////////////////////////////////////////

JobType::JobType() : ExecDef() {
  // Every JobType has an implicit working directory argument.
  init_working_dir_arg();

  // All executable definitions are managed by the Definitions class.
  Definitions::m_jts.insert(this);
}

void JobType::init_working_dir_arg() {
  Argument working_dir;
  working_dir.name = working_dir.info["name"] = WORKING_DIR_STRING;
  m_args.push_back(working_dir);
}

JobType::JobType(const VBJobType& jt) {
  // Copy the info...
  m_name = info["name"] = jt.shortname;
  info["description"] = jt.description;
  info["invocation"] = jt.invocation;

  vector<jobdata>::const_iterator info_iter;
  for (info_iter = jt.jobdatalist.begin(); info_iter != jt.jobdatalist.end();
       ++info_iter) {
    vector<string>::const_iterator data_iter;
    for (data_iter = info_iter->datalist.begin();
         data_iter != info_iter->datalist.end(); ++data_iter) {
      info[info_iter->key] += (*data_iter + '\n');
    }
    if (info[info_iter->key] != "") info[info_iter->key] += '\b';
  }

  // Copy the arguments...
  Argument dd;

  vector<VBArgument>::const_iterator arg_iter;
  for (arg_iter = jt.arguments.begin(); arg_iter != jt.arguments.end();
       ++arg_iter) {
    dd.name = dd.info["name"] = arg_iter->name;
    dd.info["description"] = arg_iter->description;
    dd.info["type"] = arg_iter->type;
    dd.info["defaultval"] = arg_iter->defaultval;
    dd.info["low"] = arg_iter->low;
    dd.info["high"] = arg_iter->high;
    dd.info["role"] = arg_iter->role;

    m_args.push_back(dd);
  }

  // Every JobType has an implicit working directory argument.
  init_working_dir_arg();

  // Copy the command...
  vector<VBJobType::VBcmd>::const_iterator cmd_iter;
  for (cmd_iter = jt.commandlist.begin(); cmd_iter != jt.commandlist.end();
       ++cmd_iter) {
    Command cmd;
    cmd.command = cmd_iter->command;
    cmd.script = cmd_iter->script;
    m_cmds.push_back(cmd);
  }

  // All executable definitions are managed by the Definitions class.
  Definitions::m_jts.insert(this);
}

JobType::~JobType() { Definitions::m_jts.erase(this); }

void JobType::read(istream& in) {
  DEBUG_OUT("========== JOBTYPE::READ BEGIN ==========\n");
  string line, key, value;
  bool reading_command_script = false;
  int command_line_cnt = 0;
  size_t eqpos;

  if (in.good()) {
    info.clear();
  }

  while (in.good()) {
    getline(in, line);
    trim(line);

    /* Comments and blanks */
    if (line[0] == '#' || line == "") continue;

    trim(line);

    /* Command script line */
    if (reading_command_script && line[0] == '|') {
      DEBUG_OUT("reading a line of script:");
      string script_line = line.substr(1);
      trim(script_line);
      DEBUG_OUT(script_line + "\n");
      m_cmds[command_line_cnt - 1].script.push_back(script_line);
      continue;
    } else if ((eqpos = line.find("=")) == string::npos) {
      /* Arguments */
      if (line == "argument") {
        JobType::Argument dd;
        in >> dd;
        m_args.push_back(dd);
        continue;
      }

      /* Files */
      else if (line == "file") {
        JobType::File f;
        in >> f;
        m_files.push_back(f);
        continue;
      }

      /* Commands */
      else if (line.substr(0, 7) == "command") {
        m_cmds.resize(++command_line_cnt);
        m_cmds[command_line_cnt - 1].command = line.substr(8);
        reading_command_script = true;
        continue;
      }

      else if (line == "end")
        break;

      /* Other JobType information (w/o equal sign) */
      else {
        reading_command_script = false;
        stringstream strstr;
        strstr.str(line);
        strstr >> key;
        getline(strstr, value);
        trim(value);
        info[key] = value;
      }
      //      continue;
    }

    /* Other JobType information (w/ equal sign) */
    else {
      reading_command_script = false;

      key = line.substr(0, eqpos);
      value = line.substr(eqpos + 1);

      trim(key);
      info[key] = value;

      if (key == "name")
        m_name = value;
      else if (key == "description")
        m_desc = value;
    }

    if (key == "shortname" && info.find("name") == info.end()) {
      m_name = info["name"] = value;
    }
  }
  DEBUG_OUT("==========  JOBTYPE::READ END  ==========\n");
}

istream& VB::operator>>(istream& in, JobType::Argument& dd) {
  DEBUG_OUT("...reading a JobType::Argument BEGIN...\n");
  string line, key, value;
  size_t eqpos;

  if (in.good()) dd.info.clear();

  while (in.good()) {
    getline(in, line);

    if (line[0] == '#') continue;

    eqpos = line.find("=");
    if (eqpos == string::npos) {
      trim(line);
      if (line == "end")
        break;
      else {
        istringstream line_stream(line);

        line_stream >> key;
        getline(line_stream, value);

        ltrim(value);
        if (key == "name") {
          dd.name = value;
        }
        dd.info[key] = value;
      }

      continue;
    }

    key = line.substr(0, eqpos);
    value = line.substr(eqpos + 1);

    trim(key);
    if (key == "name") {
      dd.name = value;
      DEBUG_OUT("  name is ");
      DEBUG_OUT(dd.name);
      DEBUG_OUT("\n");
    }
    dd.info[key] = value;
  }

  DEBUG_OUT("...reading a JobType::Argument END...\n");
  return in;
}

istream& VB::operator>>(istream& in, JobType::File& f) {
  DEBUG_OUT("...reading a JobType::File BEGIN...\n");
  string line, keyword, key, value, temp;
  size_t eqpos, colpos;

  if (in.good()) f.info.clear();

  while (in.good()) {
    getline(in, line);
    trim(line);

    if (line[0] == '#' || line == "") continue;

    istringstream linestream(line);

    linestream >> keyword;

    if (keyword == "fileid")
      getline(linestream, f.id);
    else if (keyword == "input")
      getline(linestream, f.in_name);
    else if (keyword == "informats") {
      getline(linestream, temp);
      f.in_types = split(temp, " ");
    } else if (keyword == "end")
      break;
    else {
      colpos = keyword.find(":");
      string format = "auto";

      if (colpos != string::npos) format = keyword.substr(0, colpos);

      if ((colpos != string::npos && keyword.substr(colpos + 1) == "output") ||
          keyword == "output") {
        // output line stuff
        getline(linestream, f.out_names[format]);
      } else if ((colpos != string::npos &&
                  keyword.substr(colpos + 1) == "set") ||
                 keyword == "set") {
        // set line stuff
        string keyval;
        getline(linestream, keyval);
        DEBUG_OUT("  a set statement: ");
        DEBUG_OUT(keyval);
        DEBUG_OUT("\n");

        eqpos = keyval.find("=");
        if (eqpos == string::npos)
        // This case, where you have a set line with no equal sign anywhere,
        // is just a problem.  At this point, the program should break, but
        // gracefully.
        {
          DEBUG_OUT("yo, there was a problem.  missing equal sign ('=').\n");
          DEBUG_OUT("...reading a JobType::File ABORT...\n");
          return in;
        }

        key = keyval.substr(0, eqpos);
        value = keyval.substr(eqpos + 1);

        f.vars[format][key] = value;
      }
    }
  }

  // ::NOTE:: I'm not sure if the following is of any use anymore.
  //   -- as of 18.Jul.2007

  //	if (!f.in_type.empty())
  //	{
  //	  if (f.out_type.empty())
  //	    f.out_type.push_back(f.in_type.front());
  //	}
  //	else
  //	{
  //	  cerr << "warning: no informats suggested." << endl;
  //	  f.in_type.push_back("");
  //	  f.out_type.push_back("");
  //	}

  DEBUG_OUT("...reading a JobType::File END...\n");
  return in;
}

void JobType::write(ostream& out) const {
  DEBUG_OUT("========== JOBTYPE::WRITE BEGIN ==========\n");

  out << "name=" << name() << vb_nl << vb_nl;

  vbforeach(const Argument& arg, m_args) {
    out << "argument" << vb_nl;
    out << "  name=" << arg.name << vb_nl;
    out << "end" << vb_nl;
  }

  vbforeach(const File& f, m_files) {
    out << "file" << vb_nl;
    out << "  fileid " << f.id << vb_nl;
    out << "  input " << f.in_name << vb_nl;
    out << "  informats ";
    vbforeach(const string& format, f.in_types) { out << format; }
    out << vb_nl << vb_nl;

    typedef pair<string, string> key_val_pair;
    vbforeach(const key_val_pair& kv, f.global_vars) {
      out << "  set " << kv.first << "=" << kv.second << vb_nl;
    }

    typedef pair<string, string> format_name_pair;
    vbforeach(const format_name_pair& fm, f.out_names) {
      out << "  " << fm.first << ":output " << fm.second << vb_nl;
    }

    typedef pair<string, map<string, string> > format_kv_pair;
    vbforeach(const format_kv_pair& fkv, f.vars) {
      vbforeach(const key_val_pair& kv, fkv.second) {
        out << "  " << fkv.first << ":set " << kv.first << "=" << kv.second
            << vb_nl;
      }
    }
    out << "end" << vb_nl;
  }

  vbforeach(const Command& cmd, m_cmds) {
    out << "command " << cmd.command << vb_nl;
    // ::NOTE:: remember to put in scripts,
    //          if a command has any.
  }

  DEBUG_OUT("==========  JOBTYPE::WRITE END  ==========\n");
}

///////////////////////////////////////////////////////////////////////////////

ExecDef* Definitions::Get(const string& n) {
  vbforeach(ExecDef * edp, m_defs) {
    if (edp->name() == n) return edp;
  }
  return 0;
}

void Definitions::Import_JobType_Folder() {
  // FIXME: DYK: added the following three lines to get rid of extern
  // declaration.  but we should probably either pass the jobtypemap
  // in somehow or move this code to the scripting application
  VBPrefs vbp;
  vbp.init();
  vbp.read_jobtypes();
  for (TI it = vbp.jobtypemap.begin(); it != vbp.jobtypemap.end(); it++)
    new JobType(it->second);

  /*  // ::NOTE:: 'glob_t' is POSIX.  Perhaps a viable alternative is to use the
    //          boost regular expression and filesystem libraries.
    vglob vg(vbp.rootdir + "etc/jobtypes/X_*.vjt");
    for (size_t i=0; i<vg.size(); i++)
    {
          DEBUG_OUT("Importing JobType "); DEBUG_OUT(vg[i].c_str());
    DEBUG_OUT(".\n"); ifstream in_file; in_file.open(vg[i].c_str()); if
    (in_file.good()) (new JobType())->read(in_file);
    }
  */
}

const set<ExecDef*>& Definitions::defs() { return m_defs; }

const set<JobType*>& Definitions::jts() { return m_jts; }

const set<BlockDef*>& Definitions::bds() { return m_bds; }
