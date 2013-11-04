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
#ifndef _qtTimeSlider_h
#define _qtTimeSlider_h

#include <qslider.h>
#include <qpalette.h>
#include <puTools/miTime.h>

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

  void clear();
  void setLastTimeStep();
  ///Current time
  miutil::miTime Value();
  /// current index
  int current() { return value(); };
  /// get start time
  miutil::miTime getStartTime() { return start; };
  ///Number of times currently in the slider
  int numTimes() const {return times.size();}
  ///Next/previous time
  bool nextTime(const int dir, miutil::miTime& time, bool restricted =false);
  void setLoop(const bool b);
  std::vector<miutil::miTime> getTimes(){return times;}
  void startAnimation(){startani= true;}
  ///Remove times from data type
  void deleteType(const std::string& type);
  
  void set(const miutil::miTime&);

 public Q_SLOTS:
  void setMinMax(const miutil::miTime& t1, const miutil::miTime& t2);
  void clearMinMax();
  ///add new times for datatype
  void insert(const std::string& datatype, const std::vector<miutil::miTime>&,bool =true);
 /// force new value
  void setTime(const miutil::miTime&);
 /// force new value if datatype match
  void setTime( const std::string& datatype, const miutil::miTime&);
  /// time-interval changed
  void setInterval(int);
  ///use times from datatype(field, sat, obs ..)
  void useData(std::string datatype);

Q_SIGNALS:
  /// emits smallest timeinterval (in hours)
  void minInterval(int);
  /// emits minimum timesteps
  void timeSteps(int,int);
  /// enable/disable spinbox
  void enableSpin(bool);
  void sliderSet();
  /// emits times
  void newTimes(std::vector<miutil::miTime>&);

private:
  std::map<std::string,std::vector<miutil::miTime> > tlist; // times
  std::map<std::string,bool>  usetlist; // false if only one spesific time is set
  std::vector<miutil::miTime> orig_times; // the actual timepoints (all)
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

  void init();
  void setFirstTime(const miutil::miTime&);
  void updateList();
  bool setSliderValue(const int v);
};

#endif
