/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diObsData.h"
#include "diStationTypes.h"

class VprofValues;
typedef std::shared_ptr<VprofValues> VprofValues_p;

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
  bool BUFRdecode(int* ibuff, int ilen);
};

// ########################################################################

class ObsDataBufr : public ObsBufr {
public:
  ObsDataBufr(int level, const miutil::miTime& time, int timeDiff);
  bool getObsData(std::vector<ObsData>& obsp, const std::string& filename);

protected:
  SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                const char* cvals, int subset, int kelem) override;

private:
  bool get_diana_data(int ktdexl, const int *ktdexp, const double* values,
                      const char* cvals, int subset, int kelem, ObsData &d);

  bool get_diana_data_level(int ktdexl, const int *ktdexp, const double* values,
                            const char* cvals, int subset, int kelem, ObsData &d);

  bool timeOK(const miutil::miTime& t) const;

private:
  std::vector<ObsData> obsdata;

  int level;
  miutil::miTime time;
  int timeDiff;
};

// ########################################################################

class VprofBufr : public ObsBufr {
public:
  VprofValues_p getVprofPlot(const std::vector<std::string>& bufr_file, const std::string& modelName, const std::string& station);

protected:
  SubsetResult handleBufrSubset(int ktdexl, const int *ktdexp, const double* values,
                                const char* cvals, int subset, int kelem) override;

private:
  bool get_data_level(int ktdexl, const int *ktdexp, const double* values,
                      const char* cvals, int subset, int kelem);

private:
  VprofValues_p vplot;

  int izone;
  int istation;
  int index;
  std::string strStation;
};

// ########################################################################

class StationBufr : public ObsBufr {
public:
  bool readStationInfo(const std::vector<std::string>& bufr_file,
      std::vector<stationInfo>& stations,
      std::vector<miutil::miTime>& timelist);

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
  std::vector<miutil::miTime> id_time;
  std::vector<float> latitude;
  std::vector<float> longitude;
};

#endif
