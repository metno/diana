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

#ifndef DIOBSREADER_H
#define DIOBSREADER_H

#include "diObsData.h"

#include "diObsDialogInfo.h"
#include "diPlotCommand.h"

#include <puTools/miTime.h>

#include <memory>
#include <set>
#include <vector>

struct ObsDataRequest
{
  miutil::miTime obstime;
  int timeDiff; // units: minutes
  int level;
  bool useArchive;
  ObsDataRequest();
};

typedef std::shared_ptr<ObsDataRequest> ObsDataRequest_p;
typedef std::shared_ptr<const ObsDataRequest> ObsDataRequest_cp;

class ObsDataResult
{
public:
  ObsDataResult();
  virtual ~ObsDataResult();

  virtual void add(const std::vector<ObsData>& data);
  virtual void setComplete(bool success);

  bool success() const { return success_; }

  const std::vector<ObsData>& data() const { return obsdata_; }

  const miutil::miTime& time() const { return time_; }
  void setTime(const miutil::miTime& t) { time_ = t; }

private:
  miutil::miTime time_;
  std::vector<ObsData> obsdata_;
  bool success_;
};

typedef std::shared_ptr<ObsDataResult> ObsDataResult_p;
typedef std::shared_ptr<const ObsDataResult> ObsDataResult_cp;

class ObsReader
{
public:
  ObsReader();
  virtual ~ObsReader();

  virtual bool configure(const std::string& key, const std::string& value);

  void setSynoptic(bool synoptic) { is_synoptic_ = synoptic; }
  bool isSynoptic() const { return is_synoptic_; }

  void setDataType(const std::string& datatype) { datatype_ = datatype; }
  const std::string& dataType() const { return datatype_; }

  /*! @brief Check if updated data are available.
   * @param useArchive true to also search in the archive
   * @return true iff updates available
   */
  virtual bool checkForUpdates(bool useArchive) = 0;

  /*! @brief Get available times.
   * this is a kind of time grouping, i.e. not observations times
   * time t from this set may contain observation with times in t+timeRangeMin, t+timeRangeMax
   */
  virtual std::set<miutil::miTime> getTimes(bool useArchive, bool update) = 0;

  /**
   * @brief List parameters available in the dataset.
   *
   * FIXME: this should include description and parameter type
   * FIXME: should this be time-dependent?
   *
   * @return list of parameters
   */
  virtual std::vector<ObsDialogInfo::Par> getParameters();

  /**
   * @brief Get extra annotations defined by the data source.
   * The default implementation returns an empty list.
   * @return a list of extra annotations
   */
  virtual PlotCommand_cpv getExtraAnnotations();

  /**
   * @brief Read observation data.
   *
   * Only select observation data with time in [request->obstime +- request->timediff] unless
   * request->timediff < 0 which indicates that all observations should be read.
   */
  virtual void getData(ObsDataRequest_cp request, ObsDataResult_p result) = 0;

private:
  bool is_synoptic_;
  std::string datatype_;
};

typedef std::shared_ptr<ObsReader> ObsReader_p;

#endif // DIOBSREADER_H
