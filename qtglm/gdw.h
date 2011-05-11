
// gdw.h
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

#ifndef GDW_H
#define GDW_H

#include "plotscreen.h"
#include "glmutil.h"
#include "fitOneOverF.h"
#include "gheaderinfo.h"
#include "vbprefs.h"
#include "block.h"
#include "paired.h"

#include <string>
#include <vector>
#include <qcheckbox.h>
#include <q3mainwindow.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3PopupMenu>
#include <QCloseEvent>

class QAction;
class QComboBox;
class QLabel;
class QLineEdit;
class Q3ListBox;
class Q3ListView;
class Q3ListViewItem;
class Q3PopupMenu;
class QRadioButton;
class QSlider;
class QStringList;
class Q3TextEdit;
class Q3VBox;
class tokenlist;

/***************************************************************************
 *                  Gdw class: G Design Workshop
 ***************************************************************************/
class Gdw: public Q3MainWindow
{
  Q_OBJECT

public:
  Gdw(int inputNumerOfPoints = 0, int inputTR = 2000, int inputMode = 0, QString analDir = QString::null);
  ~Gdw();
  void cpGlmView(Q3ListView *);
  void cpCovList(vector<VB_Vector *>);
  void readG(const string );
  void calcNoiseModel();
  void setTesList(Q3ListBox *);
  void setGUpdateFlag(bool);
  void cpGlmList(vector<VB_Vector *>);
  void cpTmpResolve(int);
  void cpGlmCondFxn(QString, tokenlist);
  void updateInterceptID();

  Q3ListView *varListView;

private slots:
  /* "File" slots */
  void fileOpen();
  void fileSave();
  void fileSaveAs();
  void rightClick();

  /* Edit: Add Interest */
  void add_I_Contrasts();
  void showContrasts(Q3TextEdit *, int, int, bool, VB_Vector *, double, QString &);
  void add_I_DS();
  void showDS(int, int, bool, VB_Vector*, double, QString&);

  void add_I_Single();
  void add_I_Trial();
  void add_I_varTrial();

  /* Edit: Add No interest */
  void add_N_Contrasts();
  void add_N_DS();

  void addGlobal();
  void slot_addIntercept();
  void addMovement();
  void addScanfx();
  void singleScanfx();
  void comboScanfx();
  void add_N_Single();
  void add_N_Trial();
  void add_N_varTrial();
  void addSpike();
  void addTxt();

  void selectAll();

  /* "Edit: Modify" slots */
  void duplicate();
  void delCov();
  void modC2D();
  void modConv();
  void showConv(VB_Vector *, double, QString&);
  void modMean();
  void meanNonZero();
  void modMult();
  void showMult(int);
  void modOrth();
  void showOrth(std::vector<int>);
  void modTS();
  void UnitExcurs();
  void unitVar();
  void modDeriv();
  void showDeriv(unsigned numDeriv, int varType);
  void modES();
  void modExpn();
  void showExpn(double, int);  
  void modFIR();
  void modFS();
  void showFS(int, int, int, int);

  /* "Edit: load pre-defined models" slots */
  void showBlockUI();
  void okBlock();
  void cancelBlock();

  void showPairUI();
  void okPair();
  void cancelPair();

  /* "Evaluate" slots*/
  void colAll();
  void col_I();
  void col_N();

  void a_LD();
  void i_LD();
  void nk_LD();
  void sel_LD();
  void showLD(int);

  void meanSD();
  void efficiency();
  void showEff(std::vector<int>);
  void evalNoise();

  /* "Tools" slots */
  void loadCondFunct();
  void loadCondLabel();
  void saveCondLabel();
  void setUpsampling();
  void saveCov2Ref();

  /* Slots on main interface */
  void selectionUpdate();
  void upperWindowUpdate(VB_Vector *);

  void changeTR();
  void changeNumberPoints();
  void secClicked();
  void TRClicked();
  void timePlotClicked();
  void freqPlotClicked();
  void setKeyEditor(int);
  void setKeyText();
  void nextKey();

  void dbClick(Q3ListViewItem *);
  void renameUpdate();
  void setVarType(int );

signals:
  void newCovLoaded(VB_Vector *);
  void cancelSignal(bool);
  void doneSignal(vector <VB_Vector *>, Q3ListView *, int, QString, tokenlist, bool);

private:  
  // private functions
  void init(int, int);
  void setMode(int );
  void setupMenu();
  void setStatusText();
  void setupWidgets();
  void setTR(int );
  void setTimePoints(int );
  void reInit(bool condFlag);
  void initGrpCount();

  VB_Vector * calcPS();
  bool chkPSstat();
  bool chkGSstat();
  tokenlist getMPFiles();
  int getUpsampling();

  void cpGlmItem(Q3ListViewItem *);
  Q3ListViewItem * findOrgParent(Q3ListViewItem *);

  void cpCondInfo();
  void setGname(string);
  void buildCovList(bool);
  void buildTree();
  bool cmpSampling(int);
  bool chkG(string);
  bool chkPreG(string);
  bool cmpTR(int);
  bool cmpTotalReps(int);
  Q3ListViewItem *searchDepth0(QString);
  void plotCov(unsigned );

  bool chkItemName();
  bool chkGrpName(Q3ListViewItem *);
  void singleSaveAs(QString &);

  // Generic functions for both "interest" and "no interest" 
  void addContrasts(QString varType);
  void addDS(QString, bool, bool);
  void addDS(QString);
  
  void addSingle(QString varType);
  void addTrialFx(QString varType);
  void addVarTrial(QString varType);
  void addIntercept();

  bool chkTR_imgNum();
  bool chkTR_imgNum_condfx();
  bool chkSpike(vector <int>);

  void cpItem(Q3ListViewItem *);
  void cpGrp(Q3ListViewItem *);
  void cpCov(Q3ListViewItem *);
  Q3ListViewItem *findParent(Q3ListViewItem *);
  int getNewDepth(Q3ListViewItem *);

  void delAll();
  void delPart();
  bool chkGrpSel(Q3ListViewItem *);

  void updateID();
  unsigned chkID(int);
  bool chkSel();
  bool chkD();
  void rankID();
  void updateSelID();
  void update_d_index();
  void updateMod();

  void insertDerivGrp(unsigned, int);

  void fs_set_covList(int, int, int, int);
  void fs_set_view(int, int);
  void insertCovDC(int , int, int);
  VB_Vector *fs_getFFT(VB_Vector *, VB_Vector *, int);

  void es_set_covList();
  void es_set_view();
  bool chkOrth();
  void fir_set_covList(int);
  void fir_set_view(int);

  bool chkColinear();
  void getColinear(unsigned colType);
  std::vector<int> findColID(int colType);

  bool chkLD();
  double ldDeterm(std::vector<int> ldList);
  std::vector<int> find_LD_ID(int ldType);

  bool chkElement(int, std::vector<int>);
  std::vector<int> findDiff(std::vector<int>);

  bool chkNoise();
  VB_Vector *getPSVec();

  void loadCondFunct(QString &);
  bool chkCondition(const char *);
  void writeLabFile(QString &);
  void writeCovFile(QString &);

  void selectGrp(Q3ListViewItem *);
  void singleCovUpdate(int);
  void loadSingleCov(Q3ListViewItem *, int);
  void multiCovUpdate(int);

  int  convtType(QString);
  void closeEvent(QCloseEvent *);

  bool chkIntercept();
  bool chkBlockUI();
  bool chkPairUI();
  
  // private variables
  vector < VB_Vector *> covList; 
  bool psFlag, fitFlag;
  Q3PopupMenu *fileMenu;
  enum gdw_mode {SINGLE, COMBO} mode;
  Q3Action *fileSaveAction;
  bool gUpdateFlag;                  
  int tesNum;                        
  Q3ListBox *tesList;
  Q3VBox *tab3_main;
  PlotScreen *upperWindow;
  const string selectedFiles;
  bool pregStat;
  VBMatrix gMatrix, pregMatrix;
  gHeaderInfo myGInfo;
  int totalReps, TR, tmpResolve;
  int contrast_count, diagonal_count;
  int trialfx_count, var_len_count;
  int scanfx_count, global_count;
  int mvpr_count, spike_count, txt_var_count;
  QLabel *trString, *samplingStr, *numberString; 
  VB_Vector *timeVector, *fftVector; 
  VB_Vector *noiseVec;
  VB_Vector *condVector;             

  QSlider *magSlider;
  int xMagnification;
  QRadioButton *timePlotButt, *secButt;
  int fftFlag;                        
  int secFlag;                        
  Q3ListBox *keyList;
  QLineEdit *keyEditor;
  QString varType; 
  
  int itemCounter, d_index;
  vector< int > interceptID;
  vector< int > selID;
  QComboBox *typeCombo;
  
  QString condRef;                   
  tokenlist condKey;                 
  QString gFileName;
  QString filterPath;
  QCheckBox *meanAll;

  BlockDesign *myBlock;
  int firstLen, secondLen, totalReps_block;
  int first, second;

  PairDesign *myPair;
  int totalReps_pair;
};

/*****************************************************************************
 *                       Contrast Interface 
 *****************************************************************************/
class G_Contrast: public QWidget
{
  Q_OBJECT

public:
  G_Contrast(int keyNum, int inputResolve, QWidget *parent = 0, const char *name = 0);
  ~G_Contrast();

private:
  void loadMatrix();
  void closeEvent(QCloseEvent *);
  bool chkConvolve();

  int condKeyNum, tmpResolve;
  Q3TextEdit *matrixText;
  int scaleFlag, centerFlag;

  QRadioButton *convButt;
  bool convStat;
  QComboBox *combo;
  QLabel *filter;
  QString filterFile;
  QLabel *filterName;
  QLabel *samplingLab;
  QLineEdit *samplingEditor;
  QLabel *tag;
  QLineEdit *tagEditor;
  VB_Vector *convVector;
  double samplingVal;

private slots:
  void makeDiagMat();
  void addZeroLine();
  void noScaleClicked();
  void scaleClicked();
  void offsetClicked();
  void centerClicked();

  void convClicked();
  void filterSelected(int );

  void cancelClicked();
  void doneClicked();

signals:
  void cancelSignal(bool);
  void doneSignal(Q3TextEdit *, int, int, bool, VB_Vector *, double, QString&);
};

/*****************************************************************************
 *                       Diagonal Set Interface 
 *****************************************************************************/
class G_DS: public QWidget
{
  Q_OBJECT

public:
  G_DS(int, QWidget *parent = 0, const char *name = 0);
  ~G_DS();

private:
  void closeEvent(QCloseEvent *);
  bool chkConvolve();

  int tmpResolve;
  int scaleFlag, centerFlag;

  QRadioButton *convButt;
  bool convStat;
  QComboBox *combo;
  QLabel *filter;
  QString filterFile;
  QLabel *filterName;
  QLabel *samplingLab;
  QLineEdit *samplingEditor;
  QLabel *tag;
  QLineEdit *tagEditor;
  VB_Vector *convVector;
  double samplingVal;

private slots:
  void noScaleClicked();
  void scaleClicked();
  void offsetClicked();
  void centerClicked();

  void convClicked();
  void filterSelected(int );

  void cancelClicked();
  void doneClicked();

signals:
  void cancelSignal(bool);
  void doneSignal(int, int, bool, VB_Vector *, double, QString&);
};

/*****************************************************************************
 *                       Convolution Interface 
 *****************************************************************************/
class G_Convolve: public QWidget
{
  Q_OBJECT

public:
  G_Convolve(int tmpResolve, QWidget *parent = 0, const char *name = 0);
  ~G_Convolve();

private:
  void closeEvent(QCloseEvent *);
  VB_Vector *convVector;
  QComboBox *combo;
  int tmpResolve;
  QString filterFile;
  QLabel *filterName;
  QLineEdit *samplingEditor;
  QLineEdit *tagEditor;

private slots:
  void filterSelected(int );
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(VB_Vector *, double, QString&); // This signal doesn't include deltaFlag!
};

/*****************************************************************************
 *                       Derivative Interface 
 *****************************************************************************/
class G_Deriv: public QWidget
{
  Q_OBJECT

public:
  G_Deriv(QWidget *parent = 0, const char *name = 0);
  ~G_Deriv();

private:
  void closeEvent(QCloseEvent *);
  QLineEdit *derivEdit;
  int varType;

private slots:
  void setVarType(int );
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(unsigned, int);
};

/*****************************************************************************
 *                       Orthogonalize Interface 
 *****************************************************************************/
class G_Orth: public QWidget
{
  Q_OBJECT

public:
  G_Orth(Q3ListView *inputView, QWidget *parent = 0, const char *name = 0);
  ~G_Orth();

private:
  void cpItem(Q3ListViewItem *);
  Q3ListViewItem * findParent(Q3ListViewItem *);
  void closeEvent(QCloseEvent *);

  Q3ListView *covView;
  std::vector<int> orthID;
  int covGroup;

private slots:
  void selectCov(int );
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(std::vector <int>);
};

/*****************************************************************************
 *                       Exponential Interface 
 *****************************************************************************/
class G_Expn: public QWidget
{
  Q_OBJECT

public:
  G_Expn(QWidget *parent = 0, const char *name = 0);
  ~G_Expn();

private:
  void closeEvent(QCloseEvent *);
  QLineEdit *inputBox;
  int centerFlag;

private slots:
  void setFlag(int );
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(double, int);
};

/*****************************************************************************
 *                       Multiply Interface 
 *****************************************************************************/
class G_Multiply: public QWidget
{
  Q_OBJECT

public:
  G_Multiply(Q3ListView *inputView, QWidget *parent = 0, const char *name = 0);
  ~G_Multiply();

private:
  void cpItem(Q3ListViewItem *);
  Q3ListViewItem *findParent(Q3ListViewItem *);
  void closeEvent(QCloseEvent *);
  Q3ListView *covView;

private slots:
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(int);
};

/*****************************************************************************
 *                       Fourier Set Interface 
 *****************************************************************************/
class G_Fourier: public QWidget
{
  Q_OBJECT

public:
  G_Fourier(double timeLength, QWidget *parent = 0, const char *name = 0);
  ~G_Fourier();

private:
  void closeEvent(QCloseEvent *);
  QLineEdit *periodEditor;
  QLineEdit *numberEditor;
  double totalTime;
  int addFlag;
  int deltaFlag;

private slots:
  void setAddZero(int);
  void setDelta(int); 
  void cancelClicked();
  void doneClicked(); 

signals:
  void cancelSignal(bool);
  void doneSignal(int, int, int, int);
};

/*****************************************************************************
 *                     Efficiency Evaluation Interface 
 *****************************************************************************/
class G_Eff: public QWidget
{
  Q_OBJECT

public:
  G_Eff(Q3ListView *inputView, std::vector<VB_Vector *> covList, 
	int TR, int totalReps, int tmpResolve,
	QWidget *parent = 0, const char *name = 0);
  ~G_Eff();

private:
  void init(int, int, int, int);
  void cpView(Q3ListView *);
  void cpItem(Q3ListViewItem *);
  Q3ListViewItem *findParent(Q3ListViewItem *);

  void modRawEff();
  void modView();
  void modFilterVec();
  double getRawEff(int);
  double getBold(VB_Vector *inputCov);
  double getEffBase();
  double getSquareSum(VB_Vector &);
  QString getEffStr(double);
  void closeEvent(QCloseEvent *);

  int TR, totalReps, tmpResolve;
  QComboBox *combo;
  int downFlag, typeFlag, baseIndex;
  QString filterFile;
  VB_Vector *filterVec;
  QLabel *filterLabel;
  Q3ListView *covView;
  QLineEdit *cutoffEdit;
  std::vector< VB_Vector *> covList;
  std::vector< double > rawEff;

private slots:
  void setDownFlag(int);
  void filterSelected(int);
  void selectCovType(int);
  void modEffBase(Q3ListViewItem *);
  void delClicked();
  void cancelClicked();

signals:
  void cancelSignal(bool);
  void delSignal(std::vector<int> delList);
};

/*****************************************************************************
 * Some functions called by both glm and gdw (originally written in gdw) 
 *****************************************************************************/
bool cmpG2preG(gHeaderInfo gInfo, gHeaderInfo pregInfo);
bool cmpArray(tokenlist, tokenlist);
bool cmpArray(deque<string>, deque<string>);
 
QString getFilterPath();
void cpView(Q3ListView *source, Q3ListView *target);
void cpItem(Q3ListViewItem *source, Q3ListView *target);
Q3ListViewItem * findOrgParent(Q3ListViewItem *source, Q3ListView *target);
Q3ListViewItem * findGrp(Q3ListViewItem *, QString);
Q3ListViewItem * getLastChild(Q3ListView *parentView);
Q3ListViewItem * getLastChild(Q3ListViewItem *parent);
void getInterceptID(Q3ListView *, vector<int>);

VB_Vector * fftNyquist(VB_Vector *);
int getLowFreq(VB_Vector *);
void writeG(string stemname, int, int, int, vector<VB_Vector *>, 
	    Q3ListView *, QString &, tokenlist, bool);
void getCovInfo(QStringList &nameList, QStringList &typeList, Q3ListView *);
QString getCovName(Q3ListViewItem *);


#endif
