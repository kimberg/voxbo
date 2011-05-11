XOXOXO DONT COMPILE ME
// dbqbool.h
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
#include <QLineEdit>
#include <QComboBox>
#include <QPainter>
#include <iostream>
#include <string>
#include "mydefs.h"
#include "dbqscorebox.h"

using namespace std;

class DBQBool : public DBQScoreBox {
  Q_OBJECT
public:
  DBQBool(QWidget *parent=NULL);
  string originalvalue;
  string getValue();
  void updateAppearance();
  void allowOther();
public slots:
  void getValue(DBscorevalue &val);
  void setValue(const DBscorevalue &val);
  void setEditable(bool c);
  // handle signals
  void newclicked();
  void revertclicked();
  void deleteclicked();
  void changed(int s);
private:
  QComboBox *cb;
  QPushButton *button_new,*button_delete,*button_revert;
  int otherindex;
signals:
};
