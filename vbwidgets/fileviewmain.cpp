
// fileviewmain.cpp
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
// original version written by Tom King and Daniel Y. Kimberg

#include <q3groupbox.h>
#include <q3listbox.h>
#include <q3popupmenu.h>
#include <q3toolbar.h>
#include <q3whatsthis.h>
#include <qaction.h>
#include <qapplication.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmenubar.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qvariant.h>
#include "fileview.h"
#include "vbprefs.h"

VBPrefs vbp;

int main(int argc, char** argv) {
  QApplication a(argc, argv);
  vbp.init();
  fileview v;
  QFont font("SansSerif", 10, 0);
  font.setStyleHint(QFont::SansSerif);
  a.setFont(font);
  v.setFont(font);
  a.setMainWidget(&v);
  v.show();
  v.ShowDirectoriesAlso(1);
  v.ShowImageInformation(1);

  a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  a.exec();
  tokenlist ff = v.ReturnFiles();
  for (int i = 0; i < ff.size(); i++) {
    cout << ff[i] << endl;
  }
  exit(0);
}
