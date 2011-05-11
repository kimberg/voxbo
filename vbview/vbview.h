
// vbview.h
// headers for vbview
// Copyright (c) 1998-2009 by The VoxBo Development Team

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

#include <QApplication>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QListWidget>
#include <QFileDialog>
#include <QStackedWidget>

#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QColorGroup>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qimage.h>
#include <QMessageBox>
#include <QSpinBox>
#include <qwindowsstyle.h>
#include <qslider.h>
#include <QLabel>
#include <QEvent>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

// custom widgets
#include "myboxes.h"
#include "vbqt_canvas.h"
#include "vbqt_masker.h"
#include "vbqt_glmselect.h"
#include "vbqt_scalewidget.h"
#include "vbview_widgets.h"
#include "plotscreen.h"

// voxbo includes
#include "glmutil.h"
#include "vbutil.h"
#include "vbio.h"
#include "vbprefs.h"
#include "imageutils.h"

enum {vbv_axial=0,vbv_coronal=1,vbv_sagittal=2,vbv_multi=3,vbv_tri=4};

class VBV_Mask;
typedef vector<VBV_Mask>::iterator MI;
class VBLayer;
typedef list<VBLayer>::iterator VBLayerI;

class VBV_Mask {
 public:
  char f_masktype;               // (M)ask, (R)egion, or (L)andmark
  int f_opacity;                 // opacity 0-100
  QColor f_color;                // render color
  VBRegion f_region;             // region
  string f_name;                 // name
  int f_index;                   // index value (for masks and points)
  QTMaskWidget *f_widget;        // pointer to relevant widget
};

class VBLayer {
public:
  VBLayer();
  Tes tes;
  Cube cube,mask,rendercube;
  GLMInfo glmi;
  enum {vb_stat,vb_mask,vb_struct,vb_corr,vb_glm} type;
  int q_dirty;
  deque<VBRegion> undo;
  string filename;
  int32 dimx,dimy,dimz,dimt;
  int32 x,y,z;  
  int q_brightness,q_contrast;
  float q_factor;            // a precalculated factor for intensity scaling
  // for thresholding and colorizing:
  float q_thresh;            // must be exceeded
  float q_high;              // advisory value used for color scaling
  bool q_flip;               // flip sign
  bool q_twotailed;          // if false, we only show positive values (after flipping)
  int alpha;                 // opacity
  bool q_visible;            // is the layer currently visible?
  bool q_ns;                 // should we render non-significant voxels?
  threshold threshdata;      // data for tcalc widget
  string threshtt;           // tooltip string for this layer's threshold

  // stuff for stat map layers only
  int q_clustersize;
  QColor q_poscolor1,q_poscolor2,q_negcolor1,q_negcolor2,q_nscolor1,q_nscolor2;

  // transform converts base coordinates to our layer's coordinates.
  // full converts screen coordinates to our layer's coordinates.
  // constructor initializes both to 4x4
  VBMatrix transform;     // transform * base layer = our layer coord
  VBMatrix full;          // full * screen coord = our layer coord
  void setAlignment(VBLayerI base);
  VBMatrix calcFullTransform(float xscale,float yscale,float zscale,
                             int xoff,int yoff,int vwidth,int vheight,
                             int orient,float position,
                             bool q_fliph,bool q_flipv);
  // layers render themselves to the rendercube, in their native
  // resolutions, in one of several ways:
  void render();
  void renderStruct();
  void renderMask();
  void renderStat();
  void renderCorr();
  void setScale();
  uint32 overlayvalue(double val,double aval);
  QString tooltipinfo();
};

class MyView {
 public:
  MyView();
  ~MyView();
  void setViewSize(list<VBLayer> &layers,int q_xscale,int q_yscale,int q_zscale);
  int xoff,yoff;
  int width,height;
  int position;    // x, y, or z position as needed
  enum {vb_xy,vb_yz,vb_xz,vb_zy} orient;
  float xscale,yscale,zscale;
  // new methods and stuff
};

// draw2img/mask should go through views, case on their orientation,
// find the position, and convert to coordinates

class VBView : public QWidget {
  Q_OBJECT
public:
  VBView(QWidget *parent=0,const char *name=0);
  ~VBView();

  // info from the base layer
  int base_dimx,base_dimy,base_dimz,base_dimt;
  string base_orientation;
  float base_origin[3];
  int base_dims;  // FIXME set to 0, 3, or 4
  // methods to retrieve various layers of interest:
  VBLayerI baseLayer();
  VBLayerI firstMaskLayer();
  VBLayerI firstStatLayer();
  VBLayerI firstCorrLayer();

  void setMinimal(bool b);
  list<VBLayer> layers;

  int q_xslice,q_yslice,q_zslice;
public slots:
  void layertableclicked(int row,int column);
  void layertabledclicked(int row,int column);
  void layertablerightclicked(QPoint);
  void deletelayer();
  void moveLayer(string dir);
  void fliplayer();

  void PopMask();

  void ToolChange(int);

  // load and replace layers
  int LoadImage(string fname="");
  int SetImage(Cube &cb);
  int SetImage(list<VBLayer> &newlayers);
  int LoadMask(string fname="");
  int NewMask();
  int SetMask(Cube &mask);
  int SetMask(list<VBLayer> &newlayers);
  int LoadStat(string fname="");
  int LoadCorr(string fname="");
  // int LoadAux(string fname="");
  void HandleCorr();

  int LoadGLM(string fname="");

  // special menu
  void NewOutline();
  void ByteSwap();
  void LoadAverages();
  void GoToOrigin();

  void LayerInfo();

  void ToggleDisplayPanel();
  void ToggleTSPanel();
  void ToggleOverlays();
  void ToggleHeaderPanel();

  int SavePNG();
  int ClearMask();
  int ChangeMask();
  int SaveImage();
  int ApplyMask();      // applies the most recent mask

  // NEW MASKVIEW EVENT HANDLERS
  void handle_savemask();
  void handle_newmask(QTMaskWidget *mw);
  void handle_maskchanged(QWidget *o);
  void handle_maskselected(int i);
  void handle_maskcopied(int i);
  void handle_inforequested(int i);

  void ShowRegionStats();
  int Copy();
  int Cut();
  int Paste();
  int FindAllRegions();

  int ClearRegions();
  int SaveFullMap();
  int SaveVisibleMap();
  int SaveRegionMask();
  // int CopyRegions();  // FIXME

  int FindCurrentRegion();

  // coordinate trasnform utils
  int win2base(int x,int y,double &xx,double &yy,double &zz);
  // setAllLayerCoords() should actually do everything we need to do
  // when our coord changes.  should rename
  void setAllLayerCoords(double x,double y,double z);

  int ShowCrosshairs(bool state);
  int SetDrawmask(bool state);
  int FlipH(bool state);
  int FlipV(bool state);
  void NewCoordSystem(int ind);
  int ShowOrigin(bool state);
  int ToggleScaling(bool state);
  void UpdateScaling();
  int Close();
  int ToggleData(bool flag);
  int NewXSlice(int newval);
  int NewYSlice(int newval);
  int NewZSlice(int newval);
  int NewTSlice(int newval);
  int NewBrightness(int newval);
  int NewContrast(int newval);
  int NewAverage();
  int NewMag(const QString &str);
  int NewView(const QString &str);
  int NewRadius (int radius);
  int NewZRadius (int state);
  int MousePressEvent(QMouseEvent me);
  int MouseMoveEvent(QMouseEvent me);
  int LeftCanvas();
  void keyPressEvent(QKeyEvent *ke);
  void wheelEvent(QWheelEvent *we);
  void UpdateTS(bool clearflag=0);
  void SaveTS();
  void SaveGraph();
  // void UpdateTesTS();
  void FillPixel(int x,int y,int z,int fromval);
  void FillPixelXY(int x,int y,int z,int fromval);
  void FillPixelYZ(int x,int y,int z,int fromval);
  void FillPixelXZ(int x,int y,int z,int fromval);

  void SetColumns(int newcols);
  void SetEveryn(int n);
  void HighlightLow();
  void HighlightHigh();
  void HighlightCluster();
  void SetLow();
  void SetHigh();
  void SetCluster();
  void GetThreshold();
  void SetTails(bool twotailed);
  void togglens(bool ns);
  void SetFlip(bool flipped);
  void CreateCorrelationMap();
  void RenderAll();
  void Print();
  // TS related
  void ChangeTSType(int);
protected:

private slots:
  void newcolors2(QColor,QColor,QColor,QColor);
  void newcolors(QColor,QColor,QColor,QColor);
  void newtrans(int);
  
signals:
  void closeme(QWidget *w);
  void renameme(QWidget *w,string label);
private:
  // mask-related
  VBLayerI currentlayer,previouslayer;
  VBLayerI lastmasklayer;

  // MAJOR MODE
  enum {vbv_mask,vbv_erase,vbv_cross,vbv_fill,vbv_none} mode,effectivemode;
  vector<MyView> viewlist;
  void paintEvent(QPaintEvent *);
  void SetViewSize();
  void BuildViews(int mode);
  void UpdateView(MyView &view);
  void RenderSingle(MyView &view);
  void RenderScale(MyView &view);
  void UniRender(MyView &view);
  void UniRenderView(MyView &view);
  void UniRenderLayer(MyView &view,VBLayerI layer,vbrect rrect);
  void UniRender(vbrect rrect);
  int32 fillmask(MyView &view,VBVoxel loc,VBLayerI masklayer,uint32 maskcolor);
  int32 colormask(MyView &view,VBVoxel loc,VBLayerI masklayer,uint32 maskcolor);
  int32 colorpixels(MyView &view,VBVoxel cvox,VBRegion &maskreg,VBLayerI masklayer);

  void updateLayerTable();
  void updatewidgets();

  void update_status();
  void reload_averages();

  void init();

  // layout and widgets of the viewer

  // the panels, status line, and image names
  QVBox *maskbox,*statbox;          // simple boxes for layer-specific controls
  QFrame *controlwidget;            // where the layer table and major controls go
  QGridLayout *buttonlayout;        // where the little buttons next to the image go
  QTableWidget *layertable;         // table showing all layers and relevant info
  QTMaskView *maskview;             // the *real* mask widget
  QFormLayout *layout_anat;
  QFormLayout *layout_stat;
  QFormLayout *layout_mask;
  QLineEdit *statusline;

  QStackedWidget *controlstack;     // stack of different control views
  QScrollArea *sview;               // image scroll area
  VBCanvas *drawarea;               // image drawing widget
  QSlider *brightslider;            // brightness widget
  QSlider *contrastslider;          // contrast widget
  QScrollArea *regionview;
  QSlider *xsliceslider,*ysliceslider,*zsliceslider,*tsliceslider;
  QLabel *xposedit,*yposedit,*zposedit,*tposedit;
  QCheckBox *overlaybox;
  QCheckBox *tailbox;
  QCheckBox *flipbox;
  QCheckBox *nsbox;
  QHBox *colbox;                    // n columns for multiview
  QHBox *everynbox;                 // display every n slices
  QComboBox *magbox;                // magnification
  QComboBox *viewbox;               // view (axial, tri, etc.)
  QComboBox *averagebox;            // ???
  QComboBox *cb_coord1,*cb_coord2;  // options dictating how coords are calculated
  // the boxes with info about the image, etc.
  QLineEdit *lowedit,*highedit,*clusteredit;
  QLabel *threshinfo;
  VBScalewidget *q_scale;
  VBScalewidget *q_scale2;
  QSlider *q_trans;
  int regionpos;
  vector<VBV_Mask> q_regions;
  QHBox *xslicebox,*yslicebox,*zslicebox,*tslicebox;
  QRgb q_bgcolor;
  uint32 xcross,ycross;       // view-relative coordinates of crosshairs

  // min and max cube values worked out on load
  // double minvalue,maxvalue;
  
  // time series browser
  QWidget *ts_window;
  QHBoxLayout *ts_layout;
  PlotScreen *tspane;
  QListWidget *tslist;
  QCheckBox *ts_meanscalebox;
  QCheckBox *ts_detrendbox;
  QCheckBox *ts_filterbox;
  QCheckBox *ts_removebox;
  QCheckBox *ts_scalebox;
  QCheckBox *ts_powerbox;
  QCheckBox *ts_pcabox;
  QCheckBox *ts_maskbox;
  QListWidget *ts_averagetype;
  enum {tsmode_mouse=0,tsmode_mask=1,tsmode_cross=2,tsmode_supra=3};
  int q_tstype;
  
  // MASK-RELATED STUFF
  enum {MASKMODE_DRAW,MASKMODE_ERASE} q_maskmode;
  // vector<VBV_Mask> q_masks;
  // vector<VBV_Mask> q_savedmasks;
  // VBV_Mask q_maskslice;
  int q_radius,q_usezradius;
  int q_currentmask,q_currentregion;
  // int maskpos;
  Cube q_mask;             // most recently copied mask
  void ClearMasks();
  
  vector<MyView>::iterator currentview;     // view identified by win2base

  QImage currentimage;

  string averagesfile;
  vector<TASpec> trialsets;
  TASpec *myaverage;

  // string q_filename;
  string q_currentdir;
  int q_viewmode;
  bool q_inmm;             // are coords in mm?  (or voxels)
  bool q_atmouse;          // show coords at mouse?  (or crosshairs)
  bool q_fromorigin;       // coords relative to origin?  (or corner)
  int q_columns;           // columns for multi-view
  int q_everyn;            // every n slices for multi-view
  bool q_crosshairs;       // are the crosshairs rendered?
  bool q_drawmask;         // are we currently drawing (editing) the mask?
  uint32 q_maskcolor;      // color of the mask we're currently drawing
  uint32 q_maskindex;      // index of the mask we're currently drawing
  bool q_showorigin;       // should we render a circle around the origin?
  bool q_showts;
  int q_xviewsize;
  int q_yviewsize;
  int q_volume;
  int q_mag;
  bool q_fliph,q_flipv;
  int q_isotropic;
  double q_xscale;       // scale factor (2.0 means stretched)
  double q_yscale;
  double q_zscale;
  float q_pitch;
  float q_roll;
  float q_yaw;
  int q_update;
  int q_ix,q_iy,q_iz,q_mx,q_my,q_mz;   // last known coordinates
};
