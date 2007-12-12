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

#include <qapplication.h>
#include <qdialog.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qvbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qvbox.h>
#include <qtUtility.h>
#include <diVcrossManager.h>
#include <qtVcrossSetup.h>
#include <qtVcrossSetupDialog.h>
#include <diVcrossOptions.h>


VcrossSetupDialog::VcrossSetupDialog( QWidget* parent, VcrossManager* vm )
  : QDialog(parent), vcrossm(vm)
{
#ifdef DEBUGPRINT
  cout<<"VcrossSetupDialog::VcrossSetupDialog called"<<endl;
#endif

  //caption to appear on top of dialog
  setCaption( tr("Diana Vertical Crossections - settings"));

  // text constants
  TEXTPLOT       =  tr("Text").latin1();
  FRAME          =  tr("Frame").latin1();
  POSNAMES       =  tr("Position names").latin1();
  LEVELNUMBERS   =  tr("Number for level").latin1();
  UPPERLEVEL     =  tr("Top level").latin1();
  LOWERLEVEL     =  tr("Bottom level").latin1();
  OTHERLEVELS    =  tr("Other levels").latin1();
  SURFACE        =  tr("Ground pressure").latin1();
  DISTANCE       =  tr("Distance").latin1();
  GRIDPOS        =  tr("Grid x,y positions").latin1();
  GEOPOS         =  tr("Geographical positions").latin1();
  VERTGRID       =  tr("Vertical gridlines").latin1();
  MARKERLINES    =  tr("Marker lines").latin1();
  VERTICALMARKER =  tr("Vertical markers").latin1();
  EXTRAPOLP      =  tr("Extrapolate to fixed P").latin1();
  BOTTOMEXT      =  tr("Extrapolate to ocean floor").latin1();
  THINARROWS     =  tr("Thin arrows").latin1();
  VERTICALTYPE   =  tr("Vertical type").latin1();
  VHSCALE        =  tr("Fixed vertical/horizontal scaling:").latin1();
  STDVERAREA     =  tr("Default area vertically:").latin1();
  STDHORAREA     =  tr("Default area horizontally:").latin1();
  BACKCOLOUR     =  tr("Background colour").latin1();
  ONMAPDRAW      =  tr("Crossections on map").latin1();
  HITMAPDRAW     =  tr("Selected crossection on map").latin1();

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
  QHBoxLayout* hlayout1 = new QHBoxLayout( 5 );
  hlayout1->addWidget( setuphelp );
  hlayout1->addWidget( standard );

  //place buttons "utfr", "help" etc. in horizontal layout
  QHBoxLayout* hlayout2 = new QHBoxLayout( 5 );
  hlayout2->addWidget( setuphide );
  hlayout2->addWidget( setupapplyhide );
  hlayout2->addWidget( setupapply );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this, 10, 10 );
  vlayout->addLayout( glayout );
  vlayout->addLayout( hlayout1 );
  vlayout->addLayout( hlayout2 );

  isInitialized=false;

#ifdef DEBUGPRINT
  cout<<"VcrossSetupDialog::VcrossSetupDialog finished"<<endl;
#endif
}


void VcrossSetupDialog::initOptions(QWidget* parent)
{
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::initOptions" << endl;
#endif

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes/spinboxes
  int numrows= 29;
  glayout = new QGridLayout(numrows,4);
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

  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,TEXTPLOT,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,POSNAMES,glayout,nrow++,opts,true));
  
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour |
	 VcrossSetup::useLineWidth | VcrossSetup::useLineType);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,FRAME,glayout,nrow++,opts,true));

  opts= VcrossSetup::useOnOff;
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,LEVELNUMBERS,glayout,nrow++,opts,true));

  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour |
	 VcrossSetup::useLineWidth | VcrossSetup::useLineType);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,UPPERLEVEL,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,LOWERLEVEL,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,OTHERLEVELS,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,SURFACE,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,VERTGRID,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,MARKERLINES,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,VERTICALMARKER,glayout,nrow++,opts,true));

  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour | 
	 VcrossSetup::useTextChoice | VcrossSetup::useTextChoice2);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,DISTANCE,glayout,nrow++,opts,true));
  vector<miString> distunit;
  distunit.push_back("km");
  distunit.push_back("nm");
  n= vcSetups.size()-1;
  vcSetups[n]->defineTextChoice(distunit,0);
  vector<miString> diststep;
  diststep.push_back("grid");
  diststep.push_back("1");
  diststep.push_back("10");
  diststep.push_back("100");
  vcSetups[n]->defineTextChoice2(diststep,0);
  
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,GRIDPOS,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,GEOPOS,glayout,nrow++,opts,true));
  opts= VcrossSetup::useOnOff;
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,EXTRAPOLP,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,BOTTOMEXT,glayout,nrow++,opts,true));

  vcSetups.push_back(new VcrossSetup(parent,vcrossm,THINARROWS,glayout,nrow++,opts,true));

  nrow++;
  opts= VcrossSetup::useTextChoice;
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,VERTICALTYPE,glayout,nrow++,opts,true));
  vector<miString> vchoice;
  vchoice.push_back("Standard/P");
  vchoice.push_back("Standard/FL");
  vchoice.push_back("Pressure/P");
  vchoice.push_back("Pressure/FL");
  vchoice.push_back("Height/m");
  vchoice.push_back("Height/Ft");
  n= vcSetups.size()-1;
  vcSetups[n]->defineTextChoice(vchoice,0);

  nrow++;
  opts= (VcrossSetup::useOnOff | VcrossSetup::useValue);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,VHSCALE,glayout,nrow++,opts,true));
  n= vcSetups.size()-1;
//vcSetups[n]->defineValue(10,600,10,150,"","x");
  vcSetups[n]->defineValue(1,600,1,150,"","x");

  opts= (VcrossSetup::useOnOff | VcrossSetup::useMinValue | VcrossSetup::useMaxValue);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,STDVERAREA,glayout,nrow++,opts,true));
  n= vcSetups.size()-1;
  vcSetups[n]->defineMinValue(0,100,5,  0,"","%");
  vcSetups[n]->defineMaxValue(0,100,5,100,"","%");

  vcSetups.push_back(new VcrossSetup(parent,vcrossm,STDHORAREA,glayout,nrow++,opts,true));
  n= vcSetups.size()-1;
  vcSetups[n]->defineMinValue(0,100,5,  0,"","%");
  vcSetups[n]->defineMaxValue(0,100,5,100,"","%");

  nrow++;
  opts= VcrossSetup::useColour;
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,BACKCOLOUR,glayout,nrow++,opts,true));

  nrow++;
  opts= (VcrossSetup::useColour |
	 VcrossSetup::useLineWidth | VcrossSetup::useLineType);
  vcSetups.push_back(new VcrossSetup(parent,vcrossm, ONMAPDRAW,glayout,nrow++,opts,true));
  vcSetups.push_back(new VcrossSetup(parent,vcrossm,HITMAPDRAW,glayout,nrow++,opts,true));

  if (nrow!=numrows) {
    cerr<<"=================================================="<<endl;
    cerr<<"===== VcrossSetupDialog: glayout numrows= "<<numrows<<endl;
    cerr<<"=====                               nrow= "<<nrow<<endl;
    cerr<<"=================================================="<<endl;
  }
}


void VcrossSetupDialog::standardClicked()
{
  //this slot is called when standard button pressed
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::standardClicked()" << endl;
#endif
  VcrossOptions * vcopt= new VcrossOptions; // diana defaults
  setup(vcopt);
  delete vcopt;
  //emit SetupApply();
}


void VcrossSetupDialog::start()
{
  if (!isInitialized){
    // pointer to logged options (the first time)
    VcrossOptions * vcopt= vcrossm->getOptions();
    setup(vcopt);
    isInitialized=true;
  }
}


void VcrossSetupDialog::setup(VcrossOptions *vcopt)
{
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::setup()" << endl;
#endif

  int n= vcSetups.size();

  for (int i=0; i<n; i++) {

    if (vcSetups[i]->name== TEXTPLOT) {
      vcSetups[i]->setOn    (vcopt->pText);
      vcSetups[i]->setColour(vcopt->textColour);

    } else if (vcSetups[i]->name== POSNAMES) {
      vcSetups[i]->setOn    (vcopt->pPositionNames);
      vcSetups[i]->setColour(vcopt->positionNamesColour);

    } else if (vcSetups[i]->name== FRAME) {
      vcSetups[i]->setOn       (vcopt->pFrame);
      vcSetups[i]->setColour   (vcopt->frameColour);
      vcSetups[i]->setLinewidth(vcopt->frameLinewidth);
      vcSetups[i]->setLinetype (vcopt->frameLinetype);

    } else if (vcSetups[i]->name== LEVELNUMBERS) {
      vcSetups[i]->setOn(vcopt->pLevelNumbers);

    } else if (vcSetups[i]->name== UPPERLEVEL) {
      vcSetups[i]->setOn       (vcopt->pUpperLevel);
      vcSetups[i]->setColour   (vcopt->upperLevelColour);
      vcSetups[i]->setLinewidth(vcopt->upperLevelLinewidth);
      vcSetups[i]->setLinetype (vcopt->upperLevelLinetype);

    } else if (vcSetups[i]->name== LOWERLEVEL) {
      vcSetups[i]->setOn       (vcopt->pLowerLevel);
      vcSetups[i]->setColour   (vcopt->lowerLevelColour);
      vcSetups[i]->setLinewidth(vcopt->lowerLevelLinewidth);
      vcSetups[i]->setLinetype (vcopt->lowerLevelLinetype);

    } else if (vcSetups[i]->name== OTHERLEVELS) {
      vcSetups[i]->setOn       (vcopt->pOtherLevels);
      vcSetups[i]->setColour   (vcopt->otherLevelsColour);
      vcSetups[i]->setLinewidth(vcopt->otherLevelsLinewidth);
      vcSetups[i]->setLinetype (vcopt->otherLevelsLinetype);

    } else if (vcSetups[i]->name== SURFACE) {
      vcSetups[i]->setOn       (vcopt->pSurface);
      vcSetups[i]->setColour   (vcopt->surfaceColour);
      vcSetups[i]->setLinewidth(vcopt->surfaceLinewidth);
      vcSetups[i]->setLinetype (vcopt->surfaceLinetype);

    } else if (vcSetups[i]->name== DISTANCE) {
      vcSetups[i]->setOn         (vcopt->pDistance);
      vcSetups[i]->setColour     (vcopt->distanceColour);
      vcSetups[i]->setTextChoice (vcopt->distanceUnit);
      vcSetups[i]->setTextChoice2(vcopt->distanceStep);

    } else if (vcSetups[i]->name== GRIDPOS) {
      vcSetups[i]->setOn    (vcopt->pXYpos);
      vcSetups[i]->setColour(vcopt->xyposColour);

    } else if (vcSetups[i]->name== GEOPOS) {
      vcSetups[i]->setOn    (vcopt->pGeoPos);
      vcSetups[i]->setColour(vcopt->geoposColour);

    } else if (vcSetups[i]->name== VERTGRID) {
      vcSetups[i]->setOn       (vcopt->pVerticalGridLines);
      vcSetups[i]->setColour   (vcopt->vergridColour);
      vcSetups[i]->setLinewidth(vcopt->vergridLinewidth);
      vcSetups[i]->setLinetype (vcopt->vergridLinetype);

    } else if (vcSetups[i]->name== MARKERLINES) {
      vcSetups[i]->setOn       (vcopt->pMarkerlines);
      vcSetups[i]->setColour   (vcopt->markerlinesColour);
      vcSetups[i]->setLinewidth(vcopt->markerlinesLinewidth);
      vcSetups[i]->setLinetype (vcopt->markerlinesLinetype);

    } else if (vcSetups[i]->name== VERTICALMARKER) {
      vcSetups[i]->setOn       (vcopt->pVerticalMarker);
      vcSetups[i]->setColour   (vcopt->verticalMarkerColour);
      vcSetups[i]->setLinewidth(vcopt->verticalMarkerLinewidth);
      vcSetups[i]->setLinetype (vcopt->verticalMarkerLinetype);

    } else if (vcSetups[i]->name== EXTRAPOLP) {
      vcSetups[i]->setOn(vcopt->extrapolateFixedLevels);

    } else if (vcSetups[i]->name== BOTTOMEXT) {
      vcSetups[i]->setOn(vcopt->extrapolateToBottom);

    } else if (vcSetups[i]->name== THINARROWS) {
      vcSetups[i]->setOn(vcopt->thinArrows);

    } else if (vcSetups[i]->name== VERTICALTYPE) {
      vcSetups[i]->setTextChoice(vcopt->verticalType);

    } else if (vcSetups[i]->name== VHSCALE) {
      vcSetups[i]->setOn   (vcopt->keepVerHorRatio);
      vcSetups[i]->setValue(vcopt->verHorRatio);

    } else if (vcSetups[i]->name== STDVERAREA) {
      vcSetups[i]->setOn      (vcopt->stdVerticalArea);
      vcSetups[i]->setMinValue(vcopt->minVerticalArea);
      vcSetups[i]->setMaxValue(vcopt->maxVerticalArea);

    } else if (vcSetups[i]->name== STDHORAREA) {
      vcSetups[i]->setOn      (vcopt->stdHorizontalArea);
      vcSetups[i]->setMinValue(vcopt->minHorizontalArea);
      vcSetups[i]->setMaxValue(vcopt->maxHorizontalArea);

    } else if (vcSetups[i]->name== BACKCOLOUR) {
      vcSetups[i]->setColour(vcopt->backgroundColour);

    } else if (vcSetups[i]->name== ONMAPDRAW) {
      vcSetups[i]->setColour   (vcopt->vcOnMapColour);
      vcSetups[i]->setLinewidth(vcopt->vcOnMapLinewidth);
      vcSetups[i]->setLinetype (vcopt->vcOnMapLinetype);

    } else if (vcSetups[i]->name== HITMAPDRAW) {
      vcSetups[i]->setColour   (vcopt->vcSelectedOnMapColour);
      vcSetups[i]->setLinewidth(vcopt->vcSelectedOnMapLinewidth);
      vcSetups[i]->setLinetype (vcopt->vcSelectedOnMapLinetype);

    } else {
      cerr<<"VcrossSetupDialog::setup ERROR : "
	  <<vcSetups[i]->name<<endl;
    }

  }
}


void VcrossSetupDialog::applySetup()
{
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::applySetup()" << endl;
#endif
  VcrossOptions * vcopt= vcrossm->getOptions();

  int n= vcSetups.size();

  for (int i=0; i<n; i++) {

    if (vcSetups[i]->name== TEXTPLOT) {
      vcopt->pText=      vcSetups[i]->isOn();
      vcopt->textColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== POSNAMES) {
      vcopt->pPositionNames=      vcSetups[i]->isOn();
      vcopt->positionNamesColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== FRAME) {
      vcopt->pFrame=         vcSetups[i]->isOn();
      vcopt->frameColour=    vcSetups[i]->getColour().name;
      vcopt->frameLinewidth= vcSetups[i]->getLinewidth();
      vcopt->frameLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== LEVELNUMBERS) {
      vcopt->pLevelNumbers= vcSetups[i]->isOn();

    } else if (vcSetups[i]->name== UPPERLEVEL) {
      vcopt->pUpperLevel=         vcSetups[i]->isOn();
      vcopt->upperLevelColour=    vcSetups[i]->getColour().name;
      vcopt->upperLevelLinewidth= vcSetups[i]->getLinewidth();
      vcopt->upperLevelLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== LOWERLEVEL) {
      vcopt->pLowerLevel=         vcSetups[i]->isOn();
      vcopt->lowerLevelColour=    vcSetups[i]->getColour().name;
      vcopt->lowerLevelLinewidth= vcSetups[i]->getLinewidth();
      vcopt->lowerLevelLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== OTHERLEVELS) {
      vcopt->pOtherLevels=         vcSetups[i]->isOn();
      vcopt->otherLevelsColour=    vcSetups[i]->getColour().name;
      vcopt->otherLevelsLinewidth= vcSetups[i]->getLinewidth();
      vcopt->otherLevelsLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== SURFACE) {
      vcopt->pSurface=         vcSetups[i]->isOn();
      vcopt->surfaceColour=    vcSetups[i]->getColour().name;
      vcopt->surfaceLinewidth= vcSetups[i]->getLinewidth();
      vcopt->surfaceLinetype=  vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== DISTANCE) {
      vcopt->pDistance=      vcSetups[i]->isOn();
      vcopt->distanceColour= vcSetups[i]->getColour().name;
      vcopt->distanceUnit=   vcSetups[i]->getTextChoice();
      vcopt->distanceStep=   vcSetups[i]->getTextChoice2();

    } else if (vcSetups[i]->name== GRIDPOS) {
      vcopt->pXYpos=      vcSetups[i]->isOn();
      vcopt->xyposColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== GEOPOS) {
      vcopt->pGeoPos=      vcSetups[i]->isOn();
      vcopt->geoposColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== VERTGRID) {
      vcopt->pVerticalGridLines= vcSetups[i]->isOn();
      vcopt->vergridColour=      vcSetups[i]->getColour().name;
      vcopt->vergridLinewidth=   vcSetups[i]->getLinewidth();
      vcopt->vergridLinetype=    vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== MARKERLINES) {
      vcopt->pMarkerlines=         vcSetups[i]->isOn();
      vcopt->markerlinesColour=    vcSetups[i]->getColour().name;
      vcopt->markerlinesLinewidth= vcSetups[i]->getLinewidth();
      vcopt->markerlinesLinetype=  vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== VERTICALMARKER) {
      vcopt->pVerticalMarker=         vcSetups[i]->isOn();
      vcopt->verticalMarkerColour=    vcSetups[i]->getColour().name;
      vcopt->verticalMarkerLinewidth= vcSetups[i]->getLinewidth();
      vcopt->verticalMarkerLinetype=  vcSetups[i]->getLinetype ();

    } else if (vcSetups[i]->name== EXTRAPOLP) {
      vcopt->extrapolateFixedLevels= vcSetups[i]->isOn();

    } else if (vcSetups[i]->name== BOTTOMEXT) {
      vcopt->extrapolateToBottom= vcSetups[i]->isOn();

    } else if (vcSetups[i]->name== THINARROWS) {
      vcopt->thinArrows= vcSetups[i]->isOn();

    } else if (vcSetups[i]->name== VERTICALTYPE) {
      vcopt->verticalType= vcSetups[i]->getTextChoice();

    } else if (vcSetups[i]->name== VHSCALE) {
      vcopt->keepVerHorRatio= vcSetups[i]->isOn();
      vcopt->verHorRatio=     vcSetups[i]->getValue();

    } else if (vcSetups[i]->name== STDVERAREA) {
      vcopt->stdVerticalArea= vcSetups[i]->isOn();
      vcopt->minVerticalArea= vcSetups[i]->getMinValue();
      vcopt->maxVerticalArea= vcSetups[i]->getMaxValue();

    } else if (vcSetups[i]->name== STDHORAREA) {
      vcopt->stdHorizontalArea= vcSetups[i]->isOn();
      vcopt->minHorizontalArea= vcSetups[i]->getMinValue();
      vcopt->maxHorizontalArea= vcSetups[i]->getMaxValue();

    } else if (vcSetups[i]->name== BACKCOLOUR) {
      vcopt->backgroundColour= vcSetups[i]->getColour().name;

    } else if (vcSetups[i]->name== ONMAPDRAW) {
      vcopt->vcOnMapColour=    vcSetups[i]->getColour().name;
      vcopt->vcOnMapLinewidth= vcSetups[i]->getLinewidth();
      vcopt->vcOnMapLinetype=  vcSetups[i]->getLinetype();

    } else if (vcSetups[i]->name== HITMAPDRAW) {
      vcopt->vcSelectedOnMapColour=    vcSetups[i]->getColour().name;
      vcopt->vcSelectedOnMapLinewidth= vcSetups[i]->getLinewidth();
      vcopt->vcSelectedOnMapLinetype=  vcSetups[i]->getLinetype();

    } else {
      cerr<<"VcrossSetupDialog::applySetup ERROR : "
	  <<vcSetups[i]->name<<endl;
    }

  }
}


void VcrossSetupDialog::helpClicked()
{
  //this slot is called when help button pressed
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::helpClicked()" << endl;
#endif
  emit showdoc("ug_verticalcrosssections.html");
}


void VcrossSetupDialog::applyClicked()
{
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::applyClicked()" << endl;
#endif
  applySetup();
  emit SetupApply();
}


void VcrossSetupDialog::applyhideClicked()
{
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"VcrossSetupDialog::applyhideClicked()" << endl;
#endif
  applySetup();
  emit SetupHide();
  emit SetupApply();
}


bool VcrossSetupDialog::close(bool alsoDelete)
{
  emit SetupHide();
  return true;
}
