/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "VcrossQtWidget.h"

#include "qtUtility.h"
#include "vcross_v2/VcrossQtManager.h"

#include <QPainter>
#include <QPrinter>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>

#include <cmath>

#define MILOGGER_CATEGORY "diana.VcrossWidget"
#include <miLogger/miLogging.h>

namespace vcross {

QtWidget::QtWidget(QWidget* parent)
  : VCROSS_GL(QGLWidget(QGLFormat(QGL::SampleBuffers), parent),QWidget(parent))
  , arrowKeyDirection(1)
{
  METLIBS_LOG_SCOPE();
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  dorubberband= false;
  dopanning= false;
}

QtWidget::~QtWidget()
{
}

void QtWidget::setVcrossManager(QtManager_p vcm)
{
  if (vcrossm == vcm)
    return;

  if (vcrossm) {
    disconnect(vcrossm.get(), SIGNAL(timeGraphModeChanged(bool)),
        this, SLOT(switchedTimeGraph(bool)));
  }

  vcrossm = vcm;

  if (vcrossm) {
    connect(vcrossm.get(), SIGNAL(timeGraphModeChanged(bool)),
        this, SLOT(switchedTimeGraph(bool)));
  }
}

void QtWidget::paintEvent(QPaintEvent* event)
{
  METLIBS_LOG_SCOPE();

  if (!vcrossm)
    return;

  diutil::OverrideCursor waitCursor;

  QPainter painter;
  painter.begin(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  vcrossm->plot(painter);
  painter.end();
}

void QtWidget::resizeEvent(QResizeEvent* event)
{
  if (vcrossm) {
    const int w = event->size().width(), h = event->size().height();
    vcrossm->setPlotWindow(w,h);
  }
}

// ---------------------- event callbacks -----------------

void QtWidget::keyPressEvent(QKeyEvent *me)
{
  if (dorubberband || dopanning || !vcrossm)
    return;

  bool change= true, handled = true;

  if (me->modifiers() & Qt::ControlModifier) {

    if (me->key()==Qt::Key_Left) {
      Q_EMIT stepTime(-1);
    } else if (me->key()==Qt::Key_Right) {
      Q_EMIT stepTime(+1);
    } else if (me->key()==Qt::Key_Down) {
      Q_EMIT stepCrossection(-1);
    } else if (me->key()==Qt::Key_Up) {
      Q_EMIT stepCrossection(+1);
    } else {
      handled = false;
      change= false;
    }
  } else {
    float x1, y1, x2, y2;
    Colour rbc;
    vcrossm->getPlotSize(x1, y1, x2, y2, rbc);
    const int plotw = fabs(x1-x2), ploth = fabs(y1-y2);
    if (me->key()==Qt::Key_Left){
      vcrossm->movePart(-arrowKeyDirection*plotw/8, 0);
    } else if (me->key()==Qt::Key_Right){
      vcrossm->movePart(arrowKeyDirection*plotw/8, 0);
    } else if (me->key()==Qt::Key_Down){
      vcrossm->movePart(0, -arrowKeyDirection*ploth/8);
    } else if (me->key()==Qt::Key_Up){
      vcrossm->movePart(0, arrowKeyDirection*ploth/8);
    } else if (me->key()==Qt::Key_X) {
      vcrossm->increasePart();
    } else if (me->key()==Qt::Key_Z && me->modifiers() & Qt::ShiftModifier) {
      vcrossm->increasePart();
    } else if (me->key()==Qt::Key_Z) {
      const int dw= int(plotw/1.3), dh= int(ploth/1.3);
      vcrossm->decreasePart(x1+dw,y1-dh,x2-dw,y2+dh);
    } else if (me->key()==Qt::Key_Home) {
      vcrossm->standardPart();
    } else if (me->key()==Qt::Key_R) {
      arrowKeyDirection *= -1;
      change= false;
    } else {
      handled = false;
      change= false;
    }
  }

  if (change)
    update();
  else if (!handled)
    VCROSS_GL(QGLWidget,QWidget)::keyPressEvent(me);
}

void QtWidget::mousePressEvent(QMouseEvent* me)
{
  if (!vcrossm)
    return;

  mousex= me->x();
  mousey= height() - me->y();

  if (me->button()==Qt::LeftButton) {
    dorubberband= true;
    firstx= mousex;
    firsty= mousey;
  } else if (me->button()==Qt::MidButton) {
    dopanning= true;
    firstx= mousex;
    firsty= mousey;
  } else if (me->button()==Qt::RightButton) {
    vcrossm->increasePart();
  }

  update();
}


void QtWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (!vcrossm)
    return;

  if (dorubberband) {
    mousex= me->x();
    mousey= height() - me->y();
    update();
  } else if (dopanning) {
    mousex= me->x();
    mousey= height() - me->y();
    vcrossm->movePart(firstx-mousex, firsty-mousey);
    firstx= mousex;
    firsty= mousey;
    update();
  } else {
    const QString text = vcrossm->axisPosition(me->x(), me->y());
    Q_EMIT mouseOverText(text);
  }
}


void QtWidget::mouseReleaseEvent(QMouseEvent* me)
{
  if (!vcrossm)
    return;

  const int rubberlimit= 15;

  if (dorubberband) {
    mousex= me->x();
    mousey= height() - me->y();
    if (abs(firstx-mousex)>rubberlimit || abs(firsty-mousey)>rubberlimit)
      vcrossm->decreasePart(firstx,firsty,mousex,mousey);
    dorubberband= false;
    update();
  } else if (dopanning) {
//    mousex= me->x();
//    mousey= height() - me->y();
//    vcrossm->movePart(firstx-mousex, firsty-mousey);
    dopanning= false;
    update();
  }
}


void QtWidget::switchedTimeGraph(bool)
{
  update();
}

void QtWidget::print(QPrinter& printer)
{
  if (!vcrossm)
    return;

  const float scale_w = printer.pageRect().width() / float(width());
  const float scale_h = printer.pageRect().height() / float(height());
  const float scale = std::min(scale_w, scale_h);

  QPainter painter;
  painter.begin(&printer);
  painter.scale(scale, scale);
  vcrossm->plot(painter);
  painter.end();
}

bool QtWidget::saveRasterImage(const QString& fname)
{
  if (!vcrossm)
    return false;

  QImage image(width(), height(), QImage::Format_ARGB32_Premultiplied);

  QPainter painter;
  painter.begin(&image);
  vcrossm->plot(painter);
  painter.end();

  return image.save(fname);
}

} // namespace vcross
