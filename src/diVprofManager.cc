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

#ifdef BUFROBS
#include "diObsBufr.h"
#endif // BUFROBS

#ifdef ROADOBS
#ifdef NEWARK_INC
#include <newarkAPI/diStation.h>
#include <newarkAPI/diRoaddata.h>
#else
#include <roadAPI/diStation.h>
#endif // !NEWARK_INC
#include "diVprofRTemp.h"
#endif // ROADOBS

#include <diField/diFieldManager.h>
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>
#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>

#define MILOGGER_CATEGORY "diana.VprofManager"
#include <miLogger/miLogging.h>

#ifdef ROADOBS
using namespace road;
#endif

using namespace std;
using miutil::miTime;

namespace {
inline vector<string> to_vector_string(const vector<miutil::miString>& m)
{ return vector<string>(m.begin(), m.end()); }
}

//#define DEBUGPRINT 1

VprofManager::VprofManager(Controller* co)
: amdarStationList(false), vpdiag(0), showObs(false),
  showObsTemp(false), showObsPilot(false), showObsAmdar(false),
  plotw(0), ploth(0), hardcopy(false)
{

  METLIBS_LOG_SCOPE();

  fieldm= co->getFieldManager(); // set fieldmanager

  vpopt= new VprofOptions();  // defaults are set

  parseSetup();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
}


VprofManager::~VprofManager()
{

  METLIBS_LOG_SCOPE();


  if (vpdiag) delete vpdiag;
  if (vpopt)  delete vpopt;

  // clean up vpdata...
  cleanup();
}


void VprofManager::cleanup()
{
// clean up vpdata...
  for (unsigned int i=0; i<vpdata.size(); i++)
    delete vpdata[i];
  vpdata.clear();
// NOTE: Flush the field cache
  fieldm->fieldcache->flush();
}


void VprofManager::parseSetup()
{

  METLIBS_LOG_SCOPE();


  filenames.clear();
  filetypes.clear();
  filesetup.clear();
  dialogModelNames.clear();
  dialogFileNames.clear();
  filePaths.clear();

  const std::string section2 = "VERTICAL_PROFILE_FILES";
  vector<std::string> vstr;

  if (miutil::SetupParser::getSection(section2,vstr)) {

    set<std::string> uniquefiles;

    std::string model,filename;
    vector<std::string> tokens,tokens1,tokens2, tokens3, tokens4;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= miutil::split(vstr[i]);
      if (tokens.size()==1) {
        tokens1= miutil::split(tokens[0], "=");
        if (tokens1.size()==2) {
          const std::string tokens1_0_lc = miutil::to_lower(tokens1[0]);
          ObsFilePath ofp;
          if (miutil::contains(tokens1_0_lc, "bufr."))
            ofp.fileformat = bufr;
          else
            continue;
          if (miutil::contains(tokens1_0_lc, ".temp")) {
            ofp.obstype = temp;
          } else if (miutil::contains(tokens1_0_lc, ".amdar")) {
            ofp.obstype = amdar;
          } else if (miutil::contains(tokens1_0_lc, ".pilot")) {
            ofp.obstype = pilot;
          } else {
            continue;
          }
          ofp.tf.initFilter(tokens1[1]);
          ofp.filepath = tokens1[1];
          filePaths.push_back(ofp);
        }
      } 
#ifdef ROADOBS
      /* Here we know that it is the extended obs format */
      else if (tokens.size()==4) {
        tokens1= miutil::split(tokens[0], "=");
        tokens2= miutil::split(tokens[1], "=");
        tokens3= miutil::split(tokens[2], "=");
        tokens4= miutil::split(tokens[3], "=");
        if (tokens1.size()==2 && tokens2.size()==2
            && tokens3.size()==2 && tokens4.size()==2
            && miutil::to_lower(tokens2[0])=="p" && miutil::to_lower(tokens3[0])=="s"
                && miutil::to_lower(tokens4[0])=="d") {
          ObsFilePath ofp;
          /* the check for roadobs must be before the check of metnoobs. or obs. */
          if (miutil::contains(tokens1[0],miutil::to_lower("roadobs.")))
            ofp.fileformat = roadobs;
          else if (miutil::contains(tokens1[0],miutil::to_lower("metnoobs."))
              || miutil::contains(tokens1[0],miutil::to_lower("obs.")))
            ofp.fileformat = metnoobs;
          else if (miutil::contains(tokens1[0],miutil::to_lower("bufr.")))
            ofp.fileformat = bufr;
          else
            continue;
          if (miutil::contains(tokens1[0],miutil::to_lower(".temp"))) {
            ofp.obstype = temp;
          } else if (miutil::contains(tokens1[0],miutil::to_lower(".amdar"))) {
            ofp.obstype = amdar;
          } else if (miutil::contains(tokens1[0],miutil::to_lower(".pilot"))) {
            ofp.obstype = pilot;
          } else {
            continue;
          }
          ofp.tf.initFilter(tokens1[1]);
          ofp.filepath = tokens1[1];
          ofp.parameterfile = tokens2[1];
          ofp.stationfile = tokens3[1];
          ofp.databasefile = tokens4[1];
          filePaths.push_back(ofp);

        }
      }
#endif
	  else {
        std::string filetype="standard",fileformat,fileconfig;
        for ( size_t j=0; j<tokens.size(); ++j) {
          tokens1= miutil::split(tokens[j], "=");
          if ( tokens1.size() != 2 ) continue;
          if ( tokens1[0] == miutil::to_lower("m") ) {
            model = tokens1[1];
          } else if( tokens1[0] == miutil::to_lower("f") ) {
            filename = tokens1[1];
          } else if( tokens1[0] == miutil::to_lower("s") ) {
            filename = tokens1[1];
          } else if( tokens1[0] == miutil::to_lower("t") ) {
            filetype = tokens1[1];
          }
        }
        filenames[model]= filename;
        filetypes[filename] = filetype;
        filesetup[filename] = vstr[i];
        uniquefiles.insert(filename);
        dialogModelNames.push_back(model);
        dialogFileNames.push_back(filename);
      }


    }

    vstr.clear();
#ifdef DEBUGPRINT_FILES
    int l= filePaths.size();
    for (int i=0; i<l; i++) {
      METLIBS_LOG_DEBUG("index: " << i);
      printObsFilePath(filePaths[i]);
    }
#endif

  } else {

    METLIBS_LOG_ERROR("Missing section " << section2 << " in setupfile.");

  }

  amdarStationFile= LocalSetupParser::basicValue("amdarstationlist");

}


void VprofManager::updateObsFileList()
{

  METLIBS_LOG_DEBUG("VprofManager::updateObsFileList");

  obsfiles.clear();
  int n= filePaths.size();
  glob_t globBuf;
  for (int j=0; j<n; j++) {
#ifdef ROADOBS
    if (filePaths[j].fileformat == roadobs)
    {
      // Due to the fact that we have a database insteda of an archive,
      // we maust fake the behavoir of anarchive
      // Assume that all stations report every hour
      // firt, get the current time.
      //
      // We assume that temograms are made 00Z, 06Z, 12Z and 18Z.
      miTime now = miTime::nowTime();
      miClock nowClock = now.clock();
      miDate nowDate = now.date();
      nowClock.setClock(nowClock.hour(),0,0);
      now.setTime(nowDate, nowClock);
      /* TBD: read from setup file */
      int daysback = 4;
      miTime starttime = now;
      if (now.hour()%6 != 0)
      {
        /* Adjust start hour */
        switch (now.hour())
        {
        case 1:
        case 7:
        case 13:
        case 19:
          now.addHour(-1);
          break;
        case 2:
        case 8:
        case 14:
        case 20:
          now.addHour(-2);
          break;
        case 3:
        case 9:
        case 15:
        case 21:
          now.addHour(-3);
          break;
        case 4:
        case 10:
        case 16:
        case 22:
          now.addHour(-4);
          break;
        case 5:
        case 11:
        case 17:
        case 23:
          now.addHour(-5);
          break;
        }
      }
      starttime.addDay(-daysback);
      int hourdiff;
      miTime time = now;
      /* init done, now loop in time */
      ObsFile of;
      of.obstype = filePaths[j].obstype;
      of.fileformat= filePaths[j].fileformat;
      of.parameterfile= filePaths[j].parameterfile;
      of.stationfile= filePaths[j].stationfile;
      of.databasefile= filePaths[j].databasefile;
      of.time = time;
      of.modificationTime= ::time(NULL);
      if (filePaths[j].obstype == temp)
        of.filename = "ROADOBSTEMP_" + time.isoDate() + "_" + time.isoClock(true, true);
      else if (filePaths[j].obstype == pilot)
        of.filename = "ROADOBSPILOT_" + time.isoDate() + "_" + time.isoClock(true, true);
      else if (filePaths[j].obstype == amdar)
        of.filename = "ROADOBSAMDAR_" + time.isoDate() + "_" + time.isoClock(true, true);
      obsfiles.push_back(of);
      time.addHour(-6);
      while ((hourdiff = miTime::hourDiff(time, starttime)) > 0) {
        of.obstype = filePaths[j].obstype;
        of.fileformat= filePaths[j].fileformat;
        of.parameterfile= filePaths[j].parameterfile;
        of.stationfile= filePaths[j].stationfile;
        of.databasefile= filePaths[j].databasefile;
        of.time = time;
        of.modificationTime= ::time(NULL);
        if (filePaths[j].obstype == temp)
          of.filename = "ROADOBSTEMP_" + time.isoDate() + "_" + time.isoClock(true, true);
        else if (filePaths[j].obstype == pilot)
          of.filename = "ROADOBSPILOT_" + time.isoDate() + "_" + time.isoClock(true, true);
        else if (filePaths[j].obstype == amdar)
          of.filename = "ROADOBSAMDAR_" + time.isoDate() + "_" + time.isoClock(true, true);
        obsfiles.push_back(of);
        time.addHour(-6);
      } 

    }
    else 
    {
      /* Use glob for ordinary files */
      glob(filePaths[j].filepath.c_str(),0,0,&globBuf);
    }
#else 
    /* Use glob for ordinary files */ 
    glob(filePaths[j].filepath.c_str(), 0, 0, &globBuf); 
#endif 


    ObsFile of;
    of.obstype   = filePaths[j].obstype;
    of.fileformat= filePaths[j].fileformat;
    if(of.fileformat == bufr){
#ifdef BUFROBS
      of.modificationTime= -1; //no need to check later
      for (size_t i=0; i<globBuf.gl_pathc; i++) {
        of.filename= std::string(globBuf.gl_pathv[i]);
        if(!filePaths[j].tf.ok() ||
            !filePaths[j].tf.getTime(of.filename,of.time)){
          ObsBufr bufr;
          if(!bufr.ObsTime(of.filename,of.time)) continue;
        }
        //time found, no need to check again
        obsfiles.push_back(of);
      }
#endif
    }
    globfree_cache(&globBuf);
  }
#ifdef DEBUGPRINT_FILES
  int l= obsfiles.size();
  for (int i=0; i<l; i++) {
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFiles(obsfiles[i]);
  }
#endif
}


void VprofManager::setPlotWindow(int w, int h)
{

  METLIBS_LOG_DEBUG("VprofManager::setPlotWindow:" << w << " " << h);

  plotw= w;
  ploth= h;
  if (vpdiag) vpdiag->setPlotWindow(plotw,ploth);
}


//*********************** end routines from controller ***********************

void VprofManager::setModel()
{

  METLIBS_LOG_DEBUG("VprofManager::setModel");


  // should not clear all data, possibly needed again...
  cleanup();

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
  set<string>::iterator p = usemodels.begin();
  for (; p!=usemodels.end(); p++) {
    std::string model= *p;
    map<std::string,std::string>::iterator pf;
    pf= filenames.find(model);
    if (pf==filenames.end()) {
      METLIBS_LOG_ERROR("NO VPROFDATA for model " << model);
    } else
      initVprofData(pf->second,model);
  }

  //define models/files  when "file" chosen in modeldialog
  vector<string>::iterator q = selectedFiles.begin();
  for (; q!=selectedFiles.end(); q++) {
    std::string file= *q;
    if (miutil::contains(file, menuConst["OBSTEMP"]))
      showObs= showObsTemp= true;
    else if (miutil::contains(file, menuConst["OBSPILOT"]))
      showObs= showObsPilot= true;
    else if (miutil::contains(file, menuConst["OBSAMDAR"]))
      showObs= showObsAmdar= true;
    else {
      map<std::string,std::string>::iterator pf=filenames.begin();
      for (; pf!=filenames.end(); pf++) {
        if (file==pf->second){
          initVprofData(file,pf->first);
          break;
        }
      }
    }
  }

  onlyObs= false;

  if (showObs && vpdata.size()==0) {
    // until anything decided:
    // check observation time and display the most recent file
    checkObsTime();
    onlyObs= true;
  }

  initTimes();
  initStations();

  if (vpdiag) {
    int nobs= (showObs) ? 1 : 0;
    int nmod= vpdata.size();
    if (nobs+nmod==0) nobs= 1;
    vpdiag->changeNumber(nobs,nmod);
  }


  METLIBS_LOG_DEBUG("VprofManager::setModels finished");

}


void VprofManager::setStation(const std::string& station)
{

  METLIBS_LOG_DEBUG("VprofManager::setStation  " << station);


  plotStation= station;
}


void VprofManager::setTime(const miTime& time)
{

  METLIBS_LOG_DEBUG("VprofManager::setTime  " << time);


  plotTime= time;

  if (onlyObs)
    initStations();
}


std::string VprofManager::setStation(int step)
{

  METLIBS_LOG_DEBUG("VprofManager::setStation   step=" << step);


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

  plotStation= nameList[i];
  return plotStation;
}


miTime VprofManager::setTime(int step)
{

  METLIBS_LOG_DEBUG("VprofManager::setTime   step=" << step);


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

  return plotTime;
}

// start hardcopy
void VprofManager::startHardcopy(const printOptions& po){
  if (hardcopy && hardcopystarted && vpdiag){
    // if hardcopy in progress and same filename: make new page
    if (po.fname == printoptions.fname){
      vpdiag->startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    vpdiag->endPSoutput();
  }
  hardcopy= true;
  printoptions= po;
  hardcopystarted= false;
}

// end hardcopy plot
void VprofManager::endHardcopy(){
  // postscript output
  if (hardcopy && vpdiag) vpdiag->endPSoutput();
  hardcopy= false;
}


bool VprofManager::plot()
{
  METLIBS_LOG_SCOPE(LOGVAL(plotStation) << LOGVAL(plotTime));

  if (!vpdiag) {
    vpdiag= new VprofDiagram(vpopt);
    vpdiag->setPlotWindow(plotw,ploth);
    int nobs= (showObs) ? 1 : 0;
    int nmod= vpdata.size();
    if (nobs+nmod==0)
      nobs= 1;
    vpdiag->changeNumber(nobs,nmod);
  }

  // postscript output
  if (hardcopy && !hardcopystarted) {
    vpdiag->startPSoutput(printoptions);
    hardcopystarted= true;
  }

  vpdiag->plot();

  if (not plotStation.empty()) {

    for (size_t i=0; i<vpdata.size(); i++) {
      std::auto_ptr<VprofPlot> vp(vpdata[i]->getData(plotStation,plotTime));
      if (vp.get())
        vp->plot(vpopt, i);
    }

    if (showObs) {
      int n= nameList.size();
      int i= 0;
      while (i<n && nameList[i]!=plotStation)
        i++;

      if (i<n && not obsList[i].empty()) {
        checkObsTime(plotTime.hour());

        vector<std::string> stationList;
        stationList.push_back(obsList[i]);
        int nf= obsfiles.size();
        int nn= 0;
        VprofPlot *vp= 0;

        while (vp==0 && nn<nf) {
          if (obsfiles[nn].modificationTime &&
              obsfiles[nn].time==plotTime) {
            if(obsfiles[nn].fileformat==bufr &&
                ((not miutil::contains(nameList[i], "Pilot") && obsfiles[nn].obstype!=pilot) ||
                    (miutil::contains(nameList[i], "Pilot") && obsfiles[nn].obstype==pilot)) ) {
#ifdef BUFROBS
              ObsBufr bufr;
              vector<std::string> vprofFiles;
              //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
              vprofFiles.push_back(obsfiles[nn].filename);
              if (nn>0  && abs(miTime::minDiff(obsfiles[nn-1].time, plotTime)) <= 60 ) {
                vprofFiles.push_back(obsfiles[nn-1].filename);
              }
              std::string modelName;
              if ( obsfiles[nn].obstype==amdar ) {
                modelName="AMDAR";
              } else if (obsfiles[nn].obstype == temp ) {
                modelName="TEMP";
              } else if (obsfiles[nn].obstype == pilot ) {
                modelName="PILOT";
              }
              vp=bufr.getVprofPlot(vprofFiles, modelName, obsList[i], plotTime);
#endif
            }
#ifdef ROADOBS
            /* NOTE! If metoobs are used, all data are fetched when constructing, for example, the VprofTemp object. */
            /* If we fetch data from road, we should fetch data for one station, obsList[i],plotTime, to improve performance */
            /*   The VprofRTemp class should be implemented, to simplify code */
            else if (obsfiles[nn].fileformat==roadobs) {
              try {
                if (showObsTemp && obsfiles[nn].obstype==temp &&
                    !miutil::contains(nameList[i], "Pilot")) {
                  //land or ship wmo station with name
                  VprofRTemp vpobs(obsfiles[nn].parameterfile,false,stationList,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                  vp= vpobs.getStation(obsList[i],plotTime);
                } else if (showObsAmdar && obsfiles[nn].obstype==amdar &&
                    !miutil::contains(nameList[i], "Pilot")) {
                  VprofRTemp vpobs(obsfiles[nn].parameterfile,true,
                      latitudeList[i],longitudeList[i],0.3f,0.3f,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                  vp= vpobs.getStation(obsList[i],plotTime);
                  if (vp!=0) vp->setName(nameList[i]);
                }
              }
              catch (...) {
                METLIBS_LOG_ERROR("Exception in: " <<obsfiles[nn].filename);
              }
            }
#endif

          }
          nn++;
        }
        if (vp) {
          vp->plot(vpopt, vpdata.size());
          delete vp;
        }
      }
    }

    vpdiag->plotText();
  }


  METLIBS_LOG_DEBUG("VprofManager::plot finished");

  return true;
}


/***************************************************************************/

vector <std::string> VprofManager::getModelNames(){

  METLIBS_LOG_DEBUG("VprofManager::getModelNames");

  updateObsFileList();
  return dialogModelNames;
}

/***************************************************************************/

vector <std::string> VprofManager::getModelFiles(){

  METLIBS_LOG_DEBUG("VprofManager::getModelFiles");

  vector<std::string> modelfiles= dialogFileNames;
  updateObsFileList();
  int n= obsfiles.size();
  for (int i=0; i<n; i++) {
#ifdef DEBUGPRINT_FILES
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFiles(obsfiles[i]);
#endif
    modelfiles.push_back(obsfiles[i].filename);
  }
  return modelfiles;
}


/***************************************************************************/

void VprofManager::setFieldModels(const vector<string>& fieldmodels)
{
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}


/***************************************************************************/

void VprofManager::setSelectedModels(const vector <string>& models,
    bool field, bool obsTemp,
    bool obsPilot, bool obsAmdar)
{
  //called when model selected in model dialog
  showObsTemp = obsTemp;
  showObsPilot= obsPilot;
  showObsAmdar= obsAmdar;
  showObs= (obsTemp || obsPilot || obsAmdar);
  asField = field;
  //set data from models, not files
  selectedFiles.clear();
  selectedModels = models;
}


/***************************************************************************/

void VprofManager::setSelectedFiles(const vector<string>& files,
    bool field, bool obsTemp,
    bool obsPilot, bool obsAmdar)
{
  //called when model selected in model dialog
  showObsTemp = obsTemp;
  showObsPilot= obsPilot;
  showObsAmdar= obsAmdar;
  showObs= (obsTemp || obsPilot || obsAmdar);
  asField = field;
  //set data from files not models
  selectedModels.clear();
  selectedFiles = files;
}


/***************************************************************************/

std::string VprofManager::getDefaultModel()
{
  //for now, just the first model in filenames list
  map<std::string,std::string>::iterator p = filenames.begin();
  std::string model = p->first;
  return model;
}


/***************************************************************************/
vector<string> VprofManager::getSelectedModels()
{
  vector<string> models = selectedModels;
  if (showObsTemp)  models.push_back(menuConst["OBSTEMP"]);
  if (showObsPilot) models.push_back(menuConst["OBSPILOT"]);
  if (showObsAmdar) models.push_back(menuConst["OBSAMDAR"]);
  if (asField)      models.push_back(menuConst["ASFIELD"]);
  return models;
}


/***************************************************************************/

bool VprofManager::initVprofData(std::string file, std::string model)
{
  METLIBS_LOG_SCOPE();

  std::auto_ptr<VprofData> vpd(new VprofData(file, model));
  const std::string filetype = filetypes[file];
  bool ok = false;
  if (filetype == "standard") {
    ok = vpd->readFile();
  } else if (filetype == "GribFile") {
    ok = vpd->readField(filetypes[file], fieldm);
  } else if (filetypes[file] == "netcdf") {
    ok = vpd->readFimex(filesetup[file]);
  } else {
    METLIBS_LOG_ERROR("VPROFDATA READ ERROR file '" << file << "' model '"
        << model << "' has unknown filetype '" << filetype << "'");
    return false;
  }
  if (ok) {
    METLIBS_LOG_INFO("VPROFDATA READ OK for model '" << model << "' filetype '"
        << filetype << "'");
    vpdata.push_back(vpd.release());
  } else {
    METLIBS_LOG_ERROR("VPROFDATA READ ERROR file '" << file << "' model '"
        << model << "' filetype '" << filetype << "'");
  }
  return ok;
}

/***************************************************************************/

void VprofManager::initStations(){
  //merge lists from all models
  int nvpdata = vpdata.size();

  METLIBS_LOG_DEBUG("VprofManager::initStations-size of vpdata " << nvpdata);


  nameList.clear();
  latitudeList.clear();
  longitudeList.clear();
  obsList.clear();

  vector <std::string> namelist;
  vector <float>    latitudelist;
  vector <float>    longitudelist;
  vector <std::string> obslist;
  vector<miTime> tlist;

  for (int i = 0;i<nvpdata;i++){
    namelist= vpdata[i]->getNames();
    latitudelist= vpdata[i]->getLatitudes();
    longitudelist= vpdata[i]->getLongitudes();
    obslist= vpdata[i]->getObsNames();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size()||n!=longitudelist.size()||
        n!=obslist.size()) {
      METLIBS_LOG_ERROR("diVprofManager::initStations - SOMETHING WRONG WITH STATIONLIST!");
    } else if (n>0) {
      // check for duplicates
      // name should be used as to check
      // all lists must be equal in size
      if (namelist.size() == obslist.size() && namelist.size() == latitudelist.size() && namelist.size() == longitudelist.size())
      {
        for (size_t index = 0; index < namelist.size(); index++)
        {
          bool found = false;
          for (size_t j = 0; j < nameList.size(); j++)
          {
            if (nameList[j] == namelist[index])
            {
              // stop searching
              found = true;
              break;
            }
          }
          if (!found)
          {
            nameList.push_back(namelist[index]);
            obsList.push_back(obslist[index]);
            latitudeList.push_back(latitudelist[index]);
            longitudeList.push_back(longitudelist[index]);
          }
        }
      }
      else
      {
        // This should not happen.....
        nameList.insert(nameList.end(),namelist.begin(),namelist.end());
        obsList.insert(obsList.end(),obslist.begin(),obslist.end());
        latitudeList.insert(latitudeList.end(),latitudelist.begin(),latitudelist.end());
        longitudeList.insert(longitudeList.end(),longitudelist.begin(),longitudelist.end());
      }

    }
  }

  // using current time until..................
  map<std::string,int> amdarCount;
  int n= obsfiles.size();
  for (int i=0; i<n; i++) {
    if (obsfiles[i].time==plotTime &&
        ((showObsTemp  && obsfiles[i].obstype==temp) ||
            (showObsPilot && obsfiles[i].obstype==pilot) ||
            (showObsAmdar && obsfiles[i].obstype==amdar))) {
      if(obsfiles[i].fileformat==bufr){
#ifdef BUFROBS
        ObsBufr bufr;
        vector<std::string> vprofFiles;
        vprofFiles.push_back(obsfiles[i].filename);
        //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
        if (i>0  && abs(miTime::minDiff(obsfiles[i-1].time, plotTime)) <= 60 ) {
          vprofFiles.push_back(obsfiles[i-1].filename);
        }
        bufr.readStationInfo(vprofFiles,
            namelist, tlist, latitudelist, longitudelist);
#endif
#ifdef ROADOBS
      } else if (obsfiles[i].fileformat==roadobs) {
        // TDB: Construct stationlist from temp, pilot or amdar stationlist
        if ((obsfiles[i].obstype==temp )||(obsfiles[i].obstype==pilot )||(obsfiles[i].obstype==amdar ))
        {
          // TBD!
          // This creates the stationlist
          // we must also intit the connect string to mora
          Roaddata::initRoaddata(obsfiles[i].databasefile);
          diStation::initStations(obsfiles[i].stationfile);
          // get the pointer to the actual station vector
          vector<diStation> * stations = NULL;
          map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(obsfiles[i].stationfile);
          if (its != diStation::station_map.end())
          {
            stations = its->second;
          }
          if (stations == NULL)
          {
            METLIBS_LOG_ERROR("Unable to find stationlist: " <<obsfiles[i].stationfile);
          }
          else
          {
            int noOfStations = stations->size();
            namelist.clear();
            latitudelist.clear();
            longitudelist.clear();
            obslist.clear();
            for (int i = 0; i < noOfStations; i++)
            {
              namelist.push_back((*stations)[i].name());
              latitudelist.push_back((*stations)[i].lat());
              longitudelist.push_back((*stations)[i].lon());
              // convert from wmonr to string
              char statid[10];
              sprintf(statid, "%i", (*stations)[i].stationID());
              obslist.push_back(statid);
            }
          }
        }
      }
#else
    }
#endif

#ifdef ROADOBS
    // Backwards compatibility with met.no observation files....
    if (!obsfiles[i].fileformat==roadobs)
    {
      obslist= namelist;
    }
#else
    obslist= namelist;
#endif
    unsigned int ns= namelist.size();
    if (ns!=latitudelist.size() || ns!=longitudelist.size() ||
        ns!=obslist.size()) {
      METLIBS_LOG_ERROR("diVprofManager::initStations - SOMETHING WRONG WITH OBS.STATIONLIST!");
    } else if (ns>0) {
      for (unsigned int j=0; j<ns; j++) {
        if (namelist[j].substr(0,2)=="99") {
#ifdef linux
          // until robs (linux swap problem) fixed
          // (note that "obslist" is kept unchanged/wrong for reading)
          std::string callsign=  namelist[j].substr(3,1)
								        + namelist[j].substr(2,1)
								        + namelist[j].substr(5,1)
								        + namelist[j].substr(4,1);
          namelist[j]= callsign;
#else
          namelist[j]= namelist[j].substr(2,namelist[j].length()-2);
#endif
        }
      }
      if (obsfiles[i].obstype==pilot) {
        // may have the same station as both Pilot and Temp
        for (unsigned int j=0; j<ns; j++)
          namelist[j]= "Pilot:" + namelist[j];
      } else if (obsfiles[i].obstype==amdar) {
        renameAmdar(namelist,latitudelist,longitudelist,obslist,tlist,amdarCount);
      }
      // check for duplicates
      // name should be used as to check
      // all lists must be equal in size
      if (namelist.size() == obslist.size() && namelist.size() == latitudelist.size() && namelist.size() == longitudelist.size())
      {
        for (size_t index = 0; index < namelist.size(); index++)
        {
          bool found = false;
          for (size_t j = 0; j < nameList.size(); j++)
          {
            if (nameList[j] == namelist[index])
            {
              // stop searching
              found = true;
              break;
            }
          }
          if (!found)
          {
            nameList.push_back(namelist[index]);
            obsList.push_back(obslist[index]);
            latitudeList.push_back(latitudelist[index]);
            longitudeList.push_back(longitudelist[index]);
          }
        }
      }
      else
      {
        // This should not happen.....
        nameList.insert(nameList.end(),namelist.begin(),namelist.end());
        obsList.insert(obsList.end(),obslist.begin(),obslist.end());
        latitudeList.insert(latitudeList.end(),latitudelist.begin(),latitudelist.end());
        longitudeList.insert(longitudeList.end(),longitudelist.begin(),longitudelist.end());
      }
    }
  }
}

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
}

METLIBS_LOG_DEBUG("plotStation" << plotStation);

}


/***************************************************************************/

void VprofManager::initTimes(){

  METLIBS_LOG_DEBUG("VprofManager::initTimes:" << plotTime.isoTime());


  timeList.clear();

  //assume common times...
  if (vpdata.size()) timeList= vpdata[0]->getTimes();

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

/***************************************************************************/

void VprofManager::checkObsTime(int hour) {

  METLIBS_LOG_DEBUG("VprofManager::checkObsTime  hour= " << hour);


  // use hour=-1 to check all files
  // hour otherwise used to spread checking of many files (with plotTime.hour)

  if (hour>23) hour=-1;
  bool newtime= !obsTime.size();
  int n= obsfiles.size();

  pu_struct_stat statbuf;

  for (int i=0; i<n; i++) {
#ifdef DEBUGPRINT_FILES
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFiles(obsfiles[i]);
#endif
    if (obsfiles[i].modificationTime<0)
      continue; //no need to check
#ifdef ROADOBS
    if (obsfiles[i].fileformat == roadobs)
    {
	  // Set to nowtime just in case.
	  obsfiles[i].modificationTime = time(NULL);
      newtime= true;
    }
#endif
  }
  /* TDB: is this correct for observations from ROAD also ? */
  /* No, always construct a new list */
#ifdef ROADOBS
  if (newtime) {
#else
    if (newtime && hour<0) {
#endif
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


  void VprofManager::mainWindowTimeChanged(const miTime& time)
  {

    METLIBS_LOG_DEBUG("VprofManager::mainWindowTimeChanged  " << time);


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


  void VprofManager::updateObs()
  {

    METLIBS_LOG_DEBUG("VprofManager::updateObs");

    updateObsFileList();
    checkObsTime();
  }


  std::string VprofManager::getAnnotationString(){
    std::string str = std::string("Vertikalprofiler ");
    if (onlyObs)
      str += plotTime.isoTime();
    else
      for (set <string>::iterator p=usemodels.begin();p!=usemodels.end();p++)
        str+=*p+std::string(" ");
    return str;
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


  void VprofManager::renameAmdar(vector<std::string>& namelist,
      vector<float>& latitudelist,
      vector<float>& longitudelist,
      vector<std::string>& obslist,
      vector<miTime>& tlist,
      map<std::string,int>& amdarCount)
  {
    //should not happen, but ...
    if(namelist.size()!=tlist.size()) return;

    if (!amdarStationList) readAmdarStationList();

    int n=namelist.size();
    int m= amdarName.size();
    int jmin,c;
    float smin,dx,dy,s;
    std::string newname;

    multimap<std::string,int> sortlist;

    for (int i=0; i<n; i++) {
      jmin=-1;
      smin=0.05*0.05 + 0.05*0.05;
      for (int j=0; j<m; j++) {
        dx=longitudelist[i]-amdarLongitude[j];
        dy= latitudelist[i]-amdarLatitude[j];
        s=dx*dx+dy*dy;
        if (s<smin) {
          smin=s;
          jmin=j;
        }
      }
      if (jmin>=0) {
        newname= amdarName[jmin];
      } else {
        std::string slat= miutil::from_number(fabsf(latitudelist[i]));
        if (latitudelist[i]>=0.) slat += "N";
        else                     slat += "S";
        std::string slon= miutil::from_number(fabsf(longitudelist[i]));
        if (longitudelist[i]>=0.) slon += "E";
        else                      slon += "W";
        newname= slat + "," + slon;
        jmin= m;
      }

      ostringstream ostr;
      ostr<<setw(4)<<setfill('0')<<jmin;
      std::string sortname= ostr.str() + newname + tlist[i].isoTime() + namelist[i];
      sortlist.insert(pair<std::string,int>(sortname,i));

      namelist[i]= newname;
    }

    map<std::string,int>::iterator p;
    multimap<std::string,int>::iterator pt, ptend= sortlist.end();

    // gather amdars from same stations (in station list sequence)
    vector<std::string> namelist2;
    vector<float>    latitudelist2;
    vector<float>    longitudelist2;
    vector<std::string> obslist2;

    for (pt=sortlist.begin(); pt!=ptend; pt++) {
      int i= pt->second;

      newname= namelist[i];
      p= amdarCount.find(newname);
      if (p==amdarCount.end())
        amdarCount[newname]= c= 1;
      else
        c= ++(p->second);
      newname+= " (" + miutil::from_number(c) + ")";

      namelist2.push_back(newname);
      latitudelist2.push_back(latitudelist[i]);
      longitudelist2.push_back(longitudelist[i]);
      obslist2.push_back(obslist[i]);
    }

    namelist=      namelist2;
    latitudelist=  latitudelist2;
    longitudelist= longitudelist2;
    obslist=       obslist2;
  }


  void VprofManager::readAmdarStationList()
  {
    amdarStationList= true;

    if (amdarStationFile.empty()) return;

    // open filestream
    ifstream file;
    file.open(amdarStationFile.c_str());
    if (file.bad()) {
      METLIBS_LOG_ERROR("Amdar station list  "<<amdarStationFile<<"  not found");
      return;
    }

    const float notFound=9999.;
    vector<std::string> vstr,vstr2;
    std::string str;
    unsigned int i;

    while (getline(file,str)) {
      std::string::size_type n= str.find('#');
      if (n!=0) {
        if (n!=string::npos) str= str.substr(0,n);
        miutil::trim(str);
        if (not str.empty()) {
          vstr= miutil::split_protected(str, '"', '"');
          float latitude=notFound, longitude=notFound;
          std::string name;
          n=vstr.size();
          for (i=0; i<n; i++) {
            vstr2= miutil::split(vstr[i], "=");
            if (vstr2.size()==2) {
              str= miutil::to_lower(vstr2[0]);
              if (str=="latitude")
                latitude= atof(vstr2[1].c_str());
              else if (str=="longitude")
                longitude= atof(vstr2[1].c_str());
              else if (str=="name") {
                name= vstr2[1];
                if (name[0]=='"')
                  name= name.substr(1,name.length()-2);
              }
            }
          }
          if (latitude!=notFound && longitude!=notFound && not name.empty()) {
            amdarLatitude.push_back(latitude);
            amdarLongitude.push_back(longitude);
            amdarName.push_back(name);
          }
        }
      }
    }

    file.close();

    return;
  }

  void VprofManager::printObsFiles(const ObsFile &of)
  {
    /*
     struct ObsFile { 
     std::string   filename; 
     obsType    obstype; 
     FileFormat fileformat; 
     miTime     time; 
     long       modificationTime; 
     }; 
     */
    METLIBS_LOG_INFO("ObsFile: < ");
    METLIBS_LOG_INFO("filename: " << of.filename);
    METLIBS_LOG_INFO("obsType: " << of.obstype);
    METLIBS_LOG_INFO("FileFormat: " << of.fileformat);
    METLIBS_LOG_INFO("Time: " << of.time.isoTime(true, true));
    METLIBS_LOG_INFO("ModificationTime: " << of.modificationTime);
#ifdef ROADOBS 
    METLIBS_LOG_INFO("Parameterfile: " << of.parameterfile);
    METLIBS_LOG_INFO("Stationfile: " << of.stationfile);
    METLIBS_LOG_INFO("Databasefile: " << of.databasefile);
#endif 
    METLIBS_LOG_INFO(">");
  }

  void VprofManager::printObsFilePath(const ObsFilePath & ofp)
  {
    /*
     struct ObsFilePath { 
     std::string   filepath; 
     obsType    obstype; 
     FileFormat fileformat; 
     TimeFilter tf; 
     }; 
     */
    METLIBS_LOG_INFO("ObsFilePath: < ");
    METLIBS_LOG_INFO("filepath: " << ofp.filepath);
    METLIBS_LOG_INFO("obsType: " << ofp.obstype);
    METLIBS_LOG_INFO("FileFormat: " << ofp.fileformat);
#ifdef ROADOBS 
    METLIBS_LOG_INFO("Parameterfile: " << ofp.parameterfile);
    METLIBS_LOG_INFO("Stationfile: " << ofp.stationfile);
    METLIBS_LOG_INFO("Databasefile: " << ofp.databasefile);
#endif 
    METLIBS_LOG_INFO(">");
  }
