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
  , savebackground(false)
  , useSavedUnderlay(false)
  , fbuffer(0)
{
}

GLwidget::~GLwidget()
{
  delete[] fbuffer;
}

void GLwidget::setCanvas(DiCanvas* canvas)
{
  contr->setCanvas(canvas);
  delete fbuffer;
  fbuffer = 0;
}

void GLwidget::paint(DiPainter* painter)
{
  if (DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter)) {
    drawUnderlay(gl);
    drawOverlay(gl);
  }
}

void GLwidget::drawUnderlay(DiGLPainter* gl)
{
  if (useSavedUnderlay && fbuffer && !savebackground) {
    float glx1, gly1, glx2, gly2, delta;
    contr->getPlotSize(glx1, gly1, glx2, gly2);
    delta = (fabs(glx1 - glx2) * 0.1 / plotw);

    gl->PixelZoom(1, 1);
    gl->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_ROWS, 0);
    gl->PixelStorei(DiGLPainter::gl_UNPACK_SKIP_PIXELS, 0);
    gl->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, plotw);
    gl->PixelStorei(DiGLPainter::gl_UNPACK_ALIGNMENT, 4);
    gl->RasterPos2f(glx1 + delta, gly1 + delta);

    gl->DrawPixels(plotw, ploth, DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, fbuffer);
    gl->PixelStorei(DiGLPainter::gl_UNPACK_ROW_LENGTH, 0);
    return;
  }

  if (contr)
    contr->plot(gl, true, false); // draw underlay

  if (gl->supportsReadPixels() && ((useSavedUnderlay && !fbuffer) || savebackground)) {
    if (!fbuffer)
      fbuffer = new DiGLPainter::GLuint[4 * plotw * ploth];

    gl->PixelZoom(1, 1);
    gl->PixelStorei(DiGLPainter::gl_PACK_SKIP_ROWS, 0);
    gl->PixelStorei(DiGLPainter::gl_PACK_SKIP_PIXELS, 0);
    gl->PixelStorei(DiGLPainter::gl_PACK_ROW_LENGTH, plotw);
    gl->PixelStorei(DiGLPainter::gl_PACK_ALIGNMENT, 4);

    gl->ReadPixels(0, 0, plotw, ploth, DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, fbuffer);
    gl->PixelStorei(DiGLPainter::gl_PACK_ROW_LENGTH, 0);
  }
}

void GLwidget::drawOverlay(DiGLPainter* gl)
{
  if (contr)
    contr->plot(gl, false, true); // draw overlay
}

//  Set up the OpenGL view port, matrix mode, etc.
void GLwidget::resize(int w, int h)
{
  if (contr)
    contr->setPlotWindow(w, h);

  plotw = w;
  ploth = h;

  delete[] fbuffer;
  fbuffer = 0;
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

  // check return values, and take appropriate action
  changeCursor(res.newcursor);
  savebackground = res.savebackground;

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

  // check if repaint requested
  if (res.repaint)
    useSavedUnderlay = !res.background;
  return res.repaint;
}

// Sends all QKeyEvents off to controller. Return values are checked,
// and any GUI-action taken
bool GLwidget::handleKeyEvents(QKeyEvent* ke)
{
  EventResult res;

  // send event to controller
  contr->sendKeyboardEvent(ke, res);

  // check return values, and take appropriate action
  changeCursor(res.newcursor);
  savebackground = res.savebackground;

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

  // check if repaint requested
  if (res.repaint)
    useSavedUnderlay = !res.background;
  return res.repaint;
}

// ---------------------- event callbacks -----------------

bool GLwidget::handleWheelEvents(QWheelEvent *we)
{
  int numDegrees = we->delta() / 8;
  int numSteps = numDegrees / 15;

  if (contr->useScrollwheelZoom() && we->orientation() == Qt::Vertical) {
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
    useSavedUnderlay = false;
    return true;
  }
  return false;
}
