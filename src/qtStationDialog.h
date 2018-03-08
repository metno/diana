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

#include "diCommonTypes.h"
#include "diController.h"
#include "diImageGallery.h"

#include <QDialog>

class QItemSelection;
class QPushButton;
class QShowEvent;
class QTreeView;
class QCheckBox;

class StationDialogModel;

class StationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StationDialog(QWidget* parent, Controller* llctrl);
  ~StationDialog();

  void updateDialog();

  //! @return command strings for current dialog state
  PlotCommand_cpv getOKString();

  //! insert command strings
  void putOKString(const PlotCommand_cpv& vstr);

  //! @return short name of current command
  std::string getShortname();

  bool show_names;

Q_SIGNALS:
  void StationApply();
  void StationHide();
  void showsource(const std::string, const std::string="");

protected:
  void showEvent(QShowEvent *event);
  void closeEvent(QCloseEvent *event);

private slots:
  void chooseSet();
  void selectSet(const QItemSelection& current);
  void reloadSets();

  void showStationNamesActivated(bool);
  void applyClicked();
  void applyHideClicked();
  void helpClicked();
  void hideClicked();

private:
  Controller* m_ctrl;

  stationDialogInfo dialogInfo;
  stationDialogInfo chosenInfo;

  StationDialogModel* model;
  StationDialogModel* chosenModel;

  QTreeView* selectedStationPlotList;
  QTreeView* stationPlotList;
  QCheckBox* showStationNames;
  QPushButton* fieldApply;
  QPushButton* fieldApplyHide;
  QPushButton* fieldHide;
  QPushButton* reloadButton;
};

#endif
