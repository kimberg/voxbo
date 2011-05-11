/* server.cpp includes functions that are solely called by database server. */
using namespace std;

#include "server.h"
#include <iostream>
#include <list>
#include <arpa/inet.h>
#include <csignal>
//#include <QtGui/QApplication>

DBdata srvSession::dbs;
int32 currentUser = 0;

// Initialize server session
void srvSession::init()
{
  clientID = 0;
  gnutls_init(&g_session, GNUTLS_SERVER);
  gnutls_priority_set_direct(g_session, "NORMAL:+SRP", NULL);
}

// Quit server session
void srvSession::exit()
{
  permissions.clear();
  gnutls_bye(g_session, GNUTLS_SHUT_WR);
  close(sd);
  gnutls_deinit(g_session);
}

/* Send global map variables to client side
 * Returns -1 if score types are not sent out to client;
 * returns -2 if score names are not sent out to client;
 * returns -3 if brain region names are not sent out to client;
 * returns -4 if brain region synonyms are not sent out to client;
 * returns -5 if brain region relationships are not sent out to client. */
int srvSession::sendGlobalMaps()
{
  if (sendTypes())
    return -1;
  if (sendScoreNames())
    return -2;
  if (sendRegionNames())
    return -3;
  if (sendSynonyms())
    return -4;
  if (sendRegionRelations())
    return -5;

  return 0;
}

/* Send patient list to client side.
 * Returns 0 if patient list is sent out to client, 
 * or no patient list is available and the message is sent out to client;
 * returns -1 if error occurs when searching patient list and error message is not sent out successfully;
 * returns  1 if error occurs when searching patient list but error message is sent out successfully;
 * returns -2 if no_patientlist message is not sent out successfully;
 * returns -3 if patient list data are not sent out successfully. */
int srvSession::sendPatientList()
{
  vector<DBpatientlist> vec;
  int foo = getPatientList(dbs.userDB, dbs.patientListDB, NULL, clientID, vec);
  if (foo) {
    statMsg = "no_data: db error code " + strnum(foo);
    if (sendStatMsg()) 
      return -1;
    return 1;
  }
  else if (!vec.size()) {
    statMsg = "no_patientlist";
    if (sendStatMsg())
      return -2;
    return 0;
  }

  int32 dat_size = getSize(vec);
  char buffer[dat_size];
  serialize(vec, buffer);

  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending patient list from server to client: " + statMsg;
    return -3;
  }

  return 0;
}

/* Send data from server to client.
 * Returns -1 if data size message not sent out successfully;
 * returns -2 id data not sent out successfully;
 * retunrs 0 if everything is ok. */
int srvSession::sendData(int32 dat_size, char* buffer)
{
  // First send a message that has buffer size info
  string tmpMsg = "server_data_size: " + num2str(dat_size);
  if (sendMessage(g_session, tmpMsg)) {
    statMsg = "failed to send data size";
    return -1;
  }

  // Now send the real data
  if (sendBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to send data";
    return -2;
  }

  return 0;
}

/* Build DBtype data buffer and send it to client side.
 * Returns 0 if everything is ok;
 * returns -1 for sending error. */
int srvSession::sendTypes()
{
  vector<DBtype> vec;
  map<string, DBtype>::const_iterator iter;
  int32 dat_size = 0;
  for (iter = dbs.typemap.begin(); iter != dbs.typemap.end(); ++iter) {
    vec.push_back(iter->second);
    dat_size += iter->second.getSize();
  }
  char buffer[dat_size];
  serialize(vec, buffer);

  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending score types to client: " + statMsg;
    return -1;
  }

  return 0;
}

/* Build DBscorename data buffer and send it to client side
 * Returns 0 if everything is ok;
 * returns -1 for sending error. */
int srvSession::sendScoreNames()
{
  vector<DBscorename> vec;
  map<string, DBscorename>::const_iterator iter;
  int32 dat_size = 0;
  for (iter = dbs.scorenames.begin(); iter != dbs.scorenames.end(); ++iter) {
    vec.push_back(iter->second);
    dat_size += iter->second.getSize();
  }

  char buffer[dat_size];
  serialize(vec, buffer);

  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending score names to client: " + statMsg;
    return -1;
  }

  return 0;
}

/* Build brain region names data buffer and send it to client side
 * Returns 0 if everything is ok;
 * returns -1 for sending error. */
int srvSession::sendRegionNames()
{
  vector<regionRec> vec;
  map<int32, regionRec>::const_iterator iter;
  int32 dat_size = 0;
  for (iter = dbs.regionNameMap.begin(); iter != dbs.regionNameMap.end(); ++iter) {
    vec.push_back(iter->second);
    dat_size += iter->second.getSize();
  }
 
  char* buffer = new char[dat_size];
  serialize(vec, buffer);
  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending brain region names to client: " + statMsg;
    delete [] buffer;
    return -1;
  }
 
  delete [] buffer;
  return 0;
}

/* Build synonyms data buffer and send it to client side.
 * Returns 0 if everything is ok;
 * returns -1 for sending error. */
int srvSession::sendSynonyms()
{
  vector<synonymRec> vec;
  map<int32, synonymRec>::const_iterator iter;
  int32 dat_size = 0;
  for (iter = dbs.synonymMap.begin(); iter != dbs.synonymMap.end(); ++iter) {
    vec.push_back(iter->second);
    dat_size += iter->second.getSize();
  }
  char buffer[dat_size];
  serialize(vec, buffer);
  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending brain region synonyms to client: " + statMsg;
    return -1;
  }

  return 0;
}

/* Build region relationships data buffer and send it to client side.
 * Returns 0 if everything is ok;
 * returns -1 for sending error. */
int srvSession::sendRegionRelations()
{
  vector<regionRelationRec> vec;
  map<int32, regionRelationRec>::const_iterator iter;
  int32 dat_size = 0;
  for (iter = dbs.regionRelationMap.begin(); iter != dbs.regionRelationMap.end(); ++iter) {
    vec.push_back(iter->second);
    dat_size += iter->second.getSize();
  }
  char buffer[dat_size];
  serialize(vec, buffer);

  if (sendData(dat_size, buffer)) {
    statMsg = "Error when sending brain region relationships to client: " + statMsg;
    return -1;
  }

  return 0;
}

// Serve client's requests based on message from client
void srvSession::serve(const char* inputMsg)
{
  cout << "message from client: " << inputMsg << endl; // for debugging purpose only

  // Do nothing if the client logs out
  if (strcmp(inputMsg, "logout") == 0) 
    return;
  
  c_tokens.clear();
  c_tokens.SetSeparator(" ");
  c_tokens.ParseLine(inputMsg);
  // client request of user info
  int stat = 0;
  if (c_tokens[0] == "req_usr_info:" && c_tokens.size() == 2) {
    stat = sendUsrInfo();
  }
  // client request of unique ID
  else if (c_tokens[0] == "req_id:" && c_tokens.size() == 2) {
    stat = sendID();
  }
  // client request of server's current time stamp
  else if (c_tokens[0] == "req_time" && c_tokens.size() == 1) {
    stat = sendTime();
  }
  // client request of patient search
  else if (c_tokens[0] == "search_patient:") {
    stat = sendPatientMatches();
  }
  // client request of all score values of a certain patient that he/she is permitted to access
  else if (c_tokens[0] == "req_patient:" && c_tokens.size() == 2) {
    stat = sendPatientScores();
  }
  // Client sends new/updated user info data
  else if (c_tokens[0] == "put_new_user:" && c_tokens.size()  == 2) {  
    stat = recvNewUsr();
  }
  // Client sends new or updated score name data 
  else if (c_tokens[0] == "put_scorename:" && c_tokens.size()  == 2) {  
    stat = recvScoreNames(); 
  }
  // Client sends new patient data
  else if (c_tokens[0] == "put_patient:" && c_tokens.size()  == 4) {  
    stat = recvNewPatient(); 
  }
  // Client sends new or updated score value record
  else if (c_tokens[0] == "put_scorevalue:" && c_tokens.size() == 2) { 
    stat = recvScoreValues();
  }
  // Client sends new or updated session data
  else if (c_tokens[0] == "put_session:" && c_tokens.size() == 2) {  
    stat = recvSessions(); 
  }
  // Client sends new patient list data
  else if (c_tokens[0] == "put_patientlist:" && c_tokens.size() == 2) {  
    stat = recvPatientList(true);
  }
  // Client sends modified patient list data
  else if (c_tokens[0] == "put_patientlist:" && c_tokens.size() == 2) {  
    stat = recvPatientList(false);
  }
  //
  // more options to come ...
  //
  else {
    statMsg = (string) "Client message not understandable: " + inputMsg;
    stat = sendStatMsg();
  }

  // print out status message (for debug only)
  if (stat) 
    cout << statMsg << endl;
}

/* Send a feedback message back to client after receiving a put message 
 * from client to tell the client server's operation status.
 * Returns 0 if message is sent out; returns 1 otherwise. */
int srvSession::sendStatMsg()
{
  if (sendMessage(g_session, statMsg)) { 
    statMsg += "(failed to send message to client)";
    return 1;
  }

  statMsg += "(message sent to client)"; 
  return 0;
}

/* Send user information to client. 
 * Message syntax check ignored !!!
 * Returns 0 if everything is ok;
 * retunrs -1 if user is not found and message is not sent out successfully;
 * returns  1 if user is not found but message is sent out successfully; 
 * returns -2 if error occurs when browsing user table, and error message is not sent out successfully;
 * returns  2 if error occurs when browsing user table, but error message is sent out successfully;
 * returns  3 if user data is not sent out successfully. */
int srvSession::sendUsrInfo()
{
  userRec myInfo;
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int foo = getUser(dbs.userDB, txn, c_tokens[1], myInfo);
  if (foo != 1) {
    if (foo == 0) {
      txn->commit(0);
      statMsg = "no_data: User not found";
      if (sendStatMsg())
	return -1;
      return 1;
    }
    else {
      txn->abort();
      statMsg = "no_data: DB error " + num2str(foo);
      if (sendStatMsg())
	return -2;
      return 2;
    }
  }

  txn->commit(0);
  int data_size = myInfo.getSize();
  char data_buf[data_size];
  myInfo.serialize(data_buf);
  // Send data to client
  if (sendData(data_size, data_buf)) {
    statMsg = "Error when sending user data to client: " + statMsg;
    return 3;
  }

  return 0;
}

/* Send unique IDs to client side. 
 * Returns  0 if everything is ok;
 * returns -1 if request from client is invalid and error message is not sent out successfully;
 * returns  1 if request from client is invalid but error message is sent out successfully;
 * returns -2 if error occurs when getting ID from db and error message is not sent out successfully; 
 * returns  2 if error occurs when getting ID from db but error message is sent out successfully; 
 * returns -3 if size of data is not sent out successfully;
 * returns -4 if data are not sent out successfully. */
int srvSession::sendID()
{
  int32 id_num;
  bool conv_stat = str2num(c_tokens[1], id_num);
  if (!conv_stat || id_num <= 0) { 
    statMsg = "no_data: Invalid number from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int32 startID = getSysID(dbs.sysDB, txn, id_num);
  if (startID <= 0) {
    txn->abort();
    statMsg = "no_data: DB error";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  txn->commit(0);
  dbs.sysDB.getDb().sync(0); // write data on disk now    
  statMsg = "server_data_size: " + num2str(sizeof(startID)); 
  if (sendStatMsg()) {
    statMsg = "Error when sending ID to client: failed to send data size message"; 
    return -3;
  }

  if (ntohs(1) == 1)
    swap(&startID);
  if (gnutls_record_send(g_session, &startID, sizeof(startID)) != sizeof(startID)) {
    statMsg = "Error when sending ID to client: failed to send data";
    return 4;
  }

  return 0;
}

/* Send server's current time stamp to client side. 
 * Return 0 if everything is ok;
 * returns -1 if time stamp can not be sent to client; */
int srvSession::sendTime()
{
  int32 t_stamp = time(NULL);
  if (ntohs(1) == 1)
    swap(&t_stamp);

  if (gnutls_record_send(g_session, &t_stamp, sizeof(t_stamp)) != sizeof(t_stamp)) {
    statMsg = "Failed to send time stamp to client";
    return -1;
  }

  return 0;
}

/* This function checks and parses client message. 
 * If the syntax error is found, set the error msg and return false; 
 * otherwise set the search tags and retuirn true. */
bool srvSession::parsePatientSearch(patientSearchTags& tags)
{
  tags.init();

  if (c_tokens.size() < 5 || c_tokens.size() == 6) {
    tags.err_msg.append("can not understand client message.");
    return false;
  }

  if (c_tokens[1] != "case_sensitive" && c_tokens[1] != "case_insensitive") {
    tags.err_msg.append("can not understand case_sensitivity tag "); 
    tags.err_msg.append(c_tokens[1]);
    return false;
  }

  if (c_tokens[2] != "any") {
    int32 dat_size;
    bool conv_stat = str2num(c_tokens[2], dat_size);
    if (!conv_stat || dat_size <= 0) {
      tags.err_msg.append("invalid score ID ");
      tags.err_msg.append(c_tokens[2]);
      return false;
    }
  }

  if (c_tokens[3] != "equal" && c_tokens[3] != "include" && c_tokens[3] != "wildcard") {
    tags.err_msg.append("unknown relationship string ");
    tags.err_msg.append(c_tokens[3]);
    return false;
  }

  if (c_tokens[1] == "case_sensitive")
    tags.case_sensitive = true;
  else
    tags.case_sensitive = false;

  if (c_tokens[2] == "any") 
    tags.scoreName = "";
  else {
    tags.scoreName = c_tokens[2];
  }

  tags.relationship = c_tokens[3];
  tags.searchStr = c_tokens[4];

  if (c_tokens.size() == 5)
    return true;

  if (c_tokens[5] != "among") {
    tags.err_msg.append("can not understand client message.");
    return false;
  }

  for (size_t i = 6; i < c_tokens.size(); i++) {
    int32 pID;
    bool conv_stat = str2num(c_tokens[i], pID);
    if (!conv_stat || pID <= 0) {
      tags.err_msg.append("Unknown patient ID string: ").append(c_tokens[i]);
      tags.patientIDs.clear();
      return false;
    }
    tags.patientIDs.push_back(pID);
  }
	
  return true;
}

/* Get a brief list of patients that matched cleint search criteria. 
 * Returns  0 if patient is found and data are sent out successfully, 
 * or no patient is found and message is sent out successfully;
 * returns -1 if permission is denied and error message is not sent out successfully;
 * returns  1 if permission is denied but error message is sent out successfully;
 * returns -2 if search criterion is invalid and error message is not sent out successfully;
 * returns  2 if search criterion is invalid but error message is sent out successfully;
 * returns -3 if error occurs when searching patient and error message is not sent out successfully;
 * returns  3 if error occurs when searching patient but error message is sent out successfully;
 * returns -4 if no patient is found in db and message is not sent out successfully;
 * returns -5 if patient is found but data are not sent out successfully. */
int srvSession::sendPatientMatches()
{
  if (!permissions.size()) {
    statMsg = "Permission denied";
    if (sendStatMsg())
      return -1;
    return 1;
  }

  patientSearchTags ps_tags;
  if (!parsePatientSearch(ps_tags)) {
    statMsg = ps_tags.err_msg;
    if (sendStatMsg())
      return -2;
    return 2;
  }

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  vector<patientMatch> currentPatients;
  int foo = searchPatients(dbs.scoreValueDB, txn, ps_tags, permissions, currentPatients);
  if (foo < 0) {
    txn->abort();
    statMsg = "no_data: DB error ";
    if (sendStatMsg())
      return -3;
    return 3;
  }

  txn->commit(0);
  if (!currentPatients.size()) {
    statMsg = "no_data: Patient not found";   
    if (sendStatMsg())
      return -4;
    return 0;
  }

  int32 dat_size = getSize(currentPatients);
  char dat_buffer[dat_size];
  serialize(currentPatients, dat_buffer);
  if (sendData(dat_size, dat_buffer)) {
    statMsg = "Error when sending patient search results to client: " + statMsg;
    return -5;
  }

  return 0;
}

/* Get all scores of a certain patient. 
 * Returns 0 if everything is ok;
 * returns -1 if permission is denied, and error message is not sent out successfully;
 * returns  1 if permission is denied, but error message is sent out successfully;
 * returns -2 if patient ID is invalid, and error message is not sent out successfully;
 * returns  2 if patient ID is invalid, but error message is sent out successfully;
 * returns -3 if error occurs when searching score value table, and error message is not sent out successfully;
 * returns  3 if error occurs when searching score value table, but error message is sent out successfully;
 * returns -4 if no score value record is found, and error message is not sent out successfully;
 * returns  4 if no score value record is found, but error message is sent out successfully;
 * returns -5 if data size message is not sent out successfully;
 * returns -6 if score value data are not sent out successfully;
 * returns -7 if no session is available, and the message is not sent out successfully;
 * returns -8 if error occurs when searching session table, and error message is not sent out successfully;
 * returns  8 if error occurs when searching session table, but error message is sent out successfully;
 * returns -9 if session data are not sent out successfully. */
int srvSession::sendPatientScores()
{
  if (!permissions.size()) {
    statMsg = "Permission denied";
    if (sendStatMsg())
      return -1;
    return 1;
  }

  // send error message to client if patient ID is invalid or score values not found
  int32 pID;
  bool conv_stat = str2num(c_tokens[1], pID);
  if (!conv_stat || pID <= 0) {
    statMsg = "no_data: Invalid patient ID " + c_tokens[1];
    if (sendStatMsg())
      return -2;
    return 2;
  }

  // get score values
  set<int32> sid_set;
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  Dbc* cursorp = NULL;
  if (dbs.scoreValueDB.getDb().cursor(txn, &cursorp, 0)) {
    txn->abort();    
    statMsg = "no_data: db error when searching score value table";
    if (sendStatMsg())
      return -3;
    return 3;
  }

  // Iterate over score values table
  Dbt key, data;
  int ret;
  int32 status = 0;
  while ((ret = cursorp->get(&key, &data, DB_NEXT_NODUP)) == 0) {
    DBscorevalue svData;
    svData.deserializeHdr(data.get_data());
    if (pID != svData.patient || svData.deleted)
      continue;
    string tmpPerm = getMaxPerm(svData, permissions);
    if (tmpPerm.empty())
      continue;
    
    /* Add session IDs into session_list vector (session 0 is ignored here 
     * because this session doesn't exist in session table anyway) */
    int32 sess_id = svData.sessionid;
    if (sess_id)
      sid_set.insert(sess_id);	

    // Add real score value records into DBscorevalue vector.
    status = 1;
    uint32 dat_size = data.get_size();
    statMsg = "scorevalue: " + num2str(dat_size) + " " + tmpPerm;
    if (sendStatMsg()) {
      cursorp->close();
      txn->abort();
      return -4;
    }
    if (sendBuffer(g_session, dat_size, (char*) data.get_data())) {
      statMsg = "Error when sending score value to client";
      cursorp->close();
      txn->abort();
      return -4;
    }
  }

  // Returns -1 if ret is non-zero and it doesn't reach the end of table 
  if (ret && ret != DB_NOTFOUND) {
    cursorp->close();
    txn->abort();    
    statMsg = "no_data: db error when searching score value table";
    if (sendStatMsg())
      return -3;
    return 3;
  }

  cursorp->close();

  if (status == 0) {
    txn->commit(0);    
    statMsg = "no_scorevalue";
    if (sendStatMsg())
      return -5;
    return 5;
  }

  // If session ID list is empty, tell client that no session info sent out
  if (sid_set.size() == 0) {
    txn->commit(0);
    statMsg = "no_session";
    if (sendStatMsg())
      return -7;
    return 0;
  }

  // get related DBsession
  vector<DBsession> sessList;
  int32 dat_size = getSessions(dbs.sessionDB, txn, sid_set, sessList);
  // Note that dat_size may be zero if all score values are static
  if (dat_size <= 0) { 
    txn->abort();
    statMsg = "no_data: Session record not found";
    if (sendStatMsg())
      return -8;
    return 8;
  }
  
  txn->commit(0);
  // send session data to client
  char sess_buff[dat_size];
  serialize(sessList, sess_buff);
  if (sendData(dat_size, sess_buff)) {
    statMsg = "Error when sending patient session data to client: " + statMsg;
    return -9;
  }
   
  return 0;
}

/* Receive updated user information data from client. 
 * Returns 0 if everything is ok;
 * returns -1 if data size from client is invalid, and this error message is not sent out successfully;
 * returns  1 if data size from client is invalid, but this error message is sent out successfully;
 * returns -2 if user data is not received, and this error error message is not sent out successfully;
 * returns  2 if user data is not received, but this error error message is sent out successfully;
 * returns -3 if user account received already exists, and this error message is not sent out successfully;
 * returns  3 if user account received already exists, but this error message is sent out successfully;
 * returns -4 if error occurs when searching user table, and error message is not sent out successfully;
 * returns  4 if error occurs when searching user table, but error message is sent out successfully;
 * returns -5 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  5 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -6 if server confirmation message is not sent out successfully. */
int srvSession::recvNewUsr()
{
  int32 dat_size;
  bool conv_stat = str2num(c_tokens[1], dat_size);
  if (!conv_stat || dat_size <= 0) {
    statMsg = "Invalid buffer size from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to receive new user data from client";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int stat = addUser(dbs.userDB, dbs.sysDB, txn, buffer);
  if (stat == 1) {
    txn->commit(0);
    statMsg = "Account already exists.";
    if (sendStatMsg())
      return -3;
    return 3;
  }

  if (stat) {
    txn->abort();
    statMsg = "DB error when adding records into user table";
    if (sendStatMsg())
      return -4;
    return 4;
  }

  if (setSysUpdate(dbs.sysDB, txn, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -5;
    return 5;
  }

  txn->commit(0);
  statMsg = "success";
  if (sendStatMsg())
    return -6;

  dbs.sysDB.getDb().sync(0);
  dbs.userDB.getDb().sync(0); // write data on disk now
  return 0;
}

/* Receive score name data from client side. 
 * Returns 0 if everything is ok;
 * returns -1 if data size from client is invalid, and this error message is not sent out successfully;
 * returns  1 if data size from client is invalid, but this error message is sent out successfully;
 * returns -2 if data is not received, and this error error message is not sent out successfully;
 * returns  2 if data is not received, but this error error message is sent out successfully;
 * returns -3 if error occurs when searching db, and error message is not sent out successfully;
 * returns  3 if error occurs when searching db, but error message is sent out successfully;
 * returns -4 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  4 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -5 if server confirmation message is not sent out successfully. */
int srvSession::recvScoreNames()
{
  int32 dat_size;
  bool conv_stat = str2num(c_tokens[1], dat_size); 
  if (!conv_stat || dat_size <= 0) {
    statMsg = "Invalid score name data size from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to receive score names from client";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  vector<DBscorename> snList;
  deserialize(dat_size, buffer, snList);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  if (addScoreName(dbs.scoreNameDB, txn, snList)) {
    txn->abort();
    statMsg = "DB error when adding score name record into score name table";
    if (sendStatMsg())
      return -3;
    return 3;
  }  

  if (setSysUpdate(dbs.sysDB, txn, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -4;
    return 4;
  }

  txn->commit(0);
  dbs.scoreNameDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);
  // Update score name maps on server side
  vbforeach(DBscorename sn, snList) {
    dbs.add_scorename(sn);
  }

  statMsg = "success";
  if (sendStatMsg())
    return -5;

  return 0;
}

/* Receive updated patient score data from client. 
 * Returns 0 if everything is ok;
 * returns -1 if data size from client is invalid, and error message is not sent out successfully;
 * returns  1 if data size from client is invalid, but error message is sent out successfully;
 * returns -2 if patient data received are invalid, and error message is not sent out successfully;
 * returns  2 if patient data from client are invalid, but error message is sent out successfully;
 * returns -3 if db error occurs when adding patient, and error message is not sent out successfully;
 * returns  3 if db error occurs when adding patient, but error message is sent out successfully;
 * returns -4 if db error occurs when adding sessions, and error message is not sent out successfully;
 * returns  4 if db error occurs when adding sessions, but error message is sent out successfully;
 * returns -5 if score value data size received is invalid, and error message is not sent out successfully;
 * returns  5 if score value data size received is invalid, but error message is sent out successfully;
 * returns -6 if score value data received are invalid, and error message is not sent out successfully;
 * returns  6 if score value data received are invalid, but error message is sent out successfully;
 * returns -7 if db error occurs when adding score value, and error message is not sent out successfully;
 * returns  7 if db error occurs when adding score value, but error message is sent out successfully;
 * returns -8 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  8 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -9 if server confirmation message is not sent out successfully. */
int srvSession::recvNewPatient()
{
  int32 p_size, s_size, sv_num;
  if (!chkPatientDatSize(p_size, s_size, sv_num)) {
    if (sendStatMsg())
      return -1;
    return 1;
  }

  int32 dat_size = p_size + s_size;
  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to receive patient data from client";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  time_t t_stamp = time(NULL);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  // Add patient data (if it is available)
  if (p_size) {
    patientRec newPatient(buffer);
    if (addPatient(dbs.patientDB, txn, newPatient)) {
      txn->abort();
      statMsg = "DB error when adding new patient record";
      if (sendStatMsg())
	return -3;
      return 3;
    }  
  }
  // Add session data (if it is available)
  if (s_size) {
    vector<DBsession> sList;
    deserialize(s_size, buffer + p_size, sList);
    if (addSession(dbs.sessionDB, txn, sList, t_stamp)) {
      txn->abort();
      statMsg = "DB error when adding new session record";
      if (sendStatMsg())
	return -4;
      return 4;
    }  
  }

  // Add score value data (if it is available)
  int32 db_time = t_stamp;
  if (ntohs(1) == 1)
    swap(&db_time);
  for (int i = 0; i < sv_num; i++) {
    uint32 sv_size = 0;
    if (gnutls_record_recv(g_session, &sv_size, sizeof(uint32)) != sizeof(uint32)) {
      txn->abort();
      statMsg = "Failed to receive size of score value data from client";
      if (sendStatMsg())
	return -5;
      return 5;
    }

    if (ntohs(1) == 1)
      swap(&sv_size);
    char* sv_dat = new char[sv_size];
    if (recvBuffer(g_session, sv_size, sv_dat)) {
      txn->abort();
      statMsg = "Failed to receive score value data from client";
      if (sendStatMsg())
	return -6;
      return 6;
    }

    *((int32*) sv_dat) = db_time; // set the new time stamp 
    int32 sv_id = *((int32*) (sv_dat + sizeof(int32))); // get score value ID
    if (ntohs(1) == 1)
      swap(&sv_id);
    Dbt db_key(&sv_id, sizeof(int32));
    Dbt db_data(sv_dat, sv_size);
    if (dbs.scoreValueDB.getDb().put(txn, &db_key, &db_data, 0)) {
      txn->abort();
      statMsg = "DB error when adding score value data";
      if (sendStatMsg())
	return -7;
      return 7;
    }  
  }

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -8;
    return 8;
  }

  txn->commit(0);
  dbs.patientDB.getDb().sync(0);
  dbs.sessionDB.getDb().sync(0);
  dbs.scoreValueDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);
  statMsg = "updated:" + num2str(t_stamp);
  if (sendStatMsg())
    return -9;

  return 0;
}

/* This function checks the three tokens in lcient message to 
 * make sure they are all valid numeric values. 
 * Returns true if they are valid, false otherwise. */
bool srvSession::chkPatientDatSize(int32& p_size, int32& s_size, int32& sv_num) 
{
  bool conv_stat = str2num(c_tokens[1], p_size);
  if (!conv_stat || p_size < 0) {
    statMsg = "Invalid patient data size from client: " + c_tokens[1];
    return false;
  }

  conv_stat = str2num(c_tokens[2], s_size);
  if (!conv_stat || s_size < 0) {
    statMsg = "Invalid session data size from client: " + c_tokens[2];
    return false;
  }

  conv_stat = str2num(c_tokens[3], sv_num);
  if (!conv_stat || sv_num < 0) {
    statMsg = "Invalid number of score value record from client: " + c_tokens[3];
    return false;
  }

  return true;
}

/* Receive updated patient score data from client. 
 * Returns 0 if everything is ok;
 * returns -1 if number of score values received is invalid, and error message is not sent out successfully; 
 * returns  1 if number of score values received is invalid, but error message is sent out successfully; 
 * returns -2 if score value data size received is invalid, and error message is not sent out successfully;
 * returns  2 if score value data size received is invalid, but error message is sent out successfully;
 * returns -3 if score value data received are invalid, and error message is not sent out successfully;
 * returns  3 if score value data received are invalid, but error message is sent out successfully;
 * returns -4 if db error occurs when adding score value, and error message is not sent out successfully;
 * returns  4 if db error occurs when adding score value, but error message is sent out successfully;
 * returns -5 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  5 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -6 if server confirmation message is not sent out successfully. */
int srvSession::recvScoreValues()
{
  int32 sv_num; 
  bool conv_stat = str2num(c_tokens[1], sv_num);
  if (!conv_stat || sv_num <= 0) {
    statMsg = "Invalid number of score value record from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  int32 t_stamp = time(NULL);
  if (ntohs(1) == 1)
    swap(&t_stamp);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  for (int i = 0; i < sv_num; i++) {
    uint32 sv_size = 0;
    if (gnutls_record_recv(g_session, &sv_size, sizeof(uint32)) != sizeof(uint32)) {
      txn->abort();
      statMsg = "Failed to receive size of score value data from client";
      if (sendStatMsg())
	return -2;
      return 2;
    }

    if (ntohs(1) == 1)
      swap(&sv_size);
    char* sv_dat = new char[sv_size];
    if (recvBuffer(g_session, sv_size, sv_dat)) {
      txn->abort();
      statMsg = "Failed to receive score value data from client";
      if (sendStatMsg())
	return -3;
      return 3;
    }

    *((int32*) sv_dat) = t_stamp; // set new time stamp 
    int32 sv_id = *((int32*) (sv_dat + sizeof(int32))); // get score value ID
    if (ntohs(1) == 1)
      swap(&sv_id);
    Dbt db_key(&sv_id, sizeof(int32));
    Dbt db_data(sv_dat, sv_size);
    if (dbs.scoreValueDB.getDb().put(txn, &db_key, &db_data, 0)) {
      txn->abort();
      statMsg = "DB error when adding score value data";
      if (sendStatMsg())
	return -4;
      return 4;
    }  
  }

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -5;
    return 5;
  }

  txn->commit(0);
  dbs.scoreValueDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);
  statMsg = "updated:" + num2str(t_stamp);
  if (sendStatMsg())
    return -6;

  return 0;
}

/* Receive patient session data from client side and add them into database. 
 * Returns 0 if everything is ok;
 * returns -1 if data size from client is invalid, and error message is not sent out successfully;
 * returns  1 if data size from client is invalid, but error message is sent out successfully;
 * returns -2 if data from client are invalid, and error message is not sent out successfully;
 * returns  2 if data from client are invalid, but error message is sent out successfully;
 * returns -3 if db error occurs when adding sessions, and error message is not sent out successfully;
 * returns  3 if db error occurs when adding sessions, but error message is sent out successfully;
 * returns -4 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  4 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -5 if server confirmation message is not sent out successfully. */
int srvSession::recvSessions()
{
  int32 dat_size;
  bool conv_stat = str2num(c_tokens[1], dat_size); 
  if (!conv_stat || dat_size <= 0) {
    statMsg = "Invalid buffer size from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to receive patient session data from client";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  vector<DBsession> sList;
  deserialize(dat_size, buffer, sList);
  time_t t_stamp = time(NULL);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  if (addSession(dbs.sessionDB, txn, sList, t_stamp)) {
    txn->abort();
    statMsg = "DB error when adding patient session data";
    if (sendStatMsg())
      return -3;
    return 3;
  }  

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -4;
    return 4;
  }

  txn->commit(0);
  dbs.sessionDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);
  statMsg = "updated:" + num2str(t_stamp);
  if (sendStatMsg())
    return -5;

  return 0;
}

/* Receive patient list data from client side. 
 * The parameter shows whether it is a new patient list or updated patient list record.  
 * Returns 0 if everything is ok;
 * returns -1 if data size from client is invalid, and error message is not sent out successfully;
 * returns  1 if data size from client is invalid, but error message is sent out successfully;
 * returns -2 if data from client are invalid, and error message is not sent out successfully;
 * returns  2 if data from client are invalid, but error message is sent out successfully;
 * returns -3 if db error occurs when adding patient list, and error message is not sent out successfully;
 * returns  3 if db error occurs when adding patient list, but error message is sent out successfully;
 * returns -4 if db error occurs when updating system time stamp, and error message is not sent out successfully;
 * returns  4 if db error occurs when updating system time stamp, but error message is sent out successfully;
 * returns -5 if server confirmation message is not sent out successfully.  */
int srvSession::recvPatientList(bool isNew)
{
  int32 dat_size;
  bool conv_stat = str2num(c_tokens[1], dat_size); 
  string server_msg;
  if (!conv_stat || dat_size <= 0) {
    server_msg = "Invalid buffer size from client: " + c_tokens[1];
    if (sendStatMsg())
      return -1;
    return 1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    statMsg = "Failed to receive patient list data from client";
    if (sendStatMsg())
      return -2;
    return 2;
  }

  DBpatientlist inputRec(buffer);
  time_t t_stamp = time(NULL);
  if (isNew)
    inputRec.runDate.setUnixTime(t_stamp);
  else
    inputRec.modDate.setUnixTime(t_stamp);

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  if (addPatientList(dbs.patientListDB, txn, inputRec)) {
    txn->abort();
    statMsg = "DB error when adding patient list data";
    if (sendStatMsg())
      return -3;
    return 3;
  }  

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, statMsg)) {
    txn->abort();
    statMsg = "DB error when updating time stamp in system table: " + statMsg;
    if (sendStatMsg())
      return -4;
    return 4;
  }

  txn->commit(0);
  dbs.patientListDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);
  statMsg = "updated:" + num2str(t_stamp);
  if (sendStatMsg())
    return -5;

  return 0;
}

// This function is based on Dan's sample code
int srp_credfunction(gnutls_session,const char* username,gnutls_datum* salt,
                     gnutls_datum* verifier, gnutls_datum* generator,
                     gnutls_datum* prime)
{
  // retrieve user data from bdb
  string account_str(username);
  userRec uData;
  int foo = getUser(srvSession::dbs.userDB, NULL, account_str, uData);
  if (foo != 1) {
    return -1;
  }
  
  // copy salt
  salt->size = 4;
  salt->data = (unsigned char*) gnutls_malloc(4);
  if (!salt->data)
    return -1;
  memcpy(salt->data, uData.getSalt().data, 4);

  // copy stock generator
  generator->size = gnutls_srp_1024_group_generator.size;
  generator->data = (unsigned char*) gnutls_malloc(generator->size);
  if (!generator->data)
    return -1;
  memcpy(generator->data, gnutls_srp_1024_group_generator.data, generator->size);

  // copy stock prime
  prime->size = gnutls_srp_1024_group_prime.size;
  prime->data = (unsigned char*) gnutls_malloc(prime->size);
  if (!prime->data)
    return -1;
  memcpy(prime->data, gnutls_srp_1024_group_prime.data, prime->size);

  // copy verifier
  verifier->size = uData.getVerifier().size;
  verifier->data = (unsigned char*) gnutls_malloc(verifier->size);
  if (!verifier->data)
    return -1;
  memcpy(verifier->data, uData.getVerifier().data,verifier->size);

  // Added by Dongbo, record user ID so that permissions can be loaded for this user
  currentUser = uData.getID();
  return 0;
}

/* Copied from Dan.
 * Needs a lot of tweaking in the future.
 * Returns 0 if everythign is ok;
 * returns 1 if server can not open env or db files, or fails to set up global maps
 * returns -1 if bind fails;
 * returns -2 if listen fails;
 * returns -3 if global maps are not sent to client successfully; */
int startServer(const string& envHome, int32 port_no)
{
  // open env and db files
  srvSession::dbs.dirname = envHome;
  if (srvSession::dbs.open()) {
    cout << srvSession::dbs.errMsg << endl;
    srvSession::dbs.close();
    return 1;
  }

  // create socket and accept gnutls connection from client
  int err;
  struct sockaddr_in sa_serv;
  struct sockaddr_in sa_cli;
  socklen_t client_len;
  char topbuf[512];
  int yes=1;
  gnutls_srp_server_credentials_t srpcred;

  printf("SERVER STARTED\n");

  gnutls_global_init();
  gnutls_global_init_extra();
  gnutls_srp_allocate_server_credentials(&srpcred);
  gnutls_srp_set_server_credentials_function(srpcred, srp_credfunction);

  // create the socket, make it reusable after exit
  int srv_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (srv_socket < 0)
    return srv_socket;
  setsockopt(srv_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  // create socket address, bind to our socket
  memset(&sa_serv, 0, sizeof(sa_serv));
  sa_serv.sin_family = AF_INET;
  sa_serv.sin_port = htons(port_no);
  sa_serv.sin_addr.s_addr = htonl(INADDR_ANY);
  err = bind(srv_socket, (struct sockaddr*)&sa_serv, sizeof(sa_serv));
  if (err == -1) {
    close(srv_socket);
    return -1; 
  }
  // express willingness to listen for up to 16 queued connections
  err = listen(srv_socket, 16);   // how much of a backlog should we allow?
  if (err == -1) {
    close(srv_socket);
    return -2; 
  }

  printf("Now listening to port %d\n", port_no);
  client_len = sizeof(sa_cli);
  printf("server waiting for connection\n");
  list<srvSession> sessions;
  while(1) {
    fd_set myset;
    FD_ZERO(&myset);
    FD_SET(srv_socket, &myset);
    int highestsocket = srv_socket;
    for (list<srvSession>::iterator si = sessions.begin(); si != sessions.end(); ++si) {
      FD_SET(si->sd, &myset);
      if (si->sd > highestsocket) 
	highestsocket = si->sd;
    }

    struct timeval mytv;
    mytv.tv_sec = 5;  // wait 5 seconds before looping
    mytv.tv_usec = 0;   
    int ret = select(highestsocket + 1, &myset, NULL, NULL, &mytv);
    if (ret <= 0) 
      continue;  // this is actually an error condition, we might have lost the socket

    if (FD_ISSET(srv_socket, &myset)) {  // handle new connection stuff here
      cout << "New connection " << sessions.size() + 1 << endl;
       // Create new session, push on back of list, get ptr to gnutls session struct
      srvSession currentConnx;
      currentConnx.init();
      gnutls_credentials_set(currentConnx.g_session, GNUTLS_CRD_SRP, srpcred);
      currentConnx.sd = accept(srv_socket, (struct sockaddr*) &sa_cli, &client_len);
      printf ("- connection from %s, port %d\n",
	      inet_ntop(AF_INET, &sa_cli.sin_addr, topbuf, sizeof(topbuf)), ntohs(sa_cli.sin_port));
      gnutls_transport_set_ptr(currentConnx.g_session, (gnutls_transport_ptr_t) currentConnx.sd);
      printf("transport ptr set, about to handshake\n");
      int hs_stat = gnutls_handshake(currentConnx.g_session);  // srp_credfunction() is called here!
      printf("handshook, or not\n");
      if (hs_stat < 0) {
        close(currentConnx.sd);
        gnutls_deinit(currentConnx.g_session);
        fprintf(stderr, "*** Handshake has failed (%s)\n\n", gnutls_strerror(hs_stat));
        continue;
      }
      printf ("- Handshake completed\n");
      currentConnx.clientID = currentUser;
      if (currentConnx.sendGlobalMaps()) {
	cout << currentConnx.statMsg << endl;
	continue;  // Make sure global maps are sent out to client side successfully
      }
      if (currentConnx.sendPatientList()) {
	cout << currentConnx.statMsg << endl;
	continue;  // Make sure global maps are sent out to client side successfully
      }

      int foo = getPerm(srvSession::dbs.userDB, srvSession::dbs.permDB, 
			NULL, currentConnx.clientID, currentConnx.permissions);
      if (foo) {
        printf("*** Error code %d: fails to retrieve user permissions\n", foo);
        continue;
      }
      sessions.push_back(currentConnx);
    }

    for (list<srvSession>::iterator si = sessions.begin(); si != sessions.end(); si++) {
      if (!FD_ISSET(si->sd, &myset)) 
	continue;
      // handle this session here
      char msg_buffer[MSG_SIZE + 1];
      memset(msg_buffer, 0, MSG_SIZE);
      int ret2 = gnutls_record_recv(si->g_session, msg_buffer, MSG_SIZE);
      if (ret2 > 0) {
 	si->serve(msg_buffer);
      }
      else if (ret2 == 0) {
	printf ("\n- Peer has closed the GNUTLS connection\n");
	si->exit();
	sessions.erase(si);
	break; // important!!! Without this line server's response will be late
      }
      else {
	fprintf (stderr, "*** corrupted data(%d), closing connection\n", ret2);
	si->exit();
	sessions.erase(si);
	break; // important!!! Without this line server's response will be late
      }
    }
  }

  close(srv_socket);
  gnutls_srp_free_server_credentials(srpcred);
  gnutls_global_deinit();

  if (srvSession::dbs.close()) {
    cout << "Failed to close DB env and files" << endl;
    return -1;
  }

  return 0;
}

/* Print out usage message on the screen */
void usage()
{
  printf("Usage: server <db_dir> <port#>\n");
  printf("  db_dir: optional db directory (default is ../env)\n");
  printf("  port#:  optional port number (default is 5556)\n");
}

/* Call this function to close env and dbs when server is terminated in abnormal way.
 * Without this signal handler, the program may get this error when terminated by Ctrl-C repeatedly:
 * "unable to allocate memory for mutex; resize mutex region.
 * Error opening database file: namespace.db
 * Failed to open brain region namespace db
 * Database handles still open at environment close
 * Error closing db enviroment
 * Segmentation fault"
 * It is dangerous to close the server but leaves env and DBs still open. */
void handleSIG(int)
{
  if (srvSession::dbs.close()) {
    cout << "Failed to close DB env and files" << endl;
    exit(1);
  }
  exit(0);
}

/* Simple main function to start the server */
int main(int argc, char* argv[]) 
{  
  //  QApplication app(argc, argv);


  //signal(SIGINT, handleSIG);


  if (argc != 1 && argc != 3) {
    usage();
    exit(1);
  }

  int32 port_no;
  string envHome;
  // If there is no argument, set the default dir location and port number
  if (argc == 1) {
    envHome = "../env";
    port_no = 5556;
  }
  else {
    envHome = string(argv[1]);
    port_no = atoi(argv[2]);
    if (port_no < 1024) {
      usage();
      exit(1);
    }
  }

  if (startServer(envHome, port_no))
    exit(1);
  
  return 0;
}
