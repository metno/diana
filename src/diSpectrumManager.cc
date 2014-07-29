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
#include <diSpectrumPlot.h>
#include "diUtilities.h"

#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>

#define MILOGGER_CATEGORY "diana.SpectrumManager"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

SpectrumManager::SpectrumManager()
: showObs(false), plotw(0), ploth(0), dataChange(true), hardcopy(false)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  spopt= new SpectrumOptions();  // defaults are set

  parseSetup();

  updateObsFileList();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
}


SpectrumManager::~SpectrumManager()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  delete spopt;

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
}


void SpectrumManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  //clear old setupinfo
  dialogModelNames.clear();
  dialogFileNames.clear();

  //const std::string section1 = "SPECTRUM_SETUP";
  const std::string section2 = "SPECTRUM_FILES";
  vector<std::string> vstr;

  //if (!SetupParser::getSection(section1,vstr)) {
  //  METLIBS_LOG_DEBUG("Missing section " << section1 << " in setupfile.");
  //  return false;
  //}
  //vstr.clear();

  if (SetupParser::getSection(section2,vstr)) {

    // not many error messages here yet...

    set<std::string> uniquefiles;

    std::string model,filename;
    vector<std::string> tokens,tokens1,tokens2;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= miutil::split_protected(vstr[i], '\"','\"'," ",true);
      if (tokens.size()==1) {
        tokens1= miutil::split(tokens[0], "=");
        if (tokens1.size()==2) {
          if (miutil::to_lower(tokens1[0])=="obs.aaa")
            obsAaaPaths.push_back(tokens1[1]);
          else if (miutil::to_lower(tokens1[0])=="obs.bbb")
            obsBbbPaths.push_back(tokens1[1]);
        }
      } else if (tokens.size()==2) {
        tokens1= miutil::split(tokens[0], "=");
        tokens2= miutil::split(tokens[1], "=");
        if (tokens1.size()==2          && tokens2.size()==2  &&
            miutil::to_lower(tokens1[0])=="m" && miutil::to_lower(tokens2[0])=="f") {
          model= tokens1[1];
          filename= tokens2[1];
          filenames[model]= filename;
          if (uniquefiles.find(filename)==uniquefiles.end()) {
            uniquefiles.insert(filename);
            dialogModelNames.push_back(model);
            dialogFileNames.push_back(filename);
          }
        }
      }
    }

    vstr.clear();

  } else {

    //METLIBS_LOG_DEBUG("Missing section " << section2 << " in setupfile.");

  }
}


void SpectrumManager::updateObsFileList()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  obsfiles.clear();
  for (diutil::string_v::const_iterator ita = obsAaaPaths.begin(); ita != obsAaaPaths.end(); ++ita) {
    ObsFile of;
    of.obstype= obsAAA;
    of.time=    miTime(1970,1,1,0,0,0);
    of.modificationTime= 0;

    const diutil::string_v matches = diutil::glob(*ita);
    for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
      of.filename = *it;
      obsfiles.push_back(of);
    }
  }
  for (diutil::string_v::const_iterator itb = obsBbbPaths.begin(); itb != obsBbbPaths.end(); ++itb) {
    ObsFile of;
    of.obstype= obsBBB;
    of.time=    miTime(1970,1,1,0,0,0);
    of.modificationTime= 0;

    const diutil::string_v matches = diutil::glob(*itb);
    for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
      of.filename = *it;
      obsfiles.push_back(of);
    }
  }
}


void SpectrumManager::setPlotWindow(int w, int h)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(w << " " << h);
#endif
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
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  // should not clear all data, possibly needed again...

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
  spfile.clear();

  //check if there are any selected models, if not use default
  //   if (!selectedModels.size()&&!selectedFiles.size()
  //       &&(!asField || !fieldModels.size())){
  //     METLIBS_LOG_DEBUG("No model selected");
  //     std::string model = getDefaultModel();
  //     usemodels.insert(model);
  //   }

  usemodels.clear();

  //if as field is selected find corresponding model
  if (asField){
    int n = fieldModels.size();
    for (int i=0;i<n;i++)
      usemodels.insert(fieldModels[i]);
  }

  //models from model dialog
  int m= selectedModels.size();
  for (int i=0;i<m;i++)
    usemodels.insert(selectedModels[i]);

  //define models/files  when "model" chosen in modeldialog
  set <std::string>::iterator p = usemodels.begin();
  for (; p!=usemodels.end(); p++) {
    std::string model= *p;
    map<std::string,std::string>::iterator pf;
    pf= filenames.find(model);
    if (pf==filenames.end()) {
      METLIBS_LOG_ERROR("NO SPECTRUMFILE for model " << model);
    } else
      initSpectrumFile(pf->second,model);
  }

  //define models/files  when "file" chosen in modeldialog
  vector<string>::iterator q = selectedFiles.begin();
  for (; q!=selectedFiles.end(); q++) {
    std::string file= *q;
    //HK ??? cheating...
    if (miutil::contains(file, "obs")) {
      showObs = true;
    } else {
      map<std::string,std::string>::iterator pf=filenames.begin();
      for (; pf!=filenames.end(); pf++) {
        if (file==pf->second){
          initSpectrumFile(file,pf->first);
          break;
        }
      }
    }
  }

  onlyObs= false;

  if (showObs && spfile.size()==0) {
    // until anything decided:
    // check observation time and display the most recent file
    checkObsTime();
    onlyObs= true;
  }

  initTimes();
  initStations();

  dataChange= true;
}


void SpectrumManager::setStation(const std::string& station)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(station);
#endif

  plotStation= station;

  dataChange= true;
}


void SpectrumManager::setTime(const miTime& time)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(time);
#endif

  plotTime= time;

  if (onlyObs)
    initStations();

  dataChange= true;
}


std::string SpectrumManager::setStation(int step)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(step);
#endif

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
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(step);
#endif

  if (timeList.size()==0)
    return miTime::nowTime();

  int n= timeList.size();
  int i= 0;
  while (i<n && timeList[i]!=plotTime) i++;
  if (i<n) {
    i+=step;
    //if (i<0)  i= n-1;
    //if (i>=n) i= 0;
    //HK changed to noncyclic...
    if (i<0)  i= 0;
    if (i>=n) i= n-1;
  } else {
    i= 0;
  }

  plotTime= timeList[i];

  if (onlyObs)
    initStations();

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
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(LOGVAL(plotStation) << LOGVAL(plotTime));
#endif

  if (dataChange) {
    preparePlot();
    dataChange= false;
  }

  // postscript output
  if (hardcopy && !hardcopystarted) {
    SpectrumPlot::startPSoutput(printoptions);
    hardcopystarted= true;
  }

  int nobs= (showObs) ? 1 : 0;
  int nmod= spfile.size();

  SpectrumPlot::startPlot(nobs+nmod,plotw,ploth,spopt);

  if (not plotStation.empty()) {

    int m= spectrumplots.size();

    for (int i=0; i<m; i++) {
      if (spectrumplots[i]) {
        spectrumplots[i]->plot(spopt);
      }
    }

    if (showObs) {
      int n= nameList.size();
      int i= 0;
      while (i<n && nameList[i]!=plotStation) i++;

      if (i<n && not obsList[i].empty()) {
        checkObsTime(plotTime.hour());

        vector<std::string> stationList;
        stationList.push_back(obsList[i]);
        SpectrumPlot *spp= 0;
        /**********************************************************************
        int nn= 0;
        while (spp==0 && nn<nf) {
	  if (obsfiles[nn].modificationTime &&
	      obsfiles[nn].time==plotTime) {
            try {
	      if (obsfiles[nn].obstype==obsAAA) {
	        SpectrumObsAAA spobs(obsfiles[nn].filename,stationList);
	        spp= spobs.getStation(obsList[i],plotTime);
	      } else if (obsfiles[nn].obstype==obsBBB) {
	        SpectrumObsBBB spobs(obsfiles[nn].filename,stationList);
	        spp= spobs.getStation(obsList[i],plotTime);
	      }
            }
            catch (...) {
              METLIBS_LOG_DEBUG("Exception in: " <<obsfiles[nn].filename);
            }
          }
          nn++;
        }
         **********************************************************************/
        if (spp) {
          spp->plot(spopt);
          delete spp;
        }
      }
    }

    //    SpectrumPlot::plotText(); // ????????????????????????????????
  }

  SpectrumPlot::plotDiagram(spopt);

  // postscript output
  //  if (hardcopy) SpectrumPlot::endPSoutput();
  //  hardcopy= false;

  return true;
}


void SpectrumManager::preparePlot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

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
}


vector <std::string> SpectrumManager::getModelNames()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  updateObsFileList();
  return dialogModelNames;
}


vector <std::string> SpectrumManager::getModelFiles()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  vector<std::string> modelfiles= dialogFileNames;
  updateObsFileList();
  int n= obsfiles.size();
  for (int i=0; i<n; i++)
    modelfiles.push_back(obsfiles[i].filename);

  return modelfiles;
}


void SpectrumManager::setFieldModels(const vector<string>& fieldmodels)
{
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}


void SpectrumManager::setSelectedModels(const vector<string>& models, bool obs ,bool field)
{
  //called when model selected in model dialog
  showObs= obs;
  asField = field;
  //set data from models, not files
  selectedFiles.clear();
  selectedModels = models;
}


void SpectrumManager::setSelectedFiles(const vector<string>& files, bool obs ,bool field)
{
  //called when model selected in model dialog
  showObs= obs;
  asField = field;
  //set data from files not models
  selectedModels.clear();
  selectedFiles = files;
}


std::string SpectrumManager::getDefaultModel()
{
  //for now, just the first model in filenames list
  map<std::string,std::string>::iterator p = filenames.begin();
  std::string model = p->first;
  return model;
}


vector<string> SpectrumManager::getSelectedModels()
{
  vector <string> models = selectedModels;
  if (showObs)
    models.push_back(menuConst["OBS"]);
  if (asField)
    models.push_back(menuConst["ASFIELD"]);
  return models;
}


bool SpectrumManager::initSpectrumFile(std::string file, std::string model)
{
  SpectrumFile *spf= new SpectrumFile(file,model);
  //if (spf->readFileHeader()) {
  if (spf->update()) {
    METLIBS_LOG_INFO("SPECTRUMFILE READFILE OK for model " << model);
    spfile.push_back(spf);
    return true;
  } else {
    METLIBS_LOG_ERROR("SPECTRUMFILE READFILE ERROR file " << file);
    delete spf;
    return false;
  }
}


void SpectrumManager::initStations()
{
  //merge lists from all models
  int nspfile = spfile.size();
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("size of spfile " << nspfile);
#endif

  nameList.clear();
  obsList.clear();

  map<std::string,StationPos> stations;

  vector<std::string> namelist;
  vector<float>    latitudelist;
  vector<float>    longitudelist;
  vector<std::string> obslist;

  for (int i = 0;i<nspfile;i++){
    namelist= spfile[i]->getNames();
    latitudelist= spfile[i]->getLatitudes();
    longitudelist= spfile[i]->getLongitudes();
    //obslist= spfile[i]->getObsNames();
    obslist= spfile[i]->getNames();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size()||n!=longitudelist.size()||
        n!=obslist.size()) {
      METLIBS_LOG_ERROR("diSpectrumManager::initStations - SOMETHING WRONG WITH STATIONLIST!");
    } else{
      for (unsigned int j = 0;j<n;j++){
        StationPos newPos;
        newPos.latitude= latitudelist[j];
        newPos.longitude=longitudelist[j];
        newPos.obs=obslist[j];
        stations[namelist[j]] = newPos;
      }
    }
  }

  namelist.clear();
  latitudelist.clear();
  longitudelist.clear();
  obslist.clear();

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Number of stations" << nstations);
#endif
  map<std::string,StationPos>::iterator p=stations.begin();
  for (; p!=stations.end(); p++) {
    std::string name=p->first;
    StationPos pos = p->second;
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Station name " << name);
#endif
    namelist.push_back(name);
    latitudelist.push_back(pos.latitude);
    longitudelist.push_back(pos.longitude);
    obslist.push_back(pos.obs);
  }
  nameList=     namelist;
  latitudeList= latitudelist;
  longitudeList=longitudelist;
  obsList=      obslist;

  // remember station
  if (!plotStation.empty()) lastStation = plotStation;
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("lastStation"  << lastStation);
#endif
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
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("plotStation" << plotStation);
#endif
}


void SpectrumManager::initTimes()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumManager::initTimes");
#endif

  timeList.clear();

  //assume common times...
  if (spfile.size()) timeList= spfile[0]->getTimes();

  if (onlyObs)
    timeList= obsTime;

  int n= timeList.size();
  int i= 0;
  while (i<n && timeList[i]!=plotTime) i++;

  if (i==n && n>0) {
    if (onlyObs)
      plotTime= timeList[n-1]; // the newest observations
    else
      plotTime= timeList[0];
  }
}


void SpectrumManager::checkObsTime(int hour)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("hour= " << hour);
#endif

  // use hour=-1 to check all files
  // hour otherwise used to spread checking of many files (with plotTime.hour)

  if (hour>23) hour=-1;
  bool newtime= false;
  int n= obsfiles.size();

  pu_struct_stat statbuf;

  for (int i=0; i<n; i++) {
    if (obsfiles[i].modificationTime==0 || hour<0 ||
        obsfiles[i].time.hour()==hour) {
      if (pu_stat(obsfiles[i].filename.c_str(),&statbuf)==0) {
        if (obsfiles[i].modificationTime!=statbuf.st_mtime) {
          obsfiles[i].modificationTime= statbuf.st_mtime;
          /***************************************************************************
          try {
            obs ofile;
	    ofile.readFileHeader(obsfiles[i].filename);
	    if (obsfiles[i].time != ofile.fileObsTime()) newtime= true;
	    obsfiles[i].time= ofile.fileObsTime();
          }
          catch (...) {
            METLIBS_LOG_DEBUG("Exception in: "<<obsfiles[i].filename);
	  }
           ***************************************************************************/
        }
      }
    }
  }

  if (newtime && hour<0) {
    set<miTime> timeset;
    for (int i=0; i<n; i++)
      if (obsfiles[i].modificationTime)
        timeset.insert(obsfiles[i].time);
    obsTime.clear();
    set<miTime>::iterator p= timeset.begin(), pend= timeset.end();
    for (; p!=pend; p++)
      obsTime.push_back(*p);
  }
}


void SpectrumManager::mainWindowTimeChanged(const miTime& time)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(time);
#endif

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


void SpectrumManager::updateObs()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  updateObsFileList();
  checkObsTime();
}


std::string SpectrumManager::getAnnotationString()
{
  std::string str = std::string("B�lgespekter ");
  if (onlyObs)
    str += plotTime.isoTime();
  else
    for (set <std::string>::iterator p=usemodels.begin();p!=usemodels.end();p++)
      str+=*p+std::string(" ");
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
