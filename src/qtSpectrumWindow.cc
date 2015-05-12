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
#include "diPaintGLPainter.h"

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
#include <QSvgGenerator>

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

  spectrumw= new SpectrumWidget(spectrumm);
  spectrumqw = DiPaintable::createWidget(spectrumw, this);

  setCentralWidget(spectrumqw);
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
  spectrumqw->update();
}


void SpectrumWindow::rightStationClicked()
{
  //called when the right Station button is clicked
  std::string s= spectrumm->setStation(+1);
  stationChangedSlot(+1);
  spectrumqw->update();
}


void SpectrumWindow::leftTimeClicked()
{
  //called when the left time button is clicked
  spectrumm->setTime(-1);
  timeChangedSlot(-1);
  spectrumqw->update();
}


void SpectrumWindow::rightTimeClicked()
{
  //called when the right Station button is clicked
  spectrumm->setTime(+1);
  timeChangedSlot(+1);
  spectrumqw->update();
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
    stationBox->insertItem(0,sq);
    stationBox->setCurrentIndex(0);
    return false;
  }
}


void SpectrumWindow::printClicked()
{
  // FIXME same as MainWindow::hardcopy
  QPrinter printer;
  QPrintDialog printerDialog(&printer, this);
  if (printerDialog.exec() != QDialog::Accepted || !printer.isValid())
    return;

  diutil::OverrideCursor waitCursor;
  paintOnDevice(&printer);
}

void SpectrumWindow::saveClicked()
{
  // FIXME this is the same as MainWindow::saveraster
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = QFileDialog::getSaveFileName(this,
      tr("Save plot as image"),
      fname,
      tr("Images (*.png *.jpeg *.jpg *.xpm *.bmp *.svg);;PDF Files (*.pdf);;All (*.*)"));

  if (s.isNull())
    return;
  fname = s;
  saveRasterImage(fname);
}

void SpectrumWindow::saveRasterImage(const QString& filename)
{
  // FIXME this is almost the same as MainWindow::saveRasterImage

  METLIBS_LOG_SCOPE(LOGVAL(filename.toStdString()));
  QPrinter* printer = 0;
  QImage* image = 0;
  std::auto_ptr<QPaintDevice> device;
  if (filename.endsWith(".pdf")) {
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setOutputFileName(filename);
    printer->setFullPage(true);
    printer->setPaperSize(spectrumqw->size(), QPrinter::DevicePixel);

    // FIXME copy from bdiana
    // According to QTBUG-23868, orientation and custom paper sizes do not
    // play well together. Always use portrait.
    printer->setOrientation(QPrinter::Portrait);

    device.reset(printer);
  } else if (filename.endsWith(".svg")) {
    QSvgGenerator* generator = new QSvgGenerator();
    generator->setFileName(filename);
    generator->setSize(spectrumqw->size());
    generator->setViewBox(QRect(0, 0, spectrumqw->width(), spectrumqw->height()));
    generator->setTitle(tr("diana image"));
    generator->setDescription(tr("Created by diana %1.").arg(PVERSION));

    // FIXME copy from bdiana
    // For some reason, QPrinter can determine the correct resolution to use, but
    // QSvgGenerator cannot manage that on its own, so we take the resolution from
    // a QPrinter instance which we do not otherwise use.
    QPrinter sprinter;
    generator->setResolution(sprinter.resolution());

    device.reset(generator);
  } else {
    image = new QImage(spectrumqw->size(), QImage::Format_ARGB32_Premultiplied);
    image->fill(Qt::transparent);
    device.reset(image);
  }

  paintOnDevice(device.get());

  if (image)
    image->save(filename);
}

void SpectrumWindow::paintOnDevice(QPaintDevice* device)
{
  // FIXME this is almost the same as MainWindow::paintOnDevice
  METLIBS_LOG_SCOPE();
  DiCanvas* oldCanvas = spectrumw->canvas();

  std::auto_ptr<DiPaintGLCanvas> glcanvas(new DiPaintGLCanvas(device));
  std::auto_ptr<DiPaintGLPainter> glpainter(new DiPaintGLPainter(glcanvas.get()));
  glpainter->printing = (dynamic_cast<QPrinter*>(device) != 0);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);

  const int ww = spectrumqw->width(), wh = spectrumqw->height(), dw = device->width(), dh = device->height();
  METLIBS_LOG_DEBUG(LOGVAL(ww) << LOGVAL(wh) << LOGVAL(dw) << LOGVAL(dh));

  QPainter painter;
  painter.begin(device);

  spectrumw->setCanvas(glcanvas.get());
#if 1
  glpainter->Viewport(0, 0, dw, dh);
  spectrumw->resize(dw, dh);
#else
  painter.setWindow(0, 0, ww, wh);
  glpainter->Viewport(0, 0, ww, wh);
  spectrumw->resize(ww, wh);
#endif

  glpainter->begin(&painter);
  spectrumw->paint(glpainter.get());
  glpainter->end();
  painter.end();

  spectrumw->setCanvas(oldCanvas);
  spectrumw->resize(ww, wh);
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
  METLIBS_LOG_DEBUG("quit clicked");

  //for now, only hide window, not really quit !
  spToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);

  active = false;
  emit SpectrumHide();
  vector<miutil::miTime> t;
  emit emitTimes("spectrum",t);
}


void SpectrumWindow::updateClicked()
{
  METLIBS_LOG_DEBUG("update clicked");

  miutil::miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}


void SpectrumWindow::helpClicked()
{
  METLIBS_LOG_DEBUG("help clicked");
  emit showsource("ug_spectrum.html");
}


void SpectrumWindow::MenuOK()
{
  //obsolete - nothing happens here
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
  spectrumqw->update();
}


void SpectrumWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked

  METLIBS_LOG_DEBUG("SpectrumWindow::changeSetup()");

  spectrumqw->update();
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
  spectrumqw->update();
  QString sq = sbs.c_str();
  emit spectrumChanged(sq); //name of current station (to mainWindow)
  //}
}


void SpectrumWindow::timeBoxActivated(int index)
{
  vector<miutil::miTime> times= spectrumm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    spectrumm->setTime(times[index]);

    spectrumqw->update();
  }
}


bool SpectrumWindow::changeStation(const std::string& station)
{

  METLIBS_LOG_DEBUG("SpectrumWindow::changeStation");

  spectrumm->setStation(station);
  spectrumqw->update();
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
  spectrumqw->update();
}


void SpectrumWindow::startUp(const miutil::miTime& t)
{

  METLIBS_LOG_DEBUG("spectrumWindow::startUp called with time " << t);

  active = true;
  spToolbar->show();
  tsToolbar->show();
  spModelDialog->updateModelfileList();
  spectrumqw->update();
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
