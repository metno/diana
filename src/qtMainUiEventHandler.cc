/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

  return res.repaint;
}

// ---------------------- event callbacks -----------------

bool MainUiEventHandler::handleWheelEvents(QWheelEvent* we)
{
  if (useScrollwheelZoom() && we->orientation() == Qt::Vertical) {
    const int numDegrees = we->delta() / 8;
    const int numSteps = numDegrees / 15;
    if (numSteps != 0) {
      // convert wheel event position to "phys" coordinate system used by PlotModule
      const int we_phys_x = we->x(), we_phys_y = -(we->y() - p->size().height());
      const float frac_x = we_phys_x / float(p->size().width());
      const float frac_y = we_phys_y / float(p->size().height());

      p->controller()->zoomAt(numSteps, frac_x, frac_y);

      p->update_background_buffer = true;
      return true;
    }
  }
  return false;
}
