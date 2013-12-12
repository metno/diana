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

//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtVcrossDialog.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#ifdef USE_VCROSS_V2
#include "vcross_v2/diVcrossManager.h"
#else
#include "vcross_v1/diVcross1Manager.h"
#endif

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <qcombobox.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
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
#include <QApplication>

#include <cmath>

#include "up20x20.xpm"
#include "down20x20.xpm"
#include "up12x12.xpm"
#include "down12x12.xpm"

#define MILOGGER_CATEGORY "diana.VcrossDialog"
#include <miLogger/miLogging.h>

VcrossDialog::VcrossDialog( QWidget* parent, VcrossManager* vm )
: QDialog(parent), vcrossm(vm)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle( tr("Vertical Crossections"));

  m_advanced= false;

  //  FIRST INITIALISATION OF STATE

  historyPos= -1;

  models = vcrossm->getAllModels();

  // get all fieldnames from setup file
  //#################  fieldnames = vcrossm->getAllFieldNames();
  //#################################################################
  //  for (i=0; i<fieldnames.size(); i++)
  //    METLIBS_LOG_DEBUG("   field "<<setw(3)<<i<<" : "<<fieldnames[i]);
  //#################################################################

  // get all field plot options from setup file
  fieldOptions = vcrossm->getAllFieldOptions();

  std::map<std::string,std::string>::iterator pfopt, pfend= fieldOptions.end();
  for (pfopt=fieldOptions.begin(); pfopt!=pfend; pfopt++)
    changedOptions[pfopt->first]= false;
  //#################################################################
  //  for (pfopt=fieldOptions.begin(); pfopt!=pfend; pfopt++)
  //    METLIBS_LOG_DEBUG(pfopt->first << "   " << pfopt->second);
  //#################################################################

  //  END FIRST INNITIALISATION OF STATE

  // Colours
  colourInfo = Colour::getColourInfo();
  nr_colors    = colourInfo.size();

  csInfo      = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();

  // linewidths
  nr_linewidths= 12;

  // linetypes
  linetypes = Linetype::getLinetypeNames();
  nr_linetypes= linetypes.size();

  // density (of arrows etc, 0=automatic)
  densityStringList << "Auto";
  for (int i=0;  i<10; i++) {
    densityStringList << QString::number(i);
  }
  for (int i=10;  i<60; i+=10) {
    densityStringList << QString::number(i);
  }
  densityStringList << QString::number(100);

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
  cp->addKey("extreme.type",   "",0,CommandParser::cmdString);
  cp->addKey("extreme.size",   "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.limits", "",0,CommandParser::cmdString);
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

  const int n = models.size();
  for (int i=0; i<n; i++)
    modelbox->addItem(QString::fromStdString(models[i]));

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

  const int wbutton = hbutton-4;

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
  lineintervalCbox=  new QComboBox( this );
  lineintervalCbox->setEnabled( false );

  connect( lineintervalCbox, SIGNAL( activated(int) ),
      SLOT( lineintervalCboxActivated(int) ) );

  // density
  densitylabel= new QLabel( tr("Density"), this );
  densityCbox=  new QComboBox( this );
  densityCbox->setEnabled( false );

  connect( densityCbox, SIGNAL( activated(int) ),
      SLOT( densityCboxActivated(int) ) );

  // vectorunit
  vectorunitlabel= new QLabel( tr("Unit"), this );
  vectorunitCbox=  new QComboBox(this );

  connect( vectorunitCbox, SIGNAL( activated(int) ),
      SLOT( vectorunitCboxActivated(int) ) );

  // help
  fieldhelp = NormalPushButton( tr("Help"), this );
  connect( fieldhelp, SIGNAL(clicked()), SLOT(helpClicked()));

  // allTimeStep
  allTimeStepButton = 0;
  //allTimeStepButton = NormalPushButton( tr("All times"), this );
  //allTimeStepButton->setToggleButton(true);
  //allTimeStepButton->setChecked(false);

  // advanced
  advanced= new ToggleButton(this, tr("<<Less"), tr("More>>"));
  advanced->setChecked(false);
  connect( advanced, SIGNAL(toggled(bool)), SLOT(advancedToggled(bool)));

  // hide
  fieldhide = NormalPushButton( tr("Hide"), this );
  connect( fieldhide, SIGNAL(clicked()), SLOT( hideClicked()));

  // applyhide
  fieldapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( fieldapplyhide, SIGNAL(clicked()), SLOT( applyhideClicked()));

  // apply
  fieldapply = NormalPushButton( tr("Apply"), this );
  fieldapply->setDefault( true );
  connect( fieldapply, SIGNAL(clicked()), SLOT( applyClicked()));

  // layout
  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->addWidget( modellabel );
  v1layout->addWidget( modelbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( fieldlabel );
  v1layout->addWidget( fieldbox );
  v1layout->addSpacing( 5 );
  v1layout->addWidget( selectedFieldlabel );
  v1layout->addWidget( selectedFieldbox );

  QVBoxLayout* h2layout= new QVBoxLayout();
  h2layout->addWidget( upFieldButton );
  h2layout->addWidget( downFieldButton );
  h2layout->addWidget( resetOptionsButton );
  h2layout->addStretch(1);

  QHBoxLayout* v1h4layout = new QHBoxLayout();
  v1h4layout->addWidget( Delete );
  v1h4layout->addWidget( copyField );

  QHBoxLayout* vxh4layout = new QHBoxLayout();
  vxh4layout->addWidget( deleteAll );
  vxh4layout->addWidget( changeModelButton );

  QVBoxLayout* v3layout= new QVBoxLayout();
  v3layout->addLayout( v1h4layout );
  v3layout->addLayout( vxh4layout );

  QHBoxLayout* v1h5layout= new QHBoxLayout();
  v1h5layout->addWidget( historyBackButton );
  v1h5layout->addWidget( historyForwardButton );

  QVBoxLayout* v4layout= new QVBoxLayout();
  v4layout->addLayout( v1h5layout );
  v4layout->addWidget( historyOkButton, 1 );

  QHBoxLayout* h3layout= new QHBoxLayout();
  h3layout->addLayout( v3layout );
  h3layout->addLayout( v4layout );

  //optlayout = new QGridLayout( 7, 2, 1 );
  QGridLayout* optlayout = new QGridLayout();
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

  QHBoxLayout* h4layout = new QHBoxLayout();
  h4layout->addLayout( h2layout );
  h4layout->addLayout( optlayout );

  QHBoxLayout* h5layout = new QHBoxLayout();
  h5layout->addWidget( fieldhelp );
  //h5layout->addWidget( allTimeStepButton );
  h5layout->addWidget( advanced );

  QHBoxLayout* h6layout = new QHBoxLayout();
  h6layout->addWidget( fieldhide );
  h6layout->addWidget( fieldapplyhide );
  h6layout->addWidget( fieldapply );

  QVBoxLayout* v6layout= new QVBoxLayout();
  v6layout->addLayout( h5layout );
  v6layout->addLayout( h6layout );

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout( this );

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
  METLIBS_LOG_DEBUG("VcrossDialog::ConstructorCernel returned");
#endif
}


void VcrossDialog::toolTips()
{
  upFieldButton->setToolTip(tr("move selected field up"));
  downFieldButton->setToolTip(tr("move selected field down" ));
  Delete->setToolTip(tr("remove selected field"));
  deleteAll->setToolTip(tr("remove all selected fields") );
  copyField->setToolTip(tr("copy field") );
  resetOptionsButton->setToolTip(tr("reset plot layout"));
  changeModelButton->setToolTip(tr("change model/modeltime"));
  historyBackButton->setToolTip(tr("history backward"));
  historyForwardButton->setToolTip(tr("history forward"));
  historyOkButton->setToolTip(tr("use current history"));
  extremeSizeSpinBox->setToolTip(tr("Size of min/max marker"));
  extremeLimitMinComboBox->setToolTip(tr("Find min/max value above this vertical level (unit hPa)"));
  extremeLimitMaxComboBox->setToolTip(tr("Find min/max value below this vertical level (unit hPa)"));
  //allTimeStepButton,    tr("all times / union of times") );
}

void VcrossDialog::advancedToggled(bool on)
{
  METLIBS_LOG_DEBUG("VcrossDialog::advancedToggled  on= " << on);
  this->showExtension(on);
  m_advanced= on;
}


void VcrossDialog::CreateAdvanced()
{
  METLIBS_LOG_DEBUG("VcrossDialog::CreateAdvanced");

  advFrame= new QWidget(this);

  // mark min/max values
  extremeValueCheckBox= new QCheckBox(tr("Min/max values"), advFrame);
  extremeValueCheckBox->setChecked( false );
  extremeValueCheckBox->setEnabled( false );
  connect( extremeValueCheckBox, SIGNAL( toggled(bool) ),
      SLOT( extremeValueCheckBoxToggled(bool) ) );

  //QLabel* extremeTypeLabelHead= new QLabel( "Min,max", advFrame );
  QLabel* extremeSizeLabel= new QLabel( tr("Size"),  advFrame );
  extremeSizeSpinBox= new QSpinBox( advFrame );
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled( false );
  connect( extremeSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( extremeSizeChanged(int) ) );

  extremeLimits<<"Off";
  QString qstr;
  for (int i=0; i<MetNo::Constants::nLevelTable; i++) {
    qstr.setNum(MetNo::Constants::pLevelTable[i]);
    extremeLimits<<qstr;
  }

  QLabel* extremeLimitMinLabel = new QLabel(tr("Level low"));
  extremeLimitMinComboBox= new QComboBox( advFrame );
  extremeLimitMinComboBox->addItems(extremeLimits);
  extremeLimitMinComboBox->setEnabled(false);
  connect( extremeLimitMinComboBox, SIGNAL( activated(int) ),
      SLOT( extremeLimitsChanged() ) );

  QLabel* extremeLimitMaxLabel = new QLabel(tr("Level high"));
  extremeLimitMaxComboBox= new QComboBox( advFrame );
  extremeLimitMaxComboBox->addItems(extremeLimits);
  extremeLimitMaxComboBox->setEnabled(false);
  connect( extremeLimitMaxComboBox, SIGNAL( activated(int) ),
      SLOT( extremeLimitsChanged() ) );
  QGridLayout* extremeLayout = new QGridLayout();
  extremeLayout->addWidget(extremeSizeLabel,        0, 0);
  extremeLayout->addWidget(extremeLimitMinLabel,    0, 1);
  extremeLayout->addWidget(extremeLimitMaxLabel,    0, 2);
  extremeLayout->addWidget(extremeSizeSpinBox,      1, 0);
  extremeLayout->addWidget(extremeLimitMinComboBox, 1, 1);
  extremeLayout->addWidget(extremeLimitMaxComboBox, 1,2);

  // line smoothing
  QLabel* lineSmoothLabel= new QLabel( tr("Smooth lines"), advFrame );
  lineSmoothSpinBox= new QSpinBox( advFrame );
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled( false );
  connect( lineSmoothSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( lineSmoothChanged(int) ) );

  QLabel* labelSizeLabel= new QLabel( tr("Digit size"),  advFrame );
  labelSizeSpinBox= new QSpinBox( advFrame );
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(399);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled( false );
  connect( labelSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( labelSizeChanged(int) ) );

  QLabel* hourOffsetLabel= new QLabel( tr("Time offset"),  advFrame );
  hourOffsetSpinBox= new QSpinBox( advFrame );
  hourOffsetSpinBox->setMinimum(-72);
  hourOffsetSpinBox->setMaximum(72);
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

  shadingSpinBox = new QSpinBox(advFrame);
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(99);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  shadingSpinBox->setEnabled(false);
  connect( shadingSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldComboBox=  PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString() );
  connect( shadingcoldComboBox, SIGNAL( activated(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldSpinBox = new QSpinBox(advFrame);
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(99);
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
  alphaSpinBox = new QSpinBox(advFrame);
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
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
  // a separator
  QFrame* advSep= new QFrame( advFrame );
  advSep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  advSep->setLineWidth( 5 );
  QFrame* advSep3= new QFrame( advFrame );
  advSep3->setFrameStyle( QFrame::HLine | QFrame::Raised );
  advSep3->setLineWidth( 5 );

  QVBoxLayout *adv1Layout = new QVBoxLayout();
  int space= 6;
  adv1Layout->addStretch();
  adv1Layout->addWidget(extremeValueCheckBox);
  adv1Layout->addSpacing(space);
  adv1Layout->addLayout(extremeLayout );
  adv1Layout->addSpacing(space);
  adv1Layout->addWidget(advSep3);
  adv1Layout->addSpacing(space);
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


  QFrame* advSep2= new QFrame( advFrame );
  advSep2->setFrameStyle( QFrame::HLine | QFrame::Raised );
  advSep2->setLineWidth( 5 );

  QGridLayout* adv2Layout = new QGridLayout();
  adv2Layout->addWidget(advSep2,               0, 0,1,3);
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

  QVBoxLayout *advLayout = new QVBoxLayout();
  advLayout->addLayout(adv1Layout);
  advLayout->addLayout(adv2Layout);

  QHBoxLayout *hLayout = new QHBoxLayout( advFrame);

  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);
}


void VcrossDialog::modelboxClicked(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();

  int index = modelbox->row(item);
  QApplication::setOverrideCursor( Qt::WaitCursor );
  fields= vcrossm->getFieldNames(models[index]);
  QApplication::restoreOverrideCursor();

  //###  fieldbox->clearSelection();
  if (fieldbox->count()>0) {
    fieldbox->clearSelection();
    fieldbox->clear();
  }

  int nf= fields.size();

  if (nf<1) return;

  fieldbox->blockSignals(true);

  for (int i=0; i<nf; i++)
    fieldbox->addItem(QString::fromStdString(fields[i]));
  fieldbox->setEnabled( true );

  countSelected.resize(nf);
  for (int i=0; i<nf; ++i) countSelected[i]= 0;

  const int n= selectedFields.size();
  for (int i=0; i<n; ++i) {
    if (selectedFields[i].model==models[index]) {
      int j=0;
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
}


void VcrossDialog::fieldboxChanged(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();

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

      std::map<std::string,std::string>::iterator pfopt;
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
}


void VcrossDialog::enableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  disableFieldOptions();

  float e;
  int   index, lastindex, nc, i;

  index= selectedFieldbox->currentRow();
  lastindex= selectedFields.size()-1;

  if (index<0 || index>lastindex) {
    METLIBS_LOG_ERROR("PROGRAM ERROR.1 in VcrossDialog::enableFieldOptions");
    METLIBS_LOG_ERROR("       index,selectedFields.size: "
        << index << " " << selectedFields.size());
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
  //  METLIBS_LOG_DEBUG("fieldOpts: "<<selectedFields[index].fieldOpts);
  //###############################################################################

  if (selectedFields[index].fieldOpts==currentFieldOpts) return;

  currentFieldOpts= selectedFields[index].fieldOpts;

  //###############################################################################
  //  METLIBS_LOG_DEBUG("VcrossDialog::enableFieldOptions: "
  //       << fieldnames[selectedFields[index].fieldnumber]);
  //  METLIBS_LOG_DEBUG("             " << selectedFields[index].fieldOpts);
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
    METLIBS_LOG_DEBUG("VcrossDialog::enableFieldOptions: "
         << fieldnames[selectedFields[index].fieldnumber]);
    METLIBS_LOG_DEBUG("             " << selectedFields[index].fieldOpts);
    for (j=0; j<n; j++) {
      METLIBS_LOG_DEBUG("  parse " << j << " : key= " << vpcopt[j].key
	   << "  idNumber= " << vpcopt[j].idNumber);
      METLIBS_LOG_DEBUG("            allValue: " << vpcopt[j].allValue);
      for (k=0; k<vpcopt[j].strValue.size(); k++)
        METLIBS_LOG_DEBUG("               " << k << "    strValue: " << vpcopt[j].strValue[k]);
      for (k=0; k<vpcopt[j].floatValue.size(); k++)
        METLIBS_LOG_DEBUG("               " << k << "  floatValue: " << vpcopt[j].floatValue[k]);
      for (k=0; k<vpcopt[j].intValue.size(); k++)
        METLIBS_LOG_DEBUG("               " << k << "    intValue: " << vpcopt[j].intValue[k]);
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
    if(miutil::to_lower(vpcopt[nc].allValue) == "off" ||
        miutil::to_lower(vpcopt[nc].allValue) == "av" ){
      updateFieldOptions("colour","off");
      colorCbox->setCurrentIndex(0);
    } else {
      while (i<nr_colors
          && miutil::to_lower(vpcopt[nc].allValue)!=colourInfo[i].name) i++;
      if (i==nr_colors) i=0;
      updateFieldOptions("colour",colourInfo[i].name);
      colorCbox->setCurrentIndex(i+1);
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
    std::vector<std::string> tokens = miutil::split(vpcopt[nc].allValue,",");
    std::vector<std::string> stokens = miutil::split(tokens[0],";");
    if(stokens.size()==2)
      shadingSpinBox->setValue(atoi(stokens[1].c_str()));
    else
      shadingSpinBox->setValue(0);
    int nr_cs = csInfo.size();
    std::string str;
    i=0;
    while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
    if (i==nr_cs) {
      str = "off";
      shadingComboBox->setCurrentIndex(0);
      shadingcoldComboBox->setCurrentIndex(0);
    }else {
      str = tokens[0];
      shadingComboBox->setCurrentIndex(i+1);
    }
    if(tokens.size()==2){
      std::vector<std::string> stokens = miutil::split(tokens[1],";");
      if(stokens.size()==2)
        shadingcoldSpinBox->setValue(atoi(stokens[1].c_str()));
      shadingcoldSpinBox->setValue(0);
      i=0;
      while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
      if (i==nr_cs) {
        shadingcoldComboBox->setCurrentIndex(0);
      }else {
        str += "," + tokens[1];
        shadingcoldComboBox->setCurrentIndex(i+1);
      }
    } else {
      shadingcoldComboBox->setCurrentIndex(0);
    }
    updateFieldOptions("palettecolours",str,-1);
  } else {
    updateFieldOptions("palettecolours","off",-1);
    shadingComboBox->setCurrentIndex(0);
    shadingComboBox->setEnabled(false);
    shadingcoldComboBox->setCurrentIndex(0);
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
    std::string value = vpcopt[nc].allValue;
    //METLIBS_LOG_DEBUG("patterns:"<<value);
    int nr_p = patternInfo.size();
    std::string str;
    i=0;
    while (i<nr_p && value!=patternInfo[i].name) i++;
    if (i==nr_p) {
      str = "off";
      patternComboBox->setCurrentIndex(0);
    }else {
      str = patternInfo[i].name;
      patternComboBox->setCurrentIndex(i+1);
    }
    updateFieldOptions("patterns",str,-1);
  } else {
    updateFieldOptions("patterns","off",-1);
    patternComboBox->setCurrentIndex(0);
  }

  //pattern colour
  if ((nc=cp->findKey(vpcopt,"patterncolour"))>=0) {
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      updateFieldOptions("patterncolour","remove");
      patternColourBox->setCurrentIndex(0);
    }else {
      updateFieldOptions("patterncolour",colourInfo[i].name);
      patternColourBox->setCurrentIndex(i+1);
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
    lineTypeCbox->setCurrentIndex(i);
  } else if (lineTypeCbox->isEnabled()) {
    lineTypeCbox->setEnabled(false);
  }

  // linewidth
  if ((nc=cp->findKey(vpcopt,"linewidth"))>=0) {
    lineWidthCbox->setEnabled(true);
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miutil::from_number(i+1)) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("linewidth",miutil::from_number(i+1));
    }
    lineWidthCbox->setCurrentIndex(i);
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
    std::string s;
    if (!vpcopt[nc].strValue.empty()) {
      s= vpcopt[nc].strValue[0];
    } else {
      s= "0";
      updateFieldOptions("density",s);
    }
    if (s=="0") {
      i=0;
    } else {
      i = densityStringList.indexOf(QString(s.c_str()));
      if (i==-1) {
        densityStringList <<QString(s.c_str());
        densityCbox->addItem(QString(s.c_str()));
        i=densityCbox->count()-1;
      }
    }
    densityCbox->setCurrentIndex(i);
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
  }

  if ((nc=cp->findKey(vpcopt,"extreme.size"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeSizeSpinBox->setValue(i);
  } else {
    extremeSizeSpinBox->setValue(100);
  }

  if ((nc=cp->findKey(vpcopt,"extreme.limits"))>=0) {
    std::string value = vpcopt[nc].allValue;
    std::vector<std::string> tokens = miutil::split(value,",");
    if ( tokens.size() > 0 ) {
      int index = extremeLimits.indexOf(tokens[0].c_str());
      if ( index > -1 ) {
        extremeLimitMinComboBox->setCurrentIndex(index);//todo
      }
      if ( tokens.size() > 1 ) {
        index = extremeLimits.indexOf(tokens[1].c_str());
        if ( index > -1 ) {
          extremeLimitMaxComboBox->setCurrentIndex(index);//todo
        }
      }
    }
  }

  // extreme.type (value or none)
  if ((nc=cp->findKey(vpcopt,"extreme.type"))>=0) {
    if( miutil::to_lower(vpcopt[nc].allValue) == "value" ) {
      extremeValueCheckBox->setChecked(true);
    } else {
      extremeValueCheckBox->setChecked(false);
    }
    updateFieldOptions("extreme.type", vpcopt[nc].allValue);
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
    extremeSizeSpinBox->setEnabled(true);
  }
  extremeValueCheckBox->setEnabled(true);
  /*************************************************************************
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
  std::string base;
  if (ekv>0. && (nc=cp->findKey(vpcopt,"base"))>=0) {
    if (!vpcopt[nc].floatValue.empty())
      e = vpcopt[nc].floatValue[0];
    else
      e = 0.0;
    zero1ComboBox->setEnabled(true);
    base = baseList(zero1ComboBox,vpcopt[nc].floatValue[0],ekv/2.0);
    if (not base.empty())
      cp->replaceValue(vpcopt[nc],base, 0);
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
    undefMaskingCbox->setCurrentIndex(i);
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
    undefColourCbox->setCurrentIndex(i);
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
    undefLinewidthCbox->setCurrentIndex(i);
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
    undefLinetypeCbox->setCurrentIndex(i);
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
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(min1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      min1ComboBox->setCurrentIndex(0);
  } else {
    min1ComboBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"maxvalue");
  if (nc>=0) {
    max1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(max1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      max1ComboBox->setCurrentIndex(0);
  } else {
    max1ComboBox->setEnabled( false );
  }
}


void VcrossDialog::disableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  if (currentFieldOpts.empty())
    return;
  currentFieldOpts.clear();

  Delete->setEnabled( false );
  deleteAll->setEnabled( false );
  copyField->setEnabled( false );
  changeModelButton->setEnabled( false );
  upFieldButton->setEnabled( false );
  downFieldButton->setEnabled( false );
  resetOptionsButton->setEnabled( false );

  colorCbox->setEnabled( false );
  shadingComboBox->setCurrentIndex(0);
  shadingComboBox->setEnabled( false );
  shadingSpinBox->setValue(0);
  shadingSpinBox->setEnabled( false );
  shadingcoldComboBox->setCurrentIndex(0);
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

  extremeValueCheckBox->setChecked( false );
  extremeValueCheckBox->setEnabled( false );

  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled( false );

  extremeLimitMinComboBox->setCurrentIndex(0);
  extremeLimitMinComboBox->setEnabled(false);
  extremeLimitMaxComboBox->setCurrentIndex(0);
  extremeLimitMaxComboBox->setEnabled(false);

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
}


std::vector<std::string> VcrossDialog::numberList( QComboBox* cBox, float number )
{
  METLIBS_LOG_SCOPE();

  cBox->clear();

  std::vector<std::string> vnumber;

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
    vnumber.push_back(miutil::from_number(enormal[k]*ex));
  }
  n=1+nupdown*2;

  QString qs;
  for (i=0; i<n; ++i) {
    cBox->addItem(QString(vnumber[i].c_str()));
  }

  cBox->setCurrentIndex(nupdown);

  return vnumber;
}

std::string VcrossDialog::baseList( QComboBox* cBox,
    float base,
    float ekv,
    bool onoff )
{
  std::string str;

  int n;
  if (base<0.) n= int(base/ekv - 0.5);
  else         n= int(base/ekv + 0.5);
  if (fabsf(base-ekv*float(n))>0.01*ekv) {
    base= ekv*float(n);
    str = miutil::from_number(base);
  }
  n=21;
  int k=n/2;
  int j=-k-1;

  cBox->clear();

  if(onoff)
    cBox->addItem(tr("Off"));

  for (int i=0; i<n; ++i) {
    j++;
    float e= base + ekv*float(j);
    if(fabs(e)<ekv/2)
      cBox->addItem("0");
    else{
      const std::string estr = miutil::from_number(e);
      cBox->addItem(QString::fromStdString(estr));
    }
  }

  if(onoff)
    cBox->setCurrentIndex(k+1);
  else
    cBox->setCurrentIndex(k);

  return str;
}

void VcrossDialog::selectedFieldboxClicked(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();

  int index = selectedFieldbox->row(item);

  // may get here when there is none selected fields (the last is removed)
  if (index<0 || selectedFields.size()==0) return;

  enableFieldOptions();
}


void VcrossDialog::colorCboxActivated(int index)
{
  if (index==0)
    updateFieldOptions("colour","off");
  else
    updateFieldOptions("colour",colourInfo[index-1].name);
}


void VcrossDialog::lineWidthCboxActivated(int index)
{
  updateFieldOptions("linewidth", miutil::from_number(index+1));
}


void VcrossDialog::lineTypeCboxActivated(int index)
{
  updateFieldOptions("linetype", linetypes[index]);
}


void VcrossDialog::lineintervalCboxActivated(int index)
{
  updateFieldOptions("line.interval", lineintervals[index]);
  // update the list (with selected value in the middle)
  float a= atof(lineintervals[index].c_str());
  lineintervals= numberList( lineintervalCbox, a);
}


void VcrossDialog::densityCboxActivated(int index)
{
  if (index==0)
    updateFieldOptions("density","0");
  else
    updateFieldOptions("density",densityCbox->currentText().toStdString());
}


void VcrossDialog::vectorunitCboxActivated(int index)
{
  updateFieldOptions("vector.unit",vectorunit[index]);
  // update the list (with selected value in the middle)
  float a= atof(vectorunit[index].c_str());
  vectorunit= numberList( vectorunitCbox, a);
}


void VcrossDialog::extremeValueCheckBoxToggled(bool on)
{
  if (on) {
    updateFieldOptions("extreme.type","Value");
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
    extremeSizeSpinBox->setEnabled(true);
  } else {
    updateFieldOptions("extreme.type","None");
    extremeLimitMinComboBox->setEnabled(false);
    extremeLimitMaxComboBox->setEnabled(false);
    extremeSizeSpinBox->setEnabled(false);
  }
  extremeLimitsChanged();
}

void VcrossDialog::extremeLimitsChanged()
{
  std::string extremeString = "remove";
  std::ostringstream ost;
  if (extremeLimitMinComboBox->isEnabled() && extremeLimitMinComboBox->currentIndex() > 0 ) {
    ost << extremeLimitMinComboBox->currentText().toStdString();
    if(  extremeLimitMaxComboBox->currentIndex() > 0 ) {
      ost <<","<<extremeLimitMaxComboBox->currentText().toStdString();
    }
    extremeString = ost.str();
  }
  updateFieldOptions("extreme.limits",extremeString);
}

void VcrossDialog::extremeSizeChanged(int value)
{
  const std::string str = miutil::from_number(float(value)*0.01);
  updateFieldOptions("extreme.size", str);
}


void VcrossDialog::lineSmoothChanged(int value)
{
  const std::string str = miutil::from_number(value);
  updateFieldOptions("line.smooth",str);
}


void VcrossDialog::labelSizeChanged(int value)
{
  const std::string str = miutil::from_number(float(value)*0.01);
  updateFieldOptions("label.size",str);
}


void VcrossDialog::hourOffsetChanged(int value)
{
  const int n = selectedFieldbox->currentRow();
  selectedFields[n].hourOffset = value;
}


//void VcrossDialog::undefMaskingActivated(int index){
//  updateFieldOptions("undef.masking",miutil::miString(index));
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


void VcrossDialog::zeroLineCheckBoxToggled(bool on)
{
  updateFieldOptions("zero.line", on ? "1" : "0");
}


void VcrossDialog::valueLabelCheckBoxToggled(bool on)
{
  updateFieldOptions("value.label", on ? "1" : "0");
}


void VcrossDialog::tableCheckBoxToggled(bool on)
{
  updateFieldOptions("table", on ? "1" : "0");
}

void VcrossDialog::patternComboBoxToggled(int index)
{
  if(index == 0){
    updateFieldOptions("patterns","off");
  } else {
    updateFieldOptions("patterns", patternInfo[index-1].name);
  }
  updatePaletteString();
}

void VcrossDialog::patternColourBoxToggled(int index)
{
  if(index == 0){
    updateFieldOptions("patterncolour","remove");
  } else {
    updateFieldOptions("patterncolour",colourInfo[index-1].name);
  }
  updatePaletteString();
}


void VcrossDialog::repeatCheckBoxToggled(bool on)
{
  updateFieldOptions("repeat", on ? "1" : "0");
}

void VcrossDialog::shadingChanged()
{
  updatePaletteString();
}

void VcrossDialog::updatePaletteString(){

  if(patternComboBox->currentIndex()>0 && patternColourBox->currentIndex()>0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }

  int index1 = shadingComboBox->currentIndex();
  int index2 = shadingcoldComboBox->currentIndex();
  int value1 = shadingSpinBox->value();
  int value2 = shadingcoldSpinBox->value();

  if(index1==0 && index2==0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }

  std::string str;
  if(index1>0){
    str = csInfo[index1-1].name;
    if(value1>0)
      str += ";" + miutil::from_number(value1);
    if(index2>0)
      str += ",";
  }
  if(index2>0){
    str += csInfo[index2-1].name;
    if(value2>0)
      str += ";" + miutil::from_number(value2);
  }
  updateFieldOptions("palettecolours",str,-1);
}

void VcrossDialog::alphaChanged(int index)
{
  updateFieldOptions("alpha", miutil::from_number(index));
}

void VcrossDialog::zero1ComboBoxToggled(int index)
{
  if(!zero1ComboBox->currentText().isNull() ){
    std::string str = zero1ComboBox->currentText().toStdString();
    updateFieldOptions("base",str);
    float a = atof(str.c_str());
    float b = lineintervalCbox->currentText().toInt();
    baseList(zero1ComboBox,a,b,true);
  }
}

void VcrossDialog::min1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("minvalue","off");
  else if(!min1ComboBox->currentText().isNull() ){
    std::string str = min1ComboBox->currentText().toStdString();
    updateFieldOptions("minvalue",str);
    float a = atof(str.c_str());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(min1ComboBox,a,b,true);
  }
}

void VcrossDialog::max1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("maxvalue","off");
  else if(!max1ComboBox->currentText().isNull() ){
    std::string str = max1ComboBox->currentText().toStdString();
    updateFieldOptions("maxvalue", max1ComboBox->currentText().toStdString());
    float a = atof(str.c_str());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(max1ComboBox,a,b,true);
  }
}
void VcrossDialog::updateFieldOptions(const std::string& name,
    const std::string& value, int valueIndex)
{
  METLIBS_LOG_SCOPE("name= " << name << "  value= " << value);

  if (currentFieldOpts.empty()) return;

  int n= selectedFieldbox->currentRow();

  if(value == "remove")
    cp->removeValue(vpcopt,name);
  else
    cp->replaceValue(vpcopt,name,value,valueIndex);

  currentFieldOpts= cp->unParse(vpcopt);

  selectedFields[n].fieldOpts= currentFieldOpts;

  // update private settings
  const std::string& field = selectedFields[n].field;
  fieldOptions[field] = currentFieldOpts;
  changedOptions[field] = true;
}


std::vector<std::string> VcrossDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  if (historyOkButton->isEnabled())
    historyOk();

  //#################################################################
  //  METLIBS_LOG_DEBUG("VcrossDialog::getOKString  selectedFields.size()= "
  //      <<selectedFields.size());
  //#################################################################

  std::vector<std::string> vstr;
  if (selectedFields.size()==0)
    return vstr;

  std::vector<std::string> hstr;

  int n= selectedFields.size();

  for (int i=0; i<n; i++) {

    std::ostringstream ostr;

    ostr << "model=" << selectedFields[i].model
        << " field=" <<  selectedFields[i].field;
    //#############################################################
    //METLIBS_LOG_DEBUG("OK: " << ostr.str());
    //#############################################################

    if (selectedFields[i].hourOffset!=0)
      ostr << " hour.offset=" << selectedFields[i].hourOffset;

    ostr << " " << selectedFields[i].fieldOpts;

    std::string str;

    str= "VCROSS " + ostr.str();

    // the History string
    hstr.push_back(ostr.str());

    // the OK string
    vstr.push_back(str);

    //#############################################################
    //METLIBS_LOG_DEBUG("OK: " << str);
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
    //if (newcommand) METLIBS_LOG_DEBUG("NEW COMMAND !!!!!!!!!!!!!");
    //else            METLIBS_LOG_DEBUG("SAME COMMAND !!!!!!!!!!!!!");
    //###############################################################
  }
  //###############################################################
  //if (hstr.size()>0) {
  //  METLIBS_LOG_DEBUG("OK-----------------------------");
  //  n= hstr.size();
  //  bool eq;
  //  vector< vector<miutil::miString> >::iterator p, pend= commandHistory.end();
  //  for (p=commandHistory.begin(); p!=pend; p++) {
  //    if (p->size()==n) {
  //	eq= true;
  //	for (i=0; i<n; i++)
  //	  if (p[i]!=hstr[i]) eq= false;
  //	if (eq) METLIBS_LOG_DEBUG("FOUND");
  //  }
  //  }
  //  METLIBS_LOG_DEBUG("-------------------------------");
  //}
  //###############################################################

  historyPos= commandHistory.size();

  historyBackButton->setEnabled(true);
  historyForwardButton->setEnabled(false);

  return vstr;
}

std::string VcrossDialog::getShortname()
{
  int n = selectedFields.size();
  std::ostringstream ostr;
  std::string pmodelName;

  for (int i = 0; i < n; i++) {
    const std::string& modelName = selectedFields[i].model;
    const std::string& fieldName = selectedFields[i].field;

    if (i > 0)
      ostr << "  ";

    if (modelName != pmodelName) {
      ostr << modelName;
      pmodelName = modelName;
    }

    ostr << " " << fieldName;
  }

  if (n > 0)
    return ostr.str();
  return std::string();
}


void VcrossDialog::historyBack()
{
  showHistory(-1);
}


void VcrossDialog::historyForward()
{
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

    std::vector<std::string> vstr;
    int n= commandHistory[historyPos].size();

    for (int i=0; i<n; i++) {
      std::vector<ParsedCommand> vpc= cp->parse( commandHistory[historyPos][i] );
      int m= vpc.size();
      std::string str;
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


void VcrossDialog::historyOk()
{
  METLIBS_LOG_SCOPE();

  if (historyPos<0 || historyPos>=int(commandHistory.size())) {
    std::vector<std::string> vstr;
    putOKString(vstr,false,false);
  } else {
    putOKString(commandHistory[historyPos],false,false);
  }
}


void VcrossDialog::putOKString(const std::vector<std::string>& vstr,
    bool vcrossPrefix, bool checkOptions)
{
  METLIBS_LOG_SCOPE();

  deleteAllSelected();

  int nc= vstr.size();

  if (nc==0)
    return;

  std::string fields2_model,model,field,fOpts;
  int ic,i,j,m,n,hourOffset;
  std::vector<std::string> fields2;
  int nf2= 0;
  std::vector<ParsedCommand> vpc;

  for (ic=0; ic<nc; ic++) {

    //######################################################################
    //    METLIBS_LOG_DEBUG("P.OK>> " << vstr[ic]);
    //######################################################################
    if (checkOptions) {
      std::string str= checkFieldOptions(vstr[ic],vcrossPrefix);
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
      //METLIBS_LOG_DEBUG("   " << j << " : " << vpc[j].key << " = " << vpc[j].strValue[0]
      //     << "   " << vpc[j].allValue);
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
    //    METLIBS_LOG_DEBUG(" ->" << model << " " << field);
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

      std::string str= model + " " + field;
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
}


std::vector<std::string> VcrossDialog::writeLog()
{
  std::vector<std::string> vstr;

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

  std::map<std::string,std::string>::iterator p, pend= fieldOptions.end();

  for (p=fieldOptions.begin(); p!=pend; p++) {
    if (changedOptions[p->first])
      vstr.push_back( p->first + " " + p->second );
  }
  vstr.push_back("================");

  return vstr;
}


void VcrossDialog::readLog(const std::vector<std::string>& vstr,
    const std::string& thisVersion,
    const std::string& logVersion)
{
  std::string str,fieldname,fopts;
  std::vector<std::string> hstr;
  size_t pos,end;

  std::map<std::string,std::string>::iterator pfopt, pfend= fieldOptions.end();
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
      if (not str.empty())
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
        std::vector<ParsedCommand> vpopt= cp->parse( pfopt->second );
        std::vector<ParsedCommand> vplog= cp->parse( fopts );
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
              //METLIBS_LOG_DEBUG("    D.CH: " << fieldname << " "
              //                     << vpopt[j].key << " "
              //                     << vpopt[j].allValue << " -> "
              //                     << vplog[i].allValue);
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
          //METLIBS_LOG_DEBUG("D.OLD: " << fieldname << " " << pfopt->second);
          //METLIBS_LOG_DEBUG("D.LOG: " << fieldname << " " << fopts);
          //###########################################################
          pfopt->second= cp->unParse(vpopt);
          //###########################################################
          //METLIBS_LOG_DEBUG("D.NEW: " << fieldname << " " << pfopt->second);
          //###########################################################
          changedOptions[fieldname]= true;
        }
        //###########################################################
        //else METLIBS_LOG_DEBUG("D.OK:  " << fieldname << " " << pfopt->second);
        //###########################################################
      }
      //###########################################################
      //else METLIBS_LOG_DEBUG("D.UNKNOWN: " << fieldname << " " << fopts);
      //###########################################################
    }
  }
  ivstr++;

  historyPos= commandHistory.size();

  historyBackButton->setEnabled(historyPos>0);
  historyForwardButton->setEnabled(false);
}


std::string VcrossDialog::checkFieldOptions(const std::string& str, bool vcrossPrefix)
{
  METLIBS_LOG_SCOPE();

  std::string newstr;

  std::map<std::string,std::string>::iterator pfopt;
  std::string fieldname;
  int nopt;

  std::vector<ParsedCommand> vplog= cp->parse( str );
  int nlog= vplog.size();
  int first= (vcrossPrefix) ? 1 : 0;

  if (nlog>=2+first && vplog[first].key=="model"
      && vplog[first+1].key=="field") {
    fieldname= vplog[first+1].allValue;
    pfopt= setupFieldOptions.find(fieldname);
    if (pfopt!=setupFieldOptions.end()) {
      std::vector<ParsedCommand> vpopt= cp->parse( pfopt->second );
      nopt= vpopt.size();
      //##################################################################
      //    METLIBS_LOG_DEBUG("    nopt= " << nopt << "  nlog= " << nlog);
      //    for (int j=0; j<nlog; j++)
      //	METLIBS_LOG_DEBUG("        log " << j << " : id " << vplog[j].idNumber
      //	     << "  " << vplog[j].key << " = " << vplog[j].allValue);
      //##################################################################
      if (vcrossPrefix) newstr= "VCROSS ";
      for (int i=first; i<nlog; i++) {
        if (vplog[i].idNumber==1)
          newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }

      //loop through current options, replace the value if the new string has same option with different value
      for (int j=0; j<nopt; j++) {
        int i=0;
        while (i<nlog && vplog[i].key!=vpopt[j].key) i++;
        if (i<nlog) {
          // there is no option with variable no. of values, YET...
          if (vplog[i].allValue!=vpopt[j].allValue &&
              vplog[i].strValue.size()==vpopt[j].strValue.size())
            cp->replaceValue(vpopt[j],vplog[i].allValue,-1);
        }
      }

      //loop through new options, add new option if it is not a part of current options
       for (int i = 2; i < nlog; i++) {
         int j = 0;
         while (j < nopt && vpopt[j].key != vplog[i].key)
           j++;
         if (j == nopt) {
           cp->replaceValue(vpopt, vplog[i].key, vplog[i].allValue);
         }
       }


      newstr+= " ";
      newstr+= cp->unParse(vpopt);
      if (vcrossPrefix) {
        // from quickmenu, keep "forecast.hour=..." and "forecast.hour.loop=..."
        for (int i=2+first; i<nlog; i++) {
          if (vplog[i].idNumber==4 || vplog[i].idNumber==-1)
            newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
        }
      }
      for (int i=2+first; i<nlog; i++) {
        if (vplog[i].idNumber==3)
          newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
    }
  }
  //##################################################################
  //  METLIBS_LOG_DEBUG("cF str:    "<<str);
  //  METLIBS_LOG_DEBUG("cF newstr: "<<newstr);
  //##################################################################

  return newstr;
}


void VcrossDialog::deleteSelected()
{
  METLIBS_LOG_SCOPE();

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
    countSelected[indexF]--;
    if (countSelected[indexF] == 0) {
      fieldbox->item(indexF)->setSelected(false);
    }
  }
  selectedFieldbox->takeItem(index);
  for (int i = index; i < ns; i++) {
    selectedFields[i] = selectedFields[i + 1];
  }
  selectedFields.pop_back();

  if (selectedFields.size()>0) {
    if (index>=int(selectedFields.size())) index= selectedFields.size()-1;
    selectedFieldbox->setCurrentRow( index );
    selectedFieldbox->item(index)->setSelected( true );
    enableFieldOptions();
  } else {
    disableFieldOptions();
  }
}


void VcrossDialog::deleteAllSelected()
{
  METLIBS_LOG_SCOPE();

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
}

void VcrossDialog::copySelectedField()
{
  METLIBS_LOG_SCOPE();

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
}


void VcrossDialog::changeModel()
{
  METLIBS_LOG_SCOPE();

  if (selectedFieldbox->count()==0) return;
  int i, n= selectedFields.size();
  if (n==0) return;

  int index= selectedFieldbox->currentRow();
  if (index<0 || index>=n) return;

  if (modelbox->count()==0) return;

  int indexM= modelbox->currentRow();
  if (indexM<0) return;

  std::string oldmodel= selectedFields[index].model;
  std::string    model= models[indexM];

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
        std::string str= model + " " + fields[j];
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
}


void VcrossDialog::upField()
{
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


void VcrossDialog::downField()
{
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


void VcrossDialog::resetOptions()
{
  if (selectedFieldbox->count()==0)
    return;
  int n= selectedFields.size();
  if (n==0)
    return;

  int index= selectedFieldbox->currentRow();
  if (index<0 || index>=n)
    return;

  std::string field= selectedFields[index].field;
  selectedFields[index].fieldOpts= setupFieldOptions[field];
  selectedFields[index].hourOffset= 0;
  enableFieldOptions();
}


void VcrossDialog::applyClicked()
{
  if (historyOkButton->isEnabled())
    historyOk();
  const std::vector<std::string> vstr = getOKString();
  bool modelChange = vcrossm->setSelection(vstr);
  /*emit*/ VcrossDialogApply(modelChange);
}


void VcrossDialog::applyhideClicked()
{
  applyClicked();
  hideClicked();
}


void VcrossDialog::hideClicked()
{
  /*emit*/ VcrossDialogHide();
}


void VcrossDialog::helpClicked()
{
  /*emit*/ showsource("ug_verticalcrosssections.html");
}


void VcrossDialog::closeEvent(QCloseEvent* e)
{
  /*emit*/ VcrossDialogHide();
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
  METLIBS_LOG_SCOPE();

  deleteAllSelected();
  fieldbox->clear();
  fields.clear();

  // one way of unselecting model (in a single selection box)...
  modelbox->clearSelection();
  modelbox->clear();
  const int n = models.size();
  for (int i=0; i<n; i++)
    modelbox->addItem(QString::fromStdString(models[i]));
  fieldbox->setFocus();
}
