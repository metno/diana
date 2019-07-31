#ifndef DIOBSREADERMETNOURL_H
#define DIOBSREADERMETNOURL_H

#include "diObsReaderTimeInterval.h"

#include <QUrl>

class ObsReaderMetnoUrl : public ObsReaderTimeInterval
{
public:
  ObsReaderMetnoUrl();

  bool configure(const std::string& key, const std::string& value) override;

  void getData(ObsDataRequest_cp request, ObsDataResult_p result) override;
  std::vector<ObsDialogInfo::Par> getParameters() override;

private:
  QUrl url_;
  std::vector<std::string> headerinfo_;
  std::string metadata_url_;
};

#endif // DIOBSREADERMETNOURL_H
