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

#include "vcross_qt/qtVcrossFieldPage.h"

#include "vcross_qt/qtVcrossWizard.h"

#include "qtUtility.h"

#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VcrossFieldPage"
#include <miLogger/miLogging.h>

VcrossFieldPage::VcrossFieldPage(QWidget* parent)
  : QWizardPage(parent)
  , fieldFilter(new QLineEdit(this))
  , fieldList(new QListView(this))
  , fieldNames(new QStringListModel(this))
  , fieldSorter(new QSortFilterProxyModel(this))
{
  METLIBS_LOG_SCOPE();
  fieldSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  fieldSorter->setSourceModel(fieldNames);
  fieldList->setModel(fieldSorter);
  fieldList->setSelectionMode(QAbstractItemView::MultiSelection);
  fieldList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  connect(fieldFilter, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterTextChanged(const QString&)));
  connect(fieldList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SIGNAL(completeChanged()));
  connect(fieldList, SIGNAL(activated(const QModelIndex&)), parent, SLOT(next()));

  retranslateUI();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(fieldFilter);
  layout->addWidget(fieldList);
  setLayout(layout);
}

void VcrossFieldPage::retranslateUI()
{
  setTitle(tr("Choose field(s)"));
  fieldFilter->setPlaceholderText(tr("Filter fields"));
}

void VcrossFieldPage::initializePage()
{
  METLIBS_LOG_SCOPE();
  VcrossWizard* vw = static_cast<VcrossWizard*>(wizard());

  const QString model = vw ? vw->getSelectedModel() : "?";
  setSubTitle(tr("Choose field(s) for model '%1'").arg(model));

  fieldFilter->clear();
  diutil::OverrideCursor waitCursor;
  fieldNames->setStringList(vw->getAvailableFields());
}

bool VcrossFieldPage::isComplete() const
{
  return not fieldList->selectionModel()->selectedIndexes().isEmpty();
}

QStringList VcrossFieldPage::selectedFields() const
{
  const QStringList& fields = fieldNames->stringList();
  QStringList fsl;
  const QModelIndexList si = fieldList->selectionModel()->selectedIndexes();
  for (int i=0; i<si.size(); ++i)
    fsl << fields.at(fieldSorter->mapToSource(si.at(i)).row());
  return fsl;
}

void VcrossFieldPage::onFilterTextChanged(const QString& text)
{
  fieldSorter->setFilterFixedString(text);
}
