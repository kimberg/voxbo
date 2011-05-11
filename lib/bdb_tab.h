
// bdb_tab.h
// classes used to hold records in Berkeley DB tables
// Copyright (c) 1998-2010 by The VoxBo Development Team

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

// DYK: due to some crazy organization, don't include this file
// directly, include mydefs.h.

#ifndef BDB_TAB_H
#define BDB_TAB_H

using namespace std;

#include <string>
#include "typedefs.h"
#include <gnutls/gnutls.h>
#include "vbutil.h"

// System class to hold system-wide information
class sysRec
{
 public:
  sysRec();
  sysRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;
  
  void setName(const string& inputStr) { name = inputStr; }
  void setValue(const string& inputStr) { value = inputStr; }
  string getName() const { return name; }
  string getValue() const { return value; }

 private:
  string name, value;
};

// View record class
class viewRec
{
 public:
  viewRec();
  viewRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setName(const string& inputStr) { name = inputStr; }
  void setID(int32 inputVal) { ID = inputVal; }
  string getName() const { return name; }
  int32 getID() const { return ID; }
  
 private:
  int32 ID;
  string name;
};

// View entry record class
class viewEntryRec
{
 public:
  viewEntryRec();
  viewEntryRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;
  
  void setViewID(int32 inputVal) { viewID = inputVal; }
  void setScorename(const string& inputStr) { scorename = inputStr; }
  void setFlags(const string& inputStr) { flags = inputStr; }
  int32 getViewID() const { return viewID; }
  string getScorename() const { return scorename; }
  string getFlags() const { return flags; }

 private:
  int32 viewID;
  string scorename, flags;
};

// User record class
class userRec
{
 public:
  userRec();
  userRec(void*);
  ~userRec();
  void init();
  void deserialize(void*);
  void clear();
  void serialize(char* ) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; } 
  void setAccount(const string& inputStr) { account = inputStr; }
  // void setSalt(gnutls_datum_t char* inputStr) { salt = inputStr; }
  // void setVeriSize(uint32 inputVal) { veriSize = inputVal; }
  // void setVerifier(unsigned char* inputStr) { verifier = inputStr; }
  void setName(const string& inputStr) { name = inputStr; }
  void setPhone(const string& inputStr) { phone = inputStr; }
  void setEmail(const string& inputStr) { email = inputStr; }
  void setAddress(const string& inputStr) { address = inputStr; }
  void setGroups(const string& inputStr) { groups = inputStr; }
  void setMisc(const string& inputStr) { misc = inputStr; }

  int32 getID() const { return ID; }
  string getAccount() const { return account; }
  gnutls_datum_t &getSalt() { return salt; }
  //uint32 getVeriSize() const { return veriSize; }
  gnutls_datum_t &getVerifier() { return verifier; }
  string getName() const { return name; }
  string getPhone() const { return phone; }
  string getEmail() const { return email; }
  string getAddress() const { return address; }
  string getGroups() const { return groups; }  
  string getMisc() const { return misc; }
  int gen_salt_and_verifier(const string &password);
 private:
  int32 ID;
  string account;
  gnutls_datum_t salt;
  gnutls_datum_t verifier;
  string name, phone, email, address, groups, misc;
};

// User relationship record class
class userRelRec
{
 public:
  userRelRec();
  userRelRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setUserID(int32 inputVal) { userID = inputVal; }
  void setRelationship(const string& inputStr) { relationship = inputStr; }
  void setOtherID(int32 inputVal) { otherID = inputVal; }

  int32 getID() const { return ID; }
  int32 getUserID() const { return userID; }
  string getRelationship() const { return relationship; }
  int32 getOtherID() const { return otherID; }

 private:
  int32 ID, userID;
  string relationship;
  int32 otherID;
};

// User group record class
class userGrpRec
{
 public:
  userGrpRec();
  userGrpRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; } 
  void setName(const string& inputStr) { name = inputStr; }
  void setDesc(const string& inputStr) { desc = inputStr; }
  void setOwner(int32 inputVal) { owner = inputVal; } 

  int32 getID() const { return ID; }
  string getName() const { return name; }
  string getDesc() const { return desc; }  
  int32 getOwner() const { return owner; }

 private:
  int32 ID;
  string name, desc;
  int32 owner;
};

// User permission record class
class permRec
{
 public:
  permRec();
  permRec(void*);

  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setAccessID(const string& inputStr) { accessID = inputStr; }
  void setDataID(const string& inputStr) { dataID = inputStr; }
  void setPermission(const string& inputStr) { permission = inputStr; }
  string getAccessID() const { return accessID; }
  string getDataID() const { return dataID; }
  string getPermission() const { return permission; }

 private:
  string accessID, dataID;
  string permission;
};

// Contact record class
class contactRec
{
 public:
  contactRec();
  contactRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setUserID(int32 inputVal) { userID = inputVal; }
  void setPatientID(int32 inputVal) { patientID = inputVal; }
  void setContactDate(int32 inputVal) { contactDate = inputVal; }
  void setRefID(int32 inputVal) { refID = inputVal; }
  void setNotes(const string& inputStr) { notes = inputStr; }
  void setAddDate(int32 inputVal) { addDate = inputVal; }
  void setDelFlag(uint8 inputFlag) { delFlag = inputFlag; }

  int32 getID() const { return ID; }
  int32 getUserID() const { return userID; }
  int32 getPatientID() const { return patientID; }
  int32 getContactDate() const { return contactDate; }
  int32 getRefID() const { return refID; }
  string getNotes() const { return notes; }
  int32 getAddDate() const { return addDate; }
  uint8 getDelFlag() const { return delFlag; }

 private:
  int32 ID, userID, patientID;
  int32 contactDate, addDate;
  int32 refID;
  string notes;
  uint8 delFlag;
};

// Study record class
class studyRec
{
 public:
  studyRec();
  studyRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setName(const string& inputStr) { name = inputStr; }
  void setPI(const string& inputStr) { PI = inputStr; }
  int32 getID() const { return ID; }
  string getName() const { return name; }
  string getPI() const { return PI; }

 private:
  int32 ID;
  string name, PI;
};

// Patient record class
class patientRec
{
 public:
  patientRec();
  patientRec(void*);
  void serialize(char*) const;
  int32 getSize() const;
  void clear();
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setPrivate(uint8 inputFlag) { privateFlag = inputFlag; }
  int32 getID() const { return ID; }
  uint8 getPrivate() const { return privateFlag; }

 private:
  int32 ID;
  uint8 privateFlag;
};

// Patient group record class
class pgrpRec
{
 public:
  pgrpRec();
  pgrpRec(void*);

  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setDesc(const string& inputStr) { desc = inputStr; }
  int32 getID() const { return ID; }
  string getDesc() const { return desc; }

 private:
  int32 ID;
  string desc;
};

// Patient group member record class
class pgrpMemberRec
{
 public:
  pgrpMemberRec();
  pgrpMemberRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;
  
  void setPatientID(int32 inputVal) { patientID = inputVal; }
  void setGroupID(int32 inputVal) { groupID = inputVal; }
  int32 getPatientID() const { return patientID; }
  int32 getGroupID() const { return groupID; }

 private:
  int32 patientID, groupID;
};

// Patient list record class
class pListRec
{
 public:
  pListRec();
  pListRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setOwnerID(int32 inputVal) { ownerID = inputVal; }
  void setName(const string& inputStr) { name = inputStr; }
  void setRunDate(int32 inputVal) { runDate = inputVal; }
  void setDirtyFlag(uint8 inputFlag) { dirtyFlag = inputFlag; }

  int32 getID() const { return ID; }
  int32 getOwnerID() const { return ownerID; }
  string getName() const { return name; }
  int32 getRunDate() const { return runDate; }
  uint8 getDirtyFLag() const { return dirtyFlag; }

 private:
  int32 ID, ownerID;
  string name;
  int32 runDate;
  uint8 dirtyFlag;
};

// Patient list members record class
class pListMemberRec
{
 public:
  pListMemberRec();
  pListMemberRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setListID(int32 inputVal) { listID = inputVal; }
  void setPatientID(int32 inputVal) { patientID = inputVal; }
  int32 getListID() const { return listID; }
  int32 getPatientID() const { return patientID; }

 private:
  int32 listID, patientID;
};


#endif
