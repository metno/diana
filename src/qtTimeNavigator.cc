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

#include "qtTimeNavigator.h"

#include "qtTimeSlider.h"
#include "qtTimeControl.h"
#include "qtTimeStepSpinbox.h"

#include <QAction>
#include <QLabel>
#include <QShortcut>
#include <QTimerEvent>
#include <QToolBar>

#include <bakover.xpm>
#include <clock.xpm>
#include <forover.xpm>
#include <loop.xpm>
#include <start.xpm>
#include <stopp.xpm>
#include <slutt.xpm>

namespace {
const QString styleTimeFound("QLabel { color : black; }");
const QString styleTimeMissing("QLabel { color : red; }");
} // namespace

TimeNavigator::TimeNavigator(QWidget *parent)
  : QObject(parent)
  , animationDirection_(0)
  , timeout_ms(100)
  , timeloop(false)
{
  createUi(parent);
}

void TimeNavigator::createUi(QWidget* parent)
{
  // timecommands ======================
  // --------------------------------------------------------------------
  timeBackwardAction = new QAction(QIcon( QPixmap(start_xpm )),tr("Run Backwards"), this);
  timeBackwardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeBackwardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Left);
  timeBackwardAction->setCheckable(true);
  timeBackwardAction->setIconVisibleInMenu(true);
  connect( timeBackwardAction, SIGNAL( triggered() ) ,  SLOT( animationBackward() ) );
  // --------------------------------------------------------------------
  timeForewardAction = new QAction(QIcon( QPixmap(slutt_xpm )),tr("Run Forewards"), this);
  timeForewardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeForewardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Right);
  timeForewardAction->setCheckable(true);
  timeForewardAction->setIconVisibleInMenu(true);
  connect( timeForewardAction, SIGNAL( triggered() ) ,  SLOT( animationForward() ) );
  // --------------------------------------------------------------------
  timeStepBackwardAction = new QAction(QIcon( QPixmap(bakover_xpm )),tr("Step Backwards"), this);
  timeStepBackwardAction->setShortcut(Qt::CTRL+Qt::Key_Left);
  timeStepBackwardAction->setCheckable(false);
  timeStepBackwardAction->setIconVisibleInMenu(true);
  connect( timeStepBackwardAction, SIGNAL( triggered() ) ,  SLOT( stepTimeBackward() ) );
  // --------------------------------------------------------------------
  timeStepForewardAction = new QAction(QIcon( QPixmap(forward_xpm )),tr("Step Forewards"), this);
  timeStepForewardAction->setShortcut(Qt::CTRL+Qt::Key_Right);
  timeStepForewardAction->setCheckable(false);
  timeStepForewardAction->setIconVisibleInMenu(true);
  connect( timeStepForewardAction, SIGNAL( triggered() ) ,  SLOT( stepTimeForward() ) );
  // --------------------------------------------------------------------
  timeStopAction = new QAction(QIcon( QPixmap(stop_xpm )),tr("Stop"), this );
  timeStopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeStopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Down);
  timeStopAction->setCheckable(false);
  timeStopAction->setIconVisibleInMenu(true);
  connect( timeStopAction, SIGNAL( triggered() ) ,  SLOT( animationStop() ) );
  // --------------------------------------------------------------------
  timeLoopAction = new QAction(QIcon( QPixmap(loop_xpm )),tr("Run in loop"), this );
  timeLoopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeLoopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Up);
  timeLoopAction->setCheckable(true);
  timeLoopAction->setIconVisibleInMenu(true);
  connect( timeLoopAction, SIGNAL( triggered() ) ,  SLOT( animationLoop() ) );
  // --------------------------------------------------------------------
  timeControlAction = new QAction(QIcon( QPixmap( clock_xpm )),tr("Time control"), this );
  timeControlAction->setCheckable(true);
  timeControlAction->setIconVisibleInMenu(true);
  connect( timeControlAction, SIGNAL( triggered() ) ,  SLOT( timecontrolslot() ) );
  // --------------------------------------------------------------------

  //Time step up/down
  timeStepUpAction = new QShortcut(Qt::CTRL+Qt::Key_Up, parent);
  connect(timeStepUpAction, SIGNAL(activated()), SLOT(increaseTimeStep()));
  timeStepDownAction = new QShortcut(Qt::CTRL+Qt::Key_Down, parent);
  connect(timeStepDownAction, SIGNAL(activated()), SLOT(decreaseTimeStep()));

  // ----------------Timer widgets -------------------------

  tslider= new TimeSlider(Qt::Horizontal, parent);
  tslider->setMinimumWidth(90);
  //tslider->setMaximumWidth(90);
  connect(tslider, &TimeSlider::valueChanged, this, &TimeNavigator::updateTimeLabelFromSlider);
  connect(tslider, &TimeSlider::sliderReleased, this, &TimeNavigator::timeSliderReleased);
  connect(tslider, &TimeSlider::sliderSet, this, &TimeNavigator::updateTimeLabelFromSlider);
  connect(tslider, &TimeSlider::lastStep, this, &TimeNavigator::lastStep);

  timestep= new TimeStepSpinbox(parent);
  connect(tslider,SIGNAL(minInterval(int)),
      timestep,SLOT(setValue(int)));
  connect(tslider,SIGNAL(timeSteps(int,int)),
      timestep,SLOT(setTimeSteps(int,int)));
  connect(tslider,SIGNAL(enableSpin(bool)),
      timestep,SLOT(setEnabled(bool)));
  connect(timestep,SIGNAL(valueChanged(int)),
      tslider,SLOT(setInterval(int)));


  timelabel= new QLabel("0000-00-00 00:00:00", parent);
  timelabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  timelabel->setMinimumSize(timelabel->sizeHint());

  timecontrol = new TimeControl(parent);
  connect(timecontrol, SIGNAL(timeoutChanged(float)),
      SLOT(timeoutChanged(float)));
  connect(timecontrol, SIGNAL(minmaxValue(const miutil::miTime&, const miutil::miTime&)),
      tslider, SLOT(setMinMax(const miutil::miTime&, const miutil::miTime&)));
  connect(timecontrol, SIGNAL(clearMinMax()),
      tslider, SLOT(clearMinMax()));
  connect(tslider, &TimeSlider::newTimes, timecontrol, &TimeControl::setTimes);
  connect(timecontrol, SIGNAL(data(std::string)),
      tslider, SLOT(useData(std::string)));
  connect(timecontrol, SIGNAL(timecontrolHide()),
      SLOT(timecontrolslot()));

  toolbar_ = new QToolBar("TimerToolBar", parent);
  toolbar_->setObjectName("TimerToolBar");

  toolbar_->addAction(timeBackwardAction);
  toolbar_->addAction(timeStepBackwardAction);
  toolbar_->addAction(timeStepForewardAction);
  toolbar_->addAction(timeForewardAction);
  toolbar_->addAction(timeStopAction);
  toolbar_->addAction(timeLoopAction);
  toolbar_->addWidget(tslider);
  toolbar_->addAction(timeControlAction);
  toolbar_->addWidget(timestep);
  toolbar_->addWidget(timelabel);

  updateTimeLabelFromTime(selectedTime());
}

bool TimeNavigator::hasTimes() const
{
  return tslider->numTimes() != 0;
}

miutil::miTime TimeNavigator::selectedTime()
{
  return tslider->Value();
}

void TimeNavigator::removeTimes(int id)
{
  const std::vector<std::string> type = timecontrol->deleteType(id);
  for(unsigned int i=0;i<type.size();i++)
    tslider->deleteType(type[i]);
}

void TimeNavigator::useData(const std::string& type, int id)
{
  timecontrol->useData(type, id);
}

std::vector<miutil::miTime> TimeNavigator::animationTimes() const
{
  const miutil::miTime current = tslider->Value();

  std::vector<miutil::miTime> times;

  miutil::miTime t = tslider->getStartTime();
  tslider->setTime(t);
  times.push_back(t);

  while (tslider->nextTime(1, t)) {
    tslider->setTime(t);
    times.push_back(t);
  }

  tslider->setTime(current);

  return times;
}

void TimeNavigator::timerEvent(QTimerEvent *e)
{
  if (e->timerId() == animationTimer) {
    miutil::miTime t;
    if (tslider->nextTime(animationDirection_, t)) {
      setTime(t);
    } else {
      animationStop();
    }
  }
}

void TimeNavigator::insertAndUse(const std::string& datatype, const plottimes_t& times)
{
  insert(datatype, times, true);
}

void TimeNavigator::insert(const std::string& datatype, const plottimes_t& vt, bool use)
{
  tslider->insert(datatype, vt, use);
}

void TimeNavigator::requestTime(const miutil::miTime& t)
{
  updateTimeLabelFromTime(t);
  if (t != selectedTime())
    setTime(t);
}

void TimeNavigator::setTime(const miutil::miTime& t)
{
  updateTimeLabelFromTime(t);
  tslider->setTime(t);
  Q_EMIT timeSelected(t);
}

void TimeNavigator::setTimeForDataType(const std::string& datatype, const miutil::miTime& t)
{
  if (tslider->setTimeForDataType(datatype, t)) {
    updateTimeLabelFromTime(t);
    Q_EMIT timeSelected(t);
  }
}

void TimeNavigator::timeSliderReleased()
{
  setTime(selectedTime());
}

void TimeNavigator::updateTimeLabelFromTime(const miutil::miTime& t)
{
  updateTimeLabelText();
  updateTimeLabelStyle(tslider->hasTime(t));
}

void TimeNavigator::updateTimeLabelFromSlider()
{
  updateTimeLabelText();
  updateTimeLabelStyle(true);
}

void TimeNavigator::updateTimeLabelStyle(bool found)
{
  timelabel->setStyleSheet(found ? styleTimeFound : styleTimeMissing);
}

void TimeNavigator::updateTimeLabelText()
{
  const miutil::miTime t = selectedTime();
  timelabel->setText(QString::fromStdString(t.isoTime()));
}

void TimeNavigator::animationLoop()
{
  timeloop= !timeloop;
  tslider->setLoop(timeloop);

  timeLoopAction->setChecked(timeloop);
}

void TimeNavigator::animationForward()
{
  startAnimation(+1);
  // after start, because animationStop might uncheck
  timeForewardAction->setChecked(true);
}

void TimeNavigator::animationBackward()
{
  startAnimation(-1);
  // after start, because animationStop might uncheck
  timeBackwardAction->setChecked(true);
}

void TimeNavigator::startAnimation(int direction)
{
  if (animationDirection_ != 0)
    animationStop();

  tslider->startAnimation();
  animationTimer= startTimer(timeout_ms);
  animationDirection_ = direction;
}

void TimeNavigator::animationStop()
{
  timeBackwardAction->setChecked( false );
  timeForewardAction->setChecked( false );

  killTimer(animationTimer);
  animationDirection_=0;
}

void TimeNavigator::stepTime(int direction)
{
  if (animationDirection_)
    return;
  miutil::miTime t;
  if (tslider->nextTime(direction > 0 ? 1 : -1, t)) {
    setTime(t);
  }
}

void TimeNavigator::stepTimeForward()
{
  stepTime(+1);
}

void TimeNavigator::stepTimeBackward()
{
  stepTime(-1);
}

void TimeNavigator::decreaseTimeStep()
{
  int v= timestep->value() - timestep->singleStep();
  if (v<0)
    v=0;
  timestep->setValue(v);
}

void TimeNavigator::increaseTimeStep()
{
  int v= timestep->value() + timestep->singleStep();
  timestep->setValue(v);
}

void TimeNavigator::timeoutChanged(float value)
{
  int msecvalue= static_cast<int>(value*1000);
  if (msecvalue != timeout_ms) {
    timeout_ms= msecvalue;

    if (animationDirection_!=0) {
      killTimer(animationTimer);
      animationTimer= startTimer(timeout_ms);
    }
  }
}

void TimeNavigator::timecontrolslot()
{
  bool b = timecontrol->isVisible();
  timecontrol->setVisible(!b);
  timeControlAction->setChecked(!b);
}
