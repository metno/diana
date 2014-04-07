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

#include "VcrossQtManager.h"

#include <QtGui/QPainter>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

#include <cmath>

#define MILOGGER_CATEGORY "diana.VcrossWidget"
#include <miLogger/miLogging.h>

namespace vcross {

QtWidget::QtWidget(QtManager_p vcm, QWidget* parent)
  : VCROSS_GL(QGLWidget(QGLFormat(QGL::SampleBuffers), parent),QWidget(parent))
  , vcrossm(vcm), arrowKeyDirection(1)
  , timeGraph(false), startTimeGraph(false)
{
  METLIBS_LOG_SCOPE();
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(false);

  dorubberband= false;
  dopanning= false;
}

QtWidget::~QtWidget()
{
}

void QtWidget::paintEvent(QPaintEvent* event)
{
  METLIBS_LOG_SCOPE();

  if (!vcrossm)
    return;

  QApplication::setOverrideCursor(Qt::WaitCursor);

  QPainter painter;
  painter.begin(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  vcrossm->plot(painter);
  painter.end();

  QApplication::restoreOverrideCursor();
}

void QtWidget::resizeEvent(QResizeEvent* event)
{
  const int w = event->size().width(), h = event->size().height();
  vcrossm->setPlotWindow(w,h);
  saveRasterImage("/tmp/vcross.png", "png");
}

// ---------------------- event callbacks -----------------

void QtWidget::keyPressEvent(QKeyEvent *me)
{
  if (dorubberband || dopanning)
    return;

  bool change= true;

  if (me->modifiers() & Qt::ControlModifier) {

    if (me->key()==Qt::Key_Left && timeGraph){
      vcrossm->setTimeGraphPos(-1);
    } else if (me->key()==Qt::Key_Right && timeGraph){
      vcrossm->setTimeGraphPos(+1);
    } else if (me->key()==Qt::Key_Left){
      vcrossm->setTime(-1);
      /*emit*/ timeChanged(-1);
    } else if (me->key()==Qt::Key_Right){
      vcrossm->setTime(+1);
      /*emit*/ timeChanged(+1);
    } else if (me->key()==Qt::Key_Down){
      vcrossm->setCrossection(-1);
      /*emit*/ crossectionChanged(-1);
    } else if (me->key()==Qt::Key_Up){
      vcrossm->setCrossection(+1);
      /*emit*/ crossectionChanged(+1);
    } else {
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
      change= false;
    }
  }

  if (change)
    update();
}


void QtWidget::mousePressEvent(QMouseEvent* me)
{
  mousex= me->x();
  mousey= height() - me->y();

  if (me->button()==Qt::LeftButton) {
    if (startTimeGraph) {
      vcrossm->setTimeGraphPos(mousex,mousey);
      startTimeGraph= false;
      timeGraph= true;
    } else {
      dorubberband= true;
      firstx= mousex;
      firsty= mousey;
    }
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
  }
}


void QtWidget::mouseReleaseEvent(QMouseEvent* me)
{
  const int rubberlimit= 15;

  if (dorubberband) {
    mousex= me->x();
    mousey= height() - me->y();
    if (abs(firstx-mousex)>rubberlimit ||
	abs(firsty-mousey)>rubberlimit)
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


void QtWidget::enableTimeGraph(bool on)
{
  METLIBS_LOG_SCOPE("on=" << on);

  timeGraph= false;
  startTimeGraph= on;
}


bool QtWidget::saveRasterImage(const std::string& fname,
    const std::string& format, const int quality)
{
  QImage image(width(), height(), QImage::Format_ARGB32);

  QPainter painter;
  painter.begin(&image);
  vcrossm->plot(painter);
  painter.end();

  image.save(QString::fromStdString(fname), format.c_str(), quality);

  return true;
}

} // namespace vcross
