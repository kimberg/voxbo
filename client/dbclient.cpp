/* Define member functions of DBclient and its two derived classes. */
 
using namespace std;

#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include "dbclient.h"

/* Default constructor, do nothing right now. */
DBclient::DBclient() : online(false)
{

}

/* Another DBclient ctor that sets username and passwd. */

DBclient::DBclient(const string &username,const string &password) 
  : username(username), 
    passwd(password),
    online(false)
{

}

/* dtor for DBclient does nothing so far. */
DBclient::~DBclient()
{

}

/* Default localClient ctor. */
localClient::localClient()
{

}

/* Destructor closes db and env. */
localClient::~localClient()
{
  exit();
}

/* Ctor of localClient. */
localClient::localClient(const string& username, const string& password, const string& inputDir)
  : DBclient(username, password)
{
  dbs.dirname = inputDir;
}

/* Function that handles local login. 
 * Successful login is also followed by these steps:
 * (1) Open enviroment and db files.
 * (2) Read local score type file and build the type map.
 * (3) Build scorenames, testmap and scorenamechildren maps.
 * (4) Set user ID and user-specific permissions.
 * Returns 0 for successful login;
 * returns 1 if client has already logged in;
 * returns 2 if username is blank;
 * returns 3 if passwd is blank;
 * returns 4 if local server env is not set;
 * returns -1 if db env can not be opened.
 * returns -2 if user permission records can not be loaded.
 * returns -3 if patient list can not be loaded. */
int localClient::login()
{
  if (online) {
    errMsg = "Client already logged in.";
    return 1;
  }
  if (username.empty()) {
    errMsg = "Username not set";
    return 2;
  }
  if (passwd.empty()) {
    errMsg = "Password not set";
    return 3;
  }
  if (dbs.dirname.empty()) {
    errMsg = "Local db directory not set";
    return 4;
  }

  if (dbs.open()) {
    dbs.close();
    errMsg = dbs.errMsg;
    return -1;
  }

  int32 status = chkPasswd(dbs.userDB, NULL, username, passwd);
  if (status == 0) {
    errMsg = "Account " + username + " does not exist.";
    dbs.close();
    return -1;
  }
  if (status == -1 || status == -2) {
    errMsg = "Failed to retrive db records";
    dbs.close();
    return -1;
  }
  if (status == -3) {
    errMsg = "make_verifier error";
    dbs.close();
    return -1;
  }
  if (status == -4) {
    errMsg = "Wrong password for " + username;
    dbs.close();
    return -1;
  }  

  // Both user name and passwd are correct, log into the main interface
  online = true;
  userID = status;
  int foo = getPerm(dbs.userDB, dbs.permDB, NULL, userID, permissions);
  if (foo) {
    errMsg = "Failed to load user permissions.";
    dbs.close();
    return -2;
  }

  if (getPatientList(dbs.userDB, dbs.patientListDB, NULL, userID, pList)) {
    errMsg = "Failed to load user's patient list.";
    dbs.close();
    return -3;
  }

  return 0;
}

/* This function requests a certain number of unique IDs. 
 * Returns the new ID if everything is ok;
 * or returns 0 if new ID can not be retrieved from server.*/ 
int32 localClient::reqID(uint32 id_num)
{
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int32 startID = getSysID(dbs.sysDB, txn, id_num);
  if (startID <= 0) {
    txn->abort();
    return 0;
  }
  
  txn->commit(0);
  dbs.sysDB.getDb().sync(0); // write data on disk now    
  return startID;
}

/* This function returns current time stamp on server side. */
int32 localClient::reqTime()
{
  return time(NULL);
}

/* Add a new user account to local db.
 * Returns 0 if everything is ok;
 * returns -1 if account already exists; 
 * returns -2 if user table can not be updated;
 * returns -3 if time stamp can not be updated. */
int localClient::putNewUser(const userGUI& uiData)
{
  int32 size = uiData.getSize();
  char buff[size];
  uiData.serialize(buff);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int stat = addUser(dbs.userDB, dbs.sysDB, txn, buff);
  if (stat == 1) {
    errMsg = "Account already exists.";
    txn->commit(0);
    return -1;
  }

  if (stat < 0) {
    errMsg = "DB error when adding new user account";
    txn->abort();
    return -2;
  }
  
  if (setSysUpdate(dbs.sysDB, txn, errMsg)) {
    txn->abort();
    return -3;
  }

  txn->commit(0);
  dbs.sysDB.getDb().sync(0);
  dbs.userDB.getDb().sync(0); // write data on disk now    
  return 0;
}

/* Add new score names to local db.
 * Returns 0 if everything is ok;
 * returns -1 if socre name table ca not be updated; 
 * returns -2 if time stamp can not be updated in system table. */
int localClient::putNewScoreName(const vector<DBscorename>& snList)
{
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  // Add new patient score values (test results)
  if (addScoreName(dbs.scoreNameDB, txn, snList)) {
    errMsg = "DB error when adding new score name record";
    txn->abort();
    return -1;
  }  

  if (setSysUpdate(dbs.sysDB, txn, errMsg)) {
    txn->abort();
    return -2;
  }

  txn->commit(0);
  dbs.scoreNameDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);

  // Update map for local client
  for (uint i = 0; i < snList.size(); ++i) {
    dbs.add_scorename(snList[i]);
  }

  return 0;
}

/* Add new patient, new session and new score values into local db files.
 * Returns 0 if operation is successful;
 * returns -1 if new patient record can not be updated in db;
 * returns -2 if new session record can not be updated in db;  
 * returns -3 if new score value record can not be updated in db;
 * returns -4 if time stamp can not be updated in db. */
int localClient::putNewPatient(DBpatient& patient_in)
{
  time_t t_stamp = time(NULL);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);

  // Add new patient
  patientRec newPatient;
  newPatient.setID(patient_in.patientID);
  if (addPatient(dbs.patientDB, txn, newPatient)) {
    errMsg = "DB error when adding new patient record";
    txn->abort();
    return -1;
  }  

  // Add new patient sessions
  map<int32, DBsession>::iterator sess_it;
  for (sess_it = patient_in.sessions.begin(); sess_it != patient_in.sessions.end(); ++sess_it) {
    if (sess_it->second.id<=0) continue;
    sess_it->second.date.setUnixTime(t_stamp);
    if (addSession(dbs.sessionDB, txn, sess_it->second)) {
      errMsg = "DB error when adding new session record";
      txn->abort();
      return -2;
    }  
  }

  // Add new patient score values
  map<int32, DBscorevalue>::iterator sv_it;
  for (sv_it = patient_in.scores.begin(); sv_it != patient_in.scores.end(); ++sv_it) {
    if (sv_it->second.id<=0) continue;
    sv_it->second.whenset.setUnixTime(t_stamp);
    if (addScoreValue(dbs.scoreValueDB, txn, sv_it->second)) {
      errMsg = "DB error when adding new score value record";
      txn->abort();
      return -3;
    }  
  }

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, errMsg)) {
    txn->abort();
    return -4;
  }

  txn->commit(0);
  dbs.patientDB.getDb().sync(0);
  dbs.sessionDB.getDb().sync(0);
  dbs.scoreValueDB.getDb().sync(0);
  dbs.sysDB.getDb().sync(0);

  return 0;
}

/* Add new session data into local db file.
 * Returns 0 if everything is ok, 
 * returns -1 if new session record can not be updated in db; 
 * returns -2 if time stamp can not be updated in db. */
int localClient::putNewSession(vector<DBsession>& sList)
{
  time_t t_stamp = time(NULL);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  if (addSession(dbs.sessionDB, txn, sList, t_stamp)) {
    errMsg = "DB error when adding new session record";
    txn->abort();
    return -1;
  }  

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, errMsg)) {
    txn->abort();
    return -2;
  }

  txn->commit(0);
  dbs.sessionDB.getDb().sync(0); // write data on disk now
  dbs.sysDB.getDb().sync(0); 

  return 0;
}

/* Add score values (test results) into local db.
 * Returns 0 if everything is ok;
 * returns -1 if ID in input set is not found in input map;
 * returns -2 if score value record can not be updated in db;
 * returns -3 if time stamp can not be updated in db. */
int localClient::putScoreValues(set<int> svID, map<int32, DBscorevalue>& svMap) 
{
  time_t t_stamp = time(NULL);
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);

  set<int>::const_iterator it;
  for (it = svID.begin(); it != svID.end(); ++it) {
    int32 tmpID = *it;
    map<int32, DBscorevalue>::iterator map_it = svMap.find(tmpID);
    if (map_it == svMap.end()) {
      errMsg = "Invalid score value ID in DBscorevalue map: " + num2str(tmpID);
      txn->abort();
      return -1;
    }

    map_it->second.whenset.setUnixTime(t_stamp);
    if (addScoreValue(dbs.scoreValueDB, txn, map_it->second)) {
      errMsg = "DB error when adding new score value record";
      txn->abort();
      return -2;
    }  
  }

  if (setSysUpdate(dbs.sysDB, txn, t_stamp, errMsg)) {
    txn->abort();
    return -2;
  }

  txn->commit(0);
  dbs.scoreValueDB.getDb().sync(0); // write data on disk now
  dbs.sysDB.getDb().sync(0); 

  return 0;
}

/* Add score values (test results) into local db.
 * Returns 0 if everything is ok;
 * returns -1 if patient list record can not be updated in db;
 * returns -2 if time stamp can not be updated in db. */
int localClient::putPatientList(DBpatientlist& inputRec, bool isNew) 
{
  time_t t_stamp = time(NULL);
  if (isNew)
    inputRec.runDate.setUnixTime(t_stamp);
  else
    inputRec.modDate.setUnixTime(t_stamp);

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  if (addPatientList(dbs.patientListDB, txn, inputRec)) {
    errMsg = "DB error when adding new score value record";
    txn->abort();
    return -1;
  }
  
  if (setSysUpdate(dbs.sysDB, txn, errMsg)) {
    txn->abort();
    return -2;
  }

  txn->commit(0);
  dbs.patientListDB.getDb().sync(0); // write data on disk now
  dbs.sysDB.getDb().sync(0); 

  return 0;
}

/* Local client searching a certain user by account name.
 * Returns 0 if user is found on local server;
 * returns -1 if user is not found. 
 * returns -2 for db errors. */
int localClient::reqUser(const string& inputUser, userRec& rec_out)
{
  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int stat = getUser(dbs.userDB, txn, inputUser, rec_out);
  if (stat == 1) {
    txn->commit(0);
    return 0;
  }

  if (stat == 0) {
    errMsg = "User not found";
    txn->commit(0);
    return -1;
  }

  errMsg = "DB error " + num2str(stat);
  txn->abort();
  return -2;
}

/* Local client requests a patient search on local server. 
 * Returns 0 if any patient that matches search criterion is found;
 * returns -1 if permission is denied; 
 * returns -2 for db error;
 * returns -3 if no patient is found. */
int localClient::reqSearchPatient(const patientSearchTags& tags, vector<patientMatch>& matchList)
{
  if (!permissions.size()) {
    errMsg = "Permission denied";
    return -1;
  }

  DbTxn* txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int foo = searchPatients(dbs.scoreValueDB, txn, tags, permissions, matchList);
  if (foo) {
    txn->abort();
    errMsg = "DB error";
    return -2;
  }

  txn->commit(0);

  if (matchList.empty()) {
    errMsg = "Patient not found";
    return -3;
  }

  return 0;
}

/* Local client requests all score values of a certain patient.
 * Retunrs 0 if score values are found succesfully;
 * returns -1 if permission is denied;
 * returns -2 for db search error. */
int localClient::reqOnePatient(int32 pID, DBpatient& myPatient)
{
  if (!permissions.size()) {
    errMsg = "Permission denied";
    return -1;
  }

  DbTxn * txn = NULL;
  dbs.env.getEnv()->txn_begin(NULL, &txn, 0);
  int stat = getOnePatient(dbs.scoreValueDB, dbs.sessionDB, txn, pID, permissions, myPatient);
  if (stat) {
    cout << "FIXME getonepatient error " << stat << endl;
    txn->abort();
    return -2;
  }

  txn->commit(0);
  return 0;
}

/* Close local database.
 * returns 0 if everything is ok;
 * returns -1 for db close error. */
int localClient::exit()
{
  errMsg = "";
  if (dbs.close()) {
    errMsg = "Failed to close local database";
    return -1;
  }

  online = false;
  return 0;
}

/**********************************************************
 * Member function definition of remoteClient class. 
 *********************************************************/
/* Default ctor for remoteClient. */
remoteClient::remoteClient() 
  : init_flag(false)
{

}

/* Destrucotr closes client connection. */
remoteClient::~remoteClient()
{
  exit();
}

/* Ctor for remote client. */
remoteClient::remoteClient(const string& inputUser, const string& inputPwd, 
			   const string& inputSrv, uint32 inputPort)
  : DBclient(inputUser, inputPwd), 
    serverName(inputSrv), 
    serverPort(inputPort), 
    init_flag(false)
{

}

/* Function that handles remote login. 
 * Returns 0 if everything is ok;
 * returns 1 if client has already logged in;
 * returns 2 if username is not set;
 * returns 3 if passwd is not set;
 * returns 4 if server name is not set;
 * returns 5 if server port number is invalid;
 * retunrs -1 if server not available;
 * returns -2 if handshake between server and client fails;
 * returns -3 if maps not received successfully from server. */
int remoteClient::login()
{
  if (online) {
    errMsg = "Client already logged in.";
    return 1;
  }
  if (username.empty()) {
    errMsg = "Username not set";
    return 2;
  }
  if (passwd.empty()) {
    errMsg = "Password not set";
    return 3;
  }
  if (serverName.empty()) {
    errMsg = "Server name not set";
    return 4;
  }
  if (serverPort <= 0) {
    errMsg = "Invalid server port number: " + num2str(serverPort);
    return 4;
  }

  // Check server availability
  if (chkServer()) {
    return -1;
  }
  // User authentification
  if (chkHandshake()) {
    exit();
    return -2;
  }

  online = true;
  // Make sure maps are received successfully
  if (recvMaps()) {
    exit();
    return -3;
  }

  return 0;
}

/* Check server availability. 
 * returns 0 if server is ok;
 * returns -1 if host is not found;
 * returns -2 if connection fails. */
int remoteClient::chkServer()
{
  struct hostent *hp;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(serverPort);
  hp = gethostbyname(serverName.c_str());
  if (!hp) {
    errMsg = "Couldn't find host: ";
    errMsg.append(serverName);
    return -1;
  }
  memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
  c_stat = safe_connect(&addr, 10);
  if (c_stat < 0) {
    errMsg = "Couldn't connect to " + serverName;
    return -2;
  }

  fcntl(c_stat, F_SETFL, 0);  
  return 0;
}

/* Test handshake using gnutls 
 * Returns 0 if handshake succeeds;
 * returns -1 if handshake fails. */
int remoteClient::chkHandshake()
{
  // INIT GNUTLS
  gnutls_global_init();
  gnutls_global_init_extra();
  gnutls_srp_allocate_client_credentials(&srpcred);
  gnutls_srp_set_client_credentials(srpcred, username.c_str(), passwd.c_str());
  init_flag = true;
  // MORE GNUTLS SETUP and handshake
  gnutls_init(&g_session, GNUTLS_CLIENT);
  gnutls_priority_set_direct(g_session, "NORMAL:+SRP", NULL);
  gnutls_credentials_set(g_session, GNUTLS_CRD_SRP, srpcred);
  gnutls_transport_set_ptr(g_session, (gnutls_transport_ptr_t) c_stat);
  int ret = gnutls_handshake(g_session);
  if (ret < 0) {
    errMsg = "Client error: Handshake failed";
    gnutls_perror(ret);
    return -1;   // FIXME need to be more elegant about this
  }

  return 0;
}

/* This function parses the server message that has buffer size information.
 * The real data buffer will be followed.
 * Returns the size of data buffer that is going to follow this message;
 * returns -1 if server message is not received;
 * returns -2 if no patient list is available;
 * returns -3 if no score value is available;
 * returns -4 if no valid session available (all patient score values are static);
 * returns -5 if server sends a scorevalue message;
 * returns 0 if the message doesn't have size info or is not understandable. */
int32 remoteClient::parseServerMsg()
{
  errMsg = "";
  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server message not received";
    return -1;
  }

  s_tokens.clear();
  s_tokens.SetSeparator(" ");
  s_tokens.ParseLine(msg_recv);
  if (s_tokens[0] == "no_data:") {
    string tmpStr(msg_recv);
    int i = tmpStr.find(" ");
    errMsg = tmpStr.substr(i + 1, tmpStr.size() - i - 1);
    return 0;
  }

  if (s_tokens[0] == "no_patientlist")
    return -2;

  if (s_tokens[0] == "no_scorevalue")
    return -3;

  if (s_tokens[0] == "no_session")
    return -4;

  if (s_tokens[0] == "scorevalue:" && s_tokens.size() == 3)
    return -5;

  if (s_tokens.size() != 2 || s_tokens[0] != "server_data_size:") {
    errMsg = (string) "Server message not understandable: " + msg_recv;;
    return 0;
  }

  int32 dat_size = atoi(s_tokens(1));
  if (dat_size <= 0) {
    errMsg = (string) "Unknown data size: " + s_tokens[1];
    return 0;
  }

  return dat_size;
}

/* Receive global map data from server.
 * Returns 0 if everything is ok;
 * returns -1 if score type data are not received correctly; 
 * returns -2 if score name data are not received correctly; 
 * returns -3 if brain region name data are not received correctly; 
 * returns -4 if brain region synonym data are not received correctly; 
 * returns -5 if brain region relationship data are not received correctly; 
 * returns -6 if patient list data are not received correctly. */
int remoteClient::recvMaps()
{
  if (recvTypes())
    return -1;

  if (recvScoreNames())
    return -2;

  if (recvRegionNames())
    return -3;

  if (recvSynonyms())
    return -4;

  if (recvRegionRelations())
    return -5;

  if (recvPatientList())
    return -6;

  return 0;
}

/* Receive typemap info from server.
 * Returns 0 if type data are received correctly;
 * returns -1 if type data size is invalid;
 * returns -2 if type data buffer is not received correctly. */
int remoteClient::recvTypes()
{
  int32 dat_size = parseServerMsg();
  if (dat_size <= 0) {
    errMsg = "Invalid size of score type data buffer: " + num2str(dat_size);
    return -1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive score types data from server";
    return -2;
  }

  vector<DBtype> type_vec;
  deserialize(dat_size, buffer, type_vec);
  // set local type map
  for (uint i = 0; i < type_vec.size(); i++) {
    string tname = type_vec[i].name;
    dbs.typemap[tname] = type_vec[i];
  }

  return 0;
}

/* Receive score names map from server.
 * Returns 0 if score name data are received correctly;
 * returns -1 if score name data size is invalid;
 * returns -2 if score name data buffer is not received correctly. */
int remoteClient::recvScoreNames()
{
  int32 dat_size = parseServerMsg();
  if (dat_size <= 0) {
    errMsg = "Invalid size of score name data buffer: " + num2str(dat_size);
    return -1;
  }
  
  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive score name data from server";
    return -2;
  }

  vector<DBscorename> sn_vec;
  // set local score name maps
  deserialize(dat_size, buffer, sn_vec);
  for (uint i = 0; i < sn_vec.size(); ++i) {
    dbs.scorenames[sn_vec[i].name] = sn_vec[i];
    string parentStr = scoreparent(sn_vec[i].name);
    dbs.scorenamechildren.insert(pair<string, string>(parentStr, sn_vec[i].name));
  }

  return 0;
}

/* Receive region names map from server.
 * Returns 0 if region name data are received correctly;
 * returns -1 if region name data size is invalid;
 * returns -2 if region name data buffer is not received correctly. */
int remoteClient::recvRegionNames()
{
  int32 dat_size = parseServerMsg();
  if (dat_size <= 0) {
    errMsg = "Invalid size of brain region name data buffer: " + num2str(dat_size);
    return -1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive brain region name data from server";
    return -2;
  }

  vector<regionRec> vec;
  deserialize(dat_size, buffer, vec);
  // set local region name map
  for (uint i = 0; i < vec.size(); ++i) 
    dbs.regionNameMap[vec[i].getID()] = vec[i];

  return 0;
}

/* Receive synonym map from server.
 * Returns 0 if synonym data are received correctly;
 * returns -1 if synonym data size is invalid;
 * returns -2 if synonym data buffer is not received correctly. */
int remoteClient::recvSynonyms()
{
  int32 dat_size = parseServerMsg();
  if (dat_size <= 0) {
    errMsg = "Invalid size of brain region synonym data buffer: " + num2str(dat_size);
    return -1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive brain region synonym data from server";
    return -2;
  }

  vector<synonymRec> vec;
  deserialize(dat_size, buffer, vec);
  // set local synonym map
  for (uint i = 0; i < vec.size(); ++i) 
    dbs.synonymMap[vec[i].getID()] = vec[i];

  return 0;
}

/* Receive brain region relationship map from server
 * Returns 0 if region relationship data are received correctly;
 * returns -1 if region relationship data size is invalid;
 * returns -2 if region relationship data buffer is not received correctly. */
int remoteClient::recvRegionRelations()
{
  int32 dat_size = parseServerMsg();
  if (dat_size <= 0) {
    errMsg = "Invalid size of brain region relationship data buffer: " + num2str(dat_size);
    return -1;
  }

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive brain region relationship data from server";
    return -2;
  }

  vector<regionRelationRec> vec;
  deserialize(dat_size, buffer, vec);
  // set local region relationship map
  for (uint i = 0; i < vec.size(); ++i) 
    dbs.regionRelationMap[vec[i].getID()] = vec[i];

  return 0;
}

/* Receive patient list data from server.
 * Returns 0 if patient list data are received correctly (or no patient list is available);
 * returns -1 if patient list data size is invalid;
 * returns -2 if patient list data buffer is not received correctly. */
int remoteClient::recvPatientList()
{
  int32 dat_size = parseServerMsg();
  if (dat_size == -2)  // This user doesn't have patient list 
    return 0;

  if (dat_size <= 0) {
    errMsg = "Invalid size of patient list data buffer: " + num2str(dat_size);
    return -1;
  } 

  char buffer[dat_size];
  if (recvBuffer(g_session, dat_size, buffer)) {
    errMsg = "Failed to receive patient list data from server";
    return -2;
  }

  deserialize(dat_size, buffer, pList);
  return 0;
} 

/* This function requests a certain number of unique IDs.
 * Returns the new ID if everything is ok;
 * or returns 0 if new ID can not be retrieved from server.
 * returns -1 if cleint message fails to be sent out. */
int32 remoteClient::reqID(uint32 id_num)
{
  string msg2srv("req_id: ");
  msg2srv.append(num2str(id_num));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Client request not sent out";
    return -1;
  }

  int32 dat_size = parseServerMsg();
  if (dat_size <= 0)
    return 0;

  int32 startID = 0;
  if (gnutls_record_recv(g_session, &startID, sizeof(int32)) != sizeof(int32)) {
    errMsg = "New ID not received successully from server";
    return 0;
  }

  if ((ntohs(1) == 1))
    swap(&startID);  
  if (startID <= 0) { 
    errMsg = "Invalid ID from server: " + num2str(startID);
    return 0;
  }

  return startID;
}

/* This function sends a request of current time stamp to server. 
 * Returns -1 if client message not sent successfully;
 * returns 0 if the time stamp from server is non-positive;
 * returns the time stamp integer if everything is ok. */
int32 remoteClient::reqTime()
{
  string msg2srv("req_time");
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Client request not sent out";
    return -1;
  }

  int32 t_stamp = 0;
  if (gnutls_record_recv(g_session, &t_stamp, sizeof(int32)) != sizeof(int32)) {
    errMsg = "Time stamp not received successully from server";
    return 0;
  }

  if ((ntohs(1) == 1))
    swap(&t_stamp);  
  if (t_stamp <= 0) { 
    errMsg = "Invalid time stamp from server: " + num2str(t_stamp);
    return 0;
  }

  return t_stamp;
}

/* Add a new user account to remote db.
 * Returns 0 if everything is ok;
 * returns -1 if put message can not be sent out to server;
 * returns -2 if data of new user can not be sent out to server;
 * returns -3 if server confirmation message is not received;
 * returns -4 if server confirmation message is invalid. */
int remoteClient::putNewUser(const userGUI& uiData)
{
  int32 dat_size = uiData.getSize();
  char buff[dat_size];
  uiData.serialize(buff);

  string msg2srv("put_new_user: ");
  msg2srv.append(num2str(dat_size));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }
  if (sendBuffer(g_session, dat_size, buff)) {
    errMsg = "Failed to send new user data to server";
    return -2;
  }

  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server feedback not received";
    return -3;
  }
  if (strcmp(msg_recv, "success")) {
    errMsg = msg_recv;
    return -4;
  }

  return 0;
}

/* Add new score name records into remote db.
 * Returns 0 if everything is ok;
 * returns -1 if put message can not be sent out to server;
 * returns -2 if data of new score name(s) can not be sent out to server;
 * returns -3 if server confirmation message is not received;
 * returns -4 if server confirmation message is invalid. */
int remoteClient::putNewScoreName(const vector<DBscorename>& snList)
{
  int32 dat_size = getSize(snList);
  char buff[dat_size];
  serialize(snList, buff);

  string msg2srv("put_scorename: ");
  msg2srv.append(num2str(dat_size));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }
  if (sendBuffer(g_session, dat_size, buff)) {
    errMsg = "Failed to send score name data to server";
    return -2;
  }

  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server feedback not received";
    return -3;
  }
  if (strcmp(msg_recv, "success")) {
    errMsg = msg_recv;
    return -4;
  }
  // update score name maps on client side
  for (uint i = 0; i < snList.size(); ++i) {
    dbs.add_scorename(snList[i]);
  }

  return 0;
}

/* Add new patient, test sessions and score values into remote db.
 * Returns 0 if everything is ok;
 * returns -1 if put message can not be sent out to server;
 * returns -2 if new patient and sessions data can not be sent out to server;
 * returns -3 if data size of a certain score value can not be sent out to server;  
 * returns -4 if a certain score value data can not be sent out to server; 
 * returns -5 if server confirmation message is not received or it is invalid. */
int remoteClient::putNewPatient(DBpatient& patient_in)
{
  patientRec newPatient;
  newPatient.setID(patient_in.patientID);

  uint32 p_size = newPatient.getSize();
  uint32 s_size = patient_in.getSessionSize();
  uint32 v_num = patient_in.scores.size();
  string msg2srv("put_patient: ");
  msg2srv += num2str(p_size) + " " + num2str(s_size) + " " + num2str(v_num);

  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }

  char buff[p_size + s_size];
  newPatient.serialize(buff);
  patient_in.serializeSessions(buff + p_size);
  if (sendBuffer(g_session, p_size + s_size, buff)) {
    errMsg = "Failed to send new patient and session data to server";
    return -2;
  }

  // Send patient score values to server, one by one 
  map<int32, DBscorevalue>::iterator sv_it;
  for (sv_it = patient_in.scores.begin(); sv_it != patient_in.scores.end(); ++sv_it) {
    if (sv_it->second.id<=0) continue;
    uint32 sv_size = sv_it->second.getSize();
    uint32 size_net = sv_size;
    if (ntohs(1) == 1)
      swap(&size_net);
    //if (sendBuffer(g_session, sizeof(uint32), (char*) (&size_net))) {
    if (gnutls_record_send(g_session, &size_net, sizeof(uint32)) != sizeof(uint32)) {
      errMsg = "Failed to send size of score value " + sv_it->second.scorename + " to server";
      return -3;
    }
    
    char* sv_dat = new char[sv_size];
    sv_it->second.serialize(sv_dat);    
    if (sendBuffer(g_session, sv_size, sv_dat)) {
      errMsg = "Failed to send score value " + sv_it->second.scorename + " to server";
      return -4;
    }
  }

  uint32 t_stamp = recvTimeMsg();
  if (t_stamp == 0)
    return -5;

  patient_in.updateTime(t_stamp);
  return 0;
}

/* This function parses an input string. 
 * If it is in a format of "updated:xxx", converts xxx into a uint32 and returns it;
 * returns 0 if one of the following errors happens:
 *( if message can not be received from server;
 * returns -2 if the format is not correct;
 * returns -3 if it is in correct format but xxx can not be converted to valid uint32. */
uint32 remoteClient::recvTimeMsg()
{
  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server feedback not received";
    return 0;
  }

  string tmpStr(msg_recv);
  if (tmpStr.size() <= 8 || tmpStr.substr(0, 8) != "updated:") {
    errMsg = msg_recv;
    return 0;
  }

  uint32 time_int = 0;
  string t_str = tmpStr.substr(8, tmpStr.size() - 8);
  if (!str2num(t_str, time_int) || time_int <= 0) {
    errMsg = "Invalid update time stamp from server: " + t_str;
    return 0;
  }

  return time_int;
}

/* Only add new sessions into remote database. 
 * Returns 0 if everything is ok;
 * returns -1 if put message can not be sent out to server;
 * returns -2 if session data can not be sent out to server;
 * returns -3 if server confirmation message is not received;
 * returns -4 if server conformation message is invalid. */
int remoteClient::putNewSession(vector<DBsession>& sList)
{
  int32 dat_size = getSize(sList);
  char buff[dat_size];
  serialize(sList, buff);

  string msg2srv("put_session: ");
  msg2srv.append(num2str(dat_size));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }
  if (sendBuffer(g_session, dat_size, buff)) {
    errMsg = "Failed to send session data to server";
    return -2;
  }

  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server feedback not received";
    return -3;
  }
  if (strcmp(msg_recv, "success")) {
    errMsg = msg_recv;
    return -4;
  }

  return 0;
}

/* Add score values (test results) into remote db.
 * Returns 0 if everything is ok; 
 * returns -1 if put message can not be sent out to server;
 * returns -2 if score value ID in input set is not found in input map;
 * returns -3 if score value header data can not be sent out to server;
 * returns -4 if any score value data can not be sent out to server;
 * returns -5 if time stamp from server is not received correctly. */
int remoteClient::putScoreValues(set<int32> svID, map<int32, DBscorevalue>& svMap) 
{
  int32 sv_num = svID.size();;
  string msg2srv("put_scorevalue: ");
  msg2srv.append(num2str(sv_num));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }

  set<int32>::iterator id_it;
  for (id_it = svID.begin(); id_it != svID.end(); ++id_it) {
    int32 tmpID = *id_it;
    map<int32, DBscorevalue>::iterator map_it = svMap.find(tmpID);
    if (map_it == svMap.end()) {
      errMsg = "Invalid score value ID in DBscorevalue map: " + num2str(tmpID);
      return -2;
    }

    uint32 sv_size = svMap[tmpID].getSize();
    uint32 size_net = sv_size;
    if (ntohs(1) == 1)
      swap(&size_net);
    if (gnutls_record_send(g_session, &size_net, sizeof(uint32)) != sizeof(uint32)) {
      errMsg = "Failed to send size of score value of " + map_it->second.scorename + " to server";
      return -3;
    }
    
    char* sv_dat = new char[sv_size];
    map_it->second.serialize(sv_dat);    
    if (sendBuffer(g_session, sv_size, sv_dat)) {
      errMsg = "Failed to send score value of " + map_it->second.scorename + " to server";
      delete [] sv_dat;
      return -4;
    }
    delete [] sv_dat;
  }

  uint32 t_stamp = recvTimeMsg();
  if (t_stamp == 0)
    return -5;

  // Update score value time stamps on client side
  for (id_it = svID.begin(); id_it != svID.end(); ++id_it) {
    int32 tmpID = *id_it;
    svMap[tmpID].whenset.setUnixTime(t_stamp);
  }

  return 0;
}

/* Add score values (test results) into remote db.
 * Returns 0 if everything is ok;
 * returns -1 if put message can not be sent out to server;
 * returns -2 if patient list data can not be sent out to server;
 * returns -3 if server confirmation message is not received;
 * returns -4 if server confirmation message is invalid. */
int remoteClient::putPatientList(DBpatientlist& inputRec, bool isNew) 
{
  int32 dat_size = inputRec.getSize();
  char buff[dat_size];
  inputRec.serialize(buff);

  string msg2srv;
  if (isNew)
    msg2srv = "put_patientlist: ";
  else
    msg2srv = "mod_patientlist: ";

  msg2srv.append(num2str(dat_size));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }
  if (sendBuffer(g_session, dat_size, buff)) {
    errMsg = "Failed to send patient list data to server";
    return -2;
  }

  char msg_recv[MSG_SIZE + 1];
  if (recvMessage(g_session, msg_recv)) {
    errMsg = "Server feedback not received";
    return -3;
  }

  uint32 t_stamp = recvTimeMsg();
  if (t_stamp == 0)
    return -4;

  if (isNew)
    inputRec.runDate.setUnixTime(t_stamp);
  else
    inputRec.modDate.setUnixTime(t_stamp);
  
  return 0;
}

/* This function accepts a user name and loads that user's information into outInfo.
 * Returns 0 if everything is ok;
 * returns -1 if request message can not be sent to server;
 * returns -2 if size of data from server is invalid;
 * returns -3 if data from server can not be received. */
int remoteClient::reqUser(const string& usrname, userRec& outInfo)
{
  string msg2srv("req_usr_info: ");
  msg2srv.append(usrname);
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send request to server";
    return -1;
  }

  int32 dat_size = parseServerMsg();
  if (!dat_size)
    return -2;

  char dat_buff[dat_size];
  if (recvBuffer(g_session, dat_size, dat_buff)) {
    errMsg = "Failed to receive user data from server";
    return -3;
  }

  outInfo.deserialize(dat_buff);
  return 0;
}

/* This function sends client's patient search requests to server 
 * and parses the data buffer from server. 
 * Returns 0 if everything is ok;
 * returns -1 if request message can not be sent out to server;
 * returns -2 if size of data from server is invalid;
 * returns -3 if data from server can not be received. */
int remoteClient::reqSearchPatient(const patientSearchTags& tag_in, vector<patientMatch>& pMatches)
{
  string msg2srv = tag_in.getStr(); 
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }

  int32 dat_size = parseServerMsg();
  if (!dat_size)
    return -2;

  char dat_buff[dat_size];
  if (recvBuffer(g_session, dat_size, dat_buff)) {
    errMsg = "Failed to receive patient data from server";
    return -3;
  }
    
  deserialize(dat_size, dat_buff, pMatches);
  return 0;
}

/* This function sends server request of getting all score values 
 * of a certain patient and parses the data buffer from server. 
 * Returns 0 if everything is ok;
 * returns -1 if request message can not be sent out to server;
 * returns -2 if not score value is available for this patient;
 * returns -3 for unknown message form server;
 * returns -4 if size of score value is invalid;
 * returns -5 if score value data is invalid;
 * returns -6 if size of session data is invalid; 
 * returns -7 if session data are invalid. */
int remoteClient::reqOnePatient(int32 pID, DBpatient& patient_out)
{
  string msg2srv("req_patient: ");
  msg2srv.append(num2str(pID));
  if (sendMessage(g_session, msg2srv)) {
    errMsg = "Failed to send message to server";
    return -1;
  }

  // Receive score values from server
  int32 srvStat = parseServerMsg();
  if (srvStat == -3) {
    errMsg = "No score value for this patient";
    return -2;
  }
  if (srvStat != -5)
    return -3;

  while (srvStat == -5) {
    int32 dat_size = atoi(s_tokens(1));
    if (dat_size <= 0) {
      errMsg = "Invalid data size of score value: " + s_tokens[1];
      return -4;
    }

    string newPerm = s_tokens[2];
    char* sv_buff= new char[dat_size];
    if (recvBuffer(g_session, dat_size, sv_buff)) {
      errMsg = "Failed to receive patient score value data from server";
      delete [] sv_buff;
      return -5;
    }

    DBscorevalue tmpRec;
    tmpRec.deserializeHdr(sv_buff);
    patient_out.scores[tmpRec.id] = tmpRec;
    patient_out.scores[tmpRec.id].deserialize(sv_buff);
    patient_out.scores[tmpRec.id].permission = newPerm;
    delete [] sv_buff;

    srvStat = parseServerMsg();
  }

  if (srvStat == -4) 
    return 0;  // no session available from server

  if (srvStat <= 0) 
    return -6;

  vector<DBsession> sessList; 
  char sess_buff[srvStat];
  if (recvBuffer(g_session, srvStat, sess_buff)) {
    errMsg = "Failed to receive test session data from server";
    return -7;
  }

  deserialize(srvStat, sess_buff, sessList); 
  patient_out.setSessionMap(sessList);
  patient_out.resetMaps();

  return 0;
}

// Close client connection cleanly
int remoteClient::exit()
{
  if (!init_flag)
    return 0;

  if (online)
    gnutls_bye(g_session, GNUTLS_SHUT_RDWR);

  close(c_stat);
  gnutls_deinit(g_session);
  gnutls_srp_free_client_credentials(srpcred);
  gnutls_global_deinit();
  online = false;

  return 0;
}

