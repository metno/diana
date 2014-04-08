/*
  Diana - A Free Meteorological Visualisation Tool

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
#ifdef USE_VCROSS_V2
#include "vcross_v2/VcrossQtManager.h"
#else
#include "vcross_v1/diVcross1Manager.h"
#endif

#include <vector>

class VcrossOptions;
class VcrossSetupUI;
class QGridLayout;

/**
   \brief Dialogue to select Vertical Crossection background/diagram options
*/
class VcrossSetupDialog: public QDialog
{
  Q_OBJECT

public:
  VcrossSetupDialog(QWidget* parent, vcross::QtManager_p vm);
  void start();

protected:
  void closeEvent(QCloseEvent*);

private:
  vcross::QtManager_p vcrossm;

  void initOptions(QWidget* parent);

  void setup(vcross::VcrossOptions * vcopt);
  void printSetup();
  void applySetup();

  QGridLayout* glayout;

  std::vector<VcrossSetupUI*> vcSetups;

  bool isInitialized;

  std::string TEXTPLOT;
  std::string FRAME;
  std::string POSNAMES;
  std::string LEVELNUMBERS;
  std::string UPPERLEVEL;
  std::string LOWERLEVEL;
  std::string OTHERLEVELS;
  std::string SURFACE;
  std::string DISTANCE;
  std::string GRIDPOS;
  std::string GEOPOS;
  std::string VERTGRID;
  std::string MARKERLINES;
  std::string VERTICALMARKER;
  std::string EXTRAPOLP;
  std::string BOTTOMEXT;
  std::string THINARROWS;
  std::string VERTICALTYPE;
  std::string VHSCALE;
  std::string STDVERAREA;
  std::string STDHORAREA;
  std::string BACKCOLOUR;
  std::string ONMAPDRAW;
  std::string HITMAPDRAW;

private Q_SLOTS:
  void standardClicked();
  void helpClicked();
  void applyClicked();
  void applyhideClicked();

Q_SIGNALS:
  void SetupHide();
  void SetupApply();
  void showsource(const std::string&, const std::string& = "");
};

#endif
