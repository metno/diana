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
#ifndef SPECTRUMMODELDIALOG_H
#define SPECTRUMMODELDIALOG_H

#include <qdialog.h>
#include <diCommonTypes.h>
#include <miString.h>
#include <vector>

using namespace std;

class QFont;
class QPalette;
class SpectrumManager;
class Q3ListBox;
class Q3ButtonGroup;
class ToggleButton;

/**
   \brief Dialogue to selecet Wave Spectrum data sources

*/
class SpectrumModelDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  SpectrumModelDialog( QWidget* parent, SpectrumManager * vm );
  void setSelection();
  bool close(bool alsoDelete);

private:
  SpectrumManager * spectrumm;

  //qt widget
  Q3ButtonGroup* modelfileBut;
  ToggleButton* modelButton;
  ToggleButton* fileButton;
  Q3ListBox * modelfileList;

  //dialog indices, changed when something is clicked
  int m_modelfileButIndex;

  miString ASFIELD;
  miString OBS;

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
  void showdoc(const miString); // activate help
};

#endif
