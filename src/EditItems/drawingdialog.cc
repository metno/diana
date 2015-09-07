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
#include "EditItems/timefilesextractor.h"
#include "EditItems/toolbar.h"

#include "qtUtility.h"

#include <drawing.xpm> // ### for now

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTreeView>
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
  connect(drawm_, SIGNAL(drawingLoaded(QString)), &drawingsModel_, SLOT(appendDrawing(QString)));

  // Create the GUI.
  setWindowTitle(tr("Drawing Dialog"));
  setFocusPolicy(Qt::StrongFocus);

  // Products and selected products

  drawingsList_ = new QTreeView();
  drawingsList_->setHeaderHidden(true);
  drawingsList_->setRootIsDecorated(true);
  drawingsList_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  drawingsList_->setModel(&drawingsModel_);
  drawingsList_->setSelectionMode(QAbstractItemView::MultiSelection);
  drawingsList_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(drawingsList_, SIGNAL(customContextMenuRequested(const QPoint &)),
          SLOT(showDrawingContextMenu(const QPoint &)));

  activeList_ = new QTreeView();
  activeList_->setHeaderHidden(true);
  activeList_->setRootIsDecorated(false);
  activeList_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
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

  // Editing/Filtering

  QFrame *editTopSeparator = new QFrame();
  editTopSeparator->setFrameShape(QFrame::HLine);
  QFrame *editBottomSeparator = new QFrame();
  editBottomSeparator->setFrameShape(QFrame::HLine);

  QCheckBox *editModeCheckBox = new QCheckBox(tr("Edit mode"));
  connect(editModeCheckBox, SIGNAL(toggled(bool)), SIGNAL(editingMode(bool)));
  connect(editm_, SIGNAL(editing(bool)), editModeCheckBox, SLOT(setChecked(bool)));

  filterButton_ = new QPushButton(tr("Show filters >>>"));
  filterButton_->setCheckable(true);
  filterButton_->setChecked(false);
  filterWidget_ = new FilterDrawingWidget();
  connect(filterButton_, SIGNAL(toggled(bool)), SLOT(extend(bool)));
  connect(filterWidget_, SIGNAL(updated()), SIGNAL(updated()));

  QCheckBox *showAllCheckBox = new QCheckBox(tr("Show all items"));
  connect(showAllCheckBox, SIGNAL(toggled(bool)), drawm_, SLOT(setAllItemsVisible(bool)));
  connect(showAllCheckBox, SIGNAL(toggled(bool)), editm_, SLOT(setAllItemsVisible(bool)));
  connect(showAllCheckBox, SIGNAL(toggled(bool)), drawm_, SIGNAL(updated()));
  connect(showAllCheckBox, SIGNAL(toggled(bool)), filterWidget_, SLOT(disableFilter(bool)));

  quickSaveButton_ = new QPushButton(tr("Quick save"));
  quickSaveButton_->setEnabled(false);
  connect(editm_, SIGNAL(drawingLoaded(QString)), SLOT(updateQuickSaveButton()));
  connect(editm_, SIGNAL(itemStatesReplaced()), SLOT(updateQuickSaveButton()));
  connect(quickSaveButton_, SIGNAL(clicked()), SLOT(quickSave()));
  connect(editm_, SIGNAL(reloadRequested()), SLOT(reload()));
  connect(editm_, SIGNAL(saveRequested()), SLOT(quickSave()));

  QToolButton *saveAsButton = new QToolButton();
  saveAsButton->setText(tr("Save as"));
  saveAsButton->setPopupMode(QToolButton::InstantPopup);
  QMenu *saveAsMenu = new QMenu(tr("Save as..."), this);
  connect(saveAsMenu->addAction(tr("Save All Items...")), SIGNAL(triggered()), SLOT(saveAllItems()));
  connect(saveAsMenu->addAction(tr("Save Filtered Items...")), SIGNAL(triggered()), SLOT(saveFilteredItems()));
  connect(saveAsMenu->addAction(tr("Save Visible Items...")), SIGNAL(triggered()), SLOT(saveVisibleItems()));
  connect(saveAsMenu->addAction(tr("Save Selected Items...")), SIGNAL(triggered()), SLOT(saveSelectedItems()));
  saveAsButton->setMenu(saveAsMenu);

  editButton_ = new QPushButton(tr("Edit"));
  editButton_->setEnabled(false);
  connect(editButton_, SIGNAL(clicked()), SLOT(editDrawings()));

  QPushButton *clearButton = new QPushButton(tr("Clear"));
  clearButton->setEnabled(true);
  connect(clearButton, SIGNAL(clicked()), SLOT(clearItems()));

  QHBoxLayout *saveButtonLayout = new QHBoxLayout();
  saveButtonLayout->addWidget(quickSaveButton_);
  saveButtonLayout->addWidget(saveAsButton);
  saveButtonLayout->addWidget(editButton_);
  saveButtonLayout->addWidget(clearButton);

  QHBoxLayout *filterButtonLayout = new QHBoxLayout();
  filterButtonLayout->addWidget(showAllCheckBox);
  filterButtonLayout->addWidget(filterButton_);

  QVBoxLayout *editLayout = new QVBoxLayout();
  editLayout->addWidget(editTopSeparator);
  editLayout->addWidget(TitleLabel(tr("Editing/Filtering"), this));
  editLayout->addWidget(editModeCheckBox);
  editLayout->addLayout(filterButtonLayout);
  editLayout->addLayout(saveButtonLayout);
  editLayout->addWidget(editBottomSeparator);

  // Apply and hide

  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);
  QPushButton *applyButton = NormalPushButton(tr("Apply"), this);
  QPushButton *quickLoadButton = new QPushButton(tr("Reload"));

  connect(hideButton, SIGNAL(clicked()), SLOT(close()));
  connect(applyButton, SIGNAL(clicked()), SIGNAL(applyData()));
  connect(applyButton, SIGNAL(clicked()), SLOT(updateQuickSaveButton()));
  connect(quickLoadButton, SIGNAL(clicked()), SLOT(reload()));

  QHBoxLayout *hideApplyLayout = new QHBoxLayout();
  hideApplyLayout->addWidget(hideButton);
  hideApplyLayout->addStretch();
  hideApplyLayout->addWidget(quickLoadButton);
  hideApplyLayout->addStretch();
  hideApplyLayout->addWidget(applyButton);

  QVBoxLayout *leftLayout = new QVBoxLayout();
  leftLayout->addLayout(addLayout);
  leftLayout->addWidget(drawingsList_);
  leftLayout->addWidget(TitleLabel(tr("Selected Products"), this));
  leftLayout->addWidget(activeList_);
  leftLayout->addLayout(editLayout);
  leftLayout->addStretch();
  leftLayout->addLayout(hideApplyLayout);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(leftLayout);

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

  connect(drawm_, SIGNAL(itemsHovered(const QList<DrawingItemBase *> &)),
                  SLOT(showItemInformation(const QList<DrawingItemBase *> &)));

  setOrientation(Qt::Horizontal);
  setExtension(filterWidget_);
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

/**
 * Read the names of deselected and selected drawings, removing from and
 * adding to the list of active drawings as necessary.
 */
void DrawingDialog::activateDrawing(const QItemSelection &selected, const QItemSelection &deselected)
{
  QMap<QString, QString> activeDrawings = activeDrawingsModel_.items();

  // Remove any deselected drawings.
  foreach (const QModelIndex &index, deselected.indexes()) {
    if (!drawingsModel_.hasChildren(index)) {
      QString name = index.data().toString();
      QString fileName = index.data(DrawingModel::FileNameRole).toString();
      activeDrawings.remove(index.data().toString());
    }
  }

  // Add any selected drawings.
  foreach (const QModelIndex &index, selected.indexes()) {
    if (!drawingsModel_.hasChildren(index)) {
      QString name = index.data().toString();
      QString fileName = index.data(DrawingModel::FileNameRole).toString();
      activeDrawings[name] = fileName;
    }
  }

  activeDrawingsModel_.setItems(activeDrawings);
}

void DrawingDialog::makeProduct()
{
  // Compile a list of strings describing the files in use.
  QMap<QString, QString> items = activeDrawingsModel_.items();
  QMap<QString, QString>::const_iterator it;

  std::vector<std::string> inp;
  for (it = items.constBegin(); it != items.constEnd(); ++it)
    inp.push_back("DRAWING name=\"" + it.key().toStdString() + "\"");

  putOKString(inp);

  // Update the available times.
  updateTimes();
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

  // Obtain all editing items and save the resulting collection.
  saveFile(editm_->allItems(), fileName);
}

void DrawingDialog::saveFilteredItems()
{
  QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save Filtered Items"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  // Obtain all drawing and editing items, filter them for visibility, and
  // save the resulting collection.
  QList<DrawingItemBase *> items;

  foreach (DrawingItemBase *item, editm_->allItems()) {
    if (editm_->matchesFilter(item))
      items.append(item);
  }

  saveFile(items, fileName);
}

void DrawingDialog::saveSelectedItems()
{
  QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save Selected Items"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  // Obtain all drawing and editing items and save the resulting collection.
  QList<DrawingItemBase *> items = editm_->selectedItems();

  saveFile(items, fileName);
}

void DrawingDialog::saveVisibleItems()
{
  QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save Visible Items"),
    drawm_->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));

  if (fileName.isEmpty())
    return;

  // Obtain all visible drawing and editing items and save the resulting collection.
  QList<DrawingItemBase *> items;

  foreach (DrawingItemBase *item, editm_->allItems()) {
    if (editm_->isItemVisible(item))
      items.append(item);
  }

  saveFile(items, fileName);
}

void DrawingDialog::saveFile(const QList<DrawingItemBase *> &items, const QString &fileName)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString error = KML::saveItemsToFile(items, fileName);
  QApplication::restoreOverrideCursor();

  if (error.isEmpty())
    updateFileInfo(items, fileName);
  else
    QMessageBox::warning(this, tr("Save File"), tr("Failed to save file '%1'. Error was '%2'.").arg(fileName).arg(error));
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

  // Disable the quick save button because we are no longer working on a named product.
  quickSaveButton_->setEnabled(false);
}

void DrawingDialog::updateButtons()
{
  editButton_->setEnabled(activeList_->selectionModel()->hasSelection());
}

/**
 * Clears the editable drawings.
 */
void DrawingDialog::clearItems()
{
  editm_->selectAllItems();
  editm_->deleteSelectedItems();
  editm_->pushUndoCommands();
}

/**
 * Copy the drawings highlighted in the active list to the edit manager.
 */
void DrawingDialog::editDrawings()
{
  if (!editm_->isEmpty()) {
    QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Clear Existing Objects"),
      tr("You are already editing some objects. Shall I remove them?"),
      QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
      clearItems();
  }

  // Obtain lists of names and file names.
  QStringList names;
  QStringList fileNames;
  foreach (const QModelIndex &index, activeList_->selectionModel()->selectedIndexes()) {
    names.append(index.data().toString());
    fileNames.append(index.data(DrawingModel::FileNameRole).toString());
  }

  // Load the drawings into the edit manager.
  QApplication::setOverrideCursor(Qt::WaitCursor);

  for (int i = 0; i < names.size(); ++i)
    editm_->loadDrawing(names.at(i), fileNames.at(i));

  QApplication::restoreOverrideCursor();

  // Remove the selected active drawings.
  removeActiveDrawings();

  // Deselecting the drawing from the list does not automatically remove it
  // from the drawing manager if it was present there, so we need to apply
  // the change.
  emit applyData();

  emit editingMode(true);
}

void DrawingDialog::removeActiveDrawings()
{
  // Construct a selection that will be used to deselect items in the drawing list.
  QItemSelection selection;

  foreach (const QModelIndex &index, activeList_->selectionModel()->selectedIndexes()) {
    QString fileName = index.data(DrawingModel::FileNameRole).toString();
    QModelIndex dindex = drawingsModel_.findFile(fileName);

    // If no corresponding name is found in the drawings list, remove the
    // item directly.
    if (dindex.isValid())
      selection.select(dindex, dindex);
    else
      activeDrawingsModel_.removeRow(index.row());
  }

  // Deselect the named drawings from the drawing list in order to remove
  // them from the active list.
  drawingsList_->selectionModel()->select(selection, QItemSelectionModel::Toggle);
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
  int products = activeList_->selectionModel()->selectedRows().size();
  QAction *editAction = menu.addAction(
    qApp->translate("DrawingDialog", "Edit %n product(s)", 0, QCoreApplication::CodecForTr, products));
  QAction *removeAction = menu.addAction(
    qApp->translate("DrawingDialog", "Remove %n product(s)", 0, QCoreApplication::CodecForTr, products));
  QAction *result = menu.exec(activeList_->viewport()->mapToGlobal(pos));
  if (result == editAction)
    editDrawings();
  else if (result == removeAction)
    removeActiveDrawings();
}

void DrawingDialog::showDrawingContextMenu(const QPoint &pos)
{
  //QItemSelectionModel *selectionModel = drawingsList_->selectionModel();
  QModelIndex index = drawingsList_->indexAt(pos);

  if (!index.isValid())
    return;

  QString fileName = index.data(DrawingModel::FileNameRole).toString();
  // ### It should be possible to directly mark the file for editing.
}

/**
 * Returns a set of string pairs containing the product and source file of
 * each item in the given list.
 */
QSet<QPair<QString, QString> > DrawingDialog::itemProducts(const QList<DrawingItemBase *> &items)
{
  QSet<QPair<QString, QString> > products;

  // Find all the products being edited.
  foreach (DrawingItemBase *item, items) {
    QString srcFile = item->property("srcFile").toString();
    if (srcFile.isEmpty())
      continue;
    QString product = item->property("product").toString();
    if (product == srcFile)
      product = QFileInfo(srcFile).fileName();
    products.insert(QPair<QString, QString>(product, srcFile));
  }

  return products;
}

void DrawingDialog::updateQuickSaveButton()
{
  QSet<QPair<QString, QString> > products = itemProducts(editm_->allItems());

  if (products.size() == 1 && !products.values().first().second.isEmpty()) {
    QString visibleName = products.values().first().first;
    quickSaveName_ = products.values().first().second;
    quickSaveButton_->setText(tr("Quick save '%1'").arg(visibleName));
    quickSaveButton_->setEnabled(true);
  } else {
    quickSaveName_ = QString();
    quickSaveButton_->setText(tr("Quick save"));
    quickSaveButton_->setEnabled(false);
  }
}

/**
 * Reload items from all drawings.
 */
void DrawingDialog::reload()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Update the list of drawings.

  // Record the active drawings and clear the active list by toggling the
  // selected drawings.
  QSet<QString> fileNames = activeDrawingsModel_.items().values().toSet();
  drawingsList_->selectionModel()->select(drawingsList_->selectionModel()->selection(),
                                          QItemSelectionModel::Toggle);

  QMap<QString, QString> items = drawingsModel_.items();
  drawingsModel_.setItems(items);

  // Filter out any drawings that are no longer available.
  QItemSelection selection;
  foreach (const QString &fileName, fileNames) {
    QModelIndex index = drawingsModel_.findFile(fileName);
    if (index.isValid())
      selection.select(index, index);
  }

  drawingsList_->selectionModel()->select(selection, QItemSelectionModel::Toggle);

  // Reload all editable items that have files associated with them.

  QSet<QPair<QString, QString> > products = itemProducts(editm_->allItems());
  fileNames.clear();

  QSet<QPair<QString, QString> >::const_iterator it;
  for (it = products.begin(); it != products.end(); ++it)
    fileNames.insert(it->second);

  // Find all the items belonging to the products and remove them.
  foreach (DrawingItemBase *item, editm_->allItems()) {
    if (fileNames.contains(item->property("srcFile").toString()))
      editm_->removeItem(item);
  }

  for (it = products.begin(); it != products.end(); ++it)
    QString error = editm_->loadDrawing(it->first, it->second);

  // Reload all non-editable drawings by effectively clicking the apply button.
  emit applyData();

  QApplication::restoreOverrideCursor();
}

/**
 * Saves all items to the last edited file.
 */
void DrawingDialog::quickSave()
{
  QList<DrawingItemBase *> items = editm_->allItems();
  saveFile(items, quickSaveName_);
}

void DrawingDialog::extend(bool enable)
{
  showExtension(enable);
  filterButton_->setText(enable ? tr("Hide filters <<<") : tr("Show filters >>>"));
}

void DrawingDialog::showItemInformation(const QList<DrawingItemBase *> &items)
{
  // Don't show property tooltips when in editing mode.
  if (editm_->isEditing())
    return;

  // Create a tooltip containing the values of any filtered properties for
  // the first item in the list that contains them.

  QSet<QString> allowed = filterWidget_->properties().toSet();

  foreach (DrawingItemBase *item, items) {
    if (!item->isVisible())
      continue;

    QStringList lines;

    QVariantMap properties = item->propertiesRef();
    foreach (const QString &key, properties.keys()) {
      if (allowed.contains(key))
        lines.append(QString("%1: %2").arg(key).arg(properties.value(key).toString()));
    }

    if (!lines.isEmpty()) {
      QString text = lines.join("\n");
      QToolTip::showText(QCursor::pos(), text);
      return;
    }
  }
}

// ====================================================================
// Model for holding information about names and file names of drawings
// ====================================================================

DrawingModel::DrawingModel(QObject *parent)
  : QAbstractItemModel(parent)
{
}

DrawingModel::~DrawingModel()
{
}

QModelIndex DrawingModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid()) {
    // Top level item - use an invalid row value as an identifier.
    if (row < order_.size())
      return createIndex(row, column, 0xffffffff);
  } else {
    // Child item - store the parent's row in the created index.
    if (parent.row() >= 0 && parent.row() < order_.size())
      return createIndex(row, column, parent.row());
  }

  return QModelIndex();
}

QModelIndex DrawingModel::parent(const QModelIndex &index) const
{
  // The root item and top level items return an invalid index.
  if (!index.isValid() || index.internalId() == 0xffffffff)
    return QModelIndex();

  // Child items return an index that refers to the relevant top level item.
  return createIndex(index.internalId(), 0, 0xffffffff);
}

bool DrawingModel::hasChildren(const QModelIndex &index) const
{
  // The root item has children.
  if (!index.isValid())
    return true;

  // Child items have no children.
  if (index.internalId() != 0xffffffff)
    return false;

  // Top level items have children if there is more than one file associated
  // with them.
  if (index.row() >= 0 && index.row() < items_.size()) {
    QString name = order_.at(index.row());
    QString fileName = items_.value(name);
    return fileName.contains("[");
  } else
    return false;
}

int DrawingModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

int DrawingModel::rowCount(const QModelIndex &parent) const
{
  if (!parent.isValid())
    return order_.size();

  else if (parent.row() >= 0 && parent.row() < order_.size() && parent.column() == 0) {
    if (hasChildren(parent)) {
      // If the top level item has children then use its file name to obtain
      // a list of matching files.
      QString name = order_.at(parent.row());
      QString fileName = items_.value(name);
      return listFiles(fileName).size();
    } else
      return 1;
  } else
    return 0;
}

QVariant DrawingModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.column() != 0)
    return QVariant();

  if (index.internalId() == 0xffffffff) {
    // Top level item
    QString name = order_.at(index.row());
    if (index.row() < order_.size()) {
      switch (role) {
      case NameRole:
        return QVariant(name);
      case FileNameRole:
        return QVariant(items_.value(name));
      default:
        ;
      }
    }
  } else if (index.internalId() < order_.size()) {
    // Child item
    QString name = order_.at(index.internalId());
    QString fileName = items_.value(name);
    if (role == NameRole || role == FileNameRole) {
      QStringList files = listFiles(fileName);
      if (index.row() < files.size())
        return QVariant(files.at(index.row()));
    }
  }

  return QVariant();
}

QStringList DrawingModel::listFiles(const QString &fileName) const
{
  if (fileCache_.contains(fileName))
    return fileCache_.value(fileName);

  QList<QPair<QFileInfo, QDateTime> > tfiles = TimeFilesExtractor::getFiles(fileName);
  QStringList files;
  for (int i = 0; i < tfiles.size(); ++i)
    files.append(tfiles.at(i).first.filePath());

  // We would like to be able to cache lists of files for consistency but
  // the API insists on everything being const, so we step around that to
  // get what we want.
  const_cast<DrawingModel *>(this)->fileCache_[fileName] = files;
  return files;
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
  if (index.isValid() && hasChildren(index))
    return Qt::ItemIsEnabled;
  else
    return QAbstractItemModel::flags(index);
}

bool DrawingModel::removeRows(int row, int count, const QModelIndex &parent)
{
  if (row < 0 || row >= order_.size())
    return false;

  beginRemoveRows(parent, row, count);
  while (count > 0) {
    // Find the key that corresponds to the given row.
    QString key = order_.at(row);
    items_.remove(key);
    order_.removeAt(row);
    count--;
  }
  endRemoveRows();
  return true;
}

QMap<QString, QString> DrawingModel::items() const
{
  return items_;
}

void DrawingModel::setItems(const QMap<QString, QString> &items)
{
  beginResetModel();
  items_ = items;
  order_ = items.keys();
  fileCache_.clear();
  endResetModel();
}

QModelIndex DrawingModel::find(const QString &name) const
{
  return createIndex(order_.indexOf(name), 0, 0xffffffff);
}

QModelIndex DrawingModel::findFile(const QString &fileName) const
{
  // Check the items in the model to ensure that we don't add an existing product.
  for (int row = 0; row < order_.size(); ++row) {
    QString name = order_.at(row);
    QString topLevelFileName = items_.value(name);
    if (topLevelFileName == fileName)
      return createIndex(row, 0, 0xffffffff);

    // Check children if items have them.
    if (topLevelFileName.contains("[")) {
      QStringList files = listFiles(topLevelFileName);
      for (int child = 0; child < files.size(); ++child) {
        if (files.value(child) == fileName)
          return createIndex(child, 0, row);
      }
    }
  }

  // We didn't find the file, so return an invalid model index.
  return QModelIndex();
}

void DrawingModel::appendDrawing(const QString &fileName)
{
  if (!findFile(fileName).isValid())
    appendDrawing(fileName, fileName);
}

void DrawingModel::appendDrawing(const QString &name, const QString &fileName)
{
  // If the drawing is already in the model then return immediately.
  if (items_.contains(name))
    return;

  int row = order_.size();

  beginInsertRows(QModelIndex(), row, row);
  items_[name] = fileName;
  order_.append(name);
  endInsertRows();
}

} // namespace
