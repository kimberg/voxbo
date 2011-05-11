
// dbdialogs.h
// simple dialogs for database
// Copyright (c) 2009 by The VoxBo Development Team

// This file is part of VoxBo
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 3, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file named COPYING.  If not, write
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA
//
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QTreeWidget>

#include "mydefs.h"
#include "dbclient.h"

using namespace std;

class DBnewsession : public QDialog {
  Q_OBJECT
public:
  DBnewsession(QWidget *parent);
  DBdate date() {return DBdate(w_date->date().month(),w_date->date().day(),
                               w_date->date().year(),w_date->time().hour(),
                               w_date->time().minute(),w_date->time().second());}
  string location() {return w_location->text().toStdString();}
  string examiner() {return w_examiner->text().toStdString();}
  string notes() {return w_notes->text().toStdString();}
public slots:
private:
  QLineEdit *w_name;
  QDateTimeEdit *w_date;
  QLineEdit *w_location;
  QLineEdit *w_examiner;
  QLineEdit *w_notes;
signals:
};

class DBpicksession : public QDialog {
  Q_OBJECT
public:
  DBpicksession(QWidget *parent,DBpatient *p);
  int32 selectedsession();
public slots:
private:
  QTreeWidget *w_tree;
signals:
};

class DBpicktest : public QDialog {
  Q_OBJECT
public:
  DBpicktest(QWidget *parent,DBclient *c);
  string selectedtest();
public slots:
private:
  QTreeWidget *w_tree;
signals:
};

class DBplist : public QDialog {
  Q_OBJECT
public:
  DBplist(QWidget *parent,DBpatientlist &plist);
public slots:
private:
  QTreeWidget *w_tree;
signals:
};

class DBuserinfo : public QDialog {
  Q_OBJECT
public:
  DBuserinfo(QWidget *parent);
  // username, password, contact info, group memberships
  string username() {return w_username->text().toStdString();}
  string password() {return w_password->text().toStdString();}
  string name() {return w_name->text().toStdString();}
  string phone() {return w_phone->text().toStdString();}
  string email() {return w_email->text().toStdString();}
  string address() {return w_address->text().toStdString();}
  void setusername(string s) {w_username->setText(s.c_str());}
  void setpassword(string s) {w_password->setText(s.c_str());}
  void setname(string s) {w_name->setText(s.c_str());}
  void setphone(string s) {w_phone->setText(s.c_str());}
  void setemail(string s) {w_email->setText(s.c_str());}
  void setaddress(string s) {w_address->setText(s.c_str());}
public slots:
private:
  QLineEdit *w_username;
  QLineEdit *w_password;
  QLineEdit *w_name;
  QLineEdit *w_phone;
  QLineEdit *w_email;
  QLineEdit *w_address;
signals:
};

class DBlocallogin : public QDialog {
  Q_OBJECT
public:
  DBlocallogin(QWidget *parent);
  string username() {return w_username->text().toStdString();}
  string password() {return w_password->text().toStdString();}
  string dirname() {return w_dirname->text().toStdString();}
  string message() {return w_message->text().toStdString();}
  void setusername(string s) {w_username->setText(s.c_str());}
  void setpassword(string s) {w_password->setText(s.c_str());}
  void setdirname(string s) {w_dirname->setText(s.c_str());}
  void setmessage(string s) {w_message->setText(s.c_str());}
public slots:
private:
  QLineEdit *w_username;
  QLineEdit *w_password;
  QLineEdit *w_dirname;
  QLabel *w_message;
signals:
};

class DBremotelogin : public QDialog {
  Q_OBJECT
public:
  DBremotelogin(QWidget *parent);
  string username() {return w_username->text().toStdString();}
  string password() {return w_password->text().toStdString();}
  string servername() {return w_servername->text().toStdString();}
  uint16 serverport() {return strtol(w_serverport->text().toStdString());}
  string message() {return w_message->text().toStdString();}
  void setusername(string s) {w_username->setText(s.c_str());}
  void setpassword(string s) {w_password->setText(s.c_str());}
  void setservername(string s) {w_servername->setText(s.c_str());}
  void setserverport(uint16 p) {w_serverport->setText(strnum(p).c_str());}
  void setmessage(string s) {w_message->setText(s.c_str());}
public slots:
private:
  QLineEdit *w_username;
  QLineEdit *w_password;
  QLineEdit *w_servername;
  QLineEdit *w_serverport;
  QLabel *w_message;
signals:
};
