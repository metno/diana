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

#include <puCtools/glob.h>
#include <puCtools/glob_cache.h>
#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>

using namespace::miutil;

SpectrumManager::SpectrumManager()
: showObs(false), plotw(0), ploth(0), dataChange(true), hardcopy(false)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager constructed" << endl;
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
  cerr << "SpectrumManager destructor" << endl;
#endif

  if (spopt) delete spopt;

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
}


void SpectrumManager::parseSetup()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::parseSetup" << endl;
#endif

  //const miString section1 = "SPECTRUM_SETUP";
  const miString section2 = "SPECTRUM_FILES";
  vector<miString> vstr;

  //if (!SetupParser::getSection(section1,vstr)) {
  //  cerr << "Missing section " << section1 << " in setupfile." << endl;
  //  return false;
  //}
  //vstr.clear();

  if (SetupParser::getSection(section2,vstr)) {

    // not many error messages here yet...

    set<miString> uniquefiles;

    miString model,filename;
    vector<miString> tokens,tokens1,tokens2;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split('\"','\"'," ",true);
      if (tokens.size()==1) {
        tokens1= tokens[0].split("=");
        if (tokens1.size()==2) {
          if (tokens1[0].downcase()=="obs.aaa")
            obsAaaPaths.push_back(tokens1[1]);
          else if (tokens1[0].downcase()=="obs.bbb")
            obsBbbPaths.push_back(tokens1[1]);
        }
      } else if (tokens.size()==2) {
        tokens1= tokens[0].split("=");
        tokens2= tokens[1].split("=");
        if (tokens1.size()==2          && tokens2.size()==2  &&
            tokens1[0].downcase()=="m" && tokens2[0].downcase()=="f") {
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

    //cerr << "Missing section " << section2 << " in setupfile." << endl;

  }
}


void SpectrumManager::updateObsFileList()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::updateObsFileList" << endl;
#endif
  obsfiles.clear();
  int n= obsAaaPaths.size();
  for (int j=0; j<n; j++) {
    ObsFile of;
    of.obstype= obsAAA;
    of.time=    miTime(1970,1,1,0,0,0);
    of.modificationTime= 0;
    glob_t globBuf;
    glob_cache(obsAaaPaths[j].c_str(),0,0,&globBuf);
    for (int i=0; i<globBuf.gl_pathc; i++) {
      of.filename= miString(globBuf.gl_pathv[i]);
      obsfiles.push_back(of);
    }
    globfree_cache(&globBuf);
  }
  n= obsBbbPaths.size();
  for (int j=0; j<n; j++) {
    ObsFile of;
    of.obstype= obsBBB;
    of.time=    miTime(1970,1,1,0,0,0);
    of.modificationTime= 0;
    glob_t globBuf;
    glob_cache(obsBbbPaths[j].c_str(),0,0,&globBuf);
    for (int i=0; i<globBuf.gl_pathc; i++) {
      of.filename= miString(globBuf.gl_pathv[i]);
      obsfiles.push_back(of);
    }
    globfree_cache(&globBuf);
  }
}


void SpectrumManager::setPlotWindow(int w, int h)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::setPlotWindow:" << w << " " << h << endl;
#endif
  plotw= w;
  ploth= h;

  if (hardcopy) SpectrumPlot::resetPage();
}


//*************************routines from controller*************************

vector<miString> SpectrumManager::getLineThickness()
{
  vector<miString> linethickness;
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
  cerr << "SpectrumManager::setModel" << endl;
#endif

  // should not clear all data, possibly needed again...

  for (unsigned int i=0; i<spfile.size(); i++)
    delete spfile[i];
  spfile.clear();

  //check if there are any selected models, if not use default
  //   if (!selectedModels.size()&&!selectedFiles.size()
  //       &&(!asField || !fieldModels.size())){
  //     cerr << "No model selected" << endl;
  //     miString model = getDefaultModel();
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
  set <miString>::iterator p = usemodels.begin();
  for (; p!=usemodels.end(); p++) {
    miString model= *p;
    map<miString,miString>::iterator pf;
    pf= filenames.find(model);
    if (pf==filenames.end()) {
      cerr << "NO SPECTRUMFILE for model " << model << endl;
    } else
      initSpectrumFile(pf->second,model);
  }

  //define models/files  when "file" chosen in modeldialog
  vector <miString>::iterator q = selectedFiles.begin();
  for (; q!=selectedFiles.end(); q++) {
    miString file= *q;
    //HK ??? cheating...
    if (file.contains("obs")) {
      showObs = true;
    } else {
      map<miString,miString>::iterator pf=filenames.begin();
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

#ifdef DEBUGPRINT
  cerr << "SpectrumManager::setModels finished" << endl;
#endif
}


void SpectrumManager::setStation(const miString& station)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::setStation  " << station << endl;
#endif

  plotStation= station;

  dataChange= true;
}


void SpectrumManager::setTime(const miTime& time)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::setTime  " << time << endl;
#endif

  plotTime= time;

  if (onlyObs)
    initStations();

  dataChange= true;
}


miString SpectrumManager::setStation(int step)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::setStation   step=" << step << endl;
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
  cerr << "SpectrumManager::setTime   step=" << step << endl;
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
  cerr << "SpectrumManager::plot  " << plotStation << "  " << plotTime << endl;
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

  if (plotStation.exists()) {

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

      if (i<n && obsList[i].exists()) {
        checkObsTime(plotTime.hour());

        vector<miString> stationList;
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
              cerr<<"Exception in: " <<obsfiles[nn].filename<<endl;
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

#ifdef DEBUGPRINT
  cerr << "SpectrumManager::plot finished" << endl;
#endif
  return true;
}


void SpectrumManager::preparePlot()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::preparePlot" << endl;
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


vector <miString> SpectrumManager::getModelNames()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::getModelNames" << endl;
#endif
  updateObsFileList();
  return dialogModelNames;
}


vector <miString> SpectrumManager::getModelFiles()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::getModelFiles" << endl;
#endif
  vector<miString> modelfiles= dialogFileNames;
  updateObsFileList();
  int n= obsfiles.size();
  for (int i=0; i<n; i++)
    modelfiles.push_back(obsfiles[i].filename);

  return modelfiles;
}


void SpectrumManager::setFieldModels(const vector<miString>& fieldmodels)
{
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}


void SpectrumManager::setSelectedModels(const vector<miString>& models ,
    bool obs ,bool field)
{
  //called when model selected in model dialog
  showObs= obs;
  asField = field;
  //set data from models, not files
  selectedFiles.clear();
  selectedModels = models;
}


void SpectrumManager::setSelectedFiles(const vector<miString>& files,
    bool obs ,bool field)
{
  //called when model selected in model dialog
  showObs= obs;
  asField = field;
  //set data from files not models
  selectedModels.clear();
  selectedFiles = files;
}


miString SpectrumManager::getDefaultModel()
{
  //for now, just the first model in filenames list
  map<miString,miString>::iterator p = filenames.begin();
  miString model = p->first;
  return model;
}


vector<miString> SpectrumManager::getSelectedModels()
{
  vector <miString> models = selectedModels;
  if (showObs) models.push_back(menuConst["OBS"]);
  if (asField) models.push_back(menuConst["ASFIELD"]);
  return models;
}


bool SpectrumManager::initSpectrumFile(miString file,miString model)
{
  SpectrumFile *spf= new SpectrumFile(file,model);
  //if (spf->readFileHeader()) {
  if (spf->update()) {
    cerr << "SPECTRUMFILE READFILE OK for model " << model << endl;
    spfile.push_back(spf);
    return true;
  } else {
    cerr << "SPECTRUMFILE READFILE ERROR file " << file << endl;
    delete spf;
    return false;
  }
}


void SpectrumManager::initStations()
{
  //merge lists from all models
  int nspfile = spfile.size();
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::initStations-size of spfile " << nspfile << endl;
#endif

  nameList.clear();
  obsList.clear();

  map<miString,StationPos> stations;

  vector<miString> namelist;
  vector<float>    latitudelist;
  vector<float>    longitudelist;
  vector<miString> obslist;

  for (int i = 0;i<nspfile;i++){
    namelist= spfile[i]->getNames();
    latitudelist= spfile[i]->getLatitudes();
    longitudelist= spfile[i]->getLongitudes();
    //obslist= spfile[i]->getObsNames();
    obslist= spfile[i]->getNames();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size()||n!=longitudelist.size()||
        n!=obslist.size()) {
      cerr << "diSpectrumManager::initStations - SOMETHING WRONG WITH STATIONLIST!"
      << endl;
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
  cerr << "Number of stations" << nstations << endl;
#endif
  map<miString,StationPos>::iterator p=stations.begin();
  for (; p!=stations.end(); p++) {
    miString name=p->first;
    StationPos pos = p->second;
#ifdef DEBUGPRINT
    cerr <<"Station name " << name << endl;
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
  cerr << "lastStation"  << lastStation << endl;
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
  cerr <<"plotStation" << plotStation << endl;
#endif
}


void SpectrumManager::initTimes()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumManager::initTimes" << endl;
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
  cerr << "SpectrumManager::checkObsTime  hour= " << hour << endl;
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
            cerr<<"Exception in: "<<obsfiles[i].filename<<endl;
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
  cerr << "SpectrumManager::mainWindowTimeChanged  " << time << endl;
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
  cerr << "SpectrumManager::updateObs" << endl;
#endif
  updateObsFileList();
  checkObsTime();
}


miString SpectrumManager::getAnnotationString()
{
  miString str = miString("Bølgespekter ");
  if (onlyObs)
    str += plotTime.isoTime();
  else
    for (set <miString>::iterator p=usemodels.begin();p!=usemodels.end();p++)
      str+=*p+miString(" ");
  return str;
}


vector<miString> SpectrumManager::writeLog()
{
  return spopt->writeOptions();
}


void SpectrumManager::readLog(const vector<miString>& vstr,
    const miString& thisVersion,
    const miString& logVersion)
{
  spopt->readOptions(vstr);
}
