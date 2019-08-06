
// vbpermgen.cpp
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
// original version written by Tom King and Dan Kimberg

#include <q3frame.h>
#include <qspinbox.h>
#include <Q3BoxLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <fstream>
#include <iostream>
#include "runseq.h"
#include "vbcontrast.h"
#include "vbpermgen.hlp.h"
#include "vbpermgen.moc.h"
#include "vbprefs.h"
#include "vbx.h"

using namespace std;

void vbpermgen_help();
void vbpermgen_version();
int commandlineExecute(int argc, char **argv);

VBPrefs vbp;
VBSequence seq;
VBJobSpec js;
string tempPRM;
int msgboxsetpermnumber;
gsl_rng *theRNG;

int main(int argc, char **argv) {
  vbp.init();
  vbp.read_jobtypes();
  if (argc > 2) {
    int err = 0;
    err = commandlineExecute(argc, argv);
    if (err) switch (err) {
        case 1:
          printErrorMsg(VB_ERROR, "vbpermgen: bad argument structure.\n");
          return -1;
        case 2:
          printErrorMsg(VB_ERROR,
                        "vbpermgen: no matrix stem name indicated.\n");
          return -1;
        case 3:
          printErrorMsg(VB_ERROR,
                        "vbpermgen: no permutation directory indicated.\n");
          return -1;
        case 4:
          printErrorMsg(VB_ERROR, "vbpermgen: no method indicated.\n");
          return -1;
        case 5:
          printErrorMsg(VB_ERROR,
                        "vbpermgen: number of permutations not indicated.\n");
          return -1;
        case 6:
          printErrorMsg(VB_ERROR, "vbpermgen: scale not indicated.\n");
          return -1;
        case 7:
          printErrorMsg(VB_ERROR, "vbpermgen: no contrasts inciated.\n");
          return -1;
        case 8:
          printErrorMsg(VB_ERROR,
                        "vbpermgen: number of permuations desired exceeds "
                        "number possible.\n");
          return -1;
      }
    return 0;
  }
  QApplication a(argc, argv);
  permGenerator pg;
  if (argc > 0) {
    if (strcmp(argv[argc - 1], "-h") == 0) {
      vbpermgen_help();
      exit(0);
    }
  }
  QFont font("SansSerif", 10, 0);
  font.setStyleHint(QFont::SansSerif);
  a.setFont(font);
  pg.setFont(font);
  a.setMainWidget(&pg);
  pg.show();
  a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  return a.exec();
}

permGenerator::permGenerator(QWidget *parent, const char *name, bool modal,
                             Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)

{
  if (!name) setName("permGenerator");

  Q3BoxLayout *x = new Q3VBoxLayout(this);

  txtPRM = new QLineEdit(this, "txtPRM");
  txtPRM->setGeometry(QRect(250, 10, 170, 24));
  connect(txtPRM, SIGNAL(textChanged(const QString &)), this,
          SLOT(CalculateNumPerms()));

  pbDir = new QPushButton(this, "pbDir");
  pbDir->setGeometry(QRect(50, 10, 180, 20));
  connect(pbDir, SIGNAL(clicked()), this, SLOT(getPRMFile()));

  Q3BoxLayout *y = new Q3HBoxLayout(x);
  y->insertWidget(0, pbDir, 10, Qt::AlignRight);
  y->addSpacing(30);
  y->insertWidget(2, txtPRM, 10, Qt::AlignTop);
  y->addStretch(1);
  y->setMargin(5);

  txtPermDir = new QLineEdit(this, "txtPermDir");
  txtPermDir->setGeometry(QRect(250, 40, 170, 24));

  lblPermDir = new QLabel(this, "lblPermNumber");
  lblPermDir->setGeometry(QRect(90, 130, 140, 20));

  Q3BoxLayout *y1 = new Q3HBoxLayout(x);
  y1->insertWidget(0, lblPermDir, 10, Qt::AlignRight);
  y1->addSpacing(30);
  y1->insertWidget(2, txtPermDir, 10, Qt::AlignTop);
  y1->addStretch(1);
  y1->setMargin(5);

  txtContrasts = new QLineEdit(this, "txtContrasts");
  txtContrasts->setGeometry(QRect(250, 70, 100, 24));

  btnContrasts = new QPushButton(this, "btnContrasts");
  btnContrasts->setGeometry(QRect(395, 70, 100, 24));
  btnContrasts->setText("Edit");
  QObject::connect(btnContrasts, SIGNAL(clicked()), this, SLOT(getContrast()));

  lblContrasts = new QLabel(this, "lblContrasts");
  lblContrasts->setGeometry(QRect(130, 70, 100, 20));

  Q3BoxLayout *y2 = new Q3HBoxLayout(x);
  y2->insertWidget(0, lblContrasts, 10, Qt::AlignRight);
  y2->addSpacing(30);
  y2->insertWidget(2, txtContrasts, 10, Qt::AlignTop);
  y2->addSpacing(5);
  y2->insertWidget(4, btnContrasts, 5, Qt::AlignTop);
  y2->addStretch(1);
  y2->setMargin(5);

  // cbScale = new QComboBox( this, "txtScale" );
  // cbScale->setGeometry( QRect( 250, 100, 170, 24 ) );

  // lblScale = new QLabel( this, "lblScale" );
  // lblScale->setGeometry( QRect( 130, 100, 100, 20 ) );

  // Q3BoxLayout *y5 = new Q3HBoxLayout(x);
  // y5->insertWidget(0, lblScale, 10, Qt::AlignRight);
  // y5->addSpacing(30);
  // y5->insertWidget(2, cbScale, 10, Qt::AlignTop);
  // y5->addStretch(1);
  // y5->setMargin(5);

  txtPermNumber = new QLineEdit(this, "txtPermNumber");
  txtPermNumber->setGeometry(QRect(250, 130, 170, 24));
  connect(txtPermNumber, SIGNAL(textChanged(const QString &)), this,
          SLOT(warnCalculateNumPerms()));

  lblPermNumber = new QLabel(this, "lblPermNumber");
  lblPermNumber->setGeometry(QRect(90, 130, 140, 20));

  Q3BoxLayout *y3 = new Q3HBoxLayout(x);
  y3->insertWidget(0, lblPermNumber, 10, Qt::AlignRight);
  y3->addSpacing(30);
  y3->insertWidget(2, txtPermNumber, 10, Qt::AlignTop);
  y3->addStretch(1);
  y3->setMargin(5);

  lblPseudoT = new QLabel(this, "lblPseudoT");
  lblPseudoT->setGeometry(QRect(10, 150, 143, 40));

  QCheckBox *on = new QCheckBox("on/off", this);
  on->setChecked(0);
  on->setGeometry(QRect(160, 160, 60, 21));
  QObject::connect(on, SIGNAL(toggled(bool)), this, SLOT(mod(bool)));

  txtZ = new QLineEdit(this, "txtZ");
  txtZ->setGeometry(QRect(390, 160, 30, 24));
  txtZ->setDisabled(true);

  lblZ = new QLabel(this, "lblZ");
  lblZ->setGeometry(QRect(370, 160, 16, 21));

  txtX = new QLineEdit(this, "txtX");
  txtX->setGeometry(QRect(270, 160, 30, 24));
  txtX->setDisabled(true);

  txtY = new QLineEdit(this, "txtY");
  txtY->setGeometry(QRect(330, 160, 30, 24));
  txtY->setDisabled(true);

  lblX = new QLabel(this, "lblX");
  lblX->setGeometry(QRect(250, 160, 16, 20));

  lblY = new QLabel(this, "lblY");
  lblY->setGeometry(QRect(310, 160, 16, 20));

  Q3BoxLayout *y4 = new Q3HBoxLayout(x);
  y4->addSpacing(10);
  y4->insertWidget(1, lblPseudoT, 10, Qt::AlignRight);
  y4->addSpacing(10);
  y4->insertWidget(3, on, 10, Qt::AlignRight);
  y4->addSpacing(10);
  y4->insertWidget(5, lblX, 10, Qt::AlignRight);
  y4->addSpacing(5);
  y4->insertWidget(7, txtX, 10, Qt::AlignTop);
  y4->addSpacing(10);
  y4->insertWidget(9, lblY, 10, Qt::AlignRight);
  y4->addSpacing(5);
  y4->insertWidget(11, txtY, 10, Qt::AlignTop);
  y4->addSpacing(10);
  y4->insertWidget(13, lblZ, 10, Qt::AlignRight);
  y4->addSpacing(5);
  y4->insertWidget(15, txtZ, 10, Qt::AlignTop);
  y4->addStretch(1);
  y4->setMargin(5);

  lblPermType = new QLabel(this, "lblPermType");
  lblPermType->setGeometry(QRect(0, 190, 230, 20));

  cbPermType = new QComboBox(this, "cbPermType");
  cbPermType->setGeometry(QRect(250, 190, 170, 24));

  Q3BoxLayout *y6 = new Q3HBoxLayout(x);
  y6->insertWidget(0, lblPermType, 10, Qt::AlignRight);
  y6->addSpacing(30);
  y6->insertWidget(2, cbPermType, 10, Qt::AlignTop);
  y6->addStretch(1);
  y6->setMargin(5);

  line2 = new Q3Frame(this, "line2");
  line2->setGeometry(QRect(21, 220, 400, 20));
  line2->setFrameShape(Q3Frame::HLine);
  line2->setFrameShadow(Q3Frame::Sunken);
  line2->setFrameShape(Q3Frame::HLine);

  Q3BoxLayout *y7 = new Q3HBoxLayout(x);
  y7->addSpacing(10);
  y7->addWidget(line2, 10, Qt::AlignTop);
  y7->addSpacing(10);
  y7->setMargin(5);

  sbPriority = new QSpinBox(this, "sbPriority");
  sbPriority->setGeometry(QRect(360, 240, 41, 20));
  sbPriority->setMaxValue(5);
  sbPriority->setMinValue(0);
  sbPriority->setValue(3);

  txtSN = new QLineEdit(this, "txtSN");
  txtSN->setGeometry(QRect(140, 240, 130, 23));

  lblPriority = new QLabel(this, "lblPriority");
  lblPriority->setGeometry(QRect(300, 240, 50, 21));

  lblSN = new QLabel(this, "lblSequenceName");
  lblSN->setGeometry(QRect(30, 240, 111, 21));

  Q3BoxLayout *y8 = new Q3HBoxLayout(x);
  y8->addSpacing(30);
  y8->insertWidget(1, lblSN, 10, Qt::AlignRight);
  y8->addSpacing(30);
  y8->insertWidget(3, txtSN, 10, Qt::AlignTop);
  y8->addSpacing(10);
  y8->insertWidget(5, lblPriority, 10, Qt::AlignRight);
  y8->addSpacing(30);
  y8->insertWidget(7, sbPriority, 10, Qt::AlignLeft);
  y8->addStretch(1);
  y8->setMargin(5);

  pbCancel = new QPushButton(this, "pbCancel");
  pbCancel->setGeometry(QRect(310, 280, 110, 30));
  connect(pbCancel, SIGNAL(clicked()), this, SLOT(exitProgram()));

  pbGO = new QPushButton(this, "pbGO");
  pbGO->setGeometry(QRect(30, 280, 120, 30));
  connect(pbGO, SIGNAL(clicked()), this, SLOT(pressed()));

  Q3BoxLayout *y10 = new Q3HBoxLayout(x);
  y10->addSpacing(30);
  y10->insertWidget(1, pbGO, 10, Qt::AlignTop);
  y10->addSpacing(100);
  y10->insertWidget(3, pbCancel, 10, Qt::AlignTop);
  y10->addSpacing(30);
  y10->addStretch(1);
  y10->setMargin(5);

  languageChange();
  resize(QSize(436, 318).expandedTo(minimumSizeHint()));
}

void permGenerator::languageChange() {
  setCaption(tr("Permutation Generator"));
  lblZ->setText(tr("z:"));
  lblX->setText(tr("x:"));
  lblY->setText(tr("y:"));
  txtX->insert("0");
  txtY->insert("0");
  txtZ->insert("0");
  pbGO->setText(tr("Submit Jobs"));
  lblContrasts->setText(tr("Contrast:"));
  // lblScale->setText( tr( "Scale:" ) );
  txtPermNumber->insert("0");
  lblPermNumber->setText(tr("Number of Permutations:"));
  lblPseudoT->setText(
      tr("Smoothing for Pseudo-t \n"
         "map (FWHM in Voxels):"));
  lblPermType->setText(tr(" Perm Type:"));
  pbDir->setText(tr("Select PRM File"));
  lblPriority->setText(tr("Priority:"));
  lblSN->setText(tr("Sequence Name:"));
  lblPermDir->setText(tr("Perm Directory (to be created):"));
  txtPermDir->setText("perm");

  cbPermType->addItem("sign flipping");
  cbPermType->addItem("order permutation");

  pbCancel->setText(tr("Cancel"));
  msgboxsetpermnumber = 0;
}

void permGenerator::mod(bool x) {
  txtX->setEnabled(x);
  txtY->setEnabled(x);
  txtZ->setEnabled(x);
  if (x == FALSE) {
    txtX->clear();
    txtX->insert("0");
    txtY->clear();
    txtY->insert("0");
    txtZ->clear();
    txtZ->insert("0");
  }
  return;
}

QString permGenerator::getPRMFile() {
  QString s = Q3FileDialog::getOpenFileName(
      ".", "All (*.prm)", this, "open .prm file", "Choose a .prm file to load");
  if (s == QString::null) return QString("");
  tempPRM = s.ascii();
  txtPRM->clear();
  createfullpath(s.ascii());
  txtPRM->insert(s.ascii());
  if (s == QString::null)
    return QString("");
  else
    return s;
}

void permGenerator::getContrast() {
  VB::VBContrastParamScalingWidget vv;
  vv.showBrowseButton(1);
  // vv.writeFilesOnExit(1);
  string stem = xsetextension(txtPRM->text().toStdString(), "");
  vv.LoadContrastInfo(stem);
  if (vv.exec()) {
    string cstring = "anon ";
    cstring += vv.selectedContrast()->scale + " vec ";
    GLMInfo glmi;
    glmi.setup(stem);
    for (size_t i = 0; i < glmi.interestlist.size(); i++) {
      cstring += (format("%g ") %
                  vv.selectedContrast()->contrast[glmi.interestlist[i]])
                     .str();
    }
    txtContrasts->setText(cstring.c_str());
  }
}

int commandlineExecute(int argc, char **argv) {
  GLMInfo glmi;
  string matrixStemName, permdir, seqname, clist, plist;
  VB_permtype method = vb_noperm;
  tokenlist args;
  int nperms = 1;
  uint32 rngseed = 0;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h") {
      vbpermgen_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbpermgen_version();
      exit(0);
    } else if (args[i] == "-m" && i < args.size() - 1)
      matrixStemName = args[++i];
    else if (args[i] == "-d" && i < args.size() - 1)
      permdir = args[++i];
    else if (args[i] == "-t" && i < args.size() - 1)
      method = permclass::methodtype(args[++i]);
    else if (args[i] == "-n" && i < args.size() - 1)
      nperms = strtol(args[++i]);
    else if (args[i] == "-a" && i < args.size() - 1)
      seqname = args[++i];
    else if (args[i] == "-s" && i < args.size() - 1)
      rngseed = strtol(args[++i]);
    else if (args[i] == "-c" && i < args.size() - 1)
      clist = args[++i];
    else if (args[i] == "-p" && i < args.size() - 1)
      plist = args[++i];
    else if (args[i] == "-b" && i < args.size() - 1)
      seq.priority.set(args[++i]);
  }

  if (matrixStemName.size() == 0) return 2;
  if (permdir.size() == 0) return 3;
  if (clist.size() == 0) return 7;

  int maxPerms = 0;
  glmi.setup(matrixStemName);
  glmi.gMatrix.ReadHeader(glmi.stemname + ".G");
  double orderG = glmi.gMatrix.m;
  glmi.setup(matrixStemName);

  if (pow(2.0, (orderG - 1)) < 1000)
    maxPerms = (int)pow(2.0, (orderG - 1));
  else
    maxPerms = 100000;
  if (seqname.size() == 0) seqname = glmi.stemname;
  if (nperms > maxPerms) return 8;

  string presentDir = xgetcwd() + "/";
  struct stat my_stat;
  if (glmi.parsecontrast(clist) != 0) {
    cout << "[E] vbpermgen: failed to derive a valid contrast from " << clist
         << endl;
    return -1;
  }
  permclass pc;
  pc.rngseed = rngseed;
  pc.stemname = glmi.stemname;
  pc.permdir = permdir;
  pc.method = method;
  pc.contrast = clist;
  pc.pseudotlist = plist;
  pc.SetFileName("perminfo.txt");
  pc.AddPrmTimeStamp(glmi.stemname);
  string pfile = xdirname(glmi.stemname) + "/" + permdir + "/permutations.mat";
  permStart(pc);
  pc.SavePermClass();
  memset((void *)&my_stat, 0, sizeof(my_stat));
  seq.init();
  QString name;
  int count = 0;
  if (nperms > 0) {
    for (int permIndex = 0; permIndex < nperms; permIndex++) {
      js.init();
      js.arguments.clear();
      js.jobtype = "shellcommand";
      string cmd =
          (format("permstep -m %s -d %s -t %s -c '%s' %s -n %d") %
           glmi.stemname % permdir % permclass::methodstring(method) % clist %
           (plist.size() ? (string) "-p " + plist : "") % permIndex)
              .str();
      if (method == vb_signperm) cmd += " -e";
      js.arguments["command"] = cmd;
      js.magnitude = 0;
      js.name = permdir;
      js.dirname = presentDir;
      js.logdir = xdirname(glmi.stemname) + "/" + permdir + "/logs";
      js.jnum = permIndex;
      count = permIndex;
      seq.addJob(js);
    }
  }
  js.init();
  js.arguments.clear();
  js.jobtype = "shellcommand";
  string cmd =
      (format("vbperminfo -p %s %s 0.05") %
       (xdirname(glmi.stemname) + "/" + permdir + "/iterations/permcube") %
       (xdirname(glmi.stemname) + "/" + permdir + "/results.ref"))
          .str();
  js.arguments["command"] = cmd;
  js.magnitude = 0;
  js.name = "finish";
  js.dirname = presentDir;
  js.logdir = xdirname(glmi.stemname) + "/" + permdir + "/logs";
  js.jnum = count + 1;
  for (int i = 0; i <= count; i++) js.waitfor.insert(i);
  seq.addJob(js);
  seq.name = seqname;
  if (seq.name.size() == 0) seq.name = "perm" + matrixStemName;
  seq.seqnum = (int)getpid();
  if (vbp.cores == 0) {
    if (seq.Submit(vbp) == 0) {
      cout << "[I] vbpermgen: successfully submitted the sequence.\n";
    } else {
      cout << "[E] vbpermgen: failed to submit the sequence.\n";
      return -1;
    }
  } else {
    runseq(vbp, seq, vbp.cores);
  }
  return 0;
}

void permGenerator::pressed() {
  GLMInfo glmi;
  string presentDir = xgetcwd() + "/";
  string matrixStemName = txtPRM->text().toStdString();
  string permDir = txtPermDir->text().toStdString();
  // string scale = cbScale->currentText().toStdString();
  VB_permtype method;
  if (cbPermType->currentItem() == 0)
    method = vb_signperm;
  else
    method = vb_orderperm;
  int numPerms = strtol(txtPermNumber->text().toStdString());
  string contrast = txtContrasts->text().toStdString();
  string plist = (string)txtX->text().toStdString() + " " +
                 txtY->text().toStdString() + " " + txtZ->text().toStdString();
  if (!txtX->isEnabled()) plist = "0 0 0";
  permclass pc;
  glmi.setup(matrixStemName);
  pc.stemname = glmi.stemname;
  pc.permdir = permDir;
  pc.method = method;
  pc.contrast = contrast;
  pc.pseudotlist = plist;
  pc.AddPrmTimeStamp(glmi.stemname);
  pc.SetFileName("perminfo.txt");
  if (matrixStemName.size() == 0) {
    QMessageBox::warning(
        this, "Warning!",
        "Must specify the matrix stem name (glm directory).\n");
    return;
  }
  if (permDir.size() == 0) {
    QMessageBox::warning(this, "Warning!",
                         "Must specify the permutation directory name.\n");
    return;
  }
  // FIXME test validity of contrast?
  struct stat my_stat;
  // check permutation directory if mat file already exists
  string pfile = xdirname(glmi.stemname) + "/" + permDir + "permutations.mat";
  // FIXME if permstart returns an error, we're in trouble.  but most
  // of permstart shouold really be a job instead
  permStart(pc);
  pc.SavePermClass();
  memset((void *)&my_stat, 0, sizeof(my_stat));
  seq.init();
  QString name;
  int count = 0;
  if (numPerms > 0) {
    for (int permIndex = 0; permIndex < numPerms; permIndex++) {
      js.init();
      js.arguments.clear();
      js.jobtype = "shellcommand";
      string cmd =
          (format("permstep -m %s -d %s -t %s -c '%s' %s -n %d") %
           glmi.stemname % permDir % permclass::methodstring(method) %
           contrast % (plist.size() ? (string) "-p " + plist : "") % permIndex)
              .str();
      if (method == vb_signperm) cmd += " -e";
      js.arguments["command"] = cmd;
      js.magnitude = 0;
      js.name = permDir;
      js.dirname = presentDir;
      js.logdir = xdirname(glmi.stemname) + "/" + permDir + "/logs";
      js.jnum = permIndex;
      count = permIndex;
      seq.addJob(js);
    }
  }
  js.init();
  js.arguments.clear();
  js.jobtype = "shellcommand";
  string cmd =
      (format("vbperminfo -p %s %s 0.05") %
       (xdirname(glmi.stemname) + "/" + permDir + "/iterations/permcube") %
       (xdirname(glmi.stemname) + "/" + permDir + "/results.ref"))
          .str();
  js.arguments["command"] = cmd;
  js.magnitude = 0;
  js.name = "finish";
  js.dirname = presentDir;
  js.logdir = xdirname(glmi.stemname) + "/" + permDir + "/logs";
  js.jnum = count + 1;
  for (int i = 0; i <= count; i++) js.waitfor.insert(i);
  seq.addJob(js);
  seq.name = txtSN->text().ascii();
  if (seq.name.size() == 0) seq.name = "perm" + matrixStemName;
  seq.priority = sbPriority->value();
  seq.seqnum = (int)getpid();
  if (vbp.cores == 0) {
    if (seq.Submit(vbp) == 0) {
      QMessageBox::warning(this, "Information",
                           "The requested sequence was submitted.\n");
      return;
    } else {
      QMessageBox::warning(this, "Information",
                           "The requested sequence failed to submit.\n");
      return;
    }
  } else {
    // runseq(vbp,seq,vbp.cores);
    QRunSeq qr;
    qr.Go(vbp, seq, vbp.cores);
    qr.exec();
  }
  msgboxsetpermnumber = 0;
  return;
}

int permGenerator::warnCalculateNumPerms() {
  if (msgboxsetpermnumber) return 1;
  VBMatrix headerMatrix;
  GLMInfo glmi;
  struct stat my_stat;
  int numPerms = 0;
  string matrixStemName;
  if (txtPRM->text().length())
    matrixStemName = txtPRM->text().ascii();
  else
    matrixStemName = tempPRM;
  string headerName = xrootname(matrixStemName) + ".G";
  if (stat(headerName.c_str(), &my_stat)) {
    return 1;
  }
  headerMatrix.ReadHeader(headerName);
  double orderG = (double)headerMatrix.m;
  if (pow(2.0, (orderG - 1)) < 1000)
    numPerms = (int)pow(2.0, (orderG - 1));
  else
    numPerms = 1000;
  char num[STRINGLEN];
  char obs[STRINGLEN];
  char old[STRINGLEN];
  sprintf(num, "%d", numPerms);
  sprintf(obs, "%d", (int)orderG);
  sprintf(old, "%d", atoi(txtPermNumber->text().ascii()));
  string snum = num;
  string og = obs;
  string sold = old;
  string msg = "Maximum permutations is " + snum +
               " based on your data.\n Do you want to generate the " + snum +
               " possible?";
  if (atoi(txtPermNumber->text().ascii()) > numPerms) {
    QMessageBox mb("vbpermgen", msg.c_str(), QMessageBox::Information,
                   QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
                   QMessageBox::Cancel | QMessageBox::Escape);
    mb.setButtonText(QMessageBox::Yes, "Replace");
    mb.setButtonText(QMessageBox::No, "Abort");
    switch (mb.exec()) {
      case QMessageBox::Yes:
        msgboxsetpermnumber = 1;
        txtPermNumber->clear();
        txtPermNumber->insert(num);
        msgboxsetpermnumber = 0;
        return 0;
        break;
      case QMessageBox::No:
        msgboxsetpermnumber = 1;
        txtPermNumber->clear();
        txtPermNumber->insert("0");
        msgboxsetpermnumber = 0;
        return 1;
        break;
      case QMessageBox::Cancel:
        msgboxsetpermnumber = 1;
        txtPermNumber->clear();
        txtPermNumber->insert("0");
        msgboxsetpermnumber = 0;
        return 1;
        break;
    }
  }
  return numPerms;
}

void permGenerator::CalculateNumPerms() {
  if (msgboxsetpermnumber) return;
  VBMatrix headerMatrix;
  GLMInfo glmi;
  struct stat my_stat;
  int numPerms = 0;
  string matrixStemName;
  if (txtPRM->text().length())
    matrixStemName = txtPRM->text().ascii();
  else
    matrixStemName = tempPRM;
  string headerName = xrootname(matrixStemName) + ".G";
  if (stat(headerName.c_str(), &my_stat)) {
    return;
  }
  headerMatrix.ReadHeader(headerName);
  double orderG = (double)headerMatrix.m;
  if (pow(2.0, (orderG - 1)) < 1000)
    numPerms = (int)pow(2.0, (orderG - 1));
  else
    numPerms = 1000;
  char num[STRINGLEN];
  sprintf(num, "%d", numPerms);
  txtPermNumber->clear();
  txtPermNumber->insert(num);
  return;
}

void permGenerator::closeEvent(QCloseEvent *ce) {
  ce->accept();
  return;
}

void permGenerator::exitProgram() {
  exit(0);
  return;
}

void vbpermgen_help() { cout << boost::format(myhelp) % vbversion; }

void vbpermgen_version() {
  printf("VoxBo vbpermgen (v%s)\n", vbversion.c_str());
}
