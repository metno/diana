/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013 met.no

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

//#define DEBUGPRINT
//#define DEBUGFDIFF
#include "diFieldManager.h"
#include "diMetConstants.h"
#include "GridCollection.h"

#include <puTools/miStringFunctions.h>
#include "puTools/mi_boost_compatibility.hh"
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iomanip>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  /* HAVE_CONFIG_H */

#ifdef FIMEX
#include "FimexIO.h"
#endif

#define MILOGGER_CATEGORY "diField.FieldManager"
#include "miLogger/miLogging.h"

using namespace std;
using namespace miutil;
using namespace MetNo::Constants;

// static class members
GridConverter FieldManager::gc;    // Projection-converter

// Default constructor
FieldManager::FieldManager() :
    fieldcache(new FieldCache())
{
  METLIBS_LOG_SCOPE();
  // test - this has to come by the GUI/setup/log...
  // test with 1 GBYTE
  // test with 0, the fields are deleted after 5 minutes
  try {
    fieldcache->setMaximumsize(1024, FieldCache::MEGABYTE);
  } catch (ModifyFieldCacheException& e) {
    METLIBS_LOG_WARN("createCacheSize(): " << e.what());
  }

  // Initialize setup-types for each GridIO type
#ifdef FIMEX
  gridio_sections[FimexIO::getSourceType()] = FimexIOsetup::getSectionName();
  gridio_setups[FimexIO::getSourceType()] = GridIOsetupPtr(new FimexIOsetup());
#endif
}

FieldManager::~FieldManager()
{
  METLIBS_LOG_SCOPE();
}

std::vector<std::string> FieldManager::subsections()
{
  std::vector<std::string> subs;
  subs.push_back(section());

  // new GridIO structure
  for (gridio_sections_t::const_iterator it_gs = gridio_sections.begin();
      it_gs != gridio_sections.end(); ++it_gs)
    subs.push_back(it_gs->second);

  subs.push_back(FieldFunctions::FIELD_COMPUTE_SECTION());

  subs.push_back(FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION());

  subs.push_back(FieldCache::section());

  return subs;
}

bool FieldManager::parseSetup(const std::vector<std::string>& lines,
    const std::string& token, std::vector<std::string>& errors)
{
  if (lines.empty())
    return true;

  METLIBS_LOG_SCOPE(LOGVAL(token));

  if (token == section())
    return updateFileSetup(lines, errors, true);

  for (gridio_sections_t::iterator sitr = gridio_sections.begin();
      sitr != gridio_sections.end(); ++sitr) {
    if (sitr->second == token) {
      std::map<std::string, GridIOsetupPtr>::iterator itr = gridio_setups.find(
          sitr->first);
      if (itr != gridio_setups.end()) {
        return itr->second->parseSetup(lines, errors);
      }
    }
  }

  if (token == FieldFunctions::FIELD_COMPUTE_SECTION())
    return FieldFunctions::parseComputeSetup(lines, errors);

  if (token == FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION())
    return FieldFunctions::parseVerticalSetup(lines, errors);

  if (token == FieldCache::section()) {
    if (lines.empty()) {
      //      cerr << "Missing section " << FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION()  << " in setupfile." << endl;
      return true;
    }
    return fieldcache->parseSetup(lines, errors);
  }

  return true;
}

void FieldManager::setFieldNames(const std::vector<std::string>& vfieldname)
{
  FieldFunctions::setFieldNames(vfieldname);
}

bool FieldManager::updateFileSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors, bool clearSources, bool top)
{
  METLIBS_LOG_SCOPE();

  if (clearSources) {
    gridSources.clear();
    fieldDialogInfo.clear();
  }

  std::string groupName = "First Group"; // default
  std::string groupType = "fileGroup";

  const int nlines = lines.size();
  for (int l = 0; l < nlines; l++) {
    const std::vector<std::string> tokens = miutil::split_protected(lines[l],
        '"', '"');

    std::string modelName;
    std::string fieldFileType;
    vector<std::string> fileNames;
    vector<std::string> options;
    vector<std::string> format;
    vector<std::string> config;
    std::string guiOptions;
    std::string gridioType = "fimex";
    bool validTimeFromFilename = false;
    bool clearFileGroup = false;
    bool clearFiles = false;

    const int m = tokens.size();
    for (int j = 0; j < m; j++) {
      //delete "Model group" and all models (FieldSource-objects) in the group
      const std::string tu = miutil::to_upper(tokens[j]);
      if (tu == "CLEAR_FILEGROUP") {
        clearFiles = true;
        clearFileGroup = true;
        break;
      }
      //delete all models (FieldSource-objects) in the model group
      if (tu == "CLEAR_FILES") {
        clearFiles = true;
        break;
      }

      if (tu == "OK") { // OK/DELETE: used to skip some models
        continue;
      }

      std::vector<std::string> stokens = miutil::split_protected(tokens[j], '"',
          '"', "=", true);
      if (stokens.size() < 2) {
        std::string error = section() + "|" + miutil::from_number(l)
            + "|Missing argument to keyword: " + tokens[j];
        errors.push_back(error);
        continue;
      }
      std::string key = miutil::to_lower(stokens[0]);
      miutil::remove(key, '"');
      miutil::remove(stokens[1], '"');
      if (key == "default_file") {
        const std::vector<std::string> sstoken = miutil::split(stokens[1], ":");
        if (sstoken.size() == 2) {
          defaultFile[sstoken[0]] = sstoken[1];
        }
      } else if (key == "default_config") {
        const std::vector<std::string> sstoken = miutil::split(stokens[1], ":");
        if (sstoken.size() == 2)
          defaultConfig[sstoken[0]] = sstoken[1];
      } else if (key == "filegroup" || key == "archivefilegroup"
          || key == "profetfilegroup") {
        // group name (only used in dialog)
        groupName = stokens[1];
        groupType = key;
      } else if (key == "m" && modelName.empty()) {
        modelName = stokens[1];
      } else if (key == "t") {
        if ( stokens[1] != "fimex")
          format.push_back(stokens[1]);
      } else if (key == "f") {
        fileNames.push_back(stokens[1]);
      } else if (key == "o") {
        guiOptions = stokens[1];
      } else if (key == "format") {
        format.push_back(stokens[1]);
      } else if (key == "config" || key == "c") {
        config.push_back(stokens[1]);
      } else if (key == "gridiotype") {
        gridioType = stokens[1];
      } else if (key == "time") {
        validTimeFromFilename = (miutil::to_lower(stokens[1]) == "validtime");
      } else {
        options.push_back(tokens[j]);
      }
    }

    //delete models from current model group
    if (clearFiles) {
      std::vector<FieldDialogInfo>::iterator p = fieldDialogInfo.begin();
      while (p != fieldDialogInfo.end() and p->groupName != groupName)
        p++;
      if (p != fieldDialogInfo.end()) {
      if (clearFileGroup) {
        fieldDialogInfo.erase(p); //remove group and models
      } else {
        p->modelNames.clear(); //remove models
      }
    }
  }

    if (!modelName.empty() && (not fileNames.empty())) {

      vector<std::string> vModelNames;
      vector<vector<std::string> > vFileNames;

      if (miutil::contains(modelName, "*")) { // the * is replaced by the filename (without path)

        std::string mpart1, mpart2;
        unsigned int nstar = modelName.find_first_of('*');
        if (nstar > 0)
          mpart1 = modelName.substr(0, nstar);
        if (nstar < modelName.length() - 1)
          mpart2 = modelName.substr(nstar + 1);

        for (size_t j = 0; j < fileNames.size(); j++) {
          glob_t globBuf;
          glob_cache(fileNames[j].c_str(), 0, 0, &globBuf);
          for (size_t k = 0; k < globBuf.gl_pathc; k++) {
            std::string fname = globBuf.gl_pathv[k];
            size_t pb = fname.rfind('/');
            if (pb == string::npos)
              pb = 0;
            else
              pb++;
            modelName = mpart1 + fname.substr(pb) + mpart2;
            vector<std::string> vf;
            vf.push_back(fname);
            vModelNames.push_back(modelName);
            vFileNames.push_back(vf);
          }
          globfree_cache(&globBuf);
        }

      } else {

        vModelNames.push_back(modelName);
        vFileNames.push_back(fileNames);

      }

      unsigned int groupIndex = 0;
      while (groupIndex < fieldDialogInfo.size()
          && fieldDialogInfo[groupIndex].groupName != groupName)
        groupIndex++;
      if (groupIndex == fieldDialogInfo.size()) {
        FieldDialogInfo fdi;
        fdi.groupName = groupName;
        fdi.groupType = groupType;
        if (top) {
          fieldDialogInfo.insert(fieldDialogInfo.begin(), (fdi));
          groupIndex = 0;
        } else {
          fieldDialogInfo.push_back(fdi);
        }
      }

      for (unsigned int n = 0; n < vModelNames.size(); n++) {
        modelName = vModelNames[n];

        //remove old definition
        gridSources.erase(modelName);

        GridIOsetupPtr setup;
        if (gridio_setups.count(gridioType) > 0) {
          setup = gridio_setups[gridioType];
        }
        // make a new GridCollection typically containing one GridIO for each file..
        GridCollectionPtr gridcollection = GridCollectionPtr(
            new GridCollection);
        if (gridcollection->setContents(gridioType, modelName, vFileNames[n],
            format, config, options, setup.get(), validTimeFromFilename)) {
          gridSources[modelName] = gridcollection;
          if (not miutil::contains(miutil::to_lower(guiOptions),
              "notingui")) {
            fieldDialogInfo[groupIndex].modelNames.push_back(modelName);
          }
        } else {
          ostringstream ost;
          ost << section() << "|" << l << "|Bad or no GridIO with type= "
              << gridioType << "  for model= " << modelName;
          errors.push_back(ost.str());
        }

      }
    }
  }
  return true;
}

bool FieldManager::addModels(const std::vector<std::string>& configInfo)
{
  std::vector<std::string> lines;

  BOOST_FOREACH(const std::string& ci, configInfo){

  std::string datasource;
  std::string sourcetype;
  std::string dataset;
  std::string referencetime;
  std::string model;
  std::string guiOption;
  std::string config;
  std::string gridioType = "fimex";
  std::vector<std::string> options;

  std::string file;

  const std::vector<std::string> tokens = miutil::split_protected(ci, '"', '"');
  if (tokens.size() < 3) {
    lines.push_back(ci);
    continue;
  }

  BOOST_FOREACH(const std::string& tok, tokens) {
    std::vector<std::string> stokens= miutil::split_protected(tok, '"', '"', "=", true);
    if (stokens.size()<2) {
      std::cerr << "Missing argument to keyword: '" << tok << "', assuming it is an option" << std::endl;
      options.push_back(tok);
      continue;
    }
    std::string key = miutil::to_lower(stokens[0]);
    miutil::remove(key, '"');
    miutil::remove(stokens[1], '"');
    if (key == "datasource") {
      datasource = stokens[1];
      //        file = stokens[1];
    } else if (key == "sourcetype") {
      sourcetype = stokens[1];
    } else if (key == "dataset" ) {
      dataset = stokens[1];
    } else if (key == "referencetime" ) {
      referencetime = stokens[1];
      miutil::remove(referencetime, ':');
    } else if (key == "model" ) {
      model = stokens[1];
    } else if (key == "o" ) {
      guiOption = stokens[1];
    } else if (key == "gridioType" ) {
      gridioType = miutil::to_lower(stokens[1]);
    } else if (key == "config" || key == "c" ) {
      config = stokens[1];
    } else if (key == "file" ) {
      file = stokens[1];
    } else {
      options.push_back(tok);
    }

  }

  if (config.empty() && defaultConfig.count(sourcetype)>0 ) {
    config = defaultConfig[sourcetype];
  }

  if (file.empty() && defaultFile.count(sourcetype)>0 ) {
    file = defaultFile[sourcetype];
  }

  //make setup string
  if (sourcetype == "wdb" ) {
    ostringstream source;
    source <<"\"file="<<file<<";dataprovider="<<dataset<<";host="<<datasource<<";referencetime="<<referencetime<<"\"";
    file = source.str();
  }

  ostringstream ost;
  ost <<"m="<<model<<" t=" <<sourcetype<< " f="<<file;

  if (not config.empty()) {
    ost <<" config="<<config;
  }

  if (not guiOption.empty()) {
    ost <<" o="<<guiOption;
  }
  
  if (not options.empty()) {
    ost << ' ';
    std::copy(options.begin(), options.end(), std::ostream_iterator<std::string>(ost, " "));
  }

  lines.push_back(ost.str());
}

  bool top = true;
  bool clearsources = false;
  std::vector<std::string> errors;
  return updateFileSetup(lines, errors, clearsources, top);
}

bool FieldManager::modelOK(const std::string& modelName)
{
    GridCollectionPtr pgc = getGridCollection(modelName, "");
    if (not pgc)
      return false;
  return true;
}
void FieldManager::getFieldInfo(const std::string& modelName, const std::string& refTime,
    std::map<std::string,FieldInfo>& fieldInfo)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName)<<LOGVAL(refTime));

  fieldInfo.clear();

  GridCollectionPtr pgc = getGridCollection(modelName, refTime, false, false);
  if (not pgc)
    return;

  gridinventory::Inventory inventory = pgc->getExpandedInventory();

  std::map<std::string, gridinventory::ReftimeInventory>::iterator ritr =
      inventory.reftimes.find(refTime);
  if (ritr == inventory.reftimes.end()) {
    if (refTime.empty() && inventory.reftimes.size()) {
      ritr = inventory.reftimes.begin();
    } else {
      METLIBS_LOG_INFO( " refTime not found: " << LOGVAL(refTime));
      return;
    }
  }

  BOOST_FOREACH(const gridinventory::GridParameter& gp, ritr->second.parameters){
    set<gridinventory::Zaxis>::iterator zitr = ritr->second.zaxes.find(gp.zaxis_id);
//    std::string extraAxis = gp.key.extraaxis;
    FieldInfo vi;
    vi.fieldName = gp.key.name;
    vi.standard_name = gp.standard_name;
    if ( zitr!=ritr->second.zaxes.end() ) {
      vi.vlevels= zitr->getStringValues();
      vi.vcoord = zitr->verticalType;
      if (zitr->vc_type == FieldFunctions::vctype_oceandepth) {
        vi.default_vlevel = vi.vlevels.front();
      }
    }
    set<gridinventory::ExtraAxis>::iterator eitr = ritr->second.extraaxes.find(gp.extraaxis_id);
    if ( eitr!=ritr->second.extraaxes.end() ) {
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


FieldManager::GridCollectionPtr FieldManager::getGridCollection(
    const std::string& modelName, const std::string& refTime, bool rescan,
    bool checkSourceChanged)
{
  METLIBS_LOG_TIME();

  GridSources_t::iterator p = gridSources.find(std::string(modelName));
  if (p == gridSources.end()) {
    METLIBS_LOG_WARN("Undefined model '" << modelName << "'");
    return GridCollectionPtr();
  }

  GridCollectionPtr pgc = p->second;
  if (checkSourceChanged && pgc->sourcesChanged()) {
    rescan = true;
  }

  if (!rescan && pgc->inventoryOk(refTime))
    return pgc;

  // make new inventory
  if (rescan)
    pgc->updateSources();

  pgc->makeInventory(refTime);

  using namespace gridinventory;
  Inventory inventory = pgc->getInventory();
    Inventory::reftimes_t& reftimes = inventory.reftimes;
    for (Inventory::reftimes_t::iterator it_r = reftimes.begin();
        it_r != reftimes.end(); ++it_r) {
      ReftimeInventory& rti = it_r->second;
      addComputedParameters(rti);
    }

  pgc->setExpandedInventory(inventory);
  return pgc;
}

bool FieldManager::addGridCollection(const std::string& gridioType,
    const std::string& modelName, const std::vector<std::string>& filenames,
    const std::vector<std::string>& format,
    const std::vector<std::string>& config,
    const std::vector<std::string>& option)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName));

  GridIOsetupPtr setup;
  if (gridio_setups.count(gridioType) > 0) {
    setup = gridio_setups[gridioType];
  }
  GridCollectionPtr gridcollection = GridCollectionPtr(new GridCollection);

  std::vector<std::string> config_mod(config);
  if (config_mod.empty()) {
    for (size_t i = 0; i < format.size(); ++i) {
      if (defaultConfig.count(format[i]) > 0) {
        config_mod.push_back(defaultConfig[format[i]]);
      }
    }
  }

  if (gridcollection->setContents(gridioType, modelName, filenames, format,
      config_mod, option, setup.get())) {
    gridSources[modelName] = gridcollection;
    return true;
  } else {
    return false;
  }
}

// ==================================================
// ==================================================

std::string FieldManager::mergeTaxisNames(
    gridinventory::ReftimeInventory& inventory, const std::string& taxis1,
    const std::string& taxis2)
{
  // merge time axes
  // if one axis is a subset of the other, return the other
  //if not, merge times, and insert new axis in inventory with name taxs1:taxis2

  if (taxis1.empty()) {
    return taxis2;
  }
  if (taxis2.empty() or taxis1 == taxis2) {
    return taxis1;
  }

  set<gridinventory::Taxis>::iterator titr1 = inventory.taxes.find(taxis1);
  if (titr1 == inventory.taxes.end()) {
    return taxis2;
  }

  set<gridinventory::Taxis>::iterator titr2 = inventory.taxes.find(taxis2);
  if (titr2 == inventory.taxes.end()) {
    return taxis1;
  }

  const vector<double>& times1 = titr1->getSortedValues();
  const vector<double>& times2 = titr2->getSortedValues();

  const bool t1longer = (times2.size() < times1.size());
  const vector<double> &longer = t1longer ? times1 : times2, &shorter =
      t1longer ? times2 : times1;
  std::vector<double> vtmp;
  std::set_difference(shorter.begin(), shorter.end(), longer.begin(),
      longer.end(), std::back_inserter(vtmp));
  if (vtmp.empty()) {
    return t1longer ? taxis2 : taxis1;
  } else {
    const std::string taxis = taxis1 + ":" + taxis2;
    set<gridinventory::Taxis>::iterator titr = inventory.taxes.find(taxis);
    if (titr == inventory.taxes.end()) {
      std::set_intersection(times2.begin(), times2.end(), times1.begin(),
          times1.end(), std::back_inserter(vtmp));
      gridinventory::Taxis taxisnew;
      taxisnew.name = taxis;
      taxisnew.values = vtmp;
      inventory.taxes.insert(taxisnew);
    }
    return taxis;
  }
}

void FieldManager::addComputedParameters(
    gridinventory::ReftimeInventory& inventory)
{
  METLIBS_LOG_SCOPE();

  //add computed parameters to inventory

  // loop through all functions
  int i = -1;
  BOOST_FOREACH(const FieldFunctions::FieldCompute& fc, FieldFunctions::vFieldCompute){
  i += 1;
  const std::string& computeParameterName = fc.name;
  METLIBS_LOG_DEBUG(LOGVAL(fc.name));
  //check if parameter exists
  set<gridinventory::GridParameter>::iterator pitr = inventory.parameters.begin();
  // loop through parameters
  for (; pitr != inventory.parameters.end(); ++pitr) {
    if (pitr->name == computeParameterName) {
      break; //param already exists
    }
  }
  if (pitr != inventory.parameters.end()) {
    break;
  }

  //Compute parameter?
  //find input parameters and check

  bool inputOk = true;
  std::string computeZaxis;// = VerticalName[FieldFunctions::vctype_none]; //default, change if input parameter has different zaxis
  std::string computeTaxis;
  std::string computeEaxis;
  // loop trough input params with same zaxis
  std::string fchour;
  BOOST_REVERSE_FOREACH(std::string inputParameterName, fc.input) {
    //levelSpecified true if param:level=value
    METLIBS_LOG_DEBUG(LOGVAL(inputParameterName));
    std::string inputLevelName;
    FieldFunctions::FieldSpec fs;
    bool levelSpecified = FieldFunctions::splitFieldSpecs(inputParameterName, fs);
    fchour = fs.fcHour;
    pitr = inventory.parameters.begin();
    // loop through parameters
    for (; pitr != inventory.parameters.end(); ++pitr) {
      std::string pitr_name;
      if( fs.use_standard_name ) {
        pitr_name = pitr->standard_name;
      } else {
        pitr_name = pitr->key.name;
      }
      METLIBS_LOG_DEBUG(LOGVAL(pitr_name));

      if (pitr_name ==fs.paramName ) {

        set<gridinventory::Zaxis>::iterator zitr = inventory.zaxes.find(pitr->zaxis_id);

        //ask for level which do not exists
        if (levelSpecified && !zitr->valueExists(fs.levelName)) {
          inputOk = false;
          break;
        }
        //ask for axis which do not exists
        if ( !fs.ecoordName.empty() && pitr->key.extraaxis.empty() ) {
          inputOk = false;
          break;
        }

        if (fs.vcoordName.empty()  && !levelSpecified && computeZaxis.empty()) {
          computeZaxis = pitr->key.zaxis;
        }
        if (fs.ecoordName.empty() && fs.elevel.empty() && pitr->key.extraaxis!="") {
          computeEaxis = pitr->key.extraaxis;
        }
        computeTaxis = mergeTaxisNames(inventory, computeTaxis, pitr->key.taxis);

        break;
      }
    }
    if (pitr == inventory.parameters.end()) {
      inputOk = false;
      break; //can't make this computeParameter
    }
  }

  if (inputOk) {
    METLIBS_LOG_DEBUG("check time");
    //   check time axis
    if (ffunc.isTimeStepFunction(fc.function)) {
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
      set<gridinventory::Taxis>::iterator titr = inventory.taxes.find(pitr->taxis_id);
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
    if ( computeZaxis.empty() ) {
      newparameter.zaxis_id.clear();
    }
    newparameter.key.taxis = computeTaxis;
    newparameter.key.extraaxis = computeEaxis;
    if ( computeEaxis.empty() ) {
      newparameter.extraaxis_id.clear();
    }
    ostringstream ost;
    ost <<"function:"<<i<<endl;
    newparameter.nativekey = ost.str();
    inventory.parameters.insert(newparameter);
    METLIBS_LOG_DEBUG("Add new parameter");
    METLIBS_LOG_DEBUG(LOGVAL(newparameter.key.name) <<LOGVAL(computeTaxis)<<LOGVAL(computeZaxis));
    METLIBS_LOG_DEBUG(LOGVAL(newparameter.nativekey));
    METLIBS_LOG_DEBUG(LOGVAL(newparameter.zaxis_id));
    METLIBS_LOG_DEBUG(LOGVAL(newparameter.key.extraaxis));
    //add parameter with derived zaxis (flightlevel)
    set<gridinventory::Zaxis>::iterator dzitr = inventory.zaxes.find(newparameter.key.zaxis);
//    if (dzitr != inventory.zaxes.end() && !(*dzitr).nativeName.empty() ) {
//      gridinventory::GridParameter derivedparameter = newparameter;
//      newparameter.key.zaxis = (*dzitr).nativeName;
      inventory.parameters.insert(newparameter);
//    }
  } else {
    METLIBS_LOG_DEBUG("not found");
  }
  // } //end parameter loop

} //end function loop

}

gridinventory::Grid FieldManager::getGrid(const std::string& modelName)
{

  std::string reftime = getBestReferenceTime(modelName, 0, -1);
  gridinventory::Grid grid;
  GridCollectionPtr pgc = getGridCollection(modelName, reftime, false);

  if (pgc) {
    grid = pgc->getGrids();
  }

  return grid;
}


std::vector<miutil::miTime> FieldManager::getFieldTime(
    const std::vector<FieldRequest>& fieldrequest,
    bool updateSource)
{
  METLIBS_LOG_SCOPE();
  const int nfields = fieldrequest.size();

  std::set<miTime> tNormal;
  miTime refTime;
  std::set<miTime> tn;
  bool allTimeSteps = false;

  for (int n = 0; n < nfields; n++) {
    const FieldRequest& frq = fieldrequest[n];
    allTimeSteps |= frq.allTimeSteps;

    METLIBS_LOG_DEBUG(LOGVAL(frq.modelName) << LOGVAL(frq.refTime) << LOGVAL(frq.paramName));
    METLIBS_LOG_DEBUG(LOGVAL(frq.zaxis) << LOGVAL(frq.plevel));

    bool gotfieldtime = false;

    GridCollectionPtr pgc = getGridCollection(frq.modelName, frq.refTime,
        updateSource);
    if (pgc) {

      std::string refTimeStr = frq.refTime;
      if (refTimeStr.empty()) {
        refTimeStr = getBestReferenceTime(frq.modelName, frq.refoffset,
            frq.refhour);
        pgc = getGridCollection(frq.modelName, refTimeStr);
      }

      // fetch inventory for this model
      gridinventory::Inventory inventory = pgc->getExpandedInventory();


      //search for referenceTime
      map<std::string, gridinventory::ReftimeInventory>::iterator ritr =
          inventory.reftimes.find(refTimeStr);
      if (ritr == inventory.reftimes.end()) {
        METLIBS_LOG_INFO("refTime '" << refTimeStr << "' not found");
        break;
      }

      // if fieldrequest.paramName is a standard_name, find key.name
      std::string paramName = frq.paramName;
      if (frq.standard_name) {
        set<gridinventory::GridParameter>::iterator pitr;
        pitr = ritr->second.parameters.begin();
        while (pitr != ritr->second.parameters.end()
            && pitr->standard_name != paramName)
          pitr++;
        if (pitr == ritr->second.parameters.end()) {
          METLIBS_LOG_DEBUG(
              __FUNCTION__ << ": parameter standard_name: " << paramName
              << "  not found in inventory");
          continue;
        } else {
          paramName = pitr->key.name;
        }
      }

      // string -> miTime
      time_t t = atof(ritr->second.referencetime.c_str());
      refTime = miTime(t);
      gridinventory::GridParameter param;
      if (pgc->dataExists(refTimeStr, paramName, frq.zaxis,
          frq.taxis, frq.eaxis, frq.version, frq.plevel, frq.ptime,
          frq.elevel, frq.time_tolerance, param)) {

        set<gridinventory::GridParameter>::iterator pitr =
            ritr->second.parameters.find(param);
        if (pitr != ritr->second.parameters.end()) {
          set<gridinventory::Taxis>::iterator titr = ritr->second.taxes.find(
              param.taxis_id);

          tNormal.clear();
          if (titr != ritr->second.taxes.end()) {

            //Is the parameter made from function with several time steps?
            bool timeStepFunc = false;
            int functionIndex = -1;
            if (pitr->nativekey.find(':') != std::string::npos) {
              std::string index = pitr->nativekey.substr(
                  pitr->nativekey.find(':') + 1);
              functionIndex = atoi(index.c_str());
              timeStepFunc = ffunc.isTimeStepFunction(
                  FieldFunctions::vFieldCompute[functionIndex].function);
            }

            set<miTime> setTime;
            if (pgc->useTimeFromFilename()) {
              if (timeStepFunc) {
                setTime = pgc->getTimesFromFilename();
              } else {
                tNormal= pgc->getTimesFromFilename();
              }
            } else {
              vector<double> values = titr->values;
              for (size_t i = 0; i < values.size(); ++i) {
                // double -> miTime
                time_t t = values[i];
                miTime tt(t);

                if (timeStepFunc) {
                  setTime.insert(tt);
                } else {
                  tNormal.insert(tt);
                }
              }
            }

            //Check if all time steps are available
            if (timeStepFunc && FieldFunctions::vFieldCompute[functionIndex].input.size() ) {
              std::vector<float> constants;
              std::string inputParamName = FieldFunctions::vFieldCompute[functionIndex].input[0];
              FieldFunctions::FieldSpec fs;
              FieldFunctions::splitFieldSpecs(inputParamName,fs);
              if ( !fs.fcHour.empty()) {
                constants.push_back(atoi(fs.fcHour.c_str()));
              } else {
                constants = FieldFunctions::vFieldCompute[functionIndex].constants;
              }

              set<miTime>::iterator it = setTime.begin();
              for (; it != setTime.end(); ++it) {
                size_t i = 0;
                for (i = 0; i < constants.size(); ++i) {
                  miTime tmpTime = *it;
                  tmpTime.addHour(constants[i]);
                  if (!setTime.count(tmpTime)) {
                    //time step not available
                    break;
                  }
                }
                if (i == constants.size()) {
                  //all time steps ok
                  tNormal.insert(*it);
                }
              }
            }
            gotfieldtime = true;
          }
        }
      } else {
        METLIBS_LOG_INFO("parameter '" << paramName << "' not found");
        continue;
      }

    }


    if (!gotfieldtime) {
      METLIBS_LOG_INFO(
          "getFieldTime: got no times for model '" << frq.modelName << "'");
    }

    if (gotfieldtime) {
      // ==================================================
      // ==================================================

      if (!tNormal.empty() ) {
        if ((frq.hourOffset != 0 || frq.minOffset != 0) ) {
          set<miTime> twork = tNormal;
          set<miTime>::iterator pt, ptend;
          tNormal.clear();
          for (pt = twork.begin(); pt != twork.end(); pt++) {
            miTime tt = *pt;
            tt.addHour(-frq.hourOffset);
            tt.addMin(-frq.minOffset);
            tNormal.insert(tt);
          }
        }
        if (allTimeSteps) {
          tn.insert(tNormal.begin(), tNormal.end());
        } else if (!tNormal.empty()) {
          if (tn.empty()) {
            tn = tNormal;
          } else {
            vector<miTime> vt(tn.size());
            vector<miTime>::iterator pvt2, pvt1 = vt.begin();
            pvt2 = set_intersection(tn.begin(), tn.end(), tNormal.begin(),
                tNormal.end(), pvt1);
            tn.clear();
            tn = set<miTime>(pvt1, pvt2);
          }
        }
      }

    } else if (!allTimeSteps) {
      tn.clear();
      break;
    }
  }

  vector<miTime> fieldTime;
  if (!tn.empty() ) {
    fieldTime = vector<miTime>(tn.begin(), tn.end());
  }

  return fieldTime;
}

std::set<std::string> FieldManager::getReferenceTimes(const std::string& modelName)
{
  set<std::string> refTimes;
  GridCollectionPtr pgc = getGridCollection(modelName, "", true);
  if (pgc)
    refTimes = pgc->getReferenceTimes();
  return refTimes;
}

std::string FieldManager::getBestReferenceTime(const std::string& modelName,
    int refOffset, int refHour)
{
  set<std::string> refTimes;
  GridCollectionPtr pgc = getGridCollection(modelName, "", true);

  if (pgc)
    refTimes = pgc->getReferenceTimes();

  if (refTimes.empty())
    return "";

  //if refime is not a valid time, return string
  if (!miTime::isValid(*(refTimes.rbegin()))) {
    return (*(refTimes.rbegin()));
  }

  miTime refTime(*(refTimes.rbegin()));

  if (refHour > -1) {
    miDate date = refTime.date();
    miClock clock(refHour, 0, 0);
    refTime = miTime(date, clock);
  }

  refTime.addDay(refOffset);
  std::string refString = refTime.isoTime("T");

  set<std::string>::iterator p = refTimes.begin();
  while (p != refTimes.end() && *p != refString)
    ++p;
  if (p != refTimes.end()) {
    return *p;
  }

  //referencetime not found. If refHour is given and no refoffset, try yesterday
  if (refHour > -1 && refOffset == 0) {
    refTime.addDay(-1);
    refString = refTime.isoTime("T");
    p = refTimes.begin();
    while (p != refTimes.end() && *p != refString)
      ++p;
    if (p != refTimes.end()) {
      return *p;
    }
  }
  return "";
}

bool FieldManager::makeField(Field*& fout, FieldRequest fieldrequest,
    int cacheOptions)
{
  METLIBS_LOG_TIME(LOGVAL(fieldrequest.modelName) << LOGVAL(fieldrequest.paramName)
      << LOGVAL(fieldrequest.zaxis) << LOGVAL(fieldrequest.refTime)
      << LOGVAL(fieldrequest.ptime) << LOGVAL(fieldrequest.plevel)
      << LOGVAL(fieldrequest.elevel) << LOGVAL(fieldrequest.unit));

  //Find best reference time
  if (fieldrequest.refTime.empty()) {
    fieldrequest.refTime = getBestReferenceTime(fieldrequest.modelName,
        fieldrequest.refoffset, fieldrequest.refhour);
  }

  GridCollectionPtr pgc = getGridCollection(fieldrequest.modelName, fieldrequest.refTime,
      false, fieldrequest.checkSourceChanged);
  if (!pgc) {
    METLIBS_LOG_WARN("could not find grid collection for model=" << fieldrequest.modelName
        << " reftime=" << fieldrequest.refTime);
    return false;
  }

  // fetch inventory for this model
  gridinventory::Inventory inventory = pgc->getExpandedInventory();

  //search for referenceTime
  map<std::string, gridinventory::ReftimeInventory>::iterator ritr;
  if (fieldrequest.refTime.empty()) {
    ritr = inventory.reftimes.begin();
  } else {
    ritr = inventory.reftimes.find(fieldrequest.refTime);
  }
  if (ritr == inventory.reftimes.end()) {
    METLIBS_LOG_WARN("refTime not found: " << fieldrequest.refTime);
    return false;
  }
  fieldrequest.refTime = ritr->second.referencetime;

  fout = getField(pgc, ritr->second, fieldrequest, cacheOptions);

  if (fout == 0) {
    METLIBS_LOG_WARN(
        "No field read: model: " << fieldrequest.modelName << ", parameter:"
            << fieldrequest.paramName << ", level:" << fieldrequest.plevel
            << ", time:" << fieldrequest.ptime << ", referenceTime:"
            << fieldrequest.refTime);
    return false;
  }

  fout->leveltext = fieldrequest.plevel;
  fout->idnumtext = fieldrequest.elevel;
  fout->paramName = fieldrequest.paramName;
  fout->modelName = fieldrequest.modelName;

  if (!fieldrequest.palette.empty()) {
    fout->palette = pgc->getVariable(fieldrequest.refTime, fieldrequest.palette);
  }

  return true;
}

bool FieldManager::freeField(Field* field)
{
  try {
    fieldcache->freeField(field);
    return true;
  } catch (ModifyFieldCacheException&) {
    return false;
  }
}

/*
 YE: It seems that the ptime is not used to determine which fields to be used when computing, why?
 */

void FieldManager::writeToCache(Field*& fout)
{
  METLIBS_LOG_SCOPE();

  FieldCacheKeyset fkey(fout);

  // check if in cache
  if (!fieldcache->hasField(fkey)) {
    METLIBS_LOG_DEBUG(
        "FieldManager: Adding field " << fkey << " with size "
            << fout->bytesize() << " to cache");
    try {
      fieldcache->set(fout, true);
    } catch (ModifyFieldCacheException& e) {
      METLIBS_LOG_INFO(e.what());
    }
  } else {
    METLIBS_LOG_WARN(
        "FieldManager: Replacing field " << fkey << " with size "
            << fout->bytesize() << " to cache");
    try {
      fieldcache->replace(fkey, fout);
    } catch (ModifyFieldCacheException& e) {
      METLIBS_LOG_INFO(e.what());
    }
    delete fout;
    fout = fieldcache->get(fkey);
  }
}

bool FieldManager::makeDifferenceFields(std::vector<Field*> & fv1,
    std::vector<Field*> & fv2)
{
  unsigned int dim = fv1.size();
  if (fv2.size() != dim)
    return false; // must have same dimentions

  //change projection
  const GridArea& area1 = fv1[0]->area;
  const GridArea& area2 = fv2[0]->area;
  const bool differentGrid = (area1 != area2);
  bool res = true;
  if (differentGrid) {
    unsigned int j = 0;
    while (res && j < dim) {
      res = fv2[j]->changeGrid(area1, "simple.interpolation");
      j++;
    }
    if (res && dim == 2) {
      float *x, *y;
      gc.getGridPoints(area1, area2, false, &x, &y);
      int npos = fv2[0]->area.gridSize();
      res = gc.getVectors(area2, area1, npos, x, y, fv2[0]->data, fv2[1]->data);
    }
  }

  //Subtract fields
  unsigned int j = 0;
  while (res && j < dim) {
    res = fv1[j]->subtract(*(fv2[j]));
    j++;
  }

  if (res) {
    for (j = 0; j < dim; j++)
      fv1[j]->difference = true;

    if (fv1[0]->validFieldTime < fv2[0]->validFieldTime)
      fv1[0]->validFieldTime = fv2[0]->validFieldTime;   // or maybe not ???

    if (fv1[0]->analysisTime < fv2[0]->analysisTime)
      fv1[0]->analysisTime = fv2[0]->analysisTime;       // or maybe not ???

  } else {
    for (unsigned int i = 0; i < fv1.size(); i++) {
      fieldcache->freeField(fv1[i]);
      fv1[i] = NULL;
    }
    fv1.clear();
  }

  for (unsigned int i = 0; i < fv2.size(); i++) {
    fieldcache->freeField(fv2[i]);
    fv2[i] = NULL;
  }

  return res;
}

// remove parentheses from end of string
std::string FieldManager::removeParenthesesFromString(const std::string& origName)
{
  const char* optionFirst = "([{<";
  const char* optionLast = ")]}>";

  std::string name = origName;

  std::string::size_type l = name.length() - 1;
  int i = 0;

  while (i < 4 && name[l] != optionLast[i])
    i++;
  if (i < 4) {
    if (((l = name.find_last_of(optionFirst[i])) != string::npos) && l > 0)
      name = name.substr(0, l);
  }

  return name;
}

bool FieldManager::writeField(FieldRequest fieldrequest, const Field* field)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldrequest.modelName) << LOGVAL(fieldrequest.paramName)
      << LOGVAL(fieldrequest.zaxis) << LOGVAL(fieldrequest.refTime)
      << LOGVAL(fieldrequest.ptime) << LOGVAL(fieldrequest.plevel)
      << LOGVAL(fieldrequest.elevel) << LOGVAL(fieldrequest.unit));

  GridCollectionPtr pgc = getGridCollection(fieldrequest.modelName,
      fieldrequest.refTime, false, fieldrequest.checkSourceChanged);
  if (not pgc) {
    COMMON_LOG::getInstance("common").warnStream()
        << "FieldManager::putField: grid collection for model '"
        << fieldrequest.modelName << "', refTime '" << fieldrequest.refTime
        << "' not found";
    return false;
  }

  gridinventory::Inventory inventory = pgc->getExpandedInventory();


  map<std::string, gridinventory::ReftimeInventory>::iterator ritr =
      inventory.reftimes.find(fieldrequest.refTime);
  if (ritr == inventory.reftimes.end()) {
    if (fieldrequest.refTime.empty() && inventory.reftimes.size()) {
      ritr = inventory.reftimes.begin();
    } else {
      COMMON_LOG::getInstance("common").warnStream() << "refTime '"
          << fieldrequest.refTime << "' not found";
      return false;
    }
  }
  fieldrequest.refTime = ritr->second.referencetime;

  gridinventory::GridParameter param;
  if (!paramExist(ritr->second, fieldrequest, param)) {
    METLIBS_LOG_DEBUG(
        __FUNCTION__ << ": parameter " << fieldrequest.paramName
            << "  not found in inventory");
    return false;
  }
  return pgc->putData(fieldrequest.refTime,
      param.key.name, param.key.zaxis, param.key.taxis, param.key.extraaxis,
      param.key.version, fieldrequest.plevel, fieldrequest.ptime,
      fieldrequest.elevel, fieldrequest.unit, fieldrequest.output_time,
      fieldrequest.time_tolerance, field);
}

Field* FieldManager::getField(GridCollectionPtr gridCollection,
    gridinventory::ReftimeInventory& inventory, FieldRequest fieldrequest,
    int cacheOptions)
{
  METLIBS_LOG_TIME("SEARCHING FOR :" << fieldrequest.paramName << " : "
      << fieldrequest.zaxis << " : " << fieldrequest.plevel);

  Field* field = 0;
  // if fieldrequest.paramName is a standard_name, find key.name
  if (fieldrequest.standard_name) {
    set<gridinventory::GridParameter>::iterator pitr;
    pitr = inventory.parameters.begin();
    while (pitr != inventory.parameters.end()
        && pitr->standard_name != fieldrequest.paramName)
      pitr++;
    if (pitr == inventory.parameters.end()) {
      METLIBS_LOG_DEBUG(
          __FUNCTION__ << ": parameter standard_name: "
              << fieldrequest.paramName << "  not found in inventory");
      return NULL;
    } else {
      fieldrequest.paramName = pitr->key.name;
    }
  }

  // If READ_RESULT, try to read from cache
  if ((cacheOptions & (READ_RESULT | READ_ALL)) != 0
      && miTime::isValid(fieldrequest.refTime) && !fieldrequest.ptime.undef()) {
    FieldCacheKeyset keyset;
    miTime reftime(fieldrequest.refTime);
    keyset.setKeys(fieldrequest.modelName, reftime, fieldrequest.paramName,
        fieldrequest.plevel, fieldrequest.elevel, fieldrequest.ptime);

    METLIBS_LOG_DEBUG("SEARCHING FOR :" << keyset << " in CACHE");

    if (fieldcache->hasField(keyset)) {
      return fieldcache->get(keyset);
    }

  }

  //check if requested parameter exist, and init param
  gridinventory::GridParameter param;

  // check if param is in inventory

  if (!gridCollection->dataExists(fieldrequest.refTime,
      fieldrequest.paramName, fieldrequest.zaxis, fieldrequest.taxis,
      fieldrequest.eaxis, fieldrequest.version, fieldrequest.plevel,
      fieldrequest.ptime, fieldrequest.elevel, fieldrequest.time_tolerance,
      param)) {
    METLIBS_LOG_INFO(
        __FUNCTION__ << ": parameter " << fieldrequest.paramName
            << "  not found by dataExists");
    return NULL;
  }

  set<gridinventory::GridParameter>::iterator pitr;
  pitr = inventory.parameters.find(param);
  if (pitr == inventory.parameters.end()) {
    METLIBS_LOG_INFO("parameter " << fieldrequest.paramName
        << "  not found in inventory even if dataExists returned true");
    return NULL;
  }

  //If not computed parameter, read field from GridCollection and return
  if (pitr->nativekey.find("function:") == std::string::npos) {

    field = gridCollection->getData(fieldrequest.refTime, param.key.name, param.key.zaxis, param.key.taxis,
        param.key.extraaxis, param.key.version, fieldrequest.plevel,
        fieldrequest.ptime, fieldrequest.elevel, fieldrequest.unit,
        fieldrequest.time_tolerance);
    if ((cacheOptions & (WRITE_ALL | WRITE_RESULT)) != 0) {
      writeToCache(field);
    }

    return field;
  }

  //parameter must be computed from input parameters using some function

  //Find function, number of input parameters and constants
  std::string index = pitr->nativekey.substr(pitr->nativekey.find(':') + 1);
  int functionIndex = atoi(index.c_str());
  int nInputParameters =
      FieldFunctions::vFieldCompute[functionIndex].input.size();
  int nOutputParameters =
      FieldFunctions::vFieldCompute[functionIndex].results.size();
  vector<Field*> vfield; // Input fields
  vector<Field*> vfresults; //Output fields
  bool fieldOK = false;

//Functions using fields with different forecast time
  if (ffunc.isTimeStepFunction(
                    FieldFunctions::vFieldCompute[functionIndex].function)){

    FieldRequest fieldrequest_new = fieldrequest;
    std::string inputParamName =
        FieldFunctions::vFieldCompute[functionIndex].input[0];

    //levelSpecified true if param:level=value
    FieldFunctions::FieldSpec fs;
    bool levelSpecified = FieldFunctions::splitFieldSpecs(inputParamName,fs);
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

    if( !fs.fcHour.empty()) {
      int fch=atoi(fs.fcHour.c_str());
      if ( !getAllFields_timeInterval(gridCollection, inventory, vfield, fieldrequest_new,
          fch, (fs.option == "accumulate_flux"), cacheOptions)){
        return NULL;
      }
    } else {
      if ( !getAllFields(gridCollection, inventory, vfield, fieldrequest_new,
          FieldFunctions::vFieldCompute[functionIndex].constants, cacheOptions)){
        return NULL;
      }
    }

    //make output field
    Field* ff = NULL;
    ff = new Field();
    ff->shallowMemberCopy(*vfield[0]);
    ff->reserve(vfield[0]->area.nx, vfield[0]->area.ny);
    ff->validFieldTime = fieldrequest.ptime;
    ff->unit = fieldrequest.unit;
    vfresults.push_back(ff);

    if (!ffunc.fieldComputer(FieldFunctions::functionMap[FieldFunctions::vFieldCompute[functionIndex].function],
        FieldFunctions::vFieldCompute[functionIndex].constants,
        vfield, vfresults, gc)) {
      METLIBS_LOG_WARN("FieldManager::getField: fieldComputer returned false");
      fieldOK = false;
    } else {
      fieldOK = true;
    }

  } else {

    std::string computeZaxis; // = VerticalName[FieldFunctions::vctype_none]; //default, change if input parameter has different zaxis

    // loop trough input params with same zaxis
    for (int j = 0; j < nInputParameters; j++) {

      FieldRequest fieldrequest_new = fieldrequest;
      std::string inputParamName =
          FieldFunctions::vFieldCompute[functionIndex].input[j];

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
      fieldrequest_new.unit = fs.unit;
      if (!fs.elevel.empty()) {
        fieldrequest_new.elevel = fs.elevel;
      }

      if ( fs.ecoordName.empty() && fs.vcoordName.empty() ){
        Field * f = getField(gridCollection, inventory, fieldrequest_new,
            cacheOptions);
        if (f == NULL) {
          METLIBS_LOG_DEBUG(
              "FieldManager::getField: unable to read: " << inputParamName);
          return field;
        } else {
          vfield.push_back(f);
        }


      } else {

        vector<std::string> values;
        if ( !fs. ecoordName.empty() ){
          gridinventory::ExtraAxis eaxs = inventory.getExtraAxis(fs.ecoordName);
          values = eaxs.stringvalues;
        } else if ( !fs.vcoordName.empty() ){
          gridinventory::Zaxis zaxs = inventory.getZaxis(fs.vcoordName);
          values = zaxs.stringvalues;
        }
        fieldrequest.paramName = fs.paramName;
        if ( values.size() ==  0 ) {
          return NULL;
        }
        for (size_t i = 0; i < values.size(); i++) {
          if ( !fs.ecoordName.empty() ){
            fieldrequest.elevel = values[i];
          } else if ( !fs.vcoordName.empty() ){
            fieldrequest.plevel = values[i];
          }
          Field * f = getField(gridCollection, inventory, fieldrequest,
              cacheOptions);

          if (f == NULL) {
            return NULL;
          } else {
            vfield.push_back(f);
          }
        }

      }

    } //end loop inputParameters

    if (vfield.size() == 0) {
      return NULL;
    }

    for (int j = 0; j < nOutputParameters; j++) {
      Field* ff = NULL;
      ff = new Field();
      ff->shallowMemberCopy(*vfield[0]);
      ff->reserve(vfield[0]->area.nx, vfield[0]->area.ny);
      ff->unit = fieldrequest.unit;
      vfresults.push_back(ff);
    }

    if (!ffunc.fieldComputer(FieldFunctions::vFieldCompute[functionIndex].function,
        FieldFunctions::vFieldCompute[functionIndex].constants, vfield,
        vfresults, gc)) {
      METLIBS_LOG_WARN("FieldManager::getField: fieldComputer returned false");
      fieldOK = false;
    } else {
      fieldOK = true;
    }

  }

  //delete input fields
  for (size_t i = 0; i < vfield.size(); ++i) {
    fieldcache->freeField(vfield[i]);
  }

  //return output field
  if (fieldOK) {
    return vfresults[0];
  } else {
    return NULL;
  }

  return NULL;
}

bool FieldManager::paramExist(gridinventory::ReftimeInventory& inventory,
    const FieldRequest& fieldrequest, gridinventory::GridParameter& param)
{
  //METLIBS_LOG_DEBUG(__FUNCTION__);

  // init Gridparameter
  param.key.name = fieldrequest.paramName;
  param.key.zaxis = fieldrequest.zaxis;
  param.key.taxis = fieldrequest.taxis;
  param.key.extraaxis = fieldrequest.eaxis;
  param.key.version = fieldrequest.version;
  METLIBS_LOG_DEBUG("param.key.name: " << param.key.name);
  METLIBS_LOG_DEBUG("param.key.zaxis: " << param.key.zaxis);
  METLIBS_LOG_DEBUG("param.key.taxis: " << param.key.taxis);
  METLIBS_LOG_DEBUG("param.key.extraaxis: " << param.key.extraaxis);
  METLIBS_LOG_DEBUG("param.grid: " << param.grid);

  set<gridinventory::GridParameter>::iterator pitr = inventory.parameters.find(
      param);

  //Parameter ok
  if (pitr != inventory.parameters.end()) {
    param = *pitr;
    return true;
  }

  return false;
}

void FieldManager::updateSources()
{
  for (GridSources_t::iterator it_gs = gridSources.begin();
      it_gs != gridSources.end(); ++it_gs)
    it_gs->second->updateSources();
}

void FieldManager::updateSource(const std::string& modelName)
{
  getGridCollection(modelName, "", true);
}

std::vector<std::string> FieldManager::getFileNames(
    const std::string& modelName)
{
  METLIBS_LOG_SCOPE();
  std::vector<std::string> filenames;
    GridCollectionPtr gridCollection = getGridCollection(modelName, "", true);
    if (gridCollection)
      filenames = gridCollection->getRawSources();
  return filenames;
}


bool FieldManager::getAllFields_timeInterval(GridCollectionPtr gridCollection,
    gridinventory::ReftimeInventory& inventory, vector<Field*>& vfield,
    FieldRequest fieldrequest, int fch, bool accumulate_flux, int cacheOptions)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldrequest.paramName));

  // get all available timesteps between start and end.
  vector<FieldRequest> fieldrequests;
  fieldrequests.push_back(fieldrequest);
  vector<miTime> fieldTimes = getFieldTime(fieldrequests,false);
  miTime endTime = fieldrequest.ptime;
  miTime startTime = fieldrequest.ptime;
  startTime.addHour(fch);
  set<miTime> actualfieldTimes;
  for (size_t i = 0; i < fieldTimes.size(); i++) {
    if (fieldTimes[i] >= startTime && fieldTimes[i] <= endTime)
    {
      actualfieldTimes.insert(fieldTimes[i]);
    }
  }

  if ( !actualfieldTimes.count(startTime))
    return false;

  if ( accumulate_flux ) {
    set<miTime>::iterator ip = actualfieldTimes.begin();
    miTime lastTime = *ip;
    ++ip;
    for (; ip != actualfieldTimes.end(); ip++) {
      fieldrequest.ptime = *ip;
      Field * f = getField(gridCollection, inventory, fieldrequest, cacheOptions);
      if (f == NULL) {
        return false;
      } else {
        float sec_diff = miTime::secDiff(*ip, lastTime);
        if ( !multiplyFieldByTimeStep(f, sec_diff) )
          return false;
        vfield.push_back(f);
        lastTime = *ip;
      }
    }
  } else {
    BOOST_FOREACH(miTime t,  actualfieldTimes) {
      fieldrequest.ptime = t;
      Field * f = getField(gridCollection, inventory, fieldrequest, cacheOptions);
      if (f == NULL) {
        return false;
      } else {
        vfield.push_back(f);
      }
    }
  }
  return true;

}

bool FieldManager::getAllFields(GridCollectionPtr gridCollection,
    gridinventory::ReftimeInventory& inventory, std::vector<Field*>& vfield,
    FieldRequest fieldrequest, const std::vector<float>& constants, int cacheOptions)
{
  int nConstants = constants.size();
  miTime startTime= fieldrequest.ptime;
  if ( nConstants == 0 )
    return false;
  for (int i = nConstants - 1; i >= 0; i--) {
    fieldrequest.ptime = startTime;
    fieldrequest.ptime.addHour(constants[i]);
    Field * f = getField(gridCollection, inventory, fieldrequest,
        cacheOptions);
    if (f == NULL) {
      return false;
    } else {
      vfield.push_back(f);
    }
  }
  return true;
}

bool FieldManager::multiplyFieldByTimeStep(Field* f, float sec_diff)
{
  vector<float> constants;
  constants.push_back(sec_diff);
  vector<Field *> vfield;
  vfield.push_back(f);
  if (ffunc.fieldComputer(FieldFunctions::f_multiply_f_c, constants, vfield, vfield, gc)) {
    return true;
  }
  METLIBS_LOG_WARN("fieldComputer returned false");
  return false;
}
