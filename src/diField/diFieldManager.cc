/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013-2021 met.no

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

#include "diana_config.h"

#include "diFieldManager.h"

#include "GridCollection.h"
#include "diField.h"
#include "miSetupParser.h"

#include "../diFieldUtil.h"
#include "../diUtilities.h"
#include "util/misc_util.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "diana_config.h"

#ifdef FIMEX
#include "FimexIO.h"
#endif

#define MILOGGER_CATEGORY "diField.FieldManager"
#include "miLogger/miLogging.h"

using namespace miutil;
using namespace miutil::constants;

namespace {

const std::string FIELD_FILES = "FIELD_FILES";

} // namespace

// static class members
GridConverter FieldManager::gc;    // Projection-converter

FieldManager::FieldManager()
{
  METLIBS_LOG_SCOPE();

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
  subs.push_back(FIELD_FILES);

  // new GridIO structure
  for (gridio_sections_t::const_iterator it_gs = gridio_sections.begin();
      it_gs != gridio_sections.end(); ++it_gs)
    subs.push_back(it_gs->second);

  subs.push_back(FieldFunctions::FIELD_COMPUTE_SECTION());

  subs.push_back(FieldVerticalAxes::FIELD_VERTICAL_COORDINATES_SECTION());

  return subs;
}

bool FieldManager::parseSetup()
{
  // Parse field sections
  std::vector<std::string> errors;
  std::vector<std::string> lines;
  for (const std::string& suse : subsections()) {
    SetupParser::getSection(suse, lines);
    parseSetup(lines, suse, errors);
  }
  // Write error messages
  int nerror = errors.size();
  for (int i = 0; i < nerror; i++) {
    std::vector<std::string> token = miutil::split(errors[i], "|");
    SetupParser::errorMsg(token[0], atoi(token[1].c_str()), token[2]);
  }

  return true; // FIXME this ignores errors
}

bool FieldManager::parseSetup(const std::vector<std::string>& lines,
    const std::string& token, std::vector<std::string>& errors)
{
  if (lines.empty())
    return true;

  METLIBS_LOG_SCOPE(LOGVAL(token));

  if (token == FIELD_FILES)
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

  if (token == FieldVerticalAxes::FIELD_VERTICAL_COORDINATES_SECTION())
    return FieldVerticalAxes::parseVerticalSetup(lines, errors);

  return true;
}

bool FieldManager::updateFileSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors, bool clearSources, bool top)
{
  METLIBS_LOG_SCOPE();

  if (clearSources) {
    gridSources.clear();
    fieldModelGroups.clear();
  }

  std::string groupName = "First Group"; // default
  std::string groupType = "fileGroup";

  const int nlines = lines.size();
  for (int l = 0; l < nlines; l++) {
    const std::vector<std::string> tokens = miutil::split_protected(lines[l], '"', '"');

    std::string modelName;
    std::string fieldFileType;
    std::vector<std::string> fileNames;
    std::vector<std::string> options;
    std::vector<std::string> format;
    std::vector<std::string> config;
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
        std::string error = FIELD_FILES + "|" + miutil::from_number(l) + "|Missing argument to keyword: " + tokens[j];
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
      FieldModelGroupInfo_v::iterator p = fieldModelGroups.begin();
      while (p != fieldModelGroups.end() and p->groupName != groupName)
        p++;
      if (p != fieldModelGroups.end()) {
        if (clearFileGroup) {
          fieldModelGroups.erase(p); // remove group and models
        } else {
          p->models.clear(); // remove models
        }
      }
    }

    if (!modelName.empty() && (not fileNames.empty())) {

      std::vector<std::string> vModelNames;
      std::vector<std::vector<std::string> > vFileNames;

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
            if (pb == std::string::npos)
              pb = 0;
            else
              pb++;
            modelName = mpart1 + fname.substr(pb) + mpart2;
            vModelNames.push_back(modelName);
            std::vector<std::string> vf(1, fname);
            vFileNames.push_back(vf);
          }
        }

      } else {

        vModelNames.push_back(modelName);
        vFileNames.push_back(fileNames);

      }

      unsigned int groupIndex = 0;
      while (groupIndex < fieldModelGroups.size() && fieldModelGroups[groupIndex].groupName != groupName)
        groupIndex++;
      if (groupIndex == fieldModelGroups.size()) {
        FieldModelGroupInfo fdi;
        fdi.groupName = groupName;
        if (groupType == "archivefilegroup")
          fdi.groupType = FieldModelGroupInfo::ARCHIVE_GROUP;
        else {
          if (groupType != "filegroup")
            METLIBS_LOG_WARN("group type '" << groupType << "' unknown, using standard group");
          fdi.groupType = FieldModelGroupInfo::STANDARD_GROUP;
        }
        if (top) {
          fieldModelGroups.insert(fieldModelGroups.begin(), fdi);
          groupIndex = 0;
        } else {
          fieldModelGroups.push_back(fdi);
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
            fieldModelGroups[groupIndex].models.push_back(FieldModelInfo(mn, lines[l]));
          }
        } else {
          std::ostringstream ost;
          ost << FIELD_FILES << "|" << l << "|Bad or no GridIO with type= " << gridioType << "  for model='" << mn << "'";
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
    std::string sourcetype;
    std::string model;
    std::string guiOption;
    std::string config;
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
      if (key == "sourcetype") {
        sourcetype = stokens[1];
      } else if (key == "model") {
        model = stokens[1];
      } else if (key == "o") {
        guiOption = stokens[1];
      } else if (key == "config" || key == "c") {
        config = stokens[1];
      } else if (key == "file") {
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

    std::ostringstream ost;
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

std::map<std::string,std::string> FieldManager::getGlobalAttributes(const std::string& modelName, const std::string& refTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName)<<LOGVAL(refTime));

  if (GridCollectionPtr pgc = getGridCollection(modelName, refTime))
    return pgc->getGlobalAttributes(refTime);
  else
    return std::map<std::string,std::string>();
}

const std::map<std::string, FieldPlotInfo>& FieldManager::getFieldPlotInfo(const std::string& modelName, const std::string& refTime)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName)<<LOGVAL(refTime));
  if (GridCollectionPtr pgc = getGridCollection(modelName, refTime))
    return pgc->getFieldPlotInfo(refTime);

  static const std::map<std::string, FieldPlotInfo> EMPTY;
  return EMPTY;
}

bool FieldManager::updateGridCollection(const std::string& modelName)
{
  METLIBS_LOG_TIME(LOGVAL(modelName));

  GridSources_t::iterator p = gridSources.find(modelName);
  if (p == gridSources.end()) {
    METLIBS_LOG_WARN("Undefined model '" << modelName << "'");
    return false;
  }

  GridCollectionPtr pgc = p->second;
  if (pgc->updateSources())
    pgc->makeInventory("");
  return true;
}

FieldManager::GridCollectionPtr FieldManager::getGridCollection(const std::string& modelName, const std::string& refTime)
{
  METLIBS_LOG_TIME(LOGVAL(modelName) << LOGVAL(refTime));

  GridSources_t::iterator p = gridSources.find(modelName);
  if (p == gridSources.end()) {
    METLIBS_LOG_WARN("Undefined model '" << modelName << "'");
    return GridCollectionPtr();
  }

  GridCollectionPtr pgc = p->second;
  if (!pgc->inventoryOk(refTime))
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
  GridCollectionPtr pgc = getGridCollection(modelName, reftime);

  if (pgc) {
    grid = pgc->getGrids();
  }

  return grid;
}

plottimes_t FieldManager::getFieldTime(const FieldRequest& frq)
{
  METLIBS_LOG_SCOPE(LOGVAL(frq.modelName) << LOGVAL(frq.refTime) << LOGVAL(frq.paramName) << LOGVAL(frq.zaxis) << LOGVAL(frq.plevel));
  plottimes_t times;

  if (GridCollectionPtr pgc = getGridCollection(frq.modelName, frq.refTime)) {
    // if fieldrequest.paramName is a standard_name, find variable_name (times will be empty if translation to variable name fails)
    std::string paramName = frq.paramName;
    if (!frq.standard_name || pgc->standardname2variablename(frq.refTime, frq.paramName, paramName))
      times = pgc->getTimes(frq.refTime, paramName);

    if (frq.hourOffset != 0 || frq.minOffset != 0) {
      plottimes_t times_with_offset;
      for (miTime t : times) {
        t.addHour(-frq.hourOffset);
        t.addMin(-frq.minOffset);
        times_with_offset.insert(t);
      }
      times = std::move(times_with_offset);
    }
  }

  METLIBS_LOG_DEBUG(LOGVAL(times.size()));
  return times;
}

plottimes_t FieldManager::getFieldTime(const std::vector<FieldRequest>& fieldrequests)
{
  METLIBS_LOG_SCOPE();

  plottimes_t all_times;
  bool allTimeSteps = true;

  for (const FieldRequest& frq : fieldrequests) {
    allTimeSteps &= frq.allTimeSteps;

    const plottimes_t frq_times = getFieldTime(frq);
    if (allTimeSteps) {
      diutil::insert_all(all_times, frq_times);
    } else if (all_times.empty()) {
      all_times = frq_times;
    } else if (!frq_times.empty()) {
      plottimes_t twork;
      set_intersection(all_times.begin(), all_times.end(), frq_times.begin(), frq_times.end(), std::insert_iterator<plottimes_t>(twork, twork.begin()));
      all_times = std::move(twork);
    }
  }

  return all_times;
}

std::set<std::string> FieldManager::getReferenceTimes(const std::string& modelName)
{
  if (GridCollectionPtr pgc = getGridCollection(modelName, ""))
    return pgc->getReferenceTimes();
  else
    return std::set<std::string>();
}

std::string FieldManager::getBestReferenceTime(const std::string& modelName,
    int refOffset, int refHour)
{
  return ::getBestReferenceTime(getReferenceTimes(modelName), refOffset, refHour);
}

Field_p FieldManager::makeField(const FieldRequest& frq)
{
  METLIBS_LOG_TIME(LOGVAL(frq.modelName) << LOGVAL(frq.paramName) << LOGVAL(frq.zaxis) << LOGVAL(frq.refTime) << LOGVAL(frq.ptime) << LOGVAL(frq.plevel)
                                         << LOGVAL(frq.elevel) << LOGVAL(frq.unit));

  GridCollectionPtr pgc = getGridCollection(frq.modelName, frq.refTime);
  if (!pgc) {
    METLIBS_LOG_WARN("could not find grid collection for model=" << frq.modelName << " reftime=" << frq.refTime);
    return nullptr;
  }

  Field_p fout = pgc->getField(frq);
  if (!fout) {
    METLIBS_LOG_WARN("No field read: model: " << frq.modelName << ", parameter:" << frq.paramName << ", level:" << frq.plevel << ", time:" << frq.ptime
                                              << ", referenceTime:" << frq.refTime);
    return nullptr;
  }

  fout->leveltext = frq.plevel;
  fout->idnumtext = frq.elevel;
  fout->paramName = frq.paramName;
  fout->modelName = frq.modelName;

  if (!frq.palette.empty()) {
    fout->palette = pgc->getVariable(frq.refTime, frq.palette);
  }

  return fout;
}

// static
bool FieldManager::makeDifferenceFields(Field_pv& fv1, Field_pv& fv2)
{
  unsigned int dim = fv1.size();
  if (fv2.size() != dim)
    return false; // must have same dimensions

  bool res = true;

  //change projection
  const GridArea& area1 = fv1[0]->area;
  const GridArea& area2 = fv2[0]->area;
  if (area1 != area2) {
    for (unsigned int j = 0; res && j < dim; ++j) {
      res = fv2[j]->changeGrid(gc, area1, false);
    }
    if (res && dim == 2) {
      Points_p p = gc.getGridPoints(area1, area2, false);
      int npos = fv2[0]->area.gridSize();
      res = gc.getVectors(area2, area1.P(), npos, p->x, p->y, fv2[0]->data, fv2[1]->data);
    }
  }

  //Subtract fields
  for (unsigned int j = 0; res && j < dim; ++j) {
    res = fv1[j]->subtract(*(fv2[j]));
  }

  if (res) {
    miutil::maximize(fv1[0]->validFieldTime, fv2[0]->validFieldTime); // or maybe not ???
    miutil::maximize(fv1[0]->analysisTime, fv2[0]->analysisTime);     // or maybe not ???
  }

  return res;
}

bool FieldManager::writeField(const FieldRequest& fieldrequest, Field_cp field)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldrequest.modelName) << LOGVAL(fieldrequest.paramName)
      << LOGVAL(fieldrequest.zaxis) << LOGVAL(fieldrequest.refTime)
      << LOGVAL(fieldrequest.ptime) << LOGVAL(fieldrequest.plevel)
      << LOGVAL(fieldrequest.elevel) << LOGVAL(fieldrequest.unit));

  GridCollectionPtr pgc = getGridCollection(fieldrequest.modelName, fieldrequest.refTime);

  if (not pgc) {
    METLIBS_LOG_WARN(LOGVAL(fieldrequest.modelName) << LOGVAL(fieldrequest.refTime) << "' not found");
    return false;
  }

  return pgc->putData(fieldrequest.refTime,
      fieldrequest.paramName, fieldrequest.plevel, fieldrequest.ptime,
      fieldrequest.elevel, fieldrequest.unit, fieldrequest.output_time,
      field);
}
