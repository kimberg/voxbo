/* This program loads those brain region names from aal.csv to brain region name database */

using namespace std;

#include "utils/br_util.h"
#include "vbutil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define MAXLENGTH 5000

int addRegionName(int, string, long);

const char * file_in = "aal.csv";
string name_space("AAL");
string dbHome("./");
string rDbName("region_name.db");
string sDbName("synonym.db");
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
    // The first 7 lines are skipped
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
    long srcID = 0;
    if (myRow[0].length())
      srcID = atol(myRow[0].c_str());
    string newName = myRow[1];
    if (newName.length() == 0) {
      printf("Line #%d: blank region name, line skipped\n", lineNo);
      continue;
    }
    // Note that region names in this namespace are not converted to lower case
    int foo = addRegionName(lineNo, newName, srcID);
    if (foo)
      printf("Line #%d: fails to add region name into region db\n", lineNo);
  }

  return 0;
} 

/* This function adds a new region name into region name database */
int addRegionName(int lineNo, string newName, long srcID)
{
  // Make sure the name is unique among AAL namespace records in region name db
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
  // Make sure the name is unique among AAL namespace records in synonym db
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
  regionData.setOrgID(srcID);
  regionData.setSource("AAL");
  regionData.setCreator(username);
  regionData.setAddDate(time(NULL));
  foo = addRegion_db(dbHome, rDbName, aDbName, regionData);
  if (foo) {
    printf("Line #%d: failed to add %s into region name db\n", lineNo, newName.c_str());
    return 1;
  }
  return 0;
}

