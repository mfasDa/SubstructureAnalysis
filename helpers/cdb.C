#ifndef __CDB_C__
#define __CDB_C__

int getYearForRunNumber(int runnumber){
  int year=2010;
  if(runnumber < 139674) year = 2010;
  else if(runnumber < 170718) year = 2011;
  else if(runnumber < 194479) year = 2012;
  else if(runnumber < 199999) year = 2013;
  else if(runnumber < 208504) year = 2014;
  else if(runnumber <= 247170) year = 2015;
  else if(runnumber <= 267254) year = 2016;
  else if(runnumber <= 282900) year = 2017;
  else year = 2018;
  return year;
}
#endif