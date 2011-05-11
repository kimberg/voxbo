/******************************************************************************
 * showSystem.cpp: read all records in system table 
 ******************************************************************************/
using namespace std;

#include "db_util.h"
#include "bdb_tab.h"
#include <iostream>
#include <cstdlib>

/* Main function */
int main(int argc, char* argv[])
{
  if (argc > 2) {
    printf("Usage: showSystem <bdb_filename>\n");
    printf("Default system db file is: ../env/system.db");
    return 0;
  }

  string dbFile;
  if (argc == 1)
    dbFile = "../env/system.db";
  else
    dbFile = argv[1];

  showEnvDB<sysRec>(dbFile, mydb::cmp_lex);

  return 0;
}

