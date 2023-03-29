/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2022 met.no

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

#include "diObsManager.h"

#include "diKVListPlotCommand.h"
#include "diObsDataUnion.h"
#include "diObsPlot.h"
#include "diObsReaderFactory.h"

#include "miSetupParser.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#include <algorithm>

#define MILOGGER_CATEGORY "diana.ObsManager"
#include <miLogger/miLogging.h>

using std::vector;
using miutil::miTime;
using miutil::SetupParser;

ObsManager::ObsManager()
{
  useArchive = false;
}

std::vector<ObsReader_p> ObsManager::readers(ObsPlot* oplot)
{
  std::vector<ObsReader_p> readers;
  readers.reserve(oplot->readerNames().size());
  for (const std::string& rn : oplot->readerNames()) {
    METLIBS_LOG_DEBUG("reader name: '" << rn << "'");
    string_ProdInfo_m::const_iterator itP = Prod.find(rn);
    if (itP == Prod.end()) {
      METLIBS_LOG_ERROR("no reader named '" << rn << "'");
    } else {
      readers.push_back(itP->second.reader);
    }
  }
  return readers;
}

void ObsManager::setPlotDefaults(ObsPlot* oplot)
{
  // Default for flags in misc is false
  for (const auto& spt : setupPlotTypes_) {
    if (spt.plottype == oplot->plottype()) {
      if (spt.misc & ObsDialogInfo::show_VV_as_code)
        oplot->setShowVVAsCode(false);
      break;
    }
  }
}

void ObsManager::prepare(ObsPlot* oplot, const miutil::miTime& time)
{
  METLIBS_LOG_TIME();

  bool success = true;
  miutil::miTime obsTime;
  oplot->clear();
  oplot->setPopupSpec(popupSpec);
  auto obsData = std::make_shared<ObsDataUnion>();
  for (ObsReader_p reader : readers(oplot)) {
    ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
    req->obstime = time;
    req->timeDiff = oplot->getTimeDiff();
    req->level = oplot->getLevel();
    req->useArchive = useArchive;
    ObsDataResult_p res = std::make_shared<ObsDataResult>();
    reader->getData(req, res);
    if (!res->success())
      success = false;
    obsData->add(res->data());

    const miutil::miTime& rtime = res->time();
    if (!rtime.undef()) {
      if (obsTime.undef() || std::abs(miTime::minDiff(time, res->time())) < std::abs(miTime::minDiff(time, obsTime)))
        obsTime = res->time();
    }
    oplot->setParameters(reader->getParameters());
    oplot->setObsExtraAnnotations(reader->getExtraAnnotations());
  }

  auto singleObsData = obsData->single();
  oplot->setObsData(singleObsData ? singleObsData : obsData);
  oplot->setObsTime(obsTime);
  oplot->setData(success);
}

bool ObsManager::updateTimes(ObsPlot* op)
{
  METLIBS_LOG_SCOPE();
  bool updated = false;
  for (ObsReader_p reader : readers(op)) {
    if (reader->checkForUpdates(useArchive))
      updated = true;
  }
  return updated;
}

plottimes_t ObsManager::getObsTimes(const std::set<std::string>& readernames)
{
  const std::vector<std::string> rn(readernames.begin(), readernames.end());
  return getTimes(rn, true);
}

void ObsManager::getCapabilitiesTime(plottimes_t& normalTimes, int& timediff, const PlotCommand_cp& pinfo)
{
  timediff = 0;
  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pinfo);
  if (!cmd || cmd->size() < 2)
    return;

  std::vector<std::string> obsTypes;
  for (const miutil::KeyValue& kv : cmd->all()) {
    if (kv.key() == "data") {
      obsTypes = miutil::split(kv.value(), ",");
    } else if (kv.key() == "timediff") {
      timediff = kv.toInt();
    }
  }

  normalTimes = getTimes(obsTypes, true);
}

plottimes_t ObsManager::getTimes(const std::vector<std::string>& readernames, bool update)
{
  plottimes_t timeset;

  for (const std::string& rn : readernames) {
    string_ProdInfo_m::iterator it = Prod.find(rn);
    if (it != Prod.end())
      diutil::insert_all(timeset, it->second.reader->getTimes(useArchive, update));
    else
      METLIBS_LOG_ERROR("no reader named '" << rn << "'");
  }

  return timeset;
}

static void addButtons(ObsDialogInfo::PlotType& pt, const std::vector<std::string>& parnames)
{
  for (const std::string& parname : parnames) {
    const ObsDialogInfo::Par p = ObsDialogInfo::findPar(parname);
    pt.addButton(p.name, p.button_tip, p.button_low, p.button_high);
  }
}

void ObsManager::addReaders(ObsDialogInfo::PlotType& dialogInfo)
{
  std::map<int, std::string> sorted_readernames; // we want the readers in the same order as in the setup file
  for (const auto& pr : Prod) {
    const ProdInfo& pi = pr.second;
    if (pi.plottypes.count(dialogInfo.plottype))
      sorted_readernames[pi.sort_order] = pr.first;
  }
  for (const auto& sr : sorted_readernames)
    dialogInfo.readernames.push_back(sr.second);
}

ObsDialogInfo ObsManager::initDialog()
{
  ObsDialogInfo dialog;
  dialog.priority = priority;

  for (ObsDialogInfo::PlotType pt : setupPlotTypes_) { // make copies!
    const std::map<std::string, ObsDialogInfo::CriteriaList_v>::const_iterator it = criteriaList.find(pt.name);
    if (it != criteriaList.end())
      pt.criteriaList = it->second;
    addReaders(pt);
    pt.addExtraParameterButtons();
    dialog.addPlotType(pt, pt.plottype == OPT_OTHER);
  }

  for (const auto& pr : Prod) {
    if (pr.second.plottypes.count(OPT_OTHER)) {

      ObsDialogInfo::PlotType pother;
      pother.plottype = OPT_OTHER;
      pother.name = pr.first;

      pother.misc =
          ObsDialogInfo::markerboxVisible | ObsDialogInfo::orientation | ObsDialogInfo::parameterName | ObsDialogInfo::popup | ObsDialogInfo::criteria;
      pother.criteriaList = criteriaList[pr.second.name];
      pother.addExtraParameterButtons();
      dialog.plottype.push_back(pother);
    }
  }

  return dialog;
}

void ObsManager::updateDialog(ObsDialogInfo::PlotType& pt, const std::string& readername)
{
  METLIBS_LOG_SCOPE(LOGVAL(readername) << LOGVAL(pt.plottype));

  if (pt.plottype != OPT_OTHER)
    return;

  pt.name = readername;
  pt.readernames.clear();
  pt.readernames.push_back(readername);
  pt.button.clear();

  string_ProdInfo_m::iterator itp = Prod.find(readername);
  if (itp == Prod.end())
    return;

  for (const ObsDialogInfo::Par& par : itp->second.reader->getParameters())
    pt.addButton(par.name, par.button_tip, par.button_low, par.button_high);
}

bool ObsManager::parseSetup()
{
  parseFilesSetup();
  parsePrioritySetup();
  parseCriteriaSetup();
  parsePopupWindowSetup();
  parsePlotTypeSetup();
  return true;
}

bool ObsManager::parseFilesSetup()
{
  METLIBS_LOG_SCOPE();
  Prod.clear();

  const std::string obs_name = "OBSERVATION_FILES";
  std::vector<std::string> sect_obs;
  if (!SetupParser::getSection(obs_name, sect_obs)) {
    METLIBS_LOG_WARN(obs_name << " section not found");
    return true;
  }

  // ********  Common to all plot types **********************

  int sort_order = 0; // addReaders needs to know the order from the setup file

  ProdInfo* pip = nullptr;
  for (unsigned int i = 0; i < sect_obs.size(); i++) {
    const std::vector<std::string> token = miutil::split_protected(sect_obs[i], '"', '"', "=", true);
    if (token.size() != 2) {
      SetupParser::errorMsg(obs_name, i, "Line must contain '='");
      continue;
    }

    const std::string key = miutil::to_lower(token[0]);
    if (key == "prod") {
      const std::vector<std::string> stoken = miutil::split(token[1], ":");
      if (stoken.size() < 2) {
        METLIBS_LOG_ERROR("Prod specification needs to be like plottype:readername");
        pip = nullptr;
        continue;
      }
      if (stoken.size() > 2) {
        METLIBS_LOG_WARN("Prod specification ignores extra parameters after readername");
      }

      const std::string& readername = stoken[1];
      if (readername.empty()) {
        METLIBS_LOG_ERROR("Prod specification without readername");
        pip = nullptr;
        continue;
      }
      const std::string& plottype = stoken[0];
      if (plottype.empty()) {
        METLIBS_LOG_ERROR("Prod specification without plottype");
        continue;
      }

      string_ProdInfo_m::iterator itp = Prod.find(readername);
      if (itp == Prod.end()) {
        itp = Prod.insert(std::make_pair(readername, ProdInfo())).first;
        itp->second.sort_order = sort_order;
        sort_order++;
      }
      pip = &itp->second;
      pip->name = plottype;
      pip->plottypes.insert(obsPlotTypeFromText(plottype));
    } else if (pip) {
      if (!pip->reader) {
        pip->reader = makeObsReader(key);
      }
      if (!pip->reader) {
        METLIBS_LOG_WARN("could not create reader for '" << key << "'");
      } else if (!pip->reader->configure(key, token[1])) {
        METLIBS_LOG_WARN("unrecognized config '" << sect_obs[i] << "'");
        SetupParser::errorMsg(obs_name, i, "unrecognized config");
      }
    } else {
      SetupParser::errorMsg(obs_name, i, "You must give prod before '" + key + "'");
    }
  }
  return true;
}

bool ObsManager::parsePrioritySetup()
{
  // *******  Priority List ********************

  const std::string key_name = "name";
  const std::string key_file = "file";

  std::vector<std::string> tokens, stokens;
  std::string name;
  ObsDialogInfo::PriorityList pri;

  priority.clear();

  const std::string pri_name = "OBSERVATION_PRIORITY_LISTS";
  std::vector<std::string> sect_pri;

  if (SetupParser::getSection(pri_name, sect_pri)) {

    for (unsigned int i = 0; i < sect_pri.size(); i++) {
      name = "";
      std::string file = "";

      tokens = miutil::split_protected(sect_pri[i], '"', '"', " ", true);
      for (unsigned int j = 0; j < tokens.size(); j++) {
        stokens = miutil::split(tokens[j], 0, "=");
        if (stokens.size() > 1) {
          std::string key = miutil::to_lower(stokens[0]);
          std::string value = stokens[1];
          miutil::remove(value, '"');

          if (key == key_name) {
            name = value;
          } else if (key == key_file) {
            file = value;
          }
        }
      }

      if (not name.empty() and not file.empty()) {
        pri.name = name;
        pri.file = file;
        priority.push_back(pri);
      } else {
        SetupParser::errorMsg(pri_name, i, "Incomplete observation priority specification");
        continue;
      }
    }
  }
  return true;
}

bool ObsManager::parseCriteriaSetup()
{
  criteriaList.clear();

  const std::string obs_crit_name = "OBSERVATION_CRITERIA";
  std::vector<std::string> sect_obs_crit;

  if (SetupParser::getSection(obs_crit_name, sect_obs_crit)) {
    ObsDialogInfo::CriteriaList critList;
    std::string plottype;
    for (const std::string& soc : sect_obs_crit) {
      std::vector<std::string> token = miutil::split(soc, "=");
      if (token.size() == 2 && miutil::to_lower(token[0]) == "plottype") {
        if (critList.criteria.size()) {
          criteriaList[plottype].push_back(critList);
          critList.criteria.clear();
        }
        plottype = miutil::to_lower(token[1]);
      } else if (token.size() == 2 && miutil::to_lower(token[0]) == "name") {
        if (critList.criteria.size()) {
          criteriaList[plottype].push_back(critList);
          critList.criteria.clear();
        }
        critList.name = token[1];
      } else {
        critList.criteria.push_back(soc);
      }
    }
    if (critList.criteria.size())
      criteriaList[plottype].push_back(critList);
  }
  return true;
}

bool ObsManager::parsePopupWindowSetup()
{
  popupSpec.clear();

  // Handling of popup window specification
  const std::string obs_popup_data = "OBSERVATION_POPUP_SPEC";
  std::vector<std::string> sect_popup_data;

  if (SetupParser::getSection(obs_popup_data, sect_popup_data)) {
    popupSpec = sect_popup_data;
  }

  return true;
}

bool ObsManager::parsePlotTypeSetup()
{
  METLIBS_LOG_SCOPE();
  setupPlotTypes_.clear();

  const std::string obs_plottype_data = "OBSERVATION_PLOTTYPES";
  std::vector<std::string> sect_plottype_data;
  if (!SetupParser::getSection(obs_plottype_data, sect_plottype_data))
    return false;

  for (const std::string& sptd : sect_plottype_data) {
    ObsDialogInfo::PlotType pt;
    std::vector<std::string> parameters;
    for (auto kv : miutil::splitKeyValue(sptd)) {
      if (kv.key() == "type") {
        pt.plottype = obsPlotTypeFromText(kv.value());
      } else if (kv.key() == "name") {
        pt.name = kv.value();
      } else if (kv.key() == "parameters") {
        parameters = miutil::split(kv.value(), ",");
      } else if (kv.key() == "vertical_levels") {
        pt.verticalLevels.clear();
        for (auto lt : miutil::split(kv.value(), ",")) {
          pt.verticalLevels.push_back(miutil::to_int(lt));
        }
      } else if (kv.key() == "misc") {
        pt.misc = 0;
        for (auto mt : miutil::split(kv.value(), ",")) {
          pt.misc |= ObsDialogInfo::miscFromText(mt);
        }
      }
    }
    if (pt.plottype == OPT_OTHER) {
      METLIBS_LOG_WARN("adding observation plottype 'other' is not supported");
      continue;
    }

    if (pt.name.empty())
      pt.name = obsPlotTypeToText(pt.plottype);
    addButtons(pt, parameters);
    setupPlotTypes_.push_back(pt);
  }

  return true;
}
