/******************************************************************************
 * Print out all records in score name table 
 ******************************************************************************/
using namespace std;

#include "db_util.h"
#include "bdb_tab.h"
#include <iostream>
#include <cstdlib>

/* Main function */
int main(int argc, char *argv[])
{
  if (argc > 2) {
    printf("Usage: showScoreNames <bdb_filename>\n");
    printf("Default score name db file is: ../env/scorenames.db");
    return 0;
  }

  string dbFile;
  if (argc == 1)
    dbFile = "../env/scorenames.db";
  else
    dbFile = argv[1];

  showEnvDB<scoreNameRec>(dbFile);

  return 0;
}

