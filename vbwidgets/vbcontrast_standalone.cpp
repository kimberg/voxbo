
// vbcontrast_standalone.cpp
// Copyright (c) 2010 by The VoxBo Development Team

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
// original version written by Tom King?

#include "vbcontrast.h"

#include <qsizepolicy.h>
#include <qapplication.h>

using namespace std;
using namespace VB;

int main(int argc, char* argv[])
{
  QApplication a( argc, argv );

  VBContrastParamScalingWidget the_widget(0, "My Param Scaling Widget");
  the_widget.showBrowseButton(true);
  the_widget.writeFilesOnExit(true);
  
  a.setMainWidget( &the_widget );
  the_widget.show();
  return a.exec();
}
