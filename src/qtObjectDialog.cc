/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#include "qtObjectDialog.h"

#include "diController.h"
#include "diObjectManager.h"
#include "qtEditComment.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "util/string_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include <iomanip>
#include <sstream>

#include "front.xpm"

#define MILOGGER_CATEGORY "diana.ObjectDialog"
#include <miLogger/miLogging.h>


/***************************************************************************/
ObjectDialog::ObjectDialog(QWidget* parent, Controller* llctrl)
    : DataDialog(parent, llctrl)
    , m_objm(m_ctrl->getObjectManager())
    , useArchive(false)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Weather Objects"));
  m_action = new QAction(QIcon(QPixmap(front_xpm)), windowTitle(), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_J);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_objectdialogue.html";

  //********** create the various QT widgets to appear in dialog ***********

  namebox = new QListWidget(this);
  connect(namebox, &QListWidget::itemClicked, this, &ObjectDialog::nameListClicked);
  updateObjectNames();

  //**** the three buttons "auto", "tid", "fil" *************

  autoButton = new ToggleButton(this, tr("Auto"));
  timeButton = new ToggleButton(this, tr("Time"));
  fileButton = new ToggleButton(this, tr("File"));
  timefileBut = new QButtonGroup(this);
  timefileBut->addButton(autoButton, 0);
  timefileBut->addButton(timeButton, 1);
  timefileBut->addButton(fileButton, 2);
  QHBoxLayout* timefileLayout = new QHBoxLayout();
  timefileLayout->addWidget(autoButton);
  timefileLayout->addWidget(timeButton);
  timefileLayout->addWidget(fileButton);
  timefileBut->setExclusive(true);
  autoButton->setChecked(true);
  //timefileClicked is called when auto,tid,fil buttons clicked
  connect(timefileBut, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ObjectDialog::timefileClicked);

  //********** the list of files/times to choose from **************

  timefileList = new QListWidget( this );
  connect(timefileList, &QListWidget::itemClicked, this, &ObjectDialog::timefileListSlot);

  QLabel* filesLabel = TitleLabel(tr("Selected files"), this);
  selectedFileList = new QListWidget(this);

  //*****  Check boxes for selecting fronts/symbols/areas  **********

  cbs0= new QCheckBox(tr("Fronts"), this);
  cbs1= new QCheckBox(tr("Symbols"),this);
  cbs2= new QCheckBox(tr("Areas"), this);
  cbs3= new QCheckBox(tr("Form"), this);
  QVBoxLayout* cbsLayout = new QVBoxLayout();
  cbsLayout->addWidget(cbs0);
  cbsLayout->addWidget(cbs1);
  cbsLayout->addWidget(cbs2);
  cbsLayout->addWidget(cbs3);
  QGroupBox* bgroupobjects = new QGroupBox();
  bgroupobjects->setLayout(cbsLayout);
  cbs0->setChecked(true);
  cbs1->setChecked(true);
  cbs2->setChecked(true);
  cbs3->setChecked(true);


  //********* slider/lcd number showing max time difference **********

  //values for slider as in SatDialog
  int   timediff_minValue=0;
  int   timediff_maxValue=24;
  int   timediff_value=4;
  m_scalediff= 15;

  int difflength=timediff_maxValue/20 +5;

  QLabel* diffLabel = new QLabel( tr("    Time diff."), this );
  diffLcdnum= LCDNumber( difflength, this);
  diffSlider= Slider( timediff_minValue, timediff_maxValue, 1,
                      timediff_value, Qt::Horizontal, this );
  connect(diffSlider, &QSlider::valueChanged, this, &ObjectDialog::doubleDisplayDiff);

  //********* slider/lcd number showing alpha cut **************

  int alpha_minValue = 0;
  int alpha_maxValue = 100;
  int alpha_value    = 100;
  m_alphascale = 0.01;

  alpha = new ToggleButton(this, tr("Alpha"));
  connect(alpha, &ToggleButton::toggled, this, &ObjectDialog::greyAlpha);

  alphalcd = LCDNumber(4, this);

  salpha = Slider(alpha_minValue, alpha_maxValue, 1, alpha_value, Qt::Horizontal, this);
  connect(salpha, &QSlider::valueChanged, this, &ObjectDialog::alphaDisplay);

  alphaDisplay(alpha_value);
  greyAlpha(false);

  //************************* standard Buttons *****************************

   //push buttons to delete all selections
  QPushButton* deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, &QPushButton::clicked, this, &ObjectDialog::DeleteClicked);

  //toggle button for comments
  commentbutton = new ToggleButton(this, tr("Comments"));
  connect(commentbutton, &ToggleButton::toggled, this, &ObjectDialog::commentClicked);

  // ********************* place all the widgets in layouts ****************

  QGridLayout* gridlayout = new QGridLayout();
  gridlayout->addWidget( diffLabel,  0,0 );
  gridlayout->addWidget( diffLcdnum, 0,1 );
  gridlayout->addWidget( diffSlider, 0,2  );
  gridlayout->addWidget( alpha,    1,0 );
  gridlayout->addWidget( alphalcd, 1,1 );
  gridlayout->addWidget( salpha,   1,2 );
  gridlayout->addWidget( commentbutton,2,2 );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout* vlayout = new QVBoxLayout( this);
  vlayout->addWidget( namebox );
  vlayout->addLayout( timefileLayout );
  vlayout->addWidget( timefileList );
  vlayout->addWidget( filesLabel );
  vlayout->addWidget( selectedFileList );
  vlayout->addWidget( deleteButton );
  vlayout->addWidget( bgroupobjects );
  vlayout->addLayout( gridlayout );
  vlayout->addLayout(createStandardButtons(true));

  objcomment = new EditComment(this, m_objm, false);
  connect(objcomment, &EditComment::CommentHide, this, &ObjectDialog::hideComment);
  objcomment->hide();

   //set the selected prefix and get time file list
  selectedFileList->clear();
  // initialisation and default of timediff
  doubleDisplayDiff(timediff_value);
}

std::string ObjectDialog::name() const
{
  static const std::string OBJ_DATATYPE = "obj";
  return OBJ_DATATYPE;
}

void ObjectDialog::updateDialog()
{
}

/*********************************************/
void ObjectDialog::nameListClicked(QListWidgetItem*)
{
  /* DESCRIPTION: This function is called when a value in namebox is
     selected (region names or file prefixes), and is returned without doing
     anything if the new value  selected is equal to the old one.
     (HK ?? not yet) */

  METLIBS_LOG_SCOPE();

  //update the time/file list
  timefileClicked(timefileBut->checkedId());
}

/*********************************************/
void ObjectDialog::timefileClicked(int tt)
{
  /* This function is called when timefileBut (auto/time/file)is selected*/
  METLIBS_LOG_SCOPE(LOGVAL(tt));

  //update list of files
  updateTimefileList(false);

  //update the selectedFileList box
  updateSelectedFileList();

  m_objm->setObjAuto(tt == 0);
}

/*********************************************/
void ObjectDialog::timefileListSlot(QListWidgetItem*)
{
  /* DESCRIPTION: This function is called when the signal highlighted() is
     sent from the list of time/file and a new list item is highlighted
  */
  METLIBS_LOG_SCOPE();

  //update file time or name in selectedFileList box
  updateSelectedFileList();

  times.clear();
  int index = timefileList->currentRow();
  if (index>0) {
    times.insert(files[index].time);
    emitTimes(times, false);
  }
}

/***************************************************************************/

void ObjectDialog::DeleteClicked()
{
  //unselects  everything
  METLIBS_LOG_SCOPE();

  if (namebox->currentItem())
    namebox->currentItem()->setSelected(false);

  timefileList->clear();

  selectedFileList->clear();

  //Emit empty time list
  times.clear();
  emitTimes(times, false);
}

/*********************************************/

void ObjectDialog::updateTimes()
{
  METLIBS_LOG_SCOPE();

  //update the timefileList
  updateTimefileList(true);

  //update the selectedFileList box
  updateSelectedFileList();
}

/*********************************************/
void ObjectDialog::doubleDisplayDiff(int number)
{
/* This function is called when diffSlider sends a signal valueChanged(int)
   and changes the numerical value in the lcd display diffLcdnum */
  m_totalminutes=int(number*m_scalediff);
  int hours = m_totalminutes/60;
  int minutes=m_totalminutes-hours*60;
  std::ostringstream ostr;
  ostr << hours << ":" << std::setw(2) << std::setfill('0') << minutes;
  std::string str= ostr.str();
  diffLcdnum->display( str.c_str() );
}

/*********************************************/
void ObjectDialog::greyAlpha(bool on)
{
  salpha->setEnabled(on);
  alphalcd->setEnabled(on);
  if (on)
    alphalcd->display( m_alphanr );
  else
    alphalcd->display("OFF");
}

/*********************************************/
void ObjectDialog::alphaDisplay(int number)
{
   m_alphanr= ((double)number)*m_alphascale;
   alphalcd->display( m_alphanr );
}

/*********************************************/

void  ObjectDialog::commentUpdate()
{
  objcomment->readComment();
}

/**********************************************/

void  ObjectDialog::commentClicked(bool on)
{
  objcomment->setVisible(on);
  if (on) {
    //start Comment
    objcomment->readComment();
  }
}

/**********************************************/
void ObjectDialog::updateTimefileList(bool refresh)
{
  METLIBS_LOG_SCOPE(LOGVAL(refresh));

  //clear box with list of files
  timefileList->clear();

  int index= namebox->currentRow();

  // get the list of object files
  if (index>=0 && index<int(objectnames.size()))
    files = m_objm->getObjectFiles(objectnames[index], refresh);
  else
    files.clear();

  int nr_file =  files.size();

  // Put times into vector, sort, and emit
  times.clear();
  for (int i=0; i<nr_file; i++)
    times.insert(files[i].time);

  if (autoButton->isChecked()) {
    emitTimes(times, true);
  } else {
    emitTimes(plottimes_t(), false);
  }

  //update time/file list
  if (timeButton->isChecked()) {

    for (int i=0; i<nr_file; i++){
      timefileList->addItem(QString(files[i].time.isoTime().c_str()));
    }

  } else if (fileButton->isChecked()) {

    for (int i=0; i<nr_file; i++){
      timefileList->addItem(QString(files[i].name.c_str()));
    }

  }

  //set current
  if (timefileList->count()) {
    timefileList->setCurrentRow(0);
  }
}

/*************************************************************************/

void ObjectDialog::updateSelectedFileList()
{
  METLIBS_LOG_SCOPE();

  //clear box with names of files
  selectedFileList->clear();

  std::string namestr;

  int index= namebox->currentRow();
  if (index < 0)
    return;

  int timefileListIndex = timefileList->currentRow();

  if (autoButton->isChecked()) {
    namestr= objectnames[index];
  } else if (timeButton->isChecked() && timefileListIndex>-1) {
    namestr= objectnames[index] + " ";
    namestr+= files[timefileListIndex].time.isoTime();
  } else if (fileButton->isChecked() && timefileListIndex>-1) {
    namestr= files[timefileListIndex].name;
  }

  if (!namestr.empty()) {
    selectedFileList->addItem(QString(namestr.c_str()));
    selectedFileList->item( selectedFileList->count() - 1 )->setSelected(true);
  }
}

/*************************************************************************/

PlotCommand_cpv ObjectDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv vstr;

  if (selectedFileList->count()) {
    ObjectsPlotCommand_p cmd = std::make_shared<ObjectsPlotCommand>();

    miutil::KeyValue_v kvs;
    int index = namebox->currentRow();
    int timefileListIndex = timefileList->currentRow();
    if (index>-1 && index<int(objectnames.size())){
      //item has been selected in dialog
      cmd->objectname = objectnames[index];

      if (timefileListIndex>-1 && timefileListIndex<int(files.size())) {
        const ObjFileInfo& file = files[timefileListIndex];
        if (timeButton->isChecked()){
          const miutil::miTime& time=file.time;
          if (!time.undef())
            cmd->time = time;
        } else if (fileButton->isChecked()) {
          if (!file.name.empty())
            cmd->file = file.name;
        }
      }
    }

    if (cbs0->isChecked())
      cmd->objecttypes.push_back("front");
    if (cbs1->isChecked())
      cmd->objecttypes.push_back("symbol");
    if (cbs2->isChecked())
      cmd->objecttypes.push_back("area");
    if (cbs3->isChecked())
      cmd->objecttypes.push_back("anno");

    cmd->timeDiff = m_totalminutes;
    if (alpha->isChecked())
      cmd->alpha = int(m_alphanr * 255.0f);

    vstr.push_back(cmd);
  }

  return vstr;
}

void ObjectDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();

  DeleteClicked();

  if (vstr.empty())
    return;

  // loop through all PlotInfo's (if there are several plotInfos, only the last one will be used)
  if (ObjectsPlotCommand_cp c = std::dynamic_pointer_cast<const ObjectsPlotCommand>(vstr.back()))
    plotVariables = decodeString(c);

  //update dialog
  bool found=false;
  int nc = namebox->count();
  for (int j=0;j<nc;j++ ){
    std::string listname =  namebox->item(j)->text().toStdString();
    if (plotVariables.objectname==listname){
      namebox->setCurrentRow(j);
      namebox->item(j)->setSelected(true);
      nameListClicked(namebox->item(j));
      found=true;
    }
  }

  if (!found)
    return;
  if (!plotVariables.time.empty()) {
    //METLIBS_LOG_DEBUG("time =" << plotVariables.time);
    int nt=files.size();
    for (int j=0;j<nt;j++ ){
      std::string listtime = stringFromTime(files[j].time, true);
      if (plotVariables.time==listtime){
        timefileBut->button(1)->setChecked(true);
        timefileClicked(1);
        timefileList->item(j)->setSelected(true);
      }
    }
  } else if (!plotVariables.file.empty()) {
    //METLIBS_LOG_DEBUG("file =" << plotVariables.file);
    int nf = files.size();
    for (int j=0;j<nf;j++ ){
      std::string listfile =  files[j].name;
      if (plotVariables.file==listfile){
        timefileBut->button(2)->setChecked(true);
        timefileClicked(2);
        timefileList->item(j)->setSelected(true);
      }
    }
  } else {
    timefileBut->button(0)->setChecked(true);
    timefileClicked(0);
  }

  if (plotVariables.alphanr >=0) {
    //METLIBS_LOG_DEBUG("alpha =" << plotVariables.alphanr);
    int alphavalue = int(plotVariables.alphanr/m_alphascale + 0.5);
    salpha->setValue(  alphavalue );
    alpha->setChecked(true);
    greyAlpha( true );
  }
  if (plotVariables.totalminutes >=0){
    //METLIBS_LOG_DEBUG("totalminutes =" << plotVariables.totalminutes);
    int number= int(plotVariables.totalminutes/m_scalediff + 0.5);
    diffSlider->setValue( number);
  }
  if (plotVariables.useobject["front"])
    cbs0->setChecked(true);
  else
    cbs0->setChecked(false);
  if (plotVariables.useobject["symbol"])
    cbs1->setChecked(true);
  else
    cbs1->setChecked(false);
  if (plotVariables.useobject["area"])
    cbs2->setChecked(true);
  else
    cbs2->setChecked(false);
  if (plotVariables.useobject["anno"])
    cbs3->setChecked(true);
  else
    cbs3->setChecked(false);
}


// static
ObjectDialog::PlotVariables ObjectDialog::decodeString(ObjectsPlotCommand_cp cmd)
{
  METLIBS_LOG_SCOPE();

  PlotVariables okVar;
  okVar.objectname = cmd->objectname;
  okVar.useobject = WeatherObjects::decodeTypeString(cmd->objecttypes);
  okVar.file = cmd->file;
  if (!cmd->time.undef())
    okVar.time = miutil::stringFromTime(cmd->time, true);
  okVar.totalminutes = cmd->timeDiff;
  okVar.alphanr = cmd->alpha / 255.0;
  return okVar;
}


std::string ObjectDialog::getShortname()
{
  METLIBS_LOG_SCOPE();
  std::string name;

  int nameboxIndex = namebox->currentRow();
  int timefileListIndex = timefileList->currentRow();

  if (selectedFileList->count()
      && (autoButton->isChecked()
          || (timefileListIndex>=0 && timefileListIndex<int(files.size()))))
  {
    if (nameboxIndex > -1)
      name += "" + objectnames[nameboxIndex] + " ";
    else
      name+= (" FILE=") + selectedFileList->currentItem()->text().toStdString();
  }

  return name;
}

std::string ObjectDialog::makeOKString(PlotVariables & okVar)
{
  METLIBS_LOG_SCOPE();

  std::string str;
  str = "OBJECTS";

  str+=(" NAME=\"" + okVar.objectname + "\"");


  if (!okVar.time.empty())
    str+=" TIME=" + okVar.time;
  if (!okVar.file.empty())
    str+=" FILE=" + okVar.file;

  str+=" types=";

  if (okVar.useobject["front"])
    str+="front,";
  if (okVar.useobject["symbol"])
    str+="symbol,";
  if (okVar.useobject["area"])
    str+="area";

  std::ostringstream ostr;
  if (okVar.totalminutes >=0)
    ostr<<" timediff="<< okVar.totalminutes;
  if (okVar.alphanr>=0)
    ostr<<" alpha="<<okVar.alphanr;
  str += ostr.str();

  return str;
}


void ObjectDialog::archiveMode(bool on)
{
  METLIBS_LOG_SCOPE();
  useArchive= on;

  updateObjectNames();

  //everything is unselected and listboxes refreshed
  DeleteClicked();
}

void ObjectDialog::updateObjectNames()
{
  namebox->clear();
  objectnames = m_objm->getObjectNames(useArchive);
  for (const std::string& on : objectnames)
    namebox->addItem(QString::fromStdString(on));
}

void ObjectDialog::hideComment()
{
  commentbutton->setChecked(false);
  objcomment->hide();
}
