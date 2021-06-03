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

#ifndef WebMapSlippyOSMLayer_h
#define WebMapSlippyOSMLayer_h 1

#include "WebMapLayer.h"

class WebMapSlippyOSMLayer : public WebMapLayer {
public:
  WebMapSlippyOSMLayer(const std::string& identifier)
    : WebMapLayer(identifier) { }

  void setURLTemplate(const std::string& t)
    { mURLTemplate = t; }

  void setZoomRange(int minZoom, int maxZoom)
    { mMinZoom = minZoom; mMaxZoom = maxZoom; }

  void setTileFormat(const std::string& mime)
    { mTileFormat = mime; }

  int minZoom() const
    { return mMinZoom; }

  int maxZoom() const
    { return mMaxZoom; }

  const std::string& tileFormat() const
    { return mTileFormat; }

  size_t countCRS() const
    { return 1; }

  const std::string& CRS(size_t) const;

  const std::string& urlTemplate() const
    { return mURLTemplate; }

private:
  std::string mURLTemplate;
  int mMinZoom;
  int mMaxZoom;
  std::string mTileFormat;
};

typedef WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_x;
typedef const WebMapSlippyOSMLayer* WebMapSlippyOSMLayer_cx;

#endif // WebMapSlippyOSMLayer_h
