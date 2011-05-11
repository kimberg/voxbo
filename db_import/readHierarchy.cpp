/* This program loads data from plain text hierarchy file to brain region name database */

using namespace std;

#include "utils/br_util.h"
#include "tokenlist.h"
#include "vbutil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define MAXLENGTH 5000

void addRecords();
int addRegionName(int, string, string, long, string);
int addSynRec(int, string, string, string);
string convertName(string );

const char * nn_filename = "hierarchy.csv";
string name_space("NN2002");
string dbHome("./");
string rDbName("region_name.db");
string sDbName("synonym.db");
string aDbName("admin.db");
string admin_rField("Next Region ID");
string admin_sField("Next Synonym ID");
string username("admin");

/* Main function */
int main()
{
  addRecords();

  return 0;
} 

/* This function adds a certain record into db */
void addRecords()
{
  int foo = setAdmin_db(dbHome, aDbName, admin_rField, 1);
  if (foo) {
    printf("Fails to initialize region ID field in admin db\n");
    return;
  }

  foo = setAdmin_db(dbHome, aDbName, admin_sField, 1);
  if (foo) {
    printf("Fails to initialize synonym ID field in admin db\n");
    return;
  }

  FILE * ifp = fopen(nn_filename, "r");
  char line[MAXLENGTH];
  int lineNo = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // The first line is the column names, so it is skipped
    if (lineNo == 1)
      continue;

    // Remove the trailing "return" key
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

    string newName = convertName(myRow[1]);
    if (newName == "") {
      printf("Line #%d: blank region name or (M) attached in \"%s\", line skipped\n", lineNo, myRow(1));
      continue;
    }

    long srcID = atol(myRow(0));
    string abbrev = myRow[3];
    string link;
    if (myRow.size() == 7)
      link = myRow[6];
    else
      link = myRow[7];

    foo = addRegionName(lineNo, newName, abbrev, srcID, link);
    if (foo) {
      printf("Line #%d: fails to add region name into region db\n", lineNo);
      break;
    }

    string latinName = convertName(myRow[2]);
    if (latinName == "")
      printf("Line #%d: blank latin name or (M) attached in \"%s\", line skipped\n", lineNo, myRow(2));
    else if (latinName != newName) {
      foo = addSynRec(lineNo, latinName, newName, "NN2002.hierarchy.latin");
      if (foo)
	printf("Line #%d: fails to add latine name into synonym db\n", lineNo);
    }
    //else
      //printf("Line #%d: latin name is same as English name\n", lineNo);
    
    
    // abbreviation field is exported to region name table instead of synonym table now
    //     string abbrev = convertName(myRow[3]);
    //     if (abbrev == "")
    //       printf("Line #%d: blank latin name or (M) attached in \"%s\", line skipped\n", lineNo, myRow(3));
    //     else if (abbrev == newName) 
    //       printf("Line #%d: abbreviation is same as English name\n", lineNo);
    //     else if (abbrev == latinName) 
    //       printf("Line #%d: abbreviation is same as latin name\n", lineNo);
    //     else {
    //       foo = addSynRec(lineNo, abbrev, newName, "NN2002.hierarchy.abbreviation"); 
    //       if (foo)
    // 	printf("Line #%d: fails to add abbreviation into synonym db\n", lineNo);
    //     }
  } 

  fclose(ifp);
}

/* This function dumps some of the fields in a row into region name database */
int addRegionName(int lineNo, string newName, string abbrev, long srcID, string link)
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

  long rID = getAdmin_db(dbHome, aDbName, admin_rField);
  if (rID <= 0) {
    printf("Line #%d: fails to get next available region ID from admin db\n", lineNo);
    return -1;
  }

  regionRec regionData;
  regionData.setID(rID);
  regionData.setNameSpace(name_space);
  regionData.setName(newName);
  regionData.setAbbrev(abbrev);
  regionData.setOrgID(srcID);
  regionData.setSource("NN2002.hierarchy");
  regionData.setCreator(username);
  regionData.setAddDate(time(NULL));
  regionData.setLink(link);
  
  Dbt key(&rID, sizeof(long));
  void * buff = regionData.getBuffer();
  int size = regionData.getBufferSize();
  Dbt data(buff, size);

  mydb rDB(dbHome, rDbName);
  try {
    foo = rDB.getDb().put(NULL, &key, &data, DB_NOOVERWRITE);
    if (foo == DB_KEYEXIST) {
      printf("Line %d: region ID %ld exists\n", lineNo, rID);
      return 0;
    }
  }
  catch(DbException &e) {
    rDB.getDb().err(e.get_errno(), "Error in addRegionName()");
    return -1;
  } 
  catch(exception &e) {
    rDB.getDb().errx("Error in addRegionName(): %s", e.what());
    return -2;
  }

  rID++;
  foo = setAdmin_db(dbHome, aDbName, admin_rField, rID);
  if (foo) {
    printf("Line %d: fails to update next region ID in admin db\n", lineNo);
    return foo;
  }

  return 0;
}

/* add a certain synonym record */
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

