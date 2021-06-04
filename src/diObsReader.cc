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

#include "diObsReader.h"

#include "util/diKeyValue.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsReader"
#include <miLogger/miLogging.h>

ObsDataRequest::ObsDataRequest()
    : timeDiff(0)
    , level(-2)
    , useArchive(false)
{
}

void ObsDataResult::add(const std::vector<ObsData>& data)
{
  diutil::insert_all(obsdata_, data);
}

ObsDataResult::ObsDataResult()
    : success_(false)
{
}

ObsDataResult::~ObsDataResult() {}

void ObsDataResult::setComplete(bool success)
{
  success_ = success;
}

ObsReader::ObsReader()
    : is_synoptic_(true)
{
}

ObsReader::~ObsReader()
{
}

bool ObsReader::configure(const std::string& key, const std::string& value)
{
  if (key == "synoptic") {
    setSynoptic(miutil::KeyValue(key, value).toBool());
  } else if (key == "datatype") {
    setDataType(value);
  } else {
    return false;
  }
  return true;
}

std::vector<ObsDialogInfo::Par> ObsReader::getParameters()
{
  return ObsDialogInfo::vparam();
}

PlotCommand_cpv ObsReader::getExtraAnnotations()
{
  return PlotCommand_cpv();
}
