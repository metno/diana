#ifndef TIMENAVIGATOR_H
#define TIMENAVIGATOR_H

#include <puTools/miTime.h>

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

  bool hasTimes() const;
  miutil::miTime selectedTime();
  void removeTimes(int id);
  void useData(const std::string& type, int id=-1);
  void setLastTimeStep();
  bool isTimerOn() const
    { return animationDirection_ != 0; }
  std::vector<miutil::miTime> animationTimes() const;

  QToolBar* toolbar()
    { return toolbar_; }

protected:
  void timerEvent(QTimerEvent*);

Q_SIGNALS:
  void timeSelected(const miutil::miTime& time);

public Q_SLOTS:
  ///add new times for datatype
  void insert(const std::string& datatype, const std::vector<miutil::miTime>&,bool =true);

  /// force new value
  void setTime(const miutil::miTime&);

  /// force new value if datatype match
  void setTime( const std::string& datatype, const miutil::miTime&);

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
  void updateTimeLabel();

private:
  void createUi(QWidget* parent);
  void stepTime(int direction);
  void startAnimation(int direction);

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
