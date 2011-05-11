/* This program loads data from plain text brain region namespace file to namespace database */

using namespace std;

#include "tokenlist.h"
#include "vbutil.h"
#include <iostream>
#include <vector>
#include <string>

#include "mydb.h"
#include "name_space.h"

#define MAXLENGTH 5000

// Some gloabla variables
const char * inputFilename = "namespace.txt";
string dbHome("./");
string nDbName("namespace.db");

int addRecords(int, tokenlist);

/* Main function */
int main()
{
  FILE * ifp = fopen(inputFilename, "r");
  char line[MAXLENGTH];
  int lineNo = 0;
  while (fgets(line, MAXLENGTH, ifp)) {
    lineNo++;
    // The first line has column names, skipped
    if (lineNo == 1)
      continue;

    // remove the trailing "return" key
    stripchars(line, "\n");
    const string tmpStr(line);
    tokenlist myRow;
    myRow.SetQuoteChars("\"");
    myRow.SetSeparator(",");
    myRow.ParseLine(tmpStr);

    // Print out error message if the line is not blank but the number of field is less than 2
    if (myRow.size() < 2) {
      printf("Line #%d: the number of fields is %d\n", lineNo, myRow.size());
      for (int i = 0; i < myRow.size(); i++) 
	cout << myRow[i] << endl;
      continue;
    }

    int foo = addRecords(lineNo, myRow);
    if (foo)
      break;
  } 

  fclose(ifp);

  return 0;
} 

/* This function dumps some of the fields in a row into namespace database */
int addRecords(int lineNo, tokenlist myRow)
{
  namespaceRec nData;
  string nameStr = myRow[0];
  nData.setName(nameStr);
  nData.setDescription(myRow[1]);

  Dbt key((char *) nameStr.c_str(), nameStr.length() + 1);    
  void * buff = nData.getBuffer();
  int size = nData.getBufferSize();
  Dbt data(buff, size);
  mydb nDB(dbHome, nDbName);
  try {
    int foo = nDB.getDb().put(NULL, &key, &data, 0);
    if (foo) {
      printf("Line %d: namespace db put error\n", lineNo);
      return 1;
    }
  }
  catch (DbException &e) {
    nDB.getDb().err(e.get_errno(), "Error in addRecords()");
    return -1;
  } 
  catch(exception &e) {
    nDB.getDb().errx("Error in addRecords(): %s", e.what());
    return -2;
  }
  
  return 0;
}

