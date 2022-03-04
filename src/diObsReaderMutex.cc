/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "diObsReaderMutex.h"

namespace {

typedef std::unique_lock<std::mutex> scoped_lock;

} // namespace

// ------------------------------------------------------------------------

ObsReaderMutex::ObsReaderMutex(ObsReader_p wrapped)
    : wrapped_(wrapped)
{
}

ObsReaderMutex::~ObsReaderMutex() {}

bool ObsReaderMutex::configure(const std::string& key, const std::string& value)
{
  scoped_lock lock(mutex_);
  return wrapped_->configure(key, value);
}

bool ObsReaderMutex::checkForUpdates(bool useArchive)
{
  scoped_lock lock(mutex_);
  return wrapped_->checkForUpdates(useArchive);
}

std::set<miutil::miTime> ObsReaderMutex::getTimes(bool useArchive, bool update)
{
  scoped_lock lock(mutex_);
  return wrapped_->getTimes(useArchive, update);
}

std::vector<ObsDialogInfo::Par> ObsReaderMutex::getParameters()
{
  scoped_lock lock(mutex_);
  return wrapped_->getParameters();
}

void ObsReaderMutex::getData(ObsDataRequest_cp request, ObsDataResult_p result)
{
  scoped_lock lock(mutex_);
  return wrapped_->getData(request, result);
}

PlotCommand_cpv ObsReaderMutex::getExtraAnnotations()
{
  scoped_lock lock(mutex_);
  return wrapped_->getExtraAnnotations();
}
