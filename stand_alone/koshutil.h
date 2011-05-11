
// koshutil.h
// walling off some kosh functions

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
// original version written by Dan Kimberg
// substantial code merged in from Kosh Banerjee

using namespace std;

#ifndef KOSHUTIL_H
#define KOSHUTIL_H

#include <vector>
#include <list>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <regex.h>
#include <unistd.h> // For getopt().
#include <termios.h> // For definition of struct winsize.
#include <sys/ioctl.h> // For definition of struct winsize.
#include <cmath>
#include "vbutil.h"
#include "tokenlist.h"
#include "genericexcep.h"
#include "vbversion.h"

/*********************************************************************
* Some misc. utility function prototypes.                            *
*********************************************************************/
bool validateTesFileName(const string fileName);
void genusage(const unsigned short exitValue, char *progName, const char *desc, const char *basicInfo, ...);
void processOpts(int argc,char *argv[], const char *options, const char *format,...);
bool validateOptChar(const char opt, const char *options);

/*********************************************************************
* Prototypes for validating a string using a regular expression.     *
*********************************************************************/
bool validateString(const char *regularExp, const char *theString);
bool validateString(const char *regularExp, const string theString);

/*********************************************************************
* These functions modify the input style string with another string  *
* string meant to be used as a unique temporary file name. The last  *
* 6 characters of the input string must be "XXXXXX".                 *
*********************************************************************/
int getTempFileName(char *tempFile, const size_t length);
int getTempFileName(string& tempFile);

/*********************************************************************
* OPEN_SPATIAL_LOOPS is #define'd.                                   *
*********************************************************************/
#define OPEN_SPATIAL_LOOPS(X) for (int indexZ = 0; indexZ < (X).dimz; indexZ++) \
{ \
for (int indexY = 0; indexY < (X).dimy; indexY++) \
{ \
for (int indexX = 0; indexX < (X).dimx; indexX++) \
{

/*********************************************************************
* OPEN_SPATIAL_LOOPS_PTR is #define'd.                               *
*********************************************************************/
#define OPEN_SPATIAL_LOOPS_PTR(X) for (int indexZ = 0; indexZ < (X)->dimz; indexZ++) \
{ \
for (int indexY = 0; indexY < (X)->dimy; indexY++) \
{ \
for (int indexX = 0; indexX < (X)->dimx; indexX++) \
{

/*********************************************************************
* CLOSE_SPATIAL_LOOPS is #define'd to close off the open braces from *
* OPEN_SPATIAL_LOOPS.                                                *
*********************************************************************/
#define CLOSE_SPATIAL_LOOPS }}}

/*********************************************************************
* Template function for the maximum.                                 *
*********************************************************************/
template<class T, class S> T max(T i, S j)
{
  if (i >= (T ) j)
  {
    return i;
  } // if
  return (T ) j;
} // template<class T, class S> T max(T i, S j)

/*********************************************************************
* Template function for the minimum.                                 *
*********************************************************************/
template<class T, class S> T min(T i, S j)
{
  if (i <= (T ) j)
  {
    return i;
  } // if
  return (T ) j;
} // template<class T, class S> T min(T i, S j)

#endif // KOSHUTIL_H
