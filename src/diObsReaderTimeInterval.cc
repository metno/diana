#include "diObsReaderTimeInterval.h"

#include "util/misc_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReaderTimeInterval"
#include <miLogger/miLogging.h>

namespace {
inline const miutil::miTime& nowIfundef(const miutil::miTime& time, const miutil::miTime& now)
{
  return time.undef() ? now : time;
}

miutil::miTime nowYMDH()
{
  miutil::miTime n = miutil::miTime::nowTime();
  n.addSec(-n.sec());
  n.addMin(-n.min());
  return n;
}
} // namespace

// ########################################################################

ObsReaderTimeInterval::ObsReaderTimeInterval()
    : time_from_offset_(0)
    , time_to_offset_(0)
    , time_interval_(3 * 60 * 60) // 3 hours, in seconds
{
}

bool ObsReaderTimeInterval::configure(const std::string& key, const std::string& value)
{
  if (key == "time_from") {
    time_from_ = miutil::miTime(value);
  } else if (key == "time_to") {
    time_to_ = miutil::miTime(value);
  } else if (key == "time_from_offset") {
    time_from_offset_ = miutil::to_int(value);
  } else if (key == "time_to_offset") {
    time_to_offset_ = miutil::to_int(value);
  } else if (key == "time_interval") {
    time_interval_ = std::max(1, miutil::to_int(value)); // at least 1s
  } else {
    return ObsReader::configure(key, value);
  }
  return true;
}

const std::set<miutil::miTime>& ObsReaderTimeInterval::calculateTimes(const miutil::miTime& now)
{
  timeset_.clear();

  miutil::miTime from = addSec(nowIfundef(time_from_, now), time_from_offset_ - time_interval_ / 2);
  const miutil::miTime to = addSec(nowIfundef(time_to_, now), time_to_offset_ + time_interval_ / 2);
  while (from <= to) {
    timeset_.insert(from);
    from.addSec(time_interval_);
  }

  return timeset_;
}

bool ObsReaderTimeInterval::checkTimes(const miutil::miTime& now)
{
  if (!(time_from_.undef() || time_to_.undef()))
    return false;

  const miutil::miTime to = addSec(nowIfundef(time_to_, now), time_to_offset_ - time_interval_ / 2);
  if (timeset_.find(to) == timeset_.end())
    return true;

  const miutil::miTime from = addSec(nowIfundef(time_from_, now), time_from_offset_ + time_interval_ / 2);
  if (timeset_.lower_bound(from) != timeset_.begin())
    return true;

  return false;
}

std::set<miutil::miTime> ObsReaderTimeInterval::getTimes(bool, bool update)
{
  const miutil::miTime now = nowYMDH();
  if (update || checkTimes(now))
    calculateTimes(now);
  return timeset_;
}

bool ObsReaderTimeInterval::checkForUpdates(bool)
{
  return checkTimes(nowYMDH());
}
