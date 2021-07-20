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

#ifndef WebMapLayer_h
#define WebMapLayer_h 1

#include "WebMapDimension.h"

class WebMapLayer
{
public:
  WebMapLayer(const std::string& identifier);

  virtual ~WebMapLayer();

  void addDimension(const WebMapDimension& d) { mDimensions.push_back(d); }

  /*! Layer identifier */
  const std::string& identifier() const { return mIdentifier; }

  void setTitle(const std::string& title) { mTitle = title; }

  /*! human-readable title */
  const std::string& title(/*language*/) const { return mTitle; }

  void setAttribution(const std::string& a) { mAttribution = a; }

  /*! layer attribution */
  const std::string& attribution(/*language*/) const { return mAttribution; }

  /*! number of CRS available */
  virtual size_t countCRS() const = 0;

  /*! number of extra dimensions */
  size_t countDimensions() const { return mDimensions.size(); }

  /*! access to an extra dimension */
  const WebMapDimension& dimension(size_t idx) const { return mDimensions.at(idx); }

  int findDimensionByIdentifier(const std::string& dimId) const;

private:
  std::string mIdentifier;
  std::string mTitle;
  std::string mAttribution;

protected:
  WebMapDimension_v mDimensions;
};

typedef WebMapLayer* WebMapLayer_x;
typedef const WebMapLayer* WebMapLayer_cx;

#endif // WebMapLayer_h
