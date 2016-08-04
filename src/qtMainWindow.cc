/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtMainWindow.h"

#include "qtTimeSlider.h"
#include "qtTimeControl.h"
#include "qtTimeStepSpinbox.h"
#include "qtStatusGeopos.h"
#include "qtStatusPlotButtons.h"
#include "qtShowSatValues.h"
#include "qtTextDialog.h"
#include "qtImageGallery.h"
#include "qtUtility.h"
#include "qtWorkArea.h"
#include "qtVprofWindow.h"
#include "qtSpectrumWindow.h"
#include "qtDataDialog.h"
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
#include "qtSetupDialog.h"
#include "qtPrintManager.h"
#include "qtBrowserBox.h"
#include "qtAddtoMenu.h"
#include "qtUffdaDialog.h"
#include "qtAnnotationDialog.h"
#include "qtTextView.h"
#include "qtMailDialog.h"

#include "diBuild.h"
#include "diController.h"
#include "diEditItemManager.h"
#include "diPaintGLPainter.h"
#include "diPrintOptions.h"
#include "diLocalSetupParser.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "diLocationData.h"
#include "diLogFile.h"
#include "miSetupParser.h"

#include "vcross_qt/qtVcrossInterface.h"
#include "wmsclient/WebMapDialog.h"
#include "wmsclient/WebMapManager.h"

#include <qUtilities/qtHelpDialog.h>

#include <coserver/ClientSelection.h>
#include <coserver/miMessage.h>
#include <coserver/QLetterCommands.h>

#include <puDatatypes/miCoordinates.h>

#include <puTools/miStringFunctions.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFocusEvent>
#include <QFontDialog>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QSvgGenerator>
#include <QTimerEvent>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QWhatsThis>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sys/types.h>
#include <sys/time.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#include <diField/diFieldManager.h>

#include "EditItems/drawingdialog.h"
#include "EditItems/kml.h"
#include "EditItems/toolbar.h"

#define MILOGGER_CATEGORY "diana.MainWindow"
#include <qUtilities/miLoggingQt.h>

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
#include <autoupdate.xpm>
#include <drawing.xpm>
#include <editdrawing.xpm>

//#define DISABLE_VPROF 1
//#define DISABLE_VCROSS 1
//#define DISABLE_WAVESPEC 1

using namespace std;

DianaMainWindow *DianaMainWindow::self = 0;

DianaMainWindow::DianaMainWindow(Controller *co, const QString& instancename)
  : QMainWindow(),
    push_command(true),browsing(false),
    markTrajPos(false), markMeasurementsPos(false), vpWindow(0)
  , vcInterface(0)
  , vcrossEditManagerConnected(false)
  , spWindow(0), pluginB(0), contr(co)
  , timeron(0),timeout_ms(100),timeloop(false),showelem(true), autoselect(false)
{
  METLIBS_LOG_SCOPE();

  setWindowIcon(QIcon(QPixmap(diana_icon_xpm)));

  self = this;

  //-------- The Actions ---------------------------------

  // file ========================
  // --------------------------------------------------------------------
  fileSavePictAction = new QAction( tr("&Save picture..."),this );
  connect( fileSavePictAction, SIGNAL( triggered() ) , SLOT( saveraster() ) );
  // --------------------------------------------------------------------
  emailPictureAction = new QAction( tr("&Email picture..."),this );
  connect( emailPictureAction, SIGNAL( triggered() ) , SLOT( emailPicture() ) );
  // --------------------------------------------------------------------
  saveAnimationAction = new QAction( tr("Save &animation..."),this );
  connect( saveAnimationAction, SIGNAL( triggered() ) , SLOT( saveAnimation() ) );
  // --------------------------------------------------------------------
  filePrintAction = new QAction( tr("&Print..."),this );
  filePrintAction->setShortcut(Qt::CTRL+Qt::Key_P);
  connect( filePrintAction, SIGNAL( triggered() ) , SLOT( hardcopy() ) );
  // --------------------------------------------------------------------
  filePrintPreviewAction = new QAction( tr("Print pre&view..."),this );
  connect( filePrintPreviewAction, SIGNAL( triggered() ) , SLOT( previewHardcopy() ) );
  // --------------------------------------------------------------------
  readSetupAction = new QAction( tr("Read setupfile"),this );
  connect( readSetupAction, SIGNAL( triggered() ) , SLOT( parseSetup() ) );
  // --------------------------------------------------------------------
  fileQuitAction = new QAction( tr("&Quit..."), this );
  fileQuitAction->setShortcut(Qt::CTRL+Qt::Key_Q);
  fileQuitAction->setShortcutContext(Qt::ApplicationShortcut);
  connect( fileQuitAction, SIGNAL( triggered() ) , SLOT( filequit() ) );


  // options ======================
  // --------------------------------------------------------------------
  optOnOffAction = new QAction( tr("S&peed buttons"), this );
  optOnOffAction->setCheckable(true);
  connect( optOnOffAction, SIGNAL( triggered() ) ,  SLOT( showElements() ) );
  // --------------------------------------------------------------------
  optArchiveAction = new QAction( tr("A&rchive mode"), this );
  optArchiveAction->setCheckable(true);
  connect( optArchiveAction, SIGNAL( triggered() ) ,  SLOT( archiveMode() ) );
  // --------------------------------------------------------------------
  optAutoElementAction = new QAction( tr("&Automatic element choice"), this );
  optAutoElementAction->setShortcut(Qt::Key_Space);
  optAutoElementAction->setCheckable(true);
  connect( optAutoElementAction, SIGNAL( triggered() ), SLOT( autoElement() ) );
  // --------------------------------------------------------------------
  optAnnotationAction = new QAction( tr("A&nnotations"), this );
  optAnnotationAction->setCheckable(true);
  connect( optAnnotationAction, SIGNAL( triggered() ), SLOT( showAnnotations() ) );
  // --------------------------------------------------------------------
  optScrollwheelZoomAction = new QAction( tr("Scrollw&heel zooming"), this );
  optScrollwheelZoomAction->setCheckable(true);
  connect( optScrollwheelZoomAction, SIGNAL( triggered() ), SLOT( toggleScrollwheelZoom() ) );
  // --------------------------------------------------------------------
  optFontAction = new QAction( tr("Select &Font..."), this );
  connect( optFontAction, SIGNAL( triggered() ) ,  SLOT( chooseFont() ) );


  // show ======================
  // --------------------------------------------------------------------
  showResetAreaAction = new QAction(QIcon( QPixmap(thumbs_up_xpm)),tr("Reset area and replot"), this );
  showResetAreaAction->setIconVisibleInMenu(true);
  connect( showResetAreaAction, SIGNAL( triggered() ) ,  SLOT( resetArea() ) );
  //----------------------------------------------------------------------
  showResetAllAction = new QAction( QIcon(QPixmap(thumbs_down_xpm)),tr("Reset all"), this );
  showResetAllAction->setIconVisibleInMenu(true);
  connect( showResetAllAction, SIGNAL( triggered() ) ,  SLOT( resetAll() ) );
  // --------------------------------------------------------------------
  showApplyAction = new QAction( tr("&Apply plot"), this );
  showApplyAction->setShortcut(Qt::CTRL+Qt::Key_U);
  connect( showApplyAction, SIGNAL( triggered() ) ,  SLOT( MenuOK() ) );
  // --------------------------------------------------------------------
  showAddQuickAction = new QAction( tr("Add to q&uickmenu"), this );
  showAddQuickAction->setShortcutContext(Qt::ApplicationShortcut);
  showAddQuickAction->setShortcut(Qt::Key_F9);
  connect( showAddQuickAction, SIGNAL( triggered() ) ,  SLOT( addToMenu() ) );
  // --------------------------------------------------------------------
  showPrevPlotAction = new QAction( tr("P&revious plot"), this );
  showPrevPlotAction->setShortcut(Qt::Key_F10);
  connect( showPrevPlotAction, SIGNAL( triggered() ) ,  SLOT( prevHPlot() ) );
  // --------------------------------------------------------------------
  showNextPlotAction = new QAction( tr("&Next plot"), this );
  showNextPlotAction->setShortcut(Qt::Key_F11);
  connect( showNextPlotAction, SIGNAL( triggered() ) ,  SLOT( nextHPlot() ) );
  // --------------------------------------------------------------------
  showHideAllAction = new QAction( tr("&Hide All"), this );
  showHideAllAction->setShortcut(Qt::CTRL+Qt::Key_D);
  showHideAllAction->setCheckable(true);
  connect( showHideAllAction, SIGNAL( triggered() ) ,  SLOT( toggleDialogs() ) );

  // --------------------------------------------------------------------
  showQuickmenuAction = new QAction( QIcon(QPixmap(pick_xpm )),tr("&Quickmenu"), this );
  showQuickmenuAction->setShortcutContext(Qt::ApplicationShortcut);
  showQuickmenuAction->setShortcut(Qt::Key_F12);
  showQuickmenuAction->setCheckable(true);
  showQuickmenuAction->setIconVisibleInMenu(true);
  connect( showQuickmenuAction, SIGNAL( triggered() ) ,  SLOT( quickMenu() ) );
  // --------------------------------------------------------------------
  showMapDialogAction = new QAction( QPixmap(earth3_xpm ),tr("Maps"), this );
  showMapDialogAction->setShortcut(Qt::ALT+Qt::Key_K);
  showMapDialogAction->setCheckable(true);
  showMapDialogAction->setIconVisibleInMenu(true);
  connect( showMapDialogAction, SIGNAL( triggered() ) ,  SLOT( mapMenu() ) );
  // --------------------------------------------------------------------
  showFieldDialogAction = new QAction( QIcon(QPixmap(felt_xpm )),tr("&Fields"), this );
  showFieldDialogAction->setShortcut(Qt::ALT+Qt::Key_F);
  showFieldDialogAction->setCheckable(true);
  showFieldDialogAction->setIconVisibleInMenu(true);
  connect( showFieldDialogAction, SIGNAL( triggered() ) ,  SLOT( fieldMenu() ) );
  // --------------------------------------------------------------------
  showObsDialogAction = new QAction( QIcon(QPixmap(synop_xpm )),tr("&Observations"), this );
  showObsDialogAction->setShortcut(Qt::ALT+Qt::Key_O);
  showObsDialogAction->setCheckable(true);
  showObsDialogAction->setIconVisibleInMenu(true);
  connect( showObsDialogAction, SIGNAL( triggered() ) ,  SLOT( obsMenu() ) );
  // --------------------------------------------------------------------
  showSatDialogAction = new QAction( QIcon(QPixmap(sat_xpm )),tr("&Satellites and Radar"), this );
  showSatDialogAction->setShortcut(Qt::ALT+Qt::Key_S);
  showSatDialogAction->setCheckable(true);
  showSatDialogAction->setIconVisibleInMenu(true);
  connect( showSatDialogAction, SIGNAL( triggered() ) ,  SLOT( satMenu() ) );
  // --------------------------------------------------------------------
  showStationDialogAction = new QAction( QIcon(QPixmap(station_xpm )),tr("Toggle Stations"), this );
  showStationDialogAction->setShortcut(Qt::ALT+Qt::Key_A);
  showStationDialogAction->setCheckable(true);
  showStationDialogAction->setIconVisibleInMenu(true);
  connect( showStationDialogAction, SIGNAL( triggered() ) ,  SLOT( stationMenu() ) );
  // --------------------------------------------------------------------
  showEditDialogAction = new QAction( QPixmap(editmode_xpm ),tr("&Product Editing"), this );
  showEditDialogAction->setShortcut(Qt::ALT+Qt::Key_E);
  showEditDialogAction->setCheckable(true);
  showEditDialogAction->setIconVisibleInMenu(true);
  connect( showEditDialogAction, SIGNAL( triggered() ) ,  SLOT( editMenu() ) );
  // --------------------------------------------------------------------
  showObjectDialogAction = new QAction( QIcon(QPixmap(front_xpm )),tr("O&bjects"), this );
  showObjectDialogAction->setShortcut(Qt::ALT+Qt::Key_J);
  showObjectDialogAction->setCheckable(true);
  showObjectDialogAction->setIconVisibleInMenu(true);
  connect( showObjectDialogAction, SIGNAL( triggered() ) ,  SLOT( objMenu() ) );
  // --------------------------------------------------------------------
  showTrajecDialogAction = new QAction( QIcon(QPixmap( traj_xpm)),tr("&Trajectories"), this );
  showTrajecDialogAction->setShortcut(Qt::ALT+Qt::Key_T);
  showTrajecDialogAction->setCheckable(true);
  showTrajecDialogAction->setIconVisibleInMenu(true);
  connect( showTrajecDialogAction, SIGNAL( triggered() ) ,  SLOT( trajMenu() ) );
  // --------------------------------------------------------------------
  showAnnotationDialogAction = new QAction( tr("Annotation"), this );
  showAnnotationDialogAction->setShortcut(Qt::ALT+Qt::Key_L);
  showAnnotationDialogAction->setCheckable(true);
  showAnnotationDialogAction->setIconVisibleInMenu(true);
  connect( showAnnotationDialogAction, SIGNAL( triggered() ) ,  SLOT( AnnotationMenu() ) );
  // --------------------------------------------------------------------
  showProfilesDialogAction = new QAction( QIcon(QPixmap(balloon_xpm )),tr("&Vertical Profiles"), this );
  showProfilesDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_VPROF
  showProfilesDialogAction->setShortcut(Qt::ALT+Qt::Key_V);
  showProfilesDialogAction->setCheckable(false);
  connect( showProfilesDialogAction, SIGNAL( triggered() ) ,  SLOT( vprofMenu() ) );
#else
  showProfilesDialogAction->setEnabled(false);
#endif
  // --------------------------------------------------------------------
  showCrossSectionDialogAction = new QAction(QIcon( QPixmap(vcross_xpm )),tr("Vertical &Cross sections"), this );
  showCrossSectionDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_VCROSS
  showCrossSectionDialogAction->setShortcut(Qt::ALT+Qt::Key_C);
  showCrossSectionDialogAction->setCheckable(false);
  connect( showCrossSectionDialogAction, SIGNAL( triggered() ) ,  SLOT( vcrossMenu() ) );
#else
  showCrossSectionDialogAction->setEnabled(false);
#endif
  // --------------------------------------------------------------------
  showWaveSpectrumDialogAction = new QAction(QIcon( QPixmap(spectrum_xpm )),tr("&Wave spectra"), this );
  showWaveSpectrumDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_WAVESPEC
  showWaveSpectrumDialogAction->setShortcut(Qt::ALT+Qt::Key_W);
  showWaveSpectrumDialogAction->setCheckable(false);
  connect( showWaveSpectrumDialogAction, SIGNAL( triggered() ) ,  SLOT( spectrumMenu() ) );
#else
  showWaveSpectrumDialogAction->setEnabled(false);
#endif

  // --------------------------------------------------------------------
  zoomOutAction = new QAction( tr("Zoom out"), this );
  zoomOutAction->setVisible(false);
  connect( zoomOutAction, SIGNAL( triggered() ), SLOT( zoomOut() ) );
  // --------------------------------------------------------------------
  showUffdaDialogAction = new QAction( tr("&Uffda Service"), this );
  showUffdaDialogAction->setShortcut(Qt::ALT+Qt::Key_U);
  showUffdaDialogAction->setCheckable(true);
  connect( showUffdaDialogAction, SIGNAL( triggered() ), SLOT( uffMenu() ) );
  // --------------------------------------------------------------------
  showMeasurementsDialogAction = new QAction( QPixmap( ruler),tr("&Measurements"), this );
  showMeasurementsDialogAction->setShortcut(Qt::ALT+Qt::Key_M);
  showMeasurementsDialogAction->setCheckable(true);
  connect( showMeasurementsDialogAction, SIGNAL( triggered() ) ,  SLOT( measurementsMenu() ) );
  // ----------------------------------------------------------------
  //uffdaAction = new QShortcut(Qt::CTRL+Qt::Key_X,this );
  //connect( uffdaAction, SIGNAL( activated() ), SLOT( showUffda() ) );
  // ----------------------------------------------------------------

  toggleEditDrawingModeAction = new QAction( tr("Edit Drawing Mode"), this );
  toggleEditDrawingModeAction->setShortcut(Qt::CTRL+Qt::Key_B);
  connect(toggleEditDrawingModeAction, SIGNAL(triggered()), SLOT(toggleEditDrawingMode()));

  // help ======================
  // --------------------------------------------------------------------
  helpDocAction = new QAction( tr("Documentation"), this );
  helpDocAction->setShortcutContext(Qt::ApplicationShortcut);
  helpDocAction->setShortcut(Qt::Key_F1);
  helpDocAction->setCheckable(false);
  connect( helpDocAction, SIGNAL( triggered() ) ,  SLOT( showHelp() ) );
  // --------------------------------------------------------------------
  helpAccelAction = new QAction( tr("&Accelerators"), this );
  helpAccelAction->setCheckable(false);
  connect( helpAccelAction, SIGNAL( triggered() ) ,  SLOT( showAccels() ) );
  // --------------------------------------------------------------------
  helpNewsAction = new QAction( tr("&News"), this );
  helpNewsAction->setCheckable(false);
  connect( helpNewsAction, SIGNAL( triggered() ) ,  SLOT( showNews() ) );
  // --------------------------------------------------------------------
  helpTestAction = new QAction( tr("Test &results"), this );
  helpTestAction->setCheckable(false);
  connect( helpTestAction, SIGNAL( triggered() ) ,  SLOT( showUrl() ) );
  // --------------------------------------------------------------------
  helpAboutAction = new QAction( tr("About Diana"), this );
  helpAboutAction->setCheckable(false);
  connect( helpAboutAction, SIGNAL( triggered() ) ,  SLOT( about() ) );
  // --------------------------------------------------------------------


  // timecommands ======================
  // --------------------------------------------------------------------
  timeBackwardAction = new QAction(QIcon( QPixmap(start_xpm )),tr("Run Backwards"), this);
  timeBackwardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeBackwardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Left);
  timeBackwardAction->setCheckable(true);
  timeBackwardAction->setIconVisibleInMenu(true);
  connect( timeBackwardAction, SIGNAL( triggered() ) ,  SLOT( animationBack() ) );
  // --------------------------------------------------------------------
  timeForewardAction = new QAction(QIcon( QPixmap(slutt_xpm )),tr("Run Forewards"), this );
  timeForewardAction->setShortcutContext(Qt::ApplicationShortcut);
  timeForewardAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Right);
  timeForewardAction->setCheckable(true);
  timeForewardAction->setIconVisibleInMenu(true);
  connect( timeForewardAction, SIGNAL( triggered() ) ,  SLOT( animation() ) );
  // --------------------------------------------------------------------
  timeStepBackwardAction = new QAction(QIcon( QPixmap(bakover_xpm )),tr("Step Backwards"), this );
  timeStepBackwardAction->setShortcut(Qt::CTRL+Qt::Key_Left);
  timeStepBackwardAction->setCheckable(false);
  timeStepBackwardAction->setIconVisibleInMenu(true);
  connect( timeStepBackwardAction, SIGNAL( triggered() ) ,  SLOT( stepback() ) );
  // --------------------------------------------------------------------
  timeStepForewardAction = new QAction(QIcon( QPixmap(forward_xpm )),tr("Step Forewards"), this );
  timeStepForewardAction->setShortcut(Qt::CTRL+Qt::Key_Right);
  timeStepForewardAction->setCheckable(false);
  timeStepForewardAction->setIconVisibleInMenu(true);
  connect( timeStepForewardAction, SIGNAL( triggered() ) ,  SLOT( stepforward() ) );
  // --------------------------------------------------------------------
  timeStopAction = new QAction(QIcon( QPixmap(stop_xpm )),tr("Stop"), this );
  timeStopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeStopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Down);
  timeStopAction->setCheckable(false);
  timeStopAction->setIconVisibleInMenu(true);
  connect( timeStopAction, SIGNAL( triggered() ) ,  SLOT( animationStop() ) );
  // --------------------------------------------------------------------
  timeLoopAction = new QAction(QIcon( QPixmap(loop_xpm )),tr("Run in loop"), this );
  timeLoopAction->setShortcutContext(Qt::ApplicationShortcut);
  timeLoopAction->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Up);
  timeLoopAction->setCheckable(true);
  timeLoopAction->setIconVisibleInMenu(true);
  connect( timeLoopAction, SIGNAL( triggered() ) ,  SLOT( animationLoop() ) );
  // --------------------------------------------------------------------
  timeControlAction = new QAction(QIcon( QPixmap( clock_xpm )),tr("Time control"), this );
  timeControlAction->setCheckable(true);
  timeControlAction->setIconVisibleInMenu(true);
  connect( timeControlAction, SIGNAL( triggered() ) ,  SLOT( timecontrolslot() ) );
  // --------------------------------------------------------------------


  // other tools ======================
  // --------------------------------------------------------------------
  toolLevelUpAction = new QAction(QIcon( QPixmap(levelUp_xpm )),tr("Level up"), this );
  toolLevelUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelUpAction->setShortcut(Qt::CTRL+Qt::Key_PageUp);
  toolLevelUpAction->setCheckable(false);
  toolLevelUpAction->setEnabled ( false );
  toolLevelUpAction->setIconVisibleInMenu(true);
  connect( toolLevelUpAction, SIGNAL( triggered() ) ,  SLOT( levelUp() ) );
  // --------------------------------------------------------------------
  toolLevelDownAction = new QAction(QIcon( QPixmap(levelDown_xpm )),tr("Level down"), this );
  toolLevelDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelDownAction->setShortcut(Qt::CTRL+Qt::Key_PageDown);
  toolLevelDownAction->setCheckable(false);
  toolLevelDownAction->setEnabled ( false );
  toolLevelDownAction->setIconVisibleInMenu(true);
  connect( toolLevelDownAction, SIGNAL( triggered() ) ,  SLOT( levelDown() ) );
  // --------------------------------------------------------------------
  toolIdnumUpAction = new QAction(QIcon( QPixmap(idnumUp_xpm )),
      tr("EPS cluster/member etc up"), this );
  toolIdnumUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumUpAction->setShortcut(Qt::SHIFT+Qt::Key_PageUp);
  toolIdnumUpAction->setCheckable(false);
  toolIdnumUpAction->setEnabled ( false );
  toolIdnumUpAction->setIconVisibleInMenu(true);
  connect( toolIdnumUpAction, SIGNAL( triggered() ) ,  SLOT( idnumUp() ) );
  // --------------------------------------------------------------------
  toolIdnumDownAction = new QAction(QIcon( QPixmap(idnumDown_xpm )),
      tr("EPS cluster/member etc down"), this );
  toolIdnumDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumDownAction->setShortcut(Qt::SHIFT+Qt::Key_PageDown);
  toolIdnumDownAction->setEnabled ( false );
  toolIdnumDownAction->setIconVisibleInMenu(true);
  connect( toolIdnumDownAction, SIGNAL( triggered() ) ,  SLOT( idnumDown() ) );

  // Status ===============================
  // --------------------------------------------------------------------
  obsUpdateAction = new QAction(QIcon( QPixmap(synop_red_xpm)),
      tr("Update observations"), this );
  obsUpdateAction->setIconVisibleInMenu(true);
  connect( obsUpdateAction, SIGNAL( triggered() ), SLOT(updateObs()));

  // Autoupdate ===============================
  // --------------------------------------------------------------------
  autoUpdateAction = new QAction(QIcon( QPixmap(autoupdate_xpm)),
      tr("Automatic updates"), this );
  autoUpdateAction->setCheckable(true);
  autoUpdateAction->setIconVisibleInMenu(true);
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
  filemenu->addAction( filePrintPreviewAction );
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
  infoFiles = contr->getInfoFiles();
  if (not infoFiles.empty()) {
    std::map<std::string,InfoFile>::iterator p = infoFiles.begin();
    for (; p!=infoFiles.end(); p++)
      infomenu->addAction(QString::fromStdString(p->first));
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
  showmenu->addAction( showAnnotationDialogAction       );
  showmenu->addAction( showMeasurementsDialogAction    );
  showmenu->addAction( showProfilesDialogAction     );
  showmenu->addAction( showCrossSectionDialogAction );
  showmenu->addAction( showWaveSpectrumDialogAction );

  if (uffda){
    showmenu->addAction( showUffdaDialogAction );
  }
  showmenu->addMenu( infomenu );

  showmenu->addAction(toggleEditDrawingModeAction);

  //   //-------Help menu
  helpmenu = menuBar()->addMenu(tr("&Help"));
  helpmenu->addAction ( helpDocAction );
  helpmenu->addSeparator();
  helpmenu->addAction ( helpAccelAction );
  helpmenu->addAction ( helpNewsAction );
  helpmenu->addAction ( helpTestAction );
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
  connect(tslider, SIGNAL(newTimes(std::vector<miutil::miTime>&)),
      timecontrol, SLOT(setTimes(std::vector<miutil::miTime>&)));
  connect(timecontrol, SIGNAL(data(std::string)),
      tslider, SLOT(useData(std::string)));
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
#ifdef SMHI
  levelToolbar->addAction( autoUpdateAction );
#endif
  /**************** Toolbar Buttons *********************************************/

  //  mainToolbar = new QToolBar(this);
  //  addToolBar(Qt::RightToolBarArea,mainToolbar);

  mainToolbar->addAction( showResetAreaAction         );


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
  pluginB = new ClientSelection("Diana", this);
  pluginB->client()->setServerCommand(QString::fromStdString(LocalSetupParser::basicValue("qserver")));
  connect(pluginB, SIGNAL(receivedMessage(const miMessage&)),
      SLOT(processLetter(const miMessage&)));
  connect(pluginB, SIGNAL(disconnected()),
      SLOT(connectionClosed()));
  connect(pluginB, SIGNAL(renamed(const QString&)),
      SLOT(setInstanceName(const QString&)));
  setInstanceName(instancename);

  QToolButton* clientbutton = new QToolButton(statusBar());
  clientbutton->setDefaultAction(pluginB->getToolButtonAction());
  statusBar()->addPermanentWidget(clientbutton);
  optmenu->addSeparator();
  optmenu->addAction(pluginB->getMenuBarAction());

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
  QImage dr_img(drawing_xpm);
  ig.addImageToGallery(DrawingManager::instance()->plotElementTag().toStdString(), dr_img);
  QImage edr_img(editdrawing_xpm);
  ig.addImageToGallery(EditItemManager::instance()->plotElementTag().toStdString(), edr_img);

  // Read the avatars to gallery

  std::string avatarpath = LocalSetupParser::basicValue("avatars");
  if (not avatarpath.empty()) {
    const vector<std::string> vs = miutil::split(avatarpath, ":");
    for ( unsigned int i=0; i<vs.size(); i++ ){
      ig.addImagesInDirectory(vs[i]);
    }
  }

  //-------------------------------------------------

  w= new WorkArea(contr,this);
  setCentralWidget(w);

  connect(w->Glw(), SIGNAL(mouseGridPos(QMouseEvent*)),
      SLOT(catchMouseGridPos(QMouseEvent*)));

  connect(w->Glw(), SIGNAL(mouseRightPos(QMouseEvent*)),
      SLOT(catchMouseRightPos(QMouseEvent*)));

  connect(w->Glw(), SIGNAL(mouseMovePos(QMouseEvent*,bool)),
      SLOT(catchMouseMovePos(QMouseEvent*,bool)));

  connect(w->Glw(), SIGNAL(keyPress(QKeyEvent*)),
      SLOT(catchKeyPress(QKeyEvent*)));

  connect(w->Glw(), SIGNAL(mouseDoubleClick(QMouseEvent*)),
      SLOT(catchMouseDoubleClick(QMouseEvent*)));

  // ----------- init dialog-objects -------------------

  qm= new QuickMenu(this, contr);
  qm->hide();
  mainToolbar->addAction( showQuickmenuAction         );

  mm= new MapDialog(this, contr);
  mm->hide();
  mainToolbar->addAction( showMapDialogAction         );

  fm= new FieldDialog(this, contr);
  fm->hide();
  mainToolbar->addAction( showFieldDialogAction       );

  om= new ObsDialog(this, contr);
  om->hide();
  mainToolbar->addAction( showObsDialogAction         );

  sm= new SatDialog(this, contr);
  sm->hide();
  mainToolbar->addAction( showSatDialogAction         );

  stm= new StationDialog(this, contr);
  stm->hide();
  mainToolbar->addAction( showStationDialogAction     );

  objm = new ObjectDialog(this,contr);
  objm->hide();
  mainToolbar->addAction( showObjectDialogAction      );

  trajm = new TrajectoryDialog(this,contr);
  trajm->setFocusPolicy(Qt::StrongFocus);
  trajm->hide();
  mainToolbar->addAction( showTrajecDialogAction      );

  annom = new AnnotationDialog(this,contr);
  annom->setFocusPolicy(Qt::StrongFocus);
  annom->hide();

  measurementsm = new MeasurementsDialog(this,contr);
  measurementsm->setFocusPolicy(Qt::StrongFocus);
  measurementsm->hide();
  mainToolbar->addAction( showMeasurementsDialogAction   );

  uffm = new UffdaDialog(this,contr);
  uffm->hide();

  mailm = new MailDialog(this,contr);
  mailm->hide();

  EditItems::DrawingDialog *drawingDialog = new EditItems::DrawingDialog(this, contr);
  drawingDialog->hide();
  addDialog(drawingDialog);

  { WebMapDialog* wmd = new WebMapDialog(this, contr);
    wmd->hide();
    addDialog(wmd);

    connect(WebMapManager::instance(), SIGNAL(webMapsReady()),
        this, SLOT(requestBackgroundBufferUpdate()));
  }

  editDrawingToolBar = EditItems::ToolBar::instance();
  editDrawingToolBar->setObjectName("PaintToolBar");
  addToolBar(Qt::BottomToolBarArea, editDrawingToolBar);
  editDrawingToolBar->hide();
  EditItemManager *editm = EditItemManager::instance();
  connect(editm, SIGNAL(setWorkAreaCursor(const QCursor &)), SLOT(setWorkAreaCursor(const QCursor &)));
  connect(editm, SIGNAL(unsetWorkAreaCursor()), SLOT(unsetWorkAreaCursor()));
  connect(editm, SIGNAL(itemStatesReplaced()), SLOT(updatePlotElements()));
  connect(drawingDialog, SIGNAL(editingMode(bool)), SLOT(setEditDrawingMode(bool)));

  textview = new TextView(this);
  textview->setMinimumWidth(300);
  connect(textview,SIGNAL(printClicked(int)),SLOT(sendPrintClicked(int)));
  textview->hide();

  em= new EditDialog( this, contr );
  em->hide();
  mainToolbar->addAction( showEditDialogAction );

  //used for testing qickMenus without dialogs
  //connect(qm, SIGNAL(Apply(const vector<std::string>&,bool)),
  //  	  SLOT(quickMenuApply(const vector<std::string>&)));


  connect(qm, SIGNAL(Apply(const std::vector<std::string>&,bool)),
      SLOT(recallPlot(const std::vector<std::string>&,bool)));
  connect(em, SIGNAL(Apply(const std::vector<std::string>&,bool)),
      SLOT(recallPlot(const std::vector<std::string>&,bool)));


  // Mark trajectory positions
  connect(trajm, SIGNAL(markPos(bool)), SLOT(trajPositions(bool)));
  connect(trajm, SIGNAL(updateTrajectories()),SLOT(updateGLSlot()));

  // Mark measurement positions
  connect(measurementsm, SIGNAL(markMeasurementsPos(bool)), SLOT(measurementsPositions(bool)));
  connect(measurementsm, SIGNAL(updateMeasurements()),SLOT(updateGLSlot()));

  connect(em, SIGNAL(editUpdate()), SLOT(requestBackgroundBufferUpdate()));
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
  connect( annom, SIGNAL(AnnotationApply()),  SLOT(MenuOK()));

  connect( fm, SIGNAL(FieldHide()),  SLOT(fieldMenu()));
  connect( fm, SIGNAL(finished(int)),  SLOT(fieldMenu(int)));
  connect( om, SIGNAL(ObsHide()),    SLOT(obsMenu()));
  connect( om, SIGNAL(finished(int)),  SLOT(obsMenu(int)));
  connect( sm, SIGNAL(SatHide()),    SLOT(satMenu()));
  connect( sm, SIGNAL(finished(int)),  SLOT(satMenu(int)));
  connect( stm, SIGNAL(StationHide()), SLOT(stationMenu()));
  connect( stm, SIGNAL(finished(int)),  SLOT(stationMenu(int)));
  connect( mm, SIGNAL(MapHide()),    SLOT(mapMenu()));
  connect( mm, SIGNAL(finished(int)),  SLOT(mapMenu(int)));
  connect( em, SIGNAL(EditHide()),   SLOT(editMenu()));
  connect( qm, SIGNAL(QuickHide()),  SLOT(quickMenu()));
  connect( qm, SIGNAL(finished(int)),  SLOT(quickMenu(int)));
  connect( objm, SIGNAL(ObjHide()),  SLOT(objMenu()));
  connect( objm, SIGNAL(finished(int)),  SLOT(objMenu(int)));
  connect( trajm, SIGNAL(TrajHide()),SLOT(trajMenu()));
  connect( trajm, SIGNAL(finished(int)),  SLOT(trajMenu(int)));
  connect( annom, SIGNAL(AnnotationHide()),SLOT(AnnotationMenu()));
  connect( annom, SIGNAL(finished(int)),  SLOT(AnnotationMenu(int)));
  connect( measurementsm, SIGNAL(MeasurementsHide()),SLOT(measurementsMenu()));
  connect( measurementsm, SIGNAL(finished(int)),  SLOT(measurementsMenu(int)));
  connect( uffm, SIGNAL(uffdaHide()), SLOT(uffMenu()));
  connect( uffm, SIGNAL(finished(int)), SLOT(uffMenu(int)));

  // update field dialog when editing field
  connect( em, SIGNAL(emitFieldEditUpdate(std::string)),
      fm, SLOT(fieldEditUpdate(std::string)));

  // resize main window according to the active map area when using
  // an editing tool
  connect( em, SIGNAL(emitResize(int, int)),
      this, SLOT(winResize(int, int)));

  // HELP
  connect( fm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( om, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( sm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( stm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( mm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( em, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( qm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( objm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( trajm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( uffm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));

  connect(w->Glw(),SIGNAL(objectsChanged()),em, SLOT(undoFrontsEnable()));
  connect(w->Glw(),SIGNAL(fieldsChanged()), em, SLOT(undoFieldsEnable()));

  mainToolbar->addSeparator();

  // vertical profiles
  // create a new main window
#ifndef DISABLE_VPROF
  vpWindow = new VprofWindow();
  connect(vpWindow,SIGNAL(VprofHide()),SLOT(hideVprofWindow()));
  connect(vpWindow,SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect(vpWindow,SIGNAL(stationChanged(const std::vector<std::string> &)),
      SLOT(stationChangedSlot(const std::vector<std::string> &)));
  connect(vpWindow,SIGNAL(modelChanged()),SLOT(modelChangedSlot()));
  mainToolbar->addAction( showProfilesDialogAction    );
#endif

  // vertical crossections
  // create a new main window
#ifndef DISABLE_VCROSS
  vcInterface.reset(new VcrossWindowInterface);
  if (vcInterface.get()) {
    connect(vcInterface.get(), SIGNAL(VcrossHide()),
        SLOT(hideVcrossWindow()));
    connect(vcInterface.get(), SIGNAL(requestHelpPage(const std::string&, const std::string&)),
        help, SLOT(showsource(const std::string&, const std::string&)));
    connect(vcInterface.get(), SIGNAL(requestVcrossEditor(bool, bool)),
        SLOT(onVcrossRequestEditManager(bool, bool)));
    connect(vcInterface.get(), SIGNAL(crossectionChanged(const QString &)),
        SLOT(crossectionChangedSlot(const QString &)));
    connect(vcInterface.get(), SIGNAL(crossectionSetChanged(const LocationData&)),
        SLOT(crossectionSetChangedSlot(const LocationData&)));
    connect(vcInterface.get(), SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)),
        SLOT(updateVcrossQuickMenuHistory(const std::string&, const std::vector<std::string>&)));
    connect (vcInterface.get(), SIGNAL(vcrossHistoryPrevious()),
        SLOT(prevHVcrossPlot()));
    connect (vcInterface.get(), SIGNAL(vcrossHistoryNext()),
        SLOT(nextHVcrossPlot()));
  }
  mainToolbar->addAction( showCrossSectionDialogAction);
#endif

  // Wave spectrum
  // create a new main window
#ifndef DISABLE_WAVESPEC
  spWindow = new SpectrumWindow();
  connect(spWindow,SIGNAL(SpectrumHide()),SLOT(hideSpectrumWindow()));
  connect(spWindow,SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect(spWindow,SIGNAL(spectrumChanged(const QString &)),
      SLOT(spectrumChangedSlot(const QString &)));
  connect(spWindow,SIGNAL(spectrumSetChanged()),
      SLOT(spectrumSetChangedSlot()));
  mainToolbar->addAction( showWaveSpectrumDialogAction);
#endif

  // browse plots
  browser= new BrowserBox(this);
  connect(browser, SIGNAL(selectplot()),this,SLOT(browserSelect()));
  connect(browser, SIGNAL(cancel()),   this, SLOT(browserCancel()));
  connect(browser, SIGNAL(prevplot()), this, SLOT(prevQPlot()));
  connect(browser, SIGNAL(nextplot()), this, SLOT(nextQPlot()));
  connect(browser, SIGNAL(prevlist()), this, SLOT(prevList()));
  connect(browser, SIGNAL(nextlist()), this, SLOT(nextList()));
  browser->hide();

  connect(fm, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
          tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(om, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
          tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(sm, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&,bool)),
          tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&,bool)));

  connect(em, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
          tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(objm, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&,bool)),
          tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&,bool)));

  if (vpWindow) {
    connect(vpWindow, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
            tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

    connect(vpWindow, SIGNAL(setTime(const std::string&, const miutil::miTime&)),
            tslider, SLOT(setTime(const std::string&, const miutil::miTime&)));
  }
  if (vcInterface.get()) {
    connect(vcInterface.get(), SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
        tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));
    
    connect(vcInterface.get(), SIGNAL(setTime(const std::string&, const miutil::miTime&)),
        tslider, SLOT(setTime(const std::string&, const miutil::miTime&)));
  }
  if (spWindow) {
    connect(spWindow, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
            tslider, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

    connect( spWindow ,SIGNAL(setTime(const std::string&, const miutil::miTime&)),
        tslider,SLOT(setTime(const std::string&, const miutil::miTime&)));
  }

  mainToolbar->addSeparator();
  mainToolbar->addAction( showResetAllAction );

  setAcceptDrops(true);

  connect(qApp, SIGNAL(aboutToQuit()),
      SLOT(writeLogFile()));

  METLIBS_LOG_INFO("Creating DianaMainWindow done");
}


void DianaMainWindow::start()
{
  if (DiCanvas* c = w->Glw()->canvas())
    c->parseFontSetup();

  // read the log file
  readLogFile();

  // init paint mode
  setEditDrawingMode(editDrawingToolBar->isVisible());

  // init quickmenus
  qm->start();

  // make initial plot
  MenuOK();

  //statusBar()->message( tr("Ready"), 2000 );

  dialogChanged = false;

  if (showelem){
    updatePlotElements();
    statusbuttons->show();
  } else {
    statusbuttons->reset();
    statusbuttons->hide();
  }

  optOnOffAction->      setChecked( showelem );
  optAutoElementAction->setChecked( autoselect );

  w->setGlwFocus();
}

DianaMainWindow::~DianaMainWindow()
{
  contr->setCanvas(0);
  // no explicit destruction is necessary at this point
}


void DianaMainWindow::focusInEvent( QFocusEvent * )
{
  w->setGlwFocus();
}

void DianaMainWindow::quickMenuApply(const vector<string>& s)
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;
  contr->plotCommands(s);

  map<string,vector<miutil::miTime> > times;
  contr->getPlotTimes(times);

  map<string,vector<miutil::miTime> >::iterator it = times.begin();
  while (it != times.end()) {
    tslider->insert(it->first, it->second);
    ++it;
  }

  miutil::miTime t= tslider->Value();
  contr->setPlotTime(t);
  contr->updatePlots();
  requestBackgroundBufferUpdate();
  timeChanged();

  dialogChanged=false;
}

void DianaMainWindow::resetAll()
{
  mm->useFavorite();
  vector<string> pstr = mm->getOKString();
  recallPlot(pstr, true);
  MenuOK();
}

void DianaMainWindow::recallPlot(const vector<string>& vstr, bool replace)
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;

  if (!vstr.empty() && vstr[0] == "VCROSS") {
    vcrossMenu();
    if (vcInterface.get())
      vcInterface->parseQuickMenuStrings(vstr);
  } else {

    // strings for each dialog
    vector<string> mapcom,obscom,satcom,statcom,objcom,labelcom,fldcom;
    map<std::string, vector<std::string> > dialog_com;
    int n= vstr.size();
    // sort strings..
    for (int i=0; i<n; i++){
      std::string s= vstr[i];
      miutil::trim(s);
      if (s.empty())
        continue;
      vector<std::string> vs= miutil::split(s, " ");
      std::string pre= miutil::to_upper(vs[0]);
      if (pre=="MAP") mapcom.push_back(s);
      else if (pre=="AREA") mapcom.push_back(s);
      else if (pre=="FIELD") fldcom.push_back(s);
      else if (pre=="OBS") obscom.push_back(s);
      else if (pre=="SAT") satcom.push_back(s);
      else if (pre=="STATION") statcom.push_back(s);
      else if (pre=="OBJECTS") objcom.push_back(s);
      else if (pre=="LABEL") labelcom.push_back(s);
      else if (dialogNames.find(pre) != dialogNames.end()) {
        dialog_com[pre].push_back(s);
      }
    }

    // feed strings to dialogs
    if (replace || mapcom.size()) mm->putOKString(mapcom);
    if (replace || fldcom.size()) fm->putOKString(fldcom);
    if (replace || obscom.size()) om->putOKString(obscom);
    if (replace || satcom.size()) sm->putOKString(satcom);
    if (replace || statcom.size()) stm->putOKString(statcom);
    if (replace || objcom.size()) objm->putOKString(objcom);
    if (replace || labelcom.size()) annom->putOKString(labelcom);

    // Other data sources

    // If the strings are being replaced then update each of the
    // dialogs whether it has a command or not. Otherwise, only
    // update those with a corresponding string.
    map<std::string, DataDialog *>::iterator it;
    for (it = dialogNames.begin(); it != dialogNames.end(); ++it) {
      DataDialog *dialog = it->second;
      if (replace || dialog_com.find(it->first) != dialog_com.end())
        dialog->putOKString(dialog_com[it->first]);
    }

    // call full plot
    push_command= false; // do not push this command on stack
    MenuOK();
    push_command= true;
  }
}

void DianaMainWindow::toggleEditDrawingMode()
{
  if (editDrawingToolBar->isVisible())
    setEditDrawingMode(false);
  else
    setEditDrawingMode(true);

  METLIBS_LOG_DEBUG("DianaMainWindow::toggleEditDrawingMode enabled " << editDrawingToolBar->isVisible());
}

void DianaMainWindow::setEditDrawingMode(bool enabled)
{
  if (enabled)
    editDrawingToolBar->show();
  else
    editDrawingToolBar->hide();

  EditItemManager::instance()->setEditing(enabled);
}

void DianaMainWindow::winResize(int width, int height)
{
  this->resize(width, height);
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

void DianaMainWindow::getPlotStrings(vector<string> &pstr, vector<string> &shortnames)
{
  vector<string> diagstr;

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

  // annotation
  bool remove = (contr->getMapMode() != normal_mode || tslider->numTimes()==0);
  diagstr = annom->getOKString();
  for ( size_t i =0; i< diagstr.size(); i++) {
    if(!remove || not miutil::contains(diagstr[i], "$")) { //remove labels with time
      pstr.push_back(diagstr[i]);
    }
  }

  // Other data sources
  map<QAction*, DataDialog*>::iterator it;
  for (it = dialogs.begin(); it != dialogs.end(); ++it) {
    diagstr = it->second->getOKString();
    pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  }

  // remove empty lines
  for (unsigned int i = 0; i < pstr.size(); ++i){
    miutil::trim(pstr[i]);
    if (pstr[i].empty()) {
      pstr.erase(pstr.begin()+i);
      --i;
      continue;
    }
  }
}

std::vector<PlotElement> DianaMainWindow::getPlotElements() const
{
  std::vector<PlotElement> pe = contr->getPlotElements();
  return pe;
}

void DianaMainWindow::updatePlotElements()
{
  statusbuttons->setPlotElements(getPlotElements());
}

void DianaMainWindow::MenuOK()
{
  METLIBS_LOG_SCOPE();

  diutil::OverrideCursor waitCursor;

  vector<string> pstr;
  vector<string> shortnames;

  getPlotStrings(pstr, shortnames);

//init level up/down arrows
  toolLevelUpAction->setEnabled(fm->levelsExists(true,0));
  toolLevelDownAction->setEnabled(fm->levelsExists(false,0));
  toolIdnumUpAction->setEnabled(fm->levelsExists(true,1));
  toolIdnumDownAction->setEnabled(fm->levelsExists(false,1));

  if (METLIBS_LOG_DEBUG_ENABLED()) {
    string logstr = "------- the final string from all dialogs:\n";
    for (unsigned int i = 0; i < pstr.size(); ++i)
      logstr += pstr[i] + "\n";
    METLIBS_LOG_DEBUG(logstr);
  }

  miutil::miTime t = tslider->Value();
  contr->plotCommands(pstr);
  contr->setPlotTime(t);
  contr->updatePlots();
  METLIBS_LOG_INFO(contr->getMapArea());

  requestBackgroundBufferUpdate();
  timeChanged();
  dialogChanged = false;

  // push command on history-stack
  if (push_command){ // only when proper menuok
    // make shortname
    std::string plotname;
    int m= shortnames.size();
    for (int j=0; j<m; j++)
      if (not shortnames[j].empty()){
        plotname+= shortnames[j];
        if (j!=m-1) plotname+= " ";
      }
    qm->pushPlot(plotname,pstr,QuickMenu::MAP);
  }
}

void DianaMainWindow::updateVcrossQuickMenuHistory(const std::string& plotname, const std::vector<std::string>& pstr)
{
  qm->pushPlot(plotname, pstr, QuickMenu::VCROSS);
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
  std::string listname,name;
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

static void toggleDialogVisibility(QDialog* dialog, QAction* dialogAction, int result = -1)
{
  bool visible = dialog->isVisible();
  if (result == -1) {
    dialog->setVisible(!visible);
    dialogAction->setChecked(!visible);
  } else {
    dialog->setVisible(false);
    dialogAction->setChecked(false);
  }
}

void DianaMainWindow::quickMenu(int result)
{
  toggleDialogVisibility(qm, showQuickmenuAction, result);
}

void DianaMainWindow::fieldMenu(int result)
{
  toggleDialogVisibility(fm, showFieldDialogAction, result);
}

void DianaMainWindow::obsMenu(int result)
{
  toggleDialogVisibility(om, showObsDialogAction, result);
}

void DianaMainWindow::satMenu(int result)
{
  toggleDialogVisibility(sm, showSatDialogAction, result);
}

void DianaMainWindow::stationMenu(int result)
{
  toggleDialogVisibility(stm, showStationDialogAction, result);
}

void DianaMainWindow::uffMenu(int result)
{
  // If the dialog returned a non-negative result then it must be open.
  bool b = uffm->isVisible() | (result != -1);
  if( b ){
    uffm->hide();
    uffm->clearSelection();
    contr->stationCommand("hide","uffda");
  } else {
    uffm->show();
    contr->stationCommand("show","uffda");
  }
  requestBackgroundBufferUpdate();
  showUffdaDialogAction->setChecked( !b );
}


void DianaMainWindow::mapMenu(int result)
{
  toggleDialogVisibility(mm, showMapDialogAction, result);
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


void DianaMainWindow::getFieldPlotOptions(map< std::string, map<std::string,std::string> >& options)
{
  if (fm){
    fm->getEditPlotOptions(options);
  }
}


void DianaMainWindow::objMenu(int result)
{
  toggleDialogVisibility(objm, showObjectDialogAction, result);
}

void DianaMainWindow::trajMenu(int result)
{
  // If the dialog returned a non-negative result then it must be open.
  bool b = trajm->isVisible() | (result != -1);
  if (b){
    trajm->hide();
  } else {
    trajm->showplus();
  }
  updateGLSlot();
  showTrajecDialogAction->setChecked( !b );
}

void DianaMainWindow::AnnotationMenu(int result)
{
  toggleDialogVisibility(annom, showAnnotationDialogAction, result);
}

void DianaMainWindow::measurementsMenu(int result)
{
  // If the dialog returned a non-negative result then it must be open.
  bool b = measurementsm->isVisible() | (result != -1);
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
  if (!vpWindow)
    return;
  if (vpWindow->active) {
    vpWindow->raise();
  } else {
    vprofStartup();
    updateGLSlot();
  }
}


void DianaMainWindow::vcrossMenu()
{
  if (!vcInterface.get())
    return;

  miutil::miTime t;
  contr->getPlotTime(t);
  vcInterface->mainWindowTimeChanged(t);
  vcInterface->makeVisible(true);
}


void DianaMainWindow::spectrumMenu()
{
  if (!spWindow)
    return;
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
    if (miutil::contains(infoFiles[action->text().toStdString()].filename, "http")) {
      QDesktopServices::openUrl(QUrl(infoFiles[action->text().toStdString()].filename.c_str()));
    } else {
      TextDialog* td= new TextDialog(this, infoFiles[action->text().toStdString()]);
      td->show();
    }
  }
}


void DianaMainWindow::vprofStartup()
{
  if (!vpWindow)
    return;
  miutil::miTime t;
  contr->getPlotTime(t);
  vpWindow->startUp(t);
  vpWindow->show();
  contr->stationCommand("show","vprof");
}


void DianaMainWindow::spectrumStartup()
{
  if (!spWindow)
    return;
  miutil::miTime t;
  contr->getPlotTime(t);
  spWindow->startUp(t);
  spWindow->show();
  contr->stationCommand("show","spectrum");
}


void DianaMainWindow::hideVprofWindow()
{
  METLIBS_LOG_SCOPE();
  if (!vpWindow)
    return;
  vpWindow->hide();
  // delete stations
  contr->stationCommand("delete","vprof");
  updateGLSlot();
}


void DianaMainWindow::hideVcrossWindow()
{
  METLIBS_LOG_SCOPE();
  if (!vcInterface.get())
    return;

  // vcInterface hidden and locationPlots deleted
  vcInterface->makeVisible(false);
  contr->deleteLocation(LOCATIONS_VCROSS);
  updateGLSlot();
}


void DianaMainWindow::hideSpectrumWindow()
{
  METLIBS_LOG_SCOPE();
  if (!spWindow)
    return;
  // spWindow and stations only hidden, should also be possible to
  //delete all !
  spWindow->hide();
  // delete stations
  contr->stationCommand("hide","spectrum");
  updateGLSlot();
}


void DianaMainWindow::stationChangedSlot(const vector<std::string>& station)
{
  METLIBS_LOG_SCOPE();
  contr->stationCommand("setSelectedStations",station,"vprof");
  requestBackgroundBufferUpdate();
}


void DianaMainWindow::modelChangedSlot()
{
  METLIBS_LOG_SCOPE();
  if (!vpWindow)
    return;
  StationPlot * sp = vpWindow->getStations();
  sp->setName("vprof");
  sp->setImage("vprof_normal","vprof_selected");
  sp->setIcon("vprof_icon");
  contr->putStations(sp);
  updateGLSlot();
}

void DianaMainWindow::onVcrossRequestLoadCrossectionsFile(const QStringList& filenames)
{
  vcrossEditManagerEnableSignals();
}


void DianaMainWindow::vcrossEditManagerEnableSignals()
{
  if (not vcrossEditManagerConnected) {
    vcrossEditManagerConnected = true;

    EditItemManager::instance()->enableItemChangeNotification();
    connect(EditItemManager::instance(), SIGNAL(itemChanged(const QVariantMap &)),
        vcInterface.get(), SLOT(editManagerChanged(const QVariantMap &)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(itemRemoved(int)),
        vcInterface.get(), SLOT(editManagerRemoved(int)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(editing(bool)),
        vcInterface.get(), SLOT(editManagerEditing(bool)), Qt::UniqueConnection);
  }
}


void DianaMainWindow::onVcrossRequestEditManager(bool on, bool timeGraph)
{
  if (on) {
    if (timeGraph)
      EditItems::ToolBar::instance()->setCreateSymbolAction(TIME_GRAPH_TYPE);
    else
      EditItems::ToolBar::instance()->setCreatePolyLineAction(CROSS_SECTION_TYPE);

    setEditDrawingMode(true);
    vcrossEditManagerEnableSignals();
  } else {
    setEditDrawingMode(false);
  }
}


void DianaMainWindow::crossectionChangedSlot(const QString& name)
{
  METLIBS_LOG_DEBUG("DianaMainWindow::crossectionChangedSlot to " << name.toStdString());
  std::string s= name.toStdString();
  contr->setSelectedLocation(LOCATIONS_VCROSS, s);
  requestBackgroundBufferUpdate();
}


void DianaMainWindow::crossectionSetChangedSlot(const LocationData& locations)
{
  METLIBS_LOG_SCOPE();
  if (locations.elements.empty())
    contr->deleteLocation(LOCATIONS_VCROSS);
  else if (locations.name == LOCATIONS_VCROSS)
    contr->putLocation(locations);
  else {
    METLIBS_LOG_ERROR("bad name '" << locations.name << "' for vcross location data");
    return;
  }
  updateGLSlot();
}


void DianaMainWindow::spectrumChangedSlot(const QString& station)
{
  //METLIBS_LOG_DEBUG("DianaMainWindow::spectrumChangedSlot to " << name);
  METLIBS_LOG_DEBUG("DianaMainWindow::spectrumChangedSlot");
  string s =station.toStdString();
  vector<string> data;
  data.push_back(s);
  contr->stationCommand("setSelectedStation",data,"spectrum");
  //  contr->setSelectedStation(s, "spectrum");
  requestBackgroundBufferUpdate();
}


void DianaMainWindow::spectrumSetChangedSlot()
{
  METLIBS_LOG_SCOPE();
  if (!spWindow)
    return;
  StationPlot * stp = spWindow->getStations() ;
  stp->setName("spectrum");
  stp->setImage("spectrum_normal","spectrum_selected");
  stp->setIcon("spectrum_icon");
  contr->putStations(stp);
  updateGLSlot();
}


void DianaMainWindow::connectionClosed()
{
  //  METLIBS_LOG_DEBUG("Connection closed");
  qsocket = false;

  contr->stationCommand("delete","all");
  std::string dummy;
  contr->areaCommand("delete","all","all",-1);

  //remove times
  vector<std::string> type = timecontrol->deleteType(-1);
  for(unsigned int i=0;i<type.size();i++)
    tslider->deleteType(type[i]);

  textview->hide();
  contr->processHqcCommand("remove");
  om->setPlottype("Hqc_synop",false);
  om->setPlottype("Hqc_list",false);
  MenuOK();
  if (showelem) updatePlotElements();

}


void DianaMainWindow::processLetter(const miMessage &letter)
{
  METLIBS_LOG_SCOPE();
  std::string from = miutil::from_number(letter.from);
  METLIBS_LOG_DEBUG("Command: "<<letter.command<<"  ");
  METLIBS_LOG_DEBUG(" Description: "<<letter.description<<"  ");
  METLIBS_LOG_DEBUG(" commonDesc: "<<letter.commondesc<<"  ");
  METLIBS_LOG_DEBUG(" Common: "<<letter.common<<"  ");
  for( size_t i=0;i<letter.data.size();i++)
    if(letter.data[i].length()<80)
      METLIBS_LOG_DEBUG(" data["<<i<<"]:"<<letter.data[i]);;
  METLIBS_LOG_DEBUG(" From: "<<from);
  METLIBS_LOG_DEBUG("To: "<<letter.to);

  if( letter.command == "menuok"){
    MenuOK();
  }

  else if( letter.command == qmstrings::init_HQC_params){
    if( contr->initHqcdata(letter.from,
        letter.commondesc,
        letter.common,
        letter.description,
        letter.data) ){
      if(letter.common.find("synop") !=std::string::npos )
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
      vector<string> tmp;
      boost::algorithm::split(tmp, letter.data[0], boost::algorithm::is_any_of(":"));
      if(tmp.size()==2){
        float lat= atof(tmp[0].c_str());
        float lon= atof(tmp[1].c_str());
        float x=0, y=0;
        contr->GeoToPhys(lat,lon,x,y);
        int ix= int(x);
        int iy= int(y);
        //find the name of station we clicked at (from plotModul->stationPlot)
        std::string station = contr->findStation(ix,iy,letter.command);
        //now tell vpWindow about new station (this calls vpManager)
        if (vpWindow && !station.empty())
          vpWindow->changeStation(station);

        if (vcInterface.get())
          vcInterface->showTimegraph(LonLat::fromDegrees(lon, lat));
      }
    }
  }

  else if (letter.command == qmstrings::vcross) {
    //description: name
    vcrossMenu();
    if (letter.data.size()) {
        //tell vcInterface to plot this crossection
        if (vcInterface.get())
          vcInterface->changeCrossection(letter.data[0]);
    }
  }

  else if (letter.command == qmstrings::addimage){
    // description: name:image

    QtImageGallery ig;
    int n = letter.data.size();
    for(int i=0; i<n; i++){
      // separate name and data
      vector<string> vs;
      boost::algorithm::split(vs, letter.data[i], boost::algorithm::is_any_of(":"));
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
    string commondesc = letter.commondesc;
    string common = letter.common;
    string description = letter.description;

    if(letter.description.find(";") != letter.description.npos){
      vector<string> desc;
      boost::algorithm::split(desc, letter.description, boost::algorithm::is_any_of(";"));
      if( desc.size() < 2 ) return;
      std::string dataSet = desc[0];
      description=desc[1];
      commondesc = "dataset:" + letter.commondesc;
      common = dataSet + ":" + letter.common;
    }

    contr->makeStationPlot(commondesc,common,
        description,
        letter.from,letter.data);

    //    sendSelectedStations(qmstrings::selectposition);
    return;
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
    if (showelem) updatePlotElements();

  }

  else if (letter.command == qmstrings::hidepositions ){
    //description: dataset
    contr->stationCommand("hide",letter.description,letter.from);
    if (showelem) updatePlotElements();

  }

  else if (letter.command == qmstrings::changeimageandtext ){
    //METLIBS_LOG_DEBUG("Change text and image\n");
    //description: dataSet;stationname:image:text:alignment
    //find name of data set from description
    vector<string> desc;
    boost::algorithm::split(desc, letter.description, boost::algorithm::is_any_of(";"));
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
    if (showelem) updatePlotElements();
  }

  else if (letter.command == qmstrings::areacommand ){
    //commondesc command:dataSet
    vector<string> token;
    boost::algorithm::split(token, letter.common, boost::algorithm::is_any_of(":"));
    if(token.size()>1){
      int n = letter.data.size();
      if(n==0)
        contr->areaCommand(token[0],token[1],std::string(),letter.from);
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
    if (showelem) updatePlotElements();
  }

  else if (letter.command == qmstrings::showtext ){
    //description: station:text
    if(letter.data.size()){
      textview_id = letter.from;
      vector<string> token;
      boost::algorithm::split(token, letter.data[0], boost::algorithm::is_any_of(":"));
      if(token.size() == 2){
        std::string name = pluginB->getClientName(letter.from).toStdString();
        textview->setText(textview_id,name,token[1]);
        textview->show();
      }
    }
  }

  else if (letter.command == qmstrings::enableshowtext ){
    //description: dataset:on/off
    if(letter.data.size()){
      vector<string> token;
      boost::algorithm::split(token, letter.data[0], boost::algorithm::is_any_of(":"));
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
    vector<string> token;
    boost::algorithm::split(token, letter.common, boost::algorithm::is_any_of(":"));
    if(token.size()<2) return;
    int id =atoi(token[0].c_str());
    //remove stationPlots from this client
    contr->stationCommand("delete","",id);
    //remove areas from this client
    contr->areaCommand("delete","all","all",id);
    //remove times
    vector<std::string> type = timecontrol->deleteType(id);
    for(unsigned int i=0;i<type.size();i++)
      tslider->deleteType(type[i]);
    //hide textview
    textview->deleteTab(id);
    //     if(textview && id == textview_id )
    //       textview->hide();
    //remove observations from hqc
    string value = boost::algorithm::to_lower_copy(token[1]);
    if( value =="hqc"){
      contr->processHqcCommand("remove");
      om->setPlottype("Hqc_synop",false);
      om->setPlottype("Hqc_list",false);
      MenuOK();
    }
    if (showelem) updatePlotElements();

  }

  else if (letter.command == qmstrings::newclient ){
    qsocket = true;
    autoredraw[atoi(letter.common.c_str())] = true;
    autoredraw[0] = true; //from server
  }

  else if (letter.command == qmstrings::autoredraw ){
    if(letter.common == "false")
      autoredraw[letter.from] = false;
  }

  else if (letter.command == qmstrings::redraw ){
    requestBackgroundBufferUpdate();
  }

  else if (letter.command == qmstrings::settime ){
    int n = letter.data.size();
    if(letter.commondesc == "datatype"){
      timecontrol->useData(letter.common,letter.from);
      vector<miutil::miTime> times;
      for(int i=0;i<n;i++)
        times.push_back(miutil::miTime(letter.data[i]));
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
    vector<string> v1, v2;
    getPlotStrings(v1, v2);

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

  else if (letter.command == qmstrings::getmaparea) { //obsolete
    miMessage l;
    l.to = letter.from;
    l.command = qmstrings::maparea;
    std::string message = "Obsolet command: use getproj4maparea";
    l.data.push_back(message);
    sendLetter(l);
    return; // no need to repaint
  }

  // If autoupdate is active, reread sat/radarfiles and
  // show the latest timestep
  else if (letter.command == qmstrings::directory_changed) {
    METLIBS_LOG_DEBUG(letter.command <<" received");

    if (doAutoUpdate) {
      // running animation
      if (timeron != 0) {
        om->getTimes();
        sm->RefreshList();
        if (contr->satFileListChanged() || contr->obsTimeListChanged()) {
          //METLIBS_LOG_DEBUG("new satfile or satfile deleted!");
          //METLIBS_LOG_DEBUG("setPlotTime");
          //	  METLIBS_LOG_DEBUG("doAutoUpdate  timer on");
          contr->satFileListUpdated();
          contr->obsTimeListUpdated();
        }
      }
      else {
        // Avoid not needed updates
        diutil::OverrideCursor waitCursor;
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
          //METLIBS_LOG_DEBUG("new satfile or satfile deleted!");
          //METLIBS_LOG_DEBUG("setPlotTime");
          setPlotTime(t);
          contr->satFileListUpdated();
          contr->obsTimeListUpdated();
        }
        //METLIBS_LOG_DEBUG("stepforward");
        stepforward();
      }
    }
  }

  // If autoupdate is active, do the same thing as
  // when the user presses the updateObs button.
  else if (letter.command == qmstrings::file_changed) {
    METLIBS_LOG_DEBUG(letter.command <<" received");
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
    requestBackgroundBufferUpdate();
}

void DianaMainWindow::sendPrintClicked(int id)
{
  miMessage l;
  l.command = qmstrings::printclicked;
  sendLetter(l);
}


void DianaMainWindow::sendLetter(miMessage& letter)
{
  METLIBS_LOG_SCOPE(pluginB->getSelectedClientNames());
  miQMessage qmsg;
  int from, to;
  convert(letter, from, to, qmsg);
  if (to != -1)
    pluginB->sendMessage(qmsg, to);
  else
    // send to all selected clients, -1 is not allowed any more
    pluginB->sendMessage(qmsg);
}

void DianaMainWindow::updateObs()
{
  METLIBS_LOG_DEBUG("DianaMainWindow::obsUpdate()");
  diutil::OverrideCursor waitCursor;
  contr->updateObs();
  requestBackgroundBufferUpdate();
}

void DianaMainWindow::autoUpdate()
{
  doAutoUpdate = !doAutoUpdate;
  METLIBS_LOG_DEBUG("DianaMainWindow::autoUpdate(): " << doAutoUpdate);
  autoUpdateAction->setChecked(doAutoUpdate);
}

void DianaMainWindow::requestBackgroundBufferUpdate()
{
  w->Glw()->requestBackgroundBufferUpdate();
  w->updateGL();
}

void DianaMainWindow::updateGLSlot()
{
  requestBackgroundBufferUpdate();
  if (showelem)
    updatePlotElements();
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
  QDesktopServices::openUrl(QUrl(LocalSetupParser::basicValue("testresults").c_str()));
}

void DianaMainWindow::about()
{
  QString str =
      tr("Diana - a 2D presentation system for meteorological data, including fields, observations,\nsatellite- and radarimages, vertical profiles and cross sections.\nDiana has tools for on-screen fieldediting and drawing of objects (fronts, areas, symbols etc.\n")
      +"\n"
      + tr("To report a bug or enter an enhancement request, please use the bug tracking tool at http://diana.bugs.met.no (met.no users only). \n")
      +"\n\n"
      + tr("version:") + " " + VERSION + "\n"
      + tr("build: ") + " " + diana_build_string + "\n"
      + tr("commit: ") + " " + diana_build_commit;

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
  METLIBS_LOG_TIME();
  diutil::OverrideCursor waitCursor;
  if (contr->setPlotTime(t)) {
    contr->updatePlots();
    requestBackgroundBufferUpdate();
  }
  timeChanged();
}

void DianaMainWindow::timeChanged(){
  //to be done whenever time changes (step back/forward, MenuOK etc.)
  setTimeLabel();
  objm->commentUpdate();
  satFileListUpdate();
  miutil::miTime t;
  contr->getPlotTime(t);
  if (vpWindow) vpWindow->mainWindowTimeChanged(t);
  if (spWindow) spWindow->mainWindowTimeChanged(t);
  if (vcInterface.get()) vcInterface->mainWindowTimeChanged(t);
  if (showelem) updatePlotElements();

  //update sat channels in statusbar
  vector<string> channels = contr->getCalibChannels();
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
  diutil::OverrideCursor waitCursor;
  // update field dialog
  fm->changeLevel(increment,0);
  MenuOK();
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
  diutil::OverrideCursor waitCursor;
  // update field dialog
  fm->changeLevel(increment, 1);
  MenuOK();
}


void DianaMainWindow::saveraster()
{
  METLIBS_LOG_SCOPE();

#if 1
  static QString fname = "./"; // keep users preferred image-path for later
  const QString s = QFileDialog::getSaveFileName(this,
      tr("Save plot as image"),
      fname,
      tr("Images (*.png *.jpeg *.jpg *.xpm *.bmp *.svg);;PDF Files (*.pdf);;All (*.*)"));
  if (s.isNull())
    return;
  fname = s;
  saveRasterImage(fname);
#else
  saveRasterImage("/tmp/map.png");
  saveRasterImage("/tmp/map.svg");
  saveRasterImage("/tmp/map.pdf");
#endif
}

void DianaMainWindow::saveRasterImage(const QString& filename)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename.toStdString()));
  QPrinter* printer = 0;
  QImage* image = 0;
  std::auto_ptr<QPaintDevice> device;
  bool printing = false;
  if (filename.endsWith(".pdf")) {
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setOutputFileName(filename);
    printer->setFullPage(true);
    printer->setPaperSize(QSizeF(w->width(), w->height()), QPrinter::DevicePixel);

    // FIXME copy from bdiana
    // According to QTBUG-23868, orientation and custom paper sizes do not
    // play well together. Always use portrait.
    printer->setOrientation(QPrinter::Portrait);

    printing = true;
    device.reset(printer);
  } else if (filename.endsWith(".svg")) {
    QSvgGenerator* generator = new QSvgGenerator();
    generator->setFileName(filename);
    generator->setSize(w->size());
    generator->setViewBox(QRect(0, 0, w->width(), w->height()));
    generator->setTitle(tr("diana image"));
    generator->setDescription(tr("Created by diana %1.").arg(PVERSION));

    // FIXME copy from bdiana
    // For some reason, QPrinter can determine the correct resolution to use, but
    // QSvgGenerator cannot manage that on its own, so we take the resolution from
    // a QPrinter instance which we do not otherwise use.
    QPrinter sprinter;
    generator->setResolution(sprinter.resolution());

    printing = true;
    device.reset(generator);
  } else {
    image = new QImage(w->size(), QImage::Format_ARGB32_Premultiplied);
    image->fill(Qt::transparent);
    device.reset(image);
  }

  paintOnDevice(device.get(), printing);

  if (image)
    image->save(filename);
}

void DianaMainWindow::paintOnDevice(QPaintDevice* device, bool printing)
{
  METLIBS_LOG_SCOPE();
  DiCanvas* oldCanvas = contr->canvas(); // TODO obtain from w->Glw() or so

  std::auto_ptr<DiPaintGLCanvas> glcanvas(new DiPaintGLCanvas(device));
  glcanvas->parseFontSetup();
  glcanvas->setPrinting(printing);
  std::auto_ptr<DiPaintGLPainter> glpainter(new DiPaintGLPainter(glcanvas.get()));
  glpainter->printing = (dynamic_cast<QPrinter*>(device) != 0);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);

  const int ww = w->Glw()->width(), wh = w->Glw()->height(), dw = device->width(), dh = device->height();
  METLIBS_LOG_DEBUG(LOGVAL(ww) << LOGVAL(wh) << LOGVAL(dw) << LOGVAL(dh));

  QPainter painter;
  painter.begin(device);

  w->Glw()->setCanvas(glcanvas.get());
#if 1
  glpainter->Viewport(0, 0, dw, dh);
  w->Glw()->resize(dw, dh);
#else
  painter.setWindow(0, 0, ww, wh);
  glpainter->Viewport(0, 0, ww, wh);
  w->Glw()->resize(ww, wh);
#endif

  glpainter->begin(&painter);
  w->Glw()->paintUnderlay(glpainter.get());
  w->Glw()->paintOverlay(glpainter.get());
  glpainter->end();
  painter.end();

  w->Glw()->setCanvas(oldCanvas);
  w->Glw()->resize(ww, wh);
}

void DianaMainWindow::emailPicture()
{
  mailm->show();
}


#ifdef VIDEO_EXPORT
void DianaMainWindow::saveAnimation()
{
  static QString fname = "./"; // keep users preferred animation-path for later

  QString s = QFileDialog::getSaveFileName(this,
      tr("Save animation from current fields, satellite images, etc. (*.mpg or *.avi)"),
      fname,
      tr("Movies (*.mpg *.avi);;All (*.*)"));

  if (s.isNull())
    return;

  const QString suffix = QFileInfo(s).suffix();
  std::string format = "mpg";
  if (!suffix.compare(QLatin1String("avi"), Qt::CaseInsensitive)) {
    format = "avi";
  } else if (suffix.compare(QLatin1String("mpg"), Qt::CaseInsensitive)) {
    s += ".mpg";
  }
  fname = s;

  std::string filename = s.toStdString();

  float delay = timeout_ms * 0.001;
  MovieMaker moviemaker(filename, format, delay);

  QMessageBox::information(this, tr("Making animation"),
      tr("This may take some time, depending on the number of timesteps and selected delay."
          " Diana cannot be used until this process is completed."
          " A message will be displayed upon completion."
          " Press OK to begin."));

  const int ww = w->width(), wh = w->height();
  w->setVisible(false); // avoid handling repaint events while the canvas is replaced

  // first reset time-slider
  miutil::miTime startTime = tslider->getStartTime();
  tslider->set(startTime);
  setPlotTime(startTime);

  int nrOfTimesteps = tslider->numTimes();

  int maxProgress = nrOfTimesteps - tslider->current() - 1;
  QProgressDialog progress(tr("Creating animation..."), tr("Hide"),
      0, maxProgress, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.show();

  QImage image(moviemaker.frameSize(), QImage::Format_ARGB32_Premultiplied);

  // ==================== copy> ====================
  // FIXME this is a partial copy of paintOnDevice (but does not
  // create a new DiPaintGLCanvas for each time step)
  DiCanvas* oldCanvas = contr->canvas(); // TODO obtain from w->Glw() or so

  std::auto_ptr<DiPaintGLCanvas> glcanvas(new DiPaintGLCanvas(&image));
  glcanvas->parseFontSetup();
  std::auto_ptr<DiPaintGLPainter> glpainter(new DiPaintGLPainter(glcanvas.get()));
  glpainter->printing = false;
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);

  w->Glw()->setCanvas(glcanvas.get());
  glpainter->Viewport(0, 0, image.width(), image.height());
  w->Glw()->resize(image.width(), image.height());

  QPainter painter;
  // ==================== <copy ====================

  /// save frames as images
  for (int step = 0; tslider->current() < nrOfTimesteps-1; ++step, stepforward()) {
    if (!progress.isHidden())
      progress.setValue(step);

    // ==================== <copy ====================
    image.fill(Qt::transparent);
    painter.begin(&image);
    glpainter->begin(&painter);
    w->Glw()->paintUnderlay(glpainter.get());
    w->Glw()->paintOverlay(glpainter.get());
    glpainter->end();
    painter.end();
    // ==================== <copy ====================

    moviemaker.addImage(image);
  }
  if (!progress.isHidden())
    progress.setValue(nrOfTimesteps);

  // ==================== <copy ====================
  w->Glw()->setCanvas(oldCanvas);
  w->Glw()->resize(ww, wh);
  // ==================== <copy ====================
  w->setVisible(true);

  QMessageBox::information(this, tr("Done"), tr("Animation completed."));
}
#else
void DianaMainWindow::saveAnimation()
{
  QMessageBox::information(this, tr("Compiled without video export"),
      tr("Diana must be compiled with VIDEO_EXPORT defined to use this feature."));
}
#endif

void DianaMainWindow::parseSetup()
{
  METLIBS_LOG_SCOPE();
  SetupDialog *setupDialog = new SetupDialog(this);

  if( setupDialog->exec() ) {

    LocalSetupParser sp;
    std::string filename;
    if (!sp.parse(filename)){
      METLIBS_LOG_ERROR("An error occured while re-reading setup ");
    }
    if (DiCanvas* c = w->Glw()->canvas())
      c->parseFontSetup();
    contr->parseSetup();
    if (vcInterface.get())
      vcInterface->parseSetup();
    if (vpWindow)
      vpWindow->parseSetup();
    if (spWindow)
      spWindow->parseSetup();

    fm->updateModels();
    om->updateDialog();
  }
}

void DianaMainWindow::hardcopy()
{
  METLIBS_LOG_SCOPE();
  QPrinter printer;
  d_print::fromPrintOption(printer, priop);
  QPrintDialog printerDialog(&printer, this);
  if (printerDialog.exec() != QDialog::Accepted || !printer.isValid())
    return;

  diutil::OverrideCursor waitCursor;
  paintOnDevice(&printer, true);
  d_print::toPrintOption(printer, priop);
}

void DianaMainWindow::previewHardcopy()
{
  QPrinter qprt;
  d_print::fromPrintOption(qprt, priop);
  QPrintPreviewDialog previewDialog(&qprt, this);
  connect(&previewDialog, SIGNAL(paintRequested(QPrinter*)),
      this, SLOT(paintOnPrinter(QPrinter*)));
  if (previewDialog.exec() == QDialog::Accepted)
    d_print::toPrintOption(qprt, priop);
}

void DianaMainWindow::paintOnPrinter(QPrinter* printer)
{
  paintOnDevice(printer, true);
}


// left mouse click -> mark trajectory position
void DianaMainWindow::trajPositions(bool b)
{
  markTrajPos = b;
  markMeasurementsPos = false;
}

void DianaMainWindow::measurementsPositions(bool b)
{
  markMeasurementsPos = b;
  markTrajPos = false;
}

// picks up a single click on position x,y
void DianaMainWindow::catchMouseGridPos(QMouseEvent* mev)
{
  int x = mev->x();
  int y = mev->y();

  float lat=0,lon=0;
  contr->PhysToGeo(x,y,lat,lon);

  if(markTrajPos){
    trajm->mapPos(lat,lon);
    requestBackgroundBufferUpdate();
  }

  if(markMeasurementsPos) {
    measurementsm->mapPos(lat,lon);
    requestBackgroundBufferUpdate();
  }

  if( !optAutoElementAction->isChecked() ){
    catchElement(mev);
  }

  if (mev->modifiers() & Qt::ControlModifier){
    if (uffda && contr->getSatnames().size()){
      showUffda();
    }
  }
  //send position to all clients
  if(qsocket){
    std::string latstr = miutil::from_number(lat,6);
    std::string lonstr = miutil::from_number(lon,6);
    miMessage letter;
    letter.command     = qmstrings::positions;
    letter.commondesc  =  "dataset";
    letter.common      =  "diana";
    letter.description =  "lat:lon";
    letter.to = qmstrings::all;
    letter.data.push_back(std::string(latstr + ":" + lonstr));
    sendLetter(letter);
  }

  QString stationsText = contr->getStationManager()->getStationsText(mev->x(), mev->y());
  if ( !stationsText.isEmpty() ) {
    QWhatsThis::showText(w->mapToGlobal(QPoint(mev->x(), w->height() - mev->y())), stationsText, w);
  }
  std::string obsText= contr->getObsPopupText(mev->x(), mev->y());
  QString obsPopupData = QString::fromStdString(obsText);
  if ( !obsPopupData.isEmpty() ) {
         QWhatsThis::showText(w->mapToGlobal(QPoint(mev->x(), w->height() - mev->y())), obsPopupData, w);
  }
}


// picks up a single click on position x,y
void DianaMainWindow::catchMouseRightPos(QMouseEvent* mev)
{
  METLIBS_LOG_SCOPE();

  int x = mev->x();
  int y = mev->y();
  int globalX = mev->globalX();
  int globalY = mev->globalY();


  float map_x,map_y;
  contr->PhysToMap(mev->x(),mev->y(),map_x,map_y);

  xclick=x; yclick=y;

  for (int i=0; i<MaxSelectedAreas; i++){
    selectAreaAction[i]->setVisible(false);
  }

  vselectAreas=contr->findAreas(xclick,yclick);
  int nAreas=vselectAreas.size();
  if ( nAreas>0 ) {
    zoomOutAction->setVisible(true);
    for (int i=1; i<=nAreas && i<MaxSelectedAreas; i++){
      selectAreaAction[i]->setText(vselectAreas[i-1].name.c_str());
      selectAreaAction[i]->setData(i-1);
      selectAreaAction[i]->setVisible(true);
    }
    rightclickmenu->popup(QPoint(globalX, globalY), 0);
  } else {
    zoomOut();
  }
}


// picks up mousemovements (without buttonclicks)
void DianaMainWindow::catchMouseMovePos(QMouseEvent* mev, bool quick)
{
  // METLIBS_LOG_SCOPE(LOGVAL(mev->x()) << LOGVAL(mev->y()));
  int x = mev->x();
  int y = mev->y();

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
  } else if (sgeopos->areaMode()){ //Show area in km2
    double markedArea= contr->getMarkedArea(x, y);
    double windowArea= contr->getWindowArea();
    sgeopos->setPosition(markedArea/(1000.0*1000.0),windowArea/(1000.0*1000.0));
  } else if (sgeopos->distMode()){ //Show distance in km
    double horizDist = contr->getWindowDistances(x, y, true);
    double vertDist = contr->getWindowDistances(x, y, false);
    sgeopos->setPosition(vertDist/1000.0,horizDist/1000.0);
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
    requestBackgroundBufferUpdate();

  if (quick)
    return;

  if (optAutoElementAction->isChecked()) {
    catchElement(mev);
  }
}


void DianaMainWindow::catchMouseDoubleClick(QMouseEvent* mev)
{
}


void DianaMainWindow::catchElement(QMouseEvent* mev)
{
  METLIBS_LOG_SCOPE(LOGVAL(mev->x()) << LOGVAL(mev->y()));

  int x = mev->x();
  int y = mev->y();

  bool needupdate= false; // updateGL necessary

  std::string uffstation = contr->findStation(x,y,"uffda");
  if (!uffstation.empty())
    uffm->pointClicked(uffstation);

  //show closest observation
  if (contr->findObs(x,y)) {
    needupdate= true;
  }

  //find the name of stations clicked/pointed at
  vector<std::string> stations = contr->findStations(x,y,"vprof");
  //now tell vpWindow about new station (this calls vpManager)
  if (vpWindow && !stations.empty())
    vpWindow->changeStations(stations);

  //find the name of station we clicked/pointed
  //at (from plotModul->stationPlot)
  std::string station = contr->findStation(x,y,"spectrum");
  //now tell spWindow about new station (this calls spManager)
  if (spWindow && !station.empty()) {
    spWindow->changeStation(station);
    //  needupdate= true;
  }

  // locationPlots (vcross,...)
  std::string crossection= contr->findLocation(x, y, LOCATIONS_VCROSS);
  if (vcInterface.get() && !crossection.empty()) {
    vcInterface->changeCrossection(crossection);
    //  needupdate= true;
  }

  if(qsocket){

    //set selected and send position to plugin connected
    vector<int> id;
    vector<std::string> name;
    vector<std::string> station;

    bool add = false;
    //    if(mev->modifiers() & Qt::ShiftModifier) add = true; //todo: shift already used (skip editmode)
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
        std::string datastr = areas[i].name + ":on";
        letter.data.push_back(datastr);
        sendLetter(letter);
      }
    }

    if(hqcTo>0){
      std::string name;
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

  if (needupdate)
    requestBackgroundBufferUpdate();
}

void DianaMainWindow::sendSelectedStations(const std::string& command)
{
  vector<std::string> data;
  contr->getStationData(data);
  int n=data.size();
  for(int i=0;i<n;i++){
    vector<string> token;
    boost::algorithm::split(token, data[i], boost::algorithm::is_any_of(":"));
    int m = token.size();
    if(token.size()<2) continue;
    int id=atoi(token[m-1].c_str());
    std::string dataset = token[m-2];
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


void DianaMainWindow::catchKeyPress(QKeyEvent* ke)
{
}

void DianaMainWindow::undo()
{
  if(em->inedit())
    em->undoEdit();
  else {
    EditItemManager *editm = EditItemManager::instance();
    if (editm->isEditing())
      editm->undo();
  }
}

void DianaMainWindow::redo()
{
  if(em->inedit())
    em->redoEdit();
  else {
    EditItemManager *editm = EditItemManager::instance();
    if (editm->isEditing())
      editm->redo();
  }
}

void DianaMainWindow::save()
{
  if(em->inedit())
    em->saveEdit();
  else {
    EditItemManager *editm = EditItemManager::instance();
    if (editm->isEditing())
      editm->save();
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
  statusBar()->setVisible(not statusBar()->isVisible());
}

void DianaMainWindow::filequit()
{
  if (em->cleanupForExit() && uffm->okToExit()){
    // quit sends aboutToQuit SIGNAL, which is connected to slot writeLogFile
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

// static
bool DianaMainWindow::allowedInstanceName(const QString& text)
{
  return ClientSelection::isAllowedClientName("diana", text);
}

QString DianaMainWindow::instanceName() const
{
  return pluginB->getClientName();
}

QString DianaMainWindow::instanceNameSuffix() const
{
  return pluginB->getClientNameSuffix();
}

void DianaMainWindow::setInstanceName(QString name)
{
  if (!allowedInstanceName(name))
    name = "diana";

  const QString title = QString("diana " PVERSION " (%1)").arg(name);
  setWindowTitle(title);

  pluginB->setClientName(name);

  Q_EMIT instanceNameChanged(name);
}

// static
std::string DianaMainWindow::getLogFileDir()
{
  return LocalSetupParser::basicValue("homedir") + "/";
}

// static
std::string DianaMainWindow::getLogFileExt()
{
  return ".log";
}

std::string DianaMainWindow::getLogFileName() const
{
  // FIXME toStdString uses latin1 which might cause encoding problems
  return getLogFileDir() + instanceName().toStdString() + getLogFileExt();
}

void DianaMainWindow::writeLogFile()
{
  // write the system log file to $HOME/diana/.diana.log

  const std::string logfilepath = getLogFileName();
  std::ofstream file(logfilepath.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) " << logfilepath);
    return;
  }

  LogFileIO logfile;
  logfile.getSection("MAIN.LOG").addLines(writeLog(VERSION, diana_build_string));
  logfile.getSection("CONTROLLER.LOG").addLines(contr->writeLog());
  logfile.getSection("MAP.LOG").addLines(mm->writeLog());
  logfile.getSection("FIELD.LOG").addLines( fm->writeLog());
  logfile.getSection("OBS.LOG").addLines(om->writeLog());
  logfile.getSection("SAT.LOG").addLines(sm->writeLog());
  logfile.getSection("QUICK.LOG").addLines(qm->writeLog());
  logfile.getSection("TRAJ.LOG").addLines(trajm->writeLog());

  if (vpWindow) {
    logfile.getSection("VPROF.WINDOW.LOG").addLines(vpWindow->writeLog("window"));
    logfile.getSection("VPROF.SETUP.LOG").addLines( vpWindow->writeLog("setup"));
  }

  if (vcInterface.get())
    vcInterface->writeLog(logfile);

  if (spWindow) {
    logfile.getSection("SPECTRUM.WINDOW.LOG").addLines(spWindow->writeLog("window"));
    logfile.getSection("SPECTRUM.SETUP.LOG").addLines(spWindow->writeLog("setup"));
  }

  logfile.write(file);
  file.close();
  METLIBS_LOG_INFO("Finished writing " << logfilepath);
}


void DianaMainWindow::readLogFile()
{
  getDisplaySize();

  const std::string logfilepath = getLogFileName();
  std::string logVersion;

  std::ifstream file(logfilepath.c_str());
  if (!file) {
    //    METLIBS_LOG_DEBUG("Can't open " << logfilepath);
    return;
  }

  LogFileIO logfile;
  logfile.read(file);

  METLIBS_LOG_INFO("READ " << logfilepath);

  const std::string version_string(VERSION);
  readLog(logfile.getSection("MAIN.LOG").lines(), version_string, logVersion);
  contr->readLog(logfile.getSection("CONTROLLER.LOG").lines(), version_string, logVersion);
  mm->readLog(logfile.getSection("MAP.LOG").lines(), version_string, logVersion);
  fm->readLog(logfile.getSection("FIELD.LOG").lines(), version_string, logVersion);
  om->readLog(logfile.getSection("OBS.LOG").lines(), version_string, logVersion);
  sm->readLog(logfile.getSection("SAT.LOG").lines(), version_string, logVersion);
  trajm->readLog(logfile.getSection("TRAJ.LOG").lines(), version_string, logVersion);
  qm->readLog(logfile.getSection("QUICK.LOG").lines(), version_string, logVersion);
  if (vpWindow) {
    vpWindow->readLog("window", logfile.getSection("VPROF.WINDOW.LOG").lines(), version_string, logVersion, displayWidth,displayHeight);
    vpWindow->readLog("setup", logfile.getSection("VPROF.SETUP.LOG").lines(), version_string, logVersion, displayWidth,displayHeight);
  }
  if (vcInterface.get())
    vcInterface->readLog(logfile, version_string, logVersion, displayWidth,displayHeight);
  if (spWindow) {
    spWindow->readLog("window", logfile.getSection("SPECTRUM.WINDOW.LOG").lines(), version_string, logVersion, displayWidth,displayHeight);
    spWindow->readLog("setup", logfile.getSection("SPECTRUM.SETUP.LOG").lines(), version_string, logVersion, displayWidth,displayHeight);
  }

  file.close();
  METLIBS_LOG_INFO("Finished reading " << logfilepath);
}

namespace {
std::string saveXY(int x, int y)
{
  return miutil::from_number(x) + " " + miutil::from_number(y);
}

void saveDialogSize(std::vector<std::string>& vstr, const std::string& name, ShowMoreDialog* d)
{
  vstr.push_back(name + ".size " + saveXY(d->width(), d->height())
      + " " + (d->showsMore() ? "YES" : "NO"));
}

void saveDialogSize(std::vector<std::string>& vstr, const std::string& name, QWidget* d)
{
  vstr.push_back(name + ".size " + saveXY(d->width(), d->height()));
}

void saveDialogPos(std::vector<std::string>& vstr, const std::string& name, QWidget* widget)
{
  vstr.push_back(name + ".pos " + saveXY(widget->x(), widget->y()));
}

void restoreDialogSize(ShowMoreDialog* d, bool showmore, int w, int h)
{
  d->showMore(showmore);
  d->resize(w, h);
}

void restoreDialogSize(QWidget* d, bool /*showmore*/, int w, int h)
{
  d->resize(w, h);
}

void restoreDialogPos(QWidget* d, int x, int y)
{
  d->move(x, y);
}

} // anonymous namespace

vector<string> DianaMainWindow::writeLog(const string& thisVersion, const string& thisBuild)
{
  std::vector<std::string> vstr;
  std::string str;

  // version & time
  str= "VERSION " + thisVersion;
  vstr.push_back(str);
  str= "BUILD " + thisBuild;
  vstr.push_back(str);
  str= "LOGTIME " + miutil::miTime::nowTime().isoTime();
  vstr.push_back(str);
  vstr.push_back("================");

  // dialog positions
  saveDialogSize(vstr, "MainWindow", this);
  saveDialogPos(vstr, "MainWindow", this);
  saveDialogPos(vstr, "QuickMenu", qm);
  saveDialogPos(vstr, "FieldDialog", fm);
  saveDialogSize(vstr, "FieldDialog", fm);
  saveDialogPos(vstr, "ObsDialog", om);
  saveDialogPos(vstr, "SatDialog", sm);
  saveDialogPos(vstr, "StationDialog", stm);
  saveDialogPos(vstr, "MapDialog", mm);
  saveDialogPos(vstr, "EditDialog", em);
  saveDialogPos(vstr, "ObjectDialog", objm);
  saveDialogPos(vstr, "TrajectoryDialog", trajm);
  saveDialogSize(vstr, "Textview", textview);
  saveDialogPos(vstr, "Textview.pos", textview);

  map<QAction*, DataDialog*>::iterator it;
  for (it = dialogs.begin(); it != dialogs.end(); ++it) {
    DataDialog* d = it->second;
    saveDialogPos(vstr, d->name(), d);
    saveDialogSize(vstr, d->name(), d);
  }

  str="DocState " + saveDocState();
  vstr.push_back(str);
  {
    std::ostringstream p;
    p << "coserver_peers";
    const QStringList peers = pluginB->getSelectedClientNames();
    for (int i=0; i<peers.count(); ++i)
      p << ' ' << peers.at(i).toStdString();
    vstr.push_back(p.str());
  }
  vstr.push_back("================");

  // printer name & options...
  if (not priop.printer.empty()){
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
  str= "STATUSBUTTONS " + std::string(showelem ? "ON" : "OFF");
  vstr.push_back(str);
  //vstr.push_back("================");

  // Automatic element selection
  autoselect= optAutoElementAction->isChecked();
  str= "AUTOSELECT " + std::string(autoselect ? "ON" : "OFF");
  vstr.push_back(str);

  // scrollwheelzooming
  bool scrollwheelzoom = optScrollwheelZoomAction->isChecked();
  str = "SCROLLWHEELZOOM " + std::string(scrollwheelzoom ? "ON" : "OFF");
  vstr.push_back(str);

  // GUI-font
  str= "FONT " + std::string(qApp->font().toString().toStdString());
  vstr.push_back(str);
  //vstr.push_back("================");

  return vstr;
}

std::string DianaMainWindow::saveDocState()
{
  QByteArray state = saveState();
  ostringstream ost;
  int n= state.count();
  for (int i=0; i<n; i++)
    ost << setw(7) << int(state[i]);
  return ost.str();
}

void DianaMainWindow::readLog(const vector<string>& vstr, const string& thisVersion, string& logVersion)
{
  vector<string> tokens;
  int x,y;

  int nvstr= vstr.size();
  int ivstr= 0;

  logVersion.clear();

  //.....version & time .........
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    tokens= miutil::split(vstr[ivstr], 0, " ");
    if (tokens[0]=="VERSION" && tokens.size()==2) logVersion= tokens[1];
    //if (tokens[0]=="LOGTIME" && tokens.size()==3) .................
  }
  ivstr++;

  // dialog positions
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    tokens= miutil::split(vstr[ivstr], 0, " ");
    if (tokens[0]=="DocState") restoreDocState(vstr[ivstr]);
    else if (tokens[0]=="coserver_peers") {
      QStringList peers;
      for (size_t i=1; i<tokens.size(); ++i)
        peers << QString::fromStdString(tokens[i]);
      pluginB->setSelectedClientNames(peers);
    }

    if (tokens.size()==3 || tokens.size() == 4) {
      x= atoi(tokens[1].c_str());
      y= atoi(tokens[2].c_str());
      const bool showmore = (tokens.size() == 4 && tokens[3] == "YES");
      if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
        if (tokens[0]=="MainWindow.size")  this->resize(x,y);
      }
      if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
        if      (tokens[0]=="MainWindow.pos")    restoreDialogPos(this, x, y);
        else if (tokens[0]=="QuickMenu.pos")     restoreDialogPos(qm, x, y);
        else if (tokens[0]=="FieldDialog.pos")   restoreDialogPos(fm, x, y);
        else if (tokens[0]=="FieldDialog.size")  restoreDialogSize(fm, showmore, x, y);
        else if (tokens[0]=="ObsDialog.pos")     restoreDialogPos(om, x, y);
        else if (tokens[0]=="SatDialog.pos")     restoreDialogPos(sm, x, y);
        else if (tokens[0]=="MapDialog.pos")     restoreDialogPos(mm, x, y);
        else if (tokens[0]=="StationDialog.pos") restoreDialogPos(stm, x, y);
        else if (tokens[0]=="EditDialog.pos")    restoreDialogPos(em, x, y);
        else if (tokens[0]=="ObjectDialog.pos")  restoreDialogPos(objm, x, y);
        else if (tokens[0]=="TrajectoryDialog.pos") restoreDialogPos(trajm, x, y);
        else if (tokens[0]=="Textview.size")     restoreDialogSize(textview, showmore, x, y);
        else if (tokens[0]=="Textview.pos")      restoreDialogPos(textview, x, y);
        else {
          map<QAction*, DataDialog*>::iterator it;
          for (it = dialogs.begin(); it != dialogs.end(); ++it) {
            if (tokens[0] == it->second->name() + ".pos") {
              restoreDialogPos(it->second, x, y);
              break;
            } else if (tokens[0] == it->second->name() + ".size") {
              restoreDialogSize(it->second, showmore, x, y);
              break;
            }
          }
        }
      }
    }
  }
  ivstr++;

  // printer name & other options...
  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") continue;
    tokens= miutil::split(vstr[ivstr], 0, " ");
    if (tokens.size()>=2) {
      if (tokens[0]=="PRINTER") {
        priop.printer=tokens[1];
      } else if (tokens[0]=="PRINTORIENTATION") {
        if (tokens[1]=="portrait")
          priop.orientation=d_print::ori_portrait;
        else
          priop.orientation=d_print::ori_landscape;
      } else if (tokens[0]=="AUTOSELECT") {
        autoselect = (tokens[1] == "ON");
      } else if (tokens[0] == "SCROLLWHEELZOOM") {
        if (tokens[1] == "ON") {
          optScrollwheelZoomAction->setChecked(true);
          toggleScrollwheelZoom();
        }
      } else if (tokens[0] == "FONT") {
        std::string fontstr = tokens[1];
        //LB:if the font name contains blanks,
        //the string will be cut in pieces, and must be put together again.
        for(unsigned int i=2;i<tokens.size();i++)
          fontstr += " " + tokens[i];
        QFont font;
        if (font.fromString(fontstr.c_str()))
          qApp->setFont(font);
      }
    }
  }

  if (logVersion.empty()) logVersion= "0.0.0";

  if (logVersion!=thisVersion)
    METLIBS_LOG_INFO("log from version " << logVersion);
}

void DianaMainWindow::restoreDocState(std::string logstr)
{
  vector<std::string> vs= miutil::split(logstr, " ");
  int n=vs.size();
  QByteArray state(n-1,' ');
  for (int i=1; i<n; i++){
    state[i-1]= char(atoi(vs[i].c_str()));
  }

  if (!restoreState( state)){
    METLIBS_LOG_ERROR("!!!restoreState failed");
  }
}

void DianaMainWindow::getDisplaySize()
{
  displayWidth=  QApplication::desktop()->width();
  displayHeight= QApplication::desktop()->height();

  METLIBS_LOG_DEBUG("display width,height: "<<displayWidth<<" "<<displayHeight);
}


void DianaMainWindow::checkNews()
{
  std::string newsfile= LocalSetupParser::basicValue("homedir") + "/diana.news";
  std::string thisVersion= "yy";
  std::string newsVersion= "xx";

  // check modification time on news file
  std::string filename= LocalSetupParser::basicValue("docpath") + "/" + "news.html";
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
  if (QString::fromStdString(pe.type) == DrawingManager::instance()->plotElementTag())
    DrawingManager::instance()->enablePlotElement(pe);
  else if (QString::fromStdString(pe.type) == EditItemManager::instance()->plotElementTag())
    EditItemManager::instance()->enablePlotElement(pe);
  else
    contr->enablePlotElement(pe);

  vector<string> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);
  requestBackgroundBufferUpdate();
}

void DianaMainWindow::showElements()
{
  if ( showelem ) {
    statusbuttons->reset();
    statusbuttons->hide();
    optOnOffAction->setChecked( false );
    showelem= false;
  } else {
    updatePlotElements();
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
  QFont font = QFontDialog::getFont(&ok,qApp->font(), this);
  if (ok) {
    // font is set to the font the user selected
    qApp->setFont(font);
  } else {
    // the user cancelled the dialog; font is set to the initial
    // value: do nothing
  }
}

void DianaMainWindow::zoomTo(Rectangle r)
{
  if (contr)
    contr->zoomTo(r);
}

void DianaMainWindow::zoomOut()
{
  contr->zoomOut();
  requestBackgroundBufferUpdate();
}

void DianaMainWindow::showUffda()
{
  if (uffda && contr->getSatnames().size()) {
    if (xclick==xlast && yclick==ylast) {
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
    requestBackgroundBufferUpdate();
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

  std::string areaName=vselectAreas[ia].name;
  bool selected=vselectAreas[ia].selected;
  int id=vselectAreas[ia].id;
  std::string misc=(selected) ? "off" : "on";
  std::string datastr = areaName + ":" + misc;
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
    std::string str;
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

void DianaMainWindow::addDialog(DataDialog *dialog)
{
  dialogNames[dialog->name()] = dialog;
  connect(dialog, SIGNAL(applyData()), SLOT(MenuOK()));
  connect(dialog, SIGNAL(emitTimes(const std::string &, const std::vector<miutil::miTime> &)),
      tslider, SLOT(insert(const std::string &, const std::vector<miutil::miTime> &)));
  connect(dialog, SIGNAL(emitTimes(const std::string &, const std::vector<miutil::miTime> &, bool)),
      tslider, SLOT(insert(const std::string &, const std::vector<miutil::miTime> &, bool)));
  connect(dialog, SIGNAL(updated()), this, SLOT(requestBackgroundBufferUpdate()));

  if (QAction *action = dialog->action()) {
    dialogs[action] = dialog;
    connect(action, SIGNAL(toggled(bool)), dialog, SLOT(setVisible(bool)));
    connect(action, SIGNAL(toggled(bool)), this, SLOT(requestBackgroundBufferUpdate()));
    showmenu->addAction(action);
    mainToolbar->addAction(action);
  }
}

/**
 * Updates the dialog associated with the action or dialog that sent the
 * signal connected to this slot. We toggle the relevant action to hide the
 * dialog.
 */
void DianaMainWindow::updateDialog()
{
  QAction *action = qobject_cast<QAction *>(sender());
  DataDialog *dialog = qobject_cast<DataDialog *>(sender());

  if (dialog)
    action = dialog->action();
  else if (!action)
    return;

  action->toggle();
}

void DianaMainWindow::setWorkAreaCursor(const QCursor &cursor)
{
    w->setGlwCursor(cursor);
}

void DianaMainWindow::unsetWorkAreaCursor()
{
    w->unsetGlwCursor();
}

void DianaMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls()) {
    foreach (QUrl url, event->mimeData()->urls()) {

      // Return if we encounter a non-file URL, causing the event to be ignored.
      if (!(url.scheme() == "file"))
        return;

      // Continue if we encounter a NetCDF file, causing the event to be accepted
      // as long as any other files are acceptable.
      if (QFileInfo(url.toLocalFile()).suffix() == "nc")
        continue;

      // Continue if we encounter a SVG file, causing the event to be accepted as
      // long as any other files are acceptable.
      if (QFileInfo(url.toLocalFile()).suffix() == "svg")
        continue;

      // Continue if we encounter a KML file, causing the event to be accepted
      // as long as any other files are acceptable.
      if (QFileInfo(url.toLocalFile()).suffix() == "kml")
        continue;

      // Return if we encounter a file that we can't handle, causing the event to
      // be ignored.
      return;
    }

    event->accept();
  }
}

void DianaMainWindow::dropEvent(QDropEvent *event)
{
  // ### TODO: Open a dialog to configure the options for the FIELD_FILES section of the
  // user's setup file.
  QString filegroup = tr("imported files");

  std::vector<std::string> extra_field_lines;
  extra_field_lines.push_back("filegroup=\"" + filegroup.toStdString() + "\"");
  int fieldsAdded = 0;
  int symbolsAdded = 0;
  int drawingsAdded = 0;

  if (event->mimeData()->hasUrls()) {

    foreach (QUrl url, event->mimeData()->urls()) {

      if (url.scheme() == "file") {
        QString suffix = QFileInfo(url.toLocalFile()).suffix();

        if (suffix == "nc") {
          QFileInfo fi(url.toLocalFile());
          QString s = QString("m=%1 t=fimex f=%2 format=netcdf").arg(fi.baseName()).arg(url.toLocalFile());
          extra_field_lines.push_back(s.toStdString());
          fieldsAdded++;

        } else if (suffix == "svg") {
          QString fileName = url.toLocalFile();
          QString name = QFileInfo(fileName).fileName();
          QString section = QFileInfo(fileName).dir().dirName();
          DrawingManager::instance()->loadSymbol(fileName, section, name);
          editDrawingToolBar->addSymbol(section, name);
          symbolsAdded++;

        } else if (suffix == "kml") {
          QString fileName = url.toLocalFile();
          QString name = QFileInfo(fileName).fileName();
          EditItems::DrawingDialog::instance()->loadFile(fileName);
          drawingsAdded++;
        }
      }
    }
  }

  std::vector<std::string> field_errors;
  if (!contr->updateFieldFileSetup(extra_field_lines, field_errors)) {
    METLIBS_LOG_ERROR("ERROR, an error occurred while adding new fields:");
    for (unsigned int kk = 0; kk < field_errors.size(); ++kk)
      METLIBS_LOG_ERROR(field_errors[kk]);
  }

  if (fieldsAdded > 0) {
    fm->updateModels();
    statusBar()->showMessage(tr("Imported model data to the \"%1\" field group.").arg(filegroup), 2000);
  } else if (symbolsAdded > 0) {
    statusBar()->showMessage(tr("Imported %1 symbol(s).", "", symbolsAdded).arg(symbolsAdded), 2000);
  } else if (drawingsAdded > 0) {
    statusBar()->showMessage(tr("Imported %1 drawing(s).", "", drawingsAdded).arg(drawingsAdded), 2000);
  }
}

DianaMainWindow *DianaMainWindow::instance()
{
  return DianaMainWindow::self;
}
