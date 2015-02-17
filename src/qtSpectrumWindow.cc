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
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "qtSpectrumWindow.h"
#include "diStationPlot.h"
#include "qtSpectrumWidget.h"
#include "qtSpectrumModelDialog.h"
#include "qtSpectrumSetupDialog.h"
#include "diSpectrumManager.h"
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

#define MILOGGER_CATEGORY "diana.SpectrumWindow"
#include <miLogger/miLogging.h>

#include "forover.xpm"
#include "bakover.xpm"

using namespace std;

SpectrumWindow::SpectrumWindow()
: QMainWindow( 0)
{

  spectrumm = new SpectrumManager();

  setWindowTitle( tr("Diana Wavespectrum") );

#if !defined(USE_PAINTGL)
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
#endif
  //central widget
#if !defined(USE_PAINTGL)
  spectrumw= new SpectrumWidget(spectrumm, fmt, this);
#else
  spectrumw= new SpectrumWidget(spectrumm, this);
#endif
  setCentralWidget(spectrumw);
  connect(spectrumw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(spectrumw, SIGNAL(stationChanged(int)),SLOT(stationChangedSlot(int)));


  //tool bar and buttons
  spToolbar = new QToolBar(this);
  //tool bar for selecting time and station
  tsToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,spToolbar);
  addToolBar(Qt::TopToolBarArea,tsToolbar);



  //button for modeldialog-starts new dialog
  modelButton = new ToggleButton(this, tr("Model"));
  connect( modelButton, SIGNAL( toggled(bool)), SLOT( modelClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this, tr("Settings"));
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for update
  QPushButton * updateButton = NormalPushButton(tr("Refresh"),this);
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

  QPushButton * leftStationButton = new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftStationButton, SIGNAL(clicked()), SLOT(leftStationClicked()) );
  leftStationButton->setAutoRepeat(true);

  //combobox to select station
  vector<std::string> stations;
  stations.push_back("                        ");
  stationBox = ComboBox( this, stations, true, 0);
  connect( stationBox, SIGNAL( activated(int) ),
      SLOT( stationBoxActivated(int) ) );

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
  connect( timeBox, SIGNAL( activated(int) ),
      SLOT( timeBoxActivated(int) ) );

  QPushButton *rightTimeButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  spToolbar->addWidget(modelButton);
  spToolbar->addWidget(setupButton);
  spToolbar->addWidget(updateButton);
  spToolbar->addWidget(printButton);
  spToolbar->addWidget(saveButton);
  spToolbar->addWidget(quitButton);
  spToolbar->addWidget(helpButton);

  insertToolBarBreak(tsToolbar);

  tsToolbar->addWidget(leftStationButton);
  tsToolbar->addWidget(stationBox);
  tsToolbar->addWidget(rightStationButton);
  tsToolbar->addWidget(leftTimeButton);
  tsToolbar->addWidget(timeBox);
  tsToolbar->addWidget(rightTimeButton);

  //connected dialogboxes

  spModelDialog = new SpectrumModelDialog(this,spectrumm);
  connect(spModelDialog, SIGNAL(ModelApply()),SLOT(changeModel()));
  connect(spModelDialog, SIGNAL(ModelHide()),SLOT(hideModel()));
  connect(spModelDialog, SIGNAL(showsource(const std::string, const std::string)),
      SIGNAL(showsource(const std::string, const std::string)));


  spSetupDialog = new SpectrumSetupDialog(this,spectrumm);
  connect(spSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(spSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(spSetupDialog, SIGNAL(showsource(const std::string, const std::string)),
      SIGNAL(showsource(const std::string, const std::string)));


  //initialize everything in startUp
  firstTime = true;
  active = false;
  //mainWindowTime= miutil::miTime::nowTime();


  METLIBS_LOG_DEBUG("SpectrumWindow::SpectrumWindow() finished");

}


void SpectrumWindow::modelClicked( bool on )
{
  //called when the model button is clicked
  if( on ){

    METLIBS_LOG_DEBUG("Model button clicked on");

    spModelDialog->show();
  } else {

    METLIBS_LOG_DEBUG("Model button clicked off");

    spModelDialog->hide();
  }
}


void SpectrumWindow::leftStationClicked()
{
  //called when the left Station button is clicked
  std::string s= spectrumm->setStation(-1);
  stationChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightStationClicked()
{
  //called when the right Station button is clicked
  std::string s= spectrumm->setStation(+1);
  stationChangedSlot(+1);
  spectrumw->updateGL();
}


void SpectrumWindow::leftTimeClicked()
{
  //called when the left time button is clicked
  spectrumm->setTime(-1);
  timeChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightTimeClicked()
{
  //called when the right Station button is clicked
  spectrumm->setTime(+1);
  timeChangedSlot(+1);
  spectrumw->updateGL();
}


bool SpectrumWindow::timeChangedSlot(int diff)
{
  //called if signal timeChanged is emitted from graphics
  //window (qtSpectrumWidget)

  METLIBS_LOG_DEBUG("timeChangedSlot(int) is called ");

  int index=timeBox->currentIndex();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=timeBox->count()-1;
    }
    timeBox->setCurrentIndex(index);
    diff++;
  }
  while(diff>0){
    if(++index > timeBox->count()-1) {
      //set index to the first in the box !
      index=0;
    }
    timeBox->setCurrentIndex(index);
    diff--;
  }
  miutil::miTime t = spectrumm->getTime();
  std::string tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  std::string tbs=timeBox->currentText().toStdString();
  if (tbs!=tstring){
    //search timeList
    int n = timeBox->count();
    for (int i = 0; i<n;i++){
      if(tstring ==timeBox->itemText(i).toStdString()){
        timeBox->setCurrentIndex(i);
        tbs=timeBox->currentText().toStdString();
        break;
      }
    }
  }
  if (tbs!=tstring){
    METLIBS_LOG_WARN("WARNING! timeChangedSlot  time from spectrumm ="
    << t    <<" not equal to timeBox text = " << tbs);
    METLIBS_LOG_WARN("You should search through timelist!");
    return false;
  }

  emit setTime("spectrum",t);

  return true;
}


bool SpectrumWindow::stationChangedSlot(int diff)
{

  METLIBS_LOG_DEBUG("stationChangedSlot(int) is called ");

  int index=stationBox->currentIndex();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=stationBox->count()-1;
    }
    stationBox->setCurrentIndex(index);
    diff++;
  }
  while(diff>0){
    if(++index > stationBox->count()-1) {
      //set index to the first in the box !
      index=0;
    }
    stationBox->setCurrentIndex(index);
    diff--;
  }
  //get current station
  std::string s = spectrumm->getStation();
  //if (!stationBox->count()) return false;
  //if no current station, use last station plotted
  if (s.empty()) s = spectrumm->getLastStation();
  std::string sbs=stationBox->currentText().toStdString();
  if (sbs!=s){
    int n = stationBox->count();
    for(int i = 0;i<n;i++){
      if (s==stationBox->itemText(i).toStdString()){
        stationBox->setCurrentIndex(i);
        sbs=std::string(stationBox->currentText().toStdString());
        break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) {
    emit spectrumChanged(sq); //name of current station (to mainWindow)
    return true;
  } else {
    //    METLIBS_LOG_WARN("WARNING! stationChangedSlot  station from spectrumm ="
    // 	 << s    <<" not equal to stationBox text = " << sbs);
    //current or last station plotted is not in the list, insert it...
    stationBox->insertItem(0,sq);
    stationBox->setCurrentIndex(0);
    return false;
  }
}


void SpectrumWindow::printClicked()
{
  printerManager pman;
  std::string command= pman.printCommand();

  QPrinter qprt;
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

    spectrumm->startHardcopy(priop);
    spectrumw->updateGL();
    spectrumm->endHardcopy();
    spectrumw->updateGL();

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


void SpectrumWindow::saveClicked()
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
    spectrumw->saveRasterImage(filename, format, quality);
  }
}


void SpectrumWindow::makeEPS(const std::string& filename)
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

  spectrumm->startHardcopy(priop);
  spectrumw->updateGL();
  spectrumw->updateGL();
}


void SpectrumWindow::setupClicked(bool on)
{
  //called when the setup button is clicked
  if( on ){

    METLIBS_LOG_DEBUG("Setup button clicked on");

    spSetupDialog->start();
    spSetupDialog->show();

  } else {

    METLIBS_LOG_DEBUG("Setup button clicked off");

    spSetupDialog->hide();
  }
}


void SpectrumWindow::quitClicked()
{
  //called when the quit button is clicked

  METLIBS_LOG_DEBUG("quit clicked");

  //for now, only hide window, not really quit !
  spToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);

  /*****************************************************************
  // cleanup selections in dialog and data in memory
  spModelDialog->cleanup();
  spectrumm->cleanup();

  stationBox->clear();
  timeBox->clear();
   *****************************************************************/

  active = false;
  emit SpectrumHide();
  vector<miutil::miTime> t;
  emit emitTimes("spectrum",t);
}


void SpectrumWindow::updateClicked()
{
  //called when the update button in Spectrumwindow is clicked

  METLIBS_LOG_DEBUG("update clicked");

  miutil::miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}


void SpectrumWindow::helpClicked()
{
  //called when the help button in Spectrumwindow is clicked

  METLIBS_LOG_DEBUG("help clicked");

  emit showsource("ug_spectrum.html");
}


void SpectrumWindow::MenuOK()
{
  //obsolete - nothing happens here

  METLIBS_LOG_DEBUG("SpectrumWindow::MenuOK()");

}


void SpectrumWindow::changeModel()
{
  //called when the apply button from model dialog is clicked
  //... or field is changed ?

  METLIBS_LOG_DEBUG("SpectrumWindow::changeModel()");

  spectrumm->setModel();

  //emit to main Window (updates stationPlot)
  emit spectrumSetChanged();
  //update combobox lists of stations and time
  updateStationBox();
  updateTimeBox();
  //get correct selection in comboboxes
  stationChangedSlot(0);
  timeChangedSlot(0);
  spectrumw->updateGL();
}


void SpectrumWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked

  METLIBS_LOG_DEBUG("SpectrumWindow::changeSetup()");

  spectrumw->updateGL();
}


void SpectrumWindow::hideModel()
{
  //called when the hide button (from model dialog) is clicked

  METLIBS_LOG_DEBUG("SpectrumWindow::hideModel()");

  spModelDialog->hide();
  modelButton->setChecked(false);
}


void SpectrumWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked

  METLIBS_LOG_DEBUG("SpectrumWindow::hideSetup()");

  spSetupDialog->hide();
  setupButton->setChecked(false);
}


StationPlot* SpectrumWindow::getStations()
{

  METLIBS_LOG_DEBUG("SpectrumWindow::getStations()");

  const vector <std::string> stations = spectrumm->getStationList();
  const vector <float> latitude = spectrumm->getLatitudes();
  const vector <float> longitude = spectrumm->getLongitudes();
  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  std::string ann = spectrumm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);

  // ADC set plotname (for StatusPlotButtons)
  stationPlot->setPlotName(ann);

  //the coordinates are defined here

  //  for (int i = 0; i<n;i++){
  //     METLIBS_LOG_DEBUG("Station number " << i << " name = " << stations[i]
  // 	 << " latitude = " << latitude[i]
  // 	 << " longitude = " << longitude[i]);
  //   }


  return stationPlot;
}


void SpectrumWindow::updateStationBox()
{
  //update list of stations in stationBox

  METLIBS_LOG_DEBUG("SpectrumWindow::updateStationBox");


  stationBox->clear();
  vector<std::string> stations= spectrumm->getStationList();
  //add dummy to make stationBox wide enough
  stations.push_back("                        ");

  int n =stations.size();
  for (int i=0; i<n; i++){
    stationBox->addItem(QString(stations[i].c_str()));
  }

}


void SpectrumWindow::updateTimeBox()
{
  //update list of times

  METLIBS_LOG_DEBUG("SpectrumWindow::updateTimeBox");


  timeBox->clear();
  vector<miutil::miTime> times= spectrumm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).c_str()));
  }

  emit emitTimes("spectrum",times);
}


void SpectrumWindow::stationBoxActivated(int index)
{
  //vector<std::string> stations= spectrumm->getStationList();
  std::string sbs=stationBox->currentText().toStdString();
  //if (index>=0 && index<stations.size()) {
  spectrumm->setStation(sbs);
  spectrumw->updateGL();
  QString sq = sbs.c_str();
  emit spectrumChanged(sq); //name of current station (to mainWindow)
  //}
}


void SpectrumWindow::timeBoxActivated(int index)
{
  vector<miutil::miTime> times= spectrumm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    spectrumm->setTime(times[index]);

    spectrumw->updateGL();
  }
}


bool SpectrumWindow::changeStation(const std::string& station)
{

  METLIBS_LOG_DEBUG("SpectrumWindow::changeStation");

  spectrumm->setStation(station);
  spectrumw->updateGL();
  raise();
  if (stationChangedSlot(0))
    return true;
  else
    return false;
}

void SpectrumWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active) return;

  METLIBS_LOG_DEBUG("spectrumWindow::mainWindowTimeChanged called with time " << t);

  spectrumm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  stationChangedSlot(0);
  timeChangedSlot(0);
  spectrumw->updateGL();
}


void SpectrumWindow::startUp(const miutil::miTime& t)
{

  METLIBS_LOG_DEBUG("spectrumWindow::startUp called with time " << t);

  active = true;
  spToolbar->show();
  tsToolbar->show();
  spModelDialog->updateModelfileList();
  spectrumw->updateGL();
  changeModel();
  mainWindowTimeChanged(t);
}

void SpectrumWindow::parseSetup()
{
  spectrumm->parseSetup();
  spModelDialog->updateModelfileList();
}

vector<string> SpectrumWindow::writeLog(const string& logpart)
{
  vector<string> vstr;
  std::string str;

  if (logpart=="window") {

    str= "SpectrumWindow.size " + miutil::from_number(this->width()) + " " + miutil::from_number(this->height());
    vstr.push_back(str);
    str= "SpectrumWindow.pos "  + miutil::from_number(this->x()) + " " + miutil::from_number(this->y());
    vstr.push_back(str);
    str= "SpectrumModelDialog.pos " + miutil::from_number(spModelDialog->x()) + " " + miutil::from_number(spModelDialog->y());
    vstr.push_back(str);
    str= "SpectrumSetupDialog.pos " + miutil::from_number(spSetupDialog->x()) + " " + miutil::from_number(spSetupDialog->y());
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

    vstr= spectrumm->writeLog();

  }

  return vstr;
}


void SpectrumWindow::readLog(const std::string& logpart, const vector<string>& vstr,
    const string& thisVersion, const string& logVersion,
    int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<string> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= miutil::split(vstr[i], 0, " ");

      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="SpectrumWindow.size")  this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="SpectrumWindow.pos")      this->move(x,y);
          else if (tokens[0]=="SpectrumModelDialog.pos") spModelDialog->move(x,y);
          else if (tokens[0]=="SpectrumSetupDialog.pos") spSetupDialog->move(x,y);
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

    spectrumm->readLog(vstr,thisVersion,logVersion);

  }
}


void SpectrumWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
