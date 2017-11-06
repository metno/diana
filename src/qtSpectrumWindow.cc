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
#include "qtSpectrumWidget.h"
#include "qtSpectrumModelDialog.h"
#include "qtSpectrumSetupDialog.h"
#include "diSpectrumManager.h"
#include "qtPrintManager.h"
#include "diPaintGLPainter.h"
#include "diStationPlot.h"

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

#define MILOGGER_CATEGORY "diana.SpectrumWindow"
#include <miLogger/miLogging.h>

#include "forover.xpm"
#include "bakover.xpm"

using namespace std;

SpectrumWindow::SpectrumWindow()
  : QMainWindow(0)
{
  spectrumm = new SpectrumManager();

  setWindowTitle( tr("Diana Wavespectrum") );

  spectrumw= new SpectrumWidget(spectrumm);
  spectrumqw = DiPaintable::createWidget(spectrumw, this);

  setCentralWidget(spectrumqw);
  connect(spectrumw, SIGNAL(timeChanged(int)),SLOT(timeClicked(int)));
  connect(spectrumw, SIGNAL(stationChanged(int)),SLOT(stationClicked(int)));

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

  const QSizePolicy sp_fix_ex(QSizePolicy::Fixed,   QSizePolicy::MinimumExpanding);
  const QSizePolicy sp_min_ex(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

  //combobox to select station
  QPushButton *leftStationButton = new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftStationButton, SIGNAL(clicked()), SLOT(leftStationClicked()) );
  leftStationButton->setAutoRepeat(true);

  stationBox = new QComboBox( this);
  stationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  stationBox->setSizePolicy(sp_min_ex);
  connect( stationBox, SIGNAL( activated(int) ), SLOT( stationBoxActivated(int) ) );

  QPushButton *rightStationButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightStationButton, SIGNAL(clicked()), SLOT(rightStationClicked()) );
  rightStationButton->setAutoRepeat(true);

  //combobox to select time
  QPushButton *leftTimeButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftTimeButton, SIGNAL(clicked()), SLOT(leftTimeClicked()) );
  leftTimeButton->setAutoRepeat(true);

  timeBox = new QComboBox( this);
  timeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  timeBox->setSizePolicy(sp_min_ex);
  connect( timeBox, SIGNAL( activated(int) ), SLOT( timeBoxActivated(int) ) );

  QPushButton *rightTimeButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  timeSpinBox = new QSpinBox( this );
  timeSpinBox->setValue(0);

  realizationSpinBox = new QSpinBox(this);
  realizationSpinBox->setMinimum(0);
  realizationSpinBox->setValue(0);
  realizationSpinBox->setToolTip(tr("Select ensemble member"));
  connect(realizationSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SpectrumWindow::realizationChanged);

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
  tsToolbar->addWidget(timeSpinBox);
  tsToolbar->addWidget(realizationSpinBox);

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
  active = false;
}


void SpectrumWindow::modelClicked( bool on )
{
  spModelDialog->setVisible(on);
}


void SpectrumWindow::leftStationClicked()
{
  stationClicked(-1);
}

void SpectrumWindow::rightStationClicked()
{
  stationClicked(1);
}

void SpectrumWindow::stationClicked(int i)
{
  int index = stationBox->currentIndex() - i;
  if (index < 0)
    index=stationBox->count()-1;
  else if (index > stationBox->count()-1)
    index = 0;

  stationBox->setCurrentIndex(index);
  const QString qs = stationBox->currentText();
  const std::string s = qs.toStdString();
  spectrumm->setStation(qs.toStdString());

  Q_EMIT spectrumChanged(qs); //name of current stations (to mainWindow)

  spectrumqw->update();
}
void SpectrumWindow::leftTimeClicked()
{
  timeClicked(-1);
}

void SpectrumWindow::rightTimeClicked()
{
  timeClicked(1);
}

void SpectrumWindow::timeClicked(int i)
{
  spectrumm->setTime(timeSpinBox->value(), i);
  timeChanged();
  spectrumqw->update();
}

void SpectrumWindow::timeChanged()
{
  METLIBS_LOG_SCOPE();

  if (!timeBox->count())
    return;

  const miutil::miTime& t = spectrumm->getTime();

  const QString qt = QString::fromStdString(t.isoTime(false,false));
  for (int i = 0; i<timeBox->count(); i++) {
    if (qt == timeBox->itemText(i)) {
      timeBox->setCurrentIndex(i);
      break;
    }
  }

  Q_EMIT setTime("spectrum", t);
}

void SpectrumWindow::realizationChanged(int value)
{
  spectrumm->setRealization(value);
  spectrumqw->update();
}

void SpectrumWindow::stationChanged()
{
  METLIBS_LOG_SCOPE();
  spectrumqw->update();
  raise();

  //get current station
  const QString qs = spectrumm->getLastStation().c_str();

  Q_EMIT spectrumChanged(qs); //name of current stations (to mainWindow)
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
  std::unique_ptr<QPaintDevice> device;
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

  std::unique_ptr<DiPaintGLCanvas> glcanvas(new DiPaintGLCanvas(device));
  glcanvas->parseFontSetup();
  glcanvas->setPrinting(dynamic_cast<QPrinter*>(device) != 0);
  std::unique_ptr<DiPaintGLPainter> glpainter(new DiPaintGLPainter(glcanvas.get()));
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
  spectrumw->paintOverlay(glpainter.get());
  glpainter->end();
  painter.end();

  spectrumw->setCanvas(oldCanvas);
  spectrumw->resize(ww, wh);
}

void SpectrumWindow::setupClicked(bool on)
{
  METLIBS_LOG_SCOPE(LOGVAL(on));
  //called when the setup button is clicked
  if( on ){
    spSetupDialog->start();
    spSetupDialog->show();
  } else {
    spSetupDialog->hide();
  }
}


void SpectrumWindow::quitClicked()
{
  METLIBS_LOG_SCOPE();

  //for now, only hide window, not really quit !
  spToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);

  spModelDialog->deleteAllClicked();

  active = false;
  Q_EMIT SpectrumHide();
  vector<miutil::miTime> t;
  Q_EMIT emitTimes("spectrum",t);
}


void SpectrumWindow::updateClicked()
{
  METLIBS_LOG_SCOPE();

  mainWindowTimeChanged(mainWindowTime); // use the main time (fields etc.)
}


void SpectrumWindow::helpClicked()
{
  METLIBS_LOG_SCOPE();
  Q_EMIT showsource("ug_spectrum.html");
}


void SpectrumWindow::changeModel()
{
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
  METLIBS_LOG_SCOPE();

  { diutil::OverrideCursor waitCursor;
    spectrumm->setModel();
  }

  //emit to main Window (updates stationPlot)
  Q_EMIT spectrumSetChanged();
  //update combobox lists of stations and time
  updateStationBox();
  updateTimeBox();
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  spectrumqw->update();
}


void SpectrumWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked
  METLIBS_LOG_SCOPE();

  spectrumqw->update();
}


void SpectrumWindow::hideModel()
{
  //called when the hide button (from model dialog) is clicked
  METLIBS_LOG_SCOPE();

  spModelDialog->hide();
  modelButton->setChecked(false);
}


void SpectrumWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked
  METLIBS_LOG_SCOPE();

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

  stationPlot->setPlotName(ann);

  return stationPlot;
}


void SpectrumWindow::updateStationBox()
{
  //update list of stations in stationBox
  METLIBS_LOG_SCOPE();

  stationBox->clear();
  vector<std::string> stations= spectrumm->getStationList();

  int n =stations.size();
  for (int i=0; i<n; i++) {
    stationBox->addItem(QString::fromStdString(stations[i]));
  }
}


void SpectrumWindow::updateTimeBox()
{
  //update list of times
  METLIBS_LOG_SCOPE();

  timeBox->clear();
  vector<miutil::miTime> times= spectrumm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString::fromStdString(times[i].isoTime(false,false)));
  }

  Q_EMIT emitTimes("spectrum",times);
}


void SpectrumWindow::stationBoxActivated(int index)
{
  std::string sbs=stationBox->currentText().toStdString();
  spectrumm->setStation(sbs);
  spectrumqw->update();
  QString sq = QString::fromStdString(sbs);
  Q_EMIT spectrumChanged(sq); //name of current station (to mainWindow)

}


void SpectrumWindow::timeBoxActivated(int index)
{
  vector<miutil::miTime> times= spectrumm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    spectrumm->setTime(times[index]);

    spectrumqw->update();
  }
}


void SpectrumWindow::changeStation(const std::string& station)
{
  METLIBS_LOG_SCOPE();

  spectrumm->setStation(station);
  stationChanged();
}

void SpectrumWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  if (mainWindowTime == t)
    return;

  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active)
    return;

  METLIBS_LOG_SCOPE(LOGVAL(t));

  spectrumm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  stationChanged();
  timeChanged();
  spectrumqw->update();
}


void SpectrumWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));

  if (DiCanvas* c = spectrumw->canvas()) {
    c->parseFontSetup();
  }
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
  if (DiCanvas* c = spectrumw->canvas()) {
    c->parseFontSetup();
  }
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
#ifdef DIANA_RESTORE_DIALOG_POSITIONS
          else if (tokens[0]=="SpectrumModelDialog.pos") spModelDialog->move(x,y);
          else if (tokens[0]=="SpectrumSetupDialog.pos") spSetupDialog->move(x,y);
#endif
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

    spectrumm->readLog(vstr,thisVersion,logVersion);

  }
}


void SpectrumWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
