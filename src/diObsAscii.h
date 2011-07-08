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
#ifndef diObsAscii_h
#define diObsAscii_h

#include <puTools/miString.h>
#include <puTools/miTime.h>

class ObsPlot;

using namespace std;


/**

  \brief Reading ascii observation files 
  
  In-house met.no format

*/
class ObsAscii {
private:

  miutil::miString separator;

  void readFile(const miutil::miString &filename, const miutil::miString &headerfile,
		const miutil::miTime &filetime, ObsPlot *oplot, bool readData);

  void decodeHeader(ObsPlot *oplot, vector<miutil::miString> lines);
  void decodeData(ObsPlot *oplot, vector<miutil::miString> lines, const miutil::miTime &filetime);

  void getFromFile(const miutil::miString &filename, vector<miutil::miString>& lines);
  void getFromHttp(const miutil::miString &url, vector<miutil::miString>& lines);

public:
  ObsAscii(const miutil::miString &filename, const miutil::miString &headerfile,
	   const miutil::miTime &filetime, ObsPlot *oplot, bool readData);

};

#endif




