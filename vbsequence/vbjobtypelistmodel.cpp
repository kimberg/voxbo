
// vbjobtypelistmodel.cpp
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
// original version written by Mjumbe Poe

#include <QtGui>
#include <QAbstractListModel>

#include <iostream>

#include "vbjobtypelistmodel.h"

using namespace QtVB;
using namespace VB;
using namespace std;

JobTypeListModel::JobTypeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant JobTypeListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DecorationRole && index.column() == 0)
    return QIcon(pixmaps.value(index.row()).scaled(60, 60,
                 Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
  /*else*/ if (role == Qt::DisplayRole)
  {
    JobType* jt = jobs[index.row()];
    
    QString data_names = "";
    switch (index.column())
    {
      case 0:
        return QString::fromStdString(jt->name());
        
      case 1:
        return QString::fromStdString(jt->description());
        
      case 2:
        vbforeach (const JobType::Argument& dd, jt->args())
        {
          data_names += QString::fromStdString(dd.name + ", ");
        }
        return data_names;
        
/*      case 3:
        vbforeach (const JobType::Data& dd, jt->resources)
        {
          data_names += QString::fromStdString(dd.name + ", ");
        }
        return data_names;
  */      
      default:
        break;
    }
  }

//  else if (role == Qt::UserRole + 1)
//    return locations.value(index.row());

    return QVariant();
}

QVariant JobTypeListModel::headerData ( int section, Qt::Orientation orientation, int  ) const
{
  if (orientation == Qt::Horizontal)
  {
    switch (section)
    {
      case 0:
        return QString("Type Name");
        
      case 1:
        return QString("Description");
        
      case 2:
        return QString("Argument Names");
        
/*      case 3:
        return QString("Resource Names");
  */      
      default:
        break;
    }
  }
  return QVariant();
}

int JobTypeListModel::columnCount ( const QModelIndex & ) const
{
  return 4;
}

bool JobTypeListModel::insertAt(int index, JobType *job, const QPixmap& pixmap)
{
  beginInsertRows(QModelIndex(), index, index);
  if (index <= jobs.size())
  {
    jobs.insert(index, job);
    pixmaps.insert(index, pixmap);

    endInsertRows();
    return true;
  }
  
  endInsertRows();
  return false;
}

bool JobTypeListModel::removeAt(int index)
{
  return removeRows(index, 1, QModelIndex());
}

Qt::ItemFlags JobTypeListModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable
              | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    }

    return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
}

bool JobTypeListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    if (row >= jobs.size() || row + count <= 0)
        return false;

    int beginRow = qMax(0, row);
    int endRow = qMin(row + count - 1, jobs.size() - 1);

    beginRemoveRows(parent, beginRow, endRow);

    while (beginRow <= endRow) {
        jobs.remove(beginRow);
        pixmaps.remove(beginRow);
        ++beginRow;
    }

    endRemoveRows();
    return true;
}

QStringList JobTypeListModel::mimeTypes() const
{
    QStringList types;
    types << "image/x-voxbo-jobtype";
    return types;
}

QMimeData *JobTypeListModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  vbforeach (QModelIndex index, indexes) {
    if (index.isValid()) {
      int row = index.row();
      // Since a QDataStream doesn't know how to write or read a JobType*, it's 
      // easier to just make the pointer an int (or a long?) and convert it
      // before/after the write/read.
      void * pointer_val = (void *)jobs[row];
      stream << pixmaps[row] << pointer_val;
    }
  }

  mimeData->setData("image/x-voxbo-jobtype", encodedData);
  return mimeData;
}

bool JobTypeListModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                               int row, int column, const QModelIndex &parent)
{
    if (!data->hasFormat("image/x-voxbo-jobtype"))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    int endRow;

    if (!parent.isValid() && row < 0)
        endRow = jobs.size();
    else if (!parent.isValid())
        endRow = qMin(row, jobs.size());
    else
        endRow = parent.row();

    QByteArray encodedData = data->data("image/x-voxbo-jobtype");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        QPixmap pixmap;
        JobType* job_p;
        int pointer_val;
        stream >> pixmap >> pointer_val;
        job_p = (JobType*)pointer_val;

        insertAt(endRow, job_p, pixmap);

        ++endRow;
    }

    return true;
}

int JobTypeListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else
        return jobs.size();
}

Qt::DropActions JobTypeListModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}
