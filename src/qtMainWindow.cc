/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diana_config.h"

#include "qtMainWindow.h"

#include "qtAddtoMenu.h"
#include "qtAnnotationDialog.h"
#include "qtBrowserBox.h"
#include "qtDataDialog.h"
#include "qtEditDialog.h"
#include "qtFieldDialog.h"
#include "qtFieldDialogData.h"
#include "qtImageGallery.h"
#include "qtMainUiEventHandler.h"
#include "qtMapDialog.h"
#include "qtMeasurementsDialog.h"
#include "qtObjectDialog.h"
#include "qtObsDialog.h"
#include "qtQuickMenu.h"
#include "qtSatDialog.h"
#include "qtSatDialogData.h"
#include "qtSetupDialog.h"
#include "qtShowSatValues.h"
#include "qtSpectrumWindow.h"
#include "qtStationDialog.h"
#include "qtStatusGeopos.h"
#include "qtStatusPlotButtons.h"
#include "qtTextDialog.h"
#include "qtTextView.h"
#include "qtTimeNavigator.h"
#include "qtTrajectoryDialog.h"
#include "qtUtility.h"
#include "qtWorkArea.h"

#include "diBuild.h"
#include "diController.h"
#include "diEditItemManager.h"
#include "diLabelPlotCommand.h"
#include "diLocalSetupParser.h"
#include "diLocationData.h"
#include "diLogFile.h"
#include "diMainPaintable.h"
#include "diPaintGLPainter.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "miSetupParser.h"

#include "util/misc_util.h"
#include "util/qstring_util.h"
#include "util/string_util.h"
#include "vcross_qt/qtVcrossInterface.h"
#include "vprof/qtVprofWindow.h"
#include "wmsclient/WebMapDialog.h"
#include "wmsclient/WebMapManager.h"

#include "export/DianaImageSource.h"
#include "export/PrinterDialog.h"
#include "export/qtExportImageDialog.h"

#include <qUtilities/qtHelpDialog.h>

#include <coserver/ClientSelection.h>
#include <coserver/miMessage.h>
#include <coserver/QLetterCommands.h>

#include <puDatatypes/miCoordinates.h>

#include <puTools/miStringFunctions.h>

#include <puCtools/stat.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileDialog>
#include <QFocusEvent>
#include <QFontDialog>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QShortcut>
#include <QStatusBar>
#include <QStringList>
#include <QToolBar>
#include <QToolButton>
#include <QToolTip>
#include <QUrl>
#include <QWhatsThis>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>

#include "EditItems/drawingdialog.h"
#include "EditItems/kml.h"
#include "EditItems/toolbar.h"

#define MILOGGER_CATEGORY "diana.MainWindow"
#include <qUtilities/miLoggingQt.h>

#include <Tool_32_draw.xpm>
#include <autoupdate_off.xpm>
#include <autoupdate_on.xpm>
#include <autoupdate_warn.xpm>
#include <balloon.xpm>
#include <diana_icon.xpm>
#include <drawing.xpm>
#include <earth3.xpm>
#include <editdrawing.xpm>
#include <felt.xpm>
#include <front.xpm>
#include <idnumDown.xpm>
#include <idnumUp.xpm>
#include <info.xpm>
#include <levelDown.xpm>
#include <levelUp.xpm>
#include <pick.xpm>
#include <ruler.xpm>
#include <sat.xpm>
#include <spectrum.xpm>
#include <station.xpm>
#include <synop.xpm>
#include <synop_red.xpm>
#include <thumbs_down.xpm>
#include <thumbs_up.xpm>
#include <traj.xpm>
#include <vcross.xpm>

#define DIANA_RESTORE_DIALOG_POSITIONS
//#define DISABLE_VPROF 1
//#define DISABLE_VCROSS 1
//#define DISABLE_WAVESPEC 1


DianaMainWindow *DianaMainWindow::self = 0;

DianaMainWindow::DianaMainWindow(Controller* co, const QString& instancename)
    : QMainWindow()
    , browsing(false)
    , markTrajPos(false)
    , markMeasurementsPos(false)
    , vpWindow(0)
    , vcrossEditManagerConnected(false)
    , spWindow(0)
    , exportImageDialog_(0)
    , pluginB(0)
    , contr(co)
    , showelem(true)
    , autoselect(false)
    , handlingTimeMessage(false)
{
  METLIBS_LOG_SCOPE();

  setWindowIcon(QIcon(QPixmap(diana_icon_xpm)));

  self = this;

  createHelpDialog();

  timeNavigator = new TimeNavigator(this);
  connect(timeNavigator, &TimeNavigator::timeSelected, this, &DianaMainWindow::setPlotTime);

  addStandardDialog(fm = new FieldDialog(this, new DianaFieldDialogData(contr->getFieldPlotManager())));
  addStandardDialog(om = new ObsDialog(this, contr));
  addStandardDialog(sm = new SatDialog(new DianaSatDialogData(contr->getSatelliteManager()), this));
  addStandardDialog(stm = new StationDialog(this, contr));
  addStandardDialog(objm = new ObjectDialog(this, contr));
  connect(timeNavigator, &TimeNavigator::lastStep, om, &ObsDialog::updateTimes);
  connect(timeNavigator, &TimeNavigator::lastStep, sm, &SatDialog::updateTimes);
  connect(timeNavigator, &TimeNavigator::lastStep, objm, &ObjectDialog::updateTimes);
  connect(timeNavigator, &TimeNavigator::lastStep, this, &DianaMainWindow::checkAutoUpdate);

  //-------- The Actions ---------------------------------

  // file ========================
  // --------------------------------------------------------------------
  QAction* fileSavePictAction = new QAction(tr("&Export image/movie..."), this);
  fileSavePictAction->setShortcut(tr("Ctrl+Shift+P"));
  connect( fileSavePictAction, SIGNAL( triggered() ) , SLOT( saveraster() ) );
  // --------------------------------------------------------------------
  QAction* filePrintAction = new QAction(tr("&Print..."), this);
  filePrintAction->setShortcut(Qt::CTRL+Qt::Key_P);
  connect( filePrintAction, SIGNAL( triggered() ) , SLOT( hardcopy() ) );
  // --------------------------------------------------------------------
  QAction* filePrintPreviewAction = new QAction(tr("Print pre&view..."), this);
  connect( filePrintPreviewAction, SIGNAL( triggered() ) , SLOT( previewHardcopy() ) );
  // --------------------------------------------------------------------
  QAction* readSetupAction = new QAction(tr("Read setupfile"), this);
  connect(readSetupAction, &QAction::triggered, this, &DianaMainWindow::parseSetup);
  // --------------------------------------------------------------------
  QAction* fileQuitAction = new QAction(tr("&Quit..."), this);
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
  QAction* showResetAreaAction = new QAction(QIcon(QPixmap(thumbs_up_xpm)), tr("Reset area and replot"), this);
  showResetAreaAction->setIconVisibleInMenu(true);
  connect( showResetAreaAction, SIGNAL( triggered() ) ,  SLOT( resetArea() ) );
  //----------------------------------------------------------------------
  QAction* showResetAllAction = new QAction(QIcon(QPixmap(thumbs_down_xpm)), tr("Reset all"), this);
  showResetAllAction->setIconVisibleInMenu(true);
  connect( showResetAllAction, SIGNAL( triggered() ) ,  SLOT( resetAll() ) );
  // --------------------------------------------------------------------
  QAction* showApplyAction = new QAction(tr("&Apply plot"), this);
  showApplyAction->setShortcut(Qt::CTRL+Qt::Key_U);
  connect( showApplyAction, SIGNAL( triggered() ) ,  SLOT( MenuOK() ) );
  // --------------------------------------------------------------------
  QAction* showAddQuickAction = new QAction(tr("Add to q&uickmenu"), this);
  showAddQuickAction->setShortcutContext(Qt::ApplicationShortcut);
  showAddQuickAction->setShortcut(Qt::Key_F9);
  connect( showAddQuickAction, SIGNAL( triggered() ) ,  SLOT( addToMenu() ) );
  // --------------------------------------------------------------------
  QAction* showPrevPlotAction = new QAction(tr("P&revious plot"), this);
  showPrevPlotAction->setShortcut(Qt::Key_F10);
  connect( showPrevPlotAction, SIGNAL( triggered() ) ,  SLOT( prevHPlot() ) );
  // --------------------------------------------------------------------
  QAction* showNextPlotAction = new QAction(tr("&Next plot"), this);
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
  showEditDialogAction = new QAction( QPixmap(editmode_xpm ),tr("&Product Editing"), this );
  showEditDialogAction->setShortcut(Qt::ALT+Qt::Key_E);
  showEditDialogAction->setCheckable(true);
  showEditDialogAction->setIconVisibleInMenu(true);
  connect( showEditDialogAction, SIGNAL( triggered() ) ,  SLOT( editMenu() ) );
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
  QAction* showProfilesDialogAction = new QAction(QIcon(QPixmap(balloon_xpm)), tr("&Vertical Profiles"), this);
  showProfilesDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_VPROF
  showProfilesDialogAction->setShortcut(Qt::ALT+Qt::Key_V);
  showProfilesDialogAction->setCheckable(false);
  connect( showProfilesDialogAction, SIGNAL( triggered() ) ,  SLOT( vprofMenu() ) );
#else
  showProfilesDialogAction->setEnabled(false);
#endif
  // --------------------------------------------------------------------
  QAction* showCrossSectionDialogAction = new QAction(QIcon(QPixmap(vcross_xpm)), tr("Vertical &Cross sections"), this);
  showCrossSectionDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_VCROSS
  showCrossSectionDialogAction->setShortcut(Qt::ALT+Qt::Key_C);
  showCrossSectionDialogAction->setCheckable(false);
  connect( showCrossSectionDialogAction, SIGNAL( triggered() ) ,  SLOT( vcrossMenu() ) );
#else
  showCrossSectionDialogAction->setEnabled(false);
#endif
  // --------------------------------------------------------------------
  QAction* showWaveSpectrumDialogAction = new QAction(QIcon(QPixmap(spectrum_xpm)), tr("&Wave spectra"), this);
  showWaveSpectrumDialogAction->setIconVisibleInMenu(true);
#ifndef DISABLE_WAVESPEC
  showWaveSpectrumDialogAction->setShortcut(Qt::ALT+Qt::Key_W);
  showWaveSpectrumDialogAction->setCheckable(false);
  connect( showWaveSpectrumDialogAction, SIGNAL( triggered() ) ,  SLOT( spectrumMenu() ) );
#else
  showWaveSpectrumDialogAction->setEnabled(false);
#endif

  // --------------------------------------------------------------------
  QAction* zoomOutAction = new QAction(tr("Zoom out"), this);
  zoomOutAction->setVisible(false);
  connect( zoomOutAction, SIGNAL( triggered() ), SLOT( zoomOut() ) );
  // --------------------------------------------------------------------
  showMeasurementsDialogAction = new QAction( QPixmap( ruler),tr("&Measurements"), this );
  showMeasurementsDialogAction->setShortcut(Qt::ALT+Qt::Key_M);
  showMeasurementsDialogAction->setCheckable(true);
  connect( showMeasurementsDialogAction, SIGNAL( triggered() ) ,  SLOT( measurementsMenu() ) );
  // ----------------------------------------------------------------
  QAction* toggleEditDrawingModeAction = new QAction(tr("Edit Drawing Mode"), this);
  toggleEditDrawingModeAction->setShortcut(Qt::CTRL+Qt::Key_B);
  connect(toggleEditDrawingModeAction, SIGNAL(triggered()), SLOT(toggleEditDrawingMode()));

  // help ======================
  // --------------------------------------------------------------------
  QAction* helpDocAction = new QAction(tr("Documentation"), this);
  helpDocAction->setShortcutContext(Qt::ApplicationShortcut);
  helpDocAction->setShortcut(Qt::Key_F1);
  helpDocAction->setCheckable(false);
  connect( helpDocAction, SIGNAL( triggered() ) ,  SLOT( showHelp() ) );
  // --------------------------------------------------------------------
  QAction* helpAccelAction = new QAction(tr("&Accelerators"), this);
  helpAccelAction->setCheckable(false);
  connect( helpAccelAction, SIGNAL( triggered() ) ,  SLOT( showAccels() ) );
  // --------------------------------------------------------------------
  QAction* helpNewsAction = new QAction(tr("&News"), this);
  helpNewsAction->setCheckable(false);
  connect( helpNewsAction, SIGNAL( triggered() ) ,  SLOT( showNews() ) );
  // --------------------------------------------------------------------
  QAction* helpTestAction = new QAction(tr("Test &results"), this);
  helpTestAction->setCheckable(false);
  connect( helpTestAction, SIGNAL( triggered() ) ,  SLOT( showUrl() ) );
  // --------------------------------------------------------------------
  QAction* helpAboutAction = new QAction(tr("About Diana"), this);
  helpAboutAction->setCheckable(false);
  connect( helpAboutAction, SIGNAL( triggered() ) ,  SLOT( about() ) );
  // --------------------------------------------------------------------


  // other tools ======================
  // --------------------------------------------------------------------
  toolLevelUpAction = new QAction(QIcon( QPixmap(levelUp_xpm )),tr("Vertical level up"), this );
  toolLevelUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelUpAction->setShortcut(Qt::CTRL+Qt::Key_PageUp);
  toolLevelUpAction->setCheckable(false);
  toolLevelUpAction->setEnabled ( false );
  toolLevelUpAction->setIconVisibleInMenu(true);
  connect( toolLevelUpAction, SIGNAL( triggered() ) ,  SLOT( levelUp() ) );
  // --------------------------------------------------------------------
  toolLevelDownAction = new QAction(QIcon( QPixmap(levelDown_xpm )),tr("Vertical level down"), this );
  toolLevelDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolLevelDownAction->setShortcut(Qt::CTRL+Qt::Key_PageDown);
  toolLevelDownAction->setCheckable(false);
  toolLevelDownAction->setEnabled ( false );
  toolLevelDownAction->setIconVisibleInMenu(true);
  connect( toolLevelDownAction, SIGNAL( triggered() ) ,  SLOT( levelDown() ) );
  // --------------------------------------------------------------------
  toolIdnumUpAction = new QAction(QIcon( QPixmap(idnumUp_xpm )),
      tr("Extra level up (EPS member, probability, …)"), this );
  toolIdnumUpAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumUpAction->setShortcut(Qt::SHIFT+Qt::Key_PageUp);
  toolIdnumUpAction->setCheckable(false);
  toolIdnumUpAction->setEnabled ( false );
  toolIdnumUpAction->setIconVisibleInMenu(true);
  connect( toolIdnumUpAction, SIGNAL( triggered() ) ,  SLOT( idnumUp() ) );
  // --------------------------------------------------------------------
  toolIdnumDownAction = new QAction(QIcon( QPixmap(idnumDown_xpm )),
      tr("Extra level down (EPS member, probability, …)"), this );
  toolIdnumDownAction->setShortcutContext(Qt::ApplicationShortcut);
  toolIdnumDownAction->setShortcut(Qt::SHIFT+Qt::Key_PageDown);
  toolIdnumDownAction->setEnabled ( false );
  toolIdnumDownAction->setIconVisibleInMenu(true);
  connect( toolIdnumDownAction, SIGNAL( triggered() ) ,  SLOT( idnumDown() ) );

  // Status ===============================
  // --------------------------------------------------------------------
  QAction* obsUpdateAction = new QAction(QIcon(QPixmap(synop_red_xpm)), tr("Update observations"), this);
  obsUpdateAction->setIconVisibleInMenu(true);
  connect( obsUpdateAction, SIGNAL( triggered() ), SLOT(updateObs()));

  // edit  ===============================
  // --------------------------------------------------------------------
  QAction* undoAction = new QAction(this);
  undoAction->setShortcutContext(Qt::ApplicationShortcut);
  undoAction->setShortcut(Qt::CTRL+Qt::Key_Z);
  connect(undoAction, SIGNAL( triggered() ), SLOT(undo()));
  addAction( undoAction );
  // --------------------------------------------------------------------
  QAction* redoAction = new QAction(this);
  redoAction->setShortcutContext(Qt::ApplicationShortcut);
  redoAction->setShortcut(Qt::CTRL+Qt::Key_Y);
  connect(redoAction, SIGNAL( triggered() ), SLOT(redo()));
  addAction( redoAction );
  // --------------------------------------------------------------------
  QAction* saveAction = new QAction(this);
  saveAction->setShortcutContext(Qt::ApplicationShortcut);
  saveAction->setShortcut(Qt::CTRL+Qt::Key_S);
  connect(saveAction, SIGNAL( triggered() ), SLOT(save()));
  addAction( saveAction );
  // --------------------------------------------------------------------

  // Browsing quick menus ===============================
  // --------------------------------------------------------------------
  QShortcut* leftBrowsingAction = new QShortcut(Qt::ALT + Qt::Key_Left, this);
  connect( leftBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  QShortcut* rightBrowsingAction = new QShortcut(Qt::ALT + Qt::Key_Right, this);
  connect( rightBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  QShortcut* upBrowsingAction = new QShortcut(Qt::ALT + Qt::Key_Up, this);
  connect( upBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));
  QShortcut* downBrowsingAction = new QShortcut(Qt::ALT + Qt::Key_Down, this);
  connect( downBrowsingAction, SIGNAL( activated() ), SLOT( startBrowsing()));

  /*
    ----------------------------------------------------------
    Menu Bar
    ----------------------------------------------------------
   */

  //-------File menu
  QMenu* filemenu = menuBar()->addMenu(tr("File"));
  filemenu->addAction( fileSavePictAction );
  filemenu->addAction( filePrintAction );
  filemenu->addAction( filePrintPreviewAction );
  filemenu->addSeparator();
  filemenu->addAction( fileQuitAction );

  //-------Options menu
  QMenu* optmenu = menuBar()->addMenu(tr("O&ptions"));
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

  initCoserverClient();
  setInstanceName(instancename);

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
  showmenu->addAction(fm->action());
  showmenu->addAction(om->action());
  showmenu->addAction(sm->action());
  showmenu->addAction(stm->action());
  showmenu->addAction( showEditDialogAction         );
  showmenu->addAction(objm->action());
  showmenu->addAction( showTrajecDialogAction       );
  showmenu->addAction( showAnnotationDialogAction       );
  showmenu->addAction( showMeasurementsDialogAction    );
  showmenu->addAction( showProfilesDialogAction     );
  showmenu->addAction( showCrossSectionDialogAction );
  showmenu->addAction( showWaveSpectrumDialogAction );

  showmenu->addMenu( infomenu );

  showmenu->addAction(toggleEditDrawingModeAction);

  //   //-------Help menu
  QMenu* helpmenu = menuBar()->addMenu(tr("&Help"));
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

  levelToolbar= new QToolBar("levelToolBar",this);
  mainToolbar = new QToolBar("mainToolBar",this);
  levelToolbar->setObjectName("levelToolBar");
  mainToolbar->setObjectName("mainToolBar");
  addToolBar(Qt::RightToolBarArea,mainToolbar);
  addToolBar(Qt::TopToolBarArea,levelToolbar);
  insertToolBar(levelToolbar, timeNavigator->toolbar());

  levelToolbar->addAction( toolLevelUpAction );
  levelToolbar->addAction( toolLevelDownAction );
  levelToolbar->addSeparator();
  levelToolbar->addAction( toolIdnumUpAction );
  levelToolbar->addAction( toolIdnumDownAction );
  levelToolbar->addSeparator();
  levelToolbar->addAction( obsUpdateAction );

  // Autoupdate button / functionality is enabled via setup file
  if (LocalSetupParser::basicValue("autoupdate") == "true") {
    autoUpdateAction = new QAction(tr("Automatic updates"), this);
    showAutoUpdateStatus(AUTOUPDATE_OFF);
    autoUpdateAction->setCheckable(true);
    autoUpdateAction->setIconVisibleInMenu(true);
    doAutoUpdate = false;
    connect(autoUpdateAction, &QAction::triggered, this, &DianaMainWindow::autoUpdate);

    levelToolbar->addAction(autoUpdateAction);
  } else {
    autoUpdateAction = nullptr;
  }

  mainToolbar->addAction(showResetAreaAction);

  /****************** Status bar *****************************/

  statusbuttons= new StatusPlotButtons();
  connect(statusbuttons, SIGNAL(toggleElement(PlotElement)),
      SLOT(toggleElement(PlotElement)));
  connect(statusbuttons, SIGNAL(releaseFocus()),
      SLOT(setFocus()));
  statusBar()->addWidget(statusbuttons);
  if (!showelem) statusbuttons->hide();

  connect(contr, &Controller::repaintNeeded, this, &DianaMainWindow::updatePlotElements);

  showsatval = new ShowSatValues();
  statusBar()->addPermanentWidget(showsatval);

  sgeopos = new StatusGeopos(contr);
  statusBar()->addPermanentWidget(sgeopos);


  archiveL = new QLabel(tr("ARCHIVE"),statusBar());
  archiveL->setFrameStyle( QFrame::Panel );
  archiveL->setAutoFillBackground(true);
  QPalette palette;
  palette.setColor(archiveL->backgroundRole(), "red");
  archiveL->setPalette(palette);
  statusBar()->addPermanentWidget(archiveL);
  archiveL->hide();

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
    const std::vector<std::string> vs = miutil::split(avatarpath, ":");
    for ( unsigned int i=0; i<vs.size(); i++ ){
      ig.addImagesInDirectory(vs[i]);
    }
  }

  //-------------------------------------------------

  w= new WorkArea(contr,this);
  setCentralWidget(w);
  const int w_margin = 1;
  centralWidget()->layout()->setContentsMargins(w_margin, w_margin, w_margin, w_margin);

  connect(w->Gli(), &MainUiEventHandler::mouseGridPos, this, &DianaMainWindow::catchMouseGridPos);
  connect(w->Gli(), &MainUiEventHandler::mouseRightPos, this, &DianaMainWindow::catchMouseRightPos);
  connect(w->Gli(), &MainUiEventHandler::mouseMovePos, this, &DianaMainWindow::catchMouseMovePos);
  connect(w->Gli(), &MainUiEventHandler::mouseDoubleClick, this, &DianaMainWindow::catchMouseDoubleClick);
  connect(w->Gli(), &MainUiEventHandler::keyPress, this, &DianaMainWindow::catchKeyPress);

  // ----------- init dialog-objects -------------------

  std::vector<QuickMenuDefs> qdefs;
  LocalSetupParser::getQuickMenus(qdefs);
  qm = new QuickMenu(this, qdefs);
  qm->hide();
  mainToolbar->addAction( showQuickmenuAction         );

  mm= new MapDialog(this, contr);
  mmdock = new QDockWidget(tr("Map and Area"), this);
  mmdock->setObjectName("dock_map");
  mmdock->setWidget(mm);
  addDockWidget(Qt::RightDockWidgetArea, mmdock);
  mmdock->hide();
  mainToolbar->addAction( showMapDialogAction         );

  mainToolbar->addAction(fm->action());

  mainToolbar->addAction(om->action());

  mainToolbar->addAction(sm->action());

  mainToolbar->addAction(stm->action());

  mainToolbar->addAction(objm->action());

  trajm = new TrajectoryDialog(this,contr);
  trajm->hide();
  mainToolbar->addAction( showTrajecDialogAction      );

  annom = new AnnotationDialog(this,contr);
  annom->hide();

  measurementsm = new MeasurementsDialog(this,contr);
  measurementsm->setFocusPolicy(Qt::StrongFocus);
  measurementsm->hide();
  mainToolbar->addAction( showMeasurementsDialogAction   );

  EditItems::DrawingDialog *drawingDialog = new EditItems::DrawingDialog(this, contr);
  addDialog(drawingDialog);

  addDialog(new WebMapDialog(this, contr));

  editDrawingToolBar = EditItems::ToolBar::instance(this);
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

  connect(qm, &QuickMenu::Apply, this, &DianaMainWindow::recallPlot);
  connect(em, &EditDialog::Apply, this, &DianaMainWindow::recallPlot);

  // Mark trajectory positions
  connect(trajm, SIGNAL(markPos(bool)), SLOT(trajPositions(bool)));
  connect(trajm, SIGNAL(updateTrajectories()),SLOT(updateGLSlot()));

  // Mark measurement positions
  connect(measurementsm, SIGNAL(markMeasurementsPos(bool)), SLOT(measurementsPositions(bool)));
  connect(measurementsm, SIGNAL(updateMeasurements()),SLOT(updateGLSlot()));

  connect(em, SIGNAL(editUpdate()), SLOT(requestBackgroundBufferUpdate()));
  connect(em, SIGNAL(editMode(bool)), SLOT(inEdit(bool)));

  connect( mm, SIGNAL(MapApply()),   SLOT(MenuOK()));
  connect( em, SIGNAL(editApply()),  SLOT(editApply()));
  connect( annom, SIGNAL(AnnotationApply()),  SLOT(MenuOK()));

  connect( mm, SIGNAL(MapHide()),    SLOT(mapMenu()));
  connect( mmdock, SIGNAL(visibilityChanged(bool)),  SLOT(mapDockVisibilityChanged(bool)));
  connect( em, SIGNAL(EditHide()),   SLOT(editMenu()));
  connect( qm, SIGNAL(QuickHide()),  SLOT(quickMenu()));
  connect( qm, SIGNAL(finished(int)),  SLOT(quickMenu(int)));
  connect( trajm, SIGNAL(TrajHide()),SLOT(trajMenu()));
  connect( trajm, SIGNAL(finished(int)),  SLOT(trajMenu(int)));
  connect( annom, SIGNAL(AnnotationHide()),SLOT(AnnotationMenu()));
  connect( annom, SIGNAL(finished(int)),  SLOT(AnnotationMenu(int)));
  connect( measurementsm, SIGNAL(MeasurementsHide()),SLOT(measurementsMenu()));
  connect( measurementsm, SIGNAL(finished(int)),  SLOT(measurementsMenu(int)));

  // update field dialog when editing field
  connect(em, &EditDialog::emitFieldEditUpdate, fm, &FieldDialog::fieldEditUpdate);

  // HELP
  connect( stm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( mm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( em, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( qm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));
  connect( trajm, SIGNAL(showsource(const std::string, const std::string)),
      help,SLOT(showsource(const std::string, const std::string)));

  connect(w->Gli(), &MainUiEventHandler::objectsChanged, em, &EditDialog::undoFrontsEnable);
  connect(w->Gli(), &MainUiEventHandler::fieldsChanged, em, &EditDialog::undoFieldsEnable);

  mainToolbar->addSeparator();

  // vertical profiles
  // create a new main window
#ifndef DISABLE_VPROF
  vpWindow = new VprofWindow();
  connect(vpWindow, &VprofWindow::VprofHide, this, &DianaMainWindow::hideVprofWindow);
  connect(vpWindow, &VprofWindow::showsource, help, &HelpDialog::showsource);
  connect(vpWindow, &VprofWindow::stationChanged, this, &DianaMainWindow::stationChangedSlot);
  connect(vpWindow, &VprofWindow::modelChanged, this, &DianaMainWindow::modelChangedSlot);
  mainToolbar->addAction(showProfilesDialogAction);
#endif

  // vertical crossections
  // create a new main window
#ifndef DISABLE_VCROSS
  vcInterface.reset(new VcrossWindowInterface);
  if (VcrossInterface* vc = vcInterface.get()) {
    connect(vc, &VcrossInterface::VcrossHide, this, &DianaMainWindow::hideVcrossWindow);
    connect(vc, &VcrossInterface::requestHelpPage, help, &HelpDialog::showsource);
    connect(vc, &VcrossInterface::requestVcrossEditor, this, &DianaMainWindow::onVcrossRequestEditManager);
    connect(vc, &VcrossInterface::crossectionChanged, this, &DianaMainWindow::crossectionChangedSlot);
    connect(vc, &VcrossInterface::crossectionSetChanged, this, &DianaMainWindow::crossectionSetChangedSlot);
    connect(vc, &VcrossInterface::quickMenuStrings, this, &DianaMainWindow::updateVcrossQuickMenuHistory);
    connect(vc, &VcrossInterface::vcrossHistoryPrevious, this, &DianaMainWindow::prevHVcrossPlot);
    connect(vc, &VcrossInterface::vcrossHistoryNext, this, &DianaMainWindow::nextHVcrossPlot);
  }
  mainToolbar->addAction(showCrossSectionDialogAction);
#endif

  // Wave spectrum
  // create a new main window
#ifndef DISABLE_WAVESPEC
  spWindow = new SpectrumWindow();
  connect(spWindow, &SpectrumWindow::SpectrumHide, this, &DianaMainWindow::hideSpectrumWindow);
  connect(spWindow, &SpectrumWindow::showsource, help, &HelpDialog::showsource);
  connect(spWindow, &SpectrumWindow::spectrumChanged, this, &DianaMainWindow::spectrumChangedSlot);
  connect(spWindow, &SpectrumWindow::spectrumSetChanged, this, &DianaMainWindow::spectrumSetChangedSlot);
  mainToolbar->addAction( showWaveSpectrumDialogAction);
#endif

  // browse plots
  browser= new BrowserBox(this);
  connect(browser, &BrowserBox::selectplot, this, &DianaMainWindow::browserSelect);
  connect(browser, &BrowserBox::cancel, this, &DianaMainWindow::browserCancel);
  connect(browser, &BrowserBox::prevplot, this, &DianaMainWindow::prevQPlot);
  connect(browser, &BrowserBox::nextplot, this, &DianaMainWindow::nextQPlot);
  connect(browser, &BrowserBox::prevlist, this, &DianaMainWindow::prevList);
  connect(browser, &BrowserBox::nextlist, this, &DianaMainWindow::nextList);
  browser->hide();

  connect(fm, &FieldDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  connect(om, &ObsDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  connect(sm, &SatDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  connect(em, &EditDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  connect(objm, &ObjectDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  if (vpWindow) {
    connect(vpWindow, &VprofWindow::sendTimes, timeNavigator, &TimeNavigator::insert);
    connect(vpWindow, &VprofWindow::setTime, timeNavigator, &TimeNavigator::setTimeForDataType);
  }
  if (VcrossInterface* vc = vcInterface.get()) {
    connect(vc, &VcrossInterface::sendTimes, timeNavigator, &TimeNavigator::insert);
    connect(vc, &VcrossInterface::setTime, timeNavigator, &TimeNavigator::setTimeForDataType);
  }
  if (spWindow) {
    connect(spWindow, &SpectrumWindow::sendTimes, timeNavigator, &TimeNavigator::insert);
    connect(spWindow, &SpectrumWindow::setTime, timeNavigator, &TimeNavigator::setTimeForDataType);
  }

  mainToolbar->addSeparator();
  mainToolbar->addAction( showResetAllAction );

  setAcceptDrops(true);

  METLIBS_LOG_INFO("Creating DianaMainWindow done");
}

void DianaMainWindow::initCoserverClient()
{
  qsocket = false;
  pluginB = new ClientSelection("Diana", this);
  pluginB->client()->setServerCommand(QString::fromStdString(LocalSetupParser::basicValue("qserver")));
  connect(pluginB, SIGNAL(receivedMessage(int, const miQMessage&)),
      SLOT(processLetter(int, const miQMessage&)));
  connect(pluginB, SIGNAL(disconnected()),
      SLOT(connectionClosed()));
  connect(pluginB, SIGNAL(renamed(const QString&)),
      SLOT(setInstanceName(const QString&)));
}

void DianaMainWindow::createHelpDialog()
{
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

void DianaMainWindow::resetAll()
{
  mm->useFavorite();
  PlotCommand_cpv pstr = mm->getOKString();
  recallPlot(pstr, true);
  MenuOK();
}

void DianaMainWindow::recallPlot(const PlotCommand_cpv& vstr, bool replace)
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;

  if (!vstr.empty() && vstr.front()->commandKey() == "VCROSS") {
    vcrossMenu();
    if (vcInterface.get())
      vcInterface->parseQuickMenuStrings(vstr);
    return;
  }

  if (!vstr.empty() && vstr.front()->commandKey() == "VPROF") {
    vprofMenu();
    vpWindow->applyPlotCommands(vstr);
    return;
  }

  // strings for each dialog
  PlotCommand_cpv mapcom,obscom,satcom,statcom,objcom,labelcom,fldcom;
  std::map<std::string, PlotCommand_cpv> dialog_com;
  for (PlotCommand_cp c : vstr) {
    METLIBS_LOG_DEBUG(LOGVAL(c->toString()));
    const std::string& pre = c->commandKey();
    if (pre=="MAP") mapcom.push_back(c);
    else if (pre=="AREA") mapcom.push_back(c);
    else if (pre=="FIELD") fldcom.push_back(c);
    else if (pre=="SAT") satcom.push_back(c);
    else if (pre=="OBS") obscom.push_back(c);
    else if (pre=="STATION") statcom.push_back(c);
    else if (pre=="OBJECTS") objcom.push_back(c);
    else if (pre=="LABEL") labelcom.push_back(c);
    else if (dialogNames.find(pre) != dialogNames.end()) {
      dialog_com[pre].push_back(c);
    }
  }

  // feed strings to dialogs
  if (replace || mapcom.size()) mm->putOKString(mapcom);
  if (replace || fldcom.size()) fm->putOKString(fldcom);
  if (replace || satcom.size()) sm->putOKString(satcom);
  if (replace || obscom.size()) om->putOKString(obscom);
  if (replace || statcom.size()) stm->putOKString(statcom);
  if (replace || objcom.size()) objm->putOKString(objcom);
  if (replace || labelcom.size()) annom->putOKString(labelcom);

  // Other data sources

  // If the strings are being replaced then update each of the
  // dialogs whether it has a command or not. Otherwise, only
  // update those with a corresponding string.
  std::map<std::string, DataDialog *>::iterator it;
  for (it = dialogNames.begin(); it != dialogNames.end(); ++it) {
    DataDialog *dialog = it->second;
    if (replace || dialog_com.find(it->first) != dialog_com.end())
      dialog->putOKString(dialog_com[it->first]);
  }

  applyPlotCommandsFromDialogs(false);
}

void DianaMainWindow::toggleEditDrawingMode()
{
  METLIBS_LOG_SCOPE();
  setEditDrawingMode(!editDrawingToolBar->isVisible());

  METLIBS_LOG_DEBUG("enabled " << editDrawingToolBar->isVisible());
}

void DianaMainWindow::setEditDrawingMode(bool enabled)
{
  if (enabled)
    editDrawingToolBar->show();
  else
    editDrawingToolBar->hide();

  EditItemManager::instance()->setEditing(enabled);
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
  applyPlotCommandsFromDialogs(false);
}

namespace {

bool isLabelCommandWithTime(PlotCommand_cp cmd)
{
  if (LabelPlotCommand_cp c = std::dynamic_pointer_cast<const LabelPlotCommand>(cmd)) {
    for (const miutil::KeyValue& kv : c->all()) {
      if (miutil::contains(kv.value(), "$"))
        return true;
    }
  }
  return false;
}
} // namespace

void DianaMainWindow::getPlotStrings(PlotCommand_cpv &pstr, std::vector<std::string> &shortnames)
{
  // fields
  pstr = fm->getOKString();
  shortnames.push_back(fm->getShortname());

  // Observations
  diutil::insert_all(pstr, om->getOKString());
  shortnames.push_back(om->getShortname());

  //satellite
  diutil::insert_all(pstr, sm->getOKString());
  shortnames.push_back(sm->getShortname());

  // Stations
  diutil::insert_all(pstr, stm->getOKString());
  shortnames.push_back(stm->getShortname());

  // objects
  diutil::insert_all(pstr, objm->getOKString());
  shortnames.push_back(objm->getShortname());

  // map
  diutil::insert_all(pstr, mm->getOKString());
  shortnames.push_back(mm->getShortname());

  // annotation
  const bool remove = (contr->editManagerIsInEdit() || !timeNavigator->hasTimes());
  for (PlotCommand_cp cmd : annom->getOKString()) {
    if (!remove || !isLabelCommandWithTime(cmd))
      pstr.push_back(cmd);
  }

  // Other data sources
  for (auto it : dialogs) {
    diutil::insert_all(pstr, it.second->getOKString());
  }
}

void DianaMainWindow::updatePlotElements()
{
  if (showelem)
    statusbuttons->setPlotElements(contr->getPlotElements());
}

void DianaMainWindow::MenuOK()
{
  METLIBS_LOG_SCOPE();
  applyPlotCommandsFromDialogs(true);
}

void DianaMainWindow::applyPlotCommandsFromDialogs(bool addToHistory)
{
  METLIBS_LOG_SCOPE();

  diutil::OverrideCursor waitCursor;

  PlotCommand_cpv pstr;
  std::vector<std::string> shortnames;
  getPlotStrings(pstr, shortnames);

  // init level up/down arrows
  toolLevelUpAction->setEnabled(fm->levelsExists(true,0));
  toolLevelDownAction->setEnabled(fm->levelsExists(false,0));
  toolIdnumUpAction->setEnabled(fm->levelsExists(true,1));
  toolIdnumDownAction->setEnabled(fm->levelsExists(false,1));

  contr->plotCommands(pstr);
  METLIBS_LOG_INFO(contr->getMapArea());
  setPlotTime(timeNavigator->selectedTime());

  // push command on history-stack
  if (addToHistory) { // only when proper menuok
    // make shortname
    std::string plotname;
    for (const std::string& sn : shortnames)
      diutil::appendText(plotname, sn);
    qm->addPlotToHistory(plotname, pstr, QuickMenu::HISTORY_MAP);
  }
}

void DianaMainWindow::updateVcrossQuickMenuHistory(const std::string& plotname, const PlotCommand_cpv& pstr)
{
  qm->addPlotToHistory(plotname, pstr, QuickMenu::HISTORY_VCROSS);
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
  qm->prevHPlot(QuickMenu::HISTORY_MAP);
}

// recall next plot in plot-stack (if exists)
void DianaMainWindow::nextHPlot()
{
  qm->nextHPlot(QuickMenu::HISTORY_MAP);
}

void DianaMainWindow::prevHVcrossPlot()
{
  qm->prevHPlot(QuickMenu::HISTORY_VCROSS);
}

// recall next plot in plot-stack (if exists)
void DianaMainWindow::nextHVcrossPlot()
{
  qm->nextHPlot(QuickMenu::HISTORY_VCROSS);
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
  browserCancel();
  qm->applyPlot();
}

// add current plot to a quick-menu
void DianaMainWindow::addToMenu()
{
  AddtoMenu am(this, qm);
  am.exec();
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
    if ((visi[2] = fm->isVisible()))
      fm->setVisible(false);
    if ((visi[3] = om->isVisible()))
      om->setVisible(false);
    if ((visi[4] = sm->isVisible()))
      sm->setVisible(false);
    //    if ((visi[5]= em->isVisible()))    editMenu();
    if ((visi[6] = objm->isVisible()))
      objm->setVisible(false);
    if ((visi[7]= trajm->isVisible())) trajMenu();
    if ((visi[8]= measurementsm->isVisible())) measurementsMenu();
    if ((visi[9] = stm->isVisible()))
      stm->setVisible(false);
  } else {
    if (visi[0]) quickMenu();
    if (visi[1]) mapMenu();
    if (visi[2])
      fm->setVisible(true);
    if (visi[3])
      om->setVisible(true);
    if (visi[4])
      sm->setVisible(true);
    //    if (visi[5]) editMenu();
    if (visi[6])
      objm->setVisible(true);
    if (visi[7]) trajMenu();
    if (visi[8]) measurementsMenu();
    if (visi[9])
      stm->setVisible(true);
  }
}

static void toggleDialogVisibility(QWidget* dialog, QAction* dialogAction, int result = -1)
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

void DianaMainWindow::mapMenu(int result)
{
  toggleDialogVisibility(mmdock, showMapDialogAction, result);
}

void DianaMainWindow::mapDockVisibilityChanged(bool visible)
{
  showMapDialogAction->setChecked(visible);
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

  const miutil::miTime& t = contr->getPlotTime();
  vcInterface->mainWindowTimeChanged(!t.undef() ? t : miutil::miTime::nowTime());
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
  const std::string txt = action->text().toStdString();
  std::map<std::string,InfoFile>::const_iterator it = infoFiles.find(txt);
  if (action && it != infoFiles.end()) {
    const InfoFile& ifi = it->second;
    if (diutil::startswith(ifi.filename, "http")) {
      QDesktopServices::openUrl(QUrl(QString::fromStdString(ifi.filename)));
    } else {
      TextDialog* td= new TextDialog(this, ifi);
      td->show();
    }
  }
}


void DianaMainWindow::vprofStartup()
{
  if (!vpWindow)
    return;
  const miutil::miTime& t = contr->getPlotTime();
  vpWindow->startUp(t);
  vpWindow->show();
  contr->stationCommand("show","vprof");
}


void DianaMainWindow::spectrumStartup()
{
  if (!spWindow)
    return;
  const miutil::miTime& t = contr->getPlotTime();
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

void DianaMainWindow::stationChangedSlot(const std::vector<std::string>& station)
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
      EditItems::ToolBar::instance(this)->setCreateSymbolAction(TIME_GRAPH_TYPE);
    else
      EditItems::ToolBar::instance(this)->setCreatePolyLineAction(CROSS_SECTION_TYPE);

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
  std::string s =station.toStdString();
  std::vector<std::string> data;
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

void DianaMainWindow::applyQuickMenu(const QString& menu, const QString& item)
{
  qm->applyItem(menu.toStdString(), item.toStdString());
  qm->applyPlot();
}

void DianaMainWindow::showAutoUpdateStatus(AutoUpdateStatus aus)
{
  if (autoUpdateAction) {
    const char** icon;
    QString tooltip;
    switch (aus) {
    case AUTOUPDATE_OFF:
      icon = autoupdate_off_xpm;
      tooltip = tr("Automatic updates mode is available but off");
      break;
    case AUTOUPDATE_ON:
      icon = autoupdate_on_xpm;
      tooltip = tr("Automatic updates mode is available and on");
      break;
    case AUTOUPDATE_WARN:
      icon = autoupdate_warn_xpm;
      tooltip = tr("Automatic updates mode is available but there is a warning");
      break;
    }
    autoUpdateAction->setIcon(QIcon(QPixmap(icon)));
    autoUpdateAction->setToolTip(tooltip);
  }
}

void DianaMainWindow::connectionClosed()
{
  //  METLIBS_LOG_DEBUG("Connection closed");
  qsocket = false;
  
  if (doAutoUpdate) {
    showAutoUpdateStatus(AUTOUPDATE_WARN);
  }

  contr->stationCommand("delete","all");
  contr->areaObjectsCommand("delete","all", std::vector<std::string>(1, "all"),-1);

  timeNavigator->removeTimes(-1);

  textview->hide();
  MenuOK();
  updatePlotElements();
}


void DianaMainWindow::processLetter(int fromId, const miQMessage &qletter)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("message from " << fromId << ": " << qletter);

  miMessage letter;
  convert(fromId, 0 /*my id, unused*/, qletter, letter);

  const QString& command = qletter.command();
  if (command == "menuok") {
    MenuOK();
  }

  else if (command == qmstrings::apply_quickmenu) {
    //data[0]=menu, data[1]=item
    if (qletter.countDataRows()==2 && qletter.countDataColumns()>0) {
      const QString& menu = qletter.getDataValue(0, 0);
      const QString& item = qletter.getDataValue(1, 0);
      applyQuickMenu(menu, item);
    } else {
      MenuOK();
    }
  }

  else if (command == qmstrings::vprof) {
    //description: lat:lon
    vprofMenu();
    if (qletter.countDataRows()>0 && qletter.countDataColumns() == 2) {
      bool lat_ok = false, lon_ok = false;
      const float lat = qletter.getDataValue(0, 0).toFloat(&lat_ok);
      const float lon = qletter.getDataValue(0, 1).toFloat(&lon_ok);
      if (lon_ok && lat_ok) {
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

  else if (command == qmstrings::vcross) {
    //description: name
    vcrossMenu();
    if (qletter.countDataRows() > 0) {
        //tell vcInterface to plot this crossection
        if (vcInterface.get()) {
          const QString& cs = qletter.getDataValue(0, 0);
          vcInterface->changeCrossection(cs);
        }
    }
  }

  else if (command == qmstrings::addimage) {
    // description: name:image

    QtImageGallery ig;
    const int n = qletter.countDataRows();
    for (int i=0; i<n; i++) {
      const QStringList& row = qletter.getDataValues(i);
      if (row.size() < 2)
        continue;
      const std::string imageName = row.at(0).toStdString();
      const std::string imageStr = row.at(1).toStdString();
      ig.addImageToGallery(imageName, imageStr);
    }
  }

  else if (command == qmstrings::positions) {
    //commondesc: dataSet:image:normal:selected:icon:annotation
    //description: stationname:lat:lon:image:alpha

    //positions from diana, nothing to do
    if (letter.common == "diana")
      return;

    //obsolete -> new syntax
    std::string commondesc = letter.commondesc;
    std::string common = letter.common;
    std::string description = letter.description;

    if (letter.description.find(";") != letter.description.npos) {
      std::vector<std::string> desc;
      boost::algorithm::split(desc, letter.description, boost::algorithm::is_any_of(";"));
      if (desc.size() < 2)
        return;
      std::string dataSet = desc[0];
      description=desc[1];
      commondesc = "dataset:" + letter.commondesc;
      common = dataSet + ":" + letter.common;
    }

    contr->makeStationPlot(commondesc, common,
        description, fromId, letter.data);

    return;
  }

  else if (command == qmstrings::annotation) {
    //     commondesc = dataset;
    //     description = annotation;
    contr->stationCommand("annotation", letter.data, letter.common, fromId);
  }

  else if (command == qmstrings::showpositions || command == qmstrings::hidepositions) {
    //description: dataset
    const std::string cmd = (command == qmstrings::showpositions) ? "show" : "hide";
    contr->stationCommand(cmd, letter.description, fromId);
    updatePlotElements();
  }

  else if (command == qmstrings::changeimageandtext) {
    //METLIBS_LOG_DEBUG("Change text and image\n");
    //description: dataSet;stationname:image:text:alignment
    //find name of data set from description
    std::vector<std::string> desc;
    boost::algorithm::split(desc, letter.description, boost::algorithm::is_any_of(";"));
    if (desc.size() == 2) { //obsolete syntax
      contr->stationCommand("changeImageandText",
          letter.data,desc[0],fromId,desc[1]);
    } else { //new syntax
      //commondesc: name of dataset
      //description: stationname:image:image2:text
      //             :alignment:rotation (0-7):alpha

      contr->stationCommand("changeImageandText",
          letter.data, letter.common, fromId, letter.description);
    }
  }

  else if (command == qmstrings::selectposition) {
    //commondesc: dataset
    //description: stationName
    contr->stationCommand("selectPosition", letter.data, letter.common, fromId);
  }

  else if (command == qmstrings::showpositionname) {
    //description: normal:selected
    contr->stationCommand("showPositionName", letter.data, letter.common, fromId);
  }

  else if (command == qmstrings::showpositiontext) {
    //description: showtext:colour:size
    contr->stationCommand("showPositionText", letter.data,
        letter.common, fromId, letter.description);
  }

  else if (command == qmstrings::areas) {
    if(letter.data.size()>0) {
      contr->makeAreaObjects(letter.common, letter.data[0], fromId);
      updatePlotElements();
    }
  }

  else if (command == qmstrings::areacommand) {
    //commondesc command:dataSet
    std::string cmd, ds;
    if (qletter.countCommon() >= 2) {
      const int c_cmd = qletter.findCommonDesc("command"), c_ds = qletter.findCommonDesc("dataset");
      if (c_cmd >= 0 && c_ds >= 0) {
        cmd = qletter.getCommonValue(c_cmd).toStdString();
        ds = qletter.getCommonValue(c_ds).toStdString();
      }
    } else if (qletter.countCommon() == 1) {
      METLIBS_LOG_WARN("obsolete areacommand, commondesc should be [\"command\",\"dataset\"]");
      // miMessage converter will not split common if commondesc is empty
      const QStringList c = qletter.getCommonValue(0).split(":");
      if (c.count() >= 2) {
        cmd = c[0].toStdString();
        ds = c[1].toStdString();
      }
    } else {
      METLIBS_LOG_WARN("incomprehensible areacommand, commondesc should be [\"command\",\"dataset\"]");
      return;
    }
    const int n = qletter.countDataRows();
    if (n == 0) {
      contr->areaObjectsCommand(cmd, ds, std::vector<std::string>(), fromId);
    } else {
      for (int i=0; i<n; i++)
        contr->areaObjectsCommand(cmd, ds, diutil::toVector(qletter.getDataValues(i)), fromId);
    }
  }

  else if (command == qmstrings::showtext) {
    //description: station:text
    if (qletter.countDataRows() && qletter.countDataColumns() >= 2) {
      const std::string name = pluginB->getClientName(fromId).toStdString();
      textview->setText(fromId, name, qletter.getDataValue(0, 1).toStdString());
      textview->show();
    }
  }

  else if (command == qmstrings::enableshowtext) {
    //description: dataset:on/off
    if (qletter.countDataRows() && qletter.countDataColumns() >= 2) {
      if (qletter.getDataValue(0, 1) == "on") {
        textview->show();
      } else {
        textview->deleteTab(fromId);
      }
    }
  }

  else if (command == qmstrings::removeclient) {
    // commondesc = id:dataset
    if (qletter.countCommon() < 2)
      return;
    const int id = qletter.getCommonValue(0).toInt();
    //remove stationPlots from this client
    contr->stationCommand("delete", "", id);
    //remove areas from this client
    contr->areaObjectsCommand("delete", "all", std::vector<std::string>(1, "all"), id);
    timeNavigator->removeTimes(id);
    //hide textview
    textview->deleteTab(id);
    //     if(textview && id == textview_id )
    //       textview->hide();
    updatePlotElements();
  }

  else if (command == qmstrings::newclient) {
    qsocket = true;
    if (doAutoUpdate) {
      showAutoUpdateStatus(AUTOUPDATE_ON);
    }
    autoredraw[qletter.getCommonValue(0).toInt()] = true;
    autoredraw[0] = true; //from server
  }

  else if (command == qmstrings::autoredraw) {
    if(letter.common == "false")
      autoredraw[fromId] = false;
  }

  else if (command == qmstrings::redraw) {
    requestBackgroundBufferUpdate();
  }

  else if (!handlingTimeMessage && ((command == qmstrings::settime && qletter.findCommonDesc("time") == 0) ||
                                    (command == qmstrings::timechanged && qletter.findCommonDesc("time") == 0))) {
    const std::string l_common = qletter.getCommonValue(0).toStdString();
    miutil::miTime t(l_common);
    handlingTimeMessage = true;
    timeNavigator->requestTime(t); // triggers "timeSelected" signal, connected to "setPlotTime"
    handlingTimeMessage = false;
  }

  else if (command == qmstrings::getcurrentplotcommand) {
    PlotCommand_cpv v1;
    std::vector<std::string> v2;
    getPlotStrings(v1, v2);

    miQMessage l(qmstrings::currentplotcommand);
    l.addDataDesc(""); // FIXME
    for (PlotCommand_cp cmd : v1)
      l.addDataValues(QStringList() << QString::fromStdString(cmd->toString()));
    sendLetter(l, fromId);
    return; // no need to repaint
  }

  else if (command == qmstrings::getproj4maparea) {
    miQMessage l(qmstrings::proj4maparea);
    l.addDataDesc(""); // FIXME
    l.addDataValues(QStringList() << QString::fromStdString(contr->getMapArea().getAreaString()));
    sendLetter(l, fromId);
    return; // no need to repaint
  }

  else if (command == qmstrings::getmaparea) { //obsolete
    miQMessage l(qmstrings::maparea);
    l.addDataDesc(""); // FIXME
    l.addDataValues(QStringList() << "Obsolet command: use getproj4maparea");
    sendLetter(l, fromId);
    return; // no need to repaint
  }

  // If autoupdate is active, reread sat/radarfiles and
  // show the latest timestep
  else if (command == qmstrings::directory_changed) {
    if (doAutoUpdate) {
      if (timeNavigator->isTimerOn() != 0) {
        sm->updateTimes();
        om->updateTimes();
      } else {
        const miutil::miTime ts = timeNavigator->selectedTime();
        // get last time before update
        const miutil::miTime tb = timeNavigator->getLastTime();

        sm->updateTimes();
        om->updateTimes();

        // get last time after update
        const miutil::miTime ta = timeNavigator->getLastTime();
        if (!ts.undef() && !tb.undef() && !ta.undef()) {
          if (ts < tb) {
            showAutoUpdateStatus(AUTOUPDATE_WARN);
          } else {
            showAutoUpdateStatus(AUTOUPDATE_ON);
            handlingTimeMessage = true;
            timeNavigator->requestTime(ta); // triggers "timeSelected" signal, connected to "setPlotTime"
            handlingTimeMessage = false;
          }
        }
      }
    }
  }
  // If autoupdate is active, do the same thing as
  // when the user presses the updateObs button.
  else if (command == qmstrings::file_changed) {
    if (doAutoUpdate) {
      // Just a call to update obs will work fine
      updateObs();
    }
  }

  else {
    return; //nothing to do
  }

  // repaint window
  if (autoredraw[fromId])
    requestBackgroundBufferUpdate();
}

void DianaMainWindow::sendPrintClicked(int id)
{
  miQMessage l(qmstrings::printclicked);
  sendLetter(l);
}

void DianaMainWindow::sendLetter(const miQMessage& qmsg, int to)
{
  METLIBS_LOG_SCOPE(LOGVAL(qmsg) << LOGVAL(to));
  if (to != qmstrings::all)
    pluginB->sendMessage(qmsg, to);
  else
    pluginB->sendMessage(qmsg);
}

void DianaMainWindow::sendLetter(const miQMessage& qmsg)
{
  METLIBS_LOG_SCOPE(LOGVAL(qmsg));
  pluginB->sendMessage(qmsg);
}

void DianaMainWindow::updateObs()
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;
  contr->updateObs();
  requestBackgroundBufferUpdate();
}

void DianaMainWindow::autoUpdate()
{
  METLIBS_LOG_SCOPE();
  doAutoUpdate = !doAutoUpdate;
  METLIBS_LOG_DEBUG(LOGVAL(doAutoUpdate));
  
  if (!doAutoUpdate) {
    showAutoUpdateStatus(AUTOUPDATE_OFF);
  } else if (qsocket) {
    showAutoUpdateStatus(AUTOUPDATE_ON);
  } else {
    showAutoUpdateStatus(AUTOUPDATE_WARN);
  }
  autoUpdateAction->setChecked(doAutoUpdate);
}

void DianaMainWindow::checkAutoUpdate()
{
  if (qsocket && doAutoUpdate) {
    showAutoUpdateStatus(AUTOUPDATE_ON);
  }
}

void DianaMainWindow::requestBackgroundBufferUpdate()
{
  w->Glw()->requestBackgroundBufferUpdate();
  w->updateGL();
}

void DianaMainWindow::updateGLSlot()
{
  requestBackgroundBufferUpdate();
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
      + "version: " + VERSION + "\n"
      + "build: " + diana_build_string + "\n"
      + "commit: " + diana_build_commit;

  QMessageBox::about( this, tr("about Diana"), str );
}

void DianaMainWindow::setPlotTime(const miutil::miTime& t)
{
  METLIBS_LOG_TIME();
  diutil::OverrideCursor waitCursor;

  // broadcast new time first, such that other diana instances can plot in parallel
  /* NOTE
   * clients sending replies depending on the result of updatePlots might cause
   * issues if some Qt signal processing is done before the end of this function
   */
  if (qsocket && !handlingTimeMessage) {
    miQMessage letter(qmstrings::timechanged);
    letter.addCommon("time", QString::fromStdString(t.isoTime()));
    sendLetter(letter);
  }

  contr->changeTime(t);
  requestBackgroundBufferUpdate();

  //to be done whenever time changes (step back/forward, MenuOK etc.)
  objm->commentUpdate();
  if (vpWindow) vpWindow->mainWindowTimeChanged(t);
  if (spWindow) spWindow->mainWindowTimeChanged(t);
  if (vcInterface.get()) vcInterface->mainWindowTimeChanged(t);
  updatePlotElements();

  //update sat channels in statusbar
  std::vector<std::string> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);

  QCoreApplication::sendPostedEvents();
}


void DianaMainWindow::levelUp()
{
  if (toolLevelUpAction->isEnabled())
    levelChange(1, 0);
}

void DianaMainWindow::levelDown()
{
  if (toolLevelDownAction->isEnabled())
    levelChange(-1, 0);
}

void DianaMainWindow::levelChange(int increment, int axis)
{
  diutil::OverrideCursor waitCursor;
  // update field dialog
  fm->changeLevel(increment, axis);
  MenuOK();
}

void DianaMainWindow::idnumUp()
{
  if (toolIdnumUpAction->isEnabled())
    levelChange(1, 1);
}

void DianaMainWindow::idnumDown()
{
  if (toolIdnumDownAction->isEnabled())
    levelChange(-1, 1);
}

void DianaMainWindow::showExportDialog(ImageSource* source)
{
  METLIBS_LOG_SCOPE();
  if (!exportImageDialog_)
    exportImageDialog_ = new ExportImageDialog(this);
  exportImageDialog_->setSource(source);
  exportImageDialog_->show();
}

DianaImageSource* DianaMainWindow::imageSource()
{
  if (!imageSource_) {
    imageSource_.reset(new DianaImageSource(w->Glw()));
    connect(imageSource_.get(), &ImageSource::prepared, w, &QWidget::hide);
    connect(imageSource_.get(), &ImageSource::finished, w, &QWidget::show);
  }
  return imageSource_.get();
}

void DianaMainWindow::saveraster()
{
  imageSource()->setTimes(timeNavigator->animationTimes());
  showExportDialog(imageSource());
}

void DianaMainWindow::parseSetup()
{
  METLIBS_LOG_SCOPE();
  SetupDialog setupDialog(this);

  if (setupDialog.exec()) {
    LocalSetupParser sp;
    std::string filename;
    if (!sp.parse(filename)){
      METLIBS_LOG_ERROR("An error occured while re-reading the setup file '" << filename << "'.");
      QMessageBox::warning(this, tr("Error"), tr("An error occured while re-reading the setup file '%1'.")
                           .arg(QString::fromStdString(filename)));
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

    fm->updateDialog();
    sm->updateDialog();
    om->updateDialog();
  }
}

void DianaMainWindow::hardcopy()
{
  PrinterDialog dialog(this, imageSource(), &priop);
  dialog.print();
}

void DianaMainWindow::previewHardcopy()
{
  PrinterDialog dialog(this, imageSource(), &priop);
  dialog.preview();
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
  const int x = mev->x();
  const int y = mev->y();

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

  //send position to all clients
  if(qsocket){
    miQMessage letter(qmstrings::positions);
    letter.addCommon("dataset", "diana");
    letter.addDataDesc("lat").addDataDesc("lon");
    letter.addDataValues(QStringList() << QString::number(lat) << QString::number(lon));
    sendLetter(letter);
  }

}


// picks up a single click on position x,y
void DianaMainWindow::catchMouseRightPos(QMouseEvent* mev)
{
  METLIBS_LOG_SCOPE();
  zoomOut();
}


// picks up mousemovements (without buttonclicks)
void DianaMainWindow::catchMouseMovePos(QMouseEvent* mev, bool quick)
{
  // METLIBS_LOG_SCOPE(LOGVAL(mev->x()) << LOGVAL(mev->y()));
  int x = mev->x();
  int y = mev->y();

  float xmap=-1., ymap=-1.;
  contr->PhysToMap(x,y,xmap,ymap);

  // show geoposition in statusbar
  sgeopos->handleMousePos(x, y);

  // show sat-value at position
  std::vector<SatValues> satval;
  satval = contr->showValues(xmap, ymap);
  showsatval->ShowValues(satval);

  // no need for updateGL() here...only GUI-display

  //check if inside annotationPlot to edit
  if (contr->markAnnotationPlot(x,y))
    requestBackgroundBufferUpdate();

  if (!quick && optAutoElementAction->isChecked()) {
    catchElement(mev);
  }
}


void DianaMainWindow::catchMouseDoubleClick(QMouseEvent* mev)
{
}

void DianaMainWindow::showStationOrObsText(int x, int y)
{
  QString stationText = contr->getStationsText(x, y);
  QString obsText = QString::fromStdString(contr->getObsPopupText(x, y));
  if (!stationText.isEmpty() || !obsText.isEmpty()) {
    // undo reverted y coordinate from MainPaintable::handleMouseEvents
    QPoint popupPos = w->mapToGlobal(QPoint(x, w->height() - y));
    if (!stationText.isEmpty()) {
      QWhatsThis::showText(popupPos, stationText, this);
    }
    if (!obsText.isEmpty()) {
      QToolTip::showText(popupPos, obsText, this);
    }
  }
}


void DianaMainWindow::catchElement(QMouseEvent* mev)
{
  // METLIBS_LOG_SCOPE(LOGVAL(mev->x()) << LOGVAL(mev->y()));

  int x = mev->x();
  int y = mev->y();

  bool needupdate= false; // updateGL necessary

  //show closest observation
  if (contr->findObs(x,y)) {
    needupdate= true;
  }
  
  showStationOrObsText(x, y);

  //find the name of stations clicked/pointed at
  std::vector<std::string> stations = contr->findStations(x,y,"vprof");
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
    vcInterface->changeCrossection(QString::fromStdString(crossection));
    //  needupdate= true;
  }

  if(qsocket){

    //set selected and send position to plugin connected
    std::vector<int> id;
    std::vector<std::string> name;
    std::vector<std::string> station;

    bool add = false;
    //    if(mev->modifiers() & Qt::ShiftModifier) add = true; //todo: shift already used (skip editmode)
    contr->findStations(x,y,add,name,id,station);
    const int n = name.size();

    QList<QStringList> station_rows;
    const QStringList l_datadesc = QStringList() << "station";
    for(int i=0; i<n; i++) {
      if (i>0 && id[i-1] != id[i] && !station_rows.isEmpty()) {
        // different id, send letters
        QString name_i = QString::fromStdString(name[i-1]);
        miQMessage old_letter(qmstrings::textrequest);
        old_letter.setData(QStringList() << (name_i +";name"), station_rows);
        sendLetter(old_letter, id[i-1]);

        miQMessage letter(qmstrings::selectposition);
        letter.addCommon("name", name_i);
        letter.setData(l_datadesc, station_rows);
        sendLetter(letter, id[i-1]);

        needupdate=true;

        station_rows.clear();
      }

      station_rows << (QStringList() << QString::fromStdString(station[i]));
    }
    if (!station_rows.isEmpty()) {
      QString name_n = QString::fromStdString(name[n-1]);
      miQMessage old_letter(qmstrings::textrequest);
      old_letter.setData(QStringList() << (name_n +";name"), station_rows);
      sendLetter(old_letter, id[n-1]);

      miQMessage letter(qmstrings::selectposition);
      letter.addCommon("name", name_n);
      letter.setData(l_datadesc, station_rows);
      sendLetter(letter, id[n-1]);

      needupdate=true;
    }

  }

  if (needupdate)
    requestBackgroundBufferUpdate();
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

void DianaMainWindow::filequit()
{
  if (checkQuit()) {
    writeLogFile();
    qApp->quit();
  }
}

bool DianaMainWindow::checkQuit()
{
  return em->cleanupForExit();
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
  // FIXME toStdString uses utf8 which might cause encoding problems
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
  pu_struct_stat statbuf;
  if (pu_stat(logfilepath.c_str(), &statbuf) == 0) {
    if (statbuf.st_size < 128)
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
#ifdef DIANA_RESTORE_DIALOG_POSITIONS
  d->move(x, y);
#endif
}

} // anonymous namespace

std::vector<std::string> DianaMainWindow::writeLog(const std::string& thisVersion, const std::string& thisBuild)
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
  saveDialogPos(vstr, "EditDialog", em);
  saveDialogPos(vstr, "ObjectDialog", objm);
  saveDialogPos(vstr, "TrajectoryDialog", trajm);
  saveDialogSize(vstr, "Textview", textview);
  saveDialogPos(vstr, "Textview", textview);

  std::map<QAction*, DataDialog*>::iterator it;
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
  // FIXME hide all floatable dock widgets
  editDrawingToolBar->hide();
  mmdock->setVisible(false);

  QByteArray state = saveState();
  std::ostringstream ost;
  int n= state.count();
  for (int i=0; i<n; i++)
    ost << std::setw(7) << int(state[i]);
  return ost.str();
}

void DianaMainWindow::readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, std::string& logVersion)
{
  std::vector<std::string> tokens;
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
        if      (tokens[0]=="MainWindow.pos")    this->move(x, y);
        else if (tokens[0]=="QuickMenu.pos")     restoreDialogPos(qm, x, y);
        else if (tokens[0]=="FieldDialog.pos")   restoreDialogPos(fm, x, y);
        else if (tokens[0]=="FieldDialog.size")  restoreDialogSize(fm, showmore, x, y);
        else if (tokens[0]=="ObsDialog.pos")     restoreDialogPos(om, x, y);
        else if (tokens[0]=="SatDialog.pos")     restoreDialogPos(sm, x, y);
        else if (tokens[0]=="StationDialog.pos") restoreDialogPos(stm, x, y);
        else if (tokens[0]=="EditDialog.pos")    restoreDialogPos(em, x, y);
        else if (tokens[0]=="ObjectDialog.pos")  restoreDialogPos(objm, x, y);
        else if (tokens[0]=="TrajectoryDialog.pos") restoreDialogPos(trajm, x, y);
        else if (tokens[0]=="Textview.size")     restoreDialogSize(textview, showmore, x, y);
        else if (tokens[0]=="Textview.pos")      restoreDialogPos(textview, x, y);
        else {
          std::map<QAction*, DataDialog*>::iterator it;
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

void DianaMainWindow::restoreDocState(const std::string& logstr)
{
  std::vector<std::string> vs= miutil::split(logstr, " ");
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
  std::ifstream ifile(newsfile.c_str());
  if (ifile) {
    getline(ifile,newsVersion);
    ifile.close();
  }

  if (thisVersion!=newsVersion) {
    // open output filestream
    std::ofstream ofile(newsfile.c_str());
    if (ofile) {
      ofile << thisVersion << std::endl;
      ofile.close();
    }

    showNews();
  }
}

void DianaMainWindow::toggleElement(PlotElement pe)
{
  contr->enablePlotElement(pe);
  std::vector<std::string> channels = contr->getCalibChannels();
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
    statusbuttons->show();
    optOnOffAction->setChecked( true );
    showelem= true;
    updatePlotElements();
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

  archiveL->setVisible(on);
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
  w->Gli()->setUseScrollwheelZoom(on);
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

void DianaMainWindow::zoomOut()
{
  contr->zoomOut();
  requestBackgroundBufferUpdate();
}

void DianaMainWindow::inEdit(bool inedit)
{
  if (qsocket) {
    miQMessage letter(qmstrings::editmode);
    letter.addCommon("on/off", inedit ? "on" : "off");
    sendLetter(letter);
  }
}

void DianaMainWindow::closeEvent(QCloseEvent* event)
{
  if (!checkQuit()) {
    event->ignore();
  } else {
    writeLogFile();
    qApp->quit();
  }
}

bool DianaMainWindow::event(QEvent* event)
{
  if (event->type() == QEvent::WhatsThisClicked) {
    QWhatsThisClickedEvent* wtcEvent = static_cast<QWhatsThisClickedEvent*>(event);
    QDesktopServices::openUrl(wtcEvent->href());
    return false;
  }

  return QMainWindow::event(event);
}

void DianaMainWindow::addStandardDialog(DataDialog* dialog)
{
  dialog->hide();

  connect(dialog, &DataDialog::applyData, this, &DianaMainWindow::MenuOK);
  connect(dialog, &DataDialog::sendTimes, timeNavigator, &TimeNavigator::insert);
  connect(dialog, &DataDialog::updated, this, &DianaMainWindow::requestBackgroundBufferUpdate);
  connect(dialog, &DataDialog::showsource, help, &HelpDialog::showsource);

  if (QAction *action = dialog->action()) {
    connect(action, &QAction::toggled, dialog, &QDialog::setVisible);
    connect(action, &QAction::toggled, this, &DianaMainWindow::requestBackgroundBufferUpdate);
  }
}

void DianaMainWindow::addDialog(DataDialog* dialog)
{
  addStandardDialog(dialog);
  dialogNames[dialog->name()] = dialog;
  if (QAction* action = dialog->action()) {
    dialogs[action] = dialog;
    showmenu->addAction(action);
    mainToolbar->addAction(action);
  }
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
    for (const QUrl& url : event->mimeData()->urls()) {

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

    for (const QUrl& url : event->mimeData()->urls()) {

      if (url.scheme() == "file") {
        const QString fileName = url.toLocalFile();
        const QFileInfo fi(fileName);
        const QString suffix = fi.suffix();

        if (suffix == "nc") {
          QString s = QString("m=%1 t=fimex f=%2 format=netcdf").arg(fi.baseName()).arg(fileName);
          extra_field_lines.push_back(s.toStdString());
          fieldsAdded++;

        } else if (suffix == "svg") {
          QString name = fi.fileName();
          QString section = fi.dir().dirName();
          DrawingManager::instance()->loadSymbol(fileName, section, name);
          editDrawingToolBar->addSymbol(section, name);
          symbolsAdded++;

        } else if (suffix == "kml") {
          EditItems::DrawingDialog::instance()->loadFile(fileName);
          drawingsAdded++;
        }
      }
    }
  }

  // FIXME identical to bdiana
  std::vector<std::string> field_errors;
  if (!contr->updateFieldFileSetup(extra_field_lines, field_errors)) {
    METLIBS_LOG_ERROR("ERROR, an error occurred while adding new fields:");
    for (const std::string& fe : field_errors)
      METLIBS_LOG_ERROR(fe);
  }

  if (fieldsAdded > 0) {
    fm->updateDialog();
    statusBar()->showMessage(tr("Imported model data to the \"%1\" field group.").arg(filegroup), 2000);
  } else if (symbolsAdded > 0) {
    statusBar()->showMessage(tr("Imported %n symbol(s).", "", symbolsAdded).arg(symbolsAdded), 2000);
  } else if (drawingsAdded > 0) {
    statusBar()->showMessage(tr("Imported %n drawing(s).", "", drawingsAdded).arg(drawingsAdded), 2000);
  }
}

DianaMainWindow *DianaMainWindow::instance()
{
  return DianaMainWindow::self;
}
