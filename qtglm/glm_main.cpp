
// glm_main.cpp
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

#include "vbprefs.h"
#include "glm.h"
#include <iostream>
#include <qapplication.h>
#include <qstring.h>
#include "glm.hlp.h"

void glm_help();
void glm_version();

VBPrefs vbp;

int main( int argc, char **argv )
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  for (size_t i=0; i<args.size(); i++) {
    if (args[i]=="-v")
      glm_version(),exit(0);
    else if (args[i]=="-h")
      glm_help(),exit(0);
    else {
      glm_help(),exit(1);
    }
  }
  QApplication a( argc, argv );
  
  vbp.init();
  vbp.read_jobtypes();
  glm *glm_main = new glm(0, "glm");
  glm_main->resize( 630, 490 );
  glm_main->setCaption( "GLM Main Interface" );
  a.setMainWidget( glm_main );
  glm_main->show();
  
  a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  int result = a.exec();
  delete glm_main;
  return result;
}


void
glm_help()
{
  cout << format(myhelp) % vbversion;
}

void
glm_version()
{
  cout << format("VoxBo glm (v%s)\n")%vbversion;
}
