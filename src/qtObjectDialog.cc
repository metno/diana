/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "qtToggleButton.h"
#include "qtObjectDialog.h"
#include "qtEditComment.h"
#include "diObjectManager.h"
#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include <QLCDNumber>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QGroupBox>

#include <iomanip>

#define MILOGGER_CATEGORY "diana.ObjectDialog"
#include <miLogger/miLogging.h>

using namespace std;

/***************************************************************************/
ObjectDialog::ObjectDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), m_ctrl(llctrl)
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::ObjectDialog called");
#endif

  m_objm= m_ctrl->getObjectManager();

  //caption to appear on top of dialog
  setWindowTitle(tr("Weather Objects"));

  //initialization

  useArchive=false;
//********** create the various QT widgets to appear in dialog ***********

  namebox = new QListWidget( this );

  objectnames= m_ctrl->getObjectNames(useArchive);
  for (unsigned int i=0; i<objectnames.size(); i++){
    namebox->addItem(QString(objectnames[i].c_str()));
  }

  connect( namebox, SIGNAL(itemClicked(  QListWidgetItem * ) ),
	   SLOT( nameListClicked(  QListWidgetItem * ) ) );

  //**** the three buttons "auto", "tid", "fil" *************

  autoButton = new ToggleButton(this, tr("Auto"));
  timeButton = new ToggleButton(this, tr("Time"));
  fileButton = new ToggleButton(this, tr("File"));
  timefileBut = new QButtonGroup( this );
  timefileBut->addButton(autoButton,0);
  timefileBut->addButton(timeButton,1);
  timefileBut->addButton(fileButton,2);
  QHBoxLayout* timefileLayout = new QHBoxLayout();
  timefileLayout->addWidget(autoButton);
  timefileLayout->addWidget(timeButton);
  timefileLayout->addWidget(fileButton);
  timefileBut->setExclusive( true );
  autoButton->setChecked(true);
  //timefileClicked is called when auto,tid,fil buttons clicked
  connect( timefileBut, SIGNAL( buttonClicked(int) ),
	   SLOT( timefileClicked(int) ) );

  //********** the list of files/times to choose from **************

  timefileList = new QListWidget( this );

  connect( timefileList, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( timefileListSlot( QListWidgetItem * ) ) );


  //the box (with label) showing which files have been choosen
  QLabel* filesLabel = TitleLabel( tr("Selected files"), this);
  selectedFileList = new QListWidget( this );


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
  bgroupobjects= new QGroupBox();
  bgroupobjects->setLayout( cbsLayout );
  //  bgroupobjects->setExclusive( false );
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

  connect(diffSlider,SIGNAL( valueChanged(int)),SLOT(doubleDisplayDiff(int)));



  //********* slider/lcd number showing alpha cut **************

  int alpha_minValue = 0;
  int alpha_maxValue = 100;
  int alpha_value    = 100;
  m_alphascale = 0.01;


  alpha = new ToggleButton(this, tr("Alpha"));
  connect( alpha, SIGNAL( toggled(bool)), SLOT( greyAlpha( bool) ));


  alphalcd = LCDNumber( 4, this);

  salpha  = Slider( alpha_minValue, alpha_maxValue, 1, alpha_value,
			    Qt::Horizontal, this);

  connect( salpha, SIGNAL( valueChanged( int )),
	      SLOT( alphaDisplay( int )));

  // INITIALISATION
  alphaDisplay( alpha_value );
  greyAlpha(false);

  //************************* standard Buttons *****************************

   //push buttons to delete all selections
  QPushButton* deleteButton = NormalPushButton( tr("Delete"), this );
  connect( deleteButton, SIGNAL(clicked()), SLOT(DeleteClicked()));

  //push button to refresh filelistsw
  QPushButton* refresh =NormalPushButton( tr("Refresh"), this );
  connect( refresh, SIGNAL( clicked() ), SLOT( Refresh() ));

  //push button to show help
  QPushButton* objhelp = NormalPushButton( tr("Help"), this);
  connect(  objhelp, SIGNAL(clicked()), SLOT( helpClicked()));

  //toggle button for comments
  commentbutton = new ToggleButton(this, tr("Comments"));
  connect(  commentbutton, SIGNAL(toggled(bool)),
	    SLOT( commentClicked(bool) ));


  //push button to hide dialog
  QPushButton* objhide = NormalPushButton( tr("Hide"), this);
  connect( objhide, SIGNAL(clicked()), SIGNAL(ObjHide()));

   //push button to apply the selected command and then hide dialog
  QPushButton* objapplyhide = NormalPushButton(tr("Apply+Hide"), this );
  connect( objapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  //push button to apply the selected command
  QPushButton* objapply = NormalPushButton( tr("Apply"), this );
  objapply->setDefault( true );
  connect(objapply, SIGNAL(clicked()), SIGNAL( ObjApply()) );


// ********************* place all the widgets in layouts ****************


  QGridLayout* gridlayout = new QGridLayout();
  gridlayout->addWidget( diffLabel,  0,0 );
  gridlayout->addWidget( diffLcdnum, 0,1 );
  gridlayout->addWidget( diffSlider, 0,2  );
  gridlayout->addWidget( alpha,    1,0 );
  gridlayout->addWidget( alphalcd, 1,1 );
  gridlayout->addWidget( salpha,   1,2 );
  gridlayout->addWidget( objhelp,2,0 );
  gridlayout->addWidget( refresh,2,1  );
  gridlayout->addWidget( commentbutton,2,2 );
  gridlayout->addWidget( objhide, 3, 0 );
  gridlayout->addWidget( objapplyhide, 3,1 );
  gridlayout->addWidget( objapply, 3,2 );

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


  objcomment = new EditComment( this, m_ctrl,false );
  connect(objcomment,SIGNAL(CommentHide()),SLOT(hideComment()));
  objcomment->hide();

  //  atd = new AddtoDialog(this,m_ctrl);


   //set the selected prefix and get time file list
  selectedFileList->clear();
  // initialisation and default of timediff
  doubleDisplayDiff(timediff_value);

  //end of constructor
}



/*********************************************/
void ObjectDialog::nameListClicked(  QListWidgetItem * item)
{
/* DESCRIPTION: This function is called when a value in namebox is
 selected (region names or file prefixes), and is returned without doing
anything if the new value  selected is equal to the old one.
 (HK ?? not yet) */

#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::nameListClicked called");
#endif

  //update the time/file list
  timefileClicked(timefileBut->checkedId());

}

/*********************************************/
void ObjectDialog::timefileClicked(int tt){
/* This function is called when timefileBut (auto/time/file)is selected*/
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::timefileClicked called,tt =" << tt);
#endif


  //update list of files
  updateTimefileList(false);

  //update the selectedFileList box
  updateSelectedFileList();

  m_ctrl->setObjAuto(tt==0);

}


/*********************************************/
void ObjectDialog::timefileListSlot( QListWidgetItem * item  ){
/* DESCRIPTION: This function is called when the signal highlighted() is
sent from the list of time/file and a new list item is highlighted
*/
#ifdef dObjectDlg
   METLIBS_LOG_DEBUG("SatDialog::timefileListSlot called");
#endif


   //update file time or name in selectedFileList box
   updateSelectedFileList();

   //
   times.clear();
   int index = timefileList->currentRow();
   if(index>0){
     times.push_back(files[index].time);
     emit emitTimes( "obj",times,false );
   }
}


/***************************************************************************/

void ObjectDialog::DeleteClicked(){
  //unselects  everything
#ifdef dObjectDlg
    METLIBS_LOG_DEBUG("ObjectDialog::DeleteClicked called");
#endif

    if(namebox->currentItem())
      namebox->currentItem()->setSelected(false);

    timefileList->clear();

    selectedFileList->clear();

    //Emit empty time list
    times.clear();
    emit emitTimes("obj",times,false );

#ifdef dObjectDlg
    METLIBS_LOG_DEBUG("ObjectDialog::DeleteClicked returned");
#endif
  return;
}


/*********************************************/

void ObjectDialog::Refresh(){
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::Refresh() called); Filene blir hentet nytt fra disken";
#endif

  //update the timefileList
  updateTimefileList(true);

  //update the selectedFileList box
  updateSelectedFileList();

}


/********************************************/
void ObjectDialog::applyhideClicked(){
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("applyhideClicked()");
#endif
  emit ObjHide();
  emit ObjApply();
}

/********************************************/
void ObjectDialog::helpClicked(){
  emit showsource("ug_objectdialogue.html");
}

/*********************************************/
void ObjectDialog::doubleDisplayDiff( int number ){
/* This function is called when diffSlider sends a signal valueChanged(int)
   and changes the numerical value in the lcd display diffLcdnum */
    m_totalminutes=int(number*m_scalediff);
    int hours = m_totalminutes/60;
    int minutes=m_totalminutes-hours*60;
    ostringstream ostr;
    ostr << hours << ":" << setw(2) << setfill('0') << minutes;
    std::string str= ostr.str();
    diffLcdnum->display( str.c_str() );
}


/*********************************************/
void ObjectDialog::greyAlpha( bool on ){
    if( on ){
      salpha->setEnabled( true );
      alphalcd->setEnabled( true );
      alphalcd->display( m_alphanr );
    }
    else{
      salpha->setEnabled( false );
      alphalcd->setEnabled( false );
      alphalcd->display("OFF");
    }
}

/*********************************************/
void ObjectDialog::alphaDisplay( int number ){
   m_alphanr= ((double)number)*m_alphascale;
   alphalcd->display( m_alphanr );
}


/*********************************************/

void  ObjectDialog::commentUpdate(){
      objcomment->readComment();
}


/**********************************************/

void  ObjectDialog::commentClicked(bool on ){
  if (on){
    objcomment->show();
    //start Comment
    objcomment->readComment();
  }
  else{
    objcomment->hide();
  }
}

/*********************************************/

void ObjectDialog::showAll(){
  this->show();
  if(commentbutton ->isChecked() )
    objcomment->show();
}


void ObjectDialog::hideAll(){
  this->hide();
  if( commentbutton->isChecked() )
    objcomment->hide();
}


/**********************************************/
void ObjectDialog::updateTimefileList(bool refresh){
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::updateTimefileList called");
  if (refresh) METLIBS_LOG_DEBUG("refresh file list from disk");
#endif

  //clear box with list of files
  timefileList->clear();

  int index= namebox->currentRow();

  // get the list of object files
  if (index>=0 && index<int(objectnames.size()))
    files= m_ctrl->getObjectFiles(objectnames[index],refresh);
  else
    files.clear();

  int nr_file =  files.size();

  // Put times into vector, sort, and emit
  times.clear();
  for (int i=0; i<nr_file; i++)
    times.push_back(files[i].time);

  sort(times.begin(),times.end());

  if (autoButton->isChecked()) {
    emit emitTimes( "obj",times, true );
  } else {
    vector<miutil::miTime> noTimes; //Emit empty time list
    emit emitTimes( "obj",noTimes,false );
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
  if(timefileList->count()){
    timefileList->setCurrentRow(0);
  }

}


/*************************************************************************/

void ObjectDialog::updateSelectedFileList()
{

#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("updateSelectedFileList");
#endif

  //clear box with names of files
  selectedFileList->clear();

  std::string namestr;

  int index= namebox->currentRow();
  if(index<0) return;

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

#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("...namestr=" <<namestr);
#endif

}

/*************************************************************************/

vector<string> ObjectDialog::getOKString()
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::getOKstring");
#endif

  vector<string> vstr;

  if (selectedFileList->count()){
    std::string str;
    str = "OBJECTS";
    int index = namebox->currentRow();
    int timefileListIndex = timefileList->currentRow();
    if (index>-1 && index<int(objectnames.size())){
      //item has been selected in dialog
      str+=(" NAME=\"" + objectnames[index] + "\"");

      if ( timefileListIndex>-1 && timefileListIndex<int(files.size())) {

	ObjFileInfo file=files[timefileListIndex];

	if (timeButton->isChecked()){
	  miutil::miTime time=file.time;
	  if (!time.undef())
	    str+=(" TIME=" + stringFromTime(time));
	}
	else if (fileButton->isChecked()){
	  if (not file.name.empty())
	    str+=(" FILE=" + file.name);
	}
      }

    }

    str+=" types=";

    if (cbs0->isChecked()) str+="front,";
    if (cbs1->isChecked()) str+="symbol,";
    if (cbs2->isChecked()) str+="area,";
    if (cbs3->isChecked()) str+="anno";

    ostringstream ostr;
    ostr<<" timediff="<<m_totalminutes;
    if( alpha->isChecked() )
	ostr<<" alpha="<<m_alphanr;
    str += ostr.str();

    str+=plotVariables.external;

    vstr.push_back(str);

    // clear external variables
    plotVariables.external.clear();

  } //end if on etc.

  return vstr;


}


void ObjectDialog::putOKString(const vector<string>& vstr)
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::putOKstring");
#endif

  //clear plot
  DeleteClicked();

  // check PlotInfo (one for each plot)
  int npi= vstr.size();

  if (npi==0) return;

  // loop through all PlotInfo's
  for (int ip=0; ip<npi; ip++){
    //(if there are several plotInfos, only the last one will be
    //used
    vector<string> tokens = miutil::split_protected(vstr[ip], '"', '"');
    //get info from OKstring into struct PlotVariables
    plotVariables = decodeString(tokens);
  }


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

  if (!found) return;
  if (!plotVariables.time.empty()) {
    //METLIBS_LOG_DEBUG("time =" << plotVariables.time);
    int nt=files.size();
    for (int j=0;j<nt;j++ ){
      std::string listtime=stringFromTime(files[j].time);
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

  if (plotVariables.alphanr >=0){
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


ObjectDialog::PlotVariables ObjectDialog::decodeString(const vector<string> & tokens)
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::decodeString");
#endif

  PlotVariables okVar;
  okVar.totalminutes=-1;
  okVar.alphanr=1.0;

  int n= tokens.size();
  std::string token;

  //loop over OKstrings
  for (int i=0; i<n; i++){
    //decode string
    token= miutil::to_lower(tokens[i]);
    if (miutil::contains(token, "types=")){
      okVar.useobject = m_ctrl->decodeTypeString(token);
    } else {
      std::string key, value;
      vector<string> stokens= miutil::split(tokens[i], 0, "=");
      if ( stokens.size()==2) {
	key = miutil::to_lower(stokens[0]);
	value = stokens[1];
	if ( key=="name") {
	  if (value[0]=='"')
	    okVar.objectname = value.substr(1,value.length()-2);
	  else
	    okVar.objectname = value;
	}else if ( key=="time") {
	  okVar.time=value;
	} else if ( key=="file") {
	  okVar.file=value;
	} else if (key == "timediff" ) {
	  okVar.totalminutes = atoi(value.c_str());
	} else if ( key=="alpha" || key=="alfa") {
	  okVar.alphanr = atof(value.c_str());
	}else{
	  //anythig unknown, add to external string
	  okVar.external+=" " + tokens[i];
	}
      }else{
	//anythig unknown, add to external string
	okVar.external+=" " + tokens[i];
      }
    }
  }
  return okVar;
}


std::string ObjectDialog::getShortname()
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::getShortname");
#endif
  std::string name;

  int nameboxIndex = namebox->currentRow();
  int timefileListIndex = timefileList->currentRow();

  if ( selectedFileList->count() &&
     (autoButton->isChecked() || (timefileListIndex>=0 &&
			     timefileListIndex<int(files.size())))) {


    if (nameboxIndex > -1)
      name += "" + objectnames[nameboxIndex] + " ";
    else
      name+= (" FILE=") + std::string(selectedFileList->currentItem()->text().toStdString());
  }

  return name;
}


std::string ObjectDialog::makeOKString(PlotVariables & okVar)
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::makeOKString");
#endif


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

    ostringstream ostr;
    if (okVar.totalminutes >=0)
      ostr<<" timediff="<< okVar.totalminutes;
    if( okVar.alphanr>=0 )
      ostr<<" alpha="<<okVar.alphanr;
    str += ostr.str();

    //METLIBS_LOG_DEBUG("string from ObjectDialog::makeOKstring " << str);

    return str;

}



void ObjectDialog::archiveMode( bool on )
{
#ifdef dObjectDlg
  METLIBS_LOG_DEBUG("ObjectDialog::archiveMode called");
#endif
  useArchive= on;

  //get new Objectnames
  namebox->clear();
  objectnames= m_ctrl->getObjectNames(useArchive);
  for (unsigned int i=0; i<objectnames.size(); i++){
    namebox->addItem(objectnames[i].c_str());
  }

  //everything is unselected and listboxes refreshed
  DeleteClicked();
}


/*************************************************************************/

std::string ObjectDialog::stringFromTime(const miutil::miTime& t)
{
  ostringstream ostr;
  ostr << setw(4) << setfill('0') << t.year()
       << setw(2) << setfill('0') << t.month()
       << setw(2) << setfill('0') << t.day()
       << setw(2) << setfill('0') << t.hour()
       << setw(2) << setfill('0') << t.min();
  return ostr.str();
}


void ObjectDialog::closeEvent( QCloseEvent* e) {
  emit ObjHide();
}


void ObjectDialog::hideComment(){
  commentbutton->setChecked(false);
  objcomment->hide();
}
































