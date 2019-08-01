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

#include "diana_config.h"

#include "diObsAscii.h"

#include "diLabelPlotCommand.h"
#include "diUtilities.h"
#include "util/string_util.h"

#include <mi_fieldcalc/MetConstants.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsAscii"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

static const size_t BAD = 0xFFFFFF;

ObsAscii::ObsAscii(const string& filename, const string& headerfile,
    const vector<string>& headerinfo)
{
  METLIBS_LOG_SCOPE();
  knots = false;
  readHeaderInfo(filename, headerfile, headerinfo);
  decodeHeader();
}

const std::vector<ObsData>& ObsAscii::getObsData(const miTime &filetime, const miutil::miTime& time, int td)
{
  METLIBS_LOG_SCOPE();
  plotTime = time;
  timeDiff = td;
  fileTime = filetime;

  readDecodeData();

  return vObsData;
}

//####################################################################

void ObsAscii::readHeaderInfo(const string& filename, const string& headerfile,
    const vector<string>& headerinfo)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename) << LOGVAL(headerfile) << LOGVAL(headerinfo.size()));

  m_needDataRead = true;
  if (not headerinfo.empty()) {
    lines = headerinfo;
  } else if (not headerfile.empty()) {
    readData(headerfile);
  } else {
    m_needDataRead = false;
    readData(filename);
  }
  if (m_needDataRead)
    m_filename = filename;
}

void ObsAscii::readDecodeData()
{
  METLIBS_LOG_SCOPE();
  if (m_needDataRead)
    readData(m_filename);
  decodeData();
}

void ObsAscii::readData(const std::string& filename)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_INFO("Start reading '" << filename << "'");
  if (diutil::getFromAny(filename, lines))
    METLIBS_LOG_INFO("Done reading '" << filename << "'");
  else
    METLIBS_LOG_WARN("Error reading '" << filename << "'");
}

static void erase_comment(std::string& line, const std::string& comment = "#")
{
  const size_t cpos = line.find(comment);
  if (cpos != std::string::npos)
    line.erase(cpos);
}

bool ObsAscii::bracketContents(std::vector<std::string>& in_out)
{
  METLIBS_LOG_SCOPE();
  if (in_out.empty())
    return true;

  std::string joined = in_out[0];
  for (size_t i=1; i<in_out.size(); ++i)
    joined += " " + in_out[i];
  METLIBS_LOG_DEBUG(LOGVAL(joined));

  in_out.clear();

  size_t start = 0;
  while (start < joined.length()) {
    size_t p_open = joined.find('[', start);
    if (p_open == std::string::npos)
      break;
    size_t p_close = joined.find(']', p_open + 1);
    if (p_close == std::string::npos)
      return false;
    in_out.push_back(joined.substr(p_open+1, p_close - p_open - 1));
    METLIBS_LOG_DEBUG(LOGVAL(in_out.back()));
    start = p_close + 1;
  }
  return true;
}

void ObsAscii::parseHeaderBrackets(const std::string& str)
{
  METLIBS_LOG_SCOPE(LOGVAL(str));
  vector<string> pstr = miutil::split_protected(str, '"', '"');
  if (pstr.size() <= 1)
    return;

  if (pstr[0] == "COLUMNS") {
    if (not separator.empty())
      pstr= miutil::split(str, separator);
    for (size_t j=1; j<pstr.size(); j++) {
      diutil::remove_quote(pstr[j]);
      const vector<std::string> vs = miutil::split(pstr[j], ":");
      if (vs.size()>1) {
        m_columnName.push_back(vs[0]);
        m_columnType.push_back(miutil::to_lower(vs[1]));
        if (vs.size()>2) {
          m_columnTooltip.push_back(vs[2]);
        } else {
          m_columnTooltip.push_back("");
        }
      }
    }
  } else if (pstr[0] == "UNDEFINED") {
    const std::vector<std::string> vs= miutil::split(pstr[1], ",");
    asciiColumnUndefined.insert(vs.begin(), vs.end());
  } else if (pstr[0] == "SKIP_DATA_LINES" && pstr.size()>1) {
    asciiSkipDataLines = miutil::to_int(pstr[1]);
  } else if (pstr[0] == "LABEL") {
    labels.push_back(LabelPlotCommand::fromString(str.substr(5)));
  } else if (pstr[0] == "SEPARATOR") {
    separator = pstr[1];
  }
}

void ObsAscii::decodeHeader()
{
  METLIBS_LOG_SCOPE();

  vector<string> vstr;
  for (std::string& line : lines) { // must be reference here, used later in decodeData
    erase_comment(line);
    miutil::trim(line);

    if (line == "[DATA]")
      // end of header, start data
      break;
    METLIBS_LOG_DEBUG(LOGVAL(line));
    vstr.push_back(line);
  }

  idx_x = BAD;
  idx_y = BAD;
  idx_ff = BAD;
  idx_dd = BAD;
  idx_image = BAD;
  idx_Name = BAD;

  idx_date = BAD;
  idx_year = BAD;
  idx_month = BAD;
  idx_day = BAD;

  idx_time = BAD;
  idx_hour = BAD;
  idx_min = BAD;
  idx_sec = BAD;

  asciiSkipDataLines= 0;

  m_columnType.clear();
  m_columnName.clear();
  m_columnTooltip.clear();
  asciiColumnUndefined.clear();

  // parse header

  if (not bracketContents(vstr)) {
    METLIBS_LOG_ERROR("bad header, cannot find closing ']'");
    return;
  }
  for (size_t i=0; i<vstr.size(); ++i)
    parseHeaderBrackets(vstr[i]);
  
  METLIBS_LOG_DEBUG("#columns: " << m_columnType.size() << LOGVAL(asciiSkipDataLines));
  
  knots=false;
  for (size_t i=0; i<m_columnType.size(); i++) {
    METLIBS_LOG_DEBUG("column " << i << " : " << m_columnName[i] << "  " << m_columnType[i]);

    const std::string& ct = m_columnType[i];
    const std::string ct_lower = miutil::to_lower(ct);
    const std::string cn_lower = miutil::to_lower(m_columnName[i]);

    if      (ct=="d")
      idx_date = i;
    else if (ct=="t")
      idx_time = i;
    else if (ct=="year")
      idx_year = i;
    else if (ct=="month")
      idx_month = i;
    else if (ct=="day")
      idx_day = i;
    else if (ct=="hour")
      idx_hour = i;
    else if (ct=="min")
      idx_min = i;
    else if (ct=="sec")
      idx_sec = i;
    else if (ct_lower=="lon")
      idx_x = i;
    else if (ct_lower=="lat")
      idx_y = i;
    else if (ct_lower=="dd")
      idx_dd = i;
    else if (ct_lower=="ff") //Wind speed in m/s
      idx_ff = i;
    else if (ct_lower=="ffk") //Wind speed in knots
      idx_ff = i;
    else if (ct_lower=="image")
      idx_image = i;
    else if (cn_lower=="lon" && ct=="r") //Obsolete
      idx_x = i;
    else if (cn_lower=="lat" && ct=="r") //Obsolete
      idx_y = i;
    else if (cn_lower=="dd" && ct=="r") //Obsolete
      idx_dd = i;
    else if (cn_lower=="ff" && ct=="r") //Obsolete
      idx_ff = i;
    else if (cn_lower=="ffk" && ct=="r") //Obsolete
      idx_ff = i;
    else if (cn_lower=="image" && ct=="s") //Obsolete
      idx_image = i;
    else if (cn_lower=="name" || ct=="id")
      idx_Name = i;

    if (ct_lower=="ffk" || cn_lower=="ffk")
      knots=true;
  }
}

bool ObsAscii::checkColumn(size_t idx, const diutil::string_v& cv) const
{
  return idx != BAD && idx < cv.size() && (asciiColumnUndefined.find(cv[idx])) == asciiColumnUndefined.end();
}

void ObsAscii::decodeData()
{
  METLIBS_LOG_TIME();

  const bool isoTime = (idx_time != BAD);
  const bool useTime = isoTime || (idx_hour != BAD);
  const bool date_ymd = (idx_year != BAD) && (idx_month != BAD) && (idx_day != BAD);
  const bool date_column = (idx_date != BAD);
  const bool allTime = useTime && (date_column || date_ymd);
  const bool isoDate = useTime && date_column;

  miDate filedate= fileTime.date();

  // skip header; this relies on decodeHeader having trimmed the header lines
  size_t ii = 0;
  while (ii < lines.size() && not miutil::contains(lines[ii], "[DATA]")) {
    METLIBS_LOG_DEBUG("skip '" << lines[ii] << "'");
    ++ii;
  }
  ii += 1; //skip [DATA] line too

  for (int i=0; ii < lines.size() and i < asciiSkipDataLines; ++i) {
    METLIBS_LOG_DEBUG("skip data start '" << lines[ii++] << "'");
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0]=='#')
      continue;
    ii += 1;
  }

  vObsData.reserve(lines.size());
  for (; ii < lines.size(); ++ii) {
    // METLIBS_LOG_DEBUG("read '" << lines[ii] << "'");
    std::string& line = lines[ii];
    miutil::trim(line);
    if (line.empty() or line[0] == '#')
      continue;

    vector<string> pstr;
    if (not separator.empty())
      pstr = miutil::split(line, separator, false);
    else
      pstr = miutil::split_protected(line, '"', '"');

    ObsData  obsData;

    const size_t tmp_nColumn = std::min(pstr.size(), m_columnType.size());
    for (size_t i=0; i<tmp_nColumn; i++) {
      diutil::remove_quote(pstr[i]);
      if (not asciiColumnUndefined.count(pstr[i]))
        obsData.put_string(m_columnName[i], pstr[i]);
    }

    if (checkColumn(idx_x, pstr))
      obsData.xpos = miutil::to_float(pstr[idx_x]);
    if (checkColumn(idx_y, pstr))
      obsData.ypos = miutil::to_float(pstr[idx_y]);
    if (checkColumn(idx_Name, pstr))
      obsData.id = pstr[idx_Name];
    if (checkColumn(idx_ff, pstr)) {
      const float value = miutil::to_float(pstr[idx_ff]);
      obsData.put_float("ff", knots ? miutil::knots2ms(value) : value);
    }
    if (checkColumn(idx_dd, pstr))
      obsData.put_float("dd", miutil::to_float(pstr[idx_dd]));
    if (checkColumn(idx_image, pstr))
      obsData.put_string("image", pstr[idx_image]);

    if (useTime) {
      miClock clock;
      if (isoTime) {
        if (!checkColumn(idx_time, pstr))
          continue;
        clock.setClock(pstr[idx_time]);
      } else {
        if (!checkColumn(idx_hour, pstr))
          continue;
        int hour = miutil::to_int(pstr[idx_hour]);
        int min = 0, sec = 0;
        // no problem if missing, assume min = sec = 0
        if (checkColumn(idx_min, pstr))
          min = miutil::to_int(pstr[idx_min]);
        if (checkColumn(idx_sec, pstr))
          sec = miutil::to_int(pstr[idx_sec]);
        clock.setClock(hour, min, sec);
      }

      miDate date;
      if (isoDate) {
        if (!checkColumn(idx_date, pstr))
          continue;
        date.setDate(pstr[idx_date]);
      } else if (allTime) {
        if (!(checkColumn(idx_year, pstr) && checkColumn(idx_month, pstr) && checkColumn(idx_day, pstr)))
          continue;
        const int year = miutil::to_int(pstr[idx_year]);
        const int month = miutil::to_int(pstr[idx_month]);
        const int day = miutil::to_int(pstr[idx_day]);
        date.setDate(year, month, day);
      } else  {
        date = filedate;
      }

      miTime obstime(date, clock);
      if (!allTime && !isoDate) {
        int mdiff= miTime::minDiff(obstime,fileTime);
        if      (mdiff<-12*60)
          obstime.addHour(24);
        else if (mdiff> 12*60)
          obstime.addHour(-24);
      }
      
      // METLIBS_LOG_DEBUG(LOGVAL(obstime) << LOGVAL(plotTime) << LOGVAL(timeDiff));
      if (timeDiff < 0 || abs(miTime::minDiff(obstime, plotTime))< timeDiff)
        obsData.obsTime = obstime;
      else
        continue;
    }
    vObsData.push_back(std::move(obsData));
  }
}
