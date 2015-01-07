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
#include "vcross_qt/diVcrossSelectionManager.h"

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

VcrossAddPlotDialog::VcrossAddPlotDialog(QWidget* parent, VcrossSelectionManager* m)
  : QDialog(parent)
  , selectionManager(m)
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
    initializeModelPage(false);
}

void VcrossAddPlotDialog::onNext()
{
  const int page = ui->stack->currentIndex();
  if (page == ModelPage and isModelComplete())
    initializePlotPage(true);
}

void VcrossAddPlotDialog::onAdd()
{
  const int page = ui->stack->currentIndex();
  if (page != PlotPage or (not (isModelComplete() and isPlotComplete())))
    return;

  const std::string model = selectedModel().toStdString();
  const QStringList plots = selectedPlots();
  for (int i=0; i<plots.size(); ++i) {
    const std::string fld = plots.at(i).toStdString();
    const std::string opt = selectionManager->defaultOptions(model, fld, false);
    selectionManager->addField(model, fld, opt, selectionManager->countFields());
  }
  initializePlotPage(true);
}

void VcrossAddPlotDialog::initializeModelPage(bool forward)
{
  ui->stack->setCurrentIndex(ModelPage);

  if (forward) {
    ui->modelFilter->clear();
    modelNames->setStringList(selectionManager->allModels());
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

void VcrossAddPlotDialog::initializePlotPage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stack->setCurrentIndex(PlotPage);

  if (forward) {
    ui->plotFilter->clear();
    QString model = selectedModel();
    ui->plotLabelModel->setText(tr("Choose plot(s) for model '%1'").arg(model));

    diutil::OverrideCursor waitCursor;
    plotNames->setStringList(selectionManager->availableFields(model));
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
