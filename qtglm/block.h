/****************************************************************************
** Form interface generated from reading ui file 'block.ui'
**
** Created: Tue Jan 23 11:56:53 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.4   edited Nov
*24 2003 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef BLOCKDESIGN_H
#define BLOCKDESIGN_H

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
class QPushButton;
class QLabel;
class QLineEdit;
class Q3ButtonGroup;
class QRadioButton;

class BlockDesign : public Q3MainWindow {
  Q_OBJECT

 public:
  BlockDesign(QWidget* parent = 0, const char* name = 0,
              Qt::WFlags fl = Qt::WType_TopLevel);
  ~BlockDesign();

  QPushButton* cancelButton;
  QLabel* textLabel5;
  QLineEdit* offEditor;
  Q3ButtonGroup* orderGroup;
  QRadioButton* onFirst;
  QRadioButton* offFirst;
  QLabel* textLabel1;
  Q3ButtonGroup* unitGroup;
  QRadioButton* ms;
  QRadioButton* TR;
  QLineEdit* numberEditor;
  QPushButton* okButton;
  QLabel* textLabel3;
  QLabel* textLabel4;
  QLabel* textLabel2;
  QLineEdit* nameEditor;
  QLineEdit* onEditor;

 protected:
 protected slots:
  virtual void languageChange();
};

#endif  // BLOCKDESIGN_H
