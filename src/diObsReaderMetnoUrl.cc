/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
  auto obsdata = obsAscii.getObsData(request->obstime, request->obstime, request->timeDiff);
  for (size_t i = 0; i < obsdata->size(); ++i)
    obsdata->basic(i).dataType = dataType();

#if 0
  oplot->setLabels(obsAscii.getLabels());
  oplot->columnName = obsAscii.getColumnNames();
#endif

  result->setTime(request->obstime);
  result->add(obsdata);
  result->setComplete(!obsAscii.hasError());
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
