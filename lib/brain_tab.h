/***************************************************************************************
 * This header file includes classes used to brain region records in Berkeley DB tables.
 * Copyright (c) 1998-2010 by The VoxBo Development Team

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

 ***************************************************************************************/
#ifndef BRAIN_TAB_H
#define BRAIN_TAB_H

using namespace std;

#include "typedefs.h"
#include <string>

// Brain region namespace record class
class namespaceRec
{
 public:
  namespaceRec();
  namespaceRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

   void setName(const string& inputStr) { name = inputStr; }
   void setDescription(const string& inputStr) { description = inputStr; }
   string getName() const { return name; }
   string getDescription() const { return description; }

 private:
  string name, description;
};

// Atlas record
class atlasRec
{
 public:
  atlasRec();
  atlasRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; };
  void setName(const string& inputStr) { name = inputStr; }
  void setRef(const string& inputStr) { ref = inputStr; }
  void setType(const string& inputStr) { type = inputStr; }
  void setImage(const string& inputStr) { image = inputStr; }
  void setSpace(const string& inputStr) { space = inputStr; }
  void setPrimary(uint8 inputFlag) { primaryFlag = inputFlag; }

  int32 getID() const { return ID; }
  string getName() const { return name; }
  string getRef() const { return ref; }
  string getType() const { return type; }
  string getImage() const { return image; }
  string getSpace() const { return space; }
  uint8 getPrimaryFlag() const { return primaryFlag; }

 private:
  int32 ID;
  string name, ref, type, image, space;
  uint8 primaryFlag;
};

// Brain region name record class
class regionRec
{
 public:
  regionRec();
  regionRec(void*);
  void deserialize(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setNameSpace(const string& inputStr) { name_space = inputStr; }
  void setName(const string& inputStr) { name = inputStr; }
  void setAbbrev(const string& inputStr) { abbrev = inputStr; }
  void setOrgID(int32 inputVal) { orgID = inputVal; }
  void setSource(const string& inputStr) { source = inputStr; }
  void setPrivate(const string& inputStr) { pFlag = inputStr; }
  void setLink(const string& inputStr) { link = inputStr; }
  void setCreator(const string& inputStr) { creator = inputStr; }
  void setAddDate(int32 inputVal) { addDate = inputVal; }
  void setModifier(const string& inputStr) { modifier = inputStr; }
  void setModDate(int32 inputVal) { modDate = inputVal; }

  int32 getID() const { return ID; }
  string getNameSpace() const { return name_space; }
  string getName() const { return name; }
  string getAbbrev() const { return abbrev; }
  int32 getOrgID() const { return orgID; }
  string getSource() const { return source; }
  string getPrivate() const { return pFlag; }
  string getLink() const { return link; }
  string getCreator() const { return creator; }
  int32 getAddDate() const { return addDate; }
  string getModifier() const { return modifier; }
  int32 getModDate() const { return modDate; }

 private:
  int32 ID;
  string name_space, name, abbrev;
  int32 orgID; // original ID from input file, such as ID field in NN2002's hierarchy table
  string source, pFlag, link;
  string creator, modifier;
  int32 addDate, modDate;
};

// Synonym record class
class synonymRec
{
 public:
  synonymRec();
  synonymRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

  void setID(int32 inputVal) { ID = inputVal; }
  void setName(const string& inputStr) { name = inputStr; }
  void setPrimary(const string& inputStr) { primary = inputStr; }
  void setNameSpace(const string& inputStr) { name_space = inputStr; }
  void setSourceID(int32 inputVal) { sourceID = inputVal; }
  void setQualifier(const string& inputStr) { qualifier = inputStr; }
  void setCreator(const string& inputStr) { creator = inputStr; }
  void setAddDate(int32 inputVal) { addDate = inputVal; }
  void setModifier(const string& inputStr) { modifier = inputStr; }
  void setModDate(int32 inputVal) { modDate = inputVal; }
  void setComments(const string& inputStr) { comments = inputStr; }

  int32 getID() const { return ID; }
  string getName() const { return name; }
  string getPrimary() const { return primary; }
  string getNameSpace() const { return name_space; }
  int32 getSourceID() const { return sourceID; }
  string getQualifier() const { return qualifier; }
  string getCreator() const { return creator; }
  int32 getAddDate() const { return addDate; }
  string getModifier() const { return modifier; }
  int32 getModDate() const { return modDate; }
  string getComments() const { return comments; }

 private:
  int32 ID, sourceID;
  string name, primary, name_space;
  string qualifier;
  string creator, modifier;
  int32 addDate, modDate;
  string comments;
};

// Region relationship record class
class regionRelationRec
{
 public:
  regionRelationRec();
  regionRelationRec(void*);
  void clear();
  void serialize(char*) const;
  int32 getSize() const;
  void show() const;

   void setID(int32 inputVal) { ID = inputVal; }
   void setRegion1(int32 inputVal) { region1 = inputVal; }
   void setRegion2(int32 inputVal) { region2 = inputVal; }
   void setRelationship(const string& inputStr) { relationship = inputStr; }
   void setQualifier(const string& inputStr) { qualifier = inputStr; }
   void setCreator(const string& inputStr) { creator = inputStr; }
   void setAddDate(int32 inputVal) { addDate = inputVal; }
   void setModifier(const string& inputStr) { modifier = inputStr; }
   void setModDate(int32 inputVal) { modDate = inputVal; }
   void setComments(const string& inputStr) { comments = inputStr; }

   int32 getID() const { return ID; }
   int32 getRegion1() const { return region1; }
   int32 getRegion2() const { return region2; }
   string getRelationship() const { return relationship; }
   string getQualifier() const { return qualifier; }
   string getCreator() const { return creator; }
   int32 getAddDate() const { return addDate; }
   string getModifier() const { return modifier; }
   int32 getModDate() const { return modDate; }
   string getComments() const { return comments; }

 private:
  int32 ID, region1, region2;
  string relationship, qualifier;
  string creator, modifier;
  int32 addDate, modDate;
  string comments;
};

#endif

