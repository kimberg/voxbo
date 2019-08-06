
#include "glm_tab3.h"

#include <q3buttongroup.h>
#include <q3header.h>
#include <q3listview.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qvariant.h>
#include "myboxes.h"

glm_tab3::glm_tab3(QWidget *parent, const char *name) : QWidget(parent, name) {
  // (void)statusBar();
  if (!name) setName("glm_tab3");
  QHBoxLayout *hb = new QHBoxLayout();
  setLayout(hb);
  QVBoxLayout *vb = new QVBoxLayout();
  hb->addLayout(vb);

  label1 = new QLabel;
  vb->addWidget(label1);

  covView = new Q3ListView(this, "covView");
  vb->addWidget(covView);
  covView->addColumn(tr("Column 1"));
  // covView->setGeometry( QRect( 10, 70, 330, 310 ) );

  status = new QLabel;
  vb->addWidget(status);

  QHBox *tmph = new QHBox;
  vb->addWidget(tmph);

  editButt = new QPushButton(this, "editButt");
  tmph->addWidget(editButt);

  loadButt = new QPushButton(this, "loadButt");
  tmph->addWidget(loadButt);

  clearButt = new QPushButton(this, "clearButt");
  tmph->addWidget(clearButt);

  QGroupBox *gb = new QGroupBox("Quick Models");
  hb->addWidget(gb);
  QVBoxLayout *vv = new QVBoxLayout();
  vv->setAlignment(Qt::AlignTop);
  gb->setLayout(vv);

  blockButt = new QPushButton(this, "blockButt");
  vv->addWidget(blockButt);
  pairButt = new QPushButton(this, "pairButt");
  vv->addWidget(pairButt);
  interButt = new QPushButton(this, "interButt");
  vv->addWidget(interButt);

  languageChange();
  // resize( QSize(518, 442).expandedTo(minimumSizeHint()) );
  // clearWState( WState_Polished );
}

void glm_tab3::languageChange() {
  clearButt->setText(tr("Clear"));
  editButt->setText(tr("Edit G"));
  loadButt->setText(tr("Load G"));
  covView->header()->setLabel(0, tr("Column 1"));
  covView->clear();
  Q3ListViewItem *item = new Q3ListViewItem(covView, 0);
  item->setText(0, tr("New Item"));

  label1->setText(tr("Current G Matrix"));
  status->setText("");
  blockButt->setText(tr("Block Design"));
  pairButt->setText(tr("Paired t-Test"));
  interButt->setText(tr("Intercept-Only"));
}
