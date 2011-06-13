
using namespace std;

#include <sys/wait.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <zlib.h>
#include <limits>
#include "vbprefs.h"
#include "vbutil.h"
#include "vbio.h"
#include <map>
#include "glmutil.h"
#include "gsl/gsl_complex.h"
#include "gsl/gsl_complex_math.h"
#include "makestatcub.h"
#include "glmutil.h"
#include "imageutils.h"
#include "mydefs.h"
#include "vbx.h"
#include <omp.h>
#include <limits.h>

// #include <sys/sysinfo.h>
#include "stats.h"

#include "boost/date_time/gregorian/gregorian.hpp"
using namespace boost::gregorian;
// using namespace boost::date_time;

VBPrefs vbp;
extern char **environ;

int ReadMATFileHeader(VBMatrix &m,string fname);

extern "C" {
#include "dicom.h"
#include "errno.h"
#include "nifti.h"
}

int
main(int argc,char **argv)
{
  VB_Vector a("a.ref");
  VB_Vector b("b.ref");
  x2val tt=calc_chisquared((bitmask)a,(bitmask)b);
  cout << format("x2=%g df=%d p=%g\n")%tt.x2%tt.df%tt.p;
  tt=calc_chisquared((bitmask)a,(bitmask)b,1);
  cout << format("x2(corr)=%g df=%d p=%g\n")%tt.x2%tt.df%tt.p;
  tt=calc_fisher((bitmask)a,(bitmask)b);
  cout << format("fisher p=%g\n")%tt.p;

  cout << "  lesion (0/1): " << tt.c10 << " " << tt.c11 << endl;
  cout << "nolesion (0/1): " << tt.c00 << " " << tt.c01 << endl;
  exit(0);
  Tes foo;
  Cube cb;
  foo.ReadHeader(argv[1]);
  foo.ExtractMask(cb);
  cb.WriteFile("x.cub");
  exit(0);

  cout << argc << argv[0] << endl;

//   struct sysinfo si;
//   sysinfo(&si);
//   cout << "load " << si.loads[0] << endl;
//   cout << "load " << si.loads[1] << endl;
//   cout << "load " << si.loads[2] << endl;
//   cout << "totalram " << si.totalram*si.mem_unit/1024/1024 << endl;
//   cout << "freeram " << si.freeram*si.mem_unit/1024/1024 << endl;
//   cout << "sharedram " << si.sharedram*si.mem_unit/1024/1024 << endl;
//   cout << "bufferram " << si.bufferram*si.mem_unit/1024/1024 << endl;
//   cout << "totalswap " << si.totalswap*si.mem_unit/1024/1024 << endl;
//   cout << "freeswap " << si.freeswap*si.mem_unit/1024/1024 << endl;
  exit(0);
}







// void
// find_nearby_nonzero(Cube &cb,int xx,int yy,int zz)
// {
//   Cube mask(cb.dimx,cb.dimy,cb.dimz,vb_byte);
//   multimap<float,myvoxel> mymap;
//   pair<float,myvoxel> mypair;
//   int lastx=xx,lasty=yy,lastz=zz;
//   while(1) {
//     // first add a neighborhood around preceding voxel
//     for (int i=lastx-1; i<=lastx+1; i++) {
//       if (i<0 || i>cb.dimx-1) continue;
//       for (int j=lasty-1; j<=lasty+1; j++) {
//         if (j<0 || j>cb.dimy-1) continue;
//         for (int k=lastz-1; k<=lastz+1; k++) {
//           if (k<0 || k>cb.dimz-1) continue;
//           if (mask.testValue(i,j,k)) {
//             myvoxel vv;
//             vv.x=i;  vv.y=j;  vv.z=k;
//             dist=sqrt(((xx-i)*(xx-i))+((yy-j)*(yy-j))+((zz-k)*(zz-k)));
//             mypair.first=dist;
//             mypair.second=vv;
//             mymap.insert(mypair);
//             mask.setValue(i,j,k,0);
//           }
//         }
//       }
//     }
//     // grab the first voxel and try it
//     lastx=mymap.front().x;
//     lasty=mymap.front().y;
//     lastz=mymap.front().z;
//     // FIXME
//     // now pop it off the front and iterate
//     mymap.erase(mymap.begin());
    
//   }
// }







//   cout << correlation(aaa,bbb) << endl;
//   cout << covariance(aaa,bbb) << endl;

// CODE TO FORK AN XVFB PROCESS
//   pid_t pid=fork();
//   if (pid==0) {  // child
//     execlp("Xvfb",":10");
//   }
//   else if (pid==-1)
//     cout << 12 << endl;
//   int err=kill(pid,9);
//   cout << err << endl;  
//   cout << errno << endl;  
//   exit(0);
int
ReadMATFileHeader(VBMatrix &m,string fname)
{
  // parse filename (remove bracketed stuff)
  FILE *fp=fopen(fname.c_str(),"r");
  // grab the first 124 bytes, copy the header
  char buf[128],hdr[125];
  if (fread(buf,1,128,fp)!=128) {
    fclose(fp);
    return 101;
  }
  memcpy(hdr,buf,124);
  hdr[124]='\0';
  m.AddHeader(hdr);
  // the next two shorts are the matlab version and the endian indicator
  if (buf[126]=='M')
    m.filebyteorder=ENDIAN_BIG;
  else
    m.filebyteorder=ENDIAN_LITTLE;
  int16 mversion;
  memcpy(&mversion,buf+124,2);
  int swapped=0;
  if (m.filebyteorder!=my_endian())
    swapped=1;
  if (swapped)
    swap(&mversion);
  m.AddHeader((string)"MATLAB version "+strnum(mversion));
  // now build an index of the file by repeatedly grabbing a pair of
  // longs, the second of which is nbytes, and iterating
  uint32 dtype,dlen,skip;
  while(1) {
    if (fread(&dtype,sizeof(long),1,fp)!=1) {
      fclose(fp);
      return 102;
    }
    if (fread(&dlen,sizeof(long),1,fp)!=1) {
      fclose(fp);
      return 103;
    }
    if (swapped) {
      swap(&dtype);
      swap(&dlen);
    }
    // pad to 64-bit (8 byte) boundaries
    skip=dlen;
    if (dlen%8) skip+=8-(dlen%8);
    //printf("dtype %ld dlen %ld skip %ld\n",dtype,dlen,skip);
    
    // quick read the element
    unsigned char flags[8];
    if (fread(flags,1,8,fp)!=8) {
      fclose(fp);
      return 104;
    }



  }
  fclose(fp);
  return 0;
}


char helptext[]="\
foo bar baz\n\
  here is the:\n\
  argument structure\n\
i like help"
;
