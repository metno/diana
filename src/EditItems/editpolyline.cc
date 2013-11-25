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
#include <QDomDocument>
#include <QFileDialog>
#include <diTesselation.h>

namespace EditItem_PolyLine {

PolyLine::PolyLine()
{
    init();
    updateControlPoints();
    color_.setRed(0);
    color_.setGreen(0);
    color_.setBlue(0);
}

PolyLine::~PolyLine()
{
}

bool PolyLine::hit(const QPointF &pos, bool selected) const
{
    const qreal proximityTolerance = 3.0;
    const bool hitEdge = (points_.size() >= 2) && (distance(pos) < proximityTolerance);
    const bool hitSelectedControlPoint = selected && (hitControlPoint(pos) >= 0);
    const QPolygonF polygon(points_.toVector());
    const bool hitInterior = polygon.containsPoint(pos, Qt::OddEvenFill);
    return hitEdge || hitSelectedControlPoint || hitInterior;
}

bool PolyLine::hit(const QRectF &rect) const
{
    Q_UNUSED(rect);
    return false; // for now
}

/**
 * Returns the index of the line close to the position specified, or -1 if no
 * line was close enough.
 */
int PolyLine::hitLine(const QPointF &position) const
{
    if (points_.size() < 2)
        return -1;

    const qreal proximityTolerance = 3.0;
    qreal minDist = distance2(position, points_[0], points_[1]);
    int minIndex = 0;
    int n = points_.size();

    for (int i = 1; i < n; ++i) {
        const qreal dist = distance2(QPointF(position), QPointF(points_.at(i)), QPointF(points_.at((i + 1) % n)));
        if (dist < minDist) {
            minDist = dist;
            minIndex = i;
        }
    }
    
    if (minDist > proximityTolerance)
        return -1;

    return minIndex;
}

void PolyLine::mousePress(
    QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
    QSet<DrawingItemBase *> *itemsToCopy, QSet<DrawingItemBase *> *itemsToEdit,
    QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems, bool *multiItemOp)
{
    Q_ASSERT(undoCommands);

    if (event->button() == Qt::LeftButton) {
        pressedCtrlPointIndex_ = hitControlPoint(event->pos());
        resizing_ = (pressedCtrlPointIndex_ >= 0);
        moving_ = !resizing_;
        basePoints_ = points_;
        baseMousePos_ = event->pos();

        if (multiItemOp)
            *multiItemOp = moving_; // i.e. a move operation would apply to all selected items

    } else if (event->button() == Qt::RightButton) {
        if (selItems) {
            // open a context menu and perform the selected action
            QMenu contextMenu;
            QPointF position = event->pos();
            QAction addPoint_act(tr("&Add point"), 0);
            QAction remove_act(tr("&Remove"), 0);
            QAction removePoint_act(tr("Remove &point"), 0);
            QAction copyItems_act(tr("&Copy"), 0);
            QAction editItems_act(tr("P&roperties..."), 0);
            QAction save_act(tr("Save ..."), 0);

            // Add actions, checking for a click on a line or a point.
            const int lineIndex = hitLine(position);
            const int pointIndex = hitControlPoint(position);
            if (pointIndex != -1) {
                if (points_.size() <= 3)
                    return; // an area needs at least three points
                contextMenu.addAction(&removePoint_act);
            } else if (lineIndex != -1) {
                contextMenu.addAction(&addPoint_act);
            }
            contextMenu.addAction(&remove_act);
            if (itemsToCopy)
              contextMenu.addAction(&copyItems_act);
            if (itemsToEdit)
              contextMenu.addAction(&editItems_act);
            contextMenu.addAction(&save_act);
            QAction *action = contextMenu.exec(event->globalPos(), &remove_act);
            if (action == &remove_act)
                remove(repaintNeeded, items, selItems);
            else if (action == &removePoint_act)
                removePoint(repaintNeeded, pointIndex, items, selItems);
            else if (action == &addPoint_act)
                addPoint(repaintNeeded, lineIndex, position);
            else if (action == &copyItems_act) {
                Q_ASSERT(itemsToCopy);
                QSet<DrawingItemBase *>::const_iterator it;
                for (it = selItems->begin(); it != selItems->end(); ++it) {
                    PolyLine *polyLine = qobject_cast<PolyLine *>(Editing(*it));
                    if (polyLine)
                        itemsToCopy->insert(polyLine);
                }
            } else if (action == &editItems_act) {
                Q_ASSERT(itemsToEdit);
                //Q_ASSERT(items->contains(this));
                itemsToEdit->insert(this);
            } else if (action == &save_act) {
              EditItemManager::instance()->saveItemsToFile();
            }
        }
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

void PolyLine::addPoint(bool &repaintNeeded, int index, const QPointF &point)
{
    points_.insert(index + 1, point);
    setLatLonPoints(DrawingManager::instance()->getLatLonPoints(this));

    updateControlPoints();
    repaintNeeded = true;
}

void PolyLine::remove(bool &repaintNeeded, QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems)
{
    // Option 1: remove this item only:
    // items->remove(this);

    // Option 2: remove all selected items:
    items->subtract(*selItems);

    repaintNeeded = true;
}

void PolyLine::removePoint(bool &repaintNeeded, int index, QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems)
{
    if (points_.size() <= 3)
        items->remove(this);

    if (index >= 0 && index < points_.size()) {
        points_.removeAt(index);
        latLonPoints_.removeAt(index);
    }

    updateControlPoints();
    repaintNeeded = true;
}

qreal PolyLine::dist2(const QPointF &v, const QPointF &w)
{
  return sqr(v.x() - w.x()) + sqr(v.y() - w.y());
}

// Returns the distance between \a p and the line between \a v and \a w.
qreal PolyLine::distance2(const QPointF &p, const QPointF &v, const QPointF &w)
{
    const qreal l2 = dist2(v, w);
    if (l2 == 0) return sqrt(dist2(p, v));
    Q_ASSERT(l2 > 0);
    const qreal t = ((p.x() - v.x()) * (w.x() - v.x()) + (p.y() - v.y()) * (w.y() - v.y())) / l2;
    if (t < 0) return sqrt(dist2(p, v));
    if (t > 1) return sqrt(dist2(p, w));
    QPointF p2(v.x() + t * (w.x() - v.x()), v.y() + t * (w.y() - v.y()));
    return sqrt(dist2(p, p2));
}

// Returns the distance between \a p and the multiline (i.e. the mimimum distance between \a p and any of the line segments).
// If the multiline contains fewer than two points, the function returns -1.
qreal PolyLine::distance(const QPointF &p) const
{
    if (points_.size() < 2)
        return -1;

    int n = points_.size();

    qreal minDist = -1;
    for (int i = 1; i <= n; ++i) {
        const qreal dist = distance2(QPointF(p), QPointF(points_.at(i - 1)), QPointF(points_.at(i % n)));
        minDist = (i == 1) ? dist : qMin(minDist, dist);
    }
    Q_ASSERT(minDist >= 0);
    return minDist;
}

void PolyLine::drawIncomplete() const
{
}

void PolyLine::drawHoverHighlighting(bool incomplete) const
{
  if (incomplete)
    return;

  glColor3ub(255, 0, 0);

  if (hoveredCtrlPointIndex_ >= 0) {
    EditItemBase::drawHoveredControlPoint();
  } else {
    // highlight the polyline
    glPushAttrib(GL_LINE_BIT);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    foreach (QPointF p, points_)
      glVertex3i(p.x(), p.y(), 1);
    glEnd();
    glPopAttrib();
  }
}

} // namespace EditItem_PolyLine
