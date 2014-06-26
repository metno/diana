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
#ifndef VPROFSETUPDIALOG_H
#define VPROFSETUPDIALOG_H

#include <diCommonTypes.h>
#include <QDialog>
#include <vector>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QTabWidget;
class VprofManager;
class VcrossSetupUI;
class VprofOptions;

/**
   \brief Dialogue to select Vertical Profile diagram and data options
*/
class VprofSetupDialog: public QDialog
{
  Q_OBJECT

public:
  VprofSetupDialog(QWidget* parent, VprofManager * vm);

  void start();
protected:
  void closeEvent( QCloseEvent* );

private:
  VprofManager * vprofm;

  //initialize tabs
  void initDatatab();
  void initDiagramtab();
  void initColourtab();

  void setup(VprofOptions *vpopt);
  void applySetup();

  //QT tab widgets
  QTabWidget* twd;
  QWidget * datatab;
  QWidget* diagramtab;
  QWidget* colourtab;

  QSpinBox * pressureSpinLow;
  QSpinBox * pressureSpinHigh;
  QSpinBox * temperatureSpinLow;
  QSpinBox * temperatureSpinHigh;

  QComboBox * colourbox1;
  QComboBox * colourbox2;
  QComboBox * colourbox3;
  QComboBox * colourbox4;

  //setup values
  int pressureMin;
  int pressureMax;
  int temperatureMin;
  int temperatureMax;

  bool isInitialized;

  VcrossSetupUI* mSetupTEMP;
  VcrossSetupUI* mSetupDEWPOINT;
  VcrossSetupUI* mSetupWIND;
  VcrossSetupUI* mSetupVERTWIND;
  VcrossSetupUI* mSetupRELHUM;
  VcrossSetupUI* mSetupDUCTING;
  VcrossSetupUI* mSetupKINDEX;
  VcrossSetupUI* mSetupSIGNWIND;
  VcrossSetupUI* mSetupPRESSLINES;
  VcrossSetupUI* mSetupLINEFLIGHT;
  VcrossSetupUI* mSetupTEMPLINES;
  VcrossSetupUI* mSetupDRYADIABATS;
  VcrossSetupUI* mSetupWETADIABATS;
  VcrossSetupUI* mSetupMIXINGRATIO;
  VcrossSetupUI* mSetupPTLABELS;
  VcrossSetupUI* mSetupFRAME;
  VcrossSetupUI* mSetupTEXT;
  VcrossSetupUI* mSetupFLIGHTLEVEL;
  VcrossSetupUI* mSetupFLIGHTLABEL;
  VcrossSetupUI* mSetupSEPWIND;
  VcrossSetupUI* mSetupCONDTRAIL;
  VcrossSetupUI* mSetupGEOPOS;
  VcrossSetupUI* mSetupPRESSRANGE;
  VcrossSetupUI* mSetupTEMPRANGE;
  VcrossSetupUI* mSetupBACKCOLOUR;
  std::vector<VcrossSetupUI*> mSetupData;

private Q_SLOTS:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();
  void setPressureMin(int);
  void setPressureMax(int);
  void setTemperatureMin(int);
  void setTemperatureMax(int);

Q_SIGNALS:
  void SetupHide();
  void SetupApply();
  void showsource(const std::string&, const std::string& = "");
};

#endif
