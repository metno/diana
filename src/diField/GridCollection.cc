/*
 * GridCollection.cc
 *
 *  Created on: Mar 15, 2010
 *      Author: audunc
 */

#include "GridCollection.h"
#include "GridIO.h"
#ifdef FIMEX
#include "FimexIO.h"
#endif
#include "diFieldFunctions.h"
#include "../diUtilities.h"

#include <puCtools/puCglob.h>
#include <puTools/miTime.h>
#include <puTools/miString.h>
#include <puTools/TimeFilter.h>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm/find_if.hpp>

#define MILOGGER_CATEGORY "diField.GridCollection"
#include "miLogger/miLogging.h"

using namespace std;
using namespace gridinventory;

// static class members
GridConverter GridCollection::gc;    // Projection-converter

GridCollection::GridCollection()
: gridsetup(0)
{
}

// initialize collection from a list of sources
bool GridCollection::setContents(const std::string& type,
    const std::string& name, const vector<std::string>& filenames,
    const vector<std::string>& format,
    const vector<std::string>& config,
    const vector<std::string>& option,
    GridIOsetup* setup,
    bool validTimeFromFilename)
{
  sourcetype = type;
  collectionname = name;
  gridsetup = setup;

  formats = format;
  configs = config;
  options = option;

  rawsources = filenames;
  timeFromFilename = validTimeFromFilename;

  // clear GridIO instances
  clearGridSources();

  if (gridsetup == 0){
    METLIBS_LOG_ERROR("setContents: gridsetup is empty");
    return false;
  }
  //return makeGridIOinstances();
  return true;
}

GridCollection::~GridCollection()
{
  clearGridSources();
}

// clear the gridsources vector
void GridCollection::clearGridSources()
{
  diutil::delete_all_and_clear(gridsources);
  gridsourcesTimeMap.clear();
  inventoryOK.clear();
}

void GridCollection::updateGridSources()
{
}

// unpack the raw sources and make one or more GridIO instances
bool GridCollection::makeGridIOinstances()
{
  METLIBS_LOG_TIME();

  clearGridSources();

  // do not allow empty setup
  if (gridsetup == 0) {
    METLIBS_LOG_ERROR("gridsetup is empty");
    return false;
  }

  bool ok = true;
  sources.clear();
  refTimes.clear();
  sources_with_wildcards.clear();

  // unpack each raw source - creating one or more GridIO instances from each
  int index = -1;
  for (std::string sourcestr : rawsources) {
    ++index;

    std::set<std::string> tmpsources;

    // init time filter and replace yyyy etc. with ????
    const miutil::TimeFilter tf(sourcestr);

    // check for wild cards - expand filenames if necessary
    if (sourcestr.find_first_of("*?") != sourcestr.npos && sourcestr.find("glob:") == sourcestr.npos) {
      sources_with_wildcards.push_back(sourcestr);
      const diutil::string_v files = diutil::glob(sourcestr, GLOB_BRACE);
      if( !files.size() ) {
        METLIBS_LOG_INFO("No source available for "<<sourcestr);
        continue;
      }
      tmpsources.insert(files.begin(), files.end());
    } else {
      tmpsources.insert(sourcestr);
    }
    sources.insert(tmpsources.begin(),tmpsources.end());

    //if #formats == #rawsources, use corresponding files. If not use first format
    std::string format;
    if ( rawsources.size() == formats.size() ) {
      format = formats[index];
    } else if ( formats.size() > 0) {
      format = formats[0];
    }

    //if #configs == #rawsources, use corresponding files. If not use first config
    std::string config;
    if ( rawsources.size() == configs.size() ) {
      config = configs[index];
    } else if ( configs.size() > 0) {
      config = configs[0];
    }


    // loop through sources (filenames)
    for (const std::string& sourcename : tmpsources) {

      //Find time from filename if possible
      std::string reftime_from_filename;
      miutil::miTime time;
      bool makeFeltReader = true;
      if (tf.getTime(sourcename,time)) {
        makeFeltReader = false;
        if ( timeFromFilename ) {
          timesFromFilename.insert(time);
        } else {
          reftime_from_filename = time.isoTime("T");
        }
      }

      // new source - add them according to the source type
      GridIO* gp = 0;
#ifdef FIMEX
      if (sourcetype == FimexIO::getSourceType()) {
        gp = new FimexIO(collectionname, sourcename, reftime_from_filename, format, config,
            options, makeFeltReader, static_cast<FimexIOsetup*> (gridsetup));
      }
#endif
      if ( gp ) {
        gridsources.push_back(gp);
        if ( timeFromFilename ) {
          gridsourcesTimeMap[time]=gp;
        }
      } else {
        METLIBS_LOG_ERROR("unknown type:" << sourcetype << " for source:" << sourcename);
        ok = false;
      }
    }
  }

  if( !sources.size() ) {
    METLIBS_LOG_WARN("No sources available for "<<collectionname);
    return false;
  }
  return ok;
}

bool GridCollection::makeInventory(const std::string& refTime,
    bool updateSourceList)
{
  METLIBS_LOG_TIME();

  bool ok = true;
  // decide if we should make new GridIO instances
  if (gridsources.empty() || updateSourceList) {
    ok = makeGridIOinstances();
  }
  // make inventory on all sources and make a combined inventory for the whole collection
  inventoryOK.clear();
  inventory.clear();

  for(gridsources_t::const_iterator it_io=gridsources.begin(); it_io!=gridsources.end(); ++it_io) {
    // enforce the reference time limits
    //    (*itr)->setReferencetimeLimits(limit_min, limit_max); // not used yet

    // make inventory on each GridIO
    bool result = false;

    // make referencetime inventory from referencetime given in filename, if possible
    if (refTime.empty()) {
      const std::string reftime_from_filename = (*it_io)->getReferenceTime();
      if (!reftime_from_filename.empty()) {
        result = true;
        refTimes.insert(reftime_from_filename);
      }
    }

    if (!result && (*it_io)->referenceTimeOK(refTime)) {
      result = (*it_io)->makeInventory(refTime);
      METLIBS_LOG_DEBUG(LOGVAL(refTime));
      inventoryOK[refTime] = result;
      if (result)
        inventory = inventory.merge((*it_io)->getInventory());
    }
    ok |= result;

    //When using time from filename there is no need to make inventory for more than one source now
    if ( timeFromFilename )
      break;
  }

  if (not ok)
    METLIBS_LOG_WARN("makeInventory failed for GridIO with source:" << refTime);
  if(inventory.reftimes.size())
    addComputedParameters();
  return ok;
}


bool GridCollection::sourcesChanged()
{
  for(gridsources_t::const_iterator it_io=gridsources.begin(); it_io!=gridsources.end(); ++it_io)
    if ((*it_io)->sourceChanged(false))
      return true;
  return false;
}

std::vector<gridinventory::Inventory> GridCollection::getInventories() const
{
  std::vector<gridinventory::Inventory> invs;
  for(gridsources_t::const_iterator it_io=gridsources.begin(); it_io!=gridsources.end(); ++it_io)
    invs.push_back((*it_io)->getInventory());
  return invs;
}

/**
 * Get the grid
 */
gridinventory::Grid GridCollection::getGrids() const
{
  METLIBS_LOG_SCOPE();

  gridinventory::Grid grid;
  Inventory::reftimes_t::const_iterator it_r = inventory.reftimes.begin();
  if ( it_r != inventory.reftimes.end() ) {
    if (not it_r->second.grids.empty()) {
      grid = *it_r->second.grids.begin();
    }
  }
  return grid;
}

/**
 * Get the reference times
 */
std::set<std::string> GridCollection::getReferenceTimes() const
{
  if (not refTimes.empty())
    return refTimes;
  std::set<std::string> reftimes;
  const Inventory::reftimes_t& m_reftimes = inventory.reftimes;
  for(Inventory::reftimes_t::const_iterator it_r = m_reftimes.begin(); it_r != m_reftimes.end(); ++it_r) {
    reftimes.insert(it_r->first);
  }
  return reftimes;
}

/**
 * Check if data exists
 */
bool GridCollection::dataExists(const std::string& reftime, const std::string& paramname,
    gridinventory::GridParameter& param)
{
  METLIBS_LOG_SCOPE("searching for: " <<LOGVAL(paramname));
  for (GridIO* io : gridsources) {
    if (dataExists_reftime(io->getReftimeInventory(reftime), paramname, param)) {
      return true;
    }
  }
  METLIBS_LOG_DEBUG("SEARCHING IN COMPUTED:");
  return dataExists_reftime(computed_inventory, paramname, param);

}

/**
 * Get data slice
 */
Field * GridCollection::getData(const std::string& reftime, const std::string& paramname,
    const std::string& zaxis,
    const std::string& taxis, const std::string& extraaxis,
    const std::string& level,
    const miutil::miTime& time, const std::string& elevel,
    const std::string& unit,
    const int & time_tolerance)
{
  METLIBS_LOG_SCOPE(reftime << " | " << paramname << " | " << zaxis
      << " | " << taxis << " | " << extraaxis << " | "
      << level << " | " << time << " | " << elevel << "|" << time_tolerance);

  miutil::miTime  actualtime;

  if( timeFromFilename ){
    getActualTime(reftime, paramname, time, time_tolerance, actualtime);
        METLIBS_LOG_DEBUG(LOGVAL(actualtime));
    std::map<miutil::miTime,GridIO*>::const_iterator ip = gridsourcesTimeMap.find(actualtime);
    if ( ip != gridsourcesTimeMap.end()) {
      ip->second->makeInventory(reftime);
      gridinventory::GridParameter param;
      if (dataExists_reftime(ip->second->getReftimeInventory(reftime), paramname, param))
      {
        Field* f = ip->second->getData(reftime, param, level, actualtime, elevel, unit);
        if (f!=0)
          f->validFieldTime = time;
        return f;
      }
    }

  } else {
    for (GridIO* io : gridsources) {
      gridinventory::GridParameter param;
      if (dataExists_reftime(io->getReftimeInventory(reftime), paramname, param)
          && (param.key.taxis.empty() || getActualTime(reftime, paramname, time, time_tolerance, actualtime)))
      {
        // data exists ... calling getData
        if (Field* f = io->getData(reftime, param, level, actualtime, elevel, unit))
          return f;
      }

    }
  }
  METLIBS_LOG_WARN("giving up .. returning 0");
  return 0;
}

/**
 * Get data slice
 */
vcross::Values_p GridCollection::getVariable(const std::string& reftime, const std::string& paramname)
{
  gridinventory::GridParameter gp;
  for (GridIO* io : gridsources) {
    if (dataExists_reftime(io->getReftimeInventory(reftime), paramname, gp)) {
      return io->getVariable(paramname);
    }
  }
  METLIBS_LOG_WARN("giving up .. returning 0");
  return vcross::Values_p();
}
/**
 * Get times
 */
std::set<miutil::miTime> GridCollection::getTimes(const std::string& reftime, const std::string& paramname)
{
  METLIBS_LOG_SCOPE();

  std::set<miutil::miTime> settime;

  if (useTimeFromFilename())
    return getTimesFromFilename();

  gridinventory::GridParameter gp;
  for (GridIO* io : gridsources) {
    if (dataExists_reftime(io->getReftimeInventory(reftime), paramname,gp)) {
      const Taxis& tx = io->getTaxis(reftime,gp.key.taxis);
      for (double v : tx.values) {
        // double -> miTime
        time_t t = v;
        miutil::miTime tt(t);
        settime.insert(tt);
      }
    }
  }

  if (!settime.empty())
    return settime;

  if (dataExists_reftime(computed_inventory, paramname,gp)) {

    const size_t idx_colon = gp.nativekey.find(':');
    if (idx_colon != std::string::npos) {
      const int functionIndex = miutil::to_int(gp.nativekey.substr(idx_colon + 1));
      const FieldFunctions::FieldCompute& fcm = FieldFunctions::fieldCompute(functionIndex);
      bool firstInput = true;
      for (const std::string& pn : fcm.input) {
        FieldFunctions::FieldSpec fs;
        FieldFunctions::splitFieldSpecs(pn, fs);
        const std::set<miutil::miTime> settime2 = getTimes(reftime, fs.paramName);
        //Don't use parameters with no time axis
        if (settime2.size()==0)
          continue;
        if (firstInput) {
          settime = settime2;
          firstInput = false;
        } else {
          std::set<miutil::miTime> intersection;
          std::set_intersection(settime.begin(), settime.end(),
                                settime2.begin(), settime2.end(),
                                std::inserter(intersection, intersection.begin()));
          std::swap(settime, intersection);
        }
      }

      // check if all time steps are available
      bool timeStepFunc = FieldFunctions::isTimeStepFunction(fcm.function);
      if (timeStepFunc && !fcm.input.empty()) {
        std::vector<float> constants;
        const std::string& inputParamName = fcm.input[0];
        FieldFunctions::FieldSpec fs;
        FieldFunctions::splitFieldSpecs(inputParamName,fs);
        if (!fs.fcHour.empty()) {
          constants.push_back(miutil::to_int(fs.fcHour));
        } else {
          constants = fcm.constants;
        }
        set<miutil::miTime> settime2;
        const bool is_accumulate_flux = (fs.option == "accumulate_flux");
        const miutil::miTime rt(reftime);
        for (const miutil::miTime& t : settime) {
          size_t i = 0;
          for (; i < constants.size(); ++i) {
            miutil::miTime tmpTime = t;
            tmpTime.addHour(constants[i]);
            if (!settime.count(tmpTime)
                && (!is_accumulate_flux || (is_accumulate_flux && tmpTime != rt)))
            {
              break;
            }
          }
          if (i == constants.size()) {
            //all time steps ok
            settime2.insert(t);
          }
        }
        return settime2;
      }
    }
  }

  return settime;
}

/**
 * Put data slice
 */
bool GridCollection::putData(const std::string& reftime, const std::string& paramname,
    const std::string& level,
    const miutil::miTime& time, const std::string& elevel,
    const std::string& unit, const std::string& output_time,
    const Field* field)
{
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(paramname)
      << LOGVAL(level) << LOGVAL(time) <<  LOGVAL(elevel) << LOGVAL(output_time));

#ifdef FIMEX
  for (GridIO* io : gridsources) {
    gridinventory::GridParameter param;
    FimexIO* fio = dynamic_cast<FimexIO*>(io);
    if (not fio)
      continue;
    miutil::miTime actualtime;
    if (dataExists_reftime(fio->getReftimeInventory(reftime), paramname, param))
    {
      // data exists ... calling getData
      return fio->putData(reftime, param, level, actualtime, elevel, unit, field, output_time);
    }

  }
#endif
  METLIBS_LOG_WARN("giving up .. returning 0");
  return 0;
}

bool GridCollection::updateSources()
{
  METLIBS_LOG_SCOPE();
  //Filenames without wildcards do not change
  if (sources_with_wildcards.empty()) {
    return makeGridIOinstances();
  }

  std::set<std::string> newSources;
  for (const std::string& sourcestr : sources_with_wildcards) {
    // check for wild cards - expand filenames if necessary
    if (sourcestr.find_first_of("*?") != std::string::npos) {
      const diutil::string_v files = diutil::glob(sourcestr, GLOB_BRACE);
      newSources.insert(files.begin(), files.end());
    } else {
      newSources.insert(sourcestr);
    }
  }

  if (sources != newSources || sourcesChanged() ) {
    return makeGridIOinstances();
  }

  return true;
}

bool GridCollection::standardname2variablename(const std::string& reftime,
    const std::string& standard_name, std::string& variable_name)
{
  const map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  set<gridinventory::GridParameter>::iterator pitr = ritr->second.parameters.begin();
  while (pitr != ritr->second.parameters.end()
      && pitr->standard_name != standard_name)
    pitr++;
  if (pitr == ritr->second.parameters.end()) {
    METLIBS_LOG_DEBUG("parameter standard_name '" << standard_name << "' not found in inventory");
    return false;
  }

  variable_name = pitr->key.name;
  return true;
}


std::map<std::string,std::string> GridCollection::getGlobalAttributes(const std::string& refTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(refTime));

  std::map<std::string, gridinventory::ReftimeInventory>::const_iterator ritr =
      inventory.reftimes.find(refTime);
  if (ritr == inventory.reftimes.end()) {
    if (refTime.empty() && !inventory.reftimes.empty()) {
      ritr = inventory.reftimes.begin();
    } else {
      METLIBS_LOG_INFO( " refTime not found: " << refTime);
      return std::map<std::string,std::string>();
    }
  }
  return ritr->second.globalAttributes;
}

void GridCollection::getFieldInfo(const std::string& refTime, std::map<std::string,FieldInfo>& fieldInfo)
{
  METLIBS_LOG_SCOPE(LOGVAL(refTime));

  fieldInfo.clear();

  std::map<std::string, gridinventory::ReftimeInventory>::const_iterator ritr =
      inventory.reftimes.find(refTime);
  if (ritr == inventory.reftimes.end()) {
    if (refTime.empty() && !inventory.reftimes.empty()) {
      ritr = inventory.reftimes.begin();
    } else {
      METLIBS_LOG_INFO( " refTime not found: " << refTime);
      return;
    }
  }

  for (const gridinventory::GridParameter& gp : ritr->second.parameters) {
    FieldInfo vi;
    vi.fieldName = gp.key.name;
    vi.standard_name = gp.standard_name;

    set<gridinventory::Zaxis>::iterator zitr = ritr->second.zaxes.find(Zaxis(gp.zaxis_id));
    if (zitr != ritr->second.zaxes.end()) {
      vi.vlevels= zitr->getStringValues();
      vi.vcoord = zitr->verticalType;
      if (zitr->vc_type == FieldFunctions::vctype_oceandepth) {
        vi.default_vlevel = vi.vlevels.front();
      }
    }

    set<gridinventory::ExtraAxis>::iterator eitr = ritr->second.extraaxes.find(ExtraAxis(gp.extraaxis_id));
    if (eitr!=ritr->second.extraaxes.end()) {
      vi.ecoord = eitr->name;
      vi.elevels = eitr->getStringValues();
    }

    // groupname based on coordinates
    if ( vi.ecoord.empty() && vi.vcoord.empty()) {
      vi.groupName = "Surface";
    } else if ( vi.vcoord.empty() ) {
      vi.groupName = vi.ecoord;
    } else if ( vi.ecoord.empty() ) {
      vi.groupName = vi.vcoord;
    } else {
      vi.groupName = vi.vcoord + "_" + vi.ecoord;
    }

    fieldInfo[vi.fieldName]=vi;
  }
}

Field* GridCollection::getField(FieldRequest fieldrequest)
{
  METLIBS_LOG_TIME("SEARCHING FOR :" << fieldrequest.paramName << " : "
      << fieldrequest.zaxis << " : " << fieldrequest.plevel);

  const map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(fieldrequest.refTime);
  if (ritr == inventory.reftimes.end())
    return 0;


  // if fieldrequest.paramName is a standard_name, find key.name
  if (fieldrequest.standard_name) {
    if (!standardname2variablename(fieldrequest.refTime, fieldrequest.paramName, fieldrequest.paramName))
      return 0;
  }


  //check if requested parameter exist, and init param
  gridinventory::GridParameter param;

  // check if param is in inventory

  if (!dataExists(fieldrequest.refTime, fieldrequest.paramName, param)) {
    METLIBS_LOG_INFO("parameter '" << fieldrequest.paramName << "' not found by dataExists");
    return 0;
  }

  set<gridinventory::GridParameter>::iterator pitr = ritr->second.parameters.find(param);
  if (pitr == ritr->second.parameters.end()) {
    METLIBS_LOG_INFO("parameter " << fieldrequest.paramName
        << "  not found in inventory even if dataExists returned true");
    return 0;
  }

  //If not computed parameter, read field from GridCollection and return
  if (pitr->nativekey.find("function:") == std::string::npos) {
    METLIBS_LOG_INFO(LOGVAL(fieldrequest.ptime));
    Field* field = getData(fieldrequest.refTime, param.key.name, param.key.zaxis, param.key.taxis,
        param.key.extraaxis, fieldrequest.plevel,
        fieldrequest.ptime, fieldrequest.elevel, fieldrequest.unit,
        fieldrequest.time_tolerance);
    return field;
  }

  //parameter must be computed from input parameters using some function

  //Find function, number of input parameters and constants
  std::string index = pitr->nativekey.substr(pitr->nativekey.find(':') + 1);
  int functionIndex = atoi(index.c_str());
  const FieldFunctions::FieldCompute& fcm = FieldFunctions::fieldCompute(functionIndex);
  int nOutputParameters = fcm.results.size();
  int nInputParameters = fcm.input.size();
  vector<Field*> vfield; // Input fields
  vector<Field*> vfresults; //Output fields
  bool fieldOK = false;

  //Functions using fields with different forecast time
  if (FieldFunctions::isTimeStepFunction(fcm.function)) {

    FieldRequest fieldrequest_new = fieldrequest;
    //levelSpecified true if param:level=value
    FieldFunctions::FieldSpec fs;
    bool levelSpecified = FieldFunctions::splitFieldSpecs(fcm.input[0], fs);
    fieldrequest_new.paramName = fs.paramName;
    fieldrequest_new.standard_name = fs.use_standard_name; //functions use standard_name
    fieldrequest_new.unit = fs.unit;
    if (levelSpecified) {
      fieldrequest_new.zaxis = fs.vcoordName;
      fieldrequest_new.plevel = fs.levelName;
    }
    if (!fs.elevel.empty()) {
      fieldrequest_new.elevel = fs.elevel;
    }

    if (!fs.fcHour.empty()) {
      int fch = miutil::to_int(fs.fcHour);
      if (!getAllFields_timeInterval(vfield, fieldrequest_new,
                                     fch, (fs.option == "accumulate_flux")))
      {
        freeFields(vfield);
        return 0;
      }
    } else {
      if (!getAllFields(vfield, fieldrequest_new, fcm.constants)) {
        freeFields(vfield);
        return 0;
      }
    }

    //make output field
    Field* ff = new Field();
    ff->shallowMemberCopy(*vfield[0]);
    ff->reserve(vfield[0]->area.nx, vfield[0]->area.ny);
    ff->validFieldTime = fieldrequest.ptime;
    ff->unit = fieldrequest.unit;
    vfresults.push_back(ff);

    if (!FieldFunctions::fieldComputer(fcm.function, fcm.constants, vfield, vfresults, gc)) {
      METLIBS_LOG_WARN("fieldComputer returned false");
      fieldOK = false;
    } else {
      fieldOK = true;
    }

  } else {

    // loop trough input params with same zaxis
    for (int j = 0; j < nInputParameters; j++) {

      FieldRequest fieldrequest_new = fieldrequest;
      const std::string& inputParamName = fcm.input[j];

      //levelSpecified true if param:level=value
      FieldFunctions::FieldSpec fs;
      bool levelSpecified = FieldFunctions::splitFieldSpecs(inputParamName, fs);
      fieldrequest_new.paramName = fs.paramName;
      fieldrequest_new.standard_name = fs.use_standard_name; //functions use standard_name
      if (fs.paramName == "surface_air_pressure") { //TODO: psurf - used in hybrid functions, should be defined in setup
        fieldrequest_new.zaxis.clear();
        fieldrequest_new.plevel.clear();
      }

      if (levelSpecified) {
        fieldrequest_new.zaxis = fs.vcoordName;
        fieldrequest_new.plevel = fs.levelName;
      }
      if (!fs.unit.empty()) {
        fieldrequest_new.unit = fs.unit;
      }
      if (!fs.elevel.empty()) {
        fieldrequest_new.elevel = fs.elevel;
      }

      if (fs.ecoordName.empty() && fs.vcoordName.empty()) {
        Field * f = getField(fieldrequest_new);
        if (!f) {
          METLIBS_LOG_DEBUG("unable to read '" << inputParamName << "'");
          freeFields(vfield);
          return 0;
        } else {
          vfield.push_back(f);
        }

      } else {

        vector<std::string> values;
        if (!fs.ecoordName.empty()) {
          gridinventory::ExtraAxis eaxs = ritr->second.getExtraAxis(fs.ecoordName);
          values = eaxs.stringvalues;
        } else if (!fs.vcoordName.empty()) {
          gridinventory::Zaxis zaxs = ritr->second.getZaxis(fs.vcoordName);
          values = zaxs.stringvalues;
        }
        if (values.empty()) {
          freeFields(vfield);
          return 0;
        }
        for (size_t i = 0; i < values.size(); i++) {
          if (!fs.ecoordName.empty()) {
            fieldrequest_new.elevel = values[i];
          } else if (!fs.vcoordName.empty()) {
            fieldrequest_new.plevel = values[i];
          }
          Field * f = getField(fieldrequest_new);
          if (!f) {
            freeFields(vfield);
            return 0;
          } else {
            vfield.push_back(f);
          }
        }

      }

    } //end loop inputParameters

    if (vfield.empty()) {
      return 0;
    }

    for (int j = 0; j < nOutputParameters; j++) {
      Field* ff = new Field();
      ff->shallowMemberCopy(*vfield[0]);
      ff->reserve(vfield[0]->area.nx, vfield[0]->area.ny);
      ff->unit = fieldrequest.unit;
      vfresults.push_back(ff);
    }

    if (!FieldFunctions::fieldComputer(fcm.function, fcm.constants, vfield, vfresults, gc)) {
      METLIBS_LOG_WARN("fieldComputer returned false");
      fieldOK = false;
    } else {
      fieldOK = true;
    }
  }

  //delete input fields
  freeFields(vfield);

  //return output field
  Field* fresult = 0;
  if (fieldOK)
    std::swap(fresult, vfresults[0]);
  freeFields(vfresults);

  return fresult;
}

bool GridCollection::getActualTime(const std::string& reftime, const std::string& paramname, const miutil::miTime& time,
    const int & time_tolerance, miutil::miTime& actualtime)
{
  std::set<miutil::miTime> times = GridCollection::getTimes(reftime,paramname);
  // find nearest time within time tolerance
  long int mdiff = time_tolerance + 1;
  for (set<miutil::miTime>::const_iterator titr = times.begin(); titr != times.end(); ++titr) {
    long int d = labs(miutil::miTime::minDiff(time, *titr));
    if (d < mdiff) {
      mdiff = d;
      actualtime = *titr;
    }
  }

  return (mdiff <= time_tolerance);
}


bool GridCollection::dataExists_reftime(const gridinventory::ReftimeInventory& reftimeInv,
    const std::string& paramname, gridinventory::GridParameter& gp)
{
  METLIBS_LOG_SCOPE(LOGVAL(paramname));
  for (const gridinventory::GridParameter& p : reftimeInv.parameters) {
    if (p.key.name == paramname) {
      METLIBS_LOG_DEBUG("found paramname :-)  "<<p.key.name );
      gp = p;
      return true;
    }
  }

  return false;
}



namespace {
struct compare_name {
  compare_name(const std::string& c)
    : compare_to_(c) { }
  bool operator() (const gridinventory::GridParameter& p) const
    { return compare_to_ == p.name; }

  const std::string& compare_to_;
};
}


void GridCollection::addComputedParameters()
{
  METLIBS_LOG_SCOPE();

  //add computed parameters to inventory
  Inventory::reftimes_t& reftimes = inventory.reftimes;
  METLIBS_LOG_DEBUG(LOGVAL(inventory.reftimes.size()));

  gridinventory::ReftimeInventory& rinventory = reftimes.begin()->second;

  // loop through all functions
  int i = -1;
  for (const FieldFunctions::FieldCompute& fc : FieldFunctions::fieldComputes()) {
    i += 1;
    const std::string& computeParameterName = fc.name;
    METLIBS_LOG_DEBUG(LOGVAL(fc.name));
    METLIBS_LOG_DEBUG(LOGVAL(rinventory.parameters.size()));
    //check if parameter exists
    set<gridinventory::GridParameter>::iterator pitr = std::find_if(rinventory.parameters.begin(), rinventory.parameters.end(),
                                                                    compare_name(computeParameterName));
    METLIBS_LOG_DEBUG("not in inventory");
    if (pitr != rinventory.parameters.end()) {
      break;
    }
    //Compute parameter?
    //find input parameters and check

    bool inputOk = true;
    std::string computeZaxis;// = VerticalName[FieldFunctions::vctype_none]; //default, change if input parameter has different zaxis
    int computeZaxisValues = -1;
    std::string computeEaxis;
    // loop trough input params with same zaxis
    std::string fchour;
    for (const std::string& inputParameterName : boost::adaptors::reverse(fc.input)) {
      //levelSpecified true if param:level=value
      METLIBS_LOG_DEBUG(LOGVAL(inputParameterName));
      FieldFunctions::FieldSpec fs;
      bool levelSpecified = FieldFunctions::splitFieldSpecs(inputParameterName, fs);
      fchour = fs.fcHour;
      pitr = rinventory.parameters.begin();
      // loop through parameters
      for (; pitr != rinventory.parameters.end(); ++pitr) {
        std::string pitr_name;
        if( fs.use_standard_name ) {
          pitr_name = pitr->standard_name;
        } else {
          pitr_name = pitr->key.name;
        }

        if (pitr_name == fs.paramName) {
          METLIBS_LOG_DEBUG(LOGVAL(pitr_name));

          set<gridinventory::Zaxis>::iterator zitr = rinventory.zaxes.find(Zaxis(pitr->zaxis_id));

          //level do not exists
          if (levelSpecified && !zitr->valueExists(fs.levelName)) {
            inputOk = false;
            break;
          }
          //ask for zaxis which do not exists
          if (!levelSpecified ) {
            // if input parameters have different zaxes, ignore zaxis if number of levels=1
            if ( !computeZaxis.empty() && pitr->key.zaxis!=computeZaxis) {
              if ( computeZaxisValues == 1 ) {
                computeZaxis.clear();
              } else {
                inputOk = false;
                break;
              }
            }
            // Set computeZaxis, but don't overwrite zaxis whith more than one value
            if ( computeZaxisValues != 1 || zitr->values.size() > 1 ) {
              computeZaxis = pitr->key.zaxis;
              computeZaxisValues =  zitr->values.size();
            }
          }

          //ask for axis which do not exists
          if (!fs.ecoordName.empty() && pitr->key.extraaxis.empty()) {
            inputOk = false;
            break;
          }
          if (fs.ecoordName.empty() && fs.elevel.empty() && pitr->key.extraaxis!="") {
            computeEaxis = pitr->key.extraaxis;
          }

          break;
        }
      }
      if (pitr == rinventory.parameters.end()) {
        inputOk = false;
        break; //can't make this computeParameter
      }
    }

    if (inputOk) {
      METLIBS_LOG_DEBUG("check time");
      //   check time axis
      if (FieldFunctions::isTimeStepFunction(fc.function)) {
        // sort function constants
        int minConst=0,maxConst=0;
        if ( !fchour.empty() ) {
          maxConst = atoi(fchour.c_str());
        } else if (fc.constants.size()){
          std::vector<float> sortedconstants = fc.constants;
          std::sort(sortedconstants.begin(), sortedconstants.end());
          minConst = int(sortedconstants.front());
          maxConst = int(sortedconstants.back());
        }
        //Find time axis
        set<gridinventory::Taxis>::iterator titr = rinventory.taxes.find(Taxis(pitr->taxis_id));
        long minTimeStep = titr->getMinStep()/3600;
        long forecastLength = titr->getForecastLength()/3600;
        if (minTimeStep !=0 &&
            (minConst%minTimeStep!=0 || maxConst%minTimeStep != 0 || maxConst-minConst > forecastLength)) {
          continue;
        }
      }

      //Add computed parameter to inventory
      //computed parameter inherits taxis, extraaxis and grid from input parameter
      gridinventory::GridParameter newparameter = *pitr;
      newparameter.key.name = computeParameterName;
      newparameter.standard_name = computeParameterName;
      newparameter.key.zaxis = computeZaxis;
      if (computeZaxis.empty()) {
        newparameter.zaxis_id.clear();
      }
      newparameter.key.taxis.clear();
      newparameter.key.extraaxis = computeEaxis;
      if ( computeEaxis.empty() ) {
        newparameter.extraaxis_id.clear();
      }
      ostringstream ost;
      ost <<"function:"<<i<<endl;
      newparameter.nativekey = ost.str();
      rinventory.parameters.insert(newparameter);
      computed_inventory.parameters.insert(newparameter);
      METLIBS_LOG_DEBUG("Add new parameter");
      METLIBS_LOG_DEBUG(LOGVAL(newparameter.key.name) <<LOGVAL(computeZaxis));
      METLIBS_LOG_DEBUG(LOGVAL(newparameter.nativekey));
      METLIBS_LOG_DEBUG(LOGVAL(newparameter.zaxis_id));
      METLIBS_LOG_DEBUG(LOGVAL(newparameter.key.extraaxis));
    } else {
      METLIBS_LOG_DEBUG("not found");
    }
    // } //end parameter loop
  } //end function loop

}


bool GridCollection::getAllFields_timeInterval(vector<Field*>& vfield,
    FieldRequest fieldrequest, int fch, bool accumulate_flux)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldrequest.paramName));

  miutil::miTime endTime = fieldrequest.ptime;
  miutil::miTime startTime = fieldrequest.ptime;
  if (fch < 0) // TODO what about fch == 0?
    startTime.addHour(fch);
  else
    endTime.addHour(fch);

  // get all available timesteps between start and end.
  //const vector<FieldRequest> fieldrequests(1, fieldrequest);
  const set<miutil::miTime> fieldTimes = getTimes(fieldrequest.refTime, fieldrequest.paramName);
  set<miutil::miTime>::const_iterator ip = fieldTimes.begin();
  set<miutil::miTime> actualfieldTimes;
  for (; ip != fieldTimes.end(); ip++) {
    if (*ip >= startTime && *ip <= endTime)
      actualfieldTimes.insert(*ip);
  }
  if (actualfieldTimes.empty())
    return false;

  if (accumulate_flux) {
    if (fch > 0) {
      METLIBS_LOG_WARN("accumulate_flux with fchour> 0 is not implemented");
      return false;
    }
    if (!actualfieldTimes.count(startTime) && (startTime != miutil::miTime(fieldrequest.refTime))) {
      METLIBS_LOG_DEBUG(fieldrequest.paramName << " not available for "<< startTime);
      return false;
    }
  }
  if (fch < 0) {
    actualfieldTimes.erase(startTime);
  } else {
    actualfieldTimes.erase(endTime);
  }
  miutil::miTime lastTime = startTime; // only used iff accumulate_flux
  for (const miutil::miTime& t : actualfieldTimes) {
    fieldrequest.ptime = t;
    Field * f = getField(fieldrequest);
    if (!f) {
      METLIBS_LOG_WARN("Field not found for: " << fieldrequest.ptime);
      return false;
    } else {
      if (accumulate_flux) {
        const float sec_diff = miutil::miTime::secDiff(t, lastTime);
        if (!multiplyFieldByTimeStep(f, sec_diff))
          return false;
        lastTime = t;
      }
      vfield.push_back(f);
    }
  }
  return !vfield.empty();
}

bool GridCollection::getAllFields(std::vector<Field*>& vfield,
    FieldRequest fieldrequest, const std::vector<float>& constants)
{
  const int nConstants = constants.size();
  if (nConstants == 0)
    return false;
  const miutil::miTime startTime = fieldrequest.ptime;
  for (int i = nConstants - 1; i >= 0; i--) {
    fieldrequest.ptime = startTime;
    fieldrequest.ptime.addHour(constants[i]);
    Field * f = getField(fieldrequest);
    if (f) {
      vfield.push_back(f);
    } else {
      return false;
    }
  }
  return true;
}

bool GridCollection::multiplyFieldByTimeStep(Field* f, float sec_diff)
{
  const vector<float> constants(1, sec_diff);
  const vector<Field*> vfield(1, f);
  if (FieldFunctions::fieldComputer(FieldFunctions::f_multiply_f_c, constants, vfield, vfield, gc)) {
    return true;
  }
  METLIBS_LOG_WARN("problem in multiplyFieldByTimeStep");
  return false;
}

void GridCollection::freeFields(std::vector<Field*>& fields)
{
  METLIBS_LOG_SCOPE();
  diutil::delete_all_and_clear(fields);
}
