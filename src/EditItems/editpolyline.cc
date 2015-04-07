/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include <GL/gl.h>
#include "drawingpolyline.h"
#include "editpolyline.h"

#include <diDrawingManager.h>
#include <EditItems/drawingstylemanager.h>

#include <QAction>

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

  QObject::connect(addPoint_act_, SIGNAL(triggered()), SLOT(addPoint()));
  QObject::connect(removePoint_act_, SIGNAL(triggered()), SLOT(removePoint()));
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
    pressedCtrlPointIndex_ = hitControlPoint(event->pos());
    resizing_ = (pressedCtrlPointIndex_ >= 0);
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
  } else {
    hoverLineIndex_ = -1;
  }
}

void PolyLine::keyPress(QKeyEvent *event, bool &repaintNeeded)
{
  if (((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete)
       || (event->key() == Qt::Key_Minus)) && (hoverCtrlPointIndex_ >= 0)) {
    const QList<QPointF> origPoints = getPoints();
    removePoint();
    hoverCtrlPointIndex_ = hitControlPoint(hoverPos_);
    if (hoverCtrlPointIndex_ < 0) // no control point beneath the one we just removed, so check if we hit a line
      hoverLineIndex_ = hitLine(hoverPos_);
    repaintNeeded = true;
    event->accept();
  } else if (((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter)
              || (event->key() == Qt::Key_Plus) || (event->key() == Qt::Key_Insert))
             && (hoverCtrlPointIndex_ < 0) && (hoverLineIndex_ >= 0) && (hoverPos_ != QPoint(-1, -1))) {
    const QList<QPointF> origPoints = getPoints();
    addPoint();
    hoverCtrlPointIndex_ = hitControlPoint(hoverPos_);
    repaintNeeded = true;
    event->accept();
  } else {
    EditItemBase::keyPress(event, repaintNeeded);
  }
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
    repaintNeeded = true;
  }
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
  Q_ASSERT(pressedCtrlPointIndex_ >= 0);
  Q_ASSERT(pressedCtrlPointIndex_ < controlPoints_.size());
  Q_ASSERT(basePoints_.size() == points_.size());
  points_[pressedCtrlPointIndex_] = basePoints_.at(pressedCtrlPointIndex_) + delta;
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
    setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));
    hoverLineIndex_ = -1;
    updateControlPoints();
  }
}

void PolyLine::removePoint()
{
  if ((hoverCtrlPointIndex_ >= 0) && (hoverCtrlPointIndex_ < points_.size()) && (points_.size() > 2)) {
    points_.removeAt(hoverCtrlPointIndex_);
    latLonPoints_.removeAt(hoverCtrlPointIndex_);
    const int origHCPIndex = hoverCtrlPointIndex_;
    hoverCtrlPointIndex_ = -1;
    updateControlPoints();

    // unjoin polyline if removing a joined end point
    if (((origHCPIndex == 0) && (joinId() < 0)) || ((origHCPIndex == points_.size()) && (joinId() > 0))) {
      propertiesRef().insert("joinId", 0);
      EditItemManager::instance()->updateJoins();
    }
  }
}

void PolyLine::drawIncomplete() const
{
}

void PolyLine::drawHoverHighlightingBG(bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  // highlight the polyline
  bool ok = false;
  const int lineWidth = properties().value("style:linewidth").toInt(&ok);
  const int defaultLineWidth = 2;
  const int pad = 6;
  DrawingStyleManager::instance()->highlightPolyLine(this, points_, (ok ? lineWidth : defaultLineWidth) + pad, QColor(255, 255, 0, 180));

  // highlight the control points
  drawControlPoints(QColor(255, 0, 0, 255));
}

void PolyLine::drawHoverHighlighting(bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  if (hoverCtrlPointIndex_ >= 0) {
    // highlight the control point
    drawHoveredControlPoint(QColor(255, 0, 0, 255), 2);
    drawHoveredControlPoint(QColor(255, 255, 0, 255));
  } else {
    // highlight the control points
    if (selected)
      drawControlPoints(QColor(255, 0, 0, 255));

    if (selected && (hoverCtrlPointIndex_ < 0) && (hoverLineIndex_ >= 0)) {
      // highlight the insertion position of a new point
      glColor3ub(0, 200, 0);
      const int w = 4;
      const int w_2 = w/2;
      const QRectF r(hoverPos_.x() - w_2, hoverPos_.y() - w_2, w, w);
      glPushAttrib(GL_LINE_BIT);
      glLineWidth(2);
      const int pad = 1;
      glBegin(GL_LINE_LOOP);
      glVertex3i(r.left() - pad,  r.bottom() + pad, 1);
      glVertex3i(r.right() + pad, r.bottom() + pad, 1);
      glVertex3i(r.right() + pad, r.top() - pad, 1);
      glVertex3i(r.left() - pad,  r.top() - pad, 1);
      glEnd();
      glPopAttrib();
    }
  }
}

} // namespace EditItem_PolyLine
