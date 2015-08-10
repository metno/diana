/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifndef QTVCROSSREPLACEMODELDIALOG_H
#define QTVCROSSREPLACEMODELDIALOG_H

#include "vcross_v2/VcrossQtManager.h"

#include <QDialog>
#include <memory>

class Ui_VcrossReplaceModelDialog;

class VcrossReplaceModelDialog : public QDialog {
  Q_OBJECT

public:
  VcrossReplaceModelDialog(QWidget* parent, vcross::QtManager_p vsm);

public Q_SLOTS:
  void restart();

private:
  void setupUi();

  QStringList selectedPlots() const;
  QString selectedModel() const;
  QString selectedReferenceTime() const;

  void initializePlotsPage(bool forward);
  bool isPlotsComplete() const;

  void initializeModelPage(bool forward);
  bool isModelComplete() const;

  void initializeReftimePage(bool forward);
  bool isReftimeComplete() const;

private Q_SLOTS:
  void onBack();
  void onNext();
  void onReplace();

  void checkPlotsComplete();
  void checkModelComplete();
  void checkReftimeComplete();

private:
  enum { PlotsPage, ModelPage, ReftimePage };

  vcross::QtManager_p vcrossm;
  std::auto_ptr<Ui_VcrossReplaceModelDialog> ui;
};

#endif // QTVCROSSREPLACEMODELDIALOG_H
