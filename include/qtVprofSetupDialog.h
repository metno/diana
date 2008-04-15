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

#include <QDialog>

#include <diCommonTypes.h>
#include <miString.h>
#include <vector>

using namespace std;

class QComboBox;
class QCheckBox;
class QSpinBox;
class QTabWidget;
class VprofManager;
class VcrossSetup;
class VprofOptions;

/**
   \brief Dialogue to select Vertical Profile diagram and data options

*/
class VprofSetupDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  VprofSetupDialog( QWidget* parent, VprofManager * vm );
  void start();
  bool close(bool alsoDelete);

private:
  VprofManager * vprofm;

  //initialize tabs
  void initDatatab();
  void initDiagramtab();
  void initColourtab();

  void setup(VprofOptions *vpopt);
  void printSetup();
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

  vector <VcrossSetup*> vpSetups;

  bool isInitialized;

  miString TEMP;
  miString DEWPOINT;
  miString WIND;
  miString VERTWIND;
  miString RELHUM;
  miString DUCTING;
  miString KINDEX;
  miString SIGNWIND;
  miString PRESSLINES;
  miString LINEFLIGHT;
  miString TEMPLINES;
  miString DRYADIABATS;
  miString WETADIABATS;
  miString MIXINGRATIO;
  miString PTLABELS;
  miString FRAME;
  miString TEXT;
  miString FLIGHTLEVEL;
  miString FLIGHTLABEL;
  miString SEPWIND;
  miString CONDTRAIL;
  miString GEOPOS;
  miString PRESSRANGE;
  miString TEMPRANGE;
  miString BACKCOLOUR;

private slots:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();
  void setPressureMin(int);
  void setPressureMax(int);
  void setTemperatureMin(int);
  void setTemperatureMax(int);

signals:
  void SetupHide();
  void SetupApply();
  void showsource(const miString, const miString="");
};

#endif
