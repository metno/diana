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

#ifndef WebMapService_h
#define WebMapService_h 1

#include <QObject>

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

class Area;
class Rectangle;
class Projection;

class QImage;

class WebMapDimension {
public:
  explicit WebMapDimension(const std::string& identifier);

  void setTitle(const std::string& title)
    { mTitle = title; }

  void addValue(const std::string& value, bool isDefault);

  void clearValues();

  /*! dimension identifier, for machine identification of the layer */
  const std::string& identifier() const
    { return mIdentifier; }

  /*! human-readable title */
  const std::string& title(/*language*/) const
    { return mTitle; }

  /*! number of values available */
  size_t count() const
    { return mValues.size(); }

  /*! access a value */
  const std::string& value(size_t idx) const
    { return mValues.at(idx); }

  /*! access all values */
  const std::vector<std::string>& values() const
    { return mValues; }

  /*! access default values */
  const std::string& defaultValue() const;

  /*! true iff this is the time dimension */
  bool isTime() const;

  /*! true iff this is the elevation dimension */
  bool isElevation() const;

private:
  std::string mIdentifier;
  std::string mTitle;
  std::vector<std::string> mValues;
  size_t mDefaultIndex;
};

typedef std::vector<WebMapDimension> WebMapDimension_v;

// ========================================================================

class WebMapLayer {
public:
  WebMapLayer(const std::string& identifier);

  virtual ~WebMapLayer();

  void addDimension(const WebMapDimension& d)
    { mDimensions.push_back(d); }

  /*! Layer identifier */
  const std::string& identifier() const
    { return mIdentifier; }

  void setTitle(const std::string& title)
    { mTitle = title; }

  /*! human-readable title */
  const std::string& title(/*language*/) const
    { return mTitle; }

  void setAttribution(const std::string& a)
    { mAttribution = a; }

  /*! layer attribution */
  const std::string& attribution(/*language*/) const
    { return mAttribution; }

  /*! number of CRS available */
  virtual size_t countCRS() const = 0;

  /*! access CRS for this layer */
  virtual const std::string& CRS(size_t idx) const = 0;

  /*! number of extra dimensions */
  size_t countDimensions() const
    { return mDimensions.size(); }

  /*! access to an extra dimension */
  const WebMapDimension& dimension(size_t idx) const
    { return mDimensions.at(idx); }

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

// ========================================================================

class WebMapRequest : public QObject {
  Q_OBJECT;

public:
  virtual ~WebMapRequest();

  /*! set dimension value; ignores non-existent dimensions; dimensions
   *  without explicitly specified value are set to a default value */
  virtual void setDimensionValue(const std::string& dimIdentifier,
      const std::string& dimValue) = 0;

  /*! start fetching data */
  virtual void submit() = 0;

  /*! stop fetching data */
  virtual void abort() = 0;

  /*! number of tiles */
  virtual size_t countTiles() const = 0;

  /*! rectangle of one tile, in tileProjection coordinates */
  virtual const Rectangle& tileRect(size_t idx) const = 0;

  /*! image data of one tile; might have isNull() == true */
  virtual const QImage& tileImage(size_t idx) const = 0;

  /*! projection of all tiles */
  virtual const Projection& tileProjection() const = 0;

  /*! legend image; might have isNull() == true */
  virtual QImage legendImage() const;

Q_SIGNALS:
  /*! the request is complete, ready for rendering, or aborted */
  void completed();
};

typedef WebMapRequest* WebMapRequest_x;

// ========================================================================

class WebMapService : public QObject {
  Q_OBJECT;

protected:
  WebMapService(const std::string& identifier)
    : mIdentifier(identifier) { }

public:
  virtual ~WebMapService();

  const std::string& identifier() const
    { return mIdentifier; }

  /*! human-readable title. only valid after refreshComplete */
  const std::string& title(/*language*/) const
    { return mTitle; }

  /*! suitable refresh interval in seconds. negative for no refresh */
  virtual int refreshInterval() const;

  /*! number of layers available */
  size_t countLayers() const
    { return mLayers.size(); }

  /*! access to a layer */
  WebMapLayer_cx layer(size_t idx) const
    { return mLayers.at(idx); }

  WebMapLayer_cx findLayerByIdentifier(const std::string& identifier);

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or */
  virtual WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale) = 0;

public Q_SLOTS:
  /* trigger reload of capabilities */
  virtual void refresh();

Q_SIGNALS:
  /*! the service capabilities will be refreshed */
  void refreshStarting();

  /*! the service capabilities have been refreshed */
  void refreshFinished();

protected:
  void destroyLayers();

protected:
  std::string mIdentifier;
  std::string mTitle;
  std::vector<WebMapLayer_cx> mLayers;
};

#endif
