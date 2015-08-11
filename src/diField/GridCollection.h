/* -*- c++ -*-
 * GridCollection.h
 *
 *  Created on: Mar 15, 2010
 *      Author: audunc
 */
/*
 Copyright (C) 2013 met.no

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
#include "GridIO.h"
#include <puTools/miTime.h>
#include <boost/shared_array.hpp>
#include <string>
#include <vector>

class GridIOsetup;
class Field;

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
  bool makeInventory(const std::string& refTime, bool updateSourceList=false);

  /**
   * check if any sources has changed
   * @return status
   */
  bool sourcesChanged();

  /**
   * Check inventory
   * @return true if inventory is ok,  or if refTimes are ok when only refTimes are asked for
   */
  bool inventoryOk(const std::string& refTime)
  {
    return ((refTime.empty() && inventoryOK.size()) || (inventoryOK.count(refTime) && inventoryOK[refTime]));
  }

  /**
   * Get the combined inventory
   * @return the collection inventory
   */
  const gridinventory::Inventory & getInventory() const
  {
    return inventory;
  }

  /**
   * Get the combined inventory including calculated parameters
   * @return the expanded collection inventory
   */
  const gridinventory::Inventory & getExpandedInventory() const
  {
    return expanded_inventory;
  }

  /**
   * Set the combined inventory including calculated parameters
   * @return the expanded collection inventory
   */
  void setExpandedInventory(gridinventory::Inventory exp_inv)
  {
    expanded_inventory = exp_inv;
  }

  /**
   * Get the individual inventories
   * @return list of inventories
   */
  std::vector<gridinventory::Inventory> getInventories() const;

  /**
   * Set reference time limits
   * @param min
   * @param max
   */
  void setReferencetimeLimits(const std::string& min,
      const std::string& max)
  {
    limit_min = min;
    limit_max = max;
  }

  /**
   * Get the reference times for a specific model
   * @return set<std::string>
   */
  std::set<std::string> getReferenceTimes() const;

  bool useTimeFromFilename() const
  {
    return timeFromFilename;
  }

  std::set<miutil::miTime>  getTimesFromFilename() const
  {
    return timesFromFilename;
  }

  /**
   * Get the Grid for a specific model
   * @return gridinventory::Grid
   */
  gridinventory::Grid getGrids() const;

  /**
   *
   * @return name of collection
   */
  std::string getName() const
  {
    return collectionname;
  }

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
 bool dataExists(const std::string& reftime, const std::string& paramname,
      const std::string& zaxis, const std::string& taxis,
      const std::string& extraaxis, const std::string& version,
      const std::string& level, const miutil::miTime& time,
      const std::string& elevel, const int & time_tolerance,
      gridinventory::GridParameter & param);


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
  Field * getData(const std::string& reftime,
      const std::string& paramname,
      const std::string& zaxis, const std::string& taxis,
      const std::string& extraaxis, const std::string& version,
      const std::string& level, const miutil::miTime& time,
      const std::string& elevel, const std::string& unit,
      const int & time_tolerance);

  vcross::Values_p  getVariable(const std::string& reftime, const std::string& paramname);

  bool putData(const std::string& reftime,
      const std::string& paramname,
      const std::string& zaxis, const std::string& taxis,
      const std::string& runaxis, const std::string& version,
      const std::string& level, const miutil::miTime& time,
      const std::string& run, const std::string& unit,
      const std::string& output_time,
      const int & time_tolerance, const Field* field);


  /**
   * Get a list of raw sources.
   * @return vector of strings
   */
  const std::vector<std::string>& getRawSources() const
    { return rawsources; }

  /**
   * Updates the information held about the data sources.
   */
  void updateSources();

protected:
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
  GridIOsetup * gridsetup;
  /// inventory made, and ok
  std::map< std::string, bool > inventoryOK;
  /// lower limit on reference times
  std::string limit_min;
  /// upper limit on reference times
  std::string limit_max;
  /// No complete inventory, only reftimes from filename
  std::set<std::string> refTimes;
  /// the combined inventory
  gridinventory::Inventory inventory;
  /// the combined inventory with computed parameters
  gridinventory::Inventory expanded_inventory;
  /// the actual data-containers - list of GridIO objects
  typedef std::vector<GridIO*> gridsources_t;
  gridsources_t gridsources;
  std::map<miutil::miTime,GridIO*> gridsourcesTimeMap;

  /// unpack the raw sources and make one or more GridIO instances
  bool makeGridIOinstances();
  /// clear the gridsources vector
  void clearGridSources();
  /// update the gridsources vector
  void updateGridSources();
/// check if data exists in inventory, using GridParameterKey
  bool dataExists_gridParameter(const gridinventory::Inventory& inv,
      const std::string& reftime,
      const gridinventory::GridParameterKey& parkey, const std::string& level,
      const miutil::miTime& time, const std::string& elevel,
      const int & time_tolerance, gridinventory::GridParameter & param,
      miutil::miTime & actualtime);
  bool dataExists_variable(const gridinventory::Inventory& inv,
      const std::string& reftime, const std::string paramname);

};

#endif /* GRIDCOLLECTION_H_ */
