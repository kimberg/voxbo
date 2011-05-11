/***************************************************************************************
 * This file defines functions in brain region records classes.
 * Copyright (c) 1998-2010 by The VoxBo Development Team
 *
 * This file is part of VoxBo
 * 
 * VoxBo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * VoxBo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with VoxBo.  If not, see <http:*www.gnu.org/licenses/>.
 * 
 * For general information on VoxBo, including the latest complete
 * source code and binary distributions, manual, and associated files,
 * see the VoxBo home page at: http:*www.voxbo.org/
 * 
 * Original version written by Dongbo Hu
 *
 ***************************************************************************************/

using namespace std;

#include "brain_tab.h"
#include "vbutil.h"
#include <string.h>
#include <arpa/inet.h>

// Default constructor
namespaceRec::namespaceRec() 
{ 

}

// Constructor that parses input buffer and assigns data members
namespaceRec::namespaceRec(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  description = c_buf_in + bufLen;
  bufLen += description.size() + 1;
}

// Initialize data members
void namespaceRec::clear() 
{
  name = description = "";
}

// Set a single contiguous memory location that holds values of data members
void namespaceRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;
  
  memcpy(outBuf + bufLen, description.c_str(), description.size() + 1);
  bufLen += description.size() + 1;
}

// Return the size of the buffer
int32 namespaceRec::getSize() const
{ 
  int32 bufLen = 0;
  bufLen += name.size() + 1;
  bufLen += description.size() + 1;
  return bufLen; 
}

// Show contents of data members
void namespaceRec::show() const 
{
  printf("Namespace name: %s\n", name.c_str());
  printf("Namespace description: %s\n", description.c_str());
}

// Default constructor
atlasRec::atlasRec() : ID(0), primaryFlag(0)
{ 

}

// Constructor that parses input buffer and assigns data members
atlasRec::atlasRec(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  ref = c_buf_in + bufLen;
  bufLen += ref.size() + 1;

  type = c_buf_in + bufLen;
  bufLen += type.size() + 1;

  image = c_buf_in + bufLen;
  bufLen += image.size() + 1;

  space = c_buf_in + bufLen;
  bufLen += space.size() + 1;

  primaryFlag = *((uint8 *) (c_buf_in + bufLen));
  bufLen += sizeof(uint8); 
}

// Initialize data members
void atlasRec::clear() 
{
  ID = 0;
  name = ref = type = image = space = "";
  primaryFlag = 0;
}

// Set a single contiguous memory location that holds values of data members
void atlasRec::serialize(char* outBuf) const 
{
  memset(outBuf, 0, 1000);
  int32 bufLen = 0;

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, sizeof(int32));
  bufLen += sizeof(int32);

  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;

  memcpy(outBuf + bufLen, ref.c_str(), ref.size() + 1);
  bufLen += ref.size() + 1;

  memcpy(outBuf + bufLen, type.c_str(), type.size() + 1);
  bufLen += type.size() + 1;

  memcpy(outBuf + bufLen, image.c_str(), image.size() + 1);
  bufLen += image.size() + 1;

  memcpy(outBuf + bufLen, space.c_str(), space.size() + 1);
  bufLen += space.size() + 1;

  memcpy(outBuf + bufLen, &primaryFlag, sizeof(uint8));
  bufLen += sizeof(uint8);
}

// Return size of buffer
int32 atlasRec::getSize() const
{ 
  int32 bufLen = 0;
  bufLen += sizeof(int32);
  bufLen += name.size() + 1;
  bufLen += ref.size() + 1;
  bufLen += type.size() + 1;
  bufLen += image.size() + 1;
  bufLen += space.size() + 1;
  bufLen += sizeof(uint8);
  return bufLen; 
}

// Print out data members
void atlasRec::show() const
{
  printf("Region atlas ID: %d\n", ID);
  printf("Atlas name: %s\n", name.c_str());
  printf("Atlas reference: %s\n", ref.c_str());
  printf("Atlas type: %s\n", type.c_str());
  printf("Atlas image: %s\n", image.c_str());
  printf("Atlas space: %s\n", space.c_str());
  if (primaryFlag)
    printf("Atlas is primary? true\n");
  else
    printf("Atlas is primary? false\n");
}

// Default constructor
regionRec::regionRec() : ID(0), orgID(0), addDate(0), modDate(0)
{ 

}

// Constructor that parses input buffer and assigns data members
regionRec::regionRec(void* inputBuf) 
{ 
  deserialize(inputBuf); 
}

// Set data members based on input buffer 
void regionRec::deserialize(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
  ID = *((int32*) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  name_space = c_buf_in + bufLen;
  bufLen += name_space.size() + 1;

  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  abbrev = c_buf_in + bufLen;
  bufLen += abbrev.size() + 1;

  orgID = *((int32*) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&orgID);
  bufLen += sizeof(int32);
    
  source = c_buf_in + bufLen;
  bufLen += source.size() + 1;

  pFlag = c_buf_in + bufLen;
  bufLen += pFlag.size() + 1;

  link = c_buf_in + bufLen;
  bufLen += link.size() + 1;

  creator = c_buf_in + bufLen;
  bufLen += creator.size() + 1;

  addDate = *((int32*) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&addDate);
  bufLen += sizeof(int32);

  modifier = c_buf_in + bufLen;
  bufLen += modifier.size() + 1;

  modDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&modDate);
  bufLen += sizeof(int32);
}

// Initialize data members
void regionRec::clear() 
{
  ID = 0;
  name_space = name = abbrev = "";
  orgID = 0;
  source = pFlag = link = "";
  creator = modifier = "";
  addDate = modDate = 0;
}

// Set a single contiguous memory location that holds values of data members
void regionRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, name_space.c_str(), name_space.size() + 1);
  bufLen += name_space.size() + 1;

  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;

  memcpy(outBuf + bufLen, abbrev.c_str(), abbrev.size() + 1);
  bufLen += abbrev.size() + 1;

  foo = orgID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, source.c_str(), source.size() + 1);
  bufLen += source.size() + 1;

  memcpy(outBuf + bufLen, pFlag.c_str(), pFlag.size() + 1);
  bufLen += pFlag.size() + 1;

  memcpy(outBuf + bufLen, link.c_str(), link.size() + 1);
  bufLen += link.size() + 1;

  memcpy(outBuf + bufLen, creator.c_str(), creator.size() + 1);
  bufLen += creator.size() + 1;

  foo = addDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, modifier.c_str(), modifier.size() + 1);
  bufLen += modifier.size() + 1;

  foo = modDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;
}

// Return size of buffer
int32 regionRec::getSize() const
{ 
  int32 bufLen = 0;
  bufLen += sizeof (int32);
  bufLen += name_space.size() + 1;
  bufLen += name.size() + 1;
  bufLen += abbrev.size() + 1;
  bufLen += sizeof(int32);
  bufLen += source.size() + 1;
  bufLen += pFlag.size() + 1;
  bufLen += link.size() + 1;
  bufLen += creator.size() + 1;
  bufLen += modifier.size() + 1;
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);

  return bufLen; 
}   

// Print out all data members
void regionRec::show() const
{
  printf("Region ID: %d\n", ID);
  printf("Region namespace: %s\n", name_space.c_str());
  printf("Region name: %s\n", name.c_str());
  printf("Region abbreviation: %s\n", abbrev.c_str());
  printf("Original ID in source: %d\n", orgID);
  printf("Region source: %s\n", source.c_str());
  printf("private flag: %s\n", pFlag.c_str());
  printf("Region link: %s\n", link.c_str());
  printf("Added by: %s\n", creator.c_str());
  if (addDate) {
    time_t tmpTime = addDate;
    printf("Region added on: %s", ctime(&tmpTime));
  }
  else 
    printf("Region added on:\n");

  printf("Modified by: %s\n", modifier.c_str());
  if (modDate) {
    time_t tmpTime = modDate;
    printf("Last modified on: %s", ctime(&tmpTime));
  }
  else
    printf("Last modified on:\n");
}

// Default constructor
synonymRec::synonymRec() : ID(0), sourceID(0), addDate(0), modDate(0)
{ 

}

// Constructor that parses input buffer and assigns data members
synonymRec::synonymRec(void* inputBuf) 
{
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += sizeof(int32);

  name = c_buf_in + bufLen;
  bufLen += name.size() + 1;

  primary = c_buf_in + bufLen;
  bufLen += primary.size() + 1;

  name_space = c_buf_in + bufLen;
  bufLen += name_space.size() + 1;

  sourceID = *((int32 *)(c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&sourceID);
  bufLen += sizeof(int32);

  qualifier = c_buf_in + bufLen;
  bufLen += qualifier.size() + 1;

  creator = c_buf_in + bufLen;
  bufLen += creator.size() + 1;

  addDate = *((int32 *)(c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&addDate);
  bufLen += sizeof(int32);

  modifier = c_buf_in + bufLen;
  bufLen += modifier.size() + 1;

  modDate = *((int32 *)(c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&modDate);
  bufLen += sizeof(int32);

  comments = c_buf_in + bufLen;
  bufLen += comments.size() + 1;
}

// Initialize data members
void synonymRec::clear() 
{
  ID = 0;
  name = "";
  primary = name_space = "";
  sourceID = 0;
  qualifier = "";
  creator = modifier = "";
  addDate = modDate = 0;
  comments = "";
}

// Set a single contiguous memory location that holds values of data members
void synonymRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, name.c_str(), name.size() + 1);
  bufLen += name.size() + 1;

  memcpy(outBuf + bufLen, primary.c_str(), primary.size() + 1);
  bufLen += primary.size() + 1;

  memcpy(outBuf + bufLen, name_space.c_str(), name_space.size() + 1);
  bufLen += name_space.size() + 1;

  foo = sourceID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, qualifier.c_str(), qualifier.size() + 1);
  bufLen += qualifier.size() + 1;

  memcpy(outBuf + bufLen, creator.c_str(), creator.size() + 1);
  bufLen += creator.size() + 1;

  foo = addDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, modifier.c_str(), modifier.size() + 1);
  bufLen += modifier.size() + 1;

  foo = modDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, comments.c_str(), comments.size() + 1);
  bufLen += comments.size() + 1;
}

// Return size of buffer
int32 synonymRec::getSize() const
{ 
  int32 bufLen = 0;
  bufLen += sizeof(ID);
  bufLen += sizeof(sourceID);
  bufLen += name.size() + 1;
  bufLen += primary.size() + 1;
  bufLen += name_space.size() + 1;
  bufLen += qualifier.size() + 1;
  bufLen += creator.size() + 1;
  bufLen += modifier.size() + 1;
  bufLen += sizeof(addDate);
  bufLen += sizeof(modDate);
  bufLen += comments.size() + 1;

  return bufLen; 
}

// Print out data members
void synonymRec::show() const 
{
  printf("Synonym ID: %d\n", ID);
  printf("Synonym name: %s\n", name.c_str());
  printf("Primary structure: %s\n", primary.c_str());
  printf("Namespace: %s\n", name_space.c_str());

  if (sourceID)
    printf("ID in source file: %d\n", sourceID);
  else
    printf("ID in source file:\n");

  printf("Qualifier: %s\n", qualifier.c_str());
  
  printf("Added by: %s\n", creator.c_str());
  if (addDate) {
    time_t tmpTime = addDate;
    printf("Added on: %s", ctime(&tmpTime));
  }
  else
    printf("Added on: NA\n");

  printf("Modified by: %s\n", modifier.c_str());
  if (modDate) {
    time_t tmpTime = modDate;
    printf("Last modified on: %s", ctime(&tmpTime));
  }
  else
    printf("Last modified on: NA\n");

  printf("Comments: %s\n", comments.c_str());
}

// Default constructor
regionRelationRec::regionRelationRec() : ID(0), region1(0), region2(0), addDate(0), modDate(0)
{ 

}

// Constructor that parses input buffer and assigns data members
regionRelationRec::regionRelationRec(void* inputBuf) 
{
  int32 varLen = sizeof(int32);
  int32 bufLen = 0;
  char* c_buf_in = (char*) inputBuf;
  ID = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&ID);
  bufLen += varLen;
    
  region1 = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&region1);
  bufLen += varLen;

  region2 = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&region2);
  bufLen += varLen;

  relationship = c_buf_in + bufLen;
  bufLen += relationship.size() + 1;

  qualifier = c_buf_in + bufLen;
  bufLen += qualifier.size() + 1;
    
  creator = c_buf_in + bufLen;
  bufLen += creator.size() + 1;

  addDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&addDate);
  bufLen += sizeof(int32);

  modifier = c_buf_in + bufLen;
  bufLen += modifier.size() + 1;

  modDate = *((int32 *) (c_buf_in + bufLen));
  if (ntohs(1) == 1)
    swap(&modDate);
  bufLen += sizeof(int32);

  comments = c_buf_in + bufLen;
  bufLen += comments.size() + 1;
}

// Initialize data members
void regionRelationRec::clear() 
{
  ID = 0;
  region1 = region2 = 0;
  relationship = qualifier = "";
  creator = modifier = "";
  addDate = modDate = 0;
  comments = "";
}

// Set a single contiguous memory location that holds values of data members
void regionRelationRec::serialize(char* outBuf) const
{
  int32 bufLen = 0;
  int32 varLen = sizeof(int32);

  int32 foo = ID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  foo = region1;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  foo = region2;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, relationship.c_str(), relationship.size() + 1);
  bufLen += relationship.size() + 1;

  memcpy(outBuf + bufLen, qualifier.c_str(), qualifier.size() + 1);
  bufLen += qualifier.size() + 1;

  memcpy(outBuf + bufLen, creator.c_str(), creator.size() + 1);
  bufLen += creator.size() + 1;

  foo = addDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, modifier.c_str(), modifier.size() + 1);
  bufLen += modifier.size() + 1;

  foo = modDate;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + bufLen, &foo, varLen);
  bufLen += varLen;

  memcpy(outBuf + bufLen, comments.c_str(), comments.size() + 1);
  bufLen += comments.size() + 1;
}

// Return size of buffer
int32 regionRelationRec::getSize() const
{ 
  int32 bufLen = 0;
  bufLen += sizeof(ID);
  bufLen += sizeof(region1);
  bufLen += sizeof(region2);
  bufLen += relationship.size() + 1;
  bufLen += qualifier.size() + 1;
  bufLen += creator.size() + 1;
  bufLen += sizeof(addDate);
  bufLen += modifier.size() + 1;
  bufLen += sizeof(modDate);
  bufLen += comments.size() + 1;		  
  return bufLen; 
}

// Print out all data members
void regionRelationRec::show() const
{
  printf("Relation ID: %d\n", ID);
  printf("Region 1 ID: %d\n", region1);
  printf("Region 2 ID: %d\n", region2);
  printf("Relationship: %s\n", relationship.c_str());
  printf("Qualifier: %s\n", qualifier.c_str());

  printf("Added by: %s\n", creator.c_str());
  if (addDate) {
    time_t tmpTime = addDate;
    printf("Relation added on: %s", ctime(&tmpTime));
  }
  else
    printf("Relation added on:\n");

  printf("Modified by: %s\n", modifier.c_str());
  if (modDate) {
    time_t tmpTime = modDate;
    printf("Last modified on: %s", ctime(&tmpTime));
  }
  else
    printf("Last modified on:\n");

  printf("Comments: %s\n", comments.c_str());
}




