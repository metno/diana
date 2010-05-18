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
#ifndef VCROSSSETUPDIALOG_H
#define VCROSSSETUPDIALOG_H

#include <QDialog>

#include <diCommonTypes.h>
#include <puTools/miString.h>
#include <vector>

using namespace std;

class QGridLayout;
class VcrossManager;
class VcrossOptions;
class VcrossSetup;

/**
   \brief Dialogue to select Vertical Crossection background/diagram options

*/
class VcrossSetupDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  VcrossSetupDialog( QWidget* parent, VcrossManager* vm );
  void start();
protected:
  void closeEvent( QCloseEvent* );

private:
  VcrossManager * vcrossm;

  void initOptions(QWidget* parent);

  void setup(VcrossOptions *vcopt);
  void printSetup();
  void applySetup();

  QGridLayout* glayout;

  vector<VcrossSetup*> vcSetups;

  bool isInitialized;

  miutil::miString TEXTPLOT;
  miutil::miString FRAME;
  miutil::miString POSNAMES;
  miutil::miString LEVELNUMBERS;
  miutil::miString UPPERLEVEL;
  miutil::miString LOWERLEVEL;
  miutil::miString OTHERLEVELS;
  miutil::miString SURFACE;
  miutil::miString DISTANCE;
  miutil::miString GRIDPOS;
  miutil::miString GEOPOS;
  miutil::miString VERTGRID;
  miutil::miString MARKERLINES;
  miutil::miString VERTICALMARKER;
  miutil::miString EXTRAPOLP;
  miutil::miString BOTTOMEXT;
  miutil::miString THINARROWS;
  miutil::miString VERTICALTYPE;
  miutil::miString VHSCALE;
  miutil::miString STDVERAREA;
  miutil::miString STDHORAREA;
  miutil::miString BACKCOLOUR;
  miutil::miString ONMAPDRAW;
  miutil::miString HITMAPDRAW;

private slots:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();

signals:
  void SetupHide();
  void SetupApply();
  void showsource(const miutil::miString, const miutil::miString="");
};

#endif
