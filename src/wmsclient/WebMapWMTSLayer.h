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

#ifndef WebMapWMTSLayer_h
#define WebMapWMTSLayer_h 1

#include "WebMapLayer.h"

#include "WebMapWMTSTileMatrixSet.h"

class WebMapWMTSLayer : public WebMapLayer
{
public:
  WebMapWMTSLayer(const std::string& identifier);

  ~WebMapWMTSLayer();

  void addMatrixSet(WebMapWMTSTileMatrixSet_cx mats) { mMatrixSets.push_back(mats); }

  void setURLTemplate(const std::string& t) { mURLTemplate = t; }

  void setTileFormat(const std::string& mime) { mTileFormat = mime; }

  void setStyles(const std::vector<std::string>& styles, const std::string& defaultStyle);

  size_t countCRS() const { return mMatrixSets.size(); }

  const WebMapWMTSTileMatrixSet_cx& matrixSet(size_t idx) const { return mMatrixSets.at(idx); }

  const std::string& urlTemplate() const { return mURLTemplate; }

  const std::string& tileFormat() const { return mTileFormat; }

  const std::string& defaultStyle() const { return mDefaultStyle; }

private:
  std::string mURLTemplate;
  std::vector<WebMapWMTSTileMatrixSet_cx> mMatrixSets;
  std::string mTileFormat;
  std::string mDefaultStyle;
};

typedef WebMapWMTSLayer* WebMapWMTSLayer_x;
typedef const WebMapWMTSLayer* WebMapWMTSLayer_cx;

#endif // WebMapWMTSLayer_h
