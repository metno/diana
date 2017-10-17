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

#include "WebMapManager.h"

#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "wmsclient/WebMapPlot.h"
#include "wmsclient/WebMapUtilities.h"
#include "wmsclient/WebMapSlippyOSM.h"
#include "wmsclient/WebMapWMS.h"
#include "wmsclient/WebMapWMTS.h"

#include <puTools/miStringFunctions.h>

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>

#define MILOGGER_CATEGORY "diana.WebMapManager"
#include <miLogger/miLogging.h>

namespace {
const char WEBMAP[] = "WEBMAP";
}

// static
WebMapManager* WebMapManager::self = 0;

// static
WebMapManager* WebMapManager::instance()
{
  if (!self)
    new WebMapManager();
  return self;
}

WebMapManager::WebMapManager()
  : network(0)
{
  self = this;
  setEnabled(true);
}

WebMapManager::~WebMapManager()
{
  clearMaps();
  diutil::delete_all_and_clear(webmapservices);

  network->deleteLater();
  self = 0;
}

void WebMapManager::clearMaps()
{
  diutil::delete_all_and_clear(webmaps);
}

bool WebMapManager::parseSetup()
{
  METLIBS_LOG_SCOPE();
  if (!network) {
    network = new QNetworkAccessManager();
    const std::string& cachedir = LocalSetupParser::basicValue("cachedir");
    if (!cachedir.empty()) {
      METLIBS_LOG_DEBUG(LOGVAL(cachedir));
      QNetworkDiskCache* cache = new QNetworkDiskCache(network);
      cache->setCacheDirectory(QString::fromStdString(cachedir));
      network->setCache(cache);
    }
  }

  clearMaps();
  diutil::delete_all_and_clear(webmapservices);

  const std::string SECTION = "WEBMAP_SERVICES";
  std::vector<std::string> lines;
  if (!miutil::SetupParser::getSection(SECTION, lines)) {
    METLIBS_LOG_INFO("section '" << SECTION << "' not found");
    return true;
  }

  // service.id=... service.type=wmts/slippy service.url=...
  for (size_t l=0; l<lines.size(); l++) {
    METLIBS_LOG_DEBUG(LOGVAL(lines[l]));
    std::string service_id, service_type, service_url, service_basicauth;
    std::vector< std::pair< std::string,std::string > > service_extra_query_items;

    const std::vector<std::string> kvpairs = miutil::split(lines[l]);
    for (size_t i=0; i<kvpairs.size(); i++) {
      const std::vector<std::string> kv = miutil::split(kvpairs[i], 1, "=");
      if (kv.size() != 2)
        continue;
      const std::string key = miutil::to_lower(kv[0]);
      const std::string& value = kv[1];
      if (key == "service.id")
        service_id = value;
      else if (key == "service.type")
        service_type = value;
      else if (key == "service.url")
        service_url = value;
      else if (key == "service.basicauth")
        service_basicauth = value;
      else if (key == "service.extra_query_item") {
        const std::vector<std::string> qkv = miutil::split(value, 1, "=");
        service_extra_query_items.push_back(std::make_pair(qkv[0], qkv.size() > 1 ? qkv[1] : ""));
      }
    }
    METLIBS_LOG_DEBUG(LOGVAL(service_id) << LOGVAL(service_type) << LOGVAL(service_url));

    if (!service_id.empty() && !service_type.empty() && !service_url.empty()) {
      WebMapService* s = 0;
      if (service_type == "wmts")
        s = new WebMapWMTS(service_id, QUrl(diutil::sq(service_url)), network);
      else if (service_type == "wms")
        s = new WebMapWMS(service_id, QUrl(diutil::sq(service_url)), network);
      else if (service_type == "slippy")
        s = new WebMapSlippyOSM(service_id, QUrl(diutil::sq(service_url)), network);
      if (s) {
        if (!service_basicauth.empty())
          s->setBasicAuth(service_basicauth);
        for (auto&& kv : service_extra_query_items)
          s->addExtraQueryItem(kv.first, kv.second);
        webmapservices.push_back(s);
      }
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(webmapservices.size()));
  return true;
}

WebMapPlot* WebMapManager::createPlot(KVListPlotCommand_cp qmstring)
{
  METLIBS_LOG_SCOPE(LOGVAL(qmstring->toString()));

  // webmap.service=<service.id> webmap.layer=... webmap.dim=name=value webmap.crs=<string>
  //   webmap.time_tolerance=<int[seconds]>  webmap.time_offset=<int[seconds]>
  //   style.alpha_scale=<float> style.alpha_offset=<float> style.grey=<bool>

  std::string wm_service, wm_layer; // mandatory
  for (const miutil::KeyValue& kv : qmstring->all()) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    if (key == "webmap.service")
      wm_service = value;
    else if (key == "webmap.layer")
      wm_layer = value;
  }
  if (wm_service.empty()) {
    METLIBS_LOG_DEBUG("no service");
    return 0;
  }

  WebMapService* service = 0;
  for (size_t i=0; i<webmapservices.size(); ++i) {
    if (wm_service == webmapservices[i]->identifier()) {
      service = webmapservices[i];
      break;
    }
  }
  if (!service) {
    METLIBS_LOG_WARN("unknown webmap service '" << wm_service << "'");
    return 0;
  }
  METLIBS_LOG_DEBUG(LOGVAL(wm_service) << LOGVAL(wm_layer));

  std::unique_ptr<WebMapPlot> plot(new WebMapPlot(service, wm_layer));

  std::map<std::string, std::string> wm_dims; // optional
  std::string wm_crs, wm_time_tolerance, wm_time_offset; // optional
  float style_alpha_offset = 0, style_alpha_scale = 1;
  bool style_grey = false;
  Plot::PlotOrder plotorder = Plot::LINES;
  for (const miutil::KeyValue& kv : qmstring->all()) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    if (key == "webmap.dim") {
      const std::vector<std::string> dnv = miutil::split(value, 1, "=");
      if (dnv.size() == 2)
        plot->setDimensionValue(dnv[0], dnv[1]);
    } else if (key == "webmap.crs") {
      plot->setCRS(value);
    } else if (key == "webmap.time_tolerance") {
      plot->setTimeTolerance(miutil::to_int(value));
    } else if (key == "webmap.time_offset") {
      plot->setTimeOffset(miutil::to_int(value));
    } else if (key == "webmap.zorder") {
      if (value == "background")
        plotorder = Plot::BACKGROUND;
      else if (value == "shade_background")
        plotorder = Plot::SHADE_BACKGROUND;
      else if (value == "shade")
        plotorder = Plot::SHADE;
      else if (value == "lines_background")
        plotorder = Plot::LINES_BACKGROUND;
      else
        plotorder = Plot::LINES;
    } else if (key == "style.alpha_scale") {
      style_alpha_scale = miutil::to_float(value);
    } else if (key == "style.alpha_offset") {
      style_alpha_offset = miutil::to_float(value);
    } else if (key == "style.grey") {
      const std::string lvalue = miutil::to_lower(value);
      style_grey = (lvalue == "true" || lvalue == "yes" || lvalue == "1");
    }
  }
  plot->setStyleAlpha(style_alpha_offset, style_alpha_scale);
  plot->setStyleGrey(style_grey);
  plot->setPlotOrder(plotorder);

  service->refresh();
  return plot.release();
}

bool WebMapManager::processInput(const PlotCommand_cpv& input)
{
  METLIBS_LOG_SCOPE(LOGVAL(input.size()));
  clearMaps();
  for (size_t i=0; i<input.size(); ++i) {
    if (KVListPlotCommand_cp c = std::dynamic_pointer_cast<const KVListPlotCommand>(input[i]))
      addMap(createPlot(c));
  }
  return true;
}

void WebMapManager::addPlot(WebMapService* service, const WebMapLayer* layer)
{
  METLIBS_LOG_SCOPE();
  if (!service || !layer)
    return;
  METLIBS_LOG_DEBUG(LOGVAL(service->identifier()) << LOGVAL(layer->identifier()));

  addMap(new WebMapPlot(service, layer->identifier()));
}

void WebMapManager::addMap(WebMapPlot* plot)
{
  if (!plot)
    return;
  connect(plot, SIGNAL(update()), SIGNAL(webMapsReady()));
  webmaps.push_back(plot);
}

void WebMapManager::plot(DiGLPainter* gl, Plot::PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();
  for (size_t i = 0; i < webmaps.size(); i++)
    webmaps[i]->plot(gl, zorder);
}

std::vector<PlotElement> WebMapManager::getPlotElements()
{
  std::vector<PlotElement> pel;
  pel.reserve(webmaps.size());
  for (size_t i = 0; i < webmaps.size(); i++) {
    pel.push_back(PlotElement(WEBMAP, miutil::from_number((int) i), WEBMAP, webmaps[i]->isEnabled()));
  }
  return pel;
}

QString WebMapManager::plotElementTag() const
{
  return WEBMAP;
}

bool WebMapManager::enablePlotElement(const PlotElement& pe)
{
  for (size_t i = 0; i < webmaps.size(); i++) {
    const PlotElement pew(WEBMAP, miutil::from_number((int) i), WEBMAP, webmaps[i]->isEnabled());
    if (pew.str == pe.str) {
      if (webmaps[i]->isEnabled() != pe.enabled) {
        webmaps[i]->setEnabled(pe.enabled);
        return true;
      }
    }
  }
  return false;
}

std::vector<std::string> WebMapManager::getAnnotations() const
{
  std::string anno;
  Colour ignored;
  std::vector<std::string> annotations;
  annotations.reserve(webmaps.size());
  for (size_t i = 0; i < webmaps.size(); i++) {
    webmaps[i]->getAnnotation(anno, ignored);
    annotations.push_back(anno);
  }
  return annotations;
}

bool WebMapManager::changeProjection(const Area&)
{
  METLIBS_LOG_SCOPE();
  for (size_t i = 0; i < webmaps.size(); i++)
    webmaps[i]->changeProjection();
  return true;
}

bool WebMapManager::prepare(const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();
  for (size_t i = 0; i < webmaps.size(); i++)
    webmaps[i]->setTimeValue(time);
  return true;
}
