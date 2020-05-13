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

#include "qtFieldDialog.h"

#include "diCommandParser.h"
#include "diFieldUtil.h"
#include "diPlotOptions.h"

#include "qtToggleButton.h"
#include "qtTreeFilterProxyModel.h"
#include "qtUtility.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>

#include <diField/diRectangle.h>
#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldDialog"
#include <miLogger/miLogging.h>

#include "down12x12.xpm"
#include "felt.xpm"
#include "up12x12.xpm"

//#define DEBUGPRINT

using namespace std;

namespace { // anonymous

const int ROLE_MODELGROUP = Qt::UserRole + 1;

const std::string REMOVE = "remove";
const std::string UNITS = "units";
const std::string UNIT = "unit";

const size_t npos = size_t(-1);

const char* const modelGlobalAttributes[][2] = {
  { "title",   QT_TRANSLATE_NOOP("FieldDialog", "Title") },
  { "summary", QT_TRANSLATE_NOOP("FieldDialog", "Summary") },
};


} // anonymous namespace

// ========================================================================

FieldDialog::FieldDialog(QWidget* parent, FieldDialogData* data)
    : DataDialog(parent, 0)
    , m_data(data)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Fields"));
  m_action = new QAction(QIcon(QPixmap(felt_xpm)), windowTitle(), this);
  m_action->setCheckable(true);
  m_action->setShortcut(Qt::ALT + Qt::Key_F);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_fielddialogue.html";

  useArchive = false;

  numEditFields = 0;
  currentFieldOptsInEdit = false;

  editName = tr("EDIT").toStdString();

  m_data->getSetupFieldOptions(setupFieldOptions);

  // Colours
  csInfo = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();
  const map<std::string, unsigned int>& enabledOptions = PlotOptions::getEnabledOptions();
  const std::vector< std::vector<std::string> >& plottypes_dim = PlotOptions::getPlotTypes();
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
  densityStringList <<  "auto(0.5)";
  densityStringList <<  "auto(0.6)";
  densityStringList <<  "auto(0.7)";
  densityStringList <<  "auto(0.8)";
  densityStringList <<  "auto(0.9)";
  densityStringList <<  "auto(2)";
  densityStringList <<  "auto(3)";
  densityStringList <<  "auto(4)";
  densityStringList <<  "-1";

  //----------------------------------------------------------------

  //modelbox
  QLabel *modellabel = TitleLabel(tr("Models"), this);
  modelbox = new QTreeView(this);
  modelbox->setHeaderHidden(true);
  modelbox->setSelectionMode(QAbstractItemView::SingleSelection);
  modelbox->setSelectionBehavior(QAbstractItemView::SelectRows);
  modelItems = new QStandardItemModel(this);
  modelFilter = new TreeFilterProxyModel(this);
  modelFilter->setDynamicSortFilter(true);
  modelFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  modelFilter->setSourceModel(modelItems);
  modelbox->setModel(modelFilter);
  connect(modelbox, SIGNAL(clicked(const QModelIndex&)),
      SLOT(modelboxClicked(const QModelIndex&)));
  modelFilterEdit = new QLineEdit("", this);
  modelFilterEdit->setPlaceholderText(tr("Type to filter model names"));
  connect(modelFilterEdit, SIGNAL(textChanged(const QString&)),
      SLOT(filterModels(const QString&)));

  // refTime
  QLabel *refTimelabel = TitleLabel(tr("Reference time"), this);
  refTimeComboBox = new QComboBox(this);
  connect( refTimeComboBox, SIGNAL( activated( int ) ),
      SLOT( updateFieldGroups(  ) ) );

  // fieldGRbox
  QLabel *fieldGRlabel = TitleLabel(tr("Field group"), this);
  fieldGRbox = new QComboBox(this);
  connect(fieldGRbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialog::fieldGRboxActivated);

  //fieldGroupCheckBox
  predefinedPlotsCheckBox = new QCheckBox(tr("Predefined plots"));
  predefinedPlotsCheckBox->setChecked(true);
  connect(predefinedPlotsCheckBox, SIGNAL(toggled(bool)), SLOT(updateFieldGroups()));

  // fieldbox
  QLabel *fieldlabel = TitleLabel(tr("Fields"), this);
  fieldbox = new QListWidget(this);
  fieldbox->setSelectionMode(QAbstractItemView::MultiSelection);
  connect( fieldbox, SIGNAL( itemClicked(QListWidgetItem*) ),
      SLOT( fieldboxChanged(QListWidgetItem*) ) );

  // selectedFieldbox
  QLabel *selectedFieldlabel = TitleLabel(tr("Selected fields"), this);
  selectedFieldbox = new QListWidget(this);
  selectedFieldbox->setSelectionMode(QAbstractItemView::SingleSelection);

  connect( selectedFieldbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
      SLOT( selectedFieldboxClicked( QListWidgetItem * ) ) );

  // Level: slider & label for the value
  levelLabel = new QLabel("1000hPa", this);
  levelLabel->setMinimumSize(levelLabel->sizeHint().width() + 10,
      levelLabel->sizeHint().height() + 10);
  levelLabel->setMaximumSize(levelLabel->sizeHint().width() + 10,
      levelLabel->sizeHint().height() + 10);
  levelLabel->setText(" ");

  levelLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  levelLabel->setLineWidth(2);
  levelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  levelSlider = new QSlider(Qt::Vertical, this);
  levelSlider->setInvertedAppearance(true);
  levelSlider->setMinimum(0);
  levelSlider->setMaximum(1);
  levelSlider->setPageStep(1);
  levelSlider->setValue(0);

  connect( levelSlider, SIGNAL( valueChanged( int )),
      SLOT( levelChanged( int)));
  connect(levelSlider, SIGNAL(sliderPressed()), SLOT(levelPressed()));
  connect(levelSlider, SIGNAL(sliderReleased()), SLOT(updateLevel()));

  levelInMotion = false;

  // sliderlabel
  QLabel *levelsliderlabel = new QLabel(tr("Vertical axis"), this);

  // Idnum: slider & label for the value
  idnumLabel = new QLabel("EPS.Total", this);
  idnumLabel->setMinimumSize(idnumLabel->sizeHint().width() + 10,
      idnumLabel->sizeHint().height() + 10);
  idnumLabel->setMaximumSize(idnumLabel->sizeHint().width() + 10,
      idnumLabel->sizeHint().height() + 10);
  idnumLabel->setText(" ");

  idnumLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  idnumLabel->setLineWidth(2);
  idnumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  idnumSlider = new QSlider(Qt::Vertical, this);
  idnumSlider->setMinimum(0);
  idnumSlider->setMaximum(1);
  idnumSlider->setPageStep(1);
  idnumSlider->setValue(0);

  connect( idnumSlider, SIGNAL( valueChanged( int )),
      SLOT( idnumChanged( int)));
  connect(idnumSlider, SIGNAL(sliderPressed()), SLOT(idnumPressed()));
  connect(idnumSlider, SIGNAL(sliderReleased()), SLOT(updateIdnum()));

  idnumInMotion = false;

  // sliderlabel
  QLabel *idnumsliderlabel = new QLabel(tr("Extra axis"), this);

  QLabel* unitlabel = new QLabel(tr("Unit"), this);
  unitLineEdit = new QLineEdit(this);
  connect(unitLineEdit, &QLineEdit::editingFinished, this, &FieldDialog::unitEditingFinished);

  // copyField
  copyField = new QPushButton(tr("Copy"), this);
  connect(copyField, SIGNAL(clicked()), SLOT(copySelectedField()));

  // deleteSelected
  deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, SIGNAL(clicked()), SLOT(deleteSelected()));

  // deleteAll
  deleteAll = new QPushButton(tr("Delete all"), this);
  connect(deleteAll, SIGNAL(clicked()), SLOT(deleteAllSelected()));

  // changeModelButton
  changeModelButton = new QPushButton(tr("Model"), this);
  connect(changeModelButton, SIGNAL(clicked()), SLOT(changeModel()));

  int width = changeModelButton->sizeHint().width()/3;
  int height = changeModelButton->sizeHint().height();;

  // upField
  upFieldButton = new QPushButton(QPixmap(up12x12_xpm), "", this);
  upFieldButton->setMaximumSize(width,height);
  connect(upFieldButton, SIGNAL(clicked()), SLOT(upField()));

  // downField
  downFieldButton = new QPushButton(QPixmap(down12x12_xpm), "", this);
  downFieldButton->setMaximumSize(width,height);
  connect(downFieldButton, SIGNAL(clicked()), SLOT(downField()));

  // resetOptions
  resetOptionsButton = new QPushButton(tr("Default"), this);
  connect(resetOptionsButton, SIGNAL(clicked()), SLOT(resetOptions()));

  // minus
  minusButton = new ToggleButton(this, tr("Minus"));
  connect( minusButton, SIGNAL(toggled(bool)), SLOT(minusField(bool)));

  // plottype
  QLabel* plottypelabel = new QLabel(tr("Plot type"), this);
  plottypeComboBox = ComboBox(this, plottypes);
  connect( plottypeComboBox, SIGNAL( activated(int) ),
      SLOT( plottypeComboBoxActivated(int) ) );


  // colorCbox
  QLabel* colorlabel = new QLabel(tr("Line colour"), this);
  colorCbox = ColourBox(this, false, 0, tr("off").toStdString(),true);
  colorCbox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect( colorCbox, SIGNAL( activated(int) ),
      SLOT( colorCboxActivated(int) ) );

  // linewidthcbox
  QLabel* linewidthlabel = new QLabel(tr("Line width"), this);
  lineWidthCbox = LinewidthBox(this, false);
  connect( lineWidthCbox, SIGNAL( activated(int) ),
      SLOT( lineWidthCboxActivated(int) ) );

  // linetypecbox
  QLabel* linetypelabel = new QLabel(tr("Line type"), this);
  lineTypeCbox = LinetypeBox(this, false);
  connect( lineTypeCbox, SIGNAL( activated(int) ),
      SLOT( lineTypeCboxActivated(int) ) );

  // lineinterval
  QLabel* lineintervallabel = new QLabel(tr("Line interval"), this);
  lineintervalCbox = new QComboBox(this);
  connect( lineintervalCbox, SIGNAL( activated(int) ),
      SLOT( lineintervalCboxActivated(int) ) );

  // density
  QLabel* densitylabel = new QLabel(tr("Density"), this);
  densityCbox = new QComboBox(this);
  densityCbox->addItems(densityStringList);
  connect( densityCbox, SIGNAL( activated(int) ),
      SLOT( densityCboxActivated(int) ) );

  // vectorunit
  QLabel* vectorunitlabel = new QLabel(tr("Unit"), this);
  vectorunitCbox = new QComboBox(this);
  connect( vectorunitCbox, SIGNAL( activated(int) ),
      SLOT( vectorunitCboxActivated(int) ) );

  // allTimeStep
  allTimeStepButton = new ToggleButton(this, tr("All time steps"));
  allTimeStepButton->setCheckable(true);
  allTimeStepButton->setChecked(true);
  connect(allTimeStepButton, &QPushButton::toggled, this, &FieldDialog::allTimeStepToggled);

  // advanced
  advanced = new ToggleButton(this, tr("<<Less"), tr("More>>"));
  advanced->setChecked(false);
  connect(advanced, &QAbstractButton::toggled, this, &DataDialog::showMore);

  // layout
  QHBoxLayout* grouplayout = new QHBoxLayout();
  grouplayout->addWidget(fieldGRbox);
  grouplayout->addWidget(predefinedPlotsCheckBox);

  QHBoxLayout* modellayout = new QHBoxLayout();
  modellayout->addWidget(modellabel);
  modellayout->addWidget(modelFilterEdit);

  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->setSpacing(1);
  v1layout->addLayout(modellayout);
  v1layout->addWidget(modelbox, 2);
  v1layout->addWidget(refTimelabel);
  v1layout->addWidget(refTimeComboBox);
  v1layout->addWidget(fieldGRlabel);
  v1layout->addLayout(grouplayout);
  v1layout->addWidget(fieldlabel);
  v1layout->addWidget(fieldbox, 4);
  v1layout->addWidget(selectedFieldlabel);
  v1layout->addWidget(selectedFieldbox, 2);

  //  QVBoxLayout* h2layout = new QVBoxLayout();
  //  h2layout->addStretch(1);

  QHBoxLayout* v1h4layout = new QHBoxLayout();
  v1h4layout->addWidget(upFieldButton);
  v1h4layout->addWidget(deleteButton);
  v1h4layout->addWidget(minusButton);
  v1h4layout->addWidget(copyField);

  QHBoxLayout* vxh4layout = new QHBoxLayout();
  vxh4layout->addWidget(downFieldButton);
  vxh4layout->addWidget(deleteAll);
  vxh4layout->addWidget(resetOptionsButton);
  vxh4layout->addWidget(changeModelButton);

  QVBoxLayout* v3layout = new QVBoxLayout();
  v3layout->addLayout(v1h4layout);
  v3layout->addLayout(vxh4layout);

  QHBoxLayout* h3layout = new QHBoxLayout();
  h3layout->addLayout(v3layout);
  //  h3layout->addLayout(v4layout);

  QGridLayout* optlayout = new QGridLayout();
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

  QHBoxLayout* levelsliderlayout = new QHBoxLayout();
  levelsliderlayout->setAlignment(Qt::AlignHCenter);
  levelsliderlayout->addWidget(levelSlider);

  QHBoxLayout* levelsliderlabellayout = new QHBoxLayout();
  levelsliderlabellayout->setAlignment(Qt::AlignHCenter);
  levelsliderlabellayout->addWidget(levelsliderlabel);

  QVBoxLayout* levellayout = new QVBoxLayout();
  levellayout->addWidget(levelLabel);
  levellayout->addLayout(levelsliderlayout);
  levellayout->addLayout(levelsliderlabellayout);

  QHBoxLayout* idnumsliderlayout = new QHBoxLayout();
  idnumsliderlayout->setAlignment(Qt::AlignHCenter);
  idnumsliderlayout->addWidget(idnumSlider);

  QHBoxLayout* idnumsliderlabellayout = new QHBoxLayout();
  idnumsliderlabellayout->setAlignment(Qt::AlignHCenter);
  idnumsliderlabellayout->addWidget(idnumsliderlabel);

  QVBoxLayout* idnumlayout = new QVBoxLayout();
  idnumlayout->addWidget(idnumLabel);
  idnumlayout->addLayout(idnumsliderlayout);
  idnumlayout->addLayout(idnumsliderlabellayout);

  QHBoxLayout* h4layout = new QHBoxLayout();
  //h4layout->addLayout(h2layout);
  h4layout->addLayout(optlayout);
  h4layout->addLayout(levellayout);
  h4layout->addLayout(idnumlayout);

  QHBoxLayout* h5layout = new QHBoxLayout();
  h5layout->addWidget(allTimeStepButton);
  h5layout->addWidget(advanced);

  QLayout* h6layout = createStandardButtons(false);

  QVBoxLayout* v6layout = new QVBoxLayout();
  v6layout->addLayout(h5layout);
  v6layout->addLayout(h6layout);

  QVBoxLayout* vlayout = new QVBoxLayout();
  vlayout->addLayout(v1layout);
  vlayout->addLayout(h3layout);
  vlayout->addLayout(h4layout);
  vlayout->addLayout(v6layout);

  CreateAdvanced();

  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addLayout(vlayout, 0, 0);
  mainLayout->addWidget(advFrame, 0, 1);
  mainLayout->setColumnStretch(1, 1);
  setLayout(mainLayout);

  doShowMore(false);

  // tool tips
  toolTips();

  updateDialog();
  setDefaultFieldOptions();
}

void FieldDialog::toolTips()
{
  predefinedPlotsCheckBox->setToolTip(tr("Show predefined plots or all parameters from file"));
  upFieldButton->setToolTip(tr("move selected field up"));
  downFieldButton->setToolTip(tr("move selected field down"));
  deleteButton->setToolTip(tr("delete selected field"));
  deleteAll->setToolTip(tr("delete all selected fields"));
  copyField->setToolTip(tr("copy field"));
  resetOptionsButton->setToolTip(tr("reset plot options"));
  minusButton->setToolTip(tr("selected field minus the field above"));
  changeModelButton->setToolTip(tr("change model/termin"));
  allTimeStepButton->setToolTip(tr("all time steps / only common time steps"));

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

void FieldDialog::doShowMore(bool more)
{
  advanced->setChecked(more);
  advFrame->setVisible(more);
}

bool FieldDialog::showsMore()
{
  return advanced->isChecked();
}

std::string FieldDialog::name() const
{
  static const std::string FIELD_DATATYPE = "field";
  return FIELD_DATATYPE;
}

void FieldDialog::updateTimes()
{
}

void FieldDialog::CreateAdvanced()
{
  METLIBS_LOG_SCOPE();

  advFrame = new QWidget(this);

  // Extreme (min,max): type, size and search radius
  QLabel* extremeTypeLabel = TitleLabel(tr("Min,max"), advFrame);
  extremeType.push_back("None");
  extremeType.push_back("L+H");
  extremeType.push_back("L+H+Value");
  extremeType.push_back("C+W");
  extremeType.push_back("Value");
  extremeType.push_back("Minvalue");
  extremeType.push_back("Maxvalue");
  extremeTypeCbox = ComboBox(advFrame, extremeType);
  connect( extremeTypeCbox, SIGNAL( activated(int) ),
      SLOT( extremeTypeActivated(int) ) );

  QLabel* extremeSizeLabel = new QLabel(tr("Size"), advFrame);
  extremeSizeSpinBox = new QSpinBox(advFrame);
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  connect( extremeSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( extremeSizeChanged(int) ) );

  QLabel* extremeRadiusLabel = new QLabel(tr("Radius"), advFrame);
  extremeRadiusSpinBox = new QSpinBox(advFrame);
  extremeRadiusSpinBox->setMinimum(5);
  extremeRadiusSpinBox->setMaximum(300);
  extremeRadiusSpinBox->setSingleStep(5);
  extremeRadiusSpinBox->setWrapping(true);
  extremeRadiusSpinBox->setSuffix("%");
  extremeRadiusSpinBox->setValue(100);
  connect( extremeRadiusSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( extremeRadiusChanged(int) ) );

  // line smoothing
  QLabel* lineSmoothLabel = new QLabel(tr("Smooth lines"), advFrame);
  lineSmoothSpinBox = new QSpinBox(advFrame);
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  connect( lineSmoothSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( lineSmoothChanged(int) ) );

  // field smoothing
  QLabel* fieldSmoothLabel = new QLabel(tr("Smooth fields"), advFrame);
  fieldSmoothSpinBox = new QSpinBox(advFrame);
  fieldSmoothSpinBox->setMinimum(0);
  fieldSmoothSpinBox->setMaximum(20);
  fieldSmoothSpinBox->setSingleStep(1);
  fieldSmoothSpinBox->setSpecialValueText(tr("Off"));
  fieldSmoothSpinBox->setValue(0);
  connect( fieldSmoothSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( fieldSmoothChanged(int) ) );

  labelSizeSpinBox = new QSpinBox(advFrame);
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(300);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  connect( labelSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( labelSizeChanged(int) ) );

  valuePrecisionBox = new QComboBox(advFrame);
  valuePrecisionBox->addItem("0");
  valuePrecisionBox->addItem("1");
  valuePrecisionBox->addItem("2");
  valuePrecisionBox->addItem("3");
  connect( valuePrecisionBox, SIGNAL( activated(int) ),
      SLOT( valuePrecisionBoxActivated(int) ) );

  // grid values
  gridValueCheckBox = new QCheckBox(QString(tr("Grid value")), advFrame);
  gridValueCheckBox->setChecked(false);
  connect( gridValueCheckBox, SIGNAL( toggled(bool) ),
      SLOT( gridValueCheckBoxToggled(bool) ) );

  // grid lines
  QLabel* gridLinesLabel = new QLabel(tr("Grid lines"), advFrame);
  gridLinesSpinBox = new QSpinBox(advFrame);
  gridLinesSpinBox->setMinimum(0);
  gridLinesSpinBox->setMaximum(50);
  gridLinesSpinBox->setSingleStep(1);
  gridLinesSpinBox->setSpecialValueText(tr("Off"));
  gridLinesSpinBox->setValue(0);
  connect( gridLinesSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( gridLinesChanged(int) ) );

  // grid lines max
  //   QLabel* gridLinesMaxLabel= new QLabel( tr("Max grid l."), advFrame );
  //   gridLinesMaxSpinBox= new QSpinBox( 0,200,5, advFrame );
  //   gridLinesMaxSpinBox->setSpecialValueText(tr("All"));
  //   gridLinesMaxSpinBox->setValue(0);
  //   connect( gridLinesMaxSpinBox, SIGNAL( valueChanged(int) ),
  // 	   SLOT( gridLinesMaxChanged(int) ) );


  QLabel* hourOffsetLabel = new QLabel(tr("Time offset"), advFrame);
  hourOffsetSpinBox = new QSpinBox(advFrame);
  hourOffsetSpinBox->setMinimum(-72);
  hourOffsetSpinBox->setMaximum(72);
  hourOffsetSpinBox->setSingleStep(1);
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  connect( hourOffsetSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( hourOffsetChanged(int) ) );

  QLabel* hourDiffLabel = new QLabel(tr("Time diff."), advFrame);
  hourDiffSpinBox = new QSpinBox(advFrame);
  hourDiffSpinBox->setMinimum(0);
  hourDiffSpinBox->setMaximum(24);
  hourDiffSpinBox->setSingleStep(1);
  hourDiffSpinBox->setSuffix(tr(" hour(s)"));
  hourDiffSpinBox->setPrefix(" +/-");
  hourDiffSpinBox->setValue(0);
  connect( hourDiffSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( hourDiffChanged(int) ) );

  // Undefined masking
  QLabel* undefMaskingLabel = TitleLabel(tr("Undefined"), advFrame);
  undefMasking.push_back(tr("Unmarked").toStdString());
  undefMasking.push_back(tr("Coloured").toStdString());
  undefMasking.push_back(tr("Lines").toStdString());
  undefMaskingCbox = ComboBox(advFrame, undefMasking);
  connect( undefMaskingCbox, SIGNAL( activated(int) ),
      SLOT( undefMaskingActivated(int) ) );

  // Undefined masking colour
  undefColourCbox = ColourBox(advFrame, false, 0, "", true);
  connect( undefColourCbox, SIGNAL( activated(int) ),
      SLOT( undefColourActivated(int) ) );

  // Undefined masking linewidth
  undefLinewidthCbox = LinewidthBox(advFrame, false);
  connect( undefLinewidthCbox, SIGNAL( activated(int) ),
      SLOT( undefLinewidthActivated(int) ) );

  // Undefined masking linetype
  undefLinetypeCbox = LinetypeBox(advFrame, false);
  connect( undefLinetypeCbox, SIGNAL( activated(int) ),
      SLOT( undefLinetypeActivated(int) ) );

  // enable/disable numbers on isolines
  valueLabelCheckBox = new QCheckBox(QString(tr("Numbers")), advFrame);
  valueLabelCheckBox->setChecked(true);
  connect( valueLabelCheckBox, SIGNAL( toggled(bool) ),
      SLOT( valueLabelCheckBoxToggled(bool) ) );

  //Options
  QLabel* shadingLabel = new QLabel(tr("Palette"), advFrame);
  QLabel* shadingcoldLabel = new QLabel(tr("Palette (-)"), advFrame);
  QLabel* patternLabel = new QLabel(tr("Pattern"), advFrame);
  QLabel* alphaLabel = new QLabel(tr("Alpha"), advFrame);
  QLabel* headLabel = TitleLabel(tr("Extra contour lines"), advFrame);
  QLabel* colourLabel = new QLabel(tr("Line colour"), advFrame);
  QLabel* intervalLabel = new QLabel(tr("Line interval"), advFrame);
  QLabel* baseLabel = new QLabel(tr("Basis value"), advFrame);
  QLabel* minLabel = new QLabel(tr("Min"), advFrame);
  QLabel* maxLabel = new QLabel(tr("Max"), advFrame);
  QLabel* base2Label = new QLabel(tr("Basis value"), advFrame);
  QLabel* min2Label = new QLabel(tr("Min"), advFrame);
  QLabel* max2Label = new QLabel(tr("Max"), advFrame);
  QLabel* linewidthLabel = new QLabel(tr("Line width"), advFrame);
  QLabel* linetypeLabel = new QLabel(tr("Line type"), advFrame);
  QLabel* threeColourLabel = TitleLabel(tr("Three colours"), advFrame);

  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
  connect( tableCheckBox, SIGNAL( toggled(bool) ),
      SLOT( tableCheckBoxToggled(bool) ) );

  repeatCheckBox = new QCheckBox(tr("Repeat"), advFrame);
  connect( repeatCheckBox, SIGNAL( toggled(bool) ),
      SLOT( repeatCheckBoxToggled(bool) ) );

  //3 colours
  //  threeColoursCheckBox = new QCheckBox(tr("Three colours"), advFrame);

  for (size_t i = 0; i < 3; i++) {
    threeColourBox.push_back(ColourBox(advFrame, true, 0,
        tr("Off").toStdString(),true));
    connect (threeColourBox[i], SIGNAL(activated(int)),
        SLOT(threeColoursChanged()));
  }

  //shading
  shadingComboBox=
      PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString(),true );
  shadingComboBox->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( shadingComboBox, SIGNAL( activated(int) ),
      SLOT( shadingChanged() ) );

  shadingSpinBox= new QSpinBox( advFrame );
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(255);
  shadingSpinBox->setSingleStep(1);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  connect( shadingSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldComboBox=
      PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString(),true );
  shadingcoldComboBox->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( shadingcoldComboBox, SIGNAL( activated(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldSpinBox= new QSpinBox( advFrame );
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(255);
  shadingcoldSpinBox->setSingleStep(1);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  connect( shadingcoldSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( shadingChanged() ) );

  //pattern
  patternComboBox =
      PatternBox( advFrame,patternInfo,false,0,tr("Off").toStdString(),true );
  patternComboBox ->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( patternComboBox, SIGNAL( activated(int) ),
      SLOT( patternComboBoxToggled(int) ) );

  //pattern colour
  patternColourBox = ColourBox(advFrame,false,0,tr("Auto").toStdString(),true);
  connect( patternColourBox, SIGNAL( activated(int) ),
      SLOT( patternColourBoxToggled(int) ) );

  //alpha blending
  alphaSpinBox= new QSpinBox( advFrame );
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
  alphaSpinBox->setValue(255);
  connect( alphaSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( alphaChanged(int) ) );

  //colour
  colour2ComboBox = ColourBox(advFrame, false,0,tr("Off").toStdString(),true);
  connect( colour2ComboBox, SIGNAL( activated(int) ),
      SLOT( colour2ComboBoxToggled(int) ) );

  //line interval
  interval2ComboBox = new QComboBox(advFrame);
  connect( interval2ComboBox, SIGNAL( activated(int) ),
      SLOT( interval2ComboBoxToggled(int) ) );

  //zero value
  zero1ComboBox= new QComboBox( advFrame );
  zero2ComboBox = new QComboBox(advFrame);
  connect( zero1ComboBox, SIGNAL( activated(int) ),
      SLOT( zero1ComboBoxToggled(int) ) );
  connect( zero2ComboBox, SIGNAL( activated(int) ),
      SLOT( zero2ComboBoxToggled(int) ) );

  //min
  min1ComboBox = new QComboBox(advFrame);
  min2ComboBox = new QComboBox(advFrame);

  //max
  max1ComboBox = new QComboBox(advFrame);
  max2ComboBox = new QComboBox(advFrame);

  //line values
  linevaluesField = new QLineEdit(advFrame);
  connect(linevaluesField, SIGNAL(editingFinished()),
      SLOT(linevaluesFieldEdited()));
  // log line values
  linevaluesLogCheckBox = new QCheckBox(QString(tr("Log")), advFrame);
  linevaluesLogCheckBox->setChecked(false);
  connect(linevaluesLogCheckBox, SIGNAL(toggled(bool)),
      SLOT(linevaluesLogCheckBoxToggled(bool)));

  connect( min1ComboBox, SIGNAL( activated(int) ),
      SLOT( min1ComboBoxToggled(int) ) );
  connect( max1ComboBox, SIGNAL( activated(int) ),
      SLOT( max1ComboBoxToggled(int) ) );
  connect( min2ComboBox, SIGNAL( activated(int) ),
      SLOT( min2ComboBoxToggled(int) ) );
  connect( max2ComboBox, SIGNAL( activated(int) ),
      SLOT( max2ComboBoxToggled(int) ) );

  //linewidth
  linewidth2ComboBox = LinewidthBox( advFrame);
  connect( linewidth2ComboBox, SIGNAL( activated(int) ),
      SLOT( linewidth2ComboBoxToggled(int) ) );
  //linetype
  linetype2ComboBox = LinetypeBox( advFrame,false );
  connect( linetype2ComboBox, SIGNAL( activated(int) ),
      SLOT( linetype2ComboBoxToggled(int) ) );

  // Plot frame
  frameCheckBox= new QCheckBox(QString(tr("Frame")), advFrame);
  frameCheckBox->setChecked( true );
  connect( frameCheckBox, SIGNAL( toggled(bool) ),
      SLOT( frameCheckBoxToggled(bool) ) );

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox= new QCheckBox(QString(tr("Zero line")), advFrame);
  //  zeroLineColourCBox= new QComboBox(advFrame);
  zeroLineCheckBox->setChecked( true );

  connect( zeroLineCheckBox, SIGNAL( toggled(bool) ),
      SLOT( zeroLineCheckBoxToggled(bool) ) );

  // Create horizontal frame lines
  QFrame *line0 = new QFrame( advFrame );
  line0->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line1 = new QFrame( advFrame );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line2 = new QFrame( advFrame );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line3 = new QFrame( advFrame );
  line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line4 = new QFrame( advFrame );
  line4->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line5 = new QFrame( advFrame );
  line5->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line6 = new QFrame( advFrame );
  line6->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // layout......................................................

  QGridLayout* advLayout = new QGridLayout( );
  advLayout->setSpacing(1);
  int line = 0;
  advLayout->addWidget( extremeTypeLabel, line, 0 );
  advLayout->addWidget(extremeSizeLabel, line, 1 );
  advLayout->addWidget(extremeRadiusLabel, line, 2 );
  line++;
  advLayout->addWidget( extremeTypeCbox, line, 0 );
  advLayout->addWidget(extremeSizeSpinBox, line, 1 );
  advLayout->addWidget(extremeRadiusSpinBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);
  advLayout->addWidget(line0, line,0,1,3 );

  line++;
  advLayout->addWidget(gridLinesLabel, line, 0 );
  advLayout->addWidget(gridLinesSpinBox, line, 1 );
  advLayout->addWidget(gridValueCheckBox, line, 2 );
  line++;
  advLayout->addWidget(lineSmoothLabel, line, 0 );
  advLayout->addWidget(lineSmoothSpinBox, line, 1 );
  line++;
  advLayout->addWidget(fieldSmoothLabel, line, 0 );
  advLayout->addWidget(fieldSmoothSpinBox, line, 1 );
  line++;
  advLayout->addWidget(hourOffsetLabel, line, 0 );
  advLayout->addWidget(hourOffsetSpinBox, line, 1 );
  line++;
  advLayout->addWidget(hourDiffLabel, line, 0 );
  advLayout->addWidget(hourDiffSpinBox, line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line4, line,0, 1,3 );
  line++;
  advLayout->addWidget(undefMaskingLabel, line, 0 );
  advLayout->addWidget(undefMaskingCbox, line, 1 );
  line++;
  advLayout->addWidget(undefColourCbox, line, 0 );
  advLayout->addWidget(undefLinewidthCbox, line, 1 );
  advLayout->addWidget(undefLinetypeCbox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line1, line,0, 1,4 );

  line++;
  advLayout->addWidget(frameCheckBox, line, 0 );
  advLayout->addWidget(zeroLineCheckBox, line, 1 );
  line++;
  advLayout->addWidget(valueLabelCheckBox, line, 0 );
  advLayout->addWidget(labelSizeSpinBox, line, 1 );
  advLayout->addWidget(valuePrecisionBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line2, line,0, 1,3 );

  line++;
  advLayout->addWidget( tableCheckBox, line, 0 );
  advLayout->addWidget( repeatCheckBox, line, 1 );
  line++;
  advLayout->addWidget( shadingLabel, line, 0 );
  advLayout->addWidget( shadingComboBox, line, 1 );
  advLayout->addWidget( shadingSpinBox, line, 2 );
  line++;
  advLayout->addWidget( shadingcoldLabel, line, 0 );
  advLayout->addWidget( shadingcoldComboBox,line, 1 );
  advLayout->addWidget( shadingcoldSpinBox, line, 2 );
  line++;
  advLayout->addWidget( patternLabel, line, 0 );
  advLayout->addWidget( patternComboBox, line, 1 );
  advLayout->addWidget( patternColourBox, line, 2 );
  line++;
  advLayout->addWidget( alphaLabel, line, 0 );
  advLayout->addWidget( alphaSpinBox, line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line6, line,0, 1,3 );

  line++;
  advLayout->addWidget( baseLabel, line, 0 );
  advLayout->addWidget( minLabel, line, 1 );
  advLayout->addWidget( maxLabel, line, 2 );
  line++;
  advLayout->addWidget( zero1ComboBox, line, 0 );
  advLayout->addWidget( min1ComboBox, line, 1 );
  advLayout->addWidget( max1ComboBox, line, 2 );
  line++;
  advLayout->addWidget( new QLabel(tr("Values"), this), line, 0 );
  advLayout->addWidget( linevaluesField, line, 1 );
  advLayout->addWidget( linevaluesLogCheckBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line3, line,0,1,3 );

  line++;
  advLayout->addWidget(headLabel,line,0,1,2);
  line++;
  advLayout->addWidget( colourLabel, line, 0 );
  advLayout->addWidget( colour2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( intervalLabel, line, 0 );
  advLayout->addWidget( interval2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( linewidthLabel, line, 0 );
  advLayout->addWidget( linewidth2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( linetypeLabel, line, 0 );
  advLayout->addWidget( linetype2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( base2Label, line, 0 );
  advLayout->addWidget( min2Label, line, 1 );
  advLayout->addWidget( max2Label, line, 2 );
  line++;
  advLayout->addWidget( zero2ComboBox, line, 0 );
  advLayout->addWidget( min2ComboBox, line, 1 );
  advLayout->addWidget( max2ComboBox, line, 2 );

  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line5, line,0, 1,3 );
  line++;
  advLayout->addWidget( threeColourLabel, line, 0 );
  //  advLayout->addWidget( threeColoursCheckBox, 38, 0 );
  line++;
  advLayout->addWidget( threeColourBox[0], line, 0 );
  advLayout->addWidget( threeColourBox[1], line, 1 );
  advLayout->addWidget( threeColourBox[2], line, 2 );

  // a separator
  QFrame* advSep= new QFrame( advFrame );
  advSep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  advSep->setLineWidth(5);

  QHBoxLayout *hLayout = new QHBoxLayout(advFrame);
  hLayout->setMargin(0);
  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);
}

void FieldDialog::updateModelBoxes()
{
  METLIBS_LOG_SCOPE();

  //keep old plots
  const PlotCommand_cpv vstr = getOKString();

  modelItems->clear();
  modelFilterEdit->clear();

  int nr_m = m_modelgroup.size();
  if (nr_m == 0)
    return;

  if (useArchive) {
    for (int i = 0; i < nr_m; i++) {
      if (m_modelgroup[i].groupType == FieldModelGroupInfo::ARCHIVE_GROUP) {
        addModelGroup(i);
      }
    }
  }
  for (int i = 0; i < nr_m; i++) {
    if (m_modelgroup[i].groupType == FieldModelGroupInfo::STANDARD_GROUP) {
      addModelGroup(i);
    }
  }

  if (selectedFields.size() > 0)
    deleteAllSelected();

  //replace old plots
  putOKString(vstr);
}

void FieldDialog::addModelGroup(int modelgroupIndex)
{
  METLIBS_LOG_SCOPE(LOGVAL(modelgroupIndex));
  const FieldModelGroupInfo& mgr = m_modelgroup[modelgroupIndex];
  METLIBS_LOG_DEBUG(LOGVAL(mgr.groupName));
  QStandardItem* group = new QStandardItem(QString::fromStdString(mgr.groupName));
  group->setData(modelgroupIndex, ROLE_MODELGROUP);
  group->setFlags(Qt::ItemIsEnabled);
  for (const FieldModelInfo& fdmi : mgr.models) {
    METLIBS_LOG_DEBUG(LOGVAL(fdmi.modelName));
    QStandardItem* child = new QStandardItem(QString::fromStdString(fdmi.modelName));
    child->setToolTip(QString::fromStdString(fdmi.setupInfo).split(" ", QString::SkipEmptyParts).join("\n"));
    child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    group->appendRow(child);
  }
  modelItems->appendRow(group);
}

void FieldDialog::filterModels(const QString& filtertext)
{
  modelFilter->setFilterFixedString(filtertext);
  if (!filtertext.isEmpty()) {
    modelbox->expandAll();
    if (modelFilter->rowCount() > 0)
      modelbox->scrollTo(modelFilter->index(0, 0));
  }
}

void FieldDialog::updateDialog()
{
  m_modelgroup = m_data->getFieldModelGroups();
  updateModelBoxes();
}

void FieldDialog::archiveMode(bool on)
{
  useArchive = on;
  updateModelBoxes();
}

void FieldDialog::modelboxClicked(const QModelIndex& filterIndex)
{
  METLIBS_LOG_TIME();

  diutil::OverrideCursor waitCursor;

  const QModelIndex index = modelFilter->mapToSource(filterIndex);
  QStandardItem* clickItem = modelItems->itemFromIndex(index);
  QStandardItem* parentItem = clickItem->parent();
  if (!parentItem) {
    // it is a model group, do nothing
    return;
  }

  refTimeComboBox->clear();
  fieldGRbox->clear();
  fieldbox->clear();

  const int indexM = clickItem->row();
  const int indexMGR = parentItem->data(ROLE_MODELGROUP).toInt();
  METLIBS_LOG_DEBUG(LOGVAL(indexMGR) << LOGVAL(indexM));

  currentModel = m_modelgroup[indexMGR].models[indexM].modelName;

  const set<std::string> refTimes = m_data->getFieldReferenceTimes(currentModel);

  for (const std::string& rt : refTimes) {
    refTimeComboBox->addItem(QString::fromStdString(rt));
  }
  if ( refTimeComboBox->count() ) {
    refTimeComboBox->setCurrentIndex(refTimeComboBox->count()-1);
    updateFieldGroups();
  }
}

void FieldDialog::updateFieldGroups()
{
  METLIBS_LOG_TIME();

  fieldGRbox->clear();
  fieldbox->clear();

  getFieldGroups(currentModel, refTimeComboBox->currentText().toStdString(), predefinedPlotsCheckBox->isChecked(), vfgi);

  int nvfgi = vfgi.size();

  if (nvfgi > 0) {
    for (int i = 0; i < nvfgi; i++) {
      fieldGRbox->addItem(QString::fromStdString(vfgi[i].groupName()));
    }

    int indexFGR = -1;
    int i = 0;
    while (i < nvfgi && vfgi[i].groupName() != lastFieldGroupName)
      i++;
    if (i < nvfgi) {
      indexFGR = i;
    }
    if (indexFGR < 0)
      indexFGR = 0;
    lastFieldGroupName = vfgi[indexFGR].groupName();
    fieldGRbox->setCurrentIndex(indexFGR);
    fieldGRboxActivated(indexFGR);
  }

  if (!selectedFields.empty()) {
    enableFieldOptions();
  }

  updateTime();
}

void FieldDialog::fieldGRboxActivated(int indexFGR)
{
  METLIBS_LOG_SCOPE();

  fieldbox->clear();
  fieldbox->blockSignals(true);

  if (!vfgi.empty()) {
    FieldPlotGroupInfo& fpgi = vfgi[indexFGR];
    lastFieldGroupName = fpgi.groupName();
    for (const FieldPlotInfo& plot : fpgi.plots) {
      QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(plot.fieldName));
      item->setToolTip(QString::fromStdString(plot.variableName));
      fieldbox->addItem(item);
    }
  }

  fieldbox->blockSignals(false);
}

void FieldDialog::setLevel()
{
  METLIBS_LOG_SCOPE();

  int i = selectedFieldbox->currentRow();
  int n = 0, l = 0;
  if (i >= 0 && !selectedFields[i].level.empty()) {
    lastLevel = selectedFields[i].level;
    n = selectedFields[i].levelOptions.size();
    l = 0;
    while (l < n && selectedFields[i].levelOptions[l] != lastLevel)
      l++;
    if (l == n)
      l = 0; // should not happen!
  }

  levelSlider->blockSignals(true);

  if (n > 0) {
    currentLevels = selectedFields[i].levelOptions;
    levelSlider->setRange(0, n - 1);
    levelSlider->setValue(l);
    levelSlider->setEnabled(true);
    levelLabel->setText(QString::fromStdString(lastLevel));
  } else {
    currentLevels.clear();
    // keep slider in a fixed position when disabled
    levelSlider->setEnabled(false);
    levelSlider->setValue(1);
    levelSlider->setRange(0, 1);
    levelLabel->clear();
  }

  levelSlider->blockSignals(false);
}

void FieldDialog::levelPressed()
{
  levelInMotion = true;
}

void FieldDialog::levelChanged(int index)
{
  METLIBS_LOG_SCOPE("index="<<index);

  int n = currentLevels.size();
  if (index >= 0 && index < n) {
    lastLevel = currentLevels[index];
    levelLabel->setText(QString::fromStdString(lastLevel));
  }

  if (!levelInMotion)
    updateLevel();
}

void FieldDialog::changeLevel(int increment, int type)
{
  METLIBS_LOG_SCOPE("increment="<<increment);

  // called from MainWindow levelUp/levelDown

  std::string level;
  vector<std::string> vlevels;
  int n = selectedFields.size();

  //For some reason (?) vertical levels and extra levels are sorted i opposite directions
  if ( type == 0 ) {
    increment *= -1;
  }

  //find first plot with levels, use use this plot to select next level
  int i = 0;
  if ( type==0 ) { //vertical levels
    while ( i < n && selectedFields[i].levelOptions.size() < 2 ) i++;
    if( i != n ) {
      vlevels = selectedFields[i].levelOptions;
      level = selectedFields[i].level;
    }
  } else { // extra levels
    while ( i < n && selectedFields[i].idnumOptions.size() < 2 ) i++;
    if( i != n ) {
      vlevels = selectedFields[i].idnumOptions;
      level = selectedFields[i].idnum;
    }
  }


  //plot with levels exists, find next level
  if( i != n ) {
    std::string level_incremented;
    int m = vlevels.size();
    int current = 0;
    while (current < m && vlevels[current] != level) current++;
    int new_index = current + increment;
    if (new_index < m && new_index > -1) {
      level_incremented = vlevels[new_index];

      //loop through all plots to see if it is possible to plot:

      if ( type == 0 ) { //vertical levels
        for (int j = 0; j < n; j++) {
          if (selectedFields[j].levelmove && selectedFields[j].level == level) {
            selectedFields[j].level = level_incremented;
            //update dialog
            if(j==selectedFieldbox->currentRow()){
              levelSlider->blockSignals(true);
              levelSlider->setValue(new_index);
              levelSlider->blockSignals(false);
              levelChanged(new_index);
            }
          } else {
            selectedFields[j].levelmove = false;
          }
        }
      } else { // extra levels
        for (int j = 0; j < n; j++) {
          if (selectedFields[j].idnummove && selectedFields[j].idnum == level) {
            selectedFields[j].idnum = level_incremented;
            //update dialog
            if(j==selectedFieldbox->currentRow()){
              idnumSlider->blockSignals(true);
              idnumSlider->setValue(new_index);
              idnumSlider->blockSignals(false);
              idnumChanged(new_index);
            }
          } else {
            selectedFields[j].idnummove = false;
          }
        }
      }
    }
  }
}

void FieldDialog::updateLevel()
{
  METLIBS_LOG_SCOPE();

  int i = selectedFieldbox->currentRow();

  if (i >= 0 && i < int(selectedFields.size())) {
    selectedFields[i].level = lastLevel;
    updateTime();
  }

  levelInMotion = false;
}

void FieldDialog::setIdnum()
{
  METLIBS_LOG_SCOPE();

  int i = selectedFieldbox->currentRow();
  int n = 0, l = 0;
  if (i >= 0 && !selectedFields[i].idnumOptions.empty()) {
    n = selectedFields[i].idnumOptions.size();
    if (!selectedFields[i].idnum.empty()) {
      lastIdnum = selectedFields[i].idnum;
      while (l < n && selectedFields[i].idnumOptions[l] != lastIdnum)
        l++;
      if (l == n)
        l = 0;
    }
  }

  idnumSlider->blockSignals(true);

  if (n > 0) {
    currentIdnums = selectedFields[i].idnumOptions;
    idnumSlider->setRange(0, n - 1);
    idnumSlider->setValue(l);
    idnumSlider->setEnabled(true);
    idnumLabel->setText(QString::fromStdString(lastIdnum));
  } else {
    currentIdnums.clear();
    // keep slider in a fixed position when disabled
    idnumSlider->setEnabled(false);
    idnumSlider->setValue(1);
    idnumSlider->setRange(0, 1);
    idnumLabel->clear();
  }

  idnumSlider->blockSignals(false);
}

void FieldDialog::idnumPressed()
{
  idnumInMotion = true;
}

void FieldDialog::idnumChanged(int index)
{
  METLIBS_LOG_SCOPE();

  int n = currentIdnums.size();
  if (index >= 0 && index < n) {
    lastIdnum = currentIdnums[index];
    idnumLabel->setText(QString::fromStdString(lastIdnum));
  }

  if (!idnumInMotion)
    updateIdnum();
}


void FieldDialog::updateIdnum()
{
  METLIBS_LOG_SCOPE();

  if (selectedFieldbox->currentRow() >= 0) {
    unsigned int i = selectedFieldbox->currentRow();
    if (i < selectedFields.size()) {
      selectedFields[i].idnum = lastIdnum;
      updateTime();
    }
  }

  idnumInMotion = false;
}

void FieldDialog::fieldboxChanged(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldbox->count()));

  if (!fieldGRbox->count())
    return;
  if (!fieldbox->count())
    return;

  int indexFGR = fieldGRbox->currentIndex();

  if (item->isSelected() ) {

    SelectedField sf;
    sf.inEdit = false;
    sf.external = false;
    sf.modelName = currentModel;
    sf.fieldName = fieldbox->currentItem()->text().toStdString();
    sf.refTime = refTimeComboBox->currentText().toStdString();
    const FieldPlotInfo& fi = vfgi[indexFGR].plots[fieldbox->currentRow()];
    sf.levelOptions = fi.vlevels();
    sf.idnumOptions = fi.elevels();
    sf.zaxis = fi.vcoord();
    sf.extraaxis = fi.ecoord();

    sf.predefinedPlot = predefinedPlotsCheckBox->isChecked();
    sf.minus = false;

    int n = sf.levelOptions.size();
    int i = 0;
    while (i < n && sf.levelOptions[i] != lastLevel)
      i++;
    if (i < n) {
      sf.level = lastLevel;
    } else {
      if (!fi.default_vlevel().empty()) {
        sf.level = fi.default_vlevel();
      } else if (sf.levelOptions.size() ) {
        sf.level = sf.levelOptions[sf.levelOptions.size()-1];
      }
    }
    n = fi.elevels().size();
    i = 0;
    while (i < n && fi.elevels()[i] != lastIdnum)
      i++;
    if (i < n) {
      sf.idnum = lastIdnum;
    } else {
      if (!fi.default_elevel().empty()) {
        sf.idnum = fi.default_elevel();
      } else if (sf.idnumOptions.size() ) {
        sf.idnum = sf.idnumOptions[0];
      }
    }
    sf.hourOffset = 0;
    sf.hourDiff = 0;

    sf.fieldOpts = getFieldOptions(sf.fieldName, false);

    selectedFields.push_back(sf);

    std::string text = sf.modelName + " " + sf.fieldName + " " + sf.refTime;
    int newCurrent = selectedFieldbox->count();
    selectedFieldbox->addItem(QString::fromStdString(text));
    selectedFieldbox->setCurrentRow(newCurrent);
    selectedFieldbox->item(newCurrent)->setSelected(true);
    enableFieldOptions();

  } else if (!item->isSelected()) {
    std::string fieldName = item->text().toStdString();
    int n = selectedFields.size();
    int j;
    for (j = 0; j < n; j++) {
      if (selectedFields[j].modelName == currentModel && selectedFields[j].fieldName == fieldName) {
        selectedFieldbox->takeItem(j);
        break;
      }
    }

    if( j < n ) {
      for (int i = j; i < n - 1; i++)
        selectedFields[i] = selectedFields[i + 1];
      selectedFields.pop_back();
    }
    //select next field or last field
    if ( selectedFieldbox->count() ) {
      int newCurrent = selectedFieldbox->count() - 1;
      if ( j < selectedFieldbox->count())
        newCurrent = j;
      selectedFieldbox->setCurrentRow(newCurrent);
      selectedFieldbox->item(newCurrent)->setSelected(true);
      enableFieldOptions();
    } else {
      enableWidgets("none");
    }
  }


  updateTime();

  //first field can't be minus
  if (selectedFieldbox->count() > 0 && selectedFields[0].minus) {
    minusButton->setChecked(false);
  }
}

void FieldDialog::enableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  setDefaultFieldOptions();

  size_t nc;
  int i, n;

  int index = selectedFieldbox->currentRow();
  int lastindex = selectedFields.size() - 1;

  if (index < 0 || index > lastindex) {
    METLIBS_LOG_ERROR("POGRAM ERROR.1 in FieldDialog::enableFieldOptions");
    METLIBS_LOG_ERROR("       index,selectedFields.size: " << index << " "
        << selectedFields.size());
    enableWidgets("none");
    return;
  }

  SelectedField& sf = selectedFields[index];
  if (sf.inEdit) {
    changeModelButton->setEnabled(false);
    deleteButton->setEnabled(false);
    copyField->setEnabled(false);
  } else {
    upFieldButton->setEnabled((index > numEditFields));
    downFieldButton->setEnabled((index < lastindex));
    changeModelButton->setEnabled(true);
    deleteButton->setEnabled(true);
    copyField->setEnabled(true);
  }

  setLevel();
  setIdnum();
  minusButton->setEnabled(index > 0 && !selectedFields[index - 1].minus);
  deleteAll->setEnabled(true);
  resetOptionsButton->setEnabled(true);

  if (sf.fieldOpts == currentFieldOpts
      && sf.inEdit == currentFieldOptsInEdit
      && !sf.minus)
    return;

  currentFieldOpts = sf.fieldOpts;
  currentFieldOptsInEdit = sf.inEdit;

  if (sf.minus && !minusButton->isChecked())
    minusButton->setChecked(true);
  else if (!sf.minus && minusButton->isChecked())
    minusButton->setChecked(false);

  //hourOffset
  if (currentFieldOptsInEdit) {
    hourOffsetSpinBox->setValue(0);
    hourOffsetSpinBox->setEnabled(false);
  } else {
    i = sf.hourOffset;
    hourOffsetSpinBox->setValue(i);
    hourOffsetSpinBox->setEnabled(true);
  }

  //hourDiff
  if (currentFieldOptsInEdit) {
    hourDiffSpinBox->setValue(0);
    hourDiffSpinBox->setEnabled(false);
  } else {
    i = sf.hourDiff;
    hourDiffSpinBox->setValue(i);
    hourDiffSpinBox->setEnabled(true);
  }

  if (sf.minus)
    return;

  vpcopt = sf.fieldOpts;

  int nr_linetypes = linetypes.size();
  enableWidgets("contour");

  //unit
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

  //dimension (1dim = contour,..., 2dim=wind,...)
  const std::vector< std::vector<std::string> >& plottypes_dim = PlotOptions::getPlotTypes();
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

  //plottype
  if ((nc = miutil::find(vpcopt, PlotOptions::key_plottype)) != npos) {
    const std::string& value = vpcopt[nc].value();
    size_t i = 0;
    while (i < plottypes.size() && value != plottypes[i])
      i++;
    if (i == plottypes.size())
      i=0;
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
    if (miutil::to_lower(vpcopt[nc].value()) == "off" ) {
      updateFieldOptions(PlotOptions::key_colour_2, "off");
      colour2ComboBox->setCurrentIndex(0);
    } else {
      SetCurrentItemColourBox(colour2ComboBox,vpcopt[nc].value());
      updateFieldOptions(PlotOptions::key_colour_2, vpcopt[nc].value());
    }
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_colour)) != npos) {
  //  int nr_colours = colorCbox->count();
    if (miutil::to_lower(vpcopt[nc].value()) == "off" ) {
      updateFieldOptions(PlotOptions::key_colour, "off");
      colorCbox->setCurrentIndex(0);
    } else {
      SetCurrentItemColourBox(colorCbox,vpcopt[nc].value());
      updateFieldOptions(PlotOptions::key_colour, vpcopt[nc].value());
    }
  }

  // 3 colours
  if ((nc = miutil::find(vpcopt, PlotOptions::key_colours)) != npos) {
    vector<std::string> colours = miutil::split(vpcopt[nc].value(),",");
    if (colours.size() == 3) {
      for (size_t j = 0; j < 3; j++) {
        SetCurrentItemColourBox(threeColourBox[j],colours[j]);
      }
      threeColoursChanged();
    }
  }

  //contour shading updating FieldOptions
  if ((nc = miutil::find(vpcopt, PlotOptions::key_palettecolours)) != npos) {
    if (miutil::to_lower(vpcopt[nc].value()) == "off" ) {
      updateFieldOptions(PlotOptions::key_palettecolours, "off");
      shadingComboBox->setCurrentIndex(0);
      shadingcoldComboBox->setCurrentIndex(0);
    } else {
      const vector<std::string> strValue = CommandParser::parseString(vpcopt[nc].value());
      vector<std::string> stokens = miutil::split(strValue[0],";");

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
        ExpandPaletteBox(shadingComboBox,ColourShading(vpcopt[nc].value()));
        ExpandPaletteBox(shadingcoldComboBox,ColourShading(vpcopt[nc].value())); //MC
        ColourShading::ColourShadingInfo info;
        info.name=vpcopt[nc].value();
        info.colour=ColourShading::getColourShading(vpcopt[nc].value());
        csInfo.push_back(info);
      } else if(strValue.size() == 2) {
        coldStokens = miutil::split(strValue[1],";");

        while (j < nr_cs && coldStokens[0] != csInfo[j].name) {
          j++;
        }

        if (j < nr_cs)
          updateClodshading = true;
      }

      str = vpcopt[nc].value();//tokens[0];
      shadingComboBox->setCurrentIndex(i + 1);
      updateFieldOptions(PlotOptions::key_palettecolours, str);
      // Need to set this here otherwise the signal is changing
      // the vpcopt[nc].value() variable to off
      if (stokens.size() == 2)
        shadingSpinBox->setValue(miutil::to_int(stokens[1]));
      else
        shadingSpinBox->setValue(0);

      if(updateClodshading) {
        shadingcoldComboBox->setCurrentIndex(j + 1);

        if (coldStokens.size() == 2)
          shadingcoldSpinBox->setValue(miutil::to_int(coldStokens[1]));
        else
          shadingcoldSpinBox->setValue(0);
      }
    }
  }
  //pattern
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

  //pattern colour
  if ((nc = miutil::find(vpcopt, PlotOptions::key_pcolour)) != npos) {
    SetCurrentItemColourBox(patternColourBox, vpcopt[nc].value());
    updateFieldOptions(PlotOptions::key_pcolour, vpcopt[nc].value());
  }

  //table
  nc = miutil::find(vpcopt, PlotOptions::key_table);
  if (nc != npos) {
    bool on = vpcopt[nc].value() == "1";
    tableCheckBox->setChecked(on);
    tableCheckBoxToggled(on);
  }

  //repeat
  nc = miutil::find(vpcopt, PlotOptions::key_repeat);
  if (nc != npos) {
    bool on = vpcopt[nc].value() == "1";
    repeatCheckBox->setChecked(on);
    repeatCheckBoxToggled(on);
  }

  //alpha shading
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
  if ((nc = miutil::find(vpcopt, PlotOptions::key_linewidth)) != npos  && CommandParser::isInt(vpcopt[nc].value())) {
    i = vpcopt[nc].toInt();;
    int nr_linewidths = lineWidthCbox->count();
    if (  i  > nr_linewidths )  {
      ExpandLinewidthBox(lineWidthCbox, i);
    }
    updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(i));
    lineWidthCbox->setCurrentIndex(i-1);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_linewidth_2)) != npos  && CommandParser::isInt(vpcopt[nc].value())) {
    int nr_linewidths = linewidth2ComboBox->count();
    i = vpcopt[nc].toInt();;
    if ( i  > nr_linewidths )  {
      ExpandLinewidthBox(linewidth2ComboBox, i);
    }
    updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(i));
    linewidth2ComboBox->setCurrentIndex(i-1);
  }


  // line interval (isoline contouring)
  { const size_t nci = miutil::find(vpcopt, PlotOptions::key_lineinterval),
        ncv = miutil::find(vpcopt, PlotOptions::key_linevalues),
        nclv = miutil::find(vpcopt, PlotOptions::key_loglinevalues);
    if (nci  != npos or ncv != npos or nclv != npos) {
      bool enable_values = true;
      if (nci != npos && CommandParser::isFloat(vpcopt[nci].value())) {
        float ekv = vpcopt[nci].toFloat();
        lineintervals = numberList(lineintervalCbox, ekv,true);
        lineintervals2 = numberList(interval2ComboBox, ekv,true);
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
    if ((nc = miutil::find(vpcopt, PlotOptions::key_lineinterval_2)) != npos
        && (CommandParser::isFloat(vpcopt[nc].value()))) {
      float ekv = vpcopt[nc].toFloat();;
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
    n= extremeType.size();
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
      e = vpcopt[nc].toFloat();;
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeSizeSpinBox->setValue(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_extremeRadius)) != npos) {
    float e;
    if (CommandParser::isFloat(vpcopt[nc].value())) {
      e = vpcopt[nc].toFloat();;
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeRadiusSpinBox->setValue(i);
  }

  if ((nc = miutil::find(vpcopt, PlotOptions::key_lineSmooth)) != npos && CommandParser::isInt(vpcopt[nc].value())) {
    i = vpcopt[nc].toInt();;
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
    if (CommandParser::isInt(vpcopt[nc].value()) && vpcopt[nc].toInt() < valuePrecisionBox->count() ) {
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

  //   if ((nc=miutil::find(vpcopt,"grid.lines.max"))>=0) {
  //     if (CommandParser::isInt(vpcopt[nc].value())) i=vpcopt[nc].toInt();;
  //     else i=0;
  //     gridLinesMaxSpinBox->setValue(i);
  //     gridLinesMaxSpinBox->setEnabled(true);
  //   } else if (gridLinesMaxSpinBox->isEnabled()) {
  //     gridLinesMaxSpinBox->setValue(0);
  //     gridLinesMaxSpinBox->setEnabled(false);
  //   }

  // undefined masking
  int iumask = 0;
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefMasking)) != npos) {
    if (CommandParser::isInt(vpcopt[nc].value())) {
      iumask = vpcopt[nc].toInt();;
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
  if ((nc = miutil::find(vpcopt, PlotOptions::key_undefLinewidth)) != npos&& CommandParser::isInt(vpcopt[nc].value())) {
    int nr_linewidths = undefLinewidthCbox->count();
    i = vpcopt[nc].toInt();;
    if ( i  > nr_linewidths )  {
      ExpandLinewidthBox(undefLinewidthCbox, i);
    }
    updateFieldOptions(PlotOptions::key_undefLinewidth, miutil::from_number(i));
    undefLinewidthCbox->setCurrentIndex(i-1);
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

  if ((nc = miutil::find(vpcopt, PlotOptions::key_frame)) != npos ) {
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
    if (CommandParser::isFloat(vpcopt[nc].value())){
      e = vpcopt[nc].toFloat();;
      baseList(zero1ComboBox, e);
    }
    if ((nc = miutil::find(vpcopt, PlotOptions::key_basevalue_2)) != npos) {
      if (CommandParser::isFloat(vpcopt[nc].value())) {
        e = vpcopt[nc].toFloat();;
        baseList(zero2ComboBox, e);
      }
    }
  }

  if (( nc = miutil::find(vpcopt, PlotOptions::key_minvalue)) != npos ) {
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

  if (( nc = miutil::find(vpcopt, PlotOptions::key_maxvalue)) != npos ) {
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

void FieldDialog::setDefaultFieldOptions()
{
  METLIBS_LOG_SCOPE();

  // show levels for the current field group
  //  setLevel();

  currentFieldOpts.clear();

  deleteButton->setEnabled( false );
  deleteAll->setEnabled( false );
  copyField->setEnabled( false );
  changeModelButton->setEnabled( false );
  upFieldButton->setEnabled( false );
  downFieldButton->setEnabled( false );
  resetOptionsButton->setEnabled( false );
  minusButton->setEnabled( false );

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
  repeatCheckBox->
  setChecked(false);
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

  lineintervals  = numberList(lineintervalCbox, 10, true);
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

  //hour.offset and hour.diff are not plotOptions and signals must be blocked
  //in order not to change the selectedField values of hour.offset and hour.diff
  hourOffsetSpinBox->blockSignals(true);
  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->blockSignals(false);
  hourDiffSpinBox->blockSignals(true);
  hourDiffSpinBox->setValue(0);
  hourDiffSpinBox->blockSignals(false);
}

void FieldDialog::enableWidgets(const std::string& plottype)
{
  METLIBS_LOG_SCOPE("plottype="<<plottype);

  bool enable = (plottype != "none");

  //used for all plottypes
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

vector<std::string> FieldDialog::numberList(QComboBox* cBox, float number, bool onoff)
{
  const float enormal[] = {
    1., 1.5,2., 2.5, 3.,3.5, 4.,4.5, 5.,5.5,
    6.,6.5, 7.,7.5, 8.,8.5, 9.,9.5, -1
  };
  return diutil::numberList(cBox, number, enormal, onoff);
}

void FieldDialog::baseList(QComboBox* cBox, float base, bool onoff)
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

void FieldDialog::selectedFieldboxClicked(QListWidgetItem * item)
{
  int index = selectedFieldbox->row(item);

  // may get here when there is none selected fields (the last is removed)
  if (index < 0 || selectedFields.size() == 0)
    return;

  enableFieldOptions();
}

void FieldDialog::unitEditingFinished()
{
  updateFieldOptions(UNIT, REMOVE);
  updateFieldOptions(UNITS, unitLineEdit->text().toStdString());
}

void FieldDialog::plottypeComboBoxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_plottype, plottypes[index]);
  enableWidgets(plottypes[index]);
}

void FieldDialog::colorCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_colour, "off");
  else
    updateFieldOptions(PlotOptions::key_colour,colorCbox->currentText().toStdString());
}

void FieldDialog::lineWidthCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(index + 1));
}

void FieldDialog::lineTypeCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_linetype, linetypes[index]);
}

void FieldDialog::lineintervalCboxActivated(int index)
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

void FieldDialog::densityCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_density, "0");
  else
    updateFieldOptions(PlotOptions::key_density, densityCbox->currentText().toStdString());
}

void FieldDialog::vectorunitCboxActivated(int index)
{
  updateFieldOptions(PlotOptions::key_vectorunit, vectorunit[index]);
  // update the list (with selected value in the middle)
  float a = miutil::to_float(vectorunit[index]);
  vectorunit = numberList(vectorunitCbox, a, false);
}

void FieldDialog::extremeTypeActivated(int index)
{
  updateFieldOptions(PlotOptions::key_extremeType, extremeType[index]);
}

void FieldDialog::extremeSizeChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_extremeSize, str);
}

void FieldDialog::extremeRadiusChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_extremeRadius, str);
}

void FieldDialog::lineSmoothChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_lineSmooth, str);
}

void FieldDialog::fieldSmoothChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_fieldSmooth, str);
}

void FieldDialog::labelSizeChanged(int value)
{
  std::string str = miutil::from_number(float(value) * 0.01);
  updateFieldOptions(PlotOptions::key_labelSize, str);
}

void FieldDialog::valuePrecisionBoxActivated( int index )
{
  std::string str = miutil::from_number(index);
  updateFieldOptions(PlotOptions::key_precision, str);
}


void FieldDialog::gridValueCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_gridValue, "1");
  else
    updateFieldOptions(PlotOptions::key_gridValue, "0");
}

void FieldDialog::gridLinesChanged(int value)
{
  std::string str = miutil::from_number(value);
  updateFieldOptions(PlotOptions::key_gridLines, str);
}

// void FieldDialog::gridLinesMaxChanged(int value)
// {
//   std::string str= miutil::from_number( value );
//   updateFieldOptions("grid.lines.max",str);
// }


void FieldDialog::hourOffsetChanged(int value)
{
  int n = selectedFieldbox->currentRow();
  selectedFields[n].hourOffset = value;
  updateTime();
}

void FieldDialog::hourDiffChanged(int value)
{
  int n = selectedFieldbox->currentRow();
  selectedFields[n].hourDiff = value;
  updateTime();
}

void FieldDialog::undefMaskingActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefMasking, miutil::from_number(index));
  undefColourCbox->setEnabled(index > 0);
  undefLinewidthCbox->setEnabled(index > 1);
  undefLinetypeCbox->setEnabled(index > 1);
}

void FieldDialog::undefColourActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefColour, undefColourCbox->currentText().toStdString());
}

void FieldDialog::undefLinewidthActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefLinewidth, miutil::from_number(index + 1));
}

void FieldDialog::undefLinetypeActivated(int index)
{
  updateFieldOptions(PlotOptions::key_undefLinetype, linetypes[index]);
}

void FieldDialog::frameCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_frame, "1");
  else
    updateFieldOptions(PlotOptions::key_frame, "0");
}

void FieldDialog::zeroLineCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_zeroLine, "1");
  else
    updateFieldOptions(PlotOptions::key_zeroLine, "0");
}

void FieldDialog::valueLabelCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_valueLabel, "1");
  else
    updateFieldOptions(PlotOptions::key_valueLabel, "0");
}

void FieldDialog::colour2ComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_colour_2, "off");
    enableType2Options(false);
    colour2ComboBox->setEnabled(true);
  } else {
    updateFieldOptions(PlotOptions::key_colour_2, colour2ComboBox->currentText().toStdString());
    enableType2Options(true); //check if needed
    //turn of 3 colours (not possible to combine threeCols and col_2)
    for (int i=0; i<3; ++i)
      threeColourBox[i]->setCurrentIndex(0);
    threeColoursChanged();
  }
}

void FieldDialog::tableCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_table, "1");
  else
    updateFieldOptions(PlotOptions::key_table, "0");
}

void FieldDialog::patternComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_patterns, "off");
  } else {
    updateFieldOptions(PlotOptions::key_patterns, patternInfo[index - 1].name);
  }
  updatePaletteString();
}

void FieldDialog::patternColourBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions(PlotOptions::key_pcolour, REMOVE);
  } else {
    updateFieldOptions(PlotOptions::key_pcolour, patternColourBox->currentText().toStdString());
  }
  updatePaletteString();
}

void FieldDialog::repeatCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions(PlotOptions::key_repeat, "1");
  else
    updateFieldOptions(PlotOptions::key_repeat, "0");
}

void FieldDialog::threeColoursChanged()
{
  if (threeColourBox[0]->currentIndex() == 0
      || threeColourBox[1]->currentIndex() == 0
      || threeColourBox[2]->currentIndex() == 0) {

    updateFieldOptions(PlotOptions::key_colours, REMOVE);

  } else {

    //turn of colour_2 (not possible to combine threeCols and col_2)
    colour2ComboBox->setCurrentIndex(0);
    colour2ComboBoxToggled(0);

    std::string str = threeColourBox[0]->currentText().toStdString() + ","
        + threeColourBox[1]->currentText().toStdString() + ","
        + threeColourBox[2]->currentText().toStdString();

    updateFieldOptions(PlotOptions::key_colours, REMOVE);
    updateFieldOptions(PlotOptions::key_colours, str);
  }
}

void FieldDialog::shadingChanged()
{
  updatePaletteString();
}

void FieldDialog::updatePaletteString()
{
  if (patternComboBox->currentIndex() > 0 && patternColourBox->currentIndex()
      > 0) {
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

void FieldDialog::alphaChanged(int index)
{
  updateFieldOptions(PlotOptions::key_alpha, miutil::from_number(index));
}

void FieldDialog::interval2ComboBoxToggled(int index)
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

void FieldDialog::zero1ComboBoxToggled(int)
{
  if (!zero1ComboBox->currentText().isNull()) {
    baseList(zero1ComboBox, zero1ComboBox->currentText().toFloat());
    updateFieldOptions(PlotOptions::key_basevalue, zero1ComboBox->currentText().toStdString());
  }
}

void FieldDialog::zero2ComboBoxToggled(int)
{
  if (!zero2ComboBox->currentText().isNull()) {
    const float a = zero2ComboBox->currentText().toFloat();
    updateFieldOptions(PlotOptions::key_basevalue_2, zero2ComboBox->currentText().toStdString());
    baseList(zero2ComboBox, a);
  }
}

void FieldDialog::min1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_minvalue, "off");
  else if (!min1ComboBox->currentText().isNull()) {
    baseList(min1ComboBox, min1ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_minvalue, min1ComboBox->currentText().toStdString());
  }
}

void FieldDialog::max1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_maxvalue, "off");
  else if (!max1ComboBox->currentText().isNull()) {
    baseList(max1ComboBox, max1ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_maxvalue, max1ComboBox->currentText().toStdString());
  }
}

void FieldDialog::min2ComboBoxToggled(int index)
{

  if (index == 0)
    updateFieldOptions(PlotOptions::key_minvalue_2, REMOVE);
  else if (!min2ComboBox->currentText().isNull()) {
    baseList(min2ComboBox, min2ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_minvalue_2, min2ComboBox->currentText().toStdString());
  }
}

void FieldDialog::max2ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions(PlotOptions::key_maxvalue_2, REMOVE);
  else if (!max2ComboBox->currentText().isNull()) {
    baseList(max2ComboBox, max2ComboBox->currentText().toFloat(), true);
    updateFieldOptions(PlotOptions::key_maxvalue_2, max2ComboBox->currentText().toStdString());
  }
}

void FieldDialog::linevaluesFieldEdited()
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

void FieldDialog::linevaluesLogCheckBoxToggled(bool)
{
  linevaluesFieldEdited();
}

void FieldDialog::linewidth1ComboBoxToggled(int index)
{
  lineWidthCbox->setCurrentIndex(index);
  updateFieldOptions(PlotOptions::key_linewidth, miutil::from_number(index + 1));
}

void FieldDialog::linewidth2ComboBoxToggled(int index)
{
  updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(index + 1));
}

void FieldDialog::linetype1ComboBoxToggled(int index)
{
  lineTypeCbox->setCurrentIndex(index);
  updateFieldOptions(PlotOptions::key_linetype, linetypes[index]);
}

void FieldDialog::linetype2ComboBoxToggled(int index)
{
  updateFieldOptions(PlotOptions::key_linetype_2, linetypes[index]);
}

void FieldDialog::enableType2Options(bool on)
{
  colour2ComboBox->setEnabled(on);

  //enable the rest only if colour2 is on
  on = (colour2ComboBox->currentIndex() != 0);

  interval2ComboBox->setEnabled(on);
  zero2ComboBox->setEnabled(on);
  min2ComboBox->setEnabled(on);
  max2ComboBox->setEnabled(on);
  linewidth2ComboBox->setEnabled(on);
  linetype2ComboBox->setEnabled(on);

  if (on) {
    if (!interval2ComboBox->currentText().isNull())
      updateFieldOptions(PlotOptions::key_lineinterval_2,
          interval2ComboBox->currentText().toStdString());
    if (!zero2ComboBox->currentText().isNull())
      updateFieldOptions(PlotOptions::key_basevalue_2, zero2ComboBox->currentText().toStdString());
    if (!min2ComboBox->currentText().isNull() && min2ComboBox->currentIndex()
        > 0)
      updateFieldOptions(PlotOptions::key_minvalue_2,
          min2ComboBox->currentText().toStdString());
    if (!max2ComboBox->currentText().isNull() && max2ComboBox->currentIndex()
        > 0)
      updateFieldOptions(PlotOptions::key_maxvalue_2,
          max2ComboBox->currentText().toStdString());
    updateFieldOptions(PlotOptions::key_linewidth_2, miutil::from_number(
        linewidth2ComboBox->currentIndex() + 1));
    updateFieldOptions(PlotOptions::key_linetype_2,
        linetypes[linetype2ComboBox->currentIndex()]);
  } else {
    colour2ComboBox->setCurrentIndex(0);
    updateFieldOptions(PlotOptions::key_colour_2, "off");
    updateFieldOptions(PlotOptions::key_lineinterval_2, REMOVE);
    updateFieldOptions(PlotOptions::key_basevalue_2, REMOVE);
    updateFieldOptions(PlotOptions::key_linewidth_2, REMOVE);
    updateFieldOptions(PlotOptions::key_linetype_2, REMOVE);
  }
}

void FieldDialog::updateFieldOptions(const std::string& name, const std::string& value)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(value));

  if (currentFieldOpts.empty())
    return;

  int n = selectedFieldbox->currentRow();

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
  selectedFields[n].fieldOpts = currentFieldOpts;

  // not update private settings if external/QuickMenu command...
  if (!selectedFields[n].external) {
    fieldOptions[selectedFields[n].fieldName] = currentFieldOpts;
  }
}

void FieldDialog::getFieldGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfg)
{
  METLIBS_LOG_TIME(LOGVAL(modelName));

  { diutil::OverrideCursor waitCursor;
    m_data->getFieldPlotGroups(modelName, refTime, predefinedPlots, vfg);
    QString tooltip;
    const auto global_attributes = m_data->getFieldGlobalAttributes(modelName, refTime);
    for (const char* const* mga : modelGlobalAttributes) {
      const auto ita = global_attributes.find(mga[0]);
      if (ita != global_attributes.end()) {
        tooltip += QString("<tr><td>%1</td><td>%2</td></tr>").arg(tr(mga[1]), QString::fromStdString(ita->second));
      }
    }
    if (!tooltip.isEmpty())
      tooltip = "<table>" + tooltip + "</table>";
    refTimeComboBox->setToolTip(tooltip);
  }
}

PlotCommand_cpv FieldDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv vstr;
  if (selectedFields.empty())
    return vstr;

  for (SelectedField& sf : selectedFields) {
    sf.levelmove = true;
    sf.idnummove = true;
  }

  const int n = selectedFields.size();
  vstr.reserve(n);
  for (int i = 0; i < n; i++) {
    const SelectedField& sf = selectedFields[i];
    if (sf.minus)
      continue;

    FieldPlotCommand_p cmd = std::make_shared<FieldPlotCommand>(sf.inEdit);

    const bool minus = (i + 1 < n && selectedFields[i + 1].minus);
    getParamString(sf, cmd->field);
    if (minus)
      getParamString(selectedFields[i + 1], cmd->minus);

    cmd->addOptions(sf.fieldOpts);
    cmd->time = selectedFields[i].time;

    vstr.push_back(cmd);
    METLIBS_LOG_DEBUG("OK: " << cmd->toString());
  }

  return vstr;
}

void FieldDialog::getParamString(const SelectedField& sf, FieldPlotCommand::FieldSpec& fs)
{
  fs.model = (sf.inEdit) ? editName : sf.modelName;
  fs.reftime = sf.refTime;
  if (sf.predefinedPlot)
    fs.plot = sf.fieldName;
  else
    fs.parameters = std::vector<std::string>(1, sf.fieldName);

  if (!sf.level.empty()) {
    fs.vcoord = sf.zaxis;
    fs.vlevel = sf.level;
  }
  fs.elevel = sf.idnum;

  fs.hourOffset = sf.hourOffset;
  fs.hourDiff = sf.hourDiff;
  fs.allTimeSteps = allTimeStepButton->isChecked();
}

std::string FieldDialog::getShortname()
{
  // AC: simple version for testing...the shortname could perhaps
  // be made in getOKString?

  std::string name;
  int n = selectedFields.size();
  ostringstream ostr;
  std::string pmodelName;
  bool fielddiff = false, paramdiff = false, leveldiff = false;

  for (int i = numEditFields; i < n; i++) {
    std::string modelName = selectedFields[i].modelName;
    std::string fieldName = selectedFields[i].fieldName;
    std::string level = selectedFields[i].level;
    std::string idnum = selectedFields[i].idnum;

    //difference field
    if (i < n - 1 && selectedFields[i + 1].minus) {

      std::string modelName2 = selectedFields[i + 1].modelName;
      std::string fieldName2 = selectedFields[i + 1].fieldName;
      std::string level_2 = selectedFields[i + 1].level;
      std::string idnum_2 = selectedFields[i + 1].idnum;

      fielddiff = (modelName != modelName2);
      paramdiff = (fieldName != fieldName2);
      leveldiff = (level.empty() || level != level_2 || idnum != idnum_2);

      if (modelName != pmodelName || modelName2 != pmodelName) {

        if (i > numEditFields)
          ostr << "  ";
        if (fielddiff)
          ostr << "( ";
        ostr << modelName;
        pmodelName = modelName;
      }

      if (!fielddiff && paramdiff)
        ostr << " ( ";
      if (!fielddiff || (fielddiff && paramdiff) || (fielddiff && leveldiff))
        ostr << " " << fieldName;

      if (!selectedFields[i].level.empty()) {
        if (!fielddiff && !paramdiff)
          ostr << " ( " << selectedFields[i].level;
        else if ((fielddiff || paramdiff) && leveldiff)
          ostr << " " << selectedFields[i].level;
      }
      if (!selectedFields[i].idnum.empty() && leveldiff)
        ostr << " " << selectedFields[i].idnum;

    } else if (selectedFields[i].minus) {
      ostr << " - ";

      if (fielddiff) {
        ostr << modelName;
        pmodelName.clear();
      }

      if (fielddiff && !paramdiff && !leveldiff)
        ostr << " ) " << fieldName;
      else if (paramdiff || (fielddiff && leveldiff))
        ostr << " " << fieldName;

      if (!selectedFields[i].level.empty()) {
        if (!leveldiff && paramdiff)
          ostr << " )";
        ostr << " " << selectedFields[i].level;
        if (!selectedFields[i].idnum.empty())
          ostr << " " << selectedFields[i].idnum;
        if (leveldiff)
          ostr << " )";
      } else {
        ostr << " )";
      }

      fielddiff = paramdiff = leveldiff = false;

    } else { // Ordinary field

      if (i > numEditFields)
        ostr << "  ";

      if (modelName != pmodelName) {
        ostr << modelName;
        pmodelName = modelName;
      }

      ostr << " " << fieldName;

      if (!selectedFields[i].level.empty())
        ostr << " " << selectedFields[i].level;
      if (!selectedFields[i].idnum.empty())
        ostr << " " << selectedFields[i].idnum;
    }
  }

  if (n > 0)
    name = "<font color=\"#000099\">" + ostr.str() + "</font>";

  return name;
}

bool FieldDialog::levelsExists(bool up, int type)
{

  //returns true if there exist plots with levels available: up/down of type (0=vertical/ 1=extra/eps)

  int n = selectedFields.size();

  if ( type == 0 ) {

    int i = 0;
    while ( i < n && selectedFields[i].levelOptions.size() <2 ) i++;
    if( i == n ) {
      return false;
    } else {
      int m = selectedFields[i].levelOptions.size();
      if ( up ) {
        return ( selectedFields[i].level != selectedFields[i].levelOptions[0]);
      } else {
        return ( selectedFields[i].level != selectedFields[i].levelOptions[m-1]);
      }
    }

  } else {

    int i = 0;
    while ( i < n && selectedFields[i].idnumOptions.size() <2 ) i++;
    if( i == n ) {
      return false;
    } else {
      int m = selectedFields[i].idnumOptions.size();
      if ( up ) {
        return ( selectedFields[i].idnum != selectedFields[i].idnumOptions[m-1]);
      } else {
        return ( selectedFields[i].idnum != selectedFields[i].idnumOptions[0]);
      }
    }
  }

  return false;
}

void FieldDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();

  deleteAllSelected();
  allTimeStepButton->setChecked(true);

  if (vstr.empty()) {
    updateTime();
    return;
  }

  for (PlotCommand_cp pc : vstr) {
    FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(pc);
    if (!cmd)
      continue;

    SelectedField sf;
    sf.external = true;
    if (decodeCommand(cmd, cmd->field, sf))
      addSelectedField(sf);

    if (cmd->hasMinusField()) {
      SelectedField sfsubtract;
      sfsubtract.external = true;
      if (decodeCommand(cmd, cmd->minus, sfsubtract)) {
        addSelectedField(sfsubtract);
        minusField(true);
      }
    }
  }

  int m = selectedFields.size();

  if (m > 0) {
    selectedFieldbox->setCurrentRow(0);
    selectedFieldbox->item(0)->setSelected(true);
    enableFieldOptions();
  }

  updateTime();
}

void FieldDialog::addSelectedField(const SelectedField& sf)
{
  selectedFields.push_back(sf);

  std::string text = sf.modelName + " " + sf.fieldName + " " + sf.refTime;
  selectedFieldbox->addItem(QString::fromStdString(text));

  selectedFieldbox->setCurrentRow(selectedFieldbox->count() - 1);
  selectedFieldbox->item(selectedFieldbox->count() - 1)->setSelected(true);
}

bool FieldDialog::decodeCommand(FieldPlotCommand_cp cmd, const FieldPlotCommand::FieldSpec& fs, SelectedField& sf)
{
  sf.inEdit = cmd->isEdit;

  sf.modelName = fs.model;
  sf.fieldName = fs.name();
  if (sf.fieldName.empty())
    return false;
  sf.predefinedPlot = fs.isPredefinedPlot();
  sf.refTime = fs.reftime;
  if (sf.refTime.empty())
    sf.refTime = m_data->getBestFieldReferenceTime(sf.modelName, fs.refoffset, fs.refhour);

  sf.zaxis = fs.vcoord;
  sf.level = fs.vlevel;
  sf.extraaxis = fs.ecoord;
  sf.idnum = fs.elevel;

  sf.hourOffset = fs.hourOffset;
  sf.hourDiff = fs.hourDiff;

  sf.fieldOpts = cmd->options();

  if (!fs.allTimeSteps)
      allTimeStepButton->setChecked(false);

  // merge with options from setup/logfile for this fieldname
  mergeFieldOptions(sf.fieldOpts, getFieldOptions(fs.name(), true));


  METLIBS_LOG_DEBUG(LOGVAL(sf.modelName) << LOGVAL(sf.fieldName) << LOGVAL(sf.level) << LOGVAL(sf.idnum));

  FieldPlotGroupInfo_v vfg;
  getFieldGroups(sf.modelName, sf.refTime, sf.predefinedPlot, vfg);

  //find index of fieldgroup
  const FieldPlotInfo* fi_found = 0;
  for (FieldPlotGroupInfo& fgi : vfg) {
    const FieldPlotInfo_v& fgip = fgi.plots;
    FieldPlotInfo_v::const_iterator it = std::find_if(fgip.begin(), fgip.end(), [&](const FieldPlotInfo& f) { return f.fieldName == sf.fieldName; });
    if (it == fgip.end())
      continue;
    const FieldPlotInfo& p = *it;
    if ((sf.zaxis == p.vcoord() || (sf.zaxis.empty() && p.vlevels().size() == 1))) {
      fi_found = &(*it);
      // if there are elevels, but the elevel isn't specified, use first elevel
      // if there are no elevels, but an elevel is specified, remove it.
      if (sf.idnum.empty() && !p.elevels().empty()) {
        sf.idnum = p.elevels()[0];
      } else if (!sf.idnum.empty() && p.elevels().empty()) {
        sf.idnum.clear();
      }
      break;
    }
  }

  if (!fi_found) {
    METLIBS_LOG_DEBUG("Field not found: " << LOGVAL(fs.plot));
    return false;
  }

  sf.levelOptions = fi_found->vlevels();
  sf.idnumOptions = fi_found->elevels();
  sf.minus = false;
  return true;
}

inline std::string sub(const std::string& s, std::string::size_type begin, std::string::size_type end)
{
  return s.substr(begin, end - begin);
}

vector<std::string> FieldDialog::writeLog()
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

  //write edit field options
  if (editFieldOptions.size() > 0) {
    vstr.push_back("--- EDIT ---");
    pfend = editFieldOptions.end();
    for (pfopt = editFieldOptions.begin(); pfopt != pfend; pfopt++) {
      miutil::KeyValue_v sopts = getFieldOptions(pfopt->first, true);
      // only logging options if different from setup
      if (sopts != pfopt->second) {
        vstr.push_back(pfopt->first + " " + miutil::mergeKeyValue(pfopt->second));
      }
    }
  }

  vstr.push_back("================");

  return vstr;
}

void FieldDialog::readLog(const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion)
{

  // field options:
  // do not destroy any new options in the program
  bool editOptions = false;
  for (const std::string& ls : vstr) {
    if (ls.empty())
      continue;
    if (ls.substr(0, 4) == "====")
      break;
    if (ls == "--- EDIT ---") {
      editOptions = true;
      continue;
    }
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
        if (editOptions) {
          editFieldOptions[fieldname] = setup_opts;
        } else {
          fieldOptions[fieldname] = setup_opts;
        }
      }
    }
  }
}

void FieldDialog::deleteSelected()
{
  METLIBS_LOG_SCOPE();

  int index = selectedFieldbox->currentRow();

  int ns = selectedFields.size() - 1;

  if (index < 0 || index > ns)
    return;
  if (selectedFields[index].inEdit)
    return;

  int indexF = -1;

  if (vfgi.size() > 0) {
    int n = fieldbox->count();
    int i = 0;
    while (i < n && fieldbox->item(i)->text().toStdString()
        != selectedFields[index].fieldName)
      i++;
    if (i < n)
      indexF = i;
  }


  if (indexF >= 0) {
    fieldbox->item(indexF)->setSelected(false);
  }
  selectedFieldbox->takeItem(index);
  for (int i = index; i < ns; i++) {
    selectedFields[i] = selectedFields[i + 1];
  }
  selectedFields.pop_back();

  if (selectedFields.size() > 0) {
    if (index >= int(selectedFields.size()))
      index = selectedFields.size() - 1;
    selectedFieldbox->setCurrentRow(index);
    selectedFieldbox->item(index)->setSelected(true);
    enableFieldOptions();
  } else {
    setDefaultFieldOptions();
    enableWidgets("none");
    setLevel();
    setIdnum();
  }

  updateTime();

  //first field can't be minus
  if (selectedFieldbox->count() > 0 && selectedFields[0].minus)
    minusButton->setChecked(false);
}

void FieldDialog::deleteAllSelected()
{
  METLIBS_LOG_SCOPE();

  int n = fieldbox->count();

  if (n > 0) {
    fieldbox->blockSignals(true);
    fieldbox->clearSelection();
    fieldbox->blockSignals(false);
  }

  selectedFields.resize(numEditFields);
  selectedFieldbox->clear();
  minusButton->setChecked(false);
  minusButton->setEnabled(false);

  if (numEditFields > 0) {
    // show edit fields
    for (int i = 0; i < numEditFields; i++) {
      std::string str = editName + " " + selectedFields[i].fieldName + " " + selectedFields[i].refTime;
      selectedFieldbox->addItem(QString::fromStdString(str));
    }
    selectedFieldbox->setCurrentRow(0);
    selectedFieldbox->item(0)->setSelected(true);
    enableFieldOptions();
  } else {
    setDefaultFieldOptions();
    enableWidgets("none");
    setLevel();
    setIdnum();
  }

  updateTime();
}

void FieldDialog::copySelectedField()
{
  METLIBS_LOG_SCOPE();

  if (selectedFieldbox->count() == 0)
    return;

  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();


  selectedFields.push_back(selectedFields[index]);
  selectedFields[n].hourOffset = 0;

  selectedFieldbox->addItem(selectedFieldbox->item(index)->text());
  selectedFieldbox->setCurrentRow(n);
  selectedFieldbox->item(n)->setSelected(true);
  enableFieldOptions();
}

void FieldDialog::changeModel()
{
  METLIBS_LOG_SCOPE();

  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n)
    return;

  const QModelIndexList selected = modelbox->selectionModel()->selectedIndexes();
  if (selected.count() != 1 || currentModel.empty())
    return;

  int indexFGR = fieldGRbox->currentIndex();
  if (indexFGR < 0 || indexFGR >= int(vfgi.size()))
    return;

  std::string oldModel = selectedFields[index].modelName;
  std::string oldRefTime = selectedFields[index].refTime;
  std::string newModel = currentModel;
  std::string newRefTime = refTimeComboBox->currentText().toStdString();
  if ( (oldModel == newModel) && (oldRefTime == newRefTime) )
    return;

  fieldbox->blockSignals(true);

  int nvfgi = vfgi.size();

  for (int i = 0; i < n; i++) {
    std::string selectedModel = selectedFields[i].modelName;
    std::string selectedRefTime = selectedFields[i].refTime;
    if ( (selectedModel == oldModel) && (selectedRefTime == oldRefTime) ) {
      // check if field exists for the new model
      int gbest = -1,fbest = -1;
      for ( int j=0; j < nvfgi;++j) {

        const int m = vfgi[j].plots.size();
        int k = 0;
        while (k < m && !(vfgi[j].plots[k].fieldName == selectedFields[i].fieldName)) {
          k++;
        }
        // Check if parameters have same vcoord, ignore if no. levels == 0,1
        if (k < m && ((vfgi[j].plots[k].vcoord() == selectedFields[i].zaxis) || selectedFields[i].levelOptions.size() < 2)) {
          gbest = j;
          fbest = k;
          break;
        }
      }
      if ( gbest > -1 ) {
        if (indexFGR == gbest) {
          if (fbest > 0 && fbest < fieldbox->count()) {
            fieldbox->setCurrentRow(fbest);
            fieldbox->item(fbest)->setSelected(true);
          }
        }

        const FieldPlotInfo& fi_best = vfgi[gbest].plots[fbest];
        selectedFields[i].modelName = newModel;
        selectedFields[i].refTime = newRefTime;
        selectedFields[i].levelOptions = fi_best.vlevels();
        selectedFields[i].idnumOptions = fi_best.elevels();
        selectedFields[i].predefinedPlot = predefinedPlotsCheckBox->isChecked();

        std::string str = selectedFields[i].modelName + " " + selectedFields[i].fieldName + " " + selectedFields[i].refTime;
        selectedFieldbox->item(i)->setText(QString::fromStdString(str));
      }
    }
  }

  fieldbox->blockSignals(false);

  selectedFieldbox->setCurrentRow(index);
  selectedFieldbox->item(index)->setSelected(true);
  enableFieldOptions();

  updateTime();
}

void FieldDialog::upField()
{
  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 1 || index >= n)
    return;

  SelectedField sf = selectedFields[index];
  selectedFields[index] = selectedFields[index - 1];
  selectedFields[index - 1] = sf;

  QString qstr1 = selectedFieldbox->item(index - 1)->text();
  QString qstr2 = selectedFieldbox->item(index)->text();
  selectedFieldbox->item(index - 1)->setText(qstr2);
  selectedFieldbox->item(index)->setText(qstr1);

  //some fields can't be minus
  for (int i = 0; i < n; i++) {
    selectedFieldbox->setCurrentRow(i);
    if (selectedFields[i].minus && (i == 0 || selectedFields[i - 1].minus))
      minusButton->setChecked(false);
  }

  index--;
  selectedFieldbox->setCurrentRow(index);
  upFieldButton->setEnabled((index > numEditFields));
  downFieldButton->setEnabled((index < (n-1)));
}

void FieldDialog::downField()
{
  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n - 1)
    return;

  SelectedField sf = selectedFields[index];
  selectedFields[index] = selectedFields[index + 1];
  selectedFields[index + 1] = sf;

  QString qstr1 = selectedFieldbox->item(index)->text();
  QString qstr2 = selectedFieldbox->item(index + 1)->text();
  selectedFieldbox->item(index)->setText(qstr2);
  selectedFieldbox->item(index + 1)->setText(qstr1);

  //some fields can't be minus
  for (int i = 0; i < n; i++) {
    selectedFieldbox->setCurrentRow(i);
    if (selectedFields[i].minus && (i == 0 || selectedFields[i - 1].minus))
      minusButton->setChecked(false);
  }

  index++;
  selectedFieldbox->setCurrentRow(index);
  upFieldButton->setEnabled((index > numEditFields));
  downFieldButton->setEnabled((index < (n-1)));
}

void FieldDialog::resetOptions()
{
  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n)
    return;

  SelectedField& sf = selectedFields[index];
  const miutil::KeyValue_v fopts = getFieldOptions(sf.fieldName, true);
  if (fopts.empty())
    return;

  sf.fieldOpts = fopts;
  sf.hourOffset = 0;
  sf.hourDiff = 0;
  enableWidgets("none");
  currentFieldOpts.clear();
  enableFieldOptions();
}

miutil::KeyValue_v FieldDialog::getFieldOptions(const std::string& fieldName, bool reset) const
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

  //default
  PlotOptions po;
  return po.toKeyValueList();
}

void FieldDialog::minusField(bool on)
{
  int i = selectedFieldbox->currentRow();

  if (i < 0 || i >= selectedFieldbox->count())
    return;

  QString qstr = selectedFieldbox->currentItem()->text();

  if (on) {
    if (!selectedFields[i].minus) {
      selectedFields[i].minus = true;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->item(i)->setText("  -  " + qstr);
      selectedFieldbox->blockSignals(false);
    }
    enableWidgets("none");
    //next field can't be minus
    if (selectedFieldbox->count() > i + 1 && selectedFields[i + 1].minus) {
      selectedFieldbox->setCurrentRow(i + 1);
      minusButton->setChecked(false);
      selectedFieldbox->setCurrentRow(i);
    }
  } else {
    if (selectedFields[i].minus) {
      selectedFields[i].minus = false;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->item(i)->setText(qstr.remove(0, 5));
      selectedFieldbox->blockSignals(false);
      currentFieldOpts.clear();
      enableFieldOptions();
    }
  }
}

void FieldDialog::updateTime()
{
  METLIBS_LOG_SCOPE();
  plottimes_t fieldtime;

  std::vector<FieldRequest> request;
  for (const SelectedField& sf : selectedFields) {
    if (!sf.inEdit) {
      request.push_back(FieldRequest());
      FieldRequest& fr = request.back();
      fr.modelName = sf.modelName;
      fr.paramName = sf.fieldName;
      fr.plevel = sf.level;
      fr.elevel = sf.idnum;
      fr.hourOffset = sf.hourOffset;
      fr.refTime = sf.refTime;
      fr.zaxis = sf.zaxis;
      fr.eaxis = sf.extraaxis;
      fr.predefinedPlot = sf.predefinedPlot;
      fr.allTimeSteps = allTimeStepButton->isChecked();
    }
  }

  if (!request.empty()) {
    diutil::insert_all(fieldtime, m_data->getFieldTime(request));
  }

  METLIBS_LOG_DEBUG(LOGVAL(fieldtime.size()));
  emitTimes(fieldtime);
}

void FieldDialog::fieldEditUpdate(std::string str)
{
  METLIBS_LOG_SCOPE();
  if (str.empty())
    METLIBS_LOG_DEBUG("STOP");
  else
    METLIBS_LOG_DEBUG("START "<<str);

  int i, j, m, n = selectedFields.size();

  if (str.empty()) {

    // remove fixed edit field(s)
    vector<int> keep;
    m = selectedField2edit_exists.size();
    bool change = false;
    for (i = 0; i < n; i++) {
      if (!selectedFields[i].inEdit) {
        keep.push_back(i);
      } else if (i < m && selectedField2edit_exists[i]) {
        selectedFields[i] = selectedField2edit[i];
        std::string text = selectedFields[i].modelName + " " + selectedFields[i].fieldName;
        selectedFieldbox->item(i)->setText(QString::fromStdString(text));
        keep.push_back(i);
        change = true;
      }
    }
    m = keep.size();
    if (m < n) {
      for (i = 0; i < m; i++) {
        j = keep[i];
        selectedFields[i] = selectedFields[j];
        QString qstr = selectedFieldbox->item(j)->text();
        selectedFieldbox->item(i)->setText(qstr);
      }
      selectedFields.resize(m);
      if (m == 0) {
        selectedFieldbox->clear();
      } else {
        for (i = m; i < n; i++)
          selectedFieldbox->takeItem(i);
      }
    }

    numEditFields = 0;
    selectedField2edit.clear();
    selectedField2edit_exists.clear();
    if (change) {
      updateTime();

      METLIBS_LOG_DEBUG("FieldDialog::fieldEditUpdate emit FieldApply");

      Q_EMIT applyData();
    }

  } else {

    // add edit field (and remove the original field)
    int indrm = -1;
    SelectedField sf;

    vector<std::string> vstr = miutil::split(str, " ");

    if (vstr.size() == 1) {
      // str=fieldName if the field is not already read
      sf.fieldName = vstr[0];
    } else if (vstr.size() > 1)    {
      // str="modelname fieldName level" if field from dialog is selected
      sf.modelName = vstr[0];
      sf.fieldName = vstr[1];
      for (i = 0; i < n; i++) {
        if (!selectedFields[i].inEdit) {
          if (selectedFields[i].modelName == sf.modelName
              && selectedFields[i].fieldName == sf.fieldName)
            break;
        }
      }
      if (i < n) {
        sf = selectedFields[i];
        indrm = i;
      }
    }

    map<std::string, miutil::KeyValue_v>::const_iterator pfo;
    if ((pfo = editFieldOptions.find(sf.fieldName)) != editFieldOptions.end()) {
      sf.fieldOpts = pfo->second;
    } else if ((pfo = fieldOptions.find(sf.fieldName)) != fieldOptions.end()) {
      sf.fieldOpts = pfo->second;
    } else if ((pfo = setupFieldOptions.find(sf.fieldName)) != setupFieldOptions.end()) {
      sf.fieldOpts = pfo->second;
    }
    METLIBS_LOG_DEBUG(LOGVAL(sf.fieldOpts));

    sf.inEdit = true;
    sf.external = false;
    sf.hourOffset = 0;
    sf.hourDiff = 0;
    sf.minus = false;
    if (indrm >= 0) {
      selectedField2edit.push_back(selectedFields[indrm]);
      selectedField2edit_exists.push_back(true);
      n = selectedFields.size();
      for (i = indrm; i < n - 1; i++)
        selectedFields[i] = selectedFields[i + 1];
      selectedFields.pop_back();
      selectedFieldbox->takeItem(indrm);
    } else {
      SelectedField sfdummy;
      selectedField2edit.push_back(sfdummy);
      selectedField2edit_exists.push_back(false);
    }

    const size_t idx_field_smooth = miutil::find(sf.fieldOpts, "field.smooth");
    if (idx_field_smooth != npos)
      sf.fieldOpts[idx_field_smooth] = miutil::KeyValue("field.smooth", "0");

    n = selectedFields.size();
    SelectedField sfdummy;
    selectedFields.push_back(sfdummy);
    for (i = n; i > numEditFields; i--)
      selectedFields[i] = selectedFields[i - 1];
    selectedFields[numEditFields] = sf;

    std::string text = editName + " " + sf.fieldName;
    selectedFieldbox->insertItem(numEditFields, QString::fromStdString(text));
    selectedFieldbox->setCurrentRow(numEditFields);
    numEditFields++;

    updateTime();
  }

  if (!selectedFields.empty())
    enableFieldOptions();
  else
    enableWidgets("none");
}

void FieldDialog::allTimeStepToggled(bool)
{
  updateTime();
}
