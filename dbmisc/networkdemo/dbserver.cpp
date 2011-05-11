
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <fcntl.h>
#include "vbutil.h"
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include <list>

using namespace std;

int main_server();
int main_client();

const int SERVERPORT=5556;
const int BUFSIZE=1024;
const int DH_BITS=1024;

class dbpermissions {
public:
  long data_id;          // id of the thing to which permission is granted
  string permission;     // r, rw, or b
};

// temp class for server to keep track of multiple connections

class DBSession {
public:
  gnutls_session_t session;
  int sd;
};

// db_sessions, includes all the major bits of state information you'd
// want about a session.

class dbsession {
public:
  long userid;
  list<long> groupids;
  long authtoken[4];
  time_t logtime;
  time_t lastaction;
  string msg;
};

list<dbsession> sessionlist;
// static gnutls_dh_params_t dh_params;

int
main(int argc,char **argv)
{
  if (argc!=2)
    exit(0);
  if ((string)argv[1]=="server")
    main_server();
  else
    main_client();
  exit(0);
}

// make_gnutls_verifier() -- this function generates a random salt
// value and a password, and generates a verifier.  the size of the
// verifier will be in verifier->size, the data in verifier->data.
// the calling function should declare the following:
//   char salt[4];
//   gnutls_datum verifier;
// the salt and verifier will be deposited appropriately.  this function
// allocates storage for the verifier using gnutls_malloc.  when done with
// the verifier, the datum should be de-allocated like this:
//   gnutls_free(verifier.data);

int
make_gnutls_verifier(string username,string password,char *salt,gnutls_datum *verifier)
{
  // init salt structure for gnutls
  gnutls_datum mysalt;
  mysalt.size=4;
  mysalt.data=(unsigned char *)gnutls_malloc(4);
  if (!mysalt.data)
    return -1;
  // generate random salt, copy for calling function and to salt struct
  unsigned long st=VBRandom();
  memcpy(mysalt.data,&st,4);
  memcpy(salt,&st,4);

  // FIXME here we generate a verifier using the password foo.  what
  // we really need to do is copy the verifier from the database, and
  // init the verifier structure using the same method as above
  return gnutls_srp_verifier(username.c_str(),password.c_str(),&mysalt,
			     &(gnutls_srp_1024_group_generator),
			     &(gnutls_srp_1024_group_prime),
			     verifier);
}

// srp_credfunction() -- this function takes a username argument and
// fills out the salt, generator, prime, and verifier.  the salt and
// verifier come from the user record, the generator and prime are
// hardcoded gnutls constants.  note that we have to make copies of
// the generator and prime because the calling function expects us to
// do so.

int
srp_credfunction(gnutls_session session,const char *username, gnutls_datum *salt,
		 gnutls_datum *verifier, gnutls_datum *generator, gnutls_datum *prime)
{
  salt->size=4;
  salt->data=(unsigned char *)gnutls_malloc(4);
  if (!salt->data)
    return -1;
  memset(salt->data,'a',4);  // FIXME get salt from database

  // copy stock generator
  generator->size=gnutls_srp_1024_group_generator.size;
  generator->data=(unsigned char *)gnutls_malloc(generator->size);
  if (!generator->data)
    return -1;
  memcpy(generator->data,gnutls_srp_1024_group_generator.data,generator->size);
  // copy stock prime
  prime->size=gnutls_srp_1024_group_prime.size;
  prime->data=(unsigned char *)gnutls_malloc(prime->size);
  if (!prime->data)
    return -1;
  memcpy(prime->data,gnutls_srp_1024_group_prime.data,prime->size);
  // FIXME here we generate a verifier using the password foo.  what
  // we really need to do is copy the verifier from the database, and
  // init the verifier structure using the same method as above
  return gnutls_srp_verifier(username,"foo",salt,generator,prime,verifier);
}

int
main_server()
{
  int err,i;
  struct sockaddr_in sa_serv;
  struct sockaddr_in sa_cli;
  socklen_t client_len;
  char topbuf[512];
  gnutls_session_t session;
  char buffer[BUFSIZE + 1];
  int yes=1;
  gnutls_srp_server_credentials_t srpcred;

  printf("SERVER MODE\n");

  gnutls_global_init();
  gnutls_global_init_extra();
  gnutls_srp_allocate_server_credentials(&srpcred);
  gnutls_srp_set_server_credentials_function(srpcred,srp_credfunction);

  // note that the reason we use the _function version above is that
  // the alternative is a _file version, which means we have to use a
  // special srp file.  we want to keep the info in our database



  // generate and set diffie-hellman params
  //   gnutls_dh_params_init(&dh_params);
  //   gnutls_dh_params_generate2 (dh_params, DH_BITS);
  //   gnutls_anon_set_server_dh_params (anoncred, dh_params);
 
  int s;
  // create the socket, make it reusable after exit
  s=socket(PF_INET,SOCK_STREAM,0);
  if (s < 0)
    return s;
  setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
  // create socket address, bind to our socket
  memset(&sa_serv,0,sizeof(sa_serv));
  sa_serv.sin_family=AF_INET;
  sa_serv.sin_port=htons(SERVERPORT);
  sa_serv.sin_addr.s_addr=htonl(INADDR_ANY);
  err=bind(s,(struct sockaddr *)&sa_serv,sizeof(sa_serv));
  if (err == -1) {
    close(s);
    return -2;
  }
  // express willingness to listen for up to 16 queued connections
  err = listen(s,16);   // how much of a backlog should we allow?
  if (err == -1) {
    close(s);
    return -3;
  }
  printf("Now listening to port %d\n",SERVERPORT);
  client_len=sizeof(sa_cli);
  printf("server waiting for connection\n");

  list<DBSession> sessions;

  while(1) {
    cout << 111 << endl;
    fd_set myset;
    FD_ZERO(&myset);
    FD_SET(s,&myset);
    struct timeval mytv;
    mytv.tv_sec=mytv.tv_usec=5;
    int ret=select(s+1,&myset,NULL,NULL,&mytv);
    // use select() to see if s is readable (we have a waiting
    // connection).  if so, accept connection, do handshaking, and
    // create a mysession
    cout << 111 << endl;
    
    if (ret>0) {
      cout << "new connection " << sessions.size()+1 << endl;
      // create new session, push on back of list, get ptr to gnutls session struct
      sessions.push_back(DBSession());
      gnutls_session_t *sptr=&(sessions.back().session);
      gnutls_init(sptr,GNUTLS_SERVER);
      gnutls_priority_set_direct(*sptr,"NORMAL:+SRP",NULL);
      gnutls_credentials_set(*sptr,GNUTLS_CRD_SRP,srpcred);
      int sd=accept(s,(struct sockaddr *)&sa_cli,&client_len);
      //printf ("- connection from %s, port %d\n",
      //      inet_ntop (AF_INET, &sa_cli.sin_addr, topbuf,
      //                 sizeof (topbuf)), ntohs (sa_cli.sin_port));
      gnutls_transport_set_ptr(*sptr,(gnutls_transport_ptr_t) sd);
      //printf("transport ptr set, about to handshake\n");
      ret = gnutls_handshake (*sptr);
      //printf("handshook, or not\n");
      if (ret < 0) {
        close (sd);
        gnutls_deinit (*sptr);
        fprintf (stderr, "*** Handshake has failed (%s)\n\n",
                 gnutls_strerror (ret));
        sessions.pop_back();
        continue;
      }
      sessions.back().sd=sd;
      printf ("- Handshake was completed\n");
    }
    for (list<DBSession>::iterator si=sessions.begin(); si!=sessions.end(); si++) {
      fd_set myset;
      FD_ZERO(&myset);
      FD_SET(si->sd,&myset);
      struct timeval mytv;
      mytv.tv_sec=mytv.tv_usec=0;
      int ret=select(si->sd+1,&myset,NULL,NULL,&mytv);
      if (ret>0) {
        cout << "found data" << endl;

        memset (buffer, 0, BUFSIZE + 1);
        ret = gnutls_record_recv (si->session, buffer, BUFSIZE);
        if (ret == 0) {
          printf ("\n- Peer has closed the GNUTLS connection\n");
          break;
        }
        else if (ret < 0) {
          fprintf (stderr, "*** corrupted data(%d), closing connection\n", ret);
          break;
        }
        else if (ret > 0) {
          /* echo data back to the client
           */
          string foo=(string)"got "+buffer;
          cout << foo << endl;
          gnutls_record_send(si->session,foo.c_str(),foo.size());
        }
        
        
      }
    }
  }

  cout << "shouldn't reach here" << endl;


  while(1) {
    int sd,ret;
    gnutls_init(&session,GNUTLS_SERVER);
    gnutls_priority_set_direct(session,"NORMAL:+SRP",NULL);
    gnutls_credentials_set(session,GNUTLS_CRD_SRP,srpcred);

    sd = accept(s,(struct sockaddr *)&sa_cli,&client_len);
    printf ("- connection from %s, port %d\n",
            inet_ntop (AF_INET, &sa_cli.sin_addr, topbuf,
                       sizeof (topbuf)), ntohs (sa_cli.sin_port));
    gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) sd);
    printf("transport ptr set, about to handshake\n");
    ret = gnutls_handshake (session);
    printf("handshook, or not\n");
    if (ret < 0) {
      close (sd);
      gnutls_deinit (session);
      fprintf (stderr, "*** Handshake has failed (%s)\n\n",
               gnutls_strerror (ret));
      continue;
    }
    printf ("- Handshake was completed\n");

    i = 0;
    while (1) {
      memset (buffer, 0, BUFSIZE + 1);
      ret = gnutls_record_recv (session, buffer, BUFSIZE);
      if (ret == 0) {
        printf ("\n- Peer has closed the GNUTLS connection\n");
        break;
      }
      else if (ret < 0) {
        fprintf (stderr, "*** corrupted data(%d), closing connection\n", ret);
        break;
      }
      else if (ret > 0) {
        /* echo data back to the client
         */
	string foo=(string)"got "+buffer;
	cout << foo << endl;
        gnutls_record_send(session,foo.c_str(),foo.size());
      }
    }
    gnutls_bye (session, GNUTLS_SHUT_WR);

    close (sd);
    gnutls_deinit (session);

  }

  close (s);
  gnutls_srp_free_server_credentials(srpcred);
  gnutls_global_deinit ();
}

int
main_client()
{
  // GET LOGON INFORMATION LOCALLY
  printf("CLIENT MODE\n");
  char buf[BUFSIZE+1];

//   printf("server: ");
//   if (!fgets(buf,500,stdin))
//     return 0;
//   else
//     buf[strlen(buf)-1]='\0';
//   string server=buf;

//   printf("username: ");
//   if (!fgets(buf,500,stdin))
//     return 0;
//   else
//     buf[strlen(buf)-1]='\0';
//   string username=buf;

//   printf("password: ");
//   if (!fgets(buf,500,stdin))
//     return 0;
//   else
//     buf[strlen(buf)-1]='\0';
//   string password=buf;

  string server="rage";
  string username="dan";
  string password="foo";

  printf("%s %s %s\n",server.c_str(),username.c_str(),password.c_str());

  int ret;
  gnutls_session_t session;
  char buffer[BUFSIZE + 1];
  gnutls_srp_client_credentials_t srpcred;

  // INIT GNUTLS
  gnutls_global_init();
  gnutls_global_init_extra();
  gnutls_srp_allocate_client_credentials(&srpcred);
  gnutls_srp_set_client_credentials(srpcred,username.c_str(),password.c_str());


  // SET UP SERVER ADDRESS TO CONNECT TO
  struct hostent *hp;
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(struct sockaddr_in));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(SERVERPORT);
  hp=gethostbyname(server.c_str());
  if (!hp) {
    printf("couldn't find host\n");
    exit(102);
  }
  memcpy(&addr.sin_addr,hp->h_addr_list[0],hp->h_length);
  int s=safe_connect(&addr,10);
  if (s<0) {
    printf("couldn't connect!\n");
    exit(100);
  }
  fcntl(s,F_SETFL,0);

  // MORE GNUTLS SETUP and handshake
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_priority_set_direct(session, "NORMAL:+SRP", NULL);
  gnutls_credentials_set(session,GNUTLS_CRD_SRP,srpcred);
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) s);
  ret = gnutls_handshake (session);
  if (ret < 0) {
    fprintf (stderr, "*** Handshake failed\n");
    fprintf (stderr,"fatal: %d\n",gnutls_error_is_fatal(ret));
    gnutls_perror (ret);
    exit(999);   // FIXME need to be more elegant about this
  }
  else {
    printf ("- Handshake was completed\n");
  }

  while (1) {
    printf("your message: ");
    if (!fgets(buf,500,stdin))
      return 0;
    else
      buf[strlen(buf)-1]='\0';
    gnutls_record_send(session,buf,strlen(buf));  // FIXME check success?
    ret = gnutls_record_recv (session, buffer, BUFSIZE);
    if (ret == 0) {
      printf ("- Peer has closed the TLS connection\n");
      exit(999);  // FIXME
    }
    else if (ret < 0) {
      fprintf (stderr, "*** Error: %s\n", gnutls_strerror (ret));
      exit(999);  // FIXME
    }
    // FIXME print answer
  }

  gnutls_bye (session, GNUTLS_SHUT_RDWR);

  close(s);
  gnutls_deinit(session);
  gnutls_srp_free_client_credentials(srpcred);
  gnutls_global_deinit();
  return 0;
}

void
serve()
{
  // get the request.  if we're running as an inetd process, this will
  // include the authentication token, and we'll have to find the
  // session in our sessionlist.
  // string str=get_msg();
  // find the relevant session
  // process the request.  
}


string
serve_logon(string msg)
{
  // process msg, which should include name/pw, create session, and
  // return result + authtoken
  return "";
}

void
serve_search(string msg)
{
}
