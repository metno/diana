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

using namespace std;

// libemos interface (to fortran routines)
extern "C" {
  extern void pbopen_(int* iunit, const char* bufr_file, const char* rw,
		      int* iret, int len_bufr_file, int len_bufr_access);
  extern void pbclose_(int* iunit, int* iret);
  extern void pbbufr_(int* iunit, int* ibuff, int* ibflen, int* ilen, int* iret);
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

}

class VprofPlot;

/**
  \brief Reading BUFR observation files

   using the ECMWF emos library (libemos)
*/
class ObsBufr {

private:

  bool BUFRdecode(int* ibuff, int ilen, const miutil::miString& format);
  bool get_diana_data(int ktdexl, int *ktdexp, double* values,
		      const char cvals[][80], int len_cvals, 
		      int subset, int kelem, ObsData &d);

  bool get_station_info(int ktdexl, int *ktdexp, double* values,
			const char cvals[][80], int len_cvals,
			int subset, int kelem);

  bool get_diana_data_level(int ktdexl, int *ktdexp, double* values,
			    const char cvals[][80], int len_cvals, 
			    int subset, int kelem, ObsData &d,
			    int level);

  bool get_data_level(int ktdexl, int *ktdexp, double* values,
		      const char cvals[][80], int len_cvals, 
		      int subset, int kelem, miutil::miTime time);

  miutil::miString cloudAmount(int i);
  miutil::miString cloudHeight(int i);
  miutil::miString cloud_TCU_CB(int i);
  float height_of_clouds(double height);
  void cloud_type(ObsData& d, double v);
  float ms2code4451(float v);

  miutil::miTime obsTime;
  VprofPlot *vplot;
  ObsPlot   *oplot;
  map<miutil::miString,int> idmap;
  vector<miutil::miString> id;
  vector<miutil::miTime> id_time;
  vector<float> latitude;
  vector<float> longitude;
  int izone;
  int istation;
  int index;
  miutil::miString strStation;

public:
  ObsBufr(){;}
  bool init(const miutil::miString& filename, const miutil::miString& format);
  bool ObsTime(const miutil::miString& filename,miutil::miTime& time);
  bool readStationInfo(const vector<miutil::miString>& bufr_file,
      vector<miutil::miString>& namelist,
      vector<miutil::miTime>& timelist,
      vector<float>& latitudelist,
      vector<float>& longitudelist);
  VprofPlot* getVprofPlot(const vector<miutil::miString>& bufr_file,
      const miutil::miString& modelName,
      const miutil::miString& station,
      const miutil::miTime& time);
  ObsPlot*   getObsPlot(){return oplot;}
  void setObsPlot(ObsPlot* op){oplot=op;}
};

#endif

