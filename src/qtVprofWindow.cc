/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "qtVprofWindow.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "qtVprofWidget.h"
#include "qtVprofModelDialog.h"
#include "qtVprofSetupDialog.h"
#include "qtPrintManager.h"

#include "diPaintGLPainter.h"
#include "diStationPlot.h"
#include "diUtilities.h"
#include "diVprofManager.h"

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
#include <QSvgGenerator>

#include "forover.xpm"
#include "bakover.xpm"

#define MILOGGER_CATEGORY "diana.VprofWindow"
#include <miLogger/miLogging.h>

using namespace std;

VprofWindow::VprofWindow()
  : QMainWindow(0)
{
  vprofm = new VprofManager();

  setWindowTitle( tr("Diana Vertical Profiles") );

  vprofw= new VprofWidget(vprofm);
  vprofqw = DiPaintable::createWidget(vprofw, this);
  setCentralWidget(vprofqw);

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


  const QSizePolicy sp_fix_ex(QSizePolicy::Fixed,   QSizePolicy::MinimumExpanding);
  const QSizePolicy sp_min_ex(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

  //combobox to select station
  leftStationButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  leftStationButton->setEnabled(false);
  leftStationButton->setSizePolicy(sp_fix_ex);
  connect(leftStationButton, SIGNAL(clicked()), SLOT(leftStationClicked()) );
  leftStationButton->setAutoRepeat(true);

  stationBox = new QComboBox(this);
  stationBox->setEnabled(false);
  stationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  stationBox->setSizePolicy(sp_min_ex);
  connect( stationBox, SIGNAL( activated(int) ), SLOT( stationBoxActivated(int) ) );

  rightStationButton= new QPushButton(QPixmap(forward_xpm),"",this);
  rightStationButton->setEnabled(false);
  rightStationButton->setSizePolicy(sp_fix_ex);
  connect(rightStationButton, SIGNAL(clicked()), SLOT(rightStationClicked()) );
  rightStationButton->setAutoRepeat(true);

  leftTimeButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  leftTimeButton->setEnabled(false);
  leftTimeButton->setSizePolicy(sp_fix_ex);
  connect(leftTimeButton, SIGNAL(clicked()), SLOT(leftTimeClicked()) );
  leftTimeButton->setAutoRepeat(true);

  timeBox = new QComboBox(this);
  timeBox->setEnabled(false);
  timeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  timeBox->setSizePolicy(sp_min_ex);
  connect( timeBox, SIGNAL( activated(int) ), SLOT( timeBoxActivated(int) ) );

  rightTimeButton= new QPushButton(QPixmap(forward_xpm),"",this);
  rightTimeButton->setEnabled(false);
  rightTimeButton->setSizePolicy(sp_fix_ex);
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  timeSpinBox = new QSpinBox( this );
  timeSpinBox->setValue(0);
  timeBox->setSizePolicy(sp_min_ex);

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
}


/***************************************************************************/

void VprofWindow::modelClicked(bool on)
{
  vpModelDialog->setVisible(on);
}

/***************************************************************************/

void VprofWindow::leftStationClicked()
{
  stationClicked(-1);
}

void VprofWindow::rightStationClicked()
{
  stationClicked(1);
}

void VprofWindow::stationClicked(int i)
{
  int index = stationBox->currentIndex() - i;
  if (index < 0)
    index=stationBox->count()-1;
  else if (index > stationBox->count()-1)
    index = 0;

  stationBox->setCurrentIndex(index);
  const std::string s = stationBox->currentText().toStdString();
  vprofm->setStation(s);

  std::vector<std::string> stations(1, s);
  Q_EMIT stationChanged(stations); //name of current stations (to mainWindow)

  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::leftTimeClicked()
{
  timeClicked(-1);
}

void VprofWindow::rightTimeClicked()
{
  timeClicked(1);
}

void VprofWindow::timeClicked(int i)
{
  vprofm->setTime(timeSpinBox->value(), i);
  timeChanged();
  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::timeChanged()
{
  METLIBS_LOG_SCOPE();

  if (!timeBox->count())
    return;

  const miutil::miTime& t = vprofm->getTime();

  const QString qt = QString::fromStdString(t.isoTime(false,false));
  for (int i = 0; i<timeBox->count(); i++) {
    if (qt == timeBox->itemText(i)) {
      timeBox->setCurrentIndex(i);
      break;
    }
  }

  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    Q_EMIT modelChanged();
    updateStationBox();
    stationChanged();
  }

  Q_EMIT setTime("vprof", t);
}


/***************************************************************************/

void VprofWindow::stationChanged()
{
  METLIBS_LOG_SCOPE();
  vprofqw->update();
  raise();

  //get current station
  vector<std::string> vs = vprofm->getStations();
  //if no current station, use last station plotted
  if (!vs.size()) {
    vs.push_back(vprofm->getLastStation());
  }
  const QString qs = QString::fromStdString(vs[0]);
  for (int i = 0;i<stationBox->count(); i++) {
    if (qs == stationBox->itemText(i)) {
      stationBox->setCurrentIndex(i);
      break;
    }
  }
  Q_EMIT stationChanged(vs); //name of current stations (to mainWindow)
}


/***************************************************************************/

void VprofWindow::printClicked()
{
  // FIXME same as MainWindow::hardcopy
  QPrinter printer;
  QPrintDialog printerDialog(&printer, this);
  if (printerDialog.exec() != QDialog::Accepted || !printer.isValid())
    return;

  diutil::OverrideCursor waitCursor;
  paintOnDevice(&printer);
}


void VprofWindow::saveClicked()
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

void VprofWindow::saveRasterImage(const QString& filename)
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
    printer->setPaperSize(vprofqw->size(), QPrinter::DevicePixel);

    // FIXME copy from bdiana
    // According to QTBUG-23868, orientation and custom paper sizes do not
    // play well together. Always use portrait.
    printer->setOrientation(QPrinter::Portrait);

    device.reset(printer);
  } else if (filename.endsWith(".svg")) {
    QSvgGenerator* generator = new QSvgGenerator();
    generator->setFileName(filename);
    generator->setSize(vprofqw->size());
    generator->setViewBox(QRect(0, 0, vprofqw->width(), vprofqw->height()));
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
    image = new QImage(vprofqw->size(), QImage::Format_ARGB32_Premultiplied);
    image->fill(Qt::transparent);
    device.reset(image);
  }

  paintOnDevice(device.get());

  if (image)
    image->save(filename);
}

void VprofWindow::paintOnDevice(QPaintDevice* device)
{
  // FIXME this is almost the same as MainWindow::paintOnDevice
  METLIBS_LOG_SCOPE();
  DiCanvas* oldCanvas = vprofw->canvas();

  std::auto_ptr<DiPaintGLCanvas> glcanvas(new DiPaintGLCanvas(device));
  std::auto_ptr<DiPaintGLPainter> glpainter(new DiPaintGLPainter(glcanvas.get()));
  glpainter->printing = (dynamic_cast<QPrinter*>(device) != 0);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);

  const int ww = vprofqw->width(), wh = vprofqw->height(), dw = device->width(), dh = device->height();
  METLIBS_LOG_DEBUG(LOGVAL(ww) << LOGVAL(wh) << LOGVAL(dw) << LOGVAL(dh));

  QPainter painter;
  painter.begin(device);

  vprofw->setCanvas(glcanvas.get());
#if 1
  glpainter->Viewport(0, 0, dw, dh);
  vprofw->resize(dw, dh);
#else
  painter.setWindow(0, 0, ww, wh);
  glpainter->Viewport(0, 0, ww, wh);
  vprofw->resize(ww, wh);
#endif

  glpainter->begin(&painter);
  vprofw->paint(glpainter.get());
  glpainter->end();
  painter.end();

  vprofw->setCanvas(oldCanvas);
  vprofw->resize(ww, wh);
}

/***************************************************************************/

void VprofWindow::setupClicked(bool on)
{
  if (on)
    vpSetupDialog->start();
  vpSetupDialog->setVisible(on);
}

/***************************************************************************/

void VprofWindow::quitClicked()
{
  METLIBS_LOG_SCOPE();

  //for now, only hide window, not really quit !
  vpToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);
  // NOTE: flush the fieldCache...
  vprofm->cleanup();

  active = false;
  Q_EMIT VprofHide();
  vector<miutil::miTime> t;
  Q_EMIT emitTimes("vprof",t);
}

/***************************************************************************/

void VprofWindow::hideClicked()
{
}

/***************************************************************************/

void VprofWindow::updateClicked()
{
  METLIBS_LOG_SCOPE();

  vprofm->updateObs();      // check obs.files
  mainWindowTimeChanged(mainWindowTime); // use the main time (fields etc.)
}

/***************************************************************************/

void VprofWindow::helpClicked()
{
  Q_EMIT showsource("ug_verticalprofiles.html");
}

/***************************************************************************/

void VprofWindow::changeModel()
{
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
  METLIBS_LOG_SCOPE();

  { diutil::OverrideCursor waitCursor;
    vprofm->setModel();
  }

  onlyObs= vprofm->onlyObsState();

  //emit to main Window (updates stationPlot)
  Q_EMIT modelChanged();

  updateStationBox();
  updateTimeBox();
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::changeSetup()
{
  METLIBS_LOG_SCOPE();
  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::hideModel()
{
  vpModelDialog->hide();
  modelButton->setChecked(false);
}

/***************************************************************************/

void VprofWindow::hideSetup()
{
  vpSetupDialog->hide();
  setupButton->setChecked(false);
}

/***************************************************************************/

StationPlot* VprofWindow::getStations()
{
  METLIBS_LOG_SCOPE();

  const vector <std::string> stations = vprofm->getStationList();
  const vector <float> latitude = vprofm->getLatitudes();
  const vector <float> longitude = vprofm->getLongitudes();

  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  std::string ann = vprofm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);
  stationPlot->setPlotName(ann); // ADC set plotname (for StatusPlotButtons)
  return stationPlot;
}

/***************************************************************************/

void VprofWindow::updateStationBox()
{
  METLIBS_LOG_SCOPE();

  stationBox->clear();
  std::vector<std::string> stations = vprofm->getStationList();
  std::sort(stations.begin(),stations.end());

  for (size_t i=0; i<stations.size(); i++)
    stationBox->addItem(QString::fromStdString(stations[i]));

  stationBox        ->setEnabled(!stations.empty());
  leftStationButton ->setEnabled(!stations.empty());
  rightStationButton->setEnabled(!stations.empty());
}

/***************************************************************************/

void VprofWindow::updateTimeBox()
{
  METLIBS_LOG_SCOPE();

  timeBox->clear();
  const std::vector<miutil::miTime>& times = vprofm->getTimeList();
  for (size_t i=0; i<times.size(); i++)
    timeBox->addItem(QString::fromStdString(times[i].isoTime(false,false)));

  timeBox        ->setEnabled(!times.empty());
  leftTimeButton ->setEnabled(!times.empty());
  rightTimeButton->setEnabled(!times.empty());

  Q_EMIT emitTimes("vprof", times);
}

/***************************************************************************/

void VprofWindow::stationBoxActivated(int index)
{
  std::string sbs = stationBox->currentText().toStdString();
  vprofm->setStation(sbs);
  vprofqw->update();

  const std::vector<std::string> stations(1, sbs);
  Q_EMIT stationChanged(stations); // name of current station (to mainWindow)
}

/***************************************************************************/

void VprofWindow::timeBoxActivated(int index)
{
  const std::vector<miutil::miTime>& times = vprofm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    vprofm->setTime(times[index]);

    if (onlyObs) {
      //emit to main Window (updates stationPlot)
      Q_EMIT modelChanged();
      //update combobox lists of stations and time
      updateStationBox();
      //get correct selection in comboboxes
      stationChanged();
    }

    vprofqw->update();
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

void VprofWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));

  // keep time for next "update" (in case not found now)
  mainWindowTime = t;

  if (!active)
    return;

  vprofm->mainWindowTimeChanged(t);
  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    Q_EMIT modelChanged();
    updateStationBox();
    updateTimeBox();
  }
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  vprofqw->update();
}


/***************************************************************************/

void VprofWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));
  active = true;
  vpToolbar->show();
  tsToolbar->show();
  vpModelDialog->updateModelfileList();
  vprofqw->update();

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

#if 0
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
#endif

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

#if 0
        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
          if (tokens[1]=="portrait")
            priop.orientation=d_print::ori_portrait;
          else
            priop.orientation=d_print::ori_landscape;
        }
#endif
      }
    }

  } else if (logpart=="setup") {

    vprofm->readLog(vstr,thisVersion,logVersion);

  }
}

/***************************************************************************/

void VprofWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
