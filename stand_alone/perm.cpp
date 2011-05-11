
// perm.cpp
// 
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
// original version written by Kosh Banerjee

/*********************************************************************
 * Required include file.                                             *
 *********************************************************************/
#include "perm.h"

/*********************************************************************
 * This function sets up the permutation analysis.                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * matrixStemName     const string&   The absolute path to the GLM    *
 *                                    directory prepended to the      *
 *                                    basename of the GLM directory.  *
 * permDir            const string&   The permutation directory.      *
 * method             vb_orderperm or vb_signperm
 *                                                                    *
 * OUTPUT VARIABLES:                                                  *
 * -----------------                                                  *
 * The permutation matrix file is written out.                        *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
int
permStart(permclass pc)
{
  struct stat st;
  string mypermdir=xdirname(pc.stemname)+"/"+pc.permdir;
  if (!(vb_direxists(mypermdir+"/iterations"))) {
    if (createfullpath(mypermdir))
      return 207;
    if (createfullpath(mypermdir+"/iterations"))
      return 207;
  }
  rmdir_force(mypermdir+"/logs");
  mkdir((mypermdir+"/logs").c_str(), 0755);
  // if there's already a matrix, we keep it, so we're done
  if (stat((mypermdir+"/permutations.mat").c_str(),&st)==0)
    return 0;
  VB_permtype method = pc.method;
  if (pc.stemname.empty())
    return 200;
  if (pc.permdir.empty())
    return 201;
  map<size_t, vector<gsl_permutation *> > permHash;
  Tes paramTes;
  paramTes.ReadHeader(pc.stemname+".prm");
  if (!paramTes.header_valid)
    return 202;
  const string gMatrixFile(pc.stemname + ".G");
  if (!utils::isFileReadable(gMatrixFile))
    return 203;
  VBMatrix gMatrix;
  if (gMatrix.ReadHeader(gMatrixFile))
    return 204;
  const unsigned long orderG = gMatrix.m;
  unsigned long numPerms = MAX_PERMS;
  map<short, vector<short *> >signHash;
  switch(method) {
  case vb_orderperm:
    {
      gsl_permutation *v = gsl_permutation_calloc(orderG);
      if (!v)
        return 205;
      if (orderG <= PERMUTATION_LIMIT) {
        numPerms = (size_t ) lrint(exp(gamma(orderG + 1)));
        do {
          inHash(permHash, v);
        } while (gsl_permutation_next(v) == GSL_SUCCESS);
      }
      else {
        inHash(permHash, v);
        initRNG(pc.rngseed);
        for (size_t i = 1; i < numPerms; i++) {
          randPerm(v);
          while (inHash(permHash, v))
            randPerm(v);
        } 
        freeRNG();
        if (v) gsl_permutation_free(v);
      }
    }
    break;
  case vb_signperm:
    {
      short *signPerm = new short[orderG];
      if (orderG <= SIGN_PERMUTATION_LIMIT) {
        numPerms = (1 << (orderG - 1));
        for (size_t i = 0; i < numPerms; i++) {
          for (size_t j = 0; j < orderG; j++)
            signPerm[j] = (utils::getBit(i, j) * -2) + 1;
          inSignHash(signHash, signPerm, orderG);
        } 
      } 
      else
        {
          initRNG(pc.rngseed);
          for (size_t i = 0; i < numPerms; i++) {
            randSignPerm(signPerm, orderG);
            while (inSignHash(signHash, signPerm, orderG))
              randSignPerm(signPerm, orderG);
          }
          freeRNG();
        } 
      if (signPerm) delete signPerm; 
      signPerm = 0;
    }
    break;
  default:
    return 206;
    break;
  }
  VBMatrix permMat(orderG, numPerms);
  if (!permMat.rowdata)
    return 208;
  VB_Vector vec(orderG);
  size_t colIndex = 0;
  switch(method)
    {
    case vb_orderperm:
      {
        map<size_t, vector<gsl_permutation *> >::iterator mItr;
        for (mItr = permHash.begin(); mItr != permHash.end(); mItr++) {
          for (size_t i = 0; i < mItr->second.size(); i++) {
            for (size_t k = 0; k < orderG; k++)
              vec[k] = mItr->second[i]->data[k];
            permMat.SetColumn(colIndex, vec);
            colIndex++;
          } 
        } 
        freePermHash(permHash);
      }
      break;
    case vb_signperm:
      {
        map<short, vector<short *> >::iterator mItr;
        for (mItr = signHash.begin(); mItr != signHash.end(); mItr++) {
          for (size_t i = 0; i < mItr->second.size(); i++) {
            for (size_t k = 0; k < orderG; k++)
              vec[k] = mItr->second[i][k];
            if (orderG <= SIGN_PERMUTATION_LIMIT)
              permMat.SetColumn(rankSignPerm(mItr->second[i], orderG), vec);
            else {
              permMat.SetColumn(colIndex, vec);
              colIndex++;
            } 
          } 
        } 
        freeSignHash(signHash);
      } 
      break;
    default:
      break;
    } 
  permMat.filename=mypermdir+"/permutations.mat";
  if (permMat.WriteFile())
    return 210;
  permMat.clear();
  return 0;
} 


VBMatrix
createPermMatrix(int nperms,int ndata,VB_permtype method,uint32 rngseed)
{
  VBMatrix permMat(ndata,nperms);
  VBMatrix errormat(1,1);
  if (!permMat.valid()) {
    permMat.clear();
    return permMat;
  }
  map<size_t, vector<gsl_permutation *> > permHash;

  map<short, vector<short *> >signHash;
  gsl_permutation *v;
  short *signPerm;
  
  switch(method) {
  case vb_orderperm:
    v = gsl_permutation_calloc(ndata);
    if (!v)
      return errormat;
    if (ndata <= PERMUTATION_LIMIT) {
      nperms = (size_t ) lrint(exp(gamma(ndata + 1)));
      do {
        inHash(permHash, v);
      } while (gsl_permutation_next(v) == GSL_SUCCESS);
    }
    else {
      inHash(permHash, v);
      initRNG(rngseed);
      for (int i=1; i<nperms; i++) {
        randPerm(v);
        while (inHash(permHash, v))
          randPerm(v);
      } 
      freeRNG();
    } 
    if (v) gsl_permutation_free(v);
    break;
  case vb_signperm:
    signPerm = new short[ndata];
    if (ndata <= SIGN_PERMUTATION_LIMIT) {
      nperms = (1 << (ndata - 1));
      for (int i = 0; i < nperms; i++) {
        for (int j = 0; j < ndata; j++)
          signPerm[j] = (utils::getBit(i, j) * -2) + 1;
        inSignHash(signHash, signPerm,ndata);
      } 
    } 
    else
      {
        initRNG(rngseed);
        for (int i=0; i<nperms; i++) {
          randSignPerm(signPerm,ndata);
          while (inSignHash(signHash, signPerm,ndata))
            randSignPerm(signPerm,ndata);
        }
        freeRNG();
      } 
    if (signPerm) delete signPerm; 
    signPerm = 0;
    break;
  default:
    return errormat;
    break;
  }
  VB_Vector vec(ndata);
  size_t colIndex = 0;
  switch(method) {
  case vb_orderperm:
    {
      map<size_t, vector<gsl_permutation *> >::iterator mItr;
      for (mItr = permHash.begin(); mItr != permHash.end(); mItr++) {
        for (size_t i=0; i<mItr->second.size(); i++) {
          for (int k = 0; k < ndata; k++)
            vec[k] = mItr->second[i]->data[k];
          permMat.SetColumn(colIndex, vec);
          colIndex++;
        } 
      } 
      freePermHash(permHash);
    }
    break;
  default:
    map<short, vector<short *> >::iterator mItr;
    for (mItr = signHash.begin(); mItr != signHash.end(); mItr++) {
      for (size_t i = 0; i < mItr->second.size(); i++) {
        for (int k=0; k<ndata; k++)
          vec[k] = mItr->second[i][k];
        if (ndata <= SIGN_PERMUTATION_LIMIT)
          permMat.SetColumn(rankSignPerm(mItr->second[i], ndata), vec);
        else {
          permMat.SetColumn(colIndex, vec);
          colIndex++;
        } 
      } 
    } 
    freeSignHash(signHash);
    break;
  }
  return permMat;
} 

/*********************************************************************
 * This function computes the factorial of the input non-negative     *
 * integer.                                                           *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * n                  const size_t    The integer whose factorial is  *
 *                                    to be computed.                 *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * n!                  size_t                                         *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
size_t factorial(const size_t n)
{

  /*********************************************************************
   * If n is 0 or 1, then we simply return 1.                           *
   *********************************************************************/
  if (n <= 1)
    {
      return 1;
    } // if

  /*********************************************************************
   * If program flow ends up here, then n >= 2.                         *
   *********************************************************************/
  else
    {

      /*********************************************************************
       * fact is initialized to 1.                                          *
       *********************************************************************/
      size_t fact = 1;

      /*********************************************************************
       * The following for loop is sued to compute the product:             *
       * (2)(3)...(n)                                                       *
       *********************************************************************/
      for (size_t i = 2; i <= n; i++)
        {
          fact *= i;
        } // for i

      /*********************************************************************
       * Now returning n!.                                                  *
       *********************************************************************/
      return fact;

    } // else

} // size_t factorial(const size_t n)

/*********************************************************************
 * This function simply writes out the elements of the input GSL      *
 * permutation.                                                       *
 *********************************************************************/
void printGSLPerm(const gsl_permutation *pi)
{
  gsl_permutation_fprintf(stdout, pi, "%u ");
  cout << endl;
} // void printGSLPerm(const gsl_permutation *pi)

// pick a random permutation

void
randPerm(gsl_permutation *pi)
{
  gsl_ran_shuffle (theRNG, pi->data, pi->size,sizeof(size_t));
}

/*********************************************************************
 * This function simply increments the elements of the input          *
 * permutation.                                                       *
 *********************************************************************/
void incPerm(gsl_permutation *pi)
{
  for (size_t i = 0; i < pi->size; i++)
    {
      pi->data[i]++;
    } // for i
} // void incPerm(gsl_permutation *pi)

/*********************************************************************
 * This function returns true if the two input permutations are equal.*
 * Otherwise, false is returned.                                      *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * p1                 const gsl_permutation * The first of the two    *
 *                                            permutations to be      *
 *                                            compared.               *
 * p2                 const gsl_permutation * The second of the two   *
 *                                            permutations to be      *
 *                                            compared.               *
 * begin              const size_t    The index at which the          *
 *                                    comparison starts. The default  *
 *                                    value is 0.                     *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * If the two permutations are equal, then true is returned.          *
 * Otherwise, false is returned.                                      *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
bool arePermsEqual(const gsl_permutation *p1,
                   const gsl_permutation *p2, const size_t begin)
{

  /*********************************************************************
   * If the two input permutations are of different sizes, then false is*
   * returned.                                                          *
   *********************************************************************/
  if (p1->size != p2->size)
    {
      return false;
    } // false

  /*********************************************************************
   * The following for loop is used to compare each element of the two  *
   * input permutations, starting that the index begin. NOTE: Since we  *
   * are comparing permutations, we only have to compare the elements   *
   * up to, and including, index (p1->size - 2).                        *
   *********************************************************************/
  for (size_t i = begin; i < (p1->size - 1); i++)
    {

      /*********************************************************************
       * If the two permutations elements are not equal, then false is      *
       * returned.                                                          *
       *********************************************************************/
      if (p1->data[i] != p2->data[i])
        {
          return false;
        } // if
    } // for i

  /*********************************************************************
   * If program flow ends up here, then the two input permutations are  *
   * equal. Therefore, true is returned.                                *
   *********************************************************************/
  return true;

} // bool arePermsEqual(const gsl_permutation *p1,
  // const gsl_permutation *p2, const size_t begin)

/*********************************************************************
 * This function checks to see if the input permutation rank is valid *
 * or not. If the input rank is not valid, then an appropriate error  *
 * message is printed and then this program exits.                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * r                  const size_t    The input permutation rank.     *
 * n                  const size_t    The size of the permutation.    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void verifyPermRank(const size_t r, const size_t n)
{

  /*********************************************************************
   * If now check to see if r is <= to (n! - 1) (since r must be in     *
   * [0, 1, ..., n! - 1]). However, to avoid computing large numbers,   *
   * we use the natural log function. Moreover, we compare log(r + 1)   *
   * to log(n!) (which is given by gamma(n + 1)) knowing that log() is  *
   * monotonically increasing.                                          *
   *********************************************************************/
  if ( log((double ) (r + 1)) > gamma((double ) (n + 1)))
    {
      ostringstream errorMsg;
      errorMsg << "The index of the desired permutation [" << r
               << "] is greater than (" << n << "! - 1).";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if

} // void verifyPermRank(const size_t r, const size_t n)

/*********************************************************************
 * This function computes the i'th permutation on n letters, where    *
 * 0 <= i <= (n! - 1). This implementation is based on the paper:     *
 *                                                                    *
 * "Ranking and Unranking Permutations in Linear Time" by             *
 * Wendy Myrvold (wendym@csr.uvic.ca) and                             *
 * Frank Ruskey (frusky@csr.uvic.ca)                                  *
 *                                                                    *
 * The computed permutation is saved to the input gsl_perumation      *
 * struct. A valid permutation in a gsl_permutation struct means that *
 * each of the integers {0, 1, ..., n - 1} appears exactly once. The  *
 * advantage of this function is that no factorials are computed; the *
 * disadvantage is that this function is recursive.                   *
 *                                                                    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * n                  const size_t    The length of the permutation.  *
 * r                  const size_t    The index of the desired        *
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
void unrank1(const size_t n, const size_t r, gsl_permutation *pi)
{

  /*********************************************************************
   * If we are in the first call to this function, then n must equal    *
   * pi->size. Therefore, we ensure that r <= (n! - 1).                 *
   *********************************************************************/
  if (n == pi->size)
    {
      verifyPermRank(r, pi->size);
    } // if

  /*********************************************************************
   * Now recursively constructing the desired permutation. NOTE: The    *
   * recursion stops when the input parameter n is 0.                   *
   *********************************************************************/
  if (n)
    {
      gsl_permutation_swap(pi, n - 1, (r % n));
      unrank1(n - 1, (r / n), pi);
    } // if

} // void unrank1(const size_t n, const size_t r, gsl_permutation *pi)

/*********************************************************************
 * This function computes the rank of the input permutation. This     *
 * implementation is based on the paper:                              *
 *                                                                    *
 * "Ranking and Unranking Permutations in Linear Time" by             *
 * Wendy Myrvold (wendym@csr.uvic.ca) and                             *
 * Frank Ruskey (frusky@csr.uvic.ca)                                  *
 *                                                                    *
 * The advantage of this function is that no factorials are computed; *
 * the disadvantage is that this function is recursive. NOTE: In the  *
 * first call to this function, pi must be the identity permutation.  *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * n                  const size_t    The length of the permutation.  *
 * pi                 gsl_permutation * The input permutation.        *
 * piInv              gsl_permutation * The inverse of the input      *
 *                                      permutation.                  *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * The rank of the input permutation.                                 *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
size_t rank1(const size_t n, gsl_permutation *pi,
             gsl_permutation *piInv)
{

  /*********************************************************************
   * If n is 1, we return 0, halting the recursion.                     *
   *********************************************************************/
  if (n == 1)
    {
      return 0;
    } // if

  /*********************************************************************
   * We now recursively compute the rank of the input permutation pi.   *
   *********************************************************************/
  size_t s = pi->data[n - 1];
  gsl_permutation_swap(pi, n - 1, piInv->data[n - 1]);
  gsl_permutation_swap(piInv, s, n - 1);
  return (s + (n * rank1(n - 1, pi, piInv)));

} // size_t rank1(const size_t n, gsl_permutation *pi,
  // gsl_permutation *piInv)

/*********************************************************************
 * This function checks to see if the input permutation already       *
 * exists in the input permutation hash table or not. If the          *
 * is not yet in the hash table, it is added to the hash table.       *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * permHash           map<size_t, vector<const gsl_permutation *> >   *
 *                                    The input permutation hash      *
 *                                    table.                          *
 * pi                 const gsl_permutation * The input permutation.  *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * If the input permutation is not in the input permutation hash      *
 * table, then false is returned. Otherwise, true is returned.        *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
bool inHash(map<size_t, vector<gsl_permutation*> >&permHash, gsl_permutation *pi) {
  gsl_permutation *rho = NULL;
  if (permHash.count(pi->data[0]) == 0) {
    vector<gsl_permutation *> permVec;
    rho = gsl_permutation_alloc(pi->size);
    gsl_permutation_memcpy(rho, pi);
    permVec.push_back(rho);
    permHash.insert(pair<size_t, vector<gsl_permutation *> >(pi->data[0], permVec));
    return false;
  } 
  for (size_t i = 0; i < permHash[pi->data[0]].size(); i++) 
    if (arePermsEqual(pi,  permHash[pi->data[0]][i], 1))
      return true;
  rho = gsl_permutation_alloc(pi->size);
  gsl_permutation_memcpy(rho, pi);
  permHash[pi->data[0]].push_back(rho);
  return false;
} 

void
initRNG(uint32 rngseed)
{
  gsl_rng_env_setup();
  const gsl_rng_type *T;
  if (getenv("GSL_RNG_TYPE"))
    T = gsl_rng_default;
  else
    T = gsl_rng_taus113;
  theRNG = gsl_rng_alloc(T);
  if (rngseed==0)
    rngseed=VBRandom();
  gsl_rng_set(theRNG,rngseed);
} 

void freeRNG() {
  if (theRNG)
    gsl_rng_free(theRNG);
} 

void availableGSLRNGs()
{

  const gsl_rng_type **t, **t0;
  t0 = gsl_rng_types_setup();
  cout << "AVAILABLE GSL RNG's:" << endl;
  for (t = t0; *t != NULL; t++)
    {
      cout << (*t)->name << endl;
    } // for t

} // void availableGSLRNGs()

/*********************************************************************
 * This function prints out the contents of the input map container,  *
 * which is used has a hash table for a set of GSL permutations.      *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * theHash   map<size_t, vector<gsl_permutation *> >&                 *
 *                                    The input map container.        *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void printHash(map<size_t, vector<gsl_permutation *> >& theHash)
{

  /*********************************************************************
   * An iterator is instantiated to the beginning of the map.           *
   *********************************************************************/
  map<size_t, vector<gsl_permutation *> >::iterator mItr;

  /*********************************************************************
   * hashSize is initialized to 0. It will be used to cound up the      *
   * number of GSL permutations in the input map.                       *
   *********************************************************************/
  size_t hashSize = 0;

  /*********************************************************************
   * Printing out a separator line.                                     *
   *********************************************************************/
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

  /*********************************************************************
   * The following for loop iterates through the map container.         *
   *********************************************************************/
  for (mItr = theHash.begin(); mItr != theHash.end(); mItr++)
    {

      /*********************************************************************
       * Incrementing hashSize.                                             *
       *********************************************************************/
      hashSize += mItr->second.size();

      /*********************************************************************
       * This for loop iterates through the vector container pointed to by  *
       * mItr.                                                              *
       *********************************************************************/
      for (size_t i = 0; i < mItr->second.size(); i++)
        {

          /*********************************************************************
           * Now printing out the GSL permutation.                              *
           *********************************************************************/
          printGSLPerm(mItr->second[i]);

        } // for i

    } // for mItr

  /*********************************************************************
   * Printing out the map size, hashSize, and a separator line to       *
   * "close off" the printing of the map container.                     *
   *********************************************************************/
  cout << "MAP SIZE  = [" << theHash.size() << "]" << endl;
  cout << "HASH SIZE = [" << hashSize << "]" << endl;
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

} // void printHash(map<size_t, vector<gsl_permutation *> >& theHash)

/*********************************************************************
 * This function frees the memory allocated to the gsl_permutation    *
 * structs contained in the input map container.                      *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * theHash    map<size_t, vector<gsl_permutation *> >&                *
 *                                    The input map container.        *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void freePermHash(map<size_t, vector<gsl_permutation *> >& theHash) {
  map<size_t, vector<gsl_permutation *> >::iterator mItr;
  for (mItr = theHash.begin(); mItr != theHash.end(); mItr++)
    for (size_t i = 0; i < mItr->second.size(); i++)
      if (mItr->second[i]) 
        gsl_permutation_free(mItr->second[i]);
       
} 

/*********************************************************************
 * This function returns true if the two input sign permutations are  *
 * equal. Otherwise, false is returned.                               *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * p1                 const short *   The first of the two sign       *
 *                                    permutations to be compared.    *
 * p2                 const short *   The second of the two sign      *
 *                                    permutations to be compared.    *
 * len                const size_t    The length of both input sign   *
 *                                    permutations.                   *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * If the two permutations are equal, then true is returned.          *
 * Otherwise, false is returned.                                      *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
bool areSignsEqual(const short *arr1, const short *arr2, const size_t len)
{

  /*********************************************************************
   * If both arr1 and arr2 contain the same address, then true is       *
   * returned.                                                          *
   *********************************************************************/
  if (arr1 == arr2)
    {
      return true;
    } // if

  /*********************************************************************
   * The following for loop is used to compare the corresponding        *
   * elements of the two sign permutations.                             *
   *********************************************************************/
  for (size_t i = 0; i < len; i++)
    {

      /*********************************************************************
       * If the two corresponding elements are not equal, then false is     *
       * returned.                                                          *
       *********************************************************************/
      if (arr1[i] != arr2[i])
        {
          return false;
        } // if

    } // for i

  /*********************************************************************
   * If program flow ends up here, then no differences were found       *
   * between the two input sign permutations. Therefore, true is        *
   * is returned.                                                       *
   *********************************************************************/
  return true;

} // bool areSignsEqual(const short *arr1, const short *arr2, const size_t len)

/*********************************************************************
 * This function checks to see if the input short array of 1's and    *
 * -1's (a sign permutation array) already  exists in the input sign  *
 * permutation hash table or not. If the short array is not yet in    *
 * the hash table, it is added to the hash table. NOTE: The key used  *
 * in the input map container, i.e., the input sign permutation hash  *
 * table, is the sum of the elements of the sign permutation.         *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * signHash           map<short, vector<short *> > The input sign     *
 *                                                 permutation hash   *
 *                                                 table.             *
 *                                                                    *
 * pi                 short *         The input sign permutation.     *
 *                                                                    *
 * len                const size_t    The length of the input sign    *
 *                                    permutation.                    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * If the input sign permutation is not in the input permutation hash *
 * table, then false is returned. Otherwise, true is returned.        *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
bool inSignHash(map<short, vector<short *> > &signHash,
                short *pi, const size_t len)
{

  /*********************************************************************
   * rho will be used to copy pi, if the need arises.                   *
   *********************************************************************/
  short *rho = NULL;

  long sum=0;
  for (size_t i=0; i<len; i++)
    sum+=pi[i];

  /*********************************************************************
   * If signHash.count() returns 0, then the input sign permutation is  *
   * not yet in signHash. Therefore, it is added to signHash and false  *
   * is returned.                                                       *
   *********************************************************************/
  if (signHash.count(sum) == 0)
    {
      vector<short *> permVec;
      rho = new short[len];
      if (!rho)
        {
          ostringstream errorMsg;
          errorMsg << "Unable to allocate memory for size [" << len << "] sign permutation.";
          printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
        } // if
      memcpy(rho, pi, sizeof(short) * len);
      permVec.push_back(rho);
      signHash.insert(pair<short, vector<short *> >(sum, permVec));
      return false;
    } // if

  /*********************************************************************
   * The following for loop is used to check each element of the        *
   * signHash map container. The sum of the elements of pi is used as   *
   * the key for the signHash map. So using the map's [] operator, we   *
   * compare pi to each sign permutation in the map whose first element *
   * is the same as the sum of the elements of pi.                      *
   *********************************************************************/
  for (size_t i = 0; i < signHash[sum].size(); i++)
    {

      /*********************************************************************
       * If pi equals the sign permutation itr->second[i], then true is     *
       * returned.                                                          *
       *********************************************************************/
      if (areSignsEqual(pi,  signHash[sum][i], len))
        {
          return true;
        } // if

    } // for

  /*********************************************************************
   * If program flow ends up here, then pi is not in the hash table. So *
   * we add pi to the hash table (at hash slot itr) and return false.   *
   *********************************************************************/
  rho = new short[len];
  if (!rho)
    {
      ostringstream errorMsg;
      errorMsg << "Unable to allocate memory for size [" << len << "] sign permutation.";
      printErrorMsgAndExit(VB_ERROR, errorMsg.str(), __LINE__, __FUNCTION__, __FILE__, 1);
    } // if
  memcpy(rho, pi, sizeof(short) * len);
  signHash[sum].push_back(rho);
  return false;

} // bool inSignHash(map<short, vector<short *> > &signHash,
  // short *pi, const size_t len)

/*********************************************************************
 * This function frees the memory allocated to the sign permutations  *
 * contained in the input map container.                              *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * theHash    map<short, vector<short *> >&                           *
 *                                    The input map container.        *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void freeSignHash(map<short, vector<short *> >& theHash)
{

  /*********************************************************************
   * An iterator is instantiated to the beginning of the map.           *
   *********************************************************************/
  map<short, vector<short *> >::iterator mItr;

  /*********************************************************************
   * The following for loop iterates through the map container.         *
   *********************************************************************/
  for (mItr = theHash.begin(); mItr != theHash.end(); mItr++)
    {

      /*********************************************************************
       * This for loop iterates through the vector container pointed to by  *
       * mItr.                                                              *
       *********************************************************************/
      for (size_t i = 0; i < mItr->second.size(); i++)
        {

          /*********************************************************************
           * Now freeing the previously allocated memory.                       *
           *********************************************************************/
          delete mItr->second[i];
          mItr->second[i] = 0;

        } // for i

    } // for mItr

} // void freeSignHash(map<short, vector<short *> >& theHash)

/*********************************************************************
 * This function prints out the contents of the input map container,  *
 * which is used has a hash table for a set of sign permutations.     *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * theHash   map<short, vector<short *> >&                            *
 *                                    The input map container.        *
 * len                const size_t    The length of each sign         *
 *                                    permutation in the map          *
 *                                    container.                      *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void printSignHash(map<short, vector<short *> >& theHash, const size_t len)
{

  /*********************************************************************
   * An iterator is instantiated to the beginning of the map.           *
   *********************************************************************/
  map<short, vector<short *> >::iterator mItr;

  /*********************************************************************
   * hashSize is initialized to 0. It will be used to cound up the      *
   * number of sign permutations in the input map.                      *
   *********************************************************************/
  size_t hashSize = 0;

  /*********************************************************************
   * Printing out a separator line.                                     *
   *********************************************************************/
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

  /*********************************************************************
   * The following for loop iterates through the map container.         *
   *********************************************************************/
  for (mItr = theHash.begin(); mItr != theHash.end(); mItr++)
    {

      /*********************************************************************
       * Incrementing hashSize.                                             *
       *********************************************************************/
      hashSize += mItr->second.size();

      /*********************************************************************
       * This for loop iterates through the vector container pointed to by  *
       * mItr.                                                              *
       *********************************************************************/
      for (size_t i = 0; i < mItr->second.size(); i++)
        {

          /*********************************************************************
           * Now printing out the sign permutation.                             *
           *********************************************************************/
          printSignPerm(mItr->second[i], len);

        } // for i

    } // for mItr

  /*********************************************************************
   * Printing out the map size, hashSize, and a separator line to       *
   * "close off" the printing of the map container.                     *
   *********************************************************************/
  cout << "MAP SIZE  = [" << theHash.size() << "]" << endl;
  cout << "HASH SIZE = [" << hashSize << "]" << endl;
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

} // void printSignHash(map<short, vector<short *> >& theHash, const size_t len)

/*********************************************************************
 * This function prints out the input sign permutation.               *
 *********************************************************************/
void printSignPerm(const short *thePerm, const size_t len)
{

  /*********************************************************************
   * The following for loop is used to print out the values from        *
   * thePerm.                                                           *
   *********************************************************************/
  for (size_t i = 0; i < len; i++)
    {
      cout.width(2);
      cout.fill(' ');
      cout << thePerm[i];
      if (i != (len - 1))
        {
          cout << " ";
        } // if
    } // for i
  cout << endl;

} // void printSignPerm(const short *thePerm, const size_t len)

/*********************************************************************
 * This function constructs a random sign permutation and stores it   *
 * in the input short array.                                          *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:               DESCRIPTION:                *
 * ----------------   -----               ------------                *
 * pi                 short *             The generated random sign   *
 *                                        permutation will be stored  *
 *                                        in this variable.           *
 * len                const size_t        The length of the sign      *
 *                                        permutation.                *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * None.                                                              *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
void randSignPerm(short *pi, const size_t len)
{

  /*********************************************************************
   * The following for loop is used to generate the random sign         *
   * permutation. NOTE: The variable theRNG is a gloabl variable.       *
   *********************************************************************/
  for (size_t i = 0; i < len; i++)
    {

      /*********************************************************************
       * We use gsl_rng_uniform_int() to get a random integer in [0, 1].    *
       * The random inetegr is first multiplied by -2 and 1 is added. This  *
       * will yield a random 1 or -1.                                       *
       *********************************************************************/
      pi[i] = (gsl_rng_uniform_int(theRNG, 2) * -2) + 1;
    } // for i

} // void randSignPerm(short *pi, const size_t len)

/*********************************************************************
 * This function determines the ran of the input sign permutation.    *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * signPerm           const short *   The sign permutation.           *
 * len                const size_t    The length of the sign          *
 *                                    permutation.                    *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * N/A                 size_t         The rank of the input sign      *
 *                                    permutation.                    *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
size_t rankSignPerm(const short *signPerm, const size_t len)
{

  /*********************************************************************
   * The rank is initialized to 0.                                      *
   *********************************************************************/
  size_t rank = 0;

  /*********************************************************************
   * The following for loop traverses signPerm.                         *
   *********************************************************************/
  for (size_t i = 0; i < len; i++)
    {

      /*********************************************************************
       * The following steps are carried out to compute the rank.           *
       *                                                                    *
       * 1. We view the sign permutation as a binary representation of an   *
       *    integer.                                                        *
       * 2. The 1's are viewed as zeros and the -1's are viewed as ones.    *
       * 3. We then determine the rank by computing the integer represented *
       *    by the sign permutation.                                        *
       *                                                                    *
       * EXAMPLE SIGN PERMUTATION                                   RANK    *
       * -----------------------------------------------------------------  *
       * [1, 1, 1]    ==> ((2^0) * 0) + ((2^1) * 0) + ((2^2) * 0) = 0       *
       * [-1, 1, -1]  ==> ((2^0) * 1) + ((2^1) * 0) + ((2^2) * 1) = 5       *
       * [-1, -1, -1] ==> ((2^0) * 1) + ((2^1) * 1) + ((2^2) * 1) = 7       *
       *                                                                    *
       * If signPerm[i] is 1, we don't need to increment rank. Therefore,   *
       * the following if block only adds the i'th power of 2 when          *
       * signPerm is -1.                                                    *
       *********************************************************************/
      if (signPerm[i] == -1)
        {
          rank += (1 << i);
        } // if
    } // for i

  /*********************************************************************
   * Now returning the rank.                                            *
   *********************************************************************/
  return rank;

} // size_t rankSignPerm(const short *signPerm, const size_t len)

/*********************************************************************
 * This function performs a single step of a permutation analysis.    *
 * Specifically, the following tasks are carried out:                 *
 *                                                                    *
 * 1. Loads the permutation matrix.                                   *
 * 2. Reads the appripriate GLM files.                                *
 * 3. Does a regression to create a new *.prm files for the           *
 *    particualr permutation.                                         *
 * 4. Calls makeStatCub() to create a statistical image for that      *
 *    *.prm file.                                                     *
 * 5. Stores the appripriate maximum value for that image.            *
 *                                                                    *
 * INPUT VARIABLES:   TYPE:           DESCRIPTION:                    *
 * ----------------   -----           ------------                    *
 * matrixStemName     const string&   The matrix stem name.           *
 *                                                                    *
 * permDir            const string&   The permutation directory.      *
 *                                                                    *
 * permIndex          const unsigned  The permutation index.          *
 *                    short                                           *
 *                                                                    *
 * method             VB_permtype
 *                                                                    *
 * contrasts          VB_Vector&            The contrast array.             *
 *                                                                    *
 * scale              const unsigned  Inidcates the type of statistic *
 *                    short           that will be computed.          *
 *                                                                    *
 * pseudoT            VB_Vector&            If the number of elements in    *
 *                                    this VB_Vector object is 3, then the  *
 *                                    error cube will be smoothed by  *
 *                                    the kernel returned by          *
 *                                    makeSmoothKernel().             *
 *                                                                    *
 * OUTPUT VARIABLES:   TYPE:          DESCRIPTION:                    *
 * -----------------   -----          ------------                    *
 * Writes out a new *.prm file.                                       *
 *                                                                    *
 * EXCEPTIONS THROWN:                                                 *
 * ------------------                                                 *
 * None.                                                              *
 *********************************************************************/
int
permStep(string& matrixStemName, const string& permDir,
         const unsigned short permIndex, VB_permtype method,
         VBContrast &contrast,VB_Vector& pseudoT, int exhaustive) throw()
{
  if (matrixStemName.size() == 0)
    return 100;
  if (permDir.size() == 0)
    return 101; 
  string mypermdir=xdirname(matrixStemName)+"/"+permDir;
  Tes paramTes = Tes(matrixStemName + ".prm");
  if (!paramTes.data_valid)
    return 103;
  string tempString = paramTes.GetHeader("DataScale:"); 
  // Cube statCube;
  int err = 0;

  // set up our GLM structure
  GLMInfo glmi;
  glmi.setup(matrixStemName);
  glmi.contrast=contrast;
  glmi.pseudoT=pseudoT;

  //method 0 just returns the unpermuted stat cube
  if (method == vb_noperm) {
    glmi.calc_stat_cube();
    // err=makeStatCub(statCube, NULL, matrixStemName,contrast,pseudoT,paramTes,.01);
    string sfile=(format("%s/permcube_%06d.cub.gz")%mypermdir%permIndex).str();
    if (glmi.statcube.WriteFile(sfile))
      return 128;
    else
      return err;
  }
  size_t brainCount = 0;
  vector<unsigned long> brainPoints;
  vector<unsigned long> noBrainPoints;
  MaskXYZCoord spatialCoords;
  for (long i = 0; i < paramTes.voxels; i++) {
    setMaskCoords(&spatialCoords, i, paramTes.dimx, paramTes.dimy, paramTes.dimz);
    if (paramTes.GetMaskValue(spatialCoords.x, spatialCoords.y, spatialCoords.z)) {
      brainCount++;
      brainPoints.push_back(i);
    } 
    else
      noBrainPoints.push_back(i);
  }
  if (!brainCount)
    return 104;
  vector<string> tesList;
  utils::readInTesFiles(matrixStemName, tesList);
  if (!tesList.size())
    return 105;
  string gMatrixFile = matrixStemName + ".G";
  if (!utils::isFileReadable(gMatrixFile))
    return 106;
  VBMatrix gMatrix(gMatrixFile);
  if (!gMatrix.valid())
    return 107; 
  const unsigned long orderG = gMatrix.m;
  const unsigned long rankG = gMatrix.n;
  VB_Vector traces = VB_Vector(matrixStemName + ".traces");
  if (!traces.getState())
    return 108; 
  double traceRV = traces[0];
  string vMatrixFile = matrixStemName + ".V";
  if (!utils::isFileReadable(vMatrixFile))
    return 109;
  VBMatrix vMatrix(vMatrixFile);
  if (!vMatrix.valid())
    return 110;
  if (gMatrix.m != vMatrix.m)
    return 111; 
  string f1MatrixFile = matrixStemName + ".F1";
  if (!utils::isFileReadable(f1MatrixFile))
    return 112;
  VBMatrix origF1(f1MatrixFile);
  if (!origF1.valid())
    return 113;
  if (vMatrix.m != origF1.n)
    return 114; 
  string f3MatrixFile = matrixStemName + ".F3";
  if (!utils::isFileReadable(f3MatrixFile))
    return 116;
  VBMatrix origF3(f3MatrixFile);
  if (!origF3.valid())
    return 117; 
  if (origF1.m != origF3.n)
    return 118;
  if (origF1.n != origF3.m)
    return 119; 
  vector<double> contrastF1(origF1.n, 0.0);
  vector<double> contrastF3(origF1.n, 0.0);
  for (size_t i = 0; i < (size_t ) origF1.m; i++)
    for (size_t j = 0; j < (size_t ) origF1.n; j++) {
      contrastF1[j] += origF1(i, j) * contrast.contrast[i];
      contrastF3[j] += origF3(j, i) * contrast.contrast[i];
    } 

  double origFact = 0.0;
  for (int num = 0; num < (int)origF1.n; num++)
    origFact += (contrastF1[num] * contrastF3[num]);
  string permMatrixFile = mypermdir+"/permutations.mat";
  if (!utils::isFileReadable(permMatrixFile))
    return 120;
  VBMatrix permMatrix(permMatrixFile);
  if (!permMatrix.valid())
    return 121; 
  if (permIndex >= permMatrix.n)
    return 123;
  VB_Vector permArray(permMatrix.GetColumn(permIndex));
  vector<unsigned long> betasOfInt;
  vector<unsigned long> betasToPermute;
  for (unsigned short i = 0; i < gMatrix.header.size(); i++) {
    if (gMatrix.header[i].size() > 0) {
      tokenlist args;
      args.ParseLine(gMatrix.header[i]);
      if ((args[0]=="Parameter:") && ((args[2]=="Interest") ||
                                      (args[2]=="KeepNoInterest")))
        betasOfInt.push_back(strtol(args[1]));
      if (args[0]=="Parameter:" && args[2]=="Interest")
        betasToPermute.push_back(strtol(args[1]));
    }
  }

  if (betasOfInt.size() == 0)
    return 124;
  betasOfInt.push_back(rankG);
  paramTes.SetVolume(paramTes.dimx, paramTes.dimy, paramTes.dimz, betasOfInt.size(), vb_double);

  gsl_matrix *G = gsl_matrix_calloc(gMatrix.m, gMatrix.n);
  for (int x = 0; x < (int)gMatrix.m; x++)
    for (int y = 0; y < (int)gMatrix.n; y++)
      gsl_matrix_set(G, x, y, gMatrix(x, y));

  gsl_matrix *permuteG = gsl_matrix_calloc(G->size1, G->size2);
  for (int x = 0; x < (int)G->size1; x++)
    for (int y = 0; y < (int)G->size2; y++)
      gsl_matrix_set(permuteG, x, y, gsl_matrix_get(G, x, y));

  if ( (betasToPermute.size()) && (method == vb_orderperm) ) {
    for (size_t covarIndex = 0; covarIndex < betasToPermute.size(); covarIndex++) {
      VB_Vector tempVec(permuteG->size1);
      for (size_t k = 0; k < (size_t ) tempVec.size(); k++)
        tempVec[k] = gsl_matrix_get(G, (size_t ) permArray[k], betasToPermute[covarIndex]);
      for (size_t k = 0; k < (size_t ) tempVec.size(); k++) 
        gsl_matrix_set(permuteG, k, betasToPermute[covarIndex], tempVec[k]);
    }
  }
  double fact = 0.0;
  gsl_matrix *R = gsl_matrix_calloc(G->size1, G->size1);
  if (!R) return 126;
  gsl_matrix *F1 = gsl_matrix_calloc(G->size2, G->size1);
  if (!F1) return 127;
  err=doFactR(matrixStemName,permDir,contrast.contrast,permuteG,vMatrix,
              &fact,R,F1);
  if (err) return err;
  if (G) gsl_matrix_free(G);
  if(permuteG) gsl_matrix_free(permuteG);
  Tes tesData;
  double numChunks = 1.0;
  if (tesList.size() > 1)
    numChunks = ceil(( ((float ) orderG) * ((float ) brainCount) ) / 2000000.0);
  else
    tesData.ReadFile(tesList[0]);
  long startPoint = 0, endPoint = brainCount - 1;
  vector<vector<double> > chunkData;
  for (size_t loopChunk = 0; loopChunk < numChunks; loopChunk++) {
    if (loopChunk) {
      cout.precision(2);
      cout << setw(2) << "Percent Done: " << double(loopChunk)/double(numChunks) << endl;
    }
    if (tesList.size() > 1) {
      startPoint = long(((float ) brainCount / numChunks) * ((float ) loopChunk));
      endPoint = long(((float ) brainCount / numChunks) * ((float ) loopChunk + 1.0) - 1.0);
      if (((float ) loopChunk) >= ((float ) numChunks - 1)) endPoint = brainCount - 1;
      vector<unsigned long> theRegion;
      for (int i = startPoint; i <= (int) endPoint; i++)
        theRegion.push_back(brainPoints[i]);
      unsigned short stat = regionalTimeSeries(theRegion, tesList, chunkData, !strncmp(tempString.c_str(),"mean",4));
      if (stat)
        exit(stat);
    } 
    gsl_matrix *signal = gsl_matrix_calloc(1, orderG);
    gsl_matrix *transSignalF1 = gsl_matrix_calloc(F1->size1, signal->size1);
    gsl_matrix *residuals = gsl_matrix_calloc(R->size1, signal->size1);
    gsl_matrix *betas = gsl_matrix_calloc(1, F1->size2+1);
    gsl_matrix *ErrorSq = gsl_matrix_calloc(residuals->size2, residuals->size2);
    double signalValue = 0.0;
    for (int i = startPoint; i <= (int)endPoint; i++) {
      setMaskCoords(&spatialCoords, brainPoints[i], paramTes.dimx, paramTes.dimy, paramTes.dimz);
      if (tesList.size() == 1) {
        // if tesList.size() is 1, we've already read the whole tes
        // file and can use GetTimeSeries here (much faster)
        // tesData.ReadTimeSeries(tesList[0], spatialCoords.x, spatialCoords.y, spatialCoords.z);
        tesData.GetTimeSeries(spatialCoords.x, spatialCoords.y, spatialCoords.z);
        for (int elementNumber = 0; elementNumber < (int)orderG; elementNumber++) {
          signalValue = tesData.timeseries[elementNumber];
          if (method == vb_signperm) signalValue*= permArray[elementNumber]; 
          gsl_matrix_set(signal, 0, elementNumber, signalValue);
        }
      }
      else 
        for (int elementNumber = 0; elementNumber < (int)orderG; elementNumber++) {
          signalValue = chunkData[i - startPoint][elementNumber];
          if (method == vb_signperm) signalValue*= permArray[elementNumber];
          gsl_matrix_set(signal, 0, elementNumber, signalValue);
        }
      gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, F1, signal, 0.0, transSignalF1);
      gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, R, signal, 0.0, residuals);
      gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, residuals, residuals, 0.0, ErrorSq);
      gsl_matrix_scale(ErrorSq, (1/traceRV));
      gsl_matrix_scale(ErrorSq, fact/origFact);
      for (size_t b = 0; b < (size_t ) F1->size1; b++) 
        gsl_matrix_set(betas, 0, b, gsl_matrix_get(transSignalF1, b, 0));
      gsl_matrix_set(betas, 0, F1->size1, gsl_matrix_get(ErrorSq, 0, 0));
      for (size_t t = 0; t < (unsigned int)betasOfInt.size(); t++) {
        //cout << t<<" "<<gsl_matrix_get(betas, 0, betasOfInt[t])<<endl;;
        paramTes.SetValue(spatialCoords.x, spatialCoords.y, spatialCoords.z, t, gsl_matrix_get(betas, 0, betasOfInt[t])); 
      }
    }
    if (signal) gsl_matrix_free(signal);
    if (transSignalF1) gsl_matrix_free(transSignalF1);
    if (residuals) gsl_matrix_free(residuals);
    if (betas) gsl_matrix_free(betas);
    if (ErrorSq) gsl_matrix_free(ErrorSq);
  }
  if (F1) gsl_matrix_free(F1);
  if (R) gsl_matrix_free(R);
  // FIXME temporarily kludged to use makeStatCub()
  Cube sc;
  err = makeStatCub(sc,matrixStemName,contrast,pseudoT,paramTes);
  glmi.statcube=sc;
  //glmi.calc_stat_cube();
  if (err==103) err = 1103; //makestatcub and perm share error mesage 103
  if (err) return err;
  ostringstream statFileName, statFileName2;
  if (method == vb_orderperm || (method == vb_signperm && !exhaustive)) {
    string sfile=(format("%s/iterations/permcube_%06d.cub.gz")%mypermdir%permIndex).str();
    if (glmi.statcube.WriteFile(sfile))
      return 128;
  }
  else if (method == vb_signperm && exhaustive) {
    string sfile=(format("%s/iterations/permcube_%06da.cub.gz")%mypermdir%permIndex).str();
    if (glmi.statcube.WriteFile(sfile))
      return 128;   
    sfile=(format("%s/iterations/permcube_%06db.cub.gz")%mypermdir%permIndex).str();
    glmi.statcube /= -1.0;
    if (glmi.statcube.WriteFile(sfile))
      return 128;
  }
  return 0;
} 

int doFactR(const string&, const string&,
            VB_Vector& contrasts, gsl_matrix *permuteG, VBMatrix& V, double *fact,
            gsl_matrix *R, gsl_matrix *F1) throw()
{
  //begin: create F1 from (invert(transpose(PermuteG)##PermuteG)##transpose(PermuteG))
  gsl_matrix *Gtg = gsl_matrix_calloc(permuteG->size2, permuteG->size2);
  if (!Gtg) return 129;
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, permuteG, permuteG, 0.0, Gtg);
  gsl_matrix *invGtg = gsl_matrix_calloc(permuteG->size2, permuteG->size2);
  if (!invGtg) return 130;
  gsl_permutation *LUPerm = gsl_permutation_calloc(Gtg->size2);
  if (!LUPerm) return 131;
  int permSign = 0;
  gsl_linalg_LU_decomp(Gtg, LUPerm, &permSign);
  if (gsl_linalg_LU_invert(Gtg, LUPerm, invGtg) != GSL_SUCCESS) return 132;
  gsl_permutation_free(LUPerm);
  gsl_matrix *Ggtg = gsl_matrix_calloc(permuteG->size2, permuteG->size1);
  if (!Ggtg) return 133;
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, invGtg, permuteG, 0.0, Ggtg);
  gsl_matrix_memcpy(F1, Ggtg);
  //begin: create R by R=(-1)*(PermuteG##Fac1) and R(indgen(OrderG)*(OrderG+1L))=1+R(indgen(OrderG)*(OrderG+1L))
  gsl_matrix *rMat = gsl_matrix_calloc(permuteG->size1, permuteG->size1);
  if (!rMat) return 134;
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, -1.0, permuteG, F1, 0.0, rMat);
  for (size_t i = 0; i < permuteG->size1; i++)
    gsl_matrix_set(rMat, i, i, 1.0 + gsl_matrix_get(rMat, i, i));
  gsl_matrix_memcpy(R, rMat);

  //begin: Fac3=V##PermuteG##(invert(transpose(PermuteG)##PermuteG))         
  gsl_matrix *v = gsl_matrix_calloc(V.m, V.n);
  for (int x = 0; x < (int)V.m; x++)
    for (int y = 0; y < (int)V.n; y++)
      gsl_matrix_set(v, x, y, V(x, y));
  gsl_matrix *Gtvt = gsl_matrix_calloc(permuteG->size1, permuteG->size2);
  if (!Gtvt) return 135;
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, permuteG, invGtg, 0.0, Gtvt);
  gsl_matrix *F3 = gsl_matrix_calloc(V.m, permuteG->size2); 
  if (!F3) return 136;
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, v, Gtvt, 0.0, F3);
  //begin: ContrastArray##Fac1##Fac3##transpose(ContrastArray)
  gsl_matrix *contrastMatrix = gsl_matrix_calloc(1, contrasts.size());
  gsl_matrix *result1 = gsl_matrix_calloc(contrastMatrix->size1, F1->size2);
  gsl_matrix *result2 = gsl_matrix_calloc(result1->size1, F3->size2);
  gsl_matrix *result3 = gsl_matrix_calloc(result2->size1, contrastMatrix->size1);
  for (uint32 index = 0; index < contrasts.size(); index++)
    gsl_matrix_set(contrastMatrix, 0, index, contrasts[index]);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, contrastMatrix, F1, 0.0, result1);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, result1, F3, 0.0, result2);
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, result2, contrastMatrix, 0.0, result3);
  *fact = gsl_matrix_get(result3, 0, 0);
  if (result1) gsl_matrix_free(result1);
  if (result2) gsl_matrix_free(result2);
  if (result3) gsl_matrix_free(result3);
  if (contrastMatrix) gsl_matrix_free(contrastMatrix);
  if (rMat) gsl_matrix_free(rMat);
  if (invGtg) gsl_matrix_free(invGtg);
  if (Gtvt) gsl_matrix_free(Gtvt);
  if (Ggtg) gsl_matrix_free(Ggtg);
  if (Gtg) gsl_matrix_free(Gtg);
  if (v) gsl_matrix_free(v);
  return 0;
} 

permclass::permclass()
{
  stemname="";
  permdir="";
  scale="";
  method=vb_orderperm;
  nperms=0;
  contrast="";
  pseudotlist.clear();
  filename="";
  m_prmTimeStamp = "";
  rngseed=0;
}

permclass::~permclass(){;}

void permclass::AddPrmTimeStamp(string matrixStemName) {
  Tes prm(matrixStemName+".prm");
  m_prmTimeStamp = prm.GetHeader("TimeStamp:");
  return;
}

string
permclass::GetPrmTimeStamp()
{
  return m_prmTimeStamp;
}

void
permclass::SavePermClass()
{
  ofstream out;
  string pst;
  string file =xdirname(stemname)+"/"+permdir+"/perminfo.txt";
  out.open(file.c_str(), ios::out);
  out << "stemname " << stemname << endl;
  out << "permdir " << permdir << endl; 
  out << "method " << methodstring(method) << endl;
  out << "pseudotkernel " << pseudotlist << endl;
  out << "contrast " << contrast << endl;
  out << "prmid " << m_prmTimeStamp << endl;
  out.close();
  return;
}

// FIXME LoadPermClass still not used, but it could be

void
permclass::LoadPermClass()
{
  tokenlist lines;
  lines.ParseFile(filename);
  vbforeach(string line,lines) {
    tokenlist args(line);
    string tag=vb_tolower(args[0]);
    tag=xstripwhitespace(tag," \t\n\r:");  // strip whitespace and colons!
    if (tag=="stemname") stemname=args.Tail();
    else if (tag=="permdir") permdir=args.Tail();
    else if (tag=="method") method=methodtype(args[1]);
    // FIXME else if (tag=="pseudotkernel")
    else if (tag=="contrast") contrast=args.Tail();
    else if (tag=="prmid") m_prmTimeStamp=args.Tail();
  }
}

void
permclass::SetFileName(string fname)
{
  filename=fname;
  return;
}

void
permclass::Clear()
{
  stemname="";
  permdir="";
  scale="";
  method=vb_orderperm;
  nperms=0;
  contrast="";
  pseudotlist.clear();
  filename="";
  m_prmTimeStamp = "";
  return;
}

string
permclass::methodstring(const VB_permtype pt)
{
  if (pt==vb_noperm) return "noperm";
  if (pt==vb_orderperm) return "orderperm";
  if (pt==vb_signperm) return "signperm";
  return "noperm";
}

VB_permtype
permclass::methodtype(const string tp)
{
  string tpx=vb_tolower(xstripwhitespace(tp));
  if (tpx=="0" || tpx=="noperm") return vb_noperm;
  if (tpx=="1" || tpx=="orderperm") return vb_orderperm;
  if (tpx=="2" || tpx=="signperm") return vb_signperm;
  return vb_noperm;
}

void
permclass::Print()
{
  string pst;
  
  cout << "matrix stem name: " << stemname << endl;
  cout << "perm directory: " << permdir << endl;
  cout << "method: " << methodstring(method) << endl;
  cout << "scale: " << scale << endl;
  cout << "pseudot values: " << pseudotlist << endl;
  cout << "contrast: " << contrast << endl;
  cout << "prm timestamp: " << m_prmTimeStamp << endl;
  return;
}

