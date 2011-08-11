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
#ifndef _qt_vcrossmainwindow_
#define _qt_vcrossmainwindow_

#include <QMainWindow>
#include <qstring.h>
#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diLocationPlot.h>
#include <puTools/miString.h>
#include <vector>

using namespace std;

class QComboBox;
class QToolBar;
class ToggleButton;
class VcrossWidget;
class VcrossManager;
class VcrossDialog;
class VcrossSetupDialog;
class QPrinter;
class QAction;


/**

  \brief Window for Vertical Crossections

  Contains a crossection window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class VcrossWindow: public QMainWindow
{
  Q_OBJECT

public:
  VcrossWindow(Controller* co);
  ~VcrossWindow(){}

  void getCrossections(LocationData& locationdata);
  void getCrossectionOptions(LocationData& locationdata);
  bool changeCrossection(const miutil::miString& crossection);
  void startUp(const miutil::miTime& t);
  void mainWindowTimeChanged(const miutil::miTime& t);
  void parseQuickMenuStrings(const vector<miutil::miString>& vstr);

  vector<miutil::miString> writeLog(const miutil::miString& logpart);
  void readLog(const miutil::miString& logpart, const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion,
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
  QToolBar *vcToolbar;
  QToolBar *tsToolbar;
  ToggleButton * dataButton;
  ToggleButton * setupButton;
  ToggleButton * timeGraphButton;
  QComboBox * crossectionBox;
  QComboBox * timeBox;

  void updateCrossectionBox();
  void updateTimeBox();

  // printerdefinitions
  printOptions priop;

  void makeEPS(const miutil::miString& filename);

private slots:
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
  void MenuOK();
  void changeFields(bool modelChanged);
  void changeSetup();
  void hideDialog();
  void hideSetup();
  void crossectionBoxActivated(int index);
  void timeBoxActivated(int index);
  bool timeChangedSlot(int);
  bool crossectionChangedSlot(int);

signals:
  void VcrossHide();
  void showsource(const miutil::miString, const miutil::miString=""); // activate help
  void crossectionChanged(const QString& );
  void crossectionSetChanged();
  void crossectionSetUpdate();
  void emitTimes( const miutil::miString&,const vector<miutil::miTime>& );
  void setTime(const miutil::miString&, const miutil::miTime&);
  void updateCrossSectionPos(bool);
  void quickMenuStrings(const miutil::miString, const vector<miutil::miString>&);
  void nextHVcrossPlot();
  void prevHVcrossPlot();
};
#endif



