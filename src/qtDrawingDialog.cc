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
#define MILOGGER_CATEGORY "diana.DrawingDialog"
#include <miLogger/miLogging.h>

#include "diController.h"
#include "diDrawingManager.h"
#include "diEditItemManager.h"
#include "qtDrawingDialog.h"
#include "qtUtility.h"
#include "EditItems/edititembase.h"
#include <paint_mode.xpm>       // reused for area drawing functionality

#include <QAction>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

static QString shortClassName(const QString &className)
{
  const int separatorPos = className.lastIndexOf("::");
  return separatorPos == -1
      ? className
      : className.mid(separatorPos + 2);
}

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  // Create an action that can be used to open the dialog from within a menu or toolbar.
  m_action = new QAction(QIcon(QPixmap(paint_mode_xpm)), tr("Painting tools"), this);
  m_action->setShortcutContext(Qt::ApplicationShortcut);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  connect(m_action, SIGNAL(toggled(bool)), SLOT(toggleDrawingMode(bool)));

  // Record the editor in use.
  editm = EditItemManager::instance();

  // When the apply, hide or apply & hide buttons are pressed, we reset the undo stack.
  connect(this, SIGNAL(applyData()), editm, SLOT(reset()));
  connect(this, SIGNAL(hideData()), editm, SLOT(reset()));

  connect(this, SIGNAL(applyData()), SLOT(makeProduct()));

  QLabel *drawingListLabel = TitleLabel(tr("Available Products"), this);
  drawingList = new QListView();
  drawingList->setModel(&drawingModel);
  drawingList->setSelectionMode(QAbstractItemView::MultiSelection);
  drawingList->setSelectionBehavior(QAbstractItemView::SelectRows);

  QLabel *chosenDrawingListLabel = TitleLabel(tr("Chosen Products"), this);
  chosenDrawingList = new QListView();
  chosenDrawingList->setModel(&chosenDrawingModel);
  chosenDrawingList->setSelectionMode(QAbstractItemView::MultiSelection);
  chosenDrawingList->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(drawingList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(chooseDrawing()));

  connect(chosenDrawingList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(selectDrawing(const QItemSelection &)));

  connect(DrawingManager::instance(), SIGNAL(timesUpdated()), SLOT(updateTimes()));

  editButton = new QToolButton();
  editButton->setText(tr("Edit"));
  editButton->setCheckable(true);
  ToolPanel *tools = new ToolPanel();
  tools->setEnabled(false);

  connect(editButton, SIGNAL(toggled(bool)), tools, SLOT(setEnabled(bool)));
  connect(editButton, SIGNAL(toggled(bool)), SLOT(toggleEditingMode(bool)));
  connect(editButton, SIGNAL(toggled(bool)), drawingList, SLOT(setDisabled(bool)));
  connect(editButton, SIGNAL(toggled(bool)), chosenDrawingList, SLOT(setDisabled(bool)));

  QHBoxLayout *editLayout = new QHBoxLayout();
  editLayout->setMargin(0);
  editLayout->addWidget(editButton);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(4);
  layout->addWidget(drawingListLabel);
  layout->addWidget(drawingList);
  layout->addWidget(chosenDrawingListLabel);
  layout->addWidget(chosenDrawingList);
  layout->addLayout(editLayout);
  //layout->addWidget(editor->getUndoView());
  layout->addWidget(tools);
  layout->addLayout(createStandardButtons());

  // Populate the drawing model with data from the drawing manager.
  updateModel();

  setWindowTitle(tr("Drawing"));
}

DrawingDialog::~DrawingDialog()
{
}

std::string DrawingDialog::name() const
{
  return "DRAWING";
}

void DrawingDialog::updateTimes()
{
  std::vector<miutil::miTime> times;
  if (editm->isEnabled())
    times = DrawingManager::instance()->getTimes();
  emit emitTimes("DRAWING", times);
}

void DrawingDialog::updateDialog()
{
}

std::vector<std::string> DrawingDialog::getOKString()
{
  std::vector<std::string> lines;

  if (!editm->isEnabled())
    return lines;

  foreach (QString filePath, editm->getLoaded()) {
    QString line = "DRAWING file=\"" + filePath + "\"";
    lines.push_back(line.toStdString());
  }

  return lines;
}

void DrawingDialog::putOKString(const std::vector<std::string>& vstr)
{
  // If the current drawing has not been produced, ignore any plot commands
  // that are sent to the dialog. This blocks any strings that are sent
  // while the drawing dialog is open.
  if (!editm->isEnabled())
    return;

  // Submit the lines as new input.
  std::vector<std::string> inp;
  inp.insert(inp.begin(), vstr.begin(), vstr.end());
  DrawingManager::instance()->processInput(inp);
}

void DrawingDialog::toggleDrawingMode(bool enable)
{
  // Enabling drawing mode (opening the dialog) causes the manager to enter
  // working mode. This makes it possible to show objects that have not been
  // serialised as plot commands.
  editm->setWorking(enable);
}

void DrawingDialog::toggleEditingMode(bool enable)
{
  // When editing starts, remove any existing items and load the chosen
  // files. Mark the product as unfinished by disabling it.
  if (enable) {
    loadChosenFiles();
    editm->setEnabled(false);
  }

  editm->setEditing(enable);
}

/**
 * Populates the selected drawing model with the selected items from the drawing model.
 *
 * This is performed whenever the selection changes in the drawing list.
 */
void DrawingDialog::chooseDrawing()
{
  // Clear the list of chosen drawings and reconstruct it from the selection in the
  // list of available drawings. Remove all objects from the edit manager and load
  // those in the list of chosen drawings.
  chosenDrawingModel.clear();

  foreach (QModelIndex index, drawingList->selectionModel()->selection().indexes()) {
    if (index.column() != 0)
      continue;

    QString filePath = drawingList->model()->data(index, Qt::UserRole).toString();
    QString fileName = QFileInfo(filePath).fileName();
    QStandardItem *item = new QStandardItem(fileName);
    item->setData(filePath, Qt::UserRole);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    // Append the file name to the list of chosen files. If the file is not
    // loaded successfully then disable it in the chosen list.
    chosenDrawingModel.appendRow(item);
  }
}

/**
 * Handles the change to the selection in the chosen list of drawings.
 */
void DrawingDialog::selectDrawing(const QItemSelection& current)
{
}

/**
 * Updates the drawing model with data from the drawing manager.
 */
void DrawingDialog::updateModel()
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
 * Makes the current drawing a product that is visible outside editing mode.
 */
void DrawingDialog::makeProduct()
{
  // Leave editing mode, if necessary.
  editButton->setChecked(false);

  // Load any chosen files in case the user has not already done so by
  // editing them.
  loadChosenFiles();

  // Enable the product and update the available times.
  editm->setEnabled(true);
  updateTimes();
}

/**
 * Loads the files in the list of chosen models.
 */
void DrawingDialog::loadChosenFiles()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QSet<QString> wasLoaded = loaded.toSet();
  loaded.clear();

  for (int row = 0; row < chosenDrawingModel.rowCount(); ++row) {

    QModelIndex index = chosenDrawingModel.index(row, 0);
    QString filePath = index.data(Qt::UserRole).toString();

    if (wasLoaded.contains(filePath)) {
      loaded.append(filePath);
    } else {
      if (editm->loadItems(filePath))
        loaded.append(filePath);
      else {
        // Disable the item to indicate that it is not loaded.
        QStandardItem *item = chosenDrawingModel.itemFromIndex(index);
        item->setEnabled(false);
      }
    }
  }

  QApplication::restoreOverrideCursor();
}

void DrawingDialog::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    // Escape resets the drawing mode to selection mode. Note that this prevents
    // the dialog from being cancelled.
    editm->setSelectMode();
    event->accept();
  } else {
    DataDialog::keyPressEvent(event);
  }
}

ToolPanel::ToolPanel(QWidget *parent)
  : QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  layout->setMargin(0);

  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  QToolButton *selectButton = new QToolButton();
  selectButton->setDefaultAction(actions[EditItemManager::Select]);
  layout->addWidget(selectButton);

  QToolButton *polyLineButton = new QToolButton();
  polyLineButton->setDefaultAction(actions[EditItemManager::CreatePolyLine]);
  layout->addWidget(polyLineButton);

  QToolButton *symbolButton = new QToolButton();
  symbolButton->setDefaultAction(actions[EditItemManager::CreateSymbol]);
  layout->addWidget(symbolButton);
}

ToolPanel::~ToolPanel()
{
}
