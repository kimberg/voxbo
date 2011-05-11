/* This program adds Brodmann areas into brain region name database
 * It reads the plain text file "brodmann.info", which is generated 
 * from brodmann.nii file. FOr each Brodmann area region name, two 
 * synonyms are added too: BA<number> and BA <number>.
 * Note that no relationship record is added from this file.
 */

using namespace std;

#include "utils/br_util.h"
#include "vbutil.h"
#include <iostream>
#include <fstream>
#include <string>

#define MAXLENGTH 5000

int addRegionName(int, string);
int addSynRec(int, string, string);

const char * file_in = "brodmann.info";
string name_space("Brodmann");
string dbHome("./");
string rDbName("region_name.db");
string sDbName("synonym.db");
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
    // Remove the trailing "return" key
    stripchars(line, "\n");
    string tmpStr(line);
    uint post1 = tmpStr.find("#");
    uint post2 = tmpStr.find(":");
    if (post1 == string::npos || post2 == string::npos || post2 <= post1 + 1) {
      printf("Line #%d is invalid: %s\n", lineNo, tmpStr.c_str());
      continue;
    }      
    
    string ba_no = tmpStr.substr(post1 + 1, post2 - post1 - 1);
    string ba_name = "Brodmann's Area " + ba_no;
    int foo = addRegionName(lineNo, ba_name);
    if (foo) {
      printf("Line #%d: fails to add %s into region db\n", lineNo, ba_name.c_str());
      continue;
    }

    string ba_s1 = "BA" + ba_no;
    foo = addSynRec(lineNo, ba_s1, ba_name);
    if (foo) {
      printf("Line #%d: fails to add %s into synonym db\n", lineNo, ba_s1.c_str());
      continue;
    }

    string ba_s2 = "BA " + ba_no; 
    foo = addSynRec(lineNo, ba_s2, ba_name);
    if (foo) {
      printf("Line #%d: fails to add %s into synonym db\n", lineNo, ba_s2.c_str());
      continue;
    }
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
  regionData.setSource("brodmann.nii");
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
int addSynRec(int lineNo, string name, string primary)
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

