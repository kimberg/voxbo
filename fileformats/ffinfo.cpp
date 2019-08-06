
using namespace std;

// #include <stdio.h>
#include <sys/stat.h>
#include <string>
#include "dlfcn.h"
#include "vbio.h"
#include "vbprefs.h"

int main(int argc, char **argv) {
  stringstream tmps;
  string thisdir, prevdir;

  VBFF::LoadFileTypes();

  if (argc > 1) {
    tokenlist args;
    args.Transfer(argc - 1, argv + 1);
    for (size_t i = 0; i < args.size(); i++) {
      cout << "Eligible filetypes for file " << args[i] << endl;
      vector<VBFF> types = EligibleFileTypes(args[i]);
      for (size_t j = 0; j < types.size(); j++) {
        cout << "  " << types[j].getName() << endl;
      }
    }
    exit(0);
  }

  if (!VBFF::filetypelist.size()) {
    printErrorMsg(VB_INFO, "No filetypes found.");
    exit(0);
  }
  printf("[I] ffinfo: found the following file formats:\n");
  vector<VBFF>::iterator ff;
  for (ff = VBFF::filetypelist.begin(); ff != VBFF::filetypelist.end(); ff++) {
    if (ff->getSignature() == "NONE") continue;
    // following commented out while we're using all built-ins
    //     if ((thisdir=xdirname(VBFF::filetypelist[i].getPath())) != prevdir) {
    //       prevdir=thisdir;
    //       tmps.str("");
    //       tmps << "In directory " << thisdir << ":";
    //       printErrorMsg(VB_INFO,tmps.str());
    //     }
    printf("[I]   %s (%s) id=%s ext=%s support=", ff->getName().c_str(),
           ff->getPath().c_str(), ff->getSignature().c_str(),
           ff->extension.c_str());
    if (ff->read_1D) printf("read1d ");
    if (ff->write_1D) printf("write1d ");
    if (ff->read_data_3D) printf("read3d ");
    if (ff->write_3D) printf("write3d ");
    if (ff->read_data_4D) printf("read4d ");
    if (ff->write_4D) printf("write4d ");
    printf("\n");
  }
  exit(0);
}
