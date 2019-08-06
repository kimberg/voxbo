/****************************************************************************
** Form interface generated from reading ui file 'glm_tab3.ui'
**
** Created: Thu Feb 8 15:58:07 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.4   edited Nov
*24 2003 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef GLM_TAB3_H
#define GLM_TAB3_H

#include <q3mainwindow.h>
#include <qvariant.h>
#include <QLabel>

class QSpacerItem;
class Q3ToolBar;
class QPushButton;
class Q3ListView;
class Q3ListViewItem;
class QLabel;
class Q3ButtonGroup;

class glm_tab3 : public QWidget {
  Q_OBJECT
 public:
  glm_tab3(QWidget* parent = 0, const char* name = 0);
  QPushButton* clearButt;
  QPushButton* editButt;
  QPushButton* loadButt;
  Q3ListView* covView;
  QLabel* label1;
  QLabel* status;
  QPushButton* blockButt;
  QPushButton* pairButt;
  QPushButton* interButt;

 protected:
 protected slots:
  virtual void languageChange();
};

#endif  // GLM_TAB3_H
