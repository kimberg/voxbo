
// dbclient.h
// Copyright (c) 2010 by The VoxBo Development Team

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

#ifndef DBCLIENT_H
#define DBCLIENT_H

#include <string>
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include "typedefs.h"
#include "db_util.h"
#include "dbdata.h"
#include "tokenlist.h"

/* DBclient is an abstract base class. */
class DBclient
{
  friend class DBview;

 public:
  DBclient();
  DBclient(const string &username, const string &pwd);
  virtual ~DBclient();
  void setUserName(const string& inputStr) { username = inputStr; }
  void setPasswd(const string& inputStr) { passwd = inputStr; }
  string getErrMsg() { return errMsg; }

  virtual int login() = 0;
  virtual int putNewUser(const userGUI&) = 0;
  virtual int putNewScoreName(const vector<DBscorename>&) = 0;
  virtual int putNewPatient(DBpatient&) = 0;
  virtual int putNewSession(vector<DBsession>&) = 0;
  virtual int putScoreValues(set<int32>, map<int32, DBscorevalue>&) = 0; 
  virtual int putPatientList(DBpatientlist&, bool) = 0;

  virtual int32 reqID(uint32) = 0;
  virtual int32 reqTime() = 0;
  virtual int reqUser(const string&, userRec&) = 0;
  virtual int reqSearchPatient(const patientSearchTags&, vector<patientMatch>&) = 0;
  virtual int reqOnePatient(int32, DBpatient&) = 0;
  virtual int exit() = 0;
  operator bool() {return online;}

  DBdata dbs;
  vector<DBpatientlist> pList;

 protected:
  string username, passwd, errMsg;
  bool online;
};

/* Local client, inherited from DBclient. */
class localClient : public DBclient
{
 public:
  localClient();
  localClient(const string& username, const string& pwd, const string& filename);
  ~localClient();
  void setEnvHome(const string& inputDir) { dbs.dirname = inputDir; }

  virtual int login();
  virtual int putNewUser(const userGUI&);
  virtual int putNewScoreName(const vector<DBscorename>&);
  virtual int putNewPatient(DBpatient&);
  virtual int putNewSession(vector<DBsession>&);
  virtual int putScoreValues(set<int32>, map<int32, DBscorevalue>&); 
  virtual int putPatientList(DBpatientlist&, bool);

  virtual int32 reqID(uint32);
  virtual int32 reqTime();
  virtual int reqUser(const string&, userRec &);
  virtual int reqSearchPatient(const patientSearchTags&, vector<patientMatch>&);
  virtual int reqOnePatient(int32, DBpatient&);
  virtual int exit();

 private:
  int32 userID;
  map<string, string> permissions;   
};

/* Remote client, inherited from DBclient. */
class remoteClient : public DBclient
{
 public:
  remoteClient();
  remoteClient(const string&, const string&, const string&, uint32);
  ~remoteClient();
  void setServerName(const string& inputStr) { serverName = inputStr; }
  void setServerPort(uint32 inputVal) { serverPort = inputVal; }

  virtual int login();
  virtual int putNewUser(const userGUI&);
  virtual int putNewScoreName(const vector<DBscorename>&);
  virtual int putNewPatient(DBpatient&);
  virtual int putNewSession(vector<DBsession>&);
  virtual int putScoreValues(set<int32>, map<int32, DBscorevalue>&); 
  virtual int putPatientList(DBpatientlist&, bool);

  virtual int32 reqID(uint32);
  virtual int32 reqTime();
  virtual int reqUser(const string&, userRec&);
  virtual int reqSearchPatient(const patientSearchTags&, vector<patientMatch>&);
  virtual int reqOnePatient(int32, DBpatient&);
  virtual int exit();

 private:
  int chkServer();
  int chkHandshake();
  int32 parseServerMsg();
  int32 parseTimeMsg(char*);
  int recvMaps();
  int recvTypes();
  int recvScoreNames();
  int recvRegionNames();
  int recvSynonyms();
  int recvRegionRelations();
  int recvPatientList();
  uint32 recvTimeMsg();

  string serverName;
  uint32 serverPort;
  int32 c_stat;
  tokenlist s_tokens;
  gnutls_session_t g_session;
  gnutls_srp_client_credentials_t srpcred;
  bool init_flag;
  //const int BUFSIZE;

};

#endif
