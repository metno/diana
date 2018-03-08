/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "diPaintGLWidget.h"

#include "diPaintable.h"
#include "qtUiEventHandler.h"
#include "qtUtility.h"

#include <QtGui>

#define MILOGGER_CATEGORY "diana.DiPaintGLWidget"
#include <miLogger/miLogging.h>

DiPaintGLWidget::DiPaintGLWidget(Paintable* p, UiEventHandler* i, QWidget* parent, bool aa)
    : QWidget(parent)
    , glcanvas(new DiPaintGLCanvas(this))
    , glpainter(new DiPaintGLPainter(glcanvas.get()))
    , paintable(p)
    , interactive(i)
    , background_buffer(0)
    , antialiasing(aa)
{
  setFocusPolicy(Qt::StrongFocus);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);
  glpainter->HIGH_QUALITY_BUT_SLOW = false;
  p->setCanvas(glcanvas.get());
}

DiPaintGLWidget::~DiPaintGLWidget()
{
  delete background_buffer;
}

void DiPaintGLWidget::dropBackgroundBuffer()
{
  delete background_buffer;
  background_buffer = 0;
}

void DiPaintGLWidget::paintEvent(QPaintEvent*)
{
  QPainter painter;
  painter.begin(this);
  if (paintable)
    paint(painter);
  painter.end();
}

void DiPaintGLWidget::resizeEvent(QResizeEvent* event)
{
  dropBackgroundBuffer();
  const QSize& size = event->size();
  glpainter->Viewport(0, 0, size.width(), size.height());
  paintable->resize(size);
}

void DiPaintGLWidget::keyPressEvent(QKeyEvent *ke)
{
  if (interactive && interactive->handleKeyEvents(ke))
    update();
}

void DiPaintGLWidget::keyReleaseEvent(QKeyEvent *ke)
{
  if (interactive && interactive->handleKeyEvents(ke))
    update();
}

void DiPaintGLWidget::mousePressEvent(QMouseEvent* me)
{
  if (interactive && interactive->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (interactive && interactive->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseReleaseEvent(QMouseEvent* me)
{
  if (interactive && interactive->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  if (interactive && interactive->handleMouseEvents(me))
    update();
}

void DiPaintGLWidget::wheelEvent(QWheelEvent *we)
{
  if (interactive && interactive->handleWheelEvents(we))
    update();
}

void DiPaintGLWidget::paint(QPainter& wpainter)
{
  METLIBS_LOG_SCOPE(LOGVAL(paintable->enable_background_buffer)
      << LOGVAL((!background_buffer))
      << LOGVAL(paintable->update_background_buffer));

  glpainter->clear = true;
  if (paintable->enable_background_buffer) {
    if (!background_buffer || paintable->update_background_buffer) {
      if (!background_buffer)
        background_buffer = new QImage(size(), QImage::Format_ARGB32);
      // clear+paint into bg image
      QPainter ipainter(background_buffer);
      ipainter.setRenderHint(QPainter::Antialiasing, antialiasing);
      glpainter->begin(&ipainter);
      METLIBS_LOG_DEBUG("underlay to image");
      paintable->paintUnderlay(glpainter.get());
      glpainter->end();
      ipainter.end();
      paintable->update_background_buffer = false;
    }

    // render bg from buffer
    METLIBS_LOG_DEBUG("underlay from image");
    wpainter.setRenderHint(QPainter::Antialiasing, false);
    wpainter.drawImage(QPoint(0,0), *background_buffer);
    wpainter.setRenderHint(QPainter::Antialiasing, antialiasing);
    glpainter->clear = false;
    glpainter->begin(&wpainter);
  } else {
    METLIBS_LOG_DEBUG("underlay direct");
    dropBackgroundBuffer();
    wpainter.setRenderHint(QPainter::Antialiasing, antialiasing);
    glpainter->begin(&wpainter);
    paintable->paintUnderlay(glpainter.get());
  }

  METLIBS_LOG_DEBUG("overlay");
  paintable->paintOverlay(glpainter.get());
  glpainter->end();
}
