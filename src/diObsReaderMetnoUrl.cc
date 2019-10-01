#include "diObsReaderMetnoUrl.h"

#include "diObsAscii.h"
#include "util/time_util.h"

#include <QUrl>
#include <QUrlQuery>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReaderMetnoUrl"
#include <miLogger/miLogging.h>

namespace {
const std::string NO_HEADERFILE;
} // namespace

ObsReaderMetnoUrl::ObsReaderMetnoUrl()
{
}

bool ObsReaderMetnoUrl::configure(const std::string& key, const std::string& value)
{
  if (key == "url") {
    url_ = QUrl(QString::fromStdString(value));
  } else if (key == "headerinfo") {
    headerinfo_.push_back(value);
  } else {
    return ObsReaderTimeInterval::configure(key, value);
  }
  return true;
}

void ObsReaderMetnoUrl::getData(ObsDataRequest_cp request, ObsDataResult_p result)
{
  METLIBS_LOG_SCOPE();
  const int timediff_s = 60 * request->timeDiff;
  const miutil::miTime after = miutil::addSec(request->obstime, -timediff_s);
  const miutil::miTime until = miutil::addSec(request->obstime, +timediff_s);

  QUrl url = url_;
  QUrlQuery qurlq(url);
  qurlq.addQueryItem("Request", "GetData");
  qurlq.addQueryItem("TimeAfter", QString::fromStdString(after.isoTime("T")));
  qurlq.addQueryItem("TimeUntil", QString::fromStdString(until.isoTime("T")));
  url.setQuery(qurlq);

  ObsAscii obsAscii(std::string(url.toEncoded()), NO_HEADERFILE, headerinfo_);
  std::vector<ObsData> obsdata = obsAscii.getObsData(request->obstime, request->obstime, request->timeDiff);
  for (ObsData& obs : obsdata)
    obs.dataType = dataType();

#if 0
  oplot->setLabels(obsAscii.getLabels());
  oplot->columnName = obsAscii.getColumnNames();
#endif

  result->setTime(request->obstime);
  result->add(obsdata);
}

std::vector<ObsDialogInfo::Par> ObsReaderMetnoUrl::getParameters()
{
  METLIBS_LOG_SCOPE();
  std::vector<ObsDialogInfo::Par> parameters;

  QUrl url = url_;
  QUrlQuery qurlq(url);
  qurlq.addQueryItem("Request", "GetHeader");
  url.setQuery(qurlq);

  ObsAscii obsAscii(std::string(url.toEncoded()), NO_HEADERFILE, headerinfo_);
  for (const ObsAscii::Column& cn : obsAscii.getColumns())
    parameters.push_back(ObsDialogInfo::Par(cn.name,cn.tooltip));
  return parameters;
}
