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
#include <puTools/miString.h>
#include <map>
#include <vector>

using namespace std; 

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
  ///Current time
  miTime Value();
  /// current index
  int current() { return value(); };
  /// get start time
  miTime getStartTime() { return start; };
  ///Number of times currently in the slider
  int numTimes() const {return times.size();}
  ///Next/previous time
  bool nextTime(const int dir, miTime& time, bool restricted =false);
  void setLoop(const bool b);
  vector<miTime> getTimes(){return times;}
  void startAnimation(){startani= true;}
  ///Remove times from data type
  void deleteType(const miString& type);
  
  void set(const miTime&);

 public slots:
  void setMinMax(const miTime& t1, const miTime& t2);
  void clearMinMax();
  ///add new times for datatype
  void insert(const miString& datatype, const vector<miTime>&,bool =true);
 /// force new value
  void setTime(const miTime&);
 /// force new value if datatype match
  void setTime( const miString& datatype, const miTime&);
  /// time-interval changed
  void setInterval(int);
  ///use times from datatype(field, sat, obs ..)
  void useData(miString datatype);

  signals:
  /// emits smallest timeinterval (in hours)
  void minInterval(int);
  /// emits minimum timesteps
  void timeSteps(int,int);
  /// enable/disable spinbox
  void enableSpin(bool);
  void sliderSet();
  /// emits times
  void newTimes(vector<miTime>&);

private:
  map<miString,vector<miTime> > tlist; // times
  map<miString,bool>  usetlist; // false if only one spesific time is set
  vector<miTime> orig_times; // the actual timepoints (all)
  vector<miTime> times; // the actual timepoints (min-max)
  miTime prevtime; // previous selected time
  float interval;  // timeinterval in hours
  bool loop;       // time-loop in use
  miTime start;    // restrict nextTime()  
  miTime stop;     // -------
  bool useminmax;  // use restricted timeinterval
  QPalette pal;    // keep original slider-palette
  bool startani;   // animation just started
  miString  dataType; //dataType has priority   
  miString  dataTypeUsed; //dataType in use

  void init();
  void setFirstTime(const miTime&);
  void updateList();
  bool setSliderValue(const int v);
};

#endif
