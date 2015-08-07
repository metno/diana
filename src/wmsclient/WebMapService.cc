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

#include "WebMapService.h"

#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#include <QImage>

WebMapDimension::WebMapDimension(const std::string& identifier)
  : mIdentifier(identifier)
  , mDefaultIndex(0)
{
}

void WebMapDimension::addValue(const std::string& value, bool isDefault)
{
  if (isDefault)
    mDefaultIndex = mValues.size();
  mValues.push_back(value);
}

void WebMapDimension::clearValues()
{
  mValues.clear();
  mDefaultIndex = 0;
}

const std::string& WebMapDimension::defaultValue() const
{
  if (/*mDefaultIndex >= 0 &&*/ mDefaultIndex < mValues.size())
    return mValues[mDefaultIndex];
  static const std::string DEFAULT("default");
  return DEFAULT;
}

bool WebMapDimension::isTime() const
{
  return miutil::to_lower(identifier()) == "time";
}

bool WebMapDimension::isElevation() const
{
  return miutil::to_lower(identifier()) == "elevation";
}

// ========================================================================

WebMapLayer::WebMapLayer(const std::string& identifier)
  : mIdentifier(identifier)
{
}

WebMapLayer::~WebMapLayer()
{
}

int WebMapLayer::findDimensionByIdentifier(const std::string& dimId) const
{
  for (size_t i=0; i<countDimensions(); ++i) {
    if (dimension(i).identifier() == dimId)
      return i;
  }
  return -1;
}

// ========================================================================

WebMapRequest::~WebMapRequest()
{
}

QImage WebMapRequest::legendImage() const
{
  return QImage();
}

// ========================================================================

WebMapService::~WebMapService()
{
  destroyLayers();
}

void WebMapService::destroyLayers()
{
  diutil::delete_all_and_clear(mLayers);
}

int WebMapService::refreshInterval() const
{
  return -1;
}

void WebMapService::refresh()
{
  Q_EMIT refreshStarting();
  Q_EMIT refreshFinished();
}

WebMapLayer_cx WebMapService::findLayerByIdentifier(const std::string& identifier)
{
  for (size_t i=0; i<countLayers(); ++i) {
    if (layer(i)->identifier() == identifier)
      return layer(i);
  }
  return 0;
}
