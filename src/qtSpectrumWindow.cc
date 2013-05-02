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

#include <qapplication.h>
#include <QFileDialog>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qfont.h>
#include <QPrintDialog>
#include <QPrinter>
#include <qmotifstyle.h>
#include <QPixmap>

#include "qtToggleButton.h"
#include "qtUtility.h"
#include "qtSpectrumWindow.h"
#include "diStationPlot.h"
#include "qtSpectrumWidget.h"
#include "qtSpectrumModelDialog.h"
#include "qtSpectrumSetupDialog.h"
#include "diSpectrumManager.h"
#include "qtPrintManager.h"
#include "forover.xpm"
#include "bakover.xpm"


SpectrumWindow::SpectrumWindow()
: QMainWindow( 0)
{

  spectrumm = new SpectrumManager();

  setWindowTitle( tr("Diana Wavespectrum") );

#if !defined(USE_PAINTGL)
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
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
  modelButton = new ToggleButton(this,tr("Model").toStdString());
  connect( modelButton, SIGNAL( toggled(bool)), SLOT( modelClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this,tr("Settings").toStdString());
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
  vector<miutil::miString> stations;
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
  vector<miutil::miString> times;
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

#ifdef DEBUGPRINT
  cerr<<"SpectrumWindow::SpectrumWindow() finished"<<endl;
#endif
}


void SpectrumWindow::modelClicked( bool on )
{
  //called when the model button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Model button clicked on" << endl;
#endif
    spModelDialog->show();
  } else {
#ifdef DEBUGPRINT
    cerr << "Model button clicked off" << endl;
#endif
    spModelDialog->hide();
  }
}


void SpectrumWindow::leftStationClicked()
{
  //called when the left Station button is clicked
  miutil::miString s= spectrumm->setStation(-1);
  stationChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightStationClicked()
{
  //called when the right Station button is clicked
  miutil::miString s= spectrumm->setStation(+1);
  stationChangedSlot(+1);
  spectrumw->updateGL();
}


void SpectrumWindow::leftTimeClicked()
{
  //called when the left time button is clicked
  miutil::miTime t= spectrumm->setTime(-1);
  timeChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightTimeClicked()
{
  //called when the right Station button is clicked
  miutil::miTime t= spectrumm->setTime(+1);
  timeChangedSlot(+1);
  spectrumw->updateGL();
}


bool SpectrumWindow::timeChangedSlot(int diff)
{
  //called if signal timeChanged is emitted from graphics
  //window (qtSpectrumWidget)
#ifdef DEBUGPRINT
  cerr << "timeChangedSlot(int) is called " << endl;
#endif
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
  miutil::miString tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  miutil::miString tbs=timeBox->currentText().toStdString();
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
    cerr << "WARNING! timeChangedSlot  time from spectrumm ="
    << t    <<" not equal to timeBox text = " << tbs << endl
    << "You should search through timelist!" << endl;
    return false;
  }

  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit spectrumSetChanged();
    //update combobox lists of stations and time
    updateStationBox();
    //get correct selection in comboboxes
    stationChangedSlot(0);
  }

  emit setTime("spectrum",t);

  return true;
}


bool SpectrumWindow::stationChangedSlot(int diff)
{
#ifdef DEBUGPRINT
  cerr << "stationChangedSlot(int) is called " << endl;
#endif
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
  miutil::miString s = spectrumm->getStation();
  //if (!stationBox->count()) return false;
  //if no current station, use last station plotted
  if (s.empty()) s = spectrumm->getLastStation();
  miutil::miString sbs=stationBox->currentText().toStdString();
  if (sbs!=s){
    int n = stationBox->count();
    for(int i = 0;i<n;i++){
      if (s==stationBox->itemText(i).toStdString()){
        stationBox->setCurrentIndex(i);
        sbs=miutil::miString(stationBox->currentText().toStdString());
        break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) {
    emit spectrumChanged(sq); //name of current station (to mainWindow)
    return true;
  } else {
    //    cerr << "WARNING! stationChangedSlot  station from spectrumm ="
    // 	 << s    <<" not equal to stationBox text = " << sbs << endl;
    //current or last station plotted is not in the list, insert it...
    stationBox->insertItem(0,sq);
    stationBox->setCurrentIndex(0);
    return false;
  }
}


void SpectrumWindow::printClicked()
{
  printerManager pman;
  miutil::miString command= pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    if (!qprt.outputFileName().isNull()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else {
      priop.fname= "prt_" + miutil::miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // set printername
    if (qprt.outputFileName().isNull())
      priop.printer= qprt.printerName().toStdString();

    // start the postscript production
    QApplication::setOverrideCursor( Qt::WaitCursor );

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
        cerr << "Print command:" << command << " failed" << endl;
      }
    }
    QApplication::restoreOverrideCursor();
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
    miutil::miString filename= s.toStdString();
    miutil::miString format= "PNG";
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
    spectrumw->saveRasterImage(filename, format, quality);
  }
}


void SpectrumWindow::makeEPS(const miutil::miString& filename)
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
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

  QApplication::restoreOverrideCursor();
}


void SpectrumWindow::setupClicked(bool on)
{
  //called when the setup button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Setup button clicked on" << endl;
#endif
    spSetupDialog->start();
    spSetupDialog->show();

  } else {
#ifdef DEBUGPRINT
    cerr << "Setup button clicked off" << endl;
#endif
    spSetupDialog->hide();
  }
}


void SpectrumWindow::quitClicked()
{
  //called when the quit button is clicked
#ifdef DEBUGPRINT
  cerr << "quit clicked" << endl;
#endif
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
#ifdef DEBUGPRINT
  cerr << "update clicked" << endl;
#endif
  spectrumm->updateObs();      // check obs.files
  miutil::miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}


void SpectrumWindow::helpClicked()
{
  //called when the help button in Spectrumwindow is clicked
#ifdef DEBUGPRINT
  cerr << "help clicked" << endl;
#endif
  emit showsource("ug_spectrum.html");
}


void SpectrumWindow::MenuOK()
{
  //obsolete - nothing happens here
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::MenuOK()" << endl;
#endif
}


void SpectrumWindow::changeModel()
{
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::changeModel()" << endl;
#endif
  spectrumm->setModel();

  onlyObs= spectrumm->onlyObsState();

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
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::changeSetup()" << endl;
#endif
  spectrumw->updateGL();
}


void SpectrumWindow::hideModel()
{
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::hideModel()" << endl;
#endif
  spModelDialog->hide();
  modelButton->setChecked(false);
}


void SpectrumWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::hideSetup()" << endl;
#endif
  spSetupDialog->hide();
  setupButton->setChecked(false);
}


StationPlot* SpectrumWindow::getStations()
{
#ifdef DEBUGPRINT
  cerr <<"SpectrumWindow::getStations()" << endl;
#endif
  const vector <miutil::miString> stations = spectrumm->getStationList();
  const vector <float> latitude = spectrumm->getLatitudes();
  const vector <float> longitude = spectrumm->getLongitudes();
  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  miutil::miString ann = spectrumm->getAnnotationString();
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


void SpectrumWindow::updateStationBox()
{
  //update list of stations in stationBox
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::updateStationBox" << endl;
#endif

  stationBox->clear();
  vector<miutil::miString> stations= spectrumm->getStationList();
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
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::updateTimeBox" << endl;
#endif

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
  //vector<miutil::miString> stations= spectrumm->getStationList();
  miutil::miString sbs=stationBox->currentText().toStdString();
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

    if (onlyObs) {
      //emit to main Window (updates stationPlot)
      emit spectrumSetChanged();
      //update combobox lists of stations and time
      updateStationBox();
      //get correct selection in comboboxes
      stationChangedSlot(0);
    }

    spectrumw->updateGL();
  }
}


bool SpectrumWindow::changeStation(const miutil::miString& station)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::changeStation" << endl;
#endif
  spectrumm->setStation(station);
  spectrumw->updateGL();
  raise();
  if (stationChangedSlot(0))
    return true;
  else
    return false;
}


void SpectrumWindow::setFieldModels(const vector<miutil::miString>& fieldmodels)
{
  spectrumm->setFieldModels(fieldmodels);
  if (active) changeModel();

}


void SpectrumWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active) return;
#ifdef DEBUGPRINT
  cerr << "spectrumWindow::mainWindowTimeChanged called with time " << t << endl;
#endif
  spectrumm->mainWindowTimeChanged(t);
  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit spectrumSetChanged();
    //update combobox lists of stations and time
    updateStationBox();
  }
  //get correct selection in comboboxes
  stationChangedSlot(0);
  timeChangedSlot(0);
  spectrumw->updateGL();
}


void SpectrumWindow::startUp(const miutil::miTime& t)
{
#ifdef DEBUGPRINT
  cerr << "spectrumWindow::startUp called with time " << t << endl;
#endif
  active = true;
  spToolbar->show();
  tsToolbar->show();
  //do something first time we start
  if (firstTime){
    //vector<miutil::miString> models;
    //define models for dialogs, comboboxes and stationplot
    //spectrumm->setSelectedModels(models,false,false);
    //spModelDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    spectrumw->updateGL();
  } else {
    changeModel();
  }
  mainWindowTimeChanged(t);
}

void SpectrumWindow::parseSetup()
{
  spectrumm->parseSetup();
  spModelDialog->updateModelfileList();
}

vector<miutil::miString> SpectrumWindow::writeLog(const miutil::miString& logpart)
{
  vector<miutil::miString> vstr;
  miutil::miString str;

  if (logpart=="window") {

    str= "SpectrumWindow.size " + miutil::miString(this->width()) + " "
    + miutil::miString(this->height());
    vstr.push_back(str);
    str= "SpectrumWindow.pos "  + miutil::miString(this->x()) + " "
    + miutil::miString(this->y());
    vstr.push_back(str);
    str= "SpectrumModelDialog.pos " + miutil::miString(spModelDialog->x()) + " "
    + miutil::miString(spModelDialog->y());
    vstr.push_back(str);
    str= "SpectrumSetupDialog.pos " + miutil::miString(spSetupDialog->x()) + " "
    + miutil::miString(spSetupDialog->y());
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

    vstr= spectrumm->writeLog();

  }

  return vstr;
}


void SpectrumWindow::readLog(const miutil::miString& logpart, const vector<miutil::miString>& vstr,
    const miutil::miString& thisVersion, const miutil::miString& logVersion,
    int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<miutil::miString> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split(' ');

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
