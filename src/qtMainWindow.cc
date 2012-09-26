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
//#define DEBUGREDRAW
//#define DEBUGREDRAWCATCH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>

#include <sys/types.h>
#include <sys/time.h>

#include "qtTimeSlider.h"
#include "qtTimeControl.h"
#include "qtTimeStepSpinbox.h"
#include "qtStatusGeopos.h"
#include "qtStatusPlotButtons.h"
#include "qtShowSatValues.h"
#include "qtTextDialog.h"
#include "qtImageGallery.h"

#include <QFileDialog>
#include <QPrintDialog>
#include <QPrinter>
#include <QApplication>
#include <QTimerEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

#include <puCtools/puCglob.h>
#include <qmotifstyle.h>
#include <qwindowsstyle.h>

#include <QAction>
#include <QShortcut>
#include <QApplication>
#include <QDesktopWidget>
#include <QDateTime>

#include <qpushbutton.h>
#include <qpixmap.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <QMenu>
#include <qmenubar.h>
#include <qnamespace.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qfontdialog.h>
#include <qtooltip.h>
#include <QProgressDialog>

#include "qtMainWindow.h"
#include "qtWorkArea.h"
#include "qtVprofWindow.h"
#include "qtVcrossWindow.h"
#include "qtSpectrumWindow.h"
#include "diController.h"
#include "diPrintOptions.h"
#include "diLocalSetupParser.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "diLocationPlot.h"

#include "qtQuickMenu.h"
#include "qtObsDialog.h"
#include "qtSatDialog.h"
#include "qtStationDialog.h"
#include "qtMapDialog.h"
#include "qtFieldDialog.h"
#include "qtEditDialog.h"
#include "qtObjectDialog.h"
#include "qtTrajectoryDialog.h"
#include "qtMeasurementsDialog.h"
#include "qUtilities/qtHelpDialog.h"
#include "qtSetupDialog.h"
#include "qtPrintManager.h"
#include "qtBrowserBox.h"
#include "qtAddtoMenu.h"
#include "qtUffdaDialog.h"
#include <qUtilities/ClientButton.h>
#include "qtTextView.h"
#include <qUtilities/miMessage.h>
#include <qUtilities/QLetterCommands.h>
#include <puDatatypes/miCoordinates.h>
#include <puTools/miCommandLine.h>
#include "qtPaintToolBar.h"
#include "diGridAreaManager.h"
#include <QErrorMessage>

#include "qtMailDialog.h"

#ifdef PROFET
#include "qtDianaProfetGUI.h"
#include <profet/LoginDialog.h>
#include <profet/ProfetCommon.h>
#endif
#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>
#include <qUtilities/miLogFile.h>

#include <diana_icon.xpm>
#include <pick.xpm>
#include <earth3.xpm>
//#include <fileprint.xpm>
//#include <question.xpm>
#include <forover.xpm>
#include <bakover.xpm>
#include <start.xpm>
#include <stopp.xpm>
#include <slutt.xpm>
#include <thumbs_up.xpm>
#include <thumbs_down.xpm>
#include <loop.xpm>
#include <synop.xpm>
#include <synop_red.xpm>
#include <felt.xpm>
#include <Tool_32_draw.xpm>
#include <sat.xpm>
#include <station.xpm>
#include <clock.xpm>
#include <levelUp.xpm>
#include <levelDown.xpm>
#include <idnumUp.xpm>
#include <idnumDown.xpm>
#include <front.xpm>
#include <balloon.xpm>
#include <vcross.xpm>
#include <spectrum.xpm>
#include <traj.xpm>
#include <ruler.xpm>
#include <info.xpm>
#include <profet.xpm>
//#include <paint_mode.xpm>
#include <autoupdate.xpm>

using namespace milogger;

DianaMainWindow::DianaMainWindow(Controller *co,
    const miutil::miString ver_str,
    const miutil::miString build_str,
    miutil::miString dianaTitle,
    bool ep)
: QMainWindow(),
  enableProfet(ep), push_command(true),browsing(false),
  profetGUI(0),markTrajPos(false), markMeasurementsPos(false), markVcross(false),
  vpWindow(0), vcWindow(0), spWindow(0),contr(co),
  timeron(0),timeout_ms(100),timeloop(false),showelem(true), autoselect(false)
{
  cerr << "Creating DianaMainWindow" << endl;

  version_string = ver_str;
  build_string = build_str;

  setWindowIcon(QIcon(diana_icon_xpm));
  setWindowTitle(tr(dianaTitle.cStr()));


  //-------- The Actions ---------------------------------

  // file ========================
  // --------------------------------------------------------------------
  fileSavePictAction = new QAction( tr("&Save picture..."),this );
  fileSavePictAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( fileSavePictAction, SIGNAL( triggered() ) , SLOT( saveraster() ) );
  // --------------------------------------------------------------------
  emailPictureAction = new QAction( tr("&Email picture..."),this );
  emailPictureAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( emailPictureAction, SIGNAL( triggered() ) , SLOT( emailPicture() ) );
  // --------------------------------------------------------------------
  saveAnimationAction = new QAction( tr("Save &animation..."),this );
  saveAnimationAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( saveAnimationAction, SIGNAL( triggered() ) , SLOT( saveAnimation() ) );
  // --------------------------------------------------------------------
  filePrintAction = new QAction( tr("&Print..."),this );
  filePrintAction->setShortcut(Qt::CTRL+Qt::Key_P);
  filePrintAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( filePrintAction, SIGNAL( triggered() ) , SLOT( hardcopy() ) );
  // --------------------------------------------------------------------
  // --------------------------------------------------------------------
  readSetupAction = new QAction( tr("Read setupfile"),this );
  //restartAction->setShortcut(Qt::CTRL+Qt::Key_P);
  readSetupAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( readSetupAction, SIGNAL( triggered() ) , SLOT( parseSetup() ) );
  // --------------------------------------------------------------------
  fileQuitAction = new QAction( tr("&Quit..."), this );
  fileQuitAction->setShortcut(Qt::CTRL+Qt::Key_Q);
  fileQuitAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( fileQuitAction, SIGNAL( triggered() ) , SLOT( filequit() ) );


  // options ======================
  // --------------------------------------------------------------------
  optOnOffAction = new QAction( tr("S&peed buttons"), this );
  optOnOffAction->setShortcutContext(Qt::ApplicationShortcut);
  optOnOffAction->setCheckable(true);
  connect( optOnOffAction, SIGNAL( triggered() ) ,  SLOT( showElements() ) );
  // --------------------------------------------------------------------
  optArchiveAction = new QAction( tr("A&rchive mode"), this );
  optArchiveAction->setShortcutContext(Qt::ApplicationShortcut);
  optArchiveAction->setCheckable(true);
  connect( optArchiveAction, SIGNAL( triggered() ) ,  SLOT( archiveMode() ) );
  // --------------------------------------------------------------------
  optAutoElementAction = new QAction( tr("&Automatic element choice"), this );
  optAutoElementAction->setShortcutContext(Qt::ApplicationShortcut);
  optAutoElementAction->setShortcut(Qt::Key_Space);
  optAutoElementAction->setCheckable(true);
  connect( optAutoElementAction, SIGNAL( triggered() ), SLOT( autoElement() ) );
  // --------------------------------------------------------------------
  optAnnotationAction = new QAction( tr("A&nnotations"), this );
  optAnnotationAction->setShortcutContext(Qt::ApplicationShortcut);
  optAnnotationAction->setCheckable(true);
  connect( optAnnotationAction, SIGNAL( triggered() ), SLOT( showAnnotations() ) );
  // --------------------------------------------------------------------
  optScrollwheelZoomAction = new QAction( tr("Scrollw&heel zooming"), this );
  optScrollwheelZoomAction->setShortcutContext(Qt::ApplicationShortcut);
  optScrollwheelZoomAction->setCheckable(true);
  connect( optScrollwheelZoomAction, SIGNAL( triggered() ), SLOT( toggleScrollwheelZoom() ) );
  // --------------------------------------------------------------------
  optFontAction = new QAction( tr("Select &Font..."), this );
  optFontAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( optFontAction, SIGNAL( triggered() ) ,  SLOT( chooseFont() ) );


  // show ======================
  // --------------------------------------------------------------------
  showResetAreaAction = new QAction( QPixmap(thumbs_up_xpm),tr("Reset area and replot"), this );
  showResetAreaAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( showResetAreaAction, SIGNAL( triggered() ) ,  SLOT( resetArea() ) );
  //----------------------------------------------------------------------
  showResetAllAction = new QAction( QPixmap(thumbs_down_xpm),tr("Reset all"), this );
  showResetAllAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( showResetAllAction, SIGNAL( triggered() ) ,  SLOT( resetAll() ) );
  // --------------------------------------------------------------------
  showApplyAction = new QAction( tr("&Apply plot"), this );
  showApplyAction->setShortcutContext(Qt::ApplicationShortcut);
  showApplyAction->setShortcut(Qt::CTRL+Qt::Key_U);
  connect( showApplyAction, SIGNAL( triggered() ) ,  SLOT( MenuOK() ) );
  // --------------------------------------------------------------------
  showAddQuickAction = new QAction( tr("Add to q&uickmenu"), this );
  showAddQuickAction->setShortcutContext(Qt::ApplicationShortcut);
  showAddQuickAction->setShortcut(Qt::Key_F9);
  connect( showAddQuickAction, SIGNAL( triggered() ) ,  SLOT( addToMenu() ) );
  // --------------------------------------------------------------------
  showPrevPlotAction = new QAction( tr("P&revious plot"), this );
//  showPrevPlotAction->setShortcutContext(Qt::ApplicationShortcut);
  showPrevPlotAction->setShortcut(Qt::Key_F10);
  connect( showPrevPlotAction, SIGNAL( triggered() ) ,  SLOT( prevHPlot() ) );
  // --------------------------------------------------------------------
  showNextPlotAction = new QAction( tr("&Next plot"), this );
  //showNextPlotAction->setShortcutContext(Qt::ApplicationShortcut);
  showNextPlotAction->setShortcut(Qt::Key_F11);
  connect( showNextPlotAction, SIGNAL( triggered() ) ,  SLOT( nextHPlot() ) );
  // --------------------------------------------------------------------
  showHideAllAction = new QAction( tr("&Hide All"), this );
  showHideAllAction->setShortcutContext(Qt::ApplicationShortcut);
  showHideAllAction->setShortcut(Qt::CTRL+Qt::Key_D);
  showHideAllAction->setCheckable(true);
  connect( showHideAllAction, SIGNAL( triggered() ) ,  SLOT( toggleDialogs() ) );

  // --------------------------------------------------------------------
  showQuickmenuAction = new QAction( QPixmap(pick_xpm ),tr("&Quickmenu"), this );
  showQuickmenuAction->setShortcutContext(Qt::ApplicationShortcut);
  showQuickmenuAction->setShortcut(Qt::Key_F12);
  showQuickmenuAction->setCheckable(true);
  connect( showQuickmenuAction, SIGNAL( triggered() ) ,  SLOT( quickMenu() ) );
  // --------------------------------------------------------------------
  showMapDialogAction = new QAction( QPixmap(earth3_xpm ),tr("Maps"), this );
  showMapDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showMapDialogAction->setShortcut(Qt::ALT+Qt::Key_K);
  showMapDialogAction->setCheckable(true);
  connect( showMapDialogAction, SIGNAL( triggered() ) ,  SLOT( mapMenu() ) );
  // --------------------------------------------------------------------
  showFieldDialogAction = new QAction( QPixmap(felt_xpm ),tr("&Fields"), this );
  showFieldDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showFieldDialogAction->setShortcut(Qt::ALT+Qt::Key_F);
  showFieldDialogAction->setCheckable(true);
  connect( showFieldDialogAction, SIGNAL( triggered() ) ,  SLOT( fieldMenu() ) );
  // --------------------------------------------------------------------
  showObsDialogAction = new QAction( QPixmap(synop_xpm ),tr("&Observations"), this );
  showObsDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showObsDialogAction->setShortcut(Qt::ALT+Qt::Key_O);
  showObsDialogAction->setCheckable(true);
  connect( showObsDialogAction, SIGNAL( triggered() ) ,  SLOT( obsMenu() ) );
  // --------------------------------------------------------------------
  showSatDialogAction = new QAction( QPixmap(sat_xpm ),tr("&Satellites and Radar"), this );
  showSatDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showSatDialogAction->setShortcut(Qt::ALT+Qt::Key_S);
  showSatDialogAction->setCheckable(true);
  connect( showSatDialogAction, SIGNAL( triggered() ) ,  SLOT( satMenu() ) );
  // --------------------------------------------------------------------
  showStationDialogAction = new QAction( QPixmap(station_xpm), tr("&Stations"), this );
  showStationDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  //showStationDialogAction->setShortcut(Qt::ALT+Qt::Key_S);
  showStationDialogAction->setCheckable(true);
  connect(showStationDialogAction, SIGNAL(triggered()), SLOT(stationMenu()));
  // --------------------------------------------------------------------
  showEditDialogAction = new QAction( QPixmap(editmode_xpm ),tr("&Product Editing"), this );
  showEditDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showEditDialogAction->setShortcut(Qt::ALT+Qt::Key_E);
  showEditDialogAction->setCheckable(true);
  connect( showEditDialogAction, SIGNAL( triggered() ) ,  SLOT( editMenu() ) );
  // --------------------------------------------------------------------
  showObjectDialogAction = new QAction( QPixmap(front_xpm ),tr("O&bjects"), this );
  showObjectDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showObjectDialogAction->setShortcut(Qt::ALT+Qt::Key_J);
  showObjectDialogAction->setCheckable(true);
  connect( showObjectDialogAction, SIGNAL( triggered() ) ,  SLOT( objMenu() ) );
  // --------------------------------------------------------------------
  showTrajecDialogAction = new QAction( QPixmap( traj_xpm),tr("&Trajectories"), this );
  showTrajecDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showTrajecDialogAction->setShortcut(Qt::ALT+Qt::Key_T);
  showTrajecDialogAction->setCheckable(true);
  connect( showTrajecDialogAction, SIGNAL( triggered() ) ,  SLOT( trajMenu() ) );
  // --------------------------------------------------------------------
  showProfilesDialogAction = new QAction( QPixmap(balloon_xpm ),tr("&Vertical Profiles"), this );
  showProfilesDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showProfilesDialogAction->setShortcut(Qt::ALT+Qt::Key_V);
  showProfilesDialogAction->setCheckable(false);
  connect( showProfilesDialogAction, SIGNAL( triggered() ) ,  SLOT( vprofMenu() ) );
  // --------------------------------------------------------------------
  showCrossSectionDialogAction = new QAction( QPixmap(vcross_xpm ),tr("Vertical &Cross sections"), this );
  showCrossSectionDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showCrossSectionDialogAction->setShortcut(Qt::ALT+Qt::Key_C);
  showCrossSectionDialogAction->setCheckable(false);
  connect( showCrossSectionDialogAction, SIGNAL( triggered() ) ,  SLOT( vcrossMenu() ) );
  // --------------------------------------------------------------------
  showWaveSpectrumDialogAction = new QAction( QPixmap(spectrum_xpm ),tr("&Wave spectra"), this );
  showWaveSpectrumDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showWaveSpectrumDialogAction->setShortcut(Qt::ALT+Qt::Key_W);
  showWaveSpectrumDialogAction->setCheckable(false);
  connect( showWaveSpectrumDialogAction, SIGNAL( triggered() ) ,  SLOT( spectrumMenu() ) );
  // --------------------------------------------------------------------
  zoomOutAction = new QAction( tr("Zoom out"), this );
  zoomOutAction->setVisible(false);
  connect( zoomOutAction, SIGNAL( triggered() ), SLOT( zoomOut() ) );
  // --------------------------------------------------------------------
  showUffdaDialogAction = new QAction( tr("&Uffda Service"), this );
  showUffdaDialogAction->setShortcutContext(Qt::ApplicationShortcut);
  showUffdaDialogAction->setShortcut(Qt::ALT+Qt::Key_U);
  showUffdaDialogAction->setCheckable(true);
  connect( showUffdaDialogAction, SIGNAL( triggered() ), SLOT( uffMenu() ) );
  // --------------------------------------------------------------------
     showMeasurementsDialogAction = new QAction( QPixmap( ruler),tr("&Measurements"), this );
     showMeasurementsDialogAction->setShortcutContext(Qt::ApplicationShortcut);
     showMeasurementsDialogAction->setShortcut(Qt::ALT+Qt::Key_M);
     showMeasurementsDialogAction->setCheckable(true);
     connect( showMeasurementsDialogAction, SIGNAL( triggered() ) ,  SLOT( measurementsMenu() ) );
  // ----------------------------------------------------------------
  uffdaAction = new QShortcut(Qt::CTRL+Qt::Key_X,this );
  connect( uffdaAction, SIGNAL( activated() ), SLOT( showUffda() ) );
  // ----------------------------------------------------------------

  profetLoginError = new QErrorMessage(this);
  /* Paint mode not implemented
  if(enableProfet){
    togglePaintModeAction = new QAction( QPixmap(paint_mode_xpm),tr("&Paint"), this );
  } else {
    togglePaintModeAction = new QAction( tr("&Paint"), this );
  }
  togglePaintModeAction->setShortcutContext(Qt::ApplicationShortcut);
  togglePaintModeAction->setShortcut(Qt::ALT+Qt::Key_P);
  togglePaintModeAction->setCheckable(true);
  connect( togglePaintModeAction, SIGNAL( toggled(bool) ), SLOT( togglePaintMode() ) );
   */
  // ----------------------------------------------------------------

  if(enableProfet) {
    toggleProfetGUIAction = new QAction( QPixmap(profet_xpm ),tr("Field E&dit"), this );
    toggleProfetGUIAction->setShortcut(Qt::ALT + Qt::Key_D);
  } else {
    toggleProfetGUIAction = new QAction( QPixmap(),tr("Field E&dit"), this );
  }
  toggleProfetGUIAction->setShortcutContext(Qt::ApplicationShortcut);
  toggleProfetGUIAction->setCheckable(true);
  connect( toggleProfetGUIAction, SIGNAL( triggered() ), SLOT( toggleProfetGUI()));

  // --------------------------------------------------------------------


  // help ======================
  // --------------------------------------------------------------------
  helpDocAction = new QAction( tr("Documentation"), this );
  helpDocAction->setShortcutContext(Qt::ApplicationShortcut);
  helpDocAction->setShortcut(Qt::Key_F1);
  helpDocAction->setCheckable(false);
  connect( helpDocAction, SIGNAL( triggered() ) ,  SLOT( showHelp() ) );
  // --------------------------------------------------------------------
  helpAccelAction = new QAction( tr("&Accelerators"), this );
  helpAccelAction->setShortcutContext(Qt::ApplicationShortcut);
  helpAccelAction->setCheckable(false);
  connect( helpAccelAction, SIGNAL( triggered() ) ,  SLOT( showAccels() ) );
  // --------------------------------------------------------------------
  helpNewsAction = new QAction( tr("&News"), this );
  helpNewsAction->setShortcutContext(Qt::ApplicationShortcut);
  helpNewsAction->setCheckable(false);
  connect( helpNewsAction, SIGNAL( triggered() ) ,  SLOT( showNews() ) );
  // --------------------------------------------------------------------
  helpBugAction = new QAction( tr("&Report bug"), this );
  helpBugAction->setShortcutContext(Qt::ApplicationShortcut);
  helpBugAction->setCheckable(false);
  connect( helpBugAction, SIGNAL( triggered() ) ,  SLOT( showUrl() ) );
  // --------------------------------------------------------------------
  helpAboutAction = new QAction( tr("About Diana"), this );
  helpAboutAction->setShortcutContext(Qt::ApplicationShortcut);
  helpAboutAction->setCheckable(false);
  connect( helpAboutAction, SIGNAL( triggered() ) ,  SLOT( about() ) );
  // --------------------------------------------------------------------


  // timecommands ======================
  // --------------------------------------------------------------------
  timeBackwardAction = new QAction( QPixmap(start_xpm ),tr("Run Backwards"), this);
  timeBackwardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeBackwardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Left);
  timeBackwardAction->setCheckable(true);
  connect( timeBackwardAction, SIGNAL( triggered() ) ,  SLOT( animationBack() ) );
  // --------------------------------------------------------------------
  timeForewardAction = new QAction( QPixmap(slutt_xpm ),tr("Run Forewards"), this );
  timeForewardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeForewardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Right);
  timeForewardAction->setCheckable(true);
  connect( timeForewardAction, SIGNAL( triggered() ) ,  SLOT( animation() ) );
  // --------------------------------------------------------------------
  timeStepBackwardAction = new QAction( QPixmap(bakover_xpm ),tr("Step Backwards"), this );
  //  timeStepBackwardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeStepBackwardAction->setShortcut(Qt::CTRL+Qt::Key_Left);
  timeStepBackwardAction->setCheckable(false);
  connect( timeStepBackwardAction, SIGNAL( triggered() ) ,  SLOT( stepback() ) );
  // --------------------------------------------------------------------
  timeStepForewardAction = new QAction( QPixmap(forward_xpm ),tr("Step Forewards"), this );
  //  timeStepForewardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeStepForewardAction->setShortcut(Qt::CTRL+Qt::Key_Right);
  timeStepForewardAction->setCheckable(false);
  connect( timeStepForewardAction, SIGNAL( triggered() ) ,  SLOT( stepforward() ) );
  // --------------------------------------------------------------------
  timeStopAction = new QAction( QPixmap(stop_xpm ),tr("Stop"), this );
  timeStopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeStopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Down);
  timeStopAction->setCheckable(false);
  connect( timeStopAction, SIGNAL( triggered() ) ,  SLOT( animationStop() ) );
  // --------------------------------------------------------------------
  timeLoopAction = new QAction( QPixmap(loop_xpm ),tr("Run in loop"), this );
  timeLoopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeLoopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Up);
  timeLoopAction->setCheckable(true);
  connect( timeLoopAction, SIGNAL( triggered() ) ,  SLOT( animationLoop() ) );
  // --------------------------------------------------------------------
  timeControlAction = new QAction( QPixmap( clock_xpm ),tr("Time control"), this );
  timeControlAction->setShortcutContext(Qt::ApplicationShortcut);
  timeControlAction->setCheckable(true);
  timeControlAction->setEnabled(false);
  connect( timeControlAction, SIGNAL( triggered() ) ,  SLOT( timecontrolslot() ) );
  // --------------------------------------------------------------------


  // other tools ======================
  // --------------------------------------------------------------------
  toolLevelUpAction = new QAction( QPixmap(levelUp_xpm ),tr("Level up"), this );
  toolLevelUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelUpAction->setShortcut(Qt::CTRL+Qt::Key_PageUp);
  toolLevelUpAction->setCheckable(false);
  toolLevelUpAction->setEnabled ( false );
  connect( toolLevelUpAction, SIGNAL( triggered() ) ,  SLOT( levelUp() ) );
  // --------------------------------------------------------------------
  toolLevelDownAction = new QAction( QPixmap(levelDown_xpm ),tr("Level down"), this );
  toolLevelDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelDownAction->setShortcut(Qt::CTRL+Qt::Key_PageDown);
  toolLevelDownAction->setCheckable(false);
  toolLevelDownAction->setEnabled ( false );
  connect( toolLevelDownAction, SIGNAL( triggered() ) ,  SLOT( levelDown() ) );
  // --------------------------------------------------------------------
  toolIdnumUpAction = new QAction( QPixmap(idnumUp_xpm ),
      tr("EPS cluster/member etc up"), this );
  toolIdnumUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumUpAction->setShortcut(Qt::SHIFT+Qt::Key_PageUp);
  toolIdnumUpAction->setCheckable(false);
  toolIdnumUpAction->setEnabled ( false );
  connect( toolIdnumUpAction, SIGNAL( triggered() ) ,  SLOT( idnumUp() ) );
  // --------------------------------------------------------------------
  toolIdnumDownAction = new QAction( QPixmap(idnumDown_xpm ),
      tr("EPS cluster/member etc down"), this );
  toolIdnumDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumDownAction->setShortcut(Qt::SHIFT+Qt::Key_PageDown);
  toolIdnumDownAction->setEnabled ( false );
  connect( toolIdnumDownAction, SIGNAL( triggered() ) ,  SLOT( idnumDown() ) );

  // Status ===============================
  // --------------------------------------------------------------------
  obsUpdateAction = new QAction( QPixmap(synop_red_xpm),
      tr("Update observations"), this );
  connect( obsUpdateAction, SIGNAL( triggered() ), SLOT(updateObs()));

  // Autoupdate ===============================
  // --------------------------------------------------------------------
  autoUpdateAction = new QAction( QPixmap(autoupdate_xpm),
      tr("Automatic updates"), this );
  autoUpdateAction->setCheckable(true);
  doAutoUpdate = false;
  connect( autoUpdateAction, SIGNAL( triggered() ), SLOT(autoUpdate()));

  // edit  ===============================
  // --------------------------------------------------------------------
  undoAction = new QAction(this);
  undoAction->setShortcutContext(Qt::ApplicationShortcut);
  undoAction->setShortcut(Qt::CTRL+Qt::Key_Z);
  connect(undoAction, SIGNAL( triggered() ), SLOT(undo()));
  addAction( undoAction );
  // --------------------------------------------------------------------
  redoAction = new QAction(this);
  redoAction->setShortcutContext(Qt::ApplicationShortcut);
  redoAction->setShortcut(Qt::CTRL+Qt::Key_Y);
  connect(redoAction, SIGNAL( triggered() ), SLOT(redo()));
  addAction( redoAction );
  // --------------------------------------------------------------------
  saveAction = new QAction(this);
  saveAction->setShortcutContext(Qt::ApplicationShortcut);
  saveAction->setShortcut(Qt::CTRL+Qt::Key_S);
  connect(saveAction, SIGNAL( triggered() ), SLOT(save()));
  addAction( saveAction );
  // --------------------------------------------------------------------

  // Browsing quick menus ===============================
  // --------------------------------------------------------------------
  leftBrowsingAction = new QShortcut( Qt::ALT+Qt::Key_Left,this );
  connect( leftBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  rightBrowsingAction = new QShortcut( Qt::ALT+Qt::Key_Right,this );
  connect( rightBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  upBrowsingAction = new QShortcut( Qt::ALT+Qt::Key_Up,this );
  connect( upBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  downBrowsingAction = new QShortcut( Qt::ALT+Qt::Key_Down,this );
  connect( downBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));

  //Time step up/down
  timeStepUpAction = new QShortcut( Qt::CTRL+Qt::Key_Up,this );
  connect( timeStepUpAction, SIGNAL( activated() ), SLOT( increaseTimeStep()));
  timeStepDownAction = new QShortcut( Qt::CTRL+Qt::Key_Down,this );
  connect( timeStepDownAction, SIGNAL( activated() ), SLOT( decreaseTimeStep()));
  /*
    ----------------------------------------------------------
    Menu Bar
    ----------------------------------------------------------
   */

  //-------File menu
  filemenu = menuBar()->addMenu(tr("File"));
  filemenu->addAction( fileSavePictAction );
  filemenu->addAction( emailPictureAction );
  filemenu->addAction( saveAnimationAction );
  filemenu->addAction( filePrintAction );
  filemenu->addSeparator();
  filemenu->addAction( fileQuitAction );

  //-------Options menu
  optmenu = menuBar()->addMenu(tr("O&ptions"));
  optmenu->addAction( optOnOffAction );
  optmenu->addAction( optArchiveAction );
  optmenu->addAction( optAutoElementAction );
  optmenu->addAction( optAnnotationAction );
  optmenu->addAction( optScrollwheelZoomAction );
  optmenu->addAction( readSetupAction );
  optmenu->addSeparator();
  optmenu->addAction( optFontAction );

  optOnOffAction->setChecked( showelem );
  optAutoElementAction->setChecked( autoselect );
  optAnnotationAction->setChecked( true );


  rightclickmenu = new QMenu(this);
  rightclickmenu->addAction(zoomOutAction);
  for (int i=0; i<MaxSelectedAreas; i++){
    selectAreaAction[i] = new QAction(this);
    selectAreaAction[i]->setVisible(false);
    connect(selectAreaAction[i], SIGNAL( triggered() ), SLOT(selectedAreas()));
    rightclickmenu->addAction(selectAreaAction[i]);
  }

  uffda=contr->getUffdaEnabled();

  QMenu* infomenu= new QMenu(tr("Info"),this);
  infomenu->setIcon(QPixmap(info_xpm));
  connect(infomenu, SIGNAL(triggered(QAction *)),
      SLOT(info_activated(QAction *)));
  infoFiles= contr->getInfoFiles();
  if (infoFiles.size()>0){
    map<miutil::miString,InfoFile>::iterator p=infoFiles.begin();
    for (; p!=infoFiles.end(); p++){
      infomenu->addAction(p->first.cStr());
    }
  }
  //  infoB->setAccel(Qt::ALT+Qt::Key_N);

  //   //-------Show menu
  showmenu = menuBar()->addMenu(tr("Show"));
  showmenu->addAction( showApplyAction              );
  showmenu->addAction( showAddQuickAction           );
  showmenu->addAction( showPrevPlotAction           );
  showmenu->addAction( showNextPlotAction           );
  showmenu->addSeparator();
  showmenu->addAction( showHideAllAction            );
  showmenu->addAction( showQuickmenuAction          );
  showmenu->addAction( showMapDialogAction          );
  showmenu->addAction( showFieldDialogAction        );
  showmenu->addAction( showObsDialogAction          );
  showmenu->addAction( showSatDialogAction          );
  showmenu->addAction( showStationDialogAction      );
  showmenu->addAction( showEditDialogAction         );
  showmenu->addAction( showObjectDialogAction       );
  showmenu->addAction( showTrajecDialogAction       );
  showmenu->addAction( showMeasurementsDialogAction    );
  showmenu->addAction( showProfilesDialogAction     );
  showmenu->addAction( showCrossSectionDialogAction );
  showmenu->addAction( showWaveSpectrumDialogAction );

  if(enableProfet){
    showmenu->addAction(  toggleProfetGUIAction );
    //showmenu->addAction( togglePaintModeAction );
  }
  if (uffda){
    showmenu->addAction( showUffdaDialogAction );
  }
  showmenu->addMenu( infomenu );


  //   //-------Help menu
  helpmenu = menuBar()->addMenu(tr("&Help"));
  helpmenu->addAction ( helpDocAction );
  helpmenu->addSeparator();
  helpmenu->addAction ( helpAccelAction );
  helpmenu->addAction ( helpNewsAction );
  helpmenu->addAction ( helpBugAction );
  helpmenu->addSeparator();
  helpmenu->addAction ( helpAboutAction );


  /*
    ----------------------------------------------------------
    Tool Bars
    ----------------------------------------------------------
   */

  // ----------------Timer widgets -------------------------

  tslider= new TimeSlider(Qt::Horizontal,this);
  tslider->setMinimumWidth(90);
  //tslider->setMaximumWidth(90);
  connect(tslider,SIGNAL(valueChanged(int)),SLOT(TimeSliderMoved()));
  connect(tslider,SIGNAL(sliderReleased()),SLOT(TimeSelected()));
  connect(tslider,SIGNAL(sliderSet()),SLOT(SliderSet()));

  timestep= new TimeStepSpinbox(this);
  connect(tslider,SIGNAL(minInterval(int)),
      timestep,SLOT(setValue(int)));
  connect(tslider,SIGNAL(timeSteps(int,int)),
      timestep,SLOT(setTimeSteps(int,int)));
  connect(tslider,SIGNAL(enableSpin(bool)),
      timestep,SLOT(setEnabled(bool)));
  connect(timestep,SIGNAL(valueChanged(int)),
      tslider,SLOT(setInterval(int)));
  connect(tslider,SIGNAL(enableSpin(bool)),
      timeControlAction,SLOT(setEnabled(bool)));


  timelabel= new QLabel("000%0-00-00 00:00:00",this);
  timelabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timelabel->setMinimumSize(timelabel->sizeHint());

  timecontrol = new TimeControl(this);
  connect(timecontrol, SIGNAL(timeoutChanged(float)),
      SLOT(timeoutChanged(float)));
  connect(timecontrol, SIGNAL(minmaxValue(const miutil::miTime&, const miutil::miTime&)),
      tslider, SLOT(setMinMax(const miutil::miTime&, const miutil::miTime&)));
  connect(timecontrol, SIGNAL(clearMinMax()),
      tslider, SLOT(clearMinMax()));
  connect(tslider, SIGNAL(newTimes(vector<miutil::miTime>&)),
      timecontrol, SLOT(setTimes(vector<miutil::miTime>&)));
  connect(timecontrol, SIGNAL(data(miutil::miString)),
      tslider, SLOT(useData(miutil::miString)));
  connect(timecontrol, SIGNAL(timecontrolHide()),
      SLOT(timecontrolslot()));

  timerToolbar= new QToolBar("TimerToolBar",this);
  timeSliderToolbar= new QToolBar("TimeSliderToolBar",this);
  levelToolbar= new QToolBar("levelToolBar",this);
  mainToolbar = new QToolBar("mainToolBar",this);
  timerToolbar->setObjectName("TimerToolBar");
  timeSliderToolbar->setObjectName("TimeSliderToolBar");
  levelToolbar->setObjectName("levelToolBar");
  mainToolbar->setObjectName("mainToolBar");
  addToolBar(Qt::RightToolBarArea,mainToolbar);
  addToolBar(Qt::TopToolBarArea,levelToolbar);
  insertToolBar(levelToolbar,timeSliderToolbar);
  insertToolBar(timeSliderToolbar,timerToolbar);

  timerToolbar->addAction( timeBackwardAction    );
  timerToolbar->addAction( timeStepBackwardAction );
  timerToolbar->addAction( timeStepForewardAction );
  timerToolbar->addAction( timeForewardAction  );
  timerToolbar->addAction( timeStopAction      );
  timerToolbar->addAction( timeLoopAction        );

  timeSliderToolbar->addWidget( tslider );

  levelToolbar->addAction( timeControlAction );
  levelToolbar->addWidget( timestep );
  levelToolbar->addWidget( timelabel );
  levelToolbar->addAction( toolLevelUpAction );
  levelToolbar->addAction( toolLevelDownAction );
  levelToolbar->addSeparator();
  levelToolbar->addAction( toolIdnumUpAction );
  levelToolbar->addAction( toolIdnumDownAction );
  levelToolbar->addSeparator();
  levelToolbar->addAction( obsUpdateAction );
  //  levelToolbar->addAction( autoUpdateAction );

  /**************** Toolbar Buttons *********************************************/

  //  mainToolbar = new QToolBar(this);
  //  addToolBar(Qt::RightToolBarArea,mainToolbar);

  mainToolbar->addAction( showResetAreaAction         );
  mainToolbar->addAction( showQuickmenuAction         );
  mainToolbar->addAction( showMapDialogAction         );
  mainToolbar->addAction( showFieldDialogAction       );
  mainToolbar->addAction( showObsDialogAction         );
  mainToolbar->addAction( showSatDialogAction         );
  mainToolbar->addAction( showStationDialogAction     );
  mainToolbar->addAction( showObjectDialogAction      );
  mainToolbar->addAction( showTrajecDialogAction      );
  mainToolbar->addAction( showMeasurementsDialogAction   );
  mainToolbar->addAction( showProfilesDialogAction    );
  mainToolbar->addAction( showCrossSectionDialogAction);
  mainToolbar->addAction( showWaveSpectrumDialogAction);
  if(enableProfet){
    mainToolbar->addAction( toggleProfetGUIAction       );
    //mainToolbar->addAction( togglePaintModeAction   );
  }

  mainToolbar->addSeparator();
  mainToolbar->addAction( showEditDialogAction );
  mainToolbar->addSeparator();
  mainToolbar->addAction( showResetAllAction );


  /****************** Status bar *****************************/

  statusbuttons= new StatusPlotButtons();
  connect(statusbuttons, SIGNAL(toggleElement(PlotElement)),
      SLOT(toggleElement(PlotElement)));
  connect(statusbuttons, SIGNAL(releaseFocus()),
      SLOT(setFocus()));
  statusBar()->addWidget(statusbuttons);
  if (!showelem) statusbuttons->hide();


  showsatval = new ShowSatValues();
  statusBar()->addPermanentWidget(showsatval);

  sgeopos= new StatusGeopos();
  statusBar()->addPermanentWidget(sgeopos);


  archiveL = new QLabel(tr("ARCHIVE"),statusBar());
  archiveL->setFrameStyle( QFrame::Panel );
  archiveL->setAutoFillBackground(true);
  QPalette palette;
  palette.setColor(archiveL->backgroundRole(), "red");
  archiveL->setPalette(palette);
  statusBar()->addPermanentWidget(archiveL);
  archiveL->hide();

  hqcTo = -1;
  qsocket = false;
  miutil::miString server = LocalSetupParser::basicValue("qserver");
  pluginB = new ClientButton(tr("Diana"),server.c_str(),statusBar());
  //   pluginB->setMinimumWidth( hpixbutton );
  //   pluginB->setMaximumWidth( hpixbutton );
  connect(pluginB, SIGNAL(receivedMessage(miMessage&)),
      SLOT(processLetter(miMessage&)));
  connect(pluginB, SIGNAL(connectionClosed()),SLOT(connectionClosed()));
  statusBar()->addPermanentWidget(pluginB);

  QtImageGallery ig;
  QImage f_img(felt_xpm);
  ig.addImageToGallery("FIELD",f_img);
  QImage s_img(sat_xpm);
  ig.addImageToGallery("RASTER",s_img);
  QImage obs_img(synop_xpm);
  ig.addImageToGallery("OBS",obs_img);
  QImage obj_img(front_xpm);
  ig.addImageToGallery("OBJECTS",obj_img);
  QImage t_img(traj_xpm);
  ig.addImageToGallery("TRAJECTORY",t_img);
  QImage vp_img(balloon_xpm);
  ig.addImageToGallery("vprof_icon",vp_img);
  QImage location_img(vcross_xpm);
  ig.addImageToGallery("LOCATION",location_img);
  QImage sp_img(spectrum_xpm);
  ig.addImageToGallery("spectrum_icon",sp_img);
  QImage st_img(station_xpm);
  ig.addImageToGallery("STATION",st_img);

  // Read the avatars to gallery

  miutil::miString avatarpath = LocalSetupParser::basicValue("avatars");
  if ( avatarpath.exists() ){
    vector<miutil::miString> vs = avatarpath.split(":");
    for ( unsigned int i=0; i<vs.size(); i++ ){
      ig.addImagesInDirectory(vs[i]);
    }
  }

  //-------------------------------------------------

  w= new WorkArea(contr,this);
  setCentralWidget(w);

  connect(w->Glw(), SIGNAL(mouseGridPos(const mouseEvent)),
      SLOT(catchMouseGridPos(const mouseEvent)));

  connect(w->Glw(), SIGNAL(mouseRightPos(const mouseEvent)),
      SLOT(catchMouseRightPos(const mouseEvent)));

  connect(w->Glw(), SIGNAL(mouseMovePos(const mouseEvent,bool)),
      SLOT(catchMouseMovePos(const mouseEvent,bool)));

  connect(w->Glw(), SIGNAL(keyPress(const keyboardEvent)),
      SLOT(catchKeyPress(const keyboardEvent)));

  connect(w->Glw(), SIGNAL(mouseDoubleClick(const mouseEvent)),
      SLOT(catchMouseDoubleClick(const mouseEvent)));

  // ----------- init dialog-objects -------------------

  qm= new QuickMenu(this, contr);
  qm->hide();

  fm= new FieldDialog(this, contr);
  fm->hide();

  om= new ObsDialog(this, contr);
  om->hide();

  sm= new SatDialog(this, contr);
  sm->hide();

  stm= new StationDialog(this, contr);
  stm->hide();

  mm= new MapDialog(this, contr);
  mm->hide();

  em= new EditDialog( this, contr );
  em->hide();

  objm = new ObjectDialog(this,contr);
  objm->hide();

  trajm = new TrajectoryDialog(this,contr);
  trajm->setFocusPolicy(Qt::StrongFocus);
  trajm->hide();

  measurementsm = new MeasurementsDialog(this,contr);
  measurementsm->setFocusPolicy(Qt::StrongFocus);
  measurementsm->hide();

  uffm = new UffdaDialog(this,contr);
  uffm->hide();

  mailm = new MailDialog(this,contr);
  mailm->hide();

  paintToolBar = new PaintToolBar(this);
  paintToolBar->setObjectName("PaintToolBar");
  addToolBar(Qt::BottomToolBarArea,paintToolBar);
  paintToolBar->hide();

  textview = new TextView(this);
  textview->setMinimumWidth(300);
  connect(textview,SIGNAL(printClicked(int)),SLOT(sendPrintClicked(int)));
  textview->hide();


  //used for testing qickMenus without dialogs
  //connect(qm, SIGNAL(Apply(const vector<miutil::miString>&,bool)),
  //  	  SLOT(quickMenuApply(const vector<miutil::miString>&)));


  connect(qm, SIGNAL(Apply(const vector<miutil::miString>&,bool)),
      SLOT(recallPlot(const vector<miutil::miString>&,bool)));

  connect(em, SIGNAL(Apply(const vector<miutil::miString>&,bool)),
      SLOT(recallPlot(const vector<miutil::miString>&,bool)));


  // Mark trajectory positions
  connect(trajm, SIGNAL(markPos(bool)), SLOT(trajPositions(bool)));
  connect(trajm, SIGNAL(updateTrajectories()),SLOT(updateGLSlot()));

  // Mark measurement positions
  connect(measurementsm, SIGNAL(markMeasurementsPos(bool)), SLOT(measurementsPositions(bool)));
  connect(measurementsm, SIGNAL(updateMeasurements()),SLOT(updateGLSlot()));

  connect(em, SIGNAL(editUpdate()), SLOT(editUpdate()));
  connect(em, SIGNAL(editMode(bool)), SLOT(inEdit(bool)));

  connect(uffm, SIGNAL(stationPlotChanged()), SLOT(updateGLSlot()));

  connect(mailm, SIGNAL(saveImage(QString)), SLOT(saveRasterImage(QString)));
  // Documentation and Help

  HelpDialog::Info info;
  HelpDialog::Info::Source helpsource;
  info.path= LocalSetupParser::basicValue("docpath");
  helpsource.source= "index.html";
  helpsource.name= "Help";
  helpsource.defaultlink= "START";
  info.src.push_back(helpsource);

  helpsource.source= "ug_shortcutkeys.html";
  helpsource.name="Accelerators";
  helpsource.defaultlink= "";
  info.src.push_back(helpsource);

  helpsource.source= "news.html";
  helpsource.name="News";
  helpsource.defaultlink="";
  info.src.push_back(helpsource);

  help= new HelpDialog(this, info);
  help->hide();

  connect( fm, SIGNAL(FieldApply()), SLOT(MenuOK()));
  connect( om, SIGNAL(ObsApply()),   SLOT(MenuOK()));
  connect( sm, SIGNAL(SatApply()),   SLOT(MenuOK()));
  connect( stm, SIGNAL(StationApply()), SLOT(MenuOK()));
  connect( mm, SIGNAL(MapApply()),   SLOT(MenuOK()));
  connect( objm, SIGNAL(ObjApply()), SLOT(MenuOK()));
  connect( em, SIGNAL(editApply()),  SLOT(editApply()));

  connect( fm, SIGNAL(FieldHide()),  SLOT(fieldMenu()));
  connect( om, SIGNAL(ObsHide()),    SLOT(obsMenu()));
  connect( sm, SIGNAL(SatHide()),    SLOT(satMenu()));
  connect( stm, SIGNAL(StationHide()), SLOT(stationMenu()));
  connect( mm, SIGNAL(MapHide()),    SLOT(mapMenu()));
  connect( em, SIGNAL(EditHide()),   SLOT(editMenu()));
  connect( qm, SIGNAL(QuickHide()),  SLOT(quickMenu()));
  connect( objm, SIGNAL(ObjHide()),  SLOT(objMenu()));
  connect( trajm, SIGNAL(TrajHide()),SLOT(trajMenu()));
  connect( measurementsm, SIGNAL(MeasurementsHide()),SLOT(measurementsMenu()));
  connect( uffm, SIGNAL(uffdaHide()),SLOT(uffMenu()));

  // update field dialog when editing field
  connect( em, SIGNAL(emitFieldEditUpdate(miutil::miString)),
      fm, SLOT(fieldEditUpdate(miutil::miString)));

  // resize main window according to the active map area when using
  // an editing tool
  connect( em, SIGNAL(emitResize(int, int)),
      this, SLOT(winResize(int, int)));

  // HELP
  connect( fm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( om, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( sm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( stm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( mm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( em, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( qm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( objm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( trajm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( uffm, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect( paintToolBar, SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));

  connect(w->Glw(),SIGNAL(objectsChanged()),em, SLOT(undoFrontsEnable()));
  connect(w->Glw(),SIGNAL(fieldsChanged()), em, SLOT(undoFieldsEnable()));

  // vertical profiles
  // create a new main window
  vpWindow = new VprofWindow(contr);
  connect(vpWindow,SIGNAL(VprofHide()),SLOT(hideVprofWindow()));
  connect(vpWindow,SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect(vpWindow,SIGNAL(stationChanged(const QString &)),
      SLOT(stationChangedSlot(const QString &)));
  connect(vpWindow,SIGNAL(modelChanged()),SLOT(modelChangedSlot()));

  // vertical crossections
  // create a new main window
  vcWindow = new VcrossWindow(contr);
  connect(vcWindow,SIGNAL(VcrossHide()),SLOT(hideVcrossWindow()));
  connect(vcWindow,SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect(vcWindow,SIGNAL(crossectionChanged(const QString &)),
      SLOT(crossectionChangedSlot(const QString &)));
  connect(vcWindow,SIGNAL(crossectionSetChanged()),
      SLOT(crossectionSetChangedSlot()));
  connect(vcWindow,SIGNAL(crossectionSetUpdate()),
      SLOT(crossectionSetUpdateSlot()));
  connect(vcWindow,SIGNAL(updateCrossSectionPos(bool)),
      SLOT(vCrossPositions(bool)));
  connect(vcWindow,SIGNAL(quickMenuStrings(const miutil::miString, const vector<miutil::miString>&)),
      SLOT(updateQuickMenuHistory(const miutil::miString, const vector<miutil::miString>&)));
  connect (vcWindow, SIGNAL(prevHVcrossPlot()), SLOT(prevHVcrossPlot()));
  connect (vcWindow, SIGNAL(nextHVcrossPlot()), SLOT(nextHVcrossPlot()));
  // Wave spectrum
  // create a new main window
  spWindow = new SpectrumWindow();
  connect(spWindow,SIGNAL(SpectrumHide()),SLOT(hideSpectrumWindow()));
  connect(spWindow,SIGNAL(showsource(const miutil::miString,const miutil::miString)),
      help,SLOT(showsource(const miutil::miString,const miutil::miString)));
  connect(spWindow,SIGNAL(spectrumChanged(const QString &)),
      SLOT(spectrumChangedSlot(const QString &)));
  connect(spWindow,SIGNAL(spectrumSetChanged()),
      SLOT(spectrumSetChangedSlot()));

  // browse plots
  browser= new BrowserBox(this);
  connect(browser, SIGNAL(selectplot()),this,SLOT(browserSelect()));
  connect(browser, SIGNAL(cancel()),   this, SLOT(browserCancel()));
  connect(browser, SIGNAL(prevplot()), this, SLOT(prevQPlot()));
  connect(browser, SIGNAL(nextplot()), this, SLOT(nextQPlot()));
  connect(browser, SIGNAL(prevlist()), this, SLOT(prevList()));
  connect(browser, SIGNAL(nextlist()), this, SLOT(nextList()));
  browser->hide();

  connect( fm ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
      tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

  connect( om ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
      tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

  connect( sm ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&,bool)),
      tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&,bool)));

  connect( em ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
      tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

  connect( objm ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&,bool)),
      tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&,bool)));

  if ( vpWindow ){
    connect( vpWindow ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
        tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

    connect( vpWindow ,SIGNAL(setTime(const miutil::miString&, const miutil::miTime&)),
        tslider,SLOT(setTime(const miutil::miString&, const miutil::miTime&)));
  }
  if ( vcWindow ){
    connect( vcWindow ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
        tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

    connect( vcWindow ,SIGNAL(setTime(const miutil::miString&, const miutil::miTime&)),
        tslider,SLOT(setTime(const miutil::miString&, const miutil::miTime&)));
  }
  if ( spWindow ){
    connect( spWindow ,SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
        tslider,SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));

    connect( spWindow ,SIGNAL(setTime(const miutil::miString&, const miutil::miTime&)),
        tslider,SLOT(setTime(const miutil::miString&, const miutil::miTime&)));
  }


  //parse labels
  const miutil::miString label_name = "LABELS";
  vector<miutil::miString> sect_label;

  if (!miutil::SetupParser::getSection(label_name,sect_label)){
    cerr << label_name << " section not found" << endl;
    //default
    vlabel.push_back("LABEL data font=BITMAPFONT");
    miutil::miString labelstr= "LABEL text=\"$day $date $auto UTC\" ";
    labelstr += "tcolour=red bcolour=black ";
    labelstr+= "fcolour=white:200 polystyle=both halign=left valign=top ";
    labelstr+= "font=BITMAPFONT fontsize=12";
    vlabel.push_back(labelstr);
    vlabel.push_back
    ("LABEL anno=<table,fcolour=white:150> halign=right valign=top fcolour=white:0 margin=0");
  }

  for(unsigned int i=0; i<sect_label.size(); i++) {
    vlabel.push_back(sect_label[i]);
  }


  cerr << "Creating DianaMainWindow done" << endl;

}


void DianaMainWindow::start()
{
  // read the log file
  readLogFile();

  // init quickmenus
  qm->start();

  // make initial plot
  MenuOK();

  //statusBar()->message( tr("Ready"), 2000 );

  dialogChanged = false;

  if (showelem){
    statusbuttons->setPlotElements(contr->getPlotElements());
    statusbuttons->show();
  } else {
    statusbuttons->reset();
    statusbuttons->hide();
  }

  optOnOffAction->      setChecked( showelem );
  optAutoElementAction->setChecked( autoselect );

  w->Glw()->setFocus();

}

// -- destructor
DianaMainWindow::~DianaMainWindow()
{
  // no explicit destruction is necessary at this point
}


void DianaMainWindow::focusInEvent( QFocusEvent * )
{
  w->Glw()->setFocus();
}

void DianaMainWindow::editUpdate()
{
#ifdef DEBUGREDRAW
  cerr << "DianaMainWindow::editUpdate" << endl;
#endif
  w->Glw()->forceUnderlay(true);
  w->updateGL();
}



void DianaMainWindow::quickMenuApply(const vector<miutil::miString>& s)
{
#ifdef DEBUGPRINT
  cerr << "quickMenuApply:" << endl;
#endif
  QApplication::setOverrideCursor( Qt::WaitCursor );
  contr->plotCommands(s);

  vector<miutil::miTime> fieldtimes, sattimes, obstimes, objtimes, ptimes;
  contr->getPlotTimes(fieldtimes,sattimes,obstimes,objtimes,ptimes);

  tslider->insert("field", fieldtimes);
  tslider->insert("sat", sattimes);
  tslider->insert("obs", obstimes);
  tslider->insert("obs", objtimes);
  tslider->insert("product", ptimes);

  miutil::miTime t= tslider->Value();
  contr->setPlotTime(t);
  contr->updatePlots();
  //find current field models and send to vprofwindow..
  vector <miutil::miString> fieldmodels = contr->getFieldModels();
  if (vpWindow) vpWindow->setFieldModels(fieldmodels);
  if (spWindow) spWindow->setFieldModels(fieldmodels);
  w->updateGL();
  timeChanged();

  dialogChanged=false;
  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::resetAll()
{
  mm->useFavorite();
  vector<miutil::miString> pstr = mm->getOKString();;
  recallPlot(pstr, true);
  MenuOK();
}

void DianaMainWindow::recallPlot(const vector<miutil::miString>& vstr,bool replace)
{
  QApplication::setOverrideCursor( Qt::WaitCursor );

  if ( vstr.size() > 0 && vstr[0] == "VCROSS") {
    vcrossMenu();
    vcWindow->parseQuickMenuStrings(vstr);
  } else {

    // strings for each dialog
    vector<miutil::miString> mapcom,fldcom,obscom,satcom,statcom,objcom,labelcom;
    int n= vstr.size();
    // sort strings..
    for (int i=0; i<n; i++){
      miutil::miString s= vstr[i];
      s.trim();
      if (!s.exists()) continue;
      vector<miutil::miString> vs= s.split(" ");
      miutil::miString pre= vs[0].upcase();
      if (pre=="MAP") mapcom.push_back(s);
      else if (pre=="AREA") mapcom.push_back(s);
      else if (pre=="FIELD") fldcom.push_back(s);
      else if (pre=="OBS") obscom.push_back(s);
      else if (pre=="SAT") satcom.push_back(s);
      else if (pre=="STATION") statcom.push_back(s);
      else if (pre=="OBJECTS") objcom.push_back(s);
      else if (pre=="LABEL") labelcom.push_back(s);
    }

    vector<miutil::miString> tmplabel = vlabel;
    // feed strings to dialogs
    if (replace || mapcom.size()) mm->putOKString(mapcom);
    if (replace || fldcom.size()) fm->putOKString(fldcom);
    if (replace || obscom.size()) om->putOKString(obscom);
    if (replace || satcom.size()) sm->putOKString(satcom);
    if (replace || statcom.size()) stm->putOKString(statcom);
    if (replace || objcom.size()) objm->putOKString(objcom);
    if (replace ) vlabel=labelcom;

    // call full plot
    push_command= false; // do not push this command on stack
    MenuOK();
    push_command= true;
    vlabel = tmplabel;
  }
  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::togglePaintMode()
{
  if (paintToolBar == 0) return;
  if (paintToolBar->isVisible()) paintToolBar->hide();
  else paintToolBar->show();
  cerr << "DianaMainWindow::togglePaintMode enabled " << paintToolBar->isVisible() << endl;
  contr->setPaintModeEnabled(paintToolBar->isVisible());
}
void DianaMainWindow::setPaintMode(bool enabled)
{
  if (paintToolBar == 0) return;
  if (enabled) paintToolBar->show();
  else paintToolBar->hide();
  contr->setPaintModeEnabled(enabled);
}

void DianaMainWindow::winResize(int w, int h)
{
  this->resize(w,h); 
}

void DianaMainWindow::resetArea()
{
  contr->keepCurrentArea(false);
  MenuOK();
  contr->keepCurrentArea(true);
}


// called from EditDialog when starting/ending product-editing
// - This signal used to be connected to MenuOK() directly.
//   Now, we disable the history-stack during these operations
//   to prevent too many (nearly) identical plots on the stack
void DianaMainWindow::editApply()
{
  push_command= false;
  MenuOK();
  push_command= true;
}

void DianaMainWindow::getPlotStrings(vector<miutil::miString> &pstr,
    vector<miutil::miString> &diagstr,
    vector<miutil::miString> &shortnames)
{

  // fields
  pstr = fm->getOKString();
  shortnames.push_back(fm->getShortname());

  // Observations
  diagstr = om->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(om->getShortname());

  //satellite
  diagstr = sm->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(sm->getShortname());

  // Stations
  diagstr = stm->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(stm->getShortname());

  // objects
  diagstr = objm->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(objm->getShortname());

  // map
  diagstr = mm->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(mm->getShortname());

  // label
  bool remove = (contr->getMapMode() != normal_mode || tslider->numTimes()==0);
  for(unsigned int i=0; i<vlabel.size(); i++){
    if(!remove || !vlabel[i].contains("$"))  //remove labels with time
      pstr.push_back(vlabel[i]);
  }

  // remove empty lines
  for (unsigned int i = 0; i < pstr.size(); ++i){
    pstr[i].trim();
    if (!pstr[i].exists()){
      pstr.erase(pstr.begin()+i);
      --i;
      continue;
    }
  }
}

void DianaMainWindow::MenuOK()
{
#ifdef DEBUGREDRAW
  cerr<<"DianaMainWindow::MenuOK"<<endl;
#endif

  QApplication::setOverrideCursor( Qt::WaitCursor );

  vector<miutil::miString> pstr;
  vector<miutil::miString> diagstr;
  vector<miutil::miString> shortnames;

  getPlotStrings(pstr, diagstr, shortnames);

//init level up/down arrows
  toolLevelUpAction->setEnabled(fm->levelsExists(true,0));
  toolLevelDownAction->setEnabled(fm->levelsExists(false,0));
  toolIdnumUpAction->setEnabled(fm->levelsExists(true,1));
  toolIdnumDownAction->setEnabled(fm->levelsExists(false,1));

  // printout
  cerr << "------- the final string from all dialogs:" << endl;
  for (unsigned int i = 0; i < pstr.size(); ++i)
    cerr << pstr[i] << endl;

  miutil::miTime t = tslider->Value();
  contr->plotCommands(pstr);
  contr->setPlotTime(t);
  contr->updatePlots();
  cout <<contr->getMapArea()<<endl;

  //find current field models and send to vprofwindow..
  vector <miutil::miString> fieldmodels = contr->getFieldModels();
  if (vpWindow) vpWindow->setFieldModels(fieldmodels);
  if (spWindow) spWindow->setFieldModels(fieldmodels);
  w->updateGL();
  timeChanged();
  dialogChanged = false;

  // push command on history-stack
  if (push_command){ // only when proper menuok
    // make shortname
    miutil::miString plotname;
    int m= shortnames.size();
    for (int j=0; j<m; j++)
      if (shortnames[j].exists()){
        plotname+= shortnames[j];
        if (j!=m-1) plotname+= " ";
      }
    qm->pushPlot(plotname,pstr,QuickMenu::MAP);
  }

  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::updateQuickMenuHistory(const miutil::miString plotname, const vector<miutil::miString>& pstr)
{
  qm->pushPlot(plotname,pstr,QuickMenu::VCROSS);
}

// recall previous QuickMenu plot (if exists)
void DianaMainWindow::prevQPlot()
{
  if (qm->prevQPlot()){
    if (!browsing)
      qm->applyPlot();
    else
      updateBrowser();
  }
}

// recall next QuickMenu plot (if exists)
void DianaMainWindow::nextQPlot()
{
  if (qm->nextQPlot()){
    if (!browsing)
      qm->applyPlot();
    else
      updateBrowser();
  }
}

// recall previous plot in plot-stack (if exists)
void DianaMainWindow::prevHPlot()
{
  qm->prevHPlot(QuickMenu::MAP);
}

// recall next plot in plot-stack (if exists)
void DianaMainWindow::nextHPlot()
{
  qm->nextHPlot(QuickMenu::MAP);
}
void DianaMainWindow::prevHVcrossPlot()
{
  qm->prevHPlot(QuickMenu::VCROSS);
}

// recall next plot in plot-stack (if exists)
void DianaMainWindow::nextHVcrossPlot()
{
  qm->nextHPlot(QuickMenu::VCROSS);
}

// switch to next quick-list
void DianaMainWindow::nextList()
{
  if (browsing && qm->nextList())
    updateBrowser();
}

// switch to previous quick-list
void DianaMainWindow::prevList()
{
  if (browsing && qm->prevList())
    updateBrowser();
}

// start browser-window
void DianaMainWindow::startBrowsing()
{
  if (!updateBrowser()) return;
  browsing= true;
  qm->startBrowsing();
  browser->show();
  browser->grabKeyboard();
}

// update browser-window with quick-menu details
bool DianaMainWindow::updateBrowser()
{
  int plotidx;
  miutil::miString listname,name;
  qm->getDetails(plotidx,listname,name);
  if (plotidx<0) return false;
  browser->upDate(listname,plotidx,name);
  return true;
}

// cancel signal from browser-window
void DianaMainWindow::browserCancel()
{
  browsing= false;
  browser->hide();
  browser->releaseKeyboard();
}

// select signal from browser-window
void DianaMainWindow::browserSelect()
{
  browsing= false;
  browser->hide();
  browser->releaseKeyboard();
  qm->applyPlot();
}

// add current plot to a quick-menu
void DianaMainWindow::addToMenu()
{
  AddtoMenu* am= new AddtoMenu(this, qm);
  am->exec();
}


void DianaMainWindow::toggleDialogs()
{
  const int numdialogs= 10;
  //const int numdialogs= 8;
  static bool visi[numdialogs];

  bool b = showHideAllAction->isChecked();

  if (b){
    if ((visi[0]= qm->isVisible()))    quickMenu();
    if ((visi[1]= mm->isVisible()))     mapMenu();
    if ((visi[2]= fm->isVisible()))    fieldMenu();
    if ((visi[3]= om->isVisible()))    obsMenu();
    if ((visi[4]= sm->isVisible()))    satMenu();
//    if ((visi[5]= em->isVisible()))    editMenu();
    if ((visi[6]= objm->isVisible()))  objMenu();
    if ((visi[7]= trajm->isVisible())) trajMenu();
    if ((visi[8]= measurementsm->isVisible())) measurementsMenu();
    if ((visi[9]= stm->isVisible())) stationMenu();
  } else {
    if (visi[0]) quickMenu();
    if (visi[1]) mapMenu();
    if (visi[2]) fieldMenu();
    if (visi[3]) obsMenu();
    if (visi[4]) satMenu();
    //    if (visi[5]) editMenu();
    if (visi[6]) objMenu();
    if (visi[7]) trajMenu();
    if (visi[8]) measurementsMenu();
    if (visi[9]) stationMenu();
  }
}

void DianaMainWindow::quickMenu()
{
  bool b = qm->isVisible();
  if (b){
    qm->hide();
  } else {
    qm->show();
  }
  showQuickmenuAction->setChecked(!b);
}


void DianaMainWindow::fieldMenu()
{
  bool b = fm->isVisible();
  if (b){
    fm->hide();
  }
  else {
    fm->show();
  }
  showFieldDialogAction->setChecked(!b);
}


void DianaMainWindow::obsMenu()
{
  bool b = om->isVisible();
  if (b){
    om->hide();
  } else {
    om->show();
  }
  showObsDialogAction->setChecked( !b );
}


void DianaMainWindow::satMenu()
{
  bool b = sm->isVisible();
  if (b){
    sm->hide();
  } else {
    sm->show();
  }
  showSatDialogAction->setChecked( !b );
}


void DianaMainWindow::stationMenu()
{
  bool visible = stm->isVisible();
  stm->setVisible(!visible);
  showStationDialogAction->setChecked(!visible);
}


void DianaMainWindow::uffMenu()
{
  bool b = uffm->isVisible();
  if( b ){
    uffm->hide();
    uffm->clearSelection();
    contr->stationCommand("hide","uffda");
  } else {
    uffm->show();
    contr->stationCommand("show","uffda");
  }
  w->updateGL();
  showUffdaDialogAction->setChecked( !b );
}


void DianaMainWindow::mapMenu()
{
  bool b = mm->isVisible();
  if( b ){
    mm->hide();
  } else {
    mm->show();
  }
  showMapDialogAction->setChecked( !b );
}

void DianaMainWindow::editMenu()
{
  static bool b = false;

  if ( b ){
    em->hideAll();
  } else {
    em->showAll();
  }

  b = !b;
  showEditDialogAction->setChecked( b );
}

bool DianaMainWindow::initProfet(){
#ifdef PROFET
  miutil::miString error = "";
  if(!w || !w->Glw()) error += "GLwidget is NULL. ";
  if(!tslider) error += "TimeSlider is NULL. ";
  if(!contr) error += "diController is NULL. ";
  if(!paintToolBar) error += "PaintToolBar is NULL. ";
  if(!contr->getAreaManager()) error += "AreaManager is NULL. ";
  if(error.exists()){
    QMessageBox::critical(0,"Init profet failed",error.cStr());
    return false;
  }

  try{
    QApplication::setOverrideCursor( Qt::WaitCursor );
    contr->initProfet(); //ProfetController created if not exist
    if(contr->getProfetController()) {
      profetGUI = new DianaProfetGUI(*contr->getProfetController(),
          paintToolBar, contr->getAreaManager(), this);
      contr->getProfetController()->setGUI(profetGUI);
    }else{
      QMessageBox::warning(0,"Init profet failed",
          "ProfetController is null");
      QApplication::restoreOverrideCursor();
      return false;
    }
    connect(w->Glw(), SIGNAL(gridAreaChanged()),
        profetGUI, SLOT(gridAreaChanged()));
    connect(profetGUI, SIGNAL(toggleProfetGui()),
        this,SLOT(toggleProfetGUI()));
    connect(profetGUI, SIGNAL(setPaintMode(bool)),
        this, SLOT(setPaintMode(bool)));
    connect(profetGUI, SIGNAL(showProfetField(miutil::miString)),
        fm, SLOT(fieldEditUpdate(miutil::miString)));
    connect(profetGUI, SIGNAL(prepareAndPlot()),
        SLOT(MenuOK()));
    connect( profetGUI, SIGNAL(repaintMap(bool)),
        SLOT(plotProfetMap(bool)));
    connect( profetGUI ,
        SIGNAL(emitTimes(const miutil::miString&,const vector<miutil::miTime>&)),
        tslider,
        SLOT(insert(const miutil::miString&,const vector<miutil::miTime>&)));
    connect( profetGUI, SIGNAL(setTime(const miutil::miTime&)),
        tslider, SLOT(setTime(const miutil::miTime&)));
    connect( profetGUI, SIGNAL(updateModelDefinitions()),
        fm,SLOT(updateModels()) );
    connect( profetGUI, SIGNAL(forceDisconnect(bool)),
        this, SLOT(forceProfetDisconnect(bool)));
    connect( profetGUI, SIGNAL(getFieldPlotOptions(map< miutil::miString, map<miutil::miString,miutil::miString> >&)),
        this,SLOT(getFieldPlotOptions(map< miutil::miString, map<miutil::miString,miutil::miString> >&)));
    connect( profetGUI, SIGNAL(zoomTo(Rectangle)), this, SLOT(zoomTo(Rectangle)));

    QApplication::restoreOverrideCursor();
    return true;
  }catch(Profet::ServerException & se){
    profetLoginError->showMessage(se.what());
  }
  QApplication::restoreOverrideCursor();

#endif
  return false;
}


bool DianaMainWindow::profetConnect(){
#ifdef PROFET
  miutil::miString error = "";
  bool offerForcedConnection = false;
  bool useForcedConnection = false;
  bool retry = true;
  Profet::LoginDialog loginDialog;
  loginDialog.setUsername(QString(getenv("USER")));
  loginDialog.setRoles((QStringList() << "forecast" << "observer"));

  if(loginDialog.exec()){ // OK button pressed
    if ( loginDialog.test() ) {
      Profet::ProfetController::SERVER_HOST = "profet-test";
    }
    while(retry) {
      retry = false;
      if(loginDialog.username().isEmpty())
        error += "Username not provided. ";
      Profet::PodsUser u(miutil::miTime::nowTime(),
          getenv("HOSTNAME"),
          loginDialog.username().toStdString().data(),
          loginDialog.role().toStdString().data(),
          "",miutil::miTime::nowTime(),"");
      miutil::miString password = loginDialog.password().toStdString();
      //TODO option for file manager
      Profet::DataManagerType perferredType = Profet::DISTRIBUTED_MANAGER;
      if(contr->getProfetController() && !error.exists() ) {
        try{
          QApplication::setOverrideCursor( Qt::WaitCursor );
          Profet::DataManagerType dmt =
              contr->getProfetController()->connect(u,perferredType,password, useForcedConnection);
          QApplication::restoreOverrideCursor();
          if(dmt != perferredType)
            QMessageBox::warning(0,"Running disconnected mode",
                "Distributed field editing system is not available.");
          profetGUI->setHostname(Profet::ProfetController::SERVER_HOST);
          return true;
        }catch(Profet::ServerException & se){
          contr->getProfetController()->disconnect();
          offerForcedConnection =
              (se.getType() == Profet::ServerException::CONNECTION_ERROR &&
                  se.getMinorCode() == Profet::ServerException::DUPLICATE_CONNECTION);
          bool withMailToLink = true;
          error += se.getHtmlMessage(withMailToLink);
          QApplication::restoreOverrideCursor();
        }
      }
      if(error.exists()) {
        if(offerForcedConnection) {
          error += "<br><b>Do you want to connect anyway?<br>(Existing user will be disconnected)</b>";
          int i = QMessageBox::warning(0,"Profet", error.cStr(), QMessageBox::Yes, QMessageBox::No);
          if (i == QMessageBox::Yes){
            error = "";
            retry = true;
            useForcedConnection = true;
          }
        } else {
          QMessageBox::critical(0,"Profet",error.cStr());
        }
      } // end error handling
    } // end while retry
  }
  profetDisconnect();
#endif
  return false;
}

void DianaMainWindow::profetDisconnect(){
#ifdef PROFET
  if(contr->getProfetController())
    contr->getProfetController()->disconnect();
#endif
}

void DianaMainWindow::plotProfetMap(bool objectsOnly){

  w->updateGL();

  //   if(objectsOnly) contr->plot(false,true); // Objects in overlay
  //   else MenuOK();
  //   MenuOK();
}

void DianaMainWindow::toggleProfetGUI(){
#ifdef PROFET
  // get status
  bool inited = (contr->getProfetController() && profetGUI);
  bool connected = false;
  if(inited) connected = contr->getProfetController()->isConnected();
  bool turnOn = false;
  // check turn on / off
  if(profetGUI && profetGUI->isVisible()){
    int i = (QMessageBox::question(this, tr("End Profet"),
        tr("Do you want to stay connected to profet?"),
        tr("Quit and disconnect"), tr("Quit and stay connected "), tr("&Cancel"),
        0,      // Enter == button 0
        2 ) ); // Escape == button 2

    if(i==2 ){ // cancel: still connected
      toggleProfetGUIAction->setChecked(true); // might have been unchecked
      return;
    }

    if(i == 0){
      profetDisconnect();
      profetGUI->resetStatus();
    }
  }
  else if(inited && connected){
    turnOn = !(profetGUI->isVisible());
  }
  else if(inited){ // not connected
    turnOn = profetConnect();
  }else {
    if( initProfet() && profetConnect() ) turnOn = true;
    else turnOn = false;
  }
  // do turn on / off
  toggleProfetGUIAction->setChecked(turnOn);
  profetGUI->setVisible(turnOn);
  profetGUI->setParamColours();
#endif
}

void DianaMainWindow::forceProfetDisconnect(bool disableGuiOnly){
#ifdef PROFET
  if (disableGuiOnly) {
    profetGUI->resetStatus();
    toggleProfetGUIAction->setChecked(false);
    profetGUI->setVisible(false);
    //togglePaintModeAction->setEnabled(true);
    return;
  }
  // Disconnect
  bool inited = (contr->getProfetController() && profetGUI);
  bool connected = false;
  if(inited) connected = contr->getProfetController()->isConnected();
  if(connected){
    profetDisconnect();
    profetGUI->resetStatus();
    toggleProfetGUIAction->setChecked(false);
    profetGUI->setVisible(false);
    //togglePaintModeAction->setEnabled(true);
  }
  // Re-connect
  toggleProfetGUI();
#endif
}

bool DianaMainWindow::ProfetUpdatePlot(const miutil::miTime& t){
#ifdef PROFET
  if(profetGUI){
    if( profetGUI->selectTime(t)) {
      return true;
    }
  }
#endif
  return false;
}

bool DianaMainWindow::ProfetRightMouseClicked(float map_x,
    float map_y,
    int globalX,
    int globalY){
#ifdef PROFET
//if(togglePaintModeAction->isChecked()){
  if (paintToolBar->isVisible()) {
    profetGUI->rightMouseClicked(map_x,map_y,globalX,globalY);
    return true;
  }
#endif
  return false;
}

void DianaMainWindow::getFieldPlotOptions(map< miutil::miString, map<miutil::miString,miutil::miString> >& options)
{
  if (fm){
    fm->getEditPlotOptions(options);
  }
}


void DianaMainWindow::objMenu()
{
  bool b = objm->isVisible();
  if (b){
    objm->hideAll();
  } else {
    objm->showAll();
  }
  showObjectDialogAction->setChecked( !b );
}

void DianaMainWindow::trajMenu()
{
  bool b = trajm->isVisible();
  if (b){
    trajm->hide();
  } else {
    trajm->showplus();
  }
  updateGLSlot();
  showTrajecDialogAction->setChecked( !b );
}

void DianaMainWindow::measurementsMenu()
{
  bool b = measurementsm->isVisible();
  if (b){
    measurementsm->hide();
  } else {
    measurementsm->showplus();
  }
  updateGLSlot();
  showMeasurementsDialogAction->setChecked( !b );
}

void DianaMainWindow::vprofMenu()
{
  if ( !vpWindow ) return;
  if (vpWindow->active) {
    vpWindow->raise();
  } else {
    vprofStartup();
    updateGLSlot();
  }
}


void DianaMainWindow::vcrossMenu()
{
  if ( !vcWindow ) return;
  if (vcWindow->active) {
    vcWindow->raise();
  } else {
    vcrossStartup();
    updateGLSlot();
  }
}


void DianaMainWindow::spectrumMenu()
{
  if ( !spWindow ) return;
  if (spWindow->active) {
    spWindow->raise();
  } else {
    spectrumStartup();
    updateGLSlot();
  }
}


void DianaMainWindow::info_activated(QAction *action)
{
  if (action && infoFiles.count(action->text().toStdString())){
    if ( infoFiles[action->text().toStdString()].filename.contains("http") ) {
      QDesktopServices::openUrl(QUrl(infoFiles[action->text().toStdString()].filename.c_str()));
    } else {
      TextDialog* td= new TextDialog(this, infoFiles[action->text().toStdString()]);
      td->show();
    }
  }
}


void DianaMainWindow::vprofStartup()
{
  if ( !vpWindow ) return;
  if (vpWindow->firstTime) MenuOK();
  miutil::miTime t;
  contr->getPlotTime(t);
  vpWindow->startUp(t);
  vpWindow->show();
  contr->stationCommand("show","vprof");
}


void DianaMainWindow::vcrossStartup()
{
  if ( !vcWindow ) return;
  if (vcWindow->firstTime) MenuOK();
  miutil::miTime t;
  contr->getPlotTime(t);
  vcWindow->startUp(t);
  vcWindow->show();
}


void DianaMainWindow::spectrumStartup()
{
  if ( !spWindow ) return;
  if (spWindow->firstTime) MenuOK();
  miutil::miTime t;
  contr->getPlotTime(t);
  spWindow->startUp(t);
  spWindow->show();
  contr->stationCommand("show","spectrum");
}


void DianaMainWindow::hideVprofWindow()
{
#ifdef DEBUGPRINT
  cerr << "hideVprofWindow called !" << endl;
#endif
  if ( !vpWindow ) return;
  vpWindow->hide();
  // delete stations
  contr->stationCommand("delete","vprof");
  updateGLSlot();
}


void DianaMainWindow::hideVcrossWindow()
{
#ifdef DEBUGPRINT
  cerr << "hideVcrossWindow called !" << endl;
#endif
  if ( !vcWindow ) return;
  // vcWindow hidden and locationPlots deleted
  vcWindow->hide();
  contr->deleteLocation("vcross");
  updateGLSlot();
}


void DianaMainWindow::hideSpectrumWindow()
{
#ifdef DEBUGPRINT
  cerr << "hideSpectrumWindow called !" << endl;
#endif
  if ( !spWindow ) return;
  // spWindow and stations only hidden, should also be possible to
  //delete all !
  spWindow->hide();
  // delete stations
  contr->stationCommand("hide","spectrum");
  updateGLSlot();
}


void DianaMainWindow::stationChangedSlot(const QString& station)
{
#ifdef DEBUGPRINT
  //cerr << "DianaMainWindow::stationChangedSlot to " << station << endl;
  cerr << "DianaMainWindow::stationChangedSlot" << endl;
#endif
  miutil::miString s =station.toStdString();
  vector<miutil::miString> data;
  data.push_back(s);
  contr->stationCommand("setSelectedStation",data,"vprof");
  w->updateGL();
}


void DianaMainWindow::modelChangedSlot()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::modelChangedSlot()" << endl;
#endif
  if ( !vpWindow ) return;
  StationPlot * sp = vpWindow->getStations() ;
  sp->setName("vprof");
  sp->setImage("vprof_normal","vprof_selected");
  sp->setIcon("vprof_icon");
  contr->putStations(sp);
  updateGLSlot();
}


void DianaMainWindow::crossectionChangedSlot(const QString& name)
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::crossectionChangedSlot to " << name.toStdString() << endl;
  //cerr << "DianaMainWindow::crossectionChangedSlot " << endl;
#endif
  miutil::miString s= name.toStdString();
  contr->setSelectedLocation("vcross", s);
  w->updateGL();
}


void DianaMainWindow::crossectionSetChangedSlot()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::crossectionSetChangedSlot()" << endl;
#endif
  if ( !vcWindow ) return;
  LocationData ed;
  vcWindow->getCrossections(ed);
  ed.name= "vcross";
  if (ed.elements.size())
    contr->putLocation(ed);
  else
    contr->deleteLocation(ed.name);
  updateGLSlot();
}


void DianaMainWindow::crossectionSetUpdateSlot()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::crossectionSetUpdateSlot()" << endl;
#endif
  if ( !vcWindow ) return;
  LocationData ed;
  vcWindow->getCrossectionOptions(ed);
  ed.name= "vcross";
  contr->updateLocation(ed);
  updateGLSlot();
}


void DianaMainWindow::spectrumChangedSlot(const QString& station)
{
#ifdef DEBUGPRINT
  //cerr << "DianaMainWindow::spectrumChangedSlot to " << name << endl;
  cerr << "DianaMainWindow::spectrumChangedSlot" << endl;
#endif
  miutil::miString s =station.toStdString();
  vector<miutil::miString> data;
  data.push_back(s);
  contr->stationCommand("setSelectedStation",data,"spectrum");
  //  contr->setSelectedStation(s, "spectrum");
  w->updateGL();
}


void DianaMainWindow::spectrumSetChangedSlot()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::spectrumSetChangedSlot()" << endl;
#endif
  if ( !spWindow ) return;
  StationPlot * stp = spWindow->getStations() ;
  stp->setName("spectrum");
  stp->setImage("spectrum_normal","spectrum_selected");
  stp->setIcon("spectrum_icon");
  contr->putStations(stp);
  updateGLSlot();
}


void DianaMainWindow::connectionClosed()
{
  //  cerr <<"Connection closed"<<endl;
  qsocket = false;

  contr->stationCommand("delete","all");
  miutil::miString dummy;
  contr->areaCommand("delete","all","all",-1);

  //remove times
  vector<miutil::miString> type = timecontrol->deleteType(-1);
  for(unsigned int i=0;i<type.size();i++)
    tslider->deleteType(type[i]);

  textview->hide();
  contr->processHqcCommand("remove");
  om->setPlottype("Hqc_synop",false);
  om->setPlottype("Hqc_list",false);
  MenuOK();
  if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

}


void DianaMainWindow::processLetter(miMessage &letter)
{
  miutil::miString from(letter.from);
  //  cerr<<"Command: "<<letter.command<<"  ";
  //  cerr <<endl;
  //   cerr<<" Description: "<<letter.description<<"  ";
  //  cerr <<endl;
  //   cerr<<" commonDesc: "<<letter.commondesc<<"  ";
  //  cerr <<endl;
  //   cerr<<" Common: "<<letter.common<<"  ";
  //  cerr <<endl;
  //   for(unsigned size_t i=0;i<letter.data.size();i++)
  //    if(letter.data[i].length()<80)
  //       cerr <<" data["<<i<<"]:"<<letter.data[i]<<endl;;
  //    cerr <<" From: "<<from<<endl;
  //   cerr <<"To: "<<letter.to<<endl;

  if( letter.command == "menuok"){
    MenuOK();
  }

  else if( letter.command == qmstrings::init_HQC_params){
    if( contr->initHqcdata(letter.from,
        letter.commondesc,
        letter.common,
        letter.description,
        letter.data) ){
      if(letter.common.contains("synop"))
        om->setPlottype("Hqc_synop",true);
      else
        om->setPlottype("Hqc_list",true);
      hqcTo = letter.from;
    }
  }

  else if( letter.command == qmstrings::update_HQC_params){
    contr->updateHqcdata(letter.commondesc,letter.common,
        letter.description,letter.data);
    MenuOK();
  }

  else if( letter.command == qmstrings::select_HQC_param){
    contr->processHqcCommand("flag",letter.common);
    contr->updatePlots();
  }

  else   if( letter.command == qmstrings::station){
    contr->processHqcCommand("station",letter.common);
    contr->updatePlots();
  }

  else   if( letter.command == qmstrings::apply_quickmenu){
    //data[0]=menu, data[1]=item
    if(letter.data.size()==2) {
      qm->applyItem(letter.data[0],letter.data[1]);
      qm->applyPlot();
    } else {
      MenuOK();
    }
  }

  else if( letter.command == qmstrings::vprof){
    //description: lat:lon
    vprofMenu();
    if(letter.data.size()){
      vector<miutil::miString> tmp= letter.data[0].split(":");
      if(tmp.size()==2){
        float lat= atof(tmp[0].c_str());
        float lon= atof(tmp[1].c_str());
        float x=0, y=0;
        contr->GeoToPhys(lat,lon,x,y);
        int ix= int(x);
        int iy= int(y);
        //find the name of station we clicked at (from plotModul->stationPlot)
        miutil::miString station = contr->findStation(ix,iy,letter.command);
        //now tell vpWindow about new station (this calls vpManager)
        if (vpWindow && !station.empty()) vpWindow->changeStation(station);
      }
    }
  }

  else if( letter.command == qmstrings::vcross){
    //description: name
    vcrossMenu();
    if(letter.data.size()){
        //tell vcWindow to plot this corssection
        if (vcWindow) vcWindow->changeCrossection(letter.data[0]);
    }
  }

  else if (letter.command == qmstrings::addimage){
    // description: name:image

    QtImageGallery ig;
    int n = letter.data.size();
    for(int i=0; i<n; i++){
      // separate name and data
      vector<miutil::miString> vs= letter.data[i].split(":");
      if (vs.size()<2) continue;
      ig.addImageToGallery(vs[0], vs[1]);
    }

  }

  else if (letter.command == qmstrings::positions ){
    //commondesc: dataSet:image:normal:selected:icon:annotation
    //description: stationname:lat:lon:image:alpha

    //positions from diana, nothing to do
    if(letter.common == "diana") return;

    //obsolete -> new syntax
    if(letter.description.contains(";")){
      vector<miutil::miString> desc = letter.description.split(";");
      if( desc.size() < 2 ) return;
      miutil::miString dataSet = desc[0];
      letter.description=desc[1];
      letter.commondesc = "dataset:" + letter.commondesc;
      letter.common = dataSet + ":" + letter.common;
    }

    contr->makeStationPlot(letter.commondesc,letter.common,
        letter.description,
        letter.from,letter.data);

    //    sendSelectedStations(qmstrings::selectposition);
    return;
  }

  else if (letter.command == qmstrings::seteditpositions ){
    //     commondesc = dataset;
    //     description = name;
    contr->stationCommand("setEditStations",
        letter.data,letter.common,letter.from);
    //    sendSelectedStations(qmstrings::selectposition);
  }

  else if (letter.command == qmstrings::annotation ){
    //     commondesc = dataset;
    //     description = annotation;
    contr->stationCommand("annotation",
        letter.data,letter.common,letter.from);
  }

  else if (letter.command == qmstrings::showpositions ){
    //description: dataset
    contr->stationCommand("show",letter.description,letter.from);
    if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

  }

  else if (letter.command == qmstrings::hidepositions ){
    //description: dataset
    contr->stationCommand("hide",letter.description,letter.from);
    if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

  }

  //   else if (letter.command == qmstrings::changeimage ){ //Obsolete
  //     //description: dataset;stationname:image
  //     //find name of data set from description
  //     vector<miutil::miString> desc = letter.description.split(";");
  //     if( desc.size() < 2 ) return;
  //       contr->stationCommand("changeImageandText",
  // 			    letter.data,desc[0],letter.from,desc[1]);
  //   }

  else if (letter.command == qmstrings::changeimageandtext ){
    //cerr << "Change text and image\n";
    //description: dataSet;stationname:image:text:alignment
    //find name of data set from description
    vector<miutil::miString> desc = letter.description.split(";");
    if( desc.size() == 2 ) { //obsolete syntax
      contr->stationCommand("changeImageandText",
          letter.data,desc[0],letter.from,desc[1]);
    } else { //new syntax
      //commondesc: name of dataset
      //description: stationname:image:image2:text
      //             :alignment:rotation (0-7):alpha

      contr->stationCommand("changeImageandText",
          letter.data,letter.common,letter.from,
          letter.description);
    }
  }

  //   else if (letter.command == qmstrings::changeimageandimage ){ //Obsolete
  //     //cerr << "Change image and image\n";
  //     //description: dataset;stationname:image:image2
  //     //find name of data set from description
  //     vector<miutil::miString> desc = letter.description.split(";");
  //     if( desc.size() < 2 ) return;
  //       contr->stationCommand("changeImageandText",
  // 			    letter.data,desc[0],letter.from,desc[1]);
  //   }

  else if (letter.command == qmstrings::selectposition ){
    //commondesc: dataset
    //description: stationName
    contr->stationCommand("selectPosition",
        letter.data,letter.common,letter.from);
  }

  else if (letter.command == qmstrings::showpositionname ){
    //description: normal:selected
    contr->stationCommand("showPositionName",
        letter.data,letter.common,letter.from);
  }

  else if (letter.command == qmstrings::showpositiontext ){
    //description: showtext:colour:size
    contr->stationCommand("showPositionText",letter.data,
        letter.common,letter.from,letter.description);
  }

  else if (letter.command == qmstrings::areas ){
    if(letter.data.size()>0)
      contr->makeAreas(letter.common,letter.data[0],letter.from);
    if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());
  }

  else if (letter.command == qmstrings::areacommand ){
    //commondesc command:dataSet
    vector<miutil::miString> token = letter.common.split(":");
    if(token.size()>1){
      int n = letter.data.size();
      if(n==0) 	contr->areaCommand(token[0],token[1],miutil::miString(),letter.from);
      for( int i=0;i<n;i++ )
        contr->areaCommand(token[0],token[1],letter.data[i],letter.from);
    }
  }

  else if (letter.command == qmstrings::selectarea ){
    //commondesc dataSet
    //description name,on/off
    int n = letter.data.size();
    for( int i=0;i<n;i++ ){
      contr->areaCommand("select",letter.common,letter.data[i],letter.from);
    }
  }

  else if (letter.command == qmstrings::showarea ){
    //commondesc dataSet
    //description name,on/off
    int n = letter.data.size();
    for( int i=0;i<n;i++ ){
      contr->areaCommand("show",letter.common,letter.data[i],letter.from);
    }
  }

  else if (letter.command == qmstrings::changearea ){
    //commondesc dataSet
    //description name,colour
    int n = letter.data.size();
    for( int i=0;i<n;i++ ){
      contr->areaCommand("setcolour",letter.common,letter.data[i],letter.from);
    }
  }

  else if (letter.command == qmstrings::deletearea ){
    //commondesc dataSet
    contr->areaCommand("delete",letter.common,"all",letter.from);
    if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());
  }

  else if (letter.command == qmstrings::showtext ){
    //description: station:text
    if(letter.data.size()){
      textview_id = letter.from;
      vector<miutil::miString> token = letter.data[0].split(1,":",true);
      if(token.size() == 2){
        miutil::miString name = pluginB->getClientName(letter.from);
        textview->setText(textview_id,name,token[1]);
        textview->show();
      }
    }
  }

  else if (letter.command == qmstrings::enableshowtext ){
    //description: dataset:on/off
    if(letter.data.size()){
      vector<miutil::miString> token = letter.data[0].split(":");
      if(token.size() < 2) return;
      if(token[1] == "on"){
        textview->show();
      } else {
        textview->deleteTab(letter.from);
      }
    }
  }

  else if (letter.command == qmstrings::removeclient ){
    // commondesc = id:dataset
    vector<miutil::miString> token = letter.common.split(":");
    if(token.size()<2) return;
    int id =atoi(token[0].c_str());
    //remove stationPlots from this client
    contr->stationCommand("delete","",id);
    //remove areas from this client
    contr->areaCommand("delete","all","all",id);
    //remove times
    vector<miutil::miString> type = timecontrol->deleteType(id);
    for(unsigned int i=0;i<type.size();i++)
      tslider->deleteType(type[i]);
    //hide textview
    textview->deleteTab(id);
    //     if(textview && id == textview_id )
    //       textview->hide();
    //remove observations from hqc
    if(token[1].downcase()=="hqc"){
      contr->processHqcCommand("remove");
      om->setPlottype("Hqc_synop",false);
      om->setPlottype("Hqc_list",false);
      MenuOK();
    }
    if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

  }

  else if (letter.command == qmstrings::newclient ){
    qsocket = true;
    autoredraw[atoi(letter.common.cStr())] = true;
    autoredraw[0] = true; //from server
  }

  else if (letter.command == qmstrings::autoredraw ){
    if(letter.common == "false")
      autoredraw[letter.from] = false;
  }

  else if (letter.command == qmstrings::redraw ){
    w->updateGL();
  }

  else if (letter.command == qmstrings::settime ){
    int n = letter.data.size();
    if(letter.commondesc == "datatype"){
      timecontrol->useData(letter.common,letter.from);
      vector<miutil::miTime> times;
      for(int i=0;i<n;i++)
        times.push_back(letter.data[i]);
      tslider->insert(letter.common,times);
      contr->initHqcdata(letter.from,letter.commondesc,
          letter.common,letter.description,letter.data);

    } else if (letter.commondesc == "time"){
      miutil::miTime t(letter.common);
      tslider->setTime(t);
      contr->setPlotTime(t);
      timeChanged();
      contr->updatePlots();
    }
  }

  else if (letter.command == qmstrings::getcurrentplotcommand) {
    vector<miutil::miString> v1, v2, v3;
    getPlotStrings(v1, v2, v3);

    miMessage l;
    l.to = letter.from;
    l.command = qmstrings::currentplotcommand;
    l.data = v1;
    sendLetter(l);
    return; // no need to repaint
  }

  else if (letter.command == qmstrings::getproj4maparea) {
    miMessage l;
    l.to = letter.from;
    l.command = qmstrings::proj4maparea;
    l.data.push_back(contr->getMapArea().getAreaString());
    sendLetter(l);
    return; // no need to repaint
  }

  else if (letter.command == qmstrings::getmaparea) {
    miMessage l;
    l.to = letter.from;
    l.command = qmstrings::maparea;
    l.data.push_back(contr->getMapArea().toString());
    sendLetter(l);
    return; // no need to repaint
  }

  // If autoupdate is active, reread sat/radarfiles and
  // show the latest timestep
  else if (letter.command == qmstrings::directory_changed) {
#ifdef DEBUGPRINT
    cerr << letter.command <<" received" << endl;
#endif

    if (doAutoUpdate) {
      // running animation
      if (timeron != 0) {
        om->getTimes();
        sm->RefreshList();
        if (contr->satFileListChanged() || contr->obsTimeListChanged()) {
          //cerr << "new satfile or satfile deleted!" << endl;
          //cerr << "setPlotTime" << endl;
          //	  cerr << "doAutoUpdate  timer on" << endl;
          contr->satFileListUpdated();
          contr->obsTimeListUpdated();
        }
      }
      else {
        // Avoid not needed updates
        QApplication::setOverrideCursor( Qt::WaitCursor );
        // what to do with om->getTimes() ?
        om->getTimes();
        sm->RefreshList();
        miutil::miTime tp = tslider->Value();
        tslider->setLastTimeStep();
        miutil::miTime t= tslider->Value();
        // Check if slider was not on latest timestep
        // or new image file arrived.
        // If t > tp force repaint...
        if (contr->satFileListChanged() || contr->obsTimeListChanged() || (t > tp))
        {
          //cerr << "new satfile or satfile deleted!" << endl;
          //cerr << "setPlotTime" << endl;
          setPlotTime(t);
          contr->satFileListUpdated();
          contr->obsTimeListUpdated();
        }
        //cerr << "stepforward" << endl;
        stepforward();
        QApplication::restoreOverrideCursor();
      }
    }
  }

  // If autoupdate is active, do the same thing as
  // when the user presses the updateObs button.
  else if (letter.command == qmstrings::file_changed) {
#ifdef DEBUGPRINT
    cerr << letter.command <<" received" << endl;
#endif
    if (doAutoUpdate) {
      // Just a call to update obs will work fine
      updateObs();
    }
  }

  else {
    return; //nothing to do
  }

  // repaint window
  if(autoredraw[letter.from])
    w->updateGL();

}

void DianaMainWindow::sendPrintClicked(int id)
{
  miMessage l;
  l.command = qmstrings::printclicked;
  sendLetter(l);
}


void DianaMainWindow::sendLetter(miMessage& letter)
{
  pluginB->sendMessage(letter);
  //   cerr <<"SENDING>>>>"<<endl;
  //   cerr<<"Command: "<<letter.command<<endl;
  //   cerr<<"Description: "<<letter.description<<endl;
  //   cerr<<"commonDesc: "<<letter.commondesc<<endl;
  //   cerr<<"Common: "<<letter.common<<endl;
  //       for(size_t i=0;i<letter.data.size();i++)
  // 	cerr<<"data:"<<letter.data[i]<<endl;
  //   cerr <<"To: "<<letter.to<<endl;
}

void DianaMainWindow::updateObs()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::obsUpdate()" << endl;
#endif
  QApplication::setOverrideCursor( Qt::WaitCursor );
  contr->updateObs();
  w->updateGL();
  QApplication::restoreOverrideCursor();

}

void DianaMainWindow::autoUpdate()
{
  doAutoUpdate = !doAutoUpdate;
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::autoUpdate(): " << doAutoUpdate << endl;
#endif
  autoUpdateAction->setChecked(doAutoUpdate);
}

void DianaMainWindow::updateGLSlot()
{
  w->updateGL();
  if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());
}


void DianaMainWindow::showHelp()
{
  if(help->isVisible())
    help->hide();
  else
    help->showdoc(0,"index.html");
}


void DianaMainWindow::showAccels()
{
  help->showdoc(1,"ug_shortcutkeys.html");
}


void DianaMainWindow::showNews()
{
  help->showdoc(2,"news.html");
}

void DianaMainWindow::showUrl()
{
  QDesktopServices::openUrl(QUrl(LocalSetupParser::basicValue("bugzilla").c_str()));
}

void DianaMainWindow::about()
{
  QString str =
      tr("Diana - a 2D presentation system for meteorological data, including fields, observations,\nsatellite- and radarimages, vertical profiles and cross sections.\nDiana has tools for on-screen fieldediting and drawing of objects (fronts, areas, symbols etc.\n")+"\n" + tr("To report a bug or enter an enhancement request, please use the bug tracking tool at http://diana.bugs.met.no (met.no users only). \n") +"\n\n"+ tr("version:") + " " + version_string.c_str()+"\n"+ tr("build:") + " " + build_string.c_str();

  QMessageBox::about( this, tr("about Diana"), str );
}


void DianaMainWindow::TimeSliderMoved()
{
  miutil::miTime t= tslider->Value();
  timelabel->setText(t.isoTime().c_str());
}

void DianaMainWindow::TimeSelected()
{
  //Timeslider released
  miutil::miTime t= tslider->Value();
  if (!dialogChanged){
    setPlotTime(t);
  }
}

void DianaMainWindow::SliderSet()
{
  miutil::miTime t= tslider->Value();
  contr->setPlotTime(t);
  TimeSliderMoved();
}

void DianaMainWindow::setTimeLabel()
{
  miutil::miTime t;
  contr->getPlotTime(t);
  tslider->setTime(t);
  TimeSliderMoved();
}

void DianaMainWindow::stopAnimation()
{
  timeBackwardAction->setChecked( false );
  timeForewardAction->setChecked( false );

  killTimer(animationTimer);
  timeron=0;

}

void DianaMainWindow::animationLoop()
{
  timeloop= !timeloop;
  tslider->setLoop(timeloop);

  timeLoopAction->setChecked( timeloop );
}

void DianaMainWindow::timerEvent(QTimerEvent *e)
{
  if (e->timerId()==animationTimer){
    miutil::miTime t;
    if (!tslider->nextTime(timeron, t, true)){
      stopAnimation();
      return;
    }
    setPlotTime(t);
  }
}

void DianaMainWindow::animation()
{
  if (timeron!=0)
    stopAnimation();

  timeForewardAction->setChecked( true );

  tslider->startAnimation();
  animationTimer= startTimer(timeout_ms);
  timeron=1;
}

void DianaMainWindow::animationBack()
{
  if (timeron!=0)
    stopAnimation();

  timeBackwardAction->setChecked( true );

  tslider->startAnimation();
  animationTimer= startTimer(timeout_ms);
  timeron=-1;
}

void DianaMainWindow::animationStop()
{
  stopAnimation();
}

void DianaMainWindow::stepforward()
{
  if (timeron) return;
  miutil::miTime t;
  if (!tslider->nextTime(1, t)) return;
  setPlotTime(t);
}

void DianaMainWindow::stepback()
{
  if (timeron) return;
  miutil::miTime t;
  if (!tslider->nextTime(-1, t)) return;
  setPlotTime(t);
}

void DianaMainWindow::decreaseTimeStep()
{
  int v= timestep->value() - timestep->singleStep();
  if (v<0) v=0;
  timestep->setValue(v);
}

void DianaMainWindow::increaseTimeStep()
{
  int v= timestep->value() + timestep->singleStep();
  timestep->setValue(v);
}


void DianaMainWindow::setPlotTime(miutil::miTime& t)
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  if (contr->setPlotTime(t)) {
    contr->updatePlots();
    if( !ProfetUpdatePlot(t)){
      w->updateGL();
    }
  }
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("diana.DianaMainWindow.setPlotTime");
  COMMON_LOG::getInstance("common").infoStream() << "Plottime: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  timeChanged();
  QApplication::restoreOverrideCursor();

}

void DianaMainWindow::timeChanged(){
  //to be done whenever time changes (step back/forward, MenuOK etc.)
  objm->commentUpdate();
  satFileListUpdate();
  setTimeLabel();
  miutil::miTime t;
  contr->getPlotTime(t);
  if (vpWindow) vpWindow->mainWindowTimeChanged(t);
  if (spWindow) spWindow->mainWindowTimeChanged(t);
  if (vcWindow) vcWindow->mainWindowTimeChanged(t);
  if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

  //update sat channels in statusbar
  vector<miutil::miString> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);

  if(qsocket){
    miMessage letter;
    letter.command= qmstrings::timechanged;
    letter.commondesc= "time";
    letter.common = t.isoTime();
    letter.to = qmstrings::all;
    sendLetter(letter);
  }

  QCoreApplication::sendPostedEvents ();

}


void DianaMainWindow::levelUp()
{
  if (toolLevelUpAction->isEnabled())
    levelChange(1);
}


void DianaMainWindow::levelDown()
{
  if (toolLevelDownAction->isEnabled())
    levelChange(-1);
}


void DianaMainWindow::levelChange(int increment)
{

  QApplication::setOverrideCursor( Qt::WaitCursor );
  // update field dialog and FieldPlots
  contr->updateFieldPlot(fm->changeLevel(increment,0));
  updateGLSlot();
  QApplication::restoreOverrideCursor();
  toolLevelUpAction->  setEnabled(fm->levelsExists(true,0));
  toolLevelDownAction->setEnabled(fm->levelsExists(false,0));
}


void DianaMainWindow::idnumUp()
{
  if (toolIdnumUpAction->isEnabled())
    idnumChange(1);
}


void DianaMainWindow::idnumDown()
{
  if (toolIdnumDownAction->isEnabled())
    idnumChange(-1);
}


void DianaMainWindow::idnumChange(int increment)
{

  QApplication::setOverrideCursor( Qt::WaitCursor );
  // update field dialog and FieldPlots
  contr->updateFieldPlot(fm->changeLevel(increment, 1));
  updateGLSlot();
  QApplication::restoreOverrideCursor();
  toolIdnumUpAction->  setEnabled(fm->levelsExists(true,1));
  toolIdnumDownAction->setEnabled(fm->levelsExists(false,1));

}


void DianaMainWindow::saveraster()
{
  static QString fname = "./"; // keep users preferred image-path for later

  QString s =
      QFileDialog::getSaveFileName(this,
          tr("Save plot as image"),
          fname,
          tr("Images (*.png *.xpm *.bmp *.eps);;All (*.*)"));


  if (!s.isNull()) {// got a filename
    fname= s;
    miutil::miString filename= s.toStdString();
    miutil::miString format= "PNG";
    int quality= -1; // default quality

    // find format
    if (filename.contains(".xpm") || filename.contains(".XPM"))
      format= "XPM";
    else if (filename.contains(".bmp") || filename.contains(".BMP"))
      format= "BMP";
    else if (filename.contains(".eps") || filename.contains(".epsf")){
      // make encapsulated postscript
      // NB: not screendump!
      makeEPS(filename);
      return;
    }

    // do the save
    w->Glw()->saveRasterImage(filename, format, quality);
  }
}

void DianaMainWindow::saveRasterImage(QString filename) {

  miutil::miString fname = filename.toStdString();
  miutil::miString format= "PNG";
  int quality= -1; // default quality
  w->Glw()->saveRasterImage(fname, format, quality);

}

void DianaMainWindow::emailPicture() {
  mailm->show();
}


#ifdef VIDEO_EXPORT
void DianaMainWindow::saveAnimation() {
  static QString fname = "./"; // keep users preferred animation-path for later

  QString s =
      QFileDialog::getSaveFileName(this,
          tr("Save animation from current fields, satellite images, etc. (*.mpg or *.avi)"),
          fname,
          tr("Movies (*.mpg *.avi);;All (*.*)"));


  if (!s.isNull()) {// got a filename
    fname = s;

    const QString suffix = QFileInfo(s).suffix();
    miutil::miString format;

    if (!suffix.compare(QLatin1String("mpg"), Qt::CaseInsensitive)) {
      format = "mpg";
    } else if (!suffix.compare(QLatin1String("avi"), Qt::CaseInsensitive)) {
      format = "avi";
    } else { // default to mpg
      format = "mpg";
      s += ".mpg";
    }

    miutil::miString filename = s.toStdString();

    float delay = timeout_ms * 0.001;
    MovieMaker moviemaker(filename, format, delay);

    // save current sizes
    QSize mainWindowSize = size();
    QSize workAreaSize = w->Glw()->size();

    QMessageBox::information(
        this,
        tr("Making animation"),
        tr(
            "This may take some time, depending on the number of timesteps and selected delay. Diana cannot be used until this process is completed. A message will be displayed upon completion. Press OK to begin."));
    resize(1440, 850); ///< w/o this, grabFrameBuffer() only returns the (OpenGL-)part of the WorkArea visible in the DianaMainWindow
    showMinimized();

    // first reset time-slider
    miutil::miTime startTime = tslider->getStartTime();
    tslider->set(startTime);
    setPlotTime(startTime);

    int nrOfTimesteps = tslider->numTimes();
    int i = 0;

    int maxProgress = nrOfTimesteps - tslider->current() - 1;
    QProgressDialog progress(tr("Creating animation..."), tr("Hide"),
        0, maxProgress);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    /// save frames as images
    while(tslider->current() < nrOfTimesteps-1) {
      /// update progressbar
      if(!progress.isHidden())
        progress.setValue(i);

      w->Glw()->resize(1280, 720);
      QImage image = w->Glw()->grabFrameBuffer(true);
      moviemaker.addImage(image);

      /// go to next frame
      stepforward();

      ++i;
    }
    if(!progress.isHidden())
      progress.setValue(i);

    // restore sizes
    w->Glw()->resize(workAreaSize);
    resize(mainWindowSize);

    showNormal();
    QMessageBox::information(this, tr("Done"), tr("Animation completed."));
  }
}
#else
void DianaMainWindow::saveAnimation() {
  QMessageBox::information(this, tr("Compiled without video export"), tr("Diana must be compiled with VIDEO_EXPORT defined to use this feature."));
}
#endif

void DianaMainWindow::makeEPS(const miutil::miString& filename)
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
  printOptions priop;
  priop.fname= filename;
  priop.colop= d_print::incolour;
  priop.orientation= d_print::ori_automatic;
  priop.pagesize= d_print::A4;
  priop.numcopies= 1;
  priop.usecustomsize= false;
  priop.fittopage= false;
  priop.drawbackground= true;
  priop.doEPS= true;

#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
  w->Glw()->startHardcopy(priop);
  w->updateGL();
  w->Glw()->endHardcopy();
  w->updateGL();
#endif

  QApplication::restoreOverrideCursor();
}


void DianaMainWindow::parseSetup()
{
  cerr<<" DianaMainWindow::parseSetup()"<<endl;
  SetupDialog *setupDialog = new SetupDialog(this);

  if( setupDialog->exec() ) {

    LocalSetupParser sp;
    miutil::miString filename;
    if (!sp.parse(filename)){
      cerr << "An error occured while re-reading setup " << endl;
    }
    contr->parseSetup();
    vcWindow->parseSetup();
    vpWindow->parseSetup();
    spWindow->parseSetup();
  }
}

void DianaMainWindow::hardcopy()
{
  QPrinter qprt;

  miutil::miString command= pman.printCommand();

  fromPrintOption(qprt,priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    if (!qprt.outputFileName().isNull()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else {
      priop.fname="/tmp/";
      if (getenv("TMP") != NULL) {
        priop.fname=getenv("TMP");
      }
      priop.fname+= "prt_" + miutil::miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // set printername
    if (qprt.outputFileName().isNull())
      priop.printer= qprt.printerName().toStdString();

    // start the postscript production
    QApplication::setOverrideCursor( Qt::WaitCursor );
    //     contr->startHardcopy(priop);
#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
    w->Glw()->startHardcopy(priop);
    w->updateGL();
    w->Glw()->endHardcopy();
#else
    w->Glw()->print(&qprt);
#endif
    w->updateGL();

    // if output to printer: call appropriate command
    if (qprt.outputFileName().isNull()){
      //From Qt:On Windows, Mac OS X and X11 systems that support CUPS,
      //this will always return 1 as these operating systems can internally
      //handle the number of copies. (Doesn't work)
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      //########################################################################
      cerr<<"PRINT: "<< command << endl;
      //########################################################################
      int res = system(command.c_str());

      if (res != 0){
        cerr << "Print command:" << command << " failed" << endl;
      }

    }
    QApplication::restoreOverrideCursor();

    //     // reset number of copies (saves a lot of paper)
    //     qprt.setNumCopies(1);
  }
}

// left mouse click -> mark trajectory position
void DianaMainWindow::trajPositions(bool b)
{
  markTrajPos = b;
  markMeasurementsPos = false;
  markVcross = false;

}

void DianaMainWindow::measurementsPositions(bool b)
{
  markMeasurementsPos = b;
  markTrajPos = false;
  markVcross = false;

}

void DianaMainWindow::vCrossPositions(bool b)
{
#ifdef DEBUGPRINT
  cerr << "vCrossPositions b=" << b << endl;
#endif
  markMeasurementsPos = false;
  markTrajPos = false;
  markVcross = b;
}

// picks up a single click on position x,y
void DianaMainWindow::catchMouseGridPos(const mouseEvent mev)
{

  int x = mev.x;
  int y = mev.y;

  float lat=0,lon=0;
  contr->PhysToGeo(x,y,lat,lon);

  if(markTrajPos){
    trajm->mapPos(lat,lon);
    w->updateGL(); // repaint window
  }

  if(markMeasurementsPos) {
    measurementsm->mapPos(lat,lon);
    w->updateGL(); // repaint window
  }

  if (markVcross) {
    vcWindow->mapPos(lat,lon);
    w->updateGL(); // repaint window
  }

  if( !optAutoElementAction->isChecked() ){
    catchElement(mev);
  }

  if (mev.modifier==key_Control){
    if (uffda && contr->getSatnames().size()){
      showUffda();
    }
  }
  //send position to all clients
  if(qsocket){
    miutil::miString latstr(lat,6);
    miutil::miString lonstr(lon,6);
    miMessage letter;
    letter.command     = qmstrings::positions;
    letter.commondesc  =  "dataset";
    letter.common      =  "diana";
    letter.description =  "lat:lon";
    letter.to = qmstrings::all;
    letter.data.push_back(miutil::miString(latstr + ":" + lonstr));
    sendLetter(letter);
  }

  vector<Station*> allStations = contr->getStationManager()->findStations(mev.x, mev.y);
  vector<Station*> stations;
  for (unsigned int i = 0; i < allStations.size(); ++i) {
    if (allStations[i]->status != Station::noStatus)
      stations.push_back(allStations[i]);
  }

  if (stations.size() > 0) {

    // Count the number of times each station name appears in the list.
    // This is used later to decide whether or not to show the "auto" or
    // "vis" text.
    map<miutil::miString, unsigned int> stationNames;
    for (unsigned int i = 0; i < stations.size(); ++i) {
      unsigned int number = stationNames.count(stations[i]->name);
      stationNames[stations[i]->name] = number + 1;
    }

    QString stationsText = "<table>";
    for (unsigned int i = 0; i < stations.size(); ++i) {
      if (!stations[i]->isVisible)
        continue;

      stationsText += "<tr>";
      stationsText += "<td>";
      switch (stations[i]->status) {
      case Station::failed:
        stationsText += tr("<span style=\"background: red; color: red\">X</span>");
        break;
      case Station::underRepair:
        stationsText += tr("<span style=\"background: yellow; color: yellow\">X</span>");
        break;
      case Station::working:
        stationsText += tr("<span style=\"background: lightgreen; color: lightgreen\">X</span>");
        break;
      default:
        ;
      }
      stationsText += "</td>";

      stationsText += "<td>";
      stationsText += QString("<a href=\"%1\">%2</a>").arg(
          QString::fromStdString(stations[i]->url)).arg(QString::fromStdString(stations[i]->name));
      if (stationNames[stations[i]->name] > 1) {
        if (stations[i]->type == Station::automatic)
          stationsText += tr("&nbsp;auto");
        else
          stationsText += tr("&nbsp;vis");
      }
      stationsText += "</td>";

      stationsText += "<td>";
      if (stations[i]->lat >= 0)
        stationsText += tr("%1&nbsp;N,&nbsp;").arg(stations[i]->lat);
      else
        stationsText += tr("%1&nbsp;S,&nbsp;").arg(-stations[i]->lat);
      if (stations[i]->lon >= 0)
        stationsText += tr("%1&nbsp;E").arg(stations[i]->lon);
      else
        stationsText += tr("%1&nbsp;W").arg(-stations[i]->lon);
      stationsText += "</td>";

      stationsText += "</tr>";
    }
    stationsText += "</table>";

    QWhatsThis::showText(w->mapToGlobal(QPoint(mev.x, w->height() - mev.y)), stationsText, w);
  }
}


// picks up a single click on position x,y
void DianaMainWindow::catchMouseRightPos(const mouseEvent mev)
{
  //  cerr <<"void DianaMainWindow::catchMouseRightPos(const mouseEvent mev)"<<endl;

  int x = mev.x;
  int y = mev.y;
  int globalX = mev.globalX;
  int globalY = mev.globalY;


  float map_x,map_y;
  contr->PhysToMap(mev.x,mev.y,map_x,map_y);

  if (mev.modifier!=key_Shift &&
      ProfetRightMouseClicked(map_x,map_y,globalX,globalY)) {
    return;
  }

  xclick=x; yclick=y;

  for (int i=0; i<MaxSelectedAreas; i++){
    selectAreaAction[i]->setVisible(false);
  }

  vselectAreas=contr->findAreas(xclick,yclick);
  int nAreas=vselectAreas.size();
  if ( nAreas>0 ) {
    zoomOutAction->setVisible(true);
    for (int i=1; i<=nAreas && i<MaxSelectedAreas; i++){
      selectAreaAction[i]->setText(vselectAreas[i-1].name.cStr());
      selectAreaAction[i]->setData(i-1);
      selectAreaAction[i]->setVisible(true);
    }
    rightclickmenu->popup(QPoint(globalX, globalY), 0);
  } else {
    zoomOut();
  }

  return;
}


// picks up mousemovements (without buttonclicks)
void DianaMainWindow::catchMouseMovePos(const mouseEvent mev, bool quick)
{
#ifdef DEBUGREDRAWCATCH
  cerr<<"DianaMainWindow::catchMouseMovePos x,y: "<<mev.x<<" "<<mev.y<<endl;
#endif
  int x = mev.x;
  int y = mev.y;

  float xmap=-1., ymap=-1.;
  contr->PhysToMap(x,y,xmap,ymap);

  xclick=x; yclick=y;

  // show geoposition in statusbar
  if (sgeopos->geographicMode() )  {
    float lat=0, lon=0;
    if(contr->PhysToGeo(x,y,lat,lon)){
      sgeopos->setPosition(lat,lon);
    } else {
      sgeopos->undefPosition();
    }
  } else if (sgeopos->gridMode()) {
    float gridx=0, gridy=0;
    if(contr->MapToGrid(xmap,ymap,gridx,gridy)){
      sgeopos->setPosition(gridx,gridy);
    } else {
      sgeopos->undefPosition();
    }
  } else {
    sgeopos->setPosition(xmap,ymap);
  }

  // show sat-value at position
  vector<SatValues> satval;
  satval = contr->showValues(xmap, ymap);
  showsatval->ShowValues(satval);

  // no need for updateGL() here...only GUI-display

  //check if inside annotationPlot to edit
  if (contr->markAnnotationPlot(x,y))
    w->updateGL();

  if (quick) return;

  if( optAutoElementAction->isChecked() ){
    catchElement(mev);
  }
}


void DianaMainWindow::catchMouseDoubleClick(const mouseEvent mev)
{
}


void DianaMainWindow::catchElement(const mouseEvent mev)
{

#ifdef DEBUGREDRAWCATCH
  cerr<<"DianaMainWindow::catchElement x,y: "<<mev.x<<" "<<mev.y<<endl;
#endif
  int x = mev.x;
  int y = mev.y;

  bool needupdate= false; // updateGL necessary

  miutil::miString uffstation = contr->findStation(x,y,"uffda");
  if (!uffstation.empty()) uffm->pointClicked(uffstation);

  //show closest observation
  if( contr->findObs(x,y) ){
    needupdate= true;
  }

  //find the name of station we clicked/pointed
  //at (from plotModul->stationPlot)
  miutil::miString station = contr->findStation(x,y,"vprof");
  //now tell vpWindow about new station (this calls vpManager)
  if (vpWindow && !station.empty()) {
    vpWindow->changeStation(station);
    //  needupdate= true;
  }

  //find the name of station we clicked/pointed
  //at (from plotModul->stationPlot)
  station = contr->findStation(x,y,"spectrum");
  //now tell spWindow about new station (this calls spManager)
  if (spWindow && !station.empty()) {
    spWindow->changeStation(station);
    //  needupdate= true;
  }

  // locationPlots (vcross,...)
  miutil::miString crossection= contr->findLocation(x,y,"vcross");
  if (vcWindow && !crossection.empty()) {
    vcWindow->changeCrossection(crossection);
    //  needupdate= true;
  }

  if(qsocket){

    //set selected and send position to plugin connected
    vector<int> id;
    vector<miutil::miString> name;
    vector<miutil::miString> station;

    bool add = false;
    //    if(mev.modifier==key_Shift) add = true; //todo: shift already used (skip editmode)
    contr->findStations(x,y,add,name,id,station);
    int n = name.size();

    miMessage old_letter;
    old_letter.command = qmstrings::textrequest;
    miMessage letter;
    letter.command    = qmstrings::selectposition;
    letter.commondesc =  "name";
    letter.description =  "station";
    for(int i=0; i<n;i++){
      if(i>0 && id[i-1]!=id[i]){
        //send and clear letters
        sendLetter(old_letter);
        sendLetter(letter);
        old_letter.data.clear();
        letter.data.clear();
      }
      //Obsolete command and syntax
      old_letter.to = id[i];
      old_letter.description =  name[i]+";name";
      old_letter.data.push_back(station[i]);
      //New command and syntax
      letter.to         = id[i];
      letter.common     =  name[i];
      letter.data.push_back(station[i]);
    }
    if(letter.data.size()>0) {
      sendLetter(old_letter);
      sendLetter(letter);
      needupdate=true;
    }
    //send area to pliugin connected

    vector <selectArea> areas=contr->findAreas(x,y,true);
    int nareas = areas.size();
    if(nareas){
      miMessage letter;
      letter.command = qmstrings::selectarea;
      letter.description = "name:on/off";
      for(int i=0;i<nareas;i++){
        letter.to = areas[i].id;
        miutil::miString datastr = areas[i].name + ":on";
        letter.data.push_back(datastr);
        sendLetter(letter);
      }
    }

    if(hqcTo>0){
      miutil::miString name;
      if(contr->getObsName(x,y,name)){
        miMessage letter;
        letter.to = hqcTo;
        letter.command = qmstrings::station;
        letter.commondesc = "name,time";
        miutil::miTime t;
        contr->getPlotTime(t);
        letter.common = name + "," + t.isoTime();;
        sendLetter(letter);
      }
    }
  }

  // Perform general station selection, independent of tool-specific checks.
  vector<miutil::miString> names;
  vector<int> ids;
  vector<miutil::miString> stations;
  contr->findStations(x, y, false, names, ids, stations);

  if (needupdate) w->updateGL();
}

void DianaMainWindow::sendSelectedStations(const miutil::miString& command)
{
  vector<miutil::miString> data;
  contr->stationCommand("selected",data);
  int n=data.size();
  for(int i=0;i<n;i++){
    vector<miutil::miString> token = data[i].split(":");
    int m = token.size();
    if(token.size()<2) continue;
    int id=atoi(token[m-1].cStr());
    miutil::miString dataset = token[m-2];
    token.pop_back(); //remove id
    token.pop_back(); //remove dataset
    miMessage letter;
    letter.command    = command;
    letter.commondesc =  "dataset";
    letter.common=dataset;
    letter.description =  "name";
    letter.data = token;
    letter.to = id;
    sendLetter(letter);
  }
}



void DianaMainWindow::catchKeyPress(const keyboardEvent kev)
{
  if (!em->inedit() && qsocket) {

    if( kev.key == key_Plus || kev.key == key_Minus){
      miutil::miString dataset;
      int id;
      vector<miutil::miString> stations;
      contr->getEditStation(0,dataset,id,stations);
      if( dataset.exists() ){
        miMessage letter;
        letter.command = qmstrings::editposition;
        letter.commondesc = "dataset";
        letter.common = dataset;
        if(kev.modifier == key_Control)
          letter.description = "position:value_2";
        else if(kev.modifier == key_Alt)
          letter.description = "position:value_3";
        else
          letter.description = "position:value_1";
        for(unsigned int i=0;i<stations.size();i++){
          miutil::miString str = stations[i];
          if( kev.key == key_Plus )
            str += ":+1";
          else
            str += ":-1";
          letter.data.push_back(str);
        }
        letter.to = id;
        sendLetter(letter);
      }
    }

    else if( kev.modifier == key_Control){

      if( kev.key == key_C ) {
        sendSelectedStations(qmstrings::copyvalue);
      }

      else if( kev.key == key_U) {
        contr->stationCommand("unselect");
        sendSelectedStations(qmstrings::selectposition);
        w->updateGL();
      }

      else {
        miutil::miString keyString;
        if(kev.key == key_G) keyString = "ctrl_G";
        else if(kev.key == key_S)  keyString = "ctrl_S";
        else if(kev.key == key_Z)  keyString = "ctrl_Z";
        else if(kev.key == key_Y)  keyString = "ctrl_Y";
        else return;

        miMessage letter;
        letter.command    = qmstrings::sendkey;
        letter.commondesc =  "key";
        letter.common =  keyString;
        letter.to = qmstrings::all;;
        sendLetter(letter);
      }
    }

    else if( kev.modifier == key_Alt){
      miutil::miString keyString;
      if(kev.key == key_F5) keyString = "alt_F5";
      else if(kev.key == key_F6) keyString = "alt_F6";
      else if(kev.key == key_F7) keyString = "alt_F7";
      else if(kev.key == key_F8) keyString = "alt_F8";
      else return;

      miMessage letter;
      letter.command    = qmstrings::sendkey;
      letter.commondesc =  "key";
      letter.common =  keyString;
      letter.to = qmstrings::all;
      sendLetter(letter);
    }


    else if( kev.key == key_W || kev.key == key_S ) {
      miutil::miString name;
      int id;
      vector<miutil::miString> stations;
      int step = (kev.key == key_S) ? 1 : -1;
      if(kev.modifier==key_Shift) name = "add";
      contr->getEditStation(step,name,id,stations);
      if( name.exists() && stations.size()>0){
        miMessage letter;
        letter.to = qmstrings::all;
        //	cerr <<"To: "<<letter.to<<endl;
        letter.command    = qmstrings::selectposition;
        letter.commondesc =  "dataset";
        letter.common     =  name;
        letter.description =  "station";
        letter.data.push_back(stations[0]);
        sendLetter(letter);

      }
      w->updateGL();
    }

    else if( kev.key == key_N || kev.key == key_P ) {
      //      int id=vselectAreas[ia].id;
      miMessage letter;
      letter.command = qmstrings::selectarea;
      letter.description = "next";
      if( kev.key == key_P )
        letter.data.push_back("-1");
      else
        letter.data.push_back("+1");
      letter.to = qmstrings::all;;
      sendLetter(letter);
    }

    else if( kev.key == key_A || kev.key == key_D ) {
      miMessage letter;
      letter.command    = qmstrings::changetype;
      if( kev.key == key_A )
        letter.data.push_back("-1");
      else
        letter.data.push_back("+1");
      letter.to = qmstrings::all;;
      sendLetter(letter);
    }

    else if( kev.key == key_Escape) {
      miMessage letter;
      letter.command    = qmstrings::copyvalue;
      letter.commondesc =  "dataset";
      letter.description =  "name";
      letter.to = qmstrings::all;;
      sendLetter(letter);
    }

    //FIXME (?): Will resize stationplots when connected to a coserver, regardless of which other client(s) are connected.
    if (kev.modifier == key_Control && kev.key == key_Plus) {
      float current_scale = contr->getStationsScale();
      contr->setStationsScale(current_scale + 0.1); //FIXME: No hardcoding of increment.
      w->updateGL();
    }
    if (kev.modifier == key_Control && kev.key == key_Minus) {
      float current_scale = contr->getStationsScale();
      contr->setStationsScale(current_scale - 0.1); //FIXME: No hardcoding of decrement.
      w->updateGL();
    }

  }
}

void DianaMainWindow::undo()
{
  if(em->inedit()){
    em->undoEdit();
  }else {
    miMessage letter;
    letter.command    = qmstrings::sendkey;
    letter.commondesc =  "key";
    letter.common =  "ctrl_Z";
    letter.to = qmstrings::all;;
    sendLetter(letter);
  }
}

void DianaMainWindow::redo()
{
  if(em->inedit())
    em->redoEdit();
  else {
    miMessage letter;
    letter.command    = qmstrings::sendkey;
    letter.commondesc =  "key";
    letter.common =  "ctrl_Y";
    letter.to = qmstrings::all;;
    sendLetter(letter);
  }
}

void DianaMainWindow::save()
{
  if(em->inedit())
    em->saveEdit();
  else {
    miMessage letter;
    letter.command    = qmstrings::sendkey;
    letter.commondesc =  "key";
    letter.common =  "ctrl_S";
    letter.to = qmstrings::all;;
    sendLetter(letter);
  }
}

void DianaMainWindow::toggleToolBar()
{
  if ( mainToolbar->isVisible() ) {
    mainToolbar->hide();
  } else {
    mainToolbar->show();
  }
}

void DianaMainWindow::toggleStatusBar()
{
  if ( statusBar()->isVisible() ) {
    statusBar()->hide();
  } else {
    statusBar()->show();
  }
}

void DianaMainWindow::filequit()
{

  if (em->cleanupForExit() && uffm->okToExit()){
    writeLogFile();
    profetDisconnect();
    qApp->quit();
  }

}


void DianaMainWindow::timeoutChanged(float value)
{
  int msecvalue= static_cast<int>(value*1000);
  if (msecvalue != timeout_ms){
    timeout_ms= msecvalue;

    if (timeron!=0) {
      killTimer(animationTimer);
      animationTimer= startTimer(timeout_ms);
    }
  }
}


void DianaMainWindow::timecontrolslot()
{
  bool b= timecontrol->isVisible();
  if (b){
    timecontrol->hide();
  } else {
    timecontrol->show();
  }
  timeControlAction->setChecked(!b);
}




void DianaMainWindow::writeLogFile()
{
  // write the system log file to $HOME/.diana.log

  miLogFile milogfile; // static logger
  miutil::miString logfile= LocalSetupParser::basicValue("homedir") + "/diana.log";
  miutil::miString thisVersion= version_string;
  miutil::miString thisBuild= build_string;
  // open filestream
  ofstream file(logfile.c_str());
  if (!file){
    cerr << "ERROR OPEN (WRITE) " << logfile << endl;
    return;
  }

  vector<miutil::miString> vstr;
  int i,n;

  vstr= writeLog(thisVersion,thisBuild);
  n= vstr.size();
  file << "[MAIN.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/MAIN.LOG]" << endl;

  vstr= contr->writeLog();
  n= vstr.size();
  file << "[CONTROLLER.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/CONTROLLER.LOG]" << endl;
  file << endl;

  vstr= mm->writeLog();
  n= vstr.size();
  file << "[MAP.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/MAP.LOG]" << endl;
  file << endl;

  vstr= fm->writeLog();
  n= vstr.size();
  file << "[FIELD.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/FIELD.LOG]" << endl;
  file << endl;

  vstr= om->writeLog();
  n= vstr.size();
  file << "[OBS.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/OBS.LOG]" << endl;
  file << endl;

  vstr= sm->writeLog();
  n= vstr.size();
  file << "[SAT.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/SAT.LOG]" << endl;
  file << endl;

  vstr= qm->writeLog();
  n= vstr.size();
  file << "[QUICK.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/QUICK.LOG]" << endl;
  file << endl;

  vstr= trajm->writeLog();
  n= vstr.size();
  file << "[TRAJ.LOG]" << endl;
  for (i=0; i<n; i++) file << vstr[i] << endl;
  file << "[/TRAJ.LOG]" << endl;
  file << endl;

  if ( vpWindow ){
    vstr= vpWindow->writeLog("window");
    n= vstr.size();
    file << "[VPROF.WINDOW.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/VPROF.WINDOW.LOG]" << endl;
    file << endl;

    vstr= vpWindow->writeLog("setup");
    n= vstr.size();
    file << "[VPROF.SETUP.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/VPROF.SETUP.LOG]" << endl;
    file << endl;
  }

  if ( vcWindow ){
    vstr= vcWindow->writeLog("window");
    n= vstr.size();
    file << "[VCROSS.WINDOW.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/VCROSS.WINDOW.LOG]" << endl;
    file << endl;

    vstr= vcWindow->writeLog("setup");
    n= vstr.size();
    file << "[VCROSS.SETUP.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/VCROSS.SETUP.LOG]" << endl;
    file << endl;

    vstr= vcWindow->writeLog("field");
    n= vstr.size();
    file << "[VCROSS.FIELD.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/VCROSS.FIELD.LOG]" << endl;
    file << endl;
  }

  if ( spWindow ){
    vstr= spWindow->writeLog("window");
    n= vstr.size();
    file << "[SPECTRUM.WINDOW.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/SPECTRUM.WINDOW.LOG]" << endl;
    file << endl;

    vstr= spWindow->writeLog("setup");
    n= vstr.size();
    file << "[SPECTRUM.SETUP.LOG]" << endl;
    for (i=0; i<n; i++) file << vstr[i] << endl;
    file << "[/SPECTRUM.SETUP.LOG]" << endl;
    file << endl;
  }

  file << "[PROFET.LOG]" << endl
      << milogfile.writeString("PROFET.LOG") << endl
      << "[/PROFET.LOG]" << endl;


  file.close();
  cerr << "Finished writing " << logfile << endl;
}


void DianaMainWindow::readLogFile()
{
  // read the system log file

  getDisplaySize();

  miutil::miString logfile= LocalSetupParser::basicValue("homedir") + "/diana.log";
  miutil::miString thisVersion= version_string;
  miutil::miString logVersion;

  miLogFile milogfile; // static logger from puTools - keeps stuff in mind

  milogfile.setMaxXY(displayWidth,displayHeight);

  // open filestream
  ifstream file(logfile.c_str());
  if (!file){
    //    cerr << "Can't open " << logfile << endl;
    return;
  }

  cerr << "READ " << logfile << endl;

  miutil::miString beginStr, endStr, str;
  vector<miutil::miString> vstr;

  while (getline(file,beginStr)) {
    if (!beginStr.empty() && beginStr[0]!='#') {
      beginStr.trim();
      int l= beginStr.length();
      if (l<3 || beginStr[0]!='[' || beginStr[l-1]!=']') {
        cerr << "Bad keyword found in " << logfile << " : " << beginStr << endl;
        break;
      }

      endStr= beginStr.substr(0,1) + '/' + beginStr.substr(1,l-1);
      vstr.clear();

      while (getline(file,str)) {
        if (str==endStr) break;
        vstr.push_back(str);
      }

      if (beginStr=="[MAIN.LOG]")
        readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[CONTROLLER.LOG]")
        contr->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[MAP.LOG]")
        mm->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[FIELD.LOG]")
        fm->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[OBS.LOG]")
        om->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[SAT.LOG]")
        sm->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[TRAJ.LOG]")
        trajm->readLog(vstr,thisVersion,logVersion);
      else if (beginStr=="[QUICK.LOG]")
        qm->readLog(vstr,thisVersion,logVersion);
      else if (vpWindow && beginStr=="[VPROF.WINDOW.LOG]")
        vpWindow->readLog("window",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (vpWindow && beginStr=="[VPROF.SETUP.LOG]")
        vpWindow->readLog("setup",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (vcWindow && beginStr=="[VCROSS.WINDOW.LOG]")
        vcWindow->readLog("window",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (vcWindow && beginStr=="[VCROSS.SETUP.LOG]")
        vcWindow->readLog("setup",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (vcWindow && beginStr=="[VCROSS.FIELD.LOG]")
        vcWindow->readLog("field",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (spWindow && beginStr=="[SPECTRUM.WINDOW.LOG]")
        spWindow->readLog("window",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if (spWindow && beginStr=="[SPECTRUM.SETUP.LOG]")
        spWindow->readLog("setup",vstr,thisVersion,logVersion,
            displayWidth,displayHeight);
      else if ( beginStr=="[PROFET.LOG]") {
        milogfile.setSection("PROFET.LOG");
        milogfile.readStrings(vstr,beginStr);
      }

      //else
      //	cerr << "Unhandled log section: " << beginStr << endl;
    }
  }

  file.close();
  cerr << "Finished reading " << logfile << endl;
}


vector<miutil::miString> DianaMainWindow::writeLog(const miutil::miString& thisVersion,
    const miutil::miString& thisBuild)
{
  vector<miutil::miString> vstr;
  miutil::miString str;

  // version & time
  str= "VERSION " + thisVersion;
  vstr.push_back(str);
  str= "BUILD " + thisBuild;
  vstr.push_back(str);
  str= "LOGTIME " + miutil::miTime::nowTime().isoTime();
  vstr.push_back(str);
  vstr.push_back("================");

  // dialog positions
  str= "MainWindow.size " + miutil::miString(this->width()) + " " + miutil::miString(this->height());
  vstr.push_back(str);
  str= "MainWindow.pos "  + miutil::miString( this->x()) + " " + miutil::miString( this->y());
  vstr.push_back(str);
  str= "QuickMenu.pos "   + miutil::miString(qm->x()) + " " + miutil::miString(qm->y());
  vstr.push_back(str);
  str= "FieldDialog.pos " + miutil::miString(fm->x()) + " " + miutil::miString(fm->y());
  vstr.push_back(str);
  fm->show();
  fm->advancedToggled(false);
  str= "FieldDialog.size " + miutil::miString(fm->width()) + " " + miutil::miString(fm->height());
  vstr.push_back(str);
  str= "ObsDialog.pos "   + miutil::miString(om->x()) + " " + miutil::miString(om->y());
  vstr.push_back(str);
  str= "SatDialog.pos "   + miutil::miString(sm->x()) + " " + miutil::miString(sm->y());
  vstr.push_back(str);
  str= "MapDialog.pos "   + miutil::miString(mm->x()) + " " + miutil::miString(mm->y());
  vstr.push_back(str);
  str= "EditDialog.pos "  + miutil::miString(em->x()) + " " + miutil::miString(em->y());
  vstr.push_back(str);
  str= "ObjectDialog.pos " + miutil::miString(objm->x()) + " " + miutil::miString(objm->y());
  vstr.push_back(str);
  str= "TrajectoryDialog.pos " + miutil::miString(trajm->x()) + " " + miutil::miString(trajm->y());
  vstr.push_back(str);
  str= "Textview.size "   + miutil::miString(textview->width()) + " " + miutil::miString(textview->height());
  vstr.push_back(str);
  str= "Textview.pos "  + miutil::miString(textview->x()) + " " + miutil::miString(textview->y());
  vstr.push_back(str);
  str="DocState " + saveDocState();
  vstr.push_back(str);
  vstr.push_back("================");

  // printer name & options...
  if (priop.printer.exists()){
    str= "PRINTER " + priop.printer;
    vstr.push_back(str);
    if (priop.orientation==d_print::ori_portrait)
      str= "PRINTORIENTATION portrait";
    else
      str= "PRINTORIENTATION landscape";
    vstr.push_back(str);
  }
  //vstr.push_back("================");

  // Status-buttons
  str= "STATUSBUTTONS " + miutil::miString(showelem ? "ON" : "OFF");
  vstr.push_back(str);
  //vstr.push_back("================");

  // Automatic element selection
  autoselect= optAutoElementAction->isChecked();
  str= "AUTOSELECT " + miutil::miString(autoselect ? "ON" : "OFF");
  vstr.push_back(str);

  // scrollwheelzooming
  bool scrollwheelzoom = optScrollwheelZoomAction->isChecked();
  str = "SCROLLWHEELZOOM " + miutil::miString(scrollwheelzoom ? "ON" : "OFF");
  vstr.push_back(str);

  // GUI-font
  str= "FONT " + miutil::miString(qApp->font().toString().toStdString());
  vstr.push_back(str);
  //vstr.push_back("================");

  return vstr;
}

miutil::miString DianaMainWindow::saveDocState()
{
  QByteArray state = saveState();
  ostringstream ost;
  int n= state.count();
  for (int i=0; i<n; i++)
    ost << setw(7) << int(state[i]);
  return ost.str();
}

void DianaMainWindow::readLog(const vector<miutil::miString>& vstr,
    const miutil::miString& thisVersion,
    miutil::miString& logVersion)
{
  vector<miutil::miString> tokens;
  int x,y;

  int nvstr= vstr.size();
  int ivstr= 0;

  logVersion.clear();

  //.....version & time .........
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    tokens= vstr[ivstr].split(' ');
    if (tokens[0]=="VERSION" && tokens.size()==2) logVersion= tokens[1];
    //if (tokens[0]=="LOGTIME" && tokens.size()==3) .................
  }
  ivstr++;

  // dialog positions
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    tokens= vstr[ivstr].split(' ');
    if (tokens[0]=="DocState") restoreDocState(vstr[ivstr]);
    if (paintToolBar != 0) {
      paintToolBar->hide();
    }

    if (tokens.size()==3) {
      x= atoi(tokens[1].c_str());
      y= atoi(tokens[2].c_str());
      if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
        if (tokens[0]=="MainWindow.size")  this->resize(x,y);
      }
      if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
        if      (tokens[0]=="MainWindow.pos")  this->move(x,y);
        else if (tokens[0]=="QuickMenu.pos")   qm->move(x,y);
        else if (tokens[0]=="FieldDialog.pos") fm->move(x,y);
        else if (tokens[0]=="FieldDialog.size")fm->resize(x,y);
        else if (tokens[0]=="ObsDialog.pos")   om->move(x,y);
        else if (tokens[0]=="SatDialog.pos")   sm->move(x,y);
        else if (tokens[0]=="MapDialog.pos")   mm->move(x,y);
        else if (tokens[0]=="EditDialog.pos")  em->move(x,y);
        else if (tokens[0]=="ObjectDialog.pos") objm->move(x,y);
        else if (tokens[0]=="TrajectoryDialog.pos") trajm->move(x,y);
        else if (tokens[0]=="Textview.size")   textview->resize(x,y);
        else if (tokens[0]=="Textview.pos")    textview->move(x,y);
      }
    }
  }
  ivstr++;

  // printer name & other options...
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") continue;
    tokens= vstr[ivstr].split(' ');
    if (tokens.size()>=2) {
      if (tokens[0]=="PRINTER") {
        priop.printer=tokens[1];
      } else if (tokens[0]=="PRINTORIENTATION") {
        if (tokens[1]=="portrait")
          priop.orientation=d_print::ori_portrait;
        else
          priop.orientation=d_print::ori_landscape;
      } else if (tokens[0]=="STATUSBUTTONS") {
        showelem= (tokens[1]=="ON");
      } else if (tokens[0]=="AUTOSELECT") {
        autoselect = (tokens[1] == "ON");
      } else if (tokens[0] == "SCROLLWHEELZOOM") {
        if (tokens[1] == "ON") {
          optScrollwheelZoomAction->setChecked(true);
          toggleScrollwheelZoom();
        }
      } else if (tokens[0] == "FONT") {
        miutil::miString fontstr = tokens[1];
        //LB:if the font name contains blanks,
        //the string will be cut in pieces, and must be put together again.
        for(unsigned int i=2;i<tokens.size();i++)
          fontstr += " " + tokens[i];
        QFont font;
        if (font.fromString(fontstr.cStr()))
          qApp->setFont(font);
      }
    }
  }

  if (logVersion.empty()) logVersion= "0.0.0";

  if (logVersion!=thisVersion)
    cerr << "log from version " << logVersion << endl;
}

void DianaMainWindow::restoreDocState(miutil::miString logstr)
{
  vector<miutil::miString> vs= logstr.split(" ");
  int n=vs.size();
  QByteArray state(n-1,' ');
  for (int i=1; i<n; i++){
    state[i-1]= char(atoi(vs[i].c_str()));
  }

  if (!restoreState( state)){
    cerr << "!!!restoreState failed" << endl;
  }
}

void DianaMainWindow::getDisplaySize()
{
  displayWidth=  QApplication::desktop()->width();
  displayHeight= QApplication::desktop()->height();

  cerr << "display width,height: "<<displayWidth<<" "<<displayHeight<<endl;
}


void DianaMainWindow::checkNews()
{
  miutil::miString newsfile= LocalSetupParser::basicValue("homedir") + "/diana.news";
  miutil::miString thisVersion= "yy";
  miutil::miString newsVersion= "xx";

  // check modification time on news file
  miutil::miString filename= LocalSetupParser::basicValue("docpath") + "/" + "news.html";
  QFileInfo finfo( filename.c_str() );
  if (finfo.exists()) {
    QDateTime dt = finfo.lastModified();
    thisVersion = dt.toString("yyyy-MM-dd hh:mm:ss").toStdString();
  }

  // open input filestream
  ifstream ifile(newsfile.c_str());
  if (ifile) {
    getline(ifile,newsVersion);
    ifile.close();
  }

  if (thisVersion!=newsVersion) {
    // open output filestream
    ofstream ofile(newsfile.c_str());
    if (ofile) {
      ofile << thisVersion << endl;
      ofile.close();
    }

    showNews();
  }
}


void DianaMainWindow::satFileListUpdate()
{
  //refresh list of files in satDialog and timeslider
  //called when a request for a satellite file from satManager
  //discovers new or missing files
  if (contr->satFileListChanged()){
    sm->RefreshList();
    contr->satFileListUpdated();
  }
}

void DianaMainWindow::toggleElement(PlotElement pe)
{
  contr->enablePlotElement(pe);
  //update sat channels in statusbar
  vector<miutil::miString> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);
  w->updateGL();
}


void DianaMainWindow::showElements()
{
  if ( showelem ) {
    statusbuttons->reset();
    statusbuttons->hide();
    optOnOffAction->setChecked( false );
    showelem= false;
  } else {
    statusbuttons->setPlotElements(contr->getPlotElements());
    statusbuttons->show();
    optOnOffAction->setChecked( true );
    showelem= true;
  }
}

void DianaMainWindow::archiveMode()
{
  bool on= optArchiveAction->isChecked();

  contr->archiveMode(on);
  fm->archiveMode(on);
  om->archiveMode(on);
  sm->archiveMode();
  objm->archiveMode(on);
  if(on){
    archiveL->show();
  }else{
    archiveL->hide();
  }
}

void DianaMainWindow::autoElement()
{
  //   bool on = optAutoElementAction->isChecked();
  //   optAutoElementAction->setChecked( on );
}


void DianaMainWindow::showAnnotations()
{
  bool on = optAnnotationAction->isChecked();
  //   optAnnotationAction->setChecked( on );
  contr->showAnnotations(on);
}

void DianaMainWindow::toggleScrollwheelZoom()
{
  bool on = optScrollwheelZoomAction->isChecked();
  contr->toggleScrollwheelZoom(on);
}

void DianaMainWindow::chooseFont()
{
  bool ok;
  QFont font = QFontDialog::getFont( &ok,qApp->font(),this );
  if ( ok ) {
    // font is set to the font the user selected
    qApp->setFont(font);
  } else {
    // the user cancelled the dialog; font is set to the initial
    // value: do nothing
  }
}

void DianaMainWindow::zoomTo(Rectangle r) {
  if(contr)
    contr->zoomTo(r);
}

void DianaMainWindow::zoomOut()
{
  contr->zoomOut();
  w->updateGL();
}

void DianaMainWindow::showUffda()
{
  if (uffda && contr->getSatnames().size()){

    if (xclick==xlast && yclick==ylast){
      uffMenu();
      xlast=xclick;
      ylast=yclick;
      return;
    }
    float lat=0,lon=0;
    contr->PhysToGeo(xclick,yclick,lat,lon);
    uffm->addPosition(lat,lon);
    if (!uffm->isVisible())
      uffMenu();
    xlast=xclick;
    ylast=yclick;
    w->updateGL();
  }
}

//this is called after an area selected from rightclickmenu
void DianaMainWindow::selectedAreas()
{

  QAction *action = qobject_cast<QAction *>(sender());
  if (!action){
    return;
  }

  int ia = action->data().toInt();

  miutil::miString areaName=vselectAreas[ia].name;
  bool selected=vselectAreas[ia].selected;
  int id=vselectAreas[ia].id;
  miutil::miString misc=(selected) ? "off" : "on";
  miutil::miString datastr = areaName + ":" + misc;
  miMessage letter;
  letter.command = qmstrings::selectarea;
  letter.description = "name:on/off";
  letter.data.push_back(datastr);
  letter.to = id;
  sendLetter(letter);
}


void DianaMainWindow::inEdit(bool inedit)
{
  if(qsocket){
    miutil::miString str;
    if(inedit)
      str = "on";
    else
      str = "off";
    miMessage letter;
    letter.command    = qmstrings::editmode;
    letter.commondesc = "on/off";
    letter.common     = str;
    letter.to         = qmstrings::all;
    sendLetter(letter);
  }
}


void DianaMainWindow::closeEvent(QCloseEvent * e)
{
  filequit();
}

bool DianaMainWindow::event(QEvent* event)
{
  if (event->type() == QEvent::WhatsThisClicked) {
    QWhatsThisClickedEvent* wtcEvent = static_cast<QWhatsThisClickedEvent*>(event);
    QDesktopServices::openUrl(wtcEvent->href());
    QWhatsThis::hideText();
    return true;
  }

  return QMainWindow::event(event);
}
