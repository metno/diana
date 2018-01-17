#ifndef DIOBSREADERASCII_H
#define DIOBSREADERASCII_H

#include "diObsReaderFile.h"

#include <vector>

class QTextCodec;

class ObsReaderAscii : public ObsReaderFile
{
public:
  ObsReaderAscii();
  ~ObsReaderAscii();

  bool configure(const std::string& key, const std::string& value) override;
  std::vector<ObsDialogInfo::Par> getParameters() override;
  PlotCommand_cpv getExtraAnnotations() override;

protected:
  void getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result) override;

private:
  std::string decodeText(const std::string& text) const;

private:
  std::string headerfile_;
  QTextCodec* textcodec_;

  std::vector<ObsDialogInfo::Par> parameters_;
};

#endif // DIOBSREADERASCII_H
