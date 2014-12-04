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

#include "vcross_v2/VcrossQtManager.h"
#include "diVcrossSelectionManager.h"
#include "VcrossQtWidget.h"
#include "diPrintOptions.h"

#include <QDialog>
#include <QVariant>

#include <vector>

namespace miutil {
class miTime;
}
class ActionButton;
class Controller;
class LocationData;
class ToggleButton;
class VcrossSetupDialog;
class Ui_VcrossWindow;

class QAction;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QPrinter;
class QString;
class QSpinBox;

/**
  \brief Window for Vertical Crossections

  Contains a crossection window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class VcrossWindow: public QWidget
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

protected:
  void closeEvent(QCloseEvent*);

private:
  //! setup UI and adjust some things that cannot be done easily with qt designer
  void setupUi();

  void makeEPS(const std::string& filename);
  void emitQmenuStrings();
  void stepTime(int direction);
  void dynCrossEditManagerEnableSignals();
  void changeFields();
  void enableDynamicCsIfSupported();
  void updateCrossectionBox();
  void updateTimeBox();

private Q_SLOTS:
  // GUI slots for leayer buttons
  void onFieldAction(int position, int action);

  // slots for VcrossSelectionManager
  void onFieldAdded(const std::string& model, const std::string& field, int position);
  void onFieldUpdated(const std::string& model, const std::string& field, int position);
  void onFieldRemoved(const std::string& model, const std::string& field, int position);
  void onFieldsRemoved();

  // slots for diana main window
  bool crossectionChangedSlot(int);
  bool timeChangedSlot(int);

  // slots for diana main window drawing editor
  void dynCrossEditManagerChange(const QVariantMap &props);
  void dynCrossEditManagerRemoval(int id);
  void slotCheckEditmode(bool editing);

  // GUI slots for window
  void onAddField();
  void onRemoveAllFields();
  void leftCrossectionClicked();
  void rightCrossectionClicked();
  void crossectionBoxActivated(int index);
  void leftTimeClicked();
  void rightTimeClicked();
  void timeBoxActivated(int index);
  void printClicked();
  void saveClicked();
  void onShowSetupDialog();
  void timeGraphClicked(bool on);
  void quitClicked();
  void helpClicked();
  void dynCrossEditManagerEnabled(bool);

  // slots for vcross setup dialog
  void changeSetup();

private:
  vcross::QtManager_p vcrossm;

  std::auto_ptr<VcrossSelectionManager> selectionManager;
  std::auto_ptr<Ui_VcrossWindow> ui;

  VcrossSetupDialog* vcSetupDialog;
  bool dynEditManagerConnected;

  // printerdefinitions
  printOptions priop;
  QString mRasterFilename;

  int vcrossDialogX, vcrossDialogY;
};

#endif // _qt_vcrossmainwindow_
