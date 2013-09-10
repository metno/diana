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
#include <diEditItemManager.h>
#include <EditItems/edititembase.h>
#include <EditItems/weatherarea.h>
#include <EditItems/weatherfront.h>
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

//#define DEBUGPRINT
using namespace miutil;

DrawingManager::DrawingManager(PlotModule* pm, ObjectManager* om)
  : plotm(pm), objm(om)
{
  if (plotm==0 || objm==0){
    METLIBS_LOG_WARN("Catastrophic error: plotm or objm == 0");
  }
  editItemManager = new EditItemManager(this);
  editRect = plotm->getPlotSize();
  drawingModeEnabled = false;
  currentArea = plotm->getCurrentArea();
}

DrawingManager::~DrawingManager()
{
}

bool DrawingManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ DrawingManager::parseSetup");
#endif

  miString section="DRAWING";
  vector<miString> vstr;

  return true;
}

void DrawingManager::sendMouseEvent(QMouseEvent* event, EventResult& res)
{
  res.savebackground= true;
  res.background= true;
  res.repaint= false;
  res.newcursor= edit_cursor;

  float newx, newy;
  plotm->PhysToMap(event->x(), event->y(), newx, newy);

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  plotm->getPlotWindow(w, h);
  plotRect = plotm->getPlotSize();

  if (editItemManager->getItems().size() == 0)
    editRect = plotRect;

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates. This gives us displaced screen coordinates - these
  // are coordinates relative to the original edit rectangle.
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  // Translate the mouse event by the current displacement of the viewport.
  QMouseEvent me2(event->type(), QPoint(event->x() + dx, event->y() + dy),
                  event->globalPos(), event->button(), event->buttons(), event->modifiers());

  if (event->type() == QEvent::MouseButtonPress) {

    if ((me2.button() == Qt::RightButton)
        && (editItemManager->findHitItems(me2.pos()).size() == 0)
        && (!editItemManager->hasIncompleteItem())) {
      // Handle the event via a global context menu only; don't delegate to items via edit item manager.
      QMenu contextMenu;
      QAction copyAction("&Copy", 0);
      QAction pasteAction("&Paste", 0);
      if (editItemManager->getSelectedItems().size() > 0)
        contextMenu.addAction(&copyAction);
      if (QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"))
        contextMenu.addAction(&pasteAction);
      QAction *action = contextMenu.exec(me2.globalPos());
      if (action == &copyAction)
        copySelectedItems();
      else if (action == &pasteAction)
        pasteItems();
    } else {
      // Send the mouse press to the edit item manager.
      QSet<EditItemBase *> itemsToCopy; // items to be copied
      editItemManager->mousePress(&me2, &itemsToCopy);

      if (itemsToCopy.size() > 0) {
        copyItems(itemsToCopy);
      } else if (editItemManager->getSelectedItems().size() == 0 && !editItemManager->hasIncompleteItem()) {
        // Nothing was changed or interacted with, so create a new area and repeat the mouse click.
        EditItem_WeatherArea::WeatherArea *area = new EditItem_WeatherArea::WeatherArea();
        editItemManager->addItem(area, true);
        editItemManager->mousePress(&me2);
      }
    }

  } else if (event->type() == QEvent::MouseMove)
    editItemManager->mouseMove(&me2);

  else if (event->type() == QEvent::MouseButtonRelease)
    editItemManager->mouseRelease(&me2);

  else if (event->type() == QEvent::MouseButtonDblClick)
    editItemManager->mouseDoubleClick(&me2);

  res.repaint = editItemManager->needsRepaint();
  res.action = editItemManager->canUndo() ? objects_changed : no_action;
}

void DrawingManager::sendKeyboardEvent(QKeyEvent* event, EventResult& res)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("DrawingManager::sendKeyboardEvent");
#endif
  res.savebackground= true;
  res.background= false;
  res.repaint= false;

  editItemManager->keyPress(event);
  res.repaint = true;

  if (event->type() == QEvent::KeyPress) {
    if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier)
      copySelectedItems();
    else if (event->key() == Qt::Key_V && event->modifiers() & Qt::ControlModifier)
      pasteItems();
  }
}

QList<QPointF> DrawingManager::getLatLonPoints(EditItemBase* item) const
{
  QList<QPoint> points = item->getPoints();
  return PhysToGeo(points);
}

QList<QPointF> DrawingManager::PhysToGeo(const QList<QPoint> &points) const
{
  int w, h;
  plotm->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  int n = points.size();

  QList<QPointF> latLonPoints;

  // Convert screen coordinates to geographic coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    plotm->PhysToGeo(points.at(i).x() - dx,
                     points.at(i).y() - dy,
                     x, y, currentArea, plotRect);
    latLonPoints.append(QPointF(x, y));
  }
  return latLonPoints;
}

void DrawingManager::setLatLonPoints(EditItemBase* item, const QList<QPointF> &latLonPoints)
{
  QList<QPoint> points = GeoToPhys(latLonPoints);
  item->setPoints(points);
}

QList<QPoint> DrawingManager::GeoToPhys(const QList<QPointF> &latLonPoints)
{
  int w, h;
  plotm->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  QList<QPoint> points;
  int n = latLonPoints.size();

  // Convert geographic coordinates to screen coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    plotm->GeoToPhys(latLonPoints.at(i).x(),
                     latLonPoints.at(i).y(),
                     x, y, currentArea, plotRect);
    points.append(QPoint(x + dx, y + dy));
  }

  return points;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  int w, h;
  plotm->getPlotWindow(w, h);

  Rectangle newPlotRect = plotm->getPlotSize();

  // Obtain the items from the editor.
  QSet<EditItemBase *> items = editItemManager->getItems();

  foreach (EditItemBase *item, items) {

    QList<QPointF> latLonPoints = getLatLonPoints(item);
    QList<QPoint> points;

    for (int i = 0; i < latLonPoints.size(); ++i) {
      float x, y;
      plotm->GeoToPhys(latLonPoints.at(i).x(), latLonPoints.at(i).y(), x, y, newArea, newPlotRect);
      points.append(QPoint(x, y));
    }

    item->setPoints(points);
  }

  // Update the edit rectangle so that objects are positioned consistently.
  plotRect = editRect = newPlotRect;
  currentArea = newArea;

  return true;
}

void DrawingManager::plot(bool under, bool over)
{
  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  plotRect = plotm->getPlotSize();
  int w, h;
  plotm->getPlotWindow(w, h);
  glTranslatef(editRect.x1, editRect.y1, 0.0);
  glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);
  editItemManager->draw();
  glPopMatrix();
}

void DrawingManager::copyItems(const QSet<EditItemBase *> &items) const
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  QString text;

  text += QString("Number of items: %1\n").arg(items.size());
  QVariantList weatherAreas;

  foreach (EditItemBase *item, items) {
    QVariantMap properties;
    QVariantList vpoints;
    const QList<QPointF> points = getLatLonPoints(item);

    foreach (QPointF p, points) {
      vpoints.append(p);
      text += QString("(%1, %2 bravo) ").arg(p.x()).arg(p.y());
    }
    properties.insert("points", vpoints);
    text += "\n";
    //properties.insert("color", item->color()); // example
    // .. and so on for other properties
    weatherAreas.append(properties);
  }

  QVariantMap mainVMap;
  mainVMap.insert("weatherAreas", weatherAreas);
  stream << mainVMap;

  QMimeData *data = new QMimeData();
  data->setData("application/x-diana-object", bytes);
  data->setData("text/plain", text.toUtf8());

  QApplication::clipboard()->setMimeData(data);
}

void DrawingManager::copySelectedItems() const
{
  copyItems(editItemManager->getSelectedItems());
}

void DrawingManager::pasteItems()
{
  const QMimeData *data = QApplication::clipboard()->mimeData();
  if (data->hasFormat("application/x-diana-object")) {

    QByteArray bytes = data->data("application/x-diana-object");
    QDataStream stream(&bytes, QIODevice::ReadOnly);

    QVariantMap mainVMap;
    stream >> mainVMap;

    const QVariantList weatherAreas = mainVMap.value("weatherAreas").toList();

    foreach (QVariant weatherArea, weatherAreas) {
      const QVariantMap properties = weatherArea.toMap();
      const QVariantList vpoints = properties.value("points").toList();
      QList<QPointF> points;
      foreach (QVariant vpoint, vpoints)
        points.append(vpoint.toPointF());
      //const QColor color = properties.value("color").value<QColor>(); // example
      // .. and so on for other properties
      EditItem_WeatherArea::WeatherArea *area = new EditItem_WeatherArea::WeatherArea();
      setLatLonPoints(area, points);
      editItemManager->addItem(area, false);
    }
  }
}
