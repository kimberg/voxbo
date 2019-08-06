
// submit_sequence.cpp
// Copyright (c) 2006-2010 by The VoxBo Development Team

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

#include <vbdataset.h>
#include <vbsequence.h>
#include "vbx.h"

#include <algorithm>
#include <fstream>
#include <list>
#include <set>
#include <string>

using namespace std;

VBPrefs vbp;

void submit_sequence_help();
void submit_sequence_version();

int main(int argc, char* argv[]) {
  vbp.init();
  vbp.read_jobtypes();
  VB::Definitions::Import_JobType_Folder();
  bool submit = 1;
  bool combineflag = 0;
  string seq_arg = "";
  VBpri pp;
  list<string> ds_args;
  string odir_arg;
  string member_arg, temp_member_arg;
  list<VB::DataSet::Member*> cmd_line_members;

  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h" || args[i] == "--help") {
      submit_sequence_help();
      exit(0);
    }
    if (args[i] == "-v" || args[i] == "--version") {
      submit_sequence_version();
      exit(0);
    }
    if (args[i] == "-s" && i < args.size() - 1)
      seq_arg = args[++i];
    else if (args[i] == "-d" && i < args.size() - 1)
      ds_args.push_back(args[++i]);
    else if (args[i] == "-o" && i < args.size() - 1) {
      odir_arg = args[++i];
      submit = 0;
    } else if (args[i] == "-p" && i < args.size() - 1) {
      if (pp.set(args[++i])) {
        cerr << "[E] error in scheduling arguments" << endl;
        submit_sequence_help();
        return 1;
      }
    } else if (args[i] == "-c")
      combineflag = 1;
    else if (args[0].find("=") != string::npos) {
      member_arg = args[0];
      VB::DataSet::Member* mem = new VB::DataSet::Member;
      int eq_pos = member_arg.find("=");
      mem->name = member_arg.substr(0, eq_pos);
      mem->value = member_arg.substr(eq_pos + 1);
      VB::trim(mem->value, "\"");
      cmd_line_members.push_back(mem);
    } else {
      submit_sequence_help();
      exit(1);
    }
  }

  if (seq_arg == "" || ds_args.empty()) {
    submit_sequence_help();
    return 1;
  }

  // Construct the sequence (first argument)
  VB::Sequence* seq(new VB::Sequence());
  list<VB::DataSet*> ds_list;

  ifstream seq_file;
  seq_file.open(seq_arg.c_str());
  seq->read(seq_file);
  seq_file.close();

  vbforeach(string ds_arg, ds_args) {
    // Construct the dataset (second argument)
    size_t colpos = ds_arg.find(":");
    if (colpos == string::npos)  // Come here if there is no colon on the line.
    {
      ds_list.push_back(new VB::DataSet(ds_arg));
    } else  // Come here if there is a colon (i.e. a child dataset is
            // specified).
    {
      string dsfile_name = ds_arg.substr(0, colpos);
      string child_name = ds_arg.substr(colpos + 1);
      VB::DataSet* ds = new VB::DataSet(dsfile_name);
      if (ds) {
        list<VB::DataSet*> sub_ds_list = ds->get_children(child_name);
        vbforeach(VB::DataSet * sub_ds, sub_ds_list) {
          sub_ds->import_inherited_members();
          sub_ds->set_parent(0);
          ds_list.push_back(sub_ds);
        }
      }
      delete ds;
    }
  }

  list<VBSequence> seqlist;
  vbforeach(VB::DataSet * ds, ds_list) {
    vbforeach(VB::DataSet::Member * mem, cmd_line_members)
        ds->insert_member(new VB::DataSet::Member(*mem));
    VBSequence vbs;
    VB::do_output(*seq, *ds, vbs);
    if (vbs.specmap.size()) seqlist.push_back(vbs);
    delete ds;
  }
  if (combineflag) {
    int jnum = 0;
    VBSequence bigseq = seqlist.front();
    bigseq.specmap.clear();
    for (list<VBSequence>::iterator ss = seqlist.begin(); ss != seqlist.end();
         ss++) {
      ss->renumber(jnum);
      jnum += ss->specmap.size();
      for (SMI jj = ss->specmap.begin(); jj != ss->specmap.end(); jj++) {
        bigseq.specmap[jj->second.jnum] = jj->second;
      }
    }
    seqlist.clear();
    seqlist.push_back(bigseq);
  }
  int index = 0;
  for (list<VBSequence>::iterator ss = seqlist.begin(); ss != seqlist.end();
       ss++) {
    ss->priority = pp;
    if (submit) {
      if (ss->specmap.size()) {
        string ldir = ss->specmap.begin()->second.logdir;
        if (ldir.size()) createfullpath(ldir);
      }
      if (vbp.cores == 0)
        ss->Submit(vbp);
      else
        runseq(vbp, *ss, vbp.cores);
    } else {
      string tmpname = odir_arg + "/seq" + strnum(getpid());
      if (seqlist.size() > 1) tmpname += "_" + strnum(index++);
      ss->Write(tmpname);
    }
  }
  delete seq;

  return 0;
}

void submit_sequence_help() {
  printf("\nVoxBo submit_sequence (v%s)\n", vbversion.c_str());
  printf("summary:\n");
  printf("  submit sequence in seq_file to the VoxBo queue\n");
  printf("usage:\n");
  printf("  submit_sequence <flags>\n");
  printf("flags:\n");
  printf("  -s <seqfile>        sequence file (i.e., the script)\n");
  printf("  -d <dataset>        dataset (see below)\n");
  printf("  -o <dir>            write sequences to dir instead of queue\n");
  printf("  -p <args>           see below\n");
  printf("  -c                  combine sequences into one\n");
  printf("\n");
  printf("notes:\n");
  printf(
      "  at least one dataset and one sequence file must be specified.  if\n");
  printf(
      "  more than one is specified, all dataset-sequence combinations are\n");
  printf("  queued.\n");
  printf("\n");
  printf(
      "  the dataset can be just a filename, or can specify a sub-node "
      "within\n");
  printf(
      "  the file using the same syntax as in the dataset file.  for "
      "example,\n");
  printf(
      "  mydataset.txt:node12:subnode5.  wildcards may also be used, as in:\n");
  printf(
      "  mydataset.txt:node* (take care that the wildcard isn't subject to "
      "shell\n");
  printf(
      "  expansion).  each wildcard expansion is treated as a separate "
      "dataset.\n");
  printf("\n");
  printf("  The -p flag takes priority-setting arguments -- see voxq for\n");
  printf("  details\n");
  printf("  \n");
}

void submit_sequence_version() {
  printf("VoxBo submit_sequence (v%s)\n", vbversion.c_str());
}
