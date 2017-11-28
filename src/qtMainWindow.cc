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
#include "qtTimeNavigator.h"

#include "diBuild.h"
#include "diController.h"
#include "diEditItemManager.h"
#include "diLabelPlotCommand.h"
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
#include "util/qstring_util.h"
#include "util/string_util.h"

#include "export/MovieMaker.h"
#include "export/qtExportImageDialog.h"
#include "export/qtDianaDevicePainter.h"

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
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QStringList>
#include <QSvgGenerator>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QWhatsThis>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

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
#include <thumbs_up.xpm>
#include <thumbs_down.xpm>
#include <synop.xpm>
#include <synop_red.xpm>
#include <felt.xpm>
#include <Tool_32_draw.xpm>
#include <sat.xpm>
#include <station.xpm>
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
  , vcrossEditManagerConnected(false)
  , spWindow(0)
  , exportImageDialog_(0)
  , pluginB(0)
  , contr(co)
  , showelem(true)
  , autoselect(false)
{
  METLIBS_LOG_SCOPE();

  setWindowIcon(QIcon(QPixmap(diana_icon_xpm)));

  self = this;

  createHelpDialog();

  //-------- The Actions ---------------------------------

  // file ========================
  // --------------------------------------------------------------------
  fileSavePictAction = new QAction( tr("&Export image/movie..."),this );
  connect( fileSavePictAction, SIGNAL( triggered() ) , SLOT( saveraster() ) );
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

  /*
    ----------------------------------------------------------
    Menu Bar
    ----------------------------------------------------------
   */

  //-------File menu
  filemenu = menuBar()->addMenu(tr("File"));
  filemenu->addAction( fileSavePictAction );
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

  timeNavigator = new TimeNavigator(this);
  connect(timeNavigator, SIGNAL(timeSelected(const miutil::miTime&)),
          this, SLOT(setPlotTime(const miutil::miTime&)));

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
  const int w_margin = 1;
  centralWidget()->layout()->setContentsMargins(w_margin, w_margin, w_margin, w_margin);

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
  mmdock = new QDockWidget(tr("Map and Area"), this);
  mmdock->setObjectName("dock_map");
  mmdock->setWidget(mm);
  addDockWidget(Qt::RightDockWidgetArea, mmdock);
  mmdock->hide();
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
  trajm->hide();
  mainToolbar->addAction( showTrajecDialogAction      );

  annom = new AnnotationDialog(this,contr);
  annom->hide();

  measurementsm = new MeasurementsDialog(this,contr);
  measurementsm->setFocusPolicy(Qt::StrongFocus);
  measurementsm->hide();
  mainToolbar->addAction( showMeasurementsDialogAction   );

  uffm = new UffdaDialog(this,contr);
  uffm->hide();

  EditItems::DrawingDialog *drawingDialog = new EditItems::DrawingDialog(this, contr);
  addDialog(drawingDialog);

  { WebMapDialog* wmd = new WebMapDialog(this, contr);
    addDialog(wmd);

    connect(WebMapManager::instance(), SIGNAL(webMapsReady()),
        this, SLOT(requestBackgroundBufferUpdate()));
  }

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

  connect(uffm, SIGNAL(stationPlotChanged()), SLOT(updateGLSlot()));

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
  connect( mmdock, SIGNAL(visibilityChanged(bool)),  SLOT(mapDockVisibilityChanged(bool)));
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
    connect(vcInterface.get(), &VcrossInterface::quickMenuStrings,
        this, &DianaMainWindow::updateVcrossQuickMenuHistory);
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
          timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(om, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
          timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(sm, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&,bool)),
          timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&,bool)));

  connect(em, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
          timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

  connect(objm, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&,bool)),
          timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&,bool)));

  if (vpWindow) {
    connect(vpWindow, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
            timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

    connect(vpWindow, SIGNAL(setTime(const std::string&, const miutil::miTime&)),
            timeNavigator, SLOT(setTime(const std::string&, const miutil::miTime&)));
  }
  if (vcInterface.get()) {
    connect(vcInterface.get(), SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
        timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));
    
    connect(vcInterface.get(), SIGNAL(setTime(const std::string&, const miutil::miTime&)),
        timeNavigator, SLOT(setTime(const std::string&, const miutil::miTime&)));
  }
  if (spWindow) {
    connect(spWindow, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
            timeNavigator, SLOT(insert(const std::string&, const std::vector<miutil::miTime>&)));

    connect( spWindow ,SIGNAL(setTime(const std::string&, const miutil::miTime&)),
        timeNavigator,SLOT(setTime(const std::string&, const miutil::miTime&)));
  }

  mainToolbar->addSeparator();
  mainToolbar->addAction( showResetAllAction );

  setAcceptDrops(true);

  connect(qApp, SIGNAL(aboutToQuit()),
      SLOT(writeLogFile()));

  METLIBS_LOG_INFO("Creating DianaMainWindow done");
}

void DianaMainWindow::initCoserverClient()
{
  hqcTo = -1;
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

  // strings for each dialog
  PlotCommand_cpv mapcom,obscom,satcom,statcom,objcom,labelcom,fldcom;
  map<std::string, PlotCommand_cpv> dialog_com;
  for (PlotCommand_cp c : vstr) {
    const std::string& pre = c->commandKey();
    if (pre=="MAP") mapcom.push_back(c);
    else if (pre=="AREA") mapcom.push_back(c);
    else if (pre=="FIELD") fldcom.push_back(c);
    else if (pre=="OBS") obscom.push_back(c);
    else if (pre=="SAT") satcom.push_back(c);
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

void DianaMainWindow::getPlotStrings(PlotCommand_cpv &pstr, vector<string> &shortnames)
{
  PlotCommand_cpv diagstr;

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
  const bool remove = (contr->getMapMode() != normal_mode || !timeNavigator->hasTimes());
  diagstr = annom->getOKString();
  for (PlotCommand_cp cmd : diagstr) {
    if (!remove || !isLabelCommandWithTime(cmd))
      pstr.push_back(cmd);
  }

  // Other data sources
  map<QAction*, DataDialog*>::iterator it;
  for (it = dialogs.begin(); it != dialogs.end(); ++it) {
    diagstr = it->second->getOKString();
    pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
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

  diutil::OverrideCursor waitCursor;

  PlotCommand_cpv pstr;
  vector<string> shortnames;

  getPlotStrings(pstr, shortnames);

//init level up/down arrows
  toolLevelUpAction->setEnabled(fm->levelsExists(true,0));
  toolLevelDownAction->setEnabled(fm->levelsExists(false,0));
  toolIdnumUpAction->setEnabled(fm->levelsExists(true,1));
  toolIdnumDownAction->setEnabled(fm->levelsExists(false,1));

  contr->plotCommands(pstr);
  METLIBS_LOG_INFO(contr->getMapArea());
  setPlotTime(timeNavigator->selectedTime());

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

void DianaMainWindow::updateVcrossQuickMenuHistory(const std::string& plotname, const PlotCommand_cpv& pstr)
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

  const miutil::miTime& t = contr->getPlotTime();
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

void DianaMainWindow::applyQuickMenu(const QString& menu, const QString& item)
{
  qm->applyItem(menu.toStdString(), item.toStdString());
  qm->applyPlot();
}

void DianaMainWindow::connectionClosed()
{
  //  METLIBS_LOG_DEBUG("Connection closed");
  qsocket = false;

  contr->stationCommand("delete","all");
  contr->areaObjectsCommand("delete","all", std::vector<std::string>(1, "all"),-1);

  timeNavigator->removeTimes(-1);

  textview->hide();
  contr->processHqcCommand("remove");
  om->setPlottype("Hqc_synop",false);
  om->setPlottype("Hqc_list",false);
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

  else if (command == qmstrings::init_HQC_params) {
    if( contr->initHqcdata(fromId,
        letter.commondesc,
        letter.common,
        letter.description,
        letter.data) ){
      if(letter.common.find("synop") !=std::string::npos )
        om->setPlottype("Hqc_synop",true);
      else
        om->setPlottype("Hqc_list",true);
      hqcTo = fromId;
    }
  }

  else if (command == qmstrings::update_HQC_params){
    contr->updateHqcdata(letter.commondesc,letter.common,
        letter.description,letter.data);
    MenuOK();
  }

  else if (command == qmstrings::select_HQC_param) {
    contr->processHqcCommand("flag",letter.common);
    contr->updatePlots();
  }

  else if (command == qmstrings::station){
    contr->processHqcCommand("station",letter.common);
    contr->updatePlots();
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
    string commondesc = letter.commondesc;
    string common = letter.common;
    string description = letter.description;

    if (letter.description.find(";") != letter.description.npos) {
      vector<string> desc;
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

    //    sendSelectedStations(qmstrings::selectposition);
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
    vector<string> desc;
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

  else if (command == qmstrings::selectarea) {
    //commondesc dataSet
    //description name,on/off
    int n = qletter.countDataRows();
    for (int i=0;i<n;i++) {
      contr->areaObjectsCommand("select", letter.common, diutil::toVector(qletter.getDataValues(i)), fromId);
    }
  }

  else if (command == qmstrings::showarea) {
    //commondesc dataSet
    //description name,on/off
    int n = qletter.countDataRows();
    for (int i=0;i<n;i++) {
      contr->areaObjectsCommand("show", letter.common, diutil::toVector(qletter.getDataValues(i)), fromId);
    }
  }

  else if (command == qmstrings::changearea) {
    //commondesc dataSet
    //description name,colour
    int n = qletter.countDataRows();
    for (int i=0;i<n;i++) {
      contr->areaObjectsCommand("setcolour", letter.common, diutil::toVector(qletter.getDataValues(i)), fromId);
    }
  }

  else if (command == qmstrings::deletearea) {
    //commondesc dataSet
    contr->areaObjectsCommand("delete", letter.common, std::vector<std::string>(1, "all"), fromId);
    updatePlotElements();
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
    //remove observations from hqc
    if (qletter.getCommonValue(0).toLower() == "hqc") {
      contr->processHqcCommand("remove");
      om->setPlottype("Hqc_synop",false);
      om->setPlottype("Hqc_list",false);
      MenuOK();
    }
    updatePlotElements();
  }

  else if (command == qmstrings::newclient) {
    qsocket = true;
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

  else if (command == qmstrings::settime) {
    const int n = qletter.countDataRows();
    const std::string l_common = qletter.getCommonValue(0).toStdString();
    if (qletter.findCommonDesc("datatype") == 0) {
      timeNavigator->useData(l_common, fromId);
      vector<miutil::miTime> times;
      for(int i=0;i<n;i++)
        times.push_back(miutil::miTime(qletter.getDataValue(i, 0).toStdString()));
      timeNavigator->insert(l_common, times);
      contr->initHqcdata(fromId, letter.commondesc,
          l_common, letter.description, letter.data);

    } else if (qletter.findCommonDesc("time") == 0) {
      miutil::miTime t(l_common);
      timeNavigator->setTime(t); // triggers "timeSelected" signal, connected to "setPlotTime"
    }
  }

  else if (command == qmstrings::getcurrentplotcommand) {
    PlotCommand_cpv v1;
    vector<string> v2;
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
      // running animation
      if (timeNavigator->isTimerOn() != 0) {
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
        miutil::miTime tp = timeNavigator->selectedTime();
        timeNavigator->setLastTimeStep();
        miutil::miTime t= timeNavigator->selectedTime();
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
        timeNavigator->stepTimeForward();
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

void DianaMainWindow::setPlotTime(const miutil::miTime& t)
{
  METLIBS_LOG_TIME();
  diutil::OverrideCursor waitCursor;
  contr->setPlotTime(t);
  contr->updatePlots();
  requestBackgroundBufferUpdate();

  //to be done whenever time changes (step back/forward, MenuOK etc.)
  objm->commentUpdate();
  satFileListUpdate();
  if (vpWindow) vpWindow->mainWindowTimeChanged(t);
  if (spWindow) spWindow->mainWindowTimeChanged(t);
  if (vcInterface.get()) vcInterface->mainWindowTimeChanged(t);
  updatePlotElements();

  //update sat channels in statusbar
  vector<string> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);

  if (qsocket) {
    miQMessage letter(qmstrings::timechanged);
    letter.addCommon("time", QString::fromStdString(t.isoTime()));
    sendLetter(letter);
  }

  QCoreApplication::sendPostedEvents ();
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


void DianaMainWindow::saveraster()
{
  METLIBS_LOG_SCOPE();

  if (!exportImageDialog_) {
    exportImageDialog_ = new ExportImageDialog(this);
    connect(w->Glw(), SIGNAL(resized(int,int)), exportImageDialog_, SLOT(onDianaResized(int,int)));
    exportImageDialog_->onDianaResized(w->Glw()->width(), w->Glw()->height());
  }
  exportImageDialog_->show();
}

void DianaMainWindow::saveRasterImage(const QString& filename, const QSize& size)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename.toStdString()));
  QPrinter* printer = 0;
  QImage* image = 0;
  std::unique_ptr<QPaintDevice> device;
  bool printing = false;

  if (filename.endsWith(".pdf")) {
    printer = new QPrinter(QPrinter::ScreenResolution);
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setOutputFileName(filename);
    printer->setFullPage(true);
    printer->setPaperSize(size, QPrinter::DevicePixel);

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
    generator->setViewBox(QRect(QPoint(0,0), size));
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
    image = new QImage(size, QImage::Format_ARGB32_Premultiplied);
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
  DianaImageExporter ex(w->Glw(), device, printing);
  ex.paintOnDevice();
}

void DianaMainWindow::saveAnimation(MovieMaker& moviemaker)
{
  QMessageBox::information(this, tr("Making animation"),
      tr("This may take some time, depending on the number of timesteps and selected delay."
          " Diana cannot be used until this process is completed."
          " A message will be displayed upon completion."
          " Press OK to begin."));

  w->setVisible(false); // avoid handling repaint events while the canvas is replaced

  const miutil::miTime oldTime = timeNavigator->selectedTime();
  const std::vector<miutil::miTime> times = timeNavigator->animationTimes();

  QProgressDialog progress(tr("Creating animation..."), tr("Hide"),
      0, times.size(), this);
  progress.setWindowModality(Qt::WindowModal);
  progress.show();

  bool ok = true;

  QImage image(moviemaker.frameSize(), QImage::Format_ARGB32_Premultiplied);
  { // scope for DianaImageExporter
    DianaImageExporter ex(w->Glw(), &image, false);

    // save frames as images
    for (size_t step = 0; ok && step < times.size(); ++step) {
      if (!progress.isHidden())
        progress.setValue(step);

      setPlotTime(times[step]);
      image.fill(Qt::transparent);
      ex.paintOnDevice();

      ok = moviemaker.addImage(image);
    }
    if (ok)
      ok = moviemaker.finish();
  } // destroy DianaImageExporter, resets canvas
  setPlotTime(oldTime);

  w->setVisible(true);

  if (ok)
    QMessageBox::information(this, tr("Done"), tr("Animation completed."));
  else
    QMessageBox::warning(this, tr("Error"), tr("Problem with creating animation."));
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

  if (mev->modifiers() & Qt::ControlModifier){
    if (uffda && contr->getSatnames().size()){
      showUffda();
    }
  }
  //send position to all clients
  if(qsocket){
    miQMessage letter(qmstrings::positions);
    letter.addCommon("dataset", "diana");
    letter.addDataDesc("lat").addDataDesc("lon");
    letter.addDataValues(QStringList() << QString::number(lat) << QString::number(lon));
    sendLetter(letter);
  }

  QString popupText = contr->getStationManager()->getStationsText(x, y);
  if (popupText.isEmpty()) {
    popupText = QString::fromStdString(contr->getObsPopupText(x, y));
  }
  if (!popupText.isEmpty()) {
    // undo reverted y coordinate from GLwidget::handleMouseEvents
    QPoint popupPos = w->mapToGlobal(QPoint(x, w->height() - y));
    QWhatsThis::showText(popupPos, popupText, this);
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

  vselectAreas=contr->findAreaObjects(xclick,yclick);
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
    vcInterface->changeCrossection(QString::fromStdString(crossection));
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

    //send area to plugin connected
    vector <selectArea> areas=contr->findAreaObjects(x,y,true);
    int nareas = areas.size();
    if (nareas) {
      miQMessage letter(qmstrings::selectarea);
      const QStringList dataColumns = QStringList() << "name" << "on/off";
      for(int i=0;i<nareas;i++){
        const QStringList dataRow = QStringList() << QString::fromStdString(areas[i].name) << "on";
        letter.setData(dataColumns, QList<QStringList>() << dataRow);
        sendLetter(letter, areas[i].id);
      }
    }

    if (hqcTo > 0) {
      std::string name;
      if (contr->getObsName(x,y,name)) {
        const miutil::miTime& t = contr->getPlotTime();

        QStringList cd = QStringList() << "name" << "time";
        QStringList cv = QStringList() << QString::fromStdString(name) << QString::fromStdString(t.isoTime());

        miQMessage letter(qmstrings::station);
        letter.setCommon(QStringList() << cd.join(","), QStringList() << cv.join(","));
        // new version, to be enabled later: letter.setCommon(cd, cv);
        sendLetter(letter, hqcTo);
      }
    }
  }

  if (needupdate)
    requestBackgroundBufferUpdate();
}

void DianaMainWindow::sendSelectedStations(const std::string& command)
{
  vector< vector<std::string> > data;
  contr->getStationData(data);
  // data contains name, name, ..., name, dataset, id (as string) where "name" are selected station names

  const int n=data.size();
  for (int i=0;i<n;i++) {
    const vector<string>& token = data[i];
    const int m = token.size();
    if (m < 2)
      continue;

    const int id = miutil::to_int(token[m-1]);
    const std::string& dataset = token[m-2];

    QStringList desc, values;
    for (int j=0; j<m-2; ++j) { // skip dataset and id
      desc << "name";
      values << QString::fromStdString(token[j]);
    }

    miQMessage letter(QString::fromStdString(command));
    letter.addCommon("dataset", QString::fromStdString(dataset));
    letter.setData(desc, QList<QStringList>() << values);
    sendLetter(letter, id);
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

void DianaMainWindow::restoreDocState(const std::string& logstr)
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
  if(on){
    archiveL->show();
  }else{
    archiveL->hide();
  }
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
  w->Glw()->setUseScrollwheelZoom(on);
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

  const int ia = action->data().toInt();
  const selectArea& a = vselectAreas[ia];

  miQMessage letter(qmstrings::selectarea);
  letter.addDataDesc("name").addDataDesc("on/off");
  letter.addDataValues(QStringList() << QString::fromStdString(a.name) << (a.selected ? "off" : "on"));
  sendLetter(letter, a.id);
}


void DianaMainWindow::inEdit(bool inedit)
{
  if (qsocket) {
    miQMessage letter(qmstrings::editmode);
    letter.addCommon("on/off", inedit ? "on" : "off");
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
    return false;
  }

  return QMainWindow::event(event);
}

void DianaMainWindow::addDialog(DataDialog *dialog)
{
  dialog->hide();

  dialogNames[dialog->name()] = dialog;
  connect(dialog, SIGNAL(applyData()), SLOT(MenuOK()));
  connect(dialog, SIGNAL(emitTimes(const std::string &, const std::vector<miutil::miTime> &)),
      timeNavigator, SLOT(insert(const std::string &, const std::vector<miutil::miTime> &)));
  connect(dialog, SIGNAL(emitTimes(const std::string &, const std::vector<miutil::miTime> &, bool)),
      timeNavigator, SLOT(insert(const std::string &, const std::vector<miutil::miTime> &, bool)));
  connect(dialog, SIGNAL(updated()), this, SLOT(requestBackgroundBufferUpdate()));

  if (QAction *action = dialog->action()) {
    dialogs[action] = dialog;
    connect(action, SIGNAL(toggled(bool)), dialog, SLOT(setVisible(bool)));
    connect(action, SIGNAL(toggled(bool)), this, SLOT(requestBackgroundBufferUpdate()));
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
    Q_FOREACH(QUrl url, event->mimeData()->urls()) {

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
