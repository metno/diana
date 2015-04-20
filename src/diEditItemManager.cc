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

#include <vector>

#include <QtGui> // ### include only relevant headers ... TBD
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>

#include <diLocalSetupParser.h>
#include <puTools/miSetupParser.h>

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
#include <EditItems/drawingstylemanager.h>
#include <EditItems/toolbar.h>
#include <qtMainWindow.h>
#include "paint_select2.xpm"
#include "paint_create_polyline.xpm"
#include "paint_create_symbol.xpm"
#include "paint_create_text.xpm"

#define PLOTM PlotModule::instance()

using namespace std;
using namespace miutil;

class UndoView : public QUndoView
{
public:
  UndoView(QUndoStack *undoStack) : QUndoView(undoStack) {}
private:
  virtual void keyPressEvent(QKeyEvent *event)
  {
    // to avoid crash, support only basic keyboard navigation
    if (event->matches(QKeySequence::MoveToPreviousLine) || event->matches(QKeySequence::MoveToNextLine))
      QUndoView::keyPressEvent(event);
    else
      event->accept();
  }
};

EditItemManager *EditItemManager::self_ = 0;

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

  undoStack_.setUndoLimit(1000); // ### may need to be tuned

  connect(&undoStack_, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
  connect(&undoStack_, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
  connect(&undoStack_, SIGNAL(indexChanged(int)), this, SLOT(repaint()));

  copyAction_ = new QAction(tr("Copy"), this);
  copyAction_->setShortcut(QKeySequence::Copy);
  cutAction_ = new QAction(tr("Cut"), this);
  cutAction_->setShortcut(tr("Ctrl+X"));
  pasteAction_ = new QAction(tr("Paste"), this);
  pasteAction_->setShortcut(QKeySequence::Paste);
  joinAction_ = new QAction(tr("Join"), this);
  joinAction_->setShortcut(QString("J"));
  unjoinAction_ = new QAction(tr("Unjoin"), this);
  unjoinAction_->setShortcut(tr("Ctrl+J"));
  toggleReversedAction_ = new QAction(tr("Toggle reversed"), this);
  toggleReversedAction_->setShortcut(QString("R"));
  editPropertiesAction_ = new QAction(itemPropsDirectlyEditable_ ? tr("Edit P&roperties...") : tr("Show P&roperties..."), this);
  editPropertiesAction_->setShortcut(tr("Ctrl+R"));
  editStyleAction_ = new QAction(tr("Edit Style..."), this);
  //editStyleAction->setShortcut(tr("Ctrl+Y")); // ### already in use?
  undoAction_ = undoStack_.createUndoAction(this);
  redoAction_ = undoStack_.createRedoAction(this);

  selectAction_ = new QAction(QPixmap(paint_select2_xpm), tr("&Select"), this);
  //selectAction->setShortcut(tr("Ctrl+???"));
  selectAction_->setCheckable(true);

  createPolyLineAction_ = new QAction(QPixmap(paint_create_polyline_xpm), tr("Create &Polyline"), this);
  //createPolyLineAction->setShortcut(tr("Ctrl+???"));
  createPolyLineAction_->setCheckable(true);

  createSymbolAction_ = new QAction(QPixmap(paint_create_symbol_xpm), tr("Create &Symbol"), this);
  //createSymbolAction->setShortcut(tr("Ctrl+???"));
  createSymbolAction_->setCheckable(true);

  createTextAction_ = new QAction(QPixmap(paint_create_text_xpm), tr("Text"), this);
  createTextAction_->setCheckable(true);

  createCompositeAction_ = new QAction(tr("Composite"), this);
  createCompositeAction_->setCheckable(true);

  connect(copyAction_, SIGNAL(triggered()), SLOT(copySelectedItems()));
  connect(cutAction_, SIGNAL(triggered()), SLOT(cutSelectedItems()));
  connect(pasteAction_, SIGNAL(triggered()), SLOT(pasteItems()));
  connect(joinAction_, SIGNAL(triggered()), SLOT(joinSelectedItems()));
  connect(unjoinAction_, SIGNAL(triggered()), SLOT(unjoinSelectedItems()));
  connect(toggleReversedAction_, SIGNAL(triggered()), SLOT(toggleReversedForSelectedItems()));
  connect(editPropertiesAction_, SIGNAL(triggered()), SLOT(editProperties()));
  connect(editStyleAction_, SIGNAL(triggered()), SLOT(editStyle()));
  connect(selectAction_, SIGNAL(triggered()), SLOT(setSelectMode()));
  connect(createPolyLineAction_, SIGNAL(triggered()), SLOT(setCreatePolyLineMode()));
  connect(createSymbolAction_, SIGNAL(triggered()), SLOT(setCreateSymbolMode()));
  connect(createTextAction_, SIGNAL(triggered()), SLOT(setCreateTextMode()));
  connect(createCompositeAction_, SIGNAL(triggered()), SLOT(setCreateCompositeMode()));

  setSelectMode();
  setEnabled(true);

  Manager::setEditing(true); // get all mouse- and key events regardless of editing mode, but only allow full editing if selectingOnly_ == false
}

EditItemManager::~EditItemManager()
{
}

EditItemManager *EditItemManager::instance()
{
  if (!EditItemManager::self_)
    EditItemManager::self_ = new EditItemManager();

  return EditItemManager::self_;
}

bool EditItemManager::parseSetup()
{
  vector<string> section;

  if (!SetupParser::getSection("DRAWING", section))
    METLIBS_LOG_WARN("No DRAWING section.");

  for (unsigned int i = 0; i < section.size(); ++i) {

    // Split the line into tokens.
    vector<string> tokens = miutil::split_protected(section[i], '\"', '\"', " ", true);
    QHash<QString, QString> items;

    for (unsigned int j = 0; j < tokens.size(); ++j) {
      string key, value;
      SetupParser::splitKeyValue(tokens[j], key, value);
      items[QString::fromStdString(key)] = QString::fromStdString(value);
    }

    // Check for different types of definition.
    if (items.contains("hide-property-sections")) {
      QStringList values = items.value("hide-property-sections").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("hide", values);
    }
  }

  // Let the base class parse the section of the setup file.
  return DrawingManager::parseSetup();
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
  if (!undoView_) {
    undoView_ = new UndoView(&undoStack_);
    undoView_->setWindowTitle("Drawing tool undo/redo stack");
  }

  return undoView_;
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
    if (!item->getLatLonPoints().isEmpty())
      setFromLatLonPoints(*item, item->getLatLonPoints()); // obtain screen coords from geo coords
    addItem_(item, true, true);
  }

  if (!skipRepaint)
    repaint();
}

DrawingItemBase *EditItemManager::createItem(const QString &type)
{
  EditItemBase *item = 0;
  if (type == "PolyLine") {
    item = new EditItem_PolyLine::PolyLine();
  } else if (type == "Symbol") {
    item = new EditItem_Symbol::Symbol();
  } else if (type == "Text") {
    item = new EditItem_Text::Text();
  } else if (type == "Composite") {
    item = new EditItem_Composite::Composite();
  }
  return Drawing(item);
}

QSharedPointer<DrawingItemBase> EditItemManager::createItemFromVarMap(const QVariantMap &vmap, QString *error)
{
  Q_ASSERT(!vmap.empty());
  Q_ASSERT(vmap.contains("type"));
  Q_ASSERT(vmap.value("type").canConvert(QVariant::String));

  QString type = vmap.value("type").toString().split("::").last();
  DrawingItemBase *item = createItem(type);

  if (item) {
    item->setProperties(vmap);
    setFromLatLonPoints(*item, item->getLatLonPoints());

    EditItem_Composite::Composite *c = dynamic_cast<EditItem_Composite::Composite *>(item);
    if (c)
      c->createElements();
  }

  return QSharedPointer<DrawingItemBase>(Drawing(item));
}

void EditItemManager::addItem_(const QSharedPointer<DrawingItemBase> &item, bool updateNeeded, bool ignoreSelection)
{
  DrawingManager::addItem_(item);
  if (!ignoreSelection)
    selectItem(item, !QApplication::keyboardModifiers().testFlag(Qt::ControlModifier));
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

void EditItemManager::removeItem(const QSharedPointer<DrawingItemBase> &item)
{
  item->setLatLonPoints(getLatLonPoints(*item)); // convert screen coords to geo coords
  removeItem_(item);
}

void EditItemManager::removeItem_(const QSharedPointer<DrawingItemBase> &item, bool updateNeeded)
{
  DrawingManager::removeItem_(item);
  hitItems_.removeOne(item);
  deselectItem(item);
  updateJoins();
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

void EditItemManager::mousePress(QMouseEvent *event)
{
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteMousePress(event);
    return;
  }

  QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
  const QSet<QSharedPointer<DrawingItemBase> > origSelItems(selItems);

  const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(event->pos());
  hitItem_.clear();
  if (!hitItems.empty())
    hitItem_ = hitItems.first(); // consider only this item to be hit

  const bool hitSelItem = selItems.contains(hitItem_); // whether an already selected item was hit
  const bool selectMulti = event->modifiers() & Qt::ControlModifier;

  repaintNeeded_ = false;


  // update selection and hit status
  if (!(hitSelItem || ((!hitItem_.isNull()) && selectMulti))) {
    deselectAllItems();
  } else if (selectMulti && hitSelItem && (selItems.size() > 1)) {
    deselectItem(hitItem_);
    hitItem_.clear();
  }

  if (!hitItem_.isNull()) { // an item is still considered hit
    selectItem(hitItem_); // ensure the hit item is selected (it might already be)

    // send mouse press to the hit item
    bool multiItemOp = false;

    bool rpn = false;
    Editing(hitItem_.data())->mousePress(event, rpn, &multiItemOp);
    if (rpn) repaintNeeded_ = true;

    if (layerMgr_->selectedLayersContainItem(hitItem_)) {
      // the hit item is still there

      if (multiItemOp) {
        // send the mouse press to other selected items
        // (note that these are not allowed to modify item sets, nor requesting items to be copied,
        // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
        foreach (const QSharedPointer<DrawingItemBase> item, selItems)
          if (item != hitItem_) {
            rpn = false;
            Editing(item.data())->mousePress(event, rpn);
            if (rpn)
              repaintNeeded_ = true;
          }
      }
    } else {
      // the hit item removed itself as a result of the mouse press and it makes no sense
      // to send the mouse press to other items
    }
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
  else if (aborted)
    abortEditing();

  if (rpn)
    repaintNeeded_ = true;
}

void EditItemManager::mouseRelease(QMouseEvent *event)
{
  if (layerMgr_->selectedLayers().isEmpty()) // skip if no layers are selected
    return;

  if (hasIncompleteItem()) {
    incompleteMouseRelease(event);
    return;
  }

  repaintNeeded_ = false;

  // send to selected items
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true))
    Editing(item.data())->mouseRelease(event, repaintNeeded_);

  pushModifyItemsCommand();
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

    adjustSelectedJoinPoints();
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
    if (incompleteItem_->hit(event->pos(), false))
      hitItems_ = QList<QSharedPointer<DrawingItemBase> >() << incompleteItem_;
  } else {
    bool rpn = false;
    Editing(incompleteItem_.data())->incompleteMouseMove(event, rpn);
    if (rpn) repaintNeeded_ = true;
  }

  if (hitItems_ != origHitItems)
    repaintNeeded_ = true;
}

void EditItemManager::mouseDoubleClick(QMouseEvent *event)
{
  if (hasIncompleteItem()) {
    incompleteMouseDoubleClick(event);
    return;
  }

  const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(event->pos());
  if (!hitItems.empty()) {
    bool rpn = false;
    Editing(hitItems.first().data())->mouseDoubleClick(event, rpn);
    if (rpn) repaintNeeded_ = true;
  }
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
  if (layerMgr_->selectedLayers().isEmpty())
    return;

  if (hasIncompleteItem()) {
    incompleteKeyPress(event);
    return;
  }

  if (event->key() == Qt::Key_Escape)
    return;

  const QSet<QSharedPointer<DrawingItemBase> > origSelItems = layerMgr_->itemsInSelectedLayers(true);
  QSet<int> origSelIds;
  foreach (const QSharedPointer<DrawingItemBase> item, origSelItems)
    origSelIds.insert(item->id());

  // process each of the originally selected items
  foreach (int origSelId, origSelIds) {

    // at this point, the item may or may not exist (it may have been removed in an earlier iteration)

    QSharedPointer<DrawingItemBase> origSelItem = idToItem(origSelItems, origSelId);
    if (!origSelItem.isNull()) {
      // it still exists, so pass the event
      bool rpn = false;
      Editing(origSelItem.data())->keyPress(event, rpn);
      Q_UNUSED(rpn); // ### for now

      adjustSelectedJoinPoints();
    }
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

void EditItemManager::plot(bool under, bool over)
{
  if (!over)
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  glTranslatef(editRect_.x1, editRect_.y1, 0.0);
  glScalef(PLOTM->getStaticPlot()->getPhysToMapScaleX(),
      PLOTM->getStaticPlot()->getPhysToMapScaleY(), 1.0);

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

  // Find only selected items in selected layers.
  const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);

  QList<QSharedPointer<DrawingItemBase> > hitItems;
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers()) {
    if ((!itemsVisibilityForced_) && (!item->property("visible", true).toBool()))
      continue;
    if (item->hit(pos, selItems.contains(item)))
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
    pushModifyItemsCommand();

    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
  }
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
  a[Cut] = cutAction_;
  a[Copy] = copyAction_;
  a[Paste] = pasteAction_;
  a[EditProperties] = editPropertiesAction_;
  a[EditStyle] = editStyleAction_;
  a[Undo] = undoAction_;
  a[Redo] = redoAction_;
  a[Select] = selectAction_;
  a[CreatePolyLine] = createPolyLineAction_;
  a[CreateSymbol] = createSymbolAction_;
  a[CreateText] = createTextAction_;
  a[CreateComposite] = createCompositeAction_;
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
  foreach (QVariant item, data.at(0).toList()) {
    const QSharedPointer<DrawingItemBase> ditem = item.value<QSharedPointer<DrawingItemBase> >();
    DrawingStyleManager::instance()->setStyle(ditem.data(), DrawingStyleManager::instance()->getStyle(ditem->category(), newType));
    ditem->setProperty("style:type", newType);
  }
}

void EditItemManager::updateActions()
{
  const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
  cutAction_->setEnabled(selItems.size() > 0);
  copyAction_->setEnabled(selItems.size() > 0);
  pasteAction_->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
  editPropertiesAction_->setEnabled(selItems.size() == 1);
  editStyleAction_->setEnabled(selItems.size() > 0);
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
      props.insert("Placemark:name", item->property("Placemark:name").toString());
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

QString EditItemManager::plotElementTag() const
{
  return "EDITDRAWING";
}

// Updates joins by:
// - updating the join count of joined items, and
// - ensuring that joined end points coincide (skipped if \a updateJoinCountsOnly is true).
void EditItemManager::updateJoins(bool updateJoinCountsOnly)
{
  QHash<int, QList<DrawingItemBase *> > joins;

  // find all joins
  foreach (const QSharedPointer<EditItems::Layer> &layer, layerMgr_->orderedLayers()) {
    foreach (const QSharedPointer<DrawingItemBase> &item, layer->items()) {
      const int joinId = item->joinId();
      if (joinId)
        joins[qAbs(joinId)].append(item.data());
    }
  }

  foreach (const QList<DrawingItemBase *> &join, joins.values()) {

    // update join counts
    foreach (DrawingItemBase *item, join)
      item->setJoinCount(join.size());

    // ensure that end points of existing joins coincide
    if ((!updateJoinCountsOnly) && (join.size() > 1)) {

      // get joined end points
      QVector<QPointF> points;
      foreach (DrawingItemBase *item, join)
        points.append((item->joinId() < 0) ? item->getPoints().first() : item->getPoints().last());

      // select join point
      QPointF joinPoint(QPolygonF(points).boundingRect().center()); // use center of bounding rect by default
      foreach (DrawingItemBase *item, join)
        if (Editing(item)->hoverPos() != QPoint(-1, -1)) { // use end point of hovered item instead
          joinPoint = (item->joinId() < 0) ? item->getPoints().first() : item->getPoints().last();
          break;
        }

      // move joined end points to join point
      foreach (DrawingItemBase *item, join)
        Editing(item)->movePointTo((item->joinId() < 0) ? 0 : item->getPoints().size() - 1, joinPoint);
    }
  }
}

// Adjusts (by ensuring they coincide) all joined end points in joins that involve at least one selected item.
void EditItemManager::adjustSelectedJoinPoints()
{
  // Algorithm:
  // 1: Find all joins that involve at least one selected item (note that any hit item is assumed to be selected).
  // 2: For each join:
  //    case 1: the join contains any hit item: Move the joined end point of all other items to the one of the hit item.
  //    case 2: <otherwise>: Move the joined end points of all unselected items to the joined end point of an arbitrary selected item.

  QHash<int, QList<DrawingItemBase *> > unselJoins; // the unselected items in each join
  QHash<int, QList<DrawingItemBase *> > selJoins; // the selected items in each join

  // find all joins, separating unselected and selected items in each join
  foreach (const QSharedPointer<EditItems::Layer> &layer, layerMgr_->orderedLayers()) {
    foreach (const QSharedPointer<DrawingItemBase> &item, layer->items()) {
      const int absJoinId = qAbs(item->joinId());
      if (absJoinId) {
        if (item->selected())
          selJoins[absJoinId].append(item.data());
        else
          unselJoins[absJoinId].append(item.data());
      }
    }
  }

  const int hitJoinId = hitItem_.isNull() ? 0 : hitItem_->joinId();

  // loop over joins involving at least one selected item
  foreach (int absJoinId, selJoins.keys()) {
    if (absJoinId == qAbs(hitJoinId)) { // the hit item is part of this join
      // move the joined end points in this join to the joined end point of the hit item
      const QPointF joinPoint = (hitJoinId < 0) ? hitItem_->getPoints().first() : hitItem_->getPoints().last();
      foreach (DrawingItemBase *item, selJoins.value(absJoinId))
        Editing(item)->movePointTo((item->joinId() < 0) ? 0 : item->getPoints().size() - 1, joinPoint);
      if (unselJoins.contains(absJoinId)) {
        foreach (DrawingItemBase *item, unselJoins.value(absJoinId))
          Editing(item)->movePointTo((item->joinId() < 0) ? 0 : item->getPoints().size() - 1, joinPoint);
      }
    } else { // the hit item is not part of this join
      // move the joined end points of the unselected items in this join to the joined end point of an arbitrary selected item
      DrawingItemBase *firstSelItem = selJoins.value(absJoinId).first();
      const QPointF joinPoint = (firstSelItem->joinId() < 0) ? firstSelItem->getPoints().first() : firstSelItem->getPoints().last();
      if (unselJoins.contains(absJoinId)) {
        foreach (DrawingItemBase *item, unselJoins.value(absJoinId))
          Editing(item)->movePointTo((item->joinId() < 0) ? 0 : item->getPoints().size() - 1, joinPoint);
      }
    }
  }
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

void EditItemManager::cutSelectedItems()
{
  const QSet<QSharedPointer<DrawingItemBase> > items = layerMgr_->itemsInSelectedLayers(true);
  copyItems(items);
  foreach (const QSharedPointer<DrawingItemBase> item, items)
    removeItem(item);

  // ### the following is necessary only if items were actually removed
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
      if (item) {
        item->setSelected();
        item->propertiesRef().insert("joinId", 0);
        addItem(item, false);
      } else {
        QMessageBox::warning(0, "Error", error);
      }
    }
  }

  // ### the following is necessary only if items were actually added
  layerMgr_->deselectAllItems();
  updateActionsAndTimes();
}

// Joins currently selected items.
void EditItemManager::joinSelectedItems()
{
  const QList<QSharedPointer<DrawingItemBase> > items = layerMgr_->itemsInSelectedLayers(true).values();

  const int n = items.size();
  if (n < 2)
    return; // joining fewer than two items makes no sense
  const int limit = 10;
  if (n > limit) { // limit combinatorial explosion
    QMessageBox::warning(0, "Error", QString("at most %1 items may be joined together").arg(limit));
    return;
  }

  QRectF minBRect;
  qreal minBRectDim = -1;
  int minCombo = -1;

  // loop over end point combinations
  const int nc = qPow(2, n);
  for (int c = 0; c < nc; ++c) {

    // compute smallest bounding rect (in any dimension) for this combination and update minimum values
    QVector<QPointF> points;
    for (int i = 0; i < n; ++i) {
      const bool first = (1 << i) & c;
      points.append(first ? items.at(i)->getPoints().first() : items.at(i)->getPoints().last());
    }
    const QRectF brect = QPolygonF(points).boundingRect();
    const qreal maxBRectDim = qMax(brect.width(), brect.height());
    if ((minBRectDim < 0) || (maxBRectDim < minBRectDim)) {
      minBRect = brect;
      minBRectDim = maxBRectDim;
      minCombo = c;
    }
  }

  QBitArray first(n);
  for (int i = 0; i < n; ++i)
    if ((1 << i) & minCombo)
      first.setBit(i);

  // select join point
  QPointF joinPoint(minBRect.center()); // use center of bounding rect by default
  for (int i = 0; i < n; ++i)
    if (Editing(items.at(i).data())->hoverPos() != QPoint(-1, -1)) { // use end point of hovered item instead
      joinPoint = first.testBit(i) ? items.at(i)->getPoints().first() : items.at(i)->getPoints().last();
      break;
    }

  // move joined end points to join point and register new join ID
  QList<int> oldJoinIds;
  QList<int> newJoinIds;
  const int absNewJoinId = EditItemManager::instance()->nextJoinId();
  for (int i = 0; i < n; ++i) {
    oldJoinIds.append(items.at(i)->joinId());
    const int newJoinId = first.testBit(i) ? -absNewJoinId : absNewJoinId;
    newJoinIds.append(newJoinId);
    items.at(i)->propertiesRef().insert("joinId", newJoinId);
    Editing(items.at(i).data())->movePointTo(first.testBit(i) ? 0 : (items.at(i)->getPoints().size() - 1), joinPoint);
  }

  updateJoins(true);
}

// Unjoins currently selected items.
void EditItemManager::unjoinSelectedItems()
{
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true).values())
    item->propertiesRef().insert("joinId", 0);
  updateJoins(true);
}

void EditItemManager::toggleReversedForSelectedItems()
{
  foreach (const QSharedPointer<DrawingItemBase> &item, layerMgr_->itemsInSelectedLayers(true).values()) {
    const QVariantMap style = DrawingStyleManager::instance()->getStyle(item.data());
    const bool reversed = style.value(DSP_reversed::name()).toBool();
    item->setProperty("style:reversed", !reversed);
  }
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
  selectAction_->setChecked(true);
  emit unsetWorkAreaCursor();
}

void EditItemManager::setCreatePolyLineMode()
{
  abortEditing();
  mode_ = CreatePolyLineMode;
  createPolyLineAction_->setChecked(true);
  const QCursor cursor = QCursor(QPixmap(paint_create_polyline_xpm), 4, 4);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateSymbolMode()
{
  abortEditing();
  mode_ = CreateSymbolMode;
  createSymbolAction_->setChecked(true);
  const QCursor cursor = QCursor(QPixmap(paint_create_symbol_xpm), 4, 4);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateTextMode()
{
  abortEditing();
  mode_ = CreateTextMode;
  createTextAction_->setChecked(true);
  const QCursor cursor(Qt::IBeamCursor);
  emit setWorkAreaCursor(cursor);
}

void EditItemManager::setCreateCompositeMode()
{
  abortEditing();
  mode_ = CreateCompositeMode;
  createCompositeAction_->setChecked(true);
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
  if (!isEditing())
    return;

  event->ignore();
  res.savebackground= true;   // Save the background after painting.
  res.background= false;      // Don't paint the background.
  res.repaint= false;
  //res.newcursor= edit_cursor;
  res.newcursor= keep_it;

  if (layerMgr_->selectedLayers().isEmpty()) // skip if no layers are selected
    return;

  // Transform the mouse position into the original coordinate system used for the objects.
  int w, h;
  PLOTM->getPlotWindow(w, h);
  const Rectangle& plotRect_ = PLOTM->getPlotSize();

  if (layerMgr_->selectedLayersItemCount() == 0)
    setEditRect(PLOTM->getPlotSize());

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates. This gives us displaced screen coordinates - these
  // are coordinates relative to the original edit rectangle.
  float dx = (plotRect_.x1 - editRect_.x1) * (w/plotRect_.width());
  float dy = (plotRect_.y1 - editRect_.y1) * (h/plotRect_.height());

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


  if (event->type() == QEvent::MouseButtonPress) {

    const QSet<QSharedPointer<DrawingItemBase> > origSelItems = layerMgr_->itemsInSelectedLayers(true);

    if (!hasIncompleteItem())
      saveItemStates(); // record current item states

    if ((me2.button() == Qt::RightButton) && !hasIncompleteItem()) {
      // open a context menu

      // get actions contributed by a hit item (if any)
      const QList<QSharedPointer<DrawingItemBase> > hitItems = findHitItems(me2.pos());
      QSharedPointer<DrawingItemBase> hitItem; // consider only this item to be hit
      if (!hitItems.empty())
        hitItem = hitItems.first();

      QList<QAction *> hitItemActions;
      if (!hitItem.isNull()) {
        selectItem(hitItem);
        emit repaintNeeded();
        hitItemActions = Editing(hitItem.data())->actions(me2.pos());
      }

      // populate the menu
      QList<QSharedPointer<DrawingItemBase> > selectedItems = layerMgr_->itemsInSelectedLayers(true).toList();
      QSet<DrawingItemBase::Category> selectedCategories;
      foreach (const QSharedPointer<DrawingItemBase> &item, selectedItems)
        selectedCategories.insert(item->category());

      QMenu contextMenu;
      contextMenu.addAction(copyAction_);
      contextMenu.addAction(cutAction_);
      contextMenu.addAction(pasteAction_);
      pasteAction_->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));

      contextMenu.addSeparator();
      contextMenu.addAction(joinAction_);
      joinAction_->setEnabled(selectedItems.size() >= 2);
      contextMenu.addAction(unjoinAction_);
      unjoinAction_->setEnabled(false);
      foreach (const QSharedPointer<DrawingItemBase> &item, selectedItems) {
        if (item->joinId() && (item->joinCount() > 0)) {
          unjoinAction_->setEnabled(true);
          break;
        }
      }

      contextMenu.addSeparator();
      contextMenu.addAction(toggleReversedAction_);
      toggleReversedAction_->setEnabled(selectedItems.size() >= 1);

      contextMenu.addSeparator();
      contextMenu.addAction(editPropertiesAction_);
      editPropertiesAction_->setEnabled(selectedItems.size() == 1);
      contextMenu.addAction(editStyleAction_);
      editStyleAction_->setEnabled(!selectedItems.isEmpty() && !selectedCategories.contains(DrawingItemBase::Composite));

      QMenu styleTypeMenu;
      styleTypeMenu.setTitle("Convert");
      // Disable conversion if only composite items are selected.
      styleTypeMenu.setEnabled((selectedCategories.size() == 1) && !selectedCategories.contains(DrawingItemBase::Composite));

      if (styleTypeMenu.isEnabled()) {

        // Obtain a list of style names for the first category of the selected items.
        QStringList styleTypes = DrawingStyleManager::instance()->styles(*(selectedCategories.begin()));

        // Filter out styles from different sections - this only makes sense
        // for symbols at the moment.
        QSet<QString> sections;
        foreach (QSharedPointer<DrawingItemBase> item, selectedItems) {
          QStringList pieces = item->property("style:type").toString().split("|");
          if (pieces.size() != 1)
            sections.insert(pieces.first());
        }
        QStringList filteredTypes;
        foreach (QString styleType, styleTypes) {
          QStringList pieces = styleType.split("|");
          if (pieces.size() == 1)
            filteredTypes.append(styleType);
          else if (sections.contains(pieces.first()))
            filteredTypes.append(styleType);
        }

        qSort(filteredTypes);

        // Add each of the available style types to the menu.
        foreach (QString styleType, filteredTypes) {
          QString styleName = styleType.split("|").last();
          QAction *action = new QAction(QString("%1 %2").arg(tr("To")).arg(styleName), 0);
          QVariantList styleItems;
          foreach (const QSharedPointer<DrawingItemBase> styleItem, selectedItems)
            styleItems.append(QVariant::fromValue(styleItem));

          // Pack the selected items and the style type for this menu entry
          // into a list to be sent via the triggered signal if this entry
          // is selected.
          QVariantList data;
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

      pushModifyItemsCommand();

    } else {

      // create a new item if necessary
      if ((me2.button() == Qt::LeftButton) && !hasIncompleteItem()) {
        QSharedPointer<DrawingItemBase> item;

        if (mode_ == CreatePolyLineMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_PolyLine::PolyLine()));
          addItem(item, true);
          item->setProperty("style:type", createPolyLineAction_->data().toString());
        } else if (mode_ == CreateSymbolMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Symbol::Symbol()));
          addItem(item, true);
          item->setProperty("style:type", createSymbolAction_->data().toString());
        } else if (mode_ == CreateCompositeMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Composite::Composite()));
          addItem(item, true);
          item->setProperty("style:type", createCompositeAction_->data().toString());
        } else if (mode_ == CreateTextMode) {
          item = QSharedPointer<DrawingItemBase>(Drawing(new EditItem_Text::Text()));
          addItem(item, true);
          item->setProperty("style:type", createTextAction_->data().toString());
        }

        if (!item.isNull())
          DrawingStyleManager::instance()->setStyle(
                item.data(), DrawingStyleManager::instance()->getStyle(item->category(), item->propertiesRef().value("style:type").toString()));
      }

      // process the event further (delegating it to relevant items etc.)
      mousePress(&me2);

      if (needsRepaint() && !hasIncompleteItem())
        emitItemChanged();
    }

    const QSet<QSharedPointer<DrawingItemBase> > selItems = layerMgr_->itemsInSelectedLayers(true);
    if (selItems != origSelItems) {
      emit selectionChanged();
      repaintNeeded_ = true;
    }

    event->accept();

  } else if (event->type() == QEvent::MouseMove) {
    mouseMove(&me2);
    event->accept();
  }

  else if (event->type() == QEvent::MouseButtonRelease) {
    mouseRelease(&me2);
    event->accept();
    if (needsRepaint() && !hasIncompleteItem())
      emitItemChanged();
  }

  else if (event->type() == QEvent::MouseButtonDblClick) {
    mouseDoubleClick(&me2);
    event->accept();
  }

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

// Saves the state of all items in the current layer structure.
void EditItemManager::saveItemStates()
{
  oldItemStates_ = getLayerManager()->copyItemStates(this);
}

static void addDiffsToDescr(QString &descr, const QString &format, int nDiffs)
{
  if (nDiffs > 0)
    descr += format.arg(descr.isEmpty() ? "" : ", ").arg(nDiffs).arg((nDiffs == 1) ? "" : "s");
}

static bool fuzzyEqual(const qreal v1, const qreal v2)
{
  const qreal tolerance = 0.0001; // ### may need to be tuned
  return qAbs(v1 - v2) < tolerance;
}

static bool fuzzyEqual(const QList<QPointF> &p1, const QList<QPointF> &p2)
{
  if (p1.size() != p2.size())
    return false;

  for (int i = 0; i < p1.size(); ++i)
    if ((!fuzzyEqual(p1.at(i).x(), p2.at(i).x())) || (!fuzzyEqual(p1.at(i).y(), p2.at(i).y())))
      return false;

  return true;
}

// Returns true iff \a oldItemStates is considered equal to \a newItemStates.
// Upon returning false, \a descr provides details about the difference.
static bool itemStatesEqual(
    const QList<QList<QSharedPointer<DrawingItemBase> > > &oldItemStates,
    const QList<QList<QSharedPointer<DrawingItemBase> > > &newItemStates,
    QString &descr)
{
  if (oldItemStates.size() != newItemStates.size()) {
    descr = QString("oldItemStates.size() (%1) != newItemStates.size() (%2) (should not happen!)")
        .arg(oldItemStates.size()).arg(newItemStates.size());
    return false;
  }

  descr = QString();

  int nAdded = 0;
  int nRemoved = 0;
  int nGeomChanges = 0; // # of items with ... changed geometry
  int nPropChanges = 0; // ... changes in properties
  int nSelChanges = 0; // ... changed selection state
  int nJoinCountChanges = 0; // ... changed join counts
  int nOtherStateChanges = 0; // ... changes in other supported state

  for (int i = 0; i < oldItemStates.size(); ++i) { // loop over layers
    QSet<int> oldIds;
    QHash<int, QSharedPointer<DrawingItemBase> > oldItems;
    foreach (const QSharedPointer<DrawingItemBase> &item, oldItemStates.at(i)) {
      oldItems.insert(item->id(), item);
      oldIds.insert(item->id());
    }

    QSet<int> newIds;
    QHash<int, QSharedPointer<DrawingItemBase> > newItems;
    foreach (const QSharedPointer<DrawingItemBase> &item, newItemStates.at(i)) {
      newItems.insert(item->id(), item);
      newIds.insert(item->id());
    }

    // check for added or removed items
    nAdded += (newIds - oldIds).size();
    nRemoved += (oldIds - newIds).size();

    // check for changed items
    foreach (int id, oldIds.intersect(newIds)) {
      const QSharedPointer<DrawingItemBase> oldItem = oldItems.value(id);
      const QSharedPointer<DrawingItemBase> newItem = newItems.value(id);
      if (!fuzzyEqual(oldItem->getPoints(), newItem->getPoints()))
        nGeomChanges++;
      if (oldItem->properties() != newItem->properties())
        nPropChanges++;
      if (oldItem->selected() != newItem->selected())
        nSelChanges++;
      if (oldItem->joinCount() != newItem->joinCount())
        nJoinCountChanges++;
    }
  }

  // set description string to reflect changes
  addDiffsToDescr(descr, "%1add %2 item%3", nAdded);
  addDiffsToDescr(descr, "%1remove %2 item%3", nRemoved);
  addDiffsToDescr(descr, "%1change geometry of %2 item%3", nGeomChanges);
  addDiffsToDescr(descr, "%1change properties of %2 item%3", nPropChanges);
  addDiffsToDescr(descr, "%1change selection of %2 item%3", nSelChanges);
  addDiffsToDescr(descr, "%1change join counts of %2 item%3", nJoinCountChanges);
  addDiffsToDescr(descr, "%1change other state of %2 item%3", nOtherStateChanges);

  return descr.isEmpty();
}

// Pushes, if necessary, a ModifyItemsCommand on the undo stack to reflect changes made to items since the last call to saveItemStates().
void EditItemManager::pushModifyItemsCommand()
{
  const QList<QList<QSharedPointer<DrawingItemBase> > > newItemStates = getLayerManager()->copyItemStates(this);

  QString descr;
  if (!itemStatesEqual(oldItemStates_, newItemStates, descr)) {
    undoStack_.push(new ModifyItemsCommand(oldItemStates_, newItemStates, descr));
    // ensure that only the first call to pushModifyItemsCommand() after a call to saveItemStates() has effect:
    oldItemStates_ = newItemStates;
  }
}

void EditItemManager::sendKeyboardEvent(QKeyEvent *event, EventResult &res)
{
  event->ignore();
  res.savebackground= true;   // Save the background after painting.
  res.background= false;      // Don't paint the background.
  res.repaint= false;

  if (layerMgr_->selectedLayers().isEmpty()) // skip if no layers are selected
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

  if (event->type() == QEvent::KeyPress) {

    saveItemStates(); // record current item states

    if (cutAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      cutSelectedItems();
    } else if (copyAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      copySelectedItems();
    } else if (pasteAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      pasteItems();
    } else if (joinAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      joinSelectedItems();
    } else if (unjoinAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      unjoinSelectedItems();
    } else if (toggleReversedAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      toggleReversedForSelectedItems();
    } else if (editPropertiesAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      editProperties();
    } else if (event->modifiers().testFlag(Qt::NoModifier) && ((event->key() == Qt::Key_PageUp) || (event->key() == Qt::Key_PageDown))) {
      if (cycleHitOrder(event))
        return;
    } else if (event->modifiers().testFlag(Qt::NoModifier) && (event->key() == Qt::Key_Escape)) {
      setSelectMode();
      return;
    }
  } else {
    return;
  }

  if (!event->isAccepted())
    keyPress(event);

  repaintNeeded_ = true; // ### for now

  pushModifyItemsCommand();

  updateActionsAndTimes();

  if (event->isAccepted() || needsRepaint())
    emitItemChanged();
}

// Command classes

ModifyItemsCommand::ModifyItemsCommand(
    const QList<QList<QSharedPointer<DrawingItemBase> > > &oldItemStates,
    const QList<QList<QSharedPointer<DrawingItemBase> > > &newItemStates,
    const QString &t)
  : oldItemStates_(oldItemStates)
  , newItemStates_(newItemStates)
{
  setText(t);
}

void ModifyItemsCommand::undo()
{
  EditItemManager::instance()->getLayerManager()->replaceItemStates(oldItemStates_, EditItemManager::instance());
}

void ModifyItemsCommand::redo()
{
  EditItemManager::instance()->getLayerManager()->replaceItemStates(newItemStates_, EditItemManager::instance());
}
