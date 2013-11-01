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

#ifndef _qt_vcrossmainwindow_
#define _qt_vcrossmainwindow_

#include "diPrintOptions.h"
#include <QtGui/QDialog>
#include <vector>

namespace miutil {
class miTime;
}
class Controller;
class LocationData;
class ToggleButton;
class VcrossWidget;
class VcrossManager;
class VcrossDialog;
class VcrossSetupDialog;

class QAction;
class QComboBox;
class QPrinter;
class QString;

/**
  \brief Window for Vertical Crossections

  Contains a crossection window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class VcrossWindow: public QDialog
{
  Q_OBJECT

public:
  VcrossWindow(Controller* co);
  ~VcrossWindow();

  //! alias for getCrossections \deprecated
  void getCrossectionOptions(LocationData& locationdata)
    { return getCrossections(locationdata); }
  void getCrossections(LocationData& locationdata);
  bool changeCrossection(const std::string& crossection);
  void startUp(const miutil::miTime& t);
  void mainWindowTimeChanged(const miutil::miTime& t);
  void parseQuickMenuStrings(const std::vector<std::string>& vstr);

  void parseSetup();

  std::vector<std::string> writeLog(const std::string& logpart);

  void readLog(const std::string& logpart, const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion,
      int displayWidth, int displayHeight);

  bool firstTime;
  bool active;

  ///add position to list of positions
  void mapPos(float lat, float lon);


protected:
  void closeEvent( QCloseEvent* );

private:
  VcrossManager * vcrossm;
  VcrossWidget * vcrossw;
  VcrossDialog * vcDialog;
  VcrossSetupDialog * vcSetupDialog;

  //qt widgets
  QAction *showPrevPlotAction;
  QAction *showNextPlotAction;
  ToggleButton * dataButton;
  ToggleButton * setupButton;
  ToggleButton * timeGraphButton;
  QComboBox * crossectionBox;
  QComboBox * timeBox;

  void updateCrossectionBox();
  void updateTimeBox();

  // printerdefinitions
  printOptions priop;

  void makeEPS(const std::string& filename);
  void emitQmenuStrings();

private Q_SLOTS:
  void dataClicked(bool on);
  void leftCrossectionClicked();
  void rightCrossectionClicked();
  void leftTimeClicked();
  void rightTimeClicked();
  void printClicked();
  void saveClicked();
  void setupClicked(bool on);
  void timeGraphClicked(bool on);
  void quitClicked();
  void helpClicked();
  void dynCrossClicked();
  void changeFields(bool modelChanged);
  void changeSetup();
  void hideDialog();
  void hideSetup();
  void crossectionBoxActivated(int index);
  void timeBoxActivated(int index);
  bool timeChangedSlot(int);
  bool crossectionChangedSlot(int);

Q_SIGNALS:
  void VcrossHide();
  void showsource(const std::string&, const std::string& = ""); // activate help
  void crossectionChanged(const QString&);
  void crossectionSetChanged();
  void crossectionSetUpdate();
  void emitTimes(const std::string&, const std::vector<miutil::miTime>&);
  void setTime(const std::string&, const miutil::miTime&);
  void updateCrossSectionPos(bool);
  void quickMenuStrings(const std::string&, const std::vector<std::string>&);
  void nextHVcrossPlot();
  void prevHVcrossPlot();
};

#endif // _qt_vcrossmainwindow_
