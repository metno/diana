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

#include "qtFieldDialogStyle.h"

#include "diFieldUtil.h"
#include "diPlotOptions.h"
#include "qtFieldDialog.h" // for struct SelectedField
#include "qtUtility.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolTip>
#include <QVBoxLayout>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FieldDialogStyle"
#include <miLogger/miLogging.h>

using namespace std;

namespace { // anonymous

const size_t npos = size_t(-1);

const std::string REMOVE = "remove";
const std::string UNITS = "units";
const std::string UNIT = "unit";

vector<std::string> numberList(QComboBox* cBox, float number, bool onoff)
{
  const float enormal[] = {1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5., 5.5, 6., 6.5, 7., 7.5, 8., 8.5, 9., 9.5, -1};
  return diutil::numberList(cBox, number, enormal, onoff);
}

} // anonymous namespace

FieldDialogStyle::FieldDialogStyle(const fieldoptions_m& sfo, QWidget* parent)
    : widgetStd(new QWidget(parent))
    , widgetAdv(new QWidget(parent))
    , selectedField(0)
    , setupFieldOptions(sfo)
{
  METLIBS_LOG_SCOPE();

  // Colours
  csInfo = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();
  const map<std::string, unsigned int>& enabledOptions = PlotOptions::getEnabledOptions();
  const std::vector<std::vector<std::string>>& plottypes_dim = PlotOptions::getPlotTypes();
  if (plottypes_dim.size() > 1) {
    plottypes = plottypes_dim[1];
    for (size_t i = 0; i < plottypes_dim[0].size(); i++) {
      const std::string& ptd0i = plottypes_dim[0][i];
      const map<std::string, unsigned int>::const_iterator iptd0i = enabledOptions.find(ptd0i);
      if (iptd0i != enabledOptions.end()) {
        const unsigned int op = iptd0i->second;
        enableMap[ptd0i].contourWidgets = op & PlotOptions::POE_CONTOUR;
        enableMap[ptd0i].extremeWidgets = op & PlotOptions::POE_EXTREME;
        enableMap[ptd0i].shadingWidgets = op & PlotOptions::POE_SHADING;
        enableMap[ptd0i].lineWidgets = op & PlotOptions::POE_LINE;
        enableMap[ptd0i].fontWidgets = op & PlotOptions::POE_FONT;
        enableMap[ptd0i].densityWidgets = op & PlotOptions::POE_DENSITY;
        enableMap[ptd0i].unitWidgets = op & PlotOptions::POE_UNIT;
      }
    }
  }

  // linetypes
  linetypes = Linetype::getLinetypeNames();

  // density (of arrows etc, 0=automatic)
  QString qs;
  densityStringList << "Auto";
  for (int i = 1; i < 10; i++) {
    densityStringList << qs.setNum(i);
  }
  for (int i = 10; i < 30; i += 5) {
    densityStringList << qs.setNum(i);
  }
  for (int i = 30; i < 60; i += 10) {
    densityStringList << qs.setNum(i);
  }
  densityStringList << qs.setNum(100);
  densityStringList << "auto(0.5)";
  densityStringList << "auto(0.6)";
  densityStringList << "auto(0.7)";
  densityStringList << "auto(0.8)";
  densityStringList << "auto(0.9)";
  densityStringList << "auto(2)";
  densityStringList << "auto(3)";
  densityStringList << "auto(4)";
  densityStringList << "-1";

  CreateStandard();
  CreateAdvanced();
  toolTips();
  setDefaultFieldOptions();
}

FieldDialogStyle::~FieldDialogStyle() {}

void FieldDialogStyle::CreateStandard()
{
  QLabel* unitlabel = new QLabel(tr("Unit"), widgetStd);
  unitLineEdit = new QLineEdit(widgetStd);
  connect(unitLineEdit, &QLineEdit::editingFinished, this, &FieldDialogStyle::unitEditingFinished);

  // plottype
  QLabel* plottypelabel = new QLabel(tr("Plot type"), widgetStd);
  plottypeComboBox = ComboBox(widgetStd, plottypes);
  connect(plottypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::plottypeComboBoxActivated);

  // colorCbox
  QLabel* colorlabel = new QLabel(tr("Line colour"), widgetStd);
  colorCbox = ColourBox(widgetStd, false, 0, tr("off").toStdString(), true);
  colorCbox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect(colorCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::colorCboxActivated);

  // linewidthcbox
  QLabel* linewidthlabel = new QLabel(tr("Line width"), widgetStd);
  lineWidthCbox = LinewidthBox(widgetStd, false);
  connect(lineWidthCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::lineWidthCboxActivated);

  // linetypecbox
  QLabel* linetypelabel = new QLabel(tr("Line type"), widgetStd);
  lineTypeCbox = LinetypeBox(widgetStd, false);
  connect(lineTypeCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::lineTypeCboxActivated);

  // lineinterval
  QLabel* lineintervallabel = new QLabel(tr("Line interval"), widgetStd);
  lineintervalCbox = new QComboBox(widgetStd);
  connect(lineintervalCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::lineintervalCboxActivated);

  // density
  QLabel* densitylabel = new QLabel(tr("Density"), widgetStd);
  densityCbox = new QComboBox(widgetStd);
  densityCbox->addItems(densityStringList);
  connect(densityCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::densityCboxActivated);

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
  optlayout->addWidget(lineintervalCbox, 5, 1);
  optlayout->addWidget(densitylabel, 6, 0);
  optlayout->addWidget(densityCbox, 6, 1);
  optlayout->addWidget(vectorunitlabel, 7, 0);
  optlayout->addWidget(vectorunitCbox, 7, 1);
  widgetStd->setLayout(optlayout);
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
  connect(extremeTypeCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::extremeTypeActivated);

  QLabel* extremeSizeLabel = new QLabel(tr("Size"), widgetAdv);
  extremeSizeSpinBox = new QSpinBox(widgetAdv);
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  connect(extremeSizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::extremeSizeChanged);

  QLabel* extremeRadiusLabel = new QLabel(tr("Radius"), widgetAdv);
  extremeRadiusSpinBox = new QSpinBox(widgetAdv);
  extremeRadiusSpinBox->setMinimum(5);
  extremeRadiusSpinBox->setMaximum(300);
  extremeRadiusSpinBox->setSingleStep(5);
  extremeRadiusSpinBox->setWrapping(true);
  extremeRadiusSpinBox->setSuffix("%");
  extremeRadiusSpinBox->setValue(100);
  connect(extremeRadiusSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::extremeRadiusChanged);

  // line smoothing
  QLabel* lineSmoothLabel = new QLabel(tr("Smooth lines"), widgetAdv);
  lineSmoothSpinBox = new QSpinBox(widgetAdv);
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  connect(lineSmoothSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::lineSmoothChanged);

  // field smoothing
  QLabel* fieldSmoothLabel = new QLabel(tr("Smooth fields"), widgetAdv);
  fieldSmoothSpinBox = new QSpinBox(widgetAdv);
  fieldSmoothSpinBox->setMinimum(0);
  fieldSmoothSpinBox->setMaximum(20);
  fieldSmoothSpinBox->setSingleStep(1);
  fieldSmoothSpinBox->setSpecialValueText(tr("Off"));
  fieldSmoothSpinBox->setValue(0);
  connect(fieldSmoothSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::fieldSmoothChanged);

  labelSizeSpinBox = new QSpinBox(widgetAdv);
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(300);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  connect(labelSizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::labelSizeChanged);

  valuePrecisionBox = new QComboBox(widgetAdv);
  valuePrecisionBox->addItem("0");
  valuePrecisionBox->addItem("1");
  valuePrecisionBox->addItem("2");
  valuePrecisionBox->addItem("3");
  connect(valuePrecisionBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::valuePrecisionBoxActivated);

  // grid values
  gridValueCheckBox = new QCheckBox(QString(tr("Grid value")), widgetAdv);
  gridValueCheckBox->setChecked(false);
  connect(gridValueCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::gridValueCheckBoxToggled);

  // grid lines
  QLabel* gridLinesLabel = new QLabel(tr("Grid lines"), widgetAdv);
  gridLinesSpinBox = new QSpinBox(widgetAdv);
  gridLinesSpinBox->setMinimum(0);
  gridLinesSpinBox->setMaximum(50);
  gridLinesSpinBox->setSingleStep(1);
  gridLinesSpinBox->setSpecialValueText(tr("Off"));
  gridLinesSpinBox->setValue(0);
  connect(gridLinesSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::gridLinesChanged);

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
  connect(undefColourCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::undefColourActivated);

  // Undefined masking linewidth
  undefLinewidthCbox = LinewidthBox(widgetAdv, false);
  connect(undefLinewidthCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::undefLinewidthActivated);

  // Undefined masking linetype
  undefLinetypeCbox = LinetypeBox(widgetAdv, false);
  connect(undefLinetypeCbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::undefLinetypeActivated);

  // enable/disable numbers on isolines
  valueLabelCheckBox = new QCheckBox(QString(tr("Numbers")), widgetAdv);
  valueLabelCheckBox->setChecked(true);
  connect(valueLabelCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::valueLabelCheckBoxToggled);

  // Options
  QLabel* shadingLabel = new QLabel(tr("Palette"), widgetAdv);
  QLabel* shadingcoldLabel = new QLabel(tr("Palette (-)"), widgetAdv);
  QLabel* patternLabel = new QLabel(tr("Pattern"), widgetAdv);
  QLabel* alphaLabel = new QLabel(tr("Alpha"), widgetAdv);
  QLabel* headLabel = TitleLabel(tr("Extra contour lines"), widgetAdv);
  QLabel* colourLabel = new QLabel(tr("Line colour"), widgetAdv);
  QLabel* intervalLabel = new QLabel(tr("Line interval"), widgetAdv);
  QLabel* baseLabel = new QLabel(tr("Basis value"), widgetAdv);
  QLabel* minLabel = new QLabel(tr("Min"), widgetAdv);
  QLabel* maxLabel = new QLabel(tr("Max"), widgetAdv);
  QLabel* base2Label = new QLabel(tr("Basis value"), widgetAdv);
  QLabel* min2Label = new QLabel(tr("Min"), widgetAdv);
  QLabel* max2Label = new QLabel(tr("Max"), widgetAdv);
  QLabel* linewidthLabel = new QLabel(tr("Line width"), widgetAdv);
  QLabel* linetypeLabel = new QLabel(tr("Line type"), widgetAdv);
  QLabel* threeColourLabel = TitleLabel(tr("Three colours"), widgetAdv);

  tableCheckBox = new QCheckBox(tr("Table"), widgetAdv);
  connect(tableCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::tableCheckBoxToggled);

  repeatCheckBox = new QCheckBox(tr("Repeat"), widgetAdv);
  connect(repeatCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::repeatCheckBoxToggled);

  // 3 colours
  for (size_t i = 0; i < 3; i++) {
    threeColourBox.push_back(ColourBox(widgetAdv, true, 0, tr("Off").toStdString(), true));
    connect(threeColourBox[i], static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::threeColoursChanged);
  }

  // shading
  shadingComboBox = PaletteBox(widgetAdv, csInfo, false, 0, tr("Off").toStdString(), true);
  shadingComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect(shadingComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::shadingChanged);

  shadingSpinBox = new QSpinBox(widgetAdv);
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(255);
  shadingSpinBox->setSingleStep(1);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  connect(shadingSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::shadingChanged);

  shadingcoldComboBox = PaletteBox(widgetAdv, csInfo, false, 0, tr("Off").toStdString(), true);
  shadingcoldComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect(shadingcoldComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::shadingChanged);

  shadingcoldSpinBox = new QSpinBox(widgetAdv);
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(255);
  shadingcoldSpinBox->setSingleStep(1);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  connect(shadingcoldSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::shadingChanged);

  // pattern
  patternComboBox = PatternBox(widgetAdv, patternInfo, false, 0, tr("Off").toStdString(), true);
  patternComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect(patternComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::patternComboBoxToggled);

  // pattern colour
  patternColourBox = ColourBox(widgetAdv, false, 0, tr("Auto").toStdString(), true);
  connect(patternColourBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::patternColourBoxToggled);

  // alpha blending
  alphaSpinBox = new QSpinBox(widgetAdv);
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
  alphaSpinBox->setValue(255);
  connect(alphaSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &FieldDialogStyle::alphaChanged);

  // colour
  colour2ComboBox = ColourBox(widgetAdv, false, 0, tr("Off").toStdString(), true);
  connect(colour2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::colour2ComboBoxToggled);

  // line interval
  interval2ComboBox = new QComboBox(widgetAdv);
  connect(interval2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::interval2ComboBoxToggled);

  // zero value
  zero1ComboBox = new QComboBox(widgetAdv);
  zero2ComboBox = new QComboBox(widgetAdv);
  connect(zero1ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::zero1ComboBoxToggled);
  connect(zero2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::zero2ComboBoxToggled);

  // min
  min1ComboBox = new QComboBox(widgetAdv);
  min2ComboBox = new QComboBox(widgetAdv);

  // max
  max1ComboBox = new QComboBox(widgetAdv);
  max2ComboBox = new QComboBox(widgetAdv);

  // line values
  linevaluesField = new QLineEdit(widgetAdv);
  connect(linevaluesField, &QLineEdit::editingFinished, this, &FieldDialogStyle::linevaluesFieldEdited);
  // log line values
  linevaluesLogCheckBox = new QCheckBox(QString(tr("Log")), widgetAdv);
  linevaluesLogCheckBox->setChecked(false);
  connect(linevaluesLogCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::linevaluesLogCheckBoxToggled);

  connect(min1ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::min1ComboBoxToggled);
  connect(max1ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::max1ComboBoxToggled);
  connect(min2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::min2ComboBoxToggled);
  connect(max2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::max2ComboBoxToggled);

  // linewidth
  linewidth2ComboBox = LinewidthBox(widgetAdv);
  connect(linewidth2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::linewidth2ComboBoxToggled);
  // linetype
  linetype2ComboBox = LinetypeBox(widgetAdv, false);
  connect(linetype2ComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogStyle::linetype2ComboBoxToggled);

  // Plot frame
  frameCheckBox = new QCheckBox(QString(tr("Frame")), widgetAdv);
  frameCheckBox->setChecked(true);
  connect(frameCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::frameCheckBoxToggled);

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox = new QCheckBox(QString(tr("Zero line")), widgetAdv);
  //  zeroLineColourCBox= new QComboBox(advFrame);
  zeroLineCheckBox->setChecked(true);

  connect(zeroLineCheckBox, &QCheckBox::toggled, this, &FieldDialogStyle::zeroLineCheckBoxToggled);

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
  ;
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
  ;
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
  ;
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
  ;
  advLayout->addWidget(line6, line, 0, 1, 3);

  line++;
  advLayout->addWidget(baseLabel, line, 0);
  advLayout->addWidget(minLabel, line, 1);
  advLayout->addWidget(maxLabel, line, 2);
  line++;
  advLayout->addWidget(zero1ComboBox, line, 0);
  advLayout->addWidget(min1ComboBox, line, 1);
  advLayout->addWidget(max1ComboBox, line, 2);
  line++;
  advLayout->addWidget(new QLabel(tr("Values"), widgetAdv), line, 0);
  advLayout->addWidget(linevaluesField, line, 1);
  advLayout->addWidget(linevaluesLogCheckBox, line, 2);
  line++;
  advLayout->setRowStretch(line, 5);
  ;
  advLayout->addWidget(line3, line, 0, 1, 3);

  line++;
  advLayout->addWidget(headLabel, line, 0, 1, 2);
  line++;
  advLayout->addWidget(colourLabel, line, 0);
  advLayout->addWidget(colour2ComboBox, line, 1);
  line++;
  advLayout->addWidget(intervalLabel, line, 0);
  advLayout->addWidget(interval2ComboBox, line, 1);
  line++;
  advLayout->addWidget(linewidthLabel, line, 0);
  advLayout->addWidget(linewidth2ComboBox, line, 1);
  line++;
  advLayout->addWidget(linetypeLabel, line, 0);
  advLayout->addWidget(linetype2ComboBox, line, 1);
  line++;
  advLayout->addWidget(base2Label, line, 0);
  advLayout->addWidget(min2Label, line, 1);
  advLayout->addWidget(max2Label, line, 2);
  line++;
  advLayout->addWidget(zero2ComboBox, line, 0);
  advLayout->addWidget(min2ComboBox, line, 1);
  advLayout->addWidget(max2ComboBox, line, 2);

  line++;
  advLayout->setRowStretch(line, 5);
  ;
  advLayout->addWidget(line5, line, 0, 1, 3);
  line++;
  advLayout->addWidget(threeColourLabel, line, 0);
  //  advLayout->addWidget( threeColoursCheckBox, 38, 0 );
  line++;
  advLayout->addWidget(threeColourBox[0], line, 0);
  advLayout->addWidget(threeColourBox[1], line, 1);
  advLayout->addWidget(threeColourBox[2], line, 2);

  // a separator
  QFrame* advSep = new QFrame(widgetAdv);
  advSep->setFrameStyle(QFrame::VLine | QFrame::Raised);
  advSep->setLineWidth(5);

  QHBoxLayout* hLayout = new QHBoxLayout(widgetAdv);
  hLayout->setMargin(0);
  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);
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
  shadingSpinBox->setToolTip(tr("number of colours in the palette"));
  shadingcoldComboBox->setToolTip(tr("Palette for values below basis"));
  shadingcoldSpinBox->setToolTip(tr("number of colours in the palette"));
  patternColourBox->setToolTip(tr("Colour of pattern"));
}

void FieldDialogStyle::enableFieldOptions(SelectedField* sf)
{
  METLIBS_LOG_SCOPE();
  selectedField = sf;
  if (!selectedField) {
    enableWidgets("none");
    return;
  }

  setDefaultFieldOptions();

  size_t nc;
  int i, n;

  if (selectedField->fieldOpts == currentFieldOpts && selectedField->inEdit == currentFieldOptsInEdit && !selectedField->minus)
    return;

  currentFieldOpts = selectedField->fieldOpts;

  currentFieldOptsInEdit = selectedField->inEdit;

  // hourOffset
  if (currentFieldOptsInEdit) {
    hourOffsetSpinBox->setValue(0);
    hourOffsetSpinBox->setEnabled(false);
  } else {
    i = selectedField->hourOffset;
    hourOffsetSpinBox->setValue(i);
    hourOffsetSpinBox->setEnabled(true);
  }

  // hourDiff
  if (currentFieldOptsInEdit) {
    hourDiffSpinBox->setValue(0);
    hourDiffSpinBox->setEnabled(false);
  } else {
    i = selectedField->hourDiff;
    hourDiffSpinBox->setValue(i);
    hourDiffSpinBox->setEnabled(true);
  }

  if (selectedField->minus)
    return;

  vpcopt = selectedField->fieldOpts;

  int nr_linetypes = linetypes.size();
  enableWidgets("contour");

  // unit
  nc = miutil::find(vpcopt, UNITS);
  if (nc == npos)
    nc = miutil::find(vpcopt, UNIT);
  if (nc != npos) {
    updateFieldOptions(UNITS, vpcopt[nc].value());
    updateFieldOptions(UNIT, REMOVE);
    unitLineEdit->setText(QString::fromStdString(vpcopt[nc].value()));
  } else {
    updateFieldOptions(UNITS, REMOVE);
    updateFieldOptions(UNIT, REMOVE);
    unitLineEdit->clear();
  }

  // dimension (1dim = contour,..., 2dim=wind,...)
  const std::vector<std::vector<std::string>>& plottypes_dim = PlotOptions::getPlotTypes();
  size_t idx = 0;
  if ((nc = miutil::find(vpcopt, PlotOptions::key_dimension)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value())) {
      size_t vi = size_t(vpcopt[nc].toInt());
      if (vi < plottypes_dim.size())
        idx = vi;
    }
  }
  if (idx < plottypes_dim.size())
    plottypes = plottypes_dim[idx];
  plottypeComboBox->clear();
  for (const std::string& pt : plottypes)
    plottypeComboBox->addItem(QString::fromStdString(pt));

  // plottype
  if ((nc = miutil::find(vpcopt, PlotOptions::key_plottype)) != npos) {
    const std::string& value = vpcopt[nc].value();
    size_t i = 0;
    while (i < plottypes.size() && value != plottypes[i])
      i++;
    if (i == plottypes.size())
      i = 0;
    plottypeComboBox->setCurrentIndex(i);
    updateFieldOptions(PlotOptions::key_plottype, value);
    enableWidgets(plottypes[i]);
  } else {
    updateFieldOptions(PlotOptions::key_plottype, plottypes[0]);
    plottypeComboBox->setCurrentIndex(0);
    enableWidgets(plottypes[0]);
  }

  // colour(s)
  if ((nc = miutil::find(vpcopt, PlotOptions::key_colour_2)) != npos) {
    if (miutil::to_lower(vpcopt[nc].value()) == "off") {
      updateFieldOptions(PlotOptions::key_colour_2, "off");
      colour2ComboBox->setCurrentIndex(0);
    } else {
      SetCurrentItemColourBox(colour2ComboBox, vpcopt[nc].value());
      updateFieldOptions(PlotOptions::key_colour_2, vpcopt[nc].value());
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_colour)) != npos) {
    //  int nr_colours = colorCbox->count();
    if (miutil::to_lower(vpcopt[nc].value()) == "off") {
      updateFieldOptions(PlotOptions::key_colour, "off");
      colorCbox->setCurrentIndex(0);
    } else {
      SetCurrentItemColourBox(colorCbox, vpcopt[nc].value());
      updateFieldOptions(PlotOptions::key_colour, vpcopt[nc].value());
    }
  }

  // 3 colours
  if ((nc = miutil::find(vpcopt, PlotOptions::key_colours)) != npos) {
    vector<std::string> colours = miutil::split(vpcopt[nc].value(), ",");
    if (colours.size() == 3) {
      for (size_t j = 0; j < 3; j++) {
        SetCurrentItemColourBox(threeColourBox[j], colours[j]);
      }
      threeColoursChanged();
    }
  }

  // contour shading updating FieldOptions
  if ((nc = miutil::find(vpcopt, PlotOptions::key_palettecolours)) != npos) {
    if (miutil::to_lower(vpcopt[nc].value()) == "off") {
      updateFieldOptions(PlotOptions::key_palettecolours, "off");
      shadingComboBox->setCurrentIndex(0);
      shadingcoldComboBox->setCurrentIndex(0);
    } else {
      const vector<std::string> strValue = CommandParser::parseString(vpcopt[nc].value());
      vector<std::string> stokens = miutil::split(strValue[0], ";");

      size_t nr_cs = csInfo.size();
      std::string str;

      bool updateClodshading = false;
      vector<std::string> coldStokens;

      size_t i = 0, j = 0;
      while (i < nr_cs && stokens[0] != csInfo[i].name) {
        i++;
      }

      if (i == nr_cs) {
        ColourShading::defineColourShadingFromString(vpcopt[nc].value());
        ExpandPaletteBox(shadingComboBox, ColourShading(vpcopt[nc].value()));
        ExpandPaletteBox(shadingcoldComboBox, ColourShading(vpcopt[nc].value())); // MC
        ColourShading::ColourShadingInfo info;
        info.name = vpcopt[nc].value();
        info.colour = ColourShading::getColourShading(vpcopt[nc].value());
        csInfo.push_back(info);
      } else if (strValue.size() == 2) {
        coldStokens = miutil::split(strValue[1], ";");

        while (j < nr_cs && coldStokens[0] != csInfo[j].name) {
          j++;
        }

        if (j < nr_cs)
          updateClodshading = true;
      }

      str = vpcopt[nc].value(); // tokens[0];
      shadingComboBox->setCurrentIndex(i + 1);
      updateFieldOptions(PlotOptions::key_palettecolours, str);
      // Need to set this here otherwise the signal is changing
      // the vpcopt[nc].value() variable to off
      if (stokens.size() == 2)
        shadingSpinBox->setValue(miutil::to_int(stokens[1]));
      else
        shadingSpinBox->setValue(0);

      if (updateClodshading) {
        shadingcoldComboBox->setCurrentIndex(j + 1);

        if (coldStokens.size() == 2)
          shadingcoldSpinBox->setValue(miutil::to_int(coldStokens[1]));
        else
          shadingcoldSpinBox->setValue(0);
      }
    }
  }
  // pattern
  if ((nc = miutil::find(vpcopt, PlotOptions::key_patterns)) != npos) {
    const std::string& value = vpcopt[nc].value();
    size_t nr_p = patternInfo.size(), i = 0;
    std::string str;
    while (i < nr_p && value != patternInfo[i].name)
      i++;
    if (i == nr_p) {
      str = "off";
      patternComboBox->setCurrentIndex(0);
    } else {
      str = patternInfo[i].name;
      patternComboBox->setCurrentIndex(i + 1);
    }
    updateFieldOptions(PlotOptions::key_patterns, str);
  } else {
    patternComboBox->setCurrentIndex(0);
  }

  // pattern colour
  if ((nc = miutil::find(vpcopt, PlotOptions::key_pcolour)) != npos) {
    SetCurrentItemColourBox(patternColourBox, vpcopt[nc].value());
    updateFieldOptions(PlotOptions::key_pcolour, vpcopt[nc].value());
  }

  // table
  nc = miutil::find(vpcopt, PlotOptions::key_table);
  if (nc != npos) {
    bool on = vpcopt[nc].value() == "1";
    tableCheckBox->setChecked(on);
    tableCheckBoxToggled(on);
  }

  // repeat
  nc = miutil::find(vpcopt, PlotOptions::key_repeat);
  if (nc != npos) {
    bool on = vpcopt[nc].value() == "1";
    repeatCheckBox->setChecked(on);
    repeatCheckBoxToggled(on);
  }

  // alpha shading
  if ((nc = miutil::find(vpcopt, PlotOptions::key_alpha)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value()))
      i = vpcopt[nc].toInt();
    else
      i = 255;
    alphaSpinBox->setValue(i);
    alphaSpinBox->setEnabled(true);
  }

  // linetype
  if ((nc = miutil::find(vpcopt, PlotOptions::key_linetype)) != npos) {
    i = 0;
    while (i < nr_linetypes && vpcopt[nc].value() != linetypes[i])
      i++;
    if (i == nr_linetypes)
      i = 0;
    updateFieldOptions(PlotOptions::key_linetype, linetypes[i]);
    lineTypeCbox->setCurrentIndex(i);
    if ((nc = miutil::find(vpcopt, PlotOptions::key_linetype_2)) != npos) {
      i = 0;
      while (i < nr_linetypes && vpcopt[nc].value() != linetypes[i])
        i++;
      if (i == nr_linetypes)
        i = 0;
      updateFieldOptions(PlotOptions::key_linetype_2, linetypes[i]);
      linetype2ComboBox->setCurrentIndex(i);
    } else {
      linetype2ComboBox->setCurrentIndex(0);
    }
  }

  // linewidth
  if ((nc = miutil::find(vpcopt, PlotOptions::key_linewidth)) != npos && CommandParser::isInt(vpcopt[nc].value())) {
    i = vpcopt[nc].toInt();
    ;
    int nr_linewidths = lineWidthCbox->count();
    if (i > nr_linewidths) {
      ExpandLinewidthBox(lineWidthCbox, i);
    }
    updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(i));
    lineWidthCbox->setCurrentIndex(i - 1);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_linewidth_2)) != npos && CommandParser::isInt(vpcopt[nc].value())) {
    int nr_linewidths = linewidth2ComboBox->count();
    i = vpcopt[nc].toInt();
    ;
    if (i > nr_linewidths) {
      ExpandLinewidthBox(linewidth2ComboBox, i);
    }
    updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(i));
    linewidth2ComboBox->setCurrentIndex(i - 1);
  }

  // line interval (isoline contouring)
  {
    const size_t nci = miutil::find(vpcopt, PlotOptions::key_lineinterval), ncv = miutil::find(vpcopt, PlotOptions::key_linevalues),
                 nclv = miutil::find(vpcopt, PlotOptions::key_loglinevalues);
    if (nci != npos or ncv != npos or nclv != npos) {
      bool enable_values = true;
      if (nci != npos && CommandParser::isFloat(vpcopt[nci].value())) {
        float ekv = vpcopt[nci].toFloat();
        lineintervals = numberList(lineintervalCbox, ekv, true);
        lineintervals2 = numberList(interval2ComboBox, ekv, true);
        enable_values = false;
      } else {
        lineintervalCbox->setCurrentIndex(0);
      }
      if (ncv != npos) {
        linevaluesField->setText(QString::fromStdString(vpcopt[ncv].value()));
        linevaluesLogCheckBox->setChecked(false);
      } else if (nclv != npos) {
        linevaluesField->setText(QString::fromStdString(vpcopt[nclv].value()));
        linevaluesLogCheckBox->setChecked(true);
      }
      linevaluesField->setEnabled(enable_values);
      linevaluesLogCheckBox->setEnabled(enable_values);
    }
    if ((nc = miutil::find(vpcopt, PlotOptions::key_lineinterval_2)) != npos && (CommandParser::isFloat(vpcopt[nc].value()))) {
      float ekv = vpcopt[nc].toFloat();
      ;
      lineintervals2 = numberList(interval2ComboBox, ekv, true);
    }
  }

  // wind/vector density
  if ((nc = miutil::find(vpcopt, PlotOptions::key_density)) != npos) {
    std::string s;
    if (!vpcopt[nc].value().empty()) {
      s = vpcopt[nc].value();
    } else {
      s = "0";
      updateFieldOptions(PlotOptions::key_density, s);
    }
    if (s == "0") {
      i = 0;
    } else {
      const QString qs = QString::fromStdString(s);
      i = densityStringList.indexOf(qs);
      if (i == -1) {
        densityStringList << qs;
        densityCbox->addItem(qs);
        i = densityCbox->count() - 1;
      }
    }
    densityCbox->setCurrentIndex(i);
  }

  // vectorunit (vector length unit)
  if ((nc = miutil::find(vpcopt, PlotOptions::key_vectorunit)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value()))
      e = vpcopt[nc].toFloat();
    else
      e = 5;
    vectorunit = numberList(vectorunitCbox, e, false);
  }

  // extreme.type (L+H, C+W or none)
  if ((nc = miutil::find(vpcopt, PlotOptions::key_extremeType)) != npos) {
    i = 0;
    n = extremeType.size();
    while (i < n && vpcopt[nc].value() != extremeType[i]) {
      i++;
    }
    if (i == n) {
      i = 0;
    }
    updateFieldOptions(PlotOptions::key_extremeType, extremeType[i]);
    extremeTypeCbox->setCurrentIndex(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_extremeSize)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value())) {
      e = vpcopt[nc].toFloat();
      ;
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeSizeSpinBox->setValue(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_extremeRadius)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value())) {
      e = vpcopt[nc].toFloat();
      ;
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeRadiusSpinBox->setValue(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_lineSmooth)) != npos && CommandParser::isInt(vpcopt[nc].value())) {
    i = vpcopt[nc].toInt();
    ;
    lineSmoothSpinBox->setValue(i);
  } else {
    lineSmoothSpinBox->setValue(0);
  }

  if (currentFieldOptsInEdit) {
    fieldSmoothSpinBox->setValue(0);
    fieldSmoothSpinBox->setEnabled(false);
  } else if ((nc = miutil::find(vpcopt, PlotOptions::key_fieldSmooth)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value()))
      i = vpcopt[nc].toInt();
    else
      i = 0;
    fieldSmoothSpinBox->setValue(i);
  } else if (fieldSmoothSpinBox->isEnabled()) {
    fieldSmoothSpinBox->setValue(0);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_labelSize)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value()))
      e = vpcopt[nc].toFloat();
    else
      e = 1.0;
    i = (int(e * 100. + 0.5)) / 5 * 5;
    labelSizeSpinBox->setValue(i);
  } else if (labelSizeSpinBox->isEnabled()) {
    labelSizeSpinBox->setValue(100);
    labelSizeSpinBox->setEnabled(false);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_precision)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value()) && vpcopt[nc].toInt() < valuePrecisionBox->count()) {
      valuePrecisionBox->setCurrentIndex(vpcopt[nc].toInt());
    } else {
      valuePrecisionBox->setCurrentIndex(0);
    }
  }

  nc = miutil::find(vpcopt, PlotOptions::key_gridValue);
  if (nc != npos) {
    if (vpcopt[nc].value() == "-1") {
      nc = npos;
    } else {
      bool on = vpcopt[nc].value() == "1";
      gridValueCheckBox->setChecked(on);
      gridValueCheckBox->setEnabled(true);
    }
  }
  if (nc == npos && gridValueCheckBox->isEnabled()) {
    gridValueCheckBox->setChecked(false);
    gridValueCheckBox->setEnabled(false);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_gridLines)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value()))
      i = vpcopt[nc].toInt();
    else
      i = 0;
    gridLinesSpinBox->setValue(i);
  }

  // undefined masking
  int iumask = 0;
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefMasking)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value())) {
      iumask = vpcopt[nc].toInt();
      ;
      if (iumask < 0 || iumask >= int(undefMasking.size()))
        iumask = 0;
    } else {
      iumask = 0;
    }
    undefMaskingCbox->setCurrentIndex(iumask);
    undefMaskingActivated(iumask);
  }

  // undefined masking colour
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefColour)) != npos) {
    SetCurrentItemColourBox(undefColourCbox, vpcopt[nc].value());
    updateFieldOptions(PlotOptions::key_undefColour, vpcopt[nc].value());
  }

  // undefined masking linewidth
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefLinewidth)) != npos && CommandParser::isInt(vpcopt[nc].value())) {
    int nr_linewidths = undefLinewidthCbox->count();
    i = vpcopt[nc].toInt();
    ;
    if (i > nr_linewidths) {
      ExpandLinewidthBox(undefLinewidthCbox, i);
    }
    updateFieldOptions(PlotOptions::key_undefLinewidth, miutil::from_number(i));
    undefLinewidthCbox->setCurrentIndex(i - 1);
  }

  // undefined masking linetype
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefLinetype)) != npos) {
    i = 0;
    while (i < nr_linetypes && vpcopt[nc].value() != linetypes[i])
      i++;
    if (i == nr_linetypes) {
      i = 0;
      updateFieldOptions(PlotOptions::key_undefLinetype, linetypes[i]);
    }
    undefLinetypeCbox->setCurrentIndex(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_frame)) != npos) {
    frameCheckBox->setChecked(vpcopt[nc].value() != "0");
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_zeroLine)) != npos) {
    bool on = vpcopt[nc].value() == "1";
    zeroLineCheckBox->setChecked(on);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_valueLabel)) != npos) {
    bool on = vpcopt[nc].value() == "1";
    valueLabelCheckBox->setChecked(on);
  }

  // base
  if ((nc = miutil::find(vpcopt, PlotOptions::key_basevalue)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value())) {
      e = vpcopt[nc].toFloat();
      ;
      baseList(zero1ComboBox, e);
    }
    if ((nc = miutil::find(vpcopt, PlotOptions::key_basevalue_2)) != npos) {
      if (CommandParser::isFloat(vpcopt[nc].value())) {
        e = vpcopt[nc].toFloat();
        ;
        baseList(zero2ComboBox, e);
      }
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_minvalue)) != npos) {
    if (vpcopt[nc].value() != "off") {
      float value = vpcopt[nc].toFloat();
      baseList(min1ComboBox, value, true);
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_minvalue)) != npos) {
    if (vpcopt[nc].value() != "off") {
      float value = vpcopt[nc].toFloat();
      baseList(min2ComboBox, value, true);
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_maxvalue)) != npos) {
    if (vpcopt[nc].value() != "off") {
      float value = vpcopt[nc].toFloat();
      baseList(max1ComboBox, value, true);
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_maxvalue_2)) != npos) {
    if (vpcopt[nc].value() != "off") {
      float value = vpcopt[nc].toFloat();
      baseList(max2ComboBox, value, true);
    }
  }
}

void FieldDialogStyle::setDefaultFieldOptions()
{
  METLIBS_LOG_SCOPE();

  // show levels for the current field group
  //  setLevel();

  currentFieldOpts.clear();

  unitLineEdit->clear();
  plottypeComboBox->setCurrentIndex(0);
  colorCbox->setCurrentIndex(1);
  fieldSmoothSpinBox->setValue(0);
  gridValueCheckBox->setChecked(false);
  gridLinesSpinBox->setValue(0);
  undefMaskingCbox->setCurrentIndex(0);
  undefColourCbox->setCurrentIndex(1);
  undefLinewidthCbox->setCurrentIndex(0);
  undefLinetypeCbox->setCurrentIndex(0);
  frameCheckBox->setChecked(true);
  for (int i = 0; i < 3; i++) {
    threeColourBox[i]->setCurrentIndex(0);
  }

  lineTypeCbox->setCurrentIndex(0);
  lineSmoothSpinBox->setValue(0);
  zeroLineCheckBox->setChecked(true);
  colour2ComboBox->setCurrentIndex(0);
  interval2ComboBox->setCurrentIndex(0);
  linewidth2ComboBox->setCurrentIndex(0);
  linetype2ComboBox->setCurrentIndex(0);
  valueLabelCheckBox->setChecked(true);

  extremeTypeCbox->setCurrentIndex(0);
  extremeSizeSpinBox->setValue(100);
  extremeRadiusSpinBox->setValue(100);

  //  lineintervalCbox->setCurrentIndex(0);
  tableCheckBox->setChecked(false);
  repeatCheckBox->setChecked(false);
  shadingComboBox->setCurrentIndex(0);
  shadingcoldComboBox->setCurrentIndex(0);
  shadingSpinBox->setValue(0);
  shadingcoldSpinBox->setValue(0);
  patternComboBox->setCurrentIndex(0);
  patternColourBox->setCurrentIndex(0);
  alphaSpinBox->setValue(255);

  lineWidthCbox->setCurrentIndex(0);
  labelSizeSpinBox->setValue(0);
  valuePrecisionBox->setCurrentIndex(0);

  densityCbox->setCurrentIndex(0);

  vectorunitCbox->setCurrentIndex(0);

  lineintervals = numberList(lineintervalCbox, 10, true);
  lineintervals2 = numberList(interval2ComboBox, 10, true);
  lineintervalCbox->setCurrentIndex(0);
  interval2ComboBox->setCurrentIndex(0);
  baseList(zero1ComboBox, 0);
  baseList(zero2ComboBox, 0);
  baseList(min2ComboBox, 0, true);
  baseList(min1ComboBox, 0, true);
  baseList(max1ComboBox, 0, true);
  baseList(max2ComboBox, 0, true);
  min1ComboBox->setCurrentIndex(0);
  min2ComboBox->setCurrentIndex(0);
  max1ComboBox->setCurrentIndex(0);
  max2ComboBox->setCurrentIndex(0);

  // hour.offset and hour.diff are not plotOptions and signals must be blocked
  // in order not to change the selectedField values of hour.offset and hour.diff
  diutil::BlockSignals blockedO(hourOffsetSpinBox);
  hourOffsetSpinBox->setValue(0);
  diutil::BlockSignals blockedD(hourDiffSpinBox);
  hourDiffSpinBox->setValue(0);
}

void FieldDialogStyle::enableWidgets(const std::string& plottype)
{
  METLIBS_LOG_SCOPE("plottype=" << plottype);

  bool enable = (plottype != "none");

  // used for all plottypes
  unitLineEdit->setEnabled(enable);
  plottypeComboBox->setEnabled(enable);
  colorCbox->setEnabled(enable);
  fieldSmoothSpinBox->setEnabled(enable);
  gridValueCheckBox->setEnabled(enable);
  gridLinesSpinBox->setEnabled(enable);
  hourOffsetSpinBox->setEnabled(enable);
  hourDiffSpinBox->setEnabled(enable);
  undefMaskingCbox->setEnabled(enable);
  undefColourCbox->setEnabled(enable);
  undefLinewidthCbox->setEnabled(enable);
  undefLinetypeCbox->setEnabled(enable);
  frameCheckBox->setEnabled(enable);
  zero1ComboBox->setEnabled(enable);
  min1ComboBox->setEnabled(enable);
  max1ComboBox->setEnabled(enable);
  for (int i = 0; i < 3; i++) {
    threeColourBox[i]->setEnabled(enable);
  }

  const std::map<std::string, EnableWidget>::const_iterator itEM = enableMap.find(plottype);
  const bool knownPlotType = (itEM != enableMap.end());

  enable = knownPlotType && itEM->second.contourWidgets;
  lineTypeCbox->setEnabled(enable);
  lineSmoothSpinBox->setEnabled(enable);
  zeroLineCheckBox->setEnabled(enable);
  colour2ComboBox->setEnabled(enable);
  interval2ComboBox->setEnabled(enable);
  zero2ComboBox->setEnabled(enable);
  min2ComboBox->setEnabled(enable);
  max2ComboBox->setEnabled(enable);
  linewidth2ComboBox->setEnabled(enable);
  linetype2ComboBox->setEnabled(enable);
  valueLabelCheckBox->setEnabled(enable);

  enable = knownPlotType && itEM->second.extremeWidgets;
  extremeTypeCbox->setEnabled(enable);
  extremeSizeSpinBox->setEnabled(enable);
  extremeRadiusSpinBox->setEnabled(enable);

  enable = knownPlotType && itEM->second.shadingWidgets;
  lineintervalCbox->setEnabled(enable);
  tableCheckBox->setEnabled(enable);
  repeatCheckBox->setEnabled(enable);
  shadingComboBox->setEnabled(enable);
  shadingcoldComboBox->setEnabled(enable);
  shadingSpinBox->setEnabled(enable);
  shadingcoldSpinBox->setEnabled(enable);
  patternComboBox->setEnabled(enable);
  patternColourBox->setEnabled(enable);
  alphaSpinBox->setEnabled(enable);

  enable = enable && lineintervalCbox->currentIndex() == 0;
  linevaluesField->setEnabled(enable);
  linevaluesLogCheckBox->setEnabled(enable);

  enable = knownPlotType && itEM->second.lineWidgets;
  lineWidthCbox->setEnabled(enable);

  enable = knownPlotType && itEM->second.fontWidgets;
  labelSizeSpinBox->setEnabled(enable);
  valuePrecisionBox->setEnabled(enable);

  enable = knownPlotType && itEM->second.densityWidgets;
  densityCbox->setEnabled(enable);

  enable = knownPlotType && itEM->second.unitWidgets;
  vectorunitCbox->setEnabled(enable);
}

void FieldDialogStyle::baseList(QComboBox* cBox, float base, bool onoff)
{
  float ekv = 10;
  if (lineintervalCbox->currentIndex() > 0 && !lineintervalCbox->currentText().isNull()) {
    ekv = lineintervalCbox->currentText().toFloat();
  }

  int n;
  if (base < 0.)
    n = int(base / ekv - 0.5);
  else
    n = int(base / ekv + 0.5);
  if (fabsf(base - ekv * float(n)) > 0.01 * ekv) {
    base = ekv * float(n);
  }
  n = 21;
  int k = n / 2;
  int j = -k - 1;

  cBox->clear();

  if (onoff)
    cBox->addItem(tr("Off"));

  for (int i = 0; i < n; ++i) {
    j++;
    float e = base + ekv * float(j);
    if (fabs(e) < ekv / 2)
      cBox->addItem("0");
    else {
      cBox->addItem(QString::fromStdString(miutil::from_number(e)));
    }
  }

  if (onoff)
    cBox->setCurrentIndex(k + 1);
  else
    cBox->setCurrentIndex(k);
}

void FieldDialogStyle::unitEditingFinished()
{
  updateFieldOptions(UNIT, REMOVE);
  updateFieldOptions(UNITS, unitLineEdit->text().toStdString());
}

void FieldDialogStyle::plottypeComboBoxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_plottype, plottypes[index]);
  enableWidgets(plottypes[index]);
}

void FieldDialogStyle::colorCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_colour, "off");
  else
    updateFieldOptions(PlotOptions::key_colour, colorCbox->currentText().toStdString());
}

void FieldDialogStyle::lineWidthCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(index + 1));
}

void FieldDialogStyle::lineTypeCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_linetype, linetypes[index]);
}

void FieldDialogStyle::lineintervalCboxActivated(int index)
{
  const bool interval_off = (index == 0);
  linevaluesField->setEnabled(interval_off);
  linevaluesLogCheckBox->setEnabled(interval_off);
  if (interval_off) {
    updateFieldOptions(PlotOptions::key_lineinterval, REMOVE);
    linevaluesFieldEdited();
  } else {
    updateFieldOptions(PlotOptions::key_lineinterval, lineintervals[index]);
    // update the list (with selected value in the middle)
    float a = miutil::to_float(lineintervals[index]);
    lineintervals = numberList(lineintervalCbox, a, true);
    updateFieldOptions(PlotOptions::key_linevalues, REMOVE);
    updateFieldOptions(PlotOptions::key_loglinevalues, REMOVE);
  }
}

void FieldDialogStyle::densityCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_density, "0");
  else
    updateFieldOptions(PlotOptions::key_density, densityCbox->currentText().toStdString());
}

void FieldDialogStyle::vectorunitCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_vectorunit, vectorunit[index]);
  // update the list (with selected value in the middle)
  float a = miutil::to_float(vectorunit[index]);
  vectorunit = numberList(vectorunitCbox, a, false);
}

void FieldDialogStyle::extremeTypeActivated(int index)
{
  updateFieldOptions(PlotOptions::key_extremeType, extremeType[index]);
}

void FieldDialogStyle::extremeSizeChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_extremeSize, str);
}

void FieldDialogStyle::extremeRadiusChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_extremeRadius, str);
}

void FieldDialogStyle::lineSmoothChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_lineSmooth, str);
}

void FieldDialogStyle::fieldSmoothChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_fieldSmooth, str);
}

void FieldDialogStyle::labelSizeChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_labelSize, str);
}

void FieldDialogStyle::valuePrecisionBoxActivated(int index)
{
  std::string str = miutil::from_number(index);
  updateFieldOptions(PlotOptions::key_precision, str);
}

void FieldDialogStyle::gridValueCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_gridValue, "1");
  else
    updateFieldOptions(PlotOptions::key_gridValue, "0");
}

void FieldDialogStyle::gridLinesChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_gridLines, str);
}

void FieldDialogStyle::hourOffsetChanged(int value)
{
  selectedField->hourOffset = value;
  /*Q_EMIT*/ updateTime();
}

void FieldDialogStyle::hourDiffChanged(int value)
{
  selectedField->hourDiff = value;
  /*Q_EMIT*/ updateTime();
}

void FieldDialogStyle::undefMaskingActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefMasking, miutil::from_number(index));
  undefColourCbox->setEnabled(index > 0);
  undefLinewidthCbox->setEnabled(index > 1);
  undefLinetypeCbox->setEnabled(index > 1);
}

void FieldDialogStyle::undefColourActivated(int /*index*/)
{
  updateFieldOptions(PlotOptions::key_undefColour, undefColourCbox->currentText().toStdString());
}

void FieldDialogStyle::undefLinewidthActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefLinewidth, miutil::from_number(index + 1));
}

void FieldDialogStyle::undefLinetypeActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefLinetype, linetypes[index]);
}

void FieldDialogStyle::frameCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_frame, "1");
  else
    updateFieldOptions(PlotOptions::key_frame, "0");
}

void FieldDialogStyle::zeroLineCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_zeroLine, "1");
  else
    updateFieldOptions(PlotOptions::key_zeroLine, "0");
}

void FieldDialogStyle::valueLabelCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_valueLabel, "1");
  else
    updateFieldOptions(PlotOptions::key_valueLabel, "0");
}

void FieldDialogStyle::colour2ComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_colour_2, "off");
    enableType2Options(false);
    colour2ComboBox->setEnabled(true);
  } else {
    updateFieldOptions(PlotOptions::key_colour_2, colour2ComboBox->currentText().toStdString());
    enableType2Options(true); // check if needed
    // turn of 3 colours (not possible to combine threeCols and col_2)
    for (int i = 0; i < 3; ++i)
      threeColourBox[i]->setCurrentIndex(0);
    threeColoursChanged();
  }
}

void FieldDialogStyle::tableCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_table, "1");
  else
    updateFieldOptions(PlotOptions::key_table, "0");
}

void FieldDialogStyle::patternComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_patterns, "off");
  } else {
    updateFieldOptions(PlotOptions::key_patterns, patternInfo[index - 1].name);
  }
  updatePaletteString();
}

void FieldDialogStyle::patternColourBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_pcolour, REMOVE);
  } else {
    updateFieldOptions(PlotOptions::key_pcolour, patternColourBox->currentText().toStdString());
  }
  updatePaletteString();
}

void FieldDialogStyle::repeatCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_repeat, "1");
  else
    updateFieldOptions(PlotOptions::key_repeat, "0");
}

void FieldDialogStyle::threeColoursChanged()
{
  if (threeColourBox[0]->currentIndex() == 0 || threeColourBox[1]->currentIndex() == 0 || threeColourBox[2]->currentIndex() == 0) {

    updateFieldOptions(PlotOptions::key_colours, REMOVE);

  } else {

    // turn of colour_2 (not possible to combine threeCols and col_2)
    colour2ComboBox->setCurrentIndex(0);
    colour2ComboBoxToggled(0);

    std::string str = threeColourBox[0]->currentText().toStdString() + "," + threeColourBox[1]->currentText().toStdString() + "," +
                      threeColourBox[2]->currentText().toStdString();

    updateFieldOptions(PlotOptions::key_colours, REMOVE);
    updateFieldOptions(PlotOptions::key_colours, str);
  }
}

void FieldDialogStyle::shadingChanged()
{
  updatePaletteString();
}

void FieldDialogStyle::updatePaletteString()
{
  if (patternComboBox->currentIndex() > 0 && patternColourBox->currentIndex() > 0) {
    updateFieldOptions(PlotOptions::key_palettecolours, "off");
    return;
  }

  int index1 = shadingComboBox->currentIndex();
  int index2 = shadingcoldComboBox->currentIndex();
  int value1 = shadingSpinBox->value();
  int value2 = shadingcoldSpinBox->value();

  if (index1 == 0 && index2 == 0) {
    updateFieldOptions(PlotOptions::key_palettecolours, "off");
    return;
  }

  std::string str;
  if (index1 > 0) {
    str = csInfo[index1 - 1].name;
    if (value1 > 0)
      str += ";" + miutil::from_number(value1);
    if (index2 > 0)
      str += ",";
  }
  if (index2 > 0) {
    str += csInfo[index2 - 1].name;
    if (value2 > 0)
      str += ";" + miutil::from_number(value2);
  }
  updateFieldOptions(PlotOptions::key_palettecolours, str);
}

void FieldDialogStyle::alphaChanged(int index)
{
  updateFieldOptions(PlotOptions::key_alpha, miutil::from_number(index));
}

void FieldDialogStyle::interval2ComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_lineinterval_2, REMOVE);
  } else {
    updateFieldOptions(PlotOptions::key_lineinterval_2, lineintervals2[index]);
    // update the list (with selected value in the middle)
    float a = miutil::to_float(lineintervals2[index]);
    lineintervals2 = numberList(interval2ComboBox, a, true);
  }
}

void FieldDialogStyle::zero1ComboBoxToggled(int)
{
  if (!zero1ComboBox->currentText().isNull()) {
    baseList(zero1ComboBox, zero1ComboBox->currentText().toFloat());
    updateFieldOptions(PlotOptions::key_basevalue, zero1ComboBox->currentText().toStdString());
  }
}

void FieldDialogStyle::zero2ComboBoxToggled(int)
{
  if (!zero2ComboBox->currentText().isNull()) {
    const float a = zero2ComboBox->currentText().toFloat();
    updateFieldOptions(PlotOptions::key_basevalue_2, zero2ComboBox->currentText().toStdString());
    baseList(zero2ComboBox, a);
  }
}

void FieldDialogStyle::min1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_minvalue, "off");
  else if (!min1ComboBox->currentText().isNull()) {
    baseList(min1ComboBox, min1ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_minvalue, min1ComboBox->currentText().toStdString());
  }
}

void FieldDialogStyle::max1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_maxvalue, "off");
  else if (!max1ComboBox->currentText().isNull()) {
    baseList(max1ComboBox, max1ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_maxvalue, max1ComboBox->currentText().toStdString());
  }
}

void FieldDialogStyle::min2ComboBoxToggled(int index)
{

  if (index == 0)
    updateFieldOptions(PlotOptions::key_minvalue_2, REMOVE);
  else if (!min2ComboBox->currentText().isNull()) {
    baseList(min2ComboBox, min2ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_minvalue_2, min2ComboBox->currentText().toStdString());
  }
}

void FieldDialogStyle::max2ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_maxvalue_2, REMOVE);
  else if (!max2ComboBox->currentText().isNull()) {
    baseList(max2ComboBox, max2ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_maxvalue_2, max2ComboBox->currentText().toStdString());
  }
}

void FieldDialogStyle::linevaluesFieldEdited()
{
  const std::string line_values = linevaluesField->text().toStdString();
  if (linevaluesLogCheckBox->isChecked()) {
    updateFieldOptions(PlotOptions::key_linevalues, REMOVE);
    updateFieldOptions(PlotOptions::key_loglinevalues, line_values);
  } else {
    updateFieldOptions(PlotOptions::key_loglinevalues, REMOVE);
    updateFieldOptions(PlotOptions::key_linevalues, line_values);
  }
}

void FieldDialogStyle::linevaluesLogCheckBoxToggled(bool)
{
  linevaluesFieldEdited();
}

void FieldDialogStyle::linewidth1ComboBoxToggled(int index)
{
  lineWidthCbox->setCurrentIndex(index);
  updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(index + 1));
}

void FieldDialogStyle::linewidth2ComboBoxToggled(int index)
{
  updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(index + 1));
}

void FieldDialogStyle::linetype1ComboBoxToggled(int index)
{
  lineTypeCbox->setCurrentIndex(index);
  updateFieldOptions(PlotOptions::key_linetype, linetypes[index]);
}

void FieldDialogStyle::linetype2ComboBoxToggled(int index)
{
  updateFieldOptions(PlotOptions::key_linetype_2, linetypes[index]);
}

void FieldDialogStyle::enableType2Options(bool on)
{
  colour2ComboBox->setEnabled(on);

  // enable the rest only if colour2 is on
  on = (colour2ComboBox->currentIndex() != 0);

  interval2ComboBox->setEnabled(on);
  zero2ComboBox->setEnabled(on);
  min2ComboBox->setEnabled(on);
  max2ComboBox->setEnabled(on);
  linewidth2ComboBox->setEnabled(on);
  linetype2ComboBox->setEnabled(on);

  if (on) {
    if (!interval2ComboBox->currentText().isNull())
      updateFieldOptions(PlotOptions::key_lineinterval_2, interval2ComboBox->currentText().toStdString());
    if (!zero2ComboBox->currentText().isNull())
      updateFieldOptions(PlotOptions::key_basevalue_2, zero2ComboBox->currentText().toStdString());
    if (!min2ComboBox->currentText().isNull() && min2ComboBox->currentIndex() > 0)
      updateFieldOptions(PlotOptions::key_minvalue_2, min2ComboBox->currentText().toStdString());
    if (!max2ComboBox->currentText().isNull() && max2ComboBox->currentIndex() > 0)
      updateFieldOptions(PlotOptions::key_maxvalue_2, max2ComboBox->currentText().toStdString());
    updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(linewidth2ComboBox->currentIndex() + 1));
    updateFieldOptions(PlotOptions::key_linetype_2, linetypes[linetype2ComboBox->currentIndex()]);
  } else {
    colour2ComboBox->setCurrentIndex(0);
    updateFieldOptions(PlotOptions::key_colour_2, "off");
    updateFieldOptions(PlotOptions::key_lineinterval_2, REMOVE);
    updateFieldOptions(PlotOptions::key_basevalue_2, REMOVE);
    updateFieldOptions(PlotOptions::key_linewidth_2, REMOVE);
    updateFieldOptions(PlotOptions::key_linetype_2, REMOVE);
  }
}

void FieldDialogStyle::updateFieldOptions(const std::string& name, const std::string& value)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(value));

  if (currentFieldOpts.empty())
    return;

  const size_t pos = miutil::find(vpcopt, name);
  if (value == REMOVE) {
    if (pos != npos)
      vpcopt.erase(vpcopt.begin() + pos);
  } else {
    const miutil::KeyValue kv(name, value);
    if (pos != npos)
      vpcopt[pos] = kv;
    else
      vpcopt.push_back(kv);
  }

  currentFieldOpts = vpcopt;
  selectedField->fieldOpts = currentFieldOpts;

  // not update private settings if external/QuickMenu command...
  if (!selectedField->external) {
    fieldOptions[selectedField->fieldName] = currentFieldOpts;
  }
}

inline std::string sub(const std::string& s, std::string::size_type begin, std::string::size_type end)
{
  return s.substr(begin, end - begin);
}

vector<std::string> FieldDialogStyle::writeLog()
{
  vector<std::string> vstr;

  // write used field options

  map<std::string, miutil::KeyValue_v>::iterator pfopt, pfend = fieldOptions.end();

  for (pfopt = fieldOptions.begin(); pfopt != pfend; pfopt++) {
    miutil::KeyValue_v sopts = getFieldOptions(pfopt->first, true);
    // only logging options if different from setup
    if (sopts != pfopt->second)
      vstr.push_back(pfopt->first + " " + miutil::mergeKeyValue(pfopt->second));
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

      // get options from setup
      miutil::KeyValue_v setup_opts = getFieldOptions(fieldname, true);
      if (setup_opts.empty())
        continue;

      // update options from setup, if necessary
      const miutil::KeyValue_v log_opts = miutil::splitKeyValue(ls.substr(first_space + 1));

      // FIXME this almost the same as mergeFieldOptions in diFieldUtil.cc
      const size_t n_setup_opts = setup_opts.size();
      const size_t n_log_opts = log_opts.size();
      bool changed = false;
      for (size_t i = 0; i < n_setup_opts; i++) {
        size_t j = 0;
        while (j < n_log_opts && log_opts[j].key() != setup_opts[i].key())
          j++;
        if (j < n_log_opts) {
          if (log_opts[j].value() != setup_opts[i].value()) {
            setup_opts[i] = log_opts[j];
            changed = true;
          }
        }
      }
      for (size_t i = 0; i < n_log_opts; i++) {
        size_t j = 0;
        while (j < n_setup_opts && setup_opts[j].key() != log_opts[i].key())
          j++;
        if (j == n_setup_opts) {
          setup_opts.push_back(log_opts[i]);
          changed = true;
        }
      }
      if (changed) {
        cleanupFieldOptions(setup_opts);
        fieldOptions[fieldname] = setup_opts;
      }
    }
  }
}

void FieldDialogStyle::resetOptions()
{
  if (!selectedField)
    return;

  const miutil::KeyValue_v fopts = getFieldOptions(selectedField->fieldName, true);
  if (fopts.empty())
    return;

  selectedField->fieldOpts = fopts;
  selectedField->hourOffset = 0;
  selectedField->hourDiff = 0;
  enableWidgets("none");
  currentFieldOpts.clear();
  enableFieldOptions(selectedField);
}

const miutil::KeyValue_v& FieldDialogStyle::getFieldOptions(const std::string& fieldName, bool reset) const
{
  fieldoptions_m::const_iterator pfopt;

  if (!reset) {
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
