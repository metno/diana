/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: qtTrajectoryDialog.cc 1008 2009-01-20 06:59:41Z johan.karlsteen@smhi.se $

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

#include <qtUtility.h>
#include <qtGeoPosLineEdit.h>
#include <qtRadarEchoDialog.h>

#include <math.h>
#include <sstream>

#include <QString>

RadarEchoDialog::RadarEchoDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), contr(llctrl)
{
#ifdef DEBUGPRINT
  cout<<"RadarEchoDialog::RadarEchoDialog called"<<endl;
#endif

  //caption to appear on top of dialog
  setWindowTitle(tr("Radar echo"));
  this->setFocusPolicy(Qt::StrongFocus);
   

  
  
    //Positions
  posButton = new QCheckBox(tr("Select positions on map"),this);
  connect( posButton, SIGNAL( toggled(bool)), SLOT( posButtonToggled(bool) ) );
  posButton->setChecked(true);
  posButton->hide();
  
  QLabel* posLabel = new QLabel( tr("Write positions (Lat Lon):"), this );
  /*edit = new GeoPosLineEdit(this);
  QToolTip::add( edit, 
		 tr("Lat Lon (deg:min:sec or decimal)") );
  connect( edit, SIGNAL( returnPressed()), SLOT( editDone() ));*/  
  posLabel->hide();
  
  posList = new QListWidget(this);
  connect( posList, SIGNAL( itemClicked( QListWidgetItem * ) ),   
	   SLOT( posListSlot( ) ) );
  posList->hide();
  
  //push button to delete last pos
  deleteButton = new QPushButton(tr("Clear"), this );
  connect( deleteButton, SIGNAL(clicked()),SLOT(deleteClicked()));
  
  //push button to start calculation
  startCalcButton = new QPushButton( tr("Calculate"), this );
  connect(startCalcButton, SIGNAL(clicked()), SLOT( startCalcButtonClicked()));
  startCalcButton->hide();
  
  fieldName = new QLabel(this);
  fieldName->setAlignment(Qt::AlignCenter);
  
  //push button to show help
  QPushButton * Help = NormalPushButton( tr("Help"), this);  
  connect(  Help, SIGNAL(clicked()), SLOT( helpClicked()));    
  Help->hide();
  
  //push button to print pos
  QPushButton * print = NormalPushButton( tr("Print"), this);  
  QToolTip::add( print, 
		 tr("Print calc. positions to file: trajectory.txt") );
  connect(  print, SIGNAL(clicked()), SLOT( printClicked())); 
  print->hide();
  
  //push button to hide dialog
  QPushButton * Hide = NormalPushButton(tr("Hide"), this);
  connect( Hide, SIGNAL(clicked()), SIGNAL(RadeHide()));
  
  //push button to quit, deletes all trajectoryPlot objects
  QPushButton * quit = NormalPushButton( tr("Quit"), this);
  connect(quit, SIGNAL(clicked()), SLOT( quitClicked()) );
  
  QFrame *line1 = new QFrame( this );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  QFrame *line2 = new QFrame( this );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  QFrame *line3 = new QFrame( this );
  line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  
  startlabel1= new QLabel("Start:",this);
  startlabel1->setFrameStyle( QFrame::Panel);
  startlabel1->setMinimumSize(startlabel1->sizeHint());
  
  
  latlabel1= new QLabel("Lat:",this);
  latlabel1->setFrameStyle( QFrame::Panel);
  latlabel1->setMinimumSize(latlabel1->sizeHint());
  
  latbox1= new QLabel("00°00'N",this);
  latbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  latbox1->setMinimumSize(latbox1->sizeHint());

  lonlabel1= new QLabel("Lon:",this);
  lonlabel1->setFrameStyle( QFrame::Panel);
  lonlabel1->setMinimumSize(lonlabel1->sizeHint());  
  
  lonbox1= new QLabel("00°00'W",this);
  lonbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  lonbox1->setMinimumSize(lonbox1->sizeHint());  


  datelabel1= new QLabel("Date:",this);
  datelabel1->setFrameStyle( QFrame::Panel);
  datelabel1->setMinimumSize(datelabel1->sizeHint());  
  
  datebox1= new QLabel("0000-00-00",this);
  datebox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  datebox1->setMinimumSize(datebox1->sizeHint());  
  
  timelabel1= new QLabel("Time:",this);
  timelabel1->setFrameStyle( QFrame::Panel);
  timelabel1->setMinimumSize(timelabel1->sizeHint());  
  
  timebox1= new QLabel("00:00",this);
  timebox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timebox1->setMinimumSize(timebox1->sizeHint());  

  endlabel1= new QLabel("End:",this);
  endlabel1->setFrameStyle( QFrame::Panel);
  endlabel1->setMinimumSize(endlabel1->sizeHint());
  
  
  latlabel2= new QLabel("Lat:",this);
  latlabel2->setFrameStyle( QFrame::Panel);
  latlabel2->setMinimumSize(latlabel2->sizeHint());
    
  latbox2= new QLabel("00°00'N",this);
  latbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  latbox2->setMinimumSize(latbox2->sizeHint());

  lonlabel2= new QLabel("Lon:",this);
  lonlabel2->setFrameStyle( QFrame::Panel);
  lonlabel2->setMinimumSize(lonlabel2->sizeHint());   
  
  lonbox2= new QLabel("00°00'W",this);
  lonbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  lonbox2->setMinimumSize(lonbox2->sizeHint());  

  datelabel2= new QLabel("Date:",this);
  datelabel2->setFrameStyle( QFrame::Panel);
  datelabel2->setMinimumSize(datelabel2->sizeHint());  
  
  datebox2= new QLabel("0000-00-00",this);
  datebox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  datebox2->setMinimumSize(datebox2->sizeHint());  
  
  timelabel2= new QLabel("Time:",this);
  timelabel2->setFrameStyle( QFrame::Panel);
  timelabel2->setMinimumSize(timelabel2->sizeHint());  
  
  timebox2= new QLabel("00:00",this);
  timebox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timebox2->setMinimumSize(timebox2->sizeHint());  
    
  speedlabel1= new QLabel("Velocity:",this);
  speedlabel1->setFrameStyle( QFrame::Panel);
  speedlabel1->setMinimumSize(speedlabel1->sizeHint());    

  speedlabel2= new QLabel("Velocity:",this);
  speedlabel2->setFrameStyle( QFrame::Panel);
  speedlabel2->setMinimumSize(speedlabel2->sizeHint());    

  speedlabel3= new QLabel("Velocity:",this);
  speedlabel3->setFrameStyle( QFrame::Panel);
  speedlabel3->setMinimumSize(speedlabel3->sizeHint());      
  
  speedbox1= new QLabel("0 m/s",this);
  speedbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox1->setMinimumSize(speedbox1->sizeHint());

  speedbox2= new QLabel("0 m/s",this);
  speedbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox2->setMinimumSize(speedbox2->sizeHint());

  speedbox3= new QLabel("0 m/s",this);
  speedbox3->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox3->setMinimumSize(speedbox3->sizeHint());
    
  distancelabel= new QLabel("Distance:",this);
  distancelabel->setFrameStyle( QFrame::Panel);
  distancelabel->setMinimumSize(distancelabel->sizeHint());    
  
  distancebox= new QLabel("0 km",this);
  distancebox->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  distancebox->setMinimumSize(distancebox->sizeHint());
    
  //now create a grid layout 
  QGridLayout* gridlayout = new QGridLayout(this);
  gridlayout->setColumnStretch(1,1);
        
  gridlayout->addWidget(startlabel1,          0,0);
  
  gridlayout->addWidget(latlabel1,          1,0);
  gridlayout->addWidget(latbox1,            1,1);
  gridlayout->addWidget(lonlabel1,          2,0);
  gridlayout->addWidget(lonbox1,            2,1);
  gridlayout->addWidget(datelabel1,         3,0);
  gridlayout->addWidget(datebox1,           3,1);
  gridlayout->addWidget(timelabel1,         4,0);
  gridlayout->addWidget(timebox1,           4,1);  
  gridlayout->addWidget(endlabel1,          0,2);  
  gridlayout->addWidget(latlabel2,          1,2);
  gridlayout->addWidget(latbox2,            1,3);
  gridlayout->addWidget(lonlabel2,          2,2);
  gridlayout->addWidget(lonbox2,            2,3);  
  gridlayout->addWidget(datelabel2,         3,2);
  gridlayout->addWidget(datebox2,           3,3);
  gridlayout->addWidget(timelabel2,         4,2);
  gridlayout->addWidget(timebox2,           4,3);  
  gridlayout->addWidget(line1,              5,0,1,4);  
  gridlayout->addWidget(deleteButton,       6,0,1,4);//6,0,1,2);
  gridlayout->addWidget(startCalcButton,    6,2,1,2);  
  gridlayout->addWidget(line2,              7,0,1,4);  
  gridlayout->addWidget(speedlabel1,        8,0,1,2);
  gridlayout->addWidget(speedbox1,          8,2,1,2);
  gridlayout->addWidget(speedlabel2,        9,0,1,2);
  gridlayout->addWidget(speedbox2,          9,2,1,2);
  gridlayout->addWidget(speedlabel3,        10,0,1,2);
  gridlayout->addWidget(speedbox3,          10,2,1,2);
  gridlayout->addWidget(distancelabel,      11,0,1,2);
  gridlayout->addWidget(distancebox,        11,2,1,2);  
  gridlayout->addWidget(line3,              12,0,1,4);  
  gridlayout->addWidget( Help,              14,0,1,2);
  gridlayout->addWidget( print,          14,2,1,2);
  gridlayout->addWidget( Hide,           15,0,1,2);
  gridlayout->addWidget( quit,           15,2,1,2);   
}
/********************************************************/  


void RadarEchoDialog::posButtonToggled(bool b){
#ifdef DEBUGPRINT
  cout<<"TrajectoryDialog::posButtonToggled  b="<<b<<endl;
#endif
  //called when "Select positions om map" is clicked

  emit markRadePos(b);
  vector<miString> vstr;
  vstr.push_back("clear"); //delete all trajectories
  contr->radePos(vstr);


}


void RadarEchoDialog::editDone(){
  //this slot is called when return is pressed in the line edit

  vector<miString> vstr;
  vstr.push_back("clear"); //delete all trajectories
  contr->radePos(vstr);

  float lat=0,lon=0;
  if(!edit->getValues(lat,lon))
    cerr<<"getValues returned false"<<endl;

  mapPos(lat,lon);
  edit->clear();
}


/*********************************************/

void RadarEchoDialog::deleteClicked(){
  //delete selected group of start positions

  positionVector.clear();
    
  latbox1->setText("00°00'N");  
  lonbox1->setText("00°00'W");
  latbox2->setText("00°00'N");
  lonbox2->setText("00°00'W");
  datebox1->setText("0000-00-00");
  datebox2->setText("0000-00-00");
  timebox1->setText("00:00");
  timebox2->setText("00:00");
  speedbox1->setText("0 m/s");
  speedbox2->setText("0 km/h");
  speedbox3->setText("0 knots");
  distancebox->setText("0 km");
  
}

/*********************************************/
void RadarEchoDialog::deleteAllClicked(){
  //this slot is called when delete all button pressed

  posList->clear();
  positionVector.clear();

  vector<miString> vstr;
  vstr.push_back("clear");//delete all trajectories
  vstr.push_back("delete"); //delete all start positions
  contr->radePos(vstr); 

  emit updateRadarEchos();
}

/*********************************************/

static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
static const double EARTH_RADIUS_IN_METERS = 6372797.560856;

double RadarEchoDialog::ArcInRadians(double lat1, double lon1, double lat2, double lon2) {
    double latitudeArc  = (lat1 - lat2) * DEG_TO_RAD;
    double longitudeArc = (lon1 - lon2) * DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    latitudeH *= latitudeH;
    double lontitudeH = sin(longitudeArc * 0.5);
    lontitudeH *= lontitudeH;
    double tmp = cos(lat1*DEG_TO_RAD) * cos(lat2*DEG_TO_RAD);
    return 2.0 * asin(sqrt(latitudeH + tmp*lontitudeH));
}

double RadarEchoDialog::DistanceInMeters(double lat1, double lon1, double lat2, double lon2) {
    return EARTH_RADIUS_IN_METERS*ArcInRadians(lat1, lon1, lat2, lon2);
}

void RadarEchoDialog::startCalcButtonClicked(){
  
  calculateVelocity(); 
}


void RadarEchoDialog::calculateVelocity() {
  //this slot is called when start calc. button pressed
#ifdef DEBUGPRINT
  cout<<"TrajectoryDialog::startCalcButtonClicked"<<endl;
#endif

    
  speedbox1->clear();
  speedbox2->clear();
  speedbox3->clear();
  distancebox->clear();

  if (positionVector.size() == 2) {        
    double lat1 = positionVector[0].lat;
    double lat2 = positionVector[1].lat;
    double lon1 = positionVector[0].lon;
    double lon2 = positionVector[1].lon;
    miTime time1 = positionVector[0].time;
    miTime time2 = positionVector[1].time;
        
    double d = DistanceInMeters(lat1, lon1, lat2, lon2);
    int t = abs(miutil::miTime::secDiff(time1, time2)); 
            
    QString speedresult1, speedresult2, speedresult3, distanceresult;
    
    speedresult1.sprintf("%.2f m/s", (float)d/t);
    speedresult2.sprintf("%.2f km/h", (float)(d/t)*3.6);
    speedresult3.sprintf("%.2f knots", (float)((d/t)*3.6)/1.852);
    
    distanceresult.sprintf("%.2f km", (float)d/1000);
           
    speedbox1->setText(speedresult1);
    speedbox2->setText(speedresult2);
    speedbox3->setText(speedresult3);
    distancebox->setText(distanceresult);    
  }
  
 
  //ask for new fields
  vector<miString> fields = contr->getRadarEchoFields();
  int nr_fields=fields.size();
  miString fName;
    
  //send field name to TrajectoryPlot
  miString str = " field=\"";
  str+= fName;
  str+= "\"";
  vector<miString> vstr;
  vstr.push_back("clear");
  vstr.push_back(str);
  contr->radePos(vstr);

  emit updateRadarEchos();

  //contr->startRadarEchoComputation();
  
    
}


/*********************************************/

void RadarEchoDialog::quitClicked(){

  vector<miString> vstr;
  vstr.push_back("quit");
  contr->radePos(vstr);
  //   posList->clear();
  emit markRadePos(false);
  emit RadeHide();
}
/*********************************************/

void RadarEchoDialog::helpClicked(){
  emit showsource("ug_trajectories.html");
}

/*********************************************/

void RadarEchoDialog::printClicked(){
  contr->printTrajectoryPositions("trajectory.txt");
}

/*********************************************/

void RadarEchoDialog::applyhideClicked(){
  emit RadeHide();
}

/*********************************************/

miString RadarEchoDialog::makeString() {

  miString str;


  ostringstream ss;
  //ss <<" radius="<<radiusSpin->value();
  //ss <<" numpos="<<numposBox->currentText().toStdString();
  str+= ss.str();

  return str;
}

/*********************************************/


void RadarEchoDialog::mapPos(float lat, float lon) {
#ifdef DEBUGPRINT
  cout<<"RadarEchoDialog::mapPos"<<endl;
#endif

  //Put this position in vector of positions
  posStruct pos;
  pos.lat=lat;
  pos.lon=lon;
  
  miTime t;
  contr->getPlotTime(t);        
  pos.time = t;
  
  
  //pos.radius=radiusSpin->value();
  //pos.numPos=numposBox->currentText().toStdString();
  if (positionVector.size() < 3) {
    positionVector.push_back(pos);
  }
  
  //Make string and insert in posList
  if ((positionVector.size() == 1) || (positionVector.size() == 2)) {    
    update_posList(lat,lon,t, positionVector.size());
  }

  
  //Make string and send to trajectoryPlot
  ostringstream str;
  str << setw(5) << setprecision(2)<< setiosflags(ios::fixed);
  //str << "latitudelongitude=" << lat << "," << lon;
  str << "latitudelongitude=" << lat << "," << lon;
  //str <<" radius="<<radiusSpin->value();
  //str <<" numpos="<<pos.numPos;
  str <<" numpos="<<1;
  str <<" time="<<pos.time;
    
  miString posString = str.str();
  vector<miString> vstr;
  vstr.push_back("clear");
  vstr.push_back(posString);
  
  contr->radePos(vstr);
  
  emit updateRadarEchos();


}

/*********************************************/

void RadarEchoDialog::update_posList(float lat, float lon, miTime t, int index) {

#ifdef DEBUGPRINT
  cout<<"RadarEchoDialog::update_posList"<<endl;
#endif

  int latdeg, latmin, londeg, lonmin;
  miString latdir, londir; 
  QString latstr, lonstr, timestr, datestr;
  
  latmin= int(fabsf(lat)*60.+0.5);
  latdeg= latmin/60;
  latmin= latmin%60;
  
  if (lat>=0.0) {
    latdir = "N";
  } else {
    latdir = "S";
  }
  
  lonmin= int(fabsf(lon)*60.+0.5);
  londeg= lonmin/60;
  lonmin= lonmin%60;
  
  if (lon>=0.0) {
    londir = "E";
  } else {
    londir = "W";
  }  
  
  latstr.sprintf("%d°%d '%s", latdeg, latmin, latdir.cStr());
  lonstr.sprintf("%d°%d '%s", londeg, lonmin, londir.cStr());
  datestr.sprintf("%s", t.isoDate().cStr());
  timestr.sprintf("%s", t.isoClock().cStr());
  
  switch (index) {
    case 1:
      latbox1->setText(latstr);
      lonbox1->setText(lonstr);
      datebox1->setText(datestr);
      timebox1->setText(timestr);
      break;
    case 2:
      latbox2->setText(latstr);
      lonbox2->setText(lonstr);
      datebox2->setText(datestr);
      timebox2->setText(timestr);
      
      calculateVelocity();
      
      break;
  }
  
  //Make string and insert in posList

#ifdef DEBUGPRINT 
  cerr << "Make string for posList Insertion." << endl;
#endif
  
  ostringstream ost;

  int deg, min;
  
  min= int(fabsf(lat)*60.+0.5);
  deg= min/60;
  min= min%60;
  ost << setw(2) << deg << "°" << setw(2) << setfill('0') << min;
  if (lat>=0.0)
    ost << "'N   ";
  else
    ost << "'S   ";
  
  min= int(fabsf(lon)*60.+0.5);
  deg= min/60;
  min= min%60;
  ost << setfill(' ')<< setw(3) << deg << "°" 
      << setw(2) << setfill('0') << min;
  if (lon>=0.0)
    ost << "'E";
  else
    ost << "'W";
  
  // qt4 fix: insertItem takes QString as argument 
  posList->addItem(QString(ost.str().c_str()));  
}

/*********************************************/

void RadarEchoDialog::sendAllPositions(){
#ifdef DEBUGPRINT
  cout<<"RadarEchoDialog::sendAllPositions"<<endl;
#endif

  vector<miString> vstr;

  int npos=positionVector.size();
  for( int i=0; i<npos; i++){
    ostringstream str;
    str << setw(5) << setprecision(2)<< setiosflags(ios::fixed);
    str << "latitudelongitude=";
    str << positionVector[i].lat << "," << positionVector[i].lon;
    str <<" radius="<<positionVector[i].radius;
    str <<" numpos="<<positionVector[i].numPos;
    miString posString = str.str();
    vstr.push_back(posString);
  }
  contr->radePos(vstr);
  emit updateRadarEchos();
}


void RadarEchoDialog::showplus(){
#ifdef DEBUGPRINT
  cout<<"RadarEchoDialog::showplus"<<endl;
#endif
  this->show();

  if(  posButton->isOn() )
    emit markRadePos(true);

  ostringstream ss;

//  ss <<"colour="<<colourInfo[colbox->currentItem()].name;
//  ss <<" linewidth="<< lineWidthBox->currentItem() + 1;  // 1,2,3,...
//  ss <<" linetype="<< linetypes[lineTypeBox->currentItem()];  // 1,2,3,...

  ss <<"colour=name: black red: 000 green: 000 blue: 000 alpha: 255 Index: 52";
  ss <<" linewidth=2";  // 1,2,3,...
  ss <<" linetype=2";  // 1,2,3,...

  ss <<" plot=on";

  miString str= ss.str();

  vector<miString> vstr;
  vstr.push_back(str);
  contr->radePos(vstr);


  sendAllPositions();

  emit updateRadarEchos();
}
/*********************************************/


vector<miString> RadarEchoDialog::writeLog(){
  vector<miString> vstr;

  int n=positionVector.size();
  for(int i=0;i<n; i++){
    ostringstream ost;
    ost << "latitudelongitude=";
    ost << positionVector[i].lat << "," << positionVector[i].lon;
    ost <<" radius="<<positionVector[i].radius;
    ost <<" numpos="<<positionVector[i].numPos;
    miString str = ost.str();
    vstr.push_back(str);
  }
  ostringstream ostr;
  //ostr <<"colour="<<colourInfo[colbox->currentItem()].name;
  //ostr <<" linewidth="<< lineWidthBox->currentItem() + 1;  // 1,2,3,...
  //ostr <<" linetype="<< linetypes[lineTypeBox->currentItem()];
  miString str = ostr.str() + makeString();
  vstr.push_back(str);
  vstr.push_back("================");
  return vstr;
   
}
/*********************************************/

void RadarEchoDialog::readLog(const vector<miString>& vstr,
			       const miString& thisVersion,
			       const miString& logVersion){

  int n=0, nvstr= vstr.size();
  int radius;
  miString numPos;

  while (n<nvstr && vstr[n].substr(0,4)!="====") {

    posStruct pos;
    bool position=false;
    vector<miString> parts= vstr[n].split(' ',true);

    int nr=parts.size();
    for( int i=0; i<nr; i++){
      vector<miString> tokens= parts[i].split('=',true);
      if( tokens.size() == 2) {
	miString key = tokens[0].downcase();
	miString value = tokens[1].downcase();
	if (key == "radius" ){
	  radius=atoi(value.c_str());
	  pos.radius=radius;
	}else if (key == "numpos" ){
	  numPos=pos.numPos=value;
	}else if (key == "latitudelongitude" ){
	  vector<miString> latlon = value.split(',');
	  int nr=latlon.size();
	  if(nr!=2) continue;
	  pos.lat=atof(latlon[0].c_str());
	  pos.lon=atof(latlon[1].c_str());	  
	  position=true;
 	  
	  update_posList(pos.lat,pos.lon,pos.time, 1);
	}
      }
    }
    if( position)
      positionVector.push_back(pos);
    n++;
  }

  //radiusSpin->setValue(radius);
  //if(radius>100)
    // qt4 fix: insertStrList() -> insertStringList()
    // (uneffective, have to make QStringList and QString!)
    // Old line: radiusSpin->setSteps(50,50);
    //radiusSpin->setSingleStep(50);
  /*if(numPos == "5")
    numposBox->setCurrentItem(1);
  else if(numPos == "1")
    numposBox->setCurrentItem(0);
  else 
    numposBox->setCurrentItem(2);
  */
  //positions are not sent to TrajectoryPlot yet

}


bool RadarEchoDialog::close(bool alsoDelete){
  emit markRadePos(false);
  emit RadeHide();
  return true;
}

bool RadarEchoDialog::hasFocus() {
  emit markRadePos(true);
  return this->isActiveWindow();
}

void RadarEchoDialog::focusInEvent( QFocusEvent * )
{
  emit markRadePos(true);
}

void RadarEchoDialog::focusOutEvent( QFocusEvent * )
{
  emit markRadePos(false);
}

void RadarEchoDialog::focusChanged( QWidget * old, QWidget * now )
{
    QWidget* p = now;
    while(p)
    {
        if(p == this) 
        p = p->parentWidget();
    }
}




