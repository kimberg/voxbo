
// db_util.cpp
// This file defines some generic functions for record exportation.
// Copyright (c) 2008-2010 by The VoxBo Development Team

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

#include "db_util.h"
#include "vbutil.h"
#include <cstring>
#include <string>
#include <arpa/inet.h>

/********************************************
 * Functions in userGUI class 
 ********************************************/
// Default constructor
userGUI::userGUI() 
{ 

}

// Constructor that parses input buffer and assigns values to data members
userGUI::userGUI(void *buffer) {
  char *buf = (char *) buffer;
  int32 bufLen = 0;

  account = buf + bufLen;
  bufLen += account.size() + 1;
    
  passwd = buf + bufLen;
  bufLen += passwd.size() + 1;

  name = buf + bufLen;
  bufLen += name.size() + 1;

  phone = buf + bufLen;
  bufLen += phone.size() + 1;

  email = buf + bufLen;
  bufLen += email.size() + 1;

  address = buf + bufLen;
  bufLen += address.size() + 1;

  groups = buf + bufLen;
  bufLen += groups.size() + 1;
}

// Initialize data members
void userGUI::clear() 
{
  account = passwd = name = "";
  phone = email = address = groups = "";
}

// Serialize data members into a single contiguous memory location
void userGUI::serialize(char *databuf) const
{
  int32 offset = 0;

  memcpy(databuf + offset, account.c_str(), account.size() + 1);
  offset += account.size() + 1;

  memcpy(databuf + offset, passwd.c_str(), passwd.size() + 1);
  offset += passwd.size() + 1;

  memcpy(databuf + offset, name.c_str(), name.size() + 1);
  offset += name.size() + 1;

  memcpy(databuf + offset, phone.c_str(), phone.size() + 1);
  offset += phone.size() + 1;

  memcpy(databuf + offset, email.c_str(), email.size() + 1);
  offset += email.size() + 1;

  memcpy(databuf + offset, address.c_str(), address.size() + 1);
  offset += address.size() + 1;

  memcpy(databuf + offset, groups.c_str(), groups.size() + 1);
  offset += groups.size() + 1;
}

// Returns the size of the buffer
int32 userGUI::getSize() const 
{ 
  int32 offset = 0;
  offset += account.size() + 1;
  offset += passwd.size() + 1;
  offset += name.size() + 1;
  offset += phone.size() + 1;
  offset += email.size() + 1;
  offset += address.size() + 1;
  offset += groups.size() + 1;

  return offset; 
}

// Show the contents of date members
void userGUI::show() const 
{
  printf("Account Name: %s\n", account.c_str());
  printf("Real Name: %s\n", name.c_str());
  printf("Phone #: %s\n", phone.c_str());
  printf("Email: %s\n", email.c_str());
  printf("Address: %s\n", address.c_str());
  printf("Groups: %s\n", groups.c_str());
}

/********************************************
 * Functions in patientMatch class 
 ********************************************/
// Default constructor
patientMatch::patientMatch() : patientID(0)
{ 

}

// Ctor that accepts three arguments
patientMatch::patientMatch(int32 pID, const string& nameStr, const string& valStr) 
{
  patientID = pID;
  scoreMap[nameStr] = valStr;
}

// Constructor that accepts a data block
patientMatch::patientMatch(void *buffer_in) 
{
  char *buf = (char *) buffer_in;
  int32 offset = 0;

  patientID = *((int32 *) (buf + offset));
  if (ntohs(1) == 1)
    swap(&patientID);
  offset += sizeof(int32);

  uint32 score_count = *((uint32 *) (buf + offset));
  if (ntohs(1) == 1)
    swap(&score_count);
  offset += sizeof(uint32);

  for (uint i = 0; i < score_count; i++) {
    string nameStr = buf + offset;
    offset += nameStr.size() + 1;
    string newVal = buf + offset;
    offset += newVal.size() + 1;
    scoreMap[nameStr] = newVal;
  }
}

// Initialize data members
void patientMatch::clear() 
{
  patientID = 0;
  scoreMap.clear();
}

// Set a single contiguous memory location that holds values of data members
void patientMatch::serialize(char *databuf) const
{
  int32 offset = 0;
  int32 foo = patientID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(databuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  
  uint32 map_size = scoreMap.size();
  if (ntohs(1) == 1)
    swap(&map_size);
  memcpy(databuf + offset, &map_size, sizeof(uint32));
  offset += sizeof(uint32);

  for (map<string, string>::const_iterator iter = scoreMap.begin(); 
       iter != scoreMap.end(); ++iter) {
    string nameStr = iter->first, valStr = iter->second;
    memcpy(databuf + offset, nameStr.c_str(), nameStr.size() + 1);
    offset += nameStr.size() + 1;
    memcpy(databuf + offset, valStr.c_str(), valStr.size() + 1);
    offset += valStr.size() + 1;
  }
}

// Returns the size of the buffer
int32 patientMatch::getSize() const
{ 
  int32 bufSize = 0;
  bufSize += sizeof(int32);
  bufSize += sizeof(uint32);
  for (map<string, string>::const_iterator iter = scoreMap.begin(); 
       iter != scoreMap.end(); ++iter) {
    bufSize += (iter->first).size() + 1;
    bufSize += (iter->second).size() + 1;
  }

  return bufSize; 
}

// Add a new score value
void patientMatch::addScore(const string& newName, const string& newVal) 
{
  scoreMap[newName] = newVal;
}

// Print out data members
void patientMatch::show() const
{
  printf("Patient ID: %d\n", (int)patientID);
  for (map<string, string>::const_iterator iter = scoreMap.begin(); 
       iter != scoreMap.end(); ++iter) {
    printf("Score ID: %s\n", iter->first.c_str());
    printf("Score value: %s\n", iter->second.c_str());
  }
}

// Initialize data members in patient search tag class 
void patientSearchTags::init() 
{
  case_sensitive = true;
  scoreName = relationship = searchStr = "";
  patientIDs.clear();
  err_msg = string("no_data_sent: ");
}

// Initialize data members in patient search tag class 
string patientSearchTags::getStr() const
{
  string tagStr("search_patient: ");
  if (case_sensitive)
    tagStr.append("case_sensitive ");
  else
    tagStr.append("case_insensitive ");

  if (scoreName.size())
    tagStr.append(scoreName);
  else
    tagStr.append("any");
  
  tagStr.append(" ");
  tagStr.append(relationship);
  tagStr.append(" ");
  tagStr.append(searchStr);

  if (patientIDs.size()) {
    tagStr.append(" ");
    for (unsigned i = 0; i < patientIDs.size(); ++i) {
      tagStr.append(num2str(patientIDs[i]));
      if (i != patientIDs.size() - 1)
	tagStr.append(" ");
    }
  }

  return tagStr;
}

/* A simple function that checks whether a certain patient ID is already in patient match list. */
int32 getPatientIndex(int32 pID, const vector<patientMatch>& pMatches)
{
  for (uint32 i = 0; i < pMatches.size(); i++) {
    if (pID == pMatches[i].patientID)
      return i;
  }

  return -1;
}

/* This function converts each letter in an input string into lower case and returns the new string. */
string toLowerCase(const string& inputStr)
{
  string newStr = inputStr;
  for (unsigned i = 0; i< inputStr.size(); i++)
    newStr[i] = tolower(inputStr[i]);

  return newStr;
}

/* A simple function to parse comma separated line */
vector<string> parseCSV(const string& inputStr) 
{
  int qm1 = -99;
  int qm2 = -99;
  int strLen = inputStr.length();
  int startPost = 0;
  vector<string> fields;

  for (int i = 0; i < strLen; i++) {
    if (inputStr[i] == '\"') {
      if (qm1 == -99)
	qm1 = i;
      else
	qm2 = i;
    }
    else if (inputStr[i] == ',' && (qm1 == -99 || qm2 != -99)) {
      string tmpStr = "";
      if (qm1 == -99)
	tmpStr = inputStr.substr(startPost, i - startPost);
      else
	tmpStr = inputStr.substr(startPost + 1, i - startPost - 2);

      qm1 = qm2 = -99;
      startPost = i + 1;
      fields.push_back(tmpStr);
    }
  }

  string endStr = "";
  if (qm1 == -99 || qm2 == -99)
    endStr = inputStr.substr(startPost, strLen - startPost);
  else
    endStr = inputStr.substr(startPost + 1, strLen - startPost - 2);

  fields.push_back(endStr);

  return fields;
}

/* This function is written to parse the group field in user db. 
 * It converts the string argument into a list of int32 integers. */
bool parseGrp(const string& inputStr, const string& separator, vector<int32>& outList)
{
  int32 foo = 0;
  // FIXME should use tokenlist for this
  size_t x = inputStr.find(separator);
  string newStr = inputStr;
  while (x != string::npos) {
    string grpStr = newStr.substr(0, x);
    if (grpStr.length()) {
      bool conv_stat = str2num(grpStr, foo);
      if (!conv_stat || foo <= 0)
	return false;
      outList.push_back(foo);
    }
    int strLen = newStr.length();
    newStr = newStr.substr(x + 1, strLen - x - 1);  
    x = newStr.find(separator);
  }

  bool conv_stat = str2num(newStr, foo);
  if (!conv_stat || foo <= 0)
    return false;

  outList.push_back(foo);
  return true;
}

/* Simple function that tells whether the first argument is an element of the second array or not. */
bool isElement(const string& inputStr, const vector<int32>& inputArray)
{
  if (inputStr == "*")
    return true;

  int32 foo;
  bool conv_stat = str2num(inputStr, foo);
  if (!conv_stat)
    return false;

  return isElement(foo, inputArray);
}

/* Simple function that checks whether a certain inputID exists on inputList or not. */
bool isElement(int32 inputID, const vector<int32>& inputList)
{
  for (uint i = 0; i < inputList.size(); i++) {
    if (inputID == inputList[i])
      return true;
  }

  return false;
}

/* Remove an input string's leading and trailing white space characters */
// FIXME can we use xstripwhitespace() from vbutil?
string tailorStr(string inputStr)
{
  int strLen = inputStr.length();
  int start = 0, end = 0;
  int i;
  for (i = 0; i < strLen; i++) {
    if (inputStr[i] != 9 && inputStr[i] != 32) {
      start = i;
      break;
    }
  }
  // if string only includes space and tab characters, reset it to blank
  if (i == strLen)
    return "";

  for (int j = strLen - 1; j >= 0; j--) {
    if (inputStr[j] != 9 && inputStr[j] != 32) {
      end = j;
      break;
    }
  }
  // return the substring that does not leading and trailing space/tab characters
  return inputStr.substr(start, end - start + 1);
}

/* Check whether two input strings match each other or not. 
 * The third argument determines whether comparison is case sensitive or not;
 * the fourth argument determines the relationship: equal, include or wildcard.*/
bool cmpStr(const string& str1, const string& str2, bool case_sensitive, const string& relStr)
{
  string foo = str1, bar = str2;
  if (!case_sensitive) {
    foo = toLowerCase(str1);
    bar = toLowerCase(str2);
  }
   
  if (relStr == "equal" && foo == bar)
    return true;
  
  if (relStr == "include" && foo.find(bar) != string::npos) // foo includes bar
    return true;

  if (relStr == "wildcard" && GeneralTextCompare((char *) foo.c_str(), (char *) bar.c_str(), true)) 
    return true;

  return false;
}

/* Wildcard comparison function copied from:
 * http://www.ddj.com/architect/210200888
 */
bool GeneralTextCompare(
			char* pTameText,             
			// A string without wildcards
			char* pWildText,             
			// A (potentially) corresponding string with wildcards
			bool bCaseSensitive,  
			// By default, match on "X" vs "x"
			char cAltTerminator    
			// For function names, for example, you can stop at the first "("
			)
{
  bool bMatch = true;
  char * pAfterLastWild = NULL; 
  // Location after last "*", if we've encountered one
  char t, w;
  // Walk the text strings one character at a time.
  while (1) {
    t = *pTameText;
    w = *pWildText;
    // How do you match a unique text string?
    if (!t || t == cAltTerminator) {
      // Easy: unique up on it!
      if (!w || w == cAltTerminator) {
	break;                 // "x" matches "x"
      }
      else if (w == '*') {
	pWildText++;
	continue;              // "x*" matches "x" or "xy"
      }
      bMatch = false;
      break;                     // "x" doesn't match "xy"
    }
    else {
      if (!bCaseSensitive) {
	// Lowercase the characters to be compared.
	if (t >= 'A' && t <= 'Z') {
	  t += ('a' - 'A');
	}

	if (w >= 'A' && w <= 'Z') { 
	  w += ('a' - 'A'); 
	} 
      } 
      // How do you match a tame text string? 
      if (t != w) { 
	// The tame way: unique up on it! 
	if (w == '*') { 
	  pAfterLastWild = ++pWildText; 
	  continue; // "*y" matches "xy" 
	} 
	else if (pAfterLastWild) { 
	  pWildText = pAfterLastWild; 
	  w = *pWildText; 
	  if (!w || w == cAltTerminator) { 
	    break; // "*" matches "x" 
	  } 
	  else if (t == w) { 
	    pWildText++; 
	  } 
	  pTameText++; 
	  continue; // "*sip*" matches "mississippi" 
	} 
	else { 
	  bMatch = false; 
	  break; // "x" doesn't match "y" 
	} 
      } 
    } 
    pTameText++; 
    pWildText++; 
  } 
  return bMatch; 
} 

// Print out contents of score type map (for debugging purpose)
void showTypeMap(const map<string, DBtype>& inputMap)
{
  for (map<string, DBtype>::const_iterator iter = inputMap.begin(); 
       iter != inputMap.end(); ++iter) {
    cout << "-------------------------------" << endl;
    cout << iter->first << endl;
    iter->second.show();
  }
}

// Print out contents of score name map (for debugging purpose)
void showScoreNameMap(const map<int32, DBscorename>& inputMap)
{
  for (map<int32, DBscorename>::const_iterator iter = inputMap.begin(); 
       iter != inputMap.end(); ++iter) {
    cout << "-------------------------------" << endl;
    cout << iter->first << endl;
    iter->second.show();
  }
}

// Print out contents of region name map (for debugging purpose)
void showRegionNameMap(const map<int32, regionRec>& inputMap)
{
  for (map<int32, regionRec>::const_iterator iter = inputMap.begin(); 
       iter != inputMap.end(); ++iter) {
    cout << "-------------------------------" << endl;
    cout << iter->first << endl;
    iter->second.show();
  }
}

// Print out contents of region synonym map (for debugging purpose)
void showSynonymMap(const map<int32, synonymRec>& inputMap)
{
  for (map<int32, synonymRec>::const_iterator iter = inputMap.begin(); 
       iter != inputMap.end(); ++iter) {
    cout << "-------------------------------" << endl;
    cout << iter->first << endl;
    iter->second.show();
  }
}

// Print out contents of region relationship map (for debugging purpose)
void showRelationMap(const map<int32, regionRelationRec>& inputMap)
{
  for (map<int32, regionRelationRec>::const_iterator iter = inputMap.begin(); 
       iter != inputMap.end(); ++iter) {
    cout << "-------------------------------" << endl;
    cout << iter->first << endl;
    iter->second.show();
  }
}


/* This function "translates" the relationship string in DB file 
 * to description string that will be shown on QT interface. 
 * The inputFlag specifies whether the relationship should be keep 
 * in orginal order or reversed (i.e. "child" to "parent", "part of" to "include". */
string trRel_db2ui(const string& inputStr, bool keepOrder)
{
  string newRel;
  if (inputStr == "child") {
    if (keepOrder)
      newRel = "is child of";
    else
      newRel = "is parent of";
  }
  else if (inputStr == "part-of") {
    if (keepOrder)
      newRel = "is part of";
    else
      newRel = "includes";
  }
  else if (inputStr == "overlap")
    newRel = "overlaps with";
  else if (inputStr == "equiv")
    newRel = "is equiv to";

  return newRel;
}

/* This function translates an input string into relationship record in DB file. */
void trRel_ui2db(int32 *r1_ID, int32 *r2_ID, string& relStr)
{
  int32 tmpVal = *r1_ID;
  if (relStr == "includes") {
    *r1_ID = *r2_ID;
    *r2_ID = tmpVal;
    relStr = "part-of";
  }
  else if (relStr == "is part of")
    relStr = "part-of";
  else if (relStr == "overlaps with")
    relStr = "overlap";
  else if (relStr == "is equiv to")
    relStr = "equiv";
}

/* This function checks whether a new relationship is compatible with the existing ones or not.
 * If confliction is found, return the uncompatible relationship's index; 
 * If no confliction is found, return -1. */ 
int chkRelation(vector<string> relList, const string& newRel)
{
  for (unsigned i = 0; i < relList.size(); i++) {
    if (!chkRelation(relList[i], newRel))
      return i;
  }

  return -1;
}

/* This function checks whether two input relations are compatible or not.
 * So far only two relationships are compatible: 
 * parent vs. include;
 * superset vs. include;
 * child vs. part-of;
 * subset vs. part-of; */
bool chkRelation(const string& relation1, const string& newRel)
{
  if (relation1 == "is parent of" && newRel == "includes")
    return true;

  if (relation1 == "is superset of" && (newRel == "includes" || newRel == "is parent of"))
    return true;

  if (relation1 == "is child of" && newRel == "is part of")
    return true;

  if (relation1 == "is subset of" && (newRel == "is part of" || newRel == "is child of"))
    return true;

  return false;
}


// make_salt_verifier() generates a random salt string and creates a
// verifier for the username and password provided.  The size of the
// verifier will be in verifier->size, the data in verifier->data.
// This function allocates storage for the verifier using
// gnutls_malloc.  When done with the verifier, the datum can be
// de-allocated like this: gnutls_free(verifier.data);

int
make_salt_verifier(const string& username, const string& password, 
                       gnutls_datum_t &salt, gnutls_datum_t &verifier)
{
  // init salt structure for gnutls
  salt.size = 4;
  salt.data = (unsigned char *) gnutls_malloc(4);
  if (!salt.data)
    return -1;
  // generate random salt, copy for calling function and to salt struct
  uint32 st = VBRandom();
  memcpy(salt.data,&st,4);
  return gnutls_srp_verifier(username.c_str(),password.c_str(),&salt,
                             &(gnutls_srp_1024_group_generator),
                             &(gnutls_srp_1024_group_prime),
                             &verifier);
}

// make_verifier() generates the verifier from a username, pw, and
// salt

int make_verifier(const string& username, const string& password, 
                  gnutls_datum_t &salt, gnutls_datum_t &verifier)
{
  return gnutls_srp_verifier(username.c_str(),password.c_str(),&salt,
                             &(gnutls_srp_1024_group_generator),
                             &(gnutls_srp_1024_group_prime),
                             &verifier);
}

/* Send s simple message between client and server. 
 * Returns 0 if message is sent out successfully;
 * returns -1 otherwise. */
int sendMessage(gnutls_session_t& g_session, const string& message)
{
  uint32 foo = gnutls_record_send(g_session, message.c_str(), message.size() + 1);
  if (foo != message.size() + 1) 
    return -1;

  return 0;
}

/* Receive simple message between client and server. 
 * Returns 0 if the size of received data is positive;
 * returns -1 otherwise. */
int recvMessage(gnutls_session_t& g_session, char* message)
{
  if (gnutls_record_recv(g_session, message, MSG_SIZE) > 0)
    return 0;

  return -1;
}

/* Send a data buffer to remote side through gnutls session.
 * Returns 0 if data buffer is sent out successfully;
 * returns -1 otherwise; */
int sendBuffer(gnutls_session_t& g_session, int32 buf_size, char* buffer)
{
  int32 buf_max = gnutls_record_get_max_size(g_session);

  if (buf_size <= buf_max) {
    int32 foo = gnutls_record_send(g_session, buffer, buf_size);
    if (foo != buf_size)
      return -1;
    return 0;
  }
  
  int32 last = buf_size % buf_max;
  int32 offset = 0;
  while (offset < buf_size - last) {
    int32 foo = gnutls_record_send(g_session, buffer + offset, buf_max);
    if (foo != buf_max)
      return -1;
    offset += buf_max;
  }

  if (last) {
    int32 foo = gnutls_record_send(g_session, buffer + offset, last);
    if (foo != last)
      return -1;
  }
  
  return 0;
}

/* Receive buffer from remote side through gnutls ression.
 * Returns 0 if data buffer is received successfully;
 * returns -1 otherwise. */
int recvBuffer(gnutls_session_t& g_session, int32 buf_size, char* buffer)
{
  int32 buf_max = gnutls_record_get_max_size(g_session);
  if (buf_size <= buf_max) {
    int32 foo = gnutls_record_recv(g_session, buffer, buf_size);
    if (foo != buf_size)
      return -1;
    return 0;
  }

  int32 last = buf_size % buf_max;
  int32 offset = 0;
  while (offset < buf_size - last) {
    int32 foo = gnutls_record_recv(g_session, buffer + offset, buf_max);
    if (foo != buf_max)
      return -1;
    offset += buf_max;
  }
  if (last) {
    int32 foo = gnutls_record_recv(g_session, buffer + offset, last);
    if (foo != last)
      return -1;
  }
  
  return 0;
}

// This function parses an input string and get env name and db filename
bool parseEnvPath(const string& inputPath, string& envHome, string& dbName)
{
  // FIXME this is probably unix-specific and should probably use
  // xdirname/xfilename from vbutil
  size_t x = inputPath.rfind("/");
  if (x == inputPath.size() - 1) {
    printf("Invalid bdb file: %s\n", inputPath.c_str());
    return false;
  }

  if (x != string::npos) {
    int strLen = inputPath.size();
    envHome = inputPath.substr(0, x + 1);
    dbName = inputPath.substr(x + 1, strLen - x - 1);
  }
  else {
    envHome = "./";
    dbName = inputPath;
  }

  return true;
}

// This function removes a db file in db environment 
bool rmDB(const string& envHome, const string& dbName)
{
  // open env
  myEnv env(envHome);
  if (!env) {
    cout << "Failed to open env: " << envHome << endl;
    return false;
  }

  DbTxn* txn = NULL;
  env.getEnv()->txn_begin(NULL, &txn, 0);
  if (env.getEnv()->dbremove(txn, dbName.c_str(), NULL, 0)) {
    cout << "Faield to remove db file: " << dbName << endl;    
    txn->abort();
    env.close();
    return false;
  }
  
  txn->commit(0);
  env.close();
  return true;
}

// Removes a db file without env
bool rmDB(const string& dbName)
{
  // FIXME never call this
  return false;
  mydb junkDB;
  junkDB.initxxx();
  if (junkDB.getDb().remove(dbName.c_str(), NULL, 0)) {
    cout << "Failed to remove db file: " << dbName << endl;
    return false;
  }

  return true;
}

// Calculate total header size of an input DBscorevalue vector
int32 getHdrSize(const vector<DBscorevalue>& in_list)
{
  int32 total = 0;
  for (unsigned i = 0; i < in_list.size(); ++i) { 
    total += in_list[i].getHdrSize();
  }

  return total;
} 

// Serialize header info of an input DBscorevalue vector
void serializeHdr(const vector<DBscorevalue>& in_list, char* out_buff)
{
  int32 offset = 0;
  for (unsigned i = 0; i < in_list.size(); ++i) { 
    int32 dat_size = in_list[i].getHdrSize();
    in_list[i].serializeHdr(out_buff + offset);
    offset += dat_size;
  }
}

// Set DBscorevalue header info based on input buffer
void deserializeHdr(int32 dat_size, char* in_buf, vector<DBscorevalue>& out_list)
{
  int32 offset = 0;
  while (offset < dat_size) {
    DBscorevalue tmpRec;
    tmpRec.deserializeHdr(in_buf + offset);
    out_list.push_back(tmpRec);
    offset += tmpRec.getHdrSize();
  }
}

/********************************************************************
 * DB operation functions, used by both local and remote server 
 ********************************************************************/
/* This function gets a certain filed's value from system.db and 
 * put it to fieldValue string. 
 * returns 0 if the key is NOT found;
 * returns 1 if the key is found;
 * returns -1 for db errors; */
int getSystem(mydb& sysDB, DbTxn* txn, const string& fieldName, string& fieldValue)
{
  Dbt key((char *) fieldName.c_str(), fieldName.length() + 1);
  Dbt data;
  int ret;
  int32 stat = 0;
  Dbc *cursorp = NULL;
  if (sysDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  ret = cursorp->get(&key, &data, DB_SET);
  if (ret == 0) {
    sysRec sData(data.get_data());
    fieldValue = sData.getValue();
    stat = 1;
  }
  else if (ret == DB_NOTFOUND)
    stat = 0;
  else
    stat = -1; 

  cursorp->close();  
  return stat;
}

/* This function sets a certain record's value in system.db to input string. 
 * returns 0 if everything is ok.
 * Returns -1 for db errors; */
int setSystem(mydb& sysDB, DbTxn* txn, const string& fieldName, const string& inputStr)
{
  Dbt key((char *) fieldName.c_str(), fieldName.length() + 1);
  sysRec sData;
  sData.setName(fieldName);
  sData.setValue(inputStr);
  int32 size = sData.getSize();
  char buff[size];
  sData.serialize(buff);
  Dbt data(buff, size);

  int foo = sysDB.getDb().put(txn, &key, &data, 0);
  if (foo)
    return -1;

  return 0;
}

/* Retrieve time stamp of the last update from system db.
 * Returns valid time stamp if everything is ok;
 * returns 0 if the record doesn't exist in system table yet; 
 * returns -1 for db errors;
 * returns -2 if string in system table can't be converted into a valid time stamp integer. */
int32 getSysUpdate(mydb& sysDB, DbTxn* txn)
{  
  string fieldStr = "last_updated";
  string time_str;
  int foo = getSystem(sysDB, txn, fieldStr, time_str);
  if (foo <= 0)
    return foo;
  
  int32 time_int = 0;
  if (!str2num(time_str, time_int) || time_int <= 0)
    return -2;

  return time_int;
}

/* Set "last_updated" record in system table to a certain time stamp integer.
 * returns -1 if we can't retrieve time stamp stored in system table;
 * returns -2 if time stamp in system table is later than input time stamp;
 * returns -3 if new time stamp can not be updated in system table;
 * returns 0 if everything is ok. */
int setSysUpdate(mydb& sysDB, DbTxn* txn, int32 time_in, string& outputStr)
{
  int32 org_time = getSysUpdate(sysDB, txn);
  if (org_time < 0) {
    outputStr = "Invalid time stamp of last update: " + num2str(org_time);
    return -1;
  }

  if (org_time > time_in) {
    time_t tmp = org_time;
    outputStr = string("Original DB update time is later than current update: ") + ctime(&tmp);
    return -2;
  }

  if (org_time == time_in)
    return 0;

  string fieldStr = "last_updated";
  if (setSystem(sysDB, txn, fieldStr, num2str(time_in))) {
    outputStr = "DB error: failed to set time stamp of last update";
    return -3;
  }

  return 0;
}

/* Overloaded function that sets "last_updated" record in system table to current unix time stamp.
 * returns -1 if we can't retrieve time stamp stored in system table;
 * returns -2 if time stamp in system table is later than curren time stamp;
 * returns -3 if current time stamp can not be updated in system table;
 * returns 0 if everything is ok. */
int setSysUpdate(mydb& sysDB, DbTxn* txn, string& outputStr)
{
  int32 org_time = getSysUpdate(sysDB, txn);
  if (org_time < 0) {
    outputStr = "Invalid time stamp of last update: " + num2str(org_time);
    return -1;
  }

  int32 new_time = time(NULL);
  if (org_time > new_time) {
    time_t tmp = org_time;
    outputStr = string("Original DB update time is later than current update: ") + ctime(&tmp);
    return -2;
  }

  if (org_time == new_time)
    return 0;

  string fieldStr = "last_updated";
  if (setSystem(sysDB, txn, fieldStr, num2str(new_time))) {
    outputStr = "DB error: failed to set time stamp of last update";
    return -3;
  }

  return 0;
}

/* This function gets the starting value of a series of unique IDs from system.db table.
 * The default value of id_no is 1. 
 * Returns the starting ID if everything is ok;
 * returns -1 for db errors;
 * returns -2 if ID is not valid positive integer;
 * returns -3 if new ID in system.db can not be updated. */
int32 getSysID(mydb& sysDB, DbTxn* txn, uint32 id_no)
{
  Dbc *cursorp = NULL;
  if (sysDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  string fieldStr = "Next Unique ID";
  Dbt key((char*) fieldStr.c_str(), fieldStr.length() + 1);
  Dbt data;
  int ret;
  int32 stat = 0;

  ret = cursorp->get(&key, &data, DB_SET);
  if (ret) {
    cursorp->close();
    return -1;
  }
  cursorp->close();

  sysRec sData(data.get_data());
  string fieldValue = sData.getValue();
  bool conv_stat = str2num(fieldValue, stat);
  if (!conv_stat || stat <= 0)
    return -2;

  int32 newVal = stat + id_no;
  if (setSystem(sysDB, txn, fieldStr, num2str(newVal)))
    return -3;
  
  return stat;  
}

/* This function retrieves a certain user's information out of user table.
 * returns 0 if user account is NOT found;
 * returns 1 if account is found;
 * returns -1 for db errors. */
int getUser(mydb& userDB, DbTxn* txn, const string& username, userRec& outData)
{
  Dbc *cursorp = NULL;
  if (userDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  int ret;
  int status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    userRec tmpData(data.get_data());
    if (username == tmpData.getAccount()) { 
      outData.deserialize(data.get_data());
      status = 1;
      break;
    }
  } 
  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -1;

  cursorp->close();
  return status;
}

/* This function adds a user record into user table. 
 * returns 0 if everything is ok; 
 * returns -1 for db errors;
 * returns -2 if ID can not be retrieved from db. */
int addUser(mydb& userDB, mydb& sysDB, DbTxn* txn, userRec& data_in)
{
  int32 recID = data_in.getID();
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  int32 bufLen = data_in.getSize();
  char buff[bufLen];
  data_in.serialize(buff);
  Dbt key(&recID, sizeof(int32));
  Dbt data(buff, bufLen);

  if (userDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function interprets a data block as user record adn add it into user table. 
 * If the user name is unique, the record will be added to user db. 
 * Returns 0 if everything is correct;
 * returns 1 if user already exists in db;
 * returns -1 for db error when checking user;
 * returns -2 if new ID can not be created in db;
 * returns -3 if new user record can not be added in to db;
 * returns -4 if verifier can not be created successfully. */
int addUser(mydb& userDB, mydb& sysDB, DbTxn* txn, void* buff_in)
{
  userGUI guiInfo(buff_in);
  int32 usr_stat = chkUser(userDB, txn, guiInfo.getAccount());
  if (usr_stat == 1)
    return 1;
  if (usr_stat < 0)
    return -1;

  int32 uid = getSysID(sysDB, txn);
  if (uid <= 0)
    return -2;
  
  userRec dbUser;
  dbUser.setID(uid);
  dbUser.setAccount(guiInfo.getAccount());
  dbUser.setName(guiInfo.getName());
  dbUser.setPhone(guiInfo.getPhone());
  dbUser.setEmail(guiInfo.getEmail());
  dbUser.setAddress(guiInfo.getAddress());
  dbUser.setGroups(guiInfo.getGroups());

  string passwd = guiInfo.getPasswd();
  if (dbUser.gen_salt_and_verifier(guiInfo.getPasswd()))
    return -3;
  
  int32 bufLen = dbUser.getSize();
  char buff[bufLen];
  dbUser.serialize(buff);
  Dbt key(buff, sizeof(int32));
  Dbt data(buff, bufLen);
  if (userDB.getDb().put(txn, &key, &data, 0))
    return -4;

  return 0;
}

/* This function checks whether an input usr_name exists in user database or not.
 * Returns 0 if user does not exist; 
 * returns 1 if user exists;
 * returns -1 for db errors. */
int chkUser(mydb& userDB, DbTxn* txn, const string& usr_name)
{
  Dbc *cursorp = NULL;
  if (userDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  int ret;
  int status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    userRec uData(data.get_data());
    if (usr_name == uData.getAccount()) {
      status = 1;
      break;
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -1;

  cursorp->close();
  return status;
}

/* This function checks whether an input user's passwd is correct or not.
 * returns the user ID if input passwd matches;
 * returns 0 if user is not found in database;
 * returns -1 for db errors;
 * returns -2 if verifier generation fails;
 * returns -3 if user exists but passwd is incorrect. */
int chkPasswd(mydb& userDB, DbTxn* txn, const string& username, const string& passwd)
{
  Dbc *cursorp = NULL;
  if (userDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  int ret;
  int status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    userRec tmpData(data.get_data());
    if (username == tmpData.getAccount()) {
      gnutls_datum_t vf;
      if (make_verifier(username, passwd,tmpData.getSalt(),vf))
        status = -2;
      else if (vf.size==tmpData.getVerifier().size &&
               memcmp(vf.data,tmpData.getVerifier().data,vf.size)==0)
        status = tmpData.getID();
      else
        status = -3;      
      gnutls_free(vf.data);
      break;
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -1;

  cursorp->close();
  return status;
}

/* This function adds new permission record into permission table. 
 * returns 0 if the record is added successfully;
 * returns -1 for db errors. */
int addPerm(mydb& permDB, DbTxn* txn, const permRec& data_in)
{
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt key(buff, data_in.getAccessID().size() + 1);
  Dbt data(buff, size);

  if (permDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function collects a certain user's permissions and load them into permMap. 
 * Returns 0 if everything is ok;
 * returns -1 if group ID collection fails;
 * returns -2 for permission db search errors; */
int getPerm(mydb& userDB, mydb& permDB, DbTxn* txn, int32 uID, map<string, string>& permMap)
{
  vector<int32> grpList; 
  grpList.push_back(uID);
  int foo = getGrpIDs(userDB, txn, uID, grpList);
  if (foo)
    return -1;

  foo = getPerm(permDB, txn, grpList, permMap);
  if (foo < 0)
    return -2;

  return 0;
}

/* This function collects all permission records that a certain user has in permission db.
 * Returns 0 if no match is found;
 * returns 1 if any match is found in permission table;
 * returns -1 for db errors. */
int getPerm(mydb& permDB, DbTxn* txn, const vector<int32>& memberList, map<string, string>& permMap)
{
  Dbc *cursorp = NULL;
  if (permDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  // Iterate over permission table
  Dbt key, data;
  int ret;
  int status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    permRec tmpData(data.get_data());
    string aID = tmpData.getAccessID();
    if (isElement(aID, memberList)) {
      mergePerm(tmpData, permMap);
      status = 1;
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -1;

  cursorp->close();
  return status;
}

// Compare current permissions in the map with the new permission and merge them together
void mergePerm(const permRec& newPerm, map<string, string>& permMap)
{
  string datID = newPerm.getDataID();
  string permStr = newPerm.getPermission();
  map<string, string>::iterator it = permMap.find(datID);
  if (it == permMap.end() || it->second == "b" || permStr == "rw") 
    permMap[datID] = permStr;  
}

/* getGrpIDs() collects a certain user's group ID and put it into the output list.
 * returns 0 if everything is ok;
 * returns 1 if input uid not found in the table.
 * reutnrs 2 if group field can not be parsed successfully;
 * returns -1 for db errors; */
int getGrpIDs(mydb& userDB, DbTxn* txn, int32 uid, vector<int32>& grpList)
{
  Dbc *cursorp = NULL;
  if (userDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  key.set_data(&uid);
  key.set_ulen(sizeof(int32));
  int ret = cursorp->get(&key, &data, DB_SET);
  if (ret == DB_NOTFOUND) {
    cursorp->close();
    return 1;
  }
  else if (ret) {
    cursorp->close();
    return -1;
  }

  cursorp->close();
  userRec uData(data.get_data());
  string grpStr = uData.getGroups();
  if (grpStr.size()) {
    bool parse_stat = parseGrp(grpStr, " ", grpList);
    if (!parse_stat)
      return 2;
  }

  return 0;
}

/* This function adds a contact record into contact table. 
 * Returns 0 if everything is ok;
 * returns -1 for db errors;
 * returns -2 if unique ID cannot be retrieved or updated; */
int addContact(mydb& contactDB, mydb& sysDB, DbTxn* txn, contactRec& data_in)
{
  int32 recID = data_in.getID();
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size]; 
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (contactDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a study record into study table. 
 * Returns 0 if everything is ok.
 * returns -1 for db errors;
 * returns -2 if unique ID cannot be retrieved or updated; */
int addStudy(mydb& studyDB, mydb& sysDB, DbTxn* txn, studyRec& data_in)
{
  int32 recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (studyDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a user group into the corresponding table. 
 * Returns 0 if everything is ok;
 * returns -1 for db errors;
 * returns -2 if unique ID cannot be retrieved or updated; */
int addUserGrp(mydb& userGrpDB, mydb& sysDB, DbTxn* txn, userGrpRec& data_in)
{
  int32 recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size]; 
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (userGrpDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a user relationship record into corresponding table. 
 * Returns 0 if everything is ok;
 * returns -1 for db errors;
 * returns -2 if unique ID cannot be retrieved or updated; */
int addUserRelation(mydb& userRelationDB, mydb& sysDB, DbTxn* txn, userRelRec& data_in)
{
  int32 recID = data_in.getID();
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);

  Dbt data(buff, size);
  if (userRelationDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}


/* This function adds patient record into patient table. 
 * The patient ID should have been set in input data_in argument.
 * Returns 0 if everything is ok;
 * returns -1 for db errors; */
int addPatient(mydb& patientDB, DbTxn* txn, const patientRec& data_in)
{
  int32 recID = data_in.getID();
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (patientDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* Add a patient whose only private flag is set by the input flag.
 * This function first sets patient ID, then calls the generic 
 * addPatient() function defined above. 
 * Returns 0 if everything is ok;
 * returns -1 for db put error;
 * returns -2 if unique ID can not be retrived from system table.*/
int addPatient(mydb& patientDB, mydb& sysDB, DbTxn* txn, bool privateFlag)
{
  patientRec tmpRec;
  tmpRec.setPrivate(privateFlag);
  int32 recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;
  tmpRec.setID(recID);

  return addPatient(patientDB, txn, tmpRec);
}

/* This function adds patient group record into patient group table. 
 * returns 0 if everything is ok;
 * returns -1 for db errors;
 * returns -2 if unique ID cannot be retrieved or updated; */
int addPatientGrp(mydb& patientGrpDB, mydb& sysDB, DbTxn* txn, pgrpRec& data_in)
{
  int32 recID = data_in.getID();
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (patientGrpDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds patient group membership record into corresponding table. 
 * Returns 0 if everything is ok.
 * returns -1 for db errors; */
int addPgrpMember(mydb& patientGrpMemDB, DbTxn* txn, pgrpMemberRec& data_in)
{
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt key(buff, sizeof(int32));
  Dbt data(buff, size);

  if (patientGrpMemDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a session record into corresponding table.
 * Returns 0 if everything is ok,
 * returns -1 for db errors,
 * returns -2 if the unique ID can not be retrieved or updated. */
int addSession(mydb& sessionDB, mydb& sysDB, DbTxn* txn, DBsession& data_in)
{
  int32 recID = data_in.id;
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.id = recID;
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (sessionDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds one DBsession record into db file. 
 * Returns 0 if everything is ok; return -1 for db put error. */
int addSession(mydb& sessionDB, DbTxn* txn, const DBsession& session_in)
{
    int32 recID = session_in.id;
    Dbt key(&recID, sizeof(int32));
    int32 size = session_in.getSize();
    char buff[size];
    session_in.serialize(buff);
    Dbt data(buff, size);
    if (sessionDB.getDb().put(txn, &key, &data, 0))
      return -1;
    
    return 0;
}

/* This function adds a series of DBsession objects into db file. 
 * Returns 0 if everything is ok; return -1 for db put error. 
 * Note that in this function, we assume that each DBsession already has a valid ID. */
int addSession(mydb& sessionDB, DbTxn* txn, vector<DBsession>& sList, time_t t_stamp)
{
  for (vector<DBsession>::iterator it = sList.begin(); it != sList.end(); ++it) {
    it->date.setUnixTime(t_stamp);
    int32 recID = it->id;
    Dbt key(&recID, sizeof(int32));
    int32 size = it->getSize();
    char buff[size];
    it->serialize(buff);
    Dbt data(buff, size);
    if (sessionDB.getDb().put(txn, &key, &data, 0))
      return -1;
  }

  return 0;
}

/* This function adds a view record into corresponding table.
 * Returns 0 if everything is ok,
 * returns -1 for db errors,
 * returns -2 if the unique ID can not be retrieved or updated. */
int addView(mydb& viewDB, mydb& sysDB, DbTxn* txn, viewRec& data_in)
{
  int32 recID = data_in.getID();
  if (recID <= 0)
    recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  data_in.setID(recID);
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (viewDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a view entry record into view entry table.
 * Returns 0 if everything is ok,
 * returns -1 for db errors. */
int addViewEntry(mydb& viewEntryDB, DbTxn* txn, const viewEntryRec& data_in)
{
  int32 recID = data_in.getViewID();
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);
  
  if (viewEntryDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a certain score (aka. attribute) name into 
 * corresponding table.
 * Returns 0 if everything is ok,
 * returns -1 for db errors. 
 * Note that score name string is the key in score name table now. */
int addScoreName(mydb& scoreNameDB, DbTxn* txn, const DBscorename& data_in)
{
  Dbt key((char *) data_in.name.c_str(), data_in.name.size() + 1);
  int32 dat_size = data_in.getSize();
  char tmpBuf[dat_size];
  data_in.serialize(tmpBuf);
  Dbt data(tmpBuf, dat_size);
  if (scoreNameDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function adds a series of DBscorename objects into db file. 
 * Returns 0 if everything is ok; return a non-zero error code otherwise. */
int addScoreName(mydb& scoreNameDB, DbTxn* txn, const vector<DBscorename>& snList)
{
  for (vector<DBscorename>::const_iterator it = snList.begin(); it != snList.end(); ++it) {
    int stat = addScoreName(scoreNameDB, txn, *it);   
    if (stat)
      return stat;
  }

  return 0;
}

/* This function searches score flags and put the score names or descriptions 
 * into the output fields. 
 * Returns 0 if everything is ok, 
 * returns -1 for db errors. */
int getSearchFields(mydb& scoreNameDB, DbTxn* txn, vector<string>& out_fields)
{
  Dbc *cursorp = NULL;
  if (scoreNameDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  int ret;
  int stat = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    DBscorename sData(data.get_data());
    if (sData.flags.count("searchable"))
      out_fields.push_back(sData.name);
  }

  cursorp->close();
  return stat;
}

/* This function adds a vector of DBscorevalue objects into db file. 
 * Returns 0 if everything is ok; return non-zero error code otherwise. */
// int addScoreValue(mydb& scoreValueDB, DbTxn* txn, vector<DBscorevalue>& svList) 
// {
//   for (unsigned i = 0; i < svList.size(); i++) {
//     int stat = addScoreValue(scoreValueDB, txn, svList[i]);
//     if (stat)
//       return stat;
//   }

//   return 0;
// }

/* This function adds score value record into scorevalue table.
 * Returns 0 if everything is ok, 
 * returns -1 for db errors when getting key label;
 * returns -2 for db errors when adding new record into db table.
 * Note that score value ID should have already been assigned in data_in object 
 * before this function is called. */
int addScoreValue(mydb& scoreValueDB, DbTxn* txn, DBscorevalue& data_in)
{
  int label = getKeyLabel(scoreValueDB, txn, data_in.id, data_in.index);
  if (label < 0)
    return -1;

  data_in.key = strnum(data_in.id) + "-" + strnum(data_in.index) + "-" + strnum(label);
  Dbt key((void*) data_in.key.c_str(), data_in.key.size() + 1);
  int32 size = data_in.getSize();
  char* buff = new char[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (scoreValueDB.getDb().put(txn, &key, &data, 0)) {
    delete [] buff;
    return -1;
  }

  delete [] buff;
  return 0;
}

/* Returns the next available integer that will be appended at the end of score value key string.
 * return -1 for cursor error, 
 * returns -2 if label is negative;
 * returns a non-negative integer if everything is ok. */
int getKeyLabel(mydb& scoreValueDB, DbTxn* txn, int32 svID, uint32 index)
{
  int foo = -1;
  Dbc *cursorp = NULL;
  if (scoreValueDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  Dbt key, data;
  int ret;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    DBscorevalue svData(data.get_data());
    if (svData.id == svID && svData.index == index) {
      string tmp_key = svData.key;
      int label_pos = tmp_key.rfind("-");
      string label_str = tmp_key.substr(label_pos + 1, tmp_key.size() - label_pos - 1);
      int label = atoi(label_str.c_str());
      if (label < 0)
	return -2;
      if (label > foo)
	foo = label;
    }
  }

  cursorp->close();
  return foo + 1;
} 

/* Overloaded function that adds a score value record into scorevalue table.
 * In this function, score value ID has to be set by calling system DB.
 * Returns 0 if everything is ok, returns -1 for db errors, 
 * returns -2 if unique ID can not be retrieved or updated in system DB. 
 * This function is only used by command line to initialize score value table. */
int addScoreValue(mydb& scoreValueDB, mydb& sysDB, DbTxn* txn, DBscorevalue& data_in)
{
  int32 recID = getSysID(sysDB, txn);
  if (recID <= 0)
    return -2;

  // new score value always ends with "-0"
  data_in.key = strnum(recID) + "-" + strnum(data_in.index) + "-0";
  data_in.id = recID;
  Dbt key((void*) data_in.key.c_str(), data_in.key.size() + 1);
  int32 size = data_in.getSize();
  char* buff = new char[size];
  data_in.serialize(buff);
  Dbt data(buff, size);

  if (scoreValueDB.getDb().put(txn, &key, &data, 0)) {
    delete [] buff;
    return -1;
  }

  delete [] buff;
  return 0;
}

/* This function adds a vector of DBpatientlist objects into db file. 
 * Returns 0 if everything is ok; return non-zero error code otherwise. */
int addPatientList(mydb& patientListDB, DbTxn* txn, const vector<DBpatientlist>& inputList) 
{
  for (unsigned i = 0; i < inputList.size(); i++) {
    int stat = addPatientList(patientListDB, txn, inputList[i]);
    if (stat)
      return stat;
  }

  return 0;
}

/* Add one patient list record into database.
 * returns -1 for db errors;
 * returns 0 if everything is ok. */
int addPatientList(mydb& patientListDB, DbTxn* txn, const DBpatientlist& data_in)
{
  int32 recID = data_in.id;
  Dbt key(&recID, sizeof(int32));
  int32 size = data_in.getSize();
  char buff[size];
  data_in.serialize(buff);
  Dbt data(buff, size);
  if (patientListDB.getDb().put(txn, &key, &data, 0))
    return -1;

  return 0;
}

/* This function collects patient list records that belongs to the input user 
 * or the user's groups and put them into pList. 
 * Returns 0 if everything is ok;
 * returns -1 if group ID collection fails;
 * returns -2 for patient list db search errors; */
int getPatientList(mydb& userDB, mydb& patientListDB, DbTxn* txn, int32 uID, vector<DBpatientlist>& pList)
{
  vector<int32> memberList; 
  memberList.push_back(uID);
  int foo = getGrpIDs(userDB, txn, uID, memberList);
  if (foo)
    return -1;

  Dbc *cursorp = NULL;
  if (patientListDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  // Iterate over permission table
  Dbt key, data;
  int ret;
  int status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    DBpatientlist tmpData(data.get_data());
    int32 oID = tmpData.ownerID;
    if (isElement(oID, memberList)) {
      pList.push_back(tmpData);
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -2;

  cursorp->close();
  return status;
}

/* This function searches patient score values and collects the brief info 
 * of matched score value record into the last vexctor argument. 
 * returns 0 if there is no error;
 * Returns -1 for db errors. */
int searchPatients(mydb& scoreValueDB, DbTxn* txn, const patientSearchTags& tags_in, 
		   const map<string, string>& permMap, vector<patientMatch>& outList)
{
  Dbc *cursorp = NULL;
  if (scoreValueDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  string score_in = tags_in.scoreName;
  string rel_in = tags_in.relationship;
  string val_in = tags_in.searchStr;
  bool case_in = tags_in.case_sensitive;
  Dbt key, data;
  int ret;
  int stat = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 ) {
    DBscorevalue svData(data.get_data());
    // skip records that are not permitted to read or are deleted
    if (!readPermitted(svData, permMap) || svData.deleted)
      continue;

    string dbScorename = svData.scorename;
    string dbVal = svData.getDatStr();    
    // If score value data can not converted to string, skip it
    if (dbVal.size() == 0)
      continue;

    int32 dbPid = svData.patient;
    // if refinery list is not empty and patient ID is not on the list, skip the record
    if (tags_in.patientIDs.size() && !isElement(dbPid, tags_in.patientIDs))
      continue;
    // if input score ID is non-zero and does not match scoreID in db record, skip it
    if (score_in.size() && score_in != dbScorename)
      continue;
    /* Compare input string with score value. If they match and patient ID 
     * is not collected yet, insert the basic patient information into output vector. */  
    if (cmpStr(dbVal, val_in, case_in, rel_in)) {
      int32 pIndex = getPatientIndex(dbPid, outList);
      if (pIndex == -1) {
	patientMatch tmpMatch(dbPid, dbScorename, dbVal);
	outList.push_back(tmpMatch);
      }
      else {
	outList[pIndex].addScore(dbScorename, dbVal);
      }  
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    stat = -1;

  cursorp->close();
  return stat;
}

/* Returns true if the input score value record has 
 * at least read permission, returns false otherwise. */
bool readPermitted(const DBscorevalue& svRec, const map<string, string>& permMap)
{
  vector<string> ids;
  ids.push_back("*");
  ids.push_back(strnum(svRec.id));
  ids.push_back(strnum(svRec.patient));
  ids.push_back(svRec.scorename);
  ids.push_back(strnum(svRec.sessionid));
  for (uint i = 0; i < ids.size(); ++i) {
    map<string, string>::const_iterator it = permMap.find(ids[i]);
    if (it == permMap.end())
      continue;
    if (it->second == "r" || it->second == "rw")
      return true;
  }

  return false;
}

/* This function gets all score values and sessions of an input patient ID. 
 * Returns 0 if everything is ok;
 * returns -1 if score values can not be retrieved;
 * returns -2 if sessions can not be retrieved; */
int getOnePatient(mydb& scoreValueDB, mydb& sessionDB, DbTxn* txn, int32 pID, 
		  const map<string, string>& permMap, DBpatient& patient_out)
{
  Dbc* cursorp = NULL;
  if (scoreValueDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  // Iterate over score value table
  set<int32> sidList;
  Dbt key, data;
  int ret;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    void* db_buf = data.get_data();
    DBscorevalue svData;
    // only deserialize header now (to save memory)
    svData.deserializeHdr(db_buf);  
    // skip record if patient ID doesn't match or "deleted" flag is set
    if (pID != svData.patient || svData.deleted) 
      continue; 
    string newPerm = getMaxPerm(svData, permMap);
    if (newPerm.empty())
      continue;

    int32 tmpID = svData.id;
    patient_out.scores.insert(map<int32, DBscorevalue>::value_type(tmpID, svData)); 
    patient_out.scores[tmpID].deserialize(db_buf);   // deserialize everything now
    patient_out.scores[tmpID].permission = newPerm;  // reset permission

    /* Add session IDs into session_list vector (session 0 is ignored here 
     * because this session doesn't exist in session table anyway) */
    int32 sess_id = svData.sessionid;
    if (sess_id)
      sidList.insert(sess_id);	
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND) {
    cursorp->close();
    return -1;
  }

  cursorp->close();

  // get related DBsession
  if (getSessions(sessionDB, txn, sidList, patient_out.sessions) < 0)
    return -2;
  
  patient_out.resetMaps();
  patient_out.patientID=pID;
  return 0;
}

/* This function collects all score values of a certain patient and 
 * put them into an array of DBscorevalue object. 
 * Returns buffer size of output array if everything is ok;
 * returns 0 if no score value record matches search criteria;
 * returns -1 for db errors. */
int32 getScoreValues(mydb& scoreValueDB, DbTxn* txn, int32 patientID, const map<string, string>& permMap, 
		     vector<DBscorevalue>& sv_list, set<int32>& sid_set)
{
  Dbc* cursorp = NULL;
  if (scoreValueDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  // Iterate over test result (aka. score value) table
  Dbt key, data;
  int ret;
  int32 status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
    DBscorevalue svData(data.get_data());
    if (patientID != svData.patient)
      continue;
    string tmpPerm = getMaxPerm(svData, permMap);
    if (tmpPerm.empty())
      continue;
    
    // Add real score value records into DBscorevalue vector.
    svData.permission = tmpPerm;
    sv_list.push_back(svData);
    status += svData.getSize();
    /* Add session IDs into session_list vector (session 0 is ignored here 
     * because this session doesn't exist in session table anyway) */
    int32 sess_id = svData.sessionid;
    if (sess_id)
      sid_set.insert(sess_id);	
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND)
    status = -1;

  cursorp->close();
  return status;
}

// Return the maximum permission available in permission map
string getMaxPerm(const DBscorevalue& svRec, const map<string, string>& permMap)
{
  vector<string> ids;
  ids.push_back("*");
  ids.push_back(strnum(svRec.id));
  ids.push_back(strnum(svRec.patient));
  ids.push_back(svRec.scorename);
  ids.push_back(strnum(svRec.sessionid));
  string maxPerm;
  for (uint i = 0; i < ids.size(); ++i) {
    map<string, string>::const_iterator it = permMap.find(ids[i]);
    if (it == permMap.end())
      continue;

    if (maxPerm.empty() || it->second == "rw" || maxPerm == "b")
      maxPerm = it->second;
  }

  return maxPerm;
}

/* Collects DBsession information from the database. 
 * Returns the length of buffers if everything is ok;
 * returns -1 for cursor errors;
 * returns -2 if a session ID is not found in database;
 * returns -3 for other db errors. */
int32 getSessions(mydb& sessionDB, DbTxn* txn, const set<int32>& sid_set, 
		  vector<DBsession>& session_list)
{
  Dbc* cursorp = NULL;
  if (sessionDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  int32 status = 0;
  set<int32>::const_iterator it;
  for (it = sid_set.begin(); it != sid_set.end(); ++it) {
    int32 tmpID = *it;
    Dbt key(&tmpID, sizeof(int32)); 
    Dbt data;
    int get_stat = cursorp->get(&key, &data, DB_SET);
    if (get_stat == DB_NOTFOUND) {
      cursorp->close();
      return -2;
    }
    else if (get_stat) {
      cursorp->close();
      return -3;
    }
    DBsession tmpSess(data.get_data());
    session_list.push_back(tmpSess);
    status += tmpSess.getSize();
  }

  cursorp->close();
  return status;
}

/* Collects DBsession information from the database. 
 * Returns the length of buffers if everything is ok;
 * returns -1 for cursor errors;
 * returns -2 if a session ID is not found in database;
 * returns -3 for other db errors. */
int32 getSessions(mydb& sessionDB, DbTxn* txn, const set<int32>& sid_set, 
		  map<int32, DBsession>& out_map)
{
  Dbc* cursorp = NULL;
  if (sessionDB.getDb().cursor(txn, &cursorp, 0))
    return -1;

  int32 status = 0;
  set<int32>::const_iterator it;
  for (it = sid_set.begin(); it != sid_set.end(); ++it) {
    int32 tmpID = *it;
    Dbt key(&tmpID, sizeof(int32)); 
    Dbt data;
    int get_stat = cursorp->get(&key, &data, DB_SET);
    if (get_stat == DB_NOTFOUND) {
      cursorp->close();
      return -2;
    }
    else if (get_stat) {
      cursorp->close();
      return -3;
    }
    DBsession tmpSess(data.get_data());
    out_map[tmpID] = tmpSess;
    status += tmpSess.getSize();
  }

  cursorp->close();
  return status;
}

// Extract patient ID out of DBscorevalue record, used to build secondary db based on score value table
int getPID(Db*, const Dbt*, const Dbt* pdata, Dbt* skey)
{
  DBscorevalue tmpRec(pdata->get_data());
  int32 pid = tmpRec.patient;
  skey->set_data(&pid);
  skey->set_size(sizeof(int32));

  return 0;
}

