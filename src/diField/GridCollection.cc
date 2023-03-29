/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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
 * GridCollection.cc
 *
 *  Created on: Mar 15, 2010
 *      Author: audunc
 */

#include "diana_config.h"

#include "GridCollection.h"

#include "CachedGridIO.h"
#ifdef FIMEX
#include "FimexIO.h"
#endif
#include "../diFieldUtil.h"
#include "../diUtilities.h"
#include "VcrossUtil.h"
#include "diField.h"
#include "diFieldFunctions.h"
#include "util/misc_util.h"
#include "util/nearest_element.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puCtools/puCglob.h>
#include <puTools/miTime.h>
#include <puTools/miString.h>
#include <puTools/TimeFilter.h>

#include <boost/range/adaptor/reversed.hpp>

#define MILOGGER_CATEGORY "diField.GridCollection"
#include "miLogger/miLogging.h"

using namespace gridinventory;

namespace {

static const std::string FUNCTION = "function:";

void updateFieldRequestFromFieldSpec(const FieldFunctions::FieldCompute& fcm, int arg, FieldRequest& frq, FieldFunctions::FieldSpec& fs)
{
  const bool levelSpecified = FieldFunctions::splitFieldSpecs(fcm.input[arg], fs);
  frq.paramName = fs.paramName;
  frq.standard_name = fs.use_standard_name;     // functions use standard_name
  if (fs.paramName == "surface_air_pressure") { // TODO: psurf - used in hybrid functions, should be defined in setup
    frq.zaxis.clear();
    frq.plevel.clear();
  }

  if (levelSpecified) {
    frq.plevel = fs.levelName;
  }
  if (!fs.elevel.empty()) {
    frq.elevel = fs.elevel;
  }

  if (!fs.unit.empty()) {
    frq.unit = fs.unit;
  }
  if (fcm.func) {
    const auto& argsf = fcm.func->args_field;
    const bool is_varargs_f = (fcm.func->varargs & FieldFunctions::varargs_field);
    const int iarg = is_varargs_f ? argsf.size() - 1 : arg;
    if (iarg < (int)argsf.size() && !argsf[iarg].units.empty()) {
      frq.unit = argsf[iarg].units;
      if (!fs.unit.empty() && !vcross::util::unitsIdentical(fs.unit, frq.unit))
        METLIBS_LOG_WARN("ignoring unit '" << fs.unit << "' from compute setup as function requires unit '" << frq.unit << "'");
    }
  }
}

bool addInputField(Field_pv& vfield, Field_p f, const FieldFunctions::FieldCompute& fcm)
{
  if (!f)
    return false;

  if (!vfield.empty() && fcm.func && (fcm.func->varargs & FieldFunctions::varargs_field)) {
    f = convertUnit(f, vfield.front()->unit);
  }

  if (!f)
    return false;

  vfield.push_back(f);
  return true;
}

std::vector<Field_p> createOutputFields(const FieldFunctions::FieldCompute& fcm, const Field_pv& vfield, const FieldRequest& fieldrequest)
{
  if (vfield.empty() || std::find(vfield.begin(), vfield.end(), nullptr) != vfield.end())
    return std::vector<Field_p>();

  std::vector<Field_p> vfresults;
  vfresults.reserve(fcm.results.size());
  for (size_t j = 0; j < fcm.results.size(); j++) {
    Field_p ff = std::make_shared<Field>();
    ff->shallowMemberCopy(*vfield[0]);
    ff->reserve(vfield[0]->area.nx, vfield[0]->area.ny);
    if (fcm.func) {
      const bool is_varargs_f = (fcm.func->varargs & FieldFunctions::varargs_field);
      const bool is_unit0 = (fcm.func->units[j] == "=0");
      const std::string& unit = (is_varargs_f || is_unit0) ? vfield[0]->unit : fcm.func->units[j];
      if (!unit.empty() && !fieldrequest.unit.empty() && !vcross::util::unitsConvertible(unit, fieldrequest.unit))
        return std::vector<Field_p>();
      ff->unit = unit;
    } else {
      const bool is_add = (fcm.function == FieldFunctions::f_add_f_f);
      const bool is_subtract = (fcm.function == FieldFunctions::f_subtract_f_f);
      const bool is_add_const = (fcm.function == FieldFunctions::f_add_f_c || fcm.function == FieldFunctions::f_add_c_f);
      const bool is_subtract_const = (fcm.function == FieldFunctions::f_subtract_f_c || fcm.function == FieldFunctions::f_subtract_c_f);
      if ((is_add || is_subtract) && vfield[0]->unit == vfield[1]->unit) {
        ff->unit = vfield[0]->unit;
      } else if (is_add_const || is_subtract_const) {
          ff->unit = vfield[0]->unit;
      } else {
        ff->unit.clear();
      }
    }
    vfresults.push_back(ff);
  }
  return vfresults;
}

/** Extract function index from nativekey.
  \return function index if computed, or -1 if not computed
*/
const FieldFunctions::FieldCompute* extractFunction(const std::string& nativekey)
{
  if (!diutil::startswith(nativekey, FUNCTION))
    return nullptr;

  const int functionIndex = atoi(nativekey.substr(FUNCTION.size()).c_str());
  return &FieldFunctions::fieldCompute(functionIndex);
}

} // namespace

// static class members
GridConverter GridCollection::gc;    // Projection-converter

GridCollection::GridCollection()
: gridsetup(0)
{
}

// initialize collection from a list of sources
bool GridCollection::setContents(const std::string& type, const std::string& name, const std::vector<std::string>& filenames,
                                 const std::vector<std::string>& format, const std::vector<std::string>& config, const std::vector<std::string>& option,
                                 GridIOsetup* setup, bool validTimeFromFilename)
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

namespace {
const std::string& index_or_first(const std::vector<std::string>& available, size_t index, size_t count)
{
  if (count == available.size() && index < count) {
    return available[index];
  } else if (!available.empty()) {
    return available.front();
  } else {
    static const std::string empty;
    return empty;
  }
}
} // namespace

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
      diutil::insert_all(tmpsources, files);
    } else {
      tmpsources.insert(sourcestr);
    }
    diutil::insert_all(sources,tmpsources);

    //if #formats == #rawsources, use corresponding files. If not use first format
    const std::string& format = index_or_first(formats, index, rawsources.size());

    //if #configs == #rawsources, use corresponding files. If not use first config
    const std::string& config = index_or_first(configs, index, rawsources.size());

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
      GridIOBase* gp = 0;
#ifdef FIMEX
      if (sourcetype == FimexIO::getSourceType()) {
        gp = new FimexIO(collectionname, sourcename, reftime_from_filename, format, config,
            options, makeFeltReader, static_cast<FimexIOsetup*> (gridsetup));
      }
#endif
      if ( gp ) {
#if 0
        gp = new CachedGridIO(gp);
#endif
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

bool GridCollection::makeInventory(const std::string& refTime)
{
  METLIBS_LOG_TIME(LOGVAL(refTime));

  reftime_fieldplotinfo_.erase(refTime);

  bool ok = true;
  // decide if we should make new GridIO instances
  if (gridsources.empty()) {
    ok = makeGridIOinstances();
  }
  // make inventory on all sources and make a combined inventory for the whole collection
  inventoryOK.clear();
  inventory.clear();

  for (const auto io : gridsources) {
    // enforce the reference time limits
    //    (*itr)->setReferencetimeLimits(limit_min, limit_max); // not used yet

    // make inventory on each GridIO
    bool result = false;

    // make referencetime inventory from referencetime given in filename, if possible
    if (refTime.empty()) {
      const std::string reftime_from_filename = io->getReferenceTime();
      if (!reftime_from_filename.empty()) {
        result = true;
        refTimes.insert(reftime_from_filename);
      }
    }

    if (!result && io->referenceTimeOK(refTime)) {
      result = io->makeInventory(refTime);
      METLIBS_LOG_DEBUG(LOGVAL(refTime));
      inventoryOK[refTime] = result;
      if (result)
        inventory = inventory.merge(io->getInventory());
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
  for (const auto& io : gridsources)
    if (io->sourceChanged())
      return true;
  return false;
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
  if (!refTimes.empty())
    return refTimes;

  std::set<std::string> reftimes;
  for (const auto& ir : inventory.reftimes)
    reftimes.insert(ir.first);
  return reftimes;
}

/**
 * Check if data exists
 */
const gridinventory::GridParameter* GridCollection::dataExists(const std::string& reftime, const std::string& paramname)
{
  METLIBS_LOG_SCOPE("searching for: " <<LOGVAL(paramname));
  for (const auto io : gridsources) {
    if (const gridinventory::GridParameter* param = dataExists_reftime(io->getReftimeInventory(reftime), paramname)) {
      return param;
    }
  }
  METLIBS_LOG_DEBUG("SEARCHING IN COMPUTED:");
  return dataExists_reftime(computed_inventory, paramname);
}

/**
 * Get data slice
 */
Field_p GridCollection::getData(const std::string& reftime, const std::string& paramname, const std::string& zaxis, const std::string& taxis,
                                const std::string& extraaxis, const std::string& level, const miutil::miTime& time, const std::string& elevel,
                                const int& time_tolerance)
{
  METLIBS_LOG_SCOPE(reftime << " | " << paramname << " | " << zaxis
      << " | " << taxis << " | " << extraaxis << " | "
      << level << " | " << time << " | " << elevel << "|" << time_tolerance);

  miutil::miTime  actualtime;

  if (timeFromFilename && getActualTime(reftime, paramname, time, time_tolerance, actualtime)) {
    const auto ip = gridsourcesTimeMap.find(actualtime);
    if (ip != gridsourcesTimeMap.end()) {
      ip->second->makeInventory(reftime);
      if (const gridinventory::GridParameter* param = dataExists_reftime(ip->second->getReftimeInventory(reftime), paramname)) {
        // Ignore time from file, just use the first timestep
        Field_p f = ip->second->getData(reftime, *param, level, miutil::miTime(), elevel);
        if (f)
          f->validFieldTime = actualtime;
        return f;
      }
    }

  } else {
    for (const auto io : gridsources) {
      gridinventory::GridParameter param;
      if (const gridinventory::GridParameter* param = dataExists_reftime(io->getReftimeInventory(reftime), paramname)) {
        if (param->key.taxis.empty() || getActualTime(reftime, paramname, time, time_tolerance, actualtime)) {
          // data exists ... calling getData
          if (Field_p f = io->getData(reftime, *param, level, actualtime, elevel))
            return f;
        }
      }
    }
  }
  METLIBS_LOG_DEBUG("giving up .. returning 0");
  return 0;
}

/**
 * Get data slice
 */
vcross::Values_p GridCollection::getVariable(const std::string& reftime, const std::string& paramname)
{
  for (const auto io : gridsources) {
    if (const gridinventory::GridParameter* gp = dataExists_reftime(io->getReftimeInventory(reftime), paramname)) {
      return io->getVariable(paramname);
    }
  }
  METLIBS_LOG_WARN("giving up .. returning 0");
  return vcross::Values_p();
}

std::set<miutil::miTime> GridCollection::getTimesFromIO(const std::string& reftime, const std::string& paramname)
{
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(paramname));
  std::set<miutil::miTime> times;

  for (const auto io : gridsources) {
    if (const gridinventory::GridParameter* gp = dataExists_reftime(io->getReftimeInventory(reftime), paramname)) {
      const Taxis& tx = io->getTaxis(reftime, gp->key.taxis);
      for (double v : tx.values) {
        // double -> miTime
        time_t t = v;
        miutil::miTime tt(t);
        if (!tt.undef())
          times.insert(tt);
      }
    }
  }

  return times;
}

std::set<miutil::miTime> GridCollection::getTimesFromCompute(const std::string& reftime, const std::string& paramname)
{
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(paramname));

  const gridinventory::GridParameter* gp = dataExists_reftime(computed_inventory, paramname);
  if (!gp)
    return std::set<miutil::miTime>();

  const FieldFunctions::FieldCompute* fcm = extractFunction(gp->nativekey);
  if (!fcm) // not computed
    return std::set<miutil::miTime>();

  std::set<miutil::miTime> times;
  for (const std::string& pn : fcm->input) {
    FieldFunctions::FieldSpec fs;
    FieldFunctions::splitFieldSpecs(pn, fs);

    if (fs.use_standard_name && !standardname2variablename(reftime, fs.paramName, fs.paramName))
      return std::set<miutil::miTime>();
    const std::set<miutil::miTime> param_times = getTimes(reftime, fs.paramName); // recursive
    if (times.empty()) {
      times = param_times;
    } else if (!param_times.empty()) {
      std::set<miutil::miTime> intersection;
      std::set_intersection(times.begin(), times.end(), param_times.begin(), param_times.end(), std::inserter(intersection, intersection.begin()));
      times = std::move(intersection);
    }
  }

  // check if all time steps are available
  if (!FieldFunctions::isTimeStepFunction(fcm->function) || fcm->input.empty())
    return times;

  const std::string& inputParamName = fcm->input[0];
  FieldFunctions::FieldSpec fs;
  FieldFunctions::splitFieldSpecs(inputParamName, fs);
  std::vector<float> constants;
  if (!fs.fcHour.empty()) {
    constants.push_back(miutil::to_int(fs.fcHour));
  } else {
    constants = fcm->constants;
  }

  std::set<miutil::miTime> times_stepfunc;
  const bool is_accumulate_flux = (fs.option == "accumulate_flux");
  const miutil::miTime rt(reftime);
  for (const miutil::miTime& t : times) {
    size_t i = 0;
    for (; i < constants.size(); ++i) {
      miutil::miTime tmpTime = t;
      tmpTime.addHour(constants[i]);
      if (!times.count(tmpTime) && (!is_accumulate_flux || (is_accumulate_flux && tmpTime != rt))) {
        break;
      }
    }
    if (i == constants.size()) {
      // all time steps ok
      times_stepfunc.insert(t);
    }
  }
  return times_stepfunc;
}

std::set<miutil::miTime> GridCollection::getTimes(const std::string& reftime, const std::string& paramname)
{
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(paramname));
  if (useTimeFromFilename())
    return getTimesFromFilename();

  std::set<miutil::miTime> times = getTimesFromIO(reftime, paramname);
  if (times.empty())
    times = getTimesFromCompute(reftime, paramname);
  return times;
}

/**
 * Put data slice
 */
bool GridCollection::putData(const std::string& reftime, const std::string& paramname, const std::string& level, const miutil::miTime& time,
                             const std::string& elevel, const std::string& unit, const std::string& output_time, const Field_cp field)
{
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(paramname)
      << LOGVAL(level) << LOGVAL(time) <<  LOGVAL(elevel) << LOGVAL(output_time));

  for (auto io : gridsources) {
    if (const gridinventory::GridParameter* param = dataExists_reftime(io->getReftimeInventory(reftime), paramname)) {
      // data exists ... calling getData
      miutil::miTime actualtime;
      return io->putData(reftime, *param, level, actualtime, elevel, unit, field, output_time);
    }
  }

  METLIBS_LOG_WARN("giving up ...");
  return false;
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
      diutil::insert_all(newSources, diutil::glob(sourcestr, GLOB_BRACE));
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
  METLIBS_LOG_SCOPE(LOGVAL(reftime) << LOGVAL(standard_name));

  const std::map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(reftime);
  if (ritr == inventory.reftimes.end()) {
    METLIBS_LOG_DEBUG("reftime '" << reftime << "' not found in inventory");
    return false;
  }

  for (const gridinventory::GridParameter& p : ritr->second.parameters) {
    if (standard_name == p.standard_name) {
      variable_name = p.key.name;
      return true;
    }
  }

  METLIBS_LOG_DEBUG("parameter standard_name '" << standard_name << "' not found in inventory");
  return false;
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

namespace {

template <class Axis>
FieldPlotAxis_cp createFieldPlotAxisFromGridInventory(const Axis& axis)
{
  FieldPlotAxis_p ax = std::make_shared<FieldPlotAxis>();
  ax->name = axis.getName();
  ax->values = axis.getStringValues();
  return ax;
}

FieldPlotAxis_cp createFieldPlotAxisFromGridInventory(const Zaxis& zaxis)
{
  FieldPlotAxis_p vax = std::make_shared<FieldPlotAxis>();
  vax->name = zaxis.verticalType;
  vax->values = zaxis.getStringValues();
  if (zaxis.vc_type == FieldVerticalAxes::vctype_oceandepth) {
    vax->default_value_index = 0;
  }
  return vax;
}

FieldPlotAxis_cp createFieldPlotAxisFromGridInventory(const gridinventory::Taxis& taxis)
{
  FieldPlotAxis_p tax = std::make_shared<FieldPlotAxis>();
  tax->name = taxis.getName();
  tax->values.reserve(taxis.values.size());
  for (double v : taxis.values) {
    // double -> miTime -> iso string
    time_t t = v;
    miutil::miTime tt(t);
    if (!tt.undef())
      tax->values.push_back(tt.isoTime("T"));
  }
  return tax;
}

template <class Axis>
struct AxisCache
{
  std::map<std::string, FieldPlotAxis_cp> axes_by_id;
  const std::set<Axis>& inventory_axes;

  AxisCache(const std::set<Axis>& ia)
      : inventory_axes(ia)
  {
  }
  FieldPlotAxis_cp find(const std::string& id);
};

template <class Axis>
FieldPlotAxis_cp AxisCache<Axis>::find(const std::string& id)
{
  METLIBS_LOG_SCOPE();
  if (id.empty())
    return nullptr;

  const auto it_ax = axes_by_id.find(id);
  if (it_ax != axes_by_id.end())
    return it_ax->second;

  FieldPlotAxis_cp ax;

  typename std::set<Axis>::iterator it = inventory_axes.find(Axis(id));
  if (it != inventory_axes.end())
    ax = createFieldPlotAxisFromGridInventory(*it);

  axes_by_id.insert(std::make_pair(id, ax)); // also add if not found, to avoid lookups
  return ax;
}

} // namespace

std::map<std::string, FieldPlotInfo> GridCollection::buildFieldPlotInfo(const std::string& refTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(refTime));

  std::map<std::string, FieldPlotInfo> fieldInfo;

  auto ritr = inventory.reftimes.find(refTime);
  if (ritr == inventory.reftimes.end()) {
    if (refTime.empty() && !inventory.reftimes.empty()) {
      ritr = inventory.reftimes.begin();
    } else {
      METLIBS_LOG_INFO("refTime '" << refTime << "' not found, list of plots will be empty");
      return fieldInfo;
    }
  }

  AxisCache<gridinventory::Zaxis> plot_axes_vertical(ritr->second.zaxes);
  AxisCache<gridinventory::ExtraAxis> plot_axes_realization(ritr->second.extraaxes);
  AxisCache<gridinventory::Taxis> plot_axes_time(ritr->second.taxes);

  FieldPlotAxis_p tax_from_filenames;
  if (useTimeFromFilename()) {
    const auto& tff = getTimesFromFilename();
    tax_from_filenames = std::make_shared<FieldPlotAxis>();
    tax_from_filenames->name = "time";
    tax_from_filenames->values.reserve(tff.size());
    for (const auto& tt : getTimesFromFilename())
      tax_from_filenames->values.push_back(tt.isoTime("T"));
  }

  for (const gridinventory::GridParameter& gp : ritr->second.parameters) {
    FieldPlotInfo vi;
    vi.fieldName = gp.key.name;
    vi.standard_name = gp.standard_name;
    vi.units = gp.unit;

    if (tax_from_filenames)
      vi.time_axis = tax_from_filenames;
    else
      vi.time_axis = plot_axes_time.find(gp.taxis_id);
    vi.vertical_axis = plot_axes_vertical.find(gp.zaxis_id);
    vi.realization_axis = plot_axes_realization.find(gp.extraaxis_id);

    // groupname based on coordinates
    if (vi.ecoord().empty() && vi.vcoord().empty()) {
      vi.groupName = "Surface";
    } else if (vi.vcoord().empty()) {
      vi.groupName = vi.ecoord();
    } else if (vi.ecoord().empty()) {
      vi.groupName = vi.vcoord();
    } else {
      vi.groupName = vi.vcoord() + "_" + vi.ecoord();
    }

    fieldInfo[vi.fieldName] = vi;
  }

  return fieldInfo;
}

const std::map<std::string, FieldPlotInfo>& GridCollection::getFieldPlotInfo(const std::string& refTime)
{
  auto it = reftime_fieldplotinfo_.find(refTime);
  if (it == reftime_fieldplotinfo_.end())
    it = reftime_fieldplotinfo_.insert(std::make_pair(refTime, buildFieldPlotInfo(refTime))).first;
  return it->second;
}

Field_p GridCollection::getField(const FieldRequest& fieldrequest)
{
  METLIBS_LOG_TIME("SEARCHING FOR :" << fieldrequest.paramName << " : " << fieldrequest.zaxis << " : " << fieldrequest.plevel);

  const std::map<std::string, ReftimeInventory>::const_iterator ritr = inventory.reftimes.find(fieldrequest.refTime);
  if (ritr == inventory.reftimes.end())
    return 0;

  std::string param_name = fieldrequest.paramName;
  // if fieldrequest.paramName is a standard_name, find key.name
  if (fieldrequest.standard_name) {
    if (!standardname2variablename(fieldrequest.refTime, param_name, param_name))
      return 0;
  }

  // check if param is in inventory
  const gridinventory::GridParameter* param = dataExists(fieldrequest.refTime, param_name);
  if (!param) {
    METLIBS_LOG_INFO("parameter '" << param_name << "' not found by dataExists");
    return 0;
  }

  std::set<gridinventory::GridParameter>::iterator pitr = ritr->second.parameters.find(*param);
  if (pitr == ritr->second.parameters.end()) {
    METLIBS_LOG_INFO("parameter " << param_name << "  not found in inventory even if dataExists returned true");
    return 0;
  }

  const FieldFunctions::FieldCompute* fcm = extractFunction(pitr->nativekey);
  if (!fcm) {
    // not a computed parameter, read field from GridIO and return
    METLIBS_LOG_INFO(LOGVAL(fieldrequest.ptime));
    Field_p field = getData(fieldrequest.refTime, param->key.name, param->key.zaxis, param->key.taxis, param->key.extraaxis, fieldrequest.plevel,
                            fieldrequest.ptime, fieldrequest.elevel, fieldrequest.time_tolerance);
    return convertUnit(field, fieldrequest.unit);
  }

  // parameter must be computed from input parameters using some function

  Field_pv vfield;    // Input fields
  vfield.reserve(fcm->input.size());

  // special treatment for functions using fields with different forecast time
  if (FieldFunctions::isTimeStepFunction(fcm->function)) {
    FieldRequest fieldrequest_new = fieldrequest;
    fieldrequest_new.unit.clear();
    FieldFunctions::FieldSpec fs;
    updateFieldRequestFromFieldSpec(*fcm, 0, fieldrequest_new, fs);

    if (!fs.fcHour.empty()) {
      int fch = miutil::to_int(fs.fcHour);
      if (!getAllFields_timeInterval(vfield, fieldrequest_new, fch, (fs.option == "accumulate_flux")))
        return nullptr;
    } else {
      if (!getAllFields(vfield, fieldrequest_new, fcm->constants))
        return nullptr;
    }
  } else {

    // loop trough input params with same zaxis
    for (size_t j = 0; j < fcm->input.size(); j++) {
      FieldRequest fieldrequest_new = fieldrequest;
      fieldrequest_new.unit.clear();
      FieldFunctions::FieldSpec fs;
      updateFieldRequestFromFieldSpec(*fcm, j, fieldrequest_new, fs);

      if (!fs.ecoord && !fs.vcoord) {
        if (!addInputField(vfield, getField(fieldrequest_new), *fcm)) {
          METLIBS_LOG_DEBUG("unable to read '" << fcm->input[j] << "'");
          return nullptr;
        }

      } else {

        // vertical- and extra-axis functions.
        const gridinventory::GridParameter* param_new = dataExists(fieldrequest.refTime, fieldrequest_new.paramName);
        if (!param_new) {
          METLIBS_LOG_INFO("parameter '" << fieldrequest_new.paramName << "' not found by dataExists");
          return 0;
        }
        std::set<gridinventory::GridParameter>::iterator pitr_new = ritr->second.parameters.find(*param_new);
        if (pitr_new == ritr->second.parameters.end()) {
          METLIBS_LOG_INFO("parameter " << fieldrequest_new.paramName << "  not found in inventory");
          return 0;
        }

        const std::vector<std::string>* values = nullptr;
        if (fs.ecoord) {
          const gridinventory::ExtraAxis& eaxs = ritr->second.getExtraAxis(pitr_new->key.extraaxis);
          values = &eaxs.stringvalues;
        } else if (fs.vcoord) {
          const gridinventory::Zaxis& zaxs = ritr->second.getZaxis(pitr_new->key.zaxis);
          values = &zaxs.stringvalues;
        }
        if (!values || values->empty())
          return nullptr;
        for (const auto& v : *values) {
          if (fs.ecoord) {
            fieldrequest_new.elevel = v;
          } else if (fs.vcoord) {
            fieldrequest_new.plevel = v;
          }
          if (!addInputField(vfield, getField(fieldrequest_new), *fcm))
            return nullptr;
        }
      }
    } // end loop inputParameters
  }

  if (vfield.empty())
    return nullptr;

  const Field_pv vfresults = createOutputFields(*fcm, vfield, fieldrequest);
  if (!FieldFunctions::fieldComputer(fcm->function, fcm->constants, vfield, vfresults, gc)) {
    METLIBS_LOG_WARN("fieldComputer returned false for '" << (fcm->func ? fcm->func->name : "?") << "'");
    return nullptr;
  }

  return convertUnit(vfresults[0], fieldrequest.unit);
}

bool GridCollection::getActualTime(const std::string& reftime, const std::string& paramname, const miutil::miTime& time, int time_tolerance,
                                   miutil::miTime& actualtime)
{
  typedef std::set<miutil::miTime> times_t;
  const times_t times = getTimes(reftime, paramname);

  const times_t::const_iterator itBest = diutil::nearest_element(times, time, miutil::miTime::minDiff);
  if (itBest == times.end())
    return false;

  actualtime = *itBest;
  return (std::abs(miutil::miTime::minDiff(*itBest, time)) <= time_tolerance);
}

const gridinventory::GridParameter* GridCollection::dataExists_reftime(const gridinventory::ReftimeInventory& reftimeInv, const std::string& paramname)
{
  METLIBS_LOG_SCOPE(LOGVAL(paramname));
  for (const gridinventory::GridParameter& p : reftimeInv.parameters) {
    if (p.key.name == paramname) {
      METLIBS_LOG_DEBUG("found paramname :-)  "<<p.key.name );
      return &p;
    }
  }

  return nullptr;
}



namespace {
struct compare_name {
  compare_name(const std::string& c)
    : compare_to_(c) { }
  bool operator() (const gridinventory::GridParameter& p) const
    { return compare_to_ == p.key.name; }

  const std::string& compare_to_;
};

long getMinStep(const std::set<miutil::miTime>& sorted)
{
  long minValue = LONG_MAX;

  std::set<miutil::miTime>::const_iterator itr1 = sorted.begin();
  std::set<miutil::miTime>::const_iterator itr2 = sorted.begin();
  if (itr2 != sorted.end())
    ++itr2;
  for (; itr2 != sorted.end(); ++itr1, ++itr2) {
    miutil::minimize(minValue, miutil::miTime::hourDiff(*itr2, *itr1));
  }

  return minValue;
}

long getForecastLength(const std::set<miutil::miTime>& sorted)
{
  if (sorted.size() < 2)
    return -1;

  std::set<miutil::miTime>::const_iterator first = sorted.begin();
  std::set<miutil::miTime>::const_reverse_iterator last = sorted.rbegin();

  return miutil::miTime::hourDiff(*last, *first);
}
} // namespace

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

    //check if parameter exists
    std::set<gridinventory::GridParameter>::iterator pitr =
        std::find_if(rinventory.parameters.begin(), rinventory.parameters.end(), compare_name(computeParameterName));
    if (pitr != rinventory.parameters.end()) {
      METLIBS_LOG_DEBUG(LOGVAL(fc.name) << " found in inventory");
      continue;
    }

    METLIBS_LOG_DEBUG(LOGVAL(fc.name) << "not found in inventory");

    //Compute parameter?
    //find input parameters and check

    bool inputOk = true;
    std::string computeZaxis;// = VerticalName[FieldVerticalAxes::vctype_none]; //default, change if input parameter has different zaxis
    int computeZaxisValues = -1;
    std::string computeEaxis;
    // loop trough input params with same zaxis
    std::string fchour;
    FieldFunctions::FieldSpec fs;
    for (const std::string& inputParameterName : boost::adaptors::reverse(fc.input)) {
      //levelSpecified true if param:level=value
      METLIBS_LOG_DEBUG(LOGVAL(inputParameterName));
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

          std::set<gridinventory::Zaxis>::iterator zitr = rinventory.zaxes.find(Zaxis(pitr->zaxis_id));

          // if the funcion is a hybrid function, the variables must have vctype_hybrid or a vertical axis with just one level.
          // The variable with one vertical level are supposed to be the surface pressuer, but there are no futher tests.
          if (fc.vctype == FieldVerticalAxes::vctype_hybrid && zitr->vc_type != FieldVerticalAxes::vctype_hybrid && zitr->values.size() > 1)
            continue;

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
          if (fs.ecoord && pitr->key.extraaxis.empty()) {
            inputOk = false;
            break;
          }
          if (!fs.ecoord && fs.elevel.empty() && pitr->key.extraaxis != "") {
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

      //   check time axis
      if (FieldFunctions::isTimeStepFunction(fc.function)) {
        METLIBS_LOG_DEBUG("check time");
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

        const std::set<miutil::miTime> sorted = getTimes(rinventory.referencetime, pitr->key.name);
        const long minTimeStep = getMinStep(sorted);
        if (minTimeStep != 0 && (minConst % minTimeStep != 0 || maxConst % minTimeStep != 0 || maxConst - minConst > getForecastLength(sorted))) {
          continue;
        }
      }

      //Add computed parameter to inventory
      //computed parameter inherits taxis, extraaxis and grid from input parameter
      gridinventory::GridParameter newparameter = *pitr;
      newparameter.key.name = computeParameterName;
      newparameter.standard_name = computeParameterName;
      if (fc.func) {
        if (!fc.func->units.front().empty())
          newparameter.unit = fc.func->units.front();
        else if (!fs.unit.empty())
          newparameter.unit = fs.unit;
      }
      newparameter.key.zaxis = computeZaxis;
      if (computeZaxis.empty()) {
        newparameter.zaxis_id.clear();
      }
      newparameter.key.taxis.clear();
      newparameter.key.extraaxis = computeEaxis;
      if ( computeEaxis.empty() ) {
        newparameter.extraaxis_id.clear();
      }
      std::ostringstream ost;
      ost << FUNCTION << i << std::endl;
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

bool GridCollection::getAllFields_timeInterval(Field_pv& vfield, FieldRequest fieldrequest, int fch, bool accumulate_flux)
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
  const std::set<miutil::miTime> fieldTimes = getTimes(fieldrequest.refTime, fieldrequest.paramName);
  std::set<miutil::miTime>::const_iterator ip = fieldTimes.begin();
  std::set<miutil::miTime> actualfieldTimes;
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
    Field_p f = getField(fieldrequest);
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

bool GridCollection::getAllFields(Field_pv& vfield, FieldRequest fieldrequest, const std::vector<float>& constants)
{
  const int nConstants = constants.size();
  if (nConstants == 0)
    return false;
  const miutil::miTime startTime = fieldrequest.ptime;
  for (int i = nConstants - 1; i >= 0; i--) {
    fieldrequest.ptime = startTime;
    fieldrequest.ptime.addHour(constants[i]);
    Field_p f = getField(fieldrequest);
    if (f) {
      vfield.push_back(f);
    } else {
      return false;
    }
  }
  return true;
}

// static
bool GridCollection::multiplyFieldByTimeStep(Field_p f, float sec_diff)
{
  const std::vector<float> constants(1, sec_diff);
  const Field_pv vfield(1, f);
  if (FieldFunctions::fieldComputer(FieldFunctions::f_multiply_f_c, constants, vfield, vfield, gc)) {
    return true;
  }
  METLIBS_LOG_WARN("problem in multiplyFieldByTimeStep");
  return false;
}
