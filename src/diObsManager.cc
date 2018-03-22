/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2013 met.no

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

#include "diana_config.h"

#include "diObsManager.h"

#include "diKVListPlotCommand.h"
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

bool ObsManager::prepare(ObsPlot* oplot, const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();

  oplot->clear();
  oplot->setPopupSpec(popupSpec);
  for (ObsReader_p reader : readers(oplot)) {
    ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
    req->obstime = time;
    req->timeDiff = oplot->getTimeDiff();
    req->level = oplot->getLevel();
    req->useArchive = useArchive;
    ObsDataResult_p res = std::make_shared<ObsDataResult>();
    reader->getData(req, res);
    oplot->addObsData(res->data());

    if (!res->time().undef())
      oplot->setObsTime(res->time());
    oplot->setParameters(reader->getParameters());
    oplot->setObsExtraAnnotations(reader->getExtraAnnotations());
  }

  oplot->setData();
  return true;
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

vector<miTime> ObsManager::getObsTimes(const std::vector<miutil::KeyValue_v>& pinfos)
{
  vector<std::string> obsTypes;

  for (const miutil::KeyValue_v& pinfo : pinfos) {
    for (const miutil::KeyValue& kv : pinfo) {
      if (kv.key() == "data") {
        obsTypes = miutil::split(kv.value(), 0, ",");
        break;
      }
    }
  }

  return getTimes(obsTypes);
}

void ObsManager::getCapabilitiesTime(vector<miTime>& normalTimes, int& timediff, const PlotCommand_cp& pinfo)
{
  timediff = 0;
  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pinfo);
  if (!cmd || cmd->size() < 2)
    return;

  vector<std::string> obsTypes;
  for (const miutil::KeyValue& kv : cmd->all()) {
    if (kv.key() == "data") {
      obsTypes = miutil::split(kv.value(), ",");
    } else if (kv.key() == "timediff") {
      timediff = kv.toInt();
    }
  }

  normalTimes = getTimes(obsTypes);
}

vector<miTime> ObsManager::getTimes(const std::vector<std::string>& readernames)
{
  std::set<miTime> timeset;

  for (const std::string& rn : readernames) {
    string_ProdInfo_m::iterator it = Prod.find(rn);
    if (it != Prod.end())
      diutil::insert_all(timeset, it->second.reader->getTimes(useArchive));
    else
      METLIBS_LOG_ERROR("no reader named '" << rn << "'");
  }

  return vector<miTime>(timeset.begin(), timeset.end());
}

static void addButtons(ObsDialogInfo::PlotType& pt, const std::vector<std::string>& parnames)
{
  for (const std::string& parname : parnames) {
    const ObsDialogInfo::Par p = ObsDialogInfo::findPar(parname);
    pt.addButton(p.name, p.button_tip, p.button_low, p.button_high);
  }
}

static const std::vector<std::string> pp_synop = {"Wind",   "TTT",  "TdTdTd", "PPPP",  "ppp", "a",    "h",  "VV",       "N",        "RRR",    "ww",
                                                  "W1",     "W2",   "Nh",     "Cl",    "Cm",  "Ch",   "vs", "ds",       "TwTwTw",   "PwaHwa", "dw1dw1",
                                                  "Pw1Hw1", "TxTn", "sss",    "911ff", "s",   "fxfx", "Id", "St.no(3)", "St.no(5)", "Time"};
static const std::vector<std::string> pp_metar = {"Wind", "dndx", "fmfm", "TTT", "TdTdTd", "ww", "REww", "VVVV/Dv", "VxVxVxVx/Dvx", "Clouds", "PHPHPHPH", "Id"};
static const std::vector<std::string> pp_list = {"Pos", "dd",     "ff",     "TTT",    "TdTdTd", "PPPP", "ppp",   "a",      "h",      "VV",
                                                 "N",   "RRR",    "ww",     "W1",     "W2",     "Nh",   "Cl",    "Cm",     "Ch",     "vs",
                                                 "ds",  "TwTwTw", "PwaHwa", "dw1dw1", "Pw1Hw1", "TxTn", "sss",   "911ff",  "s",      "fxfx",
                                                 "Id",  "Date",   "Time",   "Height", "Zone",   "Name", "RRR_6", "RRR_12", "RRR_24", "quality"};
static const std::vector<std::string> pp_pressure = {"Pos", "dd", "ff", "TTT", "TdTdTd", "PPPP", "Id", "Date", "Time", "HHH", "QI", "QI_NM", "QI_RFF"};
static const std::vector<int> levels_pressure = {10, 30, 50, 70, 100, 150, 200, 250, 300, 400, 500, 700, 850, 925, 1000};

static const std::vector<std::string> pp_tide = {"Pos", "Date", "Time", "Name", "TE"};

static const std::vector<std::string> pp_ocean = {"Pos", "Id", "Date", "Time", "depth", "TTTT", "SSSS"};
static const std::vector<int> levels_ocean = {0,   10,  20,  30,  50,  75,  100,  125,  150,  200,  250,  300,
                                              400, 500, 600, 700, 800, 900, 1000, 1200, 1500, 2000, 3000, 4000};

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
  { //+++++++++Plot type = Synop+++++++++++++++
    ObsDialogInfo::PlotType psynop;
    psynop.plottype = OPT_SYNOP;
    psynop.name = obsPlotTypeToText(OPT_SYNOP);
    psynop.misc = ObsDialogInfo::dev_field_button | ObsDialogInfo::tempPrecision | ObsDialogInfo::unit_ms | ObsDialogInfo::criteria |
                  ObsDialogInfo::qualityflag | ObsDialogInfo::wmoflag;
    psynop.criteriaList = criteriaList["synop"];
    addButtons(psynop, pp_synop);
    addReaders(psynop);
    dialog.plottype.push_back(psynop);
  }

  { //+++++++++Plot type = Metar+++++++++++++++
    ObsDialogInfo::PlotType pmetar;
    pmetar.plottype = OPT_METAR;
    pmetar.name = obsPlotTypeToText(OPT_METAR);
    pmetar.misc = ObsDialogInfo::tempPrecision | ObsDialogInfo::criteria;
    pmetar.criteriaList = criteriaList["metar"];
    addButtons(pmetar, pp_metar);
    addReaders(pmetar);
    dialog.plottype.push_back(pmetar);
  }

  { //+++++++++Plot type = List+++++++++++++++
    ObsDialogInfo::PlotType plist;
    plist.plottype = OPT_LIST;
    plist.name = obsPlotTypeToText(OPT_LIST);
    plist.misc = ObsDialogInfo::dev_field_button | ObsDialogInfo::tempPrecision | ObsDialogInfo::unit_ms | ObsDialogInfo::markerboxVisible |
                 ObsDialogInfo::orientation | ObsDialogInfo::criteria | ObsDialogInfo::qualityflag | ObsDialogInfo::wmoflag | ObsDialogInfo::parameterName;
    plist.criteriaList = criteriaList["list"];
    addButtons(plist, pp_list);
    addReaders(plist);
    dialog.plottype.push_back(plist);
  }

  { //+++++++++Plot type = Pressure levels+++++++++++++++
    ObsDialogInfo::PlotType ppressure;
    ppressure.plottype = OPT_PRESSURE;
    ppressure.name = obsPlotTypeToText(OPT_PRESSURE);
    ppressure.misc =
        ObsDialogInfo::markerboxVisible | ObsDialogInfo::asFieldButton | ObsDialogInfo::orientation | ObsDialogInfo::parameterName | ObsDialogInfo::criteria;
    ppressure.criteriaList = criteriaList["pressure"];
    ppressure.verticalLevels = levels_pressure;
    addButtons(ppressure, pp_pressure);
    addReaders(ppressure);
    dialog.plottype.push_back(ppressure);
  }

  for (const auto& pr : Prod) {
    if (pr.second.plottypes.count(OPT_OTHER)) {

      ObsDialogInfo::PlotType pother;
      pother.plottype = OPT_OTHER;
      pother.name = pr.first;

      pother.misc =
          ObsDialogInfo::markerboxVisible | ObsDialogInfo::orientation | ObsDialogInfo::parameterName | ObsDialogInfo::popup | ObsDialogInfo::criteria;
      pother.criteriaList = criteriaList["ascii"];

      dialog.plottype.push_back(pother);
    }
  }

  { //+++++++++Plot type = Tide+++++++++++++++
    ObsDialogInfo::PlotType ptide;
    ptide.plottype = OPT_TIDE;
    ptide.name = obsPlotTypeToText(OPT_TIDE);
    ptide.misc = ObsDialogInfo::markerboxVisible | ObsDialogInfo::orientation | ObsDialogInfo::criteria;
    ptide.criteriaList = criteriaList["tide"];
    addButtons(ptide, pp_tide);
    addReaders(ptide);
    dialog.plottype.push_back(ptide);
  }

  { //+++++++++Plot type = Ocean levels+++++++++++++++
    ObsDialogInfo::PlotType pocean;
    pocean.plottype = OPT_OCEAN;
    pocean.name = obsPlotTypeToText(OPT_OCEAN);
    pocean.misc =
        ObsDialogInfo::markerboxVisible | ObsDialogInfo::asFieldButton | ObsDialogInfo::orientation | ObsDialogInfo::parameterName | ObsDialogInfo::criteria;
    pocean.criteriaList = criteriaList["ocean"];
    pocean.verticalLevels = levels_ocean;
    addButtons(pocean, pp_ocean);
    addReaders(pocean);
    dialog.plottype.push_back(pocean);
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
  dialog.plottype.clear();
  dialog.priority.clear();

  parseFilesSetup();
  parsePrioritySetup();
  parseCriteriaSetup();
  parsePopupWindowSetup();
  return true;
}

bool ObsManager::parseFilesSetup()
{
  METLIBS_LOG_SCOPE();
  const std::string obs_name = "OBSERVATION_FILES";
  vector<std::string> sect_obs;

  if (!SetupParser::getSection(obs_name, sect_obs)) {
    METLIBS_LOG_WARN(obs_name << " section not found");
    return true;
  }

  // ********  Common to all plot types **********************

  Prod.clear();
  int sort_order = 0; // addReaders needs to know the order from the setup file

  ProdInfo* pip = nullptr;
  for (unsigned int i = 0; i < sect_obs.size(); i++) {
    const vector<std::string> token = miutil::split_protected(sect_obs[i], '"', '"', "=", true);
    if (token.size() != 2) {
      SetupParser::errorMsg(obs_name, i, "Line must contain '='");
      continue;
    }

    const std::string key = miutil::to_lower(token[0]);
    if (key == "prod") {
      const vector<std::string> stoken = miutil::split(token[1], ":");
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

      pip->plottypes.insert(obsPlotTypeFromText(plottype));
    } else if (pip) {
      ObsReader_p& reader = pip->reader;
      if (!reader) {
        reader = makeObsReader(key);
      }
      if (!reader) {
        METLIBS_LOG_WARN("could not create reader for '" << key << "'");
      } else if (!reader->configure(key, token[1])) {
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

  vector<std::string> tokens, stokens;
  std::string name;
  ObsDialogInfo::PriorityList pri;

  dialog.priority.clear();

  const std::string pri_name = "OBSERVATION_PRIORITY_LISTS";
  vector<std::string> sect_pri;

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
        dialog.priority.push_back(pri);
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
  const std::string obs_crit_name = "OBSERVATION_CRITERIA";
  vector<std::string> sect_obs_crit;

  if (SetupParser::getSection(obs_crit_name, sect_obs_crit)) {
    ObsDialogInfo::CriteriaList critList;
    std::string plottype;
    for (const std::string& soc : sect_obs_crit) {
      vector<std::string> token = miutil::split(soc, "=");
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
  // Handling of popup window specification
  const std::string obs_popup_data = "OBSERVATION_POPUP_SPEC";
  vector<std::string> sect_popup_data;

  if (SetupParser::getSection(obs_popup_data, sect_popup_data)) {
    popupSpec = sect_popup_data;
  }

  return true;
}
