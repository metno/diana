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
#include <qslider.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qlcdnumber.h>
#include <qcheckbox.h>
#include <qpalette.h>
#include <qtooltip.h>
//#include <qfocusdata.h>
#include <q3frame.h>
#include <qapplication.h>
#include <qimage.h>
#include <q3scrollview.h>
#include <q3frame.h>

#include <qtButtonLayout.h>
#include <qtObsWidget.h>
#include <qtUtility.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3VBoxLayout>

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <math.h>




/*
  GENERAL DESCRIPTION: This widget takes several datatypes ( that might
  represent one plottype ) as input and generates a widget where zero, one or
  more datatypes might be selected by pressing the topbuttons.
  It also includes some sliders, checkboxes, comboboxes etc.
*/

ObsWidget::ObsWidget( QWidget* parent ):QWidget(parent)
{

#ifdef dObsDlg
  cerr<<"ObsWidget::ObsWidget called"<<endl;
#endif
  initOK=false;
}

void ObsWidget::setDialogInfo( Controller* ctrl,
			  ObsDialogInfo dialog,
			  int plottype_nr)
{
#ifdef dObsDlg
  cerr<<"ObsWidget::setDialogInfo"<<endl;
#endif

  initOK=true;
  ObsDialogInfo::PlotType &dialogInfo = dialog.plottype[plottype_nr];

  plotType = dialogInfo.name;

  // Button names and default values
  nr_dataTypes = dialogInfo.datatype.size();
  for( int i=0; i<nr_dataTypes; i++ ){
    ObsDialogInfo::Button b;
    b.name = dialogInfo.datatype[i].name;
    dataTypeButton.push_back(b);
    datatype.push_back(dialogInfo.datatype[i]);
  }
  button = dialogInfo.button;

  // Info about sliders, check boxes etc.
  pressureLevels = dialogInfo.pressureLevels.size();
  scaledensity   = dialog.density.scale;
  maxdensity     = dialog.density.maxValue;
  scalesize      = dialog.size.scale;
  scalediff      = dialog.timediff.scale;
  priorityList   = dialog.priority; //priority list

  //Info about colours
  cInfo = Colour::getColourInfo();

  vector<miString> defV = dialog.defValues.split(" ");
  int n=defV.size();
  int colIndex, devcol1Index, devcol2Index;
  for(int i=0;i<n;i++){
    vector<miString> stokens = defV[i].split("=");
    if(stokens.size()==2){
      if(stokens[0]=="colour")
	colIndex=getIndex(cInfo,stokens[1]);
      else if(stokens[0]=="devcolour1")
	devcol1Index=getIndex(cInfo,stokens[1]);
      if(stokens[0]=="devcolour2")
	devcol2Index=getIndex(cInfo,stokens[1]);
    }
  }

  nobutton = !button.size();
  bool devField=false;
  bool tempPrecision=false;
  bool allAirepsLevels=false;
  bool asFieldButton=false;
  bool orient=false;
  bool parameterName=false;
  bool moreTimes=false;
  markerboxVisible=false;
  leveldiffs=false;
  bool criteria=true;
  vector<miString> tokens = dialogInfo.misc.split(" ");
  n=tokens.size();
  for(int i=0;i<n;i++){
    vector<miString> stokens = tokens[i].split("=");
    bool on = true;
    if(stokens.size()==2 && stokens[1].downcase()=="false") on=false;
    if(stokens.size()){
      if(stokens[0]=="dev_field_button")
	devField = on;
      else if(stokens[0]=="tempPrecision")
	tempPrecision = on;
      else if(stokens[0]=="allAirepsLevels")
	allAirepsLevels = on;
      else if(stokens[0]=="markerboxVisible")
	markerboxVisible = on;
      else if(stokens[0]=="asFieldButton")
	asFieldButton = on;
      else if(stokens[0]=="leveldiff")
	leveldiffs = on;
      else if(stokens[0]=="orientation")
	orient = on;
      else if(stokens[0]=="parameterName")
	parameterName = on;
      else if(stokens[0]=="more_times")
	moreTimes = on;
      else if(stokens[0]=="criteria")
	criteria = on;
    }
  }

  // DECLARATION OF BUTTONS

  datatypeButtons =
    new ButtonLayout(this, dataTypeButton, 3);

  // sv = new QScrollView(this);
//     sv->setHScrollBarMode(QScrollView::AlwaysOff);
    parameterButtons=
      new ButtonLayout(this, button, 3);
//     sv->addChild(parameterButtons);

  connect( datatypeButtons,SIGNAL(outGroupClicked(int)),
	   SLOT(outTopClicked(int)));
  connect( datatypeButtons, SIGNAL(inGroupClicked(int)),
	   SLOT( inTopClicked(int)));
  connect( datatypeButtons, SIGNAL(rightClickedOn(miString)),
	   SLOT(rightClickedSlot(miString)));
  connect( parameterButtons, SIGNAL(rightClickedOn(miString)),
	   SLOT(rightClickedSlot(miString)));
  connect( this,SIGNAL(setRightClicked(miString,bool)),
	   parameterButtons,SLOT(setRightClicked(miString,bool)));

  //AND-Buttons
  allButton  = NormalPushButton(tr("All"),this);
  noneButton = NormalPushButton(tr("None"),this);
  defButton  = NormalPushButton(tr("Default"),this);
  Q3HBoxLayout* andLayout = new Q3HBoxLayout();
  andLayout->addWidget( allButton  );
  andLayout->addWidget( noneButton );
  andLayout->addWidget( defButton  );
  connect( allButton,SIGNAL(clicked()),
	   parameterButtons,SLOT(ALLClicked()));
  connect( noneButton,SIGNAL(clicked()),
	   parameterButtons,SLOT(NONEClicked()));
  connect( defButton,SIGNAL(clicked()),
	   parameterButtons,SLOT(DEFAULTClicked()));

  // PRESSURE LEVELS
  Q3HBoxLayout* pressureLayout = new Q3HBoxLayout();

  if(pressureLevels ){
    QLabel *pressureLabel;
    if(leveldiffs)
      pressureLabel = new QLabel(tr("Depth"),this);
    else
      pressureLabel = new QLabel(tr("Pressure"),this);

    pressureComboBox = new QComboBox(this);
    pressureComboBox->insertItem(tr("As field"));
    levelMap["asfield"] = 0;
//     vector<miString> vstr;
    int psize=dialogInfo.pressureLevels.size();
//     for(int i=0; i<psize; i++)
//       vstr.push_back(miString(dialogInfo.pressureLevels[i]));
    for(int i=1; i<psize+1; i++){
      miString str(dialogInfo.pressureLevels[psize-i]);
      pressureComboBox->insertItem(str.cStr());
      levelMap[str]=i;
    }

    pressureLayout->addWidget( pressureLabel );
    pressureLayout->addWidget( pressureComboBox );

    if(leveldiffs){
      QLabel* leveldiffLabel = new QLabel(tr("deviation"),this);
      leveldiffComboBox = new QComboBox(this);
      for(int i=0;i<4;i++){
	int aa=(int)pow(10.0,i);
	miString tmp(aa);
	leveldiffComboBox->insertItem(tmp.cStr());
	leveldiffMap[tmp]=i*3;
	tmp = miString(aa*2);
	leveldiffComboBox->insertItem(tmp.cStr());
	leveldiffMap[tmp]=1*3+1;
	tmp = miString(aa*5);
	leveldiffComboBox->insertItem(tmp.cStr());
	leveldiffMap[tmp]=i*3+2;
      }
      pressureLayout->addWidget( leveldiffLabel );
      pressureLayout->addWidget( leveldiffComboBox );
    }
    pressureLayout->addStretch();

  }

  //checkboxes
  orientCheckBox= new QCheckBox(tr("Horisontal orientation"),this);
  if(!orient)
    orientCheckBox->hide();
  showposCheckBox= new QCheckBox(tr("Show all positions"),this);
  tempPrecisionCheckBox= new QCheckBox(tr("Temperatures as integers"),this);
  if(!tempPrecision) tempPrecisionCheckBox->hide();
  parameterNameCheckBox= new QCheckBox(tr("Name of parameter"),this);
  if(!parameterName) parameterNameCheckBox->hide();
  moreTimesCheckBox= 
    new QCheckBox(tr("All observations (mixing different times)"),this);
  if(!moreTimes) moreTimesCheckBox->hide();
  devFieldCheckBox= new QCheckBox(tr("PPPP - MSLP-field"),this);
  if(!devField) devFieldCheckBox->hide();
  devColourBox1 = ColourBox( this, cInfo, true, devcol1Index );
  devColourBox2 = ColourBox( this, cInfo, true, devcol2Index );
  devColourBox1->hide();
  devColourBox2->hide();
  Q3HBoxLayout* devLayout = new Q3HBoxLayout();
  devLayout->addWidget(devFieldCheckBox);
  devLayout->addWidget(devColourBox1);
  devLayout->addWidget(devColourBox2);
  allAirepsLevelsCheckBox= new QCheckBox(tr("Aireps in all levels"),this);
  if( !allAirepsLevels ) allAirepsLevelsCheckBox->hide();

  //Onlypos & marker
  onlyposCheckBox= new QCheckBox(tr("Positions only"),this);

  markerBox = PixmapBox( this, markerName);
  if( markerName.size() == 0 )
    onlyposCheckBox->setEnabled(false);

  Q3HBoxLayout* onlyposLayout = new Q3HBoxLayout();
  onlyposLayout->addWidget( onlyposCheckBox );
  onlyposLayout->addWidget( markerBox );


  if( !markerboxVisible ) {
    markerBox->hide();
  }

  //Criteria list
  criteriaList = dialogInfo.criteriaList;
  currentCriteria = -1;
  criteriaCheckBox = new QCheckBox(tr("Criterias"),this);
  criteriaChecked(false);
  miString more_str[2] = { (tr("<<Less").latin1()),
			   (tr("More>>").latin1()) };
  moreButton= new ToggleButton( this, more_str);
  moreButton->setOn(false);
  if(!criteria){
    criteriaCheckBox->hide();
    moreButton->hide();
  }
  Q3HBoxLayout* criteriaLayout = new Q3HBoxLayout();
  criteriaLayout->addWidget( criteriaCheckBox );
  criteriaLayout->addWidget( moreButton );


  connect( moreButton, SIGNAL( toggled(bool)),SLOT( extensionSlot( bool ) ));
  connect(criteriaCheckBox,SIGNAL(toggled(bool)),SLOT(criteriaChecked(bool)));
  connect(devFieldCheckBox, SIGNAL(toggled(bool)),SLOT(devFieldChecked(bool)));
  connect(onlyposCheckBox, SIGNAL(toggled(bool)), SLOT(onlyposChecked(bool)));

  tempPrecisionCheckBox->setChecked( true );
  allAirepsLevelsCheckBox->setChecked( false );


  // LCD numbers and sliders

  QLabel* densityLabel = new QLabel( tr("Density"), this);
  QLabel* sizeLabel = new QLabel( tr("Size"), this);
  QLabel *diffLabel = new QLabel( tr("Timediff"), this);

  densityLcdnum = LCDNumber( 5, this); // 4 siffer
  sizeLcdnum = LCDNumber( 5, this);
  diffLcdnum = LCDNumber( 5, this);

  for(int i=0;i<13;i++)
    time_slider2lcd.push_back(i*15);

  densitySlider = Slider( dialog.density.minValue, dialog.density.maxValue,
			  1, dialog.density.value,Qt::Horizontal, this);
  sizeSlider = Slider( dialog.size.minValue, dialog.size.maxValue,
		       1, dialog.size.value, Qt::Horizontal, this);
  diffSlider= Slider( 0,time_slider2lcd.size(), 1, 4,
		      Qt::Horizontal, this);

  diffComboBox = new QComboBox(this);
  diffComboBox->insertItem("3t");
  diffComboBox->insertItem("24t");
  diffComboBox->insertItem("7d");

  displayDiff(diffSlider->value());

  Q3GridLayout*slidergrid = new Q3GridLayout( 3, 3);
  slidergrid->addWidget( densityLabel, 0, 0 );
  slidergrid->addWidget( densityLcdnum,0, 1 );
  slidergrid->addWidget( densitySlider,0, 2 );
  slidergrid->addWidget( sizeLabel,    1, 0 );
  slidergrid->addWidget( sizeLcdnum,   1, 1);
  slidergrid->addWidget( sizeSlider,   1, 2 );
  slidergrid->addWidget( diffLabel,    2, 0 );
  slidergrid->addWidget( diffLcdnum,   2, 1 );
  slidergrid->addWidget( diffSlider,   2, 2  );
  slidergrid->addWidget( diffComboBox, 2, 3  );


  //Priority list
  vector<miString> priName;
  for(int i=0; i<priorityList.size(); i++)
    priName.push_back(priorityList[i].name);
  pribox = ComboBox( this,priName,true);
  pribox->insertItem(tr("No priority list"),0);

  //Colour
   colourBox = ColourBox( this, cInfo, true, colIndex );


// CONNECT
  connect( densitySlider,SIGNAL( valueChanged(int)),SLOT(displayDensity(int)));
  connect( sizeSlider,SIGNAL(valueChanged(int)),SLOT(displaySize(int)));
  connect( diffSlider,SIGNAL(valueChanged(int)),SLOT(displayDiff(int)));
  connect( diffComboBox,SIGNAL(activated(int)),SLOT(diffComboBoxSlot(int)));
  connect( pribox, SIGNAL( activated(int) ), SLOT( priSelected(int) ) );


  // Layout for priority list, colours, criteria and extension
  Q3HBoxLayout* colourlayout = new Q3HBoxLayout();
  colourlayout->addWidget( pribox );
  colourlayout->addWidget( colourBox );

  // layout
  datatypelayout = new Q3HBoxLayout(5);
  datatypelayout->setAlignment(Qt::AlignHCenter);
  datatypelayout->addWidget( datatypeButtons );
  parameterlayout = new Q3HBoxLayout(5);
  parameterlayout->setAlignment(Qt::AlignHCenter);
  parameterlayout->addWidget( parameterButtons );
//   parameterlayout->addWidget( sv );


  // Create horizontal frame lines
  Q3Frame *line0 = new Q3Frame( this );
  line0->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );
  Q3Frame *line1 = new Q3Frame( this );
  line1->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );

  // LAYOUT
  vcommonlayout= new Q3VBoxLayout(2);
  vcommonlayout->addLayout( andLayout );
  vcommonlayout->addSpacing( 5 );
  vcommonlayout->addLayout( pressureLayout );
  vcommonlayout->addSpacing( 5 );
  vcommonlayout->addWidget( line0 );
  vcommonlayout->addWidget( orientCheckBox );
  vcommonlayout->addWidget( showposCheckBox );
  vcommonlayout->addWidget( tempPrecisionCheckBox );
  vcommonlayout->addWidget( parameterNameCheckBox );
  vcommonlayout->addWidget( moreTimesCheckBox );
  vcommonlayout->addLayout( devLayout );
  vcommonlayout->addWidget( allAirepsLevelsCheckBox );
  vcommonlayout->addLayout( onlyposLayout);
  vcommonlayout->addLayout( criteriaLayout );
  vcommonlayout->addWidget( line1 );
  vcommonlayout->addLayout( slidergrid );
  vcommonlayout->addLayout( colourlayout );

  vlayout= new Q3VBoxLayout( this, 5 ,5);
  vlayout->addSpacing( 5 );
  vlayout->addLayout( datatypelayout );
  if(parameterButtons)
    vlayout->addLayout( parameterlayout );
  vlayout->addLayout( vcommonlayout );

  densityLcdnum->display(((double)dialog.density.value)*dialog.density.scale);
  sizeLcdnum->display( ((double)dialog.size.value)*dialog.size.scale) ;

  allObs = false;

  pri_selected = 0;

  parameterButtons->DEFAULTClicked();
  parameterButtons->setEnabled(false);



// TOOLTIPS
  ToolTip();

}//END CONSTRUCTOR

void ObsWidget::ToolTip(){
  QToolTip::add( datatypeButtons,  tr("Data type") );
  QToolTip::add( devColourBox1,    tr("PPPP-MSLP<0"));
  QToolTip::add( devColourBox2,    tr("PPPP-MSLP>0"));
  QToolTip::add( moreTimesCheckBox,
		 tr("Affecting synoptic data: All observations in the time interval given, mixing observations with different times"));
  QToolTip::add( diffLcdnum,       tr("Max time difference"));
  QToolTip::add( diffComboBox,     tr("Max value for the slider"));
  QToolTip::add( colourBox,        tr("Colour") );
  return;
}



/***************************************************************************/


void ObsWidget::devFieldChecked(bool on){
  if (on) {
    devColourBox1->show();
    devColourBox2->show();
  } else {
    devColourBox1->hide();
    devColourBox2->hide();
  }
}

void ObsWidget::onlyposChecked(bool on){
  if (on) {
    markerBox->show();
    densitySlider->setEnabled(false);
    showposCheckBox->setEnabled(false);
    densityLcdnum->setEnabled(false);
  } else {
    if(!markerboxVisible){
      markerBox->hide();
    }
    densitySlider->setEnabled(true);
    showposCheckBox->setEnabled(true);
    densityLcdnum->setEnabled(true);
  }
}

void ObsWidget::criteriaChecked(bool on){
  if(on) {
    emit criteriaOn();
  } else {
    emit setRightClicked("ALL_PARAMS",false);
  }
}

/***************************************************************************/


void ObsWidget::priSelected( int index ){
  //priority file
  pri_selected = index;
}

/***************************************************************************/

void ObsWidget::displayDensity( int number ){
// This function is called when densitySlider sends a signal
// valueChanged(int)
// and changes the numerical value in the lcd display densityLcdnum

  double scalednumber= number* scaledensity;
  if(number==maxdensity){
    densityLcdnum->display( "ALLE" );
    allObs = true;
  }
  else{
    densityLcdnum->display( scalednumber );
    allObs = false;
  }
}

/***************************************************************************/

void ObsWidget::displaySize( int number ){
  /* This function is called when sizeSlider sends a signal valueChanged(int)
     and changes the numerical value in the lcd display sizeLcdnum */

  double scalednumber= number* scalesize;
  sizeLcdnum->display( scalednumber );
}


void ObsWidget::displayDiff( int number ){
/* This function is called when diffSslider sends a signal valueChanged(int)
     and changes the numerical value in the lcd display diffLcdnum */

  if( number == time_slider2lcd.size() ) {
    diffLcdnum->display( tr("ALL") );
    timediff_minutes = "alltimes";
    return;
  }

  miString str;
  int totalminutes = time_slider2lcd[number];
  timediff_minutes = miString(totalminutes);
  if(diffComboBox->currentItem()<2){
    int hours = totalminutes/60;
    int minutes= totalminutes-hours*60;
    ostringstream ostr;
    ostr << hours << ":" << setw(2) << setfill('0') << minutes;
    str= ostr.str();
  } else {
    ostringstream ostr;
    ostr << (totalminutes/60/24) << " d";
    str= ostr.str();
  }

  diffLcdnum->display( str.c_str() );

}

/***************************************************************************/
void ObsWidget::diffComboBoxSlot( int number )
{

  int index = time_slider2lcd[diffSlider->value()];
  bool maxValue = ( diffSlider->value() == time_slider2lcd.size() );

  int maxSliderValue, sliderStep;
  if(number==0){
    maxSliderValue = 13;
    sliderStep = 15;
  } else if(number==1){
    maxSliderValue = 25;
    sliderStep = 60;
  } else {
    maxSliderValue = 8;
    sliderStep = 60*24;
  }

  diffSlider->setRange(0,maxSliderValue);
  time_slider2lcd.clear();
  for(int i=0;i<maxSliderValue;i++)
    time_slider2lcd.push_back(i*sliderStep);

  //set slider
  if(maxValue){
    index = maxSliderValue;
  } else {
    index /= sliderStep;
    if(index>maxSliderValue-1) index = maxSliderValue-1;
  }

  diffSlider->setValue(index);
  displayDiff(diffSlider->value());
}
/***************************************************************************/

void ObsWidget::inTopClicked( int id )
{
//  This function is called when datatypeButtons sends a signal
// inGroupClicked(int),
//      and is sent when a new datatype is selected.

  if(parameterButtons)
    parameterButtons->enableButtons(datatype[id].active);


  // Names of datatypes selected are sent to controller
  QApplication::setOverrideCursor( Qt::waitCursor );
  emit getTimes();
  QApplication::restoreOverrideCursor();

  //  cerr<<"ObsWidget::inTopClicked returned"<<endl;
}


void ObsWidget::outTopClicked( int id )
{

// This function is called when datatypeButtons sends a signal
//  outGroupClicked(int), and is sent when an already selected
//  datatype is unselected, that is the button is pressed out.

  if(parameterButtons)
    parameterButtons->setEnabled(false); //disable all parameter buttons

  // "click the other buttons again"
  for(int i=0; i<nr_dataTypes; i++){
    if(datatypeButtons->isOn(i))
      if(parameterButtons)
	parameterButtons->enableButtons(datatype[i].active);
  }

  // Names of datatypes selected are sent to controller
  QApplication::setOverrideCursor( Qt::waitCursor );
  emit getTimes();
  QApplication::restoreOverrideCursor();


  //  cerr<<"ObsWidget::outTopClicked returned"<<endl;
}

void ObsWidget::markButton(const miString& str,bool on)
{
  if(criteriaCheckBox->isChecked())
    emit setRightClicked(str,on);
}

void ObsWidget::rightClickedSlot(miString str)
{
  if(criteriaCheckBox->isChecked())
    emit rightClicked(str);
}

/*********************************************/



/*****************************************************************/
vector<miString> ObsWidget::getDataTypes(void){

  vector<miString> vstr = datatypeButtons->getOKString();
  return vstr;

}

/****************************************************************/
miString ObsWidget::makeString(bool forLog){

  miString str, datastr;

  if(forLog)
    str = "plot=" + plotType + " ";
  else
    str = "OBS plot=" + plotType + " ";

  if (dVariables.data.size()) {
    str+= "data=";
    for (int i=0; i<dVariables.data.size(); i++)
      datastr+= dVariables.data[i] + ",";
    datastr= datastr.substr(0,datastr.length()-1);
    str+= datastr + ' ';
  }

  if (dVariables.parameter.size()) {
    str+= "parameter=";
    for (int i=0; i<dVariables.parameter.size(); i++)
      str+= dVariables.parameter[i] + ",";
    str[str.length()-1]=' ';
  }

  map<miString,miString>::iterator p= dVariables.misc.begin();
  for (; p!=dVariables.misc.end(); p++)
    str += p->first + "=" + p->second + " ";

  shortname = "OBS " + plotType + " " + datastr;

  return str;
}

miString ObsWidget::getOKString(bool forLog){

  shortname.clear();

  miString str;

  dVariables.plotType = plotType;

  dVariables.data = datatypeButtons->getOKString();

  if(!dVariables.data.size() && !forLog)
    return str;
  if(parameterButtons)
    dVariables.parameter = parameterButtons->getOKString(forLog);

  if( tempPrecisionCheckBox->isOn() )
    dVariables.misc["tempprecision"]="true";

  if( parameterNameCheckBox->isOn() )
    dVariables.misc["parametername"]="true";

  if( moreTimesCheckBox->isOn() )
    dVariables.misc["moretimes"]="true";

  if( orientCheckBox->isOn() )
    dVariables.misc["orientation"]="horizontal";

  if( showposCheckBox->isOn() )
    dVariables.misc["showpos"]="true";

  if( onlyposCheckBox->isOn() ){
    dVariables.misc["onlypos"]="true";
  }
  if(markerName.size())
    dVariables.misc["image"] = markerName[markerBox->currentItem()];

  if( devFieldCheckBox->isOn() ){
    dVariables.misc["devfield"] = "true";
    dVariables.misc["devcolour1"] = cInfo[devColourBox1->currentItem()].name;
    dVariables.misc["devcolour2"] = cInfo[devColourBox2->currentItem()].name;
  }

  if( allAirepsLevelsCheckBox->isOn() )
    dVariables.misc["allAirepsLevels"]="true";

  if( pressureLevels ){
    if(pressureComboBox->currentItem()>0 ){
      dVariables.misc["level"] = pressureComboBox->currentText().latin1();
    } else {
      dVariables.misc["level"] = "asfield";
    }
  }

  if( leveldiffs ){
   dVariables.misc["leveldiff"] = leveldiffComboBox->currentText().latin1();
  }

  if( allObs )
    dVariables.misc["density"] = "allobs";
  else{
    miString tmp(densityLcdnum->value());
    dVariables.misc["density"]= tmp;
  }


  miString sc(sizeLcdnum->value());
  dVariables.misc["scale"] = sc;

  dVariables.misc["timediff"]= timediff_minutes;

  if( pri_selected > 0 )
    dVariables.misc["priority"]=priorityList[pri_selected-1].file;

  dVariables.misc["colour"] = cInfo[colourBox->currentItem()].name;

  str = makeString(forLog);

  //clear old settings
  dVariables.misc.clear();

  //Criteria
  //  cerr <<"getokstring - criteria"<<endl;
  if(!str.exists()) return str;

  if(forLog){
    int n = criteriaList.size();
    for(int i=0; i<n; i++){
      int m = criteriaList[i].criteria.size();
      if( m==0 ) continue;
      str+= " criteria=";
      str+= criteriaList[i].name;
      str += ";";
      for( int j=0; j<m; j++){
	//      cerr <<"+++Criteria:"<<criteriaList[i].criteria[j]<<endl;
	vector<miString> sub = criteriaList[i].criteria[j].split(" ");
      int size=sub.size();
      for(int k=0;k<size;k++){
	str += sub[k];
	if(k<size-1) str += ",";
      }
      if(j<m-1) str += ";";
      }
    }
  } else{
    if(!criteriaCheckBox->isChecked()) return str;
    int m = savedCriteria.criteria.size();
    if( m==0 ) return str;
    str+= " criteria=";
    for( int j=0; j<m; j++){
      vector<miString> sub = savedCriteria.criteria[j].split(" ");
      int size=sub.size();
      for(int k=0;k<size;k++){
	str += sub[k];
	if(k<size-1) str += ",";
      }
      if(j<m-1) str += ";";
    }
  }


  return str;
}

miString ObsWidget::getShortname()
{

  return shortname;

}



void ObsWidget::putOKString(const miString& str){

  dVariables.misc.clear();

  decodeString(str,dVariables,false);
  updateDialog(true);

//#####################################################
  emit getTimes();
//#####################################################
}


void ObsWidget::readLog(const miString& str){

  //  cerr <<"ObsWidget::readLog"<<endl;
  setFalse();
  dVariables.misc.clear();
  decodeString(str,dVariables,true);
  updateDialog(false);

}

void ObsWidget::requestQuickUpdate(miString& oldstr,
				miString& newstr)
{
//   cerr <<"ObsWidget::requestQuickUpdate"<<endl;

  dialogVariables oldvar;

  decodeString(oldstr,dVariables);
  miString ostr = getOKString();
  decodeString(ostr,oldvar);
  decodeString(newstr,dVariables);

  int n,m;
  bool ok= true;
  //data must be the same or more
  n=oldvar.data.size();
  m=dVariables.data.size();
  if(n>0 && oldvar.data[0].contains("@")){ //ok if oldstr contains @
    dVariables.data = oldvar.data;
  } else if (m<n) {
    ok= false;
  } else {
    for (int i=0; i<n; i++) {
      int j=0;
      while (j<m &&
	     oldvar.data[i].downcase() != dVariables.data[j].downcase()) j++;
      if (j==m) ok= false;
    }
  }

  //parameter must be the same or more
  n=oldvar.parameter.size();
  m=dVariables.parameter.size();
  if(n>0 && oldvar.parameter[0].contains("@")){ //ok if oldstr contains @
    dVariables.parameter = oldvar.parameter;
  } else if (m<n) {
    ok= false;
  } else {
    for (int i=0; i<n; i++) {
      int j=0;
      while (j<m &&
	     oldvar.parameter[i].downcase() != dVariables.parameter[j].downcase())
	j++;
      if (j==m) ok= false;
    }
  }

  if (!ok) {
    newstr = oldstr;
    return;
  }


  //it is allowed to change the rest
  //if old contains "@", old will be used, else new will be used

  map<miString,miString>::iterator p= oldvar.misc.begin();
  for( ; p!=oldvar.misc.end(); p++){
     if( p->second.contains("@") )
       dVariables.misc[p->first]=p->second;
  }

  // level change not allowed
  dVariables.misc["level"] = oldvar.misc["level"];


  newstr = makeString();
  //  cerr <<"ObsWidget:newstr:"<<newstr<<endl;

}

void ObsWidget::updateDialog(bool setOn){

//    cerr<<"ObsWidget::updateDialog"<<endl;
  int number,m,j;
  double scalednumber;

  //plotType
  plotType = dVariables.plotType;

  //data types
  if ( setOn ){
    m = dVariables.data.size();
    for(j=0; j<m; j++){
      //cerr <<"updateDialog: "<<dVariables.data[j]<<endl;
      int index = datatypeButtons->setButtonOn(dVariables.data[j]);
      if(index<0) continue;
      if(parameterButtons)
	parameterButtons->enableButtons(datatype[index].active);
    }
  }

  //parameter
  if(parameterButtons) {
  parameterButtons->NONEClicked();
  m = dVariables.parameter.size();
  for(j=0; j<m; j++){
    //old syntax
    miString para = dVariables.parameter[j].downcase();
    if(para == "dd_ff" || para == "vind")
      dVariables.parameter[j] = "wind";
    if(para == "kjtegn")
      dVariables.parameter[j] = "id";
    if(para == "dato")
      dVariables.parameter[j] = "date";
    if(para == "tid") 
      dVariables.parameter[j] = "time";
    if(para == "høyde") 
      dVariables.parameter[j] = "height";
    parameterButtons->setButtonOn(dVariables.parameter[j]);
  }
  }
  //temp precision
  if (dVariables.misc.count("tempprecision") &&
      dVariables.misc["tempprecision"] == "true"){
    tempPrecisionCheckBox->setChecked(true);
  }

  //parameterName
  if (dVariables.misc.count("parametername") &&
dVariables.misc["parametername"] == "true"){
    parameterNameCheckBox->setChecked(true);
  }

  //moreTimes (not from log)
  if (setOn && dVariables.misc.count("moretimes") &&
      dVariables.misc["moretimes"] == "true"){
    moreTimesCheckBox->setChecked(true);
  }

  //dev from field
  if (dVariables.misc.count("devfield") &&
      dVariables.misc["devfield"] == "true"){
    devFieldCheckBox->setChecked(true);
    devFieldChecked(true);
    number= getIndex( cInfo, dVariables.misc["devcolour1"]);
    if (number>=0) {
      devColourBox1->setCurrentItem(number);
    }
    number= getIndex( cInfo, dVariables.misc["devcolour2"]);
    if (number>=0) {
      devColourBox2->setCurrentItem(number);
    }
  }

  //allAirepsLevels
  if (dVariables.misc.count("allairepslevels") &&
      dVariables.misc["allairepslevels"] == "true"){
    allAirepsLevelsCheckBox ->setChecked(true);
  }

  //orient
  if (dVariables.misc.count("orientation") &&
      dVariables.misc["orientation"] == "horizontal"){
    orientCheckBox ->setChecked(true);
  }

  //showpos
  if (dVariables.misc.count("showpos") &&
      dVariables.misc["showpos"] == "true"){
    showposCheckBox ->setChecked(true);
  }

  //criteria
  if (dVariables.misc.count("criteria") &&
      dVariables.misc["criteria"] == "true"){
    criteriaCheckBox ->setChecked(true);
    criteriaChecked(true);
  }

  //level
  if (pressureLevels && dVariables.misc.count("level")
      && levelMap.count(dVariables.misc["level"])){
    number = levelMap[dVariables.misc["level"]];
    pressureComboBox->setCurrentItem(number);
  }

  if (leveldiffs && dVariables.misc.count("leveldiff")
      && leveldiffMap.count(dVariables.misc["leveldiff"])){
    number = leveldiffMap[dVariables.misc["leveldiff"]];
    leveldiffComboBox->setCurrentItem(number);
  }

  //density
  if (dVariables.misc.count("density")){
    if(dVariables.misc["density"]=="allobs") {
      allObs=true;
      number= maxdensity;
    } else {
      scalednumber= atof(dVariables.misc["density"].c_str());
      number= int(scalednumber/scaledensity + 0.5);
      allObs=false;
    }
  } else {
    number= int(1/scaledensity + 0.5);
    allObs=false;
  }
  densitySlider->setValue(number);
  displayDensity(number);

  //scale
  if (dVariables.misc.count("scale") ){
    scalednumber= atof(dVariables.misc["scale"].c_str());
  }else{
    scalednumber = 1.0;
  }
  number= int(scalednumber/scalesize + 0.5);
  sizeSlider->setValue(number);
  displaySize(number);

  //timediff
  int i=0;
  int maxSliderValue = 13;
  int sliderStep = 15;
  number = 60/sliderStep;
  if (dVariables.misc.count("timediff") ){
    timediff_minutes= dVariables.misc["timediff"];
    if( timediff_minutes == "alltimes"){
      number=time_slider2lcd.size();
    } else {
      int iminutes = atoi(timediff_minutes.cStr());
      if(iminutes<3*60){
	i=0;
	maxSliderValue = 13;
	sliderStep = 15;
      } else if(iminutes<24*60){
	i=1;
	maxSliderValue = 25;
	sliderStep = 60;
      } else {
	i=2;
	maxSliderValue = 8;
	sliderStep = 60*24;
      }
      number = iminutes/sliderStep;
      if(number>maxSliderValue-1) number = maxSliderValue-1;
    }
  }
  diffComboBox->setCurrentItem(i);
  diffSlider->setRange(0,maxSliderValue);
  time_slider2lcd.clear();
  for(int i=0;i<maxSliderValue;i++)
    time_slider2lcd.push_back(i*sliderStep);
  diffSlider->setValue(number);
  displayDiff(number);

  //onlypos
  if (dVariables.misc.count("onlypos") &&
      dVariables.misc["onlypos"] == "true"){
    onlyposCheckBox ->setChecked(true);
    onlyposChecked(true);
  }

  //Image
  if (dVariables.misc.count("image") ){
    number = getIndex(markerName,dVariables.misc["image"]);
    if(number>=0)
      markerBox->setCurrentItem(number);
  }

  //priority
  m= priorityList.size();
  j= 0;
  if(dVariables.misc.count("priority")){
    while (j<m && dVariables.misc["priority"]!=priorityList[j].file) j++;
    if (j<m) {
      pri_selected= ++j;
      pribox->setCurrentItem(pri_selected);
    } else {
      pribox->setCurrentItem(0);
    }
  } else {
    pribox->setCurrentItem(0);
  }

  //colour
  if (dVariables.misc.count("colour") ){
    number= getIndex( cInfo, dVariables.misc["colour"]);
    if (number>=0)
      colourBox->setCurrentItem(number);
  }


}

void ObsWidget::decodeString(const miString& str, dialogVariables& var,
			  bool fromLog)
{
//   cerr <<"decodeString:"<<str<<endl;
  vector<miString> parts= str.split(' ',true);
  vector<miString> tokens;
  int nparts= parts.size();

  for (int i=0; i<nparts; i++) {
    tokens= parts[i].split(1,'=',false);
    if (tokens.size()==2) {
      if (tokens[0]=="plot" ){
	var.plotType = tokens[1];
      } else if (tokens[0]=="data" ){
	var.data = tokens[1].split(',');
      }else if (tokens[0]=="parameter" ){
	var.parameter = tokens[1].split(',');
      }else if (tokens[0]=="criteria" ){
	if(!fromLog){
	  var.misc[tokens[0]]="true";
	  miString ss = tokens[1].replace(',',' ');
	  saveCriteria(ss.split(';'));
	} else {
	  miString ss = tokens[1].replace(',',' ');
	  vector<miString> vstr = ss.split(';');
	  if(vstr.size()>1){
	    miString name=vstr[0];
	    vstr.erase(vstr.begin());
	    saveCriteria(vstr,name);
	  } else {
	    saveCriteria(tokens[1].split(","));
	  }
	}
      } else {
	var.misc[tokens[0].downcase()]=tokens[1];
      }
    }
  }
}

void ObsWidget::setFalse(){

  datatypeButtons->NONEClicked();
  //   parameterButtons->NONEClicked();
  if(parameterButtons)
    parameterButtons->setEnabled(false);

  devFieldCheckBox->setChecked(false);
  devFieldChecked(false);

  tempPrecisionCheckBox->setChecked(false);

  parameterNameCheckBox->setChecked(false);

  moreTimesCheckBox->setChecked(false);

  allAirepsLevelsCheckBox->setChecked(false);

  orientCheckBox->setChecked(false);

  showposCheckBox->setChecked(false);

  onlyposCheckBox->setChecked(false);
  onlyposChecked(false);

  criteriaCheckBox->setChecked(false);
  criteriaChecked(false);

  if( pressureLevels ){
    pressureComboBox->setCurrentItem(0);
  }
}

void ObsWidget::setDatatype( const miString& type)
{
  int index = datatypeButtons->setButtonOn(type);
  if(index<0) return;
  if(parameterButtons){
    parameterButtons->enableButtons(datatype[index].active);
    parameterButtons->DEFAULTClicked();
  }
}

void ObsWidget::newParamButtons(ObsDialogInfo dialog, int nr)
{
  //  cerr <<"newParamButtons"<<endl;
  miString ok = getOKString();

  if(parameterButtons){
//     sv->removeChild(parameterButtons);
    parameterButtons->hide();
    delete parameterButtons;
    parameterButtons=NULL;
  }

  if(nr>-1){
    datatype=dialog.plottype[nr].datatype;
    button = dialog.plottype[nr].button;
    parameterButtons =
    new ButtonLayout(this, button, 3);
  }

//     parameterButtons =
//       new ButtonLayout(this, button, 3, true, colours, true);
//     connect( parameterButtons, SIGNAL(rightClickedOn(miString)),
// 	     SLOT(rightClickedSlot(miString)));
//     connect( this,SIGNAL(setRightClicked(miString,bool)),
// 	     parameterButtons,SLOT(setRightClicked(miString,bool)));
//     connect( allButton,SIGNAL(clicked()),
// 	     parameterButtons,SLOT(ALLClicked()));
//     connect( noneButton,SIGNAL(clicked()),
// 	     parameterButtons,SLOT(NONEClicked()));
//     connect( defButton,SIGNAL(clicked()),
// 	     parameterButtons,SLOT(DEFAULTClicked()));

//   }

//   sv->addChild(parameterButtons);
//   sv->update();

  vlayout->removeChild( datatypelayout );
  vlayout->removeChild( parameterlayout );
  vlayout->removeChild( vcommonlayout );

  if(parameterButtons)
    parameterButtons->show();

  delete vlayout;

  if(parameterButtons){
    parameterlayout->removeChild( parameterButtons );
    delete parameterlayout;
    parameterlayout = new Q3HBoxLayout(5);
    parameterlayout->setAlignment(Qt::AlignHCenter);
    parameterlayout->addWidget( parameterButtons );
  }
  vlayout= new Q3VBoxLayout( this, 5 ,5);
  vlayout->addLayout( datatypelayout );
  if(parameterButtons)
    vlayout->addLayout( parameterlayout );
  vlayout->addLayout( vcommonlayout );
  //  vlayout->activate();
  vlayout->freeze();

  if(parameterButtons){
    if(ok.exists()){
      putOKString(ok);
    } else {
      datatypeButtons->ALLClicked();
      parameterButtons->DEFAULTClicked();
    }
  }

//   nobutton=false;

}


void ObsWidget::extensionSlot(bool on)
{
  if(on)
    criteriaCheckBox->setChecked(true);

  emit extensionToggled(on);

}


ObsDialogInfo::CriteriaList ObsWidget::getCriteriaList()
{

  if(criteriaList.size()==0 || currentCriteria < 0){
    ObsDialogInfo::CriteriaList cl;
    return cl;
  }

  savedCriteria = criteriaList[currentCriteria];
  return criteriaList[currentCriteria];

}

bool ObsWidget::setCurrentCriteria(int i)
{

  if( i<criteriaList.size() ){
    currentCriteria=i;
    return true;
  }

  return false;

}

void ObsWidget::saveCriteria(const vector<miString>& vstr)
{

  savedCriteria.criteria = vstr;

}

bool ObsWidget::saveCriteria(const vector<miString>& vstr,
			     const miString& name)
{
  //  cerr <<"saveCriteria"<<endl;

  //don't save list whithout name
  if( !name.exists() ) return false;

  //find list
    int n = criteriaList.size();
    int i=0;
    while(i<n && criteriaList[i].name != name) i++;

  //list not found, make new list
  if(i==n){
    if(vstr.size()==0) return false;  //no list
    //    cerr <<"Ny liste:"<<name<<endl;
    ObsDialogInfo::CriteriaList clist;
    clist.name=name;
    clist.criteria=vstr;
    criteriaList.push_back(clist);
    return true; //new item
  }

  //list found
  if(vstr.size()==0) {  //delete list
    criteriaList.erase(criteriaList.begin()+i);
    return false;
  }

  //change list
  criteriaList[i].criteria =vstr;
  return false; //no new item

}


bool ObsWidget::getCriteriaLimits(const miString& name, int& low, int&high)
{

  int n = button.size();
  for(int i=0; i<n; i++)
    if(button[i].name  == name){
      low = button[i].low;
      high = button[i].high;
//       cerr <<"High:"<<high<<endl;
//       cerr <<"Low:"<<low<<endl;
      if(high==low) return false;
      return true;
    }

  if(name=="dd"){
    low = 0;
    high = 360;
    return true;
  }
  if(name=="ff"){
    low = 0;
    high = 100;
    return true;
  }

  return false;

}

vector<miString> ObsWidget::getCriteriaNames()
{

  vector<miString> critName;
  for(int i=0; i<criteriaList.size(); i++)
    critName.push_back(criteriaList[i].name);
  return critName;

}


