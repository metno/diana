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

#include <qtToggleButton.h>
#include <qtObjectDialog.h>
#include <qtEditComment.h>
#include <diObjectManager.h>
#include <qtUtility.h>

/***************************************************************************/
ObjectDialog::ObjectDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), m_ctrl(llctrl)
{
#ifdef dObjectDlg
  cout<<"ObjectDialog::ObjectDialog called"<<endl;
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

  autoButton = new ToggleButton(this, tr("Auto").toStdString());
  timeButton = new ToggleButton(this, tr("Time").toStdString());
  fileButton = new ToggleButton(this, tr("File").toStdString());
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
  int   timediff_maxValue=12;
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


  alpha = new ToggleButton(this,tr("Alpha").toStdString());
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
  commentbutton = new ToggleButton(this,tr("Comments").toStdString());
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
  cerr<<"ObjectDialog::nameListClicked called"<<endl;
#endif

  //update the time/file list
  timefileClicked(timefileBut->checkedId());

}

/*********************************************/
void ObjectDialog::timefileClicked(int tt){
/* This function is called when timefileBut (auto/time/file)is selected*/
#ifdef dObjectDlg
  cerr<<"ObjectDialog::timefileClicked called,tt =" << tt << endl;
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
   cerr<<"SatDialog::timefileListSlot called"<<endl;
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
    cerr<<"ObjectDialog::DeleteClicked called"<<endl;
#endif

    if(namebox->currentItem())
      namebox->currentItem()->setSelected(false);

    timefileList->clear();

    selectedFileList->clear();

    //Emit empty time list
    times.clear();
    emit emitTimes("obj",times,false );

#ifdef dObjectDlg
    cerr<<"ObjectDialog::DeleteClicked returned"<<endl;
#endif
  return;
}


/*********************************************/

void ObjectDialog::Refresh(){
#ifdef dObjectDlg
  cerr<<"ObjectDialog::Refresh() called; Filene blir hentet nytt fra disken"
      <<endl;
#endif

  //update the timefileList
  updateTimefileList(true);

  //update the selectedFileList box
  updateSelectedFileList();

}


/********************************************/
void ObjectDialog::applyhideClicked(){
#ifdef dObjectDlg
  cerr<<"applyhideClicked()"<<endl;
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
    miString str= ostr.str();
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
  if(commentbutton ->isOn() )
    objcomment->show();
}


void ObjectDialog::hideAll(){
  this->hide();
  if( commentbutton->isOn() )
    objcomment->hide();
}


/**********************************************/
void ObjectDialog::updateTimefileList(bool refresh){
#ifdef dObjectDlg
  cerr<<"ObjectDialog::updateTimefileList called" <<endl;
  if (refresh) cerr << "refresh file list from disk" << endl;
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

  if (autoButton->isOn()) {
    emit emitTimes( "obj",times, true );
  } else {
    vector<miTime> noTimes; //Emit empty time list
    emit emitTimes( "obj",noTimes,false );
  }

  //update time/file list
  if (timeButton->isOn()) {

    for (int i=0; i<nr_file; i++){
      timefileList->addItem(QString(files[i].time.isoTime().cStr()));
    }

  } else if (fileButton->isOn()) {

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
  cerr <<"updateSelectedFileList" << endl;
#endif

  //clear box with names of files
  selectedFileList->clear();

  miString namestr;

  int index= namebox->currentRow();
  if(index<0) return;

  int timefileListIndex = timefileList->currentRow();

  if (autoButton->isOn()) {
    namestr= objectnames[index];
  } else if (timeButton->isOn() && timefileListIndex>-1) {
    namestr= objectnames[index] + " ";
    namestr+= files[timefileListIndex].time.isoTime();
  } else if (fileButton->isOn() && timefileListIndex>-1) {
    namestr= files[timefileListIndex].name;
  }

  if (!namestr.empty()) {
    selectedFileList->addItem(QString(namestr.c_str()));
    selectedFileList->item( selectedFileList->count() - 1 )->setSelected(true);
  }

#ifdef dObjectDlg
  cerr << "...namestr=" <<namestr << endl;
#endif

}

/*************************************************************************/

vector<miString> ObjectDialog::getOKString(){
#ifdef dObjectDlg
  cerr << "ObjectDialog::getOKstring" << endl;
#endif

  vector<miString> vstr;

  if (selectedFileList->count()){
    miString str;
    str = "OBJECTS";
    int index = namebox->currentRow();
    int timefileListIndex = timefileList->currentRow();
    if (index>-1 && index<int(objectnames.size())){
      //item has been selected in dialog
      str+=(" NAME=\"" + objectnames[index] + "\"");

      if ( timefileListIndex>-1 && timefileListIndex<int(files.size())) {

	ObjFileInfo file=files[timefileListIndex];

	if (timeButton->isOn()){
	  miTime time=file.time;
	  if (!time.undef())
	    str+=(" TIME=" + stringFromTime(time));
	}
	else if (fileButton->isOn()){
	  if (file.name.exists())
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
    if( alpha->isOn() )
	ostr<<" alpha="<<m_alphanr;
    str += ostr.str();

    str+=plotVariables.external;

    vstr.push_back(str);

    // clear external variables
    plotVariables.external.clear();

  } //end if on etc.

  return vstr;


}


void ObjectDialog::putOKString(const vector<miString>& vstr)
{
#ifdef dObjectDlg
  cerr << "ObjectDialog::putOKstring" << endl;
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
    vector<miString> tokens= vstr[ip].split('"','"');
    //get info from OKstring into struct PlotVariables
    plotVariables = decodeString(tokens);
  }


  //update dialog
  bool found=false;
  int nc = namebox->count();
  for (int j=0;j<nc;j++ ){
    miString listname =  namebox->item(j)->text().toStdString();
    if (plotVariables.objectname==listname){
      namebox->setCurrentRow(j);
      namebox->item(j)->setSelected(true);
      nameListClicked(namebox->item(j));
      found=true;
    }
  }

  if (!found) return;
  if (!plotVariables.time.empty()) {
    //cerr << "time =" << plotVariables.time << endl;
    int nt=files.size();
    for (int j=0;j<nt;j++ ){
      miString listtime=stringFromTime(files[j].time);
      if (plotVariables.time==listtime){
	timefileBut->button(1)->setChecked(true);
	timefileClicked(1);
	timefileList->item(j)->setSelected(true);
      }
    }
  } else if (!plotVariables.file.empty()) {
    //cerr << "file =" << plotVariables.file << endl;
    int nf = files.size();
    for (int j=0;j<nf;j++ ){
      miString listfile =  files[j].name;
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
    //cerr << "alpha =" << plotVariables.alphanr << endl;
    int alphavalue = int(plotVariables.alphanr/m_alphascale + 0.5);
    salpha->setValue(  alphavalue );
    alpha->setOn(true);
    greyAlpha( true );
  }
  if (plotVariables.totalminutes >=0){
    //cerr << "totalminutes =" << plotVariables.totalminutes << endl;
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


ObjectDialog::PlotVariables
ObjectDialog::decodeString(const vector <miString> & tokens)
{
#ifdef dObjectDlg
  cerr << "ObjectDialog::decodeString" << endl;
#endif

  PlotVariables okVar;
  okVar.totalminutes=-1;
  okVar.alphanr=1.0;

  int n= tokens.size();
  miString token;

  //loop over OKstrings
  for (int i=0; i<n; i++){
    //decode string
    token= tokens[i].downcase();
    if (token.contains("types=")){
      okVar.useobject = m_ctrl->decodeTypeString(token);
    } else {
      miString key, value;
      vector<miString> stokens= tokens[i].split('=');
      if ( stokens.size()==2) {
	key = stokens[0].downcase();
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


void ObjectDialog::requestQuickUpdate(const vector<miString>& oldstr,
				      vector<miString>& newstr){
#ifdef dObjectDlg
  cerr << "ObjectDialog::requestQuickUpdate" << endl;
  cerr << "start requestQuickUpdate" << endl;
  for(int i = 0;i<oldstr.size();i++)
    cerr << " oldstr = " << oldstr[i] << endl;
  for(int i = 0;i<newstr.size();i++)
    cerr << " newstr = " << newstr[i] << endl;
#endif


  //returner hvis ulikt antall på strengene...
  if (!oldstr.size() || oldstr.size() != newstr.size()){
    newstr=oldstr;
    return;
  }

  bool diff = false;
  for ( unsigned int i=0; i<oldstr.size(); i++)
    if ( oldstr[i] != newstr[i] ){
      diff = true;
      break;
    }
  if ( !diff ) return;

  vector<miString> oldtokens= oldstr;
  vector<miString> newtokens= newstr;
  newstr.clear();

  // Structs containing info from OKstring
  PlotVariables oldOkVar = decodeString(oldtokens);
  PlotVariables newOkVar = decodeString(newtokens);

  //check allowed changes
  if (oldOkVar.objectname !=newOkVar.objectname
      && !oldOkVar.objectname.contains("@") ){
    cerr << "requestQuickUpdate, changes not accepted !(1)" << endl;
    newstr=oldstr;
    return;
  }
  if(oldOkVar.time!=newOkVar.time || oldOkVar.file!=newOkVar.file){
    cerr << "requestQuickUpdate, changes not accepted !(2)" << endl;
    newstr=oldstr;
    return;
  }
  if(oldOkVar.totalminutes!=newOkVar.totalminutes){
    cerr << "requestQuickUpdate, changes not accepted !(3)" << endl;
    newstr=oldstr;
    return;
  }
  if(oldOkVar.useobject!=newOkVar.useobject){
    cerr << "requestQuickUpdate, changes not accepted !(4)" << endl;
    newstr=oldstr;
    return;
  }
  oldOkVar.alphanr=newOkVar.alphanr;

  miString resultstr= makeOKString(oldOkVar);
  newstr.push_back(resultstr);

#ifdef dObjectDlg
  cerr << "End requestQuickUpdate" << endl;
  for(int i = 0;i<oldstr.size();i++)
    cerr << " oldstr = " << oldstr[i] << endl;
  for(int i = 0;i<newstr.size();i++)
    cerr << " newstr = " << newstr[i] << endl;
#endif
}



miString ObjectDialog::getShortname()
{
#ifdef dObjectDlg
  cerr << "ObjectDialog::getShortname" << endl;
#endif
  miString name;

  int nameboxIndex = namebox->currentRow();
  int timefileListIndex = timefileList->currentRow();

  if ( selectedFileList->count() &&
     (autoButton->isOn() || (timefileListIndex>=0 &&
			     timefileListIndex<int(files.size())))) {


    if (nameboxIndex > -1)
      name += "" + objectnames[nameboxIndex] + " ";
    else
      name+= (" FILE=") + miString(selectedFileList->currentItem()->text().toStdString());
  }

  return name;
}


miString ObjectDialog::makeOKString(PlotVariables & okVar){
#ifdef dObjectDlg
  cerr << "ObjectDialog::makeOKString" << endl;
#endif


    miString str;
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

    //cerr << "string from ObjectDialog::makeOKstring " << str << endl;

    return str;

}



void ObjectDialog::archiveMode( bool on )
{
#ifdef dObjectDlg
  cerr<<"ObjectDialog::archiveMode called"<<endl;
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

miString ObjectDialog::stringFromTime(const miTime& t){

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
  commentbutton->setOn(false);
  objcomment->hide();
}
































