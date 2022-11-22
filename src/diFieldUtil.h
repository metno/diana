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

#ifndef difieldutil_h
#define difieldutil_h

#include "util/diKeyValue.h"
#include "diField/diFieldFwd.h"

#include <memory>
#include <set>

struct FieldRequest;

bool splitDifferenceCommandString(const miutil::KeyValue_v& pin, miutil::KeyValue_v& fspec1, miutil::KeyValue_v& fspec2);

void makeFieldText(Field_p fout, const std::string& plotName, bool flightlevel);

std::string getBestReferenceTime(const std::set<std::string>& refTimes, int refOffset, int refHour);

void flightlevel2pressure(FieldRequest& frq);

Field_p convertUnit(Field_p input, const std::string& output_unit);

miutil::KeyValue_v mergeSetupAndQuickMenuOptions(const miutil::KeyValue_v& setup, const miutil::KeyValue_v& cmd);

#endif // difieldutil_h
