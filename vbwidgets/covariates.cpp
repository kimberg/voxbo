
// covariates.cpp
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
// original version written by Tom King?

#include "covariates.h"
#include <q3header.h>
#include <qmessagebox.h>

using namespace VB;
using namespace std;

/*---------------------------------------------------------------------------*/

// Covariate

Covariate::Covariate(QObject* parent, const char* name)
    : QObject(parent, name) {}

Covariate::Covariate(const Covariate& aCov)
    : QObject(), mName(aCov.mName), mInterest(aCov.mInterest) {}

Covariate::InterestType Covariate::str2type(const std::string& str) {
  // Looks at the first character in str and returns the appropriate type.

  char interest = str[0];
  switch (interest) {
    case 'I':
      return Covariate::INTEREST_I;
    case 'N':
      return Covariate::INTEREST_N;
    case 'K':
      return Covariate::INTEREST_K;
    case 'U':
      return Covariate::INTEREST_U;
    case 'D':
      return Covariate::INTEREST_D;
    default:
      return Covariate::INTEREST_ERR;
  }
}

std::string Covariate::type2str(Covariate::InterestType inter) {
  // Looks at the first character in str and returns the appropriate type.

  switch (inter) {
    case Covariate::INTEREST_I:
      return "I";
    case Covariate::INTEREST_N:
      return "N";
    case Covariate::INTEREST_K:
      return "K";
    case Covariate::INTEREST_U:
      return "U";
    case Covariate::INTEREST_D:
      return "D";
    default:
      return "error";
  }
}

void Covariate::setName(const std::string& aName) {
  mName = aName;
  emit nameChanged(aName);
}

void Covariate::setType(Covariate::InterestType aType) {
  mInterest = aType;
  emit typeChanged(aType);
}

const std::string& Covariate::getName() const { return mName; }

const Covariate::InterestType& Covariate::getType() const { return mInterest; }

Covariate& Covariate::operator=(const Covariate& aCov) {
  mName = aCov.mName;
  mInterest = aCov.mInterest;
  return *this;
}

/*---------------------------------------------------------------------------*/

// Contrast

Contrast::Contrast(std::vector<Covariate>& aCovList, QObject* parent,
                   const char* name)
    : QObject(parent, name), Covariates(aCovList) {}

Contrast::Parameter& Contrast::operator[](int aIndex) {
  return Weights[aIndex];
}

Contrast::Parameter& Contrast::operator[](std::string aCovName) {
  int index = 0;
  for (vector<Covariate>::iterator iter = Covariates.begin();
       iter != Covariates.end(); ++iter) {
    if (iter->getName() == aCovName) break;
    ++index;
  }

  // i'm not doing bounds checking.  maybe VoxBo should start throwing around
  // exceptions.  i've always liked the idea of exceptions.
  return Weights[index];
}

/********************************************************************************
 * VB::CovatiatesView class
 ********************************************************************************/

const char* CovariatesView::NAME_COL = "Name";
const char* CovariatesView::ID_COL = "ID";
const char* CovariatesView::TYPE_COL = "Type";

CovariatesView::CovariatesView(QWidget* parent, const char* name)
    : Q3ListView(parent, name) {
  setSelectionMode(Q3ListView::Extended);
  setRootIsDecorated(true);
  setupColumns();

  connect(this, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
}

void CovariatesView::setupColumns() {
  addColumn(NAME_COL);
  addColumn(TYPE_COL);
  setColumnAlignment(1, Qt::AlignHCenter);
  addColumn(ID_COL);
  setColumnAlignment(2, Qt::AlignHCenter);
  setSortColumn(-1);
}

void CovariatesView::buildTree(GLMInfo* aGLMInfo, bool aShowAll) {
  std::vector<std::string> names;
  std::vector<std::string> types;

  // Prepare and build the parameter tree
  std::vector<std::string>::iterator name_iter;
  for (name_iter = aGLMInfo->cnames.begin();
       name_iter != aGLMInfo->cnames.end(); ++name_iter) {
    names.push_back(name_iter->substr(1));
    types.push_back(name_iter->substr(0, 1));
  }
  buildTree(names, types, aShowAll);
}

void CovariatesView::buildTree(std::vector<VB::Covariate>& aCovList,
                               bool aShowAll) {
  vector<string> name_list;
  vector<string> type_list;

  string name;
  string interest;

  // Copy the covariate information into the name and interest type lists.
  vector<Covariate>::iterator cov_iter;
  for (cov_iter = aCovList.begin(); cov_iter != aCovList.end(); ++cov_iter) {
    name = cov_iter->getName();
    interest = Covariate::type2str(cov_iter->getType());

    name_list.push_back(name);
    type_list.push_back(interest);
  }
  buildTree(name_list, type_list, aShowAll);
}

void CovariatesView::buildTree(vector<string>& aNameList,
                               vector<string>& aTypeList, bool aShowAll) {
  clearSelection();

  QString nameStr, sectionStr, typeStr;
  QStringList qList;
  for (unsigned i = 0; i < aNameList.size(); i++) {
    // Get the root of this ListView.
    //    cerr << "finding the root of the listview...\n";
    Q3ListViewItem* parent = firstChild();
    //    cerr << "first child is at "<< parent << ".\n";

    // Get the name for the item...
    nameStr = QString(aNameList[i].c_str());
    //    cerr << "name for this item: " << nameStr << endl;
    // And get it's type.
    typeStr = QString(aTypeList[i].c_str());
    //    cerr << "type for this item: " << typeStr << endl;
    // Parse the hierarchy for this item from the name.
    QStringList qList = QStringList::split("->", nameStr);
    //    cerr << "...parsed the hierarchy for this file.\n";

    // Iterate through the hierarchy list...
    //    cerr << "iterating through the hierarchy...\n";
    for (int32 j = 0; j < qList.size(); j++) {
      sectionStr = *qList.at(j);
      // Covariate is a direct child of the root
      if (qList.size() == 1) {
        (void)new Q3ListViewItem(this, lastChild(), sectionStr, typeStr,
                                 QString::number(i));
        break;
      }
      // Covariate belongs to a certain group
      else if (j == qList.size() - 1) {
        (void)new Q3ListViewItem(parent, lastChild(parent), sectionStr, typeStr,
                                 QString::number(i));
        break;
      }
      // Create the covariate's first layer group item (if not available)
      else if (j == 0) {
        parent = findGroup(sectionStr);
        if (!parent) {
          parent = new Q3ListViewItem(this, lastChild(), sectionStr);
          parent->setOpen(true);
        }
      }
      // Create group items after the first layer
      else {
        // ::FIXED:: have to make a copy of the parent in this case so that,
        //           in case the subgroup does not yet exist, we still know
        //           the address of the parent and we can create it.
        //                                          Mjumbe 2007.04.26
        Q3ListViewItem* sub_parent = findGroup(parent, sectionStr);
        if (!sub_parent) {
          sub_parent =
              new Q3ListViewItem(parent, lastChild(parent), sectionStr);
          sub_parent->setOpen(true);
        }
        parent = sub_parent;
      }
    }
  }

  if (!aShowAll) showInterestOnly();

  //  cerr << "done!\n";
}

/* cpView() copies input QListView and add it on the main interface */
void CovariatesView::copyTree(const CovariatesView* aCovView, bool aShowAll) {
  clearSelection();

  // Copy the input tree, select 1st selectable covariate and set baseIndex
  Q3ListViewItemIterator it(const_cast<CovariatesView*>(aCovView));
  while (it.current()) {
    // Copy the current item
    Q3ListViewItem* newItem;
    Q3ListViewItem* inputItem = it.current();

    if (inputItem->text(2).isEmpty()) {
      if (!inputItem->childCount()) return;
      if (inputItem->depth() == 0)
        newItem = new Q3ListViewItem(this, lastChild(), inputItem->text(0));
      else
        newItem = new Q3ListViewItem(findParent(inputItem),
                                     lastChild(findParent(inputItem)),
                                     inputItem->text(0));
      newItem->setOpen(true);
      newItem->setEnabled(false);
      return;
    }

    if (inputItem->depth() == 0)
      newItem = new Q3ListViewItem(this, lastChild(), inputItem->text(0),
                                   inputItem->text(1), inputItem->text(2));
    else
      newItem = new Q3ListViewItem(
          findParent(inputItem), lastChild(findParent(inputItem)),
          inputItem->text(0), inputItem->text(1), inputItem->text(2));
    // By default, only enable type I covariates
    if (newItem->text(1) != "I") newItem->setEnabled(false);

    ++it;
  }

  Q3ListViewItemIterator all(this, Q3ListViewItemIterator::Selectable);

  if (!aShowAll) showInterestOnly();
}

/* lastChild(...) gets the last 1st-gen child of parent, or of the View if
 * parent is NULL.                                                           */
Q3ListViewItem* CovariatesView::lastChild(const Q3ListViewItem* parent) const {
  Q3ListViewItem *last_child, *temp_child;

  if (parent)
    temp_child = firstChild(parent);
  else
    temp_child = firstChild();

  for (last_child = 0; temp_child; temp_child = temp_child->nextSibling())
    last_child = temp_child;

  //  cerr << "done looking at the last child" << endl;
  return last_child;
}

/* firstChild(...) gets the first 1st-gen child of parent, or of the View if
 * parent is NULL.                                                           */
Q3ListViewItem* CovariatesView::firstChild(const Q3ListViewItem* parent) const {
  Q3ListViewItem* first_child;

  //  cerr << "  parent is at " << parent << ".\n";
  if (parent)
    first_child = parent->firstChild();
  else
    first_child = Q3ListView::firstChild();

  //  cerr << "done looking at the first child" << endl;
  return first_child;
}

/* findChild(...) finds the first item in column that matches text and is a
 * direct descendant of parent (or is at depth 0 if parent is NULL).         */
Q3ListViewItem* CovariatesView::findChild(const Q3ListViewItem* parent,
                                          const QString& text,
                                          int column) const {
  Q3ListViewItem* found_item;

  for (found_item = firstChild(parent); found_item;
       found_item = found_item->nextSibling()) {
    if (found_item->text(column) == text) break;
  }

  return found_item;
}

Q3ListViewItem* CovariatesView::findChild(const QString& text,
                                          int column) const {
  return findChild(0, text, column);
}

/* findGroup(...) finds the first item in column that matches text and is a
 * direct descendant of parent (or is at depth 0 if parent is NULL).         */
Q3ListViewItem* CovariatesView::findGroup(const Q3ListViewItem* parent,
                                          const QString& text) const {
  Q3ListViewItem* found_item;

  for (found_item = firstChild(parent); found_item;
       found_item = found_item->nextSibling()) {
    if (found_item->text(0) == text && found_item->text(2).isEmpty()) break;
  }

  return found_item;
}

/* findGroup(...) finds the first item in column that matches text and is a
 * direct descendant of parent (or is at depth 0 if parent is NULL).         */
Q3ListViewItem* CovariatesView::findGroup(const QString& text) const {
  return findGroup(0, text);
}

/* Modified based on gdw's same function */
Q3ListViewItem* CovariatesView::findParent(Q3ListViewItem* inputItem) const {
  int lastDepth = lastItem()->depth();
  int inputDepth = inputItem->depth();
  if (lastDepth < inputDepth) return lastItem();

  Q3ListViewItem* newParent = lastItem()->parent();
  while (newParent->depth() > inputDepth - 1) newParent = newParent->parent();
  return newParent;
}

void CovariatesView::showInterestOnly(bool aInterestOnly) {
  Q3ListViewItemIterator it(this);
  while (it.current()) {
    Q3ListViewItem* item = it.current();
    if (item->text(columnNumber(ID_COL)) != "") {
      if (aInterestOnly && item->text(columnNumber(TYPE_COL)) != "I")
        item->setVisible(false);
      else
        item->setVisible(true);
    }
    ++it;
  }
}

int CovariatesView::columnNumber(const QString& text) {
  for (int i = 0; i < columns(); ++i)
    if (text == columnText(i)) return i;

  return -1;
}

void CovariatesView::setColumnText(int column, const QStringList& textList) {
  int id_col = columnNumber(ID_COL);

  Q3ListViewItemIterator it(this);
  QStringList::ConstIterator string_it = textList.begin();
  while (it.current() && string_it != textList.end()) {
    Q3ListViewItem* item = it.current();
    if (!item->text(id_col).isEmpty()) {
      item->setText(column, *string_it);
      ++string_it;
    }
    ++it;
  }
}

void CovariatesView::setColumnText(int column, const QString& text) {
  int id_col = columnNumber(ID_COL);

  Q3ListViewItemIterator it(this);
  while (it.current()) {
    Q3ListViewItem* item = it.current();
    if (!item->text(id_col).isEmpty()) item->setText(column, text);
    ++it;
  }
}

void CovariatesView::setColumnText(const QString& column,
                                   const QStringList& textList) {
  setColumnText(columnNumber(column), textList);
}

void CovariatesView::setColumnText(const QString& column, const QString& text) {
  setColumnText(columnNumber(column), text);
}

void CovariatesView::setSelectedColumnText(int column, const QString& text) {
  int id_col = columnNumber(ID_COL);
  list<Q3ListViewItem*>::iterator iter;

  for (iter = mSelection.begin(); iter != mSelection.end(); ++iter) {
    if (!(*iter)->text(id_col).isEmpty()) (*iter)->setText(column, text);
  }
}

void CovariatesView::setSelectedColumnText(const QString& column,
                                           const QString& text) {
  setSelectedColumnText(columnNumber(column), text);
}

int CovariatesView::itemIndex(const Q3ListViewItem* item) {
  int index = 0;

  Q3ListViewItemIterator it(this);
  while (it.current()) {
    if (it.current() == item) return index;
    ++it;
    ++index;
  }
  return -1;
}

list<Q3ListViewItem*>& CovariatesView::selectedItems() { return mSelection; }

list<int>& CovariatesView::selectedItemIDs() { return mSelectionIDs; }

void CovariatesView::onSelectionChanged() {
  mSelection.clear();
  mSelectionIDs.clear();

  int id_col = columnNumber(ID_COL);

  Q3ListViewItemIterator it(this);
  while (it.current()) {
    Q3ListViewItem* item = it.current();
    if (isSelected(item)) {
      mSelection.push_back(item);
      if (!item->text(id_col).isEmpty())
        mSelectionIDs.push_back(item->text(id_col).toInt());
    }
    ++it;
  }
}

void CovariatesView::clear() {
  mCovariates = 0;
  mSelection.clear();
  mSelectionIDs.clear();
  return Q3ListView::clear();
}

/********************************************************************************
 * VB::ContrastsView class
 ********************************************************************************/

ContrastsView::ContrastsView(QWidget* parent, const char* name)
    : Q3ListView(parent, name) {
  setSelectionMode(Q3ListView::Single);
  setRootIsDecorated(false);
  addColumn("Name");
  addColumn("Scale Type");
  setSorting(-1);
  mCurrentContrast = NULL;  // dyk: added to avoid crash if "done" clicked with
                            // no contrast selected
  // header()->hide();

  connect(this, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));

  connect(this, SIGNAL(itemRenamed(Q3ListViewItem*, int, const QString&)), this,
          SLOT(onContrastRenamed(Q3ListViewItem*, int, const QString&)));
}

void ContrastsView::buildList(GLMInfo* aGLMInfo) {
  vector<VBContrast*> contrast_list;
  VBContrast contrast;

  vector<VBContrast>::iterator cont_iter;
  for (cont_iter = aGLMInfo->contrasts.begin();
       cont_iter != aGLMInfo->contrasts.end(); ++cont_iter) {
    contrast_list.push_back(new VBContrast(*cont_iter));
  }
  buildList(contrast_list);
}

void ContrastsView::buildList(std::vector<Contrast>& aContList) {
  vector<VBContrast*> contrast_list;
  VBContrast contrast;

  // Copy the contrast information into the name, scale, and weight lists.
  vector<Contrast>::iterator cont_iter;
  for (cont_iter = aContList.begin(); cont_iter != aContList.end();
       ++cont_iter) {
    contrast.name = cont_iter->Name;
    // contrast.scale = Contrast::scale2str(cont_iter->Scale);
    contrast.contrast = VB_Vector(cont_iter->Weights);

    contrast_list.push_back(new VBContrast(contrast));
  }
  buildList(contrast_list);
}

void ContrastsView::buildList(vector<string> aNameList,
                              vector<string> aScaleList,
                              vector<VB_Vector> aWeightList) {
  vector<VBContrast*> contrast_list;
  VBContrast contrast;

  int len = aNameList.size();  // should i check that they're all the same size?
  for (int i = 0; i < len; ++i) {
    contrast.name = aNameList[i];
    contrast.scale = aScaleList[i];
    contrast.contrast = VB_Vector(aWeightList[i]);

    contrast_list.push_back(new VBContrast(contrast));
  }
  buildList(contrast_list);
}

void ContrastsView::buildList(vector<VBContrast*>& aContrastList) {
  mContrasts = aContrastList;
  clear();

  clearSelection();

  vector<VBContrast*>::iterator cont_iter;
  for (cont_iter = aContrastList.begin(); cont_iter != aContrastList.end();
       ++cont_iter) {
    Q3ListViewItem* temp_item = new Q3ListViewItem(
        this, lastItem(), QString((*cont_iter)->name.c_str()),
        QString((*cont_iter)->scale.c_str()));
    temp_item->setRenameEnabled(0, true);
    //    cerr << "Contrast: " << (*cont_iter)->name.c_str() << endl;
  }
  //  cerr << "Contrast list has been populated." << endl;
}

void ContrastsView::insertContrast(VBContrast* aContrast) {
  mContrasts.push_back(aContrast);
  Q3ListViewItem* temp_item = new Q3ListViewItem(
      this, lastItem(), aContrast->name.c_str(), aContrast->scale.c_str());
  temp_item->setRenameEnabled(0, true);
}

void ContrastsView::takeContrast(VBContrast* aContrast) {
  Q3ListViewItemIterator list_iter(this);
  vector<VBContrast*>::iterator iter;

  for (iter = mContrasts.begin(); iter != mContrasts.end(); ++iter) {
    if (*iter == aContrast) break;
    ++list_iter;
  }
  mContrasts.erase(iter);
  takeItem(*list_iter);
}

int ContrastsView::itemIndex(const Q3ListViewItem* item) {
  int index = 0;

  Q3ListViewItemIterator it(this);
  while (it.current()) {
    if (it.current() == item) return index;
    ++it;
    ++index;
  }
  return -1;
}

VBContrast* ContrastsView::selectedContrast() const { return mCurrentContrast; }

void ContrastsView::onSelectionChanged() {
  Q3ListViewItem* item = selectedItem();
  mCurrentContrast = contrastAt(item);
}

VBContrast* ContrastsView::contrastAt(const Q3ListViewItem* item, bool diag) {
  VBContrast* contrast;
  if (!item) {
    contrast = 0;
    if (diag) cerr << "No contrast is selected." << endl;
  } else {
    int index = itemIndex(item);
    contrast = mContrasts[index];
    if (diag)
      cerr << "Contrast selected: " << contrast->name << endl
           << "  " << contrast->contrast << endl;
  }

  return contrast;
}

void ContrastsView::onContrastRenamed(Q3ListViewItem* item, int /* col */,
                                      const QString& text) {
  VBContrast* contrast = contrastAt(item);
  contrast->name = text.ascii();
}

/********************************************************************************
 * VB::ContParamsView class
 ********************************************************************************/

const char* ContParamsView::WEIGHT_COL = "Weight";

ContParamsView::ContParamsView(QWidget* parent, const char* name)
    : CovariatesView(parent, name) {
  addColumn(WEIGHT_COL);
  setColumnWidthMode(columnNumber(ID_COL), Q3ListView::Manual);
  header()->setResizeEnabled(FALSE, columnNumber(ID_COL));
  hideColumn(columnNumber(ID_COL));
}

void ContParamsView::buildTree(GLMInfo* aGLMInfo, bool aShowAll) {
  clearSelection();
  clearContrast();
  CovariatesView::buildTree(aGLMInfo, aShowAll);
}
void ContParamsView::buildTree(std::vector<Covariate>& aCovList,
                               bool aShowAll) {
  clearSelection();
  clearContrast();
  CovariatesView::buildTree(aCovList, aShowAll);
}

void ContParamsView::buildTree(std::vector<std::string>& aNameList,
                               std::vector<std::string>& aTypeList,
                               bool aShowAll) {
  clearSelection();
  clearContrast();
  CovariatesView::buildTree(aNameList, aTypeList, aShowAll);
}

void ContParamsView::setContrast(const VBContrast& aContrast) {
  setContrast(aContrast.contrast);
}

void ContParamsView::setContrast(const VB_Vector& aContrast) {
  QStringList str_list;
  for (size_t i = 0; i < aContrast.size(); ++i)
    str_list += QString::number(double(aContrast[i]), 'f', 2);

  setColumnText(WEIGHT_COL, str_list);
}

void ContParamsView::clearContrast() { setColumnText(WEIGHT_COL, QString("")); }
