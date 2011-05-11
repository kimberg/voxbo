/* This program duplicates a certain db table. 
 * Note that we can do it by db utilities like this:
 * db_dump <original db file> | db_load <new db file>
 * but the new db file won't be able to keep the original key comparasion. 
 * So if records in original db file has numerical keys and are sorted by key values,
 * keys in the new db file will be sorted lexically (default key comparision). 
 * Before running this command, please also make sure the new db file does NOT exist,
 * otherwise the keys that already exists in destination db will be overwritten 
 * instead of being created from scratch.
 */

using namespace std;

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "mydb.h"

int main(int argc, char* argv[])
{
  if (argc != 4 && argc != 6) {
    cout << "Usage: cpDB <env dir> <source_db> <destination_db> [int | lex] [dup | nodup]" << endl;
    exit(1);
  }

  string envHome = argv[1];
  string dbname_src = argv[2];
  string dbname_dest = argv[3];
  mydb::key_comp cmp_method = mydb::cmp_int;
  bool allowDup = false;
  if (argc == 6) {
    if (strcmp(argv[4], "int") == 0) 
      cmp_method = mydb::cmp_int;
    else if (strcmp(argv[4], "lex") == 0)
      cmp_method = mydb::cmp_lex;
    else {
      cout << "Unknown argument: " << argv[4] << endl;
      cout << "Usage: cpDB <env dir> <source_db> <destination_db> [int | lex] [dup | nodup]" << endl;
      exit(1);
    }

    if (strcmp(argv[5], "dup") == 0) 
      allowDup = true;
    else if (strcmp(argv[5], "nodup") == 0)
      allowDup = false;
    else {
      cout << "Unknown argument: " << argv[5] << endl;
      cout << "Usage: cpDB <env dir> <source_db> <destination_db> [int | lex] [dup | nodup]" << endl;
      exit(1);
    }
  }

  // open env
  myEnv env(envHome);
  if (!env.isOpen())
    exit(1);
  // open source db in env
  mydb srcDB(dbname_src, env, cmp_method, allowDup);
  if (!srcDB.isOpen()) {
    //env.close();
    exit(1);
  }
  // open destination db in env
  mydb destDB(dbname_dest, env, cmp_method, allowDup);
  if (!destDB.isOpen()) {
    //env.close();
    exit(1);
  }

  Dbc *cursorp = NULL;
  srcDB.getDb().cursor(NULL, &cursorp, 0);
  Dbt key, data;
  int ret;
  DbTxn* txn = NULL;
  env.getEnv().txn_begin(NULL, &txn, 0);
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    destDB.getDb().put(txn, &key, &data, 0);      
  }
  cursorp->close();
  txn->commit(0);

  srcDB.close();
  destDB.close();
  env.close();

  return 0;
} 

