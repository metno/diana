#ifndef DIOBSREADERMETNOURL_H
#define DIOBSREADERMETNOURL_H

#include "diObsReader.h"

class ObsMetaData;
typedef std::shared_ptr<ObsMetaData> ObsMetaData_p;

class ObsReaderMetnoUrl : public ObsReader
{
public:
  ObsReaderMetnoUrl();

  bool configure(const std::string& key, const std::string& value) override;

  std::set<miutil::miTime> getTimes(bool useArchive, bool update) override;
  void getData(ObsDataRequest_cp request, ObsDataResult_p result) override;
  std::vector<ObsDialogInfo::Par> getParameters() override;
  bool checkForUpdates(bool useArchive) override;

private:
  bool addStationsAndTimeFromMetaData(std::string& url, const miutil::miTime& time);

private:
  std::string url_;
  std::vector<std::string> headerinfo_;
  std::string metadata_url_;

  std::set<miutil::miTime> timeset_; // derived from configure 'timeinfo'
  ObsMetaData_p metadata_values_;
};

#endif // DIOBSREADERMETNOURL_H
