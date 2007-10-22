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

#include <fstream>

#include <qfiledialog.h>
#include <qprinter.h>
#include <qaction.h>
#include <qtTimeSlider.h>
#include <qtTimeControl.h>
#include <qtTimeStepSpinbox.h>
#include <qtStatusGeopos.h>
#include <qtStatusPlotButtons.h>
#include <qtShowSatValues.h>
#include <qtTextDialog.h>
#include <qtImageGallery.h>
#include <qapplication.h>
#include <glob.h>
#include <qmotifstyle.h>
#include <qwindowsstyle.h>

#include <qpushbutton.h>
#include <qpixmap.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qkeycode.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qaccel.h>
#include <qlabel.h>
#include <qfontdialog.h>
#include <qtooltip.h>

#include <qtMainWindow.h>
#include <qtWorkArea.h>
#include <qtVprofWindow.h>
#include <qtVcrossWindow.h>
#include <qtSpectrumWindow.h>
#include <diController.h>
#include <diPrintOptions.h>
#include <diSetupParser.h>
#include <diStationPlot.h>
#include <diLocationPlot.h>

#include <qtQuickMenu.h>
#include <qtObsDialog.h>
#include <qtSatDialog.h>
#include <qtMapDialog.h>
#include <qtFieldDialog.h>
#include <qtEditDialog.h>
#include <qtObjectDialog.h>
#include <qtTrajectoryDialog.h>
#include <qtHelpDialog.h>
#include <qtEditTimeDialog.h>
#include <qtPrintManager.h>
#include <qtBrowserBox.h>
#include <qtAddtoMenu.h>
#include <qtUffdaDialog.h>
#include <qtClientButton.h>
#include <qtTextView.h>
#include <miMessage.h>
#include <QLetterCommands.h>
#include <miCoordinates.h>
#include <miCommandLine.h>
#include <qtPaintToolBar.h>
#include <diGridAreaManager.h>

#include <pick.xpm>
#include <earth3.xpm>
#include <fileprint.xpm>
#include <question.xpm>
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
#include <info.xpm>
#include <profet.xpm>
#include <paint_mode.xpm>


DianaMainWindow::DianaMainWindow(Controller *co, 
				 const miString ver_str, 
				 const miString build_str, 
				 bool ep)
  : QMainWindow( 0, "DIANA main window"),
    contr(co),timeron(0),timeloop(false),timeout_ms(100),
    showelem(true),autoselect(false),push_command(true),browsing(false),
    markTrajPos(false),
    vpWindow(0), vcWindow(0), spWindow(0), enableProfet(ep)
{

  version_string= ver_str;
  build_string  = build_str;

  setCaption( tr("Diana") );

  menuBar()->hide();// The menubar must be hidden to let the sentralwidget
                    // fill all the space

  SetupParser setup;
  

  /*
    The Actions
  */
  // file ========================
  // --------------------------------------------------------------------
  fileSavePictAction = new QAction( QPixmap( ),tr("&Save picture..."),QKeySequence(),this );
  connect( fileSavePictAction, SIGNAL( activated() ) , this, SLOT( saveraster() ) );
  // --------------------------------------------------------------------
  filePrintAction = new QAction( QPixmap( fileprint_xpm ),tr("&Print..."),CTRL+Key_P,this );
  connect( filePrintAction, SIGNAL( activated() ) , this, SLOT( hardcopy() ) );
  // --------------------------------------------------------------------
  fileQuitAction = new QAction( QPixmap( ),tr("&Quit..."),CTRL+Key_Q, this );
  connect( fileQuitAction, SIGNAL( activated() ) , this, SLOT( filequit() ) );


  // options ======================
  // --------------------------------------------------------------------
  optXYAction = new QAction( QPixmap( ),tr("&X,Y positions"),QKeySequence(), this );
  optXYAction->setToggleAction(true);
  connect( optXYAction, SIGNAL( activated() ) , this, SLOT( xyGeoPos() ) );
  // --------------------------------------------------------------------
  optLatLonAction = new QAction( QPixmap( ),
				 tr("Lat/Lon in decimal degrees"),QKeySequence(), this );
  optLatLonAction->setToggleAction(true);
  connect( optLatLonAction, SIGNAL( activated() ) , this, SLOT( latlonGeoPos() ) );
  // --------------------------------------------------------------------
  optOnOffAction = new QAction( QPixmap( ),tr("S&peed buttons"),QKeySequence(), this );
  optOnOffAction->setToggleAction(true);
  connect( optOnOffAction, SIGNAL( activated() ) , this, SLOT( showElements() ) );
  // --------------------------------------------------------------------
  optArchiveAction = new QAction( QPixmap( ),tr("A&rchive mode"),QKeySequence(), this );
  optArchiveAction->setToggleAction(true);
  connect( optArchiveAction, SIGNAL( activated() ) , this, SLOT( archiveMode() ) );
  // --------------------------------------------------------------------
  optAutoElementAction = new QAction( QPixmap( ),tr("&Automatic element choice"),Key_Space, this );
  optAutoElementAction->setToggleAction(true);
  connect( optAutoElementAction, SIGNAL( activated() ) , this, SLOT( autoElement() ) );
  // --------------------------------------------------------------------
  optAnnotationAction = new QAction( QPixmap( ),tr("A&nnotations"),QKeySequence(), this );
  optAnnotationAction->setToggleAction(true);
  connect( optAnnotationAction, SIGNAL( activated() ) , this, SLOT( showAnnotations() ) );
  // --------------------------------------------------------------------
  optFontAction = new QAction( QPixmap( ),tr("Select &Font..."),QKeySequence(), this );
  connect( optFontAction, SIGNAL( activated() ) , this, SLOT( chooseFont() ) );


  // show ======================
  // --------------------------------------------------------------------
  showResetAreaAction = new QAction( QPixmap(thumbs_up_xpm),tr("Reset area and replot"),QKeySequence(), this );
  connect( showResetAreaAction, SIGNAL( activated() ) , this, SLOT( resetArea() ) );
  //----------------------------------------------------------------------
  showResetAllAction = new QAction( QPixmap(thumbs_down_xpm),tr("Reset all"),QKeySequence(), this );
  connect( showResetAllAction, SIGNAL( activated() ) , this, SLOT( resetAll() ) );
  // --------------------------------------------------------------------
  showApplyAction = new QAction( QPixmap( ),tr("&Apply plot"),CTRL+Key_U, this );
  connect( showApplyAction, SIGNAL( activated() ) , this, SLOT( MenuOK() ) );
  // --------------------------------------------------------------------
  showAddQuickAction = new QAction( QPixmap( ),tr("Add to q&uickmenu"),Key_F9, this );
  connect( showAddQuickAction, SIGNAL( activated() ) , this, SLOT( addToMenu() ) );
  // --------------------------------------------------------------------
  showPrevPlotAction = new QAction( QPixmap( ),tr("P&revious plot"),Key_F10, this );
  connect( showPrevPlotAction, SIGNAL( activated() ) , this, SLOT( prevHPlot() ) );
  // --------------------------------------------------------------------
  showNextPlotAction = new QAction( QPixmap( ),tr("&Next plot"),Key_F11, this );
  connect( showNextPlotAction, SIGNAL( activated() ) , this, SLOT( nextHPlot() ) );
  // --------------------------------------------------------------------
  showHideAllAction = new QAction( QPixmap( ),tr("&Hide All"),CTRL+Key_D, this );
  showHideAllAction->setToggleAction(true);
  connect( showHideAllAction, SIGNAL( activated() ) , this, SLOT( toggleDialogs() ) );

  // --------------------------------------------------------------------
  showQuickmenuAction = new QAction( QPixmap(pick_xpm ),tr("&Quickmenu"),Key_F12, this );
  showQuickmenuAction->setToggleAction(true);
  connect( showQuickmenuAction, SIGNAL( activated() ) , this, SLOT( quickMenu() ) );
  // --------------------------------------------------------------------
  showMapDialogAction = new QAction( QPixmap(earth3_xpm ),tr("&Maps"),ALT+Key_K, this );
  showMapDialogAction->setToggleAction(true);
  connect( showMapDialogAction, SIGNAL( activated() ) , this, SLOT( mapMenu() ) );
  // --------------------------------------------------------------------
  showFieldDialogAction = new QAction( QPixmap(felt_xpm ),tr("&Fields"),ALT+Key_F, this );
  showFieldDialogAction->setToggleAction(true);
  connect( showFieldDialogAction, SIGNAL( activated() ) , this, SLOT( fieldMenu() ) );
  // --------------------------------------------------------------------
  showObsDialogAction = new QAction( QPixmap(synop_xpm ),tr("&Observations"),ALT+Key_O, this );
  showObsDialogAction->setToggleAction(true);
  connect( showObsDialogAction, SIGNAL( activated() ) , this, SLOT( obsMenu() ) );
  // --------------------------------------------------------------------
  showSatDialogAction = new QAction( QPixmap(sat_xpm ),tr("&Satellites and Radar"),ALT+Key_S, this );
  showSatDialogAction->setToggleAction(true);
  connect( showSatDialogAction, SIGNAL( activated() ) , this, SLOT( satMenu() ) );
  // --------------------------------------------------------------------
  showEditDialogAction = new QAction( QPixmap(editmode_xpm ),tr("&Product Editing"),ALT+Key_E, this );
  showEditDialogAction->setToggleAction(true);
  connect( showEditDialogAction, SIGNAL( activated() ) , this, SLOT( editMenu() ) );
  // --------------------------------------------------------------------
  showObjectDialogAction = new QAction( QPixmap(front_xpm ),tr("O&bjects"),ALT+Key_J, this );
  showObjectDialogAction->setToggleAction(true);
  connect( showObjectDialogAction, SIGNAL( activated() ) , this, SLOT( objMenu() ) );
  // --------------------------------------------------------------------
  showTrajecDialogAction = new QAction( QPixmap( traj_xpm),tr("&Trajectories"),ALT+Key_T, this );
  showTrajecDialogAction->setToggleAction(true);
  connect( showTrajecDialogAction, SIGNAL( activated() ) , this, SLOT( trajMenu() ) );
  // --------------------------------------------------------------------
  showProfilesDialogAction = new QAction( QPixmap(balloon_xpm ),tr("&Vertical Profiles"),ALT+Key_V, this );
  showProfilesDialogAction->setToggleAction(false);
  connect( showProfilesDialogAction, SIGNAL( activated() ) , this, SLOT( vprofMenu() ) );
  // --------------------------------------------------------------------
  showCrossSectionDialogAction = new QAction( QPixmap(vcross_xpm ),tr("Vertical &Cross sections"),ALT+Key_C, this );
  showCrossSectionDialogAction->setToggleAction(false);
  connect( showCrossSectionDialogAction, SIGNAL( activated() ) , this, SLOT( vcrossMenu() ) );
  // --------------------------------------------------------------------
  showWaveSpectrumDialogAction = new QAction( QPixmap(spectrum_xpm ),tr("&Wave spectra"),ALT+Key_W, this );
  showWaveSpectrumDialogAction->setToggleAction(false);
  connect( showWaveSpectrumDialogAction, SIGNAL( activated() ) , this, SLOT( spectrumMenu() ) );
  // --------------------------------------------------------------------
  showUffdaDialogAction = new QAction( QPixmap( ),tr("&Uffda Service"),ALT+Key_U, this );
  showUffdaDialogAction->setToggleAction(true);
  connect( showUffdaDialogAction, SIGNAL( activated() ) , this, SLOT( uffMenu() ) );
  // ----------------------------------------------------------------
  if(enableProfet) togglePaintModeAction = new QAction( QPixmap(paint_mode_xpm),tr("&Paint"),ALT+Key_P, this );
  else togglePaintModeAction = new QAction( QPixmap( ),tr("&Paint"),QKeySequence(), this );
  togglePaintModeAction->setToggleAction(true);
  connect( togglePaintModeAction, SIGNAL( toggled(bool) ) , this, SLOT( togglePaintMode() ) );
  // ----------------------------------------------------------------
  if(enableProfet) {showEditTimeDialogAction = 
    new QAction( QPixmap(profet_xpm ),tr("&EditTime"),QKeySequence(), this );
  } else {
    showEditTimeDialogAction = 
      new QAction( QPixmap(),tr("&EditTime"),QKeySequence(), this );
  }
  showEditTimeDialogAction->setToggleAction(true);
  connect( showEditTimeDialogAction, SIGNAL( activated() ) ,  this, SLOT( toggleEditTimeDialog() ) );
  // connect( showEditTimeDialogAction, SIGNAL( activated() ) ,  this, SLOT( togglePaintMode()      ) );
  // --------------------------------------------------------------------


  // help ======================
  // --------------------------------------------------------------------
  helpDocAction = new QAction( QPixmap( ),tr("&Documentation"),Key_F1, this );
  helpDocAction->setToggleAction(false);
  connect( helpDocAction, SIGNAL( activated() ) , this, SLOT( showHelp() ) );
  // --------------------------------------------------------------------
  helpAccelAction = new QAction( QPixmap( ),tr("&Accelerators"),QKeySequence(), this );
  helpAccelAction->setToggleAction(false);
  connect( helpAccelAction, SIGNAL( activated() ) , this, SLOT( showAccels() ) );
  // --------------------------------------------------------------------
  helpNewsAction = new QAction( QPixmap( ),tr("&News"),QKeySequence(), this );
  helpNewsAction->setToggleAction(false);
  connect( helpNewsAction, SIGNAL( activated() ) , this, SLOT( showNews() ) );
  // --------------------------------------------------------------------
  helpAboutAction = new QAction( QPixmap( ),tr("About Diana"),QKeySequence(), this );
  helpAboutAction->setToggleAction(false);
  connect( helpAboutAction, SIGNAL( activated() ) , this, SLOT( about() ) );
  // --------------------------------------------------------------------


  // timecommands ======================
  // --------------------------------------------------------------------
  timeBackwardAction = new QAction( QPixmap(start_xpm ),tr("Run Backwards"),SHIFT+CTRL+Key_Left, this );
  timeBackwardAction->setToggleAction(true);
  connect( timeBackwardAction, SIGNAL( activated() ) , this, SLOT( animationBack() ) );
  // --------------------------------------------------------------------
  timeForewardAction = new QAction( QPixmap(slutt_xpm ),tr("Run Forewards"),SHIFT+CTRL+Key_Right, this );
  timeForewardAction->setToggleAction(true);
  connect( timeForewardAction, SIGNAL( activated() ) , this, SLOT( animation() ) );
  // --------------------------------------------------------------------
  timeStepBackwardAction = new QAction( QPixmap(bakover_xpm ),tr("Step Backwards"),CTRL+Key_Left, this );
  timeStepBackwardAction->setToggleAction(false);
  connect( timeStepBackwardAction, SIGNAL( activated() ) , this, SLOT( stepback() ) );
  // --------------------------------------------------------------------
  timeStepForewardAction = new QAction( QPixmap(forward_xpm ),tr("Step Forewards"),CTRL+Key_Right, this );
  timeStepForewardAction->setToggleAction(false);
  connect( timeStepForewardAction, SIGNAL( activated() ) , this, SLOT( stepforward() ) );
  // --------------------------------------------------------------------
  timeStopAction = new QAction( QPixmap(stop_xpm ),tr("Stop"),SHIFT+CTRL+Key_Down, this );
  timeStopAction->setToggleAction(false);
  connect( timeStopAction, SIGNAL( activated() ) , this, SLOT( animationStop() ) );
  // --------------------------------------------------------------------
  timeLoopAction = new QAction( QPixmap(loop_xpm ),tr("Run in loop"),SHIFT+CTRL+Key_Up, this );
  timeLoopAction->setToggleAction(true);
  connect( timeLoopAction, SIGNAL( activated() ) , this, SLOT( animationLoop() ) );
  // --------------------------------------------------------------------
  timeControlAction = new QAction( QPixmap( clock_xpm ),tr("Time control"),QKeySequence(), this );
  timeControlAction->setToggleAction(true);
  timeControlAction->setEnabled(false);
  connect( timeControlAction, SIGNAL( activated() ) , this, SLOT( timecontrolslot() ) );
  // --------------------------------------------------------------------


  // other tools ======================
  // --------------------------------------------------------------------
  toolLevelUpAction = new QAction( QPixmap(levelUp_xpm ),tr("Level up"),CTRL+Key_PageUp, this );
  toolLevelUpAction->setToggleAction(false);
  toolLevelUpAction->setEnabled ( false );
  connect( toolLevelUpAction, SIGNAL( activated() ) , this, SLOT( levelUp() ) );
  // --------------------------------------------------------------------
  toolLevelDownAction = new QAction( QPixmap(levelDown_xpm ),tr("Level down"),CTRL+Key_PageDown, this );
  toolLevelDownAction->setToggleAction(false);
  toolLevelDownAction->setEnabled ( false );
  connect( toolLevelDownAction, SIGNAL( activated() ) , this, SLOT( levelDown() ) );
  // --------------------------------------------------------------------
  toolIdnumUpAction = new QAction( QPixmap(idnumUp_xpm ),
				   tr("EPS cluster/member etc up"),SHIFT+Key_PageUp, this );
  toolIdnumUpAction->setToggleAction(false);
  toolIdnumUpAction->setEnabled ( false );
  connect( toolIdnumUpAction, SIGNAL( activated() ) , this, SLOT( idnumUp() ) );
  // --------------------------------------------------------------------
  toolIdnumDownAction = new QAction( QPixmap(idnumDown_xpm ),
				     tr("EPS cluster/member etc down"),SHIFT+Key_PageDown, this );
  toolIdnumDownAction->setToggleAction(false);
  toolIdnumDownAction->setEnabled ( false );
  connect( toolIdnumDownAction, SIGNAL( activated() ) , this, SLOT( idnumDown() ) );
  // --------------------------------------------------------------------


  /*
    ----------------------------------------------------------
    Menu Bar
    ----------------------------------------------------------
  */

  menuToolbar = new QToolBar( this );
  addDockWindow( menuToolbar, tr( "Menuline" ), Top, FALSE );

  mainmenubar = new QMenuBar( menuToolbar );
  mainmenubar->setFrameStyle( QFrame::NoFrame );

  menuToolbar->setStretchableWidget( mainmenubar );
  setDockEnabled( menuToolbar, Left, FALSE );
  setDockEnabled( menuToolbar, Right, FALSE );


  //-------File menu
  filemenu = new QPopupMenu(this);
  mainmenubar->insertItem( tr("&File"), filemenu );
  fileSavePictAction->     addTo( filemenu );
  filePrintAction->        addTo( filemenu );
  filemenu->insertSeparator();
  fileQuitAction->         addTo( filemenu );

  //-------Options menu
  optmenu = new QPopupMenu(this);
  optmenu->setCheckable(true);
  mainmenubar->insertItem( tr("O&ptions"),optmenu );
  optXYAction ->           addTo( optmenu );
  optLatLonAction ->       addTo( optmenu );
  optOnOffAction ->        addTo( optmenu );
  optArchiveAction ->      addTo( optmenu );
  optAutoElementAction ->  addTo( optmenu );
  optAnnotationAction ->   addTo( optmenu );
  optmenu->insertSeparator();
  optFontAction ->         addTo( optmenu );

  optOnOffAction->setOn( showelem );
  optAutoElementAction->setOn( autoselect );
  optAnnotationAction->setOn( true );


  rightclickmenu = new QPopupMenu(this);
  connect(rightclickmenu,SIGNAL(aboutToShow()),this,
	  SLOT(fillRightclickmenu()));
  connect(rightclickmenu,SIGNAL(activated(int)),this,
	  SLOT(rightClickMenuActivated(int)));
  lastRightClicked= tr("Zoom out");
  uffda=contr->getUffdaEnabled();


  //-------Show menu
  showmenu = new QPopupMenu(this);
  showmenu->setCheckable(true);
  mainmenubar->insertItem( tr("&Show"), showmenu );

  showApplyAction->              addTo( showmenu );
  showAddQuickAction->           addTo( showmenu );
  showPrevPlotAction->           addTo( showmenu );
  showNextPlotAction->           addTo( showmenu );
  showmenu->insertSeparator();
  showHideAllAction->            addTo( showmenu );
  showQuickmenuAction->          addTo( showmenu );
  showMapDialogAction->          addTo( showmenu );
  showFieldDialogAction->        addTo( showmenu );
  showObsDialogAction->          addTo( showmenu );
  showSatDialogAction->          addTo( showmenu );
  showEditDialogAction->         addTo( showmenu );
  showObjectDialogAction->       addTo( showmenu );
  showTrajecDialogAction->       addTo( showmenu );
  showProfilesDialogAction->     addTo( showmenu );
  showCrossSectionDialogAction-> addTo( showmenu );
  showWaveSpectrumDialogAction-> addTo( showmenu );
  if(enableProfet)
    showEditTimeDialogAction->	 addTo( showmenu );

  if (uffda)
    showUffdaDialogAction->      addTo( showmenu );
//  if(enableProfet)
//    togglePaintModeAction->		 addTo( showmenu );

  //-------Help menu
  helpmenu = new QPopupMenu(this);
  mainmenubar->insertItem( tr("&Help"), helpmenu );
  helpDocAction->        addTo ( helpmenu );
  helpmenu->insertSeparator();
  helpAccelAction->      addTo ( helpmenu );
  helpNewsAction->       addTo ( helpmenu );
  helpmenu->insertSeparator();
  helpAboutAction->      addTo ( helpmenu );


  /*
    ----------------------------------------------------------
    Tool Bars
    ----------------------------------------------------------
  */

  mainToolbar = new QToolBar(tr("Dataaccess/Tools"), this,
			     QMainWindow::Right, FALSE,
			     "maintoolbar");

  timerToolbar= new QToolBar(tr("Time control"), this,
			     QMainWindow::Top, FALSE,
			     "timertoolbar");

  setDockEnabled( timerToolbar, Left, FALSE );
  setDockEnabled( timerToolbar, Right, FALSE );

  /**************** Toolbar Buttons *********************************************/

  showResetAreaAction-> addTo( mainToolbar );

  mainToolbar->addSeparator();

  showQuickmenuAction->          addTo( mainToolbar );
  showMapDialogAction->          addTo( mainToolbar );
  showFieldDialogAction->        addTo( mainToolbar );
  showObsDialogAction->          addTo( mainToolbar );
  showSatDialogAction->          addTo( mainToolbar );
  showObjectDialogAction->       addTo( mainToolbar );
  showTrajecDialogAction->       addTo( mainToolbar );
  showProfilesDialogAction->     addTo( mainToolbar );
  showCrossSectionDialogAction-> addTo( mainToolbar );
  showWaveSpectrumDialogAction-> addTo( mainToolbar );
  if(enableProfet)
    showEditTimeDialogAction->     addTo( mainToolbar );
//  if(enableProfet)
//    togglePaintModeAction->        addTo( mainToolbar );

  infoB = new QToolButton(QPixmap(info_xpm),
			  tr("Information..."),
			  "",
			  0, 0,
			  mainToolbar,
			  tr("show information...") );

  QPopupMenu* infomenu= new QPopupMenu(mainToolbar, "infomenu");
  connect(infomenu, SIGNAL(activated(int)),
	  this, SLOT(info_activated(int)));
  infoFiles= contr->getInfoFiles();
  if (infoFiles.size()>0){
    for (int j=0; j<infoFiles.size(); j++){
      infomenu->insertItem(infoFiles[j].name.cStr(),j);
    }
  }
  infoB->setPopupDelay(100);
  infoB->setPopup(infomenu);
  infoB->setAccel(ALT+Key_N);

  mainToolbar->addSeparator();

  showEditDialogAction->  addTo( mainToolbar );

  mainToolbar->addSeparator();

  showResetAllAction-> addTo( mainToolbar );
  // ---------------------------------------------------

  timeBackwardAction->     addTo( timerToolbar );
  timeStepBackwardAction-> addTo( timerToolbar );
  timeStepForewardAction-> addTo( timerToolbar );
  timeForewardAction->     addTo( timerToolbar );
  timeStopAction->         addTo( timerToolbar );
  timeLoopAction->         addTo( timerToolbar );

  timerToolbar->addSeparator();

  // ---------------------------------------------------
  tslider= new TimeSlider(QSlider::Horizontal,timerToolbar,"tslider");
  tslider->setMinimumWidth(150);
  connect(tslider,SIGNAL(valueChanged(int)),SLOT(TimeChanged()));
  connect(tslider,SIGNAL(sliderReleased()),SLOT(TimeSelected()));
  connect(tslider,SIGNAL(sliderSet()),SLOT(SliderSet()));

  timerToolbar->addSeparator();
  timestep= new TimeStepSpinbox(timerToolbar, "timestep");
  connect(tslider,SIGNAL(minInterval(int)),
	  timestep,SLOT(setValue(int)));
  connect(tslider,SIGNAL(timeSteps(int,int)),
	  timestep,SLOT(setTimeSteps(int,int)));
  connect(tslider,SIGNAL(enableSpin(bool)),
	  timestep,SLOT(setEnabled(bool)));
  connect(timestep,SIGNAL(valueChanged(int)),
	  tslider,SLOT(setInterval(int)));

  // Accelerator for time, timestep and level
  QAccel* taccel = new QAccel(this);
  taccel->connectItem( taccel->insertItem( CTRL+Key_Down ), this,
		       SLOT(decreaseTimeStep()) );
  taccel->connectItem( taccel->insertItem( CTRL+Key_Up ), this,
		       SLOT(increaseTimeStep()) );

  timerToolbar->addSeparator();

  timelabel= new QLabel("0000-00-00 00:00:00",timerToolbar,"timelabel");
  timelabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timelabel->setMinimumSize(timelabel->sizeHint());

  timerToolbar->addSeparator();

  timeControlAction->   addTo( timerToolbar );

  connect(tslider,SIGNAL(enableSpin(bool)),
	  timeControlAction,SLOT(setEnabled(bool)));

  timecontrol = new TimeControl(this);
  connect(timecontrol, SIGNAL(timeoutChanged(float)),
	  SLOT(timeoutChanged(float)));
  connect(timecontrol, SIGNAL(minmaxValue(const miTime&, const miTime&)),
	  tslider, SLOT(setMinMax(const miTime&, const miTime&)));
  connect(timecontrol, SIGNAL(clearMinMax()),
	  tslider, SLOT(clearMinMax()));
  connect(tslider, SIGNAL(newTimes(vector<miTime>&)),
	  timecontrol, SLOT(setTimes(vector<miTime>&)));
  connect(timecontrol, SIGNAL(data(miString)),
	  tslider, SLOT(useData(miString)));
  connect(timecontrol, SIGNAL(timecontrolHide()),
	   SLOT(timecontrolslot()));

  timerToolbar->addSeparator();

  toolLevelUpAction->   addTo( timerToolbar );
  toolLevelDownAction-> addTo( timerToolbar );

  timerToolbar->addSeparator();

  toolIdnumUpAction->   addTo( timerToolbar );
  toolIdnumDownAction-> addTo( timerToolbar );


  /***********************************************************/
  /****************** Status bar *****************************/

  statusbuttons= new StatusPlotButtons(statusBar(),"statusbuttons");
  connect(statusbuttons, SIGNAL(toggleElement(PlotElement)),
	  this, SLOT(toggleElement(PlotElement)));
  statusBar()->addWidget(statusbuttons,1,true);
  if (!showelem) statusbuttons->hide();


  showsatval = new ShowSatValues(statusBar(),"showsatval");
  statusBar()->addWidget(showsatval,0,true);

  sgeopos= new StatusGeopos(statusBar(),"sgeopos");
  statusBar()->addWidget(sgeopos,0,true);

  obsUpdateB = new QPushButton(QPixmap(synop_red_xpm),
			       "",
			       statusBar());
  int hpixbutton=  obsUpdateB->sizeHint().height();
  obsUpdateB->setMinimumWidth( hpixbutton );
  obsUpdateB->setMaximumWidth( hpixbutton );
  QToolTip::add(obsUpdateB,tr("Update observations"));

  connect(obsUpdateB, SIGNAL(clicked()),SLOT(updateObs()));
  statusBar()->addWidget(obsUpdateB,0,true);

  archiveL = new QLabel(tr("ARCHIVE"),statusBar());
  statusBar()->addWidget(archiveL,0,true);
  archiveL->setPaletteBackgroundColor("red");
  archiveL->hide();

  hqcTo = -1;
  qsocket = false;
  miString server = setup.basicValue("qserver");
  pluginB = new ClientButton(tr("Diana"),server.c_str(),statusBar());
  pluginB->setMinimumWidth( hpixbutton );
  pluginB->setMaximumWidth( hpixbutton );
  connect(pluginB, SIGNAL(receivedLetter(miMessage&)),
	  SLOT(processLetter(miMessage&)));
  connect(pluginB, SIGNAL(connectionClosed()),SLOT(connectionClosed()));
  statusBar()->addWidget(pluginB,0,true);

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

  //-------------------------------------------------

  w= new WorkArea(contr,this,"WORKAREA");
  setCentralWidget(w);

  connect(w->Glw(), SIGNAL(mouseGridPos(const mouseEvent)),
	  SLOT(catchMouseGridPos(const mouseEvent)));

  connect(w->Glw(), SIGNAL(mouseRightPos(const mouseEvent)),
	  SLOT(catchMouseRightPos(const mouseEvent)));

  connect(w->Glw(), SIGNAL(mouseMovePos(const mouseEvent,bool)),
	  SLOT(catchMouseMovePos(const mouseEvent,bool)));

  connect(w->Glw(), SIGNAL(keyPress(const keyboardEvent)),
	  SLOT(catchKeyPress(const keyboardEvent)));

  connect(w->Glw(), SIGNAL(gridAreaChanged()),
	  SLOT(catchGridAreaChanged()));
  

  // ----------- init dialog-objects -------------------

  qm= new QuickMenu(this, contr, "QuickMenu");
  qm->hide();

  fm= new FieldDialog(this, contr);
  fm->hide();

  om= new ObsDialog(this, contr);
  om->hide();

  sm= new SatDialog(this, contr);
  sm->hide();

  mm= new MapDialog(this, contr);
  mm->hide();

  em= new EditDialog( this, contr );
  em->hide();

  objm = new ObjectDialog(this,contr);
  objm->hide();

  trajm = new TrajectoryDialog(this,contr);
  trajm->hide();

  editTimeDialog= new EditTimeDialog(this, contr->getGridEditManager());
  editTimeDialog->hide();
  connect( editTimeDialog, SIGNAL( updateGL() ) , SLOT( updateGLSlot() ) );
  connect( editTimeDialog, SIGNAL(gridAreaChanged()), 
	   SLOT(catchGridAreaChanged()));
  uffm = new UffdaDialog(this,contr);
  uffm->hide();
  
  paintToolBar = new PaintToolBar(this,contr->getGridEditManager()->getGridAreaManager());
  
  connect( editTimeDialog, SIGNAL( paintMode(int)) , 
 				   paintToolBar, SLOT( setPaintMode(int) ) );
  paintToolBar->hide();
  connect( paintToolBar, SIGNAL(hidePaintToolBar()), SLOT(disablePaintMode()) ); 
  connect( paintToolBar, SIGNAL(updateGridAreaPlot()), SLOT(updateGLSlot()) ); 

  textview = new TextView(this);
  textview->setMinimumWidth(300);
  connect(textview,SIGNAL(printClicked(int)),SLOT(sendPrintClicked(int)));
  textview->hide();


  // GAMMEL PLOTTING, KOMMENTERT VEKK, MEN IKKE FJERN DENNE !
  //HK !!! kjekk å ha for å teste hurtigmenyer/OKstring uten dialoger
  //connect(qm, SIGNAL(Apply(const vector<miString>&,bool)),
  //  	  SLOT(quickMenuApply(const vector<miString>&)));

  // DENNE SKAL BRUKES ETTER AT putOKString ER IMPLEMENTERT
   connect(qm, SIGNAL(Apply(const vector<miString>&,bool)),
	   SLOT(recallPlot(const vector<miString>&,bool)));

  connect(qm, SIGNAL(requestUpdate(const vector<miString>&,vector<miString>&)),
   	  SLOT(requestQuickUpdate(const vector<miString>&,vector<miString>&)));

  connect(em, SIGNAL(Apply(const vector<miString>&,bool)),
 	  SLOT(recallPlot(const vector<miString>&,bool)));


  // Mark trajectory positions
  connect(trajm, SIGNAL(markPos(bool)), SLOT(trajPositions(bool)));
  connect(trajm, SIGNAL(update()),SLOT(updateGLSlot()));

  connect(em, SIGNAL(editUpdate()), SLOT(editUpdate()));
  connect(em, SIGNAL(editMode(bool)), SLOT(inEdit(bool)));

  connect(uffm, SIGNAL(stationPlotChanged()), SLOT(updateGLSlot()));

  // Documentation and Help
  help = new HelpDialog(this, setup.basicValue("docpath"), "");
  help->hide();

  connect( fm, SIGNAL(FieldApply()), SLOT(MenuOK()));
  connect( om, SIGNAL(ObsApply()),   SLOT(MenuOK()));
  connect( sm, SIGNAL(SatApply()),   SLOT(MenuOK()));
  connect( mm, SIGNAL(MapApply()),   SLOT(MenuOK()));
  connect( objm, SIGNAL(ObjApply()), SLOT(MenuOK()));
  connect( em, SIGNAL(editApply()),  SLOT(editApply()));

  connect( fm, SIGNAL(FieldHide()),  SLOT(fieldMenu()));
  connect( om, SIGNAL(ObsHide()),    SLOT(obsMenu()));
  connect( sm, SIGNAL(SatHide()),    SLOT(satMenu()));
  connect( mm, SIGNAL(MapHide()),    SLOT(mapMenu()));
  connect( em, SIGNAL(EditHide()),   SLOT(editMenu()));
  connect( qm, SIGNAL(QuickHide()),  SLOT(quickMenu()));
  connect( objm, SIGNAL(ObjHide()),  SLOT(objMenu()));
  connect( trajm, SIGNAL(TrajHide()),SLOT(trajMenu()));
  connect( editTimeDialog, SIGNAL(hideDialog()),SLOT(toggleEditTimeDialog()));
  connect( uffm, SIGNAL(uffdaHide()),SLOT(uffMenu()));

  //define shortcuts for all dialogs
  QAccel *mAccel = new QAccel(mm);
  defineShortcuts(mAccel);
  QAccel *fAccel = new QAccel(fm);
  defineShortcuts(fAccel);
  QAccel *oAccel = new QAccel(om);
  defineShortcuts(oAccel);
  QAccel *sAccel = new QAccel(sm);
  defineShortcuts(sAccel);
  QAccel *eAccel = new QAccel(em);
  defineShortcuts(eAccel);
  QAccel *objAccel = new QAccel(objm);
  defineShortcuts(objAccel);
  QAccel *hAccel = new QAccel(help);
  defineShortcuts(hAccel);
  QAccel *uAccel = new QAccel(uffm);
  defineShortcuts(uAccel);

  //problem with quickmenu alt-keys already defined
  //QAccel *qAccel = new QAccel(qm);
  //defineShortcuts(qAccel);
  QAccel *tAccel = new QAccel(trajm);
  defineShortcuts(tAccel);


  // update field dialog when editing field
  connect( em, SIGNAL(emitFieldEditUpdate(miString)),
	   fm, SLOT(fieldEditUpdate(miString)));

  // HELP
  connect( fm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( om, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( sm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( mm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( em, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( qm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( objm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( trajm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));
  connect( uffm, SIGNAL(showdoc(const miString)),
	   help,SLOT(showdoc(const miString)));

  connect(w->Glw(),SIGNAL(objectsChanged()),em, SLOT(undoFrontsEnable()));
  connect(w->Glw(),SIGNAL(fieldsChanged()), em, SLOT(undoFieldsEnable()));

  // accelerator
  QAccel * edAccel = new QAccel(this);
  edAccel->connectItem(edAccel->insertItem(CTRL+Key_Z),this,SLOT(undo()));
  edAccel->connectItem(edAccel->insertItem(CTRL+Key_Y),this,SLOT(redo()));
  edAccel->connectItem(edAccel->insertItem(CTRL+Key_S),this,SLOT(save()));
  edAccel->connectItem(edAccel->insertItem(CTRL+Key_E),em,
		       SLOT(EditMarkedText()));
  edAccel->connectItem(edAccel->insertItem(CTRL+Key_Delete),em,
		       SLOT(DeleteMarkedAnnotation()));



  edAccel->connectItem(edAccel->insertItem(ALT+Key_Left),
		       this,SLOT(startBrowsing()));
  edAccel->connectItem(edAccel->insertItem(ALT+Key_Right),
		       this,SLOT(startBrowsing()));
  edAccel->connectItem(edAccel->insertItem(ALT+Key_Up),
		       this,SLOT(startBrowsing()));
  edAccel->connectItem(edAccel->insertItem(ALT+Key_Down),
		       this,SLOT(startBrowsing()));

  edAccel->connectItem(edAccel->insertItem(Key_End),
		       statusbuttons,SLOT(setfocus()));

  //uffda
  QAccel * uffAccel = new QAccel(this);
  uffAccel->connectItem(uffAccel->insertItem(CTRL+Key_X),this,SLOT(showUffda()));

  // print dialog
  qprt= new QPrinter();

  // vertical profiles
  // create a new main window
  vpWindow = new VprofWindow();
  connect(vpWindow,SIGNAL(VprofHide()),SLOT(hideVprofWindow()));
  connect(vpWindow,SIGNAL(showdoc(const miString)),
	  help,SLOT(showdoc(const miString)));
  connect(vpWindow,SIGNAL(stationChanged(const QString &)),
	  SLOT(stationChangedSlot(const QString &)));
  connect(vpWindow,SIGNAL(modelChanged()),SLOT(modelChangedSlot()));

  // vertical crossections
  // create a new main window
  vcWindow = new VcrossWindow();
  connect(vcWindow,SIGNAL(VcrossHide()),SLOT(hideVcrossWindow()));
  connect(vcWindow,SIGNAL(showdoc(const miString)),
	  help,SLOT(showdoc(const miString)));
  connect(vcWindow,SIGNAL(crossectionChanged(const QString &)),
	  SLOT(crossectionChangedSlot(const QString &)));
  connect(vcWindow,SIGNAL(crossectionSetChanged()),
		     SLOT(crossectionSetChangedSlot()));
  connect(vcWindow,SIGNAL(crossectionSetUpdate()),
		     SLOT(crossectionSetUpdateSlot()));

  // Wave spectrum
  // create a new main window
  spWindow = new SpectrumWindow();
  connect(spWindow,SIGNAL(SpectrumHide()),SLOT(hideSpectrumWindow()));
  connect(spWindow,SIGNAL(showdoc(const miString)),
	  help,SLOT(showdoc(const miString)));
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

  connect( fm ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	   tslider,SLOT(insert(const miString&,const vector<miTime>&)));

  connect( om ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	   tslider,SLOT(insert(const miString&,const vector<miTime>&)));

  connect( sm ,SIGNAL(emitTimes(const miString&,const vector<miTime>&,bool)),
	  tslider,SLOT(insert(const miString&,const vector<miTime>&,bool)));

  connect( em ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	   tslider,SLOT(insert(const miString&,const vector<miTime>&)));

  connect( objm ,SIGNAL(emitTimes(const miString&,const vector<miTime>&,bool)),
	  tslider,SLOT(insert(const miString&,const vector<miTime>&,bool)));

  if ( vpWindow ){
    connect( vpWindow ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	     tslider,SLOT(insert(const miString&,const vector<miTime>&)));
    
    connect( vpWindow ,SIGNAL(setTime(const miString&, const miTime&)),
	     tslider,SLOT(setTime(const miString&, const miTime&)));
  }
  if ( vcWindow ){
    connect( vcWindow ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	     tslider,SLOT(insert(const miString&,const vector<miTime>&)));
    
    connect( vcWindow ,SIGNAL(setTime(const miString&, const miTime&)),
	     tslider,SLOT(setTime(const miString&, const miTime&)));
  }
  if ( spWindow ){
    connect( spWindow ,SIGNAL(emitTimes(const miString&,const vector<miTime>&)),
	     tslider,SLOT(insert(const miString&,const vector<miTime>&)));
    
    connect( spWindow ,SIGNAL(setTime(const miString&, const miTime&)),
	     tslider,SLOT(setTime(const miString&, const miTime&)));
  }


  //Profet
  connect( editTimeDialog, SIGNAL(showField(miString)), 
	   fm, SLOT(addField(miString)));
  connect( editTimeDialog, SIGNAL(apply()), 
	    SLOT(MenuOK()));

  connect( editTimeDialog, SIGNAL(setTime(const miTime&)), 
	   tslider,SLOT(setTime(const miTime&)));


  //parse labels
  const miString label_name = "LABELS";
  vector<miString> sect_label;

  if (!setup.getSection(label_name,sect_label)){
    cerr << label_name << " section not found" << endl;
    //default
    vlabel.push_back("LABEL data font=Helvetica");
    miString labelstr= "LABEL text=\"$day $date $auto UTC\" ";
    labelstr += "tcolour=red bcolour=black ";
    labelstr+= "fcolour=white:200 polystyle=both halign=left valign=top ";
    labelstr+= "font=Helvetica fontsize=12";
    vlabel.push_back(labelstr);
    vlabel.push_back
      ("LABEL anno=<table,fcolour=white:150> halign=right valign=top fcolour=white:0 margin=0");
  }

  for(int i=0; i<sect_label.size(); i++) {
    vlabel.push_back(sect_label[i]);
  }

// //HK - Eksempel på hvordan legge inn ekstra StationPlot
 //  vector <float> lon(3);
//   vector <float> lat(3);
//   vector <miString> names(3);
//   vector <miString> images(3);
//   lon[0]=18.93; lat[0]=69.65;names[0]="Tromsø"; images[0]="";
//   lon[1]=10.72; lat[1]=59.95; names[1]="Oslo";images[1]="metno";
//   lon[2]=5.23; lat[2]=60.20;names[2]="Bergen";images[2]="metno";
//   StationPlot * sp = new StationPlot(names,lon,lat,images);
//   sp->setName("dnmi");
//   sp->setUseStationName(true,true);
//   sp->setStationImageAndImage("vprof_normal","vprof_selected","Oslo");
//   sp->setSelectedStation("Oslo");
//   contr->putStations(sp);
}


void DianaMainWindow::start()
{
  // read the log file
  readLogFile();

  // init quickmenues
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

  optOnOffAction->      setOn( showelem );
  optAutoElementAction->setOn( autoselect );

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



void DianaMainWindow::quickMenuApply(const vector<miString>& s)
{
#ifdef DEBUGPRINT
  cerr << "quickMenuApply:" << endl;
#endif
  QApplication::setOverrideCursor( waitCursor );
  contr->plotCommands(s);

  vector<miTime> ftimes, stimes, otimes, ptimes;
  contr->getPlotTimes(ftimes,stimes,otimes, ptimes);

  tslider->insert("field", ftimes);
  tslider->insert("sat", stimes);
  tslider->insert("obs", otimes);
  tslider->insert("product", ptimes);

  miTime t= tslider->Value();
  contr->setPlotTime(t);
  contr->updatePlots();
  //find current field models amd send to vprofwindow..
  vector <miString> fieldmodels = contr->getFieldModels();
  if (vpWindow) vpWindow->setFieldModels(fieldmodels);
  if (spWindow) spWindow->setFieldModels(fieldmodels);
  w->updateGL();
  timeChanged();

  dialogChanged=false;
  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::resetAll()
{
  mm->useFavourite();
  vector<miString> pstr = mm->getOKString();;
  recallPlot(pstr, true);
  MenuOK();
}

void DianaMainWindow::requestQuickUpdate(const vector<miString>& oldstr,
				      vector<miString>& newstr)
{
  QApplication::setOverrideCursor( waitCursor );
  // strings for each dialog
  vector<miString> mapcom,fldcom,obscom,satcom,objcom,labcom;
  vector<miString> oldfldcom,oldobscom,oldsatcom,oldobjcom;

  int n= newstr.size();
  // sort new strings..
  for (int i=0; i<n; i++){
    miString s= newstr[i];
    s.trim();
    if (!s.exists()) continue;
    vector<miString> vs= s.split(" ");
    miString pre= vs[0].upcase();
    if (pre=="FIELD")      fldcom.push_back(s);
    else if (pre=="OBS")   obscom.push_back(s);
    else if (pre=="MAP")   mapcom.push_back(s);
    else if (pre=="SAT")   satcom.push_back(s);
    else if (pre=="OBJECTS") objcom.push_back(s);
    else if (pre=="LABEL") labcom.push_back(s);
  }
  n= oldstr.size();
  // sort old strings..
  for (int i=0; i<n; i++){
    miString s= oldstr[i];
    s.trim();
    if (!s.exists()) continue;
    vector<miString> vs= s.split(" ");
    miString pre= vs[0].upcase();
    if (pre=="FIELD")      oldfldcom.push_back(s);
    else if (pre=="OBS")   oldobscom.push_back(s);
    else if (pre=="SAT")   oldsatcom.push_back(s);
    else if (pre=="OBJECTS") oldobjcom.push_back(s);
  }

  // maps and labels taken as is

  // strings to dialogs
  fm->requestQuickUpdate(oldfldcom,fldcom);
  om->requestQuickUpdate(oldobscom,obscom);
  sm->requestQuickUpdate(oldsatcom,satcom);
  objm->requestQuickUpdate(oldobjcom,objcom);

  newstr.clear();
  for (int i=0; i<mapcom.size(); i++) newstr.push_back(mapcom[i]);
  for (int i=0; i<fldcom.size(); i++) newstr.push_back(fldcom[i]);
  for (int i=0; i<obscom.size(); i++) newstr.push_back(obscom[i]);
  for (int i=0; i<satcom.size(); i++) newstr.push_back(satcom[i]);
  for (int i=0; i<objcom.size(); i++) newstr.push_back(objcom[i]);
  for (int i=0; i<labcom.size(); i++) newstr.push_back(labcom[i]);

  QApplication::restoreOverrideCursor();
}


void DianaMainWindow::recallPlot(const vector<miString>& vstr,bool replace)
{
  QApplication::setOverrideCursor( waitCursor );
  // strings for each dialog
  vector<miString> mapcom,fldcom,obscom,satcom,objcom,labelcom;
  int n= vstr.size();
  // sort strings..
  for (int i=0; i<n; i++){
    miString s= vstr[i];
    s.trim();
    if (!s.exists()) continue;
    vector<miString> vs= s.split(" ");
    miString pre= vs[0].upcase();
    if (pre=="MAP") mapcom.push_back(s);
    else if (pre=="FIELD") fldcom.push_back(s);
    else if (pre=="OBS") obscom.push_back(s);
    else if (pre=="SAT") satcom.push_back(s);
    else if (pre=="OBJECTS") objcom.push_back(s);
    else if (pre=="LABEL") labelcom.push_back(s);
  }
  vector<miString> tmplabel = vlabel;
  // feed strings to dialogs
  if (replace || mapcom.size()) mm->putOKString(mapcom);
  if (replace || fldcom.size()) fm->putOKString(fldcom);
  if (replace || obscom.size()) om->putOKString(obscom);
  if (replace || satcom.size()) sm->putOKString(satcom);
  if (replace || objcom.size()) objm->putOKString(objcom);
  if (replace ) vlabel=labelcom;

  // call full plot
  push_command= false; // do not push this command on stack
  MenuOK();
  push_command= true;
  vlabel = tmplabel;
  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::togglePaintMode()
{
  bool inPaintMode = togglePaintModeAction->isOn();
  contr->setPaintModeEnabled(inPaintMode);
  if(inPaintMode) paintToolBar->show();
  else            paintToolBar->hide();
  
}
void DianaMainWindow::disablePaintMode()
{
  if(togglePaintModeAction->isOn())
    togglePaintModeAction->toggle();
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

void DianaMainWindow::MenuOK()
{
  //#ifdef DEBUGREDRAW
  cerr<<"DianaMainWindow::MenuOK"<<endl;
  //#endif
  int i;
  vector<miString> pstr;
  vector<miString> diagstr;
  vector<miString> shortnames;

  levelList.clear();
  levelSpec.clear();
  levelIndex= -1;

  idnumList.clear();
  idnumSpec.clear();
  idnumIndex= -1;

  QApplication::setOverrideCursor( waitCursor );

  // fields
  pstr= fm->getOKString();
  shortnames.push_back(fm->getShortname());

  if (pstr.size()>0) {
    fm->getOKlevels(levelList,levelSpec);
    int n= levelList.size();
    int i= 0;
    while (i<n && levelList[i]!=levelSpec) i++;
    levelIndex= (i<n) ? i : -1;
  }
  if (levelIndex>=0) {
    toolLevelUpAction->setEnabled(levelIndex<levelList.size()-1);
    toolLevelDownAction->setEnabled(levelIndex>0);
  } else {
    toolLevelUpAction->setEnabled(false);
    toolLevelDownAction->setEnabled(false);
  }

  if (pstr.size()>0) {
    fm->getOKidnums(idnumList,idnumSpec);
    int n= idnumList.size();
    int i= 0;
    while (i<n && idnumList[i]!=idnumSpec) i++;
    idnumIndex= (i<n) ? i : -1;
  }
  if (idnumIndex>=0) {
    toolIdnumUpAction->setEnabled(idnumIndex<idnumList.size()-1);
    toolIdnumDownAction->setEnabled(idnumIndex>0);
  } else {
    toolIdnumUpAction->setEnabled(false);
    toolIdnumDownAction->setEnabled(false);
  }

  // Observations
  diagstr = om->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(om->getShortname());

  //satellite
  diagstr = sm->getOKString();
  pstr.insert(pstr.end(), diagstr.begin(), diagstr.end());
  shortnames.push_back(sm->getShortname());

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
  for( int i=0; i<vlabel.size(); i++){
    if(!remove || !vlabel[i].contains("$"))  //remove labels with time
      pstr.push_back(vlabel[i]);
  }

  // remove empty lines
  cerr << "------- the final string from all dialogs:" << endl;
  for (i=0; i<pstr.size(); i++){
    pstr[i].trim();
    if (!pstr[i].exists()){
      pstr.erase(pstr.begin()+i);
      i--;
      continue;
    }
    cerr << pstr[i] << endl;
  }

  miTime t;
  t= tslider->Value();
  contr->plotCommands(pstr);
  contr->setPlotTime(t);
  contr->updatePlots();

  //find current field models and send to vprofwindow..
  vector <miString> fieldmodels = contr->getFieldModels();
  if (vpWindow) vpWindow->setFieldModels(fieldmodels);
  if (spWindow) spWindow->setFieldModels(fieldmodels);
  w->updateGL();
  timeChanged();
  dialogChanged=false;

  // push command on history-stack
  if (push_command){ // only when proper menuok
    // make shortname
    miString plotname;
    int m= shortnames.size();
    for (int j=0; j<m; j++)
      if (shortnames[j].exists()){
	plotname+= shortnames[j];
	if (j!=m-1) plotname+= " ";
      }
    qm->pushPlot(plotname,pstr);
  }

  QApplication::restoreOverrideCursor();
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
  qm->prevHPlot();
}

// recall next plot in plot-stack (if exists)
void DianaMainWindow::nextHPlot()
{
  qm->nextHPlot();
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
  miString listname,name;
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
  const int numdialogs= 8;
  static bool visi[numdialogs];

  bool b = showHideAllAction->isOn();

  if (b){
    if (visi[0]= qm->isVisible())    quickMenu();
    if (visi[1]= mm->isVisible())    mapMenu();
    if (visi[2]= fm->isVisible())    fieldMenu();
    if (visi[3]= om->isVisible())    obsMenu();
    if (visi[4]= sm->isVisible())    satMenu();
    //if (visi[5]= em->isVisible())    editMenu();
    if (visi[6]= objm->isVisible())  objMenu();
    if (visi[7]= trajm->isVisible()) trajMenu();
  } else {
    if (visi[0]) quickMenu();
    if (visi[1]) mapMenu();
    if (visi[2]) fieldMenu();
    if (visi[3]) obsMenu();
    if (visi[4]) satMenu();
    //if (visi[5]) editMenu();
    if (visi[6]) objMenu();
    if (visi[7]) trajMenu();
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
  showQuickmenuAction->setOn(!b);
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
  showFieldDialogAction->setOn(!b);
}


void DianaMainWindow::obsMenu()
{
  bool b = om->isVisible();
  if (b){
    om->hide();
  } else {
    om->show();
  }
  showObsDialogAction->setOn( !b );
}


void DianaMainWindow::satMenu()
{
  bool b = sm->isVisible();
  if (b){
    sm->hide();
  } else {
    sm->show();
  }
  showSatDialogAction->setOn( !b );
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
  showUffdaDialogAction->setOn( !b );
}


void DianaMainWindow::mapMenu()
{
  bool b = mm->isVisible();
  if( b ){
    mm->hide();
  } else {
    mm->show();
  }
  showMapDialogAction->setOn( !b );
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
  showEditDialogAction->setOn( b );
}

void DianaMainWindow::toggleEditTimeDialog()
{
  static bool b = editTimeDialog->isVisible();
  QApplication::setOverrideCursor( waitCursor );
  if ( b ){
    editTimeDialog->hide();
  } else {
    editTimeDialog->showDialog();
  }

  b = editTimeDialog->isVisible();

  showEditTimeDialogAction->setOn( b );
  togglePaintModeAction->setOn(b); 

  QApplication::restoreOverrideCursor();
}


void DianaMainWindow::objMenu()
{
  bool b = objm->isVisible();
  if (b){
    objm->hideAll();
  } else {
    objm->showAll();
  }
  showObjectDialogAction->setOn( !b );
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
  showTrajecDialogAction->setOn( !b );
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


void DianaMainWindow::info_activated(int id)
{
  if (id >= 0 && id < infoFiles.size()){
    TextDialog* td= new TextDialog(this, infoFiles[id]);
    td->show();
  }
}


void DianaMainWindow::vprofStartup()
{
  if ( !vpWindow ) return;
  if (vpWindow->firstTime) MenuOK();
  miTime t;
  contr->getPlotTime(t);
  vpWindow->startUp(t);
  vpWindow->show();
  contr->stationCommand("show","vprof");
}


void DianaMainWindow::vcrossStartup()
{
  if ( !vcWindow ) return;
  if (vcWindow->firstTime) MenuOK();
  miTime t;
  contr->getPlotTime(t);
  vcWindow->startUp(t);
  vcWindow->show();
}


void DianaMainWindow::spectrumStartup()
{
  if ( !spWindow ) return;
  if (spWindow->firstTime) MenuOK();
  miTime t;
  contr->getPlotTime(t);
  spWindow->startUp(t);
  spWindow->show();
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
  cerr << "DianaMainWindow::stationChangedSlot to " << station << endl;
#endif
  miString s =station.latin1();
  vector<miString> data;
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
  cerr << "DianaMainWindow::crossectionChangedSlot to " << name << endl;
#endif
  miString s= name.latin1();
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
  cerr << "DianaMainWindow::spectrumChangedSlot to " << name << endl;
#endif
  miString s =station.latin1();
  vector<miString> data;
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
  miString dummy;
  contr->areaCommand("delete","all","all",-1);

  //remove times
  vector<miString> type = timecontrol->deleteType(-1);
  for(int i=0;i<type.size();i++)
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
  miString from(letter.from);
//  cerr<<"Command: "<<letter.command<<"  ";
//  cerr <<endl;
//   cerr<<" Description: "<<letter.description<<"  ";
//  cerr <<endl;
//   cerr<<" commonDesc: "<<letter.commondesc<<"  ";
//  cerr <<endl;
//   cerr<<" Common: "<<letter.common<<"  ";
//  cerr <<endl;
//   for(int i=0;i<letter.data.size();i++)
//     if(letter.data[i].length()<80)
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
      vector<miString> tmp= letter.data[0].split(":");
      if(tmp.size()==2){
	float lat= atof(tmp[0].c_str());
	float lon= atof(tmp[1].c_str());
	float x=0, y=0;
	contr->GeoToPhys(lat,lon,x,y);
	int ix= int(x);
	int iy= int(y);
	//find the name of station we clicked at (from plotModul->stationPlot)
	miString station = contr->findStation(ix,iy,letter.command);
	//now tell vpWindow about new station (this calls vpManager)
	if (vpWindow && !station.empty()) vpWindow->changeStation(station);
      }
    }

  }

  else if (letter.command == qmstrings::addimage){
    // description: name:image

    QtImageGallery ig;
    int n = letter.data.size();
    for(int i=0; i<n; i++){
      // separate name and data
      vector<miString> vs= letter.data[i].split(":");
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
      vector<miString> desc = letter.description.split(";");
      if( desc.size() < 2 ) return;
      miString dataSet = desc[0];
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
//     vector<miString> desc = letter.description.split(";");
//     if( desc.size() < 2 ) return;
//       contr->stationCommand("changeImageandText",
// 			    letter.data,desc[0],letter.from,desc[1]);
//   }

  else if (letter.command == qmstrings::changeimageandtext ){
    //cerr << "Change text and image\n";
    //description: dataSet;stationname:image:text:alignment
    //find name of data set from description
    vector<miString> desc = letter.description.split(";");
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
//     vector<miString> desc = letter.description.split(";");
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
    vector<miString> token = letter.common.split(":");
    if(token.size()>1){
      int n = letter.data.size();
      if(n==0) 	contr->areaCommand(token[0],token[1],miString(),letter.from);
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
      vector<miString> token = letter.data[0].split(1,":",true);
      if(token.size() == 2){
	miString name = pluginB->getClientName(letter.from);
	textview->setText(textview_id,name,token[1]);
	textview->show();
      }
    }
  }

  else if (letter.command == qmstrings::enableshowtext ){
    //description: dataset:on/off
    if(letter.data.size()){
      vector<miString> token = letter.data[0].split(":");
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
    vector<miString> token = letter.common.split(":");
    if(token.size()<2) return;
    int id =atoi(token[0].c_str());
    //remove stationPlots from this client
    contr->stationCommand("delete","",id);
    //remove areas from this client
    contr->areaCommand("delete","all","all",id);
    //remove times
    vector<miString> type = timecontrol->deleteType(id);
    for(int i=0;i<type.size();i++)
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
      vector<miTime> times;
      for(int i=0;i<n;i++)
	times.push_back(letter.data[i]);
      tslider->insert(letter.common,times);
      contr->initHqcdata(letter.from,letter.commondesc,
			 letter.common,letter.description,letter.data);

    } else if (letter.commondesc == "time"){
      miTime t(letter.common);
      tslider->setTime(t);
      contr->setPlotTime(t);
      timeChanged();
      contr->updatePlots();
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
  l.to = id;
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
//       for(int i=0;i<letter.data.size();i++)
// 	cerr<<"data:"<<letter.data[i]<<endl;
//   cerr <<"To: "<<letter.to<<endl;
}

void DianaMainWindow::updateObs()
{
#ifdef DEBUGPRINT
  cerr << "DianaMainWindow::obsUpdate()" << endl;
#endif
  QApplication::setOverrideCursor( waitCursor );
  contr->updateObs();
  w->updateGL();
  QApplication::restoreOverrideCursor();

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
    help->showdoc("index.html");
}


void DianaMainWindow::showAccels()
{
  help->showdoc("ug_shortcutkeys.html");
}


void DianaMainWindow::showNews()
{
  help->showdoc("news.html");
}

void DianaMainWindow::about()
{
  QString str =
    tr("Diana - a 2D presentation system for meteorological data, including fields, observations,\nsatellite- and radarimages, vertical profiles and cross sections.\nDiana has tools for on-screen fieldediting and drawing of objects (fronts, areas, symbols etc.")+"\n\n"+ tr("version:") + " " + version_string.c_str()+"\n"+ tr("build:") + " " + build_string.c_str();

  QMessageBox::about( this, tr("about Diana"), str );
}


void DianaMainWindow::TimeChanged()
{
  miTime t= tslider->Value();
  timelabel->setText(t.isoTime().c_str());
}

void DianaMainWindow::TimeSelected()
{
  miTime t= tslider->Value();
  QApplication::setOverrideCursor( waitCursor );
  if (contr->setPlotTime(t) && !dialogChanged){
    contr->updatePlots();
    w->updateGL();
  }
  timeChanged();
  QApplication::restoreOverrideCursor();
}

void DianaMainWindow::SliderSet()
{
  //HK kommenter vekk neste linje, virker overflødig ???
  //dialogChanged=true;
  miTime t= tslider->Value();
  contr->setPlotTime(t);
  TimeChanged();
}

void DianaMainWindow::setTimeLabel()
{
  miTime t;
  contr->getPlotTime(t);
  tslider->setTime(t);
  TimeChanged();
}

void DianaMainWindow::stopAnimation()
{
  timeBackwardAction->setOn( false );
  timeForewardAction->setOn( false );

  killTimer(animationTimer);
  timeron=0;
}

void DianaMainWindow::animationLoop()
{
  timeloop= !timeloop;
  tslider->setLoop(timeloop);

  timeLoopAction->setOn( timeloop );
}

void DianaMainWindow::timerEvent(QTimerEvent *e)
{
  if (e->timerId()==animationTimer){
    miTime t;
    if (!tslider->nextTime(timeron, t, true)){
      stopAnimation();
      return;
    }
    if (contr->setPlotTime(t)){
      contr->updatePlots();
      w->updateGL();
    }
    timeChanged();
  }
}


void DianaMainWindow::animation()
{
  if (timeron!=0)
    stopAnimation();

  timeForewardAction->setOn( true );

  tslider->startAnimation();
  animationTimer= startTimer(timeout_ms);
  timeron=1;
}


void DianaMainWindow::animationBack()
{
  if (timeron!=0)
    stopAnimation();

  timeBackwardAction->setOn( true );

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
  miTime t;
  if (!tslider->nextTime(1, t)) return;
  if (contr->setPlotTime(t)){
    QApplication::setOverrideCursor( waitCursor );
    contr->updatePlots();
    w->updateGL();
    QApplication::restoreOverrideCursor();
  }
  timeChanged();
}


void DianaMainWindow::stepback()
{
  if (timeron) return;
  miTime t;
  if (!tslider->nextTime(-1, t)) return;
  if (contr->setPlotTime(t)){
    QApplication::setOverrideCursor( waitCursor );
    contr->updatePlots();
    w->updateGL();
    QApplication::restoreOverrideCursor();
  }
  timeChanged();
}


void DianaMainWindow::decreaseTimeStep()
{
  int v= timestep->value() - timestep->lineStep();
  if (v<0) v=0;
  timestep->setValue(v);
}


void DianaMainWindow::increaseTimeStep()
{
  int v= timestep->value() + timestep->lineStep();
  timestep->setValue(v);
}


void DianaMainWindow::timeChanged(){
  //to be done whenever time changes (step back/forward, MenuOK etc.)
  objm->commentUpdate();
  satFileListUpdate();
  setTimeLabel();
  miTime t;
  contr->getPlotTime(t);
  if (vpWindow) vpWindow->mainWindowTimeChanged(t);
  if (spWindow) spWindow->mainWindowTimeChanged(t);
  if (vcWindow) vcWindow->mainWindowTimeChanged(t);
  if (showelem) statusbuttons->setPlotElements(contr->getPlotElements());

  //update sat channels in statusbar
  vector<miString> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);

  if(qsocket){
    miMessage letter;
    letter.command= qmstrings::timechanged;
    letter.commondesc= "time";
    letter.common = t.isoTime();
    letter.to = qmstrings::all;
    sendLetter(letter);
  }
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
  levelIndex+=increment;
  int n= levelList.size()-1;
  if (levelIndex<0)
    levelIndex= 0;
  else if (levelIndex>n)
    levelIndex= n;

  toolLevelUpAction->  setEnabled(levelIndex<n);
  toolLevelDownAction->setEnabled(levelIndex>0);

  QApplication::setOverrideCursor( waitCursor );
  fm->changeLevel(levelList[levelIndex]); // update field dialog
  contr->updateLevel(levelSpec,levelList[levelIndex]);
  updateGLSlot();
  QApplication::restoreOverrideCursor();
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
  idnumIndex+=increment;
  int n= idnumList.size()-1;
  if (idnumIndex<0)
    idnumIndex= 0;
  else if (idnumIndex>n)
    idnumIndex= n;

  toolIdnumUpAction->  setEnabled(idnumIndex<n);
  toolIdnumDownAction->setEnabled(idnumIndex>0);

  QApplication::setOverrideCursor( waitCursor );
  fm->changeIdnum(idnumList[idnumIndex]); // update field dialog
  contr->updateIdnum(idnumSpec,idnumList[idnumIndex]);
  updateGLSlot();
  QApplication::restoreOverrideCursor();
}


void DianaMainWindow::saveraster()
{
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = QFileDialog::getSaveFileName(fname,
					   tr("Images (*.png *.xpm *.bmp *.eps);;All (*.*)"),
					   this, "save_file_dialog",
					   tr("Save plot as image") );


  if (!s.isNull()) {// got a filename
    fname= s;
    miString filename= s.latin1();
    miString format= "PNG";
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

void DianaMainWindow::makeEPS(const miString& filename)
{
  QApplication::setOverrideCursor( waitCursor );
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

//   contr->startHardcopy(priop);
  w->Glw()->startHardcopy(priop);
  w->updateGL();
  w->Glw()->endHardcopy();
  w->updateGL();

  QApplication::restoreOverrideCursor();
}


void DianaMainWindow::hardcopy()
{
  miString command= pman.printCommand();
  printOptions priop;

  if (qprt->setup(this)){
    if (qprt->outputToFile()) {
      priop.fname= qprt->outputFileName().latin1();
    } else if (command.substr(0,4)=="lpr ") {
      priop.fname= "prt_" + miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
#ifdef linux
      command= "lpr -r " + command.substr(4,command.length()-4);
#else
      command= "lpr -r -s " + command.substr(4,command.length()-4);
#endif
    } else {
      priop.fname= "tmp_diana.ps";
    }

    // fill printOption from qprinter-selections
    fillPrintOption(qprt, priop);

    // set printername
    if (!qprt->outputToFile())
      priop.printer= qprt->printerName().latin1();

    // start the postscript production
    //statusBar()->message( tr("Starter utskrift"), 2000 );
    QApplication::setOverrideCursor( waitCursor );
    //     contr->startHardcopy(priop);
    w->Glw()->startHardcopy(priop);
    w->updateGL();
    w->Glw()->endHardcopy();
    w->updateGL();

    // if output to printer: call appropriate command
    if (!qprt->outputToFile()){
      priop.numcopies= qprt->numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

//########################################################################
      cerr<<"PRINT: "<< command << endl;
//########################################################################
      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();

    // reset number of copies (saves a lot of paper)
    qprt->setNumCopies(1);
  }
}

// left mouse click -> mark trajectory position
void DianaMainWindow::trajPositions(bool b)
{
  markTrajPos = b;
  //LB: quit while overriderCursor is set -> core dump (why?)
//   if(b)
//     QApplication::setOverrideCursor(crossCursor);
//   else
//     QApplication::restoreOverrideCursor();

}

// picks up a single click on position x,y
void DianaMainWindow::catchMouseGridPos(const mouseEvent mev)
{
  int x = mev.x;
  int y = mev.y;
  if(markTrajPos){
    float lat=0,lon=0;
    contr->PhysToGeo(x,y,lat,lon);
    trajm->mapPos(lat,lon);
    w->updateGL(); // repaint window
  }

  if( !optAutoElementAction->isOn() ){
    catchElement(mev);
  }

  if (mev.modifier==key_Control){
    if (uffda && contr->getSatnames().size()){
      showUffda();
    }
    else {
      //send position to all clients
      float lat=0,lon=0;
      contr->PhysToGeo(x,y,lat,lon);
      miString latstr(lat,6);
      miString lonstr(lon,6);
      miMessage letter;
      letter.command     = qmstrings::positions;
      letter.commondesc  =  "dataset";
      letter.common      =  "diana";
      letter.description =  "lat:lon";
      letter.to = qmstrings::all;
      letter.data.push_back(miString(latstr + ":" + lonstr));
      sendLetter(letter);

    }
  }
}


// picks up a single click on position x,y
void DianaMainWindow::catchMouseRightPos(const mouseEvent mev)
{
  //Here we must check what should be in the Popupmenu
  //(are we pointing at satpicture,classified, inEdit etc..)
  int x = mev.x;
  int y = mev.y;
  int globalX = mev.globalX;
  int globalY = mev.globalY;

  xclick=x; yclick=y;
  //fill list of menuItems
  vrightclickMenu.clear();
  rightclickMenuItem rItem;
  rItem.menuText= tr("Zoom out");
  rItem.member=SLOT(zoomOut());
  rItem.checked=false;
  vrightclickMenu.push_back(rItem);
//   if (contr->inSatTable(xclick,yclick)){
//     rItem.menuText= tr("Skjul/vis klassifisert");
//     rItem.member=SLOT(hideClassified());
//     rItem.checked=false;
//     rItem.param=0;
//     vrightclickMenu.push_back(rItem);
//   }
//   if (uffda && contr->getSatnames().size()){
//     rItem.menuText= tr("Uffda");
//     rItem.member=SLOT(showUffda());
//     rItem.checked=false;
//     rItem.param=0;
//     vrightclickMenu.push_back(rItem);
//   }
  vselectAreas=contr->findAreas(xclick,yclick);
  int n=vselectAreas.size();
  if (n)
    rightsep =vrightclickMenu.size();
  else
    rightsep=0;
  for (int i=0;i<n;i++){
    rItem.menuText=vselectAreas[i].name.cStr();
    rItem.member=SLOT(selectedAreas(int));
    rItem.checked=vselectAreas[i].selected;
    rItem.param=i;
    vrightclickMenu.push_back(rItem);
  }
  if (!vrightclickMenu.size()){
    return;
  }
  else if (vrightclickMenu.size()==1){
    zoomOut();
    return;
  }
  else{
    rightclickmenu->popup(QPoint(globalX,globalY),0);
    int item=0;
    int n= rightclickmenu->count();
    for (int i = 0;i<n;i++){
      int id = rightclickmenu->idAt(i);
      QString rightClicked=rightclickmenu->text(id);
      if (rightClicked== lastRightClicked)
	item=i;
    }
    rightclickmenu->setActiveItem(item);
  }
}

// picks up mousemovements (without buttonclicks)
void DianaMainWindow::catchMouseMovePos(const mouseEvent mev, bool quick)
{
#ifdef DEBUGREDRAWCATCH
  cerr<<"DianaMainWindow::catchMouseMovePos x,y: "<<mev.x<<" "<<mev.y<<endl;
#endif
  int x = mev.x;
  int y = mev.y;
  //HK ??? nb
  xclick=x; yclick=y;
  // show geoposition in statusbar
  if (sgeopos->getGeographicMode()) {
    float lat=0, lon=0;
    contr->PhysToGeo(x,y,lat,lon);
    sgeopos->setPosition(lat,lon);
  } else {
    float xmap=-1., ymap=-1.;
    contr->PhysToMap(x,y,xmap,ymap);
    sgeopos->setPosition(xmap,ymap);
  }

  // show sat-value at position
  vector<SatValues> satval;
  satval = contr->showValues(x, y);
  showsatval->ShowValues(satval);

  // no need for updateGL() here...only GUI-display

  //check if inside annotationPlot to edit
  if (contr->markAnnotationPlot(x,y))
    w->updateGL();

  if (quick) return;

  if( optAutoElementAction->isOn() ){
    catchElement(mev);
  }
}


void DianaMainWindow::catchElement(const mouseEvent mev)
{

#ifdef DEBUGREDRAWCATCH
  cerr<<"DianaMainWindow::catchElement x,y: "<<mev.x<<" "<<mev.y<<endl;
#endif
  int x = mev.x;
  int y = mev.y;

  bool needupdate= false; // updateGL necessary

  miString uffstation = contr->findStation(x,y,"uffda");
  if (!uffstation.empty()) uffm->pointClicked(uffstation);

  //show closest observation
  if( contr->findObs(x,y) ){
    needupdate= true;
  }

  //find the name of station we clicked/pointed
  //at (from plotModul->stationPlot)
  miString station = contr->findStation(x,y,"vprof");
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
  miString crossection= contr->findLocation(x,y,"vcross");
  if (vcWindow && !crossection.empty()) {
    vcWindow->changeCrossection(crossection);
//  needupdate= true;
  }

  if(qsocket){

    //set selected and send position to plugin connected
    vector<int> id;
    vector<miString> name;
    vector<miString> station;

    bool add = false;
    if(mev.modifier==key_Shift) add = true;
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
      int id;
      for(int i=0;i<nareas;i++){
	letter.to = areas[i].id;
	miString datastr = areas[i].name + ":on";
	letter.data.push_back(datastr);
	sendLetter(letter);
      }
    }

    if(hqcTo>0){
      miString name;
      if(contr->getObsName(x,y,name)){
	miMessage letter;
	letter.to = hqcTo;
	letter.command = qmstrings::station;
	letter.commondesc = "name,time";
	miTime t;
	contr->getPlotTime(t);
	letter.common = name + "," + t.isoTime();;
	sendLetter(letter);
      }
    }
  }

  if (needupdate) w->updateGL();
}

void DianaMainWindow::catchGridAreaChanged(){
  editTimeDialog->catchGridAreaChanged();
  paintToolBar->catchGridAreaChanged();
}


void DianaMainWindow::sendSelectedStations(const miString& command)
{
  vector<miString> data;
  contr->stationCommand("selected",data);
  int n=data.size();
  for(int i=0;i<n;i++){
    vector<miString> token = data[i].split(":");
    int m = token.size();
    if(token.size()<2) continue;
    int id=atoi(token[m-1].cStr());
    miString dataset = token[m-2];
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
  if(!em->inedit() && qsocket){

    if( kev.key == key_Plus || kev.key == key_Minus){
      miString dataset;
      int id;
      vector<miString> stations;
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
	for(int i=0;i<stations.size();i++){
	  miString str = stations[i];
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
	miString keyString;
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
      miString keyString;
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
      miString name;
      int id;
      vector<miString> stations;
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

void DianaMainWindow::toggleMenuBar()
{
//   if ( tb2->isVisible() ) {//if ( menuBar()->isVisible() ) {
//       //menuBar()->hide();
//     tb2->hide();
//     //showmenu->setItemChecked( mb, FALSE );
//   } else {

//     //menuBar()->show();
//     tb2->show();
//     //showmenu->setItemChecked( mb, TRUE );
//   }
}


void DianaMainWindow::toggleToolBar()
{
  if ( mainToolbar->isVisible() ) {
    mainToolbar->hide();
    //showmenu->setItemChecked( tb, FALSE );
  } else {
    mainToolbar->show();
    moveToolBar(mainToolbar);
    //showmenu->setItemChecked( tb, TRUE );
  }
}

void DianaMainWindow::toggleStatusBar()
{
  if ( statusBar()->isVisible() ) {
    statusBar()->hide();
    //showmenu->setItemChecked( sb, FALSE );
  } else {
    statusBar()->show();
    //showmenu->setItemChecked( sb, TRUE );
  }
}

void DianaMainWindow::filequit()
{
  if (em->cleanupForExit() && uffm->okToExit()){
    writeLogFile();
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
  timeControlAction->setOn(!b);
}


void DianaMainWindow::PrintPS(miString& filestr )
{
  if (qprt->setup(this)){

    //statusBar()->message( tr("Starter utskrift"), 2000 );
    QApplication::setOverrideCursor( waitCursor );

    if (!qprt->outputToFile()){
      int numcopies= qprt->numCopies();
      miString command= pman.printCommand();
      miString printern= qprt->printerName().latin1();

#ifndef linux
      if (command.substr(0,4)=="lpr ") {
        command= "lpr -s " + command.substr(4,command.length()-4);
      }
#endif

      command.replace("{printer}",printern);
      command.replace("{filename}",filestr);
#ifdef linux
      // current Debian linux version uses this # as start of comment !!!
      command.replace("-{hash}","");
      command.replace("{numcopies}","");
#else
      command.replace("{hash}","#");
      command.replace("{numcopies}",miString(numcopies));
#endif
      cerr<<"PRINT: "<< command << endl;

      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();

    // reset number of copies (saves a lot of paper)
    qprt->setNumCopies(1);
  }
}


void DianaMainWindow::PrintPS(vector <miString>& filenames )
{
  if (qprt->setup(this)){

    //statusBar()->message( "Starter utskrift", 2000 );
    QApplication::setOverrideCursor( waitCursor );

    if (!qprt->outputToFile()){
      miString printern= qprt->printerName().latin1();
      miString command;
      // Hmmm..should we really allow multiple copies here
      int numcopies= qprt->numCopies();
      for (int j=0; j<numcopies; j++)
	for (int i = 0;i<filenames.size();i++){
	  command= pman.printCommand();

#ifndef linux
          if (command.substr(0,4)=="lpr ") {
            command= "lpr -s " + command.substr(4,command.length()-4);
          }
#endif

	  command.replace("{printer}",printern);
	  command.replace("{filename}",filenames[i]);
#ifdef linux
          // current Debian linux version uses this # as start of comment !!!
          command.replace("-{hash}","");
          command.replace("{numcopies}","");
#else
	  command.replace("{hash}","#");
	  command.replace("{numcopies}","1");// to avoid a mess
#endif
          cerr<<"PRINT: "<< command << endl;

	  system(command.c_str());
	}
    }
    QApplication::restoreOverrideCursor();

    // reset number of copies (saves a lot of paper)
    qprt->setNumCopies(1);
  }
}


void DianaMainWindow::writeLogFile()
{
  // write the system log file

  //miString cmd;
  //cmd= "mv -f diana.log-2 diana.log-3";
  //system(cmd.c_str());
  //cmd= "mv -f diana.log-1 diana.log-2";
  //system(cmd.c_str());
  //cmd= "mv -f diana.log   diana.log-1";
  //system(cmd.c_str());

  // should be a cleanup in all the xxxManagers first to release memory...

  miString logfile= "diana.log";
  miString thisVersion= version_string;
  miString thisBuild= build_string;

  // open filestream
  ofstream file(logfile.c_str());
  if (!file){
    cerr << "ERROR OPEN (WRITE) " << logfile << endl;
    return;
  }

  vector<miString> vstr;
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
  file.close();
  cerr << "Finished writing " << logfile << endl;
}


void DianaMainWindow::readLogFile()
{
  // read the system log file

  getDisplaySize();

  miString logfile= "diana.log";
  miString thisVersion= version_string;
  miString logVersion;

  // open filestream
  ifstream file(logfile.c_str());
  if (!file){
    cerr << "ERROR OPEN (READ) " << logfile << endl;
    return;
  }

  cerr << "READ " << logfile << endl;

  miString beginStr, endStr, str;
  vector<miString> vstr;

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
    //else
    //	cerr << "Unhandled log section: " << beginStr << endl;
    }
  }

  file.close();
  cerr << "Finished reading " << logfile << endl;
}


vector<miString> DianaMainWindow::writeLog(const miString& thisVersion,
					   const miString& thisBuild)
{
  vector<miString> vstr;
  miString str;

  // version & time
  str= "VERSION " + thisVersion;
  vstr.push_back(str);
  str= "BUILD " + thisBuild;
  vstr.push_back(str);
  str= "LOGTIME " + miTime::nowTime().isoTime();
  vstr.push_back(str);
  vstr.push_back("================");

  // dialog positions
  str= "MainWindow.size " + miString(this->width()) + " " + miString(this->height());
  vstr.push_back(str);
  str= "MainWindow.pos "  + miString( this->x()) + " " + miString( this->y());
  vstr.push_back(str);
  str= "QuickMenu.pos "   + miString(qm->x()) + " " + miString(qm->y());
  vstr.push_back(str);
  str= "FieldDialog.pos " + miString(fm->x()) + " " + miString(fm->y());
  vstr.push_back(str);
  str= "ObsDialog.pos "   + miString(om->x()) + " " + miString(om->y());
  vstr.push_back(str);
  str= "ObsDialog.size " + miString(om->width()) + " " + miString(om->height());
  vstr.push_back(str);
  str= "SatDialog.pos "   + miString(sm->x()) + " " + miString(sm->y());
  vstr.push_back(str);
  str= "MapDialog.pos "   + miString(mm->x()) + " " + miString(mm->y());
  vstr.push_back(str);
  str= "EditDialog.pos "  + miString(em->x()) + " " + miString(em->y());
  vstr.push_back(str);
  str= "ObjectDialog.pos " + miString(objm->x()) + " " + miString(objm->y());
  vstr.push_back(str);
  str= "Textview.size "   + miString(textview->width()) + " " + miString(textview->height());
  vstr.push_back(str);
  str= "Textview.pos "  + miString(textview->x()) + " " + miString(textview->y());
  vstr.push_back(str);
  vstr.push_back("================");

  // printer name & options...
  QString qstr= qprt->printerName();
  if (!qstr.isEmpty()) {
    str= "PRINTER " + miString(qstr.latin1());
    vstr.push_back(str);
    if (qprt->orientation()==QPrinter::Portrait)
      str= "PRINTORIENTATION portrait";
    else
      str= "PRINTORIENTATION landscape";
    vstr.push_back(str);
  }
  //vstr.push_back("================");

  // Status-buttons
  str= "STATUSBUTTONS " + miString(showelem ? "ON" : "OFF");
  vstr.push_back(str);
  //vstr.push_back("================");

  // Automatic element selection
  autoselect= optAutoElementAction->isOn();
  str= "AUTOSELECT " + miString(autoselect ? "ON" : "OFF");
  vstr.push_back(str);

  // GUI-font
  str= "FONT " + miString(qApp->font().toString().latin1());
  vstr.push_back(str);
  //vstr.push_back("================");

  return vstr;
}


void DianaMainWindow::readLog(const vector<miString>& vstr,
			   const miString& thisVersion,
			   miString& logVersion)
{
  vector<miString> tokens;
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
        else if (tokens[0]=="ObsDialog.pos")   om->move(x,y);
        else if (tokens[0]=="ObsDialog.size")  om->resize(x,y);
        else if (tokens[0]=="SatDialog.pos")   sm->move(x,y);
        else if (tokens[0]=="MapDialog.pos")   mm->move(x,y);
        else if (tokens[0]=="EditDialog.pos")  em->move(x,y);
        else if (tokens[0]=="ObjectDialog.pos") objm->move(x,y);
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
        qprt->setPrinterName(QString(tokens[1].cStr()));
      } else if (tokens[0]=="PRINTORIENTATION") {
	if (tokens[1]=="portrait")
	  qprt->setOrientation(QPrinter::Portrait);
	else
	  qprt->setOrientation(QPrinter::Landscape);
      } else if (tokens[0]=="STATUSBUTTONS") {
	showelem= (tokens[1]=="ON");
      } else if (tokens[0]=="AUTOSELECT") {
	autoselect= (tokens[1]=="ON");
      } else if (tokens[0]=="FONT") {
	miString fontstr = tokens[1];
	//LB:if the font name contains blanks,
	//the string will be cut in pieces, and must be put together again.
	for( int i=2;i<tokens.size();i++)
	  fontstr += " " + tokens[i];
	QFont font;
	if (font.fromString(fontstr.cStr()))
	  qApp->setFont(font, true);
      }
    }
  }

  if (logVersion.empty()) logVersion= "0.0.0";

  if (logVersion!=thisVersion)
    cerr << "log from version " << logVersion << endl;
}


void DianaMainWindow::getDisplaySize()
{
  displayWidth=  QApplication::desktop()->width();
  displayHeight= QApplication::desktop()->height();

  cerr << "display width,height: "<<displayWidth<<" "<<displayHeight<<endl;
}


void DianaMainWindow::checkNews()
{
  miString newsfile= "diana.news";
  miString thisVersion= "yy";
  miString newsVersion= "xx";

  // check modification time on news file
  SetupParser setup;
  miString filename= setup.basicValue("docpath") + "/" + "news.html";
  QFileInfo finfo( filename.c_str() );
  if (finfo.exists()) {
    QDateTime dt = finfo.lastModified();
    thisVersion = dt.toString("yyyy-MM-dd hh:mm:ss").latin1();
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


void DianaMainWindow::xyGeoPos()
{
  sgeopos->changeMode();
}

void DianaMainWindow::latlonGeoPos()
{
  sgeopos->changeLatLonMode();
}


void DianaMainWindow::defineShortcuts(QAccel * accel)
{
  //define shortcuts for use in dialogs - work same way as in mainWindow
  //
  accel->connectItem( accel->insertItem( Key_F1), this,SLOT(showHelp()));
  accel->connectItem( accel->insertItem( Key_F12), this,SLOT(quickMenu()));
  accel->connectItem( accel->insertItem( Key_F10), this,SLOT(prevHPlot()));
  accel->connectItem( accel->insertItem( Key_F11), this,SLOT(nextHPlot()));
  accel->connectItem( accel->insertItem( Key_End), statusbuttons,SLOT(setfocus()));
  accel->connectItem( accel->insertItem( ALT+Key_K),this, SLOT(mapMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_F),this, SLOT(fieldMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_O),this, SLOT(obsMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_S),this, SLOT(satMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_J),this, SLOT(objMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_T),this, SLOT(trajMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_V),this, SLOT(vprofMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_C),this, SLOT(vcrossMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_W),this, SLOT(spectrumMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_E),this, SLOT(editMenu()));
  accel->connectItem( accel->insertItem( ALT+Key_U),this, SLOT(uffMenu()));
  accel->connectItem( accel->insertItem( CTRL+Key_U),this, SLOT(MenuOK()));
  accel->connectItem( accel->insertItem( CTRL+Key_P),this, SLOT(hardcopy()));
  accel->connectItem( accel->insertItem( CTRL+Key_Q),this, SLOT(filequit()));
  accel->connectItem( accel->insertItem( CTRL+Key_D),this,SLOT(toggleDialogs()));
  accel->connectItem(accel->insertItem(CTRL+Key_X),this,SLOT(showUffda()));
  //timestep
  accel->connectItem(accel->insertItem( CTRL+Key_Left ), this,
		       SLOT(stepback()) );
  accel->connectItem(accel->insertItem( CTRL+Key_Right ), this,
		       SLOT(stepforward()) );
  accel->connectItem(accel->insertItem( CTRL+Key_Down ), this,
		       SLOT(decreaseTimeStep()) );
  accel->connectItem(accel->insertItem( CTRL+Key_Up ), this,
		       SLOT(increaseTimeStep()) );
  accel->connectItem(accel->insertItem( CTRL+Key_PageUp ), this,
		       SLOT(levelUp()) );
  accel->connectItem(accel->insertItem( CTRL+Key_PageDown ), this,
		       SLOT(levelDown()) );
  accel->connectItem(accel->insertItem( SHIFT+Key_PageUp ), this,
		       SLOT(idnumUp()) );
  accel->connectItem(accel->insertItem( SHIFT+Key_PageDown ), this,
		       SLOT(idnumDown()) );
  accel->connectItem(accel->insertItem( SHIFT+CTRL+Key_Left ), this,
		       SLOT(animationBack()) );
  accel->connectItem(accel->insertItem( SHIFT+CTRL+Key_Right ), this,
		       SLOT(animation()) );
  accel->connectItem(accel->insertItem( SHIFT+CTRL+Key_Down ), this,
		       SLOT(animationStop()) );
  accel->connectItem(accel->insertItem( SHIFT+CTRL+Key_Up ), this,
		       SLOT(animationLoop()) );

  //edit
  accel->connectItem(accel->insertItem(CTRL+Key_Z),this,SLOT(undo()));
  accel->connectItem(accel->insertItem(CTRL+Key_Y),this,SLOT(redo()));
  accel->connectItem(accel->insertItem(CTRL+Key_S),this,SLOT(save()));
  accel->connectItem(accel->insertItem(CTRL+Key_E),em,SLOT(EditMarkedText()));
}


void DianaMainWindow::toggleElement(PlotElement pe)
{
  contr->enablePlotElement(pe);
  //update sat channels in statusbar
  vector<miString> channels = contr->getCalibChannels();
  showsatval->SetChannels(channels);
  w->updateGL();
}


void DianaMainWindow::showElements()
{
  if ( showelem ) {
    statusbuttons->reset();
    statusbuttons->hide();
    optOnOffAction->setOn( false );
    showelem= false;
  } else {
    statusbuttons->setPlotElements(contr->getPlotElements());
    statusbuttons->show();
    optOnOffAction->setOn( true );
    showelem= true;
  }
}

void DianaMainWindow::archiveMode()
{
  bool on= optArchiveAction->isOn();
//   optArchiveAction->setOn( on );

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
//   bool on = optAutoElementAction->isOn();
//   optAutoElementAction->setOn( on );
}


void DianaMainWindow::showAnnotations()
{
  bool on = optAnnotationAction->isOn();
//   optAnnotationAction->setOn( on );
  contr->showAnnotations(on);
}

void DianaMainWindow::chooseFont()
{
  bool ok;
  QFont font = QFontDialog::getFont( &ok,qApp->font(),this );
  if ( ok ) {
    // font is set to the font the user selected
    qApp->setFont(font, true);
  } else {
    // the user cancelled the dialog; font is set to the initial
    // value: do nothing
  }
}


//SLOTS called from PopupMenu
void DianaMainWindow::fillRightclickmenu()
{
  rightclickmenu->clear();
  int n=vrightclickMenu.size();
  for (int i=0;i<n;i++){
    int ir = rightclickmenu->insertItem(vrightclickMenu[i].menuText,this,
			     vrightclickMenu[i].member);
    if (vrightclickMenu[i].checked)
      rightclickmenu->setItemChecked(ir,true);
    else
      rightclickmenu->setItemChecked(ir,false);
    rightclickmenu->setItemParameter(ir,vrightclickMenu[i].param);
  }
  rightclickmenu->insertSeparator(rightsep);
}


void DianaMainWindow::rightClickMenuActivated(int i)
{
  lastRightClicked=rightclickmenu->text(i);
}


void DianaMainWindow::zoomOut()
{
  contr->zoomOut();
  w->updateGL();
}

// void DianaMainWindow::hideClassified(){
//   // check to see if we clicked inside a satellitte classification table
//   // if we clicked on the title bar, show or hide it...
//   contr->showSatTable(xclick,yclick);

//   w->updateGL();
// }

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
void DianaMainWindow::selectedAreas(int ia)
{
//   cerr << "DianaMainWindow::selectedAreas "
//        << lastRightClicked << " " << ia << endl;

  //struct selectArea er i diCommonTypes.h
  miString areaName=vselectAreas[ia].name;
  bool selected=vselectAreas[ia].selected;
  int id=vselectAreas[ia].id;
  //det som er "selected" skal slås av og vice versa
  // (lastRightClicked skal være det samme som areaName HER)
  miString misc=(selected) ? "off" : "on";
  miString datastr = areaName + ":" + misc;
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
    miString str;
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


bool DianaMainWindow::close(bool alsoDelete)
{
  filequit();
  return true;
}
