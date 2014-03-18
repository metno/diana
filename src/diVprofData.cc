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

#include "diVprofData.h"
#include "diFtnVfile.h"
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <diField/VcrossData.h>
#include <diField/VcrossUtil.h>

#include "vcross_v2/VcrossEvaluate.h"
#include "vcross_v2/VcrossSetup.h"

//#include <boost/make_shared.hpp>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <boost/foreach.hpp>

#define MILOGGER_CATEGORY "diana.VprofData"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;
using namespace vcross;

VprofData::VprofData(const std::string& filename, const std::string& modelname)
: fileName(filename), modelName(modelname),readFromFimex(false), readFromField(false), fieldManager(NULL),
  numPos(0), numTime(0), numParam(0), numLevel(0),
  dataBuffer(0)
{
  METLIBS_LOG_SCOPE();
}


VprofData::~VprofData()
{
  METLIBS_LOG_SCOPE();
  if (dataBuffer)
    delete[] dataBuffer;
}

bool VprofData::readFimex(const std::string& setup_line)
{
  METLIBS_LOG_SCOPE();

  //parameters and computations should be defined in setup
  Setup_p setup = miutil::make_shared<Setup>();
  string_v fields;
  fields.push_back("air_temperature_celsius_ml");
  fields.push_back("dew_point_temperature_celsius_ml");
  fields.push_back("x_wind_ml");
  fields.push_back("y_wind_ml");
  fields.push_back("relative_humidity_ml");
  fields.push_back("upward_air_velocity_ml");
  string_v plots;
  string_v plot_defs;
  for ( size_t i = 0; i < fields.size(); ++i ) {
    plots.push_back(std::string("name=") + fields[i] + std::string(" plot=CONTOUR(") + fields[i] + std::string(")"));
    plot_defs.push_back(modelName + std::string(" ") + fields[i]);
  }

  string_v computations;
  computations.push_back("relative_humidity_ml=rh_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("tdk=tdk_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("air_temperature_celsius_ml=convert_unit(air_temperature_ml,celsius)");
  computations.push_back("dew_point_temperature_celsius_ml=convert_unit(tdk,celsius)");
  string_v sources;
  sources.push_back(setup_line);
  vc_configure(setup, sources, computations, plots);

  collector = miutil::make_shared<Collector>(setup);

  vc_select_plots(collector, plot_defs);

  vcross::Inventory_cp inv = collector->getResolver()->getInventory(modelName);

  std::set<std::string> csLabels;
  vector<station> stations;
  BOOST_FOREACH(vcross::Crossection_cp cs, inv->crossections) {

    if ( cs->points.size() != 1 ) continue;
    csLabels.insert(cs->label);
    station st;
    st.id = "id";
    st.name = cs->label;
    st.lat = cs->points[0].latDeg();
    st.lon = cs->points[0].lonDeg();
    st.height = 0;
    st.barHeight = 0;
    stations.push_back(st);
  }

  BOOST_FOREACH(vcross::Time::timevalue_t time, inv->times.values) {
    validTime.push_back(vcross::util::to_miTime(inv->times.unit, time));
  }

  for (size_t i = 0; i < stations.size(); i++) {
    posName.push_back(stations[i].name);
    obsName.push_back(stations[i].id);
    posLatitude.push_back(stations[i].lat);
    posLongitude.push_back(stations[i].lon);
    posDeltaLatitude.push_back(0.0);
    posDeltaLongitude.push_back(0.0);
    posTemp.push_back(0);
  }

  numPos = posName.size();
  numTime = validTime.size();
  numParam = 6;
  mainText.push_back(modelName);

  validTime.push_back(miTime("2014-03-05 00:00:00"));
  miTime t = validTime[0];
  for (size_t i = 0; i < validTime.size(); i++) {
    forecastHour.push_back(miTime::hourDiff(validTime[i],t));
    progText.push_back(std::string("+" + miutil::from_number(forecastHour[i])));
  }
  readFromFimex = true;
  vProfPlot = 0;

  //return success;
  return true;
}


bool VprofData::readField(std::string type, FieldManager* fieldm)
{
  METLIBS_LOG_DEBUG("++ VprofData::readField  model= " << modelName << " type=" << type << " path=" << fileName);
  FILE *stationfile;
  char line[1024];
  std::string correctFileName = fileName;
  miutil::replace(correctFileName, modelName, "");
  if ((stationfile = fopen(correctFileName.c_str(), "rb")) == NULL) {
    METLIBS_LOG_ERROR("Unable to open file! " << correctFileName);
    return false;
  }
  fieldManager = fieldm;

  vector<std::string> stationVector;
  vector<station> stations;
  std::string miLine;
  while (fgets(line, 1024, stationfile) != NULL) {
    miLine = std::string(line);
    // just skip the first line if present.
    if (miutil::contains(miLine, "obssource"))
      continue;
    if (miutil::contains(miLine, ";"))
    {
      // the new format
      stationVector = miutil::split(miLine, ";", false);
      if (stationVector.size() == 7) {
        station st;
        char stid[10];
        int wmo_block = miutil::to_int(stationVector[0])*1000;
        int wmo_number = miutil::to_int(stationVector[1]);
        int wmo_id = wmo_block + wmo_number;
        sprintf(stid, "%05d", wmo_id);
        st.id = stid;
        st.name = stationVector[2];
        st.lat = miutil::to_double(stationVector[3]);
        st.lon = miutil::to_double(stationVector[4]);
        st.height = miutil::to_int(stationVector[5], -1);
        st.barHeight = miutil::to_int(stationVector[6], -1);
        stations.push_back(st);
      } else {
        if (stationVector.size() == 6)
        {
          station st;
          st.id = stationVector[0];
          st.name = stationVector[1];
          st.lat = miutil::to_double(stationVector[2]);
          st.lon = miutil::to_double(stationVector[3]);
          st.height = miutil::to_int(stationVector[4], -1);
          st.barHeight = miutil::to_int(stationVector[5], -1);
          stations.push_back(st);
        }
        else {
          METLIBS_LOG_ERROR("Something is wrong with: " << miLine);
        }
      }
    }
    else
    {
      // the old format
      stationVector = miutil::split(miLine, ",", false);
      if (stationVector.size() == 7) {
        station st;
        st.id = stationVector[0];
        st.name = stationVector[1];
        st.lat = miutil::to_double(stationVector[2]);
        st.lon = miutil::to_double(stationVector[3]);
        st.height = miutil::to_int(stationVector[4], -1);
        st.barHeight = miutil::to_int(stationVector[5], -1);
        stations.push_back(st);
      } else {
        METLIBS_LOG_ERROR("Something is wrong with: " << miLine);
      }
    }
  }
  for (size_t i = 0; i < stations.size(); i++) {
    posName.push_back(stations[i].name);
    obsName.push_back(stations[i].id);
    posLatitude.push_back(stations[i].lat);
    posLongitude.push_back(stations[i].lon);
    posDeltaLatitude.push_back(0.0);
    posDeltaLongitude.push_back(0.0);
    posTemp.push_back(0);
  }

  bool success = fieldManager->invVProf(modelName, validTime, forecastHour);
  numPos = posName.size();
  numTime = validTime.size();
  numParam = 6;
  mainText.push_back(modelName);
  for (size_t i = 0; i < forecastHour.size(); i++) {
    progText.push_back(std::string("+" + miutil::from_number(forecastHour[i])));
  }
  readFromField = true;
  vProfPlot = 0;

  return success;
  //return true;
}


bool VprofData::readFile() {
  METLIBS_LOG_DEBUG("++ VprofData::readFile  fileName= " << fileName);

  // reading and storing all information and unpacked data

  int bufferlength= 512;

  FtnVfile *vfile= new FtnVfile(fileName, bufferlength);

  int length, ctype;
  int *head=0, *content=0, *posid=0, *tmp=0;

  numPos= numTime= numParam= numLevel= 0;

  bool success= true;

  try {
    vfile->init();
    length= 8;
    head= vfile->getInt(length);
    if (head[0]!=201 || head[1]!=2 || head[2]!=bufferlength)
      throw VfileError();
    length= head[7];
    if (length!=12)
      throw VfileError();
    content= vfile->getInt(length);

    int ncontent= head[7];

    int nlines,nposid;

    bool c1,c2,c3,c4,c5,c6;
    c1= c2= c3= c4= c5= c6= false;

    for (int ic=0; ic<ncontent; ic+=2) {
      ctype=  content[ic];
      length= content[ic+1];

      switch (ctype) {
      case 101:
        if (length<4)
          throw VfileError();
        tmp= vfile->getInt(length);
        numPos=   tmp[0];
        numTime=  tmp[1];
        numParam= tmp[2];
        numLevel= tmp[3];
        if (numPos<1 || numTime<1 || numParam<2 || numLevel<2)
          throw VfileError();
        //if (length>4) prodNum= tmp[4];
        //if (length>5) gridNum= tmp[5];
        //if (length>6) vCoord=  tmp[6];
        //if (length>7) interpol=tmp[7];
        //if (length>8) isurface=tmp[8];
        delete[] tmp;
        tmp= 0;
        c1= true;
        break;

      case 201:
        if (numTime<1)
          throw VfileError();
        tmp= vfile->getInt(5*numTime);
        for (int n=0; n<5*numTime; n+=5) {
          miTime t= miTime(tmp[n],tmp[n+1],tmp[n+2],tmp[n+3],0,0);
          if (tmp[n+4]!=0)
            t.addHour(tmp[n+4]);
          validTime.push_back(t);
          forecastHour.push_back(tmp[n+4]);
        }
        delete[] tmp;  tmp= 0;
        tmp= vfile->getInt(numTime);
        for (int n=0; n<numTime; n++) {
          std::string str= vfile->getString(tmp[n]);
          progText.push_back(str);
        }
        delete[] tmp;
        tmp= 0;
        c2= true;
        break;

      case 301:
        nlines= vfile->getInt();
        tmp= vfile->getInt(2*nlines);
        for (int n=0; n<nlines; n++) {
          std::string str= vfile->getString(tmp[n*2]);
          mainText.push_back(str);
        }
        delete[] tmp;  tmp= 0;
        c3= true;
        break;

      case 401:
        if (numPos<1) throw VfileError();
        tmp= vfile->getInt(numPos);
        for (int n=0; n<numPos; n++) {
          std::string str= vfile->getString(tmp[n]);
          // may have one space at the end (n*2 characters stored in file)
          miutil::trim(str, false, true);
          posName.push_back(str);
        }
        delete[] tmp;  tmp= 0;
        c4= true;
        break;

      case 501:
        if (numPos<1) throw VfileError();
        nposid= vfile->getInt();
        posid= vfile->getInt(2*nposid);
        tmp= vfile->getInt(nposid*numPos);
        for (int n=0; n<nposid; n++) {
          int nn= n*2;
          float scale= powf(10.,posid[nn+1]);
          if (posid[nn]==-1) {
            for (int i=0; i<numPos; i++)
              posLatitude.push_back(scale*tmp[n+i*nposid]);
          } else if (posid[nn]==-2) {
            for (int i=0; i<numPos; i++)
              posLongitude.push_back(scale*tmp[n+i*nposid]);
          } else if (posid[nn]==-3 && posid[nn+2]==-4) {
            for (int i=0; i<numPos; i++)
              posTemp.push_back(tmp[n+i*nposid]*1000 + tmp[n+1+i*nposid]);
          } else if (posid[nn]==-5) {
            for (int i=0; i<numPos; i++)
              posDeltaLatitude.push_back(scale*tmp[n+i*nposid]);
          } else if (posid[nn]==-6) {
            for (int i=0; i<numPos; i++)
              posDeltaLongitude.push_back(scale*tmp[n+i*nposid]);
          }
        }
        delete[] posid;  posid= 0;
        delete[] tmp;  tmp= 0;
        if (int(posTemp.size())==numPos) {
          for (int i=0; i<numPos; i++) {
            if (posTemp[i]>1000 && posTemp[i]<99000) {
              std::ostringstream ostr;
              ostr << setw(5) << setfill('0') << posTemp[i];
              obsName.push_back(std::string(ostr.str()));
            } else if (posTemp[i]>=99000 && posTemp[i]<=99999) {
              obsName.push_back("99");
            } else {
              obsName.push_back("");
            }
          }
        }
        c5= true;
        break;

      case 601:
        if (numParam<1) throw VfileError();
        tmp= vfile->getInt(numParam);
        for (int n=0; n<numParam; n++)
          paramId.push_back(tmp[n]);
        delete[] tmp;  tmp= 0;
        tmp= vfile->getInt(numParam);
        for (int n=0; n<numParam; n++)
          paramScale.push_back(powf(10.,tmp[n]));
        delete[] tmp;  tmp= 0;
        c6= true;
        break;

      default:
        throw VfileError();
      }
    }

    delete[] head;     head= 0;
    delete[] content;  content= 0;

    if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6)
      throw VfileError();

    // read all data, but keep data in a short int buffer until used
    length= numPos*numTime*numParam*numLevel;

    // dataBuffer[numPos][numTime][numParam][numLevel]
    dataBuffer= vfile->getShortInt(length);

  }  // end of try

  catch (...) {
    METLIBS_LOG_ERROR("Bad Vprof file: " << fileName);
    success= false;
  }

  delete vfile;
  if (head)    delete[] head;
  if (content) delete[] content;
  if (posid)   delete[] posid;
  if (tmp)     delete[] tmp;

  return success;
}


VprofPlot* VprofData::getData(const std::string& name, const miTime& time) {
  METLIBS_LOG_DEBUG("++ VprofData::getData  " << name << "  " << time
      << "  " << modelName);



  VprofPlot *vp= 0;

  int iPos=0;

  while (iPos<numPos && posName[iPos]!=name) iPos++;

  int iTime=0;
  while (iTime<numTime && validTime[iTime]!=time) iTime++;

  if (iPos==numPos || iTime==numTime) return vp;

  vp= new VprofPlot();

  vp->text.index= -1;
  vp->text.prognostic= true;
  vp->text.modelName= modelName;
  vp->text.posName= posName[iPos];
  vp->text.forecastHour= forecastHour[iTime];
  vp->text.validTime= validTime[iTime];
  vp->text.latitude= posLatitude[iPos];
  vp->text.longitude= posLongitude[iPos];
  vp->text.kindexFound= false;

  vp->prognostic= true;
  vp->maxLevels= numLevel;
  vp->windInKnots = true;

  //####  ostringstream ostr;
  //####  ostr << " (" << setiosflags(ios::showpos) << forecastHour[iTime] << ") ";
  //####
  //####  vp->text= modelName + " " + posName[iPos]
  //####	   + ostr.str() + validTime[iTime].isoTime();

  size_t k;

  if (readFromFimex) {

    LonLat pos = LonLat::fromDegrees(posLongitude[iPos],posLatitude[iPos]);
    const Time user_time(util::from_miTime(time));

    model_values_m model_values = vc_fetch_pointValues(collector, pos, user_time);

    const EvaluatedPlot_cpv evaluated_plots = vc_evaluate_plots(collector, model_values);

    //If air_temperature or dew_point_temperature are missing, VprofPlot will crash
    if ( (evaluated_plots[0]->name() != "air_temperature_celsius_ml" ) ||
        (evaluated_plots[1]->name() != "dew_point_temperature_celsius_ml" )) {
          return vp;
        }

    for (size_t i=0; i<evaluated_plots[0]->values(0)->nlevel(); ++i){
      vp->tt.push_back(evaluated_plots[0]->values(0)->value(0,i));
      vp->ptt.push_back(evaluated_plots[0]->z_values->value(0,i));
    }
    for (size_t i=0; i<evaluated_plots[1]->values(0)->nlevel(); ++i){
      vp->td.push_back(evaluated_plots[1]->values(0)->value(0,i));
    }

    size_t index = 2;
    if( index < evaluated_plots.size() && evaluated_plots[index]->name() == "x_wind_ml" ) {
      for (size_t i=0; i<evaluated_plots[index]->values(0)->nlevel(); ++i){
        vp->uu.push_back(evaluated_plots[index]->values(0)->value(0,i));
      }
      ++index;
    }
    if( index < evaluated_plots.size() && evaluated_plots[index]->name() == "y_wind_ml" ) {
      for (size_t i=0; i<evaluated_plots[index]->values(0)->nlevel(); ++i){
        vp->vv.push_back(evaluated_plots[index]->values(0)->value(0,i));
      }
      ++index;
    }
    if( index < evaluated_plots.size() && evaluated_plots[index]->name() == "relative_humidity_ml" ) {
      for (size_t i=0; i<evaluated_plots[index]->values(0)->nlevel(); ++i){
        vp->rhum.push_back(evaluated_plots[index]->values(0)->value(0,i));
      }
      ++index;
    }
    if( index < evaluated_plots.size() && evaluated_plots[index]->name() == "upward_air_velocity_ml" ) {
      for (size_t i=0; i<evaluated_plots[index]->values(0)->nlevel(); ++i){
        vp->om.push_back(evaluated_plots[index]->values(0)->value(0,i)/100);
      }
      ++index;
    }

    vp->windInKnots = false;
    numLevel = vp->ptt.size();
    vp->maxLevels= numLevel;

  } else if (readFromField) {

    if((name == vProfPlotName) && (time == vProfPlotTime)) {
      METLIBS_LOG_DEBUG("returning cached VProfPlot");
      for (k=0; k<vProfPlot->ptt.size(); k++)
        vp->ptt.push_back(vProfPlot->ptt[k]);
      for (k=0; k<vProfPlot->tt.size(); k++)
        vp->tt.push_back(vProfPlot->tt[k]);
      for (k=0; k<vProfPlot->ptd.size(); k++)
        vp->ptd.push_back(vProfPlot->ptd[k]);
      for (k=0; k<vProfPlot->td.size(); k++)
        vp->td.push_back(vProfPlot->td[k]);
      for (k=0; k<vProfPlot->puv.size(); k++)
        vp->puv.push_back(vProfPlot->puv[k]);
      for (k=0; k<vProfPlot->uu.size(); k++)
        vp->uu.push_back(vProfPlot->uu[k]);
      for (k=0; k<vProfPlot->vv.size(); k++)
        vp->vv.push_back(vProfPlot->vv[k]);
      for (k=0; k<vProfPlot->om.size(); k++)
        vp->om.push_back(vProfPlot->om[k]);
      for (k=0; k<vProfPlot->pom.size(); k++)
        vp->pom.push_back(vProfPlot->pom[k]);
    } else {
      bool success = fieldManager->makeVProf(modelName,validTime[iTime],posLatitude[iPos],posLongitude[iPos],
          vp->tt,vp->ptt,vp->td,vp->ptd,vp->uu,vp->vv,vp->puv,vp->om,vp->pom);
      if (!success) {
        delete vp;
        return NULL;
      }

      numLevel = vp->tt.size();
      vp->maxLevels = numLevel;

      vProfPlotTime = miTime(time);
      vProfPlotName = std::string(name);
      if(vProfPlot) delete vProfPlot;
      vProfPlot = new VprofPlot();
      for (k=0; k<vp->ptt.size(); k++)
        vProfPlot->ptt.push_back(vp->ptt[k]);
      for (k=0; k<vp->tt.size(); k++)
        vProfPlot->tt.push_back(vp->tt[k]);
      for (k=0; k<vp->ptd.size(); k++)
        vProfPlot->ptd.push_back(vp->ptd[k]);
      for (k=0; k<vp->td.size(); k++)
        vProfPlot->td.push_back(vp->td[k]);
      for (k=0; k<vp->puv.size(); k++)
        vProfPlot->puv.push_back(vp->puv[k]);
      for (k=0; k<vp->uu.size(); k++)
        vProfPlot->uu.push_back(vp->uu[k]);
      for (k=0; k<vp->vv.size(); k++)
        vProfPlot->vv.push_back(vp->vv[k]);
      for (k=0; k<vp->om.size(); k++)
        vProfPlot->om.push_back(vp->om[k]);
      for (k=0; k<vp->pom.size(); k++)
        vProfPlot->pom.push_back(vp->pom[k]);
    }
    //iPos = 0;
    //iTime = 0;
    for (k=0; k<vp->ptt.size(); k++)
      METLIBS_LOG_DEBUG("ptt["<<k<<"]: " <<vp->ptt[k]);
    for (k=0; k<vp->tt.size(); k++)
      METLIBS_LOG_DEBUG("tt["<<k<<"]: " <<vp->tt[k]);
    for (k=0; k<vp->ptd.size(); k++)
      METLIBS_LOG_DEBUG("ptd["<<k<<"]: " <<vp->ptd[k]);
    for (k=0; k<vp->td.size(); k++)
      METLIBS_LOG_DEBUG("td["<<k<<"]: " <<vp->td[k]);
    for (k=0; k<vp->puv.size(); k++)
      METLIBS_LOG_DEBUG("puv["<<k<<"]: " <<vp->puv[k]);
    for (k=0; k<vp->uu.size(); k++)
      METLIBS_LOG_DEBUG("uu["<<k<<"]: " <<vp->uu[k]);
    for (k=0; k<vp->vv.size(); k++)
      METLIBS_LOG_DEBUG("vv["<<k<<"]: " <<vp->vv[k]);
    for (k=0; k<vp->om.size(); k++)
      METLIBS_LOG_DEBUG("om["<<k<<"]: " <<vp->om[k]);
    for (k=0; k<vp->pom.size(); k++)
      METLIBS_LOG_DEBUG("pom["<<k<<"]: " <<vp->pom[k]);
  } else {

    int j,k,n;
    float scale;

    for (n=0; n<numParam; n++) {
      j= iPos*numTime*numParam*numLevel + iTime*numParam*numLevel + n*numLevel;
      scale= paramScale[n];
      if (paramId[n]==8) {
        for (k=0; k<numLevel; k++)
          vp->ptt.push_back(scale*dataBuffer[j++]);
      } else if (paramId[n]==4) {
        for (k=0; k<numLevel; k++)
          vp->tt.push_back(scale*dataBuffer[j++]);
      } else if (paramId[n]==5) {
        for (k=0; k<numLevel; k++)
          vp->td.push_back(scale*dataBuffer[j++]);
      } else if (paramId[n]==2) {
        for (k=0; k<numLevel; k++)
          vp->uu.push_back(scale*dataBuffer[j++]);
      } else if (paramId[n]==3) {
        for (k=0; k<numLevel; k++)
          vp->vv.push_back(scale*dataBuffer[j++]);
      } else if (paramId[n]==13) {
        for (k=0; k<numLevel; k++)
          vp->om.push_back(scale*dataBuffer[j++]);
      }
    }
    for (k=0; k<numLevel; k++) {
      METLIBS_LOG_DEBUG("ptt["<<k<<"]" <<vp->ptt[k]);
      METLIBS_LOG_DEBUG("tt["<<k<<"]" <<vp->tt[k]);
      METLIBS_LOG_DEBUG("td["<<k<<"]" <<vp->td[k]);
      METLIBS_LOG_DEBUG("uu["<<k<<"]" <<vp->uu[k]);
      METLIBS_LOG_DEBUG("vv["<<k<<"]" <<vp->vv[k]);
      METLIBS_LOG_DEBUG("om["<<k<<"]" <<vp->om[k]);
    }
  }


  // dd,ff and significant levels (as in temp observation...)
  if (int(vp->uu.size())==numLevel && int(vp->vv.size())==numLevel) {
    float degr= 180./3.141592654;
    float uew,vns;
    int dd,ff;
    int kmax= 0;
    for (k = 0; k < size_t(numLevel); k++) {
      uew= vp->uu[k];
      vns= vp->vv[k];
      ff= int(sqrtf(uew*uew+vns*vns) + 0.5);
      dd= int(270.-degr*atan2f(vns,uew) + 0.5);
      if (dd>360)
        dd-=360;
      if (dd<= 0)
        dd+=360;
      if (ff==0) dd= 0;
      vp->dd.push_back(dd);
      vp->ff.push_back(ff);
      vp->sigwind.push_back(0);
      if (ff>vp->ff[kmax]) kmax=k;
    }
    for (size_t l = 0; l < (vp->sigwind.size()); l++){
      for (k = 1; k < size_t(numLevel) - 1; k++) {
        if (vp->ff[k] < vp->ff[k - 1] && vp->ff[k] < vp->ff[k + 1])
          vp->sigwind[k] = 1;
        if (vp->ff[k] > vp->ff[k - 1] && vp->ff[k] > vp->ff[k + 1])
          vp->sigwind[k] = 2;
      }
    }
    vp->sigwind[kmax]= 3;
  }

  //################################################################
  //METLIBS_LOG_DEBUG("    "<<vp->posName<<"  "<<vp->validTime.isoTime());
  //METLIBS_LOG_DEBUG("           vp->ptt.size()= "<<vp->ptt.size());
  //METLIBS_LOG_DEBUG("           vp->tt.size()=  "<<vp->tt.size());
  //METLIBS_LOG_DEBUG("           vp->ptd.size()= "<<vp->ptd.size());
  //METLIBS_LOG_DEBUG("           vp->td.size()=  "<<vp->td.size());
  //METLIBS_LOG_DEBUG("           vp->puv.size()= "<<vp->puv.size());
  //METLIBS_LOG_DEBUG("           vp->uu.size()=  "<<vp->uu.size());
  //METLIBS_LOG_DEBUG("           vp->vv.size()=  "<<vp->vv.size());
  //METLIBS_LOG_DEBUG("           vp->pom.size()= "<<vp->pom.size());
  //METLIBS_LOG_DEBUG("           vp->om.size()=  "<<vp->om.size());
  //METLIBS_LOG_DEBUG("           vp->dd.size()=  "<<vp->dd.size());
  //METLIBS_LOG_DEBUG("           vp->ff.size()=  "<<vp->ff.size());
  //METLIBS_LOG_DEBUG("           vp->sigwind.size()=  "<<vp->sigwind.size());
  //################################################################

  return vp;
}
