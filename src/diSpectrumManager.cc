/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diSpectrumManager.h"

#include "diField/VcrossUtil.h"
#include "diSpectrumData.h"
#include "diSpectrumFile.h"
#include "diSpectrumOptions.h"
#include "diSpectrumPlot.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/misc_util.h"
#include "util/time_util.h"
#include "vcross_v2/VcrossCollector.h"
#include "vcross_v2/VcrossSetup.h"

#include <QSize>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.SpectrumManager"
#include <miLogger/miLogging.h>

using namespace::miutil;

SpectrumManager::SpectrumManager()
  : spopt(new SpectrumOptions)  // defaults are set
  , plotw(0), ploth(0)
  , realizationCount(1)
  , realization(0)
  , dataChange(true)
{
  METLIBS_LOG_SCOPE();

  plotTime= miTime::nowTime();
}


SpectrumManager::~SpectrumManager()
{
  METLIBS_LOG_SCOPE();

  diutil::delete_all_and_clear(spfile);
  diutil::delete_all_and_clear(spdata);
}


void SpectrumManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  //clear old setupinfo
  filenames.clear();
  filetypes.clear();
  filesetup.clear();
  dialogModelNames.clear();
  dialogFileNames.clear();

  const std::string section2 = "SPECTRUM_FILES";
  std::vector<std::string> vstr;

  if (SetupParser::getSection(section2,vstr)) {

    std::set<std::string> uniquefiles;
    std::vector<std::string> sources;

    int n= vstr.size();

    for (int i=0; i<n; i++) {
      std::vector<std::string> tokens= miutil::split(vstr[i]);
      std::string model,filename;
      std::string filetype = "standard";
      for ( size_t j=0; j<tokens.size(); ++j) {
        std::vector<std::string> tokens1= miutil::split(tokens[j], "=");
        if (tokens1.size() != 2)
          continue;
        if (tokens1[0] == "m") {
          model = tokens1[1];
        } else if (tokens1[0] == "f") {
          filename = tokens1[1];
        } else if (tokens1[0] == "t") {
          filetype = tokens1[1];
        }
      }
      if ( filetype !="standard" ) {
        sources.push_back(vstr[i]);
      }

      filenames[model]= filename;
      filetypes[model]=filetype;
      filesetup[model] = vstr[i];
      if (uniquefiles.find(filename)==uniquefiles.end()) {
        uniquefiles.insert(filename);
        dialogModelNames.push_back(model);
        dialogFileNames.push_back(filename);
      }
    }

    setup = std::make_shared<vcross::Setup>();
    setup->configureSources(sources);
  }
}

void SpectrumManager::setPlotWindow(const QSize& size)
{
  METLIBS_LOG_SCOPE();
  plotw = size.width();
  ploth = size.height();
}


//*************************routines from controller*************************

std::vector<std::string> SpectrumManager::getLineThickness()
{
  std::vector<std::string> linethickness;
  linethickness.push_back("1");
  linethickness.push_back("2");
  linethickness.push_back("3");
  linethickness.push_back("4");
  linethickness.push_back("5");
  linethickness.push_back("6");

  return linethickness;
}

//*********************** end routines from controller ***********************

void SpectrumManager::setModel()
{
  METLIBS_LOG_SCOPE();

  // should not clear all data, possibly needed again...

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
  spfile.clear();
  for (unsigned int i=0; i<spdata.size(); i++)
    delete spdata[i];
  spdata.clear();

  //models from model dialog
  int m= selectedModels.size();
  for (int i=0;i<m;i++) {
    std::map<std::string,std::string>::iterator pf;
    pf= filenames.find(selectedModels[i].model);
    if (pf==filenames.end()) {
      METLIBS_LOG_ERROR("NO SPECTRUMFILE for model " << selectedModels[i].model);
    } else {
      initSpectrumFile(selectedModels[i]);
    }
  }

  initTimes();
  initStations();

  dataChange= true;
}


void SpectrumManager::setRealization(int r)
{
  realization = std::max(std::min(r, realizationCount-1), 0);
}

void SpectrumManager::setStation(const std::string& station)
{
  METLIBS_LOG_SCOPE(station);

  plotStation= station;
  dataChange= true;
}


void SpectrumManager::setTime(const miTime& time)
{
  METLIBS_LOG_SCOPE(time);

  plotTime= time;
  dataChange= true;
}

void SpectrumManager::setTime(int step, int dir)
{
  METLIBS_LOG_DEBUG(LOGVAL(step) << LOGVAL(dir));

  if (timeList.empty())
    return;

  plottimes_t::const_iterator it;
  if (step == 0) {
    it = miutil::step_time(timeList, plotTime, dir);
  } else {
    it = miutil::step_time(timeList, plotTime, miutil::addHour(plotTime, step * dir));
  }
  if (it == timeList.end())
    it = --timeList.end();

  plotTime = *it;

  dataChange= true;
}

void SpectrumManager::setStation(int step)
{
  METLIBS_LOG_SCOPE(step);

  if (nameList.empty())
    return;

  int i= 0;
  int n= nameList.size();
  if (!plotStation.empty())
    while (i<n && nameList[i]!=plotStation) i++;

  if (i<n) {
    i+=step;
    if (i<0)  i= n-1;
    if (i>=n) i= 0;
  } else {
    i= 0;
  }

  dataChange= true;
  plotStation= nameList[i];
}

void SpectrumManager::setTime(int step)
{
  METLIBS_LOG_SCOPE(step);
  setTime(0, step);
}

bool SpectrumManager::plot(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(LOGVAL(plotStation) << LOGVAL(plotTime));

  if (dataChange) {
    preparePlot();
    dataChange= false;
  }

  int nmod= spfile.size();
  if (nmod == 0)
    nmod = spdata.size();

  SpectrumPlot::startPlot(nmod, plotw, ploth, spopt.get(), gl);

  if (!plotStation.empty()) {
    for (SpectrumPlot* sp : spectrumplots) {
      if (sp)
        sp->plot(spopt.get(), gl);
    }
  }

  SpectrumPlot::plotDiagram(spopt.get(), gl);
  return true;
}


void SpectrumManager::preparePlot()
{
  METLIBS_LOG_SCOPE();

  diutil::delete_all_and_clear(spectrumplots);

  if (plotStation.empty())
    return;

  for (SpectrumFile* sf : spfile) {
    spectrumplots.push_back(sf->getData(plotStation, plotTime));
  }
  for (SpectrumData* sd: spdata) {
    spectrumplots.push_back(sd->getData(plotStation, plotTime, realization));
  }
}


std::vector <std::string> SpectrumManager::getModelNames()
{
  METLIBS_LOG_SCOPE();
  parseSetup();
  return dialogModelNames;
}


std::vector <std::string> SpectrumManager::getModelFiles()
{
  METLIBS_LOG_SCOPE();
  return dialogFileNames;
}

std::vector <std::string> SpectrumManager::getReferencetimes(const std::string& modelName)
{
  std::vector <std::string> rf;
  if ( filetypes[modelName] == "standard" )
    return rf;

  vcross::Collector_p collector = std::make_shared<vcross::Collector>(setup);

  collector->getResolver()->getSource(modelName)->update();
  const vcross::Time_s reftimes = collector->getResolver()->getSource(modelName)->getReferenceTimes();
  std::vector<miTime> rtv;
  rtv.reserve(reftimes.size());
  for (vcross::Time_s::const_iterator it=reftimes.begin(); it != reftimes.end(); ++it){
    rf.push_back(vcross::util::to_miTime(*it).isoTime("T"));
  }

  return rf;
}

void SpectrumManager::setSelectedModels(const std::vector<std::string>& models)
{
  selectedModels.clear();
  for ( size_t i=0; i<models.size(); ++i ) {
    SelectedModel selectedModel;
    std::vector<std::string> vstr = miutil::split(models[i]," ");
    if ( vstr.size() > 0 ) {
      selectedModel.model = vstr[0];
    }
    if ( vstr.size() > 1 ) {
      selectedModel.reftime = vstr[1];
    }
    selectedModels.push_back(selectedModel);
  }
}


std::string SpectrumManager::getDefaultModel()
{
  //for now, just the first model in filenames list
  std::map<std::string,std::string>::iterator p = filenames.begin();
  std::string model = p->first;
  return model;
}


bool SpectrumManager::initSpectrumFile(const SelectedModel& selectedModel)
{
  METLIBS_LOG_SCOPE();
  if ( filetypes[selectedModel.model] == "standard" ) {
    SpectrumFile *spf= new SpectrumFile(filenames[selectedModel.model],selectedModel.model);
    if (spf->update()) {
      METLIBS_LOG_INFO("OK for model " << selectedModel.model);
      spfile.push_back(spf);
      return true;
    } else {
      METLIBS_LOG_ERROR("Model " << selectedModel.model);
      delete spf;
      return false;
    }
  } else if ( filetypes[selectedModel.model] == "netcdf" ) {
    SpectrumData *spd = new SpectrumData(selectedModel.model);

    if ( spd->readFileHeader(setup,selectedModel.reftime)) {
      METLIBS_LOG_INFO("OK for model " << selectedModel.model);
      spdata.push_back(spd);
      return true;
    } else {
      METLIBS_LOG_ERROR("Model " << selectedModel.model);
      delete spd;
      return false;
    }
  }
  return false;
}

void SpectrumManager::initStations()
{
  METLIBS_LOG_SCOPE();

  //merge lists from all models

  nameList.clear();

  std::map<std::string,StationPos> stations;

  std::vector<std::string> namelist;
  std::vector<float>    latitudelist;
  std::vector<float>    longitudelist;

  int nspfile = spfile.size();
  for (int i = 0;i<nspfile;i++){
    namelist= spfile[i]->getNames();
    latitudelist= spfile[i]->getLatitudes();
    longitudelist= spfile[i]->getLongitudes();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size() || n!=longitudelist.size()) {
      METLIBS_LOG_ERROR("diSpectrumManager::initStations - SOMETHING WRONG WITH STATIONLIST!");
    } else{
      for (unsigned int j = 0;j<n;j++){
        StationPos newPos;
        newPos.latitude= latitudelist[j];
        newPos.longitude=longitudelist[j];
        stations[namelist[j]] = newPos;
      }
    }
  }

  int nspdata = spdata.size();
  for (int i = 0;i<nspdata;i++){
    namelist= spdata[i]->getNames();
    latitudelist= spdata[i]->getLatitudes();
    longitudelist= spdata[i]->getLongitudes();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size() || n!=longitudelist.size()) {
      METLIBS_LOG_ERROR("diSpectrumManager::initStations - SOMETHING WRONG WITH STATIONLIST!");
    } else{
      for (unsigned int j = 0;j<n;j++){
        StationPos newPos;
        newPos.latitude= latitudelist[j];
        newPos.longitude=longitudelist[j];
        stations[namelist[j]] = newPos;
      }
    }
  }

  namelist.clear();
  latitudelist.clear();
  longitudelist.clear();

  std::map<std::string,StationPos>::iterator p=stations.begin();
  for (; p!=stations.end(); p++) {
    std::string name=p->first;
    StationPos pos = p->second;

    METLIBS_LOG_DEBUG("Station name " << name);

    namelist.push_back(name);
    latitudelist.push_back(pos.latitude);
    longitudelist.push_back(pos.longitude);
  }
  nameList=     namelist;
  latitudeList= latitudelist;
  longitudeList=longitudelist;

  METLIBS_LOG_DEBUG("plotStation" << plotStation);
}


void SpectrumManager::initTimes()
{
  METLIBS_LOG_SCOPE();

  timeList.clear();

  //assume common times...
  if (spdata.size()) {
    diutil::insert_all(timeList, spdata[0]->getTimes());
    realizationCount = spdata[0]->getRealizationCount();
  } else if (spfile.size()) {
    diutil::insert_all(timeList, spfile[0]->getTimes());
    realizationCount = 1;
  }
  setRealization(realization);
}


void SpectrumManager::mainWindowTimeChanged(const miTime& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time));

  plottimes_t::const_iterator best = miutil::nearest(timeList, time);
  if (best != timeList.end())
    setTime(*best);
}


std::string SpectrumManager::getAnnotationString()
{
  std::string str = "Wave spectrum";
  for (const SelectedModel& sm : selectedModels)
    str += " " + sm.model;
  return str;
}


std::vector<std::string> SpectrumManager::writeLog()
{
  return spopt->writeOptions();
}

void SpectrumManager::readLog(const std::vector<std::string>& vstr, const std::string& /*thisVersion*/, const std::string& /*logVersion*/)
{
  spopt->readOptions(vstr);
}
