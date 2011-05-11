
// dbqscorebox.h
// 
// Copyright (c) 2008-2010 by The VoxBo Development Team

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

#ifndef SCOREBOX_H
#define SCOREBOX_H

using namespace std;

#include <QObject>
#include <QFrame>
#include <QLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <string>
#include <iostream>
#include "mydefs.h"

class DBQScoreBox : public QFrame {
  Q_OBJECT
public:
  DBQScoreBox(QWidget *parent);

  bool f_dirty;                  // has the underlying data been modified?
  bool f_editable;               // is the data editable, independently of f_deleted
  bool f_set;                    // is it set to a meaningful value right now?
  bool f_originallyset;          // was it originally SetValue()'ed

  string scorename;              // pointer to the name in the global structure
  int32 scorevalueid;            // pointer to the associated value in the patient's record
  DBscorevalue newvalue;         // when the above scorevalueid is <0, the value is here

  // virtual string getValue()=0;      // get string representation of underlying data
  virtual void setEditable(bool)=0; // make the thing editable (or not)
  virtual void revertclicked()=0;   // revert this item
  virtual void setValue(const DBscorevalue &val)=0;   // get the data from scorevalue
  virtual void getValue(DBscorevalue &val)=0;         // pull the data up into scorevalue
  QHBoxLayout *layout;              // horiz layout for this widget's info and controls
  QFormLayout *childlayout;         // grid for children for widgets that have them
  QLabel *label;                    // pointer to label created externally for this widget

  string style_dirty,style_clean,style_deleted;
  string style_labeldirty,style_labelclean,style_labeldeleted;
  void updateAppearance();       // children can call this to update styles based on the flags
public slots:
protected:
  QPushButton *button_delete,*button_revert;
  QLineEdit *valueline;
private:
};

#endif  // SCOREBOX_H
