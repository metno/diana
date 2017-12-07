/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2017 met.no

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
//#define DEBUGREDRAW

#include "diana_config.h"

#include "diController.h"
#include "diGLPainter.h"
#include "diMainPaintable.h"
#include "qtMainUiEventHandler.h"

#include <QKeyEvent>
#include <QMouseEvent>

#define MILOGGER_CATEGORY "diana.MainPaintable"
#include <miLogger/miLogging.h>

MainUiEventHandler::MainUiEventHandler(MainPaintable* g)
    : p(g)
    , scrollwheelZoom(false)
{
}

void MainUiEventHandler::setFlagsFromEventResult(const EventResult& res)
{
  changeCursor(res.newcursor);
  p->enable_background_buffer = res.enable_background_buffer;
  if (res.repaint && res.update_background_buffer)
    p->update_background_buffer = true;
}

// Sends all QMouseEvents off to controller. Return values are checked,
// and any GUI-action taken
bool MainUiEventHandler::handleMouseEvents(QMouseEvent* me)
{
  EventResult res;

  // Duplicate the event, but transform the position of the cursor into the
  // plot's coordinate system.
  QMouseEvent me2(me->type(), QPoint(me->x(), p->size().height() - me->y()), me->globalPos(), me->button(), me->buttons(), me->modifiers());
  // send event to controller
  p->controller()->sendMouseEvent(&me2, res);
  setFlagsFromEventResult(res);

  // check if any specific GUI-action requested
  if (res.action != no_action) {
    switch (res.action) {
    case browsing:
      emit mouseMovePos(&me2, false);
      break;
    case quick_browsing:
      emit mouseMovePos(&me2, true);
      break;
    case pointclick:
      emit mouseGridPos(&me2);
      break;
    case rightclick:
      emit mouseRightPos(&me2);
      break;
    case objects_changed:
      emit objectsChanged();
      break;
    case fields_changed:
      emit fieldsChanged();
      break;
    case doubleclick:
      emit mouseDoubleClick(&me2);
      break;
    case keypressed:
      break;
    case no_action:
      break;
    }
  }

  return res.repaint;
}

// Sends all QKeyEvents off to controller. Return values are checked,
// and any GUI-action taken
bool MainUiEventHandler::handleKeyEvents(QKeyEvent* ke)
{
  EventResult res;
  p->controller()->sendKeyboardEvent(ke, res);
  setFlagsFromEventResult(res);

  // check if any specific GUI-action requested
  if (res.action != no_action) {
    switch (res.action) {
    case objects_changed:
      emit objectsChanged();
      break;
    case keypressed:
      emit keyPress(ke);
      break;
    default:
      break;
    }
  }

  return res.repaint;
}

// ---------------------- event callbacks -----------------

bool MainUiEventHandler::handleWheelEvents(QWheelEvent* we)
{
  if (useScrollwheelZoom() && we->orientation() == Qt::Vertical) {
    int numDegrees = we->delta() / 8;
    int numSteps = numDegrees / 15;
    if (numSteps > 0) {
      float x1, y1, x2, y2;
      float xmap, ymap;

      p->controller()->getPlotSize(x1, y1, x2, y2);
      /// (why -(y-height())? I have no idea ...)
      p->controller()->PhysToMap(we->x(), -(we->y() - p->size().height()), xmap, ymap);

      int wd = static_cast<int>((x2 - x1) / 3.);
      int hd = static_cast<int>((y2 - y1) / 3.);

      Rectangle r(xmap - wd, ymap - hd, xmap + wd, ymap + hd);
      p->controller()->zoomTo(r);
    } else {
      p->controller()->zoomOut();
    }
    p->update_background_buffer = true;
    return true;
  }
  return false;
}
