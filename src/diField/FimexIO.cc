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
//  Created on: Mar 11, 2010
//      Author: audunc

#include "FimexIO.h"

#include "diField.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>
#include <puTools/miDirtools.h>

#include <fimex/CDM.h>
#include <fimex/CDMException.h>
#include <fimex/CDMFileReaderFactory.h>
#include <fimex/CDMReaderWriter.h>
#include <fimex/Data.h>
#include <fimex/interpolation.h>
#include <fimex/CoordinateSystemSliceBuilder.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMExtractor.h>
#include <fimex/vertical_coordinate_transformations.h>
#include <fimex/coordSys/verticalTransform/HybridSigmaPressure1.h>
#include <fimex/coordSys/verticalTransform/Pressure.h>
#include <fimex/coordSys/verticalTransform/Height.h>
#include <fimex/coordSys/verticalTransform/ToVLevelConverter.h>
#include <fimex/Units.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/greg_duration.hpp>

#include <sstream>

#define MILOGGER_CATEGORY "diField.FimexIO"
#include "miLogger/miLogging.h"

using namespace miutil;
using namespace MetNoFimex;

namespace {
const std::string format_metnofieldfile = "metnofieldfile";
const std::string fimex_metnofieldfile = "felt";

const char METER[] = "m";
const char SECONDS_SINCE_1970[] = "seconds since 1970-01-01 00:00:00";

const char REPROJECTION[] = "reprojection";
const char EXTRACT[] = "extract";

//! copy the axis' name if the axis is not null
void copyAxisName(CoordinateAxis_cp axis, std::string& nameVar)
{
  if (axis)
    nameVar = axis->getName();
}

} // anonymous namespace

// #######################################################################

int FimexIO::findTimeIndex(const gridinventory::Taxis& taxis, const miutil::miTime& time)
{
  if (time.undef())
    return 0;

  int taxis_index = 0;
  // TODO convert time to std::string in seconds-since-epoch, then use std::find
  for (double tv : taxis.values) {
    time_t tt = tv;
    miTime t(tt);
    if (t == time) {
      if (noOfClimateTimes > 0)
        taxis_index %= noOfClimateTimes;
      return taxis_index;
    }
    taxis_index += 1;
  }
  return -1;
}

int FimexIO::findZIndex(const gridinventory::Zaxis& zaxis, const std::string& zlevel)
{
  // if zaxis has just one value, do not even check
  if (zaxis.stringvalues.size() == 1)
    return 0;

  // find the index to the correct Z value
  const auto& zv = zaxis.stringvalues;
  const auto it = std::find(zv.begin(), zv.end(), zlevel);
  if (it == zv.end())
    return -1;
  if (!zaxis.positive) {
    return std::distance(zv.begin(), it);
  } else {
    return std::distance(it, zv.end()) - 1;
  }
}

int FimexIO::findExtraIndex(const gridinventory::ExtraAxis& extraaxis, const std::string& elevel)
{
  const auto& ev = extraaxis.stringvalues;
  const auto it = std::find(ev.begin(), ev.end(), elevel);
  if (it == ev.end())
    return -1;
  else
    return std::distance(ev.begin(), it);
}

// find an appropriate coordinate system for the variable
MetNoFimex::CoordinateSystem_cp findCoordinateSystem(const CoordinateSystem_cp_v& coordSys, const std::string& varName)
{
  CoordinateSystem_cp_v::const_iterator varSysIt = std::find_if(coordSys.begin(), coordSys.end(),
      CompleteCoordinateSystemForComparator(varName));
  if (varSysIt == coordSys.end()) {
    METLIBS_LOG_INFO("No coordinate system found for '" << varName << '\'' );
    return MetNoFimex::CoordinateSystem_cp();
  }
  CoordinateSystem_cp varCS = *varSysIt;

  if (not varCS->isSimpleSpatialGridded()) {
    METLIBS_LOG_INFO("Coordinate system for '" << varName << "' is not a simple spatial grid" );
    return CoordinateSystem_cp();
  }

  // check that geographical X/Lon and Y/Lat axes are defined
  if (not varCS->getGeoXAxis() or not varCS->getGeoYAxis()) {
    METLIBS_LOG_INFO("Missing one geographical axis for '" << varName << '\'');
    return CoordinateSystem_cp();
  }

  return varCS;
}

/**
 * Parse setup information
 */
bool FimexIOsetup::parseSetup(std::vector<std::string> lines,
    std::vector<std::string>& errors)
{
  optionMap.clear();
  const int nlines = lines.size();
  for (int l = 0; l < nlines; l++) {
    const std::vector<std::string> tokens = miutil::split_protected(lines[l],
        '"', '"');
    const int m = tokens.size();
    std::map<std::string, std::string> key_valueMap;
    std::string current_name;
    for (int j = 0; j < m; j++) {
      std::vector<std::string> stokens = miutil::split_protected(tokens[j], '"',
          '"', "=", true);
      if (stokens.size() < 2) {
        std::string error = getSectionName() + "|" + miutil::from_number(l)
            + "|Missing argument to keyword: " + tokens[j];
        errors.push_back(error);
        continue;
      }
      std::string key = stokens[0];
      std::string value = stokens[1];
      miutil::remove(key, '"');
      miutil::remove(value, '"');
      if (key == "name") {
        current_name = value;
      } else {
        key_valueMap[key] = value;
      }
    }
    if (current_name.empty()) {
      //no name found
      std::string error = getSectionName() + "|" + miutil::from_number(l)
          + "|Missing name keyword: " + lines[l];
      errors.push_back(error);
      continue;
    } else {
      // Valid option found
      optionMap[current_name] = key_valueMap;
    }
  }

  return true;
}

// ------------------------------------------------------------------------

FimexIO::FimexIO(const std::string& modelname, const std::string& sourcename, const std::string& reftime, const std::string& format, const std::string& config,
                 const std::vector<std::string>& options, bool makeFeltReader, FimexIOsetup* s)
    : sourceOk(false)
    , modificationTime(0)
    , source_name(sourcename)
    , model_name(modelname)
    , source_type(format)
    , config_filename(config)
    , reftime_from_file(reftime)
    , singleTimeStep(false)
    , noOfClimateTimes(0)
    , writeable(false)
    , turnWaveDirection(false)
    , setup(s)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelname) << LOGVAL(sourcename) << LOGVAL(reftime) << LOGVAL(format) << LOGVAL(config));

  // support for old diana-names
  if (source_type == format_metnofieldfile)
    source_type = fimex_metnofieldfile;

  for ( size_t i=0; i<options.size(); i++) {
    int pos = options[i].find_first_of('=');
    std::string key = options[i].substr(0,pos);
    boost::algorithm::to_lower(key);
    std::string value = options[i].substr(pos+1);
    if (key == "proj4string") {
      projDef = value;
      diutil::remove_quote(projDef);
    } else if (key == "singletimestep") {
      singleTimeStep = true;
    } else if (key == "writeable") {
      boost::algorithm::to_lower(value);
      writeable = (value == "true");
    } else if (key == "turnwavedirection") {
      boost::algorithm::to_lower(value);
      turnWaveDirection = (value == "true");
    } else if (key == "vectorprojectionlonlat") {
      boost::algorithm::to_lower(value);
      vectorProjectionLonLat = (value == "true");
    } else if (key == "r") {
      reproj_name=value;
    } else {
      METLIBS_LOG_ERROR("unknown option" << LOGVAL(key) << LOGVAL(value));
    }
  }

  if (makeFeltReader) {
    feltReader = createReader();
    reftime_from_file = fallbackGetReferenceTime();
    checkSourceChanged(true);
  }
  METLIBS_LOG_DEBUG(LOGVAL(reftime_from_file));
}

FimexIO::~FimexIO()
{
}

/**
 * Returns whether the source has changed since the last makeInventory
 */
bool FimexIO::checkSourceChanged(bool update)
{
  //Unchecked source, not possible to check
  if (!update && modificationTime == 0) {
    return false;
  }

  const long ctime_ = miutil::path_ctime(source_name);
  if (ctime_ != modificationTime) {
    if (update) {
      modificationTime = ctime_;
    }
    return true;
  }

  return false;
}

bool FimexIO::sourceChanged()
{
  return checkSourceChanged(false);
}

namespace /* anonymous */ {

std::string isoFromTimeT(time_t ref_time_t)
{
  const miTime ref_miTime(ref_time_t);
  return ref_miTime.isoTime("T");
}
std::string makeId(const std::string& name, size_t size)
{
  if (size == 1)
    return name;
  std::ostringstream id;
  id << name << "_" << size;
  return id.str();
}

typedef std::map<std::string, std::string> name2id_t;

void addTimeAxis(CoordinateAxis_cp tAxis, const std::vector<double>& values, const std::string& reftime, gridinventory::Inventory& inventory)
{
  gridinventory::Taxis invt(tAxis->getName(), values);
  gridinventory::ReftimeInventory invr(reftime);
  invr.taxes.insert(invt);
  inventory.reftimes[invr.referencetime] = invr;
}

typedef std::map<std::string, std::string> reproj_config_t;

const std::string& get_string(const reproj_config_t& rc, const std::string& key)
{
  reproj_config_t::const_iterator it = rc.find(key);
  static const std::string EMPTY;
  return it != rc.end() ? it->second : EMPTY;
}

float get_float(const reproj_config_t& rc, const std::string& key)
{
  return atof(get_string(rc, key).c_str());
}

} // namespace anonymous

CDMReader_p FimexIO::createReader()
{
  try {
    CDMReader_p feltReader;
    if (writeable)
        feltReader = CDMFileReaderFactory::createReaderWriter(source_type, source_name,config_filename);
    else
        feltReader = CDMFileReaderFactory::create(source_type, source_name,config_filename);

    if (!reproj_name.empty()) {
      // get the reprojection data from optionMap
      const auto reproj_data_it = setup->optionMap.find(reproj_name);
      if (reproj_data_it != setup->optionMap.end()) {
        const std::map<std::string, std::string>& reproj_data = reproj_data_it->second;
        // Valid keys name, type, projString, xAxisValues, yAxisValues, xAxisUnit, yAxisUnit,
        // method = nearestneighbor, bilinear,bicubic, forward_max, forward_min, forward_mean, forward_median, forward_sum, coord_nearestneighbor, coord_kdtree
        if (get_string(reproj_data, "type") == REPROJECTION) {
          CDMInterpolator_p interpolator = std::make_shared<CDMInterpolator>(feltReader);
          int method = mifi_string_to_interpolation_method(get_string(reproj_data, "method").c_str());
          if ( method == MIFI_INTERPOL_UNKNOWN ) {
            /* Parse error, log and use default interpolation type.*/
            METLIBS_LOG_WARN("Invalid interpolation type in setup file: " << get_string(reproj_data, "method"));
            METLIBS_LOG_WARN("Using default interpolation type: MIFI_INTERPOL_NEAREST_NEIGHBOR");
            method = MIFI_INTERPOL_NEAREST_NEIGHBOR;
          }
          interpolator->changeProjection(method, get_string(reproj_data, "projString"),
                                         get_string(reproj_data, "xAxisValues"), get_string(reproj_data, "yAxisValues"),
                                         get_string(reproj_data, "xAxisUnit"), get_string(reproj_data, "yAxisUnit"));
          return interpolator;
        } else if (get_string(reproj_data, "type") == EXTRACT) {
            std::shared_ptr<CDMExtractor> extractor = std::make_shared<CDMExtractor>(feltReader);
            extractor->reduceLatLonBoundingBox(get_float(reproj_data, "south"), get_float(reproj_data, "north"),
                                               get_float(reproj_data, "west"), get_float(reproj_data, "east"));
            return extractor;
        } else {
          METLIBS_LOG_WARN("Invalid option type in FimexIO::createReader: " << get_string(reproj_data, "type"));
        }
      } else {
        METLIBS_LOG_WARN("reprojection data not found in FimexIO::createReader: " << reproj_name);
      }
    }

    // Return no interpolation.
    return feltReader;
  } catch (CDMException& ex) {
    METLIBS_LOG_WARN("exception in FimexIO::createReader: " << ex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("exception in FimexIO::createReader: " << ex.what());
  }
  return CDMReader_p();
}

void FimexIO::inventoryExtractGridProjection(MetNoFimex::Projection_cp projection, gridinventory::Grid& grid,
    CoordinateAxis_cp xAxis, CoordinateAxis_cp yAxis)
{
  grid.x_0 = 0;
  grid.y_0 = 0;

  const std::string xyUnit = projection->isDegree() ? "degree" : METER;
  DataPtr xdata = feltReader->getScaledDataInUnit(xAxis->getName(), xyUnit);
  DataPtr ydata = feltReader->getScaledDataInUnit(yAxis->getName(), xyUnit);
  if (xdata) {
    size_t nx = xdata->size();
    METLIBS_LOG_DEBUG("X-data.size():" << nx);
    if (nx > 1) {
      MetNoFimex::shared_array<float> fdata = xdata->asFloat();
      grid.nx = nx;
      grid.x_resolution = (fdata[nx - 1] - fdata[0]) / (nx - 1);
      grid.x_0 = fdata[0];
      METLIBS_LOG_DEBUG(" x_resolution:" << grid.x_resolution << " x_0:" << grid.x_0);
    }
  }
  if (ydata) {
    size_t ny = ydata->size();
    METLIBS_LOG_DEBUG("Y-data.size():" << ny);
    if (ny > 1) {
      MetNoFimex::shared_array<float> fdata = ydata->asFloat();
      grid.ny = ny;
      grid.y_direction_up = !(fdata[0] > fdata[1]);
      grid.y_resolution = std::abs((fdata[ny - 1] - fdata[0]) / (ny - 1));
      grid.y_0 = fdata[grid.y_direction_up ? 0 : ny - 1];
      METLIBS_LOG_DEBUG(" y_resolution:" << grid.y_resolution << " y_0:" << grid.y_0);
    }
  }

  if (!projDef.empty()) {
    grid.projection = projDef;
    METLIBS_LOG_DEBUG("using projection from setup: '" << grid.projection << "'");
  } else {
    grid.projection = projection->getProj4String();
  }
}

void FimexIO::inventoryExtractGrid(std::set<gridinventory::Grid>& grids, CoordinateSystem_cp cs,
    CoordinateAxis_cp xAxis, CoordinateAxis_cp yAxis)
{
  METLIBS_LOG_TIME();
  // The Grid type for this coordinate system
  gridinventory::Grid grid;
  grid.name = xAxis->getName() + yAxis->getName();
  
  if (cs->hasProjection()) {
    grid.name += "_";
    grid.name += cs->getProjection()->getName();
  }
  if (grid.name == "_") {
    grid.name = "grid";
  }
  grid.id = grid.name;
  if (grids.find(grid) == grids.end()) {
    if (cs->hasProjection()) {
      MetNoFimex::Projection_cp projection = cs->getProjection();
      inventoryExtractGridProjection(projection, grid, xAxis, yAxis);
    }

    METLIBS_LOG_DEBUG("Inserting grid:" <<grid.name
        << " [" << grid.projection << "] nx:" << grid.nx << " ny:" << grid.ny);
    grids.insert(grid);
  } else {
    METLIBS_LOG_DEBUG("Known grid:" <<grid.name);
  }
}

void FimexIO::inventoryExtractVAxis(std::set<gridinventory::Zaxis>& zaxes, name2id_t& name2id,
    CoordinateAxis_cp vAxis, CoordinateSystem_cp& cs)
{
  METLIBS_LOG_TIME();
  if (name2id.find(vAxis->getName()) != name2id.end()) {
    METLIBS_LOG_DEBUG("vertical axis '" << vAxis->getName() << "' seems to be known already");
  }

  std::string verticalType;
  if (cs->hasVerticalTransformation()) {
    VerticalTransformation_cp vtran = cs->getVerticalTransformation();
    verticalType = vtran->getName();
  } else {
    verticalType = vAxis->getName();
  }

  //vertical axis recognized
  DataPtr vdata;
  if (verticalType == "pressure")
    vdata = feltReader->getScaledDataInUnit(vAxis->getName(), "hPa");
  else
    vdata= feltReader->getScaledData(vAxis->getName());
  if (!vdata || vdata->size() == 0)
    return;

  METLIBS_LOG_DEBUG(LOGVAL(vAxis->getName()) << LOGVAL(vdata->size()));
  MetNoFimex::shared_array<double> idata = vdata->asDouble();
  const std::vector<double> levels(idata.get(), idata.get() + vdata->size());

  CDMAttribute attr;
  bool positive = true;
  if (feltReader->getCDM().getAttribute(vAxis->getName(), "positive", attr)) {
    positive = (attr.getStringValue() == "up");
  }

  const std::string id = makeId(vAxis->getName(), levels.size());
  zaxes.insert(gridinventory::Zaxis(id, vAxis->getName(), positive, levels,verticalType));
  name2id[vAxis->getName()] = id;
}

void FimexIO::inventoryExtractExtraAxes(std::set<gridinventory::ExtraAxis>& extraaxes, name2id_t& name2id,
    const std::vector<std::string>& unsetList, const CoordinateAxis_cp_v axes)
{
  const std::set<std::string> unset(unsetList.begin(), unsetList.end());
  for (CoordinateAxis_cp axis : axes) {
    const std::string& name = axis->getName();
    if (unset.find(name) == unset.end())
      continue;

    if (name2id.find(axis->getName()) != name2id.end()) {
      METLIBS_LOG_DEBUG("extra axis '" << axis->getName() << "' seems to be known already");
      continue;
    }

    DataPtr edata = feltReader->getScaledData(name);
    METLIBS_LOG_DEBUG("extra axis '" << name << "' size " << edata->size());
    
    MetNoFimex::shared_array<double> idata = edata->asDouble();
    const std::vector<double> elevels(idata.get(), idata.get() + edata->size());

    const std::string id = makeId(name, edata->size());
    extraaxes.insert(gridinventory::ExtraAxis(id, name, elevels));
    name2id[name] = id;
  }
}

std::string FimexIO::fallbackGetReferenceTime()
{
  try {
    if (feltReader) {
      const FimexTime refTime = getUniqueForecastReferenceTimeFT(feltReader);
      return make_time_string_extended(refTime);
    }
  } catch (CDMException& ex) {
    METLIBS_LOG_DEBUG("exception occurred: " << ex.what() );
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("exception occurred: " << ex.what() );
  }
  return "";
}

/**
 * Build the inventory from source
 *
 * The parameter reftime generally instructs us which subdata we
 * should focus on, but in FimexIO we always do the whole source..
 * makeInventory is normally called twice:
 * phase 1:
 * phase 2: with reference time
 * means we can generally skip phase 2 - checking timestamp for phase 1
 */
bool FimexIO::makeInventory(const std::string& reftime)
{
  METLIBS_LOG_TIME();
  if (!reftime_from_file.empty() && reftime_from_file != reftime)
    return true;

  if (!checkSourceChanged(true) && sourceOk)
    return true;

  METLIBS_LOG_INFO("Source:" << source_name <<" : "<<config_filename<<" : " <<reftime_from_file);
  if (not feltReader){
    feltReader = createReader();
    if (not feltReader)
      return false;
  }

  if (reftime_from_file.empty()) {
    reftime_from_file = fallbackGetReferenceTime();
  }

  std::set<gridinventory::GridParameter> parameters;
  std::set<gridinventory::Grid> grids;
  std::set<gridinventory::Zaxis> zaxes;
  std::set<gridinventory::Taxis> taxes;
  std::set<gridinventory::ExtraAxis> extraaxes;

  std::map<std::string, std::string> name2id;

  if (not feltReader)
    return false;

  try {

    // Get the CDM from the reader
    const CDM& cdm = feltReader->getCDM();

    // get all coordinate systems from file, usually one, but may be a few (theoretical limit: # of variables)
    coordSys = MetNoFimex::listCoordinateSystems(feltReader);

    // First add a set of empty axes (in case they are missing in the data)
    extraaxes.insert(gridinventory::ExtraAxis(""));
    zaxes.insert(gridinventory::Zaxis(""));
    taxes.insert(gridinventory::Taxis(""));

    std::vector<std::string> referenceTimes;
    std::string referenceTime = reftime_from_file;

    METLIBS_LOG_DEBUG("Coordinate Systems Loop");
    for (CoordinateSystem_cp cs : coordSys) {
      METLIBS_LOG_DEBUG("NEW Coordinate System");
      if (not cs->isSimpleSpatialGridded())
        continue;

      METLIBS_LOG_DEBUG("CS is simple spatial grid");

      // find the geographical axes, returns 0 axes if not found
      CoordinateAxis_cp xAxis = cs->getGeoXAxis(); // X or Lon
      CoordinateAxis_cp yAxis = cs->getGeoYAxis(); // Y or Lat
      if (!xAxis || !yAxis)
        continue;
      
      CoordinateSystemSliceBuilder sb(cdm, cs);
      sb.setStartAndSize(xAxis,0,1);
      sb.setStartAndSize(yAxis,0,1);
      inventoryExtractGrid(grids, cs, xAxis, yAxis);

      if (CoordinateAxis_cp vAxis = cs->getGeoZAxis()) {
        sb.setStartAndSize(vAxis,0,1);
        inventoryExtractVAxis(zaxes, name2id, vAxis, cs);
      }

      // find time axis
      if (CoordinateAxis_cp tAxis = cs->getTimeAxis()) {
        METLIBS_LOG_DEBUG("found time axis " << tAxis->getName());
        if (cs->hasAxisType(CoordinateAxis::ReferenceTime)) { //has reference time axis
          CoordinateAxis_cp rtAxis = cs->findAxisOfType(CoordinateAxis::ReferenceTime);
          DataPtr refTimes = feltReader->getScaledDataInUnit(rtAxis->getName(), SECONDS_SINCE_1970);
          MetNoFimex::shared_array<int> refdata = refTimes->asInt();
          METLIBS_LOG_DEBUG(LOGVAL(refTimes->size()));
          //if one time step per refTime, use refTime as time
          std::vector<double> values;
          for (size_t i = 0; i < refTimes->size(); ++i) {
            referenceTimes.push_back(isoFromTimeT(refdata[i]));
            
            sb.setReferenceTimePos(i);
            DataPtr times_offset = feltReader->getScaledDataSliceInUnit(tAxis->getName(),
                SECONDS_SINCE_1970, sb.getTimeVariableSliceBuilder());
            MetNoFimex::shared_array<int> timeOffsetData = times_offset->asInt();

            METLIBS_LOG_DEBUG(LOGVAL(times_offset->size()) << LOGVAL(singleTimeStep));
            values.insert(values.end(), &timeOffsetData[0], &timeOffsetData[0] + times_offset->size());

            if (!singleTimeStep) {
              addTimeAxis(tAxis, values, referenceTimes[i], inventory);
              values.clear();
            }
          }

          if (singleTimeStep && !referenceTimes.empty())
            addTimeAxis(tAxis, values, referenceTimes[0], inventory);
          
          sb.setTimeStartAndSize(0,1);

        } else {
          // time axis but no reference time axis
          METLIBS_LOG_DEBUG("no reftime axis found for time axis '" << tAxis->getName() << "'");
          sb.setStartAndSize(tAxis, 0, 1);

          gridinventory::Taxis invt(tAxis->getName(), std::vector<double>());
          if (taxes.find(invt) == taxes.end()) {
            // time axis recognized but not found yet
            DataPtr tdata = feltReader->getScaledDataInUnit(tAxis->getName(), "days since 0-01-01 00:00:00");
            MetNoFimex::shared_array<double> timedata = tdata->asDouble();
            METLIBS_LOG_DEBUG(LOGVAL(tdata->size()));

            if (tdata->size() && timedata[0] > 365) {
              noOfClimateTimes = 0;
              tdata = feltReader->getScaledDataInUnit(tAxis->getName(), SECONDS_SINCE_1970);
              timedata = tdata->asDouble();
              invt.values.insert(invt.values.end(), &timedata[0], &timedata[0] + tdata->size());

            } else {
              noOfClimateTimes = tdata->size();
              const boost::posix_time::ptime t0_unix = boost::posix_time::time_from_string("1970-01-01 00:00");
              boost::posix_time::ptime pt = boost::posix_time::time_from_string("1902-01-01 00:00:00");
              boost::gregorian::years year(1);
              for (size_t j = 0; j < 135; j++) {
                pt += year;
                for (size_t i = 0; i < tdata->size(); ++i) {
                  boost::gregorian::days dd(timedata[i]);
                  boost::posix_time::ptime pt2 = pt + dd;
                  int seconds = (pt2 - t0_unix).total_seconds();
                  invt.values.push_back(seconds);
                }
              }
            }
            taxes.insert(invt);

            // if no reference Time axis, get unique refTime
            if (referenceTime.empty() && invt.values.size() > 1)
              referenceTime = fallbackGetReferenceTime();

            // if no reference Time, use first time value
            if (referenceTime.empty() && invt.values.size() > 1 && noOfClimateTimes == 0)
              referenceTime = isoFromTimeT(invt.values.front());
          }
        }
      }

      // ReferenceTime; if no reference Time axis, get uniqe refTime
      METLIBS_LOG_DEBUG(LOGVAL(referenceTime));
      if (referenceTime.empty())
        referenceTime = fallbackGetReferenceTime();

      // find extra axes
      const std::vector<std::string>& unsetList = sb.getUnsetDimensionNames();
      if (!unsetList.empty()) {
        METLIBS_LOG_DEBUG(LOGVAL(unsetList.size()));
        inventoryExtractExtraAxes(extraaxes, name2id, unsetList, cs->getAxes());
      }
    }
    METLIBS_LOG_DEBUG("Coordinate Systems Loop FINISHED");

    const std::vector<CDMVariable>& variables = cdm.getVariables();
    const std::vector<CDMDimension>& dimensions = cdm.getDimensions();

    // find all dimension variable names
    std::set<std::string> dimensionnames;
    for (const CDMDimension& dim : dimensions) {
      dimensionnames.insert(dim.getName());
    }

    // Loop through all non-dimensional variables
    for (const auto& var : variables) {
      const std::string& varName = var.getName();
      if (dimensionnames.find(varName) != dimensionnames.end()) {
        continue;
      }
      METLIBS_LOG_DEBUG("NEXT VAR: " << varName);

      // search for coordinate system for varName
      std::string zaxisname, taxisname, eaxisname;
      std::string grid_id; // must be constructed as in 'inventoryExtractGrid'
      if (CoordinateSystem_cp cs = findCompleteCoordinateSystemFor(coordSys, varName)) {
        std::string xaxisname, yaxisname;
        copyAxisName(cs->getTimeAxis(), taxisname);
        copyAxisName(cs->getGeoXAxis(), xaxisname);
        copyAxisName(cs->getGeoYAxis(), yaxisname);
        copyAxisName(cs->getGeoZAxis(), zaxisname);
        grid_id = xaxisname + yaxisname;

        if (MetNoFimex::Projection_cp projection = cs->getProjection()) {
          grid_id += "_";
          grid_id += projection->getName();
        }
        if (grid_id == "_")
          grid_id = "grid";
      }

      // extraAxis -- Each parameter may only have one extraAxis
      const std::vector<std::string>& shape = cdm.getVariable(varName).getShape();
      for (const std::string& dim : shape) {
        for (const gridinventory::ExtraAxis& eaxis : extraaxes) {
          if (eaxis.name == dim) {
            eaxisname = dim;
            break;
          }
        }
      }

      gridinventory::GridParameterKey paramkey(varName, zaxisname, taxisname, eaxisname);
      gridinventory::GridParameter param(paramkey);
      param.nativename = varName;
      param.grid = grid_id;
      CDMAttribute attr;
      if (cdm.getAttribute(varName, "standard_name", attr)) {
        param.standard_name  = attr.getStringValue();
      }
      if (cdm.getAttribute(varName, "long_name", attr)) {
        param.long_name  = attr.getStringValue();
      }
      //add axis/grid-id - not a part of key, but used to find times, levels, etc
      param.zaxis_id = name2id[zaxisname];
      param.taxis_id = taxisname;
      param.extraaxis_id = name2id[eaxisname];
      //unit
      param.unit = cdm.getUnits(varName);

      parameters.insert(param);
    }

    gridinventory::Inventory::reftimes_t& rtimes = inventory.reftimes;
    if(rtimes.empty()) {
      gridinventory::ReftimeInventory reftime(referenceTime);
      reftime.taxes = taxes;
      rtimes[reftime.referencetime] = reftime;
    }

    //get global attributes
    std::map<std::string,std::string> globalAttributes;
    const std::vector<CDMAttribute> attributes = cdm.getAttributes(cdm.globalAttributeNS());
    for (const CDMAttribute& a : attributes) {
      globalAttributes[a.getName()] = a.getStringValue();
    }

    //loop throug reftimeinv
    for (auto& r_i : rtimes) {
      gridinventory::ReftimeInventory& ri = r_i.second;
      ri.parameters = parameters;
      ri.grids = grids;
      ri.zaxes = zaxes;
      diutil::insert_all(ri.taxes, taxes);
      ri.extraaxes = extraaxes;
      ri.globalAttributes = globalAttributes;
    }

    //METLIBS_LOG_DEBUG(LOGVAL(inventory));
  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
    return false;
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
    return false;
  }

  sourceOk = true;
  return true;
}

CoordinateSystemSliceBuilder FimexIO::createSliceBuilder(CDMReader_p reader, const CoordinateSystem_cp& varCS, const std::string& reftime,
    const gridinventory::GridParameter& param, int taxis_index, int zaxis_index, int eaxis_index)
{
  // create a slice-builder for the variable;
  // the slicebuilder starts with the maximum variable size;
  // as we want the complete field, we leave the x- and y-axis alone
  CoordinateSystemSliceBuilder sb(reader->getCDM(), varCS);

  // handling of time axis
  if (CoordinateAxis_cp tAxis = varCS->getTimeAxis()) {
    // time-Axis, eventually multi-dimensional, i.e. forecast_reference_time
    if (CoordinateAxis_cp rtAxis = varCS->findAxisOfType(CoordinateAxis::ReferenceTime)) {
      DataPtr refTimes = reader->getScaledDataInUnit(rtAxis->getName(), SECONDS_SINCE_1970);
      MetNoFimex::shared_array<int> refdata = refTimes->asInt();
      // FIXME instead of converting each time from int to string, convert 'reftime' to int
      for (size_t i = 0; i < refTimes->size(); ++i) {
        if (reftime == isoFromTimeT(refdata[i])) {
          sb.setReferenceTimePos(i);
          break;
        }
      }
    }
    sb.setTimeStartAndSize(taxis_index, 1);
  }

  // find and select zaxis
  if (CoordinateAxis_cp vAxis = varCS->getGeoZAxis())
    sb.setStartAndSize(vAxis, zaxis_index, 1);

  // find and select extraaxis, unless empty
  if (!param.key.extraaxis.empty()) {
    sb.setStartAndSize(param.key.extraaxis, eaxis_index, 1);
  }

  return sb;
}

CoordinateSystemSliceBuilder FimexIO::createSliceBuilder(const CoordinateSystem_cp& varCS,
    const std::string& reftime, const gridinventory::GridParameter& param,
    int taxis_index, int zaxis_index, int eaxis_index)
{
  return createSliceBuilder(feltReader, varCS, reftime, param, taxis_index, zaxis_index, eaxis_index);
}

bool FimexIO::paramExists(const std::string& reftime, const gridinventory::GridParameter& param)
{
  using namespace gridinventory;
  const std::map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  if (ritr == inventory.reftimes.end())
    return false;
  const ReftimeInventory& reftimeInv = ritr->second;
  return (reftimeInv.parameters.find(param) != reftimeInv.parameters.end());
}

void FimexIO::setHybridParametersIfPresent(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& ap_name,
                                           const std::string& b_name, size_t zaxis_index, Field_p field)
{
  // check if the zaxis has the hybrid parameters ap and b
  gridinventory::GridParameter param_ap(gridinventory::GridParameterKey(ap_name, param.key.zaxis,"", ""));
  gridinventory::GridParameter param_b(gridinventory::GridParameterKey(b_name, param.key.zaxis, "", ""));
  if (paramExists(reftime, param_ap) && paramExists(reftime, param_b)) {
    DataPtr ap = feltReader->getScaledDataInUnit(ap_name, "hPa");
    DataPtr b  = feltReader->getScaledData(b_name);
    if (ap && b && zaxis_index < ap->size() && zaxis_index < b->size()) {
      field->aHybrid = ap->getDouble(zaxis_index);
      field->bHybrid = b ->getDouble(zaxis_index);
    }
  } else {
    //        METLIBS_LOG_DEBUG("no hybrid params");
  }
}

void FimexIO::copyFieldSwapY(const bool no_swap_y, const int nx, const int ny, const float* fdataSrc, float* fdataDst)
{
  if (no_swap_y) {
    std::copy(fdataSrc, fdataSrc + (nx * ny), fdataDst);
  } else {
    for (int j = 0; j < ny / 2 + 1; j++) {
      int i1 = j * nx;
      int i2 = (ny - 1 - j) * nx;
      for (int i = 0; i < nx; i++, i1++, i2++) {
        fdataDst[i1] = fdataSrc[i2];
        fdataDst[i2] = fdataSrc[i1];
      }
    }
  }
}

const std::string& FimexIO::extractVariableName(const gridinventory::GridParameter& param)
{
  // internal fimex name for this parameter
  return param.nativename; //mapping["param"];
}

// find an appropriate coordinate system for the variable
MetNoFimex::CoordinateSystem_cp FimexIO::findCoordinateSystem(const gridinventory::GridParameter& param)
{
  return ::findCoordinateSystem(coordSys, extractVariableName(param));
}

/**
 * Get data slice
 */
Field_p FimexIO::getData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                         const std::string& elevel)
{
  METLIBS_LOG_TIME();
  std::string timestr;
  if ( time.undef() ) {
    timestr = "-";
  } else {
     timestr = time.isoTime();
  }
  METLIBS_LOG_INFO(" Param: "<<param.key.name<<"  time:"<<timestr<<" Source:"<<source_name<<"  : "<<config_filename);
  METLIBS_LOG_DEBUG(LOGVAL(reftime) << LOGVAL(level) << LOGVAL(elevel));

  //todo: if time, level, elevel etc not found, return NULL

  Field_p field = initializeField(model_name, reftime, param, level, time, elevel);
  if (!field) {
    METLIBS_LOG_DEBUG( " initializeField returned NULL");
    return nullptr;
  }

  try {
    CoordinateSystem_cp varCS = findCoordinateSystem(param);
    if (!varCS)
      return nullptr;

    const int taxis_index = findTimeIndex(getTaxis(reftime, param.key.taxis), time);
    const int zaxis_index = findZIndex(getZaxis(reftime, param.key.zaxis), level);
    const int eaxis_index = findExtraIndex(getExtraAxis(reftime, param.key.extraaxis), elevel);
    if (taxis_index < 0 || (!param.key.zaxis.empty() && zaxis_index < 0) || (!param.key.extraaxis.empty() && eaxis_index < 0))
      return nullptr;
    const CoordinateSystemSliceBuilder sb = createSliceBuilder(varCS, reftime, param, taxis_index, zaxis_index, eaxis_index);

    // fetch the data
    const std::string& varName = extractVariableName(param);
    const DataPtr data = feltReader->getScaledDataSlice(varName, sb);
    field->unit = feltReader->getCDM().getUnits(varName);

    const size_t dataSize = data->size(), fieldSize = field->area.gridSize();
    if (dataSize != fieldSize) {
      METLIBS_LOG_DEBUG("getDataSlice returned " << dataSize << " datapoints, but nx*ny =" << fieldSize );
      field->fill(miutil::UNDEF);
    } else {
      MetNoFimex::shared_array<float> fdata = data->asFloat();
      mifi_nanf2bad(&fdata[0], &fdata[0]+dataSize, fieldUndef);

      const gridinventory::Grid& grid = getGrid(reftime, param.grid);
      copyFieldSwapY(grid.y_direction_up, field->area.nx, field->area.ny, &fdata[0], field->data);

      field->checkDefined();
    }

    // get a-hybrid and b-hybrid (used to calculate pressure of hybrid levels)
    if (const VerticalTransformation_cp vtran = varCS->getVerticalTransformation()) {
      if (vtran->getName() == HybridSigmaPressure1::NAME()) {
        if (std::shared_ptr<const HybridSigmaPressure1> hyb1 = std::dynamic_pointer_cast<const HybridSigmaPressure1>(vtran)) {
          setHybridParametersIfPresent(reftime, param, hyb1->ap, hyb1->b, zaxis_index, field);
        }
      }
    }

    //Some data sets have defined the wave direction in the "opposite" direction (ecwam)
    field->turnWaveDirection = turnWaveDirection;
    field->vectorProjectionLonLat = vectorProjectionLonLat;

    return field;
  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
  }
  return nullptr;
}

/**
 * Get data
 */
vcross::Values_p  FimexIO::getVariable(const std::string& varName)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_INFO(LOGVAL(varName));

  try {
    // Get the CDM from the reader
    const CDM& cdm = feltReader->getCDM();

    const std::vector<std::string>& shape = cdm.getVariable(varName).getShape();
    if ( shape.size() != 2 ) {
      METLIBS_LOG_INFO("Only 2-dim varibles supported yet, dim ="<<shape.size());
      return  vcross::Values_p();
    }

    CDMDimension dim1 = cdm.getDimension(shape[0]);
    CDMDimension dim2 = cdm.getDimension(shape[1]);

    const DataPtr data = feltReader->getData(varName);
    MetNoFimex::shared_array<float> fdata = data->asFloat();
    vcross::Values_p p_values = std::make_shared<vcross::Values>(dim1.getLength(),dim2.getLength(),fdata);
    return p_values;

  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
  }

  return  vcross::Values_p();
}

bool FimexIO::putData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                      const std::string& elevel, const std::string& unit, Field_cp field, const std::string& output_time)
{
  METLIBS_LOG_TIME();

  if (!field)
    return false;

  METLIBS_LOG_INFO("Param: "<<param.key.name<<"  time:"<<time<<" Source:"<<source_name);

  CDMReaderWriter_p feltWriter = std::dynamic_pointer_cast<CDMReaderWriter>(feltReader);
  if (!feltWriter) {
    METLIBS_LOG_INFO("No feltWriter");
    return false;
  }

  try {
    CoordinateSystem_cp varCS = findCoordinateSystem(param);
    if (!varCS)
      return false;

    const int taxis_index = findTimeIndex(getTaxis(reftime, param.key.taxis), time);
    const int zaxis_index = findZIndex(getZaxis(reftime, param.key.zaxis), level);
    const int eaxis_index = findExtraIndex(getExtraAxis(reftime, param.key.extraaxis), elevel);
    if (taxis_index < 0 || (!param.key.zaxis.empty() && zaxis_index < 0) || (!param.key.extraaxis.empty() && eaxis_index < 0))
      return false;
    const CoordinateSystemSliceBuilder sb = createSliceBuilder(varCS, reftime, param, taxis_index, zaxis_index, eaxis_index);

    const size_t fieldSize = field->area.gridSize();
    MetNoFimex::shared_array<float> fdata(new float[fieldSize]);

    mifi_bad2nanf(&fdata[0], &fdata[0]+fieldSize, fieldUndef);

    const gridinventory::Grid grid = getGrid(reftime, param.grid);
    copyFieldSwapY(grid.y_direction_up, field->area.nx, field->area.ny, field->data, &fdata[0]);

    //change time and forecast_reference_time to output_time
    //this only works when there is one time step
    if ( !output_time.empty() ) {
      MetNoFimex::shared_array<float> fdata2(new float[1]);
      fdata2[0]=0;
      const DataPtr data2 = MetNoFimex::createData(1,fdata2 );
      std::string time_unit = "seconds since " + output_time + " +00:00";
      feltWriter->putScaledDataSliceInUnit("time", time_unit, 0,data2);
    }

    const DataPtr data = MetNoFimex::createData(fieldSize, fdata);
    const std::string& varName = extractVariableName(param);
    if (unit.empty())
      feltWriter->putScaledDataSlice(varName, sb, data);
    else
      feltWriter->putScaledDataSliceInUnit(varName, unit, sb, data);
    feltWriter->sync();
    return true;
  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not write " << source_name << ", CDMException is: " << cdmex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not write " << source_name << ", exception is: " << ex.what());
  }
  return false;
}
