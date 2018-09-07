/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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
#ifndef VPROFMODELDIALOG_H
#define VPROFMODELDIALOG_H

#include "diPlotCommand.h"

#include <QDialog>

#include <vector>

class VprofManager;
class QListWidget;
class QListWidgetItem;

/**
   \brief Dialogue to selecet Vertical Profile data sources

   Choose between observation types and prognostic models
*/
class VprofModelDialog : public QDialog
{
  Q_OBJECT

public:
  VprofModelDialog(QWidget* parent, VprofManager* vm);
  void updateModelfileList();
  void getModel();

  PlotCommand_cpv getPlotCommands();

protected:
  void closeEvent(QCloseEvent*);

private:
  VprofManager* vprofm;

  QListWidget* modelfileList;
  QListWidget* reftimeWidget;
  QListWidget* selectedModelsWidget;

  QString getSelectedModelString();

private Q_SLOTS:
  void modelfilelistClicked(QListWidgetItem*);
  void reftimeWidgetClicked(QListWidgetItem*);
  void deleteClicked();
  void helpClicked();
  void applyhideClicked();
  void applyClicked();

public Q_SLOTS:
  void deleteAllClicked();

Q_SIGNALS:
  void ModelHide();
  void ModelApply();
  void showsource(const std::string, const std::string = "");
};

#endif
