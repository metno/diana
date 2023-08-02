/* -*- c++ -*-
 * GridCollection.h
 *
 *  Created on: Mar 15, 2010
 *      Author: audunc
 */
/*
 Copyright (C) 2013-2020 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GRIDCOLLECTION_H_
#define GRIDCOLLECTION_H_

#include "GridInventoryTypes.h"
#include "VcrossData.h"
#include "diCommonFieldTypes.h"
#include "diGridConverter.h"
#include "diField/diFieldFwd.h"

#include <puTools/miTime.h>

#include <string>
#include <vector>

class GridIOBase;
class GridIOsetup;

class GridCollection {
public:
  GridCollection();
  virtual ~GridCollection();

  /**
   * Initialize collection from a list of grid sources (all same source type)
   * @param type, GridIO subclass name
   * @param name, collection name
   * @param filenames, list of sources
   * @param options, extra options
   * @param setup, setup-information
   * @return status
   */
  bool setContents(const std::string& type, const std::string& name,
      const std::vector<std::string>& filenames,
      const std::vector<std::string>& format,
      const std::vector<std::string>& config,
      const std::vector<std::string>& option,
      GridIOsetup* setup,
      bool validTimeFromFilename=false);

  /**
   * make the inventory
   * @param reftime, reference time to make inventory for. Use empty string for none
   * @return status
   */
  bool makeInventory(const std::string& refTime);

  /**
   * Check inventory
   * @return true if inventory is ok,  or if refTimes are ok when only refTimes are asked for
   */
  bool inventoryOk(const std::string& refTime)
  {
    return ((refTime.empty() && inventoryOK.size()) || (inventoryOK.count(refTime) && inventoryOK[refTime]));
  }


  /**
   * Get the reference times for a specific model
   * @return set<std::string>
   */
  std::set<std::string> getReferenceTimes() const;

  /**
   * Get the Grid for a specific model
   * @return gridinventory::Grid
   */
  gridinventory::Grid getGrids() const;

  diutil::Values_p getVariable(const std::string& reftime, const std::string& paramname);

  std::set<miutil::miTime> getTimes(const std::string& reftime, const std::string& paramname);

  bool putData(const std::string& reftime, const std::string& paramname, const std::string& level, const miutil::miTime& time, const std::string& run,
               const std::string& unit, const std::string& output_time, const Field_cp field);

  /**
   * Updates the information held about the data sources.
   */
  bool updateSources();

  bool standardname2variablename(const std::string& reftime, const std::string& standard_name, std::string& variable_name);

  std::map<std::string, std::string> getGlobalAttributes(const std::string& refTime);

  const std::map<std::string, FieldPlotInfo>& getFieldPlotInfo(const std::string& refTime);

  Field_p getField(const FieldRequest& fieldrequest);

private:
  static GridConverter gc;

  std::set<miutil::miTime> timesFromFilename;
  /// name of collection
  std::string collectionname;
  /// source type
  std::string sourcetype;
  /// options to GridIO
  std::vector<std::string> formats;
  std::vector<std::string> configs;
  std::vector<std::string> options;
  /// unpacked sources
  std::vector<std::string> rawsources;
  std::vector<std::string> sources_with_wildcards;
  std::set<std::string> sources;
  /// use time from filename
  bool timeFromFilename;
  /// the setup class
  GridIOsetup* gridsetup;
  /// inventory made, and ok
  std::map<std::string, bool> inventoryOK;
  /// No complete inventory, only reftimes from filename
  std::set<std::string> refTimes;
  /// the combined inventory
  gridinventory::Inventory inventory;
  /// the combined inventory with computed parameters
  gridinventory::ReftimeInventory computed_inventory;
  /// plot information for each reference time
  std::map<std::string, std::map<std::string, FieldPlotInfo>> reftime_fieldplotinfo_;
  /// the actual data-containers - list of GridIO objects
  typedef std::vector<GridIOBase*> gridsources_t;
  gridsources_t gridsources;
  std::map<miutil::miTime, GridIOBase*> gridsourcesTimeMap;

  bool useTimeFromFilename() const { return timeFromFilename; }

  const std::set<miutil::miTime>& getTimesFromFilename() const { return timesFromFilename; }

  /**
   * Check if data exists
   * @param reference time
   * @param parameter name
   * @param grid specification
   * @param vertical coordinate
   * @param time axis
   * @param extraaxis
   * @param version
   * @param level
   * @param time
   * @param elevel
   * @param time_tolerance in minutes
   * @param return GridParameter param if data found
   * @param return actualtime, may differ from time if time_tolerance != 0
   * @return true/false
   */
  const gridinventory::GridParameter* dataExists(const std::string& reftime, const std::string& paramname);

  /**
   * Get data slice
   * @param reftime
   * @param paramname
   * @param grid
   * @param zaxis
   * @param taxis
   * @param extraaxis
   * @param version
   * @param level
   * @param time
   * @param elevel
   * @param time_tolerance in minutes
   * @param return actualtime, may differ from time if time_tolerance != 0
   * @return field pointer
   */
  Field_p getData(const std::string& reftime, const std::string& paramname, const std::string& zaxis, const std::string& taxis, const std::string& extraaxis,
                  const std::string& level, const miutil::miTime& time, const std::string& elevel, const int& time_tolerance);

  std::map<std::string, FieldPlotInfo> buildFieldPlotInfo(const std::string& refTime);

  /**
   * check if any sources has changed
   * @return status
   */
  bool sourcesChanged();

  /// unpack the raw sources and make one or more GridIO instances
  bool makeGridIOinstances();
  /// clear the gridsources vector
  void clearGridSources();

  std::set<miutil::miTime> getTimesFromIO(const std::string& reftime, const std::string& paramname);
  std::set<miutil::miTime> getTimesFromCompute(const std::string& reftime, const std::string& paramname);

  //! find nearest time within time tolerance
  /*!
   * \param time time to search for
   * \param time_tolerance tolerance in minutes
   * \param actualtime found time, changed only if returning true
   * \return true if time was found within time_tolerance
   */
  bool getActualTime(const std::string& reftime, const std::string& paramname, const miutil::miTime& time, int time_tolerance, miutil::miTime& actualtime);

  const gridinventory::GridParameter* dataExists_reftime(const gridinventory::ReftimeInventory& reftimInv, const std::string& paramname);
  void addComputedParameters();
  bool getAllFields_timeInterval(Field_pv& vfield, FieldRequest fieldrequest, int fch, bool accumulate_flux);
  bool getAllFields(Field_pv& vfield, FieldRequest fieldrequest, const std::vector<float>& constants);
  static bool multiplyFieldByTimeStep(Field_p f, float sec_diff);
};

#endif /* GRIDCOLLECTION_H_ */
