
// utils.cpp
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

#include "utils.h"

void utils::seg_fault(int) {
  cout << "SIGSEGV RECEIVED" << endl;
  struct rlimit theLimit;
  if (getrlimit(RLIMIT_CORE, &theLimit)) {
    ostringstream errorMsg;
    errorMsg << "Line Number [" << __LINE__
             << "] Call to getrlimit() returned the error: [" << strerror(errno)
             << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
  } else {
    if ((theLimit.rlim_cur > 0) && (!access(".", W_OK))) {
      char cwd[STRINGLEN];
      getcwd(cwd, STRINGLEN - 1);
      cout << "CREATING CORE FILE in directory [" << cwd << "]." << endl;
      abort();
    }
  }
  cout << "UNABLE TO CREATE CORE FILE. EXITING." << endl;
  exit(139);
}

short utils::refOrTextOrMat(const string& vbFile, bool printErrs) {
  FILE* reader1D = fopen(vbFile.c_str(), "r");
  if (!reader1D) {
    if (printErrs) {
      ostringstream errorMsg;
      errorMsg << "Unable to open file: [" << vbFile << "].";
      printErrorMsg(VB_ERROR, errorMsg.str());
    }
    return UNKN_TYPE;
  }
  char fileLine[TEMP_STRING_LENGTH];
  memset(fileLine, 0, TEMP_STRING_LENGTH);
  fgets(fileLine, TEMP_STRING_LENGTH, reader1D);
  string fileLineStr = xstripwhitespace(fileLine, " ;\n\t");
  if (fileLineStr != "VB98") {
    fclose(reader1D);
    if (printErrs) {
      ostringstream errorMsg;
      errorMsg << "The file [" << vbFile
               << "] does not appear to be a VxoBo file.";
      printErrorMsg(VB_ERROR, errorMsg.str());
    }
    return UNKN_TYPE;
  }
  memset(fileLine, 0, TEMP_STRING_LENGTH);
  fgets(fileLine, TEMP_STRING_LENGTH, reader1D);
  fclose(reader1D);
  fileLineStr = xstripwhitespace(fileLine, " ;\n\t");
  if (fileLineStr == "REF1")
    return utils::REF_TYPE;
  else if (fileLineStr == "TXT1")
    return utils::TXT_TYPE;
  else if (fileLineStr == "MAT1")
    return utils::MAT_TYPE;
  if (printErrs) {
    ostringstream errorMsg;
    errorMsg << "Unknown file type [" << fileLine << "].";
    printErrorMsg(VB_ERROR, errorMsg.str());
  }
  return utils::UNKN_TYPE;
}

unsigned short utils::determineVBFileDims(const string& theFile) {
  switch (utils::refOrTextOrMat(theFile)) {
    case utils::REF_TYPE:
    case utils::TXT_TYPE:
      return 1;
      break;
    case utils::MAT_TYPE:
      return 2;
      break;
    default:
      break;
  }
  vector<VBFF> formats = EligibleFileTypes(theFile);
  VBFF fileformat;
  if (!formats.size()) return 0;
  fileformat = formats[0];
  return fileformat.getDimensions();
}

// char * utils::determineVBFile(const string& theFile) {
//   switch (utils::refOrTextOrMat(theFile)) {
//     case utils::REF_TYPE:
//       return "REF1";
//     break;
//     case utils::TXT_TYPE:
//       return "TXT1";
//     break;
//     case utils::MAT_TYPE:
//       return "MAT1";
//     break;
//     default:
//     break;
//   }
//   vector<VBFF> formats = EligibleFileTypes(theFile);
//   VBFF fileformat;
//   if (!formats.size())
//     return "UNKN";
//   fileformat=formats[0];
//   Tes theTes = Tes();
//   Cube theCube = Cube();
//   switch(fileformat.getDimensions()) {
//     case 3:
//       if (theCube.ReadHeader(theFile))
//         return "UNKN";
//       return "CUB1";
//     break;
//     case 4:
//       if (theTes.ReadHeader(theFile))
//         return "UNKN";
//       return "TES1";
//     break;
//     default:
//       return "UNKN";
//     break;
//   }
// }

string utils::determineVBFFName(const string& theFile) {
  switch (utils::refOrTextOrMat(theFile)) {
    case utils::TXT_TYPE:
      return string("VoxBo TXT1");
      break;
    case utils::MAT_TYPE:
      return string("VoxBo MAT1");
      break;
    default:
      break;
  }
  vector<VBFF> formats = EligibleFileTypes(theFile);
  VBFF fileformat;
  if (!formats.size()) return string("UNKNOWN");
  fileformat = formats[0];
  return fileformat.getName();
}

bool utils::isFileReadable(const string& fileName) throw() {
  if (access(fileName.c_str(), R_OK))
    return 0;
  else
    return 1;
}

int utils::readInTesFiles(const string& matrixStemName,
                          vector<string>& tesList) throw() {
  if (matrixStemName.size() == 0) return 1;
  string subFile = matrixStemName + ".sub";
  if (!isFileReadable(subFile)) return 2;
  if (utils::refOrTextOrMat(subFile) != utils::TXT_TYPE) return 3;
  ifstream subFileStream(subFile.c_str(), ios::in);
  if (!subFileStream) return 4;
  char inputLine[TIME_STR_LEN];
  memset(inputLine, 0, TIME_STR_LEN);
  while (subFileStream.getline(inputLine, TIME_STR_LEN)) {
    if ((inputLine[0] == ';') || (strlen(inputLine) == 0)) continue;
    unsigned short nwsIndex = 0;
    unsigned short commentIndex = 0;
    for (; nwsIndex < strlen(inputLine); nwsIndex++)
      if (!isspace(inputLine[nwsIndex])) break;
    if (nwsIndex == strlen(inputLine)) continue;
    for (commentIndex = nwsIndex; commentIndex < strlen(inputLine);
         commentIndex++)
      if ((inputLine[commentIndex] == ';') || (inputLine[commentIndex] == '#'))
        break;
    if (nwsIndex == commentIndex) continue;
    char* editedLine = new char[commentIndex - nwsIndex + 1];
    memset(editedLine, 0, commentIndex - nwsIndex + 1);
    memcpy(editedLine, inputLine + nwsIndex, commentIndex - nwsIndex);
    if (access(editedLine, R_OK)) {
      string editedLineFile2 = (string)editedLine + ".tes";
      if (access(editedLineFile2.c_str(), R_OK))
        return 5;
      else
        tesList.push_back(string(editedLine) + ".tes");
    } else
      tesList.push_back(string(editedLine));
    delete[] editedLine;
    editedLine = 0;
  }
  subFileStream.close();
  return 0;
}
