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

#include "vcross_qt/qtVcrossReplaceModelDialog.h"
#include "vcross_qt/qtVcrossModelPage.h"
#include "vcross_qt/qtVcrossReftimePage.h"

#include "diUtilities.h"
#include "qtUtility.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#define MILOGGER_CATEGORY "diana.VcrossReplaceModelDialog"
#include <miLogger/miLogging.h>

#include "vcross_replace_model_dialog.ui.h"

#include "forover.xpm"
#include "bakover.xpm"
#include "start.xpm"

VcrossReplaceModelDialog::VcrossReplaceModelDialog(QWidget* parent, vcross::QtManager_p m)
  : QDialog(parent)
  , vcrossm(m)
  , ui(new Ui_VcrossReplaceModelDialog)
{
  METLIBS_LOG_SCOPE();

  setupUi();
}

void VcrossReplaceModelDialog::setupUi()
{
  ui->setupUi(this);
  ui->stack->setCurrentIndex(0);

  ui->buttonRestart->setIcon(QPixmap(start_xpm));
  ui->buttonBack->setIcon(QPixmap(bakover_xpm));
  ui->buttonNext->setIcon(QPixmap(forward_xpm));

  ui->plotsPage->setManager(vcrossm);
  ui->modelPage->setManager(vcrossm);
  ui->reftimePage->setManager(vcrossm);

  connect(ui->plotsPage, SIGNAL(selectionChanged()),
      this, SLOT(checkPlotsComplete()));

  connect(ui->modelPage, SIGNAL(completeStatusChanged(bool)),
      this, SLOT(checkModelComplete()));
  connect(ui->modelPage, SIGNAL(requestNext()),
      this, SLOT(onNext()));

  connect(ui->reftimePage, SIGNAL(completeStatusChanged(bool)),
      this, SLOT(checkReftimeComplete()));
  connect(ui->reftimePage, SIGNAL(requestNext()),
      this, SLOT(onNext()));

  connect(ui->buttonRestart, SIGNAL(clicked()), this, SLOT(restart()));
  connect(ui->buttonBack, SIGNAL(clicked()), this, SLOT(onBack()));
  connect(ui->buttonNext, SIGNAL(clicked()), this, SLOT(onNext()));
  connect(ui->buttonReplace,  SIGNAL(clicked()), this, SLOT(onReplace()));
}

void VcrossReplaceModelDialog::restart()
{
  initializePlotsPage(true);
}

void VcrossReplaceModelDialog::onBack()
{
  const int page = ui->stack->currentIndex();
  if (page == ModelPage)
    initializePlotsPage(false);
  else if (page == ReftimePage)
    initializeModelPage(false);
}

void VcrossReplaceModelDialog::onNext()
{
  const int page = ui->stack->currentIndex();
  if (page == PlotsPage and isPlotsComplete())
    initializeModelPage(true);
  else if (page == ModelPage and isModelComplete())
    initializeReftimePage(true);
}

void VcrossReplaceModelDialog::onReplace()
{
  const int page = ui->stack->currentIndex();
  if (page != ReftimePage
      || (!(isPlotsComplete() && isModelComplete() && isReftimeComplete())))
  {
    return;
  }

  const QList<int> selected = ui->plotsPage->selected();
  if (!selected.isEmpty()) {
    const std::string model = selectedModel().toStdString();
    const miutil::miTime reftime(selectedReferenceTime().toStdString());
    vcrossm->fieldChangeStart(false);
    for (int i=0; i<selected.size(); ++i) {
      const int index = selected.at(i);
      const std::string fld = vcrossm->getFieldAt(index);
      const std::string opt = vcrossm->getOptionsAt(index);
      const vcross::QtManager::PlotSpec ps(model, reftime, fld);
      // first add, then remove to avoid empty list
      const int added = vcrossm->addField(ps, opt, index);
      if (added >= 0)
        vcrossm->removeField(index + 1);
    }
    vcrossm->fieldChangeDone();
    restart();
  }
}

void VcrossReplaceModelDialog::initializePlotsPage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stack->setCurrentIndex(PlotsPage);
  if (forward) {
    ui->plotsPage->selectAll();
  }
  ui->buttonRestart->setEnabled(false);
  ui->buttonBack->setEnabled(false);
  ui->buttonNext->setEnabled(isPlotsComplete());
  ui->buttonReplace->setEnabled(false);
}

bool VcrossReplaceModelDialog::isPlotsComplete() const
{
  return !ui->plotsPage->selected().isEmpty();
}

void VcrossReplaceModelDialog::checkPlotsComplete()
{
  bool havePlots = isPlotsComplete();
  ui->buttonNext->setEnabled(havePlots);
}

void VcrossReplaceModelDialog::initializeModelPage(bool forward)
{
  ui->stack->setCurrentIndex(ModelPage);
  ui->modelPage->initialize(forward);
  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  checkModelComplete();
  ui->buttonReplace->setEnabled(false);
}

bool VcrossReplaceModelDialog::isModelComplete() const
{
  return ui->modelPage->isComplete();
}

void VcrossReplaceModelDialog::checkModelComplete()
{
  bool haveModel = isModelComplete();
  ui->buttonNext->setEnabled(haveModel);
}

QString VcrossReplaceModelDialog::selectedModel() const
{
  return ui->modelPage->selected();
}

void VcrossReplaceModelDialog::initializeReftimePage(bool forward)
{
  ui->stack->setCurrentIndex(ReftimePage);
  ui->reftimePage->initialize(selectedModel(), forward);
  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  ui->buttonNext->setEnabled(false);
  checkReftimeComplete();
}

bool VcrossReplaceModelDialog::isReftimeComplete() const
{
  return ui->reftimePage->isComplete();
}

void VcrossReplaceModelDialog::checkReftimeComplete()
{
  bool haveReftime = isReftimeComplete();
  ui->buttonReplace->setEnabled(haveReftime);
}

QString VcrossReplaceModelDialog::selectedReferenceTime() const
{
  return ui->reftimePage->selected();
}
