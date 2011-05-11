
//  o  voxbo should fork off a process that will act as a lock server.
//     processes requiring any kind of lock, file or system, should just
//     send a request and wait for an okay.  (this adds quite a bit of
//     overhead to locking, so we should be careful about when we do it.)
//     when a lock request comes in, check for conflicting locks.
//     the function to request a lock should connect, requesting the lock,
//     and on failure usleep for useconds, up to a maximum number of retries.
//     if the lock server isn't found, fall back on fcntls of some lockfile
//     or something.  the lock server should listen on port 6006, with
//     a configurably sized queue.  one version of the (un)lock function should
//     accept an address, but another should work with the resolved host, to
//     speed (un)locking.  the function should connect, send the pathname,
//     lock type, lowrange, and highrange, and receive back a yes or no.

// voxbo needs to periodically check to make sure the lock server is still
// running, and if not, restart it and send email

// the locking function should have multiple methods available, with
// the understanding that they are mutually incompatible

using namespace std;

class VBLock {
private:
  enum {readlock,writelock,mutex} type;
public:
  char id[128];
  int low,high;     // for range locks
};

void
wait_lock_request()
{
  tokenlist args;

  err = bind();
  while(TRUE) {
    err = listen();
    err = accept();
    err = recv();
    args.ParseLine(recv);
    if (args.size() < 2)
      continue;
    // figure out the lock type
    if (dancmp(args[0],"unlock")) {
    }
    else if (dancmp(args[0],"readlock")) {
    }
    else if (dancmp(args[0],"writelock")) {
    }
  }
}

void
handle_voxbo_lock(char *id,char *type,int low,int high)
{
  int conflicts = 0;
  for (LI thislock=locklist.begin(); LI != locklist.end(); LI++) {
    if (!dancmp(name,thislock->name))
      continue;
    if (thislock->type == readlock && type == readlock)
      continue;
    if (thislock.type == writelock && thislock.end == 0)
      conflicts++;
    if (type == writelock && high == 0)
      conflicts++;
    if (vbp.simulwrites)
      ;
  }
}

void
handle_voxbo_unlock()
{
}



int
get_voxbo_lock(locktype)
{
  while (TRUE) {
    // try to get the lock
    // if success, return TRUE
    // if nth failure, return FALSE
    // if no connection, return FALSE
  }
}
