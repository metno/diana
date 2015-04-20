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
#include <EditItems/kml.h>
#include <EditItems/edititembase.h>
#include <EditItems/editpolyline.h>
#include <EditItems/editsymbol.h>
#include <EditItems/edittext.h>
#include <EditItems/editcomposite.h>
#include <EditItems/layergroupspane.h>
#include <EditItems/dialogcommon.h>
#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>

#include "fileopen.xpm"

#include <QApplication>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QToolButton>

#define MILOGGER_CATEGORY "diana.LayerGroupsPane"
#include <miLogger/miLogging.h>

namespace EditItems {

LayerGroupWidget::LayerGroupWidget(const QSharedPointer<LayerGroup> &layerGroup, bool showInfo, QWidget *parent)
  : QWidget(parent)
  , layerGroup_(layerGroup)
{
  setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);

  nameLabel_ = new ClickableLabel;
  nameLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  connect(nameLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  mainLayout->addWidget(nameLabel_);

  infoLabel_ = new ClickableLabel("<info>");
  infoLabel_->setVisible(showInfo);
  infoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  connect(infoLabel_, SIGNAL(mouseClicked(QMouseEvent *)), SIGNAL(mouseClicked(QMouseEvent *)));
  mainLayout->addWidget(infoLabel_);

  mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred));

  updateLabels();
}

LayerGroupWidget::~LayerGroupWidget()
{
}

void LayerGroupWidget::updateLabels()
{
  const QString nameText = QString("%1").arg(layerGroup_->name());
  nameLabel_->setText(nameText);

  int nitems = 0;
  foreach (const QSharedPointer<Layer> &layer, layerGroup_->layersRef())
    nitems += layer->itemCount();
  const QString infoText =
      QString(" (editable=%1, nlayers=%2, nitems=%3)")
      .arg(layerGroup_->isEditable() ? 1 : 0)
      .arg(layerGroup_->layersRef().size())
      .arg(nitems);
  infoLabel_->setText(infoText);

  const QString ssheet(layerGroup_->isActive() ? "QLabel { background-color : #f27b4b; color : black; }" : "");
  nameLabel_->setStyleSheet(ssheet);
  infoLabel_->setStyleSheet(ssheet);
}

void LayerGroupWidget::showInfo(bool checked)
{
  infoLabel_->setVisible(checked);
  updateLabels();
}

const QSharedPointer<LayerGroup> LayerGroupWidget::layerGroup() const
{
  return layerGroup_;
}

LayerGroupsPane::LayerGroupsPane(LayerManager *layerManager)
  : showInfo_(false)
  , layerMgr_(layerManager)
{
  QVBoxLayout *vboxLayout1 = new QVBoxLayout;
  vboxLayout1->setContentsMargins(0, 2, 0, 2);

  QWidget *LayerGroupsWidget = new QWidget;
  LayerGroupsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout_ = new QVBoxLayout(LayerGroupsWidget);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  layout_->setMargin(0);

  scrollArea_ = new ScrollArea;
  scrollArea_->setWidget(LayerGroupsWidget);
  scrollArea_->setWidgetResizable(true);

  vboxLayout1->addWidget(scrollArea_);

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(addToNewLGFromFileButton_ = createToolButton(
        QPixmap(fileopen_xpm), "Load layers from file into a new layer group", this, SLOT(addToNewLGFromFile())));
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  vboxLayout1->addLayout(bottomLayout);

  QGroupBox *groupBox = new QGroupBox(tr("Layer Groups"));
  groupBox->setLayout(vboxLayout1);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(groupBox);
  setLayout(mainLayout);

  updateWidgetStructure();

#if 0 // disabled for now
  QGroupBox *groupBox = new QGroupBox(tr("Layer Collections"));

  drawingList = new QListView();
  drawingList->setModel(&drawingModel);
  drawingList->setSelectionMode(QAbstractItemView::MultiSelection);
  drawingList->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(drawingList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateButtons()));

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(importFilesButton_ = createToolButton(QPixmap(movedown_xpm), "Import files as layer", this, SLOT(importChosenFiles())));
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));

  QVBoxLayout *layout = new QVBoxLayout(groupBox);
  layout->setContentsMargins(0, 2, 0, 2);
  layout->addWidget(drawingList);
  layout->addLayout(bottomLayout);
#endif
}

void LayerGroupsPane::showInfo(bool checked)
{
  for (int i = 0; i < layout_->count(); ++i) {
    LayerGroupWidget *lgWidget = qobject_cast<LayerGroupWidget *>(layout_->itemAt(i)->widget());
    lgWidget->showInfo(checked);
  }
  showInfo_ = checked;
}

void LayerGroupsPane::addWidgetForLG(const QSharedPointer<LayerGroup> &layerGroup)
{
  LayerGroupWidget *lgWidget = new LayerGroupWidget(layerGroup, showInfo_);
  layout_->addWidget(lgWidget);
  connect(lgWidget, SIGNAL(mouseClicked(QMouseEvent *)), SLOT(mouseClicked(QMouseEvent *)));
}

void LayerGroupsPane::addToLGFromFile()
{
  QString error;
  const QList<QSharedPointer<Layer> > layers = createLayersFromFile(layerMgr_, true, &error);
  if (!error.isEmpty()) {
    QMessageBox::warning(0, "Error", QString("failed to add to layer group from file: %1").arg(error));
    return;
  }

  if (layers.isEmpty())
      return;

  layerMgr_->addToNewLayerGroup(layers);
  EditItemManager::instance()->updateJoins();

  emit updated();
  updateWidgetContents();
  updateWidgetStructure(); // add widget for the new layer group
}

void LayerGroupsPane::addToNewLGFromFile()
{
  addToLGFromFile();
}

void LayerGroupsPane::mouseClicked(QMouseEvent *event)
{
  METLIBS_LOG_SCOPE();

  LayerGroupWidget *lgWidget = qobject_cast<LayerGroupWidget *>(sender());
  Q_ASSERT(lgWidget);
// FIXME defaultLayerGroup does not exist  Q_ASSERT(lgWidget->layerGroup() != layerMgr_->defaultLayerGroup());
  const bool active = !lgWidget->layerGroup()->isActive();
  lgWidget->layerGroup()->setActive(active);

  // Only load the contents of a file when the layer group is selected.
  if (active)
    loadLayers(lgWidget);

  lgWidget->updateLabels();

  emit updated();
}

/**
 * Load the layers referred to by the given \a lgWidget.
*/
void LayerGroupsPane::loadLayers(LayerGroupWidget *lgWidget)
{
  QString source = lgWidget->layerGroup()->name();
  layerMgr_->addToNewLayerGroup(lgWidget->layerGroup(), source);
}

QList<LayerGroupWidget *> LayerGroupsPane::allWidgets()
{
  QList<LayerGroupWidget *> allLGWidgets;
  for (int i = 0; i < layout_->count(); ++i) {
    LayerGroupWidget *lgWidget = qobject_cast<LayerGroupWidget *>(layout_->itemAt(i)->widget());
    allLGWidgets.append(lgWidget);
  }
  return allLGWidgets;
}

void LayerGroupsPane::removeWidget(LayerGroupWidget *lgWidget)
{
  if ((!lgWidget) || (layout_->count() == 0))
    return;
  layout_->removeWidget(lgWidget);
  delete lgWidget;
}

// Updates internal contents of widgets.
void LayerGroupsPane::updateWidgetContents()
{
  for (int i = 0; i < layout_->count(); ++i) {
    LayerGroupWidget *lgWidget = qobject_cast<LayerGroupWidget *>(layout_->itemAt(i)->widget());
    lgWidget->updateLabels();
  }
}

// Updates widget structure (i.e. number and order of widgets).
void LayerGroupsPane::updateWidgetStructure()
{
  // clear existing widgets
  QList<LayerGroupWidget *> lgWidgets = allWidgets();
  for (int i = 0; i < lgWidgets.size(); ++i)
    removeWidget(lgWidgets.at(i));

  // insert widgets for existing layer groups
  foreach (const QSharedPointer<LayerGroup> &layerGroup, layerMgr_->layerGroups())
    addWidgetForLG(layerGroup);
}

} // namespace
