
// vbsequence.cpp
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

#include "vbsequence.h"
#include "vbdataset.h"

#include <sstream>

using namespace std;
using namespace std::tr1;
using namespace VB;

unsigned Exec::GLOBAL_EXEC_ID = 0;

string Exec::instance_line() const
{
  ostringstream oss;
  
  typedef shared_ptr<Exec> ei_pointer;
  
  oss << def->name();
  if (name != "") oss << " name " << name;
  if (path != "") oss << " using " << path;

  for (list<ei_pointer>::const_iterator iter = depends.begin(); 
       iter != depends.end(); ++iter)
  {
    const ei_pointer& ei = *iter;
    oss << " waitfor " << ei->name;
  }
  if (depends.empty()) oss << " nowait";
  
  for (map<string,string>::const_iterator iter = data.begin(); 
       iter != data.end(); ++iter)
  {
    const pair<string,string>& name_val = *iter;
    oss << " " << name_val.first << "=\"" << name_val.second << "\"";
  }
  
  return oss.str();
}

Exec::Exec(shared_ptr<Block> p) : path(""), parent(p), id(GLOBAL_EXEC_ID++) {}
Job::Job(shared_ptr<Block> p) : Exec(p) {}
Block::Block(shared_ptr<Block> p) : Exec(p) {}

// recursive_contains determines whether the given executable instance 
// pointer is a member of the given container, or is a part of a block
// in that container.
template<typename container>
bool recursive_contains(container& ct, const shared_ptr<Exec>& ep)
{
  typedef shared_ptr<Exec> EP;
  // for each EP sub_ep in the container ct
  for (typename container::const_iterator iter = ct.begin();
       iter != ct.end(); ++iter)
  {
    const EP& sub_ep = *iter;
    
    if (sub_ep == ep || 
        sub_ep->contains(ep)) return true;
  }
  return false;
}

bool Exec::depends_on(const shared_ptr<Exec>& ep) const
{
  return recursive_contains(depends, ep);
}

bool Block::contains(const shared_ptr<Exec>& ep) const
{
  return recursive_contains(execs, ep);
}

bool Job::contains(const shared_ptr<Exec>&) const
{
  return false;
}

string Exec::full_path() const
{
  string full_path = "";
  shared_ptr<Block> p = parent.lock();
  if (p)
  {
    if (path == "")
      return p->full_path();
    else
      return p->full_path() +  ":" + path;
  }
  
  return path;
}

///////////////////////////////////////////////////////////////////////////////

Sequence::Sequence(BlockDef* root_bdp)
{
  m_vars[SEQUENCE_NAME_STRING] = "";
  m_vars["PRIORITY"] = "3";
  
  root_block_def(root_bdp);
}

Sequence::~Sequence()
{
}

const shared_ptr<Block>& Sequence::root_block() const
{
  return m_root_block;
}

void Sequence::root_block(shared_ptr<Block>& root_bp)
{
  m_root_block = root_bp;
}

void Sequence::root_block_def(BlockDef* root_bdp)
{
  if (m_root_block)
    m_root_block.reset();
  
  if (root_bdp)
    m_root_block = static_pointer_cast<Block>(root_bdp->declare());
}

const map<string,string>& Sequence::vars() const
{
  return m_vars;
}

map<string,string>& Sequence::vars()
{
  return m_vars;
}

const string& Sequence::name() const
{
  map<string,string>::const_iterator iter = m_vars.find(SEQUENCE_NAME_STRING);
  if (iter == m_vars.end())
  {
    DEBUG_OUT("  error: Sequence has no name variable.");
  }
  return iter->second;
}

void Sequence::name(const string& n)
{
  m_vars[SEQUENCE_NAME_STRING] = n;
}

const string& Sequence::priority() const
{
  map<string,string>::const_iterator iter = m_vars.find("PRIORITY");
  if (iter == m_vars.end())
  {
    DEBUG_OUT("  error: Sequence has no priority variable.");
  }
  return iter->second;
}

void Sequence::priority(const string& p)
{
  m_vars["PRIORITY"] = p;
}

/*
 * Topologically sorting the sequence executables according to dependency.
 */
bool Sequence::execs_are_sorted() const
{
  typedef shared_ptr<Exec> ExecPointer;
  list<ExecPointer>& execs = m_root_block->execs;
  
  set<ExecPointer> seen_execs;
  vbforeach (ExecPointer exec, execs)
  {
    list<ExecPointer>& wait_execs = exec->depends;
    vbforeach (ExecPointer wait_exec, wait_execs)
    {
      if (seen_execs.find(wait_exec) == seen_execs.end())
        return false;
    }
    seen_execs.insert(exec);
  }
  return true;
}

list<shared_ptr<Exec> >& Sequence::sort_execs()
{
  typedef shared_ptr<Exec> ExecPointer;
  list<ExecPointer>& execs = m_root_block->execs;
  
  if (!execs_are_sorted())
  {
    // Build a depth table.  Yay, dynamic programming!
    map<shared_ptr<Exec>, int> depths;
    build_depth_table(&depths);
    
    // Let multimap do the work of semi-efficiently sorting the table according to
    // the Job depths.
    multimap<int, shared_ptr<Exec> > sorted_depth_table;
    
    typedef pair<shared_ptr<Exec>, int> ExecIntPair;
    vbforeach (ExecIntPair table_pair, depths)
      sorted_depth_table.insert(pair<int, shared_ptr<Exec> >(table_pair.second, table_pair.first));
    
    // Now, I don't actually care what the depths are; I just needed them to sort
    // the Jobs topologically.  So now let's get rid of the depths and just return
    // a list of sorted Jobs.
    execs.clear();
    typedef pair<int, shared_ptr<Exec> > IntExecPair;
    vbforeach (IntExecPair order_pair, sorted_depth_table)
      execs.push_back(order_pair.second);
  }
  
  return execs;
}

void Sequence::build_depth_table(map<shared_ptr<Exec>, int>* table)
{
  typedef shared_ptr<Exec> ExecPointer;
  list<ExecPointer>& execs = m_root_block->execs;
  
  for (list<ExecPointer>::iterator iter = execs.begin(); 
       iter != execs.end(); ++iter)
  {
    ExecPointer& exec = *iter;
    build_depth_table_helper(exec, table);
  }
}

void Sequence::build_depth_table_helper(shared_ptr<Exec> exec, map<shared_ptr<Exec>, int>* table)
{
  typedef shared_ptr<Exec> ExecPointer;
  
  // We don't need to build what we've already built, so check whether we've
  // done this Job.  Get out if we have.
  if (table->find(exec) != table->end())
    return;
    
  int depth = 0;
  
  // Get the direct dependencies of this Job.
  list<ExecPointer>& deps = exec->depends;
  if (!deps.empty())
  {
    // Make sure that all of the dependencies are in the depth table.
    for (list<ExecPointer>::iterator iter = deps.begin(); 
         iter != deps.end(); ++iter)
    {
      ExecPointer& wait_exec = *iter;
      build_depth_table_helper(wait_exec, table);
    }
      
    
    // Set the depth of this Job to the max depth of its dependencies plus one.
    for (list<ExecPointer>::iterator iter = deps.begin(); 
         iter != deps.end(); ++iter)
    {
      ExecPointer& wait_exec = *iter;
      if (depth <= (*table)[wait_exec])
        depth = (*table)[wait_exec] + 1;
    }
      
  }

  (*table)[exec] = depth;  
}

/* 
 * Input and output
 */

void Sequence::read(istream& in)
{
  DEBUG_OUT("========== SEQUENCE::READ BEGIN ==========\n");
  int linenum = 0;
  string line, token;
  
  shared_ptr<Exec> prev_ep;
  // ::NOTE:: 'prev_ep' refers to the executable instance that was listed 
  //          directly before the one currently being processed in the pipeline.
  //          It is, in other words, the implicitly depended on executable.
  
  BlockDef* root_bdp = new BlockDef();
  // ::NOTE:: 'root_bdp' is the definition of the root block.
  
  while (in.good())
  {
    ++linenum;
    getline(in, line);
    trim(line);
    
    DEBUG_OUT("  Got and trimmed line "); DEBUG_OUT(linenum); DEBUG_OUT(": "); DEBUG_OUT(line); DEBUG_OUT("\n");
    
    /* Comments and blanks */
    // check for (and get rid of) comment at end of line
    size_t comment_pos = line.find("#");
    if (comment_pos != string::npos &&
        (comment_pos == 0 || line[comment_pos-1] != '\\'))
    {
      line = line.substr(0,comment_pos);
      ltrim(line);
    }
    
    // if it's a blank line, then loop
    if (line == "") continue;
    
    istringstream linestream(line);
    linestream >> token;
    
    /* Setting variables */
    if (token == "set")
    {
      string nameval_string;
      getline(linestream, nameval_string);
      
      size_t eqpos = nameval_string.find("=");
      if (eqpos == string::npos)
      {
        DEBUG_OUT("  error: Line with 'set' directive but no '=' sign.  Skipping.\n");
        continue;
      }
      
      // grab the name (whitespace-trimmed on both sides) and the value (not
      // trimmed on either side).
      string name_string, val_string;
      name_string = nameval_string.substr(0, eqpos);
      val_string = nameval_string.substr(eqpos+1);
      trim(name_string);
      
      m_vars[name_string] = val_string;
      continue;
    }
    
    /* Including other sequence/block library files */
    if (token == "include")
    {
      string include_filename;
      getline(linestream, include_filename);
      // ::NOTE:: we need to figure out how we want to deal with
      //          importing files.  there are a few things to consider:
      //           - how do we want to protect against cycles?
      //           - should we use posix, knowing that we want windows 
      //             operability?  (yes, we just have to do multiple 
      //             versions enclosed within #ifdefs).
      //           - do we read them by calling Sequence::read on another
      //             stream?
      
      static set<string> files_already_seen;
      char cur_working_dir[512];
      getcwd(cur_working_dir, 512);
      include_filename = string(cur_working_dir) + include_filename;
      
    }
    
    /* Defining blocks */
    if (token == "defblock")
    {
      BlockDef* bdp = new BlockDef();
      string name_string = "";
      linestream >> name_string;
      if (name_string == "")
      {
        DEBUG_OUT("  error: Block definition without name.  Continuing on.\n");
      }
      
      bdp->name(name_string);
      bdp->read(in);
      m_exec_defs.push_back(bdp);
      continue;
    }
    
    /* Pipeline */
    // if we are here, then it should be an executable line or an anonymous block.
    ExecDef* edp;
    shared_ptr<Exec> ep;
    
    // Anonymous block
    if (token == "block")
    {
      BlockDef* bdp = new BlockDef();
      bdp->read(in);
      bdp->anonymous(true);
      
      edp = bdp;
    }
    
    // Executable line
    else
    {
      string exec_def_name = token;
      edp = Definitions::Get(exec_def_name);
      if (edp == 0)
      {
        DEBUG_OUT("  error: Cannot find executable definition named '"); DEBUG_OUT(exec_def_name); DEBUG_OUT("'.  Skipping.\n");
        continue;
      }
    }
    
    ep = read_inst_line(linestream, edp, root_bdp->m_execs, prev_ep);
    root_bdp->m_execs.push_back(ep);
    prev_ep = ep;
  }
  
  m_root_block = static_pointer_cast<Block>(root_bdp->declare());
  DEBUG_OUT("==========  SEQUENCE::READ END  ==========\n");
}

shared_ptr<Exec> VB::read_inst_line(istream& in_line, ExecDef* edp, 
                                        list<shared_ptr<Exec> >& wait_set, shared_ptr<Exec>& prev_ep)
{
  shared_ptr<Exec> ep = edp->declare();
  
    bool does_not_wait = false;
    while (in_line.good())
    {
      string token;
      in_line >> token;
      
      // Exec name
      if (token == "name")
      {
        in_line >> ep->name;
      }
      
      // Exec dataset path
      else if (token == "using")
      {
        in_line >> ep->path;
      }
      
      // Exec waitfors
      else if (token == "waitfor")
      {
        if (does_not_wait)
        {
          DEBUG_OUT("  warning: waitfor/nowait confusion on same line.\n");
        }
        string wait_pattern;
        in_line >> wait_pattern;
        
        // vbforeach shared_ptr<Exec> wait_ep in the list wait_set...
        for (list<shared_ptr<Exec> >::iterator iter = wait_set.begin();
             iter != wait_set.end(); ++iter)
        {
          shared_ptr<Exec>& wait_ep = *iter;
          if (matches(wait_ep->name, wait_pattern))
          {
            ep->depends.push_back(wait_ep);
          }
        }
      }
      else if (token == "nowait")
      {
        does_not_wait = true;
        if (!ep->depends.empty())
        {
          DEBUG_OUT("  warning: waitfor/nowait confusion on same line.\n");
        }
        ep->depends.clear();
      }
      
      // Exec argument setting
      else
      {
        size_t eqpos = token.find("=");
        if (eqpos == string::npos)
        {
          DEBUG_OUT("  error: Unrecognized token -- "); DEBUG_OUT(token); DEBUG_OUT(".  Ignoring.\n");
          continue;
        }
        
        string varname = token.substr(0,eqpos);
        string varval = token.substr(eqpos+1);
        
        if (*(varval.begin()) == '\"')
        {
          varval = varval.substr(1);
          while (*(varval.rbegin()) != '\"')
          {
            if (!in_line.good())
            {
              DEBUG_OUT("  warning: Open quotation with no matching close.\n");
              break;
            }
            varval += in_line.get();
          }
          varval = varval.substr(0,varval.length()-1);
        }
        
        ep->data[varname] = varval;
      }
    }
    
    if (!does_not_wait &&
        ep->depends.empty() &&
        prev_ep)
    {
      ep->depends.push_back(prev_ep);
    }
    
  return ep;
}

void Sequence::write(ostream& out) const
{
  DEBUG_OUT("========== SEQUENCE::WRITE BEGIN ==========\n");
  
  // output the variables
  typedef pair<string,string> name_val_pair;
  vbforeach (const name_val_pair& nv, m_vars)
  {
    out << "set " << nv.first << "=" << nv.second << vb_nl;
  }
  out << vb_endl;
  
  // include any necessary files
  vbforeach(const string& inc, m_includes)
  {
    out << "include " << inc << vb_nl;
  }
  out << vb_endl;
  
  // output the executable definitions
  vbforeach (const ExecDef* edp, m_exec_defs)
  {
    if (!edp->anonymous())
    {
      edp->write(out);
    }
  }
  out << vb_endl;
  
  // output the pipeline (i.e. the main block)
  static_cast<BlockDef*>(m_root_block->def)->write_internals(out);
  out << vb_endl;
  
  DEBUG_OUT("==========  SEQUENCE::WRITE END  ==========\n");
}

Sequence& Sequence::operator=(const Sequence& s)
{
  m_vars = s.m_vars;
  m_root_block = static_pointer_cast<Block>(s.m_root_block->def->declare());
  m_exec_defs = s.m_exec_defs;
  m_includes = s.m_includes;
  
  return *this;
}





/*******************************************************************************
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
 *  === === === === === === === === === === === === === === === === === ===    *
 *     =================================================================       *
 *         All this stuff is used only for exporting and submitting
 *         sequences to disk and to the queue, respectively.  It is
 *         stuff seen only by this file.  Lots of global variables 
 *         and such.  Don't judge me!
 *     =================================================================       *
 *  === === === === === === === === === === === === === === === === === ===    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
 ******************************************************************************/

// ***Global variables***
typedef string FileID;
typedef shared_ptr<Job> JobPtr;
typedef string FileName;
typedef string FileFormat;
struct FileSnapShot
{
  JobPtr      jp;
  FileName    ifn;
  FileFormat  iff;
  FileName    ofn;
  FileFormat  off;
};

map<FileID, list<FileSnapShot> > global_file_info_map;
map<shared_ptr<Exec>, set<shared_ptr<Job> > > final_jobs;
unsigned int max_jnum;

// ***Methods***

extern string my_itoa(int);

typedef shared_ptr<VB::Exec> EP;
typedef shared_ptr<VB::Job> JP;
typedef shared_ptr<VB::Block> BP;

/* build_final_jobs_map
 *
 * Construct the map from each executable instance to the set of jobs that are
 * "final" in that executable.  A job is final if there are no other jobs that
 * have it as a direct or indirect dependency.  The final job map comes in 
 * handy when converting a sequence (in the format of nested executables) to 
 * a VBSequence (in the format of a list of job instances).
 */
void build_final_jobs_map(const shared_ptr<VB::Exec>& ep)
{
  DEBUG_OUT("  Processing final_jobs for exec inst "); DEBUG_OUT(ep->name + "\n");
  set<JP>& fj = final_jobs[ep];
  fj.clear();
  
  // If the executable is a job, then its final job set contains only itself.
  shared_ptr<VB::Job> jp = dynamic_pointer_cast<VB::Job>(ep);
  if (jp)
  {
    fj.insert(jp);
    DEBUG_OUT("  done! (it was a job)\n");
    return;
  }
  
  // If the executable is a block, then the final job set contains the final
  // jobs of all executables in the block that are "final".
  shared_ptr<VB::Block> bp = dynamic_pointer_cast<VB::Block>(ep);
  if (bp)
  {
    EP prev_ep;
    
    for (list<EP>::iterator iter = bp->execs.begin();
         iter != bp->execs.end(); ++iter)
    {
      EP& child_ep = *iter;
      build_final_jobs_map(child_ep);
      
      if (child_ep->depends.empty())
      {
        DEBUG_OUT("    this executable doesn't have any depends.\n");
      }
      else
      {
        for (list<EP>::iterator iter = child_ep->depends.begin();
             iter != child_ep->depends.end(); ++iter)
        {
          EP& wait_ep = *iter;
          DEBUG_OUT("    this executable depends on "); DEBUG_OUT(wait_ep->name + ".\n");
          
          set<JP>& wait_fj = final_jobs[wait_ep];
          for (set<JP>::iterator iter = wait_fj.begin();
               iter != wait_fj.end(); ++iter)
          {
            JP wait_jp = *iter;
            fj.erase(wait_jp);
          }
        }
      }
      
      set<JP>& child_fj = final_jobs[child_ep];
      fj.insert(child_fj.begin(), child_fj.end());
      
      prev_ep = child_ep;
    }
  }
  DEBUG_OUT("  done! (it was a block)\n");
  DEBUG_OUT("  final jobs are ");
  vbforeach (JP fj, final_jobs[ep])
    DEBUG_OUT(fj->name + " ");
  DEBUG_OUT("\n");
      
}

/* calc_job_num
 *
 * Get a unique identifier for a jobspec based on the executable id and the 
 * dataset node id.
 */
int VB::calc_job_num(const shared_ptr<VB::Job>& jp, const VB::DataSet* const dsp)
{
  static map<int,int> job_num_map;
  
  pair<map<int,int>::iterator, bool> insert_value = 
    job_num_map.insert(pair<int,int>(jp->id * 1000 + dsp->get_id(), max_jnum+1));
  
  map<int,int>::iterator jnum_iter = insert_value.first;
  bool was_inserted = insert_value.second;
  
  if (was_inserted) max_jnum = (max_jnum+1)%100000;
  
  return jnum_iter->second;
}

/* common_dataset_root
 */
string common_dataset_root(string path1, string path2)
{
  list<string> node_names1 = split(path1, ":");
  list<string> node_names2 = split(path2, ":");
  
  string common_path = "";
  
  list<string>::iterator iter1 = node_names1.begin();
  list<string>::iterator iter2 = node_names2.begin();
  while (iter1 != node_names1.end() && iter2 != node_names2.end())
  {
    string& node_name1 = *iter1;
    string& node_name2 = *iter2;
    
    if (node_name1 == "")
    {
      ++iter1;
      continue;
    }
    if (node_name2 == "")
    {
      ++iter2;
      continue;
    }
    
    if (!matches(node_name1, node_name2))
    {
      break;
    }
    
    common_path += node_name2;
  }
  
  return common_path;
}

/* common_ancestor
 */
BP common_ancestor(const JP& jp1, const JP& jp2)
{
  set<BP> ancestors1;
  
  BP parent1 = jp1->parent.lock();
  while (parent1)
  {
    ancestors1.insert(parent1);
    parent1 = parent1->parent.lock();
  }
  
  BP parent2 = jp2->parent.lock();
  while (parent2)
  {
    if (ancestors1.find(parent2) != ancestors1.end())
      return parent2;
      
    parent2 = parent2->parent.lock();
  }
  
  return BP();
}

/* get_all_descendant_nodes
 *
 * Return all of the descendant nodes (and the node itself) of a given dataset.
 */
set<DataSet*>& get_all_descendant_nodes(DataSet* dsp)
{
  // node_map is a map from each DataSet to a set of all of its descendents
  static map<DataSet*, set<DataSet*> > node_map;
  
  map<DataSet*, set<DataSet*> >::iterator iter = node_map.find(dsp);
  if (iter == node_map.end())
  {
    iter = node_map.insert(pair<DataSet*, set<DataSet*> >(dsp, set<DataSet*>())).first;
    set<DataSet*>& descendants = iter->second;
    
    // insert each of the children and all the descendants of each of the
    // children.
    
    const list<DataSet*>& children = dsp->get_children();
//    descendants.insert(children.begin(), children.end());
    
    for (list<DataSet*>::const_iterator child_iter = children.begin();
         child_iter != children.end(); ++child_iter)
    {
      DataSet* child_dsp = *child_iter;
      set<DataSet*> child_descendants = get_all_descendant_nodes(child_dsp);
      descendants.insert(child_descendants.begin(), child_descendants.end());
    }
    descendants.insert(dsp);
  }
  
  return iter->second;
}

int fill_file_vars(string &str, const string& infilename, map<string,string> &mymap)
{
  int value = 0;
  
  size_t pos=0;
  while ((pos = str.find("$$", pos)) != string::npos)
  {
    str.replace(pos, 2, infilename);
    pos += infilename.length();
    ++value;
  }
  
  string setext_string = "setextension(";
  if ((pos = str.find(setext_string)) != string::npos)
  {
    string rest = str.substr(pos + setext_string.length());
    
    // trim the closing paren
    size_t paren_pos = rest.find_last_of(")");
    if (paren_pos != string::npos)
      rest = rest.substr(0, paren_pos);
    else 
      DEBUG_OUT("no closing paren for "); DEBUG_OUT(setext_string + "\n");
    
    trim(rest);
    
    // get the new extension
    size_t comma_pos = rest.find_last_of(",");
    string newext = "";
    if (comma_pos != string::npos && comma_pos != rest.length()-1) 
      newext = rest.substr(comma_pos+1);
    else
      DEBUG_OUT("no new extension specified for "); DEBUG_OUT(setext_string + "\n");
    
    trim(newext);
    
    // get the original filename
    string oldfilename = rest.substr(0,comma_pos);
    trim(oldfilename);
    if (oldfilename != "" && 
        oldfilename[0] == '\"' && 
        oldfilename[oldfilename.length()-1] == '\"')
      oldfilename = oldfilename.substr(1,oldfilename.length()-2);
      
    str.replace(pos, str.length()-pos-1, xsetextension(oldfilename, newext));
    ++value;
  }
  
  value += fill_vars2(str, mymap);
  return value;
}

/* create_single_jobspec
 * 
 * Create a VBJobSpec from a job and a dataset (and, if needed, a conversion
 * job).
 */
vector<VBJobSpec> create_single_jobspec(const shared_ptr<VB::Job>& jp, VB::DataSet* dsp)
{
  DEBUG_OUT("========== JOBSPEC CREATE BEGIN ==========\n");
  VBJobSpec js;
  JobType* jtp = static_cast<VB::JobType*>(jp->def);
  shared_ptr<Job> conversion_jp;
  vector<VBJobSpec> js_list;
  
  // Initialize the jobspec 
  js.init();
  js.jobtype = jtp->name();
  js.magnitude = 0;    // set it to something!
  js.name = jp->name;
  js.jnum = calc_job_num(jp, dsp);
  
  // Create a tokenlist with all relevant variables, starting with the dataset
  map<string,string> variables = dsp->to_map();
  DEBUG_OUT("  Created the dataset map...\n");
  typedef pair<string,string> name_val_pair;
  vbforeach (name_val_pair nv, variables)
  {
    DEBUG_OUT("    '"); DEBUG_OUT(nv.first + "' = '" + nv.second + "'\n");
  }
  
  // Add the job data to the variables
  typedef pair<string,string> name_val_pair;
  vbforeach (name_val_pair nv, jp->data)
  {
    string value = nv.second;
    fill_vars2(value, variables);
    variables[nv.first] = value;
  }
  
  // Also add the arguments that have default values to the variables
  vbforeach (JobType::Argument arg, jtp->args())
  {
    if (variables.find(arg.name) == variables.end() &&
        arg.info.find("default") != arg.info.end())
    {
      DEBUG_OUT("  using a default value.\n");
      string value = arg.info["default"];
      fill_vars2(value, variables);
      variables[arg.name] = value;
    }
  }
  
  // Insert file format conversions
  vbforeach (JobType::File f, jtp->files())
  {
    // Check what format the file needs to be in.
    //
    // ::NOTE:: Input formats and filenames should not be so arbitrarily
    //          chosen as choosing the first one.  Think of another way,
    //          perhaps minimizing the number of conflicts along the edges
    //          in the file network.
    string informat = "";
    string infilename = "";
    if (!f.in_types.empty())
    {
      informat = f.in_types.front();
      fill_vars2(informat, variables);
    }
      
    string prevfilename;
    string prevformat;
    shared_ptr<Job> prevjp;
    
    // In order to know whether we need any conversions, we need to check the 
    // most recent use of this file.
    for (list<FileSnapShot>::reverse_iterator iter = global_file_info_map[f.id].rbegin();
         iter != global_file_info_map[f.id].rend(); ++iter)
    {
      if (jp->depends_on(iter->jp))
      {
        prevjp = iter->jp;
        prevfilename = iter->ofn;
        prevformat = iter->off;
        break;
      }
    }
    
    // If the out format of the last job that dealt with this file is
    // different from the in format that this job needs the file in, 
    // then we need a conversion.
    if (informat != "" && informat != prevformat)
    {
      conversion_jp = static_pointer_cast<VB::Job>(VB::Definitions::Get("x_vbconv")->declare());
      
      conversion_jp->path = jp->path;
      conversion_jp->depends.push_back(prevjp);
      jp->depends.push_back(conversion_jp);
      
      infilename = prevfilename + "." + my_itoa(calc_job_num(jp,dsp)) + "." + informat;
      conversion_jp->data["infile"] = prevfilename + "[" + prevformat + "]";
      conversion_jp->data["outfile"] = infilename + "[" + informat + "]";
    }
    
    // If no informat is specified, then we assume that the job can handle
    // whatever format is thrown at it and no conversion is necessary.
    else
    {
      infilename = prevfilename;
      informat = prevformat;
    }
    
    // Add the file's global variables to job variables.
    for (map<string,string>::iterator iter = f.global_vars.begin(); 
         iter != f.global_vars.end();++iter)
    {
      pair<const string,string>& key_val = *iter;
      
      string value = key_val.second;
      fill_file_vars(value, infilename, variables);
      variables[key_val.first] = value;
    }
    
    // The output name of the file depends on the format (though it will 
    // check the default case when no format is matched).
    //
    // Also, add the file's extension-dependent variables to the job variables.
    //
    // ::NOTE:: Output formats and filenames should not necessarily be
    //          determined according to the input file information.  Think
    //          of another way.  Possibly by reducing the number of conflicts
    //          along the edges in the file network.
    string outfilename;
    string outformat;
    
    if (f.out_names.find(informat) != f.out_names.end())
    {
      outfilename = f.out_names[informat];
      outformat = informat;
      
      for (map<string,string>::iterator iter = f.vars[informat].begin(); 
           iter != f.vars[informat].end(); ++iter)
      {
        pair<const string,string>& key_val = *iter;
        
        string value = key_val.second;
        fill_file_vars(value, infilename, variables);
        variables[key_val.first] = value;
      }
    }
    else if (f.out_names.find("") != f.out_names.end())
    {
      outfilename = f.out_names[""];
      outformat = informat;
      
      for (map<string,string>::iterator iter = f.vars[""].begin(); 
           iter != f.vars[""].end(); ++iter)
      {
        pair<const string,string>& key_val = *iter;
        
        string value = key_val.second;
        fill_file_vars(value, infilename, variables);
        variables[key_val.first] = value;
      }
    }
    else if (!f.out_names.empty())
    {
      outfilename = f.out_names.begin()->second;
      outformat = f.out_names.begin()->first;
      
      for (map<string,string>::iterator iter = f.vars[outformat].begin(); 
           iter != f.vars[outformat].end(); ++iter)
      {
        pair<const string,string>& key_val = *iter;
        
        string value = key_val.second;
        fill_file_vars(value, infilename, variables);
        variables[key_val.first] = value;
      }
    }
    else
    {
      outfilename = infilename;
      outformat = informat;
      
      for (map<string,string>::iterator iter = f.vars[informat].begin(); 
           iter != f.vars[informat].end(); ++iter)
      {
        pair<const string,string>& key_val = *iter;
        
        string value = key_val.second;
        fill_file_vars(value, infilename, variables);
        variables[key_val.first] = value;
      }
    }
    
    // Get the global and format-specific variables.
    vbforeach (name_val_pair nv, f.global_vars)
    {
      string value = nv.second;
      fill_vars2(value, variables);
      variables[nv.first] = value;
    }
    
    if (f.vars.find(outformat) != f.vars.end())
    {
      vbforeach (name_val_pair nv, f.vars[outformat])
      {
        string value = nv.second;
        fill_vars2(value, variables);
        variables[nv.first] = value;
      }
    }
    
    // Take a file snapshot and put it in the global file information map.
    FileSnapShot fss;
    
    fill_file_vars(outfilename, infilename, variables);
    fill_file_vars(outformat, infilename, variables);
    
    fss.jp = jp;
    fss.ifn = infilename;
    fss.iff = informat;
    fss.ofn = outfilename;
    fss.off = outformat;
    
    global_file_info_map[f.id].push_back(fss);
  }
  
  // Set the arguments to the VBJobSpec
  vbforeach (JobType::Argument arg, jtp->args())
  {
    string value;
    if (variables.find(arg.name) != variables.end())
    {
      value = variables[arg.name];
      fill_vars2(value, variables);
      js.arguments[arg.name]=value;
      if (arg.name == "DIR") js.dirname = value,js.logdir=value+"/logs";
    }
    else
    {
      DEBUG_OUT("  error: argument "); DEBUG_OUT(arg.name); DEBUG_OUT(" has no value.\n");
    }
  }
  
  /* Set the waitfors of the jobspec. */
  
  // If this is the first job or a nowait job, then it should wait for the 
  // waitfors of its parent block.
  if (jp->depends.empty())
  {
    BP parent_bp = jp->parent.lock();
    while (parent_bp)
    {
      jp->depends.insert(jp->depends.end(), parent_bp->depends.begin(), parent_bp->depends.end());
      if (!jp->depends.empty())
        break;
      
      parent_bp = parent_bp->parent.lock();
    }
  }
  
  // iterated_nodes is a map from each job pointer to the complete set of
  // dataset nodes over which it iterates
  map<JP, set<DataSet*> > iterated_nodes;
  
  // for each EP wait_ep in the list jp->depends
  for (list<EP>::iterator iter = jp->depends.begin();
       iter != jp->depends.end(); ++iter)
  {
    EP& wait_ep = *iter;
    
    // Here, we're going to go through each job in the final_jobs set for this
    // executable.  Remember, the final_jobs set is the set of all jobs in a
    // given block on which no other jobs depend directly.  When converted to 
    // JobSpecs, any executables that wait for this one (wait_ep) will actually
    // wait for the jobs in wait_ep's final_jobs set.
    for (set<shared_ptr<Job> >::const_iterator iter = final_jobs[wait_ep].begin();
         iter != final_jobs[wait_ep].end(); ++iter)
    {
      shared_ptr<Job> final_jp = *iter;
      
      // get the iterated_nodes of this final_jp
      string wait_path = final_jp->full_path();
      if (iterated_nodes.find(final_jp) == iterated_nodes.end())
      {
        iterated_nodes[final_jp].clear();
        list<DataSet*> final_iterated_nodes = dsp->get_root()->get_children(wait_path);
        iterated_nodes[final_jp].insert(final_iterated_nodes.begin(), final_iterated_nodes.end());
      }
      
      DEBUG_OUT("    The ID of this job is "); DEBUG_OUT(calc_job_num(jp, dsp)); DEBUG_OUT("\n");
      DEBUG_OUT("    The job waits for jobs that iterate over: DataSet"); DEBUG_OUT(wait_path + "\n");
      
      BP common_ancestor_block = common_ancestor(jp, final_jp);
      // We have to find the common ancestor block of jp and final_jp (which jp
      // waits for) becuase within a block, jobs should wait for corresponding
      // instances of their dependencies.
      
      string ancestor_path = "";
      if (common_ancestor_block) ancestor_path = common_ancestor_block->full_path();
      else DEBUG_OUT("error: jobs do not have a common ancestor; must not be from the same sequence.\n");
      string ds_path = dsp->get_long_name(); if (ds_path.substr(0,7) == "DataSet") ds_path = ds_path.substr(7);
      list<string> ancestor_path_list = split(ancestor_path, ":");
      list<string> ds_path_list = split(ds_path, ":");
      
      DEBUG_OUT("    The path of the current dataset node is DataSet"); DEBUG_OUT(ds_path + "\n");
      DEBUG_OUT("    The path of the nearest common ancestor's node is DataSet"); DEBUG_OUT(ancestor_path + "\n");
      
      DataSet* specific_ancestor_node = dsp->get_root();
      DEBUG_OUT("    The root node of this dataset is "); DEBUG_OUT(dsp->get_root()->get_long_name() + "\n");
      while (!ds_path_list.empty() && !ancestor_path_list.empty())
      {
        string ancestor_node_name = ancestor_path_list.front();
        string ds_node_name = ds_path_list.front();
        if (ancestor_node_name == "")
        {
          ancestor_path_list.pop_front();
          continue;
        }
        if (ds_node_name == "")
        {
          ds_path_list.pop_front();
          continue;
        }
        
        if (!matches(ds_node_name, ancestor_node_name))
          break;
        
        DEBUG_OUT("      Getting node child "); DEBUG_OUT(ds_node_name + "\n");
        specific_ancestor_node = specific_ancestor_node->get_child(ds_node_name);
        
        ds_path_list.pop_front();
        ancestor_path_list.pop_front();
      }
      
      DEBUG_OUT("    The specific path of the ancestor node is "); DEBUG_OUT(specific_ancestor_node->get_long_name() + "\n");
      
      set<DataSet*>& all_descendants = get_all_descendant_nodes(specific_ancestor_node);
      set<DataSet*>& all_wait_iterated_nodes = iterated_nodes[final_jp];
      set<DataSet*> wait_children;
      
      // ::NOTE:: Not sure if this should be the case, but if the two executables
      //          have the same dataset node path and the same parent block,
      //          then only wait for corresponding jobspecs.
      if (final_jp->parent.lock() == jp->parent.lock() &&
          final_jp->path == jp->path)
      {
        wait_children.insert(dsp);
      }
      else
      {
        set_intersection(all_descendants.begin(), all_descendants.end(),
                         all_wait_iterated_nodes.begin(), all_wait_iterated_nodes.end(),
                         inserter(wait_children, wait_children.begin()));
      }
      
      for (set<DataSet*>::iterator iter = wait_children.begin();
           iter != wait_children.end(); ++iter)
      {
        DataSet* wait_dsp = *iter;
        js.waitfor.insert(calc_job_num(final_jp, wait_dsp));
        DEBUG_OUT("      This job waits for "); DEBUG_OUT(calc_job_num(final_jp, wait_dsp)); DEBUG_OUT("\n");
      }
      DEBUG_OUT("\n");
    }
    // wait for the final jobs according to the same rules as above
  }
  
  js_list.push_back(js);
  if (conversion_jp)
  {
    VBJobSpec conversion_js = create_single_jobspec(conversion_jp, dsp).front();
    js_list.push_back(conversion_js);
  }

  DEBUG_OUT("==========  JOBSPEC CREATE END  ==========\n");
  return js_list;
}

/* create_jobspecs
 *
 * Generates a list of VBJobSpecs from Job and a DataSet
 */
vector<VBJobSpec> create_jobspecs(const shared_ptr<VB::Job>& jp, VB::DataSet* dsp)
{
//  list<DataSet*> datasets = dsp->get_children(jp->path);
  vector<VBJobSpec> jobspecs;
  
  // ::NOTE:: I'd love to do the file-format conversion stuff here, but it has
  //          may be instances where the file name and such depend on some
  //          dataset variable that's in the children.  So I can't be certain
  //          that the file info will be able to fully resolve at this point.
  
/*  if (datasets.empty()) 
  {
    DEBUG_OUT("warning: no datasets found for the job ");
    DEBUG_OUT(jp->name); DEBUG_OUT(" ("); DEBUG_OUT(jp->full_path());DEBUG_OUT(")\n"); 
    dsp->spit_tree_to_stdout();
  }
  vbforeach (DataSet* dspi, datasets)
  {*/
    // create_single_jobspec will return the jobspec and the conversion, if
    // a conversion is necessary.
    vector<VBJobSpec> js_and_conv = create_single_jobspec(jp, dsp);
    vbforeach (VBJobSpec& js, js_and_conv)
    {
      jobspecs.push_back(js);
    }
/*  }
  if (jobspecs.empty())
  {
    DEBUG_OUT("warning: no jobspecs created for the job ");
    DEBUG_OUT(jp->name); DEBUG_OUT("\n"); 
  }*/
  
  return jobspecs;
}

/* process_block
 *
 * converts a block to a list of VBJobSpec structures.
 */
vector<VBJobSpec> process_block(const shared_ptr<VB::Block>& this_bp, VB::DataSet* ds, set<shared_ptr<VB::Job> >& prev_jobs)
{
  vector<VBJobSpec> jobspecs;
  if (ds)
  {
    DEBUG_OUT("  Begin processing the block '"); DEBUG_OUT(this_bp->name + "'\n");
  //  DataSet my_ds = ds;
    
    set<JP> local_prev_jobs = prev_jobs;
    
    // Go through each executable instance in the block and process it in turn.
    vbforeach (EP ep, this_bp->execs)
    {
      list<DataSet*> datasets = ds->get_children(ep->path);
       BlockDef* bdp = dynamic_cast<BlockDef*>(ep->def);
       JP jp = dynamic_pointer_cast<Job>(ep);
      
      vbforeach (DataSet* dspi, datasets)
      {
        
        // If the instance is a block, it needs to be unfolded as well.  So here's
        // a bit of recursion for you.
        if (bdp)
        {
          vector<VBJobSpec> block_jobspecs = process_block(static_pointer_cast<Block>(ep), dspi, local_prev_jobs);
          jobspecs.insert(jobspecs.end(), block_jobspecs.begin(), block_jobspecs.end());
          continue;
        }
        
        // If the instance is a job, then add it to the list of final jobs and 
        // remove its dependencies from the list.
        if (jp)
        {
          // file conversion and jobspec translation step
          vector<VBJobSpec> job_jobspecs = create_jobspecs(jp, dspi);
          jobspecs.insert(jobspecs.end(), job_jobspecs.begin(), job_jobspecs.end());
          
          local_prev_jobs.clear();
          local_prev_jobs.insert(jp);
          continue;
        }
        
        // Otherwise, we have a problem.
        DEBUG_OUT("ok, this exec instance appears to neither be a block or a job.\n");
      }
    }
    
    prev_jobs = final_jobs[static_pointer_cast<Exec>(this_bp)];
    
    DEBUG_OUT("  End processing the block '"); DEBUG_OUT(this_bp->name + "' (returning VBJobSpecs)\n");
  
  }
  else
  {
    DEBUG_OUT("  Encountered a bogus dataset\n");
  }
  return jobspecs;
}

/* jobspec_depends_on
 *
 * Does the VBJobSpec identified by the job number jnum1 depend on that 
 * identified by jnum2?  Uses the maps for some dynamic programming.
 */
bool jobspec_depends_on(int jnum1, int jnum2, VBSequence& seq, 
                        map<int, VBJobSpec>& spec_map, 
                        map<pair<int,int>, bool>& depend_map)
{
  pair<int,int> jnum_pair(jnum1,jnum2);
  
  // Make sure the job is in the spec_map
  if (spec_map.find(jnum1) != spec_map.end())
  {
    for (SMI jsi = seq.specmap.begin(); jsi != seq.specmap.end(); ++jsi)
    {
      if (jsi->second.jnum == jnum1)
      {
        spec_map[jnum1] = jsi->second;
        break;
      }
    }
  }
  VBJobSpec js = spec_map[jnum1];
  
  // If it's already figured out, then whatever.
  if (depend_map.find(jnum_pair) != depend_map.end())
    ;
  
  // If it's a direct dependency, then yes.
  else if (find(js.waitfor.begin(), js.waitfor.end(), jnum2) != js.waitfor.end())
    depend_map[jnum_pair] = true;
  
  // If they're equal, then yes (arbitrarily).
  else if (jnum1 == jnum2)
    depend_map[jnum_pair] = true;
  
  // If we just don't know, then assume no and check for dependency of waitfors.
  else
  {
    depend_map[jnum_pair] = false;
    vbforeach(int32 ww,js.waitfor) {
      if (jobspec_depends_on(ww, jnum2, seq, spec_map, depend_map)) {
        depend_map[jnum_pair] = true;
        break;
      }
    }
  }
  
  return depend_map[jnum_pair];
}

/* get_initial_jobs
 */
list<shared_ptr<Job> > get_initial_jobs(shared_ptr<Exec> e)
{
  list<shared_ptr<Job> > jlist;
  
  if (shared_ptr<Job> j = dynamic_pointer_cast<Job>(e))
  {
    jlist.push_back(j);
  }
  
  else if (shared_ptr<Block> b = dynamic_pointer_cast<Block>(e))
  {
    for (list<shared_ptr<Exec> >::iterator iter = b->execs.begin(); 
         iter != b->execs.end(); ++iter)
    {
      shared_ptr<Exec>& sube = *iter;
      list<shared_ptr<Job> > subjlist = get_initial_jobs(sube);
      jlist.insert(jlist.end(), subjlist.begin(), subjlist.end());
    }
  }
  
  return jlist;
}

/* do_output
 * 
 * Essentially creates a VBSequence from a Sequence.  Purposefully takes the
 * sequence by value and not by reference; we don't want to be modifying the 
 * original, but we can't have this one be const.  It's just a throw-away.
 */
void VB::do_output(const VB::Sequence& my_seq, VB::DataSet my_ds, VBSequence& seq, 
		   ExecPoint beginpoint,ExecPoint endpoint)
{
  DEBUG_OUT("========== BEGIN DO_OUTPUT OF SEQUENCE ==========\n");
  
  max_jnum = 0;
  
  // Put the Sequence meta data into the dataset
  typedef pair<string,string> name_val_pair;
  vbforeach (name_val_pair nvp, my_seq.vars())
  {
    my_ds.insert_member(nvp.first, true)->value = nvp.second;
  }
  
  map<string,string> vars = my_ds.to_map();
  
  // Initialize the VBSequence
  seq.init();
  seq.priority = atoi(my_seq.priority().c_str());
  seq.email = "";
  seq.name = my_seq.name();
  if (my_seq.vars().find("DIR") != my_seq.vars().end())
    seq.seqdir = my_seq.vars().find("DIR")->second;
  fill_vars2(seq.name, vars);
  fill_vars2(seq.email, vars);
  fill_vars2(seq.seqdir, vars);
  
  DataSet* temp_ds = new DataSet(my_ds);
  
  // Push the sequence variables into the dataset
  typedef pair<string,string> name_val_pair;
  vbforeach (name_val_pair nv, my_seq.vars())
  {
    if (!temp_ds->get_member(nv.first))
      temp_ds->insert_member(nv.first)->value = nv.second;
  }
  
  set<JP> last_set;
  build_final_jobs_map(my_seq.root_block());
  vector<VBJobSpec> tmpj=process_block(my_seq.root_block(), temp_ds, last_set);
  vbforeach (VBJobSpec &js,tmpj)
    seq.specmap[js.jnum]=js;
  delete temp_ds;
  
  // Now prune according to the checkpoints
  //
  // ::NOTE:: The following two maps serve only the purpose of ensuring that the
  //          jobspec_depends_on function doesn't take absolutely forever (on
  //          really large sequences).  That's a good purpose though :)
  map<int, VBJobSpec> jobnum_map;
  map<pair<int,int>, bool> jobspec_depend_map;
  set<int> jnums_to_keep;
  set<int> jnums_to_remove;
  
  if (beginpoint.first && beginpoint.second)
  {
    list<shared_ptr<Job> > begin_job_list = get_initial_jobs(beginpoint.first);
    DataSet* ds = beginpoint.second;
    for (SMI iter = seq.specmap.begin(); iter != seq.specmap.end(); ++iter)
    {
      VBJobSpec& js = iter->second;
      bool in_set = false;
      for (list<shared_ptr<Job> >::iterator ji = begin_job_list.begin(); ji != begin_job_list.end(); ++ji)
      {
        int begin_jnum = calc_job_num(*ji, ds);
        if (jobspec_depends_on(js.jnum, begin_jnum, seq, jobnum_map, jobspec_depend_map))
        {
          in_set = true;
          break;
        }
      }
      if (!in_set)
        jnums_to_remove.insert(js.jnum);
    }
  }
  
  if (endpoint.first && endpoint.second)
  {
    list<shared_ptr<Job> > end_job_list = get_initial_jobs(endpoint.first);
    DataSet* ds = endpoint.second;
    for (SMI iter = seq.specmap.begin(); iter != seq.specmap.end(); ++iter)
    {
      VBJobSpec& js = iter->second;
      bool in_set = false;
      for (list<shared_ptr<Job> >::iterator ji = end_job_list.begin(); ji != end_job_list.end(); ++ji)
      {
        int end_jnum = calc_job_num(*ji, ds);
        if (jobspec_depends_on(end_jnum, js.jnum, seq, jobnum_map, jobspec_depend_map))
        {
          in_set = true;
          break;
        }
      }
      if (!in_set)
        jnums_to_remove.insert(js.jnum);
    }
  }
  
  for (SMI iter = seq.specmap.begin(); iter != seq.specmap.end(); ++iter)
  {
    VBJobSpec& js = iter->second;
    if (jnums_to_remove.find(js.jnum) != jnums_to_remove.end())
      seq.specmap.erase(iter->second.jnum);
      
    else
    {
      vbforeach(int32 ww,js.waitfor) {
        if (jnums_to_remove.find(ww) != jnums_to_remove.end())
          js.waitfor.erase(ww);
      }
    }
  }
  
  DEBUG_OUT("==========  END DO_OUTPUT OF SEQUENCE  ==========\n");
}

// FIXME following two methods not too helpful

void Sequence::export_to_disk(const VB::DataSet&, const string&, 
                              ExecPoint, ExecPoint) const
{
//   // In case multiple sequences are being exported during the same process...
//   static int num = -1;
  
//   VBSequence seq;
  
//   // do_output(*this, ds, seq, beginpoint, endpoint);
//   seq.seqnum=getpid();

//   ofstream ofile;
//   string ofilename;
  
//   stringstream ofnamestream;
//   ofnamestream << dir << "/seq" << (long)seq.seqnum;
//   if (++num) ofnamestream << "." << num;
//   string fulldir = ofnamestream.str();
//   /*return*/ seq.Write(fulldir);
}

void Sequence::submit(const VB::DataSet&, 
                      ExecPoint,
                      ExecPoint) const
{
//   VBSequence seq;
//   do_output(*this, ds, seq);
//   /*return*/ seq.Submit();
}
