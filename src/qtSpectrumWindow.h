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
#ifndef _qt_spectrummainwindow_
#define _qt_spectrummainwindow_

#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diStationPlot.h>

#include <QMainWindow>
#include <QString>

#include <vector>

class QComboBox;
class QToolBar;
class ToggleButton;
class SpectrumWidget;
class SpectrumManager;
class SpectrumModelDialog;
class SpectrumSetupDialog;
class QPrinter;


/**
  \brief Window for Wave Spectrum

  Contains a diagram window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class SpectrumWindow: public QMainWindow
{
  Q_OBJECT
public:
  SpectrumWindow();
  ~SpectrumWindow(){}

  StationPlot* getStations();
  bool changeStation(const std::string& station); //change plotstation
  void startUp(const miutil::miTime& t);
  void mainWindowTimeChanged(const miutil::miTime& t);

  void parseSetup();
  std::vector<std::string> writeLog(const std::string& logpart);
  void readLog(const std::string& logpart, const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion,
	       int displayWidth, int displayHeight);

  bool firstTime;
  bool active;

protected:
  void closeEvent( QCloseEvent* );

private:
  SpectrumManager * spectrumm;
  SpectrumWidget * spectrumw;
  SpectrumModelDialog * spModelDialog;
  SpectrumSetupDialog * spSetupDialog;

  //qt widgets
  QToolBar *spToolbar;
  QToolBar *tsToolbar;


  ToggleButton *modelButton;
  ToggleButton * setupButton;
  QComboBox * stationBox;
  QComboBox * timeBox;

  void updateStationBox();
  void updateTimeBox();

  // printerdefinitions
  printOptions priop;

  miutil::miTime mainWindowTime;

  void makeEPS(const std::string& filename);

private slots:
  void modelClicked(bool on);
  void leftStationClicked();
  void rightStationClicked();
  void leftTimeClicked();
  void rightTimeClicked();
  void printClicked();
  void saveClicked();
  void setupClicked(bool on);
  void quitClicked();
  void updateClicked();
  void helpClicked();
  void MenuOK();
  void changeModel();
  void changeSetup();
  void hideModel();
  void hideSetup();
  void stationBoxActivated(int index);
  void timeBoxActivated(int index);
  bool timeChangedSlot(int);
  bool stationChangedSlot(int);

signals:
  void SpectrumHide();
  void showsource(const std::string, const std::string=""); // activate help
  void spectrumChanged(const QString& );
  void spectrumSetChanged();
  void emitTimes(const std::string&, const std::vector<miutil::miTime>& );
  void setTime(const std::string&, const miutil::miTime&);
};
#endif



