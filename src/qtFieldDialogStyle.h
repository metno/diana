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

#ifndef _fielddialogstyle_h
#define _fielddialogstyle_h

#include <QWidget>

#include "diColourShading.h"
#include "diPattern.h"
#include "diPlotOptions.h"
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
class QDoubleSpinBox;

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

public:
  FieldDialogStyle(const fieldoptions_m& setupFieldOptions, QWidget* parent);
  virtual ~FieldDialogStyle();

  QWidget* standardWidget() const { return widgetStd; }
  QWidget* advancedWidget() const { return widgetAdv; }

  void updateFieldOptions(SelectedField* sf);
  void enableFieldOptions(const SelectedField* sf);
  const miutil::KeyValue_v& getFieldOptions(const std::string& fieldName, bool ignoreUserOptions) const;

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);

  void resetFieldOptions(SelectedField* selectedField);

Q_SIGNALS:
  void updateTime();

private Q_SLOTS:
  void plottypeComboBoxActivated(int index);
  void vectorunitCboxActivated(int index);
  void lineIntervalChanged(double value);
  void hourOffsetChanged(int value);
  void hourDiffChanged(int value);
  void undefMaskingActivated(int index);
  void colour2ComboBoxToggled(int index);
  void threeColoursChanged();
  void frameCheckBoxToggled(bool);

private:
  void CreateStandard();
  void CreateAdvanced();
  void toolTips();

  void setToPlotOptions(PlotOptions& po);
  void setFromPlotOptions(const PlotOptions& po, int dimension);

  void enableWidgets(const std::string& plottype);
  void enableType2Options(bool);

  const std::string& getPlotType() const;

private:
  QWidget* widgetStd;
  QWidget* widgetAdv;

  bool selectedFieldInEdit;

  const fieldoptions_m setupFieldOptions;
  fieldoptions_m fieldOptions;

  std::vector<std::string> plottypes;

  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  QStringList densityStringList;
  std::vector<std::string> vectorunit;
  std::vector<std::string> extremeType;
  std::vector<std::string> undefMasking;

  QLineEdit* unitLineEdit;
  QComboBox* plottypeComboBox;
  QComboBox* colorCbox;
  QComboBox* lineWidthCbox;
  QComboBox* lineTypeCbox;
  QDoubleSpinBox* spinLineInterval;
  QComboBox* densityCbox;
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
  QLineEdit* linevaluesField;
  QCheckBox* linevaluesLogCheckBox;
  QDoubleSpinBox* spinLineInterval2;
  QDoubleSpinBox* spinBaseValue1;
  QDoubleSpinBox* spinBaseValue2;
  QCheckBox* min1OnOff;
  QCheckBox* max1OnOff;
  QCheckBox* min2OnOff;
  QCheckBox* max2OnOff;
  QDoubleSpinBox* min1SpinBox;
  QDoubleSpinBox* max1SpinBox;
  QDoubleSpinBox* min2SpinBox;
  QDoubleSpinBox* max2SpinBox;
  QComboBox* linewidth2ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* linetype2ComboBox;

  bool frameFill;
};

#endif // _fielddialogstyle_h
