/* initScoreNames.cpp: populate patient score (attribute) names from plain text file. 
 * NOTE: This program updates database WITHOUT db environment! */

#include "tokenlist.h"
#include "mydb.h"
#include "bdb_tab.h"
#include "db_util.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

void addRecords(char*, mydb&);

/* Main function */
int main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("Usage: initScoreNames <txt_filename> <db_dir>\n");
    exit(0);
  }

  const string inputFile = argv[1];
  if (!vb_fileexists(inputFile)) {  // Make sure input file exists
    cout << "File not exists: " << inputFile << endl;
    exit(1);
  }

  string dirname = argv[2];
  if (!vb_direxists(dirname)) {  // Make sure db env dir exists
    cout << "DB env directory not exists: " << dirname << endl;
    exit(1);
  }

  if (dirname[dirname.size() - 1] != '/')
    dirname.append("/");  // don't let the trailing '/' mess up db file path
  string dbname = "scorenames.db";
  // Remove original db file if it already exists in env dir
  if (vb_fileexists(dirname + dbname)) {
    if (!rmDB(dirname + dbname)) {  
      cout << "Faild to remove original db file: " << dbname << endl;
      exit(1);
    }
  }

  // open db in non-transaction mode
  mydb newDB(dirname + dbname, mydb::cmp_lex);
  if (!newDB.isOpen()) {
    cout << "db open error: " << dirname + dbname << endl;
    exit(1);
  }

  addRecords(argv[1], newDB);
  newDB.close();

  return 0;
}


// Add records into db
void addRecords(char* txt_file, mydb& inputDB)
{
  FILE* ifp = fopen(txt_file, "r");
  // Make sure file is readable
  if (!ifp) {
    printf("File read error: %s\n", txt_file);
    return;
  }

  const int MAXLENGTH = 5000;
  char line[MAXLENGTH];
  int lineNo = 0;
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
    myRow.SetSeparator(" ");
    myRow.ParseLine(tmpStr);

    // Print out error message if the line is not blank but the number of field is not 7
    if (myRow.size() < 2) {
      printf("%s line #%d: the number of fields is %d\n", txt_file, lineNo, myRow.size());
      continue;
    }

    string scoreName = myRow[0];
    DBscorename sData;
    sData.name = scoreName;
    sData.screen_name = scoreName;
    sData.datatype = myRow[1];

    for (int i = 2; i < myRow.size(); i++) {
      string flag_str = myRow[i];
      sData.flags[flag_str] = "true";
    }

    if (addScoreName(inputDB, NULL, sData)) {
      printf("%s line #%d: failed to add %s into score db\n", txt_file, lineNo, scoreName.c_str());
    }
    inputDB.getDb().sync(0);
   } 

  fclose(ifp);
}

