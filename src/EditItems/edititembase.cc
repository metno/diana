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
#include "weatherarea.h"

EditItemBase::EditItemBase()
    : moving_(false)
    , resizing_(false)
{}

void EditItemBase::repaint()
{
    emit repaintNeeded();
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
 * If \a itemsToCopy is non-null, then items that should be copied (typically to the clipboard)
 * as a result of the event should be inserted into \a itemsToCopy.
 *
 * If \a itemsToEdit is non-null, then items that should be edited as a result of the event should
 * be inserted into \a itemsToEdit.
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
    QSet<DrawingItemBase *> *itemsToCopy, QSet<DrawingItemBase *> *itemsToEdit,
    QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems, bool *multiItemOp)
{
    Q_UNUSED(event)
    Q_UNUSED(repaintNeeded)
    Q_UNUSED(undoCommands)
    Q_UNUSED(itemsToCopy)
    Q_UNUSED(itemsToEdit)
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

void EditItemBase::keyPress(
        QKeyEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
        QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems)
{
    if (items && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))) {
        items->remove(Drawing(this));
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
        DrawingItemBase *ditem = dynamic_cast<DrawingItemBase *>(this);
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

void EditItemBase::moveBy(const QPointF &pos)
{
    Q_UNUSED(pos);
}

QVariantMap EditItemBase::clipboardVarMap() const
{
  QVariantMap vmap;
  vmap.insert("type", metaObject()->className());
  return vmap;
}

QString EditItemBase::clipboardPlainText() const
{
  return QString();
}

EditItemBase *EditItemBase::createItemFromVarMap(const QVariantMap &vmap, QString *error)
{
  Q_ASSERT(!vmap.empty());
  Q_ASSERT(vmap.contains("type"));
  Q_ASSERT(vmap.value("type").canConvert(QVariant::String));
  EditItemBase *item = 0;
  *error = QString();
  if (vmap.value("type").toString() == "EditItem_WeatherArea::WeatherArea") {
    EditItem_WeatherArea::WeatherArea *area = new EditItem_WeatherArea::WeatherArea(vmap, error);
    if (!error->isEmpty())
      delete area;
    else
      item = area;
  } else {
    *error = QString("unsupported item type: %1, expected %2")
        .arg(vmap.value("type").toString()).arg("EditItem_WeatherArea::WeatherArea");
  }
  return item;
}
