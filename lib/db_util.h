
// db_util.h
// generic functions for record exportation
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

#ifndef DB_UTIL_H
#define DB_UTIL_H

#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include "typedefs.h"
#include "tokenlist.h"
#include "mydefs.h"
#include "brain_tab.h"
#include "mydb.h"

#define MSG_SIZE 1024

// User information class defined for GUI on client side
// FIXME: DYK: this should probably be phased out in favor of userRec

class userGUI
{
 public:
  userGUI();
  userGUI(void *); 
  void clear();
  void serialize(char *) const;
  int32 getSize() const;
  void show() const;

  inline void setAccount(string inputStr) { account = inputStr; }
  inline void setPasswd(string inputStr) { passwd = inputStr; }
  inline void setName(string inputStr) { name = inputStr; }
  inline void setPhone(string inputStr) { phone = inputStr; }
  inline void setEmail(string inputStr) { email = inputStr; }
  inline void setAddress(string inputStr) { address = inputStr; }
  inline void setGroups(string inputStr) { groups = inputStr; }

  inline string getAccount() const { return  account; }
  inline string getPasswd() const { return  passwd; }
  inline string getName() const { return  name; }
  inline string getPhone() const { return  phone; }
  inline string getEmail() const { return  email; }
  inline string getAddress() const { return  address; }
  inline string getGroups() const { return  groups; }  

 private:
  string account, passwd, name; 
  string phone, email, address, groups;
};

/* This class holds a patient's certain score 
 * information collected from all related tables. */
class patientMatch
{
 public:
  patientMatch();
  patientMatch(int32, const string&, const string&);
  patientMatch(void *);
  void clear();
  void serialize(char *) const;
  int32 getSize() const;
  void addScore(const string&, const string&);
  void show() const;

  int32 patientID;
  map<string, string> scoreMap; 
};

// Another simple class used for patient search
class patientSearchTags {
 public:
  void init();
  string getStr() const;

  bool case_sensitive;
  string scoreName;
  string relationship, searchStr;
  vector<int32> patientIDs;
  string err_msg;
};

// inpout/output format related functions
string toLowerCase(const string&);
vector<string> parseCSV(const string&);
bool parseGrp(const string&, const string&, vector<int32>&);
bool isElement(const string&, const vector<int32>&);
bool isElement(int32, const vector<int32>&);
void stripchars(char *, const char*);
string tailorStr(string);

// Utility functions that show global maps on client/server side (for debugging only)
void showTypeMap(const map<string, DBtype>&);
void showScoreNameMap(const map<int32, DBscorename>&);
void showRegionNameMap(const map<int32, regionRec>&);
void showSynonymMap(const map<int32, synonymRec>&);
void showRelationMap(const map<int32, regionRelationRec>&);

// brain region functions
string trRel_db2ui(const string& inputStr, bool keepOrder);
void trRel_ui2db(int32*, int32*, string& relStr);
int chkRelation(vector<string>, const string&);
bool chkRelation(const string&, const string&);

// gnutls related functions
int make_salt_verifier(const string &,const string &,gnutls_datum_t &,gnutls_datum_t &);
int make_verifier(const string &,const string &,gnutls_datum_t &salt,gnutls_datum_t &verifier);
int sendMessage(gnutls_session_t&, const string&);
int recvMessage(gnutls_session_t&, char*);
int sendBuffer(gnutls_session_t&, int32, char*);
int recvBuffer(gnutls_session_t&, int32, char*);

// patient search functions
bool cmpStr(const string&, const string&, bool, const string&);
bool GeneralTextCompare(char*, char*, bool = false, char = '\0');
int32 getPatientIndex(int32, const vector<patientMatch>&);
bool parseEnvPath(const string&, string&, string&);
bool rmDB(const string&, const string&);
bool rmDB(const string&);

// Functions related to data transfer on network
int32 getHdrSize(const vector<DBscorevalue>&);
void serializeHdr(const vector<DBscorevalue>&, char*);
void deserializeHdr(int32, char*, vector<DBscorevalue>&);

// Function template that returns size of buffer that will hold input vector of data structor
template<typename T> 
int32 getSize(const vector<T>& in_list)
{
  int32 total = 0;
  for (unsigned i = 0; i < in_list.size(); ++i) { 
    total += in_list[i].getSize();
  }

  return total;
}

// Function template that converts a vector of objects into serialized data buffer. 
template<typename T> 
void serialize(const vector<T>& in_list, char* out_buff)
{
  int32 offset = 0;
  for (unsigned i = 0; i < in_list.size(); ++i) { 
    in_list[i].serialize(out_buff + offset);
    offset += in_list[i].getSize();;
  }
}

// Function template that converts serialized data back into a vector of objects.
template<typename T> 
void deserialize(int32 dat_size, char* in_buf, vector<T>& out_list)
{
  int32 offset = 0;
  while (offset < dat_size) {
    T tmpRec(in_buf + offset);
    out_list.push_back(tmpRec);
    offset += tmpRec.getSize();
  }
}

/* Function template that converts a non-string type into C++ string
 * It is mainly used to convert a numeric value (such as int32 to string). */
// FIXME replace with vbutil strnum()
template<typename T>  
string num2str(T inputNum) 
{
  string foo;
  stringstream out;
  out << inputNum;
  foo = out.str();

  return foo;
}

/* Function template that converts a string into non-string type.
 * It converts a string into numeric type (such as int32). 
 * Returns true is conversion is successful, returns false otherwise. */
template<typename T>
bool str2num(const string& inputStr, T& outVal) 
{
  std::istringstream iss(inputStr);
  if (iss >> outVal)
    return true;

  return false;
}

// Function template that prints out all db record (db environment not considered)
template<typename T> 
void showDB(string dbname,                             // db filename
	    mydb::key_comp cmp_method = mydb::cmp_int, // by default, keys compared numerically 
	    mydb::dup_flag dupsort = mydb::no_dup)     // by default, duplicate keys not allowed
{
  // open db in read-only mode
  mydb currentDB(dbname, cmp_method, dupsort, DB_RDONLY);
  if (!currentDB) {
    return;
  }

  Dbc* cursorp = NULL;
  if (currentDB.getDb().cursor(NULL, &cursorp, 0)) {
    cout << "DB cursor error" << endl;
    currentDB.close();
    return;
  }

  Dbt key, data;
  int ret;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    T db_rec(data.get_data());
    db_rec.show();
    printf("\n");
  }
  cursorp->close();

  if (ret && ret != DB_NOTFOUND) {
    cout << "DB error" << endl;
  }

  currentDB.close();
  return;
}

// Function template that prints out all db record in a db environment
template<typename T> 
void showEnvDB(string dbPath,                             // db filename
	       mydb::key_comp cmp_method = mydb::cmp_int, // by default, keys compared numerically 
	       mydb::dup_flag dupsort = mydb::no_dup)     // by default, duplicate keys not allowed
{
  // get env dir and db filename
  string envHome;
  string dbName;
  if (!parseEnvPath(dbPath, envHome, dbName)) {
    return;
  }

  // open env
  myEnv env(envHome);
  if (!env)
    return;
  // open db in env
  mydb currentDB(dbName, env, cmp_method, dupsort);
  if (!currentDB) {
    env.close();
    return;
  }

  Dbc* cursorp = NULL;
  if (currentDB.getDb().cursor(NULL, &cursorp, 0)) {
    cout << "DB cursor error" << endl;
    currentDB.close();
    env.close();
    return;
  }

  Dbt key, data;
  int ret;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    T db_rec(data.get_data());
    db_rec.show();
    printf("\n");
  }
  cursorp->close();

  if (ret && ret != DB_NOTFOUND) {
    cout << "DB error" << endl;
  }

  currentDB.close();
  env.close();
}

/********************************************************************
 * DB operation functions, used by both local and remote server 
 ********************************************************************/
int getSystem(mydb&, DbTxn*, const string&, string&);
int setSystem(mydb&, DbTxn*, const string&, const string&);
int32 getSysUpdate(mydb&, DbTxn*); 
int setSysUpdate(mydb&, DbTxn*, int32, string&);
int setSysUpdate(mydb&, DbTxn*, string&);
int32 getSysID(mydb&, DbTxn*, uint32 = 1);

int getUser(mydb&, DbTxn*, const string&, userRec&);
int addUser(mydb&, mydb&, DbTxn*, userRec&);
int addUser(mydb&, mydb&, DbTxn*, void*);
int chkUser(mydb&, DbTxn*, const string&);
int chkPasswd(mydb&, DbTxn*, const string&, const string&);
int addPerm(mydb&, DbTxn*, const permRec&);
int getPerm(mydb&, DbTxn*, const vector<int32>&, map<string, string>&);
int getPerm(mydb&, mydb&, DbTxn*, int32, map<string, string>&);
void mergePerm(const permRec&, map<string, string>&);
int getGrpIDs(mydb&, DbTxn*, int32, vector<int32>&);
int addContact(mydb&, mydb&, DbTxn*, contactRec&);
int addStudy(mydb&, mydb&, DbTxn*, studyRec&);
int addUserGrp(mydb&, mydb&, DbTxn*, userGrpRec&);
int addUserRelation(mydb&, mydb&, DbTxn*, userRelRec&);

int addPatient(mydb&, DbTxn*, const patientRec&);
int addPatient(mydb&, mydb&, DbTxn*, bool);
int addPatientGrp(mydb&, mydb&, DbTxn*, pgrpRec&);
int addPgrpMember(mydb&, DbTxn*, pgrpMemberRec&);
int addSession(mydb&, mydb&, DbTxn*, DBsession&);
int addSession(mydb&, DbTxn*, const DBsession&);
int addSession(mydb&, DbTxn*, vector<DBsession>&, time_t);
int addView(mydb&, mydb&, DbTxn*, viewRec&);
int addViewEntry(mydb&, DbTxn*, const viewEntryRec&);
int addScoreName(mydb&, DbTxn*, const DBscorename&);
int addScoreName(mydb&, DbTxn*, const vector<DBscorename>&);

int getSearchFields(mydb&, DbTxn*, const string&, vector<string>&);
//int addScoreValue(mydb&, DbTxn*, vector<DBscorevalue>&); 
int addScoreValue(mydb&, DbTxn*, DBscorevalue&);
int getKeyLabel(mydb&, DbTxn*, int32, uint32);
int addScoreValue(mydb&, mydb&, DbTxn*, DBscorevalue&);

int addPatientList(mydb&, DbTxn*, const vector<DBpatientlist>&);
int addPatientList(mydb&, DbTxn*, const DBpatientlist&);
int getPatientList(mydb&, mydb&, DbTxn*, int32, vector<DBpatientlist>&);

int searchPatients(mydb&, DbTxn*, const patientSearchTags&, const map<string, string>&, 
		   vector<patientMatch>&);
bool readPermitted(const DBscorevalue&, const map<string, string>&);
int getOnePatient(mydb&, mydb&, DbTxn*, int32, const map<string, string>&, DBpatient&);
int32 getScoreValues(mydb&, DbTxn* txn, int32, const map<string, string>&, 
		     vector<DBscorevalue>&, set<int32>&); 
string getMaxPerm(const DBscorevalue&, const map<string, string>&);
int32 getSessions(mydb&, DbTxn* txn, const set<int32>&, vector<DBsession>&);
int32 getSessions(mydb&, DbTxn* txn, const set<int32>&, map<int32, DBsession>&);

// Functions called to build secondary patient score value database
int getPID(Db*sdbp, const Dbt* pkey, const Dbt* pdata, Dbt* skey);



#endif

