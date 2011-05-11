
// dbtool.cpp
// text client for creating/examining a local database
// Copyright (c) 2009-2010 by The VoxBo Development Team

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 3, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file named COPYING.  If not, write
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA
//
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg

using namespace std;

#include "dbclient.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "dbfiles.h"

class DBtool {
public:
  DBtool(int argc,char **argv);
private:
  localClient client;
  void create(tokenlist &args);
  void dump(tokenlist &args);
  void dump_types();
  void dump_scorenames();
  void dump_allscores();
  void dump_views();
  void dump_patients();
  void help(tokenlist &args);
  void nextid(tokenlist &args);
  void open(tokenlist &args);
  void close(tokenlist &args);
  string ask(string prompt);
  int err;    // just so i don't have to keep declaring it
};

int
main(int argc,char **argv)
{
  DBtool dbt(argc,argv);
}

DBtool::DBtool(int argc,char **argv)
{
  char *ret;
  string cmd;
  tokenlist args;

  // special usage dbtool maketest makes test db
  if (argc==2 && (string)"maketest"==argv[1]) {
    args.Add("create");
    args.Add("test");
    args.Add("test");
    this->create(args);
    exit(0);
  }

  if (argc==2) {
    args.Add("open");
    args.Add(argv[1]);
    this->open(args);
  }

  while (1) {
    ret=readline("dbtool> ");
    if (ret==NULL) {
      if (client)
        client.exit();
      cout << endl;
      break;
    }
    add_history(ret);
    args.ParseLine(ret);
    if (args[0]=="create")
      this->create(args);
    else if (args[0]=="dump")
      this->dump(args);
    else if (args[0]=="help")
      this->help(args);
    else if (args[0]=="nextid")
      this->nextid(args);
    else if (args[0]=="open")
      this->open(args);
    else if (args[0]=="close")
      this->close(args);
    else if (args[0].size()) {
      cout << "unrecognized command " << args[0] << endl;
    }
  }
}

void
DBtool::create(tokenlist &args)
{
  if (args.size()<2) {
    cout << "[E] dbtool: create takes at least one argument\n";
    return;
  }
  DBdata dbd;
  int err;
  if ((err=dbd.initDB(args[1],"test"))) {
    cout << "*** error " << err << " creating DB " << args[1] << endl;
    return;
  }
  else
    cout << "DB " << args[1] << " created successfully\n";

  if (args.size()==3 && args[2]=="test") {
    FILE *fp;
    err=0;
    fp=fopen((args[1]+"/scorenames.txt").c_str(),"w");
    if (fp) {
      if (fwrite(test_sn,1,sizeof(test_sn)-1,fp) != sizeof(test_sn)-1)
        err++;
      fclose(fp);
    }
    else err++;
    fp=fopen((args[1]+"/views.txt").c_str(),"w");
    if (fp) {
      if (fwrite(test_vi,1,sizeof(test_vi)-1,fp) != sizeof(test_vi)-1)
        err++;
      fclose(fp);
    }
    else err++;
    fp=fopen((args[1]+"/types.txt").c_str(),"w");
    if (fp) {
      if (fwrite(test_ty,1,sizeof(test_ty)-1,fp) != sizeof(test_ty)-1)
        err++;
      fclose(fp);
    }
    else err++;
  }
}

void
DBtool::dump(tokenlist &args)
{
  string mode,ret;
  if (args.size()==1) {
    ret=ask("dump [t]ypes, score[n]ames, [s]cores, [v]iews, [p]atients, or [a]ll? ");
    cout << endl;
    ret=vb_tolower(ret);
    if (ret=="t") mode="types";
    else if (ret=="n") mode="scorenames";
    else if (ret=="s") mode="scores";
    else if (ret=="v") mode="views";
    else if (ret=="p") mode="patients";
    else if (ret=="a") mode="all";
  }
  else
    mode=vb_tolower(args[1]);
  if (mode=="patients" || mode=="p" || mode=="all")
    dump_patients();
  if (mode=="scorenames" || mode=="sn" || mode=="s" || mode=="all")
    dump_scorenames();
  if (mode=="views" || mode=="v" || mode=="all")
    dump_views();
  if (mode=="allscores" || mode=="scores" || mode=="all")
    dump_allscores();
  if (mode=="types" || mode=="t" || mode=="all")
    dump_types();
}

void
DBtool::dump_patients()
{
  // FIXME the only way to get all patients is to search for *
  patientSearchTags s_tags;
  s_tags.scoreName="";
  s_tags.relationship="wildcard";
  s_tags.searchStr="*";
  s_tags.case_sensitive=0;
  vector<patientMatch> pmList;
  err=client.reqSearchPatient(s_tags,pmList);
  cout << "found " << pmList.size() << " patients in db\n";
  vbforeach(patientMatch pm,pmList) {
    DBpatient p;
    err=client.reqOnePatient(pm.patientID,p);
    if (err) {
      cout << format("[E] dbtool: error %d requesting patient %d\n")%err%pm.patientID;
      continue;
    }
    cout << format("Patient %d:\n")%p.patientID;
    cout << format("   sessions: %d\n")%p.sessions.size();
    pair<int32,DBscorevalue> sv;
    vbforeach(sv,p.scores) {
      cout << format("  %05d %05d %s=%s sid=%05d idx=%d dt=%s setby %s")%
        sv.second.id%sv.second.parentid%sv.second.scorename%sv.second.v_string%
        sv.second.sessionid%sv.second.index%sv.second.datatype%sv.second.setby;
      if (sv.second.deleted) cout << " (DELETED)";
      cout << endl;
    }
  }
}

void
DBtool::dump_allscores()
{
  cout << "\nALL SCORES:\n";
  client.dbs.dumpscorevalues();
}

void
DBtool::dump_views()
{
  cout << "views coming soon\n";
}

void
DBtool::dump_types()
{
  cout << "types coming soon\n";
}

void
DBtool::dump_scorenames()
{
  cout << format("[I] dbtool: found %d scorenames:\n")%client.dbs.scorenames.size();
  deque<string> snq;
  snq.push_front("");
  uint32 count=0;
  string item;
  while (snq.size()) {
    count++;
    item=snq.front();
    snq.pop_front();
    if (item!="")
      cout << "    " << item << endl;
    vector<string> tmp=getchildren(client.dbs.scorenamechildren,item);
    vbforeach(string ss,tmp)
      snq.push_front(ss);
  }
  if (count-1 != client.dbs.scorenames.size()) {
    cout << format("[E] dbtool: internal error -- %d types in scorenamechildren map, %d in scorenames map\n")
      %(count-1)%client.dbs.scorenames.size();
  }
}

void
DBtool::help(tokenlist &)
{
  cout << "create <dirname>" << endl;
  cout << "dump" << endl;
  cout << "help" << endl;
  cout << "open <dirname>" << endl;
}

void
DBtool::nextid(tokenlist &)
{
  DbTxn* txn = NULL;
  client.dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int32 startID = getSysID(client.dbs.sysDB, txn,1);
  if (startID <= 0) {
    cout << "argh!" << endl;
    txn->abort();
    return;
  }
  txn->commit(0);
  client.dbs.sysDB.getDb().sync(0); // write data on disk now    
  cout << startID << endl;
}

void
DBtool::close(tokenlist &)
{
  if (client)
    client.exit();
  else
    cout << "dbtool: not connected to any database right now\n";
}

void
DBtool::open(tokenlist &args)
{
  string dbdir,username,password;
  char *ret;
  
  // get and check dir
  if (args.size()>1)
    dbdir=args[1];
  else {
    ret=readline("db directory: ");
    if (ret==NULL) return;
    dbdir=ret;
  }
  if (!vb_direxists(args[1])) {
    cout << "no such directory\n";
    return;
  }
  // get and check username
  if (args.size()>2)
    username=args[2];
  else {
    ret=readline("username: ");
    if (ret==NULL) return;
    username=ret;
  }

  // get and check password
  if (args.size()>3)
    password=args[3];
  else {
    ret=readline("password: ");
    if (ret==NULL) return;
    password=ret;
  }

  client.setUserName(username);
  client.setPasswd(password);
  client.setEnvHome(dbdir);
  if (client.login()) {
    cout << "error opening db: " << client.getErrMsg() << endl;
    return;
  }
  cout << "DB opened" << endl;
  cout << format("       directory: %s\n")%client.dbs.dirname;
  cout << format("       typefname: %s\n")%client.dbs.typeFilename;
  cout << format("  scorenamefname: %s\n")%client.dbs.scoreNameFilename;
  cout << format("       viewfname: %s\n")%client.dbs.viewFilename;
  cout << format("     total types: %d\n")%client.dbs.typemap.size();
  cout << format("total scorenames: %d\n")%client.dbs.scorenames.size();
  cout << format("     total views: %d\n")%client.dbs.viewspecs.size();
}


string
DBtool::ask(string prompt)
{
  termios tsave,tnew;
  tcgetattr(0,&tsave);
  tcgetattr(0,&tnew);
  tnew.c_lflag&=~(ICANON|ECHO);
  tcsetattr(0,TCSADRAIN,&tnew);
  string str;
  cout << prompt << flush;
  str=cin.get();
  tcsetattr(0,TCSADRAIN,&tsave);
  return str;
}
