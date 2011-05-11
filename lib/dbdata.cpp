
// dbdata.cpp
// Member function definitions of DBdata
// Copyright (c) 2007-2010 by The VoxBo Development Team

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

#include "dbdata.h"
#include "db_util.h"
#include <fstream>

/* Hard-code table names
 * If db tables are created in a transactional environment, inputDir is blank;
 * If no environment is available, create them as non-transactional table. */
void DBdata::setDbNames(string inputDir)
{
  // Append "/" at the end of inputDir to make sure path is correct
  if (inputDir.length() && inputDir[inputDir.length() - 1] != '/')
    inputDir.append("/");

  sysDbName = inputDir + "system.db";
  userDbName = inputDir + "user.db";
  scoreNameDbName = inputDir + "scorenames.db";
  scoreValueDbName = inputDir + "scorevalues.db";
  patientScoreDbName = inputDir + "patientscores.sdb";
  sessionDbName = inputDir + "session.db";
  permissionDbName = inputDir + "permissions.db";
  regionDbName = inputDir + "region_name.db";
  synonymDbName = inputDir + "synonym.db";
  regionRelationDbName = inputDir + "region_relation.db";
  namespaceDbName = inputDir + "namespace.db";
  patientDbName = inputDir + "patient.db";
  patientListDbName = inputDir + "patientlist.db";
}

/* Open enviroment and databases files.
 * Returns 0 if db env is already open or everything is ok;
 * returns -1 for db enviroment open errors;
 * returns -2 for any global DBs open errors; */
int DBdata::open(string dir)
{
  if (!dir.empty()) dirname=dir;
  // use abs path because files are continually re-opened, and someone
  // might chdir() on us
  dirname=xabsolutepath(dirname);
  if (env.open(dirname)) {
    errMsg = "Failed to open DB env";
    return -1;
  }
  
  // initialize some stuff
  setDbNames();
  typemap.clear();
  scorenames.clear();
  scorenamechildren.clear();
  viewspecs.clear();
  regionNameMap.clear();
  synonymMap.clear();
  regionRelationMap.clear();

  // Open system table, keys ranked lexically
  sysDB.open(sysDbName, env.getEnv(), mydb::cmp_lex);
  // Open user table
  userDB.open(userDbName, env.getEnv());
  // Open permission table: keys ranked lexically, allow duplicate keys 
  permDB.open(permissionDbName, env.getEnv(), mydb::cmp_lex, mydb::sort_default);
  // Open score name table: keys ranked lexically, score name ID no longer exists
  scoreNameDB.open(scoreNameDbName, env.getEnv(), mydb::cmp_lex);
  // Open session table
  sessionDB.open(sessionDbName, env.getEnv());
  // Open patient table
  patientDB.open(patientDbName, env.getEnv());
  // Open patient list table
  patientListDB.open(patientListDbName, env.getEnv());
  // Open score value table: keys ranked lexically
  scoreValueDB.open(scoreValueDbName, env.getEnv(), mydb::cmp_lex);
  // Open patient score value table (secondary sb based on score value table): 
  // keys ranked numerically, duplicate keys allowed, default sort method used
  patientScoreDB.open(patientScoreDbName, env.getEnv(),mydb::cmp_int,mydb::sort_default);

  // make sure it all opened
  if (!sysDB || !userDB || !permDB || !scoreNameDB || !sessionDB || !patientDB
      || !patientListDB || ! scoreValueDB || !patientScoreDB) {
    errMsg="one or more db files not opened";
    return -2;
  }

  // Associate secondary db with primary db
  scoreValueDB.getDb().associate(NULL, &patientScoreDB.getDb(), getPID, 0);

  int err;
  err = readTypes(dirname+"/types.txt");
  if (err) {
    errMsg = "could not load types.txt, error code: " + strnum(err);
    close();
    return -2;
  }
  err = readViews(dirname+"/views.txt");
  if (err) {
    errMsg = "could not load views,txt, error code: " + strnum(err);
    close();
    return -2;
  }
  err = readScorenames(dirname+"/scorenames.txt");
  if (err) {
    errMsg = "could not load scorenames.txt, error code: " + strnum(err);
    close();
    return -2;
  }

  if (loadRegionData()) {
    close();
    return -3;
  }

  return 0;
}

/* This function closes all open DBs and finally the db enviroment. 
 * Returns 0 if everything is ok;
 * returns -1 for db env close error;
 * returns -2 for any db close error. */
int
DBdata::close()
{
  int errs=0;
  errs+=closeTables();
  errs+=env.close();
  return errs;
}

// closeTables() returns number of tables not closed successfully (0
// on success)
int DBdata::closeTables()
{
  int err=0;
  if (sysDB.close()) err++;
  if (userDB.close()) err++;
  if (permDB.close()) err++;
  if (scoreNameDB.close()) err++;
  if (sessionDB.close()) err++;
  if (patientDB.close()) err++;
  if (patientListDB.close()) err++;
  if (patientScoreDB.close()) err++;
  if (scoreValueDB.close()) err++;
  // close brain region related tables
  if (regionNameDB.close()) err++;
  if (synonymDB.close()) err++;
  if (regionRelationDB.close()) err++;
  if (namespaceDB.close()) err++;
  if (err)
    cout << "FIXME DEBUG error closing something\n";

  return err;
}

/* Set maps on server side. 
 * Returns 0 if everything is ok;
 * returns -1 if any global map can not be loaded successfully; */
int DBdata::loadRegionData()
{
  // open namespace db (not used right now)
  if (namespaceDB.open(namespaceDbName, env.getEnv(), mydb::cmp_lex)) {
    errMsg = "Failed to open brain region namespace db";
    return -2;
  }
  
  // set brain region name map
  Dbc *cursorp = NULL;
  Dbt key, data;
  int ret;
  if (regionNameDB.open(regionDbName, env.getEnv())) {
    errMsg = "Failed to open region name db";
    return -2;
  }

  if (regionNameDB.getDb().cursor(NULL, &cursorp, 0)) { // validate cursor
    errMsg = "Invalid region name db cursor";
    regionNameDB.close();
    return -1;    // validate cursor
  }

  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    regionRec rData(data.get_data());
    regionNameMap[rData.getID()] = rData;
  }
  cursorp->close();
  regionNameDB.close();
  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND) {
    errMsg = "Region name record not found";
    return -1;
  }
  
  // set brain region synoyms map
  if (synonymDB.open(synonymDbName, env.getEnv())) {
    errMsg = "Failed to open synonym db";
    return -2;
  }
  if (synonymDB.getDb().cursor(NULL, &cursorp, 0)) { // validate cursor
    errMsg = "Invalid synonym db cursor";
    synonymDB.close();
    return -1;
  }
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    synonymRec synData(data.get_data());
    synonymMap[synData.getID()] = synData;
  }
  cursorp->close();
  synonymDB.close();
  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND) {
    errMsg = "Synonym record not found";
    return -1;
  }

  // set brain region relationship map
  if (regionRelationDB.open(regionRelationDbName, env.getEnv())) {
    errMsg = "Failed to open region relationship db";
    return -2;
  }
  if (regionRelationDB.getDb().cursor(NULL, &cursorp, 0)) { // validate cursor
    errMsg = "Invalid region relationship db cursor";
    regionRelationDB.close();
    return -1;
  }
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    regionRelationRec rrData(data.get_data());
    regionRelationMap[rrData.getID()] = rrData;
  }
  cursorp->close();
  regionRelationDB.close();
  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND) {
    errMsg = "Region relationship record not found";
    return -1;
  }

  return 0;
}

// Read type txt file and set type map, written by Dan
int DBdata::readTypes(string fname)
{
  const int MAXLEN = 4096;
  FILE * ifp = fopen(fname.c_str(), "r");
  if (!ifp) {
    printf("%s not available for reading.", fname.c_str());
    return -1;
  }

  char line[MAXLEN];
  int32 lineNo = 0;
  while (fgets(line,MAXLEN,ifp)) {
    lineNo++;
    tokenlist toks;
    toks.ParseLine(line);
    if (toks.size()==0) continue;
    if (toks[0][0]=='#') continue;
    if (toks[0][0]=='%') continue;
    if (toks[0][0]=='!') continue;
    if (toks[0][0]==';') continue;
    // Print out error message if the line is not blank but the number of field is not 2
    if (toks.size()==1) {
      printf("%s line #%d: invalid type specification\n", fname.c_str(), lineNo);
      // FIXME we can continue with an invalid line
      // fclose(ifp); return -2;
    }
    typemap[toks[0]].name = toks[0];
    if (toks[1]=="description" && toks.size()>2)
      typemap[toks[0]].description=toks.Tail(2);
    else
      typemap[toks[0]].values.push_back(toks[1]);
  }
  fclose(ifp);
  return 0;
}

int
DBdata::readScorenames(string fname)
{
  FILE *fp;
  tokenlist toks;
  char buf[1024];
  if ((fp=fopen(fname.c_str(),"r"))==NULL)
    return -1;
  while (fgets(buf,1023,fp)) {
    toks.ParseLine(buf);
    if (toks.size()<2) continue;
    if (toks[0][0]=='#') continue;
    if (toks[0][0]=='%') continue;
    if (toks[0][0]=='!') continue;
    if (toks[0][0]==';') continue;
    DBscorename si;
    si.flags["customizable"]="1";
    si.flags["leaf"]="1";
    si.name=si.screen_name=toks[0];  // FIXME eventually we can have screen name be separate
    si.datatype=toks[1];
    string parentname=scoreparent(si.name);
    // FIXME commented out the condition below, we can use children of "" to find *TESTS*  ok?
    // map parent's id onto our id if there's a parent
    // if (parentname!="")
      scorenamechildren.insert(pair<string,string>(parentname,si.name));
    // map name to id
    // set flags
    if (si.datatype=="stub")
      si.flags.erase("leaf");
    for (size_t i=2; i<toks.size(); i++) {
      if (toks[i]=="repeating")
	si.flags["repeating"]="1";
      else if (toks[i]=="searchable")
	si.flags["searchable"]="1";
      // if we don't recognize it, assume it takes one argument
      else {
	si.flags[toks[i]]=toks[i+1];
	i++;
      }
    }
    // add it to the main list
    scorenames[si.name]=si;
  }
  fclose(fp);

  return 0;
}


// Read view spec txt file and set a map, written by Dan
// view file includes the following line types:
// myview x x x x x x   [add all the stuff as an entry to myview]
// myview newtab        [add a new tab to myview]

int
DBdata::readViews(string fname)
{
  const int MAXLEN = 4096;
  FILE * ifp = fopen(fname.c_str(), "r");
  if (!ifp) {
    printf("%s not available for reading.",fname.c_str());
    return -1;
  }

  char line[MAXLEN];
  int32 lineNo = 0;
  while (fgets(line,MAXLEN,ifp)) {
    lineNo++;
    tokenlist toks;
    toks.ParseLine(line);
    if (toks.size()==0) continue;
    if (toks[0][0]=='#') continue;
    if (toks[0][0]=='%') continue;
    if (toks[0][0]=='!') continue;
    if (toks[0][0]==';') continue;
    // Print out error message if the line is not blank but the number of field is not >=2
    if (toks.size()<2) {
      printf("%s line #%d: invalid view specification\n",fname.c_str(), lineNo);
      // FIXME we can continue with an invalid line
      // fclose(ifp); return -2;
    }
    viewspecs[toks[0]].name = toks[0];
    viewspecs[toks[0]].entries.push_back(toks.Tail());
  }
  fclose(ifp);
  return 0;
}

/* This function collects all score name information and 
 * put them into an array of DBscorename object. 
 * returns 0 if everything is ok;
 * or returns -1 for db errors. */
// int DBdata::getScoreNames(DbTxn* txn)
// {
//   Dbc *cursorp = NULL;
//   if (scoreNameDB.getDb().cursor(txn, &cursorp, 0)) { // validate curosr
//     errMsg = "Invalid score name db cursor";
//     return -1;
//   }

//   Dbt key, data;
//   int ret;
//   int status = 0;
//   while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
//     scoreNameRec sData(data.get_data());
//     DBscorename tmp_score(sData);
//     add_scorename(tmp_score);
//   }

//   // Returns -1 if ret is non-zero and it doesn't reach the end of table 
//   if (ret && ret != DB_NOTFOUND) {
//     errMsg = "Score name record not found";
//     status = -1;
//   }

//   cursorp->close();
//   return status;
// }

// Add a DBscorename record into scorename maps, written by Dan
void DBdata::add_scorename(const DBscorename &sn)
{
  // add to the master map of scorename ids to scorenames
  scorenames[sn.name]=sn;
  // add to the master map of parents to children
  scorenamechildren.insert(pair<string,string>(scoreparent(sn.name),sn.name));
  // if the parent is zero, it's a "test" -- add to map of test names to ids
  //   if (sn.parentname.size()==0)
  //     testmap[sn.name]=sn.id;
}


/* Utility function written by Dan. */
void DBdata::print_types()
{
  map<string, DBtype>::iterator ti;
  for (ti=typemap.begin(); ti!=typemap.end(); ti++) {
    printf ("TYPE %s\n",ti->first.c_str());
    vector<string>::iterator vi;
    for (vi=ti->second.values.begin(); vi!=ti->second.values.end(); vi++)
      printf("     | %s\n",vi->c_str());
  }
}


// FIXME if one db isn't created, the rest aren't closed properly
/* Initialize tables without envirionment/transaction. 
 * Returns 0 if everything is ok;
 * returns -1 if any db file already exists;
 * returns -2 if any db can not be created; 
 * returns -3 for db close errors;
 * returns -4 for any other db initialization error.*/
int DBdata::initDB(string inputDir, string admin_passwd, uint32 id_start)
{
  dirname=xabsolutepath(inputDir);
  if (mkdir(inputDir.c_str(),0700))
    return -1;
  setDbNames(inputDir);

  // Open system table, keys ranked lexically
  if (sysDB.open(sysDbName,NULL,mydb::cmp_lex,DB_CREATE)) { 
    errMsg = "Failed to open system db";
    return -2;
  }
  // Add two records into system table
  for (int i = 0; i < 2; i++) {
    sysRec sData;
    if (i == 0) {
      sData.setName("Next Unique ID");
      sData.setValue(strnum(id_start + 1));  // id_start will be reserved for admin's user ID
    }
    else {
      sData.setName("last_updated");
      sData.setValue(strnum(time(NULL)));
    }

    Dbt key((char*) sData.getName().c_str(), sData.getName().length() + 1);
    int32 size = sData.getSize();
    char* buff = new char[size];
    sData.serialize(buff);
    Dbt data(buff, size);
    if (sysDB.getDb().put(NULL, &key, &data, 0)) {
      errMsg = string("Failed to add ") + sData.getName();
      delete [] buff;
      return -4;
    }
    delete [] buff;
  }

  // Open user table
  if (userDB.open(userDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open user db";
    return -2;
  }
  // Add admin account and passwd into user table
  userRec dbUser;
  dbUser.setID(id_start);
  dbUser.setAccount("admin");
  dbUser.setName("Database Administrator");
  if (dbUser.gen_salt_and_verifier(admin_passwd)) {
    errMsg = "Failed to make verifier for admin account";
    return -4;
  }
  int32 bufLen = dbUser.getSize();
  char buff[bufLen];
  dbUser.serialize(buff);
  Dbt key(buff, sizeof(int32));
  Dbt data(buff, bufLen);
  if (userDB.getDb().put(NULL, &key, &data, 0)) {
    errMsg = "Failed to add admin into user table";
    return -4;
  }

  // Open permission table: keys ranked lexically, allow duplicate keys 
  if (permDB.open(permissionDbName,NULL,mydb::cmp_lex,DB_CREATE,mydb::sort_default)) {
    errMsg = "Failed to open permission db";
    return -2;
  }
  // Grant full permissions to admin
  permRec pRec;
  pRec.setAccessID(strnum(id_start));
  pRec.setDataID("*");
  pRec.setPermission("rw");
  if (addPerm(permDB, NULL, pRec)) {
    errMsg = "Failed to add admin permission record";
    return -4;
  }
  
  // Open score name table: keys ranked lexically, score name ID no longer exists
  if (scoreNameDB.open(scoreNameDbName,NULL,mydb::cmp_lex,DB_CREATE)) { 
    errMsg = "Failed to open score name db";
    return -2;
  }
  // Open session table
  if (sessionDB.open(sessionDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open session db";
    return -2;
  }
  // Open patient table
  if (patientDB.open(patientDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open patient db";
    return -2;
  }
  // Open patient list table
  if (patientListDB.open(patientListDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open patient list db";
    return -2;
  }
  // Open score value table: keys ranked lexically
  if (scoreValueDB.open(scoreValueDbName,NULL,mydb::cmp_lex,DB_CREATE)) {
    errMsg = "Failed to open score value db";
    return -2;
  }
  // Open patient score value table (secondary sb based on score value table): 
  // keys ranked numerically, duplicate keys allowed, default sort method used
  if (patientScoreDB.open(patientScoreDbName,NULL,mydb::cmp_int,
                          DB_CREATE,mydb::sort_default)) {
    errMsg = "Failed to open patient score value db";
    return -2;
  }
  // Associate secondary db with primary db
  scoreValueDB.getDb().associate(NULL, &patientScoreDB.getDb(), getPID, 0);

  // brain region related tables
  if (regionNameDB.open(regionDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open region name db";
    return -2;
  }
  if (synonymDB.open(synonymDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open synonym db";
    return -2;
  }
  if (regionRelationDB.open(regionRelationDbName,NULL,mydb::cmp_int,DB_CREATE)) {
    errMsg = "Failed to open region relationship db";
    return -2;
  }
  if (namespaceDB.open(namespaceDbName,NULL,mydb::cmp_lex,DB_CREATE)) {
    errMsg = "Failed to open brain region namespace db";
    return -2;
  }

  // close all tables now
  if (closeTables())
    return -3;

  // create nearly empty types, views, and scorenames files
  ofstream ofile;
  ofile.open((dirname+"/types.txt").c_str(),ios::out);
  if (!ofile) {
    errMsg = "failed to create types.txt";
    return -2;
  }
  ofile << "gender description gender" << endl;
  ofile << "gender male" << endl;
  ofile << "gender female" << endl;
  ofile << "gender other" << endl;
  ofile.close();

  ofile.open((dirname+"/scorenames.txt").c_str(),ios::out);
  if (!ofile) {
    errMsg = "failed to create scorenames.txt";
    return -2;
  }
  ofile << "demographics stub" << endl;
  ofile << "demographics:firstname shortstring" << endl;
  ofile << "demographics:lastname shortstring" << endl;
  ofile << "demographics:DOB date" << endl;
  ofile.close();

  ofile.open((dirname+"/views.txt").c_str(),ios::out);
  if (!ofile) {
    errMsg = "failed to create views.txt";
    return -2;
  }
  ofile << "patient newtab Patient Summary" << endl;
  ofile << "patient banner todaysdate space username" << endl;
  ofile << "patient sessionlist" << endl;
  ofile << "patient testlist" << endl;
  ofile << "# here is a new patient view" << endl;
  ofile << "newpatient banner todaysdate space username" << endl;
  ofile << "newpatient test demographics" << endl;
  ofile.close();

  return 0;
}



void
DBdata::dumpscorevalues()
{
  if (!scoreValueDB) return;
  Dbc *c=NULL;
  if (scoreValueDB.getDb().cursor(NULL,&c,0)) {
    cout << "dumpscorevalues(): couldn't get a cursor\n";
    return;
  }
  Dbt key,data;
  int ret;
  while ((ret=c->get(&key,&data,DB_NEXT))==0) {
    void *buf=data.get_data();
    DBscorevalue sv;
    sv.deserialize(buf);
    cout << format("  pt=%d svid=%d parent=%05d sid=%05d idx=%d %s=%s dt=%s setby %s\n")%
      sv.patient%sv.id%sv.parentid%sv.sessionid%sv.index%sv.scorename%(sv.printable(1))%
      sv.datatype%sv.setby;
  }
}

void
DBdata::dumpsys()
{
  if (!sysDB) return;
}

void
DBdata::dumppatients()
{
  if (!patientDB) return;
}
