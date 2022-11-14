/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011-2019 met.no

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

#include "qtStationDialog.h"

#include "diController.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "diStationPlotCommand.h"
#include "qtStationDialogModel.h"
#include "qtUtility.h"

#include <QAction>
#include <QCheckBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QInputDialog>
#include <QItemDelegate>
#include <QItemSelection>
#include <QPushButton>
#include <QShowEvent>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>

#include "station.xpm"

#define MILOGGER_CATEGORY "diana.StationDialog"
#include <miLogger/miLogging.h>


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
StationDialog::StationDialog(QWidget* parent, Controller* llctrl)
    : DataDialog(parent, llctrl)
    , show_names(false)
{
  setWindowTitle(tr("Stations"));
  m_action = new QAction(QIcon(QPixmap(station_xpm)), windowTitle(), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_A);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_stationdialogue.html";

  model = new StationDialogModel(dialogInfo, this);

  QLabel *stationPlotLabel = TitleLabel(tr("Sets"), this);
  stationPlotList = new QTreeView();
  stationPlotList->setModel(model);
  stationPlotList->setSelectionMode(QAbstractItemView::MultiSelection);
  stationPlotList->setSelectionBehavior(QAbstractItemView::SelectRows);

  chosenModel = new StationDialogModel(chosenInfo, this);

  QLabel *selectedStationPlotLabel = TitleLabel(tr("Chosen Sets"), this);
  selectedStationPlotList = new QTreeView();
  selectedStationPlotList->setModel(chosenModel);
  selectedStationPlotList->setSelectionMode(QAbstractItemView::MultiSelection);
  selectedStationPlotList->setSelectionBehavior(QAbstractItemView::SelectRows);

  showStationNames= new QCheckBox(tr("Show station names"));
  showStationNames->setToolTip(tr("Show station names on the map") );
  connect(showStationNames, SIGNAL(toggled(bool)), SLOT(showStationNamesActivated(bool)));
  showStationNames->setChecked(show_names);

  connect(stationPlotList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(chooseSet()));

  connect(selectedStationPlotList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(selectSet(const QItemSelection &)));

  QGridLayout* buttonLayout = new QGridLayout();
  buttonLayout->addWidget(showStationNames, 0, 1);

  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->addWidget(stationPlotLabel);
  mainLayout->addWidget(stationPlotList);
  mainLayout->addWidget(selectedStationPlotLabel);
  mainLayout->addWidget(selectedStationPlotList);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addLayout(createStandardButtons(true));

  setLayout(mainLayout);
}

StationDialog::~StationDialog()
{
}

std::string StationDialog::name() const
{
  static const std::string STATION_DATATYPE = "station";
  return STATION_DATATYPE;
}

void StationDialog::updateDialog()
{
  dialogInfo = m_ctrl->getStationManager()->initDialog();

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

void StationDialog::chooseSet()
{
  stationDialogInfo info;

  for (QModelIndex index : stationPlotList->selectionModel()->selection().indexes()) {
    if (index.column() != 0)
      continue;

    stationSetInfo& s_info = model->set(index.row());
    info.sets.push_back(s_info);
  }

  chosenModel->updateData(info);
}

void StationDialog::showStationNamesActivated(bool)
{
  show_names = showStationNames->isChecked();
}

void StationDialog::selectSet(const QItemSelection& current)
{
  setRefreshEnabled(!current.indexes().isEmpty());
}

void StationDialog::updateTimes()
{
  reloadSets();
}

void StationDialog::reloadSets()
{
  QItemSelectionModel* selectionModel = selectedStationPlotList->selectionModel();
  for (QModelIndex index : selectionModel->selectedRows(1)) {
    std::string url = index.data().toString().toStdString();
    QModelIndex nameIndex = chosenModel->index(index.row(), 0);
    std::string name = nameIndex.data().toString().toStdString();
    if (show_names)
      m_ctrl->stationCommand("showPositionName", name, -1);

#if 0 // FIXME leaks StationPlot*
    if (m_ctrl->getStationManager()->importStations(name, url))
      selectionModel->select(index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
#endif
  }
}

PlotCommand_cpv StationDialog::getOKString()
{
  // Clear the set of chosen sets and add the new chosen sets to it.
  PlotCommand_cpv strings;

  for (const stationSetInfo& ssi : dialogInfo.sets) {

    dialogInfo.chosen[ssi.url] = false;

    for (const stationSetInfo& cis : chosenInfo.sets) {
      if (cis.url == ssi.url) {

        // Load the list of stations from the URL.
        StationPlot* plot = m_ctrl->getStationManager()->importStations(ssi.name, ssi.url);
        if (plot) {
          m_ctrl->putStations(plot);
          dialogInfo.chosen[ssi.url] = true;
        }

        break;
      }
    }

    StationPlotCommand_p cmd = std::make_shared<StationPlotCommand>();
    cmd->name = ssi.name;
    cmd->url = ssi.url;
    cmd->select = (!dialogInfo.chosen[ssi.url]) ? "hidden" : "unselected";
    strings.push_back(cmd);
  }
  return strings;
}

void StationDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();
  if (!vstr.empty())
    METLIBS_LOG_ERROR("not implemented, sorry");
}

std::string StationDialog::getShortname()
{
  return "";
}

void StationDialog::showEvent(QShowEvent *event)
{
  updateDialog();
  DataDialog::showEvent(event);
}
