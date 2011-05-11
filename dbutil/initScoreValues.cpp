/* initScoreValues.cpp: populate patient score values table from plain text file. 
 * NOTE: This program updates database WITHOUT db environment! */

using namespace std;

#include "tokenlist.h"
#include "mydb.h"
#include "bdb_tab.h"
#include "db_util.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

void addRecords(char*, mydb&, mydb&);

/* Main function */
int main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("Usage: initScoreValues <txt_filename> <db_dir>\n");
    exit(0);
  }

  const string inputFile = argv[1];
  if (!vb_fileexists(inputFile)) {  // Make sure input file exists
    cout << "File not exists: " << inputFile << endl;
    exit(1);
  }

  string dirname = argv[2];
  if (!vb_direxists(dirname)) {  // Make sure db env dir exists
    cout << "DB env directory not exists: " << dirname << endl;
    exit(1);
  }

  if (dirname[dirname.size() - 1] != '/')
    dirname.append("/");  // don't let the trailing '/' mess up db file path
  string dbname = "scorevalues.db";
  string secd_dbname = "patientscores.sdb";
  string sysname = "system.db";
  // Remove original db file if it already exists in env dir
  if (vb_fileexists(dirname + dbname)) {
    if (!rmDB(dirname + dbname)) {  
      cout << "Faild to remove original db file: " << dbname << endl;
      exit(1);
    }
  }

  // open db in non-transaction mode
  mydb primDB(dirname + dbname, mydb::cmp_lex);
  if (!primDB.isOpen()) {
    cout << "db open error: " << dirname + dbname << endl;
    exit(1);
  }
  mydb secdDB(dirname + secd_dbname, mydb::cmp_int, mydb::sort_default);
  if (!secdDB.isOpen()) {
    cout << "db open error: " << dirname + secd_dbname << endl;
    primDB.close();
    exit(1);
  }
  primDB.getDb().associate(NULL, &secdDB.getDb(), getPID, 0);

  mydb sysDB(dirname + sysname, mydb::cmp_lex);
  if (!sysDB.isOpen()) {
    cout << "db open error: " << dirname + sysname << endl;
    secdDB.close();
    primDB.close();
    exit(1);
  }

  addRecords(argv[1], primDB, sysDB);
  secdDB.close();
  primDB.close();
  sysDB.close();

  return 0;
}


// Add records into db
void addRecords(char* txt_file, mydb& inputDB, mydb& sysDB)
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
  time_t t_stamp = time(NULL);
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

    int32 pID = atoi(myRow(0));
    if (pID <= 0) {
      printf("%s line #%d: invalid patient ID %s\n", txt_file, lineNo, myRow(0));
      continue;
    }
    // dhu: should we also verify that this patient ID exists in patient table?
 
    DBscorevalue svData;
    svData.patient = pID;
    svData.datatype = "string";
    // dhu: should we also verify this score name exists in score name table?
    svData.scorename = myRow[1];
    svData.v_string = myRow[2];
    svData.setby = "admin";
    svData.whenset.setUnixTime(t_stamp);
    
    if (addScoreValue(inputDB, sysDB, NULL, svData)) {
      printf("%s line #%d: failed to add record into score value db\n", txt_file, lineNo);
    }
    inputDB.getDb().sync(0);
  } 

  string statMsg;
  if (setSysUpdate(sysDB, NULL, t_stamp, statMsg))
    cout << statMsg << endl;

  fclose(ifp);
}

