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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <diObsAscii.h>
#include <diObsPlot.h>
#include <diObsMetaData.h>
#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsAscii"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

ObsAscii::ObsAscii(const string& filename, const string& headerfile,
    const vector<string>& headerinfo)
{
  METLIBS_LOG_SCOPE();
  fileOK = false;
  knots = false;
  readHeaderInfo(filename, headerfile, headerinfo);
  decodeHeader();
}

void ObsAscii::yoyoPlot(const miTime &filetime, ObsPlot *oplot)
{
  METLIBS_LOG_SCOPE();
  oplot->setLabels(labels);
  oplot->columnName = m_columnName;

  plotTime = oplot->getObsTime();
  timeDiff= oplot->getTimeDiff();
  fileTime = filetime;

  readDecodeData();

  oplot->addObsData(vObsData);
}

void ObsAscii::yoyoMetadata(ObsMetaData *metaData)
{
  METLIBS_LOG_SCOPE();
  readDecodeData();
  metaData->setMetaData(mObsData);
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
  if (not diutil::getFromAny(filename, lines))
    METLIBS_LOG_WARN("could not read '" << filename << "'");
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
      miutil::remove(pstr[j], '"');
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
    labels.push_back(str);
  } else if (pstr[0] == "SEPARATOR") {
    separator = pstr[1];
  }
}

void ObsAscii::decodeHeader()
{
  METLIBS_LOG_SCOPE();

  vector<string> vstr;
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i]; // must be reference here, used later in decodeData
    erase_comment(line);
    miutil::trim(line);

    if (line == "[DATA]")
      // end of header, start data
      break;
    METLIBS_LOG_DEBUG(LOGVAL(line));
    vstr.push_back(line);
  }

  asciiColumn.clear();
  asciiSkipDataLines= 0;

  m_columnType.clear();
  m_columnName.clear();
  m_columnTooltip.clear();
  asciiColumnUndefined.clear();

  // parse header

  if (not bracketContents(vstr)) {
    METLIBS_LOG_ERROR("bad header, cannot find closing ']'");
    fileOK = false;
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
      asciiColumn["date"]= i;
    else if (ct=="t")
      asciiColumn["time"]= i;
    else if (ct=="year")
      asciiColumn["year"] = i;
    else if (ct=="month")
      asciiColumn["month"] = i;
    else if (ct=="day")
      asciiColumn["day"] = i;
    else if (ct=="hour")
      asciiColumn["hour"] = i;
    else if (ct=="min")
      asciiColumn["min"] = i;
    else if (ct=="sec")
      asciiColumn["sec"] = i;
    else if (ct_lower=="lon")
      asciiColumn["x"]= i;
    else if (ct_lower=="lat")
      asciiColumn["y"]= i;
    else if (ct_lower=="dd")
      asciiColumn["dd"]= i;
    else if (ct_lower=="ff") //Wind speed in m/s
      asciiColumn["ff"]= i;
    else if (ct_lower=="ffk") //Wind speed in knots
      asciiColumn["ff"]= i;
    else if (ct_lower=="image")
      asciiColumn["image"]= i;
    else if (cn_lower=="lon" && ct=="r") //Obsolete
      asciiColumn["x"]= i;
    else if (cn_lower=="lat" && ct=="r") //Obsolete
      asciiColumn["y"]= i;
    else if (cn_lower=="dd" && ct=="r") //Obsolete
      asciiColumn["dd"]= i;
    else if (cn_lower=="ff" && ct=="r") //Obsolete
      asciiColumn["ff"]= i;
    else if (cn_lower=="ffk" && ct=="r") //Obsolete
      asciiColumn["ff"]= i;
    else if (cn_lower=="image" && ct=="s") //Obsolete
      asciiColumn["image"]= i;
    else if (cn_lower=="name" || ct=="id")
      asciiColumn["Name"]= i;

    if (ct_lower=="ffk" || cn_lower=="ffk")
      knots=true;
  }

  fileOK= true;
  return;
}

ObsAscii::string_size_m::const_iterator ObsAscii::getColumn(const std::string& cn, const std::vector<std::string>& cv) const
{
  string_size_m::const_iterator it = asciiColumn.find(cn);
  if (it == asciiColumn.end() or it->second >= cv.size() or asciiColumnUndefined.count(cv[it->second]))
    return asciiColumn.end();
  else
    return it;
}

bool ObsAscii::getColumnValue(const std::string& cn, const diutil::string_v& cv, float& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = miutil::to_float(cv[it->second]);
  return true;
}

bool ObsAscii::getColumnValue(const std::string& cn, const diutil::string_v& cv, int& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = miutil::to_int(cv[it->second]);
  return true;
}

bool ObsAscii::getColumnValue(const std::string& cn, const diutil::string_v& cv, std::string& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = cv[it->second];
  return true;
}

void ObsAscii::decodeData()
{
  METLIBS_LOG_SCOPE();
  
  const bool isoTime = asciiColumn.count("time");
  const bool useTime = isoTime || asciiColumn.count("hour");
  const bool date_ymd = asciiColumn.count("year") && asciiColumn.count("month") && asciiColumn.count("day");
  const bool date_column = asciiColumn.count("date");
  const bool allTime = useTime && (date_column || date_ymd);
  const bool isoDate = useTime && date_column;

  miTime obstime;

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

  for (; ii < lines.size(); ++ii) {
    METLIBS_LOG_DEBUG("read '" << lines[ii] << "'");
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0]=='#')
      continue;

    vector<string> pstr;
    if (not separator.empty())
      pstr = miutil::split(lines[ii], separator, false);
    else
      pstr = miutil::split_protected(lines[ii], '"', '"');

    ObsData  obsData;

    const size_t tmp_nColumn = std::min(pstr.size(), m_columnType.size());
    for (size_t i=0; i<tmp_nColumn; i++) {
      if (not asciiColumnUndefined.count(pstr[i]))
        obsData.stringdata[m_columnName[i]] = pstr[i];
    }

    float value;
    std::string text;
    if (getColumnValue("x", pstr, value))
      obsData.xpos = value;
    if (getColumnValue("y", pstr, value))
      obsData.ypos = value;
    if (getColumnValue("Name", pstr, text))
      obsData.id = text;
    if (getColumnValue("ff", pstr, value))
      obsData.fdata["ff"] = knots ? diutil::knots2ms(value) : value;
    if (getColumnValue("dd", pstr, value))
      obsData.fdata["dd"] = value;
    if (getColumnValue("image", pstr, text))
      obsData.stringdata["image"] = text;

    if (useTime) {
      miClock clock;
      miDate date;
      if (isoTime) {
        if (getColumnValue("time", pstr, text)) {
          clock = miClock(text);
        } else {
          METLIBS_LOG_WARN("time column missing");
          continue;
        }
      } else {
        int hour=0, min=0, sec=0;
        if (getColumnValue("hour", pstr, hour)) {
          getColumnValue("min", pstr, min); // no problem if missing, assume min = sec = 00
          getColumnValue("sec", pstr, sec);
          clock = miClock(hour, min, sec);
        } else {
          METLIBS_LOG_WARN("hour column missing");
          continue;
        }
      }

      if (isoDate) {
        if (getColumnValue("date", pstr, text)) {
          date = miDate(text);
        } else {
          METLIBS_LOG_WARN("date column missing");
          continue;
        }
      } else if (allTime) {
        int year=0,month=0,day=0;
        if (getColumnValue("year", pstr, year) and getColumnValue("month", pstr, month) and getColumnValue("day", pstr, day)) {
          date = miDate(year, month, day);
        } else {
          METLIBS_LOG_WARN("year/month/day column missing");
          continue;
        }
      } else  {
        date = filedate;
      }
      obstime = miTime(date,clock);
      
      if (not allTime) {
        int mdiff= miTime::minDiff(obstime,fileTime);
        if      (mdiff<-12*60)
          obstime.addHour(24);
        else if (mdiff> 12*60)
          obstime.addHour(-24);
      }
      
      METLIBS_LOG_DEBUG(LOGVAL(obstime) << LOGVAL(plotTime) << LOGVAL(timeDiff));
      if (timeDiff < 0 || abs(miTime::minDiff(obstime, plotTime))< timeDiff)
        obsData.obsTime = obstime;
      else
        continue;
    }
    vObsData.push_back(obsData);
    mObsData[obsData.id] = obsData;
  }
}
