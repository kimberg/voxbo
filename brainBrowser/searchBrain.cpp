
// searchBrain.cpp
// Copyright (c) 2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Dongbo Hu

/* This program searches brain region in text mode.
 */
 
#include "searchBrain.h"

string dbHome;
string rDbName = "region_name.db";
string rrDbName = "region_relation.db";
string sDbName = "synonym.db";
bool a_flag, ik_flag, ins_flag, ir_flag, is_flag;
string keyword_in, rName_in, synm_in, ns_in;

int main(int argc, char *argv[]) {
  // Validate arguments
  int foo = parseArg(argc - 1, argv + 1);
  if (!foo)
    help_msg();
  // Find directory where bdb files are located
  if (!setDbDir()) {
    err_msg();
    exit(1);
  }

  vector <regionRec> regionList;
  vector <synonymRec> synmList;
  if (ns_in.length()) {
    if (keyword_in.length()) {
      searchRegionName(keyword_in, ik_flag, ns_in, regionList);
      searchSynonym(keyword_in, ik_flag, ns_in, synmList);
    }
    else if (!rName_in.length() && !synm_in.length()) {
      // collect all names in a certain namespace
      searchRegionNS(ns_in, regionList);
      searchSynmNS(ns_in, synmList);
    }
    else {
      if (rName_in.length())
	searchRegionName(rName_in, ir_flag, ns_in, regionList);
      if (synm_in.length())
	searchSynonym(synm_in, is_flag, ns_in, synmList);
    }
  }
  else if (keyword_in.length()) {
    searchRegionName(keyword_in, ik_flag, regionList);
    searchSynonym(keyword_in, ik_flag, synmList);
  }
  else {
    if (rName_in.length())
      searchRegionName(rName_in, ir_flag, regionList);
    if (synm_in.length())
      searchSynonym(synm_in, is_flag, synmList);
  }
   
  showMatches(regionList);
  showMatches(synmList);

  return 0;
}

/* Parse arguments in command line 
 * returns 0 if arguments are in illegal format;
 * returns 1 otherwise. */
int parseArg(int argNo, char *inputArg[])
{
  if (argNo == 0) 
    return 0;

  vector <string> argList;
  for (int i = 0; i < argNo; i++) {
    string tmpStr(inputArg[i]);
    argList.push_back(tmpStr);
  }

  // by default, both detailed view and case-insensi
  a_flag = ik_flag = ins_flag = ir_flag = is_flag = false;
  for (int j = 0; j < (int) argList.size(); j++) {
    if (argList[j] == "-h")
      return 0;
    if (argList[j] == "-a" && (j == argNo -1 || argList[j + 1][0] == '-'))
      a_flag = true;
    else if ((argList[j] == "-k" || argList[j] == "-ik") && j != argNo - 1) {
      if (rName_in.length() || keyword_in.length() || argList[j + 1][0] == '-')
	return 0;
      keyword_in = argList[j + 1];
      if (argList[j] == "-ik")
	ik_flag = true;
      j++;
    }
    else if ((argList[j] == "-r" || argList[j] == "-ir") && j != argNo - 1) {
      if (rName_in.length() || keyword_in.length() || argList[j + 1][0] == '-')
	return 0;
      rName_in = argList[j + 1];
      if (argList[j] == "-ir")
	ir_flag = true;
      j++;
    }
    else if ((argList[j] == "-s" || argList[j] == "-is") && j != argNo - 1) {
      if (synm_in.length() || keyword_in.length() || argList[j + 1][0] == '-')
	return 0;
      synm_in = argList[j + 1];
      if (argList[j] == "-is")
	is_flag = true;
      j++;
    }
    else if ((argList[j] == "-ns" || argList[j] == "-ins") && j != argNo - 1) {
      if (ns_in.length() || argList[j + 1][0] == '-')
	return 0;
      ns_in = argList[j + 1];
      if (argList[j] == "-ins")
	ins_flag = true;     
      j++;
    }
    else 
      return 0;
  }

  return 1;
}

/* This is the usage message that will be printed out if the arguments are invalid. */
void help_msg()
{
  printf("Usage: searchBrain <flag_1> <value_1> [<flag_2> <value_2>] ...\n");
  printf("flags:\n");
  printf("    -h                print out this screen\n");
  printf("    -k <keyword>      search both regions names and synonyms by case-sensitive string\n");
  printf("    -ik <keyword>     search both regions names and synonyms by case-insensitive string\n");
  printf("    -r <region name>  search regions names by case-sensitive string\n");
  printf("    -ir <region name> search region names by case-insensitive string\n");
  printf("    -s <synonym>      search synonyms by case-sensitive string\n");
  printf("    -is <synonym>     search synonyms by case-insensitive string\n");
  printf("    -ns <namespace>   search region names and/or synonyms that are in a case-sensitive namespace string\n"); 
  printf("                      (NN2002, AAL or Brodmann)\n");
  printf("    -ins <namespace>  search region names and/or synonyms that are in a case-insensitive namespace string\n");
  printf("    -a                print all information available about the matched regions\n");

  printf("\n");
  printf("======= ACKNOWLEDGEMENTS ========\n\n");
  printf("This initial release of the VoxBo Brain Structure Browser was written by Dongbo Hu (code) \
and Daniel Kimberg (sage advice). ");
  printf("It is distributed along with structure information derived from the NeuroNames project \
(Bowden and Dubach, 2002) as well as the AAL atlas (Automatic Anatomical Labeling, \
Tzourio-Mazoyer et al., 2002).\n\n");
  printf("References:\n\n");
  printf("* Tzourio-Mazoyer N, Landeau B, Papathanassiou D, Crivello F, Etard O, Delcroix N, \
Mazoyer B, Joliot M (2002). \"Automated Anatomical Labeling of activations in SPM using a Macroscopic Anatomical \
Parcellation of the MNI MRI single-subject brain.\" NeuroImage 15 (1), 273-89.\n\n");
  printf("* Bowden D and Dubach M (2003).  NeuroNames 2002.  Neuroinformatics, 1, 43-59.\n");
  printf("\n");
}

/* Set location of bdb files. */
bool setDbDir()
{
  vector <string> dirList;
  dirList.push_back("/usr/share/brainregions/");
  dirList.push_back("/usr/local/brainregions/");

  char *homeDir = getenv("HOME");
  if (homeDir) {
    string tmpStr(homeDir);
    tmpStr.append("/brainregions/");
    dirList.push_back(string(tmpStr));
  }

  dirList.push_back("./");

  unsigned i;
  for (i = 0; i < dirList.size(); i++) {
    string fn1 = dirList[i] + rDbName;
    string fn2 = dirList[i] + rrDbName;
    string fn3 = dirList[i] + sDbName;
    if (isReadableReg(fn1) && isReadableReg(fn2) && isReadableReg(fn3)) {
      dbHome = dirList[i];
      break;
    }
  }

  if (i < dirList.size())
    return true;

  return false;
}

/* Check whether the input file is a regular readable file. */
bool isReadableReg(string inputFile)
{
  struct stat fileInfo;
  int foo = stat(inputFile.c_str(), &fileInfo);
  if (foo || !(S_IFREG & fileInfo.st_mode))
    return false;

  if (fileInfo.st_uid == geteuid() && (S_IRUSR & fileInfo.st_mode))
    return true;
  if (fileInfo.st_gid == getegid() && (S_IRGRP & fileInfo.st_mode))
    return true;
  if (S_IROTH & fileInfo.st_mode)
    return true;

  return false;
}

/* Print error message when db files are not found. */
void err_msg()
{
  printf("Database files not found in the following directories:\n");
  printf("  /usr/share/brainregions/\n");
  printf("  /usr/local/brainregions/\n");
  printf("  $HOME/brainregions/\n");
  printf("  ./\n");
  printf("Application aborted.\n");
}

/* This function searches a certain string in region name db and 
 * put matches in outputList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchRegionName(string nameStr, bool nameFlag, vector<regionRec> &outputList)
{
  printf("Searching %s%s records that contain %s ...\n", 
	 dbHome.c_str(), rDbName.c_str(), nameStr.c_str());
  int searchStat = findRegionNames(dbHome, rDbName, nameStr, nameFlag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", rDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function searches a certain string in a certain namespace 
 * in region name db and put matches in matchList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchRegionName(string nameStr, bool nameFlag, string nsStr, vector <regionRec> &outputList)
{
  printf("Searching %s%s records that contain %s in %s namespace ...\n", 
	 dbHome.c_str(), rDbName.c_str(), nameStr.c_str(), nsStr.c_str());
  int searchStat = findRegionNames(dbHome, rDbName, nameStr, nameFlag, nsStr, ins_flag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", rDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function searches regions that belong to a certain namespace and 
 * put matches in outputList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchRegionNS(string nsStr, vector <regionRec> &outputList)
{
  printf("Searching %s%s records that are in %s naemspace ...\n", 
	 dbHome.c_str(), rDbName.c_str(), nsStr.c_str());
  int searchStat = findRegionNS(dbHome, rDbName, nsStr, ins_flag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", rDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function searches a certain string in synonym db and 
 * put matches in outputList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchSynonym(string synmStr, bool synmFlag, vector <synonymRec> &outputList)
{
  printf("Searching %s%s records that contain %s ...\n", 
	 dbHome.c_str(), sDbName.c_str(), synmStr.c_str());
  int searchStat = findSynonyms(dbHome, sDbName, synmStr, synmFlag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", sDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function searches a certain string in a certain namespace 
 * in synonym db and put matches in matchList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchSynonym(string synmStr, bool synmFlag, string nsStr, vector <synonymRec> &outputList)
{
  printf("Searching %s%s records that contain %s in %s namespace ...\n", 
	 dbHome.c_str(), sDbName.c_str(), synmStr.c_str(), nsStr.c_str());
  int searchStat = findSynonyms(dbHome, sDbName, synmStr, synmFlag, nsStr, ins_flag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", sDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function searches regions that belong to a certain namespace and 
 * put matches in outputList.
 * Returns 0 if search is successful, non-zero otherwise.
 */
void searchSynmNS(string nsStr, vector <synonymRec> &outputList)
{
  printf("Searching %s%s records that are in %s namespace ...\n", 
	 dbHome.c_str(), sDbName.c_str(), nsStr.c_str());
  int searchStat = findSynmNS(dbHome, sDbName, nsStr, ins_flag, outputList);
  if (searchStat) {
    printf("Search failed in %s\n", sDbName.c_str());
    exit(1);
  }
  printSummary(outputList.size());
}

/* This function prints out the number of matches. */
void printSummary(unsigned matchNo)
{
  if (matchNo == 0)
    printf("No match found\n");
  else if (matchNo == 1)
    printf("1 match found\n"); 
  else 
    printf("%u matches found\n", matchNo);
}

/* This function prints out the matches found. */
void showMatches(vector <regionRec> rList)
{
  unsigned matchNo = rList.size();
  if (matchNo == 0)
    return;

  printf("\nRecord(s) found in region name db:\n");
  // Default brief view
  for (unsigned i = 0; i < matchNo; i++) {
    string briefMsg;
    if (ns_in.length())
      briefMsg = rList[i].getName();
    else
      briefMsg = rList[i].getName() + " (" + rList[i].getNameSpace() + ")";
    printf("(%u) %s\n", i + 1, briefMsg.c_str());

    if (!a_flag)
      continue;

    showRegion(rList[i]);
  }
}

/* This function prints out the matches found. */
void showMatches(vector <synonymRec> sList)
{
  unsigned matchNo = sList.size();
  if (matchNo == 0)
    return;

  printf("\nRecord(s) found in synonym db:\n");
  // Default brief view
  for (unsigned i = 0; i < matchNo; i++) {
    string sName = sList[i].getName();
    string rName = sList[i].getPrimary();    
    string briefMsg;
    if (ns_in.length())
      briefMsg = sName + " (synonym of \"" + rName + "\")";
    else
      briefMsg = sName + " (synonym of \"" + rName + "\" in " + sList[i].getNameSpace() + ")";
    printf("(%u) %s\n", i + 1, briefMsg.c_str());
    
    if (!a_flag)
      continue;

    string nsName = sList[i].getNameSpace();
    regionRec myRegion;
    int foo = getRegionRec(dbHome, rDbName, rName, nsName, myRegion);
    if (foo != 1) {
      printf("Region name db error.\n");
      return;
    }
    showRegion(myRegion);
  }
}

/* This function collects a certain region's detailed information and prints them out. */
void showRegion(regionRec myRegion) 
{
  long rID = myRegion.getID();
  // Get parent and child(ren) information
  string parentStr;
  vector <string> cList;
  int foo = setParentChild(rID, parentStr, cList);
  if (foo)
    return;
  // Get synonyms
  string rName = myRegion.getName();
  string nsName = myRegion.getNameSpace();
  vector <string> symList;
  foo = getSynonym(dbHome, sDbName, rName, nsName, symList);
  if (foo < 0) {
    printf("Synonym db exception\n");
    return;
  }
  // Get relationships
  vector <string> relList;
  foo = setRelationships(rID, relList);
  if (foo)
    return;

  printf("Region name: %s\n", myRegion.getName().c_str());
  printf("Namespace: %s\n", myRegion.getNameSpace().c_str());
  printf("Parent's name: %s\n", parentStr.c_str());
  printf("Child(ren)'s name:\n");
  for (uint i = 0; i < cList.size(); i++)
    printf("  * %s\n", cList[i].c_str());
  printf("Source: %s\n", myRegion.getSource().c_str());
  printf("Link: %s\n", myRegion.getLink().c_str());
  printf("Relationship(s):\n");
  for (uint j = 0; j < relList.size(); j++)
    printf("  * %s\n", relList[j].c_str());
  printf("\n");
}

/* This function returns parent and child names. */
int setParentChild(long rID, string &pStr, vector <string> &cStrList)
{
  long pID = 0;
  vector <long> cList;
  int foo = getParentChild(dbHome, rrDbName, rID, &pID, cList);
  if (foo < 0) {
    printf("DB error when searching parent and child in showMatches()\n");
    return 1;
  }

  string spaceStr;
  if (pID) {
    foo = getRegionName(dbHome, rDbName, pID, pStr, spaceStr);
    if (foo != 1) {
      printf("DB error when getting parent name in showMatches()\n");
      return 2;
    }
  }

  for (unsigned i = 0; i < cList.size(); i++) {
    long cID = cList[i];
    string cName, spaceStr;
    foo = getRegionName(dbHome, rDbName, cID, cName, spaceStr);
    if (foo != 1) {
      printf("DB error when getting child name in showMatches()\n");
      return 3;
    }
    cStrList.push_back(cName);
  }
  
  return 0;
} 
 
/* This function returns all relationships of a certain region ID and put it in the last argument. */
int setRelationships(long rID, vector <string> &outputList)
{
  vector<long> r2List;
  vector<string> relList;
  int foo = getRel_ui(dbHome, rrDbName, rID, r2List, relList);
  if (foo < 0) {
    printf("Relationship db error\n");
    return 1;
  }

  for (unsigned i = 0; i < relList.size(); i++) {
    string r2_name, r2_space;
    foo = getRegionName(dbHome, rDbName, r2List[i], r2_name, r2_space);
    if (foo != 1) {
      printf("Region name db error\n");
      return 2;
    }

    string tmpStr = relList[i] + ": " + r2_name + " (" + r2_space + ")";
    outputList.push_back(tmpStr);
  }

  return 0;
}

/*********************************************************************
* This function is copied from vbutil.cpp (comments removed).        
* It uses the input regular expression to validate the               
* input string. If the input regular expression matches the input    
* string, then true is returned. Otherwise, false is returned.       
*********************************************************************/
// bool validateString(const char *regularExp, const char *theString)
// {
//   regex_t regex;
//   memset(&regex, 0, sizeof(regex_t));

//   int err = 0;
//   if ( (err = regcomp(&regex, regularExp, 0)) != 0 )
//   {
//     int len = regerror(err, &regex, NULL, 0);
//     char regexErr[len];
//     regerror(err, &regex, regexErr, len);

//     printf("Compiling regular expression: [%s]", regerror);
//     exit(1);
//   }

//   size_t no_sub = 1;
//   regmatch_t *result;
//   if((result = (regmatch_t *) malloc(sizeof(regmatch_t) * no_sub))==0)
//   {
//     perror("Unable to allocate memory for a regmatch_t.");
//     exit(1);
//   }

//   if (regexec(&regex, theString, no_sub, result, REG_NOSUB) == 0)
//   {
//     regfree(&regex);
//     return true;
//   }

//   regfree(&regex);
//   return false;
// } 

