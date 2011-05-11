
// mydefs.cpp
// defs for database
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
// original version written by Dan Kimberg, many contributions
// by Dongbo Hu

using namespace std;

#include "mydefs.h"
#include "vbutil.h"
#include <arpa/inet.h>

using boost::format;

/************************************
 *   DBType functions
 ************************************/
/* Default constructor initializes members. */
DBtype::DBtype()
{
  t_custom = 0;
}

/* Initialization function. */
void DBtype::clear()
{
  name = description = "";
  values.clear();;
  t_custom = 0;
}

/* Constructor that accepts a data block and assigns values to individual members. */
DBtype::DBtype(void* inputBuf)
{
  deserialize(inputBuf);
}

/* Set class members based on input data buffer. */
void DBtype::deserialize(void* inputBuf)
{
  int32 offset = 0;
  char* c_buf_in = (char*) inputBuf;

  name = c_buf_in + offset;
  offset += name.size() + 1;

  description = c_buf_in + offset;
  offset += description.size() + 1;

  t_custom = *((uint8 *) (c_buf_in + offset));
  offset += sizeof(uint8);

  uint32 vec_len = *((uint32*) (c_buf_in + offset));
  // make sure vec_len is parsed correctly
  if (ntohs(1) == 1)
    swap(&vec_len);
  offset += sizeof(uint32);
  for (uint32 i = 0; i < vec_len; i++) {
    string tmpVal = c_buf_in + offset;
    values.push_back(tmpVal);
    offset += tmpVal.size() + 1;
  }
}

/* Serialize class members */
void DBtype::serialize(char* outBuf) const
{
  int32 offset = 0;
  memcpy(outBuf + offset, name.c_str(), name.size() + 1);
  offset += name.size() + 1;

  memcpy(outBuf + offset, description.c_str(), description.size() + 1);
  offset += description.size() + 1;

  memcpy(outBuf + offset, &t_custom, sizeof(uint8));
  offset += sizeof(uint8);

  uint32 vec_len = values.size();
  if (ntohs(1) == 1)
    swap(&vec_len);

  memcpy(outBuf + offset, &vec_len, sizeof(uint32));
  offset += sizeof(uint32);

  for (size_t i = 0; i < values.size(); i++) {
    memcpy(outBuf + offset, values[i].c_str(), values[i].size() + 1);
    offset += values[i].size() + 1;
  }
}

/* This function returns the size of buffer that holds serialized class members. */
int32 DBtype::getSize() const
{
  int32 bufLen = 0;
  bufLen += name.size() + 1;
  bufLen += description.size() + 1; 
  bufLen += sizeof(uint8);
  bufLen += sizeof(uint32);

  for (size_t i = 0; i < values.size(); i++)
    bufLen += values[i].size() + 1; 

  return bufLen;
}

/* Print out class members (mainly for debugging) */
void DBtype::show() const
{
  printf("Type name: %s\n", name.c_str());
  printf("     desc: %s\n", description.c_str());
  printf("     custom flag: ");
  if (t_custom)
    printf("true\n");
  else
    printf("false\n");
  printf("     Possible values:\n");
  for (size_t i = 0; i < values.size(); i++)
    printf("       %s\n", values[i].c_str());
}

void
DBtype::print() const
{
  cout << format("DBtype %s (%s):\n")%name%description;
  vbforeach(string v,values)
    cout << "  value: " << v << endl;
}

/**************************************************
 *   DBsession functions 
 **************************************************/
/* Default constructor initializes members. */
DBsession::DBsession()
{
  id = 0;
  studyID = 0;
  patient = 0;
  pubFlag = 0;
}

/* Constructor that accepts a data block and assigns values to individual members. */
DBsession::DBsession(void* inputBuf)
{
  deserialize(inputBuf);
}

/* Set class members based on input data buffer. */
void DBsession::deserialize(void* inputBuf)
{
  int32 offset = 0;
  char* c_buf_in = (char*) inputBuf;

  id = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&id);
  offset += sizeof(int32);

  studyID = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&studyID);
  offset += sizeof(int32);

  patient = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&patient);
  offset += sizeof(int32);

  examiner = c_buf_in + offset;
  offset += examiner.size() + 1;

  int32 sec_unix = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&sec_unix);
  date = DBdate(sec_unix);
  offset += sizeof(sec_unix); 

  pubFlag = *((uint8 *) (c_buf_in + offset));
  offset += sizeof(uint8);

  notes = c_buf_in + offset;
  offset += notes.size() + 1;
}

/* Initialization function. */
void DBsession::clear()
{
  id = studyID = patient = 0;
  examiner = notes = "";
  date.clear();
  pubFlag = 0;
}

/* Serialize class members */
void DBsession::serialize(char* outBuf) const
{
  int32 offset = 0;
  int32 foo = id;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  foo = studyID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  foo = patient;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  memcpy(outBuf + offset, examiner.c_str(), examiner.size() + 1);
  offset += examiner.size() + 1;

  int32 sec_unix = date.getUnixTime();
  if (ntohs(1) == 1)
    swap(&sec_unix);
  memcpy(outBuf + offset, &sec_unix, sizeof(int32));
  offset += sizeof(int32);

  memcpy(outBuf + offset, &pubFlag, sizeof(uint8));
  offset += sizeof(uint8);

  memcpy(outBuf + offset, notes.c_str(), notes.size() + 1);
  offset += notes.size() + 1;
}

/* This function returns the size of buffer that holds serialized class members. */
int32 DBsession::getSize() const
{
  int32 bufLen = 0;
  bufLen += sizeof(id);
  bufLen += sizeof(studyID);
  bufLen += sizeof(patient);
  bufLen += examiner.size() + 1;
  bufLen += sizeof(int32);
  bufLen += sizeof(uint8);
  bufLen += notes.size() + 1;

  return bufLen;
}

/* Print out class members (mainly for debugging) */
void DBsession::show() const
{
  printf("Session ID: %d\n", id);
  printf("        study ID: %d\n", studyID);
  printf("        patient ID: %d\n", patient);
  printf("        examiner: %s\n", examiner.c_str());
  printf("        date: %s\n", date.getDateStr());
  printf("        public flag: %d\n", pubFlag);
  printf("        notes: %s\n", notes.c_str());
}

void
DBsession::print() const
{
  cout << format("DBsession %d: studyid %d patientid %d examiner %s\n")%id%studyID%
    patient%examiner;
}

/*****************************************
 *    DBpatient functions
 *****************************************/
// Default constructor
DBpatient::DBpatient()
{
  patientID = 0;
}

// Constructor that sets patient id and maps 
DBpatient::DBpatient(int32 pid_in, vector<DBscorevalue> score_vec, vector<DBsession> sess_vec)
{
  setData(pid_in, score_vec, sess_vec);
}

// Initialize class members
void DBpatient::clear()
{
  patientID = 0;
  sessions.clear();
  scores.clear();
  children.clear();
  names.clear();
  sessiontests.clear();
}

// Set maps and multimaps
void DBpatient::setData(int32 pid_in, vector<DBscorevalue> score_vec, vector<DBsession> sess_vec)
{
  patientID = pid_in;

  typedef map<int32, DBsession> sess_map;
  typedef map<int32, DBscorevalue> score_map;
  typedef multimap<int32, int32> children_map;
  typedef multimap<string, int32> name_map;
  typedef multimap<int32, int32> ss_map; 

  // set sessions map
  for (uint32 i = 0; i < sess_vec.size(); i++)
    sessions.insert(sess_map::value_type(sess_vec[i].id, sess_vec[i]));

  // set the other four maps: scores, children, names, sessiontests
  for (uint32 i = 0; i < score_vec.size(); i++) {
    int32 tmp_id = score_vec[i].id;
    string scorename = score_vec[i].scorename;
    int32 tmp_sess = score_vec[i].sessionid;
    int32 tmp_parent = score_vec[i].parentid;
    // string tmp_val = score_vec[i].value;
    scores.insert(score_map::value_type(tmp_id, score_vec[i]));
    children.insert(children_map::value_type(tmp_parent, tmp_id));
    names.insert(name_map::value_type(scorename, tmp_id));
    // session 0 is also included (in order to tell whether test is static or not)
    if (tmp_parent == 0)
      sessiontests.insert(ss_map::value_type(tmp_sess, tmp_id));
  }
}

// Set sessions map based on input vector
void DBpatient::setSessionMap(vector<DBsession>& inputVec)
{
  for (uint32 i = 0; i < inputVec.size(); i++) {
    int32 sid = inputVec[i].id;
    sessions[sid] = inputVec[i];
  }
}

// Reset score value related maps when scores are changed
void DBpatient::resetMaps()
{
  // set the other four maps: scores, children, names, sessiontests
  map<int32, DBscorevalue>::const_iterator it;
  for (it = scores.begin(); it != scores.end(); ++it) {
    int32 tmp_id = it->first;
    string scorename = it->second.scorename;
    int32 tmp_sess = it->second.sessionid;
    int32 tmp_parent = it->second.parentid;
    children.insert(multimap<int32, int32>::value_type(tmp_parent, tmp_id));
    names.insert(multimap<string, int32>::value_type(scorename, tmp_id));
    // session 0 is also included (in order to tell whether test is static or not)
    if (tmp_parent == 0)
      sessiontests.insert(multimap<int32, int32>::value_type(tmp_sess, tmp_id));
  }
}

// Update DBdate objects in session and scorevalue maps
void DBpatient::updateTime(uint32 t_stamp)
{
  map<int32, DBsession>::iterator sess_it;
  for (sess_it = sessions.begin(); sess_it != sessions.end(); ++sess_it)
    sess_it->second.date.setUnixTime(t_stamp);

  map<int32, DBscorevalue>::iterator sv_it;
  for (sv_it = scores.begin(); sv_it != scores.end(); ++sv_it)
    sv_it->second.whenset.setUnixTime(t_stamp);
}

// Returns data size of patient sessions
uint32 DBpatient::getSessionSize() const
{
  uint32 sess_size = 0;
  map<int32, DBsession>::const_iterator it;
  for (it = sessions.begin(); it != sessions.end(); ++it)
    sess_size += it->second.getSize();

  return sess_size;
}

// Serialize sessions in the map only
void DBpatient::serializeSessions(char* outBuff) const
{
  uint32 offset = 0;
  map<int32, DBsession>::const_iterator it;
  for (it = sessions.begin(); it != sessions.end(); ++it) {
    if (it->second.id<=0) continue;
    it->second.serialize(outBuff + offset);
    offset += it->second.getSize();
  }
}

void
DBpatient::print() const
{
  cout << format("DBpatient %d:\n")%patientID;
  pair<int32,DBscorevalue> ss;
  vbforeach(ss,scores) {
    cout << "  ";
    ss.second.print();
  }
}

/*************************************************
 *    DBscorename functions 
 *************************************************/
/* Simple construtor that initialize members. */ 
DBscorename::DBscorename() 
{ 
  // do nothing now
}

/* Constructor that accepts a data block and assigns values to individual members. */
DBscorename::DBscorename(void* inputBuf)
{
  deserialize(inputBuf);
}

/* Set variables beased on input buffer. */
void DBscorename::deserialize(void* inputBuf)
{
  int32 offset = 0;
  char* c_buf_in = (char*) inputBuf;
  // score name
  name = c_buf_in + offset;
  offset += name.size() + 1;
  // screen name
  screen_name = c_buf_in + offset;
  offset += screen_name.size() + 1;
  // datatype
  datatype = c_buf_in + offset;
  offset += datatype.size() + 1; 
  // description
  desc = c_buf_in + offset;
  offset += desc.size() + 1; 
  // flags
  uint32 flag_num = *((uint32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&flag_num);
  offset += sizeof(uint32);

  for (uint i = 0; i < flag_num; i++) {
    string f_name = c_buf_in + offset;
    offset += f_name.size() + 1;
    string f_val = c_buf_in + offset;
    offset += f_val.size() + 1;
    flags[f_name] = f_val;
  }
}

/* Initialization function. */
void DBscorename::clear()
{
  name = screen_name = datatype = desc = "";
  flags.clear();
}

/* This function returns the size of data buffer that holds serialized class members. */
int32 DBscorename::getSize() const
{
  int32 buf_size = 0;
  buf_size += name.size() + 1;
  buf_size += screen_name.size() + 1;
  buf_size += datatype.size() + 1; 
  buf_size += desc.size() + 1;
  // map of flags
  buf_size += sizeof(uint32); // size of map is needed in serialized data
  map<string, string>::const_iterator it;
  for (it = flags.begin(); it != flags.end(); ++it) {
    buf_size += it->first.size() + 1;
    buf_size += it->second.size() + 1;
  }

  return buf_size;
}

/* Serialize class members */
void DBscorename::serialize(char* outBuf) const
{
  int32 offset = 0;
  // score name
  memcpy(outBuf + offset, name.c_str(), name.size() + 1);
  offset += name.size() + 1;
  // screen name
  memcpy(outBuf + offset, screen_name.c_str(), screen_name.size() + 1);
  offset += screen_name.size() + 1;
  // datatype
  memcpy(outBuf + offset, datatype.c_str(), datatype.size() + 1);
  offset += datatype.size() + 1;
  // description
  memcpy(outBuf + offset, desc.c_str(), desc.size() + 1);
  offset += desc.size() + 1;
  // map size has to be serialized too
  uint32 flag_num = flags.size();
  if (ntohs(1) == 1)
    swap(&flag_num);
  memcpy(outBuf + offset, &flag_num, sizeof(uint32));
  offset += sizeof(uint32);
  // serialize elements in map of flags
  map<string, string>::const_iterator it;
  for (it = flags.begin(); it != flags.end(); ++it) {
    memcpy(outBuf + offset, it->first.c_str(), it->first.size() + 1);
    offset += it->first.size() + 1;
    memcpy(outBuf + offset, it->second.c_str(), it->second.size() + 1);
    offset += it->second.size() + 1;
  }
}

/* Print out class members (mainly for debugging) */
void DBscorename::show() const 
{
  printf("Score name: %s\n", name.c_str());
  printf("screen name: %s\n", screen_name.c_str());
  printf("data type: %s\n", datatype.c_str());
  printf("description: %s\n", desc.c_str());
  printf("flags:\n");
  map<string, string>::const_iterator it;
  for (it = flags.begin(); it != flags.end(); ++it) 
    printf("%s: %s\n", it->first.c_str(), it->second.c_str());
}

void
DBscorename::print() const
{
  cout << format("DBscorename %s (%s): %s\n")%name%datatype%desc;
}

/**************************************************
 *    DBscorevalue functions 
 **************************************************/
/* Simple constructor of DBscorevalue */
DBscorevalue::DBscorevalue() 
{
  id=0;
  patient=0;
  sessionid=0;
  parentid=0;
  index=0;
  deleted=0;
}

/* DBscorevalue class that interprets an input block of data */
DBscorevalue::DBscorevalue(void* inputBuf)
{
  deserialize(inputBuf);
}

/* Copy constructor is required by map insert operations. 
 * Not sure why. Without this ctor segv will occur. */
DBscorevalue::DBscorevalue(const DBscorevalue& inputRec)
{
  key = inputRec.key;
  id = inputRec.id;
  patient = inputRec.patient;
  scorename = inputRec.scorename;
  datatype = inputRec.datatype;
  sessionid = inputRec.sessionid;
  parentid = inputRec.parentid;
  index = inputRec.index;
  deleted = inputRec.deleted;
  permission = inputRec.permission;
  whenset = inputRec.whenset;
  setby = inputRec.setby;
  v_date = inputRec.v_date;
  v_cube = inputRec.v_cube;
  v_pixmap = inputRec.v_pixmap;
  v_string = inputRec.v_string;
}

/* Initialize members in DBscorevalue class */
void DBscorevalue::clear()
{
  id = patient = parentid = sessionid = 0;
  key = permission = setby = scorename = datatype = "";
  index = 0;
  deleted = 0;
  v_date = 0;
}

/* Function that interprets input data block and assigns values to class members */
void DBscorevalue::deserialize(void* inputBuf)
{
  uint32 offset = deserializeHdr(inputBuf);
  // get score value data size
  uint32 datasize = *((uint32*) ((char*) inputBuf + offset));
  if (ntohs(1) == 1)
    swap(&datasize);
  offset += sizeof(uint32);
  // set data, depending on the data type
  deserialize((uint8*) inputBuf + offset, datasize);
}

/* Function that interprets input data block and assigns values to class members */
uint32 DBscorevalue::deserializeHdr(void* inputBuf)
{
  uint32 offset = 0;
  char* c_buf_in = (char*) inputBuf;

  /* whenset: time stamp at which this record is set.
   * whenset is added at the very beginning of data on purpose, so that
   * server can update it without deserialize any other fields. */
  int32 sec_unix = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&sec_unix);
  whenset = DBdate(sec_unix);
  offset += sizeof(int32); 

  // unique key
  key =  c_buf_in + offset;
  offset += key.size() + 1;
  // score value ID
  id = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&id);
  offset += sizeof(int32);
  // patient ID
  patient = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&patient);
  offset += sizeof(int32);
  // score name string
  scorename = c_buf_in + offset;
  offset += scorename.size() + 1;
  // datatype
  datatype = c_buf_in + offset;
  offset += datatype.size() + 1;
  // session ID
  sessionid = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&sessionid);
  offset += sizeof(int32);
  // parent ID
  parentid = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&parentid);
  offset += sizeof(int32);
  // index
  index = *((uint32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&index);
  offset += sizeof(uint32);
  // deleted flag
  deleted = *((uint8*) (c_buf_in + offset));
  offset += sizeof(uint8);
  // permission
  permission = c_buf_in + offset;
  offset += permission.size() + 1;
  // setby: user name who sets this score value record
  setby = c_buf_in + offset;
  offset += setby.size() + 1;

  // score value data excluded in this function

  return offset;
}

/* This function returns the size of buffer that holds class members */
uint32 DBscorevalue::getSize() 
{
  uint32 buf_len = getHdrSize();
  buf_len += sizeof(uint32);
  buf_len += getDatSize();

  return buf_len;
}

/* This function returns the size of buffer that holds class members */
uint32 DBscorevalue::getHdrSize() const
{
  uint32 buf_size = 0;
  buf_size += sizeof(int32);  // whenset is converted to int32 in buffer
  buf_size += key.size() + 1;
  buf_size += sizeof(id);
  buf_size += sizeof(patient);
  buf_size += scorename.size() + 1;
  buf_size += datatype.size() + 1;
  buf_size += sizeof(sessionid);
  buf_size += sizeof(parentid);
  buf_size += sizeof(index);
  buf_size += sizeof(deleted);
  buf_size += permission.size() + 1;
  buf_size += setby.size() + 1;
  // score value data is excluded

  return buf_size;
}

/* Returns score value data size */
uint32 DBscorevalue::getDatSize() 
{
  uint32 dat_len = 0;

  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    string tmps=(format("%d %d %d %d %d %d") % v_date.getYear() % v_date.getMonth() % v_date.getDay()
		 % v_date.getHour() % v_date.getMinute() % v_date.getSecond()).str();
    dat_len = tmps.size()+1;
  }
  else if (datatype=="brainimage") {
    string header=v_cube.header2string();
    int cdatasize=v_cube.datasize*v_cube.dimx*v_cube.dimy*v_cube.dimz;
    dat_len = header.size()+1+cdatasize;
  }
  else if (datatype=="image") {
    dat_len=v_pixmap.size;
  }
  else {  // default includes string, shortstring, text, custom types, etc.
    dat_len = v_string.size()+1;
  }

  return dat_len;  
}

/* Serialize class member values into the input data buffer */
void DBscorevalue::serialize(char* outBuf)
{
  int32 offset = serializeHdr(outBuf);
  // Include score value data size
  uint32 dat_len = getDatSize();
  if (ntohs(1) == 1)
    swap(&dat_len);
  memcpy(outBuf + offset, &dat_len, sizeof(uint32));
  offset += sizeof(uint32);

  // Concatenate  score value data in binary format at the end of data buffer
  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    string tmps=(format("%d %d %d %d %d %d") % v_date.getYear() % v_date.getMonth() % v_date.getDay()
		 % v_date.getHour() % v_date.getMinute() % v_date.getSecond()).str();
    memcpy(outBuf + offset, tmps.c_str(), dat_len);
  }
  else if (datatype=="brainimage") {
    string header=v_cube.header2string();
    int cdatasize=v_cube.datasize*v_cube.dimx*v_cube.dimy*v_cube.dimz;
    memcpy(outBuf + offset, header.c_str(), header.size()+1);
    memcpy(outBuf + offset + header.size() + 1, v_cube.data, cdatasize);
  }
  else if (datatype=="image") {
    memcpy(outBuf+offset,v_pixmap.data,v_pixmap.size);
  }
  else {  // default includes string, shortstring, text, custom types, etc.
    memcpy(outBuf + offset,v_string.c_str(), dat_len);
  }
}

// create a short string version of this field

string
DBscorevalue::printable(bool full) const
{
  // Concatenate  score value data in binary format at the end of data buffer
  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    return (format("%d %d %d %d %d %d") % v_date.getYear() % v_date.getMonth() % v_date.getDay()
            % v_date.getHour() % v_date.getMinute() % v_date.getSecond()).str();
  }
  else if (datatype=="brainimage") {
    return "<brain image>";
  }
  else if (datatype=="image") {
    return "<image>";
  }
  else {  // default includes string, shortstring, text, custom types, etc.
    string tmps=v_string;
    if (!full) tmps=tmps.substr(0,40);
    return tmps;
  }
}

/* Serialize class member values into the input data buffer */
uint32 DBscorevalue::serializeHdr(char* outBuf) const
{
  int32 offset = 0;
  /* whenset: time stamp at which this record is set
   * whenset is put at the very beginning of serialized data on purpose, so that 
   * the server can always update it without deserialize any other fields. */
  int32 foo = whenset.getUnixTime();
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  // unique key
  memcpy(outBuf + offset, key.c_str(), key.size() + 1);
  offset += key.size() + 1;
  // score value ID
  foo = id;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  // patient ID
  foo = patient;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  // score name string
  memcpy(outBuf + offset, scorename.c_str(), scorename.size() + 1);
  offset += scorename.size() + 1;
  // datatype string
  memcpy(outBuf + offset, datatype.c_str(), datatype.size() + 1);
  offset += datatype.size() + 1;
  // session ID
  foo = sessionid;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  // parent ID
  foo = parentid;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);
  // index 
  foo = index;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(uint32));
  offset += sizeof(uint32);
  // deleted flag
  memcpy(outBuf + offset, &deleted, sizeof(uint8));
  offset += sizeof(uint8);
  // permission
  memcpy(outBuf + offset, permission.c_str(), permission.size() + 1);
  offset += permission.size() + 1;
  // setby: user name who sets this score value record
  memcpy(outBuf + offset, setby.c_str(), setby.size() + 1);
  offset += setby.size() + 1;

  // binary score value data excluded in this function

  return offset;
}

/* Print out class members (mainly for debugging) */
void DBscorevalue::show()
{ 
  printf("Score value Key: %s\n", key.c_str());
  printf("      score value ID: %d\n", id);
  printf("      patient ID: %d\n", patient);
  printf("      score name: %s\n", scorename.c_str());
  printf("      data type: %s\n", datatype.c_str());
  printf("      session ID: %d\n", sessionid);
  printf("      parent ID: %d\n", parentid);
  printf("      index: %d\n", index);
  if (deleted)
    printf("      deleted flag: yes\n");
  else
    printf("      deleted flag: no\n");

  printf("      permission: %s\n", permission.c_str());
  printf("      time created: %s\n", whenset.getDateStr());
  printf("      set by: %s\n", setby.c_str());

  string tmp = getDatStr();
  if (tmp.size())
    printf("      score value: %s\n", tmp.c_str());
  else
    printf("      score value: image type not shown");
}

/* Return data in string format (if the data type is convertable).
 * Otherwise return a empty string. */
string DBscorevalue::getDatStr()
{
  string foobar;
  if (datatype == "brainimage" || datatype == "image") {
    return foobar;
  }

  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    foobar = (format("%d %d %d %d %d %d") % v_date.getYear() % v_date.getMonth() % v_date.getDay()
	      % v_date.getHour() % v_date.getMinute() % v_date.getSecond()).str();
    return foobar;
  }

  return v_string;
}

// serialize/deserialize return/take pointers to the serial data and
// length and convert from/to the data in the data union

pair<uint8*, uint32>
DBscorevalue::serialize()
{
  uint8 *data = 0;
  uint32 length = 0;
  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    string tmps=(format("%d %d %d %d %d %d") % v_date.getYear() % v_date.getMonth() % v_date.getDay()
		 % v_date.getHour() % v_date.getMinute() % v_date.getSecond()).str();
    length=tmps.size()+1;
    data=new uint8[length];
    memcpy(data,tmps.c_str(),length);
  }
  else if (datatype=="brainimage") {
    string header=v_cube.header2string();
    int cdatasize=v_cube.datasize*v_cube.dimx*v_cube.dimy*v_cube.dimz;
    length=header.size()+1+cdatasize;
    data=new uint8[length];
    memcpy(data,header.c_str(),header.size()+1);
    memcpy(data+header.size()+1,v_cube.data,cdatasize);
  }
  else if (datatype=="image") {
    length=v_pixmap.size;
    data=new uint8[length];
    memcpy(data,v_pixmap.data,length);
  }
  // default includes string, shortstring, text, int, float, bool,
  // custom types.  note that int/float are stored as strings because
  // we mostly just need to punch them into a lineedit widget
  else {
    length=v_string.size()+1;
    data=new uint8[length];
    memcpy(data,v_string.c_str(),length);
  }

  return make_pair(data, length);
}

void
DBscorevalue::deserialize(uint8 *data,uint32 length)
{
  if (datatype=="date" || datatype=="time" || datatype=="timedate" || datatype=="datetime") {
    if (data[length-1]==0) {
      tokenlist foo;
      foo.ParseLine((char *)data);
      if (foo.size()!=6)
        v_date.clear();
      else {
        v_date.setDate(strtol(foo[1]),strtol(foo[2]),strtol(foo[0]));
        v_date.setTime(strtol(foo[3]),strtol(foo[4]),strtol(foo[5]));
      }
    }
    else
      v_date.clear();
  }
  else if (datatype=="brainimage") {
    string hdr;
    int offset=0;
    char c;
    while ((c=data[offset++])!='\0')
      hdr+=c;
    v_cube.string2header(hdr);
    int cdatasize=v_cube.datasize*v_cube.dimx*v_cube.dimy*v_cube.dimz;
    v_cube.data=new unsigned char[cdatasize];
    memcpy(v_cube.data,data+offset,cdatasize);
  }
  else if (datatype=="image") {
    v_pixmap.init(data,length);
  }
  else {  // default includes string, shortstring, text, custom types, etc.
    if (data[length-1]!=0)
      v_string="";
    else
      v_string=(char *)data;
  }
}

void
DBscorevalue::print() const
{
  cout << format("DBscorevalue %d (%s): sid=%d idx=%d %s=%s\n")%
    id%datatype%sessionid%index%scorename%(printable(1));  
}

/* Default constructor */
DBpatientlist::DBpatientlist()
{
  id = 0;
  ownerID = 0;
  dirtyFlag = 0;
}

/* Constructor that accepts a serialized data buffer. */
DBpatientlist::DBpatientlist(void* inputBuf)
{
  int32 offset = 0;
  char* c_buf_in = (char*) inputBuf;

  id = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&id);
  offset += sizeof(int32);

  ownerID= *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&ownerID);
  offset += sizeof(int32);

  name = c_buf_in + offset;
  offset += name.size() + 1;

  search_strategy = c_buf_in + offset;
  offset += search_strategy.size() + 1;

  notes = c_buf_in + offset;
  offset += notes.size() + 1;

  int32 sec_unix = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&sec_unix);
  runDate = DBdate(sec_unix);
  offset += sizeof(int32); 

  sec_unix = *((int32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&sec_unix);
  modDate = DBdate(sec_unix);
  offset += sizeof(int32); 

  dirtyFlag = *((uint8 *) (c_buf_in + offset));
  offset += sizeof(uint8);

  uint32 patientNum = *((uint32*) (c_buf_in + offset));
  if (ntohs(1) == 1)
    swap(&patientNum);
  offset += sizeof(uint32); 
  for (uint32 i = 0; i < patientNum; i++) {
    int32 pID = *((int32*) (c_buf_in + offset));
    if (ntohs(1) == 1)
      swap(&pID);
    patientIDs.insert(pID);
    offset += sizeof(int32);
  }
}

/* Serialize class members */
void DBpatientlist::serialize(char* outBuf) const
{
  int32 offset = 0;
  int32 foo = id;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  foo = ownerID;
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  memcpy(outBuf + offset, name.c_str(), name.size() + 1);
  offset += name.size() + 1;

  memcpy(outBuf + offset, search_strategy.c_str(), search_strategy.size() + 1);
  offset += search_strategy.size() + 1;

  memcpy(outBuf + offset, notes.c_str(), notes.size() + 1);
  offset += notes.size() + 1;

  foo = runDate.getUnixTime();
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  foo = modDate.getUnixTime();
  if (ntohs(1) == 1)
    swap(&foo);
  memcpy(outBuf + offset, &foo, sizeof(int32));
  offset += sizeof(int32);

  memcpy(outBuf + offset, &dirtyFlag, sizeof(uint8));
  offset += sizeof(uint8);

  uint32 pNum = patientIDs.size();
  if (ntohs(1) == 1)
    swap(&pNum);
  memcpy(outBuf + offset, &pNum, sizeof(uint32));
  offset += sizeof(uint32);

  set<int32>::const_iterator it = patientIDs.begin();
  while (it != patientIDs.end()) {
    foo = *it;
    if (ntohs(1) == 1)
      swap(&foo);
    memcpy(outBuf + offset, &foo, sizeof(int32));
    offset += sizeof(int32);
    ++it;
  }
}

/* Returns the size of buffer that holds serialized class members. */
int32 DBpatientlist::getSize() const
{
  int32 bufLen = 0;
  bufLen += sizeof(id);
  bufLen += sizeof(ownerID);
  bufLen += name.size() + 1; 
  bufLen += search_strategy.size() + 1; 
  bufLen += notes.size() + 1; 
  bufLen += sizeof(int32);
  bufLen += sizeof(int32);
  bufLen += sizeof(dirtyFlag);
  bufLen += sizeof(uint32);  // number of patient IDs
  bufLen += patientIDs.size() * sizeof(int32); 

  return bufLen;
}

/* Print out data members */
void DBpatientlist::show() const
{
  printf("Patient list ID: %d\n", id);
  printf("      owner ID: %d\n", ownerID);
  printf("      name: %s\n", name.c_str());
  printf("      search strategy: %s\n", search_strategy.c_str());
  printf("      notes: %s\n", notes.c_str());
  printf("      run date: %s\n", runDate.getDateStr());
  printf("      modification date: %s\n", modDate.getDateStr());
  if (dirtyFlag)
    printf("      flag: true\n");
  else
    printf("      flag: false\n");
  printf("      patient IDs:");
  set<int32>::const_iterator it = patientIDs.begin();
  while (it != patientIDs.end()) {
    printf(" %d ", *it);
    ++it;
  }
  printf("\n");
}

/***********************************************
 *    Utility functions added by Dan 
 ***********************************************/
vector<int32>
getchildren(multimap<int32,int32>& mymap, int32 parent)
{
  vector<int32> ret;
  multimap<int32,int32>::iterator it;
  pair<multimap<int32,int32>::iterator,multimap<int32,int32>::iterator> range;
  range=mymap.equal_range(parent);
  for (it=range.first; it!=range.second; it++)
    ret.push_back(it->second);
  return ret;
}

vector<int32>
getchildren(multimap<string,int32>& mymap,string parent)
{
  vector<int32> ret;
  multimap<string,int32>::iterator it;
  pair<multimap<string,int32>::iterator,multimap<string,int32>::iterator> range;
  range=mymap.equal_range(parent);
  for (it=range.first; it!=range.second; it++)
    ret.push_back(it->second);
  return ret;
}

vector<string>
getchildren(multimap<string,string>& mymap,string parent)
{
  vector<string> ret;
  multimap<string,string>::iterator it;
  pair<multimap<string,string>::iterator,multimap<string,string>::iterator> range;
  range=mymap.equal_range(parent);
  for (it=range.first; it!=range.second; it++)
    ret.push_back(it->second);
  return ret;
}


string
scoreparent(const string& str)
{
  size_t pos=str.rfind(":");
  if (pos==string::npos)
    return "";
  return str.substr(0,pos);
}

string
scoretest(const string& str)
{
  size_t pos=str.find(":");
  if (pos==string::npos)
    return str;
  return str.substr(0,pos);
}

string
scorebasename(const string& str)
{
  size_t pos=str.rfind(":");
  if (pos==string::npos)
    return str;
  return str.substr(pos+1,str.size()-pos);
}

