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
#ifndef _qt_vprofmainwindow_
#define _qt_vprofmainwindow_

#include <qstring.h>
#include <diCommonTypes.h>
#include <diPrintOptions.h>

#include <QMainWindow>
#include <vector>

class QComboBox;
class QSpinBox;
class QToolBar;
class ToggleButton;
class VprofWidget;
class VprofManager;
class VprofModelDialog;
class VprofSetupDialog;
class StationPlot;
class QPrinter;

/**
  \brief Window for Vertical Profiles (soundings)

  Contains a diagram window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class VprofWindow: public QMainWindow
{
  Q_OBJECT

public:
  VprofWindow();
  ~VprofWindow(){}

  StationPlot* getStations();
  void changeStation(const std::string& station);
  void changeStations(const std::vector<std::string>& stations);
  void startUp(const miutil::miTime& t);
  void mainWindowTimeChanged(const miutil::miTime& t);

  void parseSetup();
  std::vector<std::string> writeLog(const std::string& logpart);
  void readLog(const std::string& logpart, const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion,
	       int displayWidth, int displayHeight);

  bool active;

protected:
  void closeEvent( QCloseEvent* );

private:
  VprofManager * vprofm;
  VprofWidget * vprofw;
  VprofModelDialog * vpModelDialog;
  VprofSetupDialog * vpSetupDialog;

  //qt widgets
  QToolBar *vpToolbar;
  QToolBar *tsToolbar;

  ToggleButton *modelButton;
  ToggleButton * setupButton;
  QComboBox * stationBox;
  QComboBox * timeBox;
  QSpinBox * timeSpinBox;

  void updateStationBox();
  void updateTimeBox();
  void stationChanged();
  void timeChanged();

  // printerdefinitions
  printOptions priop;

  miutil::miTime mainWindowTime;
  bool onlyObs; // if only observations, stations changes with time

  void makeEPS(const std::string& filename);

private slots:
  void modelClicked(bool on);
  void leftStationClicked();
  void rightStationClicked();
  void stationClicked( int i);
  void leftTimeClicked();
  void rightTimeClicked();
  void timeClicked(int i);
  void printClicked();
  void saveClicked();
  void setupClicked(bool on);
  void quitClicked();
  void updateClicked();
  void hideClicked();
  void helpClicked();
  void MenuOK();
  void changeModel();
  void changeSetup();
  void hideModel();
  void hideSetup();
  void stationBoxActivated(int index);
  void timeBoxActivated(int index);

signals:
  void VprofHide();
  void showsource(const std::string, const std::string=""); // activate help
  void stationChanged(const std::vector<std::string>& );
  void modelChanged();
  void emitTimes(const std::string&, const std::vector<miutil::miTime>& );
  void setTime(const std::string&, const miutil::miTime&);
};
#endif
