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

#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <diCommonTypes.h>
#include <miString.h>
#include <vector>

using namespace std;

class QFont;
class Q3GridLayout;
class Q3VBoxLayout;
class SpectrumManager;
class SpectrumOptions;
class SpectrumSetup;

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
  bool close(bool alsoDelete);

private:
  SpectrumManager * spectrumm;

  void initOptions(QWidget* parent);

  void setup(SpectrumOptions *spopt);
  void printSetup();
  void applySetup();

  Q3GridLayout* glayout;
  Q3VBoxLayout* vlayout;

  vector<SpectrumSetup*> spSetups;

  bool isInitialized;

  miString TEXTPLOT;
  miString FIXEDTEXT;
  miString FRAME;
  miString SPECTRUMLINES;
  miString SPECTRUMCOLOUR;
  miString ENERGYLINE;
  miString ENERGYCOLOUR;
  miString PLOTWIND;
  miString PLOTPEAKDIREC;
  miString FREQUENCYMAX;
  miString BACKCOLOUR;

private slots:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();

signals:
  void SetupHide();
  void SetupApply();
  void showdoc(const miString); // activate help
};

#endif
