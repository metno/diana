/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diana_config.h"

#include "qtTimeSlider.h"
#include <climits>
#define MILOGGER_CATEGORY "diana.TimeSlider"
#include <miLogger/miLogging.h>


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
  plottimes_t tmp;
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

miutil::miTime TimeSlider::Value()
{
  int v = value();
  if (v>= 0 && v < int(times.size()))
    return times[size_t(v)];
  else
    return miutil::miTime::nowTime();
}

miutil::miTime TimeSlider::getLastTime()
{
  if (orig_times.size())
    return *orig_times.rbegin();
  return miutil::miTime();
}

bool TimeSlider::hasTime(const miutil::miTime& time) const
{
  return (std::find(times.begin(), times.end(), time) != times.end());
}

void TimeSlider::setInterval(int in)
{
  interval= in;
}

void TimeSlider::setMinMax(const miutil::miTime& t1, const miutil::miTime& t2)
{
  if (!orig_times.empty()) {
    const miutil::miTime& first = *orig_times.begin();
    const miutil::miTime& last = *orig_times.rbegin();
    if (t1 >= first && t1 <= last && t2 >= t1 && t2 <= last) {
      start = t1;
      stop = t2;
      useminmax = true;
      QPalette p(Qt::red);
      setPalette(p);
    }
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

void TimeSlider::setLoop(bool b)
{
  loop= b;
}

bool TimeSlider::nextTime(const int dir, miutil::miTime& time)
{
  if (dir == 0)
    return false;

  {
    const int v = value(), n = times.size();
    if (n == 0 || v < 0 || v >= n)
      return false;
    if (v == n - 1 && times[v] == *orig_times.rbegin())
      Q_EMIT lastStep(); // might change 'times'
  }

  const int n = times.size(), last = n - 1;
  int v = value();
  if (n == 0 || v < 0 || v >= n) // check again, 'times' might have changed
    return false;

  const bool wrap = loop || startani;
  if (!wrap) {
    if (dir > 0 && v == last)
      return false;
    if (dir < 0 && v == 0)
      return false;
  }

  const miutil::miTime& current = times[v];
  if (interval == 0) {
    if (dir > 0) {
      if (v < last)
        v += 1;
      else if (wrap)
        v = 0;
      else
        return false;
    } else { // dir < 0
      if (v > 0)
        v -= 1;
      else if (wrap)
        v = last;
      else
        return false;
    }
  } else {
    miutil::miTime t = current;
    t.addMin(int(dir * interval * 60));
    if (dir > 0) {
      while (t > times[v] && v < last)
        v++;
      if (wrap && t > times[v])
        v = 0;
    } else { // dir < 0
      while (t < times[v] && v > 0)
        v--;
      if (wrap && t < times[v])
        v = last;
    }
  }

  startani = false;
  time = times[v];
  return time != current;
}

bool TimeSlider::useDataType(const std::string& dt, bool ifUsed)
{
  tlist_t::const_iterator it = tlist.find(dt);
  if (it == tlist.end())
    return false;
  if (ifUsed) {
    usetlist_t::const_iterator ut = usetlist.find(dt);
    if (ut == usetlist.end() || !ut->second)
      return false;
  }
  if (it->second.empty())
    return false;
  orig_times = it->second;
  dataTypeUsed = dt;
  return true;
}

static const struct DataTypeUse
{
  std::string dataType;
  bool ifUsed;
} dataTypeOrder[] = {{"field", false}, {"sat", true},       {"obs", false},    {"obj", true},  {"product", false},
                     {"vprof", false}, {"spectrum", false}, {"vcross", false}, {"sat", false}, {"obj", false}};

void TimeSlider::updateList()
{
  const int maxticks= 20;

  orig_times.clear();

  // FIXME this is very similar to bdiana_capi.cc:selectTime / BdianaMain::getTime
  bool found = false;
  if (!dataType.empty())
    found = useDataType(dataType, true);
  if (!found) {
    for (const DataTypeUse& dtu : dataTypeOrder) {
      if ((found = useDataType(dtu.dataType, dtu.ifUsed)))
        break;
    }
  }
  if (!found) {
    // Just use the first list of times in the collection.
    for (const auto& tt : tlist) {
      if (useDataType(tt.first, false)) {
        break;
      }
    }
  }

  plottimes_t::const_iterator qs, qe;
  if (useminmax) {
    qs = std::lower_bound(orig_times.begin(), orig_times.end(), start);
    qe = std::lower_bound(qs, orig_times.end(), stop);
    if (qe != orig_times.end())
      ++qe;
  } else {
    qs = orig_times.begin();
    qe = orig_times.end();
  }

  // make sure productimes are included
  tlist_t::const_iterator pt = tlist.find("product");
  plottimes_t timeset;
  if (pt != tlist.end() && !pt->second.empty()) {
    timeset.insert(qs, qe);
    timeset.insert(pt->second.begin(), pt->second.end());
    qs = timeset.begin();
    qe = timeset.end();
  }
  times = std::vector<miutil::miTime>(qs, qe);

  const int n= times.size();
  if (n>1) {
    // find time-intervals
    int iv, miniv=INT_MAX, maxiv=0;
    for (int i=1; i<n; i++){
      iv= miutil::miTime::minDiff(times[i],times[i-1]);
      if (iv<miniv)
        miniv= iv;
      if (iv>maxiv)
        maxiv= iv;
    }
    if (miniv<1)
      miniv= 1;
    int meaniv = (miniv+maxiv)/2;
    //HK added next line in order to avoid division by zero
    if (meaniv<1)
      meaniv= 1;
    // calculate reasonable tickmarks
    iv= 180/meaniv;
    if (iv==0)
      iv= 1;
    while (n/iv>maxticks)
      iv*= 2;

    setTickPosition(TicksBelow);
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

void TimeSlider::setTime(const miutil::miTime& t)
{
  set(t);
}

bool TimeSlider::setTimeForDataType(const std::string& datatype, const miutil::miTime& t)
{
  if (datatype != dataTypeUsed)
    return false;
  set(t);
  return true;
}


// Set slider to reasonable value - called after
// changes in the total timeseries. Try to keep current
// time close to previous selected time - if that is impossible,
// choose time nearest nowtime - and last resort: first time
void TimeSlider::setFirstTime(const miutil::miTime& t)
{
  miutil::miTime ptime;
  miutil::miTime now = miutil::miTime::nowTime();
  if (t.undef()) { // only very first time we enter setFirstTime
    ptime= now;  // try to choose nowtime
  } else
    ptime= t;

  int n= times.size();
  int i;

  for (i=0; i<n && times[i]<ptime; i++)
    ;
  if (i==n)
    i= n-1;

  if (times[i] == ptime) { // exact time
    setSliderValue(i);
  } else if (i>=n-1 || i==0) { // previous time outside interval
    // check if nowtime inside interval
    if (now>=times[0] && now<=times[n-1]) {
      for (i=0; i<n && times[i]<now; i++)
        ;
      if (i==n)
        i= n-1;
      setSliderValue(i);

    } else {
      // use time closest to nowtime
      if (now < times[0]) {
        setSliderValue(0);
      } else if (tlist["field"].size() == 0) {
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
    else
      setSliderValue(i-1);
  }

  emit sliderSet();
}


// Set slider to value v (if legal value)
// Remember corresponding miutil::miTime as prevtime.
bool TimeSlider::setSliderValue(int v)
{
  int n= times.size();
  if (v>=0 && v<n) {
    setValue(v);
    prevtime = times[v];
    return true;
  } else {
    return false;
  }
}


// set slider to value corresponding to a miutil::miTime
// For illegal values: do nothing
void TimeSlider::set(const miutil::miTime& t)
{
  int n= times.size();
  int v=-1, i;
  for (i=0; i<n && times[i]<t; i++) {
  }
  if ((i<n) && (t==times[i]))
    v=i;

  setSliderValue(v);
}

void TimeSlider::insert(const std::string& datatype, const plottimes_t& vt, bool use)
{
  tlist[datatype] = vt;
  usetlist[datatype] = use;
  updateList();
  Q_EMIT newTimes(orig_times);
}

void TimeSlider::useData(const std::string& datatype)
{
  dataType = datatype;
  updateList();
  Q_EMIT newTimes(orig_times);
}

void TimeSlider::deleteType(const std::string& type)
{
  tlist_t::iterator p = tlist.find(type);
  if (p != tlist.end())
    tlist.erase(p);
}
