/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include "qtUtility.h"
#ifdef USE_VCROSS_V2
#include "vcross_v2/VcrossOptions.h"
#define DISABLE_UNUSED_OPTIONS 1
#else
#include "vcross_v1/diVcross1Manager.h"
#include "vcross_v1/diVcross1Options.h"
#endif

#include "qtVcrossSetup.h"
#include "qtVcrossSetupDialog.h"
#include <boost/make_shared.hpp>
#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.VcrossSetupDialog"
#include <miLogger/miLogging.h>

VcrossSetupDialog::VcrossSetupDialog( QWidget* parent, vcross::QtManager_p vm )
  : QDialog(parent), vcrossm(vm)
{
  METLIBS_LOG_SCOPE();

  //caption to appear on top of dialog
  setWindowTitle( tr("Diana Vertical Crossections - settings"));

  // text constants
  TEXTPLOT       =  tr("Text").toStdString();
  FRAME          =  tr("Frame").toStdString();
#ifndef DISABLE_UNUSED_OPTIONS
  POSNAMES       =  tr("Position names").toStdString();
#endif
  LEVELNUMBERS   =  tr("Number for level").toStdString();
#ifndef DISABLE_UNUSED_OPTIONS
  UPPERLEVEL     =  tr("Top level").toStdString();
  LOWERLEVEL     =  tr("Bottom level").toStdString();
  OTHERLEVELS    =  tr("Other levels").toStdString();
#endif
  SURFACE        =  tr("Ground pressure").toStdString();
  DISTANCE       =  tr("Distance").toStdString();
  GEOPOS         =  tr("Geographical positions").toStdString();
  VERTGRID       =  tr("Vertical gridlines").toStdString();
#ifndef DISABLE_UNUSED_OPTIONS
  MARKERLINES    =  tr("Marker lines").toStdString();
  VERTICALMARKER =  tr("Vertical markers").toStdString();
  EXTRAPOLP      =  tr("Extrapolate to fixed P").toStdString();
  BOTTOMEXT      =  tr("Extrapolate to ocean floor").toStdString();
#endif
  THINARROWS     =  tr("Thin arrows").toStdString();
  VERTICALTYPE   =  tr("Vertical type").toStdString();
  VHSCALE        =  tr("Fixed vertical/horizontal scaling:").toStdString();
  STDVERAREA     =  tr("Default area vertically:").toStdString();
#ifndef DISABLE_UNUSED_OPTIONS
  STDHORAREA     =  tr("Default area horizontally:").toStdString();
#endif
  BACKCOLOUR     =  tr("Background colour").toStdString();
#ifndef DISABLE_UNUSED_OPTIONS
  ONMAPDRAW      =  tr("Crossections on map").toStdString();
  HITMAPDRAW     =  tr("Selected crossection on map").toStdString();
#endif

  //******** create the various QT widgets to appear in dialog *****

  vcSetups.clear();
  initOptions( this );


  //******** standard buttons **************************************


//Press enter -> apply and hide. Must declare setupapplyhide first.

  // push button to apply the selected setup and then hide dialog
  QPushButton * setupapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( setupapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  // push button to show help
  QPushButton * setuphelp = NormalPushButton( tr("Help"), this );
  connect(  setuphelp, SIGNAL(clicked()), SLOT( helpClicked()));

  // push button to set to default
  QPushButton * standard = NormalPushButton( tr("Default"), this );
  connect(  standard, SIGNAL(clicked()), SLOT( standardClicked()));

  // push button to hide dialog
  QPushButton * setuphide = NormalPushButton( tr("Hide"), this );
  connect( setuphide, SIGNAL(clicked()), SIGNAL(SetupHide()));

  // push button to apply the selected setup
  QPushButton * setupapply = NormalPushButton( tr("Apply"), this );
  connect(setupapply, SIGNAL(clicked()), SLOT( applyClicked()) );

  // *********** place all the widgets in layouts ****************

  //place buttons "oppdater", "hjelp" etc. in horizontal layout
  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget( setuphelp );
  hlayout1->addWidget( standard );

  //place buttons "utfr", "help" etc. in horizontal layout
  QHBoxLayout* hlayout2 = new QHBoxLayout();
  hlayout2->addWidget( setuphide );
  hlayout2->addWidget( setupapplyhide );
  hlayout2->addWidget( setupapply );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this );
  vlayout->addLayout( glayout );
  vlayout->addLayout( hlayout1 );
  vlayout->addLayout( hlayout2 );

  isInitialized=false;
}


void VcrossSetupDialog::initOptions(QWidget* parent)
{
  METLIBS_LOG_SCOPE();

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes/spinboxes
  int numrows= 29;
  glayout = new QGridLayout();
  glayout->setMargin( 5 );
  glayout->setSpacing( 2 );

  int nrow=0;

  QLabel* label1= new QLabel(tr("On/off"),parent);
  QLabel* label2= new QLabel(tr("Colour"),parent);
  QLabel* label3= new QLabel(tr("Line thickness"),parent);
  QLabel* label4= new QLabel(tr("Line type"),parent);
  glayout->addWidget(label1,nrow,0);
  glayout->addWidget(label2,nrow,1);
  glayout->addWidget(label3,nrow,2);
  glayout->addWidget(label4,nrow,3);
  nrow++;

  int n,opts;

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  vcSetups.push_back(new VcrossSetupUI(parent,TEXTPLOT,glayout,nrow++,opts));
#ifndef DISABLE_UNUSED_OPTIONS
  vcSetups.push_back(new VcrossSetupUI(parent,POSNAMES,glayout,nrow++,opts));
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  vcSetups.push_back(new VcrossSetupUI(parent,FRAME,glayout,nrow++,opts));

  opts= VcrossSetupUI::useOnOff;
  vcSetups.push_back(new VcrossSetupUI(parent,LEVELNUMBERS,glayout,nrow++,opts));

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
#ifndef DISABLE_UNUSED_OPTIONS
  vcSetups.push_back(new VcrossSetupUI(parent,UPPERLEVEL,glayout,nrow++,opts));
  vcSetups.push_back(new VcrossSetupUI(parent,LOWERLEVEL,glayout,nrow++,opts));
  vcSetups.push_back(new VcrossSetupUI(parent,OTHERLEVELS,glayout,nrow++,opts));
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  vcSetups.push_back(new VcrossSetupUI(parent,SURFACE,glayout,nrow++,opts));

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  vcSetups.push_back(new VcrossSetupUI(parent,VERTGRID,glayout,nrow++,opts));
#ifndef DISABLE_UNUSED_OPTIONS
  vcSetups.push_back(new VcrossSetupUI(parent,MARKERLINES,glayout,nrow++,opts));
  vcSetups.push_back(new VcrossSetupUI(parent,VERTICALMARKER,glayout,nrow++,opts));
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useTextChoice
#ifndef DISABLE_UNUSED_OPTIONS
      | VcrossSetupUI::useTextChoice2
#endif
);
  vcSetups.push_back(new VcrossSetupUI(parent,DISTANCE,glayout,nrow++,opts));
  std::vector<std::string> distunit;
  distunit.push_back("km");
  distunit.push_back("nm");
  n= vcSetups.size()-1;
  vcSetups[n]->defineTextChoice(distunit,0);
#ifndef DISABLE_UNUSED_OPTIONS
  std::vector<std::string> diststep;
  diststep.push_back("grid");
  diststep.push_back("1");
  diststep.push_back("10");
  diststep.push_back("100");
  vcSetups[n]->defineTextChoice2(diststep,0);
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  vcSetups.push_back(new VcrossSetupUI(parent,GEOPOS,glayout,nrow++,opts));
#ifndef DISABLE_UNUSED_OPTIONS
  opts= VcrossSetupUI::useOnOff;
  vcSetups.push_back(new VcrossSetupUI(parent,EXTRAPOLP,glayout,nrow++,opts));
  vcSetups.push_back(new VcrossSetupUI(parent,BOTTOMEXT,glayout,nrow++,opts));
#endif

  vcSetups.push_back(new VcrossSetupUI(parent,THINARROWS,glayout,nrow++,opts));

  nrow++;
  opts= VcrossSetupUI::useTextChoice;
  vcSetups.push_back(new VcrossSetupUI(parent,VERTICALTYPE,glayout,nrow++,opts));
  std::vector<std::string> vchoice;
  vchoice.push_back("Standard/hPa");
  vchoice.push_back("Standard/FL");
  vchoice.push_back("Pressure/hPa");
  vchoice.push_back("Pressure/FL");
  vchoice.push_back("Height/m");
  vchoice.push_back("Height/Ft");
  n= vcSetups.size()-1;
  vcSetups[n]->defineTextChoice(vchoice,0);

  nrow++;
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useValue);
  vcSetups.push_back(new VcrossSetupUI(parent,VHSCALE,glayout,nrow++,opts));
  n= vcSetups.size()-1;
//vcSetups[n]->defineValue(10,600,10,150,"","x");
  vcSetups[n]->defineValue(1,600,1,150,"","x");

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useMinValue | VcrossSetupUI::useMaxValue);
  vcSetups.push_back(new VcrossSetupUI(parent,STDVERAREA,glayout,nrow++,opts));
  n= vcSetups.size()-1;
  vcSetups[n]->defineMinValue(0,100,5,  0,"","%");
  vcSetups[n]->defineMaxValue(0,100,5,100,"","%");

#ifndef DISABLE_UNUSED_OPTIONS
  vcSetups.push_back(new VcrossSetupUI(parent,STDHORAREA,glayout,nrow++,opts));
  n= vcSetups.size()-1;
  vcSetups[n]->defineMinValue(0,100,5,  0,"","%");
  vcSetups[n]->defineMaxValue(0,100,5,100,"","%");
#endif

  nrow++;
  opts= VcrossSetupUI::useColour;
  vcSetups.push_back(new VcrossSetupUI(parent,BACKCOLOUR,glayout,nrow++,opts));

#ifndef DISABLE_UNUSED_OPTIONS
  nrow++;
  opts= (VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  vcSetups.push_back(new VcrossSetupUI(parent, ONMAPDRAW,glayout,nrow++,opts));
  vcSetups.push_back(new VcrossSetupUI(parent,HITMAPDRAW,glayout,nrow++,opts));
#endif

  if (nrow!=numrows) {
    METLIBS_LOG_DEBUG("==================================================");
    METLIBS_LOG_DEBUG("===== VcrossSetupDialog: glayout numrows= "<<numrows);
    METLIBS_LOG_DEBUG("=====                               nrow= "<<nrow);
    METLIBS_LOG_DEBUG("==================================================");
  }
}


void VcrossSetupDialog::standardClicked()
{
  //this slot is called when standard button pressed
  METLIBS_LOG_SCOPE();

  vcross::VcrossOptions * vcopt= new vcross::VcrossOptions(); // diana defaults
  setup(vcopt);
  delete vcopt;
  //emit SetupApply();
}


void VcrossSetupDialog::start()
{
  if (!isInitialized){
    // pointer to logged options (the first time)
    vcross::VcrossOptions * vcopt= vcrossm->getOptions();
    setup(vcopt);
    isInitialized=true;
  }
}


void VcrossSetupDialog::setup(vcross::VcrossOptions* vcopt)
{
  METLIBS_LOG_SCOPE();

  int n= vcSetups.size();

  for (int i=0; i<n; i++) {

    if (vcSetups[i]->name== TEXTPLOT) {
      vcSetups[i]->setChecked    (vcopt->pText);
      vcSetups[i]->setColour(vcopt->textColour);

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== POSNAMES) {
      vcSetups[i]->setChecked    (vcopt->pPositionNames);
      vcSetups[i]->setColour(vcopt->positionNamesColour);
#endif

    } else if (vcSetups[i]->name== FRAME) {
      vcSetups[i]->setChecked       (vcopt->pFrame);
      vcSetups[i]->setColour   (vcopt->frameColour);
      vcSetups[i]->setLinewidth(vcopt->frameLinewidth);
      vcSetups[i]->setLinetype (vcopt->frameLinetype);

    } else if (vcSetups[i]->name== LEVELNUMBERS) {
      vcSetups[i]->setChecked(vcopt->pLevelNumbers);

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== UPPERLEVEL) {
      vcSetups[i]->setChecked       (vcopt->pUpperLevel);
      vcSetups[i]->setColour   (vcopt->upperLevelColour);
      vcSetups[i]->setLinewidth(vcopt->upperLevelLinewidth);
      vcSetups[i]->setLinetype (vcopt->upperLevelLinetype);

    } else if (vcSetups[i]->name== LOWERLEVEL) {
      vcSetups[i]->setChecked       (vcopt->pLowerLevel);
      vcSetups[i]->setColour   (vcopt->lowerLevelColour);
      vcSetups[i]->setLinewidth(vcopt->lowerLevelLinewidth);
      vcSetups[i]->setLinetype (vcopt->lowerLevelLinetype);

    } else if (vcSetups[i]->name== OTHERLEVELS) {
      vcSetups[i]->setChecked       (vcopt->pOtherLevels);
      vcSetups[i]->setColour   (vcopt->otherLevelsColour);
      vcSetups[i]->setLinewidth(vcopt->otherLevelsLinewidth);
      vcSetups[i]->setLinetype (vcopt->otherLevelsLinetype);
#endif

    } else if (vcSetups[i]->name== SURFACE) {
      vcSetups[i]->setChecked       (vcopt->pSurface);
      vcSetups[i]->setColour   (vcopt->surfaceColour);
#ifndef DISABLE_UNUSED_OPTIONS
      vcSetups[i]->setLinewidth(vcopt->surfaceLinewidth);
      vcSetups[i]->setLinetype (vcopt->surfaceLinetype);
#endif

    } else if (vcSetups[i]->name== DISTANCE) {
      vcSetups[i]->setChecked         (vcopt->pDistance);
      vcSetups[i]->setColour     (vcopt->distanceColour);
      vcSetups[i]->setTextChoice (vcopt->distanceUnit);
      vcSetups[i]->setTextChoice2(vcopt->distanceStep);

    } else if (vcSetups[i]->name== GEOPOS) {
      vcSetups[i]->setChecked    (vcopt->pGeoPos);
      vcSetups[i]->setColour(vcopt->geoposColour);

    } else if (vcSetups[i]->name== VERTGRID) {
      vcSetups[i]->setChecked       (vcopt->pVerticalGridLines);
      vcSetups[i]->setColour   (vcopt->vergridColour);
      vcSetups[i]->setLinewidth(vcopt->vergridLinewidth);
      vcSetups[i]->setLinetype (vcopt->vergridLinetype);

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== MARKERLINES) {
      vcSetups[i]->setChecked       (vcopt->pMarkerlines);
      vcSetups[i]->setColour   (vcopt->markerlinesColour);
      vcSetups[i]->setLinewidth(vcopt->markerlinesLinewidth);
      vcSetups[i]->setLinetype (vcopt->markerlinesLinetype);

    } else if (vcSetups[i]->name== VERTICALMARKER) {
      vcSetups[i]->setChecked       (vcopt->pVerticalMarker);
      vcSetups[i]->setColour   (vcopt->verticalMarkerColour);
      vcSetups[i]->setLinewidth(vcopt->verticalMarkerLinewidth);
      vcSetups[i]->setLinetype (vcopt->verticalMarkerLinetype);

    } else if (vcSetups[i]->name== EXTRAPOLP) {
      vcSetups[i]->setChecked(vcopt->extrapolateFixedLevels);

    } else if (vcSetups[i]->name== BOTTOMEXT) {
      vcSetups[i]->setChecked(vcopt->extrapolateToBottom);
#endif

    } else if (vcSetups[i]->name== THINARROWS) {
      vcSetups[i]->setChecked(vcopt->thinArrows);

    } else if (vcSetups[i]->name== VERTICALTYPE) {
      vcSetups[i]->setTextChoice(vcopt->verticalType);

    } else if (vcSetups[i]->name== VHSCALE) {
      vcSetups[i]->setChecked   (vcopt->keepVerHorRatio);
      vcSetups[i]->setValue(vcopt->verHorRatio);

    } else if (vcSetups[i]->name== STDVERAREA) {
      vcSetups[i]->setChecked      (vcopt->stdVerticalArea);
      vcSetups[i]->setMinValue(vcopt->minVerticalArea);
      vcSetups[i]->setMaxValue(vcopt->maxVerticalArea);

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== STDHORAREA) {
      vcSetups[i]->setChecked      (vcopt->stdHorizontalArea);
      vcSetups[i]->setMinValue(vcopt->minHorizontalArea);
      vcSetups[i]->setMaxValue(vcopt->maxHorizontalArea);
#endif

    } else if (vcSetups[i]->name== BACKCOLOUR) {
      vcSetups[i]->setColour(vcopt->backgroundColour);

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== ONMAPDRAW) {
      vcSetups[i]->setColour   (vcopt->vcOnMapColour);
      vcSetups[i]->setLinewidth(vcopt->vcOnMapLinewidth);
      vcSetups[i]->setLinetype (vcopt->vcOnMapLinetype);

    } else if (vcSetups[i]->name== HITMAPDRAW) {
      vcSetups[i]->setColour   (vcopt->vcSelectedOnMapColour);
      vcSetups[i]->setLinewidth(vcopt->vcSelectedOnMapLinewidth);
      vcSetups[i]->setLinetype (vcopt->vcSelectedOnMapLinetype);
#endif

    } else {
      METLIBS_LOG_ERROR("VcrossSetupDialog::setup ERROR : " <<vcSetups[i]->name);
    }
  }
}


void VcrossSetupDialog::applySetup()
{
  METLIBS_LOG_SCOPE();

  vcross::VcrossOptions * vcopt= vcrossm->getOptions();

  int n= vcSetups.size();

  for (int i=0; i<n; i++) {

    if (vcSetups[i]->name== TEXTPLOT) {
      vcopt->pText=      vcSetups[i]->isChecked();
      vcopt->textColour= vcSetups[i]->getColour().name;

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== POSNAMES) {
      vcopt->pPositionNames=      vcSetups[i]->isChecked();
      vcopt->positionNamesColour= vcSetups[i]->getColour().name;
#endif

    } else if (vcSetups[i]->name== FRAME) {
      vcopt->pFrame=         vcSetups[i]->isChecked();
      vcopt->frameColour=    vcSetups[i]->getColour().name;
      vcopt->frameLinewidth= vcSetups[i]->getLinewidth();
      vcopt->frameLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== LEVELNUMBERS) {
      vcopt->pLevelNumbers= vcSetups[i]->isChecked();

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== UPPERLEVEL) {
      vcopt->pUpperLevel=         vcSetups[i]->isChecked();
      vcopt->upperLevelColour=    vcSetups[i]->getColour().name;
      vcopt->upperLevelLinewidth= vcSetups[i]->getLinewidth();
      vcopt->upperLevelLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== LOWERLEVEL) {
      vcopt->pLowerLevel=         vcSetups[i]->isChecked();
      vcopt->lowerLevelColour=    vcSetups[i]->getColour().name;
      vcopt->lowerLevelLinewidth= vcSetups[i]->getLinewidth();
      vcopt->lowerLevelLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== OTHERLEVELS) {
      vcopt->pOtherLevels=         vcSetups[i]->isChecked();
      vcopt->otherLevelsColour=    vcSetups[i]->getColour().name;
      vcopt->otherLevelsLinewidth= vcSetups[i]->getLinewidth();
      vcopt->otherLevelsLinetype=  vcSetups[i]->getLinetype();
#endif

    } else if (vcSetups[i]->name== SURFACE) {
      vcopt->pSurface=         vcSetups[i]->isChecked();
      vcopt->surfaceColour=    vcSetups[i]->getColour().name;
#ifndef DISABLE_UNUSED_OPTIONS
      vcopt->surfaceLinewidth= vcSetups[i]->getLinewidth();
      vcopt->surfaceLinetype=  vcSetups[i]->getLinetype ();
#endif

    } else if (vcSetups[i]->name== DISTANCE) {
      vcopt->pDistance=      vcSetups[i]->isChecked();
      vcopt->distanceColour= vcSetups[i]->getColour().name;
      vcopt->distanceUnit=   vcSetups[i]->getTextChoice();
#ifndef DISABLE_UNUSED_OPTIONS
      vcopt->distanceStep=   vcSetups[i]->getTextChoice2();
#endif

    } else if (vcSetups[i]->name== GEOPOS) {
      vcopt->pGeoPos=      vcSetups[i]->isChecked();
      vcopt->geoposColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== VERTGRID) {
      vcopt->pVerticalGridLines= vcSetups[i]->isChecked();
      vcopt->vergridColour=      vcSetups[i]->getColour().name;
      vcopt->vergridLinewidth=   vcSetups[i]->getLinewidth();
      vcopt->vergridLinetype=    vcSetups[i]->getLinetype ();

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== MARKERLINES) {
      vcopt->pMarkerlines=         vcSetups[i]->isChecked();
      vcopt->markerlinesColour=    vcSetups[i]->getColour().name;
      vcopt->markerlinesLinewidth= vcSetups[i]->getLinewidth();
      vcopt->markerlinesLinetype=  vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== VERTICALMARKER) {
      vcopt->pVerticalMarker=         vcSetups[i]->isChecked();
      vcopt->verticalMarkerColour=    vcSetups[i]->getColour().name;
      vcopt->verticalMarkerLinewidth= vcSetups[i]->getLinewidth();
      vcopt->verticalMarkerLinetype=  vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== EXTRAPOLP) {
      vcopt->extrapolateFixedLevels= vcSetups[i]->isChecked();

    } else if (vcSetups[i]->name== BOTTOMEXT) {
      vcopt->extrapolateToBottom= vcSetups[i]->isChecked();
#endif

    } else if (vcSetups[i]->name== THINARROWS) {
      vcopt->thinArrows= vcSetups[i]->isChecked();

    } else if (vcSetups[i]->name== VERTICALTYPE) {
      vcopt->verticalType= vcSetups[i]->getTextChoice();
      //tmp
      std::vector<std::string> tokens = miutil::split(vcopt->verticalType,"/");
      if ( tokens.size() == 2 ) {
        if ( tokens[0] == "Standard" ) {
          vcopt->verticalScale = "exner";
          vcopt->verticalCoordinate = "Pressure";
        } else {
          vcopt->verticalScale = "linear";
          vcopt->verticalCoordinate = tokens[0];
        }
        vcopt->verticalUnit = tokens[1];
      }

    } else if (vcSetups[i]->name== VHSCALE) {
      vcopt->keepVerHorRatio= vcSetups[i]->isChecked();
      vcopt->verHorRatio=     vcSetups[i]->getValue();

    } else if (vcSetups[i]->name== STDVERAREA) {
      vcopt->stdVerticalArea= vcSetups[i]->isChecked();
      vcopt->minVerticalArea= vcSetups[i]->getMinValue();
      vcopt->maxVerticalArea= vcSetups[i]->getMaxValue();

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== STDHORAREA) {
      vcopt->stdHorizontalArea= vcSetups[i]->isChecked();
      vcopt->minHorizontalArea= vcSetups[i]->getMinValue();
      vcopt->maxHorizontalArea= vcSetups[i]->getMaxValue();
#endif

    } else if (vcSetups[i]->name== BACKCOLOUR) {
      vcopt->backgroundColour= vcSetups[i]->getColour().name;

#ifndef DISABLE_UNUSED_OPTIONS
    } else if (vcSetups[i]->name== ONMAPDRAW) {
      vcopt->vcOnMapColour=    vcSetups[i]->getColour().name;
      vcopt->vcOnMapLinewidth= vcSetups[i]->getLinewidth();
      vcopt->vcOnMapLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== HITMAPDRAW) {
      vcopt->vcSelectedOnMapColour=    vcSetups[i]->getColour().name;
      vcopt->vcSelectedOnMapLinewidth= vcSetups[i]->getLinewidth();
      vcopt->vcSelectedOnMapLinetype=  vcSetups[i]->getLinetype();
#endif

    } else {
      METLIBS_LOG_ERROR("VcrossSetupDialog::applySetup ERROR : " <<vcSetups[i]->name);
    }

  }
  vcrossm->standardPart();
}


void VcrossSetupDialog::helpClicked()
{
  //this slot is called when help button pressed
  METLIBS_LOG_SCOPE();
  /*emit*/ showsource("ug_verticalcrosssections.html");
}


void VcrossSetupDialog::applyClicked()
{
  //this slot is called when applyhide button pressed
  METLIBS_LOG_SCOPE();
  applySetup();
  /*emit*/ SetupApply();
}


void VcrossSetupDialog::applyhideClicked()
{
  //this slot is called when applyhide button pressed
  METLIBS_LOG_SCOPE();

  applySetup();
  /*emit*/ SetupHide();
  /*emit*/ SetupApply();
}


void VcrossSetupDialog::closeEvent( QCloseEvent* e)
{
  /*emit*/ SetupHide();
}
