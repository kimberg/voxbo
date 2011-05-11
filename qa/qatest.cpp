
using namespace std;

#include "display.moc.h"
#include "vb_vector.h"
#include <qpainter.h>
#include <qpixmap.h>
#include <iostream>
#include <fstream>
#include <string>

class VBQA {
public:
  string datadir;        // where to look for data
  string outputdir;      // where to put the reports
  ofstream outfile;      // the output file, open during execution
  int Go(string,string);
  void ShowNormSlices();
  void PlotMoveParams();
  void GenerateMovementWarnings();
  void PlotGlobalSignal();
  void CopyGLMInfo();
};

// EXAMPLE OF HOW TO CAPTURE A PLOT
//   QFrame me;
//   PlotScreen ps(&me);
//   ps.setInputFile("test.ref");
//   //ps.setPlotMode(1);
//   ps.setUpdatesEnabled(TRUE);
//   ps.setYCaption("Hi There");
//   ps.setOrgXLength(250);
//   ps.setXCaption("Yo yo yo");
//   me.show();
//   QPixmap::grabWidget(&ps).save("test.png","PNG");

int
main(int argc,char **argv)
{
  QApplication a(argc,argv); // qt init
  QFont f("Helvetica",10,0);
  a.setFont(f);
  if (argc<3)
    exit(100);
  VBQA qa;
  qa.Go(argv[1],argv[2]);
  exit(0);
}


int
VBQA::Go(string data_dir,string output_dir)
{
  datadir=data_dir;
  outputdir=output_dir;
  // FIXME should really get the dirs graphically
  mkdir(outputdir.c_str(),0777);
  outfile.open((outputdir+(string)"/"+"index.html").c_str(),ios::out);
  if (!outfile)
    return (100);
  ShowNormSlices();
  PlotMoveParams();
  GenerateMovementWarnings();
  PlotGlobalSignal();
  CopyGLMInfo();
  outfile.close();
}

void
VBQA::ShowNormSlices()
{
  QWidget w;
  int i,interval;
  glob_t gb;

  glob("*/nNorm.cub",0,NULL,&gb);
  for (i=0; i<gb.gl_pathc; i++) {  // should only be one, but what the heck
    // open the cube
    interval=cube.dimz/5;
    for (j=1; j<5; j++) {
      // render slice (interval*j)
      QPixmap::grabWidget(&ps).save("norm_1.png","PNG");
    }
  }
  globfree(&gb);
}


void
VBQA::PlotMoveParams()
{
  glob_t gb;
  VB_Vector allmoveparams,x,y,z,pitch,roll,yaw,iterations;
  int i;
  
  glob("*/*_MoveParams.ref",0,NULL,&gb);

  for (i=0; i<gb.gl_pathc; i++) {
    VB_Vector tmpv(gb.gl_pathv[i]);
    if (i==0) {
      allmoveparams.resize(tmpv.getLength());
      allmoveparams=tmpv;
    }
    else
      allmoveparams.concatenate(tmpv);
  }
  int points=allmoveparams.getLength()/7;
  outfile << "total length of move params: " << points << endl;
  globfree(&gb);
  
  x.resize(points);
  y.resize(points);
  z.resize(points);
  pitch.resize(points);
  roll.resize(points);
  yaw.resize(points);
  iterations.resize(points);

  int ind;
  for (i=0; i<points; i++) {
    ind=i*7;
    x[i]=allmoveparams[ind+0];
    y[i]=allmoveparams[ind+1];
    z[i]=allmoveparams[ind+2];
    pitch[i]=allmoveparams[ind+3];
    roll[i]=allmoveparams[ind+4];
    yaw[i]=allmoveparams[ind+5];
    iterations[i]=allmoveparams[ind+6];
  }

  // QFrame me;
  //PlotScreen ps(&me);
  PlotScreen ps(NULL);
  // ps.setInputVector(&x);
  // ps.setPlotMode(1);
  // ps.setUpdatesEnabled(TRUE);
  ps.setYCaption("Estimate (mm)");
  ps.setOrgXLength(points);
  ps.setXCaption("Image Number");

  return;

  ps.setInputVector(&x);
  QPixmap::grabWidget(&ps).save("move_x.png","PNG");

  ps.setInputVector(&y);
  QPixmap::grabWidget(&ps).save("move_y.png","PNG");

  ps.setInputVector(&z);
  QPixmap::grabWidget(&ps).save("move_z.png","PNG");

  ps.setInputVector(&pitch);
  QPixmap::grabWidget(&ps).save("move_pitch.png","PNG");

  ps.setInputVector(&roll);
  QPixmap::grabWidget(&ps).save("move_roll.png","PNG");

  ps.setInputVector(&yaw);
  QPixmap::grabWidget(&ps).save("move_yaw.png","PNG");

  ps.setInputVector(&iterations);
  QPixmap::grabWidget(&ps).save("move_iter.png","PNG");
}


void
VBQA::GenerateMovementWarnings()
{
}


void
VBQA::PlotGlobalSignal()
{
}

void
VBQA::CopyGLMInfo()
{
}


void
VBQA::RenderSlice(QWidget &w,Cube &c,int slice)
{
  int i,j,jj,k,l,mx,my,rowsize,rr,gg,bb;
  unsigned int *p,*q;
  int val,mval;
  int q_mag=1;

  // blow up little images
  if (cube.dimx < 65)
    q_mag=4;

  QImage qim(cube.dimx*q_mag,cube.dimy*q_mag,32);

  for(i=0; i<cube.dimx; i++) {
    for(j=0; j<cube.dimy; j++) {
      // figure out the greyscale value
      val=scaledvalue(minvalue,maxvalue,q_brightness,q_contrast,cube.GetValue(i,j,q_slice));
      rr=gg=bb=val;
      // render the actual voxel, taking the magnification into account
      for(k=0; k<q_mag; k++) {
        p=(unsigned int *)qim.scanLine((cube.dimy-j-1)*q_mag+k);
        p+=q_mag*i;
        for(l=0; l<q_mag; l++) {
	  *p=qRgb(rr,gg,bb);
	  p++;
	}
      }
    }
  }
  currentimage=qim;
}
