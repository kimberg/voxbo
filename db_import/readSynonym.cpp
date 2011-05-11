/* This program loads data from plain text synonym file to synonym database */

using namespace std;

#include "utils/br_util.h"
#include "vbutil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define MAXLENGTH 5000

void addRecords();
int addSynonym(int, string, long, string);
string convertName(string );
bool isValidSpec(string);
bool onList(string);

const char * nn_filename = "synonym.csv";
string name_space("NN2002");
string dbHome("./");
string rDbName("region_name.db");
string sDbName("synonym.db");
string aDbName("admin.db");
string admin_sField("Next Synonym ID");
string username("admin");
vector<string> removal;

/* Main function */
int main()
{
  addRecords();
  return 0;
} 

/* This function adds a certain score into synonym database */
void addRecords()
{
  FILE * ifp = fopen(nn_filename, "r");
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
    vector<string> myRow = parseCSV(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 7
    if (myRow.size() != 7) {
      printf("Line %d: the number of fields is %d\n", lineNo, myRow.size());
      printf("%s\n", line);
      continue;
    }

    string tmpSpec = myRow[2];
    if (!isValidSpec(tmpSpec))
      continue;

    // skip the line whose "synonym" or "hierarchy lookup" field is blank
    if (myRow[1].length() == 0 || myRow[4].length() == 0) {
      //printf("Line #%d: hierarchy lookup is blank, skipped\n", lineNo);
      continue;
    }
    if (myRow[1] == myRow[4]) {
      continue;
    }
    string synStr = convertName(myRow[1]);
    if (synStr == "") {
      //printf("Line #%d: (M) attached in synonym \"%s\", skipped\n", lineNo, myRow[1].c_str());
      continue;
    }
    
    string primaryName = convertName(myRow[4]);
    if (primaryName == "") {
      //printf("Line #%d: (M) attached in primary name \"%s\", skipped\n", lineNo, myRow[4].c_str());
      continue;
    }

    // skip the line whose name is same as hierarchy lookup
    if (synStr == primaryName) {
      //printf("Line #%d: synonym is identical to hierarchy lookup in lower case, skipped\n", lineNo);
      continue;
    }

    long srcID = atol(myRow[0].c_str());
    int foo = addSynonym(lineNo, synStr, srcID, primaryName);
    if (foo)
      break;
  } 

  fclose(ifp);

  // Remove synonyms on removal list
  for (uint i = 0; i < removal.size(); i++) {
    string delStr = removal[i];
    printf("Delete %s ...\n", delStr.c_str());
    int delStat = delSyn_db(dbHome, sDbName, delStr, name_space);
    if (delStat) 
      printf("Line #%d: fails to delete %s from synonym db.\n", lineNo, delStr.c_str());
  }

}

/* This function checks whether the string in species field is valid or not. */
bool isValidSpec(string spec)
{
  if (spec == "human" || spec == "Unspecified")
    return true;

  if (spec.size() >= 6 && spec.substr(0, 6) == "Macaca")
    return true;

  return false;
}

/* This function dumps some of the fields in a row into synonym database */
int addSynonym(int lineNo, string synStr, long srcID, string primary_in)
{
  bool chopExtra = false;
  string primary;
  string extraStr(" - not otherwise specified");
  int foo = chkRegionName(dbHome, rDbName, primary_in, name_space);
  if (foo < 0) {
    printf("Line %d: fails to check primary name availability of %s in region name db.\n", 
	   lineNo, primary_in.c_str());
    return foo;
  }
  else if (foo == 1)
    primary = primary_in;
  else {
    int strLen = primary_in.length();
    if (strLen > 26 && primary_in.substr(strLen - 26, 26) == extraStr) {
      string newName = primary_in.substr(0, strLen - 26);
      foo = chkRegionName(dbHome, rDbName, newName, name_space);
      if (foo == 1) {
	primary = newName;
	chopExtra = true;
      }
      else if (foo < 0) {
	printf("Line %d: fails to check availability of %s in region name db.\n", lineNo, newName.c_str());
	return foo;
      }
      else {
	printf("Line %d: unknown primary name (%s)\n", lineNo, primary_in.c_str());
	return -1;
      }
    }
  }

  string nameStr = synStr;
  string comments;
  if (chopExtra) {
    int nameLen = synStr.length();
    if (nameLen > 26 && synStr.substr(nameLen - 26, 26) == extraStr) {
      nameStr = synStr.substr(0, nameLen - 26);
      comments = "\"- not otherwise specified\" chopped from synonym and primary name";
    }
    else
      comments = "\"- not otherwise specified\" chopped from primary name";
  }

  foo = chkRegionName(dbHome, rDbName, nameStr, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check synonym availability of %s in region name db.\n", 
	   lineNo, nameStr.c_str());
    return -1;
  }
  // If synonym already exists, skip it instead of terminating the program
  if (foo == 1) {
    printf("Line #%d: %s exists in region name db.\n", lineNo, nameStr.c_str()); 
    return 0;
  }

  foo = chkSynonymStr(dbHome, sDbName, nameStr, name_space);
  if (foo < 0) {
    printf("Line #%d: fails to check availability of %s in synonym db.\n", lineNo, nameStr.c_str());
    return -1;
  }
  // If synonym already exists, skip this line
  if (foo == 1) {
    string pName;
    int bar = getPrimary(dbHome, sDbName, nameStr, name_space, pName);
    if (bar != 1) 
      printf("Line #%d: error in retrieving primary name from synonym db.\n", lineNo); 
    else if (pName != primary) {
      printf("Line #%d: %s exists in synonym db with different primary name.\n", 
	     lineNo, nameStr.c_str()); 
      if (!onList(nameStr))
	  removal.push_back(nameStr);
    }
    //else 
      //printf("Line #%d: %s exists in synonym db with same primary name.\n", lineNo, nameStr.c_str()); 
    return 0;
  }

  long sID = getAdmin_db(dbHome, aDbName, admin_sField);
  if (sID < 0) {
    printf("Line #%d: fails to get next available synonym ID from admin db\n", lineNo);
    return -1;
  }

  synonymRec sData;
  sData.clear();
  sData.setID(sID);
  sData.setName(nameStr);
  sData.setPrimary(primary);
  sData.setNameSpace(name_space);
  sData.setSourceID(srcID);
  sData.setAddDate(time(NULL));
  sData.setCreator(username);
  sData.setComments(comments);

  Dbt key(&sID, sizeof(long));  
  void * buff = sData.getBuffer();
  int size = sData.getBufferSize();
  Dbt data(buff, size);
  mydb sDB(dbHome, sDbName);
  try {
    int foo = sDB.getDb().put(NULL, &key, &data, DB_NOOVERWRITE);
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

/* This function checks whether the input string is identical to any element in removal list.*/
bool onList(string inputStr)
{
  for (uint i = 0; i < removal.size(); i++) {
    if (removal[i] == inputStr)
      return true;
  }

  return false;
}
