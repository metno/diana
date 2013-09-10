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

#include <QtGui>

#include <diDrawingManager.h>
#include <diEditItemManager.h>
#include <EditItems/edititembase.h>

static QString undoCommandText(int nadded, int nremoved, int nmodified)
{
    QString s;
    if (nadded > 0)
        s += QString("add %1 item%2").arg(nadded).arg((nadded != 1) ? "s" : "");
    if (nremoved > 0)
        s += QString("%1remove %2 item%3").arg(s.isEmpty() ? "" : ", ").arg(nremoved).arg((nremoved != 1) ? "s" : "");
    if (nmodified > 0)
        s += QString("%1modify %2 item%3").arg(s.isEmpty() ? "" : ", ").arg(nmodified).arg((nmodified != 1) ? "s" : "");
    return s;
}

void EditItemManager::createUndoView()
{
    QUndoView *undoView = new QUndoView(&undoStack_);
    undoView->setWindowTitle("EditItemManager undo stack");
    undoView->resize(600, 300);
    undoView->show();
}

EditItemManager::EditItemManager(DrawingManager *drawm)
    : hoverItem_(0)
    , incompleteItem_(0)
    , repaintNeeded_(false)
    , skipRepaint_(false)
    , drawingManager_(drawm)
{
    connect(&undoStack_, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
    connect(&undoStack_, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
}

EditItemManager::~EditItemManager()
{
}

// Adds an item to the scene. \a incomplete indicates whether the item is in the process of being manually placed.
void EditItemManager::addItem(EditItemBase *item, bool incomplete)
{
    if (incomplete) {
        // set this item as the incomplete item
        if (incompleteItem_) {
            // issue warning?
        }
        incompleteItem_ = item;
        emit incompleteEditing(true);
    } else {
        // create undo command
        QSet<EditItemBase *> addedItems;
        addedItems.insert(item);
        QSet<EditItemBase *> removedItems;
        AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
        arCmd->eim_ = this;
        undoStack_.push(arCmd);
    }

//    qDebug() << "   ###### addItem()" << item << ", incomplete:" << incomplete << ", item infos:";
//    foreach (EditItemBase *item, items_)
//        qDebug() << "   ###### - " << item->infoString().toLatin1().data();

    repaint();
}

void EditItemManager::addItem_(EditItemBase *item)
{
    items_.insert(item);
    connect(item, SIGNAL(repaintNeeded()), this, SLOT(repaint()));
    if (false) selItems_.insert(item); // for now, don't pre-select new items
}

void EditItemManager::removeItem(EditItemBase *item)
{
    items_.remove(item);
    disconnect(item, SIGNAL(repaintNeeded()), this, SLOT(repaint()));
    selItems_.remove(item);
}

void EditItemManager::storeItems(const QSet<EditItemBase *> &items)
{
    foreach (EditItemBase *item, items) {
        // Convert the item's screen coordinates to geographic coordinates.
        item->setLatLonPoints(drawingManager_->getLatLonPoints(item));
        removeItem(item);
    }
}

void EditItemManager::retrieveItems(const QSet<EditItemBase *> &items)
{
    foreach (EditItemBase *item, items) {
        // The items stored on the undo stack have been given geographic
        // coordinates, so we use those to obtain screen coordinates.
        if (!item->getLatLonPoints().isEmpty())
            drawingManager_->setLatLonPoints(item, item->getLatLonPoints());
        addItem_(item);
    }
}

QList<QPointF> EditItemManager::PhysToGeo(const QList<QPoint> &points)
{
    return drawingManager_->PhysToGeo(points);
}

QList<QPoint> EditItemManager::GeoToPhys(const QList<QPointF> &points)
{
    return drawingManager_->GeoToPhys(points);
}

void EditItemManager::reset()
{
    // FIXME: We need to make certain that this is how we want to handle the undo stack.
    QSet<EditItemBase *> addedItems;
    AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, items_);
    arCmd->eim_ = this;
    undoStack_.push(arCmd);
}

QUndoStack * EditItemManager::undoStack()
{
    return &undoStack_;
}

QSet<EditItemBase *> EditItemManager::getItems() const
{
    return items_;
}

QSet<EditItemBase *> EditItemManager::getSelectedItems() const
{
    return selItems_;
}

void EditItemManager::mousePress(QMouseEvent *event, QSet<EditItemBase *> *itemsToCopy)
{
    if (incompleteItem_) {
        incompleteMousePress(event);
        return;
    }

    const QSet<EditItemBase *> hitItems = findHitItems(event->pos());
    EditItemBase *hitItem = // consider only this item to be hit
        hitItems.empty()
        ? 0
        : *(hitItems.begin()); // for now; eventually use the one with higher z-value etc. ... 2 B DONE
    const bool hitSelItem = selItems_.contains(hitItem); // whether an already selected item was hit
    const bool selectMulti = event->modifiers() & Qt::ControlModifier;

    repaintNeeded_ = false;

    QSet<EditItemBase *> origSelItems(selItems_);

    // update selection and hit status
    if (!(hitSelItem || (hitItem && selectMulti))) {
        selItems_.clear();
    } else if (selectMulti && hitSelItem && (selItems_.size() > 1)) {
        selItems_.remove(hitItem);
        hitItem = 0;
    }

    QSet<EditItemBase *> addedItems;
    QSet<EditItemBase *> removedItems;
    QList<QUndoCommand *> undoCommands;

    if (hitItem) { // an item is still considered hit
        selItems_.insert(hitItem); // ensure the hit item is selected (it might already be)

        // send mouse press to the hit item
        bool multiItemOp = false;
        QSet<EditItemBase *> eventItems(selItems_); // operate on current selection

        bool rpn = false;
        hitItem->mousePress(event, rpn, &undoCommands, itemsToCopy, &eventItems, &multiItemOp);
        if (rpn) repaintNeeded_ = true;
        addedItems = eventItems - selItems_;
        removedItems = selItems_ - eventItems;

        if (items_.contains(hitItem)) {
            // the hit item is still there
            if (multiItemOp) {
                // send the mouse press to other selected items
                // (note that these are not allowed to modify item sets, nor requesting items to be copied,
                // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
                foreach (EditItemBase *item, selItems_)
                    if (item != hitItem) {
                        rpn = false;
                        item->mousePress(event, rpn, &undoCommands);
                        if (rpn)
                            repaintNeeded_ = true;
                    }
            }
        } else {
            // the hit item removed itself as a result of the mouse press and it makes no sense
            // to send the mouse press to other items
        }
    }

    const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());
    const bool modifiedItems = !undoCommands.empty();
    if (addedOrRemovedItems || modifiedItems)
        pushCommands(addedItems, removedItems, undoCommands);

    // repaint if necessary
    if (repaintNeeded_ || (selItems_ != origSelItems))
        repaint();
}

// Handles a mouse press event for an item in the process of being completed.
void EditItemManager::incompleteMousePress(QMouseEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    bool completed = false;
    bool aborted = false;
    incompleteItem_->incompleteMousePress(event, rpn, completed, aborted);
    if (completed)
        completeEditing();
    else {
        if (aborted)
            abortEditing();
        if (rpn)
            repaint();
    }
}

void EditItemManager::mouseRelease(QMouseEvent *event)
{
    if (incompleteItem_) {
        incompleteMouseRelease(event);
        return;
    }

    repaintNeeded_ = false;

    QList<QUndoCommand *> undoCommands;

    // send to selected items
    foreach (EditItemBase *item, selItems_)
        item->mouseRelease(event, repaintNeeded_, &undoCommands);

    const bool modifiedItems = !undoCommands.empty();
    if (modifiedItems) {
        // combine the aggregated effect of the operation into one undo command
        undoStack_.beginMacro(undoCommandText(0, 0, undoCommands.size()));
        skipRepaint_ = true; // temporarily prevent redo() calls from repainting
        // push sub-commands representing individual item modifications
        foreach (QUndoCommand *undoCmd, undoCommands) {
            if (EditItemCommand *cmd = static_cast<EditItemCommand *>(undoCmd))
                cmd->eim_ = this;
            undoStack_.push(undoCmd);
        }
        undoStack_.endMacro();
        skipRepaint_ = false;
        repaintNeeded_ = true;
    }

    if (repaintNeeded_)
        repaint();
}

// Handles a mouse release event for an item in the process of being completed.
void EditItemManager::incompleteMouseRelease(QMouseEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    bool completed = false;
    bool aborted = false;
    incompleteItem_->incompleteMouseRelease(event, rpn, completed, aborted);
    if (completed)
        completeEditing();
    else {
        if (aborted)
            abortEditing();
        if (rpn)
            repaint();
    }
}

void EditItemManager::mouseMove(QMouseEvent *event)
{
    if (incompleteItem_) {
        incompleteMouseMove(event);
        return;
    }

    // Check if the event is part of a multi-select operation using a rubberband-rectangle.
    // In that case, the event should only be used to update the selection (and tell items to redraw themselves as
    // approproate), and NOT be passed on through the mouseMove() functions of the selected items ... 2 B DONE!

    repaintNeeded_ = false;

    EditItemBase *origHoverItem = hoverItem_;
    hoverItem_ = 0;
    const bool hover = !event->buttons();
    bool rpn = false;

    if (hover) {
        const QSet<EditItemBase *> hitItems = findHitItems(event->pos());
        if (!hitItems.empty()) {
            // consider only the topmost item that was hit ... 2 B DONE
            // for now, consider only the first that was found
            hoverItem_ = *(hitItems.begin());
            
            // send mouse hover event to the hover item
            hoverItem_->mouseHover(event, rpn);
            if (rpn) repaintNeeded_ = true;
        } else if (origHoverItem) {
            origHoverItem->mouseHover(event, rpn);
            if (rpn) repaintNeeded_ = true;
        }
    } else {
        // send move event to all selected items
        foreach (EditItemBase *item, selItems_) {
            item->mouseMove(event, rpn);
            if (rpn) repaintNeeded_ = true;
        }
    }

    if (repaintNeeded_ || (hoverItem_ != origHoverItem))
        repaint();
}

// Handles a mouse move event for an item in the process of being completed.
void EditItemManager::incompleteMouseMove(QMouseEvent *event)
{
    Q_ASSERT(incompleteItem_);

    const EditItemBase *origHoverItem = hoverItem_;
    hoverItem_ = 0;
    const bool hover = !event->buttons();
    if (hover) {
        bool rpn = false;
        incompleteItem_->incompleteMouseHover(event, rpn);
        if (rpn) repaintNeeded_ = true;
        if (incompleteItem_->hit(event->pos(), false))
            hoverItem_ = incompleteItem_;
    } else {
        bool rpn = false;
        incompleteItem_->incompleteMouseMove(event, rpn);
        if (rpn) repaintNeeded_ = true;
    }

    if (repaintNeeded_ || (hoverItem_ != origHoverItem))
        repaint();
}

void EditItemManager::mouseDoubleClick(QMouseEvent *event)
{
    if (incompleteItem_) {
        incompleteMouseDoubleClick(event);
        return;
    }

    // do nothing for now
}

void EditItemManager::incompleteMouseDoubleClick(QMouseEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    bool completed = false;
    bool aborted = false;
    incompleteItem_->incompleteMouseDoubleClick(event, rpn, completed, aborted);
    if (completed)
        completeEditing();
    else {
        if (aborted)
            abortEditing();
        if (rpn)
            repaint();
    }
}

static EditItemBase *idToItem(const QSet<EditItemBase *> &items, int id)
{
    foreach (EditItemBase *item, items)
        if (id == item->id())
            return item;
    return 0;
}

void EditItemManager::keyPress(QKeyEvent *event)
{
    if (incompleteItem_) {
        incompleteKeyPress(event);
        return;
    }

    repaintNeeded_ = false;

    QSet<int> origSelIds; // IDs of the originally selected items
    foreach (EditItemBase *item, selItems_)
        origSelIds.insert(item->id());

    QSet<EditItemBase *> addedItems;
    QSet<EditItemBase *> removedItems;
    QList<QUndoCommand *> undoCommands;

    // process each of the originally selected items
    foreach (int origSelId, origSelIds) {

        // at this point, the item may or may not exist (it may have been removed in an earlier iteration)

        EditItemBase *origSelItem = idToItem(items_, origSelId);
        if (origSelItem) {
            // it still exists, so pass the event
            QSet<EditItemBase *> eventItems(selItems_); // operate on current selection
            bool rpn = false;
            origSelItem->keyPress(event, rpn, &undoCommands, &eventItems);
            if (rpn) repaintNeeded_ = true;
            addedItems.unite(eventItems - selItems_);
            removedItems.unite(selItems_ - eventItems);
            selItems_.subtract(removedItems);
        }
    }

    const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());
    const bool modifiedItems = !undoCommands.empty();
    if (addedOrRemovedItems || modifiedItems)
        pushCommands(addedItems, removedItems, undoCommands);

    if (repaintNeeded_)
        repaint();
}

// Handles a key press event for an item in the process of being completed.
void EditItemManager::incompleteKeyPress(QKeyEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    bool completed = false;
    bool aborted = false;
    incompleteItem_->incompleteKeyPress(event, rpn, completed, aborted);
    if (completed)
        completeEditing();
    else {
        if (aborted)
            abortEditing();
        if (rpn)
            repaint();
    }
}

void EditItemManager::keyRelease(QKeyEvent *event)
{
    if (incompleteItem_) {
        incompleteKeyRelease(event);
        return;
    }

    repaintNeeded_ = false; // whether at least one item needs to be repainted after processing the event

    // send to selected items
    foreach (EditItemBase *item, selItems_) {
        bool rpn = false;
        item->keyRelease(event, rpn);
        if (rpn)
            repaintNeeded_ = true;
    }

    if (repaintNeeded_)
        repaint();
}

// Handles a key release event for an item in the process of being completed.
void EditItemManager::incompleteKeyRelease(QKeyEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    incompleteItem_->incompleteKeyRelease(event, rpn);
    if (rpn)
        repaint();
}

void EditItemManager::draw()
{
    Q_ASSERT(!items_.contains(incompleteItem_));
    foreach (EditItemBase *item, items_) {
        EditItemBase::DrawModes modes = EditItemBase::Normal;
        if (selItems_.contains(item))
            modes |= EditItemBase::Selected;
        if (item == hoverItem_)
            modes |= EditItemBase::Hovered;
        item->draw(modes, false);
    }
    if (incompleteItem_) // note that only complete items may be selected
        incompleteItem_->draw((incompleteItem_ == hoverItem_) ? EditItemBase::Hovered : EditItemBase::Normal, true);
    emit paintDone();
}

void EditItemManager::undo()
{
    undoStack_.undo();
}

void EditItemManager::redo()
{
    undoStack_.redo();
}

void EditItemManager::repaint()
{
    if (!skipRepaint_)
        emit repaintNeeded();
}

bool EditItemManager::hasIncompleteItem() const
{
    return incompleteItem_ != 0;
}

bool EditItemManager::needsRepaint() const
{
    return repaintNeeded_;
}

bool EditItemManager::canUndo() const
{
    return undoStack_.canUndo();
}

bool EditItemManager::canRedo() const
{
    return undoStack_.canRedo();
}

QSet<EditItemBase *> EditItemManager::findHitItems(const QPoint &pos) const
{
    QSet<EditItemBase *> hitItems;
    foreach (EditItemBase *item, items_)
        if (item->hit(pos, selItems_.contains(item)))
            hitItems.insert(item);
    return hitItems;
}

void EditItemManager::abortEditing()
{
    if (incompleteItem_) {
        delete incompleteItem_; // or leave it to someone else?
        incompleteItem_ = 0;
        emit incompleteEditing(false);
    }
}

void EditItemManager::completeEditing()
{
    if (incompleteItem_) {
        addItem(incompleteItem_); // causes repaint
        incompleteItem_ = 0;
        emit incompleteEditing(false);
    }
}

void EditItemManager::copyObjects()
{
    copiedItems_ = selItems_;
}

void EditItemManager::pasteObjects()
{
    foreach (EditItemBase *item, copiedItems_) {
        addItem(item->copy());
    }
}

void EditItemManager::pushCommands(QSet<EditItemBase *> addedItems,
                                   QSet<EditItemBase *> removedItems,
                                   QList<QUndoCommand *> undoCommands)
{
    const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());

    // combine the aggregated effect of the operation into one undo command
    undoStack_.beginMacro(undoCommandText(addedItems.size(), removedItems.size(), undoCommands.size()));
    skipRepaint_ = true; // temporarily prevent redo() calls from repainting
    if (addedOrRemovedItems) {
        // push sub-command representing aggregated adding/removal of items
        AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
        arCmd->eim_ = this;
        undoStack_.push(arCmd);
    }
    // push sub-commands representing individual item modifications
    foreach (QUndoCommand *undoCmd, undoCommands) {
        if (EditItemCommand *cmd = static_cast<EditItemCommand *>(undoCmd))
            cmd->eim_ = this;
        undoStack_.push(undoCmd);
    }
    if (!undoCommands.empty())
        repaintNeeded_ = true; // assume that any item modification requires a repaint ### BUT ALWAYS SET BELOW!
    undoStack_.endMacro();
    skipRepaint_ = false;
    repaintNeeded_ = true; // ###
}


EditItemCommand::EditItemCommand(const QString &text, QUndoCommand *parent)
    : QUndoCommand(text, parent)
{}

AddOrRemoveItemsCommand::AddOrRemoveItemsCommand(
    const QSet<EditItemBase *> &addedItems, const QSet<EditItemBase *> &removedItems)
    : EditItemCommand(undoCommandText(addedItems.size(), removedItems.size(), 0))
    , addedItems_(addedItems)
    , removedItems_(removedItems)
{}

void AddOrRemoveItemsCommand::undo()
{
    eim_->retrieveItems(removedItems_);
    eim_->storeItems(addedItems_);
    eim_->repaint();
}

void AddOrRemoveItemsCommand::redo()
{
    eim_->retrieveItems(addedItems_);
    eim_->storeItems(removedItems_);
    eim_->repaint();
}

SetGeometryCommand::SetGeometryCommand(
    EditItemBase *item, const QList<QPoint> &oldGeometry, const QList<QPoint> &newGeometry)
    : item_(item)
    , oldGeometry_(oldGeometry)
    , newGeometry_(newGeometry)
{}

void SetGeometryCommand::undo()
{
    // Store the new points as geographic coordinates for later redo if necessary.
    if (newLatLonPoints_.isEmpty())
        newLatLonPoints_ = eim_->PhysToGeo(newGeometry_);

    // Retrieve the old points, if present, and convert them to screen coordinates.
    if (!oldLatLonPoints_.isEmpty())
        oldGeometry_ = eim_->GeoToPhys(oldLatLonPoints_);

    item_->setPoints(oldGeometry_);
    item_->repaint();
}

void SetGeometryCommand::redo()
{
    // Store the old points as geographic coordinates for later undo if necessary.
    if (oldLatLonPoints_.isEmpty())
        oldLatLonPoints_ = eim_->PhysToGeo(oldGeometry_);

    // Retrieve the new points, if present, and convert them to screen coordinates.
    if (!newLatLonPoints_.isEmpty())
        newGeometry_ = eim_->GeoToPhys(newLatLonPoints_);

    item_->setPoints(newGeometry_);
    item_->repaint();
}
