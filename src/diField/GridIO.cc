/*
 * GridIO.cc
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */

#include "GridIO.h"

#include "puTools/mi_boost_compatibility.hh"

#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#define MILOGGER_CATEGORY "diField.GridIO"
#include "miLogger/miLogging.h"

using namespace std;

GridIO::GridIO()
{
  // TODO Auto-generated constructor stub
}

GridIO::GridIO(const std::string source)
  : source_name(source)
{
}

GridIO::~GridIO()
{
  // TODO Auto-generated destructor stub
}



/**
 * Get the reference times
 */
std::set<std::string> GridIO::getReferenceTimes() const
{
  using namespace gridinventory;
  std::set<std::string> reftimes;
  for(gridinventory::Inventory::reftimes_t::const_iterator it_ri = inventory.reftimes.begin(); it_ri != inventory.reftimes.end(); ++it_ri) {
    const gridinventory::ReftimeInventory& ri = it_ri->second;
    if ((limit_min.empty() || limit_max.empty())
        || (ri.referencetime >= limit_min && ri.referencetime <= limit_max))
    {
      reftimes.insert(ri.referencetime);
    }
  }
  return reftimes;
}

const gridinventory::ReftimeInventory* GridIO::findModelAndReftime(const std::string& reftime) const
{
  using namespace gridinventory;

  const std::map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  if (ritr == inventory.reftimes.end())
    return 0;

  return &ritr->second;
}

/**
 * get the grid from reftime with name = grid
 */
const gridinventory::Grid& GridIO::getGrid(const std::string & reftime, const std::string & grid)
{
  const gridinventory::ReftimeInventory* rti = findModelAndReftime(reftime);
  if (rti) {
    const std::set<gridinventory::Grid>::const_iterator gitr = miutil::find_if(rti->grids, boost::bind(&gridinventory::Grid::getName, _1) == grid);
    if (gitr != rti->grids.end()) {
      return *gitr;
    }
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
  const ReftimeInventory* rti = findModelAndReftime(reftime);
  if (rti) {
    const std::set<Zaxis>::const_iterator zaitr = miutil::find_if(rti->zaxes, boost::bind(&Zaxis::getName, _1) == zaxis);
    if (zaitr != rti->zaxes.end())
      return *zaitr;
  }
  static const gridinventory::Zaxis EMPTY_ZAXIS;
  return EMPTY_ZAXIS;
}

/**
 * get the taxis from reftime with name = taxis
 */
const gridinventory::Taxis& GridIO::getTaxis(const std::string & reftime, const std::string & taxis)
{
  using namespace gridinventory;
  const ReftimeInventory* rti = findModelAndReftime(reftime);
  if (rti) {
    const std::set<Taxis>::const_iterator taitr = miutil::find_if(rti->taxes, boost::bind(&Taxis::getName, _1) == taxis);
    if (taitr != rti->taxes.end())
      return *taitr;
  }
  static const gridinventory::Taxis EMPTY_TAXIS;
  return EMPTY_TAXIS;
}

/**
 * get the extraaxis from reftime with name = extraaxis
 */
const gridinventory::ExtraAxis& GridIO::getExtraAxis(const std::string & reftime, const std::string & extraaxis)
{
  using namespace gridinventory;
  const ReftimeInventory* rti = findModelAndReftime(reftime);
  if (rti) {
    const std::set<ExtraAxis>::const_iterator xit = miutil::find_if(rti->extraaxes, boost::bind(&ExtraAxis::getName, _1) == extraaxis);
    if (xit != rti->extraaxes.end())
      return *xit;
  }
  static const gridinventory::ExtraAxis EMPTY_EXTRAAXIS;
  return EMPTY_EXTRAAXIS;
}

/**
 * make and initialize field
 */
Field * GridIO::initializeField(const std::string& modelname,
    const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& level, const miutil::miTime& time,
    const std::string& elevel, const::string& unit)
{
  // find the grid
  gridinventory::Grid grid = getGrid(reftime, param.grid);
  if (grid.nx < 1 || grid.ny < 1) {
    METLIBS_LOG_DEBUG(LOGVAL(grid.nx) << LOGVAL(grid.ny));
    return (Field*) (0);
  }

  // make the projection and area types
  Projection proj(grid.projection);
  Rectangle rect(0, 0, (grid.nx - 1) * grid.x_resolution, (grid.ny - 1) * grid.y_resolution);
  // TODO: which one is correct?
  // Rectangle rect(grid.x_0, grid.y_0, grid.x_0 + (grid.nx -1) * grid.x_resolution, grid.y_0 + (grid.ny - 1) * grid.y_resolution);

  Field * field = new Field();
  field->data = new float[grid.nx * grid.ny];
  field->area = GridArea(Area(proj, rect), grid.nx, grid.ny, grid.x_resolution, grid.y_resolution);
  field->allDefined = false;
  field->level = atoi(level.c_str());
  field->idnum = atoi(elevel.c_str());
  field->forecastHour = -32767;
  field->unit = unit;

  if ( !reftime.empty() ) {
    field->analysisTime = miutil::miTime(reftime);
  }

  field->validFieldTime = time;
  field->name = param.key.name;
  field->text = param.key.name;
  field->fulltext = param.key.name;
  field->modelName = modelname;
  field->paramName = param.key.name;
  field->fieldText = "";
  field->leveltext = "";
  field->idnumtext = "";
  field->progtext = "";
  field->timetext = "";

  return field;
}


/**
 * Get data slice
 */
Field * GridIO::getData(const std::string& reftime, const std::string& paramname,
    const std::string& grid, const std::string& zaxis,
    const std::string& taxis, const std::string& extraaxis,
    const std::string& version, const std::string& level,
    const miutil::miTime& time, const std::string& elevel,
    const std::string& unit)
{
  gridinventory::GridParameter gparam(gridinventory::GridParameterKey(
      paramname, zaxis, taxis, extraaxis, version));
  return getData(reftime, gparam, level, time, elevel, unit);
}
