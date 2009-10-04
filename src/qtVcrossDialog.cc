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

//#define DEBUGPRINT

#include <qcombobox.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QFrame>
#include <QVBoxLayout>

#include <qtVcrossDialog.h>
#include <qtUtility.h>
#include <qtToggleButton.h>
#include <diVcrossManager.h>

#include <iostream>
#include <math.h>

#include <up20x20.xpm>
#include <down20x20.xpm>
#include <up12x12.xpm>
#include <down12x12.xpm>


VcrossDialog::VcrossDialog( QWidget* parent, VcrossManager* vm )
: QDialog(parent), vcrossm(vm)
{
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::VcrossDialog called"<<endl;
#endif

  setWindowTitle( tr("Vertical Crossections"));

  m_advanced= false;

  //  FIRST INITIALISATION OF STATE

  historyPos= -1;

  models= vcrossm->getAllModels();

  // get all fieldnames from setup file
//#################  fieldnames = vcrossm->getAllFieldNames();
//#################################################################
//  for (i=0; i<fieldnames.size(); i++)
//    cerr<<"   field "<<setw(3)<<i<<" : "<<fieldnames[i]<<endl;
//#################################################################

  // get all field plot options from setup file
  setupFieldOptions = vcrossm->getAllFieldOptions();
  fieldOptions = setupFieldOptions;

  map<miString,miString>::iterator pfopt, pfend= fieldOptions.end();
  for (pfopt=fieldOptions.begin(); pfopt!=pfend; pfopt++)
    changedOptions[pfopt->first]= false;
//#################################################################
//  for (pfopt=fieldOptions.begin(); pfopt!=pfend; pfopt++)
//    cerr << pfopt->first << "   " << pfopt->second << endl;
//#################################################################

  //  END FIRST INNITIALISATION OF STATE

  int i, n;

  // Colours
  colourInfo = Colour::getColourInfo();
  csInfo      = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();

  // linewidths
   nr_linewidths= 12;

  // linetypes
  linetypes = Linetype::getLinetypeNames();
  nr_linetypes= linetypes.size();

  // density (of arrows etc, 0=automatic)
  densityStringList << "Auto";
    QString qs;
  for (i=0;  i<10; i++) {
    densityStringList << qs.setNum(i);
  }
  for (i=10;  i<60; i+=10) {
    densityStringList << qs.setNum(i);
  }
  densityStringList << qs.setNum(100);

  //----------------------------------------------------------------
  cp = new CommandParser();

  // add options to the cp's keyDataBase
  cp->addKey("model",      "",1,CommandParser::cmdString);
  cp->addKey("field",      "",1,CommandParser::cmdString);
  cp->addKey("hour.offset","",1,CommandParser::cmdInt);

  // add more plot options to the cp's keyDataBase
  cp->addKey("colour",         "",0,CommandParser::cmdString);
  cp->addKey("colours",        "",0,CommandParser::cmdString);
  cp->addKey("linewidth",      "",0,CommandParser::cmdInt);
  cp->addKey("linetype",       "",0,CommandParser::cmdString);
  cp->addKey("line.interval",  "",0,CommandParser::cmdFloat);
  cp->addKey("density",        "",0,CommandParser::cmdInt);
  cp->addKey("vector.unit",    "",0,CommandParser::cmdFloat);
  cp->addKey("rel.size",       "",0,CommandParser::cmdFloat);
  //cp->addKey("extreme.type",   "",0,CommandParser::cmdString);
  //cp->addKey("extreme.size",   "",0,CommandParser::cmdFloat);
  //cp->addKey("extreme.radius", "",0,CommandParser::cmdFloat);
  cp->addKey("line.smooth",    "",0,CommandParser::cmdInt);
  cp->addKey("zero.line",      "",0,CommandParser::cmdInt);
  cp->addKey("value.label",    "",0,CommandParser::cmdInt);
  cp->addKey("label.size",     "",0,CommandParser::cmdFloat);
  cp->addKey("base",           "",0,CommandParser::cmdFloat);
  //cp->addKey("undef.masking",  "",0,CommandParser::cmdInt);
  //cp->addKey("undef.colour",   "",0,CommandParser::cmdString);
  //cp->addKey("undef.linewidth","",0,CommandParser::cmdInt);
  //cp->addKey("undef.linetype", "",0,CommandParser::cmdString);
  cp->addKey("palettecolours", "",0,CommandParser::cmdString);
  cp->addKey("minvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("maxvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("table",          "",0,CommandParser::cmdInt);
  cp->addKey("patterns",       "",0,CommandParser::cmdString);
  cp->addKey("patterncolour",  "",0,CommandParser::cmdString);
  cp->addKey("repeat",      "",0,CommandParser::cmdInt);
  cp->addKey("alpha",          "",0,CommandParser::cmdInt);

  //----------------------------------------------------------------

  // modellabel
  QLabel *modellabel= TitleLabel( tr("Models"), this );
  //h1 modelbox
  modelbox = new QListWidget( this );
#ifdef DISPLAY1024X768
  modelbox->setMinimumHeight( 60 );
  modelbox->setMaximumHeight( 60 );
#else
  modelbox->setMinimumHeight( 80 );
  modelbox->setMaximumHeight( 80 );
#endif

  n= models.size();
  if (n>0) {
    for (i=0; i<n; i++)
      modelbox->addItem(QString(models[i].c_str()));
  }

  connect( modelbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( modelboxClicked( QListWidgetItem * ) ) );

  // fieldlabel
  QLabel *fieldlabel= TitleLabel( tr("Fields"), this );

  // fieldbox
  fieldbox = new QListWidget( this );
#ifdef DISPLAY1024X768
  fieldbox->setMinimumHeight( 64 );
  fieldbox->setMaximumHeight( 64 );
#else
  fieldbox->setMinimumHeight( 132 );
  fieldbox->setMaximumHeight( 132 );
#endif
  fieldbox->setSelectionMode(QAbstractItemView::MultiSelection);

  connect( fieldbox, SIGNAL( itemClicked(QListWidgetItem*) ),
  	   SLOT( fieldboxChanged(QListWidgetItem*) ) );

  // selectedFieldlabel
  QLabel *selectedFieldlabel= TitleLabel( tr("Selected Fields"), this );

  // selectedFieldbox
  selectedFieldbox = new QListWidget( this );
#ifdef DISPLAY1024X768
  selectedFieldbox->setMinimumHeight( 55 );
  selectedFieldbox->setMaximumHeight( 55 );
#else
  selectedFieldbox->setMinimumHeight( 80 );
  selectedFieldbox->setMaximumHeight( 80 );
#endif
  selectedFieldbox->setEnabled( true );

  connect( selectedFieldbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( selectedFieldboxClicked( QListWidgetItem * ) ) );

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
  deleteAll = NormalPushButton( tr("Delete All"), this );
  deleteAll->setEnabled( false );

  connect( deleteAll, SIGNAL(clicked()), SLOT(deleteAllSelected()));

  // changeModelButton
  changeModelButton = NormalPushButton( tr("Model"), this );
  changeModelButton->setEnabled( false );

  connect(changeModelButton, SIGNAL(clicked()), SLOT(changeModel()));

  // historyBack
//QPixmap hback = QPixmap(up32x24_xpm);
  QPixmap hback = QPixmap(up20x20_xpm);
//##//historyBackButton = PixmapButton( hback, this, 4, 4 );
//##  historyBackButton = PixmapButton( hback, this, 2, 8 );
  historyBackButton= new QPushButton(hback,"",this);
  historyBackButton->setMinimumSize(hbutton+4,hbutton);
  historyBackButton->setMaximumSize(hbutton+4,hbutton);
  historyBackButton->setEnabled( false );

  connect( historyBackButton, SIGNAL(clicked()), SLOT(historyBack()));

  // historyForward
//QPixmap hforward = QPixmap(down32x24_xpm);
  QPixmap hforward = QPixmap(down20x20_xpm);
//##//historyForwardButton = PixmapButton( hforward, this, 4, 4 );
//##  historyForwardButton = PixmapButton( hforward, this, 2, 8 );
  historyForwardButton= new QPushButton(hforward,"",this);
  historyForwardButton->setMinimumSize(hbutton+4,hbutton);
  historyForwardButton->setMaximumSize(hbutton+4,hbutton);
  historyForwardButton->setEnabled( false );

  connect( historyForwardButton,SIGNAL(clicked()),SLOT(historyForward()));

  // historyOk
  historyOkButton = NormalPushButton( tr("OK"), this );
  historyOkButton->setMinimumWidth(hbutton*2);
//historyOkButton->setMaximumWidth(hbutton*2+8);
  highlightButton(historyOkButton,false);

  connect( historyOkButton, SIGNAL(clicked()), SLOT(historyOk()));

  int wbutton= hbutton-4;

  // upField
  QPixmap upfield = QPixmap(up12x12_xpm);
//upFieldButton = PixmapButton( upfield, this, 14, 12 );
//upFieldButton = PixmapButton( upfield, this, 12, 12 );
  upFieldButton= new QPushButton(upfield,"",this);
  upFieldButton->setMinimumSize(wbutton,hbutton);
  upFieldButton->setMaximumSize(wbutton,hbutton);
  upFieldButton->setEnabled( false );

  connect( upFieldButton, SIGNAL(clicked()), SLOT(upField()));

  // downField
  QPixmap downfield = QPixmap(down12x12_xpm);
//downFieldButton = PixmapButton( downfield, this, 14, 12 );
//downFieldButton = PixmapButton( downfield, this, 12, 12 );
  downFieldButton= new QPushButton(downfield,"",this);
  downFieldButton->setMinimumSize(wbutton,hbutton);
  downFieldButton->setMaximumSize(wbutton,hbutton);
  downFieldButton->setEnabled( false );

  connect( downFieldButton, SIGNAL(clicked()), SLOT(downField()));

  // resetOptions
  resetOptionsButton = NormalPushButton( tr("R"), this );
  resetOptionsButton->setMinimumWidth(wbutton);
//resetOptionsButton->setMaximumWidth(wbutton);
  resetOptionsButton->setEnabled( false );

  connect( resetOptionsButton, SIGNAL(clicked()), SLOT(resetOptions()));

  // colorCbox
  colorlabel= new QLabel( tr("Colour"), this );
  colorCbox= ColourBox(this,colourInfo,false,0,tr("off").toStdString(),true);
  colorCbox->setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  colorCbox->setEnabled( false );

  connect( colorCbox, SIGNAL( activated(int) ),
	   SLOT( colorCboxActivated(int) ) );

  // linewidthcbox
  linewidthlabel= new QLabel( tr("Line width"), this );
  lineWidthCbox=  LinewidthBox( this, false );
  lineWidthCbox->setEnabled( false );

  connect( lineWidthCbox, SIGNAL( activated(int) ),
	   SLOT( lineWidthCboxActivated(int) ) );

  // linetypecbox
  linetypelabel= new QLabel( tr("Line type"), this );
  lineTypeCbox = LinetypeBox( this,false );
  lineTypeCbox->setEnabled( false );

  connect( lineTypeCbox, SIGNAL( activated(int) ),
	   SLOT( lineTypeCboxActivated(int) ) );

  // lineinterval
  lineintervallabel= new QLabel( tr("Line interval"), this );
  lineintervalCbox=  new QComboBox( false, this );
  lineintervalCbox->setEnabled( false );

  connect( lineintervalCbox, SIGNAL( activated(int) ),
	   SLOT( lineintervalCboxActivated(int) ) );

  // density
  densitylabel= new QLabel( tr("Density"), this );
  densityCbox=  new QComboBox( false, this );
  densityCbox->setEnabled( false );

  connect( densityCbox, SIGNAL( activated(int) ),
	   SLOT( densityCboxActivated(int) ) );

  // vectorunit
  vectorunitlabel= new QLabel( tr("Unit"), this );
  vectorunitCbox=  new QComboBox( false, this );
  vectorunitCbox->setEnabled( false );

  connect( vectorunitCbox, SIGNAL( activated(int) ),
	   SLOT( vectorunitCboxActivated(int) ) );

  // Extreme (min,max): type, size and search radius
  //QLabel* extremeTypeLabel= new QLabel( "Min,max", this );
  //extremeTypeCbox= new QComboBox( false, this );
  //extremeTypeCbox->setEnabled( false );
  //extremeType.push_back("Ingen");
  //extremeType.push_back("L+H");
  //extremeType.push_back("C+W");
  //connect( extremeTypeCbox, SIGNAL( activated(int) ),
  //	   SLOT( extremeTypeActivated(int) ) );

  // help
  fieldhelp = NormalPushButton( tr("Help"), this );
  connect( fieldhelp, SIGNAL(clicked()), SLOT(helpClicked()));

  // allTimeStep
  allTimeStepButton = 0;
  //allTimeStepButton = NormalPushButton( tr("All times"), this );
  //allTimeStepButton->setToggleButton(true);
  //allTimeStepButton->setOn(false);

  // advanced
  miString more_str[2] = { tr("<<Less").toStdString(), tr("More>>").toStdString() };
  advanced= new ToggleButton( this, more_str);
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
  QVBoxLayout* v1layout = new QVBoxLayout( 5 );
  v1layout->addWidget( modellabel );
  v1layout->addWidget( modelbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( fieldlabel );
  v1layout->addWidget( fieldbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( selectedFieldlabel );
  v1layout->addWidget( selectedFieldbox );

  QVBoxLayout* h2layout= new QVBoxLayout( 2 );
  h2layout->addWidget( upFieldButton );
  h2layout->addWidget( downFieldButton );
  h2layout->addWidget( resetOptionsButton );
  h2layout->addStretch(1);

  QHBoxLayout* v1h4layout = new QHBoxLayout( 2 );
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

  //optlayout = new QGridLayout( 7, 2, 1 );
  QGridLayout* optlayout = new QGridLayout( 6, 2, 1 );
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
  //optlayout->addWidget( extremeTypeLabel, 6, 0 );
  //optlayout->addWidget( extremeTypeCbox,  6, 1 );

  QHBoxLayout* h4layout = new QHBoxLayout( 5 );
  h4layout->addLayout( h2layout );
  h4layout->addLayout( optlayout );

  QHBoxLayout* h5layout = new QHBoxLayout( 2 );
  h5layout->addWidget( fieldhelp );
  //h5layout->addWidget( allTimeStepButton );
  h5layout->addWidget( advanced );

  QHBoxLayout* h6layout = new QHBoxLayout( 2 );
  h6layout->addWidget( fieldhide );
  h6layout->addWidget( fieldapplyhide );
  h6layout->addWidget( fieldapply );

  QVBoxLayout* v6layout= new QVBoxLayout( 2 );
  v6layout->addLayout( h5layout );
  v6layout->addLayout( h6layout );

  // vlayout
#ifdef DISPLAY1024X768
  QVBoxLayout* vlayout = new QVBoxLayout( this, 5, 5 );
#else
  QVBoxLayout* vlayout = new QVBoxLayout( this, 10, 10 );
#endif
  vlayout->addLayout( v1layout );
  vlayout->addLayout( h3layout );
  vlayout->addLayout( h4layout );
  vlayout->addLayout( v6layout );

  vlayout->activate();
//vlayout->freeze();

  CreateAdvanced();

  this->setOrientation(Qt::Horizontal);
  this->setExtension(advFrame);
  advancedToggled( false );

  // tool tips
  toolTips();

  // keep focus away from the modelbox (with none selected)
  fieldbox->setFocus();

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::ConstructorCernel returned"<<endl;
#endif
}


void VcrossDialog::toolTips()
{
  QToolTip::add( upFieldButton,        tr("move selected field up"));
  QToolTip::add( downFieldButton,      tr("move selected field down" ));
  QToolTip::add( Delete,               tr("remove selected field"));
  QToolTip::add( deleteAll,            tr("remove all selected fields") );
  QToolTip::add( copyField,            tr("copy field") );
  QToolTip::add( resetOptionsButton,   tr("reset plot layout"));
  QToolTip::add( changeModelButton,    tr("change model/modeltime"));
  QToolTip::add( historyBackButton,    tr("history backward"));
  QToolTip::add( historyForwardButton, tr("history forward"));
  QToolTip::add( historyOkButton,      tr("use current history"));
  //QToolTip::add( allTimeStepButton,    tr("all times / union of times") );
}

void VcrossDialog::advancedToggled(bool on){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::advancedToggled  on= " << on <<endl;
#endif

  this->showExtension(on);
  m_advanced= on;
}


void VcrossDialog::CreateAdvanced() {
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::CreateAdvanced" <<endl;
#endif

  advFrame= new QWidget(this,"frame");

  //QLabel* extremeTypeLabelHead= new QLabel( "Min,max", advFrame );
  //QLabel* extremeSizeLabel= new QLabel( "Størrelse",  advFrame );
  //extremeSizeSpinBox= new QSpinBox( 5,300,5, advFrame );
  //extremeSizeSpinBox->setWrapping(true);
  //extremeSizeSpinBox->setSuffix("%");
  //extremeSizeSpinBox->setValue(100);
  //extremeSizeSpinBox->setEnabled( false );
  //connect( extremeSizeSpinBox, SIGNAL( valueChanged(int) ),
  //	   SLOT( extremeSizeChanged(int) ) );

  //QLabel* extremeRadiusLabel= new QLabel( "Søkeradius", advFrame );
  //extremeRadiusSpinBox= new QSpinBox( 5,300,5, advFrame );
  //extremeRadiusSpinBox->setWrapping(true);
  //extremeRadiusSpinBox->setSuffix("%");
  //extremeRadiusSpinBox->setValue(100);
  //extremeRadiusSpinBox->setEnabled( false );
  //connect( extremeRadiusSpinBox, SIGNAL( valueChanged(int) ),
  //	   SLOT( extremeRadiusChanged(int) ) );

  // line smoothing
  QLabel* lineSmoothLabel= new QLabel( tr("Smooth lines"), advFrame );
  lineSmoothSpinBox= new QSpinBox( 0,50,2, advFrame );
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled( false );
  connect( lineSmoothSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( lineSmoothChanged(int) ) );

  QLabel* labelSizeLabel= new QLabel( tr("Digit size"),  advFrame );
  labelSizeSpinBox= new QSpinBox( 5,300,5, advFrame );
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled( false );
  connect( labelSizeSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( labelSizeChanged(int) ) );

  QLabel* hourOffsetLabel= new QLabel( tr("Time offset"),  advFrame );
  hourOffsetSpinBox= new QSpinBox( -72,72,1, advFrame );
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->setEnabled( false );
  connect( hourOffsetSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( hourOffsetChanged(int) ) );

  // Undefined masking
  ////QLabel* undefMaskingLabel= new QLabel( "Udefinert", advFrame );
  //undefMaskingCbox= new QComboBox( false, advFrame );
  //undefMaskingCbox->setEnabled( false );
  //undefMasking.push_back("Umarkert");
  //undefMasking.push_back("Farget");
  //undefMasking.push_back("Linjer");
  //connect( undefMaskingCbox, SIGNAL( activated(int) ),
  //	   SLOT( undefMaskingActivated(int) ) );

  // Undefined masking colour
  ////QLabel* undefColourLabel= new QLabel( COLOR, advFrame );
  //undefColourCbox= new QComboBox( false, advFrame );
  //undefColourCbox->setEnabled( false );
  //connect( undefColourCbox, SIGNAL( activated(int) ),
  //	   SLOT( undefColourActivated(int) ) );

  // Undefined masking linewidth (if used)
  ////QLabel* undefLinewidthLabel= new QLabel( LINEWIDTH, advFrame );
  //undefLinewidthCbox= new QComboBox( false, advFrame );
  //undefLinewidthCbox->setEnabled( false );
  //connect( undefLinewidthCbox, SIGNAL( activated(int) ),
  //	   SLOT( undefLinewidthActivated(int) ) );

  // Undefined masking linetype (if used)
  ////QLabel* undefLinetypeLabel= new QLabel( LINETYPE, advFrame );
  //undefLinetypeCbox= new QComboBox( false, advFrame );
  //undefLinetypeCbox->setEnabled( false );
  //connect( undefLinetypeCbox, SIGNAL( activated(int) ),
  //	   SLOT( undefLinetypeActivated(int) ) );

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox= new QCheckBox(tr("Zero-line"), advFrame);
  zeroLineCheckBox->setChecked( true );
  zeroLineCheckBox->setEnabled( false );
  connect( zeroLineCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( zeroLineCheckBoxToggled(bool) ) );

  // enable/disable numbers on isolines
valueLabelCheckBox= new QCheckBox(tr("Number on line"), advFrame);
  valueLabelCheckBox->setChecked( true );
  valueLabelCheckBox->setEnabled( false );
  connect( valueLabelCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( valueLabelCheckBoxToggled(bool) ) );

  QLabel* shadingLabel    = new QLabel( tr("Palette"),            advFrame );
  QLabel* shadingcoldLabel= new QLabel( tr("Palette (-)"),        advFrame );
  QLabel* patternLabel    = new QLabel( tr("Pattern"),            advFrame );
  QLabel* alphaLabel      = new QLabel( tr("Alpha"),              advFrame );
  QLabel* baseLabel       = new QLabel( tr("Basis value"),        advFrame);
  QLabel* minLabel        = new QLabel( tr("Min"),                advFrame);
  QLabel* maxLabel        = new QLabel( tr("Max"),               advFrame);


//  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
//  tableCheckBox->setEnabled(false);
//  connect( tableCheckBox, SIGNAL( toggled(bool) ),
//	   SLOT( tableCheckBoxToggled(bool) ) );

  repeatCheckBox = new QCheckBox(tr("Repeat"), advFrame);
  repeatCheckBox->setEnabled(false);
  connect( repeatCheckBox, SIGNAL( toggled(bool) ),
	   SLOT( repeatCheckBoxToggled(bool) ) );

  //shading
  shadingComboBox=  PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString() );
  connect( shadingComboBox, SIGNAL( activated(int) ),
	   SLOT( shadingChanged() ) );

  shadingSpinBox = new QSpinBox(0,99,1,advFrame);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  shadingSpinBox->setEnabled(false);
  connect( shadingSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( shadingChanged() ) );

  shadingcoldComboBox=  PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString() );
  connect( shadingcoldComboBox, SIGNAL( activated(int) ),
	   SLOT( shadingChanged() ) );

  shadingcoldSpinBox = new QSpinBox(0,99,1,advFrame);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  shadingcoldSpinBox->setEnabled(false);
  connect( shadingcoldSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( shadingChanged() ) );

  //pattern
  patternComboBox = PatternBox( advFrame,patternInfo,false,0,tr("Off").toStdString() );
  connect( patternComboBox, SIGNAL( activated(int) ),
	   SLOT( patternComboBoxToggled(int) ) );

  //pattern colour
  patternColourBox = ColourBox(advFrame,colourInfo,false,0,tr("Auto").toStdString());
  connect( patternColourBox, SIGNAL( activated(int) ),
	   SLOT( patternColourBoxToggled(int) ) );

  //alpha blending
  alphaSpinBox = new QSpinBox(0,255,5,advFrame);
  alphaSpinBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  connect( alphaSpinBox, SIGNAL( valueChanged(int) ),
	   SLOT( alphaChanged(int) ) );
  //zero value
  zero1ComboBox= new QComboBox( advFrame );
  zero1ComboBox->setEnabled( false );
  connect( zero1ComboBox, SIGNAL( activated(int) ),
	   SLOT( zero1ComboBoxToggled(int) ) );

  //min
  min1ComboBox = new QComboBox(advFrame);
  min1ComboBox->setEnabled( false );

  //max
  max1ComboBox = new QComboBox(advFrame);
  max1ComboBox->setEnabled( false );

  connect( min1ComboBox, SIGNAL( activated(int) ),
	   SLOT( min1ComboBoxToggled(int) ) );
  connect( max1ComboBox, SIGNAL( activated(int) ),
	   SLOT( max1ComboBoxToggled(int) ) );


  // layout......................................................

  QVBoxLayout *adv1Layout = new QVBoxLayout( 1 );
  int space= 6;
  adv1Layout->addStretch();
  adv1Layout->addWidget(lineSmoothLabel);
  adv1Layout->addWidget(lineSmoothSpinBox);
  adv1Layout->addSpacing(space);
  adv1Layout->addWidget(labelSizeLabel);
  adv1Layout->addWidget(labelSizeSpinBox);
  adv1Layout->addSpacing(space);
  adv1Layout->addWidget(hourOffsetLabel);
  adv1Layout->addWidget(hourOffsetSpinBox);
  adv1Layout->addSpacing(space);
  adv1Layout->addWidget(zeroLineCheckBox);
  adv1Layout->addWidget(valueLabelCheckBox);
  adv1Layout->addSpacing(space);

  // a separator
  QFrame* advSep= new QFrame( advFrame );
  advSep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  advSep->setLineWidth( 5 );

  QFrame* advSep2= new QFrame( advFrame );
  advSep2->setFrameStyle( QFrame::HLine | QFrame::Raised );
  advSep2->setLineWidth( 5 );

  QGridLayout* adv2Layout = new QGridLayout( 10, 3);
  adv2Layout->addWidget(advSep2,               0, 0);
//  adv2Layout->addWidget( tableCheckBox,      1, 0 );
  adv2Layout->addWidget( repeatCheckBox,  1, 0 );
  adv2Layout->addWidget( shadingLabel,       2, 0 );
  adv2Layout->addWidget( shadingComboBox,    2, 1 );
  adv2Layout->addWidget( shadingSpinBox,     2, 2 );
  adv2Layout->addWidget( shadingcoldLabel,   3, 0 );
  adv2Layout->addWidget( shadingcoldComboBox,3, 1 );
  adv2Layout->addWidget( shadingcoldSpinBox, 3, 2 );
  adv2Layout->addWidget( patternLabel,       4, 0 );
  adv2Layout->addWidget( patternComboBox,    4, 1 );
  adv2Layout->addWidget( patternColourBox,   4, 2 );
  adv2Layout->addWidget( alphaLabel,         5, 0 );
  adv2Layout->addWidget( alphaSpinBox,       5, 1 );
  adv2Layout->addWidget( baseLabel,          6, 0 );
  adv2Layout->addWidget( zero1ComboBox,      6, 1 );
  adv2Layout->addWidget( minLabel,           7, 0 );
  adv2Layout->addWidget( min1ComboBox,       7, 1 );
  adv2Layout->addWidget( maxLabel,           8, 0 );
  adv2Layout->addWidget( max1ComboBox,       8, 1 );

  QVBoxLayout *advLayout = new QVBoxLayout( 1 );
  advLayout->addLayout(adv1Layout);
  advLayout->addLayout(adv2Layout);

  QHBoxLayout *hLayout = new QHBoxLayout( advFrame,5,5 );

  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);

  return;
}


void VcrossDialog::modelboxClicked( QListWidgetItem * item  ){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::modelboxHighlighted called"<<endl;
#endif

  int index = modelbox->row(item);

  fields= vcrossm->getFieldNames(models[index]);

//###  fieldbox->clearSelection();
  if (fieldbox->count()>0) {
    fieldbox->clearSelection();
    fieldbox->clear();
  }

  int nf= fields.size();

  if (nf<1) return;

  fieldbox->blockSignals(true);

  for (int i=0; i<nf; i++)
      fieldbox->addItem(QString(fields[i].c_str()));
  fieldbox->setEnabled( true );

  countSelected.resize(nf);
  for (int i=0; i<nf; ++i) countSelected[i]= 0;

  int j,n= selectedFields.size();

  for (int i=0; i<n; ++i) {
    if (selectedFields[i].model==models[index]) {
      j=0;
      while (j<nf && selectedFields[i].field!=fields[j]) j++;
      if (j<nf) {
	fieldbox->item(j)->setSelected(true);
	countSelected[j]++;
      }
    }
  }

  fieldbox->blockSignals(false);

  // possibly enable changeModel button
  if (selectedFields.size()>0)
    enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::modelboxHighlighted returned"<<endl;
#endif
}


void VcrossDialog::fieldboxChanged(QListWidgetItem* item)
{
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::fieldboxChanged called"<<endl;
#endif

  if (historyOkButton->isEnabled()) deleteAllSelected();

  int i, j, jp, n;
  int indexM   = modelbox->currentRow();

  // NOTE: multiselection list, current item may only be the last...

  int nf= fields.size();

  int last= -1;
  int lastdelete= -1;

  for (int indexF=0; indexF<nf; ++indexF) {

    if (fieldbox->item(indexF)->isSelected() && countSelected[indexF]==0) {

      SelectedField sf;
      sf.model=    models[indexM];
      sf.field=    fields[indexF];
      sf.hourOffset= 0;

      map<miString,miString>::iterator pfopt;
      pfopt= fieldOptions.find(fields[indexF]);
      if (pfopt!=fieldOptions.end())
	sf.fieldOpts= pfopt->second;

      selectedFields.push_back(sf);

      countSelected[indexF]++;

      selectedFieldbox->addItem(modelbox->item(indexM)->text() + " " +
				fieldbox->item(indexF)->text());
      last= selectedFields.size()-1;

    } else if (!fieldbox->item(indexF)->isSelected() && countSelected[indexF]>0) {

      n= selectedFields.size();
      j= jp= -1;
      int jsel=-1, isel= selectedFieldbox->currentRow();
      for (i=0; i<n; i++) {
	if (selectedFields[i].model==models[indexM]  &&
	    selectedFields[i].field==fields[indexF]) {
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
	  fieldbox->setCurrentRow(indexF);
	  fieldbox->item(indexF)->setSelected(true);
	  fieldbox->blockSignals(false);
	  lastdelete= jp;
	} else {
	  lastdelete= j;
	}
	countSelected[indexF]--;
        selectedFieldbox->takeItem(j);
	for (i=j; i<n-1; i++)
	  selectedFields[i]=selectedFields[i+1];
	selectedFields.pop_back();
      }
    }
  }

  if (last<0 && lastdelete>=0 && selectedFields.size()>0) {
    last= lastdelete;
    if (last>=int(selectedFields.size())) last= selectedFields.size() - 1;
  }

  if (last>=0 && selectedFieldbox->item(last)) {
    selectedFieldbox->setCurrentRow(last);
    selectedFieldbox->item(last)->setSelected(true);
    enableFieldOptions();
  } else if (selectedFields.size()==0) {
    disableFieldOptions();
  }

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::fieldboxChanged returned"<<endl;
#endif
  return;
}


void VcrossDialog::enableFieldOptions(){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::enableFieldOptions called"<<endl;
#endif

  float e;
  int   index, lastindex, nc, i;

  index= selectedFieldbox->currentRow();
  lastindex= selectedFields.size()-1;

  if (index<0 || index>lastindex) {
    cerr << "PROGRAM ERROR.1 in VcrossDialog::enableFieldOptions" << endl;
    cerr << "       index,selectedFields.size: "
	 << index << " " << selectedFields.size() << endl;
    disableFieldOptions();
    return;
  }

  int indexM= modelbox->currentRow();

  upFieldButton->setEnabled( (index>0) );
  downFieldButton->setEnabled( (index<lastindex) );
  if (models.size()==0 || indexM<0 || indexM>=int(models.size()))
    changeModelButton->setEnabled( false );
  else if (selectedFields[index].model==models[indexM])
    changeModelButton->setEnabled( false );
  else
    changeModelButton->setEnabled( true );
  Delete->setEnabled( true );
  copyField->setEnabled( true );
//###############################################################################
//  cerr<<"fieldOpts: "<<selectedFields[index].fieldOpts<<endl;
//###############################################################################

  if (selectedFields[index].fieldOpts==currentFieldOpts) return;

  currentFieldOpts= selectedFields[index].fieldOpts;

//###############################################################################
//  cerr << "VcrossDialog::enableFieldOptions: "
//       << fieldnames[selectedFields[index].fieldnumber] << endl;
//  cerr << "             " << selectedFields[index].fieldOpts << endl;
//###############################################################################

  deleteAll->setEnabled( true );
  resetOptionsButton->setEnabled( true );

  vpcopt= cp->parse(selectedFields[index].fieldOpts);

//###############################################################################
/*******************************************************
  n=vpcopt.size();
  bool err=false;
  bool listall= true;
  for (j=0; j<n; j++)
    if (vpcopt[j].key=="unknown") err=true;
  if (err || listall) {
    cerr << "VcrossDialog::enableFieldOptions: "
         << fieldnames[selectedFields[index].fieldnumber] << endl;
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
//###############################################################################

  // colour(s)
  if ((nc=cp->findKey(vpcopt,"colour"))>=0) {
    shadingComboBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    if (!colorCbox->isEnabled()) {
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
    colorCbox->setEnabled(false);
  }

  //contour shading
  if ((nc=cp->findKey(vpcopt,"palettecolours"))>=0) {
    shadingComboBox->setEnabled(true);
    shadingSpinBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    shadingcoldSpinBox->setEnabled(true);
//    tableCheckBox->setEnabled(true);
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
//    tableCheckBox->setEnabled(false);
//    updateFieldOptions("table","remove");
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
    //cerr <<"patterns:"<<value<<endl;
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
//  nc=cp->findKey(vpcopt,"table");
//  if (nc>=0) {
//    bool on= vpcopt[nc].allValue=="1";
//    tableCheckBox->setChecked( on );
//    tableCheckBox->setEnabled(true);
//    tableCheckBoxToggled(on);
//  }

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
    if (vpcopt[nc].intValue.size()>0) i=vpcopt[nc].intValue[0];
    else i=255;
    alphaSpinBox->setValue(i);
    alphaSpinBox->setEnabled(true);
  } else {
    alphaSpinBox->setValue(255);
    updateFieldOptions("alpha","remove");
  }

  // linetype
  if ((nc=cp->findKey(vpcopt,"linetype"))>=0) {
    lineTypeCbox->setEnabled(true);
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("linetype",linetypes[i]);
    }
    lineTypeCbox->setCurrentItem(i);
  } else if (lineTypeCbox->isEnabled()) {
    lineTypeCbox->setEnabled(false);
  }

  // linewidth
  if ((nc=cp->findKey(vpcopt,"linewidth"))>=0) {
    lineWidthCbox->setEnabled(true);
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miString(i+1)) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("linewidth",miString(i+1));
    }
    lineWidthCbox->setCurrentItem(i);
  } else if (lineWidthCbox->isEnabled()) {
    lineWidthCbox->setEnabled(false);
  }

  // line interval (isoline contouring)
  float ekv=-1.;
  if ((nc=cp->findKey(vpcopt,"line.interval"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) ekv=vpcopt[nc].floatValue[0];
    else ekv= 10.;
    lineintervals= numberList( lineintervalCbox, ekv);
    lineintervalCbox->setEnabled(true);
  } else if (lineintervalCbox->isEnabled()) {
    lineintervalCbox->clear();
    lineintervalCbox->setEnabled(false);
  }

  // wind/vector density
  if ((nc=cp->findKey(vpcopt,"density"))>=0) {
    if (!densityCbox->isEnabled()) {
      densityCbox->addItems(densityStringList);
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
      i = densityStringList.indexOf(QString(s.cStr()));
      if (i==-1) {
        densityStringList <<QString(s.cStr());
	densityCbox->addItem(QString(s.cStr()));
	i=densityCbox->count()-1;
      }
    }
    densityCbox->setCurrentItem(i);
  } else if (densityCbox->isEnabled()) {
    densityCbox->clear();
    densityCbox->setEnabled(false);
  }

  // vectorunit (vector length unit)
  if ((nc=cp->findKey(vpcopt,"vector.unit"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e= vpcopt[nc].floatValue[0];
    else e=5;
    vectorunit= numberList( vectorunitCbox, e);
  } else if (vectorunitCbox->isEnabled()) {
    vectorunitCbox->clear();
    vectorunitCbox->setEnabled(false);
  }

/*************************************************************************
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
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeSizeSpinBox->setValue(i);
    extremeSizeSpinBox->setEnabled(true);
  } else if (extremeSizeSpinBox->isEnabled()) {
    extremeSizeSpinBox->setValue(100);
    extremeSizeSpinBox->setEnabled(false);
  }

  if (extreme && (nc=cp->findKey(vpcopt,"extreme.radius"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeRadiusSpinBox->setValue(i);
    extremeRadiusSpinBox->setEnabled(true);
  } else if (extremeRadiusSpinBox->isEnabled()) {
    extremeRadiusSpinBox->setValue(100);
    extremeRadiusSpinBox->setEnabled(false);
  }
*************************************************************************/

  if ((nc=cp->findKey(vpcopt,"line.smooth"))>=0) {
    if (vpcopt[nc].intValue.size()>0) i=vpcopt[nc].intValue[0];
    else i=0;
    lineSmoothSpinBox->setValue(i);
    lineSmoothSpinBox->setEnabled(true);
  } else if (lineSmoothSpinBox->isEnabled()) {
    lineSmoothSpinBox->setValue(0);
    lineSmoothSpinBox->setEnabled(false);
  }

  if ((nc=cp->findKey(vpcopt,"label.size"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e= 1.0;
    i= (int(e*100.+0.5))/5 * 5;
    labelSizeSpinBox->setValue(i);
    labelSizeSpinBox->setEnabled(true);
  } else if (labelSizeSpinBox->isEnabled()) {
    labelSizeSpinBox->setValue(100);
    labelSizeSpinBox->setEnabled(false);
  }

  // base
  miString base;
  if (ekv>0. && (nc=cp->findKey(vpcopt,"base"))>=0) {
    zero1ComboBox->setEnabled(true);
    base = baseList(zero1ComboBox,vpcopt[nc].floatValue[0],ekv/2.0);
    if( base.exists() ) cp->replaceValue(vpcopt[nc],base,0);
    base.clear();
  } else if (zero1ComboBox->isEnabled()) {
    zero1ComboBox->clear();
    zero1ComboBox->setEnabled(false);
  }

  i= selectedFields[index].hourOffset;
  hourOffsetSpinBox->setValue(i);
  hourOffsetSpinBox->setEnabled(true);

/*************************************************************************
  // undefined masking
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
      i= vpcopt[nc].intValue[0];
      if (i<0 || i>=undefMasking.size()) i=0;
    } else {
      i= 0;
    }
    undefMaskingCbox->setCurrentItem(i);
  } else if (undefMaskingCbox->isEnabled()) {
    undefMaskingCbox->clear();
    undefMaskingCbox->setEnabled(false);
  }

  // undefined masking colour
  if ((nc=cp->findKey(vpcopt,"undef.colour"))>=0) {
    if (!undefColourCbox->isEnabled()) {
      for( i=0; i<nr_colors; i++)
        undefColourCbox->insertItem( *pmapColor[i] );
      undefColourCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      i=0;
      updateFieldOptions("undef.colour",colourInfo[i].name);
    }
    undefColourCbox->setCurrentItem(i);
  } else if (undefColourCbox->isEnabled()) {
    undefColourCbox->clear();
    undefColourCbox->setEnabled(false);
  }

  // undefined masking linewidth (if used)
  if ((nc=cp->findKey(vpcopt,"undef.linewidth"))>=0) {
    if (!undefLinewidthCbox->isEnabled()) {
      for( i=0; i < nr_linewidths; i++)
        undefLinewidthCbox->insertItem ( *pmapLinewidths[i] );
      undefLinewidthCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=linewidths[i]) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("undef.linewidth",linewidths[i]);
    }
    undefLinewidthCbox->setCurrentItem(i);
  } else if (undefLinewidthCbox->isEnabled()) {
    undefLinewidthCbox->clear();
    undefLinewidthCbox->setEnabled(false);
  }

  // undefined masking linetype (if used)
  if ((nc=cp->findKey(vpcopt,"undef.linetype"))>=0) {
    if (!undefLinetypeCbox->isEnabled()) {
      for( i=0; i < nr_linetypes; i++)
        undefLinetypeCbox->insertItem ( *pmapLinetypes[i] );
      undefLinetypeCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("undef.linetype",linetypes[i]);
    }
    undefLinetypeCbox->setCurrentItem(i);
  } else if (undefLinetypeCbox->isEnabled()) {
    undefLinetypeCbox->clear();
    undefLinetypeCbox->setEnabled(false);
  }
*************************************************************************/

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
    zeroLineCheckBox->setEnabled( false );
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
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.cStr());
    else
      value = atof(vpcopt[nc].allValue.cStr());
    baseList(min1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      min1ComboBox->setCurrentItem(0);
  } else {
    min1ComboBox->setEnabled( false );
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
    if(vpcopt[nc].allValue=="off")
      max1ComboBox->setCurrentItem(0);
  } else {
    max1ComboBox->setEnabled( false );
  }

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::enableFieldOptions returned"<<endl;
#endif
}


void VcrossDialog::disableFieldOptions(){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::disableFieldOptions called"<<endl;
#endif

  if (currentFieldOpts.empty()) return;
  currentFieldOpts.clear();

  Delete->setEnabled( false );
  deleteAll->setEnabled( false );
  copyField->setEnabled( false );
  changeModelButton->setEnabled( false );
  upFieldButton->setEnabled( false );
  downFieldButton->setEnabled( false );
  resetOptionsButton->setEnabled( false );

  colorCbox->setEnabled( false );
  shadingComboBox->setCurrentItem(0);
  shadingComboBox->setEnabled( false );
  shadingSpinBox->setValue(0);
  shadingSpinBox->setEnabled( false );
  shadingcoldComboBox->setCurrentItem(0);
  shadingcoldComboBox->setEnabled( false );
  shadingcoldSpinBox->setValue(0);
  shadingcoldSpinBox->setEnabled( false );
//  tableCheckBox->setEnabled(false);
  patternComboBox->setEnabled(false);
  patternColourBox->setEnabled(false);
  repeatCheckBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  alphaSpinBox->setEnabled(false);

  lineTypeCbox->setEnabled( false );

  lineWidthCbox->setEnabled( false );

  lineintervalCbox->clear();
  lineintervalCbox->setEnabled( false );

  densityCbox->clear();
  densityCbox->setEnabled( false );

  vectorunitCbox->clear();
  vectorunitCbox->setEnabled( false );

  //extremeTypeCbox->clear();
  //extremeTypeCbox->setEnabled( false );

  //extremeSizeSpinBox->setValue(100);
  //extremeSizeSpinBox->setEnabled( false );

  //extremeRadiusSpinBox->setValue(100);
  //extremeRadiusSpinBox->setEnabled( false );

  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled( false );

  zeroLineCheckBox->setChecked( true );
  zeroLineCheckBox->setEnabled( false );

  valueLabelCheckBox->setChecked( true );
  valueLabelCheckBox->setEnabled( false );

  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled( false );

  zero1ComboBox->clear();
  zero1ComboBox->setEnabled( false );

  min1ComboBox->setEnabled(false);
  max1ComboBox->setEnabled(false);

  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->setEnabled(false);

  //undefMaskingCbox->clear();
  //undefMaskingCbox->setEnabled(false);

  //undefColourCbox->clear();
  //undefColourCbox->setEnabled(false);

  //undefLinewidthCbox->clear();
  //undefLinewidthCbox->setEnabled(false);

  //undefLinetypeCbox->clear();
  //undefLinetypeCbox->setEnabled(false);

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::disableFieldOptions returned"<<endl;
#endif
}


vector<miString> VcrossDialog::numberList( QComboBox* cBox, float number ){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::numberList called"<<endl;
#endif

  cBox->clear();

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

  QString qs;
  for (i=0; i<n; ++i) {
    cBox->addItem(QString(vnumber[i].cStr()));
  }

  cBox->setCurrentItem(nupdown);

  return vnumber;
}

miString VcrossDialog::baseList( QComboBox* cBox,
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

void VcrossDialog::selectedFieldboxClicked( QListWidgetItem * item  )
{
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::selectedFieldboxHighlighted called"<<endl;
#endif
  int index = selectedFieldbox->row(item);

  // may get here when there is none selected fields (the last is removed)
  if (index<0 || selectedFields.size()==0) return;

  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::selectedFieldboxHighlighted returned"<<endl;
#endif
  return;
}


void VcrossDialog::colorCboxActivated( int index ){
  if (index==0)
    updateFieldOptions("colour","off");
  else
    updateFieldOptions("colour",colourInfo[index-1].name);
}


void VcrossDialog::lineWidthCboxActivated( int index ){
  updateFieldOptions("linewidth",miString(index+1));
}


void VcrossDialog::lineTypeCboxActivated( int index ){
  updateFieldOptions("linetype",linetypes[index]);
}


void VcrossDialog::lineintervalCboxActivated( int index ){
  updateFieldOptions("line.interval",lineintervals[index]);
  // update the list (with selected value in the middle)
  float a= atof(lineintervals[index].c_str());
  lineintervals= numberList( lineintervalCbox, a);
}


void VcrossDialog::densityCboxActivated( int index ){
  if (index==0) updateFieldOptions("density","0");
  else  updateFieldOptions("density",densityCbox->currentText().toStdString());
}


void VcrossDialog::vectorunitCboxActivated( int index ){
  updateFieldOptions("vector.unit",vectorunit[index]);
  // update the list (with selected value in the middle)
  float a= atof(vectorunit[index].c_str());
  vectorunit= numberList( vectorunitCbox, a);
}


//void VcrossDialog::extremeTypeActivated(int index){
//  updateFieldOptions("extreme.type",extremeType[index]);
//}


//void VcrossDialog::extremeSizeChanged(int value){
//  miString str= miString( float(value)*0.01 );
//  updateFieldOptions("extreme.size",str);
//}


//void VcrossDialog::extremeRadiusChanged(int value){
//  miString str= miString( float(value)*0.01 );
//  updateFieldOptions("extreme.radius",str);
//}


void VcrossDialog::lineSmoothChanged(int value){
  miString str= miString( value );
  updateFieldOptions("line.smooth",str);
}


void VcrossDialog::labelSizeChanged(int value){
  miString str= miString( float(value)*0.01 );
  updateFieldOptions("label.size",str);
}


void VcrossDialog::hourOffsetChanged(int value){
  int n= selectedFieldbox->currentRow();
  selectedFields[n].hourOffset= value;
}


//void VcrossDialog::undefMaskingActivated(int index){
//  updateFieldOptions("undef.masking",miString(index));
//}


//void VcrossDialog::undefColourActivated( int index ){
//  updateFieldOptions("undef.colour",colourInfo[index].name);
//}


//void VcrossDialog::undefLinewidthActivated( int index ){
//  updateFieldOptions("undef.linewidth",linewidths[index]);
//}


//void VcrossDialog::undefLinetypeActivated( int index ){
//  updateFieldOptions("undef.linetype",linetypes[index]);
//}


void VcrossDialog::zeroLineCheckBoxToggled(bool on){
  if (on) updateFieldOptions("zero.line","1");
  else    updateFieldOptions("zero.line","0");
}


void VcrossDialog::valueLabelCheckBoxToggled(bool on){
  if (on) updateFieldOptions("value.label","1");
  else    updateFieldOptions("value.label","0");
}


void VcrossDialog::tableCheckBoxToggled(bool on){
  if (on) updateFieldOptions("table","1");
  else    updateFieldOptions("table","0");
}

void VcrossDialog::patternComboBoxToggled(int index){
  if(index == 0){
    updateFieldOptions("patterns","off");
  } else {
    updateFieldOptions("patterns",patternInfo[index-1].name);
  }
  updatePaletteString();
}

void VcrossDialog::patternColourBoxToggled(int index){
  if(index == 0){
    updateFieldOptions("patterncolour","remove");
  } else {
    updateFieldOptions("patterncolour",colourInfo[index-1].name);
  }
  updatePaletteString();
}


void VcrossDialog::repeatCheckBoxToggled(bool on){
  if (on) updateFieldOptions("repeat","1");
  else    updateFieldOptions("repeat","0");
}

void VcrossDialog::shadingChanged(){
  updatePaletteString();
}

void VcrossDialog::updatePaletteString(){

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

void VcrossDialog::alphaChanged(int index){
  updateFieldOptions("alpha",miString(index));
}

void VcrossDialog::zero1ComboBoxToggled(int index){
  if(!zero1ComboBox->currentText().isNull() ){
    miString str = zero1ComboBox->currentText().toStdString();
    updateFieldOptions("base",str);
    float a = atof(str.cStr());
    float b = lineintervalCbox->currentText().toInt();
    baseList(zero1ComboBox,a,b,true);
  }
}

void VcrossDialog::min1ComboBoxToggled(int index){
  if( index == 0 )
    updateFieldOptions("minvalue","off");
  else if(!min1ComboBox->currentText().isNull() ){
    miString str = min1ComboBox->currentText().toStdString();
    updateFieldOptions("minvalue",str);
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(min1ComboBox,a,b,true);
  }
}

void VcrossDialog::max1ComboBoxToggled(int index){
  if( index == 0 )
    updateFieldOptions("maxvalue","off");
  else if(!max1ComboBox->currentText().isNull() ){
    miString str = max1ComboBox->currentText().toStdString();
    updateFieldOptions("maxvalue", max1ComboBox->currentText().toStdString());
    float a = atof(str.cStr());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(max1ComboBox,a,b,true);
  }
}
void VcrossDialog::updateFieldOptions(const miString& name,
				      const miString& value,
				      int valueIndex) {
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::updateFieldOptions  name= " << name
			           << "  value= " << value <<endl;
#endif

  if (currentFieldOpts.empty()) return;

  int n= selectedFieldbox->currentRow();

  if(value == "remove")
    cp->removeValue(vpcopt,name);
  else
    cp->replaceValue(vpcopt,name,value,valueIndex);

  currentFieldOpts= cp->unParse(vpcopt);

  selectedFields[n].fieldOpts= currentFieldOpts;

  // update private settings
  miString field= selectedFields[n].field;
  fieldOptions[field]= currentFieldOpts;
  changedOptions[field]= true;
}


vector<miString> VcrossDialog::getOKString(){

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::getOKString called"<<endl;
#endif

  if (historyOkButton->isEnabled()) historyOk();

//#################################################################
//  cerr<<"VcrossDialog::getOKString  selectedFields.size()= "
//      <<selectedFields.size()<<endl;
//#################################################################

  vector<miString> vstr;
  if (selectedFields.size()==0) return vstr;

  vector<miString> hstr;

  int n= selectedFields.size();

  for (int i=0; i<n; i++) {

    ostringstream ostr;

    ostr << "model=" << selectedFields[i].model
         << " field=" <<  selectedFields[i].field;
    //#############################################################
    //cerr << "OK: " << ostr.str() << endl;
    //#############################################################

    if (selectedFields[i].hourOffset!=0)
      ostr << " hour.offset=" << selectedFields[i].hourOffset;

    ostr << " " << selectedFields[i].fieldOpts;

    miString str;

    str= "VCROSS " + ostr.str();

    // the History string
    hstr.push_back(ostr.str());

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


void VcrossDialog::historyBack() {
  showHistory(-1);
}


void VcrossDialog::historyForward() {
  showHistory(1);
}


void VcrossDialog::showHistory(int step) {

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

    // enable model/field boxes
    deleteAllSelected();

  } else {

    if (!historyOkButton->isEnabled()) {
      deleteAllSelected(); // some cleanup before browsing history
      fieldbox->setEnabled(false);
      selectedFieldbox->setEnabled(false);
    }

    selectedFieldbox->clear();

    vector<miString> vstr;
    int n= commandHistory[historyPos].size();

    for (int i=0; i<n; i++) {
      vector<ParsedCommand> vpc= cp->parse( commandHistory[historyPos][i] );
      int m= vpc.size();
      miString str;
      bool modelfound= false, fieldfound= false;
      for (int j=0; j<m; j++) {
	if (vpc[j].idNumber>0) {
	  if (vpc[j].key=="model") modelfound= true;
	  if (vpc[j].key=="field") fieldfound= true;
	  if (vpc[j].key=="model") str= vpc[j].allValue;
	  if (vpc[j].key=="field") str+=(" " + vpc[j].allValue);
	}
      }
      if (modelfound && fieldfound)
	vstr.push_back(str);
    }

    int nvstr= vstr.size();
    if (nvstr>0) {
      for (int i=0; i<nvstr; i++)
	  selectedFieldbox->addItem(QString(vstr[i].c_str()));
      deleteAll->setEnabled(true);
    }

    highlightButton(historyOkButton,true);
  }

  historyBackButton->setEnabled(historyPos>-1);
  historyForwardButton->setEnabled(historyPos<hs);
}


void VcrossDialog::historyOk() {
#ifdef DEBUGPRINT
  cerr << "VcrossDialog::historyOk()" << endl;
#endif

  if (historyPos<0 || historyPos>=int(commandHistory.size())) {
    vector<miString> vstr;
    putOKString(vstr,false,false);
  } else {
    putOKString(commandHistory[historyPos],false,false);
  }
}


void VcrossDialog::putOKString(const vector<miString>& vstr,
			      bool vcrossPrefix, bool checkOptions)
{
#ifdef DEBUGPRINT
  cerr << "VcrossDialog::putOKString starts" << endl;
#endif

  deleteAllSelected();

  int nc= vstr.size();

  if (nc==0)
    return;

  miString fields2_model,model,field,fOpts;
  int ic,i,j,m,n,hourOffset;
  vector<miString> fields2;
  int nf2= 0;
  vector<ParsedCommand> vpc;

  for (ic=0; ic<nc; ic++) {

//######################################################################
//    cerr << "P.OK>> " << vstr[ic] << endl;
//######################################################################
    if (checkOptions) {
      miString str= checkFieldOptions(vstr[ic],vcrossPrefix);
      if (str.empty()) continue;
      vpc= cp->parse( str );
    } else {
      vpc= cp->parse( vstr[ic] );
    }

    model.clear();
    field.clear();
    fOpts.clear();
    hourOffset= 0;

    m=vpc.size();
    for (j=0; j<m; j++) {
//######################################################################
//cerr << "   " << j << " : " << vpc[j].key << " = " << vpc[j].strValue[0]
//     << "   " << vpc[j].allValue << endl;
//######################################################################
      if      (vpc[j].key=="model")      model=      vpc[j].strValue[0];
      else if (vpc[j].key=="field")      field=      vpc[j].strValue[0];
      else if (vpc[j].key=="hour.offset")hourOffset= vpc[j].intValue[0];
      else if (vpc[j].key!="unknown") {
	if (fOpts.length()>0) fOpts+=" ";
	fOpts+=(vpc[j].key + "=" + vpc[j].allValue);
      }
    }
//######################################################################
//    cerr << " ->" << model << " " << field << endl;
//######################################################################

    if (model!=fields2_model) {
      fields2= vcrossm->getFieldNames(model);
      fields2_model= model;
      nf2= fields2.size();
    }

    j= 0;
    while (j<nf2 && field!=fields2[j]) j++;

    if (j<nf2) {
      SelectedField sf;
      sf.model=  model;
      sf.field=  field;
      sf.hourOffset= hourOffset;
      sf.fieldOpts= fOpts;

      selectedFields.push_back(sf);

      miString str= model + " " + field;
      QString qstr= str.c_str();
      selectedFieldbox->addItem(qstr);
    }
  }

  m= selectedFields.size();

  if (m>0) {
    if (fields.size()>0) {
      model= models[modelbox->currentRow()];
      n= fields.size();
      bool change= false;
      for (i=0; i<m; i++) {
        if (selectedFields[i].model==model) {
          j=0;
          while (j<n && fields[j]!=selectedFields[i].field) j++;
          if (j<n) {
            countSelected[j]++;
            fieldbox->item(j)->setSelected(true);
	    change= true;
          }
        }
      }
      if (change)     fieldboxChanged(fieldbox->currentItem());
    }
    i= 0;
    selectedFieldbox->setCurrentRow(i);
    selectedFieldbox->item(i)->setSelected(true);
    enableFieldOptions();
  }

#ifdef DEBUGPRINT
  cerr << "VcrossDialog::putOKString finished" << endl;
#endif
}


vector<miString> VcrossDialog::writeLog() {

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

  map<miString,miString>::iterator p, pend= fieldOptions.end();

  for (p=fieldOptions.begin(); p!=pend; p++) {
    if (changedOptions[p->first])
      vstr.push_back( p->first + " " + p->second );
  }
  vstr.push_back("================");

  return vstr;
}


void VcrossDialog::readLog(const vector<miString>& vstr,
			   const miString& thisVersion,
			   const miString& logVersion) {

  miString str,fieldname,fopts;
  vector<miString> hstr;
  size_t pos,end;

  map<miString,miString>::iterator pfopt, pfend= fieldOptions.end();
  int nopt,nlog,i,j;
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
      str= checkFieldOptions(vstr[ivstr],false);
      if (str.exists())
	hstr.push_back(str);
    }
  }
  ivstr++;

  // field options
  // (do not destroy any new options in the program,
  //  and get rid of old unused options)

  for (; ivstr<nvstr; ivstr++) {
    if (vstr[ivstr].substr(0,4)=="====") break;
    str= vstr[ivstr];
    end= str.length();
    pos= str.find_first_of(' ');
    if (pos>0 && pos<end) {
      fieldname=str.substr(0,pos);
      pos++;
      fopts= str.substr(pos,end-pos);

      pfopt= fieldOptions.find(fieldname);
      if (pfopt!=pfend) {
        vector<ParsedCommand> vpopt= cp->parse( pfopt->second );
        vector<ParsedCommand> vplog= cp->parse( fopts );
	nopt= vpopt.size();
	nlog= vplog.size();
	changed= false;
	for (i=0; i<nopt; i++) {
	  j=0;
	  while (j<nlog && vplog[j].key!=vpopt[i].key) j++;
	  if (j<nlog) {
	    // there is no option with variable no. of values, YET...
	    if (vplog[j].allValue!=vpopt[i].allValue &&
	        vplog[j].strValue.size()==vpopt[i].strValue.size()) {
	      //###########################################################
	      //cerr << "    D.CH: " << fieldname << " "
              //                     << vpopt[j].key << " "
              //                     << vpopt[j].allValue << " -> "
              //                     << vplog[i].allValue << endl;
	      //###########################################################
	      cp->replaceValue(vpopt[j],vplog[i].allValue,-1);
	      changed= true;
	    }
	  }
	}
	for (i=0; i<nlog; i++) {
	  j=0;
	  while (j<nopt && vpopt[j].key!=vplog[i].key) j++;
	  if (j==nopt) {
	    cp->replaceValue(vpopt,vplog[i].key,vplog[i].allValue);
	  }
	}
        if (changed) {
	  //###########################################################
	  //cerr << "D.OLD: " << fieldname << " " << pfopt->second << endl;
	  //cerr << "D.LOG: " << fieldname << " " << fopts << endl;
	  //###########################################################
	  pfopt->second= cp->unParse(vpopt);
	  //###########################################################
	  //cerr << "D.NEW: " << fieldname << " " << pfopt->second << endl;
	  //###########################################################
	  changedOptions[fieldname]= true;
	}
	//###########################################################
	//else cerr << "D.OK:  " << fieldname << " " << pfopt->second << endl;
	//###########################################################
      }
      //###########################################################
      //else cerr << "D.UNKNOWN: " << fieldname << " " << fopts << endl;
      //###########################################################
    }
  }
  ivstr++;

  historyPos= commandHistory.size();

  historyBackButton->setEnabled(historyPos>0);
  historyForwardButton->setEnabled(false);
}


miString VcrossDialog::checkFieldOptions(const miString& str, bool vcrossPrefix)
{
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::checkFieldOptions"<<endl;
#endif

  miString newstr;

  map<miString,miString>::iterator pfopt;
  miString fieldname;
  int nopt,i,j;

  vector<ParsedCommand> vplog= cp->parse( str );
  int nlog= vplog.size();
  int first= (vcrossPrefix) ? 1 : 0;

  if (nlog>=2+first && vplog[first].key=="model"
	            && vplog[first+1].key=="field") {
    fieldname= vplog[first+1].allValue;
    pfopt= setupFieldOptions.find(fieldname);
    if (pfopt!=setupFieldOptions.end()) {
      vector<ParsedCommand> vpopt= cp->parse( pfopt->second );
      nopt= vpopt.size();
//##################################################################
//    cerr << "    nopt= " << nopt << "  nlog= " << nlog << endl;
//    for (j=0; j<nlog; j++)
//	cerr << "        log " << j << " : id " << vplog[j].idNumber
//	     << "  " << vplog[j].key << " = " << vplog[j].allValue << endl;
//##################################################################
      if (vcrossPrefix) newstr= "VCROSS ";
      for (i=first; i<nlog; i++) {
	if (vplog[i].idNumber==1)
	  newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
      for (j=0; j<nopt; j++) {
	i=0;
	while (i<nlog && vplog[i].key!=vpopt[j].key) i++;
	if (i<nlog) {
	  // there is no option with variable no. of values, YET...
	  if (vplog[i].allValue!=vpopt[j].allValue &&
              vplog[i].strValue.size()==vpopt[j].strValue.size())
	    cp->replaceValue(vpopt[j],vplog[i].allValue,-1);
	}
      }
      newstr+= " ";
      newstr+= cp->unParse(vpopt);
      if (vcrossPrefix) {
	// from quickmenu, keep "forecast.hour=..." and "forecast.hour.loop=..."
        for (i=2+first; i<nlog; i++) {
	  if (vplog[i].idNumber==4 || vplog[i].idNumber==-1)
	    newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
        }
      }
      for (i=2+first; i<nlog; i++) {
	if (vplog[i].idNumber==3)
	  newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
    }
  }
//##################################################################
//  cerr<<"cF str:    "<<str<<endl;
//  cerr<<"cF newstr: "<<newstr<<endl;
//##################################################################

  return newstr;
}


void VcrossDialog::deleteSelected(){
#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::deleteSelected called"<<endl;
#endif

  int index = selectedFieldbox->currentRow();

  int i, ns= selectedFields.size() - 1;

  if (index<0 || index>ns) return;

  int indexF= -1;
  if (fields.size()>0) {
    int indexM = modelbox->currentRow();
    int n=       fields.size();
    if (selectedFields[index].model==models[indexM]) {
      i=0;
      while (i<n && fields[i]!=selectedFields[index].field) i++;
      if (i<n) indexF= i;
    }
  }

  if (indexF>=0) {
    fieldbox->setCurrentRow(indexF);
    fieldbox->item(indexF)->setSelected(false);
    fieldboxChanged(fieldbox->currentItem());
  } else {
    selectedFieldbox->takeItem(index);
    for (i=index; i<ns; i++)
      selectedFields[i]= selectedFields[i+1];
    selectedFields.pop_back();
  }

  if (selectedFields.size()>0) {
    if (index>=int(selectedFields.size())) index= selectedFields.size()-1;
    selectedFieldbox->setCurrentRow( index );
    selectedFieldbox->item(index)->setSelected( true );
    enableFieldOptions();
  } else {
    disableFieldOptions();
  }

#ifdef DEBUGPRINT
  cerr<<"VcrossDialog::deleteSelected returned"<<endl;
#endif
  return;
}


void VcrossDialog::deleteAllSelected(){
#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::deleteAllSelected() called"<<endl;
#endif

  // calls:
  //  1: button clicked (while selecting fields)
  //  2: button clicked when history is shown (get out of history)
  //  3: from putOKString when accepting history command
  //  4: from putOKString when inserting QuickMenu command
  //     (possibly when dialog in "history" state...)

  int n= fieldbox->count();
  for (int i=0; i<n; i++)
    countSelected[i]= 0;

  if (n>0) {
    fieldbox->blockSignals(true);
    fieldbox->clearSelection();
    fieldbox->blockSignals(false);
  }

  selectedFields.clear();
  selectedFieldbox->clear();

  if (historyOkButton->isEnabled()) {
    // terminate browsing history
    if (fieldbox->count()>0) fieldbox->setEnabled(true);
    selectedFieldbox->setEnabled(true);
    highlightButton(historyOkButton,false);
  }

  disableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::deleteAllSelected() returned"<<endl;
#endif
  return;
}


void VcrossDialog::copySelectedField(){
#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::copySelectedField called"<<endl;
#endif

  if (selectedFieldbox->count()==0) return;

  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();

  if (fields.size()>0) {
    int n=         fields.size();
    int i=0;
    while (i<n && fields[i]!=selectedFields[index].field) i++;
    if (i<n) countSelected[i]++;
  }

  selectedFields.push_back(selectedFields[index]);
  selectedFields[n].hourOffset= 0;

  selectedFieldbox->addItem(selectedFieldbox->item(index)->text());
  selectedFieldbox->setCurrentRow(n);
  selectedFieldbox->item(n)->setSelected( true );
  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::copySelectedField returned"<<endl;
#endif
  return;
}


void VcrossDialog::changeModel(){
#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::changeModel called"<<endl;
#endif

  if (selectedFieldbox->count()==0) return;
  int i, n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();
  if (index<0 || index>=n) return;

  if (modelbox->count()==0) return;

  int indexM= modelbox->currentRow();
  if (indexM<0) return;

  miString oldmodel= selectedFields[index].model;
  miString    model= models[indexM];

  fieldbox->blockSignals(true);

  int j, nf= fields.size();

  for (i=0; i<n; i++) {
    if (selectedFields[i].model==oldmodel) {
      // check if field exists for the new model
      j= 0;
      while (j<nf && selectedFields[i].field!=fields[j]) j++;
      if (j<nf) {
	countSelected[j]++;
	if (countSelected[j]==1) {
	  countSelected[j]++;
	  fieldbox->setCurrentRow( j );
	  fieldbox->item(j)->setSelected( true );
	}
        selectedFields[i].model= model;
        miString str= model + " " + fields[j];
        QString qstr= str.c_str();
        selectedFieldbox->item(i)->setText(qstr);
      }
      // if not ok, then not changing the model (at least yet...)
    }
  }

  fieldbox->blockSignals(false);

  selectedFieldbox->setCurrentRow( index );
  selectedFieldbox->item(index)->setSelected( true );
  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::changeModel returned"<<endl;
#endif
  return;
}


void VcrossDialog::upField() {

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();
  if (index<1 || index>=n) return;

  SelectedField sf= selectedFields[index];
  selectedFields[index]= selectedFields[index-1];
  selectedFields[index-1]= sf;

  QString qstr1= selectedFieldbox->item(index-1)->text();
  QString qstr2= selectedFieldbox->item(index)->text();
  selectedFieldbox->item(index-1)->setText(qstr2);
  selectedFieldbox->item(index)->setText(qstr1);

  selectedFieldbox->setCurrentRow( index-1 );
}


void VcrossDialog::downField() {

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();
  if (index<0 || index>=n-1) return;

  SelectedField sf= selectedFields[index];
  selectedFields[index]= selectedFields[index+1];
  selectedFields[index+1]= sf;

  QString qstr1= selectedFieldbox->item(index)->text();
  QString qstr2= selectedFieldbox->item(index+1)->text();
  selectedFieldbox->item(index)->setText(qstr2);
  selectedFieldbox->item(index+1)->setText(qstr1);

  selectedFieldbox->setCurrentRow( index+1 );
}


void VcrossDialog::resetOptions() {

  if (selectedFieldbox->count()==0) return;
  int n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();
  if (index<0 || index>=n) return;

  miString field= selectedFields[index].field;
  selectedFields[index].fieldOpts= setupFieldOptions[field];
  selectedFields[index].hourOffset= 0;
  enableFieldOptions();
}


void VcrossDialog::applyClicked(){
  if (historyOkButton->isEnabled()) historyOk();
  vector<miString> vstr= getOKString();
  bool modelChange= vcrossm->setSelection(vstr);
  emit VcrossDialogApply(modelChange);
}


void VcrossDialog::applyhideClicked(){
  if (historyOkButton->isEnabled()) historyOk();
  vector<miString> vstr= getOKString();
  bool modelChange= vcrossm->setSelection(vstr);
  emit VcrossDialogHide();
  emit VcrossDialogApply(modelChange);
}


void VcrossDialog::hideClicked(){
  emit VcrossDialogHide();
}


void VcrossDialog::helpClicked(){
  emit showsource("ug_verticalcrosssections.html");
}


void VcrossDialog::closeEvent( QCloseEvent* e) {
  emit VcrossDialogHide();
}

void VcrossDialog::highlightButton(QPushButton* button, bool on)
{
  if (button->isEnabled()!=on) {
    if (on) {
      button->setPalette( QPalette(QColor(255,0,0),QColor(192,192,192)) );
      button->setEnabled( true );
    } else {
      button->setPalette( QPalette(this->palette()) );
      button->setEnabled( false );
    }
  }
}


void VcrossDialog::cleanup()
{
#ifdef DEBUGPRINT
  cerr<<" VcrossDialog::cleanup() called"<<endl;
#endif
  deleteAllSelected();
  fieldbox->clear();
  fields.clear();

  // one way of unselecting model (in a single selection box)...
  modelbox->clearSelection();
  modelbox->clear();
  int n= models.size();
  if (n>0) {
    for (int i=0; i<n; i++)
	modelbox->addItem(QString(models[i].c_str()));
  }
  fieldbox->setFocus();
}
