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
#include <EditItems/layergroup.h>
#include <EditItems/toolbar.h>
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
  : selectingOnly_(true)
  , repaintNeeded_(false)
  , skipRepaint_(false)
  , hitOffset_(0)
  , undoView_(0)
  , itemChangeNotificationEnabled_(false)
  , itemsVisibilityForced_(false)
  , itemPropsDirectlyEditable_(false)
{
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
  editPropertiesAction = new QAction(itemPropsDirectlyEditable_ ? tr("Edit P&roperties...") : tr("Show P&roperties..."), this);
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
  setEnabled(true);

  Manager::setEditing(true); // get all mouse- and key events regardless of editing mode, but only allow full editing if selectingOnly_ == false
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

void EditItemManager::setEditing(bool enable)
{
  selectingOnly_ = !enable;

  emit editing(enable);
  if (!enable) {
    hitItems_.clear();
    abortEditing();
    emit unsetWorkAreaCursor();
  } else {
    // restore mode
    if (mode_ == SelectMode)
      setSelectMode();
    else if (mode_ == CreatePolyLineMode)
      setCreatePolyLineMode();
    else if (mode_ == CreateSymbolMode)
      setCreateSymbolMode();
    else if (mode_ == CreateTextMode)
      setCreateTextMode();
    else if (mode_ == CreateCompositeMode)
      setCreateCompositeMode();
    else
      METLIBS_LOG_WARN("EditItemManager::setEditing(): invalid mode: " << mode_);

    // restore any hover item etc.
    QMouseEvent hoverEvent(QEvent::MouseMove, lastHoverPos_, Qt::NoButton, Qt::MouseButtons(), Qt::KeyboardModifiers());
    mouseMove(&hoverEvent);
  }
}

/**
 * Ignores any plot commands passed as a vector of strings.
 */
bool EditItemManager::processInput(const std::vector<std::string>& inp)
{
  return false;
}

QUndoView *EditItemManager::getUndoView()
{
    if (!undoView_)
        undoView_ = new QUndoView(&undoStack_);

    return undoView_;
}

// Adds an item to the scene. \a incomplete indicates whether the item is in the process of being manually placed.
void EditItemManager::addItem(
    const QSharedPointer<DrawingItemBase> &item, QSet<QSharedPointer<DrawingItemBase> > * addedItems, bool incomplete, bool skipRepaint)
{
  if (incomplete) {
    // set this item as the incomplete item
    if (hasIncompleteItem()) {
      // issue warning?
    }
    // Start editing the item.
    editItem(item);

  } else {
    addedItems->insert(item);
  }

  if (!skipRepaint)
    repaint();
}

QSharedPointer<DrawingItemBase> EditItemManager::createItemFromVarMap(const QVariantMap &vmap, QString *error)
{
  return QSharedPointer<DrawingItemBase>(
        createItemFromVarMap_<DrawingItemBase, EditItem_PolyLine::PolyLine, EditItem_Symbol::Symbol,
        EditItem_Text::Text, EditItem_Composite::Composite>(vmap, error));
}

void EditItemManager::addItem_(const QSharedPointer<DrawingItemBase> &item, bool updateNeeded, bool ignoreSelection)
{
  DrawingManager::addItem_(item);
  if (!ignoreSelection)
    selectItem(item, !QApplication::keyboardModifiers().testFlag(Qt::ControlModifier));
  if (!EditItems::ToolBar::instance()->nonSelectActionLocked())
    EditItems::ToolBar::instance()->setSelectAction();
  emit itemAdded(item.data());
  if (updateNeeded)
    update();
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

void EditItemManager::removeItem(const QSharedPointer<DrawingItemBase> &item, QSet<QSharedPointer<DrawingItemBase> > * removedItems)
{
  removedItems->insert(item);
}

void EditItemManager::removeItem_(const QSharedPointer<DrawingItemBase> &item, bool updateNeeded)
{
  DrawingManager::removeItem_(item);
  hitItems_.removeOne(item);
  deselectItem(item);
  emit itemRemoved(item->id());
  if (updateNeeded)
    update();
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
  int i = 0;
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    // Convert the item's screen coordinates to geographic coordinates.
    item->setLatLonPoints(getLatLonPoints(*item));
    removeItem_(item, ++i == items.size());
  }
}

void EditItemManager::retrieveItems(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  int i = 0;
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    // The items stored on the undo stack have been given geographic
    // coordinates, so we use those to obtain screen coordinates.
    if (!item->getLatLonPoints().isEmpty())
      setFromLatLonPoints(*item, item->getLatLonPoints());
    addItem_(item, ++i == items.size(), true);
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

void EditItemManager::mousePress(
    QMouseEvent *event, QList<QUndoCommand *> *undoCommands, QSet<QSharedPointer<DrawingItemBase> > *addedItems,
    QSet<QSharedPointer<DrawingItemBase> > *removedItems, bool &selChanged)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteMousePress(event, addedItems);
    return;
  }

  QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
  const QSet<QSharedPointer<DrawingItemBase> > origSelItems(selItems);

  const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(event->pos());
  QSharedPointer<DrawingItemBase> hitItem; // consider only this item to be hit
  if (!hitItems.empty())
    hitItem = hitItems.first();

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

  if (!hitItem.isNull()) { // an item is still considered hit
    selectItem(hitItem); // ensure the hit item is selected (it might already be)

    // send mouse press to the hit item
    bool multiItemOp = false;
    QSet<QSharedPointer<DrawingItemBase> > eventItems(layerMgr_->itemsInSelectedLayers());

    bool rpn = false;
    Editing(hitItem.data())->mousePress(event, rpn, undoCommands, &eventItems, &selItems, &multiItemOp);
    if (rpn) repaintNeeded_ = true;
    *addedItems = eventItems - layerMgr_->itemsInSelectedLayers();
    *removedItems = layerMgr_->itemsInSelectedLayers() - eventItems;

    if (layerMgr_->selectedLayersContainItem(hitItem)) {
      // the hit item is still there
      if (multiItemOp) {
        // send the mouse press to other selected items
        // (note that these are not allowed to modify item sets, nor requesting items to be copied,
        // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
        foreach (const QSharedPointer<DrawingItemBase> item, selItems)
          if (item != hitItem) {
            rpn = false;
            Editing(item.data())->mousePress(event, rpn, undoCommands);
            if (rpn)
              repaintNeeded_ = true;
          }
      }
    } else {
      // the hit item removed itself as a result of the mouse press and it makes no sense
      // to send the mouse press to other items
    }
  }

  if (layerMgr_->itemsInSelectedLayers(true) != origSelItems)
    selChanged = true;
}

// Handles a mouse press event for an item in the process of being completed.
void EditItemManager::incompleteMousePress(QMouseEvent *event, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMousePress(event, rpn, completed, aborted);

  // Record geographic coordinates for the item as they are added to it.
  incompleteItem_->setLatLonPoints(getLatLonPoints(*incompleteItem_));
  if (completed)
    completeEditing(addedItems);
  else if (aborted)
    abortEditing();

  if (rpn)
    repaintNeeded_ = true;
}

void EditItemManager::mouseRelease(QMouseEvent *event, QList<QUndoCommand *> *undoCommands, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteMouseRelease(event, addedItems);
    return;
  }

  repaintNeeded_ = false;

  // send to selected items
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true))
    Editing(item.data())->mouseRelease(event, repaintNeeded_, undoCommands);
}

// Handles a mouse release event for an item in the process of being completed.
void EditItemManager::incompleteMouseRelease(QMouseEvent *event, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMouseRelease(event, rpn, completed, aborted);
  if (completed)
    completeEditing(addedItems);
  else {
    if (aborted)
      abortEditing();
    if (rpn)
      repaintNeeded_ = true;
  }
}

void EditItemManager::mouseMove(QMouseEvent *event)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteMouseMove(event);
    return;
  }

  // Check if the event is part of a multi-select operation using a rubberband-rectangle.
  // In that case, the event should only be used to update the selection (and tell items to redraw themselves as
  // appropriate), and NOT be passed on through the mouseMove() functions of the selected items ... 2 B DONE!

  repaintNeeded_ = false;

  QList<QSharedPointer<DrawingItemBase> > origHitItems = hitItems_;
  hitItems_.clear();
  const bool hover = !event->buttons();
  bool rpn = false;

  if (hover) {
    lastHoverPos_ = event->pos();
    QList<QSharedPointer<DrawingItemBase> > missedItems;
    const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(lastHoverPos_, &missedItems);
    foreach (const QSharedPointer<DrawingItemBase> &hitItem, hitItems)
      Editing(hitItem.data())->updateHoverPos(lastHoverPos_);
    foreach (const QSharedPointer<DrawingItemBase> &missedItem, missedItems)
      Editing(missedItem.data())->updateHoverPos(QPoint(-1, -1));

    if (!hitItems.empty()) {
      hitItems_ = hitItems;

      // send mouse hover event to the hover item
      Editing(hitItems_.first().data())->mouseHover(event, rpn, selectingOnly_);
      if (rpn) repaintNeeded_ = true;
    } else if (!origHitItems.isEmpty()) {
      Editing(origHitItems.first().data())->mouseHover(event, rpn, selectingOnly_);
      if (rpn) repaintNeeded_ = true;
    }
  } else {
    // send move event to all selected items
    foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true)) {
      Editing(item.data())->mouseMove(event, rpn);
      item->setLatLonPoints(getLatLonPoints(*item));
      if (rpn) repaintNeeded_ = true;
    }
  }

  if (hitItems_ != origHitItems)
    repaintNeeded_ = true;
}

// Handles a mouse move event for an item in the process of being completed.
void EditItemManager::incompleteMouseMove(QMouseEvent *event)
{
  Q_ASSERT(hasIncompleteItem());

  const QList<QSharedPointer<DrawingItemBase> > origHitItems = hitItems_;
  hitItems_.clear();
  const bool hover = !event->buttons();
  if (hover) {
    bool rpn = false;
    Editing(incompleteItem_.data())->incompleteMouseHover(event, rpn);
    incompleteItem_->setLatLonPoints(getLatLonPoints(*incompleteItem_));
    if (rpn) repaintNeeded_ = true;
    if (Editing(incompleteItem_.data())->hit(event->pos(), false))
      hitItems_ = QList<QSharedPointer<DrawingItemBase> >() << incompleteItem_;
  } else {
    bool rpn = false;
    Editing(incompleteItem_.data())->incompleteMouseMove(event, rpn);
    if (rpn) repaintNeeded_ = true;
  }

  if (hitItems_ != origHitItems)
    repaintNeeded_ = true;
}

void EditItemManager::mouseDoubleClick(QMouseEvent *event, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  if (hasIncompleteItem()) {
    incompleteMouseDoubleClick(event, addedItems);
    return;
  }

  // do nothing for now
}

void EditItemManager::incompleteMouseDoubleClick(QMouseEvent *event, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteMouseDoubleClick(event, rpn, completed, aborted);
  if (completed)
    completeEditing(addedItems);
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

void EditItemManager::keyPress(
    QKeyEvent *event, QList<QUndoCommand *> *undoCommands, QSet<QSharedPointer<DrawingItemBase> > *addedItems,
    QSet<QSharedPointer<DrawingItemBase> > *removedItems, bool &selChanged)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteKeyPress(event, addedItems);
    return;
  }

  repaintNeeded_ = false;
  const QSet<QSharedPointer<DrawingItemBase> > items = layerMgr_->itemsInSelectedLayers();
  QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);

  const QSet<QSharedPointer<DrawingItemBase> > origSelItems(selItems);
  QSet<int> origSelIds;
  foreach (const QSharedPointer<DrawingItemBase> item, origSelItems)
    origSelIds.insert(item->id());

  // process each of the originally selected items
  foreach (int origSelId, origSelIds) {

    // at this point, the item may or may not exist (it may have been removed in an earlier iteration)

    QSharedPointer<DrawingItemBase> origSelItem = idToItem(items, origSelId);
    if (!origSelItem.isNull()) {
      // it still exists, so pass the event
      QSet<QSharedPointer<DrawingItemBase> > eventItems(items);
      bool rpn = false;
      Editing(origSelItem.data())->keyPress(event, rpn, undoCommands, &eventItems, &selItems);
      if (rpn) repaintNeeded_ = true;
      addedItems->unite(eventItems - items);
      removedItems->unite(items - eventItems);
      selItems.subtract(*removedItems);
    }
  }

  if (selItems != origSelItems)
    selChanged = true;
}

// Handles a key press event for an item in the process of being completed.
void EditItemManager::incompleteKeyPress(QKeyEvent *event, QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_.data())->incompleteKeyPress(event, rpn, completed, aborted);
  if (completed)
    completeEditing(addedItems);
  else {
    if (aborted)
      abortEditing();
    if (rpn)
      repaintNeeded_ = true;
  }
}

void EditItemManager::keyRelease(QKeyEvent *event)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteKeyRelease(event);
    return;
  }

  repaintNeeded_ = false; // whether at least one item needs to be repainted after processing the event

  // send to selected items
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true)) {
    bool rpn = false;
    Editing(item.data())->keyRelease(event, rpn);
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

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  setPlotRect(PLOTM->getPlotSize());
  int w, h;
  PLOTM->getPlotWindow(w, h);
  glTranslatef(editRect.x1, editRect.y1, 0.0);
  glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);

  const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
  const QList<QSharedPointer<EditItems::Layer> > &layers = layerMgr_->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {

    const QSharedPointer<EditItems::Layer> layer = layers.at(i);
    if (layer->isActive() && layer->isVisible()) {

      QList<QSharedPointer<DrawingItemBase> > items = layer->items();
      qStableSort(items.begin(), items.end(), DrawingManager::itemCompare());

      foreach (const QSharedPointer<DrawingItemBase> item, items) {
        EditItemBase::DrawModes modes = EditItemBase::Normal;
        if (isEditing()) {
          if (selItems.contains(item))
            modes |= EditItemBase::Selected;
          if ((!hitItems_.isEmpty()) && (item == hitItems_.first()))
            modes |= EditItemBase::Hovered;
        }
        if (itemsVisibilityForced_ || item->property("visible", true).toBool()) {
          applyPlotOptions(item);
          setFromLatLonPoints(*item, item->getLatLonPoints());
          Editing(item.data())->draw(modes, false, EditItemsStyle::StyleEditor::instance()->isVisible());
        }
      }
    }
  }
  if (hasIncompleteItem()) { // note that only complete items may be selected
    setFromLatLonPoints(*incompleteItem_, incompleteItem_->getLatLonPoints());
    Editing(incompleteItem_.data())->draw(
          ((!hitItems_.isEmpty()) && (incompleteItem_ == hitItems_.first())) ? EditItemBase::Hovered : EditItemBase::Normal, true);
  }

  glPopMatrix();
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

QList<QSharedPointer<DrawingItemBase> > EditItemManager::findHitItems(
    const QPointF &pos, QList<QSharedPointer<DrawingItemBase> > *missedItems) const
{
  if (layerMgr_->selectedLayers().isEmpty())
    return QList<QSharedPointer<DrawingItemBase> >();

  const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);

  QList<QSharedPointer<DrawingItemBase> > hitItems;
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers()) {
    if ((!itemsVisibilityForced_) && (!item->property("visible", true).toBool()))
      continue;
    if (Editing(item.data())->hit(pos, selItems.contains(item)))
      hitItems.append(item);
    else if (missedItems)
      missedItems->append(item);
  }

  if (hitItems.size() > 1) { // rotate list
    const int steps = hitOffset_ % hitItems.size();
    for (int i = 0; i < steps; ++i)
      hitItems.append(hitItems.takeFirst());
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
    hitItems_.clear();
    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
    emit repaintNeeded();
  }
}

void EditItemManager::completeEditing(QSet<QSharedPointer<DrawingItemBase> > *addedItems)
{
  if (incompleteItem_) {
    // Release the keyboard focus.
    setFocus(false);

    addItem(incompleteItem_, addedItems); // causes repaint
    incompleteItem_.clear();
    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
  }
}

// Pushes commands onto the undo stack.
void EditItemManager::pushCommands(const QList<QUndoCommand *> &undoCommands,
                                   const QSet<QSharedPointer<DrawingItemBase> > &addedItems,
                                   const QSet<QSharedPointer<DrawingItemBase> > &removedItems)
{
  const bool modifiedItems = !undoCommands.isEmpty();
  const bool addedOrRemovedItems = (!addedItems.empty()) || (!removedItems.empty());

  if ((!modifiedItems) && (!addedOrRemovedItems))
    return;

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

  undoStack_.endMacro();
  skipRepaint_ = false;
  repaintNeeded_ = true;
}

bool EditItemManager::selectItem(const QSharedPointer<DrawingItemBase> &item, bool exclusive, bool notify)
{
  if (layerMgr_->selectItem(item, exclusive)) {
    if (notify)
      emit selectionChanged();
    return true;
  }
  return false;
}

bool EditItemManager::selectItem(int id, bool exclusive, bool notify)
{
  if (layerMgr_->selectItem(id, exclusive)) {
    if (notify)
      emit selectionChanged();
    return true;
  }
  return false;
}

void EditItemManager::deselectItem(const QSharedPointer<DrawingItemBase> &item, bool notify)
{
  if (layerMgr_->deselectItem(item, notify) && notify)
    emit selectionChanged();
}

void EditItemManager::deselectAllItems(bool notify)
{
  if (layerMgr_->deselectAllItems(notify) && notify)
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
  Q_ASSERT(layerMgr_->itemsInSelectedLayers(true).size() == 1);
  QSharedPointer<DrawingItemBase> item = *(layerMgr_->itemsInSelectedLayers(true).begin());
  if (Properties::PropertiesEditor::instance()->edit(item, !itemPropsDirectlyEditable_))
    repaint();
}

void EditItemManager::editStyle()
{
  EditItemsStyle::StyleEditor::instance()->edit(layerMgr_->itemsInSelectedLayers(true));
}

// Sets the style type of the currently selected items.
void EditItemManager::setStyleType()
{
  const QVariantList data = qobject_cast<QAction *>(sender())->data().toList();
  QList<QSharedPointer<DrawingItemBase> > items;
  const QString newType = data.at(1).toString();
  foreach (QVariant item, data.at(0).toList())
    items.append(item.value<QSharedPointer<DrawingItemBase> >());
  undoStack()->push(new SetStyleTypeCommand("set style type", items, newType));
}

void EditItemManager::updateActions()
{
  const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
  cutAction->setEnabled(selItems.size() > 0);
  copyAction->setEnabled(selItems.size() > 0);
  pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
  editPropertiesAction->setEnabled(selItems.size() == 1);
  editStyleAction->setEnabled(selItems.size() > 0);
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

void EditItemManager::update()
{
  updateActionsAndTimes();
  repaint();
}

void EditItemManager::enableItemChangeNotification(bool enabled)
{
  itemChangeNotificationEnabled_ = enabled;
}

void EditItemManager::setItemChangeFilter(const QString &itemType)
{
  itemChangeFilter_ = itemType;
}

// Emits the itemChanged() signal to notify about a _potential_ change to a single-selected
// item filtered according to setItemChangeFilter().
// The signal is also emitted whenever the above condition goes from true to false.
void EditItemManager::emitItemChanged() const
{
  if (!itemChangeNotificationEnabled_)
    return;

  QList<QVariantMap> itemProps;

  QList<QSharedPointer<EditItems::Layer> > layers = layerMgr_->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {
    const QSharedPointer<EditItems::Layer> layer = layers.at(i);

    foreach (const QSharedPointer<DrawingItemBase> item, layer->selectedItems()) {
      const QString type(item->properties().value("style:type").toString());
      if (itemChangeFilter_ != type)
        continue;

      QVariantMap props;
      props.insert("type", type);
      props.insert("layer:index", i);
      props.insert("layer:visible", layer->isVisible());
      props.insert("id", item->id());
      props.insert("visible", item->property("visible", true).toBool());
      //
      setFromLatLonPoints(*item, item->getLatLonPoints());
      QVariantList latLonPoints;
      foreach (QPointF p, item->getLatLonPoints())
        latLonPoints.append(p);
      props.insert("latLonPoints", latLonPoints);

      itemProps.append(props);
    }
  }

  static bool lastCallMatched = false;
  static QVariantMap lastEmittedVMap;
  if (itemProps.size() == 1) {
    QVariantMap vmap = itemProps.first();
    if (vmap != lastEmittedVMap) { // avoid emitting successive duplicates
      emit itemChanged(vmap);
      lastEmittedVMap = vmap;
    }
    lastCallMatched = true;
  } else {
    if (lastCallMatched) {
      // send an invalid variant map to notify about the transition from match to mismatch
      emit itemChanged(QVariantMap());
      lastEmittedVMap = QVariantMap();
    }
    lastCallMatched = false;
  }
}

void EditItemManager::emitLoadFile(const QString &fileName) const
{
  emit loadFile(fileName);
}

void EditItemManager::setItemsVisibilityForced(bool forced)
{
  itemsVisibilityForced_ = forced;
  repaint(); // or a full handleLayersUpdate()?
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
  copyItems(layerMgr_->itemsInSelectedLayers(true));
}

void EditItemManager::cutSelectedItems(QSet<QSharedPointer<DrawingItemBase> > *removedItems)
{
  const QSet<QSharedPointer<DrawingItemBase> > items = layerMgr_->itemsInSelectedLayers(true);
  copyItems(items);
  foreach (const QSharedPointer<DrawingItemBase> item, items)
    removeItem(item, removedItems);

  updateActionsAndTimes();
}

void EditItemManager::cutSelectedItems()
{
  QSet<QSharedPointer<DrawingItemBase> > removedItems;
  cutSelectedItems(&removedItems);
  pushCommands(QList<QUndoCommand *>(), QSet<QSharedPointer<DrawingItemBase> >(), removedItems);
}

void EditItemManager::pasteItems(QSet<QSharedPointer<DrawingItemBase> > *addedItems)
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
      if (item) {
        item->setSelected();
        addItem(item, addedItems, false);
      } else {
        QMessageBox::warning(0, "Error", error);
      }
    }
  }

  layerMgr_->deselectAllItems();
  updateActionsAndTimes();
}

void EditItemManager::pasteItems()
{
  QSet<QSharedPointer<DrawingItemBase> > addedItems;
  pasteItems(&addedItems);
  pushCommands(QList<QUndoCommand *>(), addedItems, QSet<QSharedPointer<DrawingItemBase> >());
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
  const QCursor cursor = QCursor(QPixmap(paint_create_polyline_xpm), 4, 4);
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
  const QCursor cursor = QCursor(QPixmap(paint_create_symbol_xpm), 4, 4);
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
  emitItemChanged();
}

// Manager API

void EditItemManager::sendMouseEvent(QMouseEvent *event, EventResult &res)
{
  event->ignore();
  res.savebackground= true;   // Save the background after painting.
  res.background= false;      // Don't paint the background.
  res.repaint= false;
  //res.newcursor= edit_cursor;
  res.newcursor= keep_it;

  // skip if no layers are selected
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  PLOTM->getPlotWindow(w, h);
  setPlotRect(PLOTM->getPlotSize());

  if (layerMgr_->selectedLayersItemCount() == 0)
    setEditRect(PLOTM->getPlotSize());

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates. This gives us displaced screen coordinates - these
  // are coordinates relative to the original edit rectangle.
  float dx = (plotRect.x1 - editRect.x1) * (w/plotRect.width());
  float dy = (plotRect.y1 - editRect.y1) * (h/plotRect.height());

  // Translate the mouse event by the current displacement of the viewport.
  QMouseEvent me2(event->type(), QPoint(event->x() + dx, event->y() + dy),
                  event->globalPos(), event->button(), event->buttons(), event->modifiers());

  if (selectingOnly_) {
   // Only allow single-selection and basic hover highlighting.
   // Modifying items otherwise is not possible in this mode.

    if (event->type() == QEvent::MouseButtonPress) {
      if (me2.button() == Qt::LeftButton) {
        deselectAllItems(false);
        const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(me2.pos());
        if (!hitItems.empty()) {
          selectItem(hitItems.first(), true, false);
          event->accept();
        }
        emit selectionChanged();
        repaintNeeded_ = true;
      }
    } else if (event->type() == QEvent::MouseMove) {
      mouseMove(&me2);
    }

    if (repaintNeeded_)
      emit repaintNeeded();

    return;
  }

  QList<QUndoCommand *> undoCommands;
  QSet<QSharedPointer<DrawingItemBase> > addedItems;
  QSet<QSharedPointer<DrawingItemBase> > removedItems;
  bool selChanged = false;

  if (event->type() == QEvent::MouseButtonPress) {

    if ((me2.button() == Qt::RightButton) && !hasIncompleteItem()) {
      // open a context menu

      // get actions contributed by a hit item (if any)
      const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(me2.pos());
      QSharedPointer<DrawingItemBase> hitItem; // consider only this item to be hit
      if (!hitItems.empty())
        hitItem = hitItems.first();

      QList<QAction *> hitItemActions;
      QList<QPointF> hitItemOrigPoints;
      if (!hitItem.isNull()) {
        selectItem(hitItem);
        emit repaintNeeded();
        hitItemActions = Editing(hitItem.data())->actions(me2.pos());
        hitItemOrigPoints = hitItem->getPoints();
      }

      // populate the menu
      QList<QSharedPointer<DrawingItemBase> > selectedItems = layerMgr_->itemsInSelectedLayers(true).toList();
      QSet<DrawingItemBase::Category> selectedCategories;
      for (int i = 0; i < selectedItems.size(); ++i)
        selectedCategories.insert(selectedItems.at(i)->category());

      QMenu contextMenu;
      contextMenu.addAction(cutAction);
      contextMenu.addAction(copyAction);
      contextMenu.addAction(pasteAction);
      pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));

      contextMenu.addSeparator();
      contextMenu.addAction(editPropertiesAction);
      editPropertiesAction->setEnabled(selectedItems.size() == 1);
      contextMenu.addAction(editStyleAction);
      editStyleAction->setEnabled(!selectedItems.isEmpty());

      QMenu styleTypeMenu;
      styleTypeMenu.setTitle("Convert");
      styleTypeMenu.setEnabled(selectedCategories.size() == 1);
      if (styleTypeMenu.isEnabled()) {
        QStringList styleTypes = DrawingStyleManager::instance()->styles(*(selectedCategories.begin()));
        qSort(styleTypes);
        foreach (QString styleType, styleTypes) {
          QAction *action = new QAction(QString("%1 %2").arg(tr("To")).arg(styleType), 0);
          QVariantList data;
          QVariantList styleItems;
          foreach (const QSharedPointer<DrawingItemBase> styleItem, selectedItems)
            styleItems.append(QVariant::fromValue(styleItem));
          data.append(QVariant(styleItems));
          data.append(styleType);
          action->setData(data);
          connect(action, SIGNAL(triggered()), SLOT(setStyleType()));
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

      if (!hasIncompleteItem())
        emitItemChanged();

      if ((!hitItem.isNull()) && (hitItem->getPoints() != hitItemOrigPoints))
        undoCommands.append(new SetGeometryCommand(Editing(hitItem.data()), hitItemOrigPoints, hitItem->getPoints()));

    } else {

      // create a new item if necessary
      if ((me2.button() == Qt::LeftButton) && !hasIncompleteItem()) {
        QSharedPointer<DrawingItemBase> item;

        if (mode_ == CreatePolyLineMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_PolyLine::PolyLine()));
          addItem(item, &addedItems, true);
          item->setProperty("style:type", createPolyLineAction->data().toString());
        } else if (mode_ == CreateSymbolMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Symbol::Symbol()));
          addItem(item, &addedItems, true);
          item->setProperty("style:type", createSymbolAction->data().toString());
        } else if (mode_ == CreateCompositeMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Composite::Composite()));
          addItem(item, &addedItems, true);
          item->setProperty("style:type", createCompositeAction->data().toString());
        } else if (mode_ == CreateTextMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Text::Text()));
          addItem(item, &addedItems, true);
          item->setProperty("style:type", createTextAction->data().toString());
        }

        if (!item.isNull())
          DrawingStyleManager::instance()->setStyle(
                item.data(), DrawingStyleManager::instance()->getStyle(item->category(), item->propertiesRef().value("style:type").toString()));
      }

      // process the event further (delegating it to relevant items etc.)
      mousePress(&me2, &undoCommands, &addedItems, &removedItems, selChanged);

      if (needsRepaint() && !hasIncompleteItem())
        emitItemChanged();
    }

    if (selChanged) {
      emit selectionChanged();
      repaintNeeded_ = true;
    }

    event->accept();

  } else if (event->type() == QEvent::MouseMove) {
    mouseMove(&me2);
    event->accept();
  }

  else if (event->type() == QEvent::MouseButtonRelease) {
    mouseRelease(&me2, &undoCommands, &addedItems);
    event->accept();
    if (needsRepaint() && !hasIncompleteItem())
      emitItemChanged();
  }

  else if (event->type() == QEvent::MouseButtonDblClick) {
    mouseDoubleClick(&me2, &addedItems);
    event->accept();
  }

  pushCommands(undoCommands, addedItems, removedItems);

  res.repaint = needsRepaint();
  res.action = canUndo() ? objects_changed : no_action;

  if (event->type() != QEvent::MouseMove)
    updateActionsAndTimes();
}

bool EditItemManager::cycleHitOrder(QKeyEvent *event)
{
  if (hitItems_.size() > 1) {
    // multiple items are hit at the last hover position, so cycle through them
    hitOffset_ += ((event->key() == Qt::Key_PageUp) ? 1 : -1);
    QMouseEvent hoverEvent(QEvent::MouseMove, lastHoverPos_, Qt::NoButton, Qt::MouseButtons(), event->modifiers());
    mouseMove(&hoverEvent);
    event->accept();
    return true;
  }
  return false;
}

void EditItemManager::sendKeyboardEvent(QKeyEvent *event, EventResult &res)
{
  event->ignore();
  res.savebackground= true;   // Save the background after painting.
  res.background= false;      // Don't paint the background.
  res.repaint= false;

  // skip if no layers are selected
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  res.repaint = true;
  res.background = true;


  if (selectingOnly_) {
    // Only allow cycling the hit order.
    // Modifying items otherwise is not possible in this mode.

    if (event->type() == QEvent::KeyPress) {
      if (event->modifiers().testFlag(Qt::NoModifier) && ((event->key() == Qt::Key_PageUp) || (event->key() == Qt::Key_PageDown))) {
        if (cycleHitOrder(event))
          return;
      }
    }

    return;
  }

  QList<QUndoCommand *> undoCommands;
  QSet<QSharedPointer<DrawingItemBase> > addedItems;
  QSet<QSharedPointer<DrawingItemBase> > removedItems;
  bool selChanged = false;

  if (event->type() == QEvent::KeyPress) {
    if (cutAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      cutSelectedItems(&removedItems);
    } else if (copyAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      copySelectedItems();
    } else if (pasteAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      pasteItems(&addedItems);
    } else if (editPropertiesAction->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      editProperties();
    } else if (event->modifiers().testFlag(Qt::NoModifier) && ((event->key() == Qt::Key_PageUp) || (event->key() == Qt::Key_PageDown))) {
      if (cycleHitOrder(event))
        return;
    }
  } else {
    return;
  }

  if (!event->isAccepted())
    keyPress(event, &undoCommands, &addedItems, &removedItems, selChanged);

  if (selChanged)
    repaintNeeded_ = true;

  pushCommands(undoCommands, addedItems, removedItems);

  updateActionsAndTimes();

  if (event->isAccepted() || needsRepaint())
    emitItemChanged();
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

SetStyleTypeCommand::SetStyleTypeCommand(const QString &text, const QList<QSharedPointer<DrawingItemBase> > &items, const QString &newType, QUndoCommand *parent)
    : QUndoCommand(text, parent)
    , items_(items)
    , newType_(newType)
{
  foreach (const QSharedPointer<DrawingItemBase> item, items_)
    oldTypes_.append(item->propertiesRef().value("style:type").toString());
}

void SetStyleTypeCommand::undo()
{
  for (int i = 0; i < items_.size(); ++i) {
    DrawingStyleManager::instance()->setStyle(items_.at(i).data(), DrawingStyleManager::instance()->getStyle(items_.at(i)->category(), oldTypes_.at(i)));
    items_.at(i)->setProperty("style:type", oldTypes_.at(i));
  }
}

void SetStyleTypeCommand::redo()
{
  foreach (const QSharedPointer<DrawingItemBase> item, items_) {
    DrawingStyleManager::instance()->setStyle(item.data(), DrawingStyleManager::instance()->getStyle(item->category(), newType_));
    item->setProperty("style:type", newType_);
  }
}
