/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2018 met.no

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

#ifndef TIMENAVIGATOR_H
#define TIMENAVIGATOR_H

#include "diTimeTypes.h"

#include <QObject>

#include <vector>

class QAction;
class QLabel;
class QShortcut;
class QTimerEvent;
class QToolBar;
class TimeSlider;
class TimeStepSpinbox;
class TimeControl;

class TimeNavigator : public QObject
{
  Q_OBJECT
public:
  explicit TimeNavigator(QWidget *parent = 0);

  void requestTime(const miutil::miTime&);
  bool hasTimes() const;
  miutil::miTime selectedTime();
  void removeTimes(int id);
  void useData(const std::string& type, int id=-1);
  bool isTimerOn() const
    { return animationDirection_ != 0; }
  std::vector<miutil::miTime> animationTimes() const;

  QToolBar* toolbar()
    { return toolbar_; }
	
  miutil::miTime getLatestPlotTime();

protected:
  void timerEvent(QTimerEvent*);

Q_SIGNALS:
  void timeSelected(const miutil::miTime& time);
  void lastStep();

public Q_SLOTS:
  /// add new times for datatype
  void insert(const std::string& datatype, const plottimes_t&, bool use);

  /// force new value
  void setTime(const miutil::miTime&);

  /// force new value if datatype match
  void setTimeForDataType(const std::string& datatype, const miutil::miTime&);

  void timecontrolslot();
  void timeoutChanged(float value);
  void animationForward();
  void animationBackward();
  void animationStop();
  void animationLoop();
  void stepTimeForward();
  void stepTimeBackward();
  void decreaseTimeStep();
  void increaseTimeStep();
  void timeSliderReleased();
  void updateTimeLabelFromSlider();

private:
  void createUi(QWidget* parent);
  void stepTime(int direction);
  void startAnimation(int direction);
  void updateTimeLabelFromTime(const miutil::miTime& t);
  void updateTimeLabelText();
  void updateTimeLabelStyle(bool found);

private:
  QToolBar* toolbar_;
  QAction* timeBackwardAction;
  QAction* timeForewardAction;
  QAction* timeStepBackwardAction;
  QAction* timeStepForewardAction;
  QAction* timeStopAction;
  QAction* timeLoopAction;
  QAction* timeControlAction;

  QShortcut* timeStepUpAction;
  QShortcut* timeStepDownAction;

  // timer types
  int animationDirection_;   ///> animation direction +1=forward, -1=backward, 0=no animation
  int animationTimer;        ///> the main timer id
  int timeout_ms;            ///> animation timeout in millisecs
  bool timeloop;             ///> animation in loop
  TimeSlider* tslider;       ///> the time slider
  TimeStepSpinbox* timestep; ///> the timestep widget
  TimeControl* timecontrol;  ///> time control dialog
  QLabel *timelabel;         ///> showing current time
};

#endif // TIMENAVIGATOR_H
