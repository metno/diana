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

#include <EditItems/activelayerspane.h>
#include <EditItems/dialogcommon.h>
#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>
#include <EditItems/kml.h>
#include <diEditItemManager.h>

#include "addempty.xpm"
#include "duplicate.xpm"
#include "edit.xpm"
#include "hideall.xpm"
#include "mergevisible.xpm"
#include "movedown.xpm"
#include "moveup.xpm"
#include "remove.xpm"
#include "showall.xpm"
#include "visible.xpm"
#include "unsavedchanges.xpm"
#include "filesave.xpm"

#include <QApplication>
#include <QFileDialog>
#include <QAction>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>
#include <QToolButton>

#include <QDebug>

namespace EditItems {

LayerWidget::LayerWidget(LayerManager *layerManager, const QSharedPointer<Layer> &layer, bool showInfo, QWidget *parent)
  : QWidget(parent)
  , layer_(layer)
{
  setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);

  visibleLabel_ = new CheckableLabel(
        layer_ ? layer_->isVisible() : false, visible_xpm, "the layer is visible\n(click to make it invisible)", "the layer is invisible\n(click to make it visible)");
  connect(visibleLabel_, SIGNAL(checked(bool)), SLOT(handleVisibilityChanged(bool)));
  mainLayout->addWidget(visibleLabel_);

  static int nn = 0;
  unsavedChangesLabel_ = new CheckableLabel(
        nn++ % 2, unsavedchanges_xpm, "the layer has unsaved changes\n(do ??? to save them)", "the layer does not have any unsaved changes", false);
  connect(unsavedChangesLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  //mainLayout->addWidget(unsavedChangesLabel_);

  nameLabel_ = new ClickableLabel(layer_ ? layer_->name() : QString());
  nameLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  connect(nameLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  connect(nameLabel_, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SIGNAL(mouseDoubleClicked(QMouseEvent *)));
  mainLayout->addWidget(nameLabel_);

  infoLabel_ = new ClickableLabel("<info>");
  infoLabel_->setVisible(showInfo);
  infoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  connect(infoLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  connect(infoLabel_, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SIGNAL(mouseDoubleClicked(QMouseEvent *)));
  mainLayout->addWidget(infoLabel_);

  mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred));
}

LayerWidget::~LayerWidget()
{
  delete visibleLabel_;
  delete unsavedChangesLabel_;
  delete nameLabel_;
  delete infoLabel_;
}

QSharedPointer<Layer> LayerWidget::layer() const
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
  layer_->setName(trimmedName);
  updateLabels();
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
  infoLabel_->setStyleSheet(ssheet);
}

void LayerWidget::editName()
{
  QString name = layer_->name();

  while (true) {
    bool ok;
    name = QInputDialog::getText(this, "Edit layer name", "Layer name:", QLineEdit::Normal, name, &ok).trimmed();
    if (ok) {
      const QSharedPointer<Layer> existingLayer = layerManager->findLayer(name);
      if (!existingLayer) {
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

void LayerWidget::setState(const QSharedPointer<Layer> &layer)
{
  layer_ = layer;
  visibleLabel_->setChecked(layer->isVisible());
  unsavedChangesLabel_->setChecked(layer->hasUnsavedChanges());
  nameLabel_->setText(layer->name());
}

void LayerWidget::showInfo(bool checked)
{
  infoLabel_->setVisible(checked);
  updateLabels();
}

void LayerWidget::updateLabels()
{
  const QSharedPointer<LayerGroup> &layerGroup = layer_->layerGroupRef();

  const QString nameText = QString("%1").arg((layerGroup->name() == "default") ? QString("<b>%1</b>").arg(layer_->name()) : layer_->name());
  nameLabel_->setText(nameText);

  const QString infoText =
      QString(" (editable=%1, nitems=%2, selected items=%3)")
      .arg(layer_->isEditable())
      .arg(layer_->itemCount())
      .arg(layer_->selectedItemCount());
  infoLabel_->setText(infoText);

//  const QString ssheet(layerGroup->isActive() ? "QLabel { background-color : #f27b4b; color : black; }" : "");
//  nameLabel_->setStyleSheet(ssheet);
//  infoLabel_->setStyleSheet(ssheet);
}

void LayerWidget::handleVisibilityChanged(bool visible)
{
  layer_->setVisible(visible);
  emit visibilityChanged(visible);
}

ActiveLayersPane::ActiveLayersPane(EditItems::LayerManager *layerManager)
  : showInfo_(false)
  , layerManager(layerManager)
{
  QVBoxLayout *vboxLayout1 = new QVBoxLayout;
  vboxLayout1->setContentsMargins(0, 2, 0, 2);

  QWidget *activeLayersWidget = new QWidget;
  activeLayersWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout_ = new QVBoxLayout(activeLayersWidget);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  layout_->setMargin(0);

  scrollArea_ = new ScrollArea;
  scrollArea_->setWidget(activeLayersWidget);
  scrollArea_->setWidgetResizable(true);

  vboxLayout1->addWidget(scrollArea_);

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(addEmptyButton_ = createToolButton(QPixmap(addempty_xpm), "Add an empty layer", this, SLOT(addEmpty())));
  bottomLayout->addWidget(mergeVisibleButton_ = createToolButton(QPixmap(mergevisible_xpm), "Merge visible layers into a new layer", this, SLOT(mergeVisible())));
  bottomLayout->addWidget(showAllButton_ = createToolButton(QPixmap(showall_xpm), "Show all layers", this, SLOT(showAll())));
  bottomLayout->addWidget(hideAllButton_ = createToolButton(QPixmap(hideall_xpm), "Hide all layers", this, SLOT(hideAll())));
  bottomLayout->addWidget(duplicateCurrentButton_ = createToolButton(QPixmap(duplicate_xpm), "Duplicate the current layer", this, SLOT(duplicateCurrent())));
  bottomLayout->addWidget(removeCurrentButton_ = createToolButton(QPixmap(remove_xpm), "Remove the current layer", this, SLOT(removeCurrent())));
  bottomLayout->addWidget(moveCurrentUpButton_ = createToolButton(QPixmap(moveup_xpm), "Move the current layer up", this, SLOT(moveCurrentUp())));
  bottomLayout->addWidget(moveCurrentDownButton_ = createToolButton(QPixmap(movedown_xpm), "Move the current layer down", this, SLOT(moveCurrentDown())));
  bottomLayout->addWidget(editCurrentButton_ = createToolButton(QPixmap(edit_xpm), "Edit the current layer", this, SLOT(editCurrent())));
  bottomLayout->addWidget(saveVisibleButton_ = createToolButton(QPixmap(filesave), "Save visible layers to file", this, SLOT(saveVisible())));
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  vboxLayout1->addLayout(bottomLayout);

  QGroupBox *groupBox = new QGroupBox("Active Layers");
  groupBox->setLayout(vboxLayout1);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(groupBox);
  setLayout(mainLayout);

  updateWidgetStructure();
  updateButtons();
}

void ActiveLayersPane::showInfo(bool checked)
{
  for (int i = 0; i < layout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget());
    layerWidget->showInfo(checked);
  }
  showInfo_ = checked;
}

void ActiveLayersPane::initialize(LayerWidget *layerWidget)
{
  connect(layerWidget, SIGNAL(mouseClicked(QMouseEvent *)), SLOT(mouseClicked(QMouseEvent *)), Qt::UniqueConnection);
  connect(layerWidget, SIGNAL(mouseDoubleClicked(QMouseEvent *)), SLOT(mouseDoubleClicked(QMouseEvent *)), Qt::UniqueConnection);
  connect(layerWidget, SIGNAL(visibilityChanged(bool)), SLOT(handleWidgetsUpdate()), Qt::UniqueConnection);
  if (layerWidget->layer()) {
    layerWidget->updateLabels();
    connect(layerWidget->layer().data(), SIGNAL(updated()), SIGNAL(updated()), Qt::UniqueConnection);
    connect(layerWidget->layer().data(), SIGNAL(updated()), layerWidget, SLOT(updateLabels()), Qt::UniqueConnection);
  }
}

void ActiveLayersPane::keyPressEvent(QKeyEvent *event)
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
    QWidget::keyPressEvent(event);
  }
}

void ActiveLayersPane::setCurrentIndex(int index)
{
  setCurrent(atPos(index));
}

void ActiveLayersPane::setCurrent(LayerWidget *layerWidget)
{
  if (!layerWidget)
    return;
  layerManager->setCurrentLayer(layerWidget->layer());
  updateCurrent();
  ensureVisible(layerWidget);
  handleWidgetsUpdate();
}

void ActiveLayersPane::updateCurrent()
{
  for (int i = 0; i < layout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget());
    layerWidget->setCurrent(layerWidget->layer() == layerManager->currentLayer());
  }
}

// Returns the index of the current layer, or -1 if not found.
int ActiveLayersPane::currentPos() const
{
  for (int i = 0; i < layout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget());
    if (layerWidget->layer() == layerManager->currentLayer())
      return i;
  }
  return -1;
}

LayerWidget *ActiveLayersPane::current()
{
  return atPos(currentPos());
}

LayerWidget *ActiveLayersPane::atPos(int pos)
{
  if (pos >= 0 && pos < layout_->count())
    return qobject_cast<LayerWidget *>(layout_->itemAt(pos)->widget());
  return 0;
}

void ActiveLayersPane::duplicate(LayerWidget *srcLayerWidget)
{
  const QSharedPointer<Layer> newLayer = layerManager->createDuplicateLayer(srcLayerWidget->layer());
  layerManager->addToDefaultLayerGroup(newLayer);
  const int srcIndex = layout_->indexOf(srcLayerWidget);
  LayerWidget *newLayerWidget = new LayerWidget(layerManager, newLayer, showInfo_);
  layout_->insertWidget(srcIndex + 1, newLayerWidget);
  initialize(newLayerWidget);
  setCurrent(newLayerWidget);
  ensureCurrentVisible();
  handleWidgetsUpdate();
}

void ActiveLayersPane::duplicateCurrent()
{
  duplicate(current());
}

void ActiveLayersPane::remove(LayerWidget *layerWidget, bool widgetOnly)
{
  if ((!layerWidget) || (layout_->count() == 0))
    return;

  if ((!widgetOnly) && (!layerWidget->layer()->isEmpty()) &&
      (QMessageBox::warning(
         this, "Remove layer", "Really remove layer?",
         QMessageBox::Yes | QMessageBox::No) == QMessageBox::No))
    return;

  const int index = layout_->indexOf(layerWidget);
  layout_->removeWidget(layerWidget);
  if (!widgetOnly)
    layerManager->removeLayer(layerWidget->layer());
  delete layerWidget;
  if (layout_->count() > 0)
    setCurrentIndex(qMin(index, layout_->count() - 1));

  if (!widgetOnly)
    handleWidgetsUpdate();
}

void ActiveLayersPane::remove(int index)
{
  remove(atPos(index));
}

void ActiveLayersPane::removeCurrent()
{
  remove(current());
}

void ActiveLayersPane::move(LayerWidget *layerWidget, bool up)
{
  const int index = layout_->indexOf(layerWidget);
  const int dstIndex = index + (up ? -1 : 1);
  if ((dstIndex < 0) || (dstIndex >= (layout_->count())))
    return;
  const QSharedPointer<Layer> dstLayer = atPos(dstIndex)->layer();
  layout_->removeWidget(layerWidget);
  layout_->insertWidget(dstIndex, layerWidget);
  updateCurrent();
  layerManager->moveLayer(layerWidget->layer(), dstLayer);
  if (layerWidget == current())
    ensureCurrentVisible();
  handleWidgetsUpdate();
}

void ActiveLayersPane::moveUp(LayerWidget *layerWidget)
{
  move(layerWidget, true);
}

void ActiveLayersPane::moveUp(int index)
{
  moveUp(atPos(index));
}

void ActiveLayersPane::moveCurrentUp()
{
  moveUp(current());
}

void ActiveLayersPane::moveDown(LayerWidget *layerWidget)
{
  move(layerWidget, false);
}

void ActiveLayersPane::moveDown(int index)
{
  moveDown(atPos(index));
}

void ActiveLayersPane::moveCurrentDown()
{
  moveDown(current());
}

void ActiveLayersPane::editCurrent()
{
  current()->editName(); // ### only the name for now
}

void ActiveLayersPane::saveVisible() const
{
  const QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save File"),
    DrawingManager::instance()->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));
  if (fileName.isEmpty())
    return;

  QString error = saveVisible(fileName);

  if (!error.isEmpty())
    QMessageBox::warning(0, "Error", QString("failed to save visible layers to file: %1").arg(error));
}

QString ActiveLayersPane::saveVisible(const QString &fileName) const
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  const QList<QSharedPointer<Layer> > visLayers = layers(visibleWidgets());
  QString error;
  KML::saveToFile(fileName, visLayers, &error);
  QApplication::restoreOverrideCursor();

  return error;
}

void ActiveLayersPane::mouseClicked(QMouseEvent *event)
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

void ActiveLayersPane::mouseDoubleClicked(QMouseEvent *event)
{
  if (event->button() & Qt::LeftButton)
    current()->editName();
}

void ActiveLayersPane::ensureVisible(LayerWidget *layer)
{
  qApp->processEvents();
  scrollArea_->ensureWidgetVisible(layer);
}

void ActiveLayersPane::ensureCurrentVisibleTimeout()
{
  ensureVisible(current());
}

void ActiveLayersPane::ensureCurrentVisible()
{
  QTimer::singleShot(0, this, SLOT(ensureCurrentVisibleTimeout()));
}

void ActiveLayersPane::add(const QSharedPointer<Layer> &layer, bool skipUpdate)
{
  LayerWidget *layerWidget = new LayerWidget(layerManager, layer, showInfo_);
  layout_->addWidget(layerWidget);
  initialize(layerWidget);
  if (!skipUpdate) {
    setCurrent(layerWidget);
    ensureCurrentVisible();
    handleWidgetsUpdate();
  }
}

void ActiveLayersPane::addEmpty()
{
  const QSharedPointer<Layer> newLayer = layerManager->createNewLayer();
  layerManager->addToDefaultLayerGroup(newLayer);
  add(newLayer);
}

void ActiveLayersPane::mergeVisible()
{
  const QList<LayerWidget *> visLayerWidgets = visibleWidgets();

  if (visLayerWidgets.size() > 1) {
    if (QMessageBox::warning(
          this, "Merge visible layers", "Really merge visible layers?",
          QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      return;

    const QSharedPointer<Layer> newLayer = layerManager->createNewLayer("merged layer");
    layerManager->addToDefaultLayerGroup(newLayer);

    layerManager->mergeLayers(layers(visLayerWidgets), newLayer);

    LayerWidget *newLayerWidget = new LayerWidget(layerManager, newLayer, showInfo_);
    layout_->insertWidget(layout_->count(), newLayerWidget);
    initialize(newLayerWidget);
    setCurrent(newLayerWidget);
    ensureCurrentVisible();
    handleWidgetsUpdate();

    // ### remove source layers (i.e. those associated with visLayerWidgets) ... TBD
  }

  handleWidgetsUpdate();
}

void ActiveLayersPane::setAllVisible(bool visible)
{
  for (int i = 0; i < layout_->count(); ++i)
    qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget())->setLayerVisible(visible);
  handleWidgetsUpdate();
}

void ActiveLayersPane::showAll()
{
  setAllVisible(true);
}

void ActiveLayersPane::hideAll()
{
  setAllVisible(false);
}

QList<LayerWidget *> ActiveLayersPane::visibleWidgets() const
{
  QList<LayerWidget *> visLayerWidgets;
  for (int i = 0; i < layout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget());
    if (layerWidget->isLayerVisible())
      visLayerWidgets.append(layerWidget);
  }
  return visLayerWidgets;
}

QList<LayerWidget *> ActiveLayersPane::allWidgets() const
{
  QList<LayerWidget *> allLayerWidgets;
  for (int i = 0; i < layout_->count(); ++i) {
    LayerWidget *layerWidget = qobject_cast<LayerWidget *>(layout_->itemAt(i)->widget());
    allLayerWidgets.append(layerWidget);
  }
  return allLayerWidgets;
}

QList<QSharedPointer<Layer> > ActiveLayersPane::layers(const QList<LayerWidget *> &layerWidgets) const
{
  QList<QSharedPointer<Layer> > layers_;
  foreach (LayerWidget *layerWidget, layerWidgets)
    layers_.append(layerWidget->layer());
  return layers_;
}

void ActiveLayersPane::updateButtons()
{
  const int visSize = visibleWidgets().size();
  const int allSize = layout_->count();
  mergeVisibleButton_->setEnabled(visSize > 1);
  showAllButton_->setEnabled(visSize < allSize);
  hideAllButton_->setEnabled(visSize > 0);
  duplicateCurrentButton_->setEnabled(current());

  // the layer may be removed iff it is 1) current, 2) editable, and 3) not the last layer in the default layer group
  removeCurrentButton_->setEnabled(
        current() && current()->layer()->isEditable()
        && !((current()->layer()->layerGroupRef() == layerManager->defaultLayerGroup())
            && (layerManager->defaultLayerGroup()->layersRef().size() == 1)));

  moveCurrentUpButton_->setEnabled(currentPos() > 0);
  moveCurrentDownButton_->setEnabled(currentPos() < (allSize - 1));
  editCurrentButton_->setEnabled(current());
  saveVisibleButton_->setEnabled(visSize > 0);
#if 0 // disabled for now
  importFilesButton_->setEnabled(drawingList->selectionModel()->selection().size() != 0);
#endif
}

void ActiveLayersPane::handleWidgetsUpdate()
{
  updateButtons();
  emit updated();
}

// Updates widget structure (i.e. number and order of widgets).
void ActiveLayersPane::updateWidgetStructure()
{
  QList<QSharedPointer<Layer> > activeLayers;
  foreach (const QSharedPointer<Layer> &layer, layerManager->orderedLayers()) {
    if (layer->isActive())
      activeLayers.append(layer);
  }

  // add/remove widgets to match the number of layers
  const int diff = layout_->count() - activeLayers.size();
  if (diff < 0) {
    for (int i = 0; i < -diff; ++i) {
      LayerWidget *layerWidget = new LayerWidget(layerManager, QSharedPointer<Layer>(), showInfo_);
      layout_->insertWidget(0, layerWidget);
    }
  } else if (diff > 0) {
    for (int i = 0; i < diff; ++i)
      remove(atPos(0), true);
  }

  // ### for now (remove when tested):
  if (layout_->count() != activeLayers.size())
    qFatal("ASSERTION FAILED: layout_->count()=%d != activeLayers.size()=%d",
           layout_->count(), activeLayers.size());

  // update widget contents and current index
  int currIndex = -1;
  for (int i = 0; i < activeLayers.size(); ++i) {
    LayerWidget *layerWidget = atPos(i);
    QSharedPointer<Layer> layer = activeLayers.at(i);
    layerWidget->setState(layer);
    initialize(layerWidget);
    if (activeLayers.at(i) == layerManager->currentLayer())
      currIndex = i;
  }
  Q_ASSERT(currIndex >= 0);
  setCurrentIndex(currIndex);
}

} // namespace
