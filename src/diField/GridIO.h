/* -*- c++ -*-
 * GridIO.h
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */
/*
 $Id:$

 Copyright (C) 2006 met.no

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

#ifndef GRIDIO_H_
#define GRIDIO_H_

#include "GridInventoryTypes.h"
#include "diField.h"
#include "VcrossData.h"

#include <puTools/miTime.h>
#include <boost/shared_array.hpp>

#include <map>
#include <set>
#include <string>


class GridIOsetup {
public:
  GridIOsetup()
  {
  }
  virtual ~GridIOsetup()
  {
  }
  virtual bool parseSetup(std::vector<std::string> lines, std::vector<
      std::string>& errors) = 0;
};

/**
 * Base class for all grid sources
 * Pure virtual.
 * The virtual functions you need to implement for a specific grid IO are:
 * - bool makeInventory()
 * - Field * getData(...)
 */

class GridIO {
protected:
  std::string source_name;
  gridinventory::Inventory inventory; ///< The inventory for this grid source

  std::string limit_min; ///< only include reference times greater than this
  std::string limit_max; ///< only include reference times less than this


public:
  typedef boost::shared_array<float> FloatArray;

public:
  GridIO();
  GridIO(const std::string source);
  virtual ~GridIO();

  /**
   * Get source string
   * @return std::string
   */
  const std::string & getSourceName() const
  {
    return source_name;
  }

  /**
   * Get inventory
   * @return Inventory
   */
  const gridinventory::Inventory & getInventory() const
  {
    return inventory;
  }

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
   * Get the referencetimes
   * @return set<miTime>
   */
  std::set<std::string> getReferenceTimes() const;

  /**
   * get the grid from reftime with name = grid
   * @param reftime
   * @param grid-name
   * @return grid
   */
  const gridinventory::Grid& getGrid(const std::string & reftime, const std::string & grid);

  /**
   * get the Taxis from reftime with name = taxis
   * @param reftime
   * @param taxis-name
   * @return taxis
   */
  const gridinventory::Taxis& getTaxis(const std::string & reftime, const std::string & taxis);

  /**
   * get the Zaxis from reftime with name = zaxis
   * @param reftime
   * @param zaxis-name
   * @return zaxis
   */
  const gridinventory::Zaxis& getZaxis(const std::string & reftime, const std::string & zaxis);

  /**
   * get the extraaxis from reftime with name = extraaxis
   * @param reftime
   * @param extraaxis-name
   * @return extraaxis
   */
  const gridinventory::ExtraAxis& getExtraAxis(const std::string & reftime, const std::string & extraaxis);

  /**
   * make and initialize Field
   * @param modelname
   * @param reftime
   * @param param
   * @param level
   * @param time
   * @param elevel
   * @return Field pointer
   */
  Field * initializeField(const std::string& modelname,
      const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& level, const miutil::miTime& time,
      const std::string& elevel, const std::string& unit);

  /**
   * Get data slice as Field
   * @param reftime
   * @param paramname
   * @param grid
   * @param z axis
   * @param time axis
   * @param extraaxis
   * @param version
   * @param level
   * @param time
   * @param elevel
   * @return field
   */
  virtual Field * getData(const std::string& reftime, const std::string& paramname,
      const std::string& grid, const std::string& zaxis,
      const std::string& taxis, const std::string& extraaxis,
      const std::string& version, const std::string& level,
      const miutil::miTime& time, const std::string& elevel,
      const std::string& unit);

  // ===================== PURE VIRTUAL FUNCTIONS BELOW THIS LINE ============================

  /**
   * Returns whether the source has changed since the last makeInventory
   * @return bool
   */
  virtual bool sourceChanged(bool update) = 0;

  /**
   * Build inventory containing only referencetime for filename
   * @return status
   */

  /**
   * Return referencetime from filename given in constructor
   * @return reftime_from_filename
   */
  virtual std::string getReferenceTime() const = 0;

  /**
    * Returns true if referencetime matches
    * @return bool
    */
  virtual bool referenceTimeOK(const std::string & refTime) = 0;

  /**
   * Build the inventory from source
   * @param reftime, reference time to make inventory for. Use miTime::undef for none
   * @return status
   */
  virtual bool makeInventory(const std::string& reftime) = 0;

  /**
   * Get data slice as Field
   * @param reftime
   * @param param is a GridParameter
   * @param level
   * @param time
   * @param elevel
   * @return field
   */
  virtual Field * getData(const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& level, const miutil::miTime& time,
      const std::string& elevel, const std::string& unit) = 0;

  virtual vcross::Values_p getVariable(const std::string& varName)=0;

  /**
   * Get data slice
   * @param inventory
   * @return field
   */
  //virtual Field * getData(const gridinventory::Inventory& inv) = 0;

private:
  const gridinventory::ReftimeInventory* findModelAndReftime(const std::string& reftime) const;
};

#endif /* GRIDIO_H_ */
