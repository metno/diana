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

#ifndef DIObsReaderMutex_H
#define DIObsReaderMutex_H

#include "diObsReader.h"

#include <mutex>

class ObsReaderMutex : public ObsReader
{
public:
  ObsReaderMutex(ObsReader_p wrapped);
  ~ObsReaderMutex();

  bool configure(const std::string& key, const std::string& value) override;

  bool checkForUpdates(bool useArchive) override;

  std::set<miutil::miTime> getTimes(bool useArchive, bool update) override;

  std::vector<ObsDialogInfo::Par> getParameters() override;

  PlotCommand_cpv getExtraAnnotations() override;

  void getData(ObsDataRequest_cp request, ObsDataResult_p result) override;

private:
  ObsReader_p wrapped_;
  std::mutex mutex_;
};

#endif // DIObsReaderMutex_H
