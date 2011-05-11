
// bdb_tab.cpp
// member functions for classes that hold records in Berkeley DB tables.
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

#include "mydefs.h"
#include "db_util.h"
#include <string>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>



/********************************************
 * Functions in system class 
 ********************************************/
// Default constructor
sysRec::sysRec() 
{ 

}

// Constructor that parses input buffer and assigns data members
sysRec::sysRec(void* inputBuf) 
{
  int bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  name = c_buf_in + bufLen;
  bufLen = name.size() + 1;

  value = c_buf_in + bufLen;
  bufLen += value.size() + 1;
}

// Initialize data members
void sysRec::clear() 
{
  name = value = "";
}

// Set a single contiguous memory location that holds values of data members
void sysRec::serialize(char* outBuf) const
{
  int32 strLen = name.size() + 1;
  int32 bufLen = 0;
  memcpy(outBuf + bufLen, name.c_str(), strLen);
  bufLen += strLen;
  
  strLen = value.size() + 1;
  memcpy(outBuf + bufLen, value.c_str(), strLen);
  bufLen += strLen;
}

// Return size of buffer
int32 sysRec::getSize() const 
{ 
  int32 bufLen = name.size() + 1 + value.size() + 1;
  return bufLen; 
}

// Print out data members
void sysRec::show() const 
{
  printf("%s: %s\n", name.c_str(), value.c_str());
}
  
/********************************************
 * Functions in viewRec class 
 ********************************************/
// Default constructor
viewRec::viewRec() : ID(0)
{ 

}

// Constructor that parses input buffer and assigns data members
viewRec::viewRec(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  name = c_buf_in + bufLen;
  bufLen = name.size() + 1;
}

// Initialize data members
void viewRec::clear() 
{
  ID = 0;
  name = "";
}

// Set a single contiguous memory location that holds values of data members
void viewRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);

  int32 strLen = name.size() + 1;
  memcpy(outBuf + bufLen, name.c_str(), strLen);
  bufLen += strLen;
}

// Return the size of the buffer
int32 viewRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += name.size() + 1;
  return bufLen; 
}

// Print out data members
void viewRec::show() const 
{
  printf("View name: %s\n", name.c_str());
  printf("View ID: %d\n", ID);
}
  
/********************************************
 * Functions in viewEntryRec class 
 ********************************************/
// Default constructor
viewEntryRec::viewEntryRec() : viewID(0)
{ 

}

// Constructor that parses input buffer and assigns data members
viewEntryRec::viewEntryRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  viewID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&viewID);
  bufLen += sizeof(int32);

  scorename = c_buf_in + bufLen;
  bufLen += scorename.size() + 1;

  flags = c_buf_in + bufLen;
  bufLen += flags.size() + 1;
}

// Initialize data members
void viewEntryRec::clear() 
{
  viewID = 0;
  scorename = flags = "";
}

// Set a single contiguous memory location that holds values of data members
void viewEntryRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = viewID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, scorename.c_str(), scorename.size() + 1);
  bufLen += scorename.size() + 1;

  memcpy(outBuf + bufLen, flags.c_str(), flags.size() + 1);
  bufLen += flags.size() + 1;
}

// Return the size of the buffer
int32 viewEntryRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += scorename.size() + 1;
  bufLen += flags.size() + 1;

  return bufLen; 
}

// Print out data members
void viewEntryRec::show() const 
{
  printf("View ID: %d\n", viewID);
  printf("Score name: %s\n", scorename.c_str());
  printf("Flags: %s\n", flags.c_str());
}

/********************************************
 * Functions in user record class 
 ********************************************/
// Default constructor
userRec::userRec()
{
  init();
}

// Constructor that parses input buffer to set data members
userRec::userRec(void* inputBuf)
{
  init();
  deserialize(inputBuf);
}

userRec::~userRec()
{
  if (verifier.size) gnutls_free(verifier.data);
  if (salt.size) gnutls_free(salt.data);
}

void
userRec::init()
{
  ID=0;
  verifier.size=0;
  salt.size=0;
}

// Set data members based on input buffer
void userRec::deserialize(void*  inputBuf) 
{
  int32 offset = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&ID);
  offset += sizeof(int32);

  account = c_buf_in + offset;
  offset += account.size() + 1;
    
  salt.size=4;
  salt.data=(unsigned char *)gnutls_malloc(4);
  if (salt.data)
    memcpy(salt.data,c_buf_in + offset,4);
  else
    salt.size=0;
  offset += 4;

  verifier.size = *((uint32 *) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&verifier.size);
  offset += sizeof(uint32);

  verifier.data=(unsigned char *)gnutls_malloc(verifier.size);
  if (verifier.data)
    memcpy(verifier.data,c_buf_in + offset,verifier.size);
  else
    verifier.size=0;
  offset += verifier.size;

  name = c_buf_in + offset;
  offset += name.size() + 1;

  phone = c_buf_in + offset;
  offset += phone.size() + 1;

  email = c_buf_in + offset;
  offset += email.size() + 1;

  address = c_buf_in + offset;
  offset += address.size() + 1;

  groups = c_buf_in + offset;
  offset += groups.size() + 1;

  misc = c_buf_in + offset;
  offset += misc.size() + 1;
}

// Initialize data members
void userRec::clear() 
{
  ID = 0;
  account = "";
  name = phone = email = address = groups = misc = "";
  if (verifier.size) gnutls_free(verifier.data);
  if (salt.size) gnutls_free(salt.data);
  verifier.size=0;
  salt.size=0;
}

// Set combined data members into a single contiguous memory location
void userRec::serialize(char* outBuf) const
{
  int32 offset = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  memcpy(outBuf + offset, account.c_str(), account.size() + 1);
  offset += account.size() + 1;

  memcpy(outBuf + offset, salt.data, 4);
  offset += 4;

  uint32 bar = verifier.size;
  if (ntohs(1) == 1)
    swap(&bar);
  memcpy(outBuf + offset, &bar, sizeof(uint32));
  offset += sizeof(uint32);

  memcpy(outBuf + offset, verifier.data, verifier.size);
  offset += verifier.size;

  memcpy(outBuf + offset, name.c_str(), name.size() + 1);
  offset += name.size() + 1;

  memcpy(outBuf + offset, phone.c_str(), phone.size() + 1);
  offset += phone.size() + 1;

  memcpy(outBuf + offset, email.c_str(), email.size() + 1);
  offset += email.size() + 1;

  memcpy(outBuf + offset, address.c_str(), address.size() + 1);
  offset += address.size() + 1;

  memcpy(outBuf + offset, groups.c_str(), groups.size() + 1);
  offset += groups.size() + 1;

  memcpy(outBuf + offset, misc.c_str(), misc.size() + 1);
  offset += misc.size() + 1;
}

// Returns the size of the buffer
int32 userRec::getSize() const 
{ 
  int32 bufSize = 0;
  bufSize += sizeof(int32);
  bufSize += account.size() + 1;
  bufSize += 4;  // salt
  bufSize += sizeof(uint32);  // verifier size
  bufSize += verifier.size;
  bufSize += name.size() + 1;
  bufSize += phone.size() + 1;
  bufSize += email.size() + 1;
  bufSize += address.size() + 1;
  bufSize += groups.size() + 1;
  bufSize += misc.size() + 1;

  return bufSize; 
}

// Show contents of data members
void userRec::show() const {
  printf("User ID: %d\n", ID);
  printf("Account Name: %s\n", account.c_str());
  printf("Real Name: %s\n", name.c_str());
  printf("Phone #: %s\n", phone.c_str());
  printf("Email: %s\n", email.c_str());
  printf("Address: %s\n", address.c_str());
  printf("Groups: %s\n", groups.c_str());
  printf("Misc: %s\n", misc.c_str());
}

int
userRec::gen_salt_and_verifier(const string &password)
{
  return make_salt_verifier(account,password,salt,verifier);
}

/***********************************************
 * Functions in user relationship record class 
 ***********************************************/
// Default constructor
userRelRec::userRelRec() : ID(0), userID(0), otherID(0)
{ 

}

// Constructor that parses input buffer to set data members
userRelRec::userRelRec(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  userID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&userID);
  bufLen += sizeof(int32);

  relationship = c_buf_in + bufLen;
  bufLen += relationship.size() + 1;

  otherID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&otherID);
  bufLen += sizeof(int32);
}

// Initialize data members
void userRelRec::clear() 
{
  ID = userID = 0;
  relationship = "";
  otherID = 0;
}

// Set combined data members into a single contiguous memory location
void userRelRec::serialize(char* outBuf) const
{
  int32 bufLen = 0, varLen = sizeof(int32);
  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  foo = userID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  int32 strLen = relationship.size() + 1;
  memcpy(outBuf + bufLen, relationship.c_str(), strLen);
  bufLen += strLen;

  foo = otherID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;
}

// Returns the size of the buffer
int32 userRelRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += relationship.size() + 1;
  bufLen += sizeof(int32);

  return bufLen; 
}

// Show the contents of data members
void userRelRec::show() const {
  printf("Relationship ID: %d\n", ID);
  printf("User ID: %d\n", userID);
  printf("Relationship: %s\n", relationship.c_str());
  printf("OtherID: %d\n", otherID);
}

/***********************************************
 * Functions in user group record class 
 ***********************************************/
// Default constructor
userGrpRec::userGrpRec() : ID(0), owner(0)
{ 
 
}

// Constructor that parses an input buffer to assign values to class members
userGrpRec::userGrpRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  desc = c_buf_in + bufLen;
  bufLen += desc.size() + 1;

  owner = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&owner);
  bufLen += sizeof(int32);
}

// Initialize data members
void userGrpRec::clear() 
{
  ID = 0;
  name = desc = "";
  owner = 0;
}

// Set a single contiguous memory location that stores data members
void userGrpRec::serialize(char* outBuf) const
{
  int32 bufLen = 0, varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;

  memcpy(outBuf + bufLen, desc.c_str(), desc.size() + 1);
  bufLen += desc.size() + 1;

  foo = owner;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;
}

// Return the size of the buffer
int32 userGrpRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += name.size() + 1;
  bufLen += desc.size() + 1;
  bufLen += sizeof(int32);
  
  return bufLen; 
}

// Show the contents data members
void userGrpRec::show() const {
  printf("Group ID: %d\n", ID);
  printf("Group Name: %s\n", name.c_str());
  printf("Desc: %s\n", desc.c_str());
  printf("Owner: %d\n", owner);
}

/********************************************
 * Functions in user permission record class 
 ********************************************/
// Default constructor
permRec::permRec()
{ 
  // no need to call clear() since default ctor of string will be called
}

// Constructor that parses an input buffer to assign data members
permRec::permRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  accessID = c_buf_in + bufLen;
  bufLen += accessID.size() + 1;

  dataID = c_buf_in + bufLen;
  bufLen += dataID.size() + 1;

  permission = c_buf_in + bufLen;
  bufLen += permission.size() + 1;
}

// Initialize data members
void permRec::clear() 
{
  accessID = dataID = "";
  permission = "";
}

// Set an input buffer that holds data members as a single contiguous memory location
void permRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;

  memcpy(outBuf + bufLen, accessID.c_str(), accessID.size() + 1);
  bufLen += accessID.size() + 1;

  memcpy(outBuf + bufLen, dataID.c_str(), dataID.size() + 1);
  bufLen += dataID.size() + 1;

  memcpy(outBuf + bufLen, permission.c_str(), permission.size() + 1);
  bufLen += permission.size() + 1;
}

// Return the size of the buffer
int32 permRec::getSize() const 
{ 
  int32 buflen = 0;
  buflen += accessID.size() + 1;
  buflen += dataID.size() + 1;
  buflen += permission.size() + 1;

  return buflen;
}

// Show contents of date members
void permRec::show() const 
{
  printf("Accessor ID: %s\n", accessID.c_str());
  printf("Data ID: %s\n", dataID.c_str());
  printf("Permission: %s\n", permission.c_str());
}

/********************************************
 * Functions in viewEntryRec class 
 ********************************************/
// Default constructor
contactRec::contactRec() 
  : ID(0), userID(0), patientID(0), 
    contactDate(0), addDate(0),
    refID(0), delFlag(0)
{ 
  // no need to call clear()
}

// Constructor that parses input buffer and assigns data members
contactRec::contactRec(void* inputBuf)
{
  int32 varLen = sizeof(int32);
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += varLen;

  userID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&userID);
  bufLen += varLen;

  patientID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&patientID);
  bufLen += varLen;

  contactDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&contactDate);
  bufLen += varLen;

  refID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&refID);
  bufLen += varLen;

  notes = c_buf_in + bufLen;
  bufLen += notes.size() + 1;
  
  addDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&addDate);
  bufLen += varLen;

  delFlag = *((uint8 *) (c_buf_in + bufLen));
  bufLen += sizeof(uint8);
}  

// Initialize data members
void contactRec::clear() 
{
  ID = userID = patientID = 0;
  contactDate = addDate = 0;
  refID = 0;
  notes = "";
  delFlag = 0;
}

// Set a single contiguous memory location to hold values of data members
void contactRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += varLen;

  foo = userID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += varLen;

  foo = patientID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += varLen;

  foo = contactDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += varLen;


  foo = refID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += varLen;

  memcpy(outBuf + bufLen, notes.c_str(), notes.size() + 1);
  bufLen += notes.size() + 1;

  foo = addDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += sizeof(int32);

  memcpy(outBuf + bufLen, &delFlag, sizeof(uint8));
  bufLen += sizeof(uint8);
}

// Return size of buffer
int32 contactRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += notes.size() + 1;
  bufLen += sizeof(int32);
  bufLen += sizeof(uint8);

  return bufLen; 
}   

// Print out all data members
void contactRec::show() const {
  printf("Contact ID: %d\n", ID);
  printf("User ID: %d\n", userID);
  printf("Patient ID: %d\n", patientID);

  if (contactDate) {
    time_t tmpTime = contactDate;
    printf("Contact Date: %s\n", ctime(&tmpTime));
  }
  else
    printf("Contact Date: NA\n");
  
  printf("Ref ID: %d\n", refID);
  printf("Notes: %s\n", notes.c_str());
  
  if (addDate) {
    time_t tmpTime = addDate;
    printf("Added Date: %s\n", ctime(&tmpTime));
  }
  else
    printf("Added Date: NA\n");
  
  if (delFlag)
    printf("Deleted? Yes\n");
  else
    printf("Deleted? No\n");
}

/********************************************
 * Functions in studyRec class 
 ********************************************/
// Default constructor
studyRec::studyRec() : ID(0)
{ 

}

// Constructor that parses input buffer and assigns data members
studyRec::studyRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);
 
  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  PI = c_buf_in + bufLen;
  bufLen += PI.size() + 1;
}
  
// Initialize data members
void studyRec::clear() 
{
  ID = 0;
  name = PI = "";
}

// Set a single contiguous memory location that holds values of data members
void studyRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);

  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;

  memcpy(outBuf + bufLen, PI.c_str(), PI.size() + 1);
  bufLen += PI.size() + 1;
}

// Return the size of the buffer
int32 studyRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += name.size() + 1;
  bufLen += PI.size() + 1;
  return bufLen; 
}

// Print out data members
void studyRec::show() const 
{
  printf("Study ID: #%d\n", ID);
  printf("Study Name: %s\n", name.c_str());
  printf("Study PI: %s\n", PI.c_str());
}

/********************************************
 * Functions in sessionRec class 
 ********************************************/
// Default constructor
// sessionRec::sessionRec() : ID(0), patientID(0), studyID(0), date(0), pubFlag(0)
// { 

// }

// // Constructor that parses input buffer and assigns data members
// sessionRec::sessionRec(void* inputBuf)
// {
//   int32 offset = 0;
//   char* c_buf_in = (char*) inputBuf;

//   ID = *((int32 *) (c_buf_in + offset));
//   if (ntohs(1) == 1)
//     swap(&ID);
//   offset = sizeof(int32);

//   patientID = *((int32 *) (c_buf_in + offset));
//   if (ntohs(1) == 1)
//     swap(&patientID);
//   offset += sizeof(int32);

//   studyID = *((int32 *) (c_buf_in + offset));
//   if (ntohs(1) == 1)
//     swap(&studyID);
//   offset += sizeof(int32);

//   date = *((int32 *) (c_buf_in + offset));
//   if (ntohs(1) == 1)
//     swap(&date);
//   offset += sizeof(int32);

//   examiner = c_buf_in + offset;
//   offset += examiner.size() + 1;

//   pubFlag = *((uint8 *) (c_buf_in + offset));
//   offset += sizeof(uint8);

//   notes = c_buf_in + offset;
//   offset += notes.size() + 1;
// }

// // Initialize data members
// void sessionRec::clear() 
// {
//   ID = patientID = studyID = 0;
//   date = 0;
//   examiner = "";
//   pubFlag = 0;
//   notes = "";
// }

// // Set a single contiguous memory location that holds values of data members
// void sessionRec::serialize(char* outBuf) const
// {
//   int32 offset = 0;

//   int32 foo = ID;
//   if (ntohs(1) == 1)
//     swap(&foo);
//   memcpy(outBuf + offset, &foo, sizeof(int32));
//   offset += sizeof(int32);

//   foo = patientID;
//   if (ntohs(1) == 1)
//     swap(&foo);
//   memcpy(outBuf + offset, &foo, sizeof(int32));
//   offset += sizeof(int32);

//   foo = studyID;
//   if (ntohs(1) == 1)
//     swap(&foo);
//   memcpy(outBuf + offset, &foo, sizeof(int32));
//   offset += sizeof(int32);

//   foo = date;
//   if (ntohs(1) == 1)
//     swap(&foo);
//   memcpy(outBuf + offset, &foo, sizeof(int32));
//   offset += sizeof(int32);

//   memcpy(outBuf + offset, examiner.c_str(), examiner.size() + 1);
//   offset += examiner.size() + 1;

//   memcpy(outBuf + offset, &pubFlag, sizeof(uint8));
//   offset += sizeof(uint8);

//   memcpy(outBuf + offset, notes.c_str(), notes.size() + 1);
//   offset += notes.size() + 1;
// }

// // Return the size of the buffer
// int32 sessionRec::getSize() const 
// { 
//   int32 bufLen = 0;
//   bufLen += sizeof(int32);
//   bufLen += sizeof(int32);
//   bufLen += sizeof(int32);
//   bufLen += sizeof(int32);
//   bufLen += examiner.size() + 1;
//   bufLen += sizeof(uint8);
//   bufLen += notes.size() + 1;

//   return bufLen; 
// }

// // Print out data members
// void sessionRec::show() const 
// {
//   printf("Session ID: %d\n", ID);
//   printf("Patient ID: %d\n", patientID);
//   printf("Study ID: %d\n", studyID);

//   if (date) {
//     time_t tmpTime = date;
//     printf("Date: %s", ctime(&tmpTime));
//   }
//   else
//     printf("Date: NA\n");

//   printf("Examiner: %s\n", examiner.c_str());
//   if (pubFlag)
//     printf("Session is public? true\n");
//   else
//     printf("Session is public? false\n");
//   printf("Session notes: %s\n", notes.c_str());
// }

/*************************************
 * Functions in patient record class 
 *************************************/
// Default constructor
patientRec::patientRec() : ID(0), privateFlag(0) 
{ 

}

// Constructor that accepts an input buffer 
patientRec::patientRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);
  
  privateFlag = *((uint8 *) (c_buf_in + bufLen));
  bufLen += sizeof(uint8);
}

// Initialize data members
void patientRec::clear() {
  ID = 0;
  privateFlag = 0;
}

// Serialize class members into the input buffer
void patientRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);

  memcpy(outBuf + bufLen, &privateFlag, sizeof(uint8));
  bufLen += sizeof(uint8);
}

// Return size of the buffer that will hold this structure
int32 patientRec::getSize() const 
{ 
  int32 buflen = sizeof(int32) + sizeof(uint8);

  return buflen;
}   

// Show contents of date members
void patientRec::show() const 
{
  if (privateFlag)
    printf("Patient #%d: private\n", ID);
  else
    printf("Patient #%d: public\n", ID);
}
  
/*************************************
 * Functions in patient group class 
 *************************************/
// Default constructor
pgrpRec::pgrpRec() : ID(0)
{ 

}

// Constructor that parses an input buffer to assign values to class members
pgrpRec::pgrpRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);
 
  desc = c_buf_in + bufLen;
  bufLen += desc.size() + 1;
}

// Initialize data members
void pgrpRec::clear() 
{
  ID = 0;
  desc = "";
}

// Set a single contiguous memory location that stores data members
void pgrpRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);
  
  int32 strLen = desc.size() + 1;
  memcpy(outBuf + bufLen, desc.c_str(), strLen);
  bufLen += strLen;
}

// Return the size of the buffer
int32 pgrpRec::getSize() const 
{ 
  int32 buflen = sizeof(int32) + desc.size() + 1;
  return buflen;
}

// Show the contents of date members
void pgrpRec::show() const 
{
  printf("Group #%d: %s\n", ID, desc.c_str());
}
  
/********************************************
 * Functions in patient group member class 
 ********************************************/
// Default constructor
pgrpMemberRec::pgrpMemberRec() : patientID(0), groupID(0)
{ 

}

// Constructor that parses an input buffer to assign values to class members
pgrpMemberRec::pgrpMemberRec(void* inputBuf)
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
    
  patientID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&patientID);
  bufLen += sizeof(int32);
    
  groupID = * ((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&groupID);
  bufLen += sizeof(int32);
}

// Initialize data members
void pgrpMemberRec::clear() 
{
  patientID = groupID = 0;
}

// Save data members into a single contiguous memory location
void pgrpMemberRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;;
  
  int32 foo = patientID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);
  
  foo = groupID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);
}

// Return size of buffer
int32 pgrpMemberRec::getSize() const 
{ 
  return sizeof(int32) * 2; 
}

// Print out data members
void pgrpMemberRec::show() const 
{
  printf("Patient #%d beint32s to group #%d\n", patientID, groupID);
}
  
/*******************************************
 * Functions in patient list record class
 *******************************************/
// Default constructor
pListRec::pListRec() : ID(0), ownerID(0), runDate(0), dirtyFlag(0)
{ 

}

// COnstructor that parses an input buffer and assigns member variables
pListRec::pListRec(void* inputBuf)
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);
  char* c_buf_in = (char*) inputBuf;

  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  ownerID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ownerID);
  bufLen += sizeof(int32);

  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  runDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&runDate);
  bufLen += varLen;
    
  dirtyFlag = *((uint8 *) (c_buf_in + bufLen));
  bufLen += sizeof(uint8);
}

// Initialize data members
void pListRec::clear() {
  ID = ownerID = 0;
  name = "";
  runDate = 0;
  dirtyFlag = 0;
}

// Serialize data members into a single contiguous memory location
void pListRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  foo = ownerID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  int32 strLen = name.size() + 1;
  memcpy(outBuf + bufLen, name.c_str(), strLen);
  bufLen += strLen;

  foo = runDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, &dirtyFlag, sizeof(uint8));
  bufLen += sizeof(uint8);
}

// Return the size of the buffer. */
int32 pListRec::getSize() const 
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += name.size() + 1;
  bufLen += sizeof(int32);
  bufLen += sizeof(uint8);

  return bufLen; 
}   

// Print out data members
void pListRec::show() const {
  printf("Patient List ID: %d\n", ID);
  printf("Owner ID: %d\n", ownerID);
  printf("Name: %s\n", name.c_str());

  if (runDate) {
    time_t tmpTime = runDate;
    printf("Last run date: %s", ctime(&tmpTime));
  }
  else
    printf("Region added on: NA\n");

  if (dirtyFlag)
    printf("List modified after search? true");
  else
    printf("List modified after search? false");
}

/***********************************************
 * Functions in patient list member record class
 ***********************************************/
// Default constructor
pListMemberRec::pListMemberRec() : listID(0), patientID(0)
{ 

}

// Constructor that parses an input buffer to assign data members
pListMemberRec::pListMemberRec(void* inputBuf)
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);
  char* c_buf_in = (char*) inputBuf;

  listID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&listID);
  bufLen = varLen;

  patientID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&patientID);
  bufLen += varLen;
}

// Initialize data members
void pListMemberRec::clear() 
{
  listID = patientID = 0;
}

// Serialize data members into a single contiguous memory location
void pListMemberRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = listID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  foo = patientID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;
}

// Return the size of the buffer. */
int32 pListMemberRec::getSize() const { 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);

  return bufLen; 
}   

// Print out all data members
void pListMemberRec::show() const 
{
  printf("List ID: %d\n", listID);
  printf("Patient ID: %d\n", patientID);
}

