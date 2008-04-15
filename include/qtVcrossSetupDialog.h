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
#include <miString.h>
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
  bool close(bool alsoDelete);

private:
  VcrossManager * vcrossm;

  void initOptions(QWidget* parent);

  void setup(VcrossOptions *vcopt);
  void printSetup();
  void applySetup();

  QGridLayout* glayout;

  vector<VcrossSetup*> vcSetups;

  bool isInitialized;

  miString TEXTPLOT;
  miString FRAME;
  miString POSNAMES;
  miString LEVELNUMBERS;
  miString UPPERLEVEL;
  miString LOWERLEVEL;
  miString OTHERLEVELS;
  miString SURFACE;
  miString DISTANCE;
  miString GRIDPOS;
  miString GEOPOS;
  miString VERTGRID;
  miString MARKERLINES;
  miString VERTICALMARKER;
  miString EXTRAPOLP;
  miString BOTTOMEXT;
  miString THINARROWS;
  miString VERTICALTYPE;
  miString VHSCALE;
  miString STDVERAREA;
  miString STDHORAREA;
  miString BACKCOLOUR;
  miString ONMAPDRAW;
  miString HITMAPDRAW;

private slots:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();

signals:
  void SetupHide();
  void SetupApply();
  void showsource(const miString, const miString="");
};

#endif
