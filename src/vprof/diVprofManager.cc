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

#include "diVprofManager.h"

#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "diFieldUtil.h"
#include "diVprofBoxFactory.h"
#include "diVprofBoxLine.h"
#include "diVprofData.h"
#include "diVprofDiagram.h"
#include "diVprofModelSettings.h"
#include "diVprofOptions.h"
#include "diVprofPlotCommand.h"
#include "diVprofUtils.h"
#include "miSetupParser.h"
#include "util/misc_util.h"
#include "util/time_util.h"
#include "vcross_v2/VcrossSetup.h"

#include <mi_fieldcalc/math_util.h>

#include <puCtools/stat.h>
#include <puTools/TimeFilter.h>
#include <puTools/miStringFunctions.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

#define MILOGGER_CATEGORY "diana.VprofManager"
#include <miLogger/miLogging.h>

using miutil::miTime;

namespace /* anonymous */ {
miutil::miTime nowTime()
{
  miutil::miTime plottime = miTime::nowTime();
  miutil::miClock cl = plottime.clock();

  cl.setClock(cl.hour(), 0, 0);
  return miutil::miTime(plottime.date(), cl);
}

bool isFimexType(const std::string& filetype)
{
  return (filetype == "felt" || filetype == "netcdf" || filetype == "ncml" || filetype == "grbml");
}
} // anonymous namespace

VprofManager::VprofManager()
    : vpdiag(0)
    , realizationCount(1)
    , realization(0)
    , mCanvas(0)
{
  METLIBS_LOG_SCOPE();

  vpopt = new VprofOptions(); // defaults are set

  plotTime = nowTime();
}

VprofManager::~VprofManager()
{
  METLIBS_LOG_SCOPE();

  delete vpdiag;
  delete vpopt;

  // clean up vpdata...
  cleanup();
}

void VprofManager::cleanup()
{
  vpdata.clear();
  // TODO flush the field cache
}

void VprofManager::init()
{
  parseSetup();
}

void VprofManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  stationsfilenames.clear();
  filetypes.clear();
  reader_bufr = std::make_shared<VprofReaderBufr>();
  reader_fimex = std::make_shared<VprofReaderFimex>();
  reader_roadobs = std::make_shared<VprofReaderRoadobs>();
  dialogModelNames.clear();
  dialogFileNames.clear();

  const std::string section2 = "VERTICAL_PROFILE_FILES";
  std::vector<std::string> lines;

  if (!miutil::SetupParser::getSection(section2, lines)) {
    METLIBS_LOG_WARN("Missing section " << section2 << " in setupfile.");
    return;
  }

  std::vector<std::string> fimex_sources;
  for (const auto& line : lines) {
    METLIBS_LOG_DEBUG(LOGVAL(line));
    std::vector<std::string> tokens = miutil::split(line);
    std::string filetype = "standard", model, filename, stationsfilename, db_parameterfile, db_connectfile;

    // obsolete
    if (tokens.size() == 1) {
      const std::vector<std::string> tokens1 = miutil::split(tokens[0], "=");
      if (tokens1.size() != 2)
        continue;
      std::ostringstream ost;
      ost << "m=" << tokens1[0] << " f=" << tokens1[1] << " t=bufr";
      if (tokens1[0] == "bufr.amdar") {
        ost << " s=" << LocalSetupParser::basicValue("amdarstationlist");
      }
      tokens = miutil::split(ost.str());
    }
    for (size_t j = 0; j < tokens.size(); ++j) {
      METLIBS_LOG_DEBUG(LOGVAL(tokens[j]));
      const std::vector<std::string> tokens1 = miutil::split(tokens[j], "=");
      if (tokens1.size() != 2)
        continue;
      const std::string tokens1_0_lc = miutil::to_lower(tokens1[0]);
      if (tokens1_0_lc == "m") {
        model = tokens1[1];
      } else if (tokens1_0_lc == "f") {
        filename = tokens1[1];
      } else if (tokens1_0_lc == "s") {
        stationsfilename = tokens1[1];
      } else if (tokens1_0_lc == "t") {
        filetype = tokens1[1];
      } else if (tokens1_0_lc == "p") {
        db_parameterfile = tokens1[1];
      } else if (tokens1_0_lc == "d") {
        db_connectfile = tokens1[1];
      }
    }
    if (isFimexType(filetype)) {
      METLIBS_LOG_DEBUG(LOGVAL(line));
      fimex_sources.push_back(line);
    }

    stationsfilenames[model] = stationsfilename;
    filetypes[model] = filetype;

    reader_bufr->filenames[model] = filename;

    reader_roadobs->db_parameters[model] = db_parameterfile;
    reader_roadobs->db_connects[model] = db_connectfile;

    dialogModelNames.push_back(model);
    dialogFileNames.push_back(filename);
  }

  reader_fimex->setup->configureSources(fimex_sources);

  std::vector<std::string> computations;
  miutil::SetupParser::getSection("VERTICAL_PROFILE_COMPUTATIONS", computations);
  reader_fimex->setup->configureComputations(computations);
}

void VprofManager::setPlotWindow(const QSize& size)
{
  METLIBS_LOG_SCOPE();

  plotsize = size;
  if (vpdiag)
    vpdiag->setPlotWindow(plotsize);
}

void VprofManager::applyPlotCommands(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();

  // old style
  miutil::KeyValue_v vprof_options;
  VprofPlotCommand_cp cmd_station;

  // new style
  VprofPlotCommand_cp cmd_diagram;
  VprofPlotCommand_cpv cmd_boxes, cmd_graphs, cmd_data;

  for (PlotCommand_cp c : vstr) {
    METLIBS_LOG_DEBUG(LOGVAL(c->toString()));
    if (VprofPlotCommand_cp pc = std::dynamic_pointer_cast<const VprofPlotCommand>(c)) {
      if (pc->type() == VprofPlotCommand::OPTIONS) {
        diutil::insert_all(vprof_options, pc->all());
      } else if (pc->type() == VprofPlotCommand::STATION) {
        if (!cmd_station)
          cmd_station = pc;
        else
          METLIBS_LOG_WARN("ignoring multiple vprof STATION commands");
      } else if (pc->type() == VprofPlotCommand::MODELS) {
        for (const std::string& m : pc->items()) {
          METLIBS_LOG_DEBUG(LOGVAL(m));
          const VprofSelectedModel sm = VprofSelectedModel::fromSpaceText(m);
          if (sm.model.empty())
            continue;
          VprofPlotCommand_p dc = std::make_shared<VprofPlotCommand>(VprofPlotCommand::DATA);
          dc->add("name", sm.model);
          if (!sm.reftime.empty())
            dc->add("reftime", sm.reftime);
          cmd_data.push_back(dc);
        }
      } else if (pc->type() == VprofPlotCommand::DIAGRAM) {
        if (!cmd_diagram)
          cmd_diagram = pc;
        else
          METLIBS_LOG_WARN("ignoring multiple VPROF_DIAGRAM commands");
      } else if (pc->type() == VprofPlotCommand::BOX) {
        cmd_boxes.push_back(pc);
      } else if (pc->type() == VprofPlotCommand::GRAPH) {
        cmd_graphs.push_back(pc);
      } else if (pc->type() == VprofPlotCommand::DATA) {
        cmd_data.push_back(pc);
      }
    }
  }

  const bool opt_style = (!vprof_options.empty());
  const bool box_style = (cmd_diagram && !cmd_boxes.empty());
  METLIBS_LOG_DEBUG(LOGVAL(opt_style) << LOGVAL(box_style));
  if (opt_style && box_style) {
    METLIBS_LOG_ERROR("vprof command style mixture");
  }
  if (opt_style) {
    cmd_diagram = VprofPlotCommand_cp();
    cmd_boxes.clear();
    cmd_graphs.clear();
    vpopt->readOptions(vprof_options);
    vpopt->checkValues();
    const VprofPlotCommand_cpv cmds = vprof::createVprofCommandsFromOptions(vpopt);
    miutil::KeyValue_v diagram_options;
    for (VprofPlotCommand_cp cmd : cmds) {
      if (cmd->type() == VprofPlotCommand::DIAGRAM) {
        cmd_diagram = cmd;
      } else if (cmd->type() == VprofPlotCommand::BOX) {
        cmd_boxes.push_back(cmd);
      } else if (cmd->type() == VprofPlotCommand::GRAPH) {
        cmd_graphs.push_back(cmd);
      }
    }
  }

  if (!vpdiag) {
    vpdiag = new VprofDiagram();
    vpdiag->setPlotWindow(plotsize);
    vpdiag->changeNumber(vpdata.size());
  }

  miutil::KeyValue_v diagram_options;
  if (cmd_diagram)
    diagram_options = cmd_diagram->all();
  METLIBS_LOG_DEBUG(LOGVAL(diagram_options));
  vpdiag->configureDiagram(diagram_options);

  for (VprofPlotCommand_cp cmd_b : cmd_boxes) {
    METLIBS_LOG_DEBUG(LOGVAL(cmd_b->toString()));
    vpdiag->addBox(vprof::createBox(cmd_b->all()));
  }

  for (VprofPlotCommand_cp cmd_g : cmd_graphs) {
    METLIBS_LOG_DEBUG(LOGVAL(cmd_g->toString()));
    const miutil::KeyValue_v& config = cmd_g->all();
    size_t i_box = miutil::rfind(config, VprofBoxLine::key_graph_box);
    if (i_box == size_t(-1))
      continue;
    VprofBox_p box = vpdiag->box(config.at(i_box).value());
    if (!box)
      continue;
    box->addGraph(config);
  }

  if (!cmd_data.empty()) {
    VprofSelectedModel_v models;

    for (VprofPlotCommand_cp dc : cmd_data) {
      VprofSelectedModel sm;
      int refhour=-1, refoffset=0;
      for (const miutil::KeyValue& kv : dc->all()) {
        const std::string& key = kv.key();
        if (key == "model" || key == "obs" || key=="name") {
          sm.model = kv.value();
        } else if (key == "reftime") {
          sm.reftime = kv.value();
        } else if (key == "refhour") {
          refhour = kv.toInt();
        } else if (key == "refoffset") {
          refoffset = kv.toInt();
        }
      }
      METLIBS_LOG_DEBUG(LOGVAL(sm.model) << LOGVAL(sm.reftime) << LOGVAL(refhour) << LOGVAL(refoffset));
      if (sm.model.empty())
        continue;
      if (sm.reftime.empty()) {
        const std::set<std::string> reftimes = getReferencetimes(sm.model);
        if (!reftimes.empty()) {
          if (refhour == -1) {
            sm.reftime = *reftimes.rbegin();
          } else {
            sm.reftime = ::getBestReferenceTime(reftimes, refoffset, refhour);
          }
        }
      }
      models.push_back(sm);
    }
    setSelectedModels(models);
  }
  if (cmd_station)
    setStations(cmd_station->items());
  if (!cmd_data.empty() || cmd_station)
    setModel();
}

//*********************** end routines from controller ***********************

void VprofManager::setModel()
{
  METLIBS_LOG_SCOPE(LOGVAL(selectedModels.size()));

  // should not clear all data, possibly needed again...
  cleanup();

  // models from model dialog
  for (VprofSelectedModel& sm : selectedModels) {
    initVprofData(sm);
  }

  initTimes();
  initStations();

  if (vpdiag) {
    vpdiag->changeNumber(vpdata.size());
  }
}

void VprofManager::setRealization(int r)
{
  realization = std::max(std::min(r, realizationCount - 1), 0);
}

void VprofManager::setStation(const std::string& station)
{
  plotStations.clear();
  plotStations.push_back(station);
  updateSelectedStations();
}

void VprofManager::setStations(const std::vector<std::string>& stations)
{
  METLIBS_LOG_SCOPE(LOGVAL(stations.size()));
  if (!stations.empty()) {
    plotStations = stations;
    updateSelectedStations();
  }
}

void VprofManager::setTime(const miTime& time)
{
  plotTime = time;
  initStations();
}

std::string VprofManager::stepStation(int step)
{
  METLIBS_LOG_SCOPE(LOGVAL(step) << LOGVAL(stationList.size()));

  if (stationList.empty() || plotStations.empty())
    return "";

  std::vector<stationInfo>::const_iterator it = std::find_if(stationList.begin(), stationList.end(), diutil::eq_StationName(plotStations.front()));
  int i = 0;
  if (it != stationList.end()) {
    i = ((it - stationList.begin()) + step) % stationList.size();
    if (i < 0)
      i += stationList.size();
  }

  plotStations.clear();
  plotStations.push_back(stationList[i].name);
  updateSelectedStations();
  return plotStations.front();
}

miTime VprofManager::setTime(int step, int dir)
{
  METLIBS_LOG_DEBUG(LOGVAL(step) << LOGVAL(dir));

  if (timeList.empty())
    return miTime::nowTime();

  plottimes_t::const_iterator it;
  if (step == 0) {
    it = miutil::step_time(timeList, plotTime, dir);
  } else {
    it = miutil::step_time(timeList, plotTime, miutil::addHour(plotTime, step * dir));
  }
  if (it == timeList.end())
    it = --timeList.end();

  plotTime = *it;

  initStations();

  return plotTime;
}

void VprofManager::setCanvas(DiCanvas* c)
{
#if 0
  delete vpdiag;
  vpdiag = 0;
#endif

  mCanvas = c;
}

void VprofManager::updateSelectedStations()
{
  selectedStations.clear();

  VprofValuesRequest request = prepareValuesRequest();
  for (VprofData_p vpd : vpdata) {
    for (const std::string& s : plotStations) {
      METLIBS_LOG_DEBUG(LOGVAL(s));
      request.name = s;
      if (!vpd->getValues(request).empty()) {
        selectedStations.push_back(s);
        break;
      }
    }
  }

  for (const stationInfo& s : stationList) {
    std::vector<std::string>::const_iterator it = std::find(plotStations.begin(), plotStations.end(), s.name);
    if (it != plotStations.end()) {
      selectedStations.push_back(*it);
      break;
    }
  }
}

VprofValuesRequest VprofManager::prepareValuesRequest()
{
  METLIBS_LOG_SCOPE();
  VprofValuesRequest request;
  request.time = plotTime;
  request.realization = realization;
  if (vpdiag) {
    request.variables = vpdiag->variables();
    request.vertical_axis = vpdiag->verticalType();
  }
  METLIBS_LOG_DEBUG(LOGVAL(request.time) << LOGVAL(request.vertical_axis));
  return request;
}

bool VprofManager::plot(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();
  if (!vpdiag) {
    METLIBS_LOG_WARN("no diagram, cannot plot");
    return false;
  }

  std::unique_ptr<VprofPainter> painter(new VprofPainter(gl));
  vpdiag->plot(painter.get());

  bool dataok = false; // true when data are found

  if (!plotStations.empty()) {
    const int nstyles = std::min(vpopt->dataColour.size(), std::min(vpopt->dataLinewidth.size(), vpopt->windLinewidth.size()));

    VprofValuesRequest request = prepareValuesRequest();
    for (size_t i = 0; i < vpdata.size(); i++) {
      for (const std::string& s : plotStations) {
        request.name = s;
        const VprofValues_cpv vvv = vpdata[i]->getValues(request);
        for (int j = 0; j < (int)vvv.size(); ++j) {
          const int istyle = i % nstyles;
          VprofModelSettings ms;
          ms.nplot = i;
          ms.isSelectedRealization = (vvv.size() == 1) || (j == realization);
          ms.setColour(Colour(vpopt->dataColour[istyle]));
          ms.dataLinewidth = vpopt->dataLinewidth[istyle];
          ms.windLinewidth = vpopt->windLinewidth[istyle];

          vpdiag->plotValues(painter.get(), vvv[j], ms);
        }
        if (!vvv.empty())
          break;
      }
    }

    vpdiag->plotText(painter.get());
  }

  return dataok;
}

/***************************************************************************/

const std::vector<std::string>& VprofManager::getModelNames()
{
  METLIBS_LOG_SCOPE();
  init();
  return dialogModelNames;
}

/***************************************************************************/

std::set<std::string> VprofManager::getReferencetimes(const std::string& modelName)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName));
  std::set<std::string> rf;
  if (VprofReader_p reader = getReader(modelName))
    rf = reader->getReferencetimes(modelName);
  return rf;
}

/***************************************************************************/

void VprofManager::setSelectedModels(const VprofSelectedModel_v& models)
{
  METLIBS_LOG_SCOPE();
  // called when model selected in model dialog or bdiana
  selectedModels = models;
}

/***************************************************************************/

VprofReader_p VprofManager::getReader(const std::string& modelName)
{
  const std::string model_part = modelName.substr(0, modelName.find("@"));
  std::map<std::string, std::string>::const_iterator it = filetypes.find(model_part);
  if (it != filetypes.end() && it->second == "bufr") {
    return reader_bufr;
  } else if (it != filetypes.end() && it->second == "roadobs") {
    return reader_roadobs;
  } else {
    return reader_fimex;
  }
}

bool VprofManager::initVprofData(const VprofSelectedModel& selectedModel)
{
  METLIBS_LOG_SCOPE();
  if (VprofReader_p reader = getReader(selectedModel.model)) {
    if (VprofData_p vpd = reader->find(selectedModel, stationsfilenames[selectedModel.model])) {
      METLIBS_LOG_INFO("VPROFDATA READ OK for model '" << selectedModel.model << "'");
      vpdata.push_back(vpd);
      return true;
    }
  }

  METLIBS_LOG_ERROR("VPROFDATA READ ERROR for model '" << selectedModel.model << "'");
  return false;
}

/***************************************************************************/

void VprofManager::initStations()
{
  METLIBS_LOG_SCOPE();

  // Clear the stationlist to avoid duplicate stations or stations from previous selections with no data
  stationList.clear();

  // merge lists from all models
  for (VprofData_p vpd : vpdata) {
    if (vpd->updateStationList(plotTime)) {
      const std::vector<stationInfo>& stations = vpd->getStations();
      if (!stations.empty()) {
        // check for duplicates
        // name should be used as to check
        // all lists must be equal in size
        std::set<std::string> names;
        for (const stationInfo& si : stations) {
          if (names.count(si.name))
            continue;

          names.insert(si.name);

          stationList.push_back(si);

          if (miutil::trimmed(si.name).empty())
            METLIBS_LOG_DEBUG("empty vpdata name @ " << LOGVAL(si.lat) << LOGVAL(si.lon));
        }
      }
    }
  }

  // remember station
  if (!plotStations.empty())
    lastStation = plotStations[0];

  METLIBS_LOG_DEBUG("lastStation = '" << lastStation << "'");

  // if it's the first time, plot the first station
  if (plotStations.empty() && !stationList.empty())
    plotStations.push_back(stationList[0].name);
}

void VprofManager::initTimes()
{
  METLIBS_LOG_SCOPE(plotTime.isoTime());
  realizationCount = 1;

  timeList.clear();
  for (VprofData_p vp : vpdata) {
    miutil::maximize(realizationCount, vp->getRealizationCount());
    diutil::insert_all(timeList, vp->getTimes());
  }

  plottimes_t::const_iterator it = timeList.find(plotTime);
  if (it == timeList.end() && !timeList.empty()) {
    plotTime = nowTime();
    it = timeList.find(plotTime);
    if (it == timeList.end() && !timeList.empty()) {
      plotTime = *timeList.rbegin();
    }
  }

  setRealization(realization); // clamp to max
  METLIBS_LOG_DEBUG(plotTime.isoTime());
}

/***************************************************************************/

void VprofManager::mainWindowTimeChanged(const miTime& mainWindowTime)
{
  METLIBS_LOG_SCOPE(mainWindowTime);

  plottimes_t::const_iterator best = miutil::nearest(timeList, mainWindowTime);
  if (best != timeList.end())
    setTime(*best);
}

std::string VprofManager::getAnnotationString()
{
  std::ostringstream ost;
  ost << "Vertical profiles ";
  for (std::vector<VprofSelectedModel>::iterator p = selectedModels.begin(); p != selectedModels.end(); p++)
    ost << p->model << ' ';
  return ost.str();
}

std::vector<std::string> VprofManager::writeLog()
{
  return vpopt->writeOptions();
}

void VprofManager::readLog(const std::vector<std::string>& vstr, const std::string& /*thisVersion*/, const std::string& /*logVersion*/)
{
  miutil::KeyValue_v options;
  for (const std::string& line : vstr)
    diutil::insert_all(options, miutil::splitKeyValue(line));
  vpopt->readOptions(options);
}
