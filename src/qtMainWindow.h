/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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
#ifndef _qt_mainwindow_
#define _qt_mainwindow_

#include "EditItems/toolbar.h"
#include "diAreaTypes.h"
#include "diCommonTypes.h"
#include "diField/diRectangle.h"
#include "diMapMode.h"
#include "diPlotCommand.h"
#include "diPlotElement.h"
#include "diPrintOptions.h"

#include <puTools/miTime.h>

#include <QMainWindow>
#include <QLabel>
#include <QFocusEvent>

#include <vector>
#include <deque>

class QAction;
class QDockWidget;
class QMenu;
class QMenuBar;
class QLabel;
class QPushButton;
class QShortcut;
class QToolBar;
class QToolButton;

class WorkArea;
class QuickMenu;
class FieldDialog;
struct LocationData;
class ObsDialog;
class SatDialog;
class StationDialog;
class MapDialog;
class ObjectDialog;
class TrajectoryDialog;
class AnnotationDialog;
class MeasurementsDialog;

class Controller;
class DataDialog;
class DianaImageSource;
class EditDialog;
class ExportImageDialog;
class ImageSource;
class HelpDialog;
class ShowSatValues;
class VprofWindow;
class VcrossInterface;
class SpectrumWindow;
class StatusGeopos;
class StatusPlotButtons;
class BrowserBox;
class ClientSelection;
class miMessage;
class miQMessage;
class StationPlot;
class TextView;
class TimeNavigator;
class MovieMaker;

/**

  \brief Main application window

  The main application window containing:
  - map area
  - toolbars and menues

*/

class DianaMainWindow: public QMainWindow
{
  Q_OBJECT
public:
  DianaMainWindow(Controller*, const QString& instancename="Diana");
  ~DianaMainWindow();

  /// check if news file has changed since last startup
  void checkNews();
  void start();

  /// Returns the application's main window instance.
  static DianaMainWindow *instance();

  static std::string getLogFileDir();
  static std::string getLogFileExt();
  static bool allowedInstanceName(const QString& text);

  QString instanceName() const;
  QString instanceNameSuffix() const;

  void showExportDialog(ImageSource* source);

  void applyQuickMenu(const QString& qmenu, const QString& qitem);

protected:
  void focusInEvent ( QFocusEvent * );
  void closeEvent( QCloseEvent* );
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);
  bool event(QEvent* event);

private Q_SLOTS:
  void levelUp();
  void levelDown();
  void idnumUp();
  void idnumDown();
  void mapMenu(int result = -1);
  void mapDockVisibilityChanged(bool visible);
  void editMenu();
  void vprofMenu();
  void spectrumMenu();
  void trajMenu(int result = -1);
  void AnnotationMenu(int result = -1);
  void measurementsMenu(int result = -1);
  void quickMenu(int result = -1);

  void showHelp();
  void showAccels();
  void showNews();
  void showUrl();

  void about();
  void recallPlot(const PlotCommand_cpv&, bool);
  void resetArea();
  void resetAll();
  void editApply();
  void MenuOK();
  void trajPositions(bool);
  void measurementsPositions(bool);
  void catchMouseGridPos(QMouseEvent*);
  void catchMouseRightPos(QMouseEvent*);
  void catchMouseMovePos(QMouseEvent*,bool);
  void catchKeyPress(QKeyEvent*);
  void catchMouseDoubleClick(QMouseEvent*);
  void catchElement(QMouseEvent*);
  void showStationOrObsText(int x, int y);
  void undo();
  void redo();
  void save();
  void parseSetup();

  void hardcopy();
  void previewHardcopy();
  void saveraster();
  void setPlotTime(const miutil::miTime& t);
  void requestBackgroundBufferUpdate();

  void toggleDialogs();
  void filequit();
  void writeLogFile();

  void info_activated(QAction *);
  void hideVprofWindow();
  void hideSpectrumWindow();
  void stationChangedSlot(const std::vector<std::string>&);
  void modelChangedSlot();

  //! show vcross window
  void vcrossMenu();
  void hideVcrossWindow();
  void crossectionSetChangedSlot(const LocationData& locations);
  void crossectionChangedSlot(const QString& name);
  void updateVcrossQuickMenuHistory(const std::string& plotname, const PlotCommand_cpv&);
  void onVcrossRequestLoadCrossectionsFile(const QStringList& filenames);
  void onVcrossRequestEditManager(bool on, bool timeGraph);

  void spectrumChangedSlot(const QString& name);
  void spectrumSetChangedSlot();

  void connectionClosed();
  void processLetter(int fromId, const miQMessage&);
  void sendLetter(const miQMessage& qmsg, int to);
  void sendLetter(const miQMessage& qmsg);

  void updateObs();
  void autoUpdate();
  void updateGLSlot();
  void showElements();
  void archiveMode();
  void showAnnotations();
  void toggleScrollwheelZoom();
  void chooseFont();
  void toggleElement(PlotElement);
  void prevHPlot();
  void nextHPlot();
  void prevHVcrossPlot();
  void nextHVcrossPlot();
  void prevQPlot();
  void nextQPlot();
  void prevList();
  void nextList();
  void startBrowsing();
  void browserSelect();
  void browserCancel();
  void addToMenu();

  void zoomOut();

  void winResize(int, int);
  void inEdit(bool);
  void sendPrintClicked(int);
  void toggleEditDrawingMode();
  void setEditDrawingMode(bool);

  void setWorkAreaCursor(const QCursor &);
  void unsetWorkAreaCursor();

  void updatePlotElements();

  void setInstanceName(QString instancename);

Q_SIGNALS:
  void instanceNameChanged(const QString&);

private:
  /// Adds a dialog for use with a manager.
  void addDialog(DataDialog* dialog);

  /// Adds a standard dialog (fields, ...)
  void addStandardDialog(DataDialog* dialog);

  void applyPlotCommandsFromDialogs(bool addToHistory);
  void initCoserverClient();
  void createHelpDialog();
  void vcrossEditManagerEnableSignals();
  DianaImageSource* imageSource();

private:
  bool browsing;       // user browsing through plot-stack
  BrowserBox* browser; // shows plot-stack
  bool updateBrowser();// update browser-window
  bool doAutoUpdate;

  int displayWidth,displayHeight;

  QAction * optOnOffAction;
  QAction * optArchiveAction;
  QAction * optAutoElementAction;
  QAction * optAnnotationAction;
  QAction * optScrollwheelZoomAction;
  QAction * optFontAction;

  QAction * showHideAllAction;
  QAction * showQuickmenuAction;
  QAction * showMapDialogAction;
  QAction * showEditDialogAction;
  QAction * showTrajecDialogAction;
  QAction * showAnnotationDialogAction;
  QAction * showMeasurementsDialogAction;

  QAction * toolLevelUpAction;
  QAction * toolLevelDownAction;

  QAction * toolIdnumUpAction;
  QAction * toolIdnumDownAction;

  QAction * autoUpdateAction;

  QMenu * showmenu;

  QToolBar * mainToolbar;
  QToolBar * levelToolbar;

  TimeNavigator* timeNavigator;

  // printerdefinitions
  printOptions priop;

  WorkArea          * w;
  QuickMenu         * qm;
  MapDialog         * mm;
  QDockWidget* mmdock;
  EditDialog        * em;
  FieldDialog       * fm;
  ObsDialog         * om;
  SatDialog         * sm;
  StationDialog     * stm;
  ObjectDialog      * objm;
  TrajectoryDialog  * trajm;
  AnnotationDialog  * annom;
  MeasurementsDialog   * measurementsm;
  HelpDialog        * help;
  EditItems::ToolBar * editDrawingToolBar;

  bool                markTrajPos; //left mouse click -> mark trajectory position
  bool                markMeasurementsPos; //left mouse click -> mark measurement position

  VprofWindow       * vpWindow;
  std::unique_ptr<VcrossInterface> vcInterface;
  bool vcrossEditManagerConnected;

  SpectrumWindow    * spWindow;
  std::map<std::string,InfoFile> infoFiles;

  ExportImageDialog* exportImageDialog_;

  // statusbar widgets
  StatusGeopos      * sgeopos;
  StatusPlotButtons * statusbuttons;
  ShowSatValues     * showsatval;
  QLabel            * archiveL;
  ClientSelection   * pluginB;

  Controller* contr;

  std::unique_ptr<DianaImageSource> imageSource_;

  bool          showelem;    ///> show on/off buttons
  bool          autoselect;  ///> autoselect element on mousemove

  //QSocket
  TextView *textview;
  bool qsocket;
  bool handlingTimeMessage;
  std::map<int,bool> autoredraw;

  void levelChange(int increment, int axis);

  std::string getLogFileName() const;
  void readLogFile();
  std::string saveDocState();
  void restoreDocState(const std::string& logstr);

  std::vector<std::string> writeLog(const std::string& thisVersion,
      const std::string& thisBuild);
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, std::string& logVersion);
  void getDisplaySize();
  void vprofStartup();
  void spectrumStartup();
  void getPlotStrings(PlotCommand_cpv &pstr, std::vector<std::string> &shortnames);

  std::map<QAction*, DataDialog*> dialogs;
  std::map<std::string, DataDialog*> dialogNames;

  static DianaMainWindow *self;   // singleton instance pointer
};

#endif
