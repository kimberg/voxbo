
// connect.cpp
// safe, easy connection-related functions for VoxBo and beyond!
// Copyright (c) 1998-2008 by The VoxBo Development Team

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

#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <errno.h>
#include <sys/un.h>
#include "vbutil.h"

// buffer size for send/receive file
#define F_BUFSIZE 65536

int
safe_recv(int sock,char *buf,int len,float secs)
{
  fd_set ff;
  struct timeval t_start,t_deadline,t_current,t_timeout;
  int pos=0,err;

  // buf is valid even if we fail utterly
  buf[0]='\0';
  // before we do anything time-consuming, set a deadline
  gettimeofday(&t_start,NULL);
  t_deadline.tv_sec = (int)secs;
  t_deadline.tv_usec = lround(((secs - floor(secs))*1000000.0));
  t_deadline=t_start+t_deadline;

  while(1) {
    // create socket mask for just the one socket
    FD_ZERO(&ff);
    FD_SET(sock,&ff);
    // timeout as specified
    gettimeofday(&t_current,NULL);
    t_timeout=t_deadline-t_current;
    // make sure there's data available
    err = select(sock+1,&ff,NULL,NULL,&t_timeout);
    if (err < 1)
      return err;
    err=recv(sock,buf+pos,len-pos,0);
    // break if we didn't get anything before the deadline
    // buf[pos+err]='\0';  // FIXME
    if (err<=0) {
      break;
      if (errno==EAGAIN)  // FIXME not needed now that we're nonblocking
        continue;
      else
        break;
    }
    pos+=err;
    // break if we got something EOT-terminated
    if (buf[pos-1]=='\0')
      break;
    // break if we filled the buffer
    if (pos>=len)
      break;
  }
  // null-terminate if there's room, in case someone lazy uses it as a string
  if (pos > 0 && pos<len)
    buf[pos]='\0';
  return pos;
}

int
safe_send(int sock,char *buf,int len,float secs)
{
// safe_send() just makes sure the socket is writeable before we send.
// at present it does not try to break the message up into pieces if
// it's too big.  so whoever calls safe_send should be careful of
// that.

  fd_set ff;
  struct timeval t_start,t_deadline,t_current,t_timeout;
  int err;

  // before we do anything time-consuming, set a deadline
  gettimeofday(&t_start,NULL);
  t_deadline.tv_sec = (int)secs;
  t_deadline.tv_usec = lround(((secs - floor(secs))*1000000.0));
  t_deadline=t_start+t_deadline;

  // create socket mask for just the one socket
  FD_ZERO(&ff);
  FD_SET(sock,&ff);
  // timeout as specified
  gettimeofday(&t_current,NULL);
  t_timeout=t_deadline-t_current;
  // make sure we can send
  err = select(sock+1,NULL,&ff,NULL,&t_timeout);
  if (err < 1)
    return err;
  err=send(sock,buf,len,0);
  if (err!=len)
    return 101;
  else
    return 0;
}

int
safe_connect(struct sockaddr_in *addr,float secs)
{
  return safe_connect((struct sockaddr *)addr,secs);
}

int
safe_connect(struct sockaddr_un *addr,float secs)
{
  return safe_connect((struct sockaddr *)addr,secs);
}

int
safe_connect(struct sockaddr *addr,float secs)
{
// safe_connect() is a version of connect that will timeout after a
// given deadline.  to do this correctly, we put the socket in
// nonblocking mode first, call connect, and if connect doesn't fail
// immediately, we're either connected or in EINPROGRESS.  if the
// latter, we can use getsockopt to see if there's an error.  see
// http://www.lcg.org/sock-faq/connect.c for some details.
// (non-blocking send/recv is a lot easier.  just use select() with a
// timeout to make sure the socket is ready to read/write.)

  int err,s,sz;
  fd_set ff;
  struct timeval tv;

  s = socket(addr->sa_family,SOCK_STREAM,0);
  if (s == -1)
    return (-1);

  fcntl(s,F_SETFL,O_NONBLOCK);

  if (addr->sa_family==AF_INET)
    sz=sizeof(struct sockaddr_in);
  else
    sz=sizeof(struct sockaddr_un);
  err = connect(s,addr,sz);
  if (err && (errno != EINPROGRESS)) {
    close(s);
    return(-2);
  }
  // set up for select()
  FD_ZERO(&ff);
  FD_SET(s,&ff);
  tv.tv_sec = (int)secs;
  tv.tv_usec = lround(((secs - floor(secs))*1000000.0));

  // wait until socket is writeable
  err = select(s+1,NULL,&ff,NULL,&tv);
  if (err < 1) {
    close(s);
    return (-3);
  }
  socklen_t len=sizeof(int);
  if (getsockopt(s,SOL_SOCKET,SO_ERROR,&err,&len) == -1) {
    close(s);
    return (-4);
  }
  return s;
}

int
send_file(int s,string fname)
{
// send_file() tries to send a file through a socket.  it's presumed
// that the process on the other end of the socket is already
// expecting a file to be sent (i.e., is calling receive_file).

  struct stat st;
  FILE *fp=fopen(fname.c_str(),"r");
  if (!fp)
    return 101;
  if (fstat(fileno(fp),&st)) {
    fclose(fp);
    return 111;
  }

  int filesize=st.st_size;
  char buf[F_BUFSIZE];
  // send filename
  sprintf(buf,"send %s %d",fname.c_str(),filesize);
  if (safe_send(s,buf,strlen(buf)+1,10.0)) {
    fclose(fp);
    return 102;
  }

  int bytestosend=filesize;
  int packetsize;
  while(bytestosend>0) {
    packetsize=(F_BUFSIZE>bytestosend?bytestosend:F_BUFSIZE);
    // read a chunk
    fread(buf,1,packetsize,fp);
    // send it
    if (safe_send(s,buf,packetsize,10.0)) {
      fclose(fp);
      return 103;
    }
    bytestosend-=packetsize;
  }
  if (safe_recv(s,buf,F_BUFSIZE,10.0)<0)
    return 55;
  buf[4]='\0';
  if ((string)buf=="ACK")
    return 0;
  else return 66;
}

int
receive_file(int s,string fname,int filesize)
{
  FILE *fp=fopen(fname.c_str(),"w");
  if (!fp)
    return 101;
  char buf[F_BUFSIZE];

  int packetsize,cnt=0,total=0;
  while(1) {
    // read a chunk
    if ((cnt=safe_recv(s,buf,F_BUFSIZE,10.0))<0) {
      fclose(fp);
      return 103;
    }
    // write it
    cnt=fwrite(buf,1,cnt,fp);
    total+=cnt;
  }
  fclose(fp);
  if (total!=filesize)
    return 103;
  if (safe_send(s,buf,packetsize,10.0)) {
    fclose(fp);
    return 105;
  }
  return 0;
}

