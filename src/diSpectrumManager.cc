/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#include <diSpectrumManager.h>

#include <diSpectrumOptions.h>
#include <diSpectrumFile.h>
#include <diSpectrumData.h>
#include <diSpectrumPlot.h>
#include "diUtilities.h"
#include "vcross_v2/VcrossSetup.h"
#include <diField/VcrossUtil.h>
#include "vcross_v2/VcrossCollector.h"

#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>
#include <puTools/mi_boost_compatibility.hh>

#define MILOGGER_CATEGORY "diana.SpectrumManager"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

SpectrumManager::SpectrumManager()
: plotw(0), ploth(0), dataChange(true), hardcopy(false)
{

  METLIBS_LOG_SCOPE();


  spopt= new SpectrumOptions();  // defaults are set

  //  parseSetup();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
}


SpectrumManager::~SpectrumManager()
{

  METLIBS_LOG_SCOPE();


  delete spopt;

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
  for (unsigned int i=0; i<spdata.size(); i++)
    delete spdata[i];
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
  vector<std::string> vstr;

  if (SetupParser::getSection(section2,vstr)) {

    set<std::string> uniquefiles;
    vector<std::string> sources;

    int n= vstr.size();

    for (int i=0; i<n; i++) {
      vector<std::string> tokens= miutil::split(vstr[i]);
      std::string model,filename;
      std::string filetype = "standard";
      for ( size_t j=0; j<tokens.size(); ++j) {
        vector<std::string> tokens1= miutil::split(tokens[j], "=");
        if ( tokens1.size() != 2 ) continue;
        if ( tokens1[0] == miutil::to_lower("m") ) {
          model = tokens1[1];
        } else if( tokens1[0] == miutil::to_lower("f") ) {
          filename = tokens1[1];
        } else if( tokens1[0] == miutil::to_lower("t") ) {
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

    setup = miutil::make_shared<vcross::Setup>();
    setup->configureSources(sources);
  }
}


void SpectrumManager::setPlotWindow(int w, int h)
{

  METLIBS_LOG_SCOPE(w << " " << h);

  plotw= w;
  ploth= h;

  if (hardcopy) SpectrumPlot::resetPage();
}


//*************************routines from controller*************************

vector<std::string> SpectrumManager::getLineThickness()
{
  vector<std::string> linethickness;
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
    map<std::string,std::string>::iterator pf;
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


std::string SpectrumManager::setStation(int step)
{

  METLIBS_LOG_SCOPE(step);


  if (nameList.size()==0)
    return "";

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
  return plotStation;
}


miTime SpectrumManager::setTime(int step)
{

  METLIBS_LOG_SCOPE(step);


  if (timeList.size()==0)
    return miTime::nowTime();

  int n= timeList.size();
  int i= 0;
  while (i<n && timeList[i]!=plotTime) i++;
  if (i<n) {
    i+=step;
    if (i<0)  i= 0;
    if (i>=n) i= n-1;
  } else {
    i= 0;
  }

  plotTime= timeList[i];
  dataChange= true;

  return plotTime;
}


// start hardcopy
void SpectrumManager::startHardcopy(const printOptions& po)
{
  if (hardcopy && hardcopystarted){
    // if hardcopy in progress and same filename: make new page
    if (po.fname == printoptions.fname){
      SpectrumPlot::startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    SpectrumPlot::endPSoutput();
  }
  hardcopy= true;
  printoptions= po;
  hardcopystarted= false;
}


// end hardcopy plot
void SpectrumManager::endHardcopy(){
  // postscript output
  if (hardcopy) SpectrumPlot::endPSoutput();
  hardcopy= false;
}


bool SpectrumManager::plot()
{

  METLIBS_LOG_SCOPE(LOGVAL(plotStation) << LOGVAL(plotTime));


  if (dataChange) {
    preparePlot();
    dataChange= false;
  }

  // postscript output
  if (hardcopy && !hardcopystarted) {
    SpectrumPlot::startPSoutput(printoptions);
    hardcopystarted= true;
  }

  int nmod= spfile.size();
  if (nmod == 0) nmod = spdata.size();

  SpectrumPlot::startPlot(nmod,plotw,ploth,spopt);

  if (not plotStation.empty()) {

    int m= spectrumplots.size();

    for (int i=0; i<m; i++) {
      if (spectrumplots[i]) {
        spectrumplots[i]->plot(spopt);
      }
    }

  }

  SpectrumPlot::plotDiagram(spopt);

  return true;
}


void SpectrumManager::preparePlot()
{

  METLIBS_LOG_SCOPE();

  int n= spectrumplots.size();
  for (int i=0; i<n; i++)
    delete spectrumplots[i];
  spectrumplots.clear();

  if (plotStation.empty())
    return;

  int m= spfile.size();

  for (int i=0; i<m; i++) {
    SpectrumPlot *spp= spfile[i]->getData(plotStation,plotTime);
    spectrumplots.push_back(spp);
  }
  for (size_t i=0; i<spdata.size(); i++) {
    SpectrumPlot *spp= spdata[i]->getData(plotStation,plotTime);
    spectrumplots.push_back(spp);
  }
}


vector <std::string> SpectrumManager::getModelNames()
{

  METLIBS_LOG_SCOPE();
parseSetup();
  return dialogModelNames;
}


vector <std::string> SpectrumManager::getModelFiles()
{

  METLIBS_LOG_SCOPE();

  vector<std::string> modelfiles= dialogFileNames;

  return modelfiles;
}

std::vector <std::string> SpectrumManager::getReferencetimes(const std::string& modelName)
{
  std::vector <std::string> rf;
  if ( filetypes[modelName] == "standard" )
    return rf;

  vcross::Collector_p collector = miutil::make_shared<vcross::Collector>(setup);

  collector->getResolver()->getSource(modelName)->update();
  const vcross::Time_s reftimes = collector->getResolver()->getSource(modelName)->getReferenceTimes();
   vector<miTime> rtv;
  rtv.reserve(reftimes.size());
  for (vcross::Time_s::const_iterator it=reftimes.begin(); it != reftimes.end(); ++it){
    rf.push_back(vcross::util::to_miTime(*it).isoTime("T"));
  }

  return rf;
}

void SpectrumManager::setSelectedModels(const vector<std::string>& models)
{
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


std::string SpectrumManager::getDefaultModel()
{
  //for now, just the first model in filenames list
  map<std::string,std::string>::iterator p = filenames.begin();
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

  map<std::string,StationPos> stations;

  vector<std::string> namelist;
  vector<float>    latitudelist;
  vector<float>    longitudelist;

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

  map<std::string,StationPos>::iterator p=stations.begin();
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

  // remember station
  if (!plotStation.empty()) lastStation = plotStation;

  METLIBS_LOG_DEBUG("lastStation"  << lastStation);

  //if it's the first time, plotStation is first in list
  if (lastStation.empty() && nameList.size())
    plotStation=nameList[0];
  else{
    int n = nameList.size();
    bool found = false;
    //find plot station
    for (int i=0;i<n;i++){
      if(nameList[i]== lastStation){
        plotStation=nameList[i];
        found=true;
      }
    }
    if (!found) plotStation.clear();
    dataChange= true;
  }

  METLIBS_LOG_DEBUG("plotStation" << plotStation);

}


void SpectrumManager::initTimes()
{

  METLIBS_LOG_DEBUG("SpectrumManager::initTimes");


  timeList.clear();

  //assume common times...
  if (spdata.size())
    timeList= spdata[0]->getTimes();
  else if (spfile.size())
    timeList= spfile[0]->getTimes();

  int n= timeList.size();
  int i= 0;
  while (i<n && timeList[i]!=plotTime) i++;

  if (i==n && n>0) {
      plotTime= timeList[0];
  }
}

void SpectrumManager::mainWindowTimeChanged(const miTime& time)
{

  METLIBS_LOG_SCOPE(time);


  miTime mainWindowTime = time;
  //change plotTime
  int maxdiff= miTime::minDiff (mainWindowTime,ztime);
  int diff,itime=-1;
  int n = timeList.size();
  for (int i=0;i<n;i++){
    diff = abs(miTime::minDiff(timeList[i],mainWindowTime));
    if(diff<maxdiff){
      maxdiff = diff;
      itime=i;
    }
  }
  if (itime>-1) setTime(timeList[itime]);
}




std::string SpectrumManager::getAnnotationString()
{
  std::string str = std::string("Wave spectrum ");
  for (vector <SelectedModel>::iterator p=selectedModels.begin();p!=selectedModels.end();p++)
    str+=(*p).model+std::string(" ");
  return str;
}


vector<string> SpectrumManager::writeLog()
{
  return spopt->writeOptions();
}


void SpectrumManager::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion)
{
  spopt->readOptions(vstr);
}
