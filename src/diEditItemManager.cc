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

#include <fstream>
#include <iostream>
#define MILOGGER_CATEGORY "diana.EditItemManager"
#include <miLogger/miLogging.h>

#include <QtGui>

#include <diEditItemManager.h>
#include <diPlotModule.h>
#include <EditItems/editcomposite.h>
#include <EditItems/edititembase.h>
#include <EditItems/editpolyline.h>
#include <EditItems/editsymbol.h>
#include <EditItems/edittext.h>
#include <EditItems/kml.h>
#include <EditItems/properties.h>
#include <EditItems/style.h>
#include "paint_select2.xpm"
#include "paint_create_polyline.xpm"
#include "paint_create_symbol.xpm"

#define PLOTM PlotModule::instance()

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

EditItemManager *EditItemManager::self = 0;

EditItemManager::EditItemManager()
    : hoverItem_(0)
    , incompleteItem_(0)
    , repaintNeeded_(false)
    , skipRepaint_(false)
    , undoView_(0)
{
    self = this;

    connect(this, SIGNAL(itemAdded(DrawingItemBase *)), SLOT(initNewItem(DrawingItemBase *)));
    connect(this, SIGNAL(selectionChanged()), SLOT(handleSelectionChange()));

    connect(&undoStack_, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
    connect(&undoStack_, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
    connect(&undoStack_, SIGNAL(indexChanged(int)), this, SLOT(repaint()));

    cutAction = new QAction(tr("Cut"), this);
    cutAction->setShortcut(tr("Ctrl+X"));
    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);
    pasteAction = new QAction(tr("&Paste"), this);
    pasteAction->setShortcut(QKeySequence::Paste);
    editPropertiesAction = new QAction(tr("Edit P&roperties..."), this);
    editPropertiesAction->setShortcut(tr("Ctrl+R"));
    editStyleAction = new QAction(tr("Edit Style..."), this);
    //editStyleAction->setShortcut(tr("Ctrl+Y")); // ### already in use?
    loadAction = new QAction(tr("&Load..."), this);
    loadAction->setShortcut(tr("Ctrl+L"));
    saveAction = new QAction(tr("&Save..."), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    undoAction = undoStack_.createUndoAction(this);
    redoAction = undoStack_.createRedoAction(this);

    selectAction = new QAction(QPixmap(paint_select2_xpm), tr("&Select"), this);
    //selectAction->setShortcut(tr("Ctrl+???"));
    selectAction->setCheckable(true);

    createPolyLineAction = new QAction(QPixmap(paint_create_polyline_xpm), tr("Create &Polyline"), this);
    //createPolyLineAction->setShortcut(tr("Ctrl+???"));
    createPolyLineAction->setCheckable(true);

    createSymbolAction = new QAction(QPixmap(paint_create_symbol_xpm), tr("Create &Symbol"), this);
    //createSymbolAction->setShortcut(tr("Ctrl+???"));
    createSymbolAction->setCheckable(true);

    createTextAction = new QAction(tr("Text"), this);
    createCompositeAction = new QAction(tr("Composite"), this);

    connect(cutAction, SIGNAL(triggered()), SLOT(cutSelectedItems()));
    connect(copyAction, SIGNAL(triggered()), SLOT(copySelectedItems()));
    connect(editPropertiesAction, SIGNAL(triggered()), SLOT(editProperties()));
    connect(editStyleAction, SIGNAL(triggered()), SLOT(editStyle()));
    connect(pasteAction, SIGNAL(triggered()), SLOT(pasteItems()));
    connect(loadAction, SIGNAL(triggered()), SLOT(loadItemsFromFile()));
    connect(saveAction, SIGNAL(triggered()), SLOT(saveItemsToFile()));
    connect(selectAction, SIGNAL(triggered()), SLOT(setSelectMode()));
    connect(createPolyLineAction, SIGNAL(triggered()), SLOT(setCreatePolyLineMode()));
    connect(createSymbolAction, SIGNAL(triggered()), SLOT(setCreateSymbolMode()));
    connect(createTextAction, SIGNAL(triggered()), SLOT(setCreateTextMode()));
    connect(createCompositeAction, SIGNAL(triggered()), SLOT(setCreateCompositeMode()));

    connect(EditItems::Layers::instance(), SIGNAL(updated()), SLOT(repaint()));

    setSelectMode();
}

EditItemManager::~EditItemManager()
{
}

EditItemManager *EditItemManager::instance()
{
  if (!EditItemManager::self)
    EditItemManager::self = new EditItemManager();

  return EditItemManager::self;
}

/**
 * Returns true if working or finished products are available.
 */
bool EditItemManager::isEnabled() const
{
  return hasWorking() | (DrawingManager::isEnabled() && CurrentLayer && !CurrentLayer->items().isEmpty());
}

bool EditItemManager::hasWorking() const
{
  return CurrentLayer && working;
}

void EditItemManager::setWorking(bool enable)
{
  working = enable;

  // open layer dialog and toolbar ... 2 B DONE!
}

QUndoView *EditItemManager::getUndoView()
{
    if (!undoView_)
        undoView_ = new QUndoView(&undoStack_);

    return undoView_;
}

// Adds an item to the scene. \a incomplete indicates whether the item is in the process of being manually placed.
void EditItemManager::addItem(EditItemBase *item, bool incomplete, bool skipRepaint)
{
    addItem(Drawing(item), incomplete, skipRepaint);
}

void EditItemManager::addItem(DrawingItemBase *item, bool incomplete, bool skipRepaint)
{
    if (incomplete) {
        // set this item as the incomplete item
        if (incompleteItem_) {
            // issue warning?
        }
        incompleteItem_ = Editing(item);
        emit incompleteEditing(true);
    } else {
        // create undo command
        QSet<DrawingItemBase *> addedItems;
        addedItems.insert(Drawing(item));
        QSet<DrawingItemBase *> removedItems;
        AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
        undoStack_.push(arCmd);
    }

//    qDebug() << "   ###### addItem()" << item << ", incomplete:" << incomplete << ", item infos:";
//    foreach (DrawingItemBase *item, CurrentLayer->items()) // ### handle layers ... TBD
//        qDebug() << "   ###### - " << item->infoString().toLatin1().data();

    if (!skipRepaint)
      repaint();
}

DrawingItemBase *EditItemManager::createItemFromVarMap(const QVariantMap &vmap, QString *error)
{
    return createItemFromVarMap_<DrawingItemBase, EditItem_PolyLine::PolyLine, EditItem_Symbol::Symbol,
                                 EditItem_Text::Text, EditItem_Composite::Composite>(vmap, error);
}

void EditItemManager::addItem_(DrawingItemBase *item)
{
    DrawingManager::addItem_(item);
    if (false) selectItem(item); // for now, don't pre-select new items
    emit itemAdded(item);
}

void EditItemManager::removeItem(DrawingItemBase *item)
{
    // create undo command
    QSet<DrawingItemBase *> addedItems;
    QSet<DrawingItemBase *> removedItems;
    removedItems.insert(Drawing(item));
    AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
    undoStack_.push(arCmd);
}

void EditItemManager::removeItem_(DrawingItemBase *item)
{
    DrawingManager::removeItem_(item);
    if (Drawing(hoverItem_) == item)
      hoverItem_ = 0;
    deselectItem(item);
    emit itemRemoved(item);
}

void EditItemManager::initNewItem(DrawingItemBase *item)
{
/*
  // Use the current time for the new item.
  miutil::miTime time;
  PLOTM->getPlotTime(time);

  QVariantMap p = item->propertiesRef();
  if (!p.contains("time"))
    p["time"] = QDateTime::fromString(QString::fromStdString(time.isoTime()), "yyyy-MM-dd hh:mm:ss");

  item->setProperties(p);
*/
  // Let other components know about any changes to item times.
  emit timesUpdated();
}

void EditItemManager::storeItems(const QSet<DrawingItemBase *> &items)
{
    foreach (DrawingItemBase *item, items) {
        // Convert the item's screen coordinates to geographic coordinates.
        item->setLatLonPoints(getLatLonPoints(item));
        removeItem_(item);
    }
}

void EditItemManager::retrieveItems(const QSet<DrawingItemBase *> &items)
{
    foreach (DrawingItemBase *item, items) {
        // The items stored on the undo stack have been given geographic
        // coordinates, so we use those to obtain screen coordinates.
        if (!item->getLatLonPoints().isEmpty())
            setFromLatLonPoints(item, item->getLatLonPoints());
        addItem_(item);
    }
}

void EditItemManager::reset()
{
    undoStack_.clear();
}

QUndoStack * EditItemManager::undoStack()
{
    return &undoStack_;
}

QSet<DrawingItemBase *> EditItemManager::getSelectedItems() const
{
  if (!CurrentLayer)
    return QSet<DrawingItemBase *>();
  else
    return CurrentLayer->selectedItems();
}

void EditItemManager::mousePress(QMouseEvent *event)
{
    if (hasIncompleteItem()) {
        incompleteMousePress(event);
        return;
    }

    QSet<DrawingItemBase *> selItems = getSelectedItems();
    const QSet<DrawingItemBase *> origSelItems(selItems);

    const QSet<DrawingItemBase *> hitItems = findHitItems(event->pos());
    DrawingItemBase *hitItem = // consider only this item to be hit
        hitItems.empty()
        ? 0
        : *(hitItems.begin()); // for now; eventually use the one with higher z-value etc. ... 2 B DONE
    const bool hitSelItem = selItems.contains(hitItem); // whether an already selected item was hit
    const bool selectMulti = event->modifiers() & Qt::ControlModifier;

    repaintNeeded_ = false;


    // update selection and hit status
    if (!(hitSelItem || (hitItem && selectMulti))) {
        deselectAllItems();
    } else if (selectMulti && hitSelItem && (selItems.size() > 1)) {
        deselectItem(hitItem);
        hitItem = 0;
    }

    QSet<DrawingItemBase *> addedItems;
    QSet<DrawingItemBase *> removedItems;
    QList<QUndoCommand *> undoCommands;

    if (hitItem) { // an item is still considered hit
        selectItem(hitItem); // ensure the hit item is selected (it might already be)

        // send mouse press to the hit item
        bool multiItemOp = false;
        QSet<DrawingItemBase *> eventItems(CurrentLayer->items());

        bool rpn = false;
        Editing(hitItem)->mousePress(event, rpn, &undoCommands, &eventItems, &selItems, &multiItemOp);
        if (rpn) repaintNeeded_ = true;
        addedItems = eventItems - CurrentLayer->items();
        removedItems = CurrentLayer->items() - eventItems;

        if (CurrentLayer->items().contains(hitItem)) {
            // the hit item is still there
            if (multiItemOp) {
                // send the mouse press to other selected items
                // (note that these are not allowed to modify item sets, nor requesting items to be copied,
                // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
                foreach (DrawingItemBase *item, selItems)
                    if (item != hitItem) {
                        rpn = false;
                        Editing(item)->mousePress(event, rpn, &undoCommands);
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

    if (getSelectedItems() != origSelItems) {
        emit selectionChanged();
        repaintNeeded_ = true;
    }
}

// Handles a mouse press event for an item in the process of being completed.
void EditItemManager::incompleteMousePress(QMouseEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    bool completed = false;
    bool aborted = false;
    incompleteItem_->incompleteMousePress(event, rpn, completed, aborted);
    // Record geographic coordinates for the item as they are added to it.
    Drawing(incompleteItem_)->setLatLonPoints(getLatLonPoints(Drawing(incompleteItem_)));
    if (completed)
        completeEditing();
    else {
        if (aborted)
            abortEditing();
        if (rpn)
            repaintNeeded_ = true;

    }
}

void EditItemManager::mouseRelease(QMouseEvent *event)
{
    if (hasIncompleteItem()) {
        incompleteMouseRelease(event);
        return;
    }

    repaintNeeded_ = false;

    QList<QUndoCommand *> undoCommands;

    // send to selected items
    foreach (DrawingItemBase *item, CurrentLayer->selectedItems())
        Editing(item)->mouseRelease(event, repaintNeeded_, &undoCommands);

    const bool modifiedItems = !undoCommands.empty();
    if (modifiedItems) {
        skipRepaint_ = true; // temporarily prevent redo() calls from repainting
        // push sub-commands representing individual item modifications
        foreach (QUndoCommand *undoCmd, undoCommands)
            undoStack_.push(undoCmd);

        skipRepaint_ = false;
        repaintNeeded_ = true;
    }
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
            repaintNeeded_ = true;
    }
}

void EditItemManager::mouseMove(QMouseEvent *event)
{
    if (hasIncompleteItem()) {
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
        const QSet<DrawingItemBase *> hitItems = findHitItems(event->pos());
        if (!hitItems.empty()) {
            // consider only the topmost item that was hit ... 2 B DONE
            // for now, consider only the first that was found
            hoverItem_ = Editing(*(hitItems.begin()));

            // send mouse hover event to the hover item
            hoverItem_->mouseHover(event, rpn);
            if (rpn) repaintNeeded_ = true;
        } else if (origHoverItem) {
            Editing(origHoverItem)->mouseHover(event, rpn);
            if (rpn) repaintNeeded_ = true;
        }
    } else {
        // send move event to all selected items
        foreach (DrawingItemBase *item, CurrentLayer->selectedItems()) {
            Editing(item)->mouseMove(event, rpn);
            item->setLatLonPoints(getLatLonPoints(item));
            if (rpn) repaintNeeded_ = true;
        }
    }

    if (hoverItem_ != origHoverItem)
        repaintNeeded_ = true;
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
        Drawing(incompleteItem_)->setLatLonPoints(getLatLonPoints(Drawing(incompleteItem_)));
        if (rpn) repaintNeeded_ = true;
        if (incompleteItem_->hit(event->pos(), false))
            hoverItem_ = incompleteItem_;
    } else {
        bool rpn = false;
        incompleteItem_->incompleteMouseMove(event, rpn);
        if (rpn) repaintNeeded_ = true;
    }

    if (hoverItem_ != origHoverItem)
        repaintNeeded_ = true;
}

void EditItemManager::mouseDoubleClick(QMouseEvent *event)
{
    if (hasIncompleteItem()) {
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
            repaintNeeded_ = true;
    }
}

static EditItemBase *idToItem(const QSet<DrawingItemBase *> &items, int id)
{
    foreach (DrawingItemBase *item, items) {
        if (id == item->id())
            return Editing(item);
    }
    return 0;
}

void EditItemManager::keyPress(QKeyEvent *event)
{
    if (hasIncompleteItem()) {
        incompleteKeyPress(event);
        return;
    }

    repaintNeeded_ = false;
    QSet<DrawingItemBase *> items = CurrentLayer->items();
    QSet<DrawingItemBase *> selItems = CurrentLayer->selectedItems();

    const QSet<DrawingItemBase *> origSelItems(selItems);
    QSet<int> origSelIds;
    foreach (DrawingItemBase *item, origSelItems)
        origSelIds.insert(item->id());

    QSet<DrawingItemBase *> addedItems;
    QSet<DrawingItemBase *> removedItems;
    QList<QUndoCommand *> undoCommands;

    // process each of the originally selected items
    foreach (int origSelId, origSelIds) {

        // at this point, the item may or may not exist (it may have been removed in an earlier iteration)

        EditItemBase *origSelItem = idToItem(items, origSelId);
        if (origSelItem) {
            // it still exists, so pass the event
            QSet<DrawingItemBase *> eventItems(items);
            bool rpn = false;
            origSelItem->keyPress(event, rpn, &undoCommands, &eventItems, &selItems);
            if (rpn) repaintNeeded_ = true;
            addedItems.unite(eventItems - items);
            removedItems.unite(items - eventItems);
            selItems.subtract(removedItems);
        }
    }

    const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());
    const bool modifiedItems = !undoCommands.empty();
    if (addedOrRemovedItems || modifiedItems)
        pushCommands(addedItems, removedItems, undoCommands);

    if (selItems != origSelItems) {
        emit selectionChanged();
        repaintNeeded_ = true;
    }
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
            repaintNeeded_ = true;
    }
}

void EditItemManager::keyRelease(QKeyEvent *event)
{
    if (hasIncompleteItem()) {
        incompleteKeyRelease(event);
        return;
    }

    repaintNeeded_ = false; // whether at least one item needs to be repainted after processing the event

    // send to selected items
    foreach (DrawingItemBase *item, CurrentLayer->selectedItems()) {
        bool rpn = false;
        Editing(item)->keyRelease(event, rpn);
        if (rpn)
            repaintNeeded_ = true;
    }
}

// Handles a key release event for an item in the process of being completed.
void EditItemManager::incompleteKeyRelease(QKeyEvent *event)
{
    Q_ASSERT(incompleteItem_);
    bool rpn = false;
    incompleteItem_->incompleteKeyRelease(event, rpn);
    if (rpn)
        repaintNeeded_ = true;
}

void EditItemManager::plot(bool under, bool over)
{
    if (!over)
        return;

    if (hasWorking()) {
        // Apply a transformation so that the items can be plotted with screen coordinates
        // while everything else is plotted in map coordinates.
        glPushMatrix();
        plotRect = PLOTM->getPlotSize();
        int w, h;
        PLOTM->getPlotWindow(w, h);
        glTranslatef(editRect.x1, editRect.y1, 0.0);
        glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);

        EditItems::Layers *layers = EditItems::Layers::instance();
        for (int i = 0; i < layers->size(); ++i) {

            EditItems::Layer *layer = layers->at(i);
            if (layer->isVisible()) {
                QList<DrawingItemBase *> items = layer->items().values();
                qStableSort(items.begin(), items.end(), DrawingManager::itemCompare());

                foreach (DrawingItemBase *item, items) {
                    EditItemBase::DrawModes modes = EditItemBase::Normal;
                    if (CurrentLayer->selectedItems().contains(item))
                        modes |= EditItemBase::Selected;
                    if (item == Drawing(hoverItem_))
                        modes |= EditItemBase::Hovered;
                    if (item->property("visible", true).toBool()) {
                        applyPlotOptions(item);
                        setFromLatLonPoints(item, item->getLatLonPoints());
                        Editing(item)->draw(modes, false, EditItemsStyle::StyleEditor::instance()->isVisible());
                    }
                }
            }
        }
        if (incompleteItem_) { // note that only complete items may be selected
            setFromLatLonPoints(Drawing(incompleteItem_), Drawing(incompleteItem_)->getLatLonPoints());
            incompleteItem_->draw((incompleteItem_ == hoverItem_) ? EditItemBase::Hovered : EditItemBase::Normal, true);
        }
        emit paintDone();

        glPopMatrix();
    } else
        DrawingManager::plot(under, over);
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

QSet<DrawingItemBase *> EditItemManager::findHitItems(const QPointF &pos) const
{
    QSet<DrawingItemBase *> hitItems;
    foreach (DrawingItemBase *item, CurrentLayer->items()) {
        if (item->property("visible", false).toBool() == false)
            continue;
        if (Editing(item)->hit(pos, CurrentLayer->selectedItems().contains(item)))
            hitItems.insert(item);
    }
    return hitItems;
}

void EditItemManager::abortEditing()
{
    if (incompleteItem_) {
        delete incompleteItem_; // or leave it to someone else?
        incompleteItem_ = 0;
        hoverItem_ = 0;
        //setSelectMode(); // restore default mode
        emit incompleteEditing(false);
    }
}

void EditItemManager::completeEditing()
{
    if (incompleteItem_) {
        addItem(incompleteItem_); // causes repaint
        incompleteItem_ = 0;
        //setSelectMode(); // restore default mode
        emit incompleteEditing(false);
    }
}

void EditItemManager::pushCommands(QSet<DrawingItemBase *> addedItems,
                                   QSet<DrawingItemBase *> removedItems,
                                   QList<QUndoCommand *> undoCommands)
{
    const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());

    // combine the aggregated effect of the operation into one undo command
    undoStack_.beginMacro(undoCommandText(addedItems.size(), removedItems.size(), undoCommands.size()));
    skipRepaint_ = true; // temporarily prevent redo() calls from repainting
    if (addedOrRemovedItems) {
        // push sub-command representing aggregated adding/removal of items
        AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
        undoStack_.push(arCmd);
    }
    // push sub-commands representing individual item modifications
    foreach (QUndoCommand *undoCmd, undoCommands)
        undoStack_.push(undoCmd);
    if (!undoCommands.empty())
        repaintNeeded_ = true; // assume that any item modification requires a repaint ### BUT ALWAYS SET BELOW!
    undoStack_.endMacro();
    skipRepaint_ = false;
    repaintNeeded_ = true; // ###
}

void EditItemManager::selectItem(DrawingItemBase *item)
{
  CurrentLayer->selectedItems().insert(Drawing(item));
  emit selectionChanged();
}

void EditItemManager::deselectItem(DrawingItemBase *item)
{
  CurrentLayer->selectedItems().remove(Drawing(item));
  emit selectionChanged();
}

void EditItemManager::deselectAllItems()
{
  CurrentLayer->selectedItems().clear();
  emit selectionChanged();
}

// Action handling

QHash<EditItemManager::Action, QAction*> EditItemManager::actions()
{
  QHash<Action, QAction*> a;
  a[Cut] = cutAction;
  a[Copy] = copyAction;
  a[Paste] = pasteAction;
  a[EditProperties] = editPropertiesAction;
  a[EditStyle] = editStyleAction;
  a[Load] = loadAction;
  a[Save] = saveAction;
  a[Undo] = undoAction;
  a[Redo] = redoAction;
  a[Select] = selectAction;
  a[CreatePolyLine] = createPolyLineAction;
  a[CreateSymbol] = createSymbolAction;
  a[CreateText] = createTextAction;
  a[CreateComposite] = createCompositeAction;
  return a;
}

void EditItemManager::editProperties()
{
  // NOTE: we only support editing properties for one item at a time for now
  Q_ASSERT(getSelectedItems().size() == 1);
  DrawingItemBase *item = *(getSelectedItems().begin());
  if (Properties::PropertiesEditor::instance()->edit(item)) {
    emit itemChanged(item);
    repaint();
  }
}

void EditItemManager::editStyle()
{
  EditItemsStyle::StyleEditor::instance()->edit(getSelectedItems());
}

void EditItemManager::loadItemsFromFile()
{
    // select file
    const QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"), getWorkDir(), tr("KML files (*.kml);; All files (*)"));
    if (fileName.isNull())
        return; // operation cancelled

    loadItems(fileName);
}

bool EditItemManager::loadItems(const QString &fileName)
{
    // parse file and create items
    QString error;
    const QSet<EditItemBase *> items = \
      KML::createFromFile<EditItemBase, EditItem_PolyLine::PolyLine, EditItem_Symbol::Symbol,
                          EditItem_Text::Text, EditItem_Composite::Composite>(fileName, &error);

    if (!error.isEmpty()) {
      QMessageBox::warning(
          0, "Error", QString("failed to create items from file %1: %2")
          .arg(fileName).arg(error));
      return false;
    }
    if (items.isEmpty()) {
      QMessageBox::warning(
          0, "Warning", QString("file contained no items: %1")
          .arg(fileName));
      return false;
    }

    // add items
    int i = 0;
    foreach (EditItemBase *item, items) {
      setFromLatLonPoints(Drawing(item), Drawing(item)->getLatLonPoints()); // convert lat/lon values into screen coordinates
      addItem(item, false, ++i == items.size());
    }

    drawings_.insert(fileName);
    loaded_.insert(fileName);
    updateActionsAndTimes();
    return true;
}

void EditItemManager::saveItemsToFile()
{
    // select file
    const QString fileName = QFileDialog::getSaveFileName(0, tr("Open File"), getWorkDir(), tr("KML files (*.kml)"));
    if (fileName.isNull())
        return; // operation cancelled

    QString error;
    KML::saveToFile(fileName, getItems(), getSelectedItems(), &error);
    if (!error.isEmpty()) {
      QMessageBox::warning(
          0, "Error", QString("failed to save items to file %1: %2")
          .arg(fileName).arg(error));
      return;
    }
}

void EditItemManager::updateActions()
{
    cutAction->setEnabled(getSelectedItems().size() > 0);
    copyAction->setEnabled(getSelectedItems().size() > 0);
    pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
    editPropertiesAction->setEnabled(getSelectedItems().size() == 1);
    editStyleAction->setEnabled(getSelectedItems().size() > 0);
}

void EditItemManager::updateTimes()
{
    // Let other components know about any changes to item times.
    emit timesUpdated();

    // Update the visibility of items based on the current plot time.
    miutil::miTime time;
    PLOTM->getPlotTime(time);
    prepare(time);
}

void EditItemManager::updateActionsAndTimes()
{
  updateActions();
  updateTimes();
}

// Clipboard operations

void EditItemManager::copyItems(const QSet<DrawingItemBase *> &items)
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  QString text;

  text += QString("Number of items: %1\n").arg(items.size());
  QVariantList cbItems;

  foreach (DrawingItemBase *item, items) {
    cbItems.append(Editing(item)->clipboardVarMap());
    text += QString("%1\n").arg(Editing(item)->clipboardPlainText());
  }

  stream << cbItems;

  QMimeData *data = new QMimeData();
  data->setData("application/x-diana-object", bytes);
  data->setData("text/plain", text.toUtf8());

  QApplication::clipboard()->setMimeData(data);
  updateActionsAndTimes();
}

void EditItemManager::copySelectedItems()
{
  copyItems(getSelectedItems());
}

void EditItemManager::cutSelectedItems()
{
  QSet<DrawingItemBase*> items = getSelectedItems();
  copyItems(items);
  foreach (DrawingItemBase* item, items)
    removeItem(item);

  updateActionsAndTimes();
}

void EditItemManager::pasteItems()
{
  const QMimeData *data = QApplication::clipboard()->mimeData();
  if (data->hasFormat("application/x-diana-object")) {

    QByteArray bytes = data->data("application/x-diana-object");
    QDataStream stream(&bytes, QIODevice::ReadOnly);

    QVariantList cbItems;
    stream >> cbItems;

    foreach (QVariant cbItem, cbItems) {
      QString error;
      DrawingItemBase *item = createItemFromVarMap(cbItem.toMap(), &error);
      if (item)
        addItem(item, false);
      else
        QMessageBox::warning(0, "Error", error);
    }
  }

  updateActionsAndTimes();
}

//static void clearCursorStack()
//{
//    while (qApp->overrideCursor())
//        qApp->restoreOverrideCursor();
//}

void EditItemManager::setSelectMode()
{
  mode_ = SelectMode;
  selectAction->setChecked(true);
  //clearCursorStack();
  emit unsetWorkAreaCursor();
}

void EditItemManager::setCreatePolyLineMode()
{
  mode_ = CreatePolyLineMode;
  createPolyLineAction->setChecked(true);
  //clearCursorStack();
  //qApp->setOverrideCursor(Qt::CrossCursor); // FOR NOW
  const QCursor cursor = QPixmap(paint_create_polyline_xpm);
  //qApp->setOverrideCursor(cursor);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateSymbolMode()
{
  mode_ = CreateSymbolMode;
  createSymbolAction->setChecked(true);
  //clearCursorStack();
  //qApp->setOverrideCursor(Qt::PointingHandCursor); // FOR NOW
  const QCursor cursor = QPixmap(paint_create_symbol_xpm);
  //qApp->setOverrideCursor(cursor);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateTextMode()
{
  mode_ = CreateTextMode;
  qApp->setOverrideCursor(Qt::PointingHandCursor); // FOR NOW
}

void EditItemManager::setCreateCompositeMode()
{
  mode_ = CreateCompositeMode;
  qApp->setOverrideCursor(Qt::PointingHandCursor); // FOR NOW
}

void EditItemManager::handleSelectionChange()
{
  updateActions();
}

// Manager API

void EditItemManager::sendMouseEvent(QMouseEvent *event, EventResult &res)
{
  res.savebackground= true;
  res.background= false;
  res.repaint= false;
  //res.newcursor= edit_cursor;
  res.newcursor= keep_it;

  // Do not process the event if there is no current layer.
  if (!CurrentLayer)
    return;

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  PLOTM->getPlotWindow(w, h);
  plotRect = PLOTM->getPlotSize();

  if (getItems().size() == 0)
    editRect = plotRect;

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates. This gives us displaced screen coordinates - these
  // are coordinates relative to the original edit rectangle.
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  // Translate the mouse event by the current displacement of the viewport.
  QMouseEvent me2(event->type(), QPoint(event->x() + dx, event->y() + dy),
                  event->globalPos(), event->button(), event->buttons(), event->modifiers());

  if (event->type() == QEvent::MouseButtonPress) {
    if ((me2.button() == Qt::RightButton) && !hasIncompleteItem()) {
      // open a context menu

      // get actions contributed by a hit item (if any)
      const QSet<DrawingItemBase *> hitItems = findHitItems(me2.pos());
      DrawingItemBase *hitItem = // consider only this item to be hit
          hitItems.empty()
          ? 0
          : *(hitItems.begin()); // for now; eventually use the one with higher z-value etc. ... 2 B DONE
      QList<QAction *> hitItemActions;
      if (hitItem) {
        selectItem(hitItem);
        emit repaintNeeded();
        hitItemActions = Editing(hitItem)->actions(me2.pos());
      }

      // populate the menu
      QMenu contextMenu;
      contextMenu.addAction(cutAction);
      contextMenu.addAction(copyAction);
      contextMenu.addAction(pasteAction);
      pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
      contextMenu.addSeparator();
      contextMenu.addAction(editPropertiesAction);
      editPropertiesAction->setEnabled(getSelectedItems().size() == 1);
      contextMenu.addAction(editStyleAction);
      editStyleAction->setEnabled(getSelectedItems().size() > 0);
      contextMenu.addSeparator();
      contextMenu.addAction(loadAction);
      contextMenu.addAction(saveAction);
      saveAction->setEnabled(getSelectedItems().size() > 0);
      if (!hitItemActions.isEmpty()) {
        contextMenu.addSeparator();
        foreach (QAction *action, hitItemActions)
          contextMenu.addAction(action);
      }

      // execute the menu (assuming all its actions are connected to slots)
      if (!contextMenu.isEmpty())
        contextMenu.exec(me2.globalPos());

    } else {

      // create a new item if necessary
      if ((me2.button() == Qt::LeftButton) && !hasIncompleteItem()) {
        if (mode_ == CreatePolyLineMode)
          addItem(Drawing(new EditItem_PolyLine::PolyLine()), true);
        else if (mode_ == CreateSymbolMode)
          addItem(Drawing(new EditItem_Symbol::Symbol()), true);
        else if (mode_ == CreateCompositeMode)
          addItem(Drawing(new EditItem_Composite::Composite()), true);
        else if (mode_ == CreateTextMode) {
          addItem(Drawing(new EditItem_Text::Text()), true);
        }
      }

      // process the event further (delegating it to relevant items etc.)
      mousePress(&me2);
    }
    event->setAccepted(true);
  } else if (event->type() == QEvent::MouseMove) {
    mouseMove(&me2);
    event->setAccepted(false); // ### WHAT DOES THIS MEAN? WHY NOT SET TO true IN OTHER CASES???
  }

  else if (event->type() == QEvent::MouseButtonRelease) {
    mouseRelease(&me2);
    event->setAccepted(true);
  }

  else if (event->type() == QEvent::MouseButtonDblClick) {
    mouseDoubleClick(&me2);
    event->setAccepted(true);
  }

  res.repaint = needsRepaint();
  res.action = canUndo() ? objects_changed : no_action;

  if (event->type() != QEvent::MouseMove)
    updateActionsAndTimes();
}

void EditItemManager::sendKeyboardEvent(QKeyEvent *event, EventResult &res)
{
  event->accept();
  res.savebackground= true;
  res.background= false;
  res.repaint= false;

  // Do not process the event if there is no current layer.
  if (!CurrentLayer)
    return;

  if (event->type() == QEvent::KeyPress) {
    if (cutAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      cutSelectedItems();
    else if (copyAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      copySelectedItems();
    else if (pasteAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      pasteItems();
    else if (editPropertiesAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      editProperties();
    else if (loadAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      loadItemsFromFile();
    else if (saveAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch)
      saveItemsToFile();
    else
      event->ignore();
  }

  res.repaint = true;
  res.background = true;
  if (event->isAccepted())
    return;

  keyPress(event);

  updateActionsAndTimes();
}

// Command classes

EditItemCommand::EditItemCommand(const QString &text, QUndoCommand *parent)
    : QUndoCommand(text, parent)
{}

AddOrRemoveItemsCommand::AddOrRemoveItemsCommand(
    const QSet<DrawingItemBase *> &addedItems, const QSet<DrawingItemBase *> &removedItems)
    : EditItemCommand(undoCommandText(addedItems.size(), removedItems.size(), 0))
    , addedItems_(addedItems)
    , removedItems_(removedItems)
{}

void AddOrRemoveItemsCommand::undo()
{
    EditItemManager::instance()->retrieveItems(removedItems_);
    EditItemManager::instance()->storeItems(addedItems_);
    EditItemManager::instance()->repaint();
}

void AddOrRemoveItemsCommand::redo()
{
    EditItemManager::instance()->retrieveItems(addedItems_);
    EditItemManager::instance()->storeItems(removedItems_);
    EditItemManager::instance()->repaint();
}

SetGeometryCommand::SetGeometryCommand(
    EditItemBase *item, const QList<QPointF> &oldGeometry, const QList<QPointF> &newGeometry)
    : item_(item)
{
    oldLatLonPoints_ = EditItemManager::instance()->PhysToGeo(oldGeometry);
    newLatLonPoints_ = EditItemManager::instance()->PhysToGeo(newGeometry);
    setText(EditItemManager::tr("Item moved"));
}

EditItemBase *SetGeometryCommand::item() const
{
    return item_;
}

QList<QPointF> SetGeometryCommand::newLatLonPoints() const
{
    return newLatLonPoints_;
}

void SetGeometryCommand::undo()
{
    Drawing(item_)->setLatLonPoints(oldLatLonPoints_);
}

void SetGeometryCommand::redo()
{
    Drawing(item_)->setLatLonPoints(newLatLonPoints_);
}

int SetGeometryCommand::id() const
{
    return 0x53657447;  // "SetG"
}

bool SetGeometryCommand::mergeWith(const QUndoCommand *command)
{
    if (command->id() != id())
        return false;

    const SetGeometryCommand *setgeo = static_cast<const SetGeometryCommand *>(command);

    if (setgeo->item() != item_)
        return false;

    newLatLonPoints_ = setgeo->newLatLonPoints();
    return true;
}
