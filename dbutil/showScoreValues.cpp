/******************************************************************************
 * Print out all records in score value (test results)  table 
 ******************************************************************************/
using namespace std;

#include "db_util.h"
#include "bdb_tab.h"
#include <iostream>
#include <cstdlib>
//#include <QtGui/QApplication>

/* Main function */
int main(int argc, char *argv[])
{
  if (argc > 2) {
    printf("Usage: showScoreValues <bdb_filename>\n");
    printf("Default score value db file is: ../env/scorevalues.db");
    return 0;
  }

  string dbFile;
  if (argc == 1)
    dbFile = "../env/scorevalues.db";
  else
    dbFile = argv[1];

  showEnvDB<DBscorevalue>(dbFile, mydb::cmp_lex);

  return 0;
}

