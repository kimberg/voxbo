
#include "vbdatasetwidget.h"
#include "vbutil.h"

#include <QStringList>

using namespace VB;
using namespace QtVB;

DataSetWidget::DataSetWidget(QWidget* parent) : QTreeWidget(parent) {
  setColumnCount(2);
  QStringList labels;
  labels.append("Name");
  labels.append("Value");
  setHeaderLabels(labels);
}

void DataSetWidget::setDataset(const DataSet& ds) {
  clear();
  _dataset = ds;
  set_dataset_helper(&ds, 0);
}

void DataSetWidget::set_dataset_helper(const DataSet* this_ds,
                                       QTreeWidgetItem* parent) {
  QTreeWidgetItem* this_ds_item;
  if (parent)
    this_ds_item = new QTreeWidgetItem(
        parent, QStringList(QString::fromStdString(this_ds->get_name())),
        DS_NODE);
  else
    this_ds_item = new QTreeWidgetItem(
        this, QStringList(QString::fromStdString(this_ds->get_name())),
        DS_NODE);

  // Get the members of the dataset node only--do not recurse to parents.
  std::list<DataSet::Member*> members = this_ds->get_members(false);
  vbforeach(const DataSet::Member* mem, members) {
    QStringList sl;
    sl.append(QString::fromStdString(mem->name));
    sl.append(QString::fromStdString(mem->value));
    new QTreeWidgetItem(this_ds_item, sl, DS_MEMBER);
  }

  vbforeach(DataSet * ds, this_ds->get_children()) {
    if (ds) set_dataset_helper(ds, this_ds_item);
    //		else
    //			cerr << "bad child dataset!" << endl;
  }
}

const DataSet& DataSetWidget::dataset() const { return _dataset; }

DataSet& DataSetWidget::dataset() { return _dataset; }
