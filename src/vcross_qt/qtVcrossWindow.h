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

#include "diVcrossInterface.h"

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

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE /* empty */
#endif

/**
  \brief Window for Vertical Crossections

  Contains a crossection window, toolbars and menues.
  Receives and sends "events" from/to DianaMainWindow.
*/
class VcrossWindow: public QWidget
{
  Q_OBJECT

public:
  VcrossWindow();
  ~VcrossWindow();

  // ========== begin called via VcrossWindowInterface
  void makeVisible(bool visible);
  bool changeCrossection(const std::string& crossection);
  void mainWindowTimeChanged(const miutil::miTime& t);
  void parseQuickMenuStrings(const std::vector<std::string>& vstr);
  void parseSetup();

  void writeLog(LogFileIO& logfile);
  void readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
      int displayWidth, int displayHeight);

  void dynCrossEditManagerChange(const QVariantMap &props);
  void dynCrossEditManagerRemoval(int id);
  void slotCheckEditmode(bool editing);
  // ========== end called via VcrossWindowInterface

Q_SIGNALS: // defined in VcrossInterface
  void VcrossHide();
  void requestHelpPage(const std::string&, const std::string& = ""); // activate help
  void requestLoadCrossectionFiles(const QStringList& filenames);
  //! called when draw/edit button is toggled
  void requestVcrossEditor(bool on);
  void crossectionChanged(const QString&);
  void crossectionSetChanged(const LocationData& locations);
  void emitTimes(const std::string&, const std::vector<miutil::miTime>&);
  void setTime(const std::string&, const miutil::miTime&);
  void quickMenuStrings(const std::string&, const std::vector<std::string>&);
  void nextHVcrossPlot();
  void prevHVcrossPlot();

protected:
  void closeEvent(QCloseEvent*);

private:
  //! setup UI and adjust some things that cannot be done easily with qt designer
  void setupUi();

  void makeEPS(const std::string& filename);

  // emits SIGNAL(crossectionSetChanged) with list of crossections from vcross-manager
  void emitCrossectionSet();
  void emitQmenuStrings();
  void stepTime(int direction);
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

  bool firstTime, active;
};

class VcrossWindowInterface : public VcrossInterface
{
  Q_INTERFACES(VcrossInterface)

public:
  VcrossWindowInterface();
  ~VcrossWindowInterface();

  void makeVisible(bool visible) Q_DECL_OVERRIDE
    { window->makeVisible(visible); }

  void parseSetup() Q_DECL_OVERRIDE
    { window->parseSetup(); }

  bool changeCrossection(const std::string& csName) Q_DECL_OVERRIDE
    { return window->changeCrossection(csName); }

  void mainWindowTimeChanged(const miutil::miTime& t) Q_DECL_OVERRIDE
    { window->mainWindowTimeChanged(t); }

  void parseQuickMenuStrings(const std::vector<std::string>& vstr) Q_DECL_OVERRIDE
    { window->parseQuickMenuStrings(vstr); }

  void writeLog(LogFileIO& logfile) Q_DECL_OVERRIDE
    { window->writeLog(logfile); }

  void readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
      int displayWidth, int displayHeight) Q_DECL_OVERRIDE
    { window->readLog(logfile, thisVersion, logVersion, displayWidth, displayHeight); }

public: /* Q_SLOT implementations */
  void editManagerChanged(const QVariantMap &props) Q_DECL_OVERRIDE
    { window->dynCrossEditManagerChange(props); }

  void editManagerRemoved(int id) Q_DECL_OVERRIDE
    { window->dynCrossEditManagerRemoval(id); }

  void editManagerEditing(bool editing) Q_DECL_OVERRIDE
    { window->slotCheckEditmode(editing); }

private:
  VcrossWindow* window;
};

#endif // _qt_vcrossmainwindow_
