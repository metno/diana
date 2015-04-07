/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#include "vcross_v2/VcrossOptions.h"
#define DISABLE_UNUSED_OPTIONS 1

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
  setWindowTitle( tr("Diana Vertical Crossections - settings"));

  //******** create the various QT widgets to appear in dialog *****

  initOptions();

  //******** standard buttons **************************************
  //Press enter -> apply and hide. Must declare setupapplyhide first.

  // push button to apply the selected setup and then hide dialog
  QPushButton * setupapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( setupapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  // push button to show help
  QPushButton * setuphelp = NormalPushButton( tr("Help"), this );
  connect(setuphelp, SIGNAL(clicked()), SLOT( helpClicked()));

  // push button to set to default
  QPushButton * standard = NormalPushButton( tr("Default"), this );
  connect(  standard, SIGNAL(clicked()), SLOT( standardClicked()));

  // push button to hide dialog
  QPushButton * setuphide = NormalPushButton( tr("Hide"), this );
  connect(setuphide, SIGNAL(clicked()), SLOT(reject()));

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
  hlayout2->addWidget(setuphide);
  hlayout2->addWidget(setupapplyhide);
  hlayout2->addWidget(setupapply);

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this );
  vlayout->addLayout(glayout);
  vlayout->addLayout(hlayout1);
  vlayout->addLayout(hlayout2);

  isInitialized=false;
}


void VcrossSetupDialog::initOptions()
{
  METLIBS_LOG_SCOPE();

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes/spinboxes
  int numrows= 29;
  glayout = new QGridLayout();
  glayout->setMargin( 5 );
  glayout->setSpacing( 2 );

  int nrow=0;

  QLabel* label1= new QLabel(tr("On/off"), this);
  QLabel* label2= new QLabel(tr("Colour"), this);
  QLabel* label3= new QLabel(tr("Line thickness"), this);
  QLabel* label4= new QLabel(tr("Line type"), this);
  glayout->addWidget(label1,nrow,0);
  glayout->addWidget(label2,nrow,1);
  glayout->addWidget(label3,nrow,2);
  glayout->addWidget(label4,nrow,3);
  nrow++;

  int opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  mSetupTEXTPLOT = new VcrossSetupUI(this, tr("Text"), glayout, nrow++, opts);
#ifndef DISABLE_UNUSED_OPTIONS
  mSetupPOSNAMES = new VcrossSetupUI(this, tr("Position names"), glayout, nrow++, opts);
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
      VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupFRAME = new VcrossSetupUI(this, tr("Frame"), glayout, nrow++, opts);

  opts= VcrossSetupUI::useOnOff;
  mSetupLEVELNUMBERS = new VcrossSetupUI(this, tr("Number for level"), glayout, nrow++, opts);

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
      VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
#ifndef DISABLE_UNUSED_OPTIONS
  mSetupUPPERLEVEL = new VcrossSetupUI(this, tr("Top level"), glayout, nrow++, opts);
  mSetupLOWERLEVEL = new VcrossSetupUI(this, tr("Bottom level"), glayout, nrow++, opts);
  mSetupOTHERLEVELS = new VcrossSetupUI(this, tr("Other levels"), glayout, nrow++, opts);
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  mSetupSURFACE = new VcrossSetupUI(this, tr("Surface Pressure (Topography)"), glayout, nrow++, opts);

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
      VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupINFLIGHT = new VcrossSetupUI(this, tr("Inflight lines"), glayout, nrow++, opts);
  mSetupHORGRID = new VcrossSetupUI(this, tr("Horizontal gridlines"), glayout, nrow++, opts);
#ifndef DISABLE_UNUSED_OPTIONS
  mSetupVERTGRID = new VcrossSetupUI(this, tr("Vertical gridlines"), glayout, nrow++, opts);
  mSetupMARKERLINES = new VcrossSetupUI(this, tr("Marker lines"), glayout, nrow++, opts);
  mSetupVERTICALMARKER = new VcrossSetupUI(this, tr("Vertical markers"), glayout, nrow++, opts);
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
      VcrossSetupUI::useTextChoice
#ifndef DISABLE_UNUSED_OPTIONS
      | VcrossSetupUI::useTextChoice2
#endif
    );
  mSetupDISTANCE = new VcrossSetupUI(this, tr("Distance"), glayout, nrow++, opts);
  std::vector<std::string> distunit;
  distunit.push_back("km");
  distunit.push_back("nm");
  mSetupDISTANCE->defineTextChoice(distunit,0);

#ifndef DISABLE_UNUSED_OPTIONS
  std::vector<std::string> diststep;
  diststep.push_back("grid");
  diststep.push_back("1");
  diststep.push_back("10");
  diststep.push_back("100");
  mSetupDISTANCE->defineTextChoice2(diststep,0);
#endif

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  mSetupGEOPOS = new VcrossSetupUI(this, tr("Geographical positions"), glayout, nrow++, opts);

  opts= (VcrossSetupUI::useOnOff);
  mSetupCOMPASS = new VcrossSetupUI(this, tr("Direction indicators"), glayout, nrow++, opts);

#ifndef DISABLE_UNUSED_OPTIONS
  opts= VcrossSetupUI::useOnOff;
  mSetupEXTRAPOLP = new VcrossSetupUI(this, tr("Extrapolate to fixed P"), glayout, nrow++, opts);
  mSetupBOTTOMEXT = new VcrossSetupUI(this, tr("Extrapolate to ocean floor"), glayout, nrow++, opts);
#endif

  nrow++;
  opts= VcrossSetupUI::useTextChoice;
  mSetupVERTICALTYPE = new VcrossSetupUI(this, tr("Vertical type"), glayout, nrow++, opts);
  std::vector<std::string> vchoice;
  vchoice.push_back("Standard/hPa");
  vchoice.push_back("Standard/FL");
  vchoice.push_back("Pressure/hPa");
  vchoice.push_back("Pressure/FL");
  vchoice.push_back("Altitude/m");
  vchoice.push_back("Altitude/Ft");
  mSetupVERTICALTYPE->defineTextChoice(vchoice,0);

  nrow++;
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useValue);
  mSetupVHSCALE = new VcrossSetupUI(this, tr("Fixed vertical/horizontal scaling:"), glayout, nrow++, opts);
  mSetupVHSCALE->defineValue(1,600,1,150,"","x");

#ifndef DISABLE_UNUSED_OPTIONS
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useMinValue | VcrossSetupUI::useMaxValue);
  mSetupSTDVERAREA = new VcrossSetupUI(this, tr("Default area vertically:"), glayout, nrow++, opts);
  mSetupSTDVERAREA->defineMinValue(0,100,5,  0,"","%");
  mSetupSTDVERAREA->defineMaxValue(0,100,5,100,"","%");

  mSetupSTDHORAREA = new VcrossSetupUI(this, tr("Default area horizontally:"), glayout, nrow++, opts);
  mSetupSTDHORAREA->defineMinValue(0,100,5,  0,"","%");
  mSetupSTDHORAREA->defineMaxValue(0,100,5,100,"","%");
#endif

  nrow++;
  opts= VcrossSetupUI::useColour;
  mSetupBACKCOLOUR = new VcrossSetupUI(this, tr("Background colour"), glayout, nrow++, opts);

#ifndef DISABLE_UNUSED_OPTIONS
  nrow++;
  opts= (VcrossSetupUI::useColour |
      VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupONMAPDRAW = new VcrossSetupUI(this, tr("Crossections on map"), glayout, nrow++, opts);
  mSetupHITMAPDRAW = new VcrossSetupUI(this, tr("Selected crossection on map"), glayout, nrow++, opts);
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
  METLIBS_LOG_SCOPE();

  vcross::VcrossOptions * vcopt= new vcross::VcrossOptions(); // diana defaults
  setup(vcopt);
  delete vcopt;
  //emit SetupApply();
}


void VcrossSetupDialog::start()
{
  if (isInitialized)
    return;

  // pointer to logged options (the first time)
  vcross::VcrossOptions* vcopt = vcrossm->getOptions();
  setup(vcopt);
  isInitialized = true;
}


void VcrossSetupDialog::setup(vcross::VcrossOptions* vcopt)
{
  mSetupTEXTPLOT->setChecked    (vcopt->pText);
  mSetupTEXTPLOT->setColour(vcopt->textColour);

#ifndef DISABLE_UNUSED_OPTIONS
  mSetupPOSNAMES->setChecked    (vcopt->pPositionNames);
  mSetupPOSNAMES->setColour(vcopt->positionNamesColour);
#endif

  mSetupFRAME->setChecked       (vcopt->pFrame);
  mSetupFRAME->setColour   (vcopt->frameColour);
  mSetupFRAME->setLinewidth(vcopt->frameLinewidth);
  mSetupFRAME->setLinetype (vcopt->frameLinetype);

  mSetupLEVELNUMBERS->setChecked(vcopt->pLevelNumbers);

#ifndef DISABLE_UNUSED_OPTIONS
  mSetupUPPERLEVEL->setChecked       (vcopt->pUpperLevel);
  mSetupUPPERLEVEL->setColour   (vcopt->upperLevelColour);
  mSetupUPPERLEVEL->setLinewidth(vcopt->upperLevelLinewidth);
  mSetupUPPERLEVEL->setLinetype (vcopt->upperLevelLinetype);

  mSetupLOWERLEVEL->setChecked       (vcopt->pLowerLevel);
  mSetupLOWERLEVEL->setColour   (vcopt->lowerLevelColour);
  mSetupLOWERLEVEL->setLinewidth(vcopt->lowerLevelLinewidth);
  mSetupLOWERLEVEL->setLinetype (vcopt->lowerLevelLinetype);

  mSetupOTHERLEVELS->setChecked       (vcopt->pOtherLevels);
  mSetupOTHERLEVELS->setColour   (vcopt->otherLevelsColour);
  mSetupOTHERLEVELS->setLinewidth(vcopt->otherLevelsLinewidth);
  mSetupOTHERLEVELS->setLinetype (vcopt->otherLevelsLinetype);
#endif

  mSetupSURFACE->setChecked       (vcopt->pSurface);
  mSetupSURFACE->setColour   (vcopt->surfaceColour);
#ifndef DISABLE_UNUSED_OPTIONS
  mSetupSURFACE->setLinewidth(vcopt->surfaceLinewidth);
  mSetupSURFACE->setLinetype (vcopt->surfaceLinetype);
#endif

  mSetupINFLIGHT->setChecked  (vcopt->pInflight);
  mSetupINFLIGHT->setColour   (vcopt->inflightColour);
  mSetupINFLIGHT->setLinewidth(vcopt->inflightLinewidth);
  mSetupINFLIGHT->setLinetype (vcopt->inflightLinetype);

  mSetupDISTANCE->setChecked         (vcopt->pDistance);
  mSetupDISTANCE->setColour     (vcopt->distanceColour);
  mSetupDISTANCE->setTextChoice (vcopt->distanceUnit);
#ifndef DISABLE_UNUSED_OPTIONS
  mSetupDISTANCE->setTextChoice2(vcopt->distanceStep);
#endif

  mSetupGEOPOS->setChecked (vcopt->pGeoPos);
  mSetupGEOPOS->setColour  (vcopt->geoposColour);

  mSetupCOMPASS->setChecked(vcopt->pCompass);

  mSetupHORGRID->setChecked  (vcopt->pHorizontalGridLines);
  mSetupHORGRID->setColour   (vcopt->horgridColour);
  mSetupHORGRID->setLinewidth(vcopt->horgridLinewidth);
  mSetupHORGRID->setLinetype (vcopt->horgridLinetype);

#ifndef DISABLE_UNUSED_OPTIONS
  mSetupVERTGRID->setChecked  (vcopt->pVerticalGridLines);
  mSetupVERTGRID->setColour   (vcopt->vergridColour);
  mSetupVERTGRID->setLinewidth(vcopt->vergridLinewidth);
  mSetupVERTGRID->setLinetype (vcopt->vergridLinetype);

  mSetupMARKERLINES->setChecked       (vcopt->pMarkerlines);
  mSetupMARKERLINES->setColour   (vcopt->markerlinesColour);
  mSetupMARKERLINES->setLinewidth(vcopt->markerlinesLinewidth);
  mSetupMARKERLINES->setLinetype (vcopt->markerlinesLinetype);

  mSetupVERTICALMARKER->setChecked       (vcopt->pVerticalMarker);
  mSetupVERTICALMARKER->setColour   (vcopt->verticalMarkerColour);
  mSetupVERTICALMARKER->setLinewidth(vcopt->verticalMarkerLinewidth);
  mSetupVERTICALMARKER->setLinetype (vcopt->verticalMarkerLinetype);

  mSetupEXTRAPOLP->setChecked(vcopt->extrapolateFixedLevels);

  mSetupBOTTOMEXT->setChecked(vcopt->extrapolateToBottom);
#endif

  mSetupVERTICALTYPE->setTextChoice(vcopt->verticalType);

  mSetupVHSCALE->setChecked   (vcopt->keepVerHorRatio);
  mSetupVHSCALE->setValue(vcopt->verHorRatio);

#ifndef DISABLE_UNUSED_OPTIONS
  mSetupSTDVERAREA->setChecked      (vcopt->stdVerticalArea);
  mSetupSTDVERAREA->setMinValue(vcopt->minVerticalArea);
  mSetupSTDVERAREA->setMaxValue(vcopt->maxVerticalArea);

  mSetupSTDHORAREA->setChecked      (vcopt->stdHorizontalArea);
  mSetupSTDHORAREA->setMinValue(vcopt->minHorizontalArea);
  mSetupSTDHORAREA->setMaxValue(vcopt->maxHorizontalArea);
#endif

  mSetupBACKCOLOUR->setColour(vcopt->backgroundColour);

#ifndef DISABLE_UNUSED_OPTIONS
  mSetupONMAPDRAW->setColour   (vcopt->vcOnMapColour);
  mSetupONMAPDRAW->setLinewidth(vcopt->vcOnMapLinewidth);
  mSetupONMAPDRAW->setLinetype (vcopt->vcOnMapLinetype);

  mSetupHITMAPDRAW->setColour   (vcopt->vcSelectedOnMapColour);
  mSetupHITMAPDRAW->setLinewidth(vcopt->vcSelectedOnMapLinewidth);
  mSetupHITMAPDRAW->setLinetype (vcopt->vcSelectedOnMapLinetype);
#endif
}


void VcrossSetupDialog::applySetup()
{
  METLIBS_LOG_SCOPE();

  vcross::VcrossOptions* vcopt = vcrossm->getOptions();

  vcopt->pText=      mSetupTEXTPLOT->isChecked();
  vcopt->textColour= mSetupTEXTPLOT->getColour().name;

#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->pPositionNames=      mSetupPOSNAMES->isChecked();
  vcopt->positionNamesColour= mSetupPOSNAMES->getColour().name;
#endif

  vcopt->pFrame=         mSetupFRAME->isChecked();
  vcopt->frameColour=    mSetupFRAME->getColour().name;
  vcopt->frameLinewidth= mSetupFRAME->getLinewidth();
  vcopt->frameLinetype=  mSetupFRAME->getLinetype();

  vcopt->pLevelNumbers= mSetupLEVELNUMBERS->isChecked();

#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->pUpperLevel=         mSetupUPPERLEVEL->isChecked();
  vcopt->upperLevelColour=    mSetupUPPERLEVEL->getColour().name;
  vcopt->upperLevelLinewidth= mSetupUPPERLEVEL->getLinewidth();
  vcopt->upperLevelLinetype=  mSetupUPPERLEVEL->getLinetype();

  vcopt->pLowerLevel=         mSetupLOWERLEVEL->isChecked();
  vcopt->lowerLevelColour=    mSetupLOWERLEVEL->getColour().name;
  vcopt->lowerLevelLinewidth= mSetupLOWERLEVEL->getLinewidth();
  vcopt->lowerLevelLinetype=  mSetupLOWERLEVEL->getLinetype();

  vcopt->pOtherLevels=         mSetupOTHERLEVELS->isChecked();
  vcopt->otherLevelsColour=    mSetupOTHERLEVELS->getColour().name;
  vcopt->otherLevelsLinewidth= mSetupOTHERLEVELS->getLinewidth();
  vcopt->otherLevelsLinetype=  mSetupOTHERLEVELS->getLinetype();
#endif

  vcopt->pSurface=         mSetupSURFACE->isChecked();
  vcopt->surfaceColour=    mSetupSURFACE->getColour().name;
#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->surfaceLinewidth= mSetupSURFACE->getLinewidth();
  vcopt->surfaceLinetype=  mSetupSURFACE->getLinetype ();
#endif

  vcopt->pInflight        = mSetupINFLIGHT->isChecked();
  vcopt->inflightColour    = mSetupINFLIGHT->getColour().name;
  vcopt->inflightLinewidth = mSetupINFLIGHT->getLinewidth();
  vcopt->inflightLinetype  = mSetupINFLIGHT->getLinetype ();

  vcopt->pDistance=      mSetupDISTANCE->isChecked();
  vcopt->distanceColour= mSetupDISTANCE->getColour().name;
  vcopt->distanceUnit=   mSetupDISTANCE->getTextChoice();
#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->distanceStep=   mSetupDISTANCE->getTextChoice2();
#endif

  vcopt->pGeoPos=      mSetupGEOPOS->isChecked();
  vcopt->geoposColour= mSetupGEOPOS->getColour().name;

  vcopt->pCompass=     mSetupCOMPASS->isChecked();

  vcopt->pHorizontalGridLines= mSetupHORGRID->isChecked();
  vcopt->horgridColour=        mSetupHORGRID->getColour().name;
  vcopt->horgridLinewidth=     mSetupHORGRID->getLinewidth();
  vcopt->horgridLinetype=      mSetupHORGRID->getLinetype ();

#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->pVerticalGridLines= mSetupVERTGRID->isChecked();
  vcopt->vergridColour=      mSetupVERTGRID->getColour().name;
  vcopt->vergridLinewidth=   mSetupVERTGRID->getLinewidth();
  vcopt->vergridLinetype=    mSetupVERTGRID->getLinetype ();

  vcopt->pMarkerlines=         mSetupMARKERLINES->isChecked();
  vcopt->markerlinesColour=    mSetupMARKERLINES->getColour().name;
  vcopt->markerlinesLinewidth= mSetupMARKERLINES->getLinewidth();
  vcopt->markerlinesLinetype=  mSetupMARKERLINES->getLinetype ();

  vcopt->pVerticalMarker=         mSetupVERTICALMARKER->isChecked();
  vcopt->verticalMarkerColour=    mSetupVERTICALMARKER->getColour().name;
  vcopt->verticalMarkerLinewidth= mSetupVERTICALMARKER->getLinewidth();
  vcopt->verticalMarkerLinetype=  mSetupVERTICALMARKER->getLinetype ();

  vcopt->extrapolateFixedLevels= mSetupEXTRAPOLP->isChecked();

  vcopt->extrapolateToBottom= mSetupBOTTOMEXT->isChecked();
#endif

  { vcopt->verticalType= mSetupVERTICALTYPE->getTextChoice();
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
  }

  vcopt->keepVerHorRatio= mSetupVHSCALE->isChecked();
  vcopt->verHorRatio=     mSetupVHSCALE->getValue();

#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->stdVerticalArea= mSetupSTDVERAREA->isChecked();
  vcopt->minVerticalArea= mSetupSTDVERAREA->getMinValue();
  vcopt->maxVerticalArea= mSetupSTDVERAREA->getMaxValue();

  vcopt->stdHorizontalArea= mSetupSTDHORAREA->isChecked();
  vcopt->minHorizontalArea= mSetupSTDHORAREA->getMinValue();
  vcopt->maxHorizontalArea= mSetupSTDHORAREA->getMaxValue();
#endif

  vcopt->backgroundColour= mSetupBACKCOLOUR->getColour().name;

#ifndef DISABLE_UNUSED_OPTIONS
  vcopt->vcOnMapColour=    mSetupONMAPDRAW->getColour().name;
  vcopt->vcOnMapLinewidth= mSetupONMAPDRAW->getLinewidth();
  vcopt->vcOnMapLinetype=  mSetupONMAPDRAW->getLinetype();

  vcopt->vcSelectedOnMapColour=    mSetupHITMAPDRAW->getColour().name;
  vcopt->vcSelectedOnMapLinewidth= mSetupHITMAPDRAW->getLinewidth();
  vcopt->vcSelectedOnMapLinetype=  mSetupHITMAPDRAW->getLinetype();
#endif

  vcrossm->updateOptions();
}


void VcrossSetupDialog::helpClicked()
{
  Q_EMIT showsource("ug_verticalcrosssections.html");
}


void VcrossSetupDialog::applyClicked()
{
  METLIBS_LOG_SCOPE();
  applySetup();
  Q_EMIT SetupApply();
}


void VcrossSetupDialog::applyhideClicked()
{
  applyClicked();
  hide();
}
