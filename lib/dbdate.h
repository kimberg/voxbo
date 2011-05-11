
// dbdate.h
// Date class written to overcome the problems in struct tm
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
// written by Dongbo Hu

#ifndef DBDATE_H
#define DBDATE_H

#include <time.h>
#include "typedefs.h"

using namespace std;

class DBdate {
 public:
  DBdate();               // default constructor sets timestamp to 0
  DBdate(const DBdate&);  // copy constructor
  DBdate(int32 );         // accepts a unix timestamp
  DBdate(uint32, uint32, uint32); // accepts month/day/year and set default hour/min/sec
  DBdate(uint32, uint32, uint32, uint32, uint32, uint32); // accepts month/day/year/hour/min/sec

  void setUnixTime(int32);                   // set unix time directly
  void setDate(uint32, uint32, uint32);      // set month/day/year only
  void setTime(uint32, uint32, uint32 = 0);  // set hour, min and second only
  void setCurrent();
  void setCurrentDate();
  void setCurrentTime();
  void clear();

  int32 getUnixTime() const;      // return unix timestamp
  const char* getDateStr() const; // return readable local time string
  uint32 getSecond() const;       // return second in local time, [0-60] (1 leap second)
  uint32 getMinute() const;       // return minute in local time, [0-59]
  uint32 getHour() const;         // return hour in local time, [0-23]
  uint32 getDay() const;          // return day of month in local time, [1-31]
  uint32 getMonth() const;        // return month in local time, [1-12]
  uint32 getYear() const;         // return year (absolute value in four digits such as 19xx, 20xx)
  uint32 getWday() const;         // return day in a week, [0-6] (Sunday is 0, Saturday is 6) 
  uint32 getYday() const;         // return day of a year , [1-366], (01/01 is 1, 12/31 is 365/366) 
  int32 getDST() const;           // return daylight saving time offset (-1/0/1)
  int32 getAge(const DBdate&) const; // calculate age (number of years) based on input birth date 

  // Some overloaded operators
  DBdate& operator=(const DBdate&);
  int32 operator-(const DBdate&) const;
  bool operator==(const DBdate&) const;
  bool operator!=(const DBdate&) const;
  bool operator<(const DBdate&) const;
  bool operator<=(const DBdate&) const;
  bool operator>(const DBdate&) const;
  bool operator>=(const DBdate&) const;

 private: 
  time_t sec_unix;
};

#endif
