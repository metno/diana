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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtTimeSlider.h"
#include <climits>

using namespace std;

TimeSlider::TimeSlider(QWidget* parent)
  : QSlider(parent)
  , loop(false)
{
  init();
}


TimeSlider::TimeSlider(Qt::Orientation ori, QWidget *parent)
  : QSlider(ori, parent)
  , loop(false)
{
  init();
}


void TimeSlider::init()
{
  vector<miutil::miTime> tmp;
  tlist["field"]= tmp;
  tlist["sat"]= tmp;
  tlist["obs"]= tmp;
  tlist["product"]= tmp;

  interval= 0.0;
  setEnabled(false);
  useminmax= false;
  pal= palette();
  startani= false;
}

miutil::miTime TimeSlider::Value(){
  unsigned int v= value();
  if (v < times.size())
    return times[v];
  else return miutil::miTime::nowTime();
}


void TimeSlider::clear(){
  init();
  //  merge();
}

void TimeSlider::setLastTimeStep() {
  setSliderValue(times.size()-1);
  emit sliderSet();
}

void TimeSlider::setInterval(int in){
  interval= in;
}

void TimeSlider::setMinMax(const miutil::miTime& t1, const miutil::miTime& t2)
{

  int n= orig_times.size()-1;
  if (t1>=orig_times[0] && t1<=orig_times[n] &&
      t2>=t1 && t2<=orig_times[n]){
    start= t1;
    stop= t2;
    useminmax= true;
    // change colours..
//     QColorGroup cg= pal.active();
//     cg.setColor(QColorGroup::Foreground,Qt::red);
//     cg.setColor(QColorGroup::Button,Qt::red);
    QPalette p(Qt::red);
    setPalette(p);
  }

  updateList();

}

void TimeSlider::clearMinMax()
{
  useminmax= false;
  // reset colours..
  setPalette(pal);
  updateList();
}

void TimeSlider::setLoop(const bool b){
  loop= b;
}

bool TimeSlider::nextTime(const int dir, miutil::miTime& time, bool restricted){

  miutil::miTime t, current;
  int v= value();
  int n= times.size();
  if (n==0 || v>=n) return false;

  // start-stop indices
  int i1= 0, i2= n-1;

  current= times[v];

  if (!loop && !startani){
    if (dir>0 && v==i2) return false;
    if (dir<0 && v==i1) return false;
  }

  t= current;
  t.addMin(int(dir*interval*60));

  if (dir>0){
    // if interval==0: pick next time
    if (t==current) {
      if (v<i2)
	time= times[v+1];
      else if (loop || startani)
	time= times[i1];
      else return false;
      // interval!=0
    } else {
      while (t>times[v] && v<i2)
	v++;
      if (t>times[v]) {
	if (loop || startani) v= i1;
	else v= i2;
      }
      time= times[v];
    }

    // backwards timestep
  } else if (dir<0) {
    // if interval==0: pick previous time
    if (t==current) {
      if (v>i1)
	time= times[v-1];
      else if (loop || startani)
	time= times[i2];
      else return false;
      // interval!=0
    } else {
      while (t<times[v] && v>i1)
	v--;
      if (t<times[v]) {
	if (loop || startani) v=i2;
	else v= i1;
      }
      time= times[v];
    }
  } else time= current;

  startani= false;
  return time != current;
}

void TimeSlider::insert(const std::string& datatype,
                        const std::vector<miutil::miTime>& vt,
                        bool use)
{
  tlist[datatype]= vt;
  usetlist[datatype] = use;
  updateList();
  emit newTimes(orig_times);
}

void TimeSlider::updateList()
{
  const int maxticks= 20;

  times.clear();
  if( tlist[dataType].size()>0 && usetlist[dataType]){
    times = tlist[dataType];
    dataTypeUsed = dataType;
  }
  else if( tlist["field"].size()>0 ){
    times = tlist["field"];
    dataTypeUsed = "field";
  }
  else if( tlist["sat"].size()>0 && usetlist["sat"]){
    times = tlist["sat"];
    dataTypeUsed = "sat";
  }
  else if( tlist["obs"].size()>0 ){
    times = tlist["obs"];
    dataTypeUsed = "obs";
  }
  else if ( tlist["obj"].size()>0 && usetlist["obj"]) {
    times = tlist["obj"];
    dataTypeUsed = "obj";
  }
  else if( tlist["product"].size()>0 ){
    times = tlist["product"];
    dataTypeUsed = "product";
  }
  else if( tlist["vprof"].size()>0 ){
    times = tlist["vprof"];
    dataTypeUsed = "vprof";
  }
  else if( tlist["spectrum"].size()>0 ){
    times = tlist["spectrum"];
    dataTypeUsed = "spectrum";
  }
  else if( tlist["vcross"].size()>0 ){
    times = tlist["vcross"];
    dataTypeUsed = "vcross";
  }
  else if ( tlist["sat"].size()>0) {
    times = tlist["sat"];
    dataTypeUsed = "sat";
  }
  else if ( tlist["obj"].size()>0) {
    times = tlist["obj"];
    dataTypeUsed = "obj";
  }
  else {
    // Just use the first list of times in the collection.
    map<std::string,vector<miutil::miTime> >::iterator it;
    for (it = tlist.begin(); it != tlist.end(); ++it) {
      if (it->second.size() > 0) {
        times = it->second;
        dataTypeUsed = it->first;
        break;
      }
    }
  }

  orig_times = times; // orig_times = all times

  if(useminmax){
    vector<miutil::miTime>::iterator q;
    for (q=times.begin(); q!=times.end() && *q<start; q++)
	;
    times.erase(times.begin(),q);

    for (q=times.begin(); q!=times.end() && *q<stop; q++)
      ;
    if( q!=times.end() )
      q++;
    times.erase(q,times.end());

  }



  // make sure productimes are included
  vector<miutil::miTime> vpt= tlist["product"];
  int pn= vpt.size();
  if (pn>0){
    for (int i=0; i<pn; i++){
      vector<miutil::miTime>::iterator q;
      for (q=times.begin(); q!=times.end() && *q<vpt[i]; q++)
	;
      if ((q==times.end()) || (vpt[i]!=*q))
	times.insert(q, vpt[i]);
    }
  }

  int i,n;
  n= times.size();
  if (n>1) {
    // find time-intervals
    int iv, miniv=INT_MAX, maxiv=0;
    for (i=1; i<n; i++){
      iv= miutil::miTime::minDiff(times[i],times[i-1]);
      if (iv<miniv) miniv= iv;
      if (iv>maxiv) maxiv= iv;
    }
    if (miniv<1) miniv= 1;
     int meaniv = (miniv+maxiv)/2;
     //HK added next line in order to avoid division by zero
    if (meaniv<1) meaniv= 1;
    // calculate reasonable tickmarks
    iv= 180/meaniv;
    if (iv==0) iv= 1;
    while (n/iv>maxticks) iv*= 2;

    // qt4 fix: Edited the line below (old line commented out)
    setTickPosition(TicksBelow); //setTickmarks(TickSetting(Below));
    setTickInterval(iv);
    setSingleStep(1);
    setPageStep(iv);

    float hourinterval= miniv/60.0;
    // emit smallest timeinterval (in hours)
    // and steps for interval-spinbox
    if(hourinterval<1) {
      if (hourinterval>interval)
	emit minInterval(0);
      emit timeSteps(1,2);
    } else {
      emit timeSteps(int(hourinterval),int(2*hourinterval));
    }

    emit enableSpin(true);
    setEnabled(true);
    setRange(0,n-1);
    setFirstTime(prevtime);
  } else if(n==1){
    setFirstTime(prevtime);
    //emit minInterval(0);
    emit enableSpin(false);
    setEnabled(false);
  } else {
    //emit minInterval(0);
    emit enableSpin(false);
    setEnabled(false);
  }

  // check if start/stop still valid
  //  if (useminmax) setMinMax(start,stop);
}

void TimeSlider::setTime(const miutil::miTime& t){
  set(t);
}

void TimeSlider::setTime(const std::string& datatype, const miutil::miTime& t)
{
  if (datatype == dataTypeUsed)
    set(t);
}


// Set slider to reasonable value - called after
// changes in the total timeseries. Try to keep current
// time close to previous selected time - if that is impossible,
// choose time nearest nowtime - and last resort: first time
void TimeSlider::setFirstTime(const miutil::miTime& t){
  miutil::miTime ptime;
  miutil::miTime now = miutil::miTime::nowTime();
  if (t.undef()){ // only very first time we enter setFirstTime
    ptime= now;  // try to choose nowtime
  } else
    ptime= t;

  int n= times.size();
  int i;

  for (i=0; i<n && times[i]<ptime; i++)
    ;
  if (i==n) i= n-1;

  if(times[i]==ptime){ // exact time
    setSliderValue(i);
  } else if (i>=n-1 || i==0){ // previous time outside interval
    // check if nowtime inside interval
    if (now>=times[0] && now<=times[n-1]){
      for (i=0; i<n && times[i]<now; i++)
	;
      if (i==n) i= n-1;
      setSliderValue(i);

    } else {
      // use time closest to nowtime
      if (now < times[0]) {
	setSliderValue(0);
      } else if (tlist["field"].size()==0) {
	setSliderValue(n-1);
      } else {
	miutil::miTime tmax= times[n-1];
	tmax.addHour(24);
	if (now<tmax)
	  setSliderValue(n-1);
	else
	  setSliderValue(0);
      }
    }

  } else {
    // find closest time
    int nsec= miutil::miTime::secDiff(times[i],ptime);
    int psec= miutil::miTime::secDiff(ptime,times[i-1]);
    if (nsec < psec)
      setSliderValue(i);
    else setSliderValue(i-1);
  }

  emit sliderSet();
}


// Set slider to value v (if legal value)
// Remember corresponding miutil::miTime as prevtime.
bool TimeSlider::setSliderValue(const int v)
{
  int n= times.size();
  if (v>=0 && v<n){
    setValue(v);
    prevtime= times[v];
  } else return false;
  return true;
}


// set slider to value corresponding to a miutil::miTime
// For illegal values: do nothing
void TimeSlider::set(const miutil::miTime& t){
  int n= times.size();
  int v=-1, i;
  for (i=0; i<n && times[i]<t; i++)
    ;
  if ((i<n) && (t==times[i])) v=i;

  setSliderValue(v);
}

void TimeSlider::useData(std::string datatype){
  dataType = datatype;
  updateList();
  emit newTimes(orig_times);

}

void TimeSlider::deleteType(const std::string& type)
{
  map< std::string, vector<miutil::miTime> >::iterator p    = tlist.begin();
  map< std::string, vector<miutil::miTime> >::iterator pend = tlist.end();

  while( p!=pend && type!=p->first ) p++;
  if(p!=pend)
    tlist.erase(p);

}
