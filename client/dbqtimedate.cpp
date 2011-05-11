
#include "dbqtimedate.moc.h"

using namespace std;

DBQTimeDate::DBQTimeDate(QWidget *parent)
  : DBQScoreBox(parent)
{
  // DBdate original auto-sets to current time/date in its constructor
  layout=new QHBoxLayout();
  layout->setSpacing(4);
  layout->setMargin(1);
  layout->setAlignment(Qt::AlignLeft);
  this->setLayout(layout);

  f_date=1,f_time=0;

  dt=new QDateTimeEdit();
  dt->setCalendarPopup(1);
  dt->setMinimumDate(QDate(1900,1,1));
  dt->setMaximumDate(QDate(2100,1,1));

  dt->setDisplayFormat("d-MMM-yyyy h:mmap");
  dt->setTime(QTime(12,0,0));
  dt->setDate(QDate(1900,1,1));
  o_year=dt->date().year();
  o_month=dt->date().month();
  o_day=dt->date().day();
  o_hour=dt->time().hour();
  o_minute=dt->time().minute();
  o_second=dt->time().second();
  layout->addWidget(dt);
  
  valueline=new QLineEdit();
  valueline->setFrame(0);
  valueline->setReadOnly(1);
  valueline->setText("<nodata>");
  layout->addWidget(valueline);
  layout->setStretchFactor(valueline,2);
  valueline->hide();
 
  layout->insertStretch(-1,0);

  button_delete=new QPushButton("set",this);
  layout->addWidget(button_delete);
  button_revert=new QPushButton("revert",this);
  layout->addWidget(button_revert);

  QObject::connect(button_delete,SIGNAL(clicked()),this,SLOT(deleteclicked()));
  QObject::connect(button_revert,SIGNAL(clicked()),this,SLOT(revertclicked()));
  QObject::connect(dt,SIGNAL(dateTimeChanged(QDateTime)),this,SLOT(changed()));
}

void
DBQTimeDate::setValue(const DBscorevalue &val)
{
  DBdate date=val.v_date;
  f_originallyset=1;
  f_set=1;
  dt->setDate(QDate(date.getYear(),date.getMonth(),date.getDay()));
  dt->setTime(QTime(date.getHour(),date.getMinute(),date.getSecond()));
  o_year=date.getYear();
  o_month=date.getMonth();
  o_day=date.getDay();
  o_hour=date.getHour();
  o_minute=date.getMinute();
  o_second=date.getSecond();
  setvalueline();
  f_dirty=0;
  updateAppearance();
}

void
DBQTimeDate::setTime(uint16 hours,uint16 minutes,uint16 seconds)
{
  dt->setTime(QTime(hours,minutes,seconds));
  setvalueline();
  f_dirty=0;
  updateAppearance();
}

void
DBQTimeDate::setDate(uint16 year,uint16 month,uint16 day)
{
  dt->setDate(QDate(year,month,day));
  setvalueline();
  f_dirty=0;
  updateAppearance();
}

void
DBQTimeDate::setFormat(bool time,bool date)
{
  if (time) f_time=1; else f_time=0;
  if (date || !time) f_date=1; else f_date=0;
  if (time & date)
    dt->setDisplayFormat("d-MMM-yyyy h:mmap");
  else if (time & !date)
    dt->setDisplayFormat("h:mmap");
  else
    dt->setDisplayFormat("d-MMM-yyyy");
  setvalueline();
}

void
DBQTimeDate::setEditable(bool e)
{
  f_editable=e;
  updateAppearance();
}

void
DBQTimeDate::updateAppearance()
{
  if (f_set!=f_originallyset)
    f_dirty=1;
  else if (!f_set)
    f_dirty=0;
  else if (f_date && (dt->date().year()!=o_year || dt->date().month()!=o_month ||
                      dt->date().day()!=o_day))
    f_dirty=1;
  else if (f_time && (dt->time().hour()!=o_hour || dt->time().minute()!=o_minute ||
                 dt->time().second()!=o_second))
    f_dirty=1;
  else
    f_dirty=0;
  DBQScoreBox::updateAppearance();
  setvalueline();
  if (f_editable && f_set)
    dt->show();
  else
    dt->hide();
}

void
DBQTimeDate::changed()
{
  setvalueline();
  updateAppearance();
}

void
DBQTimeDate::setvalueline()
{
  if (!f_set) return;
//   if (!f_set) {
//     valueline->setText("<nodata>");
//     return;
//   }
  string val;
  if (f_date)
    val+=dt->date().toString("d-MMM-yyyy ").toStdString();
  if (f_time)
    val+=dt->time().toString("h:mmap").toStdString();
  valueline->setText(val.c_str());
}

void
DBQTimeDate::revertclicked()
{
  // reset date and/or time if we originally had one
  if (f_date && f_originallyset)
    dt->setDate(QDate(o_year,o_month,o_day));
  if (f_time && f_originallyset)
    dt->setTime(QTime(o_hour,o_minute,o_second));
  // restore original set status
  f_set=f_originallyset;
  updateAppearance();
}

void
DBQTimeDate::deleteclicked()
{
  f_set=!f_set;
  updateAppearance();
}

void
DBQTimeDate::getValue(DBscorevalue &val)
{
  val.v_date.setDate(dt->date().month(),dt->date().day(),dt->date().year());
  val.v_date.setTime(dt->time().hour(),dt->time().minute(),dt->time().second());
  val.scorename=scorename;
  // boost::format fmter("%d %d %d %d %d %d");
  // fmter %
  //   (f_date ? dt->date().year() : 0) %
  //   (f_date ? dt->date().month() : 0) %
  //   (f_date ? dt->date().day() : 0) %
  //   (f_time ? dt->time().hour() : 0) %
  //   (f_time ? dt->time().minute() : 0) %
  //   (f_time ? dt->time().second() : 0);
  // return fmter.str();
}
