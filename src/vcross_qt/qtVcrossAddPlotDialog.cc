/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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

//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vcross_qt/qtVcrossAddPlotDialog.h"

#include "diUtilities.h"
#include "qtUtility.h"
#include "vcross_v2/VcrossQtManager.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#define MILOGGER_CATEGORY "diana.VcrossAddPlotDialog"
#include <miLogger/miLogging.h>

#include "vcross_plot_add_dialog.ui.h"

#include "forover.xpm"
#include "bakover.xpm"
#include "start.xpm"

VcrossAddPlotDialog::VcrossAddPlotDialog(QWidget* parent, vcross::QtManager_p m)
  : QDialog(parent)
  , vcrossm(m)
  , ui(new Ui_VcrossAddPlotDialog)
{
  METLIBS_LOG_SCOPE();

  setupUi();
}

void VcrossAddPlotDialog::setupUi()
{
  ui->setupUi(this);
  ui->stack->setCurrentIndex(0);

  ui->buttonRestart->setIcon(QPixmap(start_xpm));
  ui->buttonBack->setIcon(QPixmap(bakover_xpm));
  ui->buttonNext->setIcon(QPixmap(forward_xpm));

  modelNames = new QStringListModel(this);
  modelSorter = new QSortFilterProxyModel(this);
  modelSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  modelSorter->setSourceModel(modelNames);
  ui->modelList->setModel(modelSorter);

  referenceTimes = new QStringListModel(this);
  ui->reftimeList->setModel(referenceTimes);

  plotNames = new QStringListModel(this);
  plotSorter = new QSortFilterProxyModel(this);
  plotSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  plotSorter->setSourceModel(plotNames);
  ui->plotList->setModel(plotSorter);

  connect(ui->modelFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onModelFilter(const QString&)));
  connect(ui->modelList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkModelComplete()));
  connect(ui->modelList, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onNext()));

  connect(ui->reftimeList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkReftimeComplete()));
  connect(ui->reftimeList, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onNext()));

  connect(ui->plotFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onPlotFilter(const QString&)));
  connect(ui->plotList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkPlotComplete()));
  connect(ui->plotList, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onAdd()));

  connect(ui->buttonRestart, SIGNAL(clicked()), this, SLOT(restart()));
  connect(ui->buttonBack, SIGNAL(clicked()), this, SLOT(onBack()));
  connect(ui->buttonNext, SIGNAL(clicked()), this, SLOT(onNext()));
  connect(ui->buttonAdd,  SIGNAL(clicked()), this, SLOT(onAdd()));
}

void VcrossAddPlotDialog::restart()
{
  initializeModelPage(true);
}

void VcrossAddPlotDialog::onBack()
{
  const int page = ui->stack->currentIndex();
  if (page == PlotPage)
    initializeReftimePage(false);
  else if (page == ReftimePage)
    initializeModelPage(false);
}

void VcrossAddPlotDialog::onNext()
{
  const int page = ui->stack->currentIndex();
  if (page == ModelPage and isModelComplete())
    initializeReftimePage(true);
  else if (page == ReftimePage and isReftimeComplete())
    initializePlotPage(true);
}

void VcrossAddPlotDialog::onAdd()
{
  const int page = ui->stack->currentIndex();
  if (page != PlotPage
      || (!(isModelComplete() && isReftimeComplete() && isPlotComplete())))
  {
    return;
  }

  const std::string model = selectedModel().toStdString();
  const miutil::miTime reftime(selectedReferenceTime().toStdString());
  vcross::QtManager::PlotSpec ps(model, reftime, "");

  const QStringList plots = selectedPlots();
  if (!plots.isEmpty()) {
    vcrossm->fieldChangeStart(false);
    for (int i=0; i<plots.size(); ++i) {
      const std::string fld = plots.at(i).toStdString();
      const std::string opt = vcrossm->getPlotOptions(fld, false);
      ps.setField(fld);
      vcrossm->addField(ps, opt, -1);
    }
    vcrossm->fieldChangeDone();
  }
  initializePlotPage(true);
}

void VcrossAddPlotDialog::initializeModelPage(bool forward)
{
  ui->stack->setCurrentIndex(ModelPage);

  if (forward) {
    ui->modelFilter->clear();

    const std::vector<std::string> models = vcrossm->getAllModels();
    QStringList msl;
    for (size_t i=0; i<models.size(); ++i)
      msl << QString::fromStdString(models[i]);

    modelNames->setStringList(msl);
  }
  ui->buttonRestart->setEnabled(false);
  ui->buttonBack->setEnabled(false);
  ui->buttonNext->setEnabled(isModelComplete());
  ui->buttonAdd->setEnabled(false);
}

void VcrossAddPlotDialog::onModelFilter(const QString& text)
{
  modelSorter->setFilterFixedString(text);
}

bool VcrossAddPlotDialog::isModelComplete() const
{
  return (not ui->modelList->selectionModel()->selectedIndexes().isEmpty());
}

void VcrossAddPlotDialog::checkModelComplete()
{
  bool haveModel = isModelComplete();
  ui->buttonNext->setEnabled(haveModel);
}

QString VcrossAddPlotDialog::selectedModel() const
{
  const QModelIndexList si = ui->modelList->selectionModel()->selectedIndexes();
  if (si.size() == 1)
    return modelNames->stringList().at(modelSorter->mapToSource(si.at(0)).row());
  else
    return QString();
}

void VcrossAddPlotDialog::initializeReftimePage(bool forward)
{
  ui->stack->setCurrentIndex(ReftimePage);

  const QString model = selectedModel();
  ui->reftimeLabelModel->setText(tr("Chosen model: %1")
      .arg(model));
  if (forward) {
    diutil::OverrideCursor waitCursor;
    const vcross::QtManager::vctime_v reftimes = vcrossm->getModelReferenceTimes(selectedModel().toStdString());
    QStringList rsl;
    for (size_t i=0; i<reftimes.size(); ++i)
      rsl << QString::fromStdString(reftimes[i].isoTime());

    referenceTimes->setStringList(rsl);
    if (referenceTimes->rowCount() > 0) {
      const QModelIndex latest = referenceTimes->index(referenceTimes->rowCount()-1, 0);
      ui->reftimeList->selectionModel()->setCurrentIndex(latest, QItemSelectionModel::ClearAndSelect);
    }
  }
  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  ui->buttonNext->setEnabled(isReftimeComplete());
  ui->buttonAdd->setEnabled(false);
}

bool VcrossAddPlotDialog::isReftimeComplete() const
{
  return (not ui->reftimeList->selectionModel()->selectedIndexes().isEmpty());
}

void VcrossAddPlotDialog::checkReftimeComplete()
{
  bool haveReftime = isReftimeComplete();
  ui->buttonNext->setEnabled(haveReftime);
}

QString VcrossAddPlotDialog::selectedReferenceTime() const
{
  const QModelIndexList si = ui->reftimeList->selectionModel()->selectedIndexes();
  if (si.size() == 1)
    return referenceTimes->stringList().at(si.at(0).row());
  else
    return QString();
}

void VcrossAddPlotDialog::initializePlotPage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stack->setCurrentIndex(PlotPage);

  const QString model = selectedModel();
  ui->plotLabelModel->setText(tr("Chosen model: %1")
      .arg(model));
  const QString referencetime = selectedReferenceTime();
  ui->plotLabelReftime->setText(tr("Chosen reference time: %1")
      .arg(referencetime));
  if (forward) {
    ui->plotFilter->clear();
    diutil::OverrideCursor waitCursor;

    const vcross::QtManager::vctime_t reftime(referencetime.toStdString());
    const std::vector<std::string> fields
        = vcrossm->getFieldNames(model.toStdString(), reftime, true);
    QStringList fsl;
    for (size_t i=0; i<fields.size(); ++i)
      fsl << QString::fromStdString(fields[i]);

    plotNames->setStringList(fsl);
  }
  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  ui->buttonNext->setEnabled(false);
  ui->buttonAdd->setEnabled(isPlotComplete());
}

void VcrossAddPlotDialog::onPlotFilter(const QString& text)
{
  plotSorter->setFilterFixedString(text);
}

bool VcrossAddPlotDialog::isPlotComplete() const
{
  return not ui->plotList->selectionModel()->selectedIndexes().isEmpty();
}

void VcrossAddPlotDialog::checkPlotComplete()
{
  bool havePlot = isPlotComplete();
  ui->buttonAdd->setEnabled(havePlot);
}

QStringList VcrossAddPlotDialog::selectedPlots() const
{
  const QStringList& plots = plotNames->stringList();
  QStringList fsl;
  const QModelIndexList si = ui->plotList->selectionModel()->selectedIndexes();
  for (int i=0; i<si.size(); ++i)
    fsl << plots.at(plotSorter->mapToSource(si.at(i)).row());
  return fsl;
}
