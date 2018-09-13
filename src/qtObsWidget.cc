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

#include "diana_config.h"

#include "qtObsWidget.h"

#include "diKVListPlotCommand.h"
#include "diUtilities.h"
#include "qtButtonLayout.h"
#include "qtUtility.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QLCDNumber>
#include <QToolTip>
#include <QFrame>
#include <QImage>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>

#include <iomanip>
#include <sstream>
#include <cmath>

#define MILOGGER_CATEGORY "diana.ObsWidget"
#include <miLogger/miLogging.h>

using namespace std;

/*
  GENERAL DESCRIPTION: This widget takes several datatypes ( that might
  represent one plottype ) as input and generates a widget where zero, one or
  more datatypes might be selected by pressing the topbuttons.
  It also includes some sliders, checkboxes, comboboxes etc.
 */

ObsWidget::ObsWidget(QWidget* parent)
    : QWidget(parent)
    , moreButton(0)
{
  METLIBS_LOG_SCOPE();
  initOK = false;
}

void ObsWidget::setDialogInfo(ObsDialogInfo::PlotType dialoginfo)
{
  METLIBS_LOG_SCOPE();

  initOK = true;

  ObsDialogInfo::PlotType& dialogInfo = dialoginfo;

  plotType = dialogInfo.name;

  // Button names
  std::vector<ObsDialogInfo::Button> dataTypeButton;
  nr_dataTypes = dialogInfo.readernames.size();
  for (const std::string& rn : dialogInfo.readernames) {
    ObsDialogInfo::Button b;
    b.name = rn;
    dataTypeButton.push_back(b);
  }

  // Info about sliders, check boxes etc.
  int density_minValue = 5;
  int density_value = 10;
  scaledensity = 0.1;
  maxdensity = 25;

  int size_minValue = 1;
  int size_maxValue = 35;
  int size_value = 10;
  scalesize = 0.1;

  // timer
  int timediff_minValue = 0;
  int timediff_maxValue = 48;

  verticalLevels = dialogInfo.verticalLevels.size();

  // Info about colours
  cInfo = Colour::getColourInfo();
  int colIndex = getIndex(cInfo, "black");
  int devcol1Index = getIndex(cInfo, "red");
  int devcol2Index = getIndex(cInfo, "blue");

  bool devField = (dialogInfo.misc & ObsDialogInfo::dev_field_button) != 0;
  bool tempPrecision = (dialogInfo.misc & ObsDialogInfo::tempPrecision) != 0;
  bool unit_ms = (dialogInfo.misc & ObsDialogInfo::unit_ms) != 0;
  bool orient = (dialogInfo.misc & ObsDialogInfo::orientation) != 0;
  bool parameterName = (dialogInfo.misc & ObsDialogInfo::parameterName) != 0;
  bool popupWindow = (dialogInfo.misc & ObsDialogInfo::popup) != 0;
  bool qualityFlag = (dialogInfo.misc & ObsDialogInfo::qualityflag) != 0;
  bool wmoFlag = (dialogInfo.misc & ObsDialogInfo::wmoflag) != 0;
  markerboxVisible = (dialogInfo.misc & ObsDialogInfo::markerboxVisible) != 0;
  bool criteria = (dialogInfo.misc & ObsDialogInfo::criteria) != 0;

  // DECLARATION OF BUTTONS

  datatypeButtons = new ButtonLayout(this, dataTypeButton, 3);

  dialoginfo.addExtraParameterButtons();
  button = dialogInfo.button;
  parameterButtons = new ButtonLayout(this, dialogInfo.button, 3);

  QScrollArea* scrollArea = new QScrollArea(this);
  scrollArea->setWidget(parameterButtons);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(datatypeButtons, SIGNAL(buttonClicked(int)), SLOT(datatypeButtonClicked(int)));
  connect( datatypeButtons, SIGNAL(rightClickedOn(std::string)),
      SLOT(rightClickedSlot(std::string)));
  connect( parameterButtons, SIGNAL(rightClickedOn(std::string)),
      SLOT(rightClickedSlot(std::string)));

  //AND-Buttons
  allButton  = NormalPushButton(tr("All"),this);
  noneButton = NormalPushButton(tr("None"),this);
  QHBoxLayout* andLayout = new QHBoxLayout();
  andLayout->addWidget( allButton  );
  andLayout->addWidget( noneButton );
  connect( allButton,SIGNAL(clicked()),
      parameterButtons,SLOT(ALLClicked()));
  connect( noneButton,SIGNAL(clicked()),
      parameterButtons,SLOT(NONEClicked()));

  // PRESSURE LEVELS
  QHBoxLayout* pressureLayout = new QHBoxLayout();

  if (verticalLevels) {
    QLabel* pressureLabel = new QLabel(tr("Vertical level"), this);

    pressureComboBox = new QComboBox(this);
    pressureComboBox->addItem(tr("As field"));
    levelMap["asfield"] = 0;
    int psize = dialogInfo.verticalLevels.size();
    for(int i=1; i<psize+1; i++){
      std::string str = miutil::from_number(dialogInfo.verticalLevels[psize - i]);
      pressureComboBox->addItem(str.c_str());
      levelMap[str]=i;
    }

    pressureLayout->addWidget( pressureLabel );
    pressureLayout->addWidget( pressureComboBox );
    pressureLayout->addStretch();

  }

  //checkboxes
  orientCheckBox= new QCheckBox(tr("Horisontal orientation"),this);
  alignmentCheckBox= new QCheckBox(tr("Align right"),this);
  if(!orient){
    orientCheckBox->hide();
    alignmentCheckBox->hide();
  }

  showposCheckBox= new QCheckBox(tr("Show all positions"),this);
  tempPrecisionCheckBox= new QCheckBox(tr("Temperatures as integers"),this);
  if(!tempPrecision) tempPrecisionCheckBox->hide();
  unit_msCheckBox= new QCheckBox(tr("Wind speed in m/s"),this);
  if(!unit_ms) unit_msCheckBox->hide();
  parameterNameCheckBox= new QCheckBox(tr("Name of parameter"),this);
  if(!parameterName) parameterNameCheckBox->hide();
  popupWindowCheckBox= new QCheckBox(tr("Selected observation in popup window"),this);
  if(!popupWindow) popupWindowCheckBox->hide();
  devFieldCheckBox= new QCheckBox(tr("PPPP - MSLP-field"),this);
  if(!devField) devFieldCheckBox->hide();
  devColourBox1 = ColourBox( this, cInfo, true, devcol1Index );
  devColourBox2 = ColourBox( this, cInfo, true, devcol2Index );
  devColourBox1->hide();
  devColourBox2->hide();
  QHBoxLayout* devLayout = new QHBoxLayout();
  devLayout->addWidget(devFieldCheckBox);
  devLayout->addWidget(devColourBox1);
  devLayout->addWidget(devColourBox2);
  qualityCheckBox= new QCheckBox(tr("Quality stations"),this);
  if ( !qualityFlag ) qualityCheckBox->hide();
  wmoCheckBox= new QCheckBox(tr("WMO stations"),this);
  if ( !wmoFlag ) wmoCheckBox->hide();

  //Onlypos & marker
  onlyposCheckBox= new QCheckBox(tr("Positions only"),this);

  markerBox = PixmapBox( this, markerName);
  markerName.push_back("off");
  markerBox->addItem("off");
  if( markerName.size() == 0 )
    onlyposCheckBox->setEnabled(false);

  QHBoxLayout* onlyposLayout = new QHBoxLayout();
  onlyposLayout->addWidget( onlyposCheckBox );
  onlyposLayout->addWidget( markerBox );


  if( !markerboxVisible ) {
    markerBox->hide();
  }

  //Criteria list
  ObsDialogInfo::CriteriaList cl;
  criteriaList.push_back(cl);
  criteriaList.insert(criteriaList.end(),
      dialogInfo.criteriaList.begin(),
      dialogInfo.criteriaList.end());
  currentCriteria = -1;
  criteriaCheckBox = new QCheckBox(tr("Criterias"),this);
  criteriaChecked(false);
  moreButton = new ToggleButton(this, tr("<<Less"), tr("More>>"));
  moreButton->setChecked(false);
  if(!criteria){
    criteriaCheckBox->hide();
    moreButton->hide();
  }
  QHBoxLayout* criteriaLayout = new QHBoxLayout();
  criteriaLayout->addWidget( criteriaCheckBox );
  criteriaLayout->addWidget( moreButton );


  connect( moreButton, SIGNAL( toggled(bool)),SLOT( extensionSlot( bool ) ));
  connect(criteriaCheckBox,SIGNAL(toggled(bool)),SLOT(criteriaChecked(bool)));
  connect(devFieldCheckBox, SIGNAL(toggled(bool)),SLOT(devFieldChecked(bool)));
  connect(onlyposCheckBox, SIGNAL(toggled(bool)), SLOT(onlyposChecked(bool)));

  tempPrecisionCheckBox->setChecked( true );
  unit_msCheckBox->setChecked( false );


  // LCD numbers and sliders

  QLabel* densityLabel = new QLabel( tr("Density"), this);
  QLabel* sizeLabel = new QLabel( tr("Size"), this);
  QLabel *diffLabel = new QLabel( tr("Timediff"), this);

  densityLcdnum = LCDNumber( 5, this); // 4 siffer
  sizeLcdnum = LCDNumber( 5, this);
  diffLcdnum = LCDNumber( 5, this);

  for(int i=0;i<13;i++)
    time_slider2lcd.push_back(i*15);

  densitySlider = Slider(density_minValue, maxdensity, 1, density_value, Qt::Horizontal, this);
  sizeSlider = Slider(size_minValue, size_maxValue, 1, size_value, Qt::Horizontal, this);
  diffSlider= Slider( 0,time_slider2lcd.size(), 1, 4,
      Qt::Horizontal, this);

  diffComboBox = new QComboBox(this);
  diffComboBox->addItem("3t");
  diffComboBox->addItem("24t");
  diffComboBox->addItem("7d");

  displayDiff(diffSlider->value());

  QGridLayout*slidergrid = new QGridLayout();
  slidergrid->addWidget( densityLabel, 0, 0 );
  slidergrid->addWidget( densityLcdnum,0, 1 );
  slidergrid->addWidget( densitySlider,0, 2 );
  slidergrid->addWidget( sizeLabel,    1, 0 );
  slidergrid->addWidget( sizeLcdnum,   1, 1);
  slidergrid->addWidget( sizeSlider,   1, 2 );
  slidergrid->addWidget( diffLabel,    2, 0 );
  slidergrid->addWidget( diffLcdnum,   2, 1 );
  slidergrid->addWidget( diffSlider,   2, 2  );
  slidergrid->addWidget( diffComboBox, 2, 3  );


  //Priority list
  vector<std::string> priName;
  for(unsigned int i=0; i<priorityList.size(); i++)
    priName.push_back(priorityList[i].name);
  pribox = ComboBox( this,priName,true);
  pribox->insertItem(0,tr("No priority list"));
  QLabel *priLabel = new QLabel( tr("Priority "), this);
  pricheckbox = new QCheckBox(tr("Prioritized only"), this);

  //Parameter sort
  std::vector<std::string> buttonNames;
  for (unsigned int i = 0; i < dialogInfo.button.size(); i++)
    buttonNames.push_back(dialogInfo.button[i].name);

  QLabel *sortLabel = new QLabel( tr("Sort "), this);
  sortBox = ComboBox( this,buttonNames,true);
  sortBox->insertItem(0,tr("No sort criteria"));
  sortBox->setCurrentIndex(0);
  sortRadiogroup = new QButtonGroup( this );
  ascsortButton = new QRadioButton(tr("Asc"), this);
  descsortButton = new QRadioButton(tr("Desc"), this);
  sortRadiogroup->addButton(ascsortButton);
  sortRadiogroup->addButton(descsortButton);

  //Colour
  QLabel *colourLabel = new QLabel( tr("Colour"), this);
  colourBox = ColourBox( this, cInfo, true, colIndex );

  // CONNECT
  connect( densitySlider,SIGNAL( valueChanged(int)),SLOT(displayDensity(int)));
  connect( sizeSlider,SIGNAL(valueChanged(int)),SLOT(displaySize(int)));
  connect( diffSlider,SIGNAL(valueChanged(int)),SLOT(displayDiff(int)));
  connect( diffComboBox,SIGNAL(activated(int)),SLOT(diffComboBoxSlot(int)));
  connect( pribox, SIGNAL( activated(int) ), SLOT( priSelected(int) ) );

  // Layout for priority list, sort, colours, criteria and extension
  QGridLayout* prilayout = new QGridLayout();

  prilayout->addWidget( priLabel, 0, 0 );
  prilayout->addWidget( pribox, 0, 1 );
  prilayout->addWidget( pricheckbox, 0, 2 );

  prilayout->addWidget( sortLabel, 1, 0 );
  prilayout->addWidget( sortBox, 1, 1 );
  prilayout->addWidget( ascsortButton, 1, 2 );
  prilayout->addWidget( descsortButton, 1, 3 );

  prilayout->addWidget( colourLabel, 2, 0 );
  prilayout->addWidget( colourBox, 2, 1 );

  // layout
  datatypelayout = new QHBoxLayout();
  datatypelayout->setSpacing(1);
  datatypelayout->setAlignment(Qt::AlignHCenter);
  datatypelayout->addWidget( datatypeButtons );

  // Create horizontal frame lines
  QFrame *line0 = new QFrame( this );
  line0->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line1 = new QFrame( this );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // LAYOUT
  vcommonlayout= new QVBoxLayout();
  vcommonlayout->setSpacing(1);
  vcommonlayout->addLayout( andLayout );
  vcommonlayout->addSpacing( 5 );
  vcommonlayout->addLayout( pressureLayout );
  vcommonlayout->addSpacing( 5 );
  vcommonlayout->addWidget( line0 );
  vcommonlayout->addWidget( orientCheckBox );
  vcommonlayout->addWidget( alignmentCheckBox );
  vcommonlayout->addWidget( showposCheckBox );
  vcommonlayout->addWidget( tempPrecisionCheckBox );
  vcommonlayout->addWidget( unit_msCheckBox );
  vcommonlayout->addWidget( parameterNameCheckBox );
  vcommonlayout->addWidget( qualityCheckBox );
  vcommonlayout->addWidget( wmoCheckBox );
  vcommonlayout->addLayout( devLayout );
  vcommonlayout->addWidget( popupWindowCheckBox );
  vcommonlayout->addLayout( onlyposLayout);
  vcommonlayout->addLayout( criteriaLayout );
  vcommonlayout->addWidget( line1 );
  vcommonlayout->addLayout( slidergrid );
  vcommonlayout->addLayout( prilayout );

  vlayout= new QVBoxLayout( this);
  vlayout->addSpacing( 5 );
  vlayout->addLayout( datatypelayout );
  if(parameterButtons)
    vlayout->addWidget( scrollArea );
  vlayout->addLayout( vcommonlayout );

  densityLcdnum->display(((double)density_value) * scaledensity);
  sizeLcdnum->display(((double)size_value) * scalesize);

  allObs = false;

  pri_selected = 0;

  parameterButtons->setEnabled(false);

  ToolTip();
}

void ObsWidget::ToolTip()
{
  datatypeButtons->setToolTip(tr("Data type") );
  devColourBox1->setToolTip(tr("PPPP-MSLP<0"));
  devColourBox2->setToolTip(tr("PPPP-MSLP>0"));
  qualityCheckBox->setToolTip(tr("Only show stations with quality flag good."));
  wmoCheckBox->setToolTip(tr("Only show stations with wmo number"));
  diffLcdnum->setToolTip(tr("Max time difference"));
  diffComboBox->setToolTip(tr("Max value for the slider"));
  pricheckbox->setToolTip(tr("Show only observations in the priority list") );
  colourBox->setToolTip(tr("Colour of the observations") );
}

/***************************************************************************/

void ObsWidget::devFieldChecked(bool on)
{
  devColourBox1->setVisible(on);
  devColourBox2->setVisible(on);
}

void ObsWidget::onlyposChecked(bool on)
{
  if(on || !markerboxVisible)
    markerBox->setVisible(on);
  densitySlider->setEnabled(!on);
  showposCheckBox->setEnabled(!on);
  densityLcdnum->setEnabled(!on);
}

void ObsWidget::criteriaChecked(bool on)
{
  if (on)
    Q_EMIT criteriaOn();
}

/***************************************************************************/

void ObsWidget::priSelected(int index)
{
  //priority file
  pri_selected = index;
  bool off = (pribox->currentText() == tr("No priority list"));
  pricheckbox->setEnabled(!off);
}

/***************************************************************************/

void ObsWidget::displayDensity(int number)
{
  // This function is called when densitySlider sends a signal
  // valueChanged(int)
  // and changes the numerical value in the lcd display densityLcdnum

  double scalednumber= number* scaledensity;
  if (number == maxdensity) {
    densityLcdnum->display("ALLE");
    allObs = true;
  } else {
    densityLcdnum->display(scalednumber);
    allObs = false;
  }
}

/***************************************************************************/

void ObsWidget::displaySize(int number)
{
  /* This function is called when sizeSlider sends a signal valueChanged(int)
     and changes the numerical value in the lcd display sizeLcdnum */

  double scalednumber= number* scalesize;
  sizeLcdnum->display( scalednumber );
}

void ObsWidget::displayDiff(int number)
{
  /* This function is called when diffSslider sends a signal valueChanged(int)
     and changes the numerical value in the lcd display diffLcdnum */

  if( number == int(time_slider2lcd.size()) ) {
    diffLcdnum->display( tr("ALL") );
    timediff_minutes = "alltimes";
    return;
  }

  std::string str;
  int totalminutes = time_slider2lcd[number];
  timediff_minutes = miutil::from_number(totalminutes);
  if(diffComboBox->currentIndex()<2){
    int hours = totalminutes/60;
    int minutes= totalminutes-hours*60;
    ostringstream ostr;
    ostr << hours << ":" << setw(2) << setfill('0') << minutes;
    str= ostr.str();
  } else {
    ostringstream ostr;
    ostr << (totalminutes/60/24) << " d";
    str= ostr.str();
  }

  diffLcdnum->display( str.c_str() );
}

/***************************************************************************/
void ObsWidget::diffComboBoxSlot(int number)
{
  int index = time_slider2lcd[diffSlider->value()];
  bool maxValue = ( diffSlider->value() == int(time_slider2lcd.size()) );

  int maxSliderValue, sliderStep;
  if(number==0){
    maxSliderValue = 13;
    sliderStep = 15;
  } else if(number==1){
    maxSliderValue = 25;
    sliderStep = 60;
  } else {
    maxSliderValue = 8;
    sliderStep = 60*24;
  }

  diffSlider->setRange(0,maxSliderValue);
  time_slider2lcd.clear();
  for(int i=0;i<maxSliderValue;i++)
    time_slider2lcd.push_back(i*sliderStep);

  //set slider
  if(maxValue){
    index = maxSliderValue;
  } else {
    index /= sliderStep;
    if(index>maxSliderValue-1) index = maxSliderValue-1;
  }

  diffSlider->setValue(index);
  displayDiff(diffSlider->value());
}
/***************************************************************************/

void ObsWidget::datatypeButtonClicked(int id)
{
  parameterButtons->setEnabled(!datatypeButtons->noneChecked());

  // Names of datatypes selected are sent to controller
  diutil::OverrideCursor waitCursor;
  emit getTimes(true);
}

void ObsWidget::rightClickedSlot(std::string str)
{
  if (criteriaCheckBox->isChecked())
    Q_EMIT rightClicked(str);
}

/*********************************************/



/*****************************************************************/
vector<std::string> ObsWidget::getDataTypes()
{
  return datatypeButtons->getOKString();
}

/****************************************************************/
miutil::KeyValue_v ObsWidget::makeString()
{
  miutil::KeyValue_v kvs;
  kvs << miutil::KeyValue("plot", plotType);
  METLIBS_LOG_DEBUG(LOGVAL(plotType));
  std::string datastr;
  if (dVariables.data.size()) {
    for (const string& d : dVariables.data)
      diutil::appendText(datastr, d, ",");
    kvs << miutil::KeyValue("data", datastr);
  }

  if (dVariables.parameter.size()) {
    std::string pstr;
    for (const string& p : dVariables.parameter)
      diutil::appendText(pstr, p, ",");
    kvs << miutil::KeyValue("parameter", pstr);
  }

  for (auto&& m : dVariables.misc)
    kvs << miutil::KeyValue(m.first, m.second);

  shortname = "OBS " + plotType + " " + datastr;

  return kvs;
}

KVListPlotCommand_cp ObsWidget::getOKString(bool forLog)
{
  shortname.clear();
  METLIBS_LOG_DEBUG(LOGVAL(plotType));

  dVariables.plotType = plotType;

  dVariables.data = datatypeButtons->getOKString();

  if(!dVariables.data.size() && !forLog)
    return KVListPlotCommand_cp();

  if(parameterButtons)
    dVariables.parameter = parameterButtons->getOKString(forLog);

  if( tempPrecisionCheckBox->isChecked() )
    dVariables.misc["tempprecision"]="true";

  if( unit_msCheckBox->isChecked() )
    dVariables.misc["unit_ms"]="true";

  if( parameterNameCheckBox->isChecked() )
    dVariables.misc["parametername"]="true";

  if( qualityCheckBox->isChecked() )
    dVariables.misc["qualityflag"]="true";

  if( wmoCheckBox->isChecked() )
    dVariables.misc["wmoflag"]="true";

  if( orientCheckBox->isChecked() )
    dVariables.misc["orientation"]="horizontal";

  if( alignmentCheckBox->isChecked() )
    dVariables.misc["alignment"]="right";

  if( showposCheckBox->isChecked() )
    dVariables.misc["showpos"]="true";

  if( onlyposCheckBox->isChecked() ){
    dVariables.misc["onlypos"]="true";
  }
  if( popupWindowCheckBox->isChecked() )
    dVariables.misc["popup"]="true";

  if( pricheckbox->isChecked() ){
      dVariables.misc["showonlyprioritized"]="true";
  }
  if(markerName.size())
    dVariables.misc["image"] = markerName[markerBox->currentIndex()];

  if( devFieldCheckBox->isChecked() ){
    dVariables.misc["devfield"] = "true";
    dVariables.misc["devcolour1"] = cInfo[devColourBox1->currentIndex()].name;
    dVariables.misc["devcolour2"] = cInfo[devColourBox2->currentIndex()].name;
  }

  if (verticalLevels) {
    if(pressureComboBox->currentIndex()>0 ){
      dVariables.misc["level"] = pressureComboBox->currentText().toStdString();
    } else {
      dVariables.misc["level"] = "asfield";
    }
  }

  if( allObs )
    dVariables.misc["density"] = "allobs";
  else{
    std::string tmp = miutil::from_number(densityLcdnum->value());
    dVariables.misc["density"]= tmp;
  }

  std::string sc = miutil::from_number(sizeLcdnum->value());
  dVariables.misc["scale"] = sc;

  dVariables.misc["timediff"]= timediff_minutes;

  if( pri_selected > 0 )
    dVariables.misc["priority"]=priorityList[pri_selected-1].file;

  dVariables.misc["colour"] = cInfo[colourBox->currentIndex()].name;

  KVListPlotCommand_p cmd = std::make_shared<KVListPlotCommand>("OBS");
  cmd->add(makeString());

  //clear old settings
  dVariables.misc.clear();

  //Criteria
  if (cmd->all().empty())
    return KVListPlotCommand_cp();

  bool addsort = true;
  if(forLog){
    int n = criteriaList.size();
    for(int i=1; i<n; i++){
      int m = criteriaList[i].criteria.size();
      if (m==0)
        continue;
      string criteria = criteriaList[i].name;
      for (int j=0; j<m; j++) {
        const vector<std::string> sub = miutil::split(criteriaList[i].criteria[j], " ");
        string subcriteria;
        for(const string& subs : sub)
          diutil::appendText(subcriteria, subs, ",");
        diutil::appendText(criteria, subcriteria, ";");
      }
      cmd->add("criteria", criteria);
    }
  } else if(criteriaCheckBox->isChecked()) {
    int m = savedCriteria.criteria.size();
    if( m==0 )
      addsort = false;
    else {
      string criteria;
      for (int j=0; j<m; j++) {
        const vector<std::string> sub = miutil::split(savedCriteria.criteria[j], " ");
        string subcriteria;
        for(const string& subs : sub)
          diutil::appendText(subcriteria, subs, ",");
        diutil::appendText(criteria, subcriteria, ";");
      }
      cmd->add("criteria", criteria);
    }
  }

  if (addsort && sortBox->currentIndex() > 0 && !sortBox->currentText().isEmpty()) {
    string sort = sortBox->currentText().toStdString();
    sort += ",";
    sort += descsortButton->isChecked() ? "desc" : "asc";
    cmd->add("sort", sort);
  }
  return cmd;
}

std::string ObsWidget::getShortname()
{
  return shortname;
}

void ObsWidget::putOKString(const PlotCommand_cp& str)
{
  dVariables.misc.clear();

  if (KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(str))
    decodeString(cmd->all(), dVariables, false);
  updateDialog(true);

  Q_EMIT getTimes(true);
}

void ObsWidget::readLog(const miutil::KeyValue_v& kvs)
{
  setFalse();
  dVariables.misc.clear();
  decodeString(kvs, dVariables,true);
  updateDialog(false);
}


void ObsWidget::updateDialog(bool setChecked)
{
  int number,m,j;
  double scalednumber;

  //plotType
  plotType = dVariables.plotType;
  METLIBS_LOG_DEBUG(LOGVAL(plotType));

  //data types
  if ( setChecked ){
    m = dVariables.data.size();
    for(j=0; j<m; j++){
      //METLIBS_LOG_DEBUG("updateDialog: "<<dVariables.data[j]);
      int index = datatypeButtons->setButtonOn(dVariables.data[j]);
    }
    parameterButtons->setEnabled(!datatypeButtons->noneChecked());
  }

  //parameter
  if(parameterButtons) {
    parameterButtons->NONEClicked();
    m = dVariables.parameter.size();
    for(j=0; j<m; j++){
      //old syntax
      std::string para = miutil::to_lower(dVariables.parameter[j]);
      if(para == "dd_ff" || para == "vind")
        dVariables.parameter[j] = "wind";
      if(para == "kjtegn")
        dVariables.parameter[j] = "id";
      if(para == "dato")
        dVariables.parameter[j] = "date";
      if(para == "tid")
        dVariables.parameter[j] = "time";
      if(para == "h�yde")
        dVariables.parameter[j] = "height";
      parameterButtons->setButtonOn(dVariables.parameter[j]);
    }
  }

  //temp precision
  if (dVariables.misc.count("tempprecision") &&
      dVariables.misc["tempprecision"] == "true"){
    tempPrecisionCheckBox->setChecked(true);
  }

  //wind unit m/s
  if (dVariables.misc.count("unit_ms") &&
      dVariables.misc["unit_ms"] == "true"){
    unit_msCheckBox->setChecked(true);
  }

  //parameterName
  if (dVariables.misc.count("parametername") &&
      dVariables.misc["parametername"] == "true"){
    parameterNameCheckBox->setChecked(true);
  }

  //popupWindow
  if (dVariables.misc.count("popup") &&
      dVariables.misc["popup"] == "true"){
    popupWindowCheckBox->setChecked(true);
  }

  //Quality flag
  if (setChecked && dVariables.misc.count("qualityflag") &&
      dVariables.misc["qualityflag"] == "true"){
    qualityCheckBox->setChecked(true);
  }

  //WMO number
  if (setChecked && dVariables.misc.count("wmoflag") &&
      dVariables.misc["wmoflag"] == "true"){
    wmoCheckBox->setChecked(true);
  }

  //dev from field
  if (dVariables.misc.count("devfield") &&
      dVariables.misc["devfield"] == "true"){
    devFieldCheckBox->setChecked(true);
    devFieldChecked(true);
    number= getIndex( cInfo, dVariables.misc["devcolour1"]);
    if (number>=0) {
      devColourBox1->setCurrentIndex(number);
    }
    number= getIndex( cInfo, dVariables.misc["devcolour2"]);
    if (number>=0) {
      devColourBox2->setCurrentIndex(number);
    }
  }

  //orient
  if (dVariables.misc.count("orientation") &&
      dVariables.misc["orientation"] == "horizontal"){
    orientCheckBox ->setChecked(true);
  }

  //alignment
  if (dVariables.misc.count("alignment") &&
      dVariables.misc["alignment"] == "right"){
    alignmentCheckBox ->setChecked(true);
  }

  //showpos
  if (dVariables.misc.count("showpos") &&
      dVariables.misc["showpos"] == "true"){
    showposCheckBox ->setChecked(true);
  }

  //criteria
  if (dVariables.misc.count("criteria") &&
      dVariables.misc["criteria"] == "true"){
    criteriaCheckBox ->setChecked(true);
  }

  //level
  if (verticalLevels && dVariables.misc.count("level") && levelMap.count(dVariables.misc["level"])) {
    number = levelMap[dVariables.misc["level"]];
    pressureComboBox->setCurrentIndex(number);
  }

  //density
  if (dVariables.misc.count("density")){
    if(dVariables.misc["density"]=="allobs") {
      allObs=true;
      number= maxdensity;
    } else {
      scalednumber= atof(dVariables.misc["density"].c_str());
      number= int(scalednumber/scaledensity + 0.5);
      allObs=false;
    }
  } else {
    number= int(1/scaledensity + 0.5);
    allObs=false;
  }
  densitySlider->setValue(number);
  displayDensity(number);

  //scale
  if (dVariables.misc.count("scale") ){
    scalednumber= atof(dVariables.misc["scale"].c_str());
  }else{
    scalednumber = 1.0;
  }
  number= int(scalednumber/scalesize + 0.5);
  sizeSlider->setValue(number);
  displaySize(number);

  //timediff
  int i=0;
  int maxSliderValue = 13;
  int sliderStep = 15;
  number = 60/sliderStep;
  if (dVariables.misc.count("timediff") ){
    timediff_minutes= dVariables.misc["timediff"];
    if( timediff_minutes == "alltimes"){
      number=time_slider2lcd.size();
    } else {
      int iminutes = atoi(timediff_minutes.c_str());
      if(iminutes<3*60){
        i=0;
        maxSliderValue = 13;
        sliderStep = 15;
      } else if(iminutes<24*60){
        i=1;
        maxSliderValue = 25;
        sliderStep = 60;
      } else {
        i=2;
        maxSliderValue = 8;
        sliderStep = 60*24;
      }
      number = iminutes/sliderStep;
      if(number>maxSliderValue-1) number = maxSliderValue-1;
    }
  }
  diffComboBox->setCurrentIndex(i);
  diffSlider->setRange(0,maxSliderValue);
  time_slider2lcd.clear();
  for(int i=0;i<maxSliderValue;i++)
    time_slider2lcd.push_back(i*sliderStep);
  diffSlider->setValue(number);
  displayDiff(number);

  //onlypos
  if (dVariables.misc.count("onlypos") &&
      dVariables.misc["onlypos"] == "true"){
    onlyposCheckBox ->setChecked(true);
    onlyposChecked(true);
  }

  //Image
  if (dVariables.misc.count("image") ){
    number = getIndex(markerName,dVariables.misc["image"]);
    if(number>=0)
      markerBox->setCurrentIndex(number);
  }

  //priority
  m= priorityList.size();
  j= 0;
  if(dVariables.misc.count("priority")){
    while (j<m && dVariables.misc["priority"]!=priorityList[j].file) j++;
    if (j<m) {
      pri_selected= ++j;
      pribox->setCurrentIndex(pri_selected);
    } else {
      pribox->setCurrentIndex(0);
    }
  } else {
    pribox->setCurrentIndex(0);
  }

  if (pribox->currentText() == tr("No priority list")) {
    pricheckbox->setEnabled(false);
  } else {
    pricheckbox->setEnabled(true);
  }

  // show only prioritized
  if (dVariables.misc.count("showonlyprioritized")
      && dVariables.misc["showonlyprioritized"] == "true") {
    pricheckbox->setChecked(true);
  }

  //Sort Criteria
  if(dVariables.misc.count("sort")){
    std::vector<std::string> sc = miutil::split(dVariables.misc["sort"], 0, ",");
    int index = -1;
    for(unsigned int i=0; i<button.size(); i++) {
      if (sc[0]== button[i].name) {
        index = i+1;
        break;
      }
    }
    if ( index != -1 ) { // -1 for not found
      sortBox->setCurrentIndex(index);
    } else {
      sortBox->setCurrentIndex(0);
    }
    if(sc.size() > 1 && sc[1] == "desc") {
      descsortButton->setChecked(true);
    } else {
      ascsortButton->setChecked(true);
    }
  } else {
    sortBox->setCurrentIndex(0);
    ascsortButton->setChecked(true);
  }

  //colour
  if (dVariables.misc.count("colour") ){
    number= getIndex( cInfo, dVariables.misc["colour"]);
    if (number>=0)
      colourBox->setCurrentIndex(number);
  }
}

void ObsWidget::decodeString(const miutil::KeyValue_v& kvs, dialogVariables& var, bool fromLog)
{
  for (const miutil::KeyValue& kv : kvs) {
    if (kv.hasValue()) {
      if (kv.key() == "plot" ){
        if (kv.value() == "Enkel" )
          var.plotType = "List";
        else if (kv.value() == "Trykk" )
          var.plotType = "Pressure";
        else
          var.plotType = kv.value();
      } else if (kv.key() == "data" ){
        var.data = miutil::split(kv.value(), 0, ",");
      } else if (kv.key() == "parameter" ){
        var.parameter = miutil::split(kv.value(), 0, ",");
      } else if (kv.key() == "criteria" ){
        if(!fromLog){
          var.misc[kv.key()]="true";
          std::string ss = kv.value();
          miutil::replace(ss, ',', ' ');
          saveCriteria(miutil::split(ss, 0, ";"),"");
        } else {
          std::string ss = kv.value();
          miutil::replace(ss, ',', ' ');
          vector<std::string> vstr = miutil::split(ss, 0, ";");
          if(vstr.size()>1){
            std::string name=vstr[0];
            vstr.erase(vstr.begin());
            saveCriteria(vstr,name);
          } else {
            saveCriteria(miutil::split(kv.value(), ","));
          }
        }
      } else {
        var.misc[kv.key()] = kv.value();
      }
    }
  }
}

void ObsWidget::setFalse()
{
  datatypeButtons->NONEClicked();
  //   parameterButtons->NONEClicked();
  if(parameterButtons)
    parameterButtons->setEnabled(false);

  devFieldCheckBox->setChecked(false);
  devFieldChecked(false);

  tempPrecisionCheckBox->setChecked(false);

  unit_msCheckBox->setChecked(false);

  parameterNameCheckBox->setChecked(false);

  qualityCheckBox->setChecked(false);

  wmoCheckBox->setChecked(false);

  orientCheckBox->setChecked(false);

  alignmentCheckBox->setChecked(false);

  showposCheckBox->setChecked(false);

  onlyposCheckBox->setChecked(false);
  onlyposChecked(false);

  criteriaCheckBox->setChecked(false);
  criteriaChecked(false);

  popupWindowCheckBox->setChecked(false);

  if (verticalLevels) {
    pressureComboBox->setCurrentIndex(0);
  }
}

void ObsWidget::extensionSlot(bool on)
{
  if(on)
    criteriaCheckBox->setChecked(true);
  Q_EMIT extensionToggled(on);
}

ObsDialogInfo::CriteriaList ObsWidget::getCriteriaList()
{
  if(criteriaList.size()==0 || currentCriteria < 0){
    ObsDialogInfo::CriteriaList cl;
    return cl;
  }

  savedCriteria = criteriaList[currentCriteria];
  return criteriaList[currentCriteria];
}

bool ObsWidget::setCurrentCriteria(int i)
{
  if (i < int(criteriaList.size())) {
    currentCriteria=i;
    return true;
  }

  return false;
}

void ObsWidget::saveCriteria(const vector<std::string>& vstr)
{
  savedCriteria.criteria = vstr;
}

bool ObsWidget::saveCriteria(const vector<std::string>& vstr,
    const std::string& name)
{
  //don't save list whithout name
  if (name.empty()) {
    saveCriteria(vstr);
  }

  //find list
  int n = criteriaList.size();
  int i=0;
  while(i<n && criteriaList[i].name != name) i++;

  //list not found, make new list
  if(i==n){
    if(vstr.size()==0) return false;  //no list
    //    METLIBS_LOG_DEBUG("Ny liste:"<<name);
    ObsDialogInfo::CriteriaList clist;
    clist.name=name;
    clist.criteria=vstr;
    criteriaList.push_back(clist);
    return true; //new item
  }

  //list found
  if(vstr.size()==0) {  //delete list
    criteriaList.erase(criteriaList.begin()+i);
    return false;
  }

  //change list
  criteriaList[i].criteria =vstr;
  return false; //no new item
}

bool ObsWidget::getCriteriaLimits(const std::string& name, int& low, int&high)
{
  int n = button.size();
  for(int i=0; i<n; i++)
    if(button[i].name  == name){
      low = button[i].low;
      high = button[i].high;
      if(high==low) return false;
      return true;
    }

  if (name == "dd") {
    low = 0;
    high = 360;
  } else if (name == "ff") {
    low = 0;
    high = 100;
  } else if (name == "lat") {
    low = -90;
    high = 90;
  } else if (name == "lon") {
    low = -180;
    high = 180;
  } else {
    return false;
  }
  return true;
}

vector<std::string> ObsWidget::getCriteriaNames()
{
  vector<std::string> critName;
  critName.reserve(criteriaList.size());
  for(unsigned int i=0; i<criteriaList.size(); i++)
    critName.push_back(criteriaList[i].name);
  return critName;
}
