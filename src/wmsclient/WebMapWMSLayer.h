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

#ifndef WebMapWMSLayer_h
#define WebMapWMSLayer_h 1

#include "WebMapLayer.h"

#include <diField/diProjection.h>

struct WebMapWmsCrsBoundingBox
{
  std::string crs;
  Projection projection;
  double metersPerUnit;
  Rectangle boundingbox;
  WebMapWmsCrsBoundingBox(const std::string& c, const Projection& p, const Rectangle& bb);
};

typedef std::vector<WebMapWmsCrsBoundingBox> WebMapWmsCrsBoundingBox_v;
typedef const WebMapWmsCrsBoundingBox* WebMapWmsCrsBoundingBox_cx;

// ========================================================================

class WebMapWMSLayer : public WebMapLayer
{
public:
  WebMapWMSLayer(const std::string& identifier);

  void setDimensions(const WebMapDimension_v& dimensions) { mDimensions = dimensions; }

  void setDefaultStyle(const std::string& styleId, const std::string& legendUrl = std::string())
    { mDefaultStyle = styleId; mLegendUrl = legendUrl; }

  void setZoomRange(int minZoom, int maxZoom)
    { mMinZoom = minZoom; mMaxZoom = maxZoom; }

  void addCRS(const std::string& crs, const Projection& proj, const Rectangle& bbox) { mCRS.push_back(WebMapWmsCrsBoundingBox(crs, proj, bbox)); }

  size_t countCRS() const { return mCRS.size(); }

  const WebMapWmsCrsBoundingBox& crsBoundingBox(size_t idx) const { return mCRS.at(idx); }

  int minZoom() const { return mMinZoom; }

  int maxZoom() const { return mMaxZoom; }

  const std::string& defaultStyle() const { return mDefaultStyle; }

  const std::string& legendUrl() const { return mLegendUrl; }

private:
  WebMapWmsCrsBoundingBox_v mCRS;
  int mMinZoom;
  int mMaxZoom;
  std::string mDefaultStyle;
  std::string mLegendUrl;
};

typedef WebMapWMSLayer* WebMapWMSLayer_x;
typedef const WebMapWMSLayer* WebMapWMSLayer_cx;

#endif // WebMapWMSLayer_h
