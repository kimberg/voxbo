
// koshutil.cpp
// stuff used only in stand_alone
// Copyright (c) 1998-2003 by The VoxBo Development Team

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
// code contributed by Kosh Banerjee and Thomas King

using namespace std;

#include "vbutil.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <utime.h>
#include <ctype.h>
#include <pwd.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>
#include <iostream>
#include <fstream>
#include "koshutil.h"

/*********************************************************************
* This function is used to make sure that the input string fileName  *
* ends in ".tes".                                                    *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* fileName           string          string object holding the file  *
*                                    name.                           *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* N/A                 bool            true is fileName ends in       *
*                                     ".tes", false otherwise.       *
*********************************************************************/
bool validateTesFileName(const string fileName)
{
  if ( (fileName.at(fileName.length() - 1) != 's') ||
       (fileName.at(fileName.length() - 2) != 'e') ||
       (fileName.at(fileName.length() - 3) != 't') ||
       (fileName.at(fileName.length() - 4) != '.') )
  {
    return false;
  } // if

  return true;

} // bool validateTesFileName(const string fileName)

/*********************************************************************
* This function prints out usage information (to stderr) for a       *
* program and then exits with the specified exit value. It is meant  *
* to be called from a "main" program. All parameters for this        *
* function, except for the first, are meant to be C-style strings,   *
* i.e., "const char *". The first parameter is the desired exit      *
* value, the second parameter must be the program name (when called  *
* from a "main" program, just use argv[0]  and progName must not be  *
* of type "const char *" but of type "char *" because basename()     *
* takes an argument of type "char *"), the third parameter should be *
* a short description of the program, the fourth parameter should be *
* a string with basic usage information, and the last parameter      *
* *must* be the empty string (""). The fifth through the             *
* penultimate parameters should be one line descriptions of each     *
* option used by the program. Here's an example call:                *
*                                                                    *
* genusage(1, argv[0], "-Adds two integers and displays the sum.",   *
*         "-i[first integer] -j[second integer]",                    *
*         "-i <first integer>        Specify 1st int. Required.",    *
*         "-j <second integer>       Specify 2nd int. Required.",    *
*         ""); // Note the empty string as the last parameter.       *
*                                                                    *
* Here's is the output from the example call:                        *
*                                                                    *
* main -Adds two integers and displays the sum.                      *
*                                                                    *
* usage: main -i[first integer] -j[second integer]                   *
* flags:                                                             *
* -i <first integer>        Specify 1st int. Required.               *
* -j <second integer>       Specify 2nd int. Required.               *
*********************************************************************/
void genusage(const unsigned short exitValue, char *progName, const char *desc, const char *basicInfo, ...)
{

/*********************************************************************
* A va_list variable is required to process the variable number of   *
* parameters used by this function.                                  *
*********************************************************************/
  va_list argp;

/*********************************************************************
* str will point to parameters for this function.                    *
*********************************************************************/
  const char *str;

/*********************************************************************
* Calling va_start() to initialize argp to be used by va_arg() and   *
* va_end(). The second argument to va_start() must be the last known *
* argument passed to this function.                                  *
*********************************************************************/
  va_start(argp, basicInfo);

/*********************************************************************
* Now printing out the program name, its short description, and its  *
* basic usage information to stderr.                                 *
*********************************************************************/
  cout << endl << xfilename(progName) << " ";
  cout << desc << endl << endl;
  cout << "usage: " << xfilename(progName) << " " << basicInfo << endl;
  cout << "flags:" << endl;

/*********************************************************************
* The following while loop is used to print out the one line         *
* descriptions of the options used by the program. Program flow will *
* leave the while loop when the empty string is encountered.         *
*********************************************************************/
  while ( (str = va_arg(argp, const char *)) )
  {
    if (strlen(str) == 0)
    {
      break;
    } // if
    cout << str << endl;
  } // while

/*********************************************************************
* Now calling va_end() since we are done with processing the         *
* variable list of arguments.                                        *
*********************************************************************/
  va_end(argp);

/*********************************************************************
* Now exiting from the program.                                      *
*********************************************************************/
  exit(exitValue);

} // void genusage(const unsigned short exitValue, char *progName, const char *desc, const char *basicInfo, ...)

/*********************************************************************
* This function processes the command line options passed to the     *
* main function.                                                     *
*                                                                    *
* INPUTS:                                                            *
* -------                                                            *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* argc               int             argc from main.                 *
* argv[]             char **         argv[] from main.               *
* options            char *          The list of valid command line  *
*                                    options, as a C-style string,   *
*                                    e.g., ":ab:c:h".                *
* format             char *          A C-style string that specifies *
*                                    the type for each command line  *
*                                    option.                         *
*                                                                    *
* The allowable characters for the format string are:                *
*                                                                    *
*  b ==> bool                                                        *
*  c ==> char                                                        *
*  C ==> unsigned char                                               *
*  s ==> short                                                       *
*  u ==> unsigned short                                              *
*  i ==> int                                                         *
*  I ==> unsigned int                                                *
*  l ==> long                                                        *
*  L ==> unsigned long                                               *
*  f ==> float                                                       *
*  d ==> double                                                      *
*  S ==> char[] (A C-style string)                                   *
*  Z ==> C++ string object                                           *
*                                                                    *
*  The remaining inputs to processOpts() are pointers to variables   *
*  of the appropriate type.                                          *
*                                                                    *
*  Say that options is ":ab:c:h". This means that the allowable      *
*  command line options for main are "-a", "-b", "-c", and "-h", and *
*  further, options "-b" and "-c" require arguments. Therefore, the  *
*  program could possibly be called from the command line as:        *
*                                                                    *
*      myprog -a -b23 -c"ABCDEFG" -h                                 *
*                                                                    *
*  For those command line options that do not require an argument    *
*  ("-a" and "-h" above), they are treated as flags and are viewed by*
*  processOpts() as being boolean variables. Therefore, considering  *
*  the above example, the "argument" for "-a" is of type bool, the   *
*  argument for "-b" is of type short, the argument for "-c" is of   *
*  type C-style string, and the "argument" for "-h" is also of type  *
*  bool. This implies that the call to processOpts() must be:        *
*                                                                    *
*    processOpts(argc, argv, ":ab:c:h", "bsSb", &my_bool1, &my_int,  *
*    my_string, &my_bool2);                                          *
*                                                                    *
*  where:                                                            *
*    my_bool1    - previously declared bool variable.                *
*    my_int      - previously declared short variable.               *
*    my_string   - previously declared char[] variable.              *
*    my_bool2    - previously declared bool variable.                *
*                                                                    *
* NOTE: The options parameter to processOpts() is ":ab:c:h" and it   *
*       implies that there are 4 distinct possible options for       *
*       main(). This means that the parameter format must contain    *
*       exactly 4 characters, and that the format parameter is then  *
*       followed by 4 pointer variables.                             *
*                                                                    *
* OUTPUTS:                                                           *
* --------                                                           *
* The pointer input variables (those input variables after           *
* char *format) have values assigned to them, as determined from     *
* argv (by getopt()).                                                *
*********************************************************************/
void processOpts(int argc, char *argv[], const char *options, const char *format, ...)
{

/*********************************************************************
* Declaring local variables of type pointer to each of the allowed   *
* primitive types (within the context of this function, a C-style    *
* string and a C++ string object, are considered a primitive types). *
* Also, initializing each of these pointers to NULL, except for      *
* S_C_string, which is set to the empty string.                      *
*********************************************************************/
  bool *b_bool = NULL;
  char *c_char = NULL;
  unsigned char *C_unsigned_char = NULL;
  short *s_short = NULL;
  unsigned short *u_unsigned_short = NULL;
  int *i_int = NULL;
  unsigned int *I_unsigned_int = NULL;
  long *l_long = NULL;
  unsigned long *L_unsigned_long = NULL;
  float *f_float = NULL;
  double *d_double = NULL;
  string *cpp_string = NULL;
  char S_C_string[OPT_STRING_LENGTH];
  memset (S_C_string, 0, OPT_STRING_LENGTH);

/*********************************************************************
* Now declaring an array of type optInfo structs to hold information *
* about the command line options used by the program. Note that the  *
* length of the array is equal to the number of available options    *
* for the program, which also equals the length of char *format.     *
*********************************************************************/
  optInfo optInfoArr[strlen(format)];

/*********************************************************************
* optCount is used to count the number of options in char *options.  *
* Basically, this means count all the characters in char *options    *
* except ':'. These two numbers must be equal to each other.         *
*********************************************************************/
  unsigned int optCount = 0;

/*********************************************************************
* The following for lop is used to count the number of options. The  *
* for loop is started at 1 (instead of 0) because the first character*
* of char *options[] is always ':'.                                  *
*********************************************************************/
  for (unsigned int i = 1; i < strlen(options); i++)
  {

/*********************************************************************
* If options[i] is not ':', then optCount is incremented.            *
*********************************************************************/
    if (options[i] != ':')
    {
      optCount++;
    } // if
  } // for

/*********************************************************************
* If the number of options does not equal the number of format       *
* specifiers, then an error message is printed and this program      *
* exits. Here is an example when the number of options does not equal*
* the number of format specifiers:                                   *
*                                                                    *
* char *options is: ":ab:c:" (number of options = 3)                 *
* char *format is: "bfdS" (number of format specifiers = 4)          *
*********************************************************************/
  if (optCount != strlen(format))
  {
    ostringstream errorMsg;
    errorMsg << "FUNCTION [" << __FUNCTION__ << "]. The number of options ["
    << optCount << "] does not equal the number of format specifiers ["
    << strlen(format) << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    exit(1);
  } // if

/*********************************************************************
* The following for loop is used to initialize the the array         *
* optInfoArr[]. i is used to index the array and j is used to index  *
* char *options. Recall that the options string for getopt() always  *
* has ':' as the first character. This is why j is initialized to 1  *
* and not zero.                                                      *
*********************************************************************/
  unsigned int j = 1;
  for (unsigned int i = 0; i < strlen(format); i++)
  {

/*********************************************************************
* Now assigning the name of the option, e.g., if char *options is    *
* ":a:bc:h", then the option names are 'a', 'b', 'c', and 'h', to the*
* optName field.                                                     *
*********************************************************************/
    optInfoArr[i].optName = options[j];

/*********************************************************************
* If j is less than (strlen(options) - 1), this means that options[j]*
* is not the last character in char *options[]. Therefore,           *
* options[j+1] can be accessed. We want to access options[j+1] to see*
* if it is ':', in which case, the option requires an argument.      *
*********************************************************************/
    if ( (j < (strlen(options) - 1)) && (options[j+1] == ':') )
    {
       optInfoArr[i].requiredArg = true;

/*********************************************************************
* Incrementing j because we know that the next character in          *
* char *options is ':'.                                              *
*********************************************************************/
      j++;
    } // if
    else
    {
/*********************************************************************
* If program flow ends up here, then we know that an argument is not *
* required for this option.                                          *
*********************************************************************/
      optInfoArr[i].requiredArg = false;
    } // else

/*********************************************************************
* Incrementing j so the next character in char *options can be       *
* accessed.                                                          *
*********************************************************************/
    j++;

/*********************************************************************
* Now assigning the type information to the typeArg field and setting*
* optPresent to the default value of false.                          *
*********************************************************************/
    optInfoArr[i].typeArg = format[i];
    optInfoArr[i].optPresent = false;

  } // for i

/*********************************************************************
* A va_list variable is required to process the variable list of     *
* parameters used by this function. optChar will hold the output from*
* getopt().                                                          *
*********************************************************************/
  va_list ap;
  char optChar;

/*********************************************************************
* The following while loop is used to traverse argv and process the  *
* command line options for this program. After the last command line *
* option is encountered, getopt() will return -1.                    *
*********************************************************************/
  while ((optChar = getopt(argc, argv, options)) != -1)
  {

/*********************************************************************
* If optChar is not a valid command line option, then an error       *
* message is printed out and then this program exits.                *
*********************************************************************/
    if (!validateOptChar(optChar, options))
    {
      printErrorMsg(VB_ERROR, "Invalid command line option used.");
      exit(1);
    } // if

/*********************************************************************
* Now optChar holds one of the command line parameters. The following*
* for loop is used to determine which one. The array optInfoArr[] is *
* traversed until the correct command line character is encountered. *
*********************************************************************/
    for (unsigned int i = 0; i < strlen(format); i++)
    {

/*********************************************************************
* If optInfoArr[i].optName is equal to optChar, then we have found   *
* the correct struct in the array optInfoArr[].                      *
*********************************************************************/
      if (optInfoArr[i].optName == optChar)
      {

/*********************************************************************
* If optInfoArr[i].optName requires an argument, then go ahead and   *
* copy optarg into optInfoArr[i].optArg.                             *
*********************************************************************/
        if (optInfoArr[i].requiredArg)
        {
					optInfoArr[i].optArg = optarg;
        } // if

/*********************************************************************
* If program flow ends up here, then optInfoArr[i].optName does not  *
* require an argument. Therefore, optInfoArr[i].optName is treated   *
* as a flag, i.e., a boolean variable, and by default, it is set to  *
* true.                                                              *
*********************************************************************/
        else
        {
					optInfoArr[i].optArg = "true";
        } // else

/*********************************************************************
* Since we have found the appropriate command line option in         *
* optInfoArr[], optInfoArr[i].optPresent is set to true. Also, we    *
* out of the preceding for loop.                                     *
*********************************************************************/
        optInfoArr[i].optPresent = true;
        break;
      } // if
    } // for i

  } // while

/*********************************************************************
* The following for loop is used to print out the contents of        *
* optInfoArr[], used for debugging. Normally, it should be commented *
* out.                                                               *
*********************************************************************/
//  for (int i = 0; i < strlen(format) ; i++)*/
//  {*/
//    printOptInfo(optInfoArr[i]);*/
//  } // for i*/

/*********************************************************************
* Calling va_start() to initialize ap to be used by va_arg() and     *
* va_end(). The second argument to va_start() must be the last known *
* argument passed to this function.                                  *
*********************************************************************/
  va_start(ap, format);

/*********************************************************************
* The following for loop is used to actually assign the proper values*
* to the variable list of pointer parameters passed into this        *
* function. This for loop traverses optInfoArr[].                    *
*********************************************************************/
  for (unsigned int i = 0; i < strlen(format); i++)
  {

/*********************************************************************
* If the option is actually present in argv, then go ahead and       *
* process it, i.e., determine its type and assign the option the     *
* appropriate value.                                                 *
*********************************************************************/

/*********************************************************************
* The following "if" is used to see if the option is actually        *
* present. If it is, then a switch block is used to process the      *
* option.                                                            *
*********************************************************************/
    if (optInfoArr[i].optPresent)
    {

/*********************************************************************
* The following switch block is used to determine the type of the    *
* command line option.                                               *
*********************************************************************/
      switch(optInfoArr[i].typeArg)
      {
        case 'b':

/*********************************************************************
* This case block is for bool. va_arg(ap, bool *) will return the    *
* appropriate pointer to one of the boolean pointers from the        *
* variable list of parameters passed to this function and it gets    *
* stored in b_bool. Then b_bool is assigned true (which is the       *
* default for these types of "flag" variables).                      *
*********************************************************************/
          b_bool = va_arg(ap, bool *);
          (*b_bool) = true;
        break;

        case 'c':

/*********************************************************************
* This case block is for char. va_arg(ap, char *) will return the    *
* appropriate pointer to one of the char pointers from the           *
* variable list of parameters passed to this function and it gets    *
* stored in c_char. Then c_char is assigned the value from           *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          c_char = va_arg(ap, char *);
          (*c_char) = optInfoArr[i].optArg[0];
        break;

        case 'C':

/*********************************************************************
* This case block is for unsigned char. va_arg(ap, unsigned char *)  *
* will return the appropriate pointer to one of the unsigned char    *
* pointers from the variable list of parameters passed to this       *
* function and it gets stored in C_unsigned_char. Then               *
* C_unsigned_char is assigned assigned the value from                *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          C_unsigned_char = va_arg(ap, unsigned char *);
          (*C_unsigned_char) = optInfoArr[i].optArg[0];
        break;

        case 's':

/*********************************************************************
* This case block is for short. va_arg(ap, short *) will return the  *
* appropriate pointer to one of the short pointers from the          *
* variable list of parameters passed to this function and it gets    *
* stored in s_short. Then s_short is assigned the value from         *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          s_short = va_arg(ap, short *);
          (*s_short) = (short) strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'u':

/*********************************************************************
* This case block is for unsigned short. va_arg(ap, unsigned short *)*
* will return the appropriate pointer to one of the unsigned short   *
* pointers from the variable list of parameters passed to this       *
* function and it gets stored in u_unsigned_short. Then              *
* u_unsigned_short is assigned assigned the value from               *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          u_unsigned_short = va_arg(ap, unsigned short *);
          (*u_unsigned_short) = (unsigned short) strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'i':

/*********************************************************************
* This case block is for int. va_arg(ap, int *) will return the      *
* appropriate pointer to one of the int pointers from the            *
* variable list of parameters passed to this function and it gets    *
* stored in i_int. Then i_int is assigned the value from             *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          i_int = va_arg(ap, int *);
          (*i_int) = strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'I':

/*********************************************************************
* This case block is for unsigned int. va_arg(ap, unsigned int *)    *
* will return the appropriate pointer to one of the unsigned int     *
* pointers from the variable list of parameters passed to this       *
* function and it gets stored in I_unsigned_int. Then                *
* I_unsigned_int is assigned assigned the value from                 *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          I_unsigned_int = va_arg(ap, unsigned int *);
          (*I_unsigned_int) = (unsigned int) strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'l':

/*********************************************************************
* This case block is for long. va_arg(ap, long *) will return the    *
* appropriate pointer to one of the long pointers from the           *
* variable list of parameters passed to this function and it gets    *
* stored in l_long. Then l_long is assigned the value from           *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          l_long = va_arg(ap, long *);
          (*l_long) = strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'L':

/*********************************************************************
* This case block is for unsigned long. va_arg(ap, unsigned long *)  *
* will return the appropriate pointer to one of the unsigned long    *
* pointers from the variable list of parameters passed to this       *
* function and it gets stored in L_unsigned_long. Then               *
* L_unsigned_long is assigned assigned the value from                *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          L_unsigned_long = va_arg(ap, unsigned long *);
          (*L_unsigned_long) = (unsigned long) strtol(optInfoArr[i].optArg.c_str());
        break;

        case 'f':

/*********************************************************************
* This case block is for float. va_arg(ap, float *) will return the  *
* appropriate pointer to one of the float pointers from the          *
* variable list of parameters passed to this function and it gets    *
* stored in f_float. Then f_float is assigned the value from         *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          f_float = va_arg(ap, float *);
          (*f_float) = (float) atof(optInfoArr[i].optArg.c_str());
        break;

        case 'd':

/*********************************************************************
* This case block is for double. va_arg(ap, double *) will return the*
* appropriate pointer to one of the double pointers from the         *
* variable list of parameters passed to this function and it gets    *
* stored in d_double. Then d_double is assigned the value from       *
* optInfoArr[i].optArg[0].                                           *
*********************************************************************/
          d_double = va_arg(ap, double *);
          (*d_double) = (float) atof(optInfoArr[i].optArg.c_str());
        break;

        case 'S':

/*********************************************************************
* This case block is for char[]. va_arg(ap, char *) will return the  *
* appropriate pointer to one of the char[] pointers from the         *
* variable list of parameters passed to this function. Then strcpy() *
* is used to store optInfoArr[i].optArg in the char[] pointer.       *
*********************************************************************/
          strcpy(va_arg(ap, char *), optInfoArr[i].optArg.c_str());
        break;

        case 'Z':

/*********************************************************************
* This case block is for string. va_arg(ap, string *) will return    *
* the appropriate pointer to one of the string pointers from the     *
* variable list of parameters passed to this function and it gets    *
* stored in cpp_string. Then cpp_string is assigned                  *
* string(optInfoArr[i].optArg).                                      *
*********************************************************************/
          cpp_string = va_arg(ap, string *);
          (*cpp_string) = optInfoArr[i].optArg;
        break;

        default:
          ostringstream errorMsg;
          errorMsg << "FUNCTION [" << __FUNCTION__ << "]. Unknown primitive type for processOpts: ["
          << optInfoArr[i].typeArg << "] in option number [" << i << "]. Option name: [" << optInfoArr[i].optName << "] Exiting.";
          printErrorMsgAndExit(VB_ERROR, errorMsg.str(), 1);
        break;

      } // switch

    } // if

/*********************************************************************
* If program flow ends up here, then the option is not present, so   *
* we need to skip over the corresponding variable in the variable    *
* list of arguments.                                                 *
*********************************************************************/
    else
    {
      (void ) va_arg(ap, void *);
    } // else

  } // for i

/*********************************************************************
* Now calling va_end() since we are done with processing the         *
* variable list of arguments.                                        *
*********************************************************************/
  va_end(ap);


} // void processOpts(int argc, char *argv[], char *options, char *format, ...)

/*********************************************************************
* This function is used to make sure that the input character opt    *
* appears in the list of characters pointed to by *options. If opt   *
* is not present in *options, then false is returned. Otherwise true *
* is returned.                                                       *
*********************************************************************/
bool validateOptChar(const char opt, const char *options)
{
  for (unsigned int i = 0; i < strlen(options); i++)
  {
    if (opt == options[i])
    {
      return true;
    } // if

  } // for i

/*********************************************************************
* If program flow ends up here, then opt is not present in options[].*
* Therefore, false is returned.                                      *
*********************************************************************/
  return false;

} // bool validateOptChar(const char opt, const char *options)

/*********************************************************************
* This function uses the input regular expression to validate the    *
* input string. If the input regular expression matches the input    *
* string, then true is returned. Otherwise, false is returned.       *
* This function is derived from mygrep.c by:                         *
* Ben Tindale <ben@bluesat.unsw.edu.au>                              *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* regularExp         const char *    The regular expression, as a    *
*                                    C-style string.                 *
* theString          const char *    The C-style string to be        *
*                                    validated.                      *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* true                bool            If the string is valid.        *
* false               bool            If the string is invalid.      *
*********************************************************************/
bool validateString(const char *regularExp, const char *theString)
{

/*********************************************************************
* Declaring and clearing regex. regex will hold the "compiled"       *
* regular expression.                                                *
*********************************************************************/
  regex_t regex;
  memset(&regex, 0, sizeof(regex_t));

/*********************************************************************
* Now compiling the regular expression. err will hold the return     *
* value from the call to regcomp(). If err is not zero, then an error*
* occurred.                                                          *
*********************************************************************/
  int err = 0;
  if ( (err = regcomp(&regex, regularExp, 0)) != 0 )
  {

/*********************************************************************
* If program flow ends up here, then an error occurred when trying   *
* to compile the regular expression. The first call to regerror() is *
* used to get the length of a NULL terminated C-style string         *
* sufficiently long enough to hold the error message.                *
*********************************************************************/
    int len = regerror(err, &regex, NULL, 0);

/*********************************************************************
* regexErr[] is used to hold the error message. regerror() is then   *
* called (for a second time) to populate regexErr[] with the error   *
* message.                                                           *
*********************************************************************/
    char regexErr[len];
    regerror(err, &regex, regexErr, len);

/*********************************************************************
* The regular expression compilation error message is printed out and*
* then this program exits.                                           *
*********************************************************************/
    ostringstream errorMsg;
    errorMsg << "Compiling regular expression: [" << regexErr << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
    exit(1);

  } // if

  size_t no_sub = 1;
  regmatch_t *result;
  if((result = (regmatch_t *) malloc(sizeof(regmatch_t) * no_sub))==0)
  {
    perror("Unable to allocate memory for a regmatch_t.");
    exit(1);
  } // if

  if (regexec(&regex, theString, no_sub, result, REG_NOSUB) == 0)
  {
    regfree(&regex);
    return true;
  } // if

  regfree(&regex);
  return false;

} // bool validateString(const char *regularExp, const char *theString)

/*********************************************************************
* This function uses the input regular expression to validate the    *
* input string. If the input regular expression matches the input    *
* string, then true is returned. Otherwise, false is returned.       *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* regularExp         const char *    The regular expression, as a    *
*                                    C-style string.                 *
* theString          const string    The string object to be         *
*                                    validated.                      *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* true                bool            If the string is valid.        *
* false               bool            If the string is invalid.      *
*********************************************************************/
bool validateString(const char *regularExp, const string theString)
{

/*********************************************************************
* Returing the value of validateString(const char *, const char *).  *
*********************************************************************/
  return validateString(regularExp, theString.c_str());

} // bool validateString(const char *regularExp, const string theString)

/*********************************************************************
* This function modifies the input C-style string with another       *
* C-style string meant to be used as a unique temporary file name.   *
* The last 6 characters of the input C-style string must be "XXXXXX".*
* The sequence of "XXXXXX" are the characters that get modified to   *
* create a unique temporary file name. The input variable length is  *
* the length of the array tempFile[]. NOTE: The input variable length*
* is *not* the string length of tempFile[]. Unpon success, 0 is      *
* returned. Otherwise, -1 is returned.                               *
*********************************************************************/
int getTempFileName(char *tempFile, const size_t length)
{

/*********************************************************************
* Declaring a needed integer constant.                               *
*********************************************************************/
  const size_t SIX = 6;

/*********************************************************************
* It is expected that the last 6 characters of tempFile[] are        *
* "XXXXXX". We now check to see if this is indeed the case. If not,  *
* then "XXXXXX" will be appended to tempFile[]. This assumes that    *
* tempFile[] has sufficient space remaining to append the 6 X's.     *
*                                                                    *
* last6[] is declared and cleared. Then the last 6 charactes from    *
* tempFile[] are copied to it, if the string length of tempFile is   *
* >= 6.                                                              *
*********************************************************************/
  char last6[SIX + 1];
  memset(last6, 0, SIX + 1);
  if (strlen(tempFile) >= SIX)
  {
    strncpy(last6, tempFile + (strlen(tempFile) - SIX), SIX);
  } // if

/*********************************************************************
* Checking to see if the last 6 characters of tempFile[] are         *
* "XXXXXX".                                                          *
*********************************************************************/
  if (strcmp(last6, "XXXXXX") != 0)
  {

/*********************************************************************
* If tempFile[] has room for 6 more characters, then "XXXXXX" is     *
* appended to it. NOTE: This may result in tempFile[] having more    *
* then 6 X's at the end.                                             *
*********************************************************************/
    if ( (strlen(tempFile) + SIX) < length )
    {
      strcat(tempFile, "XXXXXX");
    } // if

/*********************************************************************
* If program flow ends up here, then tempFile[] does not have room   *
* for 6 more characters. Therefore, an error message is sent to      *
* stderr and -1 is returned.                                         *
*********************************************************************/
    else
    {
      ostringstream errorMsg;
      errorMsg << "Line Number [" << __LINE__ << "] File: [" << __FILE__
      << "] Function: [" << __FUNCTION__ << "] Invalid tempFile ["
      << tempFile << "] for temporary file name. insufficient space to append \"XXXXXX\".";
      printErrorMsg(VB_ERROR, errorMsg.str());
      return -1;
    } // else

  } // if

/*********************************************************************
* errno is set to 0. If the call to mkstemp() has an error, errno    *
* will be set to the appropriate error code.                         *
*********************************************************************/
  errno = 0;

/*********************************************************************
* If an error occurs in mkstemp(), then -1 will be returned. In that *
* case, an appropriate error message will be sent to stderr, as      *
* determined by strerror(). NOTE: mkstemp(), upon success, returns   *
* an open file descriptor and creates an empty file. The file        *
* descriptor is stored in fd. ALSO NOTE: If the user does not have   *
* write permission in the current directory, mkstemp() will fail.    *
*********************************************************************/
  int fd = mkstemp(tempFile);
  if (fd == -1)
  {

/*********************************************************************
* The apropriate error message is sent to stderr.                    *
*********************************************************************/
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] File: [" << __FILE__
    << "] Function: [" << __FUNCTION__ << "] Unable to create temporary file name from tempFile ["
    << tempFile << "] due to: [" << strerror(errno) << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());

/*********************************************************************
* Since an error occurred, -1 is returned.                           *
*********************************************************************/
    return -1;

  } // if

/*********************************************************************
* If program flow ends up here, then we were successful in           *
* creating a temporary file name. Therefore, the file is closed,     *
* deleted, and 0 is returned. If the call to unlink() results in an  *
* error, then an appropriate error message is sent to stderr. errno  *
* is set to zero. If unlink() has an error, errno will be set to a   *
* non-zero value, i.e., the appropriate error code.                  *
*********************************************************************/
  close(fd);
  errno = 0;
  if (unlink(tempFile) == -1)
  {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__ << "] File: [" << __FILE__
    << "] Function: [" << __FUNCTION__ << "] Unable to delete temporary file from tempFile ["
    << tempFile << "] due to: [" << strerror(errno) << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
  } // if
  return 0;

} // int getTempFileName(char *tempFile, const size_t length)

/*********************************************************************
* This function modifies the input string object so that it can be   *
* used as a tempoarary file name.                                    *
*********************************************************************/
int getTempFileName(string& tempFile)
{

/*********************************************************************
* Declaring a needed constant.                                       *
*********************************************************************/
  const int SIX = 6;

/*********************************************************************
* The C-style string part of tempFile will be copied to tempString[] *
* since it is not directly modifiable. tempString[] is declared and  *
* then zeroed out.                                                   *
*********************************************************************/
  char tempString[tempFile.length() + 1 + SIX];
  memset(tempString, 0, tempFile.length() + 1 + SIX);

/*********************************************************************
* The C-style string part of tempFile is copied to tempString[].     *
*********************************************************************/
  strcpy(tempString, tempFile.c_str());

/*********************************************************************
* retValue is used to hold the return value from                     *
* getTempFileName(char *, size_t). If retVal is non-zero, then we    *
* were unsuccessful in creating a unique temporary file name.        *
*********************************************************************/
  int retValue = getTempFileName(tempString, tempFile.length() + 1 + SIX);

/*********************************************************************
* If retVal is 0, then tempString is assigned to tempFile.           *
*********************************************************************/
  if (!retValue)
  {
    tempFile = tempString;
  } // if

/*********************************************************************
* Returning retVal.                                                  *
*********************************************************************/
  return retValue;

} // int getTempFileName(string& tempFile)

short getScreenCols()
{
  return 72;
}
