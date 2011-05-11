
// dbdate.cpp
// DBdate class definition
// Copyright (c) 2007-2010 by The VoxBo Development Team

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

#include "dbdate.h"
#include <string.h>

/* Default constructor sets the current time. */
DBdate::DBdate() : sec_unix(0)
{

}

DBdate::DBdate(const DBdate& date_ref)
{
  sec_unix = date_ref.sec_unix;
}

/* This constructor accepts an int32 argument as unix time stamp. */
DBdate::DBdate(int32 int_in)
{
  sec_unix = int_in;
}

/* Constructor that sets date only. */
DBdate::DBdate(uint32 month, uint32 day, uint32 year)
{
  sec_unix = 0;
  setDate(month, day, year);
}

/* Constructor that sets certain date and time. */
DBdate::DBdate(uint32 month, uint32 day, uint32 year, uint32 hour, uint32 minute, uint32 second)
{
  sec_unix = 0;
  setDate(month, day, year);
  setTime(hour, minute, second);
}

/* Set the number of seconds since 01/01/1970, GMT. Negative value is acceptable. */
void DBdate::setUnixTime(int32 int_in)
{
  sec_unix = int_in;
}

/* Set a new date. 
 * Note that we have to adjust sec_unix to make sure it is the real local time. 
 * Suppose the current date is 12/10/2008 and there is no adjustment, when we set date to 6/1/2008,
 * the local time will be 1:00:00 on 6/1/2008, instead of 00:00:00 on 6/12/2008. */
void DBdate::setDate(uint32 month, uint32 day, uint32 year)
{
  struct tm dateinfo = *localtime(&sec_unix);
  int32 dst_org = dateinfo.tm_isdst;
  dateinfo.tm_year = year - 1900;
  dateinfo.tm_mon = month - 1;
  dateinfo.tm_mday = day;
  sec_unix = mktime(&dateinfo);

  // daylight saving time adjustment 
  int32 dst_now = dateinfo.tm_isdst;
  sec_unix += (dst_org - dst_now) * 3600;
}

/* Set hour, minute and second only. 
 * Daylight saving time on/off is considered too. */
void DBdate::setTime(uint32 hour, uint32 minute, uint32 second)
{
  struct tm dateinfo = *localtime(&sec_unix);
  int32 dst_org = dateinfo.tm_isdst;
  dateinfo.tm_hour = hour;
  dateinfo.tm_min = minute;
  dateinfo.tm_sec = second;
  sec_unix = mktime(&dateinfo);
  
  int32 dst_now = dateinfo.tm_isdst;
  sec_unix += (dst_org - dst_now) * 3600;
}

/* Set both date and time to current. */
void DBdate::setCurrent()
{
  sec_unix = time(NULL);
}

/* Set timestamp to current date (hour/minute/second are not modified). */
void DBdate::setCurrentDate()
{
  struct tm dateinfo = *localtime(&sec_unix);
  int32 dst_org = dateinfo.tm_isdst;

  time_t sec_now = time(NULL);
  struct tm date_now = *localtime(&sec_now);

  dateinfo.tm_year = date_now.tm_year;
  dateinfo.tm_mon = date_now.tm_mon;
  dateinfo.tm_mday = date_now.tm_mday;

  sec_unix = mktime(&dateinfo);
  int32 dst_now = dateinfo.tm_isdst;
  sec_unix += (dst_org - dst_now) * 3600;
}

/* Set timestamp to current time (month/day/year are not modified). */
void DBdate::setCurrentTime()
{
  struct tm dateinfo = *localtime(&sec_unix);
  int32 dst_org = dateinfo.tm_isdst;

  time_t sec_now = time(NULL);
  struct tm date_now = *localtime(&sec_now);

  dateinfo.tm_hour = date_now.tm_hour;
  dateinfo.tm_min = date_now.tm_min;
  dateinfo.tm_sec = date_now.tm_sec;

  sec_unix = mktime(&dateinfo);
  int32 dst_new = dateinfo.tm_isdst;
  sec_unix += (dst_org - dst_new) * 3600;
}

/* Reset unix timestamp to 0. */
void DBdate::clear()
{
  sec_unix = 0;
}

/* Return number of second since 01/01/1970 GMT. */
int32 DBdate::getUnixTime() const
{
  return sec_unix;
}

/* Return the date in string format. The final '\n' is removed. */
const char* DBdate::getDateStr() const
{
  char* foo = ctime(&sec_unix);
  int len = strlen(foo);
  foo[len - 1] = '\0';
  
  return foo;
}

/* Return "second" part of date, range is [0-60]. */
uint32 DBdate::getSecond() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_sec;
}

/* Return "minute" part of date, range is [0-59]. */
uint32 DBdate::getMinute() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_min;
}

/* Return "hour" part of date, range is [0-23]. */
uint32 DBdate::getHour() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_hour;
}

/* Return day of month, range is [1-31]. */
uint32 DBdate::getDay() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_mday;
}

/* Return month, range is [1-12]. 
 * Note that tm_mday in struct tm is between 0 and 11. */
uint32 DBdate::getMonth() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_mon + 1;
}

/* Return the year value in four digit format.
 * Note that tm_year in struct tm is substracted by 1900. */
uint32 DBdate::getYear() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_year + 1900;
}

/* Return the day in a week. Range is [0-6], starting from Sunday. */
uint32 DBdate::getWday() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_wday;
}

/* Return day in a year. Rnage is [1-366], starting from January 1st.
 * Note that the range of tm_yday in struct tm is [0-365]. */
uint32 DBdate::getYday() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_yday + 1;
}

/* Return daylight saving time offset. Possible values are -1, 0, 1. */
int32 DBdate::getDST() const
{
  struct tm dateinfo = *localtime(&sec_unix);
  return dateinfo.tm_isdst;
}

/* Return number of years since a reference time point. The argument is birth date.
 * If the birth date is later than the object's time stamp, return a time interval that is negative. */  
int32 DBdate::getAge(const DBdate& birthday) const
{
  time_t start_sec = birthday.sec_unix;
  time_t end_sec = sec_unix;
  if (end_sec < start_sec) {
    start_sec = sec_unix;
    end_sec = birthday.sec_unix;
  }

  struct tm start_info = *localtime(&start_sec);
  struct tm end_info = *localtime(&end_sec);
  int32 year_diff = end_info.tm_year - start_info.tm_year;
  int32 mon_diff = end_info.tm_mon - start_info.tm_mon;
  int32 day_diff = end_info.tm_mday - start_info.tm_mday;
  if (!year_diff)
    return 0;

  if ( mon_diff < 0 || (mon_diff == 0 && day_diff < 0) )
    return --year_diff;

  return year_diff;
}

/* Overloaded assignment operator */
DBdate&  DBdate::operator=(const DBdate& date_ref)
{
  sec_unix = date_ref.sec_unix;
  return *this;
}

/* Overloaded "-" operator returns the number of seconds between two DBdate object. */
int32 DBdate::operator-(const DBdate& date_ref) const
{
  int32 diff_sec = sec_unix - date_ref.getUnixTime();
  return diff_sec;
}

/* Overloaded "==" operator */
bool DBdate::operator==(const DBdate& date_ref) const
{
  return (sec_unix == date_ref.sec_unix);
}

/* Overloaded "!=" operator */
bool DBdate::operator!=(const DBdate& date_ref) const
{
  return (sec_unix != date_ref.sec_unix);
}

/* Overloaded "<" operator */
bool DBdate::operator<(const DBdate& date_ref) const
{
  return (sec_unix < date_ref.sec_unix);
}

/* Overloaded "<=" operator */
bool DBdate::operator<=(const DBdate& date_ref) const
{
  return (sec_unix <= date_ref.sec_unix);
}

/* Overloaded ">" operator */
bool DBdate::operator>(const DBdate& date_ref) const
{
  return (sec_unix > date_ref.sec_unix);
}

/* Overloaded ">=" operator */
bool DBdate::operator>=(const DBdate& date_ref) const
{
  return (sec_unix >= date_ref.sec_unix);
}
