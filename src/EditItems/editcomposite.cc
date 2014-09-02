/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#include "drawingcomposite.h"
#include "editcomposite.h"
#include "editsymbol.h"
#include "edittext.h"
#include <QDebug>

namespace EditItem_Composite {

Composite::Composite()
{
}

Composite::~Composite()
{
}

DrawingItemBase *Composite::cloneSpecial() const
{
  Composite *item = new Composite;
  copyBaseData(item);
  // ### copy special data from this into item ... TBD
  return item;
}

bool Composite::hit(const QPointF &pos, bool selected) const
{
  QRectF box(points_.at(0), points_.at(1));
  return box.contains(pos);
}

bool Composite::hit(const QRectF &bbox) const
{
  QRectF box(points_.at(0), points_.at(1));
  return box.intersects(bbox);
}

void Composite::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                           QSet<QSharedPointer<DrawingItemBase> > *items,
                           const QSet<QSharedPointer<DrawingItemBase> > *selItems,
                           bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    pressedCtrlPointIndex_ = -1;
    resizing_ = (pressedCtrlPointIndex_ >= 0);
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items
  }
}

/**
 * Processes a mouse press event when the object is incomplete.
 * This implementation only handles left button clicks, adding a single point to the
 * internal list on the first click.
 * Any additional clicks cause the object to be marked as complete.
 */
void Composite::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;

  if (event->button() == Qt::LeftButton) {
    // Create two points: one for the current mouse position and another to be
    // updated during the following move events.
    points_.append(QPointF(event->pos()));
    points_.append(QPointF(event->pos()));

    // Ensure that the manager gets the keyboard focus so that key events are
    // delivered to this item.
    EditItemManager::instance()->setFocus(true);

    // Update the geographic points and the control points.
    setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));
    createElements();
    updateRect();

    complete = true;
  }
}

void Composite::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;
}

void Composite::move(const QPointF &pos)
{
  const QPointF delta = pos - baseMousePos_;
  QList<QPointF> newPoints;
  for (int i = 0; i < points_.size(); ++i)
    newPoints.append(basePoints_.at(i) + delta);

  setPoints(newPoints);
  updateControlPoints();
}

void Composite::resize(const QPointF &)
{
}

void Composite::updateControlPoints()
{
}

void Composite::setPoints(const QList<QPointF> &points)
{
  QPointF offset;

  if (points_.size() > 0) {
    // Calculate the offset for each of the points using the first point passed.
    offset = points.at(0) - points_.at(0);
  } else
    offset = QPointF(0, 0);

  // Adjust the child elements using the offset.
  foreach (DrawingItemBase *element, elements_) {

    QList<QPointF> newPoints;
    foreach (QPointF point, element->getPoints())
      newPoints.append(point + offset);

    element->setPoints(newPoints);
    element->setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*element));
  }

  // Update the list with the new points.
  setGeometry(points);
}

void Composite::drawHoverHighlighting(bool) const
{
  QRectF bbox = boundingRect();

  glColor3ub(255, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox.bottomLeft().x(), bbox.bottomLeft().y());
  glVertex2f(bbox.bottomRight().x(), bbox.bottomRight().y());
  glVertex2f(bbox.topRight().x(), bbox.topRight().y());
  glVertex2f(bbox.topLeft().x(), bbox.topLeft().y());
  glEnd();
}

void Composite::drawIncomplete() const
{
}

} // namespace EditItem_Composite
