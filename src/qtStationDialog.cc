/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011 met.no

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

#include <diStationManager.h>
#include <diStationPlot.h>
#include <qtStationDialog.h>
#include "qtUtility.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QModelIndex>
#include <QPushButton>
#include <QShowEvent>
#include <QTreeView>
#include <QVariant>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QCheckBox>

#include <sstream>

using namespace std;

/**
 * \class StationDialog
 * \brief Provides a dialog to display and edit StationPlot and Station information.
 *
 * The StationDialog class provides a dialog in which to display data held in a
 * collection of StationPlot objects.
 *
 * Initially, the dialog contains no data. When it is shown, it obtains information
 * from the application-wide StationManager object about the StationPlot objects in use.
 *
 * StationPlot and Station objects are not accessed directly by the dialog. Instead,
 * it operates on data structures that represent these classes. Adding and removing
 * sets of stations and stations themselves causes this data structure to be updated.
 * When the dialog is accepted or its contents are applied, the new information is
 * serialized as a string that the manager will later use to construct new StationPlot
 * and Station objects.
 */

/**
 * Constructs a new station dialog with the given \a parent and controller, \a llctrl.
 *
 * The controller is used by the dialog to obtain new data from the application-wide
 * StationManager object.
 */
StationDialog::StationDialog(QWidget* parent, Controller* llctrl) :
    QDialog(parent), m_ctrl(llctrl)
{
  model = new StationDialog::Model(dialogInfo, this);

  QLabel *stationPlotLabel = TitleLabel(tr("Sets"), this);
  stationPlotList = new QTreeView();
  stationPlotList->setModel(model);
  stationPlotList->setSelectionMode(QAbstractItemView::MultiSelection);
  stationPlotList->setSelectionBehavior(QAbstractItemView::SelectRows);

  chosenModel = new StationDialog::Model(chosenInfo, this);

  QLabel *selectedStationPlotLabel = TitleLabel(tr("Chosen Sets"), this);
  selectedStationPlotList = new QTreeView();
  selectedStationPlotList->setModel(chosenModel);
  selectedStationPlotList->setSelectionMode(QAbstractItemView::MultiSelection);
  selectedStationPlotList->setSelectionBehavior(QAbstractItemView::SelectRows);

  showStationNames= new QCheckBox(tr("Show station names"));
  showStationNames->setToolTip(tr("Show station names on the map") );
  connect( showStationNames, SIGNAL( toggled(bool) ),SLOT( showStationNamesActivated(bool) ) );
  showStationNames->setChecked(show_names);
 
  QPushButton *helpButton = NormalPushButton(tr("Help"), this);

  fieldHide = NormalPushButton(tr("Hide"), this);
  fieldApplyHide = NormalPushButton(tr("Apply+Hide"), this);
  fieldApply = NormalPushButton(tr("Apply"), this);

  reloadButton = NormalPushButton(tr("Reload"), this);
  reloadButton->setEnabled(false);

  connect(helpButton, SIGNAL(clicked()), SLOT(helpClicked()));
  connect(fieldHide, SIGNAL(clicked()), SLOT(hideClicked()));
  connect(fieldApplyHide, SIGNAL(clicked()), SLOT(applyHideClicked()));
  connect(fieldApply, SIGNAL(clicked()), SLOT(applyClicked()));

  connect(stationPlotList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(chooseSet()));

  connect(selectedStationPlotList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(selectSet(const QItemSelection &)));

  connect(reloadButton, SIGNAL(clicked()), SLOT(reloadSets()));

  QGridLayout* buttonLayout = new QGridLayout();
  buttonLayout->addWidget(showStationNames, 0, 1);
  buttonLayout->addWidget(reloadButton, 0, 2);
  buttonLayout->addWidget(helpButton, 1, 0);
  buttonLayout->addWidget(fieldHide, 2, 0);
  buttonLayout->addWidget(fieldApplyHide, 2, 1);
  buttonLayout->addWidget(fieldApply, 2, 2);

  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->addWidget(stationPlotLabel);
  mainLayout->addWidget(stationPlotList);
  mainLayout->addWidget(selectedStationPlotLabel);
  mainLayout->addWidget(selectedStationPlotList);
  mainLayout->addLayout(buttonLayout);

  setLayout(mainLayout);
}

/**
 * Destroys the application-wide station dialog.
 */
StationDialog::~StationDialog()
{
}

/**
 * Updates the contents of the dialog with new data which is obtained from the
 * manager.
 *
 * This is only performed when the dialog is shown.
 */
void StationDialog::updateDialog()
{
  dialogInfo = m_ctrl->initStationDialog();

  // Read the contents of the dialogInfo object and update the list views.
  model->updateData(dialogInfo);

  for (int i = 0; i < model->rowCount(); ++i) {
    QModelIndex index = model->index(i, 0);
    if (index.data(Qt::UserRole).toBool())
      stationPlotList->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    else
      stationPlotList->selectionModel()->select(index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
  }
}

/**
 * Populates the selected set model with the selected items from the set model.
 *
 * This is performed whenever the selection changes in the station set view.
 */
void StationDialog::chooseSet()
{
  stationDialogInfo info;

  foreach (QModelIndex index, stationPlotList->selectionModel()->selection().indexes()) {
    if (index.column() != 0)
      continue;

    stationSetInfo& s_info = model->set(index.row());
    info.sets.push_back(s_info);
  }

  chosenModel->updateData(info);
}
/**
 * Populates the selected set model with the selected items from the set model.
 *
 * This is performed whenever the selection changes in the station set view.
 */
void StationDialog::showStationNamesActivated(bool on)
{
  if (showStationNames->isChecked()) 
      show_names = true;
  else 
      show_names = false;
}

/**
 * Handles the change to a new set of stations.
 */
void StationDialog::selectSet(const QItemSelection& current)
{
  reloadButton->setEnabled(!current.indexes().isEmpty());
  //emit StationApply();
}

/**
 * Reloads the stations for the selected sets in the chosen set list view.
 */
void StationDialog::reloadSets()
{
  QItemSelectionModel* selectionModel = selectedStationPlotList->selectionModel();
  foreach (QModelIndex index, selectionModel->selectedRows(1)) {
    std::string url = index.data().toString().toStdString();
    QModelIndex nameIndex = chosenModel->index(index.row(), 0);
    std::string name = nameIndex.data().toString().toStdString();
    if (show_names)
        m_ctrl->getStationManager()->stationCommand("showPositionName",name,-1);

    if (m_ctrl->getStationManager()->importStations(name, url))
      selectionModel->select(index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
  }
}

/**
 * Returns a vector of strings describing each set of stations and the
 * stations themselves.
 */
vector<string> StationDialog::getOKString()
{
  // Clear the set of chosen sets and add the new chosen sets to it.
  vector<stationSetInfo>::iterator it;
  vector<string> strings;

  for (it = dialogInfo.sets.begin(); it != dialogInfo.sets.end(); ++it) {

    dialogInfo.chosen[it->url] = false;

    vector<stationSetInfo>::iterator itC;
    for (itC = chosenInfo.sets.begin(); itC != chosenInfo.sets.end(); ++itC) {
      if (itC->url == it->url) {

        // Load the list of stations from the URL.
        StationPlot* plot = m_ctrl->getStationManager()->importStations(it->name, it->url);
        if (plot) {
          m_ctrl->putStations(plot);
          dialogInfo.chosen[it->url] = true;
        }

        break;
      }
    }

    ostringstream s;

    // Serialized station plots start with the STATION token.
    s << "STATION " << it->name << " " << it->url;
    if (!dialogInfo.chosen[it->url])
      s << " hidden";
    else
      s << " unselected";

    strings.push_back(s.str());
  }
  return strings;
}

void StationDialog::putOKString(const vector<string>& vstr)
{
  return;
}

std::string StationDialog::getShortname()
{
  return "";
}

void StationDialog::showEvent(QShowEvent *event)
{
  updateDialog();
  event->accept();
}

void StationDialog::applyClicked()
{
  //if (historyOkButton->isEnabled())
  //  historyOk();
  emit StationApply();
}

void StationDialog::applyHideClicked()
{
  //if (historyOkButton->isEnabled())
  //  historyOk();
  emit StationHide();
  emit StationApply();
}

void StationDialog::helpClicked()
{
  emit showsource("ug_stationdialogue.html");
}

void StationDialog::hideClicked()
{
  emit StationHide();
}

void StationDialog::closeEvent(QCloseEvent *event)
{
  emit StationHide();
}


/**
 * \class StationDialog::Model
 * \brief Provides a model to reference StationPlot and Station information held in a
 * data structure.
 *
 * The StationDialog::Model class provides a model that encapsulates the data structures
 * describing a collection of StationPlot objects and the Station objects they contain.
 *
 * The model exposes a single level structure to views. The top level contains two columns
 * and a series of rows, each corresponding to a StationPlot object. The model index for the
 * first column of each row can be dereferenced to obtain the name of the StationPlot; the
 * index for the second column can be used to obtain the URL of the data.
 */

/**
 * Constructs a new model for accessing data held by \a info with the given \a parent.
 */
StationDialog::Model::Model(stationDialogInfo& info, QObject *parent) :
    QAbstractItemModel(parent), m_info(info)
{
}

/**
 * \reimp
 */
QModelIndex StationDialog::Model::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid()) {
    if (row >= 0 && (unsigned int)row < m_info.sets.size() && column >= 0 && column < 2)
      return createIndex(row, column, -1);
  }

  return QModelIndex();
}

/**
 * \reimp
 */
QModelIndex StationDialog::Model::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

/**
 * \reimp
 * Returns the number of rows contained by the item corresponding to the given
 * \a parent index.
 *
 * For top-level items, this function returns the total number of StationPlot
 * objects. For child items, it returns the total number of Station objects held
 * by the StationPlot corresponding to \a parent.
 */
int StationDialog::Model::rowCount(const QModelIndex& parent) const
{
  if (!parent.isValid())
    return m_info.sets.size();

  return 0;
}

/**
 * \reimp
 */
int StationDialog::Model::columnCount(const QModelIndex& parent) const
{
  return 2;
}

/**
 * \reimp
 * Returns the data for the item corresponding to the given \a index, for
 * the specified \a role.
 *
 * For top-level items, this function returns the name of the corresponding
 * StationPlot object. For child items, it returns the relevant property for
 * the corresponding Station object.
 */
QVariant StationDialog::Model::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::UserRole)
    return QVariant();

  if (index.internalId() == -1) {
    // Top level item
    if (index.row() >= 0 && (unsigned int)index.row() < m_info.sets.size()) {
      if (index.column() == 0) {
        if (role == Qt::DisplayRole)
          return QVariant(QString::fromStdString(m_info.sets[index.row()].name));
        else {
          return QVariant(m_info.chosen[m_info.sets[index.row()].url]);
        }
      } else if (index.column() == 1) {
        if (role == Qt::DisplayRole)
          return QVariant(QString::fromStdString(m_info.sets[index.row()].url));
        else
          return QVariant(m_info.chosen[m_info.sets[index.row()].url]);
      }
    }
  }

  return QVariant();
}

QVariant StationDialog::Model::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (section == 0)
    return tr("Name");
  else if (section == 1)
    return tr("Source");

  return QVariant();
}

/**
 * Updates the model with the new data specified by \a info and resets the model
 * to ensure that attached views display the new data.
 */
void StationDialog::Model::updateData(stationDialogInfo& info)
{
  beginResetModel();
  m_info = info;
  endResetModel();
}

/**
 * \reimp
 */
Qt::ItemFlags StationDialog::Model::flags(const QModelIndex& index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * Sets new data for the item corresponding to the given \a index.
 * The data given by \a value is stored according to the specified \a role.
 *
 * For top-level items, the data stored in the first column is the name of
 * the corresponding StationPlot object.
 *
 * For child items, the data stored in each column is the value of the relevant
 * property of the corresponding Station object.
 */
bool StationDialog::Model::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid() || !value.isValid())
    return false;
  else if (role != Qt::EditRole)
    return false;
  else if (index.row() < 0 || (unsigned int)index.row() >= m_info.sets.size())
    return false;

  if (index.internalId() == -1) {
    // Top level item

    if (index.column() == 0)
      m_info.sets[index.row()].name = value.toString().toStdString();
    else if (index.column() == 1)
      m_info.sets[index.row()].url = value.toString().toStdString();
    else
      return false;

    emit dataChanged(index, index);
    return true;
  }

  return false;
}

stationSetInfo& StationDialog::Model::set(int row) const
{
  return m_info.sets[row];
}
