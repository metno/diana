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

#include "../diUtilities.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

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
      std::map<std::string, GridIOsetupPtr>::iterator itr = gridio_setups.find(sitr->first);
      if (itr != gridio_setups.end()) {
        return itr->second->parseSetup(lines, errors);
      }
    }
  }

  if (token == FieldFunctions::FIELD_COMPUTE_SECTION())
    return FieldFunctions::parseComputeSetup(lines, errors);

  if (token == FieldFunctions::FIELD_VERTICAL_COORDINATES_SECTION())
    return FieldFunctions::parseVerticalSetup(lines, errors);

  if (token == FieldCache::section())
    return fieldcache->parseSetup(lines, errors);

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
    const std::vector<std::string> tokens = miutil::split_protected(lines[l], '"', '"');

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

      std::vector<std::string> stokens = miutil::split_protected(tokens[j], '"', '"', "=", true);
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
          || key == "profetfilegroup")
      {
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
          const diutil::string_v matches = diutil::glob(fileNames[j], 0);
          for (size_t k = 0; k < matches.size(); k++) {
            const std::string& fname = matches[k];
            size_t pb = fname.rfind('/');
            if (pb == string::npos)
              pb = 0;
            else
              pb++;
            modelName = mpart1 + fname.substr(pb) + mpart2;
            vModelNames.push_back(modelName);
            vector<std::string> vf(1, fname);
            vFileNames.push_back(vf);
          }
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
          fieldDialogInfo.insert(fieldDialogInfo.begin(), fdi);
          groupIndex = 0;
        } else {
          fieldDialogInfo.push_back(fdi);
        }
      }

      for (unsigned int n = 0; n < vModelNames.size(); n++) {
        const std::string& mn = vModelNames[n];

        //remove old definition
        gridSources.erase(mn);

        GridIOsetupPtr setup;
        if (gridio_setups.count(gridioType) > 0) {
          setup = gridio_setups[gridioType];
        }
        // make a new GridCollection typically containing one GridIO for each file..
        GridCollectionPtr gridcollection(new GridCollection);
        if (gridcollection->setContents(gridioType, mn, vFileNames[n],
            format, config, options, setup.get(), validTimeFromFilename))
        {
          gridSources[mn] = gridcollection;
          if (!miutil::contains(miutil::to_lower(guiOptions), "notingui")) {
            fieldDialogInfo[groupIndex].modelNames.push_back(mn);
            fieldDialogInfo[groupIndex].setupInfo.push_back(lines[l]);
          }
        } else {
          ostringstream ost;
          ost << section() << "|" << l << "|Bad or no GridIO with type= "
              << gridioType << "  for model='" << mn << "'";
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

  for (const std::string& ci : configInfo) {

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

    for (const std::string& tok : tokens) {
      std::vector<std::string> stokens= miutil::split_protected(tok, '"', '"', "=", true);
      if (stokens.size()<2) {
        METLIBS_LOG_INFO("Missing argument to keyword: '" << tok << "', assuming it is an option");
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

std::map<std::string,std::string> FieldManager::getGlobalAttributes(const std::string& modelName, const std::string& refTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName)<<LOGVAL(refTime));

  if (GridCollectionPtr pgc = getGridCollection(modelName, refTime, false, false))
    return pgc->getGlobalAttributes(refTime);
  else
    return std::map<std::string,std::string>();
}


void FieldManager::getFieldInfo(const std::string& modelName, const std::string& refTime,
    std::map<std::string,FieldInfo>& fieldInfo)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName)<<LOGVAL(refTime));

  fieldInfo.clear();

  if (GridCollectionPtr pgc = getGridCollection(modelName, refTime, false, false))
    pgc->getFieldInfo(refTime, fieldInfo);
}


FieldManager::GridCollectionPtr FieldManager::getGridCollection(
    const std::string& modelName, const std::string& refTime, bool rescan,
    bool checkSourceChanged)
{
  METLIBS_LOG_TIME();

  GridSources_t::iterator p = gridSources.find(modelName);
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
  if (rescan && !pgc->updateSources())
    return pgc;

  pgc->makeInventory(refTime);

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


    GridCollectionPtr pgc = getGridCollection(frq.modelName, frq.refTime, updateSource);
    if (pgc) {

      std::string refTimeStr = frq.refTime;
       if (refTimeStr.empty()) {
         refTimeStr = getBestReferenceTime(frq.modelName, frq.refoffset, frq.refhour);
         pgc = getGridCollection(frq.modelName, refTimeStr);
       }

      std::string paramName = frq.paramName;
      // if fieldrequest.paramName is a standard_name, find variable_name
      if (frq.standard_name) {
        if ( !pgc->standardname2variablename(refTimeStr, frq.paramName, paramName) )
          continue;
      }
      tNormal = pgc->getTimes(refTimeStr, paramName);
    }
    METLIBS_LOG_DEBUG(LOGVAL(tNormal.size()));

    if (!tNormal.empty() ) {
      if ((frq.hourOffset != 0 || frq.minOffset != 0) ) {
        set<miTime> twork;
        for (set<miTime>::iterator pt = tNormal.begin(); pt != tNormal.end(); pt++) {
          miTime tt = *pt;
          tt.addHour(-frq.hourOffset);
          tt.addMin(-frq.minOffset);
          twork.insert(tt);
        }
        std::swap(twork, tNormal);
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
          tn = set<miTime>(pvt1, pvt2);
        }
      }

    }
  }

  return vector<miTime>(tn.begin(), tn.end());
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

  GridCollectionPtr pgc = getGridCollection(fieldrequest.modelName, fieldrequest.refTime,
      false, fieldrequest.checkSourceChanged);
  if (!pgc) {
    METLIBS_LOG_WARN("could not find grid collection for model=" << fieldrequest.modelName
        << " reftime=" << fieldrequest.refTime);
    return false;
  }

  fout = pgc->getField(fieldrequest);

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

  if ((cacheOptions & (WRITE_ALL | WRITE_RESULT)) != 0) {
    writeToCache(fout);
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

bool FieldManager::freeFields(std::vector<Field*>& fields)
{
  METLIBS_LOG_SCOPE();
  bool all_ok = true;

  for (std::vector<Field*>::iterator it = fields.begin(); it != fields.end(); ++it) {
    all_ok &= freeField(*it); // FIXME what to do with those that cannot be deleted?
  }
  fields.clear();
  return all_ok;
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
      res = fv2[j]->changeGrid(area1, false);
      j++;
    }
    if (res && dim == 2) {
      float *x, *y;
      gc.getGridPoints(area1, area2, false, &x, &y);
      int npos = fv2[0]->area.gridSize();
      res = gc.getVectors(area2, area1.P(), npos, x, y, fv2[0]->data, fv2[1]->data);
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
    freeFields(fv1);
  }

  freeFields(fv2);

  return res;
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
    METLIBS_LOG_WARN(LOGVAL(fieldrequest.modelName) << LOGVAL(fieldrequest.refTime) << "' not found");
    return false;
  }

  return pgc->putData(fieldrequest.refTime,
      fieldrequest.paramName, fieldrequest.plevel, fieldrequest.ptime,
      fieldrequest.elevel, fieldrequest.unit, fieldrequest.output_time,
      field);
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

std::vector<std::string> FieldManager::getFileNames(const std::string& modelName)
{
  METLIBS_LOG_SCOPE();
  std::vector<std::string> filenames;
  GridCollectionPtr gridCollection = getGridCollection(modelName, "", true);
  if (gridCollection)
    filenames = gridCollection->getRawSources();
  return filenames;
}
