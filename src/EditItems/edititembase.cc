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

#include "edititembase.h"
#include <GL/gl.h>

EditItemBase::EditItemBase()
    : moving_(false)
    , resizing_(false)
{}

void EditItemBase::init()
{
  moving_ = false;
  resizing_ = false;
  pressedCtrlPointIndex_ = -1;
  hoveredCtrlPointIndex_ = -1;
}

void EditItemBase::copyBaseData(EditItemBase *item) const
{
  item->setGeometry(geometry());
}


qreal EditItemBase::sqr(qreal x) { return x * x; }

QList<QPointF> EditItemBase::geometry() const
{
  return ConstDrawing(this)->points_;
}

void EditItemBase::setGeometry(const QList<QPointF> &points)
{
    Drawing(this)->points_ = points;
    updateControlPoints();
}

QList<QPointF> EditItemBase::baseGeometry() const
{
    return basePoints_;
}

QList<QPointF> EditItemBase::getBasePoints() const
{
    return baseGeometry();
}

// Returns the index (>= 0)  of the control point hit by \a pos, or -1 if no
// control point was hit.
int EditItemBase::hitControlPoint(const QPointF &pos) const
{
    for (int i = 0; i < controlPoints_.size(); ++i)
        if (controlPoints_.at(i).contains(pos))
            return i;
    return -1;
}

// Moves the item by the specified amount (i.e. \a pos is relative to the item's current position).
void EditItemBase::moveBy(const QPointF &pos)
{
    baseMousePos_ = QPointF();
    basePoints_ = Drawing(this)->points_;
    move(pos);
}

void EditItemBase::move(const QPointF &pos)
{
    const QPointF delta = pos - baseMousePos_;
    Q_ASSERT(basePoints_.size() == points_.size());
    for (int i = 0; i < Drawing(this)->points_.size(); ++i)
        Drawing(this)->points_[i] = basePoints_.at(i) + delta;
    updateControlPoints();
}

void EditItemBase::drawControlPoints() const
{
    glColor3ub(0, 0, 0);
    foreach (QRectF c, controlPoints_) {
        glBegin(GL_POLYGON);
        glVertex3i(c.left(),  c.bottom(), 1);
        glVertex3i(c.right(), c.bottom(), 1);
        glVertex3i(c.right(), c.top(),    1);
        glVertex3i(c.left(),  c.top(),    1);
        glEnd();
    }
}

// Highlight the hovered control point.
void EditItemBase::drawHoveredControlPoint() const
{
  Q_ASSERT(hoveredCtrlPointIndex_ >= 0);
  const QRectF *r = &controlPoints_.at(hoveredCtrlPointIndex_);
  glPushAttrib(GL_LINE_BIT);
  glLineWidth(2);
  const int pad = 1;
  glBegin(GL_LINE_LOOP);
  glVertex3i(r->left() - pad,  r->bottom() + pad, 1);
  glVertex3i(r->right() + pad, r->bottom() + pad, 1);
  glVertex3i(r->right() + pad, r->top() - pad, 1);
  glVertex3i(r->left() - pad,  r->top() - pad, 1);
  glEnd();
  glPopAttrib();
}

// Draws the item.
// \a modes indicates whether the item is selected, hovered, both, or neither.
// \a incomplete is true iff the item is in the process of being completed (i.e. during manual placement of a new item).
// \a editingStyle is true iff the item's style is currently being edited.
void EditItemBase::draw(DrawModes modes, bool incomplete, bool editingStyle)
{
  if (incomplete)
    drawIncomplete();

  Drawing(this)->draw();

  // draw highlighting if hovered, unless we're editing the style (since then the highlighting would be in the way)
  if ((modes & Hovered) && !editingStyle)
    drawHoverHighlighting(incomplete);

  // draw control points if selected
  if (modes & Selected)
    drawControlPoints();
}

/**
 * The default draw method only draws normal, unselected, non-hovered, complete items.
 */
void EditItemBase::draw()
{
    draw(Normal, false);
}

/**
 * Handles a mouse press event for an item in its normal state.
 *
 * \a event is the event.
 *
 * \a repaintNeeded is set to true iff the scene needs to be repainted (typically if the item
 * modified it's state in a way that is reflected visually).
 *
 * Undo-commands representing the effect of this event should be inserted into \a undoCommands.
 * NOTE: commands must not be removed from this container (it may contain commands from other
 * items as well).
 *
 * \a items is, if non-null, a set of items that may potentially be operated on by the event
 * (always including this item).
 * Items may be inserted into or removed from this container to reflect how items were inserted or
 * removed as a result of the operation.
 * NOTE: While new items may be created (with the new operator), existing items must never be
 * deleted (using the delete operator) while in this function. This will be done from the outside.
 *
 * \a selItems is, if non-null, the subset of currently selected items.
 *
 * \a multiItemOp is, if non-null, set to true iff the event starts an operation that may involve
 * other items (such as a move operation).
 */
void EditItemBase::mousePress(
    QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
    QSet<QSharedPointer<DrawingItemBase> > *items, const QSet<QSharedPointer<DrawingItemBase> > *selItems, bool *multiItemOp)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(undoCommands)
    Q_UNUSED(items)
    Q_UNUSED(selItems)
    Q_UNUSED(multiItemOp)
}

/**
 * Handles a mouse press event for an item in the process of being completed (i.e. during manual
 * placement of a new item).
 *
 * \a complete is set to true iff the item is in a complete state upon returning from the function.
 *
 * \a aborted is set to true iff completing the item should be cancelled. This causes the item to
 * be deleted.
 *
 * See mousePress() for the documentation of other arguments.
 */
void EditItemBase::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(complete)
    Q_UNUSED(aborted)
}

void EditItemBase::mouseRelease(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands)
{
    Q_UNUSED(event);
    Q_UNUSED(repaintNeeded); // no need to set this
    Q_ASSERT(undoCommands);
    DrawingItemBase *ditem = dynamic_cast<DrawingItemBase *>(this);
    if ((moving_ || resizing_) && (ditem->getPoints() != getBasePoints()))
        undoCommands->append(new SetGeometryCommand(this, getBasePoints(), ditem->getPoints()));
    moving_ = resizing_ = false;
}

void EditItemBase::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
  if (moving_) {
    move(event->pos());
    repaintNeeded = true;
  } else if (resizing_) {
    resize(event->pos());
    repaintNeeded = true;
  }
}

void EditItemBase::mouseHover(QMouseEvent *event, bool &repaintNeeded)
{
  hoveredCtrlPointIndex_ = hitControlPoint(event->pos());
  repaintNeeded = true;
}

void EditItemBase::mouseDoubleClick(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::keyPress(
        QKeyEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
        QSet<QSharedPointer<DrawingItemBase> > *items, const QSet<QSharedPointer<DrawingItemBase> > *selItems)
{
    if (items && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))) {
        items->remove(QSharedPointer<DrawingItemBase>(Drawing(this)));
    } else if (
               (event->modifiers() & Qt::GroupSwitchModifier) && // "Alt Gr" modifier key
               ((event->key() == Qt::Key_Left)
                || (event->key() == Qt::Key_Right)
                || (event->key() == Qt::Key_Down)
                || (event->key() == Qt::Key_Up))) {
        QPointF pos;
        const qreal nudgeVal = 1; // nudge item by this much
        if (event->key() == Qt::Key_Left) pos += QPointF(-nudgeVal, 0);
        else if (event->key() == Qt::Key_Right) pos += QPointF(nudgeVal, 0);
        else if (event->key() == Qt::Key_Down) pos += QPointF(0, -nudgeVal);
        else pos += QPointF(0, nudgeVal); // Key_Up
        moveBy(pos);
        DrawingItemBase *ditem = Drawing(this);
        undoCommands->append(new SetGeometryCommand(this, getBasePoints(), ditem->getPoints()));
        repaintNeeded = true;
    }
}

void EditItemBase::keyRelease(QKeyEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(complete)
    Q_UNUSED(aborted)
}

void EditItemBase::incompleteMouseMove(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseDoubleClick(QMouseEvent *event, bool &repaintNeeded, bool &complete,
                                              bool &aborted)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(complete)
    Q_UNUSED(aborted)
}

void EditItemBase::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(complete)
    Q_UNUSED(aborted)
}

void EditItemBase::incompleteKeyRelease(QKeyEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

QVariantMap EditItemBase::clipboardVarMap() const
{
  QVariantMap vmap = ConstDrawing(this)->properties();
  vmap.insert("type", metaObject()->className());
  QVariantList vpoints;
  foreach (QPointF p, DrawingManager::instance()->PhysToGeo(ConstDrawing(this)->getPoints()))
    vpoints.append(p);
  vmap.insert("points", vpoints);
  return vmap;
}

QString EditItemBase::clipboardPlainText() const
{
  QString s;
  foreach (QPointF p, DrawingManager::instance()->PhysToGeo(ConstDrawing(this)->getPoints()))
    s.append(QString("(%1, %2) ").arg(p.x()).arg(p.y()));
  return s;
}

QList<QAction *> EditItemBase::actions(const QPoint &) const
{
  return QList<QAction *>();
}
