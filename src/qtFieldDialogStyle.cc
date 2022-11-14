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

#include "qtFieldDialogStyle.h"

#include "diColourShading.h"
#include "diFieldUtil.h"
#include "diLinetype.h"
#include "diPattern.h"
#include "diPlotOptions.h"
#include "qtFieldDialog.h" // for struct SelectedField
#include "qtUtility.h"
#include "util/diKeyValue.h"
#include "util/qtAnyDoubleSpinBox.h"

#include <mi_fieldcalc/FieldDefined.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>
#include <iterator>

#define MILOGGER_CATEGORY "diana.FieldDialogStyle"
#include <miLogger/miLogging.h>


namespace { // anonymous

const std::string OFF = "off";

std::vector<std::string> numberList(QComboBox* cBox, float number, bool onoff)
{
  const float enormal[] = {1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5., 5.5, 6., 6.5, 7., 7.5, 8., 8.5, 9., 9.5, -1};
  return diutil::numberList(cBox, number, enormal, onoff);
}

void setListCombo(QComboBox* box, const std::vector<std::string>& available, const std::string& value)
{
  const auto it = std::find(available.begin(), available.end(), value);
  size_t i = 0;
  if (it != available.end())
    i = std::distance(available.begin(), it);
  box->setCurrentIndex(i);
}

const std::string& getListCombo(QComboBox* box, const std::vector<std::string>& available)
{
  if (available.empty())
    throw std::runtime_error("empty list of available values");
  const int index = miutil::constrain_value(box->currentIndex(), 0, (int)available.size() - 1);
  return available[index];
}

std::string getMinMaxOption(diutil::AnyDoubleSpinBox* spin)
{
  return spin->isOff() ? "off" : spin->text().toStdString();
}

void setMinMaxSpinBox(diutil::AnyDoubleSpinBox* spin, double value)
{
  if (value == -fieldUndef || value == +fieldUndef)
    value = std::nan("");
  spin->setValue(value);
}

QString formatDensity(int density, float densityFactor)
{
  if (density != 0)
    return QString::number(density);
  else if (densityFactor == 1)
    return "Auto";
  else
    return QString("auto(%1)").arg(densityFactor);
}

int fractionToPercent(float fraction)
{
  return (int(fraction * 100. + 0.5)) / 5 * 5;
}

double replaceNaN(double value, double replacement)
{
  return std::isnan(value) ? replacement : value;
}

const std::vector<std::string>& getPlotTypesForDim(int dim)
{
  // clang-format off
  static const std::vector<std::string> plottypes_all{
    fpt_contour,
    fpt_contour1,
    fpt_contour2,
    fpt_value,
    fpt_symbol,
    fpt_alpha_shade,
    fpt_rgb,
    fpt_alarm_box,
    fpt_fill_cell,
    fpt_direction,
    fpt_wind,
    fpt_vector,
    fpt_wind_temp_fl,
    fpt_wind_value,
    fpt_streamlines,
    fpt_frame
  };

  static const std::vector<std::string> plottypes_dim_1{
    fpt_contour,
    fpt_contour1,
    fpt_contour2,
    fpt_value,
    fpt_symbol,
    fpt_alpha_shade,
    fpt_alarm_box,
    fpt_fill_cell,
    fpt_direction,
    fpt_frame
  };

  static const std::vector<std::string> plottypes_dim_2{
    fpt_wind,
    fpt_vector,
    fpt_direction,
    fpt_streamlines,
    fpt_value,
    fpt_frame
  };

  static const std::vector<std::string> plottypes_dim_3{
    fpt_wind,
    fpt_vector,
    fpt_value,
    fpt_rgb,
    fpt_wind_temp_fl,
    fpt_wind_value,
    fpt_frame
  };

  static const std::vector<std::string> plottypes_dim_4{
    fpt_value,
    fpt_frame
  };
  // clang-format on

  switch (std::min(dim, 4)) {
  case 1:
    return plottypes_dim_1;
  case 2:
    return plottypes_dim_2;
  case 3:
    return plottypes_dim_3;
  case 4:
    return plottypes_dim_4;
  }
  return plottypes_all;
}

} // anonymous namespace

FieldDialogStyle::FieldDialogStyle(const fieldoptions_m& sfo, QWidget* parent)
    : widgetStd(new QWidget(parent))
    , widgetAdv(new QWidget(parent))
    , selectedFieldInEdit(false)
    , setupFieldOptions(sfo)
{
  METLIBS_LOG_SCOPE();

  // Colours
  csInfo = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();

  plottypes = getPlotTypesForDim(1);

  // linetypes
  linetypes = Linetype::getLinetypeNames();

  // density (of arrows etc, 0=automatic)
  densityStringList << "Auto";
  for (int i = 1; i < 10; i++)
    densityStringList << formatDensity(i, 1);
  for (int i = 10; i < 30; i += 5)
    densityStringList << formatDensity(i, 1);
  for (int i = 30; i < 60; i += 10)
    densityStringList << formatDensity(i, 1);
  densityStringList << formatDensity(100, 1);
  densityStringList << formatDensity(0, 0.5);
  densityStringList << formatDensity(0, 0.6);
  densityStringList << formatDensity(0, 0.7);
  densityStringList << formatDensity(0, 0.8);
  densityStringList << formatDensity(0, 0.9);
  densityStringList << formatDensity(0, 2);
  densityStringList << formatDensity(0, 3);
  densityStringList << formatDensity(0, 4);
  densityStringList << "-1";

  CreateStandard();
  CreateAdvanced();
  toolTips();
}

FieldDialogStyle::~FieldDialogStyle() {}

void FieldDialogStyle::CreateStandard()
{
  QLabel* unitlabel = new QLabel(tr("Unit"), widgetStd);
  unitLineEdit = new QLineEdit(widgetStd);

  // plottype
  QLabel* plottypelabel = new QLabel(tr("Plot type"), widgetStd);
  plottypeComboBox = ComboBox(widgetStd, plottypes);
  connect(plottypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::plottypeComboBoxActivated);

  // colorCbox
  QLabel* colorlabel = new QLabel(tr("Line colour"), widgetStd);
  colorCbox = ColourBox(widgetStd, false, 0, tr("off").toStdString(), true);
  colorCbox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  // linewidthcbox
  QLabel* linewidthlabel = new QLabel(tr("Line width"), widgetStd);
  lineWidthCbox = LinewidthBox(widgetStd, false);

  // linetypecbox
  QLabel* linetypelabel = new QLabel(tr("Line type"), widgetStd);
  lineTypeCbox = LinetypeBox(widgetStd, false);

  // lineinterval
  QLabel* lineintervallabel = new QLabel(tr("Line interval"), widgetStd);
  spinLineInterval = new diutil::AnyDoubleSpinBox(widgetStd);
  spinLineInterval->setMinimum(0);
  spinLineInterval->setSpecialValueText(tr("Off"));
  connect(spinLineInterval, &diutil::AnyDoubleSpinBox::valueChanged, this, &FieldDialogStyle::lineIntervalChanged);

  // density
  QLabel* densitylabel = new QLabel(tr("Density"), widgetStd);
  densityCbox = new QComboBox(widgetStd);
  densityCbox->addItems(densityStringList);

  // vectorunit
  QLabel* vectorunitlabel = new QLabel(tr("Unit"), widgetStd);
  vectorunitCbox = new QComboBox(widgetStd);
  connect(vectorunitCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::vectorunitCboxActivated);

  // layout

  QGridLayout* optlayout = new QGridLayout();
  optlayout->setMargin(0);
  optlayout->addWidget(unitlabel, 0, 0);
  optlayout->addWidget(unitLineEdit, 0, 1);
  optlayout->addWidget(plottypelabel, 1, 0);
  optlayout->addWidget(plottypeComboBox, 1, 1);
  optlayout->addWidget(colorlabel, 2, 0);
  optlayout->addWidget(colorCbox, 2, 1);
  optlayout->addWidget(linewidthlabel, 3, 0);
  optlayout->addWidget(lineWidthCbox, 3, 1);
  optlayout->addWidget(linetypelabel, 4, 0);
  optlayout->addWidget(lineTypeCbox, 4, 1);
  optlayout->addWidget(lineintervallabel, 5, 0);
  optlayout->addWidget(spinLineInterval, 5, 1);
  optlayout->addWidget(densitylabel, 6, 0);
  optlayout->addWidget(densityCbox, 6, 1);
  optlayout->addWidget(vectorunitlabel, 7, 0);
  optlayout->addWidget(vectorunitCbox, 7, 1);
  widgetStd->setLayout(optlayout);

  diutil::setTabOrder({unitLineEdit, plottypeComboBox, colorCbox, lineWidthCbox, lineTypeCbox, spinLineInterval, densityCbox, vectorunitCbox});
}

void FieldDialogStyle::CreateAdvanced()
{
  METLIBS_LOG_SCOPE();

  // Extreme (min,max): type, size and search radius
  QLabel* extremeTypeLabel = TitleLabel(tr("Min,max"), widgetAdv);
  extremeType.push_back("None");
  extremeType.push_back("L+H");
  extremeType.push_back("L+H+Value");
  extremeType.push_back("C+W");
  extremeType.push_back("Value");
  extremeType.push_back("Minvalue");
  extremeType.push_back("Maxvalue");
  extremeTypeCbox = ComboBox(widgetAdv, extremeType);

  QLabel* extremeSizeLabel = new QLabel(tr("Size"), widgetAdv);
  extremeSizeSpinBox = new QSpinBox(widgetAdv);
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);

  QLabel* extremeRadiusLabel = new QLabel(tr("Radius"), widgetAdv);
  extremeRadiusSpinBox = new QSpinBox(widgetAdv);
  extremeRadiusSpinBox->setMinimum(5);
  extremeRadiusSpinBox->setMaximum(300);
  extremeRadiusSpinBox->setSingleStep(5);
  extremeRadiusSpinBox->setWrapping(true);
  extremeRadiusSpinBox->setSuffix("%");
  extremeRadiusSpinBox->setValue(100);

  // line smoothing
  QLabel* lineSmoothLabel = new QLabel(tr("Smooth lines"), widgetAdv);
  lineSmoothSpinBox = new QSpinBox(widgetAdv);
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);

  // field smoothing
  QLabel* fieldSmoothLabel = new QLabel(tr("Smooth fields"), widgetAdv);
  fieldSmoothSpinBox = new QSpinBox(widgetAdv);
  fieldSmoothSpinBox->setMinimum(0);
  fieldSmoothSpinBox->setMaximum(20);
  fieldSmoothSpinBox->setSingleStep(1);
  fieldSmoothSpinBox->setSpecialValueText(tr("Off"));
  fieldSmoothSpinBox->setValue(0);

  labelSizeSpinBox = new QSpinBox(widgetAdv);
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(300);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);

  valuePrecisionBox = new QComboBox(widgetAdv);
  valuePrecisionBox->addItem("0");
  valuePrecisionBox->addItem("1");
  valuePrecisionBox->addItem("2");
  valuePrecisionBox->addItem("3");

  // grid values
  gridValueCheckBox = new QCheckBox(QString(tr("Grid value")), widgetAdv);
  gridValueCheckBox->setChecked(false);

  // grid lines
  QLabel* gridLinesLabel = new QLabel(tr("Grid lines"), widgetAdv);
  gridLinesSpinBox = new QSpinBox(widgetAdv);
  gridLinesSpinBox->setMinimum(0);
  gridLinesSpinBox->setMaximum(50);
  gridLinesSpinBox->setSingleStep(1);
  gridLinesSpinBox->setSpecialValueText(tr("Off"));
  gridLinesSpinBox->setValue(0);

  QLabel* hourOffsetLabel = new QLabel(tr("Time offset"), widgetAdv);
  hourOffsetSpinBox = new QSpinBox(widgetAdv);
  hourOffsetSpinBox->setMinimum(-72);
  hourOffsetSpinBox->setMaximum(72);
  hourOffsetSpinBox->setSingleStep(1);
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  connect(hourOffsetSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::hourOffsetChanged);

  QLabel* hourDiffLabel = new QLabel(tr("Time diff."), widgetAdv);
  hourDiffSpinBox = new QSpinBox(widgetAdv);
  hourDiffSpinBox->setMinimum(0);
  hourDiffSpinBox->setMaximum(24);
  hourDiffSpinBox->setSingleStep(1);
  hourDiffSpinBox->setSuffix(tr(" hour(s)"));
  hourDiffSpinBox->setPrefix(" +/-");
  hourDiffSpinBox->setValue(0);
  connect(hourDiffSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::hourDiffChanged);

  // Undefined masking
  QLabel* undefMaskingLabel = TitleLabel(tr("Undefined"), widgetAdv);
  undefMasking.push_back(tr("Unmarked").toStdString());
  undefMasking.push_back(tr("Coloured").toStdString());
  undefMasking.push_back(tr("Lines").toStdString());
  undefMaskingCbox = ComboBox(widgetAdv, undefMasking);
  connect(undefMaskingCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::undefMaskingActivated);

  // Undefined masking colour
  undefColourCbox = ColourBox(widgetAdv, false, 0, "", true);

  // Undefined masking linewidth
  undefLinewidthCbox = LinewidthBox(widgetAdv, false);

  // Undefined masking linetype
  undefLinetypeCbox = LinetypeBox(widgetAdv, false);

  // enable/disable numbers on isolines
  valueLabelCheckBox = new QCheckBox(QString(tr("Numbers")), widgetAdv);
  valueLabelCheckBox->setChecked(true);

  // Options
  QLabel* shadingLabel = new QLabel(tr("Palette"), widgetAdv);
  QLabel* shadingcoldLabel = new QLabel(tr("Palette (-)"), widgetAdv);
  QLabel* patternLabel = new QLabel(tr("Pattern"), widgetAdv);
  QLabel* alphaLabel = new QLabel(tr("Alpha"), widgetAdv);
  QLabel* headLabel = TitleLabel(tr("Extra contour lines"), widgetAdv);
  QLabel* colourLabel = new QLabel(tr("Line colour"), widgetAdv);
  QLabel* intervalLabel = new QLabel(tr("Line interval"), widgetAdv);
  QLabel* baseLabel = new QLabel(tr("Basis value"), widgetAdv);
  QLabel* base2Label = new QLabel(tr("Basis value"), widgetAdv);
  QLabel* linewidthLabel = new QLabel(tr("Line width"), widgetAdv);
  QLabel* linetypeLabel = new QLabel(tr("Line type"), widgetAdv);
  QLabel* threeColourLabel = TitleLabel(tr("Three colours"), widgetAdv);

  tableCheckBox = new QCheckBox(tr("Table"), widgetAdv);

  repeatCheckBox = new QCheckBox(tr("Repeat"), widgetAdv);

  // 3 colours
  for (size_t i = 0; i < 3; i++) {
    threeColourBox.push_back(ColourBox(widgetAdv, true, 0, tr("Off").toStdString(), true));
    connect(threeColourBox[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::threeColoursChanged);
  }

  // shading
  shadingComboBox = PaletteBox(widgetAdv, csInfo, false, 0, tr("Off").toStdString(), true);
  shadingComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  shadingSpinBox = new QSpinBox(widgetAdv);
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(255);
  shadingSpinBox->setSingleStep(1);
  shadingSpinBox->setSpecialValueText(tr("Auto"));

  shadingcoldComboBox = PaletteBox(widgetAdv, csInfo, false, 0, tr("Off").toStdString(), true);
  shadingcoldComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  shadingcoldSpinBox = new QSpinBox(widgetAdv);
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(255);
  shadingcoldSpinBox->setSingleStep(1);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));

  // pattern
  patternComboBox = PatternBox(widgetAdv, patternInfo, false, 0, tr("Off").toStdString(), true);
  patternComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  // pattern colour
  patternColourBox = ColourBox(widgetAdv, false, 0, tr("Auto").toStdString(), true);

  // alpha blending
  alphaSpinBox = new QSpinBox(widgetAdv);
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
  alphaSpinBox->setValue(255);

  // colour
  colour2ComboBox = ColourBox(widgetAdv, false, 0, tr("Off").toStdString(), true);
  connect(colour2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::colour2ComboBoxToggled);

  // line interval
  spinLineInterval2 = new diutil::AnyDoubleSpinBox(widgetAdv);
  spinLineInterval2->setMinimum(0);
  spinLineInterval2->setSpecialValueText(tr("Off"));

  // zero value
  spinBaseValue1 = new diutil::AnyDoubleSpinBox(widgetAdv);
  spinBaseValue2 = new diutil::AnyDoubleSpinBox(widgetAdv);

  // min/max
  QLabel* labelMin1 = new QLabel(tr("Min"), widgetAdv);
  QLabel* labelMax1 = new QLabel(tr("Max"), widgetAdv);
  QLabel* labelMin2 = new QLabel(tr("Min"), widgetAdv);
  QLabel* labelMax2 = new QLabel(tr("Max"), widgetAdv);

  min1SpinBox = new diutil::AnyDoubleSpinBox(widgetAdv);
  min1SpinBox->setSpecialValueText(tr("Off"));
  max1SpinBox = new diutil::AnyDoubleSpinBox(widgetAdv);
  max1SpinBox->setSpecialValueText(tr("Off"));
  min2SpinBox = new diutil::AnyDoubleSpinBox(widgetAdv);
  min2SpinBox->setSpecialValueText(tr("Off"));
  max2SpinBox = new diutil::AnyDoubleSpinBox(widgetAdv);
  max2SpinBox->setSpecialValueText(tr("Off"));

  // line values
  linevaluesField = new QLineEdit(widgetAdv);

  // log line values
  linevaluesLogCheckBox = new QCheckBox(QString(tr("Log")), widgetAdv);
  linevaluesLogCheckBox->setChecked(false);

  // linewidth
  linewidth2ComboBox = LinewidthBox(widgetAdv);

  // linetype
  linetype2ComboBox = LinetypeBox(widgetAdv, false);

  // Plot frame
  frameCheckBox = new QCheckBox(QString(tr("Frame")), widgetAdv);
  frameCheckBox->setChecked(true);
  connect(frameCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::frameCheckBoxToggled);

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox = new QCheckBox(QString(tr("Zero line")), widgetAdv);
  zeroLineCheckBox->setChecked(true);

  // Create horizontal frame lines
  QFrame* line0 = new QFrame(widgetAdv);
  line0->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line1 = new QFrame(widgetAdv);
  line1->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line2 = new QFrame(widgetAdv);
  line2->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line3 = new QFrame(widgetAdv);
  line3->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line4 = new QFrame(widgetAdv);
  line4->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line5 = new QFrame(widgetAdv);
  line5->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  QFrame* line6 = new QFrame(widgetAdv);
  line6->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  // layout......................................................

  QGridLayout* advLayout = new QGridLayout();
  advLayout->setSpacing(1);
  int line = 0;
  advLayout->addWidget(extremeTypeLabel, line, 0);
  advLayout->addWidget(extremeSizeLabel, line, 1);
  advLayout->addWidget(extremeRadiusLabel, line, 2);
  line++;
  advLayout->addWidget(extremeTypeCbox, line, 0);
  advLayout->addWidget(extremeSizeSpinBox, line, 1);
  advLayout->addWidget(extremeRadiusSpinBox, line, 2);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line0, line, 0, 1, 3);

  line++;
  advLayout->addWidget(gridLinesLabel, line, 0);
  advLayout->addWidget(gridLinesSpinBox, line, 1);
  advLayout->addWidget(gridValueCheckBox, line, 2);
  line++;
  advLayout->addWidget(lineSmoothLabel, line, 0);
  advLayout->addWidget(lineSmoothSpinBox, line, 1);
  line++;
  advLayout->addWidget(fieldSmoothLabel, line, 0);
  advLayout->addWidget(fieldSmoothSpinBox, line, 1);
  line++;
  advLayout->addWidget(hourOffsetLabel, line, 0);
  advLayout->addWidget(hourOffsetSpinBox, line, 1);
  line++;
  advLayout->addWidget(hourDiffLabel, line, 0);
  advLayout->addWidget(hourDiffSpinBox, line, 1);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line4, line, 0, 1, 3);
  line++;
  advLayout->addWidget(undefMaskingLabel, line, 0);
  advLayout->addWidget(undefMaskingCbox, line, 1);
  line++;
  advLayout->addWidget(undefColourCbox, line, 0);
  advLayout->addWidget(undefLinewidthCbox, line, 1);
  advLayout->addWidget(undefLinetypeCbox, line, 2);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line1, line, 0, 1, 4);

  line++;
  advLayout->addWidget(frameCheckBox, line, 0);
  advLayout->addWidget(zeroLineCheckBox, line, 1);
  line++;
  advLayout->addWidget(valueLabelCheckBox, line, 0);
  advLayout->addWidget(labelSizeSpinBox, line, 1);
  advLayout->addWidget(valuePrecisionBox, line, 2);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line2, line, 0, 1, 3);

  line++;
  advLayout->addWidget(tableCheckBox, line, 0);
  advLayout->addWidget(repeatCheckBox, line, 1);
  line++;
  advLayout->addWidget(shadingLabel, line, 0);
  advLayout->addWidget(shadingComboBox, line, 1);
  advLayout->addWidget(shadingSpinBox, line, 2);
  line++;
  advLayout->addWidget(shadingcoldLabel, line, 0);
  advLayout->addWidget(shadingcoldComboBox, line, 1);
  advLayout->addWidget(shadingcoldSpinBox, line, 2);
  line++;
  advLayout->addWidget(patternLabel, line, 0);
  advLayout->addWidget(patternComboBox, line, 1);
  advLayout->addWidget(patternColourBox, line, 2);
  line++;
  advLayout->addWidget(alphaLabel, line, 0);
  advLayout->addWidget(alphaSpinBox, line, 1);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line6, line, 0, 1, 3);

  line++;
  advLayout->addWidget(baseLabel, line, 0);
  advLayout->addWidget(labelMin1, line, 1);
  advLayout->addWidget(labelMax1, line, 2);
  line++;
  advLayout->addWidget(spinBaseValue1, line, 0);
  advLayout->addWidget(min1SpinBox, line, 1);
  advLayout->addWidget(max1SpinBox, line, 2);
  line++;
  advLayout->addWidget(new QLabel(tr("Values"), widgetAdv), line, 0);
  advLayout->addWidget(linevaluesField, line, 1);
  advLayout->addWidget(linevaluesLogCheckBox, line, 2);
  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line3, line, 0, 1, 3);

  line++;
  advLayout->addWidget(headLabel, line, 0, 1, 2);
  line++;
  advLayout->addWidget(colourLabel, line, 0);
  advLayout->addWidget(colour2ComboBox, line, 1);
  line++;
  advLayout->addWidget(intervalLabel, line, 0);
  advLayout->addWidget(spinLineInterval2, line, 1);
  line++;
  advLayout->addWidget(linewidthLabel, line, 0);
  advLayout->addWidget(linewidth2ComboBox, line, 1);
  line++;
  advLayout->addWidget(linetypeLabel, line, 0);
  advLayout->addWidget(linetype2ComboBox, line, 1);
  line++;
  advLayout->addWidget(base2Label, line, 0);
  advLayout->addWidget(labelMin2, line, 1);
  advLayout->addWidget(labelMax2, line, 2);
  line++;
  advLayout->addWidget(spinBaseValue2, line, 0);
  advLayout->addWidget(min2SpinBox, line, 1);
  advLayout->addWidget(max2SpinBox, line, 2);

  line++;
  advLayout->setRowStretch(line, 5);
  advLayout->addWidget(line5, line, 0, 1, 3);
  line++;
  advLayout->addWidget(threeColourLabel, line, 0);
  //  advLayout->addWidget( threeColoursCheckBox, 38, 0 );
  line++;
  advLayout->addWidget(threeColourBox[0], line, 0);
  advLayout->addWidget(threeColourBox[1], line, 1);
  advLayout->addWidget(threeColourBox[2], line, 2);
  line++;
  advLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), line, 0);

  // a separator
  QFrame* advSep = new QFrame(widgetAdv);
  advSep->setFrameStyle(QFrame::VLine | QFrame::Raised);
  advSep->setLineWidth(5);

  QHBoxLayout* hLayout = new QHBoxLayout(widgetAdv);
  hLayout->setMargin(0);
  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);

  diutil::setTabOrder({extremeTypeCbox,    extremeSizeSpinBox,    extremeRadiusSpinBox, gridLinesSpinBox,   gridValueCheckBox,   lineSmoothSpinBox,
                       fieldSmoothSpinBox, hourOffsetSpinBox,     hourDiffSpinBox,      undefMaskingCbox,   undefColourCbox,     undefLinewidthCbox,
                       undefLinetypeCbox,  frameCheckBox,         zeroLineCheckBox,     valueLabelCheckBox, labelSizeSpinBox,    valuePrecisionBox,
                       tableCheckBox,      repeatCheckBox,        shadingComboBox,      shadingSpinBox,     shadingcoldComboBox, shadingcoldSpinBox,
                       patternComboBox,    patternColourBox,      alphaSpinBox,         spinBaseValue1,     min1SpinBox,         max1SpinBox,
                       linevaluesField,    linevaluesLogCheckBox, colour2ComboBox,      spinLineInterval2,  linewidth2ComboBox,  linetype2ComboBox,
                       spinBaseValue2,     min2SpinBox,           max2SpinBox,          threeColourBox[0],  threeColourBox[1],   threeColourBox[2]});
}

void FieldDialogStyle::toolTips()
{
  gridValueCheckBox->setToolTip(tr("Grid values but only when a few grid points are visible"));
  valueLabelCheckBox->setToolTip(tr("numbers on the contour lines"));
  labelSizeSpinBox->setToolTip(tr("Size of numbers on the countour lines and size of values in the plot type \"value\""));
  valuePrecisionBox->setToolTip(tr("Value precision, used in the plot type \"value\""));
  gridLinesSpinBox->setToolTip(tr("Grid lines, 1=all"));
  undefColourCbox->setToolTip(tr("Undef colour"));
  undefLinewidthCbox->setToolTip(tr("Undef linewidth"));
  undefLinetypeCbox->setToolTip(tr("Undef linetype"));
  frameCheckBox->setToolTip(tr("Draw a frame around the domain"));
  shadingSpinBox->setToolTip(tr("number of colours in the palette"));
  shadingcoldComboBox->setToolTip(tr("Palette for values below basis"));
  shadingcoldSpinBox->setToolTip(tr("number of colours in the palette"));
  patternColourBox->setToolTip(tr("Colour of pattern"));
}

void FieldDialogStyle::enableFieldOptions(const SelectedField* selectedField)
{
  METLIBS_LOG_SCOPE();

  if (!selectedField) {
    enableWidgets("none");
    return;
  }

  selectedFieldInEdit = selectedField->inEdit;



  if (selectedField->minus) {
    enableWidgets("none");
  } else {
    // unit -- not in PlotOptions
    unitLineEdit->setText(QString::fromStdString(selectedField->units));

    setFromPlotOptions(selectedField->po, selectedField->dimension);
  }

  // set hour offset / diff last, if these change, a Qt signal invokes updateTime,
  // which calls updateFieldOptions, which reads the UI settings, which must therefore
  // be updated before

  hourOffsetSpinBox->setValue(selectedFieldInEdit ? 0 : selectedField->hourOffset);
  hourOffsetSpinBox->setEnabled(!selectedFieldInEdit);

  hourDiffSpinBox->setValue(selectedFieldInEdit ? 0 : selectedField->hourDiff);
  hourDiffSpinBox->setEnabled(!selectedFieldInEdit);
}

namespace {
void setPalette(const std::string& value, std::vector<ColourShading::ColourShadingInfo>& csInfo, QComboBox* box, QComboBox* boxOther, QSpinBox* spin)
{
  if (value == "off") {
    box->setCurrentIndex(0);
    spin->setValue(0);
    return;
  }

  const auto n_c = miutil::split(value, ";");
  const std::string& name = n_c.front();
  const auto itH = std::find_if(csInfo.begin(), csInfo.end(), [&](const ColourShading::ColourShadingInfo& ci) { return ci.name == name; });
  if (itH == csInfo.end()) {
    // defining a new colour shading only makes sense if "name" is actually a list of colours
    ColourShading::defineColourShadingFromString(name);
    const ColourShading csh(name);
    ExpandPaletteBox(box, csh);
    ExpandPaletteBox(boxOther, csh);
    box->setCurrentIndex(box->count() - 1);

    ColourShading::ColourShadingInfo info;
    info.name = name;
    info.colour = ColourShading::getColourShading(name);
    csInfo.push_back(std::move(info));
  } else {
    box->setCurrentIndex(1 + std::distance(csInfo.begin(), itH)); // first is "off"
  }

  const int count = (n_c.size() == 2) ? miutil::to_int(n_c[1]) : 0;
  spin->setValue(count);
}

void getPalette(std::string& str, const std::vector<ColourShading::ColourShadingInfo>& csInfo, int box_index, QSpinBox* spin)
{
  str += csInfo[box_index - 1].name;
  int count = spin->value();
  if (count > 0)
    str += ";" + miutil::from_number(count);
}
} // namespace

void FieldDialogStyle::setFromPlotOptions(const PlotOptions& po, int dimension)
{
  // dimension (1dim = contour,..., 2dim=wind,...)
  {
    plottypes = getPlotTypesForDim(dimension);
    plottypeComboBox->clear();
    for (const std::string& pt : plottypes)
      plottypeComboBox->addItem(QString::fromStdString(pt));
  }

  // plottype
  {
    const auto itPT = std::find(plottypes.begin(), plottypes.end(), po.plottype);
    size_t i = 0;
    if (itPT != plottypes.end())
      i = std::distance(plottypes.begin(), itPT);

    plottypeComboBox->setCurrentIndex(i);
    enableWidgets(plottypes[i]);
  }

  // colour(s)
  if (po.options_1) {
    SetCurrentItemColourBox(colorCbox, po.linecolour.Name());
  } else {
    colorCbox->setCurrentIndex(0);
  }
  if (po.options_2) {
    SetCurrentItemColourBox(colour2ComboBox, po.linecolour_2.Name());
  } else {
    colour2ComboBox->setCurrentIndex(0);
  }

  // 3 colours
  if (po.colours.size() == 3) {
    for (size_t j = 0; j < 3; j++)
      SetCurrentItemColourBox(threeColourBox[j], po.colours[j].Name());
  } else {
    for (size_t j = 0; j < 3; j++)
      threeColourBox[j]->setCurrentIndex(0);
  }
  threeColoursChanged();

  // contour shading updating FieldOptions
  if (po.palettename.empty()) {
    shadingComboBox->setCurrentIndex(0);
    shadingcoldComboBox->setCurrentIndex(0);
    shadingSpinBox->setValue(0);
    shadingcoldSpinBox->setValue(0);
  } else {
    const auto items = miutil::split(po.palettename, ",");
    if (items.size() > 2) {
      // hot palette, user-defined
      setPalette(po.palettename, csInfo, shadingComboBox, shadingcoldComboBox, shadingSpinBox);
      shadingcoldComboBox->setCurrentIndex(0);
      shadingcoldSpinBox->setValue(0);
    } else if (items.size() == 2) {
      // hot & cold palette by name, each possibly with ";count" appended
      setPalette(items[0], csInfo, shadingComboBox, shadingcoldComboBox, shadingSpinBox);
      setPalette(items[1], csInfo, shadingcoldComboBox, shadingComboBox, shadingcoldSpinBox);
    } else {
      // hot palette by name, possibly with ";count" appended
      setPalette(items[0], csInfo, shadingComboBox, shadingcoldComboBox, shadingSpinBox);
      shadingcoldComboBox->setCurrentIndex(0);
      shadingcoldSpinBox->setValue(0);
    }
  }

  // pattern
  {
    int idx = 0;
    if (!po.patternname.empty()) {
      const auto itP = std::find_if(patternInfo.begin(), patternInfo.end(), [&](Pattern::PatternInfo& pi) { return pi.name == po.patternname; });
      if (itP != patternInfo.end())
        idx = std::distance(patternInfo.begin(), itP) + 1;
    }
    patternComboBox->setCurrentIndex(idx);
  }

  // pattern colour
  if (!po.patternname.empty()) {
    SetCurrentItemColourBox(patternColourBox, po.fillcolour.Name());
  } else {
    patternColourBox->setCurrentIndex(0);
  }

  // table
  tableCheckBox->setChecked(po.table);

  // repeat
  repeatCheckBox->setChecked(po.repeat);

  // alpha shading
  alphaSpinBox->setValue(po.alpha);

  // linetype
  setListCombo(lineTypeCbox, linetypes, po.linetype.name);
  setListCombo(linetype2ComboBox, linetypes, po.linetype_2.name);

  // linewidth
  {
    const int lw = po.linewidth;
    if (lw > lineWidthCbox->count())
      ExpandLinewidthBox(lineWidthCbox, lw);
    lineWidthCbox->setCurrentIndex(lw - 1);
  }
  {
    const int lw2 = po.linewidth_2;
    if (lw2 > linewidth2ComboBox->count())
      ExpandLinewidthBox(linewidth2ComboBox, lw2);
    linewidth2ComboBox->setCurrentIndex(lw2 - 1);
  }

  // line interval and line values (contour and fill_cell)
  {
    const bool have_log = !po.loglinevalues().empty(), have_val = !po.linevalues().empty();
    if (have_log || have_val) {
      const auto& lv = have_log ? po.loglinevalues() : po.linevalues();
      const std::string lvtext = miutil::kv("dummy", lv).value();
      linevaluesField->setText(QString::fromStdString(lvtext));
      linevaluesLogCheckBox->setChecked(have_log);
    } else {
      linevaluesField->clear();
      linevaluesLogCheckBox->setChecked(false);
    }
    spinLineInterval->setValue(std::max(po.lineinterval, 0.0f));
    lineIntervalChanged(po.lineinterval);
  }

  spinLineInterval2->setValue(std::max(po.lineinterval_2, 0.0f));

  // wind/vector density
  {
    const QString s = formatDensity(po.density, po.densityFactor);
    int i = i = densityStringList.indexOf(s);
    if (i == -1) {
      densityStringList << s;
      densityCbox->addItem(s);
      i = densityCbox->count() - 1;
    }
    densityCbox->setCurrentIndex(i);
  }

  // vectorunit (vector length unit)
  vectorunit = numberList(vectorunitCbox, po.vectorunit, false);

  // extreme.type (L+H, C+W or none)
  setListCombo(extremeTypeCbox, extremeType, po.extremeType);

  extremeSizeSpinBox->setValue(fractionToPercent(po.extremeSize));
  extremeRadiusSpinBox->setValue(fractionToPercent(po.extremeRadius));

  lineSmoothSpinBox->setValue(po.lineSmooth);

  if (selectedFieldInEdit) {
    fieldSmoothSpinBox->setValue(0);
    fieldSmoothSpinBox->setEnabled(false);
  } else {
    fieldSmoothSpinBox->setValue(po.fieldSmooth);
  }

  labelSizeSpinBox->setValue(fractionToPercent(po.labelSize));

  valuePrecisionBox->setCurrentIndex(po.precision);

  gridValueCheckBox->setChecked(po.gridValue);
  gridLinesSpinBox->setValue(po.gridLines);

  // undefined masking
  {
    const int iumask = miutil::constrain_value(po.undefMasking, 0, int(undefMasking.size()));
    undefMaskingCbox->setCurrentIndex(iumask);
    undefMaskingActivated(iumask);
  }

  // undefined masking colour
  SetCurrentItemColourBox(undefColourCbox, po.undefColour.Name());

  // undefined masking linewidth
  {
    const int lw = po.undefLinewidth;
    if (lw > undefLinewidthCbox->count())
      ExpandLinewidthBox(undefLinewidthCbox, lw);
    undefLinewidthCbox->setCurrentIndex(lw - 1);
  }

  // undefined masking linetype
  setListCombo(undefLinetypeCbox, linetypes, po.undefLinetype.name);

  frameCheckBox->setChecked((po.frame & 1) != 0);
  frameFill = (po.frame & 2) != 0;

  zeroLineCheckBox->setChecked(po.zeroLine);

  valueLabelCheckBox->setChecked(po.valueLabel);

  // base
  spinBaseValue1->setValue(po.base);
  spinBaseValue2->setValue(po.base_2);

  setMinMaxSpinBox(max1SpinBox, po.maxvalue);
  setMinMaxSpinBox(min1SpinBox, po.minvalue);
  setMinMaxSpinBox(max2SpinBox, po.maxvalue_2);
  setMinMaxSpinBox(min2SpinBox, po.minvalue_2);
}

void FieldDialogStyle::enableWidgets(const std::string& plottype)
{
  METLIBS_LOG_SCOPE("plottype=" << plottype);

  const bool pt_contour = plottype == fpt_contour || plottype == fpt_contour1 || plottype == fpt_contour2;
  const bool pt_value_symbol = plottype == fpt_value || plottype == fpt_symbol;
  const bool pt_wind_temp_value = plottype == fpt_wind_temp_fl || plottype == fpt_wind_value;
  const bool pt_wind_vector_direction = plottype == fpt_wind || plottype == fpt_vector || plottype == fpt_direction;
  const bool pt_alpha_rgb = plottype == fpt_alpha_shade || plottype == fpt_rgb;
  const bool pt_streamlines = plottype == fpt_streamlines;
  const bool pt_fill_cell = plottype == fpt_fill_cell;

  {
    // used for all plottypes
    const bool enable = (plottype != "none");
    unitLineEdit->setEnabled(enable);
    plottypeComboBox->setEnabled(enable);
    colorCbox->setEnabled(enable);
    fieldSmoothSpinBox->setEnabled(enable);
    gridValueCheckBox->setEnabled(enable);
    gridLinesSpinBox->setEnabled(enable);
    hourOffsetSpinBox->setEnabled(enable);
    hourDiffSpinBox->setEnabled(enable);
    frameCheckBox->setEnabled(enable);
    spinBaseValue1->setEnabled(enable);

    const bool e_nostream = enable && !pt_streamlines;
    undefMaskingCbox->setEnabled(e_nostream);
    undefColourCbox->setEnabled(e_nostream);
    undefLinewidthCbox->setEnabled(e_nostream);
    undefLinetypeCbox->setEnabled(e_nostream);

    min1SpinBox->setEnabled(e_nostream);
    max1SpinBox->setEnabled(e_nostream);
    for (int i = 0; i < 3; i++) {
      threeColourBox[i]->setEnabled(e_nostream);
    }
  }
  {
    const bool e_contour = pt_contour;
    lineTypeCbox->setEnabled(e_contour);
    lineSmoothSpinBox->setEnabled(e_contour);
    zeroLineCheckBox->setEnabled(e_contour);
    colour2ComboBox->setEnabled(e_contour);
    spinLineInterval2->setEnabled(e_contour);
    spinBaseValue2->setEnabled(e_contour);
    min2SpinBox->setEnabled(e_contour);
    max2SpinBox->setEnabled(e_contour);
    linewidth2ComboBox->setEnabled(e_contour);
    linetype2ComboBox->setEnabled(e_contour);
    valueLabelCheckBox->setEnabled(e_contour);
  }
  {
    const bool e_extreme = pt_contour || pt_alpha_rgb;
    extremeTypeCbox->setEnabled(e_extreme);
    extremeSizeSpinBox->setEnabled(e_extreme);
    extremeRadiusSpinBox->setEnabled(e_extreme);
  }
  {
    const bool e_shading = pt_contour || pt_fill_cell;
    spinLineInterval->setEnabled(e_shading || pt_streamlines);
    tableCheckBox->setEnabled(e_shading || pt_streamlines);
    repeatCheckBox->setEnabled(e_shading);
    shadingComboBox->setEnabled(e_shading || pt_streamlines);
    shadingSpinBox->setEnabled(e_shading || pt_streamlines);
    shadingcoldComboBox->setEnabled(e_shading);
    shadingcoldSpinBox->setEnabled(e_shading);
    patternComboBox->setEnabled(e_shading);
    patternColourBox->setEnabled(e_shading);
    alphaSpinBox->setEnabled(e_shading);

    lineIntervalChanged(spinLineInterval->value());
  }
  {
    const bool e_line = pt_contour || pt_wind_temp_value || pt_wind_vector_direction || pt_streamlines;
    lineWidthCbox->setEnabled(e_line);
  }
  {
    const bool e_font = pt_contour || pt_wind_temp_value || pt_value_symbol;
    labelSizeSpinBox->setEnabled(e_font);
    valuePrecisionBox->setEnabled(e_font);
  }
  {
    const bool e_density = pt_value_symbol || pt_fill_cell || pt_wind_vector_direction || pt_wind_temp_value;
    densityCbox->setEnabled(e_density);
  }
  {
    const bool e_vectorunit = pt_wind_vector_direction || pt_wind_temp_value;
    vectorunitCbox->setEnabled(e_vectorunit);
  }
}

void FieldDialogStyle::updateFieldOptions(SelectedField* selectedField)
{
  if (!selectedField)
    return;

  selectedField->hourOffset = hourOffsetSpinBox->value();
  selectedField->hourDiff = hourDiffSpinBox->value();
  selectedField->units = unitLineEdit->text().toStdString();
  setToPlotOptions(selectedField->po);

  fieldOptions[selectedField->fieldName] = selectedField->getFieldPlotOptions();
}

const std::string& FieldDialogStyle::getPlotType() const
{
  return plottypes[plottypeComboBox->currentIndex()];
}

void FieldDialogStyle::setToPlotOptions(PlotOptions& po)
{
  po.plottype = getPlotType();

  if (colorCbox->currentIndex() == 0) {
    po.options_1 = false;
  } else {
    po.set_colour(colorCbox->currentText().toStdString());
  }
  if (colour2ComboBox->currentIndex() == 0) {
    po.options_2 = false;
  } else {
    po.set_colour_2(colour2ComboBox->currentText().toStdString());
  }

  po.linewidth = lineWidthCbox->currentIndex() + 1;
  po.linewidth_2 = linewidth2ComboBox->currentIndex() + 1;
  po.set_linetype(getListCombo(lineTypeCbox, linetypes));
  po.set_linetype_2(getListCombo(linetype2ComboBox, linetypes));
  {
    std::string lvtext = linevaluesField->text().toStdString();
    std::string llvtext;
    if (linevaluesLogCheckBox->isChecked())
      std::swap(lvtext, llvtext);
    po.set_loglinevalues(llvtext);
    po.set_linevalues(lvtext);
  }
  po.set_lineinterval(replaceNaN(spinLineInterval->value(), 0));
  po.set_lineinterval_2(replaceNaN(spinLineInterval2->value(), 0));

  if (densityCbox->currentIndex() == 0) {
    po.density = 0;
    po.densityFactor = 1;
  } else {
    po.set_density(densityCbox->currentText().toStdString());
  }

  if (vectorunitCbox->currentIndex() > 0) {
    po.vectorunit = miutil::to_float(getListCombo(vectorunitCbox, vectorunit));
  }

  po.extremeType = extremeTypeCbox->currentText().toStdString();
  po.extremeSize = extremeSizeSpinBox->value() * 0.01;
  po.extremeRadius = extremeRadiusSpinBox->value() * 0.01;

  po.lineSmooth = lineSmoothSpinBox->value();
  po.fieldSmooth = fieldSmoothSpinBox->value();

  po.labelSize = labelSizeSpinBox->value() * 0.01;
  po.precision = valuePrecisionBox->currentIndex();
  po.gridValue = gridValueCheckBox->isChecked();
  po.gridLines = gridLinesSpinBox->value();

  po.undefMasking = undefMaskingCbox->currentIndex();
  po.undefColour = Colour(undefColourCbox->currentText().toStdString());
  po.undefLinewidth = undefLinewidthCbox->currentIndex() + 1;
  po.undefLinetype = undefLinetypeCbox->currentText().toStdString();

  po.frame = (frameCheckBox->isChecked() ? 1 : 0) | (frameFill ? 2 : 0);
  po.zeroLine = zeroLineCheckBox->isChecked();
  po.valueLabel = valueLabelCheckBox->isChecked();
  po.table = tableCheckBox->isChecked();
  po.repeat = repeatCheckBox->isChecked();

  if (patternComboBox->currentIndex() > 0) {
    po.set_patterns(patternInfo[patternComboBox->currentIndex() - 1].name);
  } else if (patternColourBox->currentIndex() > 0) {
    po.fillcolour = Colour(patternColourBox->currentText().toStdString());
  } else {
    po.set_patterns(OFF);
  }

  if (threeColourBox[0]->currentIndex() != 0 && threeColourBox[1]->currentIndex() != 0 && threeColourBox[2]->currentIndex() != 0) {
    po.colours.clear();
    po.colours.push_back(Colour(threeColourBox[0]->currentText().toStdString()));
    po.colours.push_back(Colour(threeColourBox[1]->currentText().toStdString()));
    po.colours.push_back(Colour(threeColourBox[2]->currentText().toStdString()));
  }

  {
    const int index1 = shadingComboBox->currentIndex();
    const int index2 = shadingcoldComboBox->currentIndex();
    std::string str;
    if (index1 == 0) {
      str = OFF;
    } else {
      getPalette(str, csInfo, index1, shadingSpinBox);
    }
    if (index2 > 0) {
      str += ",";
      getPalette(str, csInfo, index2, shadingcoldSpinBox);
    }
    po.set_palettecolours(str);
  }

  po.alpha = alphaSpinBox->value();
  po.base = spinBaseValue1->value();
  po.base_2 = spinBaseValue2->value();

  po.set_maxvalue(getMinMaxOption(max1SpinBox));
  po.set_minvalue(getMinMaxOption(min1SpinBox));
  po.set_maxvalue_2(getMinMaxOption(max2SpinBox));
  po.set_minvalue_2(getMinMaxOption(min2SpinBox));
}

void FieldDialogStyle::plottypeComboBoxActivated(int index)
{
  enableWidgets(plottypes[index]);
}

void FieldDialogStyle::vectorunitCboxActivated(int index)
{
  if (index >= 0) {
    // update the list (with selected value in the middle)
    float a = miutil::to_float(vectorunit[index]);
    vectorunit = numberList(vectorunitCbox, a, false);
  }
}

void FieldDialogStyle::lineIntervalChanged(double interval)
{
  const std::string& plottype = getPlotType();
  const bool e_shading = plottype == fpt_contour || plottype == fpt_contour1 || plottype == fpt_contour2 || plottype == fpt_fill_cell;

  interval = replaceNaN(interval, 0);
  const bool enable_linevalues = e_shading && (interval == 0);
  linevaluesField->setEnabled(enable_linevalues);
  linevaluesLogCheckBox->setEnabled(enable_linevalues);
}

void FieldDialogStyle::hourOffsetChanged(int value)
{
  /*Q_EMIT*/ updateTime();
}

void FieldDialogStyle::hourDiffChanged(int value)
{
  /*Q_EMIT*/ updateTime();
}

void FieldDialogStyle::undefMaskingActivated(int index)
{
  undefColourCbox->setEnabled(index > 0);
  undefLinewidthCbox->setEnabled(index > 1);
  undefLinetypeCbox->setEnabled(index > 1);
}

void FieldDialogStyle::colour2ComboBoxToggled(int index)
{
  if (index == 0) {
    enableType2Options(false);
    colour2ComboBox->setEnabled(true);
  } else {
    enableType2Options(true); // check if needed
    // turn of 3 colours (not possible to combine threeCols and col_2)
    for (int i = 0; i < 3; ++i)
      threeColourBox[i]->setCurrentIndex(0);
    threeColoursChanged();
  }
}

void FieldDialogStyle::threeColoursChanged()
{
  if (threeColourBox[0]->currentIndex() != 0 && threeColourBox[1]->currentIndex() != 0 && threeColourBox[2]->currentIndex() != 0) {
    // turn of colour_2 (not possible to combine threeCols and col_2)
    colour2ComboBox->setCurrentIndex(0);
    colour2ComboBoxToggled(0);
  }
}

void FieldDialogStyle::frameCheckBoxToggled(bool)
{
  frameFill = false;
}

void FieldDialogStyle::enableType2Options(bool on)
{
  colour2ComboBox->setEnabled(on);

  // enable the rest only if colour2 is on
  on = (colour2ComboBox->currentIndex() != 0);

  spinLineInterval2->setEnabled(on);
  spinBaseValue2->setEnabled(on);
  min2SpinBox->setEnabled(on);
  max2SpinBox->setEnabled(on);
  linewidth2ComboBox->setEnabled(on);
  linetype2ComboBox->setEnabled(on);
}

inline std::string sub(const std::string& s, std::string::size_type begin, std::string::size_type end)
{
  return s.substr(begin, end - begin);
}

std::vector<std::string> FieldDialogStyle::writeLog()
{
  std::vector<std::string> vstr;

  // write used field options
  for (const auto& fo : fieldOptions) {
    miutil::KeyValue_v sopts = getFieldOptions(fo.first, true);
    // only logging options if different from setup
    if (sopts != fo.second)
      vstr.push_back(fo.first + " " + miutil::mergeKeyValue(fo.second));
  }

  vstr.push_back("================");

  return vstr;
}

void FieldDialogStyle::readLog(const std::vector<std::string>& vstr, const std::string& /*thisVersion*/, const std::string& /*logVersion*/)
{
  // field options:
  // do not destroy any new options in the program
  for (const std::string& ls : vstr) {
    if (ls.empty())
      continue;
    if (ls.substr(0, 4) == "====")
      break;
    const size_t first_space = ls.find_first_of(' ');
    if (first_space > 0 && first_space != std::string::npos) {
      const std::string fieldname = ls.substr(0, first_space);

      miutil::KeyValue_v other;
      PlotOptions po;
      // update with options from setup
      PlotOptions::parsePlotOption(getFieldOptions(fieldname, true), po, other);
      // update with options from logfile
      PlotOptions::parsePlotOption(miutil::splitKeyValue(ls.substr(first_space + 1)), po, other);

      miutil::unique_options(other);

      auto& fo = fieldOptions[fieldname];
      fo = po.toKeyValueList();
      fo << other;
    }
  }
}

void FieldDialogStyle::resetFieldOptions(SelectedField* selectedField)
{
  if (!selectedField)
    return;

  const miutil::KeyValue_v fopts = getFieldOptions(selectedField->fieldName, true);
  fieldOptions.erase(selectedField->fieldName);

  selectedField->setFieldPlotOptions(fopts);
  selectedField->hourOffset = 0;
  selectedField->hourDiff = 0;
  enableWidgets("none");

  enableFieldOptions(selectedField);
}

const miutil::KeyValue_v& FieldDialogStyle::getFieldOptions(const std::string& fieldName, bool ignoreUserOptions) const
{
  fieldoptions_m::const_iterator pfopt;

  if (!ignoreUserOptions) {
    // try private options used
    pfopt = fieldOptions.find(fieldName);
    if (pfopt != fieldOptions.end())
      return pfopt->second;
  }

  // following only searches for original options from the setup file

  pfopt = setupFieldOptions.find(fieldName);
  if (pfopt != setupFieldOptions.end())
    return pfopt->second;

  // default
  static const miutil::KeyValue_v fallback = PlotOptions().toKeyValueList();
  return fallback;
}
