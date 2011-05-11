// Rename a db file in environment

using namespace std;

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "mydb.h"
#include "db_util.h"

int main(int argc, char* argv[])
{
  if (argc != 3) {
    cout << "Usage: mvDB <original_name> <new_dbname>" << endl;
    exit(1);
  }

  string dbPath = argv[1];
  const char* new_name = argv[2];
  string envHome, dbName;
  if (!parseEnvPath(dbPath, envHome, dbName)) {
    cout << "Invalid db path: " << dbPath << endl;
    exit(1);
  }

  // open env
  myEnv env(envHome);
  if (!env.isOpen()) {
    cout << "Failed to open db env" << endl;
    exit(1);
  }

  DbTxn* txn = NULL;
  env.getEnv().txn_begin(NULL, &txn, 0);
  if (env.getEnv().dbrename(txn, dbName.c_str(), NULL, new_name, 0)) {
    cout << "Faield to rename db file" << endl;    
    txn->abort();
  }
  else
    txn->commit(0);

  env.close();
  return 0;
} 

