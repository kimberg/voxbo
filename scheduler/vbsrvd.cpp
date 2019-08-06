
// vbsrvd.cpp
// The VoxBo server
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

#include <pwd.h>
#include <signal.h>
#include <sys/time.h>
#include "vbx.h"

void vbsrvd_phone_home();
void vbsrvd_phone_home(tokenlist &args);
float server_check_load();
int server_check_running(int snum, int jnum, pid_t pid);
int server_check_resource(const string &command);
void do_errornotify(VBJobSpec &js);
int load_job_spec(VBJobSpec &js);
void job_cleanup(VBJobSpec &js);
void cleanup_update(VBJobSpec &js);
void vbsrvd_help();
void vbsrvd_version();
void server_run_benchmark(string name, string cmd);
void server_kill_job(tokenlist &args);

VBPrefs vbp;

int main(int argc, char *argv[]) {
  string fname;
  const int BUFSIZE = 128;
  char buf[BUFSIZE];
  tokenlist args;
  int cnt;

  // kill buffers so that we can reassign stdio below
  // setbuf(stdout,(char *)NULL);
  setbuf(stderr, (char *)NULL);

  // when running as a server, trust no env vars
  char **ee = environ;
  set<string> evars;
  while (ee[0]) {
    evars.insert(ee[0]);
    ee++;
  }
  vbforeach(string s, evars) putenv((char *)(s.substr(0, s.find("="))).c_str());
  // we used to just remove these four dangerous ones
  // putenv((char *)"USER");
  // putenv((char *)"HOME");
  // putenv((char *)"LOGNAME");
  // putenv((char *)"MAIL");

  // FIXME if vbp.init() generates any messages, they will go back
  // over the socket via xinetd
  vbp.init();

  // be user voxbo
  seteuid(getuid());
  setegid(vbp.voxbogid);
  seteuid(vbp.voxbouid);
  // live in the queue dir
  chdir(vbp.queuedir.c_str());

  struct passwd *vbpw = getpwuid(vbp.voxbouid);
  vbp.username = vbpw->pw_name;

  fname = vbp.rootdir + "/etc/logs/" + vbp.thishost.nickname + ".log";
  umask(022);
  FILE *fp = fopen(fname.c_str(), "a");
  chown(fname.c_str(), vbp.voxbouid, vbp.voxbogid);
  if (fp) {
    dup2(fileno(fp), 2);
    fclose(fp);
  } else
    exit(5);  // FIXME or maybe it's a good idea

  // read jobtypes and serverfile (to get local host config).
  // FIXME: we should probably put the jobtypes off until we're sure
  // we need them
  vbp.read_jobtypes();
  vbp.read_serverfile();

  signal(SIGPIPE, SIG_IGN);  // sockets generate random sigpipes
  // signal(SIGCHLD,SIG_IGN) -- ignoring SIGCHLD prevents zombie
  // processes if we're not going to wait for forked children to exit.
  // since we do wait right now, the line is commented out

  // process command-line arguments
  if (argc > 1) {
    tokenlist args;
    args.Transfer(argc - 1, argv + 1);
    for (size_t i = 0; i < args.size(); i++) {
      if (args[i] == "-j" && i < args.size() - 2) {
        VBSequence seq(vbp, strtol(args[i + 1]), strtol(args[i + 2]));
        fprintf(stdout, "[I] %d jobs read\n", (int)seq.specmap.size());
        VBJobSpec js = seq.specmap.begin()->second;
        //         string fname=args[++i];
        //         if (js.ReadFile(fname)) {
        //           fprintf(stderr,"[E] vbsrvd: couldn't open job file
        //           %s\n",fname.c_str()); exit(5);
        //         }
        fprintf(stdout, "[I] vbsrvd: running job %05d-%03d\n", js.snum,
                js.jnum);
        if (vbp.jobtypemap.count(js.jobtype) == 0) {
          fprintf(stderr, "[E] vbsrvd: %s bad jobtype '%s'\n",
                  timedate().c_str(), js.jobtype.c_str());
          exit(105);
        }
        js.jt = vbp.jobtypemap[js.jobtype];
        fprintf(stderr, "[I] vbsrvd: asked to run job %s\n",
                js.basename().c_str());
        run_voxbo_job(vbp, js);
        job_cleanup(js);
        vbsrvd_phone_home();
      } else {
        fprintf(stderr, "[E] vbsrvd: called with invalid arguments\n");
        exit(5);
      }
    }
    exit(0);
  }

  while ((cnt = read(0, buf, BUFSIZE)) > 0) {
    buf[cnt] = '\0';  // make sure the input buffer is null terminated
    args.ParseLine(buf);
    string cmd = vb_toupper(args[0]);
    if (args.size() < 1)  // give up at first invalid request
      break;
    else if (cmd == "JOB") {  // job to run
      fprintf(stdout, "ACK");
      fputc('\0', stdout);
      fflush(stdout);
      VBJobSpec sp;
      if (!(load_job_spec(sp))) {
        fprintf(stderr, "[I] %s received job %s\n", timedate().c_str(),
                sp.basename().c_str());
        run_voxbo_job(vbp, sp);
        job_cleanup(sp);
      }
      // vbsrvd_phone_home();
      break;
    } else if (cmd == "PHONEHOME") {
      fprintf(stdout, "ACK");
      fputc('\0', stdout);
      fflush(stdout);
      vbsrvd_phone_home(args);
    } else if (cmd == "BENCHMARK") {
      server_run_benchmark(args[1], args.Tail(2));
      break;
    } else if (cmd == "TEST") {  // test
      fprintf(stdout, "ACK");
      fputc('\0', stdout);
      fflush(stdout);
    } else if (cmd == "KILLJOB") {
      server_kill_job(args);
      break;
    } else if (cmd == "QUIT") {  // just quit baby
      break;
    }
  }
  exit(0);
}

// load up the actual job info and match it with a jobtype

int load_job_spec(VBJobSpec &js) {
  const int MAXBUF = 32768;
  char buf[MAXBUF];
  tokenlist args, lines;
  lines.SetSeparator("\n");
  args.SetSeparator(" \t");
  lines.SetQuoteChars("");
  args.SetQuoteChars("");

  js.init();
  if (safe_recv(1, buf, MAXBUF, 10.0) < 8) {
    printf("NAK");
    fputc('\0', stdout);
    fflush(stdout);
    return 101;
  }
  lines.ParseLine(buf);
  if (lines.size() == 0) {
    printf("NAK");
    fputc('\0', stdout);
    fflush(stdout);
    return 101;
  }
  int valid = 0;
  for (size_t i = 0; i < lines.size(); i++) {
    args.ParseLine(lines[i]);
    if (args[0] == "EOJ") {
      valid = 1;
      break;
    }
    // fprintf(stderr,"%s %s\n",args(0),args(1));
    if (args.size() < 2 && args[0] != "argument")  // FIXME
      continue;
    if (args[0] == "snum")
      js.snum = strtol(args[1].c_str(), NULL, 10);
    else if (args[0] == "uid")
      js.uid = strtol(args[1].c_str(), NULL, 10);
    else if (args[0] == "number")
      js.jnum = strtol(args[1].c_str(), NULL, 10);
    else if (args[0] == "email")
      js.email = args[1];
    else if (args[0] == "logdir")
      js.logdir = args[1];
    else if (args[0] == "seqname")
      js.seqname = args.Tail();
    else if (args[0] == "name")
      js.name = args.Tail();
    else if (args[0] == "jobtype")
      js.jobtype = args[1];
    else if (args[0] == "dirname")
      js.dirname = args.Tail();
    else if (args[0] == "argument") {
      js.arguments[args[1]] = args.Tail(2);
    }
  }
  if (!valid) {
    printf("NAK");
    fputc('\0', stdout);
    fflush(stdout);
    fprintf(stderr, "[E] vbsrvd: %s incomplete job\n", timedate().c_str());
    return 110;
  }

  // if jobtype isn't found, we'll still ACK, but go bad later when we try to
  // run it
  if (vbp.jobtypemap.count(js.jobtype) == 0)
    fprintf(stderr, "[E] %s bad jobtype '%s'\n", timedate().c_str(),
            js.jobtype.c_str());
  else
    js.jt = vbp.jobtypemap[js.jobtype];
  printf("ACK");
  fputc('\0', stdout);
  fflush(stdout);
  return 0;
}

// FIXME following two functions should be in vbhost
int server_check_resource(const string &command) {
  FILE *fp;
  struct stat st;
  char buf[STRINGLEN];

  // command must exist
  if (stat(command.c_str(), &st)) return (0);
  // command must be a regular file
  if (!S_ISREG(st.st_mode)) return (0);
  fp = popen(command.c_str(), "r");
  if (!fp) return (0);
  while (fgets(buf, STRINGLEN, fp)) {
  };
  int status = pclose(fp);
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return 0;
}

void server_run_benchmark(string name, string cmd) {
  struct timeval tstart, tend;
  gettimeofday(&tstart, NULL);
  system(cmd.c_str());
  gettimeofday(&tend, NULL);
  struct timeval elapsed = tend - tstart;
  fprintf(stderr, "[I] %s BENCHMARK: %s %d.%06d\n", timedate().c_str(),
          name.c_str(), (int)elapsed.tv_sec, (int)elapsed.tv_usec);
}

float server_check_load() {
#ifdef CYGWIN
  return 0.0;
#else
  double la[3];
  if (getloadavg(la, 3) > 0)
    return la[0];
  else
    return 0.0;
#endif
}

void server_kill_job(tokenlist &args) {
  pid_t parentpid = strtol(args[1]);
  pid_t childpid = strtol(args[2]);

  // just in case, never kill init process!
  if (parentpid < 2 || childpid < 2) {
    printf("ACK\n");
    fputc('\0', stdout);
    return;
  }

  // definitely kill vbsrvd, probably kill child
  killpg(parentpid, SIGUSR1);
  kill(parentpid, SIGUSR1);
  usleep(100000);
  kill(childpid, SIGUSR1);
  usleep(100000);
  if (kill(childpid, 0) == -1 && errno == ESRCH) {
    printf("ACK\n");
    fputc('\0', stdout);
    return;
  }
  // child didn't die, let's throw everything at it, in order
  killpg(parentpid, SIGHUP);
  killpg(parentpid, SIGTERM);
  killpg(parentpid, SIGKILL);
  kill(childpid, SIGHUP);
  kill(childpid, SIGINT);
  kill(childpid, SIGTERM);
  kill(childpid, SIGKILL);
  sleep(1);
  if (kill(childpid, 0) == -1 && errno == ESRCH) {
    printf("ACK\n");
    fputc('\0', stdout);
    return;
  }
  fprintf(stderr, "[E] %s process %d wouldn't die\n", timedate().c_str(),
          (int)childpid);
  printf("ACK\n");
  fputc('\0', stdout);
}

void job_cleanup(VBJobSpec &js) {
  // be voxbo user to clean up
  seteuid(getuid());
  setegid(vbp.voxbogid);
  seteuid(vbp.voxbouid);
  umask(022);
  cleanup_update(js);  // update the sequence file

  // notify if necessary
  if (js.GetState() == XBad || js.GetState() == XWarn ||
      js.GetState() == XSignal)
    do_errornotify(js);

  // go back to being whoever we are
  seteuid(getuid());
  setegid(getgid());
}

void do_errornotify(VBJobSpec &js) {
  if (js.email == "none") return;

  FILE *fp;
  char str[STRINGLEN];
  string msg;

  msg = "email " + js.email + "\n";

  switch (js.GetState()) {
    case XBad:
      msg += "To: " + js.email + " (Sad VoxBo User)\n";
      msg += "Subject: Error in VoxBo Sequence \"" + js.seqname + "\" (job " +
             js.basename() + ")\n";
      msg += "Reply-To: nobody@nowhere.com\n";
      msg += "Return-Path: " + js.email + "\n";
      msg += "\n";
      break;
    case XSignal:
      msg += "To: " + js.email + " (Besieged VoxBo User)\n";
      msg += "Subject: VoxBo Sequence \"" + js.seqname + "\" (job " +
             js.basename() + ")\n";
      msg += "Reply-To: nobody@nowhere.com\n";
      msg += "Return-Path: " + js.email + "\n";
      msg += "\n";
      break;
    case XWarn:
      msg += "To: " + js.email + " (Concerned VoxBo User)\n";
      msg += "Subject: Warning concerning VoxBo Sequence \"" + js.seqname +
             "\" (job " + js.basename() + ")\n";
      msg += "Reply-To: nobody@nowhere.com\n";
      msg += "Return-Path: " + js.email + "\n";
      msg += "\n";
      break;
    case XGood:
    default:
      msg += "To: " + js.email + " (Alert VoxBo User)\n";
      msg += "Subject: Status Update on VoxBo Sequence \"" + js.seqname +
             "\" (job " + js.basename() + ")\n";
      msg += "Reply-To: nobody@nowhere.com\n";
      msg += "Return-Path: " + js.email + "\n";
      msg += "\n";
      break;
  }

  msg += "Your VoxBo job, described as \"" + js.name +
         ",\" returned error code " + strnum(js.error) +
         " with the following message:\n\n";
  msg += js.errorstring + "\n\n" + "Here is the log your job generated:\n\n";

  fp = fopen(js.logfilename().c_str(), "r");
  if (fp) {
    while (fgets(str, STRINGLEN, fp) != NULL) {
      for (int i = 0; i < (int)js.jt.nomail.size(); i++) {
        if (strncmp(str, js.jt.nomail[i].c_str(), js.jt.nomail[i].size()))
          continue;
      }
      msg += str;
    }
    fclose(fp);
  }
  tell_scheduler(vbp.queuedir, vbp.thishost.nickname, msg);
}

void cleanup_update(VBJobSpec &js) {
  // send local finished time and new status to scheduler
  if (js.GetState() == XGood || js.GetState() == XWarn) {
    tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                   (string) "setjobinfo " + strnum(js.snum) + " " +
                       strnum(js.jnum) + " status D");
    tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                   (string) "jobdone " + strnum(js.snum) + " " +
                       strnum(js.jnum) + " " + strnum(time(NULL)));
  } else if (js.GetState() == XRetry) {
    tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                   (string) "retry " + strnum(js.snum) + " " + strnum(js.jnum) +
                       " " + strnum(js.retrycount));
  } else {  // XBad, XSignal, and XNone
    tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                   (string) "setjobinfo " + strnum(js.snum) + " " +
                       strnum(js.jnum) + " status B");
    tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                   (string) "jobdone " + strnum(js.snum) + " " +
                       strnum(js.jnum) + " " + strnum(time(NULL)));
  }
}

void vbsrvd_phone_home() {
  tokenlist nojobs;
  vbsrvd_phone_home(nojobs);
}

void vbsrvd_phone_home(tokenlist &args) {
  // update our schedule, number of cpus, etc.
  vbp.thishost.CheckSchedule();
  // load average
  vbp.thishost.loadaverage = server_check_load();
  // update resource counts for those that have commands
  for (RI rr = vbp.thishost.resources.begin();
       rr != vbp.thishost.resources.end(); rr++)
    if (rr->second.command.size())
      rr->second.cnt = server_check_resource(rr->second.command);
  // check on directories that are supposed to be non-empty
  bool dirsok = 1;
  vbforeach(string dd, vbp.thishost.checkdirs) {
    if (vglob(dd + "/*").size() < 1) {
      dirsok = 0;
      break;
    }
  }
  if (!dirsok)
    vbp.thishost.status = "dead";
  else
    vbp.thishost.status = "up";
  // check on jobs supposedly running here
  map<jobid, VBJobSpec> foundjobs;
  VBJobSpec js;
  js.hostname = vbp.thishost.nickname;
  for (size_t i = 1; i < args.size(); i += 3) {
    js.snum = strtol(args[i]);
    js.jnum = strtol(args[i + 1]);
    js.pid = strtol(args[i + 2]);
    if (js.pid == 0) continue;
    if (kill(js.pid, 0) == -1)
      if (errno == ESRCH) continue;
    foundjobs[jobid(js.snum, js.jnum)] = js;
  }
  // send the host update
  tell_scheduler(vbp.queuedir, vbp.thishost.nickname,
                 (string) "hostupdate " + vbp.thishost.tobuffer(foundjobs));
}

void vbsrvd_help() {
  printf("\nVoxBo vbsrvd (v%s)\n", vbversion.c_str());
  printf("usage:\n");
  printf("  vbsrvd <flags>\n");
  printf("flags:\n");
  printf("  -j <file>      process job file\n");
  printf("  -h             help\n");
  printf("  -v             version\n");
  printf("notes:\n");
  printf(
      "   vbsrvd is normally invoked via (x)inetd or internally in "
      "single-user\n");
  printf(
      "   mode.  if you invoke it yourself via the command line, you should "
      "\n");
  printf("   already know what you're doing.\n");
  printf("\n");
}

void vbsrvd_version() { printf("VoxBo vbsrvd (v%s)\n", vbversion.c_str()); }
