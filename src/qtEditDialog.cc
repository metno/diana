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

#include "diana_config.h"

#include <QMessageBox>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>
#include <QCheckBox>
#include <QSlider>
#include <QPixmap>
#include <QImage>
#include <QInputDialog>
#include <QAction>
#include <QHBoxLayout>
#include <QFrame>
#include <QVBoxLayout>
#include <QToolTip>

#include "diController.h"
#include "diDrawingTypes.h"
#include "diEditManager.h"
#include "diFieldPlotManager.h"
#include "diLocalSetupParser.h"
#include "diObjectManager.h"
#include "qtAnnoText.h"
#include "qtComplexText.h"
#include "qtEditComment.h"
#include "qtEditDialog.h"
#include "qtEditNewDialog.h"
#include "qtEditText.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "util/plotoptions_util.h"

#include <edit_open_value.xpm>
#include <edit_lock_value.xpm>

#define MILOGGER_CATEGORY "diana.EditDialog"
#include <miLogger/miLogging.h>
#include <puTools/miStringFunctions.h>


/*********************************************/
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
  METLIBS_LOG_SCOPE();

  TABNAME_FIELD= tr("Field");
  TABNAME_OBJECTS= tr("Objects");
  TABNAME_COMBINE= tr("Merge");

  EditDialogInfo ll=llctrl->initEditDialog();

  //list of translations to appear in dialog:
  editTranslations["Change value"]=tr("Change value");
  editTranslations["Move"]=tr("Move");
  editTranslations["Change gradient"]=tr("Change gradient");
  editTranslations["Line, without smooth"]=tr("Line, without smooth");
  editTranslations["Line, with smooth"]=tr("Line, with smooth");
  editTranslations["Line, limited, without smooth"]=tr("Line, limited, without smooth");
  editTranslations["Line, limited, with smooth"]=tr("Line, limited, with smooth");
  editTranslations["Smooth"]=tr("Smooth");
  editTranslations["Replace undefined values"]=tr("Replace undefined values");
  editTranslations["Line"]=tr("Line");
  editTranslations["Copy value"]=tr("Copy value");
  editTranslations["Set undefined"]=tr("Set udefined");
  editTranslations["Cold front"]=tr("Cold front"); //Kaldfront
  editTranslations["Warm front"]=tr("Warm front"); //Varmfront
  editTranslations["Occlusion"]=tr("Occlusion"); //Okklusjon
  editTranslations["Cold occlusion"]=tr("Cold occlusion"); //Kald okklusjon
  editTranslations["Warm occlusion"]=tr("Warm occlusion"); //Varm okklusjon
  editTranslations["Stationary front"]=tr("Stationary front"); //Stasjon�r front
  editTranslations["Trough"]=tr("Trough"); //Tr�g
  editTranslations["Squall line"]=tr("Squall line"); //Bygelinje
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.v�r
  editTranslations["Significant weather TURB/VA/RC"]=tr("Significant weather TURB/VA/RC"); //Sig.v�r
  editTranslations["Significant weather ICE/TCU/CB"]=tr("Significant weather ICE/TCU/CB"); //Sig.v�r

  editTranslations["Low pressure"]=tr("Low pressure"); //Lavtrykk
  editTranslations["High pressure"]=tr("High pressure"); //H�ytrykk
  editTranslations["Cold"]=tr("Cold"); //Kald
  editTranslations["Warm"]=tr("Warm"); //Varm
  editTranslations["Fog"]=tr("Fog"); //T�ke
  editTranslations["Drizzle"]=tr("Drizzle "); //yr
  editTranslations["Freezing drizzle"]=tr("Freezing drizzle"); //Yr som fryser
  editTranslations["Freezing rain"]=tr("Freezing rain"); //Regn som fryser
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Rain showers"]=tr("Rain showers"); //Regnbyger
  editTranslations["Sleet showers"]=tr("Sleet showers"); //Sluddbyger ??
  editTranslations["Hail showers"]=tr("Hail showers"); //Haglbyger
  editTranslations["Snow showers"]=tr("Snow showers"); //Haglbyger
  editTranslations["Thunderstorm"]=tr("Thunderstorm"); //Tordenv�r
  editTranslations["Thunderstorm with hail"]=tr("Thunderstorm with hail"); //Tordenv�r m/hagl
  editTranslations["Snow"]=tr("Snow"); //Sn�(stjerne)    ??
  editTranslations["Rain"]=tr("Rain"); //Regn
  editTranslations["Sleet"]=tr("Sleet"); //Sludd

  editTranslations["Hurricane"]=tr("Hurricane"); //Tropisk orkan ??

  editTranslations["Disk"]=tr("Disk"); //
  editTranslations["Circle"]=tr("Circle"); //Sirkel
  editTranslations["Cross"]=tr("Cross");   //Kryss
  editTranslations["Text"]=tr("Text");   //Tekster
  editTranslations["EditText"]=tr("EditText");   //Tekster


  editTranslations["Precipitation"]=tr("Precipitation"); //Nedb�r ??
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Clouds"]=tr("Clouds"); //Skyer
  editTranslations["Haze"]=tr("Haze"); //Dis
  editTranslations["Ice"]=tr("Ice"); //Is
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.v�r
  editTranslations["Reduced visibility"]=tr("Reduced visibility"); //Sig.v�r
  editTranslations["Generic area"]=tr("Generic area"); //
  
  // --------------------------------------------------------------------
  editAction = new QAction(this);
  editAction->setShortcut(Qt::CTRL+Qt::Key_E);
  editAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(editAction, SIGNAL( triggered() ), SLOT(EditMarkedText()));
  addAction( editAction );
  // --------------------------------------------------------------------
  deleteAction = new QAction(this);
  deleteAction->setShortcut(Qt::CTRL+Qt::Key_Delete);
  connect(deleteAction, SIGNAL( triggered() ) , SLOT(DeleteMarkedAnnotation()));
  addAction( deleteAction );

  m_editm= m_ctrl->getEditManager();
  m_objm=  m_ctrl->getObjectManager();

  setWindowTitle(tr("Editing"));

  inEdit= false;
  productApproved= false;
  combineAction= -1;
  fieldIndex= -1;
  fieldEditToolGroup= 0;
  numFieldEditTools=  0;
  currFieldEditToolIndex= 0;

  openValuePixmap= QPixmap(edit_open_value_xpm);
  lockValuePixmap= QPixmap(edit_lock_value_xpm);

  saveButton = new QPushButton(tr("Save (local)"), this);
  saveButton->setToolTip(tr("Save product to local disk"));
  connect( saveButton, SIGNAL(clicked()), SLOT(saveClicked())  );
  saveButton->setEnabled(false);
  sendButton = new QPushButton(tr("Save (common work disk)"), this);
  sendButton->setToolTip(tr("Save product to common work disk, available to other users as input in edit mode"));
  connect( sendButton, SIGNAL(clicked()), SLOT(sendClicked())  );
  sendButton->setEnabled(false);
  approveButton = new QPushButton(tr("Approve"), this);
  approveButton->setToolTip(tr("Approve product (makes the product available for other users and trigger production)"));
  connect( approveButton, SIGNAL(clicked()), SLOT(approveClicked( ))  );
  approveButton->setEnabled(false);
  QHBoxLayout* bgroupLayout = new QHBoxLayout();
  bgroupLayout->addWidget(saveButton);
  bgroupLayout->addWidget(sendButton);
  bgroupLayout->addWidget(approveButton);


  // ********** TAB
  twd = new QTabWidget( this );

  FieldTab();
  FrontTab();
  CombineTab();

  twd->setEnabled(false); // initially disabled

  connect( twd, SIGNAL(currentChanged( int )),
      SLOT( tabSelected( int ) ));
  // **********

  //toggle button for comments dialog
  pausebutton = new ToggleButton(this, tr("Pause"));
  connect(pausebutton, SIGNAL(toggled(bool)), SLOT(pauseClicked(bool)));
  pausebutton->setChecked(false);


  //toggle button for comments dialog
  commentbutton = new ToggleButton( this, tr("Comments"));
  connect(commentbutton, SIGNAL(toggled(bool)), SLOT(commentClicked(bool)));

  QHBoxLayout* h2layout = new QHBoxLayout();
  h2layout->addWidget(pausebutton);
  h2layout->addWidget(commentbutton);

  editexit = new QPushButton(tr("Exit"), this);
  connect(  editexit, SIGNAL(clicked()), SLOT( exitClicked() ));

  // qt4 fix: QButton -> QPushButton
  QPushButton* edithide = new QPushButton(tr("Hide"), this);
  connect( edithide, SIGNAL(clicked()), SIGNAL(EditHide()));

  edithelp = new QPushButton(tr("Help"), this);
  connect(  edithelp, SIGNAL(clicked()), SLOT( helpClicked() ));


  prodlabel = new QLabel("" , this);
  prodlabel->setMaximumHeight(40);
  lStatus = new QLabel("", this);

  QVBoxLayout* lvlayout= new QVBoxLayout();
  lvlayout->addWidget(prodlabel);
  lvlayout->addWidget(lStatus);

  QHBoxLayout* hlayout = new QHBoxLayout();

  hlayout->addWidget(editexit);
  hlayout->addWidget(edithide);
  hlayout->addWidget(edithelp);

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout( this);
  vlayout->addLayout( lvlayout,1);
  vlayout->addLayout( bgroupLayout );
  vlayout->addWidget( twd );
  vlayout->addLayout( h2layout );
  vlayout->addLayout( hlayout );

  //   vlayout->activate();
  //   vlayout->freeze();

  enew = new EditNewDialog(parent, m_ctrl);
  enew->hide();
  connect(enew,
      SIGNAL(EditNewOk(EditProduct&, EditProductId&, miutil::miTime&)),
      SLOT(EditNewOk(EditProduct&, EditProductId&, miutil::miTime&)));
  connect(enew,
      SIGNAL(EditNewCombineOk(EditProduct&, EditProductId&, miutil::miTime&)),
      SLOT(EditNewCombineOk(EditProduct&, EditProductId&, miutil::miTime&)));
  connect(enew, SIGNAL(EditNewHelp()), SLOT(helpClicked()));
  connect(enew, SIGNAL(EditNewCancel()), SLOT(EditNewCancel()));

  ecomment = new EditComment(this, m_objm, true);
  connect(ecomment,SIGNAL(CommentHide()),SLOT(hideComment()));
  ecomment->hide();
}

// --------------------------------------------------------------
// --------------- FieldTab methods -----------------------------
// --------------------------------------------------------------

void  EditDialog::FieldTab()
{
  int mymargin=5;
  int myspacing=5;

  fieldtab = new QWidget(twd);


  fgroup = new QButtonGroup( fieldtab );
  fbutton = new QPushButton*[maxfields];
  QHBoxLayout* hLayout = new QHBoxLayout();

  for (int i=0; i<maxfields; i++) {
    fbutton[i]= new QPushButton( "    ", fieldtab );
    fgroup->addButton(fbutton[i],i);
    fbutton[i]->setEnabled(false);
    hLayout->addWidget(fbutton[i]);
  }
  numfields= 0;
  connect( fgroup, SIGNAL(buttonClicked(int)), SLOT(fgroupClicked(int)) );

  m_Fieldeditmethods = new QListWidget(fieldtab);
  //m_Fieldeditmethods->setMinimumHeight(HEIGHTLISTBOX);

  connect( m_Fieldeditmethods, SIGNAL( itemClicked ( QListWidgetItem * ) ),
      SLOT( FieldEditMethods(QListWidgetItem * ) ) );

  QButtonGroup* bgroupinfluence= new QButtonGroup(fieldtab);

  rbInfluence[0]= new QRadioButton(QString(tr("Circle")),         fieldtab);
  rbInfluence[1]= new QRadioButton(QString(tr("Square")),         fieldtab);
  rbInfluence[2]= new QRadioButton(QString(tr("Ellipse(centre)")),fieldtab);
  rbInfluence[3]= new QRadioButton(QString(tr("Ellipse(focus)")), fieldtab);
  QVBoxLayout* vLayout = new QVBoxLayout();
  for (int i=0; i<4; i++) {
    bgroupinfluence->addButton(rbInfluence[i],i);
    vLayout->addWidget( rbInfluence[i] );
  }
  bgroupinfluence->setExclusive(true);

  rbInfluence[1]->setEnabled(false);

  connect (bgroupinfluence, SIGNAL(buttonClicked(int)),
      SLOT(changeInfluence(int)));

  // set default (dialog and use)
  rbInfluence[0]->setChecked(true);
  changeInfluence(0); // needed as the above does not change anything


  QLabel* ellipseform = new QLabel(tr("Ellipse shape"),this);

  ellipsenumbers.clear();
  int i;
  for (i=20; i<70; i+=10) ellipsenumbers.push_back(float(i)/100.);
  for (i=70; i<85; i+=5)  ellipsenumbers.push_back(float(i)/100.);
  for (i=85; i<100; i++)  ellipsenumbers.push_back(float(i)/100.);

  ellipsenumber = new QLabel( "    ", this );
  ellipsenumber->setMinimumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setMaximumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setFrameStyle( QFrame::Box | QFrame::Plain);
  ellipsenumber->setLineWidth(2);
  ellipsenumber->setAlignment( Qt::AlignCenter );

  int n= ellipsenumbers.size()-1;
  int index= (n+1)/2;

  ellipsenumber->setNum( double(ellipsenumbers[index]) );

  ellipseslider  = new QSlider( Qt::Horizontal, this);
  ellipseslider->setMinimum(0);
  ellipseslider->setMaximum(n);
  ellipseslider->setPageStep(1);
  ellipseslider->setValue(index);
  ellipseslider->setMinimumHeight( 16 );
  ellipseslider->setMaximumHeight( 16 );
  ellipseslider->setEnabled( true );

  connect( ellipseslider, SIGNAL( valueChanged( int )),
      SLOT( fieldEllipseChanged( int)));

  connect( ellipseslider, SIGNAL( sliderReleased() ),
      SLOT( fieldEllipseShape()) );

  QHBoxLayout* ehbox = new QHBoxLayout();
  ehbox->setMargin( mymargin );
  ehbox->setSpacing( myspacing/2 );
  ehbox->addWidget(ellipseform);
  ehbox->addWidget(ellipsenumber);
  ehbox->addWidget(ellipseslider);
  // set default
  fieldEllipseShape();

  // enable/disable extra editing lines
  exlineCheckBox= new QCheckBox(tr("Show extra editing lines"), fieldtab);
  exlineCheckBox->setChecked( false );
  exlineCheckBox->setEnabled( true );

  connect( exlineCheckBox, SIGNAL( toggled(bool) ),
      SLOT( exlineCheckBoxToggled(bool) ) );

  undoFieldButton = new QPushButton(tr("Undo"), this);
  redoFieldButton = new QPushButton(tr("Redo"), this);

  undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( false );

  connect( undoFieldButton, SIGNAL(clicked()), SLOT(undofield()));
  connect( redoFieldButton, SIGNAL(clicked()), SLOT(redofield()));

  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );
  hbox->addWidget(undoFieldButton);
  hbox->addWidget(redoFieldButton);

  QVBoxLayout* vlayout = new QVBoxLayout( fieldtab);
  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addLayout( hLayout );
  vlayout->addWidget( m_Fieldeditmethods );
  vlayout->addLayout( vLayout );
  vlayout->addLayout( ehbox );
  vlayout->addWidget( exlineCheckBox );
  vlayout->addLayout( hbox );

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
  EditEvent ee(0, 0);
  ee.type= edit_undo;
  ee.order= normal_event;
  if (!m_editm->notifyEditEvent(ee))
    undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::redofield()
{
  EditEvent ee(0, 0);
  ee.type= edit_redo;
  ee.order= normal_event;
  if(!m_editm->notifyEditEvent(ee))
    redoFieldButton->setEnabled( false );
  undoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::changeInfluence( int index )
{
  EditEvent ee(0, 0);
  if      (index==0) ee.type= edit_circle;
  else if (index==1) ee.type= edit_square;
  else if (index==2) ee.type= edit_ellipse1;
  else if (index==3) ee.type= edit_ellipse2;
  ee.order= normal_event;
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
  EditEvent ee(ellipsenumbers[index], 0);
  ee.type= edit_ecellipse;
  ee.order= normal_event;
  m_editm->notifyEditEvent(ee);
  if (inEdit) emit editUpdate();
}


void EditDialog::exlineCheckBoxToggled(bool on)
{
  EditEvent ee(0, 0);
  ee.type= (on) ? edit_exline_on : edit_exline_off;
  ee.order= normal_event;
  m_editm->notifyEditEvent(ee);
}

void EditDialog::FieldEditMethods(QListWidgetItem*)
{
  METLIBS_LOG_SCOPE();

  if(m_Fieldeditmethods->count()==0) return;

  int index = m_Fieldeditmethods->currentRow();
  if(index<0) {
    m_Fieldeditmethods->setCurrentRow(0);
    index=0;
  }

  int numClassValues= classValues.size();

  if (index<numFieldEditTools) {

    //    currFieldEditToolIndex= index;

    currMapmode= m_EditDI.mapmodeinfo[0].mapmode;
    currEditmode = m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].editmode;
    currEdittool=  m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].name;
    int tool= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].index;

    m_editm->setEditMode(currMapmode,currEditmode,currEdittool);

    EditEvent ee(0, 0);
    ee.type= editType(tool);
    ee.order= normal_event;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
      index<numFieldEditTools+numClassValues) {

    //    currFieldEditToolIndex= index;

    int n= index - numFieldEditTools;
    EditEvent ee(classValues[n], 0);
    ee.type= edit_class_value;
    ee.order= normal_event;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
      index<numFieldEditTools+numClassValues*2) {

    int n= index - numFieldEditTools - numClassValues;
    EditEvent ee(classValues[n], 0);
    ee.order = normal_event;

    diutil::BlockSignals blocked(m_Fieldeditmethods);
    if (classValuesLocked[n]) {
      m_Fieldeditmethods->item(index)->setIcon(QIcon(openValuePixmap));
      classValuesLocked[n]= false;
      ee.type= edit_open_value;
    } else {
      m_Fieldeditmethods->item(index)->setIcon(QIcon(lockValuePixmap));
      classValuesLocked[n]= true;
      ee.type= edit_lock_value;
    }
    blocked.unblock();

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

  objecttab = new QWidget(twd );

  std::vector<std::string> vstr;
  m_Frontcm = ComboBox( objecttab, vstr );
  connect( m_Frontcm, SIGNAL( activated(int) ),
      SLOT( FrontTabBox(int) ) );

  m_Fronteditmethods = new QListWidget(objecttab);

  connect( m_Fronteditmethods, SIGNAL(itemClicked(QListWidgetItem *) ),
      SLOT( FrontEditClicked() ) );
  connect( m_Fronteditmethods, SIGNAL(itemDoubleClicked(QListWidgetItem *) ),
      SLOT( FrontEditDoubleClicked() ) );

  undoFrontButton = new QPushButton(tr("Undo"), this);
  redoFrontButton = new QPushButton(tr("Redo"), this);

  connect( undoFrontButton, SIGNAL(clicked()), SLOT(undofront()));
  connect( redoFrontButton, SIGNAL(clicked()), SLOT(redofront()));

  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );
  hbox->addWidget(undoFrontButton);
  hbox->addWidget(redoFrontButton);

  autoJoin = new QCheckBox( tr("Join fronts"), objecttab);
  autoJoin->setChecked(true);
  connect(autoJoin, SIGNAL(toggled(bool)), SLOT(autoJoinToggled(bool)));

  // initialize colours and ok state
  autoJoinToggled(true);
  QVBoxLayout* vlayout = new QVBoxLayout( objecttab);

  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addWidget( m_Frontcm );
  vlayout->addWidget( m_Fronteditmethods );
  vlayout->addWidget( autoJoin );
  vlayout->addLayout( hbox );

  twd->addTab( objecttab, TABNAME_OBJECTS);
}


void  EditDialog::FrontTabBox( int index )
{
  // called when an item in objects combo box selected
  if (index!=m_FrontcmIndex || m_Fronteditmethods->count()==0){
    m_FrontcmIndex=index;
    ListWidgetData( m_Fronteditmethods, 1, index);
    m_FronteditIndex=-1;
    m_Fronteditmethods->item(0)->setSelected(true);
  } else if (m_FronteditIndex < m_Fronteditmethods->count()-1){
    m_Fronteditmethods->item(m_FronteditIndex)->setSelected(true);
  }
  currEditmode= std::string(m_Frontcm->itemText(m_FrontcmIndex).toStdString());
  FrontEditClicked();
  return;
}



void EditDialog::FrontEditClicked()
{
  METLIBS_LOG_SCOPE();
  //called when an item in the objects list box clicked
  if (!inEdit || m_Fronteditmethods->count()==0) return;

  int index = m_Fronteditmethods->currentRow();
  if(index<0) {
    m_Fronteditmethods->setCurrentRow(0);
    index=0;
  }

  if (int(m_FronteditList.size())>index) currEdittool= m_FronteditList[index];
  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  if (index!=m_FronteditIndex){
    m_FronteditIndex=index;
    if (m_objm->inTextMode()){
      std::string text = m_objm->getCurrentText();
      Colour::ColourInfo colour= m_objm->getCurrentColour();
      if (text.empty()){
        if (getText(text,colour)){
          m_objm->setCurrentText(text);
          m_objm->setCurrentColour(colour);
        }
      }
    } else if (m_objm->inComplexTextMode()) {
      std::vector <std::string> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      if (getComplexText(symbolText,xText)){
        m_objm->setCurrentComplexText(symbolText,xText);
      }
    } else if (m_objm->inEditTextMode()) {
      std::vector<std::string> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      if (getEditText(symbolText)){
        m_objm->setCurrentComplexText(symbolText,xText);
      }
    }
  }
  m_objm->createNewObject();
}



void EditDialog::FrontEditDoubleClicked()
{
  METLIBS_LOG_SCOPE();
  //called when am item in the objects list box doubleclicked
  if (m_objm->inTextMode()){
    std::string text = m_objm->getCurrentText();
    Colour::ColourInfo colour=m_objm->getCurrentColour();
    if (getText(text,colour)){
      //change objectmanagers current text !
      m_objm->setCurrentText(text);
      m_objm->setCurrentColour(colour);
    }
  } else if (m_objm->inComplexTextMode()){
    std::vector<std::string> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    if (getComplexText(symbolText,xText)){
      m_objm->setCurrentComplexText(symbolText,xText);
    }
  } else if (m_objm->inEditTextMode()){
    std::vector<std::string> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    if (getEditText(symbolText)){
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
  //called from shortcut ctrl-e
  //changes all marked texts and objectmanagers current text !
  std::vector <std::string> symbolText,xText,eText, mText;
  std::string text = m_objm->getMarkedText();
  METLIBS_LOG_DEBUG("-----EditDialog::EditMarkedText called------- text = " << text);
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
    AnnoText * aText =new AnnoText(this,m_ctrl,m_editm->getProductName(), eText, xText);
    connect(aText,SIGNAL(editUpdate()),SIGNAL(editUpdate()));
    m_ctrl->startEditAnnotation();
    aText->exec();
    delete aText;
  }
  m_objm->getMarkedComplexText(symbolText,xText);
  if (symbolText.size()||xText.size()){
    METLIBS_LOG_DEBUG("-----EditDialog::getMarkedComplexText returns nonempty strings");
    if (getComplexText(symbolText,xText))
      m_objm->changeMarkedComplexText(symbolText,xText);
  }
  m_objm->getMarkedMultilineText(mText);
  if (mText.size()){
    METLIBS_LOG_DEBUG("-----EditDialog::getMarkedMultilineText returns nonempty strings");
    if (getEditText(mText))
      m_objm->changeMarkedMultilineText(mText);
  }

}

void EditDialog::DeleteMarkedAnnotation()
{
  m_ctrl->DeleteMarkedAnnotation();
}


bool EditDialog::getText(std::string & text, Colour::ColourInfo & colour)
{
  bool ok = false;

  std::vector <std::string> symbolText,xText;
  symbolText.push_back(text);
  std::set <std::string> textList=m_objm->getTextList();
  ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText, textList,true);
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

bool EditDialog::getComplexText(std::vector<std::string>& symbolText, std::vector<std::string>& xText)
{
  bool ok=false;
  if (symbolText.size()||xText.size()){
    std::set<std::string> complexList = m_ctrl->getComplexList();
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

bool EditDialog::getEditText(std::vector<std::string>& editText)
{
  bool ok=false;
  if (editText.size()) {
    std::set<std::string> complexList = m_ctrl->getComplexList();
    //set <std::string> textList=m_objm->getTextList();
    EditText * eText =new EditText(this,m_ctrl, editText, complexList, true);
    if (eText->exec()){
      eText->getEditText(editText);
      ok=true;
    }
    delete eText;
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

  combinetab = new QWidget(twd );

  QButtonGroup* bg= new QButtonGroup(combinetab);
  QRadioButton* rb1= new QRadioButton(tr("Change borders"),this);
  QRadioButton* rb2= new QRadioButton(tr("Set data sources"),this);
  bg->addButton(rb1,0);
  bg->addButton(rb2,1);
  QVBoxLayout* bgLayout = new QVBoxLayout();
  bgLayout->addWidget(rb1);
  bgLayout->addWidget(rb2);
  rb1->setChecked(true);
  connect(bg, SIGNAL(buttonClicked(int)), SLOT(combine_action(int)));

  m_SelectAreas = new QListWidget(combinetab);//listBox( group, 150, 75, false );
  m_SelectAreas->setMinimumHeight(100);

  connect( m_SelectAreas, SIGNAL( itemClicked ( QListWidgetItem *  ) ),
      SLOT( selectAreas(QListWidgetItem * ) ));

  stopCombineButton = new QPushButton( tr("Exit merge"), combinetab);
  connect(stopCombineButton, SIGNAL(clicked()), SLOT(stopCombine()));

  QVBoxLayout* vlayout = new QVBoxLayout( combinetab);
  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addLayout( bgLayout );
  vlayout->addWidget( m_SelectAreas );
  vlayout->addWidget( stopCombineButton );

  twd->addTab( combinetab, TABNAME_COMBINE );
}

void EditDialog::stopCombine()
{
  //   METLIBS_LOG_DEBUG("EditDialog::stopCombine called");

  twd->setTabEnabled(0, true);
  twd->setTabEnabled(1, true);
  twd->setCurrentIndex(0);
  twd->setTabEnabled(2, false);
  //not possible to save or send until combine stopped
  saveButton->setEnabled(true);
  sendButton->setEnabled(currid.sendable);
  approveButton->setEnabled(currid.sendable);
  m_editm->stopCombine();
}

void EditDialog::combine_action(int idx)
{
  if (idx == combineAction) return;
  combineAction= idx;
  CombineEditMethods();
}

void EditDialog::selectAreas(QListWidgetItem*)
{
  int index = m_SelectAreas->currentRow();
  std::string tmp= std::string( m_SelectAreas->item(index)->text().toStdString());
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
    if(m_SelectAreas->count() > 0) {
      m_SelectAreas->setEnabled(true);
      currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[1].editmode;
      if(m_SelectAreas->currentRow()<0) {
        m_SelectAreas->setCurrentRow(0);
      }
      currEdittool= std::string( m_SelectAreas->currentItem()->text().toStdString());
      if (inEdit) m_objm->createNewObject();
    }
  } else {
    METLIBS_LOG_ERROR("EditDialog::CombineEditMethods    unknown combineAction:"
        << combineAction);
    return;
  }
  if (inEdit) {
    m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  }

}

// --------------------------------------------------------------
// ------------- Common methods ---------------------------------
// --------------------------------------------------------------

void EditDialog::tabSelected( int tabindex)
{
  METLIBS_LOG_DEBUG("EditDialog::tabSelected:"<<tabindex);

  QString tabname = twd->tabText(tabindex);

  if (tabname == TABNAME_FIELD) {
    //unmark all objects when changing mapMode
    m_objm->editNotMarked();
    if (!inEdit) METLIBS_LOG_DEBUG("EditDialog::tabSelected emit editUpdate()...(1)");
    if (!inEdit) emit editUpdate();
    if (m_EditDI.mapmodeinfo.size()>0){
      currMapmode= m_EditDI.mapmodeinfo[0].mapmode;
      FieldEditMethods(m_Fieldeditmethods->currentItem());
    }
  } else if (tabname == TABNAME_OBJECTS) {
    if (m_EditDI.mapmodeinfo.size()>1){
      currMapmode= m_EditDI.mapmodeinfo[1].mapmode;
      m_Frontcm->setCurrentIndex(m_FrontcmIndex);
      FrontTabBox(m_FrontcmIndex);
    }
  } else if (tabname == TABNAME_COMBINE) {
    if (m_EditDI.mapmodeinfo.size()>2){
      currMapmode= m_EditDI.mapmodeinfo[2].mapmode;
      CombineEditMethods();
    }
  }
  // do a complete redraw - with underlay saving
  if (inEdit) METLIBS_LOG_DEBUG("EditDialog::tabSelected emit editUpdate()...(2)");

  if (inEdit) emit editUpdate();
}


void  EditDialog::ListWidgetData( QListWidget* list, int mindex, int index)
{

  list->clear();
  std::vector<std::string> vstr;
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools.size();
  list->setViewMode(QListView::ListMode);
  for ( int i=0; i<n; i++){
    std::string etool=m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools[i].name;
    if (inEdit) METLIBS_LOG_DEBUG("ListWidgetData etool = "<< etool);
    vstr.push_back(etool);
    QString dialog_etool;
    //find translation
    if (editTranslations.count(etool))
      dialog_etool =editTranslations[etool];
    else
      dialog_etool = etool.c_str();
    list->addItem(QString(dialog_etool));
  }

  list->setCurrentItem(0);

  if (mindex==OBJECT_INDEX)
    m_FronteditList=vstr; //list of edit tools

  if (mindex==OBJECT_INDEX && index==SIGMAP_INDEX){
    //for now, only sigmap symbols have images...
    list->clear();
    list->setViewMode(QListView::IconMode);
    for ( int i=0; i<n; i++){
      std::string path = LocalSetupParser::basicValue("imagepath");
      std::string filename = path+ m_FronteditList[i] + ".png";
      QPixmap pmap(filename.c_str());
      if(!pmap.isNull()){
        QListWidgetItem* item = new QListWidgetItem(QIcon(pmap),QString());
        list->addItem(item);
      } else {
        list->addItem(QString(m_FronteditList[i].c_str()));
      }
    }
  }

  return;
}

void EditDialog::ComboBoxData(QComboBox*, int mindex)
{
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo.size();
  std::vector<std::string> vstr;
  m_Frontcm->clear();
  for( int i=0; i<n; i++ ){
    if (m_EditDI.mapmodeinfo[mindex].editmodeinfo[i].edittools.size()){
      m_Frontcm->addItem(QString(m_EditDI.mapmodeinfo[1].editmodeinfo[i].editmode.c_str()));
    }
  }

}

void EditDialog::saveEverything(bool send, bool approved)
{
  ecomment->saveComment();
  QString message;
  m_editm->writeEditProduct(message, true, true, send, approved);

  if (!message.isEmpty()) {
    QMessageBox::warning(this, tr("Save error:"), message);
  }

  miutil::miTime t= miutil::miTime::nowTime();
  QString lcs(send ? " <font color=\"darkgreen\">"+tr("Saved") +"</font> "
      : " <font color=\"black\">"+tr("saved")+"</font> ");
  QString tcs= QString("<font color=\"black\">")+
      QString(t.isoTime().c_str()) + QString("</font> ");
  QString qs= lcs + tcs;

  if (send && approved){
    productApproved= true;
    qs += " <font color=\"darkgreen\">"+ tr("and approved") +"</font> ";
  } else if (productApproved){
    qs += " <font color=\"red\">"+ tr("(approved)") + "</font> ";
  }

  lStatus->setText(qs);
}

void  EditDialog::saveClicked()
{

  saveEverything(false,false);
  if (m_editm->showAllObjects()) emit editUpdate();
}

void  EditDialog::sendClicked()
{

  saveEverything(true,false);
  if (m_editm->showAllObjects()) emit editUpdate();
}

void  EditDialog::approveClicked()
{
  if (!undoFrontButton->isEnabled() && !undoFieldButton->isEnabled() ) {
    int ret = QMessageBox::warning(this, tr("Warning - approve product"),
        tr("Do you really want to approve an unchanged product?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if ( ret==QMessageBox::Cancel)
      return;
  }

  saveEverything(true,true);
  if (m_editm->showAllObjects()) emit editUpdate();
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
    commentbutton->setChecked(false);
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
    if(commentbutton ->isChecked() )
      ecomment->show();
  } else{
    //load and start EditNewDialog
    enew->load();
    enew->show();
  }
}


void EditDialog::hideAll()
{
  this->hide();
  enew->hide();
  if( commentbutton->isChecked() )
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
      saveEverything(false,false);
      break;
    case 1: // don't save, but exit
      break;
    case 2:
      return false; // cancel operation
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
  //m_editm->logoutDatabase(dbi);

  return true;
}


void EditDialog::exitClicked()
{
  METLIBS_LOG_DEBUG("EditDialog::exitClicked....................");
  if (!cleanupForExit()) return;
  commentbutton->setChecked(false);
  // update field dialog
  emit emitFieldEditUpdate("");
  m_editm->setEditMode("normal_mode","","");
  twd->setEnabled(false); // disable tab-widget
  saveButton->setEnabled(false);
  sendButton->setEnabled(false);
  approveButton->setEnabled(false);
  emit EditHide();
  emit editApply();
  // empty timeslider producttime
  emitTimes(plottimes_t());
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
  emit showsource("ug_editdialogue.html");
}


void EditDialog::updateLabels()
{
  // update top-labels etc.
  std::string s;
  if (inEdit) {
    s= std::string("<font color=\"darkgreen\">") +
        currprod.name + std::string("</font>") +
        std::string("<font color=\"blue\"> ") +
        currid.name + std::string("</font>");
    if ( !prodtime.undef() ) {
      s += " " + prodtime.format("%D %H:%M", "", true);
    }
  } else {
    s= "";
  }

  prodlabel->setText(s.c_str());
}


void EditDialog::EditNewOk(EditProduct& ep,
    EditProductId& ci,
    miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();
  emit editMode(true);

  // Turn off Undo-buttons
  undoFrontsDisable();
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

  QString message;
  if (!m_editm->startEdit(ep,ci,time, message)) {
    QMessageBox::warning( this, tr("Error starting edit"),message);
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
    ListWidgetData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    diutil::BlockSignals block(m_Fieldeditmethods);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      std::string ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->addItem(QString(ts.c_str()));
    }

    numFieldEditTools= n;

    getFieldClassSpecs();

    classNames.push_back(tr("Undefined").toStdString());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (unsigned int i=0; i<classNames.size(); i++) {
      std::string estr= tr("New value:").toStdString() +  classNames[i];
      m_Fieldeditmethods->addItem(QString(estr.c_str()));
    }

    for (unsigned int i=0; i<classNames.size(); i++) {
      QListWidgetItem* item
      = new QListWidgetItem(QIcon(openValuePixmap),QString(classNames[i].c_str()));
      m_Fieldeditmethods->addItem(item);
    }
  }

  //########################################################################
  EditEvent ee(0, 0);
  if (fieldEditToolGroup==2)
    ee.type= edit_show_numbers_on;
  else
    ee.type= edit_show_numbers_off;
  ee.order= normal_event;
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
    twd->setTabEnabled(0, true);
  } else{
    twd->setTabEnabled(0, false);
    FrontTabBox(0);
  }
  twd->setTabEnabled(1, not currprod.objectsFilenamePart.empty() );

  twd->setTabEnabled(2, false);
  if (twd->currentIndex()!=mm) twd->setCurrentIndex(mm);

  saveButton->setEnabled(true);
  sendButton->setEnabled(currid.sendable);
  approveButton->setEnabled(currid.sendable);
  commentbutton->setEnabled(not currprod.commentFilenamePart.empty() );

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miutil::miTime t;
  m_editm->getProductTime(t);
  plottimes_t Times;
  if (!t.undef()) {
    Times.insert(t);
  }
  emitTimes(Times);

  // update field dialog
  for (unsigned int i=0; i<currprod.fields.size(); i++){
    if (currprod.fields[i].fromfield){
      // this will remove the original field in the field dialog
      METLIBS_LOG_DEBUG("EditDialog::EditNewOk emit emitFieldEditUpdate");
      emit emitFieldEditUpdate(currprod.fields[i].fromfname);
    } else {
      // add a new selected field in the field dialog
      METLIBS_LOG_DEBUG("EditDialog::EditNewOk emit emitFieldEditUpdate...new");
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  }

  this->show();
  //qt4 fix
  tabSelected(twd->currentIndex());

  ecomment->stopComment();
  ecomment->startComment();
  pausebutton->setChecked(false);

  if (ep.OKstrings.size()){
    METLIBS_LOG_DEBUG("EditDialog::EditNewOk emit Apply(ep.OKstrings)");
    //apply commands for this EditProduct (probably MAP)
    m_ctrl->keepCurrentArea(false); // unset area conservatism
    /*emit*/ Apply(ep.OKstrings, false);
    m_ctrl->keepCurrentArea(true); // reset area conservatism
  } else {
    //  m_ctrl->keepCurrentArea(true); // reset area conservatism
    METLIBS_LOG_DEBUG("EditDialog::EditNewOk emit editApply()");
    /*emit*/ editApply();
    //  m_ctrl->keepCurrentArea(false); // reset area conservatism
  }

  METLIBS_LOG_DEBUG("REMOVED EditDialog::EditNewOk emit editUpdate()");
  //emit editUpdate();

  saveEverything(true,false);
}


void EditDialog::EditNewCombineOk(EditProduct& ep,
    EditProductId& ci,
    miutil::miTime& time)
{
  //   METLIBS_LOG_DEBUG("EditNewCombineOK");
  // Turn off Undo-buttons
  undoFrontsDisable();
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

  std::vector<std::string> combids;
  // try to start combine
  QString message;
  if (!m_editm->startCombineEdit(ep,ci,time,combids, message)){
    METLIBS_LOG_ERROR("Error starting combine");
    emit editApply();
    return;
  }

  // put combids into select-areas listbox
  m_SelectAreas->clear();
  for (unsigned int i=0; i<combids.size(); i++){
    m_SelectAreas->addItem(QString(combids[i].c_str()));
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
    ListWidgetData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    diutil::BlockSignals blocked(m_Fieldeditmethods);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      std::string ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->addItem(QString(ts.c_str()));
    }

    numFieldEditTools= n;

    getFieldClassSpecs();

    classNames.push_back(tr("Undefined").toStdString());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (unsigned int i=0; i<classNames.size(); i++) {
      std::string estr= tr("New value:").toStdString() +  classNames[i];
      m_Fieldeditmethods->addItem(QString(estr.c_str()));
    }

    for (unsigned int i=0; i<classNames.size(); i++) {
      QListWidgetItem* item
      = new QListWidgetItem(QIcon(openValuePixmap),QString(classNames[i].c_str()));
      m_Fieldeditmethods->addItem(item);
    }
  }

  //Fill object edit combobox
  ComboBoxData(m_Frontcm,1);
  //Clear object edit listbox and set indices to zero
  m_Fronteditmethods->clear();
  m_FrontcmIndex=0;
  m_FronteditIndex=-1;

  twd->setEnabled(true); // enable tab-widget
  twd->setTabEnabled(0, false);
  twd->setTabEnabled(1, false);
  twd->setTabEnabled(2, true);
  // switch to combine tab - will automatically set correct
  // currEditmode, currEdittool
  if (twd->currentIndex()!=2) twd->setCurrentIndex(2);

  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miutil::miTime t;
  if (m_editm->getProductTime(t)){
    plottimes_t Times;
    Times.insert(t);
    emitTimes(Times);
    // update field dialog
    for (unsigned int i=0; i<currprod.fields.size(); i++){
      // add a new selected field in the field dialog
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  } else {
    METLIBS_LOG_WARN("Controller returned no producttime");
  }

  m_editm->editCombine();

  this->show();
  //qt4 fix
  tabSelected(twd->currentIndex());

  pausebutton->setChecked(false);
  ecomment->stopComment();
  ecomment->startComment();
  if (ep.OKstrings.size())
    /*emit*/ Apply(ep.OKstrings, false);
  else
    /*emit*/ editApply();
  //emit editUpdate();
}

void EditDialog::EditNewCancel()
{
}

void EditDialog::closeEvent(QCloseEvent*)
{
  emit EditHide();
}

void EditDialog::hideComment()
{
  commentbutton->setChecked(false);
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
  if (inEdit) saveEverything(false,false);
}

bool EditDialog::inedit()
{
  return (inEdit && !m_editm->getEditPause());
}

void EditDialog::getFieldClassSpecs()
{
  PlotOptions po;
  unsigned int maxlen = 0;
  m_editm->getFieldPlotOptions(currprod.fields[0].name, po);
  diutil::parseClasses(po, classValues, classNames, maxlen);
  classValuesLocked.insert(classValuesLocked.end(), classValues.size(), false);
}

void EditDialog::emitTimes(const plottimes_t& times)
{
  Q_EMIT sendTimes("product", times, true);
}
