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

#include "qtFieldDialog.h"

#include "diFieldDialogData.h"
#include "diFieldUtil.h"
#include "diPlotOptions.h"
#include "qtFieldDialogAdd.h"
#include "qtFieldDialogStyle.h"
#include "qtStringSliderControl.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include "util/string_util.h"

#include <QAction>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QVBoxLayout>

#include <puTools/miStringFunctions.h>

#include <mi_fieldcalc/math_util.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldDialog"
#include <miLogger/miLogging.h>

#include "down12x12.xpm"
#include "felt.xpm"
#include "up12x12.xpm"

//#define DEBUGPRINT


namespace { // anonymous

const std::string REMOVE = "remove";
const std::string UNITS = "units";
const std::string UNIT = "unit";

const QString MINUSPREFIX = "  -  ";

QString fieldBoxText(const SelectedField& sf)
{
  QString text;
  if (sf.minus)
    text = MINUSPREFIX;
  text += QString::fromStdString(sf.modelName) + " " + QString::fromStdString(sf.fieldName) + " " + QString::fromStdString(sf.refTime);
  return text;
}

} // anonymous namespace

void SelectedField::setFieldPlotOptions(const miutil::KeyValue_v& kv)
{
  oo.clear();
  po = PlotOptions();
  PlotOptions::parsePlotOption(kv, po, oo);
  diutil::maybeSetDefaults(po);

  units = miutil::extract_option(oo, UNITS);
  const std::string unit = miutil::extract_option(oo, UNIT);
  if (units.empty())
    units = unit;

  miutil::unique_options(oo);
}

miutil::KeyValue_v SelectedField::getFieldPlotOptions() const
{
  miutil::KeyValue_v kv = po.toKeyValueList();
  kv << oo;
  if (!units.empty())
    kv << miutil::kv(UNITS, units);
  return kv;
}

// ========================================================================

FieldDialog::FieldDialog(QWidget* parent, FieldDialogData* data)
    : DataDialog(parent, 0)
    , m_data(data)
    , currentSelectedFieldIndex(-1)
    , hasEditFields(false)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Fields"));
  m_action = new QAction(QIcon(QPixmap(felt_xpm)), windowTitle(), this);
  m_action->setCheckable(true);
  m_action->setShortcut(Qt::ALT + Qt::Key_F);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_fielddialogue.html";

  editName = tr("EDIT").toStdString();

  //----------------------------------------------------------------

  fieldAdd = new FieldDialogAdd(m_data.get(), this);

  std::map<std::string, miutil::KeyValue_v> setupFieldOptions;
  m_data->getSetupFieldOptions(setupFieldOptions);
  fieldStyle = new FieldDialogStyle(setupFieldOptions, this);
  connect(fieldStyle, &FieldDialogStyle::updateTime, this, &FieldDialog::updateTime);

  // selectedFieldbox
  QLabel *selectedFieldlabel = TitleLabel(tr("Selected fields"), this);
  selectedFieldbox = new QListWidget(this);
  selectedFieldbox->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(selectedFieldbox, &QListWidget::itemClicked, this, &FieldDialog::selectedFieldboxClicked);

  // Level: slider & label for the value
  QLabel *levelsliderlabel = new QLabel(tr("Vertical axis"), this);
  QLabel* levelLabel = new QLabel("1000hPa", this);
  levelLabel->setMinimumSize(levelLabel->sizeHint().width() + 10,
                             levelLabel->sizeHint().height() + 10);
  levelLabel->setMaximumSize(levelLabel->sizeHint().width() + 10,
      levelLabel->sizeHint().height() + 10);

  levelLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  levelLabel->setLineWidth(2);
  levelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  QSlider* levelSlider = new QSlider(Qt::Vertical, this);

  levelSliderControl = new StringSliderControl(true, levelSlider, levelLabel, this);
  connect(levelSliderControl, &StringSliderControl::valueChanged, this, &FieldDialog::levelChanged);

  // Idnum: slider & label for the value
  QLabel *idnumsliderlabel = new QLabel(tr("Extra axis"), this);
  QLabel* idnumLabel = new QLabel("EPS.Total", this);
  idnumLabel->setMinimumSize(idnumLabel->sizeHint().width() + 10,
                             idnumLabel->sizeHint().height() + 10);
  idnumLabel->setMaximumSize(idnumLabel->sizeHint().width() + 10,
                             idnumLabel->sizeHint().height() + 10);

  idnumLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  idnumLabel->setLineWidth(2);
  idnumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  QSlider* idnumSlider = new QSlider(Qt::Vertical, this);

  idnumSliderControl = new StringSliderControl(false, idnumSlider, idnumLabel, this);
  connect(idnumSliderControl, &StringSliderControl::valueChanged, this, &FieldDialog::idnumChanged);

  // copyField
  copyField = new QPushButton(tr("Copy"), this);
  connect(copyField, &QPushButton::clicked, this, &FieldDialog::copySelectedField);

  // deleteSelected
  deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, &QPushButton::clicked, this, &FieldDialog::deleteSelected);

  // deleteAll
  deleteAll = new QPushButton(tr("Delete all"), this);
  connect(deleteAll, &QPushButton::clicked, this, &FieldDialog::deleteAllSelected);

  // changeModelButton
  changeModelButton = new QPushButton(tr("Model"), this);
  connect(changeModelButton, &QPushButton::clicked, this, &FieldDialog::changeModel);

  int width = changeModelButton->sizeHint().width()/3;
  int height = changeModelButton->sizeHint().height();;

  // upField
  upFieldButton = new QPushButton(QPixmap(up12x12_xpm), "", this);
  upFieldButton->setMaximumSize(width,height);
  connect(upFieldButton, &QPushButton::clicked, this, &FieldDialog::upField);

  // downField
  downFieldButton = new QPushButton(QPixmap(down12x12_xpm), "", this);
  downFieldButton->setMaximumSize(width,height);
  connect(downFieldButton, &QPushButton::clicked, this, &FieldDialog::downField);

  // resetOptions
  resetOptionsButton = new QPushButton(tr("Default"), this);
  connect(resetOptionsButton, &QPushButton::clicked, this, &FieldDialog::resetFieldOptionsClicked);

  // minus
  minusButton = new ToggleButton(this, tr("Minus"));
  connect(minusButton, &ToggleButton::toggled, this, &FieldDialog::minusField);

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
  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->setSpacing(1);
  v1layout->addWidget(fieldAdd, 1);
  v1layout->addWidget(selectedFieldlabel);
  v1layout->addWidget(selectedFieldbox);

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
  h4layout->addWidget(fieldStyle->standardWidget());
  h4layout->addLayout(levellayout);
  h4layout->addLayout(idnumlayout);

  QHBoxLayout* h5layout = new QHBoxLayout();
  h5layout->addWidget(allTimeStepButton);
  h5layout->addWidget(advanced);

  QLayout* h6layout = createStandardButtons(false);

  QVBoxLayout* vlayout = new QVBoxLayout();
  vlayout->addLayout(v1layout, 1);
  vlayout->addLayout(v3layout);
  vlayout->addLayout(h4layout);
  vlayout->addLayout(h5layout);
  vlayout->addLayout(h6layout);

  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addLayout(vlayout, 0, 0);
  mainLayout->addWidget(fieldStyle->advancedWidget(), 0, 1);
  mainLayout->setColumnStretch(1, 1);
  setLayout(mainLayout);

  doShowMore(false);

  // tool tips
  toolTips();

  updateDialog();
}

FieldDialog::~FieldDialog()
{
  delete fieldAdd;
  delete fieldStyle;
}

void FieldDialog::toolTips()
{
  upFieldButton->setToolTip(tr("move selected field up"));
  downFieldButton->setToolTip(tr("move selected field down"));
  deleteButton->setToolTip(tr("delete selected field"));
  deleteAll->setToolTip(tr("delete all selected fields"));
  copyField->setToolTip(tr("copy field"));
  resetOptionsButton->setToolTip(tr("reset plot options"));
  minusButton->setToolTip(tr("selected field minus the field above"));
  changeModelButton->setToolTip(tr("change model/termin"));
  allTimeStepButton->setToolTip(tr("all time steps / only common time steps"));
}

void FieldDialog::doShowMore(bool more)
{
  advanced->setChecked(more);
  fieldStyle->advancedWidget()->setVisible(more);
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

void FieldDialog::updateTimes() {}

void FieldDialog::updateModelBoxes()
{
  METLIBS_LOG_SCOPE();

  // keep old plots
  const PlotCommand_cpv vstr = getOKString();

  fieldAdd->updateModelBoxes();

  if (!selectedFields.empty())
    deleteAllSelected();

  // replace old plots
  putOKString(vstr);
}

void FieldDialog::updateDialog()
{
  // FIXME reset setupFieldOptions
  updateModelBoxes();
}

void FieldDialog::archiveMode(bool on)
{
  fieldAdd->archiveMode(on);
}

void FieldDialog::setLevel()
{
  METLIBS_LOG_SCOPE();

  const int i = selectedFieldbox->currentRow();
  if (i >= 0 && i < (int)selectedFields.size()) {
    SelectedField& sf = selectedFields[i];
    levelSliderControl->setValues(sf.levelOptions, sf.level);
  }
}

void FieldDialog::levelChanged(const std::string& value)
{
  METLIBS_LOG_SCOPE(LOGVAL(value));

  const int i = selectedFieldbox->currentRow();
  if (i >= 0 && i < (int)selectedFields.size()) {
    selectedFields[i].level = value;
    updateTime();
  }
}

void FieldDialog::setIdnum()
{
  METLIBS_LOG_SCOPE();

  const int i = selectedFieldbox->currentRow();
  if (i >= 0 && i < (int)selectedFields.size()) {
    SelectedField& sf = selectedFields[i];
    idnumSliderControl->setValues(sf.idnumOptions, sf.idnum);
  }
}

void FieldDialog::idnumChanged(const std::string& value)
{
  METLIBS_LOG_SCOPE();

  const int i = selectedFieldbox->currentRow();
  if (i >= 0 && i < (int)selectedFields.size()) {
    selectedFields[i].idnum = value;
    updateTime();
  }
}

void FieldDialog::changeLevel(int increment, int type)
{
  METLIBS_LOG_SCOPE(LOGVAL(increment));

  // called from MainWindow levelUp/levelDown

  std::string level;
  std::vector<std::string> vlevels;
  const int n = selectedFields.size();

  // For some reason (?) vertical levels and extra levels are sorted i opposite directions
  if (type == 0) {
    increment *= -1;
  }

  // find first plot with levels, use use this plot to select next level
  int i = 0;
  if (type == 0) { // vertical levels
    while (i < n && selectedFields[i].levelOptions.size() < 2)
      i++;
    if (i != n) {
      vlevels = selectedFields[i].levelOptions;
      level = selectedFields[i].level;
    }
  } else { // extra levels
    while (i < n && selectedFields[i].idnumOptions.size() < 2)
      i++;
    if (i != n) {
      vlevels = selectedFields[i].idnumOptions;
      level = selectedFields[i].idnum;
    }
  }

  if (i == n) {
    // no plot with levels / idnumoptions found
    return;
  }

  // plot with levels exists, find next level
  std::string level_incremented;
  int m = vlevels.size();
  int current = 0;
  while (current < m && vlevels[current] != level)
    current++;
  int new_index = current + increment;
  if (new_index < m && new_index > -1) {
    level_incremented = vlevels[new_index];

    // loop through all plots to see if it is possible to plot:

    if (type == 0) { // vertical levels
      for (int j = 0; j < n; j++) {
        SelectedField& sf = selectedFields[j];
        if (sf.levelmove && sf.level == level) {
          sf.level = level_incremented;
          // update dialog
          if (j == selectedFieldbox->currentRow())
            levelSliderControl->setValue(sf.level);
        } else {
          sf.levelmove = false;
        }
      }
    } else { // extra levels
      for (int j = 0; j < n; j++) {
        SelectedField& sf = selectedFields[j];
        if (sf.idnummove && sf.idnum == level) {
          sf.idnum = level_incremented;
          // update dialog
          if (j == selectedFieldbox->currentRow())
            idnumSliderControl->setValue(sf.idnum);
        } else {
          sf.idnummove = false;
        }
      }
    }
  }
}

std::vector<SelectedField>::iterator FieldDialog::findSelectedField(const std::string& model, const std::string& reftime, bool predefined,
                                                                    const std::string& field)
{
  return std::find_if(selectedFields.begin(), selectedFields.end(), [&](const SelectedField& sf) {
    return sf.modelName == model && sf.refTime == reftime && sf.predefinedPlot == predefined && sf.fieldName == field;
  });
}

bool FieldDialog::isSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field)
{
  return findSelectedField(model, reftime, predefined, field) != selectedFields.end();
}

void FieldDialog::addPlot(const std::string& model, const std::string& reftime, bool predefined, const FieldPlotInfo& plot)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(reftime) << LOGVAL(predefined) << LOGVAL(plot.fieldName));

  SelectedField sf;
  sf.inEdit = false;
  sf.external = false;
  sf.modelName = model;
  sf.fieldName = plot.fieldName;
  sf.refTime = reftime;

  sf.levelOptions = plot.vlevels();
  sf.idnumOptions = plot.elevels();
  sf.zaxis = plot.vcoord();
  sf.extraaxis = plot.ecoord();

  sf.predefinedPlot = predefined;
  sf.minus = false;

  int n = sf.levelOptions.size();
  int i = 0;
  while (i < n && sf.levelOptions[i] != levelSliderControl->value())
    i++;
  if (i < n) {
    sf.level = levelSliderControl->value();
  } else {
    if (!plot.default_vlevel().empty()) {
      sf.level = plot.default_vlevel();
    } else if (!sf.levelOptions.empty()) {
      sf.level = sf.levelOptions.back();
    }
  }
  n = plot.elevels().size();
  i = 0;
  while (i < n && plot.elevels()[i] != idnumSliderControl->value())
    i++;
  if (i < n) {
    sf.idnum = idnumSliderControl->value();
  } else {
    if (!plot.default_elevel().empty()) {
      sf.idnum = plot.default_elevel();
    } else if (!sf.idnumOptions.empty()) {
      sf.idnum = sf.idnumOptions.front();
    }
  }
  sf.hourOffset = 0;
  sf.hourDiff = 0;
  sf.dimension = m_data->getFieldPlotDimension({sf.fieldName}, sf.predefinedPlot);

  sf.setFieldPlotOptions(fieldStyle->getFieldOptions(sf.fieldName, false));

  addSelectedField(sf);

  enableFieldOptions();
  updateTime();
}

void FieldDialog::removePlot(const std::string& model, const std::string& reftime, bool predefined, const FieldPlotInfo& plot)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(reftime) << LOGVAL(predefined) << LOGVAL(plot.fieldName));

  const auto it = findSelectedField(model, reftime, predefined, plot.fieldName);
  if (it != selectedFields.end())
    deleteSelectedField(std::distance(selectedFields.begin(), it));
}

void FieldDialog::updateFieldOptions()
{
  if (currentSelectedFieldIndex >= 0 && currentSelectedFieldIndex < selectedFields.size()) {
    fieldStyle->updateFieldOptions(&selectedFields[currentSelectedFieldIndex]);
  }
}

void FieldDialog::enableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  const int index = selectedFieldbox->currentRow();
  if (index != currentSelectedFieldIndex)
    updateFieldOptions();
  currentSelectedFieldIndex = index;

  const int lastindex = selectedFields.size() - 1;
  if (index < 0 || index > lastindex) {
    fieldStyle->enableFieldOptions(nullptr);
    return;
  }

  const int numEditFields = hasEditFields ? 1 : 0;

  SelectedField& sf = selectedFields[index];
  changeModelButton->setEnabled(!sf.inEdit);
  deleteButton->setEnabled(!sf.inEdit);
  copyField->setEnabled(!sf.inEdit);
  upFieldButton->setEnabled(!sf.inEdit && index > numEditFields);
  downFieldButton->setEnabled(!sf.inEdit && index < lastindex);

  setLevel();
  setIdnum();
  deleteAll->setEnabled(true);
  resetOptionsButton->setEnabled(true);

  minusButton->setEnabled(index > numEditFields && !selectedFields[index - 1].minus);
  minusButton->setChecked(sf.minus);

  fieldStyle->enableFieldOptions(&sf);
}

void FieldDialog::simulateSelectField(int index)
{
  selectedFieldbox->setCurrentRow(index);
  selectedFieldboxClicked(nullptr);
}

void FieldDialog::selectedFieldboxClicked(QListWidgetItem*)
{
  enableFieldOptions();
}

PlotCommand_cpv FieldDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  updateFieldOptions();

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

    cmd->addOptions(sf.getFieldPlotOptions());
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
  const int n = selectedFields.size();
  const int field_start = (hasEditFields ? 1 : 0);
  if (n <= field_start)
    return std::string();

  std::ostringstream ostr;
  std::string pmodelName;
  bool fielddiff = false, paramdiff = false, leveldiff = false;

  ostr << "<font color=\"#000099\">";
  for (int i = field_start; i < n; i++) {
    SelectedField& sf = selectedFields[i];
    const std::string& modelName = sf.modelName;
    const std::string& fieldName = sf.fieldName;
    const std::string& level = sf.level;
    const std::string& idnum = sf.idnum;

    //difference field
    if (i < n - 1 && selectedFields[i + 1].minus) {
      SelectedField& sfm = selectedFields[i + 1];

      const std::string& modelName2 = sfm.modelName;
      const std::string& fieldName2 = sfm.fieldName;
      const std::string& level_2 = sfm.level;
      const std::string& idnum_2 = sfm.idnum;

      fielddiff = (modelName != modelName2);
      paramdiff = (fieldName != fieldName2);
      leveldiff = (level.empty() || level != level_2 || idnum != idnum_2);

      if (modelName != pmodelName || modelName2 != pmodelName) {

        if (i > 0)
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

      if (!sf.level.empty()) {
        if (!fielddiff && !paramdiff)
          ostr << " ( " << sf.level;
        else if ((fielddiff || paramdiff) && leveldiff)
          ostr << " " << sf.level;
      }
      if (!sf.idnum.empty() && leveldiff)
        ostr << " " << sf.idnum;

    } else if (sf.minus) {
      ostr << " - ";

      if (fielddiff) {
        ostr << modelName;
        pmodelName.clear();
      }

      if (fielddiff && !paramdiff && !leveldiff)
        ostr << " ) " << fieldName;
      else if (paramdiff || (fielddiff && leveldiff))
        ostr << " " << fieldName;

      if (!sf.level.empty()) {
        if (!leveldiff && paramdiff)
          ostr << " )";
        ostr << " " << sf.level;
        if (!sf.idnum.empty())
          ostr << " " << sf.idnum;
        if (leveldiff)
          ostr << " )";
      } else {
        ostr << " )";
      }

      fielddiff = paramdiff = leveldiff = false;

    } else { // Ordinary field

      if (i > 0)
        ostr << "  ";

      if (modelName != pmodelName) {
        ostr << modelName;
        pmodelName = modelName;
      }

      ostr << " " << fieldName;

      if (!sf.level.empty())
        ostr << " " << sf.level;
      if (!sf.idnum.empty())
        ostr << " " << sf.idnum;
    }
  }
  ostr << "</font>";
  return ostr.str();
}

bool FieldDialog::levelsExists(bool up, int type)
{
  // return true if there exists a plot with levels available for up/down of type 0=vertical or 1=extra/eps

  const int n = selectedFields.size();

  if (type == 0) // for vertical levels, "up" is reversed
    up = !up;

  for (int i = 0; i < n; ++i) {
    const SelectedField& sf = selectedFields[i];
    const auto& levels = (type == 0) ? sf.levelOptions : sf.idnumOptions;
    if (levels.size() >= 2) {
      const auto& level = (type == 0) ? sf.level : sf.idnum;
      const auto& last = up ? levels.back() : levels.front();
      return level != last;
    }
  }

  return false;
}

void FieldDialog::maybeUpdateFieldReferenceTimes(std::set<std::string>& updated_models, const std::string& model)
{
  if (updated_models.insert(model).second) {
    m_data->updateFieldReferenceTimes(model);
  }
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

  std::set<std::string> updated_models;

  for (PlotCommand_cp pc : vstr) {
    FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(pc);
    if (!cmd)
      continue;

    maybeUpdateFieldReferenceTimes(updated_models, cmd->field.model);
    if (cmd->hasMinusField())
      maybeUpdateFieldReferenceTimes(updated_models, cmd->minus.model);

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
  }

  enableFieldOptions();
  updateTime();
}

void FieldDialog::addSelectedField(const SelectedField& sf)
{
  selectedFields.push_back(sf);

  selectedFieldbox->addItem(fieldBoxText(sf));
  const int last = selectedFieldbox->count() - 1;
  selectedFieldbox->setCurrentRow(last);
  selectedFieldbox->item(last)->setSelected(true);

  fieldAdd->addedSelectedField(sf.modelName, sf.refTime, sf.predefinedPlot, sf.fieldName);
}

void FieldDialog::deleteSelectedField(int index)
{
  {
    const SelectedField& sf = selectedFields[index];
    fieldAdd->removingSelectedField(sf.modelName, sf.refTime, sf.predefinedPlot, sf.fieldName);
  }

  selectedFieldbox->takeItem(index);
  selectedFields.erase(selectedFields.begin() + index);

  if (!selectedFields.empty()) {
    miutil::minimize(index, selectedFields.size() - 1);
    selectedFieldbox->setCurrentRow(index);
    selectedFieldbox->item(index)->setSelected(true);
  } else {
    setLevel();
    setIdnum();
  }

  enableFieldOptions();
  updateTime();
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
  sf.dimension = m_data->getFieldPlotDimension({sf.fieldName}, sf.predefinedPlot);

  if (!fs.allTimeSteps)
    allTimeStepButton->setChecked(false);

  // merge with options from setup/logfile for this fieldname
  const auto kv = mergeSetupAndQuickMenuOptions(fieldStyle->getFieldOptions(fs.name(), true), cmd->options());
  sf.setFieldPlotOptions(kv);

  METLIBS_LOG_DEBUG(LOGVAL(sf.modelName) << LOGVAL(sf.fieldName) << LOGVAL(sf.level) << LOGVAL(sf.idnum));

  FieldPlotGroupInfo_v vfg;
  m_data->getFieldPlotGroups(sf.modelName, sf.refTime, sf.predefinedPlot, vfg);

  //find index of fieldgroup
  const FieldPlotInfo* fi_found = 0;
  for (FieldPlotGroupInfo& fgi : vfg) {
    const FieldPlotInfo_v& fgip = fgi.plots;
    FieldPlotInfo_v::const_iterator it = std::find_if(fgip.begin(), fgip.end(), [&](const FieldPlotInfo& f) { return f.fieldName == sf.fieldName; });
    if (it == fgip.end())
      continue;
    const FieldPlotInfo& p = *it;
    if ((sf.zaxis == p.vcoord() || (p.vlevels().size() == 1))) {
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

std::vector<std::string> FieldDialog::writeLog()
{
  return fieldStyle->writeLog();
}

void FieldDialog::readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion)
{
  fieldStyle->readLog(vstr, thisVersion, logVersion);
}

void FieldDialog::deleteSelected()
{
  METLIBS_LOG_SCOPE();

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= (int)selectedFields.size() || selectedFields[index].inEdit)
    return;

  deleteSelectedField(index);
}

void FieldDialog::deleteAllSelected()
{
  METLIBS_LOG_SCOPE();

  const int numEditFields = hasEditFields ? 1 : 0;
  for (int i = selectedFields.size() - 1; i >= numEditFields; --i) {
    const SelectedField& sf = selectedFields[i];
    fieldAdd->removingSelectedField(sf.modelName, sf.refTime, sf.predefinedPlot, sf.fieldName);
  }

  selectedFields.resize(numEditFields);
  while (selectedFieldbox->count() > numEditFields)
    selectedFieldbox->takeItem(selectedFieldbox->count() - 1);

  if (numEditFields > 0) {
    selectedFieldbox->setCurrentRow(0);
    selectedFieldbox->item(0)->setSelected(true);
  }

  enableFieldOptions();
  updateTime();
}

void FieldDialog::copySelectedField()
{
  METLIBS_LOG_SCOPE();

  const int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= (int)selectedFields.size())
    return;

  updateFieldOptions();

  SelectedField sf = selectedFields[index]; // make a copy
  sf.hourOffset = 0;
  addSelectedField(sf);

  enableFieldOptions();
}

void FieldDialog::changeModel()
{
  METLIBS_LOG_SCOPE();

  if (selectedFieldbox->count() == 0 || selectedFields.empty())
    return;
  int n = selectedFields.size();

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n)
    return;

  const std::string oldModel = selectedFields[index].modelName;
  const std::string oldRefTime = selectedFields[index].refTime;
  std::string newModel, newRefTime;
  bool predefined;
  if (!fieldAdd->currentModelReftime(newModel, newRefTime, predefined))
    return;
  if ( (oldModel == newModel) && (oldRefTime == newRefTime) )
    return;

  for (int i = 0; i < n; i++) {
    SelectedField& sf = selectedFields[i];
    if ((sf.modelName == oldModel) && (sf.refTime == oldRefTime)) {
      // check if field exists for the new model
      FieldPlotGroupInfo_v vfg;
      m_data->getFieldPlotGroups(newModel, newRefTime, sf.predefinedPlot, vfg);

      int gbest = -1,fbest = -1;
      for (size_t j = 0; j < vfg.size(); ++j) {

        const int m = vfg[j].plots.size();
        int k = 0;
        while (k < m && !(vfg[j].plots[k].fieldName == sf.fieldName)) {
          k++;
        }
        // Check if parameters have same vcoord, ignore if no. levels == 0,1
        if (k < m && ((vfg[j].plots[k].vcoord() == sf.zaxis) || sf.levelOptions.size() < 2)) {
          gbest = j;
          fbest = k;
          break;
        }
      }
      if ( gbest > -1 ) {
        const FieldPlotInfo& fi_best = vfg[gbest].plots[fbest];
        SelectedField& sf = selectedFields[i];
        fieldAdd->removingSelectedField(sf.modelName, sf.refTime, sf.predefinedPlot, sf.fieldName);
        sf.modelName = newModel;
        sf.refTime = newRefTime;
        sf.levelOptions = fi_best.vlevels();
        sf.idnumOptions = fi_best.elevels();
        sf.predefinedPlot = predefined;
        fieldAdd->addedSelectedField(sf.modelName, sf.refTime, sf.predefinedPlot, sf.fieldName);
        selectedFieldbox->item(i)->setText(fieldBoxText(sf));
      }
    }
  }

  selectedFieldbox->setCurrentRow(index);
  selectedFieldbox->item(index)->setSelected(true);
  enableFieldOptions();

  updateTime();
}

void FieldDialog::upField()
{
  moveField(-1);
}

void FieldDialog::downField()
{
  moveField(+1);
}

void FieldDialog::moveField(int delta)
{
  if (selectedFieldbox->count() == 0)
    return;
  const int n = selectedFields.size();
  if (n == 0)
    return;

  const int index = selectedFieldbox->currentRow();
  const int other = index + delta;
  if (index < 0 || other < 0 || index > n || other > n)
    return;

  std::swap(selectedFields[index], selectedFields[other]);
  currentSelectedFieldIndex = other;

  const QString qindex = selectedFieldbox->item(index)->text();
  const QString qother = selectedFieldbox->item(other)->text();
  selectedFieldbox->item(index)->setText(qother);
  selectedFieldbox->item(other)->setText(qindex);
  selectedFieldbox->setCurrentRow(other);

  enableFieldOptions();
}

void FieldDialog::minusField(bool on)
{
  const int numEditFields = hasEditFields ? 1 : 0;
  const int i = selectedFieldbox->currentRow();
  if (i <= numEditFields || i >= selectedFieldbox->count() || selectedFields[i - 1].minus)
    return;

  SelectedField& sf = selectedFields[i];
  sf.minus = on;
  selectedFieldbox->item(i)->setText(fieldBoxText(sf));
  if (on && i + 1 < (int)selectedFields.size()) {
    //next field can't be minus
    SelectedField& sf1 = selectedFields[i + 1];
    sf1.minus = false;
    selectedFieldbox->item(i + 1)->setText(fieldBoxText(sf1));
  }

  enableFieldOptions();
}

void FieldDialog::updateTime()
{
  METLIBS_LOG_SCOPE();

  updateFieldOptions();

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

void FieldDialog::fieldEditUpdate(const std::string& str)
{
  METLIBS_LOG_SCOPE(LOGVAL(str));

  if (str.empty() && !hasEditFields)
    return;

  if (hasEditFields) {
    METLIBS_LOG_DEBUG("STOP");
    selectedFields[0].inEdit = false;
    if (selectedFields[0].modelName.empty()) {
      selectedFieldbox->setCurrentRow(0);
      deleteSelected();
    } else {
      std::string text = selectedFields[0].modelName + " " + selectedFields[0].fieldName;
      selectedFieldbox->item(0)->setText(QString::fromStdString(text));
    }
    hasEditFields = false;

  } else {

    // add edit field (and remove the original field)
    SelectedField sf;
    std::vector<std::string> vstr = miutil::split(str, " ");

    if (vstr.size() == 1) {
      // str=fieldName if the field is not already read
      sf.inEdit = true;
      sf.fieldName = vstr[0];
      sf.setFieldPlotOptions(fieldStyle->getFieldOptions(sf.fieldName, false));
      selectedFields.push_back(sf);
      std::string text = editName + " " + sf.fieldName;
      selectedFieldbox->addItem(QString::fromStdString(text));
      selectedFieldbox->setCurrentRow(selectedFieldbox->count() - 1);

    } else if (vstr.size() > 1)    {
      // str="modelname fieldName level" if field from dialog is selected
      sf.modelName = vstr[0];
      sf.fieldName = vstr[1];

      size_t n = selectedFields.size();
      size_t i = 0;
      while (i < n && !(selectedFields[i].modelName == sf.modelName && selectedFields[i].fieldName == sf.fieldName))
        ++i;

      if (i < n) {
        selectedFields[i].inEdit = true;
        std::string text = editName + " " + sf.fieldName;
        selectedFieldbox->item(i)->setText(QString::fromStdString(text));
        selectedFieldbox->setCurrentRow(i);
      } else {
        return;
      }
    }

    while (selectedFieldbox->currentRow() > 0) {
      upField();
    }

    hasEditFields = true;
  }

  updateTime();
  enableFieldOptions();
  Q_EMIT applyData();
}

void FieldDialog::allTimeStepToggled(bool)
{
  updateTime();
}

void FieldDialog::resetFieldOptionsClicked()
{
  if (currentSelectedFieldIndex >= 0 && currentSelectedFieldIndex < selectedFields.size()) {
    fieldStyle->resetFieldOptions(&selectedFields[currentSelectedFieldIndex]);
  }
}
