
// vbview.h
// headers for vbview
// Copyright (c) 1998-2006 by The VoxBo Development Team

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
// original version written by Dan Kimberg

#include <qapplication.h>
#include <qcursor.h>
#include <qwidget.h>
#include <qbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qframe.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <qlistview.h>
#include <qtextbrowser.h>
#include <qmime.h>
#include <qfiledialog.h>
#include <qvgroupbox.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qmainwindow.h>
#include <qimage.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qwindowsstyle.h>
#include <qsgistyle.h>
#include <qslider.h>
#include <qpainter.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qdockwindow.h>
#include <qdockarea.h>

#include <qsplitter.h>

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

// include custom widgets
#include "vbqt_canvas.h"
#include "vbqt_masker.h"
#include "vbqt_glmselect.h"
#include "vbqt_scalewidget.h"
#include "plotscreen.h"
// voxbo includes
#include "glmutil.h"
#include "vbutil.h"
#include "vbio.h"
#include "vbprefs.h"
#include "imageutils.h"

enum {vbv_axial=0,vbv_coronal=1,vbv_sagittal=2,vbv_multi=3,vbv_tri=4};

class VBV_Mask {
 public:
  char f_masktype;               // (M)ask, (R)egion, or (L)andmark
  // int f_visible;              // visible or not
  int f_opacity;                 // opacity 0-100
  QColor f_color;                // render color
  VBRegion f_region;             // region
  // Cube cube;
  string f_name;                 // name
  int f_index;                   // index value (for masks and points)
  QTMaskWidget *f_widget;        // pointer to relevant widget
};

typedef vector<VBV_Mask>::iterator MI;

class MyView {
 public:
  MyView();
  ~MyView();
  int xoff,yoff;
  int width,height;
  int position;    // x, y, or z position as needed
  int *ivals;
  float *svals;
  enum {vb_xy,vb_yz,vb_xz,vb_zy,vb_scale} orient;
};

// draw2img/mask should go through views, case on their orientation,
// find the position, and convert to coordinates

class VBView : public QWidget {
  Q_OBJECT
public:
  VBView(QWidget *parent=0,const char *name=0);
  ~VBView();
  int SetImage(string fname);
  int SetImage(Cube &cb);
  int SetImage(Tes &tes);
  int SetOverlay(string fname);
  int SetOverlay(Cube &cb);
  Tes tes;
  Cube cube;
  Cube statmap,backupmap;
  Tes q_maptes;    // tes file for generating stat map, not for direct display
  // Cube data;
  // Cube mask,savedmask;
  // QTMaskWidget **q_masktable,**savedmasktable;
  int q_maskdirty;
  int q_imagedirty;
public slots:
  void PushMask();
  void PopMask();

  void ToolChange(int);

  int LoadFile();
  //int LoadDir();
  int LoadMask();
  int SetMask(Cube &mask);
  int LoadMaskSample();
  int LoadOverlay(string fname);
  int LoadOverlay();
  int LoadGLM();

  // special menu
  void ByteSwap();
  void LoadAverages();

  void ToggleDisplayPanel();
  void ToggleMaskPanel();
  void ToggleStatPanel();
  void ToggleTSPanel();
  void ToggleHeaderPanel();

  int SavePNG();
  int ClearMask();
  int SaveMask();
  int ChangeMask();
  int SaveImage();
  int SetOrigin();
  int SaveSeparateMasks();
  int SaveCombinedMask();
  int ApplyMasks();
  int UnMask();

  int SelectMask(QTMaskWidget *);
  void UpdateMaskStatus();
  int SelectRegion(QTMaskWidget *);
  void ShowMaskInfo(QTMaskWidget *);
  void ShowMaskStat(QTMaskWidget *mw);
  void ShowRegionStats();
  int UpdateMask();
  int UpdateRegion();
  void NewMask(char masktype='M');
  int CopySlice();
  int CutSlice();
  int PasteSlice();
  int FindAllRegions();

  int ClearRegions();
  int SaveFullMap();  // FIXME
  int SaveVisibleMap();  // FIXME
  int CopyRegions();  // FIXME


  int FindCurrentRegion();
  int ShowAllMasks();
  int HideAllMasks();




  int ToggleCrosshairs();
  int ToggleOrigin();

  int HideOverlay(bool state);
  int ToggleScaling(bool state);
  void UpdateScaling();
  int TSpane(bool state);
  int Close();
  int ToggleData(bool flag);
  int NewXSlice(int newval);
  int NewYSlice(int newval);
  int NewZSlice(int newval);
  int NewVolume(int newval);
  int NewBrightness(int newval);
  int NewContrast(int newval);
  int NewAverage();
  int NewMag(const QString &str);
  int NewView(const QString &str);
  int NewRadius (int radius);
  int NewZRadius (int state);
  int MousePressEvent(QMouseEvent me);
  int MouseMoveEvent(QMouseEvent me);
  int KeyPressEvent(QKeyEvent ke);
  void UpdatePosition();
  void UpdateTS();
  void SaveTS();
  void SaveGraph();
  // void UpdateTesTS();
  void ColorPixel(int x,int y,int z);
  void FillPixel(int x,int y,int z,int fromval);
  void FillPixelXY(int x,int y,int z,int fromval);
  void FillPixelYZ(int x,int y,int z,int fromval);
  void FillPixelXZ(int x,int y,int z,int fromval);

  void RegionHandleMenu(QTMaskWidget *mm,string msg);
  
  void SetColumns(int newcols);
  void HighlightLow();
  void HighlightHigh();
  void HighlightCluster();
  void SetLow();
  void SetHigh();
  void SetCluster();
  void ReThreshold();
  void SetTails(bool twotailed);
  void LoadCorrelationVolume();
  void CreateCorrelationMap();
  void RenderAll();
  void Print();
  // TS related
  void ChangeTSType(int);

protected:

private slots:
  void newcolors(QColor,QColor,QColor,QColor);
  
signals:
  void closeme(QWidget *w);
  void renameme(QWidget *w,string label);
private:
  // MAJOR MODE
  enum {vbv_mask,vbv_erase,vbv_cross,vbv_fill,vbv_none} mode,effectivemode;

  // the panels, status line, and image names
  QVBox *panel_tools;
  QVGroupBox *panel_display;
  QLineEdit *statusline;
  QLineEdit *imagenamebox,*image2namebox,*masknamebox,*overlaynamebox;
  QVGroupBox *panel_masks;
  QVGroupBox *panel_stats;

  vector<MyView> viewlist;
  void paintEvent(QPaintEvent *);
  void SetViewSize();
  void BuildViews(int mode);
  void PreCalcView(MyView &view);
  void UpdateView(MyView &view);
  void RenderSingle(MyView &view);

  void RenderScale(MyView &view);
  void UniRender(MyView &view);
  void UniRenderMask(MyView &view,int mx,int my,int mz,QColor color);
  void RenderRegionOnAllViews(VBRegion &rr,QColor cc);


  void NewMaskWidget(VBV_Mask &mm);
  void NewRegionWidget(VBV_Mask &mm);

  void update_status();
  void reload_averages();

  void CopyInfo();
  // for rendering intensity
  void setscale();    // set scale according to current cube?  slice?
  int overlayvalue(double low,double high,double val,int &rr,int &gg,int &bb);
  double q_low,q_high,q_factor;    // used for scaling values, factor is always 255/(high-low)

  void init();
  
  QVGroupBox *helpbox;

  // MASK-RELATED STUFF
  enum {MASKMODE_DRAW,MASKMODE_ERASE} q_maskmode;
  // vector<QTMaskWidget *> maskwidgets;
  vector<VBV_Mask> q_masks;
  vector<VBV_Mask> q_savedmasks;
  VBV_Mask q_maskslice;
  int q_radius,q_usezradius;
  int q_currentmask,q_currentregion;
  QVBoxLayout *masklayout;
  QTMaskList *maskview;
  QVBoxLayout *maskbox;
  QSpinBox *radiusbox;
  QCheckBox *zradiusbox;
  int maskpos;
  int q_maskx,q_masky,q_maskz;
  void ClearMasks();
  int GetMaskValue(int x,int y,int z);
  int GetMaskIndex(int x,int y,int z);
  void ClearMaskValue(int x,int y,int z);
  void SetMaskValue(int x,int y,int z,int maskvalue);
  void SetMaskIndex(int x,int y,int z,int maskindex);
  void AddMaskIndex(int x,int y,int z,int maskindex);
  vector<string> maskheader;
  
  // draw area
  VBCanvas *drawarea;

  // min and max cube values worked out on load
  double minvalue,maxvalue;

  // image control widgets, visible and otherwise
  QCheckBox *overlaybox;
  
// -- BEG Mjumbewu 2005.11.28
  QSplitter *topsplitter;
  QSplitter *mainsplitter;
  QWidget *topbox;
  QHBoxLayout *toplayout;
  
//  QDockArea *topdockarea;
//  QDockWindow *displaydockwindow;
//  QDockWindow *masksdockwindow;
//  QDockWindow *statsdockwindow;
  
//  bool displayvisible;
//  bool masksvisible;
//  bool statsvisible;
// -- END Mjumbewu 2005.11.28
  
// -- COM Mjumbewu 2005.11.28
//  QVBoxLayout *headerlayout;
//  QHBoxLayout *toplayout;
  
  QLabel *imageposlabel,*maskposlabel,*statposlabel;
  QTMaskList *regionview;
  QSlider *xsliceslider,*ysliceslider,*zsliceslider,*volumeslider;
  QLabel *xposedit,*yposedit,*zposedit,*tposedit;
  QSlider *brightslider;
  QSlider *contrastslider;
  QCheckBox *maskcheckbox;
  QCheckBox *datacheckbox;
  QSpinBox *colbox;
  QComboBox *magbox,*viewbox,*averagebox;
  // the boxes with info about the image, etc.
  QLineEdit *lowedit,*highedit,*clusteredit;
  VBScalewidget *q_scale;
  QColor q_poscolor1,q_poscolor2,q_negcolor1,q_negcolor2;
  int regionpos;
  vector<VBV_Mask> q_regions;
  QLineEdit *imageinfo,*maskinfobox;
  QLineEdit *q_dstatus,*q_mstatus,*q_sstatus;
  QLineEdit *q_mstatus2,*q_sstatus2;
  QHBox *xslicebox,*yslicebox,*zslicebox,*volumebox;
  QCheckBox *infobox;
  QTextEdit *panel_header;
  QRgb q_bgcolor;
  
  // time series browser
  PlotScreen *tspane;
  QListBox *tslist;
  QHBox *panel_ts;
  QCheckBox *ts_meanscalebox;
  QCheckBox *ts_detrendbox;
  QCheckBox *ts_filterbox;
  QCheckBox *ts_removebox;
  QCheckBox *ts_scalebox;
  QCheckBox *ts_powerbox;
  QCheckBox *ts_pcabox;
  QRadioButton *ts_mousebutton;
  QRadioButton *ts_maskbutton;
  QRadioButton *ts_crossbutton;
  QListBox *ts_averagetype;
  enum {tsmode_mouse=0,tsmode_mask=1,tsmode_cross=2,tsmode_supra=3};
  int q_tstype;
  
  // coordinate conversions
  int Draw2Stat(int &x,int &y,int &z);
  int Draw2Mask(int &x,int &y,int &z);
  int Draw2Img(int &x,int &y,int &z);
  void img2coords(int x,int y,int z,int &mx,int &my,int &mz,int &sx,int &sy,int &sz);
  MyView *q_currentview;     // view identified by draw2img or draw2mask

  QScrollView *sview;
  QImage currentimage;

  GLMInfo glmi;
  string averagesfile;
  vector<TASpec> trialsets;
  TASpec *myaverage;

  // parameters
  int q_mapmode;
  double q_lowthresh,q_highthresh;
  int q_clustersize;
  string q_filename;
  string q_currentdir;
  int q_viewmode;
  int q_twotailed;
  int q_columns;          // columns for multi-view
  int q_crosshairs;
  int q_hideoverlay;
  int q_showorigin;
  int q_showts;
  int q_xviewsize;
  int q_yviewsize;
  int q_xslice,q_yslice,q_zslice;
  int q_volume;
  int q_mag;
  float q_mxoffset;     // offset of mask from start of image in mm
  float q_myoffset;
  float q_mzoffset;
  float q_sxoffset;     // offset of statmap from start of image in mm
  float q_syoffset;
  float q_szoffset;
  int q_isotropic;
  double q_xscale;       // scale factor (2.0 means stretched)
  double q_yscale;
  double q_zscale;
  int q_maskxratio;
  int q_maskyratio;
  int q_maskzratio;
  int q_statxratio;
  int q_statyratio;
  int q_statzratio;
  int q_maskflag;
  int q_dataflag;
  int q_brightness;
  int q_contrast;
  int q_update;
  int q_ix,q_iy,q_iz,q_mx,q_my,q_mz;   // last known coordinates
};

class VBViewMain : public QVBox {
  Q_OBJECT
public:
  VBViewMain(QWidget *parent=0,const char *name=0);
  VBView *AddView();
public slots:
  // latest slots
  void NewPane();
  void SaveImage();
  void Close();
  void OpenFile();
  void OpenDirectory();
  void Help();
  void Quit();
  void SavePNG();
  void Print();
  void SaveMask();
  void SaveSeparateMasks();
  void Undo();
  void ClearMask();
  void CopySlice();
  void CutSlice();
  void PasteSlice();
  void ViewAxial();
  void ViewCoronal();
  void ViewSagittal();
  void ViewMulti();
  void ViewTri();
  void ToggleDisplayPanel();
  void ToggleMaskPanel();
  void ToggleStatPanel();
  void ToggleTSPanel();
  void ToggleHeaderPanel();
  void LoadOverlay();
  void LoadGLM();
  void CorrelationMap();
  void LoadMask();
  void LoadMaskSample();
  void AddSNRView();
  void ByteSwap();
  void LoadAverages();
  void About();
  
  // int Separate();
  int ClosePanel(QWidget *w);
  int RenamePanel(QWidget *w,string label);
  void keyPressEvent(QKeyEvent *ke);
protected:
private slots:
private:
QTabWidget *imagetab;
};
