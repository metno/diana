/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtGLwidget.h"
#include "diController.h"
#include "diGLPainter.h"

#include <QMouseEvent>
#include <QKeyEvent>

#define MILOGGER_CATEGORY "diana.GLwidget"
#include <miLogger/miLogging.h>

GLwidget::GLwidget(Controller* c)
  : contr(c)
  , plotw(1)
  , ploth(1)
  , scrollwheelZoom(false)
{
}

GLwidget::~GLwidget()
{
}

void GLwidget::setCanvas(DiCanvas* canvas)
{
  DiPaintable::setCanvas(canvas);
  contr->setCanvas(canvas);
  requestBackgroundBufferUpdate();
}

void GLwidget::paintUnderlay(DiPainter* painter)
{
  if (!contr)
    return;

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (!gl)
    return;

  contr->plot(gl, true, false); // draw underlay
}

void GLwidget::paintOverlay(DiPainter* painter)
{
  if (!contr)
    return;

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (!gl)
    return;

  contr->plot(gl, false, true); // draw overlay
}

//  Set up the OpenGL view port, matrix mode, etc.
void GLwidget::resize(int w, int h)
{
  if (contr)
    contr->setPlotWindow(w, h);

  plotw = w;
  ploth = h;

  Q_EMIT resized(w, h);
}

void GLwidget::setFlagsFromEventResult(const EventResult& res)
{
  changeCursor(res.newcursor);
  enable_background_buffer = res.enable_background_buffer;
  if (res.repaint && res.update_background_buffer)
    update_background_buffer = true;
}

// Sends all QMouseEvents off to controller. Return values are checked,
// and any GUI-action taken
bool GLwidget::handleMouseEvents(QMouseEvent* me)
{
  EventResult res;

  // Duplicate the event, but transform the position of the cursor into the
  // plot's coordinate system.
  QMouseEvent me2(me->type(), QPoint(me->x(), ploth - me->y()), me->globalPos(),
                  me->button(), me->buttons(), me->modifiers());
  // send event to controller
  contr->sendMouseEvent(&me2, res);
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
bool GLwidget::handleKeyEvents(QKeyEvent* ke)
{
  EventResult res;
  contr->sendKeyboardEvent(ke, res);
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

bool GLwidget::handleWheelEvents(QWheelEvent *we)
{
  if (useScrollwheelZoom() && we->orientation() == Qt::Vertical) {
    int numDegrees = we->delta() / 8;
    int numSteps = numDegrees / 15;
    if (numSteps > 0) {
      float x1, y1, x2, y2;
      float xmap, ymap;

      contr->getPlotSize(x1, y1, x2, y2);
      /// (why -(y-height())? I have no idea ...)
      contr->PhysToMap(we->x(), -(we->y() - ploth), xmap, ymap);

      int wd = static_cast<int> ((x2 - x1) / 3.);
      int hd = static_cast<int> ((y2 - y1) / 3.);

      Rectangle r(xmap - wd, ymap - hd, xmap + wd, ymap + hd);
      contr->zoomTo(r);
    } else {
      contr->zoomOut();
    }
    update_background_buffer = true;
    return true;
  }
  return false;
}
