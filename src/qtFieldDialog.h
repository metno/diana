/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diFieldDialogData.h"
#include "qtDataDialog.h"

#include "diColourShading.h"
#include "diCommandParser.h"
#include "diField/diCommonFieldTypes.h"
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

private:
  struct SelectedField {
    bool inEdit;
    bool external;     // from QuickMenu,...
    std::string modelName;
    std::string fieldName;
    std::string level;
    std::string idnum;
    int  hourOffset;
    int  hourDiff;
    miutil::KeyValue_v fieldOpts;
    std::vector<std::string> levelOptions;
    std::vector<std::string> idnumOptions;
    bool minus;
    std::string time;
    //Used in gridio
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
        , minus(false)
        , predefinedPlot(true)
        , levelmove(true)
        , idnummove(true)
    {
    }
  };

  struct EnableWidget {
    bool contourWidgets;
    bool extremeWidgets;
    bool shadingWidgets;
    bool lineWidgets;
    bool fontWidgets;
    bool densityWidgets;
    bool unitWidgets;
  };

public:
  FieldDialog(QWidget* parent, FieldDialogData* data);

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

public /*Q_SLOTS*/:
  void updateTimes() override;

  void updateDialog() override;

public Q_SLOTS:
  void fieldEditUpdate(std::string str);

protected:
  void doShowMore(bool more) override;

private Q_SLOTS:
  void modelboxClicked(const QModelIndex& index);
  void filterModels(const QString& filtertext);

  void updateFieldGroups();
  void fieldGRboxActivated(int index);
  void fieldboxChanged(QListWidgetItem*);

  void selectedFieldboxClicked(QListWidgetItem* item);

  void upField();
  void downField();
  void minusField(bool on);
  void deleteSelected();
  void deleteAllSelected();
  void copySelectedField();
  void resetOptions();
  void changeModel();
  void unitEditingFinished();
  void plottypeComboBoxActivated(int index);
  void colorCboxActivated(int index);
  void lineWidthCboxActivated(int index);
  void lineTypeCboxActivated(int index);
  void lineintervalCboxActivated(int index);
  void densityCboxActivated(int index);
  void vectorunitCboxActivated(int index);
  void extremeTypeActivated(int index);

  void levelChanged(int number);
  void updateLevel();
  void levelPressed();

  void idnumChanged(int number);
  void updateIdnum();
  void idnumPressed();

  void allTimeStepToggled(bool on);

  void extremeSizeChanged(int value);
  void extremeRadiusChanged(int value);
  void lineSmoothChanged(int value);
  void fieldSmoothChanged(int value);
  void labelSizeChanged(int value);
  void valuePrecisionBoxActivated(int index);
  void gridValueCheckBoxToggled(bool on);
  void gridLinesChanged(int value);
  //  void gridLinesMaxChanged(int value);
  void hourOffsetChanged(int value);
  void hourDiffChanged(int value);
  void undefMaskingActivated(int index);
  void undefColourActivated(int index);
  void undefLinewidthActivated(int index);
  void undefLinetypeActivated(int index);
  void frameCheckBoxToggled(bool on);
  void zeroLineCheckBoxToggled(bool on);
  void valueLabelCheckBoxToggled(bool on);
  void colour2ComboBoxToggled(int index);
  void tableCheckBoxToggled(bool on);
  void repeatCheckBoxToggled(bool on);
  void shadingChanged();
  void threeColoursChanged();
  void patternComboBoxToggled(int index);
  void patternColourBoxToggled(int index);
  void alphaChanged(int index);
  void interval2ComboBoxToggled(int index);
  void zero1ComboBoxToggled(int index);
  void zero2ComboBoxToggled(int index);
  void min1ComboBoxToggled(int index);
  void max1ComboBoxToggled(int index);
  void min2ComboBoxToggled(int index);
  void max2ComboBoxToggled(int index);
  void linevaluesFieldEdited();
  void linevaluesLogCheckBoxToggled(bool);
  void linewidth1ComboBoxToggled(int index);
  void linewidth2ComboBoxToggled(int index);
  void linetype1ComboBoxToggled(int index);
  void linetype2ComboBoxToggled(int index);
  void updatePaletteString();

private:
  bool decodeString(const miutil::KeyValue_v& kvs, SelectedField& sf, bool& allTimeSteps);

  void updateModelBoxes();
  void setDefaultFieldOptions();
  void enableWidgets(const std::string& plottype);
  void enableFieldOptions();
  void enableType2Options(bool);
  void updateFieldOptions(const std::string& key, const std::string& value);
  void updateTime();
  void setLevel();
  void setIdnum();
  void getFieldGroups(const std::string& model, const std::string& refTime, bool plotDefinitions, FieldPlotGroupInfo_v& vfg);
  void checkFieldOptions(miutil::KeyValue_v& fieldopts);
  miutil::KeyValue_v getFieldOptions(const std::string& fieldName, bool reset) const;

  void toolTips();

  std::vector<std::string> numberList( QComboBox* cBox, float number, bool onoff= false );

  void baseList( QComboBox* cBox, float base, bool onoff= false );

  miutil::KeyValue_v getParamString(int i);

  void CreateAdvanced();
  void addModelGroup(int modelgroupIndex);

private:
  std::unique_ptr<FieldDialogData> m_data;
  bool useArchive;

  bool levelInMotion;
  bool idnumInMotion;

  std::string lastFieldGroupName;

  miutil::KeyValue_v vpcopt;

  std::string editName;  // replacing the modelName during editing

  // map<fieldName,fieldOptions>
  typedef std::map<std::string, miutil::KeyValue_v> fieldoptions_m;
  fieldoptions_m setupFieldOptions;
  fieldoptions_m fieldOptions;
  fieldoptions_m editFieldOptions;

  // possible extensions of fieldnames (not found in setup)
  std::set<std::string> fieldPrefixes;
  std::set<std::string> fieldSuffixes;

  std::vector<SelectedField> selectedFields;
  int numEditFields;
  std::vector<SelectedField> selectedField2edit;
  std::vector<bool>          selectedField2edit_exists;

  std::vector<std::string> plottypes;

  std::map<std::string, EnableWidget> enableMap;
  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  std::vector<std::string> lineintervals;
  std::vector<std::string> lineintervals2;
  QStringList      densityStringList;
  std::vector<std::string> vectorunit;
  std::vector<std::string> extremeType;
  miutil::KeyValue_v currentFieldOpts;
  bool     currentFieldOptsInEdit;

  // info about selected model, fields, levels, idnums and plot options
  std::string currentModel;
  FieldPlotGroupInfo_v vfgi;

  FieldModelGroupInfo_v m_modelgroup;

  std::string lastLevel;
  std::string lastIdnum;
  std::vector<std::string> currentLevels;
  std::vector<std::string> currentIdnums;

  QColor* color;

  QTreeView* modelbox;
  QSortFilterProxyModel* modelFilter;
  QStandardItemModel* modelItems;
  QLineEdit* modelFilterEdit;
  QComboBox* refTimeComboBox;
  QComboBox* fieldGRbox;
  QCheckBox* predefinedPlotsCheckBox;
  QListWidget*  fieldbox;
  QListWidget*  selectedFieldbox;

  QSlider* levelSlider;
  QLabel*  levelLabel;
  QSlider* idnumSlider;
  QLabel*  idnumLabel;

  QPushButton*  upFieldButton;
  ToggleButton* minusButton;
  QPushButton*  downFieldButton;
  QPushButton*  deleteButton;
  QPushButton*  deleteAll;
  QPushButton*  copyField;
  QPushButton*  resetOptionsButton;
  QPushButton*  changeModelButton;

  QLineEdit* unitLineEdit;
  QComboBox* plottypeComboBox;
  QComboBox* colorCbox;

  QComboBox* lineWidthCbox;
  QComboBox* lineTypeCbox;

  QComboBox* lineintervalCbox;

  QComboBox* densityCbox;
  const char** cdensities;
  int        nr_densities;

  QComboBox* extremeTypeCbox;

  QComboBox* vectorunitCbox;

  ToggleButton* advanced;
  ToggleButton* allTimeStepButton;

  QWidget*   advFrame;
  QSpinBox*  extremeSizeSpinBox;
  QSpinBox*  extremeRadiusSpinBox;
  QSpinBox*  lineSmoothSpinBox;
  QSpinBox*  fieldSmoothSpinBox;
  QSpinBox*  labelSizeSpinBox;
  QComboBox* valuePrecisionBox;
  QCheckBox* gridValueCheckBox;
  QSpinBox*  gridLinesSpinBox;
  QSpinBox*  hourOffsetSpinBox;
  QSpinBox*  hourDiffSpinBox;
  QComboBox* undefMaskingCbox;
  QComboBox* undefColourCbox;
  QComboBox* undefLinewidthCbox;
  QComboBox* undefLinetypeCbox;
  QCheckBox* frameCheckBox;
  QCheckBox* zeroLineCheckBox;
  QCheckBox* valueLabelCheckBox;
  QCheckBox* tableCheckBox;
  QCheckBox* repeatCheckBox;
  std::vector<QComboBox*> threeColourBox;
  QComboBox* shadingComboBox;
  QComboBox* shadingcoldComboBox;
  QSpinBox*  shadingSpinBox;
  QSpinBox*  shadingcoldSpinBox;
  QComboBox* patternComboBox;
  QComboBox* patternColourBox;
  QSpinBox*  alphaSpinBox;
  QComboBox* colour2ComboBox;
  QComboBox* zero1ComboBox;
  QComboBox* min1ComboBox;
  QComboBox* max1ComboBox;
  QLineEdit* linevaluesField;
  QCheckBox* linevaluesLogCheckBox;
  QComboBox* interval2ComboBox;
  QComboBox* zero2ComboBox;
  QComboBox* min2ComboBox;
  QComboBox* max2ComboBox;
  QComboBox* linewidth1ComboBox;
  QComboBox* linewidth2ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* linetype2ComboBox;

  std::vector<std::string> undefMasking;
};

#endif
