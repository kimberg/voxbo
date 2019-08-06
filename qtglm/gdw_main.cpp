
// gdw_main.cpp
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

using namespace std;

#include <qapplication.h>
#include <qstring.h>
#include <iostream>
#include "gdw.h"
#include "gdw.hlp.h"

void gdw_help();
void gdw_version();

VBPrefs vbp;

/* The main function accepts up to two arguments:
 * If there is no argument, simply launch the interface;
 * If there is only one argument, then set it to be a filename.
 * If there are two or more arguments, set the first is number of time points,
 * the second is TR value (msec) */
int main(int argc, char **argv) {
  QApplication a(argc, argv);
  // By default, set number of time points and TR to be 0
  int inputNumberOfPoints = 0, inputTR = 0, inputMode = 0;
  QString fileName = QString::null;
  tokenlist args;
  vector<string> nonflags;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h")
      gdw_help(), exit(0);
    else if (args[i] == "-v")
      gdw_version(), exit(0);
    else
      nonflags.push_back(args[i]);
  }

  // If there is no any argument, set TR to be 2000 msec
  if (nonflags.empty()) inputTR = 2000;
  // If there is only one argument, set it to be the filename of either G matrix
  // or condition function
  else if (nonflags.size() == 1)
    fileName = QString(nonflags[0].c_str());
  // If there are two or more arguments, set the first to be numberOfPoints, the
  // second to be TR (ms)
  else if (nonflags.size() == 2) {
    inputNumberOfPoints = strtol(nonflags[0]);
    // FIXME why is input tr an int???
    inputTR = strtol(nonflags[1]);
  } else {
    gdw_help();
    exit(1);
  }

  vbp.init();
  Gdw *myGdw = new Gdw(inputNumberOfPoints, inputTR, inputMode, fileName);
  a.setMainWidget(myGdw);
  myGdw->show();

  int result = a.exec();
  return result;
}

void gdw_help() { cout << format(myhelp) % vbversion; }

void gdw_version() { cout << format("VoxBo gdw (v%s)\n") % vbversion; }
