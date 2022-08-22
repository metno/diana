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
#ifndef _diObsBufr_h
#define _diObsBufr_h

#include "diObsDataVector.h"
#include "diStationTypes.h"
#include "vprof/diVprofSimpleData.h"
#include "vprof/diVprofSimpleValues.h"

/**
  \brief Reading BUFR observation files

   using the ECMWF emos library (libemos)
*/
class ObsBufr {
protected:
  enum SubsetResult { BUFR_CONTINUE, BUFR_OK, BUFR_ERROR };
  struct Subset{
    int ktdexl;
    const int *ktdexp;
    const double* values;
    const char* cvals;
    int subset;
    int kelem;
  };

  ObsBufr();
  bool init(const std::string& filename);
  virtual SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                        const char* cvals, int subset, int kelem) = 0;

public:
  static bool ObsTime(const std::string& filename, miutil::miTime& time);

private:
  struct BUFRDecodeData;
  bool BUFRdecode(int* ibuff, int ilen, BUFRDecodeData& b);
};

// ########################################################################

class ObsDataBufr : public ObsBufr {
public:
  // clang-format-off
  enum BUFR_KEY {
    k_911ff,
    k_911ff_10,
    k_911ff_180,
    k_911ff_360,
    k_911ff_60,
    k_a,
    k_auto,
    k_Ch,
    k_Cl,
    k_Cm,
    k_Date,
    k_dd,
    k_depth,
    k_dndndn,
    k_ds,
    k_Dv,
    k_dw1dw1,
    k_dxdxdx,
    k_ff,
    k_fmfm,
    k_fxfx,
    k_fxfx_180,
    k_fxfx_360,
    k_fxfx_60,
    k_h,
    k_Height,
    k_HHH,
    k_Hw1Hw1,
    k_HwaHwa,
    k_HwHw,
    k_Id,
    k_lat,
    k_lon,
    k_N,
    k_Name,
    k_Nh,
    k_PHPHPHPH,
    k_ppp,
    k_PPPP,
    k_Pw1Pw1,
    k_PwaPwa,
    k_PwPw,
    k_QI,
    k_QI_NM,
    k_QI_RFF,
    k_quality,
    k_RRR,
    k_RRR_1,
    k_RRR_12,
    k_RRR_24,
    k_RRR_3,
    k_RRR_6,
    k_RRR_accum,
    k_s,
    k_sss,
    k_SSSS,
    k_SWH,
    k_TdTdTd,
    k_TE,
    k_Time,
    k_TTT,
    k_TTTT,
    k_TwTwTw,
    k_TxTn,
    k_vs,
    k_VV,
    k_VVVV,
    k_W1,
    k_W2,
    k_wmonumber,
    k_ww,
    BUFR_KEYS_END
  };
  // clang-format-on

  struct ObsDataB
  {
    ObsDataVector_p obsdata;
    size_t ki_BUFR[BUFR_KEYS_END];

    ObsDataB();
    ~ObsDataB();
    size_t push() { obsdata->push_back(); return obsdata->size(); }
    void pop() { obsdata->pop_back(); }

    size_t last() const { return obsdata->size() - 1; }

    ObsDataBasic& basic() { return obsdata->basic(last()); }
    ObsDataMetar& metar() { return obsdata->metar(last()); }
    const float* get_float(BUFR_KEY k) const { return obsdata->get_float(last(), ki_BUFR[k]); }
    void put_float(BUFR_KEY k, float v) { obsdata->put_float(last(), ki_BUFR[k], v); }
    void put_new_float(BUFR_KEY k, float v) { obsdata->put_new_float(last(), ki_BUFR[k], v); }
    void put_string(BUFR_KEY k, const std::string& v) { obsdata->put_string(last(), ki_BUFR[k], v); }
  };

  ObsDataBufr(int level, const miutil::miTime& time, int timeDiff);
  ObsDataVector_p getObsData(const std::string& filename);

protected:
  SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                const char* cvals, int subset, int kelem) override;

private:
  bool get_diana_data(int ktdexl, const int* ktdexp, const double* values, const char* cvals, int subset, int kelem);

  bool get_diana_data_level(int ktdexl, const int* ktdexp, const double* values, const char* cvals, int subset, int kelem);

  bool timeOK(const miutil::miTime& t) const;

private:
  ObsDataB odb;

  int level;
  miutil::miTime time;
  int timeDiff;
};

// ########################################################################

class VprofBufr : public ObsBufr {
public:
  enum VerticalAxis { PRESSURE, ALTITUDE };
  VprofValues_p getVprofPlot(const std::vector<std::string>& bufr_file, const std::string& station, VerticalAxis vertical_axis);

protected:
  SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                const char* cvals, int subset, int kelem) override;

private:
  bool get_data_level(int ktdexl, const int *ktdexp, const double* values,
                      const char* cvals, int subset, int kelem);
  bool setVerticalValue(float& vertical_value, float bufr_value, float scale_factor = 1);
  void addValues(float p, float& tt, float& td, float& dd, float& ff, int& bpart, float& ffmax, int& kmax);

private:
  VprofSimpleValues_p vplot;

  VprofSimpleData_p temperature, dewpoint_temperature, wind_dd, wind_ff, wind_sig;

  VerticalAxis vertical_axis_;
  int izone;
  int istation;
  int index;
  std::string strStation;
};

// ########################################################################

class StationBufr : public ObsBufr {
public:
  bool readStationInfo(const std::vector<std::string>& bufr_file, std::vector<stationInfo>& stations);

protected:
  SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                const char* cvals, int subset, int kelem) override;

private:
  void get_station_info(int ktdexl, const int *ktdexp, const double* values,
                        const char* cvals, int subset, int kelem);

private:
  typedef std::map<std::string,int> idmap_t;
  idmap_t idmap;
  std::vector<std::string> id;
  std::vector<float> latitude;
  std::vector<float> longitude;
};

#endif
