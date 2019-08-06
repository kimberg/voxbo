
// voxbo.cpp
// VoxBo job scheduling
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

#include <signal.h>
#include <sys/signal.h>
#include <sys/un.h>
#include <list>
#include <map>
#include "schedlib.h"
#include "vbjobspec.h"
#include "vbprefs.h"
#include "vbutil.h"

using namespace std;
using boost::format;

// GLOBALS!
map<jobid, VBJobSpec> runningmap;
VBPrefs vbp;
list<VBHost> hostlist;
map<int, VBSequence> seqlist;
enum { SERVER_OK, SERVER_DIE };
bool f_debug = 0;
bool f_running = 1;
int f_totalcpus = 0;  // total available CPUs, from host responses
const int S_MISSING =
    180;  // seconds since last response before a job is considered missing

// prototypes for strictly internal functions

int server_sleep(int s);
int server_create();
int server_create_inet();
int server_create_unix();
int getvoxbolock();
void returnvoxbolock();
int reassignstdio(string &fname);
void fork_send_voxbo_job(VBJobSpec &js, HI &myhost);
int send_voxbo_job(int socket, VBJobSpec &js);
void send_benchmarks();
void ping_hosts();
void ping_hosts_inet();
void ping_hosts_unix();
void run_jobs();

void update_hostlist();
void check_sent();

void setjobinfo_update_hostlist(VBJobSpec *newjs);

void process_vbx();
void process_dropbox();
int process_jobrunning(string hostname, int snum, int jnum, pid_t pid,
                       pid_t childpid, long stime);
int process_jobdone(int snum, int jnum, long ftime);
int process_setseqinfo(int ss, string newstatusinfo);
int process_killsequence(int ss, string newstatus);
int process_setjobinfo(int ss, int jj, string newstatusinfo);
int process_email(string recipient, string fname);
int process_adminemail(string fname);
int process_hostupdate(string hh);
int process_retry(string line);
int do_retry(int snum, int jnum, int generations);
int process_saveline(string line);

// int send_email(string msg);

string handle_server_setsched(tokenlist &args, string username);
string handle_server_gimme(tokenlist &args, string username);
string handle_server_changejobstatus(tokenlist &args, string username);
string handle_server_setseqinfo(tokenlist &args, string username);
string handle_server_killsequence(tokenlist &args, string username);
int handle_server_sendfile(int ns, string username);
void populate_running_jobs(map<int, VBSequence> &seqlist);
void handle_server_submit(int ns, string fname);

int haltsequence(tokenlist &args, string username);

void addrunningjob(VBJobSpec &js, string &hostname);
void removerunningjob(jobid jid);
void removerunningjob(int snum, int jnum);
map<string, int> availableresources();

int server_add(string nickname);
int server_delete(string nickname);
void send_hosts(int ns);
void send_sequences(int ns);
void read_serverlist();
void print_diagnostics(string arg);
void vbscheduler_help();
void vbscheduler_version();

VBHost *findhost(string hname);

// VBevents are maintained by the running scheduler for about a day

class VBevent {
 public:
  string type;
  time_t etime;
  int32 snum, jnum;
  VBevent(int32 ss, int32 jj, string ty, time_t ti) {
    snum = ss;
    jnum = jj;
    type = ty;
    etime = ti;
  }
};

deque<VBevent> dayevents;
deque<VBevent> hourevents;

int main(int argc, char *argv[]) {
  string logfile, pidfile;
  int newpid, mysocket, err;
  struct stat st;
  FILE *fp;
  int f_detach = 0;
  int f_usestdio = 1;
  string f_myqueue;

  // ignoring SIGCHLD avoids zombies when we don't wait()
  signal(SIGCHLD, SIG_IGN);
  // ignoring SIGPIPE avoids issue when popen command is missing
  signal(SIGPIPE, SIG_IGN);
  // in case we're going to send stdout and stderr to a file -- kill
  // the buffers to make sure everything comes out in order
  setbuf(stdout, (char *)NULL);
  setbuf(stderr, (char *)NULL);

  // parse arguments
  tokenlist args;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      vbscheduler_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbscheduler_version();
      exit(0);
    } else if (args[i] == "-l" && i < args.size() - 1) {
      logfile = args[++i];
      f_usestdio = 0;
    } else if (args[i] == "-d") {
      f_detach = 1;
    } else if (args[i] == "-x") {
      f_debug = 1;
    } else if (args[i] == "-q" && i < args.size() - 1) {
      f_myqueue = args[++i];
    }
  }
  if (f_detach) f_usestdio = 0;

  vbp.init();

  if (f_myqueue.size()) {
    vbp.queuedir = f_myqueue;
  }
  vbp.read_jobtypes();
  // FIXME below should be moved to somewhere later, find another way
  // to get nickname
  // FIXME probably shouldn't read serverlist in single-user mode
  read_serverlist();

  // become user voxbo if possible
  setgid(vbp.voxbogid);
  setuid(vbp.voxbouid);

  if (getuid() != vbp.voxbouid) {
    cerr << format(
                "[E] only the voxbo user (uid=%d) may start the scheduler.\n") %
                vbp.voxbouid;
    exit(0);
  }

  // FIXME check to see if scheduler is already running

  if (f_detach) {
    newpid = fork();

    if (newpid == -1) {
      cerr << "error: scheduler couldn't fork\n";
    } else if (newpid != 0) {
      cerr << "Scheduler is started.\n";
      exit(0);
    }
  }

  if (!f_usestdio) {
    close(0);
    close(1);
    close(2);
  }

  if (f_detach)
    if (setsid() == -1) exit(5);

  umask(022);  // for now, log files etc can be read/written by whoever

  if (logfile.size() == 0) logfile = vbp.rootdir + "/etc/logs/voxbo.log";
  if (!f_usestdio)
    if (!reassignstdio(logfile)) exit(5);

  // sit in default queue dir
  chdir(vbp.queuedir.c_str());

  cerr << "==============================================\n";
  cerr << format("[I] %s VoxBo scheduler started on host %s\n") % timedate() %
              vbp.thishost.nickname;
  cerr << format("[I] %s version %s\n") % timedate() % vbversion;
  cerr << format("[I] %s Queue located at %s\n") % timedate() % vbp.queuedir;

  pidfile = vbp.rootdir + "/etc/scheduler.pid";  // record pid somewhere
  fp = fopen(pidfile.c_str(), "w");
  if (fp) {
    fprintf(fp, "%s %ld\n", vbp.thishost.hostname.c_str(), (long)getpid());
    fclose(fp);
  }

  mysocket = server_create();
  if (f_debug) printf("[D] voxbo: reading queue\n");
  read_queue(vbp.queuedir, seqlist);
  if (f_debug) printf("[D] voxbo: populating hostlist with running jobs\n");
  populate_running_jobs(seqlist);

  while (1) {
    if (f_debug) printf("[D] voxbo: XXX\n");

    // make sure we have a log file if we need it
    if (!f_usestdio) {
      if (stat(logfile.c_str(), &st) !=
          0) {  // if logfile has disappeared, try again
        if (!reassignstdio(logfile)) break;
      }
    }

    // process vbx notices, dropbox, update the hostlist, and clean up the queue
    process_vbx();
    process_dropbox();
    update_hostlist();
    cleanupqueue(seqlist, vbp.queuedir);

    // send benchmarks, pings, and jobs, then wait
    send_benchmarks();
    ping_hosts();
    if (f_running) {
      vbforeach(VBHost & h, hostlist) h.rand = VBRandom();
      hostlist.sort(cmp_host_pri_taken);
      run_jobs();
    }
    if (mysocket < 0) mysocket = server_create();
    err = server_sleep(mysocket);
    if (err == SERVER_DIE) break;
  }
  if (mysocket > -1) close(mysocket);
  unlink(pidfile.c_str());
  fprintf(stderr, "[I] %s voxbo scheduler terminated normally on host %s\n",
          timedate().c_str(), vbp.thishost.nickname.c_str());
  returnvoxbolock();
  exit(0);
}

void print_diagnostics(string arg) {
  string a = vb_tolower(arg);
  if (a.size() == 0) a = "all";
  if (a == "all" || a == "jt") {
    printf(
        "===========================> JOBTYPES <===========================\n");
    for (TI i = vbp.jobtypemap.begin(); i != vbp.jobtypemap.end(); i++)
      i->second.print();
  }
  if (a == "all" || a.substr(0, 3) == "seq") {
    printf(
        "===========================> SEQUENCES <==========================\n");
    for (SI seq = seqlist.begin(); seq != seqlist.end(); seq++)
      seq->second.print();
  }
  // FIXME replace this?
  //   if (a=="all" || a.substr(0,2)=="sj") {
  //     printf("===========================> JOBSPECS (short)
  //     <===================\n"); for (JLI j=speclist.begin();
  //     j!=speclist.end(); j++) {
  //       printf("[I] DIAG_SJ: %08d-%05d: [%c] jobtype=%s started=%d
  //       finished=%d host=%s\n",
  //              j->snum,j->jnum,j->status,j->jobtype.c_str(),
  //              (int)j->startedtime,(int)j->finishedtime,
  //              j->hostname.c_str());
  //     }
  //   }
  if (a == "all" || a.substr(0, 3) == "job") {
    printf(
        "===========================> JOBSPECS <===========================\n");
    pair<jobid, VBJobSpec> js;
    vbforeach(js, runningmap) {
      js.second.print();
      cout << time(NULL) - js.second.lastreport << endl;
    }
  }
  if (a == "all" || a.substr(0, 3) == "hos" || a.substr(0, 3) == "ser") {
    printf("===========================> HOSTS <===========================\n");
    for (HI h = hostlist.begin(); h != hostlist.end(); h++) h->print();
  }
}

int server_create() { return server_create_inet(); }

int server_create_inet() {
  int s, err, yes;
  struct sockaddr_in addr;

  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < 0) return s;
  yes = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = PF_INET;
  addr.sin_port = htons(vbp.serverport + 1);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  err = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (err == -1) {
    close(s);
    return -2;
  }
  err = listen(s, 10);
  if (err == -1) {
    close(s);
    return -3;
  }
  printf("Now listening to port %d\n", vbp.serverport + 1);
  return s;
}

int server_create_unix() {
  int s, err;
  struct sockaddr_un addr;
  char sockname[1024];
  sprintf(sockname, "/tmp/vbsocket.%s", vbp.username.c_str());

  unlink(sockname);
  s = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (s < 0) return s;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = PF_LOCAL;
  strcpy(addr.sun_path, sockname);
  err = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (err == -1) {
    close(s);
    return -2;
  }
  err = listen(s, 10);
  if (err == -1) {
    close(s);
    return -3;
  }
  printf("Now listening to UNIX domain socket\n");
  return s;
}

// server_sleep() -- this cute little function waits for someone to
// connect to the socket and send a message.  it currently is a bit
// lacking in the security department.  but it will eventually be
// rewritten to do cryptographic authentication.

int server_sleep(int s) {
  struct timeval tv;
  fd_set ff;
  int err, ns, len;
  socklen_t ssize;
  struct sockaddr_in addr;
  char buf[STRINGLEN];
  time_t start_time;
  tokenlist args;
  string username;

  ssize = sizeof(struct sockaddr_in);
  start_time = time(NULL);

  // if no socket, complain
  if (s < 0) {
    sleep(vbp.queuedelay);
    printf("error: couldn't create server port, waiting without listening\n");
    return SERVER_OK;
  }

  while (time(NULL) - start_time < vbp.queuedelay) {
    FD_ZERO(&ff);
    FD_SET(s, &ff);
    tv.tv_sec = vbp.queuedelay;
    tv.tv_usec = 0;
    err = select(s + 1, &ff, NULL, NULL, &tv);  // can we receive?
    if (err < 1) {
      sleep(1);
      continue;
    }
    ns = accept(s, (struct sockaddr *)&addr, &ssize);
    if (ns == -1) {
      sleep(1);
      continue;
    }

    // FIXME -- here is where we need to do authentication
    len = safe_recv(ns, buf, STRINGLEN - 1, 10.0);  // receive signon
    if (len < 1) {
      close(ns);
      continue;
    }
    args.ParseLine(buf);
    if (args.size()) username = args[0];

    send(ns, "ACK", 4, 0);

    len = safe_recv(ns, buf, STRINGLEN - 1, 10.0);  // receive instruction
    if (len == 0) {
      close(ns);
      continue;
    }
    args.ParseLine(buf);

    string rmsg;
    if (args.size() > 0) {
      string cmd = vb_toupper(args[0]);
      if (cmd == "DIE") {  // DIE
        send(ns, "ACK", 4, 0);
        close(ns);
        return SERVER_DIE;
      } else if (cmd == "GIMME") {  // GIMME <hostname> <hours>
        rmsg = handle_server_gimme(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "QUEUEON") {
        f_running = 1;
        send(ns, "ACK", 4, 0);
      } else if (cmd == "QUEUEOFF") {
        f_running = 0;
        send(ns, "ACK", 4, 0);
      } else if (cmd == "SUBMIT") {
        send(ns, "ok", 2, 0);
        fprintf(stderr, "[E] somebody sent a SUBMIT\n");
        // handle_server_submit(ns,args[1]);
      } else if (cmd == "SENDFILE") {
        handle_server_sendfile(ns, username);
      } else if (cmd == "GIVEBACK") {  // GIVEBACK <hostname>
        rmsg = handle_server_gimme(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "CHANGEJOBSTATUS") {
        rmsg = handle_server_changejobstatus(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "SETSEQINFO") {
        rmsg = handle_server_setseqinfo(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "SETSCHED") {
        rmsg = handle_server_setsched(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "KILLSEQUENCE") {
        rmsg = handle_server_killsequence(args, username);
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "HOSTS") {  // HOSTS
        send_hosts(ns);
      } else if (cmd == "SEQUENCES") {  // HOSTS
        send_sequences(ns);
      } else if (cmd == "ADDSERVER") {  // HOSTS
        if (server_add(args[1]))
          rmsg = "error adding server " + args[1];
        else
          rmsg = "server " + args[1] + " added successfully";
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "DELSERVER") {  // HOSTS
        if (server_delete(args[1]))
          rmsg = "error deleting server " + args[1];
        else
          rmsg = "server " + args[1] + " removed successfully";
        send(ns, rmsg.c_str(), rmsg.size() + 1, 0);
      } else if (cmd == "DIAG") {
        print_diagnostics(args[1]);
      } else if (cmd == "TEST") {
        send(ns, "ACK", 4, 0);
      } else if (cmd == "RESET") {
        printf("\n[I] %s scheduler reset begin\n", timedate().c_str());
        hostlist.clear();
        seqlist.clear();
        runningmap.clear();
        vbp.init();
        vbp.read_jobtypes();
        read_serverlist();
        ping_hosts();
        read_queue(vbp.queuedir, seqlist);
        populate_running_jobs(seqlist);
        printf("[I] %s scheduler reset complete\n\n", timedate().c_str());
      }
    }
    close(ns);
  }
  return SERVER_OK;
}

void send_hosts(int ns) {
  // CR-separated list of hosts, each line parseable by
  // VBHost.frombuffer()
  for (HI h = hostlist.begin(); h != hostlist.end(); h++) {
    if (h->valid == 0) continue;
    if (h->hostname.size() == 0) continue;
    if (h->nickname.size() == 0) continue;
    string buf = h->tobuffer(runningmap);
    send(ns, buf.c_str(), buf.size(), 0);
    send(ns, "\n", 1, 0);
  }
  send(ns, "\0", 1, 0);
}

void send_sequences(int ns) {
  char buf[STRINGLEN];
  string str;

  for (SI seq = seqlist.begin(); seq != seqlist.end(); seq++) {
    seq->second.updatecounts();
    sprintf(buf, "[name '%s']", seq->second.name.c_str());
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[num %d]", seq->second.seqnum);
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[priority %d]", seq->second.priority.priority);
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[owner %s]", seq->second.owner.c_str());
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[status %c]", seq->second.status);
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[idle %d]", (int)(time(NULL) - seq->second.modtime));
    send(ns, buf, strlen(buf), 0);
    sprintf(buf, "[counts %d %d %d %d %d]", seq->second.jobcnt,
            seq->second.waitcnt, seq->second.runcnt, seq->second.badcnt,
            seq->second.donecnt);
    send(ns, buf, strlen(buf), 0);
    send(ns, "[EOS]", 5, 0);
  }
  send(ns, "\0", 1, 0);
}

// handle_server_gimme()
// GIMME host <hrs>
// GIVEBACK host

string handle_server_gimme(tokenlist &args, string username) {
  VBHost *myhost;
  string rhost = args[1];
  if (!(myhost = findhost(rhost)))
    return (string) "host " + rhost + " not found";
  string gimfile = myhost->nickname + "." + username + ".gim";
  if (vb_tolower(args[0]) == "gimme") {
    if (args.size() != 3) return (string) "illegal reservation";
    if ((int)myhost->reservations.size() > myhost->total_cpus)
      return (string) "host " + myhost->nickname + " already totally reserved";
    time_t deadline = time(NULL) + (strtol(args[2]) * 3600);
    string gimfile = myhost->nickname + "." + username + ".gim";
    umask(022);
    FILE *fp = fopen(gimfile.c_str(), "w");
    if (fp) {
      fprintf(fp, "%s %ld %ld\n", username.c_str(), (long)time(NULL),
              (long)deadline);
      fclose(fp);
    }
    VBReservation rr;
    rr.owner = username;
    rr.start = time(NULL);
    rr.end = deadline;
    myhost->reservations.push_back(rr);
    return (string) "host " + rhost + " reserved successfully";
  } else if (vb_tolower(args[0]) == "giveback") {
    if (args.size() != 2) return (string) "illegal unreservation";
    int myres = -1;
    for (int i = 0; i < (int)myhost->reservations.size(); i++) {
      if (myhost->reservations[i].owner == username) myres = i;
    }
    if (myres == -1)
      return (string) "host " + myhost->nickname +
             " not currently reserved in your name";
    myhost->reservations.erase(myhost->reservations.begin() + myres);
    unlink(gimfile.c_str());
    return (string) "host " + myhost->nickname + " reserved";
  }
  return (string) "unfathomable error";
}

void send_benchmarks() {
  list<VBenchmark>::iterator bb;
  for (bb = vbp.benchmarks.begin(); bb != vbp.benchmarks.end(); bb++) {
    // invalid name
    if (bb->name == "") continue;
    // not yet yet scheduled
    if (bb->scheduled == 0) {
      // bb->scheduled=time(NULL)+(int)(VBRandom() & 0x1ff)-256;
      bb->scheduled = time(NULL) + bb->interval;
      continue;
    }
    // scheduled time not yet reached
    if (bb->scheduled > time(NULL)) continue;
    // run it!
    hostlist.sort(cmp_host_random);
    for (HI host = hostlist.begin(); host != hostlist.end(); host++) {
      if (host->status != "up") continue;
      if (host->loadaverage > 0.2) continue;
      // no more excuses, ask this host to benchmark itself
      int s = safe_connect(&(host->addr), 20.0);
      if (s < 0) continue;
      // sends this small shouldn't block, so we're okay not to select
      string msg;
      msg = "BENCHMARK " + bb->name + " " + bb->cmd;
      send(s, msg.c_str(), msg.size(), 0);
      close(s);
      bb->scheduled = time(NULL) + bb->interval;
      break;
    }
  }
}

// update all the hosts statuses, availability, etc

void ping_hosts() { ping_hosts_inet(); }

void ping_hosts_inet() {
  for (HI host = hostlist.begin(); host != hostlist.end(); host++) {
    // currently we don't skip host if it's "down," because we need to
    // know if jobs that may still be running on the host still are

    // for dead hosts, if it's been less than two minutes, skip
    if (host->status == "dead") {
      if (time(NULL) - host->lastresponse < 120) continue;
      host->lastresponse = time(NULL);  // using lastresponse as last check
    }
    // skip host if the last update process is still going
    if (host->updatepid != 0 && kill(host->updatepid, 0) != -1) continue;
    // skip host if we've had a response in the last 20 seconds -- the
    // dead check is because we just set lastresponse for dead hosts
    if (host->lastresponse != 0 && time(NULL) - host->lastresponse < 20 &&
        host->status != "dead")
      continue;
    host->updatepid = fork();
    // child (pid=0) should ping and exit, parent should continue
    // regardless of whether or not fork was successful
    if (host->updatepid == 0) {
      // use swap() trick below because clear() doesn't free memory.
      // note that there's a bad OS interaction here, because if the
      // pages are copy-on-write, we're better off not clearing
      // anything.  for now, let it pass.

      // list<VBSequence>().swap(seqlist);
      VBHost hh = *host;
      // list<VBHost>().swap(hostlist);
      hh.Ping(runningmap);
      _exit(0);
    }
  }
}

void ping_hosts_unix() {}

void update_hostlist() {
  for (HI host = hostlist.begin(); host != hostlist.end(); host++) {
    if (host->lastresponse != 0 && time(NULL) - host->lastresponse > 600) {
      host->status = "dead";
      host->lastresponse = time(NULL);  // stow last check
      vector<int> snums, jnums;
      // build list of jobs to mark waiting
      for (JI j = host->runninglist.begin(); j != host->runninglist.end();
           j++) {
        snums.push_back(j->snum);
        jnums.push_back(j->jnum);
      }
      // now mark them (can't do this in one step because
      // process_setjobinfo modifies the host's job list)
      for (int i = 0; i < (int)snums.size(); i++) {
        printf("[E] %s job %08d-%05d was running on dead host %s\n",
               timedate().c_str(), snums[i], jnums[i], host->hostname.c_str());
        process_setjobinfo(snums[i], jnums[i], "status W");
      }
    }
  }
}

void fork_send_voxbo_job(VBJobSpec &js, HI &myhost) {
  cerr << format("[I] %s sending job %05d-%05d to host %s\n") %
              timedate().c_str() % js.snum % js.jnum % js.hostname;

  js.startedtime = time(NULL);
  js.finishedtime = 0;
  js.serverstartedtime = 0;
  js.serverfinishedtime = 0;
  js.percentdone = -1;

  // flag startedtime
  int errs = 0;
  errs += process_setjobinfo(
      js.snum, js.jnum, (string) "startedtime " + strnum((int)js.startedtime));
  errs +=
      process_setjobinfo(js.snum, js.jnum, (string) "host " + myhost->nickname);
  errs += process_setjobinfo(js.snum, js.jnum, (string) "status R");

  if (errs) {
    cerr << format("[E] %s %d error(s) setting job info for %08d-%05d\n") %
                timedate() % errs % js.snum % js.jnum;
    process_setjobinfo(js.snum, js.jnum, (string) "status W");
    removerunningjob(js.snum, js.jnum);
  }

  pid_t mypid = fork();
  if (mypid < 0) {  // error
    cerr << format("[E] %s couldn't even fork\n") % timedate();
    process_setjobinfo(js.snum, js.jnum, (string) "status W");
    removerunningjob(js.snum, js.jnum);
    return;
  } else if (mypid == 0) {  // child
    // get socket
    int s = safe_connect(&myhost->addr, 3.0);
    if (s < 0) _exit(404);
    int ret = send_voxbo_job(s, js);
    _exit(ret);
  }
  // we are the parent
}

int send_voxbo_job(int socket, VBJobSpec &js) {
  string msg;
  char tmp[STRINGLEN];
  int err;
  tokenlist args;

  msg = "JOB\n";
  err = send(socket, msg.c_str(), msg.length() + 1, 0);
  if (err < 0) {
    printf("[E] %s couldn't send to server %s\n", timedate().c_str(),
           js.basename().c_str());
    return 109;
  }
  // recv the acknowledgment
  err = safe_recv(socket, tmp, STRINGLEN - 1, 10.0);
  if (err < 1) return 102;

  msg = "snum " + strnum(js.snum) + "\n";
  msg += "number " + strnum(js.jnum) + "\n";
  msg += "uid " + strnum((int)js.uid) + "\n";
  msg += "actualcpus " + strnum(js.actualcpus) + "\n";
  msg += "email " + js.email + "\n";
  msg += "logdir " + js.logdir + "\n";
  msg += "seqname " + js.seqname + "\n";
  msg += "name " + js.name + "\n";
  msg += "jobtype " + js.jobtype + "\n";
  msg += "dirname " + js.dirname + "\n";
  pair<string, string> pp;
  vbforeach(pp, js.arguments) msg +=
      "argument " + pp.first + " " + pp.second + "\n";
  msg += "EOJ\n";
  err = send(socket, msg.c_str(), msg.length() + 1, 0);
  if (err < (int)msg.length() + 1) {
    printf("[E] %s failed to send entire job %s\n", timedate().c_str(),
           js.basename().c_str());
    return 101;
  }
  err = safe_recv(socket, tmp, STRINGLEN - 1, 10.0);
  if (err < 1) {
    printf("[E] %s no acknowledgment for job %s\n", timedate().c_str(),
           js.basename().c_str());
    return 109;
  }
  tmp[err] = '\0';
  args.ParseLine(tmp);
  if (args[0] == "NAK") {
    fprintf(stderr, "[E] %s received NAK from %s for job %s\n",
            timedate().c_str(), js.hostname.c_str(), js.basename().c_str());
    return 122;
  }

  // acknowledge the response (w/pid) from the server if we get one
  if (args[0] != "ACK")
    fprintf(stderr, "[E] %s received non-ack (%s) from %s for job %s\n",
            timedate().c_str(), args.Tail(0).c_str(), js.hostname.c_str(),
            js.basename().c_str());
  return 0;
}

// getvoxbolock() returns 1 if we are able to be the only voxbo,
//     by locking the log file

int getvoxbolock() {
  struct flock numlock;

  numlock.l_type = F_WRLCK;
  numlock.l_start = 0;
  numlock.l_len = 1;
  numlock.l_whence = SEEK_SET;
  if (fcntl(1, F_SETLK, &numlock) == -1)  // lock stdout, ie the log file
    return 0;
  return 1;
}

void returnvoxbolock() {
  struct flock numlock;

  numlock.l_type = F_UNLCK;
  numlock.l_start = 0;
  numlock.l_len = 1;
  numlock.l_whence = SEEK_SET;
  fcntl(1, F_SETLK, &numlock);
}

VBHost *findhost(string hname) {
  for (HI host = hostlist.begin(); host != hostlist.end(); host++) {
    if (host->nickname == hname || host->hostname == hname) return &(*(host));
  }
  return NULL;
}

int reassignstdio(string &logfile) {
  int fd;

  close(0);
  close(1);
  close(2);
  FILE *fp = fopen(logfile.c_str(), "a");
  if (fp) {
    fd = fileno(fp);
    dup2(fd, 1);
    dup2(fd, 2);
    fclose(fp);
    if (!getvoxbolock()) {
      fprintf(stderr, "Couldn't lock log file.  VoxBo already running?\n");
      return 0;
    }
    return 1;
  } else
    return 0;
}

int haltsequence(VBSequence &seq) {
  // fprintf(stderr,"[HALT] halting sequence %d\n",seq.seqnum);
  for (SMI j = seq.specmap.begin(); j != seq.specmap.end(); j++) {
    if (j->second.status != 'R') continue;
    VBHost *hh = findhost(j->second.hostname);
    if (!hh) continue;
    // fprintf(stderr,"[HALT] halting job %d on host
    // %s\n",j->second.jnum,hh->nickname.c_str());
    hh->SendMsg((string) "KILLJOB " + strnum(j->second.pid) + " " +
                strnum(j->second.childpid));
  }
  // fprintf(stderr,"[HALT] done halting sequence %d\n",seq.seqnum);
  return 0;
}

int handle_server_sendfile(int ns, string) {
  // receive filename and size
  char buf[STRINGLEN];
  int len = safe_recv(ns, buf, STRINGLEN - 1, 10.0);
  if (len < 1) return 1;
  tokenlist args;
  args.ParseLine(buf);
  if (args.size() != 3) return 2;
  fprintf(stderr, "[SENDFILE] name %s size %s\n", args(1), args(2));
  int err = receive_file(ns, "/tmp/foo.dat", strtol(args[2]));
  fprintf(stderr, "receive_file returned %d\n", err);
  return 0;
}

string handle_server_changejobstatus(tokenlist &args, string) {
  if (args.size() != 4) return "[E] malformed job status request";
  int seqnum = strtol(args[1]);
  string from = args[2];
  string to = args[3];
  int ret = 0;
  if (seqlist.count(seqnum) == 0) {
    return (string) "[E] sequence not found";
  }
  VBSequence *seq = &(seqlist[seqnum]);
  for (SMI j = seq->specmap.begin(); j != seq->specmap.end(); j++) {
    if (j->second.status == from[0] ||
        (from == "*" && j->second.status != to[0]))
      ret +=
          process_setjobinfo(seqnum, j->second.jnum, (string) "status " + to);
  }

  if (ret == 0)
    return (string) "[I] sequence successfully updated";
  else
    return (string) "[E] sequence not fully updated";
}

string handle_server_setseqinfo(tokenlist &args, string username) {
  if (args.size() != 4) return "[E] malformed setseqinfo request";
  int seqnum = strtol(args[1]);
  string tag = args[2];
  string value = args[3];
  if (seqlist.count(seqnum) != 1) return "missing";
  if (!((seqlist[seqnum].owner == username) ||
        (vbp.superusers.count(username))))
    return "perm";
  int ret = process_setseqinfo(seqnum, tag + " " + value);
  if (ret == 0)
    return "success";
  else
    return "error";
}

string handle_server_setsched(tokenlist &args, string username) {
  // args should be one of the following:
  //     setsched sched seqnums pri max pri2 max2 maxperhost
  //         0      1      2     3   4   5    6      7
  //             m/max  seqnums max
  //             012345 seqnums pri
  //             sched  seqnums default
  //             sched  seqnums offhours
  //             sched  seqnums nice
  //             sched  seqnums xnice
  VBpri pp;
  set<int> seqnums = numberset(args[2]);
  int validnums = 0, invalidnums = 0;
  vector<string> newseqinfo;

  vbforeach(int s, seqnums) if (seqlist.count(s)) validnums++;
  else invalidnums--;
  if (validnums == 0)
    return "[E] no valid sequence numbers specified for scheduling change";

  if (args[1] == "sched") {
    // build just the args
    tokenlist tmpl = args;
    args.DeleteFirst();
    args.DeleteFirst();
    args.DeleteFirst();
    if (pp.set(args)) return "[E] invalid scheduling parameters";
    newseqinfo.push_back("priority " + strnum(pp.priority));
    newseqinfo.push_back("maxjobs " + strnum(pp.maxjobs));
    newseqinfo.push_back("priority2 " + strnum(pp.priority2));
    newseqinfo.push_back("maxjobs2 " + strnum(pp.maxjobs2));
    newseqinfo.push_back("maxperhost " + strnum(pp.maxperhost));
  } else if ((args[1] == "m" || args[1] == "max") && args.size() == 4) {
    newseqinfo.push_back("maxjobs " + strnum(strtol(args[3])));
  }

  string status;
  int succeededcnt = 0, failedcnt = 0;
  vbforeach(int s, seqnums) {
    if (seqlist.count(s) == 0) continue;
    if (seqlist[s].owner != username && vbp.superusers.count(username) == 0) {
      status += "[E] you are not the owner of sequence " + strnum(s) + "\n";
      continue;
    }
    bool bad = 0;
    vbforeach(string ss, newseqinfo) {
      int ret = process_setseqinfo(s, ss);
      if (ret) bad = 1;
    }
    if (bad)
      failedcnt++;
    else
      succeededcnt++;
  }
  if (succeededcnt) {
    status += "[I] " + strnum(succeededcnt) + " sequences successfully updated";
    if (failedcnt) status += "\n";
  }
  if (failedcnt)
    status += "[E] " + strnum(succeededcnt) + " sequences could not be updated";
  return status;
}

string handle_server_killsequence(tokenlist &args, string username) {
  if (args.size() != 3) return "[E] malformed kill request";
  int seqnum = strtol(args[1]);
  string value = args[2];
  if (seqlist.count(seqnum) != 1) return "missing";
  if (!((seqlist[seqnum].owner == username) ||
        (vbp.superusers.count(username))))
    return "perm";
  int ret = process_killsequence(seqnum, value);
  if (ret == 0)
    return "success";
  else
    return "error";
}

void handle_server_submit(int ns, string fname) {
  FILE *numfile;
  string tmp = vbp.queuedir + "/vb.num";
  numfile = fopen(tmp.c_str(), "r+");
  if (!numfile) {
    tmp = "[E] couldn't open sequence counter file in " + vbp.queuedir;
    send(ns, tmp.c_str(), tmp.size(), 0);
    return;
  }

  char numdata[STRINGLEN];
  if (fgets(numdata, STRINGLEN, numfile) == NULL) {
    tmp = "[E] couldn't get data from counter file in " + vbp.queuedir;
    send(ns, tmp.c_str(), tmp.size(), 0);
    return;
  }
  int seqnum = strtol(numdata);
  rewind(numfile);
  fprintf(numfile, "%d\n", seqnum + 1);
  fclose(numfile);

  char sname[STRINGLEN];
  sprintf(sname, "%s/%08d", vbp.queuedir.c_str(), seqnum);

  VBSequence seq;
  seq.LoadSequence(fname);
  if (!seq.valid) {
    tmp = "[E] invalid sequence dir " + fname;
    send(ns, tmp.c_str(), tmp.size(), 0);
    return;
  }
  seq.seqnum = seqnum;
  for (SMI j = seq.specmap.begin(); j != seq.specmap.end(); j++)
    j->second.snum = seqnum;
  seq.queuedtime = time(NULL);
  int err = seq.Write(sname);
  if (err) {
    tmp = "[E] error " + strnum(err) + " writing sequence";
    send(ns, tmp.c_str(), tmp.size(), 0);
    return;
  } else
    rmdir_force(fname);
  tmp = "[I] sequence " + strnum(seqnum) + " has been submitted";
  send(ns, tmp.c_str(), tmp.size(), 0);
  // now just to be safe (and to make sure we know what dir we're in...)
  seq.LoadSequence(sname);
  seqlist[seq.seqnum] = seq;
  return;
}

int server_add(string sname) {
  // if it's on the list already, bring it back
  for (HI h = hostlist.begin(); h != hostlist.end(); h++) {
    if (h->nickname == sname || h->hostname == sname) {
      h->status = "up";
      h->lastresponse = 0;
      printf("[I] %s marked host %s (%s) up\n", timedate().c_str(),
             h->nickname.c_str(), h->hostname.c_str());
      ping_hosts();
      return 0;
    }
  }

  // not on list, let's add it, and update its taken_cpus
  VBHost hh(sname, sname, vbp.serverport);
  // find nickname or hostname from vbp.servers
  pair<string, string> ss;
  vbforeach(ss, vbp.servers) {
    if (ss.first == sname) hh.hostname = ss.second;
    if (ss.second == sname) hh.nickname == ss.first;
  }
  // count running jobs (it might have some after a restart)
  for (map<jobid, VBJobSpec>::iterator vv = runningmap.begin();
       vv != runningmap.end(); vv++) {
    if (vv->second.hostname == hh.nickname)
      hh.taken_cpus++;  // FIXME always just one cpu
  }
  hostlist.push_back(hh);
  printf("[I] %s added host %s (%s)\n", timedate().c_str(), hh.nickname.c_str(),
         hh.hostname.c_str());
  return 0;
}

int server_delete(string nickname) {
  for (HI h = hostlist.begin(); h != hostlist.end(); h++) {
    if (h->nickname == nickname) {
      h->status = "down";
      printf("[I] %s marked host %s (%s) down\n", timedate().c_str(),
             nickname.c_str(), h->hostname.c_str());
      return 0;
    }
  }
  return 101;
}

void read_serverlist() {
  pair<string, string> ss;
  vbforeach(ss, vbp.servers) server_add(ss.first);
}

void process_vbx() {
  tokenlist line;
  vglob vg("*.vbx");
  if (!vg.size()) return;
  for (size_t i = 0; i < vg.size(); i++) {
    line.ParseFirstLine(vg[i]);
    if (line[0] == "setjobinfo")
      process_setjobinfo(strtol(line[1]), strtol(line[2]), line.Tail(3));
    else if (line[0] == "jobrunning")
      process_jobrunning(line[1], strtol(line[2]), strtol(line[3]),
                         strtol(line[4]), strtol(line[5]), strtol(line[6]));
    else if (line[0] == "jobdone")
      process_jobdone(strtol(line[1]), strtol(line[2]), strtol(line[3]));
    else if (line[0] == "setseqinfo")
      process_setseqinfo(strtol(line[1]), line.Tail(2));
    else if (line[0] == "killsequence")
      process_killsequence(strtol(line[1]), line[2]);
    else if (line[0] == "email")
      process_email(line[1], vg[i]);
    else if (line[0] == "adminemail")
      process_adminemail(vg[i]);
    else if (line[0] == "hostupdate")
      process_hostupdate(line.Tail());
    else if (line[0] == "retry")
      process_retry(line.Tail());
    else if (line[0] == "saveline")
      process_saveline(line.Tail());
    else {
      printf("[E] %s invalid vbx file %s\n", timedate().c_str(), vg[i].c_str());
      // FIXME put it somewhere?
    }
    unlink(vg[i].c_str());
    // rename(gb.gl_pathv[i],((string)"tmp/"+gb.gl_pathv[i]).c_str());
  }
}

// FIXME in process_dropbox, we call loadsequence twice to make sure
// that the seq structure and all the jobs have the right info.  job
// fields affected include basename, email, seqname, logfile, uid,
// etc.  there is certainly a better way to do this.

void process_dropbox() {
  tokenlist numx;
  string numfile = vbp.queuedir + "/vb.num";
  string droppat = vbp.rootdir + "/drop/submit*";
  numx.ParseFirstLine(numfile);
  int seqnum = strtol(numx[0]);
  // just in case...
  while (seqlist.count(seqnum)) seqnum++;

  vglob vg(droppat);
  for (size_t i = 0; i < vg.size(); i++) {
    VBSequence seq;
    seq.LoadSequence(vg[i]);
    if (!seq.valid) continue;
    string sname = (format("%s/%08d") % vbp.queuedir % seqnum).str();
    seq.seqnum = seqnum;
    for (SMI j = seq.specmap.begin(); j != seq.specmap.end(); j++)
      j->second.snum = seqnum;
    seq.queuedtime = time(NULL);
    int err = seq.Write(sname);
    if (err) {
      fprintf(stderr, "[E] error %d writing sequence %s\n", err, sname.c_str());
      return;
    } else
      rmdir_force(vg[i].c_str());
    if (f_debug) printf("[I] sequence %d received\n", seqnum);
    seq.LoadSequence(sname);
    seqlist[seq.seqnum] = seq;
    seqnum++;
  }

  FILE *nfile = fopen(numfile.c_str(), "w");
  if (!nfile) {
    printf("[E] couldn't open sequence counter file in %s\n",
           vbp.queuedir.c_str());
    return;
  }
  fprintf(nfile, "%d\n", seqnum);
  fclose(nfile);
  return;
}

int process_jobrunning(string hostname, int snum, int jnum, pid_t pid,
                       pid_t childpid, long stime) {
  if (runningmap.count(jobid(snum, jnum)) == 0) {
    printf("[E] %s job %08d-%08d running but not in runningmap\n",
           timedate().c_str(), snum, jnum);
    return 101;
  }
  VBJobSpec *js = &(runningmap[jobid(snum, jnum)]);
  if (!js) {
    printf("[E] %s received jobrunning message for non-running job %08d-%05d\n",
           timedate().c_str(), snum, jnum);
    return -1;
  }

  // update jobspec
  js->serverstartedtime = stime;
  js->pid = pid;
  js->childpid = childpid;
  if (js->status != 'R') {
    js->status = 'R';
  }
  // now make sure it's set everywhere (esp. the job files)
  process_setjobinfo(snum, jnum, "status R");
  process_setjobinfo(snum, jnum, (string) "host " + hostname);
  process_setjobinfo(snum, jnum, (string) "pid " + strnum(pid));
  process_setjobinfo(snum, jnum, (string) "childpid " + strnum(childpid));
  process_setjobinfo(snum, jnum, (string) "serverstartedtime " + strnum(stime));
  return 0;
}

int process_jobdone(int snum, int jnum, long ftime) {
  process_setjobinfo(snum, jnum,
                     (string) "serverfinishedtime " + strnum(ftime));
  removerunningjob(snum, jnum);
  return 1;
}

int process_setjobinfo(int ss, int jj, string newstatusinfo) {
  // update the file
  char jobfile[STRINGLEN];
  int err;
  // first append the line to the actual job file
  sprintf(jobfile, "%08d/%05d.job", ss, jj);
  if ((err = appendline(jobfile, newstatusinfo))) {
    printf("[E] %s couldn't update job file %s (%d,%d,%s) [%d]\n",
           timedate().c_str(), jobfile, ss, jj, newstatusinfo.c_str(), err);
    return 101;
  }
  // seqlist[ss] is our sequence, seqlist[ss].specmap[jj] is our jobspec
  if (seqlist.count(ss) != 1) {
    printf("[E] %s seq not in seqlist\n", timedate().c_str());
    return 102;
  }
  if (seqlist[ss].specmap.count(jj) != 1) {
    printf("[E] %s jobspec not in seq's speclist\n", timedate().c_str());
    return 103;
  }
  seqlist[ss].specmap[jj].ParseJSLine(newstatusinfo);
  seqlist[ss].modtime = time(NULL);
  // ...and in the runningmap
  if (runningmap.count(jobid(ss, jj)))
    runningmap[jobid(ss, jj)].ParseJSLine(newstatusinfo);
  return 0;
}

int process_setseqinfo(int ss, string newstatusinfo) {
  // update the file
  string seqfile = (format("%08d/info.seq") % ss).str();
  if (appendline(seqfile, newstatusinfo)) {
    printf("[E] %s couldn't update sequence file\n", timedate().c_str());
    return 101;
  }
  // update it in the seqlist
  if (seqlist.count(ss)) {
    seqlist[ss].ParseSeqLine(newstatusinfo);
    seqlist[ss].modtime = time(NULL);
  } else
    printf("[E] %s setseqinfo for non-existent sequence %d\n",
           timedate().c_str(), ss);
  return 0;
}

int process_killsequence(int ss, string newstatus) {
  printf("[I] %s killing sequence %d with %s\n", timedate().c_str(), ss,
         newstatus.c_str());
  // update the file
  string seqfile = (format("%08d/info.seq") % ss).str();
  if (appendline(seqfile, "status " + newstatus)) {
    printf("[E] %s couldn't update sequence file\n", timedate().c_str());
    return 101;
  }
  // update it in the seqlist
  if (seqlist.count(ss)) {
    seqlist[ss].ParseSeqLine((string) "status " + newstatus);
    haltsequence(seqlist[ss]);
  } else
    printf("[E] %s setseqinfo for non-existent sequence %d\n",
           timedate().c_str(), ss);
  return 0;
}

int process_hostupdate(string hh) {
  string hostinfo, field;

  HI tmph = hostlist.end();
  tokenlist args, argx;
  args.SetQuoteChars("[<(\"'");
  argx.SetQuoteChars("[<(\"'");
  args.ParseLine(hh);
  // first argument better be full hostname
  argx.ParseLine(args[0]);
  if (argx[0] != "hostname") {
    printf("[E] %s malformed host update\n", timedate().c_str());
    return 101;
  }
  string hostname = argx[1];
  for (HI h = hostlist.begin(); h != hostlist.end(); h++) {
    if (h->hostname == hostname) {
      tmph = h;
    }
  }
  if (tmph == hostlist.end()) {
    printf("[E] %s invalid host update from %s\n", timedate().c_str(),
           hostname.c_str());
    return 101;
  }

  tmph->resources.clear();
  string newstatus;
  for (size_t i = 1; i < args.size(); i++) {
    argx.ParseLine(args[i]);
    field = argx[0];
    // fields we don't update that might be in this bundle include
    // taken_cpus, avail_cpus, and running jobs
    if (field == "hostname")
      tmph->hostname = argx[1];
    else if (field == "nickname")
      tmph->nickname = argx[1];
    else if (field == "currentpri")
      tmph->currentpri = strtol(argx[1]);
    else if (field == "job") {
      if (argx.size() < 3) continue;
      int snum = strtol(argx[1]);
      int jnum = strtol(argx[2]);
      // make sure it's in the running list and the seqlist
      if (runningmap.count(jobid(snum, jnum)) < 1) continue;
      if (seqlist.count(snum) < 1) continue;
      if (seqlist[snum].specmap.count(jnum) < 1) continue;
      // update lastreport time in both runningmap and seqlist
      // printf("XXXXX updating %d %d\n",snum,jnum);
      runningmap[jobid(snum, jnum)].lastreport = time(NULL);
      seqlist[snum].specmap[jnum].lastreport = time(NULL);
    } else if (field == "load")
      tmph->loadaverage = strtod(argx[1]);
    else if (field == "resource") {
      VBResource rr;
      rr.name = argx[1];
      rr.f_global = strtol(argx[2]);
      rr.cnt = strtol(argx[3]);
      tmph->resources[rr.name] = rr;
    } else if (field == "total_cpus")
      tmph->total_cpus = strtol(argx[1]);
    else if (field == "status")
      newstatus = argx[1];
  }
  tmph->Update();
  tmph->lastresponse = time(NULL);
  if ((tmph->status == "dead" || tmph->status == "unknown") &&
      newstatus == "up")
    tmph->status = "up";
  else if (newstatus == "dead")
    tmph->status = "dead";
  f_totalcpus = 0;
  vbforeach(const VBHost &hh, hostlist) f_totalcpus += hh.avail_cpus;
  return 0;
}

int process_retry(string line) {
  tokenlist args;
  args.ParseLine(line);
  if (args.size() != 3) {
    printf("[E] %s invalid retry line\n", timedate().c_str());
    return 101;
  }
  int snum = strtol(args[0]);
  int jnum = strtol(args[1]);
  int generations = strtol(args[2]);
  return do_retry(snum, jnum, generations);
}

int do_retry(int snum, int jnum, int generations) {
  // retry this job
  if (seqlist.count(snum) != 1) return 101;
  if (seqlist[snum].specmap.count(jnum) != 1) return 102;
  process_setjobinfo(snum, jnum, "status W");
  if (generations) {
    vbforeach(int wf, seqlist[snum].specmap[jnum].waitfor)
        do_retry(snum, wf, generations - 1);
  }
  return 0;
}

int process_saveline(string line) {
  printf("[!] %s asked to save line: %s\n", timedate().c_str(), line.c_str());
  return 0;
}

void populate_running_jobs(map<int, VBSequence> &seqlist) {
  for (SI s = seqlist.begin(); s != seqlist.end(); s++) {
    for (SMI j = s->second.specmap.begin(); j != s->second.specmap.end(); j++) {
      if (j->second.status == 'R') addrunningjob(j->second, j->second.hostname);
    }
  }
}

int process_adminemail(string fname) {
  if (vbp.sendmail.size() == 0) return 200;
  FILE *mailpipe;
  string email;
  string msg;
  ifstream fs;
  char buf[STRINGLEN], cmd[STRINGLEN];

  printf("[I] %s emailing %s\n", timedate().c_str(), vbp.sysadmin.c_str());
  fs.open(fname.c_str(), ios::in);
  if (!fs) return 101;
  fs.getline(buf, STRINGLEN, '\n');  // throw away first line
  snprintf(cmd, STRINGLEN, "%s -f %s -t", vbp.sendmail.c_str(),
           vbp.sysadmin.c_str());
  mailpipe = popen(cmd, "w");
  if (!mailpipe) {
    fprintf(stderr,
            "[E] vbsrvd: serious error, couldn't email sysadmin about it\n");
    fs.close();
    return 101;
  }
  fprintf(mailpipe, "To: %s\n", vbp.sysadmin.c_str());
  fprintf(mailpipe, "Reply-To: %s\n", vbp.sysadmin.c_str());
  while (1) {
    fs.read(buf, STRINGLEN - 1);
    buf[fs.gcount()] = '\0';
    if (fs.gcount()) fprintf(mailpipe, "%s", msg.c_str());
    if (fs.gcount() < STRINGLEN - 1) break;
  }
  pclose(mailpipe);
  fs.close();
  return 0;
}

int process_email(string recipient, string fname) {
  if (vbp.sendmail.size() == 0) return 200;
  // punt on specific bogus addresses
  if (recipient == "nobody@nowhere.com" || recipient.size() == 0) return 0;
  // punt on local addresses
  if (recipient.find("@") == string::npos) return 0;
  FILE *mailpipe;
  string email, msg, cmd;
  ifstream fs;
  char buf[STRINGLEN];

  printf("[I] %s emailing %s\n", timedate().c_str(), recipient.c_str());
  fs.open(fname.c_str(), ios::in);
  if (!fs) return 101;
  fs.getline(buf, STRINGLEN, '\n');  // throw away first line
  cmd = (format("%s -f %s -t") % (vbp.sendmail) % recipient).str();
  mailpipe = popen(cmd.c_str(), "w");
  if (!mailpipe) {
    fprintf(stderr,
            "[E] vbsrvd: serious error, couldn't email sysadmin about it\n");
    fs.close();
    return 101;
  }
  while (1) {
    fs.read(buf, STRINGLEN - 1);
    buf[fs.gcount()] = '\0';
    if (fs.gcount()) fprintf(mailpipe, "%s", buf);
    if (fs.gcount() < STRINGLEN - 1) break;
  }
  pclose(mailpipe);
  fs.close();
  return 0;
}

// new function to decide which jobs to run

void run_jobs() {
  // build set of eligible sequences
  set<int> slist;
  typedef pair<const int, VBSequence> spair;
  vbforeach(spair & ss, seqlist) {
    VBSequence *seq = &(ss.second);
    if (seq->status != 'R') continue;
    if (seq->priority.maxjobs && seq->priority.maxjobs2 &&
        seq->runcnt >= (seq->priority.maxjobs + seq->priority.maxjobs2))
      continue;
    seq->effectivepriority = seq->priority.priority;
    if (seq->priority.maxjobs && seq->runcnt >= seq->priority.maxjobs)
      seq->effectivepriority = seq->priority.priority2;
    slist.insert(seq->seqnum);
  }

  // FIXME this block of code creates a list that is never used
  // make a copy of the hostlist with all "up" hosts
  // list<VBHost> myhosts;
  // map<string,VBResource *> globals;
  // vbforeach (VBHost &hh,hostlist) {
  //   if (hh.status!="up") continue;
  //   myhosts.push_back(hh);
  // }

  // get the list of avail resources
  map<string, int> rlist = availableresources();
  while (1) {
    // if we're out of eligible sequences, escape
    if (slist.size() < 1) break;
    // FIXME no available CPUs?  break
    // find the sequence with the highest priority
    int maxpri = 0;
    int maxseq = 0;
    VBSequence *seq;
    vbforeach(int snum, slist) {
      seq = &(seqlist[snum]);
      if (seq->effectivepriority > maxpri) {
        maxseq = seq->seqnum;
        maxpri = seq->effectivepriority;
      }
    }
    // no jobs have any priority
    if (maxpri == 0) break;
    // we have our sequence
    seq = &(seqlist[maxseq]);

    // build a map of how may of this sequence's jobs are running on each host
    map<string, int> hostcnt;
    for (map<jobid, VBJobSpec>::iterator jj = runningmap.begin();
         jj != runningmap.end(); jj++) {
      if (jj->first.snum != seq->seqnum) continue;
      if (hostcnt.count(jj->second.hostname))
        hostcnt[jj->second.hostname]++;
      else
        hostcnt[jj->second.hostname] = 1;
    }

    // iterate all jobs, finding ones that we can run
    int unmet;
    for (SMI j = seq->specmap.begin(); j != seq->specmap.end(); j++) {
      if (j->second.status != 'W') continue;
      VBJobType *jt = &(vbp.jobtypemap[j->second.jobtype]);
      // check dependencies
      unmet = 0;
      vbforeach(int32 ww, j->second.waitfor) {
        if (seq->specmap[ww].status != 'D') {
          unmet = 1;
          break;
        }
      }
      if (unmet) continue;
      // find a host; first build combined requirement list
      map<string, int> reqs = jt->requires;
      for (map<string, int>::iterator rr = seq->requires.begin();
           rr != seq->requires.end(); rr++)
        if (rr->second > reqs[rr->first]) reqs[rr->first] = rr->second;
      HI myhost = hostlist.end();
      for (HI p = hostlist.begin(); p != hostlist.end(); p++) {
        if (p->status != "up") continue;
        p->Update();  // recalc available cpus
        if (!(p->valid)) continue;
        if (p->avail_cpus < 1) continue;  // any available cpus?
        if (seq->effectivepriority < p->currentpri) continue;
        if (seq->priority.maxperhost &&
            hostcnt[p->nickname] >= seq->priority.maxperhost)
          continue;
        if (j->second.forcedhosts.size() &&
            j->second.forcedhosts.count(p->nickname) == 0)
          continue;
        // do we have the needed resources?  first combine jt-based
        // and seq-based requires lists
        unmet = 0;
        for (map<string, int>::iterator rr = reqs.begin(); rr != reqs.end();
             rr++) {
          // available on this host?
          string rname = p->nickname + ":" + rr->first;
          if (rlist.count(rname))
            if (rlist[rname] >= rr->second) continue;
          // available globally?
          rname = "*:" + rr->first;
          if (rlist.count(rname))
            if (rlist[rname] >= rr->second) continue;
          unmet = 1;
          break;
        }
        // if we can't get the resources, try the next host
        if (unmet) continue;
        // get out of the host list loop to queue the job
        myhost = p;
        break;
      }
      // if we have no valid myhost for this job...
      if (myhost == hostlist.end()) continue;

      // let's take the resources we're using
      for (map<string, int>::iterator rr = reqs.begin(); rr != reqs.end();
           rr++) {
        if (rr->second == 0) continue;
        // first try locally
        string rname = myhost->nickname + ":" + rr->first;
        if (rlist.count(rname)) {
          if (rlist[rname] >= rr->second) {
            rlist[rname] -= rr->second;
            continue;
          }
        }
        // then globally
        rname = "*:" + rr->first;
        if (rlist.count(rname)) {
          if (rlist[rname] >= rr->second) {
            rlist[rname] -= rr->second;
            continue;
          }
        }
      }

      // since we're going to try to grab this machine, we can do the
      // following (note that avail_cpus will be updated when we call
      // addrunningjob)
      addrunningjob(j->second, myhost->nickname);

      if (f_debug)
        cout << format("[D] voxbo: sending to %s\n") % myhost->nickname;

      fork_send_voxbo_job(j->second, myhost);

      if (f_debug) cout << format("[D] voxbo: done sending\n");

      // if we have a maxperhost, update the host counts
      if (seq->priority.maxperhost) {
        if (hostcnt.count(myhost->nickname))
          hostcnt[myhost->nickname]++;
        else
          hostcnt[myhost->nickname] = 1;
        // cout << myhost->nickname << " : " << hostcnt[myhost->nickname] <<
        // endl;  // FIXME REMOVE
      }
      // cout << "MAX " << seq->priority.maxperhost << endl;

      // update runcnt and waitcnt, figure out new priority and max
      seq->runcnt++;
      seq->waitcnt--;
      if (seq->priority.maxjobs && (seq->runcnt >= seq->priority.maxjobs)) {
        // if we can't run jobs under pri2 then we're done with this seq
        if (seq->priority.priority2 < 1 ||
            (seq->priority.maxjobs2 && seq->runcnt >= seq->priority.maxjobs2)) {
          slist.erase(seq->seqnum);
          break;
        }
        // otherwise, make sure we're using pri2.  if that means a
        // change, break out to re-prioritize sequences
        if (seq->effectivepriority != seq->priority.priority2) {
          seq->effectivepriority = seq->priority.priority2;
          break;
        }
      }
    }
    // we checked all the jobs, remove this sequence and keep going
    slist.erase(seq->seqnum);
  }
}

// int
// send_email(string msg)
// {
//   if (vbp.sendmail.size()==0)
//     return 200;
//   FILE *mailpipe;
//   char str[STRINGLEN];
//   string email;

//   snprintf(str,STRINGLEN,"%s -f %s
//   -t",vbp.sendmail.c_str(),vbp.username.c_str()); mailpipe = popen(str,"w");
//   if (!mailpipe) {
//     fprintf(stderr,"[E] vbsrvd: serious error, couldn't email sysadmin about
//     it\n"); return 101;
//   }
//   fprintf(mailpipe,"%s\n",msg.c_str());
//   pclose(mailpipe);
//   return 0;
// }

void addrunningjob(VBJobSpec &js, string &hostname) {
  js.hostname = hostname;
  js.lastreport = time(NULL);
  runningmap[jobid(js.snum, js.jnum)] = js;
  VBHost *hh;
  if ((hh = findhost(hostname)) != NULL) {
    hh->taken_cpus++;  // FIXME update for jobs using more than once cpu
    hh->Update();
  } else
    cout << "couldn't find host " << hostname << endl;
}

void removerunningjob(int snum, int jnum) {
  removerunningjob(jobid(snum, jnum));
}

void removerunningjob(jobid jid) {
  if (!runningmap.count(jid)) {
    cerr << format("[E] voxbo: tried to remove non-running job %08d-%05d\n") %
                jid.snum % jid.jnum;
    return;
  }
  string hostname = runningmap[jid].hostname;
  runningmap.erase(jid);
  for (HI hh = hostlist.begin(); hh != hostlist.end(); hh++) {
    if (hh->nickname == hostname || hh->hostname == hostname) {
      hh->taken_cpus--;
      hh->Update();
      break;
    }
  }
}

// availableresources() returns a map of resource names to the amount
// available.  the resource names are prepended with the host, as in
// mymachine:myresource.  for global (not machine-locked) resources,
// it's *:myresource.

map<string, int> availableresources() {
  // FIXME go through hosts, make a list, go through jobs, mod the list
  map<string, int> rlist;
  for (HI hh = hostlist.begin(); hh != hostlist.end(); hh++) {
    for (map<string, VBResource>::iterator ri = hh->resources.begin();
         ri != hh->resources.end(); ri++) {
      string rname = hh->nickname + ":" + ri->second.name;
      if (ri->second.f_global) rname = "*:" + ri->second.name;
      if (rlist.count(rname)) {
        if (rlist[rname] < ri->second.cnt) rlist[rname] = ri->second.cnt;
      } else
        rlist[rname] = ri->second.cnt;
    }
  }
  // now we have a list of all the resources, let's subtract what's running
  for (map<jobid, VBJobSpec>::iterator vv = runningmap.begin();
       vv != runningmap.end(); vv++) {
    VBJobType jt = vbp.jobtypemap[vv->second.jobtype];
    for (map<string, int>::iterator rr = jt.requires.begin();
         rr != jt.requires.end(); rr++) {
      string rname = vv->second.hostname + ":" + rr->first;
      if (rlist.count(rname)) {
        rlist[rname] -= rr->second;
        continue;
      }
      rname = vv->second.hostname + ":" + rr->first;
      if (rlist.count(rname)) {
        rlist[rname] -= rr->second;
        continue;
      }
    }
  }

  return rlist;
}

// cleanupqueue() -- delete sequences as needed

void cleanupqueue(map<int, VBSequence> &seqlist, string qdir) {
  chdir(qdir.c_str());
  for (SI ss = seqlist.begin(); ss != seqlist.end(); ss++) {
    VBSequence *seq = &(ss->second);
    seq->updatecounts();
    if (seq->status == 'X' && seq->runcnt == 0) {
      // rmdir_force(seq->seqdir);
      rename(seq->seqdir.c_str(), (seq->seqdir + "_defunct").c_str());
    }
    // if all done, mark as private and remove seq waitfors
    else if (seq->jobcnt > 0 && seq->donecnt == seq->jobcnt) {
      remove_seqwait(vbp.queuedir, seq->seqnum);
      rename(seq->seqdir.c_str(), (seq->seqdir + "_defunct").c_str());
      seq->status = 'X';
    } else if (seq->status == 'K' && seq->runcnt == 0) {
      rename(seq->seqdir.c_str(), (seq->seqdir + "_defunct").c_str());
      seq->status = 'X';
    }
    // FIXME if it's too old, mark as private
    // else if ((time(NULL) - seq->queuedtime) > SECONDSINAWEEK)
    // setseqinfo(seq->seqnum,"status","X");
  }
  // remove sequences with status X from the seqlist
  set<int> removenums;
  for (SI ss = seqlist.begin(); ss != seqlist.end(); ss++)
    if (ss->second.status == 'X') removenums.insert(ss->second.seqnum);
  vbforeach(int n, removenums) seqlist.erase(n);
  // remove directories of jobs that should be dead
  vglob vg(qdir + "/*_defunct");
  for (size_t i = 0; i < vg.size(); i++) rmdir_force(vg[i]);
  // go through runninglist, get rid of jobs that shouldn't be in here
  // (seq just killed) and restart jobs that have gone missing
  pair<jobid, VBJobSpec> js;
  set<jobid> r_remove;
  set<jobid> r_restart;
  vbforeach(js, runningmap) {
    if (removenums.count(js.first.snum)) {
      // this sequence was just removed
      printf(
          "[W] %s job %05d-%08d was removed from runninglist after seq "
          "removed\n",
          timedate().c_str(), js.first.snum, js.first.jnum);
      r_remove.insert(js.first);
    }
    // FIXME hardcoded 3 min missing deadline
    else if ((time(NULL) - js.second.lastreport) > S_MISSING)
      r_restart.insert(js.first);
  }
  vbforeach(jobid ji, r_remove) removerunningjob(ji);
  vbforeach(jobid ji, r_restart) {
    printf("[E] %s job %05d-%08d was missing and set to run again\n",
           timedate().c_str(), js.first.snum, js.first.jnum);
    process_setjobinfo(ji.snum, ji.jnum, (string) "status W");
    removerunningjob(ji);
    // FIXME should flag the machine as losing jobs and eventually shut it down
  }
}

void vbscheduler_help() {
  printf("\nVoxBo scheduler (v%s)\n", vbversion.c_str());
  printf("usage:\n");
  printf("  voxbo <flags>\n");
  printf("flags:\n");
  printf("  -l <file>      log file\n");
  printf("  -d             detach\n");
  printf("  -x             debug\n");
  printf("  -q <dir>       specify queue directory\n");
  // printf("  -s             single-user mode\n");
  printf("  -h             help\n");
  printf("  -v             version\n");
  printf("notes:\n");
  printf("   ");
  printf("\n");
}

void vbscheduler_version() {
  printf("VoxBo scheduler (v%s)\n", vbversion.c_str());
}
