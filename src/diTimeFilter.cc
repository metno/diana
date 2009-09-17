/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <diTimeFilter.h>

// Default constructor
TimeFilter::TimeFilter():OK(false){
}

//find and remember position of time info,
//return filename whith time info replaced by *
bool TimeFilter::initFilter(miString &filename){

  miString filter=filename;

  if(filter.contains("["))
    advanced=true;
  else
    advanced=false;

  if(filter=="OFF"){
    OK = false;
    return OK;
  }

  if(filter.find("/") == filter.npos){
    noSlash = true;
  } else {
    noSlash =false;
  }

  if(advanced){
    bool legal=false;
    int n = filter.size();
    legalPos.clear();
    for(int i=0; i<n;i++){
      if(filter[i]=='[') legal=true;
      else if(filter[i]==']') legal=false;
      else legalPos.push_back(legal);
    }
    filter.remove('[');
    filter.remove(']');
  }

  if( (yyyy = findPos(filter,"yyyy")) == filter.npos )
    yy = findPos(filter,"yy");

  mm = findPos(filter,"mm");
  dd = findPos(filter,"dd");
  HH = findPos(filter,"HH");
  if( (MM = findPos(filter,"MM")) == filter.npos )
    M  = findPos(filter,"M");
  XX = findPos(filter,"XX");
  dat = (findPos(filter,".dat*") != filter.npos) && dd==filter.npos
    && mm==filter.npos && yy==filter.npos && yyyy==filter.npos;

  if(dat)
    OK=true;

  else if(dd!=filter.npos && mm!=filter.npos &&
	  (yy!=filter.npos || yyyy!=filter.npos) )
    OK=true;

  else
    OK=false;

  if(advanced){
    //replace [...] with [??..??]
    unsigned int pos1,pos2;
    while(((pos1=filename.find("[")) != filename.npos)
	  && ((pos2=filename.find("]")) != filename.npos)){
      miString s1= filename.substr(0,pos1);
      miString s2 = filename.substr(pos2+1);
      miString s3 = filename.substr(pos1+1,pos2-pos1-1);
      replaceKey(s3);
      filename = s1 + s3 + s2;
    }

  } else {
    replaceKey(filename);
  }

  return OK;

}

void TimeFilter::replaceKey(miString& str)
{
  str.replace("yyyy","????");
  str.replace("yy","??");
  str.replace("mm","??");
  str.replace("dd","??");
  str.replace("HH","??");
  str.replace("MM","??");
  str.replace("XX","??");
}

unsigned int TimeFilter::findPos( const miString& filter, const miString& s)
{

  if(advanced){
    unsigned int pos=0;
    while(true){
      pos = filter.find(s,pos);
      if( pos==filter.npos)return filter.npos;
      if( legalPos[pos] ) return pos;
      pos++;
    }

  }else {
    return filter.find(s);
  }

}

bool TimeFilter::getClock(miString name, miClock &clock) {

  if( name.empty() )
    return false;

  if(noSlash){
    if(name.find("/") != name.npos)
      name = name.substr(name.find_last_of("/")+1,name.size()-1);
  }

  int hour=12,min=0,sec=0;
  miString s;

  if (XX!=name.npos){     // Metar files
    if(XX>name.size()-2) return false;
    s = name.substr(XX,2);
    int x=atoi(s.c_str());
    hour=x/2;
    if(hour*2 != x)
      min=30;
  }

  if (HH!=name.npos){
    if( HH>name.size()-2) return false;
    s = name.substr(HH,2);
    hour=atoi(s.c_str());
  }

  if (MM!=name.npos){
    if(MM>name.size()-2) return false;
    s = name.substr(MM,2);
    min=atoi(s.c_str());
  } else if (M!=name.npos){
    if(M>name.size()-1) return false;
    s = name.substr(M,1);
    min=atoi(s.c_str())*10;
  }

  miClock c(hour,min,sec);
  clock=c;

  return true;

}


bool TimeFilter::getTime(miString name, miTime &time) {

  //   cerr <<"getTime:"<<name<<endl;
  if( name.empty() )
    return false;
  if(noSlash){
    if(name.find("/") != name.npos)
      name = name.substr(name.find_last_of("/")+1,name.size()-1);
  }

  if(!OK)
    return false;

  miClock clock;
  if(!getClock(name,clock))
    return false;

  if (dat){

    if(name.find(".dat") == name.npos)
      return false;

    miString endstr = name.substr(name.find(".dat"),name.size());
    //return false if not  .dat or .dat-1, .dat-2 etc
    if(endstr.size()!=4 && endstr.size()!=6)
       return false;

    miTime now=miTime::nowTime();
    miClock nowClock=now.clock();
    miDate nowDate=now.date();


    bool after = nowClock < clock;
    time = miTime(nowDate,clock);                // this day
    if(name.find(".dat-") == name.npos){
      if(after)
	time.addDay(-1);                           //last day
    } else{
      int x = atoi(name.substr(name.find_last_of("-"),name.size()-1).c_str());
      if(after)
	time.addDay(x-1);
      else
	time.addDay(x);
    }

    //Don't know if this file has been updated or not, time is set anyway
    if(abs(miClock::minDiff(clock,nowClock))<15)
      return false;

    return true;
  }

  int y,m,d;

  if(dd==name.npos || dd>name.size()-2)
    return false;

  miString s = name.substr(dd,2);
  d=atoi(s.c_str());

  if(mm==name.npos || mm>name.size()-2)
    return false;

  s = name.substr(mm,2);
  m=atoi(s.c_str());

  if(yyyy==name.npos && yy==name.npos)
    return false;

  if(yyyy!=name.npos  && yyyy<name.size()-4){
    s = name.substr(yyyy,4);
    y=atoi(s.c_str());
  }
  else if( yy<name.size()-2){
    s = name.substr(yy,2);
    y=atoi(s.c_str());
    if( y>50)
      y+=1900;
    else
      y+=2000;
  }

  miDate date(y,m,d);
  time=miTime(date,clock);

  if(time.undef()) return false;

  return true;

}

