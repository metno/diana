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

#include "vcross_qt/qtVcrossStylePage.h"

#include "vcross_v2/VcrossQtManager.h"

#include <QLabel>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VcrossStylePage"
#include <miLogger/miLogging.h>

VcrossStylePage::VcrossStylePage(QWidget* parent)
  : QWizardPage(parent)
  , fieldLabel(new QLabel(this))
{
  retranslateUI();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(fieldLabel);
  setLayout(layout);
}

void VcrossStylePage::retranslateUI()
{
  fieldLabel->setText(tr("Maybe it will be possible to edit field options here, later and only if exactly one field has been chosen."));
  fieldLabel->setWordWrap(true);
}

void VcrossStylePage::initializePage()
{
}

bool VcrossStylePage::isComplete() const
{
  return true;
}

std::string VcrossStylePage::getOptions()
{
  return "line.color=red";
}
