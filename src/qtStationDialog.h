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

#ifndef qtStationDialog_h
#define qtStationDialog_h

#include <QAbstractItemModel>
#include <QDialog>
#include <QItemDelegate>
#include <QItemSelection>
#include <QSortFilterProxyModel>

#include <diCommonTypes.h>
#include <diController.h>
#include <diImageGallery.h>

class QPushButton;
class QShowEvent;
class QTreeView;
class QCheckBox;

class StationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StationDialog(QWidget* parent, Controller* llctrl);
  ~StationDialog();

  void updateDialog();
  ///return command strings
  std::vector<std::string> getOKString();
  ///insert command strings
  void putOKString(const std::vector<std::string>& vstr);
  ///return short name of current command
  std::string getShortname();
  bool show_names;
  class Model : public QAbstractItemModel
  {
  public:
    Model(stationDialogInfo& info, QObject* parent = 0);
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    stationSetInfo& set(int row) const;
    void updateData(stationDialogInfo& info);

  private:
    stationDialogInfo& m_info;
  };

signals:
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

  StationDialog::Model* model;
  StationDialog::Model* chosenModel;

  QTreeView* selectedStationPlotList;
  QTreeView* stationPlotList;
  QCheckBox* showStationNames;
  QPushButton* fieldApply;
  QPushButton* fieldApplyHide;
  QPushButton* fieldHide;
  QPushButton* reloadButton;
};

#endif
