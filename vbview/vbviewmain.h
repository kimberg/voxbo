
// vbviewmain.h
// headers for vbview main window
// Copyright (c) 1998-2010 by The VoxBo Development Team

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
#include <QFileDialog>
#include <QKeyEvent>
#include <string>

// voxbo includes
#include "plotscreen.h"
#include "vbio.h"
#include "vbutil.h"
#include "vbview.h"

class VBViewMain : public Q3VBox {
  Q_OBJECT
 public:
  VBViewMain(QWidget *parent = 0, const char *name = 0);
  VBView *AddView();
 public slots:
  // latest slots
  void NewPane();
  void SaveImage();
  void Close();
  void OpenFile();
  void OpenTemplate(const QString &ff);
  void OpenMask(const QString &ff);
  void OpenDirectory();
  void Quit();
  void SavePNG();
  void Print();
  void SaveMask();
  void Undo();
  void ClearMask();
  void Copy();
  void Cut();
  void Paste();
  void ViewAxial();
  void ViewCoronal();
  void ViewSagittal();
  void ViewMulti();
  void ViewTri();
  void ToggleDisplayPanel();
  void ToggleTSPanel();
  void ToggleOverlays();
  void ToggleHeaderPanel();
  void LoadMask();
  void NewMask();
  void LoadStat();
  void LoadCorr();
  // void LoadAux();
  void LoadGLM();
  // void AddSNRView();
  void LayerInfo();
  void NewOutline();
  void ByteSwap();
  void LoadAverages();
  void GoToOrigin();
  void About();
  void Help();

  // int Separate();
  int ClosePanel(QWidget *w);
  int RenamePanel(QWidget *w, string label);
  void keyPressEvent(QKeyEvent *ke);

 protected:
 private slots:
 private:
  QTabWidget *imagetab;
  map<int, Cube>
      volmap;  // no images will be loaded, but we can store info here
  void LoadMaps();
};
