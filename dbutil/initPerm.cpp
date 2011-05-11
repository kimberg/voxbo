/* initPerm.cpp: populate permissions table from plain text file. 
 * NOTE: This program updates database WITHOUT db environment. */

#include "tokenlist.h"
#include "mydb.h"
#include "bdb_tab.h"
#include "db_util.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

string userDbName = "user.db";
string permDbName = "permissions.db";
string dirname;
bool envFlag;
myEnv env;
mydb permDB, userDB;

bool chkArg(int, char* argv[]);
bool setEnvDB();
bool setDB();
void addRecords(char*, DbTxn*);

/* Main function */
int main(int argc, char *argv[])
{
  if (!chkArg(argc, argv))
    exit(1);

  DbTxn* txn = NULL;
  if (envFlag) {
    if (!setEnvDB()) {
      userDB.close();
      permDB.close();
      env.close();
      exit(1);
    }
    env.getEnv().txn_begin(NULL, &txn, 0);    
  }
  else if (!setDB()) {
    userDB.close();
    permDB.close();
    exit(1);
  }

  addRecords(argv[1], txn);
  userDB.close();
  permDB.close();
  if (envFlag)
    env.close();

  return 0;
}

// Check input arguments on command line
bool chkArg(int argc, char* argv[])
{
  if (argc != 4) {
    printf("Usage: initPerm <txt_filename> [--env | --dir] <db_dir>\n");
    return false;
  }

  string optStr = argv[2];
  if (optStr == "--env")
    envFlag = true;
  else if (optStr == "--dir")
    envFlag = false;
  else {
    printf("Usage: initPerm <txt_filename> [--env | --dir] <db_dir>\n");
    return false;
  }

  const string inputFile = argv[1];
  if (!vb_fileexists(inputFile)) {  // Make sure input file exists
    cout << "File not exists: " << inputFile << endl;
    return false;
  }

  dirname = argv[3];
  if (!vb_direxists(dirname)) {  // Make sure db dir exists
    cout << "DB directory not exists: " << dirname << endl;
    return false;
  }

  if (dirname[dirname.size() - 1] != '/')
    dirname.append("/");  // don't let the trailing '/' mess up db file path

  // Remove original db file if it already exists
  if (vb_fileexists(dirname + permDbName)) {
    bool rmStat;
    if (envFlag) 
      rmStat = rmDB(dirname, permDbName); 
    else
      rmStat = rmDB(dirname + permDbName);

    if (!rmStat) {
      cout << "Failed to remove original db file: " << permDbName << endl;
      return false;
    }
  }

  return true;
}

// Open DBs in env
bool setEnvDB()
{
  if (env.open(dirname)) {
    cout << "Failed to open env" << endl;
    return false;
  }
  if (userDB.open(userDbName, env)) {
    cout << "Failed to open user db" << endl;
    return false;
  }
  if (permDB.open(permDbName, env, mydb::cmp_lex, true)) {
    cout << "Failed to open user db" << endl;
    return false;
  }
  return true;
}

// Open DBs w/o env
bool setDB()
{
  if (userDB.open(dirname + userDbName)) {
   cout << "db open error: " << dirname + permDbName << endl;
    return false;
  }

  if (permDB.open(dirname + permDbName, mydb::cmp_lex, mydb::sort_default, DB_CREATE)) {
    cout << "db open error: " << dirname + permDbName << endl;
    return false;
  }

  return true;
}

// Add records into db
void addRecords(char* txt_file, DbTxn* txn)
{
  FILE* ifp = fopen(txt_file, "r");
  // Make sure file is readable
  if (!ifp) {
    printf("File read error: %s\n", txt_file);
    return;
  }

  const int MAXLENGTH = 5000;
  char line[MAXLENGTH];
  int lineNo = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // if a line starts with "#", ignore the line
    if (strchr("#\n", line[0]))
      continue;

    // remove the trailing "return" key
    stripchars(line, "\n");
    const string tmpStr(line);
    tokenlist myRow;
    myRow.SetQuoteChars("\"");
    myRow.SetSeparator(" ");
    myRow.ParseLine(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 7
    if (myRow.size() != 3) {
      printf("%s line #%d: the number of fields is %d\n", txt_file, lineNo, myRow.size());
      continue;
    }

    string userName = myRow[0];
    userRec uRec;
    int32 stat = getUser(userDB, NULL, userName, uRec);
    if (stat < 0) {
      printf("%s line #%d: db error when getting user ID  of %s\n", txt_file, lineNo, userName.c_str());
      continue;
    }
    if (stat == 0) {
      printf("%s line #%d: user %s not exist\n", txt_file, lineNo, userName.c_str());
      continue;
    }
    string accessID = strnum(uRec.getID());
    string dataID = myRow[1];
    string permStr = myRow[2];
    if (permStr != "b" && permStr != "r" && permStr != "rw") {
      printf("%s line #%d: user %s not exist\n", txt_file, lineNo, userName.c_str());
      continue;
    }

    permRec pRec;
    pRec.setAccessID(accessID);
    pRec.setDataID(dataID);
    pRec.setPermission(permStr);
    if (addPerm(permDB, txn, pRec)) {
      printf("%s line #%d: failed to add new record into permission db\n", txt_file, lineNo);
      if (envFlag) {
	txn->abort();
	fclose(ifp);
	return;
      }
    }
  } 
  if (envFlag)
    txn->commit(0);
  permDB.getDb().sync(0);
 
  fclose(ifp);
}



