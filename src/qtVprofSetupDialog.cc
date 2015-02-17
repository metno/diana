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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include "qtUtility.h"
#include "diVprofManager.h"
#include "vcross_qt/qtVcrossSetup.h"
#include "qtVprofSetupDialog.h"
#include "diVprofOptions.h"

#define MILOGGER_CATEGORY "diana.VprofSetupDialog"
#include <miLogger/miLogging.h>

namespace {
const int pStep= 50;
const int tStep= 5;
}

/***************************************************************************/
VprofSetupDialog::VprofSetupDialog( QWidget* parent,VprofManager * vm )
  : QDialog(parent),vprofm(vm)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Diana Vertical Profiles - settings"));

  //********** create the various QT widgets to appear in dialog ***********

  twd = new QTabWidget( this );
  initDatatab();
  initDiagramtab();
  initColourtab();

  // standard buttons *******************************************

  //push button to show help
  QPushButton * setuphelp = NormalPushButton( tr("Help"), this );
  connect(  setuphelp, SIGNAL(clicked()), SLOT( helpClicked()));

  //push button to set to default
  QPushButton * standard = NormalPushButton( tr("Default"), this );
  connect(  standard, SIGNAL(clicked()), SLOT( standardClicked()));

  //push button to hide dialog
  QPushButton * setuphide = NormalPushButton( tr("Hide"), this );
  connect( setuphide, SIGNAL(clicked()), SIGNAL(SetupHide()));

   //push button to apply the selected command and then hide dialog
  QPushButton  * setupapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( setupapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  //push button to apply the selected command
  QPushButton * setupapply = NormalPushButton( tr("Apply"), this );
  connect(setupapply, SIGNAL(clicked()), SLOT( applyClicked()) );

  // *********** place all the widgets in layouts ****************

  //place buttons "oppdater", "hjelp" etc. in horizontal layout
  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget( setuphelp );
  hlayout1->addWidget( standard );

  //place buttons "utfør", "help" etc. in horizontal layout
  QHBoxLayout* hlayout2 = new QHBoxLayout();
  hlayout2->addWidget( setuphide );
  hlayout2->addWidget( setupapplyhide );
  hlayout2->addWidget( setupapply );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this);
  vlayout->addWidget( twd );
  vlayout->addLayout( hlayout1 );
  vlayout->addLayout( hlayout2 );

  isInitialized=false;
}


/*********************************************/

void VprofSetupDialog::setPressureMin(int value){
  pressureMin = value;
  pressureSpinHigh->setMinimum(pressureMin+pStep);
}


void VprofSetupDialog::setPressureMax(int value){
  pressureMax = value;
  pressureSpinLow->setMaximum(pressureMax-pStep);
}


void VprofSetupDialog::setTemperatureMin(int value){
  temperatureMin = value;
  temperatureSpinHigh->setMinimum(temperatureMin+tStep);
}


void VprofSetupDialog::setTemperatureMax(int value){
  temperatureMax = value;
  temperatureSpinLow->setMaximum(temperatureMax-tStep);
}

/*********************************************/

void VprofSetupDialog::standardClicked(){
  //this slot is called when standard button pressed
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofSetupDialog::standardClicked()");
#endif
  VprofOptions * vpopt= new VprofOptions; // diana defaults
  setup(vpopt);
  delete vpopt;
  //emit SetupApply();
}

/*********************************************/

void VprofSetupDialog::start(){
  //printSetup();
  if (!isInitialized){
    // pointer to logged options (the first time)
    VprofOptions * vpopt= vprofm->getOptions();
    setup(vpopt);
    isInitialized=true;
  }
  //printSetup();

}

/*********************************************/

void VprofSetupDialog::setup(VprofOptions *vpopt)
{
  METLIBS_LOG_SCOPE();

  mSetupTEMP->setChecked(vpopt->ptttt);
  mSetupDEWPOINT->setChecked(vpopt->ptdtd);
  mSetupWIND->setChecked(vpopt->pwind);
  mSetupVERTWIND->setChecked(vpopt->pvwind);
  mSetupRELHUM->setChecked(vpopt->prelhum);
  mSetupDUCTING->setChecked(vpopt->pducting);
  mSetupKINDEX->setChecked(vpopt->pkindex);
  mSetupSIGNWIND->setChecked(vpopt->pslwind);
  mSetupTEXT->setChecked(vpopt->ptext);
  mSetupGEOPOS->setChecked(vpopt->pgeotext);

  mSetupPRESSLINES->setChecked(vpopt->pplines);
  mSetupPRESSLINES->setColour(vpopt->pColour);
  mSetupPRESSLINES->setLinewidth(vpopt->pLinewidth1);
  mSetupPRESSLINES->setLinetype(vpopt->pLinetype);
  
  mSetupLINEFLIGHT->setChecked(vpopt->pplinesfl);

  mSetupTEMPLINES->setChecked(vpopt->ptlines);
  mSetupTEMPLINES->setColour(vpopt->tColour);
  mSetupTEMPLINES->setLinewidth(vpopt->tLinewidth1);
  mSetupTEMPLINES->setLinetype(vpopt->tLinetype);

  mSetupDRYADIABATS->setChecked(vpopt->pdryadiabat);
  mSetupDRYADIABATS->setColour(vpopt->dryadiabatColour);
  mSetupDRYADIABATS->setLinewidth(vpopt->dryadiabatLinewidth);
  mSetupDRYADIABATS->setLinetype(vpopt->dryadiabatLinetype);

  mSetupWETADIABATS->setChecked(vpopt->pwetadiabat);
  mSetupWETADIABATS->setColour(vpopt->wetadiabatColour);
  mSetupWETADIABATS->setLinewidth(vpopt->wetadiabatLinewidth);
  mSetupWETADIABATS->setLinetype(vpopt->wetadiabatLinetype);

  mSetupMIXINGRATIO->setChecked(vpopt->pmixingratio);
  mSetupMIXINGRATIO->setColour(vpopt->mixingratioColour);
  mSetupMIXINGRATIO->setLinewidth(vpopt->mixingratioLinewidth);
  mSetupMIXINGRATIO->setLinetype(vpopt->mixingratioLinetype);

  mSetupPTLABELS->setChecked(vpopt->plabelp);

  mSetupFRAME->setChecked(vpopt->pframe);
  mSetupFRAME->setColour(vpopt->frameColour);
  mSetupFRAME->setLinewidth(vpopt->frameLinewidth);
  mSetupFRAME->setLinetype(vpopt->frameLinetype);

  mSetupFLIGHTLEVEL->setChecked(vpopt->pflevels);
  mSetupFLIGHTLEVEL->setColour(vpopt->flevelsColour);
  mSetupFLIGHTLEVEL->setLinewidth(vpopt->flevelsLinewidth1);
  mSetupFLIGHTLEVEL->setLinetype(vpopt->flevelsLinetype);

  mSetupFLIGHTLABEL->setChecked(vpopt->plabelflevels);

  mSetupCONDTRAIL->setChecked(vpopt->pcotrails);
  mSetupCONDTRAIL->setColour(vpopt->cotrailsColour);
  mSetupCONDTRAIL->setLinewidth(vpopt->cotrailsLinewidth);
  mSetupCONDTRAIL->setLinetype(vpopt->cotrailsLinetype);

  mSetupSEPWIND->setChecked(vpopt->windseparate);

  mSetupBACKCOLOUR->setColour(vpopt->backgroundColour);

  for (size_t j=0; j<mSetupData.size(); ++j) {
    mSetupData[j]->setColour(vpopt->dataColour[j]);
    mSetupData[j]->setLinewidth(vpopt->dataLinewidth[j]);
  }

  int pmin=  (vpopt->pminDiagram/pStep) * pStep;
  int pmax= ((vpopt->pmaxDiagram + pStep-1)/pStep) * pStep;
  int tmin=  (vpopt->tminDiagram/tStep) * tStep;
  int tmax= ((vpopt->tmaxDiagram + tStep-1)/tStep) * tStep;

  //min/max pressure
  setPressureMin(pmin);
  setPressureMax(pmax);
  pressureSpinLow->setValue(pmin);
  pressureSpinHigh->setValue(pmax);

  //min/max temperature
  setTemperatureMin(tmin);
  setTemperatureMax(tmax);
  temperatureSpinLow->setValue(tmin);
  temperatureSpinHigh->setValue(tmax);

}

/*********************************************/

void VprofSetupDialog::applySetup()
{
  METLIBS_LOG_SCOPE();
  VprofOptions * vpopt= vprofm->getOptions();

  vpopt->ptttt=mSetupTEMP->isChecked();
  vpopt->ptdtd=mSetupDEWPOINT->isChecked();
  vpopt->pwind=mSetupWIND->isChecked();
  vpopt->pvwind=mSetupVERTWIND->isChecked();
  vpopt->prelhum=mSetupRELHUM->isChecked();
  vpopt->pducting=mSetupDUCTING->isChecked();
  vpopt->pkindex=mSetupKINDEX->isChecked();
  vpopt->pslwind=mSetupSIGNWIND->isChecked();
  vpopt->ptext=mSetupTEXT->isChecked();
  vpopt->pgeotext=mSetupGEOPOS->isChecked();

  vpopt->pplines=mSetupPRESSLINES->isChecked();
  vpopt->pColour=mSetupPRESSLINES->getColour().name;
  vpopt->pLinewidth1=mSetupPRESSLINES->getLinewidth();
  vpopt->pLinetype=mSetupPRESSLINES->getLinetype();

  vpopt->pplinesfl=mSetupLINEFLIGHT->isChecked();

  vpopt->ptlines=mSetupTEMPLINES->isChecked();
  vpopt->tColour=mSetupTEMPLINES->getColour().name;
  vpopt->tLinewidth1=mSetupTEMPLINES->getLinewidth();
  vpopt->tLinetype=mSetupTEMPLINES->getLinetype();
  
  vpopt->pdryadiabat=mSetupDRYADIABATS->isChecked();
  vpopt->dryadiabatColour=mSetupDRYADIABATS->getColour().name;
  vpopt->dryadiabatLinewidth=mSetupDRYADIABATS->getLinewidth();
  vpopt->dryadiabatLinetype=mSetupDRYADIABATS->getLinetype();

  vpopt->pwetadiabat=mSetupWETADIABATS->isChecked();
  vpopt->wetadiabatColour=mSetupWETADIABATS->getColour().name;
  vpopt->wetadiabatLinewidth=mSetupWETADIABATS->getLinewidth();
  vpopt->wetadiabatLinetype=mSetupWETADIABATS->getLinetype();

  vpopt->pmixingratio=mSetupMIXINGRATIO->isChecked();
  vpopt->mixingratioColour=mSetupMIXINGRATIO->getColour().name;
  vpopt->mixingratioLinewidth=mSetupMIXINGRATIO->getLinewidth();
  vpopt->mixingratioLinetype=mSetupMIXINGRATIO->getLinetype();

  vpopt->plabelp=mSetupPTLABELS->isChecked();
  vpopt->plabelt=mSetupPTLABELS->isChecked();

  vpopt->pframe=mSetupFRAME->isChecked();
  vpopt->frameColour=mSetupFRAME->getColour().name;
  vpopt->frameLinewidth=mSetupFRAME->getLinewidth();
  vpopt->frameLinetype=mSetupFRAME->getLinetype();
  
  vpopt->pflevels=mSetupFLIGHTLEVEL->isChecked();
  vpopt->flevelsColour=mSetupFLIGHTLEVEL->getColour().name;
  vpopt->flevelsLinewidth1=mSetupFLIGHTLEVEL->getLinewidth();
  vpopt->flevelsLinetype=mSetupFLIGHTLEVEL->getLinetype();

  vpopt->plabelflevels=mSetupFLIGHTLABEL->isChecked();

  vpopt->pcotrails=mSetupCONDTRAIL->isChecked();
  vpopt->cotrailsColour=mSetupCONDTRAIL->getColour().name;
  vpopt->cotrailsLinewidth=mSetupCONDTRAIL->getLinewidth();
  vpopt->cotrailsLinetype=mSetupCONDTRAIL->getLinetype();

  vpopt->windseparate=mSetupSEPWIND->isChecked();

  vpopt->backgroundColour=mSetupBACKCOLOUR->getColour().name;

  for (size_t j=0; j<mSetupData.size(); ++j) {
    vpopt->dataColour[j]=    mSetupData[j]->getColour().name;
    vpopt->dataLinewidth[j]= mSetupData[j]->getLinewidth();
  }

  //min/max pressure
  vpopt->pminDiagram= (pressureMin>10) ? pressureMin : 10;
  vpopt->pmaxDiagram= pressureMax;

  //min/max temperature
  vpopt->tminDiagram= temperatureMin;
  vpopt->tmaxDiagram= temperatureMax;

  vpopt->changed= true;
}

/*********************************************/

void VprofSetupDialog::helpClicked()
{
  Q_EMIT showsource("ug_verticalprofiles.html");
}

/*********************************************/

void VprofSetupDialog::applyClicked()
{
  applySetup();
  Q_EMIT SetupApply();
}

/*********************************************/

void VprofSetupDialog::applyhideClicked()
{
  applySetup();
  Q_EMIT SetupHide();
  Q_EMIT SetupApply();
}

/*********************************************/

void VprofSetupDialog::initDatatab()
{
  datatab = new QWidget(twd);
  twd->addTab( datatab, tr("Diagram") );

  int mymargin=5;
  int myspacing=5;

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(datatab);
  glayout->setMargin( mymargin );
  glayout->setSpacing( myspacing );

  int nrow=0;

  QLabel * label1 = new QLabel(tr("On/off"),datatab);

  glayout->addWidget(label1,nrow,0);
  nrow++;

  int opts= VcrossSetupUI::useOnOff;

  mSetupTEMP = new VcrossSetupUI(datatab, tr("Temperature"), glayout, nrow++, opts);
  mSetupDEWPOINT = new VcrossSetupUI(datatab, tr("Dewpoint"), glayout, nrow++, opts);
  mSetupWIND = new VcrossSetupUI(datatab, tr("Wind"), glayout, nrow++, opts);
  mSetupVERTWIND = new VcrossSetupUI(datatab, tr("Vertical wind (model)"), glayout, nrow++, opts);
  mSetupRELHUM = new VcrossSetupUI(datatab, tr("Relative humidity"), glayout, nrow++, opts);
  mSetupDUCTING = new VcrossSetupUI(datatab, tr("Refraction index"), glayout, nrow++, opts);
  mSetupSIGNWIND = new VcrossSetupUI(datatab, tr("Significant wind (dd-ff)"), glayout, nrow++, opts);
  mSetupTEXT = new VcrossSetupUI(datatab, tr("Text"), glayout, nrow++, opts);
  mSetupKINDEX = new VcrossSetupUI(datatab, tr("K-index"), glayout, nrow++, opts);
  mSetupGEOPOS = new VcrossSetupUI(datatab, tr("Geographical position in text"), glayout, nrow++, opts);


  //spinbox for pressure and temperature range
  //value of spinLow must not exceed value of spinHigh

  QLabel * pressurelabel = new QLabel(tr("Pressure range"),datatab);
  QLabel * templabel = new QLabel(tr("Temperature range"),datatab);
  pressurelabel->setAlignment(Qt::AlignLeft);
  templabel->setAlignment(Qt::AlignLeft);
  //pressure range 10-1200, steps of 50, init value 100-1050
  pressureSpinLow  = new QSpinBox(datatab);
  pressureSpinLow->setMinimum(0);
  pressureSpinLow->setMaximum(1200);
  pressureSpinLow->setSingleStep(50);
  pressureSpinHigh = new QSpinBox(datatab);
  pressureSpinHigh->setMinimum(0);
  pressureSpinHigh->setMaximum(1200);
  pressureSpinHigh->setSingleStep(50);
  //temperature range -70-70, steps of 5, init value -30-30
  temperatureSpinLow = new QSpinBox(datatab);
  temperatureSpinLow->setMinimum(-70);
  temperatureSpinLow->setMaximum(70);
  temperatureSpinLow->setSingleStep(5);
  temperatureSpinHigh = new QSpinBox(datatab);
  temperatureSpinLow->setMinimum(-70);
  temperatureSpinLow->setMaximum(70);
  temperatureSpinLow->setSingleStep(5);
  //standardvalues
  pressureSpinLow->setValue(100);
  pressureSpinHigh->setValue(1050);
  temperatureSpinLow->setValue(-30);
  temperatureSpinHigh->setValue(+30);
  //units
  pressureSpinLow->setSuffix(" hPa");
  pressureSpinHigh->setSuffix(" hPa");
  temperatureSpinLow->setSuffix("\xB0""C");
  temperatureSpinHigh->setSuffix("\xB0""C");

  connect(  pressureSpinLow, SIGNAL(valueChanged(int)),
	    SLOT( setPressureMin(int)));
  connect(  pressureSpinHigh, SIGNAL(valueChanged(int)),
	    SLOT( setPressureMax(int)));
  connect(  temperatureSpinLow, SIGNAL(valueChanged(int)),
	    SLOT( setTemperatureMin(int)));
  connect(  temperatureSpinHigh, SIGNAL(valueChanged(int)),
	    SLOT( setTemperatureMax(int)));

  setPressureMin(pressureSpinLow->value());
  setPressureMax(pressureSpinHigh->value());
  setTemperatureMin(temperatureSpinLow->value());
  setTemperatureMax(temperatureSpinHigh->value());

  nrow++;

  glayout->addWidget(pressurelabel,nrow,0);
  glayout->addWidget(pressureSpinLow,nrow,1);
  glayout->addWidget(pressureSpinHigh,nrow,2);
  nrow++;

  glayout->addWidget(templabel,nrow,0);
  glayout->addWidget(temperatureSpinLow,nrow,1);
  glayout->addWidget(temperatureSpinHigh,nrow,2);
  nrow++;
}


void VprofSetupDialog::initDiagramtab()
{
  diagramtab = new QWidget(twd);
  twd->addTab( diagramtab, tr("Diagram") );

  int mymargin=5;
  int myspacing=5;

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(diagramtab);
  glayout->setMargin( mymargin );
  glayout->setSpacing( myspacing );

  QLabel * label1 = new QLabel(tr("On/off"),diagramtab);
  QLabel * label2 = new QLabel(tr("Colour"),diagramtab);
  QLabel * label3 = new QLabel(tr("Line thickness"),diagramtab);
  QLabel * label4 = new QLabel(tr("Line type"),diagramtab);
  glayout->addWidget(label1,0,0);
  glayout->addWidget(label2,0,1);
  glayout->addWidget(label3,0,2);
  glayout->addWidget(label4,0,3);


  //here each of setup lines are defined - each VcrossSetupUI is a line with
  //a checkbox and up to three comboboxes

  int nrow = 0;
  int opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupPRESSLINES = new VcrossSetupUI(diagramtab, tr("Pressure lines"), glayout,++nrow,opts);
  mSetupTEMPLINES = new VcrossSetupUI(diagramtab,tr("Temperature lines"),glayout,++nrow,opts);

  opts= VcrossSetupUI::useOnOff;
  mSetupLINEFLIGHT = new VcrossSetupUI(diagramtab,tr("Lines in flight levels"),glayout,++nrow,opts);

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupDRYADIABATS = new VcrossSetupUI(diagramtab, tr("Dry adiabatic"),glayout,++nrow,opts);
  mSetupWETADIABATS = new VcrossSetupUI(diagramtab,tr("Wet adiabatic"),glayout,++nrow,opts);
  mSetupMIXINGRATIO = new VcrossSetupUI(diagramtab,tr("Mixing rate"),glayout,++nrow,opts);

  opts= VcrossSetupUI::useOnOff;
  mSetupPTLABELS = new VcrossSetupUI(diagramtab,tr("P and T units"),glayout,++nrow,opts);

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupFRAME = new VcrossSetupUI(diagramtab, tr("Frame"),glayout,++nrow,opts);
  mSetupFLIGHTLEVEL = new VcrossSetupUI(diagramtab, tr("Flight levels"),glayout,++nrow,opts);

  opts= VcrossSetupUI::useOnOff;
  mSetupFLIGHTLABEL = new VcrossSetupUI(diagramtab,tr("FL units"),glayout,++nrow,opts);
  mSetupSEPWIND = new VcrossSetupUI(diagramtab,tr("Separate wind columns"),glayout,++nrow,opts);

  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth | VcrossSetupUI::useLineType);
  mSetupCONDTRAIL = new VcrossSetupUI(diagramtab,tr("Lines for condensation trails"),glayout,++nrow,opts);

  opts= (VcrossSetupUI::useColour);
  mSetupBACKCOLOUR = new VcrossSetupUI(diagramtab,tr("Background colour"),glayout,++nrow,opts);
}


void VprofSetupDialog::initColourtab()
{
  colourtab = new QWidget(twd);
  twd->addTab( colourtab, tr("Colours") );

  int mymargin=5;
  int myspacing=5;

  // make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(colourtab);
  glayout->setMargin( mymargin );
  glayout->setSpacing( myspacing );

  QLabel * label1 = new QLabel(" ",colourtab);
  QLabel * label2 = new QLabel(tr("Colour"),colourtab);
  QLabel * label3 = new QLabel(tr("Line thickness"),colourtab);
  glayout->addWidget(label1,0,0);
  glayout->addWidget(label2,0,1);
  glayout->addWidget(label3,0,2);

  int nrow = 0;
  int opts= (VcrossSetupUI::useColour | VcrossSetupUI::useLineWidth);

  for (int j=0; j<8; ++j)
    mSetupData.push_back(new VcrossSetupUI(colourtab,tr("Data%1").arg(j+1), glayout, ++nrow, opts));
}

void VprofSetupDialog::closeEvent(QCloseEvent* e)
{
  Q_EMIT SetupHide();
}
