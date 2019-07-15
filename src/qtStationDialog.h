/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2011-2018 met.no

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

#ifndef qtStationDialog_h
#define qtStationDialog_h

#include "qtDataDialog.h"

#include "diStationTypes.h"

#include <QDialog>

class QItemSelection;
class QPushButton;
class QShowEvent;
class QTreeView;
class QCheckBox;

class StationDialogModel;

class StationDialog : public DataDialog
{
  Q_OBJECT

public:
  explicit StationDialog(QWidget* parent, Controller* llctrl);
  ~StationDialog();

  std::string name() const override;

  //! @return command strings for current dialog state
  PlotCommand_cpv getOKString() override;

  //! insert command strings
  void putOKString(const PlotCommand_cpv& vstr) override;

  //! @return short name of current command
  std::string getShortname();

public Q_SLOTS:
  //! Updates the contents of the dialog with new data which is obtained from the manager.
  /*! This is only performed when the dialog is shown.
   */
  void updateDialog() override;

  void updateTimes() override;

protected:
  void showEvent(QShowEvent* event) override;

private Q_SLOTS:
  //! Populates the selected set model with the selected items from the set model.
  /*! This is performed whenever the selection changes in the station set view.
   */
  void chooseSet();

  //! Handles the change to a new set of stations.
  void selectSet(const QItemSelection& current);

  //! Reloads the stations for the selected sets in the chosen set list view.
  void reloadSets();

  //! Populates the selected set model with the selected items from the set model.
  /*! This is performed whenever the selection changes in the station set view.
   */
  void showStationNamesActivated(bool);

private:
  bool show_names;

  stationDialogInfo dialogInfo;
  stationDialogInfo chosenInfo;

  StationDialogModel* model;
  StationDialogModel* chosenModel;

  QTreeView* selectedStationPlotList;
  QTreeView* stationPlotList;
  QCheckBox* showStationNames;
};

#endif
