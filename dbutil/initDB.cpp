/******************************************************************************
 * initSystem.cpp: initialize system table 
 ******************************************************************************/
using namespace std;

#include <iostream>
#include <cstdlib>
#include "bdb_tab.h"
#include "dbdata.h"

/* Main function */
int main(int argc, char *argv[])
{
  if (argc != 4) {
    printf("Usage: initDB <directory> <admin_password> <start_id>\n");
    return 0;
  }

  string db_dir(argv[1]);
  if (!vb_direxists(db_dir)) {
    cout << "Directory not exist: " << argv[1] << endl;
    exit(1);
  }

  uint32 start_id = atol(argv[3]);
  if (start_id <= 0) {
    cout << "Invalid start ID: " << argv[3] << endl;
    exit(1);
  }

  DBdata new_dbs;
  if (new_dbs.initDB(db_dir, argv[2], start_id))
    cout << new_dbs.errMsg << endl;

  return 0;
}

