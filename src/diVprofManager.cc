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
#include "vcross_v2/VcrossSetup.h"
#include <diField/VcrossUtil.h>

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

#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>
#include <puTools/mi_boost_compatibility.hh>

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

VprofManager::VprofManager()
: amdarStationList(false), vpdiag(0), showObs(false),
  showObsTemp(false), showObsPilot(false), showObsAmdar(false),
  plotw(0), ploth(0), hardcopy(false)
{
  METLIBS_LOG_SCOPE();

  vpopt= new VprofOptions();  // defaults are set

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
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
  // clean up vpdata...
  for (unsigned int i=0; i<vpdata.size(); i++)
    delete vpdata[i];
  vpdata.clear();
  // NOTE: Flush the field cache
}

void VprofManager::init()
{
  parseSetup();
  updateObsFileList();
}

void VprofManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  filenames.clear();
  stationsfilenames.clear();
  filetypes.clear();
  dialogModelNames.clear();
  dialogFileNames.clear();
  filePaths.clear();

  const std::string section2 = "VERTICAL_PROFILE_FILES";
  vector<std::string> vstr;

  if (miutil::SetupParser::getSection(section2,vstr)) {

    std::string model, filename, stationsfilename;
    vector<std::string> tokens,tokens1,tokens2, tokens3, tokens4;
    int n= vstr.size();
    vector<std::string> sources;

    for (int i=0; i<n; i++) {
      tokens= miutil::split(vstr[i]);
      if (tokens.size()==1) {
        tokens1= miutil::split(tokens[0], "=");
        if (tokens1.size()==2) {
          std::string tokens1_0_lc = miutil::to_lower(tokens1[0]);
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
          miutil::replace(tokens1_0_lc,"bufr","obs");
          dialogModelNames.push_back(tokens1_0_lc);
          dialogFileNames.push_back(tokens1[1]);
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
            stationsfilename = tokens1[1];
          } else if( tokens1[0] == miutil::to_lower("t") ) {
            filetype = tokens1[1];
          }
        }
        if ( filetype !="standard" ) {
          sources.push_back(vstr[i]);
        }
//          stationsfilenames[model]= stationsfilename;
//          filetypes[model] = filetype;
//        } else {
          filenames[model]= filename;
          stationsfilenames[model]= stationsfilename;
          filetypes[model] = filetype;
          dialogModelNames.push_back(model);
          dialogFileNames.push_back(filename);

        //}
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


  amdarStationFile= LocalSetupParser::basicValue("amdarstationlist");

  miutil::SetupParser::getSection("VERTICAL_PROFILE_COMPUTATIONS", computations);
  setup = miutil::make_shared<vcross::Setup>();
  setup->configureSources(sources);
//  vcross::string_v models = setup->getAllModelNames();
//  for ( size_t i=0; i<models.size() ;i++ ) {
//    METLIBS_LOG_INFO(LOGVAL(models[i]));
//    filenames[models[i]]= models[i];
//    dialogModelNames.push_back(models[i]);
//    dialogFileNames.push_back(models[i]);
//  }

  setup->configureComputations(computations);

  } else {
    METLIBS_LOG_ERROR("Missing section " << section2 << " in setupfile.");
  }
}

void VprofManager::updateObsFileList()
{
  METLIBS_LOG_SCOPE();

  obsfiles.clear();
  int n= filePaths.size();
  diutil::string_v matches;
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
      matches = diutil::glob(filePaths[j].filepath);
    }
#else
    /* Use glob for ordinary files */
    matches = diutil::glob(filePaths[j].filepath);
#endif

    ObsFile of;
    of.obstype   = filePaths[j].obstype;
    of.fileformat= filePaths[j].fileformat;
    if(of.fileformat == bufr){
#ifdef BUFROBS
      of.modificationTime= -1; //no need to check later
      for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
        of.filename= *it;
        if (!filePaths[j].tf.ok()
            || !filePaths[j].tf.getTime(of.filename,of.time))
        {
          ObsBufr bufr;
          if (!bufr.ObsTime(of.filename,of.time))
            continue;
        }
        //time found, no need to check again
        obsfiles.push_back(of);
      }
#endif
    }
  }
#ifdef DEBUGPRINT_FILES
  for (size_t i=0; i<obsfiles.size(); i++) {
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFiles(obsfiles[i]);
  }
#endif
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
  METLIBS_LOG_SCOPE();

  // should not clear all data, possibly needed again...
  cleanup();

  //models from model dialog
  int m= selectedModels.size();
  for (int i=0;i<m;i++) {
    if ( !miutil::contains(selectedModels[i].model,"obs.") )
      initVprofData(selectedModels[i]);
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
}


void VprofManager::setStation(const std::string& station)
{
  METLIBS_LOG_SCOPE(station);
  plotStations.clear();
  plotStations.push_back(station);
}

void VprofManager::setStations(const std::vector<std::string>& stations)
{
  METLIBS_LOG_SCOPE(stations.size());
  if ( stations.size() )
    plotStations = stations;
}

void VprofManager::setTime(const miTime& time)
{
  METLIBS_LOG_SCOPE(time);

  plotTime= time;
  if (onlyObs)
    initStations();
}


std::string VprofManager::setStation(int step)
{
  METLIBS_LOG_SCOPE(LOGVAL(step));

  if (nameList.size()==0)
    return "";

  int i= 0;
  int n= nameList.size();
  if (plotStations.size()>0){
    while (i<n && nameList[i]!=plotStations[0]) i++;
  }
  if (i<n) {
    i+=step;
    if (i<0)  i= n-1;
    if (i>=n) i= 0;
  } else {
    i= 0;
  }

  plotStations.clear();
  plotStations.push_back(nameList[i]);
  return plotStations[0];
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
  METLIBS_LOG_SCOPE(LOGVAL(plotStations.size()) << LOGVAL(plotTime));
  selectedStations.clear();
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

  if ( plotStations.size() != 0 ) {

    for (size_t i=0; i<vpdata.size(); i++) {

      std::auto_ptr<VprofPlot> vp;

      for ( size_t j=0; j<plotStations.size(); ++j){
        vp.reset(vpdata[i]->getData(plotStations[j],plotTime));
        if (vp.get()) {
          selectedStations.push_back(plotStations[j]);
          break;
        }
      }
      if (vp.get()){
        vp->plot(vpopt, i);
      }
    }
    if (showObs) {
      // obsList corresponds to the obsList.size() first entries of nameList
      int n= obsList.size();
      int m= plotStations.size();
      int i=0;
      bool found = false;
      for ( i = 0; i<n; i++) {
        int j=0;
        while (j<m && nameList[i]!=plotStations[j])
          j++;
        if ( j < m ) {
          found = true;
          selectedStations.push_back(plotStations[j]);
          break;
        }
      }
      if ( found ) {
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

  return true;
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
  std::vector <std::string> rf;
  if ( miutil::contains(modelName,"obs") || filetypes[modelName] == "standard" )
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

/***************************************************************************/

void VprofManager::setFieldModels(const vector<string>& fieldmodels)
{
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}


/***************************************************************************/

void VprofManager::setSelectedModels(const vector <std::string>& models,
    bool obs)
{
  METLIBS_LOG_SCOPE();
  //called when model selected in model dialog

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

  showObsTemp = showObsPilot = showObsAmdar = obs;
  for ( size_t i=0; i<models.size(); ++i ) {
    if( selectedModels[i].model == "obs.temp" ) {
      showObsTemp = true;
    } else if( selectedModels[i].model == "obs.pilot" ) {
      showObsPilot= true;
    } else if( selectedModels[i].model == "obs.amdar" ) {
      showObsAmdar= true;
    }
  }
  showObs= (showObsTemp || showObsPilot || showObsAmdar );

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

bool VprofManager::initVprofData(const SelectedModel& selectedModel)
{
  METLIBS_LOG_SCOPE();
  std::string model_part=selectedModel.model.substr(0,selectedModel.model.find("@"));
  std::auto_ptr<VprofData> vpd(new VprofData( selectedModel.model, stationsfilenames[selectedModel.model]));
  bool ok = false;
  if ( filetypes[model_part] == "standard") {
    ok = vpd->readFile(filenames[selectedModel.model]);
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
  METLIBS_LOG_DEBUG("size of vpdata " << nvpdata);

  nameList.clear();
  latitudeList.clear();
  longitudeList.clear();
  obsList.clear();

  vector <std::string> namelist;
  vector <float>    latitudelist;
  vector <float>    longitudelist;
  vector <std::string> obslist;
  vector<miTime> tlist;

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
          namelist[j]= namelist[j].substr(2,namelist[j].length()-2);
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

for (int i = 0;i<nvpdata;i++){
  namelist= vpdata[i]->getNames();
  latitudelist= vpdata[i]->getLatitudes();
  longitudelist= vpdata[i]->getLongitudes();
  //    obslist= vpdata[i]->getObsNames();
  unsigned int n=namelist.size();
  if (n!=latitudelist.size()||n!=longitudelist.size()) {
    METLIBS_LOG_ERROR("diVprofManager::initStations - SOMETHING WRONG WITH STATIONLIST!");
  } else if (n>0) {
    // check for duplicates
    // name should be used as to check
    // all lists must be equal in size
    if (namelist.size() == latitudelist.size() && namelist.size() == longitudelist.size())
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
          //            obsList.push_back(obslist[index]);
          latitudeList.push_back(latitudelist[index]);
          longitudeList.push_back(longitudelist[index]);
        }
      }
    }
    else
    {
      // This should not happen.....
      nameList.insert(nameList.end(),namelist.begin(),namelist.end());
      //        obsList.insert(obsList.end(),obslist.begin(),obslist.end());
      latitudeList.insert(latitudeList.end(),latitudelist.begin(),latitudelist.end());
      longitudeList.insert(longitudeList.end(),longitudelist.begin(),longitudelist.end());
    }

  }
}

// remember station
if (plotStations.size() != 0) lastStation = plotStations[0];

METLIBS_LOG_DEBUG("lastStation"  << lastStation);

//if it's the first time, plot the first station
if (plotStations.empty() && nameList.size())
  plotStations.push_back(nameList[0]);

}


/***************************************************************************/

void VprofManager::initTimes()
{
  METLIBS_LOG_SCOPE(plotTime.isoTime());

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

void VprofManager::checkObsTime(int hour)
{
  METLIBS_LOG_SCOPE("hour= " << hour);

  // use hour=-1 to check all files
  // hour otherwise used to spread checking of many files (with plotTime.hour)

  if (hour>23) hour=-1;
  bool newtime= !obsTime.size();
  int n= obsfiles.size();

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
  if (newtime
#ifndef ROADOBS
      && hour<0
#endif
  )
  {
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

void VprofManager::updateObs()
{
  METLIBS_LOG_SCOPE();

  updateObsFileList();
  checkObsTime();
}

std::string VprofManager::getAnnotationString()
{
  std::string str = std::string("Vertical profiles ");
  if (onlyObs)
    str += plotTime.isoTime();
  else
    for (vector <SelectedModel>::iterator p=selectedModels.begin();p!=selectedModels.end();p++)
      str+=(*p).model+std::string(" ");
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

