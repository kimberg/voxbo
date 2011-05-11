/* load.cpp: load data from plain text file to patient database */

using namespace std;

#include "utils/br_util.h"
#include "tokenlist.h"
#include "vbutil.h"
#include <iostream>
#include <vector>
#include <string>

#define MAXLENGTH 5000

// Some gloabla variables
const char * inputFilename = "hierarchy.csv";
string name_space("NN2002");
string dbHome("./");
string rName("region_name.db");
string rrName("region_relation.db");
string aDbName("admin.db");
string id_field("Next Relationship ID");
string username("admin");

void addRecords();
int addRegionRelation(int, string, long);
long getRegionID(string, string, string, long);
string convertName(string );

/* Main function */
int main()
{
  int foo = setAdmin_db(dbHome, aDbName, id_field, 1);
  if (foo) {
    printf("Unable to initialize relationship ID to 1 in admin db\n");
    return 0;
  }

  addRecords();
  return 0;
} 

/* This function adds a certain score into score database */
void addRecords()
{
  FILE * ifp = fopen(inputFilename, "r");
  char line[MAXLENGTH];
  int lineNo = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // The first line is the column names, so it is skipped
    if (lineNo == 1)
      continue;

    // remove the trailing "return" key
    stripchars(line, "\n");
    const string tmpStr(line);
    tokenlist myRow;
    myRow.SetQuoteChars("\"");
    myRow.SetSeparator(",");
    myRow.ParseLine(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 7 or 8
    if (myRow.size() != 8 && myRow.size() != 7) {
      printf("Line #%d: the number of fields is %d\n", lineNo, myRow.size());
      for (int i = 0; i < myRow.size(); i++) 
	cout << myRow[i] << endl;
      continue;
    }
    // Skip the region whose parent ID is same as source ID
    if (myRow[0] == myRow[5]) {
      printf("Line #%d: source ID is same as parent ID, skipped\n", lineNo);
      continue;
    }

    string newName = convertName(myRow[1]);
    if (newName == "") {
      printf("Line #%d: blank region name or (M) attached in \"%s\", line skipped\n", lineNo, myRow(1));
      continue;
    }

    long parentID = atol(myRow(5));
    int foo = addRegionRelation(lineNo, newName, parentID);
    if (foo)
      break;
  } 

  fclose(ifp);
}

/* This function dumps some of the fields in a row into region relation database */
int addRegionRelation(int lineNo, string r1_name, long parentID)
{
  long relationID = getAdmin_db(dbHome, aDbName, id_field);
  if (relationID <= 0) {
    printf("Line #%ld: fails to get next available relationship ID from admin db", relationID);
    return -1;
  }

  long r1_id = getRegionID(dbHome, rName, r1_name, name_space);
  if (r1_id <= 0) {
    printf("Line #%d: fails to get region ID of %s from region name db\n", lineNo, r1_name.c_str());
    return 1;
  }

  long r2_id = getRegionID(dbHome, rName, name_space, parentID);
  if (r2_id <= 0) {
    printf("Line %d: parent ID not found (source ID is %ld)\n", lineNo, parentID);
    return -1;
  }
  if (r1_id == r2_id) {
    printf("Line %d: region ID is same as parent ID, skipped\n", lineNo);
    return 0;
  }

  regionRelationRec rrData;
  rrData.setID(relationID);
  rrData.setRegion1(r1_id);
  rrData.setRegion2(r2_id);
  rrData.setRelationship("child");
  rrData.setAddDate(time(NULL));
  rrData.setCreator(username);
  rrData.setComments("NN2002.hierarchy.parent");

  Dbt key(&relationID, sizeof(long));    
  void * buff = rrData.getBuffer();
  int size = rrData.getBufferSize();
  Dbt data(buff, size);
  mydb regionRelationDB(dbHome, rrName);
  try {
    int foo = regionRelationDB.getDb().put(NULL, &key, &data, 0);
    if (foo) {
      printf("Line %d: relationship db put error\n", lineNo);
      return 1;
    }
  }
  catch (DbException &e) {
    regionRelationDB.getDb().err(e.get_errno(), "Error in addRegionRelation()");
    return -1;
  } 
  catch(exception &e) {
    regionRelationDB.getDb().errx("Error in addRegionRelation(): %s", e.what());
    return -2;
  }
  
  relationID++;
  int bar = setAdmin_db(dbHome, aDbName, id_field, relationID);
  if (bar) {
    printf("Line %d: fails to set relationship ID to %ld\n", lineNo, relationID);
    return 2;
  }

  return 0;
}

/* This function searches region name db according to its original ID and namespace
 * and returns the region ID.
 * Returns -1 or -2 for exceptions;
 * returns 0 if it is not found or any other errors;
 */
long getRegionID(string dbHome, string rDbName, string name_space, long orgID)
{
  mydb rDB(dbHome, rDbName);  
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  long foo = 0;
  try {    
    rDB.getDb().cursor(NULL, &cursorp, 0);    
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      if (rData.getNameSpace() == name_space && rData.getOrgID() == orgID) {
	foo = rData.getID();
	break;
      }
    }
  } 
  catch(DbException &e) {
    rDB.getDb().err(e.get_errno(), "Error in getRegionID()");
    foo = -1;
  } 
  catch(exception &e) {
    rDB.getDb().errx("Error in getRegionID(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function first checks the input region name. 
 * If it ends with "(H)", remove that part and convert it to lower case letters. 
 * If if ends with "(M)", skip the record completely;
 */
string convertName(string inputStr) 
{
  int strLen = inputStr.length();
  if (strLen < 4) 
    return toLowerCase(inputStr);

  string newStr;
  string last4 = inputStr.substr(strLen - 4, 4);
  if (last4 == " (M)")
    return "";
  else if (last4 == " (H)")
    newStr = inputStr.substr(0, strLen - 4);
  else
    newStr = inputStr;

  return toLowerCase(newStr);
}

