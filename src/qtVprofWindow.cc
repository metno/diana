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
#include <QFileDialog>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtToggleButton.h>
#include <qlayout.h>
#include <qfont.h>
#include <qmotifstyle.h>
#include <qtUtility.h>
#include <qtVprofWindow.h>
//Added by qt3to4:
#include <QPixmap>
#include <diStationPlot.h>
#include <qtVprofWidget.h>
#include <qtVprofModelDialog.h>
#include <qtVprofSetupDialog.h>
#include <diVprofManager.h>
#include <qtPrintManager.h>
#include <forover.xpm>
#include <bakover.xpm>


VprofWindow::VprofWindow()
  : QMainWindow( 0)
{
#ifndef linux
  qApp->setStyle(new QMotifStyle);
#endif

  //HK ??? temporary create new VprofManager here
  vprofm = new VprofManager();

  setWindowTitle( tr("Diana Vertical Profiles") );

  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  //central widget
  vprofw= new VprofWidget(vprofm, fmt, this, "Vprof");
  setCentralWidget(vprofw);
  connect(vprofw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(vprofw, SIGNAL(stationChanged(int)),SLOT(stationChangedSlot(int)));


  // tool bar and buttons
  vpToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,vpToolbar);

  // tool bar for selecting time and station
  tsToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,tsToolbar);

  // button for modeldialog-starts new dialog
  modelButton = new ToggleButton(this,tr("Model").toStdString());
  connect( modelButton, SIGNAL( toggled(bool)), SLOT( modelClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this,tr("Settings").toStdString());
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for update
  QPushButton * updateButton = NormalPushButton( tr("Refresh"),this);
  connect( updateButton, SIGNAL(clicked()), SLOT(updateClicked()) );

  //button to print - starts print dialog
  QPushButton* printButton = NormalPushButton(tr("Print"),this);
  connect( printButton, SIGNAL(clicked()), SLOT( printClicked() ));

  //button to save - starts save dialog
  QPushButton* saveButton = NormalPushButton(tr("Save"),this);
  connect( saveButton, SIGNAL(clicked()), SLOT( saveClicked() ));

  //button for quit
  QPushButton * quitButton = NormalPushButton(tr("Quit"),this);
  connect( quitButton, SIGNAL(clicked()), SLOT(quitClicked()) );

  //button for help - pushbutton
  QPushButton * helpButton = NormalPushButton(tr("Help"),this);
  connect( helpButton, SIGNAL(clicked()), SLOT(helpClicked()) );


  //combobox to select station
  QToolButton *leftStationButton= new QToolButton(QPixmap(bakover_xpm),
						  tr("previous station"), "",
						  this, SLOT(leftStationClicked()),
						  this, "vpSstepB" );
  leftStationButton->setUsesBigPixmap(false);
  leftStationButton->setAutoRepeat(true);

  vector<miString> stations;
  stations.push_back("                         ");
  stationBox = ComboBox( this, stations, true, 0);
  connect( stationBox, SIGNAL( activated(int) ),
		       SLOT( stationBoxActivated(int) ) );

  QToolButton *rightStationButton= new QToolButton(QPixmap(forward_xpm),
						   tr("next station"), "",
						   this, SLOT(rightStationClicked()),
						   this, "vpSstepF" );
  rightStationButton->setUsesBigPixmap(false);
  rightStationButton->setAutoRepeat(true);

  QToolButton *leftTimeButton= new QToolButton(QPixmap(bakover_xpm),
					       tr("previous timestep"), "",
					       this, SLOT(leftTimeClicked()),
					       this, "vpTstepB" );
  leftTimeButton->setUsesBigPixmap(false);
  leftTimeButton->setAutoRepeat(true);

  //combobox to select time
  vector<miString> times;
  times.push_back("2002-01-01 00"); 
  timeBox = ComboBox( this, times, true, 0);
  connect( timeBox, SIGNAL( activated(int) ),
		    SLOT( timeBoxActivated(int) ) );

  QToolButton *rightTimeButton= new QToolButton(QPixmap(forward_xpm),
						tr("next timestep"), "",
						this, SLOT(rightTimeClicked()),
						this, "vpTstepF" );
  rightTimeButton->setUsesBigPixmap(false);
  rightTimeButton->setAutoRepeat(true);

  vpToolbar->addWidget(modelButton);
  vpToolbar->addWidget(setupButton);
  vpToolbar->addWidget(updateButton);
  vpToolbar->addWidget(printButton);
  vpToolbar->addWidget(saveButton);
  vpToolbar->addWidget(quitButton);
  vpToolbar->addWidget(helpButton);

  insertToolBarBreak(tsToolbar);

  tsToolbar->addWidget(leftStationButton);
  tsToolbar->addWidget(stationBox);
  tsToolbar->addWidget(rightStationButton);
  tsToolbar->addWidget(leftTimeButton);
  tsToolbar->addWidget(timeBox);
  tsToolbar->addWidget(rightTimeButton);

  //connected dialogboxes

  vpModelDialog = new VprofModelDialog(this,vprofm);
  connect(vpModelDialog, SIGNAL(ModelApply()),SLOT(changeModel()));
  connect(vpModelDialog, SIGNAL(ModelHide()),SLOT(hideModel()));
  connect(vpModelDialog, SIGNAL(showsource(const miString, const miString)),
	  SIGNAL(showsource(const miString, const miString)));


  vpSetupDialog = new VprofSetupDialog(this,vprofm);
  connect(vpSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vpSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vpSetupDialog, SIGNAL(showsource(const miString, const miString)),
	  SIGNAL(showsource(const miString, const miString)));

  //initialize everything in startUp
  firstTime = true;
  active = false;
  mainWindowTime= miTime::nowTime();

#ifdef DEBUGPRINT
  cerr<<"VprofWindow::VprofWindow() finished"<<endl;
#endif
}


/***************************************************************************/

void VprofWindow::modelClicked( bool on ){
  //called when the model button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Model button clicked on" << endl;
#endif
    vpModelDialog->show();
  } else {
#ifdef DEBUGPRINT
    cerr << "Model button clicked off" << endl;
#endif
    vpModelDialog->hide();
  }
}

/***************************************************************************/

void VprofWindow::leftStationClicked(){
  //called when the left Station button is clicked
  miString s= vprofm->setStation(-1);
  stationChangedSlot(-1);
  vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::rightStationClicked(){
  //called when the right Station button is clicked
  miString s= vprofm->setStation(+1);
  stationChangedSlot(+1);
  vprofw->updateGL();
}

/***************************************************************************/


void VprofWindow::leftTimeClicked(){
  //called when the left time button is clicked
  miTime t= vprofm->setTime(-1);
  //update combobox
  timeChangedSlot(-1);
  vprofw->updateGL();
}

/***************************************************************************/

void VprofWindow::rightTimeClicked(){
  //called when the right Station button is clicked
  miTime t= vprofm->setTime(+1);
  timeChangedSlot(+1);
  vprofw->updateGL();
}


/***************************************************************************/

bool VprofWindow::timeChangedSlot(int diff){
  //called if signal timeChanged is emitted from graphics
  //window (qtVprofWidget)
#ifdef DEBUGPRINT
  cerr << "timeChangedSlot(int) is called " << endl;
#endif
  int index=timeBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=timeBox->count()-1;
    } 
    timeBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > timeBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    timeBox->setCurrentItem(index);
    diff--;
  }
  miTime t = vprofm->getTime();
  miString tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  miString tbs=timeBox->currentText().toStdString();
  if (tbs!=tstring){
    //search timeList
    int n = timeBox->count();
    for (int i = 0; i<n;i++){      
      if(tstring ==timeBox->text(i).toStdString()){
	timeBox->setCurrentItem(i);
	tbs=timeBox->currentText().toStdString();
	break;
      }
    }
  }
  if (tbs!=tstring){
    cerr << "WARNING! timeChangedSlot  time from vprofm ="
	 << t    <<" not equal to timeBox text = " << tbs << endl
	 << "You should search through timelist!" << endl;
    return false;
  }

  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit modelChanged();
    //update combobox lists of stations and time
    updateStationBox();
    //get correct selection in comboboxes
    stationChangedSlot(0);
  }

  emit setTime("vprof",t);

  return true;
}


/***************************************************************************/

bool VprofWindow::stationChangedSlot(int diff){
#ifdef DEBUGPRINT
  cerr << "stationChangedSlot(int) is called " << endl;
#endif
  int index=stationBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=stationBox->count()-1;
    } 
    stationBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > stationBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    stationBox->setCurrentItem(index);
    diff--;
  }
  //get current station
  miString s = vprofm->getStation();
  //if (!stationBox->count()) return false;
  //if no current station, use last station plotted
  if (s.empty()) s = vprofm->getLastStation();
  miString sbs=stationBox->currentText().toStdString();
  if (sbs!=s){
    int n = stationBox->count();
    for(int i = 0;i<n;i++){
      if (s==stationBox->text(i).toStdString()){
	stationBox->setCurrentItem(i);
	sbs=miString(stationBox->currentText().toStdString());
	break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) { 
    emit stationChanged(sq); //name of current station (to mainWindow)      
    return true;
  } else {
    //    cerr << "WARNING! stationChangedSlot  station from vprofm ="
    // 	 << s    <<" not equal to stationBox text = " << sbs << endl;	
    //current or last station plotted is not in the list, insert it...
    stationBox->insertItem(sq,0);
    stationBox->setCurrentItem(0);
    return false;
  }
}


/***************************************************************************/

void VprofWindow::printClicked(){
  printerManager pman;
  //called when the print button is clicked
  miString command= pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  if (qprt.setup(this)){
    if (qprt.outputToFile()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else if (command.substr(0,4)=="lpr ") {
      priop.fname= "prt_" + miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
#ifdef linux
      command= "lpr -r " + command.substr(4,command.length()-4);
#else
      command= "lpr -r -s " + command.substr(4,command.length()-4);
#endif
    } else {
      priop.fname= "tmp_vprof.ps";
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // start the postscript production
    QApplication::setOverrideCursor( Qt::waitCursor );

    vprofm->startHardcopy(priop);
    vprofw->updateGL();
    vprofm->endHardcopy();
    vprofw->updateGL();

    // if output to printer: call appropriate command
    if (!qprt.outputToFile()){
      priop.numcopies= qprt.numCopies();
      
      // expand command-variables
      pman.expandCommand(command, priop);
      
      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();
  }
}

/***************************************************************************/

void VprofWindow::saveClicked()
{
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = QFileDialog::getSaveFileName(this,
				 tr("Save plot as image"),
				 fname,
				 tr("Images (*.png *.xpm *.bmp *.eps);;All (*.*)"));


  if (!s.isNull()) {// got a filename
    fname= s;
    miString filename= s.toStdString();
    miString format= "PNG";
    int quality= -1; // default quality

    // find format
    if (filename.contains(".xpm") || filename.contains(".XPM"))
      format= "XPM";
    else if (filename.contains(".bmp") || filename.contains(".BMP"))
      format= "BMP";
    else if (filename.contains(".eps") || filename.contains(".epsf")){
      // make encapsulated postscript
      // NB: not screendump!
      makeEPS(filename);
      return;
    }

    // do the save
    vprofw->saveRasterImage(filename, format, quality);
  }
}


void VprofWindow::makeEPS(const miString& filename)
{
  QApplication::setOverrideCursor( Qt::waitCursor );
  printOptions priop;
  priop.fname= filename;
  priop.colop= d_print::incolour;
  priop.orientation= d_print::ori_automatic;
  priop.pagesize= d_print::A4;
  priop.numcopies= 1;
  priop.usecustomsize= false;
  priop.fittopage= false;
  priop.drawbackground= true;
  priop.doEPS= true;

  vprofm->startHardcopy(priop);
  vprofw->updateGL();
  vprofm->endHardcopy();
  vprofw->updateGL();

  QApplication::restoreOverrideCursor();
}

/***************************************************************************/

void VprofWindow::setupClicked(bool on){
  //called when the setup button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Setup button clicked on" << endl;
#endif
    vpSetupDialog->start();
    vpSetupDialog->show();

  } else {
#ifdef DEBUGPRINT
    cerr << "Setup button clicked off" << endl;
#endif
    vpSetupDialog->hide();
  }
}


/***************************************************************************/

void VprofWindow::quitClicked(){
  //called when the quit button is clicked
#ifdef DEBUGPRINT
    cerr << "quit clicked" << endl;
#endif
    //for now, only hide window, not really quit !
    vpToolbar->hide();
    tsToolbar->hide();
    modelButton->setOn(false);
    setupButton->setOn(false);
    active = false;
    emit VprofHide();
    vector<miTime> t;
    emit emitTimes("vprof",t);
}


/***************************************************************************/

void VprofWindow::hideClicked(){
  //called when the hide button in Vprofwindow is clicked
#ifdef DEBUGPRINT
    cerr << "hide clicked" << endl;
#endif
}

/***************************************************************************/

void VprofWindow::updateClicked(){
  //called when the update button in Vprofwindow is clicked
#ifdef DEBUGPRINT
  cerr << "update clicked" << endl;
#endif
  vprofm->updateObs();      // check obs.files
  miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}

/***************************************************************************/

void VprofWindow::helpClicked(){
  //called when the help button in Vprofwindow is clicked
#ifdef DEBUGPRINT
    cerr << "help clicked" << endl;
#endif
    emit showsource("ug_verticalprofiles.html");
}


/***************************************************************************/

void VprofWindow::MenuOK(){
  //obsolete - nothing happens here 
#ifdef DEBUGPRINT
    cerr << "VprofWindow::MenuOK()" << endl;
#endif
}


/***************************************************************************/

void VprofWindow::changeModel(){
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
    cerr << "VprofWindow::changeModel()" << endl;
#endif
    vprofm->setModel();
    
    onlyObs= vprofm->onlyObsState();
    
    //emit to main Window (updates stationPlot)
    emit modelChanged();  
    //update combobox lists of stations and time
    updateStationBox();
    updateTimeBox();
    //get correct selection in comboboxes 
    stationChangedSlot(0);
    timeChangedSlot(0);
    vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::changeSetup(){
  //called when the apply from setup dialog is clicked
#ifdef DEBUGPRINT
    cerr << "VprofWindow::changeSetup()" << endl;
#endif
    vprofw->updateGL();
}



/***************************************************************************/

void VprofWindow::hideModel(){
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
    cerr << "VprofWindow::hideModel()" << endl;
#endif
    vpModelDialog->hide();
    modelButton->setOn(false);
}

/***************************************************************************/
void VprofWindow::hideSetup(){
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
    cerr << "VprofWindow::hideSetup()" << endl;
#endif
    vpSetupDialog->hide();
    setupButton->setOn(false);
}

/***************************************************************************/

StationPlot* VprofWindow::getStations(){
#ifdef DEBUGPRINT
  cerr <<"VprofWindow::getStations()" << endl;
#endif
  const vector <miString> stations = vprofm->getStationList();
  const vector <float> latitude = vprofm->getLatitudes();
  const vector <float> longitude = vprofm->getLongitudes();
  int n =stations.size();
  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  miString ann = vprofm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);

  // ADC set plotname (for StatusPlotButtons)
  stationPlot->setPlotName(ann);

  //the coordinates are defined here
#ifdef DEBUGPRINT
 //  for (int i = 0; i<n;i++){ 
//     cerr <<"Station number " << i << " name = " << stations[i] 
// 	 << " latitude = " << latitude[i]
// 	 << " longitude = " << longitude[i] << endl; 
//   }
#endif

  return stationPlot;
}

/***************************************************************************/

void VprofWindow::updateStationBox(){
  //update list of stations in stationBox
#ifdef DEBUGPRINT
  cerr << "VprofWindow::updateStationBox" << endl;
#endif

  stationBox->clear();
  vector<miString> stations= vprofm->getStationList();

  std::sort(stations.begin(),stations.end());

  //add dummy to make stationBox wide enough
  stations.push_back("                        "); 

  int n =stations.size();
  for (int i=0; i<n; i++){
    stationBox->addItem(QString(stations[i].c_str()));		
  }
}


/***************************************************************************/

void VprofWindow::updateTimeBox(){
  //update list of times
#ifdef DEBUGPRINT
  cerr << "VprofWindow::updateTimeBox" << endl;
#endif

  timeBox->clear();
  vector<miTime> times= vprofm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).cStr()));
  }		

  emit emitTimes("vprof",times);
}

/***************************************************************************/

void VprofWindow::stationBoxActivated(int index){


  //vector<miString> stations= vprofm->getStationList();
  miString sbs=stationBox->currentText().toStdString();
  //if (index>=0 && index<stations.size()) {
  vprofm->setStation(sbs);
  vprofw->updateGL();
  QString sq = sbs.c_str();
  emit stationChanged(sq); //name of current station (to mainWindow)      
  //}
}

/***************************************************************************/

void VprofWindow::timeBoxActivated(int index){

  vector<miTime> times= vprofm->getTimeList();

  if (index>=0 && index<times.size()) {
    vprofm->setTime(times[index]);

    if (onlyObs) {
      //emit to main Window (updates stationPlot)
      emit modelChanged();
      //update combobox lists of stations and time
      updateStationBox();
      //get correct selection in comboboxes
      stationChangedSlot(0);
    }

    vprofw->updateGL();
  }
}

/***************************************************************************/

bool VprofWindow::changeStation(const miString& station){
#ifdef DEBUGPRINT
  cerr << "VprofWindow::changeStation" << endl;
#endif
  vprofm->setStation(station); //HK ??? should check if station exists ?
  vprofw->updateGL();
  raise();
  if (stationChangedSlot(0))
    return true;
  else
    return false;
}


/***************************************************************************/

void VprofWindow::setFieldModels(const vector<miString>& fieldmodels){
  vprofm->setFieldModels(fieldmodels);
  if (active) changeModel();
  
}

/***************************************************************************/

void VprofWindow::mainWindowTimeChanged(const miTime& t){

  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active) return;
#ifdef DEBUGPRINT
  cerr << "vprofWindow::mainWindowTimeChanged called with time " << t << endl;
#endif
  vprofm->mainWindowTimeChanged(t);
  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit modelChanged();
    //update combobox lists of stations and time
    updateStationBox();
  }
  //get correct selection in comboboxes 
  stationChangedSlot(0);
  timeChangedSlot(0);
  vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::startUp(const miTime& t){
#ifdef DEBUGPRINT
  cerr << "vprofWindow::startUp called with time " << t << endl;
#endif
  active = true;
  vpToolbar->show();
  tsToolbar->show();
  //do something first time we start Vertical profiles
  if (firstTime){
    vector<miString> models;
    //define models for dialogs, comboboxes and stationplot
    vprofm->setSelectedModels(models,false,true,true,true);
    vpModelDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    vprofw->updateGL();
  }

  changeModel();
  mainWindowTimeChanged(t);
}

/***************************************************************************/

vector<miString> VprofWindow::writeLog(const miString& logpart)
{
  vector<miString> vstr;
  miString str;

  if (logpart=="window") {

    str= "VprofWindow.size " + miString(this->width()) + " "
			     + miString(this->height());
    vstr.push_back(str);
    str= "VprofWindow.pos "  + miString(this->x()) + " "
			     + miString(this->y());
    vstr.push_back(str);
    str= "VprofModelDialog.pos " + miString(vpModelDialog->x()) + " "
			         + miString(vpModelDialog->y());
    vstr.push_back(str);
    str= "VprofSetupDialog.pos " + miString(vpSetupDialog->x()) + " "
			         + miString(vpSetupDialog->y());
    vstr.push_back(str);

    // printer name & options...
    if (priop.printer.exists()){
      str= "PRINTER " + priop.printer;
      vstr.push_back(str);
      if (priop.orientation==d_print::ori_portrait)
	str= "PRINTORIENTATION portrait";
      else
	str= "PRINTORIENTATION landscape";
      vstr.push_back(str);
    }

  } else if (logpart=="setup") {

    vstr= vprofm->writeLog();

  }

  return vstr;
}


void VprofWindow::readLog(const miString& logpart, const vector<miString>& vstr,
			  const miString& thisVersion, const miString& logVersion,
			  int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<miString> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split(' ');

      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="VprofWindow.size")  this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="VprofWindow.pos")      this->move(x,y);
          else if (tokens[0]=="VprofModelDialog.pos") vpModelDialog->move(x,y);
          else if (tokens[0]=="VprofSetupDialog.pos") vpSetupDialog->move(x,y);
	}

      } else if (tokens.size()==2) {

        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
	  if (tokens[1]=="portrait")
	    priop.orientation=d_print::ori_portrait;
	  else
	    priop.orientation=d_print::ori_landscape;
        }

      }
    }

  } else if (logpart=="setup") {

    vprofm->readLog(vstr,thisVersion,logVersion);

  }
}

/***************************************************************************/

bool VprofWindow::close(bool alsoDelete){
  quitClicked();
  return true;
}
