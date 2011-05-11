
// dbqtimedate.h
// 
// Copyright (c) 2008 by The VoxBo Development Team

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
// original version written by Dan Kimberg

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTimeEdit>
// #include <QComboBox>
#include <QLineEdit>
#include <iostream>
#include <string>
#include "mydefs.h"
#include "vbutil.h"
#include "tokenlist.h"
#include "boost/format.hpp"
#include "dbqscorebox.h"

using namespace std;

class DBQTimeDate : public DBQScoreBox {
  Q_OBJECT
public:
  DBQTimeDate(QWidget *parent=NULL);
  int16 o_year,o_month,o_day,o_hour,o_minute,o_second;
  void updateAppearance();
public slots:
  void getValue(DBscorevalue &val);
  void setValue(const DBscorevalue &val);
  void setTime(uint16 hours,uint16 minutes,uint16 seconds);
  void setDate(uint16 month,uint16 day,uint16 year);
  void setFormat(bool time,bool date);
  void setEditable(bool e);
  void changed();
  // handle signals
  void deleteclicked();
  void revertclicked();
  // void textedited(const QString &qs);
private:
  bool f_date,f_time;
  QDateTimeEdit *dt;
  void setvalueline();
signals:
};
