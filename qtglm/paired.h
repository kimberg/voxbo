/****************************************************************************
** Form interface generated from reading ui file 'paired.ui'
**
** Created: Tue Jan 23 10:57:45 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.4   edited Nov
*24 2003 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef PAIRDESIGN_H
#define PAIRDESIGN_H

#include <q3mainwindow.h>
#include <qvariant.h>
// Added by qt3to4:
#include <Q3ActionGroup>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <Q3PopupMenu>
#include <Q3VBoxLayout>
#include <QLabel>

class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QSpacerItem;
class QAction;
class Q3ActionGroup;
class Q3ToolBar;
class Q3PopupMenu;
class QLabel;
class QLineEdit;
class Q3ButtonGroup;
class QRadioButton;
class QPushButton;

class PairDesign : public Q3MainWindow {
  Q_OBJECT

 public:
  PairDesign(QWidget* parent = 0, const char* name = 0,
             Qt::WFlags fl = Qt::WType_TopLevel);
  ~PairDesign();

  QLabel* textLabel2;
  QLineEdit* numberEditor;
  Q3ButtonGroup* orderGroup;
  QRadioButton* subject;
  QRadioButton* group;
  QLabel* textLabel1;
  QLabel* textLabel5;
  QLineEdit* nameEditor;
  QPushButton* cancelButton;
  QPushButton* okButton;

 protected:
 protected slots:
  virtual void languageChange();
};

#endif  // PAIRDESIGN_H
