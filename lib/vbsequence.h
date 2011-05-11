
// vbsequence.h
// 
// Copyright (c) 1998-2007 by The VoxBo Development Team

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

#ifndef VBSEQUENCE_H
#define VBSEQUENCE_H

#include <vector>
#include <list>
#include <string>
#include <set>
#include <map>
#include <tr1/memory>
#include <boost/foreach.hpp>

#include <istream>
#include <ostream>

#include "vbscripttools.h"
#include "vbexecdef.h"

using std::list;
using std::vector;
using std::string;
using std::set;
using std::map;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::istream;
using std::ostream;

#ifndef vbforeach
#define vbforeach BOOST_FOREACH
#endif

#define max(a,b) ((a)>(b) ? (a) : (b))

namespace VB
{
  /*
   * Exec
   * ========
   * 
   * Use the ExecDef::declare() method to create an Exec.  
   * ExecDef::declare() will create a shared pointer to a new insatnce.
   */
  struct Exec
  {
    friend class ExecDef;
    
    virtual ~Exec() {}
    
    string name;
    string path;
    map<string,string> data;
    list<shared_ptr<Exec> > depends;
    weak_ptr<Block> parent;
    ExecDef* def;
    
    string instance_line() const;
    list<shared_ptr<Job> > last_jobs();
    bool depends_on(const shared_ptr<Exec>&) const;
    string full_path() const;
    
    virtual bool contains(const shared_ptr<Exec>&) const = 0;
    
    const unsigned id;
    static unsigned GLOBAL_EXEC_ID;
    
    protected:
      Exec(shared_ptr<Block> parent = shared_ptr<Block>());
  };
  
  struct ExecNet;
  struct ExecNode;
  typedef pair<shared_ptr<Exec>, DataSet*> ExecPoint;
  
  /*
   * Job
   * ===
   *
   * A Job is the smallest unit of executable instance.  All pipelines are
   * in essence a network of many jobs.  Use JobType::declare() to create a
   * shared pointer to a new job.
   */
  struct Job : public Exec
  {
    friend class ExecDef;
    friend class JobType;
    
    virtual ~Job() {}
    
    virtual bool contains(const shared_ptr<Exec>&) const;
    
    protected:
      Job(shared_ptr<Block> parent = shared_ptr<Block>());
  };
  
  /*
   * Block
   * =====
   *
   * A Block is a black-box style composite executable instance.  It contains
   * pointers to the executable instances that make it up.  Use 
   * BlockDef::declare() to create a shared pointer to a new block.
   */
  struct Block : public Exec
  {
    friend class ExecDef;
    friend class BlockDef;
    
    virtual ~Block() {}
    
    // ::NOTE:: Cycles in blocks (e.g. BlockA has a BlockB ... has a BlockA)
    //          would be bad news.  Need to check for them at some point.
    list<shared_ptr<Exec> > execs;
    
    virtual bool contains(const shared_ptr<Exec>&) const;
    
    protected:
      Block(shared_ptr<Block> parent = shared_ptr<Block>());
  };
  
  shared_ptr<Exec> read_inst_line(istream&, ExecDef* def, 
                                      list<shared_ptr<Exec> >& wait_set, 
                                      shared_ptr<Exec>& prev_inst);
  int calc_job_num(const shared_ptr<Job>&, const DataSet* const);
  
  /*
   * Sequence
   * ========
   *
   * A Sequence contains several sections: 
   *  - a variables section defining all of the sequence-specific variables, 
   *  - a library section for any sequence-specific executable definitions,
   *  - a pipeline section, which is essentially a block definition.
   */
  class Sequence
  {
    public:
      Sequence(BlockDef* root_def = 0);
      ~Sequence();
      
      const shared_ptr<Block>& root_block() const;
      void root_block(shared_ptr<Block>&);
      void root_block_def(BlockDef*);
      
      const map<string,string>& vars() const;
      map<string,string>& vars();
      
      const string& name() const;
      void name(const string& n);
      
      const string& priority() const;
      void priority(const string& p);
      
      bool execs_are_sorted() const;
      std::list<shared_ptr<Exec> >& sort_execs();
      void build_depth_table(std::map<shared_ptr<Exec>, int>* table);
      void build_depth_table_helper(shared_ptr<Exec> exec, std::map<shared_ptr<Exec>, int>* table);
            
      void read(istream&);
      void write(ostream&) const;
      
      void export_to_disk(const VB::DataSet&, const string& dirname, 
                          ExecPoint beginpoint = ExecPoint(shared_ptr<Exec>(),0),
                          ExecPoint endpoint = ExecPoint(shared_ptr<Exec>(),0)) const;
      void submit(const VB::DataSet&, 
                  ExecPoint beginpoint = ExecPoint(shared_ptr<Exec>(),0),
                  ExecPoint endpoint = ExecPoint(shared_ptr<Exec>(),0)) const;
      
      // For beginning execpoints, there are ___ options:
      // - in starting from an execpoint, either assume that dependencies are 
      //   satisfied or run them.
      // - when a node depends on a branch from the start point _and_ another
      //   branch, either run the dependencies along the other branch, or 
      //   assume they are fulfilled.
      
      // Be able to mark certain exec points as unsatisfied.  In other words:
      // - in starting from an execpoint, assume that dependencies are 
      //   satisfied (because everything is initially satisfied).
      // - when a node depends on a branch from the start point _and_ another
      //   branch, assume the dependencies on the second branch are filled.
      
      Sequence& operator=(const Sequence& s);
      
      
    protected:
      map<string,string> m_vars;
      
      shared_ptr<Block> m_root_block;
      
      // ::NOTE:: At the time being (2007.06.11), we are only allowing block
      //          definitions in sequence files.  Should job types be allowed
      //          also?  Not a stretch of my imagination.
      //                                                             - mjumbe
      list<ExecDef*> m_exec_defs;
      list<string> m_includes;
      
  };
  
/*  struct ExecNode
  {
    ExecPoint ep;
    bool validated;
    list<ExecNode*> out_to;
    list<ExecNode*> in_from;
    
    bool directly_in_from(ExecNode* en_from) {
      return (find(en_from, in_from.begin(), in_from.end()) != in_from.end());
    }
    
    bool has_in_from(ExecNode* en_from) {
      bool found = directly_in_from(en_from);
      for (list<ExecNode*>::iterator iter = in_from.begin(); !found && iter != in_from.end(); ++iter) {
        ExecNode*& en = *iter;
        found = en->has_in_from(en_from);
      }
      return found;
    }
  };
  
  struct ExecNet
  {
    list<ExecNode*> nodes;
    
    void clear() {
      for (list<ExecNode*>::iterator iter = nodes.begin();  iter != nodes.end(); ++iter) {
        ExecNode* en = *iter;
        delete en;
      }
      nodes.clear();
    }
    
    void validate_all() {
      for (list<ExecNode*>::iterator iter = nodes.begin();  iter != nodes.end(); ++iter) {
        ExecNode* en = *iter;
        en->validated = true;
      }
    }
    
    void invalidate(ExecNode* en_from)
    {
      en_from.validated = false;
      for (list<ExecNode*>::iterator iter = nodes.begin();  iter != nodes.end(); ++iter) {
        ExecNode* en = *iter;
        if (en->directly_in_from(en_from)) {
          invalidate(en);
        }
      }
    }
  };*/

void do_output(const VB::Sequence& my_seq, VB::DataSet my_ds, VBSequence& seq, 
               ExecPoint beginpoint = ExecPoint(shared_ptr<Exec>(),0),
               ExecPoint endpoint = ExecPoint(shared_ptr<Exec>(),0));





} // namespace VB
  
#endif // VBSEQUENCE_H
