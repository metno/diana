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

#include "diDrawingManager.h"
#include "diController.h"

#include "EditItems/drawingdialog.h"
#include "EditItems/editpolyline.h"
#include "EditItems/editsymbol.h"
#include "EditItems/edittext.h"
#include "EditItems/editcomposite.h"
#include "EditItems/kml.h"
#include "EditItems/layergroup.h"
#include "EditItems/toolbar.h"

#include "qtUtility.h"

#include <drawing.xpm> // ### for now

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

namespace EditItems {

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  drawm_ = DrawingManager::instance();

  // create an action that can be used to open the dialog from within a menu or toolbar
  m_action = new QAction(QIcon(QPixmap(drawing_xpm)), tr("Drawing Dialog"), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);

  // Populate the dialog with the drawings held by the drawing manager.
  drawingsModel_.setItems(drawm_->getDrawings());

  // Create the GUI.
  setWindowTitle(tr("Drawing Dialog"));
  setFocusPolicy(Qt::StrongFocus);

  QListView *drawingsList = new QListView();
  drawingsList->setModel(&drawingsModel_);
  drawingsList->setSelectionMode(QAbstractItemView::MultiSelection);

  QListView *activeList = new QListView();
  activeList->setModel(&activeDrawingsModel_);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(TitleLabel(tr("Available Drawings"), this));
  mainLayout->addWidget(drawingsList);
  mainLayout->addWidget(TitleLabel(tr("Active Drawings"), this));
  mainLayout->addWidget(activeList);

  mainLayout->addLayout(createStandardButtons());

  connect(drawingsList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          SLOT(activateDrawing(const QItemSelection &, const QItemSelection &)));
  connect(this, SIGNAL(applyData()), SLOT(makeProduct()));
}

/**
 * Returns the name of the component. This is used to relate the dialog to the
 * corresponding manager, which also shares the same name.
 */
std::string DrawingDialog::name() const
{
  return "DRAWING";
}

void DrawingDialog::updateTimes()
{
  std::vector<miutil::miTime> times;
  times = drawm_->getTimes();
  emit emitTimes("DRAWING", times);
}

void DrawingDialog::updateDialog()
{
}

std::vector<std::string> DrawingDialog::getOKString()
{
  std::vector<std::string> lines;

  if (!drawm_->isEnabled())
    return lines;

  QMap<QString, QString> loaded = drawm_->getLoaded();
  foreach (QString name, loaded.keys()) {
    QString line;
    QString fileName = loaded[name];
    if (fileName == name)
      line = "DRAWING file=\"" + fileName + "\"";
    else
      line = "DRAWING name=\"" + name + "\"";
    lines.push_back(line.toStdString());
  }

  return lines;
}

void DrawingDialog::putOKString(const std::vector<std::string>& vstr)
{
  // Submit the lines as new input.
  std::vector<std::string> inp;
  inp.insert(inp.begin(), vstr.begin(), vstr.end());
  drawm_->processInput(inp);
}

void DrawingDialog::activateDrawing(const QItemSelection &selected, const QItemSelection &deselected)
{
  QMap<QString, QString> activeDrawings = activeDrawingsModel_.items();

  // Read the names of deselected and selected drawings, removing from and
  // adding to the list of active drawings as necessary.
  foreach (const QModelIndex &index, deselected.indexes())
    activeDrawings.remove(index.data().toString());

  foreach (const QModelIndex &index, selected.indexes())
    activeDrawings[index.data().toString()] = index.data(Qt::UserRole).toString();

  activeDrawingsModel_.setItems(activeDrawings);
}

void DrawingDialog::makeProduct()
{
  // Compile a list of strings describing the files in use.
  std::vector<std::string> inp;
  foreach (const QString &name, activeDrawingsModel_.items().keys())
    inp.push_back("DRAWING name=\"" + name.toStdString() + "\"");

  putOKString(inp);

  // Update the available times.
  updateTimes();

  indicateUnappliedChanges(false); // indicate that the DrawingManager is now synchronized with the dialog
}

void DrawingDialog::handleDialogUpdated()
{
  indicateUnappliedChanges(true);
  //layersPane_->updateButtons();
}


DrawingModel::DrawingModel(QObject *parent)
  : QAbstractListModel(parent)
{
}

DrawingModel::~DrawingModel()
{
}

QModelIndex DrawingModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid()) {
    if (row >= 0 && row < items_.size() && column == 0)
      return createIndex(row, column, -1);
  }
  return QModelIndex();
}

int DrawingModel::rowCount(const QModelIndex &parent) const
{
  if (!parent.isValid())
    return items_.size();
  else
    return 0;
}

QVariant DrawingModel::data(const QModelIndex &index, int role) const
{
  if (index.isValid() && index.row() >= 0 && index.row() < items_.size()) {
    switch (role) {
    case Qt::DisplayRole:
      return QVariant(items_.keys().at(index.row()));
    case Qt::UserRole:
      return QVariant(items_.values().at(index.row()));
    default:
      ;
    }
  }

  return QVariant();
}

QVariant DrawingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (section == 0)
    return tr("Name");
  else if (section == 1)
    return tr("Source");

  return QVariant();
}

Qt::ItemFlags DrawingModel::flags(const QModelIndex &index) const
{
  if (index.isValid())
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  else
    return Qt::ItemIsEnabled;
}

QMap<QString, QString> DrawingModel::items() const
{
  return items_;
}

void DrawingModel::setItems(const QMap<QString, QString> &items)
{
  beginResetModel();
  items_ = items;
  endResetModel();
}

} // namespace
