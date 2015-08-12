// -*- c++ -*-
/*

  Diana - A Free Meteorological Visualisation Tool

  $Id: diTimeFilter.h 2934 2012-05-21 09:07:40Z davidb $

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
#ifndef TimeFilter_h
#define TimeFilter_h

#include "puTools/miTime.h"
#include "puTools/miClock.h"

#include <vector>

/**

  \brief get time info from file name

*/
class TimeFilter {
private:

  std::string::size_type yyyy,yy,mm,dd,HH,MM,SS;
  bool OK;
  bool noSlash;
  std::vector<bool> legalPos;
  bool advanced;

  void memberCopy(const TimeFilter& rhs);
  void replaceKey(std::string& str);
  std::string::size_type findPos(const std::string& filter, const std::string& s);
  bool getClock(std::string name, miutil::miClock &);

public:

  TimeFilter();

  ///remember time info, return filename whith time info replaced by *
  bool initFilter(std::string& filename, bool advanced_=false);
  ///returns true if initFilter found timeinfo in filename
  bool ok(void) {return OK;}
  ///find time from filename
  bool getTime(std::string name, miutil::miTime & t);

  std::string getTimeStr(std::string name);
};

#endif


