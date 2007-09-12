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
#include <qdialog.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qvbuttongroup.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qlcdnumber.h>
#include <qfiledialog.h>
#include <qtToggleButton.h>
#include <qtObjectDialog.h>
#include <qtEditComment.h>
#include <qtAddtoDialog.h>
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
  setCaption(tr("Weather Objects"));
   
  //initialization  

  useArchive=false;
//********** create the various QT widgets to appear in dialog ***********

  // combobox for selecting object
  namebox = new QListBox( this );
  namebox->setMinimumHeight(100);

  objectnames= m_ctrl->getObjectNames(useArchive);  
  for (int i=0; i<objectnames.size(); i++){
    namebox->insertItem(objectnames[i].c_str());
  }
  
  connect( namebox, SIGNAL(selectionChanged(QListBoxItem *) ), 
	   SLOT( nameActivated( ) ) );  
  namebox->setEnabled(true);
  m_nameboxIndex = -1;

  //**** the three buttons "auto", "tid", "fil" *************

  timefileBut = new QButtonGroup( this );
  autoButton = new ToggleButton(timefileBut, tr("Auto").latin1());
  timeButton = new ToggleButton(timefileBut, tr("Time").latin1());
  fileButton = new ToggleButton(timefileBut, tr("File").latin1());
  QHBoxLayout* timefileLayout = new QHBoxLayout(timefileBut);
  timefileLayout->addWidget(autoButton);
  timefileLayout->addWidget(timeButton);
  timefileLayout->addWidget(fileButton);
  timefileBut->setExclusive( true );
  timefileBut->setButton(0);
  timefileBut->setEnabled(false);
  //timefileClicked is called when auto,tid,fil buttons clicked 
  connect( timefileBut, SIGNAL( clicked(int) ),SLOT( timefileClicked(int) ) );

  //********** the list of files/times to choose from **************

  timefileList = new QListBox( this );
  timefileList->setMinimumHeight(100);

  connect( timefileList, SIGNAL( highlighted( int ) ), 
	   SLOT( timefileListSlot( int ) ) );
  m_timefileListIndex = -1;  

  //*****  Check boxes for selecting fronts/symbols/areas  **********

  bgroupobjects= new QVButtonGroup(this);


  cbs0= new QCheckBox(tr("Fronts"), bgroupobjects);
  cbs1= new QCheckBox(tr("Symbols"),bgroupobjects);
  cbs2= new QCheckBox(tr("Areas"), bgroupobjects);
  cbs3= new QCheckBox(tr("Form"), bgroupobjects);
  cbs0->setChecked(true);
  cbs1->setChecked(true);
  cbs2->setChecked(true);
  cbs3->setChecked(true);

  //the box (with label) showing which files have been choosen
  filesLabel = TitleLabel( tr("Selected files"), this);
  filenames = new QListBox( this );
  filenames->setMinimumHeight(40);

  //********* slider/lcd number showing max time difference **********

  //values for slider as in SatDialog
  int   timediff_minValue=0;
  int   timediff_maxValue=12;
  int   timediff_value=4;
  m_scalediff= 15;

  int difflength=timediff_maxValue/20 +5;

  diffLabel = new QLabel( tr("    Time diff."), this );
  diffLcdnum= LCDNumber( difflength, this);
  diffSlider= Slider( timediff_minValue, timediff_maxValue, 1, 
		      timediff_value, QSlider::Horizontal, this );

  connect(diffSlider,SIGNAL( valueChanged(int)),SLOT(doubleDisplayDiff(int))); 



  //********* slider/lcd number showing alpha cut **************

  int alpha_minValue = 0;
  int alpha_maxValue = 100;
  int alpha_value    = 100;
  m_alphascale = 0.01;


  alpha = new ToggleButton(this,tr("Alpha").latin1()); 
  connect( alpha, SIGNAL( toggled(bool)), SLOT( greyAlpha( bool) ));
  
  
  alphalcd = LCDNumber( 4, this);
  
  salpha  = Slider( alpha_minValue, alpha_maxValue, 1, alpha_value, 
			    QSlider::Horizontal, this);
  
  connect( salpha, SIGNAL( valueChanged( int )), 
	      SLOT( alphaDisplay( int )));
  
  // INITIALISATION 
  alphaDisplay( alpha_value );
  greyAlpha(false);
  
  //************************* standard Buttons ***************************** 


   //push buttons to delete all selections
  Delete = NormalPushButton( tr("Delete"), this );
  connect( Delete, SIGNAL(clicked()), SLOT(DeleteClicked()));

  //push button to refresh filelistsw
  refresh =NormalPushButton( tr("Refresh"), this );
  connect( refresh, SIGNAL( clicked() ), SLOT( Refresh() )); 

  //push button to show help
  objhelp = NormalPushButton( tr("Help"), this);  
  connect(  objhelp, SIGNAL(clicked()), SLOT( helpClicked()));    

  //toggle button for comments 
  commentbutton = new ToggleButton(this,tr("Comments").latin1());     
  connect(  commentbutton, SIGNAL(toggled(bool)), 
	    SLOT( commentClicked(bool) ));


  //push button to hide dialog
  objhide = NormalPushButton( tr("Hide"), this);
  connect( objhide, SIGNAL(clicked()), SIGNAL(ObjHide()));

   //push button to apply the selected command and then hide dialog
  objapplyhide = NormalPushButton(tr("Apply+Hide"), this );
  connect( objapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));
 
  //push button to apply the selected command
  objapply = NormalPushButton( tr("Apply"), this );
  connect(objapply, SIGNAL(clicked()), SIGNAL( ObjApply()) );

//   //push button for new files
//   newfilebutton = NormalPushButton( tr("Velg ny fil"), this);      
//   connect(  newfilebutton, SIGNAL(clicked()), 
// 	    SLOT( newfileClicked() ));

//   //push button for new dialog selections
//   addtodialogbutton = NormalPushButton(tr("Legg til dialogvalg"), this);      
//   connect(  addtodialogbutton, SIGNAL(clicked()), 
// 	    SLOT( addtodialogClicked() ));


// ********************* place all the widgets in layouts ****************

  //place "auto","tid","fil" buttons group and list of times/files in
  //vertical layout
  v3layout = new QVBoxLayout( 5 );
  v3layout->addWidget( timefileBut );
  v3layout->addWidget( timefileList );

  // place file name box and label in vertical layout
  v5layout = new QVBoxLayout( 5 );
  v5layout->addWidget( filesLabel );
  v5layout->addWidget( filenames );


  //"delete" and "refresh" buttons in hor.layout
  hlayout = new QHBoxLayout( 5 );
  hlayout->addWidget( Delete );
  //  hlayout->addWidget( refresh );

  //place lcd and slider in horizontal layout
  QGridLayout* gridlayout = new QGridLayout( 3, 2); 
  //  difflayout = new QHBoxLayout(5);
  gridlayout->addWidget( diffLabel,  0,0 );
  gridlayout->addWidget( diffLcdnum, 0,1 );
  gridlayout->addWidget( diffSlider, 0,2  );

  //  alphalayout = new QHBoxLayout(5);
  gridlayout->addWidget( alpha,    1,0 );
  gridlayout->addWidget( alphalcd, 1,1 );  
  gridlayout->addWidget( salpha,   1,2 );


  //place "help" button in horizontal layout
  hlayout3 = new QHBoxLayout( 5 );
  hlayout3->addWidget( objhelp );
  hlayout3->addWidget( refresh  );
  hlayout3->addWidget( commentbutton );

  //place buttons "utfør", "help" etc. in horizontal layout
  hlayout2 = new QHBoxLayout( 5 );
  hlayout2->addWidget( objhide );
  hlayout2->addWidget( objapplyhide );
  hlayout2->addWidget( objapply );

//   hlayout4 = new QHBoxLayout(5);
//   hlayout4->addWidget( newfilebutton );
//   hlayout4->addWidget( addtodialogbutton );


  //now create a vertical layout to put all the other layouts in
  vlayout = new QVBoxLayout( this,  5, 5);                            
  vlayout->addWidget( namebox ); 
  vlayout->addLayout( v3layout ); 
  vlayout->addLayout( v5layout ); 
  vlayout->addWidget(bgroupobjects);
  vlayout->addLayout( hlayout );
//   vlayout->addLayout( difflayout );
//   vlayout->addLayout( alphalayout );
  vlayout->addLayout( gridlayout );
  vlayout->addLayout( hlayout3 );
  vlayout->addLayout( hlayout2 );
  // vlayout->addLayout( hlayout4 );


  objcomment = new EditComment( this, m_ctrl,false );
  connect(objcomment,SIGNAL(CommentHide()),SLOT(hideComment()));
  objcomment->hide();
  
  atd = new AddtoDialog(this,m_ctrl);


   //set the selected prefix and get time file list
  filenames->clear();
  // initialisation and default of timediff
  doubleDisplayDiff(timediff_value);
  
  //end of constructor
}



/*********************************************/
void ObjectDialog::timefileClicked(int tt){
/* This function is called when timefileBut (auto/time/file)is selected*/
#ifdef dObjectDlg 
  cerr<<"ObjectDialog::timefileClicked called,tt =" << tt << endl;
#endif


  //update list of files
  updateTimefileList(false);

  //update the filenames box
  updateFilenames();

  if (tt==0) {

    //AUTO clicked

    //makes time animation work without pressing "utfør/apply"
    m_ctrl->setObjAuto(true);
    //timefilelist is the list of times/files when "auto" is chosen 
    //it should not be possible to choose something from there 
    timefileList->setEnabled( false );

  } else  {

    //stops time animation working without pressing "utfør/apply"
    m_ctrl->setObjAuto(false);
     //timefilelist is the list of times/files when "tid/fil" is chosen 
    //it should  be possible to choose something from there 
    timefileList->setEnabled ( true );

  }

}


/*********************************************/
void ObjectDialog::timefileListSlot( int index ){
/* DESCRIPTION: This function is called when the signal highlighted() is
sent from the list of time/file and a new list item is highlighted
*/
#ifdef dObjectDlg
   cerr<<"SatDialog::timefileListSlot called"<<endl;
   cerr<<"index "<<index<<endl;
   cerr<<"m_timefileListIndex"<<m_timefileListIndex<<endl;
#endif

   if( index<0 ){
#ifdef dObjectDlg
     cerr<<"index er mindre enn 0"<<endl; 
#endif
     return;
   }

   //set currently selected index
   m_timefileListIndex = index;
   //update file time or name in filenames box
   updateFilenames();

   //
   times.clear();
   times.push_back(files[index].time);
   emit emitTimes( "obj",times,false );
}


/*********************************************/
void ObjectDialog::nameActivated( ){
/* DESCRIPTION: This function is called when a value in namebox is 
 selected (region names or file prefixes), and is returned without doing 
anything if the new value  selected is equal to the old one.
 (HK ?? not yet) */

#ifdef dObjectDlg
     cerr<<"ObjectDialog::nameActivated called"<<endl;
#endif

     m_nameboxIndex = namebox->currentItem();

     if (m_nameboxIndex>-1){
       timefileBut->setEnabled(true);
       //update the time/file list
       timefileClicked(timefileBut->id(timefileBut->selected()));
     }
}

/***************************************************************************/

void ObjectDialog::DeleteClicked(){
  //called when delete/slett button is called
  //unselects and unhighlights everything
#ifdef dObjectDlg 
    cerr<<"ObjectDialog::DeleteClicked called"<<endl;
#endif 
    namebox->clear();
    //clearFocus, necessary to make sure no item
    //selected in namebox
    namebox->clearFocus();

    //new Objectnames
    objectnames= m_ctrl->getObjectNames(useArchive);
    for (int i=0; i<objectnames.size(); i++){
      namebox->insertItem(objectnames[i].c_str());
    }
    namebox->setEnabled(true);
    m_nameboxIndex = -1;

    //set the timefile button to auto and disable it
    timefileBut->setEnabled(false);
    autoButton->setOn(true);

    //clear the list of files
    timefileList->clear();
    //no selection of time/file yet
    m_timefileListIndex = -1;

    //clear the box with name of file
    filenames->clear();

    //Emit empty time list
    times.clear();
    emit emitTimes("obj",times,false );

    namebox->setFocus();

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

  m_timefileListIndex = -1;  

  //update the filenames box
  updateFilenames();

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
  emit showdoc("ug_objectdialogue.html");
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

void  ObjectDialog::newfileClicked(){
#ifdef dObjectDlg 
  cerr << "ObjectDialog::newfileClicked !" << endl;
#endif
    //help user by putting in some probable choices  
    miString newdir="/opdata/diana/Bakkeanalyse/DNMI-objects/";
    miString filestring;
    if (m_nameboxIndex>-1){ 
      if (files.size()) filestring=files[0].name;   
    }
    else if (filenames->count())
      filestring= filenames->currentText().latin1();
    if (!filestring.empty()){
      vector <miString> parts= filestring.split('/');
      int l =parts.back().length();
      int n = filestring.length();
      newdir = filestring.substr(0,n-l);
    }
    //user input
    QFileDialog *fdialog = new QFileDialog;
    QString file =
      fdialog->getOpenFileName(newdir.c_str(),"*.*");
    delete fdialog;
    if ( file.isEmpty() )
      return; 
    //clear all choices
    DeleteClicked();
    //put into filenames box
    filenames->insertItem(file);
    filenames->setCurrentItem( filenames->count() - 1 );
}

/*********************************************/

void  ObjectDialog::addtodialogClicked(){
  /* DESCRIPTION: This function is called when addtoDialog button is clicked.
  A dialog box where user can input dialog name and file string is shown. If
  OK is clicked this choice is added to the dialog   
  */
#ifdef dObjectDlg 
  cerr << "ObjectDialog::addtodialogClicked !" << endl;
#endif
  //help user by putting in some probable choices  
  miString file="/opdata/diana/Bakkeanalyse/DNMI-Objects/ANAdraw";
  miString name= "MIN Analyse";
  miString filestring;
  if (m_nameboxIndex>-1){ 
    name=objectnames[m_nameboxIndex];
    if (files.size()) filestring=files[0].name;   
  }
  else if (filenames->count())
    filestring= filenames->currentText().latin1();
  vector <miString> parts= filestring.split('.');
  if (parts.size()==2) file = parts[0];    
  atd-> putName(name);
  atd-> putFile(file);
  if (atd->exec()){
    name = atd->getName();
    file = atd->getFile();
    m_objm->insertObjectName(name,file);
    DeleteClicked(); //updates namebox !
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

  //no selection of time/file yet
  m_timefileListIndex = -1;

  //clear box with list of files 
  timefileList->clear();

  // get the list of object files
  int index= namebox->currentItem();

  if (index>=0 && index<objectnames.size())
    files= m_ctrl->getObjectFiles(objectnames[index],refresh);
  else
    files.clear();

  times.clear();
  int nr_file =  files.size();

  // Put times into vector, sort, and emit
  for (int i=0; i<nr_file; i++)
    times.push_back(files[i].time);

  sort(times.begin(),times.end());

  //HK ??? emitTimes call differs tt=0, tt=1..
  if (autoButton->isOn()) {
    emit emitTimes( "obj",times, true );
  } else {
    //Emit empty time list
    vector<miTime> noTimes;
    emit emitTimes( "obj",noTimes,false );
  }
 
  if (autoButton->isOn()) {

    filenames->clear();

    if (index>=0 && index<objectnames.size())
      filenames->insertItem(objectnames[index].c_str());

  } else if (timeButton->isOn()) {

    //make a string list with times to insert into timefileList
    const char** ltid= new const char*[nr_file];
    miString* tt= new miString[nr_file];
      
    for (int i=0; i<nr_file; i++){
      tt[i]=files[i].time.isoTime();
      ltid[i]= tt[i].c_str();
    }

    //insert into timefilelist
    timefileList->insertStrList( ltid, nr_file );

    delete[] tt;
    delete[] ltid;

  } else if (fileButton->isOn()) {
      
    const char** lfil= new const char*[nr_file];

    for (int i=0; i<nr_file; i++)
      lfil[i]= files[i].name.c_str();

    timefileList->insertStrList( lfil, nr_file );
      
    delete[] lfil;

  }
}


/*************************************************************************/

void ObjectDialog::updateFilenames()
{

#ifdef dObjectDlg
  cerr <<"updateFilenames" << endl;
#endif

  //clear box with names of files
  filenames->clear();

  miString namestr;

  if (autoButton->isOn()) {
    int index= namebox->currentItem();
    if (index > -1 )
      namestr= objectnames[index];
  } else if (timeButton->isOn() && m_timefileListIndex>=0) {
    int index= namebox->currentItem();
    if (index > -1 ) 
      namestr= objectnames[index] + " ";
    namestr+= files[m_timefileListIndex].time.isoTime();
  } else if (fileButton->isOn() && m_timefileListIndex>=0) {
    namestr= files[m_timefileListIndex].name;
  }

  if (!namestr.empty()) {
    filenames->insertItem(namestr.c_str());
    filenames->setCurrentItem( filenames->count() - 1 );
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

  if (filenames->count()){
    miString str;
    str = "OBJECTS";
    int index = m_nameboxIndex;
    if (index>-1 && index<objectnames.size()){
      //item has been selected in dialog
      str+=(" NAME=\"" + objectnames[index] + "\"");

      if ( m_timefileListIndex>-1 && m_timefileListIndex<files.size()) {

	ObjFileInfo file=files[m_timefileListIndex];

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
    //     else {  HK??? comment out 14/12/05 
    //       //file chosen in filedialog
    //       miString filestring= filenames->currentText().latin1();
    //       str+= " FILE=" + filestring;	
    //     }

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

//     //add plotoptions 
//     PlotOptions po=m_objm->getPlotOptions(objectnames[index]);
//     //only for shapefiles so far
//     if (!po.palettename.empty() )
//       str+=" palettecolours=" + po.palettename; 
//     if (!po.fname.empty() )
//       str+=" fname=" + po.fname; 
//     int pfn=po.fdescr.size();
//     if (pfn){
//       str+=" fdescr='";
// 	for (int i=0;i<pfn;i++){
// 	  str+=po.fdescr[i];
// 	  if (i<pfn-1)
// 	    str+=":";
// 	  else
// 	    str+="'";
// 	}
//     }


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
  bool found;
  // only one plotVariables - one plot
  if (plotVariables.objectname.empty()){
    //HK ??? new ########################################################
    //if only a file is selected- put it in filenames
    if (!plotVariables.file.empty()) {
      cerr << "only file selected -" << plotVariables.file << endl;
      //put into filenames box
      filenames->insertItem(plotVariables.file.c_str());
      filenames->setCurrentItem( filenames->count() - 1 );
    }    
    //###################################################################
  } else {
    //cerr << "objectname =" << plotVariables.objectname << endl;
    int nc = namebox->count();
    for (int j=0;j<nc;j++ ){
      miString listname =  namebox->text(j).latin1();
      if (plotVariables.objectname==listname){
	namebox->setSelected(j,true);
	nameActivated();
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
	  timefileBut->setButton(1);
	  timefileList->setSelected(j,true);
	}
      }
    } else if (!plotVariables.file.empty()) {
      //cerr << "file =" << plotVariables.file << endl;
      int nf = files.size();
      for (int j=0;j<nf;j++ ){
	miString listfile =  files[j].name;
	if (plotVariables.file==listfile){
	  timefileBut->setButton(2);
	  timefileList->setSelected(j,true);
	}
      } 
    }    
  }
  if (plotVariables.alphanr >=0){
    //cerr << "alpha =" << plotVariables.alphanr << endl;
    //HK ??? warning 
    //float alphavalue = plotVariables.alphanr/m_alphascale;
    int alphavalue = int(plotVariables.alphanr/m_alphascale + 0.5);
    salpha->setValue(  alphavalue );
    alpha->setOn(true);
    greyAlpha( true );
  }
  if (plotVariables.totalminutes >=0){
    //cerr << "totalminutes =" << plotVariables.totalminutes << endl;
    //HK ??? warning 
    //float number= int(plotVariables.totalminutes/m_scalediff + 0.5);
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
  //okVar.alphanr=-1.0;
  //default is 1.0
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
  for ( int i=0; i<oldstr.size(); i++)
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

  if ( filenames->count() &&
     (autoButton->isOn() || (m_timefileListIndex>=0 &&  
				m_timefileListIndex<files.size()))) {

    
    if (m_nameboxIndex > -1)
      name += "" + objectnames[m_nameboxIndex] + " ";
    else
      name+= (" FILE=") + miString(filenames->currentText().latin1());	
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


bool ObjectDialog::close(bool alsoDelete){
  emit ObjHide();
  return true;
} 


void ObjectDialog::hideComment(){
  commentbutton->setOn(false);
  objcomment->hide();
}
































