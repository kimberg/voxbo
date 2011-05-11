using namespace std;

#include "dbclient.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[])
{
  if (argc != 2) {
    cout << "Usage: connnx2srv local | rmt" << endl;
    exit(1);
  }

  string mode = argv[1];
  if (mode != "local" && mode != "rmt" && mode != "remote") {
    cout << "Usage: connnx2srv local | rmt | remote" << endl;
    exit(1);
  }

  /****************************************
   *  User login (local or remote)
   ****************************************/
  string username("dhu");
  string passwd("dhu");
  DBclient* dbcp = 0;
  if (mode == "local") {
    string envHome("../env");    
    dbcp = new localClient(username, passwd, envHome);
  }
  else {
    string serverName("ajax");
    uint32 serverPort = 5556;
    dbcp = new remoteClient(username, passwd, serverName, serverPort);
  }

  if (dbcp->login()) {
    cout << dbcp->getErrMsg() << endl;
    delete dbcp;
    exit(1);
  }

  /****************************************
   *   New ID request test
   ****************************************/

//   int32 foo = dbcp->reqID(1);
//   if (foo <= 0) {
//     cout << dbcp->getErrMsg() << endl;
//     delete dbcp;
//     exit(1);
//   }

//   DBscorename newScore;
//   newScore.id = foo;
//    vector<DBscorename> snlist;
//    snlist.push_back(newScore);
//    if (dbcp->putNewScoreName(snlist))
//      cout << dbcp->getErrMsg() << endl;


/****************************************
 *   Print out global map data
 ****************************************/

//   cout << "types #: " << dbcp->dbs.typemap.size() << endl;
//   cout << "score name #: " << dbcp->dbs.scorenames.size() << endl;
//   cout << "Region name #: " << dbcp->dbs.regionNameMap.size() << endl;
//   cout << "Synonym #: " << dbcp->dbs.synonymMap.size() << endl;
//   cout << "Region relationship #: " << dbcp->dbs.regionRelationMap.size() << endl;

  //showTypeMap(dbcp->typemap);
  //showScoreNames(dbcp->scorenames);    
  //showRegionNameMap(dbcp->regionNameMap);
  //showRelationMap(dbcp->regionRelationMap);

  
/****************************************
 *   Add new database user account
 ****************************************/

//   userGUI uiData;
//   string str1("admin2");
//   string str2("admin");
//   string str3("DB Administrator"); 
//   string str4("admin@cfn.upenn.edu");
//   uiData.setAccount(str1);
//   uiData.setPasswd(str2);
//   uiData.setName(str3);
//   uiData.setEmail(str4);
//   if (dbcp->putNewUser(uiData))
//     cout << dbcp->getErrMsg() << endl;


/****************************************
 *   Add new sessions and score values
 ****************************************/

//   int32 newID = dbcp->reqID(7);
//   if (newID <= 0) {
//     cout << dbcp->getErrMsg() << endl;
//     delete dbcp;
//     exit(1);
//   }

//   // one new patient record
//   DBpatient pRec;
//   pRec.patientID = newID++;
//   // two new DBsession objects
//   vector<DBsession> sList;
//   for (int i = 0; i < 2; i++) {
//     DBsession newSession;
//     newSession.id = newID++;
//     newSession.patient = pRec.patientID;
//     newSession.examiner = "dhu";
//     sList.push_back(newSession);
//   }
//   pRec.setSessionMap(sList);

//   // four DBscorevalue objects
//   set<int32> id_set;
//   for (int i = 0; i < 4; i++) {
//     DBscorevalue newSV;
//     newSV.id = newID++;
//     newSV.patient = pRec.patientID;
//     newSV.scorename = "fake scorename " + num2str(i + 1);
//     string tmpSV = "dhu scorevalue " + num2str(i + 1);
//     newSV.v_string = tmpSV;
//     newSV.setby = "dhu";
//     pRec.scores.insert(map<int32, DBscorevalue>::value_type(newSV.id, newSV));
//     if (newSV.id % 2 == 0)
//       id_set.insert(newSV.id);
//   }

//  if (dbcp->putScoreValues(id_set, pRec.scores)) 
//    cout << dbcp->getErrMsg() << endl;

//   //if (dbcp->putNewPatient(pRec)) 
//   //   cout << dbcp->getErrMsg() << endl;




/****************************************
 *   Test of time request
 ****************************************/
//   int32 t_stamp = dbcp->reqTime();
//   if (t_stamp <= 0) 
//     cout << dbcp->getErrMsg() << endl;

/****************************************
 *   patient list test
 ****************************************/

  
  //for (unsigned i = 0; i < dbcp->pList.size(); i++) 
  //  dbcp->pList[i].show();

//   DBpatientlist tmpRec;
//   tmpRec.id = 2901;
//   tmpRec.ownerID = 2351;
//   tmpRec.name = "rmt client test";
//   tmpRec.search_strategy = "case_sensitive any include 215";
//   tmpRec.notes = "rmt client notes";
//   tmpRec.runDate.setUnixTime(t_stamp);
//   tmpRec.patientIDs.insert(2217);
//   tmpRec.patientIDs.insert(2258);
//   //vector<DBpatientlist> vec;
//   //vec.push_back(tmpRec);
//   if (dbcp->putPatientList(tmpRec, true)) 
//     cout << dbcp->getErrMsg() << endl;

/****************************************
 *   patient search test
 ****************************************/

//    vector <patientMatch> pMatches;
//    patientSearchTags searchTag;
//    searchTag.init();
//    searchTag.case_sensitive = true;
//    searchTag.scoreName = "";
//    searchTag.relationship = "include";
//    searchTag.searchStr = "215";
//    if (dbcp->reqSearchPatient(searchTag, pMatches)) {
//      for (uint i = 0; i < pMatches.size(); i++) {
//        cout << "--------------------------------" << endl;
//        pMatches[i].show();
//      }
//    }
//    else
//      cout << dbcp->getErrMsg() << endl;
  

/****************************************
 *   patient score value request test
 ****************************************/

  DBpatient pRec;
  if (dbcp->reqOnePatient(2258, pRec)) 
    cout << dbcp->getErrMsg() << endl;

  //cout << "# of sessions: " << pRec.sessions.size() << endl;
  //cout << "# of scores: " << pRec.scores.size() << endl;
//   for (map<int32, DBscorevalue>::iterator iter = pRec.scores.begin(); 
//        iter != pRec.scores.end(); ++iter) {
//     cout << "-------------------------------" << endl;
//     cout << iter->first << endl;
//     iter->second.show();
//     cout << endl;
//   }

  pRec.scores[2627].v_string = "dhu new fake value";
  set<int> id_set;
  id_set.insert(2627);
  if (dbcp->putScoreValues(id_set, pRec.scores)) 
    cout << dbcp->getErrMsg() << endl;

//   DBscorevalue newSV;
//   newSV.id = 2627;
//   newSV.patient = 2258;
//   newSV.scorename = "fake scorename " + num2str(i + 1);
//   string tmpSV = "dhu scorevalue " + num2str(i + 1);
//   newSV.v_string = tmpSV;
//   newSV.setby = "dhu";
//   pRec.scores.insert(map<int32, DBscorevalue>::value_type(newSV.id, newSV));
//   id_set.insert(newSV.id);



  delete dbcp;
  return 0;
}

