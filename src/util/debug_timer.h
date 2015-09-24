#ifndef UTIL_DEBUG_TIMER_H
#define UTIL_DEBUG_TIMER_H 1

#include <sys/time.h>

namespace diutil {

class Timer {
public:
  Timer();
  ~Timer();
  void reset();
  void start();
  void stop();
  double elapsed() const
    { return mElapsed; }
  int count() const
    { return mCount; }

private:
  double mElapsed; //! elapsed time in s
  int mCount;

  bool mStarted;
  struct timeval mStartedAt;
};

class TimerSection {
public:
  TimerSection(Timer& timer) : mTimer(timer) { mTimer.start(); }
  ~TimerSection() { mTimer.stop(); }

private:
  TimerSection(const TimerSection&);
  TimerSection& operator=(const TimerSection&);

private:
  Timer& mTimer;
};

class TimerPrinter {
public:
  TimerPrinter(const char* name, Timer& timer);
  ~TimerPrinter();

private:
  TimerPrinter(const TimerPrinter&);
  TimerPrinter& operator=(const TimerPrinter&);

private:
  const char* mName;
  Timer& mTimer;
};

} // namespace diutil

#endif // UTIL_DEBUG_TIMER_H
