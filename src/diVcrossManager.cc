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

#include <diVcrossManager.h>

#include <diVcrossOptions.h>
#include <diVcrossFile.h>
#include <diVcrossField.h>
#include <diVcrossPlot.h>
#include <set>

#include <diLocalSetupParser.h>
#include <puCtools/puCglob.h>

//#define DEBUGPRINT 1

using namespace::miutil;

VcrossManager::VcrossManager(Controller *co)
: dataChange(true), timeGraphPos(-1) , timeGraphPosMax(-1), hardcopy(false)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager constructed" << endl;
#endif

  fieldm= co->getFieldManager(); // set fieldmanager

  vcopt= new VcrossOptions();  // defaults are set

  parseSetup();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
}


VcrossManager::~VcrossManager()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager destructor" << endl;
#endif

  cleanup();

  if (vcopt) delete vcopt;
}


void VcrossManager::cleanup()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::cleanup" << endl;
#endif

  plotCrossection.clear();
  masterFile.clear();
  usedModels.clear();

  selectedModels.clear();
  selectedFields.clear();
  selectedHourOffset.clear();
  selectedPlotOptions.clear();
  selectedPlotShaded.clear();
  selectedVcFile.clear();
  selectedVcData.clear();

  int n= vcdata.size();
  for (int i=0; i<n; i++)
    delete vcdata[i].vcplot;

  vcdata.clear();

  map<miString,VcrossFile*>::iterator vf,vfend= vcfiles.end();

  for (vf=vcfiles.begin(); vf!=vfend; vf++) {
    if (vf->second) {
      delete vf->second;
      vf->second= 0;
    }
  }

  map<miString, VcrossField*>::iterator vfi, vfiend = vcfields.end();
  for (vfi = vcfields.begin(); vfi != vfiend; vfi++) {
    if (vfi->second) {
      delete vfi->second;
      vfi->second = 0;
    }
  }
  vcfields.clear();

}

void VcrossManager::cleanupDynamicCrossSections() {
#ifdef DEBUGPRINT
  cerr << "VcrossManager::cleanupDynamicCrossSections" << endl;
#endif
  map<miString, VcrossField*>::iterator vfi, vfiend = vcfields.end();
  for (vfi = vcfields.begin(); vfi != vfiend; vfi++) {
    if (vfi->second) {
      vfi->second->cleanup();
    }
  }
}

bool VcrossManager::parseSetup()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::parseSetup" << endl;
#endif

  const miString section1 = "VERTICAL_CROSSECTION_FILES";

  vector<miString> vstr;

  bool ok= true;

  if (SetupParser::getSection(section1,vstr)) {

    set<miString> uniquemodels;

    miString model,filename;
    vector<miString> tokens,tokens1,tokens2;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split();
      if (tokens.size()==2) {
        tokens1= tokens[0].split("=");
        tokens2= tokens[1].split("=");
        if (tokens1.size()==2          && tokens2.size()==2) {
	  if (tokens1[0].downcase()=="m" && tokens2[0].downcase()=="f") {
	    model= tokens1[1];
	    filename= tokens2[1];
	    filenames[model]= filename;
	    modelnames.push_back(model);
	    filetypes[model] = "standard";
	    vcfiles[filename]= 0;
	  }
	  else if (tokens1[0].downcase() == "m" && tokens2[0].downcase() == "t") {
            model = tokens1[1];
            miString filetype = tokens2[1];
	    modelnames.push_back(model);
            filetypes[model] = filetype;
	    vcfiles[filename]= 0;
          }
	}
      }
    }

    vstr.clear();

  } else {
    //cerr << "Missing section " << section1 << " in setupfile." << endl;
    //ok= false;
  }

  // parse remaining setup sections
  if (!VcrossPlot::parseSetup()) ok= false;

  return ok;
}


vector< vector<Colour::ColourInfo> > VcrossManager::getMultiColourInfo(int multiNum){
  return LocalSetupParser::getMultiColourInfo(multiNum);
}


void VcrossManager::setCrossection(const miString& crossection)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setCrossection  " << crossection << endl;
#endif

  plotCrossection= crossection;
  dataChange= true;
}

bool VcrossManager::setCrossection(float lat, float lon) {
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setCrossection (" << lat << "," << lon << ")" << endl;
#endif
  map<miutil::miString, VcrossField*>::iterator vfi;
  if (vcfields.size() == 0)
    return false;
  for (vfi=vcfields.begin(); vfi != vcfields.end(); vfi++) {
    (*vfi).second->setLatLon(lat, lon);
    // Get the new namelist with the crossections
    nameList = (*vfi).second->getNames();
    timeList = (*vfi).second->getTimes();
  }
  return true;
}

void VcrossManager::setTime(const miTime& time)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setTime  " << time << endl;
#endif

  plotTime= time;
  dataChange= true;
}


miString VcrossManager::setCrossection(int step)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setCrossection   step=" << step << endl;
#endif

  if (nameList.size()==0)
    return "";

  int i= 0;
  int n= nameList.size();
  if (!plotCrossection.empty())
    while (i<n && nameList[i]!=plotCrossection) i++;

  if (i<n) {
    i+=step;
    if (i<0)  i= n-1;
    if (i>=n) i= 0;
  } else {
    i= 0;
  }

  dataChange= true;
  plotCrossection= nameList[i];

  return plotCrossection;
}


miTime VcrossManager::setTime(int step)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setTime   step=" << step << endl;
#endif

  if (timeList.size()==0)
    return miTime::nowTime();

  int n= timeList.size();
  int i= 0;
  while (i<n && timeList[i]!=plotTime) i++;
  if (i<n) {
    i+=step;
    if (i<0)  i= n-1;
    if (i>=n) i= 0;
  } else {
    i= 0;
  }

  dataChange= true;
  plotTime= timeList[i];

  return plotTime;
}


// start hardcopy
void VcrossManager::startHardcopy(const printOptions& po){
  if (hardcopy && hardcopystarted){
    // if hardcopy in progress and same filename: make new page
    if (po.fname == printoptions.fname){
      VcrossPlot::startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    VcrossPlot::endPSoutput();
  }
  hardcopy= true;
  printoptions= po;
  hardcopystarted= false;
}

// end hardcopy plot
void VcrossManager::endHardcopy(){
  if (hardcopy) VcrossPlot::endPSoutput();
  hardcopy= false;
}


bool VcrossManager::plot()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::plot" << endl;
#endif

  if (dataChange) {
    preparePlot();
    dataChange= false;
  }

  int numplot= selectedModels.size();

  VcrossPlot::startPlot(numplot,vcopt);

  if (plotCrossection.empty() || numplot==0)
    return true;

  // postscript output
  //if (hardcopy) VcrossPlot::startPSoutput(printoptions);
  if (hardcopy && !hardcopystarted){
    VcrossPlot::startPSoutput(printoptions);
    hardcopystarted= true;
  }

#ifdef DEBUGPRINT
  cerr << "   plot  c=" << plotCrossection << "  t=" << plotTime << endl;
#endif

  bool plotShaded[2]= { true, false };
  int jback= -1;

  for (int p=0; p<2; p++) {
    for (int i=0; i<numplot; i++) {
      int j= selectedVcData[i];
      if (j>=0 && selectedPlotShaded[i]==plotShaded[p]) {
        if (jback<0) jback= j;
        vcdata[j].vcplot->plot(vcopt,selectedFields[i],
            selectedPlotOptions[i]);
      }
    }
  }

  if (jback>=0) vcdata[jback].vcplot->plotBackground(selectedLabel);

  if (vcopt->pText) VcrossPlot::plotText();

#ifdef DEBUGPRINT
  cerr << "VcrossManager::plot finished" << endl;
#endif
  return true;
}


void VcrossManager::preparePlot()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::preparePlot" << endl;
#endif

  int numplot= selectedModels.size();

  vector<VcrossData> prevdata= vcdata;
  int j,n,m= prevdata.size();
  vector<bool> prevUsed(m,false);
  vcdata.clear();

  map<miString,miString>::iterator p, pend=filenames.end();
  map<miString,VcrossFile*>::iterator vf, vfend=vcfiles.end();

  for (int i=0; i<numplot; i++) {

    selectedVcData[i]= -1;

    miString model= selectedModels[i];
    p= filenames.find(model);

    if (p!=pend) {

      miString filename= p->second;
      vf= vcfiles.find(filename);

      if (vf!=vfend && vf->second) {

        VcrossFile *vcfile= vf->second;

        miTime t= plotTime;
        if (selectedHourOffset[i]!=0)
          t.addHour(selectedHourOffset[i]);

        n= vcdata.size();
        j= 0;
        while (j<n && (vcdata[j].filename!=filename ||
            vcdata[j].crossection!=plotCrossection ||
            vcdata[j].tgpos!=timeGraphPos ||
            vcdata[j].time!=t)) j++;

        if (j<n) {

          selectedVcData[i]= j;

        } else {

          j= 0;
          while (j<m && (prevdata[j].filename!=filename ||
              prevdata[j].crossection!=plotCrossection ||
              prevdata[j].tgpos!=timeGraphPos ||
              prevdata[j].time!=t)) j++;

          if (j<m) {
            selectedVcData[i]= vcdata.size();
            vcdata.push_back(prevdata[j]);
            prevUsed[j]= true;
          } else {
            VcrossPlot *vcp= vcfile->getCrossection(plotCrossection,t,
                timeGraphPos);
            if (vcp) {
              timeGraphPosMax= vcp->getHorizontalPosNum() - 1;
              selectedVcData[i]= vcdata.size();
              VcrossData vcd;
              vcd.filename= filename;
              vcd.crossection= plotCrossection;
              vcd.tgpos= timeGraphPos;
              vcd.time= t;
              vcd.vcplot= vcp;
              vcdata.push_back(vcd);
            }
          }
        }
      } else if (filetypes[model] == "GribFile") {
        miTime t = plotTime;
        if (selectedHourOffset[i] != 0)
          t.addHour(selectedHourOffset[i]);
        VcrossField *vcfield = vcfields[model];
        if(vcfield) {
          VcrossPlot *vcp = vcfield->getCrossection(plotCrossection,t,timeGraphPos);
          if (vcp) {
            timeGraphPosMax = vcp->getHorizontalPosNum() - 1;
            selectedVcData[i] = vcdata.size();
            VcrossData vcd;
            vcd.filename = model;
            vcd.crossection = plotCrossection;
            vcd.tgpos = timeGraphPos;
            vcd.time = t;
            vcd.vcplot = vcp;
            vcdata.push_back(vcd);
          }
        }
      }
    }
  }

  for (int j=0; j<m; j++)
    if (!prevUsed[j])
      delete prevdata[j].vcplot;

  if (m>0 && vcdata.size()>0) {
    if (prevdata[0].crossection!=vcdata[0].crossection ||
        (prevdata[0].tgpos<0 && vcdata[0].tgpos>=0) ||
        (prevdata[0].tgpos>=0 && vcdata[0].tgpos<0))
      VcrossPlot::standardPart();
  }

}


vector<miString> VcrossManager::getAllModels()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::getAllModels" << endl;
#endif
  return modelnames;
}


map<miString,miString> VcrossManager::getAllFieldOptions()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::getAllFieldOptions" << endl;
#endif

  return VcrossPlot::getAllFieldOptions();
}


vector<miString> VcrossManager::getFieldNames(const miString& model)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::getFields  model= " << model << endl;
  cerr << "filetypes[model]=" << filetypes[model] << endl;
#endif

  vector<miString> empty;

  if(filetypes[model] == "standard") {
  map<miString,miString>::iterator p= filenames.find(model);
  if (p==filenames.end())
    return empty;

  map<miString,VcrossFile*>::iterator vf= vcfiles.find(p->second);
  if (vf!=vcfiles.end()) {
    if (!vf->second) {
      VcrossFile *vcfile= new VcrossFile(p->second,model);
      if (!vcfile->update()) {
        delete vcfile;
        return empty;
      }
      vf->second= vcfile;
    } else if (!vf->second->update()) {
      return empty;
    }

  } else {
    return empty;
  }

  return VcrossPlot::getFieldNames(p->second);
 } else if(filetypes[model] == "GribFile") {
    VcrossField *vcfield = new VcrossField(model,fieldm);
    if(vcfield->getInventory()) {
      vcfields[model] = vcfield;
      return VcrossPlot::getFieldNames(model);
    } else {
      cerr << "Error making inventory of model " << model << endl;
      return empty;
    }
  } else {
    return empty;
  }
  return empty;
}

/***************************************************************************/

void VcrossManager::getCrossections(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr << "VcrossManager::getCrossections" << endl;
#endif

  if (masterFile.exists()) {

    map<miString,VcrossFile*>::iterator vf= vcfiles.find(masterFile);

    if (vf!=vcfiles.end()) {

      Projection pgeo;
      pgeo.setGeographic();
      Rectangle rgeo(0,0,90,360);
      Area geoArea(pgeo,rgeo);

      miString annot= "Vertikalsnitt";
      int m= usedModels.size();
      for (int i=0; i<m; i++)
        annot+=(" "+usedModels[i]);

      vf->second->getMapData(locationdata.elements);

      locationdata.name=         "vcross";
      locationdata.locationType= location_line;
      locationdata.area=         geoArea;
      locationdata.annotation=   annot;
      locationdata.colour=            vcopt->vcOnMapColour;
      locationdata.linetype=          vcopt->vcOnMapLinetype;
      locationdata.linewidth=         vcopt->vcOnMapLinewidth;
      locationdata.colourSelected=    vcopt->vcSelectedOnMapColour;
      locationdata.linetypeSelected=  vcopt->vcSelectedOnMapLinetype;
      locationdata.linewidthSelected= vcopt->vcSelectedOnMapLinewidth;
    }
    else {
      map<miString, VcrossField*>::iterator vfi = vcfields.find(masterFile);
      if (vfi != vcfields.end()) {
#ifdef DEBUGPRINT
        cerr << "Found masterFile in vcfields" << endl;
#endif

        miString annot = "Vertikalsnitt";
        int m = usedModels.size();
        for (int i = 0; i < m; i++)
          annot += (" " + usedModels[i]);

	Projection pgeo;
	pgeo.setGeographic();
	Rectangle rgeo(0,0,90,360);
	Area geoArea(pgeo,rgeo);

        vfi->second->getMapData(locationdata.elements);
        locationdata.name = "vcross";
        locationdata.locationType = location_line;
        locationdata.area = geoArea;
        locationdata.annotation = annot;
        locationdata.colour = vcopt->vcOnMapColour;
        locationdata.linetype = vcopt->vcOnMapLinetype;
        locationdata.linewidth = vcopt->vcOnMapLinewidth;
        locationdata.colourSelected = vcopt->vcSelectedOnMapColour;
        locationdata.linetypeSelected = vcopt->vcSelectedOnMapLinetype;
        locationdata.linewidthSelected = vcopt->vcSelectedOnMapLinewidth;
      }
    }
  }

  // remember crossection
  if (!plotCrossection.empty()) lastCrossection = plotCrossection;
#ifdef DEBUGPRINT
  cerr << "lastCrossection: "  << lastCrossection << endl;
#endif
  //if it's the first time, plotCrossection is first in list
  if (lastCrossection.empty() && nameList.size()) {
    plotCrossection=nameList[0];
  } else {
    int n = nameList.size();
    bool found = false;
    //find plot crossection
    for (int i=0;i<n;i++){
      if(nameList[i]== lastCrossection){
        plotCrossection=nameList[i];
        found=true;
      }
    }
    if (!found) plotCrossection.clear();
  }
#ifdef DEBUGPRINT
  cerr <<"plotCrossection: " << plotCrossection << endl;
#endif
}

/***************************************************************************/

void VcrossManager::getCrossectionOptions(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr << "VcrossManager::getCrossections" << endl;
#endif
  locationdata.name=              "vcross";
  locationdata.colour=            vcopt->vcOnMapColour;
  locationdata.linetype=          vcopt->vcOnMapLinetype;
  locationdata.linewidth=         vcopt->vcOnMapLinewidth;
  locationdata.colourSelected=    vcopt->vcSelectedOnMapColour;
  locationdata.linetypeSelected=  vcopt->vcSelectedOnMapLinetype;
  locationdata.linewidthSelected= vcopt->vcSelectedOnMapLinewidth;
}


void VcrossManager::mainWindowTimeChanged(const miTime& time)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::mainWindowTimeChanged  " << time << endl;
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


bool VcrossManager::setSelection(const vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setSelection" << endl;
#endif

  //save plotStrings
  plotStrings = vstr;

  selectedModels.clear();
  selectedFields.clear();
  selectedHourOffset.clear();
  selectedPlotOptions.clear();
  selectedPlotShaded.clear();
  selectedVcFile.clear();
  selectedVcData.clear();
  selectedLabel.clear();

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    cerr<<vstr[i]<<endl;

    vector<miString> vs1= vstr[i].split(' ');
    int m= vs1.size();
    if( m>1 && vs1[1].upcase()=="LABEL"){
      selectedLabel.push_back(vstr[i]);
    }
    miString model,field,options;
    int hourOffset= 0;
    bool plotShaded= false;
    for (int j=0; j<m; j++) {
      vector<miString> vs2= vs1[j].split('=');
      if (vs2.size()==2) {
        miString key= vs2[0].downcase();
        if (key=="model") {
          model=vs2[1];
        } else if (key=="field") {
          field=vs2[1];
        } else if (key=="hour.offset") {
          hourOffset= atoi(vs2[1].c_str());
        } else {
          options+= (vs1[j] + " ");
          if (key=="palettecolours") {
            if (vs2[1] != "off") plotShaded= true;
          }
        }
      }
    }
    if (model.exists() && field.exists() &&
        ((filenames.find(model)!=filenames.end())
	 || (filetypes[model] == "GribFile"))) {
      // there may be options not handled in dialog
      // or uncomplete batch input
      miString defaultOptions= VcrossPlot::getFieldOptions(field);
      if (defaultOptions.exists()) {
        PlotOptions poptions;
        PlotOptions::parsePlotOption(defaultOptions,poptions);
        PlotOptions::parsePlotOption(options,poptions);
        selectedModels.push_back(model);
        selectedFields.push_back(field);
        selectedHourOffset.push_back(hourOffset);
        selectedPlotOptions.push_back(poptions);
        selectedPlotShaded.push_back(plotShaded);
        selectedVcFile.push_back(filenames[model]);
        selectedVcData.push_back(-1);
      }
    }
  }

  dataChange= true;

  return setModels();
}


bool VcrossManager::setModels()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setModels" << endl;
#endif

  int numplot= selectedModels.size();

  map<miString,VcrossFile*>::iterator vf, vfend=vcfiles.end();
  set<miString> vcfilesUsed;

  miString prevmasterFile= masterFile;

  VcrossFile *vcfile1= 0;
  VcrossField *vcfield = 0;
  masterFile.clear();
  usedModels.clear();

  for (int i=0; i<numplot; i++) {

    miString model=    selectedModels[i];
    miString filename= selectedVcFile[i];

     if (filetypes[model] == "GribFile") {
      vcfield = vcfields[model];
      int m = usedModels.size();
      int j = 0;
      while (j < m && usedModels[j] != model)
        j++;
      if (j == m)
        usedModels.push_back(model);
      masterFile = model;
    } else if (vcfilesUsed.find(filename)==vcfilesUsed.end()) {

      vcfilesUsed.insert(filename);

      vf= vcfiles.find(filename);

      if (vf!=vfend) {

        VcrossFile *vcfile= 0;

        if (vf->second) {
          vcfile= vf->second;
          if (!vcfile->update()) {
            delete vcfile;
            vcfile= vf->second= 0;
          }
        } else {
          vcfile= new VcrossFile(filename,model);
          if (vcfile->update()) {
            vf->second= vcfile;
          } else {
            delete vcfile;
            vcfile= 0;
          }
        }

        if (vcfile) {
          if (!vcfile1) {
            vcfile1= vcfile;
            masterFile= filename;
          }
          int m= usedModels.size();
          int j= 0;
          while (j<m && usedModels[j]!=model) j++;
          if (j==m) usedModels.push_back(model);
        }
      }
    }
  }

  // delete unused files (from active list), header information
  set<miString>::iterator uend= vcfilesUsed.end();

  for (vf=vcfiles.begin(); vf!=vfend; vf++) {
    if (vf->second) {
      if (vcfilesUsed.find(vf->first)==uend) {
        delete vf->second;
        vf->second= 0;
      }
    }
  }

  bool modelChange= false;

  if (masterFile.empty()) {
    nameList.clear();
    timeList.clear();
    modelChange= prevmasterFile.exists();
  } else if (masterFile!=prevmasterFile) {
    // lists from the first model only, yet...
    if(vcfield) {
      nameList = vcfield->getNames();
      timeList = vcfield->getTimes();
    } else {
      nameList = vcfile1->getNames();
      timeList = vcfile1->getTimes();
    }
    //---------------------------------------------
    if (nameList.size() && timeList.size()) {
      int maxdiff= miTime::minDiff (plotTime,ztime);
      int diff,itime=-1;
      int n= timeList.size();
      for (int i=0; i<n; i++){
        diff= abs(miTime::minDiff(timeList[i],plotTime));
        if(diff<maxdiff){
          maxdiff = diff;
          itime=i;
        }
      }
      if (itime>-1) setTime(timeList[itime]);
      int j= 0;
      if (plotCrossection.exists()) {
        n= nameList.size();
        while (j<n && nameList[j]!=plotCrossection) j++;
        if (j==n) j=0;
      }
      setCrossection(nameList[j]);
    }
    //---------------------------------------------
    modelChange= true;
  }

#ifdef DEBUGPRINT
  cerr << "VcrossManager::setModels finish  modelChange="<<modelChange<<endl;
#endif

  return modelChange;
}


bool VcrossManager::timeGraphOK()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::timeGraphOK" << endl;
#endif

  return (selectedModels.size()>0);
}


void VcrossManager::disableTimeGraph()
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::disableTimeGraph" << endl;
#endif

  timeGraphPos= -1;
  dataChange= true;

  //###############  VcrossPlot::standardPart();
}


void VcrossManager::setTimeGraphPos(int plotx, int ploty)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setTimeGraphPos(x,y)" << endl;
#endif

  int numplot= selectedModels.size();
  for (int i=0; i<numplot; i++) {
    int j= selectedVcData[i];
    if (j>=0) {
      timeGraphPos= vcdata[j].vcplot->getNearestPos(plotx);
      break;
    }
  }

  dataChange= true;

  //###############  VcrossPlot::standardPart();
}


void VcrossManager::setTimeGraphPos(int incr)
{
#ifdef DEBUGPRINT
  cerr << "VcrossManager::setTimeGraphPos(incr)" << endl;
#endif

  int tgp= timeGraphPos;

  timeGraphPos+=incr;

  if (timeGraphPos<0)               timeGraphPos= 0;
  if (timeGraphPos>timeGraphPosMax) timeGraphPos= timeGraphPosMax;

  dataChange= (timeGraphPos!=tgp);
}

void VcrossManager::parseQuickMenuStrings( const vector<miutil::miString>& vstr )
{
  vector<miString> vcross_data, vcross_options;
  miString crossection;

  bool data_exist = false;
  int n = vstr.size();
  for (int i = 1; i < n; i++) {
    miString line = vstr[i];
    line.trim();
    if (!line.exists())
      continue;
    miString upline = line.upcase();

    if (upline.contains("CROSSECTION=")) {
      vector<miString> vs = line.split("=");
      crossection = vs[1];
      if (crossection.contains("\""))
        crossection.remove('\"');
    } else if (upline.contains("VCROSS ")) {
      if (!data_exist)
        vcross_data.clear();
      vcross_data.push_back(line);
      data_exist = true;
    } else {
      // assume plot-options
      vcross_options.push_back(line);
    }
  }

    getOptions()->readOptions(vcross_options);
    setSelection(vcross_data);
    setCrossection(crossection);

}

vector<miutil::miString> VcrossManager::getQuickMenuStrings()
{

  vector<miString> vstr;

  vector<miString> vstrOpt = getOptions()->writeOptions();
  vector<miString> vstrPlot = getPlotStrings();
  miString crossection = "CROSSECTION=" + getCrossection();

  vstr.push_back("VCROSS");
  vstr.insert(vstr.end(),vstrOpt.begin(),vstrOpt.end());
  vstr.insert(vstr.end(),vstrPlot.begin(),vstrPlot.end());
  vstr.push_back(crossection);

  return vstr;
}

vector<miString> VcrossManager::writeLog()
{
  return vcopt->writeOptions();
}


void VcrossManager::readLog(const vector<miString>& vstr,
    const miString& thisVersion,
    const miString& logVersion)
{
  vcopt->readOptions(vstr);
}
