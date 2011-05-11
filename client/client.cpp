
// client.cpp
// Copyright (c) 2009-2010 by The VoxBo Development Team

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 3, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file named COPYING.  If not, write
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA
//
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg

using namespace std;

#include <QApplication>
#include <QMainWindow>
#include <QImageReader>
#include "dbmainwindow.h"

// This should be made conditional -- only works with static Qt
// (maybe trigger by VB_SHARED ?)
//Q_IMPORT_PLUGIN(qjpeg)
//Q_IMPORT_PLUGIN(qgif)
//Q_IMPORT_PLUGIN(qtiff)

int
main(int argc, char **argv) 
{
  // Qt application
  QApplication app(argc,argv);
  app.setWindowIcon(QIcon(QPixmap(":/icons/windowlogo.png")));
  // parse arguments (none so far)
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
  }
  // create a db main window inside a qt main window
  // QMainWindow *mw=new QMainWindow;
  DBmainwindow *dbmw=new DBmainwindow;
  // mw->setCentralWidget(dbmw);

  
  // DBmainwindow mw;
  dbmw->show();
  return app.exec();
}

