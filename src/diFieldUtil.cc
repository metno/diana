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

#include "diFieldUtil.h"

#include "diField/diField.h"
#include "diField/diFlightLevel.h"
#include "diPlotOptions.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <sstream>

namespace {

const size_t npos = size_t(-1);

const std::string UNITS = "units";
const std::string UNIT = "unit";

// these had idNumber == 1 with CommandParser
// these are put into FieldRequest by FieldPlotManager::parseString
const std::set<std::string> cp__idnum1 = {"model", "plot",    "parameter", "vlevel",    "elevel",      "vcoord",    "ecoord",
                                          UNITS,   "reftime", "refhour",   "refoffset", "hour.offset", "hour.diff", "MINUS"};

} // namespace

void mergeFieldOptions(miutil::KeyValue_v& fieldopts, miutil::KeyValue_v defaultopts)
{
  if (defaultopts.empty())
    return;

  miutil::KeyValue_v new_fieldopts;
  for (const miutil::KeyValue& kv : fieldopts) {
    if (cp__idnum1.count(kv.key()))
      new_fieldopts.push_back(kv);
  }

  // skip "line.interval" from default options if "(log.)line.values" are given
  if (miutil::find(fieldopts, PlotOptions::key_linevalues) != npos || miutil::find(fieldopts, PlotOptions::key_loglinevalues) != npos) {
    size_t i_interval;
    while ((i_interval = miutil::find(defaultopts, PlotOptions::key_lineinterval)) != npos) {
      defaultopts.erase(defaultopts.begin() + i_interval);
    }
  }

  // loop through current options, replace the value if the new string has same option with different value
  for (miutil::KeyValue& opt : defaultopts) {
    const size_t i = miutil::find(fieldopts, opt.key());
    if (i != npos) {
      // there is no option with variable no. of values, YET !!!!!
      if (fieldopts[i].value() != opt.value())
        opt = fieldopts[i];
    }
  }

  // loop through new options, add new option if it is not a part of current options
  for (miutil::KeyValue& fopt : fieldopts) {
    if (fopt.key() == "level" || fopt.key() == "idnum")
      continue;

    const size_t j = miutil::find(defaultopts, fopt.key());
    if (j == npos) {
      defaultopts.push_back(fopt);
    }
  }

  diutil::insert_all(new_fieldopts, defaultopts);
  std::swap(new_fieldopts, fieldopts);
}

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

void cleanupFieldOptions(miutil::KeyValue_v& vpopt)
{
  size_t ncus = miutil::rfind(vpopt, UNITS);
  size_t ncu = miutil::rfind(vpopt, UNIT);
  if (ncu != npos && ncus != npos) {
    const std::string value = vpopt[std::max(ncus, ncu)].value();
    while (ncus != npos) {
      vpopt.erase(vpopt.begin() + ncus);
      ncus = miutil::rfind(vpopt, UNITS, ncus - 1);
    }
    if (!vpopt.empty()) {
      ncu = miutil::rfind(vpopt, UNIT);
      while (ncu != npos) {
        vpopt.erase(vpopt.begin() + ncu);
        ncu = miutil::rfind(vpopt, UNIT, ncu - 1);
      }
    }
    vpopt.push_back(miutil::KeyValue(UNIT, value));
  }
}

void makeFieldText(Field* fout, const std::string& plotName, bool flightlevel)
{
  std::string fieldtext = fout->modelName + " " + plotName;
  if (!fout->leveltext.empty()) {
    diutil::appendText(fieldtext, " ");
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
