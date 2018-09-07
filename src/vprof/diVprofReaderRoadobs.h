/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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
#ifndef VPROFREADERROADOBS_H
#define VPROFREADERROADOBS_H

#include "diVprofReader.h"

struct VprofDataRoadobs : public VprofData
{
  VprofDataRoadobs(const std::string& modelname, const std::string& stationsfilename = "");
  VprofValues_cpv getValues(const VprofValuesRequest& request) override;
  virtual bool updateStationList(const miutil::miTime& plotTime) override;

  bool setRoadObs(const miutil::miTime& plotTime);

  std::string db_parameterfile;
  std::string db_connectfile;
};

struct VprofReaderRoadobs : public VprofReader
{
  std::map<std::string, std::string> db_parameters;
  std::map<std::string, std::string> db_connects;
  VprofData_p find(const VprofSelectedModel& vsm, const std::string& stationsfilename) override;

private:
  bool readRoadObs(const std::string& databasefile, const std::string& parameterfile);
};

typedef std::shared_ptr<VprofDataRoadobs> VprofDataRoadobs_p;
typedef std::shared_ptr<VprofReaderRoadobs> VprofReaderRoadobs_p;

#endif
