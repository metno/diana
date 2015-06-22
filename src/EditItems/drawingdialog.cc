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

#include "diController.h"
#include "diDrawingManager.h"
#include "diEditItemManager.h"

#include "EditItems/drawingdialog.h"
#include "EditItems/filterdrawingdialog.h"
#include "EditItems/editpolyline.h"
#include "EditItems/editsymbol.h"
#include "EditItems/edittext.h"
#include "EditItems/editcomposite.h"
#include "EditItems/itemgroup.h"
#include "EditItems/kml.h"
#include "EditItems/toolbar.h"

#include "qtUtility.h"

#include <drawing.xpm> // ### for now

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace EditItems {

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  drawm_ = DrawingManager::instance();
  editm_ = EditItemManager::instance();

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

  drawingsList_ = new QListView();
  drawingsList_->setModel(&drawingsModel_);
  drawingsList_->setSelectionMode(QAbstractItemView::MultiSelection);

  activeList_ = new QListView();
  activeList_->setModel(&activeDrawingsModel_);
  activeList_->setSelectionMode(QAbstractItemView::MultiSelection);
  activeList_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(activeList_, SIGNAL(customContextMenuRequested(const QPoint &)),
          SLOT(showActiveContextMenu(const QPoint &)));

  QPushButton *loadFileButton = new QPushButton(tr("Load drawing..."));
  loadFileButton->setIcon(qApp->style()->standardIcon(QStyle::SP_FileIcon));
  connect(loadFileButton, SIGNAL(clicked()), SLOT(loadFile()));

  QHBoxLayout *addLayout = new QHBoxLayout();
  addLayout->addWidget(TitleLabel(tr("Products"), this));
  addLayout->addStretch();
  addLayout->addWidget(loadFileButton);

  QPushButton *filterButton = new QPushButton(tr("Filter items..."));
  filterButton->setCheckable(true);
  filterDialog_ = new FilterDrawingDialog(this);
  connect(filterButton, SIGNAL(toggled(bool)), filterDialog_, SLOT(setVisible(bool)));
  connect(filterDialog_, SIGNAL(updated()), SIGNAL(updated()));
  connect(filterDialog_, SIGNAL(finished(int)), filterButton, SLOT(toggle()));

  QToolButton *fileButton = new QToolButton();
  fileButton->setText(tr("File"));
  QMenu *fileMenu = new QMenu(tr("File"), this);
  connect(fileMenu->addAction(tr("Save Visible Items...")), SIGNAL(triggered()), SLOT(saveVisibleItems()));
  connect(fileMenu->addAction(tr("Save All Items...")), SIGNAL(triggered()), SLOT(saveAllItems()));
  fileButton->setMenu(fileMenu);

  editButton_ = new QPushButton(tr("Edit drawings"));
  editButton_->setEnabled(false);
  connect(editButton_, SIGNAL(clicked()), SLOT(editDrawings()));

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addWidget(filterButton);
  buttonLayout->addWidget(fileButton);
  buttonLayout->addWidget(editButton_);
  buttonLayout->addStretch();

  QFrame *editTopSeparator = new QFrame();
  editTopSeparator->setFrameShape(QFrame::HLine);
  QFrame *editBottomSeparator = new QFrame();
  editBottomSeparator->setFrameShape(QFrame::HLine);

  QCheckBox *editModeCheckBox = new QCheckBox(tr("Edit Mode"));
  connect(editModeCheckBox, SIGNAL(toggled(bool)), SIGNAL(editingMode(bool)));
  connect(editm_, SIGNAL(editing(bool)), editModeCheckBox, SLOT(setChecked(bool)));
  QCheckBox *hideEditItemsCheckBox = new QCheckBox(tr("Hide Edit Items"));
  hideEditItemsCheckBox->setEnabled(false);
  connect(hideEditItemsCheckBox, SIGNAL(toggled(bool)), SLOT(hideEditItems(bool)));
  connect(editModeCheckBox, SIGNAL(toggled(bool)), hideEditItemsCheckBox, SLOT(setEnabled(bool)));

  QVBoxLayout *editLayout = new QVBoxLayout();
  editLayout->addWidget(editTopSeparator);
  editLayout->addWidget(TitleLabel(tr("Edit"), this));
  editLayout->addWidget(editModeCheckBox);
  editLayout->addWidget(hideEditItemsCheckBox);
  editLayout->addWidget(editBottomSeparator);

  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);
  QPushButton *applyButton = NormalPushButton(tr("Apply"), this);

  connect(hideButton, SIGNAL(clicked()), SLOT(close()));
  connect(applyButton, SIGNAL(clicked()), SIGNAL(applyData()));

  QHBoxLayout *hideApplyLayout = new QHBoxLayout();
  hideApplyLayout->addWidget(hideButton);
  hideApplyLayout->addStretch();
  hideApplyLayout->addWidget(applyButton);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(addLayout);
  mainLayout->addWidget(drawingsList_);
  mainLayout->addWidget(TitleLabel(tr("Selected Products"), this));
  mainLayout->addWidget(activeList_);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addLayout(editLayout);
  mainLayout->addStretch();
  mainLayout->addLayout(hideApplyLayout);

  connect(drawingsList_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          SLOT(activateDrawing(const QItemSelection &, const QItemSelection &)));
  connect(drawingsList_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          SLOT(updateButtons()));
  connect(activeList_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          SLOT(updateButtons()));
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
}

void DrawingDialog::loadFile()
{
  QString fileName = QFileDialog::getOpenFileName(0, QObject::tr("Open File"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString error = drawm_->loadDrawing(fileName, fileName);
  QApplication::restoreOverrideCursor();

  if (error.isEmpty()) {
    // Update the working directory and the list of available drawings.
    QFileInfo fi(fileName);
    DrawingManager::instance()->setWorkDir(fi.dir().absolutePath());
    // Append a new row and insert the new drawing as an item there.
    drawingsModel_.appendDrawing(fileName, fileName);
  } else {
    QMessageBox warning(QMessageBox::Warning, tr("Open File"),
                        tr("Failed to open file: %1").arg(fileName),
                        QMessageBox::Cancel, this);
    warning.setDetailedText(error);
    warning.exec();
  }
}

void DrawingDialog::saveAllItems()
{
  QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save All Items"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  // Obtain all visible drawing and editing items, filter them for visibility,
  // and save the resulting collection.
  QList<DrawingItemBase *> items = drawm_->allItems() + editm_->allItems();

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString error = KML::saveItemsToFile(items, fileName);
  QApplication::restoreOverrideCursor();

  if (error.isEmpty())
    updateFileInfo(items, fileName);
  else
    QMessageBox::warning(this, tr("Save File"), tr("Failed to save file: %1").arg(fileName));
}

void DrawingDialog::saveVisibleItems()
{
  QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save Visible Items"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  // Obtain all visible drawing and editing items, filter them for visibility,
  // and save the resulting collection.
  QList<DrawingItemBase *> items;

  foreach (DrawingItemBase *item, drawm_->allItems()) {
    if (drawm_->isItemVisible(item))
      items.append(item);
  }

  foreach (DrawingItemBase *item, editm_->allItems()) {
    if (editm_->isItemVisible(item))
      items.append(item);
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString error = KML::saveItemsToFile(items, fileName);
  QApplication::restoreOverrideCursor();

  if (error.isEmpty())
    updateFileInfo(items, fileName);
  else
    QMessageBox::warning(this, tr("Save File"), tr("Failed to save file: %1").arg(fileName));
}

void DrawingDialog::updateFileInfo(const QList<DrawingItemBase *> &items, const QString &fileName)
{
  // Update the working directory and the list of available drawings.
  QFileInfo fi(fileName);
  DrawingManager::instance()->setWorkDir(fi.dir().absolutePath());

  QVariantMap props;
  props["srcFile"] = fileName;
  foreach (DrawingItemBase *item, items)
    editm_->updateItem(item, props);

  editm_->pushUndoCommands();
}

void DrawingDialog::updateButtons()
{
  editButton_->setEnabled(activeList_->selectionModel()->hasSelection());
}

/**
 * Copy the drawings highlighted in the active list to the edit manager.
 */
void DrawingDialog::editDrawings()
{
  // Obtain lists of names and file names.
  QStringList names;
  QStringList fileNames;
  foreach (const QModelIndex &index, activeList_->selectionModel()->selectedIndexes()) {
    names.append(index.data().toString());
    fileNames.append(index.data(Qt::UserRole).toString());
  }

  // Load the drawings into the edit manager and construct a selection that
  // will be used to deselect items in the drawing list.
  QItemSelection selection;

  QApplication::setOverrideCursor(Qt::WaitCursor);

  for (int i = 0; i < names.size(); ++i) {
    QString error = editm_->loadDrawing(names.at(i), fileNames.at(i));
    if (error.isEmpty()) {
      QModelIndex index = drawingsModel_.find(names.at(i));
      selection.select(index, index);
    }
  }

  QApplication::restoreOverrideCursor();

  // Deselect the named drawings from the drawing list in order to remove
  // them from the active list.
  drawingsList_->selectionModel()->select(selection, QItemSelectionModel::Toggle);

  // Deselecting the drawing from the list does not automatically remove it
  // from the drawing manager if it was present there, so we need to apply
  // the change.
  emit applyData();

  emit editingMode(true);
}

void DrawingDialog::showActiveContextMenu(const QPoint &pos)
{
  QItemSelectionModel *selectionModel = activeList_->selectionModel();
  QModelIndex index = activeList_->indexAt(pos);
  if (index.isValid() && !selectionModel->isSelected(index))
    selectionModel->select(index, QItemSelectionModel::Select);

  if (!activeList_->selectionModel()->hasSelection())
    return;

  QMenu menu;
  QAction *editAction = menu.addAction(tr("Edit"));
  if (menu.exec(activeList_->viewport()->mapToGlobal(pos)) == editAction)
    editDrawings();
}

void DrawingDialog::hideEditItems(bool hidden)
{
  editm_->setEnabled(!hidden);
  emit updated();
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

QModelIndex DrawingModel::find(const QString &name) const
{
  return createIndex(items_.keys().indexOf(name), 0, -1);
}

void DrawingModel::appendDrawing(const QString &name, const QString &fileName)
{
  // Since the underlying data is sorted, we need to insert the drawing into
  // the map in order to find out where it will be inserted, so we do that to
  // a new map before notifying other components and updating the map.
  QMap<QString, QString> items = items_;
  items[name] = fileName;
  int row = items.keys().indexOf(name);

  beginInsertRows(QModelIndex(), row, row);
  items_ = items;
  endInsertRows();
}

} // namespace
