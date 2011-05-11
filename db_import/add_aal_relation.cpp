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

void addRelationship(int, string, string);
vector<string> parseRelStr(string inputStr, string separator);
int getRelInfo(int, string, long *, string &);

const char * file_in = "aal.csv";
string name_space("AAL");
string dbHome("./");
string rDbName("region_name.db");
string rrDbName("region_relation.db");
string aDbName("admin.db");
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
    if (lineNo < 8)
      continue;

    // Remove the trailing "return" key
    stripchars(line, "\n");
    string tmpStr(line);
    vector<string> myRow = parseCSV(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 3
    if (myRow.size() != 6) {
      printf("Line #%d: the number of fields is %d\n", lineNo, myRow.size());
      for (uint i = 0; i < myRow.size(); i++) 
	cout << myRow[i] << endl;
      continue;
    }
    // add region name into region db
    string newName = myRow[1];
    if (newName.length() == 0) {
      printf("Line #%d: blank region name, line skipped\n", lineNo);
      continue;
    }

    string nn_equiv = myRow[4];
    if (nn_equiv.length() && nn_equiv != "?????") {
      int foo = addRelation_db(dbHome, rDbName, rrDbName, aDbName, username, 
			       newName, name_space, nn_equiv, "NN2002", "equiv"); 
      if (foo)
	printf("Error code %d: fails to add equiv relationship in line #%d\n", foo, lineNo);
    }

    addRelationship(lineNo, newName, myRow[5]);
  }

  return 0;
} 

/* Parse relationship field and add it into relationship db */
void addRelationship(int lineNo, string r1Name, string relStr)
{
  if (relStr.length() == 0)
    return;
  // Get AAL region ID first
  long r1ID = getRegionID(dbHome, rDbName, r1Name, "AAL");
  if (r1ID <= 0) {
    printf("Line #%d: failed to get region ID of %s\n", lineNo, r1Name.c_str());
    return;
  }

  // deal with relationship string
  vector<string> relList = parseRelStr(relStr, ", ");
  for (uint i = 0; i < relList.size(); i++) {
    long r2ID;
    string dbRel;
    int foo = getRelInfo(lineNo, relList[i], &r2ID, dbRel);
    if (foo)
      continue;

    foo = addRelation_db(dbHome, rrDbName, aDbName, username, r1ID, r2ID, dbRel); 
    if (foo)
      printf("Error code %d: fails to add relationship in line #%d: %s\n", 
	     foo, lineNo, relList[i].c_str());
  }
}

/* This function returns ID of an input region name. It first searches AAL namespace,
 * if it is not available, search NN2002 namespace. 
 * Returns 0 if r2 name is found;
 * returns 1 if input relationship string couldn't be parsed.
 * returns 2 if the region name is not available in either namespace;
 * returns -1 for db error.
 */
int getRelInfo(int lineNo, string inputRel, long * r2ID, string & outputRel)
{
  if (inputRel.length() == 0) 
    return 1;
  if (inputRel.length() < 9) {
    printf("Line #%d: can not parse relationship of %s\n", lineNo, inputRel.c_str());
    return 1;
  }

  string header = inputRel.substr(0, 9);
  if (header == "child of ")
    outputRel = "child";
  else if (header == "overlaps ")
    outputRel = "overlap";
  else {
    printf("Line #%d: can not parse relationship of %s\n", lineNo, inputRel.c_str());
    return 1;
  }

  string r2Name = inputRel.substr(9, inputRel.length() - 9);
  (*r2ID) = getRegionID(dbHome, rDbName, r2Name, "AAL");
  if (*r2ID < 0) {
    printf("Line #%d: db error when searching %s in AAL namespace\n", lineNo, r2Name.c_str());
    return -1;
  }

  if (*r2ID > 0) 
    return 0;

  (*r2ID) = getRegionID(dbHome, rDbName, r2Name, "NN2002");
  if (*r2ID < 0) {
    printf("Line #%d: db error when searching %s in NN2002 namespace\n", lineNo, r2Name.c_str());
    return -1;
  }
  if (*r2ID == 0) {
    printf("Line #%d: %s not found in either AAL or NN2002 namespace\n", lineNo, r2Name.c_str());
    return 2;
  }

  if (outputRel == "child")
    outputRel = "part-of";

  return 0;
}

/* This function parses an input string and separates it into individual sections.
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
