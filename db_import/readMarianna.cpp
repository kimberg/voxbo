/* This program loads those brain region names from mariana.csv to brain region name database */

using namespace std;

#include "utils/br_util.h"
#include "tokenlist.h"
#include "vbutil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define MAXLENGTH 5000

int addRegionName(int, string);
int addSynRec(int, string, string, string);
void addRelationship(int, string, string);
vector<string> parseRelStr(string inputStr, string separator);

const char * file_in = "marianna.csv";
string name_space("Marianna");
string dbHome("./");
string rDbName("region_name.db");
string sDbName("synonym.db");
string rrDbName("region_relation.db");
string aDbName("admin.db");
string admin_sField("Next Synonym ID");
string username("admin");

/* Main function */
int main()
{
  FILE * ifp = fopen(file_in, "r");
  if (!ifp) {
    printf("Fails to read the file: %s\n", file_in);
    exit(0);
  }

  char line[MAXLENGTH];
  int lineNo = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // The first 4 lines are skipped
    if (lineNo < 5)
      continue;

    // Remove the trailing "return" key
    stripchars(line, "\n");
    string tmpStr(line);
    vector<string> myRow = parseCSV(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 3
    if (myRow.size() != 3) {
      printf("Line #%d: the number of fields is %d\n", lineNo, myRow.size());
      for (uint i = 0; i < myRow.size(); i++) 
	cout << myRow[i] << endl;
      continue;
    }
    // add region name into region db
    string newName = myRow[0];
    if (newName.length() == 0) {
      printf("Line #%d: blank region name, line skipped\n", lineNo);
      continue;
    }
    // Note that region names in this namespace are not converted to lower case
    int foo = addRegionName(lineNo, newName);
    if (foo) {
      printf("Line #%d: fails to add region name into region db\n", lineNo);
      break;
    }
    // synonym is not converted to lowercase either
    string x_synonym = myRow[1];
    if (x_synonym == newName)
      printf("Line #%d: synonym is identical to region name", lineNo);
    else if (x_synonym.length()) {
      foo = addSynRec(lineNo, x_synonym, newName, "extra synonym from Marianna");
      if (foo)
	printf("Line #%d: fails to add extra synonym into synonym db\n", lineNo);
    }
    // add relationship
    string relStr = myRow[2];
    addRelationship(lineNo, newName, relStr);
  }

  return 0;
} 

/* This function adds a new region name into region name database */
int addRegionName(int lineNo, string newName)
{
  int foo = chkRegionName(dbHome, rDbName, newName, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check region name availability in region db\n", lineNo);
    return foo;
  }
  // If region name already exists, skip it instead of terminating the program
  if (foo == 1) {
    printf("Line #%d: region \"%s\" exists in region name db\n", lineNo, newName.c_str());
    return 0;
  }

  foo = chkSynonymStr(dbHome, sDbName, newName, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check region name availability in synonym db\n", lineNo);
    return foo;
  }
  // If region name already exists, skip it instead of terminating the program
  if (foo == 1) {
    printf("Line #%d: region name \"%s\" exists in synonym name db\n", lineNo, newName.c_str());
    return 1;
  }

  regionRec regionData;
  regionData.setNameSpace(name_space);
  regionData.setName(newName);
  regionData.setSource("Marianna");
  regionData.setCreator(username);
  regionData.setAddDate(time(NULL));
  foo = addRegion_db(dbHome, rDbName, aDbName, regionData);
  if (foo) {
    printf("Line #%d: failed to add %s into region name db\n", lineNo, newName.c_str());
    return 1;
  }
  return 0;
}

/* Add a synonym record into synonym db */
int addSynRec(int lineNo, string name, string primary, string comments)
{
  int foo = chkRegionName(dbHome, rDbName, name, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check synonym's uniqueness in region db\n", lineNo);
    return -1;
  }
  // If region name already exists, skip it instead of terminating the program
  if (foo == 1) {
    printf("Line #%d: synonym \"%s\" exists in region name db\n", lineNo, name.c_str());
    return 0;
  }

  foo = chkSynonymStr(dbHome, sDbName, name, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check availability of %s in synonym db\n", lineNo, name.c_str());
    return -1;
  }
  // If synonym already exists, skip it instead of terminating the program
  if (foo == 1) {
    printf("Line #%d: synonym \"%s\" exists in synonym db\n", lineNo, name.c_str()); 
    return 0;
  }

  long sID = getAdmin_db(dbHome, aDbName, admin_sField);
  if (sID <= 0) {
    printf("Line #%d: fails to get next available synonym ID from admin db\n", lineNo);
    return -1;
  }

  synonymRec sData;
  sData.setID(sID);
  sData.setName(name);
  sData.setPrimary(primary);
  sData.setNameSpace(name_space);
  sData.setAddDate(time(NULL));
  sData.setCreator(username);
  sData.setComments(comments);

  Dbt key(&sID, sizeof(long));
  void *buff = sData.getBuffer();
  int size = sData.getBufferSize();
  Dbt data(buff, size);

  mydb sDB(dbHome, sDbName);
  try {
    foo = sDB.getDb().put(NULL, &key, &data, DB_NOOVERWRITE);
    if (foo == DB_KEYEXIST) {
      printf("Line %d: key %ld exists\n", lineNo, sID); 
      return 1;
    }
  }
  catch(DbException &e) {
    sDB.getDb().err(e.get_errno(), "Error in addSynRec()");
    return -1;
  } 
  catch(exception &e) {
    sDB.getDb().errx("Error in addSynRec(): %s", e.what());
    return -2;
  }

  sID++;
  foo = setAdmin_db(dbHome, aDbName, admin_sField, sID);
  if (foo < 0) {
    printf("Line #%d: fails to set next available synonym ID (%ld) from admin db\n", lineNo, sID);
    return -1;
  }

  return 0;
}

/* Parse relationship field and add it into relationship db */
void addRelationship(int lineNo, string regionName, string relStr)
{
  // "NONE" means no relationship 
  if (relStr == "NONE")
    return;
  // "equiv" means it is equivalent to the same region in NN2002
  if (relStr == "equiv") {
    string r2Name = toLowerCase(regionName);
    int foo = addRelation_db(dbHome, rDbName, rrDbName, aDbName, username, 
		   regionName, name_space, r2Name, "NN2002", "equiv"); 
    if (foo)
      printf("Error code %d: fails to add relationship in line #%d: %s\n", foo, lineNo, relStr.c_str());
    return;
  }
  // "equiv <string>" means it is equivalent to a region in NN2002 whose name is the string   
  if (relStr.length() > 6 && relStr.substr(0, 6) == "equiv ") {
    int strLen = relStr.length();
    string r2Name = relStr.substr(6, strLen - 6);
    int foo = addRelation_db(dbHome, rDbName, rrDbName, aDbName, username, 
		   regionName, name_space, r2Name, "NN2002", "equiv"); 
    if (foo)
      printf("Error code %d: fails to add relationship in line #%d: %s\n", foo, lineNo, relStr.c_str());
    return;
  }
  // deal with "includes <>; includes <> ..."
  vector<string> relList = parseRelStr(relStr, "; ");
  for (uint i = 0; i < relList.size(); i++) {
    if (relList[i].length() > 9 && relList[i].substr(0, 9) == "includes ") {
      int strLen = relList[i].length();
      string r2Name = relList[i].substr(9, strLen - 9);
      int foo = addRelation_db(dbHome, rDbName, rrDbName, aDbName, username, 
		     r2Name, "NN2002", regionName, name_space, "part-of"); 
      if (foo) 
	printf("Error code %d: fails to add relationship for %s in line #%d: %s\n", 
	       foo, regionName.c_str(), lineNo, relList[i].c_str());
    }
    else
      printf("Unknown relationship in line #%d: %s\n", lineNo, relList[i].c_str());
  }
}

/* THis function parses an input string and separates it into individual sections.
 * The first argument is the input string, the second is the separator string. */
vector<string> parseRelStr(string inputStr, string sepStr)
{
  string foo = inputStr;
  vector <string> sections;
  int strLen = foo.length();
  int sepLen = sepStr.length();
  if (strLen == 0 || sepLen == 0) {
    sections.push_back(inputStr);
    return sections;
  }
    
  string bar;
  while (foo.find(sepStr) != string::npos) {
    int sepPost = foo.find(sepStr);
    if (sepPost == 0)
      bar = "";
    else
      bar = foo.substr(0, sepPost);
    sections.push_back(bar);
    foo = foo.substr(sepPost + sepLen, foo.length() - sepPost - sepLen);
  }
  sections.push_back(foo);

  return sections;
}
