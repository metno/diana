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

#include <qapplication.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <qgroupbox.h>
#include <qvbox.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qframe.h>

#include <qtFieldDialog.h>
#include <qtUtility.h>
#include <qtToggleButton.h>

#include <diController.h>
#include <diRectangle.h>
#include <diPlotOptions.h>

#include <iostream>
#include <math.h>

#include <up20x20.xpm>
#include <down20x20.xpm>
#include <up12x12.xpm>
#include <down12x12.xpm>
#include <minus12x12.xpm>


FieldDialog::FieldDialog( QWidget* parent, Controller* lctrl )
: QDialog(parent)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::FieldDialog called"<<endl;
#endif

  m_ctrl=lctrl;

  vector<FieldDialogInfo> fdi= m_ctrl->initFieldDialog();

  setCaption(tr("Fields"));

  useArchive= false;

  numEditFields= 0;
  currentFieldOptsInEdit= false;
  historyPos= -1;

  editName= tr("EDIT").latin1();

  // translations of fieldGroup names (Qt linguist translations)
  fgTranslations["EPS Probability"]= tr("EPS Probability").latin1();
  fgTranslations["EPS Clusters"]=    tr("EPS Clusters").latin1();
  fgTranslations["EPS Members"]=     tr("EPS Members").latin1();

  fgTranslations["Analysis"]=        tr("Analysis").latin1();
  fgTranslations["Constant fields"]= tr("Constant fields").latin1();

  fgTranslations["Surface etc"]=        tr("Surface etc.").latin1();
  fgTranslations["Pressure Levels"]=    tr("Pressure Levels").latin1();
  fgTranslations["FlightLevels"]=       tr("FlightLevels").latin1();
  fgTranslations["Model Levels"]=       tr("Model Levels").latin1();
  fgTranslations["Isentropic Levels"]=  tr("Isentropic Levels").latin1();
  fgTranslations["Temperature Levels"]= tr("Temperature Levels").latin1();
  fgTranslations["PV Levels"]=          tr("PV Levels").latin1();
  fgTranslations["Ocean Depths"]=       tr("Ocean Depths").latin1();
  fgTranslations["Ocean Model Levels"]= tr("Ocean Model Levels").latin1();
  //fgTranslations[""]= tr("");

  int i, n;

  // get all field plot options from setup file
  vector<miString> fieldNames;
  m_ctrl->getAllFieldNames(fieldNames,fieldPrefixes,fieldSuffixes);
  PlotOptions::getAllFieldOptions(fieldNames,setupFieldOptions);

//#################################################################
//  map<miString,miString>::iterator pfopt, pfend= setupFieldOptions.end();
//  for (pfopt=setupFieldOptions.begin(); pfopt!=pfend; pfopt++)
//    cerr << pfopt->first << "   " << pfopt->second << endl;
//#################################################################

  m_modelgroup = fdi;

  // Colours
  colourInfo  = Colour::getColourInfo();
  csInfo      = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();

  nr_colors= colourInfo.size();
  QColor pixcolor;
  pmapColor = new QPixmap*[nr_colors];
  for (i=0; i<nr_colors; i++) {
    pixcolor= QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
		      colourInfo[i].rgb[2] );
    pmapColor[i] = new QPixmap( 20, 20 );
    pmapColor[i]->fill( pixcolor );
  }

  // linewidths
   nr_linewidths= 12;

  // linetypes
  linetypes = Linetype::getLinetypeNames();
  nr_linetypes= linetypes.size();

  // density (of arrows etc, 0=automatic)
  vector<int> density_value;
  for (i=0;  i<10; i++)   density_value.push_back(i);
  for (i=10; i<60; i+=10) density_value.push_back(i);
  density_value.push_back(100);
  n=density_value.size();
      density.push_back("Auto");
  for (i=1; i<n; ++i) {
      ostringstream ostr;
      ostr << density_value[i];
      density.push_back(ostr.str());
    }

  //----------------------------------------------------------------
  cp = new CommandParser();

  // add level options to the cp's keyDataBase
  cp->addKey("level", "",1,CommandParser::cmdString);
  cp->addKey("idnum","",1,CommandParser::cmdString);

  cp->addKey("hour.offset","",1,CommandParser::cmdInt);
  cp->addKey("hour.diff","",1,CommandParser::cmdInt);

  cp->addKey("MINUS","",1,CommandParser::cmdNoValue);

  // add more plot options to the cp's keyDataBase
  cp->addKey("colour",         "",0,CommandParser::cmdString);
  cp->addKey("colours",        "",0,CommandParser::cmdString);
  cp->addKey("linewidth",      "",0,CommandParser::cmdInt);
  cp->addKey("linetype",       "",0,CommandParser::cmdString);
  cp->addKey("line.interval",  "",0,CommandParser::cmdFloat);
  cp->addKey("density",        "",0,CommandParser::cmdInt);
  cp->addKey("vector.unit",    "",0,CommandParser::cmdFloat);
  cp->addKey("rel.size",       "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.type",   "",0,CommandParser::cmdString);
  cp->addKey("extreme.size",   "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.radius", "",0,CommandParser::cmdFloat);
  cp->addKey("line.smooth",    "",0,CommandParser::cmdInt);
  cp->addKey("field.smooth",   "",0,CommandParser::cmdInt);
  cp->addKey("zero.line",      "",0,CommandParser::cmdInt);
  cp->addKey("value.label",    "",0,CommandParser::cmdInt);
  cp->addKey("label.size",     "",0,CommandParser::cmdFloat);
  cp->addKey("grid.value",     "",0,CommandParser::cmdInt);
  cp->addKey("grid.lines",     "",0,CommandParser::cmdInt);
  cp->addKey("grid.lines.max", "",0,CommandParser::cmdInt);
  cp->addKey("base",           "",0,CommandParser::cmdFloat);
  cp->addKey("base_2",         "",0,CommandParser::cmdFloat);
  cp->addKey("undef.masking",  "",0,CommandParser::cmdInt);
  cp->addKey("undef.colour",   "",0,CommandParser::cmdString);
  cp->addKey("undef.linewidth","",0,CommandParser::cmdInt);
  cp->addKey("undef.linetype", "",0,CommandParser::cmdString);
  cp->addKey("discontinuous",  "",0,CommandParser::cmdInt);
  cp->addKey("palettecolours", "",0,CommandParser::cmdString);
  cp->addKey("options.1",      "",0,CommandParser::cmdInt);
  cp->addKey("options.2",      "",0,CommandParser::cmdInt);
  cp->addKey("minvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("maxvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("minvalue_2",     "",0,CommandParser::cmdFloat);
  cp->addKey("maxvalue_2",     "",0,CommandParser::cmdFloat);
  cp->addKey("colour_2",       "",0,CommandParser::cmdString);
  cp->addKey("linewidth_2",    "",0,CommandParser::cmdInt);
  cp->addKey("linetype_2",     "",0,CommandParser::cmdString);
  cp->addKey("line.interval_2","",0,CommandParser::cmdFloat);
  cp->addKey("table",          "",0,CommandParser::cmdInt);
  cp->addKey("patterns",       "",0,CommandParser::cmdString);
  cp->addKey("patterncolour",  "",0,CommandParser::cmdString);
  cp->addKey("repeat",      "",0,CommandParser::cmdInt);
  cp->addKey("alpha",          "",0,CommandParser::cmdInt);

  // yet only from "external" (QuickMenu) commands
  cp->addKey("forecast.hour",     "",2,CommandParser::cmdInt);
  cp->addKey("forecast.hour.loop","",2,CommandParser::cmdInt);

  cp->addKey("allTimeSteps","",3,CommandParser::cmdString);
  //----------------------------------------------------------------

  // modelGRlabel
  QLabel *modelGRlabel= TitleLabel( tr("Model group"), this );

  // modelGRbox
  modelGRbox= new QComboBox( false, this );

  connect( modelGRbox, SIGNAL( activated( int ) ),
	   SLOT( modelGRboxActivated( int ) ) );

  // modellabel
  QLabel *modellabel= TitleLabel( tr("Models"), this );
  //h1 modelbox
  modelbox = new QListBox( this );
// #ifdef DISPLAY1024X768
//   modelbox->setMinimumHeight( 60 );
//   modelbox->setMaximumHeight( 60 );
// #else
//   modelbox->setMinimumHeight( 80 );
//   modelbox->setMaximumHeight( 80 );
// #endif

  connect( modelbox, SIGNAL( highlighted( int ) ),
	   SLOT( modelboxHighlighted( int ) ) );

  // fieldGRlabel
  QLabel *fieldGRlabel= TitleLabel( tr("Field group"), this );

  // fieldGRbox
  fieldGRbox= new QComboBox( false, this );

  connect( fieldGRbox, SIGNAL( activated( int ) ),
	   SLOT( fieldGRboxActivated( int ) ) );

  // fieldlabel
  QLabel *fieldlabel= TitleLabel( tr("Fields"), this );

  // fieldbox
  fieldbox = new QListBox( this );
// #ifdef DISPLAY1024X768
//   fieldbox->setMinimumHeight( 64 );
//   fieldbox->setMaximumHeight( 64 );
// #else
//   fieldbox->setMinimumHeight( 132 );
//   fieldbox->setMaximumHeight( 132 );
// #endif
  fieldbox->setSelectionMode( QListBox::Multi );

  connect( fieldbox, SIGNAL( selectionChanged() ),
  	   SLOT( fieldboxChanged() ) );

  // selectedFieldlabel
  QLabel *selectedFieldlabel= TitleLabel( tr("Selected fields"), this );

  // selectedFieldbox
  selectedFieldbox = new QListBox( this );
// #ifdef DISPLAY1024X768
//   selectedFieldbox->setMinimumHeight( 55 );
//   selectedFieldbox->setMaximumHeight( 55 );
// #else
//   selectedFieldbox->setMinimumHeight( 80 );
//   selectedFieldbox->setMaximumHeight( 80 );
// #endif
  selectedFieldbox->setSelectionMode( QListBox::Single );
  selectedFieldbox->setEnabled( true );

  connect( selectedFieldbox, SIGNAL( highlighted( int ) ),
	   SLOT( selectedFieldboxHighlighted( int ) ) );

  // Level: slider & label for the value
  levelLabel = new QLabel( "1000hPa", this );
  levelLabel->setMinimumSize(levelLabel->sizeHint().width() +10,
		             levelLabel->sizeHint().height()+10);
  levelLabel->setMaximumSize(levelLabel->sizeHint().width() +10,
		             levelLabel->sizeHint().height()+10);
  levelLabel->setText(" ");

  levelLabel->setFrameStyle( QFrame::Box | QFrame::Plain);
  levelLabel->setLineWidth(2);
  levelLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

  levelSlider = new QSlider( 0, 1, 1, 0, QSlider::Vertical, this);

  levelSlider->setEnabled( false );

  connect( levelSlider, SIGNAL( valueChanged( int )),
	                SLOT( levelChanged( int)));
  connect( levelSlider, SIGNAL( sliderPressed() ),
	                SLOT( levelPressed()) );
  connect( levelSlider, SIGNAL( sliderReleased() ),
	                SLOT( updateLevel()) );

  levelInMotion= false;

  // sliderlabel
  QLabel *levelsliderlabel= new QLabel( tr("Level"), this );

  // Idnum: slider & label for the value
  idnumLabel = new QLabel( "EPS.Total", this );
  idnumLabel->setMinimumSize(idnumLabel->sizeHint().width() +10,
		             idnumLabel->sizeHint().height()+10);
  idnumLabel->setMaximumSize(idnumLabel->sizeHint().width() +10,
		             idnumLabel->sizeHint().height()+10);
  idnumLabel->setText(" ");

  idnumLabel->setFrameStyle( QFrame::Box | QFrame::Plain);
  idnumLabel->setLineWidth(2);
  idnumLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

  idnumSlider = new QSlider( 0, 1, 1, 0, QSlider::Horizontal, this);

  idnumSlider->setEnabled( false );

  connect( idnumSlider, SIGNAL( valueChanged( int )),
	                SLOT( idnumChanged( int)));
  connect( idnumSlider, SIGNAL( sliderPressed() ),
	                SLOT( idnumPressed()) );
  connect( idnumSlider, SIGNAL( sliderReleased() ),
	                SLOT( updateIdnum()) );

  idnumInMotion= false;

  // sliderlabel
  QLabel *idnumsliderlabel= new QLabel( tr("Type"), this );

  // deleteSelected
  Delete = NormalPushButton( tr("Delete"), this );
  Delete->setEnabled( false );

  connect( Delete, SIGNAL(clicked()), SLOT(deleteSelected()));

  // get the standard button height from first PushButton
  int hbutton= Delete->sizeHint().height();

  // copyField
  copyField = NormalPushButton( tr("Copy"), this );
  copyField->setEnabled( false );

  connect( copyField, SIGNAL(clicked()), SLOT(copySelectedField()));

  // deleteAllSelected
  deleteAll = NormalPushButton( tr("Delete all"), this );
  deleteAll->setEnabled( false );

  connect( deleteAll, SIGNAL(clicked()), SLOT(deleteAllSelected()));

  // changeModelButton
  changeModelButton = NormalPushButton( tr("Model"), this );
  changeModelButton->setEnabled( false );

  connect(changeModelButton, SIGNAL(clicked()), SLOT(changeModel()));

  // historyBack
  historyBackButton= new QPushButton(QPixmap(up20x20_xpm),"",this);
  historyBackButton->setMinimumSize(hbutton+4,hbutton);
  historyBackButton->setMaximumSize(hbutton+4,hbutton);
  historyBackButton->setEnabled( false );

  connect( historyBackButton, SIGNAL(clicked()), SLOT(historyBack()));

  // historyForward
  historyForwardButton= new QPushButton(QPixmap(down20x20_xpm),"",this);
  historyForwardButton->setMinimumSize(hbutton+4,hbutton);
  historyForwardButton->setMaximumSize(hbutton+4,hbutton);
  historyForwardButton->setEnabled( false );

  connect( historyForwardButton,SIGNAL(clicked()),SLOT(historyForward()));

  // historyOk
  historyOkButton = NormalPushButton( "OK", this );
  historyOkButton->setMinimumWidth(hbutton*2);
//historyOkButton->setMaximumWidth(hbutton*2+8);
  highlightButton(historyOkButton,false);

  connect( historyOkButton, SIGNAL(clicked()), SLOT(historyOk()));

  int wbutton= hbutton-4;

  // upField
  upFieldButton= new QPushButton(QPixmap(up12x12_xpm),"",this);
  upFieldButton->setMinimumSize(wbutton,hbutton);
  upFieldButton->setMaximumSize(wbutton,hbutton);
  upFieldButton->setEnabled( false );

  connect( upFieldButton, SIGNAL(clicked()), SLOT(upField()));

  // downField
  downFieldButton= new QPushButton(QPixmap(down12x12_xpm),"",this);
  downFieldButton->setMinimumSize(wbutton,hbutton);
  downFieldButton->setMaximumSize(wbutton,hbutton);
  downFieldButton->setEnabled( false );

  connect( downFieldButton, SIGNAL(clicked()), SLOT(downField()));

  // resetOptions
  resetOptionsButton = NormalPushButton(tr("R"), this );
  resetOptionsButton->setMinimumWidth(wbutton);
  resetOptionsButton->setEnabled( false );

  connect( resetOptionsButton, SIGNAL(clicked()), SLOT(resetOptions()));

  // minus
  minusButton = new ToggleButton( this, QPixmap(minus12x12_xpm) );
  minusButton->setMinimumSize(wbutton,hbutton);
  minusButton->setMaximumSize(wbutton,hbutton);
  minusButton->setEnabled( false );

  connect( minusButton, SIGNAL(toggled(bool)), SLOT(minusField(bool)));

  // colorCbox
  QLabel* colorlabel= new QLabel( tr("Line colour"), this );
  colorCbox=  new QComboBox( false, this );
  colorCbox->setEnabled( false );

  connect( colorCbox, SIGNAL( activated(int) ),
	   SLOT( colorCboxActivated(int) ) );

  // linewidthcbox
  QLabel* linewidthlabel= new QLabel( tr("Line width"), this );
  lineWidthCbox=  LinewidthBox( this, false);
  lineWidthCbox->setEnabled( false );

  connect( lineWidthCbox, SIGNAL( activated(int) ),
	   SLOT( lineWidthCboxActivated(int) ) );

  // linetypecbox
  QLabel* linetypelabel= new QLabel( tr("Line type"), this );
  lineTypeCbox=  LinetypeBox( this,false );

  connect( lineTypeCbox, SIGNAL( activated(int) ),
	   SLOT( lineTypeCboxActivated(int) ) );

  // lineinterval
  QLabel* lineintervallabel= new QLabel( tr("Line interval"), this );
  lineintervalCbox=  new QComboBox( false, this );
  lineintervalCbox->setEnabled( false );

  connect( lineintervalCbox, SIGNAL( activated(int) ),
	   SLOT( lineintervalCboxActivated(int) ) );

// density
  QLabel* densitylabel= new QLabel( tr("Density"), this );
  densityCbox=  new QComboBox( false, this );
  densityCbox->setEnabled( false );
  // values inserted when enabled (cleared when disabled)
  nr_densities= density.size();
  cdensities= new const char*[nr_densities];
  for (i=0; i<nr_densities; i++)
    cdensities[i]= density[i].c_str();

  connect( densityCbox, SIGNAL( activated(int) ),
	   SLOT( densityCboxActivated(int) ) );

  // vectorunit
  QLabel* vectorunitlabel= new QLabel( tr("Unit"), this );
  vectorunitCbox=  new QComboBox( false, this );
  vectorunitCbox->setEnabled( false );

  connect( vectorunitCbox, SIGNAL( activated(int) ),
	   SLOT( vectorunitCboxActivated(int) ) );

  // help
  fieldhelp = NormalPushButton( tr("Help"), this );
  connect( fieldhelp, SIGNAL(clicked()), SLOT(helpClicked()));

  // allTimeStep
  allTimeStepButton = new ToggleButton( this, tr("All time steps").latin1() );
  allTimeStepButton->setToggleButton(true);
  allTimeStepButton->setOn(false);
  connect( allTimeStepButton, SIGNAL(toggled(bool)),
	   SLOT(allTimeStepToggled(bool)));

  // advanced
  miString more_str[2] = { (tr("<<Less").latin1()), (tr("More>>").latin1()) };
  advanced= new ToggleButton( this, more_str );
  advanced->setOn(false);
  connect( advanced, SIGNAL(toggled(bool)), SLOT(advancedToggled(bool)));

  // hide
  fieldhide = NormalPushButton( tr("Hide"), this );
  connect( fieldhide, SIGNAL(clicked()), SLOT( hideClicked()));

  // applyhide
  fieldapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( fieldapplyhide, SIGNAL(clicked()), SLOT( applyhideClicked()));

  // apply
  fieldapply = NormalPushButton( tr("Apply"), this );
  connect( fieldapply, SIGNAL(clicked()), SLOT( applyClicked()));

  // layout
  v1layout = new QVBoxLayout( 5 );
  v1layout->addWidget( modelGRlabel );
  v1layout->addWidget( modelGRbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( modellabel );
  v1layout->addWidget( modelbox,2 );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( fieldGRlabel );
  v1layout->addWidget( fieldGRbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( fieldlabel );
  v1layout->addWidget( fieldbox,4 );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( selectedFieldlabel );
  v1layout->addWidget( selectedFieldbox,2 );

  QVBoxLayout* h2layout= new QVBoxLayout( 2 );
  h2layout->addWidget( upFieldButton );
  h2layout->addWidget( downFieldButton );
  h2layout->addWidget( resetOptionsButton );
  h2layout->addWidget( minusButton );
  h2layout->addStretch(1);

  v1h4layout = new QHBoxLayout( 2 );
  v1h4layout->addWidget( Delete );
  v1h4layout->addWidget( copyField );

  QHBoxLayout* vxh4layout = new QHBoxLayout( 2 );
  vxh4layout->addWidget( deleteAll );
  vxh4layout->addWidget( changeModelButton );

  QVBoxLayout* v3layout= new QVBoxLayout( 2 );
  v3layout->addLayout( v1h4layout );
  v3layout->addLayout( vxh4layout );

  QHBoxLayout* v1h5layout= new QHBoxLayout( 2 );
  v1h5layout->addWidget( historyBackButton );
  v1h5layout->addWidget( historyForwardButton );

  QVBoxLayout* v4layout= new QVBoxLayout( 2 );
  v4layout->addLayout( v1h5layout );
  v4layout->addWidget( historyOkButton, 1 );

  QHBoxLayout* h3layout= new QHBoxLayout( 2 );
  h3layout->addLayout( v3layout );
  h3layout->addLayout( v4layout );


  optlayout = new QGridLayout( 6, 2, 1 );
  optlayout->addWidget( colorlabel,       0, 0 );
  optlayout->addWidget( colorCbox,        0, 1 );
  optlayout->addWidget( linewidthlabel,   1, 0 );
  optlayout->addWidget( lineWidthCbox,    1, 1 );
  optlayout->addWidget( linetypelabel,    2, 0 );
  optlayout->addWidget( lineTypeCbox,     2, 1 );
  optlayout->addWidget( lineintervallabel,3, 0 );
  optlayout->addWidget( lineintervalCbox, 3, 1 );
  optlayout->addWidget( densitylabel,     4, 0 );
  optlayout->addWidget( densityCbox,      4, 1 );
  optlayout->addWidget( vectorunitlabel,  5, 0 );
  optlayout->addWidget( vectorunitCbox,   5, 1 );

  QHBoxLayout* levelsliderlayout= new QHBoxLayout(0);
  levelsliderlayout->setAlignment( Qt::AlignHCenter );
  levelsliderlayout->addWidget( levelSlider );

  QHBoxLayout* levelsliderlabellayout= new QHBoxLayout(0);
  levelsliderlabellayout->setAlignment( Qt::AlignHCenter );
  levelsliderlabellayout->addWidget( levelsliderlabel );

  levellayout = new QVBoxLayout(3);
  levellayout->addWidget( levelLabel );
  levellayout->addLayout( levelsliderlayout );
  levellayout->addLayout( levelsliderlabellayout );

  h4layout = new QHBoxLayout( 5 );
  h4layout->addLayout( h2layout );
  h4layout->addLayout( optlayout );
  h4layout->addLayout( levellayout );

  idnumlayout = new QHBoxLayout(3);
  idnumlayout->addWidget( idnumLabel );
  idnumlayout->addWidget( idnumSlider );
  idnumlayout->addWidget( idnumsliderlabel );

  h5layout = new QHBoxLayout( 2 );
  h5layout->addWidget( fieldhelp );
  h5layout->addWidget( allTimeStepButton );
  h5layout->addWidget( advanced );

  h6layout = new QHBoxLayout( 2 );
  h6layout->addWidget( fieldhide );
  h6layout->addWidget( fieldapplyhide );
  h6layout->addWidget( fieldapply );

  QVBoxLayout* v6layout= new QVBoxLayout( 2 );
  v6layout->addLayout( h5layout );
  v6layout->addLayout( h6layout );

  // vlayout
#ifdef DISPLAY1024X768
  vlayout = new QVBoxLayout( this, 5, 5 );
#else
  vlayout = new QVBoxLayout( this, 10, 10 );
#endif
  vlayout->addLayout( v1layout );
  vlayout->addLayout( h3layout );
  vlayout->addLayout( h4layout );
  vlayout->addLayout( idnumlayout );
  vlayout->addLayout( v6layout );

  vlayout->activate();

  CreateAdvanced();

  this->setOrientation(Qt::Horizontal);
  this->setExtension(advFrame);
  advancedToggled( false );

  lastFieldGroupName= "Bakke m.m.";
//##############################################
//  lastFieldGroupName= tr("Surface etc."""";
//##############################################

  // tool tips
  toolTips();

  archiveMode( false );

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::ConstructorCernel returned"<<endl;
#endif
}


void FieldDialog::toolTips()
{
  QToolTip::add( upFieldButton,       tr("move selected field up") );
  QToolTip::add( downFieldButton,     tr("move selected field down") );
  QToolTip::add( Delete,              tr("delete selected field") );
  QToolTip::add( deleteAll,           tr("delete all selected fields") );
  QToolTip::add( copyField,           tr("copy field") );
  QToolTip::add( resetOptionsButton,  tr("reset plot options") );
  QToolTip::add( minusButton,         tr("selected field minus the field above") );
  QToolTip::add( changeModelButton,   tr("change model/termin") );
  QToolTip::add( historyBackButton,   tr("history back") );
  QToolTip::add( historyForwardButton,tr("history forward") );
  QToolTip::add( historyOkButton,     tr("use history shown") );
  QToolTip::add( allTimeStepButton,
		 tr("all time steps / only common time steps") );
  QToolTip::add( valueLabelCheckBox,  tr("numbers on the contour lines") );

  QToolTip::add( gridValueCheckBox,   
		 tr("Grid values, but only when a few grid points are visible") );
  QToolTip::add( gridLinesSpinBox,    tr("Grid lines, 1=all") );
//   QToolTip::add( gridLinesMaxSpinBox, 
// 		 tr("Grid lines, but only when a few grid points are visible") );
  QToolTip::add( undefColourCbox,     tr("Undef colour") );
  QToolTip::add( undefLinewidthCbox,  tr("Undef linewidth") );
  QToolTip::add( undefLinetypeCbox,   tr("Undef linetype") );
  QToolTip::add( shadingSpinBox,      tr("number of colours in the palette") );
  QToolTip::add( shadingcoldComboBox, tr("Palette for values below basis") );
  QToolTip::add( shadingcoldSpinBox,  tr("number of colours in the palette") );
  QToolTip::add( patternColourBox,    tr("Colour of pattern") );
}

void FieldDialog::advancedToggled(bool on){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::advancedToggled  on= " << on <<endl;
#endif

  this->showExtension(on);

}


void FieldDialog::CreateAdvanced() {
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::CreateAdvanced" <<endl;
#endif

  advFrame= new QWidget(this,"frame");

  // Extreme (min,max): type, size and search radius
  QLabel* extremeTypeLabel= TitleLabel( tr("Min,max"), advFrame );
  extremeTypeCbox= new QComboBox( false, advFrame );
  extremeTypeCbox->setEnabled( false );
  extremeType.push_back(tr("None").latin1());
  extremeType.push_back("L+H");
  extremeType.push_back("C+W");
  connect( extremeTypeCbox, SIGNAL( activated(int) ),
	   SLOT( extremeTypeActivated(int) ) );

  QLabel* extremeSizeLabel= new QLabel( tr("Size"),  advFrame );
  extremeSizeSpinBox= new QSpinBox( 5,300,5, advFrame );
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled( false );
  connect( extremeSizeSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( extremeSizeChanged(int) ) );

  QLabel* extremeRadiusLabel= new QLabel( tr("Radius"), advFrame );
  extremeRadiusSpinBox= new QSpinBox( 5,300,5, advFrame );
  extremeRadiusSpinBox->setWrapping(true);
  extremeRadiusSpinBox->setSuffix("%");
  extremeRadiusSpinBox->setValue(100);
  extremeRadiusSpinBox->setEnabled( false );
  connect( extremeRadiusSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( extremeRadiusChanged(int) ) );

  // line smoothing
  QLabel* lineSmoothLabel= new QLabel( tr("Smooth lines"), advFrame );
  lineSmoothSpinBox= new QSpinBox( 0,50,2, advFrame );
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled( false );
  connect( lineSmoothSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( lineSmoothChanged(int) ) );

  // field smoothing
  QLabel* fieldSmoothLabel= new QLabel( tr("Smooth fields"), advFrame );
  fieldSmoothSpinBox= new QSpinBox( 0,10,1, advFrame );
  fieldSmoothSpinBox->setSpecialValueText(tr("Off"));
  fieldSmoothSpinBox->setValue(0);
  fieldSmoothSpinBox->setEnabled( false );
  connect( fieldSmoothSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( fieldSmoothChanged(int) ) );

  //  QLabel* labelSizeLabel= new QLabel( "Tallstørrelse",  advFrame );
  labelSizeSpinBox= new QSpinBox( 5,300,5, advFrame );
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled( false );
  connect( labelSizeSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( labelSizeChanged(int) ) );

  // grid values
  //  QLabel* gridLabel= TitleLabel( tr("Grid"), advFrame );
  gridValueCheckBox= new QCheckBox(QString(tr("Grid value")), advFrame);
  gridValueCheckBox->setChecked( false );
  gridValueCheckBox->setEnabled( false );
  connect( gridValueCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( gridValueCheckBoxToggled(bool) ) );

  // grid lines
  QLabel* gridLinesLabel= new QLabel( tr("Grid lines"), advFrame );
  gridLinesSpinBox= new QSpinBox( 0,50,1, advFrame );
  gridLinesSpinBox->setSpecialValueText(tr("Off"));
  gridLinesSpinBox->setValue(0);
  gridLinesSpinBox->setEnabled( false );
  connect( gridLinesSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( gridLinesChanged(int) ) );

  // grid lines max
//   QLabel* gridLinesMaxLabel= new QLabel( tr("Max grid l."), advFrame );
//   gridLinesMaxSpinBox= new QSpinBox( 0,200,5, advFrame );
//   gridLinesMaxSpinBox->setSpecialValueText(tr("All"));
//   gridLinesMaxSpinBox->setValue(0);
//   gridLinesMaxSpinBox->setEnabled( false );
//   connect( gridLinesMaxSpinBox, SIGNAL( valueChanged(int) ),
// 	   SLOT( gridLinesMaxChanged(int) ) );


  QLabel* hourOffsetLabel= new QLabel( tr("Time offset"),  advFrame );
  hourOffsetSpinBox= new QSpinBox( -72,72,1, advFrame );
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->setEnabled( false );
  connect( hourOffsetSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( hourOffsetChanged(int) ) );

  QLabel* hourDiffLabel= new QLabel( tr("Time diff."),  advFrame );
  hourDiffSpinBox= new QSpinBox( 0,12,1, advFrame );
  hourDiffSpinBox->setSuffix(tr(" hour(s)"));
  hourDiffSpinBox->setPrefix(" +/-");
  hourDiffSpinBox->setValue(0);
  hourDiffSpinBox->setEnabled( false );
  connect( hourDiffSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( hourDiffChanged(int) ) );

  // Undefined masking
  QLabel* undefMaskingLabel= TitleLabel( tr("Undefined"), advFrame );
  undefMaskingCbox= new QComboBox( false, advFrame );
  undefMaskingCbox->setEnabled( false );
  undefMasking.push_back(tr("Unmarked").latin1());
  undefMasking.push_back(tr("Coloured").latin1());
  undefMasking.push_back(tr("Lines").latin1());
  connect( undefMaskingCbox, SIGNAL( activated(int) ),
	   SLOT( undefMaskingActivated(int) ) );

  // Undefined masking colour
  undefColourCbox= new QComboBox( false, advFrame );
  undefColourCbox->setEnabled( false );
  connect( undefColourCbox, SIGNAL( activated(int) ),
	   SLOT( undefColourActivated(int) ) );

  // Undefined masking linewidth
  undefLinewidthCbox= LinewidthBox( advFrame,false );
  undefLinewidthCbox->setEnabled( false );
  connect( undefLinewidthCbox, SIGNAL( activated(int) ),
	   SLOT( undefLinewidthActivated(int) ) );

  // Undefined masking linetype
  undefLinetypeCbox= LinetypeBox( advFrame,false );
  undefLinetypeCbox->setEnabled( false );
  connect( undefLinetypeCbox, SIGNAL( activated(int) ),
	   SLOT( undefLinetypeActivated(int) ) );

  // enable/disable numbers on isolines
  valueLabelCheckBox= new QCheckBox(QString(tr("Numbers")), advFrame);
  valueLabelCheckBox->setChecked( true );
  valueLabelCheckBox->setEnabled( false );
  connect( valueLabelCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( valueLabelCheckBoxToggled(bool) ) );

  //Options
  QLabel* shadingLabel    = new QLabel( tr("Palette"),            advFrame );
  QLabel* shadingcoldLabel= new QLabel( tr("Palette (-)"),        advFrame );
  QLabel* patternLabel    = new QLabel( tr("Pattern"),            advFrame );
  QLabel* alphaLabel      = new QLabel( tr("Alpha"),              advFrame );
  QLabel* headLabel       = TitleLabel( tr("Extra contour lines"),advFrame);
  QLabel* colourLabel     = new QLabel( tr("Line colour"),        advFrame);
  QLabel* intervalLabel   = new QLabel( tr("Line interval"),      advFrame);
  QLabel* baseLabel       = new QLabel( tr("Basis value"),        advFrame);
  QLabel* minLabel        = new QLabel( tr("Min"),                advFrame);
  QLabel* maxLabel        = new QLabel( tr("Max"),                advFrame);
  QLabel* base2Label      = new QLabel( tr("Basis value"),        advFrame);
  QLabel* min2Label       = new QLabel( tr("Min"),                advFrame);
  QLabel* max2Label       = new QLabel( tr("Max"),                advFrame);
  QLabel* linewidthLabel  = new QLabel( tr("Line width"),         advFrame);
  QLabel* linetypeLabel   = new QLabel( tr("Line type"),          advFrame);
  QLabel* threeColourLabel=TitleLabel( tr("Three colours"),      advFrame);

  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
  tableCheckBox->setEnabled(false);
  connect( tableCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( tableCheckBoxToggled(bool) ) );

  repeatCheckBox = new QCheckBox(tr("Repeat"), advFrame);
  repeatCheckBox->setEnabled(false);
  connect( repeatCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( repeatCheckBoxToggled(bool) ) );

  //3 colours
  //  threeColoursCheckBox = new QCheckBox(tr("Three colours"), advFrame);

  for(int i=0;i<3;i++){
    threeColourBox.push_back(ColourBox(advFrame,colourInfo,true,0,tr("Off").latin1()));
    connect( threeColourBox[i], SIGNAL( activated(int) ),
	     SLOT( threeColoursChanged() ) );
  }

  //shading
  shadingComboBox=  PaletteBox( advFrame,csInfo,false,0,tr("Off").latin1() );
  connect( shadingComboBox, SIGNAL( activated(int) ),
	   SLOT( shadingChanged() ) );

  shadingSpinBox = new QSpinBox(0,99,1,advFrame);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  shadingSpinBox->setEnabled(false);
  connect( shadingSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( shadingChanged() ) );

  shadingcoldComboBox=  PaletteBox( advFrame,csInfo,false,0,tr("Off").latin1() );
  connect( shadingcoldComboBox, SIGNAL( activated(int) ),
	   SLOT( shadingChanged() ) );

  shadingcoldSpinBox = new QSpinBox(0,99,1,advFrame);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  shadingcoldSpinBox->setEnabled(false);
  connect( shadingcoldSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( shadingChanged() ) );

  //pattern
  patternComboBox = PatternBox( advFrame,patternInfo,false,0,tr("Off").latin1() );
  connect( patternComboBox, SIGNAL( activated(int) ),
	   SLOT( patternComboBoxToggled(int) ) );

  //pattern colour
  patternColourBox = ColourBox(advFrame,colourInfo,false,0,tr("Auto").latin1());
  connect( patternColourBox, SIGNAL( activated(int) ),
	   SLOT( patternColourBoxToggled(int) ) );

  //alpha blending
  alphaSpinBox = new QSpinBox(0,255,5,advFrame);
  alphaSpinBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  connect( alphaSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( alphaChanged(int) ) );

  //colour
  colour2ComboBox = ColourBox(advFrame,colourInfo,false,0,tr("Off").latin1());
  connect( colour2ComboBox, SIGNAL( activated(int) ),
	   SLOT( colour2ComboBoxToggled(int) ) );

  //line interval
  interval2ComboBox = new QComboBox(advFrame);
  interval2ComboBox->setEnabled( false );
  connect( interval2ComboBox, SIGNAL( activated(int) ),
 	   SLOT( interval2ComboBoxToggled(int) ) );

  //zero value
  zero1ComboBox= new QComboBox( advFrame );
  zero2ComboBox = new QComboBox(advFrame);
  zero1ComboBox->setEnabled( false );
  zero2ComboBox->setEnabled( false );
  connect( zero1ComboBox, SIGNAL( activated(int) ),
	   SLOT( zero1ComboBoxToggled(int) ) );
  connect( zero2ComboBox, SIGNAL( activated(int) ),
	   SLOT( zero2ComboBoxToggled(int) ) );

  //min
  min1ComboBox = new QComboBox(advFrame);
  min2ComboBox = new QComboBox(advFrame);
  min1ComboBox->setEnabled( false );
  min2ComboBox->setEnabled( false );

  //max
  max1ComboBox = new QComboBox(advFrame);
  max2ComboBox = new QComboBox(advFrame);
  max1ComboBox->setEnabled( false );
  max2ComboBox->setEnabled( false );

  connect( min1ComboBox, SIGNAL( activated(int) ),
	   SLOT( min1ComboBoxToggled(int) ) );
  connect( max1ComboBox, SIGNAL( activated(int) ),
	   SLOT( max1ComboBoxToggled(int) ) );
  connect( min2ComboBox, SIGNAL( activated(int) ),
	   SLOT( min2ComboBoxToggled(int) ) );
  connect( max2ComboBox, SIGNAL( activated(int) ),
	   SLOT( max2ComboBoxToggled(int) ) );

  //linewidth
  linewidth2ComboBox = LinewidthBox( advFrame);
  linewidth2ComboBox->setEnabled( false );
  connect( linewidth2ComboBox, SIGNAL( activated(int) ),
	   SLOT( linewidth2ComboBoxToggled(int) ) );
  //linetype
  linetype2ComboBox = LinetypeBox( advFrame,false );
  linetype2ComboBox->setEnabled( false );
  connect( linetype2ComboBox, SIGNAL( activated(int) ),
	   SLOT( linetype2ComboBoxToggled(int) ) );



  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox= new QCheckBox(QString(tr("Zero line")), advFrame);
    //  zeroLineColourCBox= new QComboBox(advFrame);
  zeroLineCheckBox->setChecked( true );

  //  zeroLineCheckBox->setEnabled( false );
  zeroLineCheckBox->setEnabled( true );
  connect( zeroLineCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( zeroLineCheckBoxToggled(bool) ) );

  // Create horizontal frame lines
  QFrame *line0 = new QFrame( advFrame );
  line0->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line1 = new QFrame( advFrame );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line2 = new QFrame( advFrame );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line3 = new QFrame( advFrame );
  line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line4 = new QFrame( advFrame );
  line4->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line5 = new QFrame( advFrame );
  line5->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line6 = new QFrame( advFrame );
  line6->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  // layout......................................................

  QGridLayout* advLayout = new QGridLayout( 33, 3, 1 );
  int line = 0;
  advLayout->addWidget( extremeTypeLabel,    line, 0 );
  advLayout->addWidget(extremeSizeLabel,     line, 1 );
  advLayout->addWidget(extremeRadiusLabel,   line, 2 );
  line++;
  advLayout->addWidget( extremeTypeCbox,     line, 0 );
  advLayout->addWidget(extremeSizeSpinBox,   line, 1 );
  advLayout->addWidget(extremeRadiusSpinBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line0,    line,line,0,2 );

  line++;
  advLayout->addWidget(gridLinesLabel,      line, 0 );
  advLayout->addWidget(gridLinesSpinBox,     line, 1 );
  advLayout->addWidget(gridValueCheckBox,    line, 2 );
  line++;
  advLayout->addWidget(lineSmoothLabel,      line, 0 );
  advLayout->addWidget(lineSmoothSpinBox,    line, 1 );
  line++;
  advLayout->addWidget(fieldSmoothLabel,     line, 0 );
  advLayout->addWidget(fieldSmoothSpinBox,   line, 1 );
  line++;
  advLayout->addWidget(hourOffsetLabel,      line, 0 );
  advLayout->addWidget(hourOffsetSpinBox,    line, 1 );
  line++;
  advLayout->addWidget(hourDiffLabel,        line, 0 );
  advLayout->addWidget(hourDiffSpinBox,      line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line4,   line,line, 0,2 );
  line++;
  advLayout->addWidget(undefMaskingLabel,   line, 0 );
  advLayout->addWidget(undefMaskingCbox,    line, 1 );
  line++;
  advLayout->addWidget(undefColourCbox,     line, 0 );
  advLayout->addWidget(undefLinewidthCbox,  line, 1 );
  advLayout->addWidget(undefLinetypeCbox,   line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line1,   line,line, 0,3 );

  line++;
  advLayout->addWidget(zeroLineCheckBox,    line, 0 );
  advLayout->addWidget(valueLabelCheckBox,  line, 1 );
  advLayout->addWidget(labelSizeSpinBox,    line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line2,   line,line, 0,2 );

  line++;
  advLayout->addWidget( tableCheckBox,      line, 0 );
  advLayout->addWidget( repeatCheckBox,  line, 1 );
  line++;
  advLayout->addWidget( shadingLabel,       line, 0 );
  advLayout->addWidget( shadingComboBox,    line, 1 );
  advLayout->addWidget( shadingSpinBox,     line, 2 );
  line++;
  advLayout->addWidget( shadingcoldLabel,   line, 0 );
  advLayout->addWidget( shadingcoldComboBox,line, 1 );
  advLayout->addWidget( shadingcoldSpinBox, line, 2 );
  line++;
  advLayout->addWidget( patternLabel,       line, 0 );
  advLayout->addWidget( patternComboBox,    line, 1 );
  advLayout->addWidget( patternColourBox,   line, 2 );
  line++;
  advLayout->addWidget( alphaLabel,         line, 0 );
  advLayout->addWidget( alphaSpinBox,       line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line6,   line,line, 0,2 );

  line++;
  advLayout->addWidget( baseLabel,          line, 0 );
  advLayout->addWidget( minLabel,           line, 1 );
  advLayout->addWidget( maxLabel,           line, 2 );
  line++;
  advLayout->addWidget( zero1ComboBox,      line, 0 );
  advLayout->addWidget( min1ComboBox,       line, 1 );
  advLayout->addWidget( max1ComboBox,       line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line3,   line,line, 0,2 );

  line++;
  advLayout->addMultiCellWidget(headLabel,line,line,0,1);
  line++;
  advLayout->addWidget( colourLabel,        line, 0 );
  advLayout->addWidget( colour2ComboBox,    line, 1 );
  line++;
  advLayout->addWidget( intervalLabel,      line, 0 );
  advLayout->addWidget( interval2ComboBox,  line, 1 );
  line++;
  advLayout->addWidget( linewidthLabel,     line, 0 );
  advLayout->addWidget( linewidth2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( linetypeLabel,      line, 0 );
  advLayout->addWidget( linetype2ComboBox,  line, 1 );
  line++;
  advLayout->addWidget( base2Label,         line, 0 );
  advLayout->addWidget( min2Label,          line, 1 );
  advLayout->addWidget( max2Label,          line, 2 );
  line++;
  advLayout->addWidget( zero2ComboBox,      line, 0 );
  advLayout->addWidget( min2ComboBox,       line, 1 );
  advLayout->addWidget( max2ComboBox,       line, 2 );

  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addMultiCellWidget(line5,      line,line, 0,2 );
  line++;
  advLayout->addWidget( threeColourLabel,    line, 0 );
  //  advLayout->addWidget( threeColoursCheckBox, 38, 0 );
  line++;
  advLayout->addWidget( threeColourBox[0],    line, 0 );
  advLayout->addWidget( threeColourBox[1],    line, 1 );
  advLayout->addWidget( threeColourBox[2],    line, 2 );

  // a separator
  QFrame* advSep= new QFrame( advFrame );
  advSep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  advSep->setLineWidth( 5 );

  QHBoxLayout *hLayout = new QHBoxLayout( advFrame,5,5 );

  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);

  return;
}


void FieldDialog::archiveMode( bool on )
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::archiveMode called"<<endl;
#endif

  useArchive= on;

  modelGRbox->clear();
  indexMGRtable.clear();

  int nr_m = m_modelgroup.size();
  if (nr_m==0) return;

  const char** cvstr= new const char*[nr_m];
  int m= 0;
  if (useArchive) {
    for (int i=0; i<nr_m; i++) {
      if (m_modelgroup[i].archiveGroup) {
	indexMGRtable.push_back(i);
        cvstr[m++]=  m_modelgroup[i].groupName.c_str();
      }
    }
  }
  for (int i=0; i<nr_m; i++) {
    if (!m_modelgroup[i].archiveGroup) {
      indexMGRtable.push_back(i);
      cvstr[m++]=  m_modelgroup[i].groupName.c_str();
    }
  }

  modelGRbox->insertStrList( cvstr, m );
  modelGRbox->setCurrentItem(0);

  delete[] cvstr;

  if (selectedFields.size()>0) deleteAllSelected();

  // show models in the first modelgroup
  modelGRboxActivated(0);

}

// add modelgroups and models
void FieldDialog::addModelGroup(const FieldDialogInfo& fdi)
{

  m_modelgroup.insert(m_modelgroup.begin(),fdi);
  archiveMode( useArchive );


}

void FieldDialog::modelGRboxActivated( int index )
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelGRboxActivated called"<<endl;
#endif

  if (index<0 || index>=indexMGRtable.size()) return;

  int indexMGR= indexMGRtable[index];
  modelbox->clear();
  fieldGRbox->clear();
  fieldbox->clear();
  fieldGRbox->setEnabled(false);
  fieldbox->setEnabled(false);
  levelSlider->setEnabled(false);
  levelLabel->clear();

  int nr_model = m_modelgroup[indexMGR].modelNames.size();
  const char** cvstr= new const char*[nr_model];

  for( int i=0; i<nr_model; i++ )
    cvstr[i]=  m_modelgroup[indexMGR].modelNames[i].c_str();

  modelbox->insertStrList( cvstr, nr_model );
  modelbox->setEnabled( true );

  delete[] cvstr;

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelGRboxActivated returned"<<endl;
#endif
}


void FieldDialog::modelboxHighlighted( int index ){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelboxHighlighted called"<<endl;
#endif

  fieldGRbox->clear();
  fieldbox->clear();
  levelSlider->setEnabled(false);
  levelLabel->clear();

  int indexM = index;
  int indexMGR = indexMGRtable[modelGRbox->currentItem()];

  miString model = m_modelgroup[indexMGR].modelNames[indexM];

  getFieldGroups(model,indexMGR,indexM,vfgi);

  int i, indexFGR, nvfgi = vfgi.size();

  if (nvfgi>0) {
    const char** cvstr= new const char*[nvfgi];
    for (i=0; i<nvfgi; i++)
      cvstr[i]=  vfgi[i].groupName.c_str();
    fieldGRbox->insertStrList( cvstr, nvfgi );
    fieldGRbox->setEnabled( true );
    delete[] cvstr;
    indexFGR= -1;
    i= 0;
    while (i<nvfgi && vfgi[i].groupName!=lastFieldGroupName) i++;
    if (i<nvfgi) {
      indexFGR=i;
    } else {
      int l1,l2= lastFieldGroupName.length(), lm=0;
      for (i=0; i<nvfgi; i++) {
	l1= vfgi[i].groupName.length();
	if (l1>l2) l1=l2;
	if (l1>lm && vfgi[i].groupName.substr(0,l1)==
	             lastFieldGroupName.substr(0,l1)) {
	  lm= l1;
	  indexFGR=i;
	}
      }
    }
    if (indexFGR<0) indexFGR= 0;
    lastFieldGroupName= vfgi[indexFGR].groupName;
    fieldGRbox->setCurrentItem(indexFGR);
    fieldGRboxActivated(indexFGR);
  }

  if (!selectedFields.empty()) {
    // possibly enable changeModel button
    enableFieldOptions();
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelboxHighlighted returned"<<endl;
#endif
}


void FieldDialog::fieldGRboxActivated( int index ){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldGRboxActivated called"<<endl;
#endif

  fieldbox->clear();
  fieldbox->setEnabled( false );
  fieldbox->blockSignals(true);

  int i, j, n;
  int last=-1;

  if (vfgi.size()>0) {

    int indexMGR = indexMGRtable[modelGRbox->currentItem()];
    int indexM   = modelbox->currentItem();
    int indexFGR = index;

    lastFieldGroupName= vfgi[indexFGR].groupName;

    int nfield= vfgi[indexFGR].fieldNames.size();

    const char** cvstr= new const char*[nfield];
    for (i=0; i<nfield; i++)
      cvstr[i]= vfgi[indexFGR].fieldNames[i].c_str();

    fieldbox->insertStrList(cvstr, nfield);
    fieldbox->setEnabled( true );
    delete[] cvstr;

    countSelected.resize(nfield);
    for (i=0; i<nfield; ++i) countSelected[i]= 0;

    n= selectedFields.size();
    int ml;

    for (i=0; i<n; ++i) {
      if (!selectedFields[i].inEdit &&
          selectedFields[i].indexMGR==indexMGR &&
	  selectedFields[i].indexM  ==indexM) {
	j=0;
	if( selectedFields[i].modelName==vfgi[indexFGR].modelName) {
	  while (j<nfield && selectedFields[i].fieldName!=
		 vfgi[indexFGR].fieldNames[j])  j++;
	  if (j<nfield) {
	    if ((ml=vfgi[indexFGR].levelNames.size())>0) {
	      int l= 0;
	      while (l<ml && vfgi[indexFGR].levelNames[l]!=selectedFields[i].level) l++;
	      if (l==ml) j= nfield;
	    }
	    if ((ml=vfgi[indexFGR].idnumNames.size())>0) {
	      int l= 0;
	      while (l<ml && vfgi[indexFGR].idnumNames[l]!=selectedFields[i].idnum) l++;
	      if (l==ml) j= nfield;
	    }
	    if (j<nfield) {
	      fieldbox->setSelected(j,true);
	      countSelected[j]++;
	      last= i;
	    }
	  }
	}
      }
    }
  }

  if (last>=0) {
    selectedFieldbox->setCurrentItem(last);
    selectedFieldbox->setSelected(last,true);
    enableFieldOptions();
  } else if (selectedFields.empty()) {
    disableFieldOptions();
  }

  fieldbox->blockSignals(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldGRboxActivated returned"<<endl;
#endif
}


void FieldDialog::setLevel(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setLevel called"<<endl;
#endif

  if (!selectedFields.empty()) {

    int i= selectedFieldbox->currentItem();
    int n=0, l=0;
    if (selectedFields[i].level.exists()) {
      lastLevel= selectedFields[i].level;
      n= selectedFields[i].levelOptions.size();
      l= 0;
      while (l<n && selectedFields[i].levelOptions[l]!=lastLevel) l++;
      if (l==n) l= 0;  // should not happen!
    }

    levelSlider->blockSignals(true);

    if (n>0) {
      currentLevels= selectedFields[i].levelOptions;
      levelSlider->setRange(0,n-1);
      levelSlider->setValue( l );
      levelSlider->setEnabled(true);
      QString qstr= lastLevel.c_str();
      levelLabel->setText(qstr);
    } else {
      currentLevels.clear();
      // keep slider in a fixed position when disabled
      levelSlider->setEnabled(false);
      levelSlider->setValue(1);
      levelSlider->setRange(0,1);
      levelLabel->clear();
    }

    levelSlider->blockSignals(false);
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setLevel returned"<<endl;
#endif
  return;
}


void FieldDialog::levelPressed(){
  levelInMotion= true;
}


void FieldDialog::levelChanged( int index ){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::levelChanged called"<<endl;
#endif

  int n= currentLevels.size();
  if (index>=0 && index<n) {
    QString qstr= currentLevels[index].c_str();
    levelLabel->setText(qstr);
    lastLevel= currentLevels[index];
  }

  if (!levelInMotion) updateLevel();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::levelChanged returned"<<endl;
#endif
  return;
}


void FieldDialog::changeLevel(const miString& level)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::changeLevel called"<<endl;
#endif
  // called from MainWindow levelUp/levelDown

  int index= selectedFieldbox->currentItem();
  bool setlevel= false;

  int i, n= selectedFields.size();

  for (i=0; i<n; i++) {
    if (selectedFields[i].level==levelOKspec) {
      selectedFields[i].level= level;
      if (i==index) setlevel= true;
    }
  }

  levelOKspec= level;

  if (setlevel) {
    n= currentLevels.size();
    i= 0;
    while (i<n && currentLevels[i]!=levelOKspec) i++;
    if (i<n) {
      levelSlider->blockSignals(true);
      levelSlider->setValue( i );
      levelSlider->blockSignals(false);
      levelChanged(i);
    }
  }
}


void FieldDialog::updateLevel(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateLevel called"<<endl;
#endif

  int i= selectedFieldbox->currentItem();

  if (i>=0 && i<selectedFields.size()) {
    selectedFields[i].level= lastLevel;
    updateTime();
  }

  levelInMotion= false;

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateLevel returned"<<endl;
#endif
  return;
}


void FieldDialog::setIdnum(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setIdnum called"<<endl;
#endif

  if (!selectedFields.empty()) {

    int i= selectedFieldbox->currentItem();
    int n=0, l=0;
    if (selectedFields[i].idnum.exists()) {
      lastIdnum= selectedFields[i].idnum;
      n= selectedFields[i].idnumOptions.size();
      l= 0;
      while (l<n && selectedFields[i].idnumOptions[l]!=lastIdnum) l++;
      if (l==n) l= 0;  // should not happen!
    }

    idnumSlider->blockSignals(true);

    if (n>0) {
      currentIdnums= selectedFields[i].idnumOptions;
      idnumSlider->setRange(0,n-1);
      idnumSlider->setValue( l );
      idnumSlider->setEnabled(true);
      QString qstr= lastIdnum.c_str();
      idnumLabel->setText(qstr);
    } else {
      currentIdnums.clear();
      // keep slider in a fixed position when disabled
      idnumSlider->setEnabled(false);
      idnumSlider->setValue(1);
      idnumSlider->setRange(0,1);
      idnumLabel->clear();
    }

    idnumSlider->blockSignals(false);
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setIdnum returned"<<endl;
#endif
  return;
}


void FieldDialog::idnumPressed(){
  idnumInMotion= true;
}


void FieldDialog::idnumChanged( int index ){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::idnumChanged called"<<endl;
#endif

  int n= currentIdnums.size();
  if (index>=0 && index<n) {
    QString qstr= currentIdnums[index].c_str();
    idnumLabel->setText(qstr);
    lastIdnum= currentIdnums[index];
  }

  if (!idnumInMotion) updateIdnum();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::idnumChanged returned"<<endl;
#endif
  return;
}


void FieldDialog::changeIdnum(const miString& idnum)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::changeIdnum called"<<endl;
#endif
  // called from MainWindow idnumUp/idnumDown

  int index= selectedFieldbox->currentItem();
  bool setidnum= false;

  int i, n= selectedFields.size();

  for (i=0; i<n; i++) {
    if (selectedFields[i].idnum==idnumOKspec) {
      selectedFields[i].idnum= idnum;
      if (i==index) setidnum= true;
    }
  }

  idnumOKspec= idnum;

  if (setidnum) {
    n= currentIdnums.size();
    i= 0;
    while (i<n && currentIdnums[i]!=idnumOKspec) i++;
    if (i<n) {
      idnumSlider->blockSignals(true);
      idnumSlider->setValue( i );
      idnumSlider->blockSignals(false);
      idnumChanged(i);
    }
  }
}


void FieldDialog::updateIdnum(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateIdnum called"<<endl;
#endif

  int i= selectedFieldbox->currentItem();

  if (i>=0 && i<selectedFields.size()) {
    selectedFields[i].idnum= lastIdnum;
    updateTime();
  }

  idnumInMotion= false;

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateIdnum returned"<<endl;
#endif
  return;
}


void FieldDialog::fieldboxChanged(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldboxChanged called"<<endl;
#endif

  if (historyOkButton->isEnabled()) deleteAllSelected();

  int i, j, jp, n;
  int indexMGR = indexMGRtable[modelGRbox->currentItem()];
  int indexM   = modelbox->currentItem();
  int indexFGR = fieldGRbox->currentItem();

  // Note: multiselection list, current item may only be the last...

  int nf = vfgi[indexFGR].fieldNames.size();

  int last= -1;
  int lastdelete= -1;

  for (int indexF=0; indexF<nf; ++indexF) {

    if (fieldbox->isSelected(indexF) && countSelected[indexF]==0) {

      SelectedField sf;
      sf.inEdit=        false;
      sf.external=      false;
      sf.forecastSpec=  false;
      sf.indexMGR=      indexMGR;
      sf.indexM=        indexM;
      sf.modelName=     vfgi[indexFGR].modelName;
      sf.fieldName=     vfgi[indexFGR].fieldNames[indexF];
      sf.levelOptions=  vfgi[indexFGR].levelNames;
      sf.idnumOptions=  vfgi[indexFGR].idnumNames;
      sf.minus=         false;

      if (!vfgi[indexFGR].defaultLevel.empty()) {
	n= vfgi[indexFGR].levelNames.size();
	i= 0;
	while (i<n && vfgi[indexFGR].levelNames[i]!=lastLevel) i++;
	if (i<n) sf.level= lastLevel;
	else     sf.level= vfgi[indexFGR].defaultLevel;
      }
      if (!vfgi[indexFGR].defaultIdnum.empty()) {
	n= vfgi[indexFGR].idnumNames.size();
	i= 0;
	while (i<n && vfgi[indexFGR].idnumNames[i]!=lastIdnum) i++;
	if (i<n) sf.idnum= lastIdnum;
	else     sf.idnum= vfgi[indexFGR].defaultIdnum;
      }

      sf.hourOffset= 0;
      sf.hourDiff= 0;

      sf.fieldOpts= getFieldOptions(sf.fieldName, false);

      selectedFields.push_back(sf);

      countSelected[indexF]++;

      miString text= sf.modelName + " " + sf.fieldName;
      selectedFieldbox->insertItem(QString(text.c_str()));
      last= selectedFields.size()-1;

    } else if (!fieldbox->isSelected(indexF) && countSelected[indexF]>0) {

      miString fieldName= vfgi[indexFGR].fieldNames[indexF];
      n= selectedFields.size();
      j= jp= -1;
      int jsel=-1, isel= selectedFieldbox->currentItem();
      for (i=0; i<n; i++) {
	if (selectedFields[i].indexMGR   ==indexMGR    &&
	    selectedFields[i].indexM     ==indexM      &&
            selectedFields[i].fieldName==fieldName) {
	  jp=j;
	  j=i;
	  if (i==isel) jsel=isel;
	}
      }
      if (j>=0) {   // anything else is a program error!
	// remove item in selectedFieldbox,
        // a field may be selected (by copy) more than once,
        // the selected or the last of those are removed
        if (jsel>=0 && jp>=0) {
	  jp= j;
	  j= jsel;
        }
	if (jp>=0) {
	  fieldbox->blockSignals(true);
	  fieldbox->setCurrentItem(indexF);
	  fieldbox->setSelected(indexF, true);
	  fieldbox->blockSignals(false);
	  lastdelete= jp;
	} else {
	  lastdelete= j;
	}
	countSelected[indexF]--;
        selectedFieldbox->removeItem(j);
	for (i=j; i<n-1; i++)
	  selectedFields[i]=selectedFields[i+1];
	selectedFields.pop_back();
      }
    }
  }

  if (last<0 && lastdelete>=0 && selectedFields.size()>0) {
    last= lastdelete;
    if (last>=selectedFields.size()) last= selectedFields.size() - 1;
  }

  if (last>=0) {
    selectedFieldbox->setCurrentItem(last);
    selectedFieldbox->setSelected(last,true);
    enableFieldOptions();
  } else if (selectedFields.size()==0) {
    disableFieldOptions();
  }

  updateTime();

  //first field can't be minus
  if(selectedFieldbox->count()>0 && selectedFields[0].minus){
    minusButton->setOn(false);
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldboxChanged returned"<<endl;
#endif
  return;
}


void FieldDialog::enableFieldOptions(){
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableFieldOptions called"<<endl;
#endif

  float e;
  int   index, lastindex, nc, i, j, k, n;

  index= selectedFieldbox->currentItem();
  lastindex= selectedFields.size()-1;

  if (index<0 || index>lastindex) {
    cerr << "POGRAM ERROR.1 in FieldDialog::enableFieldOptions" << endl;
    cerr << "       index,selectedFields.size: "
	 << index << " " << selectedFields.size() << endl;
    disableFieldOptions();
    return;
  }

  if (selectedFields[index].inEdit) {
    upFieldButton->setEnabled( false );
    downFieldButton->setEnabled( false );
    changeModelButton->setEnabled( false );
    Delete->setEnabled( false );
    copyField->setEnabled( false );
  } else {
    upFieldButton->setEnabled( (index>numEditFields) );
    downFieldButton->setEnabled( (index<lastindex) );
    if (vfgi.size()==0)
      changeModelButton->setEnabled( false );
    else if (selectedFields[index].indexMGR==modelGRbox->currentItem() &&
             selectedFields[index].indexM  ==modelbox->currentItem() )
      changeModelButton->setEnabled( false );
    else
      changeModelButton->setEnabled( true );
    Delete->setEnabled( true );
    copyField->setEnabled( true );
  }

  setLevel();
  setIdnum();
  minusButton->setEnabled( index>0 && !selectedFields[index-1].minus );

  if (selectedFields[index].fieldOpts==currentFieldOpts &&
      selectedFields[index].inEdit==currentFieldOptsInEdit &&
      !selectedFields[index].minus) return;

  currentFieldOpts= selectedFields[index].fieldOpts;
  currentFieldOptsInEdit= selectedFields[index].inEdit;

//###############################################################################
//  cerr << "FieldDialog::enableFieldOptions: "
//       << selectedFields[index].fieldName << endl;
//  cerr << "             " << selectedFields[index].fieldOpts << endl;
//###############################################################################

  deleteAll->setEnabled( true );
  resetOptionsButton->setEnabled( true );

  if (selectedFields[index].minus && !minusButton->isOn())
    minusButton->setOn(true);
  else if (!selectedFields[index].minus && minusButton->isOn())
    minusButton->setOn(false);

  //hourOffset
  if (currentFieldOptsInEdit) {
    hourOffsetSpinBox->setValue(0);
    hourOffsetSpinBox->setEnabled(false);
  } else {
    i= selectedFields[index].hourOffset;
    hourOffsetSpinBox->setValue(i);
    hourOffsetSpinBox->setEnabled(true);
  }

  //hourDiff
  if (currentFieldOptsInEdit) {
    hourDiffSpinBox->setValue(0);
    hourDiffSpinBox->setEnabled(false);
  } else {
    i= selectedFields[index].hourDiff;
    hourDiffSpinBox->setValue(i);
    hourDiffSpinBox->setEnabled(true);
  }

  if (selectedFields[index].minus) return;

  vpcopt= cp->parse(selectedFields[index].fieldOpts);

//   cerr <<endl;
//   cerr <<"STRENGEN SOM PARSES:"<<selectedFields[index].fieldOpts<<endl;
//   cerr <<endl;
/*******************************************************
  n=vpcopt.size();
  bool err=false;
  bool listall= true;
  for (j=0; j<n; j++)
    if (vpcopt[j].key=="unknown") err=true;
  if (err || listall) {
    cerr << "FieldDialog::enableFieldOptions: "
         << selectedFields[index].fieldName << endl;
    cerr << "             " << selectedFields[index].fieldOpts << endl;
    for (j=0; j<n; j++) {
      cerr << "  parse " << j << " : key= " << vpcopt[j].key
	   << "  idNumber= " << vpcopt[j].idNumber << endl;
      cerr << "            allValue: " << vpcopt[j].allValue << endl;
      for (k=0; k<vpcopt[j].strValue.size(); k++)
        cerr << "               " << k << "    strValue: " << vpcopt[j].strValue[k] << endl;
      for (k=0; k<vpcopt[j].floatValue.size(); k++)
        cerr << "               " << k << "  floatValue: " << vpcopt[j].floatValue[k] << endl;
      for (k=0; k<vpcopt[j].intValue.size(); k++)
        cerr << "               " << k << "    intValue: " << vpcopt[j].intValue[k] << endl;
    }
  }
*******************************************************/

  bool enableType2Opt=false;

  // colour(s)
  if ((nc=cp->findKey(vpcopt,"colour_2"))>=0) {
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      updateFieldOptions("colour_2","off");
      colour2ComboBox->setCurrentItem(0);
    }else {
      updateFieldOptions("colour_2",colourInfo[i].name);
      colour2ComboBox->setCurrentItem(i+1);
    }
  }
  if ((nc=cp->findKey(vpcopt,"colour"))>=0) {
    enableType2Opt = true;
    shadingComboBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    if(colour2ComboBox->currentItem()>0)
      enableType2Opt = true;
    if (!colorCbox->isEnabled()) {
      colorCbox->insertItem( tr("Off") );
      for( i=0; i<nr_colors; i++)
        colorCbox->insertItem( *pmapColor[i] );
      colorCbox->setEnabled(true);
    }
    i=0;
    if(vpcopt[nc].allValue.downcase() == "off" ||
       vpcopt[nc].allValue.downcase() == "av" ){
      updateFieldOptions("colour","off");
      colorCbox->setCurrentItem(0);
    } else {
      while (i<nr_colors
	     && vpcopt[nc].allValue.downcase()!=colourInfo[i].name) i++;
      if (i==nr_colors) i=0;
      updateFieldOptions("colour",colourInfo[i].name);
      colorCbox->setCurrentItem(i+1);
    }
  } else if (colorCbox->isEnabled()) {
    colorCbox->clear();
    colorCbox->setEnabled(false);
  }

  // 3 colours
  if ((nc=cp->findKey(vpcopt,"colours"))>=0) {
    vector<miString> colours = vpcopt[nc].allValue.split(",");
    if(colours.size()==3){
      for(int j=0;j<3;j++){
	i=0;
	while (i<nr_colors && colours[j]!=colourInfo[i].name) i++;
	if(i<nr_colors)
	  threeColourBox[j]->setCurrentItem(i+1);
      }
      threeColoursChanged();
    }
  }

  //contour shading
  if ((nc=cp->findKey(vpcopt,"palettecolours"))>=0) {
    shadingComboBox->setEnabled(true);
    shadingSpinBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    shadingcoldSpinBox->setEnabled(true);
    tableCheckBox->setEnabled(true);
    patternComboBox->setEnabled(true);
    repeatCheckBox->setEnabled(true);
    alphaSpinBox->setEnabled(true);
    vector<miString> tokens = vpcopt[nc].allValue.split(",");
    vector<miString> stokens = tokens[0].split(";");
    if(stokens.size()==2)
      shadingSpinBox->setValue(atoi(stokens[1].cStr()));
    else
      shadingSpinBox->setValue(0);
    int nr_cs = csInfo.size();
    miString str;
    i=0;
    while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
    if (i==nr_cs) {
      str = "off";
      shadingComboBox->setCurrentItem(0);
      shadingcoldComboBox->setCurrentItem(0);
    }else {
      str = tokens[0];
      shadingComboBox->setCurrentItem(i+1);
    }
    if(tokens.size()==2){
      vector<miString> stokens = tokens[1].split(";");
      if(stokens.size()==2)
	shadingcoldSpinBox->setValue(atoi(stokens[1].cStr()));
      else
	shadingcoldSpinBox->setValue(0);
      i=0;
      while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
      if (i==nr_cs) {
	shadingcoldComboBox->setCurrentItem(0);
      }else {
	str += "," + tokens[1];
	shadingcoldComboBox->setCurrentItem(i+1);
      }
    } else {
      shadingcoldComboBox->setCurrentItem(0);
    }
    updateFieldOptions("palettecolours",str,-1);
  } else {
    updateFieldOptions("palettecolours","off",-1);
    shadingComboBox->setCurrentItem(0);
    shadingComboBox->setEnabled(false);
    shadingcoldComboBox->setCurrentItem(0);
    shadingcoldComboBox->setEnabled(false);
    tableCheckBox->setEnabled(false);
    updateFieldOptions("table","remove");
    patternComboBox->setEnabled(false);
    updateFieldOptions("patterns","remove");
    repeatCheckBox->setEnabled(false);
    updateFieldOptions("repeat","remove");
    alphaSpinBox->setEnabled(false);
    updateFieldOptions("alpha","remove");
  }

  //pattern
  if ((nc=cp->findKey(vpcopt,"patterns"))>=0) {
    patternComboBox->setEnabled(true);
    patternColourBox->setEnabled(true);
    miString value = vpcopt[nc].allValue;
    int nr_p = patternInfo.size();
    miString str;
    i=0;
    while (i<nr_p && value!=patternInfo[i].name) i++;
    if (i==nr_p) {
      str = "off";
      patternComboBox->setCurrentItem(0);
    }else {
      str = patternInfo[i].name;
      patternComboBox->setCurrentItem(i+1);
    }
    updateFieldOptions("patterns",str,-1);
  } else {
    updateFieldOptions("patterns","off",-1);
    patternComboBox->setCurrentItem(0);
  }

  //pattern colour
  if ((nc=cp->findKey(vpcopt,"patterncolour"))>=0) {
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      updateFieldOptions("patterncolour","remove");
      patternColourBox->setCurrentItem(0);
    }else {
      updateFieldOptions("patterncolour",colourInfo[i].name);
      patternColourBox->setCurrentItem(i+1);
    }
  }

  //table
  nc=cp->findKey(vpcopt,"table");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    tableCheckBox->setChecked( on );
    tableCheckBox->setEnabled(true);
    tableCheckBoxToggled(on);
  }

  //repeat
  nc=cp->findKey(vpcopt,"repeat");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    repeatCheckBox->setChecked( on );
    repeatCheckBox->setEnabled(true);
    repeatCheckBoxToggled(on);
  }

  //alpha shading
  if ((nc=cp->findKey(vpcopt,"alpha"))>=0) {
    if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
    else i=255;
    alphaSpinBox->setValue(i);
    alphaSpinBox->setEnabled(true);
  } else {
    alphaSpinBox->setValue(255);
    updateFieldOptions("alpha","remove");
  }

  // linetype
  if ((nc=cp->findKey(vpcopt,"linetype"))>=0) {
    if (!lineTypeCbox->isEnabled()) {
      lineTypeCbox->setEnabled(true);
      if(colour2ComboBox->currentItem()>0)
	linetype2ComboBox->setEnabled(true);
    }
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) i=0;
    updateFieldOptions("linetype",linetypes[i]);
    lineTypeCbox->setCurrentItem(i);
    if ((nc=cp->findKey(vpcopt,"linetype_2"))>=0) {
      i=0;
      while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
      if (i==nr_linetypes) i=0;
      updateFieldOptions("linetype_2",linetypes[i]);
      linetype2ComboBox->setCurrentItem(i);
    } else {
      linetype2ComboBox->setCurrentItem(0);
    }
  } else if (lineTypeCbox->isEnabled()) {
    lineTypeCbox->setEnabled(false);
    linetype2ComboBox->setEnabled(false);
  }

  // linewidth
  if ((nc=cp->findKey(vpcopt,"linewidth"))>=0) {
    if (!lineWidthCbox->isEnabled()) {
      lineWidthCbox->setEnabled(true);
      if(colour2ComboBox->currentItem()>0)
	linewidth2ComboBox->setEnabled(true);
    }
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miString(i+1)) i++;
    if (i==nr_linewidths) i=0;
    updateFieldOptions("linewidth",miString(i+1));
    lineWidthCbox->setCurrentItem(i);
    if ((nc=cp->findKey(vpcopt,"linewidth_2"))>=0) {
      i=0;
      while (i<nr_linewidths && vpcopt[nc].allValue!=miString(i+1)) i++;
      if (i==nr_linewidths) i=0;
      updateFieldOptions("linewidth_2",miString(i+1));
      linewidth2ComboBox->setCurrentItem(i);
    } else {
      linewidth2ComboBox->setCurrentItem(0);
    }
  } else if (lineWidthCbox->isEnabled()) {
    lineWidthCbox->setEnabled(false);
    linewidth2ComboBox->setEnabled(false);
  }

  // line interval (isoline contouring)
  float ekv=-1.;
  float ekv_2=-1.;
  if ((nc=cp->findKey(vpcopt,"line.interval"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) ekv=vpcopt[nc].floatValue[0];
    else ekv= 10.;
    lineintervals= numberList( lineintervalCbox, ekv);
    lineintervalCbox->setEnabled(true);
    numberList( interval2ComboBox, ekv);
    if ((nc=cp->findKey(vpcopt,"line.interval_2"))>=0) {
      if (!vpcopt[nc].floatValue.empty()) ekv_2=vpcopt[nc].floatValue[0];
      else ekv_2= 10.;
      numberList( interval2ComboBox, ekv_2);
    }
    if(colour2ComboBox->currentItem()>0)
      enableType2Opt = true;
  } else {
    interval2ComboBox->clear();
    enableType2Options(false);
    if (lineintervalCbox->isEnabled()) {
      lineintervalCbox->clear();
      lineintervalCbox->setEnabled(false);
    }
  }

  // wind/vector density
  if ((nc=cp->findKey(vpcopt,"density"))>=0) {
    if (!densityCbox->isEnabled()) {
      densityCbox->insertStrList(cdensities, nr_densities);
      densityCbox->setEnabled(true);
    }
    miString s;
    if (!vpcopt[nc].strValue.empty()) {
      s= vpcopt[nc].strValue[0];
    } else {
      s= "0";
      updateFieldOptions("density",s);
    }
    if (s=="0") {
      i=0;
    } else {
      n=density.size();
      i=1;
      while (i<n && density[i]!=s) i++;
      if (i==n) {
        density.push_back(s);
	delete[] cdensities;
	nr_densities= n;
	cdensities= new const char*[nr_densities];
	for (i=0; i<nr_densities; i++)
	  cdensities[i]= density[i].c_str();
	densityCbox->clear();
        densityCbox->insertStrList(cdensities, nr_densities);
      }
    }
    densityCbox->setCurrentItem(i);
  } else if (densityCbox->isEnabled()) {
    densityCbox->clear();
    densityCbox->setEnabled(false);
  }

  // vectorunit (vector length unit)
  if ((nc=cp->findKey(vpcopt,"vector.unit"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e= vpcopt[nc].floatValue[0];
    else e=5;
    vectorunit= numberList( vectorunitCbox, e);
    vectorunitCbox->setEnabled(true);
  } else if (vectorunitCbox->isEnabled()) {
    vectorunitCbox->clear();
    vectorunitCbox->setEnabled(false);
  }

  // extreme.type (L+H, C+W or none)
  bool extreme= false;
  if ((nc=cp->findKey(vpcopt,"extreme.type"))>=0) {
    extreme= true;
    n= extremeType.size();
    if (!extremeTypeCbox->isEnabled()) {
      const char** cvstr= new const char*[n];
      for (i=0; i<n; i++ )
        cvstr[i]=  extremeType[i].c_str();
      extremeTypeCbox->insertStrList( cvstr, n );
      delete[] cvstr;
      extremeTypeCbox->setEnabled(true);
    }
    i=0;
    while (i<n && vpcopt[nc].allValue!=extremeType[i]) i++;
    if (i==n) {
      i=0;
      updateFieldOptions("extreme.type",extremeType[i]);
    }
    extremeTypeCbox->setCurrentItem(i);
  } else if (extremeTypeCbox->isEnabled()) {
    extremeTypeCbox->clear();
    extremeTypeCbox->setEnabled(false);
  }

  if (extreme && (nc=cp->findKey(vpcopt,"extreme.size"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeSizeSpinBox->setValue(i);
    extremeSizeSpinBox->setEnabled(true);
  } else if (extremeSizeSpinBox->isEnabled()) {
    extremeSizeSpinBox->setValue(100);
    extremeSizeSpinBox->setEnabled(false);
  }

  if (extreme && (nc=cp->findKey(vpcopt,"extreme.radius"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeRadiusSpinBox->setValue(i);
    extremeRadiusSpinBox->setEnabled(true);
  } else if (extremeRadiusSpinBox->isEnabled()) {
    extremeRadiusSpinBox->setValue(100);
    extremeRadiusSpinBox->setEnabled(false);
  }

  if ((nc=cp->findKey(vpcopt,"line.smooth"))>=0) {
    if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
    else i=0;
    lineSmoothSpinBox->setValue(i);
    lineSmoothSpinBox->setEnabled(true);
  } else if (lineSmoothSpinBox->isEnabled()) {
    lineSmoothSpinBox->setValue(0);
    lineSmoothSpinBox->setEnabled(false);
  }

  if (currentFieldOptsInEdit) {
    fieldSmoothSpinBox->setValue(0);
    fieldSmoothSpinBox->setEnabled(false);
  } else if ((nc=cp->findKey(vpcopt,"field.smooth"))>=0) {
    if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
    else i=0;
    fieldSmoothSpinBox->setValue(i);
    fieldSmoothSpinBox->setEnabled(true);
  } else if (fieldSmoothSpinBox->isEnabled()) {
    fieldSmoothSpinBox->setValue(0);
    fieldSmoothSpinBox->setEnabled(false);
  }

  if ((nc=cp->findKey(vpcopt,"label.size"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e=vpcopt[nc].floatValue[0];
    else e= 1.0;
    i= (int(e*100.+0.5))/5 * 5;
    labelSizeSpinBox->setValue(i);
    labelSizeSpinBox->setEnabled(true);
  } else if (labelSizeSpinBox->isEnabled()) {
    labelSizeSpinBox->setValue(100);
    labelSizeSpinBox->setEnabled(false);
  }

  nc=cp->findKey(vpcopt,"grid.value");
  if (nc>=0) {
    if (vpcopt[nc].allValue=="-1") {
      nc=-1;
    } else {
      bool on= vpcopt[nc].allValue=="1";
      gridValueCheckBox->setChecked( on );
      gridValueCheckBox->setEnabled(true);
    }
  }
  if (nc<0 && gridValueCheckBox->isEnabled()) {
    gridValueCheckBox->setChecked( false );
    gridValueCheckBox->setEnabled( false );
  }

  if ((nc=cp->findKey(vpcopt,"grid.lines"))>=0) {
    if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
    else i=0;
    gridLinesSpinBox->setValue(i);
    gridLinesSpinBox->setEnabled(true);
  } else if (gridLinesSpinBox->isEnabled()) {
    gridLinesSpinBox->setValue(0);
    gridLinesSpinBox->setEnabled(false);
  }

//   if ((nc=cp->findKey(vpcopt,"grid.lines.max"))>=0) {
//     if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
//     else i=0;
//     gridLinesMaxSpinBox->setValue(i);
//     gridLinesMaxSpinBox->setEnabled(true);
//   } else if (gridLinesMaxSpinBox->isEnabled()) {
//     gridLinesMaxSpinBox->setValue(0);
//     gridLinesMaxSpinBox->setEnabled(false);
//   }

  // base
  miString base;
  if (ekv>0. && (nc=cp->findKey(vpcopt,"base"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e=vpcopt[nc].floatValue[0];
    else e=0.0;
    zero1ComboBox->setEnabled(true);
    if(colour2ComboBox->currentItem()>0){
      zero2ComboBox->setEnabled(true);
    }
    base = baseList(zero1ComboBox,e,ekv/2.0);
    if( base.exists() ) cp->replaceValue(vpcopt[nc],base,0);
    base.clear();
    base = baseList(zero2ComboBox,e,ekv/2.0);
    if( base.exists() ) updateFieldOptions("base_2",base);
    miString base_2;
    if (ekv>0. && (nc=cp->findKey(vpcopt,"base_2"))>=0) {
    if (!vpcopt[nc].floatValue.empty()) e=vpcopt[nc].floatValue[0];
    else e=0.0;
      base_2 = baseList(zero2ComboBox,e,ekv_2/2.0);
      if( base_2.exists() ) cp->replaceValue(vpcopt[nc],base_2,0);
    }
  } else if (zero1ComboBox->isEnabled()) {
    zero1ComboBox->clear();
    zero1ComboBox->setEnabled(false);
    zero2ComboBox->clear();
    zero2ComboBox->setEnabled(false);
  }

  // undefined masking
  int iumask= 0;
  if ((nc=cp->findKey(vpcopt,"undef.masking"))>=0) {
    n= undefMasking.size();
    if (!undefMaskingCbox->isEnabled()) {
      const char** cvstr= new const char*[n];
      for (i=0; i<n; i++ )
        cvstr[i]=  undefMasking[i].c_str();
      undefMaskingCbox->insertStrList( cvstr, n );
      delete[] cvstr;
      undefMaskingCbox->setEnabled(true);
    }
    if (vpcopt[nc].intValue.size()==1) {
      iumask= vpcopt[nc].intValue[0];
      if (iumask<0 || iumask>=undefMasking.size()) iumask=0;
    } else {
      iumask= 0;
    }
    undefMaskingCbox->setCurrentItem(iumask);
  } else if (undefMaskingCbox->isEnabled()) {
    undefMaskingCbox->clear();
    undefMaskingCbox->setEnabled(false);
  }

  // undefined masking colour
  if ((nc=cp->findKey(vpcopt,"undef.colour"))>=0) {
    if (undefColourCbox->count()==0) {
      for( i=0; i<nr_colors; i++)
        undefColourCbox->insertItem( *pmapColor[i] );
    }
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      i=0;
      updateFieldOptions("undef.colour",colourInfo[i].name);
    }
    undefColourCbox->setCurrentItem(i);
    undefColourCbox->setEnabled( iumask>0 );
  } else if (undefColourCbox->isEnabled()) {
    undefColourCbox->clear();
    undefColourCbox->setEnabled(false);
  }

  // undefined masking linewidth
  if ((nc=cp->findKey(vpcopt,"undef.linewidth"))>=0) {
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miString(i+1)) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("undef.linewidth",miString(i+1));
    }
    undefLinewidthCbox->setCurrentItem(i);
    undefLinewidthCbox->setEnabled( iumask>1 );
  } else if (undefLinewidthCbox->isEnabled()) {
    undefLinewidthCbox->setEnabled(false);
  }

  // undefined masking linetype
  if ((nc=cp->findKey(vpcopt,"undef.linetype"))>=0) {
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("undef.linetype",linetypes[i]);
    }
    undefLinetypeCbox->setCurrentItem(i);
    undefLinetypeCbox->setEnabled( iumask>1 );
  } else if (undefLinetypeCbox->isEnabled()) {
    undefLinetypeCbox->setEnabled(false);
  }

  nc=cp->findKey(vpcopt,"zero.line");
  if (nc>=0) {
    if (vpcopt[nc].allValue=="-1") {
      nc=-1;
    } else {
      bool on= vpcopt[nc].allValue=="1";
      zeroLineCheckBox->setChecked( on );
      zeroLineCheckBox->setEnabled(true);
    }
  }
  if (nc<0 && zeroLineCheckBox->isEnabled()) {
    zeroLineCheckBox->setChecked( true );
    //    zeroLineCheckBox->setEnabled( false );
    zeroLineCheckBox->setEnabled( true );
  }

  nc=cp->findKey(vpcopt,"value.label");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    valueLabelCheckBox->setChecked( on );
    valueLabelCheckBox->setEnabled(true);
  }
  if (nc<0 && valueLabelCheckBox->isEnabled()) {
    valueLabelCheckBox->setChecked( true );
    valueLabelCheckBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"minvalue");
  if (nc>=0) {
    min1ComboBox->setEnabled(true);
    if(colour2ComboBox->currentItem()>0)
      min2ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.cStr());
    else
      value = atof(vpcopt[nc].allValue.cStr());
    baseList(min1ComboBox,value,ekv,true);
    baseList(min2ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      min1ComboBox->setCurrentItem(0);
    min2ComboBox->setCurrentItem(0);
    nc=cp->findKey(vpcopt,"minvalue_2");
    if (nc>=0) {
      float value;
      if(vpcopt[nc].allValue=="off")
	value=atof(base.cStr());
      else
	value = atof(vpcopt[nc].allValue.cStr());
      baseList(min2ComboBox,value,ekv,true);
      if(vpcopt[nc].allValue=="off")
	min2ComboBox->setCurrentItem(0);
    }
  } else {
    min1ComboBox->setEnabled( false );
    min2ComboBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"maxvalue");
  if (nc>=0) {
    max1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.cStr());
    else
      value = atof(vpcopt[nc].allValue.cStr());
    baseList(max1ComboBox,value,ekv,true);
    baseList(max2ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      max1ComboBox->setCurrentItem(0);
    max2ComboBox->setCurrentItem(0);
    nc=cp->findKey(vpcopt,"maxvalue_2");
    if (nc>=0) {
      float value;
      if(vpcopt[nc].allValue=="off")
	value=atof(base.cStr());
      else
	value = atof(vpcopt[nc].allValue.cStr());
      baseList(max2ComboBox,value,ekv,true);
      if(vpcopt[nc].allValue=="off")
	max2ComboBox->setCurrentItem(0);
    }
  } else {
    max1ComboBox->setEnabled( false );
    max2ComboBox->setEnabled( false );
  }

  if ( enableType2Opt )
    enableType2Options(true);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableFieldOptions returned"<<endl;
#endif
}


void FieldDialog::disableFieldOptions(int type)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::disableFieldOptions called:"<<type<<endl;
#endif

  // show levels for the current field group
  setLevel();

  if (currentFieldOpts.empty()) return;
  currentFieldOpts.clear();

  if(type==0){
    Delete->setEnabled( false );
    deleteAll->setEnabled( false );
    copyField->setEnabled( false );
    changeModelButton->setEnabled( false );
    upFieldButton->setEnabled( false );
    downFieldButton->setEnabled( false );
    resetOptionsButton->setEnabled( false );
    minusButton->setOn(false);
    minusButton->setEnabled( false );
  }

  colorCbox->clear();
  colorCbox->setEnabled( false );
  colour2ComboBox->setCurrentItem(0);
  colour2ComboBox->setEnabled( false );
  shadingComboBox->setCurrentItem(0);
  shadingComboBox->setEnabled( false );
  shadingSpinBox->setValue(0);
  shadingSpinBox->setEnabled( false );
  shadingcoldComboBox->setCurrentItem(0);
  shadingcoldComboBox->setEnabled( false );
  shadingcoldSpinBox->setValue(0);
  shadingcoldSpinBox->setEnabled( false );
  tableCheckBox->setEnabled(false);
  patternComboBox->setEnabled(false);
  patternColourBox->setEnabled(false);
  repeatCheckBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  alphaSpinBox->setEnabled(false);

  //  lineTypeCbox->clear();
  lineTypeCbox->setEnabled( false );
  linetype2ComboBox->setEnabled( false );

  //  lineWidthCbox->clear();
  lineWidthCbox->setEnabled( false );
  linewidth2ComboBox->setEnabled( false );

  lineintervalCbox->clear();
  lineintervalCbox->setEnabled( false );
  interval2ComboBox->clear();
  interval2ComboBox->setEnabled( false );

  densityCbox->clear();
  densityCbox->setEnabled( false );

  vectorunitCbox->clear();
  vectorunitCbox->setEnabled( false );

  extremeTypeCbox->clear();
  extremeTypeCbox->setEnabled( false );

  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled( false );

  extremeRadiusSpinBox->setValue(100);
  extremeRadiusSpinBox->setEnabled( false );

  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled( false );

  fieldSmoothSpinBox->setValue(0);
  fieldSmoothSpinBox->setEnabled( false );

  zeroLineCheckBox->setChecked( true );
  //  zeroLineCheckBox->setEnabled( false );
  zeroLineCheckBox->setEnabled( true );

  valueLabelCheckBox->setChecked( true );
  valueLabelCheckBox->setEnabled( false );

  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled( false );

  gridValueCheckBox->setChecked( false );
  gridValueCheckBox->setEnabled( false );

  gridLinesSpinBox->setValue(0);
  gridLinesSpinBox->setEnabled( false );

//   gridLinesMaxSpinBox->setValue(0);
//   gridLinesMaxSpinBox->setEnabled( false );

  zero1ComboBox->clear();
  zero1ComboBox->setEnabled( false );

  zero2ComboBox->clear();
  zero2ComboBox->setEnabled( false );

  if(type==0){
    hourOffsetSpinBox->setValue(0);
    hourOffsetSpinBox->setEnabled(false);

    hourDiffSpinBox->setValue(0);
    hourDiffSpinBox->setEnabled(false);
  }

  undefMaskingCbox->clear();
  undefMaskingCbox->setEnabled(false);

  undefColourCbox->clear();
  undefColourCbox->setEnabled(false);

  undefLinewidthCbox->setEnabled(false);

  undefLinetypeCbox->setEnabled(false);

  min1ComboBox->setEnabled(false);
  max1ComboBox->setEnabled(false);
  min2ComboBox->setEnabled(false);
  max2ComboBox->setEnabled(false);

  for(int i=0;i<3;i++){
    threeColourBox[i]->setCurrentItem(0);
    //    threeColourBox[i]->setEnabled(false);
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::disableFieldOptions returned"<<endl;
#endif
}


vector<miString> FieldDialog::numberList( QComboBox* cBox, float number )
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::numberList called"<<endl;
#endif

  vector<miString> vnumber;

  const int nenormal = 10;
  const float enormal[nenormal] = { 1., 2., 2.5, 3., 4., 5.,
			            6., 7., 8., 9. };
  float e, elog, ex, d, dd;
  int   i, j, k, n, ielog, nupdown;

  e= number;
  if( e<=0 ) e=1.0;
  elog= log10f(e);
  if (elog>=0.) ielog= int(elog);
  else          ielog= int(elog-0.99999);
  ex = powf(10., ielog);
  n= 0;
  d= fabsf(e - enormal[0]*ex);
  for (i=1; i<nenormal; ++i) {
    dd = fabsf(e - enormal[i]*ex);
    if (d>dd) {
      d=dd;
      n=i;
    }
  }
  nupdown= nenormal*2/3;
  for (i=n-nupdown; i<=n+nupdown; ++i) {
    j= i/nenormal;
    k= i%nenormal;
    if (i<0) j--;
    if (k<0) k+=nenormal;
    ex= powf(10., ielog+j);
    vnumber.push_back(miString(enormal[k]*ex));
  }
  n=1+nupdown*2;
  const char** cvstr= new const char*[n];
  for (i=0; i<n; ++i) cvstr[i]= vnumber[i].c_str();
  cBox->clear();
  cBox->insertStrList(cvstr,n);
  cBox->setCurrentItem(nupdown);
  //  cBox->setEnabled(true);
  delete[] cvstr;

  return vnumber;
}

miString FieldDialog::baseList( QComboBox* cBox,
				float base,
				float ekv,
				bool onoff )
{
  miString str;

  int n;
  if (base<0.) n= int(base/ekv - 0.5);
  else         n= int(base/ekv + 0.5);
  if (fabsf(base-ekv*float(n))>0.01*ekv) {
    base= ekv*float(n);
    str = miString(base);
  }
  n=21;
  int k=n/2;
  int j=-k-1;

  cBox->clear();

  if(onoff)
    cBox->insertItem(tr("Off"));

  for (int i=0; i<n; ++i) {
    j++;
    float e= base + ekv*float(j);
    if(fabs(e)<ekv/2)
    cBox->insertItem("0");
    else{
      miString estr(e);
      cBox->insertItem(estr.cStr());
    }
  }

  if(onoff)
    cBox->setCurrentItem(k+1);
  else
    cBox->setCurrentItem(k);

  return str;
}


void FieldDialog::selectedFieldboxHighlighted( int index )
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::selectedFieldboxHighlighted called:"<<index<<endl;
#endif

  // may get here when there is none selected fields (the last is removed)
  if (index<0 || selectedFields.size()==0) return;

  disableFieldOptions(1);
  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::selectedFieldboxHighlighted returned"<<endl;
#endif
  return;
}


void FieldDialog::colorCboxActivated( int index )
{
  //turn of 3 colours 
  threeColourBox[0]->setCurrentItem(0);  
  threeColourBox[1]->setCurrentItem(0);  
  threeColourBox[2]->setCurrentItem(0);  
  threeColoursChanged();
  
  if (index==0)
    updateFieldOptions("colour","off");
  else
    updateFieldOptions("colour",colourInfo[index-1].name);
}


void FieldDialog::lineWidthCboxActivated( int index )
{
  updateFieldOptions("linewidth",miString(index+1));
}


void FieldDialog::lineTypeCboxActivated( int index )
{
  updateFieldOptions("linetype",linetypes[index]);
}


void FieldDialog::lineintervalCboxActivated( int index )
{
  updateFieldOptions("line.interval",lineintervals[index]);
  // update the list (with selected value in the middle)
  float a= atof(lineintervals[index].c_str());
  lineintervals= numberList( lineintervalCbox, a);
}


void FieldDialog::densityCboxActivated( int index )
{
  if (index==0) updateFieldOptions("density","0");
  else          updateFieldOptions("density",density[index]);
}


void FieldDialog::vectorunitCboxActivated( int index )
{
  updateFieldOptions("vector.unit",vectorunit[index]);
  // update the list (with selected value in the middle)
  float a= atof(vectorunit[index].c_str());
  vectorunit= numberList( vectorunitCbox, a);
}


void FieldDialog::extremeTypeActivated(int index)
{
  updateFieldOptions("extreme.type",extremeType[index]);
}


void FieldDialog::extremeSizeChanged(int value)
{
  miString str= miString( float(value)*0.01 );
  updateFieldOptions("extreme.size",str);
}


void FieldDialog::extremeRadiusChanged(int value)
{
  miString str= miString( float(value)*0.01 );
  updateFieldOptions("extreme.radius",str);
}


void FieldDialog::lineSmoothChanged(int value)
{
  miString str= miString( value );
  updateFieldOptions("line.smooth",str);
}


void FieldDialog::fieldSmoothChanged(int value)
{
  miString str= miString( value );
  updateFieldOptions("field.smooth",str);
}


void FieldDialog::labelSizeChanged(int value)
{
  miString str= miString( float(value)*0.01 );
  updateFieldOptions("label.size",str);
}


void FieldDialog::gridValueCheckBoxToggled(bool on)
{
  if (on) updateFieldOptions("grid.value","1");
  else    updateFieldOptions("grid.value","0");
}


void FieldDialog::gridLinesChanged(int value)
{
  miString str= miString( value );
  updateFieldOptions("grid.lines",str);
}


// void FieldDialog::gridLinesMaxChanged(int value)
// {
//   miString str= miString( value );
//   updateFieldOptions("grid.lines.max",str);
// }


void FieldDialog::baseoptionsActivated( int index )
{
  updateFieldOptions("base",zero1ComboBox->currentText().latin1());
}

void FieldDialog::hourOffsetChanged(int value)
{
  int n= selectedFieldbox->currentItem();
  selectedFields[n].hourOffset= value;
  updateTime();
}

void FieldDialog::hourDiffChanged(int value)
{
  int n= selectedFieldbox->currentItem();
  selectedFields[n].hourDiff= value;
  updateTime();
}


void FieldDialog::undefMaskingActivated(int index)
{
  updateFieldOptions("undef.masking",miString(index));
  undefColourCbox->setEnabled( index>0 );
  undefLinewidthCbox->setEnabled( index>1 );
  undefLinetypeCbox->setEnabled( index>1 );
}


void FieldDialog::undefColourActivated( int index )
{
  updateFieldOptions("undef.colour",colourInfo[index].name);
}


void FieldDialog::undefLinewidthActivated( int index )
{
  updateFieldOptions("undef.linewidth",miString(index+1));
}


void FieldDialog::undefLinetypeActivated( int index )
{
  updateFieldOptions("undef.linetype",linetypes[index]);
}


void FieldDialog::zeroLineCheckBoxToggled(bool on)
{
  if (on) updateFieldOptions("zero.line","1");
  else    updateFieldOptions("zero.line","0");
}


void FieldDialog::valueLabelCheckBoxToggled(bool on)
{
  if (on) updateFieldOptions("value.label","1");
  else    updateFieldOptions("value.label","0");
}

void FieldDialog::colour1ComboBoxToggled(int index)
{
}


void FieldDialog::colour2ComboBoxToggled(int index)
{
  if(index == 0){
    updateFieldOptions("colour_2","off");
    enableType2Options(false);
    colour2ComboBox->setEnabled(true);
  } else {
    updateFieldOptions("colour_2",colourInfo[index-1].name);
    enableType2Options(true); //check if needed
    //turn of 3 colours (not possible to combine threeCols and col_2)
    threeColourBox[0]->setCurrentItem(0);  
    threeColoursChanged();
  }
}


void FieldDialog::tableCheckBoxToggled(bool on)
{
  if (on) updateFieldOptions("table","1");
  else    updateFieldOptions("table","0");
}


void FieldDialog::patternComboBoxToggled(int index)
{
  if(index == 0){
    updateFieldOptions("patterns","off");
  } else {
    updateFieldOptions("patterns",patternInfo[index-1].name);
  }
  updatePaletteString();
}

void FieldDialog::patternColourBoxToggled(int index)
{
  if(index == 0){
    updateFieldOptions("patterncolour","remove");
  } else {
    updateFieldOptions("patterncolour",colourInfo[index-1].name);
  }
  updatePaletteString();
}


void FieldDialog::repeatCheckBoxToggled(bool on)
{
  if (on) updateFieldOptions("repeat","1");
  else    updateFieldOptions("repeat","0");
}

void FieldDialog::threeColoursChanged()
{
  int c1=threeColourBox[0]->currentItem();
  int c2=threeColourBox[1]->currentItem();
  int c3=threeColourBox[2]->currentItem();
  if (c1==0 || c2==0 || c3 ==0){
    //    threeColoursCheckBox->setChecked(false);
    updateFieldOptions("colours","remove");
  } else { 

    //turn of line colour
    colorCbox->setCurrentItem(0);
    updateFieldOptions("colour","off");

    //turn of colour_2 (not possible to combine threeCols and col_2)
    colour2ComboBox->setCurrentItem(0);
    colour2ComboBoxToggled(0);

    miString str = colourInfo[c1-1].name + "," 
      + colourInfo[c2-1].name + "," + colourInfo[c3-1].name;
    updateFieldOptions("colours","remove");
    updateFieldOptions("colours",str);
  }
}

void FieldDialog::shadingChanged()
{
  updatePaletteString();
}

void FieldDialog::updatePaletteString()
{
  if(patternComboBox->currentItem()>0 && patternColourBox->currentItem()>0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }

  int index1 = shadingComboBox->currentItem();
  int index2 = shadingcoldComboBox->currentItem();
  int value1 = shadingSpinBox->value();
  int value2 = shadingcoldSpinBox->value();

  if(index1==0 && index2==0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }

  miString str;
  if(index1>0){
    str = csInfo[index1-1].name;
    if(value1>0)
      str += ";" + miString(value1);
    if(index2>0)
      str += ",";
  }
  if(index2>0){
    str += csInfo[index2-1].name;
    if(value2>0)
      str += ";" + miString(value2);
  }
  updateFieldOptions("palettecolours",str,-1);
}

void FieldDialog::alphaChanged(int index)
{
  updateFieldOptions("alpha",miString(index));
}

void FieldDialog::interval2ComboBoxToggled(int index)
{
  miString str = interval2ComboBox->currentText().latin1();
  updateFieldOptions("line.interval_2",str);
  // update the list (with selected value in the middle)
  float a= atof(str.c_str());
  numberList( interval2ComboBox, a);
}

void FieldDialog::zero1ComboBoxToggled(int index)
{
  if(!zero1ComboBox->currentText().isNull() ){
    miString str = zero1ComboBox->currentText().latin1();
    updateFieldOptions("base",str);
    float a = atof(str.cStr());
    float b = atof(lineintervalCbox->currentText().latin1())/2.0;
    baseList(zero1ComboBox,a,b);
  }
}

void FieldDialog::zero2ComboBoxToggled(int index)
{
  if(!zero2ComboBox->currentText().isNull() ){
    miString str = zero2ComboBox->currentText().latin1();
    updateFieldOptions("base_2",str);
    float a = atof(str.cStr());
    float b = atof(lineintervalCbox->currentText().latin1())/2.0;
    baseList(zero2ComboBox,a,b);
  }
}

void FieldDialog::min1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("minvalue","off");
  else if(!min1ComboBox->currentText().isNull() ){
    miString str = min1ComboBox->currentText().latin1();
    updateFieldOptions("minvalue",str);
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = atof(lineintervalCbox->currentText().latin1());
    baseList(min1ComboBox,a,b,true);
  }
}

void FieldDialog::max1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("maxvalue","off");
  else if(!max1ComboBox->currentText().isNull() ){
    miString str = max1ComboBox->currentText().latin1();
    updateFieldOptions("maxvalue", max1ComboBox->currentText().latin1());
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = atof(lineintervalCbox->currentText().latin1());
    baseList(max1ComboBox,a,b,true);
  }
}

void FieldDialog::min2ComboBoxToggled(int index)
{

  if( index == 0 )
    updateFieldOptions("minvalue_2","remove");
  else if( !min2ComboBox->currentText().isNull() ){
    miString str = min2ComboBox->currentText().latin1();
    updateFieldOptions("minvalue_2",min2ComboBox->currentText().latin1());
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = atof(interval2ComboBox->currentText().latin1());
    baseList(min2ComboBox,a,b,true);
  }
}

void FieldDialog::max2ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("maxvalue_2","remove");
  else if( !max2ComboBox->currentText().isNull() ){
    miString str = max2ComboBox->currentText().latin1();
    updateFieldOptions("maxvalue_2", max2ComboBox->currentText().latin1());
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = atof(interval2ComboBox->currentText().latin1());
    baseList(max2ComboBox,a,b,true);
  }
}

void FieldDialog::linewidth1ComboBoxToggled(int index)
{
  lineWidthCbox->setCurrentItem(index);
  updateFieldOptions("linewidth",miString(index+1));
}

void FieldDialog::linewidth2ComboBoxToggled(int index)
{
  updateFieldOptions("linewidth_2",miString(index+1));
}

void FieldDialog::linetype1ComboBoxToggled(int index)
{
  lineTypeCbox->setCurrentItem(index);
  updateFieldOptions("linetype",linetypes[index]);
}

void FieldDialog::linetype2ComboBoxToggled(int index)
{
  updateFieldOptions("linetype_2",linetypes[index]);
}


void FieldDialog::enableType2Options(bool on)
{
  colour2ComboBox->setEnabled( on );

  //enable the rest only if colour2 is on
  on = (colour2ComboBox->currentItem()!=0);

  interval2ComboBox->setEnabled( on );
  zero2ComboBox->setEnabled( on );
  min2ComboBox->setEnabled( on );
  max2ComboBox->setEnabled( on );
  linewidth2ComboBox->setEnabled( on );
  linetype2ComboBox->setEnabled( on );

  if(on){
    if(!interval2ComboBox->currentText().isNull())
      updateFieldOptions("line.interval_2",
			 interval2ComboBox->currentText().latin1());
    if(!zero2ComboBox->currentText().isNull())
      updateFieldOptions("base_2",
			 zero2ComboBox->currentText().latin1());
    if(!min2ComboBox->currentText().isNull() && min2ComboBox->currentItem()>0)
      updateFieldOptions("minvalue_2",
			 min2ComboBox->currentText().latin1());
    if(!max2ComboBox->currentText().isNull() && max2ComboBox->currentItem()>0)
      updateFieldOptions("maxvalue_2",
			 max2ComboBox->currentText().latin1());
    updateFieldOptions("linewidth_2",
		       miString(linewidth2ComboBox->currentItem()+1));
    updateFieldOptions("linetype_2",
		       linetypes[linetype2ComboBox->currentItem()]);
  } else {
    colour2ComboBox->setCurrentItem(0);
    updateFieldOptions("colour_2","off");
    updateFieldOptions("line.interval_2","remove");
    updateFieldOptions("base_2","remove");
    updateFieldOptions("value.range_2","remove");
    updateFieldOptions("linewidth_2","remove");
    updateFieldOptions("linetype_2","remove");
  }
}

void FieldDialog::updateFieldOptions(const miString& name,
				     const miString& value,
				     int valueIndex)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateFieldOptions  name= " << name
			           << "  value= " << value <<endl;
#endif

  if (currentFieldOpts.empty()) return;

  int n= selectedFieldbox->currentItem();

  if(value == "remove")
    cp->removeValue(vpcopt,name);
  else
    cp->replaceValue(vpcopt,name,value,valueIndex);

  currentFieldOpts= cp->unParse(vpcopt);
  selectedFields[n].fieldOpts= currentFieldOpts;

    // not update private settings if external/QuickMenu command...
  if (!selectedFields[n].external)
    fieldOptions[selectedFields[n].fieldName.downcase()]= currentFieldOpts;
}


void FieldDialog::getFieldGroups(const miString& model,
				 int& indexMGR, int& indexM,
				 vector<FieldGroupInfo>& vfg)
{

  miString modelName;

  QApplication::setOverrideCursor( waitCursor );

  m_ctrl->getFieldGroups(model,modelName,vfg);

  QApplication::restoreOverrideCursor();

  if (indexMGR>=0 && indexM>=0) {
    // field groups will be shown, translate the name parts
    map<miString,miString>::const_iterator pt, ptend=fgTranslations.end();
    size_t pos;
    for (int n=0; n<vfg.size(); n++) {
      for (pt=fgTranslations.begin(); pt!=ptend; pt++) {
	if ((pos=vfg[n].groupName.find(pt->first))!=string::npos)
	  vfg[n].groupName.string::replace(pos,pt->first.size(),pt->second);
      }
    }
  } else {
    int i, n, ng= m_modelgroup.size();
    indexMGR=0;
    indexM= -1;
    while (indexMGR<ng && indexM<0) {
      n= m_modelgroup[indexMGR].modelNames.size();
      i= 0;
      while (i<n && modelName!=m_modelgroup[indexMGR].modelNames[i]) i++;
      if (i<n) indexM= i;
      else     indexMGR++;
    }
    if (indexMGR==ng) {
      indexMGR= -1;
      vfg.clear();
    }
  }
}


vector<miString> FieldDialog::getOKString()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::getOKString called"<<endl;
#endif

  if (historyOkButton->isEnabled()) historyOk();

  vector<miString> vstr;
  if (selectedFields.size()==0) return vstr;

  bool allTimeSteps= allTimeStepButton->isOn();

  vector<miString> hstr;

  levelOKlist.clear();
  levelOKspec.clear();
  idnumOKlist.clear();
  idnumOKspec.clear();

  int n= selectedFields.size();

  for (int i=0; i<n; i++) {

    ostringstream ostr;

    if (selectedFields[i].minus) continue;
    bool minus=false;
    if(i+1<n && selectedFields[i+1].minus){
      minus=true;
      ostr <<"( ";
    }

    if (selectedFields[i].inEdit)
      ostr << editName;
    else
      ostr << selectedFields[i].modelName;

    ostr << " " << selectedFields[i].fieldName;

    if (selectedFields[i].level.exists())
      ostr << " level=" << selectedFields[i].level;
    if (selectedFields[i].idnum.exists())
      ostr << " idnum=" << selectedFields[i].idnum;

    if (levelOKspec.empty() &&
	selectedFields[i].levelOptions.size()>1) {
      levelOKspec= selectedFields[i].level;
      levelOKlist= selectedFields[i].levelOptions;
    }
    if (idnumOKspec.empty() &&
	selectedFields[i].idnumOptions.size()>1) {
      idnumOKspec= selectedFields[i].idnum;
      idnumOKlist= selectedFields[i].idnumOptions;
    }

    if (selectedFields[i].hourOffset!=0)
      ostr << " hour.offset=" << selectedFields[i].hourOffset;

    if (selectedFields[i].hourDiff!=0)
      ostr << " hour.diff=" << selectedFields[i].hourDiff;

    if (minus) {
      ostr << " - ";

      ostr << selectedFields[i+1].modelName;
      ostr << " " << selectedFields[i+1].fieldName;

      if (selectedFields[i+1].level.exists())
	ostr << " level=" << selectedFields[i+1].level;
      if (selectedFields[i+1].idnum.exists())
	ostr << " idnum=" << selectedFields[i+1].idnum;

      if (levelOKspec.empty() &&
	  selectedFields[i+1].levelOptions.size()>1) {
	levelOKspec= selectedFields[i+1].level;
	levelOKlist= selectedFields[i+1].levelOptions;
      }
      if (idnumOKspec.empty() &&
	  selectedFields[i+1].idnumOptions.size()>1) {
	idnumOKspec= selectedFields[i+1].idnum;
	idnumOKlist= selectedFields[i+1].idnumOptions;
      }

      if (selectedFields[i+1].hourOffset!=0)
	ostr << " hour.offset=" << selectedFields[i+1].hourOffset;

      if (selectedFields[i+1].hourDiff!=0)
	ostr << " hour.diff=" << selectedFields[i+1].hourDiff;

      ostr <<" )";
   }

    ostr << " " << selectedFields[i].fieldOpts;

    // for local and global history and for QuickMenues
    if (allTimeSteps)
      ostr << " allTimeSteps=on";

    miString str;

    if (selectedFields[i].inEdit) {

      str= "EDITFIELD " + ostr.str();

    } else {

      str= "FIELD " + ostr.str();

      // the History string (but not if quickmenu command)
      if (!selectedFields[i].external)
	hstr.push_back(ostr.str());

    }

    // the OK string
    vstr.push_back(str);

    //#############################################################
    //cerr << "OK: " << str << endl;
    //#############################################################
  }

  // could check if a previous equal command should be deleted...
  // check if previous command was equal (the easiest...)
  if (hstr.size()>0) {
    bool newcommand= true;
    int hs= commandHistory.size();
    if (hs>0) {
      hs--;
      if (commandHistory[hs].size()==hstr.size()) {
	newcommand= false;
	n= hstr.size();
	for (int i=0; i<n; i++)
	  if (commandHistory[hs][i]!=hstr[i]) newcommand= true;
      }
    }
    if (newcommand) commandHistory.push_back(hstr);
    //###############################################################
    //if (newcommand) cerr << "NEW COMMAND !!!!!!!!!!!!!" << endl;
    //else            cerr << "SAME COMMAND !!!!!!!!!!!!!" << endl;
    //###############################################################
  }
  //###############################################################
  //if (hstr.size()>0) {
  //  cerr << "OK-----------------------------" << endl;
  //  n= hstr.size();
  //  bool eq;
  //  vector< vector<miString> >::iterator p, pend= commandHistory.end();
  //  for (p=commandHistory.begin(); p!=pend; p++) {
  //    if (p->size()==n) {
  //	eq= true;
  //	for (i=0; i<n; i++)
  //	  if (p[i]!=hstr[i]) eq= false;
  //	if (eq) cerr << "FOUND" << endl;
  //  }
  //  }
  //  cerr << "-------------------------------" << endl;
  //}
  //###############################################################

  historyPos= commandHistory.size();

  historyBackButton->setEnabled(true);
  historyForwardButton->setEnabled(false);

  return vstr;
}


miString FieldDialog::getShortname()
{
  // AC: simple version for testing...the shortname could perhaps
  // be made in getOKString?

  miString name;
  int n= selectedFields.size();
  ostringstream ostr;
  miString pmodelName;
  bool fielddiff=false, paramdiff=false, leveldiff=false;

  for (int i=numEditFields; i<n; i++) {
    miString modelName = selectedFields[i].modelName;
    miString fieldName = selectedFields[i].fieldName;
    miString level  = selectedFields[i].level;
    miString idnum     = selectedFields[i].idnum;

    //difference field
    if (i<n-1 && selectedFields[i+1].minus){

      miString modelName2 = selectedFields[i+1].modelName;
      miString fieldName2 = selectedFields[i+1].fieldName;
      miString level_2  = selectedFields[i+1].level;
      miString idnum_2    = selectedFields[i+1].idnum;

      fielddiff = (modelName!=modelName2);
      paramdiff = (fieldName!=fieldName2);
      leveldiff = (!level.exists() || level!=level_2 || idnum!=idnum_2);

      if (modelName!=pmodelName || modelName2!=pmodelName) {

      if (i>numEditFields) ostr << "  ";
	if (fielddiff) ostr << "( ";
	ostr << modelName;
	pmodelName= modelName;
      }

      if(!fielddiff && paramdiff)
	ostr << " ( ";
      if (!fielddiff || (fielddiff && paramdiff) || (fielddiff && leveldiff))
	ostr << " " << fieldName;

      if (selectedFields[i].level.exists()){
	if(!fielddiff && !paramdiff)
	  ostr << " ( "<< selectedFields[i].level;
	else if((fielddiff || paramdiff) && leveldiff)
	  ostr << " "<<selectedFields[i].level;
      }
      if (selectedFields[i].idnum.exists() && leveldiff)
	ostr << " " << selectedFields[i].idnum;

    } else if (selectedFields[i].minus){
      ostr << " - ";

      if (fielddiff) {
	ostr << modelName;
	pmodelName.clear();
      }

      if(fielddiff && !paramdiff && !leveldiff)
	ostr << " ) "<< fieldName;
      else if(paramdiff || (fielddiff && leveldiff))
	ostr << " " << fieldName;

      if (selectedFields[i].level.exists()) {
	if(!leveldiff && paramdiff)
	  ostr << " )";
	ostr << " " << selectedFields[i].level;
	if (selectedFields[i].idnum.exists())
	  ostr << " " << selectedFields[i].idnum;
	if(leveldiff)
	  ostr <<  " )";
      } else {
	  ostr << " )";
      }

      fielddiff=paramdiff=leveldiff=false;

    } else {    // Ordinary field

      if (i>numEditFields) ostr << "  ";

      if (modelName!=pmodelName) {
	ostr << modelName;
	pmodelName= modelName;
      }

      ostr << " " << fieldName;

      if (selectedFields[i].level.exists())
	ostr << " " << selectedFields[i].level;
      if (selectedFields[i].idnum.exists())
	ostr << " " << selectedFields[i].idnum;
    }
  }

  if (n>0)
    name= "<font color=\"#000099\">" + ostr.str() + "</font>";

  return name;
}


void FieldDialog::getOKlevels(vector<miString>& levelList,
			      miString& levelSpec)
{
  levelList.clear();
  for (int i=levelOKlist.size()-1; i>=0; i--)
    levelList.push_back(levelOKlist[i]);
  levelSpec= levelOKspec;
}


void FieldDialog::getOKidnums(vector<miString>& idnumList,
			      miString& idnumSpec)
{
  idnumList.clear();
  for (int i=0; i<idnumOKlist.size(); i++)
    idnumList.push_back(idnumOKlist[i]);
  idnumSpec= idnumOKspec;
}


void FieldDialog::historyBack() {
  showHistory(-1);
}


void FieldDialog::historyForward() {
  showHistory(1);
}


void FieldDialog::showHistory(int step) {

  int hs= commandHistory.size();
  if (hs==0) {
    historyPos= -1;
    historyBackButton->setEnabled(false);
    historyForwardButton->setEnabled(false);
    highlightButton(historyOkButton,false);
    return;
  }

  historyPos+=step;
  if (historyPos<-1) historyPos=-1;
  if (historyPos>hs) historyPos=hs;

  if (historyPos<0 || historyPos>=hs) {

    // enable model/field boxes and show edit fields
    deleteAllSelected();

  } else {

    if (!historyOkButton->isEnabled()) {
      deleteAllSelected(); // some cleanup before browsing history
      if (numEditFields>0) disableFieldOptions();
      fieldGRbox->setEnabled(false);
      fieldbox->setEnabled(false);
      selectedFieldbox->setEnabled(false);
    }

    // not show edit fields during history browsing
    selectedFieldbox->clear();

    bool minus = false;
    miString history_str;
    vector<miString> vstr;
    int n= commandHistory[historyPos].size();

    for (int i=0; i<n; i++) {

      if (history_str.empty())
	history_str = commandHistory[historyPos][i];

      //if (field1 - field2)
      miString field1,field2;
      if (fieldDifference(history_str,field1,field2))
	history_str= field1;

      vector<ParsedCommand> vpc= cp->parse( history_str );

/*******************************************************
  int mmm= (vpc.size()<10) ? vpc.size() : 10;
  cerr<<"--------------------------------"<<endl;
    for (int j=0; j<mmm; j++) {
      cerr << "  parse " << j << " : key= " << vpc[j].key
	   << "  idNumber= " << vpc[j].idNumber << endl;
      cerr << "            allValue: " << vpc[j].allValue << endl;
      for (int k=0; k<vpc[j].strValue.size(); k++)
        cerr << "               " << k << "    strValue: " << vpc[j].strValue[k] << endl;
      for (int k=0; k<vpc[j].floatValue.size(); k++)
        cerr << "               " << k << "  floatValue: " << vpc[j].floatValue[k] << endl;
      for (int k=0; k<vpc[j].intValue.size(); k++)
        cerr << "               " << k << "    intValue: " << vpc[j].intValue[k] << endl;
    }
*******************************************************/
      miString str;
      if (minus) str= "  -  ";

      if (vpc.size()>1 && vpc[0].key=="unknown") {
	str+= vpc[0].allValue;  // modelName
	if (vpc[1].key=="unknown") {
	  str+=(" " + vpc[1].allValue); // fieldName
	  int nc;
          if ((nc=cp->findKey(vpc,"level"))>=0)
	    str+=(" " + vpc[nc].allValue);
          if ((nc=cp->findKey(vpc,"idnum"))>=0)
	    str+=(" " + vpc[nc].allValue);
	  vstr.push_back(str);
	}
      }

      //reset
      minus = false;
      history_str.clear();

      //if ( field1 - field2 )
      if (field2.exists()){
	minus=true;
	history_str = field2;
	i--;
      }
    }

    int nvstr= vstr.size();
    if (nvstr>0) {
      const char** cvstr = new const char*[nvstr];
      for (int i=0; i<nvstr; i++)
	cvstr[i]= vstr[i].c_str();
      selectedFieldbox->insertStrList(cvstr,nvstr);
      deleteAll->setEnabled(true);
      delete[] cvstr;
    }

    highlightButton(historyOkButton,true);
  }

  historyBackButton->setEnabled(historyPos>-1);
  historyForwardButton->setEnabled(historyPos<hs);
}


void FieldDialog::historyOk() {
#ifdef DEBUGPRINT
  cerr << "FieldDialog::historyOk()" << endl;
#endif

  if (historyPos<0 || historyPos>=commandHistory.size()) {
    vector<miString> vstr;
    putOKString(vstr,false,false);
  } else {
    putOKString(commandHistory[historyPos],false,false);
  }
}


void FieldDialog::putOKString(const vector<miString>& vstr,
			      bool checkOptions, bool external)
{
#ifdef DEBUGPRINT
  cerr << "FieldDialog::putOKString starts" << endl;
#endif

  deleteAllSelected();

  bool allTimeSteps= false;

  int nc= vstr.size();

  if (nc==0) {
    updateTime();
    return;
  }

  miString vfg2_model,model,field,level,idnum,fOpts;
  int hourOffset,hourDiff;
  int indexMGR,indexM,indexFGR,indexF;
  bool forecastSpec;
  vector<FieldGroupInfo> vfg2;
  int nvfg= 0;
  vector<ParsedCommand> vpc;
  bool minus=false;
  miString str;

  for (int ic=0; ic<nc; ic++) {
    if (str.empty())
      str=vstr[ic];
//######################################################################
//    cerr << "P.OK>> " << vstr[ic] << endl;
//######################################################################

    //if prefix, remove it
    if (str.length()>6 && str.substr(0,6)=="FIELD ")
      str= str.substr(6,str.size()-6);

    //if (field1 - field2)
    miString field1,field2;
    if (fieldDifference(str,field1,field2))
      str= field1;

    if (checkOptions) {
      str= checkFieldOptions(str);
      if (str.empty()) continue;
    }

    vpc= cp->parse( str );

    model.clear();
    field.clear();
    level.clear();
    idnum.clear();
    fOpts.clear();
    hourOffset= 0;
    hourDiff= 0;
    forecastSpec= false;

//######################################################################
//  for (int j=0; j<vpc.size(); j++)
//    cerr << "   " << j << " : " << vpc[j].key << " = " << vpc[j].strValue[0]
//         << "   " << vpc[j].allValue << endl;
//######################################################################

    if (vpc.size()>1 && vpc[0].key=="unknown") {
      model= vpc[0].allValue;  // modelName
      if (vpc[1].key=="unknown") {
	field= vpc[1].allValue; // fieldName
	for (int j=2; j<vpc.size(); j++) {
	  if (vpc[j].key=="level")
	    level= vpc[j].allValue;
	  else if (vpc[j].key=="idnum")
	    idnum= vpc[j].allValue;
	  else if (vpc[j].key=="hour.offset" && !vpc[j].intValue.empty())
	    hourOffset= vpc[j].intValue[0];
	  else if (vpc[j].key=="hour.diff"   && !vpc[j].intValue.empty())
	    hourDiff= vpc[j].intValue[0];
	  else if (vpc[j].key=="allTimeSteps" && vpc[j].allValue=="on")
	    allTimeSteps= true;
	  else if (vpc[j].key!="unknown") {
	    if (!fOpts.empty()) fOpts+=" ";
	    fOpts+= (vpc[j].key + "=" + vpc[j].allValue);
	    if (vpc[j].idNumber==2) forecastSpec= true;
	  }
 	}
      }
    }

//######################################################################
//    cerr << " ->" << model << " " << field
//    	   << " ln= " << levelName << " l2n= " << idnumName
//    	   << " l= " << level << " l2= " << idnum
//         << " " << fGroupName << endl;
//######################################################################

    if (model!=vfg2_model) {
      indexMGR=indexM=-1;
      getFieldGroups(model,indexMGR,indexM,vfg2);
      vfg2_model= model;
      nvfg= vfg2.size();
    }

    indexF=   -1;
    indexFGR= -1;
    int j= 0;
    bool ok= false;

    while (!ok && j<nvfg) {

      // Old syntax: Model, new syntax: Model(gridnr) 
      miString modelName=vfg2[j].modelName;
      if(vfg2[j].modelName.contains("(") && !model.contains("(")){
	modelName = modelName.substr(0,modelName.find(("("))); 
      }

      if (modelName.downcase()==model.downcase()) {

        int m= vfg2[j].fieldNames.size();
        int i= 0;
        while (i<m && vfg2[j].fieldNames[i]!=field) i++;

        if (i<m) {
	  ok= true;
	  int m;
          if ((m=vfg2[j].levelNames.size())>0 && !level.empty()) {
            int l= 0;
            while (l<m && vfg2[j].levelNames[l]!=level) l++;
	    if (l==m && cp->isInt(level)) {
	      level+="hPa";
              l= 0;
              while (l<m && vfg2[j].levelNames[l]!=level) l++;
	      if (l<m) level= vfg2[j].levelNames[l];
	    }
            if (l==m) ok= false;
          } else if (!vfg2[j].levelNames.empty()) {
            ok= false;
          } else {
            level.clear();
          }
          if ((m=vfg2[j].idnumNames.size())>0 && !idnum.empty()) {
            int l= 0;
            while (l<m && vfg2[j].idnumNames[l]!=idnum) l++;
            if (l==m) ok= false;
          } else if (!vfg2[j].idnumNames.empty()) {
            ok= false;
          } else {
            idnum.clear();
          }
	  if (ok) {
            indexFGR= j;
	    indexF= i;
          }
	}
      }
      j++;
    }

    if (indexFGR>=0 && indexF>=0) {
      SelectedField sf;
      sf.inEdit=        false;
      sf.external=      external;     // from QuickMenu
      sf.forecastSpec=  forecastSpec; // only if external
      sf.indexMGR=      indexMGR;
      sf.indexM=        indexM;
      sf.modelName=     vfg2[indexFGR].modelName;
      sf.fieldName=     vfg2[indexFGR].fieldNames[indexF];
      sf.levelOptions=  vfg2[indexFGR].levelNames;
      sf.idnumOptions=  vfg2[indexFGR].idnumNames;
      sf.level=         level;
      sf.idnum=         idnum;
      sf.hourOffset=    hourOffset;
      sf.hourDiff=      hourDiff;
      sf.fieldOpts=     fOpts;
      sf.minus=         false;

      selectedFields.push_back(sf);

      miString text= sf.modelName + " " + sf.fieldName;
      selectedFieldbox->insertItem(QString(text.c_str()));

      selectedFieldbox->setCurrentItem(selectedFieldbox->count()-1);
      selectedFieldbox->setSelected(selectedFieldbox->count()-1,true);

      //############################################################################
      //cerr << "  ok: " << str << " " << fOpts << endl;
      //############################################################################
    }
    //############################################################################
    //else cerr << "  error" << endl;
    //############################################################################

    if (minus) {
      minus=false;
      minusField(true);
    }

    if (!field2.empty()) {
      str = field2;
      ic--;
      minus=true;
    } else {
      str.clear();
    }

  }

  int m= selectedFields.size();
  int ml;

  if (m>0) {
    if (vfgi.size()>0) {
      indexMGR = indexMGRtable[modelGRbox->currentItem()];
      indexM   = modelbox->currentItem();
      indexFGR = fieldGRbox->currentItem();
      int n=     vfgi[indexFGR].fieldNames.size();
      bool change= false;
      for (int i=0; i<m; i++) {
        if (selectedFields[i].indexMGR==indexMGR &&
            selectedFields[i].indexM  ==indexM) {
          int j=0;
          while (j<n &&
		 vfgi[indexFGR].fieldNames[j]!=
		 selectedFields[i].fieldName) j++;
          if (j<n) {
            if ((ml=vfgi[indexFGR].levelNames.size())>0) {
              int l= 0;
              while (l<ml && vfgi[indexFGR].levelNames[l]!=selectedFields[i].level) l++;
              if (l==ml) j= n;
            }
            if ((ml=vfgi[indexFGR].idnumNames.size())>0) {
              int l= 0;
              while (l<ml && vfgi[indexFGR].idnumNames[l]!=selectedFields[i].idnum) l++;
              if (l==ml) j= n;
            }
            if (j<n) {
              countSelected[j]++;
              fieldbox->setSelected(j, true);
	      change= true;
            }
          }
        }
      }
      if (change) fieldboxChanged();
    }
    selectedFieldbox->setCurrentItem(0);
    selectedFieldbox->setSelected(0,true);
    enableFieldOptions();
  }

  if (m>0 && allTimeSteps!=allTimeStepButton->isOn()) {
    allTimeStepButton->setOn(allTimeSteps);
    allTimeStepToggled(allTimeSteps);
  } else {
    updateTime();
  }

#ifdef DEBUGPRINT
  cerr << "FieldDialog::putOKString finished" << endl;
#endif
}


void FieldDialog::requestQuickUpdate(const vector<miString>& oldstr,
                                     vector<miString>& newstr)
{
#ifdef DEBUGPRINT
  cerr << "FieldDialog::requestQuickUpdate" << endl;
#endif

  if (oldstr.size()==0 || oldstr.size()!=newstr.size()) {
    newstr= oldstr;
    return;
  }

  miString field;

  bool ok= true;
  int nc= oldstr.size();
  vector< vector<ParsedCommand> > vpold;

  // it is allowd to change the field sequence, unfortunately...
  for (int ic=0; ic<nc; ic++)
    vpold.push_back( cp->parse(oldstr[ic]) );

  for (int ic=0; ic<nc; ic++)
    if (vpold[ic].size()<3) ok= false;

  if (!ok) {
    newstr= oldstr;
    return;
  }

  // not allowed to change hour.offset or field.smooth
  vector<miString> optkeep;
  optkeep.push_back("hour.offset");
  optkeep.push_back("field.smooth");

  vector<bool> used(nc,false);
  vector<miString> updstr;

  for (int ic=0; ic<nc; ic++) {

    vector<ParsedCommand> vpnew= cp->parse( newstr[ic] );
    int m= vpnew.size();

    if (m<3 || vpnew[2].key!="unknown") {
      ok= false;
      break;
    }

    field= vpnew[2].allValue;

    miString fopts= getFieldOptions(field, true);

    if (fopts.empty()) {
      ok= false;
      break;
    }

    vector<ParsedCommand> vpopt= cp->parse( fopts );

    int iold= 0;
    if (nc>1) {
      // find the old best matching the new
      iold= ic;
      float comp, compbest=-1, compmax=0;
      for (int jc=0; jc<nc; jc++) {
        if (!used[jc]) {
	  // check model
	  int n= vpold[jc][1].allValue.find('@');
	  if (n==string::npos) n= vpold[jc][1].allValue.length();
	  int nm= 0;
	  for (int i=0; i<n; i++)
	    if (vpold[jc][1].allValue[i]==vpnew[1].allValue[i]) nm++;
	  comp= float(nm)/float(n);
	  compmax+=1;
	  // check field
	  n= vpold[jc][2].allValue.find('@');
	  if (n==string::npos) n= vpold[jc][2].allValue.length();
	  nm= 0;
	  for (int i=0; i<n; i++)
	    if (vpold[jc][2].allValue[i]==vpnew[2].allValue[i]) nm++;
	  comp= float(nm)/float(n);
	  compmax+=1;
	  // check level
	  int nold= vpold[jc].size();
	  for (int i=3; i<nold; i++) {
	    if (vpold[jc][i].key=="level"      ||
		vpold[jc][i].key=="idnum"     ||
                vpold[jc][i].key=="hour.offset") {
	      for (int j=3; j<m; j++)
	        if (vpnew[j].key==vpold[jc][i].key &&
                    vpnew[j].allValue==vpold[jc][i].allValue) comp+=1;
	      compmax+=1;
	    }
	  }
	  comp/=compmax;
	  if (comp>compbest) {
	    compbest= comp;
	    iold= jc;
	  }
	}
      }
    }

    // not allowed to change hour.offset or field.smooth
    for (int j=0; j<optkeep.size(); j++) {
      int i1= cp->findKey(vpnew,optkeep[j]);
      int i2= cp->findKey(vpold[iold],optkeep[j]);
      int i3= cp->findKey(vpopt,optkeep[j]);
      if (i1>=0) {
	if (i2>=0 && vpold[iold][i2].allValue != vpnew[i1].allValue)
	  cp->replaceValue(vpnew[i1],vpold[iold][i2].allValue,-1);
	else if (i3>=0 && vpopt[i3].allValue != vpnew[i1].allValue)
	  cp->replaceValue(vpnew[i1],vpopt[i3].allValue,-1);
      }
    }

    used[iold]= true;
    int nold= vpold[iold].size();

    vector<bool> newset(m,false);
    miString str;
    str= vpold[iold][0].allValue + " " +
         vpold[iold][1].allValue + " " +
         vpold[iold][2].allValue;

    for (int j=3; j<nold; j++) {
      int i= cp->findKey(vpnew,vpold[iold][j].key);
      if (vpold[iold][j].idNumber!=0 ||
          vpold[iold][j].allValue.find('@')!=string::npos || i<0) {
	str+=(" " + vpold[iold][j].key + "=" + vpold[iold][j].allValue);
	if (i>=0) newset[i]= true;
      } else {
	str+=(" " + vpnew[i].key + "=" + vpnew[i].allValue);
	newset[i]= true;
      }
    }

    for (int i=3; i<m; i++) {
      if (!newset[i]) {
	// not adding options equal to the defaults (in diana.setup)
        int j= cp->findKey(vpopt,vpnew[i].key);
	if (j>=0 && vpnew[i].allValue!=vpopt[j].allValue)
	  str+=(" " + vpnew[i].key + "=" + vpnew[i].allValue);
      }
    }

    updstr.push_back(str);
  }

  if (ok)
    newstr= updstr;
  else
    newstr= oldstr;

}


bool FieldDialog::fieldDifference(const miString& str,
				  miString& field1, miString& field2) const
{
  size_t beginOper = str.find("( ");
  if (beginOper!=string::npos) {
    size_t oper = str.find(" - ",beginOper+3);
    if (oper!=string::npos) {
      size_t endOper = str.find(" )",oper+4);
      if (endOper!=string::npos) {
	size_t end = str.size();
	if (beginOper>1 && endOper<end-2) {
	  field1 = str.substr(0,beginOper)
		 + str.substr(beginOper+2,oper-beginOper-2)
	         + str.substr(  endOper+2, end-endOper-1);
	  field2 = str.substr(0,beginOper-1)
		 + str.substr(oper+2, endOper-oper-2);
	} else if (endOper<end-2) {
	  field1 = str.substr(beginOper+2,oper-beginOper-2)
	         + str.substr(  endOper+2, end-endOper-1);
	  field2 = str.substr(oper+3, endOper-oper-3);
	} else {
	  field1 = str.substr(0,beginOper)
		 + str.substr(beginOper+2,oper-beginOper-2);
	  field2 = str.substr(0,beginOper)
		 + str.substr(oper+3, endOper-oper-3);
	}
	return true;
      }
    }
  }
  return false;
}


vector<miString> FieldDialog::writeLog() {

  vector<miString> vstr;

  // write history

  int i,n,h, hf=0, hs= commandHistory.size();

  // avoid eternal history
  if (hs>100) hf=hs-100;

  for (h=hf; h<hs; h++) {
    n= commandHistory[h].size();
    for (i=0; i<n; i++)
      vstr.push_back(commandHistory[h][i]);
    vstr.push_back("----------------");
  }
  vstr.push_back("================");

  // write used field options

  map<miString,miString>::iterator pfopt, pfend= fieldOptions.end();

  for (pfopt=fieldOptions.begin(); pfopt!=pfend; pfopt++) {
    miString sopts= getFieldOptions(pfopt->first, true);
    // only logging options if different from setup
    if (sopts != pfopt->second)
      vstr.push_back( pfopt->first + " " + pfopt->second );
  }
  vstr.push_back("================");

  return vstr;
}


void FieldDialog::readLog(const vector<miString>& vstr,
			  const miString& thisVersion,
			  const miString& logVersion) {

  miString str,fieldname,fopts,sopts;
  vector<miString> hstr;
  size_t pos,end;

  map<miString,miString>::iterator pfopt, pfend= fieldOptions.end();
  int nopt,nlog;
  bool changed;

  int nvstr= vstr.size();
  int ivstr= 0;

  // history of commands,
  // many checks in case program and/or diana.setup has changed

  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    if (vstr[ivstr].substr(0,4)=="----") {
      if (hstr.size()>0) {
        commandHistory.push_back(hstr);
	hstr.clear();
      }
    } else {
      hstr.push_back(vstr[ivstr]);
    }
  }
  ivstr++;

  // field options:
  // do not destroy any new options in the program

  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    str= vstr[ivstr];
    end= str.length();
    pos= str.find_first_of(' ');
    if (pos>0 && pos<end) {
      fieldname=str.substr(0,pos);
      pos++;
      fopts= str.substr(pos);

      // get options from setup
      sopts= getFieldOptions(fieldname,true);

      if (!sopts.empty()) {
	// update options from setup, if necessary
        vector<ParsedCommand> vpopt= cp->parse( sopts );
        vector<ParsedCommand> vplog= cp->parse( fopts );
	nopt= vpopt.size();
	nlog= vplog.size();
	changed= false;
	for (int i=0; i<nopt; i++) {
	  int j=0;
	  while (j<nlog && vplog[j].key!=vpopt[i].key) j++;
	  if (j<nlog) {
	    if (vplog[j].allValue!=vpopt[i].allValue){
	      cp->replaceValue(vpopt[i],vplog[j].allValue,-1);
	      changed= true;
	    }
	  }
	}
	for (int i=0; i<nlog; i++) {
	  int j=0;
	  while (j<nopt && vpopt[j].key!=vplog[i].key) j++;
	  if (j==nopt) {
	    cp->replaceValue(vpopt,vplog[i].key,vplog[i].allValue);
	    changed= true;
	  }
	}
        if (changed) fieldOptions[fieldname.downcase()]= cp->unParse(vpopt);
      }
    }
  }
  ivstr++;

  historyPos= commandHistory.size();

  historyBackButton->setEnabled(historyPos>0);
  historyForwardButton->setEnabled(false);
}


miString FieldDialog::checkFieldOptions(const miString& str)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::checkFieldOptions"<<endl;
#endif

  miString newstr;

  miString fieldname;

  vector<ParsedCommand> vplog= cp->parse( str );
  int nlog= vplog.size();

  if (nlog>=2 && vplog[0].key=="unknown"
	      && vplog[1].key=="unknown") {
    fieldname= vplog[1].allValue;

    miString fopts= getFieldOptions(fieldname, true);

    if (!fopts.empty()) {
      vector<ParsedCommand> vpopt= cp->parse( fopts );
      int nopt= vpopt.size();
//##################################################################
//    cerr << "    nopt= " << nopt << "  nlog= " << nlog << endl;
//    for (int j=0; j<nlog; j++)
//	cerr << "        log " << j << " : id " << vplog[j].idNumber
//	     << "  " << vplog[j].key << " = " << vplog[j].allValue << endl;
//##################################################################
      newstr+= vplog[0].allValue + " " + vplog[1].allValue;
      for (int i=2; i<nlog; i++) {
	if (vplog[i].idNumber==1)
	  newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
      for (int j=0; j<nopt; j++) {
	int i=0;
	while (i<nlog && vplog[i].key!=vpopt[j].key) i++;
	if (i<nlog) {
	  // there is no option with variable no. of values, YET !!!!!
	  if (vplog[i].allValue!=vpopt[j].allValue)
	    cp->replaceValue(vpopt[j],vplog[i].allValue,-1);
	}
      }

      for (int i=2; i<nlog; i++) {
	if (vplog[i].key!="level" && vplog[i].key!="idnum") {
	  int j=0;
	  while (j<nopt && vpopt[j].key!=vplog[i].key) j++;
	  if (j==nopt) {
	    cp->replaceValue(vpopt,vplog[i].key,vplog[i].allValue);
	  }
	}
      }

      newstr+= " ";
      newstr+= cp->unParse(vpopt);
      // from quickmenu, keep "forecast.hour=..." and "forecast.hour.loop=..."
      for (int i=2; i<nlog; i++) {
	if (vplog[i].idNumber==2 || vplog[i].idNumber==3 || vplog[i].idNumber==-1)
	  newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
    }
  }

  return newstr;
}


void FieldDialog::deleteSelected()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::deleteSelected called"<<endl;
#endif

  int index = selectedFieldbox->currentItem();

  int ns= selectedFields.size() - 1;

  if (index<0 || index>ns) return;
  if (selectedFields[index].inEdit) return;

  int indexF= -1;
  int ml;

  if (vfgi.size()>0) {
    int indexMGR = indexMGRtable[modelGRbox->currentItem()];
    int indexM   = modelbox->currentItem();
    int indexFGR = fieldGRbox->currentItem();
    if (selectedFields[index].indexMGR==indexMGR &&
        selectedFields[index].indexM  ==indexM) {
      int n= vfgi[indexFGR].fieldNames.size();
      int i=0;
      while (i<n && vfgi[indexFGR].fieldNames[i]!=
		    selectedFields[index].fieldName) i++;
      if (i<n) {
        if ((ml=vfgi[indexFGR].levelNames.size())>0) {
          int l= 0;
          while (l<ml && vfgi[indexFGR].levelNames[l]!=selectedFields[index].level) l++;
          if (l==ml) i= n;
        }
        if ((ml=vfgi[indexFGR].idnumNames.size())>0) {
          int l= 0;
          while (l<ml && vfgi[indexFGR].idnumNames[l]!=selectedFields[index].idnum) l++;
          if (l==ml) i= n;
	}
        if (i<n) indexF= i;
      }
    }
  }

  if (indexF>=0) {
    fieldbox->setCurrentItem(indexF);
    fieldbox->setSelected(indexF, false);
    fieldboxChanged();
  } else {
    selectedFieldbox->removeItem(index);
    for (int i=index; i<ns; i++)
      selectedFields[i]= selectedFields[i+1];
    selectedFields.pop_back();
  }

  if (selectedFields.size()>0) {
    if (index>=selectedFields.size()) index= selectedFields.size()-1;
    selectedFieldbox->setCurrentItem( index );
    selectedFieldbox->setSelected( index, true );
    enableFieldOptions();
  } else {
    disableFieldOptions();
  }

  updateTime();

  //first field can't be minus
  if (selectedFieldbox->count()>0 && selectedFields[0].minus)
    minusButton->setOn(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::deleteSelected returned"<<endl;
#endif
  return;
}


void FieldDialog::deleteAllSelected()
{
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::deleteAllSelected() called"<<endl;
#endif

  // calls:
  //  1: button clicked (while selecting fields)
  //  2: button clicked when history is shown (get out of history)
  //  3: from putOKString when accepting history command
  //  4: from putOKString when inserting QuickMenu command
  //     (possibly when dialog in "history" state...)
  //  5: variable useArchive changed

  int n= fieldbox->count();
  for (int i=0; i<n; i++)
    countSelected[i]= 0;

  if (n>0) {
    fieldbox->blockSignals(true);
    fieldbox->clearSelection();
    fieldbox->blockSignals(false);
  }

  selectedFields.resize(numEditFields);
  selectedFieldbox->clear();
  minusButton->setOn(false);
  minusButton->setEnabled(false);

  if (historyOkButton->isEnabled()) {
    // terminate browsing history
    if (fieldGRbox->count()>0) fieldGRbox->setEnabled(true);
    if (fieldbox->count()>0) fieldbox->setEnabled(true);
    selectedFieldbox->setEnabled(true);
    highlightButton(historyOkButton,false);
  }

  if (numEditFields>0) {
    // show edit fields
    for (int i=0; i<numEditFields; i++) {
      miString str= editName + " " + selectedFields[i].fieldName;
      selectedFieldbox->insertItem(QString(str.c_str()));
    }
    selectedFieldbox->setCurrentItem(0);
    selectedFieldbox->setSelected(0, true);
    enableFieldOptions();
  } else {
    disableFieldOptions();
  }

  updateTime();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::deleteAllSelected() returned"<<endl;
#endif
  return;
}


void FieldDialog::copySelectedField(){
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::copySelectedField called"<<endl;
#endif

  if (selectedFieldbox->count()==0) return;

  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentItem();

  if (vfgi.size()>0) {
    int indexMGR= indexMGRtable[modelGRbox->currentItem()];
    int indexM  = modelbox->currentItem();
    int indexFGR= fieldGRbox->currentItem();
    if (selectedFields[index].indexMGR ==indexMGR &&
        selectedFields[index].indexM   ==indexM) {
      int n= vfgi[indexFGR].fieldNames.size();
      int i= 0;
      while (i<n &&
	     vfgi[indexFGR].fieldNames[i]!=
	     selectedFields[index].fieldName) i++;
      if (i<n) {
        int ml= vfgi[indexFGR].levelNames.size();
        if (ml>0 && selectedFields[index].level.exists()) {
          int l= 0;
          while (l<ml && vfgi[indexFGR].levelNames[l]!=selectedFields[index].level) l++;
          if (l==ml) i= n;
        } else if (ml>0 || selectedFields[index].level.exists()) {
	  i= n;
	}
        ml= vfgi[indexFGR].idnumNames.size();
        if (ml>0 && selectedFields[index].idnum.exists()) {
          int l= 0;
          while (l<ml && vfgi[indexFGR].idnumNames[l]!=selectedFields[index].idnum) l++;
          if (l==ml) i= n;
        } else if (ml>0 || selectedFields[index].idnum.exists()) {
	  i= n;
        }
	if (i<n) countSelected[i]++;
      }
    }
  }

  selectedFields.push_back(selectedFields[index]);
  selectedFields[n].hourOffset= 0;

  selectedFieldbox->insertItem(selectedFieldbox->text(index));
  selectedFieldbox->setCurrentItem(n);
  selectedFieldbox->setSelected( n, true );
  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::copySelectedField returned"<<endl;
#endif
  return;
}


void FieldDialog::changeModel()
{
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::changeModel called"<<endl;
#endif

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentItem();
  if (index<0 || index>=n) return;

  if (modelGRbox->count()==0 || modelbox->count()==0) return;

  int indexMGR= modelGRbox->currentItem();
  int indexM=   modelbox->currentItem();
  if (indexMGR<0 || indexM<0) return;
  indexMGR= indexMGRtable[indexMGR];

  int indexFGR = fieldGRbox->currentItem();
  if (indexFGR<0 || indexFGR>=vfgi.size()) return;

  miString oldModel= selectedFields[index].modelName.downcase();
  miString newModel= vfgi[indexFGR].modelName.downcase();
  if (oldModel==newModel) return;

  fieldbox->blockSignals(true);

  int nvfgi= vfgi.size();
  int gbest,fbest,gnear,fnear;

  for (int i=0; i<n; i++) {
    if (selectedFields[i].modelName.downcase()==oldModel) {
      // check if field exists for the new model
      gbest=fbest=gnear=fnear= -1;
      int j= 0;
      while (gbest<0 && j<nvfgi) {
        if (vfgi[j].modelName.downcase()==newModel) {
 	  int m= vfgi[j].fieldNames.size();
	  int k= 0;
	  while (k<m && vfgi[j].fieldNames[k]!=
		         selectedFields[i].fieldName) k++;
	  if (k<m) {
            int ml= vfgi[j].levelNames.size();
            if (ml>0 && !selectedFields[i].level.empty()) {
              int l= 0;
              while (l<ml && vfgi[j].levelNames[l]!=selectedFields[i].level) l++;
              if (l==ml) k= m;
            } else if (ml>0 || !selectedFields[i].level.empty()) {
	      k= m;
	    }
            ml= vfgi[j].idnumNames.size();
            if (ml>0 && !selectedFields[i].idnum.empty()) {
              int l= 0;
              while (l<ml && vfgi[j].idnumNames[l]!=selectedFields[i].idnum) l++;
              if (l==ml) k= m;
            } else if (ml>0 || !selectedFields[i].idnum.empty()) {
	      k= m;
            }
	    if (k<m) {
	      gbest= j;
	      fbest= k;
            } else if (gnear<0) {
	      gnear= j;
	      fnear= k;
	    }
	  }
        }
	j++;
      }
      if (gbest>=0 || gnear>=0) {
	if (gbest<0) {
	  gbest= gnear;
	  fbest= fnear;
	}
	if (indexFGR==gbest) {
	  countSelected[fbest]++;
	  if (countSelected[fbest]==1) {
	    fieldbox->setCurrentItem( fbest );
	    fieldbox->setSelected( fbest, true );
	  }
	}
        selectedFields[i].indexMGR = indexMGR;
        selectedFields[i].indexM   = indexM;
	selectedFields[i].modelName= vfgi[gbest].modelName;
	selectedFields[i].levelOptions= vfgi[gbest].levelNames;
	selectedFields[i].idnumOptions= vfgi[gbest].idnumNames;

	miString str= selectedFields[i].modelName + " " +
		      selectedFields[i].fieldName;
        selectedFieldbox->changeItem(QString(str.c_str()),i);
      }
    }
  }

  fieldbox->blockSignals(false);

  selectedFieldbox->setCurrentItem( index );
  selectedFieldbox->setSelected( index, true );
  enableFieldOptions();

  updateTime();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::changeModel returned"<<endl;
#endif
  return;
}


void FieldDialog::upField() {

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentItem();
  if (index<1 || index>=n) return;

  SelectedField sf= selectedFields[index];
  selectedFields[index]= selectedFields[index-1];
  selectedFields[index-1]= sf;

  QString qstr1= selectedFieldbox->text(index-1);
  QString qstr2= selectedFieldbox->text(index);
  selectedFieldbox->changeItem(qstr2,index-1);
  selectedFieldbox->changeItem(qstr1,index);

  //some fields can't be minus
  for(int i=0;i<n;i++){
    selectedFieldbox->setCurrentItem( i );
    if(selectedFields[i].minus && (i==0 || selectedFields[i-1].minus))
      minusButton->setOn(false);
  }

  selectedFieldbox->setCurrentItem( index-1 );
}


void FieldDialog::downField() {

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentItem();
  if (index<0 || index>=n-1) return;

  SelectedField sf= selectedFields[index];
  selectedFields[index]= selectedFields[index+1];
  selectedFields[index+1]= sf;

  QString qstr1= selectedFieldbox->text(index);
  QString qstr2= selectedFieldbox->text(index+1);
  selectedFieldbox->changeItem(qstr2,index);
  selectedFieldbox->changeItem(qstr1,index+1);

  //some fields can't be minus
  for(int i=0;i<n;i++){
    selectedFieldbox->setCurrentItem( i );
    if(selectedFields[i].minus && (i==0 || selectedFields[i-1].minus))
      minusButton->setOn(false);
  }

  selectedFieldbox->setCurrentItem( index+1 );
}


void FieldDialog::resetOptions()
{
  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentItem();
  if (index<0 || index>=n) return;

  miString fopts= getFieldOptions(selectedFields[index].fieldName, true);
  if (fopts.empty()) return;

  selectedFields[index].fieldOpts= fopts;
  selectedFields[index].hourOffset= 0;
  selectedFields[index].hourDiff= 0;
  disableFieldOptions();
  currentFieldOpts.clear();
  enableFieldOptions();
}


miString FieldDialog::getFieldOptions(const miString& fieldName, bool reset) const
{
  miString fieldname= fieldName.downcase();

  map<miString,miString>::const_iterator pfopt;

  if (!reset) {
    // try private options used
    pfopt= fieldOptions.find(fieldname);
    if (pfopt!=fieldOptions.end())
      return pfopt->second;
  }

  // following only searches for original options from the setup file

  pfopt= setupFieldOptions.find(fieldname);
  if (pfopt!=setupFieldOptions.end())
    return pfopt->second;

  // test known suffixes and prefixes to the original name.

  map<miString,miString>::const_iterator pfend= setupFieldOptions.end();

  set<miString>::const_iterator ps;
  size_t l, lname= fieldname.length();

  ps=fieldSuffixes.begin();

  while (pfopt==pfend && ps!=fieldSuffixes.end()) {
    if ((l=(*ps).length())<lname && fieldname.substr(lname-l)==(*ps))
      pfopt= setupFieldOptions.find(fieldname.substr(0,lname-l));
    ps++;
  }

  if (pfopt!=pfend)
    return pfopt->second;

  ps=fieldPrefixes.begin();

  while (pfopt==pfend && ps!=fieldPrefixes.end()) {
    if ((l=(*ps).length())<lname && fieldname.substr(0,l)==(*ps))
      pfopt= setupFieldOptions.find(fieldname.substr(l));
    ps++;
  }

  if (pfopt!=pfend)
    return pfopt->second;

  return miString();
}


void FieldDialog::minusField(bool on)
{
  int i= selectedFieldbox->currentItem();
  if (i<0 || i>=selectedFieldbox->count()) return;

  QString qstr = selectedFieldbox->currentText();

  if (on) {
    if (!selectedFields[i].minus){
      selectedFields[i].minus=true;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->changeItem("  -  " + qstr,i);
      selectedFieldbox->blockSignals(false);
    }
    disableFieldOptions(1);
    //next field can't be minus
    if (selectedFieldbox->count()>i+1 && selectedFields[i+1].minus){
      selectedFieldbox->setCurrentItem( i+1 );
      minusButton->setOn(false);
      selectedFieldbox->setCurrentItem( i );
    }
  } else {
    if (selectedFields[i].minus){
      selectedFields[i].minus=false;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->changeItem(qstr.remove(0,5),i);
      selectedFieldbox->blockSignals(false);
      currentFieldOpts.clear();
      enableFieldOptions();
    }
  }

}


void FieldDialog::updateTime(){

  vector<miTime> fieldtime;
  int m;

  if ((m=selectedFields.size())>0) {

    vector<FieldTimeRequest> request;
    FieldTimeRequest ftr;

    int nr=0;

    for (int i=0; i<m; i++) {
      if (!selectedFields[i].inEdit) {
	request.push_back(ftr);
        request[nr].modelName=  selectedFields[i].modelName;
        request[nr].fieldName=  selectedFields[i].fieldName;
        request[nr].levelName=  selectedFields[i].level;
        request[nr].idnumName=  selectedFields[i].idnum;
        request[nr].hourOffset=  selectedFields[i].hourOffset;
        request[nr].hourDiff=    selectedFields[i].hourDiff;
	request[nr].forecastSpec= 0;

	if (selectedFields[i].forecastSpec) {
 	  vector<ParsedCommand> vpc= cp->parse( selectedFields[i].fieldOpts );
	  int nvpc= vpc.size();
	  int j= 0;
	  while (j<nvpc && vpc[j].idNumber!=2) j++;
	  if (j<nvpc) {
	    if (vpc[j].key=="forecast.hour")
	      request[nr].forecastSpec= 1;
	    else if (vpc[j].key=="forecast.hour.loop")
	      request[nr].forecastSpec= 2;
	    request[nr].forecast= vpc[j].intValue;
          }
	}
	nr++;
      }
    }

    if (nr>0) {
      bool allTimeSteps= allTimeStepButton->isOn();
      fieldtime= m_ctrl->getFieldTime(request,allTimeSteps);
    }
  }

#ifdef DEBUGREDRAW
  cerr<<"FieldDialog::updateTime emit emitTimes  fieldtime.size="
      <<fieldtime.size()<<endl;
#endif
  emit emitTimes("field",fieldtime);

  //  allTimeStepButton->setOn(false);
}

void FieldDialog::addField(miString str) 
{

  bool remove = false;
  vector<miString> token = str.split(2," ",true);
  if(token.size()==2 && token[0]=="REMOVE") {
    str = token[1];
    remove = true;
  }

  vector<miString> vstr = getOKString();
  vector<miString>::iterator p=vstr.begin();;
  for(;p!=vstr.end();p++){
    if((*p).contains(str)){
      p = vstr.erase(p);
      if( p == vstr.end() ) break;
    }
  } 
  if(!remove){
    vstr.push_back(str);
  }
  putOKString(vstr);

}

void FieldDialog::fieldEditUpdate(miString str) {

#ifdef DEBUGREDRAW
  if (str.empty()) cerr<<"FieldDialog::fieldEditUpdate STOP"<<endl;
  else             cerr<<"FieldDialog::fieldEditUpdate START "<<str<<endl;
#endif

  int i,j,m, n= selectedFields.size();

  if (str.empty()) {

    // remove fixed edit field(s)
    vector<int> keep;
    m= selectedField2edit_exists.size();
    bool change= false;
    for (i=0; i<n; i++) {
      if (!selectedFields[i].inEdit) {
	keep.push_back(i);
      } else if (i<m && selectedField2edit_exists[i]) {
	selectedFields[i]= selectedField2edit[i];
        miString text= selectedFields[i].modelName + " "
		     + selectedFields[i].fieldName;
        QString qtext= text.c_str();
        selectedFieldbox->changeItem(qtext,i);
	keep.push_back(i);
	change= true;
      }
    }
    m= keep.size();
    if (m<n) {
      for (i=0; i<m; i++) {
        j= keep[i];
        selectedFields[i]= selectedFields[j];
	QString qstr= selectedFieldbox->text(j);
	selectedFieldbox->changeItem(qstr,i);
      }
      selectedFields.resize(m);
      if (m==0) {
	selectedFieldbox->clear();
      } else {
        for (i=m; i<n; i++)
	  selectedFieldbox->removeItem(i);
      }
    }

    numEditFields= 0;
    selectedField2edit.clear();
    selectedField2edit_exists.clear();
    if (change) {
      updateTime();
#ifdef DEBUGREDRAW
      cerr<<"FieldDialog::fieldEditUpdate emit FieldApply"<<endl;
#endif
      emit FieldApply();
    }

  } else {

    // add edit field (and remove the original field)
    bool found= false;
    int indrm= -1;
    SelectedField sf;
    vector<miString> vstr= str.split(' ');
    if (vstr.size()>=2) {
      // new edit field
      miString modelname= vstr[0];
      miString fieldname= vstr[1];
      for (i=0; i<n; i++) {
        if (!selectedFields[i].inEdit) {
	  if (selectedFields[i].modelName==modelname &&
              selectedFields[i].fieldName==fieldname) break;
	}
      }
      if (i<n) {
	sf= selectedFields[i];
	indrm= i;
	found= true;
      }
    } else if (vstr.size()==1) {
      // open/combine edit field
      miString fieldname= vstr[0];
      map<miString,miString>::const_iterator pfo;
      if ((pfo=fieldOptions.find(fieldname.downcase()))!=fieldOptions.end()) {
	sf.fieldName= fieldname;
	sf.fieldOpts= pfo->second;
	found= true;
      }
    }
    if (found) {
      sf.inEdit=     true;
      sf.indexMGR=   -1;
      sf.indexM=     -1;
      sf.hourOffset=  0;
      sf.hourDiff=    0;
      sf.minus=   false;
      if (indrm>=0) {
	selectedField2edit.push_back(selectedFields[indrm]);
	selectedField2edit_exists.push_back(true);
	n= selectedFields.size();
	for (i=indrm; i<n-1; i++)
	  selectedFields[i]= selectedFields[i+1];
	selectedFields.pop_back();
	selectedFieldbox->removeItem(indrm);
      } else {
        SelectedField sfdummy;
	selectedField2edit.push_back(sfdummy);
	selectedField2edit_exists.push_back(false);
      }

      vector<ParsedCommand> vpopt= cp->parse( sf.fieldOpts );
      cp->replaceValue(vpopt,"field.smooth","0",0);
      sf.fieldOpts= cp->unParse(vpopt);

      n= selectedFields.size();
      SelectedField sfdummy;
      selectedFields.push_back(sfdummy);
      for (i=n; i>numEditFields; i--)
	selectedFields[i]= selectedFields[i-1];
      selectedFields[numEditFields]= sf;
      miString text= editName + " " + sf.fieldName;
      selectedFieldbox->insertItem(QString(text.c_str()),numEditFields);
      selectedFieldbox->setCurrentItem(numEditFields);
      numEditFields++;

      updateTime();
    }
  }

  if (!selectedFields.empty())
    enableFieldOptions();
  else
    disableFieldOptions();
}


void FieldDialog::allTimeStepToggled(bool on)
{
  updateTime();
  
}

void FieldDialog::applyClicked()
{
  if (historyOkButton->isEnabled()) historyOk();
  emit FieldApply();
}


void FieldDialog::applyhideClicked()
{
  if (historyOkButton->isEnabled()) historyOk();
  emit FieldHide();
  emit FieldApply();
}


void FieldDialog::hideClicked()
{
  emit FieldHide();
}


void FieldDialog::helpClicked()
{
  emit showdoc("ug_fielddialogue.html");
}


bool FieldDialog::close(bool alsoDelete)
{
  emit FieldHide();
  return true;
}

void FieldDialog::highlightButton(QPushButton* button, bool on)
{
  if (button->isEnabled()!=on) {
    if (on) {
      button->setPalette( QPalette(QColor(255,0,0),QColor(192,192,192) ));
      button->setEnabled( true );
    } else {
      button->setPalette( this->palette() );
      button->setEnabled( false );
    }
  }
}

