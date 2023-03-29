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

#include "diana_config.h"

#include "qtVprofModelDialog.h"

#include "diVprofManager.h"
#include "diVprofPlotCommand.h"
#include "diVprofReader.h"
#include "qtUtility.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VprofModelDialog"
#include <miLogger/miLogging.h>


//#define HEIGHTLISTBOX 100

/***************************************************************************/

VprofModelDialog::VprofModelDialog(QWidget* parent, VprofManager* vm)
    : QDialog(parent)
    , vprofm(vm)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  // caption to appear on top of dialog
  setWindowTitle(tr("Diana Vertical Profiles"));

  modelfileList = new QListWidget(this);
  connect(modelfileList, &QListWidget::itemClicked, this, &VprofModelDialog::modelfilelistClicked);

  QLabel* refLabel = TitleLabel(tr("Referencetime"), this);
  reftimeWidget = new QListWidget(this);
  connect(reftimeWidget, &QListWidget::itemClicked, this, &VprofModelDialog::reftimeWidgetClicked);

  QLabel* selectedLabel = TitleLabel(tr("Selected models"), this);
  selectedModelsWidget = new QListWidget(this);

  // push button to show help
  QPushButton* modelhelp = new QPushButton(tr("Help"), this);
  connect(modelhelp, &QPushButton::clicked, this, &VprofModelDialog::helpClicked);

  // push button to delete
  QPushButton* deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, &QPushButton::clicked, this, &VprofModelDialog::deleteClicked);
  QPushButton* deleteAllButton = new QPushButton(tr("Delete all"), this);
  connect(deleteAllButton, &QPushButton::clicked, this, &VprofModelDialog::deleteAllClicked);

  // push button to hide dialog
  QPushButton* modelhide = new QPushButton(tr("Hide"), this);
  connect(modelhide, &QPushButton::clicked, this, &VprofModelDialog::ModelHide);

  // push button to apply the selected command and then hide dialog
  QPushButton* modelapplyhide = new QPushButton(tr("Apply+Hide"), this);
  connect(modelapplyhide, &QPushButton::clicked, this, &VprofModelDialog::applyhideClicked);

  // push button to apply the selected command
  QPushButton* modelapply = new QPushButton(tr("Apply"), this);
  connect(modelapply, &QPushButton::clicked, this, &VprofModelDialog::applyClicked);

  // ************ place all the widgets in layouts ****************

  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget(modelhelp);
  hlayout1->addWidget(deleteButton);
  hlayout1->addWidget(deleteAllButton);

  QHBoxLayout* hlayout3 = new QHBoxLayout();
  hlayout3->addWidget(modelhide);
  hlayout3->addWidget(modelapplyhide);
  hlayout3->addWidget(modelapply);

  // create a vertical layout to put all widgets and layouts in
  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->addWidget(modelfileList);
  vlayout->addWidget(refLabel);
  vlayout->addWidget(reftimeWidget);
  vlayout->addWidget(selectedLabel);
  vlayout->addWidget(selectedModelsWidget);
  vlayout->addLayout(hlayout1);
  vlayout->addLayout(hlayout3);
}

static QString currentItem(QListWidget* list)
{
  QListWidgetItem* i = list->currentItem();
  return i ? i->text() : QString();
}

void VprofModelDialog::modelfilelistClicked(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();

  diutil::OverrideCursor waitCursor;

  reftimeWidget->clear();
  const std::set<std::string> rfv = vprofm->getReferencetimes(item->text().toStdString());

  if (rfv.empty()) {
    const QString model = currentItem(modelfileList);
    if (!selectedModelsWidget->count() || model != currentItem(selectedModelsWidget)) {
      selectedModelsWidget->addItem(model);
    } else {
      return;
    }
  } else {
    for (const std::string& rt : rfv)
      reftimeWidget->addItem(QString::fromStdString(rt));

    reftimeWidget->setCurrentRow(reftimeWidget->count() - 1);
    selectedModelsWidget->addItem(getSelectedModelString());
  }
  selectedModelsWidget->setCurrentRow(selectedModelsWidget->count() - 1);
}

void VprofModelDialog::reftimeWidgetClicked(QListWidgetItem*)
{
  const QString model = currentItem(modelfileList);
  if (selectedModelsWidget->count() && currentItem(selectedModelsWidget).contains(model)) {
    selectedModelsWidget->currentItem()->setText(getSelectedModelString());
  }
}

void VprofModelDialog::deleteClicked()
{
  METLIBS_LOG_SCOPE();
  if (selectedModelsWidget->count()) {
    int row = selectedModelsWidget->currentRow();
    selectedModelsWidget->takeItem(row);
  }
}

void VprofModelDialog::deleteAllClicked()
{
  METLIBS_LOG_SCOPE();
  selectedModelsWidget->clear();
  reftimeWidget->clear();
}

/*********************************************/

void VprofModelDialog::helpClicked()
{
  METLIBS_LOG_SCOPE();
  emit showsource("ug_verticalprofiles.html");
}

/*********************************************/

void VprofModelDialog::applyClicked()
{
  METLIBS_LOG_SCOPE();
  Q_EMIT ModelApply();
}

/*********************************************/

void VprofModelDialog::applyhideClicked()
{
  METLIBS_LOG_SCOPE();
  Q_EMIT ModelHide();
  Q_EMIT ModelApply();
}

void VprofModelDialog::getModel()
{
  selectedModelsWidget->clear();
  for (auto& sm : vprofm->getSelectedModels()) {
    std::string reftime = sm.reftime;
    if (reftime.empty()) {
      const std::set<std::string> rfv = vprofm->getReferencetimes(sm.model);
      if (!rfv.empty())
        reftime = *rfv.rbegin();
    }
    selectedModelsWidget->addItem(QString::fromStdString(sm.model + " " + reftime));
  }
}

QString VprofModelDialog::getSelectedModelString()
{
  QString qstr;
  if (modelfileList->currentItem() && reftimeWidget->currentItem()) {
    qstr = currentItem(modelfileList) + " " + currentItem(reftimeWidget);
  }
  return qstr;
}

PlotCommand_cpv VprofModelDialog::getPlotCommands()
{
  const int n = selectedModelsWidget->count();
  std::vector<std::string> selectedModels;
  selectedModels.reserve(n);
  for (int i = 0; i < n; i++) {
    selectedModels.push_back(selectedModelsWidget->item(i)->text().toStdString());
  }
  VprofPlotCommand_p cmd = std::make_shared<VprofPlotCommand>(VprofPlotCommand::MODELS);
  cmd->setItems(selectedModels);

  return PlotCommand_cpv(1, cmd);
}

/*********************************************/

void VprofModelDialog::updateModelfileList()
{
  METLIBS_LOG_SCOPE();

  // want to keep the selected models
  std::set<QString> current;
  for (int i = 0; i < modelfileList->count(); i++) {
    if (modelfileList->item(i)->isSelected())
      current.insert(modelfileList->item(i)->text());
  }

  // clear box with list of files
  modelfileList->clear();

  const std::vector<std::string>& modelnames = vprofm->getModelNames();
  const std::vector<std::string>& modelfiles = vprofm->getModelFiles();
  for (size_t i = 0; i < modelnames.size(); i++) {
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(modelnames[i]), modelfileList);
    if (i < modelfiles.size())
      item->setToolTip(QString::fromStdString(modelfiles[i]));
  }

  for (int i = 0; i < modelfileList->count(); i++) {
    if (current.find(modelfileList->item(i)->text()) != current.end())
      modelfileList->item(i)->setSelected(true);
  }
}

void VprofModelDialog::closeEvent(QCloseEvent*)
{
  METLIBS_LOG_SCOPE();
  Q_EMIT ModelHide();
}
