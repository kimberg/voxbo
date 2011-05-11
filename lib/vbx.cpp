
// vbx.cpp
// The VoxBo job execution engine
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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <grp.h>
#include "vbx.h"

int fork_command(VBJobSpec &js,int ind);
void run_command(VBJobSpec &js,int i);
void talk2child(VBJobSpec &js,vector<string> myscript,int &logpipe,int &cmdpipe);
void test_outputline(VBJobSpec &js,string &line);
void execute_action(VBJobSpec &js,string &line,TGI tgi);
void parse_status(VBJobSpec &js,int status);  // FIXME used?
void do_internal(VBJobSpec &js);
vector<string> build_script(VBJobSpec &js,int ind);
void exec_command(VBJobSpec &js,vector<string>script,int ind);
void signal_handler(int signal);

int killme=0;

int
run_voxbo_job(VBPrefs &vbp,VBJobSpec &js)
{
  signal(SIGUSR1,signal_handler);
  // copy some stuff from vbp struct
  js.hostname=vbp.thishost.nickname;
  js.voxbouid=vbp.voxbouid;
  js.voxbogid=vbp.voxbogid;
  js.queuedir=vbp.queuedir;
  
  if (js.jt.commandlist.size()) {
    for (int i=0; i<(int)js.jt.commandlist.size(); i++) {
      if (killme==0)
        fork_command(js,i);
    }
  }
  else if (js.jt.invocation=="internal")
    fork_command(js,-1);    // for built-in commands
  else {
    js.SetState(XBad);
    js.error=-1;
    js.errorstring=str(format("jobtype %s has no commands")%js.jt.shortname);
  }
  return 0;
}

int
fork_command(VBJobSpec &js,int ind)
{
  // fprintf(stderr,"DB: forking cmd %d\n",ind);
  int cmdpipe[2],logpipe[2];
  js.error=-9999;
  js.errorstring="no status code reported";
  js.SetState(XNone);

  // CREATE THE PIPES AND FORK
  if (pipe(cmdpipe) < 0) {
    js.SetState(XBad);
    js.error=-1;
    js.errorstring="Internal error in vbx: couldn't create log pipe.";
    fprintf(stderr,"[E] vbx: pipe error 1\n");
    return 200;
  }
  if (pipe(logpipe) < 0) {
    js.SetState(XBad);
    js.error=-1;
    js.errorstring="Internal error in vbx: couldn't create cmd pipe.";
    close(cmdpipe[0]);
    close(cmdpipe[1]);
    fprintf(stderr,"[E] vbx: pipe error 2\n");
    return 200;
  }
  // fprintf(stderr,"log0 %d cmd1 %d\n",logpipe[0],cmdpipe[1]);
  long pid = fork();
  if (pid < 0) {
    js.SetState(XBad);
    js.error=-1;
    js.errorstring="Internal error in vbx: couldn't fork.";
    fprintf(stderr,"[E] vbx: fork error\n");
    return 1;
  }
  if (pid == 0) {     // we are the child process
    // fprintf(stderr,"DB: child log will now go to parent\n");
    close(logpipe[0]);     // close the reading end of the log pipe
    close(cmdpipe[1]);     // close the writing end of the cmd pipe
    dup2(cmdpipe[0],0);    // attach stdin to cmdpipe output
    dup2(logpipe[1],1);    // attach stdout and stder to logpipe
    dup2(logpipe[1],2);

    run_command(js,ind);   // actually run the job

    close(logpipe[1]);     // be nice and close the pipes
    close(cmdpipe[0]);

    _exit(js.error);       // child process exits, parent will clean up
  }

  // if we got here, we are the parent process
  js.pid=getpid();
  js.childpid=pid;
  // send acknowledgment back to scheduler with parent and child pids
  // fprintf(stdout,"ACK %ld %ld",(long)js.pid,(long)pid);
  if (js.f_cluster)
    tell_scheduler(js.queuedir,js.hostname,
                   (string)"jobrunning "+js.hostname+" "+strnum(js.snum)+" "+
                   strnum(js.jnum)+" "+strnum(js.pid)+" "+strnum(pid)+
                   " "+strnum(time(NULL)));
  close(logpipe[1]);      // close the writing end of logpipe
  close(cmdpipe[0]);      // close the reading end of cmdpipe
  // read log and control messages
  seteuid(getuid());       // be user voxbo if possible
  setegid(js.voxbogid);
  seteuid(js.voxbouid);

  vector<string> myscript=build_script(js,ind);
  talk2child(js,myscript,logpipe[0],cmdpipe[1]);

  seteuid(getuid());       // be ourselves again
  setegid(getgid());

  // wait for child to terminate, get status, parse it
  int status;
  wait(&status);
  parse_status(js,status);
  
  // done with the pipes
  if (logpipe[0]>0)
    close(logpipe[0]);
  if (cmdpipe[1]>0)
    close(cmdpipe[1]);

  return 0;
}

void
run_command(VBJobSpec &js,int ind)
{
  // in case we're in single-user mode, set these now
  js.SetState(XNone);
  js.error = 0;
  js.errorstring="";

  if (ind<1) {
    fprintf(stderr,"+------------------------------\n");
    fprintf(stderr,"| BEGINNING JOB %s\n",js.basename().c_str());
    fprintf(stderr,"+------------------------------\n");
  }
  fprintf(stderr,"running command %d from job %s\n",ind,js.basename().c_str());
  if (js.status != 'W' && js.status != 'R') {
    fprintf(stderr,"*** job %s arrived with status %c\n",js.basename().c_str(),js.status);
    return;
  }
  
  // if we're root, we can become the right user

  struct passwd *userpw=getpwuid(js.uid);
  seteuid(getuid());
  setgid(userpw->pw_gid);          // set our group
  initgroups(userpw->pw_name,userpw->pw_gid);   // set supplementary groups
  setuid(js.uid);                 // now set user to user
  umask(002);                      // share with your group!
  
  if (getuid() == 0) {             // don't run jobs as root
    fprintf(stderr,"*** job %05d-%05d wanted to run as root\n",js.snum,js.jnum);
    js.error=-1000;
    return;
  }

  // set the job-specific environment variables
  for (int i=0; i<(int)js.jt.setenvlist.size(); i++) {
    string jtenv=js.jt.setenvlist[i];
    fill_vars2(jtenv,envmap(environ));   // jobtype env could depend on global env!
    char *tmp=(char *)malloc(jtenv.size()+2);
    strcpy(tmp,jtenv.c_str());
    putenv(tmp);
  }

  // run the job

  fprintf(stderr,"job \"%s\" (%s), type %s\n",js.name.c_str(),
          js.basename().c_str(),js.jobtype.c_str());
  fprintf(stderr,"running on host %s\n",js.hostname.c_str());
  fprintf(stderr,"started %s\n",timedate().c_str());
  
  if (js.jt.invocation=="internal")
    do_internal(js);               // will do either notify or timewaster
  else {
    vector<string> script=build_script(js,ind);
    exec_command(js,script,ind);
  }
  return;  // in case we didn't exec
}

void
talk2child(VBJobSpec &js,vector<string> myscript,int &logpipe,int &cmdpipe)
{
  int cnt;
  char buf[STRINGLEN];
  tokenlist bufset;
  bufset.SetSeparator("\n");
  string bufstr;
  string qlogfname,ulogfname;
  if (js.f_cluster)
    qlogfname=js.queuedir+"/"+js.seqdirname()+"/"+js.basename()+".log";
  if (js.logdir.size())
    ulogfname=js.logdir+"/"+js.basename()+".log";
  // grab this user's group id
  struct passwd *userpw=getpwuid(js.uid);
  gid_t usergid=userpw->pw_gid;
  uid_t save_euid=geteuid();
  gid_t save_egid=getegid();
  
  fcntl(logpipe,F_SETFL,O_NONBLOCK); // don't block waiting for output

  ofstream qlogfile,ulogfile;
  if (ulogfname.size()) {
    seteuid(getuid());       // be root if we can, then switch to user
    setegid(usergid);
    seteuid(js.uid);
    ulogfile.open(ulogfname.c_str(),ios::app);
    seteuid(getuid());       // back to user voxbo if possible
    setegid(save_egid);
    seteuid(save_euid);
  }

  if (qlogfname.size())
    qlogfile.open(qlogfname.c_str(),ios::app);
  // set up to select
  int maxpipe=logpipe;
  if (cmdpipe>maxpipe) maxpipe=cmdpipe;
  fd_set readset,writeset;

  int nextline=0;
  while(1) {
    // wait until one of the pipes changes status
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    if (logpipe>-1)
      FD_SET(logpipe,&readset);
    if (cmdpipe>-1)
      FD_SET(cmdpipe,&writeset);
    select(maxpipe+1,&readset,&writeset,NULL,NULL);
    // check if we've been killed
    if (killme==1) {
      fprintf(stderr,"KILLING CHILD PROCESS %d\n",js.childpid);
      // be root again if that's how we started
      setegid(getgid());
      seteuid(getuid());
      kill(js.childpid,SIGHUP);
      setegid(save_egid);      // back to being user voxbo
      seteuid(save_euid);
      killme=2;
    }
    // SEND COMMANDS (at least one, if available)
    if (cmdpipe>0 && nextline<(int)myscript.size()) {
      // should rarely if ever block
      cnt=write(cmdpipe,(myscript[nextline]+"\n").c_str(),myscript[nextline].size()+1);
      if (cnt<0) {
        nextline=myscript.size();  // send no more
        close(cmdpipe);
        cmdpipe=-1;
      }
      else nextline++;
    }
    else if (cmdpipe>0) {
      close(cmdpipe);
      cmdpipe=-1;
    }
    // RECEIVE OUTPUT (as much as is available)
    cnt=read(logpipe,buf,STRINGLEN-1);
    buf[cnt]='\0';  // just in case!
    if (cnt<0 && errno==EAGAIN) continue;  // no data available
    if (cnt==0) // peer shut down
      return;
    if (cnt<0)  // other error, we're done
      return;  // FIXME any cleanup needed?
    // PROCESS THE OUTPUT (as much as we have)
    if (ulogfile) {
      // temporarily become user (effectively)
      seteuid(getuid());       // be root if we can, then switch to user
      setegid(usergid);
      seteuid(js.uid);
      ulogfile << buf << flush;
      seteuid(getuid());       // back to user voxbo if possible
      setegid(save_egid);
      seteuid(save_euid);
    }
    if (qlogfile)
      qlogfile << buf << flush;
    bufset.clear();
    bufset.ParseLine(buf);
    for (size_t i=0; i<bufset.size(); i++) {
      bufstr=bufset[i];
      test_outputline(js,bufstr);
      // FIXME the following was causing segfaults
      // if (sscanf(buf,"Percent done: %d",&pct) == 1) {
      //   tell_scheduler((string)"setjobinfo "+strnum(js.snum)+" "+
      //                  strnum(js.jnum)+" percentdone "+strnum(pct));
      // }
      // handle error lines
      if (bufstr.find(js.jt.err_tag) != string::npos) {
        js.error = strtol(buf+js.jt.err_tag.size()+1,NULL,10);
        if (js.error)
          js.SetState(XBad);
        else if (js.GetState() == XNone)  // don't overwrite a bad state
          js.SetState(XGood);
      }
      // handle message lines
      else if (bufstr.find(js.jt.msg_tag) != string::npos) {
        if (js.jt.msg_tag.size() < bufstr.size())
          js.errorstring=bufstr;
      }
      // handle warning lines
      else if (bufstr.find(js.jt.warn_tag) != string::npos) {
        // default msg
        js.errorstring="Job-specific warning generated -- see log file";
        if (js.jt.warn_tag.size() < bufstr.size())
          js.errorstring=bufstr;
        if (js.GetState() == XNone)
          js.SetState(XWarn);
      }
      // handle retry lines
      else if (bufstr.find(js.jt.retry_tag) != string::npos) {
        js.SetState(XRetry);
        js.retrycount=0;
      }
    }
    usleep(200000);  // minimize damage due to busy waiting
  }
}

void
test_outputline(VBJobSpec &js,string &line)
{
  for (TGI tgi=js.jt.triggerlist.begin(); tgi!=js.jt.triggerlist.end(); tgi++) {
    if (tgi->cond=="match") {
      if (line.find(tgi->condvalue)!=string::npos) {
        execute_action(js,line,tgi);
      }
    }
    else if (tgi->cond=="") {
      // FIXME ETC.
    }
  }
}

void
execute_action(VBJobSpec &js,string &line,TGI tgi)
{
  if (tgi->action=="fail") {
    js.SetState(XBad);
    js.errorstring=line;
  }
  else if (tgi->action=="succeed") {
    js.SetState(XGood);
    js.errorstring=line;
  }
  else if (tgi->action=="retry") {
    js.SetState(XRetry);
    js.errorstring=line;
    js.retrycount=strtol(tgi->actionvalue);
  }
  else if (tgi->action=="warn") {
    js.SetState(XWarn);
    js.errorstring=line;
  }
  else if (tgi->action=="saveline") {
    if (js.f_cluster)
      tell_scheduler(js.queuedir,js.hostname,(string)"saveline "+line);
  }
}

void
parse_status(VBJobSpec &js,int status)
{
  char tmp[STRINGLEN];

  if (WIFEXITED(status)) {
    // use err code if there was no return code in the log
    if (js.GetState() == XNone) {
      js.error=WEXITSTATUS(status);
      if (js.error)
        js.SetState(XBad);
      else
        js.SetState(XGood);
      if (!js.errorstring[0]) {
        sprintf(tmp,"Your job returned error code %d.\n",js.error);
        js.errorstring=tmp;
      }
    }
  }
  else if (WIFSIGNALED(status)) {
    js.SetState(XSignal);
    sprintf(tmp,"Your job terminated by signal %d.\n",WTERMSIG(status));
    js.errorstring=tmp;
  }
  else if (WCOREDUMP(status)) {
    js.SetState(XBad);
    js.errorstring="Your job dumped core.\n";
  }
  else {           // no way of getting a valid return code
    js.SetState(XBad);
    js.errorstring="Your job terminated for reasons unknown.\n";
  }
}

void
do_internal(VBJobSpec &js)
{
  fprintf(stderr,"internal jobtype %s\n",js.jobtype.c_str());
  if (js.jobtype=="timewaster") {
    if (js.arguments.size() < 1) {
      fprintf(stderr,"timewaster failed -- duration < 1s\n");
      return;
    }
    int secondstowaste=strtol(js.arguments["time"]);
    sleep(secondstowaste);
    fprintf(stderr,"Log message via stderr.\n");
    fprintf(stdout,"Log message via stdout.\n");

    fprintf(stdout,"Here's your environment.\n");
    int i=0;
    while (environ[i])
      fprintf(stdout,"%s\n",environ[i++]);

    if (secondstowaste % 2) {
      printf("Wasting an odd number of seconds is very curious.\n");
    }
    else {
      printf("Wasting an even number of seconds is safe and productive.\n");
    }
  }
  else if (js.jobtype=="notify" && js.f_cluster) {
    string msg="email "+js.arguments["email"]+"\n";
    msg+="To: "+js.arguments["email"]+" (Happy VoxBo User)\n";
    msg+="Subject: VoxBo Sequence \""+js.seqname+"\" ("+strnum(js.snum)+")\n";
    msg+="Reply-To: nobody@nowhere.com\n";
    msg+="Return-Path: nobody@nowhere.com\n";
    msg+="\n";
    msg+=js.arguments["msg"];
    msg+="\n";
    tell_scheduler(js.queuedir,js.hostname,msg);
  }
  else {
    fprintf(stderr,"[E] vbx: unknown built in jobtype %s\n",js.jobtype.c_str());
    // controlmsg("return",5);
  }
}

vector<string>
build_script(VBJobSpec &js,int ind)
{
  tokenlist args,tmpa;
  map<string,string> nullargs;
  string scriptx;
  vector<string> myscript;
  if (ind<0) return myscript;  // for built-in commands

  tmpa.SetQuoteChars("");

  // build null argument tokenlist to make sure any declared but un-set variables
  // get replaced with whitespace
  for (int i=0; i<(int)js.jt.arguments.size(); i++)
    nullargs[js.jt.arguments[i].name]="";

  for (int i=0; i<(int)js.jt.commandlist[ind].script.size(); i++) {
    scriptx=js.jt.commandlist[ind].script[i];
    fill_vars2(scriptx,js.arguments);  // replace args
    fill_vars2(scriptx,envmap(environ));       // replace env vars
    fill_vars2(scriptx,nullargs);      // null what's left
    myscript.push_back(scriptx);
  }
  return myscript;
}

void
exec_command(VBJobSpec &js,vector<string>script,int ind)
{
  string commandx,scriptx;

  // default dir, we'll also check for variable DIR below
  chdir(js.dirname.c_str());

  // build argument tokenlist
  tokenlist args,tmpa;
  map<string,string> nullargs;
  tmpa.SetQuoteChars("");
  fprintf(stderr,"working directory: %s\n",js.dirname.c_str());
  if (js.arguments.size())
    fprintf(stderr,"arguments:\n");
  pair<string,string> arg;
  vbforeach(arg,js.arguments) {
    fprintf(stderr,"    %s=%s\n",arg.first.c_str(),arg.second.c_str());
    if (arg.first=="DIR")
      chdir(arg.second.c_str());
  }
  // build null argument tokenlist to make sure any declared but un-set variables
  // get replaced with whitespace
  for (size_t i=0; i<js.jt.arguments.size(); i++)
    nullargs[js.jt.arguments[i].name]="";
  // FIXME re-institute $(*) ???
  // FIXME also handle $$ for the "tail"???  needed?

  commandx=js.jt.commandlist[ind].command;
  fill_vars2(commandx,js.arguments);    // match jobtype arguments
  fill_vars2(commandx,envmap(environ)); // match env vars
  fill_vars2(commandx,nullargs);        // null what's left
  
  // spew out some context
  fprintf(stderr,"script command: %s\n",commandx.c_str());
  fprintf(stderr,"below is the output of your job, with script input tagged [S]\n");
  fprintf(stderr,"----BEGIN------------------------------\n");
  for (int i=0; i<(int)script.size(); i++)
    fprintf(stderr,"[S] %s\n",script[i].c_str());
  // args.ParseLine(commandx);
  // char *argv[commandx.size()+1];
  // for (int i=0; i<args.size(); i++)
  //   argv[i]=(char *)args(i);
  // argv[args.size()]=NULL;
  execlp("/bin/sh","sh","-c",commandx.c_str(),(char *)NULL);
  exit(127);   // should never reach here
  return;
}

// status and return codes go back via the pipes, not via the
// return/status of the child.  we check that just to detect those
// kinds of crashes.  otherwise we should have recq

void
tell_scheduler(string qdir,string hostname,string buf)
{
  chdir(qdir.c_str());
  string root=uniquename(hostname);
  string name1=root+".vbtmp";
  string name2=root+".vbx";
  struct stat st1,st2;
  if (!(stat(name1.c_str(),&st1)) |  !(stat(name2.c_str(),&st2))) {
    fprintf(stderr,"*** serious error, duplicate msg file name\n");
    return;
  }
  FILE *fp=fopen(name1.c_str(),"w");
  if (fp) {
    int err=fwrite(buf.c_str(),1,buf.size(),fp);
    if (err!=(int)buf.size())
      fprintf(stderr,"*** possibly serious error, msg truncated\n");
    fclose(fp);
    rename(name1.c_str(),name2.c_str());
  }
  else
    fprintf(stderr,"*** serious error, couldn't create msg file %s\n",name1.c_str());
  return;
}






void
signal_handler(int signal)
{
  if (signal==SIGUSR1)
    killme=1;
  return;
}












int
runseq(VBPrefs &vbp,VBSequence &seq,uint32 njobs)
{
  if (seq.specmap.size()<1) {
    cout << format("[E] no jobs to run\n");
    return 1;
  }
  // if no logdir specified, do something in /tmp
  string tmplogdir;
  if (seq.specmap.begin()->second.logdir.empty()) {
    tmplogdir=uniquename("/tmp/vbbatch");
    createfullpath(tmplogdir);
  }

  if (njobs==0) njobs=ncores();

  int32 snum=seq.seqnum=(int32)time(NULL);  // almost guarnanteed unique
  bool f_quit=0;
  map<pid_t,VBJobSpec> pmap;
  int mystatus;
  pid_t mypid;
  // give each job a sensible username and seqnum, unset f_cluster, copy
  // over jobtype and tmplogdir if needed
  for (SMI js=seq.specmap.begin(); js!=seq.specmap.end(); js++) {
    js->second.snum=snum;
    js->second.owner=vbp.username;
    if (tmplogdir.size()) js->second.logdir=tmplogdir;
    js->second.f_cluster=0;   // no contacting scheduler
    if (vbp.jobtypemap.count(js->second.jobtype))
      js->second.jt=vbp.jobtypemap[js->second.jobtype];
    else {
      cout << format("[E] your sequence has at least one unrecognized jobtype (%s)\n")%js->second.jobtype;
      if (tmplogdir.size()) rmdir_force(tmplogdir);
      return 101;
    }
  }

  while (1) {
    // cout << format("==> %d running %d avail\n")%pmap.size()%njobs;
    if (f_quit==0 && pmap.size()<njobs) {
      // start up as many more jobs as we can, using fork_command
      set<int32> newjobs=readyjobs(seq,njobs-pmap.size());
      vbforeach(int32 j,newjobs) {
        cout << format("[I] running job %d (%d total, %d running)\n")%j%seq.specmap.size()%(pmap.size()+1);
        VBJobSpec js=seq.specmap[j];
        mypid=fork();
        if (mypid<0) {  // bad, shouldn't happen
          exit(99);
        }
        if (mypid==0) { // child
          //if (!vbp.jobtypemap.count(js.jobtype))
          //_exit(-1);
          //js.jt=&(vbp.jobtypemap[js.jobtype]);   // can't really use vbp here
          run_voxbo_job(vbp,js);
          _exit(js.error);
        }
        pmap[mypid]=js;
        seq.specmap[j].status='R';
      }
    }

    if (pmap.empty())
      break;
    mypid=wait(&mystatus);  // for anything that might have finished
    if (!pmap.count(mypid)) {
      // FIXME shouldn't happen!
      string errtype="unknown";
      if (errno==ECHILD) errtype="no children";
      else if (errno==EINTR) errtype="interrupted";
      cout << format("[E] wait() failed (%d/%s)\n")%mypid%errtype;
      break;
    }
    int32 jnum=pmap[mypid].jnum;
    pmap.erase(mypid);
    if (WIFSIGNALED(mystatus))
      cout << "FIXME signaled " << WTERMSIG(mystatus) << endl;
    if (WIFEXITED(mystatus) && WEXITSTATUS(mystatus)==0) {
      seq.specmap[jnum].status='D';
      //cout << format("--> job %d terminated normally\n")%jnum;
      //unlink(seq.specmap[jnum].logfilename().c_str());
    }
    else {
      seq.specmap[jnum].status='B';
      cout << format("[E] job %d crashed with %d\n\n")%jnum%WEXITSTATUS(mystatus);
      if (f_quit==0) {
        seq.specmap[jnum].print();    // inside the test, because if
                                      // we've already quit, no more
                                      // detailed info on crashes
        string resp=vb_tolower(vb_getchar("\n[v]iew log, [e]dit log, [s]kip job, [r]etry job, [c]ontinue, or [q]uit: "));
        printf("\n\n");
        if (resp=="v") {
          printf("======================BEGIN LOG FILE======================\n");
          system(str(format("cat %s")%seq.specmap[jnum].logfilename()).c_str());
          printf("=======================END LOG FILE=======================\n");
        }
        else if (resp=="e") {
          if (fork()==0) {
            string editor;
            editor=getenv("VISUAL");
            if (!editor.size())
              editor=getenv("EDITOR");
            if (!editor.size())
              editor="emacs";
            system((str(format("%s %s")%editor%seq.specmap[jnum].logfilename()).c_str()));
          }
        }
        else if (resp=="s")
          seq.specmap[jnum].status='D';
        else if (resp=="r")
          seq.specmap[jnum].status='W';
        else if (resp=="q") {
          f_quit=1;
          if (pmap.size())
            cout << "[I] waiting for the rest of your jobs to complete...\n";
        }
      }
      //unlink(seq.specmap[jnum].logfilename().c_str());
    }
  }
  seq.updatecounts();
  if (tmplogdir.size()) rmdir_force(tmplogdir);
  if (seq.jobcnt!=seq.donecnt) {
    cout << format("[I] done: out of %d jobs, %d completed and %d went bad\n")%
      seq.jobcnt%seq.donecnt%seq.badcnt;
    return 102;
  }
  cout << "[I] done: all of your jobs have completed\n";
  return 0;
}


// FIXME the following could probably be used by scheduler

set<int32>
readyjobs(VBSequence &seq,uint16 max)
{
  set<int32> readyset;
  for (SMI j=seq.specmap.begin(); j!=seq.specmap.end(); j++) {
    if (j->second.status!='W') continue;
    bool unmet=0;
    vbforeach(int32 ww,j->second.waitfor) {
      if (seq.specmap[ww].status!='D') {
        unmet=1;
        break;
      }
    }
    if (!unmet) {
      readyset.insert(j->second.jnum);
      if (readyset.size()>=max)
        return readyset;
    }
  }
  return readyset;
}
