/* load.cpp: load data from plain text file to patient database */

#include "tokenlist.h"
#include "vbutil.h"
#include "mydb.h"
#include "score.h"
#include "utils/admin_util.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

#define MAXLENGTH 5000

string dbHome("./");
string sdbName("score.db");
string adbName("admin.db");
vector <string> tables;

void addScore(char *);
string tailorStr(string);
void mkTab(string );

/* Main function */
int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: scoreConfig <initial filename>\n");
    exit(0);
  }

  try {
    addScore(argv[1]);
  }

  catch(DbException &e) {
    cerr << "Error loading databases. " << endl;
    cerr << e.what() << endl;
    return (e.get_errno());
  } 
  catch(exception &e) {
    cerr << "Error loading databases. " << endl;
    cerr << e.what() << endl;
    return -1;
  }

  return 0;
} 

/* This function adds a certain score into score database */
void addScore(char *filename)
{
  FILE * ifp = fopen(filename, "r");
  // Make sure file is readable
  if (!ifp) {
    printf("File read error: %s\n", filename);
    return;
  }

  char line[MAXLENGTH];
  int lineNo = 0;
  unsigned score_id = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // if a line starts with "#", ignore the line
    if (strchr("#\n", line[0]))
      continue;

    // remove the trailing "return" key
    stripchars(line, "\n");
    const string tmpStr(line);
    tokenlist myRow;
    myRow.SetQuoteChars("\"");
    myRow.SetSeparator(",");
    myRow.ParseLine(tmpStr);

    // Ignore line that has only space or tab characters
    if (myRow.size() == 1 && tailorStr(myRow[0]) == "")
      continue;

    // Print out error message if the line is not blank but the number of field is not 5
    if (myRow.size() != 5) {
      printf("%s line #%d: the number of fields is %d\n", filename, lineNo, myRow.size());
      continue;
    }

    score_id++;
    scoreRec sData;
    sData.clear();
    sData.setID(score_id);
    string scoreName = tailorStr(myRow[0]);
    sData.setName(scoreName);
    sData.setType(tailorStr(myRow[1]));
    string tableName = tailorStr(myRow[2]);
    sData.setTable(tableName);
    sData.setDesc(tailorStr(myRow[3]));
    sData.setFlags(tailorStr(myRow[4]));
  
    Dbt key(&score_id, sizeof(unsigned));    
    void *buff = sData.getBuffer();
    int size = sData.getBufferSize();
    Dbt data(buff, size);

    mydb scoreDB(dbHome, sdbName);
    int foo = scoreDB.getDb().put(NULL, &key, &data, 0);
    if (foo)
      printf("%s line #%d: failed to add %s into score db\n", filename, lineNo, scoreName.c_str());
    else
      mkTab(tableName);
  } 

  fclose(ifp);
  score_id++;
  setAdmin_db(dbHome, adbName, "Next Score ID", score_id);
}

/* Remove an input string's leading and traling white space characters */
string tailorStr(string inputStr)
{
  int strLen = inputStr.length();
  int start = 0, end = 0;
  int i;
  for (i = 0; i < strLen; i++) {
    if (inputStr[i] != 9 && inputStr[i] != 32) {
      start = i;
      break;
    }
  }
  // if string only includes space and tab characters, reset it to blank
  if (i == strLen)
    return "";

  for (int j = strLen - 1; j >= 0; j--) {
    if (inputStr[j] != 9 && inputStr[j] != 32) {
      end = j;
      break;
    }
  }
  // return the substring that does not leading and trailing space/tab characters
  return inputStr.substr(start, end - start + 1);
}

/* This function checks the input table name. If it is not on the list of tables, add it. */
void mkTab(string inputStr)
{
  for (uint i = 0; i < tables.size(); i++) {
    if (tables[i] == inputStr)
      return;
  }

  tables.push_back(inputStr);
  string dbFullName = inputStr + ".db";
  mydb newDB(dbHome, dbFullName, true);
}

