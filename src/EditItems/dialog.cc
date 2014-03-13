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

#include "diEditItemManager.h"
#include "diController.h"
#include <EditItems/dialog.h>
#include <EditItems/toolbar.h>

#include <paint_mode.xpm> // ### for now
#include "addempty.xpm"
#include "duplicate.xpm"
#include "edit.xpm"
#include "empty.xpm"
#include "fileopen.xpm"
#include "hideall.xpm"
#include "mergevisible.xpm"
#include "movedown.xpm"
#include "moveup.xpm"
#include "remove.xpm"
#include "showall.xpm"
#include "visible.xpm"
#include "unsavedchanges.xpm"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace EditItems {

CheckableLabel::CheckableLabel(bool checked, const QPixmap &pixmap, const QString &checkedToolTip, const QString &uncheckedToolTip, bool clickable)
  : checked_(checked)
  , pixmap_(pixmap)
  , checkedToolTip_(checkedToolTip)
  , uncheckedToolTip_(uncheckedToolTip)
  , clickable_(clickable)
{
  setMargin(0);
  setChecked(checked_);
}

void CheckableLabel::setChecked(bool enabled)
{
  checked_ = enabled;
  if (checked_) {
    setPixmap(pixmap_);
    setToolTip(checkedToolTip_);
  } else {
    setPixmap(empty_xpm);
    setToolTip(uncheckedToolTip_);
  }
}

void CheckableLabel::mousePressEvent(QMouseEvent *event)
{
  if (clickable_ && (event->button() & Qt::LeftButton))
    setChecked(!checked_);
  emit mouseClicked(event);
  emit checked(checked_);
}

NameLabel::NameLabel(const QString &name)
  : QLabel(name)
{
  setMargin(0);
}

void NameLabel::mousePressEvent(QMouseEvent *event)
{
  emit mouseClicked(event);
}

void NameLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
  emit mouseDoubleClicked(event);
}

LayerWidget::LayerWidget(const QSharedPointer<Layer> &layer, QWidget *parent)
  : QWidget(parent)
  , layer_(layer)
{
  setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);

  visibleLabel_ = new CheckableLabel(
        layer_->isVisible(), visible_xpm, "the layer is visible\n(click to make it invisible)", "the layer is invisible\n(click to make it visible)");
  connect(visibleLabel_, SIGNAL(checked(bool)), SLOT(handleVisibilityChanged(bool)));
  mainLayout->addWidget(visibleLabel_);

  static int nn = 0;
  unsavedChangesLabel_ = new CheckableLabel(
        nn++ % 2, unsavedchanges_xpm, "the layer has unsaved changes\n(do ??? to save them)", "the layer does not have any unsaved changes", false);
  connect(unsavedChangesLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  mainLayout->addWidget(unsavedChangesLabel_);

  nameLabel_ = new NameLabel(layer_->name());
  nameLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  connect(nameLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  connect(nameLabel_, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SIGNAL(mouseDoubleClicked(QMouseEvent *)));
  mainLayout->addWidget(nameLabel_);
}

LayerWidget::~LayerWidget()
{
  Layers::instance()->remove(layer_);
}

QSharedPointer<Layer> LayerWidget::layer()
{
  return layer_;
}

QString LayerWidget::name() const
{
  return layer_->name();
}

void LayerWidget::setName(const QString &name)
{
  const QString trimmedName = name.trimmed();
  nameLabel_->setText(trimmedName);
  layer_->setName(trimmedName);
}

bool LayerWidget::isLayerVisible() const
{
  return layer_->isVisible();
}

void LayerWidget::setLayerVisible(bool visible)
{
  visibleLabel_->setChecked(visible);
  layer_->setVisible(visible);
}

bool LayerWidget::hasUnsavedChanges() const
{
  return layer_->hasUnsavedChanges();
}

void LayerWidget::setUnsavedChanges(bool unsavedChanges)
{
  unsavedChangesLabel_->setChecked(unsavedChanges);
}

void LayerWidget::setCurrent(bool current)
{
  const QString ssheet(current ? "QLabel { background-color : #f27b4b; color : black; }" : "");
  visibleLabel_->setStyleSheet(ssheet);
  unsavedChangesLabel_->setStyleSheet(ssheet);
  nameLabel_->setStyleSheet(ssheet);
}

void LayerWidget::editName()
{
  QString name = nameLabel_->text();

  while (true) {
    bool ok;
    name = QInputDialog::getText(this, "Edit layer name", "Layer name:", QLineEdit::Normal, name, &ok).trimmed();
    if (ok) {
      const QSharedPointer<Layer> existingLayer = Layers::instance()->layerFromName(name);
      if (existingLayer.isNull()) {
        setName(name);
        break; // ok (changed)
      } else if (existingLayer == layer()) {
        break; // ok (unchanged)
      } else {
        QMessageBox::warning(this, "Name exists", "Another layer with this name already exists!", QMessageBox::Ok);
        // try again
      }
    } else {
      break; // cancel
    }
  }
}

void LayerWidget::handleVisibilityChanged(bool visible)
{
  layer_->setVisible(visible);
  emit visibilityChanged(visible);
}

ScrollArea::ScrollArea(QWidget *parent)
  : QScrollArea(parent)
{
}

void ScrollArea::keyPressEvent(QKeyEvent *event)
{
  event->ignore();
}

QWidget *Dialog::createAvailableLayersPane()
{
  QGroupBox *groupBox = new QGroupBox(tr("Available Layers"));

  drawingList = new QListView();
  drawingList->setModel(&drawingModel);
  drawingList->setSelectionMode(QAbstractItemView::MultiSelection);
  drawingList->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(drawingList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateButtons()));

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(importFilesButton_ = createToolButton(QPixmap(movedown_xpm), "Import files as layer", SLOT(importChosenFiles())));
  bottomLayout->addWidget(loadFileButton_ = createToolButton(QPixmap(fileopen_xpm), "Open file", SLOT(loadFile())));

  QVBoxLayout *layout = new QVBoxLayout(groupBox);
  layout->setContentsMargins(0, 2, 0, 2);
  layout->addWidget(drawingList);
  layout->addLayout(bottomLayout);

  return groupBox;
}

QWidget *Dialog::createActiveLayersPane()
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setContentsMargins(0, 2, 0, 2);

  QWidget *activeLayersWidget = new QWidget;
  activeLayersWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  activeLayersLayout_ = new QVBoxLayout(activeLayersWidget);
  activeLayersLayout_->setContentsMargins(0, 0, 0, 0);
  activeLayersLayout_->setSpacing(0);
  activeLayersLayout_->setMargin(0);

  activeLayersScrollArea_ = new ScrollArea;
  activeLayersScrollArea_->setWidget(activeLayersWidget);
  activeLayersScrollArea_->setWidgetResizable(true);

  mainLayout->addWidget(activeLayersScrollArea_);

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(addEmptyButton_ = createToolButton(QPixmap(addempty_xpm), "Add an empty layer", SLOT(addEmpty())));
  bottomLayout->addWidget(mergeVisibleButton_ = createToolButton(QPixmap(mergevisible_xpm), "Merge visible layers", SLOT(mergeVisible())));
  bottomLayout->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", SLOT(showAll())));
  bottomLayout->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", SLOT(hideAll())));
  bottomLayout->addWidget(duplicateCurrentButton_ = createToolButton(QPixmap(duplicate_xpm), "Duplicate the current layer", SLOT(duplicateCurrent())));
  bottomLayout->addWidget(removeCurrentButton_ = createToolButton(QPixmap(remove_xpm), "Remove the current layer", SLOT(removeCurrent())));
  bottomLayout->addWidget(moveCurrentUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move the current layer up", SLOT(moveCurrentUp())));
  bottomLayout->addWidget(moveCurrentDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move the current layer down", SLOT(moveCurrentDown())));
  bottomLayout->addWidget(editCurrentButton_ = createToolButton(QPixmap(edit_xpm), "Edit the current layer", SLOT(editCurrent())));
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  mainLayout->addLayout(bottomLayout);

  QGroupBox *groupBox = new QGroupBox("Active Layers");
  groupBox->setLayout(mainLayout);

  return groupBox;
}

Dialog::Dialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  // record the editor in use
  editm_ = EditItemManager::instance();

  // create the GUI
  setWindowTitle("Drawing Layers");
  setFocusPolicy(Qt::StrongFocus);
  QSplitter *splitter = new QSplitter(Qt::Vertical);
  splitter->addWidget(createAvailableLayersPane());
  splitter->addWidget(createActiveLayersPane());
  splitter->setSizes(QList<int>() << 500 << 500);
  //
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(close()));
  //
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(splitter);
  //
  mainLayout->addWidget(buttonBox);

  // create an action that can be used to open the dialog from within a menu or toolbar
  m_action = new QAction(QIcon(QPixmap(paint_mode_xpm)), tr("Painting tools"), this);
  m_action->setShortcutContext(Qt::ApplicationShortcut);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  connect(m_action, SIGNAL(toggled(bool)), SLOT(toggleDrawingMode(bool)));

  connect(EditItems::Layers::instance(), SIGNAL(replacedLayers()), SLOT(handleLayersUpdated()));
  connect(EditItems::Layers::instance(), SIGNAL(addedLayer()), SLOT(handleLayersUpdated()));

  // Populate the drawing model with data from the drawing manager.
  updateModel();

  updateButtons();
}

std::string Dialog::name() const
{
  return "DRAWING";
}

void Dialog::updateTimes()
{
  std::vector<miutil::miTime> times;
  if (editm_->isEnabled())
    times = DrawingManager::instance()->getTimes();
  emit emitTimes("DRAWING", times);
}

void Dialog::updateDialog()
{
}

std::vector<std::string> Dialog::getOKString()
{
  std::vector<std::string> lines;

  if (!editm_->isEnabled())
    return lines;

  foreach (QString filePath, editm_->getLoaded()) {
    QString line = "DRAWING file=\"" + filePath + "\"";
    lines.push_back(line.toStdString());
  }

  return lines;
}

void Dialog::putOKString(const std::vector<std::string>& vstr)
{
  // If the current drawing has not been produced, ignore any plot commands
  // that are sent to the dialog. This blocks any strings that are sent
  // while the drawing dialog is open.
  if (!editm_->isEnabled())
    return;

  // Submit the lines as new input.
  std::vector<std::string> inp;
  inp.insert(inp.begin(), vstr.begin(), vstr.end());
  DrawingManager::instance()->processInput(inp);
}

void Dialog::toggleDrawingMode(bool enable)
{
  // Enabling drawing mode (opening the dialog) causes the manager to enter
  // working mode. This makes it possible to show objects that have not been
  // serialised as plot commands.
  editm_->setWorking(enable);

  // ### for now, toggle editing mode as well:
  toggleEditingMode(enable);
}

void Dialog::toggleEditingMode(bool enable)
{
  // When editing starts, remove any existing items and load the chosen
  // files. Mark the product as unfinished by disabling it.
  if (enable)
    editm_->setEnabled(false);

  editm_->setEditing(enable);
  ToolBar::instance()->setVisible(enable);
  ToolBar::instance()->setEnabled(enable);
}

QToolButton *Dialog::createToolButton(const QIcon &icon, const QString &toolTip, const char *method) const
{
  QToolButton *button = new QToolButton;
  button->setIcon(icon);
  button->setToolTip(toolTip);
  connect(button, SIGNAL(clicked()), this, method);
  return button;
}

void Dialog::initialize(LayerWidget *layerWidget)
{
  connect(layerWidget, SIGNAL(mouseClicked(QMouseEvent *)), SLOT(mouseClicked(QMouseEvent *)));
  connect(layerWidget, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SLOT(mouseDoubleClicked(QMouseEvent *)));
  connect(layerWidget, SIGNAL(visibilityChanged(bool)), SLOT(handleLayersStateUpdate()));
}

void Dialog::keyPressEvent(QKeyEvent *event)
{
  if (event->matches(QKeySequence::Quit)) {
    qApp->quit();
  } else if (event->key() == Qt::Key_Up) {
    if (event->modifiers() & Qt::ControlModifier)
      moveCurrentUp();
    else
      setCurrentIndex(currentPos() - 1);
  } else if (event->key() == Qt::Key_Down) {
    if (event->modifiers() & Qt::ControlModifier)
      moveCurrentDown();
    else
      setCurrentIndex(currentPos() + 1);
  } else if ((event->key() == Qt::Key_Delete) || (event->key() == Qt::Key_Backspace)) {
    removeCurrent();
  } else {
    QDialog::keyPressEvent(event);
  }
}

void Dialog::setCurrentIndex(int index)
{
  setCurrent(atPos(index));
}

void Dialog::setCurrent(LayerWidget *layerWidget)
{
  if (!layerWidget)
    return;
  Layers::instance()->setCurrent(layerWidget->layer());
  updateCurrent();
  ensureVisible(layerWidget);
  handleLayersStateUpdate();
}

void Dialog::updateCurrent()
{
  for (int i = 0; i < activeLayersLayout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(activeLayersLayout_->itemAt(i)->widget());
    layerWidget->setCurrent(layerWidget->layer() == Layers::instance()->current());
  }
}

int Dialog::currentPos() const
{
  return Layers::instance()->currentPos();
}

LayerWidget *Dialog::current()
{
  return atPos(currentPos());
}

LayerWidget *Dialog::atPos(int pos)
{
  if (pos >= 0 && pos < activeLayersLayout_->count())
    return qobject_cast<LayerWidget *>(activeLayersLayout_->itemAt(pos)->widget());
  return 0;
}

void Dialog::duplicate(LayerWidget *srcLayerWidget)
{
  const QSharedPointer<Layer> newLayer = Layers::instance()->addDuplicate(srcLayerWidget->layer());
  const int srcIndex = activeLayersLayout_->indexOf(srcLayerWidget);
  LayerWidget *newLayerWidget = new LayerWidget(newLayer);
  activeLayersLayout_->insertWidget(srcIndex + 1, newLayerWidget);
  initialize(newLayerWidget);
  setCurrent(newLayerWidget);
  ensureCurrentVisible();
  handleLayersStateUpdate();
}

void Dialog::duplicateCurrent()
{
  duplicate(current());
}

void Dialog::remove(LayerWidget *layerWidget, bool implicit)
{
  if ((!layerWidget) || (activeLayersLayout_->count() == 0))
    return;

  if (!layerWidget->layer()->isEmpty() && (!implicit) && (QMessageBox::warning(
                    this, "Remove layer", "Really remove layer?",
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::No))
    return;

  const int index = activeLayersLayout_->indexOf(layerWidget);
  activeLayersLayout_->removeWidget(layerWidget);
  delete layerWidget;
  if (activeLayersLayout_->count() > 0)
    setCurrentIndex(qMin(index, activeLayersLayout_->count() - 1));

  if (!implicit)
    handleLayersStateUpdate();
}

void Dialog::remove(int index)
{
  remove(atPos(index));
}

void Dialog::removeCurrent()
{
  remove(current());
}

void Dialog::move(LayerWidget *layerWidget, bool up)
{
  const int index = activeLayersLayout_->indexOf(layerWidget);
  if ((up && (index <= 0)) ||
      (!up && (index >= (activeLayersLayout_->count() - 1))))
    return;
  activeLayersLayout_->removeWidget(layerWidget);
  activeLayersLayout_->insertWidget(index + (up ? -1 : 1), layerWidget);
  updateCurrent();
  Layers::instance()->set(layers(allLayerWidgets()), false);
  if (layerWidget == current())
    ensureCurrentVisible();
  handleLayersStateUpdate();
}

void Dialog::moveUp(LayerWidget *layerWidget)
{
  move(layerWidget, true);
}

void Dialog::moveUp(int index)
{
  moveUp(atPos(index));
}

void Dialog::moveCurrentUp()
{
  moveUp(current());
}

void Dialog::moveDown(LayerWidget *layerWidget)
{
  move(layerWidget, false);
}

void Dialog::moveDown(int index)
{
  moveDown(atPos(index));
}

void Dialog::moveCurrentDown()
{
  moveDown(current());
}

void Dialog::editCurrent()
{
  current()->editName(); // ### only the name for now
}

void Dialog::mouseClicked(QMouseEvent *event)
{
  LayerWidget *layerWidget = qobject_cast<LayerWidget *>(sender());
  Q_ASSERT(layerWidget);
  setCurrent(layerWidget);
  if (event->button() & Qt::RightButton) {
    QMenu contextMenu;
    QAction duplicate_act(QPixmap(duplicate_xpm), tr("Duplicate"), 0);
    duplicate_act.setIconVisibleInMenu(true);
    duplicate_act.setEnabled(duplicateCurrentButton_->isEnabled());
    //
    QAction remove_act(QPixmap(remove_xpm), tr("Remove"), 0);
    remove_act.setIconVisibleInMenu(true);
    remove_act.setEnabled(removeCurrentButton_->isEnabled());
    //
    QAction moveUp_act(QPixmap(moveup_xpm), tr("Move Up"), 0);
    moveUp_act.setIconVisibleInMenu(true);
    moveUp_act.setEnabled(moveCurrentUpButton_->isEnabled());
    //
    QAction moveDown_act(QPixmap(movedown_xpm), tr("Move Down"), 0);
    moveDown_act.setIconVisibleInMenu(true);
    moveDown_act.setEnabled(moveCurrentDownButton_->isEnabled());
    //
    QAction editName_act(QPixmap(edit_xpm), tr("Edit Name"), 0);
    editName_act.setIconVisibleInMenu(true);

    // add actions
    contextMenu.addAction(&duplicate_act);
    contextMenu.addAction(&remove_act);
    contextMenu.addAction(&moveUp_act);
    contextMenu.addAction(&moveDown_act);
    contextMenu.addAction(&editName_act);
    QAction *action = contextMenu.exec(event->globalPos(), &duplicate_act);
    if (action == &duplicate_act) {
      duplicate(layerWidget);
    } else if (action == &remove_act) {
      remove(layerWidget);
    } else if (action == &moveUp_act) {
      moveUp(layerWidget);
    } else if (action == &moveDown_act) {
      moveDown(layerWidget);
    } else if (action == &editName_act) {
      layerWidget->editName();
    }
  }
}

void Dialog::mouseDoubleClicked(QMouseEvent *event)
{
  if (event->button() & Qt::LeftButton)
    current()->editName();
}

void Dialog::ensureVisible(LayerWidget *layer)
{
  qApp->processEvents();
  activeLayersScrollArea_->ensureWidgetVisible(layer);
}

void Dialog::ensureCurrentVisibleTimeout()
{
  ensureVisible(current());
}

void Dialog::ensureCurrentVisible()
{
  QTimer::singleShot(0, this, SLOT(ensureCurrentVisibleTimeout()));
}

void Dialog::add(const QSharedPointer<Layer> &layer, bool skipUpdate)
{
  LayerWidget *layerWidget = new LayerWidget(layer);
  activeLayersLayout_->addWidget(layerWidget);
  initialize(layerWidget);
  if (!skipUpdate) {
    setCurrent(layerWidget);
    ensureCurrentVisible();
    handleLayersStateUpdate();
  }
}

void Dialog::addEmpty()
{
  add(Layers::instance()->addEmpty(false));
}

void Dialog::mergeVisible()
{
  const QList<LayerWidget *> visLayerWidgets = visibleLayerWidgets();

  if (visLayerWidgets.size() > 1) {
    if (QMessageBox::warning(
          this, "Merge visible layers", "Really merge visible layers?",
          QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      return;

    Layers::instance()->mergeIntoFirst(layers(visLayerWidgets));

    for (int i = 1; i < visLayerWidgets.size(); ++i)
      remove(visLayerWidgets.at(i), true);
    setCurrent(visLayerWidgets.first());
  }

  handleLayersStateUpdate();
}

void Dialog::setAllVisible(bool visible)
{
  for (int i = 0; i < activeLayersLayout_->count(); ++i)
    qobject_cast<LayerWidget *>(activeLayersLayout_->itemAt(i)->widget())->setLayerVisible(visible);
  handleLayersStateUpdate();
}

void Dialog::showAll()
{
  setAllVisible(true);
}

void Dialog::hideAll()
{
  setAllVisible(false);
}

QList<LayerWidget *> Dialog::visibleLayerWidgets()
{
  QList<LayerWidget *> visLayerWidgets;
  for (int i = 0; i < activeLayersLayout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(activeLayersLayout_->itemAt(i)->widget());
    if (layerWidget->isLayerVisible())
      visLayerWidgets.append(layerWidget);
  }
  return visLayerWidgets;
}

QList<LayerWidget *> Dialog::allLayerWidgets()
{
  QList<LayerWidget *> allLayerWidgets;
  for (int i = 0; i < activeLayersLayout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(activeLayersLayout_->itemAt(i)->widget());
    allLayerWidgets.append(layerWidget);
  }
  return allLayerWidgets;
}

QList<QSharedPointer<Layer> > Dialog::layers(const QList<LayerWidget *> &layerWidgets)
{
  QList<QSharedPointer<Layer> > layers_;
  foreach (LayerWidget *layerWidget, layerWidgets)
    layers_.append(layerWidget->layer());
  return layers_;
}

void Dialog::updateButtons()
{
  const int visSize = visibleLayerWidgets().size();
  const int allSize = activeLayersLayout_->count();
  mergeVisibleButton_->setEnabled(visSize > 1);
  showAllButton_->setEnabled(visSize < allSize);
  hideAllButton_->setEnabled(visSize > 0);
  duplicateCurrentButton_->setEnabled(current());
  removeCurrentButton_->setEnabled(current());
  moveCurrentUpButton_->setEnabled(currentPos() > 0);
  moveCurrentDownButton_->setEnabled(currentPos() < (allSize - 1));
  editCurrentButton_->setEnabled(current());
  importFilesButton_->setEnabled(drawingList->selectionModel()->selection().size() != 0);
}

void Dialog::handleLayersStateUpdate()
{
  updateButtons();
  Layers::instance()->update();
}

/**
 * Updates the drawing model with data from the drawing manager.
 */
void Dialog::updateModel()
{
  drawingModel.clear();

  foreach (QString filePath, DrawingManager::instance()->getDrawings()) {
    QString fileName = QFileInfo(filePath).fileName();
    QStandardItem *item = new QStandardItem(fileName);
    item->setData(filePath, Qt::UserRole);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    drawingModel.appendRow(item);
  }
}

/**
 * Adds new layers to the stack containing the items from the selected items in
 * the drawing model.
 *
 * This is performed whenever the selection changes in the drawing list.
 */
void Dialog::importChosenFiles()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  foreach (QModelIndex index, drawingList->selectionModel()->selection().indexes()) {
    if (index.column() != 0)
      continue;

    QString filePath = index.data(Qt::UserRole).toString();

    if (!EditItemManager::instance()->loadItems(filePath)) {
      // Disable the item to indicate that it is not loaded.
      QStandardItem *item = drawingModel.itemFromIndex(index);
      item->setEnabled(false);
    }
  }

  QApplication::restoreOverrideCursor();
}

void Dialog::loadFile()
{
  const QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"),
    DrawingManager::instance()->getWorkDir(), tr("KML files (*.kml);; All files (*)"));
  if (fileName.isNull())
    return;

  QApplication::setOverrideCursor(Qt::WaitCursor);
  EditItemManager::instance()->loadItems(fileName);
  QApplication::restoreOverrideCursor();
}

void Dialog::handleLayersUpdated()
{
  // clear existing widgets
  QList<LayerWidget *> layerWidgets = allLayerWidgets();
  for (int i = 0; i < layerWidgets.size(); ++i)
    remove(layerWidgets.at(i), true);

  // insert widgets for current layers
  for (int i = 0; i < Layers::instance()->size(); ++i)
    add(Layers::instance()->at(i), true);
  setCurrentIndex(0);
}

} // namespace
