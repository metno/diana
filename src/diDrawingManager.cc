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
  editItemManager = new EditItemManager();
  editRect = plotm->getPlotSize();
  drawingModeEnabled = false;
  createNewItem = true;
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

void DrawingManager::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  res.savebackground= true;
  res.background= true;
  res.repaint= false;
  res.newcursor= edit_cursor;

  float newx, newy;
  plotm->PhysToMap(me->x(), me->y(), newx, newy);

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  plotm->getPlotWindow(w, h);
  plotRect = plotm->getPlotSize();

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates.
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  // Translate the mouse event by the current displacement of the viewport.
  QMouseEvent me2(me->type(), QPoint(me->x() + dx, me->y() + dy),
                  me->globalPos(), me->button(), me->buttons(), me->modifiers());

  if (me->type() == QEvent::MouseButtonPress) {
    if (createNewItem) {
      EditItem_WeatherArea::WeatherArea *area = new EditItem_WeatherArea::WeatherArea();
      editItemManager->addItem(area, true);
      createNewItem = false;
    }

    editItemManager->mousePress(&me2);
  } else if (me->type() == QEvent::MouseMove)
    editItemManager->mouseMove(&me2);
  else if (me->type() == QEvent::MouseButtonRelease)
    editItemManager->mouseRelease(&me2);
  else if (me->type() == QEvent::MouseButtonDblClick)
    editItemManager->mouseDoubleClick(&me2);

  res.repaint = editItemManager->needsRepaint();
  res.action = editItemManager->canUndo() ? objects_changed : no_action;
}

void DrawingManager::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("DrawingManager::sendKeyboardEvent");
#endif
  res.savebackground= true;
  res.background= false;
  res.repaint= false;

  editItemManager->keyPress(ke);
  res.repaint = true;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  // TODO: Perform transformations on the items.

  // Update the edit rectangle so that objects are positioned consistently.
  editRect = plotm->getPlotSize();
  plotRect = plotm->getPlotSize();

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
  editItemManager->draw();    //editobjects.plot();
  glPopMatrix();
}
