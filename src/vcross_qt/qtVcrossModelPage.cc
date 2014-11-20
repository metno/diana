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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vcross_qt/qtVcrossModelPage.h"

#include "vcross_qt/qtVcrossWizard.h"

#include <QLineEdit>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VcrossModelPage"
#include <miLogger/miLogging.h>

VcrossModelPage::VcrossModelPage(QWidget* parent)
  : QWizardPage(parent)
  , modelFilter(new QLineEdit(this))
  , modelList(new QListView(this))
  , modelNames(new QStringListModel(this))
  , modelSorter(new QSortFilterProxyModel(this))
{
  modelSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  modelSorter->setSourceModel(modelNames);
  modelList->setModel(modelSorter);
  modelList->setSelectionMode(QAbstractItemView::SingleSelection);
  modelList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  connect(modelFilter, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterTextChanged(const QString&)));
  connect(modelList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SIGNAL(completeChanged()));
  connect(modelList, SIGNAL(activated(const QModelIndex&)), parent, SLOT(next()));
  
  retranslateUI();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(modelFilter);
  layout->addWidget(modelList);
  setLayout(layout);
}

void VcrossModelPage::retranslateUI()
{
  modelFilter->setPlaceholderText(tr("Filter models"));
}

void VcrossModelPage::initializePage()
{
  modelFilter->clear();
  modelNames->setStringList(static_cast<VcrossWizard*>(wizard())->getAllModels());
}

bool VcrossModelPage::isComplete() const
{
  return not modelList->selectionModel()->selectedIndexes().isEmpty();
}

QString VcrossModelPage::selectedModel() const
{
  const QModelIndexList si = modelList->selectionModel()->selectedIndexes();
  if (si.size() == 1)
    return modelNames->stringList().at(modelSorter->mapToSource(si.at(0)).row());
  else
    return QString();
}

void VcrossModelPage::onFilterTextChanged(const QString& text)
{
  modelSorter->setFilterFixedString(text);
}
