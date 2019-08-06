
// vbpreplib.cpp
// implementations for class methods used in vbprep.cpp
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
// original version written by Tom King based on code by Daniel Y. Kimberg

#include "vbpreplib.h"

using namespace std;

VBPJob::VBPJob() {
  lastjobnum = -1;
  jobtype = "";
  runparallel = 0;
}

void VBPJob::clear() {
  jobtype = -1;
  args.clear();
  lastjobnum = -1;
  runonce = 0;
  runparallel = 0;
}

VBPFile::VBPFile() {
  dimensions = 0;
  filename.clear();
  filetype = "";
  lastjob = 0;
}

VBPrep::VBPrep() {
  priority = "";
  sequenceName = "";
  directory = "";
  email = "";
  seq.init();
  js.init();
}

void VBPrep::ClearData() {
  priority = "";
  sequenceName = "";
  directory = "";
  email = "";
  seq.init();
  js.init();
  globals.clear();
  filelist.clear();
  return;
}

void VBPrep::ClearJobs() { joblist.clear(); }

VBPData::VBPData(const VBPrefs &invbp) {
  vbp = invbp;
  data.clear();
}

int VBPData::Clear() {
  study.ClearData();
  study.ClearJobs();
  data.clear();
  return 0;
}

int VBPrep::BuildJobs(VBPrefs &vbp) {
  // vbp arg not const because we later use a pointer into
  // vbp.jobtypelist for no good reason
  int jobNumber = 0;
  int waitPrev = 0;
  int waitPrevFlag = 0;
  string fileString;
  tokenlist temp;
  int iterations = 0;
  int lastRunonce = 0;
  set<int32> prevWaitList;

  // trap for no commands
  if (joblist.size() == 0) {
    return 100;
  }
  // add dir and sequenceName to globals
  if (directory.size()) globals.Add("DIR=" + directory);
  if (sequenceName.size()) globals.Add("SEQUENCENAME=" + sequenceName);
  if (email.size()) globals.Add("EMAIL=" + email);
  for (int i = 0; i < (int)joblist.size(); i++) {
    if ((joblist[i].jobtype == "waitprev") && (i == 0)) {
      return 101;
    }
    if ((joblist[i].jobtype == "waitprev") &&
        (i == ((int)joblist.size() - 1))) {
      return 102;
    }
    // if directive waitprev is set, set flag and go to next command
    if (joblist[i].jobtype == "waitprev") {
      // note: at this point js.jnun has not be reset from last file/command of
      // script
      waitPrevFlag = js.jnum;
      waitPrev = 0;
      continue;
    }
    // set previous job id if we need to wait for it
    if (waitPrevFlag) {
      if (joblist[i].jobtype == "waitprev") {
        return 103;
      }
      waitPrev = waitPrevFlag;
      waitPrevFlag = 0;
    } else
      waitPrev = 0;

    // logic: if it's a regular job expecting FILEs, but there aren't any, kill
    //       if runonce is used, we need to iterate once to process call
    //       otherwise, iterate as many times as there are FILEs
    if ((!filelist.size()) && (!joblist[i].runonce)) {
      return 104;
    } else if (joblist[i].runonce) {
      iterations = 1;
      lastRunonce = jobNumber;
    } else
      iterations = filelist.size();
    // set all the job stuff
    for (int j = 0; j < iterations; j++) {
      if (vbp.jobtypemap.count(joblist[i].jobtype) == 0) return 105;
      VBJobType *jt = &(vbp.jobtypemap[joblist[i].jobtype]);
      js.init();
      js.arguments.clear();
      js.jobtype = joblist[i].jobtype;
      for (int m = 0; m < (int)joblist[i].args.size(); m++) {
        if (m > (int)(jt->arguments.size() - 1)) break;
        string tmps = joblist[i].args[m];
        // for compound args, just grab the whole thing...
        if (jt->arguments[m].type == "compound") {
          if (joblist[i].runonce)
            tmps = joblist[i].args.Tail(m + 2);
          else
            tmps = joblist[i].args.Tail(m + 1);
        }
        if (!(joblist[i].runonce)) fill_vars(tmps, filelist[j].filename);
        if (globals.size()) fill_vars(tmps, globals);
        js.arguments[jt->arguments[m].name] = tmps;
        // ... and we're done
        if (jt->arguments[m].type == "compound") break;
      }
      if (waitPrev) js.waitfor.insert(waitPrev);
      // handle job processing order
      if (joblist[i].runparallel &&
          joblist[i - 1].runparallel) {  // previous job runparallel: inherit
                                         // its wait list
        vbforeach(int32 pw, prevWaitList) js.waitfor.insert(pw);
        // WAS PREVIOUSLY:
        // for (int k = 0; k < (int)prevWaitList.size(); k++)
        // js.waitfor.insert(prevWaitList[k]);
      } else if (jobNumber != 0) {
        if (i && (joblist[i - 1].runparallel))
          for (int counter = i - 1, waitcount = 1; counter >= 0; counter--) {
            if (!joblist[counter].runparallel) break;
            if (joblist[counter].runonce) {
              js.waitfor.insert(jobNumber - waitcount);
              waitcount++;
            } else
              for (int file = 1; file <= (int)filelist.size(); file++) {
                js.waitfor.insert(jobNumber - waitcount);
                waitcount++;
              }
          }
        else if (i && (joblist[i - 1].runonce))
          if (!joblist[i].runonce)
            js.waitfor.insert(lastRunonce);
          else
            js.waitfor.insert(lastRunonce - 1);
        else if (!joblist[i].runonce && filelist[j].lastjob)
          js.waitfor.insert(filelist[j].lastjob);
        else if (i && !joblist[i - 1].runonce && joblist[i].runonce)
          for (int counter = 1; counter <= (int)filelist.size(); counter++)
            js.waitfor.insert(jobNumber - counter);
        else if (jobNumber >= (int)filelist.size())
          js.waitfor.insert(jobNumber - filelist.size());
      }
      js.magnitude = 0;              // value of no significance
      js.name = joblist[i].jobtype;  // is this a good name?
      js.jnum = jobNumber;
      js.dirname = directory;
      js.logdir = directory + "/logs";
      seq.addJob(js);
      if (!joblist[i].runonce) filelist[j].lastjob = js.jnum;
      prevWaitList = js.waitfor;
      // WAS PREVIOUSLY
      // prevWaitList.clear();
      // for (int k = 0; k < (int)js.waitfor.size(); k++)
      // prevWaitList.push_back(js.waitfor[k]);
      jobNumber++;
    }
    // all these set values: if value is not in script, set to default
    if (priority.size())
      seq.priority = strtol(priority);
    else
      seq.priority = 3;
    if (sequenceName.size())
      seq.name = sequenceName;
    else
      seq.name = joblist[i - 1].jobtype;

    if (email.size())
      seq.email = email;
    else
      seq.email = vbp.email;
    // FIXME? temp fix
    seq.seqnum = (int)getpid();
  }
  return 0;
}

int VBPData::StoreDataFromFile(string fname, string selectedSequence) {
  ParseFile(fname, selectedSequence);
  if ((selectedSequence == study.sequenceName) || (!selectedSequence.size()))
    data.push_back(study);
  return data.size();
}

int VBPData::ParseFile(string fname, string selectedSequence) {
  char fileLine[STRINGLEN];
  string tokenString;
  tokenlist list;
  VBPJob vbpjob;
  VBPFile vbpfile;
  ifstream in;
  string directive;
  string tempFileName;
  struct stat my_stat;
  study.ClearData();
  string name = ScriptName(fname);
  string dir = xdirname(fname);

  if (name.size() == 0) return 106;

  in.open(name.c_str());
  if (!in) {
    return 106;
  }

  // iterated through script file, parsing based on first word of line
  while (in.getline(fileLine, STRINGLEN, '\n')) {
    // clear list and put line from file into tokenlist
    list.clear();
    vbpjob.runparallel = 0;
    list.SetSeparator(" ");
    list.ParseLine(fileLine);
    if (strcmp(list[0].c_str(), "|") == 0) {  // in case they indent
      list.Remove(0, 1);
      vbpjob.runparallel = 1;
    } else if (list[0][0] == '|') {
      list[0] = list[0].substr(1, list[0].size() - 1);
      vbpjob.runparallel = 1;
    }
    if (list[0][0] == '#') {
      // skip any commented-out lines
      continue;
    }
    directive = vb_toupper(list[0]);
    if (directive == "SCRIPT") {
      if (!list[1].size()) continue;
      tempFileName = dir + "/" + list[1];
      if (stat(tempFileName.c_str(), &my_stat) != 0) {
        study.ClearJobs();
        ParseFile(list[1], selectedSequence);
      } else {
        study.ClearJobs();
        ParseFile(tempFileName, selectedSequence);
      }
      // if a newdata does not end the script, do newdata commands
      if (study.filelist.size()) {
        // process only if specified or a filter is not set
        if ((selectedSequence == study.sequenceName) ||
            (!selectedSequence.size()))
          data.push_back(study);
        study.ClearData();
      }
    } else if (directive == "DATA") {
      if (!list[1].size()) continue;
      tempFileName = dir + "/" + list[1];
      if (stat(tempFileName.c_str(), &my_stat) != 0) {
        study.ClearData();
        ParseFile(list[1], selectedSequence);
      } else {
        study.ClearData();
        ParseFile(tempFileName, selectedSequence);
      }
    } else if (directive == "DIR") {
      // set directory
      study.directory = list[1];
    } else if (directive == "DOC") {
      // ignore documentation
      continue;
    } else if (directive == "VARIABLE") {
      VBVariable var;
      var.name = list[1];
      while (in.getline(fileLine, STRINGLEN, '\n')) {
        list.clear();
        list.SetSeparator(" ");
        list.ParseLine(fileLine);
        directive = vb_toupper(list[0]);
        if (strcmp(fileLine, "END") == 0)
          break;
        else if (directive == "TYPE") {
          // push files on filelist
          list.DeleteFirst();
          var.type = vb_toupper(list[0]);
        } else if (directive == "DEFAULT") {
          if (list.size() == 1)
            var.defaultValue = "";
          else {
            list.DeleteFirst();
            var.defaultValue = list.MakeString();
          }
        } else if (directive == "DESCRIPTION") {
          if (list.size() == 1)
            var.description = "";
          else {
            list.DeleteFirst();
            var.description = list.MakeString();
          }
        }
      }
      varlist.push_back(var);
    } else if (directive == "SET") {
      list.DeleteFirst();
      string name;
      for (int varnum = 0; varnum < (int)varlist.size(); varnum++) {
        if (varlist[varnum].name == list[0]) {
          if (vb_toupper(list[0]) == "DIR" ||
              vb_toupper(list[0]) == "PRIORITY" ||
              vb_toupper(list[0]) == "SEQUENCENAME" ||
              vb_toupper(list[0]) == "EMAIL")
            name = vb_toupper(list[0]);
          else
            name = list[0];
          if (varlist[varnum].type == "GLOBALS") {
            varlist[varnum].currentValue = name;
            varlist[varnum].currentValue += "=";
            if (list[1].size())
              varlist[varnum].currentValue += list[1];
            else if (varlist[varnum].defaultValue.size())
              varlist[varnum].currentValue += varlist[varnum].defaultValue;
            else
              varlist[varnum].currentValue += " ";
            if (study.globals.size())
              study.globals.Add(varlist[varnum].currentValue);
            else
              study.globals = varlist[varnum].currentValue;
            if (name == "DIR") study.directory = list[1];
            if (name == "PRIORITY") study.priority = list[1];
            if (name == "SEQUENCENAME") study.sequenceName = list[1];
            if (name == "EMAIL") study.email = list[1];
          } else if (varlist[varnum].type == "FILE") {
            varlist[varnum].currentValue = "";
            for (int count = 0; count < (int)list.size(); count += 2) {
              varlist[varnum].currentValue += list[count];
              varlist[varnum].currentValue += "=";
              if (list[count + 1].size())
                varlist[varnum].currentValue += list[count + 1];
              else if (varlist[varnum].defaultValue.size())
                varlist[varnum].currentValue += varlist[varnum].defaultValue;
              else
                varlist[varnum].currentValue += " ";
              varlist[varnum].currentValue += " ";
            }
            vbpfile.filename = varlist[varnum].currentValue;
            study.filelist.push_back(vbpfile);
          }
        }
      }
    } else if (directive == "FILE") {
      // push files on filelist
      list.DeleteFirst();
      vbpfile.filename = list;
      study.filelist.push_back(vbpfile);
    } else if (directive == "LOGGING") {
      // NO-OP
    } else if (directive == "GLOBALS") {
      list.DeleteFirst();
      if (study.globals.size())
        study.globals = list + study.globals;
      else
        study.globals = list;
    } else if (directive == "PRIORITY") {
      // set priority if specified in file
      list.DeleteFirst();
      study.priority = list[0];
    } else if (directive == "EMAIL") {
      // set email if specified in file
      list.DeleteFirst();
      study.email = list[0];
    } else if (directive == "SEQUENCENAME") {
      // set sequence name if specified in file
      list.DeleteFirst();
      study.sequenceName = list[0];
    } else if (directive == "NEWDATA") {
      // directive indicates totally new study.
      // push into data vector
      if ((study.joblist.size()) || (study.filelist.size()))
        // process only if specified or a filter is not set
        if ((selectedSequence == study.sequenceName) ||
            (!selectedSequence.size()))
          data.push_back(study);
      study.ClearData();
    } else {
      // push job on job queue
      if (list.size() != 0) {
        if (directive == "RUNONCE") {
          vbpjob.jobtype = list[1];
          vbpjob.runonce = 1;
          // make list start at 3rd element
          list.Remove(0, 2);
          vbpjob.args = list;
        } else {
          vbpjob.jobtype = list[0];
          list.DeleteFirst();
          vbpjob.args = list;
          vbpjob.runonce = 0;
        }
        study.joblist.push_back(vbpjob);
      }
    }
  }
  in.close();
  return 0;
}

string VBPData::GetDocumentation(string fname) {
  char fileLine[STRINGLEN];
  string tokenString;
  tokenlist list;
  VBPJob vbpjob;
  VBPFile vbpfile;
  ifstream in;
  string directive;
  string tempFileName;
  struct stat my_stat;

  string name = ScriptName(fname);
  string dir = xdirname(fname);

  if (name.size() == 0) return (string) "";

  in.open(name.c_str());
  if (!in) {
    return (string) "";
  }

  // iterated through script file, parsing based on first word of line
  while (in.getline(fileLine, STRINGLEN, '\n')) {
    // clear list and put line from file into tokenlist
    list.clear();
    list.ParseLine(fileLine);
    if (list[0][0] == '#') {
      // skip any commented-out lines
      continue;
    }
    directive = vb_toupper(list[0]);
    if (directive == "SCRIPT") {
      if (!list[1].size()) continue;
      tempFileName = dir + "/" + list[1];
      if (stat(tempFileName.c_str(), &my_stat) != 0) {
        GetDocumentation(list[1]);
      } else {
        GetDocumentation(tempFileName);
      }
    } else if (directive == "DATA") {
      if (!list[1].size()) continue;
      tempFileName = dir + "/" + list[1];
      if (stat(tempFileName.c_str(), &my_stat) != 0) {
        GetDocumentation(list[1]);
      } else {
        GetDocumentation(tempFileName);
      }
    }
    if (directive == "DOC") {
      list.DeleteFirst();
      return list.MakeString();
    }
  }
  return (string) "";
}

string VBPData::ScriptName(string name) {
  if (!name.size()) return string("");

  struct stat my_stat;
  string temp;

  // check current directory
  if (stat(name.c_str(), &my_stat) == 0) return name;

  // check voxbo script directory
  temp = vbp.homedir + "/VoxBo/scripts/" + name;
  if (stat(temp.c_str(), &my_stat) == 0) return temp;

  // check user home script directory
  temp = vbp.rootdir + "scripts/" + name;
  if (stat(temp.c_str(), &my_stat) == 0)
    return temp;
  else
    return string(" ");
}
