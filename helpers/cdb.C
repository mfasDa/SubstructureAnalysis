int getYearForRunNumber(int runnumber){
  int year=2015;
  if(runnumber <= 247170) year = 2015;
  else if(runnumber <= 267254) year = 2016;
  else if(runnumber <= 282900) year = 2017;
  else year = 2018;
  return year;
}