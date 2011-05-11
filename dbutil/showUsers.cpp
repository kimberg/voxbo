/******************************************************************************
 * Print out all records in user table
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
    printf("Usage: showUsers <bdb_filename>\n");
    printf("Default user db file is: ../env/user.db");
    return 0;
  }

  string dbFile;
  if (argc == 1)
    dbFile = "../env/user.db";
  else
    dbFile = argv[1];

  showEnvDB<userRec>(dbFile);

  return 0;
}

