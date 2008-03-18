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
#include <qapplication.h>
#include <q3filedialog.h>
#include <q3toolbar.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtToggleButton.h>
#include <qlayout.h>
#include <qfont.h>
#include <qmotifstyle.h>
#include <qtUtility.h>
#include <qtSpectrumWindow.h>
//Added by qt3to4:
#include <QPixmap>
#include <diStationPlot.h>
#include <qtSpectrumWidget.h>
#include <qtSpectrumModelDialog.h>
#include <qtSpectrumSetupDialog.h>
#include <diSpectrumManager.h>
#include <qtPrintManager.h>
#include <forover.xpm>
#include <bakover.xpm>


SpectrumWindow::SpectrumWindow()
  : Q3MainWindow( 0, "DIANA Spectrum window")
{
#ifndef linux
  qApp->setStyle(new QMotifStyle);
#endif

  //HK ??? temporary create new SpectrumManager here
  spectrumm = new SpectrumManager();

  setCaption( tr("Diana Wavespectrum") );

  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  //central widget
  spectrumw= new SpectrumWidget(spectrumm, fmt, this, "Spectrum");
  setCentralWidget(spectrumw);
  connect(spectrumw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(spectrumw, SIGNAL(stationChanged(int)),SLOT(stationChangedSlot(int)));


  //tool bar and buttons
  spToolbar = new Q3ToolBar(tr("Wavespectrum - control"), this,Qt::DockTop, FALSE,"spTool");
  setDockEnabled( spToolbar, Qt::DockLeft, FALSE );
  setDockEnabled( spToolbar, Qt::DockRight, FALSE );
  //tool bar for selecting time and station
  tsToolbar = new Q3ToolBar(tr("Wavespectrum - position/time"), this,Qt::DockTop, FALSE,"tsTool");
  setDockEnabled( tsToolbar, Qt::DockLeft, FALSE );
  setDockEnabled( tsToolbar, Qt::DockRight, FALSE );


  //button for modeldialog-starts new dialog
  modelButton = new ToggleButton(spToolbar,tr("Model").latin1());
  connect( modelButton, SIGNAL( toggled(bool)), SLOT( modelClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(spToolbar,tr("Settings").latin1());
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for update
  QPushButton * updateButton = NormalPushButton(tr("Refresh"),spToolbar);
  connect( updateButton, SIGNAL(clicked()), SLOT(updateClicked()) );

  //button to print - starts print dialog
  QPushButton* printButton = NormalPushButton(tr("Print"),spToolbar);
  connect( printButton, SIGNAL(clicked()), SLOT( printClicked() ));

  //button to save - starts save dialog
  QPushButton* saveButton = NormalPushButton(tr("Save"),spToolbar);
  connect( saveButton, SIGNAL(clicked()), SLOT( saveClicked() ));

  //button for quit
  QPushButton * quitButton = NormalPushButton(tr("Quit"),spToolbar);
  connect( quitButton, SIGNAL(clicked()), SLOT(quitClicked()) );

  //button for help - pushbutton
  QPushButton * helpButton = NormalPushButton(tr("Help"),spToolbar);
  connect( helpButton, SIGNAL(clicked()), SLOT(helpClicked()) );

  tsToolbar->addSeparator();

  QToolButton *leftStationButton= new QToolButton(QPixmap(bakover_xpm),
						  tr("previous station"), "",
						  this, SLOT(leftStationClicked()),
						  tsToolbar, "spSstepB" );
  leftStationButton->setUsesBigPixmap(false);
  leftStationButton->setAutoRepeat(true);

  //combobox to select station
  vector<miString> stations;
  stations.push_back("                        "); 
  stationBox = ComboBox( tsToolbar, stations, true, 0);
  connect( stationBox, SIGNAL( activated(int) ),
		       SLOT( stationBoxActivated(int) ) );

  QToolButton *rightStationButton= new QToolButton(QPixmap(forward_xpm),
						   tr("next station"), "",
						   this, SLOT(rightStationClicked()),
						   tsToolbar, "spSstepF" );
  rightStationButton->setUsesBigPixmap(false);
  rightStationButton->setAutoRepeat(true);

  tsToolbar->addSeparator();

  QToolButton *leftTimeButton= new QToolButton(QPixmap(bakover_xpm),
					       tr("previous timestep"), "",
					       this, SLOT(leftTimeClicked()),
					       tsToolbar, "spTstepB" );
  leftTimeButton->setUsesBigPixmap(false);
  leftTimeButton->setAutoRepeat(true);

  //combobox to select time
  vector<miString> times;
  times.push_back("2002-01-01 00"); 
  timeBox = ComboBox( tsToolbar, times, true, 0);
  connect( timeBox, SIGNAL( activated(int) ),
		    SLOT( timeBoxActivated(int) ) );

  QToolButton *rightTimeButton= new QToolButton(QPixmap(forward_xpm),
						tr("next timestep"), "",
						this, SLOT(rightTimeClicked()),
						tsToolbar, "spTstepF" );
  rightTimeButton->setUsesBigPixmap(false);
  rightTimeButton->setAutoRepeat(true);

  //connected dialogboxes

  spModelDialog = new SpectrumModelDialog(this,spectrumm);
  connect(spModelDialog, SIGNAL(ModelApply()),SLOT(changeModel()));
  connect(spModelDialog, SIGNAL(ModelHide()),SLOT(hideModel()));
  connect(spModelDialog, SIGNAL(showdoc(const miString)),
	  SIGNAL(showdoc(const miString)));


  spSetupDialog = new SpectrumSetupDialog(this,spectrumm);
  connect(spSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(spSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(spSetupDialog, SIGNAL(showdoc(const miString)),
	  SIGNAL(showdoc(const miString)));


  //initialize everything in startUp
  firstTime = true;
  active = false;
//mainWindowTime= miTime::nowTime();

#ifdef DEBUGPRINT
  cerr<<"SpectrumWindow::SpectrumWindow() finished"<<endl;
#endif
}


void SpectrumWindow::modelClicked( bool on )
{
  //called when the model button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Model button clicked on" << endl;
#endif
    spModelDialog->show();
  } else {
#ifdef DEBUGPRINT
    cerr << "Model button clicked off" << endl;
#endif
    spModelDialog->hide();
  }
}


void SpectrumWindow::leftStationClicked()
{
  //called when the left Station button is clicked
  miString s= spectrumm->setStation(-1);
  stationChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightStationClicked()
{
  //called when the right Station button is clicked
  miString s= spectrumm->setStation(+1);
  stationChangedSlot(+1);
  spectrumw->updateGL();
}


void SpectrumWindow::leftTimeClicked()
{
  //called when the left time button is clicked
  miTime t= spectrumm->setTime(-1);
  //update combobox
  timeChangedSlot(-1);
  spectrumw->updateGL();
}


void SpectrumWindow::rightTimeClicked()
{
  //called when the right Station button is clicked
  miTime t= spectrumm->setTime(+1);
  timeChangedSlot(+1);
  spectrumw->updateGL();
}


bool SpectrumWindow::timeChangedSlot(int diff)
{
  //called if signal timeChanged is emitted from graphics
  //window (qtSpectrumWidget)
#ifdef DEBUGPRINT
  cerr << "timeChangedSlot(int) is called " << endl;
#endif
  int index=timeBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=timeBox->count()-1;
    } 
    timeBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > timeBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    timeBox->setCurrentItem(index);
    diff--;
  }
  miTime t = spectrumm->getTime();
  miString tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  miString tbs=timeBox->currentText().latin1();
  if (tbs!=tstring){
    //search timeList
    int n = timeBox->count();
    for (int i = 0; i<n;i++){      
      if(tstring ==timeBox->text(i).latin1()){
	timeBox->setCurrentItem(i);
	tbs=timeBox->currentText().latin1();
	break;
      }
    }
  }
  if (tbs!=tstring){
    cerr << "WARNING! timeChangedSlot  time from spectrumm ="
	 << t    <<" not equal to timeBox text = " << tbs << endl
	 << "You should search through timelist!" << endl;
    return false;
  }

  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit spectrumSetChanged();
    //update combobox lists of stations and time
    updateStationBox();
    //get correct selection in comboboxes
    stationChangedSlot(0);
  }

  emit setTime("spectrum",t);

  return true;
}


bool SpectrumWindow::stationChangedSlot(int diff)
{
#ifdef DEBUGPRINT
  cerr << "stationChangedSlot(int) is called " << endl;
#endif
  int index=stationBox->currentItem();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=stationBox->count()-1;
    } 
    stationBox->setCurrentItem(index);
    diff++;
  }    
  while(diff>0){
    if(++index > stationBox->count()-1) {
      //set index to the first in the box !
      index=0;
    } 
    stationBox->setCurrentItem(index);
    diff--;
  }
  //get current station
  miString s = spectrumm->getStation();
  //if (!stationBox->count()) return false;
  //if no current station, use last station plotted
  if (s.empty()) s = spectrumm->getLastStation();
  miString sbs=stationBox->currentText().latin1();
  if (sbs!=s){
    int n = stationBox->count();
    for(int i = 0;i<n;i++){
      if (s==stationBox->text(i).latin1()){
	stationBox->setCurrentItem(i);
	sbs=miString(stationBox->currentText().latin1());
	break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) { 
    emit spectrumChanged(sq); //name of current station (to mainWindow)
    return true;
  } else {
    //    cerr << "WARNING! stationChangedSlot  station from spectrumm ="
    // 	 << s    <<" not equal to stationBox text = " << sbs << endl;	
    //current or last station plotted is not in the list, insert it...
    stationBox->insertItem(sq,0);
    stationBox->setCurrentItem(0);
    return false;
  }
}


void SpectrumWindow::printClicked()
{
  printerManager pman;
  //called when the print button is clicked
  miString command= pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  if (qprt.setup(this)){
    if (qprt.outputToFile()) {
      priop.fname= qprt.outputFileName().latin1();
    } else if (command.substr(0,4)=="lpr ") {
      priop.fname= "prt_" + miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
#ifdef linux
      command= "lpr -r " + command.substr(4,command.length()-4);
#else
      command= "lpr -r -s " + command.substr(4,command.length()-4);
#endif
    } else {
      priop.fname= "tmp_spectrum.ps";
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // start the postscript production
    QApplication::setOverrideCursor( Qt::waitCursor );

    spectrumm->startHardcopy(priop);
    spectrumw->updateGL();
    spectrumm->endHardcopy();
    spectrumw->updateGL();

    // if output to printer: call appropriate command
    if (!qprt.outputToFile()){
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();
  }
}


void SpectrumWindow::saveClicked()
{
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = Q3FileDialog::getSaveFileName(fname,
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
    spectrumw->saveRasterImage(filename, format, quality);
  }
}


void SpectrumWindow::makeEPS(const miString& filename)
{
  QApplication::setOverrideCursor( Qt::waitCursor );
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

  spectrumm->startHardcopy(priop);
  spectrumw->updateGL();
  spectrumw->updateGL();

  QApplication::restoreOverrideCursor();
}


void SpectrumWindow::setupClicked(bool on)
{
  //called when the setup button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Setup button clicked on" << endl;
#endif
    spSetupDialog->start();
    spSetupDialog->show();

  } else {
#ifdef DEBUGPRINT
    cerr << "Setup button clicked off" << endl;
#endif
    spSetupDialog->hide();
  }
}


void SpectrumWindow::quitClicked()
{
  //called when the quit button is clicked
#ifdef DEBUGPRINT
  cerr << "quit clicked" << endl;
#endif
  //for now, only hide window, not really quit !
  spToolbar->hide();
  tsToolbar->hide();
  modelButton->setOn(false);
  setupButton->setOn(false);

/*****************************************************************
  // cleanup selections in dialog and data in memory
  spModelDialog->cleanup();
  spectrumm->cleanup();

  stationBox->clear();
  timeBox->clear();
*****************************************************************/

  active = false;
  emit SpectrumHide();
  vector<miTime> t;
  emit emitTimes("spectrum",t);
}


void SpectrumWindow::updateClicked()
{
  //called when the update button in Spectrumwindow is clicked
#ifdef DEBUGPRINT
  cerr << "update clicked" << endl;
#endif
  spectrumm->updateObs();      // check obs.files
  miTime t= mainWindowTime; // use the main time (fields etc.)
  mainWindowTimeChanged(t);
}


void SpectrumWindow::helpClicked()
{
  //called when the help button in Spectrumwindow is clicked
#ifdef DEBUGPRINT
    cerr << "help clicked" << endl;
#endif
    emit showdoc("ug_spectrum.html");
}


void SpectrumWindow::MenuOK()
{
  //obsolete - nothing happens here 
#ifdef DEBUGPRINT
    cerr << "SpectrumWindow::MenuOK()" << endl;
#endif
}


void SpectrumWindow::changeModel()
{
  //called when the apply button from model dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
    cerr << "SpectrumWindow::changeModel()" << endl;
#endif
    spectrumm->setModel();

    onlyObs= spectrumm->onlyObsState();

    //emit to main Window (updates stationPlot)
    emit spectrumSetChanged();
    //update combobox lists of stations and time
    updateStationBox();
    updateTimeBox();
    //get correct selection in comboboxes 
    stationChangedSlot(0);
    timeChangedSlot(0);
    spectrumw->updateGL();
}


void SpectrumWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked
#ifdef DEBUGPRINT
    cerr << "SpectrumWindow::changeSetup()" << endl;
#endif
    spectrumw->updateGL();
}


void SpectrumWindow::hideModel()
{
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
    cerr << "SpectrumWindow::hideModel()" << endl;
#endif
    spModelDialog->hide();
    modelButton->setOn(false);
}


void SpectrumWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
    cerr << "SpectrumWindow::hideSetup()" << endl;
#endif
    spSetupDialog->hide();
    setupButton->setOn(false);
}


StationPlot* SpectrumWindow::getStations()
{
#ifdef DEBUGPRINT
  cerr <<"SpectrumWindow::getStations()" << endl;
#endif
  const vector <miString> stations = spectrumm->getStationList();
  const vector <float> latitude = spectrumm->getLatitudes();
  const vector <float> longitude = spectrumm->getLongitudes();
  int n =stations.size();
  StationPlot* stationPlot = new StationPlot(stations,longitude,latitude);
  miString ann = spectrumm->getAnnotationString();
  stationPlot->setStationPlotAnnotation(ann);

  // ADC set plotname (for StatusPlotButtons)
  stationPlot->setPlotName(ann);

  //the coordinates are defined here
#ifdef DEBUGPRINT
 //  for (int i = 0; i<n;i++){ 
//     cerr <<"Station number " << i << " name = " << stations[i] 
// 	 << " latitude = " << latitude[i]
// 	 << " longitude = " << longitude[i] << endl; 
//   }
#endif

  return stationPlot;
}


void SpectrumWindow::updateStationBox()
{
  //update list of stations in stationBox
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::updateStationBox" << endl;
#endif

  stationBox->clear();
  vector<miString> stations= spectrumm->getStationList();
  //add dummy to make stationBox wide enough
  stations.push_back("                        "); 

  int n =stations.size();
  for (int i=0; i<n; i++){
    stationBox->addItem(QString(stations[i].c_str()));		
  }

}


void SpectrumWindow::updateTimeBox()
{
  //update list of times
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::updateTimeBox" << endl;
#endif

  timeBox->clear();
  vector<miTime> times= spectrumm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).cStr()));
  }

  emit emitTimes("spectrum",times);
}


void SpectrumWindow::stationBoxActivated(int index)
{
  //vector<miString> stations= spectrumm->getStationList();
  miString sbs=stationBox->currentText().latin1();
  //if (index>=0 && index<stations.size()) {
  spectrumm->setStation(sbs);
  spectrumw->updateGL();
  QString sq = sbs.c_str();
  emit spectrumChanged(sq); //name of current station (to mainWindow)
  //}
}


void SpectrumWindow::timeBoxActivated(int index)
{
  vector<miTime> times= spectrumm->getTimeList();

  if (index>=0 && index<times.size()) {
    spectrumm->setTime(times[index]);

    if (onlyObs) {
      //emit to main Window (updates stationPlot)
      emit spectrumSetChanged();
      //update combobox lists of stations and time
      updateStationBox();
      //get correct selection in comboboxes
      stationChangedSlot(0);
    }

    spectrumw->updateGL();
  }
}


bool SpectrumWindow::changeStation(const miString& station)
{
#ifdef DEBUGPRINT
  cerr << "SpectrumWindow::changeStation" << endl;
#endif
  spectrumm->setStation(station);
  spectrumw->updateGL();
  raise();
  if (stationChangedSlot(0))
    return true;
  else
    return false;
}


void SpectrumWindow::setFieldModels(const vector<miString>& fieldmodels)
{
  spectrumm->setFieldModels(fieldmodels);
  if (active) changeModel();
  
}


void SpectrumWindow::mainWindowTimeChanged(const miTime& t)
{
  // keep time for next "update" (in case not found now)
  mainWindowTime= t;

  if (!active) return;
#ifdef DEBUGPRINT
  cerr << "spectrumWindow::mainWindowTimeChanged called with time " << t << endl;
#endif
  spectrumm->mainWindowTimeChanged(t);
  if (onlyObs) {
    //emit to main Window (updates stationPlot)
    emit spectrumSetChanged();
    //update combobox lists of stations and time
    updateStationBox();
  }
  //get correct selection in comboboxes 
  stationChangedSlot(0);
  timeChangedSlot(0);
  spectrumw->updateGL();
}


void SpectrumWindow::startUp(const miTime& t)
{
#ifdef DEBUGPRINT
  cerr << "spectrumWindow::startUp called with time " << t << endl;
#endif
  active = true;
  spToolbar->show();
  tsToolbar->show();
  //do something first time we start
  if (firstTime){
    //vector<miString> models;
    //define models for dialogs, comboboxes and stationplot
    //spectrumm->setSelectedModels(models,false,false);
    //spModelDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    spectrumw->updateGL();
  } else {
    changeModel();
  }
  mainWindowTimeChanged(t);
}


vector<miString> SpectrumWindow::writeLog(const miString& logpart)
{
  vector<miString> vstr;
  miString str;

  if (logpart=="window") {

    str= "SpectrumWindow.size " + miString(this->width()) + " "
			     + miString(this->height());
    vstr.push_back(str);
    str= "SpectrumWindow.pos "  + miString(this->x()) + " "
			     + miString(this->y());
    vstr.push_back(str);
    str= "SpectrumModelDialog.pos " + miString(spModelDialog->x()) + " "
			         + miString(spModelDialog->y());
    vstr.push_back(str);
    str= "SpectrumSetupDialog.pos " + miString(spSetupDialog->x()) + " "
			         + miString(spSetupDialog->y());
    vstr.push_back(str);

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

  } else if (logpart=="setup") {

    vstr= spectrumm->writeLog();

  }

  return vstr;
}


void SpectrumWindow::readLog(const miString& logpart, const vector<miString>& vstr,
			  const miString& thisVersion, const miString& logVersion,
			  int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<miString> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split(' ');

      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="SpectrumWindow.size")  this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="SpectrumWindow.pos")      this->move(x,y);
          else if (tokens[0]=="SpectrumModelDialog.pos") spModelDialog->move(x,y);
          else if (tokens[0]=="SpectrumSetupDialog.pos") spSetupDialog->move(x,y);
	}

      } else if (tokens.size()==2) {

        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
	  if (tokens[1]=="portrait")
	    priop.orientation=d_print::ori_portrait;
	  else
	    priop.orientation=d_print::ori_landscape;
        }

      }
    }

  } else if (logpart=="setup") {

    spectrumm->readLog(vstr,thisVersion,logVersion);

  }
}


bool SpectrumWindow::close(bool alsoDelete)
{
  quitClicked();
  return true;
}
