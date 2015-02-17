/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

  //the constructor
  VprofModelDialog( QWidget* parent, VprofManager * vm );
  void updateModelfileList();

protected:
  void closeEvent( QCloseEvent* );

private:
  VprofManager * vprofm;

  QListWidget * modelfileList;
  QListWidget* reftimeWidget;
  QListWidget* selectedModelsWidget;

  //functions
  void setModel();
  QString getSelectedModelString();


private slots:
  void modelfilelistClicked(QListWidgetItem*);
  void reftimeWidgetClicked(QListWidgetItem*);
  void refreshClicked();
  void deleteClicked();
  void deleteAllClicked();
  void helpClicked();
  void applyhideClicked();
  void applyClicked();

signals:
  void ModelHide();
  void ModelApply();
  void showsource(const std::string, const std::string="");
};

#endif
