/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diTimeFilter.cc 2803 2012-03-06 12:24:27Z lisbethb $

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "TimeFilter.h"
#include <cstdlib>

using namespace::miutil;

// Default constructor
TimeFilter::TimeFilter():OK(false) {
}

//find and remember position of time info,
//return filename whith time info replaced by *
bool TimeFilter::initFilter(std::string &filename, bool advanced_){

  if ( advanced_ &&
      ( filename.find("[") == filename.npos || filename.find("[") == filename.npos)) {
    OK = false;
    return OK;
  }

  std::string filter=filename;

  if(advanced_ || filter.find_first_of("[") != filter.npos)
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
    // remove all brackets
    while (filter.find("[") != filename.npos) {
      filter.erase(filter.find('['),1);
      filter.erase(filter.find(']'),1);
    }
  }

  if( (yyyy = findPos(filter,"yyyy")) == filter.npos ){
    yy = findPos(filter,"yy");
  } else {
    yy = filter.npos;
  }

  mm = findPos(filter,"mm");
  dd = findPos(filter,"dd");
  HH = findPos(filter,"HH");
  MM = findPos(filter,"MM");
  SS = findPos(filter,"SS");

if(dd!=filter.npos && mm!=filter.npos &&
    (yy!=filter.npos || yyyy!=filter.npos) ) {
    OK=true;
  } else {
    OK=false;
  }

  if(advanced){
    //replace [...] with [??..??]
    std::string::size_type pos1,pos2;
    while(((pos1=filename.find("[")) != filename.npos)
	  && ((pos2=filename.find("]")) != filename.npos)){
      std::string s1= filename.substr(0,pos1);
      std::string s2 = filename.substr(pos2+1);
      std::string s3 = filename.substr(pos1+1,pos2-pos1-1);
      replaceKey(s3);
      filename = s1 + s3 + s2;
    }

  } else {
    replaceKey(filename);
  }

  return OK;

}

void TimeFilter::replaceKey(std::string& str)
{
  std::string s = "????";
  try{
  str.replace(str.find("yyyy"),4,s);
  } catch (...){
  }
  s="??";
  try{
  str.replace(str.find("yy"),2,s);
  } catch (...){
  }
  try{
  str.replace(str.find("mm"),2,s);
  } catch (...){
  }
  try{
  str.replace(str.find("dd"),2,s);
  } catch (...){
  }
  try{
  str.replace(str.find("HH"),2,s);
  } catch (...){
  }
  try{
  str.replace(str.find("MM"),2,s);
  } catch (...){
  }
  try{
  str.replace(str.find("SS"),2,s);
  } catch (...){
  }
}

std::string::size_type TimeFilter::findPos( const std::string& filter, const std::string& s)
{

  if(advanced){
    std::string::size_type pos=0;
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

bool TimeFilter::getClock(std::string name, miClock &clock) {

  if( name.empty() )
    return false;

  if(noSlash){
    if(name.find("/") != name.npos)
      name = name.substr(name.find_last_of("/")+1,name.size()-1);
  }

  int hour=12,min=0,sec=0;
  std::string s;

  if (HH!=name.npos){
    if( HH>name.size()-2) return false;
    s = name.substr(HH,2);
    hour=atoi(s.c_str());
  }

  if (MM!=name.npos){
    if(MM>name.size()-2) return false;
    s = name.substr(MM,2);
    min=atoi(s.c_str());
  }

  if (SS!=name.npos){
    if(SS>name.size()-2) return false;
    s = name.substr(SS,2);
    sec=atoi(s.c_str());
  }

  miClock c(hour,min,sec);
  clock=c;

  return true;

}


bool TimeFilter::getTime(std::string name, miTime &time)
{
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

  int y = 0,m,d;

  if(dd==name.npos || dd>name.size()-2)
    return false;

  std::string s = name.substr(dd,2);
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

std::string TimeFilter::getTimeStr(std::string filename)
{

  std::string timestr;
  miTime time;
  if ( getTime(filename, time) ) {
    timestr = time.isoTime("T");
  }

  return timestr;

}
