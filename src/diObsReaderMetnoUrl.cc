#include "diObsReaderMetnoUrl.h"

#include "diObsAscii.h"
#include "diObsMetaData.h"
#include "util/misc_util.h"
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReaderMetnoUrl"
#include <miLogger/miLogging.h>

namespace {
const std::string NO_HEADERFILE;
}
ObsReaderMetnoUrl::ObsReaderMetnoUrl()
{
}

bool ObsReaderMetnoUrl::configure(const std::string& key, const std::string& value)
{
  if (key == "url") {
    url_ = value;
  } else if (key == "timeinfo") {
    std::string timeinfo = value;
    miutil::remove(timeinfo, '\"');
    miutil::miTime from = miutil::miTime::nowTime(), to = from;
    int interval = 3;
    for (const std::string& tok : miutil::split(timeinfo, ";")) {
      const std::vector<std::string> stokens = miutil::split(tok, "=");
      if (stokens.size() != 2)
        continue;
      if (stokens[0] == "from") {
        from = miutil::miTime(stokens[1]);
      } else if (stokens[0] == "to") {
        to = miutil::miTime(stokens[1]);
      } else if (stokens[0] == "interval") {
        interval = miutil::to_int(stokens[1]);
      }
    }
    // FIXME this does not allow isUpdated() to work!
    while (from < to) {
      timeset_.insert(from);
      from.addHour(interval);
    }
  } else if (key == "headerinfo") {
    headerinfo_.push_back(value);
  } else if (key == "metadata") {
    METLIBS_LOG_WARN("metadata is not supported any more, please use 'metadata_url'");
    return false;
  } else if (key == "metadata_url") {
    metadata_url_ = value;
  } else {
    return ObsReader::configure(key, value);
  }
  return true;
}

std::set<miutil::miTime> ObsReaderMetnoUrl::getTimes(bool, bool)
{
  return timeset_;
}

bool ObsReaderMetnoUrl::checkForUpdates(bool)
{
  return false;
}

bool ObsReaderMetnoUrl::addStationsAndTimeFromMetaData(std::string& url, const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();
  if (!metadata_values_) {
    if (metadata_url_.empty())
      return false;

    ObsAscii obsAscii(metadata_url_, NO_HEADERFILE, headerinfo_);
    metadata_values_ = std::make_shared<ObsMetaData>();
    metadata_values_->setObsData(obsAscii.getObsData(miutil::miTime(), miutil::miTime(), 0));
  }

  // add stations
  metadata_values_->addStationsToUrl(url);

  // add time
  miutil::replace(url, "TIME", time.format("fd=%d.%m.%Y&td=%d.%m.%Y&h=%H", "", true));
  miutil::replace(url, "MONTH", "m=" + miutil::from_number(time.month() - 1));

  return true;
}

void ObsReaderMetnoUrl::getData(ObsDataRequest_cp request, ObsDataResult_p result)
{
  std::string url = url_;
  if (!addStationsAndTimeFromMetaData(url, request->obstime)) {
    return;
  }

  ObsAscii obsAscii(url, NO_HEADERFILE, headerinfo_);
  std::vector<ObsData> obsdata = obsAscii.getObsData(request->obstime, request->obstime, request->timeDiff);
  for (ObsData& obs : obsdata)
    obs.dataType = dataType();

#if 0
  oplot->setLabels(obsAscii.getLabels());
  oplot->columnName = obsAscii.getColumnNames();
#endif

  if (metadata_values_) {
    const ObsMetaData::string_ObsData_m& metad = metadata_values_->getMetaData();
    for (ObsData& obs : obsdata) {
      const ObsMetaData::string_ObsData_m::const_iterator itM = metad.find(obs.id);
      if (itM != metad.end()) {
        obs.xpos = itM->second.xpos;
        obs.ypos = itM->second.ypos;
      }
    }
  }
  result->add(obsdata);
}

std::vector<ObsDialogInfo::Par> ObsReaderMetnoUrl::getParameters()
{
  std::vector<ObsDialogInfo::Par> parameters;
  ObsAscii obsAscii(url_, NO_HEADERFILE, headerinfo_);
  for (const std::string& cn : obsAscii.getColumnNames())
    parameters.push_back(ObsDialogInfo::Par(cn));
  return parameters;
}
