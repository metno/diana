/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#include "WebMapUtilities.h"

#include "WebMapService.h"
#include "WebMapTile.h"

#include "diGlUtilities.h"
#include "diPoint.h"
#include "diUtilities.h"

#include "util/string_util.h"

#include "diField/diArea.h"
#include "diField/diGridReprojection.h"
#include "diField/diProjection.h"
#include "diField/diRectangle.h"

#include <puTools/miStringFunctions.h>

#include <QNetworkReply>
#include <QStringList>
#include <QVariant>
#include <QUrl>

#include <boost/shared_array.hpp>

#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.WebMapUtilities"
#include <miLogger/miLogging.h>

namespace diutil {

// extern
const double WMTS_M_PER_PIXEL = 0.00028;

const double WMTS_EARTH_RADIUS_M = 6378137.0;

// definition of international foot, https://en.wikipedia.org/wiki/Foot_%28unit%29#International_foot
static const double INTERNATIONAL_FOOT = 0.3048;

// definition os US survey foot, https://en.wikipedia.org/wiki/Foot_%28unit%29#Survey_foot
static const double US_SURVEY_FOOT =  1200.0 / 3937.0;

// ========================================================================

std::string textAfter(const std::string& haystack, const std::string& key)
{
  size_t pos = haystack.find(key);
  if (pos == std::string::npos)
    return std::string();
  pos += key.size();
  const size_t end = haystack.find(" ", pos);
  if (end != std::string::npos)
    return haystack.substr(pos, end-pos);
  else
    return haystack.substr(pos);
}

// ========================================================================

double metersPerUnit(const Projection& proj)
{
  METLIBS_LOG_SCOPE();
  if (proj.isGeographic()) {
    // fixed value for all geographic systems, OGC WMS 1.3.0 section 7.2.4.6.9
#if 1 // proj4 units are radians
    return WMTS_EARTH_RADIUS_M;
#else // proj4 units are degrees
    return WMTS_EARTH_RADIUS_M * 2 * M_PI / 360;
#endif
  }

  const std::string proj4 = proj.getProjDefinitionExpanded();
  METLIBS_LOG_DEBUG(LOGVAL(proj4));
  const std::string to_meter = textAfter(proj4, "+to_meter=");
  METLIBS_LOG_DEBUG(LOGVAL(to_meter));
  if (!to_meter.empty())
    return miutil::to_double(to_meter);

  const std::string units = textAfter(proj4, "+units=");
  METLIBS_LOG_DEBUG(LOGVAL(units));
  if (!units.empty()) {
    if (units == "m")
      return 1.0;
    if (units == "ft")
      return INTERNATIONAL_FOOT;
    if (units == "us-ft")
      return US_SURVEY_FOOT;
  }

  if (miutil::contains(proj4, "+proj=lonlat") || miutil::contains(proj4, "+proj=longlat") || miutil::contains(proj4, "+proj=latlon")
      || (miutil::contains(proj4, "+proj=ob_tran") &&
          (miutil::contains(proj4, "+o_proj=lonlat") || miutil::contains(proj4, "+o_proj=longlat") || miutil::contains(proj4, "+o_proj=latlon"))))
  {
    return WMTS_EARTH_RADIUS_M;
  }

  return 1.0; // assume units are meters
}

// ========================================================================

Projection projectionForCRS(const std::string& crs)
{
  static const char PREFIX_EPSG[] = "EPSG:";
  static const char PREFIX_URN_EPSG[] = "urn:ogc:def:crs:EPSG::";

  if (crs == "EPSG:900913")
    return Projection("+init=epsg:3857");
  else if (diutil::startswith(crs, PREFIX_EPSG))
    return Projection("+init=epsg:" + crs.substr(sizeof(PREFIX_EPSG)-1));
  else if (diutil::startswith(crs, PREFIX_URN_EPSG))
    return Projection("+init=epsg:" + crs.substr(sizeof(PREFIX_URN_EPSG)-1));
  else if (crs == "CRS:84")
    return Projection("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
  else
    return Projection();
}

// ========================================================================

namespace {

struct SelectTilesCB : public GridReprojectionCB {
  SelectTilesCB(tilexy_s& t, const Rectangle& tbbx,
                int nx_, int ny_, const diutil::PointD& xyt0_, const diutil::PointD& dxyt_)
    : tiles(t)
    , r_tiles(tbbx)
    , nx(nx_)
    , ny(ny_)
    , xyt0(xyt0_)
    , dxyt(dxyt_)
  { }

  tilexy_s& tiles;
  const Rectangle& r_tiles;
  int nx, ny;
  diutil::PointD xyt0, dxyt;

  void pixelLine(const diutil::PointI& s, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int n) override;
};

void SelectTilesCB::pixelLine(const PointI & /*s*/, const diutil::PointD& xyf0, const diutil::PointD& dxyf, int n)
{
  diutil::PointD fxy = xyf0;
  tilexy txy(-1, -1);
  for (int i=0; i<n; ++i, fxy += dxyf) {
    if (!r_tiles.isinside(fxy.x(), fxy.y()))
      continue;
    const diutil::PointD ixy = (fxy - xyt0)/ dxyt;
    const int ix = int(ixy.x()), iy = int(ixy.y());
    if (ix>=0 && ix<nx && iy>=0 && iy<ny) {
      if (txy.x != ix || txy.y != iy) {
        txy.x = ix;
        txy.y = iy;
        tiles.insert(txy);
      }
    }
  }
}

} // namespace

void select_pixel_tiles(tilexy_s& tiles,
                        int w, int h, // screen size
                        int nx, float x0, float dx,
                        int ny, float y0, float dy,
                        const Rectangle& r_tiles, const Projection& p_tiles,
                        const Rectangle& r_view, const Projection& p_view)
{
  SelectTilesCB cb(tiles, r_tiles, nx, ny, diutil::PointD(x0, y0), diutil::PointD(dx, dy));
  GridReprojection::instance()->reproject(diutil::Values2<int>(w, h), r_view, p_view, p_tiles, cb);
}

namespace detail {
bool parseDecimals(int& value, const std::string& text, size_t& idx,
    int count, bool count_is_minimum)
{
  size_t end = idx + count;
  if (end > text.size())
    return false;
  if (count_is_minimum)
    end = text.size();

  value = 0;
  const size_t begin = idx;
  for (; idx<end; ++idx) {
    if (!isdigit(text[idx]))
      break;
    value = 10*value + (text[idx] - '0');
  }

  const int n = (idx - begin);
  if (count_is_minimum)
    return n >= count;
  else
    return n == count;
}

bool parseDouble(double& value, const std::string& text, size_t& idx)
{
  const size_t end = text.size();

  value = 0;
  const size_t begin = idx;
  for (; idx<end; ++idx) {
    if (!isdigit(text[idx]))
      break;
    value = 10*value + (text[idx] - '0');
  }

  if (idx<end && text[idx] == '.') {
    idx += 1;
    float f = 0.1;
    for (; idx<end; ++idx, f /= 10) {
      if (!isdigit(text[idx]))
        break;
      value += f*(text[idx] - '0');
    }
  }
  return (idx > begin);
}
} // namespace detail

WmsTime:: WmsTime()
  : resolution(INVALID)
  , year(1970), month(1), day(1), hour(0), minute(0), second(0)
  , timezone_hours(0), timezone_minutes(0)
{ }

WmsTime parseWmsIso8601(const std::string& wmsIso8601)
{
  const size_t len = wmsIso8601.size();
  size_t i = 0;
  bool year_negative = false;
  if (len > 0 && wmsIso8601[i] == '-') {
    year_negative = true;
    i += 1;
  }

  WmsTime wmstime;
  if (!detail::parseDecimals(wmstime.year, wmsIso8601, i, 4, true))
    return WmsTime(); // INVALID
  if (year_negative) {
    // TODO is it allowed to have months with negative years?
    wmstime.year *= -1;
  }
  if (i == len) {
    wmstime.resolution = WmsTime::YEAR;
    return wmstime;
  }

  if (wmsIso8601[i] != '-')
    return WmsTime(); // INVALID
  i += 1;
  if (!detail::parseDecimals(wmstime.month, wmsIso8601, i, 2) || wmstime.month < 1 || wmstime.month > 12)
    return WmsTime(); // INVALID
  if (i == len) {
    wmstime.resolution = WmsTime::MONTH;
    return wmstime;
  }

  if (wmsIso8601[i] != '-')
    return WmsTime(); // INVALID
  i += 1;
  if (!detail::parseDecimals(wmstime.day, wmsIso8601, i, 2) || wmstime.day < 1 || wmstime.day > 31)
    return WmsTime(); // INVALID
  if (i == len) {
    wmstime.resolution = WmsTime::DAY;
    return wmstime;
  }

  if (wmsIso8601[i] != 'T')
    return WmsTime(); // INVALID
  i += 1;
  if (!detail::parseDecimals(wmstime.hour, wmsIso8601, i, 2) || wmstime.hour > 24)
    return WmsTime(); // INVALID
  wmstime.resolution = WmsTime::HOUR;

  if (i < len && wmsIso8601[i] == ':') {
    // parse minute
    i += 1;
    if (!detail::parseDecimals(wmstime.minute, wmsIso8601, i, 2) || wmstime.minute > 60)
      return WmsTime(); // INVALID
    wmstime.resolution = WmsTime::MINUTE;

    if (i < len && wmsIso8601[i] == ':') {
      // parse second
      i += 1;
      int second = 0;
      if (!detail::parseDecimals(second, wmsIso8601, i, 2) || wmstime.second > 61)
        return WmsTime(); // INVALID
      wmstime.second = second;
      wmstime.resolution = WmsTime::SECOND;

      if (i < len && wmsIso8601[i] == '.') {
        // parse subsecond
        i += 1;

        const size_t begin_subsecond = i;
        int subsecond = 0;
        if (!detail::parseDecimals(subsecond, wmsIso8601, i, 1, true))
          return WmsTime(); // INVALID
        wmstime.second += (subsecond / std::pow(10, i - begin_subsecond));
        wmstime.resolution = WmsTime::SUBSECOND;
      }
    }
  }

  // as we have hour, timezone must be specified
  if (i == len)
    return WmsTime(); // INVALID

  if (wmsIso8601[i] == 'Z') {
    // UTC, no action needed
    i += 1;
  } else if (wmsIso8601[i] == '+' || wmsIso8601[i] == '-') {
    // timezone offset
    const bool zone_negative = (wmsIso8601[i] == '-');
    i += 1;

    // timezone hours
    if (!detail::parseDecimals(wmstime.timezone_hours, wmsIso8601, i, 2) || wmstime.timezone_hours > 24)
      return WmsTime(); // INVALID

    if (i < len && wmsIso8601[i] == ':') {
      // timezone minutes
      i += 1;
      if (!detail::parseDecimals(wmstime.timezone_minutes, wmsIso8601, i, 2) || wmstime.timezone_minutes > 60)
        return WmsTime(); // INVALID
    }

    if (zone_negative) {
      wmstime.timezone_hours   *= -1;
      wmstime.timezone_minutes *= -1;
    }

    // do not convert to UTC, it would need to know leap years etc
  }
  if (i != len)
    return WmsTime(); // INVALID

  return wmstime;
}

miutil::miTime to_miTime(const WmsTime& wmstime)
{
  // this cannot work perfectly, as miTime does not allow specifying resolutions "only year"
  if (wmstime.resolution == WmsTime::INVALID)
    return miutil::miTime();

  int month = 1, day = 1, hour = 0, minute = 0, second = 0;
  if (wmstime.resolution >= WmsTime::MONTH)
    month = wmstime.month;
  if (wmstime.resolution >= WmsTime::DAY)
    day = wmstime.day;
  if (wmstime.resolution >= WmsTime::HOUR)
    hour = wmstime.hour;
  if (wmstime.resolution >= WmsTime::MINUTE)
    minute = wmstime.minute;
  if (wmstime.resolution >= WmsTime::SECOND)
    second = (int)wmstime.second;

  miutil::miTime t(wmstime.year, month, day, hour, minute, second);
  t.addHour(-wmstime.timezone_hours);
  t.addMin(-wmstime.timezone_minutes);
  return t;
}

std::string to_wmsIso8601(const miutil::miTime& t, WmsTime::Resolution resolution)
{
  if (resolution == WmsTime::INVALID)
    return "default";
  std::ostringstream o;
  o << std::setfill('0') << std::setw(4) << t.year();
  if (resolution > WmsTime::YEAR) {
    o << '-';
    o << std::setfill('0') << std::setw(2) << t.month();
    if (resolution > WmsTime::MONTH) {
      o << '-';
      o << std::setfill('0') << std::setw(2) << t.day();
      if (resolution > WmsTime::DAY) {
        o << 'T';
        o << std::setfill('0') << std::setw(2) << t.hour();
        if (resolution > WmsTime::HOUR) {
          o << ':';
          o << std::setfill('0') << std::setw(2) << t.min();
          if (resolution > WmsTime::MINUTE) {
            o << ':';
            o << std::setfill('0') << std::setw(2) << t.sec();
            // FIXME SUBSECOND resolution
          }
        }
        o << 'Z'; // always UTC
      }
    }
  }
  return o.str();
}

// ========================================================================

WmsInterval::WmsInterval()
  : resolution(WmsTime::INVALID), year(0), month(0), day(0), hour(0), minute(0), second(0) { }

WmsInterval parseWmsIso8601Interval(const std::string& text)
{
  const size_t len = text.size();
  if (len == 0 || text[0] != 'P')
    return WmsInterval(); // INVALID

  size_t i = 1;
  bool inYMD = true;

  WmsInterval wmsinterval;
  for (; i < len; i += 1) {
    if (inYMD && text[i] == 'T') {
      inYMD = false;
      continue;
    }
    double value = 0;
    if (!detail::parseDouble(value, text, i))
        return WmsInterval(); // INVALID
    if (i == len)
      return WmsInterval(); // INVALID
    if (inYMD) {
      if (text[i] == 'Y') {
        if (wmsinterval.resolution >= WmsTime::YEAR)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::YEAR;
        wmsinterval.year = value;
      } else if (text[i] == 'M') {
        if (wmsinterval.resolution >= WmsTime::MONTH)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::MONTH;
        wmsinterval.month = value;
      } else if (text[i] == 'D') {
        if (wmsinterval.resolution >= WmsTime::DAY)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::DAY;
        wmsinterval.day = value;
      } else {
        return WmsInterval(); // INVALID
      }
    } else {
      if (text[i] == 'H') {
        if (wmsinterval.resolution >= WmsTime::HOUR)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::HOUR;
        wmsinterval.hour = value;
      } else if (text[i] == 'M') {
        if (wmsinterval.resolution >= WmsTime::MINUTE)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::MINUTE;
        wmsinterval.minute = value;
      } else if (text[i] == 'S') {
        if (wmsinterval.resolution >= WmsTime::SECOND)
          return WmsInterval(); // INVALID
        wmsinterval.resolution = WmsTime::SECOND;
        wmsinterval.second = value;
      } else {
        return WmsInterval(); // INVALID
      }
    }
  }
  return wmsinterval;
}

// ========================================================================

QStringList expandWmsTimes(const QString& timesSpec)
{
  const QStringList mmr = timesSpec.split("/");
  if (mmr.size() < 2)
    return QStringList(timesSpec);

  WmsTime wstart = parseWmsIso8601(qs(mmr.at(0)));
  WmsTime wend   = parseWmsIso8601(qs(mmr.at(1)));
  if (wstart.resolution == WmsTime::INVALID || wend.resolution == WmsTime::INVALID)
    return QStringList(mmr.at(0)) + QStringList(mmr.at(1));

  WmsInterval wint;
  if (mmr.size() >= 3) {
    wint = parseWmsIso8601Interval(qs(mmr.at(2)));
    if (wint.resolution < wstart.resolution)
      wint.resolution = wstart.resolution;
  } else {
    wint.resolution = std::max(wstart.resolution, wend.resolution);
    switch (wint.resolution) {
    case WmsTime::YEAR:
      wint.year = 1; break;
    case WmsTime::MONTH:
      wint.month = 1; break;
    case WmsTime::DAY:
      wint.day = 1; break;
    case WmsTime::HOUR:
      wint.hour = 1; break;
    case WmsTime::MINUTE:
      wint.minute = 1; break;
    case WmsTime::SECOND:
    case WmsTime::SUBSECOND:
      wint.second = 1; break;
    default:
      break;
    }
  }
  miutil::miTime start = to_miTime(wstart), end = to_miTime(wend);
  if (end == start)
    return QStringList(mmr.at(0)); // FIXME how to represent "repeated but not stored"?
  if (end < start)
    std::swap(start, end);

  QStringList times;
  miutil::miTime t = start;
  while (t < end) {
    times << sq(to_wmsIso8601(t, wint.resolution));

    t.addSec(wint.second);
    t.addMin(wint.minute + 60 * wint.hour);
    t.addDay(wint.day + 365.25 * (wint.month/12.0 + wint.year)); // FIXME what is the meaning of adding 3.1415 months?
  };
  times << sq(to_wmsIso8601(end, wint.resolution));
  return times;
}

QStringList expandWmsValues(const QString& valueSpec)
{
  const QStringList mmr = valueSpec.split("/");
  if (mmr.size() != 3) {
    return QStringList(valueSpec); // may be text containing '/'
  }

  bool startOK, endOK, resOK;
  double start = mmr.at(0).toDouble(&startOK);
  double end   = mmr.at(1).toDouble(&endOK);
  double resolution = std::abs(mmr.at(2).toDouble(&resOK));
  if (!(startOK && endOK && resOK))
    return QStringList(valueSpec); // may be text containing '/'

  if (start > end)
    std::swap(start, end);

  QStringList values;
  if (start == end) {
    // TODO implement start == end
    values << QString::number(start);
  } else if (resolution == 0 || (end - start) / resolution > 1000) {
      // too many values
    values << QString::number(start);
  } else {
    for (double v=start; v<=end; v += resolution)
      values << QString::number(v);
  }
  return values;
}

bool checkRedirect(WebMapService* service, WebMapImage* image)
{
  if (!image->reply())
    return false;

  const QVariant vRedirect = image->reply()->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (vRedirect.isNull())
    return false;

  QUrl redirect = vRedirect.toUrl();
  if (redirect.isRelative())
    redirect = image->reply()->url().resolved(redirect);
  image->submit(service->submitUrl(redirect));
  return true;
}


} // namespace diutil
