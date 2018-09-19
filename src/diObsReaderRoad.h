#ifndef DIOBSREADERROAD_H
#define DIOBSREADERROAD_H

#include "diObsReaderFile.h"

class ObsReaderRoad : public ObsReaderFile
{
public:
  ObsReaderRoad();

  bool configure(const std::string& key, const std::string& value) override;

protected:
  void getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result) override;

private:
  std::string headerfile;
  std::string stationfile;
  std::string databasefile;
  int daysback;
};

#endif // DIOBSREADERROAD_H
