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

#ifdef BUFROBS
#include "diObsBufr.h"
#endif // BUFROBS

#ifdef ROADOBS
#include "diObsRoad.h"
#endif // ROADOBS

#include "diLocalSetupParser.h"
#include "diField/VcrossData.h"
#include "diField/VcrossUtil.h"

#include "vcross_v2/VcrossEvaluate.h"
#include "vcross_v2/VcrossComputer.h"
#include "diUtilities.h"
#include "util/charsets.h"
#include "util/math_util.h"

#include <puTools/miStringFunctions.h>
#include <puTools/TimeFilter.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <boost/foreach.hpp>


#define MILOGGER_CATEGORY "diana.VprofData"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;
using namespace vcross;

const char VP_AIR_TEMPERATURE[]       = "vp_air_temperature_celsius";
const char VP_DEW_POINT_TEMPERATURE[] = "vp_dew_point_temperature_celsius";
const char VP_X_WIND[]                = "vp_x_wind_ms";
const char VP_Y_WIND[]                = "vp_y_wind_ms";
const char VP_RELATIVE_HUMIDITY[]     = "vp_relative_humidity";
const char VP_OMEGA[]                 = "vp_omega_pas";

VprofData::VprofData(const std::string& modelname,
    const std::string& stationsfilename) :
        modelName(modelname), stationsFileName(stationsfilename),
        numPos(0), numTime(0), numParam(0), numLevel(0)
      , numRealizations(1)
      , stationList(false)
{
  METLIBS_LOG_SCOPE();
}

VprofData::~VprofData()
{
  METLIBS_LOG_SCOPE();
}

bool VprofData::readRoadObs(const std::string& databasefile, const std::string& parameterfile)
{
  METLIBS_LOG_SCOPE();
#ifdef ROADOBS
  format = roadobs;
  db_parameterfile = parameterfile;
  db_connectfile = databasefile;
  validTime.clear();
  // Due to the fact that we have a database insteda of an archive,
  // we must fake the behavoir of anarchive
  // Assume that all stations report every hour
  // first, get the current time.
  //
  // We assume that tempograms are made 00Z, 06Z, 12Z and 18Z.
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
  try {
    // Dummy filename
    std::string filename;
    // read stationlist and init the api.
    ObsRoad road = ObsRoad(filename,db_connectfile,stationsFileName,db_parameterfile,starttime,NULL,false);
    
  } catch (...) {
      METLIBS_LOG_ERROR("Exception in ObsRoad: " << db_connectfile << "," << stationsFileName << "," << db_parameterfile << "," << starttime);
      return false;
  }
  
  starttime.addDay(-daysback);
  int hourdiff;
  miTime time = now;
  METLIBS_LOG_DEBUG(LOGVAL(time));
  validTime.push_back(time);
  time.addHour(-6);
  while ((hourdiff = miTime::hourDiff(time, starttime)) > 0) {
    METLIBS_LOG_DEBUG(LOGVAL(time));
    validTime.push_back(time);    
    time.addHour(-6);
  }
      
#endif // ROADOBS
  return true;
}

bool VprofData::readBufr(const std::string& modelname, const std::string& pattern_)
{
  METLIBS_LOG_SCOPE();
#ifdef BUFROBS

  format = bufr;
  modelName =modelname;

  validTime.clear();
  fileNames.clear();

  miutil::TimeFilter tf;
  std::string pattern=pattern_;
  tf.initFilter(pattern);
  diutil::string_v matches = diutil::glob(pattern);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    std::string filename = *it;
    miutil::miTime time;
    if (!tf.ok() || !tf.getTime(filename, time)) {
      ObsBufr bufr;
      METLIBS_LOG_DEBUG("TimeFilter not ok"<<LOGVAL(filename));
      if (!bufr.ObsTime(filename, time))
        continue;
    }
    METLIBS_LOG_DEBUG(LOGVAL(filename)<<LOGVAL(time));
    validTime.push_back(time);
    fileNames.push_back(filename);
  }
  vProfPlot.reset(0);
#endif
  return true;

}

bool VprofData::setBufr(const miutil::miTime& plotTime)
{
  METLIBS_LOG_SCOPE();
  currentFiles.clear();
  mStations.clear();
#ifdef BUFROBS
  vector<miTime> tlist;

  int n= validTime.size();
  for (int j=0; j<n; j++) {
    if (validTime[j]==plotTime) {
      ObsBufr bufr;
      currentFiles.push_back(fileNames[j]);
      //TODO: include files with time+-timediff, this is just a hack to include files with time = plottime - 1 hour
      if (j>0 && abs(miTime::minDiff(validTime[j], plotTime)) <= 60) {
        currentFiles.push_back(fileNames[j-1]);
      }
      bufr.readStationInfo(currentFiles, mStations, tlist);
      renameStations();
      break;
    }
  }
#endif

  return true;
}

bool VprofData::setRoadObs(const miutil::miTime& plotTime)
{

#ifdef ROADOBS
  mStations.clear();
  try {
    // Dummy filename
    std::string filename;
    // read stationlist and init the api.
    // does nothing if already done
    ObsRoad road = ObsRoad(filename,db_connectfile,stationsFileName,db_parameterfile,plotTime,NULL,false);
    road.getStationList(mStations);

  } catch (...) {
      METLIBS_LOG_ERROR("Exception in ObsRoad: " << db_connectfile << "," << stationsFileName << "," << db_parameterfile << "," << plotTime);
      return false;
  }
#endif

  return true;
}

bool VprofData::readFimex(vcross::Setup_p setup, const std::string& reftimestr)
{
  METLIBS_LOG_SCOPE();

  collector = std::make_shared<vcross::Collector>(setup);
  vcross::Source_p source = collector->getResolver()->getSource(modelName);
  if (!source)
    return false;


  fields.push_back(VP_AIR_TEMPERATURE);
  fields.push_back(VP_DEW_POINT_TEMPERATURE);
  fields.push_back(VP_X_WIND);
  fields.push_back(VP_Y_WIND);
  fields.push_back(VP_RELATIVE_HUMIDITY);
  fields.push_back(VP_OMEGA);

  if ( reftimestr.empty() ) {
    reftime = source->getLatestReferenceTime();
  } else {
    miTime mt(reftimestr);
    reftime = util::from_miTime(mt);
  }
  const vcross::ModelReftime mr(modelName, reftime);

  for ( size_t j = 0; j < fields.size(); ++j ) {
    METLIBS_LOG_DEBUG(LOGVAL(mr) << LOGVAL(fields[j]));
    collector->requireField(mr, fields[j]);
  }

  vcross::Inventory_cp inv = collector->getResolver()->getInventory(mr);
  if (not inv)
    return false;

  if (!stationsFileName.empty()) {
    readStationNames(stationsFileName);
  } else {
    for (vcross::Crossection_cp cs : inv->crossections) {
      if (cs->length() != 1)
        continue;
      mStations.push_back(stationInfo(cs->label(), cs->point(0).lonDeg(), cs->point(0).latDeg()));
    }
  }

  for (const vcross::Time::timevalue_t& time : inv->times.values) {
    validTime.push_back(vcross::util::to_miTime(inv->times.unit, time));
  }

  numPos = mStations.size();
  numTime = validTime.size();
  numParam = 6;
  numRealizations = inv->realizationCount;

  const miTime rt = util::to_miTime(mr.reftime);
  for (size_t i = 0; i < validTime.size(); i++) {
    forecastHour.push_back(miTime::hourDiff(validTime[i],rt));
  }
  format = fimex;
  vProfPlot.reset(0);
  return true;
}


bool VprofData::updateStationList(const miutil::miTime& plotTime)
{
  if (format == fimex) {
    return std::find(validTime.begin(), validTime.end(), plotTime) != validTime.end();
  } else if (format == roadobs) {
    setRoadObs(plotTime);
    return !mStations.empty();
  } else {
    setBufr(plotTime);
    return !mStations.empty();
  }
}

static void copy_vprof_values(Values_cp values, std::vector<float>& values_out)
{
  Values::ShapeIndex idx(values->shape());
  for (int i=0; i<values->shape().length(0); ++i) {
    idx.set(0, i);
    values_out.push_back(values->value(idx));
  }
}

static void copy_vprof_values(const name2value_t& n2v, const std::string& id, std::vector<float>& values_out)
{
  METLIBS_LOG_SCOPE(LOGVAL(id));
  name2value_t::const_iterator itN = n2v.find(id);
  if (itN == n2v.end() or not itN->second) {
    values_out.clear();
    return;
  }
  copy_vprof_values(itN->second, values_out);
}

VprofPlot* VprofData::getData(const std::string& name, const miTime& time, int realization)
{
  METLIBS_LOG_SCOPE(name << "  " << time << "  " << modelName);
  METLIBS_LOG_DEBUG(LOGVAL(validTime.size()) <<LOGVAL(mStations.size()));

  if (format == bufr) {
    if (name == vProfPlotName and time == vProfPlotTime and vProfPlot.get()) {
      METLIBS_LOG_DEBUG("returning cached VProfPlot");
      return new VprofPlot(*vProfPlot);
    }
#ifdef BUFROBS
    std::string name_=name;
    if (stationMap.count(name)) {
      name_ = stationMap[name];
    }
    ObsBufr bufr;
    std::string modelName;
    std::unique_ptr<VprofPlot> vp(bufr.getVprofPlot(currentFiles, modelName, name_, time));
    if (!vp.get())
      return 0;
    vProfPlotTime = time;
    vProfPlotName = name;
    vProfPlot.reset(new VprofPlot(*vp));
    return vp.release();
#else
    return 0;
#endif
  }

  if (format == roadobs) {
#ifdef ROADOBS
    try {
      // Dummy filename
      std::string filename;
      // read stationlist and init the api.
      // does nothing if already done
      ObsRoad road = ObsRoad(filename,db_connectfile,stationsFileName,db_parameterfile,time,NULL,false);
      std::unique_ptr<VprofPlot> vp(road.getVprofPlot(modelName, name, time));
      return vp.release();
    } catch (...) {
      METLIBS_LOG_ERROR("Exception in ObsRoad: " << db_connectfile << "," << stationsFileName << "," << db_parameterfile << "," << time);
      return 0;
    }
#else
    return 0;
#endif
  }

  if (format == fimex) {
    std::vector<stationInfo>::iterator itP = std::find_if(mStations.begin(), mStations.end(), diutil::eq_StationName(name));
    std::vector<miutil::miTime>::iterator itT = std::find(validTime.begin(), validTime.end(), time);
    if (itP == mStations.end() || itT == validTime.end())
      return 0;

    const int iPos = std::distance(mStations.begin(), itP);
    const int iTime = std::distance(validTime.begin(), itT);

    if (name == vProfPlotName and time == vProfPlotTime
        and vProfPlot.get()
        and vProfPlot->text.modelName == modelName
        and vProfPlot->text.realization == realization)
    {
      METLIBS_LOG_DEBUG("returning cached VProfPlot");
      return new VprofPlot(*vProfPlot);
    }

    std::unique_ptr<VprofPlot> vp(new VprofPlot());
    vp->text.index = -1;
    vp->text.prognostic = true;
    vp->text.modelName = modelName;
    vp->text.posName = itP->name;
    vp->text.forecastHour = forecastHour[iTime];
    vp->text.validTime = *itT;
    vp->text.latitude = itP->lat;
    vp->text.longitude = itP->lon;
    vp->text.kindexFound = false;
    vp->text.realization = realization;

    vp->prognostic = true;
    vp->maxLevels = numLevel;
    vp->windInKnots = true;

    METLIBS_LOG_DEBUG("readFomFimex"); //<<LOGVAL(reftime));
    const LonLat pos = LonLat::fromDegrees(mStations[iPos].lon, mStations[iPos].lat);
    const Time user_time(util::from_miTime(time));
    // This replaces the current dynamic crossection, if present.
    // TODO: Should be tested when more than one time step is available.
    if (!stationsFileName.empty()) {
      Source_p source = collector->getResolver()->getSource(modelName);
      if (!source)
        return 0;
      source->addDynamicCrossection(reftime, mStations[iPos].name, LonLat_v(1, pos));
    }

    const vcross::ModelReftime mr(modelName, reftime);

    FieldData_cp air_temperature = std::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, VP_AIR_TEMPERATURE));
    if (not air_temperature)
      return 0;
    ZAxisData_cp zaxis = air_temperature->zaxis();
    if (!zaxis)
      return 0;

    collector->requireVertical(Z_TYPE_PRESSURE);

    model_values_m model_values;
    try {
      model_values = vc_fetch_pointValues(collector, pos, user_time, realization);
    } catch (std::exception& e) {
      METLIBS_LOG_ERROR("exception: " << e.what());
      return 0;
    } catch (...) {
      METLIBS_LOG_ERROR("unknown exception");
      return 0;
    }

    model_values_m::iterator itM = model_values.find(mr);
    if (itM == model_values.end())
      return 0;
    name2value_t& n2v = itM->second;

    vc_evaluate_fields(collector, model_values, mr, fields);

    Values_cp z_values;
    if (util::unitsConvertible(zaxis->unit(), "hPa"))
      z_values = vc_evaluate_field(zaxis, n2v);
    else if (InventoryBase_cp pfield = zaxis->pressureField())
      z_values = vc_evaluate_field(pfield, n2v);
    if (not z_values)
      return 0;
    copy_vprof_values(z_values, vp->ptt);

    copy_vprof_values(n2v, VP_AIR_TEMPERATURE, vp->tt);
    copy_vprof_values(n2v, VP_DEW_POINT_TEMPERATURE, vp->td);
    copy_vprof_values(n2v, VP_X_WIND, vp->uu);
    copy_vprof_values(n2v, VP_Y_WIND, vp->vv);
    copy_vprof_values(n2v, VP_RELATIVE_HUMIDITY, vp->rhum);
    copy_vprof_values(n2v, VP_OMEGA, vp->om);

    vp->windInKnots = false;
    numLevel = vp->ptt.size();
    vp->maxLevels= numLevel;

    // dd,ff and significant levels (as in temp observation...)
    if (int(vp->uu.size()) == numLevel && int(vp->vv.size()) == numLevel) {
      int kmax = 0;
      for (size_t k = 0; k < size_t(numLevel); k++) {
        float uew = vp->uu[k];
        float vns = vp->vv[k];
        int ff = int(diutil::absval(uew , vns) + 0.5);
        if (!vp->windInKnots)
          ff *= 1.94384; // 1 knot = 1 m/s * 3600s/1852m
        int dd = int(270. - RAD_TO_DEG * atan2f(vns, uew) + 0.5);
        if (dd > 360)
          dd -= 360;
        if (dd <= 0)
          dd += 360;
        if (ff == 0)
          dd = 0;
        vp->dd.push_back(dd);
        vp->ff.push_back(ff);
        vp->sigwind.push_back(0);
        if (ff > vp->ff[kmax])
          kmax = k;
      }
      for (size_t l = 0; l < vp->sigwind.size(); l++) {
        for (size_t k = 1; k < size_t(numLevel) - 1; k++) {
          if (vp->ff[k] < vp->ff[k - 1] && vp->ff[k] < vp->ff[k + 1])
            vp->sigwind[k] = 1; // local ff minimum
          if (vp->ff[k] > vp->ff[k - 1] && vp->ff[k] > vp->ff[k + 1])
            vp->sigwind[k] = 2; // local ff maximum
        }
      }
      vp->sigwind[kmax] = 3; // "global" ff maximum
    }

    vProfPlotTime = time;
    vProfPlotName = name;
    vProfPlot.reset(new VprofPlot(*vp));
    METLIBS_LOG_DEBUG("returning new VProfPlot");
    return vp.release();
  }

  return 0;
}

void VprofData::readStationNames(const std::string& stationsfilename)
{
  METLIBS_LOG_SCOPE();
  std::ifstream stationfile(stationsfilename.c_str());
  if (!stationfile) {
    METLIBS_LOG_ERROR("Unable to open file '" << stationsfilename << "'");
    return;
  }

  mStations.clear();
  std::string line;
  diutil::GetLineConverter convertline("#");
  while (convertline(stationfile, line)) {
    // just skip the first line if present.
    if (miutil::contains(line, "obssource"))
      continue;
    vector<std::string> stationVector;
    int baseIdx;
    if (miutil::contains(line, ";")) {
      // the new format
      stationVector = miutil::split(line, ";", false);
      if (stationVector.size() == 7) {
        baseIdx = 2;
      } else if (stationVector.size() == 6) {
        baseIdx = 1;
      } else {
        METLIBS_LOG_ERROR("Something is wrong with: '" << line << "'");
        continue;
      }
    } else {
      // the old format
      stationVector = miutil::split(line, ",", false);
      if (stationVector.size() == 7) {
        baseIdx = 1;
      } else {
        METLIBS_LOG_ERROR("Something is wrong with: '" << line << "'");
        continue;
      }
    }
    const std::string& name = stationVector[baseIdx];
    const float lat = miutil::to_double(stationVector[baseIdx + 1]);
    const float lon = miutil::to_double(stationVector[baseIdx + 2]);
    mStations.push_back(stationInfo(name, lon, lat));
  }
}

void VprofData::renameStations()
{
  if (stationsFileName.empty())
    return;

  if (!stationList)
    readStationList();

  const int n = mStations.size(), m = stationName.size();

  multimap<std::string,int> sortlist;

  for (int i=0; i<n; i++) {
    int jmin = -1;
    float smin = 0.05*0.05 + 0.05*0.05;
    for (int j=0; j<m; j++) {
      float dx = mStations[i].lon - stationLongitude[j];
      float dy = mStations[i].lat - stationLatitude[j];
      float s = diutil::absval2(dx, dy);
      if (s < smin) {
        smin = s;
        jmin = j;
      }
    }
    std::string newname;
    if (jmin >= 0) {
      newname= stationName[jmin];
    } else {
      // TODO use formatLongitude/Latitude from diutil
      std::string slat= miutil::from_number(fabsf(mStations[i].lat));
      if (mStations[i].lat>=0.) slat += "N";
      else                     slat += "S";
      std::string slon= miutil::from_number(fabsf(mStations[i].lon));
      if (mStations[i].lon>=0.) slon += "E";
      else                      slon += "W";
      newname= slat + "," + slon;
      jmin = m;
    }

    ostringstream ostr;
    ostr << setw(4) << setfill('0') << jmin
         << newname
         // << validTime[i].isoTime() // FIXME index i may not be used here
         << mStations[i].name;
    sortlist.insert(std::make_pair(ostr.str(), i));

    stationMap[newname] = mStations[i].name;
    METLIBS_LOG_DEBUG(LOGVAL(newname)<<LOGVAL(mStations[i].name));
    mStations[i].name = newname;
  }

  // gather amdars from same stations (in station list sequence)
  vector<stationInfo> stations;
  map<std::string,int> stationCount;
  for (multimap<std::string,int>::iterator pt=sortlist.begin(); pt!=sortlist.end(); pt++) {
    int i= pt->second;

    std::string newname = mStations[i].name;
    int c;
    std::map<std::string, int>::iterator p = stationCount.find(newname);
    if (p==stationCount.end())
      stationCount[newname] = c = 1;
    else
      c = ++(p->second);
    newname += " (" + miutil::from_number(c) + ")";

    stationMap[newname] = stationMap[mStations[i].name];

    stations.push_back(stationInfo(newname, mStations[i].lon, mStations[i].lat));
  }

  std::swap(stations, mStations);
}


void VprofData::readStationList()
{
  METLIBS_LOG_SCOPE(LOGVAL(stationsFileName));
  stationList= true;

  if (stationsFileName.empty())
    return;

  ifstream file(stationsFileName.c_str());
  if (file.bad()) {
    METLIBS_LOG_ERROR("Unable to open station list '" << stationsFileName << "'");
    return;
  }

  const float notFound=9999.;
  std::string str;
  diutil::GetLineConverter convertline("#");
  while (convertline(file,str)) {
    const std::string::size_type n= str.find('#');
    if (n!=0) {
      if (n!=string::npos)
        str= str.substr(0,n);
      miutil::trim(str);
      if (not str.empty()) {
        const vector<std::string> vstr = miutil::split_protected(str, '"', '"');
        float latitude=notFound, longitude=notFound;
        std::string name;
        for (size_t i=0; i<vstr.size(); i++) {
          const vector<std::string> vstr2 = miutil::split(vstr[i], "=");
          if (vstr2.size()==2) {
            str= miutil::to_lower(vstr2[0]);
            if (str=="latitude")
              latitude= atof(vstr2[1].c_str());
            else if (str=="longitude")
              longitude= atof(vstr2[1].c_str());
            else if (str=="name") {
              name= vstr2[1];
              if (name[0]=='"' && name.length()>=3 && name[name.length()-1] == '"')
                name= name.substr(1,name.length()-2);
            }
          }
        }
        if (latitude!=notFound && longitude!=notFound && not name.empty()) {
          stationLatitude.push_back(latitude);
          stationLongitude.push_back(longitude);
          stationName.push_back(name);
        }
      }
    }
  }

  file.close();
}
