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
#include <QFileDialog>

//#define DEBUGPRINT
using namespace miutil;

DrawingManager *DrawingManager::self = 0;

DrawingManager::DrawingManager()
{
  self = this;
  editItemManager = new EditItemManager();

  connect(editItemManager, SIGNAL(itemAdded(EditItemBase *)), SLOT(initNewItem(EditItemBase *)));
  connect(editItemManager, SIGNAL(selectionChanged()), SLOT(updateActions()));

  enabled = false;
  plotm = PlotModule::instance();
  editRect = plotm->getPlotSize();
  currentArea = plotm->getCurrentArea();

  cutAction = new QAction(tr("Cut"), this);
  cutAction->setShortcut(tr("Ctrl+X"));
  copyAction = new QAction(tr("&Copy"), this);
  copyAction->setShortcut(QKeySequence::Copy);
  pasteAction = new QAction(tr("&Paste"), this);
  pasteAction->setShortcut(QKeySequence::Paste);
  editAction = new QAction(tr("P&roperties..."), this);
  editAction->setShortcut(tr("Ctrl+R"));
  loadAction = new QAction(tr("&Load..."), this);
  loadAction->setShortcut(tr("Ctrl+L"));
  undoAction = editItemManager->undoStack()->createUndoAction(this);
  redoAction = editItemManager->undoStack()->createRedoAction(this);

  connect(cutAction, SIGNAL(triggered()), SLOT(cutSelectedItems()));
  connect(copyAction, SIGNAL(triggered()), SLOT(copySelectedItems()));
  connect(editAction, SIGNAL(triggered()), SLOT(editItems()));
  connect(pasteAction, SIGNAL(triggered()), SLOT(pasteItems()));
  connect(loadAction, SIGNAL(triggered()), SLOT(loadItemsFromFile()));
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
  METLIBS_LOG_DEBUG("++ DrawingManager::parseSetup");
#endif

  miString section="DRAWING";
  vector<miString> vstr;

  return true;
}

void DrawingManager::sendMouseEvent(QMouseEvent* event, EventResult& res)
{
  res.savebackground= true;
  res.background= false;
  res.repaint= false;
  res.newcursor= edit_cursor;

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
      contextMenu.addAction(cutAction);
      contextMenu.addAction(copyAction);
      contextMenu.addAction(pasteAction);
      pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
      contextMenu.addSeparator();
      contextMenu.addAction(editAction);
      contextMenu.addSeparator();
      contextMenu.addAction(loadAction);
      editAction->setEnabled(editItemManager->getSelectedItems().size() > 0);

      // Simply execute the menu since all of the actions are connected to slots.
      if (!contextMenu.isEmpty())
        contextMenu.exec(me2.globalPos());

    } else {
      // Send the mouse press to the edit item manager.
      QSet<EditItemBase *> itemsToCopy; // items to be copied
      QSet<EditItemBase *> itemsToEdit; // items to be edited
      editItemManager->mousePress(&me2, &itemsToCopy, &itemsToEdit);

      if (itemsToCopy.size() > 0) {
        copyItems(itemsToCopy);
      } else if (itemsToEdit.size() > 0) {
        editItemManager->editItemProperties(itemsToEdit);
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
  event->setAccepted(res.repaint || (res.action != no_action));

  updateActions();
}

void DrawingManager::sendKeyboardEvent(QKeyEvent* event, EventResult& res)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("DrawingManager::sendKeyboardEvent");
#endif
  event->accept();
  res.savebackground= true;
  res.background= false;
  res.repaint= false;

  if (event->type() == QEvent::KeyPress) {
    if (cutAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      cutSelectedItems();
    else if (copyAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      copySelectedItems();
    else if (pasteAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      pasteItems();
    else if (editAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      editItems();
    else if (loadAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      loadItemsFromFile();
    else
      event->ignore();
  }

  res.repaint = true;
  res.background = true;
  if (event->isAccepted())
    return;

  editItemManager->keyPress(event);

  updateActions();
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

/**
 * Returns a vector containing the times for which the manager has data.
*/
std::vector<miutil::miTime> DrawingManager::getTimes() const
{
  std::set<miutil::miTime> times;
  QSet<EditItemBase *> items = editItemManager->getItems();

  foreach (EditItemBase *item, items) {
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
  QSet<EditItemBase *> items = editItemManager->getItems();

  foreach (EditItemBase *item, items) {
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
  if (!under)
    return;

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

void DrawingManager::cutSelectedItems()
{
  QSet<EditItemBase*> items = editItemManager->getSelectedItems();
  copyItems(items);
  foreach (EditItemBase* item, items)
    editItemManager->removeItem(item);

  updateActions();
}

void DrawingManager::copyItems(const QSet<EditItemBase *> &items)
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  QString text;

  text += QString("Number of items: %1\n").arg(items.size());
  QVariantList cbItems;

  foreach (EditItemBase *item, items) {
    cbItems.append(item->clipboardVarMap());
    text += QString("%1\n").arg(item->clipboardPlainText());
  }

  stream << cbItems;

  QMimeData *data = new QMimeData();
  data->setData("application/x-diana-object", bytes);
  data->setData("text/plain", text.toUtf8());

  QApplication::clipboard()->setMimeData(data);
  updateActions();
}

void DrawingManager::copySelectedItems()
{
  copyItems(editItemManager->getSelectedItems());
}

void DrawingManager::pasteItems()
{
  const QMimeData *data = QApplication::clipboard()->mimeData();
  if (data->hasFormat("application/x-diana-object")) {

    QByteArray bytes = data->data("application/x-diana-object");
    QDataStream stream(&bytes, QIODevice::ReadOnly);

    QVariantList cbItems;
    stream >> cbItems;

    foreach (QVariant cbItem, cbItems) {
      QString error;
      EditItemBase *item = EditItemBase::createItemFromVarMap(cbItem.toMap(), &error);
      if (item)
        editItemManager->addItem(item, false);
      else
        QMessageBox::warning(0, "Error", error);
    }
  }

  updateActions();
}

QHash<DrawingManager::Action, QAction*> DrawingManager::actions()
{
  QHash<Action, QAction*> a;
  a[Cut] = cutAction;
  a[Copy] = copyAction;
  a[Paste] = pasteAction;
  a[Edit] = editAction;
  a[Load] = loadAction;
  a[Undo] = undoAction;
  a[Redo] = redoAction;
  return a;
}

void DrawingManager::updateActions()
{
  cutAction->setEnabled(editItemManager->getSelectedItems().size() > 0);
  copyAction->setEnabled(editItemManager->getSelectedItems().size() > 0);
  pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
  editAction->setEnabled(editItemManager->getSelectedItems().size() > 0);

  // Let other components know about any changes to item times.
  emit timesUpdated();

  // Update the visibility of items based on the current plot time.
  miutil::miTime time;
  plotm->getPlotTime(time);
  prepare(time);
}

bool DrawingManager::isEnabled() const
{
  return enabled;
}

void DrawingManager::setEnabled(bool enable)
{
  enabled = enable;
}

void DrawingManager::editItems()
{
  editItemManager->editItemProperties(editItemManager->getSelectedItems());
}

void DrawingManager::loadItemsFromFile()
{
  // open file and read content
  const QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"), "/disk1/", tr("VAAC messages (*.kml)"));
  if (fileName.isNull())
      return; // operation cancelled
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(0, "Error", QString("failed to open file %1 for reading").arg(fileName));
    return;
  }
  QByteArray data = file.readAll();
  file.close();

  QString error;
  const QList<EditItem_WeatherArea::WeatherArea *> areas = EditItem_WeatherArea::WeatherArea::createFromKML(data, fileName, &error);
  if (!areas.isEmpty()) {
    foreach (EditItem_WeatherArea::WeatherArea *area, areas) {
      setLatLonPoints(area, area->getLatLonPoints());
      editItemManager->addItem(area, false);
    }
  } else {
    QMessageBox::warning(
          0, "Error", QString("failed to create areas from file %1: %2")
          .arg(fileName).arg(!error.isEmpty() ? error : "<error msg not set>"));
  }

  updateActions();
}

void DrawingManager::initNewItem(EditItemBase *item)
{
  // Use the current time for the new item.
  miutil::miTime time;
  plotm->getPlotTime(time);

  QVariantMap p = item->propertiesRef();
  if (!p.contains("time"))
    p["time"] = QDateTime::fromString(QString::fromStdString(time.isoTime()), "yyyy-MM-dd hh:mm:ss");

  item->setProperties(p);

  // Let other components know about any changes to item times.
  emit timesUpdated();
}
