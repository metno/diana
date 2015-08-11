/*
 * FimexIO.cc
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */

#define HAVE_FELT
#define HAVE_NETCDF
#define HAVE_GRIB
#define HAVE_WDB

#include "FimexIO.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>
#include <puTools/mi_boost_compatibility.hh>
#include <miLogger/LogHandler.h>

#include <fimex/CDM.h>
#include <fimex/CDMFileReaderFactory.h>
#include <fimex/CDMReaderWriter.h>
#include <fimex/Data.h>
#include <fimex/interpolation.h>
#include <fimex/CoordinateSystemSliceBuilder.h>
#include <fimex/Felt_Types.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/vertical_coordinate_transformations.h>
#include <fimex/coordSys/verticalTransform/HybridSigmaPressure1.h>
#include <fimex/coordSys/verticalTransform/Pressure.h>
#include <fimex/coordSys/verticalTransform/Height.h>
#include <fimex/coordSys/verticalTransform/ToVLevelConverter.h>
#include <fimex/Units.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/greg_duration.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_array.hpp>

#include <sstream>

#define MILOGGER_CATEGORY "diField.FimexIO"
#include "miLogger/miLogging.h"

using namespace std;
using namespace miutil;
using namespace MetNoFimex;

namespace {
const char format_metnofieldfile[] = "metnofieldfile";
const char format_netcdf[] = "netcdf";
const char format_ncml[] = "ncml";
const char format_grib[] = "grib";
const char format_wdb[] = "wdb";

const char LONGITUDE[] = "longitude";
const char LATITUDE[]  = "latitude";
const char DEGREES[] = "degrees";
const char METER[] = "m";
const char SECONDS_SINCE_1970[] = "seconds since 1970-01-01 00:00:00";

const char REPROJECTION[] = "reprojection";

DataPtr getScaledDataSlice(FimexIO::CDMReaderPtr reader, const CoordinateSystemSliceBuilder& sb,
    const std::string& varName, const std::string& unit)
{
  if (unit.empty())
    return reader->getScaledDataSlice(varName, sb);
  else
    return reader->getScaledDataSliceInUnit(varName, unit, sb);
}

} // anonymous namespace

// #######################################################################

namespace miutil {
long path_ctime(const std::string& path)
{
  pu_struct_stat buf;
  if (pu_stat(path.c_str(), &buf) != 0)
    return 0;
  return buf.st_ctime;
}
}

// #######################################################################

size_t FimexIO::findTimeIndex(const gridinventory::Taxis& taxis, const miutil::miTime& time)
{
  size_t taxis_index = 0;
  if (not time.undef()) {
    // TODO convert time to std::string in seconds-since-epoch, then use std::find
    vector<double>::const_iterator titr = taxis.values.begin();
    for (; titr != taxis.values.end(); ++titr, taxis_index++) {
      time_t tt = (*titr);
      miTime t(tt);
      if (t == time) {
        break;
      }
    }
    if (noOfClimateTimes)
      taxis_index = taxis_index % noOfClimateTimes;
  }
  return taxis_index;
}

size_t FimexIO::findZIndex(const gridinventory::Zaxis& zaxis, const std::string& zlevel)
{
  size_t zaxis_index = 0;
  // find the index to the correct Z value
  if (not (zlevel.empty() and zaxis.stringvalues.size() == 1)) {
    if (!zaxis.positive) {
      vector<std::string>::const_iterator zitr = zaxis.stringvalues.begin();
      for (; zitr != zaxis.stringvalues.end(); ++zitr, zaxis_index++) {
        if (*zitr == zlevel) {
          break;
        }
      }
    } else {
      vector<std::string>::const_reverse_iterator zitr = zaxis.stringvalues.rbegin();
      for (; zitr != zaxis.stringvalues.rend(); ++zitr, zaxis_index++) {
        if (*zitr == zlevel) {
          break;
        }
      }
    }
  }
  return zaxis_index;
}

size_t FimexIO::findExtraIndex(const gridinventory::ExtraAxis& extraaxis, const std::string& elevel)
{
  size_t extraaxis_index = 0;
  vector<std::string>::const_iterator ritr = extraaxis.stringvalues.begin();
  for (; ritr != extraaxis.stringvalues.end(); ++ritr, extraaxis_index++) {
    if (*ritr == elevel) {
      break;
    }
  }
  return extraaxis_index;
}

// find an appropriate coordinate system for the variable
FimexIO::CoordinateSystemPtr findCoordinateSystem(const FimexIO::CoordinateSystems_t& coordSys, const std::string& varName)
{
  FimexIO::CoordinateSystems_t::const_iterator varSysIt = std::find_if(coordSys.begin(), coordSys.end(),
      CompleteCoordinateSystemForComparator(varName));
  if (varSysIt == coordSys.end()) {
    METLIBS_LOG_INFO("No coordinate system found for '" << varName << '\'' );
    return FimexIO::CoordinateSystemPtr();
  }
  FimexIO::CoordinateSystemPtr varCS = *varSysIt;

  if (not varCS->isSimpleSpatialGridded()) {
    METLIBS_LOG_INFO("Coordinate system for '" << varName << "' is not a simple spatial grid" );
    return FimexIO::CoordinateSystemPtr();
  }

  // check that geographical X/Lon and Y/Lat axes are defined
  if (not varCS->getGeoXAxis() or not varCS->getGeoYAxis()) {
    METLIBS_LOG_INFO("Missing one geographical axis for '" << varName << '\'');
    return FimexIO::CoordinateSystemPtr();
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

FimexIO::FimexIO(const std::string & modelname, const std::string & sourcename,
    const std::string & reftime,
    const std::string & format, const std::string& config,
    const std::vector<std::string>& options, bool makeFeltReader, FimexIOsetup * s) :
  GridIO(sourcename), sourceOk(false), modificationTime(0), model_name(modelname), source_type(format),
  config_filename(config), reftime_from_file(reftime), singleTimeStep(false), noOfClimateTimes(0),
  writeable(false), turnWaveDirection(false), setup(s)
{
  METLIBS_LOG_SCOPE(modelname <<" : "<<sourcename<<" : "<<reftime<<" : "<<format<<" : "<<config);

  for ( size_t i=0; i<options.size(); i++) {
    int pos = options[i].find_first_of('=');
    std::string key = options[i].substr(0,pos);
    boost::algorithm::to_lower(key);
    std::string value = options[i].substr(pos+1);
    if (key == "proj4string") {
      projDef = value;
    } else if (key == "singletimestep") {
      singleTimeStep = true;
    } else if (key == "writeable") {
      boost::algorithm::to_lower(value);
      writeable = (value == "true");
    } else if (key == "turnwavedirection") {
      boost::algorithm::to_lower(value);
      turnWaveDirection = (value == "true");
    } else if (key == "r") {
      reproj_name=value;
    } else {
      METLIBS_LOG_ERROR("unknown option" << LOGVAL(key) << LOGVAL(value));
    }
  }

  if (makeFeltReader) {
    feltReader = createReader();
    reftime_from_file = fallbackGetReferenceTime();
    sourceChanged(true);
  }
  METLIBS_LOG_DEBUG("reftime" << reftime_from_file);
}

FimexIO::~FimexIO()
{
}

/**
 * Returns whether the source has changed since the last makeInventory
 */
bool FimexIO::sourceChanged(bool update)
{
  //Unchecked source, not possible to check
  if ((!update && modificationTime == 0)) {
    return false;
  }

  const long modificationTime_ = path_ctime(source_name);
  if (modificationTime_ != modificationTime) {
    if (update) {
      modificationTime = modificationTime_;
    }
    return true;
  }

  return false;
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
  ostringstream id;
  id << name << "_" << size;
  return id.str();
}

typedef std::map<std::string, std::string> name2id_t;

void addTimeAxis(CoordinateSystem::ConstAxisPtr tAxis, const std::vector<double>& values,
    const std::string& rtime, gridinventory::Inventory& inventory)
{
  const std::string& name = tAxis->getName();
  gridinventory::Taxis taxis = gridinventory::Taxis(name, values);
  gridinventory::ReftimeInventory reftime(rtime);
  reftime.taxes.insert(taxis);
  inventory.reftimes[reftime.referencetime] = reftime;
}
} // namespace anonymous

FimexIO::CDMReaderPtr FimexIO::createReader()
{
  int filetype = mifi_get_filetype(source_type.c_str());

  // support for old diana-names
  if (filetype == MIFI_FILETYPE_UNKNOWN) {
    if (source_type == format_metnofieldfile) {
      filetype = MIFI_FILETYPE_FELT;
    } else if (source_type == format_netcdf) {
      filetype = MIFI_FILETYPE_NETCDF;
    } else if (source_type == format_ncml) {
      filetype = MIFI_FILETYPE_NCML;
    } else if (source_type == format_grib) {
      filetype = MIFI_FILETYPE_GRIB;
    } else if (source_type == format_wdb) {
      filetype = MIFI_FILETYPE_WDB;
    }
  }
  if (filetype == MIFI_FILETYPE_UNKNOWN) {
    METLIBS_LOG_ERROR("unknown source_type:" << source_type);
    return CDMReaderPtr();
  }

  if (writeable)
    filetype |= MIFI_FILETYPE_RW;


  try {

    boost::shared_ptr<CDMReader> feltReader(CDMFileReaderFactory::create(filetype, source_name,config_filename));

    if (!reproj_name.empty()) {
      // get the reprojection data from optionMap
      std::map<std::string, std::map<std::string, std::string> >::iterator reproj_data_it = setup->optionMap.find(reproj_name);
      if (reproj_data_it != setup->optionMap.end()) {
        std::map<std::string, std::string> reproj_data = reproj_data_it->second;
        // Valid keys name, type, projString, xAxisValues, yAxisValues, xAxisUnit, yAxisUnit,
        // method = nearestneighbor, bilinear,bicubic, forward_max, forward_min, forward_mean, forward_median, forward_sum, coord_nearestneighbor, coord_kdtree
        if (reproj_data["type"] == REPROJECTION) {
          boost::shared_ptr<CDMInterpolator> interpolator( new CDMInterpolator(feltReader) );
          int method = mifi_string_to_interpolation_method( reproj_data["method"].c_str() );
          if ( method == MIFI_INTERPOL_UNKNOWN ) {
            /* Parse error, log and use default interpolation type.*/
            METLIBS_LOG_WARN("Invalid interpolation type in setup file: " << reproj_data["method"]);
            METLIBS_LOG_WARN("Using default interpolation type: MIFI_INTERPOL_NEAREST_NEIGHBOR");
            method = MIFI_INTERPOL_NEAREST_NEIGHBOR;
          }
          interpolator->changeProjection(method, reproj_data["projString"], reproj_data["xAxisValues"], reproj_data["yAxisValues"],
              reproj_data["xAxisUnit"], reproj_data["yAxisUnit"]);
          return boost::shared_ptr<CDMReader>(interpolator);
        } else {
          METLIBS_LOG_WARN("Invalid option type in FimexIO::createReader: " << reproj_data["type"]);
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
  return CDMReaderPtr();
}

void FimexIO::inventoryExtractGridProjection(boost::shared_ptr<const MetNoFimex::Projection> projection, gridinventory::Grid& grid,
    CoordinateSystem::ConstAxisPtr xAxis, CoordinateSystem::ConstAxisPtr yAxis)
{
  std::string projStr = projection->getProj4String();

  // replace +proj=utm ... with +proj=tmerc... (need proj that accepts +x_0/+y_0)
  std::string utmstr = "+proj=utm +zone=";
  size_t utmpos = projStr.find(utmstr,0);
  if (utmpos != std::string::npos) {
    const vector<std::string> tokens = miutil::split(projStr, 0, " ");
    for (size_t ii=0; ii<tokens.size(); ++ii) {
      const vector<std::string> stokens = miutil::split(tokens[ii], 0, "=");
      if (stokens.size() == 2 && stokens[0] == "+zone") {
        int lon_0 = (atof(stokens[1].c_str()) - 1)*6 - 180 + 3;
        utmstr += stokens[1];
        ostringstream ost;
        ost <<"+proj=tmerc +lon_0="<<lon_0<<" +lat_0=0 +k=0.9996 +x_0=500000";
        boost::replace_first(projStr,utmstr,ost.str());
        break;
      }
    }
  }
  
  //replace +proj=longlat and +proj=latlong with +proj=ob_tran +o_proj=longlat (need proj that accepts +x_0/+y_
  std::string llstr = "+proj=longlat";
  size_t llpos = projStr.find(llstr,0);
  if (llpos == std::string::npos) {
    llstr = "+proj=latlong";
    llpos = projStr.find(llstr,0);
  }
  if (llpos != std::string::npos) {
    boost::replace_first(projStr,llstr,"+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=90");
  }
  
  // Find and remove explicit false easting/northing, add them later
  const vector<std::string> tokens = miutil::split(projStr, 0, " ");
  for (size_t ii=0; ii<tokens.size(); ++ii) {
    const vector<std::string> stokens = miutil::split(tokens[ii], 0, "=");
    if (stokens.size() == 2 && stokens[0] == "+x_0") {
      grid.x_0 = -1 *atof(stokens[1].c_str());
      const size_t spos = projStr.find(tokens[ii], 0);
      assert(spos != std::string::npos);
      projStr.erase(spos, tokens[ii].length()+1);
    } else if (stokens.size() == 2 && stokens[0] == "+y_0") {
      grid.y_0 = -1 *atof(stokens[1].c_str());
      const size_t spos = projStr.find(tokens[ii], 0);
      assert(spos != std::string::npos);
      projStr.erase(spos, tokens[ii].length()+1);
    }
  }

  const float axis_scale = 1.0; // X/Y-axes are scaled by this
  const std::string xyUnit = projection->isDegree() ? "radian" : METER;
  DataPtr xdata = feltReader->getScaledDataInUnit(xAxis->getName(), xyUnit);
  DataPtr ydata = feltReader->getScaledDataInUnit(yAxis->getName(), xyUnit);
  if (xdata) {
    size_t nx = xdata->size();
    METLIBS_LOG_DEBUG("X-data.size():" << nx);
    if (nx > 1) {
      boost::shared_array<float> fdata = xdata->asFloat();
      grid.nx = nx;
      grid.x_resolution = (fdata[1] - fdata[0]) * axis_scale;
      grid.x_0 += fdata[0] * axis_scale;
      METLIBS_LOG_DEBUG(" x_resolution:" << grid.x_resolution << " x_0:" << grid.x_0);
    }
  }
  if (ydata) {
    size_t ny = ydata->size();
    METLIBS_LOG_DEBUG("Y-data.size():" << ny);
    if (ny > 1) {
      boost::shared_array<float> fdata = ydata->asFloat();
      grid.ny = ny;
      if( fdata[0] > fdata[1] ) {
        grid.y_resolution = (fdata[ny-1] - fdata[ny-2]) * axis_scale;
        grid.y_0 += fdata[ny-1] * axis_scale;
        grid.y_direction_up = false;
      } else {
        grid.y_resolution = (fdata[1] - fdata[0]) * axis_scale;
        grid.y_0 += fdata[0] * axis_scale;
        grid.y_direction_up = true;
      }
      if ( grid.y_resolution < 0 ) {
        grid.y_resolution *= -1;
      }
      METLIBS_LOG_DEBUG(" y_resolution:" << grid.y_resolution << " y_0:" << grid.y_0);
    }
  }
  
  // Finalize grid
  ostringstream ost;
  ost << projStr << " +x_0=" << -grid.x_0 << " +y_0=" << -grid.y_0;
  grid.projection += ost.str();
  
  // if defined, use projDefinition from setup
  if (not projDef.empty()) {
    grid.projection = projDef;
    METLIBS_LOG_DEBUG(" using projection from setup: "<<grid.projection);
  }
}

void FimexIO::inventoryExtractGrid(std::set<gridinventory::Grid>& grids, CoordinateSystemPtr cs,
    CoordinateSystem::ConstAxisPtr xAxis, CoordinateSystem::ConstAxisPtr yAxis)
{
  // The Grid type for this coordinate system
  gridinventory::Grid grid;
  
  std::string projectionName;
  if (cs->hasProjection()) {
    boost::shared_ptr<const MetNoFimex::Projection> projection = cs->getProjection();
    inventoryExtractGridProjection(projection, grid, xAxis, yAxis);
    projectionName = projection->getName();
  }
  
  grid.name = xAxis->getName() + yAxis->getName() + "_" + projectionName;
  if (grid.name == "_") {
    grid.name = "grid";
  }
  
  grid.id = grid.name;
  METLIBS_LOG_DEBUG("Inserting grid:" <<grid.name
      << " [" << grid.projection << "] nx:" << grid.nx << " ny:" << grid.ny);
  grids.insert(grid);
}

void FimexIO::inventoryExtractVAxis(std::set<gridinventory::Zaxis>& zaxes, name2id_t& name2id,
    CoordinateSystem::ConstAxisPtr vAxis, CoordinateSystemPtr& cs)
{
  std::string verticalType;
  if (cs->hasVerticalTransformation()) {
    boost::shared_ptr<const VerticalTransformation> vtran = cs->getVerticalTransformation();
    verticalType = vtran->getName();
  }

  //vertical axis recognized
  DataPtr vdata;
  if( verticalType == "pressure" )
    vdata = feltReader->getScaledDataInUnit(vAxis->getName(),"hPa");
  else
    vdata= feltReader->getScaledData(vAxis->getName());
  if (not (vdata and vdata->size()))
    return;

  CDMAttribute attr;
  bool positive = true;
  if (feltReader->getCDM().getAttribute(vAxis->getName(), "positive", attr)) {
    positive = (attr.getStringValue() == "up");
  }
  
  std::vector<double> levels;
  boost::shared_array<double> idata = vdata->asDouble();
  METLIBS_LOG_DEBUG("vertical data.size():" << vdata->size() << "  name:" << vAxis->getName());

  if (verticalType.empty())
    verticalType = vAxis->getName();

  for (size_t i = 0; i < vdata->size(); ++i) {
    levels.push_back(idata[i]);
  }
  
  const std::string id = makeId(vAxis->getName(), levels.size());
  zaxes.insert(gridinventory::Zaxis(id, vAxis->getName(), positive, levels,verticalType));
  name2id[vAxis->getName()] = id;
}

void FimexIO::inventoryExtractExtraAxes(std::set<gridinventory::ExtraAxis>& extraaxes, name2id_t& name2id,
    const std::vector<std::string>& unsetList, const CoordinateSystem::ConstAxisList axes)
{
  const std::set<std::string> unset(unsetList.begin(), unsetList.end());
  BOOST_FOREACH(CoordinateSystem::ConstAxisPtr axis, axes) {
    const std::string& name = axis->getName();
    if (unset.find(name) == unset.end())
      continue;
    
    DataPtr edata = feltReader->getData(name);
    METLIBS_LOG_DEBUG("extra axis '" << name << "' size " << edata->size());
    
    std::vector<double> elevels;
    boost::shared_array<double> idata = edata->asDouble();
    for (size_t i = 0; i < edata->size(); ++i) {
      elevels.push_back(idata[i]);
    }

    const std::string id = makeId(name, elevels.size());
    name2id[name] = id;
    extraaxes.insert(gridinventory::ExtraAxis(id, name, elevels));
  }
}

std::string FimexIO::fallbackGetReferenceTime()
{
  try {
    if (feltReader) {
      boost::posix_time::ptime refTime = getUniqueForecastReferenceTime(feltReader);
      return boost::posix_time::to_iso_extended_string(refTime);
    }
  } catch (CDMException& ex) {
    // METLIBS_LOG_DEBUG("exception occurred: " << ex.what() );
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

  if (source_type != format_wdb && !sourceChanged(true) && sourceOk)
    return true;

  METLIBS_LOG_INFO("Source:" << source_name <<" : "<<config_filename<<" : " <<reftime_from_file);
  if (not feltReader){
    feltReader = createReader();
    if (not feltReader)
      return false;
  }

  std::string  try_reftime_from_file= fallbackGetReferenceTime();
  if (!try_reftime_from_file.empty()) {
    reftime_from_file = try_reftime_from_file;
  }

  // checking timestamp for previous makeInventory, WDB only. TODO: ask fimex if wdb has canged
  const miutil::miTime now = miTime::nowTime();
  if ( source_type == format_wdb && inventory.reftimes.size() ) {
    const miutil::miTime& ts = inventory.getTimeStamp();
    if (abs(miutil::miTime::secDiff(now, ts)) < 3) { // less than 3 seconds since last invocation
      METLIBS_LOG_INFO("skipping makeInventory:" << source_name <<" : "<<config_filename
          << ", now:" << now << " last run" << ts);
      return true;
    }
  }

  set<gridinventory::GridParameter> parameters;
  set<gridinventory::Grid> grids;
  set<gridinventory::Zaxis> zaxes;
  set<gridinventory::Taxis> taxes;
  set<gridinventory::ExtraAxis> extraaxes;

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

    vector<std::string> referenceTimes;
    std::string referenceTime = reftime_from_file;

    METLIBS_LOG_DEBUG("Coordinate Systems Loop");
    BOOST_FOREACH(CoordinateSystemPtr cs, coordSys) {
      METLIBS_LOG_DEBUG("NEW Coordinate System");
      if (not cs->isSimpleSpatialGridded())
        continue;

      METLIBS_LOG_DEBUG("CS is simple spatial grid");

      // find the geographical axes, returns 0 axes if not found
      CoordinateSystem::ConstAxisPtr xAxis = cs->getGeoXAxis(); // X or Lon
      CoordinateSystem::ConstAxisPtr yAxis = cs->getGeoYAxis(); // Y or Lat
      if (not xAxis or not yAxis)
        continue;
      
      CoordinateSystemSliceBuilder sb(cdm, cs);
      sb.setStartAndSize(xAxis,0,1);
      sb.setStartAndSize(yAxis,0,1);
      
      inventoryExtractGrid(grids, cs, xAxis, yAxis);
      
      CoordinateSystem::ConstAxisPtr vAxis = cs->getGeoZAxis();
      if (vAxis) {
        sb.setStartAndSize(vAxis,0,1);
        inventoryExtractVAxis(zaxes, name2id, vAxis, cs);
      }
      
      // find time axis
      CoordinateSystem::ConstAxisPtr tAxis = cs->getTimeAxis();
      if (tAxis) {
        
        if (cs->hasAxisType(CoordinateAxis::ReferenceTime)) { //has reference time axis
          CoordinateSystem::ConstAxisPtr rtAxis = cs->findAxisOfType(CoordinateAxis::ReferenceTime);
          DataPtr refTimes = feltReader->getScaledDataInUnit(rtAxis->getName(), SECONDS_SINCE_1970);
          boost::shared_array<int> refdata = refTimes->asInt();
          
          METLIBS_LOG_DEBUG("reftime data.size():" << refTimes->size());
          //if one time step per refTime, use refTime as time
          std::vector<double> values;
          for (size_t i = 0; i < refTimes->size(); ++i) {
            referenceTimes.push_back(isoFromTimeT(refdata[i]));
            
            sb.setReferenceTimePos(i);
            DataPtr times_offset = feltReader->getScaledDataSliceInUnit(tAxis->getName(),
                SECONDS_SINCE_1970, sb.getTimeVariableSliceBuilder());
            boost::shared_array<int> timeOffsetData = times_offset->asInt();
            
            METLIBS_LOG_DEBUG("offset data.size():" << times_offset->size() << "singletime:"<<singleTimeStep);
            for (size_t j = 0; j < times_offset->size(); ++j) {
              values.push_back(timeOffsetData[j]);
            }
            
            if (not singleTimeStep) {
              addTimeAxis(tAxis, values, referenceTimes[i], inventory);
              values.clear();
            }
          }
          
          if (singleTimeStep and not referenceTimes.empty())
            addTimeAxis(tAxis, values, referenceTimes[0], inventory);
          
          sb.setTimeStartAndSize(0,1);
          
        } else { // no reference time axis
          
          //time axis recognized
          sb.setStartAndSize(tAxis,0,1);
          
          std::vector<double> values;
          DataPtr tdata = feltReader->getScaledDataInUnit(tAxis->getName(), "days since 0-01-01 00:00:00");
          boost::shared_array<double> timedata = tdata->asDouble();
          METLIBS_LOG_DEBUG("time data.size():" << tdata->size());
          
          if ( tdata->size() && timedata[0] > 365 ) {
            noOfClimateTimes = 0;
            tdata = feltReader->getScaledDataInUnit(tAxis->getName(), SECONDS_SINCE_1970);
            timedata = tdata->asDouble();

            for (size_t i = 0; i < tdata->size(); ++i) {
              values.push_back(timedata[i]);
            }
            
          } else {
            noOfClimateTimes = tdata->size();
            const boost::posix_time::ptime t0_unix =  boost::posix_time::time_from_string("1970-01-01 00:00");
            boost::posix_time::ptime pt = boost::posix_time::time_from_string("1902-01-01 00:00:00");
            boost::gregorian::years year(1);
            for ( size_t j = 0; j < 135; j++) {
              pt += year;
              for (size_t i = 0; i < tdata->size(); ++i) {
                boost::gregorian::days dd(timedata[i]);
                boost::posix_time::ptime pt2 = pt + dd;
                int seconds = (pt2-t0_unix).total_seconds();
                values.push_back(seconds);
              }
            }
          }
          const std::string& name = tAxis->getName();
          gridinventory::Taxis taxis = gridinventory::Taxis(name, values);
          taxes.insert(taxis);
          
          // if no reference Time axis, get unique refTime
          if (referenceTime.empty() && values.size() > 1)
            referenceTime = fallbackGetReferenceTime();
          
          // if no reference Time, use first time value
            if (referenceTime.empty() && values.size() > 1 && !noOfClimateTimes)
              referenceTime = isoFromTimeT(timedata[0]);
        }
      }
      
      // ReferenceTime; if no reference Time axis, get uniqe refTime
      if (referenceTime.empty())
        referenceTime = fallbackGetReferenceTime();

      // find extra axes
      const std::vector<std::string>& unsetList = sb.getUnsetDimensionNames();
      if (not unsetList.empty())
        inventoryExtractExtraAxes(extraaxes, name2id, unsetList, cs->getAxes());
    }
    METLIBS_LOG_DEBUG("Coordinate Systems Loop FINISHED");

    const std::vector<CDMVariable>& variables = cdm.getVariables();
    const std::vector<CDMDimension>& dimensions = cdm.getDimensions();

    // find all dimension variable names
    set<std::string> dimensionnames;
    BOOST_FOREACH(const CDMDimension& dim, dimensions) {
      dimensionnames.insert(dim.getName());
    }

    size_t nvars = variables.size();

    // Loop through all non-dimensional variables
    for (size_t j = 0; j < nvars; ++j) {
      const std::string& CDMparamName = variables[j].getName();
      if (dimensionnames.find(CDMparamName) != dimensionnames.end()) {
        continue;
      }
      METLIBS_LOG_DEBUG("NEXT VAR: " << CDMparamName);

      // search for coordinate system for varName
      vector<boost::shared_ptr<const CoordinateSystem> >::const_iterator varSysIt =
          find_if(coordSys.begin(), coordSys.end(),
              CompleteCoordinateSystemForComparator(CDMparamName));

      std::string xaxisname, yaxisname, taxisname, zaxisname, zaxis_native_name, projectionname;
      if (varSysIt != coordSys.end()) {
        CoordinateSystem::ConstAxisPtr axis = (*varSysIt)->getTimeAxis();
        taxisname = (axis.get() == 0) ? "" : axis->getName();

        axis = (*varSysIt)->getGeoXAxis();
        xaxisname = (axis.get() == 0) ? "" : axis->getName();

        axis = (*varSysIt)->getGeoYAxis();
        yaxisname = (axis.get() == 0) ? "" : axis->getName();

        // If param has zaxis with one level, do not use zaxis in inventory.
        // param.zaxis_native_name is used in getData
        axis = (*varSysIt)->getGeoZAxis();
        if ( axis.get() != 0 ) {
          DataPtr data = feltReader->getScaledData(axis->getName());
          std::string verticalType;
          if ((*varSysIt)->hasVerticalTransformation()) {
            boost::shared_ptr<const VerticalTransformation> vtran = (*varSysIt)->getVerticalTransformation();
            verticalType = vtran->getName();
          }
          if (data && (verticalType !="height" || data->size() > 1) ) {
            zaxisname = axis->getName();
          } else {
            zaxis_native_name = axis->getName();
          }
        }

        boost::shared_ptr<const MetNoFimex::Projection> projection = (*varSysIt)->getProjection();
        projectionname = (projection.get() == 0) ? "" : projection->getName();
      }

      std::string gridname = xaxisname + yaxisname + "_" + projectionname;

      //extraAxis
      //Each parameter may only have one extraAxis
      std::string eaxisname, eaxis_native_name;
      const vector<string>& shape = cdm.getVariable(CDMparamName).getShape();
      BOOST_FOREACH(const std::string& dim, shape) {
        BOOST_FOREACH(const gridinventory::ExtraAxis& eaxis, extraaxes) {
          if (eaxis.name == dim) {
            if (eaxis.values.size() > 1)
              eaxisname = dim;
            else
              eaxis_native_name = dim;
            break;
          }
        }
      }

      const std::string& paramname = CDMparamName;
      gridinventory::GridParameterKey paramkey(paramname, zaxisname,
          taxisname, eaxisname, "");
      gridinventory::GridParameter param(paramkey);
      param.nativename = CDMparamName;
      param.grid = gridname;
      CDMAttribute attr;
      if (cdm.getAttribute(CDMparamName, "standard_name", attr)) {
        param.standard_name  = attr.getStringValue();
      }
      if (cdm.getAttribute(CDMparamName, "long_name", attr)) {
        param.long_name  = attr.getStringValue();
      }
      //Name of zaxis/eaxis with one level, used in getData
      param.zaxis_native_name = zaxis_native_name;
      param.eaxis_native_name = eaxis_native_name;
      //add axis/grid-id - not a part of key, but used to find times, levels, etc
      param.zaxis_id = name2id[zaxisname];
      param.taxis_id = taxisname;
      param.extraaxis_id = name2id[eaxisname];
      //unit
      param.unit = cdm.getUnits(CDMparamName);

      parameters.insert(param);
    }

    gridinventory::Inventory::reftimes_t& rtimes = inventory.reftimes;
    if(rtimes.empty()) {
      gridinventory::ReftimeInventory reftime(referenceTime);
      reftime.taxes = taxes;
      rtimes[reftime.referencetime] = reftime;
    }

    //loop throug reftimeinv
    for (gridinventory::Inventory::reftimes_t::iterator it_ri = rtimes.begin(); it_ri != rtimes.end(); ++it_ri) {
      gridinventory::ReftimeInventory& ri = it_ri->second;
      ri.parameters = parameters;
      ri.grids = grids;
      ri.zaxes = zaxes;
      ri.taxes.insert(taxes.begin(), taxes.end());
      ri.extraaxes = extraaxes;
      ri.timestamp = now;
    }

    //METLIBS_LOG_DEBUG(LOGVAL(inventory));
  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
    return false;
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
    return false;
  }

  //METLIBS_LOG_DEBUG(inventory);

  sourceOk = true;
  return true;
}

CoordinateSystemSliceBuilder FimexIO::createSliceBuilder(CDMReaderPtr reader, const CoordinateSystemPtr& varCS,
    const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& zlevel, const miutil::miTime& time, const std::string& elevel, size_t& zaxis_index)
{

  // params with zaxis with one level: param.key.zaxis is empty, use param.zaxis_native_name
  std::string zaxisname;
  if ( param.zaxis_native_name.empty() ) {
    zaxisname = param.key.zaxis;
  } else {
    zaxisname = param.zaxis_native_name;
  }
  // find the taxis, zaxis and extraaxis for this variable
  const gridinventory::Taxis& taxis = getTaxis(reftime, param.key.taxis);
  const gridinventory::Zaxis& zaxis = getZaxis(reftime, zaxisname);
  const gridinventory::ExtraAxis& extraaxis = getExtraAxis(reftime, param.key.extraaxis);

  const size_t taxis_index = findTimeIndex(taxis, time);
  zaxis_index = findZIndex(zaxis, zlevel);
  const size_t extraaxis_index = findExtraIndex(extraaxis, elevel);

  // find vertical and time axes
  CoordinateSystem::ConstAxisPtr vAxis = varCS->getGeoZAxis();
  CoordinateSystem::ConstAxisPtr tAxis = varCS->getTimeAxis();

  // create a slice-builder for the variable
  // the slicebuilder starts with the maximum variable size
  CoordinateSystemSliceBuilder sb(reader->getCDM(), varCS);
  // We want the complete field: leave the x- and y-axis alone
  // handling of time
  if (tAxis.get() != 0) {
    // time-Axis, eventually multi-dimensional, i.e. forecast_reference_time
    if (varCS->hasAxisType(CoordinateAxis::ReferenceTime)) {
      CoordinateSystem::ConstAxisPtr rtAxis = varCS->findAxisOfType(CoordinateAxis::ReferenceTime);
      DataPtr refTimes = reader->getScaledDataInUnit(rtAxis->getName(), SECONDS_SINCE_1970);
      boost::shared_array<int> refdata = refTimes->asInt();
      // FIXME instead of converting each time from int to string, convert 'reftime' to int
      for (size_t i = 0; i < refTimes->size(); ++i) {
        if (reftime == isoFromTimeT(refdata[i])) {
          sb.setReferenceTimePos(i);
          break;
        }
      }
    }
    DataPtr times = feltReader->getDataSlice(tAxis->getName(), sb.getTimeVariableSliceBuilder());
    sb.setTimeStartAndSize(taxis_index, 1);
  }
  // select the vertical layer
  sb.setStartAndSize(vAxis, zaxis_index, 1); // should not fail, though vAxis is undefined
  if (!param.key.extraaxis.empty()) {
    sb.setStartAndSize(param.key.extraaxis, extraaxis_index, 1);
  }

  return sb;
}

CoordinateSystemSliceBuilder FimexIO::createSliceBuilder(const CoordinateSystemPtr& varCS,
    const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& level, const miutil::miTime& time, const std::string& elevel, size_t& zaxis_index)
{
  return createSliceBuilder(feltReader, varCS, reftime, param, level, time, elevel, zaxis_index);
}


bool FimexIO::paramExists(const std::string& reftime, const gridinventory::GridParameter& param)
{
  using namespace gridinventory;
    const map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
    if (ritr == inventory.reftimes.end())
      return false;
    const ReftimeInventory& reftimeInv = ritr->second;
    return ( reftimeInv.parameters.find(param) != reftimeInv.parameters.end() );
}

void FimexIO::setHybridParametersIfPresent(const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& ap_name, const std::string& b_name, size_t zaxis_index, Field* field)
{
  // check if the zaxis has the hybrid parameters ap and b
  gridinventory::GridParameter pp;
  miutil::miTime actualtime;
  gridinventory::GridParameter param_ap(gridinventory::GridParameterKey(ap_name, param.key.zaxis,
      param.key.taxis, param.key.extraaxis, param.key.version));
  gridinventory::GridParameter param_b(gridinventory::GridParameterKey(b_name, param.key.zaxis,
      param.key.taxis, param.key.extraaxis, param.key.version));
  if ( paramExists(reftime,param_ap) && paramExists(reftime,param_b) ) {
    DataPtr ap = feltReader->getScaledDataInUnit(ap_name, "hPa");
    DataPtr b  = feltReader->getScaledData(b_name);
    if (zaxis_index < ap->size() && zaxis_index < b->size()) {
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
FimexIO::CoordinateSystemPtr FimexIO::findCoordinateSystem(const gridinventory::GridParameter& param)
{
  return ::findCoordinateSystem(coordSys, extractVariableName(param));
}

/**
 * Get data slice
 */
Field* FimexIO::getData(const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& level, const miutil::miTime& time,
    const std::string& elevel, const std::string& unit)
{
  METLIBS_LOG_TIME();
  std::string timestr;
  if ( time.undef() ) {
    timestr = "-";
  } else {
     timestr = time.isoTime();
  }
  METLIBS_LOG_INFO(" Param: "<<param.key.name<<"  time:"<<timestr<<" Source:"<<source_name<<"  : "<<config_filename);
  METLIBS_LOG_DEBUG(" reftime: "<<reftime<<"  level:"<<level<<" elevel:"<<elevel<<"  unit: "<<unit);

  //todo: if time, level, elevel etc not found, return NULL

  Field* field = initializeField(model_name, reftime, param, level, time, elevel, unit);
  if (not field) {
    METLIBS_LOG_DEBUG( " initializeField returned NULL");
    return 0;
  }

  try {
    CoordinateSystemPtr varCS = findCoordinateSystem(param);
    if (not varCS)
      return 0;

    size_t zaxis_index;
    const CoordinateSystemSliceBuilder sb = createSliceBuilder(varCS, reftime, param, level, time, elevel, zaxis_index);

    // fetch the data
    const std::string& varName = extractVariableName(param);
    const DataPtr data = getScaledDataSlice(feltReader, sb, varName, unit);

    const size_t dataSize = data->size(), fieldSize = field->area.gridSize();
    if (dataSize != fieldSize) {
      METLIBS_LOG_DEBUG("getDataSlice returned " << dataSize << " datapoints, but nx*ny =" << fieldSize );
      std::fill(field->data, field->data+fieldSize, fieldUndef);
      field->allDefined = false;
    } else {
      boost::shared_array<float> fdata = data->asFloat();
      mifi_nanf2bad(&fdata[0], &fdata[0]+dataSize, fieldUndef);

      const gridinventory::Grid& grid = getGrid(reftime, param.grid);
      copyFieldSwapY(grid.y_direction_up, field->area.nx, field->area.ny, &fdata[0], field->data);

      // check undef
      size_t i = 0;
      while (i<dataSize && field->data[i]<fieldUndef)
        i++;
      field->allDefined = (i==dataSize);
    }

    // get a-hybrid and b-hybrid (used to calculate pressure of hybrid levels)
    std::vector<CoordinateSystemPtr>::iterator varSysIt =
        find_if(coordSys.begin(), coordSys.end(), CompleteCoordinateSystemForComparator(param.key.name));
    if (varSysIt != coordSys.end()) {
      if ((*varSysIt)->hasVerticalTransformation()) {
        boost::shared_ptr<const VerticalTransformation> vtran = (*varSysIt)->getVerticalTransformation();
        if (vtran->getName() == HybridSigmaPressure1::NAME()) {
          boost::shared_ptr<const HybridSigmaPressure1> hyb1 = boost::dynamic_pointer_cast<const HybridSigmaPressure1>(vtran);
          if (hyb1) {
            setHybridParametersIfPresent(reftime, param, hyb1->ap, hyb1->b, zaxis_index, field);
          }
        }
      }
    }

    //Some data sets have defined the wave direction in the "opposite" direction (ecwam)
    field->turnWaveDirection = turnWaveDirection;

    return field;
  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
  }
  return 0;
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

    const vector<string>& shape = cdm.getVariable(varName).getShape();
    if ( shape.size() != 2 ) {
      METLIBS_LOG_INFO("Only 2-dim varibles supported yet, dim ="<<shape.size());
      return  vcross::Values_p();
    }

    CDMDimension dim1 = cdm.getDimension(shape[0]);
    CDMDimension dim2 = cdm.getDimension(shape[1]);

    const DataPtr data = feltReader->getData(varName);
    boost::shared_array<float> fdata = data->asFloat();
    vcross::Values_p p_values = boost::make_shared<vcross::Values>(dim1.getLength(),dim2.getLength(),fdata);
    return p_values;

  } catch (CDMException& cdmex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", CDMException is: " << cdmex.what());
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Could not open or process " << source_name << ", exception is: " << ex.what());
  }

  return  vcross::Values_p();
}

bool FimexIO::putData(const std::string& reftime, const gridinventory::GridParameter& param,
    const std::string& level, const miutil::miTime& time,
    const std::string& elevel, const std::string& unit, const Field* field,
    const std::string& output_time)
{
  if (not field)
    return false;

  METLIBS_LOG_TIME();
  METLIBS_LOG_INFO(" Param: "<<param.key.name<<"  output_time:"<<output_time<<" Source:"<<source_name);

  boost::shared_ptr<CDMReaderWriter> feltWriter = boost::dynamic_pointer_cast<CDMReaderWriter>(feltReader);
  if (not feltWriter) {
    METLIBS_LOG_INFO(" No feltWriter");
    return false;
  }

  try {
    CoordinateSystemPtr varCS = findCoordinateSystem(param);
    if (not varCS)
      return false;

    size_t zaxis_index;
    const CoordinateSystemSliceBuilder sb = createSliceBuilder(varCS, reftime, param, level, time, elevel, zaxis_index);

    const size_t fieldSize = field->area.gridSize();
    boost::shared_array<float> fdata(new float[fieldSize]);

    mifi_bad2nanf(&fdata[0], &fdata[0]+fieldSize, fieldUndef);

    const gridinventory::Grid grid = getGrid(reftime, param.grid);
    copyFieldSwapY(grid.y_direction_up, field->area.nx, field->area.ny, field->data, &fdata[0]);

    //change time and forecast_reference_time to output_time
    //this only works when there is one time step
    if ( !output_time.empty() ) {
      boost::shared_array<float> fdata2(new float[1]);
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
