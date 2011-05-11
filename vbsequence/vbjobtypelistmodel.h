#ifndef SEQUENCEMODEL_H
#define SEQUENCEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVector>
#include <QPixmap>
#include <QPoint>
#include <QStringList>

#include "vbexecdef.h"

class QMimeData;

namespace QtVB
{

  class JobTypeListModel : public QAbstractListModel
  {
      Q_OBJECT

    public:
      JobTypeListModel(QObject *parent = 0);

      QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
      QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
      Qt::ItemFlags flags(const QModelIndex &index) const;
      bool removeRows(int row, int count, const QModelIndex &parent);
      int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

      bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                        int row, int column, const QModelIndex &parent);
      QMimeData *mimeData(const QModelIndexList &indexes) const;
      QStringList mimeTypes() const;
      int rowCount(const QModelIndex &parent) const;
      Qt::DropActions supportedDropActions() const;

      bool insertAt(int index, VB::JobType *job, const QPixmap& pixmap);
      bool removeAt(int index);

    private:
      QVector<VB::JobType*> jobs;
      QVector<QPixmap> pixmaps;
      
  };

}
#endif // SEQUENCEMODEL_H
