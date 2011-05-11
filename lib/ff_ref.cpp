
// ff_ref.cpp
// VoxBo file I/O code for REF1 format
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
// original version written by Dan Kimberg

using namespace std;

#include "vbutil.h"
#include "vbio.h"

//////////////////////////////////////////////////////////
// vectors are always doubles
// when loading a REF1, space is allocated dynamically in
//   increments of 100 doubles
// when writing a REF1, all comments are discarded
//////////////////////////////////////////////////////////

// vectors are allocated in lots of 100, and resized as needed unless
// there's a length header

extern "C" {

vf_status ref1_test(unsigned char *buf,int bufsize,string filename);
int ref1_write(VB_Vector *vec);
int ref1_read(VB_Vector *vec);

#ifdef VBFF_PLUGIN
VBFF vbff()
#else
VBFF ref1_vbff()
#endif
{
  VBFF tmp;
  tmp.name="VoxBo REF1";
  tmp.extension="ref";
  tmp.signature="ref1";
  tmp.dimensions=1;
  tmp.version_major=vbversion_major;
  tmp.version_minor=vbversion_minor;
  tmp.test_1D=ref1_test;
  tmp.read_1D=ref1_read;
  tmp.write_1D=ref1_write;
  return tmp;
}
  
vf_status
ref1_test(unsigned char *buf,int bufsize,string fname)
{
  tokenlist args;
  tokenlist line;
  args.SetSeparator("\n");
  args.SetQuoteChars("");
  if (bufsize<2)
    return vf_no;
  args.ParseLine((char *)buf);
  args.DeleteLast();  // don't try to parse partial lines
  int goodlines=0;
  for (size_t i=0; i<args.size(); i++) {
    // skip comments
    if (args[i][0]==';' || args[i][0]=='#')
      continue;
    // skip VB98/REF1 header
    if (i==0 && args[0]=="VB98") {
      if (args.size()<2)
        return vf_no;
      if (args[1]!="REF1")
        return vf_no;
      i++;
      continue;
    }
    // anything else, make sure it's either blank or parseable as a double
    line.ParseLine(args[i]);
    if (line.size()==0) continue;
    if (line.size()!=1)
      return vf_no;
    if (strtodx(line[0]).first)
      return vf_no;
    goodlines++;
  }
  if (goodlines==0)
    return vf_no;
  // go ahead and read the whole file, just to be sure
  VB_Vector vv;
  vv.setFileName(fname);
  if (ref1_read(&vv))
    return vf_no;
  return vf_yes;
}

int
ref1_write(VB_Vector *vec)
{
  size_t i;
  FILE *fp = fopen(vec->getFileName().c_str(),"w");
  if (!fp) {
    return (100);
  }
  fprintf(fp,";VB98\n;REF1\n");
  for (i=0; i<vec->header.size(); i++)
    fprintf(fp,"; %s\n",vec->header[i].c_str());
  for (i=0; i<vec->size(); i++)
    fprintf(fp,"%.20g\n",(*vec)[i]);
  fclose(fp);
  return(0);    // no error!
}

int
ref1_read(VB_Vector *vec)
{
  FILE *fp;
  char linex[STRINGLEN];
  size_t allocation,len;
  double *dd=NULL,*olddata=NULL;

  vec->clear();
  len=allocation=0;

  if ((fp=fopen(vec->getFileName().c_str(),"r"))==0)
    return (105);
  while (fgets(linex,STRINGLEN,fp)) {
    string line=xstripwhitespace(linex);
    if (line.empty()) continue;
    if (strchr(";#%",line[0])) {
      vec->header.push_back(xstripwhitespace(line.substr(1)));
      continue;
    }
    pair<bool,double> res=strtodx(line);
    if (res.first) {
      fclose(fp);
      return 112;
    }
    if (len+1 > allocation) {
      allocation += 25000;
      olddata=dd;
      dd=new double[allocation];
      assert(dd);
      if (olddata) {
        memcpy(dd,olddata,sizeof(double)*len);
        delete [] olddata;
        olddata=NULL;
      }
    }
    dd[len++]=res.second;
  }
  fclose(fp);
  vec->resize(len);
  for (size_t i=0; i<len; i++)
    vec->setElement(i,dd[i]);
  if (dd) delete [] dd;
  if (olddata) delete [] olddata;
  return(0);    // no error!
}

} // extern "C"
