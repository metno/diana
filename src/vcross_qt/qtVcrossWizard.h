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

#ifndef QTVCROSSWIZARD_H
#define QTVCROSSWIZARD_H

#include "diVcrossSelectionManager.h"

#include <QWizard>

class VcrossModelPage;
class VcrossFieldPage;
class VcrossStylePage;

/**
  \brief Dialogue for Vertical Crossection plotting

  Select model, field and plotting options.
  Can alter many, but not all, options from the setup file.
  Keeps user settings in the diana log file between sessions.
*/
class VcrossWizard : public QWizard {
  Q_OBJECT

public:
  VcrossWizard(QWidget* parent, VcrossSelectionManager* vsm);

  QStringList getAllModels();
  QString getSelectedModel();
  QStringList getAvailableFields();
  QStringList getSelectedFields();
  QString getFieldOptions(const QString& field);

private:
  VcrossSelectionManager* vsm;

  VcrossModelPage* modelPage;
  VcrossFieldPage* fieldPage;
  VcrossStylePage* stylePage;
};

#endif // QTVCROSSWIZARD_H
