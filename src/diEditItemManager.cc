/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2019 met.no

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

#include "diEditItemManager.h"

#include "EditItems/drawingstylemanager.h"
#include "EditItems/editcomposite.h"
#include "EditItems/edititembase.h"
#include "EditItems/editpolyline.h"
#include "EditItems/editsymbol.h"
#include "EditItems/edittext.h"
#include "EditItems/itemgroup.h"
#include "EditItems/kml.h"
#include "EditItems/properties.h"
#include "EditItems/toolbar.h"
#include "diEventResult.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diPlotModule.h"
#include "diStaticPlot.h"
#include "miSetupParser.h"
#include "qtMainWindow.h"

#include <puTools/miStringFunctions.h>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolTip>

#define MILOGGER_CATEGORY "diana.EditItemManager"
#include <miLogger/miLogging.h>

#include "paint_select2.xpm"
#include "paint_create_polyline.xpm"
#include "paint_create_symbol.xpm"
#include "paint_create_text.xpm"
#include "paint_create_composite.xpm"

#define PLOTM PlotModule::instance()

using namespace miutil;

namespace {
EditItems::ItemGroup* scratchGroup(const QMap<QString, EditItems::ItemGroup *>& groups)
{
  const QMap<QString, EditItems::ItemGroup *>::const_iterator itS = groups.find("scratch");
  if (itS != groups.end())
    return itS.value();
  else
    return 0;
}
} // namespace

class UndoView : public QUndoView
{
public:
  UndoView(QUndoStack *undoStack) : QUndoView(undoStack) {}
private:
  void keyPressEvent(QKeyEvent *event) override
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
  : incompleteItem_(0)
  , selectingOnly_(true)
  , repaintNeeded_(false)
  , skipRepaint_(false)
  , hitOffset_(0)
  , itemChangeNotificationEnabled_(false)
{
  // Create a default inactive layer group.
  itemGroups_["scratch"] = new EditItems::ItemGroup("scratch", true, false);

  connect(this, SIGNAL(selectionChanged()), SLOT(handleSelectionChange()));
  connect(this, SIGNAL(incompleteEditing(bool)), SLOT(startStopEditing(bool)));

  undoStack_.setUndoLimit(1000); // ### may need to be tuned

  connect(&undoStack_, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
  connect(&undoStack_, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
  connect(&undoStack_, SIGNAL(indexChanged(int)), this, SLOT(repaint()));

  selectAllAction_ = new QAction(tr("Select All"), this);
  copyAction_ = new QAction(tr("Copy"), this);
  copyAction_->setShortcut(QKeySequence::Copy);
  cutAction_ = new QAction(tr("Cut"), this);
  cutAction_->setShortcut(tr("Ctrl+X"));
  pasteAction_ = new QAction(tr("Paste"), this);
  pasteAction_->setShortcut(QKeySequence::Paste);
  lowerAction_ = new QAction(tr("Lower"), this);
  lowerAction_->setShortcut(tr("Ctrl+PgDown"));
  raiseAction_ = new QAction(tr("Raise"), this);
  raiseAction_->setShortcut(tr("Ctrl+PgUp"));
  joinAction_ = new QAction(tr("Join"), this);
  joinAction_->setShortcut(QString("J"));
  unjoinAction_ = new QAction(tr("Unjoin"), this);
  unjoinAction_->setShortcut(tr("Ctrl+J"));
  toggleReversedAction_ = new QAction(tr("Toggle Reversed"), this);
  toggleReversedAction_->setShortcut(QString("R"));
  editPropertiesAction_ = new QAction(tr("&Edit..."), this);
  editPropertiesAction_->setShortcut(tr("Ctrl+R"));
  undoAction_ = undoStack_.createUndoAction(this);
  undoAction_->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowBack));
  redoAction_ = undoStack_.createRedoAction(this);
  redoAction_->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowForward));

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

  createCompositeAction_ = new QAction(QPixmap(paint_create_composite_xpm), tr("SIGWX"), this);
  createCompositeAction_->setCheckable(true);

  connect(selectAllAction_, SIGNAL(triggered()), SLOT(selectAllItems()));
  connect(copyAction_, SIGNAL(triggered()), SLOT(copySelectedItems()));
  connect(cutAction_, SIGNAL(triggered()), SLOT(cutSelectedItems()));
  connect(pasteAction_, SIGNAL(triggered()), SLOT(pasteItems()));
  connect(lowerAction_, SIGNAL(triggered()), SLOT(lowerSelectedItems()));
  connect(raiseAction_, SIGNAL(triggered()), SLOT(raiseSelectedItems()));
  connect(joinAction_, SIGNAL(triggered()), SLOT(joinSelectedItems()));
  connect(unjoinAction_, SIGNAL(triggered()), SLOT(unjoinSelectedItems()));
  connect(toggleReversedAction_, SIGNAL(triggered()), SLOT(toggleReversedForSelectedItems()));
  connect(editPropertiesAction_, SIGNAL(triggered()), SLOT(editProperties()));
  connect(selectAction_, SIGNAL(triggered()), SLOT(setSelectMode()));
  connect(createPolyLineAction_, SIGNAL(triggered()), SLOT(setCreatePolyLineMode()));
  connect(createSymbolAction_, SIGNAL(triggered()), SLOT(setCreateSymbolMode()));
  connect(createTextAction_, SIGNAL(triggered()), SLOT(setCreateTextMode()));
  connect(createCompositeAction_, SIGNAL(triggered()), SLOT(setCreateCompositeMode()));

  connect(DrawingManager::instance(), SIGNAL(itemsHovered(const QList<DrawingItemBase *> &)),
          SLOT(showItemInformation(const QList<DrawingItemBase *> &)));
  connect(this, SIGNAL(itemsHovered(const QList<DrawingItemBase *> &)),
                SLOT(showItemInformation(const QList<DrawingItemBase *> &)));

  setSelectMode();
  setEnabled(true);
  updateActionsAndTimes();
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
  std::vector<std::string> section;

  if (!SetupParser::getSection("DRAWING", section))
    METLIBS_LOG_WARN("No DRAWING section.");

  for (unsigned int i = 0; i < section.size(); ++i) {

    // Split the line into tokens.
    std::vector<std::string> tokens = miutil::split_protected(section[i], '\"', '\"', " ", true);
    QHash<QString, QString> items;

    for (unsigned int j = 0; j < tokens.size(); ++j) {
      std::string key, value;
      SetupParser::splitKeyValue(tokens[j], key, value);
      items[QString::fromStdString(key)] = QString::fromStdString(value);
    }

    // Check for different types of definition.
    if (items.contains("hide-property-sections-drawing")) {
      QStringList values = items.value("hide-property-sections-drawing").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("hide-drawing", values);
    }
    if (items.contains("hide-property-sections-editing")) {
      QStringList values = items.value("hide-property-sections-editing").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("hide-editing", values);
    }
    if (items.contains("show-property-sections-drawing")) {
      QStringList values = items.value("show-property-sections-drawing").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("show-drawing", values);
    }
    if (items.contains("show-property-sections-editing")) {
      QStringList values = items.value("show-property-sections-editing").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("show-editing", values);
    }
    if (items.contains("common-properties")) {
      QStringList values = items.value("common-properties").split(",");
      Properties::PropertiesEditor::instance()->setPropertyRules("common-properties", values);
    }
    if (items.contains("show-tooltips-drawing")) {
      QStringList values = items.value("show-tooltips-drawing").split(",");
      tooltipDrawingProperties = values.toSet();
    }
    if (items.contains("show-tooltips-editing")) {
      QStringList values = items.value("show-tooltips-editing").split(",");
      tooltipEditingProperties = values.toSet();
    }
    if (items.contains("tooltips-merge-identical")) {
      QStringList values = items.value("tooltips-merge-identical").split(",");
      foreach (const QString &value, values) {
        QStringList pieces = value.split("|");
        if (pieces.size() > 2) {
          QString replacement = pieces.takeLast();
          tooltipMergeRules[replacement] = pieces;
        }
      }
    }
  }

  // Let the base class parse the section of the setup file.
  return DrawingManager::parseSetup();
}

void EditItemManager::setEditing(bool enable)
{
  Manager::setEditing(enable);

  // Enable the scratch layer if editing is enabled; otherwise disable it.
  scratchGroup(itemGroups_)->setActive(enable);

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
bool EditItemManager::processInput(const PlotCommand_cpv&)
{
  return false;
}

// Adds an item to the scene. \a incomplete indicates whether the item is in the process of being manually placed.
void EditItemManager::addItem(DrawingItemBase *item, bool incomplete, bool skipRepaint)
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
      setFromLatLonPoints(item, item->getLatLonPoints()); // obtain screen coords from geo coords

    scratchGroup(itemGroups_)->addItem(item);
    emit itemAdded(item);
  }

  if (!skipRepaint)
    repaint();
}

DrawingItemBase *EditItemManager::createItem(const QString &type)
{
  EditItemBase *item = 0;
  if (type == "PolyLine")
    item = new EditItem_PolyLine::PolyLine();
  else if (type == "Symbol")
    item = new EditItem_Symbol::Symbol();
  else if (type == "Text")
    item = new EditItem_Text::Text();
  else if (type == "Composite")
    item = new EditItem_Composite::Composite();

  return Drawing(item);
}

void EditItemManager::editItem(DrawingItemBase *item)
{
  incompleteItem_ = item;
  emit incompleteEditing(true);
}

void EditItemManager::removeItem(DrawingItemBase *item)
{
  // Convert screen coords to geo coords in preparation for being stored in
  // an undo command.
  item->setLatLonPoints(getLatLonPoints(item));

  scratchGroup(itemGroups_)->removeItem(item);

  hitItems_.removeOne(item);
  deselectItem(item);

  removedItems_[item->id()] = item;

  updateJoins();
  emit itemRemoved(item->id());
  update();
}

void EditItemManager::updateItem(DrawingItemBase *item, const QVariantMap &props)
{
  QMap<QString, QVariant>::const_iterator it;

  for (it = props.begin(); it != props.end(); ++it)
    item->setProperty(it.key(), it.value());

  emit itemChanged(item->properties());
}

void EditItemManager::reset()
{
  undoStack_.clear();
}

void EditItemManager::mousePress(QMouseEvent *event)
{
  if (hasIncompleteItem()) {
    incompleteMousePress(event);
    return;
  }

  QSet<DrawingItemBase *> selItems = selectedItems().toSet();

  // TODO why not use hitItems_, which is updated in mouseMove?
  QList<DrawingItemBase *> missedItems;
  const QList<DrawingItemBase *> hitItems = findHitItems(event->pos(), hitItemTypes_, missedItems);
  DrawingItemBase * hitItem = !hitItems.empty() ? hitItems.first() : 0; // consider only this item to be hit

  const bool hitSelItem = selItems.contains(hitItem); // whether an already selected item was hit
  const bool selectMulti = event->modifiers() & Qt::ControlModifier;

  repaintNeeded_ = false;

  // update selection and hit status
  if (!(hitSelItem || (hitItem && selectMulti))) {
    deselectAllItems();
  } else if (selectMulti && hitSelItem && (selItems.size() > 1)) {
    deselectItem(hitItem);
  }

  if (hitItem) { // an item is still considered hit
    selectItem(hitItem); // ensure the hit item is selected (it might already be)

    // send mouse press to the hit item
    bool multiItemOp = false;

    bool rpn = false;
    Editing(hitItem)->mousePress(event, rpn, &multiItemOp);
    if (rpn)
      repaintNeeded_ = true;

    if (multiItemOp) {
      // send the mouse press to other selected items
      // (note that these are not allowed to modify item sets, nor requesting items to be copied,
      // nor does it make sense for them to flag the event as the beginning of a potential multi-item operation)
      foreach (DrawingItemBase *item, selItems) {
        if (item != hitItem) {
          rpn = false;
          Editing(item)->mousePress(event, rpn);
          if (rpn)
            repaintNeeded_ = true;
        }
      }
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
  Editing(incompleteItem_)->incompleteMousePress(event, rpn, completed, aborted);

  // Record geographic coordinates for the item as they are added to it.
  incompleteItem_->setLatLonPoints(getLatLonPoints(incompleteItem_));
  if (completed)
    completeEditing();
  else if (aborted)
    abortEditing();

  if (rpn)
    repaintNeeded_ = true;
}

void EditItemManager::mouseRelease(QMouseEvent *event)
{
  if (hasIncompleteItem()) {
    incompleteMouseRelease(event);
    return;
  }

  repaintNeeded_ = false;

  // Send the event to the selected items.
  foreach (DrawingItemBase *item, selectedItems())
    Editing(item)->mouseRelease(event, repaintNeeded_);

  pushUndoCommands();
}

// Handles a mouse release event for an item in the process of being completed.
void EditItemManager::incompleteMouseRelease(QMouseEvent *event)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_)->incompleteMouseRelease(event, rpn, completed, aborted);
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
  // appropriate), and NOT be passed on through the mouseMove() functions of the selected items ... 2 B DONE!

  repaintNeeded_ = false;

  QList<DrawingItemBase *> origHitItems = hitItems_;
  hitItems_.clear();
  const bool hover = !event->buttons();
  bool rpn = false;

  if (hover) {
    lastHoverPos_ = event->pos();
    QList<DrawingItemBase *> missedItems;
    const QList<DrawingItemBase *> hitItems = findHitItems(lastHoverPos_, hitItemTypes_, missedItems);
    Q_FOREACH (DrawingItemBase *hitItem, hitItems)
      Editing(hitItem)->updateHoverPos(lastHoverPos_);
    Q_FOREACH (DrawingItemBase *missedItem, missedItems)
      Editing(missedItem)->updateHoverPos(QPoint(-1, -1));

    if (!hitItems.empty()) {
      hitItems_ = hitItems;

      // Send a mouse hover event to the first hover item.
      Editing(hitItem())->mouseHover(event, rpn, selectingOnly_);
      if (rpn)
        repaintNeeded_ = true;
      Q_EMIT itemsHovered(hitItems_);
    } else if (!origHitItems.isEmpty()) {
      Editing(origHitItems.first())->mouseHover(event, rpn, selectingOnly_);
      if (rpn)
        repaintNeeded_ = true;
    }
  } else {
    // send move event to all selected items
    Q_FOREACH (DrawingItemBase *item, selectedItems()) {
      Editing(item)->mouseMove(event, rpn);
      item->setLatLonPoints(getLatLonPoints(item));
      // Call the item's method to process the event as a hover event in
      // order to allow polyline items to show coordinate tooltips.
      Editing(item)->mouseHover(event, rpn);
      if (rpn)
        repaintNeeded_ = true;
    }

    adjustSelectedJoinPoints();
  }

  if (!repaintNeeded_ && hitItems_ != origHitItems)
    repaintNeeded_ = true;
}

// Handles a mouse move event for an item in the process of being completed.
void EditItemManager::incompleteMouseMove(QMouseEvent *event)
{
  Q_ASSERT(hasIncompleteItem());

  const QList<DrawingItemBase *> origHitItems = hitItems_;
  hitItems_.clear();
  const bool hover = !event->buttons();
  if (hover) {
    bool rpn = false;
    Editing(incompleteItem_)->incompleteMouseHover(event, rpn);
    incompleteItem_->setLatLonPoints(getLatLonPoints(incompleteItem_));
    if (rpn)
      repaintNeeded_ = true;
    if (incompleteItem_->hit(event->pos(), false) != DrawingItemBase::None)
      hitItems_ = QList<DrawingItemBase *>() << incompleteItem_;
  } else {
    bool rpn = false;
    Editing(incompleteItem_)->incompleteMouseMove(event, rpn);
    if (rpn)
      repaintNeeded_ = true;
  }

  if (!repaintNeeded_ && hitItems_ != origHitItems)
    repaintNeeded_ = true;
}

void EditItemManager::mouseDoubleClick(QMouseEvent *event)
{
  if (hasIncompleteItem()) {
    incompleteMouseDoubleClick(event);
    return;
  }

  QList<DrawingItemBase *> missedItems;
  const QList<DrawingItemBase *> hitItems = findHitItems(event->pos(), hitItemTypes_, missedItems);
  if (!hitItems.empty()) {
    bool rpn = false;
    Editing(hitItems.first())->mouseDoubleClick(event, rpn);
    if (rpn) repaintNeeded_ = true;
  }
}

void EditItemManager::incompleteMouseDoubleClick(QMouseEvent *event)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_)->incompleteMouseDoubleClick(event, rpn, completed, aborted);
  if (completed)
    completeEditing();
  else {
    if (aborted)
      abortEditing();
    if (rpn)
      repaintNeeded_ = true;
  }
}

void EditItemManager::keyPress(QKeyEvent *event)
{
  if (hasIncompleteItem()) {
    incompleteKeyPress(event);
    return;
  }

  if (event->key() == Qt::Key_Escape) {
    return;
  } else if ((event->key() == Qt::Key_R) && (event->modifiers() == Qt::ControlModifier)) {
    emit reloadRequested();
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Menu) {
    QPoint globalPos = QCursor::pos();
    openContextMenu(QPoint(-1, -1), globalPos);
    event->accept();
    return;
  }

  // Process each of the selected items and any currently hovered item.
  bool handledHover = false;
  DrawingItemBase *hoverItem = hitItem();

  foreach (DrawingItemBase *item, selectedItems()) {

    bool rpn = false;
    Editing(item)->keyPress(event, rpn);

    // If the item needs to be repainted, take that as a hint for us to
    // update its geographic coordinates.
    if (rpn)
      item->setLatLonPoints(getLatLonPoints(item));

    adjustSelectedJoinPoints();

    if (item == hoverItem)
      handledHover = true;
  }

  // Handle the hovered item if it wasn't a selected item.
  if (hoverItem && !handledHover) {
    bool rpn = false;
    Editing(hoverItem)->keyPress(event, rpn);

    if (rpn)
      hoverItem->setLatLonPoints(getLatLonPoints(hoverItem));

    adjustSelectedJoinPoints();
  }
}

// Handles a key press event for an item in the process of being completed.
void EditItemManager::incompleteKeyPress(QKeyEvent *event)
{
  Q_ASSERT(hasIncompleteItem());
  bool rpn = false;
  bool completed = false;
  bool aborted = false;
  Editing(incompleteItem_)->incompleteKeyPress(event, rpn, completed, aborted);
  if (completed)
    completeEditing();
  else {
    if (aborted)
      abortEditing();
    if (rpn)
      repaintNeeded_ = true;
  }
}

void EditItemManager::plot(DiGLPainter* gl, bool under, bool over)
{
  if (!over || !isEditing() || !isEnabled())
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(editRect_.x1, editRect_.y1, 0.0);
  gl->Scalef(PLOTM->getStaticPlot()->getPhysToMapScaleX(),
      PLOTM->getStaticPlot()->getPhysToMapScaleY(), 1.0);

  const QSet<DrawingItemBase *> selItems = selectedItems().toSet();

  QList<DrawingItemBase *> items = allItems();
  DrawingItemBase * hit = hitItem();

  foreach (DrawingItemBase *item, items) {
    EditItemBase::DrawModes modes = EditItemBase::Normal;
    if (isEditing()) {
      if (selItems.contains(item))
        modes |= EditItemBase::Selected;
      if (item == hit)
        modes |= EditItemBase::Hovered;
    }
    if (isItemVisible(item)) {
      applyPlotOptions(gl, item);
      setFromLatLonPoints(item, item->getLatLonPoints());
      Editing(item)->draw(gl, modes, false, Properties::PropertiesEditor::instance()->isVisible());
    }
  }

  if (hasIncompleteItem()) { // note that only complete items may be selected
    setFromLatLonPoints(incompleteItem_, incompleteItem_->getLatLonPoints());
    Editing(incompleteItem_)->draw(gl, incompleteItem_ == hit ? EditItemBase::Hovered : EditItemBase::Normal, true);
  }
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
    Q_EMIT repaintNeeded(false);
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

/**
 * Returns a list of all drawing items from all item groups. The items from
 * each group will be returned in the order in which they are stored.
 * The overall order is determined by the name of each item group.
 */
QList<DrawingItemBase *> EditItemManager::allItems() const
{
  QList<DrawingItemBase *> items;
  QMap<QString, EditItems::ItemGroup *>::const_iterator it;
  for (it = itemGroups_.begin(); it != itemGroups_.end(); ++it)
    items += it.value()->items();

  return items;
}

QList<DrawingItemBase *> EditItemManager::findHitItems(const QPointF &pos,
    QHash<DrawingItemBase::HitType, QList<DrawingItemBase *> > &hitItemTypes,
    QList<DrawingItemBase *> &missedItems) const
{
  hitItemTypes.clear();

  QList<DrawingItemBase *> all = allItems();
  QList<DrawingItemBase *>::const_iterator it = all.constEnd();
  while (it != all.constBegin()) {
    --it;
    DrawingItemBase *item = *it;
    if (!item->isVisible())
      continue;
    DrawingItemBase::HitType type = item->hit(pos, true);
    if (type != DrawingItemBase::None)
      hitItemTypes[type].append(item);
    else
      missedItems.append(item);
  }

  // Create a list of hit items in order of priority.
  QList<DrawingItemBase *> hitItems;
  hitItems += hitItemTypes.value(DrawingItemBase::Point);
  hitItems += hitItemTypes.value(DrawingItemBase::Line);
  hitItems += hitItemTypes.value(DrawingItemBase::Area);

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

    delete incompleteItem_;
    incompleteItem_ = 0;
    hitItems_.clear();

    emit incompleteEditing(false);
    Q_EMIT repaintNeeded(false);
  }
}

void EditItemManager::completeEditing()
{
  if (incompleteItem_) {
    // Release the keyboard focus.
    setFocus(false);

    addItem(incompleteItem_); // causes repaint
    incompleteItem_ = 0;
    pushUndoCommands();

    //setSelectMode(); // restore default mode
    emit incompleteEditing(false);
  }
}

QList<DrawingItemBase *> EditItemManager::selectedItems() const
{
  QList<DrawingItemBase *> items;
  foreach (DrawingItemBase *item, allItems()) {
    if (item->selected())
      items.append(item);
  }

  return items;
}

void EditItemManager::selectItem(DrawingItemBase *item, bool exclusive, bool notify)
{
  item->setSelected();
  if (notify)
    emit selectionChanged();
}

void EditItemManager::selectAllItems()
{
  bool selected = false;

  foreach (DrawingItemBase *item, allItems()) {
    if (!item->selected()) {
      item->setSelected(true);
      selected = true;
    }
  }
  if (selected)
    emit selectionChanged();
}

void EditItemManager::deselectItem(DrawingItemBase *item, bool notify)
{
  item->setSelected(false);
  if (notify)
    emit selectionChanged();
}

void EditItemManager::deselectAllItems(bool notify)
{
  bool deselected = false;

  foreach (DrawingItemBase *item, allItems()) {
    if (item->selected()) {
      item->setSelected(false);
      deselected = true;
    }
  }
  if (notify && deselected)
    emit selectionChanged();
}

// Action handling

QHash<EditItemManager::Action, QAction*> EditItemManager::actions()
{
  QHash<Action, QAction*> a;
  a[SelectAll] = selectAllAction_;
  a[Cut] = cutAction_;
  a[Copy] = copyAction_;
  a[Paste] = pasteAction_;
  a[EditProperties] = editPropertiesAction_;
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
  Properties::PropertiesEditor::instance()->edit(selectedItems());
}

// Sets the style type of the currently selected items.
void EditItemManager::setStyleType()
{
  const QVariantList data = qobject_cast<QAction *>(sender())->data().toList();
  QList<DrawingItemBase *> items;
  const QString newType = data.at(1).toString();
  foreach (QVariant item, data.at(0).toList()) {
    DrawingItemBase *ditem = item.value<DrawingItemBase *>();
    DrawingStyleManager::instance()->setStyle(ditem, DrawingStyleManager::instance()->getStyle(ditem->category(), newType));
    ditem->setProperty("style:type", newType);
  }
}

void EditItemManager::updateActions()
{
  const QList<DrawingItemBase *> selItems = selectedItems();
  selectAllAction_->setEnabled(allItems().size() > 0);
  cutAction_->setEnabled(!selItems.isEmpty());
  copyAction_->setEnabled(!selItems.isEmpty());
  pasteAction_->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));
  lowerAction_->setEnabled(!selItems.isEmpty());
  raiseAction_->setEnabled(!selItems.isEmpty());
  editPropertiesAction_->setEnabled(selItems.size() == 1);
}

void EditItemManager::updateTimes()
{
  // Update the visibility of items based on the current plot time.
  const miutil::miTime& time = PLOTM->getPlotTime();
  changeTime(time);
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

// Emits the itemChanged() signal to notify about a _potential_ change to a single-selected
// item filtered according to setItemChangeFilter().
// The signal is also emitted whenever the above condition goes from true to false.
void EditItemManager::emitItemChanged() const
{
  if (!itemChangeNotificationEnabled_)
    return;

  QList<QVariantMap> itemProps;

  foreach (DrawingItemBase *item, selectedItems()) {

    const QString type(item->properties().value("style:type").toString());
    QVariantMap props;
    props.insert("type", type);
    props.insert("id", item->id());
    props.insert("visible", item->isVisible());
    props.insert("Placemark:name", item->property("Placemark:name").toString());
    //
    setFromLatLonPoints(item, item->getLatLonPoints());
    QVariantList latLonPoints;
    foreach (QPointF p, item->getLatLonPoints())
      latLonPoints.append(p);
    props.insert("latLonPoints", latLonPoints);

    itemProps.append(props);
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

QString EditItemManager::plotElementTag() const
{
  return "EDITDRAWING";
}

namespace /* anonymous */ {

QPointF getItemJoinPoint(DrawingItemBase *item)
{
  if (item->joinId() < 0)
    return item->getPoints().first();
  else
    return item->getPoints().last();
}

void moveItemPoint(DrawingItemBase *item, const QPointF& joinPoint)
{
  if (EditItemBase* ei = Editing(item))
    ei->movePointTo((item->joinId() < 0) ? 0 : item->getPoints().size() - 1, joinPoint);
  else
    METLIBS_LOG_WARN("item is not derived from EditItemBase");
}

void moveItemsPoint(const QList<DrawingItemBase*>& items, const QPointF& joinPoint)
{
  Q_FOREACH (DrawingItemBase *item, items)
      moveItemPoint(item, joinPoint);
}

} // anonymous namespace

// Updates joins by:
// - updating the join count of joined items, and
// - ensuring that joined end points coincide (skipped if \a updateJoinCountsOnly is true).
void EditItemManager::updateJoins(bool updateJoinCountsOnly)
{
  QHash<int, QList<DrawingItemBase *> > joins;

  // find all joins
  foreach (DrawingItemBase *item, allItems()) {
    const int joinId = item->joinId();
    if (joinId)
      joins[qAbs(joinId)].append(item);
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
          joinPoint = getItemJoinPoint(item);
          break;
        }

      // move joined end points to join point
      moveItemsPoint(join, joinPoint);
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
  foreach (DrawingItemBase *item, allItems()) {
    const int absJoinId = qAbs(item->joinId());
    if (absJoinId) {
      if (item->selected())
        selJoins[absJoinId].append(item);
      else
        unselJoins[absJoinId].append(item);
    }
  }

  DrawingItemBase* hit = hitItem();
  const int hitJoinId = hit ? hit->joinId() : 0;

  // loop over joins involving at least one selected item
  Q_FOREACH (int absJoinId, selJoins.keys()) {
    if (absJoinId == qAbs(hitJoinId)) { // the hit item is part of this join
      // move the joined end points in this join to the joined end point of the hit item
      const QPointF joinPoint = getItemJoinPoint(hit);
      moveItemsPoint(selJoins.value(absJoinId), joinPoint);
      if (unselJoins.contains(absJoinId))
        moveItemsPoint(unselJoins.value(absJoinId), joinPoint);
    } else { // the hit item is not part of this join
      // move the joined end points of the unselected items in this join to the joined end point of an arbitrary selected item
      if (unselJoins.contains(absJoinId)) {
        DrawingItemBase *firstSelItem = selJoins.value(absJoinId).first();
        const QPointF joinPoint = getItemJoinPoint(firstSelItem);
        moveItemsPoint(unselJoins.value(absJoinId), joinPoint);
      }
    }
  }
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
  copyItems(selectedItems().toSet());
}

void EditItemManager::deleteSelectedItems()
{
  const QSet<DrawingItemBase *> items = selectedItems().toSet();

  foreach (DrawingItemBase *item, items)
    removeItem(item);

  // ### the following is necessary only if items were actually removed
  updateActionsAndTimes();
}

void EditItemManager::cutSelectedItems()
{
  const QSet<DrawingItemBase *> items = selectedItems().toSet();
  copyItems(items);

  foreach (DrawingItemBase *item, items)
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
      DrawingItemBase *item = createItemFromVarMap(cbItem.toMap(), error);
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
  deselectAllItems();
  updateActionsAndTimes();
}

// Joins currently selected items.
void EditItemManager::joinSelectedItems()
{
  const QList<DrawingItemBase *> items = allItems();

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
    if (Editing(items.at(i))->hoverPos() != QPoint(-1, -1)) { // use end point of hovered item instead
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
    Editing(items.at(i))->movePointTo(first.testBit(i) ? 0 : (items.at(i)->getPoints().size() - 1), joinPoint);
  }

  updateJoins(true);
}

// Unjoins currently selected items.
void EditItemManager::unjoinSelectedItems()
{
  foreach (DrawingItemBase *item, allItems())
    item->propertiesRef().insert("joinId", 0);
  updateJoins(true);
}

void EditItemManager::toggleReversedForSelectedItems()
{
  foreach (DrawingItemBase *item, selectedItems()) {
    const QVariantMap style = DrawingStyleManager::instance()->getStyle(item);
    const bool reversed = style.value("reversed").toBool();
    item->setProperty("style:reversed", !reversed);
  }
}

void EditItemManager::lowerSelectedItems()
{
  scratchGroup(itemGroups_)->lowerItems(selectedItems());
}

void EditItemManager::raiseSelectedItems()
{
  scratchGroup(itemGroups_)->raiseItems(selectedItems());
}

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
  float dx, dy;
  int w, h;

  if (!isEditing()) {
    // Allow context menus to be created for items in the drawing manager.
    if (event->type() == QEvent::MouseButtonPress && event->buttons() == Qt::LeftButton) {
      // Translate the mouse event by the current displacement of the viewport.
      getViewportDisplacement(w, h, dx, dy);
      QMouseEvent me2(event->type(), QPoint(event->x() + dx, event->y() + dy),
                      event->globalPos(), event->button(), event->buttons(), event->modifiers());

      DrawingManager *drawm = DrawingManager::instance();

      QList<DrawingItemBase *> missedItems;
      const QList<DrawingItemBase *> hitItems = drawm->findHitItems(me2.pos(), hitItemTypes_, missedItems);
      if (!hitItems.empty()) {
        Properties::PropertiesEditor::instance()->edit(hitItems, true, false);
        event->accept();
        return;
      }
    }
    // Do not handle any other mouse events if editing is not in progress.
    return;
  }

  event->ignore();
  res.do_nothing();
  res.enable_background_buffer = true;

  // Translate the mouse event by the current displacement of the viewport.
  getViewportDisplacement(w, h, dx, dy);
  QMouseEvent me2(event->type(), QPoint(event->x() + dx, event->y() + dy),
                  event->globalPos(), event->button(), event->buttons(), event->modifiers());

  if (selectingOnly_) {
   // Only allow single-selection and basic hover highlighting.
   // Modifying items otherwise is not possible in this mode.

    if (event->type() == QEvent::MouseButtonPress) {
      if (me2.button() == Qt::LeftButton) {
        deselectAllItems(false);
        QList<DrawingItemBase *> missedItems;
        const QList<DrawingItemBase *> hitItems = findHitItems(me2.pos(), hitItemTypes_, missedItems);
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
      Q_EMIT repaintNeeded(false);

    return;
  }

  if (event->type() == QEvent::MouseButtonPress) {

    const QSet<DrawingItemBase *> origSelItems = selectedItems().toSet();

    if ((me2.button() == Qt::RightButton) && !hasIncompleteItem()) {
      openContextMenu(me2.pos(), me2.globalPos());

    } else {

      // create a new item if necessary
      if ((me2.button() == Qt::LeftButton) && !hasIncompleteItem()) {
        DrawingItemBase *item = 0;

        if (mode_ == CreatePolyLineMode) {
          item = Drawing(new EditItem_PolyLine::PolyLine());
          addItem(item, true);
          item->setProperty("style:type", createPolyLineAction_->data().toString());
        } else if (mode_ == CreateSymbolMode) {
          item = Drawing(new EditItem_Symbol::Symbol());
          addItem(item, true);
          item->setProperty("style:type", createSymbolAction_->data().toString());
        } else if (mode_ == CreateCompositeMode) {
          item = Drawing(new EditItem_Composite::Composite());
          addItem(item, true);
          item->setProperty("style:type", createCompositeAction_->data().toString());
        } else if (mode_ == CreateTextMode) {
          item = Drawing(new EditItem_Text::Text());
          addItem(item, true);
          item->setProperty("style:type", createTextAction_->data().toString());
        }

        if (item)
          DrawingStyleManager::instance()->setStyle(
                item, DrawingStyleManager::instance()->getStyle(item->category(), item->propertiesRef().value("style:type").toString()));
      }

      // process the event further (delegating it to relevant items etc.)
      mousePress(&me2);

      if (needsRepaint() && !hasIncompleteItem())
        emitItemChanged();
    }

    const QSet<DrawingItemBase *> selItems = selectedItems().toSet();
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

  if (event->type() != QEvent::MouseMove) {
    updateActionsAndTimes();
    res.action = canUndo() ? objects_changed : no_action;
  } else
    res.action = browsing; // allow the main window to update the coordinates label
}

void EditItemManager::openContextMenu(const QPoint &pos, const QPoint &globalPos)
{
  // Get actions contributed by a hit item (if any) if the position is a
  // valid one in widget coordinates.
  QList<QAction *> hitItemActions;
  if (pos.x() >= 0 && pos.y() >= 0) {
    QList<DrawingItemBase *> missedItems;
    QList<DrawingItemBase *> hitItems = findHitItems(pos, hitItemTypes_, missedItems);
    if (!hitItems.empty()) {
      DrawingItemBase* hitItem = hitItems.first(); // consider only the first item
      selectItem(hitItem);
      Q_EMIT repaintNeeded(false);
      if (EditItemBase* editItem = Editing(hitItem))
        hitItemActions = editItem->actions(pos);
    }
  }

  // populate the menu
  QSet<DrawingItemBase::Category> selectedCategories;
  QList<DrawingItemBase *> selItems = selectedItems();
  foreach (const DrawingItemBase *item, selItems)
    selectedCategories.insert(item->category());

  QMenu contextMenu;
  contextMenu.addAction(selectAllAction_);
  contextMenu.addSeparator();
  contextMenu.addAction(copyAction_);
  contextMenu.addAction(cutAction_);
  contextMenu.addAction(pasteAction_);
  pasteAction_->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-diana-object"));

  contextMenu.addSeparator();
  contextMenu.addAction(raiseAction_);
  contextMenu.addAction(lowerAction_);

  contextMenu.addSeparator();
  contextMenu.addAction(joinAction_);
  joinAction_->setEnabled(selItems.size() >= 2);
  contextMenu.addAction(unjoinAction_);
  unjoinAction_->setEnabled(false);
  foreach (const DrawingItemBase *item, selItems) {
    if (item->joinId() && (item->joinCount() > 0)) {
      unjoinAction_->setEnabled(true);
      break;
    }
  }

  contextMenu.addSeparator();
  contextMenu.addAction(toggleReversedAction_);
  toggleReversedAction_->setEnabled(selItems.size() >= 1);

  contextMenu.addSeparator();
  contextMenu.addAction(editPropertiesAction_);
  bool canEditProperties = !selItems.isEmpty() && Properties::PropertiesEditor::instance()->canEdit(selItems);
  editPropertiesAction_->setEnabled(canEditProperties);

  QMenu styleTypeMenu;
  styleTypeMenu.setTitle(tr("Convert"));
  // Disable conversion if only composite items are selected.
  styleTypeMenu.setEnabled((selectedCategories.size() == 1) && !selectedCategories.contains(DrawingItemBase::Composite));
  if (styleTypeMenu.isEnabled()) {

    // Obtain a list of style names for the first category of the selected items.
    QStringList styleTypes = DrawingStyleManager::instance()->styles(*(selectedCategories.begin()));

    // Filter out styles from different sections - this only makes sense
    // for symbols at the moment.
    QSet<QString> sections;
    foreach (DrawingItemBase *item, selItems) {
      QStringList pieces = item->property("style:type").toString().split("|");
      if (pieces.size() != 1)
        sections.insert(pieces.first());
    }
    QStringList filteredTypes;
    foreach (QString styleType, styleTypes) {
      QStringList pieces = styleType.split("|");
      if (pieces.size() == 1 || sections.contains(pieces.first()))
        filteredTypes.append(styleType);
    }

    qSort(filteredTypes);

    bool haveStyleTypeActions = false;
    // Add each of the available style types to the menu.
    foreach (QString styleType, filteredTypes) {
      QString styleName = styleType.split("|").last();
      QAction *action = new QAction(QString("%1 %2").arg(tr("To")).arg(styleName), 0);
      QVariantList styleItems;
      foreach (DrawingItemBase *styleItem, selItems)
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
      haveStyleTypeActions = true;
    }
    if (haveStyleTypeActions) {
      contextMenu.addSeparator();
      contextMenu.addMenu(&styleTypeMenu);
    }
  }

  if (!hitItemActions.isEmpty()) {
    contextMenu.addSeparator();
    foreach (QAction *action, hitItemActions)
      contextMenu.addAction(action);
  }

  // execute the menu (assuming all its actions are connected to slots)
  if (!contextMenu.isEmpty())
    contextMenu.exec(globalPos);

  if (!hasIncompleteItem())
    emitItemChanged();

  pushUndoCommands();
}

void EditItemManager::getViewportDisplacement(int &w, int &h, float &dx, float &dy)
{
  // Transform the mouse position into the original coordinate system used for the objects.
  PLOTM->getPhysSize().unpack(w, h);
  const Rectangle& plotRect_ = PLOTM->getPlotSize();

  if (selectedItems().isEmpty())
    setEditRect(PLOTM->getPlotSize());

  // Determine the displacement from the edit origin to the current view origin
  // in screen coordinates. This gives us displaced screen coordinates - these
  // are coordinates relative to the original edit rectangle.
  dx = (plotRect_.x1 - editRect_.x1) * (w/plotRect_.width());
  dy = (plotRect_.y1 - editRect_.y1) * (h/plotRect_.height());
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

  res.repaint = true;
  res.update_background_buffer = true;

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

  // First, pass key presses to the items themselves.

  if (event->type() == QEvent::KeyPress)
    keyPress(event);

  // Process unhandled key presses to perform high level operations.

  if (!event->isAccepted() && event->type() == QEvent::KeyPress) {
    if (cutAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      cutSelectedItems();
    } else if (copyAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      copySelectedItems();
    } else if (pasteAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      pasteItems();
    } else if (lowerAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      lowerSelectedItems();
    } else if (raiseAction_->shortcut().matches(event->key() | event->modifiers()) == QKeySequence::ExactMatch) {
      raiseSelectedItems();
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
    } else if ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))
      deleteSelectedItems();
  }

  repaintNeeded_ = true; // ### for now

  pushUndoCommands();

  updateActionsAndTimes();

  if (event->isAccepted() || needsRepaint())
    emitItemChanged();
}

void EditItemManager::pushUndoCommands()
{
  EditItems::ItemGroup *group = scratchGroup(itemGroups_);
  QHash<int, QVariantMap> newStates = getStates(group->items());

  // Return immediately if nothing has changed.
  if (oldStates_ == newStates)
    return;

  QSet<int> oldIds = oldStates_.keys().toSet();
  QSet<int> newIds = newStates.keys().toSet();

  QList<DrawingItemBase *> removeItems;
  foreach (int id, oldIds - newIds)
    removeItems.append(removedItems_.value(id));

  QList<DrawingItemBase *> addItems;
  foreach (int id, newIds - oldIds)
    addItems.append(group->item(id));

  undoStack_.push(new ModifyItemsCommand(oldStates_, newStates, removeItems, addItems));
}

/**
 * Gets the states of a list of items, storing them in a hash.
 */
QHash<int, QVariantMap> EditItemManager::getStates(const QList<DrawingItemBase *> &items) const
{
  QHash<int, QVariantMap> states;

  foreach (DrawingItemBase *item, items) {
    QVariantMap properties = item->properties();
    QList<QVariant> llp;
    foreach (QPointF p, item->getLatLonPoints())
      llp.append(QVariant(p));
    properties["latLonPoints"] = llp;
    states[item->id()] = properties;
  }

  return states;
}

/**
 * Replaces the states of items in the layer groups with those supplied.
 */
void EditItemManager::replaceItemStates(const QHash<int, QVariantMap> &states,
    QList<DrawingItemBase *> removeItems, QList<DrawingItemBase *> addItems)
{
  EditItems::ItemGroup *group = scratchGroup(itemGroups_);

  foreach(DrawingItemBase *item, removeItems)
    group->removeItem(item);

  foreach(DrawingItemBase *item, addItems)
    group->addItem(item);

  group->replaceStates(states);

  // Record the states so that changes can be tracked against the current ones.
  oldStates_ = getStates(allItems());
  removedItems_.clear();
  emit itemStatesReplaced();
}

QString EditItemManager::loadDrawing(const QString &name, const QString &fileName)
{
  QString error;

  QList<DrawingItemBase *> items = KML::createFromFile(name, fileName, error);
  if (!error.isEmpty()) {
    METLIBS_LOG_WARN("Failed to open file: " << fileName.toStdString());
    return error;
  }

  // Return early if the file was empty.
  if (items.isEmpty())
    return error;

  foreach (DrawingItemBase *item, items)
    addItem(item, false, true);

  pushUndoCommands();

  // Record the file name.
  drawings_[name] = fileName;
  emit drawingLoaded(name);

  return error;
}

void EditItemManager::save()
{
  emit saveRequested();
}

void EditItemManager::showItemInformation(const QList<DrawingItemBase *> &items)
{
  // Create a tooltip containing the values of any filtered properties for
  // the first item in the list that contains them.

  QSet<QString> allowed;
  if (isEditing())
    allowed = tooltipEditingProperties;
  else
    allowed = tooltipDrawingProperties;

  foreach (DrawingItemBase *item, items) {
    if (!item->isVisible())
      continue;

    QStringList lines;
    QVariantMap properties = item->properties();

    // Merge properties with identical values, if a rule exists for them.
    QHash<QString, QStringList>::const_iterator it;

    for (it = tooltipMergeRules.begin(); it != tooltipMergeRules.end(); ++it) {
      QString replacement = it.key();
      QStringList names = it.value();

      // Check if all the named properties have the same value.
      QVariant value = properties.value(names.first());

      if (value.isValid()) {
        bool allEqual = true;

        for (int i = 1; i < names.size(); ++i) {
          if (properties.value(names.at(i)) != value) {
            allEqual = false;
            break;
          }
        }
        if (allEqual) {
          // Remove the properties from the map and replace them with a single
          // property.
          foreach (const QString &name, names)
            properties.remove(name);

          properties[replacement] = value;
        }
      }
    }

    foreach (const QString &key, properties.keys()) {
      foreach (const QString &section, allowed) {
        if (key.startsWith(section)) {
          // Allowed properties with no section are included verbatim; otherwise
          // the part of the property name after the section is shown.
          if (key.endsWith(":"))
            lines.append(QString("%1: %2").arg(key.mid(section.size())).arg(properties.value(key).toString()));
          else
            lines.append(QString("%1: %2").arg(key).arg(properties.value(key).toString()));
        }
      }
    }

    if (!lines.isEmpty()) {
      QString text = lines.join("\n");
      showToolTipText(1, text);
      return;
    } else
      showToolTipText(1, "");
  }
}

void EditItemManager::showToolTipText(int priority, const QString &text)
{
  // Update the tooltip if it is not visible, the existing text is empty,
  // the existing tooltip has the same priority or the new tip has a
  // higher priority and contains text.
  if (!QToolTip::isVisible() || tooltip_.text.isEmpty() || priority == tooltip_.priority
      || (priority > tooltip_.priority && !text.isEmpty())) {
    tooltip_.priority = priority;
    tooltip_.text = text;
    if (!text.isEmpty())
      QToolTip::showText(QCursor::pos(), text);
    else
      QToolTip::hideText();
  }
}

// Command classes

ModifyItemsCommand::ModifyItemsCommand(const QHash<int, QVariantMap> &oldItemStates,
                                       const QHash<int, QVariantMap> &newItemStates,
                                       QList<DrawingItemBase *> removeItems,
                                       QList<DrawingItemBase *> addItems)
  : oldItemStates_(oldItemStates),
    newItemStates_(newItemStates), removeItems_(removeItems), addItems_(addItems)
{
  QSet<int> oldIds = oldItemStates.keys().toSet();
  QSet<int> newIds = newItemStates.keys().toSet();

  // Only include information about the states that have changed.
  QSet<int> common = oldIds & newIds;
  int n = common.size();
  foreach (int id, common) {
    if (oldItemStates.value(id) == newItemStates.value(id)) {
      oldItemStates_.remove(id);
      newItemStates_.remove(id);
      n--;
    }
  }

  QStringList desc;
  if (!removeItems.isEmpty())
    desc.append(QApplication::translate("ModifyItemsCommand", "%1 items removed").arg(removeItems.size()));
  if (!addItems.isEmpty())
    desc.append(QApplication::translate("ModifyItemsCommand", "%1 items added").arg(addItems.size()));
  if (n > 0)
    desc.append(QApplication::translate("ModifyItemsCommand", "%1 items modified").arg(n));

  setText(desc.join(", "));
}

ModifyItemsCommand::~ModifyItemsCommand()
{
  // When this command is destroyed, all the items that it would add are no
  // longer relevant and can be destroyed, too.
  qDeleteAll(addItems_);
}

void ModifyItemsCommand::undo()
{
  // Pass the items that were added and removed in the original command in
  // reverse order since we are undoing that command.
  EditItemManager::instance()->replaceItemStates(oldItemStates_, addItems_, removeItems_);
}

void ModifyItemsCommand::redo()
{
  EditItemManager::instance()->replaceItemStates(newItemStates_, removeItems_, addItems_);
}

int ModifyItemsCommand::id() const
{
  return 0x4d6f6469; // "Modi"
}
