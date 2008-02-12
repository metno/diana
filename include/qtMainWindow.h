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

#include <q3mainwindow.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <QLabel>
#include <Q3PopupMenu>
#include <QFocusEvent>
#include <diCommonTypes.h>
#include <diPrintOptions.h>
#include <diMapMode.h>
#include <miString.h>
#include <vector>
#include <deque>

// qt4 fix
#include <Q3Accel>

using namespace std;

class QMenuBar;
class Q3ToolBar;
class QToolButton;
class Q3PopupMenu;
class QLabel;
class Q3Action;
class QPrinter;
class QPushButton;
class WorkArea;
class QuickMenu;
class FieldDialog;
class ObsDialog;
class SatDialog;
class MapDialog;
class ObjectDialog;
class TrajectoryDialog;
class UffdaDialog;
//class EditTimeDialog;
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
class ClientButton2;
class miMessage;
class StationPlot;
class TextView;

/**

  \brief Main application window

  The main application window containing:
  - map area
  - toolbars and menues

*/

class DianaMainWindow: public Q3MainWindow
{
  Q_OBJECT
public:
  DianaMainWindow(Controller*, 
		  const miString ver_str,
		  const miString buils_str,
		  bool profetEnabled=false);
  ~DianaMainWindow();

  /// check if news file has changed since last startup
  void checkNews();
  void start();
  bool close(bool alsoDelete);

protected:
  void timerEvent(QTimerEvent*);
  void setTimeLabel();
  void stopAnimation();
  void focusInEvent ( QFocusEvent * );


public slots:
  void toggleMenuBar();
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
  void uffMenu();
  void fieldMenu();
  void objMenu();
  void vprofMenu();
  void vcrossMenu();
  void spectrumMenu();
  void trajMenu();
  void quickMenu();
  void toggleProfetGUI();

  void showHelp();
  void showAccels();
  void showNews();

  void about();
  void quickMenuApply(const vector<miString>&);
  void recallPlot(const vector<miString>&,bool);
  void resetArea();
  void resetAll();
  void editApply();
  void MenuOK();
  void trajPositions(bool);
  void catchMouseGridPos(const mouseEvent);
  void catchMouseRightPos(const mouseEvent);
  void catchMouseMovePos(const mouseEvent,bool);
  void catchKeyPress(const keyboardEvent);
  void catchElement(const mouseEvent);
  void sendSelectedStations(const miString& command);
  void undo();
  void redo();
  void save();
  void hardcopy();
  void saveraster();
  void makeEPS(const miString& filename);
  void TimeChanged();
  void TimeSelected();
  void SliderSet();
  void editUpdate();

  void toggleDialogs();
  void toggleStatusBar();
  void filequit();

  void info_activated(int id);
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
  void updateGLSlot();
  void xyGeoPos();
  void latlonGeoPos();
  void showElements();
  void archiveMode();
  void autoElement();
  void showAnnotations();
  void chooseFont();
  void toggleElement(PlotElement);
  void prevHPlot();
  void nextHPlot();
  void prevQPlot();
  void nextQPlot();
  void prevList();
  void nextList();
  void startBrowsing();
  void browserSelect();
  void browserCancel();
  void addToMenu();
  void requestQuickUpdate(const vector<miString>&,
			  vector<miString>&);

  void zoomOut();
  void showUffda();
  void fillRightclickmenu();
  void rightClickMenuActivated(int);
  void selectedAreas(int);

  void inEdit(bool);
  void sendPrintClicked(int);
  void togglePaintMode();
  void setPaintMode(bool);
  void plotProfetMap(bool objectsOnly);

private:
  bool enableProfet;
  bool push_command;   // push current plot on stack
  bool browsing;       // user browsing through plot-stack
  BrowserBox* browser; // shows plot-stack
  bool updateBrowser();// update browser-window
  bool uffda;

  void PrintPS(miString& filestr );
  void PrintPS(vector <miString>& filestr );

  miString version_string;
  miString build_string;
  int displayWidth,displayHeight;

  /// Actions
  Q3Action * fileSavePictAction;
  Q3Action * filePrintAction;
  Q3Action * fileQuitAction;

  Q3Action * optXYAction;
  Q3Action * optLatLonAction;
  Q3Action * optOnOffAction;
  Q3Action * optArchiveAction;
  Q3Action * optAutoElementAction;
  Q3Action * optAnnotationAction;
  Q3Action * optFontAction;

  Q3Action * showResetAreaAction;
  Q3Action * showResetAllAction;
  Q3Action * showApplyAction;
  Q3Action * showAddQuickAction;
  Q3Action * showPrevPlotAction;
  Q3Action * showNextPlotAction;
  Q3Action * showHideAllAction;
  Q3Action * showQuickmenuAction;
  Q3Action * showMapDialogAction;
  Q3Action * showFieldDialogAction;
  Q3Action * showObsDialogAction;
  Q3Action * showSatDialogAction;
  Q3Action * showEditDialogAction;
  Q3Action * showObjectDialogAction;
  Q3Action * showTrajecDialogAction;
  Q3Action * showProfilesDialogAction;
  Q3Action * showCrossSectionDialogAction;
  Q3Action * showWaveSpectrumDialogAction;
  Q3Action * showUffdaDialogAction;
  Q3Action * togglePaintModeAction;
  Q3Action * toggleProfetGUIAction;

  Q3Action * helpDocAction;
  Q3Action * helpAccelAction;
  Q3Action * helpNewsAction;
  Q3Action * helpAboutAction;

  Q3Action * timeBackwardAction;
  Q3Action * timeForewardAction;
  Q3Action * timeStepBackwardAction;
  Q3Action * timeStepForewardAction;
  Q3Action * timeStopAction;
  Q3Action * timeLoopAction;
  Q3Action * timeControlAction;

  Q3Action * toolLevelUpAction;
  Q3Action * toolLevelDownAction;

  Q3Action * toolIdnumUpAction;
  Q3Action * toolIdnumDownAction;

  QMenuBar   * mainmenubar;

  Q3PopupMenu * filemenu;
  Q3PopupMenu * optmenu;
  Q3PopupMenu * showmenu;
  Q3PopupMenu * helpmenu;

  Q3PopupMenu * rightclickmenu;

  Q3ToolBar * menuToolbar;
  Q3ToolBar * mainToolbar;
  Q3ToolBar * timerToolbar;

  // printerdefinitions
  printerManager pman;
  QPrinter *qprt;

  QToolButton * infoB;

  WorkArea          * w;
  QuickMenu         * qm;
  MapDialog         * mm;
  EditDialog        * em;
  FieldDialog       * fm;
  ObsDialog         * om;
  SatDialog         * sm;
  ObjectDialog      * objm;
  TrajectoryDialog  * trajm;
  UffdaDialog       * uffm;
  HelpDialog        * help;
//  EditTimeDialog    * editTimeDialog;
  DianaProfetGUI	* profetGUI;
  PaintToolBar	    * paintToolBar;
  
  bool                markTrajPos; //left mouse click -> mark trajectory position
  VprofWindow       * vpWindow;
  VcrossWindow      * vcWindow;
  SpectrumWindow    * spWindow;
  vector<InfoFile>    infoFiles;

  // statusbar widgets
  QLabel            * smsg;
  StatusGeopos      * sgeopos;
  StatusPlotButtons * statusbuttons;
  ShowSatValues     * showsatval;
  QPushButton       * obsUpdateB;
  QLabel            * archiveL;
  ClientButton2     * pluginB;
  bool                dialogChanged;

  vector<miString> vlabel;
  QButton* mwhelp;
  Controller* contr;
  miString lastString;

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

  struct rightclickMenuItem{
    QString menuText;
    char * member;
    bool checked;
    int param;
  };
  QString lastRightClicked;

  vector <rightclickMenuItem> vrightclickMenu;
  int rightsep; //pos of separator
  vector <selectArea> vselectAreas; //selected areas for rightclickmenu

  //QSocket
  int textview_id;
  TextView *textview;
  int hqcTo;
  bool qsocket;
  map<int,bool> autoredraw;
  StationPlot *stationPlot;

  vector<miString> levelList;
  miString levelSpec;
  int levelIndex;

  vector<miString> idnumList;
  miString idnumSpec;
  int idnumIndex;

  void levelChange(int increment);
  void idnumChange(int increment);

  void writeLogFile();
  void readLogFile();

  vector<miString> writeLog(const miString& thisVersion,
			    const miString& thisBuild);
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion,
	       miString& logVersion);
  void getDisplaySize();
  void timeChanged();
  void satFileListUpdate();
  void vprofStartup();
  void vcrossStartup();
  void spectrumStartup();
  void defineShortcuts(Q3Accel*);
  bool initProfet();
};


#endif
