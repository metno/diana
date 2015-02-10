/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2015 met.no

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
#include <EditItems/editdrawingdialog.h>
#include <EditItems/toolbar.h>
#include <EditItems/layergroupspane.h>
#include <EditItems/editdrawinglayerspane.h>
#include <editdrawing.xpm> // ### for now
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QVBoxLayout>

#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>
#include <QPushButton>
#include <QCheckBox>
#include <QDir>
#include <qtUtility.h>

#ifdef ENABLE_EDITDRAWINGDIALOG_TESTING
#include <QDebug>
#endif // ENABLE_EDITDRAWINGDIALOG_TESTING

namespace EditItems {

EditDrawingDialog::EditDrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  editm_ = EditItemManager::instance();

  // create an action that can be used to open the dialog from within a menu or toolbar
  m_action = new QAction(QIcon(QPixmap(editdrawing_xpm)), tr("Edit Drawing Dialog"), this);
  m_action->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);

  // create the GUI
  setWindowTitle("Edit Drawing Dialog");
  setFocusPolicy(Qt::StrongFocus);
  layersPane_ = new EditDrawingLayersPane(editm_->getLayerManager(), "Active Layers");
  layersPane_->init();
  //
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(layersPane_);

  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);
  connect(hideButton, SIGNAL(clicked()), SIGNAL(hideData()));
  QHBoxLayout* hideLayout = new QHBoxLayout();
  hideLayout->addWidget(hideButton);
  mainLayout->addLayout(hideLayout);

#ifdef ENABLE_EDITDRAWINGDIALOG_TESTING
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
  QCheckBox *undoCBox = new QCheckBox("show undo stack");
  connect(undoCBox, SIGNAL(toggled(bool)), SLOT(showUndoStack(bool)));
  undoCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  bottomLayout->addWidget(undoCBox);
  //
  QCheckBox *forceItemsVisibleCBox = new QCheckBox("items always visible");
  connect(forceItemsVisibleCBox, SIGNAL(toggled(bool)), SLOT(setItemsVisibilityForced(bool)));
  forceItemsVisibleCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  bottomLayout->addWidget(forceItemsVisibleCBox);
  //
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));
  mainLayout->addLayout(bottomLayout);
#endif // ENABLE_EDITDRAWINGDIALOG_TESTING

  // add connections
  connect(layersPane_, SIGNAL(updated()), editm_, SLOT(update()));
}

std::string EditDrawingDialog::name() const
{
  return "EDITDRAWING";
}

void EditDrawingDialog::handleNewEditLayerRequested(const QSharedPointer<Layer> &layer)
{

  layersPane_->addDuplicate(layer);
}

#ifdef ENABLE_EDITDRAWINGDIALOG_TESTING
static void dumpLayerManagerStructure(const LayerManager *lm)
{
  const QList<QSharedPointer<LayerGroup> > &layerGroups = lm->layerGroups();
  qDebug() << QString("LAYER GROUPS (%1):").arg(layerGroups.size()).toLatin1().data();
  int i = 0;
  foreach (const QSharedPointer<LayerGroup> &lg, layerGroups) {
    QString layers_s;
    const QList<QSharedPointer<Layer> > layers = lg->layersRef();
    foreach (QSharedPointer<Layer> layer, layers)
      layers_s.append(QString("%1 ").arg((long)(layer.data()), 0, 16));
    qDebug()
        <<
           QString("  %1:%2  %3  >%4<  editable=%5  active=%6  layers: %7")
           .arg(i + 1)
           .arg(layerGroups.size())
           .arg((long)(lg.data()), 0, 16)
           .arg(lg->name(), 30)
           .arg(lg->isEditable() ? 1 : 0)
           .arg(lg->isActive() ? 1 : 0)
           .arg(layers_s)
           .toLatin1().data();
    i++;
  }

  const QList<QSharedPointer<Layer> > &layers = lm->orderedLayers();
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
           .arg(lm->selectedLayers().contains(layer) ? "sel" : "   ")
           .arg((long)(layer->layerGroupRef().data()), 0, 16)
           .arg(layer->isEditable() ? 1 : 0)
           .arg(layer->isActive() ? 1 : 0)
           .arg(layer->isVisible() ? 1 : 0)
           .arg(layer->itemCount())
           .arg(layer->selectedItemCount())
           .toLatin1().data();
    if (layer->srcFiles().isEmpty()) {
      qDebug() << "    src files: none";
    } else {
      qDebug() << "    src files:";
      foreach (const QString srcFile, layer->srcFiles())
        qDebug() << "      " << srcFile.toLatin1().data();
    }
    i++;
  }
}
#endif // ENABLE_EDITDRAWINGDIALOG_TESTING

void EditDrawingDialog::dumpStructure()
{
#ifdef ENABLE_EDITDRAWINGDIALOG_TESTING
  qDebug() << "\nLAYER MANAGERS:";
  qDebug() << "\n1: In EditDrawingManager: =====================================";
  dumpLayerManagerStructure(editm_->getLayerManager());
#endif // ENABLE_EDITDRAWINGDIALOG_TESTING
}

void EditDrawingDialog::showInfo(bool checked)
{
  layersPane_->showInfo(checked);
}

void EditDrawingDialog::showUndoStack(bool checked)
{
  editm_->getUndoView()->setVisible(checked);
}

void EditDrawingDialog::setItemsVisibilityForced(bool checked)
{
  editm_->setItemsVisibilityForced(checked);
}

} // namespace
