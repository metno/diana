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

#include "vcross_qt/qtVcrossWizard.h"

#include "vcross_qt/qtVcrossModelPage.h"
#include "vcross_qt/qtVcrossFieldPage.h"
#include "vcross_qt/qtVcrossStylePage.h"

#include "diUtilities.h"
#include "qtUtility.h"
#include "vcross_v2/VcrossQtManager.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <qcombobox.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QFrame>
#include <QVBoxLayout>

#include <cmath>

#define DISABLE_EXTREMES 1
#define DISABLE_PATTERNS 1
#define DISABLE_LINE_SMOOTHING 1

#define MILOGGER_CATEGORY "diana.VcrossWizard"
#include <miLogger/miLogging.h>

VcrossWizard::VcrossWizard(QWidget* parent, VcrossSelectionManager* m)
  : QWizard(parent)
  , vsm(m)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle( tr("Vertical Crossections"));

  addPage(modelPage = new VcrossModelPage(this));
  addPage(fieldPage = new VcrossFieldPage(this));
  addPage(stylePage = new VcrossStylePage(this));
}

QStringList VcrossWizard::getAllModels()
{
  return vsm->allModels();
}

QString VcrossWizard::getSelectedModel()
{
  return modelPage->selectedModel();
}

QStringList VcrossWizard::getAvailableFields()
{
  return vsm->availableFields(getSelectedModel());
}

QStringList VcrossWizard::getSelectedFields()
{
  return fieldPage->selectedFields();
}
