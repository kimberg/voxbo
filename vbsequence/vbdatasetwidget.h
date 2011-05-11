#ifndef __DATASETWIDGET_H__
#define __DATASETWIDGET_H__

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "vbdataset.h"

namespace QtVB
{
  /* NOTE: This should probably be a model inherited from QAbstractItemModel
   *       instead and used with a QTreeView widget.  That way drag-and-drop
   *       info could be easilly added and whatnot.  This seemed the quickest 
   *       and easiest way for now though.
   */
  class DataSetWidget : public QTreeWidget
  {
  	Q_OBJECT
  	public:
  		DataSetWidget(QWidget * parent = 0);
  		
  		void setDataset(const VB::DataSet& ds);
  		VB::DataSet& dataset();
  		const VB::DataSet& dataset() const;
  	
  	protected:
  		void set_dataset_helper(const VB::DataSet* ds, QTreeWidgetItem* parent);
  		VB::DataSet _dataset;
  };

  enum DataSetNodeType {
  	DS_NODE,
  	DS_MEMBER
  };
}

#endif // __DATASETWIDGET_H__
