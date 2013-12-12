/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

#include "diVcross1Manager.h"

#include "diVcross1Options.h"
#include "diVcross1File.h"
#include "diVcross1Field.h"
#include "diVcross1Plot.h"

#include <diLocalSetupParser.h>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>
#include <puCtools/puCglob.h>
#include <diField/diFieldManager.h>

#include <boost/foreach.hpp>

#include <set>

#define MILOGGER_CATEGORY "diana.VcrossManager"
#include <miLogger/miLogging.h>

using namespace std;
using miutil::miTime;

VcrossManager::VcrossManager(Controller *co)
: dataChange(true), timeGraphPos(-1) , timeGraphPosMax(-1), hardcopy(false)
{
  METLIBS_LOG_SCOPE();

  fieldm= co->getFieldManager(); // set fieldmanager

  vcopt= new VcrossOptions();  // defaults are set

  parseSetup();

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);

  plotTime= miTime::nowTime();
}


VcrossManager::~VcrossManager()
{
  METLIBS_LOG_SCOPE();

  cleanup();
  delete vcopt;
}


void VcrossManager::cleanup()
{
  METLIBS_LOG_SCOPE();

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

  BOOST_FOREACH(VcrossData& d, vcdata)
      delete d.vcplot;
  vcdata.clear();

  BOOST_FOREACH(VcrossFile*& f, miutil::adaptors::values(vcfiles)) {
      delete f;
      f = 0;
  }

  BOOST_FOREACH(VcrossField*& f, miutil::adaptors::values(vcfields))
      delete f;
  vcfields.clear();

  // NOTE: Flush the field cache
  fieldm->fieldcache->flush();

}

void VcrossManager::cleanupDynamicCrossSections()
{
  METLIBS_LOG_SCOPE();
  BOOST_FOREACH(VcrossField* f, miutil::adaptors::values(vcfields))
      if (f)
        f->cleanup();
}

bool VcrossManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  const std::string section1 = "VERTICAL_CROSSECTION_FILES";

  vector<std::string> vstr;

  bool ok= true;

  if (miutil::SetupParser::getSection(section1,vstr)) {

    std::string model,filename;
    vector<std::string> tokens,tokens1,tokens2;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= miutil::split(vstr[i]);
      if (tokens.size()==2) {
        tokens1= miutil::split(tokens[0], 0, "=");
        tokens2= miutil::split(tokens[1], 0, "=");
        if (tokens1.size()==2 && tokens2.size()==2) {
	  if (miutil::to_lower(tokens1[0])=="m" && miutil::to_lower(tokens2[0])=="f") {
	    model= tokens1[1];
	    filename= tokens2[1];
	    filenames[model]= filename;
	    modelnames.push_back(model);
	    filetypes[model] = "standard";
	    vcfiles[filename]= 0;
	  } else if (miutil::to_lower(tokens1[0]) == "m" && miutil::to_lower(tokens2[0]) == "t") {
            model = tokens1[1];
            std::string filetype = tokens2[1];
	    modelnames.push_back(model);
            filetypes[model] = filetype;
	    vcfiles[filename]= 0;
          }
	}
      }
    }
  } else {
    //METLIBS_LOG_DEBUG("Missing section " << section1 << " in setupfile.");
    //ok= false;
  }

  // parse remaining setup sections
  if (!VcrossPlot::parseSetup())
    ok = false;

  return ok;
}


vector< vector<Colour::ColourInfo> > VcrossManager::getMultiColourInfo(int multiNum)
{
  return LocalSetupParser::getMultiColourInfo(multiNum);
}


void VcrossManager::setPlotWindow(int xs, int ys)
{
  VcrossPlot::setPlotWindow(xs, ys);
}

void VcrossManager::setCrossection(const std::string& crossection)
{
  METLIBS_LOG_SCOPE();

  plotCrossection= crossection;
  dataChange= true;
}

bool VcrossManager::setCrossection(float lat, float lon)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(lat) << LOGVAL(lon));

  if (vcfields.empty())
    return false;
  BOOST_FOREACH(VcrossField* f, miutil::adaptors::values(vcfields)) {
    f->setLatLon(lat, lon);
    // Get the new namelist with the crossections
    nameList = f->getNames();
    timeList = f->getTimes();
  }
  return true;
}

void VcrossManager::setTime(const miTime& time)
{
  METLIBS_LOG_SCOPE();

  plotTime= time;
  dataChange= true;
}


std::string VcrossManager::setCrossection(int step)
{
  METLIBS_LOG_SCOPE();

  if (nameList.empty())
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
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(step));

  if (timeList.empty())
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
void VcrossManager::startHardcopy(const printOptions& po)
{
  if (hardcopy && hardcopystarted) {
    // if hardcopy in progress and same filename: make new page
    if (po.fname == printoptions.fname){
      VcrossPlot::startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    VcrossPlot::endPSoutput();
  }
  hardcopy = true;
  printoptions = po;
  hardcopystarted = false;
}

// end hardcopy plot
void VcrossManager::endHardcopy()
{
  if (hardcopy)
    VcrossPlot::endPSoutput();
  hardcopy= false;
}


bool VcrossManager::plot()
{
  METLIBS_LOG_SCOPE();

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
  if (hardcopy && !hardcopystarted) {
    VcrossPlot::startPSoutput(printoptions);
    hardcopystarted= true;
  }

  METLIBS_LOG_DEBUG("plot  c=" << plotCrossection << "  t=" << plotTime);

  const bool plotShaded[2] = { true, false };
  int jback= -1;

  for (int p=0; p<2; p++) {
    for (int i=0; i<numplot; i++) {
      int j= selectedVcData[i];
      if (j>=0 && selectedPlotShaded[i]==plotShaded[p]) {
        if (jback<0)
          jback= j;
        vcdata[j].vcplot->plot(vcopt,selectedFields[i],
            selectedPlotOptions[i]);
      }
    }
  }

  if (jback>=0)
    vcdata[jback].vcplot->plotBackground(selectedLabel);

  if (vcopt->pText)
    VcrossPlot::plotText();

  return true;
}


void VcrossManager::preparePlot()
{
  METLIBS_LOG_SCOPE();
  
  int numplot= selectedModels.size();

  vector<VcrossData> prevdata= vcdata;
  int j,n,m= prevdata.size();
  vector<bool> prevUsed(m,false);
  vcdata.clear();

  map<std::string,std::string>::iterator p, pend=filenames.end();
  map<std::string,VcrossFile*>::iterator vf, vfend=vcfiles.end();

  for (int i=0; i<numplot; i++) {

    selectedVcData[i]= -1;
    
    std::string model= selectedModels[i];
    p= filenames.find(model);

    if (p!=pend) {

      std::string filename= p->second;
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

        VcrossField* vcfield = getVcrossField(model);
        if (vcfield) {
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
        } else {
          METLIBS_LOG_ERROR("filetype=GribFile but no vcfield!");
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


vector<std::string> VcrossManager::getAllModels()
{
  METLIBS_LOG_SCOPE();
  return modelnames;
}


map<std::string,std::string> VcrossManager::getAllFieldOptions()
{
  METLIBS_LOG_SCOPE();
  return VcrossPlot::getAllFieldOptions();
}


vector<std::string> VcrossManager::getFieldNames(const std::string& model)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(model) << LOGVAL(filetypes[model]));

  const vector<std::string> empty;

  if(filetypes[model] == "standard") {
    map<std::string,std::string>::iterator p= filenames.find(model);
    if (p==filenames.end())
      return empty;
    
    map<std::string,VcrossFile*>::iterator vf= vcfiles.find(p->second);
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
    VcrossField *vcfield = getVcrossField(model);
    if (vcfield)
      return VcrossPlot::getFieldNames(model);
  } else {
    METLIBS_LOG_ERROR("unknown filetype for model " << model);
  }
  return empty;
}

VcrossField* VcrossManager::getVcrossField(const std::string& modelname)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(modelname));

  const std::map<std::string, std::string>::const_iterator fit = filetypes.find(modelname);
  if (fit == filetypes.end() or fit->second != "GribFile") {
    METLIBS_LOG_ERROR("model '" << modelname << "' not registered as 'GribFile'");
    return 0;
  }
  
  const std::map<std::string, VcrossField*>::iterator vfit = vcfields.find(modelname);
  if (vfit == vcfields.end() or not vfit->second) {
    VcrossField *vcfield = new VcrossField(modelname, fieldm);
    if (not vcfield->getInventory()) {
      delete vcfield;
      METLIBS_LOG_ERROR("failed to make inventory of model '" << modelname << "'");
      return 0;
    }
    METLIBS_LOG_DEBUG("inventory for model '" << modelname << "' complete");

    if (vfit == vcfields.end()) {
      vcfields.insert(std::make_pair(modelname, vcfield));
      return vcfield;
    } else {
      vfit->second = vcfield;
    }
  }
  return vfit->second;
}

/***************************************************************************/

void VcrossManager::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  if (not masterFile.empty()) {

    map<std::string,VcrossFile*>::iterator vf= vcfiles.find(masterFile);

    if (vf!=vcfiles.end()) {

      Projection pgeo;
      pgeo.setGeographic();
      Rectangle rgeo(0,0,90,360);
      Area geoArea(pgeo,rgeo);

      std::string annot= "Vertikalsnitt";
      int m= usedModels.size();
      for (int i=0; i<m; i++)
        annot+=(" "+usedModels[i]);

      vf->second->getMapData(locationdata.elements);
      locationdata.locationType= location_line;
      locationdata.area=         geoArea;
      locationdata.annotation=   annot;
      getCrossectionOptions(locationdata);
    }
    else {
      map<std::string, VcrossField*>::iterator vfi = vcfields.find(masterFile);
      if (vfi != vcfields.end()) {
        METLIBS_LOG_DEBUG("Found masterFile in vcfields");

        std::string annot = "Vertikalsnitt";
        int m = usedModels.size();
        for (int i = 0; i < m; i++)
          annot += (" " + usedModels[i]);

        Projection pgeo;
        pgeo.setGeographic();
        Rectangle rgeo(0,0,90,360);
        Area geoArea(pgeo,rgeo);

        vfi->second->getMapData(locationdata.elements);
        locationdata.locationType = location_line;
        locationdata.area = geoArea;
        locationdata.annotation = annot;
        getCrossectionOptions(locationdata);
      }
    }
  }

  // remember crossection
  if (!plotCrossection.empty())
    lastCrossection = plotCrossection;

  METLIBS_LOG_DEBUG("lastCrossection: "  << lastCrossection);

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
}

/***************************************************************************/

void VcrossManager::getCrossectionOptions(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
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
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(time));

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
  if (itime>-1)
    setTime(timeList[itime]);
}


bool VcrossManager::setSelection(const vector<std::string>& vstr)
{
  METLIBS_LOG_SCOPE();

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

    METLIBS_LOG_INFO(vstr[i]);

    vector<std::string> vs1= miutil::split(vstr[i], 0, " ");
    int m= vs1.size();
    if( m>1 && miutil::to_upper(vs1[1])=="LABEL"){
      selectedLabel.push_back(vstr[i]);
    }
    std::string model,field,options;
    int hourOffset= 0;
    bool plotShaded= false;
    for (int j=0; j<m; j++) {
      vector<std::string> vs2= miutil::split(vs1[j], 0, "=");
      if (vs2.size()==2) {
        std::string key= miutil::to_lower(vs2[0]);
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
    if (not model.empty() and not field.empty()
        and ((filenames.find(model) != filenames.end()) or (filetypes[model] == "GribFile")))
    {
      // there may be options not handled in dialog
      // or uncomplete batch input
      std::string defaultOptions= VcrossPlot::getFieldOptions(field);
      if (not defaultOptions.empty()) {
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
  METLIBS_LOG_SCOPE();

  int numplot= selectedModels.size();

  map<std::string,VcrossFile*>::iterator vf, vfend=vcfiles.end();
  set<std::string> vcfilesUsed;

  const std::string prevmasterFile= masterFile;

  VcrossFile *vcfile1= 0;
  VcrossField *vcfield = 0;
  masterFile.clear();
  usedModels.clear();

  for (int i=0; i<numplot; i++) {
    const std::string& model=    selectedModels[i];
    const std::string& filename= selectedVcFile[i];
    METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(model) << LOGVAL(filename));

    if (filetypes[model] == "GribFile") {
      vcfield = getVcrossField(model);
      if (not vcfield) {
        METLIBS_LOG_ERROR("filetype=GribFile but no vcfield!");
        return false;
      }
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
  set<std::string>::iterator uend= vcfilesUsed.end();

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
    modelChange = (not prevmasterFile.empty());
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
      if (not plotCrossection.empty()) {
        n= nameList.size();
        while (j<n && nameList[j]!=plotCrossection) j++;
        if (j==n) j=0;
      }
      setCrossection(nameList[j]);
    }
    //---------------------------------------------
    modelChange= true;
  }

  METLIBS_LOG_DEBUG(LOGVAL(modelChange));
  return modelChange;
}


bool VcrossManager::timeGraphOK()
{
  METLIBS_LOG_SCOPE();
  return (selectedModels.size()>0);
}


void VcrossManager::disableTimeGraph()
{
  METLIBS_LOG_SCOPE();
  timeGraphPos= -1;
  dataChange= true;
}


void VcrossManager::setTimeGraphPos(int plotx, int ploty)
{
  METLIBS_LOG_SCOPE();

  int numplot= selectedModels.size();
  for (int i=0; i<numplot; i++) {
    int j= selectedVcData[i];
    if (j>=0) {
      timeGraphPos= vcdata[j].vcplot->getNearestPos(plotx);
      break;
    }
  }

  dataChange= true;
}


void VcrossManager::setTimeGraphPos(int incr)
{
  METLIBS_LOG_SCOPE();

  int tgp= timeGraphPos;

  timeGraphPos+=incr;

  if (timeGraphPos<0)               timeGraphPos= 0;
  if (timeGraphPos>timeGraphPosMax) timeGraphPos= timeGraphPosMax;

  dataChange= (timeGraphPos!=tgp);
}

void VcrossManager::parseQuickMenuStrings( const vector<std::string>& vstr )
{
  vector<std::string> vcross_data, vcross_options;
  std::string crossection;

  bool data_exist = false;
  int n = vstr.size();
  for (int i = 1; i < n; i++) {
    std::string line = vstr[i];
    miutil::trim(line);
    if (line.empty())
      continue;
    std::string upline = miutil::to_upper(line);

    if (miutil::contains(upline, "CROSSECTION=")) {
      vector<std::string> vs = miutil::split(line, "=");
      crossection = vs[1];
      if (miutil::contains(crossection, "\""))
        miutil::remove(crossection, '\"');
    } else if (miutil::contains(upline, "VCROSS ")) {
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

vector<std::string> VcrossManager::getQuickMenuStrings()
{
  vector<std::string> vstr;

  vector<std::string> vstrOpt = getOptions()->writeOptions();
  vector<std::string> vstrPlot = getPlotStrings();
  std::string crossection = "CROSSECTION=" + getCrossection();

  vstr.push_back("VCROSS");
  vstr.insert(vstr.end(),vstrOpt.begin(),vstrOpt.end());
  vstr.insert(vstr.end(),vstrPlot.begin(),vstrPlot.end());
  vstr.push_back(crossection);

  return vstr;
}

vector<std::string> VcrossManager::writeLog()
{
  return vcopt->writeOptions();
}


void VcrossManager::readLog(const vector<std::string>& vstr,
    const std::string& thisVersion,
    const std::string& logVersion)
{
  vcopt->readOptions(vstr);
}
