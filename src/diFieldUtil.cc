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

#include "diFieldUtil.h"

#include "diField/VcrossUtil.h"
#include "diField/diCommonFieldTypes.h"
#include "diField/diField.h"
#include "diField/diFlightLevel.h"
#include "diPlotOptions.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

bool splitDifferenceCommandString(const miutil::KeyValue_v& pin, miutil::KeyValue_v& fspec1, miutil::KeyValue_v& fspec2)
{
  const size_t npos = size_t(-1);
  const size_t p1 = find(pin, "(");
  if (p1 == npos)
    return false;

  const size_t p2 = find(pin, "-", p1 + 1);
  if (p2 == npos)
    return false;

  const size_t p3 = find(pin, ")", p2 + 1);
  if (p3 == npos)
    return false;

  const miutil::KeyValue_v common_start(pin.begin(), pin.begin() + p1);
  const miutil::KeyValue_v common_end(pin.begin() + p3 + 1, pin.end());

  fspec1 = common_start;
  fspec1.insert(fspec1.end(), pin.begin() + p1 + 1, pin.begin() + p2);
  diutil::insert_all(fspec1, common_end);

  fspec2 = common_start;
  fspec2.insert(fspec2.end(), pin.begin() + p2 + 1, pin.begin() + p3);
  diutil::insert_all(fspec2, common_end);

  return true;
}

void makeFieldText(Field_p fout, const std::string& plotName, bool flightlevel)
{
  std::string fieldtext = fout->modelName + " " + plotName;
  if (!fout->leveltext.empty()) {
    if (flightlevel)
      diutil::appendText(fieldtext, FlightLevel::getFlightLevel(fout->leveltext));
    else
      diutil::appendText(fieldtext, fout->leveltext);
  }
  diutil::appendText(fieldtext, fout->idnumtext);

  if (!fout->analysisTime.undef() && !fout->validFieldTime.undef()) {
    fout->forecastHour = miutil::miTime::hourDiff(fout->validFieldTime, fout->analysisTime);
  }

  std::string progtext;
  if (!fout->analysisTime.undef() && fout->forecastHour != -32767) {
    std::ostringstream ostr;
    ostr.width(2);
    ostr.fill('0');
    ostr << fout->analysisTime.hour() << " ";
    if (fout->forecastHour >= 0) {
      ostr << "+" << fout->forecastHour;
    } else {
      ostr << fout->forecastHour;
    }
    progtext = "(" + ostr.str() + ")";
  }

  std::string timetext;
  if (!fout->validFieldTime.undef()) {
    std::string sclock = fout->validFieldTime.isoClock();
    std::string shour = sclock.substr(0, 2);
    std::string smin = sclock.substr(3, 2);
    timetext = fout->validFieldTime.isoDate() + " " + shour;
    if (smin != "00")
      timetext += ":" + smin;
    timetext += " UTC";
  }

  fout->name = plotName;
  fout->text = fieldtext + " " + progtext;
  fout->fulltext = fieldtext + " " + progtext + " " + timetext;
  fout->fieldText = fieldtext;
  fout->progtext = progtext;
  fout->timetext = timetext;
}

std::string getBestReferenceTime(const std::set<std::string>& refTimes, int refOffset, int refHour)
{
  if (!refTimes.empty()) {

    // if refime is not a valid time, return string
    const std::string& last = *(refTimes.rbegin());
    if (!miutil::miTime::isValid(last)) {
      return last;
    }

    miutil::miTime refTime(last);

    if (refHour > -1) {
      miutil::miDate date = refTime.date();
      miutil::miClock clock(refHour, 0, 0);
      refTime = miutil::miTime(date, clock);
    }
    if (refOffset != 0) {
      refTime.addDay(refOffset);
    }

    std::set<std::string>::const_iterator p = refTimes.find(refTime.isoTime("T"));
    if (p != refTimes.end())
      return *p;

    // referencetime not found. If refHour is given and no refoffset, try yesterday
    if (refHour > -1 && refOffset == 0) {
      refTime.addDay(-1);
      p = refTimes.find(refTime.isoTime("T"));
      if (p != refTimes.end())
        return *p;
    }
  }
  return "";
}

void flightlevel2pressure(FieldRequest& frq)
{
  if (frq.zaxis == "flightlevel") {
    frq.zaxis = "pressure";
    frq.flightlevel = true;
    if (miutil::contains(frq.plevel, "FL")) {
      frq.plevel = FlightLevel::getPressureLevel(frq.plevel);
    }
  }
}

Field_p convertUnit(Field_p input, const std::string& output_unit)
{
  if (!input)
    return nullptr;

  if (input->unit.empty() || output_unit.empty() || vcross::util::unitsIdentical(input->unit, output_unit))
    return input;

  if (!vcross::util::unitsConvertible(input->unit, output_unit))
    return nullptr;

  Field_p result = std::make_shared<Field>(*input);
  result->unit = output_unit;
  if (input->defined() == miutil::NONE_DEFINED ||
      vcross::util::unitConversion(input->unit, result->unit, result->area.gridSize(), miutil::UNDEF, input->data, result->data))
    return result;

  return nullptr;
}

namespace {
bool contains(const miutil::KeyValue_v& kvs, const std::string& key, size_t start = 0)
{
  return miutil::find(kvs, key, start) != size_t(-1);
}
} // namespace

miutil::KeyValue_v mergeSetupAndQuickMenuOptions(const miutil::KeyValue_v& setup, const miutil::KeyValue_v& cmd)
{
  // merge with options from setup/logfile for this fieldname
  miutil::KeyValue_v kv;
  kv << setup;

  const auto& co = cmd;
  while (contains(co, PlotOptions::key_linevalues) || contains(co, PlotOptions::key_loglinevalues)) {
    const size_t idx_lineinterval = miutil::find(kv, PlotOptions::key_lineinterval);
    if (idx_lineinterval != size_t(-1)) {
      kv.erase(kv.begin() + idx_lineinterval);
    } else {
      break;
    }
  }
  kv << cmd;
  return kv;
}
