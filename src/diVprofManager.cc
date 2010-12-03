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

#include <fstream>
#include <diVprofManager.h>
#include <diField/diFieldManager.h>

#include <diVprofOptions.h>
#include <diVprofData.h>
#include <diVprofDiagram.h>
#ifdef METNOOBS
#include <diVprofPlot.h>
#include <diVprofTemp.h>
#include <diVprofPilot.h>
#include <robs/obs.h>
#endif
#ifdef BUFROBS
#include <diObsBufr.h>
#endif
#ifdef ROADOBS
#include <vector>
#include <map>
#ifdef NEWARK_INC
#include <newarkAPI/diStation.h>
#else
#include <roadAPI/diStation.h>
#endif
#include <diVprofRTemp.h>
#endif
#include <diSetupParser.h>

#include <puCtools/glob.h>
#include <puCtools/glob_cache.h>
#include <puCtools/stat.h>
#include <math.h>

#ifdef ROADOBS
using namespace std;
using namespace road;
#endif

using namespace::miutil;

VprofManager::VprofManager(Controller* co)
: amdarStationList(false), vpdiag(0), showObs(false),
  showObsTemp(false), showObsPilot(false), showObsAmdar(false),
  plotw(0), ploth(0), hardcopy(false)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager constructed" << endl;
#endif
  fieldm= co->getFieldManager(); // set fieldmanager

  vpopt= new VprofOptions();  // defaults are set

  parseSetup();

  updateObsFileList();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();

}


VprofManager::~VprofManager()
{
#ifdef DEBUGPRINT
  cerr << "VprofManager destructor" << endl;
#endif

  if (vpdiag) delete vpdiag;
  if (vpopt)  delete vpopt;

  for (unsigned int i=0; i<vpdata.size(); i++)
    delete vpdata[i];
}


void VprofManager::parseSetup()
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::parseSetup" << endl;
#endif

  const miString section2 = "VERTICAL_PROFILE_FILES";
  vector<miString> vstr;

  if (sp.getSection(section2,vstr)) {

    set<miString> uniquefiles;

    miString model,filename;
    vector<miString> tokens,tokens1,tokens2, tokens3, tokens4;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split();
      if (tokens.size()==1) {
        tokens1= tokens[0].split("=");
        if (tokens1.size()==2) {
          ObsFilePath ofp;
          if (tokens1[0].downcase().contains("metnoobs.") ||
              tokens1[0].downcase().contains("obs.") )
            ofp.fileformat = metnoobs;
          else if (tokens1[0].downcase().contains("bufr."))
            ofp.fileformat = bufr;
          else
            continue;
          if (tokens1[0].downcase().contains(".temp")) {
            ofp.obstype = temp;
          } else if (tokens1[0].downcase().contains(".amdar")) {
            ofp.obstype = amdar;
          } else if (tokens1[0].downcase().contains(".pilot")) {
            ofp.obstype = pilot;
          } else {
            continue;
          }
          ofp.tf.initFilter(tokens1[1]);
          ofp.filepath = tokens1[1];
          filePaths.push_back(ofp);
        }
      } else if (tokens.size()==2) {
        tokens1= tokens[0].split("=");
        tokens2= tokens[1].split("=");
        if (tokens1.size()==2          && tokens2.size()==2  &&
            tokens1[0].downcase()=="m" && tokens2[0].downcase()=="f") {
          model= tokens1[1];
          filename= tokens2[1];
          filenames[model]= filename;
          filetypes[filename] = "standard";
          if (uniquefiles.find(filename)==uniquefiles.end()) {
            uniquefiles.insert(filename);
            dialogModelNames.push_back(model);
            dialogFileNames.push_back(filename);
          }
        }
      }
      else if (tokens.size()==3) {
        tokens1= tokens[0].split("=");
        tokens2= tokens[1].split("=");
        tokens3= tokens[2].split("=");
        if (tokens1.size()==2 && tokens2.size()==2 && tokens3.size()==2
            && tokens1[0].downcase()=="m" && tokens2[0].downcase()=="t"
                && tokens3[0].downcase()=="s") {
          model= tokens1[1];
          miString filetype = tokens2[1];
          filename= tokens3[1];
          filenames[model]= filename;
          filetypes[filename] = filetype;
          uniquefiles.insert(filename);
          dialogModelNames.push_back(model);
          dialogFileNames.push_back(filename);
        }
      }
#ifdef ROADOBS
      /* Here we know that it is the extended obs format */
      else if (tokens.size()==4) {
        tokens1= tokens[0].split("=");
        tokens2= tokens[1].split("=");
        tokens3= tokens[2].split("=");
        tokens4= tokens[3].split("=");
        if (tokens1.size()==2 && tokens2.size()==2
            && tokens3.size()==2 && tokens4.size()==2
            && tokens2[0].downcase()=="p" && tokens3[0].downcase()=="s"
                && tokens4[0].downcase()=="d") {
          ObsFilePath ofp;
          /* the check for roadobs must be before the check of metnoobs. or obs. */
          if (tokens1[0].downcase().contains("roadobs."))
            ofp.fileformat = roadobs;
          else if (tokens1[0].downcase().contains("metnoobs.")
              || tokens1[0].downcase().contains("obs.") )
            ofp.fileformat = metnoobs;
          else if (tokens1[0].downcase().contains("bufr."))
            ofp.fileformat = bufr;
          else
            continue;
          if (tokens1[0].downcase().contains(".temp")) {
            ofp.obstype = temp;
          } else if (tokens1[0].downcase().contains(".amdar")) {
            ofp.obstype = amdar;
          } else if (tokens1[0].downcase().contains(".pilot")) {
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
    }

    vstr.clear();
#ifdef DEBUGPRINT
    int l= filePaths.size();
    for (int i=0; i<l; i++) {
      cerr << "index: " << i << endl;
      printObsFilePath(filePaths[i]);
    }
#endif

  } else {

    cerr << "Missing section " << section2 << " in setupfile." << endl;

  }

  amdarStationFile= sp.basicValue("amdarstationlist");

}


void VprofManager::updateObsFileList()
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::updateObsFileList" << endl;
#endif
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
      of.modificationTime= 0;
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
        of.modificationTime= 0;
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
    if(of.fileformat == metnoobs){
#ifdef METNOOBS
      of.time      = miTime(1970,1,1,0,0,0);
      of.modificationTime= 0;
      for (int i=0; i<globBuf.gl_pathc; i++) {
        of.filename= miString(globBuf.gl_pathv[i]);
        obsfiles.push_back(of);
      }
#endif
    } else if(of.fileformat == bufr){
#ifdef BUFROBS
      of.modificationTime= -1; //no need to check later
      for (int i=0; i<globBuf.gl_pathc; i++) {
        of.filename= miString(globBuf.gl_pathv[i]);
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
#ifdef DEBUGPRINT
  int l= obsfiles.size();
  for (int i=0; i<l; i++) {
    cerr << "index: " << i << endl;
    printObsFiles(obsfiles[i]);
  }
#endif
}


void VprofManager::setPlotWindow(int w, int h)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setPlotWindow:" << w << " " << h << endl;
#endif
  plotw= w;
  ploth= h;
  if (vpdiag) vpdiag->setPlotWindow(plotw,ploth);
}


//*********************** end routines from controller ***********************

void VprofManager::setModel()
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setModel" << endl;
#endif

  // should not clear all data, possibly needed again...

  for (unsigned int i=0; i<vpdata.size(); i++)
    delete vpdata[i];
  vpdata.clear();

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
      cerr << "NO VPROFDATA for model " << model << endl;
    } else
      initVprofData(pf->second,model);
  }

  //define models/files  when "file" chosen in modeldialog
  vector <miString>::iterator q = selectedFiles.begin();
  for (; q!=selectedFiles.end(); q++) {
    miString file= *q;
    if (file.contains(menuConst["OBSTEMP"]))
      showObs= showObsTemp= true;
    else if (file.contains(menuConst["OBSPILOT"]))
      showObs= showObsPilot= true;
    else if (file.contains(menuConst["OBSAMDAR"]))
      showObs= showObsAmdar= true;
    else {
      map<miString,miString>::iterator pf=filenames.begin();
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

#ifdef DEBUGPRINT
  cerr << "VprofManager::setModels finished" << endl;
#endif
}


void VprofManager::setStation(const miString& station)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setStation  " << station << endl;
#endif

  plotStation= station;
}


void VprofManager::setTime(const miTime& time)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setTime  " << time << endl;
#endif

  plotTime= time;

  if (onlyObs)
    initStations();
}


miString VprofManager::setStation(int step)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setStation   step=" << step << endl;
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

  plotStation= nameList[i];
  return plotStation;
}


miTime VprofManager::setTime(int step)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::setTime   step=" << step << endl;
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
#ifdef DEBUGPRINT
  cerr << "VprofManager::plot  " << plotStation << "  " << plotTime << endl;
#endif

  if (!vpdiag) {
    vpdiag= new VprofDiagram(vpopt);
    vpdiag->setPlotWindow(plotw,ploth);
    int nobs= (showObs) ? 1 : 0;
    int nmod= vpdata.size();
    if (nobs+nmod==0) nobs= 1;
    vpdiag->changeNumber(nobs,nmod);
  }

  // postscript output
  if (hardcopy && !hardcopystarted) {
    vpdiag->startPSoutput(printoptions);
    hardcopystarted= true;
  }

  vpdiag->plot();

  if (plotStation.exists()) {

    int m= vpdata.size();

    for (int i=0; i<m; i++) {
      VprofPlot *vp= vpdata[i]->getData(plotStation,plotTime);
      if (vp) {
        vp->plot(vpopt,i);
        delete vp;
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
        int nf= obsfiles.size();
        int nn= 0;
        VprofPlot *vp= 0;

        while (vp==0 && nn<nf) {
          if (obsfiles[nn].modificationTime &&
              obsfiles[nn].time==plotTime) {
            if(obsfiles[nn].fileformat==metnoobs){
#ifdef METNOOBS
              try {
                if (showObsTemp && obsfiles[nn].obstype==temp &&
                    !nameList[i].contains("Pilot")) {
                  if (obsList[i]!="99") {
                    // land or ship station with name
                    VprofTemp vpobs(obsfiles[nn].filename,false,stationList);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  } else {
                    // ship station without name
                    VprofTemp vpobs(obsfiles[nn].filename,false,
                        latitudeList[i],longitudeList[i],2.0f,2.0f);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  }
                } else if (showObsPilot && obsfiles[nn].obstype==pilot &&
                    nameList[i].contains("Pilot")) {
                  if (obsList[i]!="99") {
                    // land or ship station with name
                    VprofPilot vpobs(obsfiles[nn].filename,stationList);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  } else {
                    // ship station without name
                    VprofPilot vpobs(obsfiles[nn].filename,
                        latitudeList[i],longitudeList[i],2.0f,2.0f);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  }
                } else if (showObsAmdar && obsfiles[nn].obstype==amdar &&
                    !nameList[i].contains("Pilot")) {
                  VprofTemp vpobs(obsfiles[nn].filename,true,
                      latitudeList[i],longitudeList[i],0.3f,0.3f);
                  vp= vpobs.getStation(obsList[i],plotTime);
                  if (vp!=0) vp->setName(nameList[i]);
                }
              }
              catch (...) {
                cerr<<"Exception in: " <<obsfiles[nn].filename<<endl;
              }
#endif
            } else if(obsfiles[nn].fileformat==bufr &&
                ((!nameList[i].contains("Pilot") && obsfiles[nn].obstype!=pilot) ||
                    (nameList[i].contains("Pilot") && obsfiles[nn].obstype==pilot)) ) {
#ifdef BUFROBS
              ObsBufr bufr;
              vector<miString> vprofFiles;
              //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
              vprofFiles.push_back(obsfiles[nn].filename);
              if (nn>0  && abs(miTime::minDiff(obsfiles[nn-1].time, plotTime)) <= 60 ) {
                vprofFiles.push_back(obsfiles[nn-1].filename);
              }
              vp=bufr.getVprofPlot(vprofFiles,obsList[i],plotTime);
#endif
            }
#ifdef ROADOBS
            /* NOTE! If metoobs are used, all data are fetched when constructing, for example, the VprofTemp object. */
            /* If we fetch data from road, we should fetch data for one station, obsList[i],plotTime, to improve performance */
            /*   The VprofRTemp class should be implemented, to simplify code */
            else if (obsfiles[nn].fileformat==roadobs) {
              try {
                if (showObsTemp && obsfiles[nn].obstype==temp &&
                    !nameList[i].contains("Pilot")) {
                  if (obsList[i]!="99") {
                    //land or ship station with name
                    VprofRTemp vpobs(obsfiles[nn].parameterfile,false,stationList,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  } else {
                    //ship station without name
                    VprofRTemp vpobs(obsfiles[nn].parameterfile,false,
                        latitudeList[i],longitudeList[i],2.0f,2.0f,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  }
                } else if (showObsPilot && obsfiles[nn].obstype==pilot &&
                    nameList[i].contains("Pilot")) {
                  if (obsList[i]!="99") {
                    // land or ship station with name
                    VprofPilot vpobs(obsfiles[nn].filename,stationList);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  } else {
                    // ship station without name
                    VprofPilot vpobs(obsfiles[nn].filename,
                        latitudeList[i],longitudeList[i],2.0f,2.0f);
                    vp= vpobs.getStation(obsList[i],plotTime);
                  }
                } else if (showObsAmdar && obsfiles[nn].obstype==amdar &&
                    !nameList[i].contains("Pilot")) {
                  VprofRTemp vpobs(obsfiles[nn].parameterfile,true,
                      latitudeList[i],longitudeList[i],0.3f,0.3f,obsfiles[nn].stationfile,obsfiles[nn].databasefile,plotTime);
                  vp= vpobs.getStation(obsList[i],plotTime);
                  if (vp!=0) vp->setName(nameList[i]);
                }
              }
              catch (...) {
                cerr<<"Exception in: " <<obsfiles[nn].filename<<endl;
              }
            }
#endif

          }
          nn++;
        }
        if (vp) {
          vp->plot(vpopt,m);
          delete vp;
        }
      }
    }

    vpdiag->plotText();
  }

#ifdef DEBUGPRINT
  cerr << "VprofManager::plot finished" << endl;
#endif
  return true;
}


/***************************************************************************/

vector <miString> VprofManager::getModelNames(){
#ifdef DEBUGPRINT
  cerr << "VprofManager::getModelNames" << endl;
#endif
  updateObsFileList();
  return dialogModelNames;
}

/***************************************************************************/

vector <miString> VprofManager::getModelFiles(){
#ifdef DEBUGPRINT
  cerr << "VprofManager::getModelFiles" << endl;
#endif
  vector<miString> modelfiles= dialogFileNames;
  updateObsFileList();
  int n= obsfiles.size();
  for (int i=0; i<n; i++) {
#ifdef DEBUGPRINT
    cerr << "index: " << i << endl;
    printObsFiles(obsfiles[i]);
#endif
    modelfiles.push_back(obsfiles[i].filename);
  }
  return modelfiles;
}


/***************************************************************************/

void VprofManager::setFieldModels(const vector <miString> & fieldmodels){
  //called when model selected in field dialog
  fieldModels = fieldmodels;
}


/***************************************************************************/

void VprofManager::setSelectedModels(const vector <miString>& models ,
    bool field, bool obsTemp,
    bool obsPilot, bool obsAmdar){
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

void VprofManager::setSelectedFiles(const vector <miString>& files,
    bool field, bool obsTemp,
    bool obsPilot, bool obsAmdar){
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

miString VprofManager::getDefaultModel(){
  //for now, just the first model in filenames list
  map<miString,miString>::iterator p = filenames.begin();
  miString model = p->first;
  return model;
}


/***************************************************************************/
vector<miString> VprofManager::getSelectedModels(){
  vector <miString> models = selectedModels;
  if (showObsTemp)  models.push_back(menuConst["OBSTEMP"]);
  if (showObsPilot) models.push_back(menuConst["OBSPILOT"]);
  if (showObsAmdar) models.push_back(menuConst["OBSAMDAR"]);
  if (asField)      models.push_back(menuConst["ASFIELD"]);
  return models;
}


/***************************************************************************/

bool VprofManager::initVprofData(miString file,miString model){
  VprofData *vpd= new VprofData(file,model);
  if(filetypes[file] == "standard") {
    if (vpd->readFile()) {
      cerr << "VPROFDATA READFILE OK for model " << model << endl;
      vpdata.push_back(vpd);
      return true;
    } else {
      cerr << "VPROFDATA READFILE ERROR file " << file << endl;
      delete vpd;
      return false;
    }
  } else if (filetypes[file] == "GribFile") {
    //    cerr << "Model is a gribfile" << endl;
    if (vpd->readField(filetypes[file],fieldm)) {
      cerr << "VPROFDATA READFIELD OK for model " << model << endl;
      vpdata.push_back(vpd);
      return true;
    } else {
      cerr << "VPROFDATA READFIELD ERROR: " << file << endl;
      return false;
    }
  }

  return false;
}

/***************************************************************************/

void VprofManager::initStations(){
  //merge lists from all models
  int nvpdata = vpdata.size();
#ifdef DEBUGPRINT
  cerr << "VprofManager::initStations-size of vpdata " << nvpdata << endl;
#endif

  nameList.clear();
  latitudeList.clear();
  longitudeList.clear();
  obsList.clear();

  vector <miString> namelist;
  vector <float>    latitudelist;
  vector <float>    longitudelist;
  vector <miString> obslist;
  vector<miTime> tlist;

  for (int i = 0;i<nvpdata;i++){
    namelist= vpdata[i]->getNames();
    latitudelist= vpdata[i]->getLatitudes();
    longitudelist= vpdata[i]->getLongitudes();
    obslist= vpdata[i]->getObsNames();
    unsigned int n=namelist.size();
    if (n!=latitudelist.size()||n!=longitudelist.size()||
        n!=obslist.size()) {
      cerr << "diVprofManager::initStations - SOMETHING WRONG WITH STATIONLIST!"
          << endl;
    } else if (n>0) {
      nameList.insert(nameList.end(),namelist.begin(),namelist.end());
      obsList.insert(obsList.end(),obslist.begin(),obslist.end());
      latitudeList.insert(latitudeList.end(),latitudelist.begin(),latitudelist.end());
      longitudeList.insert(longitudeList.end(),longitudelist.begin(),longitudelist.end());
    }
  }

  // using current time until..................
  map<miString,int> amdarCount;
  int n= obsfiles.size();
  for (int i=0; i<n; i++) {
    if (obsfiles[i].time==plotTime &&
        ((showObsTemp  && obsfiles[i].obstype==temp) ||
            (showObsPilot && obsfiles[i].obstype==pilot) ||
            (showObsAmdar && obsfiles[i].obstype==amdar))) {
      if(obsfiles[i].fileformat==bufr){
#ifdef BUFROBS
        ObsBufr bufr;
        vector<miString> vprofFiles;
        vprofFiles.push_back(obsfiles[i].filename);
        //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
        if (i>0  && abs(miTime::minDiff(obsfiles[i-1].time, plotTime)) <= 60 ) {
          vprofFiles.push_back(obsfiles[i-1].filename);
        }
        bufr.readStationInfo(vprofFiles,
            namelist,latitudelist,longitudelist);
#endif
      } else if(obsfiles[i].fileformat==metnoobs){
#ifdef METNOOBS
        try {
          // until robs' obs class can do the job:
          obs ofile;
          ofile.readStationHeaders(obsfiles[i].filename);
          namelist=      ofile.getStationIds();
          latitudelist=  ofile.getStationLatitudes();
          longitudelist= ofile.getStationLongitudes();
          if (obsfiles[i].obstype==amdar )
            tlist= ofile.getStationTimes();
        }
        catch (...) {
          cerr<<"Exception in: " <<obsfiles[i].filename<<endl;
        }
#endif
#ifdef ROADOBS
      } else if (obsfiles[i].fileformat==roadobs) {
        // TDB: Construct stationlist from temp, pilot or amdar stationlist
        if ((obsfiles[i].obstype==temp )||(obsfiles[i].obstype==pilot )||(obsfiles[i].obstype==amdar ))
        {
          // TBD!
          // This creates the stationlist
          diStation::initStations(obsfiles[i].stationfile);
          // get the pointer to the actual station vector
          vector<diStation> * stations = NULL;
          map<miString, vector<diStation> * >::iterator its = diStation::station_map.find(obsfiles[i].stationfile);
          if (its != diStation::station_map.end())
          {
            stations = its->second;
          }
          if (stations == NULL)
          {
            cerr<<"Unable to find stationlist: " <<obsfiles[i].stationfile << endl;
          }
          else
          {
            int noOfStations = stations->size();
            namelist.clear();
            latitudelist.clear();
            longitudelist.clear();
            for (int i = 0; i < noOfStations; i++)
            {
              namelist.push_back((*stations)[i].name());
              latitudelist.push_back((*stations)[i].lat());
              longitudelist.push_back((*stations)[i].lon());
            }
          }
        }
      }
#else
    }
#endif
    obslist= namelist;
    unsigned int ns= namelist.size();
    if (ns!=latitudelist.size() || ns!=longitudelist.size() ||
        ns!=obslist.size()) {
      cerr << "diVprofManager::initStations - SOMETHING WRONG WITH OBS.STATIONLIST!"
          << endl;
    } else if (ns>0) {
      for (unsigned int j=0; j<ns; j++) {
        if (namelist[j].substr(0,2)=="99") {
#ifdef linux
          // until robs (linux swap problem) fixed
          // (note that "obslist" is kept unchanged/wrong for reading)
          miString callsign=  namelist[j].substr(3,1)
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
      nameList.insert(nameList.end(),namelist.begin(),namelist.end());
      obsList.insert(obsList.end(),obslist.begin(),obslist.end());
      latitudeList.insert(latitudeList.end(),latitudelist.begin(),latitudelist.end());
      longitudeList.insert(longitudeList.end(),longitudelist.begin(),longitudelist.end());
    }
  }
}

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
}
#ifdef DEBUGPRINT
cerr <<"plotStation" << plotStation << endl;
#endif
}


/***************************************************************************/

void VprofManager::initTimes(){
#ifdef DEBUGPRINT
  cerr << "VprofManager::initTimes:" << plotTime.isoTime()<<endl;
#endif

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
#ifdef DEBUGPRINT
  cerr << "VprofManager::checkObsTime  hour= " << hour << endl;
#endif

  // use hour=-1 to check all files
  // hour otherwise used to spread checking of many files (with plotTime.hour)

  if (hour>23) hour=-1;
  bool newtime= !obsTime.size();
  int n= obsfiles.size();

  pu_struct_stat statbuf;

  for (int i=0; i<n; i++) {
#ifdef DEBUGPRINT
    cerr << "index: " << i << endl;
    printObsFiles(obsfiles[i]);
#endif
    if (obsfiles[i].modificationTime<0)
      continue; //no need to check
#ifdef ROADOBS
    if (obsfiles[i].fileformat == roadobs)
    {
      /* always read data from road */
      if (obsfiles[i].modificationTime==0 || hour<0 ||
          obsfiles[i].time.hour()==hour) {
        obsfiles[i].modificationTime = time(NULL);
        newtime= true;
      }
    }
    else
    {
#ifdef METNOOBS
      if (obsfiles[i].modificationTime==0 || hour<0 ||
          obsfiles[i].time.hour()==hour) {
        if (pu_stat(obsfiles[i].filename.c_str(),&statbuf)==0) {
          if (obsfiles[i].modificationTime!=statbuf.st_mtime) {
            obsfiles[i].modificationTime= statbuf.st_mtime;
            try {
              obs ofile;
              ofile.readFileHeader(obsfiles[i].filename);
              if (obsfiles[i].time != ofile.fileObsTime()) newtime= true;
              obsfiles[i].time= ofile.fileObsTime();
            }
            catch (...) {
              cerr<<"Exception in: "<<obsfiles[i].filename<<endl;
            }
          }
        }
      }
#endif
    } /* end if fileformat == roadobs */
#else
    /* no roadobs support, use the old code */
#ifdef METNOOBS
    if (obsfiles[i].modificationTime==0 || hour<0 ||
        obsfiles[i].time.hour()==hour) {
      if (pu_stat(obsfiles[i].filename.c_str(),&statbuf)==0) {
        if (obsfiles[i].modificationTime!=statbuf.st_mtime) {
          obsfiles[i].modificationTime= statbuf.st_mtime;
          try {
            obs ofile;
            ofile.readFileHeader(obsfiles[i].filename);
            if (obsfiles[i].time != ofile.fileObsTime()) newtime= true;
            obsfiles[i].time= ofile.fileObsTime();
          }
          catch (...) {
            cerr<<"Exception in: "<<obsfiles[i].filename<<endl;
          }
        }
      }
    }
#endif
#endif
  }
  /* TDB: is this correct for observations from ROAD also ? */
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


void VprofManager::mainWindowTimeChanged(const miTime& time)
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::mainWindowTimeChanged  " << time << endl;
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


void VprofManager::updateObs()
{
#ifdef DEBUGPRINT
  cerr << "VprofManager::updateObs" << endl;
#endif
  updateObsFileList();
  checkObsTime();
}


miString VprofManager::getAnnotationString(){
  miString str = miString("Vertikalprofiler ");
  if (onlyObs)
    str += plotTime.isoTime();
  else
    for (set <miString>::iterator p=usemodels.begin();p!=usemodels.end();p++)
      str+=*p+miString(" ");
  return str;
}


vector<miString> VprofManager::writeLog()
{
  return vpopt->writeOptions();
}


void VprofManager::readLog(const vector<miString>& vstr,
    const miString& thisVersion,
    const miString& logVersion)
{
  vpopt->readOptions(vstr);
}


void VprofManager::renameAmdar(vector<miString>& namelist,
    vector<float>& latitudelist,
    vector<float>& longitudelist,
    vector<miString>& obslist,
    vector<miTime>& tlist,
    map<miString,int>& amdarCount)
{
  //should not happen, but ...
  if(namelist.size()!=tlist.size()) return;

  if (!amdarStationList) readAmdarStationList();

  int n=namelist.size();
  int m= amdarName.size();
  int jmin,c;
  float smin,dx,dy,s;
  miString newname;

  multimap<miString,int> sortlist;

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
      miString slat= miString(fabsf(latitudelist[i]));
      if (latitudelist[i]>=0.) slat += "N";
      else                     slat += "S";
      miString slon= miString(fabsf(longitudelist[i]));
      if (longitudelist[i]>=0.) slon += "E";
      else                      slon += "W";
      newname= slat + "," + slon;
      jmin= m;
    }

    ostringstream ostr;
    ostr<<setw(4)<<setfill('0')<<jmin;
    miString sortname= ostr.str() + newname + tlist[i].isoTime() + namelist[i];
    sortlist.insert(pair<miString,int>(sortname,i));

    namelist[i]= newname;
  }

  map<miString,int>::iterator p;
  multimap<miString,int>::iterator pt, ptend= sortlist.end();

  // gather amdars from same stations (in station list sequence)
  vector<miString> namelist2;
  vector<float>    latitudelist2;
  vector<float>    longitudelist2;
  vector<miString> obslist2;

  for (pt=sortlist.begin(); pt!=ptend; pt++) {
    int i= pt->second;

    newname= namelist[i];
    p= amdarCount.find(newname);
    if (p==amdarCount.end())
      amdarCount[newname]= c= 1;
    else
      c= ++(p->second);
    newname+= " (" + miString(c) + ")";

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
  file.open(amdarStationFile.cStr());
  if (file.bad()) {
    cerr<<"Amdar station list  "<<amdarStationFile<<"  not found"<<endl;
    return;
  }

  const float notFound=9999.;
  vector<miString> vstr,vstr2;
  miString str;
  unsigned int i;

  while (getline(file,str)) {
    std::string::size_type n= str.find('#');
    if (n!=0) {
      if (n!=string::npos) str= str.substr(0,n);
      str.trim();
      if (str.exists()) {
        vstr= str.split(" ");
        float latitude=notFound, longitude=notFound;
        miString name;
        n=vstr.size();
        for (i=0; i<n; i++) {
          vstr2= vstr[i].split("=");
          if (vstr2.size()==2) {
            str= vstr2[0].downcase();
            if (str=="latitude")
              latitude= atof(vstr2[1].cStr());
            else if (str=="longitude")
              longitude= atof(vstr2[1].cStr());
            else if (str=="name") {
              name= vstr2[1];
              if (name[0]=='"')
                name= name.substr(1,name.length()-2);
            }
          }
        }
        if (latitude!=notFound && longitude!=notFound && name.exists()) {
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
     miString   filename; 
     obsType    obstype; 
     FileFormat fileformat; 
     miTime     time; 
     long       modificationTime; 
     }; 
   */
  cerr << "ObsFile: < " << endl; 
  cerr << "filename: " << of.filename << endl; 
  cerr << "obsType: " << of.obstype << endl; 
  cerr << "FileFormat: " << of.fileformat << endl; 
  cerr << "Time: " << of.time.isoTime(true, true) << endl; 
  cerr << "ModificationTime: " << of.modificationTime << endl; 
#ifdef ROADOBS 
  cerr << "Parameterfile: " << of.parameterfile << endl; 
  cerr << "Stationfile: " << of.stationfile << endl; 
  cerr << "Databasefile: " << of.databasefile << endl; 
#endif 
  cerr << ">" << endl; 
} 

void VprofManager::printObsFilePath(const ObsFilePath & ofp) 
{ 
  /* 
     struct ObsFilePath { 
     miString   filepath; 
     obsType    obstype; 
     FileFormat fileformat; 
     TimeFilter tf; 
     }; 
   */
  cerr << "ObsFilePath: < " << endl; 
  cerr << "filepath: " << ofp.filepath << endl; 
  cerr << "obsType: " << ofp.obstype << endl; 
  cerr << "FileFormat: " << ofp.fileformat << endl; 
#ifdef ROADOBS 
  cerr << "Parameterfile: " << ofp.parameterfile << endl; 
  cerr << "Stationfile: " << ofp.stationfile << endl; 
  cerr << "Databasefile: " << ofp.databasefile << endl; 
#endif 
  cerr << ">" << endl; 
} 
