
// runseq.cpp
// Copyright (c) 2010 by The VoxBo Development Team

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

#include "runseq.h"
#include <signal.h>
#include <sys/wait.h>
#include <QMessageBox>
#include <QVBoxLayout>
#include "myboxes.h"
#include "vbx.h"

QRunSeq::QRunSeq(QWidget *parent) : QDialog(parent) {
  f_quit = 0;
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  this->setLayout(layout);

  mytext = new QTextEdit;
  layout->addWidget(mytext);

  pbar = new QProgressBar;
  pbar->setFormat("completed %v of %m jobs");
  layout->addWidget(pbar);

  QHBox *hb = new QHBox;
  layout->addWidget(hb);

  b_quit = new QPushButton("Quit");
  hb->addWidget(b_quit);
  connect(b_quit, SIGNAL(clicked()), this, SLOT(handleQuit()));

  b_pause = new QPushButton("Pause");
  hb->addWidget(b_pause);
  connect(b_quit, SIGNAL(clicked()), this, SLOT(handleQuit()));

  setWindowTitle("Progress Monitor");
}

int QRunSeq::Go(VBPrefs &vbpx, VBSequence &seqx, uint32 njobsx) {
  seqx.seqnum = 1;
  f_quit = 0;
  njobs = njobsx;
  vbp = *(&vbpx);
  seq = *(&seqx);
  // populate the dialog, set up the timer, exec
  mytimer = new QTimer(this);
  connect(mytimer, SIGNAL(timeout()), this, SLOT(handleTimer()));
  mytimer->start(100);

  // give each job a sensible log file and username, copy over jobtype
  for (SMI js = seq.specmap.begin(); js != seq.specmap.end(); js++) {
    js->second.owner = vbp.username;
    js->second.f_cluster = 0;  // no contacting scheduler
    if (vbp.jobtypemap.count(js->second.jobtype))
      js->second.jt = vbp.jobtypemap[js->second.jobtype];
    else {
      mytext->append(
          (format(
               "[E] your sequence has at least one unrecognized jobtype (%s)") %
           js->second.jobtype)
              .str()
              .c_str());
      return 101;
    }
  }
  pbar->setRange(0, seq.specmap.size());
  pbar->setValue(0);
  donecnt = 1;
  return 0;
}

void QRunSeq::handleQuit() {
  f_quit = 1;
  pair<pid_t, VBJobSpec> pp;
  vbforeach(pp, pmap) {
    pid_t p = pp.first;
    killpg(p, SIGUSR1);
    usleep(100000);
    kill(p, SIGUSR1);
    if (kill(p, 0) == -1 && errno == ESRCH)  // process is gone!
      continue;
    killpg(p, SIGHUP);
    killpg(p, SIGTERM);
    killpg(p, SIGKILL);
  }
}

void QRunSeq::handlePause() { f_quit = 1; }

void QRunSeq::handleTimer() {
  int mystatus;
  pid_t mypid;
  if (f_quit) {
    mytimer->stop();
    // call waitpid() until it returns <=0, to help avoid zombies?
    while (1) {
      pid_t ret = waitpid(-1, &mystatus, WNOHANG);
      if (ret <= 0) break;
    }
    accept();
    return;
  }
  if (!f_quit && pmap.size() < njobs) {
    // start up as many more jobs as we can, using fork_command
    set<int32> newjobs = readyjobs(seq, njobs - pmap.size());
    vbforeach(int32 j, newjobs) {
      mytext->append((format("[I] starting job %d") % j).str().c_str());
      VBJobSpec js = seq.specmap[j];
      mypid = fork();
      if (mypid < 0) {  // bad, shouldn't happen
        exit(99);
      }
      if (mypid == 0) {  // child
        run_voxbo_job(vbp, js);
        _exit(js.error);
      }
      pmap[mypid] = js;
      seq.specmap[j].status = 'R';
    }
  }

  if (pmap.empty()) {
    f_quit = 1;
    return;
  }
  while (1) {
    mypid = waitpid(-1, &mystatus,
                    WNOHANG);  // for anything that might have finished
    if (mypid <= 0) break;     // nothing running<0, nothing returned=0
    if (!pmap.count(mypid)) {
      // FIXME shouldn't happen, but for some reason it does when we "Edit"
      // QMessageBox::information(this,"WTF","pid not found!");
      return;
    }
    int32 jnum = pmap[mypid].jnum;
    pmap.erase(mypid);

    if (WIFEXITED(mystatus) && WEXITSTATUS(mystatus) == 0) {
      seq.specmap[jnum].status = 'D';
      // unlink(seq.specmap[jnum].logfilename().c_str());
      donecnt++;
      pbar->setValue(donecnt);
    } else {
      seq.specmap[jnum].status = 'B';
      if (f_quit == 0) {
        while (1) {  // loop in case we need to revisit the job
          QDisp dd;
          string errmsg = (format("job %d crashed with %d\n\n") % jnum %
                           WEXITSTATUS(mystatus))
                              .str();
          dd.mylabel->setText(errmsg.c_str());
          dd.exec();
          if (dd.disp == "edit") {
            if (fork() == 0) {
              string editor;
              editor = getenv("VISUAL");
              if (!editor.size()) editor = getenv("EDITOR");
              if (!editor.size()) editor = "emacs";
              system_nocheck((str(format("%s %s") % editor %
                                  seq.specmap[jnum].logfilename())
                                  .c_str()));
              _exit(0);
            }
          } else if (dd.disp == "skip") {
            seq.specmap[jnum].status = 'D';
            // unlink(seq.specmap[jnum].logfilename().c_str());
            donecnt++;
            pbar->setValue(donecnt);
            break;
          } else if (dd.disp == "retry") {
            seq.specmap[jnum].status = 'W';
            break;
          } else {  // should just be quit
            f_quit = 1;
            handleQuit();
            break;
          }
        }
      }
      // unlink(seq.specmap[jnum].logfilename().c_str());
    }
  }
}

QDisp::QDisp(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  this->setLayout(layout);

  QPushButton *button;

  mylabel = new QLineEdit;
  mylabel->setText(
      "One of your jobs has gone bad.  How would you like to proceed?");
  layout->addWidget(mylabel);

  QHBox *hb = new QHBox;
  layout->addWidget(hb);

  button = new QPushButton("Stop");
  hb->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handleStop()));

  button = new QPushButton("Skip");
  hb->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handleSkip()));

  button = new QPushButton("Edit Log File");
  hb->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handleEdit()));

  button = new QPushButton("Retry");
  hb->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handleRetry()));
}
