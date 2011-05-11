
// Make a secondary db table, base on db1.db

using namespace std;

#include "db_util.h"
#include "bdb_tab.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[])
{
  string patientScoreDbName, scoreValueDbName;
  if (argc == 1) {
    patientScoreDbName = "../env/patientscores.sdb";
    scoreValueDbName = "../env/scorevalues.db";

  }
  else if (argc == 3) {
    patientScoreDbName = argv[1];
    scoreValueDbName = argv[2];
  }
  else {
    printf("Usage: showPatientScores <secondary patient score db name> <primary score value db name>\n");
    printf("Default db files are: ../env/patientscores.sdb and ../env/scorevalues.db");
    return 0;
  }

  // open primary db
  mydb scoreValueDB(scoreValueDbName, mydb::cmp_lex);
  mydb patientScoreDB(patientScoreDbName, mydb::cmp_int, mydb::sort_default);

  // associate primary and secondary db
  scoreValueDB.getDb().associate(NULL, &patientScoreDB.getDb(), getPID, 0);  

  // iterate secondary db
  Dbc *cursorp = NULL;
  patientScoreDB.getDb().cursor(NULL, &cursorp, 0);
  Dbt skey, pdata;
  int ret;
  while ((ret = cursorp->get(&skey, &pdata, DB_NEXT)) == 0) {
    DBscorevalue tmpRec(pdata.get_data());
    tmpRec.show();
  }
  cursorp->close();


  return 0;
} 
