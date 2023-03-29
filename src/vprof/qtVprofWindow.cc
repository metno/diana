/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#include "diana_config.h"

#include "qtVprofWindow.h"

#include "qtMainWindow.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "qtVprofModelDialog.h"
#include "qtVprofSetupDialog.h"
#include "qtVprofUiEventHandler.h"

#include "diPaintGLPainter.h"
#include "diPaintableWidget.h"
#include "diStationPlot.h"
#include "diStationTypes.h"
#include "diUtilities.h"
#include "diVprofManager.h"
#include "diVprofOptions.h"
#include "diVprofPaintable.h"
#include "util/misc_util.h"

#include "export/PrinterDialog.h"

#include <puTools/miStringFunctions.h>

#include <QPixmap>
#include <QSpinBox>
#include <QToolBar>
#include <qcombobox.h>
#include <qfont.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>

#include "bakover.xpm"
#include "forover.xpm"

#define MILOGGER_CATEGORY "diana.VprofWindow"
#include <miLogger/miLogging.h>


VprofWindow::VprofWindow()
    : QMainWindow()
{
  vprofm = new VprofManager();

  setWindowTitle(tr("Diana Vertical Profiles"));

  vprofw = new VprofPaintable(vprofm);
  vprofi = new VprofUiEventHandler(vprofw);
  vprofqw = diana::createPaintableWidget(vprofw, vprofi, this);
  setCentralWidget(vprofqw);

  connect(vprofi, &VprofUiEventHandler::timeChanged, this, &VprofWindow::timeClicked);
  connect(vprofi, &VprofUiEventHandler::stationChanged, this, &VprofWindow::stationStepped);

  // tool bar and buttons
  vpToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea, vpToolbar);

  // tool bar for selecting time and station
  tsToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea, tsToolbar);

  // button for modeldialog-starts new dialog
  modelButton = new ToggleButton(this, tr("Model"));
  connect(modelButton, &ToggleButton::toggled, this, &VprofWindow::modelClicked);

  // button for setup - starts setupdialog
  setupButton = new ToggleButton(this, tr("Settings"));
  connect(setupButton, &ToggleButton::toggled, this, &VprofWindow::setupClicked);

  // button to print - starts print dialog
  QPushButton* printButton = new QPushButton(tr("Print"), this);
  printButton->setShortcut(tr("Ctrl+P"));
  connect(printButton, &QPushButton::clicked, this, &VprofWindow::printClicked);

  // button to save - starts save dialog
  QPushButton* saveButton = new QPushButton(tr("Save"), this);
  saveButton->setShortcut(tr("Ctrl+Shift+P"));
  connect(saveButton, &QPushButton::clicked, this, &VprofWindow::saveClicked);

  // button for quit
  QPushButton* quitButton = new QPushButton(tr("Quit"), this);
  connect(quitButton, &QPushButton::clicked, this, &VprofWindow::quitClicked);

  // button for help - pushbutton
  QPushButton* helpButton = new QPushButton(tr("Help"), this);
  connect(helpButton, &QPushButton::clicked, this, &VprofWindow::helpClicked);

  const QSizePolicy sp_fix_ex(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
  const QSizePolicy sp_min_ex(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

  // combobox to select station
  QPushButton* leftStationButton = new QPushButton(QPixmap(bakover_xpm), "", this);
  connect(leftStationButton, &QPushButton::clicked, this, &VprofWindow::leftStationClicked);
  leftStationButton->setAutoRepeat(true);

  stationBox = new QComboBox(this);
  stationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  stationBox->setSizePolicy(sp_min_ex);
  connect(stationBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &VprofWindow::stationBoxActivated);

  QPushButton* rightStationButton = new QPushButton(QPixmap(forward_xpm), "", this);
  connect(rightStationButton, &QPushButton::clicked, this, &VprofWindow::rightStationClicked);
  rightStationButton->setAutoRepeat(true);

  // combobox to select time
  QPushButton* leftTimeButton = new QPushButton(QPixmap(bakover_xpm), "", this);
  connect(leftTimeButton, &QPushButton::clicked, this, &VprofWindow::leftTimeClicked);
  leftTimeButton->setAutoRepeat(true);

  timeBox = new QComboBox(this);
  timeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  timeBox->setSizePolicy(sp_min_ex);
  connect(timeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &VprofWindow::timeBoxActivated);

  QPushButton* rightTimeButton = new QPushButton(QPixmap(forward_xpm), "", this);
  connect(rightTimeButton, &QPushButton::clicked, this, &VprofWindow::rightTimeClicked);
  rightTimeButton->setAutoRepeat(true);

  timeSpinBox = new QSpinBox(this);
  timeSpinBox->setValue(0);

  realizationSpinBox = new QSpinBox(this);
  realizationSpinBox->setMinimum(0);
  realizationSpinBox->setValue(0);
  realizationSpinBox->setToolTip(tr("Select ensemble member"));
  connect(realizationSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VprofWindow::realizationChanged);

  vpToolbar->addWidget(modelButton);
  vpToolbar->addWidget(setupButton);
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
  tsToolbar->addWidget(realizationSpinBox);

  // connected dialogboxes

  vpModelDialog = new VprofModelDialog(this, vprofm);
  connect(vpModelDialog, &VprofModelDialog::ModelApply, this, &VprofWindow::changeSetupOrModel);
  connect(vpModelDialog, &VprofModelDialog::ModelHide, this, &VprofWindow::hideModel);
  connect(vpModelDialog, &VprofModelDialog::showsource, this, &VprofWindow::showsource);

  vpSetupDialog = new VprofSetupDialog(this, vprofm);
  connect(vpSetupDialog, &VprofSetupDialog::SetupApply, this, &VprofWindow::changeSetupOrModel);
  connect(vpSetupDialog, &VprofSetupDialog::SetupHide, this, &VprofWindow::hideSetup);
  connect(vpSetupDialog, &VprofSetupDialog::showsource, this, &VprofWindow::showsource);

  // initialize everything in startUp
  active = false;
  mainWindowTime = miutil::miTime::nowTime();
}

VprofWindow::~VprofWindow()
{
}

/***************************************************************************/

void VprofWindow::modelClicked(bool on)
{
  vpModelDialog->setVisible(on);
}

/***************************************************************************/

void VprofWindow::leftStationClicked()
{
  stationStepped(-1);
}

void VprofWindow::rightStationClicked()
{
  stationStepped(1);
}

void VprofWindow::stationStepped(int i)
{
  int index = stationBox->currentIndex() - i;
  if (index < 0)
    index = stationBox->count() - 1;
  else if (index > stationBox->count() - 1)
    index = 0;

  stationBox->setCurrentIndex(index);
  const std::string s = stationBox->currentText().toStdString();
  vprofm->setStation(s);

  std::vector<std::string> stations(1, s);
  Q_EMIT stationChanged(stations); // name of current stations (to mainWindow)

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

void VprofWindow::realizationChanged(int value)
{
  vprofm->setRealization(value);
  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::timeChanged()
{
  METLIBS_LOG_SCOPE();

  if (!timeBox->count())
    return;

  const miutil::miTime& t = vprofm->getTime();

  const QString qt = QString::fromStdString(t.isoTime(false, false));
  for (int i = 0; i < timeBox->count(); i++) {
    if (qt == timeBox->itemText(i)) {
      timeBox->setCurrentIndex(i);
      break;
    }
  }

  // emit to main Window (updates stationPlot)
  Q_EMIT modelChanged();
  updateStationBox();
  updateStation();

  Q_EMIT setTime("vprof", t);
}

void VprofWindow::emitTimes(const plottimes_t& times)
{
  Q_EMIT sendTimes("vprof", times, true);
}

/***************************************************************************/

void VprofWindow::updateStation()
{
  METLIBS_LOG_SCOPE();
  vprofqw->update();
  raise();

  const std::vector<std::string>& stations = vprofm->getStations();

  // get current station
  QString current;
  if (!stations.empty()) {
    current = QString::fromStdString(stations.front());
  } else {
    // if no current station, use last station plotted
    current = QString::fromStdString(vprofm->getLastStation());
  }
  const int index = stationBox->findText(current);
  if (index >= 0)
    stationBox->setCurrentIndex(index);

  Q_EMIT stationChanged(stations); // name of current stations (to mainWindow)
}

/***************************************************************************/

void VprofWindow::printClicked()
{
  PrinterDialog dialog(this, vprofw->imageSource());
  dialog.print();
}

void VprofWindow::saveClicked()
{
  DianaMainWindow::instance()->showExportDialog(vprofw->imageSource());
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

  // for now, only hide window, not really quit !
  vpToolbar->hide();
  tsToolbar->hide();
  modelButton->setChecked(false);
  setupButton->setChecked(false);
  // NOTE: flush the cache...
  vprofm->cleanup();

  vpModelDialog->deleteAllClicked();

  active = false;
  Q_EMIT VprofHide();
  emitTimes(plottimes_t());
}

/***************************************************************************/

void VprofWindow::helpClicked()
{
  Q_EMIT showsource("ug_verticalprofiles.html");
}

/***************************************************************************/

void VprofWindow::changeSetupOrModel()
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv cmds = vpSetupDialog->getPlotCommands();
  diutil::insert_all(cmds, vpModelDialog->getPlotCommands());

  applyPlotCommands(cmds);
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

  const std::vector<stationInfo>& stations = vprofm->getStationList();
  METLIBS_LOG_DEBUG(LOGVAL(stations.size()));

  StationPlot* stationPlot = new StationPlot(stations);
  std::string ann = vprofm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);
  stationPlot->setPlotName(ann);
  return stationPlot;
}

/***************************************************************************/

void VprofWindow::updateStationBox()
{
  METLIBS_LOG_SCOPE();

  stationBox->clear();
  std::vector<stationInfo> stations = vprofm->getStationList();
  std::sort(stations.begin(), stations.end(), diutil::lt_StationName());

  for (size_t i = 0; i < stations.size(); i++)
    stationBox->addItem(QString::fromStdString(stations[i].name));
}

/***************************************************************************/

void VprofWindow::updateTimeBox()
{
  METLIBS_LOG_SCOPE();

  timeBox->clear();
  const plottimes_t& times = vprofm->getTimeList();
  for (const miutil::miTime& t : times)
    timeBox->addItem(QString::fromStdString(t.isoTime(false, false)));

  emitTimes(times);
}

/***************************************************************************/

void VprofWindow::stationBoxActivated(int)
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
  const plottimes_t& times = vprofm->getTimeList();

  if (index >= 0 && index < int(times.size())) {
    plottimes_t::const_iterator it = times.begin();
    std::advance(it, index);
    vprofm->setTime(*it);

    // emit to main Window (updates stationPlot)
    Q_EMIT modelChanged();
    // update combobox lists of stations and time
    updateStationBox();
    // get correct selection in comboboxes
    updateStation();

    vprofqw->update();
  }
}

/***************************************************************************/

void VprofWindow::changeStation(const std::string& station)
{
  METLIBS_LOG_SCOPE(LOGVAL(station));
  vprofm->setStation(station);
  updateStation();
}

void VprofWindow::changeStations(const std::vector<std::string>& stations)
{
  METLIBS_LOG_SCOPE(LOGVAL(stations.size()));
  vprofm->setStations(stations);
  updateStation();
}

/***************************************************************************/

void VprofWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));

  if (mainWindowTime == t)
    return;

  // keep time for next "update" (in case not found now)
  mainWindowTime = t;

  if (!active)
    return;

  vprofm->mainWindowTimeChanged(t);
  // emit to main Window (updates stationPlot)
  Q_EMIT modelChanged();
  updateStationBox();
  updateTimeBox();

  // get correct selection in comboboxes
  updateStation();
  timeChanged();
  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(t));

  if (DiCanvas* c = vprofw->canvas()) {
    c->parseFontSetup();
  }
  active = true;
  vpToolbar->show();
  tsToolbar->show();
  vpModelDialog->updateModelfileList();
  vprofqw->update();

  changeSetupOrModel();
  mainWindowTimeChanged(t);
}

void VprofWindow::applyPlotCommands(const PlotCommand_cpv& cmds)
{
  {
    diutil::OverrideCursor waitCursor;
    vprofm->applyPlotCommands(cmds);
    vpModelDialog->getModel();
  }

  // emit to main Window (updates stationPlot)
  Q_EMIT modelChanged();

  updateStationBox();
  updateTimeBox();
  realizationSpinBox->setMaximum(vprofm->getRealizationCount() - 1);
  // get correct selection in comboboxes
  updateStation();
  timeChanged();

  vprofqw->update();
}

/***************************************************************************/

void VprofWindow::parseSetup()
{
  if (DiCanvas* c = vprofw->canvas()) {
    c->parseFontSetup();
  }
  vprofm->parseSetup();
  vpModelDialog->updateModelfileList();
}

std::vector<std::string> VprofWindow::writeLog(const std::string& logpart)
{
  std::vector<std::string> vstr;
  std::string str;

  if (logpart == "window") {

    str = "VprofWindow.size " + miutil::from_number(this->width()) + " " + miutil::from_number(this->height());
    vstr.push_back(str);
    str = "VprofWindow.pos " + miutil::from_number(this->x()) + " " + miutil::from_number(this->y());
    vstr.push_back(str);
    str = "VprofModelDialog.pos " + miutil::from_number(vpModelDialog->x()) + " " + miutil::from_number(vpModelDialog->y());
    vstr.push_back(str);
    str = "VprofSetupDialog.pos " + miutil::from_number(vpSetupDialog->x()) + " " + miutil::from_number(vpSetupDialog->y());
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

  } else if (logpart == "setup") {

    vstr = vprofm->writeLog();
  }

  return vstr;
}

void VprofWindow::readLog(const std::string& logpart, const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion,
                          int displayWidth, int displayHeight)
{
  if (logpart == "window") {

    int n = vstr.size();

    for (int i = 0; i < n; i++) {
      std::vector<std::string> tokens = miutil::split(vstr[i], 0, " ");

      if (tokens.size() == 3) {

        int x = atoi(tokens[1].c_str());
        int y = atoi(tokens[2].c_str());
        if (x > 20 && y > 20 && x <= displayWidth && y <= displayHeight) {
          if (tokens[0] == "VprofWindow.size")
            this->resize(x, y);
        }
        if (x >= 0 && y >= 0 && x < displayWidth - 20 && y < displayHeight - 20) {
          if (tokens[0] == "VprofWindow.pos")
            this->move(x, y);
#ifdef DIANA_RESTORE_DIALOG_POSITIONS
          else if (tokens[0] == "VprofModelDialog.pos")
            vpModelDialog->move(x, y);
          else if (tokens[0] == "VprofSetupDialog.pos")
            vpSetupDialog->move(x, y);
#endif
        }

      } else if (tokens.size() == 2) {

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

  } else if (logpart == "setup") {

    vprofm->readLog(vstr, thisVersion, logVersion);
  }
}

/***************************************************************************/

void VprofWindow::closeEvent(QCloseEvent*)
{
  quitClicked();
}
