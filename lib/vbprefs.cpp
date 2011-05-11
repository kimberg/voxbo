
// vbprefs.cpp
// code for reading preferences and initializing host information
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
// original version written by Dan Kimberg

using namespace std;

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "vbutil.h"
#include "vbjobspec.h"
#include "vbprefs.h"

extern char **environ;

VBPrefs::VBPrefs()
{
}

void
VBPrefs::init()
{
  FILE *fp;
  char prefsname[STRINGLEN];
  struct stat st;

  // clear environment variables?
  // environ=NULL;
  // first set defaults

  uid_t uid = getuid();
  struct passwd *pw = getpwuid(uid);       // look up user info
  if (pw) {
    username=pw->pw_name;                 // default username
    email=pw->pw_name;                    // default email address
    homedir=pw->pw_dir;                   // user's home directory
  }
  else {
    fprintf(stderr,"vbprefs.cpp: couldn't allocate passwd structure\n");
    exit(5);
  }
  
  // set the voxbo uid and group id
  struct passwd *vpw = getpwnam("voxbo");  // look up voxbo user info
  if (vpw) {
    voxbouid = vpw->pw_uid;
    voxbogid = vpw->pw_gid;
  }
  else {
    voxbouid=99;
    voxbogid=99;
  }

  // sendmail="/usr/lib/sendmail";
  sysadmin="root";
  superusers.clear();
  superusers.insert("root");
  su=0;
  serverport=6004;
  benchmarks.clear();

  struct utsname names;
  if (uname(&names) == -1) {
    fprintf(stderr,"vbprefs.cpp: uname failed, shouldn't happen\n");
    exit(5);
  }
  thishost.hostname=names.nodename;
  string tmps=thishost.hostname;
  if (tmps.find(".") != string::npos)
    tmps.erase(tmps.begin()+tmps.find("."),tmps.end());
  thishost.nickname=tmps;
  queuedelay = 30;
  jobtypemap.clear();

  // find rootdir

  rootdir="";
  vglob vg;
  vg.clear();
  if (getenv("VOXBO_ROOT"))
    vg.append(getenv("VOXBO_ROOT"),vglob::f_dirsonly);
  if (vg.empty())
    vg.append("/usr/local/[Vv]ox[Bb]o",vglob::f_dirsonly);
  if (vg.empty())
    vg.append("/usr/share/[Vv]ox[Bb]o",vglob::f_dirsonly);
  if (vg.empty())
    vg.append("/usr/lib/[Vv]ox[Bb]o",vglob::f_dirsonly);
  if (vg.size())
    rootdir=vg[0];

  // find userdir

  userdir="";
  vg.clear();
  // first try env var
  if (getenv("VOXBO_USERDIR")) {
    vg.append(getenv("VOXBO_USERDIR"),vglob::f_dirsonly);
    if (vg.size()) userdir=vg[0];
  }
  // then any HOME/voxbo* that has etc/jobtypes in it
  if (userdir.empty()) {
    vg.append(homedir+"/[Vv]ox[Bb]o*/etc/jobtypes",vglob::f_dirsonly);
    if (vg.size())
      userdir=xdirname(vg[0],2);
  }
  // finally either .voxbo or any other /home/voxbo*
  if (userdir.empty()) {
    vg.append(homedir+"/.[Vv]ox[Bb]o",vglob::f_dirsonly);
    vg.append(homedir+"/[Vv]ox[Bb]o*",vglob::f_dirsonly);
    if (vg.size())
      userdir=vg[0];
  }

  // if not found, create .voxbo
  if (userdir.empty()) {
    userdir=homedir+"/.voxbo";
    createfullpath(userdir);
  }

  // if we never found a root dir, use userdir
  if (rootdir.size()==0)
    rootdir=userdir;

  // queue is in either the global or user dir
  string qname;
  qname=rootdir+"/queue";
  if (!stat(qname.c_str(),&st))
    queuedir=qname;

  // number of local machine cores to use for jobs.  first check env
  // vars, then see if we have a drop dir, otherwise just use total
  // number of cores on this machine
  string corestring;
  if (getenv("VOXBO_CORES"))  // preferred spelling!
    corestring=getenv("VOXBO_CORES");
  else if (getenv("VOXBO_NCORES"))
    corestring=getenv("VOXBO_NCORES");
  else if (getenv("VB_CORES"))
    corestring=getenv("VB_CORES");
  else if (getenv("VB_NCORES"))
    corestring=getenv("VB_NCORES");
  if (corestring.size()) {
    pair<bool,int32> mycores=strtolx(corestring);
    if (mycores.first)
      cores=ncores();
    else
      cores=mycores.second;
  }
  else if (access((rootdir+"/drop").c_str(),W_OK)==0)
    cores=0;
  else
    cores=ncores();

  // if we're in cluster mode, read the system file
  if (cores==0) {
    sprintf(prefsname,"%s/etc/defaults",rootdir.c_str());
    fp = fopen(prefsname,"r");
    if (fp) {
      read_prefs(fp,1);
      fclose(fp);
    }
  }

  // finally read user-specific config
  sprintf(prefsname,"%s/prefs.txt",userdir.c_str());
  fp = fopen(prefsname,"r");
  if (fp) {
    read_prefs(fp,0);
    fclose(fp);
  }

  // set the root dir in env
  //char *str = (char *)malloc(rootdir.size()+10);
  //sprintf(str,"VOXROOT=%s",rootdir.c_str());
  //putenv(str);

  if (rootdir[rootdir.size()-1] != '/')
    rootdir+='/';
}

void
VBPrefs::read_prefs(FILE *fp,int system)
{
  char tmp[STRINGLEN];
  char line[STRINGLEN];
  string host;
  tokenlist args,argx;
  argx.SetSeparator(":");
  while(fgets(line,STRINGLEN,fp) != NULL) {
    if (args.ParseLine(line) < 2)
      continue;
    // handle machine-specific config stuff
    argx.ParseLine(args[0]);
    if (argx.size()>1) {
      if (argx[0]!=thishost.nickname)
        continue;
      args[0].erase(0,argx[0].size()+1);
    }
    if (args[0] == "rootdir")
      rootdir=args[1];
    else if ((args[0]=="nickname" || args[0]=="shortname") && system)
      thishost.nickname=args[1];
    else if (args[0] == "sysadmin" && system)
      sysadmin=args[1];
    else if (args[0] == "checkdir")
      thishost.checkdirs.push_back(args[1]);
    else if (args[0] == "benchmark" && args.size()>=4 && system) {
      VBenchmark bb;
      bb.name=args[1];
      bb.interval=strtol(args[2]);
      bb.cmd=args.Tail(3);
      bb.scheduled=0;
      if (bb.name.size() && bb.interval>59 && bb.cmd.size())
        benchmarks.push_back(bb);
    }
    else if (args[0] == "superusers" && system) {
      tokenlist ttt;
      ttt.ParseLine(args.Tail());
      for (size_t i=0; i<ttt.size(); i++)
        superusers.insert(ttt[i]);
      if (superusers.count(username))
        su=1;
    }
    else if (args[0] == "email")
      email=args[1];
    else if (args[0] == "sendmail" && system)
      sendmail=args[1];
    else if (args[0] == "server" && args.size()==3 && system) {
      servers[args[1]]=args[2];
    }
    else if (args[0] == "serveralias" && args.size()==3 && system) {
      if (thishost.hostname==args[1])
        thishost.nickname=args[2];
    }
    else if (args[0] == "serverport" && system)
      serverport = strtol(args[1]);
    else if (args[0] == (string)"voxbouser" && system) {
      struct passwd *vpw = getpwnam(args(1));
      if (!vpw)
        vpw=getpwuid(strtol(args(1)));
      if (vpw) {
        voxbouid = vpw->pw_uid;
        voxbogid = vpw->pw_gid;
      }
    }
    else if (args[0] == "setenv") {
      strcpy(tmp,args.Tail().c_str());
      stripchars(tmp,(char *)"=\n");
      // used to check if it was already set, now we don't
      char *str = (char *)malloc(args.Tail().size()+2);
      strcpy(str,args.Tail().c_str());
      putenv(str);
    }
    else if (args[0] == "queuedelay" && system)
      queuedelay = strtol(args[1]);
    else if (args[0] == "queuedir" && args.size()==3) {
      struct stat st;
      if (!stat(args[1].c_str(),&st))
        queuedir=args[1];
    }
  }
}

void
VBPrefs::read_jobtypes()
{
  jobtypemap.clear();
  vglob vg(rootdir+"/etc/jobtypes/*.vjt");
  for (size_t i=0; i<vg.size(); i++) {
    VBJobType tmpjt;
    if (!tmpjt.ReadJOB1(vg[i]))
      jobtypemap[tmpjt.shortname]=tmpjt;
    else
      fprintf(stderr,"[E] vbprefs: invalid jobtype file %s.\n",vg[i].c_str());
  }
}

// read_serverfile() -- used by a single server to read just its own
// server info into vbp.thishost

// FIXME: right now checkdirs is set in the global config file, which
// means that once we have the server file below, we need to copy it
// over before overwriting the whole structure.  this is ugly.

int
VBPrefs::read_serverfile()
{
  VBHost tmph;
  vglob vg;
  vg.append(rootdir+"/etc/servers/"+thishost.nickname);
  vg.append(rootdir+"/etc/servers/"+thishost.hostname);
  if (vg.empty())
    return 99;
  if (tmph.ReadFile(vg[0])==0) {
    tmph.checkdirs=thishost.checkdirs;
    tmph.initaddress(serverport);
    thishost=tmph;
    return 0;
  }
  return 101;
}

int
VBJobType::ReadJOB1(const string &fname)
{
  char line[STRINGLEN];
  tokenlist args;
  ifstream in;
  string tmp;
  // for processing requires lines
  tokenlist items,item;
  items.SetSeparator(",");

  in.open(fname.c_str());
  if (in.fail()) {
    fprintf(stderr,"vbprefs: couldn't open jobtype file %s\n",fname.c_str());
    return (101);
  }

  shortname=fname;    // default name
  while(!in.eof()) {
    in.getline(line,STRINGLEN);
    tmp=line;
    args.SetSeparator(" =");
    args.ParseLine(tmp);
    if (!args.size())
      continue;
    //if (args.size() < 2)
    //  continue;
    if (strchr(";/#%",args[0][0]))      // just a comment
      continue;
    if (args[0]=="shortname")
      shortname=args[1];
    else if (args[0]=="description")
      description=args.Tail();
    else if (args[0]=="command") {
      VBcmd cmd;
      cmd.command=args.Tail();
      commandlist.push_back(cmd);
    }
    else if (args[0]=="data") {
      jobdata jd;
      if (args.size() > 2) {
        jd.key = args[1]; 
        for (size_t h = 2; h < args.size(); h++)
          jd.datalist.push_back(args[h]);
      }
      else {
        tokenlist a;
        a.ParseLine(line);
        jd.key = a.Tail();
        while (!in.eof()) {
          in.getline(line, STRINGLEN);
          if (strncmp(line, "end", 3) == 0)
            break;
          else
            jd.datalist.push_back(line);
        }
      }
      jobdatalist.push_back(jd);
    } 
    else if (args[0]=="argument") {
      VBArgument tmpa;
      if (args.size() > 1) {
        for (size_t h=1; h<args.size(); h+=2) {
          if (args[h]=="type") {
            tmpa.type=args[h+1];
          }
          else if (args[h]=="name") {
            tmpa.name=(args[h+1]);
          }
          else if (args[h]=="argdesc") {
            tmpa.description=args[h+1];
          }
          else if (args[h]=="prefix") {
            continue;
          }
          else if (args[h]=="default") {
            tmpa.defaultval=(args[h+1]);
          }
          else if (args[h]=="role") {
            tmpa.role=(args[h+1]);
          }
          else if (args[h]=="high") {
            tmpa.high=(args[h+1]);
          }
          else if (args[h]=="low") {
            tmpa.low=(args[h+1]);
          }
          else if (args[h]=="end") {
            memset((void*)line,0,STRINGLEN);
            break;
          }
        }
        arguments.push_back(tmpa);
      }
      else { 
        while (!in.eof()) {
          in.getline(line, STRINGLEN);
          tmp=line;
          args.ParseLine(tmp);
          if (args[0]=="type") {
            tmpa.type=args[1];
          }
          else if (args[0]=="name") {
            tmpa.name=(args[1]);
          }
          else if (args[0]=="argdesc") {
            for (int j = 1; j < (int)args.size(); j++)
              tmpa.description+=(args[j] + " ");
          }
          else if (args[0]=="prefix") {
            continue;
          }
          else if (args[0]=="default") {
            tmpa.defaultval=(args[1]);
          }
          else if (args[0]=="role") {
            tmpa.role=(args[1]);
          }
          else if (args[0]=="high") {
            tmpa.high=(args[1]);
          }
          else if (args[0]=="low") {
            tmpa.low=(args[1]);
          }
          else if (args[0]=="end") {
            break;
          }
        }
        arguments.push_back(tmpa);
      } 
    }
    else if (args[0]=="invocation")
      invocation=args.Tail();
    else if (args[0]=="script" || args[0]=="|") {
      if (commandlist.size())
        commandlist[commandlist.size()-1].script.push_back(args.Tail());
    }
    else if (args[0]=="err_tag")
      err_tag = args.Tail();
    else if (args[0]=="warn_tag")
      warn_tag = args.Tail();
    else if (args[0]=="msg_tag")
      msg_tag = args.Tail();
    else if (args[0]=="retry_tag")
      retry_tag = args.Tail();
    else if (args[0]=="trigger") {
      tokenlist tmp;
      tmp.ParseLine(args.Tail());
      if (tmp.size()==4) {
        VBTrigger tt;
        tt.cond=tmp[0];
        tt.condvalue=tmp[1];
        tt.action=tmp[2];
        tt.actionvalue=tmp[3];
        triggerlist.push_back(tt);
      }
    }
    else if (args[0]=="setenv") {
      setenvlist.push_back(args.Tail());
    }
    else if (args[0]=="requires" || args[0]=="require") {
      items.ParseLine(args.Tail());
      for (int i=0; i<(int)items.size(); i++) {
        item.ParseLine(items[i]);
        if (item.size() >= 2)
          requires[item[0]]=strtol(item[1]);
        else
          requires[item[0]]=0;
      }
    }
    else if (args[0]=="fullname") {
      continue;  // ignore this unused tag
    }
    else {
      fprintf(stderr,"vbprefs: unrecognized keyword %s in jobtype %s\n",
              args(0),shortname.c_str());
    }
  }
  in.close();
  //if (command.size() < 1)
  //return (102);  // should do *something*
  return (0);  // no error!
}

VBJobType::VBJobType()
{
  init();
}

void
VBJobType::init()
{
  shortname="";
  // fullname="";
  description="";
  invocation="";
  commandlist.clear();
  setenvlist.clear();
  err_tag="VOXBO ERROR";
  warn_tag="VOXBO WARNING";
  msg_tag="VOXBO MESSAGE";
  retry_tag="VOXBO RETRY";
  requires.clear();
  nomail.clear();
  nomail.push_back((string)"% Compiled module:");
  nomail.push_back((string)"Percent done:");
}

tokenlist
VBJobType::getData(string key)
{
  tokenlist ret;
  for (int i = 0; i < (int)jobdatalist.size(); i++) {
    if (jobdatalist[i].key==key) {
      for (int j = 0; j < (int)jobdatalist[i].datalist.size(); j++) {
        ret.Add(jobdatalist[i].datalist[j]);
      }
      break;
    }
  }
  return ret;
}

void
VBJobType::print()
{
  int i,j;
  printf("Jobtype %s:\n",shortname.c_str());
  printf("  description: %s\n",description.c_str());
  printf("   invocation: %s\n",invocation.c_str());
  printf("      err_tag: %s\n",err_tag.c_str());
  printf("     warn_tag: %s\n",warn_tag.c_str());
  printf("      msg_tag: %s\n",msg_tag.c_str());
  printf("    retry_tag: %s\n",retry_tag.c_str());
  for (i=0; i<(int)setenvlist.size(); i++) {
    printf("       setenv: %s\n",setenvlist[i].c_str());
  }

  printf("     requires: ");
  typedef pair<string,int> sipair;
  vbforeach(sipair si,requires)
    // for (i=0; i<(int)requires.size(); i++)
    printf("%s(%d) ",si.first.c_str(),si.second);
  printf("\n");

  printf("    arguments:");
  for (i=0; i<(int)arguments.size(); i++) {
    if (i==0) printf(" "); else printf("               ");
    printf("%s (%s): %s\n",
           arguments[i].name.c_str(),
           arguments[i].role.c_str(),
           arguments[i].description.c_str());
  }

  for (i=0; i<(int)commandlist.size(); i++) {
    printf("      command: %s\n",commandlist[i].command.c_str());
    for (j=0; j<(int)commandlist[i].script.size(); j++)
      printf("             : %s\n",commandlist[i].script[j].c_str());
  }
}
