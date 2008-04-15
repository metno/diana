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

#include <diCommonTypes.h>
#include <miString.h>
#include <vector>

using namespace std;

class QFont;
class QPalette;
class VprofManager;
class QListWidget;
class QButtonGroup;
class ToggleButton;

/**
   \brief Dialogue to selecet Vertical Profile data sources

   Choose between observation types and prognostic models
*/
class VprofModelDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  VprofModelDialog( QWidget* parent, VprofManager * vm );
  void setSelection();
  bool close(bool alsoDelete);

private:
  VprofManager * vprofm;

  //qt widget
  QButtonGroup* modelfileBut;
  ToggleButton* modelButton;
  ToggleButton* fileButton;
  QListWidget * modelfileList;

  //dialog indices, changed when something is clicked
  int m_modelfileButIndex;

  miString ASFIELD;
  miString OBSTEMP;
  miString OBSPILOT;
  miString OBSAMDAR;

  //functions
  void updateModelfileList();
  void setModel();

private slots:
  void modelfileClicked(int);
  void refreshClicked();
  void deleteAllClicked();
  void helpClicked();
  void applyhideClicked();
  void applyClicked();

signals:
  void ModelHide();
  void ModelApply();
  void showsource(const miString, const miString="");
};

#endif
