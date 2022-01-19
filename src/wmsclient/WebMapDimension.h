/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2021 met.no

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

#ifndef DIANA_WMSCLIENT_WMSDIMENSION_H
#define DIANA_WMSCLIENT_WMSDIMENSION_H

#include <string>
#include <vector>

class WebMapDimension
{
public:
  explicit WebMapDimension(const std::string& identifier);

  void setTitle(const std::string& title) { mTitle = title; }

  void setUnits(const std::string& units) { mUnits = units; }

  void addValue(const std::string& value, bool isDefault);

  void clearValues();

  /*! dimension identifier, for machine identification of the layer */
  const std::string& identifier() const { return mIdentifier; }

  /*! human-readable title */
  const std::string& title(/*language*/) const { return mTitle; }

  /*! units */
  const std::string& units() const { return mUnits; }

  /*! number of values available */
  size_t count() const { return mValues.size(); }

  /*! access a value */
  const std::string& value(size_t idx) const { return mValues.at(idx); }

  /*! access all values */
  const std::vector<std::string>& values() const { return mValues; }

  /*! access default values */
  const std::string& defaultValue() const;

  /*! true iff this is the time dimension */
  bool isTime() const;

  /*! true iff this is the elevation dimension */
  bool isElevation() const;

  /*! true iff this is a dimension with ISO8601 time values*/
  bool isTimeDimension() const;

private:
  std::string mIdentifier;
  std::string mTitle;
  std::string mUnits;
  std::vector<std::string> mValues;
  size_t mDefaultIndex;
};

typedef std::vector<WebMapDimension> WebMapDimension_v;

#endif // DIANA_WMSCLIENT_WMSDIMENSION_H
