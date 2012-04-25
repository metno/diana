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
#ifndef _qt_mainwindow_
#define _qt_mainwindow_

#include <QMainWindow>
#include <QTimerEvent>
#include <QLabel>
#include <QFocusEvent>
#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diMapMode.h>
#include <diField/diRectangle.h>

#ifdef VIDEO_EXPORT
#include <MovieMaker.h>
#endif

#include <puTools/miString.h>
#include <vector>
#include <deque>

using namespace std;

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
class ObsDialog;
class SatDialog;
class StationDialog;
class MapDialog;
class ObjectDialog;
class TrajectoryDialog;
class MeasurementsDialog;
class UffdaDialog;
class MailDialog;
class DianaProfetGUI;
class PaintToolBar;

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
class VcrossWindow;
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
		  const miutil::miString ver_str,
		  const miutil::miString buils_str,
		  const miutil::miString dianaTitle="Diana",
		  bool profetEnabled=false);
  ~DianaMainWindow();

  /// check if news file has changed since last startup
  void checkNews();
  void start();

protected:
  void timerEvent(QTimerEvent*);
  void setTimeLabel();
  void stopAnimation();
  void focusInEvent ( QFocusEvent * );
  void closeEvent( QCloseEvent* );

  bool event(QEvent* event);


public slots:
  void toggleToolBar();

private slots:
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
  void vcrossMenu();
  void spectrumMenu();
  void trajMenu();
  void measurementsMenu();
  void quickMenu();
  void toggleProfetGUI();

  void showHelp();
  void showAccels();
  void showNews();
  void showUrl();

  void about();
  void quickMenuApply(const vector<miutil::miString>&);
  void recallPlot(const vector<miutil::miString>&,bool);
  void resetArea();
  void resetAll();
  void editApply();
  void MenuOK();
  void updateQuickMenuHistory(const miutil::miString plotname, const vector<miutil::miString>&);
  void trajPositions(bool);
  void measurementsPositions(bool);
  void vCrossPositions(bool);
  void catchMouseGridPos(const mouseEvent);
  void catchMouseRightPos(const mouseEvent);
  void catchMouseMovePos(const mouseEvent,bool);
  void catchKeyPress(const keyboardEvent);
  void catchMouseDoubleClick(const mouseEvent);
  void catchElement(const mouseEvent);
  void sendSelectedStations(const miutil::miString& command);
  void undo();
  void redo();
  void save();
  void hardcopy();
  void saveraster();
  void saveRasterImage(QString filename);
  void emailPicture();
  void saveAnimation();
  void makeEPS(const miutil::miString& filename);
  void TimeSliderMoved();
  void TimeSelected();
  void setPlotTime(miutil::miTime& t);
  void SliderSet();
  void editUpdate();

  void toggleDialogs();
  void toggleStatusBar();
  void filequit();

  void info_activated(QAction *);
  void hideVprofWindow();
  void hideVcrossWindow();
  void hideSpectrumWindow();
  void stationChangedSlot(const QString&);
  void modelChangedSlot();
  void crossectionChangedSlot(const QString& name);
  void crossectionSetChangedSlot();
  void crossectionSetUpdateSlot();
  void spectrumChangedSlot(const QString& name);
  void spectrumSetChangedSlot();

  void connectionClosed();
  void processLetter(miMessage&);
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
  void togglePaintMode();
  void setPaintMode(bool);
  void plotProfetMap(bool objectsOnly);
  void forceProfetDisconnect(bool disableGuiOnly);
  void getFieldPlotOptions(map< miutil::miString, map<miutil::miString,miutil::miString> >&);

private:
  bool enableProfet;
  bool push_command;   // push current plot on stack
  bool browsing;       // user browsing through plot-stack
  BrowserBox* browser; // shows plot-stack
  bool updateBrowser();// update browser-window
  bool uffda;
  bool doAutoUpdate;

  miutil::miString version_string;
  miutil::miString build_string;
  int displayWidth,displayHeight;

  /// Actions
  QAction * fileSavePictAction;
  QAction * emailPictureAction;
  QAction * saveAnimationAction;
  QAction * filePrintAction;
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
  QAction * showMeasurementsDialogAction;
  QAction * showProfilesDialogAction;
  QAction * showCrossSectionDialogAction;
  QAction * showWaveSpectrumDialogAction;
  QAction * zoomOutAction;
  QAction * showUffdaDialogAction;
  QShortcut * uffdaAction;

//  QAction * togglePaintModeAction;
  QAction * toggleProfetGUIAction;

  QAction * helpDocAction;
  QAction * helpAccelAction;
  QAction * helpNewsAction;
  QAction * helpBugAction;
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
  // Error dialog for profet login
  QErrorMessage * profetLoginError;

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
  MeasurementsDialog   * measurementsm;
  UffdaDialog       * uffm;
  MailDialog        * mailm;
  HelpDialog        * help;
//  EditTimeDialog    * editTimeDialog;
  DianaProfetGUI	* profetGUI;
  PaintToolBar	    * paintToolBar;

  bool                markTrajPos; //left mouse click -> mark trajectory position
  bool                markMeasurementsPos; //left mouse click -> mark measurement position
  bool                markVcross; //left mouse click -> mark Vcross position

  VprofWindow       * vpWindow;
  VcrossWindow      * vcWindow;
  SpectrumWindow    * spWindow;
  map<miutil::miString,InfoFile>    infoFiles;

  // statusbar widgets
  QLabel            * smsg;
  StatusGeopos      * sgeopos;
  StatusPlotButtons * statusbuttons;
  ShowSatValues     * showsatval;
  QPushButton       * obsUpdateB;
  QLabel            * archiveL;
  ClientButton      * pluginB;
  bool                dialogChanged;

  vector<miutil::miString> vlabel;
  QButton* mwhelp;
  Controller* contr;
  miutil::miString lastString;

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

  vector <selectArea> vselectAreas; //selected areas for rightclickmenu

  //QSocket
  int textview_id;
  TextView *textview;
  int hqcTo;
  bool qsocket;
  map<int,bool> autoredraw;
  StationPlot *stationPlot;

  void levelChange(int increment);
  void idnumChange(int increment);

  void writeLogFile();
  void readLogFile();
  miutil::miString saveDocState();
  void restoreDocState(miutil::miString logstr);

  vector<miutil::miString> writeLog(const miutil::miString& thisVersion,
			    const miutil::miString& thisBuild);
  void readLog(const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion,
	       miutil::miString& logVersion);
  void getDisplaySize();
  void timeChanged();
  void satFileListUpdate();
  void vprofStartup();
  void vcrossStartup();
  void spectrumStartup();
  void getPlotStrings(vector<miutil::miString> &pstr,
                      vector<miutil::miString> &diagstr,
                      vector<miutil::miString> &shortnames);

// Profet methods
  bool initProfet();
  bool profetConnect();
  void profetDisconnect();
  bool ProfetUpdatePlot(const miutil::miTime& t);
  bool ProfetRightMouseClicked(float map_x, float map_y, int globalX, int globalY);
};


#endif
