
// vbqa.cpp
// perform quality assurance on a directory
// Copyright (c) 1998-2004 by The VoxBo Development Team

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

#include <qapplication.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include "plotscreen.h"
#include "vbutil.h"
#include "vbio.h"
#include "vbprefs.h"

void boxit(ofstream &os,string text,string subtext);
void vbqa_help();

class ProcPattern {
public:
  vector <string> steps;
  vector <string> filelist;
};

class VBQA {
private:
//   vector<string> normpat;
//   vector<string> showallpat;
//   vector<string> filepat;
public:
  vector<string>dirlist;
  string outputdir;      // where to put the reports
  ofstream outfile;      // the output file, open during execution
  int levels;
  int Go(tokenlist &args);
  // the actual tests
  void ShowNormSlices(vector<string> dirlist);
  void PlotMoveParams(vector<string> dirlist);
  void GenerateMovementWarnings(vector<string> dirlist);
  void PlotGlobalSignal(vector<string> dirlist);
  void CheckGLMInfo(vector<string> dirlist);
  void ExamineTesFiles(vector<string> dirlist);
  // build this build that
  void BuildIndex();
  void BuildDirList(string dir,int level);
  // void LoadQAConf();
};

VBPrefs vbp;

int
main(int argc,char **argv)
{
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  if (args.size()==0) {
    vbqa_help();
    exit(0);
  }

  QApplication a(argc,argv); // qt init
  QWidget w(NULL,NULL);
  a.setMainWidget(&w);
  QFont f("Helvetica",10,0);
  a.setFont(f);
  VBQA qa;

  vbp.init();
  qa.Go(args); 
  exit(0);
}

int
VBQA::Go(tokenlist &args)
{
  string outputname;
  string rootdir;
  vector<string> deletelist;
  int i,deleteflag=0;
  char tmp[STRINGLEN];

  // default: no levels deep, use current dir, use dirname as name
  levels=100;
  char buf[STRINGLEN];
  getcwd(buf,STRINGLEN);
  outputname=buf;
  rootdir=buf;
  if (outputname=="") outputname="foo";

  for (i=0; i<args.size(); i++) {
    if (args[i]=="-l") {
      if (i<args.size()-1) {
        levels=strtol(args[i+1]);
        i++;
      }
    }
    else if (args[i]=="-n") {
      if (i<args.size()-1) {
        outputname=args[i+1];
        i++;
      }
    }
    else if (args[i]=="-d") {
      deleteflag=1;
    }
    else {
      if (deleteflag)
        deletelist.push_back(args[i]);
      else {
        rootdir=args[i];
        outputname=rootdir;
      }
    }
  }

  // convert slashes to _'s in outputname
  for (i=0; i<(int)outputname.size(); i++)
    if (outputname[i]=='/')
      outputname[i]='_';
  outputname=xstripwhitespace(outputname,"_");  // strip leading and trailing underscores

  // if we're deleting, just delete
  if (deleteflag) {
    for (i=0; i<(int)deletelist.size(); i++) {
      sprintf(tmp,"rm -r %s/.voxbo/reports/%s",vbp.homedir.c_str(),deletelist[i].c_str());
      system(tmp);
    }
    vb_buildindex();
    exit(0);
  }

  // load config stuff from either rootdir/etc/vbqa.conf or
  // homedir/.voxbo/vbqa.conf
  // LoadQAConf();

  // BuildDirList creates a list of all the directories to be examined
  BuildDirList(rootdir,levels);

  // try to make reports directory just in case
  sprintf(tmp,"%s/.voxbo",vbp.homedir.c_str());
  mkdir(tmp,0777);
  // try to make reports directory just in case
  sprintf(tmp,"%s/.voxbo/reports",vbp.homedir.c_str());
  mkdir(tmp,0777);
  // make a directory for this report
  sprintf(tmp,"%s/.voxbo/reports/%s",vbp.homedir.c_str(),outputname.c_str());
  mkdir(tmp,0777);
  outputdir=tmp;
  outfile.open((outputdir+(string)"/"+"index.html").c_str(),ios::out);
  if (!outfile)
    return (100);

  // now run all the tests
  ShowNormSlices(dirlist);
  PlotMoveParams(dirlist);
  GenerateMovementWarnings(dirlist);
  PlotGlobalSignal(dirlist);
  CheckGLMInfo(dirlist);
  ExamineTesFiles(dirlist);
  outfile.close();

  // now build the index to all your reports
  BuildIndex();

  return (0);
}

void
VBQA::BuildIndex()
{
  char tmp[STRINGLEN],indname[STRINGLEN];
  ofstream indstream;

  sprintf(indname,"%s/.voxbo/reports/index.html",vbp.homedir.c_str());
  indstream.open(indname,ios::out);
  if (!indstream) {
    // FIXME do something?
    return;
  }
  // start off the html file
  indstream << "<html>\n<head>\n<title>Your VoxBo Quality Assurance Page</title>\n</head>" << endl;
  indstream << "<body bgcolor=white>\n\n";
  indstream << "<h1>Your VoxBo Quality Assurance Page</h1>\n\n";

  indstream << "<p>" << endl;
  indstream << "The table below shows your collection of VoxBo quality assurance reports," << endl;
  indstream << "generated by <b>vbqa</b>.  To delete one of these reports, type" << endl;
  indstream << "<i>vbqa -d <name></i>, where <name> is the name of the report you want to" << endl;
  indstream << "delete (if it has spaces, enclose the name in double quotes.)" << endl;
  indstream << "For more information, type <i>vbqa</i> (with no arguments) at the" << endl;
  indstream << "command line." << endl;
  indstream << "</p>" << endl;

  indstream << "<table cols=1 border=1 cellspacing=0 marginwidth=0>" << endl;
  indstream << "<tr><td bgcolor=#D0D0D0><center><b>Try one of these links:</b></center></td>" << endl;
  
  vglob vg(vbp.homedir+"/.voxbo/reports/*");
  for (size_t i=0; i<vg.size(); i++) {
    // is it a directory?
    if (!vb_direxists(vg[i]))
      continue;
    string fname=xfilename(vg[i]);
    indstream << "<tr><td>" << endl;
    sprintf(tmp,"<a href=\"%s/.voxbo/reports/%s/index.html\">%s</a>",
            vbp.homedir.c_str(),fname.c_str(),fname.c_str());
    indstream << tmp << endl;
  }
  indstream << "</table>\n</body>\n</html>\n" << endl;
  indstream.close();
  return;
}

void
VBQA::ShowNormSlices(vector<string> dirlist)
{
  QWidget w;
  int j,interval;
  glob_t gb;
  char fname[512];
  Cube cube;

  if (dirlist.size()==0)
    return;
  for (size_t i=0; i<dirlist.size(); i++) {
    vg.append(dirlist[i]+"/*/nNorm.cub");
    vg.append(dirlist[i]+"/*/nAnatomical.cub");
  }
  for (size_t i=0; i<vg.size(); i++) {  // should only be one, but what the heck
    cube.ReadFile(vg[i]);
    interval=cube.dimz/5;
    for (j=1; j<5; j++) {
      sprintf(fname,"%s/nnorm_%d_%d.png",outputdir.c_str(),i,j);
      WritePNG(cube,j*interval,fname);
    }

    string subtitle=(string)"slices from <b>"+vg[i]+(string)"</b>";
    boxit(outfile,"Normalization Results",subtitle);

    //    outfile << "<h2>Normalization Results</h2>" << endl;
    outfile << "<p>" << endl;
    outfile << "Here are some normalized slices from " << vg[i] << ":" << endl;
    outfile << "</p>" << endl;
    outfile << "<p>" << endl;
    outfile << "<img src=\"nnorm_" << i << "_1.png\">" << endl;
    outfile << "<img src=\"nnorm_" << i << "_2.png\">" << endl;
    outfile << "<img src=\"nnorm_" << i << "_3.png\">" << endl;
    outfile << "<img src=\"nnorm_" << i << "_4.png\">" << endl;
    outfile << "<br>" << endl;
    outfile << "</p>" << endl;
  }
  // FIXME should also show some slices from the template, to see how things went
}

void
VBQA::PlotMoveParams(vector<string> dirlist)
{
  VB_Vector allmoveparams,x,y,z,pitch,roll,yaw,iterations;
  size_t i,j,multiples;
  char fname[STRINGLEN];
  
  for (i=0; i<dirlist.size(); i++) {
    glob vg(dirlist[i]+"/*/*_MoveParams.ref");
    // if nothing to report on...
    if (!vg.size())
      continue;
    
    for (j=0; j<vg.size(); j++) {
      VB_Vector tmpv(vg[j]);
      if (j==0) {
        allmoveparams.resize(tmpv.getLength());
        allmoveparams=tmpv;
      }
      else
        allmoveparams.concatenate(tmpv);
    }
    int points=allmoveparams.getLength()/7;
      
    // found no data?
    if (!allmoveparams.getLength())
      continue;
      
    x.resize(points);
    y.resize(points);
    z.resize(points);
    pitch.resize(points);
    roll.resize(points);
    yaw.resize(points);
    iterations.resize(points);
    
    int ind;
    multiples=0;
    for (j=0; j<points; j++) {
      ind=j*7;
      x[j]=allmoveparams[ind+0];
      y[j]=allmoveparams[ind+1];
      z[j]=allmoveparams[ind+2];
      pitch[j]=allmoveparams[ind+3];
      roll[j]=allmoveparams[ind+4];
      yaw[j]=allmoveparams[ind+5];
      iterations[j]=allmoveparams[ind+6];
      if (iterations[j]>1)
        multiples++;
    }
      
    char tmpf[STRINGLEN];
    PlotScreen ps(NULL);
    ps.setPlotMode(1);
    ps.setUpdatesEnabled(TRUE);
    // FIXME ps.setOrgXLength(points);
    ps.setXCaption("Image Number (starting with 0)");
    
    ps.addVector(&x,"red");
    ps.addVector(&y,"green");
    ps.addVector(&z,"blue");
    ps.setYCaption("Motion in xyz (mm, red/green/blue)");
    sprintf(tmpf,"%s/move_%d_xyz.png",outputdir.c_str(),i);
    QPixmap::grabWidget(&ps).save(tmpf,"PNG");
    
    ps.clear();
    ps.addVector(&pitch,"red");
    ps.addVector(&roll,"green");
    ps.addVector(&yaw,"blue");
    ps.setYCaption("Pitch, roll, and yaw (degrees, red/green/blue)");
    sprintf(tmpf,"%s/move_%d_pry.png",outputdir.c_str(),i);
    QPixmap::grabWidget(&ps).save(tmpf,"PNG");
    
    ps.clear();
    ps.addVector(&iterations);
    ps.setYCaption("Iterations applied to correct movement");
    sprintf(tmpf,"%s/move_%d_iter.png",outputdir.c_str(),i);
    QPixmap::grabWidget(&ps).save(tmpf,"PNG");
    
    string subtitle=(string)"movement parameters from directories below <b>"+(string)dirlist[i]+(string)"</b>";
    boxit(outfile,"Movement Parameter Graphs",subtitle);

    // outfile << "<h2>Movement Parameters</h2>" << endl;
    outfile << "<p>" << endl;
    outfile << "Here are graphs of your movement parameters." << endl;
    outfile << "</p>" << endl;
      
    if (multiples) {
      outfile << "<p>" << endl;
      outfile << "<font size=+1 color=red><b>Warning</b></font>" << endl;
      outfile << "Note that " << multiples << " of your images "
	      << (multiples==1 ? "was" : "were") << " corrected more than once." << endl;
      outfile << "</p>" << endl;
    }
      
    outfile << "<p>" << endl;
    outfile << "<img src=\"move_" << i << "_xyz.png\"><br>" << endl;
    outfile << "<img src=\"move_" << i << "_pry.png\"><br>" << endl;
    outfile << "<img src=\"move_" << i << "_iter.png\"><br>" << endl;
    outfile << "</p>" << endl;
  }
}


void
VBQA::GenerateMovementWarnings(vector<string> dirlist)
{
}


void
VBQA::PlotGlobalSignal(vector<string> dirlist)
{
  VB_Vector allgs;
  size_t i,j;
  char fname[STRINGLEN];
  
  for (i=0; i<dirlist.size(); i++) {
    vglob vg(dirlist[i]+"/*/*_GS.ref");
    // if nothing to report on...
    if (!vg.size())
      continue;

    for (j=0; j<vg.size(); j++) {
      VB_Vector tmpv(vg[j]);
      if (j==0) {
        allgs.resize(tmpv.getLength());
        allgs=tmpv;
      }
      else
        allgs.concatenate(tmpv);
    }

    // found no data?
    if (!allgs.getLength())
      return;

    char tmpf[STRINGLEN];
    PlotScreen ps(NULL);
    ps.setPlotMode(1);
    ps.setUpdatesEnabled(TRUE);
    // FIXME ps.setOrgXLength(allgs.getLength());
    ps.setXCaption("Image Number (starting with 0)");

    ps.addVector(&allgs);
    ps.setYCaption("Global Signal (signal values)");
    sprintf(tmpf,"%s/gs_%d.png",outputdir.c_str(),i);
    QPixmap::grabWidget(&ps).save(tmpf,"PNG");

    string subtitle=(string)"global signal from files a level below <b>"+(string)dirlist[i]+(string)"</b>";
    boxit(outfile,"Global Signal",subtitle);

    outfile << "<pre>from below directory: " << dirlist[i] << "</pre><br>" << endl;
    outfile << "<p>" << endl;
    outfile << "Here is the global signal from all your processed 4D files, concatenated" << endl;
    outfile << "into one big graph." << endl;
    outfile << "</p>" << endl;

    // hardcoded criterion for spikes: mean +/- 4sd
    double sd=sqrt(allgs.getVariance());
    double mean=allgs.getVectorMean();
    double low=mean-(3.5*sd);
    double high=mean+(3.5*sd);
    vector<int> spikes,lowvals;
    for (j=0; j<(int)allgs.getLength(); j++) {
      if (allgs[j]>high || allgs[j]<low)
        spikes.push_back(j);
      if (fabs(allgs[j])<.001)
        lowvals.push_back(j);
    }
    if (spikes.size()) {
      outfile << "<p>" << endl;
      outfile << "<font size=+1 color=red><b>Warning</b></font>" << endl;
      outfile << "We found " << spikes.size() << " spikes in your data, using a criterion of 3.5sd," << endl;
      outfile << "at the following time points (counting from 0): " << endl;
      outfile << spikes[0];
      for (j=1; j<(int)spikes.size(); j++)
        outfile << ", " << spikes[j];
      outfile << "\n</p>" << endl;
    }
    if (lowvals.size()) {
      outfile << "<p>" << endl;
      outfile << "<font size=+1 color=red><b>Warning</b></font>" << endl;
      outfile << "We found " << lowvals.size() << " values in your data below an absolute value of .001," << endl;
      outfile << "at the following time points (counting from 0): " << endl;
      for (j=0; j<(int)lowvals.size(); j++)
        outfile << lowvals[j] << " ";
      outfile << "It might not be an issue, but you might want to make sure data didn't get zeroed out somehow." << endl;
      outfile << "</p>" << endl;
    }

    outfile << "<p>" << endl;
    outfile << "<img src=\"gs_" << i << ".png\"><br>" << endl;
    outfile << "</p>" << endl;
  }
}

void
VBQA::CheckGLMInfo(vector<string> dirlist)
{
  size_t i,j;
  ifstream infofile;
  struct stat st;
  string prmname,subtitle;
  char buf[STRINGLEN];
  int goflag;
  int aftercollflag=0,collinearitywarnings=0;
  tokenlist toks;
  toks.SetSeparator("\t\n");
  
  for(i=0; i<(int)dirlist.size(); i++) {
    vglob vg("*.info");
    vg.append("*/*.info");
    for (j=0; j<vg.size(); j++) {
      // make sure there's a corresponding prm file
      prmname=vg[j];
      prmname.replace(prmname.size()-5,5,".prm");
      if (stat(prmname.c_str(),&st))
        continue;
      infofile.open(vg[j],ios::in);
      if (infofile) {
        subtitle=(string)"info file from directory <b>"+(string)gb.gl_pathv[j]+(string)"</b>";
        boxit(outfile,"GLM Audit Results",subtitle);

        outfile << "<pre>" << endl;
        goflag=0;
        while (!infofile.eof()) {
          infofile.getline(buf,STRINGLEN);
          toks.ParseLine(buf);
          // lines to ignore
          if (toks[0]==";VB98") continue;
          if (toks[0]==";TXT1") continue;
          if (toks[0]==";" && toks.size()==1) continue;
          // get rid of leading blank lines
          if (buf[0]=='\0' && !goflag) continue;
          // lines to correct
          if (toks[0]=="(null)")
            buf[0]='\0';
          if (toks[0]=="Colinearity" || toks[0]=="Collinearity")
            aftercollflag=1;
          if (toks.size()==3) {
            double rmul=strtod(toks[2]);
            if (isdigit(toks[0][0]) && rmul < 1.01 && rmul < 0.5)
              collinearitywarnings++;
          }
          outfile << buf << endl;
          goflag=1;
        }
        outfile << "</pre>" << endl;
        infofile.close();
        if (collinearitywarnings)
          outfile << "<font size=+1 color=red><b>Warning: " << collinearitywarnings
                  << " of your variables had Rmul values above 0.5</b></font>" << endl;
      }
    }
  }
}

void
boxit(ofstream &os,string text,string subtext)
{
  os << "<table border=1 cellspacing=0 marginwidth=0 width=100%>" << endl;

  os << "<tr bgcolor=#DODOFF><td><center><b><font size=+1>" << endl;
  os << text << endl;
  os << "</font></b></center></tr></td>" << endl;

  os << "<tr bgcolor=#DODOD0><td>" << endl;
  os << subtext << endl;
  os << "</tr></td>" << endl;

  os << "</table>" << endl;
}

void
VBQA::BuildDirList(string dir,int level)
{
  // first add it
  dirlist.push_back(dir);

  // if level>0, scan for children
  struct stat st;
  string entries=dir+"/*";
  if (level > 0) {
    vglob vg(entries);
    for (size_t i=0; i<vg.size(); i++) {
      if (stat(vg[i].c_str(),&st))
        continue;
      if (!S_ISDIR(st.st_mode))
        continue;
      if (vg[i][0]=='.')
        continue;
      BuildDirList(vg[i],level-1);
    }
  }
}

void
VBQA::ExamineTesFiles(vector<string> dirlist)
{
  vector<ProcPattern> pplist;
  size_t i,j,foundit;
  char tmp[STRINGLEN],fname[STRINGLEN];
  tokenlist args;
  vglob vg;

  for (i=0; i<dirlist.size(); i++)
    vg.append(dirlist[i]+"/*.tes");

  // if nothing to report on...
  if (!vg.size())
    return;

  for (i=0; i<vg.size(); i++) {
    Tes mytes;
    vector<string> mysteps;
    mytes.ReadFile(vg[i]);   // FIXME should be readheader probably
    // make a list of all the characteristics of mytes, start with
    // some basic header info
    sprintf(tmp,"dim(%d,%d,%d)",mytes.dimx,mytes.dimy,mytes.dimz);
    mysteps.push_back(tmp);
    sprintf(tmp,"voxsize(%.4f,%.4f,%.4f)",mytes.voxsize[0],mytes.voxsize[1],mytes.voxsize[2]);
    mysteps.push_back(tmp);
    
    // FIXME this loop detects common header tags that we should
    // match.  we can probably do a little better than hardcoding, if
    // we want.

    for (j=0; j<(int)mytes.header.size(); j++) {
      args.ParseLine(mytes.header[j]);
      if (!args.size()) continue;
      if (args[0]=="AcqCorrect:") {
        sprintf(tmp,"acqcorrect(%s,%s)",args[2].c_str(),args[3].c_str());
        mysteps.push_back(tmp);
      }
      if (args[0]=="TR(msecs):") {
        sprintf(tmp,"tr(%s)",args[1].c_str());
        mysteps.push_back(tmp);
      }
      if (args[0]=="PulseSeq:") {
        sprintf(tmp,"pulseseq(%s)",args[1].c_str());
        mysteps.push_back(tmp);
      }
      else if (args[0]=="ChangeOrient:") {
        sprintf(tmp,"orient(%s)",args[2].c_str());
        mysteps.push_back(tmp);
      }
      else if (args[0]=="Realigned" && args[1]=="by" && args[2]=="realign:")
        mysteps.push_back("realign");
      else if (args[0]=="realign_date")
        mysteps.push_back("realign");
      else if (args[0]=="Thresh_Abs:") {
        sprintf(tmp,"thresh(%s)",args[2].c_str());
        mysteps.push_back(tmp);
      }
      else if (args[0]=="Normalized" && args[1]=="by" && args[2]=="norm:")
        mysteps.push_back("norm");
      else if (args[0]=="SpatialSmooth:") {
        sprintf(tmp,"smooth(%s,%s,%s)",args[2].c_str(),args[3].c_str(),args[4].c_str());
        mysteps.push_back(tmp);
      }
    }
    foundit=0;
    for (j=0; j<(int)pplist.size(); j++) {
      if (pplist[j].steps==mysteps) {
	pplist[j].filelist.push_back(vg[i]);
	foundit=1;
	break;
      }
    }
    if (!foundit) {
      ProcPattern newpp;
      newpp.steps=mysteps;
      newpp.filelist.push_back(vg[i]);
      pplist.push_back(newpp);
    }
  }

  string subtitle="includes TES files from all directories specified";
  boxit(outfile,"TES File Consistency Check",subtitle);

  outfile << "<p>" << endl;
  outfile << "The TES files listed below have been grouped according to various" << endl;
  outfile << "characteristics, including their dimensions and some of the processing steps" << endl;
  outfile << "applied to them.  Files that have been grouped together" << endl;
  outfile << "are not guaranteed to have been processed identically, but files grouped separately" << endl;
  outfile << "almost certainly differ in some way." << endl;
  outfile << "So you might be concerned if you see any oddballs, files that should" << endl;
  outfile << "have been processed just like the rest but for some reason weren't." << endl;
  outfile << "Or if you half your subjects were processed one way, half another." << endl;
  outfile << "Note that this program only knows about certain things, mostly the standard" << endl;
  outfile << "set of VoxBo preprocessing steps.  So you still need to be careful, even." << endl;
  outfile << "if all your TES files end up in the expected number of groups." << endl;
  outfile << "</p>" << endl;

  // now print out the groups
  for (i=0; i<pplist.size(); i++) {
    outfile << "<p><table cols=2 border=1 marginwidth=0 cellspacing=0>" << endl;
    outfile << "<tr><td colspan=2 bgcolor=#D0D0FF><center><b>Group " << i+1 << "</b></center></td></tr>" << endl;
    outfile << "<tr><td bgcolor=#D0D0D0><center><b>Characteristics</b></center></td>" << endl;
    outfile << "<td bgcolor=#D0D0D0><center><b>Files</b></center></td></tr>" << endl;
    outfile << "<tr><td valign=top>" << endl;
    for (j=0; j<(int)pplist[i].steps.size(); j++) {
      outfile << pplist[i].steps[j] << "<br>" << endl;
    }
    outfile << "</td><td valign=top>" << endl;
    for (j=0; j<(int)pplist[i].filelist.size(); j++) {
      outfile << pplist[i].filelist[j] << "<br>" << endl;
    }
    outfile << "</td></tr></table></p>" << endl;
  }
  return;
}

// void
// LoadQAConf()
// {
//   // normpat.push_back(
//   // if no normpats, set nNorm.cub and nAnatomical.cub
// }

void
vbqa_help()
{
  printf("\nVoxBo vbqa (v%s)\n",vbversion.c_str());
  printf("summary:\n");
  printf("  vbqa produces voxbo quality assurance reports in html format.  Point\n");
  printf("  your web browser at HOME/.voxbo/reports/index.html to review your reports.\n");
  printf("usage:\n");
  printf("  vbqa [flags] <dir> ...\n");
  printf("flags:\n");
  printf("  -l <levels>      go so many levels deep (default 100)\n");
  printf("  -n <name>        use <name> for the report generated\n");
  printf("  -d [<name>...]   delete one or more reports\n");
  printf("\n");
}
