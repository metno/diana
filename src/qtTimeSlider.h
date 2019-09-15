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
#ifndef _qtTimeSlider_h
#define _qtTimeSlider_h

#include "diTimeTypes.h"

#include <qslider.h>
#include <qpalette.h>

#include <map>
#include <vector>

/**
  \brief Main plot time 

  The time slider class holds times from all data sources selected,
  but the slider itself only shows times from one data source at the time 

*/
class TimeSlider : public QSlider {
  Q_OBJECT

public:
  TimeSlider(QWidget*);
  
  TimeSlider(Qt::Orientation, QWidget*);

  ///Current time
  miutil::miTime Value();
  //! return true iff given time is in time list
  bool hasTime(const miutil::miTime& time) const;
  /// get start time
  const miutil::miTime& getStartTime() { return start; };
  ///Number of times currently in the slider
  int numTimes() const {return times.size();}
  ///Next/previous time
  bool nextTime(const int dir, miutil::miTime& time);
  void setLoop(bool b);
  void startAnimation(){startani= true;}
  ///Remove times from data type
  void deleteType(const std::string& type);
  miutil::miTime getLatestPlotTime();
  
public Q_SLOTS:
  void setMinMax(const miutil::miTime& t1, const miutil::miTime& t2);
  void clearMinMax();
  ///add new times for datatype
  void insert(const std::string& datatype, const plottimes_t&, bool = true);
  /// force new value
  void setTime(const miutil::miTime&);
  /// force new value if datatype match
  bool setTimeForDataType(const std::string& datatype, const miutil::miTime&);
  /// time-interval changed
  void setInterval(int);
  ///use times from datatype(field, sat, obs ..)
  void useData(const std::string& datatype);

Q_SIGNALS:
  /// emits smallest timeinterval (in hours)
  void minInterval(int);
  /// emits minimum timesteps
  void timeSteps(int,int);
  /// enable/disable spinbox
  void enableSpin(bool);
  void sliderSet();
  void lastStep();
  /// emits times
  void newTimes(const plottimes_t&);

private:
  void set(const miutil::miTime&);
  void init();
  void setFirstTime(const miutil::miTime&);
  bool useDataType(const std::string& dt, bool ifUsed);
  void updateList();
  bool setSliderValue(int v);

private:
  typedef std::map<std::string, plottimes_t> tlist_t;
  tlist_t tlist; // times
  typedef std::map<std::string, bool> usetlist_t;
  usetlist_t usetlist;               // false if only one spesific time is set
  plottimes_t orig_times;            // the actual timepoints (all)
  std::vector<miutil::miTime> times; // the actual timepoints (min-max)
  miutil::miTime prevtime; // previous selected time
  float interval;  // timeinterval in hours
  bool loop;       // time-loop in use
  miutil::miTime start;    // restrict nextTime()  
  miutil::miTime stop;     // -------
  bool useminmax;  // use restricted timeinterval
  QPalette pal;    // keep original slider-palette
  bool startani;   // animation just started
  std::string  dataType; //dataType has priority   
  std::string  dataTypeUsed; //dataType in use
};

#endif
