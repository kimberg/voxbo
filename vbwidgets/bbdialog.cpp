
// bbdialog.cpp
// dialogs for structure browser
// Copyright (c) 2010 by The VoxBo Development Team

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
// original version written by Dan Kimberg, based on a design by
// Dongbo Hu.

#include "bbdialog.h"
#include <QFileInfo>
#include <QMessageBox>

// BBdialog constructor
BBdialog::BBdialog(QWidget *parent) : QDialog(parent) {
  buildUI();
  if (!setFiles()) {
    QMessageBox::critical(
        0, "Error",
        QString("Database files not found in the following directories:\n") +
            QString("/usr/share/brainregions/\n") +
            QString("/usr/local/brainregions/\n") +
            QString("$HOME/brainregions/\n") + QString("./\n") +
            QString("Application aborted.\n"));
    return;
  }

  buildList();

  ui_name->setFocus();
  connect(ui_name, SIGNAL(textEdited(const QString &)), this,
          SLOT(popupList()));
  connect(ui_hintList, SIGNAL(itemActivated(QListWidgetItem *)), this,
          SLOT(selectName(QListWidgetItem *)));
  connect(ui_namespace, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeNS()));
  connect(ui_parent, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this,
          SLOT(toParent(QListWidgetItem *)));
  connect(ui_child, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this,
          SLOT(toChild(QListWidgetItem *)));
  connect(ui_relationship, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this,
          SLOT(toRelated(QListWidgetItem *)));
}

/* Set location of bdb files. */
bool BBdialog::setFiles() {
  // database names are hard-coded for now
  rDbName = "region_name.db";
  rrDbName = "region_relation.db";
  sDbName = "synonym.db";

  // Try to get the directory in which db files are located
  QStringList dbDirs;
  dbDirs << "/usr/share/brainregions/"
         << "/usr/local/brainregions/";
  char *homedir = getenv("HOME");

  if (homedir) dbDirs.append(QString(homedir) + "/brainregions/");

  dbDirs.append("./");

  int i;
  for (i = 0; i < dbDirs.count(); i++) {
    QFileInfo fi1 = dbDirs[i] + QString::fromStdString(rDbName);
    QFileInfo fi2 = dbDirs[i] + QString::fromStdString(rrDbName);
    QFileInfo fi3 = dbDirs[i] + QString::fromStdString(sDbName);

    if (fi1.isReadable() && fi2.isReadable() && fi3.isReadable()) {
      dbHome = dbDirs[i].toStdString();
      break;
    }
  }

  if (i < dbDirs.count()) return true;

  return false;
}

// Build user interface
void BBdialog::buildUI() {
  // main layout
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(2);
  layout->setMargin(4);
  this->setLayout(layout);

  // form layout
  QFormLayout *myform = new QFormLayout();
  layout->addLayout(myform);

  // stuff in form layout
  ui_name = new QLineEdit();
  ui_hintList = new QListWidget();
  ui_hintList->hide();  // hide hint list initially
  ui_namespace = new QComboBox();
  ui_namespace->addItem("All namespaces");
  ui_namespace->addItem("NN2002");
  ui_namespace->addItem("AAL");
  ui_namespace->addItem("Brodmann");

  ui_parent = new QListWidget();
  ui_child = new QListWidget();
  ui_synonyms = new QListWidget();
  ui_source = new QLineEdit();
  ui_link = new QLineEdit();
  ui_relationship = new QListWidget();

  myform->addRow("Region Name:", ui_name);
  myform->addRow("", ui_hintList);
  myform->addRow("Located in:", ui_namespace);
  myform->addRow("Parent Structure:", ui_parent);
  myform->addRow("Child Structure(s):", ui_child);
  myform->addRow("Synonyms:", ui_synonyms);
  myform->addRow("Source:", ui_source);
  myform->addRow("Link:", ui_link);
  myform->addRow("Relationship:", ui_relationship);

  QHBox *hb = new QHBox();
  layout->addWidget(hb);
  QPushButton *resetButt = new QPushButton("Reset");
  hb->addWidget(resetButt);
  QObject::connect(resetButt, SIGNAL(clicked()), this, SLOT(resetUI()));

  QPushButton *closeButt = new QPushButton("Close");
  hb->addWidget(closeButt);
  QObject::connect(closeButt, SIGNAL(clicked()), this, SLOT(close()));

  QPushButton *aboutButt = new QPushButton("About");
  hb->addWidget(aboutButt);
  QObject::connect(aboutButt, SIGNAL(clicked()), this, SLOT(about()));

  // setMinimumSize(320,1);
  setWindowTitle("VoxBo Brain Structure Name Browser");
}

/* Add structure names into nameList and spaceList */
void BBdialog::buildList() {
  nameList.clear();
  spaceList.clear();

  string currentSpace = ui_namespace->currentText().toStdString();
  int foo;
  if (currentSpace == "All namespaces")
    foo = getAllRegions(dbHome, rDbName, nameList, spaceList);
  else
    foo = getRegions(dbHome, rDbName, currentSpace, nameList);

  if (foo) {
    QMessageBox::critical(0, "Error", "Region name db exception.");
    return;
  }

  if (currentSpace == "All namespaces")
    foo = getAllSynonyms(dbHome, sDbName, nameList, spaceList);
  else
    foo = getSynonyms(dbHome, sDbName, currentSpace, nameList);

  if (foo) {
    QMessageBox::critical(0, "Error", "Synonym db exception.");
    return;
  }
}

/* This slot replies to the name_space change signal.
 * It rebuilds search list and start a new search. */
void BBdialog::changeNS() {
  buildList();
  clearUI();
  popupList();
}

// Pop up a list of brain region names and synomnyms that match the text in
// input edit box
void BBdialog::popupList() {
  // setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  ui_hintList->clear();
  QString searchStr = ui_name->text().simplified();
  if (!searchStr.length()) {
    clearUI();
    ui_hintList->hide();
    return;
  }

  for (uint i = 0; i < nameList.size(); i++) {
    QString itemStr = nameList[i].c_str();
    if (itemStr.contains(QRegExp(searchStr, Qt::CaseInsensitive))) {
      if (ui_namespace->currentIndex() == 0)
        itemStr = itemStr + " (" + spaceList[i].c_str() + ")";
      ui_hintList->addItem(itemStr);
    }
  }

  if (!ui_hintList->count()) {
    ui_hintList->hide();
    return;
  }

  ui_hintList->setMinimumHeight(165);
  ui_hintList->sortItems(Qt::AscendingOrder);
  ui_hintList->show();
}

/* This slot selects one of the names from list box and show relevant infomation
 * on interface */
void BBdialog::selectName(QListWidgetItem *mySel) {
  QString comboStr = mySel->text();
  QString nameStr, spaceStr;
  bool withNS = true;
  if (ui_namespace->currentIndex()) withNS = false;
  int foo = parseRegionName(comboStr, withNS, nameStr, spaceStr);
  if (foo == 1) {
    QMessageBox::critical(0, "Error", "Invalid selection string: " + comboStr);
    ui_hintList->hide();
    return;
  }
  if (foo == 2) {
    QMessageBox::critical(0, "Error", "Invalid name space: " + spaceStr);
    ui_hintList->hide();
    return;
  }

  ui_name->setText(nameStr);
  ui_hintList->hide();
  searchRegion(nameStr.toStdString(), spaceStr.toStdString());
  if (regionName.length()) return;

  searchSynonym(nameStr.toStdString(), spaceStr.toStdString());
  ui_name->setFocus();
}

/* This function reads the first argument and divides it into two parts:
 * region name and namespace.
 * Returns 0 if everything is ok;
 * returns 1 if input string can't be divided successfully.
 * returns 2 if namespace is unknown. */
int BBdialog::parseRegionName(QString inputStr, bool withNS, QString &nameStr,
                              QString &spaceStr) {
  if (!withNS) {
    nameStr = inputStr;
    spaceStr = ui_namespace->currentText();
    return 0;
  }

  int strLen = inputStr.length();
  int foo = inputStr.lastIndexOf("(");
  int bar = inputStr.lastIndexOf(")");
  if (foo == -1 || foo < 2 || bar == -1 || foo >= bar || bar != strLen - 1)
    return 1;

  nameStr = inputStr.left(foo - 1);
  spaceStr = inputStr.mid(foo + 1, bar - foo - 1);
  if (!chkNameSpace(spaceStr)) return 2;

  return 0;
}

/* This function checks whether the input string is a valid namespace. If it is,
 * set the namespace combo box on the interface to correct value and returns
 * true; returns false otherwise. */
bool BBdialog::chkNameSpace(QString inputStr) {
  if (inputStr == "NN2002") return true;

  if (inputStr == "AAL") return true;

  if (inputStr == "Brodmann") return true;

  if (inputStr == "Marianna") return true;

  if (inputStr == "QT_UI") return true;

  return false;
}

/* This function checks whether the input string is a valid namespace. If it is,
 * set the namespace combo box on the interface to correct value and returns
 * true; returns false otherwise. */
void BBdialog::setNameSpace(string inputStr) {
  int nsIndex;
  if (inputStr == "NN2002")
    nsIndex = 1;
  else if (inputStr == "AAL")
    nsIndex = 2;
  else if (inputStr == "Brodmann")
    nsIndex = 3;
  else {
    QMessageBox::critical(0, "Error",
                          "Unknown namespace: " + QString(inputStr.c_str()));
    return;
  }

  disconnect(ui_namespace, SIGNAL(currentIndexChanged(int)), this,
             SLOT(changeNS()));
  ui_namespace->setCurrentIndex(nsIndex);
  name_space = inputStr;
  buildList();
  connect(ui_namespace, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeNS()));
}

/* Seach a structure name in region_name.db and show its information on the
 * interface */
void BBdialog::searchRegion(string rName, string inputSpace) {
  clearUI();

  regionRec rData;
  int foo = getRegionRec(dbHome, rDbName, rName, inputSpace, rData);
  // Return if db file exception is met
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Region name db exception");
    return;
  }

  // Return if the name is not found in region name db file
  if (foo == 0) return;

  regionID = rData.getID();
  regionName = rName;

  ui_name->setText(rName.c_str());
  setNameSpace(inputSpace);
  ui_source->setText(rData.getSource().c_str());
  ui_link->setText(rData.getLink().c_str());

  showParentChild();
  showSynonym();
  showRelation();
}

/* Seach a synonym name and show relevant information on the interface */
void BBdialog::searchSynonym(string sName, string inputSpace) {
  string pName;
  int foo = getPrimary(dbHome, sDbName, sName, inputSpace, pName);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Synonym db exception.");
    return;
  }

  // quit if the name is not found in synonym db file
  if (foo == 0) {
    QMessageBox::critical(
        0, "Error",
        "Name not found in region name and synonym db files: " +
            ui_name->text());
    return;
  }

  searchRegion(pName, inputSpace);
}

/* This functions clears all information on the interface */
void BBdialog::clearUI() {
  regionID = 0;
  regionName = name_space = "";
  ui_parent->clear();
  ui_child->clear();
  ui_synonyms->clear();
  ui_source->clear();
  ui_link->clear();
  ui_relationship->clear();
}

/* This slot responds to "reset" button click. */
void BBdialog::resetUI() {
  ui_name->clear();
  clearUI();
}

/* Show parent and child information on the interface */
void BBdialog::showParentChild() {
  long parentID = 0;
  vector<long> cList;

  int foo = getParentChild(dbHome, rrDbName, regionID, &parentID, cList);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Relationship db exception.");
    return;
  }

  // If parent exists, get its name and show on interface
  if (parentID) {
    string parentStr, spaceStr;
    foo = getRegionName(dbHome, rDbName, parentID, parentStr, spaceStr);
    if (foo == 1)
      ui_parent->addItem(parentStr.c_str());
    else
      QMessageBox::critical(0, "Error",
                            "Fails to get parent region name from ID " +
                                QString::number(parentID) + " in region db.");
  }

  for (unsigned i = 0; i < cList.size(); i++) {
    long cID = cList[i];
    string cName, spaceStr;
    foo = getRegionName(dbHome, rDbName, cID, cName, spaceStr);
    if (foo == 1)
      ui_child->addItem(cName.c_str());
    else
      QMessageBox::critical(0, "Error",
                            "Fails to get child region name from ID " +
                                QString::number(cID) + " in region name db.");
  }

  if (ui_child->count()) ui_child->sortItems(Qt::AscendingOrder);
}

/* This function updates the current parent region name. */
void BBdialog::showParent() {
  ui_parent->clear();

  long pID = 0;
  int foo = getParent(dbHome, rrDbName, regionID, &pID);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Relationship db exception.");
    return;
  }

  if (!pID) return;

  string parentStr, spaceStr;
  foo = getRegionName(dbHome, rDbName, pID, parentStr, spaceStr);
  if (foo != 1) {
    QMessageBox::critical(0, "Error",
                          "Fails to get parent region name from ID " +
                              QString::number(pID) + " in region db.");
    return;
  }
  ui_parent->addItem(parentStr.c_str());
}

/* Show parent and child information on the interface */
void BBdialog::showChild() {
  ui_child->clear();

  vector<long> cList;
  int foo = getChild(dbHome, rrDbName, regionID, cList);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Relationship db exception.");
    return;
  }

  for (unsigned i = 0; i < cList.size(); i++) {
    long cID = cList[i];
    string cName, spaceStr;
    foo = getRegionName(dbHome, rDbName, cID, cName, spaceStr);
    if (foo == 1)
      ui_child->addItem(cName.c_str());
    else
      QMessageBox::critical(0, "Error",
                            "Fails to get child region name from ID " +
                                QString::number(cID) + " in region name db.");
  }

  if (ui_child->count()) ui_child->sortItems(Qt::AscendingOrder);
}

/* This function shows synonym(s) on the interface */
void BBdialog::showSynonym() {
  ui_synonyms->clear();

  vector<string> symList;
  int foo = getSynonym(dbHome, sDbName, regionName, name_space, symList);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Synonym db exception.");
    return;
  }

  for (unsigned i = 0; i < symList.size(); i++)
    ui_synonyms->addItem(symList[i].c_str());
}

/* This function collects input region's relationship information and show on
 * the interface.
 * Note that child/parent relationships are excluded because they are already
 * shown. */
void BBdialog::showRelation() {
  ui_relationship->clear();

  vector<long> r2List;
  vector<string> relList;
  int foo = getRel_ui(dbHome, rrDbName, regionID, r2List, relList);
  if (foo < 0) {
    QMessageBox::critical(0, "Error", "Relationship db exception.");
    return;
  }

  for (unsigned i = 0; i < relList.size(); i++) {
    string r2_name, r2_space;
    foo = getRegionName(dbHome, rDbName, r2List[i], r2_name, r2_space);
    if (foo == 1) {
      string tmpStr = relList[i] + ": " + r2_name + " (" + r2_space + ")";
      ui_relationship->addItem(tmpStr.c_str());
    } else
      QMessageBox::critical(0, "Error",
                            "Fails to get region name from ID " +
                                QString::number(r2List[i]) +
                                " in region name db.");
  }
}

/* This function collects parent structure info and show on the interface */
void BBdialog::toParent(QListWidgetItem *pItem) {
  QString pStr = pItem->text();
  // Return 0 if parent name is same as the structure name ("BRAIN")
  if (regionName == pStr.toStdString()) return;

  ui_name->setText(pStr);
  searchRegion(pStr.toStdString(), name_space);
}

/* This is the slot when item in child name list is double clicked */
void BBdialog::toChild(QListWidgetItem *selItem) {
  QString selName = selItem->text();
  ui_name->setText(selName);
  searchRegion(selName.toStdString(), name_space);
}

/* This slot takes care of double click signal in relationship box. */
void BBdialog::toRelated(QListWidgetItem *selItem) {
  QString relStr = selItem->text();
  int colon_post = relStr.indexOf(": ");
  int ns_post = relStr.lastIndexOf(" (");
  if (colon_post == -1 || ns_post == -1) {
    QMessageBox::critical(0, "Error", "Relationship not understood: " + relStr);
    return;
  }

  QString newRegion = relStr.mid(colon_post + 2, ns_post - colon_post - 2);
  QString newNS = relStr.mid(ns_post + 2, relStr.length() - ns_post - 3);

  ui_name->setText(newRegion);
  searchRegion(newRegion.toStdString(), newNS.toStdString());
}

/* Show acknowledgement information. */
void BBdialog::about() {
  string helptext =
      "This initial release of the VoxBo Brain Structure Browser was written "
      "by Dongbo Hu (code) and "
      "Daniel Kimberg (sage advice).  It is distributed along with structure "
      "information derived from the NeuroNames project (Bowden and Dubach, "
      "2002) as well as the AAL atlas (Automatic Anatomical Labeling, "
      "Tzourio-Mazoyer et al., 2002).<p><p>"
      "<b>References:</b><p>"
      "Tzourio-Mazoyer N, Landeau B, Papathanassiou D, Crivello F, Etard O, "
      "Delcroix N, Mazoyer B, Joliot M (2002).  \"Automated Anatomical "
      "Labeling of activations in SPM using a Macroscopic Anatomical "
      "Parcellation of the MNI MRI single-subject brain.\"  NeuroImage 15 (1), "
      "273-89.<p><p>"
      "Bowden D and Dubach M (2003).  NeuroNames 2002.  Neuroinformatics, 1, "
      "43-59.";

  QMessageBox::about(this, tr("About Application"), tr(helptext.c_str()));
}

/* This overloaded function takes care of some key press events */
void BBdialog::keyPressEvent(QKeyEvent *kEvent) {
  if (ui_name->hasFocus() || ui_hintList->hasFocus()) {
    if (ui_hintList->isVisible() && kEvent->key() == Qt::Key_Escape) {
      ui_hintList->setFocus();
      ui_hintList->hide();
      ui_name->setFocus();
    }
  }
  if (ui_name->hasFocus() && ui_hintList->count() && ui_hintList->isVisible()) {
    if (kEvent->key() == Qt::Key_Down) {
      ui_hintList->setFocus();
      ui_hintList->setCurrentRow(0);
    } else if (kEvent->key() == Qt::Key_Up) {
      ui_hintList->setFocus();
      ui_hintList->setCurrentRow(ui_hintList->count() - 1);
    } else
      ui_hintList->show();
  } else if (ui_hintList->hasFocus() &&
             ui_hintList->currentRow() == ui_hintList->count() - 1 &&
             kEvent->key() == Qt::Key_Down)
    ui_hintList->setCurrentRow(0);
  else if (ui_hintList->hasFocus() && ui_hintList->currentRow() == 0 &&
           kEvent->key() == Qt::Key_Up)
    ui_hintList->setCurrentRow(ui_hintList->count() - 1);
  else if (ui_hintList->hasFocus() && kEvent->key() == Qt::Key_Backspace) {
    ui_name->setFocus();
    ui_name->backspace();
  } else if (ui_hintList->hasFocus() && kEvent->key() == Qt::Key_Delete) {
    ui_name->setFocus();
    ui_name->del();
  } else if (ui_hintList->hasFocus() && kEvent->key() != Qt::Key_Up &&
             kEvent->key() != Qt::Key_Down && kEvent->key() != Qt::Key_Return &&
             kEvent->key() != Qt::Key_Enter &&
             kEvent->key() != Qt::Key_PageUp &&
             kEvent->key() != Qt::Key_PageDown) {
    ui_name->setFocus();
    QString orgStr = ui_name->text();
    QString newStr = kEvent->text();
    if (newStr.length()) {
      ui_name->setText(orgStr + newStr);
      popupList();
    }
  } else if (!ui_name->hasFocus() && !ui_hintList->hasFocus())
    ui_hintList->hide();

  // kEvent->accept();
  QDialog::keyPressEvent(kEvent);
}
