
// br_util.cpp
// Copyright (c) 2010 by Daniel Y. Kimberg

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

/* This file includes some generic functions to read/write records
 * in the following three brain region database tables:
 * region_name.db
 * synonym.db
 * region_relation.db
 */

using namespace std;

#include "brain_util.h"

/* This function checks whether a certain name exists in a certain namespace
 * of region name db.
 * Returns -1 or -2 for exceptions;
 * returns 0 if it does not exist;
 * returns 1 if it does exist;
 */
int chkRegionName(string dbHome, string rDbName, string rName,
                  string name_space) {
  mydb rDB(dbHome, rDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    rDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      if (rData.getName() == rName && rData.getNameSpace() == name_space) {
        foo = 1;
        break;
      }
    }
  } catch (DbException &e) {
    rDB.getDb().err(e.get_errno(), "Error in chkRegionName()");
    foo = -1;
  } catch (exception &e) {
    rDB.getDb().errx("Error in chkRegionName(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function checks whether a certain synonym string exists in a certain
 * namespace of synonym db. Returns -1 or -2 for exceptions; returns 0 if it
 * does not exist; returns 1 if it does exist;
 */
int chkSynonymStr(string dbHome, string sDbName, string sName,
                  string name_space) {
  mydb sDB(dbHome, sDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    sDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      if (sData.getName() == sName && sData.getNameSpace() == name_space) {
        foo = 1;
        break;
      }
    }
  } catch (DbException &e) {
    sDB.getDb().err(e.get_errno(), "Error in chkSynonymStr()");
    foo = -1;
  } catch (exception &e) {
    sDB.getDb().errx("Error in chkSynonymStr(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function assigns all region names and namespaces from region_name.db
 * to the last two arguments.
 * Returns 0 if database read process is correct.
 * Returns negative values if db read exception shows up. */
int getAllRegions(string dbHome, string rName, vector<string> &nameList,
                  vector<string> &spaceList) {
  mydb regionDB(dbHome, rName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      nameList.push_back(rData.getName());
      spaceList.push_back(rData.getNameSpace());
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in getAllRegions()");
    foo = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in getAllRegions(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function assigns all region names in a certain namespace in
 * region_name.db to the last argument. Returns 0 if database read process is
 * correct. Returns -1 or -2 for db exceptions. */
int getRegions(string dbHome, string rName, string name_space_in,
               vector<string> &nameList) {
  mydb regionDB(dbHome, rName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      if (rData.getNameSpace() == name_space_in)
        nameList.push_back(rData.getName());
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in getRegions()");
    foo = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in getRegions(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function assigns all synonyms and namespaces in synonym.db to
 * the last two arguments.
 * Returns 0 if database read process is correct.
 * Returns negative values if db read exception shows up. */
int getAllSynonyms(string dbHome, string sName, vector<string> &nameList,
                   vector<string> &spaceList) {
  mydb synonymDB(dbHome, sName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      nameList.push_back(sData.getName());
      spaceList.push_back(sData.getNameSpace());
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getAllSynonyms()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getAllSynonyms(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function assigns all synonyms and namespaces in synonym.db to
 * the last two arguments.
 * Returns 0 if database read process is correct.
 * Returns negative values if db read exception shows up. */
int getSynonyms(string dbHome, string sName, string name_space_in,
                vector<string> &nameList) {
  mydb synonymDB(dbHome, sName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      if (name_space_in == sData.getNameSpace())
        nameList.push_back(sData.getName());
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getSynonyms()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getSynonyms(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects a certain region's details from region_name.db
 * and put in regionRec object.
 * Returns -1 or -2 if db read exception is met.
 * Returns 0 if input region name is not found;
 * Returns 1 if input region is found. */
int getRegionRec(string dbHome, string rDbName, string inputName,
                 string name_space, regionRec &outputRec) {
  mydb regionDB(dbHome, rDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int stat = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      if (rData.getName() == inputName && rData.getNameSpace() == name_space) {
        outputRec.deserialize(data.get_data());
        stat = 1;
        break;
      }
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in getRegionRec()");
    stat = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in getRegionRec(): %s", e.what());
    stat = -2;
  }
  cursorp->close();

  return stat;
}

/* This function searches region name db according to relation ID
 * and put the region name and namespace strings into the last twqo arguments.
 * at the end of region name in parenthesis. BY default it is false.
 * Returns -1 or -2 for exceptions;
 * returns 0 if record is not found;
 * returns -3 for any other errors;
 * returns 1 if record is found and variables are assisgned successfully.
 */
int getRegionName(string dbHome, string rDbName, long rID_in, string &rName_out,
                  string &space_out) {
  mydb rDB(dbHome, rDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key(&rID_in, sizeof(rID_in));
  Dbt data;
  long foo = -3;
  try {
    rDB.getDb().cursor(NULL, &cursorp, 0);
    int ret = cursorp->get(&key, &data, DB_SET);
    if (!ret) {
      regionRec rData(data.get_data());
      rName_out = rData.getName();
      space_out = rData.getNameSpace();
      foo = 1;
    } else if (ret == DB_NOTFOUND)
      foo = 0;
  } catch (DbException &e) {
    rDB.getDb().err(e.get_errno(), "Error in getRegionName()");
    foo = -1;
  } catch (exception &e) {
    rDB.getDb().errx("Error in getRegionName(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function searches region name db according to relation name and
 * namespace and returns the region ID. Returns -1 or -2 for exceptions; returns
 * 0 if it is not found;
 */
long getRegionID(string dbHome, string rDbName, string rName,
                 string name_space) {
  mydb rDB(dbHome, rDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  long foo = 0;
  try {
    rDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      if (rData.getName() == rName && rData.getNameSpace() == name_space) {
        foo = rData.getID();
        break;
      }
    }
  } catch (DbException &e) {
    rDB.getDb().err(e.get_errno(), "Error in getRegionID()");
    foo = -1;
  } catch (exception &e) {
    rDB.getDb().errx("Error in getRegionID(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* Seach a synonym name in synonym db file and put primary structure name in
 * pName. Returns -1 or -2 if exceptions are met; returns 0 if parent name is
 * not found; returns 1 if parent name is found. */
int getPrimary(string dbHome, string sDbName, string sName_in,
               string name_space_in, string &pName) {
  mydb symDB(dbHome, sDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int stat = 0;
  int ret;

  try {
    symDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      if (sData.getName() == sName_in &&
          sData.getNameSpace() == name_space_in) {
        pName = sData.getPrimary();
        stat = 1;
        break;
      }
    }
  } catch (DbException &e) {
    symDB.getDb().err(e.get_errno(), "Error in getPname()");
    stat = -1;
  } catch (exception &e) {
    symDB.getDb().errx("Error in getPname(): %s", e.what());
    stat = -2;
  }
  cursorp->close();

  return stat;
}

/* This function collects parent and child region ID information and put in pID
 * and cList. Returns -1 or -2 if exception is met;
 * returns 0 if db file is read without error. */
int getParentChild(string dbHome, string rrDbName, long rID_in, long *pID,
                   vector<long> &cList) {
  mydb regionRelationDB(dbHome, rrDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    regionRelationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if (rrData.getRegion1() == rID_in &&
          rrData.getRelationship() == "child") {
        if (*pID)
          printf("Multiple parent regions found for %ld: %ld and %d\n", rID_in,
                 *pID, rrData.getRegion2());
        else
          *pID = rrData.getRegion2();
      } else if (rrData.getRegion2() == rID_in &&
                 rrData.getRelationship() == "child") {
        long cID = rrData.getRegion1();
        cList.push_back(cID);
      }
    }
  } catch (DbException &e) {
    regionRelationDB.getDb().err(e.get_errno(), "Error in getParentChild()");
    foo = -1;
  } catch (exception &e) {
    regionRelationDB.getDb().errx("Error in getParentChild(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects parent region name information and put in pID.
 * Returns -1 or -2 if exception is met;
 * returns 0 if db file is read successfully. */
int getParent(string dbHome, string rrDbName, long rID_in, long *pID) {
  mydb regionRelationDB(dbHome, rrDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    regionRelationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if (rrData.getRegion1() == rID_in &&
          rrData.getRelationship() == "child") {
        *pID = rrData.getRegion2();
        break;
      }
    }
  } catch (DbException &e) {
    regionRelationDB.getDb().err(e.get_errno(), "Error in getParent()");
    foo = -1;
  } catch (exception &e) {
    regionRelationDB.getDb().errx("Error in getParent(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects child region ID information and put in cList array.
 * Returns -1 or -2 if exception is met;
 * returns 0 if db file is read without error. */
int getChild(string dbHome, string rrDbName, long rID_in, vector<long> &cList) {
  mydb regionRelationDB(dbHome, rrDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    regionRelationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      // skip the region that is same as its parent ("BRAIN")
      if (rrData.getRegion2() == rID_in && rrData.getRegion1() != rID_in &&
          rrData.getRelationship() == "child") {
        long cID = rrData.getRegion1();
        cList.push_back(cID);
      }
    }
  } catch (DbException &e) {
    regionRelationDB.getDb().err(e.get_errno(), "Error in getChild()");
    foo = -1;
  } catch (exception &e) {
    regionRelationDB.getDb().errx("Error in getChild(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects a input region's synonyms and put in a string list
 * argument. Returns -1 or -2 if exceptions are met; returns 0 if db file is
 * read in w/0 error. */
int getSynonym(string dbHome, string sDbName, string rName_in,
               string name_space_in, vector<string> &sList) {
  mydb synonymDB(dbHome, sDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      if (sData.getPrimary() == rName_in &&
          sData.getNameSpace() == name_space_in)
        sList.push_back(sData.getName());
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getSynonym()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getSynonym(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function searches relationship table according to the input r1 ID, r2 ID
 * and relationship string.
 * returns -1 or -2 for exceptions;
 * returns 0 if record is not found;
 * returns the relationship ID otherwise.
 */
long getRelationID(string dbHome, string rrDbName, long r1, long r2,
                   string relStr) {
  mydb relationDB(dbHome, rrDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  long foo = 0;
  try {
    relationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if ((r1 == rrData.getRegion1() && r2 == rrData.getRegion2() &&
           relStr == rrData.getRelationship()) ||
          (relStr == "overlap" && r1 == rrData.getRegion2() &&
           r2 == rrData.getRegion1() && relStr == rrData.getRelationship()))
        foo = rrData.getID();
    }
  } catch (DbException &e) {
    relationDB.getDb().err(e.get_errno(), "Error in getRelation_ui()");
    foo = -1;
  } catch (exception &e) {
    relationDB.getDb().errx("Error in getRelation_ui(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects all non-parent/child relationship records that
 * involves rName_in. It is written to show relationships on "search region"
 * interface, so parent/child are ignored. Return -1 or -2 when exceptions are
 * met; Return 0 if reading w/o error. */
int getRel_ui(string dbHome, string rrDbName, long rID_in, vector<long> &r2List,
              vector<string> &relList) {
  mydb relationDB(dbHome, rrDbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    relationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      long r1 = rrData.getRegion1();
      long r2 = rrData.getRegion2();
      string tmpRel = rrData.getRelationship();
      if (r1 == rID_in && tmpRel != "child") {
        r2List.push_back(r2);
        relList.push_back(trRel_db2ui(tmpRel, true));
      } else if (r2 == rID_in && tmpRel != "child") {
        r2List.push_back(r1);
        relList.push_back(trRel_db2ui(tmpRel, false));
      }
    }
  } catch (DbException &e) {
    relationDB.getDb().err(e.get_errno(), "Error in getRelation_ui()");
    foo = -1;
  } catch (exception &e) {
    relationDB.getDb().errx("Error in getRelation_ui(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects relationship records that are
 * not parent/child between r1 and r2 in relationship db.
 * Return -1 or -2 when exceptions are met;
 * Return 0 if everything is ok. */
int getRel_noChild(string dbHome, string rrDbName, long excludeID, long r1,
                   long r2, vector<string> &relList) {
  mydb relationDB(dbHome, rrDbName, DB_RDONLY);
  string relStr;
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    relationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if (rrData.getID() == excludeID || rrData.getRelationship() == "child")
        continue;
      if (rrData.getRegion1() == r1 && rrData.getRegion2() == r2) {
        relStr = trRel_db2ui(rrData.getRelationship(), true);
        relList.push_back(relStr);
      } else if (rrData.getRegion1() == r2 && rrData.getRegion2() == r1) {
        relStr = trRel_db2ui(rrData.getRelationship(), false);
        relList.push_back(relStr);
      }
    }
  } catch (DbException &e) {
    relationDB.getDb().err(e.get_errno(), "Error in getRelation_all()");
    foo = -1;
  } catch (exception &e) {
    relationDB.getDb().errx("Error in getRelation_all(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects relationship records that are not
 * "child" or "part-of" between r1 and r2 in relationship db.
 * Return -1 or -2 when exceptions are met;
 * Return 0 if everything is ok. */
int getRel_noChildPart(string dbHome, string rrDbName, long r1, long r2,
                       vector<string> &relList) {
  mydb relationDB(dbHome, rrDbName, DB_RDONLY);
  string relStr;
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    relationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if (rrData.getRelationship() == "child" ||
          rrData.getRelationship() == "part-of")
        continue;
      if (rrData.getRegion1() == r1 && rrData.getRegion2() == r2) {
        relStr = trRel_db2ui(rrData.getRelationship(), true);
        relList.push_back(relStr);
      } else if (rrData.getRegion1() == r2 && rrData.getRegion2() == r1) {
        relStr = trRel_db2ui(rrData.getRelationship(), false);
        relList.push_back(relStr);
      }
    }
  } catch (DbException &e) {
    relationDB.getDb().err(e.get_errno(), "Error in getRelation_all()");
    foo = -1;
  } catch (exception &e) {
    relationDB.getDb().errx("Error in getRelation_all(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function collects all relationship records between rName1 and rName2
 * in relationship db. Note the difference between this function and previous
 * one. Return -1 or -2 when exceptions are met;
 * Return 0 if reading w/o error. */
int getRel_all(string dbHome, string rrDbName, long r1, long r2,
               vector<string> &relList) {
  mydb relationDB(dbHome, rrDbName, DB_RDONLY);
  string relStr;
  Dbc *cursorp;
  Dbt key, data;
  int ret;
  int foo = 0;
  try {
    relationDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRelationRec rrData(data.get_data());
      if (rrData.getRegion1() == r1 && rrData.getRegion2() == r2) {
        relStr = trRel_db2ui(rrData.getRelationship(), true);
        relList.push_back(relStr);
      } else if (rrData.getRegion1() == r2 && rrData.getRegion2() == r1) {
        relStr = trRel_db2ui(rrData.getRelationship(), false);
        relList.push_back(relStr);
      }
    }
  } catch (DbException &e) {
    relationDB.getDb().err(e.get_errno(), "Error in getRelation_all()");
    foo = -1;
  } catch (exception &e) {
    relationDB.getDb().errx("Error in getRelation_all(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function "translates" the relationship string in DB file
 * to description string that will be shown on QT interface.
 * The inputFlag specifies whether the relationship should be keep
 * in orginal order or reversed (i.e. "child" to "parent", "part of" to
 * "include".
 */
string trRel_db2ui(string inputStr, bool keepOrder) {
  string newRel;
  if (inputStr == "child") {
    if (keepOrder)
      newRel = "is child of";
    else
      newRel = "is parent of";
  } else if (inputStr == "part-of") {
    if (keepOrder)
      newRel = "is part of";
    else
      newRel = "includes";
  } else if (inputStr == "overlap")
    newRel = "overlaps with";
  else if (inputStr == "equiv")
    newRel = "is equiv to";

  return newRel;
}

/* This function is written specially for command line search.
 * It searches the region name db using an input string.
 * If the region name contains that string, the region name and its namespace
 * will be put into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findRegionNames(string dbHome, string dbName, string key_in, bool ik_flag,
                    vector<regionRec> &outputList) {
  string keyStr = key_in;
  if (ik_flag) keyStr = toLowerCase(key_in);

  mydb regionDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      string rName = rData.getName();
      if (ik_flag) rName = toLowerCase(rData.getName());
      string rNS = rData.getNameSpace();
      if (rName.find(keyStr) != string::npos) outputList.push_back(rData);
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in findRegionNames()");
    foo = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in findRegionNames(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function is written specially for command line search.
 * It searches the region name db using keyStr and nsStr.
 * If the region name contains keyStr and nsStr EQUALS nsStr,
 * the region name will be put into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findRegionNames(string dbHome, string dbName, string key_in, bool ik_flag,
                    string ns_in, bool ins_flag,
                    vector<regionRec> &outputList) {
  string keyStr = key_in;
  if (ik_flag) keyStr = toLowerCase(key_in);
  string nsStr = ns_in;
  if (ins_flag) keyStr = toLowerCase(ns_in);

  mydb regionDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      string rName = rData.getName();
      if (ik_flag) rName = toLowerCase(rData.getName());
      string nsName = rData.getNameSpace();
      if (ins_flag) nsName = toLowerCase(rData.getNameSpace());
      if (rName.find(keyStr) != string::npos && nsStr == nsName)
        outputList.push_back(rData);
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in findRegionNames()");
    foo = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in findRegionNames(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function is written specially for command line search.
 * It searches the region names that belong to a certain namespace.
 * If the region name contains that string, the region name and its namespace
 * will be put into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findRegionNS(string dbHome, string dbName, string ns_in, bool ins_flag,
                 vector<regionRec> &outputList) {
  string nsStr = ns_in;
  if (ins_flag) nsStr = toLowerCase(ns_in);

  mydb regionDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    regionDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      regionRec rData(data.get_data());
      string nsName = rData.getNameSpace();
      if (ins_flag) nsName = toLowerCase(rData.getNameSpace());
      if (nsName == nsStr) outputList.push_back(rData);
    }
  } catch (DbException &e) {
    regionDB.getDb().err(e.get_errno(), "Error in findRegionNames()");
    foo = -1;
  } catch (exception &e) {
    regionDB.getDb().errx("Error in findRegionNames(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function is written specially for command line search.
 * It searches the synonym db using an input string.
 * If the synonym contains that string, the synonym, region name
 * and its namespace will be put into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findSynonyms(string dbHome, string dbName, string key_in, bool ik_flag,
                 vector<synonymRec> &outputList) {
  string keyStr = key_in;
  if (ik_flag) keyStr = toLowerCase(key_in);

  mydb synonymDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      string sName = sData.getName();
      if (ik_flag) sName = toLowerCase(sData.getName());
      if (sName.find(keyStr) != string::npos) outputList.push_back(sData);
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getAllSynonyms()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getAllSynonyms(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function is written specially for command line search.
 * It searches the synonym db using keyStr and nsStr.
 * If the region name contains keyStr and nsStr EQUALS nsStr,
 * the region name will be put into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findSynonyms(string dbHome, string dbName, string key_in, bool ik_flag,
                 string ns_in, bool ins_flag, vector<synonymRec> &outputList) {
  string keyStr = key_in;
  if (ik_flag) keyStr = toLowerCase(key_in);
  string nsStr = ns_in;
  if (ins_flag) nsStr = toLowerCase(ns_in);

  mydb synonymDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      string sName = sData.getName();
      if (ik_flag) sName = toLowerCase(sData.getName());
      string nsName = sData.getNameSpace();
      if (ins_flag) nsName = toLowerCase(sData.getNameSpace());
      if (sName.find(keyStr) != string::npos && nsStr == nsName)
        outputList.push_back(sData);
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getAllSynonyms()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getAllSynonyms(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function is written specially for command line search.
 * It searches synonym records that belong to a certain namespace and
 * put them into the string vector.
 * Returns 0 if everything is ok, non-zero otherwise.
 */
int findSynmNS(string dbHome, string dbName, string ns_in, bool ins_flag,
               vector<synonymRec> &outputList) {
  string nsStr = ns_in;
  if (ins_flag) nsStr = toLowerCase(ns_in);

  mydb synonymDB(dbHome, dbName, DB_RDONLY);
  Dbc *cursorp;
  Dbt key, data;
  int ret;

  int foo = 0;
  try {
    synonymDB.getDb().cursor(NULL, &cursorp, 0);
    while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0) {
      synonymRec sData(data.get_data());
      string nsName = sData.getNameSpace();
      if (ins_flag) nsName = toLowerCase(sData.getNameSpace());
      if (nsStr == nsName) outputList.push_back(sData);
    }
  } catch (DbException &e) {
    synonymDB.getDb().err(e.get_errno(), "Error in getAllSynonyms()");
    foo = -1;
  } catch (exception &e) {
    synonymDB.getDb().errx("Error in getAllSynonyms(): %s", e.what());
    foo = -2;
  }
  cursorp->close();

  return foo;
}

/* This function converts each letter in an input string into lower case and
 * returns the new string. */
string toLowerCase(string inputStr) {
  string newStr = inputStr;
  for (uint i = 0; i < inputStr.size(); i++) newStr[i] = tolower(inputStr[i]);

  return newStr;
}
