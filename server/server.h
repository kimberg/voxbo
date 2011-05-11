#ifndef SERVER_H
#define SERVER_H

#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include <vector>
#include "bdb_tab.h"
#include "dbdata.h"
#include "db_util.h"
#include "tokenlist.h"

// Server session class
class srvSession {
  friend int startServer(const string&, int);
  friend void handleSIG(int);
  friend int srp_credfunction(gnutls_session, const char*, gnutls_datum*, 
			      gnutls_datum*, gnutls_datum*, gnutls_datum*);
 public:
  void init();
  void exit();
  void serve(const char*);
  int sendGlobalMaps();
  int sendPatientList();
  int sendUsrInfo();
  int sendID();
  int sendTime();
  int sendPatientMatches();
  int sendPatientScores();

  gnutls_session_t g_session;
  int sd;
  int32 clientID;
  map<string, string> permissions;

 private:
  bool parsePatientSearch(patientSearchTags&);  
  int recvNewUsr();
  int recvScoreNames();
  int recvNewPatient();
  bool chkPatientDatSize(int32&, int32&, int32&);
  int recvScoreValues();
  int recvSessions();
  int recvPatientList(bool);

  int sendData(int32, char*);
  int sendStatMsg();
  int sendTypes();
  int sendScoreNames();
  int sendRegionNames();
  int sendSynonyms();
  int sendRegionRelations();

  tokenlist c_tokens;
  string statMsg;
  static DBdata dbs;  
};

void usage();
void handleSIG(int);
int startServer(const string&, int32);
int srp_credfunction(gnutls_session, const char*, gnutls_datum*, 
		     gnutls_datum*, gnutls_datum*, gnutls_datum*);

#endif
