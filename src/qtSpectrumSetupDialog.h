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
#ifndef SPECTRUMSETUPDIALOG_H
#define SPECTRUMSETUPDIALOG_H

#include <QDialog>

#include <diCommonTypes.h>
#include <vector>

class QFont;
class QGridLayout;
class SpectrumManager;
class SpectrumOptions;
class VcrossSetupUI;

/**
   \brief Dialogue to select Wave Spectrum diagram and data options

*/
class SpectrumSetupDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  SpectrumSetupDialog( QWidget* parent, SpectrumManager* vm );
  void start();
protected:
  void closeEvent( QCloseEvent* );

private Q_SLOTS:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();

Q_SIGNALS:
  void SetupHide();
  void SetupApply();
  void showsource(const std::string&, const std::string& = ""); // activate help

private:
  void initOptions();
  void setup(SpectrumOptions *spopt);
  void printSetup();
  void applySetup();

private:
  SpectrumManager * spectrumm;

  QGridLayout* glayout;

  bool isInitialized;

  VcrossSetupUI* mSetupTEXTPLOT;
  VcrossSetupUI* mSetupFIXEDTEXT;
  VcrossSetupUI* mSetupFRAME;
  VcrossSetupUI* mSetupSPECTRUMLINES;
  VcrossSetupUI* mSetupSPECTRUMCOLOUR;
  VcrossSetupUI* mSetupENERGYLINE;
  VcrossSetupUI* mSetupENERGYCOLOUR;
  VcrossSetupUI* mSetupPLOTWIND;
  VcrossSetupUI* mSetupPLOTPEAKDIREC;
  VcrossSetupUI* mSetupFREQUENCYMAX;
  VcrossSetupUI* mSetupBACKCOLOUR;
};

#endif
