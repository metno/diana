/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#define MILOGGER_CATEGORY "diana.DrawingManager"
#include <miLogger/miLogging.h>

#include <diDrawingManager.h>
#include <EditItems/edititembase.h>
#include <diPlotModule.h>
#include <diObjectManager.h>
#include <puTools/miDirtools.h>
#include <diAnnotationPlot.h>
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>
#include <puTools/miSetupParser.h>

#include <iomanip>
#include <set>
#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QFileDialog>

#define PLOTM PlotModule::instance()

//#define DEBUGPRINT
using namespace miutil;

DrawingManager *DrawingManager::self = 0;

DrawingManager::DrawingManager()
{
  self = this;
  editRect = PLOTM->getPlotSize();
  currentArea = PLOTM->getCurrentArea();
}

DrawingManager::~DrawingManager()
{
}

DrawingManager *DrawingManager::instance()
{
  if (!DrawingManager::self)
    DrawingManager::self = new DrawingManager();

  return DrawingManager::self;
}

bool DrawingManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  return true;
}

bool DrawingManager::processInput(const std::vector<std::string>& inp)
{
  vector<string>::const_iterator it;
  for (it = inp.begin(); it != inp.end(); ++it)
    cerr << "*** " << *it << endl;
  return true;
}

QList<QPointF> DrawingManager::getLatLonPoints(EditItemBase* item) const
{
  QList<QPointF> points = item->getPoints();
  return PhysToGeo(points);
}

QList<QPointF> DrawingManager::PhysToGeo(const QList<QPointF> &points) const
{
  int w, h;
  PLOTM->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  int n = points.size();

  QList<QPointF> latLonPoints;

  // Convert screen coordinates to geographic coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->PhysToGeo(points.at(i).x() - dx,
                     points.at(i).y() - dy,
                     x, y, currentArea, plotRect);
    latLonPoints.append(QPointF(x, y));
  }
  return latLonPoints;
}

void DrawingManager::setLatLonPoints(EditItemBase* item, const QList<QPointF> &latLonPoints)
{
  QList<QPointF> points = GeoToPhys(latLonPoints);
  item->setPoints(points);
}

QList<QPointF> DrawingManager::GeoToPhys(const QList<QPointF> &latLonPoints)
{
  int w, h;
  PLOTM->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  QList<QPointF> points;
  int n = latLonPoints.size();

  // Convert geographic coordinates to screen coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->GeoToPhys(latLonPoints.at(i).x(),
                     latLonPoints.at(i).y(),
                     x, y, currentArea, plotRect);
    points.append(QPointF(x + dx, y + dy));
  }

  return points;
}

/**
 * Returns a vector containing the times for which the manager has data.
*/
std::vector<miutil::miTime> DrawingManager::getTimes() const
{
  std::set<miutil::miTime> times;

  foreach (EditItemBase *item, items_) {
    QVariantMap p = item->propertiesRef();
    std::string time_str = p.value("time").toString().toStdString();
    if (!time_str.empty())
        times.insert(miutil::miTime(time_str));
  }

  std::vector<miutil::miTime> output;
  output.assign(times.begin(), times.end());

  // Sort the times.
  std::sort(output.begin(), output.end());

  return output;
}

/**
 * Prepares the manager for display of, and interaction with, items that
 * correspond to the given \a time.
*/
bool DrawingManager::prepare(const miutil::miTime &time)
{
  // Change the visibility of items in the editor.

  foreach (EditItemBase *item, items_) {
    QVariantMap p = item->propertiesRef();
    if (p.contains("time")) {
      QString time_str = p.value("time").toDateTime().toString("yyyy-MM-dd hh:mm:ss");
      p["visible"] = (time_str.isEmpty() | (time.isoTime() == time_str.toStdString()));
    } else
      p["visible"] = true;
    item->setProperties(p);
  }

  return true;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  // Update the edit rectangle so that objects are positioned consistently.
  plotRect = editRect = PLOTM->getPlotSize();
  currentArea = newArea;

  return true;
}

void DrawingManager::plot(bool under, bool over)
{
  if (!under)
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  plotRect = PLOTM->getPlotSize();
  int w, h;
  PLOTM->getPlotWindow(w, h);
  glTranslatef(editRect.x1, editRect.y1, 0.0);
  glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);

  foreach (EditItemBase *item, items_) {
      EditItemBase::DrawModes modes = EditItemBase::Normal;
      if (item->properties().value("visible", true).toBool()) {
          setLatLonPoints(item, item->getLatLonPoints());
          item->draw(modes, false);
      }
  }

  glPopMatrix();
}

void DrawingManager::initNewItem(EditItemBase *item)
{
  // Let other components know about any changes to item times.
  emit timesUpdated();
}
