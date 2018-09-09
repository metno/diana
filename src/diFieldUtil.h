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

#ifndef difieldutil_h
#define difieldutil_h

#include "util/diKeyValue.h"

#include <set>

class Field;
/*! Merge field options from command (e.g. quick menu) and setup/logfile.
 * \param fieldopts options from command, will be updated
 * \param defaultopts options from setup/logfile
 */
void mergeFieldOptions(miutil::KeyValue_v& fieldopts, miutil::KeyValue_v defaultopts);

bool splitDifferenceCommandString(const miutil::KeyValue_v& pin, miutil::KeyValue_v& fspec1, miutil::KeyValue_v& fspec2);

void cleanupFieldOptions(miutil::KeyValue_v& vpopt);

void makeFieldText(Field* fout, const std::string& plotName, bool flightlevel);

std::string getBestReferenceTime(const std::set<std::string>& refTimes, int refOffset, int refHour);

#endif // difieldutil_h
