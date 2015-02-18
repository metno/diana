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
//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtTrajectoryDialog.h"
#include "qtUtility.h"
#include "qtGeoPosLineEdit.h"
#include "diLinetype.h"

#include <puTools/miStringFunctions.h>

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QString>
#include <QToolTip>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.TrajectoryDialog"
#include <miLogger/miLogging.h>

using namespace std;

TrajectoryDialog::TrajectoryDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), contr(llctrl)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  //caption to appear on top of dialog
  setWindowTitle(tr("Trajectories"));

  //define colours
  colourInfo = Colour::getColourInfo();


  // ********** create the various QT widgets to appear in dialog ***********

  //colours
  QLabel* collabel = new QLabel(tr("Colour"),this);
  colbox= ColourBox( this, colourInfo);
  connect(colbox,SIGNAL(activated ( int )),SLOT(colourSlot(int)));

  // combobox - line width
  QLabel* lineWidthLabel= new QLabel(tr("Line width"), this);
  lineWidthBox= LinewidthBox( this );
  connect( lineWidthBox, SIGNAL( activated(int) ),
	   SLOT( lineWidthSlot(int) ) );

  // line types
  linetypes = Linetype::getLinetypeNames();
  QLabel *lineTypeLabel= new QLabel( tr("Line type"), this );
  lineTypeBox=  LinetypeBox( this);
  connect( lineTypeBox, SIGNAL( activated(int) ),
	   SLOT( lineTypeSlot(int) ) );

  //time marker
  QLabel* timeLabel = new QLabel( tr("Time marks"), this );
  timeSpin = new QSpinBox(this);
  timeSpin->setMinimum(0);
  timeSpin->setMaximum(360);
  timeSpin->setSingleStep(30);
  timeSpin->setSpecialValueText(tr("Off"));
  timeSpin->setSuffix(tr("min"));
  timeSpin->setValue(0);
  timeSpin->setMaximumWidth(60);
  connect(timeSpin,SIGNAL(valueChanged(int)),SLOT(timeSpinSlot(int)));

  //Number of positions, radius and kind of marker
  QLabel* numposLabel = new QLabel(tr("No. of positions"), this);
  numposBox = new QComboBox(this);
  numposBox->addItem("1");
  numposBox->addItem("5");
  numposBox->addItem("9");
  connect(numposBox, SIGNAL(activated(int)),SLOT(numposSlot(int)));

  //radius
  QLabel* radiusLabel = new QLabel( tr("Radius"), this );
  radiusSpin = new QSpinBox(this);
  radiusSpin->setMinimum(10);
  radiusSpin->setMaximum(500);
  radiusSpin->setSingleStep(10);
  radiusSpin->setValue(100);
  radiusSpin->setSuffix(tr("km"));
  connect(radiusSpin,SIGNAL(valueChanged(int)),SLOT(radiusSpinChanged(int)));

    /*****************************************************************/

    //Positions
  posButton = new QCheckBox(tr("Select positions on map"),this);
  posButton->setChecked(true);
  connect( posButton, SIGNAL( toggled(bool)), SLOT( posButtonToggled(bool) ) );

  QLabel* posLabel = new QLabel( tr("Write positions (Lat Lon):"), this );
  edit = new GeoPosLineEdit(this);
  edit->setToolTip(tr("Lat Lon (deg:min:sec or decimal)") );
  connect( edit, SIGNAL( returnPressed()), SLOT( editDone() ));

  posList = new QListWidget(this);
  connect( posList, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( posListSlot( ) ) );

  //push button to delete last pos
  deleteButton = new QPushButton(tr("Delete"), this );
  connect( deleteButton, SIGNAL(clicked()),SLOT(deleteClicked()));

  //push button to delete all
  deleteAllButton = new QPushButton( tr("Delete all"), this);
  connect( deleteAllButton, SIGNAL(clicked()),SLOT(deleteAllClicked()));

  //push button to start calculation
  startCalcButton = new QPushButton( tr("Start computation"), this );
  connect(startCalcButton, SIGNAL(clicked()), SLOT( startCalcButtonClicked()));

  fieldName = new QLabel(this);
  fieldName->setAlignment(Qt::AlignCenter);

  //push button to show help
  QPushButton * Help = NormalPushButton( tr("Help"), this);
  connect(  Help, SIGNAL(clicked()), SLOT( helpClicked()));

  //push button to print pos
  QPushButton * print = NormalPushButton( tr("Print"), this);
  print->setToolTip(tr("Print calc. positions to file: trajectory.txt") );
  connect(  print, SIGNAL(clicked()), SLOT( printClicked()));

  //push button to hide dialog
  QPushButton * Hide = NormalPushButton(tr("Hide"), this);
  connect( Hide, SIGNAL(clicked()), SIGNAL(TrajHide()));

  //push button to quit, deletes all trajectoryPlot objects
  QPushButton * quit = NormalPushButton( tr("Quit"), this);
  connect(quit, SIGNAL(clicked()), SLOT( quitClicked()) );

  QFrame *line1 = new QFrame( this );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  //now create a grid layout
  QGridLayout* gridlayout = new QGridLayout(this);
  gridlayout->setColumnStretch(1,1);
  gridlayout->addWidget( collabel,        0, 0 );
  gridlayout->addWidget( colbox,          0, 1 );
  gridlayout->addWidget( lineWidthLabel,  1, 0 );
  gridlayout->addWidget( lineWidthBox,    1, 1 );
  gridlayout->addWidget( lineTypeLabel,   2, 0 );
  gridlayout->addWidget( lineTypeBox,     2, 1 );
  gridlayout->addWidget( timeLabel,       3, 0 );
  gridlayout->addWidget( timeSpin,        3, 1 );
  gridlayout->addWidget( line1,           4, 0, 1, 2 );
  gridlayout->addWidget( numposLabel,     5, 0 );
  gridlayout->addWidget( numposBox,       5, 1 );
  gridlayout->addWidget( radiusLabel,     6, 0 );
  gridlayout->addWidget( radiusSpin,      6, 1 );
  gridlayout->addWidget( posButton,       7, 0, 1, 2);
  gridlayout->addWidget( posLabel,        8, 0, 1, 2);
  gridlayout->addWidget( edit,            9, 0, 1, 2);
  gridlayout->addWidget( posList,        10, 0, 1, 2);
  gridlayout->addWidget( deleteButton,   11, 0);
  gridlayout->addWidget( deleteAllButton,11, 1);
  gridlayout->addWidget( startCalcButton,12, 0, 1, 2);
  gridlayout->addWidget( fieldName,      13, 0, 1, 2);
  gridlayout->addWidget( Help,           14, 0 );
  gridlayout->addWidget( print,          14,1);
  gridlayout->addWidget( Hide,           15,0 );
  gridlayout->addWidget( quit,           15,1 );

}
/********************************************************/


void TrajectoryDialog::posButtonToggled(bool b){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::posButtonToggled  b="<<b);
#endif
  //called when "Select positions om map" is clicked

  emit markPos(b);
  vector<string> vstr;
  vstr.push_back("clear"); //delete all trajectories
  contr->trajPos(vstr);

}

/*********************************************/

void TrajectoryDialog::numposSlot(int i){

  int current = posList->currentRow();
  if(current<0) return;

  positionVector[current].numPos=numposBox->currentText().toStdString();
  emit updateTrajectories();
}

/*********************************************/

void TrajectoryDialog::radiusSpinChanged(int i){
  //this slot is called when the spin box is changed

  if(i==110){
    // qt4 fix: setSteps() is removed from QRangeControl
    // using alternative method to set linestep(now called singlestep)
    // pagestep is NOT set at all!
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);

    radiusSpin->setValue(150);
  } else if(i==100){
    // Old line: radiusSpin->setSteps(10,10);
    radiusSpin->setSingleStep(10);
  }

  int current = posList->currentRow();
  if(current<0) return;

  positionVector[current].radius=i;
  emit updateTrajectories();
}


/****************************************************************************/
void TrajectoryDialog::posListSlot(){

  int current = posList->currentRow();
  if(current<0) return; //empty list

  radiusSpin->setValue(positionVector[current].radius);
  if(positionVector[current].radius>100)
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);
  if(positionVector[current].numPos == "5")
    numposBox->setCurrentIndex(1);
  else if(positionVector[current].numPos == "1")
    numposBox->setCurrentIndex(0);
  else
    numposBox->setCurrentIndex(2);

    vector<string> vstr;
    vstr.push_back("clear");//delete all trajectories
    vstr.push_back("delete"); //delete all start positions
    contr->trajPos(vstr);
    sendAllPositions();
  emit updateTrajectories();
}
/****************************************************************************/

void TrajectoryDialog::editDone(){
  //this slot is called when return is pressed in the line edit

  vector<string> vstr;
  vstr.push_back("clear"); //delete all trajectories
  contr->trajPos(vstr);

  float lat=0,lon=0;
  if(!edit->getValues(lat,lon))
    METLIBS_LOG_DEBUG("getValues returned false");

  mapPos(lat,lon);
  edit->clear();
}


/*********************************************/

void TrajectoryDialog::deleteClicked(){
  //delete selected group of start positions

  if(posList->currentRow()<0) return; //empty list

  if (positionVector.size()==1) {
    deleteAllClicked();
  }  else {
    vector<posStruct>::iterator p;
    p= positionVector.begin()+posList->currentRow();
    positionVector.erase(p);
    posList->takeItem(posList->currentRow());

    vector<string> vstr;
    vstr.push_back("clear");//delete all trajectories
    vstr.push_back("delete"); //delete all start positions
    contr->trajPos(vstr);
    sendAllPositions();

    emit updateTrajectories();
    posListSlot();
  }

}

/*********************************************/
void TrajectoryDialog::deleteAllClicked(){
  //this slot is called when delete all button pressed

  posList->clear();
  positionVector.clear();

  vector<string> vstr;
  vstr.push_back("clear");//delete all trajectories
  vstr.push_back("delete"); //delete all start positions
  contr->trajPos(vstr);

  emit updateTrajectories();
}

/*********************************************/

void TrajectoryDialog::startCalcButtonClicked(){
  //this slot is called when start calc. button pressed
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::startCalcButtonClicked");
#endif


  //ask for new fields
  vector<string> fields = contr->getTrajectoryFields();
  int nr_fields=fields.size();
  std::string fName;
  //using first field if there is any field
  if(nr_fields > 0){
    fieldName->setText(QString(fields[0].c_str()));
    fName = fields[0];
  } else {
    fieldName->setText(tr("No field selected"));
    fName ="No field selected";
  }
  //send field name to TrajectoryPlot
  std::string str = " field=\"";
  str+= fName;
  str+= "\"";
  vector<string> vstr;
  vstr.push_back("clear");
  vstr.push_back(str);
  contr->trajPos(vstr);

  emit updateTrajectories();

  contr->startTrajectoryComputation();
}
/*********************************************/

void TrajectoryDialog::quitClicked(){

  vector<string> vstr;
  vstr.push_back("quit");
  contr->trajPos(vstr);
  //   posList->clear();
  emit markPos(false);
  emit TrajHide();
}
/*********************************************/

void TrajectoryDialog::helpClicked(){
  emit showsource("ug_trajectories.html");
}

/*********************************************/

void TrajectoryDialog::printClicked(){
  contr->printTrajectoryPositions("trajectory.txt");
}

/*********************************************/

void TrajectoryDialog::applyhideClicked(){
  emit TrajHide();
}

/*********************************************/

void TrajectoryDialog::timeSpinSlot( int i) {

  ostringstream ss;
  ss << "timemarker=" << timeSpin->value();
  std::string str=ss.str();
  vector<string> vstr;
  vstr.push_back(str);
  contr->trajPos(vstr);
  emit updateTrajectories();

}

void TrajectoryDialog::colourSlot( int i) {

  std::string str;
  str= "colour=";
  str+= colourInfo[i].name;
  vector<string> vstr;
  vstr.push_back(str);
  contr->trajPos(vstr);
  emit updateTrajectories();
}
/*********************************************/

void TrajectoryDialog::lineWidthSlot( int i) {

  ostringstream ss;
  ss << "linewidth=" << i + 1;  // 1,2,3,...
  std::string str=ss.str();
  vector<string> vstr;
  vstr.push_back(str);
  contr->trajPos(vstr);
  emit updateTrajectories();
}

void TrajectoryDialog::lineTypeSlot( int i) {

  ostringstream ss;
  ss << "linetype=" << linetypes[i];
  std::string str=ss.str();
  vector<string> vstr;
  vstr.push_back(str);
  contr->trajPos(vstr);
  emit updateTrajectories();
}



std::string TrajectoryDialog::makeString()
{
  ostringstream ss;
  ss <<" radius="<<radiusSpin->value();
  ss <<" numpos="<<numposBox->currentText().toStdString();
  return ss.str();
}

/*********************************************/


void TrajectoryDialog::mapPos(float lat, float lon) {
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::mapPos");
#endif

  //Put this position in vector of positions
  posStruct pos;
  pos.lat=lat;
  pos.lon=lon;
  pos.radius=radiusSpin->value();
  pos.numPos=numposBox->currentText().toStdString();
  positionVector.push_back(pos);

  //Make string and insert in posList
  update_posList(lat,lon);

  //Make string and send to trajectoryPlot
  ostringstream str;
  str << setw(5) << setprecision(2)<< setiosflags(ios::fixed);
  str << "latitudelongitude=" << lat << "," << lon;
  str <<" radius="<<radiusSpin->value();
  str <<" numpos="<<pos.numPos;
  std::string posString = str.str();
  vector<string> vstr;
  vstr.push_back("clear");
  vstr.push_back(posString);
  contr->trajPos(vstr);
  emit updateTrajectories();


}
/*********************************************/

void TrajectoryDialog::update_posList(float lat, float lon) {
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::update_posList");
#endif

  //Make string and insert in posList

  ostringstream ost;

  int deg, min;

  min= int(fabsf(lat)*60.+0.5);
  deg= min/60;
  min= min%60;
  ost << setw(2) << deg << "\xB0" << setw(2) << setfill('0') << min;
  if (lat>=0.0)
    ost << "'N   ";
  else
    ost << "'S   ";

  min= int(fabsf(lon)*60.+0.5);
  deg= min/60;
  min= min%60;
  ost << setfill(' ')<< setw(3) << deg << "\xB0"
      << setw(2) << setfill('0') << min;
  if (lon>=0.0)
    ost << "'E";
  else
    ost << "'W";

  // qt4 fix: insertItem takes QString as argument
  posList->addItem(QString(ost.str().c_str()));

}
/*********************************************/

void TrajectoryDialog::sendAllPositions(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::sendAllPositions");
#endif

  vector<string> vstr;

  int npos=positionVector.size();
  for( int i=0; i<npos; i++){
    ostringstream str;
    str << setw(5) << setprecision(2)<< setiosflags(ios::fixed);
    str << "latitudelongitude=";
    str << positionVector[i].lat << "," << positionVector[i].lon;
    str <<" radius="<<positionVector[i].radius;
    str <<" numpos="<<positionVector[i].numPos;
    std::string posString = str.str();
    vstr.push_back(posString);
  }
  contr->trajPos(vstr);
  emit updateTrajectories();
}


void TrajectoryDialog::showplus(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("TrajectoryDialog::showplus");
#endif
  this->show();

  if(  posButton->isChecked() )
    emit markPos(true);

  ostringstream ss;

  ss <<"colour="<<colourInfo[colbox->currentIndex()].name;
  ss <<" linewidth="<< lineWidthBox->currentIndex() + 1;  // 1,2,3,...
  ss <<" linetype="<< linetypes[lineTypeBox->currentIndex()];  // 1,2,3,...
  ss <<" plot=on";

  std::string str= ss.str();

  vector<string> vstr;
  vstr.push_back(str);
  contr->trajPos(vstr);


  sendAllPositions();

  emit updateTrajectories();
}
/*********************************************/


vector<string> TrajectoryDialog::writeLog()
{
  vector<string> vstr;

  int n=positionVector.size();
  for(int i=0;i<n; i++){
    ostringstream ost;
    ost << "latitudelongitude=";
    ost << positionVector[i].lat << "," << positionVector[i].lon;
    ost <<" radius="<<positionVector[i].radius;
    ost <<" numpos="<<positionVector[i].numPos;
    std::string str = ost.str();
    vstr.push_back(str);
  }

  ostringstream ostr;
  if ( colbox->currentIndex() > -1 ) {
    ostr <<"colour="<<colourInfo[colbox->currentIndex()].name;
  }
  if ( lineWidthBox->currentIndex() > -1 ) {
    ostr <<" linewidth="<< lineWidthBox->currentIndex() + 1;  // 1,2,3,...
  }
  if ( lineTypeBox->currentIndex() > -1 ) {
    ostr <<" linetype="<< linetypes[lineTypeBox->currentIndex()];
  }
  std::string str = ostr.str() + makeString();
  vstr.push_back(str);
  vstr.push_back("================");
  return vstr;

}
/*********************************************/

void TrajectoryDialog::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion)
{
  int n=0, nvstr= vstr.size();
  int radius = 0;
  std::string numPos;

  while (n<nvstr && vstr[n].substr(0,4)!="====") {

    posStruct pos;
    bool position=false;
    vector<string> parts= miutil::split(vstr[n], 0, " ", true);

    int nr=parts.size();
    for( int i=0; i<nr; i++){
      vector<string> tokens = miutil::split(parts[i], 0, "=", true);
      if( tokens.size() == 2) {
	std::string key = miutil::to_lower(tokens[0]);
	std::string value = miutil::to_lower(tokens[1]);
	if (key == "colour" ){
	  int number= getIndex( colourInfo, value);
	  if (number>=0) {
	    colbox->setCurrentIndex(number);
	  }
	}else if (key == "line" || key == "linewidth"){
	  lineWidthBox->setCurrentIndex(atoi(value.c_str())-1);
	}else if (key == "linetype" ){
	  int j=0;
	  int nr_linetypes=linetypes.size();
	  while (j<nr_linetypes && value!=linetypes[j]) j++;
	  if (j==nr_linetypes) j=0;
	  lineTypeBox->setCurrentIndex(j);
	}else if (key == "radius" ){
	  radius=atoi(value.c_str());
	  pos.radius=radius;
	}else if (key == "numpos" ){
	  numPos=pos.numPos=value;
	}else if (key == "latitudelongitude" ){
	  vector<std::string> latlon = miutil::split(value, 0, ",");
	  int nr=latlon.size();
	  if(nr!=2) continue;
	  pos.lat=atof(latlon[0].c_str());
	  pos.lon=atof(latlon[1].c_str());
	  position=true;
	  update_posList(pos.lat,pos.lon);
	}
      }
    }
    if( position)
      positionVector.push_back(pos);
    n++;
  }

  radiusSpin->setValue(radius);
  if(radius>100)
    // qt4 fix: insertStrList() -> insertStringList()
    // (uneffective, have to make QStringList and QString!)
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);
  if(numPos == "5")
    numposBox->setCurrentIndex(1);
  else if(numPos == "1")
    numposBox->setCurrentIndex(0);
  else
    numposBox->setCurrentIndex(2);

  //positions are not sent to TrajectoryPlot yet

}


void TrajectoryDialog::closeEvent( QCloseEvent* e) {
  emit markPos(false);
  emit TrajHide();
}

