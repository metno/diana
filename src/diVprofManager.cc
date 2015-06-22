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
#include <boost/foreach.hpp>

#include <cmath>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>

#define MILOGGER_CATEGORY "diana.VprofManager"
#include <miLogger/miLogging.h>

#ifdef ROADOBS
using namespace road;
#define IF_ROADOBS(x) x
#else
#define IF_ROADOBS(x)
#endif

using namespace std;
using miutil::miTime;

VprofManager::VprofManager()
: amdarStationList(false), vpdiag(0), showObs(false),
  showObsTemp(false), showObsPilot(false), showObsAmdar(false),
  plotw(0), ploth(0), mCanvas(0)
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
  diutil::delete_all_and_clear(vpdata);
  // TODO flush the field cache
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
  vector<std::string> vstr, sources;

  if (!miutil::SetupParser::getSection(section2,vstr)) {
    METLIBS_LOG_ERROR("Missing section " << section2 << " in setupfile.");
    return;
  }

  for (size_t i=0; i<vstr.size(); i++) {
    const std::vector<std::string> tokens = miutil::split(vstr[i]);
    if (tokens.size() == 1) {
      std::vector<std::string> tokens1 = miutil::split(tokens[0], "=");
      if (tokens1.size() != 2)
        continue;

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
      miutil::replace(tokens1_0_lc, "bufr", "obs");
      dialogModelNames.push_back(tokens1_0_lc);
      dialogFileNames.push_back(tokens1[1]);
    }
#ifdef ROADOBS
    /* Here we know that it is the extended obs format */
    else if (tokens.size()==4) {
      const std::vector<std::string> tokens1= miutil::split(tokens[0], "=");
      const std::vector<std::string> tokens2= miutil::split(tokens[1], "=");
      const std::vector<std::string> tokens3= miutil::split(tokens[2], "=");
      const std::vector<std::string> tokens4= miutil::split(tokens[3], "=");
      if (tokens1.size()==2 && tokens2.size()==2 && tokens3.size()==2 && tokens4.size()==2
          && miutil::to_lower(tokens2[0])=="p"
          && miutil::to_lower(tokens3[0])=="s"
          && miutil::to_lower(tokens4[0])=="d")
      {
        ObsFilePath ofp;
        const std::string tokens1_0_lc = miutil::to_lower(tokens1[0]);
        /* the check for roadobs must be before the check of metnoobs. or obs. */
        if (miutil::contains(tokens1_0_lc, "roadobs."))
          ofp.fileformat = roadobs;
        else if (miutil::contains(tokens1_0_lc, "metnoobs.") || miutil::contains(tokens1_0_lc, "obs."))
          ofp.fileformat = metnoobs;
        else if (miutil::contains(tokens1_0_lc, "bufr."))
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
        ofp.parameterfile = tokens2[1];
        ofp.stationfile = tokens3[1];
        ofp.databasefile = tokens4[1];
        filePaths.push_back(ofp);
      }
    }
#endif // ROADOBS
    else {
      std::string filetype="standard", fileformat, fileconfig, model, filename, stationsfilename;
      for (size_t j=0; j<tokens.size(); ++j) {
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
        }
      }
      if (filetype != "standard")
        sources.push_back(vstr[i]);

      filenames[model]= filename;
      stationsfilenames[model]= stationsfilename;
      filetypes[model] = filetype;
      dialogModelNames.push_back(model);
      dialogFileNames.push_back(filename);
    }
  }

#ifdef DEBUGPRINT_FILES
  for (size_t i=0; i<filePaths.size(); i++) {
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFilePath(filePaths[i]);
  }
#endif

  amdarStationFile= LocalSetupParser::basicValue("amdarstationlist");

  miutil::SetupParser::getSection("VERTICAL_PROFILE_COMPUTATIONS", computations);
  setup = miutil::make_shared<vcross::Setup>();
  setup->configureSources(sources);
  setup->configureComputations(computations);
}

void VprofManager::updateObsFileList()
{
  METLIBS_LOG_SCOPE();

  obsfiles.clear();
  obsTime.clear();
  int n= filePaths.size();
  diutil::string_v matches;
  for (int j=0; j<n; j++) {
#ifdef ROADOBS
    if (filePaths[j].fileformat == roadobs) {
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
      if (now.hour()%6 != 0) {
        /* Adjust start hour */
        switch (now.hour()) {
        case  1: case  7: case 13: case 19: now.addHour(-1); break;
        case  2: case  8: case 14: case 20: now.addHour(-2); break;
        case  3: case  9: case 15: case 21: now.addHour(-3); break;
        case  4: case 10: case 16: case 22: now.addHour(-4); break;
        case  5: case 11: case 17: case 23: now.addHour(-5); break;
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
    } else
#endif // ROADOBS
      /* Use glob for ordinary files */
      matches = diutil::glob(filePaths[j].filepath);

    if (filePaths[j].fileformat == bufr) {
#ifdef BUFROBS
      ObsFile of;
      of.obstype    = filePaths[j].obstype;
      of.fileformat = filePaths[j].fileformat;
      of.modificationTime = -1; //no need to check later
      for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
        of.filename = *it;
        if (!filePaths[j].tf.ok()
            || !filePaths[j].tf.getTime(of.filename,of.time))
        {
          ObsBufr bufr;
          if (!bufr.ObsTime(of.filename, of.time))
            continue;
        }
        //time found, no need to check again
        obsfiles.push_back(of);
      }
#endif // BUFROBS
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
    if (!miutil::contains(selectedModels[i].model, "obs."))
      initVprofData(selectedModels[i]);
  }

  onlyObs = false;

  if (showObs && vpdata.empty()) {
    // until anything decided:
    // check observation time and display the most recent file
    checkObsTime();
    onlyObs = true;
  }

  initTimes();
  initStations();

  if (vpdiag) {
    int nobs= (showObs) ? 1 : 0;
    int nmod= vpdata.size();
    if (nobs+nmod==0)
      nobs= 1;
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
  if (onlyObs)
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

  if (onlyObs)
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
      vp.reset(vpdata[i]->getData(plotStations[j], plotTime));
      if (vp.get()) {
        selectedStations.push_back(plotStations[j]);
        break;
      }
    }
  }

  if (!showObs)
    return;

  for (size_t i=0; i<obsList.size(); i++) {
    std::vector<std::string>::const_iterator it
        = std::find(plotStations.begin(), plotStations.end(), nameList[i]);
    if (it != plotStations.end()) {
      selectedStations.push_back(*it);
      break;
    }
  }
}

void VprofManager::plotVpData(DiGLPainter* gl)
{
  for (size_t i=0; i<vpdata.size(); i++) {
    for (size_t j=0; j<plotStations.size(); ++j) {
      std::auto_ptr<VprofPlot> vp(vpdata[i]->getData(plotStations[j], plotTime));
      if (vp.get()) {
        vp->plot(gl, vpopt, i);
        break;
      }
    }
  }
}

bool VprofManager::plot(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(LOGVAL(plotStations.size()) << LOGVAL(plotTime));

  if (!vpdiag) {
    vpdiag= new VprofDiagram(vpopt, gl);
    vpdiag->setPlotWindow(plotw,ploth);
    int nobs= (showObs) ? 1 : 0;
    int nmod= vpdata.size();
    if (nobs+nmod==0)
      nobs= 1;
    vpdiag->changeNumber(nobs,nmod);
  }

  vpdiag->plot();

  if (!plotStations.empty()) {
    plotVpData(gl);

    if (showObs) {
      // obsList corresponds to the obsList.size() first entries of nameList
      METLIBS_LOG_DEBUG(LOGVAL(obsList.size()));
      size_t i = 0;
      for (i = 0; i<obsList.size(); i++) {
        if (std::find(plotStations.begin(), plotStations.end(), nameList[i]) != plotStations.end())
          break;
      }
      if (i<obsList.size()) {
        checkObsTime(plotTime.hour());

        METLIBS_LOG_DEBUG(LOGVAL(obsList.size()) << LOGVAL(i) << LOGVAL(obsList[i]));
        const std::vector<std::string> stationList(1, obsList[i]);
        int nf= obsfiles.size();
        int nn= 0;
        VprofPlot *vp= 0;

        const bool contains_Pilot = miutil::contains(nameList[i], "Pilot");
        while (vp==0 && nn<nf) {
          if (obsfiles[nn].modificationTime && obsfiles[nn].time==plotTime) {
            if (obsfiles[nn].fileformat==bufr &&
                ((not contains_Pilot && obsfiles[nn].obstype != pilot)
                    || (contains_Pilot && obsfiles[nn].obstype == pilot)))
            {
#ifdef BUFROBS
              ObsBufr bufr;
              vector<std::string> vprofFiles;
              //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
              vprofFiles.push_back(obsfiles[nn].filename);
              if (nn>0  && abs(miTime::minDiff(obsfiles[nn-1].time, plotTime)) <= 60 ) {
                vprofFiles.push_back(obsfiles[nn-1].filename);
              }
              std::string modelName;
              if (obsfiles[nn].obstype == amdar) {
                modelName="AMDAR";
              } else if (obsfiles[nn].obstype == temp) {
                modelName="TEMP";
              } else if (obsfiles[nn].obstype == pilot) {
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
                if (showObsTemp && obsfiles[nn].obstype==temp && !contains_Pilot) {
                  //land or ship wmo station with name
                  VprofRTemp vpobs(obsfiles[nn].parameterfile,false,stationList,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                  vp= vpobs.getStation(obsList[i],plotTime);
                } else if (showObsAmdar && obsfiles[nn].obstype==amdar && !contains_Pilot) {
                  VprofRTemp vpobs(obsfiles[nn].parameterfile,true,
                      latitudeList[i],longitudeList[i],0.3f,0.3f,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                  vp = vpobs.getStation(obsList[i],plotTime);
                  if (vp!=0)
                    vp->setName(nameList[i]);
                }
              } catch (...) {
                METLIBS_LOG_ERROR("Exception in: " <<obsfiles[nn].filename);
              }
            }
#endif // ROADOBS
          }
          nn++;
        }
        if (vp) {
          vp->plot(gl, vpopt, vpdata.size());
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
  if ( miutil::contains(modelName,"obs") ) {
    updateObsFileList();
    return rf;
  }
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

/***************************************************************************/

void VprofManager::setFieldModels(const vector<string>& fieldmodels)
{
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}

/***************************************************************************/

void VprofManager::setSelectedModels(const vector <std::string>& models, bool obs)
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
  std::auto_ptr<VprofData> vpd(new VprofData(selectedModel.model, stationsfilenames[selectedModel.model]));
  bool ok = false;
  if (filetypes[model_part] == "standard") {
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
    const ObsFile& ofi = obsfiles[i];
    if (ofi.time==plotTime &&
        ((showObsTemp && ofi.obstype==temp)
            || (showObsPilot && ofi.obstype==pilot)
            || (showObsAmdar && ofi.obstype==amdar)))
    {
      if (ofi.fileformat == bufr) {
        readBufrFile(i, namelist, latitudelist, longitudelist, tlist);
        obslist = namelist;
      } else if (ofi.fileformat==roadobs) {
        readRoadFile(i, namelist, latitudelist, longitudelist, obslist, tlist);
      }
    } else {
      continue;
    }

    unsigned int ns= namelist.size();
    if (ns!=latitudelist.size() || ns!=longitudelist.size() || ns!=obslist.size()) {
      METLIBS_LOG_ERROR("SOMETHING WRONG WITH OBS.STATIONLIST!");
    } else if (ns>0) {
      for (unsigned int j=0; j<ns; j++) {
        if (namelist[j].substr(0,2)=="99") {
          namelist[j]= namelist[j].substr(2,namelist[j].length()-2);
        }
      }
      if (ofi.obstype==pilot) {
        // may have the same station as both Pilot and Temp
        for (unsigned int j=0; j<ns; j++) {
          METLIBS_LOG_DEBUG(LOGVAL(j) << LOGVAL(namelist[j]));
          namelist[j]= "Pilot:" + namelist[j];
        }
      } else if (ofi.obstype==amdar) {
        renameAmdar(namelist,latitudelist,longitudelist,obslist,tlist,amdarCount);
      }
      // check for duplicates
      // name should be used as to check
      std::set<std::string> names;
      for (size_t index = 0; index < namelist.size(); index++) {
        if (names.count(namelist[index]))
          continue;

        names.insert(namelist[index]);

        nameList.push_back(namelist[index]);
        obsList.push_back(obslist[index]);
        latitudeList.push_back(latitudelist[index]);
        longitudeList.push_back(longitudelist[index]);

        if (miutil::trimmed(namelist[index]).empty())
          METLIBS_LOG_WARN("empty name @ " << LOGVAL(index) << LOGVAL(latitudelist[index]) << LOGVAL(longitudelist[index]));
      }
    }
  }

  for (int i = 0; i<nvpdata; i++) {
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

  // remember station
  if (!plotStations.empty())
    lastStation = plotStations[0];

  METLIBS_LOG_DEBUG("lastStation = '"  << lastStation << "'");

  // if it's the first time, plot the first station
  if (plotStations.empty() && !nameList.empty())
    plotStations.push_back(nameList[0]);
}

void VprofManager::readBufrFile(int i, vector <std::string>& namelist,
    vector<float>& latitudelist, vector<float>& longitudelist,
    vector<miTime>& tlist)
{
#ifdef BUFROBS
  ObsBufr bufr;
  vector<std::string> vprofFiles;
  vprofFiles.push_back(obsfiles[i].filename);
  //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
  if (i>0 && abs(miTime::minDiff(obsfiles[i-1].time, plotTime)) <= 60) {
    vprofFiles.push_back(obsfiles[i-1].filename);
  }
  bufr.readStationInfo(vprofFiles, namelist, tlist, latitudelist, longitudelist);
#endif
}

void VprofManager::readRoadFile(int i, vector <std::string>& namelist,
    vector<float>& latitudelist, vector<float>& longitudelist,
    std::vector<std::string>& obslist, vector<miTime>& tlist)
{
#ifdef ROADOBS
  // TDB: Construct stationlist from temp, pilot or amdar stationlist
  const obsType& ot = obsfiles[i].obstype;
  if (ot == temp || ot == pilot || ot == amdar) {
    // TBD!
    // This creates the stationlist
    // we must also intit the connect string to mora
    Roaddata::initRoaddata(obsfiles[i].databasefile);
    diStation::initStations(obsfiles[i].stationfile);
    // get the pointer to the actual station vector
    map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(obsfiles[i].stationfile);
    if (its == diStation::station_map.end()) {
      METLIBS_LOG_ERROR("Unable to find stationlist: " <<obsfiles[i].stationfile);
    } else {
      namelist.clear();
      latitudelist.clear();
      longitudelist.clear();
      obslist.clear();
      const vector<diStation>& stations = *its;
      for (size_t i = 0; i < stations.size(); i++) {
        const diStation& s = stations[i];
        namelist.push_back(s.name());
        latitudelist.push_back(s.lat());
        longitudelist.push_back(s.lon());
        obslist.push_back(miutil::from_number(s.stationID()));
      }
    }
  }
#endif // ROADOBS
}

/***************************************************************************/

void VprofManager::initTimes()
{
  METLIBS_LOG_SCOPE(plotTime.isoTime());

  std::set<miutil::miTime> set_times;
  for (size_t i=0; i<vpdata.size(); ++i) {
    const vector<miutil::miTime>& tmp_times = vpdata[i]->getTimes();
    set_times.insert(tmp_times.begin(), tmp_times.end());
  }
  if (onlyObs)
    set_times.insert(obsTime.begin(), obsTime.end());

  timeList.clear();
  timeList.insert(timeList.end(), set_times.begin(), set_times.end());

  std::vector<miutil::miTime>::const_iterator it = std::find(timeList.begin(), timeList.end(), plotTime);
  if (it == timeList.end() && !timeList.empty()) {
    if (onlyObs)
      plotTime = timeList.back(); // the newest observations
    else
      plotTime = timeList.front();
  }
}

/***************************************************************************/

void VprofManager::checkObsTime(int hour)
{
  METLIBS_LOG_SCOPE(LOGVAL(hour));

  // use hour=-1 to check all files
  // hour otherwise used to spread checking of many files (with plotTime.hour)

  if (hour>23)
    hour=-1;
  bool newtime = obsTime.empty();
  int n= obsfiles.size();

  for (int i=0; i<n; i++) {
#ifdef DEBUGPRINT_FILES
    METLIBS_LOG_DEBUG("index: " << i);
    printObsFiles(obsfiles[i]);
#endif
    if (obsfiles[i].modificationTime<0)
      continue; //no need to check
#ifdef ROADOBS
    if (obsfiles[i].fileformat == roadobs) {
      // Set to nowtime just in case.
      obsfiles[i].modificationTime = time(NULL);
      newtime = true;
    }
#endif
  }
  /* TDB: is this correct for observations from ROAD also ? */
  /* No, always construct a new list */
  if (newtime IF_ROADOBS(&& hour<0)) {
    set<miTime> timeset;
    for (int i=0; i<n; i++)
      if (obsfiles[i].modificationTime)
        timeset.insert(obsfiles[i].time);
    obsTime.clear();
    obsTime.insert(obsTime.end(), timeset.begin(), timeset.end());
  }
}

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

void VprofManager::updateObs()
{
  METLIBS_LOG_SCOPE();

  updateObsFileList();
  checkObsTime();
}

std::string VprofManager::getAnnotationString()
{
  std::ostringstream ost;
  ost << "Vertical profiles ";
  if (onlyObs) {
    ost << plotTime.isoTime();
  } else {
    for (vector <SelectedModel>::iterator p=selectedModels.begin(); p!=selectedModels.end(); p++)
      ost << p->model << ' ';
  }
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

void VprofManager::renameAmdar(vector<std::string>& namelist,
    vector<float>& latitudelist,
    vector<float>& longitudelist,
    vector<std::string>& obslist,
    vector<miTime>& tlist,
    map<std::string,int>& amdarCount)
{
  //should not happen, but ...
  if(namelist.size()!=tlist.size())
    return;

  if (!amdarStationList)
    readAmdarStationList();

  const int n = namelist.size(), m = amdarName.size();
  std::string newname;

  multimap<std::string,int> sortlist;

  for (int i=0; i<n; i++) {
    int jmin = -1;
    float smin = 0.05*0.05 + 0.05*0.05;
    for (int j=0; j<m; j++) {
      float dx = longitudelist[i] - amdarLongitude[j];
      float dy = latitudelist[i]  - amdarLatitude[j];
      float s = dx*dx+dy*dy;
      if (s < smin) {
        smin = s;
        jmin = j;
      }
    }
    if (jmin >= 0) {
      newname= amdarName[jmin];
    } else {
      std::string slat= miutil::from_number(fabsf(latitudelist[i]));
      if (latitudelist[i]>=0.) slat += "N";
      else                     slat += "S";
      std::string slon= miutil::from_number(fabsf(longitudelist[i]));
      if (longitudelist[i]>=0.) slon += "E";
      else                      slon += "W";
      newname= slat + "," + slon;
      jmin = m;
    }

    ostringstream ostr;
    ostr<<setw(4)<<setfill('0')<<jmin;
    std::string sortname= ostr.str() + newname + tlist[i].isoTime() + namelist[i];
    sortlist.insert(pair<std::string,int>(sortname,i));

    namelist[i]= newname;
  }

  multimap<std::string,int>::iterator pt, ptend= sortlist.end();

  // gather amdars from same stations (in station list sequence)
  vector<std::string> namelist2;
  vector<float>    latitudelist2;
  vector<float>    longitudelist2;
  vector<std::string> obslist2;
  for (pt=sortlist.begin(); pt!=ptend; pt++) {
    int i= pt->second;

    newname = namelist[i];
    int c;
    std::map<std::string, int>::iterator p = amdarCount.find(newname);
    if (p==amdarCount.end())
      amdarCount[newname] = c = 1;
    else
      c = ++(p->second);
    newname += " (" + miutil::from_number(c) + ")";

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

  if (amdarStationFile.empty())
    return;

  // open filestream
  ifstream file;
  file.open(amdarStationFile.c_str());
  if (file.bad()) {
    METLIBS_LOG_ERROR("Amdar station list '" << amdarStationFile << "' not found");
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
  METLIBS_LOG_DEBUG("ObsFile: <" << LOGVAL(of.filename) << LOGVAL(of.obstype) << LOGVAL(of.fileformat)
      << LOGVAL(of.time.isoTime(true, true)) << LOGVAL(of.modificationTime)
      IF_ROADOBS(<< LOGVAL(ofp.parameterfile) << LOGVAL(ofp.stationfile) << LOGVAL(ofp.databasefile))
      <<  '>');
}

void VprofManager::printObsFilePath(const ObsFilePath & ofp)
{
  METLIBS_LOG_DEBUG("ObsFilePath: <" << LOGVAL(ofp.filepath) << LOGVAL(ofp.obstype) << LOGVAL(ofp.fileformat)
      IF_ROADOBS(<< LOGVAL(ofp.parameterfile) << LOGVAL(ofp.stationfile) << LOGVAL(ofp.databasefile))
      <<  '>');
}

