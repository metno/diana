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

#include "diUtilities.h"
#include "qtVprofWindow.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "diStationPlot.h"
#include "qtVprofWidget.h"
#include "qtVprofModelDialog.h"
#include "qtVprofSetupDialog.h"
#include "diVprofManager.h"
#include "qtPrintManager.h"

#include <puTools/miStringFunctions.h>

#include <QFileDialog>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qfont.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QPixmap>
#include <QSpinBox>

#include "forover.xpm"
#include "bakover.xpm"

#define MILOGGER_CATEGORY "diana.VprofWindow"
#include <miLogger/miLogging.h>

using namespace std;

VprofWindow::VprofWindow()
: QMainWindow( 0)
{
  vprofm = new VprofManager();

  setWindowTitle( tr("Diana Vertical Profiles") );

#if !defined(USE_PAINTGL)
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
#endif
  //central widget
#if !defined(USE_PAINTGL)
  vprofw= new VprofWidget(vprofm, fmt, this);
#else
  vprofw= new VprofWidget(vprofm, this);
#endif
  setCentralWidget(vprofw);

  connect(vprofw, SIGNAL(timeChanged(int)),SLOT(timeClicked(int)));
  connect(vprofw, SIGNAL(stationChanged(int)),SLOT(stationClicked(int)));

  // tool bar and buttons
  vpToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,vpToolbar);

  // tool bar for selecting time and station
  tsToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,tsToolbar);

  // button for modeldialog-starts new dialog
  modelButton = new ToggleButton(this, tr("Model"));
  connect( modelButton, SIGNAL( toggled(bool)), SLOT( modelClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this, tr("Settings"));
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
  QPushButton *leftStationButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftStationButton, SIGNAL(clicked()), SLOT(leftStationClicked()) );
  leftStationButton->setAutoRepeat(true);

  vector<std::string> stations;
  stations.push_back("                         ");
  stationBox = ComboBox( this, stations, true, 0);
  connect( stationBox, SIGNAL( activated(int) ), SLOT( stationBoxActivated(int) ) );

  QPushButton *rightStationButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightStationButton, SIGNAL(clicked()), SLOT(rightStationClicked()) );
  rightStationButton->setAutoRepeat(true);

  QPushButton *leftTimeButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftTimeButton, SIGNAL(clicked()), SLOT(leftTimeClicked()) );
  leftTimeButton->setAutoRepeat(true);

  //combobox to select time
  vector<std::string> times;
  times.push_back("2002-01-01 00");
  timeBox = ComboBox( this, times, true, 0);
  connect( timeBox, SIGNAL( activated(int) ), SLOT( timeBoxActivated(int) ) );

  QPushButton *rightTimeButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  timeSpinBox = new QSpinBox( this );
  timeSpinBox->setValue(0);

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
  tsToolbar->addWidget(timeSpinBox);

  //connected dialogboxes

  vpModelDialog = new VprofModelDialog(this,vprofm);
  connect(vpModelDialog, SIGNAL(ModelApply()),SLOT(changeModel()));
  connect(vpModelDialog, SIGNAL(ModelHide()),SLOT(hideModel()));
  connect(vpModelDialog, SIGNAL(showsource(const std::string, const std::string)),
      SIGNAL(showsource(const std::string, const std::string)));


  vpSetupDialog = new VprofSetupDialog(this,vprofm);
  connect(vpSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vpSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vpSetupDialog, SIGNAL(showsource(const std::string, const std::string)),
      SIGNAL(showsource(const std::string, const std::string)));

  //initialize everything in startUp
  active = false;
  mainWindowTime= miutil::miTime::nowTime();

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::VprofWindow() finished");
#endif
}


/***************************************************************************/

void VprofWindow::modelClicked( bool on ){
  //called when the model button is clicked
  if( on ){
    METLIBS_LOG_DEBUG("Model button clicked on");
    vpModelDialog->show();
  } else {
    METLIBS_LOG_DEBUG("Model button clicked off");
    vpModelDialog->hide();
  }
}

/***************************************************************************/

void VprofWindow::leftStationClicked(){
  stationClicked(-1);
}

void VprofWindow::rightStationClicked(){
  stationClicked(1);
}

void VprofWindow::stationClicked(int i){

  int index = stationBox->currentIndex() - i;
  if(index < 0) {
    //set index to the last in the box !
    index=stationBox->count()-1;
  } else if(index > stationBox->count()-1) {
    //set index to the first in the box !
    index = 0;
  }

  stationBox->setCurrentIndex(index);
  vprofm->setStation(stationBox->currentText().toStdString());
  vector<string> stations;
  stations.push_back(stationBox->currentText().toStdString());
  emit stationChanged(stations); //name of current stations (to mainWindow)

  vprofw->updateGL();

}


/***************************************************************************/

void VprofWindow::leftTimeClicked(){
  timeClicked(-1);
}

void VprofWindow::rightTimeClicked(){
  timeClicked(1);
}

void VprofWindow::timeClicked(int i){
  //called when the right Station button is clicked
  vprofm->setTime(timeSpinBox->value(),i);
  timeChanged();
  vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::timeChanged(){
  METLIBS_LOG_SCOPE();

  if (!timeBox->count()) return;

  miutil::miTime t = vprofm->getTime();

  std::string tstring=t.isoTime(false,false);
  std::string tbs=timeBox->currentText().toStdString();
  //search timeList
  int n = timeBox->count();
  for (int i = 0; i<n;i++){
    if(tstring ==timeBox->itemText(i).toStdString()){
      METLIBS_LOG_DEBUG(LOGVAL(timeBox->itemText(i).toStdString()));
      timeBox->setCurrentIndex(i);
      tbs=timeBox->currentText().toStdString();
      break;
    }
  }

  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit modelChanged();
    //update combobox lists of stations and time
    updateStationBox();
    //get correct selection in comboboxes
    stationChanged();
  }

  emit setTime("vprof",t);

}


/***************************************************************************/

void VprofWindow::stationChanged(){
  METLIBS_LOG_SCOPE();
  vprofw->updateGL();
  raise();

  //get current station
  vector<std::string> vs = vprofm->getStations();
  //if no current station, use last station plotted
  if (!vs.size()) {
    vs.push_back(vprofm->getLastStation());
  }
  int n = stationBox->count();
  for(int i = 0;i<n;i++){
    if (vs[0]==stationBox->itemText(i).toStdString()){
      stationBox->setCurrentIndex(i);
      break;
    }
  }
  emit stationChanged(vs); //name of current stations (to mainWindow)


}


/***************************************************************************/

void VprofWindow::printClicked()
{
  QPrinter qprt;

  printerManager pman;
  std::string command= pman.printCommand();

  fromPrintOption(qprt,priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    if (!qprt.outputFileName().isNull()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else {
      priop.fname= "prt_" + miutil::miTime::nowTime().isoTime() + ".ps";
      miutil::replace(priop.fname, ' ','_');
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // set printername
    if (qprt.outputFileName().isNull())
      priop.printer= qprt.printerName().toStdString();

    // start the postscript production
    diutil::OverrideCursor waitCursor;

    vprofm->startHardcopy(priop);
    vprofw->updateGL();
    vprofm->endHardcopy();
    vprofw->updateGL();

    // if output to printer: call appropriate command
    if (qprt.outputFileName().isNull()){
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      int res = system(command.c_str());

      if (res != 0){
        METLIBS_LOG_WARN("Print command:" << command << " failed");
      }

    }
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
    std::string filename= s.toStdString();
    std::string format= "PNG";
    int quality= -1; // default quality

    // find format
    if (miutil::contains(filename, ".xpm") || miutil::contains(filename, ".XPM"))
      format= "XPM";
    else if (miutil::contains(filename, ".bmp") || miutil::contains(filename, ".BMP"))
      format= "BMP";
    else if (miutil::contains(filename, ".eps") || miutil::contains(filename, ".epsf")){
      // make encapsulated postscript
      // NB: not screendump!
      makeEPS(filename);
      return;
    }

    // do the save
    vprofw->saveRasterImage(filename, format, quality);
  }
}


void VprofWindow::makeEPS(const std::string& filename)
{
  diutil::OverrideCursor waitCursor;
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
}

/***************************************************************************/

void VprofWindow::setupClicked(bool on){
  //called when the setup button is clicked
  if( on ){
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Setup button clicked on");
#endif
    vpSetupDialog->start();
    vpSetupDialog->show();

  } else {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Setup button clicked off");
#endif
    vpSetupDialog->hide();
  }
}


/***************************************************************************/

void VprofWindow::quitClicked(){
  //called when the quit button is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("quit clicked");
#endif
  //for now, only hide window, not really quit !
  vpToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);
  // NOTE: flush the fieldCache...
  vprofm->cleanup();
  
  active = false;
  emit VprofHide();
  vector<miutil::miTime> t;
  emit emitTimes("vprof",t);
}


/***************************************************************************/

void VprofWindow::hideClicked(){
  //called when the hide button in Vprofwindow is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("hide clicked");
#endif
}

/***************************************************************************/

void VprofWindow::updateClicked(){
  //called when the update button in Vprofwindow is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("update clicked");
#endif
  vprofm->updateObs();      // check obs.files
  miutil::miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}

/***************************************************************************/

void VprofWindow::helpClicked(){
  //called when the help button in Vprofwindow is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("help clicked");
#endif
  emit showsource("ug_verticalprofiles.html");
}


/***************************************************************************/

void VprofWindow::MenuOK(){
  //obsolete - nothing happens here
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::MenuOK()");
#endif
}


/***************************************************************************/

void VprofWindow::changeModel(){
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::changeModel()");
#endif
  { diutil::OverrideCursor waitCursor;
    vprofm->setModel();
  }

  onlyObs= vprofm->onlyObsState();

  //emit to main Window (updates stationPlot)
  emit modelChanged();
  //update combobox lists of stations and time
  updateStationBox();
  updateTimeBox();
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::changeSetup(){
  //called when the apply from setup dialog is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::changeSetup()");
#endif
  vprofw->updateGL();
}



/***************************************************************************/

void VprofWindow::hideModel(){
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::hideModel()");
#endif
  vpModelDialog->hide();
  modelButton->setChecked(false);
}

/***************************************************************************/
void VprofWindow::hideSetup(){
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::hideSetup()");
#endif
  vpSetupDialog->hide();
  setupButton->setChecked(false);
}

/***************************************************************************/

StationPlot* VprofWindow::getStations(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::getStations()");
#endif
  const vector <std::string> stations = vprofm->getStationList();
  const vector <float> latitude = vprofm->getLatitudes();
  const vector <float> longitude = vprofm->getLongitudes();
  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  std::string ann = vprofm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);

  // ADC set plotname (for StatusPlotButtons)
  stationPlot->setPlotName(ann);

  //the coordinates are defined here
#ifdef DEBUGPRINT
  //  for (int i = 0; i<n;i++){
  //     METLIBS_LOG_DEBUG("Station number " << i << " name = " << stations[i]
  // 	 << " latitude = " << latitude[i]
  // 	 << " longitude = " << longitude[i]);
  //   }
#endif

  return stationPlot;
}

/***************************************************************************/

void VprofWindow::updateStationBox(){
  //update list of stations in stationBox
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::updateStationBox");
#endif

  stationBox->clear();
  vector<std::string> stations= vprofm->getStationList();

  std::sort(stations.begin(),stations.end());

  //add dummy to make stationBox wide enough
  stations.push_back("                        ");

  int n =stations.size();
  for (int i=0; i<n; i++){
    stationBox->addItem((stations[i].c_str()));
  }
}


/***************************************************************************/

void VprofWindow::updateTimeBox(){
  //update list of times
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofWindow::updateTimeBox");
#endif

  timeBox->clear();
  vector<miutil::miTime> times= vprofm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).c_str()));
  }

  emit emitTimes("vprof",times);
}

/***************************************************************************/

void VprofWindow::stationBoxActivated(int index){

  std::string sbs=stationBox->currentText().toStdString();
  vprofm->setStation(sbs);
  vprofw->updateGL();
  vector<string> stations;
  stations.push_back(sbs);
  emit stationChanged(stations); //name of current station (to mainWindow)

}

/***************************************************************************/

void VprofWindow::timeBoxActivated(int index){

  vector<miutil::miTime> times= vprofm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    vprofm->setTime(times[index]);

    if (onlyObs) {
      //emit to main Window (updates stationPlot)
      emit modelChanged();
      //update combobox lists of stations and time
      updateStationBox();
      //get correct selection in comboboxes
      stationChanged();
    }

    vprofw->updateGL();
  }
}

/***************************************************************************/


void VprofWindow::changeStation(const string& station)
{
  METLIBS_LOG_SCOPE(LOGVAL(station));
  vprofm->setStation(station);
  stationChanged();
}

void VprofWindow::changeStations(const std::vector<string>& stations)
{
  METLIBS_LOG_SCOPE(LOGVAL(stations.size()));
  vprofm->setStations(stations);

  stationChanged();
}


/***************************************************************************/

void VprofWindow::mainWindowTimeChanged(const miutil::miTime& t){

  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active) return;

  METLIBS_LOG_DEBUG("vprofWindow::mainWindowTimeChanged called with time " << t);
  vprofm->mainWindowTimeChanged(t);
  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit modelChanged();
    //update combobox lists of stations and time
    updateStationBox();
    updateTimeBox();
  }
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  vprofw->updateGL();
}


/***************************************************************************/

void VprofWindow::startUp(const miutil::miTime& t){
  METLIBS_LOG_INFO("vprofWindow::startUp called with time " << t);
  active = true;
  vpToolbar->show();
  tsToolbar->show();
  vpModelDialog->updateModelfileList();
  vprofw->updateGL();

  changeModel();
  mainWindowTimeChanged(t);
}

/***************************************************************************/

void VprofWindow::parseSetup()
{

  vprofm->parseSetup();
  vpModelDialog->updateModelfileList();

}

vector<string> VprofWindow::writeLog(const string& logpart)
{
  vector<string> vstr;
  std::string str;

  if (logpart=="window") {

    str= "VprofWindow.size " + miutil::from_number(this->width()) + " " + miutil::from_number(this->height());
    vstr.push_back(str);
    str= "VprofWindow.pos "  + miutil::from_number(this->x()) + " " + miutil::from_number(this->y());
    vstr.push_back(str);
    str= "VprofModelDialog.pos " + miutil::from_number(vpModelDialog->x()) + " " + miutil::from_number(vpModelDialog->y());
    vstr.push_back(str);
    str= "VprofSetupDialog.pos " + miutil::from_number(vpSetupDialog->x()) + " " + miutil::from_number(vpSetupDialog->y());
    vstr.push_back(str);

    // printer name & options...
    if (not priop.printer.empty()){
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


void VprofWindow::readLog(const string& logpart, const vector<string>& vstr,
    const string& thisVersion, const string& logVersion,
    int displayWidth, int displayHeight)
{
  if (logpart=="window") {

    int n= vstr.size();

    for (int i=0; i<n; i++) {
      vector<string> tokens= miutil::split(vstr[i], 0, " ");

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

void VprofWindow::closeEvent(QCloseEvent * e) {
  quitClicked();
}
