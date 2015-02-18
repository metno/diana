/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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

#include "diCommonTypes.h"
#include "diMapMode.h"
#include "diPrintOptions.h"
#include <EditItems/toolbar.h>

#include <QMainWindow>
#include <QTimerEvent>
#include <QLabel>
#include <QFocusEvent>

#include <diField/diRectangle.h>

#ifdef USE_PAINTGL
#include <QPrintPreviewDialog>
#endif

#ifdef VIDEO_EXPORT
#include <MovieMaker.h>
#endif

#include <vector>
#include <deque>

class QMenuBar;
class QToolBar;
class QToolButton;
class QMenu;
class QLabel;
class QAction;
class QShortcut;
class QPrinter;
class QPushButton;
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
class UffdaDialog;
class MailDialog;

class DataDialog;
class EditDialog;
class Controller;
class TimeSlider;
class TimeStepSpinbox;
class TimeControl;
class StatusGeopos;
class HelpDialog;
class QButton;
class ShowSatValues;
class VprofWindow;
class VcrossInterface;
class SpectrumWindow;
class StatusPlotButtons;
class BrowserBox;
class ClientButton;
class miMessage;
class StationPlot;
class TextView;
class QShortCut;
class QErrorMessage;

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
  DianaMainWindow(Controller*,
		  const std::string& ver_str,
		  const std::string& buils_str,
		  const std::string& dianaTitle="Diana");
  ~DianaMainWindow();

  /// check if news file has changed since last startup
  void checkNews();
  void start();

  /// Adds a dialog for use with a manager.
  void addDialog(DataDialog *dialog);

  /// Returns the application's main window instance.
  static DianaMainWindow *instance();

protected:
  void timerEvent(QTimerEvent*);
  void setTimeLabel();
  void stopAnimation();
  void focusInEvent ( QFocusEvent * );
  void closeEvent( QCloseEvent* );
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);
  bool event(QEvent* event);

public Q_SLOTS:
  void toggleToolBar();
  void updateDialog();

private Q_SLOTS:
  void timecontrolslot();
  void timeoutChanged(float value);
  void animation();
  void animationBack();
  void animationStop();
  void animationLoop();
  void stepforward();
  void stepback();
  void decreaseTimeStep();
  void increaseTimeStep();
  void levelUp();
  void levelDown();
  void idnumUp();
  void idnumDown();
  void mapMenu();
  void editMenu();
  void obsMenu();
  void satMenu();
  void stationMenu();
  void uffMenu();
  void fieldMenu();
  void objMenu();
  void vprofMenu();
  void spectrumMenu();
  void trajMenu();
  void AnnotationMenu();
  void measurementsMenu();
  void quickMenu();

  void showHelp();
  void showAccels();
  void showNews();
  void showUrl();

  void about();
  void quickMenuApply(const std::vector<std::string>&);
  void recallPlot(const std::vector<std::string>&, bool);
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
  void sendSelectedStations(const std::string& command);
  void undo();
  void redo();
  void save();
  void parseSetup();
  void hardcopy();
  void previewHardcopy();
  void saveraster();
  void saveRasterImage(QString filename);
  void emailPicture();
  void saveAnimation();
  void makeEPS(const std::string& filename);
  void TimeSliderMoved();
  void TimeSelected();
  void setPlotTime(miutil::miTime& t);
  void SliderSet();
  void editUpdate(bool = true);
  void handleEIMEditing(bool);
  void toggleEIMTestDialog();

  void toggleDialogs();
  void toggleStatusBar();
  void filequit();

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
  void updateVcrossQuickMenuHistory(const std::string& plotname, const std::vector<std::string>&);
  void onVcrossRequestLoadCrossectionsFile(const QStringList& filenames);
  void onVcrossRequestEditManager(bool on);

  void spectrumChangedSlot(const QString& name);
  void spectrumSetChangedSlot();

  void connectionClosed();
  void processLetter(const miMessage&);
  void sendLetter(miMessage&);

  void updateObs();
  void autoUpdate();
  void updateGLSlot();
  void showElements();
  void archiveMode();
  void autoElement();
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

  void zoomTo(Rectangle);
  void zoomOut();
  void showUffda();
  void selectedAreas();

  void winResize(int, int);
  void inEdit(bool);
  void sendPrintClicked(int);
  void toggleEditDrawingMode();
  void setEditDrawingMode(bool);
  void editDrawingToolBarVisible(bool);
  void getFieldPlotOptions(std::map< std::string, std::map<std::string,std::string> >&);

  void setWorkAreaCursor(const QCursor &);
  void unsetWorkAreaCursor();

  void updatePlotElements();

private:
  void vcrossEditManagerEnableSignals();

private:
  bool push_command;   // push current plot on stack
  bool browsing;       // user browsing through plot-stack
  BrowserBox* browser; // shows plot-stack
  bool updateBrowser();// update browser-window
  bool uffda;
  bool doAutoUpdate;

  std::string version_string;
  std::string build_string;
  int displayWidth,displayHeight;

  /// Actions
  QAction * fileSavePictAction;
  QAction * emailPictureAction;
  QAction * saveAnimationAction;
  QAction * filePrintAction;
  QAction * filePrintPreviewAction;
  QAction * readSetupAction;
  QAction * fileQuitAction;

  QAction * optOnOffAction;
  QAction * optArchiveAction;
  QAction * optAutoElementAction;
  QAction * optAnnotationAction;
  QAction * optScrollwheelZoomAction;
  QAction * optFontAction;

  QAction * showResetAreaAction;
  QAction * showResetAllAction;
  QAction * showApplyAction;
  QAction * showAddQuickAction;
  QAction * showPrevPlotAction;
  QAction * showNextPlotAction;
  QAction * showHideAllAction;
  QAction * showQuickmenuAction;
  QAction * showMapDialogAction;
  QAction * showFieldDialogAction;
  QAction * showObsDialogAction;
  QAction * showSatDialogAction;
  QAction * showStationDialogAction;
  QAction * showEditDialogAction;
  QAction * showObjectDialogAction;
  QAction * showTrajecDialogAction;
  QAction * showAnnotationDialogAction;
  QAction * showMeasurementsDialogAction;
  QAction * showProfilesDialogAction;
  QAction * showCrossSectionDialogAction;
  QAction * showWaveSpectrumDialogAction;
  QAction * zoomOutAction;
  QAction * showUffdaDialogAction;
  QShortcut * uffdaAction;

  QAction * toggleEditDrawingModeAction;

  QAction * helpDocAction;
  QAction * helpAccelAction;
  QAction * helpNewsAction;
  QAction * helpTestAction;
  QAction * helpAboutAction;

  QAction * timeBackwardAction;
  QAction * timeForewardAction;
  QAction * timeStepBackwardAction;
  QAction * timeStepForewardAction;
  QAction * timeStopAction;
  QAction * timeLoopAction;
  QAction * timeControlAction;

  QAction * toolLevelUpAction;
  QAction * toolLevelDownAction;

  QAction * toolIdnumUpAction;
  QAction * toolIdnumDownAction;

  QAction * obsUpdateAction;
  QAction * autoUpdateAction;

  QAction * undoAction;
  QAction * redoAction;
  QAction * saveAction;

  enum { MaxSelectedAreas = 5 };
  QAction * selectAreaAction[MaxSelectedAreas];

  QShortcut * leftBrowsingAction;
  QShortcut * rightBrowsingAction;
  QShortcut * upBrowsingAction;
  QShortcut * downBrowsingAction;
  QShortcut * timeStepUpAction;
  QShortcut * timeStepDownAction;


  QMenuBar   * mainmenubar;

  QMenu * filemenu;
  QMenu * optmenu;
  QMenu * showmenu;
  QMenu * helpmenu;

  QMenu * rightclickmenu;

  QToolBar * menuToolbar;
  QToolBar * mainToolbar;
  QToolBar * timerToolbar;
  QToolBar * timeSliderToolbar;
  QToolBar * levelToolbar;

  // printerdefinitions
  printerManager pman;
  printOptions priop;

  QToolButton * infoB;

  WorkArea          * w;
  QuickMenu         * qm;
  MapDialog         * mm;
  EditDialog        * em;
  FieldDialog       * fm;
  ObsDialog         * om;
  SatDialog         * sm;
  StationDialog     * stm;
  ObjectDialog      * objm;
  TrajectoryDialog  * trajm;
  AnnotationDialog  * annom;
  MeasurementsDialog   * measurementsm;
  UffdaDialog       * uffm;
  MailDialog        * mailm;
  HelpDialog        * help;
//  EditTimeDialog    * editTimeDialog;
  EditItems::ToolBar * editDrawingToolBar;

  bool                markTrajPos; //left mouse click -> mark trajectory position
  bool                markMeasurementsPos; //left mouse click -> mark measurement position

  VprofWindow       * vpWindow;
  std::auto_ptr<VcrossInterface> vcInterface;
  bool vcrossEditManagerConnected;

  SpectrumWindow    * spWindow;
  std::map<std::string,InfoFile> infoFiles;

  // statusbar widgets
  QLabel            * smsg;
  StatusGeopos      * sgeopos;
  StatusPlotButtons * statusbuttons;
  ShowSatValues     * showsatval;
  QPushButton       * obsUpdateB;
  QLabel            * archiveL;
  ClientButton      * pluginB;
  bool                dialogChanged;

  QButton* mwhelp;
  Controller* contr;
  std::string lastString;

  // timer types
  int timeron;               ///> timer is turned on
  int animationTimer;        ///> the main timer id
  int timeout_ms;            ///> animation timeout in millisecs
  bool timeloop;             ///> animation in loop
  TimeSlider *tslider;       ///> the time slider
  TimeStepSpinbox *timestep; ///> the timestep widget
  TimeControl* timecontrol;  ///> time control dialog
  QLabel *timelabel;         ///> showing current time


  bool          showelem;    ///> show on/off buttons
  bool          autoselect;  ///> autoselect element on mousemove


  // x,y position of right mouse click or move
  int xclick,yclick;
  // x,y position of last uffda action
  int xlast,ylast;

  std::vector <selectArea> vselectAreas; //selected areas for rightclickmenu

  //QSocket
  int textview_id;
  TextView *textview;
  int hqcTo;
  bool qsocket;
  std::map<int,bool> autoredraw;
  StationPlot *stationPlot;

  void levelChange(int increment);
  void idnumChange(int increment);

  void writeLogFile();
  void readLogFile();
  std::string saveDocState();
  void restoreDocState(std::string logstr);

  std::vector<std::string> writeLog(const std::string& thisVersion,
      const std::string& thisBuild);
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, std::string& logVersion);
  void getDisplaySize();
  void timeChanged();
  void satFileListUpdate();
  void vprofStartup();
  void spectrumStartup();
  void getPlotStrings(std::vector<std::string> &pstr,
                      std::vector<std::string> &diagstr,
                      std::vector<std::string> &shortnames);

  std::map<QAction*, DataDialog*> dialogs;
  std::map<std::string, DataDialog*> dialogNames;

  std::vector<PlotElement> getPlotElements() const;

  static DianaMainWindow *self;   // singleton instance pointer
};

#endif
