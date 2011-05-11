
// main.cpp
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

/* Possible bug: 
 * when adding new relationship on "add new structure" interface,
 * the new relationship's compatibility is not checked completely
 * with other relationships. 
 * For example, suppose the new structure is A, B and C already exists 
 * and B is part of (or child of) C, user is allowed to added:
 * A is part of B;
 * A includes C;
 * although they should be incompatible. 
 */
 
#include "bbdialog.moc.h"
#include <QApplication>
#include <QCompleter>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <string>

using namespace std;

/* main function */
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  BBdialog* myWindow = new BBdialog;
  myWindow->show();

  return app.exec();
}


