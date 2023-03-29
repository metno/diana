/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2022 met.no

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

#include "diana_config.h"

#include "diObsBufr.h"
#include "diObsData.h"
#include "util/format_int.h"
#include "vprof/diVprofValues.h"

#include <mi_fieldcalc/MetConstants.h>

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>

#define MILOGGER_CATEGORY "diana.ObsBufr"
#include <miLogger/miLogging.h>

// libemos interface (to fortran routines)
extern "C" {
extern long readbufr(FILE * file, char * buffer, long * bufr_len);

extern void bufrex_(int* ilen, int* ibuff, int* ksup, int* ksec0, int* ksec1,
    int* ksec2, int* ksec3, int* ksec4, int* kelem,
    const char* cnames, const char* cunits, int* kvals,
    double* values, const char* cvals, int* kerr,
    int len_cnames, int len_cunits, int len_cvals);
extern void buprt_(int* kswitch, int* ksub1, int* ksub2, int* kkelem,
    const char* cnames, const char* cunits, const char* cvals,
    int* kkvals, double* values, int* ksup, int* ksec1,
    int* kerr, int len_cnames, int len_cunits, int len_cvals);
extern void busel_(int* ktdlen, int* ktdlst, int* ktdexl, int* ktdexp,
    int* kerr);
extern void busel2_(int* ksubset, int * kelem,
    int* ktdlen, int* ktdlst, int* ktdexl, int* ktdexp,
    const char* cnames, const char* cunits,
    int* kerr);
extern void bus012_(int* ilen, int* ibuff, int* ksup,
    int* ksec0, int* ksec1, int* ksec2,int* kerr);
} // extern "C"

using namespace miutil;

namespace {
const float DEG_TO_RAD = M_PI / 180;

const double bufrMissing = 1.6e+38;

// constants for changing to met.no units
const double pa2hpa = 0.01;

// clang-format off
const char* BUFR_KEY_NAMES[ObsDataBufr::BUFR_KEYS_END] {
  "911ff",
  "911ff_10",
  "911ff_180",
  "911ff_360",
  "911ff_60",
  "a",
  "auto",
  "Ch",
  "Cl",
  "Cm",
  "Date",
  "dd",
  "depth",
  "dndndn",
  "ds",
  "Dv",
  "dw1dw1",
  "dxdxdx",
  "ff",
  "fmfm",
  "fxfx",
  "fxfx_180",
  "fxfx_360",
  "fxfx_60",
  "h",
  "Height",
  "HHH",
  "Hw1Hw1",
  "HwaHwa",
  "HwHw",
  "Id",
  "lat",
  "lon",
  "N",
  "Name",
  "Nh",
  "PHPHPHPH",
  "ppp",
  "PPPP",
  "Pw1Pw1",
  "PwaPwa",
  "PwPw",
  "QI",
  "QI_NM",
  "QI_RFF",
  "quality",
  "RRR",
  "RRR_1",
  "RRR_12",
  "RRR_24",
  "RRR_3",
  "RRR_6",
  "RRR_accum",
  "s",
  "sss",
  "SSSS",
  "SWH",
  "TdTdTd",
  "TE",
  "Time",
  "TTT",
  "TTTT",
  "TwTwTw",
  "TxTn",
  "vs",
  "VV",
  "VVVV",
  "W1",
  "W2",
  "wmonumber",
  "ww"
};
// clang-format on

const int len_cnames = 64, len_cunits = 24, len_cvals = 80;

const std::string substr(const char* cvals, int index, int len)
{
  cvals += index * len_cvals;
  return std::string(cvals, cvals + len);
}
void add_substr(std::string& s, const char* cvals, int index, int len)
{
  if ( index > -1) {
    cvals += index * len_cvals;
    s.append(cvals, len);
  }
}

void put_date_time(ObsDataBufr::ObsDataB& odb, int month, int day, int hour, int minute)
{
  std::string out = "MM-DD";
  diutil::format_int(month, out, 0, 2);
  diutil::format_int(day,   out, 3, 2);
  odb.put_string(ObsDataBufr::k_Date, out);

  out[2] = '.';
  diutil::format_int(hour,   out, 0, 2);
  diutil::format_int(minute, out, 3, 2);
  odb.put_string(ObsDataBufr::k_Time, out);
}

void format_wmo(std::string& out, int wmoBlock, int wmoSubarea, int wmoStation)
{
  if ( wmoSubarea > 0 ) {
    out.resize(7, '0');
    diutil::format_int(wmoBlock,     out, 0, 1);
    diutil::format_int(wmoSubarea,   out, 1, 1);
    diutil::format_int(wmoStation,   out, 3, 4);
  } else {
    out.resize(5, '0');
    diutil::format_int(wmoBlock,   out, 0, 2);
    diutil::format_int(wmoStation, out, 2, 3);
  }
}

//! HACK if year has only two digits, files from year 1971 to 2070 is assumed
int fix_year(int year)
{
  if (year < 1000)
    year = (year > 70) ? year + 1900 : year + 2000;
  return year;
}

std::string cloudAmount(int i)
{
  if (i == 0)
    return "SKC";
  if (i == 1)
    return "NSC";
  if (i == 8)
    return "O/";
  if (i == 11)
    return "S/";
  if (i == 12)
    return "B/";
  if (i == 13)
    return "F/";
  return "";
}

std::string cloudHeight(int i)
{
  i /= 30;
  std::string cl(3, '0');
  diutil::format_int(i, cl, 0, 3, '0');
  return cl;
}

std::string cloud_TCU_CB(int i)
{
  if (i == 3)
    return " TCU";
  if (i == 9)
    return " CB";
  return "";
}

float height_of_clouds(double height)
{
  if (height < 50)
    return 0.0;
  if (height < 100)
    return 1.0;
  if (height < 200)
    return 2.0;
  if (height < 300)
    return 3.0;
  if (height < 600)
    return 4.0;
  if (height < 1000)
    return 5.0;
  if (height < 1500)
    return 6.0;
  if (height < 2000)
    return 7.0;
  if (height < 2500)
    return 8.0;
  return 9.0;
}

void cloud_type(ObsDataBufr::ObsDataB& odb, double v)
{
  int type = int(v) / 10;
  float value = float(int(v) % 10);

  if (value < 1)
    return;

  if (type == 1)
    odb.put_float(ObsDataBufr::k_Ch, value);
  if (type == 2)
    odb.put_float(ObsDataBufr::k_Cm, value);
  if (type == 3)
    odb.put_float(ObsDataBufr::k_Cl, value);
}

float ms2code4451(float v)
{
  using miutil::constants::knots2ms;
  if (v < knots2ms)
    return 0.0;
  if (v < 5 * knots2ms)
    return 1.0;
  if (v < 10 * knots2ms)
    return 2.0;
  if (v < 15 * knots2ms)
    return 3.0;
  if (v < 20 * knots2ms)
    return 4.0;
  if (v < 25 * knots2ms)
    return 5.0;
  if (v < 30 * knots2ms)
    return 6.0;
  if (v < 35 * knots2ms)
    return 7.0;
  if (v < 40 * knots2ms)
    return 8.0;
  if (v < 25 * knots2ms)
    return 9.0;
  return 9.0;
}
} // namespace

ObsBufr::ObsBufr()
{
}

struct ObsBufr::BUFRDecodeData
{
  const int kelem = 80000; // length of subsection
  const int kvals = 4096000;

  int ksup[9];
  int ksec0[3];
  int ksec1[40];
  int ksec2[4096];
  int ksec3[4];
  int ksec4[2];

  std::unique_ptr<char[]> cnames;
  std::unique_ptr<char[]> cunits;
  std::unique_ptr<char[]> cvals;
  std::unique_ptr<double[]> values;

  std::unique_ptr<int[]> ktdlst, ktdexp;

  BUFRDecodeData();
};

ObsBufr::BUFRDecodeData::BUFRDecodeData()
    : cnames(new char[kelem * len_cnames])
    , cunits(new char[kelem * len_cunits])
    , cvals(new char[kvals * len_cvals])
    , values(new double[kvals])
    , ktdlst(new int[kelem])
    , ktdexp(new int[kelem])
{
}

// ########################################################################

ObsDataBufr::ObsDataBufr(int l, const miutil::miTime& t, int td)
    : level(l)
    , time(t)
    , timeDiff(td)
{
}

ObsDataBufr::ObsDataB::ObsDataB()
{
  obsdata = std::make_shared<ObsDataVector>();
  for (int i = 0; i < BUFR_KEYS_END; ++i) {
    ki_BUFR[i] = obsdata->add_key(BUFR_KEY_NAMES[i]);
  }
}

ObsDataBufr::ObsDataB::~ObsDataB() {}

ObsDataVector_p ObsDataBufr::getObsData(const std::string& filename)
{
  odb = ObsDataB();
  if (init(filename))
    return odb.obsdata;
  else
    return ObsDataVector_p();
}

// ########################################################################

bool ObsBufr::init(const std::string& bufr_file)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_INFO("Reading '"<<bufr_file<<"'");

  FILE* file_bufr = fopen(bufr_file.c_str(), "r");
  if (!file_bufr) {
    METLIBS_LOG_ERROR("fopen failed for '" << bufr_file << "'");
    return false;
  }

  int ibuff[50000];
  std::unique_ptr<BUFRDecodeData> bdd(new BUFRDecodeData);

  bool ok = true, next = true;
  while (next) { // get the next BUFR product
    long buf_len = sizeof(ibuff);
    const int iret = readbufr(file_bufr, (char*)ibuff, &buf_len);

    if (iret != 0) {
      if (iret == -1) {
        // EOF
      } else {
        ok = false;
        if (iret == -2)
          METLIBS_LOG_ERROR("file handling problem");
        else if (iret == -3)
          METLIBS_LOG_ERROR("array too small");
        else
          METLIBS_LOG_ERROR("other error: 0x" << std::hex << iret);
      }
      break;
    }

    next = BUFRdecode(ibuff, buf_len, *bdd.get());
  }

  fclose(file_bufr);

  //init returns false when BUFRdecode returns false
  //BUFRdecode returns false when vprof station is found. Todo: make this more intuitive
  return ok && next;
}

// static
bool ObsBufr::ObsTime(const std::string& bufr_file, miTime& time)
{
  METLIBS_LOG_SCOPE();

  FILE* file_bufr = fopen(bufr_file.c_str(), "r");
  if (!file_bufr) {
    METLIBS_LOG_ERROR("fopen failed for '" << bufr_file << "'");
    return false;
  }

  int ibuff[50000];

  long buf_len = sizeof(ibuff);
  const long iret = readbufr(file_bufr, (char*)ibuff, &buf_len);

  bool ok = true;
  if (iret != 0) {
    ok = false;
    if (iret == -1)
      METLIBS_LOG_INFO("EOF");
    else if (iret == -2)
      METLIBS_LOG_ERROR("file handling problem");
    else if (iret == -3)
      METLIBS_LOG_ERROR("array too small");
    else
      METLIBS_LOG_ERROR("other error");
  } else {
    int ksup[9];
    int ksec0[3];
    int ksec1[40];
    int ksec2[4096];
    int kerr;

    //read sec 0-2
    int ibuf_len = buf_len;
    bus012_(&ibuf_len, ibuff, ksup, ksec0, ksec1, ksec2, &kerr);

    int year = fix_year(ksec1[8]);
    time = miutil::miTime(year, ksec1[9], ksec1[10], ksec1[11], ksec1[12], 0);
    if (time.undef())
      ok = false;
  }

  fclose(file_bufr);

  return ok;
}

// ########################################################################

bool StationBufr::readStationInfo(const std::vector<std::string>& bufr_file, std::vector<stationInfo>& stations)
{
  id.clear();
  idmap.clear();

  for (size_t i=0; i< bufr_file.size(); i++)
    init(bufr_file[i]);

  stations.clear();
  stations.reserve(id.size());
  for (size_t i=0; i<id.size(); ++i)
    stations.push_back(stationInfo(id[i], longitude[i], latitude[i]));
  return true;
}

VprofValues_p VprofBufr::getVprofPlot(const std::vector<std::string>& bufr_file, const std::string& station, VerticalAxis vertical_axis)
{
  METLIBS_LOG_SCOPE(LOGVAL(station));

  index = izone = istation = 0;

  vertical_axis_ = vertical_axis;
  const std::string zUnit = (vertical_axis_ == PRESSURE) ? "hPa" : "m";
  vplot = std::make_shared<VprofSimpleValues>();
  vplot->add(temperature = std::make_shared<VprofSimpleData>(vprof::VP_AIR_TEMPERATURE, zUnit, "degree_Celsius"));
  vplot->add(dewpoint_temperature = std::make_shared<VprofSimpleData>(vprof::VP_DEW_POINT_TEMPERATURE, zUnit, "degree_Celsius"));
  vplot->add(wind_dd = std::make_shared<VprofSimpleData>(vprof::VP_WIND_DD, zUnit, vprof::VP_UNIT_COMPASS_DEGREES));
  vplot->add(wind_ff = std::make_shared<VprofSimpleData>(vprof::VP_WIND_FF, zUnit, "m/s"));
  vplot->add(wind_sig = std::make_shared<VprofSimpleData>(vprof::VP_WIND_SIG, zUnit, ""));

  //if station(no)
  std::vector<std::string> token = miutil::split(station, "(");
  METLIBS_LOG_DEBUG(LOGVAL(token.size()));
  if (token.empty())
    return 0;

  if (token.size() == 2)
    index = atoi(token[1].c_str());

  strStation = token[0];
  miutil::trim(strStation);

  if (miutil::is_int(token[0])) {
    int ii = atoi(station.c_str());
    izone = ii / 1000;
    istation = ii - izone * 1000;
  }

  for (size_t i=0; i< bufr_file.size(); i++) {
    //init returns true when reaching end of file, returns false when station is found
    if (!init(bufr_file[i])) {
      vplot->calculate();
      return vplot;
    }
  }

  // No station found
  return 0;
}

// ########################################################################

bool ObsBufr::BUFRdecode(int* ibuff, int ilen, BUFRDecodeData& b)
{
  // Decode BUFR message into fully decoded form
  METLIBS_LOG_SCOPE();

  int kerr;
  bus012_(&ilen, ibuff, b.ksup, b.ksec0, b.ksec1, b.ksec2, &kerr);
  if (kerr > 0) {
    METLIBS_LOG_ERROR("Error in BUS012: KERR=" << kerr);
    return true;
  }

  int kkvals = b.kvals;
  int kxelem = std::min(b.kvals / b.ksup[5], b.kelem);
  int ktdlen, ktdexl;

  bufrex_(&ilen, ibuff, b.ksup, b.ksec0, b.ksec1, b.ksec2, b.ksec3, b.ksec4, &kxelem, b.cnames.get(), b.cunits.get(), &kkvals, b.values.get(), b.cvals.get(),
          &kerr, len_cnames, len_cunits, len_cvals);
  if (kerr > 0) {
    METLIBS_LOG_ERROR("Error in BUFREX: KERR=" << kerr);
    return true;
  }

  const int nsubset = b.ksup[5];

  //  Convert messages with data category (BUFR table A) 0 and 1 only.
  //  0 = Surface data - land, 1 = Surface data - sea
  //  if (ksec1[5] > 1) {
  //    METLIBS_LOG_DEBUG(ksec1[5]);
  //    return true;
  //  }

  // Return list of Data Descriptors from Section 3 of Bufr message, and
  // total/requested list of elements. BUFREX must have been called before BUSEL.

  for (int i = 1; i < nsubset + 1; i++) {
    busel2_(&i, &kxelem, &ktdlen, b.ktdlst.get(), &ktdexl, b.ktdexp.get(), b.cnames.get(), b.cunits.get(), &kerr);
    if (kerr > 0) {
      METLIBS_LOG_ERROR("Error in BUSEL: KERR=" << kerr);
      continue;
    }

    const SubsetResult sr = handleBufrSubset(ktdexl, b.ktdexp.get(), b.values.get(), b.cvals.get(), i - 1, kxelem);
    if (sr == BUFR_OK)
      return true;
    else if (sr == BUFR_ERROR)
      return false;
  }

  return true;
}

// ########################################################################

ObsBufr::SubsetResult ObsDataBufr::handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                                    const char* cvals, int subset, int kelem)
{
  bool ok;
  odb.push();
  if (level < -1)
    ok = get_diana_data(ktdexl, ktdexp, values, cvals, subset, kelem);
  else
    ok = get_diana_data_level(ktdexl, ktdexp, values, cvals, subset, kelem);
  if (ok)
    ok = timeOK(odb.basic().obsTime);
  if (!ok) {
    odb.pop();
  }
  return BUFR_CONTINUE;
}

bool ObsDataBufr::timeOK(const miutil::miTime& t) const
{
  return (timeDiff < 0 || abs(miTime::minDiff(t, time)) <= timeDiff);
}

bool ObsDataBufr::get_diana_data(int ktdexl, const int* ktdexp, const double* values, const char* cvals, int subset, int kelem)
{
  using miutil::constants::t0;

  int wmoBlock = 0;
  int wmoSubarea = 0;
  int wmoStation = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  bool wmoNumber = false;
  float depth = -1.0;
  const int timePeriodMissing = -9999;
  int timePeriodHour = timePeriodMissing;
  int timePeriodMinute = timePeriodMissing;
  int timeDisplacement = 0;
  std::string cloudStr;
  unsigned int selected_wind_vector_ambiguities = 0;
  unsigned int wind_speed_selection_count = 1;
  unsigned int wind_dir_selection_count = 1;

  bool skip_time = false;
  // Will be set to true if 008021 is encountered with value 26,
  // which means 'Time of last known position', used for buoys.
  // The date/time descriptors following this will be skipped.

  std::string d_name;
  odb.metar().CAVOK = false;
  odb.basic().xpos = -32767;
  odb.basic().ypos = -32767;
  odb.metar().ship_buoy = 0;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {
    //METLIBS_LOG_DEBUG(ktdexp[i]<<" : "<<values[j]);
    switch (ktdexp[i]) {

    //   8021  TIME SIGNIFICANCE
    case 8021:
      skip_time = (int(values[j]) == 26);
      break;

      //   1001  WMO BLOCK NUMBER
    case 1001:
      if (values[j] < bufrMissing) {
        wmoBlock = int(values[j]);
      }
      break;

    case 1003:
      if (values[j] < bufrMissing) {
        wmoBlock = int(values[j]);
        odb.metar().ship_buoy = true;
      }
      break;

      //   1020 WMO REGION SUB-AREA
    case 1020:
      if (values[j] < bufrMissing) {
        wmoSubarea = int(values[j]);
      }
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      if (values[j] < bufrMissing) {
        wmoStation = int(values[j]);
        odb.put_float(k_wmonumber, float(wmoStation));
        wmoNumber = true;
      }
      break;

      //   1005 BUOY/PLATFORM IDENTIFIER
    case 1005:
      if (values[j] < bufrMissing) {
        if ( wmoBlock > 0 ) {
          wmoStation = int(values[j]);
          odb.put_float(k_wmonumber, float(wmoStation));
          wmoNumber = true;
        } else {
          odb.basic().id = miutil::from_number(values[j]);
        }
        odb.metar().ship_buoy = true;
      }
      break;

      //001006 AIRCRAFT FLIGHT NUMBER
    case 1006:
    {
      if (! wmoNumber) {
        int index = int(values[j]) / 1000 - 1;
        add_substr(odb.basic().id, cvals, index, 6);
      }
    }
    break;

    //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1011:
    {
      if ( !wmoNumber ) {
        int index = int(values[j]) / 1000 - 1;
        add_substr(odb.basic().id, cvals, index, 7);
        odb.metar().ship_buoy = true;
      }
    }
    break;

    //001015 STATION OR SITE NAME
    case 1015:
    {
      int index = int(values[j]) / 1000 - 1;
      add_substr(d_name, cvals, index, 10);
    }
    break;

    //001019 LONG STATION OR SITE NAME
    case 1019:
    {
      int index = int(values[j]) / 1000 - 1;
      add_substr(d_name, cvals, index, 10);
    }
    break;

    //001063 ICAO LOCATION INDICATOR
    case 1063:
    {
      int index = int(values[j]) / 1000 - 1;
      add_substr(odb.metar().metarId, cvals, index, 4);
    }
    break;

    //001087 WMO MARINE OBSERVING PLATFORM EXTENDED IDENTIFIER
    case 1087:
    {
      odb.basic().id = miutil::from_number(int(values[j]));
      odb.metar().ship_buoy = true;
    }
    break;

    //001102 NATIONAL STATION NUMBER
    case 1102:
      if ( !wmoNumber ) {
        odb.basic().id = miutil::from_number(int(values[j]));
      }
      break;

      //001195 MOBIL LAND STATION IDENTIFIER
    case 1195:
    {
      if ( !wmoNumber ) {
        int index = int(values[j]) / 1000 - 1;
        add_substr(odb.basic().id, cvals, index, 5);
      }
    }
    break;

    //   1012  DIRECTION OF MOTION OF MOVING OBSERVING PLATFORM, DEGREE TRUE
    case 1012:
      if (values[j] > 0 && values[j] < bufrMissing)
        odb.put_float(k_ds, values[j]);
      break;

      //   1013  SPEED OF MOTION OF MOVING OBSERVING PLATFORM, M/S
    case 1013:
      if (values[j] < bufrMissing)
        odb.put_float(k_vs, ms2code4451(values[j]));
      break;

      //   2001 TYPE OF STATION
    case 2001:
      if (values[j] < bufrMissing)
        odb.put_float(k_auto, values[j]);
      break;

      //   4001  YEAR
    case 4001:
      if (!skip_time && values[j]<bufrMissing )
        year = int(values[j]);
      break;

      //   4002  MONTH
    case 4002:
      if (!skip_time && values[j]<bufrMissing )
        month = int(values[j]);
      break;

      //   4003  DAY
    case 4003:
      if (!skip_time && values[j]<bufrMissing )
        day = int(values[j]);
      break;

      //   4004  HOUR
    case 4004:
      if (!skip_time && values[j]<bufrMissing )
        hour = int(values[j]);
      break;

      //   4005  MINUTE
    case 4005:
      if (!skip_time && values[j]<bufrMissing )
        minute = int(values[j]);
      break;

      //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
      //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      if (values[j]<bufrMissing ) {
        odb.basic().ypos = values[j];
      }
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      if (values[j] < bufrMissing) {
        odb.basic().xpos = values[j];
      }
      break;

      //   7030  HEIGHT OF STATION GROUND ABOVE MEAN SEA LEVEL, M
    case 7030:
      if (values[j] < bufrMissing)
        odb.put_float(k_Height, values[j]);
      break;

      //   7032  HEIGHT OF SENSOR, M
    case 7032:
      if (values[j] < bufrMissing) {
        /* never used heightOfSensor = values[j]; */
      }
      break;

      //   7063  Depth, M
    case 7063:
      if (values[j] < bufrMissing)
        depth = values[j];
      break;

      //   04024  Time period or displacement hour
    case 4024:
      if (ktdexp[i - 1] == 4024) { // Second occurrence, must be displacement
        if (values[j] < bufrMissing) {
          timeDisplacement = int(values[j]);
        } else {
          // Not sure what to do here; 0 looks like best guess
          timeDisplacement = 0;
        }
      } else { // First occurrence, must be time period
        timeDisplacement = 0; // resets time displacement to 'missing' value
        if (values[j] < bufrMissing) {
          timePeriodHour = int(values[j]);
        } else {
          timePeriodHour = timePeriodMissing;
        }
      }
      break;

      //   04025  Time period minute
    case 4025:
      if (values[j] < bufrMissing) {
        timePeriodMinute = int(values[j]);
      } else {
        timePeriodMinute = timePeriodMissing;
      }
      break;

      // 010008 GEOPOTENTIAL (20 bits), M**2/S**2
      // 010003 GEOPOTENTIAL (17 bits), M**2/S**2
    case 10008:
    case 10003:
      if (values[j] < bufrMissing) {
        odb.put_float(k_HHH, values[j] / miutil::constants::g);
      }
      break;
      // 010009 GEOPOTENTIAL HEIGHT
    case 10009:
      if (values[j] < bufrMissing)
        odb.put_float(k_HHH, values[j]);
      break;

      //   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
    case 10051:
    case 7004:
      if (values[j] < bufrMissing)
        odb.put_float(k_PPPP, values[j] * pa2hpa);
      break;

      //010052 ALTIMETER SETTING (QNH), Pa->hPa
    case 10052:
      if (values[j] < bufrMissing)
        odb.put_float(k_PHPHPHPH, values[j] * pa2hpa);
      break;

      //   10061  3 HOUR PRESSURE CHANGE, Pa->hpa
    case 10061:
      if (values[j] < bufrMissing)
        odb.put_float(k_ppp, values[j] * pa2hpa);
      break;

      //   10063  CHARACTERISTIC OF PRESSURE TENDENCY
    case 10063:
      if (values[j] < bufrMissing)
        odb.put_float(k_a, values[j]);
      break;

      //   11011  WIND DIRECTION AT 10 M, DEGREE TRUE
    case 11011:
      if (values[j] < bufrMissing) {
        if ( selected_wind_vector_ambiguities ) {
          if ( selected_wind_vector_ambiguities == wind_dir_selection_count ) {
            odb.put_float(k_dd, values[j]);
            //METLIBS_LOG_DEBUG("dd:"<<values[j]);
          }
          wind_dir_selection_count++;
        } else {
          odb.put_float(k_dd, values[j]);
        }
      }
      break;

      // 011001 WIND DIRECTION
    case 11001:
      if (values[j] < bufrMissing) {
        odb.put_new_float(k_dd, values[j]);
      }
      break;

      //   11012  WIND SPEED AT 10 M, m/s
    case 11012:
      if (values[j] < bufrMissing) {
        if ( selected_wind_vector_ambiguities ) {
          //METLIBS_LOG_DEBUG("wind_speed_selection_count:"<<wind_speed_selection_count);
          if ( selected_wind_vector_ambiguities == wind_speed_selection_count ) {
            odb.put_float(k_ff, values[j]);
            //  METLIBS_LOG_DEBUG("ff:"<<values[j]);
          }
          wind_speed_selection_count++;
        } else {
          odb.put_float(k_ff, values[j]);
        }
      }
      break;

      // 011002 WIND SPEED
    case 11002:
      if (values[j] < bufrMissing)
        odb.put_new_float(k_ff, values[j]);
      break;

      // 011016 EXTREME COUNTERCLOCKWISE WIND DIRECTION OF A VARIABLE WIND
    case 11016:
      if (values[j] < bufrMissing)
        odb.put_float(k_dndndn, values[j]);
      break;

      // 011017 EXTREME CLOCKWISE WIND DIRECTIONOF A VARIABLE WIND
    case 11017:
      if (values[j] < bufrMissing)
        odb.put_float(k_dxdxdx, values[j]);
      break;

      // 011041 MAXIMUM WIND SPEED (GUSTS), m/s
    case 11041:
      if (values[j] < bufrMissing) {
        //        odb.put_float(k_911ff, values[j]);
        odb.put_float(k_fmfm, values[j]); // metar;
        if (timePeriodMinute == -10) {
          odb.put_float(k_911ff_10, values[j]);
        } else if (timePeriodMinute == -60) {
          odb.put_float(k_911ff_60, values[j]);
        } else if (timePeriodMinute == -180) {
          odb.put_float(k_911ff_180, values[j]);
        } else if (timePeriodMinute == -360) {
          odb.put_float(k_911ff_360, values[j]);
        }
      }
      break;

      // 011042 MAXIMUM WIND SPEED (10 MIN MEAN WIND), m/s
    case 11042:
      if (values[j] < bufrMissing) {
        //        odb.put_float(k_fxfx, values[j]);
        if (timePeriodMinute == -60) {
          odb.put_float(k_fxfx_60, values[j]);
        } else if (timePeriodMinute == -180) {
          odb.put_float(k_fxfx_180, values[j]);
        } else if (timePeriodMinute == -360) {
          odb.put_float(k_fxfx_360, values[j]);
        }
      }
      break;

      //   12104  DRY BULB TEMPERATURE AT 2M (16 bits), K->Celsius
      //   12004  DRY BULB TEMPERATURE AT 2M (12 bits), K->Celsius
    case 12104:
    case 12004:
      if (values[j] < bufrMissing)
        odb.put_float(k_TTT, values[j] - t0);
      break;
      //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits), K->Celsius
      //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits), K->Celsius
    case 12101:
    case 12001:
      if (values[j] < bufrMissing) {
        odb.put_new_float(k_TTT, values[j] - t0);
      }
      break;

      //   12106  DEW POINT TEMPERATURE AT 2M (16 bits), K->Celsius
      //   12006  DEW POINT TEMPERATURE AT 2M (12 bits), K->Celsius
    case 12106:
    case 12006:
      if (values[j] < bufrMissing)
        odb.put_float(k_TdTdTd, values[j] - t0);
      break;
      //   12103  DEW POINT TEMPERATURE (16 bits), K->Celsius
      //   12003  DEW POINT TEMPERATURE (12 bits), K->Celsius
    case 12103:
    case 12003:
      if (values[j] < bufrMissing)
        odb.put_new_float(k_TdTdTd, values[j] - t0);
      break;

      //   12014  MAX TEMPERATURE AT 2M, K->Celsius
    case 12014:
      if (values[j] < bufrMissing && hour == 18)
        odb.put_float(k_TxTn, values[j] - t0);
      break;
      //   12111  MAX TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (16 bits), K->Celsius
      //   12011  MAX TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (12 bits), K->Celsius
    case 12111:
    case 12011:
      if (values[j] < bufrMissing && hour == 18 && timePeriodHour == -12 && timeDisplacement == 0)
        odb.put_new_float(k_TxTn, values[j] - t0);
      break;

      //   12015  MIN TEMPERATURE AT 2M, K->Celsius
    case 12015:
      if (values[j] < bufrMissing && hour == 6)
        odb.put_float(k_TxTn, values[j] - t0);
      break;

      //   12112  MIN TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (16 bits), K->Celsius
      //   12012  MIN TEMPERATURE AT HEIGHT AND OVER PERIOD SPECIFIED (12 bits), K->Celsius
    case 12112:
    case 12012:
      if (values[j] < bufrMissing && hour == 6 && timePeriodHour == -12 && timeDisplacement == 0)
        odb.put_new_float(k_TxTn, values[j] - t0);
      break;

      //   13013  Snow depth
    case 13013:
      if (values[j] < bufrMissing) {
        // note: -0.01 means a little (less than 0.005 m) snow
        //       -0.02 means snow cover not continuous
        if (values[j] < 0) {
          odb.put_float(k_sss, 0.);
        } else {
          odb.put_float(k_sss, values[j] * 100);
        }
      }
      break;
      //   20062 State of the ground (with or without snow)
    case 20062:
      if (values[j] < 13) // Indicates no snow or partially snow cover
        odb.put_new_float(k_sss, 0.);
      break;
      //   13218 Ind. for snow-depth-measurement - DNMI
    case 13218: //   0 -> MEASURED IN METRES
      //   1 -> 997     (less than 5mm)
      //   2 -> 998     (partially snow cover)
      //   3 -> 999     (measurement inaccurate or impossible)
      //   4 -> MISSING
      if (int(values[j]) == 1 || int(values[j]) == 2)
        odb.put_new_float(k_sss, 0.);
      break;

      //13019 Total precipitation past 1 hour
    case 13019:
      if (values[j] < bufrMissing) {
        odb.put_float(k_RRR_1, values[j]);
      }
      break;

      //13020 Total precipitation past 3 hour
    case 13020:
      if (values[j] < bufrMissing) {
        odb.put_float(k_RRR_3, values[j]);
      }
      break;

      //13021 Total precipitation past 6 hour
    case 13021:
      if (values[j] < bufrMissing) {
        odb.put_float(k_RRR_6, values[j]);
      }
      break;

      //13022 Total precipitation past 12 hour
    case 13022:
      if (values[j] < bufrMissing) {
        odb.put_float(k_RRR_12, values[j]);
      }
      break;

      //13023 Total precipitation past 24 hour
    case 13023:
      if (values[j] < bufrMissing) {
        odb.put_float(k_RRR_24, values[j]);
      }
      break;

      // 13011 Total precipitation / total water equivalent of snow
    case 13011:
      if (values[j] < bufrMissing) {
        //        odb.put_float(k_RRR_accum, -1 * timePeriodHour);
        //        odb.put_float(k_RRR, values[j]);
        if (timePeriodHour == -24 ) {
          odb.put_float(k_RRR_24, values[j]);
        } else if (timePeriodHour == -12 ) {
          odb.put_float(k_RRR_12, values[j]);
        } else if (timePeriodHour == -6 ) {
          odb.put_float(k_RRR_6, values[j]);
        } else if (timePeriodHour == -3 ) {
          odb.put_float(k_RRR_3, values[j]);
        } else if (timePeriodHour == -1 ) {
          odb.put_float(k_RRR_1, values[j]);
        }
      }
      break;

      //   20001  HORIZONTAL VISIBILITY M
    case 20001:
      if (values[j] > 0 && values[j] < bufrMissing) {
        odb.put_float(k_VV, values[j]);
        //Metar
        if (values[j] > 9999) //remove VVVV=0
          odb.put_float(k_VVVV, 9999);
        else
          odb.put_float(k_VVVV, values[j]);
      }
      break;

      // 5021 BEARING OR AZIMUTH, DEGREE
    case 5021: //Metar
      if (values[j] < bufrMissing)
        odb.put_float(k_Dv, values[j]);
      break;

      //  20003  PRESENT WEATHER, CODE TABLE  20003
    case 20003:
      if (values[j] < 200) //values>200 -> w1w1, ignored here
        odb.put_float(k_ww, values[j]);
      break;

      //  20004 PAST WEATHER (1),  CODE TABLE  20004
    case 20004:
      if (values[j] > 2 && values[j] < 20 && values[j] != 10)
        odb.put_float(k_W1, values[j]);
      break;

      //  20005  PAST WEATHER (2),  CODE TABLE  20005
    case 20005:
      if (values[j] > 2 && values[j] < 20 && values[j] != 10)
        odb.put_float(k_W2, values[j]);
      break;

      //   Clouds

      // 020009 GENERAL WEATHER INDICATOR (TAF/METAR)
    case 20009:
      if (values[j] < bufrMissing)
        odb.metar().CAVOK = (int(values[j]) == 2);
      break;

      // 20013 HEIGHT OF BASE OF CLOUD (M)
    case 20013:
      if (values[j] < bufrMissing) {
        odb.put_new_float(k_h, height_of_clouds(values[j]));
        cloudStr += cloudHeight(int(values[j]));
      }
      break;

      // 20010 CLOUD COVER (TOTAL %)
      // Note: 113 means "Sky obscured by fog and/other meteorological
      // phenomena" but Hirlam BUFR uses 109 for this. Corresponds to
      // N=9 in synop.
    case 20010:
      if (values[j] < bufrMissing) {
        if (values[j] == 109 || values[j] == 113) {
          odb.put_float(k_N, 9);
        } else {
          odb.put_float(k_N, values[j] / 12.5); //% -> oktas;
        }
      }
      break;

      //20011  CLOUD AMOUNT
    case 20011:
      if (values[j] < bufrMissing) {
        if (i < 32)
          odb.put_float(k_Nh, values[j]);
        if (not cloudStr.empty()) {
          odb.metar().cloud.push_back(cloudStr);
          cloudStr.clear();
        }
        cloudStr = cloudAmount(int(values[j]));
      }
      break;

      // 20012 CLOUD TYPE, CODE TABLE  20012
    case 20012:
      if (values[j] < bufrMissing) {
        cloud_type(odb, values[j]);
        cloudStr += cloud_TCU_CB(int(values[j]));
      }
      break;

      // 020019 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5
    case 20019: {
      int index = int(values[j]) / 1000 - 1;
      std::string ww = substr(cvals, index, 9);
      miutil::trim(ww);
      if (not ww.empty())
        odb.metar().ww.push_back(ww);
    }
    break;

    // 020020 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5
    case 20020: {
      int index = int(values[j]) / 1000 - 1;
      std::string REww = substr(cvals, index, 4);
      miutil::trim(REww);
      if (not REww.empty())
        odb.metar().REww.push_back(REww);
    }
    break;

    case 21102:
      if ( values[j] < bufrMissing ) {
        selected_wind_vector_ambiguities = values[j];
        //METLIBS_LOG_DEBUG("selected_wind_vector_ambiguities:"<<selected_wind_vector_ambiguities);
      }
      break;

      // 022011 PERIOD OF WAVES, s
    case 22011:
      if (values[j] < bufrMissing)
        odb.put_float(k_PwaPwa, values[j]);
      break;

      // 022021 HEIGHT OF WAVES, m
    case 22021:
      if (values[j] < bufrMissing)
        odb.put_float(k_HwaHwa, values[j]);
      break;

    // 022012 PERIOD OF WIND WAVES, s
    case 22012:
      if (values[j] < bufrMissing)
        odb.put_float(k_PwPw, values[j]);
      break;

    // 022022 HEIGHT OF WIND WAVES, m
    case 22022:
      if (values[j] < bufrMissing)
        odb.put_float(k_HwHw, values[j]);
      break;

    // 022013 PERIOD OF SWELL WAVES, s (first system of swell)
    case 22013:
      if (values[j] < bufrMissing)
        odb.put_new_float(k_Pw1Pw1, values[j]);
      break;

      // 022023 HEIGHT OF SWELL WAVES, m
    case 22023:
      if (values[j] < bufrMissing)
        odb.put_new_float(k_Hw1Hw1, values[j]);
      break;

    // 022070 SIGNIFICANT WAVE HEIGHT, m
    case 22070:
      if (values[j] < bufrMissing)
        odb.put_new_float(k_SWH, values[j]);
      break;

    // 022003 DIRECTION OF SWELL WAVES, DEGREE
    case 22003:
      if (values[j] > 0 && values[j] < bufrMissing)
        odb.put_new_float(k_dw1dw1, values[j]);
      break;

      //   22042  SEA/WATER TEMPERATURE, K->Celsius
    case 22042:
    case 22049:
      if (values[j] < bufrMissing)
        odb.put_float(k_TwTwTw, values[j] - t0);
      break;

      //   22043  SEA/WATER TEMPERATURE, K->Celsius
    case 22043:
      if (values[j] < bufrMissing && depth < 1) //??
        odb.put_new_float(k_TwTwTw, values[j] - t0);
      break;

      // 022061 STATE OF THE SEA, CODE TABLE  22061
    case 22061:
      if (values[j] < bufrMissing)
        odb.put_float(k_s, values[j]);
      break;

      // 022038 TIDE
    case 22038:
      if (values[j] < bufrMissing)
        odb.put_float(k_TE, values[j]);
      break;

    case 25053:
      if (values[j] < bufrMissing)
        odb.put_float(k_quality, values[j]);
      break;
    }
  }


  if (wmoNumber) {
    format_wmo(odb.basic().id, wmoBlock, wmoSubarea, wmoStation);
  }

  //Metar cloud
  if (not cloudStr.empty()) {
    odb.metar().cloud.push_back(cloudStr);
    cloudStr.clear();
  }

  //PRESSURE TENDENCY - ppp may or may not include sign, if a>4 then ppp<0
  const float *p_ppp, *p_a;
  if ((p_ppp = odb.get_float(k_ppp)) && (p_a = odb.get_float(k_a)) && (*p_a > 4) && (*p_ppp > 0)) {
    odb.put_float(k_ppp, *p_ppp * -1);
  }

  // when there are no clouds, height might be reported as 0m,
  // but this should be category 9, not 0
  const float *p_N, *p_h;
  if ((p_N = odb.get_float(k_N)) && (p_h = odb.get_float(k_h)) && (*p_N == 0) && (p_h == 0)) {
    odb.put_float(k_h, 9);
  }
  if (!odb.basic().id.empty())
    odb.put_string(k_Id, odb.basic().id);
  if (!d_name.empty())
    odb.put_string(k_Name, d_name);

  //TIME
  if ( miTime::isValid(year, month, day, hour, minute, 0) ) {
    odb.basic().obsTime = miTime(year, month, day, hour, minute, 0);
    put_date_time(odb, month, day, hour, minute);
  }

  //skip obs if xpos or ypos  or obsTime not ok
  if (odb.basic().xpos > -32767 && odb.basic().ypos > -32767 && !odb.basic().obsTime.undef()) {
    return true;
  }
  return false;
}

// ########################################################################

ObsBufr::SubsetResult StationBufr::handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                                    const char* cvals, int subset, int kelem)
{
  get_station_info(ktdexl, ktdexp, values, cvals, subset, kelem);
  return BUFR_CONTINUE;
}

void StationBufr::get_station_info(int ktdexl, const int *ktdexp, const double* values,
                                   const char* cvals, int subset, int kelem)
{
  METLIBS_LOG_SCOPE();
  int wmoBlock = 0;
  int wmoSubarea = 0;
  int wmoStation = 0;
  std::string station;
  bool wmoNumber = false;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {

    switch (ktdexp[i]) {

    //   1001  WMO BLOCK NUMBER
    case 1001:
      if (values[j] < bufrMissing)
        wmoBlock = int(values[j]);
      else
        METLIBS_LOG_DEBUG("1001 WMO block number == missing!");
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      if (values[j] < bufrMissing) {
        wmoStation = int(values[j]);
        wmoNumber = true;
      } else
        METLIBS_LOG_DEBUG("1002 WMO station number == missing!");
      break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1011:
    case 1194:
      if (!wmoNumber) {
        int index = int(values[j]) / 1000 - 1;
        add_substr(station, cvals, index, 6);
      }
    break;

      //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
      //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      latitude.push_back(values[j]);
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      longitude.push_back(values[j]);
      break;
    }
  }

  if (wmoNumber) {
    format_wmo(station, wmoBlock, wmoSubarea, wmoStation);
  }

  idmap_t::iterator itS = idmap.find(station);
  if (itS != idmap.end()) {
    std::ostringstream ostr;
    ostr << station << "(" << itS->second << ")";
    id.push_back(ostr.str());
    itS->second++;
  } else {
    id.push_back(station);
    idmap.insert(std::make_pair(station, 1));
  }
}

// ########################################################################

bool ObsDataBufr::get_diana_data_level(int ktdexl, const int* ktdexp, const double* values, const char* cvals, int subset, int kelem)
{
  using miutil::constants::t0;

  std::string d_name;

  int wmoBlock = 0;
  int wmoSubarea = 0;
  int wmoStation = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  bool wmoNumber = false;

  int levelmin = level - level/20;
  int levelmax = level + level/20;

  bool found = false;
  bool stop = false;
  bool replication = false;
  bool is_amv = false;
  bool found_amv_speed = false;
  bool found_amv_direction = false;
  bool checked_amv_pppp = false;
  int found_amv_conf = 0;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {

    if (!replication // station info
        || found // right pressure level
        || ktdexp[i] == 7004  // next pressure level
        || ktdexp[i] == 7062) { // next sea level
      switch (ktdexp[i]) {

      //   031001 DELAYED DESCRIPTOR REPLICATION FACTOR [NUMERIC]
      case 31001:
        replication=true;
        break;

        //   1001  WMO BLOCK NUMBER
      case 1001:
        if (values[j] < bufrMissing) {
          wmoBlock = int(values[j]);
        }
        break;

        //   1002  WMO STATION NUMBER
      case 1002:
        if (values[j] < bufrMissing) {
          wmoStation = int(values[j]);
          wmoNumber = true;
        }
        break;

        //   1005 BUOY/PLATFORM IDENTIFIER [NUMERIC]
      case 1005:
        if (values[j] < bufrMissing) {
          odb.basic().id = miutil::from_number(int(values[j]));
          odb.metar().ship_buoy = true;
        }
        break;

        //   001007 SATELLITE IDENTIFIER [CODE TABLE]
      case 1007:
      {
        if ( !wmoNumber ) {
          int index = int(values[j]) / 1000 - 1;
          add_substr(odb.basic().id, cvals, index, 4);
        }
      }
      break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1011:
      {
        if ( !wmoNumber ) {
          int index = int(values[j]) / 1000 - 1;
          add_substr(odb.basic().id, cvals, index, 4);
          odb.metar().ship_buoy = true;
        }
      }
      break;

      //   4001  YEAR
      case 4001:
        if (values[j] < bufrMissing)
        {
          year = int(values[j]);
        }
        break;

        //   4002  MONTH
      case 4002:
        if (values[j] < bufrMissing)
        {
          month = int(values[j]);
        }
        break;

        //   4003  DAY
      case 4003:
        if (values[j] < bufrMissing)
        {
          day = int(values[j]);
        }
        break;

        //   4004  HOUR
      case 4004:
        if (values[j] < bufrMissing) {
          hour = int(values[j]);
        }
        break;

        //   4005  MINUTE
      case 4005:
        if (values[j] < bufrMissing) {
          minute = int(values[j]);
        }
        break;

        //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
        //   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:
        odb.basic().ypos = values[j];
        odb.put_float(k_lat, odb.basic().ypos);
        break;

        //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
        //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
        odb.basic().xpos = values[j];
        odb.put_float(k_lon, odb.basic().xpos);
        break;

      case 2023:
        if ( values[j] >= 0 && values[j] < 16)
        {
          is_amv = true;
        }
        break;
        //   7001  HEIGHT OF STATION, M
      case 7001:
        if (values[j] < bufrMissing)
          odb.put_float(k_Height, values[j]);
        break;

        //   007004  PRESSURE, Pa->hPa
      case 7004:
        if (!is_amv && found) {
          stop = true;
        } else {
          if (values[j] < bufrMissing && (!is_amv || (is_amv && !checked_amv_pppp))) {
            const float pppp_hPa = values[j] * pa2hpa;
            if (int(pppp_hPa) > levelmin && int(pppp_hPa) < levelmax) {
              odb.put_float(k_PPPP, pppp_hPa);
              found = true;
            }
          }
          if (is_amv)
            checked_amv_pppp = true;
        }
        break;

        //   007062  DEPTH BELOW SEA/WATER SURFACE [M]
      case 7062:
        if (found) {
          stop = true;
        } else {
          if (values[j] < bufrMissing) {
            if (int(values[j]) > levelmin && int(values[j]) < levelmax) {
              found = true;
              odb.put_float(k_depth, values[j]);
            }
          }
        }
        break;

        // 011001 WIND DIRECTION
      case 11001:
        if (values[j] < bufrMissing) {
          if ( is_amv )
          {
            if ( not found_amv_direction )
            {
              odb.put_float(k_dd, values[j]);
              found_amv_direction = true;
            }
          }
          else
            odb.put_float(k_dd, values[j]);
        }
        break;

        // 011002 WIND SPEED
      case 11002:
        if (values[j] < bufrMissing) {
          if ( is_amv )
          {
            if ( not found_amv_speed )
            {
              odb.put_float(k_ff, values[j]);
              found_amv_speed = true;
            }
          }
          else
            odb.put_float(k_ff, values[j]);
        }
        break;

        //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits), K->Celsius
        //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits), K->Celsius
      case 12101:
      case 12001:
        if (values[j] < bufrMissing){
          odb.put_float(k_TTT, values[j] - t0);
        }
        break;

        //   12103  DEW POINT TEMPERATURE (16 bits), K->Celsius
        //   12003  DEW POINT TEMPERATURE (12 bits), K->Celsius
      case 12103:
      case 12003:
        if (values[j] < bufrMissing)
          odb.put_float(k_TdTdTd, values[j] - t0);
        break;

        //   10008 GEOPOTENTIAL (20 bits), M**2/S**2
        //   10003 GEOPOTENTIAL (17 bits), M**2/S**2
      case 10008:
      case 10003:
        if (values[j] < bufrMissing)
          odb.put_float(k_HHH, values[j] / miutil::constants::g);
        break;
        //   10009 GEOPOTENTIAL HEIGHT
      case 10009:
        if (values[j] < bufrMissing)
          odb.put_float(k_HHH, values[j]);
        break;

        // 022011  PERIOD OF WAVES [S]
      case 22011:
        if (values[j] < bufrMissing)
          odb.put_float(k_PwaPwa, values[j]);
        break;

        // 022021  HEIGHT OF WAVES [M]
      case 22021:
        if (values[j] < bufrMissing)
          odb.put_float(k_HwaHwa, values[j]);
        break;

        // 22043 SEA/WATER TEMPERATURE [K]
      case 22043:
        if (values[j] < bufrMissing)
          odb.put_float(k_TTTT, values[j] - t0);
        break;

        // 22062 SALINITY [PART PER THOUSAND]
      case 22062:
        if (values[j] < bufrMissing)
          odb.put_float(k_SSSS, values[j]);
        break;

      case 33007:
        if ( values[j] < bufrMissing)
        {
          if ( found_amv_conf == 0 )
            odb.put_float(k_QI, values[j]);
          else if ( found_amv_conf == 4 )
            odb.put_float(k_QI_NM, values[j]);
          else if ( found_amv_conf == 8 )
            odb.put_float(k_QI_RFF, values[j]);
        }
        found_amv_conf++;
        break;
      }
    }

    // right pressure level found and corresponding parameters read
    if (stop)
      break;
  }

  //right pressure level not found
  if (!found){
    return false;
  }
  if (wmoNumber) {
    format_wmo(odb.basic().id, wmoBlock, wmoSubarea, wmoStation);
  }

  if (!odb.basic().id.empty())
    odb.put_string(k_Id, odb.basic().id);
  if (!d_name.empty())
    odb.put_string(k_Name, d_name);
  //TIME
  odb.basic().obsTime = miTime(year, month, day, hour, minute, 0);
  put_date_time(odb, month, day, hour, minute);

  return true;
}

// ########################################################################

ObsBufr::SubsetResult VprofBufr::handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                                  const char* cvals, int subset, int kelem)
{
  // will return without reading more subsets, fix later
  if (!get_data_level(ktdexl, ktdexp, values, cvals, subset, kelem))
    return BUFR_OK;
  else
    return BUFR_ERROR;
}

bool VprofBufr::setVerticalValue(float& vertical_value, float bufr_value, float scale_factor)
{
  if (bufr_value < bufrMissing) {
    vertical_value = bufr_value * scale_factor;
    if (vertical_axis_ == PRESSURE) {
      return (vertical_value > 0 && vertical_value < 1300);
    } else if (vertical_axis_ == ALTITUDE) {
      return (vertical_value > -500 && vertical_value < 2e5);
    }
  }
  vertical_value = -12345;
  return false;
}

bool VprofBufr::get_data_level(int ktdexl, const int *ktdexp, const double* values,
                               const char* cvals, int subset, int kelem)
{
  METLIBS_LOG_SCOPE();
  using miutil::constants::t0;

  //  int wmoBlock = 0;
  //  int wmoStation = 0;
  std::string station;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  float vertical = -12345;
  bool vertical_ok = false;
  float tt = -30000, td = -30000, dd = -1, ff = -1;
  int wind_lvl = 0;
  float lat = 0., lon = 0.;
  float ffmax = -1;
  int kmax = -1;

  static int ii = 0;

  bool station_found = false;

  for (int i = 0, j = kelem * subset; i < ktdexl; i++, j++) {
    const int bufr_element = ktdexp[i];
    const double bufr_value = values[j];

    const bool element_is_pressure = (vertical_axis_ == PRESSURE) && (bufr_element == 7004    // PRESSURE
                                                                      || bufr_element == 7011 // PRESSURE (HIGH PRECISION)
                                                                      );
    const bool element_is_altitude = (vertical_axis_ == ALTITUDE) && (bufr_element == 7002     // HEIGHT OR ALTITUDE
                                                                      || bufr_element == 7003  // GEOPOTENTIAL
                                                                      || bufr_element == 7008  // GEOPOTENTIAL (HIGHER PRECISION)
                                                                      || bufr_element == 7009  // GEOPOTENTIAL HEIGHT
                                                                      || bufr_element == 10003 // GEOPOTENTIAL (non-coordinate)
                                                                      || bufr_element == 10008 // GEOPOTENTIAL (non-coordinate, HIGHER PRECISION)
                                                                      || bufr_element == 10009 // GEOPOTENTIAL HEIGHT (non-coordinate)
                                                                      );
    const bool element_is_z = element_is_pressure || element_is_altitude;

    if (bufr_element < 7000 // station info
        || vertical_ok      // vertical coordinate found in previous BUFR element
        || element_is_z)    // next vertical level
    {
      if (element_is_z) { // new vertical level, save data
        if (!station_found)
          return false;

        if (vertical_ok)
          addValues(vertical, tt, td, dd, ff, wind_lvl, ffmax, kmax);
      }

      switch (bufr_element) {

      //   1001  WMO BLOCK NUMBER
      case 1001: {
        if (izone != int(bufr_value))
          return false;
        station_found = true;
        break;
      }

      //   1002  WMO STATION NUMBER
      case 1002: {
        if (istation != int(bufr_value))
          return false;
        if (index != ii) {
          ii++;
          return false;
        }
        ii = 0;
        station_found = true;
        break;
      }

      // 1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1006:
      case 1011:
      case 1194: {
        if (!station_found) {
          int iindex = int(bufr_value) / 1000 - 1;
          station = substr(cvals, iindex, 6);
          miutil::trim(station);
          if (strStation != station)
            return false;
          if (index != ii) {
            ii++;
            return false;
          }
          if (!station.empty())
            ii=0;
          station_found = true;
        }
        break;
      }

      //   4001  YEAR
      case 4001:
        year = int(bufr_value);
        break;

        //   4002  MONTH
      case 4002:
        month = int(bufr_value);
        break;

        //   4003  DAY
      case 4003:
        day = int(bufr_value);
        break;

        //   4004  HOUR
      case 4004:
        hour = int(bufr_value);
        break;

        //   4005  MINUTE
      case 4005:
        minute = int(bufr_value);
        break;

        //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
        //   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:
        lat = bufr_value;
        break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
        lon = bufr_value;
        break;

      case 7004: // PRESSURE (Pa)
      case 7011: // PRESSURE (HIGH PRECISION) (Pa)
        if (vertical_axis_ == PRESSURE)
          vertical_ok = setVerticalValue(vertical, bufr_value, pa2hpa); // convert from Pa to hPa
        break;

      case 7002: // HEIGHT OR ALTITUDE / m
        if (vertical_axis_ == ALTITUDE)
          vertical_ok = setVerticalValue(vertical, bufr_value);
        break;

      case 7003:  // GEOPOTENTIAL
      case 7008:  // GEOPOTENTIAL (HIGHER PRECISION)
      case 10003: // GEOPOTENTIAL (non-coordinate)
      case 10008: // GEOPOTENTIAL (non-coordinate, HIGHER PRECISION)
        // value in  m**2/s**2, convert to m (approximate)
        if (vertical_axis_ == ALTITUDE)
          vertical_ok = setVerticalValue(vertical, bufr_value, 1 / miutil::constants::g);
        break;

      case 7009:  // GEOPOTENTIAL HEIGHT / m
      case 10009: // GEOPOTENTIAL HEIGHT (non-coordinate) / m
        if (vertical_axis_ == ALTITUDE)
          vertical_ok = setVerticalValue(vertical, bufr_value);
        break;

      case 8001: // VERTICAL SOUNDING SIGNIFICANCE
        if (bufr_value < bufrMissing) {
          const int flags = int(bufr_value);
          if (flags & 0x2) // significant wind level
            wind_lvl = 1;
        }
        break;

      case 8042: // EXTENDED VERTICAL SOUNDING SIGNIFICANCE
        if (bufr_value < bufrMissing) {
          const int flags = int(bufr_value);
          if (flags & 0x800) // significant wind level
            wind_lvl = 1;
        }
        break;

        //   11001  WIND DIRECTION
      case 11001:
        if (bufr_value < bufrMissing)
          dd = bufr_value; // compass degrees
        break;

        //   11002  WIND SPEED
      case 11002:
        if (bufr_value < bufrMissing)
          ff = bufr_value; // m/s
        break;

      //   12101  TEMPERATURE/DRY BULB TEMPERATURE (16 bits)
      //   12001  TEMPERATURE/DRY BULB TEMPERATURE (12 bits)
      case 12101:
      case 12001:
        if (bufr_value < bufrMissing)
          tt = bufr_value - t0; // convert from K to degC
        break;

      //   12103  DEW POINT TEMPERATURE (16 bits)
      //   12003  DEW POINT TEMPERATURE (12 bits)
      case 12103:
      case 12003:
        if (bufr_value < bufrMissing)
          td = bufr_value - t0; // convert from K to degC
        break;
      }
    }

    // right pressure level found and corresponding parameters read
  }

  if (vertical_ok)
    addValues(vertical, tt, td, dd, ff, wind_lvl, ffmax, kmax);

  vplot->text.posName = strStation;
  miutil::trim(vplot->text.posName);
  vplot->text.prognostic = false;
  vplot->text.forecastHour = 0;
  vplot->text.validTime = miTime(year, month, day, hour, minute, 0);
  vplot->text.latitude = lat;
  vplot->text.longitude = lon;
  vplot->text.kindexFound = false;
  vplot->text.realization = -1;

  if (kmax >= 0)
    wind_sig->setX(kmax, 3);

  vplot->prognostic = false;

  return true;
}

void VprofBufr::addValues(float z, float& tt, float& td, float& dd, float& ff, int& wind_lvl, float& ffmax, int& kmax)
{
  if (tt > -30000.) {
    temperature->add(z, tt);
    tt = -31000.;
    if (td > -30000.) {
      dewpoint_temperature->add(z, td);
      td = -31000.;
    }
  }

  if (dd < 0 || dd > 360 || ff < 0)
    return;

  wind_dd->add(z, dd);
  wind_ff->add(z, ff);
  wind_sig->add(z, wind_lvl);
  if (ff > ffmax) {
    ffmax = ff;
    kmax = wind_sig->length() - 1;
  }
  dd = ff = -1;
  wind_lvl = 0;
}
