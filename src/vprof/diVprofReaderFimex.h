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
#ifndef VPROFREADERFIMEX_H
#define VPROFREADERFIMEX_H

#include "diVprofReader.h"

#include "vcross_v2/VcrossCollector.h"

struct VprofDataFimex : public VprofData
{
  VprofDataFimex(const std::string& modelname, const std::string& stationsfilename = "");
  VprofValues_cpv getValues(const VprofValuesRequest& request) override;
  virtual bool updateStationList(const miutil::miTime& plotTime) override;

  static VprofData_p createData(vcross::Setup_p setup, const VprofSelectedModel& vsm, const std::string& stationsfilename);

private:
  VprofValues_cp readValues(const VprofValuesRequest& req);

private:
  vcross::Collector_p collector;
  vcross::Time reftime;

  std::vector<int> forecastHour;
};

struct VprofReaderFimex : public VprofReader
{
  vcross::Setup_p setup;
  std::vector<std::string> getReferencetimes(const std::string& modelName) override;
  VprofData_p find(const VprofSelectedModel& vsm, const std::string& stationsfilename) override;

private:
  bool readFimex(const std::string& reftimestr);
};

typedef std::shared_ptr<VprofDataFimex> VprofDataFimex_p;
typedef std::shared_ptr<VprofReaderFimex> VprofReaderFimex_p;

#endif
