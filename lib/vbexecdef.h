
// vbexecdef.h
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

#ifndef VBEXECDEF_H
#define VBEXECDEF_H

#include <vector>
#include <list>
#include <string>
#include <set>
#include <map>
#include <tr1/memory>

#include <istream>
#include <ostream>

#include "vbsequence.h"
#include "vbscripttools.h"
#include "vbjobspec.h"

using std::list;
using std::vector;
using std::string;
using std::set;
using std::map;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::istream;
using std::ostream;

namespace VB
{
	/*
	 * ExecDef
	 * =======
	 *
	 * An ExecDef is the definitional object of an executable sequence entity 
	 * instance (i.e., a block or a job).
	 */
	class ExecDef
	{
		protected:
			ExecDef();
			virtual ~ExecDef();
			
		public:
			string name() const;
			void name(const string&);
			
			string description() const;
			void description(const string&);
			
			unsigned count() const;
			virtual bool anonymous() const = 0;
			
			virtual void read(istream&) = 0;
			virtual void write(ostream&) const = 0;
			
			virtual shared_ptr<Exec> declare();
			virtual shared_ptr<Exec> declare_with_parent(const shared_ptr<Block> parent = shared_ptr<Block>()) = 0;
			
			virtual void sync_to_instance(shared_ptr<Exec>);
			virtual void update_instance(shared_ptr<Exec>) const;
			virtual void update_instances() const;
			
		protected:
			string m_name;
			string m_desc;
			unsigned int m_count;
			typedef map<shared_ptr<Exec>, unsigned int> NameCountMap;
			typedef pair<shared_ptr<Exec>, unsigned int> NameCountPair;
			NameCountMap m_count_in;
			
			map<string,string> info;
			
			void declare_helper(shared_ptr<Exec>);
			
			mutable set<weak_ptr<Exec> > instances;
	};
	
	/*
	 * JobType
	 * =======
	 *
	 * A JobType is a specific type of executable entity definition that defines 
	 * jobs.
	 */
	class JobType : public ExecDef
	{
		public:
			JobType();
			JobType(const VBJobType&);
			virtual ~JobType();
			
			struct Argument;
			struct File;
			struct Command;
			
			const list<Argument>& args() const;
			const list<File>& files() const;
			const vector<Command>& cmds() const;
			
			list<Argument>& args();
			list<File>& files();
			vector<Command>& cmds();
			
			virtual bool anonymous() const;
			
			virtual void read(istream&);
			virtual void write(ostream&) const;
      friend istream& operator>>(istream&, JobType::Argument&);
      friend istream& operator>>(istream&, JobType::File&);
      
			// Use the declare method to obtain a pointer to a Job.
			virtual shared_ptr<Exec> declare_with_parent(const shared_ptr<Block> parent = shared_ptr<Block>());
		
		protected:
			list<Argument> m_args;
			list<File> m_files;
			vector<Command> m_cmds;
			
			void init_working_dir_arg();
	};
	
	struct JobType::Argument
	{
		string name;
		map<string,string> info;
	};
	
	struct JobType::File
	{
		string id;
		string description;
		string in_name;
		list<string> in_types;
		map<string,string> out_names;
		map<string,string> global_vars;
		map<string, map<string,string> > vars;
		map<string,string> info;
	};
	
	struct JobType::Command
	{
		string command;
		vector<string> script;
	};
	
	/*
	 * BlockDef
	 * ========
	 *
	 * A BlockDef is a specific type of executable object definition that defines 
	 * blocks.
	 */
	class BlockDef : public ExecDef
	{
		friend class Sequence;
		
		public:
			BlockDef();
			virtual ~BlockDef();
			
			const list<shared_ptr<Exec> >& execs() const;
			list<shared_ptr<Exec> >& execs();
			virtual bool anonymous() const;
			void anonymous(bool anon);
			
			virtual void read(istream&);
			virtual void write(ostream&) const;
			
			// Use the declare method to obtain a pointer to a Block.
			virtual shared_ptr<Exec> declare_with_parent(const shared_ptr<Block> parent = shared_ptr<Block>());
			
			virtual void sync_to_instance(shared_ptr<Exec>);
			virtual void update_instance(shared_ptr<Exec>) const;
			
		protected:
			list<shared_ptr<Exec> > m_execs;
			bool m_anonymous;
			
			void write_internals(ostream& out) const;
	};
	
	/*
	 * Definitions
	 * ===========
	 *
	 * The Definitions class manages all of the executable definitions that are
	 * ever created.
	 */
	class Definitions
	{
		friend class ExecDef;
		friend class JobType;
		friend class BlockDef;
		
		public:
			static void Init();
			static void Import_JobType_Folder();
			static ExecDef* Get(const string&);
			
			static const set<ExecDef*>& defs();
			static const set<JobType*>& jts();
			static const set<BlockDef*>& bds();
		
		protected:
			static set<ExecDef*> m_defs;
			static set<JobType*> m_jts;
			static set<BlockDef*> m_bds;
	};
	
} // namespace VB
	
#endif // VBEXECDEF_H
