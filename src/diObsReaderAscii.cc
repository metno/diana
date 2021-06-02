/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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

#include "diObsReaderAscii.h"

#include "diObsAscii.h"
#include "diUtilities.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#include <QTextCodec>

#define MILOGGER_CATEGORY "diana.ObsReaderAscii"
#include <miLogger/miLogging.h>

namespace {
const std::vector<std::string> EMPTY_HEADERINFO;
}

ObsReaderAscii::ObsReaderAscii()
{
  setTimeRange(-180, 180);
}

ObsReaderAscii::~ObsReaderAscii()
{
}

bool ObsReaderAscii::configure(const std::string& key, const std::string& value)
{
  if (key == "file" || key == "archivefile" || key == "ascii" || key == "archive_ascii") {
    addPattern(value, key == "archive_ascii" || key == "archivefile");
  } else if (key == "headerfile") {
    headerfile_ = value;
  } else if (key == "textcodec") {
    textcodec_ = QTextCodec::codecForName(value.c_str());
  } else {
    return ObsReaderFile::configure(key, value);
  }
  return true;
}

std::string ObsReaderAscii::decodeText(const std::string& text) const
{
  if (textcodec_)
    return textcodec_->toUnicode(text.c_str()).toStdString();
  else
    return text;
}

bool ObsReaderAscii::getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result)
{
  ObsAscii obsAscii(fi.filename, headerfile_, EMPTY_HEADERINFO);
#if 0
  oplot->setLabels(obsAscii.getLabels());
#endif
  std::vector<ObsData> obsdata = obsAscii.getObsData(fi.time, request->obstime, request->timeDiff);
  for (ObsData& obs : obsdata)
    obs.dataType = dataType();
  result->add(obsdata);
  return !obsAscii.hasError();
}

std::vector<ObsDialogInfo::Par> ObsReaderAscii::getParameters()
{
  if (parameters_.empty()) {
    if (!pattern.empty()) {
      const diutil::string_v matches = diutil::glob(pattern.front().pattern);
      if (!matches.empty()) {
        ObsAscii obsAscii = ObsAscii(matches.front(), headerfile_, EMPTY_HEADERINFO);
        for (const ObsAscii::Column& cn : obsAscii.getColumns())
          parameters_.push_back(ObsDialogInfo::Par(cn.name,cn.tooltip));
      }
    }
  }
  return parameters_;
}

PlotCommand_cpv ObsReaderAscii::getExtraAnnotations()
{
  if (!pattern.empty()) {
    const diutil::string_v matches = diutil::glob(pattern.front().pattern);
    if (!matches.empty()) {
      ObsAscii obsAscii = ObsAscii(matches.front(), headerfile_, EMPTY_HEADERINFO);
      return obsAscii.getLabels();
    }
  }
  return PlotCommand_cpv();
}
