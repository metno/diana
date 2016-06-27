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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVprofManager.h"

#include "diVprofOptions.h"
#include "diVprofData.h"
#include "diVprofDiagram.h"
#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "vcross_v2/VcrossSetup.h"
#include "diField/VcrossUtil.h"
#include <puTools/TimeFilter.h>

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>
#include <puTools/mi_boost_compatibility.hh>
#include <boost/foreach.hpp>

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

miutil::miTime nowTime()
{
  miutil::miTime plottime= miTime::nowTime();
  miutil::miClock cl=plottime.clock();

  cl.setClock(cl.hour(),0,0);
  return  miutil::miTime(plottime.date(),cl);
}

VprofManager::VprofManager()
: vpdiag(0),
  plotw(0), ploth(0), mCanvas(0)
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
  diutil::delete_all_and_clear(vpdata);
  // TODO flush the field cache
}

void VprofManager::init()
{
  parseSetup();
}

void VprofManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  filenames.clear();
  stationsfilenames.clear();
  filetypes.clear();
  db_parameters.clear();
  db_connects.clear();
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
    std::string filetype="standard", fileformat, fileconfig, model, filename, stationsfilename, db_parameterfile, db_connectfile;
    miutil::TimeFilter tf;

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
    if (filetype == "netcdf" || filetype == "grbml"){
      METLIBS_LOG_DEBUG(LOGVAL(vstr[i]));
      sources.push_back(vstr[i]);
    }

    filenames[model]= filename;
    stationsfilenames[model]= stationsfilename;
    filetypes[model] = filetype;
    db_parameters[model] = db_parameterfile;
    db_connects[model] = db_connectfile;
    dialogModelNames.push_back(model);
    dialogFileNames.push_back(filename);
  }



  miutil::SetupParser::getSection("VERTICAL_PROFILE_COMPUTATIONS", computations);
  setup = miutil::make_shared<vcross::Setup>();
  setup->configureSources(sources);
  setup->configureComputations(computations);
}


void VprofManager::setPlotWindow(int w, int h)
{
  METLIBS_LOG_SCOPE(w << " " << h);

  plotw= w;
  ploth= h;
  if (vpdiag)
    vpdiag->setPlotWindow(plotw,ploth);
}

//*********************** end routines from controller ***********************

void VprofManager::setModel()
{
  METLIBS_LOG_SCOPE(LOGVAL(selectedModels.size()));

  // should not clear all data, possibly needed again...
  cleanup();

  //models from model dialog
  int m= selectedModels.size();
  for (int i=0;i<m;i++) {
    initVprofData(selectedModels[i]);
  }

  initTimes();
  initStations();

  if (vpdiag) {
    int nobs= 0;
    int nmod= vpdata.size();
    vpdiag->changeNumber(nobs, nmod);
  }
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
  METLIBS_LOG_SCOPE(LOGVAL(step) << LOGVAL(nameList.size()));

  if (nameList.empty() || plotStations.empty())
    return "";

  std::vector<std::string>::const_iterator it
      = std::find(nameList.begin(), nameList.end(), plotStations.front());
  int i = 0;
  if (it != nameList.end()) {
    i = ((it - nameList.begin()) + step) % nameList.size();
    if (i < 0)
      i += nameList.size();
  }

  plotStations.clear();
  plotStations.push_back(nameList[i]);
  updateSelectedStations();
  return plotStations.front();
}


miTime VprofManager::setTime(int step, int dir)
{
  METLIBS_LOG_DEBUG(LOGVAL(step) << LOGVAL(dir));

  if (timeList.size()==0)
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

  std::auto_ptr<VprofPlot> vp;
  for (size_t i=0; i<vpdata.size(); i++) {
    for (size_t j=0; j<plotStations.size(); ++j) {
      METLIBS_LOG_DEBUG(LOGVAL(plotStations[j]));
      vp.reset(vpdata[i]->getData(plotStations[j], plotTime));
      if (vp.get()) {
        selectedStations.push_back(plotStations[j]);
        break;
      }
    }
  }

  for (size_t i=0; i<nameList.size(); i++) {
    std::vector<std::string>::const_iterator it
        = std::find(plotStations.begin(), plotStations.end(), nameList[i]);
    if (it != plotStations.end()) {
      selectedStations.push_back(*it);
      break;
    }
  }
}

bool VprofManager::plot(DiGLPainter* gl)
{
 // METLIBS_LOG_SCOPE(LOGVAL(plotStations.size()) << LOGVAL(plotTime));

  if (!vpdiag) {
    vpdiag= new VprofDiagram(vpopt, gl);
    vpdiag->setPlotWindow(plotw,ploth);
    int nobs= 0;
    int nmod= vpdata.size();
    vpdiag->changeNumber(nobs,nmod);
  }

  vpdiag->plot();

  bool dataok = false; // true when data are found

  if (!plotStations.empty()) {

    for (size_t i=0; i<vpdata.size(); i++) {
      for (size_t j=0; j<plotStations.size(); ++j) {
        std::auto_ptr<VprofPlot> vp(vpdata[i]->getData(plotStations[j], plotTime));
        if (vp.get()) {
          dataok = vp->plot(gl, vpopt, i);
          break;
        }
      }
    }

    vpdiag->plotText();
  }

  return dataok;
}

/***************************************************************************/

vector <std::string> VprofManager::getModelNames()
{
  METLIBS_LOG_SCOPE();
  init();
  return dialogModelNames;
}

/***************************************************************************/

std::vector <std::string> VprofManager::getReferencetimes(const std::string modelName)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName));
  std::vector <std::string> rf;

  if ( filetypes[modelName] == "netcdf" || filetypes[modelName] == "grbml") {

    vcross::Collector_p collector = miutil::make_shared<vcross::Collector>(setup);

    collector->getResolver()->getSource(modelName)->update();
    const vcross::Time_s reftimes = collector->getResolver()->getSource(modelName)->getReferenceTimes();
    vector<miTime> rtv;
    rtv.reserve(reftimes.size());
    for (vcross::Time_s::const_iterator it=reftimes.begin(); it != reftimes.end(); ++it){
      rf.push_back(vcross::util::to_miTime(*it).isoTime("T"));
    }
  }

  return rf;

}


/***************************************************************************/

void VprofManager::setSelectedModels(const vector <std::string>& models)
{
  METLIBS_LOG_SCOPE();
  //called when model selected in model dialog or bdiana

  selectedModels.clear();
  for ( size_t i=0; i<models.size(); ++i ) {
    SelectedModel selectedModel;
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

bool VprofManager::initVprofData(const SelectedModel& selectedModel)
{
  METLIBS_LOG_SCOPE();
  std::string model_part=selectedModel.model.substr(0,selectedModel.model.find("@"));
  std::auto_ptr<VprofData> vpd(new VprofData(selectedModel.model, stationsfilenames[selectedModel.model]));
  bool ok = false;
  if (filetypes[model_part] == "bufr"){
    ok = vpd->readBufr(selectedModel.model, filenames[selectedModel.model]);
  } else if (filetypes[model_part] == "roadobs"){
    ok = vpd->readRoadObs(db_connects[selectedModel.model], db_parameters[selectedModel.model]);
  } else {
    ok = vpd->readFimex(setup,selectedModel.reftime);
  }
  if (ok) {
    METLIBS_LOG_INFO("VPROFDATA READ OK for model '" << selectedModel.model << "' filetype '"
        << filetypes[model_part] << "'");
    vpdata.push_back(vpd.release());
  } else {
    METLIBS_LOG_ERROR("VPROFDATA READ ERROR file '" << filenames[selectedModel.model] << "' model '"
        << selectedModel.model << "' filetype '" << filetypes[model_part] << "'");
  }
  return ok;
}

/***************************************************************************/

void VprofManager::initStations()
{
  METLIBS_LOG_SCOPE();

  //merge lists from all models
  int nvpdata = vpdata.size();

  nameList.clear();
  latitudeList.clear();
  longitudeList.clear();

  vector <std::string> namelist;
  vector <float>    latitudelist;
  vector <float>    longitudelist;

for (int i = 0; i<nvpdata; i++) {
  if ( vpdata[i]->updateStationList(plotTime) )  {
    namelist= vpdata[i]->getNames();
    latitudelist= vpdata[i]->getLatitudes();
    longitudelist= vpdata[i]->getLongitudes();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size()||n!=longitudelist.size()) {
      METLIBS_LOG_ERROR("SOMETHING WRONG WITH STATIONLIST!");
    } else if (n>0) {
      // check for duplicates
      // name should be used as to check
      // all lists must be equal in size
      std::set<std::string> names;
      for (size_t index = 0; index < namelist.size(); index++) {
        if (names.count(namelist[index]))
          continue;

        names.insert(namelist[index]);

        nameList.push_back(namelist[index]);
        latitudeList.push_back(latitudelist[index]);
        longitudeList.push_back(longitudelist[index]);

        if (miutil::trimmed(namelist[index]).empty())
          METLIBS_LOG_WARN("empty vpdata name @ " << LOGVAL(index) << LOGVAL(latitudelist[index]) << LOGVAL(longitudelist[index]));
      }
    }
  }
  }

  // remember station
  if (!plotStations.empty())
    lastStation = plotStations[0];

  METLIBS_LOG_DEBUG("lastStation = '"  << lastStation << "'");

  // if it's the first time, plot the first station
  if (plotStations.empty() && !nameList.empty())
    plotStations.push_back(nameList[0]);
}


void VprofManager::initTimes()
{
  METLIBS_LOG_SCOPE(plotTime.isoTime());
  std::set<miutil::miTime> set_times;
  for (size_t i=0; i<vpdata.size(); ++i) {
    const vector<miutil::miTime>& tmp_times = vpdata[i]->getTimes();
    set_times.insert(tmp_times.begin(), tmp_times.end());
  }

  timeList.clear();
  timeList.insert(timeList.end(), set_times.begin(), set_times.end());

  std::vector<miutil::miTime>::const_iterator it = std::find(timeList.begin(), timeList.end(), plotTime);
  if (it == timeList.end() && !timeList.empty()) {
    plotTime=nowTime();
    std::vector<miutil::miTime>::const_iterator it = std::find(timeList.begin(), timeList.end(), plotTime);
    if (it == timeList.end() && !timeList.empty()) {
      plotTime = timeList.back();
    }
  }
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
  for (vector <SelectedModel>::iterator p=selectedModels.begin(); p!=selectedModels.end(); p++)
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
  vpopt->readOptions(vstr);
}

