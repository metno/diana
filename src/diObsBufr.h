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
#ifndef _diObsBufr_h
#define _diObsBufr_h

#include <diObsPlot.h>

class VprofPlot;

/**
  \brief Reading BUFR observation files

   using the ECMWF emos library (libemos)
*/
class ObsBufr {

private:
  enum Format { FORMAT_STATIONINFO, FORMAT_VPROFPLOT, FORMAT_OBSPLOT };
  bool BUFRdecode(int* ibuff, int ilen, Format format);
  bool get_diana_data(int ktdexl, int *ktdexp, double* values,
      const char* cvals, int subset, int kelem, ObsData &d);

  bool get_station_info(int ktdexl, int *ktdexp, double* values,
      const char* cvals, int subset, int kelem);

  bool get_diana_data_level(int ktdexl, int *ktdexp, double* values,
      const char* cvals, int subset, int kelem, ObsData &d, int level);

  bool get_data_level(int ktdexl, int *ktdexp, double* values,
      const char* cvals, int subset, int kelem, miutil::miTime time);

  bool init(const std::string& filename, Format format);

  std::string cloudAmount(int i);
  std::string cloudHeight(int i);
  std::string cloud_TCU_CB(int i);
  float height_of_clouds(double height);
  void cloud_type(ObsData& d, double v);
  float ms2code4451(float v);

  miutil::miTime obsTime;
  VprofPlot *vplot;
  ObsPlot   *oplot;
  std::map<std::string,int> idmap;
  std::vector<std::string> id;
  std::vector<miutil::miTime> id_time;
  std::vector<float> latitude;
  std::vector<float> longitude;
  int izone;
  int istation;
  int index;
  std::string strStation;

public:
  ObsBufr();

  bool ObsTime(const std::string& filename,miutil::miTime& time);
  bool readStationInfo(const std::vector<std::string>& bufr_file,
      std::vector<std::string>& namelist,
      std::vector<miutil::miTime>& timelist,
      std::vector<float>& latitudelist,
      std::vector<float>& longitudelist);
  VprofPlot* getVprofPlot(const std::vector<std::string>& bufr_file,
      const std::string& modelName,
      const std::string& station,
      const miutil::miTime& time);

  ObsPlot* getObsPlot()
    { return oplot; }
  bool setObsPlot(ObsPlot* op, const std::string& filename);
};

#endif
