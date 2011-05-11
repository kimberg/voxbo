
// mydb.cpp
// Member functions in myEnv and mydb classes
// Copyright (c) 1998-2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Dongbo Hu

using namespace std;

#include "mydb.h"
#include "typedefs.h"
#include "vbutil.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>

/******************************************************
 * Member functions in myEnv class
 ******************************************************/
const uint32 myEnv::log_size = 10 * 1024 * 1024;

// Default ctor does nothing
myEnv::myEnv()
{
  env=NULL;
}

// Ctor that accepts env home directory
myEnv::myEnv(const string& envName)
{
  env=NULL;
  envDir=envName;
  open();
}

int myEnv::open(const string& envName)
{
  envDir = envName;
  return open();
}

/* Open db enviroment and a certain db.
 * Returns 0 if everything is ok;
 * returns an error code otherwise. */
int myEnv::open()
{
  env=new DbEnv(DB_CXX_NO_EXCEPTIONS);

  u_int32_t env_flags = DB_CREATE |  // If environment does not exist, create it.
    DB_INIT_LOCK  |                  // Initialize locking
    DB_INIT_LOG   |                  // Initialize logging
    DB_INIT_MPOOL |                  // Initialize the cache
    DB_INIT_TXN   |                  // Initialize transactions
    //DB_RECOVER    |
    DB_PRIVATE |                     // DYK: shared memory backed by heap memory, no __db.### files!
    DB_THREAD;                       // Feee-thread env handle

  env->set_error_stream(&cerr);                  // redirect dbg info
  env->set_encrypt("fyeo2006", DB_ENCRYPT_AES);  // FIXME hard-coded
                                                 // pass-phrase for
                                                 // encryption
  //env->log_set_config(DB_LOG_IN_MEMORY, 1);      // enable in-memory logging
  env->set_lg_bsize(log_size);                   // set in-memory log size

  int foobar = env->open(envDir.c_str(), env_flags, 0);
  if (foobar) {
    delete env;
    env=NULL;
    cout << "Error open db enviroment: " << envDir << endl;
    return foobar;
  }
  return 0;
}

/* Close db enviroment and a certain db.
 * Returns 0 if everything is ok;
 * returns an error code otherwise. */
int myEnv::close()
{
  if (!((bool)(*this)))
    return 0;
  if (env->close(0)) {
    cout << "Error closing db enviroment" << endl;
    return 111;
  }
  env=NULL;
  return 0;
}

/******************************************************
 * Member functions in mydb class
 ******************************************************/
// Default ctor 
mydb::mydb()
{
  dbp=NULL;
}

mydb::mydb(const string &dirname,const string &fname,uint32 flags)
{
  open(dirname+"/"+fname,NULL,mydb::cmp_int,flags);
}

// Ctor that builds a transactional db (requires db filename, env and
// some flags)
mydb::mydb(const string& fname,DbEnv *myenv,key_comp keyCompare,
           uint32 openflags,dup_flag dupsort, DBTYPE myType)
{
  open(fname,myenv,keyCompare,openflags,dupsort,myType);
}

// Initialize some flags before opening db w/o environment
void mydb::initxxx()
{
  // FIXME why is our init() function de-nulling the ptr?  is this
  // used anywhere?

  dbp = new Db(NULL, DB_CXX_NO_EXCEPTIONS); 
  dbp->set_error_stream(&std::cerr);
  dbp->set_encrypt("fyeo2006", DB_ENCRYPT_AES); // hard-coded pass-phrase
  dbp->set_flags(DB_ENCRYPT); // must be called AFTER set_encrypt()!!!
}

// Opens db in an environment.  This function should be called if db
// is contructed by default ctor.

int mydb::open(const string& fname,DbEnv *myenv,key_comp keyCompare,
               uint32 openflags,dup_flag dupsort,DBTYPE myType)
{
  filename=fname;
  dbp = new Db(myenv,DB_CXX_NO_EXCEPTIONS);
  dbp->set_error_stream(&std::cerr);
  // db-specified pass-phrase only allowed w/o env
  if (myenv==NULL)
    dbp->set_encrypt("fyeo2006", DB_ENCRYPT_AES);
  dbp->set_flags(DB_ENCRYPT);

  // duplicate key setup
  if (dupsort != no_dup) {
    dbp->set_flags(DB_DUPSORT);
    if (dupsort == sort_int)
      dbp->set_dup_compare(dupsort_int);
  }

  // If keyCompare is cmp_int and db is BTREE, compare keys by integer value.
  if (keyCompare == cmp_int && myType == DB_BTREE)
    dbp->set_bt_compare(compare_int);

  // always set auto_commit if there's an env
  if (myenv)
    openflags|=DB_AUTO_COMMIT;
  int foo = dbp->open(NULL, filename.c_str(),NULL,myType,openflags,0);
  if (foo) {
    delete dbp;
    dbp=NULL;
    // cout << "Error opening database file: " << filename << endl;
    return foo;
  }

  // stat = true; // db open is successful  
  return 0;
}

/* Member function that closes db, called from the class destructor. */
int mydb::close()
{
  // Do nothing if dbp is not open
  if (!dbp)
    return 0;

  // Close the db
  int foo =  dbp->close(0);
  if (foo) {
    cout << "Error closing db file: " << filename << endl;
    return foo;
  }
  delete dbp;
  dbp=NULL;

  return 0;
}

/* This function returns the number of records in database, 
 * used to generate keys automatically. */
int32 mydb::countRec()
{
  int32 length = 0;
  // Get a cursor to the patient db
  Dbc *cursorp;
  getDb().cursor(NULL, &cursorp, 0);
  Dbt key, data;
  while (cursorp->get(&key, &data, DB_NEXT) == 0)
    length++;

  cursorp->close();
  return length;
}

/* This function compares keys arithmetically instead of lexically.
 * It is called when the db key is an int32 integer and we want to 
 * rank the records according to the key value. 
 * This function is copied from bdb "Getting Started" doc. */
int compare_int(Db*, const Dbt* dbt1, const Dbt* dbt2)
{
  int32 l, r;
  memcpy(&l, dbt1->get_data(), sizeof(int32)); 
  memcpy(&r, dbt2->get_data(), sizeof(int32)); 

  if (ntohs(1) == 1) {
    swap(&l);
    swap(&r);
  }

  return l - r;
}

/* Another compare_int function that does exactly the same thing as the previous one,
 * but according to /usr/include/db_cxx.h, this definition is deprecated already. 
 * DB and DBT should be replaced by Db and Dbt. */
int compare_int_deprc(DB*, const DBT* dbt1, const DBT* dbt2)
{
  int32 l = *(int32*) dbt1->data;
  int32 r = *(int32*) dbt2->data;

  if (ntohs(1) == 1) {
    swap(&l);
    swap(&r);
  }

  return l - r;
}

/* This function compares keys arithmetically instead of lexically.
 * It is called when the db key is a string, but key/data pair should be sorted by 
 * numeric value of the int32 integer that is converted from this string.
 * Once again, note that DB and DBT are both deprecated and should be replaced by Db and Dbt. */
int compare_str(DB*, const DBT* dbt1, const DBT* dbt2)
{
  int32 l = atol((const char*) dbt1->data);
  int32 r = atol((const char*) dbt2->data);

  if (ntohs(1) == 1) {
    swap(&l);
    swap(&r);
  }

  return l - r;
}

/* This function is used to sort duplicate keys in db table.
 * It is almost identical to compare_int, except that this function 
 * puts data that has larger value before the one that has smaller value. */
int dupsort_int(Db*, const Dbt* dbt1, const Dbt* dbt2)
{
  int32 l, r;
  memcpy(&l, dbt1->get_data(), sizeof(int32)); 
  memcpy(&r, dbt2->get_data(), sizeof(int32)); 

  if (ntohs(1) == 1) {
    swap(&l);
    swap(&r);
  }

  return r - l;
}
