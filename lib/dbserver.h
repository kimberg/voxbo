
// dbserver.h
//
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
// by Dongbo Hu

#ifndef DBSERVER_H
#define DBSERVER_H

#include <set>
#include "bdb_tab.h"
#include "db_util.h"
#include "mydb.h"
#include "mydefs.h"

class srvSession;

class DBserver {
  friend class srvSession;
  friend int startServer(const string&, int);
  friend int srp_credfunction(gnutls_session, const char*, gnutls_datum*,
                              gnutls_datum*, gnutls_datum*, gnutls_datum*);

 public:
  void setEnvHome(const string&);
  string getEnvHome();
  DbEnv& getEnv();
  string getErrMsg();

 private:
  DBdata dbs;
  string errMsg;
};

#endif
