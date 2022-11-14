/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2010-2022 met.no

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

/*
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */

#include "GridIO.h"

#include "diField.h"
#include "diFieldUtil.h"

#include <algorithm>

#define MILOGGER_CATEGORY "diField.GridIO"
#include "miLogger/miLogging.h"


namespace {

template <class C>
typename C::const_iterator find_name(const C& c, const std::string& name)
{
  return std::find_if(c.begin(), c.end(), [&](const typename C::value_type& v) { return v.getName() == name; });
}

} // namespace

GridIO::GridIO() {}

GridIO::~GridIO()
{
}

const gridinventory::ReftimeInventory & GridIO::getReftimeInventory(const std::string reftime) const
{
  const std::map<std::string, gridinventory::ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  if (ritr != inventory.reftimes.end())
    return ritr->second;
  static const gridinventory::ReftimeInventory EMPTY_REFTIMEINVENTORY;
  return EMPTY_REFTIMEINVENTORY;
}


/**
 * Get the reference times
 */
std::set<std::string> GridIO::getReferenceTimes() const
{
  using namespace gridinventory;
  std::set<std::string> reftimes;
  for (const auto& ir : inventory.reftimes) {
    const gridinventory::ReftimeInventory& ri = ir.second;
    if ((limit_min.empty() || limit_max.empty()) || (ri.referencetime >= limit_min && ri.referencetime <= limit_max)) {
      reftimes.insert(ri.referencetime);
    }
  }
  return reftimes;
}

const gridinventory::ReftimeInventory& GridIO::findModelAndReftime(const std::string& reftime) const
{
  using namespace gridinventory;

  const std::map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  if (ritr == inventory.reftimes.end()) {
    static const gridinventory::ReftimeInventory EMPTY_REFTIME;
    return EMPTY_REFTIME;
  }

  return ritr->second;
}

/**
 * get the grid from reftime with name = grid
 */
const gridinventory::Grid& GridIO::getGrid(const std::string & reftime, const std::string & grid)
{
  const gridinventory::ReftimeInventory& rti = findModelAndReftime(reftime);
  const auto gitr = find_name(rti.grids, grid);
  if (gitr != rti.grids.end()) {
    return *gitr;
  }
  static const gridinventory::Grid EMPTY_GRID;
  return EMPTY_GRID;
}

/**
 * get the zaxis from reftime with name = zaxis
 */
const gridinventory::Zaxis& GridIO::getZaxis(const std::string & reftime, const std::string & zaxis)
{
  using namespace gridinventory;
  const ReftimeInventory& rti = findModelAndReftime(reftime);
  const auto zaitr = find_name(rti.zaxes, zaxis);
  if (zaitr != rti.zaxes.end())
    return *zaitr;
  static const gridinventory::Zaxis EMPTY_ZAXIS;
  return EMPTY_ZAXIS;
}

/**
 * get the taxis from reftime with name = taxis
 */
const gridinventory::Taxis& GridIO::getTaxis(const std::string & reftime, const std::string & taxis)
{
  using namespace gridinventory;
  const ReftimeInventory& rti = findModelAndReftime(reftime);
  const auto taitr = find_name(rti.taxes, taxis);
  if (taitr != rti.taxes.end())
    return *taitr;
  static const gridinventory::Taxis EMPTY_TAXIS;
  return EMPTY_TAXIS;
}

/**
 * get the extraaxis from reftime with name = extraaxis
 */
const gridinventory::ExtraAxis& GridIO::getExtraAxis(const std::string & reftime, const std::string & extraaxis)
{
  using namespace gridinventory;
  const ReftimeInventory& rti = findModelAndReftime(reftime);
  const auto xit = find_name(rti.extraaxes, extraaxis);
  if (xit != rti.extraaxes.end())
    return *xit;
  static const gridinventory::ExtraAxis EMPTY_EXTRAAXIS;
  return EMPTY_EXTRAAXIS;
}

/**
 * make and initialize field
 */
Field_p GridIO::initializeField(const std::string& modelname, const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level,
                                const miutil::miTime& time, const std::string& elevel)
{
  // find the grid
  const gridinventory::Grid& grid = getGrid(reftime, param.grid);
  if (grid.nx < 1 || grid.ny < 1) {
    METLIBS_LOG_DEBUG(LOGVAL(grid.nx) << LOGVAL(grid.ny));
    return Field_p();
  }

  Field_p field = std::make_shared<Field>();
  field->data = new float[grid.nx * grid.ny];
  field->area = diutil::makeGridArea(grid);
  field->level = atoi(level.c_str());
  field->idnum = atoi(elevel.c_str());
  field->forecastHour = -32767;

  if ( !reftime.empty() ) {
    field->analysisTime = miutil::miTime(reftime);
  }

  field->validFieldTime = time;
  field->name = param.key.name;
  field->text = param.key.name;
  field->fulltext = param.key.name;
  field->paramName = param.key.name;
  field->modelName = modelname;

  return field;
}
