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
#include <qtabwidget.h>
#include <qtUtility.h>
#include <diVprofManager.h>
#include <qtVprofSetup.h>
#include <qtVprofSetupDialog.h>
#include <diVprofOptions.h>


const int pStep= 50;
const int tStep= 5;

/***************************************************************************/
VprofSetupDialog::VprofSetupDialog( QWidget* parent,VprofManager * vm )
  : QDialog(parent),vprofm(vm)
{
#ifdef DEBUGPRINT 
  cout<<"VprofSetUpDialog::VprofSetUpDialog called"<<endl;
#endif

  //caption to appear on top of dialog
  setCaption(tr("Diana Vertical Profiles - settings"));
   

  //********** String constants

  TEMP          =  tr("Temperature").latin1();
  DEWPOINT      =  tr("Dewpoint").latin1();
  WIND          =  tr("Wind").latin1();
  VERTWIND      =  tr("Vertical wind (model)").latin1();
  RELHUM        =  tr("Relative humidity").latin1();
  DUCTING       =  tr("Refraction index").latin1();
  KINDEX        =  tr("K-index").latin1();
  SIGNWIND      =  tr("Significant wind (dd-ff)").latin1();
  PRESSLINES    =  tr("Pressure lines").latin1();
  LINEFLIGHT    =  tr("Lines in flight levels").latin1();
  TEMPLINES     =  tr("Temperature lines").latin1();
  DRYADIABATS   =  tr("Dry adiabatic").latin1();
  WETADIABATS   =  tr("Wet adiabatic").latin1();
  MIXINGRATIO   =  tr("Mixing rate").latin1();
  PTLABELS      =  tr("P and T units").latin1();
  FRAME         =  tr("Frame").latin1();
  TEXT          =  tr("Text").latin1();
  FLIGHTLEVEL   =  tr("Flight levels").latin1();
  FLIGHTLABEL   =  tr("FL units").latin1();
  SEPWIND       =  tr("Separate wind columns").latin1();
  CONDTRAIL     =  tr("Lines for condensation trails").latin1();
  GEOPOS        =  tr("Geographical position in text").latin1();
  PRESSRANGE    =  tr("Pressure range").latin1();
  TEMPRANGE     =  tr("Temperature range").latin1();
  BACKCOLOUR    =  tr("Background colour").latin1();

  //********** create the various QT widgets to appear in dialog ***********

  twd = new QTabWidget( this );

  vpSetups.clear();
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
  QHBoxLayout* hlayout1 = new QHBoxLayout( 5 );
  hlayout1->addWidget( setuphelp );
  hlayout1->addWidget( standard );

  //place buttons "utfør", "help" etc. in horizontal layout
  QHBoxLayout* hlayout2 = new QHBoxLayout( 5 );
  hlayout2->addWidget( setuphide );
  hlayout2->addWidget( setupapplyhide );
  hlayout2->addWidget( setupapply );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this, 10, 10 );                     
  vlayout->addWidget( twd );
  vlayout->addLayout( hlayout1 ); 
  vlayout->addLayout( hlayout2 );

  isInitialized=false;
}


/*********************************************/

void VprofSetupDialog::setPressureMin(int value){
  pressureMin = value;
  pressureSpinHigh->setMinValue(pressureMin+pStep);
}


void VprofSetupDialog::setPressureMax(int value){
  pressureMax = value;
  pressureSpinLow->setMaxValue(pressureMax-pStep);
}


void VprofSetupDialog::setTemperatureMin(int value){
  temperatureMin = value;
  temperatureSpinHigh->setMinValue(temperatureMin+tStep);
}


void VprofSetupDialog::setTemperatureMax(int value){
  temperatureMax = value;
  temperatureSpinLow->setMaxValue(temperatureMax-tStep);
}

/*********************************************/

void VprofSetupDialog::standardClicked(){
  //this slot is called when standard button pressed
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::standardClicked()" << endl;
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

void VprofSetupDialog::setup(VprofOptions *vpopt){
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::setup()" << endl;
#endif

  int j, n = vpSetups.size();

  for (int i = 0;i<n;i++){    
    if (vpSetups[i]->name== TEMP)
      vpSetups[i]->checkClicked(vpopt->ptttt);
    else if (vpSetups[i]->name== DEWPOINT)
      vpSetups[i]->checkClicked(vpopt->ptdtd);
    else if (vpSetups[i]->name== WIND)
      vpSetups[i]->checkClicked(vpopt->pwind);
    else if (vpSetups[i]->name== VERTWIND)
      vpSetups[i]->checkClicked(vpopt->pvwind);
    else if (vpSetups[i]->name== RELHUM)
      vpSetups[i]->checkClicked(vpopt->prelhum);
    else if (vpSetups[i]->name== DUCTING)
      vpSetups[i]->checkClicked(vpopt->pducting);
    else if (vpSetups[i]->name== KINDEX)
      vpSetups[i]->checkClicked(vpopt->pkindex);
    else if (vpSetups[i]->name== SIGNWIND)
     vpSetups[i]->checkClicked(vpopt->pslwind);
    else if (vpSetups[i]->name== TEXT)
      vpSetups[i]->checkClicked(vpopt->ptext);
    else if (vpSetups[i]->name== GEOPOS)
      vpSetups[i]->checkClicked(vpopt->pgeotext);
    else if (vpSetups[i]->name== PRESSLINES){
      vpSetups[i]->checkClicked(vpopt->pplines);
      vpSetups[i]->setColour(vpopt->pColour);
      vpSetups[i]->setLinewidth(vpopt->pLinewidth1);
      vpSetups[i]->setLinetype(vpopt->pLinetype);
    }
    else if (vpSetups[i]->name== LINEFLIGHT){
      vpSetups[i]->checkClicked(vpopt->pplinesfl);
    }
    else if (vpSetups[i]->name== TEMPLINES){
      vpSetups[i]->checkClicked(vpopt->ptlines);
      vpSetups[i]->setColour(vpopt->tColour);
      vpSetups[i]->setLinewidth(vpopt->tLinewidth1);
      vpSetups[i]->setLinetype(vpopt->tLinetype);
    }
    else if (vpSetups[i]->name== DRYADIABATS){
      vpSetups[i]->checkClicked(vpopt->pdryadiabat);
      vpSetups[i]->setColour(vpopt->dryadiabatColour);
      vpSetups[i]->setLinewidth(vpopt->dryadiabatLinewidth);
      vpSetups[i]->setLinetype(vpopt->dryadiabatLinetype);
    }
    else if (vpSetups[i]->name== WETADIABATS){
      vpSetups[i]->checkClicked(vpopt->pwetadiabat);
      vpSetups[i]->setColour(vpopt->wetadiabatColour);
      vpSetups[i]->setLinewidth(vpopt->wetadiabatLinewidth);
      vpSetups[i]->setLinetype(vpopt->wetadiabatLinetype);
    }
    else if (vpSetups[i]->name== MIXINGRATIO){
      vpSetups[i]->checkClicked(vpopt->pmixingratio);
      vpSetups[i]->setColour(vpopt->mixingratioColour);
      vpSetups[i]->setLinewidth(vpopt->mixingratioLinewidth);
      vpSetups[i]->setLinetype(vpopt->mixingratioLinetype);
    }
    else if (vpSetups[i]->name== PTLABELS){
      vpSetups[i]->checkClicked(vpopt->plabelp);
    }
    else if (vpSetups[i]->name== FRAME){
      vpSetups[i]->checkClicked(vpopt->pframe);
      vpSetups[i]->setColour(vpopt->frameColour);
      vpSetups[i]->setLinewidth(vpopt->frameLinewidth);
      vpSetups[i]->setLinetype(vpopt->frameLinetype);
    }
    else if (vpSetups[i]->name== FLIGHTLEVEL){
      vpSetups[i]->checkClicked(vpopt->pflevels);
      vpSetups[i]->setColour(vpopt->flevelsColour);
      vpSetups[i]->setLinewidth(vpopt->flevelsLinewidth1);
      vpSetups[i]->setLinetype(vpopt->flevelsLinetype);
    }
    else if (vpSetups[i]->name== FLIGHTLABEL){
      vpSetups[i]->checkClicked(vpopt->plabelflevels);
    }
    else if (vpSetups[i]->name== CONDTRAIL){
      vpSetups[i]->checkClicked(vpopt->pcotrails);
      vpSetups[i]->setColour(vpopt->cotrailsColour);
      vpSetups[i]->setLinewidth(vpopt->cotrailsLinewidth);
      vpSetups[i]->setLinetype(vpopt->cotrailsLinetype);
    }    
    else if (vpSetups[i]->name== SEPWIND){
      vpSetups[i]->checkClicked(vpopt->windseparate);
    }
    else if (vpSetups[i]->name== BACKCOLOUR){
      vpSetups[i]->setColour(vpopt->backgroundColour);
    }
    else if (vpSetups[i]->name== "Data1"){
      j= 0;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data2"){
      j= 1;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data3"){
      j= 2;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data4"){
      j= 3;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data5"){
      j= 4;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data6"){
      j= 5;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data7"){
      j= 6;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
    else if (vpSetups[i]->name== "Data8"){
      j= 7;
      vpSetups[i]->setColour(vpopt->dataColour[j]);
      vpSetups[i]->setLinewidth(vpopt->dataLinewidth[j]);
    }
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

void VprofSetupDialog::applySetup(){
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::applySetup()" << endl;
#endif
  VprofOptions * vpopt= vprofm->getOptions();

  int j, n = vpSetups.size();
  for (int i = 0;i<n;i++){
    if (vpSetups[i]->name== TEMP)
      vpopt->ptttt=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== DEWPOINT)
      vpopt->ptdtd=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== WIND)
      vpopt->pwind=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== VERTWIND)
      vpopt->pvwind=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== RELHUM)
      vpopt->prelhum=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== DUCTING)
      vpopt->pducting=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== KINDEX)
      vpopt->pkindex=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== SIGNWIND)
      vpopt->pslwind=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== TEXT)
      vpopt->ptext=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== GEOPOS)
      vpopt->pgeotext=vpSetups[i]->isOn();
    else if (vpSetups[i]->name== PRESSLINES){
      vpopt->pplines=vpSetups[i]->isOn();
      vpopt->pColour=vpSetups[i]->getColour().name;
      vpopt->pLinewidth1=vpSetups[i]->getLinewidth();
      vpopt->pLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== LINEFLIGHT){
      vpopt->pplinesfl=vpSetups[i]->isOn();
    }
    else if (vpSetups[i]->name== TEMPLINES){
      vpopt->ptlines=vpSetups[i]->isOn();
      vpopt->tColour=vpSetups[i]->getColour().name;
      vpopt->tLinewidth1=vpSetups[i]->getLinewidth();
      vpopt->tLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== DRYADIABATS){
      vpopt->pdryadiabat=vpSetups[i]->isOn();
      vpopt->dryadiabatColour=vpSetups[i]->getColour().name;
      vpopt->dryadiabatLinewidth=vpSetups[i]->getLinewidth();
      vpopt->dryadiabatLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== WETADIABATS){
      vpopt->pwetadiabat=vpSetups[i]->isOn();
      vpopt->wetadiabatColour=vpSetups[i]->getColour().name;
      vpopt->wetadiabatLinewidth=vpSetups[i]->getLinewidth();
      vpopt->wetadiabatLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== MIXINGRATIO){
      vpopt->pmixingratio=vpSetups[i]->isOn();
      vpopt->mixingratioColour=vpSetups[i]->getColour().name;
      vpopt->mixingratioLinewidth=vpSetups[i]->getLinewidth();
      vpopt->mixingratioLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== PTLABELS){
      vpopt->plabelp=vpSetups[i]->isOn();
      vpopt->plabelt=vpSetups[i]->isOn();
    }
    else if (vpSetups[i]->name== FRAME){
      vpopt->pframe=vpSetups[i]->isOn();
      vpopt->frameColour=vpSetups[i]->getColour().name;
      vpopt->frameLinewidth=vpSetups[i]->getLinewidth();
      vpopt->frameLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== FLIGHTLEVEL){
      vpopt->pflevels=vpSetups[i]->isOn();
      vpopt->flevelsColour=vpSetups[i]->getColour().name;
      vpopt->flevelsLinewidth1=vpSetups[i]->getLinewidth();
      vpopt->flevelsLinetype=vpSetups[i]->getLinetype();
    }
    else if (vpSetups[i]->name== FLIGHTLABEL){
      vpopt->plabelflevels=vpSetups[i]->isOn();
    }
    else if (vpSetups[i]->name== CONDTRAIL){
      vpopt->pcotrails=vpSetups[i]->isOn();
      vpopt->cotrailsColour=vpSetups[i]->getColour().name;
      vpopt->cotrailsLinewidth=vpSetups[i]->getLinewidth();
      vpopt->cotrailsLinetype=vpSetups[i]->getLinetype();
    }    
    else if (vpSetups[i]->name== SEPWIND){
      vpopt->windseparate=vpSetups[i]->isOn();
    }
    else if (vpSetups[i]->name== BACKCOLOUR){
      vpopt->backgroundColour=vpSetups[i]->getColour().name;
    }
    else if (vpSetups[i]->name== "Data1"){
      j= 0;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data2"){
      j= 1;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data3"){
      j= 2;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data4"){
      j= 3;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data5"){
      j= 4;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data6"){
      j= 5;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data7"){
      j= 6;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }
    else if (vpSetups[i]->name== "Data8"){
      j= 7;
      vpopt->dataColour[j]=    vpSetups[i]->getColour().name;
      vpopt->dataLinewidth[j]= vpSetups[i]->getLinewidth();
    }

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

void VprofSetupDialog::printSetup(){
#ifdef DEBUGPRINT
  //HK, used for debugging...
  cerr <<"VprofSetupDialog::printSetup()" << endl;

  int n = vpSetups.size();
  cerr<< "Number of setups =" << n << endl;
  for (int i = 0;i<n;i++){
    cerr << "Name =" << vpSetups[i]->name;
    if (vpSetups[i]->isOn()){
      cerr << " is on !" << endl;
      ColourInfo sColour = vpSetups[i]->getColour();
      cerr << "    Colour = " << sColour.name << endl;
      cerr << "    linethickness = " << vpSetups[i]->getLinewidth() << endl;
      cerr << "    linetype = " << vpSetups[i]->getLinetype() << endl;
    }else
      cerr << " is off !" << endl;
  }
#endif
}


/*********************************************/

void VprofSetupDialog::helpClicked(){
  //this slot is called when help button pressed
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::helpClicked()" << endl;
#endif
  emit showdoc("ug_verticalprofiles.html");
}

/*********************************************/

void VprofSetupDialog::applyClicked(){
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::applyClicked(int tt)" << endl;
#endif
  applySetup();
  //printSetup();
  emit SetupApply();

}

/*********************************************/

void VprofSetupDialog::applyhideClicked(){
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"VprofSetupDialog::applyhideClicked(int tt)" << endl;
#endif
  applySetup();
  //printSetup();
  emit SetupHide();
  emit SetupApply();

}

/*********************************************/

void VprofSetupDialog::initDatatab(){
  //cerr <<"initDatatab" << endl;
  //data tab (data setup could be different for each model)
  datatab = new QVBox(twd);

  int mymargin=5;
  int myspacing=5;
  datatab->setMargin( mymargin );
  datatab->setSpacing( myspacing );

  twd->addTab( datatab, "Data" );

  QWidget * xwidget = new QWidget(datatab);

  mymargin=5;
  myspacing=1;

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(xwidget,15,4);
  glayout->setMargin( mymargin );
  glayout->setSpacing( myspacing );

  int nrow=0;


  QLabel * label1 = new QLabel(tr("On/off"),xwidget);

  glayout->addWidget(label1,nrow,0);
  nrow++;

  vpSetups.push_back(new VprofSetup(xwidget,vprofm,TEMP,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,DEWPOINT,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,WIND,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,VERTWIND,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,RELHUM,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,DUCTING,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,SIGNWIND,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,TEXT,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,KINDEX,glayout,nrow++,0,true));
  vpSetups.push_back(new VprofSetup(xwidget,vprofm,GEOPOS,glayout,nrow++,0,true));


  //spinbox for pressure and temperature range
  //value of spinLow must not exceed value of spinHigh
  
  QLabel * pressurelabel = new QLabel(PRESSRANGE,xwidget);
  QLabel * templabel = new QLabel(TEMPRANGE,xwidget);
  pressurelabel->setAlignment(AlignLeft);
  templabel->setAlignment(AlignLeft);
  //pressure range 10-1200, steps of 50, init value 100-1050
  pressureSpinLow  = new QSpinBox(0,1200,50,xwidget);
  pressureSpinHigh = new QSpinBox(0,1200,50,xwidget);
  //temperature range -70-70, steps of 5, init value -30-30
  temperatureSpinLow = new QSpinBox(-70,70,5,xwidget);
  temperatureSpinHigh = new QSpinBox(-70,70,5,xwidget);
  //standardvalues
  pressureSpinLow->setValue(100);
  pressureSpinHigh->setValue(1050);
  temperatureSpinLow->setValue(-30);
  temperatureSpinHigh->setValue(+30);
  //units
  pressureSpinLow->setSuffix(" hPa");
  pressureSpinHigh->setSuffix(" hPa");
  temperatureSpinLow->setSuffix("°C");
  temperatureSpinHigh->setSuffix("°C");

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

  //end datatab
}


void VprofSetupDialog::initDiagramtab(){
  //diagram tab
  diagramtab = new QWidget(twd);
  twd->addTab( diagramtab, tr("Diagram") );

  int mymargin=5;
  int myspacing=5;

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(diagramtab,13,4);
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


  //here each of setup lines are defined - each VprofSetup is a line with
  //a checkbox and up to three comboboxes

  int nrow = 0;
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,PRESSLINES,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,TEMPLINES,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,LINEFLIGHT,glayout,++nrow,0,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,DRYADIABATS,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,WETADIABATS,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,MIXINGRATIO,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,PTLABELS,glayout,++nrow,0,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,FRAME,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,FLIGHTLEVEL,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,FLIGHTLABEL,glayout,++nrow,0,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,SEPWIND,glayout,++nrow,0,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,CONDTRAIL,glayout,++nrow,3,true));
  vpSetups.push_back(new VprofSetup(diagramtab,vprofm,BACKCOLOUR,glayout,++nrow,1,false));

  //end diagramtab

}


void VprofSetupDialog::initColourtab(){
  //colour tab
  colourtab = new QWidget(twd);
  twd->addTab( colourtab, tr("Colours") );


  int mymargin=5;
  int myspacing=5;

  // make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes
  QGridLayout * glayout = new QGridLayout(colourtab,13,4);
  glayout->setMargin( mymargin );
  glayout->setSpacing( myspacing );

  QLabel * label1 = new QLabel(" ",colourtab);
  QLabel * label2 = new QLabel(tr("Colour"),colourtab);
  QLabel * label3 = new QLabel(tr("Line thickness"),colourtab);
  glayout->addWidget(label1,0,0);
  glayout->addWidget(label2,0,1);
  glayout->addWidget(label3,0,2);

  int nrow = 0;
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data1",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data2",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data3",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data4",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data5",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data6",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data7",glayout,++nrow,2,false));
  vpSetups.push_back(new VprofSetup(colourtab,vprofm,"Data8",glayout,++nrow,2,false));

}



bool VprofSetupDialog::close(bool alsoDelete){
  emit SetupHide();
  return true;
}

