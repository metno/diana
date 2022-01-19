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
#include "diKVListPlotCommand.h"

#include <vector>

class QNetworkAccessManager;
class WebMapLayer;
class WebMapPlot;
class WebMapService;

class WebMapManager : public Manager {
  Q_OBJECT

private:
  WebMapManager();

public:
  ~WebMapManager();

  bool parseSetup() Q_DECL_OVERRIDE;

  plottimes_t getTimes() const Q_DECL_OVERRIDE;

  void changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize) Q_DECL_OVERRIDE;

  void changeTime(const miutil::miTime&) Q_DECL_OVERRIDE;

  PlotStatus getStatus() override;

  void plot(DiGLPainter*, bool, bool) Q_DECL_OVERRIDE
    { }

  void plot(DiGLPainter* gl, PlotOrder zorder) Q_DECL_OVERRIDE;

  std::vector<PlotElement> getPlotElements() Q_DECL_OVERRIDE;

  QString plotElementTag() const Q_DECL_OVERRIDE;;

  bool enablePlotElement(const PlotElement &) Q_DECL_OVERRIDE;

  bool processInput(const PlotCommand_cpv&) Q_DECL_OVERRIDE;

  void sendMouseEvent(QMouseEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  void sendKeyboardEvent(QKeyEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  std::vector<std::string> getAnnotations() const Q_DECL_OVERRIDE;

  int getServiceCount() const
    { return webmapservices.size(); }
  WebMapService* getService(int i) const
    { return webmapservices.at(i); }

  int getPlotCount() const
    { return webmaps.size(); }
  WebMapPlot* getPlot(int i) const
    { return webmaps.at(i); }

Q_SIGNALS:
  //! sent when a webmap service has refreshed
  void serviceRefreshFinished();

private Q_SLOTS:
  void onPlotUpdate();

private:
  WebMapPlot* createPlot(KVListPlotCommand_cp qmstring);
  void clearMaps();
  void addMap(WebMapPlot* map);

private:
  QNetworkAccessManager* network;
  std::vector<WebMapService*> webmapservices;
  std::vector<WebMapPlot*> webmaps;

public:
  static WebMapManager* instance();
  static void destroy();

private:
  static WebMapManager* self;
};

#endif // WebMapManager_h
