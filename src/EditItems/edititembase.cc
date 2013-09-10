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

// ### --- BEGIN - Move this class to a separate .cc file? ---
SetGeometryCommand::SetGeometryCommand(
    EditItemBase *item, const QList<QPoint> &oldGeometry, const QList<QPoint> &newGeometry)
    : item_(item)
    , oldGeometry_(oldGeometry)
    , newGeometry_(newGeometry)
{}

void SetGeometryCommand::undo()
{
    item_->setPoints(oldGeometry_);
    item_->repaint();
}

void SetGeometryCommand::redo()
{
    item_->setPoints(newGeometry_);
    item_->repaint();
}
// ### --- END - Move this class to a separate .cc file? ---

EditItemBase::EditItemBase()
    : moving_(false)
    , resizing_(false)
    , id_(nextId())
{}

int EditItemBase::id() const { return id_; }

int EditItemBase::nextId()
{
    return nextId_++; // ### not thread safe; use a mutex for that
}

void EditItemBase::repaint()
{
    emit repaintNeeded();
}

int EditItemBase::nextId_ = 0;

/**
 * Handles a mouse press event for an item in its normal state.
 *
 * \a event is the event.
 *
 * \a repaintNeeded is set to true iff the scene needs to be repainted (typically if the item
 * modified it's state in a way that is reflected visually).
 *
 * Undo-commands representing the effect of this event may be inserted into  * \a undoCommands.
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
 * \a multiItemOp is, if non-null, set to true iff the event starts an operation that may involve
 * other items (such as a move operation).
 */
void EditItemBase::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                              QSet<EditItemBase *> *items, bool *multiItemOp)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(undoCommands)
    Q_UNUSED(items)
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
    if ((moving_ || resizing_) && (getPoints() != getBasePoints()))
        undoCommands->append(new SetGeometryCommand(this, getBasePoints(), getPoints()));
    moving_ = resizing_ = false;
}

void EditItemBase::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::mouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::mouseDoubleClick(QMouseEvent *event, bool &repaintNeeded)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
}

void EditItemBase::keyPress(QKeyEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                            QSet<EditItemBase *> *items)
{
    if (items && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))) {
        Q_ASSERT(items->contains(this));
        items->remove(this);
    } else if (
               (event->modifiers() & Qt::GroupSwitchModifier) && // "Alt Gr" modifier key
               ((event->key() == Qt::Key_Left)
                || (event->key() == Qt::Key_Right)
                || (event->key() == Qt::Key_Down)
                || (event->key() == Qt::Key_Up))) {
        QPoint pos;
        const int nudgeVal = 1; // nudge item by this much
        if (event->key() == Qt::Key_Left) pos += QPoint(-nudgeVal, 0);
        else if (event->key() == Qt::Key_Right) pos += QPoint(nudgeVal, 0);
        else if (event->key() == Qt::Key_Down) pos += QPoint(0, -nudgeVal);
        else pos += QPoint(0, nudgeVal); // Key_Up
        moveBy(pos);
        undoCommands->append(new SetGeometryCommand(this, getBasePoints(), getPoints()));
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

void EditItemBase::moveBy(const QPoint &pos)
{
    Q_UNUSED(pos);
}
