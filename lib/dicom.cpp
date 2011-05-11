
// dicom.cpp
// VoxBo I/O support for DICOM format, with siemens extensions
// Copyright (c) 2003-2010 by The VoxBo Development Team

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
// original version written by Dan Kimberg

using namespace std;

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sstream>
#include "vbutil.h"
#include "vbio.h"


extern "C" {

#include "dicom.h"

#define SIEMENS_TAG "### ASCCONV BEGIN ###"
#define SIEMENS_TAG_END "### ASCCONV END ###"
  
int parse_siemens_stuff(char *buf,int cnt,dicominfo &dci);
void mask_dicom(dicominfo &dci,unsigned char *buf);

// FIXME dicominfo needs a constructor that sets defaults

dicominfo::dicominfo()
{
  init();
}
  
void
dicominfo::init()
{
  study=series=acquisition=instance=0;
  ti=te=tr=navg=slicethickness=fieldstrength=spacing=flipangle=0.0;
  slthick=skip=zpos=0.0;
  offset=datasize=bpp=bps=mosaicflag=0;
  dimx=dimy=dimz=xfov=yfov=rows=cols=0;
  byteorder=ENDIAN_BIG;
  moveparam[0]=99999;
  voxsize[0]=0.0;
  voxsize[1]=0.0;
  voxsize[2]=0.0;
  position[0]=0.0;
  position[1]=0.0;
  position[2]=0.0;
}

int
read_dicom_header(string filename,dicominfo &dci)
{
  FILE *ifile=fopen(filename.c_str(),"r");
  if (!ifile)
    return 105;
  uint16 group,element;
  char vr[5];   // value representation
  char dicm[5];
  uint32 cnt;
  bool f_bigendian=0;
  dci.mosaicflag=0;
  tokenlist args;
  dci.dimx=dci.dimy=0;
  dci.dimz=1;
  dci.slices=1;
  dci.spacing=dci.slthick=0.0;
  dci.moveparam[0]=99999;
  dci.ti=0;
  dci.te=0;
  dci.tr=0;
  dci.fieldstrength=0.0;
  dci.flipangle=0.0;
  dci.spos[0]=0.0;
  dci.spos[1]=0.0;
  dci.spos[2]=0.0;

  fseek(ifile,128,SEEK_SET);
  if (fread(dicm,1,4,ifile)!=4)
    return 202;
  dicm[4]='\0';
  dci.byteorder=my_endian();
  // if we're not a true dicom file, try acr/nema
  if (strcmp(dicm,"DICM")) {
    fseek(ifile,0,SEEK_SET);
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 111;
    if (group > 100) {
      swap(&group,1);
      if (my_endian()==ENDIAN_BIG)
        dci.byteorder=ENDIAN_LITTLE;
      else
        dci.byteorder=ENDIAN_BIG;
    }
    if (group!=8) { // ACR/NEMA files tend to start with group 8
      fclose(ifile);
      return 110;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  // otherwise, check endianness
  else {
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 111;
    if (group > 100) {
      if (my_endian()==ENDIAN_BIG)
        dci.byteorder=ENDIAN_LITTLE;
      else
        dci.byteorder=ENDIAN_BIG;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  while (1) {
    // read group,element,vr,count
    if (fread(&group,sizeof(int16),1,ifile)<1)
      break;
    if (fread(&element,sizeof(int16),1,ifile)<1)
      break;
    if (f_bigendian && group!=0x0002)
      dci.byteorder=ENDIAN_BIG;
    if (dci.byteorder!=my_endian()) {
      swap(&group,1);
      swap(&element,1);
    }

    if (fread(vr,1,2,ifile)<2)
      break;
    vr[2]='\0';
    // use "XX" for implicit value rep and back up for length (long)
    if (!isupper(vr[0]) || !isupper(vr[1])) {
      vr[0]=vr[1]='X';
      fseek(ifile,-2,SEEK_CUR);
    }
    string vrs=vr;  // convert to string for convenience

    if (vrs=="XX") {
      if (fread(&cnt,sizeof(int32),1,ifile) <1)
        break;
      if (dci.byteorder!=my_endian())
        swap(&cnt,1);
    }
    else if (vrs=="OB" || vrs=="OW" || vrs=="OF" || vrs=="SQ" || vrs=="UT" || vrs=="UN") {
      fseek(ifile,2,SEEK_CUR);
      if (fread(&cnt,sizeof(int32),1,ifile) <1)
        break;
      if (dci.byteorder!=my_endian())
        swap(&cnt,1);
    }
    else {
      int16 tmpc;
      if (fread(&tmpc,sizeof(int16),1,ifile)<1)
        break;
      if (dci.byteorder!=my_endian())
        swap(&tmpc,1);
      cnt=tmpc;
    }
    char buf[4096];
    string str;

    if (group==0x0002 && element==0x0010) {
      if (fread(buf,1,cnt,ifile)!=(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      if (str=="1.2.840.10008.1.2.2")
        f_bigendian=1;
    }
    else if (group==0x0008 && element==0x0008) {
      if (fread(buf,1,cnt,ifile)!=(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      if (str.find("MOSAIC") != string::npos)
        dci.mosaicflag=1;
    }
    else if (group==0x0008 && element==0x0022) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.date=str;
    }
    else if (group==0x0008 && element==0x0032) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.time=str;
    }
    //     else if (group==0x0008 && element==0x1030) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //       // e.g., BRAIN^ROUTINE ???
    //     }
    else if (group==0x0008 && element==0x103e) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.protocol=str;
    }
    //     else if (group==0x0008 && element==0x1090) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //     }


    //     else if (group==0x0010 && element==0x0010) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //     }
    //     else if (group==0x0010 && element==0x0020) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //     }
    else if (group==0x0010 && element==0x0030) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.dob=str;
      // date of birth
    }
    else if (group==0x0010 && element==0x0040) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.sex=str;
      // sex
    }
    else if (group==0x0010 && element==0x1010) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.age=str;
      // age of subject
    }


    //     else if (group==0x0018 && element==0x0020) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //       // sequence
    //     }
    //     else if (group==0x0018 && element==0x0021) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //       // sequence variant
    //     }
    //     else if (group==0x0018 && element==0x0022) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //       //cout << "Scan opts: " << str << endl;
    //     }
    //     else if (group==0x0018 && element==0x0023) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       str.append(buf,0,cnt);
    //       //cout << "Acq type: " << str << endl;
    //     }
    else if (group==0x0018 && element==0x0024) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.sequence=str;
      // sequence name
    }
    else if (group==0x0018 && element==0x0050) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.slthick=strtod(str);
      // slice thickness
    }
    else if (group==0x0018 && element==0x0080) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.tr=strtol(str);
      // tr
    }
    else if (group==0x0018 && element==0x0081) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.te=strtol(str);
      // echo time
    }
    else if (group==0x0018 && element==0x0082) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.ti=strtol(str);
      // inversion time
    }
    else if (group==0x0018 && element==0x0083) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.navg=strtol(str);
      // n averages
    }
    else if (group==0x0018 && element==0x0087) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.fieldstrength=strtod(str);
      // field strength
    }
    else if (group==0x0018 && element==0x0088) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.spacing=strtod(str);
      // slice spacing
    }
    else if (group==0x0018 && element==0x1030) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      if (xstripwhitespace(str).size()>0)
        dci.protocol=str;
    }
    else if (group==0x0018 && element==0x1250) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.receive_coil=str;
      // receive coil
    }
    else if (group==0x0018 && element==0x1251) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.transmit_coil=str;
    }
    else if (group==0x0018 && element==0x1310) {// && dci.mosaicflag) {
      // we'll use this as the matrix/slice size until we encounter
      // 0028.0010 and 0028.0011.  if we're mosaiced, those later
      // elements will give us the full matrix size. if we're not
      // mosaiced, those later elements will give us both the matrix
      // and the slice size
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      if (cnt==8) {
        if (dci.byteorder!=my_endian())
          swap((int16 *)buf,4);
        int16 *ss=(int16 *)buf;
        if (ss[0])
          dci.dimx=dci.rows=ss[0];
        else
          dci.dimx=dci.rows=ss[2];
        if (ss[3])
          dci.dimy=dci.cols=ss[3];
        else
          dci.dimy=dci.cols=ss[1];
      }
      // acquisition matrix
    }
    else if (group==0x0018 && element==0x1312) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.phaseencodedirection=xstripwhitespace(str);
      // phase encode direction
    }
    else if (group==0x0018 && element==0x1314) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.flipangle=strtod(str);
      // flip angle
    }

    // study id
    else if (group==0x0020 && element==0x0010) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.study=strtol(str);
    }
    // series
    else if (group==0x0020 && element==0x0011) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.series=strtol(str);
    }
    // acquisition (image number in the series)
    else if (group==0x0020 && element==0x0012) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.acquisition=strtol(str);
    }
    // instance
    else if (group==0x0020 && element==0x0013) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.instance=strtol(str);
    }
    // patient position
    else if (group==0x0020 && element==0x0032) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      args.SetSeparator(" \n\\");
      args.ParseLine(str);
      dci.position[0]=strtod(args[0]);
      dci.position[1]=strtod(args[1]);
      dci.position[2]=strtod(args[2]);
    }
    // images per acquisition (i.e., slices per volume, i.e., dimz)
    else if (group==0x0020 && element==0x1002) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      dci.slices=strtol(buf);
    }
    // patient orientation
    //     else if (group==0x0020 && element==0x0037) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       buf[cnt]='\0';
    //       str.append(buf,0,cnt);
    //     }
    // z position
    else if (group==0x0020 && element==0x1041) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.zpos=strtol(str);
      // z position
    }
    // movement parameters (siemens-specific?)
    else if (group==0x0020 && element==0x4000) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      tokenlist args;
      args.SetSeparator(" \t\n,:\\");
      args.ParseLine(buf);
      if (args[0]=="Motion" && args.size()>6) {
        dci.moveparam[0]=strtod(args[1]);
        dci.moveparam[1]=strtod(args[2]);
        dci.moveparam[2]=strtod(args[3]);
        dci.moveparam[3]=strtod(args[4]);
        dci.moveparam[4]=strtod(args[5]);
        dci.moveparam[5]=strtod(args[6]);
      }
    }
    
    // the following 2 fix some problems with older siemens mosaics
    else if (group==0x0021 && element==0x1340) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      tokenlist args;
      args.SetSeparator(" \t\n,:\\");
      args.ParseLine(str);
      dci.dimz=strtol(args[0]);
    }
    // FIXME when do we need to resort to the below instead of
    // acquisition matrix?
    else if (group==0x0028 && element==0x0010) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      int16 *pp=(int16 *)buf;
      if (dci.byteorder!=my_endian())
        swap(pp,1);
      // if (1||dci.phaseencodedirection=="ROW")
      //   dci.dimy=dci.rows=*pp;
      // else
      //   dci.dimx=dci.rows=*pp;
      dci.rows=*pp;
      if (!dci.mosaicflag) dci.dimy=dci.rows;
    }
    else if (group==0x0028 && element==0x0011) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      int16 *pp=(int16 *)buf;
      if (dci.byteorder!=my_endian())
        swap(pp,1);
      // if (1||dci.phaseencodedirection=="ROW")
      //   dci.dimx=dci.cols=*pp;
      // else
      //   dci.dimy=dci.cols=*pp;
      dci.cols=*pp;
      if (!dci.mosaicflag) dci.dimx=dci.cols;
    }
    else if (group==0x0028 && element==0x0030) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      args.SetSeparator(" \n\\");
      args.ParseLine(str);
      dci.voxsize[0]=strtod(args[0]);
      dci.voxsize[1]=strtod(args[1]);
      // xy voxel sizes
    }
    else if (group==0x0028 && element==0x0100) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      int16 *pp=(int16 *)buf;
      if (dci.byteorder!=my_endian())
        swap(pp,1);
      dci.bpp=*pp;
      // bits per pixel
    }
    else if (group==0x0028 && element==0x0101) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      int16 *pp=(int16 *)buf;
      if (dci.byteorder!=my_endian())
        swap(pp,1);
      dci.bps=*pp;
      // bits per pixel
    }
    //     else if (group==0x0028 && element==0x0101) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       } 
    //       if (dci.byteorder!=my_endian())
    // 	swap((int16 *)buf,1);
    //       // bits stored
    //     }
    //     else if (group==0x0028 && element==0x0102) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       // if (dci.byteorder!=my_endian())
    //       // swap((int16 *)buf,1);
    //       // cout << "high bit " << *((int16 *)buf) << endl;
    //     }
    //     else if (group==0x0028 && element==0x0103) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       // if (dci.byteorder!=my_endian())
    //       // swap((int16 *)buf,1);
    //       // cout << "pixel rep: " << *((int16 *)buf) << endl;
    //     }
    //     else if (group==0x0028 && element==0x0106) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       // if (dci.byteorder!=my_endian())
    //       // swap((int16 *)buf,1);
    //       // cout << "small value: " << *((int16 *)buf) << endl;
    //     }
    //     else if (group==0x0028 && element==0x0107) {
    //       if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
    //         fclose(ifile);
    //         return 105;
    //       }
    //       // if (dci.byteorder!=my_endian())
    //       // swap((int16 *)buf,1);
    //       //cout << "high value: " << *((int16 *)buf) << endl;
    //     }
    // WINDOW CENTER
    else if (group==0x0028 && element==0x1050) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.win_center=strtod(str);
    }
    // WINDOW WIDTH
    else if (group==0x0028 && element==0x1051) {
      if (fread(buf,1,cnt,ifile)<(uint32)cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      str.append(buf,0,cnt);
      dci.win_width=strtod(str);
    }
    
    else if (group==0x0029 && element==0x1020) {
      char buf[cnt];
      if (fread(buf,1,cnt,ifile)<(uint32) cnt) {
        fclose(ifile);
        return 105;
      }
      buf[cnt]='\0';
      parse_siemens_stuff(buf,cnt,dci);
    }
      
    else if (group==0x7fe0 && element==0x0010) {
      dci.offset=ftell(ifile);
      dci.datasize=cnt;
      fseek(ifile,cnt,SEEK_CUR);
    }
    else if ((vrs=="SQ"||vrs=="XX") && cnt==0xffffffff) {
      while (TRUE) {  // keep reading tags and lengths until we hit sentinel
        if (fread(&group,sizeof(int16),1,ifile) <1)
          break;
        if (fread(&element,sizeof(int16),1,ifile) <1)
          break;
        if (fread(&cnt,sizeof(int32),1,ifile) <1)  // read and swap count
          break;
        if (dci.byteorder!=my_endian()) {
          swap(&group,1);
          swap(&element,1);
          swap(&cnt,1);
        }
        if (group==0xfffe && element==0xe0dd)
          break;
        if (cnt==0xffffffff) {
          while(TRUE) {
            if (fread(&group,sizeof(int16),1,ifile)<1)
              break;
            if (dci.byteorder!=my_endian())
              swap(&group,1);
            if (group!=0xfffe) continue;
            if (fread(&group,sizeof(int16),1,ifile)<1)
              break;
            if (dci.byteorder!=my_endian())
              swap(&group,1);
            if (group==0xe00d)
              break;
          }
          fseek(ifile,4,SEEK_CUR);
        }
        else
          fseek(ifile,cnt,SEEK_CUR);
      }
    }
    else {
      fseek(ifile,cnt,SEEK_CUR);
    }
  }
  fclose(ifile);

  // the array size of each slice as stored is not reliably stored
  // anywhere for siemens mosaics, nor is the geometry of the mosaic.
  // we can get the slice size in voxels by dividing the field of view
  // in mm by the voxel sizes.

  // for other cases, we can get what we need from the combination of
  // 0018.1310 and 0028.0010/11.

  if (dci.xfov && dci.mosaicflag) {
    dci.dimx=lround(dci.xfov/dci.voxsize[0]);
    dci.dimy=lround(dci.yfov/dci.voxsize[1]);
  }

  // cleanup voxel size
  if (dci.spacing > 0.0)
    dci.voxsize[2]=dci.spacing;
  else
    dci.voxsize[2]=dci.slthick;

  // older siemens format
  if (dci.cols != dci.dimx && dci.cols!=dci.dimy)
    dci.mosaicflag=1;
  
  // cleanup mosaic issues
  if (dci.mosaicflag) {
    dci.position[0]=dci.spos[0]-((dci.dimx/2.0)*dci.voxsize[0]);
    dci.position[1]=dci.spos[1]-((dci.dimy/2.0)*dci.voxsize[1]);
    // position[2] should be correct in the header
  }
  // adjust position to reflect that we're going to flip the data
  dci.position[1]+=dci.voxsize[1]*dci.dimy;
  dci.position[1]*=-1.0;
  return 0;
}

int
print_dicom_header(string filename)
{
  FILE *ifile=fopen(filename.c_str(),"r");
  if (!ifile)
    return 105;
  uint16 group,element;
  char vr[5];   // value representation
  char dicm[5];
  uint32 cnt;
  uint32 i;
  VB_byteorder byteorder;
  dicomnames nn;    // converts group/element to a string description
  bool f_bigendian=0;

  fseek(ifile,128,SEEK_SET);
  fread(dicm,1,4,ifile);
  dicm[4]='\0';
  // if we're not a true dicom file, try acr/nema
  byteorder=my_endian();
  if (strcmp(dicm,"DICM")) {
    fseek(ifile,0,SEEK_SET);
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 111;
    if (group > 100) {
      swap(&group,1);
      if (my_endian()==ENDIAN_BIG)
        byteorder=ENDIAN_LITTLE;
      else
        byteorder=ENDIAN_BIG;
    }
    if (group!=8) { // ACR/NEMA files tend to start with group 8
      fclose(ifile);
      return 110;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  // otherwise, check endianness
  else {
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 111;
    if (group > 100) {
      if (my_endian()==ENDIAN_BIG)
        byteorder=ENDIAN_LITTLE;
      else
        byteorder=ENDIAN_BIG;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  cout << " GRP.ELEM (sz,VR) [description]: value\n";
  while (1) {
    // read group, element,count
    if (fread(&group,sizeof(int16),1,ifile)<1)
      break;
    if (fread(&element,sizeof(int16),1,ifile)<1)
      break;
    if (fread(vr,1,2,ifile)<2)
      break;
    if (f_bigendian && group!=0x0002)
      byteorder=ENDIAN_BIG;
    if (byteorder!=my_endian()) { 
      swap(&group,1);
      swap(&element,1);
    }
    vr[2]='\0';
    // use "XX" for implicit value rep and back up for length (long)
    if (!isupper(vr[0]) || !isupper(vr[1])) {
      vr[0]=vr[1]='X';
      fseek(ifile,-2,SEEK_CUR);
    }
    string vrs=vr;  // convert to string for convenience

    if (vrs=="XX") {
      if (fread(&cnt,sizeof(int32),1,ifile) <1)
        break;
      if (byteorder!=my_endian())
        swap(&cnt,1);
    }
    else if (vrs=="OB" || vrs=="OW" || vrs=="OF" || vrs=="SQ" ||
             vrs=="UT" || vrs=="UN") {
      fseek(ifile,2,SEEK_CUR);
      if (fread(&cnt,sizeof(int32),1,ifile) <1)
        break;
      if (byteorder!=my_endian())
        swap(&cnt,1);
    }
    else {
      int16 tmpc;
      if (fread(&tmpc,sizeof(int16),1,ifile)<1)
        break;
      if (byteorder!=my_endian())
        swap(&tmpc,1);
      cnt=tmpc;
    }

    // cout << format("group.element %04x.%04x (%d,%d) ")%group%element%cnt%vr;
    string desc=nn(group,element);
    if (desc.empty())
      cout << format("%04x.%04x (%2d,%s): ")%group%element%cnt%vr;
    else
      cout << format("%04x.%04x (%2d,%s) [%s]: ")%group%element%cnt%vr%desc;
    
    if (vrs=="AE" || vrs=="CS" || vrs=="DA" || vrs=="DS" || vrs=="IS" ||
        vrs=="LO" || vrs=="PN" || vrs=="SH" || vrs=="ST" || vrs=="TM" ||
        vrs=="UI" || vrs=="LT" || vrs=="AS") {
      unsigned char bb[cnt+1];
      fread(bb,1,cnt,ifile);
      bb[cnt]='\0';
      cout << bb << endl;
      if (group==0x0002 && element==0x0010) {
        string ts=(char *)bb;
        if (ts=="1.2.840.10008.1.2.2")
          f_bigendian=1;
      }
    }
    else if (vrs == "XX" && cnt < 120) {
      unsigned char bb[cnt+1];
      fread(bb,1,cnt,ifile);
      for (i=0; i<cnt; i++)
        if (((int32)bb[i]) > 31 && ((int32)bb[i]) <127)
          cout << bb[i];
      cout << endl;      
    }
    else if ((vrs=="SQ"||vrs=="XX") && cnt==0xffffffff) {
      cout << endl;
      while (TRUE) {  // keep reading tags and lengths until we hit sentinel
        if (fread(&group,sizeof(int16),1,ifile) <1)
          break;
        if (fread(&element,sizeof(int16),1,ifile) <1)
          break;
        if (fread(&cnt,sizeof(int32),1,ifile) <1)  // read and swap count
          break;
        if (byteorder!=my_endian()) {
          swap(&group,1);
          swap(&element,1);
          swap(&cnt,1);
        }
        cout << format("--> implicit group.element %04x.%04x\n")%group%element;
        if (group==0xfffe && element==0xe0dd)
          break;
        if (cnt==0xffffffff) {
          while(TRUE) {
            if (fread(&group,sizeof(int16),1,ifile)<1)
              break;
            if (byteorder!=my_endian())
              swap(&group,1);
            if (group!=0xfffe) continue;
            if (fread(&group,sizeof(int16),1,ifile)<1)
              break;
            if (byteorder!=my_endian())
              swap(&group,1);
            if (group==0xe00d)
              break;
          }
          fseek(ifile,4,SEEK_CUR);
        }
        else
          fseek(ifile,cnt,SEEK_CUR);
      }
    }
    else if (vrs=="OB") {
      unsigned char bb[cnt+1];
      fread(bb,1,cnt,ifile);
      bb[cnt]='\0';
      cout << endl;
    }
    else if (vrs=="UL") {
      cout << cnt << " ";
      uint32 tmp;
      fread(&tmp,sizeof(int32),1,ifile);
      if (byteorder!=my_endian())
        swap(&tmp,1);
      cout << tmp << endl;
    }
    else if (vrs=="US") {
      uint16 tmp;
      for (i=0; i<(cnt/sizeof(uint16)); i++) {
        fread(&tmp,sizeof(uint16),1,ifile);
        if (byteorder!=my_endian())
          swap((int16 *)&tmp,1);
        cout << tmp << " ";
      }
      cout << endl;
    }
    else {
      cout << endl;
      fseek(ifile,cnt,SEEK_CUR);
    }
  }
  return 0;
}

// FIXME anonymize_dicom_header() has serious problems with closing
// files on error conditions.  should switch to fstream.

int
anonymize_dicom_header(string infile,string out1,string out2,
                       set<uint16> &stripgroups,set<dicomge> &stripges,
                       set<string> &stripvrs,int &removedfields)
{
  const uint16 customgroup=0x1119;
  const string anonstring="voxbo_anon: de-identified by voxbo (version "+vbversion+") on "+timedate();
  const string idstring="voxbo_id: "+VBRandom_nchars(30);
  struct stat st;
  string out1tmp=out1+"_dcmsplit_tmp";
  string out2tmp=out2+"_dcmsplit_tmp";
  if (stat(infile.c_str(),&st))
    return 901;
  FILE *ifile=fopen(infile.c_str(),"r");
  if (!ifile)
    return 105;
  FILE *ofile1=NULL;
  if (out1.size()) {
    ofile1=fopen(out1tmp.c_str(),"w");
    if (!ofile1) {
      fclose(ifile);
      return 106;
    }
  }
  FILE *ofile2=NULL;
  if(out2.size()) {
    ofile2=fopen(out2tmp.c_str(),"w");
    if (!ofile2) {
      fclose(ifile);
      fclose(ofile1);
      unlink(out1tmp.c_str());
      return 107;
    }
  }

  VB_byteorder byteorder;
  bool f_bigendian=0;
  uint16 group,element;
  uint16 gg,ee,lastgroup=0;
  char vr[5];   // value representation
  char dicm[5];
  uint32 cnt;
  tokenlist args;
  bool f_err;
  removedfields=0;   // storage passed by caller

  fseek(ifile,128,SEEK_SET);
  if (fread(dicm,1,4,ifile)!=4) {
    fclose(ifile);
    if (ofile1) {fclose(ofile1); unlink(out1tmp.c_str());}
    if (ofile2) {fclose(ofile2); unlink(out2tmp.c_str());}
    return 201;  // >200 means not-a-dicom-file
  }
  dicm[4]='\0';
  byteorder=my_endian();
  // if we're not a true dicom file, try acr/nema
  if (strcmp(dicm,"DICM")) {
    fseek(ifile,0,SEEK_SET);
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 202;  // >200 means not-a-dicom-file
    if (group > 100) {
      swap(&group,1);
      if (my_endian()==ENDIAN_BIG)
        byteorder=ENDIAN_LITTLE;
      else
        byteorder=ENDIAN_BIG;
    }
    if (group!=8) { // ACR/NEMA files tend to start with group 8
      fclose(ifile);
      if (ofile1) {fclose(ofile1); unlink(out1tmp.c_str());}
      if (ofile2) {fclose(ofile2); unlink(out2tmp.c_str());}
      return 203;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  // otherwise, write the first 132 bytes and check endianness
  else {
    char stub[132];
    memset(stub,0,128);
    strncpy(stub+128,"DICM",4);
    f_err=0;
    if (ofile1) if (fwrite(stub,1,132,ofile1)!=132) f_err=1;
    if (ofile2) if (fwrite(stub,1,132,ofile2)!=132) f_err=1;
    if (f_err) {
      fclose(ifile);
      if (ofile1) {fclose(ofile1); unlink(out1tmp.c_str());}
      if (ofile2) {fclose(ofile2); unlink(out2tmp.c_str());}
      return 119;
    }
    if (fread(&group,sizeof(int16),1,ifile)<1)
      return 204;  // magic number for not-a-dicom-file
    if (group > 100) {
      if (my_endian()==ENDIAN_BIG)
        byteorder=ENDIAN_LITTLE;
      else
        byteorder=ENDIAN_BIG;
    }
    fseek(ifile,0-sizeof(int16),SEEK_CUR);
  }
  bool f_readerr=0,f_writeerr=0;
  while (1) {
    // read group,element,vr,count
    if (fread(&group,sizeof(int16),1,ifile)<1) {
      break;   // don't set readerr here, it's okay to not have a next element!
    }
    if (fread(&element,sizeof(int16),1,ifile)<1) {
      f_readerr=1;
      break;
    }
    if (f_bigendian && group !=0x0002)
      byteorder=ENDIAN_BIG;
    gg=group;
    ee=element;
    if (byteorder!=my_endian()) {
      swap(&gg,1);
      swap(&ee,1);
    }

    if (fread(vr,1,2,ifile)<2) {
      f_readerr=1;
      break;
    }
    vr[2]='\0';
    // use "XX" for implicit value rep and back up for length (long)
    if (!isupper(vr[0]) || !isupper(vr[1])) {
      vr[0]=vr[1]='X';
      fseek(ifile,-2,SEEK_CUR);
    }
    string vrs=vr;  // convert to string for convenience

    // cout << format("%04x.%04x %s\n")%group%element%vrs;

    // insert custom tags describing de-identification
    if (lastgroup<customgroup && gg>customgroup) {
      if (ofile1) {
        write_LO(ofile1,byteorder,customgroup,0x0001,anonstring);
        write_LO(ofile1,byteorder,customgroup,0x0002,idstring);
      }
      if (ofile2) {
        write_LO(ofile2,byteorder,customgroup,0x0001,anonstring);
        write_LO(ofile2,byteorder,customgroup,0x0002,idstring);
      }
    }
    lastgroup=gg;

    // HERE'S WHERE WE IDENTIFY PHI AND SET THE STRIP FLAG

    bool f_strip=0;
    // all of groups 0010 (), 0012 (clinical trial info), 0032, 0038 (admission info)
    if (gg==0x0010 || gg==0x0012 || gg==0x0032 || gg==0x0038)
      f_strip=1;
    // person name or other unique identifier
    else if (vrs=="PN")
      f_strip=1;
    else if (stripgroups.count(gg))
      f_strip=1;
    else if (stripges.count(dicomge(gg,ee)))
      f_strip=1;
    else if (stripvrs.count(vrs))
      f_strip=1;

    if (f_strip) removedfields++;

    // first write the unswapped group/element numbers
    if (ofile1) {
      if (fwrite(&group,sizeof(int16),1,ofile1)!=1) {
        f_writeerr=1;
        break;
      }
      if (fwrite(&element,sizeof(int16),1,ofile1)!=1) {
        f_writeerr=1;
        break;
      }
    }
    if (ofile2 && f_strip) {
      if (fwrite(&group,sizeof(int16),1,ofile2)!=1) {
        f_writeerr=1;
        break;
      }
      if (fwrite(&element,sizeof(int16),1,ofile2)!=1) {
        f_writeerr=1;
        break;
      }
    }

    // now get and write the cnt
    if (vrs=="XX") {
      if (fread(&cnt,sizeof(int32),1,ifile) <1) {
        f_readerr=1;
        break;
      }
      if (ofile1) {
        if (fwrite(&cnt,sizeof(int32),1,ofile1)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (ofile2 && f_strip) {
        if (fwrite(&cnt,sizeof(int32),1,ofile2)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (byteorder!=my_endian())
        swap(&cnt,1);
    }
    else if (vrs=="OB" || vrs=="OW" || vrs=="OF" || vrs=="SQ" || vrs=="UT" || vrs=="UN") {
      // explicit rep, write it out, plus filler
      if (ofile1) {
        // to strip SQ's, we change the VR to UN.  FIXME: this is not
        // ideal!
        string vrstmp=vrs;
        if (vrs=="SQ" && f_strip)
          vrstmp="UN";
        // for just 0032.1064, we change UT to UN.  this compensates
        // for an earlier bug in dcmsplit
        if (group==0x0032 && element==0x1064 && vrs=="UT" && f_strip)
          vrstmp="UN";
        fwrite(vrstmp.c_str(),1,2,ofile1);
        fwrite("  ",1,2,ofile1);
      }
      if (ofile2 && f_strip) {
        fwrite(vrs.c_str(),1,2,ofile2);
        fwrite("  ",1,2,ofile2);
      }
      // skip filler
      fseek(ifile,2,SEEK_CUR);
      if (fread(&cnt,sizeof(int32),1,ifile) <1) {
        f_readerr=1;
        break;
      }
      if (ofile1) {
        if (fwrite(&cnt,sizeof(int32),1,ofile1)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (ofile2 && f_strip) {
        if (fwrite(&cnt,sizeof(int32),1,ofile2)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (byteorder!=my_endian())
        swap(&cnt,1);
    }
    else {
      // explicit rep, write it out
      if (ofile1)
        fwrite(vrs.c_str(),1,2,ofile1);
      if (ofile2 && f_strip)
        fwrite(vrs.c_str(),1,2,ofile2);
      int16 tmpc;
      if (fread(&tmpc,sizeof(int16),1,ifile)<1) {
        f_readerr=1;
        break;
      }
      if (ofile1) {
        if (fwrite(&tmpc,sizeof(int16),1,ofile1)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (ofile2 && f_strip) {
        if (fwrite(&tmpc,sizeof(int16),1,ofile2)<1) {
          f_writeerr=1;
          break;
        }
      }
      if (byteorder!=my_endian())
        swap(&tmpc,1);
      cnt=tmpc;
    }

    // now copy the data
    const uint32 bufsz=1024*1024;
    char buf[bufsz];
    string str;

    size_t readcnt;
    int pass=1;
    if ((vrs=="SQ"||vrs=="XX") && cnt==0xffffffff) {
      // cout << "parsing delimited " << vrs  << endl;
      while (1) {  // keep reading tags and lengths until we hit sentinel
        if (fread(&group,sizeof(int16),1,ifile) <1)
          {f_readerr=1;break;}
        if (fread(&element,sizeof(int16),1,ifile) <1)
          {f_readerr=1;break;}
        if (fread(&cnt,sizeof(int32),1,ifile) <1)  // read and swap count
          {f_readerr=1;break;}
        if (ofile1) {
          fwrite(&group,sizeof(int16),1,ofile1);
          fwrite(&element,sizeof(int16),1,ofile1);
          fwrite(&cnt,sizeof(int32),1,ofile1);
        }
        if (ofile2 && f_strip) {
          fwrite(&group,sizeof(int16),1,ofile2);
          fwrite(&element,sizeof(int16),1,ofile2);
          fwrite(&cnt,sizeof(int32),1,ofile2);
        }
        
        if (byteorder!=my_endian()) {
          swap(&group,1);
          swap(&element,1);
          swap(&cnt,1);
        }
        //cout << format("--> implicit group.element %04x.%04x\n")%group%element;
        if (group==0xfffe && element==0xe0dd)
          break;
        if (cnt==0xffffffff) {
          while(1) {
            if (fread(&group,sizeof(int16),1,ifile)<1)
              {f_readerr=1;break;}
            if (ofile1)
              fwrite(&group,sizeof(int16),1,ofile1);
            if (ofile2 && f_strip)
              fwrite(&group,sizeof(int16),1,ofile2);
            if (byteorder!=my_endian())
              swap(&group,1);
            if (group!=0xfffe) continue;
            if (fread(&group,sizeof(int16),1,ifile)<1)
              {f_readerr=1;break;}
            if (ofile1)
              fwrite(&group,sizeof(int16),1,ofile1);
            if (ofile2 && f_strip)
              fwrite(&group,sizeof(int16),1,ofile2);
            if (byteorder!=my_endian())
              swap(&group,1);
            if (group==0xe00d)
              break;
          }
          // fseek(ifile,4,SEEK_CUR);
          if (fread(buf,1,4,ifile)<4)
            {f_readerr=1;break;}
          if (ofile1)
            fwrite(buf,1,4,ofile1);
          if (ofile2 && f_strip)
            fwrite(buf,1,4,ofile2);
        }
        else {
          //fseek(ifile,cnt,SEEK_CUR);
          if (cnt>bufsz)
            {f_readerr=1;break;}
          if (fread(buf,1,cnt,ifile)<cnt)
            {f_readerr=1;break;}
          if (ofile1)
            fwrite(buf,1,cnt,ofile1);
          if (ofile2 && f_strip)
            fwrite(buf,1,cnt,ofile2);
        }
      }
    }
    else {
      // cout << "parsing " << vrs << endl;
      while (1) {
        readcnt=cnt;
        if (readcnt>bufsz) readcnt=bufsz;
        if (fread(buf,1,readcnt,ifile)!=readcnt) {
          f_readerr=1;
          break;
        }
        if (ofile2 && f_strip) {
          if (fwrite(buf,1,readcnt,ofile2)!=readcnt) {
            f_writeerr=1;
            break;
          }
        }
        // parse the transfer syntax for endianness
        if (gg==0x0002 && ee==0x0010) {
          buf[cnt]='\0';
          string ts=(char *)buf;
          if (ts=="1.2.840.10008.1.2.2")
            f_bigendian=1;
        }
        // update voxbo anon date field
        if (group==customgroup && element==0x0001 && pass==1 && readcnt>5) {
          if (!strncmp(buf,"voxbo",5))
            strcpy(buf,anonstring.c_str());
        }
        // ZERO THE DATA BEFORE WRITING OUT ANON FILE (for text types,
        // set string to ANON)
        if (f_strip) {
          memset(buf,0x20,readcnt);
          if ((vrs=="AE" || vrs=="CS" || vrs=="PN" || vrs=="ST" ||
               vrs=="UI" || vrs=="LT") && pass==1 && readcnt>4)
            strncpy(buf,"ANON",4);
          else if (vrs=="DA")
            memcpy(buf,"19000101",8);
          else if ((vrs=="DS" || vrs=="IS"||vrs=="LO"||vrs=="SH") && readcnt>1)
            strncpy(buf,"0",1);
          else if (vrs=="TM" && readcnt>6)
            strncpy(buf,"000000",6);
          else if (vrs=="AS")
            memcpy(buf,"999Y",4);
        }
        
        if (ofile1) {
          if (fwrite(buf,1,readcnt,ofile1)!=readcnt) {
            f_writeerr=1;
            break;
          }
        }
        cnt-=readcnt;
        if (cnt==0)
          break;
        pass++;
      }
    }
    // if we exited the above with an error, get out
    if (f_readerr||f_writeerr)
      break;
  }
  // if we need to, and didn't have a chance previously, add voxbo tags
  if (lastgroup<customgroup) {
    if (ofile1) {
      write_LO(ofile1,byteorder,customgroup,0x0001,anonstring);
      write_LO(ofile1,byteorder,customgroup,0x0002,idstring);
    }
    if (ofile2) {
      write_LO(ofile2,byteorder,customgroup,0x0001,anonstring);
      write_LO(ofile2,byteorder,customgroup,0x0002,idstring);
    }
  }

  fclose(ifile);
  if (ofile1) fclose(ofile1);
  if (ofile2) fclose(ofile2);
  // use the output files iff we removed data and we were not interrupted mid-element
  if (removedfields && !f_readerr && !f_writeerr) {
    if (ofile1) {
      if (rename(out1tmp.c_str(),out1.c_str()))
        return 180;
      // restore original mode, owner, and group if possible FIXME this is UNIX-specific
      chmod(out1.c_str(),st.st_mode);
      chown(out1.c_str(),st.st_uid,st.st_gid);
    }
    if (ofile2) {
      if (rename(out2tmp.c_str(),out2.c_str()))
        return 181;
      // restore original mode, owner, and group if possible FIXME this is UNIX-specific
      chmod(out2.c_str(),st.st_mode);
      chown(out2.c_str(),st.st_uid,st.st_gid);
    }
  }
  else {
    if (ofile1) unlink(out1tmp.c_str());
    if (ofile2) unlink(out2tmp.c_str());
  }
  // if we broke while reading an element, return an error
  if (f_readerr) return 205;
  if (f_writeerr) return 106;
  return 0;
}

void
write_LO(FILE *ofile,VB_byteorder byteorder,uint16 group,uint16 element,string otag)
{
  if (otag.size()%2) otag+=" ";  // tags must be even length
  int16 cc=otag.size();
  if (byteorder!=my_endian()) {
    swap(&group,1);
    swap(&element,1);
    swap(&cc,1);
  }
  fwrite(&group,2,1,ofile);
  fwrite(&element,2,1,ofile);
  fwrite("LO",2,1,ofile);
  fwrite(&cc,2,1,ofile);
  fwrite(otag.c_str(),otag.size(),1,ofile);
}

void
transfer_dicom_header(dicominfo &dci,VBImage &im)
{
  stringstream tmps;
  tmps.str("");
  tmps << setfill('0')
       << "SeriesName: "
       << setw(4) << dci.series
       << "_"
       << xstripwhitespace(dci.protocol)
    ;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << setfill('0')
       << "FileName: "
       << setw(4) << dci.instance
       << "_"
       << xstripwhitespace(dci.date)
       << "_"
       << xstripwhitespace(dci.time)
    ;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "PulseSequence: " << dci.sequence;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "Protocol: " << dci.protocol;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "DateTime: " << dci.date << " " << dci.time;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "TE(msecs): " << dci.te;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "TI(msecs): " << dci.ti;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "navg: " << dci.navg;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "FieldStrength: " << dci.fieldstrength;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "SliceThickness: " << dci.slthick;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "SliceSpacing: " << dci.spacing;
  im.AddHeader(tmps.str());

  if (dci.transmit_coil.size()) {
    tmps.str("");
    tmps << "TransmitCoil: " << dci.transmit_coil;
    im.AddHeader(tmps.str());
  }
  if (dci.receive_coil.size()) {
    tmps.str("");
    tmps << "ReceiveCoil: " << dci.receive_coil;
    im.AddHeader(tmps.str());
  }

  tmps.str("");
  tmps << "AbsoluteCornerPosition: "
       << dci.position[0] << " "
       << dci.position[1] << " "
       << dci.position[2];
  im.AddHeader(tmps.str());

  tmps.str("");
  tmps << "MosaicFlag: " << dci.mosaicflag;
  im.AddHeader(tmps.str());

  //   tmps.str("");
  //   tmps << "MosaicGuess: " << (dci.rows != dci.dimx ? 1 : 0);
  //   im.AddHeader(tmps.str());

  tmps.str("");
  tmps << "DOB/Age/Sex: " << dci.dob << " / " << dci.age << " / " << dci.sex;
  im.AddHeader(tmps.str());

  tmps.str("");
  tmps << "PhaseEncodeDirection: " << dci.phaseencodedirection;
  im.AddHeader(tmps.str());

  tmps.str("");
  tmps << "window_center: " << dci.win_center;
  im.AddHeader(tmps.str());
  tmps.str("");
  tmps << "window_width: " << dci.win_width;
  im.AddHeader(tmps.str());

  if (dci.moveparam[0]<99998) {
    tmps.str("");
    tmps << "MoveParams: " << dci.moveparam[0] << " "
         << dci.moveparam[1] << " "
         << dci.moveparam[2] << " "
         << dci.moveparam[3] << " "
         << dci.moveparam[4] << " "
         << dci.moveparam[5];
    im.AddHeader(tmps.str());
  }

  im.voxsize[0]=(float)dci.voxsize[0];
  im.voxsize[1]=(float)dci.voxsize[1];
  im.voxsize[2]=(float)dci.voxsize[2];
  im.voxsize[3]=(float)dci.tr;
  im.dimx=dci.dimx;
  im.dimy=dci.dimy;
  im.dimz=dci.dimz;
  // FIXME need to be more sophisticated about data types
  im.SetDataType(vb_byte);
  if (dci.bps>8)
    im.SetDataType(vb_short);
  if (dci.bps>16)
    im.SetDataType(vb_long);
  im.orient="RPI";
  if (im.dimx>0 && im.dimy>0 && im.dimz>0)
    im.header_valid=1;
}

int
parse_siemens_stuff(char *buf,int len,dicominfo &dci)
{
  // find the beginning of the weird siemens header
  int loc=0;
  for (int i=0; i<len-1-(int32)strlen(SIEMENS_TAG); i++) {
    if (!strncmp(buf+i,SIEMENS_TAG,strlen(SIEMENS_TAG))) {
      loc=i;
      break;
    }
  }
  // if we didn't find it, exit
  if (loc==0)
    return 105;
  
  tokenlist args;
  args.SetSeparator(" \n\t=");
  while(loc<len) {
    string line;
    while (loc < len && buf[loc]!='\n') {
      line+=buf[loc++];
    }
    loc++;  // increment for next time
    if (line==SIEMENS_TAG_END)
      break;
    args.ParseLine(line);
    // parse the line

    if (args[0]=="sSliceArray.asSlice[0].dPhaseFOV") {
      if (dci.phaseencodedirection=="ROW")
        dci.xfov=strtol(args[1]);
      else
        dci.yfov=strtol(args[1]);
      // dci.xfov=strtol(args[1]);
    }
    else if (args[0]=="sSliceArray.asSlice[0].dReadoutFOV") {
      if (dci.phaseencodedirection=="ROW")
        dci.yfov=strtol(args[1]);
      else
        dci.xfov=strtol(args[1]);
      // dci.yfov=strtol(args[1]);
    }
    else if (args[0]=="sKSpace.lBaseResolution" && dci.mosaicflag) {
      if (dci.phaseencodedirection=="ROW")
        dci.dimy=strtol(args[1]);
      else
        dci.dimx=strtol(args[1]);
    }
    else if (args[0]=="sKSpace.lPhaseEncodingLines" && dci.mosaicflag) {
      if (dci.phaseencodedirection=="ROW")
        dci.dimx=strtol(args[1]);
      else
        dci.dimy=strtol(args[1]);
    }
    else if (args[0]=="sGroupArray.asGroup[0].dDistFact") {
      dci.skip=strtod(args[1]);
    }
    // for mosaics, we put nslices per volume in dimz (slices in this file)
    else if (args[0]=="sSliceArray.lSize" && dci.mosaicflag) {
      int z=strtol(args[1]);
      if (z>1)
        dci.dimz=z;
    }
    // for non-mosaics, we let dimz==1, but set slices to the correct number
    else if (args[0]=="sSliceArray.lSize" && !dci.mosaicflag) {
      int z=strtol(args[1]);
      if (z>1)
	dci.slices=z;
    }
    else if (args[0]=="sSliceArray.asSlice[0].sPosition.dSag") {
      dci.spos[0]=strtod(args[1]);
    }
    else if (args[0]=="sSliceArray.asSlice[0].sPosition.dCor") {
      dci.spos[1]=strtod(args[1]);
    }
    else if (args[0]=="sSliceArray.asSlice[0].sPosition.dTra") {
      dci.spos[2]=strtod(args[1]);
    }
    // sSliceArray.asSlice[0].sNormal.dTra      = 1
    // sSliceArray.asSlice[0].dThickness        = 5

    // sSliceArray.asSlice[0].dPhaseFOV         = 192
    // sSliceArray.asSlice[0].dReadoutFOV       = 192

    // sSliceArray.lSize                        = 18
    // sKSpace.lBaseResolution                  = 64   [ x ]
    // sKSpace.lPhaseEncodingLines              = 64   [ y ]
    // lRepetitions                             = 119  [ reps-1? ]
    // lDelayTimeInTR                           = 500000
    // dFlipAngleDegree                         = 90
    // lScanTimeSec                             = 2
    // lTotalScanTimeSec                        = 245
    // sCOIL_SELECT_MEAS.asList[0].sCoilElementID.tCoilID = "CP_HeadArray"
    // sCOIL_SELECT_MEAS.asList[0].sCoilElementID.lCoilCopy = 1
    // sCOIL_SELECT_MEAS.asList[0].sCoilElementID.tElement = "HE"
    // sCOIL_SELECT_MEAS.asList[0].lElementSelected = 1
    // sCOIL_SELECT_MEAS.asList[0].lRxChannelConnected = 1


  }
  return 0;
}

// READ 3D DATA FROM MULTIPLE FILES, ONE SLICE TO A FILE

int
read_multiple_slices(Cube *cb,tokenlist &filenames)
{
  dicominfo dci;

  if (read_dicom_header(filenames[0],dci))
    return 120;
  dci.dimz=filenames.size();
  
  if (dci.dimx == 0 || dci.dimy == 0 || dci.dimz == 0)
    return 105;
  cb->SetVolume(dci.dimx,dci.dimy,dci.dimz,vb_short);
  if (!cb->data_valid)
    return 120;
  int slicesize=dci.dimx*dci.dimy*cb->datasize;
  int rowsize=dci.dimx*cb->datasize;

  unsigned char *newdata=new unsigned char[dci.datasize];
  if (!newdata)
    return 150;
  for (size_t i=0; i<(uint32)dci.dimz; i++) {
    // prematurely out of slices, no complaint i guess
    if (i>filenames.size()-1)
      break;
    dicominfo dci2;
    if (read_dicom_header(filenames[i],dci2))
      continue;
    // FIXME!
    //     if (dci2.datasize<slicesize)
    //       continue;
    FILE *ifile = fopen(filenames(i),"r");
    if (!ifile)
      continue;
    fseek(ifile,dci2.offset,SEEK_SET);
    // FIXME if dci2.datasize is bigger than dci.datasize, following will blow up
    int cnt=fread(newdata,1,dci2.datasize,ifile);
    fclose(ifile);
    mask_dicom(dci2,newdata);
    if (cnt < dci2.datasize)
      continue;
    // the following junk inverts the slices in y
    for (int j=0; j<dci.dimy; j++) {
      memcpy(cb->data+(slicesize*i)+((cb->dimy-1-j)*rowsize),
             newdata+(j*rowsize),dci.dimx*cb->datasize);
    }
    // OLD non-inverting code: memcpy(cb->data+(slicesize*i),newdata,slicesize);
  }
  if (dci.byteorder!=my_endian())
    cb->byteswap();
  return 0;
}

void
mask_dicom(dicominfo &dci,unsigned char *buf)
{
  if (dci.bpp==32) {
    uint32 mask,*p=(uint32 *)buf;
    mask=0xffffffff >> (dci.bpp-dci.bps);
    for (int i=0; i<dci.datasize/4; i++) {
      p[i]=p[i]&mask;
    }
  }
  else if (dci.bpp==16) {
    uint16 mask,*p=(uint16 *)buf;
    mask=0xffff >> (dci.bpp-dci.bps);
    for (int i=0; i<dci.datasize/2; i++) {
      p[i]=p[i]&mask;
    }
  }
  else if (dci.bpp==8) {
    uint8 mask,*p=(uint8 *)buf;
    mask=0xff >> (dci.bpp-dci.bps);
    for (int i=0; i<dci.datasize; i++) {
      p[i]=p[i]&mask;
    }
  }
  // FIXME can only handle masking for those three
}

string
patfromname(const string fname)
{
  struct stat st;

  string pat=fname;
  // if it's a stem
  if (stat(pat.c_str(),&st))
    pat+="*";
  // if it's a dir
  else if (S_ISDIR(st.st_mode))
    pat+="/*";
  return pat;
}

int
read_head_dcm3d_3D(Cube *cb)
{
  dicominfo dci;
  stringstream tmps;
  int filecount=1;

  string fname=cb->GetFileName();
  string pat=patfromname(fname);
  if (pat != fname) {
    vglob vg(pat);
    filecount=vg.size();
    if (filecount<1)
      return 120;
    fname=vg[0];
  }

  if (read_dicom_header(fname,dci))
    return 105;

  for (int i=0; i<(int)dci.protocol.size(); i++) {
    if (dci.protocol[i]==' ')
      dci.protocol[i]='_';
  }
  dci.protocol=xstripwhitespace(dci.protocol,"_");
  transfer_dicom_header(dci,*cb);
  if ((!dci.mosaicflag) && filecount>1)
    cb->dimz=filecount;

  return(0);   // no error!
}

int
read_data_dcm3d_3D(Cube *cb)
{
  dicominfo dci;

  string fname=cb->GetFileName();
  string pat=patfromname(fname);
  if (pat != fname) {
    tokenlist filenames=vglob(pat);
    if (filenames.size()==0)
      return 100;
    // if we have multiple files, each file must be a single slice
    if (filenames.size()>1)
      return read_multiple_slices(cb,filenames);
    // otherwise, let's check to make sure we got a file, and use that
    // single file as our 3d volume
    else if (filenames.size()<1)
      return 151;
    fname=filenames[0];
  }

  // READ 3D FROM A SINGLE FILE

  if (read_dicom_header(fname,dci))
    return 150;

  if (dci.dimx != cb->dimx || dci.dimy != cb->dimy || dci.dimz != cb->dimz)
    return 105;
  
  cb->SetVolume(dci.dimx,dci.dimy,dci.dimz,vb_short);
  if (!cb->data_valid)
    return 120;
  int volumesize=dci.dimx*dci.dimy*dci.dimz*cb->datasize;
  
  // make sure we can get all the slices (there will be blanks)
  if (dci.datasize<volumesize)
    return 130;

  FILE *ifile = fopen(fname.c_str(),"r");
  if (!ifile)
    return 110;
  fseek(ifile,dci.offset,SEEK_SET);
  unsigned char *newdata=new unsigned char[dci.datasize];
  if (!newdata)
    return 160;
  int cnt=fread(newdata,1,dci.datasize,ifile);
  fclose(ifile);
  mask_dicom(dci,newdata);
  if (cnt < volumesize) {
    delete [] newdata;
    return 150;
  }

  // de-mosaic if needed
  if (dci.mosaicflag) {
    int xoffset=0;
    int yoffset=0;
    int ind=0;
    for (int k=0; k<cb->dimz; k++) {
      if (xoffset>=dci.cols) {
        xoffset=0;
        yoffset+=dci.dimy;
      }
      // first row for this cube
      int rowstart=((yoffset*dci.cols)+(xoffset))*cb->datasize;
      rowstart+=(cb->dimy-1)*cb->datasize*dci.cols;
      for (int j=0; j<cb->dimy; j++) {
        // copy a whole row and position for the next
        memcpy(cb->data+ind,newdata+rowstart,dci.dimx*cb->datasize);
        rowstart-=dci.cols*cb->datasize;
        ind+=dci.dimx*cb->datasize;
      }
      xoffset+=dci.dimx;
    }
  }
  else {
    int rowsize=dci.dimx*cb->datasize;
    for (int j=0; j<dci.dimy; j++) {
      memcpy(cb->data+((cb->dimy-1-j)*rowsize),
             newdata+(j*rowsize),dci.dimx*cb->datasize);
    }
  }
  delete [] newdata;

  // FIXME valid if what?
  if (dci.byteorder!=my_endian())
    cb->byteswap();
  cb->data_valid=1;
  return(0);   // no error!
}

vf_status
test_dcm3d_3D(unsigned char *,int bufsize,string filename)
{
  // struct stat st;
  string pat=patfromname(filename);
  // if the file exists but it's too short, go home
  if (pat==filename && bufsize<200)
    return vf_no;
  // no match, go home
  tokenlist filenames=vglob(pat);
  if (filenames.size()==0)
    return vf_no;

  dicominfo dci,lastdci;
  if (read_dicom_header(filenames[0],dci))
    return vf_no;
  // the new heuristic is simple.  if we have more than one file and
  // they're from different acquisitions, we have a time dimension and
  // are therefore not 3D.
  if (filenames.size()==1)
    return vf_yes;
  if (read_dicom_header(filenames[filenames.size()-1],lastdci))
    return vf_no;
  if (dci.acquisition != lastdci.acquisition)
    return vf_no;
  return vf_yes;

  // THE FOLLOWING OBVIATED FOR A BETTER HEURISTIC
  // we're mosaiced, but the file isn't just a straight filename
  // if (dci.mosaicflag && filename!=pat)
  //   return vf_no;
  // if (dci.mosaicflag)
  //   return vf_yes;
  // // there are supposed to be n slices per volume and we have >n files
  // if (dci.slices>1 && dci.slices<filenames.size())
  //   return vf_no;
  // check to see if the first and last files match acquisition number, if not it's 4D
}

vf_status
test_dcm4d_4D(unsigned char *,int bufsize,string filename)
{
  // struct stat st;
  string pat=patfromname(filename);
  // if the file exists but it's too short, go home
  if (pat==filename && bufsize<200)
    return vf_no;
  // no match, go home
  tokenlist filenames=vglob(pat);

  // we must have at least 2 files, and they must have different
  // acquisitions
  if (filenames.size()<2)
    return vf_no;
  dicominfo dci,lastdci;
  if (read_dicom_header(filenames[0],dci))
    return vf_no;
  if (read_dicom_header(filenames[filenames.size()-1],lastdci))
    return vf_no;
  if (dci.acquisition != lastdci.acquisition)
    return vf_yes;
  return vf_no;


  // OLD HEURISTIC OBVIATED
  // if (dci.slices>1)
  //   dci.dimz=dci.slices;

  // // if we don't have enough for a whole volume, no
  // if (cnt<=dci.dimz)
  //   return vf_no;
  // // if we don't have an integer number of volumes
  // if (cnt % dci.dimz)
  //   return vf_no;
  // // if we have exactly one volume, it's 3D, not 4D
  // if (cnt==dci.dimz)
  //   return vf_no;
  // // if the first and last files match acquisition number, it's 3D
  // if (read_dicom_header(filenames[filenames.size()-1],lastdci))
  //   return vf_no;
  // if (dci.acquisition == lastdci.acquisition)
  //   return vf_no;

  // // okay, either we're mosaiced or we have more than dimz files
  // return vf_yes;
}

int
read_multiple_slices_from_files(Cube *cb,vector<string>filenames)
{
  dicominfo dci;

  if (read_dicom_header(filenames[0],dci))
    return 120;
  if (dci.slices>1) dci.dimz=dci.slices;
  if (dci.dimx == 0 || dci.dimy == 0 || dci.dimz == 0)
    return 105;
  cb->SetVolume(dci.dimx,dci.dimy,dci.dimz,vb_short);
  if (!cb->data_valid)
    return 120;
  int slicesize=dci.dimx*dci.dimy*cb->datasize;

  unsigned char *newdata=new unsigned char[dci.datasize];
  if (!newdata)
    return 150;
  for (int i=0; i<dci.dimz; i++) {
    // prematurely out of slices, no complaint i guess
    if (i>(int)filenames.size()-1)
      break;
    dicominfo dci2;
    if (read_dicom_header(filenames[i],dci2))
      continue;
    //     if (dci2.datasize<slicesize)
    //       continue;
    FILE *ifile = fopen(filenames[i].c_str(),"r");
    if (!ifile)
      continue;
    fseek(ifile,dci2.offset,SEEK_SET);
    int cnt=fread(newdata,1,dci2.datasize,ifile);
    fclose(ifile);
    mask_dicom(dci2,newdata);
    if (cnt < dci2.datasize)
      continue;
    memcpy(cb->data+(slicesize*i),newdata,slicesize);
  }
  if (dci.byteorder!=my_endian())
    cb->byteswap();
  return 0;
}

int
read_data_dcm4d_4D(Tes *tes,int start,int count)
{
  dicominfo dci;
  int timepoints=0;

  string fname=tes->GetFileName();
  string pat=patfromname(fname);
  tokenlist filenames=vglob(pat);

  if (filenames.size()<1)
    return 110;
  
  if (read_dicom_header(filenames[0],dci))
    return 150;
  if (dci.mosaicflag) {
    timepoints=filenames.size();
  }
  else {
    if (dci.slices>1) dci.dimz=dci.slices;
    if (filenames.size()%dci.dimz)
      return 112;
    timepoints=filenames.size()/dci.dimz;
  }

  // honor volume range
  if (start==-1) {
    start=0;
    count=tes->dimt;
  }
  else if (start+count>tes->dimt)
    return 220;
  tes->dimt=count;

  // here is where we handle un-mosaiced 4D time series
  if (!dci.mosaicflag) {
    Cube cb;
    transfer_dicom_header(dci,*tes);
    tes->SetVolume(dci.dimx,dci.dimy,dci.dimz,timepoints,vb_short);
    // tes->print();
    if (!tes->data)
      return 121;
    for (int i=start; i<start+count; i++) {
      vector<string> cnames;
      for (int j=i*dci.dimz; j<(i+1)*dci.dimz; j++)
        cnames.push_back(filenames[j]);
      read_multiple_slices_from_files(&cb,cnames);
      tes->SetCube(i,cb);
    }
    // tes->print();
    tes->data_valid=1;
    return (0);
  }

  for (int i=start; i<start+count; i++) {
    Cube cb;
    cb.SetFileName(filenames[i]);
    if (read_head_dcm3d_3D(&cb))
      continue;
    if (i==0) {
      tes->SetVolume(cb.dimx,cb.dimy,cb.dimz,timepoints,cb.datatype);
      if (!tes->data)
        return 120;
      tes->voxsize[0]=cb.voxsize[0];
      tes->voxsize[1]=cb.voxsize[1];
      tes->voxsize[2]=cb.voxsize[2];
      tes->filebyteorder=cb.filebyteorder;
      tes->header=cb.header;
    }
    if (read_data_dcm3d_3D(&cb))
      continue;
    tes->SetCube(i,cb);
  }

  tes->data_valid=1;
  tes->Remask();
  return(0);   // no error!
}

int
read_head_dcm4d_4D(Tes *tes)
{
  dicominfo dci;
  stringstream tmps;
  int filecount=0;

  string fname=tes->GetFileName();
  string pat=patfromname(fname);
  if (pat != fname) {
    vglob vg(pat);
    if (vg.size()==0)
      return 120;
    fname=vg[0];
    filecount=vg.size();
  }

  if (read_dicom_header(fname,dci))
    return 150;

  for (int i=0; i<(int)dci.protocol.size(); i++) {
    if (dci.protocol[i]==' ')
      dci.protocol[i]='_';
  }
  dci.protocol=xstripwhitespace(dci.protocol,"_");

  uint32 timepoints;
  if (dci.mosaicflag) {
    timepoints=filecount;
  }
  else {
    if (dci.slices>1) dci.dimz=dci.slices;
    if (filecount%dci.dimz)
      return 112;
    timepoints=filecount/dci.dimz;
  }
  transfer_dicom_header(dci,*tes);
  tes->dimt=timepoints;
  return(0);   // no error!
}

// bool operator<(const dicomge &ge1, const dicomge &ge2)
// {
//   if (ge1.group<ge2.group)
//     return 1;
//   if (ge2.group>ge1.group)
//     return 0;
//   if (ge1.element<ge2.element)
//     return 1;
//   if (ge2.element>ge1.element)
//     return 0;
//   return 1;
// }

dicomnames::dicomnames()
{
  populate();
}

string
dicomnames::operator()(dicomge ge)
{
  return names[ge];
}

string
dicomnames::operator()(uint16 g,uint16 e)
{
  return names[dicomge(g,e)];
}

bool
dicomge::operator<(const dicomge &ge) const
{
  if (group<ge.group)
    return 1;
  if (group>ge.group)
    return 0;
  if (element<ge.element)
    return 1;
  if (element>ge.element)
    return 0;
  return 0;
}

void
dicomnames::populate()
{
  // group 0008
  names[dicomge(0x0008,0x0008)]="Image Type";
  names[dicomge(0x0008,0x0020)]="Study Date";
  names[dicomge(0x0008,0x0021)]="Series Date";
  names[dicomge(0x0008,0x0022)]="Acquisition Date";
  names[dicomge(0x0008,0x0023)]="Content Date";
  names[dicomge(0x0008,0x0030)]="Study Time";
  names[dicomge(0x0008,0x0031)]="Series Time";
  names[dicomge(0x0008,0x0032)]="Acquisition Time";
  names[dicomge(0x0008,0x0033)]="Content Time";
  names[dicomge(0x0008,0x0050)]="Accession Number";
  names[dicomge(0x0008,0x0060)]="Modality";
  names[dicomge(0x0008,0x0070)]="Manufacturer";
  names[dicomge(0x0008,0x0080)]="Institution Name";
  names[dicomge(0x0008,0x0081)]="Institution Address";
  names[dicomge(0x0008,0x0090)]="Referring Physician's Name";
  names[dicomge(0x0008,0x1010)]="Station Name";
  names[dicomge(0x0008,0x1030)]="Study Description";
  names[dicomge(0x0008,0x103e)]="Series Description";
  names[dicomge(0x0008,0x1048)]="Physician(s) of Record";
  names[dicomge(0x0008,0x1070)]="Operator's Name";
  names[dicomge(0x0008,0x1090)]="Manufacturer's Model Name";
  // group 0010
  names[dicomge(0x0010,0x0010)]="Patient's Name";
  names[dicomge(0x0010,0x0020)]="Patient ID";
  names[dicomge(0x0010,0x0030)]="Patient's Birthdate";
  names[dicomge(0x0010,0x0040)]="Patient's Sex";
  names[dicomge(0x0010,0x1010)]="Patient's Age";
  names[dicomge(0x0010,0x1030)]="Patient's Weight";
  // group 0018
  names[dicomge(0x0018,0x0020)]="Scanning Sequence";
  names[dicomge(0x0018,0x0021)]="Sequence Variant";
  names[dicomge(0x0018,0x0022)]="Scan Options";
  names[dicomge(0x0018,0x0023)]="MR Acquisition Type";
  names[dicomge(0x0018,0x0024)]="Sequence Name";
  names[dicomge(0x0018,0x0050)]="Slice Thickness";
  names[dicomge(0x0018,0x0080)]="Repetition Time";
  names[dicomge(0x0018,0x0081)]="Echo Time";
  names[dicomge(0x0018,0x0082)]="Inversion Time";
  names[dicomge(0x0018,0x0083)]="Number of Averages";
  names[dicomge(0x0018,0x0084)]="Imaging Frequency";
  names[dicomge(0x0018,0x0085)]="Imaged Nucleus";
  names[dicomge(0x0018,0x0086)]="Echo Number(s)";
  names[dicomge(0x0018,0x0087)]="Magnetic Field Strength";
  names[dicomge(0x0018,0x0088)]="Spacing Between Slices";
  names[dicomge(0x0018,0x0089)]="Number of Phase Encoding Steps";

  names[dicomge(0x0018,0x0091)]="Echo Train Length";
  names[dicomge(0x0018,0x0093)]="Percent Sampling";
  names[dicomge(0x0018,0x0094)]="Percent Phase Field of View";
  names[dicomge(0x0018,0x0095)]="Pixel Bandwidth";

  names[dicomge(0x0018,0x1020)]="Software Version(s)";
  names[dicomge(0x0018,0x1030)]="Procotol Name";
  names[dicomge(0x0018,0x1251)]="Transmit Coil Name";
  names[dicomge(0x0018,0x1310)]="Acquisition Matrix";
  names[dicomge(0x0018,0x1312)]="In-plane Phase Encoding Direction";
  names[dicomge(0x0018,0x1314)]="Flip Angle";
  names[dicomge(0x0018,0x1315)]="Variable Flip Angle Flag";
  names[dicomge(0x0018,0x1316)]="SAR";
  names[dicomge(0x0018,0x1318)]="dB/dt";
  names[dicomge(0x0018,0x5100)]="Patient Position";
  // group 0020
  names[dicomge(0x0020,0x0010)]="Study ID";
  names[dicomge(0x0020,0x0011)]="Series Number";
  names[dicomge(0x0020,0x0012)]="Acquisition Number";
  names[dicomge(0x0020,0x0013)]="Instance Number";
  names[dicomge(0x0020,0x0032)]="Image Position (Patient)";
  names[dicomge(0x0020,0x0037)]="Image Orientation (Patient)";
  names[dicomge(0x0020,0x1041)]="Slice Location";
  // group 0028
  names[dicomge(0x0028,0x0002)]="Samples per Pixel";
  names[dicomge(0x0028,0x0010)]="Rows";
  names[dicomge(0x0028,0x0011)]="Columns";
  names[dicomge(0x0028,0x0030)]="Pixel Spacing";
  names[dicomge(0x0028,0x0100)]="Bits Allocated";
  names[dicomge(0x0028,0x0101)]="Bits Stored";
  names[dicomge(0x0028,0x0102)]="High Bit";
  names[dicomge(0x0028,0x0103)]="Pixel Representation";
  names[dicomge(0x0028,0x0106)]="Smallest Image Pixel Value";
  names[dicomge(0x0028,0x0107)]="Largest Image Pixel Value";
  names[dicomge(0x0028,0x1050)]="Window Center";
  names[dicomge(0x0028,0x1051)]="Window Width";
}


} // extern "C"
