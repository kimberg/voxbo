/*********************************************************************
* Copyright (c) 1998, 1999, 2001, 2002 by Center for Cognitive       *
* Neuroscience, University of Pennsylvania.                          *
*                                                                    *
* This program is free software; you can redistribute it and/or      *
* modify it under the terms of the GNU General Public License,       *
* version 2, as published by the Free Software Foundation.           *
*                                                                    *
* This program is distributed in the hope that it will be useful,    *
* but WITHOUT ANY WARRANTY; without even the implied warranty of     *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
* General Public License for more details.                           *
*                                                                    *
* You should have received a copy of the GNU General Public License  *
* along with this program; see the file named COPYING.  If not,      *
* write to the Free Software Foundation, Inc., 59 Temple Place,      *
* Suite 330, Boston, MA 02111-1307 USA                               *
*                                                                    *
* For general information on VoxBo, including the latest complete    *
* source code and binary distributions, manual, and associated       *
* files, see the VoxBo home page at: http://www.voxbo.org/           *
*                                                                    *
* Original version written by Kosh Banerjee.                         *
* <banerjee@wernicke.ccn.upenn.edu>.                                 *
*********************************************************************/

/*********************************************************************
* Required header file.                                              *
*********************************************************************/
#include "utils.h"

/*********************************************************************
* This function returns the desired header line from the input       *
* vector<string> object, which is expected to be the header of a     *
* VBImage object. If the desired header line does not exist, then    *
* the empty string is returned (as a string object).                 *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* headerKey          const string&   The first field of the desired  *
*                                    header line, used as the key.   *
*                                    Example: "DateCreated:"         *
* theHeader    const vector<string>& The header of a VBImage object. *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* N/A                 string          The desired header line, if    *
*                                     it is found. Otherwise an      *
*                                     empty string object.           *
* NOTE: Only the first occurrence of the header line with            *
*       headerKey is returned.                                       *
*********************************************************************/
string utils::getHeaderLine(const string& headerKey,
const vector<string>& theHeader)
{

/*********************************************************************
* The following for loop is used to examine each header line in      *
* theHeader.                                                         *
*********************************************************************/
  for (unsigned int i = 0; i < theHeader.size(); i++)
  {

/*********************************************************************
* A StringTokenizer object is instantiated so that the first field of*
* the header line can be compared to the input header key. NOTE: A   *
* space and tab are used as field delimiters.                        *
*********************************************************************/
    StringTokenizer S = StringTokenizer(theHeader[i], " \t");

/*********************************************************************
* If the first field of the current header line from theHeader equals*
* the heade rkey, then true is returned.                             *
*********************************************************************/
    if (S.getToken(0) == headerKey)
    {
      return theHeader[i];
    } // if
  } // for i

/*********************************************************************
* If program flow ends up here, then the desired header line was not *
* found. Therefore, the empty string is returned.                    *
*********************************************************************/
  return string("");

} // string utils::getHeaderLine(const string& headerKey,
  // const vector<string>& theHeader)

/*********************************************************************
* This function returns the header tokens of the desired header line,*
* as specified by headerKey. Bascially, this means that the entire   *
* line is returned, except the first token, which is viewed as the   *
* "header key".
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* headerKey          const string&   The first field of the desired  *
*                                    header line, used as the key.   *
*                                    Example: "DateCreated:"         *
* theHeader    const vector<string>& The VBImage object whose header *
*                                    is to be searched for the       *
*                                    desired header line.            *
* theDelim           const string&   This will be used to delimit    *
*                                    the header tokens.              *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:           DESCRIPTION:                   *
* -----------------   -----           ------------                   *
* N/A                 string          The header tokens, if the      *
*                                     header line specified by the   *
*                                     header key is found. Otherwise,*
*                                     the empty string is returned.  *
* NOTE: Only the the tokens from the first occurrence of the header  *
*       line with headerKey are returned.                            *
*********************************************************************/
string utils::getHeaderLineTokens(const string& headerKey,
const vector<string>& theHeader, const string& theDelim)
{

/*********************************************************************
* We now get the appropriate header line.                            *
*********************************************************************/
  string theHeaderLine = utils::getHeaderLine(headerKey, theHeader);

/*********************************************************************
* If we were successful in fetching an appropriate header line, then *
* we go ahead and tokenize it.                                       *
*********************************************************************/
  if (theHeaderLine.size() > 0)
  {

/*********************************************************************
* Tokenizing theHeaderLine, using the sapce and tab characters as    *
* field delimiters.                                                  *
*********************************************************************/
    StringTokenizer S(theHeaderLine, " \t");

/*********************************************************************
* Now we return all the tokens of the header, except the zeroth      *
* token (the header key) and the tokens are delimited by theDelim.   *
*********************************************************************/
    return S.getTokenRange(1, S.getNumTokens() - 1, theDelim);
  } // if

/*********************************************************************
* If program flow ends up here, then theHeaderLine is the empty      *
* string. Therefore, we simply return theHeaderLine.                 *
*********************************************************************/
  return theHeaderLine;

} // string utils::getHeaderLineTokens(const string& headerKey,
  // const vector<string>& theHeader, const string& theDelim)

void utils::printMat(VBMatrix& M) throw()
{

/*********************************************************************
* First, if M.mode is VBMatrix::ondisk, we try to make M in-core. If *
* that fails, a simple error message is printed and then we return.  *
*********************************************************************/
  if ( (M.mode == VBMatrix::ondisk) && (M.MakeInCore()) )
  {
    cerr << "ERROR: Could not make matrix in core." << endl;
    return;
  } // if

/*********************************************************************
* Now calling utils::printGSLMat to print out the values of M.       *
*********************************************************************/
	utils::printGSLMat(&M.mview.matrix);

} // void printMat(VBMatrix& M) throw()

/*********************************************************************
* This function prints the input GSL matrix to stdout. It is         *
* expected this function will be used for debugging purposes.        *
*                                                                    *
* INPUT VARIABLES:   TYPE:              DESCRIPTION:                 *
* ----------------   -----              ------------                 *
* m                  const gsl_matrix*  The input GSL matrix.        *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:             DESCRIPTION:                 *
* -----------------   -----             ------------                 *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void utils::printGSLMat(const gsl_matrix *m) throw()
{

/*********************************************************************
* This for loop is for the row indices.                              *
*********************************************************************/
  for (size_t i = 0; i < m->size1; i++)
  {

/*********************************************************************
* This for loop is for the column indices.                           *
*********************************************************************/
    for (size_t j = 0; j < m->size2; j++)
    {

/*********************************************************************
* If we are on the first column of the current row, then an opening  *
* bracket and a space are printed.                                   *
*********************************************************************/
      if (j == 0)
      {
        cout << "[ ";
      } // if

/*********************************************************************
* Now printing out the matrix element.                               *
*********************************************************************/
      cout << gsl_matrix_get(m, i, j);

/*********************************************************************
* If we are not on the last column of the current row, then a comma  *
* and a space are printed out. Otherwise, a space, a closing bracket,*
* and a new line are printed.                                        *
*********************************************************************/
      if (j != (m->size2 - 1))
      {
        cout << ", ";
      } // if
      else
      {
        cout << " ]" << endl;
      } // else

    } // for j
  } // for i

} // void utils::printGSLMat(const gsl_matrix *m) throw()

/*********************************************************************
* This function prints the input VBMatrix to a file. It is expected  *
* this function will be used for debugging purposes.                 *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* M                  VBMatrix&       The input VBMatrix.             *
*                                                                    *
* matName            const char*     The matrix name.                *
*                                                                    *
* file               const char*     The output file name.           *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void utils::printMatFile(VBMatrix& M, const char *matName, const char *file) throw()
{

/*********************************************************************
* First, if M.mode is VBMatrix::ondisk, we try to make M in-core. If *
* that fails, a simple error message is printed and then we return.  *
*********************************************************************/
  if ( (M.mode == VBMatrix::ondisk) && (M.MakeInCore()) )
  {
    cerr << "ERROR: Could not make matrix in core." << endl;
    return;
  } // if

/*********************************************************************
* Now calling utils::printGSLMatFile to print out the values of M to *
* file.                                                              *
*********************************************************************/
	utils::printGSLMatFile(&M.mview.matrix, matName, file);

} // void utils::printMatFile(VBMatrix& M, const char *matName, const char *file) throw()

/*********************************************************************
* This function prints the input GSL matrix to a file. It is         *
* expected this function will be used for debugging purposes.        *
*                                                                    *
* INPUT VARIABLES:   TYPE:              DESCRIPTION:                 *
* ----------------   -----              ------------                 *
* m                  const gsl_matrix*  The input GSL matrix.        *
*                                                                    *
* matName            const char*        The matrix name.             *
*                                                                    *
* file               const char*        The output file name.        *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:             DESCRIPTION:                 *
* -----------------   -----             ------------                 *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void utils::printGSLMatFile(const gsl_matrix *m, const char *matName, 
const char *file) throw()
{

/*********************************************************************
* Opening the file for writing.                                      *
*********************************************************************/
  FILE *outFile = fopen(file, "w");

/*********************************************************************
* Writing the matrix name.                                           *
*********************************************************************/
  fprintf(outFile, "%s BEGIN\n", matName);

/*********************************************************************
* Traversing the rows of the matrix.                                 *
*********************************************************************/
  for (size_t i = 0; i < m->size1; i++)
  {
/*********************************************************************
* Traversing the columns of the matrix.                              *
*********************************************************************/
    for (size_t j = 0; j < m->size2; j++)
    {
/*********************************************************************
* If we are on the first column of the current row, then an opening  *
* bracket and a space are printed.                                   *
*********************************************************************/
      if (j == 0)
      {
        fprintf(outFile, "[ ");
      } // if

/*********************************************************************
* Now printing out the matrix element.                               *
*********************************************************************/
      fprintf(outFile, "%.7f", gsl_matrix_get(m, i, j));

/*********************************************************************
* If we are not on the last column of the current row, then a comma  *
* and a space are printed out. Otherwise, a space, a closing bracket,*
* and a new line are printed.                                        *
*********************************************************************/
      if (j != (m->size2 - 1))
      {
        fprintf(outFile, ", ");
      } // if
      else
      {
        fprintf(outFile, " ]\n");
      } // else

    } // for j
  } // for i

/*********************************************************************
* Printing the final lines and then closing the file.                *
*********************************************************************/
  fprintf(outFile, "\n");
  fprintf(outFile, "%s END\n", matName);
  fclose(outFile);

} // void utils::printGSLMatFile(const gsl_matrix *m, 
  // const char *matName, const char *file) throw()





/*********************************************************************
* This function computes the i'th permutation on n letters, where    *
* 0 <= i <= (n! - 1). The set of permutations is well ordered by     *
* using a lexicographic ordering. Here's the set of permutations on  *
* 3 letters in lexicographic order:                                  *
*                                                                    *
* [0, 1, 2] (the identity permutation)                               *
* [0, 2, 1]                                                          *
* [1, 0, 2]                                                          *
* [1, 2, 0]                                                          *
* [2, 0, 1]                                                          *
* [2, 1, 0]                                                          *
*                                                                    *
* NOTE: The range of the "letters" is {0, 1, ..., n - 1}.            *
*                                                                    *
* The computed permutation is saved to the input gsl_perumation      *
* struct. A valid permutation in a gsl_permutation struct means that *
* each of the integers {0, 1, ..., n - 1} appears exactly once.      *
*                                                                    *
* This implementation is based on Algorithm 2.16, p. 56 of           *
* Combinatorial Algorithms, ISBN: 0-8493-3988-X.                     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* r                  size_t          The index of the desired        *
*                                    permutation.                    *
* pi                 gsl_permutation * The struct into which the     *
*                                      computed permutation is       *
*                                      stored.                       *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void permLexUnrank(size_t r, gsl_permutation *pi)
{

/*********************************************************************
* n will hold the permutation size.                                  *
*********************************************************************/
  const size_t n = pi->size;

/*********************************************************************
* We now ensure that r is <= (n! - 1).                               *
*********************************************************************/
  verifyPermRank(r, n);

/*********************************************************************
* The last element of pi is assigned 0.                              *
*********************************************************************/
  pi->data[n - 1] = 0;

/*********************************************************************
* Declaring and initializing needed variables.                       *
*********************************************************************/
  size_t d = 0, fact = 0, prevFact = 1;

/*********************************************************************
* The following for loop is used to compute the required permutation.*
*********************************************************************/
  for (size_t j = 0; j < n; j++)
  {

/*********************************************************************
* Assigning (j + 1)! to fact.                                        *
*********************************************************************/
    if (j == 0)
    {
      fact = 1;
    } // if
    else
    {

/*********************************************************************
* To get (j + 1)! for the current value of j, all we need to do is   *
* multiply prevFact by (j + 1).                                      *
*********************************************************************/
      fact = prevFact * (j + 1);

    } // else

/*********************************************************************
* Calculating d.                                                     *
*********************************************************************/
    d = (r % fact);

/*********************************************************************
* If d is non-zero, then r is decremented by d. Also, d is divided   *
* by j! (which is stored in prevFact).                               *
*********************************************************************/
    if (d)
    {
      r -= d;
      d /= prevFact;
    } // if

/*********************************************************************
* Now saving (j + 1)! in prevFact for the current value of j.        *
*********************************************************************/
    prevFact = fact;

/*********************************************************************
* We now finish off calculating the desired permutation.             *
*********************************************************************/
    pi->data[n - j - 1] = d;
    for (size_t i = (n - j); i < n; i++)
    {
      if (pi->data[i] >= d)
      {
        pi->data[i]++;
      } // if
    } // for i

  } // for j

/*********************************************************************
* As a debugging aid, the validity of the permutation pi can be      *
* printed out. 0 is a valid permutation; -1 is an invalid            *
* permutation. However, for most circumstances, the following line   *
* should be commented out.                                           *
*********************************************************************/
//  cout << "VALID PERMUTATION = [" << gsl_permutation_valid(pi) << "]" << endl;*/

} // void permLexUnrank(size_t r, gsl_permutation *pi)

/*********************************************************************
* This function computes the rank of the input permutation in        *
* lexicographic order. Here is an example for n equal to 3:          *
*                                                                    *
* PERMUTATION                             RANK                       *
* ------------------------------------------------------------------ *
* [0, 1, 2] (the identity permutation)    0                          *
* [0, 2, 1]                               1                          *
* [1, 0, 2]                               2                          *
* [1, 2, 0]                               3                          *
* [2, 0, 1]                               4                          *
* [2, 1, 0]                               5                          *
*                                                                    *
* NOTE: The range of the "letters" is {0, 1, ..., n - 1}.            *
*                                                                    *
* The range of the rank is [0, 1, ..., n - 1].                       *
*                                                                    *
* This implementation is based on Algorithm 2.15, p. 55 of           *
* Combinatorial Algorithms, ISBN: 0-8493-3988-X.                     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* pi                 gsl_permutation * The input permutation.        *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* The permutation's   size_t                                         *
* rank.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
size_t permLexRank(const gsl_permutation *pi)
{

/*********************************************************************
* n will hold the permutation size.                                  *
*********************************************************************/
  const size_t n = pi->size;

/*********************************************************************
* r will be the computed rank, i.e., the index of the input          *
* permutation in the lexicographic ordered set of all n!             *
* permutations. r is intialized to 0.                                *
*********************************************************************/
  size_t r = 0;

/*********************************************************************
* We allocate a new gsl_permutation struct of the required size. If  *
* the allocation fails, then an error message is printed and then    *
* this program exits.                                                *
*********************************************************************/
  gsl_permutation *rho = gsl_permutation_alloc(n);
  if (isNull(rho))
  {
    ostringstream errorMsg;
    errorMsg << "Unable to allocate memory for size [" << n << "] permutation.";
    printErrorMsgAndExit(VB_ERROR, errorMsg, __LINE__, __FUNCTION__, __FILE__, 1);

  } // if

/*********************************************************************
* Copying the input permutation pi to rho (since the input           *
* permutation will be overwritten.                                   *
*********************************************************************/
  gsl_permutation_memcpy(rho, pi);

/*********************************************************************
* Now computing the rank.                                            *
*********************************************************************/
  for (size_t j = 0; j < n; j++)
  {
    r += rho->data[j] * factorial(n - j - 1);
    for (size_t i = j; i < n; i++)
    {
      if (rho->data[i] > rho->data[j])
      {
        rho->data[i]--;
      } // if
    } // for i
  } // for j

/*********************************************************************
* Now returning the permutation's rank.                              *
*********************************************************************/
  return r;

} // size_t permLexRank(const gsl_permutation *pi)

/*********************************************************************
* This function computes the rank of the input permutation in        *
* minimal change order. Here is an example for n equal to 3:         *
*                                                                    *
* PERMUTATION                             RANK                       *
* ------------------------------------------------------------------ *
* [0, 1, 2] (the identity permutation)    0                          *
* [0, 2, 1]                               1                          *
* [2, 0, 1]                               2                          *
* [2, 1, 0]                               3                          *
* [1, 2, 0]                               4                          *
* [1, 0, 2]                               5                          *
*                                                                    *
* NOTE: The range of the "letters" is {0, 1, ..., n - 1}.            *
*                                                                    *
* The range of the rank is [0, 1, ..., n - 1].                       *
*                                                                    *
* This implementation is based on Algorithm 2.17, p. 60 of           *
* Combinatorial Algorithms, ISBN: 0-8493-3988-X.                     *
*                                                                    *
* One benefit of this algorithm is that it does not rely on          *
* calculating factorials.                                            *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* pi                 gsl_permutation * The input permutation.        *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* The permutation's   size_t                                         *
* rank.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
size_t trotterJohnsonRank(const gsl_permutation *pi)
{

/*********************************************************************
* n will hold the permutation size.                                  *
*********************************************************************/
  const size_t n = pi->size;

/*********************************************************************
* The rank, r, is initialized to 0.                                  *
*********************************************************************/
  size_t r = 0;

/*********************************************************************
* The following for loop is used to compute the rank.                *
*********************************************************************/
  for (size_t j = 2; j <= n; j++)
  {
    size_t k = 1;
    size_t i = 1;

    while ((pi->data[i - 1] + 1) != j)
    {
      if ((pi->data[i - 1] + 1) < j)
      {
        k++;
      } // if
      i++;
    } // while

    if (!(r % 2))
    {
      r = (j * r) + j - k;
    } // if
    else
    {
      r = (j * r) + k - 1;
    } // else

  } // for j

/*********************************************************************
* Now returning the rank.                                            *
*********************************************************************/
  return r;

} // size_t trotterJohnsonRank(const gsl_permutation *pi)

/*********************************************************************
* This function computes the i'th permutation on n letters, where    *
* 0 <= i <= (n! - 1). The set of permutations is well ordered by     *
* using the minimal change ordering. Here's the set of permutations  *
* on 3 letters in lexicographic order:                               *
*                                                                    *
* [0, 1, 2] (the identity permutation)                               *
* [0, 2, 1]                                                          *
* [1, 0, 2]                                                          *
* [1, 2, 0]                                                          *
* [2, 0, 1]                                                          *
* [2, 1, 0]                                                          *
*                                                                    *
* NOTE: The range of the "letters" is {0, 1, ..., n - 1}.            *
*                                                                    *
* The computed permutation is saved to the input gsl_perumation      *
* struct. A valid permutation in a gsl_permutation struct means that *
* each of the integers {0, 1, ..., n - 1} appears exactly once.      *
*                                                                    *
* This implementation is based on Algorithm 2.18, p. 61 of           *
* Combinatorial Algorithms, ISBN: 0-8493-3988-X.                     *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* r                  size_t          The index of the desired        *
*                                    permutation.                    *
* pi                 gsl_permutation * The struct into which the     *
*                                      computed permutation is       *
*                                      stored.                       *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* None.                                                              *
*                                                                    *
* EXCEPTIONS THROWN:                                                 *
* ------------------                                                 *
* None.                                                              *
*********************************************************************/
void trotterJohnsonUnrank(const size_t r, gsl_permutation *pi)
{

/*********************************************************************
* n will hold the permutation size.                                  *
*********************************************************************/
  const size_t n = pi->size;

/*********************************************************************
* We now ensure that r is <= (n! - 1).                               *
*********************************************************************/
  verifyPermRank(r, n);

/*********************************************************************
* Initializing needed variables.                                     *
*********************************************************************/
  pi->data[0] = 0;
  size_t r2 = 0;
  size_t r1 = 0;
  long k = 0;

/*********************************************************************
* The following for loop creates the specified permutation.          *
*********************************************************************/
  for (long j = 2; j <= (long ) n; j++)
  {
    factorialRatio(j, n, r1);
    r1 = (size_t ) (r / (double ) r1);

    k = r1 - (j * r2);

    if (!(r2 % 2))
    {
      for (long i = j - 1; i >= (j - k); i--)
      {
        pi->data[i] = pi->data[i - 1];
      } // for i
      pi->data[j - k - 1] = j - 1;
    } // if
    else
    {
      for (long i = j - 1; i >= (k + 1); i--)
      {
        pi->data[i] = pi->data[i - 1];
      } // for i
      pi->data[k] = j - 1;
    } // else

    r2 = r1;

  } // for j

} // void trotterJohnsonUnrank(const size_t r, gsl_permutation *pi)

/*********************************************************************
* This function prints the input permutation is a "standard" form,   *
* i.e., the elements of the permutation will range from 1 to n. It   *
* is assumed that the elements of the input permutation range from   *
* 0 to (n - 1).                                                      *
*********************************************************************/
void printStdPerm(const gsl_permutation *pi)
{

/*********************************************************************
* The following for loop increments each element of rho.             *
*********************************************************************/
  for (size_t i = 0; i < pi->size; i++)
  {
    if (i != (pi->size - 1))
    {
      cout <<  (pi->data[i] + 1) << " ";
    } // if
    else
    {
      cout <<  (pi->data[i] + 1) << endl;
    } // else

  } // for i

} // void printStdPerm(const gsl_permutation *pi)

/*********************************************************************
* This function is used to compute the "ratio" of two factorials:    *
*                                                                    *
* (n!)/(d!)                                                          *
*                                                                    *
* Specifically, the ratio computed is ((max(n, d)! / min(n, d)!))    *
* If, in fact, this function computes the reciprocal of the actual   *
* ratio, then false is returned. Otherwise, true is returned. The    *
* whole point of this function is to reduce the number of            *
* multiplications and the chance for overflow.                       *
*                                                                    *
* INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
* ----------------   -----           ------------                    *
* n                  const size_t    The "inverse" factorial of the  *
*                                    numerator of the ratio.         *
* d                  const size_t    The "inverse" factorial of the  *
*                                    denominator of the ratio.       *
* ratio              size_t&         Will hold the computed "ratio". *
*                                                                    *
* OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
* -----------------   -----          ------------                    *
* N/A                 bool           true is returned if the actual  *
*                                    ratio is computed. Otherwise,   *
*                                    false is returned.              *
*********************************************************************/
bool factorialRatio(const size_t n, const size_t d, size_t &ratio)
{

/*********************************************************************
* Setting ratio to 1.                                                *
*********************************************************************/
  ratio = 1;

/*********************************************************************
* If the numerator and denominator are equal, then we have already   *
* calculated the actual ratio. Therefore, true is returned.          *
*********************************************************************/
  if (n == d)
  {
    return true;
  } // if

/*********************************************************************
* The following for loop computes ((max(n, d)! / min(n, d)!)).       *
*********************************************************************/
  for (size_t i = min(n, d) + 1; i <= max(n, d); i++)
  {
    ratio *= i;
  } // for i

/*********************************************************************
* If n is less than d, then we have computed the reciprocal of the   *
* desired ratio. Therefore, false is returned. Otherwise, we have    *
* computed the actual ratio and true is returned.                    *
*********************************************************************/
  if (n < d)
  {
    return false;
  } // if
  else
  {
    return true;
  } // else

} // bool factorialRatio(const size_t n, const size_t d, size_t &ratio)

/*********************************************************************
* This function simply decrements the elements of the input          *
* permutation.                                                       *
*********************************************************************/
void decPerm(gsl_permutation *pi)
{
  for (size_t i = 0; i < pi->size; i++)
  {
    pi->data[i]--;
  } // for i
} // void decPerm(gsl_permutation *pi)

