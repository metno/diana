#ifndef DIOBSREADERTIMEINTERVAL_H
#define DIOBSREADERTIMEINTERVAL_H

#include "diObsReader.h"

class ObsReaderTimeInterval : public ObsReader
{
public:
  ObsReaderTimeInterval();

  bool configure(const std::string& key, const std::string& value) override;

  std::set<miutil::miTime> getTimes(bool useArchive, bool update) override;
  bool checkForUpdates(bool useArchive) override;

public: // exposed for unit tests
  bool checkTimes(const miutil::miTime& now);
  const std::set<miutil::miTime>& calculateTimes(const miutil::miTime& now);

protected:
  int time_interval() const { return time_interval_; }

private:
  miutil::miTime time_from_;
  miutil::miTime time_to_;
  int time_from_offset_;             //! units=seconds
  int time_to_offset_;               //! units=seconds
  int time_interval_;                //! units=seconds
  std::set<miutil::miTime> timeset_; //! derived from configure 'timeinfo'
};

#endif // DIOBSREADERTIMEINTERVAL_H
