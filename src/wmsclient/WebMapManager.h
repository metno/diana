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

#ifndef WebMapManager_h
#define WebMapManager_h 1

#include "diManager.h"

#include <vector>

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE
#endif

class QNetworkAccessManager;
class WebMapLayer;
class WebMapPlot;
class WebMapService;

class WebMapManager : public Manager {
  Q_OBJECT;

private:
  WebMapManager();

public:
  ~WebMapManager();

  bool parseSetup() Q_DECL_OVERRIDE;

  std::vector<miutil::miTime> getTimes() const Q_DECL_OVERRIDE
    { return std::vector<miutil::miTime>(); }

  bool changeProjection(const Area&) Q_DECL_OVERRIDE;

  bool prepare(const miutil::miTime&) Q_DECL_OVERRIDE;

  void plot(DiGLPainter*, bool, bool) Q_DECL_OVERRIDE
    { }

  void plot(DiGLPainter* gl, Plot::PlotOrder zorder) Q_DECL_OVERRIDE;

  std::vector<PlotElement> getPlotElements() Q_DECL_OVERRIDE;

  bool processInput(const std::vector<std::string>&) Q_DECL_OVERRIDE;

  void sendMouseEvent(QMouseEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  void sendKeyboardEvent(QKeyEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  std::vector<std::string> getAnnotations() const Q_DECL_OVERRIDE;

  int getServiceCount() const
    { return webmapservices.size(); }
  WebMapService* getService(int i) const
    { return webmapservices.at(i); }

  void addPlot(WebMapService* service, const WebMapLayer* layer);

  int getPlotCount() const
    { return webmaps.size(); }
  WebMapPlot* getPlot(int i) const
    { return webmaps.at(i); }

Q_SIGNALS:
  //! sent when a webmap has finished its request
  void webMapsReady();

  void webMapAdded(int index);
  void webMapRemoved(int index);
  void webMapsRemoved();

private:
  WebMapPlot* createPlot(const std::string& qmstring);
  void clearMaps();
  void addMap(WebMapPlot* map);

private:
  QNetworkAccessManager* network;
  std::vector<WebMapService*> webmapservices;
  std::vector<WebMapPlot*> webmaps;

public:
  static WebMapManager* instance();

private:
  static WebMapManager* self;
};

#endif // WebMapManager_h
