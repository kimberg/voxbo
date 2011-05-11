
// glm.h
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
// original version written by Dongbo Hu

#ifndef GLM_H
#define GLM_H

#include "glm_tab3.h"
#include "gdw.h"
#include <string>
#include <vector>

#include <qdir.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QCloseEvent>
#include <QLabel>

class QCheckBox;
class Q3FileDialog; 
class Q3HBox;
class Q3HBoxLayout;
class QPushButton;

class glm : public QWidget
{
  Q_OBJECT

public:
  glm( QWidget *parent, const char *name );

private slots:
  void tab1_browse();
  void tab1_pathChanged(); 
  void enableTab2();

  void tab2Browse();
  void tab2_listChanged();
  void lb1_select(int );
  void lb2_select(int );
  void tab2_slotLeft2Right();
  void tab2_slotLeft2Right_all();
  void tab2_slotRight2Left();
  void tab2_slotRight2Left_all();
  void enableTab3();

  void tab3_edit();
  void tab3_load();
  void tab3_clear();
  void tab3_model(int);
  void tab3_blockUI();
  void tab3_pairUI();
  void tab3_inter();

  void okBlock();
  void cancelBlock();
  void okPair();
  void cancelPair();
  void gdwDone(vector <VB_Vector *>, Q3ListView *, int, QString, tokenlist, bool);
  void cpGdwList(vector <VB_Vector *>);

  void convertBelow();
  void convertAbove();
  void smoothKernSelected(int);
  void noiseModSelected(int);
  void oneOverFClicked();

  void tabChange();
  void submitAll();
  void clickNext();

private:
  void setupTab1();
  void setupTab2();
  void setupTab3();
  void setupTab4();
  void setFilterHint();
  void setupButtons();

  void closeEvent(QCloseEvent *);
  void setElementPath();
  QString getFilterFile(int );
  QString getNoiseFile(int );

  bool chkTab1();
  bool chkAnalysisDIR();

  bool chkTesFile(QString);
  int getTesTR(QString);
  void updateTR();
  int getTesImgNo(QString);
  int tesChanged();

  void setupTab3_view();
  void tab3_readG(string);
  void buildCovList(bool);
  void buildTree();
  Q3ListViewItem * searchDepth0(QString grpName);
  bool chkG(string inputName);  
  bool cmpTR(int headerTR);
  bool cmpTotalReps(int rowNum);
  bool chkPreG(string inputName);
  void clearG();
  
  bool chkBlockUI();
  bool chkPairUI();
  void addIntercept();
  void updateInterceptID();

  bool chkSubmit();

  QTabWidget *tabSection;
  Q3HBoxLayout *buttons;

  Q3VBox *tab1;
  QString pathText, fileName, gFileName;
  QLineEdit *pathEditor;
  QFileInfo folderInfo;
  QLabel *finalName;
  QLineEdit *seqEditor;
  QComboBox *priCombo;
  QCheckBox *auditCheck, *emailCheck, *pieceCheck;
  QLineEdit *emailEditor, *pieceEditor;

  Q3VBox *tab2;
  QString tab2_dirName;
  QLineEdit *dirEditor, *tab2Filter;
  QString tab2_filterText;
  Q3ListBox *tab2_lb1;
  Q3ListBox *tab2_lb2;
  int totalReps, TR;
  bool endFlag;

  glm_tab3 *tab3;
  vector< int > interceptID;
  gHeaderInfo myGInfo;  
  bool meanAll, pregStat;
  VBMatrix gMat, pregMat;
  vector< VB_Vector *> covList;
  QString condRef;
  tokenlist condKey;
  int tab3_TR, tmpResolve;
  int g_TR, g_totalReps;
  bool gUpdate;

  BlockDesign *tab3_block;
  int firstLen, secondLen;
  int first, second;
  PairDesign *tab3_pair;
  
  Q3VBox *tab4;
  QLabel *filter_hint;
  QLineEdit *belowEdit, *aboveEdit;
  QLabel *belowLab, *aboveLab;
  QLabel *smoothKernel;
  QLineEdit *kernelTR;
  QComboBox *combo1, *combo2;
  QString elementPath, filterFile;
  unsigned kTR;
  QCheckBox *meanCheck, *driftCheck; 
  QLabel *noiseModel;
  QString noiseFile;

  QPushButton *mainSubmitButton, *mainNextButton;
};

#endif
