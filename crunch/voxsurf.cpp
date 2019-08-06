
// voxsurf.cpp
// take a graphical peek at tes and cub files
// Copyright (c) 2000-2002 by The VoxBo Development Team

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

using namespace std;

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include <gtk/gtk.h>
#include <png.h>
#include "vbio.h"
#include "vbutil.h"

// for backwards compatibility with older libpngs
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#define TT_STRUCT "Load a structural image (TES1, CUB1, img, or imgdir)."
#define TT_INFO "Print info about the currently loaded structural image."
#define TT_FUNCT "Load functional data."
#define TT_LMASK "Load a mask cube."
#define TT_SMASK "Save the mask cube."
#define TT_TMASK "Toggle display of the mask."
#define TT_CMASK "Clear the current mask."
#define TT_QUIT "Quits."
#define TT_PNG "Save a snapshot as a PNG file."

class ROI {
  Cube *cb;
  GdkColor *foo;
  string name;
};

class VBView {
 private:
  guchar *rgbbuf;
  GtkWidget *window;     // the main window
  GtkWidget *darea;      // the image area
  GtkWidget *dwindow;    // the scrolled window the drawing area sits in
  GtkWidget *dirdialog;  // file selection dialog
  // the text boxes
  GtkWidget *imgname, *maskname;
  // all the buttons (some currently out)
  GtkWidget *quitbutton, *smaskbutton, *lmaskbutton, *structbutton;
  GtkWidget *infobutton, *functbutton, *cmaskbutton, *tmaskbutton, *pngbutton;
  GtkWidget *isizelabel, *msizelabel, *iposlabel, *mposlabel;
  GtkWidget *radiuswidget, *tmasklabel;
  char ipostext[32], mpostext[32], itext[32], mtext[32];
  int currentslice, width, height;
  string filename;
  int slicenum, imagenum, magnum;
  int lastmag, lastwidth, lastheight;
  double mincube, maxcube;  // ,lowval,highval;
  double blacklevel;        // center of range, from -200 to +200
  double contrast;          // halfwidth, from 0 to 100
  double contrastwidth, grayval;
  // double contrast,blacklevel;  // they're percentages now
  // the actual images, masks, etc.
  Cube *mycube, *cubemask;
  Tes *mytes, *tesmask;
  // adjustments and range widgets for the sliders
  GtkObject *sliceadj, *imageadj, *magadj, *contrastadj, *blackadj;
  GtkWidget *slicerange, *imagerange, *magrange, *contrastrange, *blackrange;
  GtkWidget *slicelabel, *imagelabel, *contrastlabel, *blacklabel;
  gchar slicetext[32], imagetext[32];
  // rendering stuff
  GdkGC *mygc;
  GdkColor grays[256];
  GdkColor maskcolors[256];
  GdkColor blues[256];
  // mask-related
  int lastx, lasty;  // so that we only draw stuff when we move out of a voxel
  int xratio, yratio;
  int fstatus;
  int maskflag;  // draw the mask?
  int maskval;   // value to draw in green
  int radius;

 public:
  VBView();
  ~VBView();
  void NewDrawArea();
  void CalcMaxMin();
  // misc
  void LoadStruct(const string fname);
  void LoadMask(const string fname);
  void SaveMask(const string fname);
  void KillFileSelection();
  void NewSlice(int slice);
  void NewImage(int image);
  void NewMag(int mag);
  void NewMaskVal(int mag);
  void NewContrast(double contrast);
  void NewBlack(double black);
  int InitColors();
  void SetFileStatus(int s);
  // dialog builders
  void LoadStructDialog();
  void LoadMaskDialog();
  void SaveFileDialog();
  // coordinate conversions
  void Draw2Mask(int &x, int &y);
  void Draw2Img(int &x, int &y);
  void Img2Mask(int &x, int &y);
  int Xoffset();
  int Yoffset();
  // handlers
  void HandleButton(GtkWidget *button);
  void PrintInfo();
  void ClickPixel(int x, int y, int button);
  void ColorMaskPixel(int x, int y, int color);
  void HandleNewRadius();
  // rgb buf stuff
  void BuildRGB();
  void DrawRGB();
  // write png
  int WritePNG(const string &filename);
  //
  guchar scaledvalue(double val);
};

const int COLUMNS = 6;
const int SPACING = 10;

void on_darea_expose(GtkWidget *, GdkEventExpose *, gpointer view);
void handle_vbview_button(GtkWidget *button, gpointer data);
void handle_vbview_close(GtkWidget *button, gpointer data);
void handle_vbview_destroy(GtkWidget *, gpointer);
void handle_vbview_radiuschanged(GtkWidget *, gpointer data);
void handle_file_ok(GtkWidget *, gpointer data);
void handle_file_cancel(GtkWidget *, gpointer data);
void handle_slice_adj(GtkWidget *adj, gpointer data);
void handle_image_adj(GtkWidget *adj, gpointer data);
void handle_mag_adj(GtkWidget *adj, gpointer data);
void handle_mask_adj(GtkWidget *adj, gpointer data);
void handle_contrast_adj(GtkWidget *adj, gpointer data);
void handle_black_adj(GtkWidget *adj, gpointer data);

static gint handle_button_down(GtkWidget *widget, GdkEventButton *event,
                               gpointer view);
static gint handle_motion(GtkWidget *widget, GdkEventMotion *event,
                          gpointer view);

int main(int argc, char *argv[]) {
  VBView *vv;

  gtk_init(&argc, &argv);
  gdk_rgb_init();  // not necessary because if gtk drawing does it
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());
  vv = new VBView();
  if (vv->InitColors())
    cerr << "Some kind of problem finding colors.  Expect weirdness." << endl;

  for (int i = 1; i < argc; i++) {
    // check for flags?
    vv->LoadStruct((string)argv[i]);
  }
  gtk_main();
  gtk_exit(0);
}

VBView::VBView() {
  GtkWidget *mainbox, *rightbox, *leftbox, *rowbox, *label, *r2box;
  GtkTooltips *tooltip;

  blacklevel = -50;
  contrast = 30;
  magnum = 1;
  maskflag = 1;
  xratio = yratio = 1;
  width = height = 1;
  slicenum = imagenum = 0;
  radius = 0;
  mycube = cubemask = (Cube *)NULL;
  mytes = tesmask = (Tes *)NULL;
  rgbbuf = (guchar *)NULL;
  lastmag = lastwidth = lastheight = 0;

  // init tooltips
  tooltip = gtk_tooltips_new();

  // this is the window
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(handle_vbview_destroy), &window);

  // this is the main box
  mainbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(mainbox), 4);
  gtk_container_add(GTK_CONTAINER(window), mainbox);

  // rowbox for leftbox and buttons
  r2box = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(r2box), 0);
  gtk_box_pack_start(GTK_BOX(mainbox), r2box, TRUE, TRUE, 2);

  // this is the leftbox
  leftbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(leftbox), 0);
  gtk_container_add(GTK_CONTAINER(r2box), leftbox);

  // this is the scrolled window the drawing area sits in
  rowbox = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(leftbox), rowbox, TRUE, TRUE, 0);

  dwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(rowbox), dwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dwindow),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize(dwindow, 300, 300);
  // gtk_signal_connect(GTK_OBJECT(dwindow),"button_press_event",
  //	     GTK_SIGNAL_FUNC(handle_button_down),(gpointer)this);
  // gtk_signal_connect(GTK_OBJECT(dwindow),"motion_notify_event",
  //	     GTK_SIGNAL_FUNC(handle_motion),(gpointer)this);

  // this is the rendering area, should really have scrollbars
  darea = gtk_drawing_area_new();
  // gtk_box_pack_start(GTK_BOX(dwindow),darea,FALSE,FALSE,0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(dwindow), darea);
  gtk_widget_set_events(darea,
                        GDK_BUTTON1_MOTION_MASK | GDK_BUTTON3_MOTION_MASK |
                            GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(darea), "expose-event",
                     GTK_SIGNAL_FUNC(on_darea_expose), (gpointer)this);
  // gtk_drawing_area_size(GTK_DRAWING_AREA(darea),256,256);
  gtk_signal_connect(GTK_OBJECT(darea), "button_press_event",
                     GTK_SIGNAL_FUNC(handle_button_down), (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(darea), "motion_notify_event",
                     GTK_SIGNAL_FUNC(handle_motion), (gpointer)this);

  // this is the rightbox
  rightbox = gtk_vbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rightbox), 0);
  gtk_box_pack_start(GTK_BOX(r2box), rightbox, TRUE, FALSE, 4);
  gtk_widget_set_usize(rightbox, 210, 0);

  structbutton = gtk_button_new_with_label("Load Image");
  gtk_widget_set_usize(structbutton, 110, 22);
  gtk_box_pack_start(GTK_BOX(rightbox), structbutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltip, structbutton, TT_STRUCT, NULL);
  gtk_signal_connect(GTK_OBJECT(structbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  // functbutton = gtk_button_new_with_label("Load Overlay");
  // gtk_widget_set_usize(functbutton,110,22);
  // gtk_box_pack_start(GTK_BOX(rightbox),functbutton,FALSE,FALSE,2);
  // gtk_tooltips_set_tip(tooltip,functbutton,TT_FUNCT,NULL);
  // gtk_signal_connect(GTK_OBJECT(functbutton),"clicked",
  //		     (GtkSignalFunc) handle_vbview_button,
  //		     (gpointer)this);

  infobutton = gtk_button_new_with_label("Info");
  gtk_widget_set_usize(infobutton, 110, 22);
  gtk_box_pack_start(GTK_BOX(rightbox), infobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltip, infobutton, TT_INFO, NULL);
  gtk_signal_connect(GTK_OBJECT(infobutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  pngbutton = gtk_button_new_with_label("Save PNG");
  gtk_widget_set_usize(pngbutton, 110, 22);
  gtk_box_pack_start(GTK_BOX(rightbox), pngbutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltip, pngbutton, TT_PNG, NULL);
  gtk_signal_connect(GTK_OBJECT(pngbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  lmaskbutton = gtk_button_new_with_label("Load Mask");
  gtk_widget_set_usize(lmaskbutton, 70, 22);
  gtk_box_pack_start(GTK_BOX(rowbox), lmaskbutton, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltip, lmaskbutton, TT_LMASK, NULL);
  gtk_signal_connect(GTK_OBJECT(lmaskbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  smaskbutton = gtk_button_new_with_label("Save Mask");
  gtk_widget_set_usize(smaskbutton, 70, 22);
  gtk_box_pack_start(GTK_BOX(rowbox), smaskbutton, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltip, smaskbutton, TT_SMASK, NULL);
  gtk_signal_connect(GTK_OBJECT(smaskbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  cmaskbutton = gtk_button_new_with_label("Clear Mask");
  gtk_widget_set_usize(cmaskbutton, 70, 22);
  gtk_box_pack_start(GTK_BOX(rowbox), cmaskbutton, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltip, cmaskbutton, TT_CMASK, NULL);
  gtk_signal_connect(GTK_OBJECT(cmaskbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  tmaskbutton = gtk_button_new_with_label("Hide Mask");
  gtk_widget_set_usize(tmaskbutton, 70, 22);
  gtk_box_pack_start(GTK_BOX(rowbox), tmaskbutton, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltip, tmaskbutton, TT_TMASK, NULL);
  gtk_signal_connect(GTK_OBJECT(tmaskbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  GList *gl = gtk_container_children(GTK_CONTAINER(tmaskbutton));
  tmasklabel = (GtkWidget *)gl->data;

  quitbutton = gtk_button_new_with_label("Quit");
  gtk_widget_set_usize(quitbutton, 110, 22);
  gtk_box_pack_start(GTK_BOX(rightbox), quitbutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltip, quitbutton, TT_QUIT, NULL);
  gtk_signal_connect(GTK_OBJECT(quitbutton), "clicked",
                     (GtkSignalFunc)handle_vbview_button, (gpointer)this);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Image Dims:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);
  isizelabel = gtk_label_new("-");
  gtk_box_pack_start(GTK_BOX(rowbox), isizelabel, FALSE, FALSE, 0);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Mask Dims:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);
  msizelabel = gtk_label_new("-");
  gtk_box_pack_start(GTK_BOX(rowbox), msizelabel, FALSE, FALSE, 0);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Image Position:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);
  iposlabel = gtk_label_new("-");
  // gtk_widget_set_usize(GTK_WIDGET(iposlabel),120,10);
  // gtk_label_set_justify(GTK_LABEL(iposlabel),GTK_JUSTIFY_RIGHT);
  gtk_box_pack_start(GTK_BOX(rowbox), iposlabel, FALSE, FALSE, 0);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Mask Position:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);
  mposlabel = gtk_label_new("-");
  // gtk_label_set_justify(GTK_LABEL(mposlabel),GTK_JUSTIFY_LEFT);
  // gtk_widget_set_usize(GTK_WIDGET(mposlabel),120,10);
  gtk_box_pack_start(GTK_BOX(rowbox), mposlabel, FALSE, FALSE, 0);

  // this is the slice selection slider
  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  label = gtk_label_new("Slice");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);

  slicerange = gtk_hscale_new(NULL);
  gtk_scale_set_digits(GTK_SCALE(slicerange), 0);
  gtk_box_pack_start(GTK_BOX(rowbox), slicerange, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(GTK_WIDGET(slicerange), FALSE);
  gtk_widget_set_usize(slicerange, 110, 30);
  gtk_range_set_update_policy(GTK_RANGE(slicerange), GTK_UPDATE_CONTINUOUS);
  // gtk_scale_set_draw_value(GTK_SCALE(slicerange),FALSE);
  gtk_scale_set_value_pos(GTK_SCALE(slicerange), GTK_POS_RIGHT);

  slicelabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(rowbox), slicelabel, FALSE, FALSE, 0);

  // this is the image (time) selection slider
  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  label = gtk_label_new("Image");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);

  imagerange = gtk_hscale_new(NULL);
  gtk_scale_set_digits(GTK_SCALE(imagerange), 0);
  gtk_box_pack_start(GTK_BOX(rowbox), imagerange, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(GTK_WIDGET(imagerange), FALSE);
  gtk_widget_set_usize(imagerange, 110, 30);
  gtk_range_set_update_policy(GTK_RANGE(imagerange), GTK_UPDATE_CONTINUOUS);
  // gtk_scale_set_draw_value(GTK_SCALE(imagerange),FALSE);
  gtk_scale_set_value_pos(GTK_SCALE(imagerange), GTK_POS_RIGHT);

  imagelabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(rowbox), imagelabel, FALSE, FALSE, 0);

  // this is the magnification selection slider
  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  label = gtk_label_new("Magnification");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);

  magrange = gtk_hscale_new(NULL);
  gtk_scale_set_digits(GTK_SCALE(magrange), 0);
  gtk_box_pack_start(GTK_BOX(rowbox), magrange, FALSE, FALSE, 0);
  gtk_widget_set_usize(magrange, 110, 30);
  gtk_range_set_update_policy(GTK_RANGE(magrange), GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_value_pos(GTK_SCALE(magrange), GTK_POS_RIGHT);

  // because mag only ever has one adjustment, we can create it here
  magadj = gtk_adjustment_new(1, 1, 9, 1, 1, 1);
  gtk_signal_connect(GTK_OBJECT(magadj), "value_changed",
                     (GtkSignalFunc)handle_mag_adj, (gpointer)this);
  gtk_range_set_adjustment(GTK_RANGE(magrange), GTK_ADJUSTMENT(magadj));

  // black and contrast level sliders
  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  label = gtk_label_new("Black level");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);

  blackrange = gtk_hscale_new(NULL);
  gtk_scale_set_digits(GTK_SCALE(blackrange), 0);
  gtk_box_pack_start(GTK_BOX(rowbox), blackrange, FALSE, FALSE, 0);
  gtk_widget_set_usize(blackrange, 110, 30);
  gtk_range_set_update_policy(GTK_RANGE(blackrange), GTK_UPDATE_CONTINUOUS);
  // gtk_scale_set_value_pos(GTK_SCALE(blackrange),GTK_POS_RIGHT);

  blackadj = gtk_adjustment_new(blacklevel, -200, 201, 1.0, 1.0, 1.0);
  gtk_signal_connect(GTK_OBJECT(blackadj), "value_changed",
                     (GtkSignalFunc)handle_black_adj, (gpointer)this);
  gtk_range_set_adjustment(GTK_RANGE(blackrange), GTK_ADJUSTMENT(blackadj));
  gtk_range_default_hslider_update(GTK_RANGE(blackrange));
  gtk_widget_set_sensitive(GTK_WIDGET(blackrange), TRUE);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);

  label = gtk_label_new("Contrast");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);

  contrastrange = gtk_hscale_new(NULL);
  gtk_scale_set_digits(GTK_SCALE(contrastrange), 0);
  gtk_box_pack_start(GTK_BOX(rowbox), contrastrange, FALSE, FALSE, 0);
  gtk_widget_set_usize(contrastrange, 110, 30);
  gtk_range_set_update_policy(GTK_RANGE(contrastrange), GTK_UPDATE_CONTINUOUS);
  // gtk_scale_set_value_pos(GTK_SCALE(contrastrange),GTK_POS_RIGHT);

  contrastadj = gtk_adjustment_new(contrast, 0, 101, 1.0, 1.0, 1.0);
  gtk_signal_connect(GTK_OBJECT(contrastadj), "value_changed",
                     (GtkSignalFunc)handle_contrast_adj, (gpointer)this);
  gtk_range_set_adjustment(GTK_RANGE(contrastrange),
                           GTK_ADJUSTMENT(contrastadj));
  gtk_range_default_hslider_update(GTK_RANGE(contrastrange));
  gtk_widget_set_sensitive(GTK_WIDGET(contrastrange), TRUE);

  rowbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(rightbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Mask draw radius:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 0);
  radiuswidget = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(rowbox), radiuswidget, TRUE, TRUE, 0);
  gtk_entry_set_text(GTK_ENTRY(radiuswidget), "0");
  gtk_signal_connect(GTK_OBJECT(radiuswidget), "changed",
                     GTK_SIGNAL_FUNC(handle_vbview_radiuschanged),
                     (gpointer)this);

  // labels and stuff for filename, etc.

  rowbox = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(mainbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Image file:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 5);
  imgname = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(rowbox), imgname, TRUE, TRUE, 0);
  gtk_entry_set_editable(GTK_ENTRY(imgname), FALSE);

  rowbox = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(rowbox), 0);
  gtk_box_pack_start(GTK_BOX(mainbox), rowbox, FALSE, FALSE, 0);
  label = gtk_label_new("Mask file:");
  gtk_box_pack_start(GTK_BOX(rowbox), label, FALSE, FALSE, 5);
  maskname = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(rowbox), maskname, TRUE, TRUE, 0);
  gtk_entry_set_editable(GTK_ENTRY(maskname), FALSE);

  // NewContrast(30);
  // NewBlack(0);

  gtk_widget_show_all(window);
  mygc = gdk_gc_new(darea->window);
}

void on_darea_expose(GtkWidget *, GdkEventExpose *, gpointer view) {
  ((VBView *)view)->DrawRGB();
}

void handle_vbview_button(GtkWidget *button, gpointer data) {
  ((VBView *)data)->HandleButton(button);
}

void handle_vbview_destroy(GtkWidget *, gpointer) { gtk_main_quit(); }

void handle_vbview_radiuschanged(GtkWidget *, gpointer data) {
  ((VBView *)data)->HandleNewRadius();
}

// the way this should work: the rendering functions need to know for
// each pixel first which pixel of the image it is, second which pixel
// of the mask it is.  we need a little translation algorithm that for
// any address in the darea will tell us either the mask or the image
// address.  here it is:

void VBView::Draw2Mask(int &x, int &y) {
  x = (int)(x / magnum) / xratio;
  y = (int)(y / magnum) / yratio;
  y = cubemask->dimy - y - 1;
}

void VBView::Draw2Img(int &x, int &y) {
  x = (int)(x / magnum);
  y = (int)(y / magnum);
  y = mycube->dimy - y - 1;
}

void VBView::Img2Mask(int &x, int &y) {
  x = (int)(x / xratio);
  y = (int)(y / yratio);
}

int VBView::Xoffset() {
  return (int)gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(dwindow))
      ->value;
}

int VBView::Yoffset() {
  return (int)gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(dwindow))
      ->value;
}

void VBView::HandleNewRadius() {
  char tmp[STRINGLEN];
  strcpy(tmp, gtk_entry_get_text(GTK_ENTRY(radiuswidget)));
  radius = strtol(tmp);

  //    char tmp2[STRINGLEN];
  //    sprintf(tmp2,"%d",radius);
  //    if ((string)tmp != (string)tmp2) {
  //      gtk_entry_set_position(GTK_ENTRY(radiuswidget),-1);
  //      gtk_entry_set_text(GTK_ENTRY(radiuswidget),tmp2);
  //      gtk_entry_set_position(GTK_ENTRY(radiuswidget),-1);
  //    }
}

static gint handle_button_down(GtkWidget *widget, GdkEventButton *event,
                               gpointer view) {
  if (event->type == GDK_2BUTTON_PRESS && event->button == 2)
    ((VBView *)view)->PrintInfo();
  else if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
    ((VBView *)view)->ClickPixel((int)event->x, (int)event->y, 3);
  else
    ((VBView *)view)->ClickPixel((int)event->x, (int)event->y, event->button);
  return TRUE;
}

static gint handle_motion(GtkWidget *widget, GdkEventMotion *event,
                          gpointer view) {
  int button = 0;
  if (event->state & GDK_BUTTON1_MASK)
    button = 1;
  else if (event->state & GDK_BUTTON2_MASK)
    button = 2;
  else if (event->state & GDK_BUTTON3_MASK)
    button = 3;
  if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK) && button == 1)
    button = 3;
  ((VBView *)view)->ClickPixel((int)event->x, (int)event->y, button);
  return TRUE;
}

void VBView::ClickPixel(int x, int y, int button) {
  if (!mycube) return;
  int maskx = x, masky = y;
  int imgx = x, imgy = y;
  int minx, miny, maxx, maxy, i, j;

  Draw2Mask(maskx, masky);
  Draw2Img(imgx, imgy);

  // if we're out of the image ...
  if (imgx > mycube->dimx - 1 || imgy > mycube->dimy - 1 || imgx < 0 ||
      imgy < 0) {
    sprintf(ipostext, "(-,-) = -");
    gtk_label_set_text(GTK_LABEL(iposlabel), ipostext);
    sprintf(mpostext, "(-,-) = -");
    gtk_label_set_text(GTK_LABEL(mposlabel), mpostext);
    return;
  }

  // update the coordinates
  sprintf(ipostext, "(%d,%d) = %.1f", imgx, imgy,
          mycube->GetValue(imgx, imgy, slicenum));
  gtk_label_set_text(GTK_LABEL(iposlabel), ipostext);
  sprintf(mpostext, "(%d,%d) = %d", maskx, masky,
          (int)cubemask->GetValue(maskx, masky, slicenum));
  gtk_label_set_text(GTK_LABEL(mposlabel), mpostext);

  if (button == 0)  // just hovering
    return;

  minx = maskx - radius;
  if (minx < 0) minx = 0;
  maxx = maskx + radius;
  if (maxx > cubemask->dimx - 1) maxx = cubemask->dimx - 1;
  miny = masky - radius;
  if (miny < 0) miny = 0;
  maxy = masky + radius;
  if (maxy > cubemask->dimy - 1) maxy = cubemask->dimy - 1;

  for (i = minx; i <= maxx; i++) {
    for (j = miny; j <= maxy; j++) {
      // make sure we're in the image
      if (i > cubemask->dimx - 1 || j > cubemask->dimy - 1 || i < 0 || j < 0)
        return;

      if (button == 1) {
        cubemask->SetValue(i, j, slicenum, 1);
        ColorMaskPixel(i, j, 1);
        //      BuildRGB();
        if (!maskflag) {
          maskflag = 1;
          DrawRGB();
        }
      } else if (button == 3) {
        cubemask->SetValue(i, j, slicenum, 0);
        ColorMaskPixel(i, j, 2);
        //      BuildRGB();
        if (!maskflag) {
          maskflag = 1;
          DrawRGB();
        }
      }
    }
  }
}

void VBView::ColorMaskPixel(int maskx, int masky, int color) {
  int cindex;

  for (int i = maskx * xratio; i < maskx * xratio + xratio; i++) {
    for (int j = masky * yratio; j < masky * yratio + yratio; j++) {
      // each i,j is a different img voxel
      cindex = scaledvalue(mycube->GetValue(i, j, slicenum));
      if (color == 1)
        gdk_gc_set_foreground(mygc, maskcolors + cindex);
      else
        gdk_gc_set_foreground(mygc, grays + cindex);
      gdk_draw_rectangle(darea->window, mygc, TRUE, i * magnum,
                         (mycube->dimy - j - 1) * magnum, magnum, magnum);
    }
  }
}

void VBView::DrawRGB() {
  if (!rgbbuf) return;  // should never happen
  gdk_draw_rgb_image(darea->window, darea->style->fg_gc[GTK_STATE_NORMAL], 0, 0,
                     width, height, GDK_RGB_DITHER_MAX, rgbbuf, width * 3);
}

inline guchar VBView::scaledvalue(double val) {
  double low = grayval - contrastwidth;
  double high = grayval + contrastwidth;
  double bval = (((val - low) * 255.0 / (high - low)));
  if (bval > 255) bval = 255;
  if (bval < 0) bval = 0;
  return (guchar)bval;
}

void VBView::BuildRGB() {
  int i, j, jj, k, l, x, y, rowsize, rr, gg, bb;
  guchar *pos, *pp, byteval;
  double dval;

  if (!mycube || !cubemask) return;
  if (!rgbbuf || magnum != lastmag || mycube->dimx != lastwidth ||
      mycube->dimy != lastheight)
    rgbbuf = new guchar[mycube->dimx * mycube->dimy * 3 * magnum * magnum];
  lastmag = magnum;
  lastwidth = mycube->dimx;
  lastheight = mycube->dimy;
  rowsize = lastwidth * 3 * magnum;
  pos = (guchar *)rgbbuf;
  for (i = 0; i < mycube->dimx; i++) {
    for (j = 0; j < mycube->dimy; j++) {
      jj = mycube->dimy - j - 1;
      pos = (guchar *)rgbbuf + (jj * magnum * rowsize) + (i * 3 * magnum);
      dval = mycube->GetValue(i, j, slicenum);
      byteval = scaledvalue(dval);
      x = i;
      y = j;
      Img2Mask(x, y);
      rr = gg = bb = byteval;
      if (cubemask->GetValue(x, y, slicenum) && maskflag)
        gg = bb = byteval * 2 / 3;
      else if (maskval && maskval == (int)dval)
        rr = bb = byteval * 2 / 3;
      for (k = 0; k < magnum; k++) {
        pp = pos + k * rowsize;
        for (l = 0; l < magnum; l++) {
          *pp = (guchar)rr;
          *(pp + 1) = (guchar)gg;
          *(pp + 2) = (guchar)bb;
          pp += 3;
        }
      }
    }
  }
}

void VBView::PrintInfo() {
  if (mytes)
    mytes->print();
  else if (mycube)
    mycube->print();
}

// HandleButton() is a clearinghouse for main window button events

void VBView::HandleButton(GtkWidget *button) {
  if (button == lmaskbutton) {
    LoadMaskDialog();
    filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dirdialog));
    KillFileSelection();
    if (fstatus) LoadMask(filename);
  } else if (button == smaskbutton) {
    SaveFileDialog();
    filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dirdialog));
    KillFileSelection();
    if (fstatus) SaveMask(filename);
  } else if (button == pngbutton) {
    SaveFileDialog();
    filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dirdialog));
    KillFileSelection();
    if (fstatus) WritePNG(filename);
  } else if (button == cmaskbutton) {
    cubemask->clear();
    BuildRGB();
    DrawRGB();
  } else if (button == tmaskbutton) {
    maskflag = 1 - maskflag;
    if (maskflag)
      gtk_label_set_text(GTK_LABEL(tmasklabel), "Hide Mask");
    else
      gtk_label_set_text(GTK_LABEL(tmasklabel), "Show Mask");
    BuildRGB();
    DrawRGB();
  } else if (button == infobutton) {
    PrintInfo();
  } else if (button == quitbutton) {
    gtk_main_quit();
  } else if (button == structbutton) {
    LoadStructDialog();
    filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dirdialog));
    KillFileSelection();
    if (fstatus) LoadStruct(filename);
  } else if (button == functbutton) {
  }
}

void VBView::LoadMaskDialog() {
  dirdialog = gtk_file_selection_new("Choose a structural image file");
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->ok_button),
                     "clicked", (GtkSignalFunc)handle_file_ok, (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->cancel_button),
                     "clicked", (GtkSignalFunc)handle_file_cancel,
                     (gpointer)this);
  // gtk_file_selection_set_filename(GTK_FILE_SELECTION(dirdialog),dir);
  gtk_grab_add(dirdialog);
  gtk_widget_show(dirdialog);
  gtk_main();
}

void VBView::SaveFileDialog() {
  dirdialog = gtk_file_selection_new("File to save:");
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->ok_button),
                     "clicked", (GtkSignalFunc)handle_file_ok, (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->cancel_button),
                     "clicked", (GtkSignalFunc)handle_file_cancel,
                     (gpointer)this);
  // gtk_file_selection_set_filename(GTK_FILE_SELECTION(dirdialog),dir);
  gtk_grab_add(dirdialog);
  gtk_widget_show(dirdialog);
  gtk_main();
}

void VBView::LoadStructDialog() {
  dirdialog = gtk_file_selection_new("Choose a structural image file");
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->ok_button),
                     "clicked", (GtkSignalFunc)handle_file_ok, (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dirdialog)->cancel_button),
                     "clicked", (GtkSignalFunc)handle_file_cancel,
                     (gpointer)this);
  // gtk_file_selection_set_filename(GTK_FILE_SELECTION(dirdialog),dir);
  gtk_grab_add(dirdialog);
  gtk_widget_show(dirdialog);
  gtk_main();
}

void VBView::LoadMask(const string fname) {
  Cube *tmpcub = new Cube;
  if (tmpcub->ReadFile(fname.c_str())) {
    cerr << "Couldn't load mask " << fname << endl;
    return;
  }
  if (!tmpcub->data_valid) {
    cerr << "Couldn't load mask " << fname << endl;
    delete tmpcub;
    return;
  }
  // should probably check z too
  if (mycube->dimx % tmpcub->dimx || mycube->dimy % tmpcub->dimy) {
    cerr << "The dimensions of mask " << fname
         << " are incompatible with this image." << endl;
    cerr << "  Mask: " << tmpcub->dimx << "x" << tmpcub->dimy << "x"
         << tmpcub->dimz << endl;
    cerr << " Image: " << mycube->dimx << "x" << mycube->dimy << "x"
         << mycube->dimz << endl;
    return;
  }
  xratio = mycube->dimx / tmpcub->dimx;
  yratio = mycube->dimy / tmpcub->dimy;
  delete cubemask;
  cubemask = tmpcub;
  gtk_entry_set_text(GTK_ENTRY(maskname), fname.c_str());
  sprintf(mtext, "%dx%d", cubemask->dimx, cubemask->dimy);
  gtk_label_set_text(GTK_LABEL(msizelabel), mtext);
  chdir(xdirname(fname).c_str());
  BuildRGB();
  DrawRGB();
}

void VBView::SaveMask(const string fname) {
  cubemask->SetFileName(fname);
  cubemask->WriteFile();
  gtk_entry_set_text(GTK_ENTRY(maskname), fname.c_str());
  chdir(xdirname(fname).c_str());
}

void VBView::LoadStruct(const string fname) {
  imagenum = slicenum = 0;

  if (mytes) delete mytes;
  if (mycube) delete mycube;
  if (cubemask) delete cubemask;
  if (tesmask) delete tesmask;
  mytes = tesmask = (Tes *)NULL;
  mycube = cubemask = (Cube *)NULL;

  vector<VBFF> ftypes = EligibleFileTypes(fname);
  if (!ftypes.size()) return;

  switch (EligibleFileTypes(fname)[0].getDimensions()) {
    case 4:
      mytes = new Tes;
      mytes->ReadFile(fname.c_str());
      if (mytes->data_valid)
        mycube = (*mytes)[0];
      else {
        delete mytes;
        mytes = (Tes *)NULL;
      }
      break;
    case 3:
      mycube = new Cube;
      mycube->ReadFile(fname.c_str());
      if (!mycube->data_valid) {
        delete mycube;
        mycube = (Cube *)NULL;
      }
      break;
    default:
      printf("voxsurf: can't handle this filetype yet\n");
      break;
  }

  if (mycube) {
    CalcMaxMin();

    // i have no idea why the following has to use dimz+1 to make it go to dimz

    sliceadj = gtk_adjustment_new(0.0, 0.0, mycube->dimz, 1.0, 1.0, 1.0);
    gtk_signal_connect(GTK_OBJECT(sliceadj), "value_changed",
                       (GtkSignalFunc)handle_slice_adj, (gpointer)this);
    gtk_range_set_adjustment(GTK_RANGE(slicerange), GTK_ADJUSTMENT(sliceadj));
    gtk_widget_set_sensitive(GTK_WIDGET(slicerange), TRUE);
    sprintf(slicetext, "(0-%d)", mycube->dimz - 1);
    gtk_label_set_text(GTK_LABEL(slicelabel), slicetext);

    if (mycube->dimx % 4 == 0 &&
        mycube->dimy % 4 == 0)  // try quarter-res masks
      xratio = yratio = 4;
    else if (mycube->dimx % 2 == 0 &&
             mycube->dimy % 2 == 0)  // try quarter-res masks
      xratio = yratio = 2;
    else
      xratio = yratio = 1;
    cubemask = new Cube(mycube->dimx / xratio, mycube->dimy / yratio,
                        mycube->dimz, vb_byte);
    cubemask->clear();

    // display the names and dimensions of the cube and mask
    gtk_entry_set_text(GTK_ENTRY(imgname), fname.c_str());
    gtk_entry_set_text(GTK_ENTRY(maskname), "");
    sprintf(itext, "%dx%d", mycube->dimx, mycube->dimy);
    gtk_label_set_text(GTK_LABEL(isizelabel), itext);
    sprintf(mtext, "%dx%d", cubemask->dimx, cubemask->dimy);
    gtk_label_set_text(GTK_LABEL(msizelabel), mtext);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(slicerange), FALSE);
  }

  if (mytes) {
    // i have no idea why the following has to use dimt to make it go to dimt-1
    imageadj = gtk_adjustment_new(0.0, 0.0, mytes->dimt, 1.0, 1.0, 1.0);
    gtk_signal_connect(GTK_OBJECT(imageadj), "value_changed",
                       (GtkSignalFunc)handle_image_adj, (gpointer)this);
    gtk_range_set_adjustment(GTK_RANGE(imagerange), GTK_ADJUSTMENT(imageadj));
    gtk_widget_set_sensitive(GTK_WIDGET(imagerange), TRUE);
    // tesmask = new
    // Tes(mytes->dimx,mytes->dimy,mytes->dimz,mytes->dimt,vb_byte);
    sprintf(imagetext, "(0-%d)", mytes->dimt - 1);
    gtk_label_set_text(GTK_LABEL(imagelabel), imagetext);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(imagerange), FALSE);
  }
  if (mycube) {
    NewDrawArea();
    gtk_adjustment_set_value(GTK_ADJUSTMENT(sliceadj), 0);
  }
  chdir(xdirname(fname).c_str());
}

void VBView::KillFileSelection() {
  gtk_grab_remove(dirdialog);
  gtk_widget_destroy(dirdialog);
}

void VBView::NewSlice(int slice) {
  if (!mycube) return;
  slicenum = slice;
  BuildRGB();
  DrawRGB();
}

void VBView::NewImage(int image) {
  if (mycube) delete mycube;
  imagenum = image;
  mycube = (*mytes)[imagenum];
  BuildRGB();
  DrawRGB();
}

void VBView::NewMag(int mag) {
  if (!mycube) return;
  magnum = mag;
  if (mycube) {
    NewDrawArea();
  }
}

void VBView::NewMaskVal(int mv) {
  if (!mycube) return;
  maskval = mv;
  BuildRGB();
  DrawRGB();
}

void VBView::NewContrast(double cont) {
  contrast = cont;
  contrastwidth = ((100.0 - (double)contrast) / 100.0) * (maxcube - mincube);
  // cout << "contrastwidth: " << contrastwidth << endl;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(contrastadj), contrast);
}

void VBView::NewBlack(double black) {
  blacklevel = black;
  grayval = mincube +
            ((maxcube - mincube) * 0.5) * (1.0 + ((double)blacklevel / 100.0));
  // cout << "grayval :" << grayval << endl;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(blackadj), black);
}

void handle_contrast_adj(GtkWidget *adj, gpointer data) {
  double cont = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewContrast(cont);
  ((VBView *)data)->BuildRGB();
  ((VBView *)data)->DrawRGB();
}

void handle_black_adj(GtkWidget *adj, gpointer data) {
  double black = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewBlack(black);
  ((VBView *)data)->BuildRGB();
  ((VBView *)data)->DrawRGB();
}

void handle_mag_adj(GtkWidget *adj, gpointer data) {
  int mag = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewMag(mag);
}

void handle_mask_adj(GtkWidget *adj, gpointer data) {
  int maskval = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewMaskVal(maskval);
}

void handle_image_adj(GtkWidget *adj, gpointer data) {
  int image = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewImage(image);
}

void handle_slice_adj(GtkWidget *adj, gpointer data) {
  int slice = (int)(GTK_ADJUSTMENT(adj)->value);
  ((VBView *)data)->NewSlice(slice);
}

void handle_file_ok(GtkWidget *, gpointer data) {
  ((VBView *)data)->SetFileStatus(TRUE);
  gtk_main_quit();
}

void handle_file_cancel(GtkWidget *, gpointer data) {
  ((VBView *)data)->SetFileStatus(FALSE);
  gtk_main_quit();
}

void VBView::SetFileStatus(int s) { fstatus = s; }

// CalcMaxMin() does what it sounds like it does

void VBView::CalcMaxMin() {
  int i, j, k;
  double val;

  maxcube = mincube = mycube->GetValue(0, 0, 0);
  for (i = 0; i < mycube->dimx; i++) {
    for (j = 0; j < mycube->dimy; j++) {
      for (k = 0; k < mycube->dimz; k++) {
        val = mycube->GetValue(i, j, k);
        if (val > maxcube) maxcube = val;
        if (val < mincube) mincube = val;
      }
    }
  }
  NewBlack(blacklevel);
  NewContrast(contrast);
}

// NewDrawArea() should be called any time we change the apparent size
// of the image in the drawing area.  Basically any time we load a new
// image or change the magnification.

void VBView::NewDrawArea() {
  width = mycube->dimx * magnum;
  height = mycube->dimy * magnum;
  gtk_drawing_area_size(GTK_DRAWING_AREA(darea), width, height);
  BuildRGB();
}

int VBView::InitColors() {
  gboolean rets[256];
  int i, err, big, little;
  for (i = 0; i < 256; i++) {
    big = i * 256;
    little = big * 2 / 3;
    grays[i].red = big;
    grays[i].green = big;
    grays[i].blue = big;
    maskcolors[i].red = big;
    maskcolors[i].green = little;
    maskcolors[i].blue = little;
    blues[i].red = little;
    blues[i].green = little;
    blues[i].blue = big;
  }
  err = gdk_colormap_alloc_colors(gdk_colormap_get_system(), grays, 256, FALSE,
                                  TRUE, rets);
  err += gdk_colormap_alloc_colors(gdk_colormap_get_system(), maskcolors, 256,
                                   FALSE, TRUE, rets);
  err += gdk_colormap_alloc_colors(gdk_colormap_get_system(), blues, 256, FALSE,
                                   TRUE, rets);
  return err;
}

int VBView::WritePNG(const string &filename) {
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 width, height;

  width = mycube->dimx * magnum;
  height = mycube->dimy * magnum;

  /* open the file */
  fp = fopen(filename.c_str(), "wb");
  if (fp == NULL) return (101);

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also check that
   * the library version is compatible with the one used at compile time,
   * in case we are using dynamically linked libraries.  REQUIRED.
   */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL) {
    fclose(fp);
    return (102);
  }

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return (103);
  }

  /* Set error handling.  REQUIRED if you aren't supplying your own
   * error handling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* If we get here, we had a problem reading the file */
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return (104);
  }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  png_uint_32 k, bpp = 3;  // bpp = bytes per pixel
  // png_byte image[height][width*bpp];
  png_bytep row_pointers[height];
  for (k = 0; k < height; k++) row_pointers[k] = rgbbuf + k * width * bpp;

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
  return (0);
}
