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

#ifndef _fielddialogstyle_h
#define _fielddialogstyle_h

#include <QWidget>

#include "diColourShading.h"
#include "diPattern.h"
#include "util/diKeyValue.h"

#include <map>
#include <vector>

class QPushButton;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;

class SelectedField;

/**
  \brief Widgets for adjusting field plotting options.
*/
class FieldDialogStyle : public QObject
{
  Q_OBJECT

public:
  // map<fieldName,fieldOptions>
  typedef std::map<std::string, miutil::KeyValue_v> fieldoptions_m;

private:
  struct EnableWidget
  {
    bool contourWidgets;
    bool extremeWidgets;
    bool shadingWidgets;
    bool lineWidgets;
    bool fontWidgets;
    bool densityWidgets;
    bool unitWidgets;
  };

public:
  FieldDialogStyle(const fieldoptions_m& setupFieldOptions, QWidget* parent);
  virtual ~FieldDialogStyle();

  QWidget* standardWidget() const { return widgetStd; }
  QWidget* advancedWidget() const { return widgetAdv; }

  void enableFieldOptions(SelectedField* sf);
  const miutil::KeyValue_v& getFieldOptions(const std::string& fieldName, bool reset) const;

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);

public Q_SLOTS:
  void resetOptions();

Q_SIGNALS:
  void updateTime();

private Q_SLOTS:
  void unitEditingFinished();
  void plottypeComboBoxActivated(int index);
  void colorCboxActivated(int index);
  void lineWidthCboxActivated(int index);
  void lineTypeCboxActivated(int index);
  void lineintervalCboxActivated(int index);
  void densityCboxActivated(int index);
  void vectorunitCboxActivated(int index);
  void extremeTypeActivated(int index);

  void extremeSizeChanged(int value);
  void extremeRadiusChanged(int value);
  void lineSmoothChanged(int value);
  void fieldSmoothChanged(int value);
  void labelSizeChanged(int value);
  void valuePrecisionBoxActivated(int index);
  void gridValueCheckBoxToggled(bool on);
  void gridLinesChanged(int value);
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
  void CreateStandard();
  void CreateAdvanced();
  void toolTips();

  void setDefaultFieldOptions();
  void enableWidgets(const std::string& plottype);
  void enableType2Options(bool);
  void updateFieldOptions(const std::string& key, const std::string& value);

  void baseList(QComboBox* cBox, float base, bool onoff = false);

private:
  QWidget* widgetStd;
  QWidget* widgetAdv;

  SelectedField* selectedField;

  miutil::KeyValue_v vpcopt;

  const fieldoptions_m setupFieldOptions;
  fieldoptions_m fieldOptions;

  std::vector<std::string> plottypes;

  std::map<std::string, EnableWidget> enableMap;
  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  std::vector<std::string> lineintervals;
  std::vector<std::string> lineintervals2;
  QStringList densityStringList;
  std::vector<std::string> vectorunit;
  std::vector<std::string> extremeType;
  miutil::KeyValue_v currentFieldOpts;
  bool currentFieldOptsInEdit;

  std::vector<SelectedField> selectedField2edit;
  std::vector<bool> selectedField2edit_exists;

  QLineEdit* unitLineEdit;
  QComboBox* plottypeComboBox;
  QComboBox* colorCbox;

  QComboBox* lineWidthCbox;
  QComboBox* lineTypeCbox;

  QComboBox* lineintervalCbox;

  QComboBox* densityCbox;
  const char** cdensities;
  int nr_densities;

  QComboBox* extremeTypeCbox;

  QComboBox* vectorunitCbox;

  QSpinBox* extremeSizeSpinBox;
  QSpinBox* extremeRadiusSpinBox;
  QSpinBox* lineSmoothSpinBox;
  QSpinBox* fieldSmoothSpinBox;
  QSpinBox* labelSizeSpinBox;
  QComboBox* valuePrecisionBox;
  QCheckBox* gridValueCheckBox;
  QSpinBox* gridLinesSpinBox;
  QSpinBox* hourOffsetSpinBox;
  QSpinBox* hourDiffSpinBox;
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
  QSpinBox* shadingSpinBox;
  QSpinBox* shadingcoldSpinBox;
  QComboBox* patternComboBox;
  QComboBox* patternColourBox;
  QSpinBox* alphaSpinBox;
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

#endif // _fielddialogstyle_h
