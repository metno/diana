/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2017 met.no

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

#include "diVprofManager.h"

#include "diVprofOptions.h"
#include "diVprofPlotCommand.h"
#include "diVprofData.h"
#include "diVprofDiagram.h"
#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/misc_util.h"
#include "vcross_v2/VcrossSetup.h"

#include "diField/VcrossUtil.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>
#include <puTools/TimeFilter.h>

#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#define MILOGGER_CATEGORY "diana.VprofManager"
#include <miLogger/miLogging.h>

using namespace std;
using miutil::miTime;

namespace /* anonymous */ {
miutil::miTime nowTime()
{
  miutil::miTime plottime= miTime::nowTime();
  miutil::miClock cl=plottime.clock();

  cl.setClock(cl.hour(),0,0);
  return  miutil::miTime(plottime.date(),cl);
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

  vpopt= new VprofOptions();  // defaults are set

  plotTime= nowTime();
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
  vector<std::string> vstr, sources;

  if (!miutil::SetupParser::getSection(section2,vstr)) {
    METLIBS_LOG_ERROR("Missing section " << section2 << " in setupfile.");
    return;
  }

  for (size_t i=0; i<vstr.size(); i++) {
    METLIBS_LOG_DEBUG(LOGVAL(vstr[i]));
    std::vector<std::string> tokens = miutil::split(vstr[i]);
    std::string filetype="standard", model, filename, stationsfilename, db_parameterfile, db_connectfile;

    //obsolete
    if ( tokens.size() == 1 ) {
      const std::vector<std::string> tokens1= miutil::split(tokens[0], "=");
      if (tokens1.size() != 2)
        continue;
      std::ostringstream ost;
      ost <<"m="<<tokens1[0]<<" f="<<tokens1[1]<<" t=bufr";
      if (tokens1[0] == "bufr.amdar") {
        ost<< " s=" << LocalSetupParser::basicValue("amdarstationlist");
      }
      tokens = miutil::split(ost.str());
    }
    for (size_t j=0; j<tokens.size(); ++j) {
      METLIBS_LOG_DEBUG(LOGVAL(tokens[j]));
      const std::vector<std::string> tokens1= miutil::split(tokens[j], "=");
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
      METLIBS_LOG_DEBUG(LOGVAL(vstr[i]));
      sources.push_back(vstr[i]);
    }

    stationsfilenames[model]= stationsfilename;
    filetypes[model] = filetype;

    reader_bufr->filenames[model] = filename;

    reader_roadobs->db_parameters[model] = db_parameterfile;
    reader_roadobs->db_connects[model] = db_connectfile;

    dialogModelNames.push_back(model);
    dialogFileNames.push_back(filename);
  }

  std::vector<std::string> computations;
  miutil::SetupParser::getSection("VERTICAL_PROFILE_COMPUTATIONS", computations);
  reader_fimex->setup = std::make_shared<vcross::Setup>();
  reader_fimex->setup->configureSources(sources);
  reader_fimex->setup->configureComputations(computations);
}

void VprofManager::setPlotWindow(const QSize& size)
{
  METLIBS_LOG_SCOPE();

  plotsize = size;
  if (vpdiag)
    vpdiag->setPlotWindow(plotsize);
}

void VprofManager::parseQuickMenuStrings(const PlotCommand_cpv& vstr)
{
  VprofPlotCommand_cp cmd_models, cmd_station;
  vector<miutil::KeyValue_v> vprof_options;
  for (PlotCommand_cp c : vstr) {
    if (VprofPlotCommand_cp pc = std::dynamic_pointer_cast<const VprofPlotCommand>(c)) {
      if (pc->type() == VprofPlotCommand::OPTIONS)
        vprof_options.push_back(pc->all());
      else if (pc->type() == VprofPlotCommand::STATION)
        cmd_station = pc;
      else if (pc->type() == VprofPlotCommand::MODELS)
        cmd_models = pc;
    }
  }
  getOptions()->readOptions(vprof_options);
  if (cmd_models)
    setSelectedModels(cmd_models->items());
  if (cmd_station)
    setStations(cmd_station->items());
  if (cmd_models || cmd_station)
    setModel();
}

//*********************** end routines from controller ***********************

void VprofManager::setModel()
{
  METLIBS_LOG_SCOPE(LOGVAL(selectedModels.size()));

  // should not clear all data, possibly needed again...
  cleanup();

  //models from model dialog
  for (VprofSelectedModel& sm : selectedModels) {
    initVprofData(sm);
  }

  initTimes();
  initStations();

  if (vpdiag) {
    int nmod= vpdata.size();
    vpdiag->changeNumber(nmod);
  }
}

void VprofManager::setRealization(int r)
{
  realization = std::max(std::min(r, realizationCount-1), 0);
}

void VprofManager::setStation(const std::string& station)
{
  plotStations.clear();
  plotStations.push_back(station);
  updateSelectedStations();
}

void VprofManager::setStations(const std::vector<std::string>& stations)
{
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

std::string VprofManager::setStation(int step)
{
  METLIBS_LOG_SCOPE(LOGVAL(step) << LOGVAL(stationList.size()));

  if (stationList.empty() || plotStations.empty())
    return "";

  std::vector<stationInfo>::const_iterator it
      = std::find_if(stationList.begin(), stationList.end(), diutil::eq_StationName(plotStations.front()));
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

  int n= timeList.size();
  int i = 0;
  if ( step == 0 ){
    while (i<n && timeList[i] != plotTime) {
      i++;
    }
    i += dir;


  } else {

    miTime newTime(plotTime);
    newTime.addHour(step * dir);

    if( dir > 0 ) {
      i = 0;
      while (i<n && timeList[i] < newTime) {
        i++;
      }
    } else {
      i = n-1;
      while (i>=0 && timeList[i] > newTime) {
        i--;
      }
    }
  }

  if (i==n) {
    i = n-1;
  }
  if (i<0) {
    i = 0;
  }

  plotTime= timeList[i];

  //if (onlyObs)
    initStations();

  return plotTime;
}

void VprofManager::setCanvas(DiCanvas* c)
{
  delete vpdiag;
  vpdiag = 0;

  mCanvas = c;
}

void VprofManager::updateSelectedStations()
{
  selectedStations.clear();

  for (VprofData_p vpd : vpdata) {
    for (const std::string& s : plotStations) {
      METLIBS_LOG_DEBUG(LOGVAL(s));
      if (!vpd->getValues(s, plotTime, realization).empty()) {
        selectedStations.push_back(s);
        break;
      }
    }
  }

  for (const stationInfo& s : stationList) {
    std::vector<std::string>::const_iterator it
        = std::find(plotStations.begin(), plotStations.end(), s.name);
    if (it != plotStations.end()) {
      selectedStations.push_back(*it);
      break;
    }
  }
}

bool VprofManager::plot(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(LOGVAL(realization));

  if (!vpdiag) {
    vpdiag= new VprofDiagram(vpopt, gl);
    vpdiag->setPlotWindow(plotsize);
    int nmod= vpdata.size();
    vpdiag->changeNumber(nmod);
  }

  vpdiag->plot();

  bool dataok = false; // true when data are found

  if (!plotStations.empty()) {
    for (size_t i=0; i<vpdata.size(); i++) {
      for (const std::string& s : plotStations) {
        const VprofValues_cpv vvv = vpdata[i]->getValues(s, plotTime, realization);
        for (int j = 0; j < (int)vvv.size(); ++j) {
          const bool selected = (vvv.size() == 1) || (j == realization);
          vpdiag->plotValues(i, *vvv[j], selected);
        }
        break;
      }
    }

    vpdiag->plotText();
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

std::vector <std::string> VprofManager::getReferencetimes(const string &modelName)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName));
  std::vector <std::string> rf;
  if (VprofReader_p reader = getReader(modelName))
    rf = reader->getReferencetimes(modelName);
  return rf;
}

/***************************************************************************/

void VprofManager::setSelectedModels(const vector <std::string>& models)
{
  METLIBS_LOG_SCOPE();
  //called when model selected in model dialog or bdiana

  selectedModels.clear();
  for ( size_t i=0; i<models.size(); ++i ) {
    VprofSelectedModel selectedModel;
    vector<std::string> vstr = miutil::split(models[i]," ");
    if ( vstr.size() > 0 ) {
      selectedModel.model = vstr[0];
    }
    if ( vstr.size() > 1 ) {
      selectedModel.reftime = vstr[1];
    }
    selectedModels.push_back(selectedModel);
  }
}

/***************************************************************************/

VprofReader_p VprofManager::getReader(const std::string& modelName)
{
  const std::string model_part = modelName.substr(0, modelName.find("@"));
  std::map<std::string,std::string>::const_iterator it = filetypes.find(model_part);
  if (it != filetypes.end() && it->second == "bufr") {
    return reader_bufr;
  } else if (it != filetypes.end() && it->second == "roadobs"){
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

  //merge lists from all models
  int nvpdata = vpdata.size();

  for (int i = 0; i<nvpdata; i++) {
    if ( vpdata[i]->updateStationList(plotTime) )  {
      const vector <stationInfo>& stations = vpdata[i]->getStations();
      if (!stations.empty()) {
        // check for duplicates
        // name should be used as to check
        // all lists must be equal in size
        std::set<std::string> names;
        for (vector <stationInfo>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
          if (names.count(it->name))
            continue;

          names.insert(it->name);

          stationList.push_back(*it);

          if (miutil::trimmed(it->name).empty())
            METLIBS_LOG_WARN("empty vpdata name @ " << LOGVAL(it->lat) << LOGVAL(it->lon));
        }
      }
    }
  }

  // remember station
  if (!plotStations.empty())
    lastStation = plotStations[0];

  METLIBS_LOG_DEBUG("lastStation = '"  << lastStation << "'");

  // if it's the first time, plot the first station
  if (plotStations.empty() && !stationList.empty())
    plotStations.push_back(stationList[0].name);
}


void VprofManager::initTimes()
{
  METLIBS_LOG_SCOPE(plotTime.isoTime());
  realizationCount = 1;

  std::set<miutil::miTime> set_times;
  for (VprofData_p vp : vpdata) {
    vcross::util::maximize(realizationCount, vp->getRealizationCount());
    diutil::insert_all(set_times, vp->getTimes());
  }

  timeList.clear();
  diutil::insert_all(timeList, set_times);

  std::vector<miutil::miTime>::const_iterator it = std::find(timeList.begin(), timeList.end(), plotTime);
  if (it == timeList.end() && !timeList.empty()) {
    plotTime=nowTime();
    std::vector<miutil::miTime>::const_iterator it = std::find(timeList.begin(), timeList.end(), plotTime);
    if (it == timeList.end() && !timeList.empty()) {
      plotTime = timeList.back();
    }
  }

  setRealization(realization); // clamp to max
  METLIBS_LOG_DEBUG(plotTime.isoTime());
}

/***************************************************************************/

void VprofManager::mainWindowTimeChanged(const miTime& mainWindowTime)
{
  METLIBS_LOG_SCOPE(mainWindowTime);

  int maxdiff = 0, itime = -1;
  const int n = timeList.size();
  for (int i=0; i<n; i++) {
    const int diff = abs(miTime::minDiff(timeList[i], mainWindowTime));
    if (itime < 0 || diff < maxdiff) {
      maxdiff = diff;
      itime = i;
    }
  }
  if (itime > -1)
    setTime(timeList[itime]);
}

std::string VprofManager::getAnnotationString()
{
  std::ostringstream ost;
  ost << "Vertical profiles ";
  for (vector <VprofSelectedModel>::iterator p=selectedModels.begin(); p!=selectedModels.end(); p++)
    ost << p->model << ' ';
  return ost.str();
}

vector<string> VprofManager::writeLog()
{
  return vpopt->writeOptions();
}

void VprofManager::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion)
{
  std::vector<miutil::KeyValue_v> options;
  for (const std::string& line : vstr)
    options.push_back(miutil::splitKeyValue(line));
  vpopt->readOptions(options);
}
