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
#include <QFileDialog>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtToggleButton.h>
#include <qlayout.h>
#include <qfont.h>
#include <qmotifstyle.h>
#include <qtUtility.h>
#include <qtVcrossWindow.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QPixmap>
#include <diLocationPlot.h>
#include <qtVcrossWidget.h>
#include <qtVcrossDialog.h>
#include <qtVcrossSetupDialog.h>
#include <diVcrossManager.h>
#include <qtPrintManager.h>
#include <forover.xpm>
#include <bakover.xpm>


VcrossWindow::VcrossWindow(Controller *co)
: QMainWindow( 0)
{
#ifndef linux
  qApp->setStyle(new QMotifStyle);
#endif

  //HK ??? temporary create new VcrossManager here
  vcrossm = new VcrossManager(co);

  setWindowTitle( tr("Diana Vertical Crossections") );

  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  //central widget
  vcrossw= new VcrossWidget(vcrossm, fmt, this);
  setCentralWidget(vcrossw);
  connect(vcrossw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(vcrossw, SIGNAL(crossectionChanged(int)),SLOT(crossectionChangedSlot(int)));


  //tool bar and buttons
  // tool bar and buttons
  vcToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,vcToolbar);

  // tool bar for selecting time and station
  tsToolbar = new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,tsToolbar);


  //button for model/field dialog-starts new dialog
  dataButton = new ToggleButton(this,tr("Model/field").toStdString());
  connect( dataButton, SIGNAL( toggled(bool)), SLOT( dataClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this,tr("Settings").toStdString());
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for timeGraph
  timeGraphButton = new ToggleButton(this,tr("TimeGraph").toStdString());
  connect( timeGraphButton, SIGNAL( toggled(bool)), SLOT( timeGraphClicked( bool) ));

  //button to print - starts print dialog
  QPushButton* printButton = NormalPushButton(tr("Print"),this);
  connect( printButton, SIGNAL(clicked()), SLOT( printClicked() ));

  //button to save - starts save dialog
  QPushButton* saveButton = NormalPushButton(tr("Save"),this);
  connect( saveButton, SIGNAL(clicked()), SLOT( saveClicked() ));

  //button for quit
  QPushButton * quitButton = NormalPushButton(tr("Quit"),this);
  connect( quitButton, SIGNAL(clicked()), SLOT(quitClicked()) );

  //button for help - pushbutton
  QPushButton * helpButton = NormalPushButton(tr("Help"),this);
  connect( helpButton, SIGNAL(clicked()), SLOT(helpClicked()) );

  //button for dynCross - pushbutton
  QPushButton * dynCrossButton = NormalPushButton(tr("Draw cross/Clear"),this);
  connect( dynCrossButton, SIGNAL(clicked()), SLOT(dynCrossClicked()) );
  dynCrossButton->hide();

  QPushButton *leftCrossectionButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftCrossectionButton, SIGNAL(clicked()), SLOT(leftCrossectionClicked()) );
  leftCrossectionButton->setAutoRepeat(true);

  //combobox to select crossection
  vector<miutil::miString> dummycross;
  dummycross.push_back("                        ");
  crossectionBox = ComboBox(this, dummycross, true, 0);
  connect( crossectionBox, SIGNAL( activated(int) ),
      SLOT( crossectionBoxActivated(int) ) );

  QPushButton *rightCrossectionButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightCrossectionButton, SIGNAL(clicked()), SLOT(rightCrossectionClicked()) );
  rightCrossectionButton->setAutoRepeat(true);

  QPushButton *leftTimeButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftTimeButton, SIGNAL(clicked()), SLOT(leftTimeClicked()) );
  leftTimeButton->setAutoRepeat(true);

  //combobox to select time
  miutil::miTime tnow= miutil::miTime::nowTime();
  miutil::miTime tset= miutil::miTime(tnow.year(),tnow.month(),tnow.day(),0,0,0);
  vector<miutil::miString> dummytime;
  dummytime.push_back(tset.isoTime(false,false));
  timeBox = ComboBox(this, dummytime, true, 0);
  connect( timeBox, SIGNAL( activated(int) ),
      SLOT( timeBoxActivated(int) ) );

  QPushButton *rightTimeButton= new QPushButton(QPixmap(forward_xpm),"",this);
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  vcToolbar->addWidget(dataButton);
  vcToolbar->addWidget(setupButton);
  vcToolbar->addWidget(timeGraphButton);
  vcToolbar->addWidget(printButton);
  vcToolbar->addWidget(saveButton);
  vcToolbar->addWidget(quitButton);
  vcToolbar->addWidget(helpButton);
  vcToolbar->addWidget(dynCrossButton);

  insertToolBarBreak(tsToolbar);

  tsToolbar->addWidget(leftCrossectionButton);
  tsToolbar->addWidget(crossectionBox);
  tsToolbar->addWidget(rightCrossectionButton);
  tsToolbar->addWidget(leftTimeButton);
  tsToolbar->addWidget(timeBox);
  tsToolbar->addWidget(rightTimeButton);

  //connected dialogboxes

  vcDialog = new VcrossDialog(this,vcrossm);
  connect(vcDialog, SIGNAL(VcrossDialogApply(bool)),SLOT(changeFields(bool)));
  connect(vcDialog, SIGNAL(VcrossDialogHide()),SLOT(hideDialog()));
  connect(vcDialog, SIGNAL(showsource(const miutil::miString, const miutil::miString)),
      SIGNAL(showsource(const miutil::miString, const miutil::miString)));


  vcSetupDialog = new VcrossSetupDialog(this,vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const miutil::miString, const miutil::miString)),
      SIGNAL(showsource(const miutil::miString, const miutil::miString)));


  //inialize everything in startUp
  firstTime = true;
  active = false;
#ifdef DEBUGPRINT
  cerr<<"VcrossWindow::VcrossWindow() finished"<<endl;
#endif
}


/***************************************************************************/

void VcrossWindow::dataClicked( bool on ){
  //called when the model button is clicked
  if( on ){
#ifdef DEBUGPRINT
    cerr << "Model/field button clicked on" << endl;
#endif
    vcDialog->show();
  } else {
#ifdef DEBUGPRINT
    cerr << "Model/field button clicked off" << endl;
#endif
    vcDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::leftCrossectionClicked(){
  //called when the left Crossection button is clicked
  miutil::miString s= vcrossm->setCrossection(-1);
  crossectionChangedSlot(-1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::rightCrossectionClicked(){
  //called when the right Crossection button is clicked
  miutil::miString s= vcrossm->setCrossection(+1);
  crossectionChangedSlot(+1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::leftTimeClicked(){
  //called when the left time button is clicked
  miutil::miTime t= vcrossm->setTime(-1);
  //update combobox
  timeChangedSlot(-1);
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::rightTimeClicked(){
  //called when the right Crossection button is clicked
  miutil::miTime t= vcrossm->setTime(+1);
  timeChangedSlot(+1);
  vcrossw->updateGL();
}

/***************************************************************************/

bool VcrossWindow::timeChangedSlot(int diff){
  //called if signal timeChanged is emitted from graphics
  //window (qtVcrossWidget)
#ifdef DEBUGPRINT
  cerr << "timeChangedSlot(int) is called " << endl;
#endif
  int index=timeBox->currentIndex();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=timeBox->count()-1;
    }
    timeBox->setCurrentIndex(index);
    diff++;
  }
  while(diff>0){
    if(++index > timeBox->count()-1) {
      //set index to the first in the box !
      index=0;
    }
    timeBox->setCurrentIndex(index);
    diff--;
  }
  miutil::miTime t = vcrossm->getTime();
  miutil::miString tstring=t.isoTime(false,false);
  if (!timeBox->count()) return false;
  miutil::miString tbs=timeBox->currentText().toStdString();
  if (tbs!=tstring){
    //search timeList
    int n = timeBox->count();
    for (int i = 0; i<n;i++){
      if(tstring ==timeBox->itemText(i).toStdString()){
        timeBox->setCurrentIndex(i);
        tbs=timeBox->currentText().toStdString();
        break;
      }
    }
  }
  if (tbs!=tstring){
    cerr << "WARNING! timeChangedSlot  time from vcrossm ="
    << t    <<" not equal to timeBox text = " << tbs << endl
    << "You should search through timelist!" << endl;
    return false;
  }

  emit setTime("vcross",t);

  return true;
}


/***************************************************************************/

bool VcrossWindow::crossectionChangedSlot(int diff){
#ifdef DEBUGPRINT
  cerr << "crossectionChangedSlot(int) is called " << endl;
#endif
  int index=crossectionBox->currentIndex();
  while(diff<0){
    if(--index < 0) {
      //set index to the last in the box !
      index=crossectionBox->count()-1;
    }
    crossectionBox->setCurrentIndex(index);
    diff++;
  }
  while(diff>0){
    if(++index > crossectionBox->count()-1) {
      //set index to the first in the box !
      index=0;
    }
    crossectionBox->setCurrentIndex(index);
    diff--;
  }
  //get current crossection
  miutil::miString s = vcrossm->getCrossection();
  if (!crossectionBox->count()) return false;
  //if no current crossection, use last crossection plotted
  if (s.empty()) s = vcrossm->getLastCrossection();
  miutil::miString sbs=crossectionBox->currentText().toStdString();
  if (sbs!=s){
    int n = crossectionBox->count();
    for(int i = 0;i<n;i++){
      if (s==crossectionBox->itemText(i).toStdString()){
        crossectionBox->setCurrentIndex(i);
        sbs=miutil::miString(crossectionBox->currentText().toStdString());
        break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) {
    emit crossectionChanged(sq); //name of current crossection (to mainWindow)
    return true;
  } else {
    //    cerr << "WARNING! crossectionChangedSlot  crossection from vcrossm ="
    // 	 << s    <<" not equal to crossectionBox text = " << sbs << endl;
    //current or last crossection plotted is not in the list, insert it...
    crossectionBox->addItem(sq,0);
    crossectionBox->setCurrentIndex(0);
    return false;
  }
}


/***************************************************************************/

void VcrossWindow::printClicked(){
  printerManager pman;
  miutil::miString command= pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    if (!qprt.outputFileName().isNull()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else if (command.substr(0,4)=="lpr ") {
      priop.fname= "prt_" + miutil::miTime::nowTime().isoTime() + ".ps";
      priop.fname= priop.fname.replace(' ','_');
#ifdef linux
      command= "lpr -r " + command.substr(4,command.length()-4);
#else
      command= "lpr -r -s " + command.substr(4,command.length()-4);
#endif
    } else {
      priop.fname= "tmp_vcross.ps";
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // set printername
    if (qprt.outputFileName().isNull())
      priop.printer= qprt.printerName().toStdString();

    // start the postscript production
    QApplication::setOverrideCursor( Qt::WaitCursor );

    vcrossw->startHardcopy(priop);
    vcrossw->updateGL();
    vcrossw->endHardcopy();
    vcrossw->updateGL();

    // if output to printer: call appropriate command
    if (qprt.outputFileName().isNull()){
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      system(command.c_str());
    }
    QApplication::restoreOverrideCursor();
  }
}

/***************************************************************************/

void VcrossWindow::saveClicked()
{
  static QString fname = "./"; // keep users preferred image-path for later
  QString s = QFileDialog::getSaveFileName(this,
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
    vcrossw->saveRasterImage(filename, format, quality);
  }
}


void VcrossWindow::makeEPS(const miutil::miString& filename)
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

  vcrossw->startHardcopy(priop);
  vcrossw->updateGL();
  vcrossw->endHardcopy();
  vcrossw->updateGL();

  QApplication::restoreOverrideCursor();
}

/***************************************************************************/

void VcrossWindow::setupClicked(bool on){
  //called when the setup button is clicked
  if( on ){
    vcSetupDialog->start();
    vcSetupDialog->show();
  } else {
    vcSetupDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::timeGraphClicked(bool on)
{
  //called when the timeGraph button is clicked
#ifdef DEBUGPRINT
  cerr << "TiemGraph button clicked on=" << on << endl;
#endif
  if (on && vcrossm->timeGraphOK()) {
    vcrossw->enableTimeGraph(true);
  } else if (on) {
    timeGraphButton->setChecked(false);
  } else {
    vcrossm->disableTimeGraph();
    vcrossw->enableTimeGraph(false);
    vcrossw->updateGL();
  }
}

/***************************************************************************/

void VcrossWindow::quitClicked(){
  //called when the quit button is clicked
#ifdef DEBUGPRINT
  cerr << "quit clicked" << endl;
#endif
  vcToolbar->hide();
  tsToolbar->hide();
  dataButton->setChecked(false);
  setupButton->setChecked(false);

  // cleanup selections in dialog and data in memory
  vcDialog->cleanup();
  vcrossm->cleanup();

  crossectionBox->clear();
  timeBox->clear();

  active = false;
  emit updateCrossSectionPos(false);
  emit VcrossHide();
  vector<miutil::miTime> t;
  emit emitTimes("vcross",t);
}

/***************************************************************************/

void VcrossWindow::helpClicked(){
  //called when the help button in Vcrosswindow is clicked
#ifdef DEBUGPRINT
  cerr << "help clicked" << endl;
#endif
  emit showsource("ug_verticalcrosssections.html");
}

/***************************************************************************/
//called when the Dynamic/Clear button in Vprofwindow is clicked
void VcrossWindow::dynCrossClicked(){
#ifdef DEBUGPRINT
  cerr << "Dynamic/Clear clicked" << endl;
#endif
  // Clean up the current dynamic crossections
  vcrossm->cleanupDynamicCrossSections();
  updateCrossectionBox();
  emit crossectionSetChanged();
  // Tell mainWindow to start receiving notifications
  // about mouse presses on the map
  emit updateCrossSectionPos(true);
  // Redraw to remove any old crossections
  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::MenuOK(){
  //obsolete - nothing happens here
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::MenuOK()" << endl;
#endif
}

/***************************************************************************/

void VcrossWindow::changeFields(bool modelChanged){
  //called when the apply button from model/field dialog is clicked
  //... or field is changed ?
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeFields()" << endl;
#endif

  if (modelChanged) {

    //emit to MainWindow (updates crossectionPlot)
    emit crossectionSetChanged();

    //update combobox lists of crossections and time
    updateCrossectionBox();
    updateTimeBox();

    //get correct selection in comboboxes
    crossectionChangedSlot(0);
    timeChangedSlot(0);
  }

  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::changeSetup(){
  //called when the apply from setup dialog is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeSetup()" << endl;
#endif

  //###if (mapOptionsChanged) {
  // emit to MainWindow
  // (updates crossectionPlot colour etc., not data, name etc.)
  emit crossectionSetUpdate();
  //###}

  vcrossw->updateGL();
}

/***************************************************************************/

void VcrossWindow::hideDialog(){
  //called when the hide button (from model dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::hideDialog()" << endl;
#endif
  vcDialog->hide();
  dataButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::hideSetup(){
  //called when the hide button (from setup dialog) is clicked
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::hideSetup()" << endl;
#endif
  vcSetupDialog->hide();
  setupButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::getCrossections(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr <<"VcrossWindow::getCrossections()" << endl;
#endif

  vcrossm->getCrossections(locationdata);

  return;
}

/***************************************************************************/

void VcrossWindow::getCrossectionOptions(LocationData& locationdata){
#ifdef DEBUGPRINT
  cerr <<"VcrossWindow::getCrossectionOptions()" << endl;
#endif

  vcrossm->getCrossectionOptions(locationdata);

  return;
}

/***************************************************************************/

void VcrossWindow::updateCrossectionBox(){
  //update list of crossections in crossectionBox
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::updateCrossectionBox" << endl;
#endif

  crossectionBox->clear();
  vector<miutil::miString> crossections= vcrossm->getCrossectionList();

  int n =crossections.size();
  for (int i=0; i<n; i++){
    crossectionBox->addItem(QString(crossections[i].c_str()));
  }

}

/***************************************************************************/

void VcrossWindow::updateTimeBox(){
  //update list of times
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::updateTimeBox" << endl;
#endif

  timeBox->clear();
  vector<miutil::miTime> times= vcrossm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).cStr()));
  }

  emit emitTimes("vcross",times);
}

/***************************************************************************/

void VcrossWindow::crossectionBoxActivated(int index){

  //vector<miutil::miString> crossections= vcrossm->getCrossectionList();
  miutil::miString cbs=crossectionBox->currentText().toStdString();
  //if (index>=0 && index<crossections.size()) {
  vcrossm->setCrossection(cbs);
  vcrossw->updateGL();
  QString sq = cbs.c_str();
  emit crossectionChanged(sq); //name of current crossection (to mainWindow)
  //}
}

/***************************************************************************/

void VcrossWindow::timeBoxActivated(int index){

  vector<miutil::miTime> times= vcrossm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    vcrossm->setTime(times[index]);

    vcrossw->updateGL();
  }
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const miutil::miString& crossection){
#ifdef DEBUGPRINT
  cerr << "VcrossWindow::changeCrossection" << endl;
#endif
  vcrossm->setCrossection(crossection); //HK ??? should check if crossection exists ?
  vcrossw->updateGL();
  raise();
  if (crossectionChangedSlot(0))
    return true;
  else
    return false;
}


/***************************************************************************/

//void VcrossWindow::setFieldModels(const vector<miutil::miString>& fieldmodels){
//  vcrossm->setFieldModels(fieldmodels);
//  if (active) changeModel();
//}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miutil::miTime& t){
  if (!active) return;
#ifdef DEBUGPRINT
  cerr << "vcrossWindow::mainWindowTimeChanged called with time " << t << endl;
#endif
  vcrossm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  crossectionChangedSlot(0);
  timeChangedSlot(0);
  vcrossw->updateGL();
}


/***************************************************************************/

void VcrossWindow::startUp(const miutil::miTime& t){
#ifdef DEBUGPRINT
  cerr << "vcrossWindow::startUp  t= " << t << endl;
#endif
  active = true;
  vcToolbar->show();
  tsToolbar->show();
  //do something first time we start Vertical crossections
  if (firstTime){
    //vector<miutil::miString> models;
    //define models for dialogs, comboboxes and crossectionplot
    //vcrossm->setSelectedModels(models, true,false);
    //vcDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    vcrossw->updateGL();
  }
  //changeModel();
  mainWindowTimeChanged(t);
}

/*
 * Set the position clicked on the map in the current VcrossPlot.
 */
void VcrossWindow::mapPos(float lat, float lon) {
#ifdef DEBUGPRINT
  cout<<"VcrossWindow::mapPos(" << lat << "," << lon << ")" <<endl;
#endif
  if(vcrossm->setCrossection(lat,lon)) {
    // If the return is true (field) update the crossection box and
    // tell mainWindow to reread the crossections.
    updateCrossectionBox();
    emit crossectionSetChanged();
  }
}

/***************************************************************************/

vector<miutil::miString> VcrossWindow::writeLog(const miutil::miString& logpart)
{
  vector<miutil::miString> vstr;
  miutil::miString str;

  if (logpart=="window") {

    str= "VcrossWindow.size " + miutil::miString(this->width()) + " "
    + miutil::miString(this->height());
    vstr.push_back(str);
    str= "VcrossWindow.pos "  + miutil::miString(this->x()) + " "
    + miutil::miString(this->y());
    vstr.push_back(str);
    str= "VcrossDialog.pos "  + miutil::miString(vcDialog->x()) + " "
    + miutil::miString(vcDialog->y());
    vstr.push_back(str);
    str= "VcrossSetupDialog.pos " + miutil::miString(vcSetupDialog->x()) + " "
    + miutil::miString(vcSetupDialog->y());
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

    vstr= vcrossm->writeLog();

  } else if (logpart=="field") {

    vstr= vcDialog->writeLog();

  }

  return vstr;
}


void VcrossWindow::readLog(const miutil::miString& logpart, const vector<miutil::miString>& vstr,
    const miutil::miString& thisVersion, const miutil::miString& logVersion,
    int displayWidth, int displayHeight)
{

  if (logpart=="window") {

    vector<miutil::miString> tokens;
    int n= vstr.size();

    for (int i=0; i<n; i++) {
      tokens= vstr[i].split(' ');

      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="VcrossWindow.size") this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="VcrossWindow.pos")      this->move(x,y);
          else if (tokens[0]=="VcrossDialog.pos")      vcDialog->move(x,y);
          else if (tokens[0]=="VcrossSetupDialog.pos") vcSetupDialog->move(x,y);
        }

      } else if (tokens.size()>=2) {

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

    vcrossm->readLog(vstr,thisVersion,logVersion);

  } else if (logpart=="field") {

    vcDialog->readLog(vstr,thisVersion,logVersion);

  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}

