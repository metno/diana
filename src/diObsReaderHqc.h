#ifndef DIOBSREADERHQC_H
#define DIOBSREADERHQC_H

#include "diObsReader.h"

class ObsReaderHqc : public ObsReader
{
public:
  ObsReaderHqc();

  std::vector<ObsDialogInfo::Par> getParameters() override;

private:
  std::vector<ObsData> hqcdata;
  std::vector<std::string> hqc_synop_parameter;
  miutil::miTime hqcTime;
  std::string hqcFlag;
  std::string hqcFlag_old;
  int hqc_from;
  std::string selectedStation;

  bool timeListChanged;
};

#endif // DIOBSREADERHQC_H
