
#include <QApplication>
#include <QImageReader>

#include <vector>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>

#include "mydefs.h"
#include "dbview.h"
#include "vbview.h"
#include "dbmainwindow.h"
#include <boost/format.hpp>

using namespace std;
using boost::format;

//void read_types();
//void read_scorenames();
void read_scoredata(DBpatient &pt);

// GLOBALS for this test program
int f_writable=0;
localClient client;


Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
Q_IMPORT_PLUGIN(qtiff)

int
main(int argc,char **argv)
{
   QList<QByteArray> foo=QImageReader::supportedImageFormats();
   vbforeach (QByteArray &xx,foo)
     cout << xx.constData() << endl;
 
  // parse command line args for testing purposes
  tokenlist args;
  args.Transfer(argc-1,argv+1);
  for (int i=0; i<args.size(); i++) {
    if (args[i]=="edit")
      f_writable=1;
  }

  // print_scorenames();

  // BOGUS QT APPLICATION FOR TESTING
  QApplication app(argc,argv);


  DBmainwindow mw;
  mw.show();
  app.exec();
  exit(0);


  // load some bogus config files for testing
  client.dbs.readViews("../env/views.txt");
  client.dbs.readTypes("../env/types.txt");
  client.dbs.readScorenames("../env/scorenames.txt");
  
  // structures for this instance
  DBpatient patient;
  read_scoredata(patient);
  DBview *view=new DBview(&client,patient);

  view->layout_view("test");

  // view->layout_view("newpatient");
  // view->layout_view("oldpatient");
  app.exec();
  exit(0);
}

void
read_scoredata(DBpatient &pt)
{
  FILE *fp;
  tokenlist toks;
  char buf[1024];
  int sessionid=0;
  if ((fp=fopen("patientdata.txt","r"))==NULL)
    return;
  while (fgets(buf,1023,fp)) {
    toks.ParseLine(buf);
    if (toks[0][0]=='#') continue;
    if (toks[0][0]=='%') continue;
    if (toks[0][0]=='!') continue;
    if (toks[0][0]==';') continue;

    if (toks[0]=="session") {
      sessionid=strtol(toks[1]);
      continue;
    }

    // at this point, must be a score line
    if (toks.size()!=4) continue;

    DBscorevalue sv;
    sv.id=strtol(toks(1),NULL,10);
    sv.parentid=strtol(toks(2),NULL,10);
    sv.scorename=toks[0];
    // find datatype via scorename
    if (client.dbs.scorenames.count(sv.scorename)) {
      DBscorename &sn=client.dbs.scorenames[sv.scorename];
      sv.datatype=sn.datatype;
    }
    else {
      cout << "unknown scorename " << sv.scorename << endl;
      sv.datatype="string";
    }
    // all data in test file are strings or dates
    //cout << " sname: " << sv.scorename << endl;
    //cout << "  type: " << sv.datatype << endl;
    //cout << "  tok3: " << toks[3] << endl;
    if(toks[3].size()) {
      char tmps[toks[3].size()+1];
      strcpy(tmps,toks(3));
      //cout << "  tmps: " << tmps << endl;
      sv.deserialize((uint8 *)tmps,strlen(tmps)+1);
    }
    //sv.v_string=toks[3];
    //cout << "string: " << sv.v_string << endl;
    //cout << "  date: " << sv.v_date.getDateStr() << endl << endl;
    
    // etc.
    sv.sessionid=sessionid;
    if (f_writable)
      sv.permission="rw";
    else
      sv.permission="r";
    // map id to the actual score
    pt.scores[sv.id]=sv;
    // add to parent-children multimap for this patient's scores
    pt.children.insert(pair<int32,int32>(sv.parentid,sv.id));
    // multimap score name id (e.g., id of "scan:type") to instances
    pt.names.insert(pair<string,int32>(sv.scorename,sv.id));
    // multimap session ids to tests
    if (sv.parentid==0)
      pt.sessiontests.insert(pair<int32,int32>(sv.sessionid,sv.id));
  }
  fclose(fp);
}
