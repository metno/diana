/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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
#ifndef _fielddialog_h
#define _fielddialog_h

#include "diPlotOptions.h"
#include "qtDataDialog.h"

#include "diColourShading.h"
#include "diField/diCommonFieldTypes.h"
#include "diFieldPlotCommand.h"
#include "diPattern.h"
#include "diTimeTypes.h"
#include "util/diKeyValue.h"

#include <vector>
#include <map>
#include <set>

class QPushButton;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QRadioButton;
class QSlider;
class QSortFilterProxyModel;
class QSpinBox;
class QStandardItemModel;
class QTreeView;
class QVBoxLayout;

class ToggleButton;
class StringSliderControl;

class FieldDialogAdd;
class FieldDialogData;
class FieldDialogStyle;

struct SelectedField
{
  bool inEdit;
  bool external; // from QuickMenu,...
  std::string modelName;
  std::string fieldName;
  std::string level;
  std::string idnum;
  int hourOffset;
  int hourDiff;
  std::string units;
  PlotOptions po;
  miutil::KeyValue_v oo; // other options, not part of PlotOptions

  int dimension; // cached value

  std::vector<std::string> levelOptions;
  std::vector<std::string> idnumOptions;
  bool minus;
  std::string time;
  // Used in gridio
  std::string refTime;
  std::string zaxis;
  std::string extraaxis;
  std::string unit;
  bool predefinedPlot;
  bool levelmove;
  bool idnummove;

  SelectedField()
      : inEdit(false)
      , external(false)
      , hourOffset(0)
      , hourDiff(0)
      , dimension(1)
      , minus(false)
      , predefinedPlot(true)
      , levelmove(true)
      , idnummove(true)
  {
  }

  void setFieldPlotOptions(const miutil::KeyValue_v& kv);
  miutil::KeyValue_v getFieldPlotOptions() const;
};

/**
  \brief Dialogue for field plotting

  Select model, field, level and plotting options.
  Can alter many, but not all, options from the setup file.
  The dialog displays all quckmenu commands for easy adjustment.
  Keeps user settings in the diana log file between sessions.
*/
class FieldDialog : public DataDialog
{
  Q_OBJECT

public:
  FieldDialog(QWidget* parent, FieldDialogData* data);
  ~FieldDialog();

  bool showsMore() override;

  std::string name() const override;

  /// returns fiels command strings, one for each field
  PlotCommand_cpv getOKString() override;

  void putOKString(const PlotCommand_cpv& vstr) override;

  /// follows levelUp/levelDown in main window toolbar
  void changeLevel(int increment, int type = 0);

  void archiveMode(bool on);

  /// return a short text for quickmenue
  std::string getShortname();

  bool levelsExists(bool up, int type=0);

  /// make contents for the diana log file
  std::vector<std::string> writeLog();

  /// digest contents from the diana log file (a previous session)
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);

  bool isSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field);
  void addPlot(const std::string& model, const std::string& reftime, bool predefined, const FieldPlotInfo& plot);
  void removePlot(const std::string& model, const std::string& reftime, bool predefined, const FieldPlotInfo& plot);

public /*Q_SLOTS*/:
  void updateTimes() override;
  void updateDialog() override;

  void fieldEditUpdate(const std::string& str);

protected:
  void doShowMore(bool more) override;

private Q_SLOTS:
  void selectedFieldboxClicked(QListWidgetItem* item);

  void upField();
  void downField();
  void minusField(bool on);
  void deleteSelected();
  void deleteAllSelected();
  void copySelectedField();
  void changeModel();

  void levelChanged(const std::string& value);
  void idnumChanged(const std::string& value);

  void allTimeStepToggled(bool on);
  void resetFieldOptionsClicked();

  void updateTime();

private:
  void addSelectedField(const SelectedField& sf);
  void deleteSelectedField(int index);
  std::vector<SelectedField>::iterator findSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field);

  void updateFieldOptions();
  void enableFieldOptions();

  void moveField(int delta);

  void updateModelBoxes();
  void setLevel();
  void setIdnum();
  bool decodeCommand(FieldPlotCommand_cp cmd, const FieldPlotCommand::FieldSpec& fs, SelectedField& sf);
  void maybeUpdateFieldReferenceTimes(std::set<std::string>& updated_models, const std::string& model);

  void toolTips();

  void getParamString(const SelectedField& sf, FieldPlotCommand::FieldSpec& fs);

private:
  std::unique_ptr<FieldDialogData> m_data;

  std::string editName;  // replacing the modelName during editing

  StringSliderControl* levelSliderControl;
  StringSliderControl* idnumSliderControl;

  FieldDialogAdd* fieldAdd;
  FieldDialogStyle* fieldStyle;

  QListWidget* selectedFieldbox;
  QPushButton*  upFieldButton;
  ToggleButton* minusButton;
  QPushButton*  downFieldButton;
  QPushButton*  deleteButton;
  QPushButton*  deleteAll;
  QPushButton*  copyField;
  QPushButton*  changeModelButton;

  std::vector<SelectedField> selectedFields;
  bool hasEditFields;
  int currentSelectedFieldIndex;

  QPushButton* resetOptionsButton;

  ToggleButton* advanced;
  ToggleButton* allTimeStepButton;
};

#endif
