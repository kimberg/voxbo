
int
permStart(permclass pc) {
  int method = pc.GetMethod();
  string matrixStemName = pc.GetMatrixStemName();
  string permDir = pc.GetPermDir(); 
  if (!matrixStemName.size())
    return 200;
  if (!permDir.size())
    return 201;
  map<size_t, vector<gsl_permutation *> > permHash;
  Tes paramTes; //= Tes();
  paramTes.ReadHeader(matrixStemName + ".prm");
  if (!paramTes.header_valid)
    return 202;
  const string gMatrixFile(matrixStemName + ".G");
  if (!utils::isFileReadable(gMatrixFile))
    return 203;
  VBMatrix gMatrix;
  if (gMatrix.ReadMAT1Header(gMatrixFile))
    return 204;
  const unsigned long orderG = gMatrix.m;
  unsigned long numPerms = MAX_PERMS;
  map<short, vector<short *> >signHash;
  switch(method) {
  case 1:
    gsl_permutation *v = gsl_permutation_calloc(orderG);
    if (isNull(v))
      return 205;
    if (orderG <= PERMUTATION_LIMIT) {
      numPerms = (size_t ) lrint(exp(gamma(orderG + 1)));
      do {
        inHash(permHash, v);
      } while (gsl_permutation_next(v) == GSL_SUCCESS);
    }
    else {
      inHash(permHash, v);
      initRNG();
      for (size_t i = 1; i < numPerms; i++) {
        randPerm(v);
        while (inHash(permHash, v))
          randPerm(v);
      } 
      freeRNG();
    } 
    if (v) gsl_permutation_free(v);
    break;
  case 2:
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
        initRNG();
        for (size_t i = 0; i < numPerms; i++) {
          randSignPerm(signPerm, orderG);
          while (inSignHash(signHash, signPerm, orderG))
            randSignPerm(signPerm, orderG);
        }
        freeRNG();
      } 
    if (signPerm) delete signPerm; 
    signPerm = 0;
    break;
  default:
    return 206;
    break;
  } 
  if (access((matrixStemName + "_" + permDir).c_str(), F_OK)) {
    errno = 0;
    if (mkdir((matrixStemName + "_" + permDir).c_str(), 0755))
      return 207;
    if (mkdir((matrixStemName + "_" + permDir + "/iterations").c_str(), 0755))
      return 207;
  } 
  VBMatrix permMat(orderG, numPerms, vb_double);
  if  (permMat.MakeInCore())
    return 208;
  if (!permMat.valid)
    return 209; 
  VB_Vector vec(orderG);
  size_t colIndex = 0;
  switch(method) {
  case 1:
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
    break;
  default:
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
    break;
  } 
  permMat.SetFileName(matrixStemName + "_" + permDir + "/permutations.mat");
  if (permMat.WriteData())
    return 210;
  permMat.clear();
  return 0;
} 


