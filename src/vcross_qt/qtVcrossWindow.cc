/*
  Diana - A Free Meteorological Visualisation Tool

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtVcrossWindow.h"

#include "diController.h"
#include "diLocationPlot.h"
#ifdef USE_VCROSS_V2
#include "diVcrossManager.h"
#else
#include "diVcross1Manager.h"
#endif
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "qtVcrossWidget.h"
#include "qtVcrossDialog.h"
#include "qtVcrossSetupDialog.h"
#include "qtPrintManager.h"

#include <qapplication.h>
#include <QFileDialog>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qfont.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QPixmap>
#include <QAction>

#define MILOGGER_CATEGORY "diana.VcrossWindow"
#include <miLogger/miLogging.h>

#include "forover.xpm"
#include "bakover.xpm"


VcrossWindow::VcrossWindow(Controller *co)
  : QMainWindow(0)
{
  METLIBS_LOG_SCOPE();

  vcrossm = new VcrossManager(co);

  setWindowTitle( tr("Diana Vertical Crossections") );

  //central widget
  vcrossw= new VcrossWidget(vcrossm, this);
  setCentralWidget(vcrossw);
  connect(vcrossw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(vcrossw, SIGNAL(crossectionChanged(int)),SLOT(crossectionChangedSlot(int)));

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

  QPushButton *leftCrossectionButton= new QPushButton(QPixmap(bakover_xpm),"",this);
  connect(leftCrossectionButton, SIGNAL(clicked()), SLOT(leftCrossectionClicked()) );
  leftCrossectionButton->setAutoRepeat(true);

  //combobox to select crossection
  std::vector<std::string> dummycross;
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
  std::vector<std::string> dummytime;
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
  connect(vcDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(showsource(const std::string&, const std::string&)));


  vcSetupDialog = new VcrossSetupDialog(this,vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(showsource(const std::string&, const std::string&)));

  // --------------------------------------------------------------------
  showPrevPlotAction = new QAction( tr("P&revious plot"), this );
  showPrevPlotAction->setShortcut(Qt::Key_F10);
  connect( showPrevPlotAction, SIGNAL( triggered() ) ,  SIGNAL( prevHVcrossPlot() ) );
  addAction( showPrevPlotAction );
  // --------------------------------------------------------------------
  showNextPlotAction = new QAction( tr("&Next plot"), this );
  showNextPlotAction->setShortcut(Qt::Key_F11);
  connect( showNextPlotAction, SIGNAL( triggered() ) ,  SIGNAL( nextHVcrossPlot() ) );
  addAction( showNextPlotAction );
  // --------------------------------------------------------------------

  //inialize everything in startUp
  firstTime = true;
  active = false;
}


/***************************************************************************/

void VcrossWindow::dataClicked( bool on ){
  //called when the model button is clicked
  if( on ){
    METLIBS_LOG_DEBUG("Model/field button clicked on");
    vcDialog->show();
  } else {
    METLIBS_LOG_DEBUG("Model/field button clicked off");
    vcDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::leftCrossectionClicked()
{
  //called when the left Crossection button is clicked
  const std::string s = vcrossm->setCrossection(-1);
  crossectionChangedSlot(-1);
  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::rightCrossectionClicked()
{
  //called when the right Crossection button is clicked
  const std::string s= vcrossm->setCrossection(+1);
  crossectionChangedSlot(+1);
  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::leftTimeClicked()
{
  //called when the left time button is clicked
  vcrossm->setTime(-1);
  //update combobox
  timeChangedSlot(-1);
  vcrossw->update();
}

/***************************************************************************/

void VcrossWindow::rightTimeClicked()
{
  //called when the right Crossection button is clicked
  vcrossm->setTime(+1);
  timeChangedSlot(+1);
  vcrossw->update();
}

/***************************************************************************/

bool VcrossWindow::timeChangedSlot(int diff)
{
  //called if signal timeChanged is emitted from graphics
  //window (qtVcrossWidget)
  METLIBS_LOG_SCOPE();

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
  const std::string tstring = t.isoTime(false,false);
  if (!timeBox->count())
    return false;
  std::string tbs = timeBox->currentText().toStdString();
  if (tbs!=tstring){
    //search timeList
    const int n = timeBox->count();
    for (int i = 0; i<n;i++){
      if(tstring == timeBox->itemText(i).toStdString()){
        timeBox->setCurrentIndex(i);
        tbs = timeBox->currentText().toStdString();
        break;
      }
    }
  }
  if (tbs!=tstring){
//    METLIBS_LOG_WARN("WARNING! timeChangedSlot  time from vcrossm ="
//    << t    <<" not equal to timeBox text = " << tbs
//    << "You should search through timelist!");
    return false;
  }

  /*emit*/ setTime("vcross",t);

  return true;
}


/***************************************************************************/

bool VcrossWindow::crossectionChangedSlot(int diff)
{
  METLIBS_LOG_SCOPE();

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
  if (!crossectionBox->count())
    return false;
  //get current crossection
  std::string s = vcrossm->getCrossection();
  //if no current crossection, use last crossection plotted
  if (s.empty())
    s = ""; // FIXME vcrossm->getLastCrossection();
  std::string sbs = crossectionBox->currentText().toStdString();
  if (sbs!=s){
    const int n = crossectionBox->count();
    for(int i = 0;i<n;i++){
      if (s==crossectionBox->itemText(i).toStdString()) {
        crossectionBox->setCurrentIndex(i);
        sbs = crossectionBox->currentText().toStdString();
        break;
      }
    }
  }
  QString sq = s.c_str();
  if (sbs==s) {
    /*emit*/ crossectionChanged(sq); //name of current crossection (to mainWindow)
    return true;
  } else {
    //    METLIBS_LOG_WARN("WARNING! crossectionChangedSlot  crossection from vcrossm ="
    // 	 << s    <<" not equal to crossectionBox text = " << sbs);
    //current or last crossection plotted is not in the list, insert it...
    crossectionBox->addItem(sq,0);
    crossectionBox->setCurrentIndex(0);
    return false;
  }
}


/***************************************************************************/

void VcrossWindow::printClicked()
{
  printerManager pman;
  std::string command = pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt,priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    if (!qprt.outputFileName().isNull()) {
      priop.fname= qprt.outputFileName().toStdString();
    } else {
      priop.fname= "prt_" + miutil::miTime::nowTime().isoTime() + ".ps";
      miutil::replace(priop.fname, ' ', '_');
    }

    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    // set printername
    if (qprt.outputFileName().isNull())
      priop.printer= qprt.printerName().toStdString();

    // start the postscript production
    QApplication::setOverrideCursor( Qt::WaitCursor );

    vcrossw->print(&qprt, priop);

    // if output to printer: call appropriate command
    if (qprt.outputFileName().isNull()){
      priop.numcopies= qprt.numCopies();

      // expand command-variables
      pman.expandCommand(command, priop);

      int res = system(command.c_str());

      if (res != 0){
        METLIBS_LOG_WARN("Print command:" << command << " failed");
      }

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
    std::string filename= s.toStdString();
    std::string format= "PNG";
    int quality= -1; // default quality

    // find format
    if (miutil::contains(filename, ".xpm") || miutil::contains(filename, ".XPM"))
      format= "XPM";
    else if (miutil::contains(filename, ".bmp") || miutil::contains(filename, ".BMP"))
      format= "BMP";
    else if (miutil::contains(filename, ".eps") || miutil::contains(filename, ".epsf")){
      // make encapsulated postscript
      // NB: not screendump!
      makeEPS(filename);
      return;
    }

    // do the save
    vcrossw->saveRasterImage(filename, format, quality);
  }
}


void VcrossWindow::makeEPS(const std::string& filename)
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

  vcrossw->print(priop);
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
  // called when the timeGraph button is clicked
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("on=" << on);

  if (on && vcrossm->timeGraphOK()) {
    vcrossw->enableTimeGraph(true);
  } else if (on) {
    timeGraphButton->setChecked(false);
  } else {
    vcrossm->disableTimeGraph();
    vcrossw->enableTimeGraph(false);
    vcrossw->update();
  }
}

/***************************************************************************/

void VcrossWindow::quitClicked()
{
  //called when the quit button is clicked
  METLIBS_LOG_SCOPE();

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
  std::vector<miutil::miTime> t;
  emit emitTimes("vcross",t);
}

/***************************************************************************/

void VcrossWindow::helpClicked()
{
  //called when the help button in Vcrosswindow is clicked
  METLIBS_LOG_SCOPE();
  /*emit*/ showsource("ug_verticalcrosssections.html");
}

/***************************************************************************/
//called when the Dynamic/Clear button in Vprofwindow is clicked
void VcrossWindow::dynCrossClicked()
{
  METLIBS_LOG_SCOPE();

  // Clean up the current dynamic crossections
  vcrossm->cleanupDynamicCrossSections();
  updateCrossectionBox();
  emit crossectionSetChanged();

  // Tell mainWindow to start receiving notifications
  // about mouse presses on the map
  emit updateCrossSectionPos(true);
  // Redraw to remove any old crossections
  vcrossw->update();
}

/***************************************************************************/

void VcrossWindow::emitQmenuStrings()
{
  const std::string plotname = "<font color=\"#005566\">" + vcDialog->getShortname() + " " + vcrossm->getCrossection() + "</font>";
  const std::vector<std::string> qm_string = vcrossm->getQuickMenuStrings();
  /*emit*/ quickMenuStrings(plotname, qm_string);
}

/***************************************************************************/

void VcrossWindow::changeFields(bool modelChanged)
{
  //called when the apply button from model/field dialog is clicked
  //... or field is changed ?
  METLIBS_LOG_SCOPE();

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

  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked
  METLIBS_LOG_SCOPE();

  //###if (mapOptionsChanged) {
  // emit to MainWindow
  // (updates crossectionPlot colour etc., not data, name etc.)
  /*emit*/ crossectionSetUpdate();
  //###}

  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::hideDialog()
{
  //called when the hide button (from model dialog) is clicked
  METLIBS_LOG_SCOPE();

  vcDialog->hide();
  dataButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked
  METLIBS_LOG_SCOPE();

  vcSetupDialog->hide();
  setupButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  vcrossm->getCrossections(locationdata);
}

/***************************************************************************/

void VcrossWindow::updateCrossectionBox()
{
  //update list of crossections in crossectionBox
  METLIBS_LOG_SCOPE();

  crossectionBox->clear();
  std::vector<std::string> crossections= vcrossm->getCrossectionList();

  int n =crossections.size();
  for (int i=0; i<n; i++){
    crossectionBox->addItem(QString(crossections[i].c_str()));
  }
}

/***************************************************************************/

void VcrossWindow::updateTimeBox()
{
  //update list of times
  METLIBS_LOG_SCOPE();

  timeBox->clear();
  std::vector<miutil::miTime> times= vcrossm->getTimeList();

  int n =times.size();
  for (int i=0; i<n; i++){
    timeBox->addItem(QString(times[i].isoTime(false,false).c_str()));
  }

  /*emit*/ emitTimes("vcross",times);
}

/***************************************************************************/

void VcrossWindow::crossectionBoxActivated(int index)
{
  const QString sq = crossectionBox->currentText();
  const std::string cbs = sq.toStdString();
  //if (index>=0 && index<crossections.size()) {
  vcrossm->setCrossection(cbs);
  vcrossw->update();
  /*emit*/ crossectionChanged(sq); //name of current crossection (to mainWindow)
  emitQmenuStrings();
  //}
}

/***************************************************************************/

void VcrossWindow::timeBoxActivated(int index)
{
  std::vector<miutil::miTime> times= vcrossm->getTimeList();

  if (index>=0 && index<int(times.size())) {
    vcrossm->setTime(times[index]);

    vcrossw->update();
  }
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const std::string& crossection)
{
  METLIBS_LOG_SCOPE();

  vcrossm->setCrossection(crossection); //HK ??? should check if crossection exists ?
  vcrossw->update();
  raise();
  emitQmenuStrings();

  if (crossectionChangedSlot(0))
    return true;
  else
    return false;
}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  if (!active)
    return;
  METLIBS_LOG_DEBUG("time = " << t);

  vcrossm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  crossectionChangedSlot(0);
  timeChangedSlot(0);
  vcrossw->update();
}


/***************************************************************************/

void VcrossWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("t= " << t);

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
    vcrossw->update();
  }
  //changeModel();
  mainWindowTimeChanged(t);
}

/*
 * Set the position clicked on the map in the current VcrossPlot.
 */
void VcrossWindow::mapPos(float lat, float lon)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(lat) << LOGVAL(lon));

  if(vcrossm->setCrossection(lat,lon)) {
    // If the return is true (field) update the crossection box and
    // tell mainWindow to reread the crossections.
    updateCrossectionBox();
    /*emit*/ crossectionSetChanged();
  }
  emitQmenuStrings();
}

void VcrossWindow::parseQuickMenuStrings(const std::vector<miutil::miString>& m)
{
    const std::vector<std::string> s(m.begin(), m.end());
  parseQuickMenuStrings(s);
}

void VcrossWindow::parseQuickMenuStrings(const std::vector<std::string>& qm_string)
{
  vcrossm->parseQuickMenuStrings(qm_string);
  vcDialog->putOKString(qm_string);
  /*emit*/ crossectionSetChanged();

  //update combobox lists of crossections and time
  updateCrossectionBox();
  updateTimeBox();

  crossectionChangedSlot(0);
  vcrossw->update();
}

/***************************************************************************/
void VcrossWindow::parseSetup()
{
  vcrossm->parseSetup();
}

std::vector<miutil::miString> VcrossWindow::writeLogMI(const std::string& lp)
{
  const std::vector<std::string> s(writeLog(lp));
  return std::vector<miutil::miString>(s.begin(), s.end());
}

std::vector<std::string> VcrossWindow::writeLog(const std::string& logpart)
{
  std::vector<std::string> vstr;

  if (logpart=="window") {
    std::string str;
    str= "VcrossWindow.size " + miutil::from_number(width()) + " "
        + miutil::from_number(height());
    vstr.push_back(str);
    str= "VcrossWindow.pos " + miutil::from_number(x()) + " "
        + miutil::from_number(y());
    vstr.push_back(str);
    str= "VcrossDialog.pos "  + miutil::from_number(vcDialog->x()) + " "
        + miutil::from_number(vcDialog->y());
    vstr.push_back(str);
    str= "VcrossSetupDialog.pos " + miutil::from_number(vcSetupDialog->x()) + " "
        + miutil::from_number(vcSetupDialog->y());
    vstr.push_back(str);

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

  } else if (logpart=="setup") {
    vstr = vcrossm->writeLog();
  } else if (logpart=="field") {
    vstr = vcDialog->writeLog();
  }

  return vstr;
}

void VcrossWindow::readLog(const std::string& lp, const std::vector<miutil::miString>& v,
                           const std::string& tv, const std::string& lv, int dw, int dh)
{
  const std::vector<std::string> s(v.begin(), v.end());
  readLog(lp, s, tv, lv, dw, dh);
}

void VcrossWindow::readLog(const std::string& logpart, const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  if (logpart=="window") {

    const int n = vstr.size();

    for (int i=0; i<n; i++) {
      const std::vector<std::string> tokens = miutil::split(vstr[i], 0, " ");
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
    vcrossm->readLog(vstr, thisVersion, logVersion);
  } else if (logpart=="field") {
    vcDialog->readLog(vstr,thisVersion,logVersion);
  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
