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
//#define DEBUGREDRAW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <QApplication>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPicture>

#define MILOGGER_CATEGORY "diana.GLwidget"
#include <miLogger/miLogging.h>

#include "qtGLwidget.h"
#include "diController.h"
#include "diDrawingManager.h"
#include "diEditItemManager.h"

#if defined(USE_PAINTGL)
#include <QPrinter>
#endif

#include <math.h>
#include <fstream>
#include <iostream>

#include <qpixmap.h>
#include <qcursor.h>
#include <paint_cursor.xpm>
#include <paint_add_crusor.xpm>
#include <paint_remove_crusor.xpm>
#include <paint_forbidden_crusor.xpm>

// GLwidget constructor
#if !defined(USE_PAINTGL)
GLwidget::GLwidget(Controller* c, const QGLFormat fmt, QWidget* parent) :
  QGLWidget(fmt, parent), curcursor(keep_it), contr(c), fbuffer(0)
#else
GLwidget::GLwidget(Controller* c, QWidget* parent) :
  PaintGLWidget(parent), curcursor(keep_it), contr(c), fbuffer(0)
#endif
{
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  savebackground = false;

  // sets default cursor in widget
  changeCursor(normal_cursor);

  EditItemManager *editm = static_cast<EditItemManager *>(contr->getManager("DRAWING"));
  if (editm)
    connect(editm, SIGNAL(repaintNeeded()), this, SLOT(editPaint())); // e.g. during undo/redo
}

//  Release allocated resources
GLwidget::~GLwidget()
{
  delete[] fbuffer;
}

void GLwidget::paintGL()
{

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("paintGL()");
#endif

#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("GLwidget::paintGL ... plot under");
#endif
  if (contr) {
    contr->plot(true, false); // draw underlay
  }

  DrawingManager *drawm = static_cast<DrawingManager *>(contr->getManager("DRAWING"));
  if (drawm && drawm->isEnabled()) {
    glColor3ub(128, 0, 0);
    renderText(10, height() - 10, "new painting mode (experimental)");
  }

  if (savebackground) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("GLwidget::paintGL ... savebackground");
#endif
    if (!fbuffer) {
      fbuffer = new GLuint[4 * plotw * ploth];
    }

    glPixelZoom(1, 1);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_ROW_LENGTH, plotw);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    glReadPixels(0, 0, plotw, ploth, GL_RGBA, GL_UNSIGNED_BYTE, fbuffer);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  }

#ifndef USE_PAINTGL
  editPaint(false);
#else
  if (contr)
    contr->plot(false, true); // draw overlay
#endif
}

void GLwidget::editPaint(bool drawb)
{
#ifdef USE_PAINTGL
  updateGL();
#else
  makeCurrent();

  if (drawb && fbuffer) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("GLwidget::editPaint ... drawbackground");
#endif
    float glx1, gly1, glx2, gly2, delta;
    contr->getPlotSize(glx1, gly1, glx2, gly2);
    delta = (fabs(glx1 - glx2) * 0.1 / plotw);

    glPixelZoom(1, 1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, plotw);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glRasterPos2f(glx1 + delta, gly1 + delta);

    glDrawPixels(plotw, ploth, GL_RGBA, GL_UNSIGNED_BYTE, fbuffer);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  }

#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("GLwidget::editPaint ... plot over");
#endif
  if (contr) {
    contr->plot(false, true); // draw overlay
  }

  swapBuffers();
#endif
}

//  Set up the OpenGL rendering state
void GLwidget::initializeGL()
{
  glShadeModel(GL_FLAT);
  setAutoBufferSwap(false);
}

//  Set up the OpenGL view port, matrix mode, etc.
void GLwidget::resizeGL(int w, int h)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("resizeGL");
#endif
  if (contr) {
    contr->setPlotWindow(w, h);
  }

  glViewport(0, 0, (GLint) w, (GLint) h);
  plotw = w;
  ploth = h;

  // make fake overlay buffer
  if (fbuffer){
    delete[] fbuffer;
    fbuffer = 0;
  }
}

// change mousepointer.
// See diMapMode.h for types
void GLwidget::changeCursor(const cursortype c)
{
  if ((c != keep_it) && (c != curcursor)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("About to change cursor to: " << c);
#endif
    switch (c) {
    case edit_cursor:
      setCursor(Qt::ArrowCursor);
      break;
    case edit_move_cursor:
      setCursor(Qt::ArrowCursor);
      break;
    case edit_value_cursor:
      setCursor(Qt::UpArrowCursor);
      break;
    case draw_cursor:
      setCursor(Qt::PointingHandCursor);
      break;
    case paint_select_cursor:
      setCursor(Qt::UpArrowCursor);
      break;
    case paint_move_cursor:
      setCursor(Qt::SizeAllCursor);
      break;
    case paint_draw_cursor:
      setCursor(QCursor(QPixmap(paint_cursor_xpm), 0, 16));
      break;
    case paint_add_crusor:
      setCursor(QCursor(QPixmap(paint_add_crusor_xpm), 7, 1));
      break;
    case paint_remove_crusor:
      setCursor(QCursor(QPixmap(paint_remove_crusor_xpm), 7, 1));
      break;
    case paint_forbidden_crusor:
      setCursor(QCursor(QPixmap(paint_forbidden_crusor_xpm), 7, 1));
      break;
    case normal_cursor:
    default:
      setCursor(Qt::ArrowCursor);
      break;
    }
    curcursor = c;
  }
}

// Sends all QMouseEvents off to controller. Return values are checked,
// and any GUI-action taken
void GLwidget::handleMouseEvents(QMouseEvent* me)
{
  EventResult res;

  // Duplicate the event, but transform the position of the cursor into the
  // plot's coordinate system.
  QMouseEvent me2(me->type(), QPoint(me->x(), height() - me->y()), me->globalPos(),
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
  if (res.repaint) {
    if (res.background) {
      updateGL(); // full paint
    } else {
      editPaint(); // only editPaint
    }
  }
}

// Sends all QKeyEvents off to controller. Return values are checked,
// and any GUI-action taken
void GLwidget::handleKeyEvents(QKeyEvent* ke)
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
  if (res.repaint) {
    if (res.background) {
      updateGL(); // full paint
    } else {
      editPaint(); // ..only editPaint
    }
  }
}

// ---------------------- event callbacks -----------------

void GLwidget::wheelEvent(QWheelEvent *we)
{
  int numDegrees = we->delta() / 8;
  int numSteps = numDegrees / 15;

  if (contr->useScrollwheelZoom() && we->orientation() == Qt::Vertical) {
    if (numSteps > 0) {
      float x1, y1, x2, y2;
      float xmap, ymap;

      contr->getPlotSize(x1, y1, x2, y2);
      /// (why -(y-height())? I have no idea ...)
      contr->PhysToMap(we->x(), -(we->y() - height()), xmap, ymap);

      int wd = static_cast<int> ((x2 - x1) / 3.);
      int hd = static_cast<int> ((y2 - y1) / 3.);

      Rectangle r(xmap - wd, ymap - hd, xmap + wd, ymap + hd);
      contr->zoomTo(r);
      updateGL();
    } else {
      contr->zoomOut();
      updateGL();
    }
  }
}

void GLwidget::keyPressEvent(QKeyEvent *ke)
{
  handleKeyEvents(ke);
}

void GLwidget::keyReleaseEvent(QKeyEvent *ke)
{
  handleKeyEvents(ke);
}

void GLwidget::mousePressEvent(QMouseEvent* me)
{
  handleMouseEvents(me);
}

void GLwidget::mouseMoveEvent(QMouseEvent* me)
{
  handleMouseEvents(me);
}

void GLwidget::mouseReleaseEvent(QMouseEvent* me)
{
  handleMouseEvents(me);
}

void GLwidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  handleMouseEvents(me);
}

// start hardcopy plot
void GLwidget::startHardcopy(const printOptions& po)
{
  makeCurrent();
  contr->startHardcopy(po);
}

// end hardcopy plot
void GLwidget::endHardcopy()
{
  makeCurrent();
  contr->endHardcopy();
}

bool GLwidget::saveRasterImage(const std::string fname, const std::string format,
    const int quality)
{

  updateGL();
  makeCurrent();
  glFlush();

  // test of new grabFrameBuffer command
  QImage image = grabFrameBuffer(true); // withAlpha=TRUE
  image.save(fname.c_str(), format.c_str(), quality);

  return true;
}
