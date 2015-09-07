/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

#include "drawingpolyline.h"
#include "editpolyline.h"
#include "diGLPainter.h"
#include "qtStatusGeopos.h"

#include <diDrawingManager.h>
#include <EditItems/drawingstylemanager.h>

#include <QAction>
#include <QToolTip>

namespace EditItem_PolyLine {

PolyLine::PolyLine(int id)
  : DrawingItem_PolyLine::PolyLine(id)
  , addPoint_act_(new QAction(tr("Add point"), this))
  , removePoint_act_(new QAction(tr("Remove point"), this))
  , hoverLineIndex_(-1)
{
  init();
  updateControlPoints();
  color_.setRed(0);
  color_.setGreen(0);
  color_.setBlue(0);

  addPoint_act_->setShortcut(tr("+"));
  QObject::connect(addPoint_act_, SIGNAL(triggered()), SLOT(addPoint()));
  QObject::connect(removePoint_act_, SIGNAL(triggered()), SLOT(removePoints()));
}

PolyLine::~PolyLine()
{
}

DrawingItemBase *PolyLine::cloneSpecial(bool setUniqueId) const
{
  PolyLine *item = new PolyLine(setUniqueId ? -1 : id());
  copyBaseData(item);
  return item;
}

bool PolyLine::hit(const QPointF &pos, bool selected) const
{
  // Have we hit a control point?
  if (selected && (hitControlPoint(pos) >= 0))
    return true;

  return DrawingItem_PolyLine::PolyLine::hit(pos, selected);
}

QList<QAction *> PolyLine::actions(const QPoint &pos) const
{
  QList<QAction *> acts;
  if (hoverCtrlPointIndex_ >= 0) {
    if (points_.size() > 2)
      acts.append(removePoint_act_);
  } else if (hoverLineIndex_ >= 0) {
    acts.append(addPoint_act_);
  }

  return acts;
}

void PolyLine::updateHoverPos(const QPoint &pos)
{
  EditItemBase::updateHoverPos(pos);
  hoverCtrlPointIndex_ = hitControlPoint(hoverPos_);
}

void PolyLine::mousePress(QMouseEvent *event, bool &repaintNeeded, bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    mousePressControlPoints(event, repaintNeeded);
    resizing_ = !pressedCtrlPointIndex_.isEmpty();
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items

    EditItemBase::mousePress(event, repaintNeeded, multiItemOp);
  }
}

void PolyLine::mouseHover(QMouseEvent *event, bool &repaintNeeded, bool selectingOnly)
{
  EditItemBase::mouseHover(event, repaintNeeded);
  if ((hoverCtrlPointIndex_ < 0) && !(selectingOnly)) {
    hoverLineIndex_ = hitLine(hoverPos_);
    QToolTip::hideText();
  } else {
    hoverLineIndex_ = -1;
    showTip();
  }
}

void PolyLine::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
  EditItemBase::mouseMove(event, repaintNeeded);
  if (hoverCtrlPointIndex_ < 0) {
    hoverLineIndex_ = hitLine(hoverPos_);
    QToolTip::hideText();
  } else {
    hoverLineIndex_ = -1;
    showTip();
  }
}

void PolyLine::showTip()
{
  QPointF p = latLonPoints_.at(hoverCtrlPointIndex_);
  int latDeg, latMin, lonDeg, lonMin;
  StatusGeopos::degreesMinutes(p.x(), latDeg, latMin);
  StatusGeopos::degreesMinutes(p.y(), lonDeg, lonMin);

  QString text, ns = (p.x() >= 0.0) ? "N" : "S", we = (p.y() >= 0.0) ? "E" : "W";
  text = QString("%1\xB0 %2'%3").arg(latDeg).arg(latMin).arg(ns);
  text += QString(" %1\xB0 %2'%3").arg(lonDeg).arg(lonMin).arg(we);

  QToolTip::showText(QCursor::pos(), text);
}

void PolyLine::keyPress(QKeyEvent *event, bool &repaintNeeded)
{
  if ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete) ||
      (event->key() == Qt::Key_Minus)) {

    if (!pressedCtrlPointIndex_.isEmpty()) {
      // Remove selected points
      removePoints();
      repaintNeeded = true;
      event->accept();
      return;
    }

  } else if ((hoverCtrlPointIndex_ < 0) &&
             (hoverLineIndex_ >= 0) &&
             (hoverPos_ != QPoint(-1, -1)) &&
             ((event->key() == Qt::Key_Return) ||
              (event->key() == Qt::Key_Enter) ||
              (event->key() == Qt::Key_Plus) ||
              (event->key() == Qt::Key_Insert))) {

    // Add point
    const QList<QPointF> origPoints = getPoints();
    addPoint();
    hoverCtrlPointIndex_ = hitControlPoint(hoverPos_);
    repaintNeeded = true;
    event->accept();
    return;

  } else if (event->modifiers() & Qt::ShiftModifier) {

    // Nudge polyline or point

    DrawingManager *drawm = DrawingManager::instance();
    QList<QPointF> latLonPoints = drawm->PhysToGeo(points_);

    foreach (const int index, pressedCtrlPointIndex_) {

      // Obtain the geographic coordinates of each pressed control point.
      QPointF p = latLonPoints.at(index);
      float latMin = qRound(p.x() * 60);
      float lonMin = qRound(p.y() * 60);

      switch (event->key()) {
      case Qt::Key_Left:
        lonMin -= 1;
        break;
      case Qt::Key_Right:
        lonMin += 1;
        break;
      case Qt::Key_Down:
        latMin -= 1;
        break;
      case Qt::Key_Up:
        latMin += 1;
        break;
      default:
        continue;
      }

      float lat = qMin(qMax(-90.0, latMin/60.0), 90.0);
      float lon = lonMin/60.0;

      if (lon < -180.0) lon += 360.0;
      if (lon >= 180.0) lon -= 360.0;

      latLonPoints[index] = QPointF(lat, lon);
      repaintNeeded = true;
    }

    if (repaintNeeded) {
      QList<QPointF> newPoints = drawm->GeoToPhys(latLonPoints);

      foreach (const int index, pressedCtrlPointIndex_)
        movePointTo(index, newPoints.at(index));

      event->accept();
      return;
    }
  }

  EditItemBase::keyPress(event, repaintNeeded);
}

void PolyLine::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (event->button() == Qt::LeftButton) {
    if (points_.size() == 0)
      points_.append(QPointF(event->pos()));
    points_.append(QPointF(event->pos()));
    repaintNeeded = true;
  } else if (event->button() == Qt::RightButton) {
    if (points_.size() >= 2) {
      complete = true; // causes repaint
    } else {
      aborted = true; // not a complete multiline
      repaintNeeded = true;
    }
  }
}

void PolyLine::incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded)
{
  if (points_.size() > 0) {
    points_.last() = QPointF(event->pos());
    hoverCtrlPointIndex_ = points_.size() - 1;
    showTip();
    repaintNeeded = true;
  } else
    QToolTip::hideText();
}

void PolyLine::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter)) {
    if (points_.size() >= 2) {
      complete = true; // causes repaint
    } else {
      aborted = true; // not a complete multiline
      repaintNeeded = true;
    }
  } else if (event->key() == Qt::Key_Escape) {
    aborted = true;
    if (points_.size() > 0)
      repaintNeeded = true;
  }
}

void PolyLine::resize(const QPointF &pos)
{
  const QPointF delta = pos - baseMousePos_;
  Q_ASSERT(!pressedCtrlPointIndex_.isEmpty());
  Q_ASSERT(basePoints_.size() == points_.size());
  foreach (const int index, pressedCtrlPointIndex_)
    points_[index] = basePoints_.at(index) + delta;

  updateControlPoints();
}

void PolyLine::updateControlPoints()
{
  controlPoints_.clear();
  const int size = controlPointSize(), size_2 = size / 2;
  foreach (QPointF p, points_)
    controlPoints_.append(QRectF(p.x() - size_2, p.y() - size_2, size, size));
}

void PolyLine::setPoints(const QList<QPointF> &points)
{
  setGeometry(points);
}

void PolyLine::addPoint()
{
  if (hoverLineIndex_ >= 0) {
    points_.insert(hoverLineIndex_ + 1, hoverPos_);
    setLatLonPoints(DrawingManager::instance()->getLatLonPoints(this));
    hoverLineIndex_ = -1;
    updateControlPoints();
  }
}

void PolyLine::removePoints()
{
  QList<QPointF> newPoints, newLatLonPoints;

  for (int i = 0; i < points_.size(); ++i) {
    if (!pressedCtrlPointIndex_.contains(i)) {
      newPoints.append(points_.at(i));
      newLatLonPoints.append(latLonPoints_.at(i));
    }
  }

  if (newPoints.size() < 2)
    return;

  points_ = newPoints;
  latLonPoints_ = newLatLonPoints;

  QSet<int> origHCPIndexes = pressedCtrlPointIndex_;
  pressedCtrlPointIndex_.clear();
  if (hoverCtrlPointIndex_ >= points_.size())
    hoverCtrlPointIndex_ = -1;

  updateControlPoints();

  // unjoin polyline if removing a joined end point
  if ((origHCPIndexes.contains(0) && (joinId() < 0)) || (origHCPIndexes.contains(points_.size()) && (joinId() > 0))) {
    propertiesRef().insert("joinId", 0);
    EditItemManager::instance()->updateJoins();
  }
}

void PolyLine::drawIncomplete(DiGLPainter*) const
{
}

void PolyLine::drawHoverHighlightingBG(DiGLPainter* gl, bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  // highlight the polyline
  bool ok = false;
  const int lineWidth = properties().value("style:linewidth").toInt(&ok);
  const int defaultLineWidth = 2;
  const int pad = 6;
  DrawingStyleManager::instance()->highlightPolyLine(gl, this, points_,
      (ok ? lineWidth : defaultLineWidth) + pad, QColor(255, 255, 0, 180));

  // highlight the control points
  drawControlPoints(gl, QColor(255, 0, 0, 255));
}

void PolyLine::drawHoverHighlighting(DiGLPainter* gl, bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  if (hoverCtrlPointIndex_ >= 0) {
    // highlight the control point
    drawHoveredControlPoint(gl, QColor(255, 0, 0, 255), 2);
    drawHoveredControlPoint(gl, QColor(255, 255, 0, 255));
  } else {
    // highlight the control points
    if (selected)
      drawControlPoints(gl, QColor(255, 0, 0, 255));

    if (selected && (hoverCtrlPointIndex_ < 0) && (hoverLineIndex_ >= 0)) {
      // highlight the insertion position of a new point
      gl->Color3ub(0, 200, 0);
      const int w = 4;
      const int w_2 = w/2;
      const QRectF r(hoverPos_.x() - w_2, hoverPos_.y() - w_2, w, w);
      gl->PushAttrib(DiGLPainter::gl_LINE_BIT);
      gl->LineWidth(2);
      const int pad = 1;
      gl->Begin(DiGLPainter::gl_LINE_LOOP);
      gl->Vertex3i(r.left() - pad,  r.bottom() + pad, 1);
      gl->Vertex3i(r.right() + pad, r.bottom() + pad, 1);
      gl->Vertex3i(r.right() + pad, r.top() - pad, 1);
      gl->Vertex3i(r.left() - pad,  r.top() - pad, 1);
      gl->End();
      gl->PopAttrib();
    }
  }
}

} // namespace EditItem_PolyLine
