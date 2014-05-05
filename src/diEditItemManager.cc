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

#include <QtGui> // ### include only relevant headers ... TBD

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
#include <EditItems/layermanager.h>
#include <qtMainWindow.h>
#include "paint_select2.xpm"
#include "paint_create_polyline.xpm"
#include "paint_create_symbol.xpm"
#include "paint_create_text.xpm"

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
    : repaintNeeded_(false)
    , skipRepaint_(false)
    , undoView_(0)
{
    self = this;

    connect(this, SIGNAL(itemAdded(DrawingItemBase *)), SLOT(initNewItem(DrawingItemBase *)));
    connect(this, SIGNAL(selectionChanged()), SLOT(handleSelectionChange()));
    connect(this, SIGNAL(incompleteEditing(bool)), SLOT(startStopEditing(bool)));

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

    createTextAction = new QAction(QPixmap(paint_create_text_xpm), tr("Text"), this);
    createTextAction->setCheckable(true);

    createCompositeAction = new QAction(tr("Composite"), this);
    createCompositeAction->setCheckable(true);

    connect(cutAction, SIGNAL(triggered()), SLOT(cutSelectedItems()));
    connect(copyAction, SIGNAL(triggered()), SLOT(copySelectedItems()));
    connect(editPropertiesAction, SIGNAL(triggered()), SLOT(editProperties()));
    connect(editStyleAction, SIGNAL(triggered()), SLOT(editStyle()));
    connect(pasteAction, SIGNAL(triggered()), SLOT(pasteItems()));
    connect(selectAction, SIGNAL(triggered()), SLOT(setSelectMode()));
    connect(createPolyLineAction, SIGNAL(triggered()), SLOT(setCreatePolyLineMode()));
    connect(createSymbolAction, SIGNAL(triggered()), SLOT(setCreateSymbolMode()));
    connect(createTextAction, SIGNAL(triggered()), SLOT(setCreateTextMode()));
    connect(createCompositeAction, SIGNAL(triggered()), SLOT(setCreateCompositeMode()));

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
  return isEditing() | DrawingManager::isEnabled();
}

QUndoView *EditItemManager::getUndoView()
{
    if (!undoView_)
        undoView_ = new QUndoView(&undoStack_);

    return undoView_;
}

bool EditItemManager::loadItems(const QString &fileName)
{
  // parse file and create item layers
  QString error;
  QList<QSharedPointer<EditItems::Layer> > layers = \
      KML::createFromFile<EditItemBase, EditItem_PolyLine::PolyLine, EditItem_Symbol::Symbol,
      EditItem_Text::Text, EditItem_Composite::Composite>(fileName, &error);

  return finishLoadingItems(fileName, error, layers);
}

// Adds an item to the scene. \a incomplete indicates whether the item is in the process of being manually placed.
void EditItemManager::addItem(const QSharedPointer<DrawingItemBase> &item, bool incomplete, bool skipRepaint)
{
  if (incomplete) {
    // set this item as the incomplete item
    if (hasIncompleteItem()) {
      // issue warning?
    }
    // Start editing the item.
    editItem(item);

  } else {
    // create undo command
    QSet<QSharedPointer<DrawingItemBase> > addedItems;
    addedItems.insert(item);
    QSet<QSharedPointer<DrawingItemBase> > removedItems;
    AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
    undoStack_.push(arCmd);
  }

  //    qDebug() << "   ###### addItem()" << item << ", incomplete:" << incomplete << ", item infos:";
  //    foreach (DrawingItemBase *item, CurrEditLayer->items())
  //        qDebug() << "   ###### - " << item->infoString().toLatin1().data();

  if (!skipRepaint)
    repaint();
}

QSharedPointer<DrawingItemBase> EditItemManager::createItemFromVarMap(const QVariantMap &vmap, QString *error)
{
  return QSharedPointer<DrawingItemBase>(
        createItemFromVarMap_<DrawingItemBase, EditItem_PolyLine::PolyLine, EditItem_Symbol::Symbol,
        EditItem_Text::Text, EditItem_Composite::Composite>(vmap, error));
}

void EditItemManager::addItem_(const QSharedPointer<DrawingItemBase> &item)
{
    DrawingManager::addItem_(item);
    if (false) selectItem(item); // for now, don't pre-select new items
    emit itemAdded(item.data());
}

void EditItemManager::editItem(const QSharedPointer<DrawingItemBase> &item)
{
    incompleteItem_ = item;
    emit incompleteEditing(true);
}

void EditItemManager::editItem(DrawingItemBase *item)
{
    incompleteItem_ = QSharedPointer<DrawingItemBase>(item);
    emit incompleteEditing(true);
}

void EditItemManager::removeItem(const QSharedPointer<DrawingItemBase> &item)
{
    // create undo command
    QSet<QSharedPointer<DrawingItemBase> > addedItems;
    QSet<QSharedPointer<DrawingItemBase> > removedItems;
    removedItems.insert(item);
    AddOrRemoveItemsCommand *arCmd = new AddOrRemoveItemsCommand(addedItems, removedItems);
    undoStack_.push(arCmd);
}

void EditItemManager::removeItem_(const QSharedPointer<DrawingItemBase> &item)
{
    DrawingManager::removeItem_(item);
    if (hoverItem_ == item)
      hoverItem_.clear();
    deselectItem(item);
    emit itemRemoved(item.data()); // ### anybody connected to this?
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

void EditItemManager::storeItems(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    // Convert the item's screen coordinates to geographic coordinates.
    item->setLatLonPoints(getLatLonPoints(*item));
    removeItem_(item);
  }
}

void EditItemManager::retrieveItems(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    // The items stored on the undo stack have been given geographic
    // coordinates, so we use those to obtain screen coordinates.
    if (!item->getLatLonPoints().isEmpty())
      setFromLatLonPoints(*item, item->getLatLonPoints());
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

QSet<QSharedPointer<DrawingItemBase> > EditItemManager::getSelectedItems() const
{
  if (!CurrEditLayer)
    return QSet<QSharedPointer<DrawingItemBase> >();
  else
    return CurrEditLayer->selectedItemSet();
}

void EditItemManager::mousePress(QMouseEvent *event)
{
  if (!CurrEditLayer)
    return;

  if (hasIncompleteItem()) {
    incompleteMousePress(event);
    return;
  }

  QSet<QSharedPointer<DrawingItemBase> > selItems = getSelectedItems();
  const QSet<QSharedPointer<DrawingItemBase> > origSelItems(selItems);

  const QSet<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(event->pos());
  QSharedPointer<DrawingItemBase> hitItem; // consider only this item to be hit
  if (!hitItems.empty())
    hitItem = *(hitItems.begin()); // for now; eventually use the one with higher z-value etc. ... TBD

  const bool hitSelItem = selItems.contains(hitItem); // whether an already selected item was hit
  const bool selectMulti = event->modifiers() & Qt::ControlModifier;

  repaintNeeded_ = false;


  // update selection and hit status
  if (!(hitSelItem || ((!hitItem.isNull()) && selectMulti))) {
    deselectAllItems();
  } else if (selectMulti && hitSelItem && (selItems.size() > 1)) {
    deselectItem(hitItem);
    hitItem.clear();
  }

  QSet<QSharedPointer<DrawingItemBase> > addedItems;
  QSet<QSharedPointer<DrawingItemBase> > removedItems;
  QList<QUndoCommand *> undoCommands;

  if (!hitItem.isNull()) { // an item is still considered hit
    selectItem(hitItem); // ensure the hit item is selected (it might already be)

    // send mouse press to the hit item
    bool multiItemOp = false;
    QSet<QSharedPointer<DrawingItemBase> > eventItems(CurrEditLayer->itemSet());

    bool rpn = false;
    Editing(hitItem.data())->mousePress(event, rpn, &undoCommands, &eventItems, &selItems, &multiItemOp);
    if (rpn) repaintNeeded_ = true;
    addedItems = eventItems - CurrEditLayer->itemSet();
    removedItems = CurrEditLayer->itemSet() - eventItems;

    if (CurrEditLayer->containsItem(hitItem)) {
      // the hit item is still there
      if (multiItemOp) {
        // send the mouse press to other selected items
        // (note that these are not allowed to modify item sets, nor requesting items to be copied,
        // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
        foreach (const QSharedPointer<DrawingItemBase> item, selItems)
          if (item != hitItem) {
            rpn = false;
            Editing(item.data())->mousePress(event, rpn, &undoCommands);
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
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMousePress(event, rpn, completed, aborted);
  // Record geographic coordinates for the item as they are added to it.
  incompleteItem_->setLatLonPoints(getLatLonPoints(*incompleteItem_));
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
  if (!CurrEditLayer)
    return;

  if (hasIncompleteItem()) {
    incompleteMouseRelease(event);
    return;
  }

  repaintNeeded_ = false;

  QList<QUndoCommand *> undoCommands;

  // send to selected items
  for (int i = 0; i < CurrEditLayer->selectedItemCount(); ++i)
    Editing(CurrEditLayer->selectedItemRef(i).data())->mouseRelease(event, repaintNeeded_, &undoCommands);

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
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMouseRelease(event, rpn, completed, aborted);
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
  if (!CurrEditLayer)
    return;

  if (hasIncompleteItem()) {
    incompleteMouseMove(event);
    return;
  }

  // Check if the event is part of a multi-select operation using a rubberband-rectangle.
  // In that case, the event should only be used to update the selection (and tell items to redraw themselves as
  // approproate), and NOT be passed on through the mouseMove() functions of the selected items ... 2 B DONE!

  repaintNeeded_ = false;

  QSharedPointer<DrawingItemBase> origHoverItem = hoverItem_;
  hoverItem_.clear();
  const bool hover = !event->buttons();
  bool rpn = false;

  if (hover) {
    const QSet<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(event->pos());
    if (!hitItems.empty()) {
      // consider only the topmost item that was hit ... 2 B DONE
      // for now, consider only the first that was found
      hoverItem_ = *(hitItems.begin());

      // send mouse hover event to the hover item
      Editing(hoverItem_.data())->mouseHover(event, rpn);
      if (rpn) repaintNeeded_ = true;
    } else if (!origHoverItem.isNull()) {
      Editing(origHoverItem.data())->mouseHover(event, rpn);
      if (rpn) repaintNeeded_ = true;
    }
  } else {
    // send move event to all selected items
    for (int i = 0; i < CurrEditLayer->selectedItemCount(); ++i) {
      Editing(CurrEditLayer->selectedItemRef(i).data())->mouseMove(event, rpn);
      CurrEditLayer->selectedItemRef(i)->setLatLonPoints(getLatLonPoints(*(CurrEditLayer->selectedItem(i))));
      if (rpn) repaintNeeded_ = true;
    }
  }

  if (hoverItem_ != origHoverItem)
    repaintNeeded_ = true;
}

// Handles a mouse move event for an item in the process of being completed.
void EditItemManager::incompleteMouseMove(QMouseEvent *event)
{
  Q_ASSERT(hasIncompleteItem());

  const QSharedPointer<DrawingItemBase> origHoverItem = hoverItem_;
  hoverItem_.clear();
  const bool hover = !event->buttons();
  if (hover) {
    bool rpn = false;
    Editing(incompleteItem_.data())->incompleteMouseHover(event, rpn);
    incompleteItem_->setLatLonPoints(getLatLonPoints(*incompleteItem_));
    if (rpn) repaintNeeded_ = true;
    if (Editing(incompleteItem_.data())->hit(event->pos(), false))
      hoverItem_ = incompleteItem_;
  } else {
    bool rpn = false;
    Editing(incompleteItem_.data())->incompleteMouseMove(event, rpn);
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
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMouseDoubleClick(event, rpn, completed, aborted);
  if (completed)
    completeEditing();
  else {
    if (aborted)
      abortEditing();
    if (rpn)
      repaintNeeded_ = true;
  }
}

static QSharedPointer<DrawingItemBase> idToItem(const QSet<QSharedPointer<DrawingItemBase> > &items, int id)
{
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    if (id == item->id())
      return item;
  }
  return QSharedPointer<DrawingItemBase>();
}

void EditItemManager::keyPress(QKeyEvent *event)
{
  if (!CurrEditLayer)
    return;

  if (hasIncompleteItem()) {
    incompleteKeyPress(event);
    return;
  }

  repaintNeeded_ = false;
  const QSet<QSharedPointer<DrawingItemBase> > items = CurrEditLayer->itemSet();
  QSet<QSharedPointer<DrawingItemBase> > selItems = CurrEditLayer->selectedItemSet();

  const QSet<QSharedPointer<DrawingItemBase> > origSelItems(selItems);
  QSet<int> origSelIds;
  foreach (const QSharedPointer<DrawingItemBase> item, origSelItems)
    origSelIds.insert(item->id());

  QSet<QSharedPointer<DrawingItemBase> > addedItems;
  QSet<QSharedPointer<DrawingItemBase> > removedItems;
  QList<QUndoCommand *> undoCommands;

  // process each of the originally selected items
  foreach (int origSelId, origSelIds) {

    // at this point, the item may or may not exist (it may have been removed in an earlier iteration)

    QSharedPointer<DrawingItemBase> origSelItem = idToItem(items, origSelId);
    if (!origSelItem.isNull()) {
      // it still exists, so pass the event
      QSet<QSharedPointer<DrawingItemBase> > eventItems(items);
      bool rpn = false;
      Editing(origSelItem.data())->keyPress(event, rpn, &undoCommands, &eventItems, &selItems);
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
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteKeyPress(event, rpn, completed, aborted);
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
  if (!CurrEditLayer)
    return;

  if (hasIncompleteItem()) {
    incompleteKeyRelease(event);
    return;
  }

  repaintNeeded_ = false; // whether at least one item needs to be repainted after processing the event

  // send to selected items
  for (int i = 0; i < CurrEditLayer->selectedItemCount(); ++i) {
    bool rpn = false;
    Editing(CurrEditLayer->selectedItemRef(i).data())->keyRelease(event, rpn);
    if (rpn)
      repaintNeeded_ = true;
  }
}

// Handles a key release event for an item in the process of being completed.
void EditItemManager::incompleteKeyRelease(QKeyEvent *event)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  Editing(incompleteItem_.data())->incompleteKeyRelease(event, rpn);
  if (rpn)
    repaintNeeded_ = true;
}

void EditItemManager::plot(bool under, bool over)
{
  if (!over)
    return;

  if (isEditing()) {
    // Apply a transformation so that the items can be plotted with screen coordinates
    // while everything else is plotted in map coordinates.
    glPushMatrix();
    plotRect = PLOTM->getPlotSize();
    int w, h;
    PLOTM->getPlotWindow(w, h);
    glTranslatef(editRect.x1, editRect.y1, 0.0);
    glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);

    const QList<QSharedPointer<EditItems::Layer> > &layers = EditItems::LayerManager::instance()->orderedLayers();
    for (int i = layers.size() - 1; i >= 0; --i) {

      const QSharedPointer<EditItems::Layer> layer = layers.at(i);
      if (layer->isActive() && layer->isVisible()) {

        QList<QSharedPointer<DrawingItemBase> > items = layer->items();
        qStableSort(items.begin(), items.end(), DrawingManager::itemCompare());

        foreach (const QSharedPointer<DrawingItemBase> item, items) {
          EditItemBase::DrawModes modes = EditItemBase::Normal;
          if (CurrEditLayer && CurrEditLayer->containsSelectedItem(item))
            modes |= EditItemBase::Selected;
          if (item == hoverItem_)
            modes |= EditItemBase::Hovered;
          if (item->property("visible", true).toBool()) {
            applyPlotOptions(item);
            setFromLatLonPoints(*item, item->getLatLonPoints());
            Editing(item.data())->draw(modes, false, EditItemsStyle::StyleEditor::instance()->isVisible());
          }
        }
      }
    }
    if (hasIncompleteItem()) { // note that only complete items may be selected
      setFromLatLonPoints(*incompleteItem_, incompleteItem_->getLatLonPoints());
      Editing(incompleteItem_.data())->draw((incompleteItem_ == hoverItem_) ? EditItemBase::Hovered : EditItemBase::Normal, true);
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
  return !incompleteItem_.isNull();
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

QSet<QSharedPointer<DrawingItemBase> > EditItemManager::findHitItems(const QPointF &pos) const
{
  if (!CurrEditLayer)
    return QSet<QSharedPointer<DrawingItemBase> >();
  QSet<QSharedPointer<DrawingItemBase> > hitItems;
  for (int i = 0; i < CurrEditLayer->itemCount(); ++i) {
    if (CurrEditLayer->item(i)->property("visible", false).toBool() == false)
      continue;
    if (Editing(CurrEditLayer->item(i).data())->hit(pos, CurrEditLayer->containsSelectedItem(CurrEditLayer->item(i))))
      hitItems.insert(CurrEditLayer->item(i));
  }
  return hitItems;
}

void EditItemManager::startStopEditing(bool start)
{
  bool enable = !start;

  // Find any actions that conflict with keyboard input.
  foreach (QAction *action, DianaMainWindow::instance()->findChildren<QAction *>()) {
    if (action->shortcut() == QKeySequence(Qt::Key_Space))
      action->setEnabled(enable);
    else if (action->shortcut() == QKeySequence(Qt::Key_End))
      action->setEnabled(enable);
  }
}

void EditItemManager::abortEditing()
{
  if (incompleteItem_) {
    // Release the keyboard focus.
    setFocus(false);

    incompleteItem_.clear();
    hoverItem_.clear();
    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
    emit repaintNeeded();
  }
}

void EditItemManager::completeEditing()
{
  if (incompleteItem_) {
    // Release the keyboard focus.
    setFocus(false);

    addItem(incompleteItem_); // causes repaint
    incompleteItem_.clear();
    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
  }
}

void EditItemManager::pushCommands(const QSet<QSharedPointer<DrawingItemBase> > &addedItems,
                                   const QSet<QSharedPointer<DrawingItemBase> > &removedItems,
                                   const QList<QUndoCommand *> &undoCommands)
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

void EditItemManager::selectItem(const QSharedPointer<DrawingItemBase> &item)
{
  if (CurrEditLayer) {
    CurrEditLayer->insertSelectedItem(item);
    emit selectionChanged();
  }
}

void EditItemManager::deselectItem(const QSharedPointer<DrawingItemBase> &item)
{
  if (CurrEditLayer) {
    CurrEditLayer->removeSelectedItem(item);
    emit selectionChanged();
  }
}

void EditItemManager::deselectAllItems()
{
  if (CurrEditLayer) {
    CurrEditLayer->clearSelectedItems();
    emit selectionChanged();
  }
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
  QSharedPointer<DrawingItemBase> item = *(getSelectedItems().begin());
  if (Properties::PropertiesEditor::instance()->edit(item)) {
    emit itemChanged(item.data()); // ### anybody conncted to this?
    repaint();
  }
}

void EditItemManager::editStyle()
{
  foreach (const QSharedPointer<DrawingItemBase> &item, getSelectedItems()) {
    const QString styleType = item->propertiesRef().value("style:type").toString();
    if (styleType != "Custom") {
      QMessageBox::warning(0, "Warning", QString(
                             "At least one non-custom item selected:\n\n    %1"
                             "\n\nPlease convert all selected items to custom type before editing style.")
                           .arg(styleType));
      return;
    }
  }

  EditItemsStyle::StyleEditor::instance()->edit(getSelectedItems());
}

// Sets the style type of the currently selected items.
void EditItemManager::setStyleType() const
{
  const QAction *action = qobject_cast<QAction *>(sender());
  const QVariantList data = action->data().toList();
  const QVariantList styleItems = data.at(0).toList();
  const QString tgtType = data.at(1).toString();

  foreach (QVariant styleItem, styleItems) {
    DrawingItemBase *item = static_cast<DrawingItemBase *>(styleItem.value<void *>());

    // copy all style properties if converting from non-custom to custom
    if (tgtType == "Custom") {
      const QString srcType = item->propertiesRef().value("style:type").toString();
      if (srcType != "Custom")
        DrawingStyleManager::instance()->setStyle(item, DrawingStyleManager::instance()->getStyle(item->category(), srcType));
    }

    item->setProperty("style:type", tgtType); // set type regardless
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

void EditItemManager::handleLayersUpdate()
{
  updateActionsAndTimes();
  repaint();
}

// Clipboard operations

void EditItemManager::copyItems(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  QString text;

  text += QString("Number of items: %1\n").arg(items.size());
  QVariantList cbItems;

  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    cbItems.append(Editing(item.data())->clipboardVarMap());
    text += QString("%1\n").arg(Editing(item.data())->clipboardPlainText());
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
  const QSet<QSharedPointer<DrawingItemBase> > items = getSelectedItems();
  copyItems(items);
  foreach (const QSharedPointer<DrawingItemBase> item, items)
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
      const QSharedPointer<DrawingItemBase> item = createItemFromVarMap(cbItem.toMap(), &error);
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
  abortEditing();

  mode_ = SelectMode;
  selectAction->setChecked(true);
  //clearCursorStack();
  emit unsetWorkAreaCursor();
}

void EditItemManager::setCreatePolyLineMode()
{
  abortEditing();

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
  abortEditing();

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
  abortEditing();

  mode_ = CreateTextMode;
  const QCursor cursor(Qt::PointingHandCursor);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateCompositeMode()
{
  abortEditing();

  mode_ = CreateCompositeMode;
  const QCursor cursor(Qt::PointingHandCursor);
  emit setWorkAreaCursor(cursor);
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

  // Do not process the event if there is no current edit layer.
  if (!CurrEditLayer)
    return;

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  PLOTM->getPlotWindow(w, h);
  plotRect = PLOTM->getPlotSize();

  if (itemCount() == 0)
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
      const QSet<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(me2.pos());
      QSharedPointer<DrawingItemBase> hitItem; // consider only this item to be hit
      if (!hitItems.empty())
        hitItem = *(hitItems.begin()); // for now; eventually use the one with higher z-value etc. ... TBD

      QList<QAction *> hitItemActions;
      if (!hitItem.isNull()) {
        selectItem(hitItem);
        emit repaintNeeded();
        hitItemActions = Editing(hitItem.data())->actions(me2.pos());
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

      QMenu styleTypeMenu;
      styleTypeMenu.setTitle("Convert");
      styleTypeMenu.setEnabled(!hitItem.isNull());

      QList<QSharedPointer<QAction> > styleTypeActions;

      if (!hitItem.isNull()) {
        QStringList styleNames = DrawingStyleManager::instance()->styles(hitItem.data()->category());
        qSort(styleNames);
        Q_ASSERT(!styleNames.contains("Custom"));
        styleNames.append("Custom");
        foreach (QString styleName, styleNames) {
          if (styleName == "Custom")
            styleTypeMenu.addSeparator();
          QAction *action = new QAction(QString("%1 %2").arg(tr("To")).arg(styleName), 0);
          QVariantList data;
          QVariantList styleItems;
          foreach (const QSharedPointer<DrawingItemBase> styleItem, getSelectedItems())
            styleItems.append(QVariant::fromValue((void *)(styleItem.data())));
          data.append(QVariant(styleItems));
          data.append(styleName);
          action->setData(data);
          connect(action, SIGNAL(triggered()), SLOT(setStyleType()));
          styleTypeActions.append(QSharedPointer<QAction>(action));
          styleTypeMenu.addAction(action);
        }
      }
      contextMenu.addMenu(&styleTypeMenu);

      contextMenu.addSeparator();
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
        if (mode_ == CreatePolyLineMode) {
          const QSharedPointer<DrawingItemBase> item(Drawing(new EditItem_PolyLine::PolyLine()));
          addItem(item, true);
          item->setProperty("style:type", createPolyLineAction->data().toString());
        }
        else if (mode_ == CreateSymbolMode) {
          QSharedPointer<DrawingItemBase> item(Drawing(new EditItem_Symbol::Symbol()));
          addItem(item, true);
          item->setProperty("style:type", createSymbolAction->data().toString());
        } else if (mode_ == CreateCompositeMode)
          addItem(QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Composite::Composite())), true);
        else if (mode_ == CreateTextMode) {
        QSharedPointer<DrawingItemBase> item(Drawing(new EditItem_Text::Text()));
          addItem(item, true);
          item->setProperty("style:type", createTextAction->data().toString());
        }
      }

      // process the event further (delegating it to relevant items etc.)
      mousePress(&me2);
    }
    event->setAccepted(true);
  } else if (event->type() == QEvent::MouseMove) {
    mouseMove(&me2);
    event->setAccepted(true);
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

  // Do not process the event if there is no current edit layer.
  if (!CurrEditLayer)
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
    const QSet<QSharedPointer<DrawingItemBase> > &addedItems, const QSet<QSharedPointer<DrawingItemBase> > &removedItems)
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
