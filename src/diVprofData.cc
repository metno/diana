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
#include "vcross_v2/VcrossComputer.h"

#include <cstdio>
#include <iomanip>
#include <sstream>
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
        readFromFimex(false), numPos(0),
        numTime(0), numParam(0), numLevel(0), dataBuffer(0)
{
  METLIBS_LOG_SCOPE();
}

VprofData::~VprofData()
{
  METLIBS_LOG_SCOPE();
  delete[] dataBuffer;
}

void VprofData::readStationNames(const std::string& stationsfilename)
{
  METLIBS_LOG_SCOPE();
  FILE *stationfile;
  char line[1024];

  if ((stationfile = fopen(stationsfilename.c_str(), "rb")) == NULL) {
    METLIBS_LOG_ERROR("Unable to open file! " << stationsfilename);
    return;
  }

  vector<std::string> stationVector;
  vector<station> stations;
  std::string miLine;
  while (fgets(line, 1024, stationfile) != NULL) {
    miLine = std::string(line);
    // just skip the first line if present.
    if (miutil::contains(miLine, "obssource"))
      continue;
    if (miutil::contains(miLine, ";")) {
      // the new format
      stationVector = miutil::split(miLine, ";", false);
      if (stationVector.size() == 7) {
        station st;
        char stid[10];
        int wmo_block = miutil::to_int(stationVector[0]) * 1000;
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
        if (stationVector.size() == 6) {
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
    } else {
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
}

bool VprofData::readFimex(vcross::Setup_p setup, const std::string& reftimestr)
{
  METLIBS_LOG_SCOPE();

  collector = miutil::make_shared<vcross::Collector>(setup);
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
    BOOST_FOREACH(vcross::Crossection_cp cs, inv->crossections) {
      if (cs->length() != 1)
        continue;
      posName.push_back(cs->label());
      posLatitude.push_back(cs->point(0).latDeg());
      posLongitude.push_back(cs->point(0).lonDeg());
      posDeltaLatitude.push_back(0.0);
      posDeltaLongitude.push_back(0.0);
      posTemp.push_back(0);
    }
  }

  BOOST_FOREACH(const vcross::Time::timevalue_t& time, inv->times.values) {
    validTime.push_back(vcross::util::to_miTime(inv->times.unit, time));
  }

  numPos = posName.size();
  numTime = validTime.size();
  numParam = 6;
  mainText.push_back(modelName);

  const miTime rt = util::to_miTime(mr.reftime);
  for (size_t i = 0; i < validTime.size(); i++) {
    forecastHour.push_back(miTime::hourDiff(validTime[i],rt));
    progText.push_back(std::string("+" + miutil::from_number(forecastHour[i])));
  }
  readFromFimex = true;
  vProfPlot.reset(0);
  return true;
}


bool VprofData::readFile( const std::string& fileName)
{
  METLIBS_LOG_SCOPE("fileName= " << fileName);

  // reading and storing all information and unpacked data

  const int bufferlength = 512;

  std::auto_ptr<FtnVfile> vfile(new FtnVfile(fileName, bufferlength));

  int length, ctype;
  int *head = 0, *content = 0, *posid = 0, *tmp = 0;

  numPos = numTime = numParam = numLevel = 0;

  bool success = true;

  try {
    vfile->init();
    length = 8;
    head = vfile->getInt(length);
    if (head[0] != 201 || head[1] != 2 || head[2] != bufferlength)
      throw VfileError();
    length = head[7];
    if (length != 12)
      throw VfileError();
    content = vfile->getInt(length);

    int ncontent = head[7];

    int nlines, nposid;

    bool c1, c2, c3, c4, c5, c6;
    c1 = c2 = c3 = c4 = c5 = c6 = false;

    for (int ic = 0; ic < ncontent; ic += 2) {
      ctype = content[ic];
      length = content[ic + 1];

      switch (ctype) {
      case 101:
        if (length < 4)
          throw VfileError();
        tmp = vfile->getInt(length);
        numPos = tmp[0];
        numTime = tmp[1];
        numParam = tmp[2];
        numLevel = tmp[3];
        if (numPos < 1 || numTime < 1 || numParam < 2 || numLevel < 2)
          throw VfileError();
        //if (length>4) prodNum= tmp[4];
        //if (length>5) gridNum= tmp[5];
        //if (length>6) vCoord=  tmp[6];
        //if (length>7) interpol=tmp[7];
        //if (length>8) isurface=tmp[8];
        delete[] tmp;
        tmp = 0;
        c1 = true;
        break;

      case 201:
        if (numTime < 1)
          throw VfileError();
        tmp = vfile->getInt(5 * numTime);
        for (int n = 0; n < 5 * numTime; n += 5) {
          miTime t = miTime(tmp[n], tmp[n + 1], tmp[n + 2], tmp[n + 3], 0, 0);
          if (tmp[n + 4] != 0)
            t.addHour(tmp[n + 4]);
          validTime.push_back(t);
          forecastHour.push_back(tmp[n + 4]);
        }
        delete[] tmp;
        tmp = 0;
        tmp = vfile->getInt(numTime);
        for (int n = 0; n < numTime; n++) {
          std::string str = vfile->getString(tmp[n]);
          progText.push_back(str);
        }
        delete[] tmp;
        tmp = 0;
        c2 = true;
        break;

      case 301:
        nlines = vfile->getInt();
        tmp = vfile->getInt(2 * nlines);
        for (int n = 0; n < nlines; n++) {
          std::string str = vfile->getString(tmp[n * 2]);
          mainText.push_back(str);
        }
        delete[] tmp;
        tmp = 0;
        c3 = true;
        break;

      case 401:
        if (numPos < 1)
          throw VfileError();
        tmp = vfile->getInt(numPos);
        for (int n = 0; n < numPos; n++) {
          std::string str = vfile->getString(tmp[n]);
          // may have one space at the end (n*2 characters stored in file)
          miutil::trim(str, false, true);
          posName.push_back(str);
        }
        delete[] tmp;
        tmp = 0;
        c4 = true;
        break;

      case 501:
        if (numPos < 1)
          throw VfileError();
        nposid = vfile->getInt();
        posid = vfile->getInt(2 * nposid);
        tmp = vfile->getInt(nposid * numPos);
        for (int n = 0; n < nposid; n++) {
          int nn = n * 2;
          float scale = powf(10., posid[nn + 1]);
          if (posid[nn] == -1) {
            for (int i = 0; i < numPos; i++)
              posLatitude.push_back(scale * tmp[n + i * nposid]);
          } else if (posid[nn] == -2) {
            for (int i = 0; i < numPos; i++)
              posLongitude.push_back(scale * tmp[n + i * nposid]);
          } else if (posid[nn] == -3 && posid[nn + 2] == -4) {
            for (int i = 0; i < numPos; i++)
              posTemp.push_back(
                  tmp[n + i * nposid] * 1000 + tmp[n + 1 + i * nposid]);
          } else if (posid[nn] == -5) {
            for (int i = 0; i < numPos; i++)
              posDeltaLatitude.push_back(scale * tmp[n + i * nposid]);
          } else if (posid[nn] == -6) {
            for (int i = 0; i < numPos; i++)
              posDeltaLongitude.push_back(scale * tmp[n + i * nposid]);
          }
        }
        delete[] posid;
        posid = 0;
        delete[] tmp;
        tmp = 0;
        if (int(posTemp.size()) == numPos) {
          for (int i = 0; i < numPos; i++) {
            if (posTemp[i] > 1000 && posTemp[i] < 99000) {
              std::ostringstream ostr;
              ostr << setw(5) << setfill('0') << posTemp[i];
              obsName.push_back(std::string(ostr.str()));
            } else if (posTemp[i] >= 99000 && posTemp[i] <= 99999) {
              obsName.push_back("99");
            } else {
              obsName.push_back("");
            }
          }
        }
        c5 = true;
        break;

      case 601:
        if (numParam < 1)
          throw VfileError();
        tmp = vfile->getInt(numParam);
        for (int n = 0; n < numParam; n++)
          paramId.push_back(tmp[n]);
        delete[] tmp;
        tmp = 0;
        tmp = vfile->getInt(numParam);
        for (int n = 0; n < numParam; n++)
          paramScale.push_back(powf(10., tmp[n]));
        delete[] tmp;
        tmp = 0;
        c6 = true;
        break;

      default:
        throw VfileError();
      }
    }

    delete[] head;
    head = 0;
    delete[] content;
    content = 0;

    if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6)
      throw VfileError();

    // read all data, but keep data in a short int buffer until used
    length = numPos * numTime * numParam * numLevel;

    // dataBuffer[numPos][numTime][numParam][numLevel]
    delete dataBuffer;
    dataBuffer = 0; // in case of exception
    dataBuffer = vfile->getShortInt(length);

  }  // end of try

  catch (...) {
    METLIBS_LOG_ERROR("Bad Vprof file: " << fileName);
    success = false;
  }

  delete[] head;
  delete[] content;
  delete[] posid;
  delete[] tmp;

  return success;
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

VprofPlot* VprofData::getData(const std::string& name, const miTime& time)
{
  METLIBS_LOG_SCOPE(name << "  " << time << "  " << modelName);

  int iPos = 0;
  while (iPos < numPos && posName[iPos] != name)
    iPos++;

  int iTime = 0;
  while (iTime < numTime && validTime[iTime] != time)
    iTime++;

  if (iPos == numPos || iTime == numTime)
    return 0;

  if (name == vProfPlotName and time == vProfPlotTime and vProfPlot.get() and vProfPlot->text.modelName == modelName) {
    METLIBS_LOG_DEBUG("returning cached VProfPlot");
    return new VprofPlot(*vProfPlot);
  }

  std::auto_ptr<VprofPlot> vp(new VprofPlot());
  vp->text.index = -1;
  vp->text.prognostic = true;
  vp->text.modelName = modelName;
  vp->text.posName = posName[iPos];
  vp->text.forecastHour = forecastHour[iTime];
  vp->text.validTime = validTime[iTime];
  vp->text.latitude = posLatitude[iPos];
  vp->text.longitude = posLongitude[iPos];
  vp->text.kindexFound = false;

  vp->prognostic = true;
  vp->maxLevels = numLevel;
  vp->windInKnots = true;

  if (readFromFimex) {

    const LonLat pos = LonLat::fromDegrees(posLongitude[iPos],posLatitude[iPos]);
    const Time user_time(util::from_miTime(time));
    // This replaces the current dynamic crossection, if present.
    // TODO: Should be tested when more than one time step is available.
    if (!stationsFileName.empty()) {
      Source_p source = collector->getResolver()->getSource(modelName);
      if (!source)
        return 0;
      source->addDynamicCrossection(reftime, posName[iPos], LonLat_v(1, pos));
    }

    const vcross::ModelReftime mr(modelName, reftime);

    FieldData_cp air_temperature = boost::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, VP_AIR_TEMPERATURE));
    if (not air_temperature)
      return 0;
    ZAxisData_cp zaxis = air_temperature->zaxis();
    if (!zaxis)
      return 0;

    collector->requireVertical(Z_TYPE_PRESSURE);

    model_values_m model_values;
    try {
      model_values = vc_fetch_pointValues(collector, pos, user_time);
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


  } else {
    for (int n = 0; n < numParam; n++) {
      const float scale = paramScale[n];
      int j = iPos * numTime * numParam * numLevel + iTime * numParam * numLevel + n * numLevel;
      if (paramId[n] == 8) {
        for (int k = 0; k < numLevel; k++)
          vp->ptt.push_back(scale * dataBuffer[j++]);
      } else if (paramId[n] == 4) {
        for (int k = 0; k < numLevel; k++)
          vp->tt.push_back(scale * dataBuffer[j++]);
      } else if (paramId[n] == 5) {
        for (int k = 0; k < numLevel; k++)
          vp->td.push_back(scale * dataBuffer[j++]);
      } else if (paramId[n] == 2) {
        for (int k = 0; k < numLevel; k++)
          vp->uu.push_back(scale * dataBuffer[j++]);
      } else if (paramId[n] == 3) {
        for (int k = 0; k < numLevel; k++)
          vp->vv.push_back(scale * dataBuffer[j++]);
      } else if (paramId[n] == 13) {
        for (int k = 0; k < numLevel; k++)
          vp->om.push_back(scale * dataBuffer[j++]);
      }
    }
  }


  // dd,ff and significant levels (as in temp observation...)
  if (int(vp->uu.size()) == numLevel && int(vp->vv.size()) == numLevel) {
    float degr = 180. / 3.141592654;
    int kmax = 0;
    for (size_t k = 0; k < size_t(numLevel); k++) {
      float uew = vp->uu[k];
      float vns = vp->vv[k];
      int ff = int(sqrtf(uew * uew + vns * vns) + 0.5);
      if (!vp->windInKnots)
        ff *= 1.94384; // 1 knot = 1 m/s * 3600s/1852m
      int dd = int(270. - degr * atan2f(vns, uew) + 0.5);
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
    for (size_t l = 0; l < (vp->sigwind.size()); l++) {
      for (size_t k = 1; k < size_t(numLevel) - 1; k++) {
        if (vp->ff[k] < vp->ff[k - 1] && vp->ff[k] < vp->ff[k + 1])
          vp->sigwind[k] = 1;
        if (vp->ff[k] > vp->ff[k - 1] && vp->ff[k] > vp->ff[k + 1])
          vp->sigwind[k] = 2;
      }
    }
    vp->sigwind[kmax] = 3;
  }

  vProfPlotTime = time;
  vProfPlotName = name;
  vProfPlot.reset(new VprofPlot(*vp));
  METLIBS_LOG_DEBUG("returning new VProfPlot");
  return vp.release();
  // end !cached VprofPlot
}
