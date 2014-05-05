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

#include <diEditItemManager.h>
#include <diController.h>
#include <EditItems/dialog.h>
#include <EditItems/toolbar.h>
#include <EditItems/layergroupspane.h>
#include <EditItems/activelayerspane.h>
#include <paint_mode.xpm> // ### for now
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QSplitter>
#include <QVBoxLayout>

#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>
#include <QPushButton> // ### FOR TESTING
#include <QCheckBox> // ### FOR TESTING
#include <QDir>
#include <QDebug>

namespace EditItems {

Dialog::Dialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  // record the editor in use
  editm_ = EditItemManager::instance();

  // create an action that can be used to open the dialog from within a menu or toolbar
  m_action = new QAction(QIcon(QPixmap(paint_mode_xpm)), tr("Painting tools"), this);
  m_action->setShortcutContext(Qt::ApplicationShortcut);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  connect(m_action, SIGNAL(toggled(bool)), SLOT(toggleEditingMode(bool)));

#if 0 // disabled for now
  // Populate the drawing model with data from the drawing manager.
  updateModel();
#endif

  // create the GUI
  setWindowTitle("Drawing Layers");
  setFocusPolicy(Qt::StrongFocus);
  QSplitter *splitter = new QSplitter(Qt::Vertical);
  layerGroupsPane_ = new LayerGroupsPane;
  splitter->addWidget(layerGroupsPane_);
  activeLayersPane_ = new ActiveLayersPane;
  splitter->addWidget(activeLayersPane_);
  splitter->setSizes(QList<int>() << 500 << 500);
  //
  connect(layerGroupsPane_, SIGNAL(updated()), activeLayersPane_, SLOT(updateWidgetStructure()));
  connect(activeLayersPane_, SIGNAL(updated()), layerGroupsPane_, SLOT(updateWidgetContents()));
  connect(layerGroupsPane_, SIGNAL(updated()), EditItemManager::instance(), SLOT(handleLayersUpdate()));
  connect(activeLayersPane_, SIGNAL(updated()), EditItemManager::instance(), SLOT(handleLayersUpdate()));
  //
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(close()));
  //
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(splitter);
  //
  mainLayout->addWidget(buttonBox);

  mainLayout->addLayout(createStandardButtons());

  // Connect the signal associated with product creation (applyData) to a slot
  // that publishes it outside editing mode.
  connect(this, SIGNAL(applyData()), SLOT(makeProduct()));

  // ### FOR TESTING
  QHBoxLayout *bottomLayout = new QHBoxLayout;
  //
  QPushButton *dsButton = new QPushButton("dump structure");
  connect(dsButton, SIGNAL(clicked()), SLOT(dumpStructure()));
  dsButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  bottomLayout->addWidget(dsButton);
  //
  QCheckBox *infoCBox = new QCheckBox("show info");
  connect(infoCBox, SIGNAL(toggled(bool)), SLOT(showInfo(bool)));
  infoCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  bottomLayout->addWidget(infoCBox);
  //
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));
  mainLayout->addLayout(bottomLayout);
}

void Dialog::dumpStructure()
{
  qDebug() << "";
  const QList<QSharedPointer<LayerGroup> > &layerGroups = LayerManager::instance()->layerGroups();
  qDebug() << QString("LAYER GROUPS (%1):").arg(layerGroups.size()).toLatin1().data();
  int i = 0;
  foreach (const QSharedPointer<LayerGroup> &lg, layerGroups) {
    QString layers_s;
    const QList<QSharedPointer<Layer> > layers = lg->layersRef();
    foreach (QSharedPointer<Layer> layer, layers)
      layers_s.append(QString("%1 ").arg((long)(layer.data()), 0, 16));
    qDebug()
        <<
           QString("  %1:%2  %3  >%4<  [%5]  editable=%6  active=%7  layers: %8")
           .arg(i + 1)
           .arg(layerGroups.size())
           .arg((long)(lg.data()), 0, 16)
           .arg(lg->name(), 30)
           .arg((lg == LayerManager::instance()->defaultLayerGroup()) ? "default" : "       ")
           .arg(lg->isEditable() ? 1 : 0)
           .arg(lg->isActive() ? 1 : 0)
           .arg(layers_s)
           .toLatin1().data();
    i++;
  }

  const QList<QSharedPointer<Layer> > &layers = LayerManager::instance()->orderedLayers();
  qDebug() << QString("ORDERED LAYERS (%1):").arg(layers.size()).toLatin1().data();
  i = 0;
  foreach (const QSharedPointer<Layer> &layer, layers) {
    qDebug()
        <<
           QString("  %1:%2  %3  >%4<  [%5]  LG:%6  editable:%7  active:%8  visible:%9  nItems:%10  nSelItems:%11")
           .arg(i + 1)
           .arg(layers.size())
           .arg((long)(layer.data()), 0, 16)
           .arg(layer->name(), 30)
           .arg(layer == LayerManager::instance()->currentLayer() ? "curr" : "    ")
           .arg((long)(layer->layerGroupRef().data()), 0, 16)
           .arg(layer->isEditable() ? 1 : 0)
           .arg(layer->isActive() ? 1 : 0)
           .arg(layer->isVisible() ? 1 : 0)
           .arg(layer->itemCount())
           .arg(layer->selectedItemCount())
           .toLatin1().data();
    i++;
  }
}

void Dialog::showInfo(bool checked)
{
  layerGroupsPane_->showInfo(checked);
  activeLayersPane_->showInfo(checked);
}

std::string Dialog::name() const
{
  return "DRAWING";
}

void Dialog::updateTimes()
{
  std::vector<miutil::miTime> times;

  if (editm_->isEnabled() && !editm_->isEditing())
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
  editm_->processInput(inp);
}

void Dialog::toggleEditingMode(bool enable)
{
  // Enabling editing mode (opening the dialog) causes the manager to enter
  // working mode. This makes it possible to show objects that have not been
  // serialised as plot commands.
  editm_->setEditing(enable);

  // When editing starts, remove any existing items and load the chosen
  // files. Mark the product as unfinished by disabling it.
  if (enable)
    editm_->setEnabled(false);

  ToolBar::instance()->setVisible(enable);
  ToolBar::instance()->setEnabled(enable);
}

/**
 * Makes the current drawing a product that is visible outside editing mode.
 */
void Dialog::makeProduct()
{
  // Write the layers to temporary files in the working directory.
  QDir dir(editm_->getWorkDir());
  QString filePath = dir.absoluteFilePath("temp.kml");
  activeLayersPane_->saveVisible(filePath);
  std::cerr << "makeProduct " << filePath.toStdString() << std::endl;

  std::vector<std::string> inp;
  inp.push_back("DRAWING file=\"" + filePath.toStdString() + "\"");

  putOKString(inp);

  // Update the available times.
  //updateTimes();
}

#if 0 // disabled for now
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
#endif

} // namespace
