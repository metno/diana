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

#ifndef DIANA_OBSSERVICE_H
#define DIANA_OBSSERVICE_H 1

#include "diObsReader.h"

#include "util/charsets.h"

#include <mutex>

#include "diana_obs_v0.pb.h" // only protobuf, not gRPC

class ObservationsService
{
private:
  struct ReaderUpdate
  {
    ObsReader_p reader;

    long updatedTimes;
    bool tUseArchive;

    long updatedParameters;

    long updatedData;
    miutil::miTime dObsTime;
    int dTimeDiff;
    int dLevel;
    bool dUseArchive;
  };

public:
  ObservationsService();

  void add(const std::string& name, ObsReader_p reader);

  void GetTimes(const diana_obs_v0::TimesRequest* request, diana_obs_v0::TimesResult* result);
  void GetParameters(const diana_obs_v0::ParametersRequest* request, diana_obs_v0::ParametersResult* result);
  void GetData(const ::diana_obs_v0::DataRequest* request, diana_obs_v0::DataResult* result);

private:
  ReaderUpdate& find(const std::string& name);

private:
  std::map<std::string, ReaderUpdate> readers_;
  diutil::CharsetConverter_p cvt;
  std::mutex mutex_;
};

typedef std::shared_ptr<ObservationsService> ObservationsService_p;

#endif // DIANA_OBSSERVICE_H
