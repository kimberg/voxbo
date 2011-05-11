using namespace std;

#include <cstdlib>
#include <iostream>
#include "mydb.h"
#include "db_util.h"

int main(int argc, char* argv[])
{
  if (argc != 3) {
    cout << "Usage: delRecord <db filename> <record id>" << endl;
    exit(1);
  }

  string dbPath = argv[1];
  string envHome;
  string dbName;
  if (!parseEnvPath(dbPath, envHome, dbName)) {
    exit(1);
  }

  int32 id = atol(argv[2]);
  if (id <= 0) {
    cout << "Invalid ID: " << argv[2] << endl;
    exit(1);
  }
    
  myEnv env(envHome);
  if (!env.isOpen())
    exit(1);

  mydb currentDB(dbName, env);
  if (!currentDB.isOpen()) {
    env.close();
    exit(1);
  }

  DbTxn* txn = NULL;
  env.getEnv().txn_begin(NULL, &txn, 0);
  Dbt key(&id, sizeof(id));
  if (currentDB.getDb().del(txn, &key, 0) == 0) {
    cout << "Record " << id << " is deleted" << endl;
    txn->commit(0);
  }
  else {
    cout << "Failed to delete record ID " << id << endl;
    txn->abort();
  }

  currentDB.close();
  env.close();

  return 0;
}

