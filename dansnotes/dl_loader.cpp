
// compile with gcc test.cpp -ldl

using namespace std;

#include "vbutil.h"
#include "vbio.h"
#include "dlfcn.h"

int
main(int argc,char **argv)
{
  while (TRUE) {
    cout << VBRandom() << endl;
    usleep(250);
  }

  VBMatrix a(argv[1]);
  VBMatrix b(argv[2]);
  VBMatrix c(a.m,b.n);
  c.FileName()=argv[3];
  c.MakeFile();
  MultiplyPartsXtY(&a,&b,&c,0,a.m-1);
  a.print();
  c.print();
  exit(0);
}

int
mainx()
{
  void *handle;
  int (*bogus)();

  handle=dlopen("/lib/libm.so.6",RTLD_NOW);
  printf("%ld\n",(long)handle);
  bogus=(int(*)())dlsym(handle,"cos");
  printf("%ld\n",(long)bogus);

  handle=dlopen("/home/kimberg/src/voxbo/libvoxbo/test2.o",RTLD_LAZY);
  printf("%ld\n",(long)handle);
  bogus=(int(*)())dlsym(handle,"bogus");
  printf("%ld\n",(long)bogus);

  bogus();
  exit(0);
}
