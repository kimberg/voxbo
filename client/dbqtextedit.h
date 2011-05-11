
// dbqtextedit.h
// 
// Copyright (c) 2008-2009 by The VoxBo Development Team

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

#include <QObject>
#include <QLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QToolButton>
#include <string>
#include "mydefs.h"
#include "dbqscorebox.h"

using namespace std;

class DBQTextEdit : public DBQScoreBox {
  Q_OBJECT
public:
  DBQTextEdit(QWidget *parent=NULL);
  string originalvalue;   // last value set explicitly
public slots:
  void getValue(DBscorevalue &val);
  void setValue(const DBscorevalue &val);
  void setEditable(bool e);
  void updateAppearance();
  // handle signals
  void deleteclicked();
  void revertclicked();
  void textedited();
  void toggle_bold(bool t);
  void toggle_ital(bool t);
  void toggle_line(bool t);
  void toggle_black(bool t);
  void toggle_red(bool t);
  void toggle_green(bool t);
  void toggle_blue(bool t);
  void update_format(const QTextCharFormat &f);
private:
  QTextBrowser *ee;
  QWidget *buttonarea;
  QToolButton *button_bold,*button_ital,*button_line;
  QToolButton *button_black,*button_red,*button_green,*button_blue;
signals:
};
