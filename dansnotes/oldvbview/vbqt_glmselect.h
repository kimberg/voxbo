
#include <qwidget.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <iostream>
#include "glmutil.h"
#include "vbutil.h"
#include <string>

class VBQT_GLMSelect : public QDialog {
  Q_OBJECT
public:
  VBQT_GLMSelect(QWidget *parent,const char *name,bool modal=TRUE);
  GLMInfo glmi;
  // StatSpec statspec;
public slots:
  // void tbtoggled(bool val);
  void newcontrast();
  void getnewstem();
  void newcontrastline(const QString &text);
  QString getcontrast();
signals:
  void changed();
  void selected(VBQT_GLMSelect *);
private:
  string stemname;
  QListBox *scalebox;
  QListBox *contrastbox;
  QLineEdit *contrastline;
  QCheckBox *tailbox;
  QLineEdit *stemlabel;
};
