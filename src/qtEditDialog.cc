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
#include <iostream>
#include <q3vbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <q3listbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
// qt4 fix
//#include <qvbuttongroup.h>
#include <qtabwidget.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qinputdialog.h>

#include <qtEditDialog.h>
#include <qtEditNewDialog.h>
#include <qtEditComment.h>
#include <qtUtility.h>
#include <qtToggleButton.h>
#include <qtTimeStepSpinbox.h>
#include <qtComplexText.h>
#include <qtAnnoText.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3Frame>
#include <Q3VBoxLayout>

#include <diSetupParser.h>
#include <diController.h>
#include <diEditManager.h>
#include <diObjectManager.h>

#include <edit_open_value.xpm>
#include <edit_lock_value.xpm>

//####################################################
#ifdef DEBUGPRINTONCE
  static bool printalltools= true;
#endif
//####################################################

/*********************************************/
#define HEIGHTLISTBOX 120
#define FIELD_INDEX 0
#define OBJECT_INDEX 1

#define FRONT_INDEX 0
#define SYMBOL_INDEX 1
#define AREA_INDEX 2
#define SIGMAP_INDEX 3

/*********************************************/
EditDialog::EditDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), m_ctrl(llctrl), m_editm(0)
{
#ifdef dEditDlg 
  cout<<"EditDialog::EditDialog called"<<endl;
#endif

  TABNAME_FIELD= tr("Field");
  TABNAME_OBJECTS= tr("Objects");
  TABNAME_COMBINE= tr("Merge");

  EditDialogInfo ll=llctrl->initEditDialog();

  //list of translations to appear in dialog:
  editTranslations["Cold front"]=tr("Cold front"); //Kaldfront
  editTranslations["Warm front"]=tr("Warm front"); //Varmfront
  editTranslations["Occlusion"]=tr("Occlusion"); //Okklusjon
  editTranslations["Cold occlusion"]=tr("Cold occlusion"); //Kald okklusjon
  editTranslations["Warm occlusion"]=tr("Warm occlusion"); //Varm okklusjon
  editTranslations["Stationary front"]=tr("Stationary front"); //Stasjonær front
  editTranslations["Trough"]=tr("Trough"); //Tråg
  editTranslations["Squall line"]=tr("Squall line"); //Bygelinje
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.vær

  editTranslations["Low pressure"]=tr("Low pressure"); //Lavtrykk
  editTranslations["High pressure"]=tr("High pressure"); //Høytrykk
  editTranslations["Cold"]=tr("Cold"); //Kald
  editTranslations["Warm"]=tr("Warm"); //Varm
  editTranslations["Fog"]=tr("Fog"); //Tåke
  editTranslations["Drizzle"]=tr("Drizzle "); //yr
  editTranslations["Freezing drizzle"]=tr("Freezing drizzle"); //Yr som fryser
  editTranslations["Freezing rain"]=tr("Freezing rain"); //Regn som fryser
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Rain showers"]=tr("Rain showers"); //Regnbyger
  editTranslations["Sleet showers"]=tr("Sleet showers"); //Sluddbyger ??
  editTranslations["Hail showers"]=tr("Hail showers"); //Haglbyger
  editTranslations["Snow showers"]=tr("Snow showers"); //Haglbyger
  editTranslations["Thunderstorm"]=tr("Thunderstorm"); //Tordenvær
  editTranslations["Thunderstorm with hail"]=tr("Thunderstorm with hail"); //Tordenvær m/hagl
  editTranslations["Snow"]=tr("Snow"); //Snø(stjerne)    ??
  editTranslations["Rain"]=tr("Rain"); //Regn
  editTranslations["Sleet"]=tr("Sleet"); //Sludd

  editTranslations["Hurricane"]=tr("Hurricane"); //Tropisk orkan ??

  editTranslations["Disk"]=tr("Disk"); //
  editTranslations["Circle"]=tr("Circle"); //Sirkel
  editTranslations["Cross"]=tr("Cross");   //Kryss
  editTranslations["Text"]=tr("Text");   //Tekster


  editTranslations["Precipitation"]=tr("Precipitation"); //Nedbør ??
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Clouds"]=tr("Clouds"); //Skyer
  editTranslations["Fog"]=tr("Fog"); //Tåke
  editTranslations["Ice"]=tr("Ice"); //Is
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.vær
  editTranslations["Generic area"]=tr("Generic area"); //

  ConstructorCernel( ll );

 
}



/*********************************************/
void EditDialog::ConstructorCernel( const EditDialogInfo mdi )
{
#ifdef dEditDlg 
  cout<<"EditDialog::ConstructorCernel called"<<endl;
#endif

  m_editm= m_ctrl->getEditManager();
  m_objm=  m_ctrl->getObjectManager();

  setCaption(tr("Editing"));

  inEdit= false;
  productApproved= false;
  combineAction= -1;
  fieldIndex= -1;
  fieldEditToolGroup= 0;
  numFieldEditTools=  0;
  currFieldEditToolIndex= 0;

  openValuePixmap= QPixmap(edit_open_value_xpm);
  lockValuePixmap= QPixmap(edit_lock_value_xpm);

  bgroup = new Q3ButtonGroup( 3, Qt::Horizontal, this );
  int m_nr_buttons=3;
  b = new QPushButton*[m_nr_buttons];
  vector<miString> vstr(3);
  vstr[prodb]=tr("Product").latin1();
  vstr[saveb]=tr("Save").latin1();
  vstr[sendb]=tr("Send").latin1();

  int i;
 
  for( i=0; i< m_nr_buttons; i++ ){
    b[i] = NormalPushButton( vstr[i].c_str(), bgroup );
    b[i]->setFocusPolicy(Qt::ClickFocus);
  }

  b[prodb]->setFocusPolicy(Qt::StrongFocus);
  b[saveb]->setEnabled(false);
  b[sendb]->setEnabled(false);

  // ********** TAB
  twd = new QTabWidget( this );

  FieldTab();
  FrontTab();
  CombineTab(); 

  twd->setEnabled(false); // initially disabled

  connect( twd, SIGNAL(selected( const QString& )),
           SLOT( tabSelected( const QString& ) ));
  // **********

  //Spinbox for observation time step
  timelabel= new QLabel( tr("Obs. timestep:"), this );
  timestepspin= new TimeStepSpinbox(this, "timestepspin");
  timestepspin->setMinValue(1);
  timestepspin->setValue(1);
  stepchanged(1);
  connect(timestepspin, SIGNAL(valueChanged(int)), SLOT(stepchanged(int)));
   
  //toggle button for comments dialog
  pausebutton = new ToggleButton( this, tr("Pause").latin1() );      
  connect(  pausebutton, SIGNAL(toggled(bool)), 
	    SLOT( pauseClicked(bool) ));
  pausebutton->setOn(false);


  //toggle button for comments dialog
  commentbutton = new ToggleButton( this, tr("Comments").latin1());
  connect(  commentbutton, SIGNAL(toggled(bool)), 
	    SLOT( commentClicked(bool) ));

  Q3HBoxLayout* h2layout = new Q3HBoxLayout( 5 );
  h2layout->addWidget(timelabel);
  h2layout->addWidget(timestepspin);
  h2layout->addWidget(pausebutton);
  h2layout->addWidget(commentbutton);

  editexit = NormalPushButton(tr("Exit"), this );
  connect(  editexit, SIGNAL(clicked()), SLOT( exitClicked() ));
  
  // qt4 fix: QButton -> QPushButton
  QPushButton* edithide = NormalPushButton(tr("Hide"), this );
  connect( edithide, SIGNAL(clicked()), SIGNAL(EditHide()));

  edithelp = NormalPushButton(tr("Help"), this );
  connect(  edithelp, SIGNAL(clicked()), SLOT( helpClicked() ));

  connect( bgroup, SIGNAL(clicked(int)), SLOT(groupClicked( int ))  );

  prodlabel = new QLabel("" , this);
  prodlabel->setMaximumHeight(40);
  lStatus = new QLabel("", this);

  Q3VBoxLayout* lvlayout= new Q3VBoxLayout();
  lvlayout->addWidget(prodlabel);
  lvlayout->addWidget(lStatus);

  Q3HBoxLayout* hlayout = new Q3HBoxLayout( 5 );

  hlayout->addWidget(editexit);
  hlayout->addWidget(edithide);
  hlayout->addWidget(edithelp);
  
  // vlayout
  vlayout = new Q3VBoxLayout( this, 5, 5 );
  vlayout->addLayout( lvlayout,1);
  vlayout->addWidget( bgroup );
  vlayout->addWidget( twd );
  vlayout->addLayout( h2layout );
  vlayout->addLayout( hlayout );

  vlayout->activate(); 
  vlayout->freeze();

  enew = new EditNewDialog( static_cast<QWidget*>(parent()), m_ctrl );
  enew->hide();
  connect(enew,
	  SIGNAL(EditNewOk(EditProduct&, EditProductId&, miTime&)),
	  SLOT(EditNewOk(EditProduct&, EditProductId&, miTime&)));
  connect(enew,
	  SIGNAL(EditNewCombineOk(EditProduct&, EditProductId&, miTime&)),
	  SLOT(EditNewCombineOk(EditProduct&, EditProductId&, miTime&)));
  connect(enew, SIGNAL(EditNewHelp()), SLOT(helpClicked()));
  connect(enew, SIGNAL(EditNewCancel()), SLOT(EditNewCancel()));
  connect(enew, SIGNAL(newLogin(editDBinfo&)), SLOT(newLogin(editDBinfo&)));

  ecomment = new EditComment( this, m_ctrl,true );
  connect(ecomment,SIGNAL(CommentHide()),SLOT(hideComment()));
  ecomment->hide();

  mb = new QMessageBox(tr("New analysis"),
		       tr("This will delete all your edits so far.\n Do you really want them to disappear?"),
		       QMessageBox::Warning,
		       QMessageBox::Yes | QMessageBox::Default,
		       QMessageBox::Cancel | QMessageBox::Escape,
		       Qt::NoButton,
		       this);
  mb->setButtonText( QMessageBox::Yes, tr("New") );
  mb->setButtonText( QMessageBox::Cancel, tr("Cancel")); 


}//end constructor EditDialog



// --------------------------------------------------------------
// --------------- FieldTab methods -----------------------------
// --------------------------------------------------------------

void  EditDialog::FieldTab()
{
  int mymargin=5;
  int myspacing=5;
    
  fieldtab = new Q3VBox(twd);
  fieldtab->setMargin( mymargin );
  fieldtab->setSpacing( myspacing );

  fgroup = new Q3ButtonGroup( maxfields, Qt::Horizontal, fieldtab );
  fbutton = new QPushButton*[maxfields];

  for (int i=0; i<maxfields; i++) {
    fbutton[i]= new QPushButton( "    ", fgroup );
    int height = fbutton[i]->sizeHint().height();
    fbutton[i]->setMinimumHeight( height );
    fbutton[i]->setMaximumHeight( height );
    fbutton[i]->setEnabled(false);
  }
  numfields= 0;

  connect( fgroup, SIGNAL(clicked(int)), SLOT(fgroupClicked(int)) );

  m_Fieldeditmethods = new Q3ListBox(fieldtab);
  m_Fieldeditmethods->setMinimumHeight(HEIGHTLISTBOX);

  connect( m_Fieldeditmethods, SIGNAL( highlighted(int) ),
           SLOT( FieldEditMethods(int) ) );

//  // double-click
//  connect( m_Fieldeditmethods, SIGNAL( selected(int) ),
//           SLOT( FieldEditMethods(int) ) );

  // set default
//m_Fieldeditmethods->setCurrentItem(0);
    
  bgroupinfluence= new Q3ButtonGroup(2,Qt::Vertical,fieldtab);

  rbInfluence[0]= new QRadioButton(QString(tr("Circle")),         bgroupinfluence);
  rbInfluence[1]= new QRadioButton(QString(tr("Square")),         bgroupinfluence);
  rbInfluence[2]= new QRadioButton(QString(tr("Ellipse(centre)")),bgroupinfluence);
  rbInfluence[3]= new QRadioButton(QString(tr("Ellipse(focus)")), bgroupinfluence);

  bgroupinfluence->setExclusive(TRUE);

  rbInfluence[1]->setEnabled(false);

  connect (bgroupinfluence, SIGNAL(clicked(int)),
           SLOT(changeInfluence(int)));

  // set default (dialog and use)
  bgroupinfluence->setButton(0);
  changeInfluence(0); // needed as the above does not change anything
    
  Q3HBox* ehbox = new Q3HBox(fieldtab);
  ehbox->setMargin( mymargin );
  ehbox->setSpacing( myspacing/2 );
  
  QLabel* ellipseform = new QLabel(tr("Ellipse shape"), ehbox );

  ellipsenumbers.clear();
  int i;
  for (i=20; i<70; i+=10) ellipsenumbers.push_back(float(i)/100.);
  for (i=70; i<85; i+=5)  ellipsenumbers.push_back(float(i)/100.);
  for (i=85; i<100; i++)  ellipsenumbers.push_back(float(i)/100.);

  ellipsenumber = new QLabel( "    ", ehbox );
  ellipsenumber->setMinimumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setMaximumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setFrameStyle( Q3Frame::Box | Q3Frame::Plain);
  ellipsenumber->setLineWidth(2);
  ellipsenumber->setAlignment( Qt::AlignCenter | Qt::TextExpandTabs );

  int n= ellipsenumbers.size()-1;
  int index= (n+1)/2;

  ellipsenumber->setNum( double(ellipsenumbers[index]) );

  ellipseslider  = new QSlider( 0, n, 1, index, Qt::Horizontal, ehbox);
  ellipseslider->setMinimumHeight( 16 );
  ellipseslider->setMaximumHeight( 16 );
  ellipseslider->setEnabled( true );
     
  connect( ellipseslider, SIGNAL( valueChanged( int )), 
           SLOT( fieldEllipseChanged( int)));

  connect( ellipseslider, SIGNAL( sliderReleased() ), 
           SLOT( fieldEllipseShape()) );

  // set default
  fieldEllipseShape();

  // enable/disable extra editing lines
  exlineCheckBox= new QCheckBox(tr("Show extra editing lines"), fieldtab);						
  exlineCheckBox->setChecked( false );
  exlineCheckBox->setEnabled( true );

  connect( exlineCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( exlineCheckBoxToggled(bool) ) );

  // NOT USED YET....
  // QCheckBox* visible = new QCheckBox( "synlig", fieldtab );

  Q3HBox* hbox = new Q3HBox(fieldtab);
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );

  undoFieldButton = NormalPushButton( tr("Undo"), hbox);
  redoFieldButton = NormalPushButton( tr("Redo"), hbox);

  undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( false );

  connect( undoFieldButton, SIGNAL(clicked()), SLOT(undofield()));
  connect( redoFieldButton, SIGNAL(clicked()), SLOT(redofield()));
    
  twd->addTab( fieldtab, TABNAME_FIELD );
}


void EditDialog::fgroupClicked( int index )
{
  if (index!=fieldIndex) {
    m_editm->activateField(index);
    fieldIndex= index;
  }
}


void EditDialog::undoFieldsEnable()
{
  undoFieldButton->setEnabled(true);
  redoFieldButton->setEnabled(false);
}


void EditDialog::undoFieldsDisable()
{
  undoFieldButton->setEnabled(false);
  redoFieldButton->setEnabled(false);
}


void EditDialog::undofield()
{
  EditEvent ee;
  ee.type= edit_undo;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  if (!m_editm->notifyEditEvent(ee))
    undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::redofield()
{
  EditEvent ee;
  ee.type= edit_redo;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  if(!m_editm->notifyEditEvent(ee))
    redoFieldButton->setEnabled( false );
  undoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::changeInfluence( int index )
{
  EditEvent ee;
  if      (index==0) ee.type= edit_circle;
  else if (index==1) ee.type= edit_square;
  else if (index==2) ee.type= edit_ellipse1;
  else if (index==3) ee.type= edit_ellipse2;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
  if (inEdit) emit editUpdate();
}


void EditDialog::fieldEllipseChanged( int index )
{
  ellipsenumber->setNum( double(ellipsenumbers[index]) );
}


void EditDialog::fieldEllipseShape()
{
  int index= ellipseslider->value();
  EditEvent ee;
  ee.type= edit_ecellipse;
  ee.order= normal_event;
  ee.x= ellipsenumbers[index];
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
  if (inEdit) emit editUpdate();
}


void EditDialog::exlineCheckBoxToggled(bool on)
{
  EditEvent ee;
  ee.type= (on) ? edit_exline_on : edit_exline_off;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
}


void EditDialog::FieldEditMethods( int index )
{
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::FieldEditMethods(index)  index= "<<index<<endl;
#endif

  int numClassValues= classValues.size();

//------------------------------------------------------
  if (index<numFieldEditTools) {
//------------------------------------------------------

#ifdef DEBUGPRINTONCE
cerr<<"FieldEditMethods.1 count,numEt,ncv,index,currIndex: "
    <<m_Fieldeditmethods->count()
    <<" "<<numFieldEditTools
    <<" "<<numClassValues<<" "<<index
    <<" "<<currFieldEditToolIndex<<endl;
#endif

//####################################################
#ifdef DEBUGPRINTONCE
    if (printalltools) {
      printalltools= false;
      cerr<<"-----------------------------------------------------------"<<endl;
      cerr<<"m,n,i"<<endl;
      cerr<<"m_EditDI.mapmodeinfo[m].mapmode"<<endl;
      cerr<<"m_EditDI.mapmodeinfo[m].editmodeinfo[n].editmode"<<endl;
      cerr<<"m_EditDI.mapmodeinfo[m].editmodeinfo[n].edittools[i].index"<<endl;
      cerr<<"m_EditDI.mapmodeinfo[m].editmodeinfo[n].edittools[i].name"<<endl;
      for (int m=0; m<m_EditDI.mapmodeinfo.size(); m++) {
        for (int n=0; n<m_EditDI.mapmodeinfo[m].editmodeinfo.size(); n++) {
          for (int i=0; i<m_EditDI.mapmodeinfo[m].editmodeinfo[n].edittools.size(); i++) {
            cerr<<m<<" "<<n<<" "<<setw(2)<<i<<" : "
                <<m_EditDI.mapmodeinfo[m].mapmode<<"  "
                <<m_EditDI.mapmodeinfo[m].editmodeinfo[n].editmode<<" "<<setw(3)
                <<m_EditDI.mapmodeinfo[m].editmodeinfo[n].edittools[i].index<<" = "
                <<m_EditDI.mapmodeinfo[m].editmodeinfo[n].edittools[i].name<<endl;
          }
        }
      }
      cerr<<"-----------------------------------------------------------"<<endl;
    }
#endif
//####################################################

    currFieldEditToolIndex= index;

    currMapmode= m_EditDI.mapmodeinfo[0].mapmode;

    currEditmode = m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].editmode;

    currEdittool=  m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].name;

    int tool= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].index;

#ifdef DEBUGREDRAW
    cerr<<"currMapmode,currEditmode,currEdittool,tool: "
        <<currMapmode<<" "<<currEditmode<<" "<<currEdittool<<" "<<tool<<endl;
#endif

//  if (inEdit) m_editm->setEditMode(currMapmode,currEditmode,currEdittool);
    m_editm->setEditMode(currMapmode,currEditmode,currEdittool);

    EditEvent ee;

    ee.type= editType(tool);
    ee.order= normal_event;
    ee.x= 0.;
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
	     index<numFieldEditTools+numClassValues) {

#ifdef DEBUGPRINTONCE
cerr<<"FieldEditMethods.2 count,numEt,ncv,index,currIndex: "
    <<m_Fieldeditmethods->count()
    <<" "<<numFieldEditTools
    <<" "<<numClassValues<<" "<<index
    <<" "<<currFieldEditToolIndex<<endl;
#endif

    currFieldEditToolIndex= index;

    int n= index - numFieldEditTools;
    EditEvent ee;

    ee.type= edit_class_value;
    ee.order= normal_event;
    ee.x= classValues[n];
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
  	     index<numFieldEditTools+numClassValues*2) {

#ifdef DEBUGPRINTONCE
cerr<<"FieldEditMethods.3 count,numEt,ncv,index,currIndex: "
    <<m_Fieldeditmethods->count()
    <<" "<<numFieldEditTools
    <<" "<<numClassValues<<" "<<index
    <<" "<<currFieldEditToolIndex<<endl;
#endif

    int n= index - numFieldEditTools - numClassValues;
    EditEvent ee;

    m_Fieldeditmethods->blockSignals(true);

//  m_Fieldeditmethods->clearSelection();

    m_Fieldeditmethods->setCurrentItem(currFieldEditToolIndex);
//  m_Fieldeditmethods->setSelected(currFieldEditToolIndex,true);

    if (classValuesLocked[n]) {
      m_Fieldeditmethods->changeItem(openValuePixmap,classNames[n].cStr(),index);
      classValuesLocked[n]= false;
      ee.type= edit_open_value;
    } else {
      m_Fieldeditmethods->changeItem(lockValuePixmap,classNames[n].cStr(),index);
      classValuesLocked[n]= true;
      ee.type= edit_lock_value;
    }

    m_Fieldeditmethods->blockSignals(false);

    ee.order= normal_event;
    ee.x= classValues[n];
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  }
}



// --------------------------------------------------------------
// ------------- ObjectTab methods ------------------------------
// --------------------------------------------------------------


void  EditDialog::FrontTab()
{
  int mymargin=5;
  int myspacing=5;

  objecttab = new Q3VBox(twd );
  objecttab->setMargin( mymargin );
  objecttab->setSpacing( myspacing );

  vector<miString> vstr;
  m_Frontcm = ComboBox( objecttab, vstr );
  connect( m_Frontcm, SIGNAL( activated(int) ),
           SLOT( FrontTabBox(int) ) );

  m_Fronteditmethods = new Q3ListBox(objecttab);

  connect( m_Fronteditmethods, SIGNAL(clicked(Q3ListBoxItem *) ),
           SLOT( FrontEditClicked() ) );
  connect( m_Fronteditmethods, SIGNAL(doubleClicked(Q3ListBoxItem *) ),
           SLOT( FrontEditDoubleClicked() ) );

  Q3HBox* hbox = new Q3HBox(objecttab);
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );

  undoFrontButton = NormalPushButton( tr("Undo"), hbox);
  redoFrontButton = NormalPushButton( tr("Redo"), hbox);

  connect( undoFrontButton, SIGNAL(clicked()), SLOT(undofront()));
  connect( redoFrontButton, SIGNAL(clicked()), SLOT(redofront()));

  autoJoin = new QCheckBox( tr("Join fronts"), objecttab);
  autoJoin->setChecked(true);
  connect(autoJoin, SIGNAL(toggled(bool)), SLOT(autoJoinToggled(bool)));

  // initialize colours and ok state
  autoJoinToggled(true);

  twd->addTab( objecttab, TABNAME_OBJECTS);
}


void  EditDialog::FrontTabBox( int index )
{
  // called when an item in objects combo box selected
  if (index!=m_FrontcmIndex || m_Fronteditmethods->count()==0){
    m_FrontcmIndex=index;
    ListBoxData( m_Fronteditmethods, 1, index);
    m_FronteditIndex=-1;
    m_Fronteditmethods->setSelected(0,true);
  } else if (m_FronteditIndex < m_Fronteditmethods->count()-1){
    m_Fronteditmethods->setSelected(m_FronteditIndex,true);
  }
  currEditmode= miString(m_Frontcm->text(m_FrontcmIndex).latin1()); 
  FrontEditClicked();
  return;
}



void EditDialog::FrontEditClicked()
{
  //cerr << "FrontEditClicked "  << endl; 
  //called when an item in the objects list box clicked
  if (!inEdit) return;
  int index = m_Fronteditmethods->currentItem();
  if (m_FronteditList.size()>index) currEdittool= m_FronteditList[index];
  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  if (index!=m_FronteditIndex){
    m_FronteditIndex=index;
    if (m_objm->inTextMode()){
      miString text = m_objm->getCurrentText();
      Colour::ColourInfo colour= m_objm->getCurrentColour();  
      if (text.empty()){
	if (getText(text,colour)){
	  m_objm->setCurrentText(text);
	  m_objm->setCurrentColour(colour);
	}
      }   
    }
    else if (m_objm->inComplexTextMode()){
      vector <miString> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      if (getComplexText(symbolText,xText)){
	m_objm->setCurrentComplexText(symbolText,xText);
      } 
    }
  }
  m_objm->createNewObject();
}



void EditDialog::FrontEditDoubleClicked()
{
  //called when am item in the objects list box doubleclicked
  if (m_objm->inTextMode()){
    miString text = m_objm->getCurrentText();   
    Colour::ColourInfo colour=m_objm->getCurrentColour();
    if (getText(text,colour)){
      //change objectmanagers current text !
      m_objm->setCurrentText(text);
      m_objm->setCurrentColour(colour);
    }   
  } else if (m_objm->inComplexTextMode()){
    vector <miString> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    if (getComplexText(symbolText,xText)){
      m_objm->setCurrentComplexText(symbolText,xText);
    } 
  }
  //create new object 
  if (inEdit)
    m_objm->createNewObject();
}


void EditDialog::undoFrontsEnable()
{
  undoFrontButton->setEnabled(true);
  redoFrontButton->setEnabled(false);
}


void EditDialog::undoFrontsDisable()
{
  undoFrontButton->setEnabled(false);
  redoFrontButton->setEnabled(false);
}


void EditDialog::undofront()
{
  m_editm->showAllObjects();
  if (!m_objm->undofront())
    undoFrontButton->setEnabled( false );
  redoFrontButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::redofront()
{
  m_editm->showAllObjects();
  if (!m_objm->redofront())
    redoFrontButton->setEnabled( false );
  undoFrontButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}



void EditDialog::autoJoinToggled(bool on)
{
  m_objm->autoJoinToggled(on);
}



void EditDialog::EditMarkedText()
{
  //called from shortcut ctrl-e in main window
  //changes all marked texts and objectmanagers current text !
  vector <miString> symbolText,xText,eText;
  miString text = m_objm->getMarkedText();
  if (!text.empty()){
    //get new text from inputdialog box
    Colour::ColourInfo colour=m_objm->getMarkedColour();
    if (getText(text,colour)){
      m_objm->changeMarkedText(text);
      m_objm->changeMarkedColour(colour);
      m_objm->setCurrentText(text);
      m_objm->setCurrentColour(colour);
    }    
  } 
  //text from annotations
  text = m_ctrl->getMarkedAnnotation();
  if (!text.empty()){
    eText.push_back(text);
    AnnoText * aText =new AnnoText(this,m_ctrl,m_editm->getProductName(), eText,xText);
    connect(aText,SIGNAL(editUpdate()),SIGNAL(editUpdate()));
    m_ctrl->startEditAnnotation();
    aText->exec(); 
    delete aText;
  }   
  m_objm->getMarkedComplexText(symbolText,xText);
  if (getComplexText(symbolText,xText))
    m_objm->changeMarkedComplexText(symbolText,xText);      
}

void EditDialog::DeleteMarkedAnnotation()
{
  m_ctrl->DeleteMarkedAnnotation();
}


bool EditDialog::getText(miString & text, Colour::ColourInfo & colour)
{
  bool ok = false;

  vector <miString> symbolText,xText;
  symbolText.push_back(text);
  set <miString> textList=m_objm->getTextList();
  ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText,
				       textList,true);
  cText->setColour(colour);
  if (cText->exec()){
    cText->getComplexText(symbolText,xText);
    cText->getColour(colour);
    if (symbolText.size())
      text=symbolText[0];
    ok=true;
  } 
  delete cText;	         

  return ok;
}


bool EditDialog::getComplexText(vector <miString> & symbolText,
				vector <miString> & xText)
{
  bool ok=false;
  if (symbolText.size()||xText.size()){
    set <miString> complexList = m_ctrl->getComplexList();
    ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText,
					   complexList);
    if (cText->exec()){
      cText->getComplexText(symbolText,xText);
      ok=true;
    } 
    delete cText;	          
  }
  return ok;
}


// --------------------------------------------------------------
// ------------- CombineTab methods -----------------------------
// --------------------------------------------------------------

void  EditDialog::CombineTab()
{
  const int mymargin= 5;
  const int myspacing= 5;

  combinetab = new Q3VBox(twd );
  combinetab->setMargin( mymargin );
  combinetab->setSpacing( myspacing );

  Q3GroupBox* group= new Q3GroupBox(2,Qt::Vertical,tr("Editing"), combinetab);
  Q3VButtonGroup* bg= new Q3VButtonGroup(group);
  bg->setFrameStyle(Q3Frame::NoFrame);
  QRadioButton* rb1= new QRadioButton(tr("Change borders"),bg,0);
  QRadioButton* rb2= new QRadioButton(tr("Set data sources"),bg,0);
  rb1->setChecked(true);
  connect(bg, SIGNAL(clicked(int)), SLOT(combine_action(int)));

  m_SelectAreas = new Q3ListBox(group);//listBox( group, 150, 75, false );
  m_SelectAreas->setMinimumHeight(100);

  connect( m_SelectAreas, SIGNAL( highlighted(int) ),
	   SLOT( selectAreas(int) ) );

  Q3HBox* hbox = new Q3HBox(combinetab);
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );
  combinetab->setStretchFactor(hbox, 20);

  stopCombineButton = new QPushButton( tr("Exit merge"), combinetab);
  connect(stopCombineButton, SIGNAL(clicked()), SLOT(stopCombine()));

  twd->addTab( combinetab, TABNAME_COMBINE );
}

void EditDialog::stopCombine()
{
  cerr << "EditDialog::stopCombine called" << endl;

  twd->setTabEnabled(fieldtab, true);
  twd->setTabEnabled(objecttab, true);
  twd->setCurrentPage(0);
  twd->setTabEnabled(combinetab, false);
  //not possible to save or send until combine stopped
  b[saveb]->setEnabled(true);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(true);
#endif
  m_editm->stopCombine();
}

void EditDialog::combine_action(int idx)
{
  if (idx == combineAction) return;
  combineAction= idx;
  CombineEditMethods();
}

void EditDialog::selectAreas(int index)
{
  miString tmp= miString( m_SelectAreas->text(index).latin1());
  if (tmp !=currEdittool){
    currEdittool= tmp;
    if (inEdit) m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  }
  if (inEdit) m_objm->createNewObject();
}

void EditDialog::CombineEditMethods()
{
  if (combineAction<0){ // first time
    currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[0].editmode;
    currEdittool= m_EditDI.mapmodeinfo[2].editmodeinfo[0].edittools[0].name;
    m_SelectAreas->setEnabled(false);
  } else if (combineAction==0){ // region border editing
    m_SelectAreas->setEnabled(false);
    currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[0].editmode;
    currEdittool= m_EditDI.mapmodeinfo[2].editmodeinfo[0].edittools[0].name;
    if (inEdit) m_objm->setAllPassive();
  } else if (combineAction==1){ // region selections
    m_SelectAreas->setEnabled(true);
    currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[1].editmode;
    currEdittool= miString( m_SelectAreas->text(m_SelectAreas->currentItem()).latin1());
    if (inEdit) m_objm->createNewObject();
  } else {
    cerr << "EditDialog::CombineEditMethods    unknown combineAction:"
	 << combineAction << endl;
    return;
  }
  if (inEdit)
    m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
}

// --------------------------------------------------------------
// ------------- Common methods ---------------------------------
// --------------------------------------------------------------

void EditDialog::tabSelected( const QString& tabname)
{
#ifdef DEBUGREDRAW
    cerr<<"EditDialog::tabSelected:"<<tabname<<endl;
#endif
  if (tabname == TABNAME_FIELD) {
    //unmark all objects when changing mapMode
    m_objm->editNotMarked();
#ifdef DEBUGREDRAW
    if (!inEdit) cerr<<"EditDialog::tabSelected emit editUpdate()...(1)"<<endl;
#endif
    if (!inEdit) emit editUpdate();
    if (m_EditDI.mapmodeinfo.size()>0){
      currMapmode= m_EditDI.mapmodeinfo[0].mapmode;
      FieldEditMethods(m_Fieldeditmethods->currentItem());
    }
  } else if (tabname == TABNAME_OBJECTS) {
    if (m_EditDI.mapmodeinfo.size()>1){
      currMapmode= m_EditDI.mapmodeinfo[1].mapmode;
      m_Frontcm->setCurrentItem(m_FrontcmIndex);
      FrontTabBox(m_FrontcmIndex);
    }
  } else if (tabname == TABNAME_COMBINE) {
    if (m_EditDI.mapmodeinfo.size()>2){
      currMapmode= m_EditDI.mapmodeinfo[2].mapmode;
      CombineEditMethods();
    }
  }
  // do a complete redraw - with underlay saving
#ifdef DEBUGREDRAW
  if (inEdit) cerr<<"EditDialog::tabSelected emit editUpdate()...(2)"<<endl;
#endif
  if (inEdit) emit editUpdate();
}


void  EditDialog::ListBoxData( Q3ListBox* list, int mindex, int index)
{
  list->clear();
  vector<miString> vstr;
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools.size();
  for ( int i=0; i<n; i++){
    miString etool=m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools[i].name;
    vstr.push_back(etool);
    QString dialog_etool;
    //find translation
    if (editTranslations.count(etool))
      dialog_etool =editTranslations[etool];
    else
      dialog_etool = etool.cStr();
    list->insertItem(dialog_etool);
  }
  list->setCurrentItem(0);
  if (mindex==OBJECT_INDEX)
    m_FronteditList=vstr; //list of edit tools 
  if (mindex==OBJECT_INDEX && index==SIGMAP_INDEX){
    //for now, only sigmap symbols have images...
    list->clear();
    list->setColumnMode(3);
    int w = list->width();
    int itemwidth=int (w/3.5);
    SetupParser sp;      
    for ( int i=0; i<n; i++){
      miString path = sp.basicValue("imagepath");
      miString filename = path+ m_FronteditList[i] + ".png";
      QImage* pimage = new QImage(filename.c_str());
      if (!pimage->isNull()){
	int pwidth=pimage->width();
	int pheight=pimage->height();
	float ratio = float(pwidth)/float(itemwidth);
	float fitemheight = float(pheight)/ratio;
	int itemheight= (int) fitemheight;
	QImage scaledpimage= pimage->smoothScale(itemwidth,itemheight);
	QPixmap * pmap= new QPixmap(scaledpimage);
	list->insertItem(*pmap);
	delete pmap;
      }else{
	list->insertItem(m_FronteditList[i].c_str());
      }
    delete pimage;
    }
  }
  return;
}


void EditDialog::ComboBoxData(QComboBox* box, int mindex)
{
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo.size();
  vector<miString> vstr;
  m_Frontcm->clear();
  for( int i=0; i<n; i++ ){
    if (m_EditDI.mapmodeinfo[mindex].editmodeinfo[i].edittools.size()){
      m_Frontcm->addItem(QString(m_EditDI.mapmodeinfo[1].editmodeinfo[i].editmode.cStr()));    
    }
  }

}


bool EditDialog::saveEverything(bool send)
{
  bool approved= false;
  
  if (send){
    switch(QMessageBox::information(this, tr("Send product"),
		tr("Start distribution of product to all regions.\n Use \"Approve produkt\" to give product official status\n as approved and ready."),
		tr("&Distribution only"), tr("&Approve product"), tr("&Cancel"),
		0,      // Enter == button 0
		2 ) ) { // Escape == button 2


    case 0: // "Kun distribusjon" clicked, Enter pressed.
      approved= false;
      break;
    case 1: // "Godkjenn produkt" clicked
      approved= true;
      break;
    case 2: // Cancel clicked, Escape pressed
      return false;
      break;
    }
  }
  
  ecomment->saveComment();
  miString message;
  bool res = m_editm->writeEditProduct(message,true,true,send,approved);

  if (!res){
    message= miString(tr("Problem saving/sending product\n").latin1()) + 
      miString(tr("Message from server:\n").latin1())
      + message;
      QMessageBox::warning( this, tr("Save error:"),
			  message.c_str());
    
    return false;
  }

  miTime t= miTime::nowTime();
  QString lcs(send ? " <font color=\"darkgreen\">"+tr("Saved") +"</font> "
	    : " <font color=\"black\">"+tr("saved")+"</font> ");
  QString tcs= QString("<font color=\"black\">")+
    QString(t.isoTime().cStr()) + QString("</font> ");
  
  QString qs= lcs + tcs;
  
  if (send && approved){
    productApproved= true;
    qs += " <font color=\"darkgreen\">"+ tr("and approved") +"</font> ";
  } else if (productApproved){
    qs += " <font color=\"red\">"+ tr("(approved)") + "</font> ";
  }

  lStatus->setText(qs);
  
  return true;
}


void  EditDialog::groupClicked( int id )
{
  switch (id){
  case 0:
    // start new product
    if (m_editm->unsavedEditChanges()){
      switch( mb->exec() ) { // ask if discard changes
      case QMessageBox::Cancel:
	return;
	break;
      }
    }
    // get productdefinitions etc. from Controller
    //if new-product-dialog already active, do nothing
    if (!enew->newActive)      {
	enew->load(dbi);
	// show start-new-product dialog
	enew->show();
      }
    break;    
  case 1:
    // Save all
    saveEverything(false);
    // show all objects if any hidden
    if (m_editm->showAllObjects()) emit editUpdate();
    break;

  case 2:
    // Send all
    saveEverything(true);
    // show all objects if any hidden
    if (m_editm->showAllObjects()) emit editUpdate();
    break;
  };
}


void  EditDialog::stepchanged(int step)
{
  m_ctrl->obsStepChanged(step);
}

void  EditDialog::commentClicked( bool on )
{
  if (inEdit){
    if (on){
      ecomment->show();
    }
    else{
      ecomment->hide();
    }
  }
  else
    //comment button should not be shown
    commentbutton->setOn(false);
}

void  EditDialog::pauseClicked( bool on )
{
  if (inEdit){
    m_editm->setEditPause(on); 
    emit editMode(!on);
  }
}


  
void EditDialog::showAll()
{
  if (inEdit){
    //show this dialog
    this->show();
    if(commentbutton ->isOn() )
      ecomment->show();
  } else{
    //load and start EditNewDialog
    enew->load(dbi);
    enew->show();
  }
}


void EditDialog::hideAll()
{
  this->hide();
  enew->hide();
  if( commentbutton->isOn() )
    ecomment->hide();
}


bool EditDialog::okToExit()
{
  //save comments to plotm->editObjects struct 
  ecomment->saveComment();
  if (m_editm->unsavedEditChanges()){
    raise(); //put dialog on top
    
    switch(QMessageBox::information(this,tr("Exit editing"),
					    tr("You have unsaved edits.\n Save before exiting?"),
					    tr("&Save"), tr("&Don't save"), tr("&Cancel"),
                                    0,2)){


    case 0: // save clicked
      saveEverything(false);
      break;
    case 1: // don't save, but exit
      break;
    case 2:
      return false; // cancel operation
      break;
    }
  }
  if(m_editm->unsentEditChanges()){
    raise(); //put dialog on top
    switch(QMessageBox::information(this,tr("Send analysis"),
				    tr("Send last saved analysis to the database?"),
				    tr("&Send"), tr("&Don't send"),0,1)){
    case 0: // send clicked
      saveEverything(true);
      break;
    case 1: // don't send, but exit
      break;
    }
  }
  return true;
}


bool EditDialog::cleanupForExit()
{
  if (!okToExit()) return false;
  ecomment->stopComment();
  m_editm->stopEdit();
  m_editm->logoutDatabase(dbi);
  
  return true;
}


void EditDialog::exitClicked()
{
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::exitClicked...................."<<endl;
#endif
  if (!cleanupForExit()) return;
  commentbutton->setOn(false);
  // update field dialog
  emit emitFieldEditUpdate("");
  m_editm->setEditMode("normal_mode","","");
  twd->setEnabled(false); // disable tab-widget
  b[saveb]->setEnabled(false);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(false);
#endif
  emit EditHide();
  emit editApply();
  // empty timeslider producttime
  vector<miTime> noTimes;
  emit emitTimes("product",noTimes);
  inEdit= false;
  productApproved= false;
  enew->newActive=false;
  // set labels
  updateLabels();
  lStatus->setText(" ");
  emit editMode(false);
}


void EditDialog::helpClicked()
{
  emit showdoc("ug_editdialogue.html"); 
}


void EditDialog::updateLabels()
{
  // update top-labels etc.
  miString s;
  if (inEdit)
    s= miString("<font color=\"darkgreen\">") +
      currprod.name + miString("</font>") +
      miString("<font color=\"blue\"> ") +
      currid.name + miString("</font>") +
      miString(" ") + prodtime.format("%D %H:%M");
  else
    s= "";
  
  prodlabel->setText(s.cStr());
}

void EditDialog::newLogin(editDBinfo& d)
{
  dbi= d;
  updateLabels();
}


void EditDialog::EditNewOk(EditProduct& ep,
			   EditProductId& ci,
			   miTime& time)
{
  cerr << "EditDialog::EditNewOk called................" << endl;

  emit editMode(true);

  // Turn off Undo-buttons
  undoFrontsEnable();
  undoFieldsDisable();
  // stop current edit
  m_editm->stopEdit();
  inEdit= false;
  productApproved= false;
  updateLabels();

  // the fields to edit
  numfields= ep.fields.size();
  for (int i=0; i<numfields; i++) {
    fbutton[i]->setText(ep.fields[i].name.c_str());
    fbutton[i]->setEnabled(true);
  }
  for (int i=numfields; i<maxfields; i++) {
    fbutton[i]->setText("    ");
    fbutton[i]->setEnabled(false);
  }
  // force field activate if last selected field was
  // the first one..
  if (fieldIndex==0) fieldIndex= -1;
  fgroupClicked( 0 );

  // update field dialog
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate(empty)"<<endl;
#endif
  emit emitFieldEditUpdate("");

  if (!m_editm->startEdit(ep,ci,time)) {
    cerr << "Error starting edit" << endl;
    emit editApply();
    return;
  }

  currprod= ep;
  currid= ci;
  prodtime= time;
  inEdit= true;
  // update Product and Id label..
  updateLabels();

  m_EditDI=m_editm->getEditDialogInfo();

  fieldEditToolGroup= 0;
  if (currprod.fields.size()>0) {
    // BUG YET: we only use the tool for the first field if more than one field!!!
    //  to be fixed later...
    if      (currprod.fields[0].editTools[0]=="standard") fieldEditToolGroup=0;
    else if (currprod.fields[0].editTools[0]=="classes")  fieldEditToolGroup=1;
    else if (currprod.fields[0].editTools[0]=="numbers")  fieldEditToolGroup=2;
  }

  if (fieldEditToolGroup==0) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  } else if (fieldEditToolGroup==1) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(true);
    rbInfluence[2]->setEnabled(false);
    rbInfluence[3]->setEnabled(false);
  } else if (fieldEditToolGroup==2) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  }

  numFieldEditTools= 0;  // don't know yet!
  classNames.clear();
  classValues.clear();
  classValuesLocked.clear();

  if (fieldEditToolGroup!=1) {

    //Fill field edit listbox
    ListBoxData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    m_Fieldeditmethods->blockSignals(true);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      miString ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->insertItem(ts.cStr());
    }

    numFieldEditTools= n;

    miString str= m_ctrl->getFieldClassSpecifications(currprod.fields[0].name);

    vector<miString> vclass= str.split(',');
    for (int i=0; i<vclass.size(); i++) {
      vector<miString> vs= vclass[i].split(":");
      if (vs.size()>=2) {
        classNames.push_back(vs[1]);
	classValues.push_back(atof(vs[0].cStr()));
	classValuesLocked.push_back(false);
      }
    }
    classNames.push_back(tr("Undefined").latin1());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (int i=0; i<classNames.size(); i++) {
      miString estr= tr("New value:").latin1() +  classNames[i];
      m_Fieldeditmethods->insertItem(estr.cStr());
    }

    for (int i=0; i<classNames.size(); i++) {
      m_Fieldeditmethods->insertItem(openValuePixmap,classNames[i].cStr());
    }

    m_Fieldeditmethods->blockSignals(false);
    currFieldEditToolIndex=0;
    m_Fieldeditmethods->setCurrentItem(currFieldEditToolIndex);

    m_Fieldeditmethods->triggerUpdate(true);
  }

//########################################################################
  EditEvent ee;
  if (fieldEditToolGroup==2)
    ee.type= edit_show_numbers_on;
  else
    ee.type= edit_show_numbers_off;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
//########################################################################

  //Fill object edit combobox
  ComboBoxData(m_Frontcm,1);
  //Clear object edit listbox and set indices to zero 
  m_Fronteditmethods->clear();
  m_FrontcmIndex=0;
  m_FronteditIndex=-1;

  //find out which tabs to enable and current tab
  int mm;
  if (numfields >0)
    mm=0;
  else
    mm=1;
  twd->setEnabled(true); // enable tab-widget
  if (mm==0){
    twd->setTabEnabled(fieldtab, true);
  } else{
    twd->setTabEnabled(fieldtab, false);
    FrontTabBox(0);
  }
  twd->setTabEnabled(objecttab, currprod.objectsFilenamePart.exists() );

  twd->setTabEnabled(combinetab, false);
  if (twd->currentPageIndex()!=mm) twd->setCurrentPage(mm);

  b[saveb]->setEnabled(true);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(true);
#endif

  commentbutton->setEnabled(currprod.commentFilenamePart.exists() );

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miTime t;
  m_editm->getProductTime(t);
  vector<miTime> Times;
  Times.push_back(t);
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::EditNewOk emit emitTimes(product): "<<Times[0]<<endl;
#endif
  emit emitTimes("product",Times);

  // update field dialog
  for (int i=0; i<currprod.fields.size(); i++){
    if (currprod.fields[i].fromfield){
      // this will remove the original field in the field dialog
#ifdef DEBUGREDRAW
      cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate"<<endl;
#endif
      emit emitFieldEditUpdate(currprod.fields[i].fromfname);
    } else {
      // add a new selected field in the field dialog
#ifdef DEBUGREDRAW
      cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate...new"<<endl;
#endif
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  }

  this->show();
  //qt4 fix
  tabSelected(twd->tabText(twd->currentIndex()));

  ecomment->stopComment();
  ecomment->startComment();
  pausebutton->setOn(false);

  if (ep.OKstrings.size()){
#ifdef DEBUGREDRAW
    cerr<<"EditDialog::EditNewOk emit Apply(ep.OKstrings)"<<endl;
#endif
    //apply commands for this EditProduct (probably MAP)
    m_ctrl->keepCurrentArea(false); // unset area conservatism
    emit Apply(ep.OKstrings,false);
    m_ctrl->keepCurrentArea(true); // reset area conservatism
  } else {
//  m_ctrl->keepCurrentArea(true); // reset area conservatism
#ifdef DEBUGREDRAW
    cerr<<"EditDialog::EditNewOk emit editApply()"<<endl;
#endif
    emit editApply();
//  m_ctrl->keepCurrentArea(false); // reset area conservatism
  }

#ifdef DEBUGREDRAW
  cerr<<"REMOVED EditDialog::EditNewOk emit editUpdate()"<<endl;
#endif
//emit editUpdate();

#ifdef DEBUGREDRAW
  cerr << "EditDialog::EditNewOk finished...................." << endl;
#endif
}


void EditDialog::EditNewCombineOk(EditProduct& ep,
				  EditProductId& ci,
				  miTime& time)
{
  cerr << "EditNewCombineOK" << endl;
  // Turn off Undo-buttons
  undoFrontsEnable();
  undoFieldsDisable();
  // stop current edit
  m_editm->stopEdit();
  inEdit= false;
  productApproved= false;
  updateLabels();

  // start combine+edit without autojoin
  autoJoin->setChecked(false);
  autoJoinToggled(false);

  // the fields to edit when ending combinations
  numfields= ep.fields.size();
  for (int i=0; i<numfields; i++) {
    fbutton[i]->setText(ep.fields[i].name.c_str());
    fbutton[i]->setEnabled(true);
  }
  for (int i=numfields; i<maxfields; i++) {
    fbutton[i]->setText("    ");
    fbutton[i]->setEnabled(false);
  }
  // force field activate if last selected field was
  // the first one..
  if (fieldIndex==0) fieldIndex= -1;
  fgroupClicked( 0 );

  // update field dialog
  emit emitFieldEditUpdate("");

  vector<miString> combids;
  // try to start combine
  if (!m_editm->startCombineEdit(ep,ci,time,combids)){
    cerr << "Error starting combine" << endl;
    emit editApply();
    return;
  }

  // put combids into select-areas listbox
  m_SelectAreas->clear();
  for (int i=0; i<combids.size(); i++){
    m_SelectAreas->insertItem(QString(combids[i].cStr()));
  }
  if(combids.size()) m_SelectAreas->setCurrentItem(0);

  inEdit= true;
  currprod= ep;
  currid= ci;
  prodtime= time;
  // update label etc..
  updateLabels();

  m_EditDI=m_editm->getEditDialogInfo();

  fieldEditToolGroup= 0;
  if (currprod.fields.size()>0) {
    // BUG YET: we only use the tool for the first field if more than one field!!!
    //  to be fixed later...
    if      (currprod.fields[0].editTools[0]=="standard") fieldEditToolGroup=0;
    else if (currprod.fields[0].editTools[0]=="classes")  fieldEditToolGroup=1;
    else if (currprod.fields[0].editTools[0]=="numbers")  fieldEditToolGroup=2;
  }

  //ListBoxData( m_Fieldeditmethods, 0, fieldEditToolGroup);
 
  if (fieldEditToolGroup==0) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  } else if (fieldEditToolGroup==1) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(true);
    rbInfluence[2]->setEnabled(false);
    rbInfluence[3]->setEnabled(false);
  } else if (fieldEditToolGroup==2) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  }

  numFieldEditTools= 0;  // don't know yet!
  classNames.clear();
  classValues.clear();
  classValuesLocked.clear();

  if (fieldEditToolGroup!=1) {

    //Fill field edit listbox
    ListBoxData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    m_Fieldeditmethods->blockSignals(true);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      miString ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->insertItem(ts.cStr());
    }

    numFieldEditTools= n;

    miString str= m_ctrl->getFieldClassSpecifications(currprod.fields[0].name);

    cerr<<" class str: "<<str<<endl;

    vector<miString> vclass= str.split(',');
    for (int i=0; i<vclass.size(); i++) {
      vector<miString> vs= vclass[i].split(":");
      if (vs.size()>=2) {
        classNames.push_back(vs[1]);
	classValues.push_back(atof(vs[0].cStr()));
	classValuesLocked.push_back(false);
      }
    }
    classNames.push_back(tr("Undefined").latin1());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (int i=0; i<classNames.size(); i++) {
      miString estr= tr("New value:").latin1() +  classNames[i];
      m_Fieldeditmethods->insertItem(estr.cStr());
    }

    for (int i=0; i<classNames.size(); i++) {
      m_Fieldeditmethods->insertItem(openValuePixmap,classNames[i].cStr());
    }

    m_Fieldeditmethods->blockSignals(false);
    currFieldEditToolIndex=0;
    m_Fieldeditmethods->setCurrentItem(currFieldEditToolIndex);

    m_Fieldeditmethods->triggerUpdate(true);
  }
 
  
  //Fill object edit combobox
  ComboBoxData(m_Frontcm,1);
  //Clear object edit listbox and set indices to zero 
  m_Fronteditmethods->clear();
  m_FrontcmIndex=0;
  m_FronteditIndex=-1;

  twd->setEnabled(true); // enable tab-widget
  twd->setTabEnabled(fieldtab, false);
  twd->setTabEnabled(objecttab, false);
  twd->setTabEnabled(combinetab, true);
  // switch to combine tab - will automatically set correct
  // currEditmode, currEdittool
  if (twd->currentPageIndex()!=2) twd->setCurrentPage(2);

  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miTime t;
  if (m_editm->getProductTime(t)){
    vector<miTime> Times;
    Times.push_back(t);
    emit emitTimes("product",Times);
    // update field dialog
    for (int i=0; i<currprod.fields.size(); i++){
      // add a new selected field in the field dialog
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  } else {
    cerr << "Controller returned no producttime" << endl;
  }

  m_editm->editCombine();

  this->show();
  //qt4 fix
  tabSelected(twd->tabText(twd->currentIndex()));

  pausebutton->setOn(false);
  ecomment->stopComment();
  ecomment->startComment();
  if (ep.OKstrings.size())
    emit Apply(ep.OKstrings,false);
  else
    emit editApply();
  //emit editUpdate();
}

void EditDialog::EditNewCancel()
{
}


bool EditDialog::close(bool alsoDelete)
{
  emit EditHide();
  return true;
} 

void EditDialog::hideComment()
{
  commentbutton->setOn(false);
  ecomment->hide();
}


void EditDialog::undoEdit()
{
  //called from shortcut ctrl-z in main window
  if (!inEdit) return;

  if (currMapmode == m_EditDI.mapmodeinfo[0].mapmode){
    //in field edit
    undofield();
  } else if (currMapmode == m_EditDI.mapmodeinfo[1].mapmode){
    //in objects edit
    undofront();
  } else if (currMapmode == m_EditDI.mapmodeinfo[2].mapmode){
    //in combine edit, not possible to undo
  }
}


void EditDialog::redoEdit()
{
  //called from shortcut ctrl-y in main window
  if (!inEdit) return;

  if (currMapmode == m_EditDI.mapmodeinfo[0].mapmode){
    //in field edit
    redofield();
  } else if (currMapmode == m_EditDI.mapmodeinfo[1].mapmode){
    //in objects edit
    redofront();
  } else if (currMapmode == m_EditDI.mapmodeinfo[2].mapmode){
    //in combine edit, not possible to do
  }
}


void EditDialog::saveEdit()
{
  //called from shortcut ctrl-s in main window
  if (inEdit) saveEverything(false);
}

bool EditDialog::inedit()
{
  return (inEdit && !m_editm->getEditPause());
}
