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

#include "qtFieldDialogAdd.h"

#include "qtFieldDialog.h"
#include "qtTreeFilterProxyModel.h"
#include "qtUtility.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.FieldDialogAdd"
#include <miLogger/miLogging.h>


namespace { // anonymous

const int ROLE_MODELGROUP = Qt::UserRole + 1;

const char* const modelGlobalAttributes[][2] = {
    {"title", QT_TRANSLATE_NOOP("FieldDialog", "Title")},
    {"summary", QT_TRANSLATE_NOOP("FieldDialog", "Summary")},
};

} // anonymous namespace

// ========================================================================

FieldDialogAdd::FieldDialogAdd(FieldDialogData* data, FieldDialog* dialog)
    : QWidget(dialog)
    , dialog(dialog)
    , m_data(data)
    , useArchive(false)
{
  setupUi();
}

FieldDialogAdd::~FieldDialogAdd() {}

void FieldDialogAdd::setupUi()
{
  // modelbox
  QLabel* modellabel = TitleLabel(tr("Models"), this);
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
  connect(modelbox, &QTreeView::clicked, this, &FieldDialogAdd::modelboxClicked);
  modelFilterEdit = new QLineEdit("", this);
  modelFilterEdit->setPlaceholderText(tr("Type to filter model names"));
  connect(modelFilterEdit, &QLineEdit::textChanged, this, &FieldDialogAdd::filterModels);

  // refTime
  QLabel* refTimelabel = TitleLabel(tr("Reference time"), this);
  refTimeComboBox = new QComboBox(this);
  refTimeComboBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  connect(refTimeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogAdd::updateFieldGroups);

  // fieldGRbox
  QLabel* fieldGRlabel = TitleLabel(tr("Field group"), this);
  fieldGRbox = new QComboBox(this);
  connect(fieldGRbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &FieldDialogAdd::fieldGRboxActivated);

  // fieldGroupCheckBox
  predefinedPlotsCheckBox = new QCheckBox(tr("Predefined plots"), this);
  predefinedPlotsCheckBox->setToolTip(tr("Show predefined plots or all parameters from file"));
  predefinedPlotsCheckBox->setChecked(true);
  connect(predefinedPlotsCheckBox, &QCheckBox::toggled, this, &FieldDialogAdd::updateFieldGroups);

  // fieldbox
  QLabel* fieldlabel = TitleLabel(tr("Fields"), this);
  fieldbox = new QListWidget(this);
  fieldbox->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(fieldbox, &QListWidget::itemClicked, this, &FieldDialogAdd::fieldboxChanged);

  // layout
  QHBoxLayout* modellayout = new QHBoxLayout();
  modellayout->addWidget(modellabel);
  modellayout->addWidget(modelFilterEdit);

  QHBoxLayout* grouplayout = new QHBoxLayout();
  grouplayout->addWidget(fieldGRbox);
  grouplayout->addWidget(predefinedPlotsCheckBox);

  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->setSpacing(1);
  v1layout->setMargin(0);
  v1layout->addLayout(modellayout);
  v1layout->addWidget(modelbox, 2);
  v1layout->addWidget(refTimelabel);
  v1layout->addWidget(refTimeComboBox);
  v1layout->addWidget(fieldGRlabel);
  v1layout->addLayout(grouplayout);
  v1layout->addWidget(fieldlabel);
  v1layout->addWidget(fieldbox, 4);
  setLayout(v1layout);
}

void FieldDialogAdd::archiveMode(bool on)
{
  useArchive = on;
  updateModelBoxes();
}

void FieldDialogAdd::updateModelBoxes()
{
  METLIBS_LOG_SCOPE();

  m_modelgroup = m_data->getFieldModelGroups();

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
}

void FieldDialogAdd::addModelGroup(int modelgroupIndex)
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

void FieldDialogAdd::getFieldGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfg)
{
  METLIBS_LOG_TIME(LOGVAL(modelName));

  diutil::OverrideCursor waitCursor;
  m_data->getFieldPlotGroups(modelName, refTime, predefinedPlots, vfg);
  QString tooltip;
  const std::map<std::string, std::string> global_attributes = m_data->getFieldGlobalAttributes(modelName, refTime);
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

void FieldDialogAdd::filterModels(const QString& filtertext)
{
  modelFilter->setFilterFixedString(filtertext);
  if (!filtertext.isEmpty()) {
    modelbox->expandAll();
    if (modelFilter->rowCount() > 0)
      modelbox->scrollTo(modelFilter->index(0, 0));
  }
}

void FieldDialogAdd::modelboxClicked(const QModelIndex& filterIndex)
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

  m_data->updateFieldReferenceTimes(currentModel);
  const std::set<std::string> refTimes = m_data->getFieldReferenceTimes(currentModel);

  for (const std::string& rt : refTimes) {
    refTimeComboBox->addItem(QString::fromStdString(rt));
  }
  if (refTimeComboBox->count()) {
    refTimeComboBox->setCurrentIndex(refTimeComboBox->count() - 1);
    updateFieldGroups();
  }
}

void FieldDialogAdd::updateFieldGroups()
{
  METLIBS_LOG_TIME();

  fieldGRbox->clear();
  fieldbox->clear();

  getFieldGroups(currentModel, currentRefTime(), currentPredefinedPlots(), vfgi);

  const int nvfgi = vfgi.size();
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
}

void FieldDialogAdd::fieldGRboxActivated(int indexFGR)
{
  fieldbox->clear();

  std::string model, reftime;
  bool predefined;
  if (currentModelReftime(model, reftime, predefined) && !vfgi.empty()) {

    FieldPlotGroupInfo& fpgi = vfgi[indexFGR];
    lastFieldGroupName = fpgi.groupName();
    for (const FieldPlotInfo& plot : fpgi.plots) {
      QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(plot.fieldName));
      item->setToolTip(QString::fromStdString(plot.variableName));
      fieldbox->addItem(item);
      // item selected state must be set after adding
      item->setSelected(dialog->isSelectedField(model, reftime, predefined, plot.fieldName));
    }
  }
}

std::string FieldDialogAdd::currentRefTime() const
{
  return refTimeComboBox->currentText().toStdString();
}

bool FieldDialogAdd::currentPredefinedPlots() const
{
  return predefinedPlotsCheckBox->isChecked();
}

bool FieldDialogAdd::currentModelReftime(std::string& model, std::string& reftime, bool& predefined)
{
  predefined = currentPredefinedPlots();

  if (currentModel.empty())
    return false;

  model = currentModel;
  reftime = currentRefTime();
  METLIBS_LOG_DEBUG(LOGVAL(model) << LOGVAL(reftime) << LOGVAL(predefined));
  return true;
}

void FieldDialogAdd::fieldboxChanged(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE(LOGVAL(fieldbox->count()));

  std::string model, reftime;
  bool predefined;
  if (currentModelReftime(model, reftime, predefined)) {
    const FieldPlotInfo& plot = vfgi[fieldGRbox->currentIndex()].plots[fieldbox->currentRow()];
    if (item->isSelected())
      dialog->addPlot(model, reftime, predefined, plot);
    else
      dialog->removePlot(model, reftime, predefined, plot);
  }
}

void FieldDialogAdd::selectField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field, bool select)
{
  if (model == currentModel && reftime == currentRefTime() && predefined == currentPredefinedPlots()) {
    const QString qfield = QString::fromStdString(field);
    for (int i = 0; i < fieldbox->count(); ++i) {
      QListWidgetItem* item = fieldbox->item(i);
      if (item->text() == qfield && item->isSelected() != select)
        item->setSelected(select);
    }
  }
}

void FieldDialogAdd::addedSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field)
{
  selectField(model, reftime, predefined, field, true);
}

void FieldDialogAdd::removingSelectedField(const std::string& model, const std::string& reftime, bool predefined, const std::string& field)
{
  selectField(model, reftime, predefined, field, false);
}
