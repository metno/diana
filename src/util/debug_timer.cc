
#include "debug_timer.h"

#include <iomanip>

#define MILOGGER_CATEGORY "diana.util.DebugTimer"
#include <miLogger/miLogging.h>

namespace diutil {

Timer::Timer()
{
  reset();
}

Timer::~Timer()
{
  stop();
}

void Timer::reset()
{
  mStarted = false;
  mElapsed = 0;
  mCount = 0;
}

void Timer::start()
{
  if (!mStarted) {
    mStarted = true;
    gettimeofday(&mStartedAt, 0);
  }
}

void Timer::stop()
{
  if (!mStarted)
    return;

  struct timeval stoppedAt;
  gettimeofday(&stoppedAt, 0);

  const double M = 1e6;
  const double s = (((double)stoppedAt.tv_sec*M + (double)stoppedAt.tv_usec)
      -((double)mStartedAt.tv_sec*M + (double)mStartedAt.tv_usec))/M;

  mStarted = false;
  mElapsed += s;
  mCount += 1;
}

TimerPrinter::TimerPrinter(const char* name, Timer& timer)
  : mName(name)
  , mTimer(timer)
{
  mTimer.reset();
}

TimerPrinter::~TimerPrinter()
{
  METLIBS_LOG_DEBUG("spent " << std::setprecision(6) << std::setw(10) << mTimer.elapsed()
      << "s in timer '" << mName << "' for " << mTimer.count() << " calls");
}

} // namespace diutil
