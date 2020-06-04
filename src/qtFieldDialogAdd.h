/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#ifndef _fielddialogadd_h
#define _fielddialogadd_h

#include <QWidget>

#include "diFieldDialogData.h"

class FieldDialog;

class QLineEdit;
class QCheckBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QTreeView;
class QSortFilterProxyModel;
class QItemSelection;
class QStandardItemModel;

class FieldDialogAdd : public QWidget
{
  Q_OBJECT

public:
  FieldDialogAdd(FieldDialogData* data, FieldDialog* dialog);
  ~FieldDialogAdd();

  void updateModelBoxes();
  void archiveMode(bool on);

  bool currentModelReftime(std::string& model, std::string& reftime, bool& predefined);

  void addedSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field);
  void removingSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field);

private Q_SLOTS:
  void modelboxClicked(const QModelIndex& index);
  void filterModels(const QString& filtertext);

  void updateFieldGroups();
  void fieldGRboxActivated(int index);
  void fieldboxChanged(QListWidgetItem*);

private:
  void setupUi();
  void addModelGroup(int modelgroupIndex);
  void getFieldGroups(const std::string& model, const std::string& refTime, bool plotDefinitions, FieldPlotGroupInfo_v& vfg);

  std::string currentRefTime() const;
  bool currentPredefinedPlots() const;
  void selectField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field, bool select);

private:
  FieldDialog* dialog;
  FieldDialogData* m_data;
  bool useArchive;
  std::string currentModel;
  FieldModelGroupInfo_v m_modelgroup;
  std::string lastFieldGroupName;
  FieldPlotGroupInfo_v vfgi;

  QTreeView* modelbox;
  QSortFilterProxyModel* modelFilter;
  QStandardItemModel* modelItems;
  QLineEdit* modelFilterEdit;
  QComboBox* refTimeComboBox;
  QComboBox* fieldGRbox;
  QCheckBox* predefinedPlotsCheckBox;
  QListWidget* fieldbox;
};

#endif // _fielddialogadd_h
