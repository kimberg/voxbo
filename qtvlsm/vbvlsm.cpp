
// vbvlsm.cpp
// VoxBo analysis tool
// Copyright (c) 2009 by The VoxBo Development Team

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

// FIXME set up a set() of time points to omit
// FIXME do it now or queue jobs, if doing it now, display the results

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPalette>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QValidator>

#include <boost/format.hpp>

#include "../stand_alone/perm.h"
#include "glmutil.h"
#include "myboxes.h"
#include "stats.h"
#include "tokenlist.h"
#include "vbio.h"
#include "vbutil.h"

using namespace std;
using boost::format;

// FIXME global required for old perm mat code
gsl_rng *theRNG = NULL;

const string p_dim1d = "%d elements";
const string p_dim2d = "%d variables of length %d";
const string p_dim3d = "%dx%dx%d";
const string p_dim4d = "%d volumes of %dx%dx%d";

enum t_stat {
  id_ttest,
  id_welchs,
  id_regression,
  id_resid,
  id_noperm,
  id_dvperm,
  id_ivperm
};
enum t_loglevel { t_status, t_log, t_error };

class ModelItem {
 public:
  string fullname;
  string filename;
  vector<int> dims;
  string dimstring;
  int criticaldim;
  int nvars;
  QTreeWidgetItem *item;
  bool f_iv, f_dv, f_pv;
  Tes tdata;
  VB_Vector vdata;
  VBMatrix mdata;
};

// collection of model items types
typedef vector<ModelItem> MIList;
typedef vector<ModelItem>::iterator MII;
typedef pair<string, ModelItem> MP;

class VSetup : public QObject {
  Q_OBJECT
 public:
  QApplication *app;
  QMainWindow *mainwindow;
  QFrame *mainframe;
  QVBoxLayout *mainlayout;
  QPalette bgpal, pal2;
  QLineEdit *w_dirname;
  QLineEdit *w_nperms;
  QLineEdit *w_stem;
  QHBox *w_permbox;
  QCheckBox *w_permcb;

  QTreeWidget *tree;
  QGroupBox *statbox;

  MIList itemlist;

  // interface
  void init(int argc, char **argv);
  void build_mainwindow();
  void build_filelist(string dir);
  // stat functions
  void run_ttest();
  void run_regression();
  int do_regression_volume(int permindex);
  int do_ttest_volume(int permindex);
  int do_regression_single(int permindex);
  int do_ttest_single(int permindex);
  // stat config and data
  t_stat mystat;
  int dimx, dimy, dimz;
  map<string, ModelItem> ivmap;
  map<string, ModelItem> dvmap;
  VBMatrix permmat;
  int permcount;
  Cube tmap, pmap, zmap;
  Cube mask;
  VB_Vector permdist;
  int ivcount, dvcount, critsize;
  bool f_volume;
  VBMatrix pmat;
  int ivperm;
  int dvperm;
 public slots:
  void dchandler(QTreeWidgetItem *, int);
  void handle_treecontext(const QPoint &pos);
  void handle_selectionchanged();
  void handle_statbutton(int val);
  void handle_permcheckbox(bool state);
  void handle_dependent(MII mi);
  void handle_independent(MII mi);
  void handle_permute(MII mi);
  void handle_remove();
  void handle_clear();
  void handle_newdir();
  void setstatus(const string &str, t_loglevel level = t_error);
  void go();
  void runstats();
};

#include "vbvlsm.moc.h"

enum { col_name = 0, col_dims, col_len, col_dv, col_iv, col_pv };

void VSetup::dchandler(QTreeWidgetItem *item, int col) {
  MII it;
  for (it = itemlist.begin(); it != itemlist.end(); it++)
    if (it->item == item) break;
  if (it == itemlist.end()) return;
  if (col == col_dv) {
    handle_dependent(it);
  }
  if (col == col_iv) {
    handle_independent(it);
  }
  if (col == col_pv) {
    handle_permute(it);
  }
}

void VSetup::handle_selectionchanged() {
  // FIXME no actual reason to handle this
}

int main(int argc, char **argv) {
  VSetup vv;
  vv.init(argc, argv);
  vv.build_mainwindow();
  vv.go();
}

void VSetup::init(int argc, char **argv) {
  app = new QApplication(argc, argv);
  //  build_filelist("/junk/vtest");
  permcount = 1;
  ivperm = 0;
  dvperm = 0;
}

void VSetup::go() { app->exec(); }

void VSetup::build_filelist(string dir) {
  tokenlist files;
  vglob vg(dir + "/*");
  for (size_t i = 0; i < vg.size(); i++) {
    if (vb_fileexists(vg[i])) files.Add(vg[i]);
  }

  Tes mytes;
  VB_Vector vec;
  VBMatrix mat;
  for (size_t i = 0; i < files.size(); i++) {
    ModelItem item;
    item.f_dv = item.f_iv = item.f_pv = 0;
    item.filename = files[i];
    item.nvars = 1;
    if (mytes.ReadHeader(item.filename) == 0) {
      item.dims.push_back(mytes.dimx);
      item.dims.push_back(mytes.dimy);
      item.dims.push_back(mytes.dimz);
      item.dims.push_back(mytes.dimt);
      item.dimstring = (boost::format(p_dim4d) % mytes.dimt % mytes.dimx %
                        mytes.dimy % mytes.dimz)
                           .str();
      item.criticaldim = mytes.dimt;
    }
    //     else if (mycube.ReadHeader()==0) {
    //       item.dims=0;
    //       item.criticaldim=mytes.dimt;
    //     }
    else if (mat.ReadFile(item.filename) == 0) {
      item.dims.push_back(mat.rows);
      item.dims.push_back(mat.cols);
      item.dimstring = (boost::format(p_dim2d) % mat.cols % mat.rows).str();
      item.nvars = mat.cols;
      item.criticaldim = mat.rows;
      if (mat.cols == 1) item.vdata = mat.GetColumn(0);
      item.mdata = mat;
    } else if (vec.ReadFile(item.filename) == 0) {
      if (vec.size()) {
        item.dims.push_back(vec.size());
        item.dimstring = (boost::format(p_dim1d) % vec.size()).str();
        item.criticaldim = vec.size();
        item.vdata = vec;
      }
    } else
      continue;
    itemlist.push_back(item);
  }

  // clear tree and a few random structures
  tree->clear();
  ivmap.clear();
  dvmap.clear();
  // tree header stuff
  QStringList *strings = new QStringList();
  strings->clear();
  *strings << "filename"
           << "dims"
           << "len"
           << "DV"
           << "IV"
           << "PV";
  tree->setHeaderLabels(*strings);

  // go through the files and add to three
  QTreeWidgetItem *it;
  for (size_t i = 0; i < itemlist.size(); i++) {
    strings->clear();
    *strings << itemlist[i].filename.c_str() << itemlist[i].dimstring.c_str()
             << strnum(itemlist[i].criticaldim).c_str() << ""
             << "";
    it = new QTreeWidgetItem(*strings);
    tree->addTopLevelItem(it);
    // it->setFlags(it->flags() | Qt::ItemIsEditable);
    itemlist[i].item = it;
    // connect it all up
  }
  tree->resizeColumnToContents(0);
  tree->resizeColumnToContents(1);
  tree->resizeColumnToContents(2);
  tree->resizeColumnToContents(3);
  tree->resizeColumnToContents(4);
  w_dirname->setText(dir.c_str());
}

void VSetup::handle_treecontext(const QPoint &pt) {
  QTreeWidgetItem *it = tree->itemAt(pt);
  cout << (long)it << endl;
  //   QMenu popup;
  //   popup.addAction("foo");
  //   popup.addAction("bar");
  //   popup.addAction("baz!!!");
  //   QAction *act=popup.exec(tree->mapToGlobal(pt));
}

void VSetup::handle_statbutton(int val) {
  if (val == id_ttest)
    mystat = id_ttest;
  else if (val == id_welchs)
    mystat = id_welchs;
  else if (val == id_regression)
    mystat = id_regression;
  else if (val == id_resid)
    mystat = id_resid;
  else
    cout << "FIXME shouldn't happen (statbutton)" << endl;
}

void VSetup::handle_permcheckbox(bool state) {
  w_permbox->setVisible(state);
  return;
}

void VSetup::handle_dependent(MII mi) {
  // if it's already a DV, unset it
  if (mi->f_dv) {
    mi->item->setIcon(col_dv, QIcon());
    mi->f_dv = 0;
    dvmap.clear();
    return;
  }
  // it's a DV
  mi->item->setDisabled(0);
  mi->item->setIcon(col_dv, QIcon(QPixmap(":/icons/icon_check.png")));
  dvmap[mi->filename] = *mi;
  mi->f_dv = 1;
  int critlen = mi->criticaldim;
  if (mi->f_iv) {
    ivmap.erase(mi->filename);
    mi->item->setIcon(col_iv, QIcon());
    mi->f_iv = 0;
  }

  // go through remaining items, gray out ones with bad length, cancel out ones
  // with dv selected
  for (MII mix = itemlist.begin(); mix != itemlist.end(); mix++) {
    if (mix == mi) continue;
    if (mix->criticaldim != critlen) {
      mix->item->setIcon(col_iv, QIcon());
      mix->item->setIcon(col_dv, QIcon());
      mix->item->setIcon(col_pv, QIcon());
      mix->f_iv = 0;
      mix->f_dv = 0;
      mix->f_pv = 0;
      mix->item->setDisabled(1);
      continue;
    } else if (mix->f_dv) {
      mix->f_dv = 0;
      mix->item->setIcon(col_dv, QIcon());
    } else
      mix->item->setDisabled(0);
  }
}

void VSetup::handle_independent(MII mi) {
  // if it's already an IV, unset it
  if (mi->f_iv) {
    mi->item->setIcon(col_iv, QIcon());
    mi->f_iv = 0;
    ivmap.erase(mi->filename);
    return;
  }
  // it's an IV
  mi->item->setDisabled(0);
  mi->item->setIcon(col_iv, QIcon(QPixmap(":/icons/icon_check.png")));
  ivmap[mi->filename] = *mi;
  mi->f_iv = 1;
  int critlen = mi->criticaldim;
  // it's not a DV
  if (mi->f_dv) {
    dvmap.clear();
    mi->item->setIcon(col_dv, QIcon());
    mi->f_dv = 0;
  }

  // go through remaining items, gray out ones with bad length
  for (MII mix = itemlist.begin(); mix != itemlist.end(); mix++) {
    if (mix == mi) continue;
    if (mix->criticaldim != critlen) {
      mix->item->setIcon(col_iv, QIcon());
      mix->item->setIcon(col_dv, QIcon());
      mix->item->setIcon(col_pv, QIcon());
      mix->f_iv = 0;
      mix->f_dv = 0;
      mix->f_pv = 0;
      mix->item->setDisabled(1);
    } else
      mix->item->setDisabled(0);
  }
}

void VSetup::handle_permute(MII mi) {
  if (mi->f_iv) {
    // just de-iv it
    mi->item->setIcon(col_iv, QIcon());
    mi->f_iv = 0;
    return;
  }
}

void VSetup::handle_remove() {}

void VSetup::handle_clear() {
  for (MII mi = itemlist.begin(); mi != itemlist.end(); mi++) {
    mi->item->setDisabled(0);
    mi->item->setText(3, "");
    // mi->type=t_none;
  }
  tree->clearSelection();
}

void VSetup::handle_newdir() {
  QString newdir = QFileDialog::getExistingDirectory(
      NULL,
      QString("Which folder/directory contains the files for your analysis?"));
  if (newdir.size()) {
    // chdir(newdir.toStdString().c_str());
    chdir(newdir.toAscii());
    build_filelist(newdir.toStdString());
  }
}

void VSetup::build_mainwindow() {
  QFrame *myframe;
  // string sectionstyle="QLabel {background:#ffffee;border:2px solid red;}";
  string sectionstyle = "QLabel {border:1px solid black;}";
  // string framestyle="QFrame {padding-left:40px;border:1px solid black;}";

  mainwindow = new QMainWindow();
  mainwindow->resize(660, 600);
  mainframe = new QFrame();
  mainwindow->setCentralWidget(mainframe);
  mainlayout = new QVBoxLayout();
  mainlayout->setSpacing(8);
  mainlayout->setMargin(4);
  mainlayout->setAlignment(Qt::AlignTop);
  mainframe->setLayout(mainlayout);

  QLabel *label;
  QHBoxLayout *hbox;
  QVBoxLayout *vbox, *fbox;
  QPushButton *button;

  // SELECT VARIABLES

  myframe = new QFrame();
  // myframe->setStyleSheet(framestyle.c_str());
  mainlayout->addWidget(myframe);
  fbox = new QVBoxLayout();
  myframe->setLayout(fbox);

  label = new QLabel("<b>Select your variables/files</b>");
  label->setStyleSheet(sectionstyle.c_str());
  fbox->addWidget(label);

  //  mainwindow->show();
  // return;

  hbox = new QHBoxLayout();
  fbox->addLayout(hbox);
  hbox->setSpacing(2);
  hbox->setMargin(0);
  hbox->addSpacing(40);

  button = new QPushButton("Directory");
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handle_newdir()));
  w_dirname = new QLineEdit();
  hbox->addWidget(w_dirname);
  w_dirname->setText("");

  tree = new QTreeWidget();
  tree->setRootIsDecorated(0);
  fbox->addWidget(tree);
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  // tree->setFocusPolicy(Qt::NoFocus);
  tree->setStyleSheet(
      "selection-background-color:#d0b0b0;alternate-background-color:yellow;");
  tree->setContextMenuPolicy(Qt::CustomContextMenu);
  tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QStringList *strings = new QStringList();
  strings->clear();
  *strings << "filename"
           << "dims"
           << "len"
           << "DV"
           << "IV"
           << "PV";
  tree->setHeaderLabels(*strings);

  QObject::connect(tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
                   this, SLOT(dchandler(QTreeWidgetItem *, int)));
  QObject::connect(tree, SIGNAL(customContextMenuRequested(const QPoint &)),
                   this, SLOT(handle_treecontext(const QPoint &)));
  QObject::connect(tree, SIGNAL(itemSelectionChanged()), this,
                   SLOT(handle_selectionchanged()));

  label = new QLabel(
      "DV: dependent variable; IV: independent variable; PV: variable to "
      "permute");
  fbox->addWidget(label);

  hbox = new QHBoxLayout();
  hbox->setAlignment(Qt::AlignLeft);
  fbox->addLayout(hbox);

  button = new QPushButton("remove");
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handle_remove()));

  button = new QPushButton("clear all");
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(handle_clear()));

  // SELECT TEST STATISTIC

  myframe = new QFrame();
  // myframe->setStyleSheet(framestyle.c_str());
  mainlayout->addWidget(myframe);
  vbox = new QVBoxLayout();
  myframe->setLayout(fbox);

  label = new QLabel("<b>Select your test statistic</b>");
  label->setStyleSheet(sectionstyle.c_str());
  mainlayout->addWidget(label);

  // statbox=new QGroupBox();  // ("Select your test statistic");
  hbox = new QHBoxLayout();
  mainlayout->addLayout(hbox);
  hbox->setSpacing(2);
  hbox->setMargin(0);
  hbox->addSpacing(40);
  vbox = new QVBoxLayout();
  hbox->addLayout(vbox);

  QButtonGroup *bg = new QButtonGroup();
  vbox->setSpacing(0);

  QRadioButton *rb;

  rb = new QRadioButton("ttest (w/df, p, and z maps)");
  rb->setChecked(1);
  mystat = id_ttest;
  bg->addButton(rb);
  bg->setId(rb, id_ttest);
  vbox->addWidget(rb);

  rb = new QRadioButton("welch's (w/df, p, and z maps)");
  bg->addButton(rb);
  bg->setId(rb, id_welchs);
  vbox->addWidget(rb);

  rb = new QRadioButton("regression t (w/df, p, and z maps)");
  bg->addButton(rb);
  bg->setId(rb, id_regression);
  vbox->addWidget(rb);

  rb = new QRadioButton("residual map");
  bg->addButton(rb);
  bg->setId(rb, id_resid);
  vbox->addWidget(rb);

  vbox->addStretch(1);
  // statbox->setLayout(vbox);
  // mainlayout->addWidget(statbox);

  QObject::connect(bg, SIGNAL(buttonClicked(int)), this,
                   SLOT(handle_statbutton(int)));

  label = new QLabel("<b>Permutation testing</b>");
  label->setStyleSheet(sectionstyle.c_str());
  mainlayout->addWidget(label);

  hbox = new QHBoxLayout();
  hbox->setSizeConstraint(QLayout::SetFixedSize);
  mainlayout->addLayout(hbox);
  hbox->setAlignment(Qt::AlignLeft);
  hbox->addSpacing(40);
  w_permcb = new QCheckBox("Permutation test");
  hbox->addWidget(w_permcb);

  QObject::connect(w_permcb, SIGNAL(toggled(bool)), this,
                   SLOT(handle_permcheckbox(bool)));

  w_permbox = new QHBox;
  hbox->addWidget(w_permbox);
  w_nperms = new QLineEdit();
  // w_nperms->setValidator(new QIntValidator(0,10000,w_stem));
  w_permbox->addWidget(w_nperms);
  w_nperms->setText(QString("1000"));
  label = new QLabel("permutations (order of DV)");
  w_permbox->addWidget(label);
  w_permbox->setVisible(0);

  label = new QLabel("<b>Output</b>");
  label->setStyleSheet(sectionstyle.c_str());
  mainlayout->addWidget(label);

  hbox = new QHBoxLayout();
  mainlayout->addLayout(hbox);
  hbox->setSpacing(2);
  hbox->setMargin(0);
  hbox->addSpacing(40);
  hbox->setAlignment(Qt::AlignLeft);
  label = new QLabel("Stem for output files:");
  hbox->addWidget(label);
  w_stem = new QLineEdit();
  hbox->addWidget(w_stem);
  w_stem->setText(QString("myvlsm"));

  mainlayout->addStretch(100);

  hbox = new QHBoxLayout();
  // hbox->setAlignment(Qt::AlignLeft);

  hbox->setSpacing(2);
  hbox->setMargin(0);
  // hbox->addSpacing(40);
  mainlayout->addLayout(hbox);

  button = new QPushButton("GO!");
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(runstats()));

  hbox->addStretch(1);

  mainwindow->show();
}

// type is MSG_STATUS or MSG_LOG
void VSetup::setstatus(const string &str, t_loglevel level) {
  if (level == t_error) cout << "ERR: ";
  // put message in status line, log window, error popup, and possibly log file
  cout << "STATUS: " << str << endl;
}

void VSetup::runstats() {
  critsize = 0;
  f_volume = 0;
  dimx = dimy = dimz = 0;
  ivcount = dvcount = 0;
  mask.invalidate();

  Tes ts;
  if (w_permcb->isChecked())
    permcount = strtol(w_nperms->text().toStdString());
  else
    permcount = 1;
  if (permcount < 1) permcount = 1;
  if (permcount > 1) permdist.resize(permcount);

  // set up QProgressDialog with either the number of iterations or
  // the number of in-mask voxels

  // first pass over variables to count variables, set crit size,
  // build mask, etc.
  vbforeach(MP mi, ivmap) {
    // vbforeach (ModelItem &mi,ivmap) {
    ivcount += mi.second.nvars;
    if (critsize == 0) critsize = mi.second.criticaldim;
    if (mi.second.criticaldim != critsize) {
      setstatus("bogus variable size, shouldn't happen", t_error);
      return;
    }
    if (mi.second.dims.size() == 4) {
      f_volume = 1;
      if (dimx == 0) {
        dimx = mi.second.dims[0];
        dimy = mi.second.dims[1];
        dimz = mi.second.dims[2];
      }
      if (mi.second.dims[0] != dimx || mi.second.dims[1] != dimy ||
          mi.second.dims[2] != dimz) {
        setstatus("bad 4D volume size", t_error);
        return;
      }
      if (ts.ReadHeader(mi.second.filename)) {
        setstatus("couldn't read 4D file header", t_error);
        return;
      }
      Cube tmpmask;
      if (ts.ExtractMask(tmpmask) == 0) {
        if (mask.data)
          mask.intersect(tmpmask);
        else
          mask = tmpmask;
      }
    }
  }
  vbforeach(MP mi, dvmap) {
    dvcount += mi.second.nvars;
    if (critsize == 0) critsize = mi.second.criticaldim;
    if (mi.second.criticaldim != critsize) {
      setstatus("bogus variable size, shouldn't happen", t_error);
      return;
    }
  }

  // run the right test
  if (mystat == id_ttest || mystat == id_welchs)
    run_ttest();
  else if (mystat == id_regression || mystat == id_resid)
    run_regression();
  else
    setstatus("shouldn't happen (runstats)", t_error);
}

void VSetup::run_ttest() {
  if (ivcount != 1) {
    setstatus(
        "For a t test, you must select a grouping variable as your independent "
        "variable.",
        t_error);
    return;
  }
  if (dvcount != 1) {
    setstatus(
        "For a t test, you must select a single dependent variable or volume.");
    return;
  }

  int err;
  int permindex = -1;  // -1 means don't permute
  if (permcount > 1)
    pmat = createPermMatrix(permcount, critsize, vb_orderperm, 0);
  format progformat("Calculating statmaps (%d of %d completed) ...");
  QProgressDialog prog((progformat % 0 % permcount).str().c_str(), "Abort", 0,
                       permcount);
  prog.setWindowModality(Qt::WindowModal);
  prog.show();
  for (int i = 0; i < permcount; i++) {
    prog.setLabelText((progformat % i % permcount).str().c_str());
    prog.setValue(i);
    if (permcount > 1) permindex = i;
    if (f_volume)
      err = do_ttest_volume(permindex);
    else
      err = do_ttest_single(permindex);
    if (err) break;  // FIXME
  }
  prog.setValue(permcount);
  // permutation test cleanup
  if (permcount > 1) {
    permdist.WriteFile("permdist.ref");
    // FIXME and more?
  } else if (mystat == id_welchs) {
    tmap.WriteFile("tmap.cub.gz");
    pmap.WriteFile("pmap.cub.gz");
    zmap.WriteFile("zmap.cub.gz");
  } else if (mystat == id_ttest) {
    tmap.WriteFile("tmap.cub.gz");
    pmap.WriteFile("pmap.cub.gz");
    zmap.WriteFile("zmap.cub.gz");
  }
}

void VSetup::run_regression() {
  if (permcount > 1) {
    // FIXME make a permutation matrix here
  }
  for (int i = 0; i < permcount; i++) {
  }
  // if it was a permutation test, do perm cleanup
  // otherwise, save the output
}

int VSetup::do_ttest_volume(int permindex) {
  VB_Vector dv, pvec;
  tmap.SetVolume(dimx, dimy, dimz, vb_double);
  pmap.SetVolume(dimx, dimy, dimz, vb_double);
  zmap.SetVolume(dimx, dimy, dimz, vb_double);
  // vec and single-column matrix data are already in there as vecs
  ModelItem ivitem = ivmap.begin()->second;
  ModelItem dvitem = dvmap.begin()->second;
  int order = ivitem.criticaldim;
  bitmask bm;
  bm.resize(order);
  // read any 4d data, otherwise copy to iv/dv
  if (ivitem.dims.size() == 4) {
    // FIXME test error
    ivitem.tdata.ReadFile(ivitem.filename);
  } else {
    for (int t = 0; t < order; t++) {
      if (fabs(ivitem.vdata[t]) > FLT_MIN)
        bm.set(t);
      else
        bm.unset(t);
    }
  }
  if (dvitem.dims.size() == 4)
    dvitem.tdata.ReadFile(dvitem.filename);
  else
    dv = dvitem.vdata;

  // FIXME right now we only offer the option of order-permuting the
  // dv.  if it's a vector we do it once, here.
  if (permcount > 1) pvec = pmat.GetColumn(permindex);
  if (permcount > 1 && dvitem.dims.size() == 1) {
    for (uint32 i = 0; i < dv.size(); i++) dv[i] = dvitem.vdata[(int)pvec[i]];
  }

  tval res;

  for (int i = 0; i < dimx; i++) {
    for (int j = 0; j < dimy; j++) {
      for (int k = 0; k < dimz; k++) {
        // if either var is 4d, get the next time series
        if (ivitem.dims.size() == 4) {
          for (int t = 0; t < order; t++) {
            if (ivitem.tdata.getValue<int16>(i, j, k, t))
              bm.set(t);
            else
              bm.unset(t);
          }
        }
        // if it's 4d data, grab the right time series
        if (dvitem.dims.size() == 4) {
          dvitem.tdata.GetTimeSeries(i, j, k);
          // FIXME right now we only offer the option of order
          // permuting the dv.  if it's image data, we do it for each
          // position, here
          if (permcount > 1) {
            for (uint32 i = 0; i < dv.size(); i++)
              dv[i] = dvitem.tdata.timeseries[(int)pvec[i]];
          } else
            dv = dvitem.tdata.timeseries;
        }
        if (mystat == id_welchs)
          res = calc_welchs(dv, bm);
        else
          res = calc_ttest(dv, bm);
        // not a permutation test, save everything
        if (permcount < 2) {
          tmap.SetValue(i, j, k, res.t);
          t_to_p_z(res);
          pmap.SetValue(i, j, k, res.p);
          zmap.SetValue(i, j, k, res.z);
        }
        // perm test with welch's, convert to z
        else if (mystat == id_welchs) {
          t_to_p_z(res);
          zmap.SetValue(i, j, k, res.z);
        }
        // perm test with regular t-test, save t value
        else if (mystat == id_ttest) {
          tmap.SetValue(i, j, k, res.t);
        }
      }
    }
  }
  if (permcount > 1) {
    double max;
    if (mystat == id_welchs)
      max = zmap.get_maximum();
    else
      max = tmap.get_maximum();
    permdist[permindex] = max;
  } else {
    // FIXME do FDR
    // calc bonferroni for unique voxels
  }

  // FIXME obey mask
  // if we're not doing a permutation test:
  //   do z conversion, saving both p and z
  //   flip if requested
  //   calc fdr thresh
  //   calc bonferroni for unique voxels thresh
  // if we are doing a perm test, just stash the stat value
  // (same for regular ttest, but call calc_ttest() instead)
  return 0;
}

int VSetup::do_regression_volume(int permindex) {
  cout << permindex << endl;
  bool f_resid = (mystat == id_resid);
  cout << f_resid << endl;
  GLMInfo glmi;
  vector<string> ivnames;
  string dvname, resname;
  vector<VBMatrix> ivmats;  // not used
  // FIXME build ivnames and dvnames -- volumeregress() will load them for us
  glmi.rescount = 0;
  int err = glmi.VolumeRegress(mask, 1, 1, ivnames, dvname, ivmats);
  // FIXME check error
  if (err) return err;
  glmi.residtes.WriteFile(resname);
  return 0;
}

int VSetup::do_regression_single(int permindex) {
  cout << permindex << endl;
  return 0;
}

int VSetup::do_ttest_single(int permindex) {
  cout << permindex << endl;
  return 0;
}
